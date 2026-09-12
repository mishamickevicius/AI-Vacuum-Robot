"""
Autonomous AI Vacuum Robot - Vision & Object Detection Pipeline
Perception system running on Raspberry Pi 4 for real-time obstacle avoidance,
room feature detection, and cleaning navigation benchmarks.
"""

import time
import cv2
import numpy as np
import torch
from ultralytics import YOLO
from ultralytics.engine.results import Results, Boxes
from picamera2 import Picamera2

# ==========================================
# 1. Capture Image via MIPI CSI-2 (Picamera2)
# ==========================================
print("AI Vacuum Robot: Initializing CSI-2 camera...")
with Picamera2() as picam2:
    # Use standard 640x480 RGB preview configuration
    config = picam2.create_preview_configuration(
        main={"format": "RGB888", "size": (640, 480)}
    )
    picam2.configure(config)
    picam2.start()

    # Allow auto-exposure and AWB (auto white balance) to settle
    time.sleep(1.0)

    # Capture raw frame as NumPy array (shape: 480, 640, 3)
    frame_raw = picam2.capture_array()
    picam2.stop()

# Resize to standard YOLO square input (640, 640, 3)
frame_rgb = cv2.resize(frame_raw, (640, 640))
print(f"Captured and prepared frame shape: {frame_rgb.shape}")

model_name = "yolov8n.pt"
print(f"\nBenchmarking {model_name} on Raspberry Pi 4 CPU...\n")

# ==========================================
# METHOD 1: ULTRALYTICS PIPELINE
# ==========================================
print("--- Method 1: Ultralytics Wrapper ---")
# Downloads yolov8n.pt automatically if not locally present
model_ul = YOLO(model_name)

# Warmup run
_ = model_ul(frame_rgb, verbose=False)

start_time = time.time()
results = model_ul(frame_rgb, verbose=False)
print(f"Length of results: {len(results)}")
result: Results = results[0]
try: 
    boxes: Boxes = result.boxes 
    print(boxes)
    print()
    
    names = result.names
    detected_class = int(boxes.cls[0])
    try:
        print(f"Class: {detected_class} ---> {names[detected_class]}")
    except KeyError:
        print("Could not get class name")
        print(detected_class)


except AttributeError:
    print("Can't find boxes")

print(result)
ul_latency = time.time() - start_time

print(f"Total Latency (Pre + Forward + NMS): {ul_latency:.4f} seconds ({1/ul_latency:.2f} FPS)")
print(f"Objects detected: {len(results[0].boxes)}")

# ==========================================
# METHOD 2: RAW PYTORCH INFERENCE
# ==========================================
# print("\n--- Method 2: Raw PyTorch Forward Pass ---")
# # Extract underlying nn.Module (weights_only=False required for Ultralytics models)
# ckpt = torch.load(model_name, map_location="cpu", weights_only=False)
# model_pt = (ckpt["model"] if "model" in ckpt else ckpt).float().eval()

# # Manual Preprocessing (HWC uint8 [0..255] -> NCHW float32 [0.0..1.0])
# img_chw = np.transpose(frame_rgb, (2, 0, 1))
# img_tensor = torch.from_numpy(img_chw).float() / 255.0
# img_tensor = img_tensor.unsqueeze(0)  # Shape: [1, 3, 640, 640]

# # Warmup run
# with torch.no_grad():
#     _ = model_pt(img_tensor)

# start_time = time.time()
# with torch.no_grad():
#     raw_preds = model_pt(img_tensor)
# pt_latency = time.time() - start_time

# # If model_pt returns a tuple/list (e.g. (preds, feats)), take index 0
# out_tensor = raw_preds[0] if isinstance(raw_preds, (tuple, list)) else raw_preds

# print(f"Pure Forward-Pass Time: {pt_latency:.4f} seconds ({1/pt_latency:.2f} FPS)")
# print(f"Raw Output Tensor Shape: {out_tensor.shape}")

# ==========================================
# OVERHEAD COMPARISON
# ==========================================
# print("\n--- Summary ---")
# overhead_ms = (ul_latency - pt_latency) * 1000
# print(f"Ultralytics Wrapper Overhead (Letterbox + NMS): {overhead_ms:.2f} ms")