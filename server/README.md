# Tonepie history server

Optional companion of the firmware: the ESP posts its state here, the database keeps every visit forever, and the home page on the ESP shows the long-term history (*History* card, cat pop-up beyond the ESP's 64 visits). Without it the litter box works exactly the same.

ASP.NET Core 10 minimal API + Entity Framework Core 10. SQLite by default (one file), PostgreSQL and SQL Server ready: each provider has its own migrations, applied automatically at start-up.

## Run it with Docker

```bash
cp .env.example .env      # then put a long random TONEPIE_API_KEY in it
docker compose up -d --build
```

The API listens on port **8090**, data lives in `./data/tonepie.db`. Then, on the home page of the litter box: gear → *History server* → enable, address `http://<server-ip>:8090`, same API key → Save. The status line under the switch shows the last successful upload or the error.

Other databases: set `Tonepie__Provider` (`Sqlite`, `Postgres`, `SqlServer`) and `ConnectionStrings__Tonepie` in `docker-compose.yml`.

## API

| Endpoint | |
|---|---|
| `POST /api/ingest` | ESP snapshot (cats, bin, litter, last 64 visits, deleted visits). Needs `Authorization: Bearer <key>`. Idempotent: resending the same snapshot changes nothing |
| `GET /api/history?device=<id>&days=<n>` | visits per local day and cat name, average weight, bag changes and litter top-ups; `days=0` = everything |
| `GET /api/devices` | known devices |
| `GET /health` | liveness |

History reads are open to the local network (CORS `*`, GET only) because the page served by the ESP reads them from the browser; only writes need the key. Do not expose the port to the internet.

### How the ESP's data is merged

- A visit is identified by the ESP's visit number **and** its time, so numbers that restart after an erased ESP never overwrite old visits.
- Corrections made on the ESP (cat reassigned, visit deleted) update the stored row; deleted visits stay in the table flagged `Deleted` and are excluded from the history.
- Visits that left the ESP's memory (older than its last 64) are kept as they are.
- History is grouped by cat **name**, because the ESP renumbers cats when one is removed.
- Days are local days of `Tonepie__TimeZone` (default `Europe/Rome`).

## Development

```bash
dotnet test                                   # integration tests (WebApplicationFactory + SQLite)
dotnet run --project Tonepie.Server           # http://localhost:5000, needs Tonepie:ApiKey (user secrets or env)
dotnet tool restore
dotnet ef migrations add <Name> --project Tonepie.Server --context SqliteTonepieDb --output-dir Migrations/Sqlite
```

After a model change, add a migration for each provider (`SqliteTonepieDb`, `PostgresTonepieDb`, `SqlServerTonepieDb`, output dirs `Migrations/Sqlite|Postgres|SqlServer`).

`tools/demo_history.py` (repository root) fills a server with a year of fake visits for the device `tonepie-demo`, which the preview server (`tools/preview.py`) shows.
