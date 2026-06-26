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

Sulla scheda modificata sono stati individuati due banchi di condensatori serie verso la bobina:

```text
Banco A: C10, C12, C14, C16 = 4 x 0.033 uF = 132 nF
Banco B: C2, C4, C6, C8     = 4 x 0.033 uF = 132 nF
```

Se i due banchi sono in serie nel loop della bobina, la capacita' equivalente vista dal tank e':

```text
Ceq = 132 nF / 2 = 66 nF
```

Con bobina Wurth Elektronik `750372078`, induttanza corretta indicata `54 uH`:

```text
f0 = 1 / (2*pi*sqrt(L*Ceq))
L = 54 uH
Ceq = 66 nF
f0 ~= 84.3 kHz
```

Per questa ragione la frequenza nominale HFC viene mantenuta a `85 kHz`, valore molto vicino alla risonanza stimata, mantenendo il range operativo precedente:

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
