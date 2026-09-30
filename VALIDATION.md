# Verifica — 30 settembre 2026

## Eseguito

- Compilazione reale PlatformIO per ESP32-C3 riuscita, framework Arduino 2.0.17, piattaforma Espressif32 6.10.0, ArduinoJson 6.21.5.
- Versione aggiornata `1.1.0-uart6-7`: UART1 RX GPIO6 / TX GPIO7, logging UART0 tramite CH340.
- RAM statica: 52.068 byte / 327.680 (15,9%). Il dato non comprende tutte le allocazioni runtime di Wi-Fi e dashboard.
- Flash applicazione: 797.208 byte / 1.310.720 (60,8%).
- Caricamento reale su ESP32-C3 revisione 0.3 via CH340 COM6 riuscito, hash flash verificati dal programmatore.
- API HTTP raggiunta su `192.168.178.94`: versione `1.1.0-uart6-7`, RX6 e TX7 confermati dal firmware in esecuzione. Wi-Fi e server web operativi.
- Log seriale CH340 a 115200 letto correttamente: heartbeat TX presente. MCU offline, zero frame RX, invii disabilitati al momento della verifica.
- `test/protocol_test.cpp` compilato ed eseguito su host: PASS. Frame heartbeat/query noti, checksum errato, prefissi rumorosi, frame concatenati/frammentati, payload massimo, lunghezza eccessiva, recupero timeout con più header falsi, TLV e tipi malformati, contatore temporale che riparte da zero, 1.000.000 di byte pseudocasuali.
- JavaScript della pagina estratto e controllato con `node --check`: PASS.
- Revisione delle scritture: API limitate a DP101/102/105/117/118; nessuna scrittura generica, reset MCU o comando di calibrazione. I comandi fisici non vengono ripetuti automaticamente dopo timeout.

## Non eseguito

Restano da verificare: livelli elettrici con strumentazione, significato degli enum UART, semantica delle scritture DP sulla tua revisione, operazioni fisiche, comportamento delle protezioni originali, persistenza delle impostazioni nella MCU, stabilità prolungata e rendering in un browser reale. Non è necessario isolare il CH340 con questo cablaggio.

## Primo collegamento alla MCU sui nuovi pin

Dopo il collegamento dell'utente, verificati heartbeat, inizializzazione prodotto/modo/rete e query a 115200 8N1 su GPIO6/7. MCU online e pronta. Dopo una query diagnostica: 51 frame validi, 19 datapoint in cache, zero errori checksum/lunghezza/TLV e zero timeout del parser. Invii operativi disabilitati; nessun comando movimento o modifica impostazioni inviato.

Tipi UART osservati: DP101/102/105 BOOL, DP117/118 VALUE. DP105=true, DP117=0, DP118=0, DP22 BITMAP=0, DP24 ENUM=6 nell'ultima lettura. È comparso anche un report DP24 BOOL prima del successivo ENUM: non assumere uno schema uniforme per quel DP. Queste osservazioni confermano traffico e tipi, non il significato fisico o la sicurezza dei comandi.

Snapshot e log in `diagnostics/mcu-first-contact.json`, senza token HTTP. L'unità si identifica nel report prodotto come `yn6wqmizg7abe5k8`, versione MCU `1.0.15`.

I test software e la compilazione non rendono il progetto un firmware hardware già collaudato. Prima dell'uso autonomo serve la sequenza di messa in servizio del README.

## Versione 1.2.0-home — 30 settembre 2026

Eseguito:

- Compilazione ESP32-C3 riuscita: RAM 53.468 / 327.680 byte, flash 850.018 / 1.310.720 byte. Caricamento su COM6 con hash verificati.
- Dopo il flash: `/` (home) e `/dev` (pagina sviluppatore) rispondono, `/api/home` valido, orologio NTP sincronizzato, archivio NVS vuoto inizializzato con i valori predefiniti.
- `test/litter_test.cpp` e `test/protocol_test.cpp` su host: PASS. Coperti: abbinamento per peso, parità e tolleranza, storico ad anello, riassegnazioni, dati corrotti in flash, segnali di visita in ordini diversi e a minuti di distanza, visite perse a ESP spento, azzeramento del contatore, wrap di millis.
- Home provata in browser con dati finti (`tools/preview.py`): vista telefono chiara e scura, primo avvio senza gatti, cassetto pieno, salvataggio impostazioni, senza errori in console.

