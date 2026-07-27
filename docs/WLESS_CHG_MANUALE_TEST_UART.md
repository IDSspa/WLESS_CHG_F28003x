# Manuale operativo per test UART BOOST, BUCK e HFC

## 1. Scopo

Questo documento descrive l'esecuzione manuale, tramite UART, dei principali
test di potenza disponibili nel firmware WLESS_CHG FW1024:

1. BOOST UniPD closed-loop con HFC disabilitato;
2. BUCK UniPD closed-loop con emulatore batteria;
3. HFC separato con DCLINK SOURCE alimentato direttamente;
4. HFC con phase shift manuale;
5. HFC closed-loop distribuito UniPD;
6. catena integrata BOOST closed-loop + HFC.

Il manuale e' destinato a personale di laboratorio che conosce le regole
generali di sicurezza elettrica ma non necessariamente la storia dello
sviluppo firmware.

Il documento non sostituisce lo schema elettrico, le prescrizioni di sicurezza
del laboratorio o una valutazione del rischio specifica del banco.

## 2. Stato di validazione e limitazioni note

Fatti consolidati:

- firmware di riferimento: `FW=1024`;
- il BOOST UniPD closed-loop e' stato validato separatamente fino a:
  - VIN = 60 V;
  - riferimento DCLINK = 90 V;
  - carico DCLINK = 83 ohm / 150 W;
  - potenza sul carico circa 99 W;
- l'HFC e il rectifier passivo sono stati validati separatamente;
- il trasferimento DCLINK-to-DCLINK e' stato validato con BOOST e BUCK spenti;
- la catena BOOST closed-loop + HFC e' stata provata con successo;
- il BUCK e' stato verificato open-loop, ma il closed-loop con emulatore
  batteria non e' ancora validato;
- il rectifier LOAD deve rimanere passivo durante le procedure qui descritte;
- la STATION e' slave e risponde solo se interrogata.

Limitazioni:

- la misura `ITANK_MOD` non e' ancora validata su tutte le combinazioni
  scheda/controlCARD;
- la lettura fisica della corrente tank sul SOURCE puo' essere palesemente
  errata; nei test WPT il SOURCE usa pertanto il valore sintetico previsto;
- il closed-loop HFC e' condizionato dalla qualita' della misura `ITANK_MOD`
  sul LOAD;
- `ITANK_PHS` non e' disponibile ed e' mantenuto sintetico;
- il BOOST attuale non pilota ancora i due switch di ciascun ramo in modalita'
  sincrona complementare come previsto dalla documentazione UniPD;
- dopo un trip il DCLINK puo' rimanere carico. Non e' ancora disponibile una
  procedura firmware validata di scarica attiva;
- il reset del latch non equivale alla scarica del DCLINK;
- la combinazione scheda/controlCARD usata per il BOOST deve fornire una misura
  VIN/VBATT fisica, calibrata e coerente con l'alimentatore;
- il BUCK attuale pilota il solo high-side e usa il ricircolo passivo: non
  implementa ancora la modulazione sincrona complementare prevista da UniPD.

## 3. Regole non derogabili

1. Non programmare o resettare una controlCARD con DCLINK o VIN alimentati.
2. Non cambiare carico o cablaggio con condensatori carichi.
3. Non abilitare BOOST, BUCK o HFC prima di avere impostato e verificato:
   - riferimento;
   - limite di corrente;
   - limite duty/phase shift;
   - soglia di sovratensione;
   - soglia di corrente di ramo;
   - durata del run;
   - condizioni di arresto.
4. Ogni modifica di parametro disabilita l'attuatore interessato: verificare
   sempre lo stato prima del run.
5. Eseguire un solo run alla volta.
6. Arrestare immediatamente il test in presenza di:
   - corrente dell'alimentatore superiore al limite stabilito;
   - tensione superiore alla soglia di arresto;
   - trip o fault firmware;
   - perdita del link nRF;
   - rumore, odore, temperatura o forma d'onda anomali;
   - oscillazione crescente o andamento divergente;
   - valore ADC incompatibile con la misura strumentale.
7. Dopo un trip:
   - disabilitare gli attuatori;
   - spegnere VIN/DCLINK;
   - attendere e misurare la scarica;
   - se necessario applicare il bleeder previsto dal banco;
   - non eseguire un nuovo run finche' il bus non e' rientrato nel valore
     passivo atteso o a 0 V, secondo il setup.

## 4. Asset di riferimento

### 4.1 Schede e comunicazione

Configurazione ordinaria:

| Funzione | Scheda/controlCARD | UART |
|---|---|---|
| SOURCE/VEHICLE | scheda SOURCE con immagine VEHICLE FW1024 | COM25 |
| LOAD/STATION | scheda LOAD con immagine STATION FW1024 | COM26 |

Configurazione UART:

```text
115200 bit/s
8 bit dati
nessuna parita'
1 bit di stop
terminazione comando: CR/LF
```

La numerazione delle porte deve essere verificata in Gestione dispositivi,
oppure tramite il seguente comando (powershell):

> (Get-CimInstance Win32_SerialPort).Name

