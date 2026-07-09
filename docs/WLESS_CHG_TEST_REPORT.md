# Wireless Charger Firmware Test Report

Data inizio verifiche: 2026-06-17

Schede usate:
- TMDSCNCD280039C controlCARD, Rev. B
- TMDSHSECDOCK docking station, Rev. F

Strumenti:
- CCS / debug via XDS110 integrato nella controlCARD
- Oscilloscopio Keysight MSOX2024A
- Alimentatori da banco
- Multimetro

Note generali:
- Le verifiche sono state eseguite su docking station, senza stadio di potenza collegato.
- La controlCARD e' la stessa prevista per l'integrazione sulla scheda TI modificata.
- I segnali EPWM6/EPWM7 sono stati verificati come segnali logici di controllo gate, non come erogazione di potenza.
- Le note architetturali SOURCE/SINK e UniPD sono raccolte in `docs/WLESS_CHG_ARCHITECTURE_NOTES.md`.

## Stato Firmware

Configurazione rilevante verificata:
- `CLLLC_SECONDARY_ENABLED = 0`
- `CLLLC_PWM3_SYNC90_ENABLED = 1`
- frequenza switching CLLLC: 85 kHz
- range switching configurato: 75 kHz - 90 kHz
- `CLLLC_ISR2_FREQUENCY_HZ = 21250 Hz`
- `TTPLPFC_EPWM67_ACTIVE_CONTROL = TTPLPFC_EPWM67_CONTROL_BBC`
- `UNIPD_BBC_ENABLE_POWER_OUTPUTS = 0`

Hook di test docking BBC:
- `TTPLPFC_bbcDockTestEnable`
- `TTPLPFC_bbcDockTestMode`
- `TTPLPFC_bbcDockTestDuty1_pu`
- `TTPLPFC_bbcDockTestDuty2_pu`

Il test hook BBC e' disabilitato di default a runtime (`TTPLPFC_bbcDockTestEnable = 0`).

## Test 1 - Connessione Debug

Obiettivo:
Verificare programmazione e debug della controlCARD tramite USB/XDS110.

Risultato:
CCS rileva un solo target:

```text
Probe ID: 0
Serial number: CC391074
Connection name: TIXDS110_Connection
Board name: F28003x ControlCARD
```

Esito: OK.

## Test 2 - Avvio Firmware e Variabili Base

Obiettivo:
Verificare che il firmware venga programmato, avviato e che le variabili base siano osservabili da CCS.

Valori osservati con firmware in esecuzione:

```text
TTPLPFC_bbcMode: TTPLPFC_BBC_MODE_DISABLED
TTPLPFC_bbcEnabled: 0

CLLLC_iPrimTankModSensedRaw: 108
CLLLC_iPrimTankPhsSensedRaw: 116
CLLLC_iPrimTankModSensed_pu: 0.0263671875
CLLLC_iPrimTankPhsSensed_pu: 0.0283203125
```

Esito: OK.

## Test 3 - PWM Principali CLLLC

Obiettivo:
Verificare presenza, frequenza e relazioni di fase dei PWM principali.

Risultati:
- PWM1 presente
- PWM2 presente
- PWM3 presente
- frequenza misurata: 85 kHz
- PWM3 sfasato di circa -90 gradi rispetto a PWM1
- duty PWM1 misurato: circa 49.1%, compatibile con duty nominale 50%

Note:
- PWM1/PWM2/PWM3 sono rimasti stabili durante le prove prolungate.
- Dopo i test EPWM6/EPWM7, PWM1/PWM2/PWM3 risultano ancora presenti a 85 kHz e con gli sfasamenti attesi.

Esito: OK.

## Test 4 - PWM Secondario Disabilitato

Obiettivo:
Verificare che i PWM non piu' necessari per il secondario originale non interferiscano con la nuova configurazione.

Risultato:
I segnali PWM secondari sono stati rimossi/disabilitati per la configurazione corrente. PWM3 e' stato riutilizzato come PWM sincronizzato e sfasato di 90 gradi rispetto a PWM1.

Esito: OK.

## Test 5 - EPWM5

Obiettivo:
Verificare che EPWM5 non generi piu' il PWM PFC a bassa frequenza e resti usato solo come timebase/trigger interno per ISR2/ADC.

Risultato:
- segnale EPWM5 esterno non richiesto/non presente
- EPWM5 mantenuto come trigger interno
- high-frequency PFC/BBC legs sincronizzati da EPWM6

Esito: OK.

## Test 6 - EPWM6/EPWM7 BBC Logic Enable

Obiettivo:
Verificare su docking station l'attivazione controllata dei soli segnali logici EPWM6/EPWM7, senza hardware di potenza collegato.

Mapping fisico usato:

```text
GPIO10 / EPWM6A -> EC1.61 -> J6.7
GPIO11 / EPWM6B -> EC1.63 -> J6.8
GPIO12 / EPWM7A -> EC1.58 -> J7.5
GPIO13 / EPWM7B -> EC1.60 -> J7.6
```

Condizione iniziale:

```text
TTPLPFC_bbcDockTestEnable = 0
```

Risultato:
EPWM6A/B ed EPWM7A/B assenti.

### Test 6.1 - Comando complementare PFC legacy

```text
TTPLPFC_bbcDockTestMode = TTPLPFC_BBC_MODE_BOOST
TTPLPFC_bbcDockTestDuty1_pu = 0.25
TTPLPFC_bbcDockTestDuty2_pu = 0.25
TTPLPFC_bbcDockTestEnable = 1
```

Misure osservate:

```text
EPWM6A: 120 kHz, duty circa 24%
EPWM6B: 120 kHz, duty circa 74%
EPWM7A: 120 kHz, duty circa 24%, sfasato 180 gradi rispetto a EPWM6A
EPWM7B: 120 kHz, duty circa 74%, sfasato 180 gradi rispetto a EPWM6B
```

Controprova a duty minimo:

```text
TTPLPFC_bbcDockTestDuty1_pu = 0.001
TTPLPFC_bbcDockTestLegMode = TTPLPFC_BBC_DOCK_TEST_LEG1_ONLY
```

Misura osservata:

```text
EPWM6A: sempre basso
EPWM6B: sempre alto
```

Interpretazione:
Il PWM complementare PFC legacy non e' adatto al test BBC BOOST. Con duty molto basso, il ramo B resta alto quasi continuativamente. Dato il mapping attuale, B corrisponde al comando low-side; questo comportamento e' coerente con il collasso di +BATT osservato nei primi test sulla scheda TI modificata.

### Test 6.2 - Comando BBC BOOST dedicato

E' stata introdotta una modalita' di test BOOST dedicata:

```text
EPWMxA / high-side: forzato basso
EPWMxB / low-side: PWM reale con duty impostato
```

Verifiche su docking:

```text
LEG1_ONLY, duty 5%:
EPWM6A = sempre basso
EPWM6B = circa 5%
frequenza = 120 kHz

LEG1_ONLY, duty 25%:
EPWM6A = sempre basso
EPWM6B = circa 25%
frequenza = 120 kHz

LEG2_ONLY, duty 5%:
EPWM7A = sempre basso
EPWM7B = circa 5%
frequenza = 120 kHz

LEG2_ONLY, duty 25%:
EPWM7A = sempre basso
EPWM7B = circa 25%
frequenza = 120 kHz
```

Risultato:
La nuova modalita' genera un comando logico coerente con il test BBC BOOST a bassa tensione: high-side spento e low-side modulato con duty reale.

Esito: OK.

## Test 7 - ISR2 e Frequenza Campionamento

Obiettivo:
Verificare la frequenza ISR2/ADC impostata a 21.25 kHz.

Valori osservati:

