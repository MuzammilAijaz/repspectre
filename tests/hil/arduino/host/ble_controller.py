#*****************************************************************************
# Interface to control bluetooth on the host device using bleak
#-----------------------------------------------------------------------------
# TODO:
# -----
#  - establish consistent api for either : exceptions on timeouts, or
#    simply return elapsed time; using exceptions causes script to fully
#    stop further execution.
#
#*****************************************************************************

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

        def detection_trigger_notify_callback(device, advertisement_data):
            nonlocal target
            print("FOUND:", device.name, device.address)
            if device.name == name:
                target = device
                event.set()

        scanner = bleak.BleakScanner(detection_trigger_notify_callback)
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

    async def subscribe(self, characteristic_uuid):
        """
        Subscribe to notifications.
        """
        self._notification_event = asyncio.Event()
        self._notification_data = None

        def cb(_, data):
            self._notification_data = bytes(data)
            self._notification_event.set()

        await self.client.start_notify(characteristic_uuid, cb)

        # allow CCCD propagation
        await asyncio.sleep(0.5)


    async def collect_notification(
        self,
        trigger_fn,
        timeout_s=5
    ):
        """
        Trigger a notification and wait for payload.
        """

        # reset BEFORE trigger
        self._notification_data = None
        self._notification_event.clear()

        # trigger notify
        trigger_fn()

        # wait for callback
        try:
            await asyncio.wait_for(
                self._notification_event.wait(),
                timeout=timeout_s
            )
        except asyncio.TimeoutError:
            return b''

        return self._notification_data


    async def unsubscribe(self, characteristic_uuid):
        await self.client.stop_notify(characteristic_uuid)

    async def write_characteristic(self, characteristic_uuid, data):
        await self.client.write_gatt_char(characteristic_uuid, data)

    async def read_characteristic(self, characteristic_uuid):
        return await self.client.read_gatt_char(characteristic_uuid)

    async def disconnect(self):
        if self.client:
            await self.client.disconnect()

    # Function that directly connects without scanning.
    async def reconnect(self, timeout_s=1):
        start = time.monotonic()
        # Attempt direct connect without scanning
        try:
            await asyncio.wait_for(
                    self.client.connect(),
                    timeout=timeout_s
                    )
            print("Reconnect time:", time.monotonic() - start)
            if not self.client.is_connected:
                raise RuntimeError("Connection failed")
        except asyncio.TimeoutError:
            raise RuntimeError(
                    f"Couldn't connect within {timeout_s} seconds."
            )
        finally:
            pass

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