Non affidarsi alla sola posizione fisica del cavo USB.

Può essere utile in caso di utilizzo di nuove schede controlCARD verificare l'assegnazione del corretto ID a queste ultime:

xdsdfu.exe -e

Nel caso la scheda non abbia un ID assegnato è necessario impostarlo tramite i seguenti comandi:

Scheda VEHICLE:

.\xdsdfu.exe -e
.\xdsdfu.exe -m
.\xdsdfu.exe -e
.\xdsdfu.exe -s VEHI0001 -r
.\xdsdfu.exe -e

Scheda STATION:

.\xdsdfu.exe -e
.\xdsdfu.exe -m
.\xdsdfu.exe -e
.\xdsdfu.exe -s STAT0001 -r
.\xdsdfu.exe -e


### 4.2 Alimentatori disponibili

| Campo | Limite |
|---|---|
| bassa tensione / alta corrente | 0...20 V, 0...20 A |
| alta tensione / corrente moderata | 0...60 V, 0...7 A |
| alimentazione logica | 12 V, limite iniziale 1 A |

Il normale assorbimento della logica di una scheda e' circa 0,2 A. Un valore
stabile vicino a 0,17 A dopo il power-cycle puo' indicare un avvio incompleto.
In questo caso scollegare i probe USB delle schede controlCARD, effettuare
un powercycle, ricollegare i probe USB.

### 4.3 Carichi disponibili

| Carico | Potenza nominale | Uso |
|---|---:|---|
| 1,2 kohm | 1 W | bleeder/smoke test; non superare 20 V |
| 83 ohm | 150 W | BOOST e trasferimento a potenza moderata |
| 2,2 ohm | 250 W | HFC ad alta corrente sul LOAD |
| 4,4 ohm | 500 W nominali, due 2,2 ohm in serie | variante LOAD |
| 1,1 ohm | limitato dalla potenza delle singole resistenze | due 2,2 ohm in parallelo; usare solo dopo calcolo termico |

La potenza resistiva deve essere calcolata con:

```text
P = VLOAD^2 / R
```

Il limite della resistenza e il limite dell'alimentatore devono essere
rispettati contemporaneamente.

### 4.4 Strumentazione minima

- multimetro su DCLINK SOURCE;
- multimetro su VLOAD/DCLINK LOAD;
- lettura corrente dall'alimentatore;
- oscilloscopio su VLOAD;
- misura temperatura carico;
- per la caratterizzazione HFC:
  - CH1 su un terminale bobina SOURCE;
  - CH2 sull'altro terminale;
  - masse CH1 e CH2 a DCLINK SOURCE negativo;
  - MATH = CH1 - CH2.

Usare esclusivamente collegamenti e sonde compatibili con la tensione
common-mode e con la categoria del banco. Le masse a coccodrillo dello scope
non devono essere collegate a nodi flottanti o di commutazione.

## 5. Power-cycle corretto

Quando e' richiesto un power-cycle completo:

1. DCLINK e VIN OFF.
2. Verificare DCLINK e VLOAD a 0 V.
3. Scollegare entrambi i cavi USB.
4. Spegnere le alimentazioni logiche 12 V.
5. Attendere la completa diseccitazione.
6. Riaccendere prima le alimentazioni logiche.
7. Verificare un assorbimento di circa 0,2 A per scheda.
8. Collegare successivamente i cavi USB.

Collegare USB prima della logica puo' lasciare la controlCARD parzialmente
alimentata e il firmware bloccato.

## 6. Controlli UART preliminari

Eseguire su entrambe le porte:

```text
FW?
VARS?
RADIO?
```

Risultati minimi attesi:

```text
FW=1024
SOURCE: Role=SOURCE, State=SOURCE_ON oppure stato previsto dal test
LOAD:   Role=LOAD, State=LOAD_ON oppure stato previsto dal test
RadioLink=OK
RADIO EN=1, INIT=1
```

Prima di toccare il banco, portare tutti gli attuatori nello stato sicuro.

Sul SOURCE:

```text
UE=0
UB=0
HFC=0
WPTHFC=0
WPT=0
UQ?
HFC?
WPT?
```

Sul LOAD:

```text
UE=0
UB=0
HFC=0
WPTHFC=0
WPT=0
UQ?
HFC?
WPT?
```

Verificare che gli enable di potenza risultino nulli e che non siano presenti
fault attivi.

## 7. Riferimento rapido dei comandi

### 7.1 Diagnostica generale

| Comando | Funzione |
|---|---|
| `FW?` | release firmware |
| `VARS?` | ruolo, stato FSM, link e variabili principali |
| `RADIO?` | stato nRF24 e contatori |
| `UQ?` | configurazione e diagnostica BOOST/BUCK UniPD |
| `HFC?` | diagnostica ponte HFC |
| `WPT?` | diagnostica integrazione WPT |
| `WPTSNAP?` | snapshot WPT |

### 7.2 BOOST/BUCK

