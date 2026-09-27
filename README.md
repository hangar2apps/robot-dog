# robot-dog

Turning a cheap app/RC "smart robot dog" toy into an autonomous robot — first by reverse-engineering it, now by giving it a new brain.

## The story so far

**Phase A — the hack (done, archived in [`recon/`](recon/)).**
Reverse-engineered the stock toy: teardown, BLE recon, and firmware/debug analysis. Key finding — the vendor app talks to the dog over an **L2CAP dynamic channel (not standard GATT/ATT)**, which is why phone HCI snooping couldn't cleanly capture commands. Full writeup mapped to OWASP IoT/FSTM is in [`recon/assessment.md`](recon/assessment.md); the blow-by-blow is in [`recon/tried-so-far.md`](recon/tried-so-far.md).

**Phase B — the brain transplant (active).**
Rather than keep fighting the closed radio, replace the stock control board with an **ESP32** and drive the hardware directly. Goal: **autonomy** — a dog that wanders and avoids obstacles on its own, with a manual override.

## Hardware

- 4× brushed DC gear-motors (skid/tank steer — left pair + right pair), speaker, 3.7 V Li-ion cell (~1200 mAh)
- **ESP32** (external-antenna module) — new brain, WiFi + BLE
- **2× DRV8833** dual H-bridge — one per side (low dropout, good for 3.7 V)
- **HC-SR04** ultrasonic — front obstacle sensing (Phase 2; needs a divider on Echo → 3.3 V)
- **MT3608** boost 3.7 V→5 V — powers the ESP32 for untethered running (Phase 2)

## Build phases

| Phase | What | Status |
|---|---|---|
| **P1** | ESP32 drives the 4 motors; phone web control pad + JSON API | 🔨 in progress → [`robodog-p1/`](robodog-p1/) |
| **P2** | HC-SR04 + on-device reactive wander/avoid (fully offline); MT3608 for untethered power | planned |
| **P3** | Off-board agent (`rodrigo`, Claude tool-use) sets high-level intent via the P1 JSON API | planned |

Architecture: **reactive reflexes live on the ESP32** (always-on, safe if WiFi/agent drops); the **agent sets intent** over the network. One JSON API serves both the web pad and the agent.

## Quickstart (P1)

```bash
cd robodog-p1
cp include/secrets.h.example include/secrets.h   # add your WiFi
# open in VS Code + PlatformIO, Upload, then open the IP the serial monitor prints
```
Full wiring + flashing steps: [`robodog-p1/README.md`](robodog-p1/README.md).

## Layout

```
robodog-p1/   active ESP32 firmware (PlatformIO)
recon/        Phase A — the reverse-engineering work (assessment, recon scripts, log)
```
