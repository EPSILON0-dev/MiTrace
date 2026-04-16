# BlenderBaker Extension Docs

This folder documents the custom glTF extensions emitted by BlenderBaker.

Included extensions:

- `EXT_light_attributes`
- `EXT_baked_lighting`
- `EXT_light_probes`

Excluded here:

- `KHR_lights_punctual`, because it is an official Khronos extension rather than a BlenderBaker-specific one.

Notes:

- These docs describe the exporter behavior implemented in `BlenderBaker.cs`.
- Properties marked optional are omitted when the exporter has no value to write.
- Document-level and node-level usage are described separately where applicable.