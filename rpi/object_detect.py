import time
from ultralytics import YOLO
from picamera2 import Picamera2

# 1. Load the model once before starting the stream
# Point to your exported NCNN folder or .pt file
model = YOLO("yolov8n_ncnn_model")

# 2. Initialize and start Picamera2
print("Initializing camera feed...")
picam2 = Picamera2()
config = picam2.create_preview_configuration(
    main={"format": "RGB888", "size": (640, 480)}
)
picam2.configure(config)
picam2.start()

# Allow camera auto-exposure and AWB to settle
time.sleep(1.0)
print("Starting continuous detection loop. Press Ctrl+C to stop.\n")

frame_idx = 0

## Keep track of object counts
frame = picam2.capture_array()
results = model(frame, imgsz=320, verbose=False)
result = results[0]
names: dict = result.names
counts = {}
for key in names.values():
    counts[key] = 0

assert len(counts) == len(names)

try:
    while True:
        t_start = time.perf_counter()

        # Capture latest frame directly from the sensor
        frame = picam2.capture_array()
        frame_idx += 1

        # Run inference (Ultralytics letterboxes to imgsz automatically)
        results = model(frame, imgsz=320, verbose=False)
        result = results[0]

        # Process detections
        boxes = result.boxes
        if len(boxes) > 0:
            for box in boxes:
                class_id = int(box.cls[0].item())
                class_name = result.names[class_id]
                counts[class_name] += 1
                confidence = float(box.conf[0].item())
                xyxy = [round(x, 1) for x in box.xyxy[0].tolist()]

                print(f"Detected: {class_name:<15} | Conf: {confidence:.2%} | Box: {xyxy}")
        else:
            print(f"[Frame {frame_idx}] No objects detected")

        # Telemetry & cycle time
        fps = 1.0 / (time.perf_counter() - t_start)
        print(f"--- Frame {frame_idx} | {fps:.2f} FPS ---\n")

except KeyboardInterrupt:
    print("\nStopping continuous detection pipeline...")

finally:
    picam2.stop()
    picam2.close()
    print("Camera released successfully.")
    print()
    for key, value in counts.items():
        if value > 0:
            print(f"{key} detected {value} times")