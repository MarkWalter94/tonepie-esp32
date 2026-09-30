"""Fills a history server with fake visits, to try the History section of the home page.

    python tools/demo_history.py [server-url] [days]

Posts snapshots the way the ESP does, as device "tonepie-demo". The API key is
read from server/.env (TONEPIE_API_KEY). Defaults: http://localhost:8090, 400 days.
"""
import json
import random
import sys
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def main():
    url = sys.argv[1].rstrip("/") if len(sys.argv) > 1 else "http://localhost:8090"
    days = int(sys.argv[2]) if len(sys.argv) > 2 else 400
    env = (ROOT / "server" / ".env").read_text(encoding="utf-8").splitlines()
    key = next(line.split("=", 1)[1].strip() for line in env if line.startswith("TONEPIE_API_KEY="))
    random.seed(3)
    now = int(time.time())
    cats = [{"index": 0, "name": "Micio", "color": 0, "weightG": 4200}, {"index": 1, "name": "Luna", "color": 1, "weightG": 5600}]
    visits, t, vid, bag = [], now - days * 86400, 1, now - days * 86400
    bags = []
    while t < now - 600:
        cat = random.choice([0, 1, 1, 0, -1] if random.random() < 0.05 else [0, 1])
        age = (now - t) / 86400
        grams = 0 if cat < 0 else int(cats[cat]["weightG"] + (-age * 1.5 if cat == 0 else age * 0.8) + random.gauss(0, 60))
        visits.append({"id": vid, "t": t, "g": max(grams, 0), "s": random.randint(30, 150), "cat": cat, "manual": False})
        vid += 1
        t += random.randint(3 * 3600, 10 * 3600)
        if t - bag > random.randint(6, 9) * 86400:
            bag = t
            bags.append(bag)
    sent = 0
    for i in range(0, len(visits), 64):
        chunk = visits[i:i + 64]
        since = max([b for b in bags if b <= chunk[-1]["t"]] or [0])
        snap = {"device": "tonepie-demo", "firmware": "demo", "now": now, "cats": cats,
                "bin": {"since": since, "visits": 0, "limit": 30}, "litterAt": 0, "visits": chunk, "deleted": []}
        req = urllib.request.Request(url + "/api/ingest", json.dumps(snap).encode(),
                                     {"Content-Type": "application/json", "Authorization": "Bearer " + key})
        sent += json.loads(urllib.request.urlopen(req).read())["changed"]
    print(f"{sent} visits and {len(bags)} bag changes sent to {url} as tonepie-demo")


if __name__ == "__main__":
    main()