```text
OBC_7_4KW_ISR2_nests: 1
OBC_7_4KW_ISR2_nestsAvg: 0.0911437646
OBC_7_4KW_ISR2_nestsMax: 1
CLLLC_ISR2_Loading: 0.0
CLLLC_ISR2_LoadingMax: 0.0

ECAP CAP1: 0x0000160F
ECAP CAP2: 0x00000B07
EPwm5Regs.TBPRD: 0x0B07
EPwm5Regs.CMPB: 0x001E
```

Interpretazione:

```text
CAP1 = 0x160F = 5647
Frequenza stimata = 120 MHz / 5647 = 21250.22 Hz
```

Esito: OK.

## Test 8 - ADC Path I_PRIM_TANK_MOD / I_PRIM_TANK_PHS

Obiettivo:
Verificare che i canali ADC liberati e riutilizzati per `I_PRIM_TANK_MOD` e `I_PRIM_TANK_PHS` aggiornino correttamente le variabili firmware.

Mapping:

```text
I_PRIM_TANK_MOD -> ADCA5  -> controlCARD/HSEC pin 21
I_PRIM_TANK_PHS -> ADCC11 -> controlCARD/HSEC pin 31
```

Misure su ADCA5 / pin 21:

```text
0 V       -> CLLLC_iPrimTankModSensedRaw = 0
~1.650 V  -> CLLLC_iPrimTankModSensedRaw = ~1900
~3.0 V    -> CLLLC_iPrimTankModSensedRaw = ~3470
```

Misure su ADCC11 / pin 31:

```text
0 V       -> CLLLC_iPrimTankPhsSensedRaw = ~0
~1.64 V   -> CLLLC_iPrimTankPhsSensedRaw = ~1900
~3.0 V    -> CLLLC_iPrimTankPhsSensedRaw = ~3450
```

Risultato:
Entrambi i canali aggiornano correttamente le variabili firmware. La scala misurata e' leggermente inferiore al teorico, ma coerente tra i due canali e monotona.

Esito: OK.

## Test 9 - Wrapper UniPD/BBC, Step Non Invasivo

Obiettivo:
Verificare che il wrapper UniPD/BBC venga eseguito, raccolga gli input attualmente mappati e mantenga EPWM6/EPWM7 disabilitati quando gli input richiesti non sono completi.

Valori osservati:

```text
UNIPD_bbcSignalValidMask: 29
UNIPD_bbcSignalMissingMask: 2018

UNIPD_bbcInputs.v_dc: 14.654355
UNIPD_bbcInputs.v_bat: 0.0
UNIPD_bbcInputs.i_l_a: -28.0461369
UNIPD_bbcInputs.i_l_b: -29.1164818
UNIPD_bbcInputs.i_coil_loc: 0.897979736
UNIPD_bbcInputs.i_coil_rem_err: 0.0
UNIPD_bbcInputs.i_coil_loc_rif: 0.0
UNIPD_bbcInputs.v_dc_pbat_rif: 0.0
UNIPD_bbcInputs.i_bat_rif_max: 0.0
UNIPD_bbcInputs.i_bat_rif_min: 0.0
UNIPD_bbcInputs.tx_1_rx_0: 0
UNIPD_bbcInputs.valid_mask: 29

UNIPD_bbcOutput.abilita_pwm: 0
UNIPD_bbcOutput.duty_cycle_pwm_a: 0.0
UNIPD_bbcOutput.duty_cycle_pwm_b: 0.0

TTPLPFC_bbcMode: TTPLPFC_BBC_MODE_DISABLED
TTPLPFC_bbcEnabled: 0
TTPLPFC_duty1_pu: 0.0
TTPLPFC_duty2_pu: 0.0
```

Interpretazione:

```text
29   = 0x001D
2018 = 0x07E2
```

Il wrapper marca validi gli input attualmente mappati:
- `V_DC`
- `I_L_A`
- `I_L_B`
- `I_COIL_LOC`

Gli input mancanti impediscono correttamente l'attivazione dell'algoritmo completo e dei PWM BBC.

Esito: OK.

## Test 10 - UniPD Input Collector Analogico

Obiettivo:
Verificare su docking la propagazione di tensioni analogiche note verso `UNIPD_bbcInputs`.

Mapping:

```text
i_l_b      -> ADCA2   -> pin 15
i_l_a      -> ADCB12  -> pin 18
v_dc       -> ADCB4   -> pin 20
i_coil_loc -> ADCA5   -> pin 21
```

Nota:
Nel codice TI gli ingressi corrente sono scambiati:

```text
ADCA2  -> TTPLPFC_IL1_FB -> UNIPD_bbcInputs.i_l_b
ADCB12 -> TTPLPFC_IL2_FB -> UNIPD_bbcInputs.i_l_a
```

Setup iniziale proposto:

```text
GND comune
resistenza serie 4.7k o 10k su ogni ingresso

pin 15 / ADCA2  -> 1.65 V
pin 18 / ADCB12 -> 1.65 V
pin 20 / ADCB4  -> 2.00 V
pin 21 / ADCA5  -> 1.00 V
```

Valori attesi indicativi:

```text
UNIPD_bbcInputs.i_l_a      circa 0 A
UNIPD_bbcInputs.i_l_b      circa 0 A
UNIPD_bbcInputs.v_dc       circa 300 V
UNIPD_bbcInputs.i_coil_loc circa 10 A
```

Valori osservati dopo correzione collegamenti:

```text
UNIPD_bbcInputs.v_dc: 297.570892
UNIPD_bbcInputs.v_bat: 0.0
UNIPD_bbcInputs.i_l_a: -1.30764639
UNIPD_bbcInputs.i_l_b: -2.31448483
UNIPD_bbcInputs.i_coil_loc: 10.2927551
UNIPD_bbcInputs.i_coil_rem_err: 0.0
UNIPD_bbcInputs.i_coil_loc_rif: 0.0
UNIPD_bbcInputs.v_dc_pbat_rif: 0.0
UNIPD_bbcInputs.i_bat_rif_max: 0.0
UNIPD_bbcInputs.i_bat_rif_min: 0.0
UNIPD_bbcInputs.tx_1_rx_0: 0
UNIPD_bbcInputs.valid_mask: 29

UNIPD_bbcSignalValidMask: 29
UNIPD_bbcSignalMissingMask: 2018
```

Controprova:
Variando la tensione iniettata sul pin 21 / ADCA5, `UNIPD_bbcInputs.i_coil_loc` passa da circa 10.29 A a:

```text
UNIPD_bbcInputs.i_coil_loc: 14.8460388
```

Controllo di non regressione:
Durante il test analogico, PWM1A/B e PWM2A/B sono risultati stabili e corretti. PWM3 e' risultato coerente e in ritardo di 90 gradi rispetto a PWM1.

Risultato:
Gli ingressi analogici attualmente mappati verso il wrapper UniPD/BBC propagano correttamente fino a `UNIPD_bbcInputs`. La maschera dei segnali validi resta coerente con i quattro ingressi disponibili.

Esito: OK.

## Test 11 - Synthetic UniPD Docking Test

Obiettivo:
Simulare gli input UniPD mancanti tramite variabili modificabili da CCS, portare `UNIPD_bbcSignalMissingMask` a zero e verificare la risposta dell'algoritmo senza hardware di potenza.

Variabili di test introdotte:

```text
UNIPD_bbcSyntheticTestEnable
UNIPD_bbcSyntheticValidMask
UNIPD_bbcSyntheticInputs
```

Condizione di sicurezza:
`UNIPD_BBC_ENABLE_POWER_OUTPUTS = 0`, quindi l'algoritmo UniPD puo' essere eseguito ma il firmware non abilita fisicamente EPWM6/EPWM7 tramite il layer BBC.

### Test 11.1 - Ingressi sintetici disabilitati

Condizione:

```text
UNIPD_bbcSyntheticTestEnable = 0
```

Risultato osservato:

