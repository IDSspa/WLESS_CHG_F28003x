# Istruzioni operative per il repository WLESS_CHG_F28003x

Questo file contiene regole vincolanti per ogni nuova sessione di lavoro sul
firmware e sull'asset hardware. Non affidarsi alla sola memoria della
conversazione.

## 0. Resource usage policy

Minimize Codex resource and allowance consumption.

- Do NOT create or delegate work to subagents unless I explicitly request it.
- Do NOT run multiple agents or tasks in parallel unless I explicitly request it.
- Prefer completing the task in the current agent/session.
- Do NOT perform repository-wide searches or analysis unless required by the task.
- For small or localized requests, inspect only the files directly relevant to the request.
- Do NOT repeat searches, builds, tests, or verification steps unless necessary.
- Do NOT run broad test suites for small/localized changes unless explicitly requested.
- Do NOT re-derive or ask for values that can reasonably be inferred from information
  already provided in the conversation or project context.
- Prefer reasonable inference over asking for confirmation when the consequence is
  easily reversible.
- Before starting expensive repository-wide analysis, extensive testing, or
  parallel work, ask for permission.
- Per qualunque correzione proposta su controllo, protezioni, bootstrap o
  percorsi di potenza: non implementare ne' proporre una correzione finche'
  non sono stati ricostruiti il call path completo e le funzioni coinvolte,
  dichiarate le assunzioni verificabili, indicato almeno un test che possa
  smentire l'ipotesi e spiegata la variabile fisica realmente limitata.

## 1. Letture obbligatorie prima di operare

Prima di analisi, modifiche firmware o prove hardware:

1. leggere `docs/WLESS_CHG_DEVELOPMENT_BASELINE.md`;
2. leggere le sezioni pertinenti di `docs/WLESS_CHG_CODE_KNOWLEDGE_BASE.md`;
3. per prove UART leggere la sezione pertinente di
   `docs/WLESS_CHG_MANUALE_TEST_UART.md`;
4. per risultati precedenti consultare `docs/WLESS_CHG_TEST_REPORT.md` e gli
   eventuali report specifici;
5. ricavare e dichiarare una checklist del test prima di comandare l'hardware.

Se documentazione e firmware non concordano, verificare il codice corrente e
segnalare esplicitamente la discrepanza. Non usare automaticamente un valore
storico come configurazione attuale.

### Registrazione su richiesta delle prove

Creare nel folder `test/` del worktree corrente un file
`test-YYYYMMDD-HHMMSS.log` solo quando l'utente richiede esplicitamente
"fai un dump del test". Il file deve contenere soltanto dati fattuali:
data/ora, release e ruoli, cablaggio/setup, alimentazioni e carichi, comandi o
setpoint applicati, durata, letture salienti, stato radio e fault/latch, esito
osservato. Non inserire commenti, ipotesi, analisi o raccomandazioni nel log:
queste restano nella conversazione o nella documentazione tecnica dedicata.

## 2. Regole inderogabili di interazione con il banco

- Non eseguire clean build, build, flash o programmazione delle controlCARD,
  salvo richiesta esplicita dell'utente. Normalmente build e flash sono eseguiti
  dall'utente.
- Se viene richiesto un build di ruolo, eseguire sempre un clean completo prima
  del build: la macro di ruolo puo' altrimenti rimanere incoerente.
- Durante il flash deve essere collegata una sola controlCARD/debug probe.
- Non accendere o spegnere alimentatori e carichi implicitamente: indicare
  chiaramente all'utente `DCLINK ON/OFF`, `VIN ON/OFF` o `LOAD ON/OFF` e attendere
  conferma.
- Eseguire un solo run osservabile alla volta. Non concatenare tentativi senza
  attendere le letture dell'operatore.
- Evitare le concatenazioni dei comandi UART. Inviare comandi singoli con una
  pausa sufficiente fra un comando e il successivo, per evitare perdita o
  interpretazione ambigua delle risposte da parte del parser e dell'operatore.
- Non usare `serial_comm.ps1` per configurare o comandare prove hardware: lo
  script e' usato dall'utente soltanto come verifica pratica di `FW?` e
  `ROLE?` dopo il build e non e' stato validato come strumento generale. Per le
  altre operazioni usare una sessione seriale diretta/persistente o il software
  UART abituale dell'utente.
- Rispettare letteralmente ogni durata richiesta. Per letture manuali usare una
  finestra sufficiente all'operatore, normalmente almeno 5-10 s.
- Prima di un nuovo punto di potenza verificare: stadio precedente OFF,
  condensatori scarichi, tensioni residue, carico, limiti PSU e temperatura.
- In caso di STOP dell'utente, interrompere immediatamente e non inviare altri
  comandi di prova.

## 3. Power cycle corretto

Il collegamento USB puo' mantenere alimentate le controlCARD. Quando e' richiesto
un vero power cycle:

1. disabilitare HFC/BOOST/BUCK e porre DCLINK/VIN OFF;
2. attendere o verificare la scarica dei bus;
3. scollegare i collegamenti USB;
4. spegnere le alimentazioni logiche;
5. rialimentare prima le logiche e verificarne l'assorbimento regolare;
6. ricollegare USB solo dopo l'avvio delle logiche.

