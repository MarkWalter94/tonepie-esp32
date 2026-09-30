"""Local preview of the firmware's web pages, with fake data.

    python tools/preview.py [port]

Serves the home page (/) and the developer page (/dev) extracted from the C++
headers, without an ESP or a litter box. Like the firmware, the fake API answers
in English when the page sends "X-Tonepie-Lang: en". Scenarios: /mock/demo,
/mock/empty, /mock/full.
"""
import json
import random
import re
import sys
import time
import urllib.error
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs

ROOT = Path(__file__).resolve().parent.parent


def page(header):
    text = (ROOT / "include" / header).read_text(encoding="utf-8")
    return re.search(r'R"HTML\((.*)\)HTML"', text, re.S).group(1).encode("utf-8")


def icons():
    """PNG icons from include/icons.h and the manifest from src/main.cpp."""
    text = (ROOT / "include" / "icons.h").read_text(encoding="utf-8")
    out = {}
    for size, body in re.findall(r"ICON_(\d+)\[\] PROGMEM = \{(.*?)\};", text, re.S):
        out[size] = bytes(int(b, 16) for b in re.findall(r"0x([0-9a-f]{2})", body))
    main = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
    manifest = re.search(r'MANIFEST\[\] PROGMEM = R"J\((.*?)\)J"', main, re.S).group(1).encode("utf-8")
    return out, manifest


ICONS, MANIFEST = icons()
EN = {"Impostazioni salvate": "Settings saved", "Aggiunta di lettiera registrata": "Litter top-up recorded",
      "Cassetto svuotato: conteggio azzerato": "Bin emptied: count reset", "Visita non trovata": "Visit not found",
      "Visita eliminata": "Visit deleted", "Visita aggiornata": "Visit updated", "Risorsa non trovata": "Not found",
      "Anteprima: impostazione finta aggiornata": "Preview: fake setting updated",
      "Anteprima: nessun comando inviato": "Preview: no command sent",
      "Impostazioni del server salvate": "Server settings saved"}


def demo():
    random.seed(7)
    now = int(time.time())
    cats = [{"name": "Micio", "weight_g": 4200, "color": 0}, {"name": "Luna", "weight_g": 5600, "color": 1}]
    visits, t, vid = [], now - 40 * 60, 200
    while t > now - 12 * 86400:
        cat = random.choice([0, 0, 1, 1, 1, -1] if len(visits) == 3 else [0, 1])
        grams = 0 if cat < 0 else cats[cat]["weight_g"] + random.choice([-100, 0, 0, 100])
        visits.append({"id": vid, "t": t, "g": grams, "s": random.randint(35, 160), "cat": cat, "manual": False})
        vid -= 1
        t -= random.randint(2 * 3600, 7 * 3600)
    today = now // 86400
    for cat, drift in zip(cats, (6, -4)):  # grams per day: one cat slowly gains, one slowly loses
        days = [d for d in range(today - 74, today + 1) if random.random() > 0.15]
        cat["days"] = days
        cat["grams"] = [round((cat["weight_g"] + drift * (d - today) + random.gauss(0, 45)) / 10) * 10 for d in days]
    return {
        "config": {"tolerance_g": 500, "bin_limit_visits": 30, "cats": cats},
        "bin": {"since": now - 3 * 86400 - 5000, "visits": 17},
        "visits": visits,
    }


def empty():
    return {"config": {"tolerance_g": 500, "bin_limit_visits": 30, "cats": []},
            "bin": {"since": 0, "visits": 0}, "visits": []}


STATE = demo()
LITTER_AT = int(time.time()) - 12 * 86400
# History server shown by the page: run the server (server/) and fill it with tools/demo_history.py.
SYNC = {"enabled": True, "url": "http://localhost:8090", "device": "tonepie-demo", "key_set": True,
        "ok_at": int(time.time()) - 120, "error": "", "pending": False}
MCU = {"online": True, "ready": True, "pending": False, "presence": False, "fault": 0, "lock": False,
       "auto": True, "wait_min": 0, "odor": True, "armed": False}
ACTIONS = {"clean", "empty", "auto_on", "auto_off", "bag", "level", "odor_on", "odor_off"}


def server_key():
    """The history server's key from server/.env, as the ESP would have it."""
    try:
        env = (ROOT / "server" / ".env").read_text(encoding="utf-8").splitlines()
        return next((line.split("=", 1)[1].strip() for line in env if line.startswith("TONEPIE_API_KEY=")), "")
    except OSError:
        return ""


