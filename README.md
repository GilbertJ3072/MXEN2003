# MXEN2003 — Microcontroller Project

A PlatformIO embedded C project developed for the MXEN2003 Microcontroller Project. The system consists of two ATmega2560 microcontrollers communicating wirelessly via XBee — one onboard the robot cart, one in a handheld controller. The cart supports three operating modes selectable from the controller.

---

## System Overview

```
[ Controller (ATmega2560) ] <---XBee---> [ Robot (ATmega2560) ]
  - Joysticks                               - DC drive motors
  - Mode button                             - Servo (claw)
  - LCD display                             - Photoresistors (LDR)
  - Serial monitor output                   - IR range sensors (x3)
                                            - Battery monitor
```

---

## Operating Modes

Modes are cycled on the controller using the left joystick button. The active mode is displayed on the controller's LCD screen.

| Mode | ID | Description |
|------|----|-------------|
| Autonomous | `0` | Cart navigates independently using three IR distance sensors (left, front, right) to avoid obstacles and follow walls |
| Manual | `1` | Cart is driven via the right joystick on the controller. Left joystick vertical axis controls the servo (claw) |
| Beacon Seek (Light Seek) | `2` | Cart uses two photoresistors to seek a light source, steering toward the brighter side using a 10-sample rolling average |

A fourth continuous behaviour — **beacon frequency detection** — runs in the background across all modes, measuring the flash frequency of a light beacon using the left photoresistor and reporting it back to the controller.

---

## Hardware

### Robot
| Component | Purpose |
|-----------|---------|
| ATmega2560 | Main microcontroller |
| XBee module | Wireless communication (Serial2) |
| L298N H-bridge (PORTF) | DC motor direction control |
| Timer1 PWM (OCR1A/OCR1B) | Motor speed control |
| Timer3 PWM (OCR3A) | Servo position control |
| Photoresistors × 2 | ADC12, ADC13 — light sensing and beacon detection |
| IR distance sensors × 3 | ADC9 (left), ADC10 (front), ADC11 (right) |
| Battery monitor | ADC8 — LED warning when low |

### Controller
| Component | Purpose |
|-----------|---------|
| ATmega2560 | Main microcontroller |
| XBee module | Wireless communication (Serial2) |
| Right joystick | ADC0 (vertical), ADC1 (horizontal) — manual drive |
| Left joystick | ADC15 (vertical) — servo/claw control |
| Left joystick button | PD1 / INT1 — mode cycling with 200ms debounce |
| LCD display | Mode name and beacon detection status |

---

## Wiring / Pin Reference

### Robot — PORTF (H-Bridge direction)
| Pin | Function |
|-----|----------|
| PF0 | Left motor IN1 |
| PF1 | Left motor IN2 |
| PF2 | Right motor IN1 |
| PF3 | Right motor IN2 |
| PF4 | Low battery LED |

### Robot — PWM
| Timer | Pin | Function |
|-------|-----|----------|
| Timer1 OCR1A | PB5 | Left motor speed |
| Timer1 OCR1B | PB6 | Right motor speed |
| Timer3 OCR3A | PE3 | Servo position |

---

## Tuning Parameters

These `#define` values in `Robot.c` can be adjusted to tune behaviour:

| Constant | Default | Description |
|----------|---------|-------------|
| `lightBoundaryValue` | `80` | ADC threshold for detecting a beacon in Light Seek mode |
| `lightDiffBoundaryValue` | `5` | Dead-band for steering in Light Seek mode |
| `sideBoundaryValue` | `50` | Side sensor threshold for turning in Autonomous mode |
| `frontBoundaryValue` | `20` | Front sensor threshold for obstacle avoidance in Autonomous mode |
| `frequencyDetectValue` | `70` | ADC threshold for detecting a beacon flash in frequency detection |

---

## XBee Communication Protocol

All communication is at 100ms intervals.

### Controller → Robot (4 bytes)
| Byte | Content |
|------|---------|
| 0 | Right joystick horizontal (`Rhorizontal`) |
| 1 | Right joystick vertical (`Rvertical`) |
| 2 | Left joystick vertical (`Lvertical`) — servo |
| 3 | Current mode (0 / 1 / 2) |

### Robot → Controller (6 bytes)
| Byte | Content |
|------|---------|
| 0 | Left photoresistor value |
| 1 | Right distance sensor |
| 2 | Front distance sensor |
| 3 | Left distance sensor |
| 4 | Beacon frequency (integer Hz) |
| 5 | Beacon frequency (decimal digit) |

---

## Building & Flashing

### Requirements
- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- ATmega2560 board definitions

### Steps

```bash
git clone https://github.com/GilbertJ3072/MXEN2003.git
cd MXEN2003
```

Open the project in VS Code with PlatformIO, then flash each target separately:

```bash
# Flash the robot
pio run -e robot --target upload

# Flash the controller
pio run -e controller --target upload
```

> If separate environments are not yet configured, update `platformio.ini` with `[env:robot]` and `[env:controller]` sections pointing to their respective `src` directories.