```text
UNIPD_bbcSignalValidMask: 29
UNIPD_bbcSignalMissingMask: 2018

UNIPD_bbcOutput.abilita_pwm: 0
UNIPD_bbcOutput.duty_cycle_pwm_a: 0.0
UNIPD_bbcOutput.duty_cycle_pwm_b: 0.0

TTPLPFC_bbcMode: TTPLPFC_BBC_MODE_DISABLED
TTPLPFC_bbcEnabled: 0
TTPLPFC_duty1_pu: 0.0
TTPLPFC_duty2_pu: 0.0
```

Esito: OK. Il comportamento resta quello del wrapper non invasivo con soli quattro input reali disponibili.

### Test 11.2 - Ingressi sintetici completi

Condizione:

```text
UNIPD_bbcSyntheticTestEnable = 1
UNIPD_bbcSyntheticValidMask = 2047
```

Valori sintetici iniziali:

```text
UNIPD_bbcInputs.v_dc: 300.0
UNIPD_bbcInputs.v_bat: 96.0
UNIPD_bbcInputs.i_l_a: 0.0
UNIPD_bbcInputs.i_l_b: 0.0
UNIPD_bbcInputs.i_coil_loc: 10.0
UNIPD_bbcInputs.i_coil_rem_err: 5.0
UNIPD_bbcInputs.i_coil_loc_rif: 12.0
UNIPD_bbcInputs.v_dc_pbat_rif: 320.0
UNIPD_bbcInputs.i_bat_rif_max: 5.0
UNIPD_bbcInputs.i_bat_rif_min: -5.0
UNIPD_bbcInputs.tx_1_rx_0: 1
UNIPD_bbcInputs.valid_mask: 2047
```

Output osservato:

```text
UNIPD_bbcSignalValidMask: 2047
UNIPD_bbcSignalMissingMask: 0

UNIPD_bbcOutput.i_coil_loc_err: 2.0
UNIPD_bbcOutput.duty_cycle_pwm_a: 0.0
UNIPD_bbcOutput.duty_cycle_pwm_b: 0.0
UNIPD_bbcOutput.abilita_pwm: 1
UNIPD_bbcOutput.duty_cycle_ps_a: 1.0
UNIPD_bbcOutput.duty_cycle_ps_b: 0.0
UNIPD_bbcOutput.abilita_ps: 1
UNIPD_bbcOutput.p_bat_rif: 480.0
UNIPD_bbcOutput.i_l_rif_a: 2.5
UNIPD_bbcOutput.i_l_rif_b: 2.5
UNIPD_bbcOutput.v_ac_rif: 381.971863

TTPLPFC_bbcMode: TTPLPFC_BBC_MODE_DISABLED
TTPLPFC_bbcEnabled: 0
TTPLPFC_duty1_pu: 0.0
TTPLPFC_duty2_pu: 0.0
```

Interpretazione:
Il wrapper riconosce tutti i segnali richiesti e l'algoritmo UniPD viene eseguito. Il punto sintetico iniziale produce saturazione del controllo (`p_bat_rif = 480 W`, `duty_cycle_ps_a = 1.0`, `duty_cycle_ps_b = 0.0`), coerente con gli errori sintetici applicati.

Nota naming:
La variabile `v_ac_rif` e' potenzialmente fuorviante nel contesto applicativo perche' non rappresenta una tensione AC misurata. E' da considerare per un refactor successivo, ad esempio verso un nome piu' legato al riferimento HFC/phase-shift.

### Test 11.3 - Reset controllore

Condizione:

```text
UNIPD_bbcSyntheticTestEnable = 0
```

Output osservato dopo reset:

```text
UNIPD_bbcOutput.i_coil_loc_err: 0.0
UNIPD_bbcOutput.duty_cycle_pwm_a: 0.0
UNIPD_bbcOutput.duty_cycle_pwm_b: 0.0
UNIPD_bbcOutput.abilita_pwm: 0
UNIPD_bbcOutput.duty_cycle_ps_a: 0.5
UNIPD_bbcOutput.duty_cycle_ps_b: 0.5
UNIPD_bbcOutput.abilita_ps: 0
UNIPD_bbcOutput.p_bat_rif: 0.0
UNIPD_bbcOutput.i_l_rif_a: 0.0
UNIPD_bbcOutput.i_l_rif_b: 0.0
UNIPD_bbcOutput.v_ac_rif: 0.0
```

Esito: OK. La disabilitazione del test sintetico riporta il wrapper in condizione di input mancanti e resetta gli stati di controllo.

### Test 11.4 - Punto sintetico neutro

Condizione:

```text
UNIPD_bbcSyntheticTestEnable = 1
UNIPD_bbcSyntheticValidMask = 2047

UNIPD_bbcSyntheticInputs.v_dc = 300.0
UNIPD_bbcSyntheticInputs.v_dc_pbat_rif = 300.0
UNIPD_bbcSyntheticInputs.v_bat = 96.0
UNIPD_bbcSyntheticInputs.i_l_a = 0.0
UNIPD_bbcSyntheticInputs.i_l_b = 0.0
UNIPD_bbcSyntheticInputs.i_coil_loc = 10.0
UNIPD_bbcSyntheticInputs.i_coil_loc_rif = 10.0
UNIPD_bbcSyntheticInputs.i_coil_rem_err = 0.0
UNIPD_bbcSyntheticInputs.i_bat_rif_max = 5.0
UNIPD_bbcSyntheticInputs.i_bat_rif_min = -5.0
UNIPD_bbcSyntheticInputs.tx_1_rx_0 = 1
```

Output osservato:

```text
UNIPD_bbcOutput.i_coil_loc_err: 0.0
UNIPD_bbcOutput.duty_cycle_pwm_a: 0.319999993
UNIPD_bbcOutput.duty_cycle_pwm_b: 0.319999993
UNIPD_bbcOutput.abilita_pwm: 1
UNIPD_bbcOutput.duty_cycle_ps_a: 0.5
UNIPD_bbcOutput.duty_cycle_ps_b: 0.5
UNIPD_bbcOutput.abilita_ps: 1
UNIPD_bbcOutput.p_bat_rif: 0.0
UNIPD_bbcOutput.i_l_rif_a: 0.0
UNIPD_bbcOutput.i_l_rif_b: 0.0
UNIPD_bbcOutput.v_ac_rif: 0.0
```

Interpretazione:
Il punto neutro non richiede trasferimento netto di potenza (`p_bat_rif = 0`) ne' correzione phase-shift (`duty_cycle_ps_a/b = 0.5`, `v_ac_rif = 0`). Il duty BBC pari a circa `0.32` e' coerente con il feed-forward nominale `v_bat / v_dc = 96 / 300 = 0.32`.

Esito complessivo Test 11: OK.

## Test 12 - Scheda TI Modificata, BBC BOOST a Bassa Tensione

Obiettivo:
Verificare sulla scheda TI modificata il comportamento dei leg BBC in modalita' BOOST a bassa tensione, con carico resistivo su DCLINK e bobina scollegata.

Setup hardware:

```text
ControlCARD TMDSCNCD280039C installata sulla scheda TI modificata
12 V logica separati
+BATT alimentato da alimentatore da banco
-BATT collegato a PGND
DCLINK misurato tra J38 e J37/PGND
Bobina scollegata
Carico DCLINK: 1.2 kOhm / 2 W
```

Nota debug/JTAG:
Con alimentazione logica 12 V attiva, il target non era raggiungibile via XDS110 integrato nella controlCARD. L'analisi ha individuato come causa l'isolatore JTAG `U51` (`ISO7761DBQR`) presente sulla scheda TI modificata: il lato verso il connettore JTAG esterno non risultava alimentato in assenza di probe esterna, portando i segnali JTAG in uno stato non valido. La rimozione fisica di `U51` ha ripristinato la stabilita' del debug via USB/XDS110 integrato.

Da annotare per successive revisioni hardware:
- se si usa JTAG esterno, e' necessario garantire alimentazione/VTREF lato connettore esterno e adattatore compatibile con header ETAS/JTAG a 12 pin;
- se si usa XDS110 integrato nella controlCARD, il percorso JTAG esterno isolato non deve interferire con i segnali della controlCARD.

