# Handoff corrente — BOOST sincrono

## 1. Obiettivo immediato

Validare il **pilotaggio sincrono del BOOST** in un test open-loop separato;
solo dopo affrontare il BUCK sincrono e l'integrazione nel governor/bootstrap.

Non usare questo test per modificare FSM, WPT, HFC o regolazione UniPD: il
primo obiettivo e' verificare l'handoff elettrico protetto fra precharge
statico e PWM complementare.

## 2. Worktree e codice di prova

Il test e' isolato nel worktree:

```text
C:\Users\m.santucci\.codex\worktrees\boost-sync-openloop\WLESS_CHG_F28003x
branch: codex/boost-sync-openloop
```

Il firmware principale rimane nel workspace:

```text
C:\Users\m.santucci\workspace_ccstheia\WLESS_CHG_F28003x
```

Il codice sperimentale del worktree usa
`TTPLPFC_BBC_SYNC_DOCK_TEST_ENABLE=1`. La macro globale di topologia
`TTPLPFC_BBC_COMPLEMENTARY_PWM_ENABLE` resta a `0`: il PWM complementare e'
abilitato soltanto nel percorso di handoff sperimentale, non nel normale
controllo BBC di produzione.

File centrali del test:

- `wless_uart/wless_uart.c`: interpreta `UB=3` e l'handoff `UB=1`;
- `ttplpfc/ttplpfc.h`: `TTPLPFC_BBC_runDockingTest()`;
- `ttplpfc/ttplpfc_hal.h`: pattern statico, PWM complementare e dead-band;
- `ttplpfc/ttplpfc_user_settings.h`: macro del test e mapping BBC.

## 3. Stato elettrico verificato

Configurazione dell'ultima verifica di precharge:

```text
VIN SOURCE: PSU 12 V, limite 2 A, inizialmente OFF
DCLINK SOURCE: carico elettronico CR 8 ohm ON
DCLINK LOAD: aperto
VIN LOAD: scollegato
```

Con `UB=3`, VIN ON e nessun PWM commutato sono stati osservati:

```text
VDCLINK SOURCE circa 11,9 V
PSU circa 12 V / 1,4...1,5 A
```

Questo e' il punto iniziale scelto per l'handoff sincrono. E' migliore del
vecchio comportamento operativo osservato storicamente (~11,3...11,5 V): non
deve dipendere dalla ricostruzione della sequenza UART storica.

Il confronto noto con CR 8 ohm e VIN 12 V e':

| Stato gate BBC | Risultato osservato |
|---|---|
| gate BBC tutti LOW / OST | circa 7 V, 0,8...0,9 A |
| high-side ON, low-side OFF | circa 11,9 V, 1,4...1,5 A |

## 4. Contratto del test UART sperimentale

Il percorso e' intenzionalmente fuori da FSM, WPT e HFC.

```text
UB=3
```

porta EPWM6/EPWM7 nel pattern statico:

```text
EPWMxA / high-side = HIGH
EPWMxB / low-side  = LOW
dead-band disabilitato
nessuna commutazione PWM
Trip-Zone ancora proprietario della protezione
```

Dopo avere osservato il punto di precharge, il comando:

```text
UB=1
```

esegue l'handoff:

1. forza prima OST su entrambi i leg;
2. sostituisce il pattern statico con AQ + dead-band complementare;
3. interpreta il duty BOOST noto come `Dlow`;
4. applica al compare high-side `Dhigh = 1 - Dlow`;
5. avvia la rampa da `Dlow=0` verso il limite `UD` con passo `US`.

Il risultato desiderato e' quindi continuita' tra high-side statico del
precharge e avvio PWM complementare con low-side inizialmente quasi nullo.
L'handoff **non e' ancora validato a banco**.

## 5. Ultimo ostacolo noto e configurazione del prossimo run

Il primo tentativo `UB=3 -> UB=1` non ha validato l'handoff: `UY` era rimasto
a 10 V e il docking test ha correttamente generato un VDC trip appena superata
la soglia. Non e' evidenza contro il mapping complementare.

Prima del prossimo run, a VIN OFF, configurare singolarmente:

```text
UY=50
UF=1
UD=600
US=10
UB=3
```

Poi:

1. richiedere `VIN ON` e osservare il precharge (`~11,9 V` atteso);
2. armare `CAP=1` prima dell'handoff;
3. inviare `UB=1` una sola volta;
4. osservare VDC, corrente PSU e eventuali trip;
5. con potenza ancora attiva inviare `CAP=0`, acquisire `CAPD?`, poi
   `UB=0` e richiedere `VIN OFF`.

Non usare `UE=1`, `WPTHFC=1`, `WPT=1`, `SOURCE`, `INITOK`, `SMBA` o comandi
FSM in questa prova isolata.

## 6. Criteri di esito del prossimo run

### Esito utile

- nessun trip VDC o IL;
- PWM complementare effettivamente attivo dopo `UB=1`;
- VDC evolve in modo compatibile con `UD=600` e CR 8 ohm;
- il capture preserva duty applicato, VDC e latch prima del disarmo.

### Esito non interpretabile

- soglia `UY` inferiore alla tensione richiesta;
- VIN/carico non coerenti con la configurazione dichiarata;
- comando UART inviato mentre il precharge non e' stato prima osservato;
- stop senza congelare il capture.

## 7. Distinzione dal firmware principale

Nel worktree principale esiste una modifica **non committata** di precharge
ePWM automatico, avviata dalla FSM entrando in `PRECHARGE_SOURCE` con
`SMBOOTMODE=1`. Essa non e' il test sincrono `UB=3` e non va mescolata a questa
validazione. Lo stato logico `PRECHARGE_SOURCE` e' storico, ma il pattern
statico ePWM high-side ON / low-side OFF e' recente.

Il documento di progetto per l'eventuale integrazione successiva e':

```text
docs/BOOST_SYNC_TRANSITION_IMPLEMENTATION_REFERENCE.md
```

## 8. Regole per il prossimo contesto

1. Leggere prima `AGENTS.md`, questo documento e
   `docs/BOOST_SYNC_TRANSITION_IMPLEMENTATION_REFERENCE.md`.
2. Non proporre correzioni a bootstrap, PWM o protezioni senza call path,
   assunzioni verificabili, test falsificante e variabile fisica limitata.
3. Non eseguire build/flash senza richiesta esplicita; l'operatore esegue
   normalmente build, flash, power cycle e azioni sugli alimentatori.
4. Un run alla volta; dopo `VIN ON` attendere le letture dell'operatore.
5. Con CR 8 ohm sul DCLINK SOURCE, `VIN OFF` implica la scarica del bus: non
   chiedere una verifica ridondante della scarica.
