#!/usr/bin/env python3

"""
    This script adds a skybox to a glTF file using the EXT_sky extension. It modifies the glTF 
    file to include a new texture, sampler, and image for the skybox.
"""

from json import load, dump

def add_skybox_to_gltf(input_file: str, output_file: str, image_uri: str) -> None:
    """
    Adds a skybox to a glTF file using the EXT_sky extension.

    Args:
        input_file (str): Path to the input glTF file.
        output_file (str): Path to the output glTF file.
        image_uri (str): URI of the image to use as the skybox.
    """
    # Load the existing glTF scene
    with open(input_file, 'r') as f:
        scene = load(f)

    # Ensure required sections exist
    scene.setdefault("images", [])
    scene.setdefault("samplers", [])
    scene.setdefault("textures", [])

    # Add the new image
    new_image = {"uri": image_uri}
    scene["images"].append(new_image)
    image_index = len(scene["images"]) - 1

    # Add the new sampler (linear-repeat)
    new_sampler = {
        "magFilter": 9729,  # LINEAR
        "minFilter": 9729,  # LINEAR
        "wrapS": 10497,     # REPEAT
        "wrapT": 10497      # REPEAT
    }
    scene["samplers"].append(new_sampler)
    sampler_index = len(scene["samplers"]) - 1

    # Add the new texture
    new_texture = {
        "sampler": sampler_index,
        "source": image_index
    }
    scene["textures"].append(new_texture)
    texture_index = len(scene["textures"]) - 1

    # Add the EXT_sky extension
    if "extensions" not in scene:
        scene["extensions"] = {}
    if "EXT_sky" not in scene["extensions"]:
        scene["extensions"]["EXT_sky"] = {}

    # Set the sky_texture property
    scene["extensions"]["EXT_sky"]["sky_texture"] = texture_index

    # Save the modified scene to a new file
    with open(output_file, 'w') as f:
        dump(scene, f)

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Add a skybox to a glTF file using the EXT_sky extension.")
    parser.add_argument("input_file", type=str, help="Path to the input glTF file.")
    parser.add_argument("output_file", type=str, help="Path to the output glTF file.")
    parser.add_argument("image_uri", type=str, help="URI of the image to use as the skybox.")

    args = parser.parse_args()
    add_skybox_to_gltf(args.input_file, args.output_file, args.image_uri)
