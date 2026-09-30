"""Update the firmware over the network, without the USB cable.

    python tools/ota.py [address] [file.bin]

Defaults: http://192.168.178.94 and .pio/build/tonepie-c3/firmware.bin
(build first with `pio run -e tonepie-c3`). The update password is read
from include/secrets.h and never printed.
"""
import json
import re
import sys
import time
import urllib.error
import urllib.request
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OPENER = urllib.request.build_opener(urllib.request.ProxyHandler({}))


def get_state(base):
    with OPENER.open(base + "/api/state", timeout=8) as response:
        return json.load(response)


def main():
    base = (sys.argv[1] if len(sys.argv) > 1 else "http://192.168.178.94").rstrip("/")
    if not base.startswith("http"):
        base = "http://" + base
    image = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / ".pio" / "build" / "tonepie-c3" / "firmware.bin"
    config = (ROOT / "include" / "secrets.h").read_text(encoding="utf-8")
    password = re.search(r'OTA_PASSWORD\[\]\s*=\s*"([^"]+)"', config).group(1)
    data = image.read_bytes()

    state = get_state(base)
    print(f"Running version: {state['firmware']} - sending {image.name} ({len(data)} bytes)")
    boundary = uuid.uuid4().hex
    body = (f"--{boundary}\r\nContent-Disposition: form-data; name=\"firmware\"; filename=\"firmware.bin\"\r\n"
            "Content-Type: application/octet-stream\r\n\r\n").encode() + data + f"\r\n--{boundary}--\r\n".encode()
    request = urllib.request.Request(base + "/api/update", data=body, method="POST", headers={
        "Content-Type": f"multipart/form-data; boundary={boundary}",
        "X-Tonepie-Token": state["token"], "X-Tonepie-Ota": password})
    try:
        with OPENER.open(request, timeout=120) as response:
            print(json.load(response).get("message", ""))
    except urllib.error.HTTPError as error:
        print("Rejected:", json.load(error).get("message", error.reason))
        return 1

    for _ in range(20):  # the module restarts on the new firmware
        time.sleep(2)
        try:
            state = get_state(base)
            if state.get("uptime_s", 999) < 60:
                print(f"Rebooted with version {state['firmware']}")
                return 0
        except OSError:
            pass
    print("The module did not come back: check power and network.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
