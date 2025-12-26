#!/usr/bin/env python3
'''
    Can you believe that stock sites take money for this shit?
    Generating a simple checkerboard texture using Python and PIL.
'''

from PIL import Image, ImageDraw

# Image and grid settings
image_size = 256
grid_size = 16
cell_size = image_size // grid_size

white = (255, 255, 255)
gray = (192, 192, 192)

# Create image
img = Image.new("RGB", (image_size, image_size), white)
draw = ImageDraw.Draw(img)

# Draw checkerboard
for y in range(grid_size):
    for x in range(grid_size):
        color = white if (x + y) % 2 == 0 else gray
        x0 = x * cell_size
        y0 = y * cell_size
        x1 = x0 + cell_size
        y1 = y0 + cell_size
        draw.rectangle([x0, y0, x1, y1], fill=color)

# Save image
img.save("checkerboard.png")
