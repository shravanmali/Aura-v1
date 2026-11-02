# Aura v1: Autonomous Mobile Companion Robot

Hi, I'm Shravan. **Aura v1** is an open-source autonomous companion robot (rover) I am building from scratch for [Hack Club Fallen](https://fallen.hackclub.com/projects/prj_kV1Fde). 

The goal of Aura v1 is to build an intelligent, mobile companion robot on a custom chassis that can follow me around my room, monitor ambient climate, act as a motion sentry, and connect to a stationary bedroom satellite pod for whole-home voice control.

---

## 🛠️ Features (✅ = Built & Working on Bench)

- **Computer Vision Person Tracking** (OpenCV with real-time serial steering). ✅
- **Dual-Chip Architecture** (Linux brain + zero-latency real-time microcontroller).
- **3S2P Battery Pack** (11.1V 4400mAh with BMS safety).
- **Acoustically Isolated Audio** (45° rear-firing speaker + upward MEMS mic).
- **Stationary Satellite Pod** (acts as a desk voice assistant for room control).
- **4WD Skid-Steer Locomotion** - 4x 200 RPM BO geared motors driven by an L298N dual H-bridge motor driver. ✅

---

## 📐 3D CAD Chassis Design

The chassis plates are designed in OpenSCAD with dual-deck standoffs to hold the electronics securely:

![Aura v1 Assembly](scad_assembly_render.png)

![Aura v1 Rear View](scad_rear_view.png)

---

## 🔌 System Architecture

```text
┌──────────────────────────────────────────────┐
│           Smartphone (DroidCam Stream)       │
│           (Live Video at 640x480 @ 30 FPS)   │
└──────────────────────┬───────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────┐
│       Python OpenCV Companion Dashboard      │
│     (Face Tracking, Centroid Math, HUD)      │
└──────────────────────┬───────────────────────┘
                       │
                       ▼ USB Serial (115200 Baud)
┌──────────────────────────────────────────────┐
│              ESP32-S3 DevKit-C               │
│    (Motor Kinematics, Watchdog, Telemetry)   │
└──────────────────────┬───────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────┐
│             L298N Motor Driver               │
│             (4WD Skid-Steer)                 │
└──────────────────────────────────────────────┘
```

---

## ⚡ Power Wiring Layout

```text
            ┌──────────────────────────────────────────┐
            │       3S 11.1V Li-ion Battery Pack       │
            └────────────────────┬─────────────────────┘
                                 │
                                 ▼
            ┌──────────────────────────────────────────┐
            │               3S 20A BMS                 │
            └────────────┬─────────────────┬───────────┘
                         │                 │
            (11.1V Line) │                 │ (11.1V Line)
                         ▼                 ▼
            ┌────────────────────┐ ┌────────────────────┐
            │    L298N Driver    │ │    XL4015 BUCK     │
            │    (Motor Power)   │ │    (Regulated 5V)  │
            └────────────┬───────┘ └─────────┬──────────┘
                         │                   │
                         ▼                   ▼
            ┌────────────────────┐ ┌────────────────────┐
            │   4x BO Motors     │ │    ESP32-S3 &      │
            │   (Chassis Drive)  │ │    Sensor Rails    │
            └────────────────────┘ └────────────────────┘
```

---

## 📦 Hardware Components Used on Bench So Far

| Item | Component | Specification / Model | Qty | Primary Role |
| :---: | :--- | :--- | :---: | :--- |
| 1 | ESP32-S3 | WROOM-1 N16R8 (16MB Flash, 8MB RAM) | 1 pc | Core MCU & Serial Bridge |
| 2 | Motor Driver | L298N Dual H-Bridge Module | 1 pc | 4WD Drive Power Stage |
| 3 | BO Motors & Wheels | 3-9V Dual Shaft BO Motor + 65mm Wheels | 4 pcs | Chassis Locomotion |
| 4 | Breadboard | MB-102 830 Points Solderless | 1 pc | Circuit Prototyping |
| 5 | Jumper Wires | Dupont M-to-M / M-to-F Assorted | ~40 pcs | Signal & Power Wiring |

---

## 📋 Bill of Materials (Hack Club Fallen Grant)

The full audited component list for the Hack Club Fallen hardware grant is saved in [`BOM.csv`](BOM.csv):

| Item | Component | Purpose on Aura v1 | Qty | Price (INR) | Vendor |
| :---: | :--- | :--- | :---: | :---: | :--- |
| **1** | Raspberry Pi 3 Model B (1GB) | Main AI Brain & Computer Vision | 1 | ₹3,850 | MakerBazar.in |
| **2** | 160° Wide-Angle CSI Camera (OV5647) | Person Tracking & Auto-Docking | 1 | ₹749 | Robu.in |
| **3** | TowerPro MG90S Metal Gear Servos | 2-Axis Pan-Tilt Camera Gimbal | 2 | ₹252 | Robu.in |
| **4** | XL4015 5A DC-DC Buck Converter | 5.1V Clean Power Rail for Pi 3B | 1 | ₹109 | Robu.in |
| **5** | LM2596S 3A DC-DC Buck Converter | 5.0V Rail for Servos & LEDs | 1 | ₹69 | Robu.in |
| **6** | 18650 2200mAh Li-ion Cells (3S2P) | 11.1V 4400mAh High-Torque Pack | 6 | ₹936 | Robu.in |
| **7** | 3S 20A Li-ion BMS Protection Board | Battery Safety & Balance Protection | 1 | ₹149 | Robu.in |
| **8** | 12.6V 2A CC/CV Smart Charger | Auto-Docking Station Power Supply | 1 | ₹380 | ElectronicsComp.com |
| **9** | ESP32-C3 SuperMini Board | Stationary Bedroom Satellite Pod | 1 | ₹279 | Robu.in |
| **10** | INMP441 I2S MEMS Microphone | Digital Voice AI Input for Pi | 1 | ₹149 | Robu.in |
| **11** | SmartElex MAX98357A I2S Amplifier | 3.2W Class-D Digital Audio Amp | 1 | ₹194 | Robu.in |
| **12** | 3W 4Ω Enclosed Cavity Speaker | Room Voice & Music Playback | 1 | ₹80 | MakerBazar.in |
| **13** | WS2812 8-Bit Straight RGB LED Strip | Visual State Feedback Lightbar | 1 | ₹49 | Robu.in |
| **14** | M3 Brass Standoffs Assorted Box | Dual-Deck Chassis Assembly | 1 | ₹160 | Robu.in |
| **TOTAL** | **Complete System Grant Request** | **Aura Rover + Satellite Pod** | **14 Items** | **₹7,405 (~$89.20 USD)** | **All Vendors** |

---

## 🚀 How to Run Locally

### 1. Upload Microcontroller Firmware
Open this repository in VS Code with PlatformIO:
```powershell
pio run --target upload --upload-port COM6
```
*(Or open `src/main.cpp` in Arduino IDE, select ESP32-S3, and upload).*

### 2. Start the Vision Companion Dashboard
1. Open DroidCam on your phone and note the Wi-Fi IP (default is `http://192.168.31.75:4747/video`).
2. Run the tracker on your computer:
```powershell
python vision_tracker.py
```

Controls:
- Stand in front of the camera and step left or right — the motors will steer to keep you centered.
- Press **W / A / S / D** on your keyboard to manually drive the motors.
- Press **Spacebar** to stop.
- Press **Q** to exit cleanly.
