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
WINDOW = 64  # visits the ESP keeps, so the most a snapshot carries


def main():
    url = sys.argv[1].rstrip("/") if len(sys.argv) > 1 else "http://localhost:8090"
    days = int(sys.argv[2]) if len(sys.argv) > 2 else 400
    env = (ROOT / "server" / ".env").read_text(encoding="utf-8").splitlines()
    key = next((line.split("=", 1)[1].strip() for line in env if line.startswith("TONEPIE_API_KEY=")), "")
    if not key:
        sys.exit("TONEPIE_API_KEY is empty in server/.env")
    random.seed(3)
    now = int(time.time()) // 3600 * 3600  # same times when re-run within the hour: nothing changes then
    cats = [{"index": 0, "name": "Micio", "color": 0, "weightG": 4200}, {"index": 1, "name": "Luna", "color": 1, "weightG": 5600}]
    visits, t, bag = [], now - days * 86400, now - days * 86400
    bags = []  # (time, index of the first visit in the new bag)
    while t < now - 600:
        if t - bag > random.randint(6, 9) * 86400:
            bag = t
            bags.append((bag, len(visits)))
        cat = random.choice([0, 1, 1, 0, -1] if random.random() < 0.05 else [0, 1])
        age = (now - t) / 86400
        grams = 0 if cat < 0 else int(cats[cat]["weightG"] + (-age * 1.5 if cat == 0 else age * 0.8) + random.gauss(0, 60))
        visits.append({"id": len(visits) + 1, "t": t, "g": max(grams, 0), "s": random.randint(30, 150), "cat": cat, "manual": False})
        t += random.randint(3 * 3600, 10 * 3600)

    # The server records one bag change per distinct bin.since, so the ESP's state is posted right after
    # each bag change, plus every 64 visits so that no visit is skipped, plus once at the end.
    ends = sorted({i + 1 for _, i in bags} | set(range(WINDOW, len(visits), WINDOW)) | {len(visits)})
    changed = 0
    for end in ends:
        since = max([b for b, i in bags if i < end] or [0])
        shown = visits[max(0, end - WINDOW):end]
        snap = {"device": "tonepie-demo", "firmware": "demo", "now": visits[end - 1]["t"], "cats": cats,
                "bin": {"since": since, "visits": sum(1 for v in visits[:end] if v["t"] >= since), "limit": 30},
                "litterAt": 0, "visits": shown, "deleted": []}
        req = urllib.request.Request(url + "/api/ingest", json.dumps(snap).encode(),
                                     {"Content-Type": "application/json", "Authorization": "Bearer " + key})
        changed += json.loads(urllib.request.urlopen(req).read())["changed"]
    print(f"{len(visits)} visits and {len(bags)} bag changes posted to {url} as tonepie-demo "
          f"in {len(ends)} snapshots; {changed} visit rows added or changed on the server")


if __name__ == "__main__":
    main()
