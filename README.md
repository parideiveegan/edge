# Wireless Robot Control via Hand Gesture Recognition

> Real-time hand gesture classification running entirely on an ESP32-S3, with recognised commands transmitted wirelessly to a robot via UDP over Wi-Fi.

---

## Overview

This project implements an **edge ML pipeline** for gesture-controlled robot operation. An IMU sensor on the wrist captures motion data, a quantized 1D CNN classifies the gesture on-device, and the result is sent as a UDP command to a robot node — no cloud, no internet required.

---

## Features

- 6-class gesture recognition: `forward`, `left`, `right`, `reverse`, `stop`, `none`
- On-device inference using **Edge Impulse int8-quantized** model (4 ms latency)
- Dual-core FreeRTOS — sampling on Core 0, inference on Core 1
- Signal conditioning: high-pass filter + EMA smoothing + deadband
- UDP command transmission over Wi-Fi
- LVGL display showing real-time gesture feedback
- Light sleep mode triggered by hardware button (UP+)

---

## Hardware

| Component | Details |
|---|---|
| Board | Espressif ESP32-S3-EYE |
| Accelerometer | QMA7981 (onboard, I²C) |
| Display | 1.3" LCD (onboard, SPI) |
| Wireless | Wi-Fi 802.11 b/g/n (onboard) |
| Power | 5V micro-USB or Li-ion battery |

---

## Project Structure

```
├── main/
│   ├── main.cpp              # Entry point, task creation
│   ├── sensor.cpp/h          # I2C + QMA7981 driver
│   ├── sampling.cpp/h        # IMU task, ring buffer, signal conditioning
│   ├── inference.cpp/h       # Edge Impulse inference task
│   ├── command_responder.cc/h# LVGL display update
│   ├── wifi_sta.cpp/h        # Wi-Fi station setup
│   ├── udp_client.cpp/h      # UDP command transmission
│   ├── sleep.cpp/h           # Light sleep management
│   ├── task_mgr.cpp/h        # FreeRTOS task registry
│   └── idf_component.yml     # Component dependencies
├── components/
│   └── edge_impulse/         # Edge Impulse SDK + model
│       ├── model-parameters/
│       └── tflite-model/
└── CMakeLists.txt
```

---

## How It Works

```
QMA7981 sensor
     │
     ▼
High-pass filter + EMA + deadband
     │
     ▼
Ring buffer (100 samples, 300 floats)
     │  ← notify every 50 samples
     ▼
Edge Impulse 1D CNN (int8, 4 ms)
     │  ← confidence > 0.80
     ▼
UDP command → robot node (192.168.4.1:3333)
     │
     ▼
LVGL display update
```

---

## Getting Started

### Prerequisites

- [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [Edge Impulse CLI](https://docs.edgeimpulse.com/docs/tools/edge-impulse-cli) (for model updates)
- ESP32-S3-EYE board

### Build and Flash

```bash
# Clone the repo
git clone https://github.com/parideiveegan/try_parideiveegan.git
cd try_parideiveegan

# Set up ESP-IDF environment
. $IDF_PATH/export.sh

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

### Wi-Fi Configuration

The controller connects to a Wi-Fi access point hosted by the robot node.

In `main/wifi_sta.cpp`, update the credentials to match your robot node:

```c
#define STA_SSID  "ESP32_HOTSPOT"
#define STA_PASS  "12345678"
```

The robot node must run a UDP server at `192.168.4.1:3333`.

---

## ML Model

The gesture classifier is a **1D CNN** trained on Edge Impulse:

| Property | Value |
|---|---|
| Input | 150 features (50 samples × 3 axes) |
| Model type | 1D Conv + MaxPool × 2 → Dense(16) → Softmax |
| Quantization | int8 (post-training) |
| Validation accuracy | 97.4% |
| Test accuracy | 93.94% |
| Inference latency | 4 ms (ESP32-S3, 240 MHz) |
| RAM | 4.3 K |
| Flash | 33.8 K |

To retrain or update the model, export the Edge Impulse C++ library and replace the contents of `components/edge_impulse/`.

---

## Gestures

| Gesture | Wrist Motion |
|---|---|
| `forward` | Tilt forward |
| `reverse` | Tilt backward |
| `left` | Tilt left |
| `right` | Tilt right |
| `stop` | Hold flat / firm stop |
| `none` | Idle / resting position |

---

## Sleep Mode

Press the **UP+ button** to enter light sleep. The display dims, Wi-Fi disconnects, and all tasks suspend. Press the **BOOT button (GPIO 0)** to wake.

---

## Code Walkthrough

| File | What it does |
|---|---|
| `main.cpp` | Starts everything — initialises the sensor, display, Wi-Fi, and launches the two main tasks |
| `sensor.cpp` | Talks to the QMA7981 accelerometer over I²C and reads raw XYZ values |
| `sampling.cpp` | Reads the sensor at 25 Hz, cleans the signal, and fills a ring buffer. Notifies inference when enough data is ready |
| `inference.cpp` | Runs the gesture model on the buffered data. If confident enough, sends the result to the display and robot |
| `command_responder.cc` | Updates the LCD screen with the recognised gesture name |
| `wifi_sta.cpp` | Connects to the robot's Wi-Fi hotspot and keeps the connection alive |
| `udp_client.cpp` | Sends the gesture command as a UDP packet to the robot |
| `sleep.cpp` | Watches the UP+ button. When pressed, puts the device to sleep to save battery |
| `task_mgr.cpp` | Keeps track of all running tasks so they can all be paused and resumed together during sleep |

---

## Authors

- **Pari Deiveegan** — Mat. No. 6436108
- **Sahana Taladahalli Mallikarjuna** — Mat. No. 6435041

---

## License

This project is for academic purposes. The Edge Impulse SDK is subject to its own [license terms](https://docs.edgeimpulse.com/docs/edge-impulse-studio/edge-impulse-license).