| Comando | Unita' | Funzione |
|---|---:|---|
| `UH=1` | - | selezione BOOST con ingressi fisici necessari e segnali non disponibili sintetici |
| `UH=2` | - | selezione BUCK; non usare nelle procedure BOOST |
| `UV=<mV>` | mV | riferimento DCLINK con risoluzione millivolt |
| `UR=<V>` | V | riferimento DCLINK intero, alternativa a `UV` |
| `UA=<mA>` | mA | limite corrente batteria UniPD |
| `UD=<mpu>` | 0,001 pu | limite massimo duty, massimo accettato 950 |
| `US=<micro-pu>` | 0,000001 pu/ciclo | passo della rampa duty, campo 1...2000 |
| `UX=<mV>` | mV | soglia trip DCLINK, campo 2...60 V |
| `UY=<V>` | V | soglia trip DCLINK, campo 2...300 V |
| `UC=<mA>` | mA | soglia trip corrente di ramo, campo 100...10000 mA |
| `UN=<cicli>` | cicli | conferma trip corrente, campo 1...1000 |
| `UF=1` | - | reset latch, solo dopo rimozione della causa |
| `UF=2` | - | calibrazione offset correnti rami e reset latch |
| `UP?` | - | legge la maschera di correzione polarita' correnti |
| `UP=0` | - | nessuna inversione delle correnti fisiche |
| `UP=1` | - | inverte `IL_A` |
| `UP=2` | - | inverte `IL_B` |
| `UP=3` | - | inverte entrambe le correnti |
| `UE=1` | - | abilita l'attuatore BOOST/BUCK closed-loop |
| `UE=0` | - | disabilita immediatamente l'attuatore |
| `UB=0` | - | disabilita la modalita' open-loop |

Nota: l'impostazione di `UV`, `UR`, `UA`, `UD`, `US`, `UX`, `UY`, `UC`,
`UN`, `UF` o `UH` disabilita l'uscita. `UP` e' accettato soltanto con BBC e
docking test disabilitati. Impostare tutti i parametri prima di `UE=1`.

### 7.3 HFC diretto e WPT

| Comando | Funzione |
|---|---|
| `HFC=1` | abilita direttamente il ponte HFC SOURCE |
| `HFC=0` | disabilita direttamente il ponte HFC |
| `WPT=1` | abilita il solo calcolo distribuito UniPD WPT; non scrive PWM |
| `WPT=0` | disabilita il calcolo WPT |
| `WPTHFC=1` | abilita l'attuatore HFC controllato da WPT |
| `WPTHFC=0` | disabilita l'attuatore HFC WPT |
| `WPTHFCPH=<0...500>` | imposta phase shift fisico manuale e disabilita l'attuatore |
| `WPTHFCPHAUTO` | torna alla fase automatica e disabilita l'attuatore |
| `WPTHFCLIM=<1...500>` | limite phase shift automatico |
| `WPTHFCRAMP=<1...100>` | passo rampa HFC in micro-pu/ciclo |
| `WPTISRC=<mA>` | corrente coil SOURCE sintetica |
| `WPTISRC=OFF` | ripristina la corrente SOURCE fisica; usare solo se validata |
| `WPTILOAD=<mA>` | corrente coil LOAD sintetica |
| `WPTILOAD=OFF` | usa corrente LOAD fisica |
| `WPTPLIMINIT=<W>` | seed limite potenza SOURCE |
| `WPTPLOADINIT=<W>` | seed potenza LOAD |
| `WPTIZERO=<mA>` | offset corrente coil LOAD |
| `WPTIMAX=<mA>` | limite massimo corrente WPT |

`WPTHFC=1` viene inibito se:

- `WPT=1` non e' attivo;
- il ruolo locale non e' SOURCE;
- il ruolo remoto non e' LOAD;
- il link radio non e' OK;
- la corrente SOURCE sintetica non e' abilitata.

## 8. Procedura A - BOOST UniPD closed-loop separato

### 8.1 Obiettivo

Validare il controllo BOOST senza sovrapporre HFC o trasferimento wireless.

### 8.2 Collegamenti

1. Bobine non coinvolte; HFC OFF.
2. Alimentatore collegato a VIN/VBATT del convertitore SOURCE.
3. Carico collegato al DCLINK SOURCE:
   - 83 ohm per i test a potenza significativa;
   - 1,2 kohm soltanto per smoke test sotto 20 V.
4. Multimetro e oscilloscopio ai capi del carico DCLINK.
5. Usare una combinazione scheda/controlCARD con misura VIN/VBATT validata.

### 8.3 Controlli a potenza OFF

Con VIN OFF e DCLINK scarico:

```text
FW?
UE=0
UB=0
HFC=0
WPTHFC=0
WPT=0
UH=1
UF=2
UQ?
```

La calibrazione `UF=2` deve avvenire senza corrente nei rami.

Accendere VIN mantenendo `UE=0`, quindi interrogare:

```text
UQ?
```

PASS preliminare:

- VIN UART coerente con l'alimentatore;
- DCLINK UART coerente con il multimetro;
- IL_A e IL_B prossime allo zero o al valore passivo atteso;
- latch VDC e IL non attivi.

Se VIN UART non e' coerente, non proseguire.

### 8.4 Parametri

Impostare nell'ordine:

