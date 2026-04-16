bl_info = {
    "name": "MiTrace GLTF Exporter",
    "author": "GitHub Copilot",
    "version": (0, 1, 0),
    "blender": (3, 6, 0),
    "location": "File > Export > glTF 2.0",
    "description": "Adds MiTrace-specific EXT_light_attributes and EXT_sky glTF export support.",
    "category": "Import-Export",
}

import bpy


ADDON_NAME = "MiTrace GLTF Exporter"
EXT_LIGHT_ATTRIBUTES = "EXT_light_attributes"
EXT_SKY = "EXT_sky"
EXTENSION_REQUIRED = False


class MiTraceExportSettings(bpy.types.PropertyGroup):
    enabled: bpy.props.BoolProperty(
        name=ADDON_NAME,
        description="Enable MiTrace extension export hooks for glTF exports.",
        default=True,
    )
    export_light_attributes: bpy.props.BoolProperty(
        name="Export EXT_light_attributes",
        description="Export MiTrace light metadata on punctual and area lights.",
        default=True,
    )
    export_sky: bpy.props.BoolProperty(
        name="Export EXT_sky",
        description="Export the active world environment texture as EXT_sky.",
        default=True,
    )


class MiTraceLightProperties(bpy.types.PropertyGroup):
    casts_shadows: bpy.props.BoolProperty(
        name="Casts Shadows",
        description="Whether the exported light should report shadow casting.",
        default=True,
    )
    use_temperature: bpy.props.BoolProperty(
        name="Use Temperature",
        description="Include color temperature metadata in EXT_light_attributes.",
        default=False,
    )
    temperature: bpy.props.FloatProperty(
        name="Temperature",
        description="Color temperature stored in EXT_light_attributes.",
        default=6500.0,
        min=0.0,
        soft_min=1000.0,
        soft_max=40000.0,
    )
    radius: bpy.props.FloatProperty(
        name="Radius",
        description="Shape radius stored in EXT_light_attributes.",
        default=0.0,
        min=0.0,
    )


class MITRACE_PT_light_properties(bpy.types.Panel):
    bl_label = "MiTrace GLTF"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "data"

    @classmethod
    def poll(cls, context):
        return context.light is not None

    def draw(self, context):
        layout = self.layout
        props = context.light.mitrace_gltf
        layout.use_property_split = True
        layout.prop(props, "casts_shadows")
        layout.prop(props, "radius")
        layout.prop(props, "use_temperature")
        column = layout.column()
        column.enabled = props.use_temperature
        column.prop(props, "temperature")


class MITRACE_PT_world_properties(bpy.types.Panel):
    bl_label = "MiTrace GLTF"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "world"

    @classmethod
    def poll(cls, context):
        return context.world is not None

    def draw(self, context):
        layout = self.layout
        settings = context.scene.mitrace_gltf_export
        layout.use_property_split = True
        layout.label(text="EXT_sky uses the active world environment texture.")
        layout.prop(settings, "export_sky")


class MITRACE_PT_export_panel(bpy.types.Panel):
    bl_label = ADDON_NAME
    bl_space_type = "FILE_BROWSER"
    bl_region_type = "TOOL_PROPS"
    bl_parent_id = "FILE_PT_operator"
    bl_options = {"DEFAULT_CLOSED"}

    @classmethod
    def poll(cls, context):
        operator = context.space_data.active_operator
        return operator is not None and operator.bl_idname == "EXPORT_SCENE_OT_gltf"

    def draw(self, context):
        draw_export(context, self.layout)