Verifiche preliminari alimentazione:

```text
5 V control board: 4.94 V stabile
3.3 V control board: 3.301 V stabile
XRSn: 3.3 V stabile
12 V logica: circa 0.21-0.23 A in condizioni idle/test
```

### Test 12.1 - Prima prova con comando PFC complementare legacy

Condizione:
Prima della correzione del comando BBC BOOST, il dock-test usava il PWM complementare legacy del PFC.

Risultato osservato:
- anche a duty molto basso, +BATT collassava rapidamente sotto 1 V;
- la corrente dell'alimentatore entrava in limitazione;
- DCLINK saliva rapidamente;
- il fenomeno era sostanzialmente identico con `LEG1_ONLY`, `LEG2_ONLY` e `BOTH`.

Interpretazione:
Il comportamento e' coerente con la verifica docking del Test 6.1: il comando low-side risultava quasi sempre alto per duty molto bassi.

### Test 12.2 - Prova con comando BOOST dedicato

Comando usato:

```text
EPWMxA / high-side: forzato basso
EPWMxB / low-side: PWM reale con duty impostato
```

Condizioni di test:

```text
TTPLPFC_bbcDockTestMode = TTPLPFC_BBC_MODE_BOOST
TTPLPFC_bbcDockTestDutyRampEnable = 0
TTPLPFC_bbcDockTestVbusTrip_Volts usato come protezione software
Carico DCLINK = 1.2 kOhm
```

Risultati a `+BATT = 6 V`, duty 5%:

```text
LEG1_ONLY:
+BATT = 5.99 V stabile
I_BATT = 0.01 A
DCLINK = 6.2 V

LEG2_ONLY:
+BATT = 5.99 V stabile
I_BATT = 0.01 A
DCLINK = 6.2 V

BOTH:
+BATT = 5.99 V stabile
I_BATT = 0.02 A
DCLINK = 7.8 V
```

Risultati con `+BATT = 12 V`, `BOTH`, carico DCLINK 1.2 kOhm:

```text
duty 5%:
+BATT = 11.99 V
I_BATT = 0.03 A
DCLINK = 15.8 V

duty 10%:
+BATT = 11.99 V
I_BATT = 0.04 A
TTPLPFC_vBus_sensed_Volts ~= 37 V
T_MOSFET = 31.7 degC
I_logica = 0.23 A

duty 15%:
+BATT = 11.99 V
I_BATT = 0.07 A
DCLINK ~= 24 V
TTPLPFC_vBus_sensed_Volts ~= 46 V
T_MOSFET = 32.7 degC

duty 20%:
+BATT = 11.99 V
I_BATT ~= 0.10 A
DCLINK ~= 32 V
TTPLPFC_vBus_sensed_Volts ~= 62 V
```

Potenza stimata sul carico:

```text
duty 5%,  DCLINK 15.8 V -> P_load ~= 0.21 W
duty 15%, DCLINK 24 V   -> P_load ~= 0.48 W
duty 20%, DCLINK 32 V   -> P_load ~= 0.85 W
```

Nota misura VBUS:
La misura firmware `TTPLPFC_vBus_sensed_Volts` risulta circa doppia rispetto alla misura reale su DCLINK. Esempi osservati:

```text
DCLINK reale ~= 24 V -> TTPLPFC_vBus_sensed_Volts ~= 46 V
DCLINK reale ~= 32 V -> TTPLPFC_vBus_sensed_Volts ~= 62 V
```

La scala VBUS deve quindi essere verificata/tarata prima di usare soglie assolute o controlli in tensione sulla scheda modificata.

### Test 12.3 - Taratura statica misura VBUS

Condizione:

```text
TTPLPFC_bbcDockTestEnable = 0
TTPLPFC_bbcDockTestLegMode = TTPLPFC_BBC_DOCK_TEST_LEG_DISABLED
Carico DCLINK = 1.2 kOhm
PWM BBC disabilitati
```

Misure:

```text
+BATT = 5.99 V  -> DCLINK = 5.031 V, I_BATT = 0.00 A, TTPLPFC_vBus_sensed_Volts ~= 9.7 V
+BATT = 8.99 V  -> DCLINK = 7.87 V,  I_BATT = 0.00 A, TTPLPFC_vBus_sensed_Volts ~= 15.3 V
+BATT = 11.99 V -> DCLINK = 10.73 V, I_BATT = 0.01 A, TTPLPFC_vBus_sensed_Volts ~= 20.7 V
+BATT = 14.99 V -> DCLINK = 13.58 V, I_BATT = 0.01 A, TTPLPFC_vBus_sensed_Volts ~= 26.5 V
+BATT = 17.99 V -> DCLINK = 16.44 V, I_BATT = 0.01 A, TTPLPFC_vBus_sensed_Volts ~= 31.8 V
```

Rapporto medio osservato:

```text
TTPLPFC_vBus_sensed_Volts / DCLINK_reale ~= 1.94
DCLINK_reale / TTPLPFC_vBus_sensed_Volts ~= 0.515
```

Correzione applicata:

```text
TTPLPFC_WLESS_VBUS_SENSE_CORRECTION = 0.515
```

La correzione e' applicata al path BBC/WLESS usato dai test sulla scheda modificata, mantenendo separata la scala legacy TI.

Validazione post-correzione:

```text
+BATT OFF -> DCLINK = 0.001 V, TTPLPFC_vBus_sensed_Volts ~= 0 V, max ~= 0.06 V

+BATT = 6 V  -> DCLINK = 5.038 V,  TTPLPFC_vBus_sensed_Volts ~= 4.99-5.05 V
+BATT = 12 V -> DCLINK = 10.73 V,  TTPLPFC_vBus_sensed_Volts ~= 10.5-10.75 V
+BATT = 18 V -> DCLINK = 16.44 V,  TTPLPFC_vBus_sensed_Volts ~= 16.43-16.56 V
```

Risultato:
La misura `TTPLPFC_vBus_sensed_Volts` e' ora coerente con la tensione DCLINK reale nel range testato.

### Test 12.4 - BBC BOOST dopo correzione VBUS

Condizione:

```text
+BATT = 12 V
Carico DCLINK = 1.2 kOhm
TTPLPFC_bbcDockTestLegMode = TTPLPFC_BBC_DOCK_TEST_LEG_BOTH
```

Misure:

```text
duty 10%:
DCLINK = 19.1 V
TTPLPFC_vBus_sensed_Volts ~= 19.2 V
I_BATT = 0.04 A
T_MOSFET = 28.8 degC
VBUS trip: non intervenuto

duty 20%:
DCLINK ~= 32 V
TTPLPFC_vBus_sensed_Volts ~= 32 V
I_BATT ~= 0.10 A
VBUS trip: non intervenuto
```

Risultato:
Il BBC mantiene la stessa risposta progressiva al duty e la misura firmware VBUS segue direttamente il valore reale di DCLINK.

### Test 12.5 - Verifica intervento VBUS trip

Condizione:

```text
+BATT = 12 V
Carico DCLINK = 1.2 kOhm
TTPLPFC_bbcDockTestLegMode = TTPLPFC_BBC_DOCK_TEST_LEG_BOTH
TTPLPFC_bbcDockTestDuty1_pu = 0.20
TTPLPFC_bbcDockTestDuty2_pu = 0.20
TTPLPFC_bbcDockTestVbusTrip_Volts = 25.0
```

Risultato osservato:
Il trip VBUS interviene correttamente e disabilita il dock-test. L'intervento e' rapido; dopo il trip DCLINK torna velocemente verso il valore passivo di circa 10.73 V, rendendo difficile leggere il valore di picco in tempo reale.

Nota firmware:
Sono state aggiunte variabili latched di supporto al test:

```text
TTPLPFC_bbcDockTestVbusTripCapture_Volts
TTPLPFC_bbcDockTestVbusMax_Volts
```

