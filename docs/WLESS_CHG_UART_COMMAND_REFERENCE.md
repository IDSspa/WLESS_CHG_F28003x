# Riferimento comandi UART correnti

## Scopo e fonte

Questo documento descrive i comandi implementati nel parser del firmware
corrente, ricavati da `wless_uart/wless_uart.c`. Non e' una sequenza di test e
non sostituisce `AGENTS.md`: in particolare, durante trasferimento di potenza
non inviare interrogazioni UART se non previste esplicitamente dal test.

I comandi sono sensibili alla forma indicata (`UE=1`, senza spazi). Il parser
puo' tecnicamente ricevere piu' token separati da `;`, ma la pratica operativa
del progetto richiede un comando per volta, attendendo la risposta.

Molti parametri BBC/WPT disabilitano l'uscita o il relativo attuatore prima di
essere aggiornati: configurarli sempre a bus spenti e riabilitare
esplicitamente l'attuatore al termine.

## Identificazione, stato e radio

| Comando | Effetto |
|---|---|
| `FW?` | Riporta la release compilata. |
| `ROLE?` | Riporta il ruolo compilato: `VEHICLE` oppure `STATION`. |
| `VARS?` | Snapshot della FSM locale: stato, ruolo, `CtrlState`, link, grandezze FSM. |
| `RADIO?` | Stato e contatori nRF24 della scheda locale. |
| `RADIOPING` | Richiede un ping diagnostico nRF24; non usare durante un plateau di potenza. |
| `WH=<Wh>` | Imposta l'energia/capacita' locale codificata per il protocollo FSM. |
| `WH?` | Legge il valore `WH` locale decodificato. |
| `VB=<V>` | Scrive il valore VBUS usato dalla FSM locale; comando diagnostico/sintetico, non una misura ADC. |
| `IB=<mA>` | Scrive la corrente batteria usata dalla FSM locale; diagnostico/sintetico. |
| `IC=<mA>` | Scrive la corrente coil usata dalla FSM locale; diagnostico/sintetico. |

## FSM distribuita

| Comando | Effetto | Disponibilita' |
|---|---|---|
| `SOURCE` | Richiede il ruolo SOURCE e cancella uno `STOP` pendente. | Solo immagine VEHICLE. |
| `LOAD` | Richiede il ruolo LOAD locale. | Solo immagine VEHICLE. |
| `AUTO` | Seleziona evoluzione automatica della FSM. | Solo VEHICLE. |
| `MANUAL` | Seleziona evoluzione manuale della FSM. | Solo VEHICLE. |
| `INITOK` | Conferma locale dell'inizializzazione; la FSM lo trasporta alla controparte secondo il protocollo. | Entrambe. |
| `STOP` | Avvia la chiusura distribuita della sessione. Non e' equivalente a `WPT=0`. | Solo VEHICLE. |

## Configurazione e persistenza delle calibrazioni

| Comando | Effetto |
|---|---|
| `CQ?` | Legge configurazione attiva: sorgente/sequence, offset e sensibilita' tank, limite tank, offset correnti BBC e offset WPT LOAD. |
| `IO?`, `IG?`, `IM?` | Leggono rispettivamente offset tank pending, sensibilita' tank pending e limite tank pending. |
| `CV?` | Valida la configurazione pending senza applicarla. |
| `IO=<mV>` | Imposta offset tank pending. |
| `IG=<mV/A>` | Imposta sensibilita' tank pending. |
| `IM=<mA>` | Imposta limite corrente tank pending. |
| `AO=<raw>` | Imposta offset ADC pending del ramo BBC A. |
| `BO=<raw>` | Imposta offset ADC pending del ramo BBC B. |
| `WPTIZERO=<mA>` | Imposta offset coil WPT LOAD pending; non salva da solo in flash. |
| `CA` | Copia la configurazione pending nella configurazione active, solo a stadi inattivi. |
| `CS` | Salva in flash la configurazione active, solo a stadi inattivi. |
| `CR` | Ricarica configurazione da flash; usa i default se il record non e' valido. |
| `CD` | Carica i default nella configurazione pending, senza salvare automaticamente. |

## Capture BBC e WPT

| Comando | Effetto |
|---|---|
| `CAP=1` | Arma il capture BBC; rifiuta l'armamento se esiste gia' un latch BBC. |
| `CAP=0` | Congela/ferma il capture BBC. Da usare prima dello stop del run. |
| `CAP?` | Stato capture BBC. |
| `CAPD?` | Dump capture BBC congelato. |
| `WPTCAP=1` | Arma capture WPT/HFC normale. |
| `WPTCAP=2` | Azzera la trace radio e arma trigger su evento/drop radio. |
| `WPTCAP=3` | Arma trigger su fault HFC. |
| `WPTCAP=0` | Congela/ferma capture WPT/HFC prima dello stop. |
| `WPTCAP?` | Stato capture WPT/HFC. |
| `WPTCAPD?` | Dump capture WPT/HFC congelato. |
| `WPTCAPDEC=<N>` | Imposta decimazione WPT capture; ammesso solo con capture disarmato. |

