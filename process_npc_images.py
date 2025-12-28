
from PIL import Image
import os

def remove_background(image_path, output_path, tolerance=30):
    try:
        img = Image.open(image_path)
        img = img.convert("RGBA")
        datas = img.getdata()

        newData = []
        # Get the color of the top-left pixel to use as background color
        bgColor = datas[0]
        
        # Simple thresholding logic
        for item in datas:
            # Check if pixel is close to background color (using simplified distance)
            if abs(item[0] - bgColor[0]) < tolerance and \
               abs(item[1] - bgColor[1]) < tolerance and \
               abs(item[2] - bgColor[2]) < tolerance:
                newData.append((255, 255, 255, 0)) # Transparent
            else:
                newData.append(item)

        img.putdata(newData)
        
        # Ensure output directory exists
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        img.save(output_path, "PNG")
        print(f"Successfully processed: {image_path} -> {output_path}")
        return True
    except Exception as e:
        print(f"Failed to process {image_path}: {e}")
        return False

# Base path
base_dir = r"f:/123/Sigeluguanshou/Resources/NPC"
input_files = [
    ("blacksmith.png", "blacksmith_processed.png"),
    ("bussiness_person.png", "bussiness_person_processed.png"),
    ("bussiness_head.png", "bussiness_head_processed.png")
]

for input_name, output_name in input_files:
    in_path = os.path.join(base_dir, input_name)
    out_path = os.path.join(base_dir, output_name)
    # Check if file exists, if not try absolute path if provided differently
    if not os.path.exists(in_path):
        print(f"Input file not found: {in_path}")
        continue
        
    remove_background(in_path, out_path)
