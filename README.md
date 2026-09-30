# Tonepie Ti Pro 25 — ESP-C3 locale

Firmware Arduino / PlatformIO **1.7.0-maintenance** per sostituire il modulo WBR3, mantenendo la MCU Tonepie. UART **115200, 8N1, RX GPIO6, TX GPIO7**. Interfaccia web in italiano, senza CDN o servizi cloud, raggiungibile su `http://tonepie.local`, `http://tonepie.fritz.box` o sull'IP assegnato dal FRITZ!Box.

## Pagine

- **`/` — home di uso quotidiano.** Gatti riconosciuti dal peso, visite di oggi e degli ultimi 7 giorni, stima del peso di escrementi nel cassetto con avviso di svuotamento, elenco visite correggibile, pulsante "Pulisci ora".
- **`/dev` — pagina sviluppatore MCU** (datapoint grezzi, log, comandi con abilitazione). Si apre dal link discreto "Pagina sviluppatore" in fondo alla home, oppure digitando l'indirizzo.

### Come funziona la home

- **Riconoscimento gatti.** Nelle impostazioni (ingranaggio) si inseriscono nome e peso di ogni gatto, fino a 4. A ogni visita il peso riportato dalla MCU viene confrontato con quelli di riferimento: vince il più vicino entro la tolleranza (predefinita 0,5 kg); a pari distanza o fuori tolleranza la visita resta "non riconosciuta" e si assegna a mano toccandola. La MCU riporta il peso a passi di 0,1 kg: due gatti con meno di ~0,3 kg di differenza non sono distinguibili in modo affidabile.
- **Peso che cambia nel tempo.** A ogni visita riconosciuta il peso di riferimento del gatto si sposta di un quinto verso il peso misurato, così il riconoscimento segue il gatto che cresce o dimagrisce. Anche una correzione manuale insegna il nuovo peso. Il valore scritto a mano nelle impostazioni resta comunque modificabile.
- **Andamento del peso.** Per ogni gatto viene salvata la media giornaliera del peso (ultimi 90 giorni con dati); la home la mostra in un grafico a 30 o 90 giorni con valori consultabili anche in tabella.
- **Rilevamento visite.** Secondo la mappatura comunitaria: DP7 visite del giorno (si azzera ogni giorno), DP6 peso del gatto, DP8 durata (s). Il peso arriva in grammi su alcune Tonepie (segnalazione tuya-local #1541, 600–10000 g) e in kg ×10 su altre: il firmware accetta entrambi i formati e li distingue dall'ordine di grandezza. Una visita nasce dall'aumento di DP7 oppure da un report spontaneo di peso/durata; i tre segnali vengono fusi anche se arrivano a minuti di distanza. I valori ripetuti nelle risposte alle query non creano visite. Le visite avvenute a ESP spento vengono recuperate dal contatore all'avvio (senza peso, tranne l'ultima). **Non ancora verificato con un gatto reale**: logica in `include/litter_logic.h`, test in `test/litter_test.cpp`.
- **Cassetto.** La MCU non pesa gli escrementi: il valore è una **stima**, visite × grammi medi per visita (predefinito 50 g, modificabile). Oltre la soglia (predefinita 1500 g) la home chiede di svuotare; "Cambio sacchetto" azzera la stima. Il conteggio "cassetto pieno" interno della MCU (DP123/124, a numero di pulizie) non viene toccato e resta attivo in parallelo.
- **Dati.** Gatti, ultime 64 visite, storico dei pesi e stima del cassetto sono salvati nella flash dell'ESP (NVS) e sopravvivono a riavvii e aggiornamenti firmware.
- **Orologio.** Per l'orario delle visite l'ESP usa NTP (`fritz.box`, poi `pool.ntp.org`), fuso orario italiano. La stessa ora viene data alla MCU quando la chiede.
- **Impostazioni della lettiera.** Dall'ingranaggio si cambiano anche tre impostazioni tenute dalla MCU: pulizia automatica (DP105), minuti di pausa prima della pulizia (DP117, 0–60) e deodorante automatico dopo la pulizia (DP129). Vengono inviate solo le voci modificate, e solo con la lettiera collegata; la home controlla poi che la MCU riporti il nuovo valore.
- **Cambio sacchetto** azzera la stima del cassetto e invia alla MCU il comando "bag replace" (DP127). **Aggiunta lettiera** registra la data e invia "level litter" (DP126), che livella la sabbia. Con la lettiera non pronta registrano soltanto. Cosa facciano fisicamente i due comandi su questo esemplare non è ancora stato osservato; DP126 non viene mai riportato dalla MCU, quindi è l'unico comando inviato senza un report recente di riscontro. Entrambi hanno gli stessi veti di presenza, fault e blocco della pulizia.
- **Pulisci ora** usa lo stesso percorso protetto della pagina sviluppatore (dati recenti, veti presenza/fault/blocco) dopo una conferma.

Anteprima delle pagine senza ESP, con dati finti: `python tools/preview.py` e apri `http://localhost:8765`.

## USB e pin: versione aggiornata

**Non occorre isolare, dissaldare o modificare il CH340.** La UART Tonepie è stata spostata su GPIO6/7. GPIO20/21 restano dedicati alla UART0 del CH340: flash e log a 115200 passano dalla normale micro-USB del kit.

Questo progetto usa `HardwareSerial(1)` per Tonepie e `Serial` per CH340. Non è necessario aggiungere un connettore USB nativo su GPIO18/19. Mantieni GPIO6/7 liberi da altri circuiti o debugger JTAG esterni.

**Migrazione dalla prima versione:** a entrambe le schede spente, sposta il filo prima su GPIO20 a GPIO6 e quello prima su GPIO21 a GPIO7. GND resta comune. Non collegare più la Tonepie ai pin RX/TX stampati sul kit (GPIO20/21). Il nuovo firmware richiede questo nuovo cablaggio; non è compatibile con quello precedente.

## Cablaggio con WBR3 rimosso

| Segnale sulla scheda Tonepie | ESP-C3-13-Kit |
|---|---|
| TX della **MCU**, pista che entrava in RXD del WBR3 | GPIO6 / RX |
| RX della **MCU**, pista che usciva da TXD del WBR3 | GPIO7 / TX |
| GND | GND |

Le etichette RX/TX del WBR3 sono dal punto di vista del modulo rimosso: non confonderle con quelle della MCU. Verifica le piste; non basarti su una posizione dei pad dedotta da una foto. La versione precedente della conversazione invertiva questi riferimenti in un passaggio.

Logica UART a **3,3 V**, non RS232 e non 5 V. Per le prime prove alimenta il kit separatamente via USB/5V appropriati e lascia Tonepie sulla propria alimentazione, con GND comune; non collegare la 3V3 della Tonepie finché capacità e tensione non sono verificate. Effettua saldature e verifiche di continuità a dispositivi spenti.

## Compilazione e flash

1. Installa VS Code con PlatformIO IDE, oppure PlatformIO Core. Apri questa cartella, quella con `platformio.ini`.
2. Il profilo `esp32-c3-devkitm-1` fornisce il target generico **ESP32-C3 / flash 4 MB**; i pin sono espliciti e non dipendono dal LED o dal pinout DevKitM. Verifica che il tuo modulo abbia flash da 4 MB.
3. Copia `include/secrets.example.h` in `include/secrets.h` e inserisci nome e password del Wi-Fi e una password di aggiornamento. `secrets.h` è escluso da git: le password finiscono solo nel firmware compilato, non nel repository. Non vengono inviate dalla dashboard né stampate nei log.
4. Compila:

   ```sh
   pio run -e tonepie-c3
   ```

5. Collega la normale micro-USB del kit (CH340). Durante la migrazione lascia scollegati i vecchi fili su GPIO20/21. Individua la porta con `pio device list` e carica:

   ```sh
   pio run -e tonepie-c3 -t upload --upload-port COM6
   ```

   Sostituisci `COM6` se la porta cambia. Se non entra nel bootloader, tieni premuto BOOT, premi/rilascia RESET e poi rilascia BOOT. La UART Tonepie su GPIO6/7 è separata dalla seriale di programmazione.

6. Per i log scegli la stessa porta **CH340**, chiudendo eventuali altri monitor prima del flash:

   ```sh
   pio device monitor --port COM6 --baud 115200
   ```

7. Avvia, verifica l'IP nel log USB oppure nella lista dispositivi del FRITZ!Box. Il C3 usa Wi-Fi 2,4 GHz: la rete deve essere disponibile su quella banda. Apri `http://tonepie.local`; se mDNS non è supportato dal client o attraversa VLAN, usa l'IP. Senza Wi-Fi per 3 minuti viene aperta la rete di recupero `Tonepie-Setup` (vedi sotto).

La pagina è incorporata nel firmware: nessun caricamento filesystem/LittleFS separato. La compilazione fissa piattaforma e dipendenza JSON per rendere riproducibile il progetto.

## Aggiornamento via rete

Dopo il primo caricamento via USB non serve più il cavo:

```sh
pio run -e tonepie-c3
python tools/ota.py            # oppure: python tools/ota.py http://<indirizzo> <file.bin>
```

In alternativa, dalla pagina `/dev`: sezione "Aggiornamento firmware via rete", scegli `.pio/build/tonepie-c3/firmware.bin` e inserisci la password. La password è `OTA_PASSWORD` in `include/secrets.h`. Il firmware nuovo viene scritto nella seconda partizione e verificato prima del riavvio: se il caricamento si interrompe resta attivo quello vecchio. Gatti, visite e pesi non vengono toccati. Durante l'aggiornamento gli invii alla MCU sono disabilitati.

## Se il Wi-Fi di casa non c'è più

Se il modulo non riesce a collegarsi al Wi-Fi per 3 minuti (router cambiato, password cambiata, rete spenta) apre una propria rete di recupero:

- rete **`Tonepie-Setup`**, password uguale a quella del Wi-Fi scritto in `include/secrets.h`;
- dal telefono o dal PC collegati a quella rete e apri **`http://192.168.4.1`** (home) o **`http://192.168.4.1/dev`**;
- da `/dev` puoi caricare un firmware nuovo oppure, nella sezione "Rete Wi-Fi", inserire nome e password della nuova rete di casa (serve la password di aggiornamento). Se le credenziali sono sbagliate, dopo 3 minuti la rete di recupero riappare.

Appena il Wi-Fi di casa torna e nessuno è collegato alla rete di recupero, questa si spegne da sola. Le credenziali salvate da `/dev` hanno la precedenza su quelle di `secrets.h` e sopravvivono agli aggiornamenti. Il Bluetooth non è usato: l'ESP32-C3 ha solo BLE, che per aggiornare il firmware richiederebbe un'app dedicata e sarebbe molto più lento.

## Prima messa in servizio

1. Verifica i nuovi pin GPIO6/7, le masse e i livelli. Collega la MCU con WBR3 già rimosso e lascia il CH340 integro.
2. Attendi heartbeat e inizializzazione. Senza risposta la dashboard resta offline e rifiuta comandi. Non occorre lasciare un computer connesso.
3. Esegui **Aggiorna dalla MCU** e osserva DP e tipi. L'assenza di un DP significa sconosciuto, non `false` o zero.
4. Confronta i report con cambiamenti eseguiti dai comandi fisici originali. Verifica soprattutto BOOL 101/102/105 e VALUE 117/118.
5. Abilita gli invii dalla dashboard per una sessione di dieci minuti. La prima prova deve essere sorvegliata e senza animale nella lettiera. Pulizia e svuotamento hanno una conferma nel browser.
6. Un messaggio “trasmesso” significa soltanto invio UART. “DP riportato” significa che la MCU ha riportato quel valore, **non** conferma di movimento completato; dopo 5 secondi senza report l'esito resta sconosciuto. Non viene ritentato automaticamente un comando.

## DP: cosa è confermato e cosa no

**Verificati sul tuo esemplare comunicazione UART e tipi ricevuti:** DP101/102/105 BOOL, DP117/118 VALUE. Non sono stati provati comandi di movimento o modifiche delle impostazioni: il loro significato resta basato sulle fonti comunitarie Ti Pro25/TPCBP-T2501. Cattura del primo collegamento in `diagnostics/mcu-first-contact.json`; dettagli in `VALIDATION.md`.

| DP | Mappatura comunitaria usata | Trattamento |
|---|---|---|
| 101 | pulizia, bool | invio `true` |
| 102 | svuotamento, bool | invio `true` |
| 105 | auto-clean, bool | ON/OFF |
| 117 | attesa, value, minuti | 0–60 |
| 118 | intervallo, value, minuti | 0–120 |
| 126 | livella lettiera, bool (mai riportato dalla MCU) | invio `true` |
| 127 | cambio sacchetto, bool | invio `true` |
| 129 | deodorante dopo la pulizia, bool | ON/OFF |
| 22 | fault | grezzo; bit non decodificati |
| 24 | stato | grezzo; ordine ENUM da verificare |
| 104 / 114 | presenza / blocco bambini | sola lettura, veto aggiuntivo quando attivi |
| altri | significato non assunto | sola lettura |

La configurazione `tuya-local` documenta le cinque scritture utilizzate. I suoi intervalli 117/118 includono zero, e la tua MCU li riporta entrambi a zero: il firmware accetta quindi anche lo zero. DP129 è indicato dalla stessa fonte come "odor removal after cleaning".

Una segnalazione Ti Pro25 documenta la presenza all'avvio di **7, 22, 24, 101, 102, 105, 114, 116, 117, 118, 123–128, 131**. Riferisce che **6, 8, 104, 113, 129, 134** possono comparire solo dopo l'uso. Queste sono osservazioni su altre unità, non garanzie per la tua scheda.

Da verificare: tipo UART effettivo, eventuali scale/unità, ordine degli ENUM e significato dei bit fault. In particolare i nomi stringa cloud di DP24/116/131 non determinano automaticamente i byte ENUM UART. Il codice non li traduce arbitrariamente. DP sconosciuti vengono mostrati senza scritture generiche.

## Sicurezze e limiti

- Nessun GPIO di motore/sensore viene pilotato; nessuna simulazione di presenza, calibrazione, reset MCU o aggiornamento MCU. DP113 è escluso dalle scritture. Tutto il controllo fisico rimane alla MCU originale.
- I controlli web aggiuntivi non certificano l'assenza dell'animale: dati presenza mancanti o non recenti non possono provarla. Non si sostituiscono alle protezioni originali, che devono essere verificate nella prova reale.
- Sono scrivibili **solo 101/102/105/117/118/126/127/129**. Il server richiede heartbeat recente, init completata, DP dello stesso tipo ricevuto negli ultimi 60 s, sessione abilitata e nessun comando pendente. Nessuna scrittura DP viene fatta all'avvio o alla riconnessione.
- La sessione si disabilita dopo timeout, perdita Wi-Fi o riavvio/perdita MCU. Non è salvata in flash. Le impostazioni numeriche vengono inviate alla MCU; la loro persistenza dipende dal firmware Tonepie.
- Interfaccia HTTP per rete domestica fidata, senza account/TLS. Token per avvio, controllo Host e header dedicato contrastano richieste browser da altri siti; **non sono autenticazione contro un client della LAN**. Non esporre la porta 80 su Internet.
- Con il Wi-Fi attivo la rete è dichiarata alla MCU come pienamente connessa (`4`): con lo stato `3` (solo router) la lettiera lascia lampeggiare la spia Wi-Fi. Non esiste comunque alcun collegamento al cloud Tuya. Il valore è `NETWORK_CONNECTED` in `include/config.h`.

## Protocollo e diagnostica

Frame `55 AA`, versione TX `00`, versione RX prevista `03`, lunghezza big-endian, checksum somma modulo 256. Payload massimo 1024 byte, buffer RX UART 4096 byte, timeout inter-byte 250 ms. Il parser recupera da rumore, checksum errati, lunghezze e frame incompleti; valida l'intero report TLV prima di applicarlo. BOOL, VALUE 32 bit big-endian, RAW, STRING, ENUM, BITMAP sono riconosciuti. VALUE visualizzato con segno; invii limitati agli intervalli positivi dei form.

Heartbeat ogni secondo finché offline, ogni 5 s online; perdita dopo 15 s. Init sequenziale heartbeat → prodotto (`01`) → modo (`02`) → rete (`03`) e query (`08`), con ritenti delle richieste init. Reset MCU rilevato tramite heartbeat `00`; cache invalidata. Query manuale e query 500 ms dopo una scrittura. DP `07` ricevuti anche spontaneamente. Richieste pairing `04/05` riconosciute senza cancellare le credenziali fisse. Richiesta ora `1C`: risposta con l'ora locale presa da NTP, oppure “non disponibile” finché l'orologio non è sincronizzato.

Cache fino a 64 DP, 128 byte conservati per DP, visualizzazione hex limitata; 60 righe di log in RAM, perse al riavvio. Log USB scartato se il buffer non è pronto; i log web continuano. Protocollo sconosciuto/versione inattesa segnalato. Nessun supporto a protocolli Tuya alternativi, aggiornamento della MCU o provisioning Smart Life.

Se non arriva nulla: controlla TX/RX dal punto di vista della MCU, GPIO6/7, GND, alimentazione e velocità. Se arrivano checksum errati: controlla livelli e cablaggi. Se il DP è rifiutato: esegui query; non modificare a caso il tipo. Se la MCU non risponde a query in stato rete `3`, documenta i frame prima di estendere il protocollo.

## Struttura e test

```text
platformio.ini             target e dipendenze fissate
include/config.h           pin, tempi e nomi di rete
include/secrets.example.h  modello per secrets.h (password, fuori dal repository)
include/tuya_protocol.h    parser/encoder indipendente da Arduino
include/web_ui.h           pagina sviluppatore (/dev)
include/web_home.h         home di uso quotidiano (/)
include/litter_logic.h     gatti, visite, stima cassetto: indipendente da Arduino
src/main.cpp               UART, init, API HTTP, cache, logging, salvataggio NVS
test/protocol_test.cpp     test host del protocollo
test/litter_test.cpp       test host di riconoscimento gatti e rilevamento visite
tools/preview.py           anteprima locale delle pagine con dati finti
tools/ota.py               aggiornamento del firmware via rete
VALIDATION.md              risultati della verifica effettuata
```

Test host con un compilatore C++11, dalla cartella del progetto:

```sh
g++ -std=c++11 -Wall -Wextra -pedantic -Iinclude test/protocol_test.cpp -o protocol_test
./protocol_test
```

Su Windows esegui `./protocol_test.exe`. I test coprono frame noti, rumore, checksum, payload massimo, recupero da troncamento, TLV malformati, wrap di millis e un milione di byte pseudocasuali. Non sostituiscono il collaudo hardware.

## Fonti consultate

- [Protocollo Wi-Fi MCU Tuya](https://developer.tuya.com/en/docs/iot/mcu-protocol?id=K9hrdpyujeotg): framing e comandi base.
- [Configurazione Ti Pro25 nel repository tuya-local](https://github.com/make-all/tuya-local/blob/main/custom_components/tuya_local/devices/ti_pro25_catlitterbox.yaml): mappature comunitarie, non specifica UART del produttore Tonepie.
- [Segnalazione Ti Pro25 #6071](https://github.com/make-all/tuya-local/issues/6071): datapoint effettivamente segnalati e varianti di disponibilità.
- [Specifiche Ai-Thinker ESP-C3-13-Kit, copia del documento del produttore](https://iot-kmutnb.github.io/blogs/esp32/esp32_c3_ai_thinker/esp-c3-13-kit-v1.0_spec.pdf).
- [Espressif: API UART Arduino](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/serial.html): scelta dei pin RX/TX della UART.

La comunicazione UART è stata verificata sulla tua Tonepie a 115200 8N1 su GPIO6/7, con handshake e query riusciti. Non è una misura elettrica con oscilloscopio. Stato delle verifiche: 30 settembre 2026.