```text
UH=1
UV=<riferimento_DCLINK_mV>
UA=<limite_corrente_mA>
UD=<duty_massimo_mpu>
US=<passo_rampa_micro_pu>
UX=<trip_DCLINK_mV>
UC=<trip_corrente_ramo_mA>
UN=<cicli_conferma>
UF=1
UQ?
```

Valori di partenza raccomandati per la ripetizione di un punto significativo:

```text
VIN alimentatore       20 V
limite alimentatore    2 A iniziali
carico                 83 ohm / 150 W
UV                     30000
UA                     1500
UD                     500
US                     500
UX                     42000
UC                     2500
UN                     valore gia' validato nel profilo di test
durata                 5 s, poi 10 s se stabile
```

Questo punto ha prodotto in precedenza VMAX circa 36,4 V e corrente ingresso
circa 0,63 A a regime nel test BOOST separato. I valori attesi dipendono dalla
combinazione scheda/controlCARD e dalla calibrazione.

### 8.5 Avvio e arresto

1. Armare lo scope.
2. Inviare:

```text
UE=1
```

3. Durante il plateau interrogare soltanto:

```text
UQ?
```

4. Per terminare il test inviare:

```text
UE=0
```

Non usare comandi di configurazione durante il run.

### 8.6 Grandezze da registrare

- VIN impostata e VIN UART;
- corrente massima e a regime dell'alimentatore;
- DCLINK massimo, minimo e valore di regime;
- overshoot, undershoot, settling time e ripple;
- IL_A e IL_B;
- duty richiesto/applicato;
- stato clamp corrente e duty;
- latch VDC/IL;
- temperatura carico;
- durata reale.

### 8.7 Punti BOOST gia' validati con carico 83 ohm

| VIN | Vref | UA | Trip rami | Risultato sintetico |
|---:|---:|---:|---:|---|
| 9 V | 12 V | 0,6 A | profilo test | stabile, VMAX circa 14,55 V |
| 12 V | 16 V | 0,8 A | profilo test | stabile, VMAX circa 20,34 V |
| 15 V | 22 V | 1,2 A | profilo test | stabile, VMAX circa 26,4 V |
| 20 V | 30 V | 1,5 A | 2,5 A | stabile, VMAX circa 36,4 V |
| 30 V | 45 V | 3 A | 4,5 A | stabile, VMAX circa 53,2 V |
| 45 V | 68 V | 4 A | 6 A | stabile, VMAX circa 76,9 V |
| 60 V | 90 V | 4 A | 6 A | stabile, VMAX circa 97 V, circa 99 W sul carico |

Non saltare direttamente al punto 60 V. Eseguire almeno un punto intermedio
per confermare cablaggio, calibrazione e segno delle misure.

## 9. Procedura B - BUCK UniPD closed-loop con emulatore batteria

### 9.1 Obiettivo e stato

Validare progressivamente:

1. acquisizione fisica di VDC, VBATT, IL_A e IL_B;
2. convenzione di segno delle correnti nel flusso `DCLINK -> VBATT`;
3. monotonia duty/corrente;
4. stabilita' del closed-loop UniPD con nodo VBATT rigido;
5. limiti, trip e arresto.

Questa procedura e' una campagna di prima validazione. Non esiste ancora una
baseline BUCK closed-loop con emulatore dichiarata PASS.

### 9.2 Collegamenti

```text
alimentatore DCLINK (+)
  -> resistenza serie 4,6...4,8 ohm, se prevista dal punto
  -> DCLINK+ scheda

alimentatore DCLINK (-)
  -> DCLINK- scheda

VBATT+/- scheda
  -> emulatore batteria in modalita' sink/CV
```

Requisiti:

- HFC e bobine non coinvolti;
- nessun carico da 1,2 kohm, 83 ohm o 2,2 ohm su VBATT;
- nessun carico DCLINK non esplicitamente previsto;
- emulatore capace di assorbire corrente alla tensione impostata;
- polarita' verificata con strumenti prima del collegamento;
- limite di corrente e limite di potenza dell'emulatore configurati;
- multimetro su DCLINK scheda, non soltanto sull'uscita dell'alimentatore;
- multimetro su VBATT;
- scope su VBATT per il primo controllo, con eventuali canali aggiuntivi su
  DCLINK o corrente se disponibili.

La resistenza serie non simula la batteria: riduce la rigidita' della sorgente
DCLINK e rende osservabile l'azione dell'anello esterno. Con alimentatore
diretto e perfettamente rigido il BUCK non puo' modificare la tensione imposta
dal generatore.

### 9.3 Condizioni iniziali e limiti

Prima del run devono essere dichiarati:

- tensione CV dell'emulatore;
- corrente massima assorbibile dall'emulatore;
- potenza massima assorbibile;
- tensione e corrente massime dell'alimentatore DCLINK;
- riferimento DCLINK `UV`;
- limite UniPD `UA`;
- limite duty `UD`;
- trip DCLINK `UX/UY`;
- trip corrente rami `UC` e conferma `UN`;
- durata;
- temperatura massima ammessa;
- azione manuale di arresto.