Se USB viene collegata prima della logica, il firmware puo' rimanere bloccato;
un assorbimento logico anomalo (storicamente circa 0,17 A anziche' circa 0,2 A)
e' un possibile indizio, non una prova autonoma.

Nel dialogo operativo, `PC OK` significa che l'operatore ha completato un vero
power cycle secondo la procedura sopra.

## 4. Ruoli, schede e porte

- Non dedurre il ruolo dal nome della COM, dalla posizione fisica o dalla
  controlCARD: verificarlo sempre con `ROLE?`.
- Verificare la release con `FW?` su entrambe le schede.
- La mappatura usata spesso e' COM25/VEHICLE e COM26/STATION, ma non e' una
  garanzia e puo' cambiare dopo scambi di controlCARD.
- La VEHICLE decide/inizia lo scambio; la STATION e' slave e risponde quando
  interrogata.
- Verificare `RADIO?` a potenza disabilitata prima dell'inizializzazione.

## 5. Inizializzazione FSM distribuita

### Consistenza delle sequenze UART

Almeno una volta per ogni sessione che preveda un run di potenza, prima del
primo `VIN ON`/`DCLINK ON`, confrontare la sequenza UART pianificata con gli
script pertinenti in `scripts/` e con il profilo di prova dichiarato. Elencare
esplicitamente ogni differenza di comando, valore, ordine o fase; non
introdurre silenziosamente seed, soglie o limiti assenti dallo script di
riferimento. Se il firmware aggiunge un nuovo parametro necessario, aggiornare
lo script oppure dichiarare la deroga e farla approvare prima del run. Ripetere
il confronto quando cambiano firmware, script, setup elettrico o profilo.

`INITOK` e' un passaggio obbligato della FSM corrente, non un dettaglio opzionale.
Sequenza consolidata:

1. configurare ruolo SOURCE/MANUAL sulla VEHICLE;
2. attendere che la STATION raggiunga `INIT_LOAD` con ruolo LOAD;
3. inviare `INITOK` alla STATION e attendere `WAIT_SOURCE/INITOK`;
4. inviare `INITOK` alla VEHICLE;
5. verificare gli stati finali e `RadioLink=OK` su entrambe.

Non inviare il secondo `INITOK` prima che la controparte abbia completato la
transizione richiesta.

### Procedura completa per un nuovo run HFC/WPT

La sola permanenza delle UART aperte o l'invio di `WPT=1` non inizializza una
nuova sessione. Prima di ogni run verificare una sequenza di stati completa.

1. A DCLINK/VIN spenti verificare `FW?`, `ROLE?`, `RADIO?` e che entrambe le
   FSM siano `STANDBY`, con `Role=NOTHING`, `CtrlState=IDLE` e `RadioLink=OK`.
   `DISCHARGE_*`, `*_OFF` e `*_END` sono stati transitori della chiusura: non
   richiedono da soli un power cycle. Attendere e verificare il loro ritorno a
   `STANDBY`; non procedere direttamente con `WPT=1` finche' la sessione
   precedente non e' conclusa e il link non e' tornato OK.
2. Con HFC fermo e WPT disabilitato, configurare su entrambe le schede soglie
   FSM (`SMVBUS`), riferimenti, limiti, clamp e seed; poi verificare i valori
   con `SMTHR?` e `WPT?`. Configurare anche il mapping/limite/rampa HFC sul
   SOURCE. Ogni modifica dei riferimenti o dei limiti disabilita WPT e richiede
   la successiva riabilitazione esplicita.
3. Inviare alla VEHICLE `MANUAL`, attendere, poi `SOURCE`. Attendere gli stati
   `INIT_SOURCE` sulla VEHICLE e `INIT_LOAD` sulla STATION.
4. Inviare `INITOK` soltanto alla STATION; attendere `WAIT_SOURCE` sulla
   STATION (controllo remoto `INITOK`). Solo allora inviare `INITOK` alla
   VEHICLE. Attendere `PRECHARGE_SOURCE` e `PRECHARGE_LOAD`, con link radio OK
   su entrambe le schede.
5. Abilitare WPT prima sulla STATION e poi sulla VEHICLE (`WPT=1`); verificare
   `EN=1`, ruoli SOURCE/LOAD, `HENA=0`, `HFLT=0` e radio OK. Questa e' la
   baseline immediatamente precedente all'alimentazione del bus.
6. Configurare il seed SOURCE prima di WPT, a bus spenti. Dalla release 1073
   resta pendente finche' HFC e VDC SOURCE fisico non sono validi; non
   riarmarlo via UART dopo DCLINK/VIN ON. Il seed LOAD e' invece consumato al
   primo riscontro fisico di VDC LOAD; per prove ripetibili dichiarare e
   verificare esplicitamente se deve essere riarmato oppure no.
7. Verificare `SOURCE_ON` e `LOAD_ON`, `RadioLink=OK` e assenza di fault;
   soltanto allora inviare `WPTHFC=1`. Se l'avvio restituisce
   `WPTHFC INHIBIT,<mask>`, non considerare il run iniziato: rimuovere la causa
   e ricominciare dalla baseline FSM, senza forzare HFC.

### Chiusura e riutilizzo della FSM

1. Prima di qualunque comando di arresto (`UE=0`, `WPTHFC=0`, `WPT=0` o
   equivalente), acquisire e congelare il capture disponibile del run. Il
   capture BBC e il capture WPT/HFC sono indipendenti: armare prima del run
   `CAP=1` **e** `WPTCAP=1` su ogni scheda interessata; a fine run, con
   potenza ancora attiva, congelare `CAP=0` **e** `WPTCAP=0` prima dello
   stop. `CAP` da solo non registra fase HFC, corrente tank, potenza WPT o
   stati del relativo hand-off. Il capture deve quindi precedere anche il
   disarmo BBC/HFC: dopo lo stop i valori di duty, fase, limiti e latch possono
   gia' essere azzerati o modificati dalla chiusura. Solo dopo il capture inviare `STOP` alla
   VEHICLE: e' il solo comando UART che imposta `WLESS_SM_stopCommand` e
   avvia la chiusura distribuita della FSM. `WPT=0` disabilita soltanto
   l'integrazione UniPD e non sostituisce `STOP`. Disarmare poi, se necessario,
   BBC/HFC con `UE=0` e `WPTHFC=0`, quindi richiedere `DCLINK OFF`/`VIN OFF`.
2. Porre DCLINK/VIN OFF e lasciare completare la scarica. Gli stati attesi
   transitano attraverso `DISCHARGE_SOURCE`/`DISCHARGE_LOAD`, gli stati OFF e
   infine `STANDBY`. Attendere la transizione: `DISCHARGE_*` non e' un latch.
3. Prima del run seguente verificare `STANDBY`/`IDLE` e radio OK. Il power cycle
   e' un recovery di ultima istanza, da usare se la FSM non converge o il link
   non torna operativo; non va imposto dalla sola osservazione di uno stato
   `DISCHARGE_*` o `*_OFF`.

#### Evoluzione dettagliata della chiusura e guardie di corrente

La chiusura distribuita non e' una sola transizione locale. Dopo `STOP`, con
bus spenti, la sequenza nominale e' la seguente:

```text
VEHICLE: DISCHARGE_SOURCE --(IB < IBMIN)--> SOURCE_OFF
         SOURCE_OFF        --(IC < ICMIN)--> SOURCE_END
         SOURCE_END        --(remote CtrlState=WPTEND)--> STANDBY / IDLE

STATION: DISCHARGE_LOAD   --(IC < ICMIN)--> LOAD_OFF
         LOAD_OFF          --(IB < IBMIN)--> LOAD_END
         LOAD_END          --(remote CtrlState=IDLE)--> STANDBY / IDLE
```

Le comparazioni sono strettamente minori (`<`), non minori o uguali. I default
storici `IBMIN=1 mA` e `ICMIN=1 mA` sono quindi incompatibili con un offset
ADC positivo anche in assenza di energia. Un caso osservato il 2026-09-18, a
`DCLINK=0 V`, aveva `IC` circa 90 mA su VEHICLE e 300--360 mA su STATION:
VEHICLE restava in `SOURCE_OFF`, STATION in `DISCHARGE_LOAD`, pur con radio OK
e `IB=0` su entrambe. Non e' un latch di potenza ne' un guasto radio.

Per diagnosticare, a bus spenti usare `VARS?`: riporta `VB`, `IB`, `IC`,
`VBMIN`, `IBMIN`, `ICMIN`, stato, ruolo e controllo. Se il valore residuo
supera la soglia, impostare *temporaneamente* con `SMICOIL=<mA>` (oppure
`SMIBAT=<mA>` per il ramo batteria) un valore appena superiore al residuo
misurato, attendere il tick FSM e verificare la progressione. Nel caso sopra
hanno chiuso la sequenza `SMICOIL=100` su VEHICLE e `SMICOIL=400` su STATION.
Sono soglie runtime volatili di conferma scarica: non sono limiti di protezione
coil, non vanno trattate come calibrazione e devono essere riesaminate quando
offset o hardware cambiano.

### Dump UART: HFC closed-loop riproducibile a 19 V

Punto consolidato il 2026-09-17 per il banco a bassa tensione: DCLINK SOURCE
24 V, carico elettronico sul DCLINK LOAD in CR 9 ohm, `SOURCE=20 V`,
`LOAD=19 V`, `IMAX=5 A`, `WPTHFCLIM=395 mpu`, run di 45 s. Risultato osservato:
`VLOAD=18,9 V`, `ILOAD=2,1 A`, `IDCLINK=2,74 A`, senza overshoot o fault HFC.

Inviare ogni comando singolarmente e attendere la risposta/stato indicato.
Nessun comando UART durante i 45 s di trasferimento.

1. A bus spenti, con entrambe le FSM in `STANDBY` e radio OK, configurare:

```text
# COM25 / VEHICLE
SMVBUS=5
SMICOIL=100
WPTSRC=20000
WPTLOAD=19000
WPTIMAX=5000
WPTISRC=0
WPTHFCLIM=395
WPTHFCPHAUTO
WPTHFCRAMP=0
WPTCLAMP=0

# COM26 / STATION
SMVBUS=5
SMICOIL=400
WPTSRC=20000
WPTLOAD=19000
WPTIMAX=5000
WPTPLOADINIT=15
```

2. Inizializzare la FSM nell'ordine seguente, verificando le transizioni:

```text
# COM25
MANUAL
SOURCE
# attendere: VEHICLE=INIT_SOURCE, STATION=INIT_LOAD

# COM26
INITOK
# attendere: STATION=WAIT_SOURCE

# COM25
INITOK
# attendere: PRECHARGE_SOURCE e PRECHARGE_LOAD, radio OK

# COM26, poi COM25
WPT=1
WPT=1
```

3. Dopo `DCLINK ON`, avviare e terminare il run senza riarmare il seed:

```text
# COM25
WPTHFC=1
# attendere 45 s senza UART
WPTHFC=0
```

Terminare con `DCLINK OFF`; attendere la chiusura FSM prima di modificare il
punto successivo. Il seed SOURCE va impostato prima di WPT, a bus spenti.

### BOOST closed-loop integrato con HFC: configurazione verificata

Punto verificato il 2026-09-17: VIN fisico 12 V, carico elettronico in CR
9 ohm sul DCLINK LOAD, riferimento BOOST 24 V, `WPTSRC=20 V`,
`WPTLOAD=19,5 V`, `WPTHFCLIM=500 mpu`. In un run di 60 s il DCLINK SOURCE ha
raggiunto 24 V e il LOAD 19,4 V. Il limite osservato sul LOAD appartiene al
trasferimento HFC/UniPD; non e' una saturazione del BOOST.

BOOST isolato riconfermato il 2026-09-18: VIN fisico 12 V / 10 A, carico
elettronico CR 18 ohm sul DCLINK SOURCE, HFC e WPT disabilitati, `UR=24`,
`UD=950`, `US=500`, `UA=UC=10000`, `UN=22`, `UY=40`. Dopo 30 s: DCLINK
misurato 24 V e VDC firmware 23,952 V, corrente VIN osservata 2,74 A, nessun
trip BBC. Questo valida il BOOST isolato in questo punto; non prova da solo
la sequenza HFC/radio integrata.

BOOST+HFC riconfermato il 2026-09-18 con FW1073 e CR 8 ohm sul DCLINK LOAD:
VIN 12 V, `WPTSRC=WPTLOAD=20 V`, `WPTHFCLIM=500`, seed SOURCE 35 W,
`US=500`, `UY=50`. Run di 45 s senza latch BBC, fault HFC o perdita radio;
letture finali LOAD 18,6 V / 2,32 A e assorbimento VIN 7,7 A. Il limite
distribuito ha pubblicato `Pwr2Ld=75 W`; non confonderlo con la potenza
elettrica effettivamente dissipata dal carico.

Per questo punto `UH=1` abilita esclusivamente l'override virtuale di VBAT:
`UW=12000` fornisce 12 V a UniPD. VDC e le correnti dei rami BBC restano
fisici. Il valore deve dunque essere coerente con il VIN realmente applicato;
non usare questa sequenza per validare la misura fisica di VBAT.

Dopo ogni power cycle ripristinare anche le soglie coil FSM: `SMICOIL=100`
su COM25/VEHICLE e `SMICOIL=400` su COM26/STATION, oltre a `SMVBUS=5`.
I default di 1 mA sono inferiori agli offset fisici osservati (circa 80--100 mA
SOURCE e 300--350 mA LOAD) e bloccano la chiusura in `SOURCE_OFF` o
`ENDCHARGE_LOAD`, pur con DCLINK scarico.

### Preflight BOOST+HFC prima di WPT

Il preflight BBC (`UE=0`, `UF=1`, `UQ?`) va eseguito a VIN OFF e con WPT
disabilitato. Non interrogare o resettare latch dopo VIN ON: l'attivita' UART
con WPT gia' operativo puo' degradare il link nRF. Se `UE=1` risponde
`UE REJECTED CONFIG`, non ritentare il comando: disabilitare HFC, porre VIN
OFF, poi interrogare `UQ?` e identificare il predicato non soddisfatto. Il run
del 2026-09-18 ha osservato un VDC-trip a 40,005 V con `UY=40`; pertanto 40 V
non e' una soglia robusta per il transitorio del BOOST del banco. Il run
integrato FW1073 e' stato validato con `UY=50` per 45 s. Ogni ulteriore
incremento richiede una scelta esplicita del responsabile del banco e deve
restare compatibile con il limite DCLINK dell'asset.

**Interpretazione dei limiti:** una soglia o un latch che ostacola un run e'
un dato diagnostico, non un'autorizzazione a rilassare la protezione. Non
alzare, disabilitare o aggirare automaticamente `UY`, trip di corrente,
clamp o altre guardie; raccogliere l'evidenza e attendere la clearance
esplicita dell'utente per una modifica intenzionale. La clearance consente
la modifica richiesta, ma non elimina le altre precondizioni della checklist.

Configurare il BBC SOURCE a VIN OFF, con uscita disarmata, inviando i comandi
singolarmente:

```text
# COM25 / VEHICLE
UE=0
UH=1
UW=12000
UR=24
UY=50
UA=10000
UC=10000
UN=22
UD=950
US=500
UF=1
UH=1
```

`US=500` e' la rampa (500 micro-pu per ciclo); `UD=950` e' il limite duty.
Non confonderli: `UD=500` limita il duty applicato a 0,5 e nel rapporto
12 V -> 24 V non lascia margine per perdite e regolazione. La prova ha quindi
richiesto `UD=950`; tale valore e' una configurazione specifica del banco,
non un nuovo default generale senza ulteriore review.

Dalla release 1073 il seed SOURCE puo' essere configurato prima di WPT:
rimane pendente finche' `WPTHFC=1` e' armato e il VDC SOURCE fisico e' nel
campo UniPD valido. Non riarmarlo tramite UART dopo VIN/DCLINK ON. Il seed
LOAD conserva la guardia esistente: viene applicato solo dopo il primo VDC
LOAD fisico e un limite remoto positivo.

Dopo la sequenza FSM, configurare `WPTPLIMINIT=35` a VIN OFF e WPT disabilitato,
quindi abilitare WPT LOAD e poi SOURCE. Solo allora richiedere `VIN ON`. Dopo
VIN ON non inviare diagnostica UART: usare i comandi seguenti singolarmente e
solo per l'avvio.

```text
# COM25 / VEHICLE
WPTHFC=1
# attendere circa 1 s, senza altri comandi
UE=1
```

L'ordine e il ritardo sono vincolanti per le prove BOOST+HFC del banco:
HFC deve essere gia' in avvio quando viene abilitato il BOOST. Non inviare
`UE=1` nello stesso burst di `WPTHFC=1` ne' prima dell'attesa di circa 1 s.
`US=500` e' la rampa BOOST media verificata per questo percorso e va
mantenuta salvo prova comparativa esplicita; una rampa piu' rapida puo'
produrre picchi di corrente e interrompere l'avvio. Dopo `UE=1` non inviare
UART durante il plateau di potenza: attendere il termine pianificato, poi
inviare nell'ordine `UE=0`, `WPTHFC=0`, `VIN OFF`.

Nel run integrato si puo' avviare HFC circa 1 s prima del BOOST; non inviare
UART durante il plateau. Per chiudere, inviare prima `UE=0`, poi `WPTHFC=0`,
quindi `VIN OFF`. Un `UQ?` eseguito dopo `UE=0` descrive solo lo stato post-stop
e non va usato per giudicare il valore regolato durante il run.

### BUCK isolato: configurazione e riferimento DCLINK

Punto verificato il 2026-09-18: PSU esterno sul DCLINK SOURCE a 20 V,
carico elettronico in CV 12 V sul VBAT LOAD, HFC e WPT disabilitati. Il BUCK
LOAD e' stato validato in closed loop con `UH=2`, `UP=0`, `UV=18000`,
`UA=4000`, `UD=950`, `US=2000`, `UX=24000`, `UC=6000`, `UN=22`; run 45 s:
12 V / 3,9 A sul carico, senza fault.

`UA` e' un limite direzionale di corrente, non un comando di corrente. Il
regolatore BBC accumula la potenza richiesta dall'errore `UV^2 - VDC^2`.
Con un PSU rigido a 20 V e `UV=20000`, l'errore e' quasi nullo e la corrente
sale soltanto per le piccole deviazioni del bus, anche se `UA` viene alzato.
Per chiedere trasferimento rapido dal DCLINK esterno usare un `UV` inferiore
alla tensione fisica del bus (18 V nel punto validato). `US=500` o `US=2000`
agiscono sulla rampa duty BBC, ma non spiegano una dinamica nell'ordine di
decine di secondi quando l'errore energetico e' nullo.

La prova integrata HFC+BUCK resta separata: non usare questo esito per
considerare validato l'avvio HFC, che nel setup corrente ha trip immediato
`HFLT=8` a `i_coil_loc` circa 5,515 A.

Tentativo HFC+BUCK del 2026-09-18: DCLINK SOURCE 24 V / 5 A, VBAT LOAD
CV 12 V, DCLINK LOAD aperto, BUCK `UV=20 V`, `UA=4 A`, `UC=6 A`, `US=2000`,
WPT SOURCE/LOAD 20 V, pre-seed SOURCE 15 W, `WPTHFCLIM=300` e capture
first-fault. Il BUCK e' stato armato prima di HFC. HFC non ha dato trip
istantaneo, ma ha poi raggiunto `HFLT=8`: snapshot 5,518 A, VDC SOURCE
23,792 V, richiesta/applicata 300 mpu e fase hardware 199 mpu. Non e' stato
osservato trasferimento utile. Il contatore `RXINVALID` e' rimasto invariato:
non attribuire l'esito alla radio. La prossima analisi deve riguardare la
catena di misura/limite coil e la corrispondenza fase HFC-corrente; non i
limiti BBC gia' validati in isolamento.

### HFC+BUCK orchestrato: profilo validato

Punto validato il 2026-09-23 con FW1081 e
`WLESS_SM_POWER_CONTROL_ENABLE=1`: DCLINK SOURCE da PSU esterna 24 V / 5 A,
DCLINK LOAD aperto, carico elettronico in CV 12 V sul VBAT LOAD; VIN SOURCE
aperto. Il bootstrap usa SOURCE esterno (`SMBOOTMODE=0`), soglia SOURCE 20 V,
soglia LOAD 15 V, HFC manuale iniziale 500 mpu che avanza verso 400 mpu in
circa 10 s, quindi hand-off senza gradino a UniPD. Configurazione: riferimenti
WPT SOURCE/LOAD 20 V, `WPTIMAX=5000`, seed SOURCE `WPTPLIMINIT=15`,
`WPTHFCLIM=200`, `WPTHFCPH=400`; BUCK LOAD `UH=2`, `UP=0`, `UV=20000`,
`UA=4000`, `UD=950`, `US=2000`, `UX=30000`, `UC=6000`, `UN=22`, `UF=1`.

Run da 60 s: DCLINK LOAD ha raggiunto e regolato 20 V, VBAT LOAD 12 V;
assorbimento PSU circa 1,8 A a regime; nessun HFLT o latch BBC. La corrente
PSU cresce gradualmente per circa 10 s, presenta una variazione visibile al
hand-off `WPTREADY -> WPTON`, poi converge con piccole oscillazioni intorno al
regime. Il comportamento e' evidenza di regolazione attiva, non da solo di
instabilita'.

Per ogni ripetizione, anche senza power cycle, riarmare il seed SOURCE a bus
spenti prima di `WPT=1`: il seed viene consumato dal run precedente. Ometterlo
puo' provocare `WPTOFF` prima del bootstrap e lasciare le FSM non riallineate;
in quel caso non forzare comandi durante la chiusura e usare il power cycle se
non tornano a `STANDBY/IDLE`. La sequenza manuale completa e' in
`docs/HFC_BUCK_CLOSED_LOOP_MANUAL.txt`.

### BOOST+HFC+BUCK orchestrato: profilo FW1097 e osservazioni iniziali

Profilo usato il 2026-09-29 con FW1097: VIN fisico SOURCE 12 V / 10 A,
DCLINK SOURCE e LOAD aperti, carico elettronico CV 12 V su VIN LOAD.
SOURCE: `SMBOOTMODE=1`, `SMBOOTSRC=20`, `SMBOOTDUTY=150`, `SMVBUS=5`,
`SMICOIL=100`, WPT SOURCE/LOAD 20 V, `WPTIMAX=5000`, seed SOURCE 15 W,
`WPTHFCLIM=200`, `WPTHFCPH=400`, clamp/rampa HFC 0; BBC `UH=1`, `UW=12000`,
`UR=24`, `UY=50`, `UA=UC=10000`, `UN=22`, `UD=950`, `US=500`, `UF=1`.
LOAD: `SMBOOTLOAD=15`, `SMVBUS=5`, `SMICOIL=400`, WPT SOURCE/LOAD 20 V,
`WPTIMAX=5000`; BUCK `UH=2`, `UP=0`, `UV=20000`, `UA=4000`, `UD=950`,
`US=2000`, `UX=30000`, `UC=6000`, `UN=22`, `UF=1`.

In un run di 45 s sono stati osservati: precharge BOOST a circa 0,1 A e
DCLINK SOURCE prossimo a 24 V; dopo 6--7 s il bus SOURCE e' sceso verso 19 V;
fra 17--18 s il LOAD ha raggiunto 12 V con corrente ancora bassa; fra 22--25 s
il PSU e' salito da circa 1,2 a 3 A mentre VDC SOURCE ha mostrato oscillazioni
ampie, poi si e' stabilizzato intorno a 24 V. L'hand-off a UniPD e' apparso
completo solo verso 40 s. Non usare `PLIM` letto dopo `UE=0/WPTHFC=0` per
caratterizzare il plateau: e' una grandezza gia' alterata dalla chiusura.

Il seed SOURCE e' un'inizializzazione dello stato UniPD, non un clamp
permanente: dopo il consumo il limite e' ricalcolato dal controllo TX. Nel
profilo attuale `UV=20000` sul LOAD rende quasi nullo l'errore energetico una
volta che DCLINK LOAD e' a 20 V; per una prova intenzionale di maggiore domanda
BUCK, confrontare separatamente `UV=18000`, senza modificare bootstrap o
limiti SOURCE. Usare una finestra almeno 90 s per distinguere bootstrap,
hand-off e regime UniPD.

I due script manuali corrispondenti sono
`scripts/BOOST_HFC_BUCK_CL_MANUAL_VEHICLE.wcx` e
`scripts/BOOST_HFC_BUCK_CL_MANUAL_STATION.wcx`. Sono divisi in fasi perche'
la FSM distribuita richiede ordine interleaved: VEHICLE A, STATION A,
VEHICLE B, STATION B, VEHICLE C. Non eseguirli integralmente come un unico
burst per ciascuna porta.

### Strategia vincolante per il precharge BOOST a DCLINK aperto

Nel test completo BOOST+HFC+BUCK il DCLINK SOURCE e' inizialmente quasi un
carico capacitivo: prima dell'innesco HFC non esiste un percorso che assorba
in modo significativo l'energia del BOOST. La rampa del solo riferimento VDC
puo' quindi essere lenta (profilo di lavoro: +1 V/s), ma non deve essere
accompagnata dal rilascio temporizzato di corrente o duty verso i limiti
ordinari. Un rilascio di questo tipo puo' caricare il condensatore oltre il
gradino VDC richiesto e produrre un trip di sovratensione prima che HFC sia
abilitato.

La sequenza desiderata e':

```text
SMBA superato (solo consenso di presenza VDC/VIN)
  -> riferimento BOOST cresce per gradini, tipicamente +1 V/s;
  -> limite corrente BBC bootstrap resta basso e fisso;
  -> limite duty BOOST bootstrap resta basso e fisso;
  -> il gradino VDC successivo e' ammesso solo dopo tracking/stabilizzazione
     del gradino corrente;
  -> raggiunti e mantenuti il riferimento finale BOOST (24 V nel banco),
     rimuovere i cap bootstrap e soltanto allora abilitare HFC;
  -> BUCK viene abilitato dal proprio lato solo dopo il livello VDC LOAD
     previsto dalla FSM.
```

La variabile fisica da limitare durante il precharge e' l'energia immessa nel
condensatore DCLINK SOURCE, equivalente alla sua pendenza `dVDC/dt`
(`I = C*dV/dt` in prima approssimazione). `SMBA` non e' un limite dinamico:
e' esclusivamente il consenso di armamento. Nel firmware osservato fino a
FW1112 il cap iniziale `SMBOOTDUTY` e il limite corrente bootstrap venivano
rilasciati nel tempo: questo comportamento e' incompatibile con il precharge
a circuito aperto e non va esteso. Il valore fisicamente corretto del cap
duty deve essere scelto dopo capture BBC, perche' `SMBOOTDUTY` agisce dopo la
mappatura BOOST e non coincide automaticamente con il duty raw UniPD.

Caratterizzazione del banco eseguita il 2026-10-01, BOOST isolato a DCLINK
SOURCE aperto, VIN SOURCE 12 V, `UR=24 V`, `UA=50 mA`, HFC e WPT disabilitati:
`UD=50` ha raggiunto 27 V, `UD=25` circa 26 V, `UD=20` circa 25,5 V e
`UD=10` circa 24,5 V. Per questo specifico precharge il riferimento empirico
e' quindi `UD=10 mpu` (da riportare nel futuro cap bootstrap, non nel limite
permanente di funzionamento). Non estrapolarlo a VIN, bus, condensatori o
hardware differenti senza una nuova caratterizzazione.