Non eseguito:

- **Nessuna verifica con la MCU collegata**: al momento del flash la MCU era scollegata (zero frame ricevuti). Da ricontrollare handshake e datapoint con il nuovo firmware.
- Nessuna visita reale osservata: DP6/7/8 (peso, contatore, durata) non sono mai stati ricevuti da questo esemplare; unità, ordine e tempi dei report sono presi dalla mappatura comunitaria.
- "Pulisci ora" dalla home non è stato premuto sul dispositivo reale.

## Versione 1.3.0-home — 30 settembre 2026

Aggiunti grafico dell'andamento del peso, peso di riferimento che segue il gatto e link alla pagina sviluppatore in fondo alla home.

- Compilazione riuscita: RAM 57.804 / 327.680 byte, flash 858.562 / 1.310.720 byte. Caricamento su COM6 con hash verificati; `/`, `/dev` e `/api/home` rispondono con la versione 1.3.0-home.
- `test/litter_test.cpp`: PASS, esteso a media giornaliera, campioni arrivati in ritardo, storico pieno, apprendimento del peso, rimozione di un gatto con spostamento di visite e pesi.
- Grafico provato in browser con dati finti (vista telefono): linee, etichette finali, legenda con variazione, tooltip e tabella dei valori, senza errori in console.
- Restano validi tutti i "non eseguito" della 1.2.0: MCU ancora scollegata al momento del flash, nessuna visita reale osservata.

### Verifica con MCU ricollegata (1.3.0-home)

Dopo il ricollegamento: handshake completo (heartbeat, prodotto, modo, rete, query), MCU online e pronta, 27 frame validi, zero errori di checksum, lunghezza, TLV e timeout, 19 datapoint con gli stessi valori del primo collegamento. Il contatore visite DP7=10 è stato preso come base senza creare visite fittizie. DP6 (peso) e DP8 (durata) non sono presenti: compaiono solo dopo un uso reale. Nessun comando inviato.

## Versioni 1.3.1 → 1.5.0-recovery — 30 settembre 2026

- **1.3.1**: stato di rete dichiarato alla MCU `4` invece di `3` (spia Wi-Fi), risposta con l'ora locale alla richiesta `1C`.
- **1.4.0**: aggiornamento firmware via rete (`tools/ota.py` e pagina `/dev`), protetto da password.
- **1.5.0**: rete di recupero `Tonepie-Setup` dopo 3 minuti senza Wi-Fi, cambio delle credenziali Wi-Fi da `/dev`.

Eseguito: compilazione (RAM 58.268 / 327.680, flash 878.438 / 1.310.720 byte); due aggiornamenti via rete riusciti con `tools/ota.py` (1.4.0 → 1.4.0 e 1.4.0 → 1.5.0), modulo riavviato sulla versione nuova; `/dev` mostra le sezioni rete e aggiornamento; sintassi JavaScript di entrambe le pagine verificata; test host PASS.

Non eseguito: **effetto dello stato `4` sulla spia e risposta all'ora non verificati**, perché dopo questi caricamenti la MCU non rispondeva (zero frame: scheda scollegata o non alimentata). Rete di recupero e cambio credenziali Wi-Fi mai provati sul dispositivo reale (richiede togliere il Wi-Fi). Aggiornamento dal modulo della pagina `/dev` non provato in browser: provato solo lo stesso endpoint tramite script.

## Versioni 1.6.0 e 1.7.0-maintenance — 30 settembre 2026

- **1.6.0**: nelle impostazioni della home, pulizia automatica (DP105), pausa prima della pulizia (DP117, ora 0–60) e deodorante automatico (DP129).
- **1.7.0**: pulsanti Cambio sacchetto (azzera la stima e invia DP127) e Aggiunta lettiera (registra la data e invia DP126).

Eseguito: compilazione (flash 884.780 / 1.310.720 byte), aggiornamento via rete riuscito, sintassi JavaScript verificata, flussi provati in browser contro il server di anteprima con dati finti (salvataggio impostazioni con conferma, cambio sacchetto, aggiunta lettiera, annullamento della pulizia).

Non eseguito: **nessuno di questi comandi è stato inviato alla MCU reale**, che durante il lavoro non rispondeva. Restano da osservare sulla lettiera l'effetto fisico di DP126 e DP127, l'accettazione di DP129 e di DP117, e il comportamento della spia Wi-Fi con lo stato di rete 4.
