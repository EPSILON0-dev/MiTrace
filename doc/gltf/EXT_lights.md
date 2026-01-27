# EXT\_lights

## Overview

**This extension is based on the `KHR_lights_punctual` extension.** 
_This file only specifies the additions in relations to the `KHR_lights_punctual` extension._

## Light Types

### Area

Area light is a rectangle that emits light, it's size can be defined with the `area_size` property as below:
```json
{
    "color": [
        1.0,
        1.0,
        1.0
    ],
    "area_size": [ 1.0, 0.5 ],
    "type": "area"
}
```
The property **IS** affected by the node transform.

### Directional

The `angle` property was added, it states how much the light ray can bend from the original direction to still be affected by the directional light. The property unit is radians.

```json
{
    "color": [
        1.0,
        1.0,
        1.0
    ],
    "angle": 0.1,
    "type": "directional"
}
```

### Point

The `size` property was added, it states how much the light ray can be offset from the original center of the point light to still be affected by it. 

```json
{
    "color": [
        1.0,
        1.0,
        1.0
    ],
    "size": 0.1,
    "type": "point"
}
```
The property **IS** affected by the node transform.

### Spot

The `size` property was added that functions equivalently to the one in the point light.
