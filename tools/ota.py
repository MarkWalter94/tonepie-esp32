"""Aggiorna il firmware via rete, senza cavo USB.

    python tools/ota.py [indirizzo] [file.bin]

Predefiniti: http://192.168.178.94 e .pio/build/tonepie-c3/firmware.bin
(compila prima con `pio run -e tonepie-c3`). La password di aggiornamento
viene letta da include/secrets.h e non viene mostrata.
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
    print(f"Versione attuale: {state['firmware']} · invio {image.name} ({len(data)} byte)")
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
        print("Rifiutato:", json.load(error).get("message", error.reason))
        return 1

    for _ in range(20):  # the module restarts on the new firmware
        time.sleep(2)
        try:
            state = get_state(base)
            if state.get("uptime_s", 999) < 60:
                print(f"Riavviato con la versione {state['firmware']}")
                return 0
        except OSError:
            pass
    print("Il modulo non è tornato raggiungibile: controlla l'alimentazione e la rete.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