Per ogni nuovo profilo di tensione BOOST a DCLINK inizialmente aperto va
quindi registrata una breve caratterizzazione precharge: VIN, setpoint `UR`,
limite corrente bootstrap, cap duty bootstrap, rampa del riferimento, massimo
VDC osservato e tempo di assestamento. Il cap duty non va dedotto per semplice
proporzione da un altro setpoint. Il profilo e' accettabile come condizione di
innesco HFC solo se VDC raggiunge e mantiene la finestra del setpoint senza
trip BBC e senza superare la banda di sovratensione scelta per il banco.

### Precharge BBC ePWM protetta (FW1135)

La diagnostica GPIO `BG=0/1` e' disabilitata in FW1135. Per il bootstrap
SOURCE in modalita' `SMBOOTMODE=1`, prima del consenso BBC ordinario il
firmware mantiene EPWM6/EPWM7 in ownership ePWM con il pattern statico
high-side ON / low-side OFF. Non e' PWM e non e' BOOST open-loop: serve a
ricostituire il punto passivo VIN -> DCLINK verificato a banco, conservando
la protezione Trip-Zone che puo' forzare entrambe le uscite LOW.

Il comando esistente `SMBA=<V>` imposta `WLESS_SM_bootBbcArmVBusMin_V`:

- non e' `SMBOOTSRC` e non abilita HFC;
- non e' un riferimento o un limite di regolazione;
- e' esclusivamente la soglia VDC fisica che consegna EPWM dal pattern statico
  al BOOST UniPD asincrono;