Queste variabili permettono di leggere da CCS il valore VBUS al momento del trip e il massimo VBUS visto durante il dock-test.

Esito:
OK. Con il comando BOOST dedicato, il BBC lavora in modo progressivo e stabile a bassa tensione. L'aumento del duty produce aumento coerente di DCLINK e corrente di ingresso, senza collasso dell'alimentatore e senza riscaldamento anomalo dei MOSFET nelle condizioni testate.

## Test 13 - Preparazione HFC e Tuning Frequenza Tank

### Razionale frequenza

La versione aggiornata del documento tecnico (`DT_2025_39_11_Progetto_WLESS_CHRG_Rev.1.2_DraftC.pdf`) conferma che la rete verso la bobina non e' un collegamento diretto: sono presenti due banchi di capacita' in serie al coil, uno per ciascun ramo HFC.

Il documento indica:

```text
Capacita' target per blocco: 165 nF
Capacita' risonante equivalente nel loop: 82.5 nF
Coil design point: 45 uH
Frequenza risonanza design point: 82.5 kHz
```

Lo stesso documento riporta anche i casi con due coil accoppiati:

```text
Gap 5 cm: L ~= 57 uH, k ~= 0.32, Ceq = 66 nF, f0 ~= 82 kHz
Gap 6 cm: L ~= 55 uH, k ~= 0.23, Ceq = 66 nF, f0 ~= 83.5 kHz
```

Nota di verifica hardware:
nel disegno del DT alcuni condensatori paralleli ai banchi sono indicati come `DNM`. Prima dei test con bobina collegata va quindi verificato quali componenti sono effettivamente montati sulla scheda modificata. La nostra ipotesi precedente, basata sui soli riferimenti gia' individuati, era:

```text
Banco A: C10, C12, C14, C16 = 4 x 0.033 uF = 132 nF
Banco B: C2, C4, C6, C8     = 4 x 0.033 uF = 132 nF
Ceq = 132 nF / 2 = 66 nF
```

Con bobina indicata dall'utente a `54 uH` e `Ceq = 66 nF`, il calcolo torna:

```text
f0 = 1 / (2*pi*sqrt(L*Ceq))
L = 54 uH
Ceq = 66 nF
f0 ~= 84.3 kHz
```

Per questa ragione la frequenza nominale HFC resta `85 kHz`: e' dentro la finestra indicata dal DT e vicina sia al calcolo con i componenti finora identificati sia ai casi accoppiati riportati nel documento. Il range operativo resta:

```text
CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ = 85 kHz
CLLLC_MIN_PWM_SWITCHING_FREQUENCY_HZ     = 75 kHz
CLLLC_MAX_PWM_SWITCHING_FREQUENCY_HZ     = 90 kHz
```

Il rapporto gia' usato tra switching HFC e ISR2 resta invariato:

```text
CLLLC_ISR2_FREQUENCY_HZ = 21.25 kHz
```

### Test HFC statico previsto

Obiettivo:
verificare lo stadio HFC a bassa tensione senza sovrapporre ancora il comportamento BBC e senza richiedere trasferimento di potenza verso una bobina accoppiata.

Condizione iniziale:

```text
Logica 12 V attiva
Debug CCS attivo
BBC disabilitato:
  TTPLPFC_bbcDockTestEnable = 0
  TTPLPFC_bbcDockTestLegMode = TTPLPFC_BBC_DOCK_TEST_LEG_DISABLED
  TTPLPFC_bbcEnabled = 0

+BATT disalimentato
DCLINK alimentato esternamente a bassa tensione, con limite di corrente
Bobina inizialmente scollegata
Carico resistivo DCLINK opzionale, da mantenere se utile per scarica/stabilita'
```

Variabili da monitorare:

```text
CLLLC_clearTrip
CLLLC_pwmFrequency_Hz
CLLLC_pwmPeriod_pu
CLLLC_pwmSwStateActive
TTPLPFC_vBus_sensed_Volts
TTPLPFC_bbcDockTestEnable
TTPLPFC_bbcEnabled
```

Grandezza esterne da osservare:

```text
DCLINK-PGND
Corrente alimentatore DCLINK
Corrente alimentazione logica 12 V
Temperatura MOSFET HFC
Eventuale tensione ai terminali bobina, solo se accessibile e con sonde adeguate
```

Sequenza prudente:

```text
1. Verificare a scheda ferma che DCLINK sia scarico.
2. Alimentare solo logica 12 V e avviare debug.
3. Confermare BBC disabilitato.
4. Alimentare DCLINK a bassa tensione, partendo da 6-12 V e corrente limitata.
5. Eseguire firmware con trip ancora attivo.
6. Abilitare i PWM HFC solo tramite la sequenza firmware gia' usata (`CLLLC_clearTrip = 1`).
7. Osservare assorbimenti, DCLINK e temperatura per alcuni minuti.
8. Ripetere eventualmente a tensione DCLINK leggermente superiore solo se il punto precedente e' stabile.
```

Nota:
con bobina scollegata non ci si aspetta una verifica energetica del tank. Questa prova serve a validare commutazione, stabilita' elettrica e assenza di anomalie evidenti dello stadio HFC prima di collegare un percorso risonante reale.

### Test HFC con rete risonante previsto

Da eseguire solo dopo il test HFC statico e dopo aver verificato la presenza/valore dei banchi capacitivi serie.

Condizioni aggiuntive:

```text
Bobina collegata ai punti coil
Seconda bobina nella posizione meccanica fissa disponibile: parallela, affacciata, gap 7 cm
DCLINK alimentato a bassa tensione e corrente limitata
BBC disabilitato
Trip/protezioni attive
```

Grandezze da osservare:

```text
DCLINK-PGND
Corrente alimentatore DCLINK
Temperatura MOSFET HFC
Temperatura banco capacitivo/bobina se misurabile
CLLLC_iPrimTankModSensedRaw / CLLLC_iPrimTankModSensed_pu
CLLLC_iPrimTankPhsSensedRaw / CLLLC_iPrimTankPhsSensed_pu
```

Obiettivo:
verificare che il path di misura `ITANK`, `ITANK_MOD` e `ITANK_PHS` risponda in modo coerente alla corrente risonante reale, prima di usare questi segnali per logiche di controllo o per algoritmi UniPD.

## Test 14 - HFC e Primo Trasferimento a Bassa Tensione

### Test HFC a vuoto

Condizioni:

```text
Scheda primaria modificata
Logica 12 V attiva
DCLINK alimentato esternamente a 12 V
Bobina scollegata
BBC disabilitato
CLLLC_LAB = 1
CLLLC_clearTrip usato per liberare i trip e abilitare la commutazione HFC
```

Esito firmware:

```text
CLLLC_tripFlag = noTrip
CLLLC_hfcGanFaultGpioLevel = 1
CLLLC_hfcGanFaultActiveLow = 0
CLLLC_epwm1GanFaultOst2Latched = 0
CLLLC_epwm2GanFaultOst2Latched = 0
CLLLC_epwm3GanFaultOst2Latched = 0
EPwm1Regs.TZFLG = 0
EPwm2Regs.TZFLG = 0
EPwm3Regs.TZFLG = 0
CLLLC_pwmFrequency_Hz ~= 85 kHz
```

Misura corretta:

```text
CH1 su un terminale coil rispetto a PGND
CH2 sull'altro terminale coil rispetto a PGND
MATH = CH1 - CH2
Sonde 10x
```

Risultato:

```text
DCLINK = 12 V
I_DCLINK ~= 0.02 A
Vcoil_diff ~= 24 Vpp
f ~= 85 kHz
T MOSFET ~= 33 degC
```

Nota di misura:
la misura single-ended direttamente tra i due terminali coil, usando il ground
clip della sonda su uno dei terminali, altera il circuito e fa salire la
corrente DCLINK. La misura differenziale ricostruita con due sonde riferite a
PGND e funzione `MATH = CH1 - CH2` non introduce lo stesso carico.

