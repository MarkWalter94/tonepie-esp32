# Validation log

What was actually tested, on which day, and what was not. Software tests and successful builds do not make this a field-proven firmware: the litter box's movements have not been commanded from it yet.

## Bench setup

Ai-Thinker ESP-C3-13-Kit (ESP32-C3 rev 0.3, CH340 on COM6), Tonepie Ti Pro 25 main board with the WBR3 removed, UART1 on GPIO6/7 at 115200 8N1. FRITZ!Box router, module at `192.168.178.94`. The main board was partly disconnected from motors and sensors during these sessions.

## 30 September 2026

### 1.1.0 — UART moved to GPIO6/7 (previous agent)

- Built and flashed; version and pins confirmed over HTTP; CH340 log readable.
- First MCU contact: handshake (heartbeat, product, mode, network, query) completed, 51 valid frames, 19 datapoints, zero checksum/length/TLV errors, zero parser timeouts. Capture in `diagnostics/mcu-first-contact.json`. Product id `yn6wqmizg7abe5k8`, MCU firmware 1.0.15.
- DP24 is reported twice in every query response, first as BOOL `true` then as ENUM `6`; the cache keeps the last one.
- Host protocol tests pass: known frames, noise, bad checksums, concatenated and fragmented frames, maximum payload, oversized lengths, timeout recovery, malformed TLVs, `millis()` wrap, one million pseudo-random bytes.

### 1.2.0 — home page

- Build: RAM 53,468 / 327,680 B, flash 850,018 / 1,310,720 B. Flashed over USB, hashes verified.
- `/`, `/dev` and `/api/home` respond; NTP synchronised; NVS store initialised with defaults.
- `test/litter_test.cpp` passes: weight matching, ties and tolerance, history ring, reassignment, corrupted flash data, visit signals in different orders and minutes apart, visits missed while off, counter reset, `millis()` wrap.
- Home page checked in a browser against fake data (phone layout, light and dark, first run, full bin, settings save). No console errors.
- With the MCU reconnected: full handshake, 27 valid frames, zero errors, same 19 datapoints; DP7 = 10 taken as baseline without creating visits. DP6 and DP8 never seen (they appear only after a real visit).

### 1.3.0 / 1.3.1 — weight chart, adaptive reference weight, network state 4

- Build: flash 858,562 B. Chart, legend, tooltip and table checked against fake data.
- 1.3.1 reports network state `4` to the MCU (was `3`, which kept the Wi-Fi LED blinking) and answers the MCU's time request with local time. **Effect on the LED not yet observed**: the MCU was disconnected during the following sessions.

### 1.4.0 / 1.5.0 — OTA update, recovery network

- Two OTA updates performed with `tools/ota.py` (1.4.0 → 1.4.0, 1.4.0 → 1.5.0); the module rebooted on the new version each time.
- Recovery network and Wi-Fi credential change **not tested** on the device (requires taking the Wi-Fi down). Upload from the `/dev` page tested only through the same endpoint via the script.

### 1.6.0 / 1.7.0 / 1.7.1 — MCU settings, maintenance buttons, weight units

- Settings (DP105, DP117 now 0–60, DP129), *bag change* (DP127) and *litter added* (DP126) flows tested in a browser against the preview server: confirmation dialogs, sequential writes, reported-value check.
- 1.7.1 accepts DP6 both in grams (600–30000) and in 0.1 kg steps (5–300), following the report in tuya-local issue #1541.
- Build: flash 884,806 B. Updated over the air.
- **None of these commands has reached the real MCU**, which was disconnected while they were developed.

### 1.8.0 — visits instead of grams, Italian/English, cat popup, home-screen icon

- The bin card counts visits since the last bag change (limit in visits); the gram estimate is gone. Settings saved by 1.x are migrated: limit in grams ÷ grams per visit (1500 ÷ 50 → 30 on this unit, confirmed over HTTP after the update). Host test covers the migration.
- Both pages in Italian and English; server messages follow `X-Tonepie-Lang`. Tapping a cat opens its visits day by day. PNG icons (180/192/512) and a web app manifest are served for "Add to Home Screen".
- Checked in a browser against the preview server (phone width, both languages, popup, settings save, icons, no console errors). Build: flash 917,650 B. Updated over the air; cats and visit history survived.
- Adding the page to a real phone's home screen not tried yet.

### 1.9.0 — history server

- Server (`server/`, .NET 10 + EF Core 10): 4 integration tests pass (key check, validation, per-day aggregation in Europe/Rome, idempotent resend, cat reassignment, deletions by gap and explicit list, renumbered cats, CORS). Migrations generated for SQLite, PostgreSQL and SQL Server; only SQLite exercised.
- Docker image built and run with Docker Desktop on the development PC; ingest, wrong key (401) and history checked with curl.
- ESP updated over the air, sync enabled towards the container: first snapshot accepted within seconds (4 real visits of Ellie and Flipper, with weights). MCU still connected, no frame errors, 126 kB free heap.
- History card and cat pop-up checked in a browser against a year of fake visits (`tools/demo_history.py`).
- 1.9.1: browsers blocked the page's direct request to the server (`ERR_BLOCKED_BY_CLIENT`, local-network protection); the ESP now relays `/api/history` (0.15 s for the real data). First upload waits 10 s after Wi-Fi connects (the first attempt at boot was refused). Checked on the real page.
- Not tried: HTTPS server, PostgreSQL/SQL Server, a long server outage (retry and backlog), the server on a NAS or Raspberry Pi.

### 1.10.0 — fixes from a full code review

- Commands from the home page: after a network error a command is never sent again (only a "wait" answer, 429, is retried), and one sequence runs at a time with the buttons disabled. Checked in the preview by making the command request fail: one attempt, "result unknown" message.
- Visit detection: a counter that goes down (midnight) counts today's visits instead of none; a counter saved on another day restarts from 0; the counter is saved only once its visits are recorded; after a restart the last visit is restored so its late signals patch it; patches target the tracker's own visit by id and a repeated weight is not learned twice; values from query responses are not given to later visits; old timestamps expire before `millis()` wraps. 9 new host tests, all passing.
- Firmware: key kept only for the same server address; history relay with the key, short timeouts and an 8 s deadline; deletions and a send "generation" kept consistent across restarts and setting changes; network state to the MCU retried 3 times then once a minute; DP126 no longer reported as "result unknown"; weight chart on local days; smaller JSON buffers. Updated over the air: MCU handshake clean (0 frame errors), one network-state frame at boot, sync working.
- Server: 20 tests (concurrency, nulls, id wrap and restart, DST with a fake clock, read authentication, body limit, pending model changes for all three providers). Named Docker volume (the old `./data` file was copied into it: no data lost), health check, no CORS, key required for reads.

## 7 October 2026

### 1.11.0 — upload panel, history without the server

- Each visit records whether this version of it reached the server (changed visits are sent again); last successful upload kept across restarts; `POST /api/sync/now`.
- Tested on the real unit with the server container stopped: History card from memory (64 visits), Send now reported "connessione rifiutata"; container started again: Send now uploaded, 64 sent / 0 to send, history merged.

## Not tested yet

- Any physical command from this firmware: clean (101), empty (102), level litter (126), bag change (127), and the writes to 105/117/118/129.
- A real cat visit: DP6/DP7/DP8 timing, weight unit, and therefore recognition and the waste estimate in practice.
- Wi-Fi LED behaviour with network state 4.
- Recovery network, Wi-Fi credential change, upload from the `/dev` page in a browser.
- Electrical levels with instruments, long-term stability, persistence of settings inside the MCU, behaviour of the MCU's own protections.