- superata `SMBA`, il firmware forza prima OST, poi abilita UniPD con il cap
  `SMBOOTDUTY` gia' configurato.

Per la verifica isolata della precharge ePWM: VIN SOURCE 12 V / 2 A OFF,
CR 8 ohm ON su DCLINK SOURCE, DCLINK LOAD e VIN LOAD aperti; configurare
`SMBOOTMODE=1`, `SMBA=30`, `SMBOOTSRC=20`, `SMBOOTDUTY=10`. Con `SMBA=30`
la consegna a UniPD resta intenzionalmente inibita: il risultato atteso e'
DCLINK SOURCE prossimo a VIN senza commutazione BBC/HFC. La FSM distribuita
deve comunque essere portata a `PRECHARGE_SOURCE`; non sostituire questa
prova con `UE=1`, `UB=1` o `WPTHFC=1`.

## 6. Soglie runtime e guardie analogiche

Le configurazioni UART volatili tornano ai default dopo flash o power cycle.
Prima di interpretare una permanenza in PRECHARGE come errore di HFC, UniPD o
nRF, interrogare sempre `SMTHR?`.

La guardia di transizione usa:

```text
WLESS_SM_vBus_V > WLESS_SM_vBusMin_V
```

Il default di `SMVBUS` e' 50 V. Nei test a bassa tensione impedisce
legittimamente `PRECHARGE_SOURCE -> SOURCE_ON`. Per la baseline HFC a DCLINK
12 V con riferimenti 10 V e' stato usato `SMVBUS=5` su entrambe le schede.