### Bobina primaria sola

Condizioni:

```text
Bobina primaria collegata
Seconda bobina assente
Secondaria non presente
DCLINK alimentato a bassa tensione
BBC disabilitato
```

Risultati principali:

```text
DCLINK = 12 V -> Vcoil_diff ~= 336 Vpp, f ~= 85 kHz
DCLINK = 6 V  -> Vcoil_diff ~= 176 Vpp, f ~= 85 kHz
```

Con `DCLINK = 3.02 V`:

```text
I_DCLINK = 0.77 A
P_in ~= 2.3 W
Vcoil_diff = 120 Vpp
f = 85 kHz
T MOSFET = 28 degC
tripFlag = noTrip
glitch commutazione = circa 20 V, durata 80 ns
stabilita' osservata >10 min
```

Osservazione:
con sola bobina primaria il tank lavora praticamente a vuoto e mostra una
forte amplificazione di tensione. La tensione differenziale sulla bobina puo'
raggiungere valori elevati anche con DCLINK basso; trattare quindi la bobina
come nodo HV AC durante i test.

### Due bobine, secondaria a vuoto

Condizioni:

```text
Due bobine affacciate, parallele, distanza meccanica disponibile 7 cm
Seconda bobina non caricata
DCLINK primario a bassa tensione
```

Risultati:

```text
DCLINK = 3 V
I_DCLINK = 0.09 A
Vcoil primaria = 65 Vpp
f = 85 kHz
T MOSFET = 30 degC
glitch = 13 V, durata 80 ns

DCLINK = 6 V
I_DCLINK = 0.14 A
Vcoil primaria = 130 Vpp
f = 85 kHz
T MOSFET = 29 degC
glitch = 27.5 V, durata 80-90 ns

DCLINK = 9 V
glitch = 39 V, durata 80-90 ns
```

Osservazione:
la presenza della seconda bobina smorza sensibilmente il tank rispetto alla
prova con sola bobina primaria. Il glitch scala approssimativamente con la
tensione ed e' compatibile con un evento legato a dead-time/commutazione HFC;
per ora viene monitorato ma non considerato bloccante.

### Seconda scheda passiva con logica alimentata

Condizioni:

```text
Scheda primaria: HFC attivo, BBC disabilitato
Scheda secondaria: logica 12 V alimentata, senza controlCARD
Carico su DCLINK secondario: 1.2 kohm tra DCLINK e PGND
Bobine affacciate, parallele, gap 7 cm
Misure coil eseguite con MATH = CH1 - CH2
```

Curva di trasferimento a carico leggero:

```text
DCLINKp | I_DCLINKp | Vcoil_p | Vcoil_s | DCLINKs | trip
3 V     | 0.10 A    | 66 Vpp  | 21 Vpp  | 4.4 V   | noTrip
6 V     | 0.16 A    | 130 Vpp | 38 Vpp  | 9.84 V  | noTrip
9 V     | 0.22 A    | 150 Vpp | 53 Vpp  | 15.84 V | noTrip
```

Temperature al punto `DCLINKp = 9 V`:

```text
T MOSFET primario = 32 degC
T MOSFET secondario = 28.2 degC
T resistenza DCLINK primario = 30 degC
T resistenza DCLINK secondario = 35 degC
```

Esito:
primo trasferimento energia a bassa tensione verificato. Il DCLINK secondario
si carica stabilmente attraverso la rete secondaria passiva e il carico da
1.2 kohm. Non sono stati osservati trip; le temperature restano contenute.

### Catena completa open-loop: +BATT -> BBC -> HFC -> bobine -> DCLINK secondario

Condizioni:

```text
Scheda primaria:
  +BATT = 6 V
  BBC in modalita' dock-test BOOST, BOTH LEGS
  HFC attivo a 85 kHz
  VBUS trip impostato circa a 9.5 V

Scheda secondaria:
  Logica 12 V alimentata
  ControlCARD assente
  Carico DCLINK secondario = 1.2 kohm

Bobine:
  Due bobine affacciate, parallele, gap 7 cm
```

Verifica preliminare BBC-only:

```text
HFC OFF
Duty BBC = 0.05
+BATT = 6 V
I_BATT = 0.02 A
DCLINK primario = 7.8 V
T MOSFET = 26 degC
tripFlag = noTrip
```

Verifica diagnostica:
con HFC attivo e duty BBC fisso, DCLINK primario scende a un punto operativo
piu' basso. Forzando OFF l'HFC con duty BBC = 0.10, DCLINK primario risale fino
a circa 9.5 V. Questo conferma che il BBC e' funzionante e che il punto
operativo piu' basso e' dovuto al carico HFC/tank/secondaria in open-loop, non
a un trip.

Mappa open-loop con HFC attivo:

```text
Duty BBC | I_BATT | DCLINKp | Vcoil_p | DCLINKs | T MOSFETp | T MOSFETs | trip
0.05     | 0.10 A | 2.5 V   | 55 Vpp  |   -     | 30 degC   |    -      | noTrip
0.07     | 0.10 A | 2.7 V   | 58 Vpp  |   -     | 32 degC   |    -      | noTrip
0.10     | 0.10 A | 2.8 V   | 62 Vpp  |   -     | 32 degC   |    -      | noTrip
0.12     | 0.12 A | 2.98 V  | 66 Vpp  | 4.25 V  | 30 degC   |    -      | noTrip
0.15     | 0.13 A | 3.2 V   | 70 Vpp  | 4.7 V   | 30 degC   |    -      | noTrip
0.20     | 0.15 A | 3.7 V   | 80 Vpp  | 5.5 V   | 31 degC   | 28 degC  | noTrip
0.25     | 0.17 A | 4.13 V  | 90 Vpp  | 6.3 V   | 30 degC   | 30 degC  | noTrip
```

Nota:
il comportamento e' coerente con una prova open-loop a duty fisso. Non essendo
ancora attivo un controllo di DCLINK/VBUS, l'accensione dell'HFC sposta il
punto operativo del bus primario in funzione del carico risonante e del carico
riflesso dalla secondaria.

### Ipotesi per test inverso BBC BUCK

Obiettivo del prossimo blocco di test:
verificare il trasferimento inverso limitandosi inizialmente al solo BBC, senza
HFC e senza bobine. La prova prevista e':

```text
DCLINK alimentato esternamente a bassa tensione
+BATT sostituito da carico passivo
BBC in modalita' BUCK open-loop
```

Ipotesi di pattern prudenziale per il primo test:

```text
EPWMxA / PHx_H / high-side = PWM con duty comandato
EPWMxB / PHx_L / low-side  = forzato basso
```

Questa ipotesi e' coerente con la semantica del duty calcolato da UniPD
(`duty = V_buckboost / V_dc`), ma non e' ancora una prescrizione esplicita del
documento tecnico. Prima del test sulla scheda modificata il pattern dovra'
essere verificato su docking station e/o confermato dal responsabile tecnico.

Per le prime prove non si usa rettificazione sincrona: il low-side resta spento
per ridurre il rischio di conduzione indesiderata. L'eventuale comando
complementare sincrono con dead-time sara' valutato solo dopo aver validato il
comportamento base.

Stato firmware:
il pattern e' predisposto solo nel dock-test BBC. Non e' collegato alle uscite
di potenza UniPD e resta inattivo se `TTPLPFC_bbcDockTestEnable = 0`.

### Verifica docking del pattern BBC BUCK

Condizioni:

```text
ControlCARD su docking station
Nessuna alimentazione di potenza collegata
Misura logica sui pin EC1/J6/J7 della docking
TTPLPFC_bbcDockTestMode = TTPLPFC_BBC_MODE_BUCK
TTPLPFC_bbcDockTestDuty1_pu = 0.05
TTPLPFC_bbcDockTestDuty2_pu = 0.05
```

Mapping verificato:

