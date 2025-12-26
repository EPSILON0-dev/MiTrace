# Material Spheres

This scene tests various material properties using spheres with different materials applied to them.

## Adding a Skybox

The `add_sky.py` script allows you to add a skybox to a glTF file using the `EXT_sky` extension.

### Usage

Run the script with the following arguments:

```bash
./add_sky.py <input_file> <output_file>
```

- `<input_file>`: Path to the input glTF file.
- `<output_file>`: Path to the output glTF file.

### Example

```bash
./add_sky.py material_spheres_raw.gltf material_spheres.gltf
```
