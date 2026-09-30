"""Local preview of the firmware's web pages, with fake data.

    python tools/preview.py [port]

Serves the home page (/) and the developer page (/dev) extracted from the C++
headers, without an ESP or a litter box. The fake API answers in Italian like
the firmware does. Scenarios: /mock/demo, /mock/empty, /mock/full.
"""
import json
import random
import re
import sys
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs

ROOT = Path(__file__).resolve().parent.parent


def page(header):
    text = (ROOT / "include" / header).read_text(encoding="utf-8")
    return re.search(r'R"HTML\((.*)\)HTML"', text, re.S).group(1).encode("utf-8")


def demo():
    random.seed(7)
    now = int(time.time())
    cats = [{"name": "Micio", "weight_g": 4200, "color": 0}, {"name": "Luna", "weight_g": 5600, "color": 1}]
    visits, t, vid = [], now - 40 * 60, 200
    while t > now - 7 * 86400:
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
        "config": {"tolerance_g": 500, "grams_per_visit": 50, "bin_limit_g": 1500, "cats": cats},
        "bin": {"g": 850, "since": now - 3 * 86400 - 5000, "visits": 17},
        "visits": visits,
    }


def empty():
    return {"config": {"tolerance_g": 500, "grams_per_visit": 50, "bin_limit_g": 1500, "cats": []},
            "bin": {"g": 0, "since": 0, "visits": 0}, "visits": []}


STATE = demo()
LITTER_AT = int(time.time()) - 12 * 86400
MCU = {"online": True, "ready": True, "pending": False, "presence": False, "fault": 0, "lock": False,
       "auto": True, "wait_min": 0, "odor": True}


class Handler(BaseHTTPRequestHandler):
    def send(self, code, body, kind="application/json"):
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
        elif path == "/api/home":
            self.send(200, dict(STATE, litter_at=LITTER_AT, firmware="anteprima", token="mock", now=int(time.time()),
                                mcu=MCU))
        elif path == "/api/state":
            self.send(200, {"firmware": "anteprima", "token": "mock", "ip": "127.0.0.1", "online": True, "ready": True,
                            "armed": False, "pending": False, "command": "Anteprima", "frames": 0, "bad_checksum": 0,
                            "bad_length": 0, "timeouts": 0, "bad_dp": 0, "heap": 0, "uptime_s": 0, "reset_reason": 1,
                            "dps": [], "logs": ["anteprima locale: nessuna MCU"]})
        elif path.startswith("/mock/"):
            STATE = {"demo": demo, "empty": empty}.get(path[6:], demo)()
            if path[6:] == "full":
                STATE["bin"].update(g=1580, visits=31)
            self.send(200, {"message": "scenario " + path[6:]})
        else:
            self.send(404, {"message": "Risorsa non trovata"})

    def do_POST(self):
        raw = self.rfile.read(int(self.headers.get("Content-Length") or 0)).decode("utf-8")
        path = self.path
        if path == "/api/config":
            data = json.loads(raw)
            old = STATE["config"]["cats"]
            for cat in data["cats"]:
                index = cat.pop("from", -1)
                source = old[index] if 0 <= index < len(old) else {}
                cat.update(days=source.get("days", []), grams=source.get("grams", []))
            STATE["config"] = {"cats": data["cats"], "tolerance_g": data["tolerance_g"],
                               "grams_per_visit": data["grams_per_visit"], "bin_limit_g": data["bin_limit_g"]}
            self.send(200, {"message": "Impostazioni salvate"})
        elif path == "/api/litter":
            global LITTER_AT
            LITTER_AT = int(time.time())
            self.send(200, {"message": "Aggiunta di lettiera registrata"})
        elif path == "/api/bin/reset":
            STATE["bin"] = {"g": 0, "since": int(time.time()), "visits": 0}
            self.send(200, {"message": "Cassetto svuotato: conteggio azzerato"})
        elif path == "/api/visit":
            form = {k: v[0] for k, v in parse_qs(raw).items()}
            visit = next((v for v in STATE["visits"] if str(v["id"]) == form.get("id")), None)
            if not visit:
                self.send(404, {"message": "Visita non trovata"})
            elif form.get("cat") == "delete":
                STATE["visits"].remove(visit)
                self.send(200, {"message": "Visita eliminata"})
            else:
                visit.update(cat=int(form["cat"]), manual=True)
                self.send(200, {"message": "Visita aggiornata"})
        elif path in ("/api/command", "/api/value"):
            form = {k: v[0] for k, v in parse_qs(raw).items()}
            action = form.get("action", "")
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
