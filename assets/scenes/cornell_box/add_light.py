#!/bin/python3

from json import load, dump

def add_light_to_cornell_box(input_file: str, output_file: str) -> None:
    # Load the existing Cornell box scene
    with open(input_file, 'r') as f:
        scene = load(f)

    # Define a new light source
    new_light = {
        "type": "area",
        "area_size": [0.5, 0.5],
        "color": [1.0, 1.0, 1.0],
        "intensity": 10.0,
    }

    # Add to the extensions section
    if "extensions" not in scene:
        scene["extensions"] = {}
    if "KHR_lights_punctual" not in scene["extensions"]:
        scene["extensions"]["KHR_lights_punctual"] = {}
    if "lights" not in scene["extensions"]["KHR_lights_punctual"]:
        scene["extensions"]["KHR_lights_punctual"]["lights"] = []
    scene["extensions"]["KHR_lights_punctual"]["lights"].append(new_light)

    # Find the Light_0 node and add a reference to the new light
    for node in scene["nodes"]:
        if node.get("name") == "Light_0":
            if "extensions" not in node:
                node["extensions"] = {}
            node["extensions"]["KHR_lights_punctual"] = {
                "light": len(scene["extensions"]["KHR_lights_punctual"]["lights"]) - 1
            }
            break

    # Save the modified scene to a new file
    with open(output_file, 'w') as f:
        dump(scene, f)

if __name__ == "__main__":
    add_light_to_cornell_box("cornell_box_raw.gltf", "cornell_box.gltf")
