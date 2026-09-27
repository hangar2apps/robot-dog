# ble_id.py — find the dog by "disappears when app connects"
import asyncio
from bleak import BleakScanner

async def snap(label):
    print(f"\n[{label}] scanning 6s...")
    devs = await BleakScanner.discover(timeout=6.0, return_adv=True)
    return {addr: (adv.local_name or "N/A", adv.rssi) for addr,(d,adv) in devs.items()}

async def main():
    input("Dog ON, official app CLOSED. Enter to snapshot A...")
    a = await snap("A: app closed")
    a2 = await snap("A2: app closed (baseline churn)")
    input("Now OPEN the app + connect to the dog (drive it). Enter to snapshot B...")
    b = await snap("B: app connected")
    stable = set(a) & set(a2)          # present both closed-scans = not random churn
    gone = stable - set(b)              # ...and vanished once app grabbed it = DOG
    print("\n=== likely dog (stable, then vanished when app connected) ===")
    for addr in gone:
        print(f"  {addr}  name={a[addr][0]}  rssi={a[addr][1]}")
    if not gone: print("  none — dog may have been asleep in A. wake + retry.")

asyncio.run(main())