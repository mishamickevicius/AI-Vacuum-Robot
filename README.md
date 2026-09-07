# Autonomous AI Vacuum Robot

An autonomous, intelligent indoor vacuum cleaning robot powered by an ESP32-S3 real-time microcontroller and a Raspberry Pi 4 edge perception engine.

## System Overview

```
+-----------------------------------------------------------+
|                      Raspberry Pi 4                       |
|   - CSI-2 Camera + YOLOv8 Object & Obstacle Detection     |
|   - High-level SLAM, Room Mapping, Cleaning Path Planner  |
+-----------------------------+-----------------------------+
                              | UART / SPI Communication
+-----------------------------v-----------------------------+
|                        ESP32-S3                           |
|   - Robot Orchestrator FSM (CLEANING_MODE, DOCKING, etc.)  |
|   - Motor Control: Suction Motor, Brush, Drive Wheels     |
|   - Sensor Driver: IR Cliff / Bumper Sensors, IMU         |
+-----------------------------------------------------------+
```

## Repository Structure

- `ai-vacuum-robot/`: ESP-IDF firmware for ESP32-S3 microcontroller.
  - `main/`: Application entry point (`main.c`) and state machine orchestrator (`robot_orchestrator.c`).
  - `components/motor_control/`: PWM and GPIO drivers for suction motor, brush motor, and drivetrain.
  - `components/ir_sensor/`: Drivers for cliff detection and proximity IR sensors.
  - `components/rpi_connector/`: UART communication bridge to the Raspberry Pi.
  - `components/gpio_driver/`: Low-level GPIO configuration wrappers.
- `KiCad_Vacuum_Robot/`: Complete KiCad hardware schematics and PCB layout.
  - `KiCad_Vacuum_Robot.kicad_pro`: KiCad project file.
  - `KiCad_Vacuum_Robot.kicad_sch`: Top-level hierarchical schematic.
  - `KiCad_Vacuum_Robot.kicad_pcb`: PCB layout design.
- `rpi/`: Python vision and telemetry scripts for Raspberry Pi 4.
  - `object_detect.py`: MIPI CSI-2 camera capture and YOLO inference benchmark.
  - `uart_connect.py`: UART ping-pong and latency benchmarking script.
- `SystemOverview.excalidraw`: High-level system architecture and state machine diagrams.

## Getting Started

### Firmware (ESP32-S3)
1. Install [ESP-IDF v5+](https://docs.espressif.com/projects/esp-idf/en/stable/).
2. Navigate to `ai-vacuum-robot/`.
3. Set the target:
   ```bash
   idf.py set-target esp32s3
   ```
4. Build and flash:
   ```bash
   idf.py build
   idf.py -p <PORT> flash monitor
   ```

### Perception (Raspberry Pi 4)
1. Set up Python virtual environment:
   ```bash
   python3 -m venv .venv && source .venv/bin/activate
   pip install ultralytics opencv-python pyserial torch
   ```
2. Run vision benchmark:
   ```bash
   python rpi/object_detect.py
   ```
3. Test UART connection to ESP32:
   ```bash
   python rpi/uart_connect.py
   ```
