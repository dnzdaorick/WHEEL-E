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

## Features: From Kinetic Circuit Sculpture to Interactive Synthesizer

W.H.E.E.L.-E. is a multi-mode interactive companion packed with custom hardware-level features and a rich personality. Below is the complete feature map of the locomotive:

### 1. Interactive OLED Face & Physics Particle Engine
W.H.E.E.L.-E.'s face is rendered on a **0.96" SSD1306 I2C OLED display** running in **400 kHz Fast-Mode I2C** to support smooth, jitter-free **30 FPS animations**.
- **Interactive Expressions (`FaceState`):**
  - **Booting:** Plays system initialization checklist (System OK, WiFi AP, I2C Bus, Battery status) and fills a boot progress bar over 3 seconds.
  - **Connecting / Online / Offline:** Transitions between search states (animating Wi-Fi antenna), active connection (glowing heart-shape mouth, blushing cheeks, floating sparks), and offline state (sad cross-eyes with falling tears).
  - **Locomotion Tracking:** Eyes and facial layout shift dynamically in real-time matching the current movement direction (`FORWARD`, `BACKWARD`, `LEFT`, `RIGHT`).
  - **Power & Charging:** Features distinct visual battery status bars, charging indicator sweeps, and a full charge complete face.
- **Dynamic Physics Particle Engine:**
  - A real-time background particle simulator drives complex animation feedback on the OLED screen.
  - Particle types include: **Sweat droplets** (giggling), **wobbling music notes** with custom sine-wave horizontal drift (singing/music mode), **drifting sleeping 'Z's** (napping), **steam clouds** (sighing/anger), **exclamation triangles** (low battery warnings), and **falling teardrops** featuring realistic gravity acceleration and splash explosions on landing.
- **Expressive Idle Personality Scheduler (`IdleActivity`):**
  - When stationary, W.H.E.E.L.-E. shifts between 10 scheduled activities to show mood changes:
    - *Resting:* Standard calm eyes with random natural blinking.
    - *Singing:* Cheerful chirps combined with floating music notes.
    - *Giggling:* Body/face shaking with sweat-laughs and high-pitched giggles.
    - *Looking:* Panning eyes left and right with a curious expression.
    - *Dizzy:* Spinning cross-eyes, dizzy chiptune melodies, and 3 orbiting stars overhead.
    - *Pouting:* Low angry growls, slanted eyebrows, steam vents, and a pulsing anger cross.
    - *Sighing:* Half-closed eyes, drooping mouth, and a slow puff of steam.
    - *Sneezing:* Tense build-up squeeze, followed by a massive droplet burst and loud sneeze chime.
    - *Yawning:* Wide mouth oscillation, teardrop spawns, and drooping yawn pitch notes.
    - *Napping:* Deep sleep mode with bottom-curved eyes, snoring, and floating 'Z's. Shows live telemetry metrics (battery percentage and audio volume) at the bottom.

### 2. Dual-Ended Control Subsystem
Commands are received via low-overhead **WebSockets** supporting concurrent dual-control interfaces:
- **Mobile Web-Based Dashboard (`web_ui.h`):**
  - Built using a premium dark-mode **glassmorphism layout** with backdrop blur filters, custom typography, and responsive touch-zones.
  - **Theme Toggle:** Tap the `WHEEL-E` logo to seamlessly transition between a sleek dark theme and a high-contrast light theme (saved to local storage).
  - **Directional Ambient Glow:** Casts a radial color shadow in the direction of current movement.
  - **Multi-Touch Support:** Resolves touch points dynamically via client-side JavaScript, enabling multi-key chord combinations.
  - **Slide-to-Adjust Volume HUD:** Drag the mode badge left or right to show a horizontal volume bar, allowing real-time adjustment from 0% (muted) to 100%. Shows custom icons reflecting volume levels (🔇 🔈 🔉 🔊).
- **Physical Remote Station (`remote-uno` & `remote-esp32`):**
  - Features an **Arduino Uno** running a hardware watchdog timer (2-second timeout) for crash safety, reading an **OSEPP 16x2 LCD Shield** resistor-ladder buttons (`A0`).
  - Connects to an **ESP32-DevKitC** via a 4-channel logic-level converter to forward commands over Wi-Fi at 115200 baud.

