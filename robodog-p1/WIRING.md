# RoboDog — Wiring (ESP32 transplant, keeping the stock charge board)

Power everything OFF while wiring. Connect order at the end: **USB in → then battery**.
Your DRV8833 modules are labeled **IN1–IN4 / OUT1–OUT4 / VCC / GND / EEP**.
On each board: **IN1,IN2 → OUT1,OUT2 = channel A**; **IN3,IN4 → OUT3,OUT4 = channel B**.

## Block diagram

```
                      ┌───────────────────────────┐
   3.7V Li-ion ──────►│  STOCK 7MM CHARGE BOARD    │   (keep — safe charge + protection)
   (stays on it)      │   VBAT+        GND         │
                      └────┬─────────────┬─────────┘
                           │ VBAT (~3.7V)│ GND
             ┌─────────────┼─────────────┼───────────────┬──────────────┐
             │             │             │               │              │
             ▼             ▼             ▼               ▼              ▼
        ┌─────────┐   ┌─────────┐   ┌─────────┐     ┌─────────┐   (common GND
        │ DRV8833 │   │ DRV8833 │   │ MT3608  │     │ ESP32   │    rail ties ALL
        │  #1 LEFT│   │ #2 RIGHT│   │ boost   │     │  GND    │    grounds together)
        │  VCC    │   │  VCC    │   │ IN+ ────┘     └─────────┘
        │  GND    │   │  GND    │   │ OUT+ = 5.0V ──► ESP32 5V/VIN
        │  EEP◄3V3│   │  EEP◄3V3│   │ IN- / OUT- ──► common GND
        └─────────┘   └─────────┘   └─────────┘
             ▲IN1..IN4     ▲IN1..IN4
             │ (signals)   │ (signals)          ESP32 3V3 ──► both EEP pins
             └── from ESP32 GPIO ──┘
```

> **Bench-testing shortcut:** for now you can skip the MT3608 and just power the ESP32 from **USB**, battery only feeding the DRV8833s. The MT3608 is only for running untethered (P2). Set the MT3608 output to **5.0 V with a multimeter BEFORE** connecting it to the ESP32 — it ships at a random voltage.

## Power connections

| From | To | Note |
|---|---|---|
| Charge board **VBAT+** | DRV8833 #1 **VCC** | motor supply ~3.7V |
| Charge board **VBAT+** | DRV8833 #2 **VCC** | |
| Charge board **VBAT+** | MT3608 **IN+** | (untethered power; skip on USB bench test) |
| MT3608 **OUT+** (set 5.0V) | ESP32 **5V / VIN** | NOT the 3V3 pin |
| ESP32 **3V3** | DRV8833 #1 **EEP** | enable (active high) |
| ESP32 **3V3** | DRV8833 #2 **EEP** | enable (active high) |
| **Common GND** | charge board GND, both DRV8833 GND, MT3608 IN-/OUT-, ESP32 GND | ⚠️ all tied together |

## Signal connections (ESP32 GPIO → DRV8833 inputs)

**DRV8833 #1 = LEFT side**
| ESP32 GPIO | DRV8833 #1 pin | Drives |
|---|---|---|
| 25 | IN1 | Left-Front (OUT1/OUT2) |
| 26 | IN2 | Left-Front |
| 27 | IN3 | Left-Rear (OUT3/OUT4) |
| 14 | IN4 | Left-Rear |

**DRV8833 #2 = RIGHT side**
| ESP32 GPIO | DRV8833 #2 pin | Drives |
|---|---|---|
| 32 | IN1 | Right-Front (OUT1/OUT2) |
| 33 | IN2 | Right-Front |
| 13 | IN3 | Right-Rear (OUT3/OUT4) |
| 23 | IN4 | Right-Rear |

## Motors (freed from the old motor board → DRV8833 outputs)

| Motor | DRV8833 | Output pins |
|---|---|---|
| Left-Front  | #1 | OUT1, OUT2 |
| Left-Rear   | #1 | OUT3, OUT4 |
| Right-Front | #2 | OUT1, OUT2 |
| Right-Rear  | #2 | OUT3, OUT4 |

Motor polarity doesn't matter for wiring — if a motor spins the wrong way, we flip it in software. Just get each motor on the **correct side** (left pair → board #1, right pair → board #2).

## Power-up sequence
1. Double-check: common ground everywhere, EEP→3V3, no VCC↔GND short (meter it).
2. **USB into ESP32** first.
3. **Battery** last (or the charge board if the cell is already on it).
4. Open the control pad at the ESP32's IP, hold an arrow.

## Power-down: **battery/charge off → then USB.**