class glTF2ExportUserExtension:
    def __init__(self):
        from io_scene_gltf2.io.com import gltf2_io
        from io_scene_gltf2.io.com.constants import TextureFilter, TextureWrap
        from io_scene_gltf2.io.com.gltf2_io_extensions import Extension
        from io_scene_gltf2.blender.exp.material.image import get_gltf_image_from_blender_image

        self.gltf2_io = gltf2_io
        self.TextureFilter = TextureFilter
        self.TextureWrap = TextureWrap
        self.Extension = Extension
        self.get_gltf_image_from_blender_image = get_gltf_image_from_blender_image
        self.settings = bpy.context.scene.mitrace_gltf_export
        self.is_critical = False
        self._area_lights = []
        self._area_light_lookup = {}
        self._sky_texture = None

    def gather_node_hook(self, gltf2_node, blender_object, export_settings):
        if not self.settings.enabled or blender_object is None or blender_object.type not in {"LIGHT", "LAMP"}:
            return

        light = blender_object.data
        if self.settings.export_light_attributes:
            if light.type in {"SUN", "POINT", "SPOT"}:
                self._attach_punctual_extension(gltf2_node, light)
            elif light.type == "AREA":
                area_index = self._ensure_area_light(blender_object)
                if area_index is not None:
                    self._attach_node_extension(
                        gltf2_node,
                        EXT_LIGHT_ATTRIBUTES,
                        {"areaLight": area_index},
                    )

    def gather_gltf_extensions_hook(self, gltf2_plan, export_settings):
        if not self.settings.enabled:
            return

        if self.settings.export_light_attributes and self._area_lights:
            if gltf2_plan.extensions is None:
                gltf2_plan.extensions = {}
            gltf2_plan.extensions[EXT_LIGHT_ATTRIBUTES] = self.Extension(
                name=EXT_LIGHT_ATTRIBUTES,
                extension={"areaLights": self._area_lights},
                required=EXTENSION_REQUIRED,
            )

        if self.settings.export_sky:
            sky_texture = self._ensure_sky_texture(export_settings)
            if sky_texture is not None:
                if gltf2_plan.extensions is None:
                    gltf2_plan.extensions = {}
                gltf2_plan.extensions[EXT_SKY] = self.Extension(
                    name=EXT_SKY,
                    extension={"sky_texture": sky_texture},
                    required=EXTENSION_REQUIRED,
                )

    def _attach_punctual_extension(self, gltf2_node, light):
        node_extensions = getattr(gltf2_node, "extensions", None) or {}
        punctual_extension = node_extensions.get("KHR_lights_punctual")
        if punctual_extension is None:
            return

        light_ref = getattr(punctual_extension, "extension", {}).get("light")
        punctual_payload = getattr(light_ref, "extension", None)
        if punctual_payload is None:
            return

        extension_payload = self._build_punctual_payload(light)
        if not extension_payload:
            return

        extensions = getattr(punctual_payload, "extensions", None)
        if extensions is None:
            punctual_payload.extensions = {}
            extensions = punctual_payload.extensions

        extensions[EXT_LIGHT_ATTRIBUTES] = self.Extension(
            name=EXT_LIGHT_ATTRIBUTES,
            extension=extension_payload,
            required=EXTENSION_REQUIRED,
        )

    def _attach_node_extension(self, gltf2_node, extension_name, payload):
        if gltf2_node.extensions is None:
            gltf2_node.extensions = {}
        gltf2_node.extensions[extension_name] = self.Extension(
            name=extension_name,
            extension=payload,
            required=EXTENSION_REQUIRED,
        )

    def _build_punctual_payload(self, light):
        props = light.mitrace_gltf
        payload = {
            "castsShadows": bool(props.casts_shadows),
        }
        if props.use_temperature:
            payload["temperature"] = float(props.temperature)
        if props.radius > 0.0:
            payload["radius"] = float(props.radius)
        return payload

    def _ensure_area_light(self, blender_object):
        key = blender_object.name_full
        if key in self._area_light_lookup:
            return self._area_light_lookup[key]

        payload = self._build_area_light_payload(blender_object)
        if payload is None:
            return None

        index = len(self._area_lights)
        self._area_lights.append(payload)
        self._area_light_lookup[key] = index
        return index

    def _build_area_light_payload(self, blender_object):
        light = blender_object.data
        props = light.mitrace_gltf
        shape = self._map_area_shape(light.shape)
        if shape is None:
            return None

        payload = {
            "name": blender_object.name,
            "shape": shape,
            "color": [float(channel) for channel in light.color[:3]],
            "intensity": float(light.energy),
            "castsShadows": bool(props.casts_shadows),
        }

        if props.use_temperature:
            payload["temperature"] = float(props.temperature)
        if props.radius > 0.0:
            payload["radius"] = float(props.radius)

        width, height = self._area_dimensions(light)
        if width is not None:
            payload["width"] = float(width)
        if height is not None:
            payload["height"] = float(height)

        return payload

    def _map_area_shape(self, shape):
        mapping = {
            "RECTANGLE": "rectangle",
            "SQUARE": "rectangle",
            "DISK": "disc",
            "ELLIPSE": "disc",
        }
        return mapping.get(shape)

    def _area_dimensions(self, light):
        if light.shape == "RECTANGLE":
            return light.size, light.size_y
        if light.shape == "ELLIPSE":
            return light.size, light.size_y
        if light.shape in {"SQUARE", "DISK"}:
            return light.size, light.size
        return None, None

    def _ensure_sky_texture(self, export_settings):
        if self._sky_texture is not None:
            return self._sky_texture

        world = bpy.context.scene.world
        image_node = self._find_active_world_environment(world)
        if image_node is None or image_node.image is None:
            return None

        image = self.get_gltf_image_from_blender_image(image_node.image.name, export_settings)
        sampler = self.gltf2_io.Sampler(
            extensions=None,
            extras=None,
            mag_filter=self.TextureFilter.Linear,
            min_filter=self.TextureFilter.LinearMipmapLinear,
            name=f"{image_node.image.name}_sampler",
            wrap_s=self._texture_wrap(image_node.extension),
            wrap_t=self._texture_wrap(image_node.extension),
        )
        self._sky_texture = self.gltf2_io.Texture(
            extensions=None,
            extras=None,
            name=f"{image_node.image.name}_sky",
            sampler=sampler,
            source=image,
        )
        return self._sky_texture

    def _find_active_world_environment(self, world):
        if world is None or not world.use_nodes or world.node_tree is None:
            return None

        output = next(
            (
                node
                for node in world.node_tree.nodes
                if node.bl_idname == "ShaderNodeOutputWorld" and node.is_active_output
            ),
            None,
        )
        if output is None:
            return None

        surface_input = output.inputs.get("Surface")
        if surface_input is None:
            return None

        return self._find_environment_texture_node(surface_input)

    def _find_environment_texture_node(self, socket, visited=None):
        if socket is None:
            return None
        if visited is None:
            visited = set()

        for link in socket.links:
            from_socket = link.from_socket
            node = from_socket.node
            key = (node.name, from_socket.identifier)
            if key in visited:
                continue
            visited.add(key)

            if node.bl_idname == "ShaderNodeTexEnvironment":
                return node

            for input_socket in getattr(node, "inputs", []):
                result = self._find_environment_texture_node(input_socket, visited)
                if result is not None:
                    return result

        return None

    def _texture_wrap(self, extension):
        mapping = {
            "EXTEND": self.TextureWrap.ClampToEdge,
            "MIRROR": self.TextureWrap.MirroredRepeat,
            "CLIP": self.TextureWrap.ClampToEdge,
        }
        return mapping.get(extension, self.TextureWrap.Repeat)


