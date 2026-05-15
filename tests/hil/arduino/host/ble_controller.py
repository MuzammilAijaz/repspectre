# Ble controller using Bleak

import bleak
import asyncio
import time
import subprocess

class BLEHost:

    def __init__(self):
        self.client = None

    async def scan_and_connect(self, name, scan_timeout_s=10, conn_timeout_s=3):
        """
        Params
        ------
        scan_timeout_s : Maximum time the scan would look for the specific
                         device, before failing (assertion).

        conn_timeout_s : Maximum time for connectio to establish.

        Return
        ------
        elapsed_scan_time : Time it took for scan to detect.
        elapsed_conn_time : Time it took for connection to establish.
        """
        target = None
        event = asyncio.Event()

        def detection_callback(device, advertisement_data):
            nonlocal target
            print("FOUND:", device.name, device.address)
            if device.name == name:
                target = device
                event.set()

        scanner = bleak.BleakScanner(detection_callback)
        start = time.monotonic()

        # find device using scan
        await scanner.start()
        try:
            await asyncio.wait_for(event.wait(), timeout=scan_timeout_s)
        except asyncio.TimeoutError:
            print(f"Couldn't discover device within :{scan_timeout_s} seconds.")
        finally:
            await scanner.stop()
            assert target is not None, (
                f"Couldn't discover device within {scan_timeout_s} seconds."
            )
        elapsed_scan_time = time.monotonic() - start

        self.client = bleak.BleakClient(target)

        # when device found, attempt connection
        start = time.monotonic()
        try:
            await asyncio.wait_for(
                self.client.connect(),
                timeout=conn_timeout_s
            )
            if not self.client.is_connected:
                raise RuntimeError("Connection failed")
        except asyncio.TimeoutError:
            raise RuntimeError(
                f"Couldn't connect within {conn_timeout_s} seconds."
            )
        finally:
            pass

        elapsed_conn_time = time.monotonic() - start
        return elapsed_scan_time, elapsed_conn_time

    async def disconnect(self):
        if self.client:
            await self.client.disconnect()

    # TODO: make it os/platform independent
    # WARN: specific to BlueZ stack, linux
    def get_host_ble_address(self):
        result = subprocess.run(["bluetoothctl", "show"], capture_output=True, text=True)
        for line in result.stdout.splitlines():
            line = line.strip()
            if line.startswith("Controller"):
                # Format: "Controller XX:XX:XX:XX:XX:XX (public)"
                return line.split()[1].lower()
        return None
