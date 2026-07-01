#!/bin/python3

from json import load, dump


def add_light_to_cornell_box(input_file: str, output_file: str) -> None:
    # Load the existing Cornell box scene
    with open(input_file, 'r') as f:
        scene = load(f)

    # Define a new area light using the documented EXT_light_attributes format
    new_light = {
        "name": "Cornell Area Light",
        "shape": "rectangle",
        "color": [1.0, 1.0, 1.0],
        "intensity": 543.0,
        "castsShadows": True,
        "width": 0.5,
        "height": 0.5,
    }

    # Add to the document-level extension section
    if "extensions" not in scene:
        scene["extensions"] = {}
    if "EXT_light_attributes" not in scene["extensions"]:
        scene["extensions"]["EXT_light_attributes"] = {}
    if "areaLights" not in scene["extensions"]["EXT_light_attributes"]:
        scene["extensions"]["EXT_light_attributes"]["areaLights"] = []
    scene["extensions"]["EXT_light_attributes"]["areaLights"].append(new_light)

    if "extensionsUsed" not in scene:
        scene["extensionsUsed"] = []
    if "EXT_light_attributes" not in scene["extensionsUsed"]:
        scene["extensionsUsed"].append("EXT_light_attributes")

    # Find the Light_0 node and add a reference to the new light
    for node in scene["nodes"]:
        if node.get("name") == "Light_0":
            if "extensions" not in node:
                node["extensions"] = {}
            node["extensions"]["EXT_light_attributes"] = {
                "areaLight": len(scene["extensions"]["EXT_light_attributes"]["areaLights"]) - 1
            }
            node["extensions"].pop("KHR_lights_punctual", None)
            break

    # Save the modified scene to a new file
    with open(output_file, 'w') as f:
        dump(scene, f, indent=4)

if __name__ == "__main__":
    add_light_to_cornell_box("cornell_box_raw.gltf", "cornell_box.gltf")
