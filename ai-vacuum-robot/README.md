# AI Vacuum Robot - Firmware

This directory contains the ESP-IDF firmware for the **Autonomous AI Vacuum Robot** (ESP32-S3).

## Directory Structure

```
ai-vacuum-robot/
├── CMakeLists.txt              # Top-level project definition
├── main/
│   ├── CMakeLists.txt          # Main component registration
│   ├── include/
│   │   └── robot_orchestrator.h # System FSM & state definitions (CLEANING_MODE, etc.)
│   ├── main.c                  # Firmware entry point (app_main)
│   └── src/
│       └── robot_orchestrator.c # Orchestrator task and motor dispatch
├── components/
│   ├── gpio_driver/            # GPIO wrapper driver
│   ├── ir_sensor/              # Cliff and obstacle IR sensor management
│   ├── motor_control/          # Suction motor, brush motor, and PWM controls
│   └── rpi_connector/          # UART communication driver with Raspberry Pi 4
└── sdkconfig                   # ESP-IDF project configuration
```

## Features

- **State Machine Orchestrator**: Handles transitions between `IDLE`, `CLEANING_MODE`, `OBSTACLE_AVOIDANCE`, `DOCKING`, `CHARGING`, and `ERROR`.
- **Vacuum Actuation**: High-frequency MCPWM driving suction motor and main roller brush.
- **Cliff & Obstacle Sensing**: Real-time IR sensor scanning to avoid falls down stairs or furniture collisions.
- **Bi-directional Bridge**: High-speed UART link exchanging navigation commands and sensor telemetry with the Raspberry Pi.

## Building and Flashing

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor
```
