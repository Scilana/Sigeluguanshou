import cv2
import numpy as np
import os

def process_image(input_path, output_path):
    print(f"Processing {input_path}...")
    
    img = cv2.imread(input_path, cv2.IMREAD_UNCHANGED) # Load with alpha if possible
    if img is None:
        print("Error: Could not load image")
        return

    # If image has alpha channel, use it for mask
    if img.shape[2] == 4:
        alpha = img[:, :, 3]
        _, mask = cv2.threshold(alpha, 10, 255, cv2.THRESH_BINARY)
    else:
        # Assume white/solid background if no alpha?
        # Or just try to crop content.
        # Let's create a mask from "not background".
        # Assume top-left pixel is background?
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        # Simple thresholding might not work if bg is complex. 
        # But usually these uploads have white or transparent bg.
        # Let's rely on finding contours of "content".
        # In this specific case, let's assume non-white/non-transparent.
        # But `process_slime` used color tracking.
        # This zombie is skeletal.
        # Let's try simple alpha check first.
        # If 3 channels, create alpha.
        mask = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        _, mask = cv2.threshold(mask, 240, 255, cv2.THRESH_BINARY_INV) # Assume white bg
    
    # Find contours
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    if not contours:
        print("Error: No content found")
        # Just save original if fail
        cv2.imwrite(output_path, img)
        return

    # Bounding box of all contours
    x_min, y_min = img.shape[1], img.shape[0]
    x_max, y_max = 0, 0
    
    for c in contours:
        x, y, w, h = cv2.boundingRect(c)
        if w * h < 50: continue # Ignore noise
        x_min = min(x_min, x)
        y_min = min(y_min, y)
        x_max = max(x_max, x + w)
        y_max = max(y_max, y + h)

    if x_max < x_min:
        print("No valid content found")
        return
        
    # Crop
    pad = 1
    x = max(0, x_min - pad)
    y = max(0, y_min - pad)
    w = min(img.shape[1], x_max + pad) - x
    h = min(img.shape[0], y_max + pad) - y
    
    cropped = img[y:y+h, x:x+w]
    
    # Resize to match "Player Size"
    # Height approx 40-48 px.
    target_h = 42
    scale = target_h / h
    new_w = int(w * scale)
    new_h = int(h * scale)
    
    print(f"Resizing from {w}x{h} to {new_w}x{new_h}")
    resized = cv2.resize(cropped, (new_w, new_h), interpolation=cv2.INTER_NEAREST)
    
    # Ensure directory
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    cv2.imwrite(output_path, resized)
    print(f"Saved to {output_path}")

try:
    process_image(r"C:\Users\20567\.gemini\antigravity\brain\cce23767-f648-4b68-899b-7b5cbc96fe1c\uploaded_image_1766684875832.png", r"F:\123\Sigeluguanshou\Resources\monsters\zombie.png")
except Exception as e:
    print(f"Exception: {e}")