## BBC / UniPD: diagnosi e configurazione comune

| Comando | Effetto |
|---|---|
| `UQ?` | Snapshot BBC/UniPD: configurazione, misure, duty, enable, trip e diagnostica. |
| `UT=0` | Disabilita il vettore di test UniPD sintetico. |
| `UT=1..20` | Carica uno dei vettori BBC sintetici di test e disabilita l'uscita. |
| `UH=1` | Seleziona profilo SOURCE/BOOST. Con la configurazione diagnostica corrente puo' usare `UW` come override VBAT; VDC e correnti restano fisiche. |
| `UH=2` | Seleziona profilo LOAD/BUCK, con limite di corrente nel verso BUCK. |
| `UH=0` | Disabilita il profilo ibrido/sintetico BBC. |
| `UE=1` | Richiede l'abilitazione closed-loop BBC; e' accettato solo se configurazione, mapping e latch sono validi. |
| `UE=0` | Disabilita subito BBC e forza i PWM in stato sicuro. |
| `UR=<V>` | Riferimento VDC BBC in volt interi. |
| `UV=<mV>` | Riferimento VDC BBC in millivolt; offre maggiore risoluzione rispetto a `UR`. |
| `UW=<mV>` | Configura VBAT sintetica/diagnostica. Diventa ingresso effettivo solo quando lo consente il profilo `UH` e la macro relativa. |
| `UA=<mA>` | Modulo del limite direzionale di corrente: BOOST `[0,+UA]`, BUCK `[-UA,0]`. |
| `UD=<mpu>` | Limite duty BBC in milli-per-unit, massimo 950. |
| `US=<micro-pu>` | Passo della rampa duty BBC in micro-per-unit per ciclo. |
| `UM=0/1` | Seleziona il mapping del duty UniPD; comando diagnostico, non cambiare a stadio attivo. |
| `UP=0..3` | Maschera di inversione segno correnti BBC: bit 0 ramo A, bit 1 ramo B. Rifiutato con BBC o docking attivo. |
| `UI=<mA>` | Scrive correnti BBC sintetiche opposte A/B nel test sintetico completo; disabilita l'uscita. |
| `UF=1` | Richiede reset dei latch BBC. |
| `UF=2` | Calibra a corrente nulla gli offset BBC A/B e aggiorna la configurazione active; non salva in flash da solo. |
| `UX=<mV>` | Soglia trip VDC BBC in millivolt. |
| `UY=<V>` | Soglia trip VDC BBC in volt; richiede anche reset del latch. |
| `UC=<mA>` | Soglia assoluta di trip per ciascun ramo BBC. |
| `UN=<cicli>` | Numero di campioni consecutivi oltre `UC` per confermare il trip. |

## BBC: docking/open-loop

| Comando | Effetto |
|---|---|
| `UB=0` | Disabilita docking/open-loop BBC. |
| `UB=1` | Avvia docking open-loop BOOST su entrambi i leg, con duty massimo `UD`, rampa `US` e trip `UY`/`UX`. |
| `UB=2` | Avvia docking open-loop BUCK su entrambi i leg, con gli stessi limiti. |

`UB=3` non appartiene al parser del firmware principale corrente: e' una
estensione sperimentale esclusiva del worktree `boost-sync-openloop` per il
precharge ePWM statico prima dell'handoff sincrono.

## HFC diretto e WPT distribuito