Condizioni minime:

```text
VDC alimentatore > VBATT emulatore
UV compatibile con la tensione DCLINK realmente ottenibile
UA <= limite sink dell'emulatore
UC > UA/2 per ramo con margine motivato
trip VDC > UV ma inferiore ai limiti hardware del banco
```

`UV` e' il riferimento del DCLINK, non il setpoint VBATT. La tensione VBATT e'
imposta dall'emulatore.

### 9.4 Preparazione a potenza OFF

Con DCLINK OFF, emulatore disabilitato e condensatori scarichi:

```text
FW?
UE=0
UB=0
HFC=0
WPTHFC=0
WPT=0
UH=2
UP?
UF=2
UQ?
```

`UF=2` deve essere eseguito senza corrente. Il valore iniziale raccomandato e'
`UP=0`; non invertire alcun canale senza una misura che dimostri la necessita'.

Abilitare soltanto l'emulatore, mantenendo DCLINK OFF, e verificare:

- VBATT UART coerente con emulatore e multimetro;
- nessun assorbimento o erogazione inattesa;
- DCLINK non si carica per backfeed oltre il valore passivo ammesso;
- IL_A/IL_B raw e corretti prossimi allo zero.

Se compare un percorso di energia inatteso, disabilitare l'emulatore e non
proseguire.

### 9.5 Verifica preliminare delle misure e del segno

Impostare tutti i limiti mantenendo `UE=0`:

```text
UH=2
UP=0
UV=<riferimento_DCLINK_mV>
UA=<limite_corrente_mA>
UD=<duty_massimo_mpu>
US=<passo_rampa_micro_pu>
UX=<trip_DCLINK_mV>
UC=<trip_corrente_ramo_mA>
UN=<cicli_conferma>
UF=1
UQ?
```

Accendere DCLINK con limite di corrente basso e controllare `UQ?`.

Prima del closed-loop:

- VDC UART deve seguire il multimetro sul DCLINK scheda;
- VBATT UART deve seguire l'emulatore;
- i valori raw di IL_A/IL_B devono essere plausibili;
- nessun latch deve essere attivo.

Nel flusso BUCK UniPD si attende:

```text
I_bat_rif_min < 0
I_bat_rif_max = 0
I_L_A_rif < 0
I_L_B_rif < 0
```

Se la corrente esterna dimostra trasferimento verso l'emulatore ma uno o
entrambi i feedback hanno segno opposto, arrestare con `UE=0`, spegnere DCLINK,
attendere la scarica e usare `UP=1`, `UP=2` o `UP=3` solo sul canale dimostrato
invertito. Ripetere quindi la verifica da fermo. Non cambiare `UP` durante il
run.

### 9.6 Primo run closed-loop

Il primo punto deve usare tensione, corrente e duty conservativi, definiti in
base ai limiti reali dell'emulatore. Non riutilizzare automaticamente i valori
BOOST.

Sequenza:

1. emulatore ON e stabile in sink/CV;
2. alimentatore DCLINK ON;
3. verificare VDC, VBATT e correnti con `UQ?`;
4. armare scope e capture:

```text
CAP=1
```

5. abilitare:

```text
UE=1
```

6. osservare corrente DCLINK, corrente sink dell'emulatore, VDC e VBATT;
7. arrestare al termine previsto:

```text
UE=0
CAP=0
```

8. acquisire:

```text
UQ?
CAPD?
```

9. spegnere DCLINK, verificare la scarica e solo successivamente disabilitare
   o scollegare l'emulatore secondo la procedura del costruttore.

### 9.7 Grandezze da registrare

- tensione DCLINK all'alimentatore e sulla scheda;
- caduta sulla resistenza serie;
- VBATT impostata, misurata e acquisita;
- corrente DCLINK massima e a regime;
- corrente sink dell'emulatore massima e a regime;
- `IL_A/IL_B` raw e corretti;
- `I_L_A_rif/I_L_B_rif` ed errori;
- duty raw, mapped, applied e ramped dei due rami;
- overshoot, undershoot, settling e ripple;
- latch e valori first-fault;
- temperature;
- durata.

### 9.8 Criteri PASS/FAIL specifici

PASS preliminare del punto:

- tutte le tensioni fisiche e UART sono coerenti;
- la corrente fluisce da DCLINK verso l'emulatore;
- segno e ordine di grandezza di entrambi i rami sono coerenti;
- aumento della richiesta produce aumento coerente della corrente;
- nessun trip, saturazione persistente o oscillazione crescente;
- il DCLINK converge o resta limitato intorno al riferimento compatibilmente
  con la rigidita' della sorgente;
- VBATT resta entro la regolazione dell'emulatore;
- duty e correnti dei due rami non divergono.

FAIL o run non validante:

- polarita' corrente non dimostrata;
- emulatore non in modalita' sink o in limitazione non prevista;
- alimentatore in current limit non dichiarato;
- DCLINK imposto rigidamente e incapace di reagire al controllo;
- ciclo limite persistente;
- duty a clamp senza raggiungere una condizione interpretabile;
- backfeed con PWM disabilitato;
- trip, sovratensione, sovracorrente o temperatura fuori limite.

