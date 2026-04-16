# `EXT_sky`

## Overview

`EXT_sky` stores a pointer to an equirectangular sky texture for skybox rendering.

## Sky Texture

The `sky_texture` property is added to the root glTF object. It contains the index of the texture used as the skybox.

```json
{
    "extensions": {
        "EXT_sky": {
            "sky_texture": 0
        }
    }
}
```

## Exporter Behavior

- The Blender exporter reads the active world node tree.
- The first reachable `Environment Texture` node connected to the active `World Output` surface is exported.
- The referenced texture is written at the glTF root and `EXT_sky.sky_texture` points to its index.

## Limitations

- Only the active world environment texture is exported.
- The extension stores a texture reference only. It does not describe sky intensity, rotation, or procedural world settings.
