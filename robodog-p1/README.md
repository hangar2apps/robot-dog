# RoboDog — Phase 1 (drive + web pad)

Goal: ESP32 replaces the stock board, you drive the 4 legs from your phone browser.
Power for P1: **ESP32 over USB** (from laptop/charger), **motors off the dog battery** via the DRV8833s.
The MT3608 boost is NOT needed yet — that's for untethered roaming (P2).

## 1. Wiring

### Two DRV8833 boards (one per side)
Each DRV8833 drives 2 motors. Left board = both left legs, Right board = both right legs.

**Power (both DRV8833 boards):**
| DRV8833 pin | Connect to |
|---|---|
| VCC (VM / motor V+) | Dog battery **+** (3.7V) |
| GND | Dog battery **−** AND ESP32 GND (common ground — REQUIRED) |
| STBY / EEP | 3V3 on ESP32 (pull high to enable; some boards have it tied high already) |

> ⚠️ Common ground is the #1 gotcha: ESP32 GND, both DRV8833 GNDs, and battery − must all connect. Without it, nothing moves.

**Signal (ESP32 GPIO -> DRV8833 inputs):**
| Motor | ESP32 pins | DRV8833 |
|---|---|---|
| Left-Front  | 25, 26 | Board #1 AIN1, AIN2 |
| Left-Rear   | 27, 14 | Board #1 BIN1, BIN2 |
| Right-Front | 32, 33 | Board #2 AIN1, AIN2 |
| Right-Rear  | 13, 23 | Board #2 BIN1, BIN2 |

**Motors -> DRV8833 outputs:** each motor's 2 wires go to that channel's AOUT1/AOUT2 (or BOUT1/BOUT2).
Direction is fixable in software later — don't worry about polarity yet.

### ESP32 power (P1)
Just plug the ESP32 into USB. Leave the MT3608 boost aside for now.

## 2. Flash it (PlatformIO)

1. Open this folder in VS Code with the PlatformIO extension.
2. Edit `src/main.cpp` — set `WIFI_SSID` / `WIFI_PASS` to your home WiFi.
3. Plug in ESP32 via USB. Click PlatformIO **Upload** (→ arrow, bottom bar).
   - If it hangs at "Connecting...", hold the ESP32 **BOOT** button until it starts writing.
4. Open the **Serial Monitor** (plug icon). It prints the IP, e.g. `http://192.168.1.42`.
   - If home WiFi fails, it makes its own network: join WiFi **RoboDog-AP** (pass `robodog123`), then open `http://192.168.4.1`.

## 3. Drive
Open that URL on your phone (same WiFi, or joined to RoboDog-AP). Hold the arrows. Legs should move.

**First-move checklist:**
- Nothing moves → check common ground, and STBY pulled high.
- One side backwards → we'll flip that motor's pins in code (tell me which).
- Runs then stops after ~0.6s → that's the safety watchdog; the web pad re-sends while held. Expected.

## 4. The JSON API (for rodrigo / P3)
Same endpoints the web pad uses — point rodrigo's tools here:
- `POST /drive` body `{"left": -255..255, "right": -255..255}` (+ = forward)
- `POST /stop`
- `GET /state` → `{"left":..,"right":..,"uptime":..}`

Example:
```bash
curl -X POST http://<dog-ip>/drive -H 'Content-Type: application/json' -d '{"left":200,"right":200}'
```

## Next (P2): add HC-SR04 (front), reactive wander+avoid on-device, MT3608 for untethered power.