class Handler(BaseHTTPRequestHandler):
    def send(self, code, body, kind="application/json"):
        if isinstance(body, dict) and "message" in body and self.headers.get("X-Tonepie-Lang") == "en":
            body = dict(body, message=EN.get(body["message"], body["message"]))
        if not isinstance(body, bytes):
            body = json.dumps(body).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", kind)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        global STATE
        path = self.path.split("?")[0]
        if path == "/":
            self.send(200, page("web_home.h"), "text/html; charset=utf-8")
        elif path == "/dev":
            self.send(200, page("web_ui.h"), "text/html; charset=utf-8")
        elif path in ("/apple-touch-icon.png", "/icon-192.png", "/icon-512.png"):
            self.send(200, ICONS["180" if "apple" in path else path[6:9]], "image/png")
        elif path == "/manifest.webmanifest":
            self.send(200, MANIFEST, "application/manifest+json")
        elif path == "/api/home":
            self.send(200, dict(STATE, litter_at=LITTER_AT, firmware="anteprima", token="mock", now=int(time.time()),
                                mcu=MCU, sync=SYNC))
        elif path == "/api/state":
            self.send(200, {"firmware": "anteprima", "token": "mock", "ip": "127.0.0.1", "online": True, "ready": True,
                            "armed": False, "pending": False, "command": "Anteprima", "frames": 0, "bad_checksum": 0,
                            "bad_length": 0, "timeouts": 0, "bad_dp": 0, "heap": 0, "uptime_s": 0, "reset_reason": 1,
                            "dps": [], "logs": ["anteprima locale: nessuna MCU"]})
        elif path == "/api/history":  # relayed to the history server with the key, like the ESP does
            if not SYNC["enabled"] or not SYNC["url"]:
                return self.send(404, {"message": "Server storico non configurato"})
            days = parse_qs(self.path.partition("?")[2]).get("days", ["90"])[0]
            days = str(min(int(days), 3660)) if days.isdigit() else "90"
            request = urllib.request.Request(f"{SYNC['url']}/api/history?device={SYNC['device']}&days={days}",
                                             headers={"Authorization": "Bearer " + server_key()})
            try:
                with urllib.request.urlopen(request, timeout=5) as r:
                    self.send(200, r.read())
            except urllib.error.HTTPError as e:
                self.send(404 if e.code == 404 else 502, {"message": f"Server storico: HTTP {e.code}"})
            except OSError as e:
                self.send(502, {"message": f"Server storico non raggiungibile: {e}"})
        elif path.startswith("/mock/"):
            STATE = {"demo": demo, "empty": empty}.get(path[6:], demo)()
            if path[6:] == "full":
                STATE["bin"].update(visits=31)
            self.send(200, {"message": "scenario " + path[6:]})
        else:
            self.send(404, {"message": "Risorsa non trovata"})

    def do_POST(self):
        raw = self.rfile.read(int(self.headers.get("Content-Length") or 0)).decode("utf-8")
        path = self.path
        if path == "/api/config":
            data = json.loads(raw)
            old = STATE["config"]["cats"]
            moved = {}  # old index -> new index, as remapCats does in the firmware
            for i, cat in enumerate(data["cats"]):
                index = cat.pop("from", -1)
                source = old[index] if 0 <= index < len(old) else {}
                if source:
                    moved[index] = i
                cat.update(days=source.get("days", []), grams=source.get("grams", []))
            for visit in STATE["visits"]:
                if visit["cat"] >= 0:
                    visit["cat"] = moved.get(visit["cat"], -1)
                    if visit["cat"] < 0:
                        visit["manual"] = False
            STATE["config"] = {"cats": data["cats"], "tolerance_g": data["tolerance_g"],
                               "bin_limit_visits": data["bin_limit_visits"]}
            self.send(200, {"message": "Impostazioni salvate"})
        elif path == "/api/litter":
            global LITTER_AT
            LITTER_AT = int(time.time())
            self.send(200, {"message": "Aggiunta di lettiera registrata"})
        elif path == "/api/bin/reset":
            STATE["bin"] = {"since": int(time.time()), "visits": 0}
            self.send(200, {"message": "Cassetto svuotato: conteggio azzerato"})
        elif path == "/api/visit":
            form = {k: v[0] for k, v in parse_qs(raw).items()}
            visit = next((v for v in STATE["visits"] if str(v["id"]) == form.get("id")), None)
            if not visit:
                self.send(404, {"message": "Visita non trovata"})
            elif form.get("cat") == "delete":
                STATE["visits"].remove(visit)
                if not (visit["t"] and STATE["bin"]["since"] and visit["t"] < STATE["bin"]["since"]):
                    STATE["bin"]["visits"] = max(0, STATE["bin"]["visits"] - 1)
                self.send(200, {"message": "Visita eliminata"})
            else:
                visit.update(cat=int(form["cat"]), manual=True)
                self.send(200, {"message": "Visita aggiornata"})
        elif path == "/api/sync":
            form = {k: v[0] for k, v in parse_qs(raw).items()}
            SYNC.update(enabled=form.get("enabled") == "1", url=form.get("url", ""),
                        key_set=SYNC["key_set"] or bool(form.get("key")))
            self.send(200, {"message": "Impostazioni del server salvate"})
        elif path == "/api/arm":
            MCU["armed"] = parse_qs(raw).get("enabled", ["0"])[0] == "1"
            self.send(200, {"message": "Invii abilitati" if MCU["armed"] else "Invii disabilitati"})
        elif path in ("/api/command", "/api/value"):
            form = {k: v[0] for k, v in parse_qs(raw).items()}
            action = form.get("action", "")
            if not MCU["armed"]:
                return self.send(409, {"message": "Abilita gli invii dalla dashboard"})
            if path == "/api/command" and action not in ACTIONS:
                return self.send(400, {"message": "Comando non ammesso"})
            if path == "/api/value" and not (form.get("dp") in ("117", "118") and form.get("value", "").isdigit()):
                return self.send(400, {"message": "Intervallo valido: DP117 0-60; DP118 0-120 minuti"})
            if action.startswith("auto_"):
                MCU["auto"] = action == "auto_on"
            elif action.startswith("odor_"):
                MCU["odor"] = action == "odor_on"
            elif form.get("dp") == "117":
                MCU["wait_min"] = int(form["value"])
            self.send(202, {"message": "Anteprima: impostazione finta aggiornata"})
        else:
            self.send(200, {"message": "Anteprima: nessun comando inviato"})

    def log_message(self, *args):
        pass


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8765
    print(f"Preview at http://localhost:{port}")
    ThreadingHTTPServer(("127.0.0.1", port), Handler).serve_forever()