Checklist obbligatoria dopo ogni flash/power cycle:

```text
SMTHR?
SMVBUS=<valore compatibile con il punto di prova>
SMTHR?
```

Se SOURCE resta in `PRECHARGE_SOURCE`, la cadenza operativa dei payload puo' non
attivarsi e rimanere circa 1 Hz. Questa e' prima di tutto una conseguenza della
guardia non soddisfatta, non una prova autonoma di guasto radio.

## 7. Grandezze fisiche, sintetiche e virtuali

Non assumere mai che un valore nominalmente chiamato `VB`, `VBAT`, `VIN`,
`VDC`, `IB` o `ITANK_MOD` sia fisico. Prima di ogni test dichiarare per ciascun
ingresso:

- grandezza fisica rappresentata;
- sorgente ADC oppure override UART;
- unita' e scala;
- validita' della calibrazione sulla scheda in uso;
- valore effettivamente passato a UniPD/FSM.

Distinzioni fondamentali:

- `VDC` e' normalmente la tensione del DCLINK misurata;
- `VBAT/VIN` e' il lato batteria/ingresso BBC, ma puo' essere fisico o sostituito
  da un override a seconda della modalita' e della maschera sintetica;
- un valore `VB` usato dalla logica WPT puo' essere virtuale/sintetico e non va
  confuso automaticamente con una tensione ADC;
