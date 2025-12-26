# EXT\_sky

## Overview

This extensions adds an ability to store a pointer to a equirectengular sky texture for rendering skyboxes.

## Sky Texture

The `sky_texture` property was added to the root glTF object. It contains the index of the texture used as skybox.

```json
{
    "extensions": {
        "EXT_sky": {
            "sky_texture": 0
        }
    }
}
```