def draw_export(context, layout):
    header, body = layout.panel("MITRACE_gltf_exporter", default_closed=False)
    header.use_property_split = False

    props = bpy.context.scene.mitrace_gltf_export
    header.prop(props, "enabled")
    if body is not None:
        body.prop(props, "export_light_attributes")
        body.prop(props, "export_sky")


def register():
    bpy.utils.register_class(MiTraceExportSettings)
    bpy.utils.register_class(MiTraceLightProperties)
    bpy.utils.register_class(MITRACE_PT_light_properties)
    bpy.utils.register_class(MITRACE_PT_world_properties)
    bpy.utils.register_class(MITRACE_PT_export_panel)

    bpy.types.Scene.mitrace_gltf_export = bpy.props.PointerProperty(type=MiTraceExportSettings)
    bpy.types.Light.mitrace_gltf = bpy.props.PointerProperty(type=MiTraceLightProperties)

    from io_scene_gltf2 import exporter_extension_layout_draw

    exporter_extension_layout_draw[ADDON_NAME] = draw_export


def unregister():
    from io_scene_gltf2 import exporter_extension_layout_draw

    if ADDON_NAME in exporter_extension_layout_draw:
        del exporter_extension_layout_draw[ADDON_NAME]

    del bpy.types.Light.mitrace_gltf
    del bpy.types.Scene.mitrace_gltf_export

    bpy.utils.unregister_class(MITRACE_PT_export_panel)
    bpy.utils.unregister_class(MITRACE_PT_world_properties)
    bpy.utils.unregister_class(MITRACE_PT_light_properties)
    bpy.utils.unregister_class(MiTraceLightProperties)
    bpy.utils.unregister_class(MiTraceExportSettings)