```text
EPWM6A / GPIO10 / PH1_H -> EC1.61 -> J6.7
EPWM6B / GPIO11 / PH1_L -> EC1.63 -> J6.8
EPWM7A / GPIO12 / PH2_H -> EC1.58 -> J7.5
EPWM7B / GPIO13 / PH2_L -> EC1.60 -> J7.6
```

Risultati:

```text
LEG1_ONLY:
  EC1.61 / EPWM6A / PH1_H = PWM 120 kHz, duty 5%
  EC1.63 / EPWM6B / PH1_L = basso
  EC1.58 / EPWM7A / PH2_H = basso
  EC1.60 / EPWM7B / PH2_L = basso

LEG2_ONLY:
  EC1.58 / EPWM7A / PH2_H = PWM 120 kHz, duty 5%
  EC1.60 / EPWM7B / PH2_L = basso
  EC1.61 / EPWM6A / PH1_H = basso
  EC1.63 / EPWM6B / PH1_L = basso
```

Esito:
OK. Il pattern BUCK non sincrono e' stato verificato a livello logico su
docking station: pulsa solo il comando high-side del ramo selezionato, mentre
il low-side e l'altro ramo restano spenti.

### Primo test BBC BUCK sulla scheda modificata

Condizioni:

```text
Scheda TI modificata
Logica 12 V attiva, limite alimentatore 1 A
DCLINK alimentato esternamente a 6 V, limite 1 A
Resistenza di carico su DCLINK-PGND
Resistenza di carico su +BATT/-BATT
HFC non utilizzato
BBC in dock-test BUCK non sincrono
```

Pre-flight:

```text
DCLINK reale = 6 V
+BATT reale = 0 V
I_DCLINK = 0 A
I_logica_12V = 0.20 A
```

Risultati:

```text
Modo       Duty   +BATT    I_DCLINK  I_logica  VBUS_FW  Note
LEG1_ONLY 0.02   ~3.3 V   0.01 A    0.22 A    ~5.95 V  duty1=0.02, duty2=0
OFF        -      coerente 0 A       -         -        spegnimento corretto
LEG2_ONLY 0.02   ~3.3 V   0.01 A    -         ~6 V     duty1=0, duty2=0.02
BOTH      0.02   ~4.5 V   0.01 A    0.23 A    ~6 V     duty1=duty2=0.02
BOTH      0.05   ~4.6 V   0.01 A    0.23 A    ~6 V     stabile
BOTH      0.10   ~5.0 V   0.01 A    0.23 A    -        stabile
BOTH      0.20   ~5.7 V   0.01 A    0.23 A    ~6 V     stabile
BOTH      0.25   ~5.5 V   -         -         -        stabile
BOTH      0.30   ~5.2 V   -         -         -        stabile
```

Esito:
OK preliminare. Entrambi i rami BBC sono stati verificati in modalita' BUCK
non sincrona a bassa tensione. Il comando e' simmetrico tra i rami e lo
spegnimento tramite `TTPLPFC_bbcDockTestEnable = 0` e' coerente.

Nota:
la curva `duty -> +BATT` non e' monotona nel setup a carico leggero. Il valore
massimo osservato e' circa `5.7 V` a duty 20%; a duty 25% e 30% la tensione
scende rispettivamente a circa `5.5 V` e `5.2 V`. In questa configurazione il
punto operativo sembra quindi dominato da conduzione discontinua, carico
effettivo ridotto e possibili percorsi passivi/parassiti. Il test valida la
commutazione BUCK a bassa energia, ma non e' sufficiente per identificare una
legge duty/tensione utilizzabile come controllo.

Secondo assetto di carico:

```text
DCLINK = 6 V
Carico +BATT = 82 ohm / 150 W
BBC BUCK, BOTH legs
```

Risultati osservati durante lo sweep duty:

```text
+BATT reale: da ~1.7 V a ~3.3 V
I_DCLINK: da ~0.01 A a ~0.03 A
I_logica: ~0.23 A
T resistenza 82 ohm: nessuna variazione sensibile
TTPLPFC_vBus_sensed_Volts: ~6 V
```

Esito:
con carico da 82 ohm la prova BUCK diventa piu' significativa rispetto al
carico da 1.2 kohm. Il trasferimento resta a bassa potenza, non sono osservati
incrementi termici apprezzabili e la corrente DCLINK resta contenuta.

Prova con DCLINK aumentato:

```text
DCLINK = 9 V
Carico +BATT = 82 ohm / 150 W
BBC BUCK, BOTH legs
```

Risultati:

```text
Duty  +BATT  I_DCLINK
0.10  2.8 V  0.03 A
0.20  4.7 V  0.04 A
0.30  4.6 V  0.04 A
```

Esito:
il comportamento resta stabile aumentando DCLINK a 9 V. La curva mostra ancora
un massimo locale intorno a duty 20%, coerente con quanto osservato a 6 V ma
con livelli di tensione/corrente piu' misurabili.

Prova a DCLINK 12 V:

```text
DCLINK = 12 V
Carico +BATT = 82 ohm / 150 W
BBC BUCK, BOTH legs
```

Risultati:

```text
Duty  +BATT  I_DCLINK
0.10  3.8 V  0.04 A
0.20  6.3 V  0.06 A
0.30  6.2 V  0.06 A
```

Esito:
anche a DCLINK 12 V il trasferimento BUCK resta stabile e a bassa corrente. Il
massimo osservato rimane intorno a duty 20%; duty 30% non aumenta ulteriormente
la tensione sul carico.

## Test 15 - Catena Completa SOURCE -> SINK Open-Loop

### Obiettivo

Verificare il trasferimento end-to-end a bassa tensione con due schede TI
modificate:

```text
+BATT1
  -> BBC BOOST scheda 1
  -> DCLINK1
  -> HFC inverter scheda 1
  -> bobine accoppiate
  -> HFC/rectifier passivo scheda 2
  -> DCLINK2
  -> BBC BUCK scheda 2
  -> carico resistivo su +BATT2
```

Il test e' eseguito in open-loop, senza UniPD attivo sulle uscite di potenza e
senza controllo chiuso di DCLINK o corrente.

### Setup

Scheda TI modificata 1, ruolo SOURCE:

```text
controlCARD montata
logica 12 V alimentata, limite 1 A
bobina collegata
+BATT1 alimentato a 6 V, limite 1 A
multimetro su DCLINK1
resistenza DCLINK1-PGND = 1.2 kohm
HFC attivo a 85 kHz
BBC in dock-test BOOST
```

Scheda TI modificata 2, ruolo SINK:

```text
controlCARD inizialmente non montata per test passivo
poi controlCARD montata per test BBC BUCK
logica 12 V alimentata, limite 1 A
bobina collegata
carico +BATT2 = 82 ohm / 150 W
multimetro su DCLINK2
resistenza DCLINK2-PGND = 1.2 kohm
HFC non comandato, usato come rectifier passivo
BBC in dock-test BUCK durante il test controllato
```

Nota:
i gate HFC della scheda 2 non sono direttamente accessibili; lo stato sicuro e'
stato verificato indirettamente tramite assenza di tensioni/correnti anomale a
riposo e tramite comportamento stabile durante i test passivi.

### Baseline a PWM secondari disabilitati

Con scheda 1 alimentata ma BBC/HFC non ancora usati per trasferimento verso il
carico:

```text
Scheda 1:
  logica 12 V: 0.20 A
  +BATT1 = 6 V
  I_BATT1 con BBC OFF = 0 A
  DCLINK1 = 5.037 V
  CLLLC_tripFlag = noTrip
  TTPLPFC_bbcDockTestEnable = 0
  CLLLC_clearTrip = 0

Scheda 2:
  logica 12 V: 0.13 A
  DCLINK2 = 0 V
  VLOAD su 82 ohm = 0 V
```

Esito:
baseline OK. La scheda 2 non mostra conduzioni o tensioni anomale con sola
logica alimentata e senza controlCARD.

