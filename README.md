# ESP32 BLE Rover Project

This project showcases a **BLE-controlled rover** built using an **ESP32** and designed to drive **four DC motor wheels** via **two L293D motor driver chips**.

## Features
- Uses **Bluetooth Low Energy (BLE)** for remote control of the rover.
- Controls **four TT motors** (or similar DC motors) through GPIO pins.
- Direction and speed control for each motor using PWM (LEDC).
- Structured for modular expansion—easily tweakable for different motor drivers or BLE commands!

## Hardware Requirements
- **ESP32** microcontroller
- **Two L293D** motor driver ICs
- **Four DC motors** (TT motor wheels recommended)
- **External power supply** (strongly recommended for reliable operation!)

  ⚠️ **Note:**  
  To achieve smooth and reliable movement, you **must** use an **external power supply** for the motors. A single 9V battery or weak source may **not** provide enough current to start all four motors simultaneously.  
  - **Recommended power**: 4xAA NiMH pack, 2S LiPo (7.4V), or USB power bank with at least 2A output.

## Wiring Overview
- Each motor uses **3 GPIO pins**: IN1, IN2 (direction) and EN (PWM enable).
- L293D driver chips connected to ESP32 GPIOs.
- Shared **ground** between ESP32 and motor power supply.

BLE Integration

The rover listens for BLE commands to:

    Move forward/backward

    Turn left/right

    Activate/Deactivate Turbo

    Stop motors

Adapt the BLE service to your control application!