- `IB` puo' essere un parametro/valore sintetico distinto dalle correnti di ramo;
- `ITANK_MOD/I_COIL` puo' essere fisico oppure sintetico separatamente su SOURCE
  e LOAD;
- `ITANK_PHS` non e' disponibile sull'asset consolidato e non deve essere
  presentato come misura fisica.

Comandi diagnostici da usare prima del run, secondo il controllo in prova:

```text
UP?
UQ?
WPT?
CQ?
SMTHR?
```

### Ordine vincolante dei primi campi `UQ,1`

`UQ?` restituisce una riga compatta non auto-descrittiva. Non dedurre mai il
significato di un campo dal suo valore numerico. Dopo il prefisso `UQ,1`, i
primi sette campi sono, nell'ordine:

| Posizione | Campo | Unita' / significato |
|---:|---|---|
| 1 | `SyntheticTestEnable` | boolean |
| 2 | `SignalValidMask` | bit mask |
| 3 | `SignalMissingMask` | bit mask; ad esempio `2016` non e' una tensione |
| 4 | `VDC_mV` | tensione DCLINK usata da UniPD |
| 5 | `VBAT_effective_mV` | VBAT usata da UniPD; puo' essere virtuale/override |
| 6 | `VBAT_ADC_raw` | conteggio ADC VIN |
| 7 | `VBAT_sensed_mV` | conversione fisica VIN/VBAT da ADC |

Per validare la catena fisica VIN usare esclusivamente `VBAT_sensed_mV`
(e, come controllo, `VBAT_ADC_raw`); non usare `VDC_mV` ne'
`VBAT_effective_mV` quando un profilo puo' applicare un override.

### Calibrazioni dell'asset corrente

Le catene di misura fisiche dell'asset corrente (VDC/VBAT, correnti di ramo
BBC e corrente tank HFC) sono state calibrate e validate nelle campagne
precedenti. In assenza di sostituzione hardware o di modifiche firmware che
tocchino coefficienti, mapping ADC, maschere sintetiche, polarita' o macro di
acquisizione, trattare le grandezze fisiche pubblicate da `UQ?`, `WPT?` e
`HFC?` come misure coerenti. Non riproporre una verifica/calibrazione generale
come spiegazione alternativa di un comportamento gia' acquisito; motivarla
solo indicando il cambiamento concreto che rende nuovamente incerta la catena.

In `WPT?`, controllare sempre almeno `ISYN`, `ILSYN`, `IPHY`, `IUSE`, riferimenti,
limiti e seed. Un test con dato sintetico valida il software/segno/sequenza, non
la catena analogica.

Le calibrazioni `IO=151` SOURCE e `IO=306` LOAD sono valori storici dell'asset
usato nel 2026 e sono volatili. Verificarli con `CQ?` e con corrente nulla; non
applicarli ciecamente dopo modifiche hardware, scambio schede o nuova taratura.

L'offset WPT del LOAD e' invece persistibile. A stadio disabilitato, la sequenza
corretta e' `WPTIZERO=<mA>`, `CA`, `CS` sulla sola STATION; `CA` copia la
configurazione pending in active e `CS` salva quella active in flash. Verificare
il valore persistito nell'ultimo campo di `CQ?` e in `WPT?` dopo il power cycle.
Non eseguire una nuova media IPHY se la scheda e la catena analogica non sono
cambiate e il valore persistito e' plausibile.

### Polarita' IL_A/IL_B BBC: evidenza consolidata

Non proporre `UP=1/2/3` come reazione a un singolo sbilanciamento fra rami.
La prova sincronizzata archiviata in `WLESS_CHG_TEST_REPORT.md`, righe
5116--5162, con DCLINK 20 V, carico 83 ohm, `UA=300 mA` e `UD=850 mpu`, ha
mostrato riferimenti identici e negativi, entrambe le correnti `IL_A/IL_B`
negative e una reazione del duty coerente per il ramo B meno carico. Esclude
un'inversione di segno come causa primaria dello sbilanciamento osservato.
La diagnostica BOOST isolato FW1068, righe 5263--5277 dello stesso report,
ha inoltre riportato in regime `IL_A/IL_B` entrambe positive (circa
0,65--0,87 A) con VIN 20 V, CR 18 ohm, `UR=24 V` e `UH=1`.

Queste prove non dimostrano identita' perfetta dei due percorsi: restano
possibili differenze di percorso, offset o guadagno. Per cambiare `UP` serve
una nuova prova che leghi simultaneamente corrente esterna, valori raw,
valori corretti e direzione di potenza nel medesimo quadrante.

## 8. HFC e radio: baseline corrente

- Il rectifier LOAD resta passivo salvo requisito esplicito differente.
- Frequenza HFC usata nella campagna corrente: circa 80 kHz; verificarla con
  `HFC?` e strumentazione, non assumerla.
