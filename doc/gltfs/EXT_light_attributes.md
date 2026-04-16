# `EXT_light_attributes`

## Purpose

`EXT_light_attributes` is BlenderBaker's custom light metadata extension.

It is used in two places:

- On punctual light definitions inside `KHR_lights_punctual`, to carry Unity-specific metadata that glTF does not store natively.
- At the glTF document level, to store exported Unity area lights, which are not represented by `KHR_lights_punctual`.
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
- `temperature`: `float`, only written when `Light.useColorTemperature` is enabled.
- `radius`: `float`, written when BlenderBaker can derive a shape radius from Unity light data.

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
- `intensity`: `float`, optional. Computed as `light.intensity * 1000 * Light Intensity Multiplier`.
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
- Unity area-like lights are detected by name from `Light.type`: `Area`, `Rectangle`, or `Disc`.
- Area light size is read from Unity's serialized `m_AreaSize` field.
- Radius is read from Unity's serialized `m_ShapeRadius` when available.
- If `m_ShapeRadius` is missing but area size exists, radius falls back to half of the smaller area dimension.
- Area lights are stored once at the document level, then referenced by index from the node.
- `EXT_light_attributes` is added to `extensionsUsed` whenever any punctual or area light using this extension is exported.

## Limitations

- The extension is BlenderBaker-specific and not part of the standard glTF ecosystem.
- Area light support depends on Unity serialized light internals such as `m_AreaSize` and `m_ShapeRadius`.
- Only `rectangle` and `disc` shapes are currently emitted.