| Comando | Effetto |
|---|---|
| `HFC=1` | Arma direttamente il ponte HFC primario per prova banco; non configura WPT/UniPD. |
| `HFC=0` | Disarma HFC diretto e forza OST. |
| `HFC?` | Riporta stato HFC, fault, frequenza, duty/fase e diagnostica attuatore WPT. |
| `WPTHFC=1` | Arma l'attuatore HFC controllato da WPT; richiede integrazione WPT, SOURCE locale, LOAD remoto, radio valida e corrente coil valida. |
| `WPTHFC=0` | Disarma il solo attuatore HFC WPT. |
| `WPTHFCLIM=<mpu>` | Limite massimo fase HFC, 1...500 mpu. Aggiorna anche il limite di bootstrap. |
| `WPTHFCRAMP=<micro-pu>` | Passo rampa fase HFC, fino a 100 micro-pu/ciclo. |
| `WPTHFCPH=<mpu>` | Imposta fase HFC manuale e abilita modalita' manuale; aggiorna il target bootstrap. |
| `WPTHFCPHAUTO` | Torna alla fase HFC calcolata automaticamente. |
| `WPTHFCINV=0/1` | Inverte mapping della fase HFC. |
| `WPTIRINV=0/1` | Inverte il segno dell'errore coil remoto usato dal WPT. |
| `WPTCLAMP=0/1` | Abilita/disabilita il clamp VAC legacy; modifica ammessa a HFC fermo. |
| `WPT=1` | Abilita il calcolo WPT distribuito; non scrive PWM da solo. |
| `WPT=0` | Disabilita il calcolo WPT distribuito; non avvia la chiusura FSM. |
| `WPT?` | Snapshot integrazione WPT e parametri correnti. |
| `WPTSNAP?` | Snapshot coerente delle grandezze principali WPT. |
| `WPTSRC=<mV>` | Riferimento VDC SOURCE WPT. Disabilita WPT prima dell'aggiornamento. |
| `WPTLOAD=<mV>` | Riferimento VDC LOAD WPT. Disabilita WPT prima dell'aggiornamento. |
| `WPTIMAX=<mA>` | Limite corrente coil WPT. Disabilita WPT prima dell'aggiornamento. |
| `WPTISRC=<mA>` | Sostituisce la corrente coil SOURCE con valore sintetico. |
| `WPTISRC=OFF` | Ripristina corrente coil SOURCE fisica. |
| `WPTILOAD=<mA>` | Sostituisce la corrente coil LOAD con valore sintetico. |
| `WPTILOAD=OFF` | Ripristina corrente coil LOAD fisica. |
| `WPTPLIMINIT=<W>` | Seed potenza SOURCE, pending fino alle guardie fisiche previste. |
| `WPTPLOADINIT=<W>` | Seed potenza LOAD, pending fino alle guardie fisiche previste. |

## Soglie FSM e governor/bootstrap

| Comando | Effetto |
|---|---|
| `SMTHR?` | Legge soglie FSM standard: VBUS, corrente batteria e corrente coil minime. |
| `SMVBUS=<V>` | Soglia VBUS per guardie FSM. |
| `SMIBAT=<mA>` | Soglia IBAT per guardie di chiusura FSM. |
| `SMICOIL=<mA>` | Soglia ICOIL per guardie di chiusura FSM. |
| `SMBOOT?` | Snapshot compatto dei parametri e della trace bootstrap. |
| `SMBOOTMODE=0/1` | Seleziona sorgente bootstrap: `0` DCLINK esterno, `1` BOOST locale. Non avvia da solo la FSM. |
| `SMBOOTSRC=<V>` | Soglia VDC SOURCE per innesco/progressione bootstrap. |
| `SMBH=<V>` | Soglia inferiore di mantenimento SOURCE/bootstrap; `0` disabilita la guardia. |
| `SMBOOTLOAD=<V>` | Soglia VDC LOAD per avanzamento/abilitazione lato LOAD. |
| `SMBA=<V>` | Soglia VDC fisica che arma il BBC dopo il precharge ePWM; non e' un riferimento di regolazione. |
| `SMBOOTDUTY=<mpu>` | Cap duty BBC nel precharge iniziale. |
| `SMBHD=<mpu>` | Cap BBC intermedio dopo `WPTBOOT`, durante handoff HFC. |
| `SMBHH=<mpu>` | Cap BBC finale dell'handoff HFC; parser compatto dedicato per non espandere le stringhe UART. |

## Diagnostica gate statica condizionata da macro

| Comando | Effetto |
|---|---|
| `BG=0` | Forza tutte le entrate gate BBC LOW. |
| `BG=1` | Forza solo gli high-side BBC HIGH e i low-side LOW, con ownership GPIO. |

Questi due comandi esistono **solo** se
`TTPLPFC_BBC_STATIC_GATE_TEST_ENABLE=1`. Nel firmware corrente la macro vale
`0`: il parser non li espone. Non sono quindi parte dell'interfaccia operativa
ordinaria.

## Note operative essenziali

- Le query sono sicure a potenza ferma; durante WPT a rate elevato una query
  UART puo' perturbare il servizio radio.
- `STOP` e' il comando di chiusura della FSM; `UE=0`, `WPTHFC=0` e `WPT=0`
  disabilitano soltanto i rispettivi attuatori/calcoli.
- `SMBOOTMODE=1` piu' una sequenza FSM che raggiunge `PRECHARGE_SOURCE` puo'
  attivare il precharge ePWM automatico del firmware corrente. Non e' attivato
  dal solo comando `SMBOOTMODE=1`.
- I comandi con suffisso `?` descrivono normalmente lo stato locale della
  controlCARD interrogata, non quello remoto.
