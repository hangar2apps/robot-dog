dog UUID - 4D97497A-6F15-FB7E-A434-0799C5BA535E


All four, sequenced. Here's the project. Each phase gates the next — don't skip.

Phase 0 — Passive recon (do first, non-destructive)
Photograph the FCC ID (molded/printed on the toy) → look it up at fcc.io/id/[ID]. FCC filings include internal photos + block diagrams. Free teardown = you'll often ID the BLE chip and radio before opening anything. Tells you if custom firmware is even feasible (ESP32/Nordic = yes; masked-ROM no-name SoC = probably not).

Phase 1 — BLE enumeration
nRF Connect app for the quick look, but you'll want programmatic access anyway. Starter script below scans + dumps every service/characteristic/property. Run it, drive the dog with the official app in the background, note which characteristic is WRITE/WRITE_NO_RESPONSE (command channel) vs NOTIFY (telemetry).

Phase 2 — Protocol capture
Android HCI snoop → Wireshark btatt filter → map every button to its payload bytes. This is the actual reverse-engineering. Watch for: a checksum byte (often last byte = XOR or sum of prior), a handshake packet on connect, a keepalive (dog disconnects/ignores you without it). Log battery/sensor notify packets separately.

Phase 3 — Your own controller
bleak for a Python/CLI or FastAPI wrapper; Web Bluetooth if you want a React control pad in-browser (Chrome only, no app/install — good demo). Once Phase 2 gives you the byte maps this is trivial.

Phase 4 — Custom firmware (risk: brick)
Only if Phase 0 shows a flashable MCU. Crack the case, find the debug/UART pads or SWD header, dump original firmware first (esptool.py read_flash for ESP32) so you can restore. Then flash your own. This voids everything and can permanently kill it — do it last, after you've extracted all the value from Phases 1–3.

Starter recon script:
Run scan first with the dog on but not connected to the phone app (BLE only allows one central — the app will hog it). Strongest RSSI is almost certainly your dog. Then --connect <addr> --listen.
Two things determine everything downstream:

Does it even show up as BLE? If nRF Connect / this script sees nothing but the phone controls it fine, it might be classic Bluetooth SPP or 2.4GHz proprietary (some cheap ones use a dongle-style RF chip) — different toolchain. Tell me what scan returns.
FCC ID from the label — send it and I'll pull the internal photos to ID the chip before you open it.