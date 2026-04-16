# MiTrace glTF Extensions

This directory documents the custom glTF extensions used by MiTrace and the Blender exporter scaffold in `scripts/blender/exporter`.

## Extensions

- `EXT_light_attributes`: light metadata for punctual lights and exported area lights.
- `EXT_sky`: root-level pointer to the exported sky texture.

## Blender Exporter

The Blender add-on is named `MiTrace GLTF Exporter` and hooks into Blender's built-in `glTF 2.0` exporter.

It currently exports:

- `EXT_light_attributes` on punctual lights.
- Document-level `areaLights[]` plus node references for Blender area lights.
- `EXT_sky` using the active world environment texture.

## Files

- `EXT_light_attributes.md`
- `EXT_sky.md`
