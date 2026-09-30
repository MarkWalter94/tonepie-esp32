# Tonepie history server

Optional companion of the firmware: the ESP posts its state here, the database keeps every visit forever, and the home page on the ESP shows the long-term history (*History* card, cat pop-up beyond the ESP's 64 visits). Without it the litter box works exactly the same.

ASP.NET Core 10 minimal API + Entity Framework Core 10. SQLite by default (one file), PostgreSQL and SQL Server ready: each provider has its own migrations, applied automatically at start-up.

## Run it with Docker

```bash
cp .env.example .env      # then put a long random TONEPIE_API_KEY in it
docker compose up -d --build
```

The API listens on port **8090**. Data lives in the named Docker volume `tonepie-data` (`/data/tonepie.db` in the container; `docker volume inspect tonepie-data` shows where on the host). Back it up with the container stopped, so the SQLite file is consistent:

```bash
docker compose stop
docker run --rm -v tonepie-data:/data -v "$PWD":/backup alpine tar czf /backup/tonepie-data.tgz -C /data .
docker compose start
# restore: same, with  tar xzf /backup/tonepie-data.tgz -C /data  (then chown -R 1654 /data)
```

Coming from an older setup with a `./data` folder: stop the container, then copy the old files into the volume with `docker run --rm -v tonepie-data:/data -v "$PWD/data":/old alpine sh -c "cp -a /old/. /data/ && chown -R 1654 /data"`.

The image has a Docker `HEALTHCHECK` (the server binary calls its own `/health`); `docker ps` shows it as healthy / unhealthy.

Then, on the home page of the litter box: gear → *History server* → enable, address `http://<server-ip>:8090`, same API key → Save. The status line under the switch shows the last successful upload or the error.

Other databases: set `Tonepie__Provider` (`Sqlite`, `Postgres`, `SqlServer`) and `ConnectionStrings__Tonepie` in `docker-compose.yml`.

## API

| Endpoint | |
|---|---|
| `POST /api/ingest` | ESP snapshot (cats, bin, litter, last 64 visits, deleted visits), at most 64 KB. Idempotent: resending the same snapshot changes nothing |
| `GET /api/history?device=<id>&days=<n>` | visits per local day and cat name, average weight, bag changes and litter top-ups; `days=0` = everything |
| `GET /api/devices` | known devices |
| `GET /health` | no key; 200 when the database answers, 503 otherwise |

Every `/api` endpoint needs `Authorization: Bearer <key>` (checked before the body is read; 401 otherwise). The browser never calls the server directly: the ESP relays the history to its home page (`GET /api/history` on the ESP), so there is no CORS. The server refuses keys shorter than 16 characters and the old example value. Do not expose the port to the internet.

### How the ESP's data is merged

- A visit is identified by the ESP's visit number **and** its time, so numbers that restart after an erased ESP never overwrite old visits.
- Corrections made on the ESP (cat reassigned, visit deleted) update the stored row; deleted visits stay in the table flagged `Deleted` and are excluded from the history.
- A stored visit missing from the snapshot counts as deleted only if its number lies inside the snapshot's number window (which may wrap: 65530..65535, 1..20), after its first visit, and it is not older than the snapshot's oldest timed visit. Visits without a time are deleted only when listed explicitly.
- Visits that left the ESP's memory (older than its last 64) are kept as they are.
- History is grouped by cat **name**, because the ESP renumbers cats when one is removed. A blank name is stored as `Cat <n>`, names are cut at 24 characters.
- Snapshots of the same device are stored one at a time.
- Days are local days of `Tonepie__TimeZone` (default `Europe/Rome`); an invalid zone stops the start-up.

## Development

```bash
dotnet test                                   # integration tests (WebApplicationFactory + SQLite)
dotnet run --project Tonepie.Server           # http://localhost:5000, needs Tonepie:ApiKey (user secrets or env)
dotnet tool restore
dotnet ef migrations add <Name> --project Tonepie.Server --context SqliteTonepieDb --output-dir Migrations/Sqlite
```

After a model change, add a migration for each provider (`SqliteTonepieDb`, `PostgresTonepieDb`, `SqlServerTonepieDb`, output dirs `Migrations/Sqlite|Postgres|SqlServer`).

`tools/demo_history.py` (repository root) fills a server with a year of fake visits and bag changes for the device `tonepie-demo` (one snapshot per bag change, as the ESP would), which the preview server (`tools/preview.py`) shows.
