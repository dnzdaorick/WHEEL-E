# W.H.E.E.L.-E. — Wire-frame Hybrid Expressive Electronic Locomotive - Explorer

W.H.E.E.L.-E. is not just a robot; it's a kinetic circuit sculpture with a life of its own. At its core, an ESP32-C3 SuperMini acts as a central Wi-Fi Access Point, hosting a WebSocket server and a rich web-based dashboard. This allows for a seamless dual-control system: command W.H.E.E.L.-E. from any smartphone's web browser or use the dedicated tactile hardware remote for a classic hands-on experience.

But control is just the beginning. W.H.E.E.L.-E. is packed with a personality engine, featuring a dynamic face, a rich sound library, and a secret synthesizer mode that transforms it from a simple rover into a polyphonic musical instrument.

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

## Features: From Kinetic Robot to Polyphonic Instrument

W.H.E.E.L.-E. has evolved from a simple kinetic robot into a full-blown musical instrument and interactive companion. Here are some of its key features:

### Expressive Personality

- **Custom Face Activities:** The OLED display is more than just a status screen; it's W.H.E.E.L.-E.'s face. It cycles through a range of animations, from curious eyes to happy blinks, all synchronized with sound effects from the passive buzzer. Each movement and interaction elicits a unique facial response, bringing the robot to life.
- **Rich Sound Library:** A passive buzzer provides a wide range of audio feedback, from simple clicks and beeps to complex musical tunes, giving W.H.E.E.L.-E. a voice to express itself.

### Secret Synthesizer Mode

- **Become the Musician:** Unlock a hidden synthesizer mode that transforms W.H.E.E.L.-E. into a playable electronic instrument. The controls shift from navigation to musical expression, allowing you to create melodies on the fly.
- **Slide Keypad:** The remote's keypad becomes a slide keypad, allowing for expressive, pitch-bending notes, similar to a ribbon synthesizer.
- **Secret Chord Mode:** Discover the secret chord feature! By holding multiple keys on the remote, you can play rich, polyphonic chords, turning a simple melody into a full harmony.
- **Auto-Exit:** The synthesizer mode is designed for spontaneous musical moments. If there's no interaction for 7 seconds, W.H.E.E.L.-E. gracefully exits the synthesizer mode and returns to its default robotic state.

### Intelligent Power Management

- **Auto-Sleep Mode:** To conserve energy, W.H.E.E.L.-E. automatically enters a low-power sleep mode after 2 minutes of inactivity. Its face will show a "sleeping" animation. A simple touch of the controls will wake it up instantly, ready for action.

## 1. Complete Project Bill of Materials (BOM)

### A. W.H.E.E.L.-E. Chassis

- Core Microcontroller: ESP32-C3 SuperMini (using castellated solder pads).
- Motor Driver: MX1508 Dual H-Bridge DC Motor Driver.
- Actuators: 2x GA12-N20 Right-Angle Worm Gear Motors (Mirrored Layout).
- Display (Face): 0.96" SSD1306 I2C OLED Display.
- **Sound:** Passive Buzzer for audio output.
- Power Module: Type-C USB 5V 2A Boost Converter Charger Board + 3.7v 14250 Battery 300mAh.
- Chassis Frame: 1mm Solid-Core Copper Wire.

### Logic Unit: Arduino Uno R3, which sends remote commands through the handheld station

- User Interface: OSEPP 16x2 LCD Display & Keypad Shield (Resistor-ladder buttons mapped to analog pin A0).
- Input: Slide Keypad functionality for synthesizer mode.
- Wireless Transceiver: ESP32-DevKitC, which bridges the serial link from the remote to Wi-Fi and forwards commands.
- Logic Level Safety: 4-Channel I2C Logic Level Converter (bridges Arduino’s 5V signals to 3.3V for the ESP32-DevKitC).

## Master Wiring Directories

### A. W.H.E.E.L.-E.Chassis Wiring

Power & Ground Connections

- ESP32-C3 (5V / VBUS) → MX1508 Motor Driver (+)
- ESP32-C3 (3V3) → I2C OLED Display (VCC)
- ESP32-C3 (GND) → MX1508 Motor Driver (-)
- ESP32-C3 (GND) → I2C OLED Display (GND)
- ESP32-C3 (GPIO 10) → Passive Buzzer (+)
- ESP32-C3 (GND) → Passive Buzzer (-)

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

The OSEPP LCD Keypad Shield snaps directly onto the Arduino Uno. Since the Arduino Uno runs on 5V and the ESP32-DevKitC uses 3.3V, a Logic Level Converter is required to bridge their serial communication lines.