Un singolo punto stabile valida soltanto quel punto operativo e il pilotaggio
BUCK non sincrono attuale. Non valida ancora la topologia complementare UniPD.

## 10. Procedura C - HFC separato con alimentazione diretta del DCLINK

### 10.1 Obiettivo

Validare ponte HFC, bobine e rectifier passivo senza BOOST e BUCK.

### 10.2 Collegamenti

```text
Alimentatore -> DCLINK SOURCE
SOURCE -> bobina (SOURCE)
LOAD -> bobina (LOAD)
DCLINK LOAD -> carico resistivo
```

- VIN/VBATT di entrambe le schede scollegate;
- BOOST e BUCK disabilitati;
- carico consigliato:
  - 83 ohm per la baseline da circa 10 W;
  - 2,2 ohm per la caratterizzazione ad alta corrente.

### 10.3 Setup scope per la forma differenziale HFC

Per misure a 85 kHz:

```text
modo                 RUN durante la regolazione dello scope
base tempi           2...5 us/div iniziali
CH1, CH2             scala adeguata, iniziare larga
MATH                 CH1 - CH2
misure               frequenza, Vpp MATH, fase CH1-CH2
```

Per il transitorio VLOAD:

```text
modo                 SINGLE
base tempi           1 s/div per run da 10 s
trigger              circa 10...20% sopra il valore passivo
scala verticale      tale da includere almeno 1,5 volte il valore atteso
```

### 10.4 Sequenza

Con DCLINK OFF:

```text
UE=0
UB=0
WPTHFC=0
WPT=0
HFC=0
HFC?
```

Accendere DCLINK, verificare corrente prossima a zero e VLOAD circa 0 V.

Armare lo scope e dichiarare la durata. Inviare:

```text
HFC=1
```

Durante il plateau:

```text
HFC?
VARS?
```

Al termine:

```text
HFC=0
```

### 10.5 Baseline validata con carico 83 ohm

| DCLINK SOURCE | Limite PSU | IDCLINK | VLOAD max indicativo | Esito |
|---:|---:|---:|---:|---|
| 12 V | 1 A | circa 0,85 A | circa 20,6 V | PASS |
| 15 V | 1,5 A | circa 1,11 A | circa 27,6 V | PASS |
| 16 V | 1,5 A | max 1,38 A, regime 1,2 A | circa 29,9 V | PASS |

La forma validata presenta:

- salita circa 40 ms;
- plateau stabile;
- discesa asintotica circa 500 ms;
- nessuna oscillazione evidente.

Il punto a 16 V corrisponde a circa 10 W sul carico.

## 11. Procedura D - Caratterizzazione HFC con phase shift manuale

### 11.1 Obiettivo

Verificare il segno e l'autorita' della relazione:

```text
phase shift -> ponte HFC -> bobine -> rectifier -> VLOAD/IDCLINK
```

Il controllo UniPD calcola in shadow, ma non determina il phase shift fisico.

### 11.2 Setup validato

```text
DCLINK SOURCE       20 V / limite 10 A
carico LOAD         2,2 ohm / 250 W
BOOST/BUCK          OFF
durata              10 s per punto
```

La convenzione verificata e':

```text
500 mpu = punto neutro
valori decrescenti = eccitazione crescente
0 mpu = massimo comando del campo manuale
```

### 11.3 Preparazione distribuita

SOURCE:

```text
WPT=0
WPTHFC=0
WPTISRC=0
WPTPLIMINIT=120
WPT=1
```

Attendere circa 1 s.

LOAD:

```text
WPT=0
WPTILOAD=OFF
WPTPLOADINIT=60
WPT=1
```

Attendere circa 1 s e verificare su entrambe:

```text
WPT?
RADIO?
```

SOURCE deve vedere ruolo remoto LOAD e link OK.

### 11.4 Esecuzione di un punto

Sul SOURCE:

```text
WPTHFC=0
WPTHFCPH=<valore_mpu>
WPT?
```

Armare lo scope. Inviare:

```text
WPTHFC=1
```

Dopo la durata prevista:

```text
WPTHFC=0
```

### 11.5 Valori di confronto validati

| WPTHFCPH | VLOAD | IDCLINK |
|---:|---:|---:|
| 500 | 0 V | 0,01 A |
| 450 | 0,06 V | 0,02 A |
| 400 | 0,90 V | 0,22 A |
| 375 | 1,30 V | 0,35 A |
| 325 | 2,10 V | 0,60 A |
| 250 | 3,12 V | 0,98 A |
| 125 | 4,30 V | 1,50 A |
| 0 | 4,80 V | 1,73 A |

PASS:

- VLOAD e IDCLINK crescono riducendo `WPTHFCPH`;
- nessun fault;
- `HPHY` coincide con il valore manuale;
- fase automatica e phase shift fisico restano distinguibili.

## 12. Procedura E - HFC closed-loop distribuito UniPD

### 12.1 Avvertenza

Questa procedura richiede:

