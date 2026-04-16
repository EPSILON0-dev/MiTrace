# MiTrace GLTF Exporter

This directory contains a Blender add-on scaffold that plugs into Blender's built-in glTF 2.0 exporter.

## Add-on

- Package: `mitrace_gltf_exporter`
- Display name: `MiTrace GLTF Exporter`

## What It Exports

- `EXT_light_attributes`
  - Punctual light metadata on `KHR_lights_punctual.lights[]`
  - Document-level `areaLights[]` payload for Blender area lights
  - Node-level `areaLight` references for area light transforms
- `EXT_sky`
  - Root-level `sky_texture` pointer for the active world environment texture

## Installation

1. In Blender, open `Edit > Preferences > Add-ons`.
2. Use `Install...` and select the `mitrace_gltf_exporter` directory as a zip or package.
3. Enable `MiTrace GLTF Exporter`.
4. Export through `File > Export > glTF 2.0`.

## Notes

- The add-on relies on Blender's stock `io_scene_gltf2` add-on.
- `EXT_sky` reads the active world node tree and exports the first reachable environment texture.
- `EXT_light_attributes` values are configured on each light in the Light data properties panel.
