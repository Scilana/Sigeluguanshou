import cv2
import numpy as np
import os

def process_image(input_path, output_path):
    print(f"Processing {input_path}...")
    
    # Load image
    img = cv2.imread(input_path)
    if img is None:
        print("Error: Could not load image")
        return

    # Convert to RGB (OpenCV uses BGR)
    img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

    # Define background color range (Dark Brown background)
    # The background seems to be dark brown/dirt tiles.
    # Let's try to detect the 4 corners to determine background or use a range.
    # Center is green. 
    
    # Strategy:
    # 1. Threshold green color to keep the slime.
    # 2. Or, since the slime is distinct green, we can just mask everything that is NOT green?
    #    Slime has multiple shades of green.
    
    # Alternative: Flood fill from corners?
    # The background is textured, so flood fill might not work perfectly if there is noise.
    # But for a small pixel art, the background blocks might be uniform enough or distinct from green.
    
    # Let's simple filter for Greenish pixels.
    # HSV is better for color formatting.
    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    
    # Slime green range
    # Hue: Green is around 60. Range 40-80?
    # S: High saturation
    # V: Varying value
    
    # Let's be generous with green.
    lower_green = np.array([35, 50, 50])
    upper_green = np.array([85, 255, 255])
    
    mask = cv2.inRange(hsv, lower_green, upper_green)
    
    # Refine mask: morphological ops
    kernel = np.ones((3,3), np.uint8)
    # mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel) # Remove noise
    # mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel) # Close holes
    
    # Find contours to crop
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    if not contours:
        print("Error: No green object found")
        return

    # Find bounding box of the largest green object
    c = max(contours, key=cv2.contourArea)
    x, y, w, h = cv2.boundingRect(c)
    
    # Crop
    # Add a little padding? 1px
    pad = 1
    x = max(0, x - pad)
    y = max(0, y - pad)
    w = min(img.shape[1] - x, w + 2*pad)
    h = min(img.shape[0] - y, h + 2*pad)
    
    cropped = img[y:y+h, x:x+w]
    
    # Make transparent
    # Create RGBA
    b_channel, g_channel, r_channel = cv2.split(cropped)
    
    # Create alpha channel based on the mask (cropped mask)
    cropped_mask = mask[y:y+h, x:x+w]
    
    # Refine alpha: strict masking
    alpha_channel = cropped_mask
    
    rgba = cv2.merge((b_channel, g_channel, r_channel, alpha_channel))
    
    # Resize to 16x16 or 32x32 based on current size?
    # User said "consistent size".
    # Previous slime was small.
    # Let's check the cropped size.
    print(f"Cropped size: {w}x{h}")
    
    # If it's close to 16x16 (e.g. 14x15), pad to 16x16 or keep as is.
    # Usually sprites are 16x16 or 32x32.
    # Let's just save the cropped version. It should be small enough.
    
    # Resize to 24x24 (half of ~48)
    if w > 24 or h > 24:
        print("Resizing to 24x24...")
        scale = 22.0 / max(w, h) # Target 22 to fit in 24
        new_w = int(w * scale)
        new_h = int(h * scale)
        rgba = cv2.resize(rgba, (new_w, new_h), interpolation=cv2.INTER_NEAREST)
    
    cv2.imwrite(output_path, rgba)
    print(f"Saved to {output_path}")

try:
    process_image(r"C:\Users\20567\.gemini\antigravity\brain\cce23767-f648-4b68-899b-7b5cbc96fe1c\uploaded_image_1766684001731.png", r"F:\123\Sigeluguanshou\Resources\monsters\slime.png")
except Exception as e:
    print(f"Exception: {e}")