- La baseline open-loop HFC consolidata a 24 V / 8 ohm usa esclusivamente il
  percorso diretto `HFC=1` per 20 s, seguito da `HFC=0`. Non sostituirlo con
  `WPTHFCPH=150`: e' un percorso di attuazione diverso e produce un punto di
  lavoro inferiore. Valori storici di confronto: IDCLINK circa 3,0 A, VLOAD
  circa 18,8 V e ILOAD circa 2,35 A, senza dropout.
- I default nominali del percorso closed-loop sono `WPTCLAMP=0` e
  `WPTHFCRAMP=0`; il clamp legacy e la rampa del wrapper sono riservati a prove
  comparative esplicitamente motivate.
- Convenzione manuale consolidata: 500 mpu e' circa neutro; riducendo `MPH`
  aumenta il trasferimento. La caratterizzazione 24 V / 8 ohm ha raggiunto la
  zona utile intorno a MPH 150-250.
- **HFC neutro non equivale a HFC disattivo.** Il comando di fase manuale a
  `MPH=500` porta la modulazione differenziale al punto neutro, ma lascia il
  bridge HFC/PWM armato (`HENA=1`). Nel bootstrap BOOST+HFC+BUCK del banco,
  l'armamento neutro ha comunque modificato il bilancio del DCLINK SOURCE e ne
  ha accelerato la scarica. Se la fase richiede HFC elettricamente assente,
  usare l'attuatore disabilitato (`HENA=0`), non soltanto `MPH=500`.
- Nel mapping automatico corrente, la relazione osservata e'
  `MPH_fisico = 500 - HREQ`; `WPTHFCLIM=350` consente di raggiungere circa
  MPH 150. Verificare sempre `HREQ`, `HAPP`, `HLIM` e `HPHY`.
- Con SOURCE alimentato da un DCLINK esterno a 24 V, `WPTSRC` non e' un
  parametro puramente descrittivo: entra nell'errore del limite di potenza
  SOURCE come `VDC_SOURCE^2 - WPTSRC^2`. Un riferimento 10 V puo' quindi
  portare il limite al massimo anche se il LOAD richiede meno potenza. Nel
  punto CR=9 ohm, LOAD=18 V, `WPTSRC=20 V` ha dato 17,9 V / 1,98 A senza
  overshoot, mentre `WPTSRC=10 V` saturava HFC a 350 mpu intorno a 18,4 V.
  Scegliere `WPTSRC` sotto ma vicino al DCLINK fisico e verificarne l'effetto
  su `PLIM_W/Pwr2Ld`; non impostarlo uguale alla sorgente senza valutare che
  il trasferimento richiesto potrebbe diventare nullo.
- Dalla release 1073 il seed SOURCE e' trattenuto finche' `WPTHFC=1` e VDC
  SOURCE fisico e' nel campo UniPD valido. Configurarlo prima di WPT, a bus
  spenti; non riarmarlo dopo DCLINK/VIN ON. Verificare `PSEEDP` prima
  dell'avvio e `PLIM_W` dopo il primo ciclo HFC.
- Nel punto precedente, `WPTHFCLIM=450` ha causato `HFLT=8` (trip coil) anche
  con soglia software portata a 5,25 A; usare 350 mpu come limite validato
  salvo caratterizzazione esplicita della corrente coil.
- Dopo flash/power cycle, ripristinare e verificare esplicitamente riferimenti,
  limiti, rampa, seed, calibrazioni e soglie FSM.
- Non modificare guadagni UniPD prima di avere escluso: guardie FSM, segni,
  saturazioni, dati sintetici involontari, mapping ePWM/HFC e feedback radio.

### Codici `HFLT` dell'attuatore HFC

`HFLT` riportato da `WPT?`/`HFC?` e' un codice di causa mutuamente esclusivo,
non una bitmap. Un valore diverso da zero disabilita immediatamente l'attuatore
HFC: azzera richiesta e fase applicata, disabilita il receiver test e forza i
trip PWM delle due gambe primarie. I codici correnti sono:

| `HFLT` | Causa | Interpretazione operativa |
|---:|---|---|
| 0 | nessun fault | attuatore disponibile o non armato normalmente |
| 1 | WPT/integrazione UniPD disabilitata | HFC richiesto senza integrazione abilitata |
| 2 | ruolo locale diverso da SOURCE | HFC puo' essere attuato solo dal SOURCE |
| 4 | `RadioLink != OK` | perdita link radio durante HFC |
| 5 | `NOACK >= 15` | watchdog di mancati ACK radio |
| 6 | FSM locale non in `PRECHARGE_SOURCE` ne' `SOURCE_ON` | comando HFC fuori dalla finestra FSM ammessa |
| 7 | integrazione disabilitata o ruolo locale non SOURCE/LOAD durante il ciclo UniPD | invalidazione runtime del contesto WPT |
| 8 | corrente tank fisica locale `i_coil_loc > CLLLC_iTankModLimit_Amps` | trip corrente coil; nel firmware corrente il limite e' inizializzato a 5,25 A |

Il codice 8 osservato il 2026-09-18 a DCLINK SOURCE 26 V / CR 8 ohm ha
interrotto il run a circa `VLOAD=19,7 V`, `ILOAD=2,45 A`, `IDCLINK~=3 A`.
Non classificarlo come normale saturazione: il comando di fase puo' essere
anche al limite, ma `HFLT=8` e' un arresto di protezione software. Prima di
alzare quel limite validare sensore, cablaggio, corrente tank reale e margine
hardware; il confronto con la misura ADC satura non basta da solo.

Il limite tank effettivo e' il campo persistito `iTankLimit_mA` (`CQ?`, sesto
valore dopo la versione), applicato all'avvio dal profilo flash e quindi
prevalente sull'inizializzazione transitoria CLLLC. Con offset 120 mV,
sensibilita' 573 mV/A e ADC 3,3 V, il massimo misurabile prudente e' 5500 mA.
Dal firmware 1072 il default e il massimo accettato da `IM=` sono 5500 mA.
Un record flash precedente resta invariato: a DCLINK/VIN OFF applicare sulla
VEHICLE `IM=5500`, `CA`, `CS`, quindi verificare `CQ?` prima di qualunque run.

Per il firmware dalla release 1071, `WPTCAP=3` sulla sola SOURCE arma un
capture first-fault HFC: al primo `HFLT!=0` il firmware congela il ring buffer
(`REASON=4`) e memorizza nello stesso tick uno snapshot dedicato della corrente
fisica di trip, VDC e fase HREQ/HAPP/HPHY. Il ring conserva il contesto
precedente; lo snapshot, non l'ultimo sample decimato, e' il valore probatorio
del trip. `WPTCAP?` riporta `FSNAP=1` e i campi `F*`; `WPTCAPD?` emette anche
`WPTCAPF,1,...`. Non usare `WPTCAP=0` o `WPTCAPD?` durante il trasferimento;
dopo il trip verificare lo stato e poi leggere il dump a HFC gia' disabilitato.
Questo e' diagnostico e non modifica nessun limite o azione di protezione.

Il firmware dalla release 1046 include validazione semantica del payload nRF:

