# Cornell box

This scene is meant to test global illumination algorithms and general renderer behaviour. It contains a Cornell box with two boxes inside and an area light on the ceiling.

![Preview from blender](./preview_blender.jpg)

## Note

Unfortunatelly blender is unable to export area lights (since there's no extension for them), thankfully we can add them manually. Run the following command to add them.
```sh
python3 add_light.py
```
