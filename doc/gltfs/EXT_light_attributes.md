# `EXT_light_attributes`

## Purpose

`EXT_light_attributes` is MiTrace's custom light metadata extension.

It is used in three places:

- On punctual light definitions inside `KHR_lights_punctual`, to carry light metadata that glTF does not store natively.
- At the glTF document level, to store exported area lights, which are not represented by `KHR_lights_punctual`.
- On glTF nodes that reference a document-level exported area light.

## Where It Appears

### 1. Punctual light object extension

Attached to an item inside `extensions.KHR_lights_punctual.lights[]`.

```json
{
  "extensions": {
    "EXT_light_attributes": {
      "castsShadows": true,
      "temperature": 6500.0,
      "radius": 0.0
    }
  }
}
```

### 2. Document-level area light collection

Attached to the top-level glTF `extensions` object.

```json
{
  "extensions": {
    "EXT_light_attributes": {
      "areaLights": [
        {
          "name": "Area Light",
          "shape": "rectangle",
          "color": [1.0, 1.0, 1.0],
          "intensity": 10000.0,
          "temperature": 6500.0,
          "radius": 0.0,
          "castsShadows": true,
          "width": 2.0,
          "height": 1.0
        }
      ]
    }
  }
}
```

### 3. Node-level area light reference

Attached to a node representing the light transform.

```json
{
  "extensions": {
    "EXT_light_attributes": {
      "areaLight": 0
    }
  }
}
```

## Schema

### Punctual-light payload

```json
{
  "castsShadows": true,
  "temperature": 6500.0,
  "radius": 0.0
}
```

Fields:

- `castsShadows`: `bool`, always written.
- `temperature`: `float`, only written when the exporter light temperature toggle is enabled.
- `radius`: `float`, only written when a non-zero MiTrace radius value is configured on the Blender light.

### Document-level payload

```json
{
  "areaLights": [
    {
      "name": "Area Light",
      "shape": "rectangle",
      "color": [1.0, 1.0, 1.0],
      "intensity": 10000.0,
      "temperature": 6500.0,
      "radius": 0.0,
      "castsShadows": true,
      "width": 2.0,
      "height": 1.0
    }
  ]
}
```

`areaLights[]` fields:

- `name`: `string`, optional.
- `shape`: `string`, required. Exporter currently writes `rectangle` or `disc`.
- `color`: `float[3]`, optional RGB.
- `intensity`: `float`, optional. Exported from Blender light energy.
- `temperature`: `float`, optional.
- `radius`: `float`, optional.
- `castsShadows`: `bool`, always written.
- `width`: `float`, optional.
- `height`: `float`, optional.

### Node-level payload

```json
{
  "areaLight": 0
}
```

Fields:

- `areaLight`: `int`, index into `extensions.EXT_light_attributes.areaLights`.

## Exporter Behavior

- Directional, point, and spot lights are exported through `KHR_lights_punctual` and receive this extension as extra metadata.
- Blender `AREA` lights are exported to the document-level `areaLights[]` payload.
- Area light shape is mapped from Blender area light shapes: `RECTANGLE` and `SQUARE` become `rectangle`, while `DISK` and `ELLIPSE` become `disc`.
- Area light size is read from Blender's area light dimensions.
- Radius is exported from the MiTrace light properties panel when configured.
- Temperature is exported from the MiTrace light properties panel when enabled.
- Area lights are stored once at the document level, then referenced by index from the node.
- `EXT_light_attributes` is added to `extensionsUsed` whenever any punctual or area light using this extension is exported.

## Limitations

- The extension is MiTrace-specific and not part of the standard glTF ecosystem.
- Only `rectangle` and `disc` shapes are currently emitted.