### Receiver passivo, scheda 2 senza controlCARD

Scheda 1 con HFC attivo e BBC BOOST progressivamente incrementato; scheda 2
senza controlCARD, usata come receiver passivo con carico DCLINK da 1.2 kohm.

Risultati:

```text
Condizione                         DCLINK1  I_BATT1  DCLINK2  VLOAD  Trip1
HFC ON, BBC OFF                    1.9 V    0.06 A   2.2 V    -      -
BOOST duty 0.05 + HFC ON           2.6 V    0.09 A   3.3 V    0 V    noTrip
BOOST duty 0.10 + HFC ON           2.9 V    0.10 A   3.9 V    0 V    noTrip
BOOST duty 0.15 + HFC ON           3.4 V    0.12 A   4.7 V    0 V    noTrip
```

Esito:
il trasferimento induttivo verso il DCLINK secondario e' progressivo e stabile.
Il carico su +BATT2 resta a 0 V, come atteso, perche' il BBC secondario non e'
attivo.

### SINK controllato: BBC BUCK sulla scheda 2

La scheda 1 viene lasciata in SOURCE open-loop. La scheda 2 monta la
controlCARD, viene debuggata separatamente e viene abilitato solo il BBC BUCK
in dock-test. HFC scheda 2 resta non comandato.

Punto iniziale prima del BUCK:

```text
Scheda 1 SOURCE:
  +BATT1 = 6 V
  I_BATT1 = 0.12 A
  BBC BOOST duty = 0.15
  HFC ON
  DCLINK1 = 3.3 V

Scheda 2:
  controlCARD montata
  DCLINK2 = 4.6 V
  CLLLC_tripFlag = noTrip
  DCLINK2 load = 1.2 kohm
```

Test BBC BUCK scheda 2:

```text
TTPLPFC_bbcDockTestMode = TTPLPFC_BBC_MODE_BUCK
TTPLPFC_bbcDockTestLegMode = TTPLPFC_BBC_DOCK_TEST_LEG_BOTH
```

Risultati:

```text
SOURCE BOOST  SINK BUCK  DCLINK1  I_BATT1  DCLINK2  VLOAD  I_logica2  Note
0.15          0.02       -        0.12 A   2.9 V    0.62 V 0.20 A     BUCK attivo, no anomalie
0.15          0.05       3.2 V    0.12 A   2.7 V    0.70 V 0.25 A     stabile
0.20          0.05       3.7 V    0.15 A   3.3 V    0.80 V 0.25 A     stabile
0.20          0.10       3.7 V    0.15 A   2.7 V    1.00 V 0.25 A     carico maggiore
0.25          0.10       4.1 V    0.18 A   3.3 V    1.10 V 0.25 A     stabile
```

Calcolo potenza sull'ultimo punto:

```text
P_in_BATT1 = 6 V * 0.18 A = 1.08 W
P_load = 1.10^2 / 82 ohm = 0.0148 W
eta_senza_logica = 0.0148 / 1.08 = 1.4 %
```

Se si includono le sole alimentazioni logiche dell'ultimo assetto:

```text
P_logica1 ~= 12 V * 0.26 A = 3.12 W
P_logica2 ~= 12 V * 0.25 A = 3.00 W
P_tot_banco ~= 1.08 W + 3.12 W + 3.00 W = 7.20 W
eta_banco ~= 0.0148 / 7.20 = 0.2 %
```

Esito:
OK. La catena energetica completa e' stata verificata in open-loop a bassa
tensione. Il valore di efficienza non e' rappresentativo del sistema finale:
il test usa DCLINK molto bassi, rectifier secondario passivo, carichi piccoli,
resistenze di scarica su entrambi i bus, nessun controllo chiuso e duty non
ottimizzati.

Conclusione del test:

```text
+BATT1 -> BBC BOOST1 -> HFC1 -> bobine -> HFC2 passivo -> BBC BUCK2 -> carico
```

Il percorso e' funzionalmente dimostrato. Non sono stati osservati trip o
instabilita' nei punti testati.

### Prossime opzioni di test

Opzione A - curva distanza/accoppiamento:

```text
Mantenere gli stessi duty dell'ultimo punto stabile:
  SOURCE BOOST = 0.25
  SINK BUCK = 0.10
+BATT1 = 6 V
RLOAD = 82 ohm

Ripetere a distanze diverse tra le bobine:
  7 cm
  5 cm
  3 cm
  distanza minima sicura

Per ogni punto registrare:
  I_BATT1
  DCLINK1
  DCLINK2
  VLOAD
  eventuali temperature
```

Questa prova permette di stimare quanto l'efficienza e il trasferimento siano
limitati dall'accoppiamento magnetico.

Opzione B - curva frequenza HFC:

```text
Distanza bobine fissa
duty SOURCE/SINK fissi
variare frequenza HFC in piccoli passi attorno a 85 kHz
registrare VLOAD, DCLINK2 e I_BATT1
```

Questa prova permette di cercare il punto risonante reale dell'assetto montato.

Opzione C - passaggio firmware:

```text
integrare una state machine minima OFF/PRECHARGE/SOURCE_OPEN_LOOP/
SINK_OPEN_LOOP/FAULT prima di collegare gli algoritmi UniPD alle uscite PWM.
```

Questa opzione e' consigliata prima di abilitare controllo chiuso o setpoint
UniPD sulle uscite di potenza.

## Test 16 - Preparazione HFC Receiver/Rectifier Attivo

Obiettivo:
predisporre un test mode firmware esplicito per comandare il ponte HFC della
scheda SINK come receiver/rectifier attivo, senza riabilitare il vecchio
secondario TI rimosso.

Modifica firmware:

```text
CLLLC_hfcReceiverTestEnable
CLLLC_hfcReceiverTestRun
CLLLC_hfcReceiverTestActive
CLLLC_hfcReceiverTestDuty_pu
CLLLC_hfcReceiverTestPhaseShiftPrimLegs_pu
```

Semantica:

```text
Enable = 0:
  nessun effetto sul comportamento HFC gia' verificato.

Enable = 1, Run = 0:
  il test mode e' armato e forza in trip one-shot EPWM1/EPWM2, piu' PWM3
  quando configurato come uscita di sincronismo/debug. Stato sicuro.

Enable = 1, Run = 1:
  vengono applicati duty e phase shift fissi. L'uscita PWM richiede ancora
  l'azione esplicita CLLLC_clearTrip = 1.
```

Valori di default:

```text
CLLLC_hfcReceiverTestEnable = 0
CLLLC_hfcReceiverTestRun = 0
CLLLC_hfcReceiverTestActive = 0
CLLLC_hfcReceiverTestDuty_pu = 0.5
CLLLC_hfcReceiverTestPhaseShiftPrimLegs_pu = 0.0
```

Verifica build:

```text
RELEASE build OK
```

Note:
questa non e' ancora una vera rettifica sincrona chiusa su misura di corrente.
E' un primo modo fixed-timing, controllabile da CCS, per capire se il comando
attivo del ponte HFC SINK migliora il trasferimento rispetto al receiver
passivo.

Esito successivo:

il test fixed-timing del ponte HFC SINK non viene mantenuto come percorso di
sviluppo per l'hardware attuale. DT Rev. 1.2 descrive i circuiti analogici
necessari per ricavare `ITANK_MOD` e `ITANK_PHS`, ma tali circuiti non risultano
implementati sulle schede TI modificate attualmente in prova. Senza una misura
locale affidabile di ampiezza/fase della corrente di tank, la rettifica
sincrona non puo' essere controllata in modo robusto.

Decisione:

```text
CLLLC_hfcReceiverTestEnable = 0
CLLLC_hfcReceiverTestRun = 0
```

La scheda SINK viene quindi usata con HFC passivo. Il percorso reverse
attualmente testabile resta:

```text
bobina -> HFC passivo -> DCLINK2 -> BBC buck -> carico/batteria
```