- misura `ITANK_MOD` LOAD validata oppure override sintetico dichiarato;
- SOURCE con corrente coil sintetica;
- link nRF stabile;
- ruoli FSM SOURCE/LOAD corretti.

Un run con `ITANK_MOD` sintetico verifica software, segno e sequenza, ma non
valida la regolazione sulla corrente fisica.

### 12.2 Preparazione

SOURCE:

```text
WPTHFC=0
WPT=0
WPTISRC=0
WPTPLIMINIT=<limite_potenza_W>
WPTHFCLIM=<limite_phase_mpu>
WPTHFCRAMP=<passo_rampa>
WPTHFCPHAUTO
WPT=1
```

LOAD con misura fisica:

```text
WPT=0
WPTILOAD=OFF
WPTIZERO=<offset_mA_validato>
WPTIMAX=<corrente_massima_mA>
WPTPLOADINIT=<potenza_iniziale_W>
WPT=1
```

LOAD con misura sintetica, solo per test software:

```text
WPT=0
WPTILOAD=<corrente_sintetica_mA>
WPTIMAX=<corrente_massima_mA>
WPTPLOADINIT=<potenza_iniziale_W>
WPT=1
```

Verificare:

```text
WPT?
RADIO?
```

### 12.3 Avvio

Armare lo scope e inviare sul SOURCE:

```text
WPTHFC=1
```

Durante il run usare soltanto query:

```text
WPT?
WPTSNAP?
HFC?
RADIO?
```

Arresto:

```text
WPTHFC=0
```

PASS minimo:

- `HREQ`, `HAPP` e comando fisico evolvono con segno coerente;
- VLOAD converge verso il riferimento previsto dal profilo;
- link sempre OK;
- nessun fault HFC;
- nessuna saturazione permanente non prevista;
- corrente fisica usata coerente con la misura strumentale.

Se la corrente LOAD e' sintetica, l'ultimo criterio non e' applicabile e il
test deve essere classificato come test software, non validazione energetica.

## 13. Procedura F - BOOST closed-loop + HFC integrati

### 13.1 Obiettivo

Validare la catena:

```text
VIN -> BOOST closed-loop -> DCLINK SOURCE -> HFC
    -> bobine -> rectifier passivo -> DCLINK LOAD -> carico
```

Il BUCK deve restare spento.

### 13.2 Collegamenti

SOURCE:

- alimentatore su VIN/VBATT;
- nessun carico resistivo locale sul DCLINK, salvo bleeder previsto;
- BOOST UniPD closed-loop;
- HFC SOURCE attivo.

LOAD:

- bobina collegata;
- rectifier passivo;
- BUCK OFF;
- carico 83 ohm sul DCLINK LOAD.

Strumentazione:

- VIN e corrente ingresso dall'alimentatore;
- multimetro DCLINK SOURCE;
- multimetro e scope VLOAD;
- temperatura carico.

### 13.3 Punto iniziale validato

```text
VIN                    12 V
limite alimentatore    almeno 2 A, coerente col banco
riferimento BOOST      16 V
carico LOAD            83 ohm
HFC                    85 kHz
durata                 5...10 s
```

Valori osservati in precedenza:

```text
VIN UART               11,64...11,84 V
corrente ingresso      2,0...2,2 A
DCLINK SOURCE UART     15,41...16,12 V
VLOAD                  circa 27 V
VMAX scope             circa 30 V
potenza LOAD           circa 8,8 W
IL_A                   1,03...1,14 A
IL_B                   1,07...1,23 A
duty applicato         circa 0,386...0,429
trip                   nessuno
```

### 13.4 Configurazione

Con VIN OFF e bus scarichi, sul SOURCE:

```text
UE=0
UB=0
HFC=0
WPTHFC=0
WPT=0
UH=1
UV=16000
UA=<limite_validato_mA>
UD=500
US=500
UX=<trip_DCLINK_mV>
UC=<trip_corrente_ramo_mA>
UN=<cicli_conferma>
UF=2
UQ?
```

Sul LOAD:

```text
UE=0
UB=0
HFC=0
WPTHFC=0
WPT=0
VARS?
```

Accendere VIN con tutti gli attuatori OFF. Verificare VIN, DCLINK passivo,
correnti di ramo e assenza latch.

### 13.5 Sequenza di avvio

Per il punto 12 V -> 16 V e' stata validata:

```text
HFC=1
attesa 1 s
UE=1
```

Per VIN = 20 V questa attesa di 1 s ha prodotto uno sbilanciamento dei rami e
trip. A 20 V usare esclusivamente un avvio quasi simultaneo:

```text
HFC=1
UE=1
```

I due comandi devono essere inviati consecutivamente, senza attesa deliberata.
Se si usano due operatori o due terminali, concordare un unico comando di
avvio.

### 13.6 Sequenza di arresto

Arresto normale:

```text
UE=0
attesa 250 ms
HFC=0
```

La disabilitazione anticipata del BOOST consente normalmente al DCLINK SOURCE
di tornare al valore passivo prima dell'arresto HFC.

Arresto di emergenza:

```text
UE=0
HFC=0
```

Poi spegnere immediatamente VIN e verificare la scarica.

