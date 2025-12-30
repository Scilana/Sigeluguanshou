
from PIL import Image, ImageDraw
import os

def remove_background_and_resize(image_path, output_path, target_height=100, tolerance=60):
    try:
        img = Image.open(image_path)
        img = img.convert("RGBA")
        
        # Resize first for efficiency and consistency
        width, height = img.size
        scale_ratio = target_height / float(height)
        new_width = int(width * scale_ratio)
        img = img.resize((new_width, target_height), Image.Resampling.LANCZOS)
        
        width, height = img.size
        pixels = img.load()
        
        # Helper to check color difference
        def color_dist(c1, c2):
            return abs(c1[0]-c2[0]) + abs(c1[1]-c2[1]) + abs(c1[2]-c2[2])

        # Get background color from top-left corner
        bg_color = pixels[0, 0]
        
        # Strategy: Global Color Replacement + Flood Fill
        # The merchant image has a grid/mosaic background that might not be fully connected or has "islands".
        # We'll traverse every pixel and if it matches the corner background color, make it transparent.
        
        for y in range(height):
            for x in range(width):
                curr_color = pixels[x, y]
                if color_dist(curr_color, bg_color) < tolerance:
                    pixels[x, y] = (0, 0, 0, 0)
        
        # Ensure output directory exists
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        img.save(output_path, "PNG")
        print(f"Processed: {image_path} -> {output_path} ({new_width}x{target_height})")
        return True
    except Exception as e:
        print(f"Failed to process {image_path}: {e}")
        return False

# Base path
base_dir = r"f:/123/Sigeluguanshou/Resources/NPC"
input_files = [
    ("blacksmith.png", "blacksmith_processed.png"),
    ("merchant_new.png", "bussiness_person_processed.png"),
    ("bussiness_head.png", "bussiness_head_processed.png")
]

for input_name, output_name in input_files:
    in_path = os.path.join(base_dir, input_name)
    out_path = os.path.join(base_dir, output_name)
    if not os.path.exists(in_path):
        print(f"Input file not found: {in_path}")
        continue
        
    # Set target height based on type
    h = 80 # default for map sprites
    if "head" in input_name:
        h = 100 # Portrait can be slightly larger
        
    remove_background_and_resize(in_path, out_path, target_height=h)
