#!/usr/bin/env python3
"""
BLE recon for the robot dog toy.
Step 1: scan, find the device.
Step 2: connect, dump all services/characteristics + properties.

Setup:
    python3 -m venv .venv && source .venv/bin/activate
    pip install bleak

Usage:
    python ble_recon.py                 # scan only, list nearby BLE devices
    python ble_recon.py --connect UUID  # connect + enumerate (macOS gives UUID not MAC)
    python ble_recon.py --connect UUID --listen  # also subscribe to NOTIFY chars (telemetry)

Why bleak: cross-platform, async, handles macOS's opaque-UUID addressing (no MAC exposed).
"""
import argparse
import asyncio
from bleak import BleakScanner, BleakClient

# BLE property flags worth flagging in output
CMD_PROPS = {"write", "write-without-response"}   # <- your command channel candidates
TELEM_PROPS = {"notify", "indicate"}              # <- battery / sensor telemetry candidates


async def scan(timeout: float = 8.0):
    print(f"scanning {timeout}s...\n")
    devices = await BleakScanner.discover(timeout=timeout, return_adv=True)
    if not devices:
        print("nothing found. is the dog on + not already connected to the phone app?")
        return
    # sort by RSSI (strongest = closest = probably your dog)
    rows = sorted(devices.values(), key=lambda d: d[1].rssi, reverse=True)
    for dev, adv in rows:
        name = dev.name or adv.local_name or "(no name)"
        print(f"{dev.address}  rssi={adv.rssi:>4}  {name}")
        if adv.service_uuids:
            print(f"    services: {adv.service_uuids}")
    print("\ngrab the address/UUID of your dog, then: --connect <that>")


async def enumerate_device(address: str, listen: bool):
    print(f"connecting to {address}...\n")
    device = await BleakScanner.find_device_by_address(address, timeout=15.0)
    if device is None:
        print("not found in scan window — dog asleep/advertising stopped. wake it, keep app closed, retry.")
        return
    async with BleakClient(device) as client:
        print(f"connected: {client.is_connected}\n")
        for service in client.services:
            print(f"[service] {service.uuid}  {service.description}")
            for ch in service.characteristics:
                props = set(ch.properties)
                tag = ""
                if props & CMD_PROPS:
                    tag = "  <-- COMMAND CHANNEL (write here)"
                elif props & TELEM_PROPS:
                    tag = "  <-- TELEMETRY (subscribe to sniff)"
                print(f"    [char] {ch.uuid}  props={sorted(props)}{tag}")
                # try reading readable chars for free intel (fw version, name, etc.)
                if "read" in props:
                    try:
                        val = await client.read_gatt_char(ch.uuid)
                        print(f"           read: {val.hex(' ')}  ascii={val!r}")
                    except Exception as e:
                        print(f"           read failed: {e}")
            print()

        if listen:
            print("subscribing to all NOTIFY/INDICATE chars. drive the dog now.")
            print("watch for packets = telemetry protocol. ctrl-c to stop.\n")

            def cb(sender, data: bytearray):
                print(f"  NOTIFY {sender}: {data.hex(' ')}")

            subscribed = []
            for service in client.services:
                for ch in service.characteristics:
                    if set(ch.properties) & TELEM_PROPS:
                        try:
                            await client.start_notify(ch.uuid, cb)
                            subscribed.append(ch.uuid)
                        except Exception as e:
                            print(f"  couldn't subscribe {ch.uuid}: {e}")
            if not subscribed:
                print("  no notify chars found.")
                return
            try:
                await asyncio.sleep(3600)
            except asyncio.CancelledError:
                pass


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--connect", metavar="ADDR", help="device address/UUID to enumerate")
    p.add_argument("--listen", action="store_true", help="subscribe to telemetry chars")
    p.add_argument("--timeout", type=float, default=8.0, help="scan duration")
    args = p.parse_args()

    if args.connect:
        asyncio.run(enumerate_device(args.connect, args.listen))
    else:
        asyncio.run(scan(args.timeout))


if __name__ == "__main__":
    main()