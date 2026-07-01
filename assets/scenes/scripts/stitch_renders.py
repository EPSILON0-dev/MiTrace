from PIL import Image, ImageDraw, ImageFont
import math
import os

RENDERS = [
    ("primary-bvh-tests", "Primary BVH Tests"),
    ("total-bvh-tests", "Total BVH Tests"),
    ("color-per-mesh", "Meshes"),
    ("color-per-triangle", "Triangles"),
    ("color-per-bounding-volume", "Bounding Volumes"),
    ("albedo", "Albedo"),
    ("metallic-roughness", "Metallic Roughness"),
    ("geometric-normal", "Geometric Normal"),
    ("shading-normal", "Shading Normal"),
    ("emission", "Emission"),
    ("direct-only", "Direct Only"),
    ("indirect-only", "Indirect Only"),
    ("depth", "Depth"),
    ("bounces", "Bounces"),
    ("reflected-direction", "Reflected Direction"),
    ("first-hit-fresnel", "Fresnel"),
    ("first-hit-brdf", "BRDF"),
    ("first-hit-pdf", "PDF"),
    ("pixel-standard-deviation", "Pixel stddev"),
    ("firefly-elimination", "Fireflies"),
]

LABEL_MARGIN = 5
LABEL_PADDING = 5
FONT_SIZE = 12
FONT_PATH = "DejaVuSansMono.ttf"


def found_all_renders() -> bool:

    images = set()
    for (render, _) in RENDERS:
        image_path = f"render-{render}.png"
        if os.path.exists(image_path):
            images.add(image_path)
    return len(images) == len(RENDERS)


def open_all_images() -> list[Image.Image]:
    images = []
    for (render, _) in RENDERS:
        image_path = f"render-{render}.png"
        if os.path.exists(image_path):
            images.append(Image.open(image_path))
    return images


def create_image_grid(images, cols, padding=0, bg_color=(0, 0, 0)):
    if not images:
        return None

    try:
        font = ImageFont.truetype(FONT_PATH, FONT_SIZE)
    except:
        font = ImageFont.load_default()

    w, h = images[0].size

    rows = math.ceil(len(images) / cols)
    grid_w = cols * w + (cols - 1) * padding
    grid_h = rows * h + (rows - 1) * padding
    grid = Image.new("RGB", (grid_w, grid_h), bg_color)

    for i, img in enumerate(images):
        x = (i % cols) * (w + padding)
        y = (i // cols) * (h + padding)
        grid.paste(img, (x, y))

        draw = ImageDraw.Draw(grid)
        bbox = draw.textbbox((0, 0), RENDERS[i][1], font=font)
        text_w = bbox[2] - bbox[0]
        text_h = bbox[3] - bbox[1]
        text_x = x + LABEL_PADDING
        text_y = y + h - text_h - LABEL_PADDING

        draw.rectangle(
            (
                text_x - LABEL_MARGIN,
                text_y - LABEL_MARGIN,
                text_x + text_w + LABEL_MARGIN * 2,
                text_y + text_h + LABEL_MARGIN * 2,
            ),
            fill=(0, 0, 0),
        )
        draw.text((text_x, text_y), RENDERS[i][1], font=font, fill=(255, 255, 255))

    return grid


def main():
    if not found_all_renders():
        print("Not all renders found. Please run the render_debug.sh script first.")
        return

    images = open_all_images()
    grid = create_image_grid(images, cols=5, padding=0)
    grid.save("renders_combined.png")


if __name__ == "__main__":
    main()