### 13.7 Punto integrato 20 V

Configurazione provata:

```text
VIN                    20 V
Vref BOOST             30 V
UD                     500
US                     500
carico LOAD            83 ohm
avvio                   HFC=1 / UE=1 quasi simultanei
```

Gradini `UA` osservati:

| UA | Esito | VLOAD/VMAX indicativo |
|---:|---|---:|
| 1,8 A | stabile, clamp potenza | VMAX circa 38 V |
| 2,2 A | stabile | VMAX circa 42,4 V |
| 2,4 A | stabile | VMAX circa 44,8 V |
| 2,6 A | PASS solo strumentale | VMAX circa 47,3 V |
| 2,8 A | stabile | VMAX circa 48,5 V |
| 2,9 A | stabile per 10 s | plateau circa 46 V, VMAX circa 50,1 V |
| 3,0 A | FAIL ripetibile | trip DCLINK a circa 42,02 V |

Non usare `UA=3,0 A` o superiore con questa configurazione. Il confine
sperimentale e' compreso fra 2,9 A e 3,0 A e non costituisce un margine
operativo di produzione.

Per l'addestramento del personale partire da `UA=1,8 A`.

## 14. Capture diagnostici

### 14.1 Capture BBC BOOST/BUCK

```text
CAP=1       arma
CAP=0       congela
CAP?        stato
CAPD?       dump, solo a buffer congelato
```

Non armare se e' gia' presente un fault latched.

### 14.2 Capture WPT sincronizzato

```text
WPTCAPDEC=<decimazione>
WPTCAP=1
WPTCAP=0
WPTCAP?
WPTCAPD?
```

Il buffer contiene 96 campioni. `WPTCAPD?` deve essere eseguito soltanto dopo
`WPTCAP=0`.

Campi principali:

- `VDC_mV`: DCLINK locale;
- `IPHY_mA`: corrente fisica letta;
- `IUSE_mA`: corrente effettivamente usata dal controllo;
- `PREF_W`, `IREF_cA`, `IERR_cA`: riferimenti ed errore;
- `HREQ_mpu`: richiesta UniPD;
- `HAPP_mpu`: fase automatica applicata dal modello;
- `HAUTO_mpu`: comando hardware automatico equivalente;
- `HPHY_mpu`: comando fisico effettivo;
- `MAN`: phase shift manuale;
- `FAULT`: fault attuatore HFC.

Per un run di 5...10 s, una decimazione `213` conserva circa l'ultima finestra
operativa utile senza generare traffico UART durante il controllo.

## 15. Criteri PASS/FAIL comuni

### PASS

- configurazione UART accettata e riletta;
- misure UART coerenti con gli strumenti;
- segno delle grandezze coerente;
- nessun trip o fault;
- nessuna perdita radio nei test distribuiti;
- tensione e corrente restano entro i limiti dichiarati;
- transitorio smorzato o plateau stabile;
- ripetibilita' su almeno un secondo run quando richiesta dal protocollo.

### FAIL

- comando ignorato o risposta UART assente;
- ruolo, stato o link non coerente;
- corrente o tensione fisica non coerente con l'ADC;
- saturazione inattesa e persistente;
- oscillazione crescente;
- trip, sovratensione o sovracorrente;
- DCLINK che resta carico senza un percorso di scarica controllato;
- corrente dei rami fortemente sbilanciata senza spiegazione;
- raggiungimento apparente del riferimento ottenuto usando dati sintetici non
  dichiarati.

## 16. Scheda di registrazione

```text
Data/ora:
Operatore:
Firmware SOURCE:
Firmware LOAD:
Scheda/controlCARD SOURCE:
Scheda/controlCARD LOAD:
Tipo test: BOOST / BUCK / HFC diretto / HFC manuale / HFC CL / BOOST+HFC

Cablaggio:
Alimentatore e limiti:
Carico e temperatura iniziale:
VIN/DCLINK SOURCE:
Riferimento:
UA:
UD:
US:
Soglia VDC:
Soglia corrente rami:
Phase shift / limite / rampa HFC:
Seed WPT:
Misura ITANK_MOD: fisica / sintetica
Durata dichiarata:

VIN misurata:
IDCLINK max/regime:
DCLINK SOURCE max/regime:
VLOAD max/regime:
IL_A:
IL_B:
Duty/phase richiesto:
Duty/phase applicato:
Overshoot:
Undershoot:
Settling time:
Ripple:
Temperatura finale:
Stato radio:
Trip/fault:

Esito: PASS / FAIL / NON VALIDANTE
Note:
```

## 17. Chiusura del banco

1. `UE=0` su entrambe le schede.
2. `WPTHFC=0` sul SOURCE.
3. `HFC=0` su entrambe le schede.
4. `WPT=0` su entrambe le schede.
5. VIN e DCLINK OFF.
6. Verificare DCLINK SOURCE, DCLINK LOAD e VLOAD a 0 V.
7. Attendere il raffreddamento dei carichi.
8. Spegnere le logiche.
9. Scollegare USB.

Il banco non e' considerato chiuso finche' le tensioni residue non sono state
misurate.