### 3. Advanced Sound Engine
Audio feedback is synthesized using the ESP32-C3's **LEDC hardware PWM generator** paired with a passive piezo buzzer.
- **Cubic Volume Scaling:** Uses a volume formula ($Volume_{cubic} = Fraction^3$) to map volume changes, compensating for human auditory curves and the non-linear activation threshold of piezo buzzers.
- **Backup Beep System:** Alternates reverse warning tones at 349 Hz (F4 note) every 250 ms while driving backwards.
- **Note Decay Envelopes:** Tracks active sound durations in the background, automatically muting notes after 150 ms in music mode to prevent hanging frequencies.
- **Sound library:** Dozens of unique note-sequences for boot sequences, alarms, status indicators, and personality transitions.

### 4. Secret Synthesizer (Synth) Mode
Unlock the secret musical mode to turn W.H.E.E.L.-E. into a playable chiptune synthesizer!
- **How to Unlock:** Quickly tap **`LEFT` ➔ `RIGHT` ➔ `LEFT` ➔ `RIGHT`** within 2 seconds. The robot plays an unlock chime and shifts the Web GUI to a golden theme.
- **Polyphonic Chiptune Chord Multiplexer:** W.H.E.E.L.-E. arpeggiates active notes at a blazing **18 ms cycle rate**, blending them together to simulate polyphonic chords over a single-voice buzzer.
- **Interactive Synthesis UI:** In synth mode, W.H.E.E.L.-E.'s eyes expand, and its mouth vibrates dynamically relative to the notes being held. Swiping or tapping the keys spawns bursts of music note particles on the OLED face in the direction of the gesture.
- **Auto-Exit Timer:** If no keys are pressed for **10 seconds**, W.H.E.E.L.-E. plays an exit sound and automatically reverts to drive mode to save power.

### 5. Intelligent Power & Battery Management
- **Auto-Sleep:** Transitions to low-power sleep (Yawning and Napping) after **2 minutes of inactivity**. Receiving any movement command immediately wakes the robot up.
- **ADC Noise Filtering:** Employs an exponential moving average (EMA) filter ($V_{EMA} = 0.8 \times V_{EMA} + 0.2 \times V_{measured}$) to eliminate analog noise on battery voltage checks.
- **Charge Protection & Telemetry:**
  - *Active Charging:* Detects charger insertion, plays a plugging tone, switches to charging face, and animates a battery gauge.
  - *Full Charge:* Detects $\ge 4.12\text{ V}$, plays a triumphant chime, and locks the OLED battery gauge at 100%.
  - *Low Battery:* Flashes a warning triangle and sounds low battery warnings every 10 seconds when voltage drops below $\le 3.45\text{ V}$.
- **Tx Power Conservation:** The remote ESP32 reduces TX power to $8.5\text{ dBm}$ since W.H.E.E.L.-E. is operated nearby, saving heat and remote battery life.

## 1. Complete Project Bill of Materials (BOM)

### A. W.H.E.E.L.-E. Chassis

- Core Microcontroller: ESP32-C3 SuperMini (using castellated solder pads).
- Motor Driver: MX1508 Dual H-Bridge DC Motor Driver.
- Actuators: 2x GA12-N20 Right-Angle Worm Gear Motors (Mirrored Layout).
- Display (Face): 0.96" SSD1306 I2C OLED Display.
- **Sound:** Passive Buzzer driven by a 2N2222A (NPN) transistor and a 1kΩ base resistor to protect the ESP32-C3 GPIO pin.
- Power Module: Type-C USB 5V 2A Boost Converter Charger Board + 3.7v 14250 Battery 300mAh.
- Chassis Frame: 1mm Solid-Core Copper Wire.

### Logic Unit: Arduino Uno R3, which sends remote commands through the handheld station

- User Interface: OSEPP 16x2 LCD Display & Keypad Shield (Resistor-ladder buttons mapped to analog pin A0).
- Input: Keypad functionality for driving and playing music.
- Wireless Transceiver: ESP32-DevKitC, which bridges the serial link from the remote to Wi-Fi and forwards commands.
- Logic Level Safety: 4-Channel I2C Logic Level Converter (bridges Arduino’s 5V signals to 3.3V for the ESP32-DevKitC).

## Master Wiring Directories

### A. W.H.E.E.L.-E.Chassis Wiring

Power & Ground Connections

- ESP32-C3 (5V / VBUS) → MX1508 Motor Driver (+)
- ESP32-C3 (3V3) → I2C OLED Display (VCC)
- ESP32-C3 (GND) → MX1508 Motor Driver (-)
- ESP32-C3 (GND) → I2C OLED Display (GND)
- ESP32-C3 (GPIO 10) → 1kΩ Resistor → 2N2222A Transistor (Base)
- ESP32-C3 (5V / VBUS) → Passive Buzzer (+)
- Passive Buzzer (-) → 2N2222A Transistor (Collector)
- ESP32-C3 (GND) → 2N2222A Transistor (Emitter)

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
