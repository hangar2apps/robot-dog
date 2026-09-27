# IoT Security Assessment — App/RC "Smart Robot Dog" (STEM Toy)

**Assessor:** Bryan Rigsby
**Date:** 2025-07-04
**Classification:** Personal research / study artifact
**Device:** BLE + 2.4 GHz "Smart Robot Dog for Kids" (Amazon ASIN B0FL6GTHL4)
**Radio/MCU board:** `TS-1300RA` (mfg 2025-07-08)

> **Scope & ethics:** Device is personally owned. Testing is on my own hardware, no cloud/backend interaction beyond the vendor app on my own phone, no third-party systems touched. This is the correct legal posture for embedded security research — own the DUT, stay off shared infrastructure.

---

## 1. Objective

Reverse-engineer a consumer IoT toy end-to-end as a hands-on embedded-security exercise: wireless recon → protocol capture → firmware extraction. Cheap consumer IoT is a near-ideal lab — no legal gray area, no scope boundaries, and it reliably exhibits the same defect classes as expensive "real" IoT (no BLE pairing, no secure boot, no flash encryption, plaintext secrets).

Findings are tracked as **Confirmed** (observed), **Inferred** (strong evidence, unverified), or **Pending** (test not yet run). Honest state-tracking is the point of a findings register — don't promote inferred to confirmed.

---

## 2. Methodology → OWASP FSTM mapping

The teardown maps cleanly onto the OWASP **Firmware Security Testing Methodology** stages:

| FSTM stage | This engagement | Status |
|---|---|---|
| 1. Info gathering & recon | Board/chip ID, radio ID, debug-port ID | In progress |
| 2. Obtaining firmware | SWD dump via exposed debug header | Pending (RDP-dependent) |
| 3–5. Analyze firmware/filesystem | strings/entropy on dumped image; hunt hardcoded creds/keys | Pending |
| 7–8. Dynamic/runtime analysis | BLE traffic capture + replay; GPIO bus sniffing | Pending |

Wireless layer additionally covered by **BLE-specific attack classes** (advertising recon, GATT enumeration, sniff/replay).

---

## 3. Device architecture (hardware recon)

Three-board stack, harness-connected:

- **Radio/MCU board — `TS-1300RA`** — the brains + radio. **Target of assessment.**
- **Motor/power board** — dual motor drivers, 16 MHz crystal, buck regulator, `ANT` pad. Not security-relevant.
- **`7MM` power/charge board** — `+5V / VBAT / GND`. Not security-relevant.

### Radio/MCU board detail
- **Exposed debug header (Confirmed):** labeled pads `GND / SWC / SWD / VDD` → ARM **Serial Wire Debug**. Unpopulated but silkscreened and accessible.
- **GPIO test pads (Confirmed):** `PI0 / PI1 / PC2 / PWMN` broken out → MCU→motor-driver control lines exposed for probing.
- **SoC identity (Pending):** top marking not yet read (photographed reverse side). Determines firmware toolchain and RDP behavior.
- **Radio protocol (Inferred):** app control + RC remote + discrete `ANT` → BLE/2.4 GHz combo SoC. App path assumed standard BLE GATT; unverified until scan.

---

## 4. Findings register → OWASP IoT Top 10

| ID | Finding | State | OWASP IoT (2018) | Severity (lab) |
|---|---|---|---|---|
| F-01 | **Exposed, unlocked-by-default debug interface.** SWD pads silkscreened and physically accessible with no tamper protection. If RDP is disabled (typical for cost-down toys), full firmware read/write via a $3 ST-Link. | Confirmed (access) / Pending (RDP) | I10 Lack of physical hardening; I9 Insecure default settings | Med–High |
| F-02 | **No secure/authenticated update mechanism (assumed).** Consumer toys at this tier ship without signed firmware; SWD reflash implies no boot-time integrity check. | Inferred | I4 Lack of secure update mechanism | Med |
| F-03 | **Unauthenticated BLE control (hypothesis).** If the app connects with no bonding/encryption and writes plaintext command bytes, any nearby central can enumerate + drive the device. This is *the* canonical cheap-BLE flaw. | Pending | I2 Insecure network services; I3 Insecure ecosystem interfaces | High if confirmed |
| F-04 | **Plaintext secrets in flash (hypothesis).** Cheap SoCs commonly leave WiFi creds / API tokens / device keys unencrypted in flash. Testable only after F-01 dump. | Pending | I1 Hardcoded creds; I7 Insecure data storage | TBD |
| F-05 | **Exposed internal bus/GPIO.** `PWMN/PI0/PI1` allow logic-analyzer capture of internal command→motion mapping — a side channel bypassing the radio entirely. | Confirmed (access) | I10 Lack of physical hardening | Low (info) |

---

## 5. Test plan (next actions)

1. **BLE recon** — `bleak` scan (app closed) to confirm the device advertises and grab its address/UUID. Arbiter for F-03's whole path.
2. **GATT enumeration** — dump services/characteristics; flag `WRITE`/`WRITE_NO_RESPONSE` (command) vs `NOTIFY` (telemetry). Note absence of pairing/encryption = F-03 evidence.
3. **Traffic capture + replay** — Android HCI snoop → Wireshark `btatt` → map button→payload. A successful replay of a captured "move" packet **confirms F-03** (unauthenticated write / no anti-replay).
4. **SoC ID** — read top marking → select toolchain; determine RDP posture.
5. **Firmware extraction** — ST-Link `read_flash`. If RDP blocks read: document the control as *present* (a positive finding for the vendor). Note: mass-erase to bypass RDP is destructive — no stock-firmware recovery.
6. **Firmware analysis** — `strings`, entropy scan, `binwalk` → hunt F-04 secrets.
7. **(Optional) GPIO side-channel** — logic analyzer on `PWMN/PI0/PI1` for internal protocol.

---

## 6. Tooling

| Purpose | Tool | Cost |
|---|---|---|
| BLE recon/enum | nRF Connect; `bleak` (Python) | free |
| Traffic capture | Android HCI snoop log; Wireshark | free |
| Firmware R/W | ST-Link v2 clone + OpenOCD / `esptool` (chip-dependent) | ~$3 |
| Firmware analysis | `binwalk`, `strings`, `radare2`/Ghidra | free |
| Bus sniffing | 8-ch logic analyzer (Saleae clone) + `sigrok`/PulseView | ~$10 |

---

## 7. References (frameworks to cite in the writeup)

- **OWASP IoT Top 10 (2018)** — device-level defect taxonomy.
- **OWASP FSTM** — Firmware Security Testing Methodology, 9-stage process.
- **OWASP IoTGoat** — deliberately vulnerable firmware for practice.
- BLE attack literature — advertising recon, GATT fuzzing, sniff/replay (unauthenticated writes).

---

## 8. Takeaway

The two highest-value findings on toys like this are almost always **F-01 (open debug port)** and **F-03 (unauthenticated BLE)** — and they're findings *before you write a single exploit*, purely from recon. That's the lesson for the studies: most embedded "vulns" are missing controls, not clever breaks. Recon *is* the assessment.