- `RXVALID` conta i payload pubblicati;
- `RXINVALID` conta i payload scartati;
- i campi remoti sono aggiornati solo dopo validazione;
- il watchdog STATION viene azzerato solo da un payload valido;
- `MAXRT`, `NOACK`, `RXINVALID`, ruolo/stato/abort remoti vanno confrontati prima
  e dopo ogni run.

Il payload applicativo non contiene ancora un CRC/sequence number end-to-end;
il nRF usa il proprio CRC radio. Non affermare che la causa di una corruzione sia
RF, SPI o software senza evidenze diagnostiche.

## 9. Controllo di potenza e macro

- `WLESS_SM_POWER_CONTROL_ENABLE=0` e' intenzionale e non significa che tutti i
  test UART siano open-loop: la modalita' di test corrente puo' comandare i
  wrapper/UniPD mediante percorsi UART separati dalla piena orchestrazione FSM.
- Non abilitare `WLESS_SM_POWER_CONTROL_ENABLE=1` senza review esplicita.
- Il PWM complementare/sincrono e' presente solo sotto macro disabilitata e non
  e' validato sul banco. Non abilitarlo senza setup bidirezionale, verifica gate,
  deadband, polarita' e percorso energetico.
- Non utilizzare TIMER1. Il tick FSM deriva dalla sorgente ISR2 esistente ed e'
  decimato a 1 kHz.

## 10. Limiti essenziali dell'asset

Verificare sempre i dati reali di targa prima del run. Baseline storica:

- PSU: 0-20 V / 20 A oppure 0-60 V / 7 A;
- 1,2 kohm: circa 1 W, non superare 20 V;
- 83 ohm: 150 W;
- 2,2 ohm: 250 W per resistore; sono disponibili due elementi;
- carico elettronico: configurazione e stato ON/OFF devono essere confermati;
- sensore coil modificato: relazione nominale comunicata dal DT
  `V_I_COIL_adc = 0,573 * I_COIL + 0,120 V`, limite coil 5 A e DCLINK non oltre
  32 V per quella configurazione hardware.

Definire prima del test condizioni di arresto per sovracorrente, sovratensione,
temperatura, perdita radio e comportamento oscillatorio. Non estrapolare la
sicurezza da un test precedente con carico o cablaggio diverso.

## 11. Disciplina di analisi e documentazione

- Distinguere sempre fatti osservati, inferenze e ipotesi.
- Non ripetere il bring-up open-loop gia' validato salvo smoke test motivato.
- Il test report contiene esclusivamente attivita' e risultati di test.
- Le modifiche funzionali vanno documentate separatamente.
- Aggiornare la knowledge base quando viene consolidata una regola riutilizzabile.
- Non eseguire commit o push salvo richiesta esplicita. L'utente normalmente
  gestisce il push su origin.
- Conservare le modifiche e i file locali dell'utente; non ripulire o cancellare
  materiale non correlato senza autorizzazione.

## 12. Regola finale prima di ogni test closed-loop

Non inviare il comando di avvio finche' non sono stati verificati e comunicati:

1. `FW?` e `ROLE?` su entrambe;
2. power cycle e ordine USB/logica;
3. stato iniziale DCLINK/VIN/VLOAD;
4. carico, limiti PSU e temperatura;
5. `SMTHR?`, in particolare `SMVBUS`;
6. sequenza `INITOK` completata;
7. origine fisica/sintetica di ogni ingresso del controllo;
8. riferimenti, limiti, seed, rampa e mapping attuatore;
9. baseline `RADIO?`, inclusi `RXINVALID`, `MAXRT` e `NOACK`;
10. durata, strumenti osservati e condizioni di arresto.

## 13. Priorita' tecnica corrente

La priorita' di sviluppo e prova e' l'abilitazione e la validazione del
pilotaggio **sincrono** del BBC:

1. prima il BOOST sincrono;
2. poi il BUCK sincrono.

Non deviare verso ulteriori caratterizzazioni della variante asincrona, salvo
che siano strettamente necessarie per rendere sicura o verificabile la
transizione sincrona. Le attivita' di bootstrap, precharge e hand-off devono
essere valutate anzitutto rispetto a questo obiettivo.

### Configurazione ePWM BBC sincrona: verifica statica 2026-10-20

La configurazione sincrona e' gia' presente nel codice ma e' oggi disabilitata
da `TTPLPFC_BBC_COMPLEMENTARY_PWM_ENABLE=0` in
`ttplpfc/ttplpfc_user_settings.h`.

Con macro a 1, sia BOOST sia BUCK richiamano
`TTPLPFC_HAL_setupBBCComplementaryPWM()` attraverso rispettivamente
`TTPLPFC_HAL_setupBBCBoostLowSidePWM()` e
`TTPLPFC_HAL_setupBBCBuckHighSidePWM()`:

- EPWM6A/GPIO10 e EPWM7A/GPIO12 sono il comando high-side;
- EPWM6B/GPIO11 e EPWM7B/GPIO13 sono generati dal modulo dead-band come
  complementari di A, non da un secondo comando software;
- AQ genera A LOW al confronto CMPA in conteggio crescente e HIGH al confronto
  CMPA in conteggio decrescente;
- RED usa A con polarita' attiva-alta; FED usa A con polarita' attiva-bassa:
  il risultato e' B complementare con non-overlap hardware;
- RED=FED=0,1 us, cioe' 12 tick con `PWMSYSCLOCK=120 MHz`;
- OST e i trip hardware configurano TZA e TZB entrambi LOW, quindi possono
  spegnere insieme high-side e low-side anche in modalita' complementare.

Il mapping fisico A/high-side e B/low-side e' gia' sostenuto dai test
asincroni low-side e dal precharge ePWM high-side statico; non deve essere
ripetuto come verifica autonoma. La prova ancora necessaria e' la transizione
protetta precharge ePWM -> configurazione complementare/PWM, verificando che
la dead-band sia riabilitata prima del rilascio OST e che non sopravvivano
forzature AQ o Trip-Zone del precharge.

### Test dedicato BOOST sincrono open-loop

Il test e' sviluppato esclusivamente nel worktree
`C:\Users\m.santucci\.codex\worktrees\boost-sync-openloop\WLESS_CHG_F28003x`.
Non usa FSM, WPT, HFC, UniPD o `SMBA`.

La sequenza UART sperimentale e' intenzionalmente a due fasi:

```text
UB=3  # precharge ePWM statico: high-side ON, low-side OFF
UB=1  # hand-off: OST, AQ/dead-band complementari, poi PWM BOOST sincrono
```

`UB=1` conserva la semantica storica del duty BOOST come `Dlow`; nel percorso
sincrono il firmware applica al compare high-side `Dhigh = 1 - Dlow`, mentre
il modulo dead-band genera il low-side complementare. L'handoff non dipende
da una soglia `SMBA`: e' esplicitamente comandato dall'operatore dopo avere
osservato il precharge. Configurare una rampa `US` bassa per il primo test,
perche' il ramp parte da `Dlow=0` (high-side quasi continuo, coerente con il
precharge) e raggiunge gradualmente il `Dlow` richiesto.
