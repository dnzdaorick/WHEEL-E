# W.H.E.E.L.-E. — Wire-frame Hybrid Expressive Electronic Locomotive - Explorer

ESP32-C3 SuperMini will act as a central Wi-Fi Access Point and host a WebSocket server. It can accept commands from a smartphone web browser and from your custom physical remote controller (Arduino Uno + OSEPP LCD Shield + ESP32-DevKitC client) simultaneously.

```
                    ┌──────────────────────────────┐
                    │         W.H.E.E.L.-E.        │
                    │  (ESP32-C3 AP & Web Server)  │
                    └──────────────┬───────────────┘
                                   │
                    ┌──────────────┴──────────────┐
                    │  Dual-Client WebSockets     │
                    ▼                             ▼
       ┌────────────────────────┐     ┌────────────────────────┐
       │     Mobile Web GUI     │     │ Handheld Remote Station│
       │ (Smartphone over Wi-Fi)│     │  (Arduino Uno + ESP32) │
       └────────────────────────┘     └────────────────────────┘
```

## 1. Complete Project Bill of Materials (BOM)

### A. W.H.E.E.L.-E. Chassis

- Core Microcontroller: ESP32-C3 SuperMini (using castellated solder pads).
- Motor Driver: MX1508 Dual H-Bridge DC Motor Driver.
- Actuators: 2x GA12-N20 Right-Angle Worm Gear Motors (Mirrored Layout).
- Display (Face): 0.96" SSD1306 I2C OLED Display.
- Power Module: Type-C USB 5V 2A Boost Converter Charger Board + 3.7v 14250 Battery 300mAh.
- Chassis Frame: 1mm Solid-Core Copper Wire.

### Logic Unit: Arduino Uno R3, which sends remote commands through the handheld station.

- User Interface: OSEPP 16x2 LCD Display & Keypad Shield (Resistor-ladder buttons mapped to analog pin A0).
- Wireless Transceiver: ESP32-DevKitC, which bridges the serial link from the remote to Wi-Fi and forwards commands.
- Logic Logic Safety: 4-Channel I2C Logic Level Converter (bridges Arduino’s 5V signals to 3.3V for the ESP32-DevKitC).aster Wiring Directories

### A. W.H.E.E.L.-E.Chassis Wiring

Power & Ground Connections

- ESP32-C3 (5V / VBUS) → MX1508 Motor Driver (+)
- ESP32-C3 (3V3) → I2C OLED Display (VCC)
- ESP32-C3 (GND) → MX1508 Motor Driver (-)
- ESP32-C3 (GND) → I2C OLED Display (GND)

Motor Driver Logic Inputs

- ESP32-C3 (GPIO 0) → MX1508 Motor Driver (IN4)
- ESP32-C3 (GPIO 1) → MX1508 Motor Driver (IN3)
- ESP32-C3 (GPIO 2) → MX1508 Motor Driver (IN2)
- ESP32-C3 (GPIO 3) → MX1508 Motor Driver (IN1)

Display I2C Bus

- ESP32-C3 (GPIO 8) → I2C OLED Display (SDA)
- ESP32-C3 (GPIO 9) → I2C OLED Display (SCL)

Motor Outputs

- MX1508 Motor Driver (MOTOR B) → Left Motor (-)
- MX1508 Motor Driver (MOTOR B) → Left Motor (+)
- MX1508 Motor Driver (MOTOR A) → Right Motor (-)
- MX1508 Motor Driver (MOTOR A) → Right Motor (+)

### B. Handheld Remote Control Station Wiring

The OSThe OSEPP LCD Keypad Shield snaps directly onto the Arduino Uno. Since the Arduino Uno runs on 5V and the ESP32-DevKitC uses 3.3V, a Logic Level Converter is required to bridge their serial communication lines.
