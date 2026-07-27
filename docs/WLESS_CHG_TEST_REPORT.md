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

CLLLC_unusedAdca5SensedRaw: 108
CLLLC_iTankModSensedRaw: 116
CLLLC_unusedAdca5Sensed_pu: 0.0263671875
CLLLC_iTankModSensed_pu: 0.0283203125
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
unused / ITANK_PHS placeholder -> ADCA5  -> J26 pin 28
ITANK_MOD -> ADCC11 -> J26 pin 31
```

Misure storiche su ADCA5:

```text
0 V       -> CLLLC_unusedAdca5SensedRaw = 0
~1.650 V  -> CLLLC_unusedAdca5SensedRaw = ~1900
~3.0 V    -> CLLLC_unusedAdca5SensedRaw = ~3470
```

Misure storiche su ADCC11:

```text
0 V       -> CLLLC_iTankModSensedRaw = ~0
~1.64 V   -> CLLLC_iTankModSensedRaw = ~1900
~3.0 V    -> CLLLC_iTankModSensedRaw = ~3450
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
CLLLC_iTankModSensedRaw / CLLLC_iTankModSensed_pu
CLLLC_unusedAdca5SensedRaw / CLLLC_unusedAdca5Sensed_pu
```

Obiettivo:
verificare che il path di misura `ITANK_MOD` risponda in modo coerente alla corrente risonante reale, prima di usare questo segnale per logiche di controllo o per algoritmi UniPD. Il canale ADCA5 / J26.28 non e' associato a un circuito `ITANK_PHS` valido sulle schede attuali e deve essere trattato come rumore. Nota DT: il circuito che genera il segnale di fase della corrente non e' stato ancora inserito nel prototipo perche' in questa fase non si sta ancora correggendo la frequenza di risonanza.

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
sviluppo definitivo. Le verifiche successive hanno chiarito la mappatura dei
segnali analogici: `ITANK_MOD` e' su J26.31 / ADCC11 ed e' l'unico segnale di
corrente bobina utilizzabile per il rectifier sulle schede attuali. Il canale
ADCA5 / J26.28 non e' associato a un circuito `ITANK_PHS` valido e campiona
solo rumore; secondo DT il circuito di fase non e' stato inserito perche' in
questa fase del prototipo non si sta ancora correggendo la frequenza di
risonanza. Prima di usare `ITANK_MOD` per rettifica sincrona resta necessaria
la calibrazione di offset/scala sulle schede TI modificate.

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

## Test 17 - UART XDS110 e Parser WLESS

Data:
2026-07-15

Obiettivo:
verificare sull'hardware reale il collegamento seriale SCIA tramite la porta
USB-C della controlCARD, la ricezione interrupt-driven, il ring buffer e
l'interpretazione dei comandi UART WLESS.

Operazioni preliminari:

- collegare il cavo USB-C alla controlCARD;
- programmare il firmware e assicurarsi che sia in esecuzione;
- impostare la posizione 2 dello switch S1 su `ON` se si vuole utilizzare la
  UART integrata nel collegamento USB-C/XDS110;
- individuare in Windows la porta `XDS110 Class Application/User UART`;
- verificare che la porta non sia occupata da CCS, terminali seriali o altri
  programmi.

Motivazione della posizione di S1.2, verificata sullo schema della
TMDSCNCD280039C Rev. B:

```text
S1.2 ON  -> GPIO28 collegato all'adattatore USB-to-UART dell'XDS110
S1.2 OFF -> GPIO28 instradato verso il connettore HSEC
```

Configurazione usata:

```text
Porta: COM16 - XDS110 Class Application/User UART
Baud rate: 115200
Formato: 8 data bit, no parity, 1 stop bit
Handshake: nessuno
WLESS_UART_ENABLE = 1
WLESS_SM_ENABLE = 0
```

Prima verifica:
con `S1.2 = OFF`, COM16 e COM17 risultavano apribili ma non veniva ricevuto
alcun byte dopo l'invio ripetuto di `WH?`, sia con terminazione CR sia con LF.

Controprova:
dopo aver impostato `S1.2 = ON`, COM16 ha risposto immediatamente ai comandi.
La prima query ha prodotto echo, risposta `WH=2880` e messaggio diagnostico
completo dello stato WLESS.

Suite automatica eseguita:

```text
Query WH                 PASS
Echo comando             PASS
Parser case-insensitive  PASS
Composizione con ';'     PASS
Comando non valido       PASS
Scrittura/rilettura WH   PASS
Ripristino WH=2880       PASS
```

Comandi significativi usati:

```text
WH?
wh=4000;wh?
BADCOMMAND
WH=2880
WH?
```

Il comando non valido ha restituito `Wrong command`. Al termine della prova il
valore di energia e' stato ripristinato a `WH=2880` e verificato con una nuova
query.

### Test 17.1 - Robustezza UART e Recupero Parser

Obiettivo:
verificare il comportamento ai limiti della lunghezza comando, con terminazioni
miste, burst di comandi, saturazione intenzionale del ring buffer e traffico
ricevuto con baud rate errato.

Risultati:

```text
Baseline WH?                         PASS
Comando di 15 caratteri              PASS
Comando di 16 caratteri              SCARTATO
Recupero dopo comando troppo lungo   PASS
CR/LF misti e righe vuote            PASS
Burst di 8 comandi WH?               PASS, 8/8 risposte
Burst di 40 comandi WH?              OVERFLOW, 16-21/40 risposte
Recupero dopo overflow con CR         PASS
Recupero dopo traffico a 9600 baud    PASS con risincronizzazione CR
Ripristino e verifica WH=2880         PASS
```

Il limite configurato e' `CMD_MAX_LEN = 16`: sono quindi disponibili 15
caratteri utili piu' il terminatore interno. Un comando ricevuto di 16
caratteri viene scartato senza risposta testuale; una successiva query valida
viene elaborata dopo l'invio di un delimitatore CR di risincronizzazione.

Nel burst di 8 comandi, contenuto entro la capacita' del ring buffer, sono
state ricevute tutte le 8 risposte. Nel burst intenzionale di 40 comandi il
ring buffer si satura e parte dei comandi viene persa. Due esecuzioni hanno
prodotto rispettivamente 21 e 16 risposte su 40; il numero dipende dalla
temporizzazione tra ricezione interrupt e trasmissione bloccante degli output.

Dopo l'overflow, una query inviata immediatamente puo' concatenarsi a un
frammento rimasto nel parser e non essere riconosciuta. La sequenza di
risincronizzazione usata e' stata:

```text
<CR><CR>WH?<CR>
```

Questa sequenza ha ripristinato la comunicazione sia dopo la saturazione del
ring buffer sia dopo l'invio intenzionale di traffico a 9600 baud. Non sono
stati osservati blocchi permanenti della UART o del parser.

Valutazione:
il comportamento nominale e il recupero sono verificati. La perdita di dati
sotto sovraccarico e l'assenza di una risposta esplicita per comando troppo
lungo sono limiti noti dell'implementazione fedele all'originale. Prima di
considerare l'interfaccia robusta rispetto a traffico seriale arbitrario e'
consigliabile aggiungere uno stato esplicito di discard-until-delimiter e una
risposta diagnostica di overflow.

### Test 17.2 - Comandi Originali con State Machine Disabilitata

Obiettivo:
verificare tramite COM16 il riconoscimento di tutti i comandi provenienti dal
parser originale, mantenendo `WLESS_SM_ENABLE = 0` e
`WLESS_SM_POWER_CONTROL_ENABLE = 0`.

La configurazione compilata e programmata e' il ramo VEHICLE:

```text
WLESS_SM_BUILD_VEHICLE = 1
```

Risultati:

```text
Comando   Echo   Parser   Effetto osservabile via UART
WH=3200   PASS   PASS     Energy[Wh]=3200
WH?       PASS   PASS     WH=3200
VB=400    PASS   PASS     non esposto nello status corrente
IB=1200   PASS   PASS     non esposto nello status corrente
IC=350    PASS   PASS     non esposto nello status corrente
SOURCE    PASS   PASS     Role=SOURCE
LOAD      PASS   PASS     Role=LOAD
AUTO      PASS   PASS     opMode non esposto nello status corrente
MANUAL    PASS   PASS     opMode non esposto nello status corrente
STOP      PASS   PASS     stopCommand non esposto nello status corrente
INITOK    PASS   PASS     initOkCommand non esposto nello status corrente
WH=2880   PASS   PASS     Energy[Wh]=2880
WH?       PASS   PASS     WH=2880
```

Nessuno dei comandi validi ha prodotto `Wrong command`. Per ogni comando sono
stati ricevuti l'echo e il messaggio diagnostico previsto.

Tracciabilita' delle conclusioni:

- per `WH`, `SOURCE` e `LOAD`, l'effetto sulle variabili e' osservabile nella
  risposta UART ed e' quindi verificato end-to-end;
- per `VB`, `IB`, `IC`, `AUTO`, `MANUAL`, `STOP` e `INITOK`, il test verifica
  il riconoscimento del parser ma non permette di osservare via UART il valore
  interno assegnato;
- con state machine disabilitata non sono attese ne' verificate transizioni di
  stato conseguenti ai comandi;
- con power control disabilitato nessun comando del test puo' abilitare lo
  stadio di potenza.

Al termine della prova l'energia e' stata ripristinata e verificata a
`WH=2880`. Restano in RAM il ruolo `LOAD` e i flag impostati da `STOP` e
`INITOK`; prima del successivo test funzionale e' necessario eseguire un reset
della controlCARD o l'inizializzazione completa della state machine.

Verifica dopo riavvio CPU:

```text
WH=2880
VH State=STANDBY
Role=NOTHING
CtrlState=IDLE
Pwr2Ld=0
IcoilErr=0
AbortState=DISABLED
```

Il ruolo `LOAD` lasciato dalla prova non era piu' presente, confermando la
reinizializzazione dello stato osservabile. Con `WLESS_SM_ENABLE = 0`, lo
stesso messaggio riportava tuttavia:

```text
WPTState=RUN
RadioLink=OK
```

Queste indicazioni non dimostrano che il trasferimento WPT o il collegamento
radio siano realmente attivi. Con la state machine esclusa, le relative
variabili conservano il valore numerico zero ottenuto dall'inizializzazione
BSS; le funzioni di formattazione UART associano attualmente tale valore alle
stringhe `RUN` e `OK`.

La diagnostica e' quindi potenzialmente fuorviante quando la FSM e'
disabilitata. La correzione suggerita, da valutare nel codice di competenza,
e' mostrare uno stato esplicito come `DISABLED` o `NOT_INITIALIZED` quando
`WLESS_SM_ENABLE = 0`. Non e' stata apportata alcuna modifica funzionale
nell'ambito di questo test.

Esito:
riconoscimento di tutti i comandi originali OK. Effetto interno verificato
soltanto per i campi attualmente esposti dalla diagnostica UART.

### Test 17.3 - Confronto Parser VEHICLE/STATION su Due Schede

Obiettivo:
verificare contemporaneamente le due varianti compile-time del parser con
state machine e controllo di potenza disabilitati, confermando in particolare
che il ramo STATION mantenga il comportamento slave e non accetti selezioni
locali SOURCE/LOAD.

Configurazione hardware e porte UART identificate automaticamente tramite la
firma del messaggio di stato:

```text
COM16 -> STATION, risposta "ST State"
COM23 -> VEHICLE, risposta "VH State"
115200 baud, 8N1, handshake assente
S1.2 ON su entrambe le controlCARD
```

Configurazione firmware comune:

```text
WLESS_SM_ENABLE = 0
WLESS_SM_POWER_CONTROL_ENABLE = 0
WLESS_UART_ENABLE = 1
```

Risultati:

```text
STATION WH=3600                  PASS
VEHICLE rimane WH=2880          PASS, isolamento dati
VEHICLE WH=2400                 PASS
STATION rimane WH=3600          PASS, isolamento dati
STATION SOURCE                  PASS, Role resta NOTHING
VEHICLE SOURCE                  PASS, Role diventa SOURCE
STATION LOAD                    PASS, Role resta NOTHING
VEHICLE LOAD                    PASS, Role diventa LOAD
VB=400 su entrambe              PASS parser/echo
IB=1200 su entrambe             PASS parser/echo
IC=350 su entrambe              PASS parser/echo
AUTO su entrambe                PASS parser/echo
MANUAL su entrambe              PASS parser/echo
STOP su entrambe                PASS parser/echo
INITOK su entrambe              PASS parser/echo
Ripristino WH=2880 entrambe     PASS
```

Conclusioni verificate:

- le due UART operano contemporaneamente e in modo indipendente;
- i valori `WH` modificati su una scheda non alterano l'altra;
- nella build VEHICLE, `SOURCE` e `LOAD` modificano il ruolo locale;
- nella build STATION, gli stessi comandi sono riconosciuti dal parser ma non
  modificano il ruolo, coerentemente con la scelta architetturale slave;
- tutti gli altri comandi originali sono riconosciuti ed ecoati da entrambe le
  build.

Limite di osservabilita':
lo status UART corrente non espone `VB`, `IB`, `IC`, operation mode,
`stopCommand` e `initOkCommand`. Per questi campi il test comparativo dimostra
il riconoscimento del comando, non l'assegnazione interna. L'estensione
diagnostica UART prevista permettera' di completare la verifica end-to-end
senza dipendere dalle watch expression di CCS.

Stato residuo dopo la prova:
l'energia e' stata ripristinata e verificata a `WH=2880` su entrambe le
schede. Il VEHICLE conserva in RAM `Role=LOAD`; i comandi `STOP` e `INITOK`
possono lasciare flag latched non osservabili. E' richiesto il reset di
entrambe le controlCARD prima del successivo test funzionale.

Esito: OK per il confronto parser VEHICLE/STATION e per il vincolo slave del
ramo STATION.

Limiti della verifica:
con `WLESS_SM_ENABLE = 0` il test valida trasporto UART, ISR RX, ring buffer,
parser, echo e notifica diagnostica, ma non valida ancora il tick o le
transizioni temporizzate della state machine.

Esito: OK con limitazioni di robustezza documentate.

## Test 18 - Avvio con WLESS_SM_ENABLE e Controprova Rollback

Obiettivo:
verificare l'avvio autonomo delle build VEHICLE e STATION dopo l'abilitazione
della state machine, mantenendo escluso il controllo dello stadio di potenza.

Configurazione della prima prova:

```text
WLESS_SM_ENABLE = 1
WLESS_SM_POWER_CONTROL_ENABLE = 0
VEHICLE: WLESS_SM_BUILD_VEHICLE = 1
STATION: WLESS_SM_BUILD_VEHICLE = 0
```

Risultato della prima prova:
dopo flash e power cycle nessuna delle due schede mostrava il normale LED
pulsante di esecuzione e nessuna UART rispondeva a `WH?`. L'assorbimento
osservato era sceso dal valore tipico di circa 0.20 A a circa 0.175 A.

La posizione S1.1 e' stata mantenuta `ON`, necessaria per utilizzare
l'emulatore XDS110 integrato per flash e debug. La posizione S1.2 e' stata
mantenuta `ON` per collegare GPIO28/29 alla UART XDS110.

Controprova rollback:
sono stati ricompilati e programmati due firmware distinti mantenendo i ruoli
VEHICLE/STATION e modificando soltanto l'abilitazione della FSM:

```text
WLESS_SM_ENABLE = 0
WLESS_SM_POWER_CONTROL_ENABLE = 0
```

Risultato dopo flash e riavvio:

```text
COM25 -> VEHICLE, risposta "VH State", WH=2880, Role=NOTHING
COM26 -> STATION, risposta "ST State", WH=2880, Role=NOTHING
LED di esecuzione nuovamente attivo
Assorbimento nuovamente circa 0.20 A
```

Conclusione verificata:
il problema di avvio si manifesta su entrambe le varianti quando
`WLESS_SM_ENABLE = 1` e scompare su entrambe quando la macro viene riportata a
zero. Il test esclude come causa specifica la sola selezione compile-time
VEHICLE/STATION.

Conclusione non ancora verificata:
la prova non identifica ancora l'istruzione o la risorsa interna alla FSM che
arresta o impedisce il normale avvio. Per localizzare la causa e' necessario
avviare una build con FSM abilitata sotto debugger oppure introdurre marker di
avanzamento prima e dopo `WLESS_SM_init()`.

Esito: FAIL con `WLESS_SM_ENABLE = 1`; rollback PASS con
`WLESS_SM_ENABLE = 0`.

## Test 19 - Sezionamento avvio FSM VEHICLE

Obiettivo:
localizzare il blocco osservato nel Test 18 abilitando progressivamente init,
tick e porzioni della FSM VEHICLE, con controllo potenza sempre escluso.

Hardware e strumenti:

```text
controlCARD TMDSCNCD280039C su docking station standard
UART XDS110: COM25, 115200 8N1
Indicatore esecuzione: LED D2 della controlCARD
Build: gmake TI C2000
Flash: DSLite, probe seriale VEHI0001
Test UART: PowerShell / .NET System.IO.Ports.SerialPort
```

Configurazione comune:

```text
WLESS_SM_ENABLE = 1
WLESS_SM_POWER_CONTROL_ENABLE = 0
WLESS_SM_BUILD_VEHICLE = 1
```

Risultati verificati in sequenza:

```text
WLESS_SM_init() soltanto                         PASS, LED/assorbimento regolari
init + hook tick decimato ISR2                   PASS, LED/assorbimento regolari
init + ISR2 + WLESS_SM_servicePendingTicks()     PASS, LED/assorbimento regolari
WLESS_SM_run() senza step specifico VEHICLE      PASS, D2 e UART regolari
sola logica VEHICLE STANDBY                      PASS, D2 e UART regolari
switch VEHICLE fino a INIT_LOAD                  PASS, D2 e UART regolari
switch VEHICLE fino a PRECHARGE_LOAD             PASS, D2 e UART regolari
switch VEHICLE fino a DISCHARGE_LOAD             PASS, D2 e UART regolari
switch VEHICLE completo                          PASS, D2 e UART regolari
```

Risposta UART verificata nelle prove su docking:

```text
WH?
WH=2880
VH State=STANDBY, Energy[Wh]=2880, Role=NOTHING, WPTState=STOP,
CtrlState=IDLE, Pwr2Ld=0, IcoilErr=0, AbortState=DISABLED, RadioLink=OK
```

Nota di tracciabilita': nelle prime tre prove il riferimento visivo era una
scheda precedentemente collegata; nelle prove su docking il LED osservato e'
stato identificato esplicitamente come D2 della controlCARD. La UART e' stata
verificata automaticamente nelle prove su docking; il LED e' stato verificato
dall'operatore.

Conclusione parziale verificata:
il blocco non e' causato da init, hook ISR2, servizio tick, dispatcher di
`WLESS_SM_run()` o dalla semplice inclusione/esecuzione del ramo VEHICLE. Il
firmware completo e' stato verificato funzionante sulla docking standard.

Conclusione aggiornata:
l'anomalia del Test 18 non e' stata riprodotta sulla docking standard. Poiche'
il fallimento originale era stato osservato con la controlCARD montata sulla
scheda TI modificata, e' necessaria una controprova A/B usando il medesimo
file `.out` e la medesima controlCARD, cambiando soltanto la carrier. Una
dipendenza dalla carrier e' al momento un'inferenza, non ancora un fatto
verificato.

Esito: PASS su docking standard; controprova su carrier modificata PENDING.

## Test 20 - Avvio FSM STATION completa su docking

Obiettivo:
verificare l'avvio della FSM completa nel ruolo STATION sulla medesima
controlCARD e docking usate per il retest VEHICLE.

Configurazione:

```text
WLESS_SM_ENABLE = 1
WLESS_SM_POWER_CONTROL_ENABLE = 0
WLESS_SM_BUILD_VEHICLE = 0
Tutte le macro diagnostiche di isolamento = 0
```

Strumenti:

```text
Build: gmake TI C2000
Flash: DSLite, probe VEHI0001
UART: PowerShell / .NET System.IO.Ports.SerialPort, COM25 115200 8N1
Indicatore esecuzione: LED D2 della controlCARD
```

Risultati verificati:

```text
Build e link                 PASS
Flash DSLite                 PASS, messaggio finale "Running... Success"
UART WH?                     PASS
Identificazione ruolo        PASS, prefisso "ST State"
Stato iniziale               PASS, STANDBY
LED D2                       PASS, lampeggiante secondo osservazione operatore
```

Risposta UART:

```text
WH?
WH=2880
ST State=STANDBY, Energy[Wh]=2880, Role=NOTHING, WPTState=STOP,
CtrlState=IDLE, Pwr2Ld=0, IcoilErr=0, AbortState=DISABLED, RadioLink=OK
```

Conclusione verificata:
le FSM complete VEHICLE e STATION avviano e mantengono operativo il firmware
sulla docking standard. L'anomalia osservata nel Test 18 non e' riprodotta in
questa configurazione hardware.

Esito: PASS.

## Test 21 - Controprova STATION sulla scheda TI modificata

Obiettivo:
verificare la dipendenza dalla carrier spostando la stessa controlCARD con lo
stesso firmware STATION del Test 20, senza rebuild e senza nuovo flash.

Condizioni controllate:

```text
controlCARD invariata
firmware invariato: STATION completa, power control disabilitato
nessun nuovo flash
carrier cambiata: docking standard -> scheda TI modificata, posizione scheda 2
alimentazione logica carrier: 12 V
```

Configurazione fisica effettiva durante la controprova:

```text
Scheda TI modificata, posizione 2: controlCARD/probe VEHI0001,
                                        firmware STATION completo
Scheda TI modificata, posizione 1: controlCARD/probe STAT0001,
                                        firmware non riverificato in questa prova
```

Gli identificatori `VEHI0001` e `STAT0001` identificano i probe/controlCARD e
non garantiscono il ruolo del firmware correntemente programmato. In questa
prova `VEHI0001` conteneva certamente il firmware STATION.

Risultati osservati:

```text
Docking standard: D2 lampeggiante, UART COM25 operativa, assorbimento di
riferimento su carrier modificata nelle condizioni normali circa 0,20 A
Scheda TI modificata: entrambe le posizioni presentavano D2 spento e
assorbimento 0,17 A ciascuna
Due controlCARD contemporaneamente collegate: COM25 e COM26; nessuna risposta
a WH? su entrambe. La prova non ha identificato l'associazione ID probe/COM.
```

Strumento UART:
PowerShell / .NET `System.IO.Ports.SerialPort`, 115200 8N1.

Conclusione verificata:
l'anomalia dipende dalla configurazione hardware sulla scheda modificata e non
dal solo file firmware STATION o dalla sola controlCARD `VEHI0001`. Il firmware
STATION completo verificato sulla docking non raggiunge il normale loop
applicativo quando la stessa controlCARD viene inserita nella posizione 2 con
entrambe le posizioni della scheda modificata popolate/alimentate.

Cause non ancora determinate:
stato di reset, tensioni locali, selezione boot, clock, segnali GPIO imposti
dalla carrier o interazione tra le due posizioni contemporaneamente popolate.
Nessuna di queste cause e' ancora verificata. La prova non isola ancora la
sola posizione 2 dalla presenza della controlCARD in posizione 1.

Esito: FAIL sulla scheda modificata; dipendenza dalla carrier CONFERMATA.

## Test 22 - Cleanup asset e avvio tramite DSLite sulla scheda modificata

Obiettivo:
ripristinare la corrispondenza tra identificatore della controlCARD, posizione
fisica e ruolo firmware, quindi verificare l'avvio immediatamente dopo flash.

Configurazione ripristinata:

```text
Scheda modificata 1: VEHI0001, firmware VEHICLE completo, COM25
Scheda modificata 2: STAT0001, firmware STATION completo, COM26
WLESS_SM_ENABLE = 1
WLESS_SM_POWER_CONTROL_ENABLE = 0
```

Procedura:
entrambe le controlCARD sono state programmate direttamente tramite ID probe
con DSLite. Entrambi i caricamenti hanno concluso con `Running... Success`.

Risultati immediatamente dopo flash:

```text
COM25: VH State=STANDBY, RadioLink=OK                 PASS
COM26: ST State=STANDBY, RadioLink=OK                 PASS
D2 controlCARD posizione 1                           lampeggiante
D2 controlCARD posizione 2                           lampeggiante
Assorbimento logica posizione 1                      normale (~0,20 A)
Assorbimento logica posizione 2                      normale (~0,20 A)
```

Conclusione parziale verificata:
la carrier modificata non impedisce permanentemente l'esecuzione dei firmware.
Entrambi i ruoli funzionano quando DSLite programma, imposta il PC all'entry
point e avvia la CPU. Resta da verificare il boot autonomo dopo power cycle,
senza intervento del debugger.

Esito: PASS dopo avvio DSLite; power-cycle test PENDING.

### Power cycle autonomo successivo al cleanup

Procedura:
spegnimento contemporaneo delle alimentazioni logiche a 12 V, attesa di circa
5 secondi e riaccensione senza CCS, DSLite o sessione di debug.

Risultati verificati:

```text
Assorbimento logica posizione 1                      normale (~0,20 A)
Assorbimento logica posizione 2                      normale (~0,20 A)
D2 entrambe le controlCARD                           lampeggiante
COM25: VH State=STANDBY, RadioLink=OK                 PASS
COM26: ST State=STANDBY, RadioLink=OK                 PASS
```

Conclusione verificata:
il boot autonomo da Flash funziona su entrambe le posizioni della scheda
modificata dopo il ripristino coerente dell'asset e la riprogrammazione dei due
firmware tramite ID probe.

Conclusione non verificata:
la causa precisa della precedente condizione con D2 spenti e assorbimento
0,17 A non e' stata identificata. I dati disponibili non consentono di
attribuirla con certezza allo scambio delle controlCARD, al firmware presente
prima del cleanup, alla sequenza di flash/reset o a un'altra condizione
transitoria.

Esito finale Test 22: PASS, incluso boot autonomo dopo power cycle.

## Test 23 - Transizioni FSM VEHICLE via UART, senza radio

Obiettivo:
verificare transizioni funzionali della FSM completa tramite i comandi UART,
con nRF24 non integrato e controllo dello stadio di potenza disabilitato.

Configurazione:

```text
COM25: VEHICLE completo
COM26: STATION completa
WLESS_SM_POWER_CONTROL_ENABLE = 0
Separatore parser per comandi multipli: ';'
```

Sequenza e risultati:

```text
MANUAL SOURCE       rifiutato: "Wrong command"; nessuna transizione
MANUAL;SOURCE       riconosciuto
                    STANDBY -> INIT_SOURCE dopo lo step FSM
                    Role=SOURCE, CtrlState=INIT
INITOK              riconosciuto
                    INIT_SOURCE -> WAIT_LOAD dopo lo step FSM
                    Role=SOURCE, CtrlState=INITOK
WH? su COM26        STATION resta STANDBY, Role=NOTHING, CtrlState=IDLE
```

Strumento:
PowerShell / .NET `System.IO.Ports.SerialPort`, con attesa di 1,6 s tra
comando e lettura per includere almeno uno step FSM nominale da 1 s.

Conclusioni verificate:

- il parser richiede `;` e non lo spazio come separatore dei token;
- la FSM VEHICLE esegue le transizioni temporizzate iniziali del ramo SOURCE;
- `INITOK` viene consumato dalla FSM e produce lo stato atteso;
- la STATION rimane passiva in assenza di dati remoti, coerentemente con il
  ruolo slave;
- in `WAIT_LOAD` il VEHICLE attende `remoteCtrlState=INITOK`, dato che il
  parser attuale non espone e che dovra' arrivare dal collegamento radio o da
  un futuro comando diagnostico esplicito.

Esito: PASS fino a WAIT_LOAD; avanzamento successivo BLOCKED BY DESIGN in
assenza di trasporto nRF24/simulazione del peer.

### Ramo LOAD e reset reale della controlCARD

Un primo power cycle delle sole alimentazioni logiche a 12 V, con USB-C ancora
collegati, non ha resettato il VEHICLE: COM25 riportava ancora `WAIT_LOAD`.
La controlCARD rimane alimentata tramite il collegamento USB-C/XDS110; pertanto
il solo ciclo dei 12 V della carrier non equivale a un reset completo.

Dopo rimozione USB-C, power cycle della carrier e successiva riconnessione
USB-C, entrambe le FSM sono tornate in `STANDBY`.

Sequenza ramo LOAD:

```text
MANUAL;LOAD         STANDBY -> INIT_LOAD
                    Role=LOAD, CtrlState=INIT
INITOK              INIT_LOAD -> WAIT_SOURCE
                    Role=LOAD, CtrlState=INITOK
```

Esito ramo LOAD: PASS fino a WAIT_SOURCE.

## Test 24 - Dipendenza dell'avvio dalla sequenza USB-C

Osservazione ripetuta dall'operatore per tre prove:

```text
USB-C gia' collegato durante l'accensione della scheda modificata:
    avvio/alimentazione anomala, D2 spento

Scheda modificata avviata senza USB-C, collegamento USB-C effettuato dopo:
    avvio regolare, D2 lampeggiante, UART operativa
```

Conclusione verificata sul banco:
l'avvio delle controlCARD sulla scheda TI modificata dipende dalla sequenza di
connessione USB-C. Per le prove successive la procedura preliminare obbligatoria
e':

1. scollegare gli USB-C;
2. accendere/alimentare la scheda modificata;
3. attendere l'avvio delle controlCARD;
4. collegare gli USB-C per UART/XDS110.

Meccanismo non ancora verificato:
non e' stato determinato quale percorso di alimentazione, reset o back-power
attraverso USB-C/XDS110 produca la condizione anomala.

Esito: comportamento RIPRODOTTO 3/3; workaround operativo CONFERMATO.

## Test 25 - Parser collegato alle variabili WLESS_SM_*

Obiettivo:
verificare end-to-end che i comandi UART aggiornino le corrispondenti variabili
`WLESS_SM_*`, superando il precedente limite di osservabilita' dello status.

Estensione diagnostica usata:
e' stato aggiunto il comando read-only `VARS?`, che espone senza modificare la
FSM:

```text
WH, VB, IB, IC, MODE, STOP, INITOK
```

Hardware e strumento:

```text
VEHICLE VEHI0001, COM25, FSM completa, power control disabilitato
PowerShell / .NET System.IO.Ports.SerialPort, 115200 8N1
```

Baseline dopo flash:

```text
VARS WH=2880, VB=0, IB=0, IC=0, MODE=AUTO, STOP=0, INITOK=0
```

Matrice verificata:

```text
VB=400             -> WLESS_SM_vBus_V=400                 PASS
IB=1200            -> WLESS_SM_iBat_mA=1200               PASS
IC=350             -> WLESS_SM_iCoil_mA=350               PASS
WH=3200            -> decode(localEnergyEncoded)=3200     PASS
AUTO               -> WLESS_SM_opMode=AUTO                PASS
MANUAL             -> WLESS_SM_opMode=MANUAL              PASS
STOP               -> WLESS_SM_stopCommand=1              PASS
INITOK             -> WLESS_SM_initOkCommand=1            PASS
WH=2880 ripristino -> decode(localEnergyEncoded)=2880     PASS
SOURCE/LOAD        -> gia' verificati nel Test 23 tramite ruolo e transizioni
```

Osservazione verificata:
se ricevuti in `STANDBY`, `STOP` e `INITOK` restano latched a 1 perche' in tale
stato la FSM non li consuma. Il test dimostra il comportamento ma non stabilisce
se sia conforme ai requisiti; prima di riusare la scheda e' necessario un reset
per eliminare i flag di test.

Conclusione:
tutti i comandi originali che scrivono variabili `WLESS_SM_*` sono ora
verificati end-to-end. Il comando `VARS?` colma il limite diagnostico che
impediva di distinguere il semplice riconoscimento del token dall'assegnazione
effettiva.

Esito: PASS, con nota sui flag latched fuori contesto.

## Test 26 - Bring-up SPI/nRF24 del porting

Obiettivo:
verificare tramite UART la lettura dei registri nRF24 dopo il porting del
driver e della configurazione SPIA/GPIO/XINT1.

Configurazione:

```text
WLESS_NRF24_ENABLE=1
WLESS_SM_ENABLE=1
WLESS_SM_POWER_CONTROL_ENABLE=0
UART 115200 8N1, comando diagnostico read-only RADIO?
```

Risultati osservati:

```text
VEHICLE / scheda modificata 1:
RADIO EN=1, INIT=0, STATUS=255, CONFIG=255, FIFO=255, IRQ=0

STATION / scheda modificata 2 / STAT0001 / COM26:
RADIO EN=1, INIT=0, STATUS=14, CONFIG=59, FIFO=17, IRQ=0
ST State=STANDBY ... RadioLink=OK
```

Interpretazione:

- STATION legge valori nRF24 coerenti (`STATUS=0x0E`, `CONFIG=0x3B`,
  `FIFO_STATUS=0x11`): collegamento SPI e transceiver risultano operativi sul
  percorso della scheda 2;
- `INIT=0` nello status STATION non viene usato da solo per dichiarare il test
  fallito, perche' le letture dei registri sono valide e lo status applicativo
  riporta `RadioLink=OK`; resta da verificare la semantica esatta del campo
  diagnostico `INIT`;
- le letture `0xFF` del VEHICLE indicano MISO alto o assenza di risposta sul
  percorso della scheda 1, ma non ne identificano ancora la causa.

Conclusione verificata:
il porting non presenta un guasto generale della comunicazione SPI/nRF24,
poiche' la stessa build di integrazione ottiene letture valide sul ramo
STATION. Tuttavia `INIT=0` rende il bring-up STATION solo parzialmente superato:
almeno una verifica di inizializzazione non coincide col valore atteso. La
pendenza principale e' circoscritta alla scheda 1/VEHICLE oppure a una
differenza specifica del ruolo. Il successivo Test 27 esegue il confronto A/B
col progetto standalone originale.

Esito: PARTIAL PASS su STATION; FAIL su VEHICLE; causa del FAIL VEHICLE non
ancora determinata in questo test.

## Test 27 - A/B con progetto nRF24 standalone originale

Obiettivo:
distinguere un difetto generale del porting da un problema del percorso
hardware VEHICLE, eseguendo sulla stessa scheda 1 il progetto originale noto
come funzionante in precedenza.

Verifiche preliminari:

- binario: `nRF24L01plus/CPU1_FLASH/nRF24L01plus.out`;
- ruolo compilato: `VEHICLE`, verificato dalla macro in
  `targetConfigs/Global_Variables.h`;
- nessun pilotaggio PWM/potenza individuato nell'ispezione statica; il progetto
  standalone usa UART, SPI/nRF24, timer e GPIO diagnostici.

Procedura:

1. lasciati invariati scheda modificata 1, controlCARD `VEHI0001`, cablaggio e
   transceiver;
2. programmato il binario FLASH originale tramite DSLite;
3. avviato il firmware e osservata COM25 a 115200 8N1.

Risultato UART spontaneo, ripetuto continuamente:

```text
Diagnostic radio initialization failure
```

Conclusione:
anche il progetto originale fallisce sul medesimo percorso VEHICLE, mentre il
porting ottiene registri nRF24 validi sulla scheda 2/STATION. Questo rende molto
probabile un problema hardware/localizzato sul percorso della scheda 1
(alimentazione modulo, transceiver, contatti/cablaggio o linee SPI/CE/CSN), e
non un guasto generale del porting.

Incertezza residua:
non e' ancora esclusa una differenza specifica del ruolo VEHICLE comune alle
due implementazioni. Per discriminare definitivamente occorrono misure sulle
linee della scheda 1 oppure uno scambio controllato del solo modulo/cablaggio
nRF24 tra i due rami.

Confidenza della conclusione hardware/localizzata: alta.

Nota di ripristino:
al termine del test su `VEHI0001` resta installato il firmware standalone
originale; prima di riprendere i test integrati deve essere riprogrammato il
firmware VEHICLE del progetto `WLESS_CHG_F28003x`.

Esito: FAIL riprodotto anche col progetto originale; problema circoscritto al
percorso VEHICLE/scheda 1.

## Test 28 - Firmware STATION eseguito sulla scheda 1

Obiettivo:
escludere che il fallimento nRF24 sulla scheda 1 sia intrinseco alla variante
firmware `VEHICLE`.

Procedura:

1. programmato temporaneamente su scheda modificata 1 / `VEHI0001` il binario
   integrato `WLESS_CHG_F28003x_STATION_NRF24_SPI_BRINGUP_PWR_OFF.out`;
2. mantenuti invariati controlCARD, carrier, modulo nRF24 e cablaggio;
3. interrogata COM25 a 115200 8N1 col comando `RADIO?`;
4. al termine ripristinato il firmware integrato VEHICLE
   `WLESS_CHG_F28003x_VEHICLE_NRF24_SPI_PTE_ACTIVE_LOW_PWR_OFF.out`.

Risultato con firmware STATION sulla scheda 1:

```text
RADIO EN=1, INIT=0, STATUS=255, CONFIG=255, FIFO=255, IRQ=0
ST State=STANDBY ... RadioLink=OK
```

Conclusione verificata:
il fallimento resta sulla scheda/percorso fisico 1 anche cambiando il ruolo
firmware da VEHICLE a STATION. Il problema non e' quindi intrinseco alla
variante firmware VEHICLE.

Nota diagnostica:
il campo applicativo `RadioLink=OK` non e' attualmente sufficiente per valutare
il bring-up nRF24, perche' resta `OK` anche con `INIT=0` e registri a `0xFF`.
Per questo test fanno fede `INIT` e le letture dirette dei registri.

Confidenza della localizzazione hardware/percorso scheda 1: molto alta.

Esito: FAIL identico con firmware STATION; firmware VEHICLE ripristinato con
successo al termine della prova.

## Test 29 - Correzione inserimento transceiver scheda 1

Durante l'ispezione fisica e' stato rilevato che il transceiver nRF24 della
scheda modificata 1 era inserito storto nel relativo slot. Il modulo e' stato
rimontato correttamente senza sostituire transceiver, controlCARD o cablaggio.

Dopo il rimontaggio, col firmware VEHICLE gia' ripristinato, il comando
`RADIO?` su COM25 ha restituito:

```text
RADIO EN=1, INIT=1, STATUS=14, CONFIG=42, FIFO=17, IRQ=0
VH State=STANDBY ... RadioLink=OK
```

Interpretazione verificata:

- `INIT=1`: sequenza di inizializzazione completata;
- `STATUS=0x0E`: risposta SPI valida;
- `CONFIG=0x2A`: configurazione prevista per VEHICLE/TX;
- `FIFO_STATUS=0x11`: stato FIFO valido.

Conclusione definitiva:
il precedente errore sulla scheda 1 era causato dal montaggio scorretto del
transceiver nello slot. Non risultano responsabili il firmware VEHICLE, il
porting generale, il cablaggio fisso o il transceiver stesso.

Confidenza: molto alta, con verifica end-to-end mediante lettura registri e
`INIT=1`.

Esito: PASS. Causa del precedente FAIL identificata e rimossa.

## Test 30 - Radio su singola scheda VEHICLE

Obiettivo:
verificare inizializzazione nRF24 e stabilita' del collegamento SPI sulla sola
scheda VEHICLE, senza richiedere una seconda radio.

Estensione diagnostica:
il comando read-only `RADIO?` e' stato modificato affinche' legga dal dispositivo
i registri `STATUS`, `CONFIG` e `FIFO_STATUS` a ogni interrogazione, invece di
stampare solamente i valori memorizzati all'avvio. Ogni campione esercita quindi
tre transazioni SPI reali.

Durante la preparazione e' stata rilevata una criticita' di build:
il target `clean` non rimuoveva `wless_radio/wless_nrf24.obj`. Dopo un cambio di
`WLESS_SM_BUILD_VEHICLE`, il linker poteva pertanto riutilizzare l'oggetto radio
del ruolo precedente. La prima serie di 50 letture, pur stabile, ha infatti
mostrato `CONFIG=0x3B` (RX/STATION) in un build nominalmente VEHICLE.

Correzione applicata:
aggiunti gli artefatti `wless_radio` (`.obj`, `.d`, `.lst`, `.asm`) al target
`clean` di `RELEASE/makefile`. Una successiva clean build ha mostrato
esplicitamente la ricompilazione di `wless_nrf24.c`.

Configurazione finale:

```text
Scheda: modificata 1 / VEHI0001
Ruolo: VEHICLE
Firmware: WLESS_CHG_F28003x_VEHICLE_NRF24_SINGLE_BOARD_LIVE_QUERY_PWR_OFF.out
UART: COM25, 115200 8N1
Campioni: 50 RADIO? consecutivi
Transazioni SPI diagnostiche: 150
```

Risultato aggregato:

```text
Samples=50 Good=50 MissingOrUnparsed=0
50 x EN=1, INIT=1, STATUS=14, CONFIG=42, FIFO=17, IRQ=0
```

Conclusione verificata:
inizializzazione nRF24 e letture SPI live sono stabili sulla singola scheda
VEHICLE. Il test non verifica ancora trasmissione RF, ricezione payload o IRQ
radio, che richiedono l'integrazione delle primitive payload e una seconda
scheda.

Esito: PASS 50/50; nessun timeout; registri coerenti col ruolo VEHICLE.

## Test 31 - Comunicazione radio tra VEHICLE e STATION

Obiettivo:
verificare lo scambio RF bidirezionale fra le due schede mantenendo la STATION
nel ruolo slave: VEHICLE interroga, STATION risponde esclusivamente mediante
ACK payload Enhanced ShockBurst.

Diagnostica implementata:

- payload di test da 9 byte con magic, sequenza e checksum;
- comando UART `RADIOPING`, accettato soltanto dal firmware VEHICLE;
- STATION precarica un `W_ACK_PAYLOAD` e lo aggiorna dopo ogni richiesta valida;
- `RADIO?` espone `TX`, `RX`, `ACK`, `MAXRT`, `TXSEQ` e `RXSEQ`.

Configurazione:

```text
VEHICLE: scheda modificata 1 / VEHI0001 / COM25
STATION: scheda modificata 2 / STAT0001 / COM26
WLESS_SM_ENABLE=1
WLESS_SM_POWER_CONTROL_ENABLE=0
```

Prima prova, 10 richieste:

```text
VEHICLE TXSEQ=10, RXSEQ=9, MAXRT=0
STATION RXSEQ=10
```

La prima prova ha verificato lo scambio RF e la semantica slave della STATION.
L'ACK iniziale e' precaricato; gli ACK successivi riportano la sequenza della
richiesta precedente.

Prova estesa iniziale, ulteriori 100 comandi:
STATION ha raggiunto `RXSEQ=110`, mentre VEHICLE e' rimasta temporaneamente a
`RXSEQ=108`, con RX FIFO non vuota (`FIFO_STATUS=0x10`). La condizione e' rimasta
invariata dopo due secondi.

Causa software verificata:
il servizio portato leggeva un solo payload per evento IRQ, mentre il codice
originale svuota l'intera RX FIFO. Eventi ravvicinati potevano quindi lasciare
ACK accodati senza un nuovo fronte IRQ.

Correzione:
il servizio radio ora legge tutti i payload presenti fino a `RX_EMPTY`, con
limite di tre elementi pari alla profondita' hardware della FIFO. Entrambi i
ruoli sono stati ricompilati con clean build e riprogrammati.

Retest finale da contatori e sequenze azzerati:

```text
Comandi RADIOPING accettati: 100/100

VEHICLE:
INIT=1, STATUS=14, CONFIG=42, FIFO=17
IRQ=115, TX=115, ACK=115, MAXRT=0, TXSEQ=100, RXSEQ=100

STATION:
INIT=1, STATUS=14, CONFIG=59, FIFO=1
IRQ=116, TX=114, RX=116, MAXRT=0, RXSEQ=100
```

I contatori IRQ/TX/RX/ACK superiori al numero dei comandi includono eventi e
ripetizioni del protocollo Enhanced ShockBurst e non vengono interpretati come
packet count applicativo. La verifica applicativa usa sequenza e checksum:
entrambi i lati hanno validato la sequenza finale 100 e VEHICLE non ha rilevato
`MAX_RT`.

Durante tutta la prova entrambe le FSM sono rimaste in `STANDBY`; non sono stati
abilitati comandi di potenza.

Limite del test:
il payload usato e' diagnostico. Resta da collegare il payload operativo da 9
byte alle variabili `WLESS_SM_*` tramite il wrapper
`WLESS_SM_setPowerCommand` e verificare le transizioni FSM end-to-end.

Esito: PASS, 100/100 sequenze validate in entrambe le direzioni, `MAXRT=0`.

## Limiti ereditati dal driver nRF24 originale

I seguenti punti sono stati identificati durante il porting ma, per scelta di
perimetro, non vengono modificati: il mandato richiede di conservare le scelte
del firmware originale e non di introdurre miglioramenti progettuali.

### Lunghezza massima e FIFO SPI

Il codice ammette una lunghezza massima logica di 32 byte, mentre la FIFO SPI
hardware contiene 16 elementi. Il payload operativo originale e' lungo 9 byte
e non raggiunge il limite. Un eventuale supporto robusto a trasferimenti da 32
byte richiederebbe una modifica rispetto alla prima stesura.

Stato: limite noto, non corretto; irrilevante per il payload operativo da 9
byte attualmente previsto.

### Polling SPI senza timeout

La transazione SPI attende in modo bloccante il riempimento della RX FIFO senza
timeout, come il firmware originale. Un guasto periferico potrebbe quindi
arrestare il background.

Stato: limite noto, non corretto; l'aggiunta di un timeout costituirebbe una
modifica di robustezza rispetto all'implementazione originale.

## Test 32 - Payload operativo originale collegato alla FSM

Obiettivo:
portare e verificare il payload operativo originale da 9 byte, senza introdurre
modifiche al protocollo o alle scelte progettuali del firmware di origine.

Formato portato:

```text
byte 0..1  energia locale codificata, big endian
byte 2     ruolo locale
byte 3     stato controller locale
byte 4..5  potenza verso il carico, big endian
byte 6..7  correzione corrente bobina, big endian
byte 8     stato abort locale
```

Il payload ricevuto aggiorna rispettivamente:

```text
WLESS_SM_remoteEnergyEncoded
WLESS_SM_remoteRole
WLESS_SM_remoteCtrlState
WLESS_SM_powerToLoad
WLESS_SM_iCoilErr
WLESS_SM_remoteAbort
```

Gestione link mantenuta fedele all'originale:

- VEHICLE azzera `WLESS_SM_noAckCount` su ricezione ACK payload;
- VEHICLE incrementa il contatore, con saturazione a 15, su `MAX_RT`;
- STATION incrementa il contatore ogni secondo e lo azzera quando viene
  interrogata;
- `WLESS_SM_radioLink` continua a essere valutato dalla funzione FSM
  equivalente a `RadioLinkKO()`; non e' stato introdotto un nuovo criterio;
- STATION precarica e aggiorna l'ACK payload soltanto se la TX FIFO non e'
  piena, come nell'originale.

Diagnostica aggiunta, non parte del protocollo:
`RADIO?` espone `INITERR` e le variabili remote per consentire la verifica
end-to-end. `INITERR=0` significa che tutti i readback di inizializzazione sono
risultati conformi.

Sequenza di banco:

1. eseguite clean build separate VEHICLE e STATION;
2. programmati i due binari operativi sulle rispettive controlCARD;
3. eseguito power cycle completo scollegando anche USB-C;
4. dopo una sequenza di flash non simultanea e' stato necessario riallineare la
   STATION per cancellare IRQ/FIFO pendenti; i contatori `MAXRT` del VEHICLE
   conservano gli errori storici prodotti prima del riallineamento;
5. con collegamento stabilizzato, impostati `WH=3200` sul VEHICLE e `WH=4000`
   sulla STATION, quindi inviato `MANUAL;SOURCE` al VEHICLE.

Risultato finale VEHICLE:

```text
INIT=1, INITERR=0, CONFIG=42, FIFO=17
ACK=27, NOACK=0
REMOTE_E=19114, REMOTE_ROLE=2, REMOTE_CTRL=1, REMOTE_ABORT=0
State=INIT_SOURCE, local Role=SOURCE, local CtrlState=INIT
```

`REMOTE_E=19114` decodifica a 4000 Wh; il ruolo remoto 2 corrisponde a LOAD.

Risultato finale STATION:

```text
INIT=1, INITERR=0, CONFIG=59, FIFO=1
RX=28, NOACK=0, MAXRT=0
REMOTE_E=5461, REMOTE_ROLE=1, REMOTE_CTRL=1, REMOTE_ABORT=0
State=INIT_LOAD, local Role=LOAD, local CtrlState=INIT
```

`REMOTE_E=5461` decodifica a 3200 Wh; il ruolo remoto 1 corrisponde a SOURCE.
La STATION non trasmette autonomamente: risponde mediante ACK payload soltanto
dopo l'interrogazione VEHICLE.

Durante il test `WLESS_SM_POWER_CONTROL_ENABLE=0`; nessun comando e' stato
applicato alla sezione di potenza.

Esito: PASS. Payload operativo, interpretazione delle variabili, schema slave
STATION e gestione originale `noAckCount` verificati end-to-end.

## Test 33 - FSM distribuita con transizioni analogiche robuste

Obiettivo:
verificare la state machine distribuita VEHICLE/STATION con controllo di
potenza disabilitato e validare la modifica descritta in
`WLESS_SM_ANALOG_TRANSITION_ROBUSTNESS_CHANGE.md`.

Configurazione:

```text
WLESS_SM_ENABLE=1
WLESS_SM_POWER_CONTROL_ENABLE=0
WLESS_SM_ENERGY_HYSTERESIS_WH=50
WLESS_SM_ROLE_DEADBAND_WH=50
WLESS_SM_ANALOG_CONFIRM_SAMPLES=2
VEHICLE COM25, STATION COM26
```

### Build e regressione radio

Entrambi i ruoli sono stati prodotti mediante clean build. Durante la prima
prova distribuita e' stata individuata una regressione del porting IRQ:
l'originale leggeva STATUS direttamente nell'ISR, mentre il porting differiva
lo SPI al background usando soltanto il fronte XINT. Con IRQ rimasto basso non
veniva generato un nuovo fronte e il link si arrestava con STATUS/FIFO pendenti.

Correzione applicata:
il background serve la radio se il flag XINT e' pendente oppure se GPIO33/IRQ e'
ancora attivo basso. Protocollo e gestione degli eventi restano invariati.

Stress successivo di 45 secondi con messaggi operativi automatici:

```text
VEHICLE: ACK=86, NOACK=0, STATUS=0x0E, FIFO=0x11
STATION: RX=56, RadioLink=OK
```

Esito regressione radio: PASS; nessun nuovo blocco del link.

### Handshake distribuito

Stimoli:

```text
VEHICLE WH=3200, MANUAL;SOURCE
STATION WH=4000
INITOK inviato prima a STATION e poi a VEHICLE dopo INIT_LOAD/INIT_SOURCE
```

Sequenza verificata:

```text
VEHICLE: STANDBY -> INIT_SOURCE -> WAIT_LOAD -> PRECHARGE_SOURCE
STATION: STANDBY -> INIT_LOAD   -> WAIT_SOURCE -> PRECHARGE_LOAD
```

Ruoli e stati controller ricevuti via payload sono risultati coerenti.

### Conferma Vbus

Il campo diagnostico `S` e' stato usato per sincronizzare lo stimolo coi passi
FSM reali.

Impulso singolo VEHICLE:

```text
step 63: VB=60 applicato
step 64: un campione sopra soglia
step 64: VB=0 ripristinato
step 66: stato ancora PRECHARGE_SOURCE
```

Applicazione stabile `VB=60` su entrambe le schede:

```text
VEHICLE -> SOURCE_ON
STATION -> LOAD_ON
```

Esito: PASS; impulso singolo rifiutato, due conferme accettate.

### Conferma energia

In `SOURCE_ON`, con carico remoto a 4000 Wh:

```text
step singolo con sorgente a 2800 Wh -> resta SOURCE_ON
ripristino a 3200 Wh                -> conteggio azzerato
2800 Wh stabile per due step        -> DISCHARGE_SOURCE
```

Esito: PASS.

### Conferma Ibat

In `DISCHARGE_SOURCE`:

```text
un solo step con IB=0 mA, poi IB=100 mA -> resta DISCHARGE_SOURCE
IB=0 mA stabile                           -> SOURCE_OFF
```

Esito: PASS.

### Conferma Icoil

In `SOURCE_OFF`:

```text
un solo step con IC=0 mA, poi IC=100 mA -> resta SOURCE_OFF
IC=0 mA stabile                          -> SOURCE_END
```

Sul ramo STATION, `IC=0` stabile ha prodotto
`DISCHARGE_LOAD -> LOAD_OFF`. Il successivo `IB=0` stabile ha prodotto
`LOAD_OFF -> LOAD_END`.

Lo scambio digitale conclusivo ha riportato entrambe le schede in STANDBY:

```text
VEHICLE SOURCE_END -> STANDBY
STATION LOAD_END   -> STANDBY
```

Esito: PASS.

### Banda morta selezione AUTO

E' stata generata una finestra controllata di link perso programmando
temporaneamente sulla STATION il firmware rollback senza radio. Dopo 15
mancati ACK il VEHICLE e' entrato in DISCOVERY. Il firmware STATION robusto e'
stato quindi ripristinato.

Risultati:

```text
VEHICLE 2900 Wh, STATION 2920 Wh, differenza -20 Wh:
    resta DISCOVERY, Role=NOTHING                         PASS

VEHICLE 3000 Wh, STATION 2920 Wh, differenza +80 Wh:
    VEHICLE -> INIT_SOURCE, STATION -> INIT_LOAD          PASS
```

Il confronto simmetrico `differenza <= -50 Wh -> LOAD` e' verificato
staticamente nel codice, ma non e' stato ripetuto su hardware mediante una
seconda finestra completa di link perso.

### Sicurezza e limiti

- `WLESS_SM_POWER_CONTROL_ENABLE` e' rimasto a zero per tutta la prova;
- nessuna azione e' stata applicata alla sezione di potenza;
- i valori 50 Wh e due campioni sono parametri iniziali configurabili, da
  validare rispetto ai requisiti fisici definitivi;
- i contatori MAXRT includono intenzionalmente le finestre di link perso create
  dal test e non rappresentano errori residui a link ristabilito.

Esito complessivo: PASS. Ciclo distribuito completo, regressione radio e
robustezza delle transizioni verificate senza controllo di potenza.

## Test 34 - Verifica quantitativa tick ISR2 decimato a 1 kHz

Obiettivo:
misurare quantitativamente sulla docking il tick logico prodotto dal fractional
accumulator agganciato a ISR2, senza usare TIMER1.

Banco:

```text
controlCARD VEHI0001
TMDSHSECDOCK docking station
UART COM25
oscilloscopio su GPIO30 / pin 80 HSEC
WLESS_SM_POWER_CONTROL_ENABLE=0
modulo nRF24 non necessario
```

Firmware diagnostico:
`WLESS_CHG_F28003x_VEHICLE_ISR2_1KHZ_GPIO30_TEST_PWR_OFF.out`.
La macro test-only `WLESS_SM_TICK_GPIO_TEST_ENABLE=1` commuta GPIO30 ogni volta
che il fractional accumulator genera un tick logico. Di conseguenza i fronti
rappresentano il tick da circa 1 kHz, mentre l'onda quadra completa ha frequenza
media circa 500 Hz.

Base temporale:

```text
ISR2 nominale = 21.250 kHz
periodo ISR2  = 47.0588 us
```

Il rapporto frazionario genera intervalli di 21 o 22 cicli ISR2. Due intervalli
formano alternativamente periodi completi da 43 e 42 cicli:

```text
43 / 21250 = 2.0235 ms -> 494.19 Hz
42 / 21250 = 1.9765 ms -> 505.95 Hz
```

Osservazioni dell'operatore:

```text
periodo visualizzato circa 2.02 ms
frequenza visualizzata circa 494 Hz
spostando la finestra di misura: valori alternati circa 494/505 Hz
segnale stabile
```

Evidenza oscilloscopio archiviata:
`docs/test_evidence/scope_23_isr2_1khz_gpio30.png`.

Valori automatici leggibili nel capture Keysight MSO-X 2024A:

```text
Period(1) = 2.0237 ms
Freq(1)   = 494.15 Hz
timebase  = 500 us/div
CH1       = 2.00 V/div, sonda 10:1, accoppiamento DC
```

Scostamento dal periodo lungo teorico:

```text
misurato  = 2.0237 ms
teorico   = 2.0235 ms
differenza circa 0.0002 ms = 0.2 us, circa 0.01%
```

Il livello alto/basso e la forma d'onda risultano regolari; nel capture non
sono visibili impulsi spuri o fronti multipli.

Interpretazione:
l'alternanza 494/505 Hz coincide con i periodi da 43 e 42 cicli previsti. La
media dei due periodi completi e' 2.000 ms, corrispondente a 500 Hz; poiche'
GPIO30 commuta a ogni tick, la frequenza media dei fronti e' 1.000 kHz. Non e'
presente errore cumulativo: 85 periodi ISR2 producono quattro tick logici in
4 ms.

Il fallimento di inizializzazione radio osservabile su docking senza modulo
nRF24 e' atteso e non influenza il tick ISR2/FSM.

Esito: PASS quantitativo. Frequenza media e jitter deterministico 21/22 cicli
coerenti col fractional accumulator.

## Test 35 - Dry-run UniPD BOOST low-voltage su docking

Obiettivo:
verificare senza stadio di potenza la propagazione di vettori SOURCE sintetici
attraverso il controllo UniPD e il layer di mapping BBC, mantenendo inibite le
uscite fisiche.

Banco e condizioni:

```text
controlCARD VEHICLE VEHI0001
TMDSHSECDOCK docking station
UART COM25, 115200 baud
WLESS_SM_POWER_CONTROL_ENABLE=0
UNIPD_bbcPowerOutputEnable=0
HFC non comandato dal test
nessun uso di TIMER1
```

Il formato diagnostico UART e':

```text
U,SYN,VALID,MISSING,VDC_mV,VBAT_mV,PREF_mW,ILREF_mA,
  DUTY_RAW_u,DUTY_MAP_u,DUTY_APPLIED_u,ALG_EN,OUT_EN,
  BBC_ENABLED,DOCK_TEST_ENABLED
```

Sequenza automatica:

```text
baseline
VDC sintetica = 6 V,  VBAT = 6 V, VDCref = 10 V
VDC sintetica = 8 V,  VBAT = 6 V, VDCref = 10 V
VDC sintetica = 10 V, VBAT = 6 V, VDCref = 10 V
VDC sintetica = 12 V, VBAT = 6 V, VDCref = 10 V
disarmo sintetico e reset UniPD
```

Per i vettori SOURCE sono stati usati `Ibat_ref_min=0 A` e
`Ibat_ref_max=2 A`. Dopo l'applicazione di ogni vettore e il completamento del
delay algoritmico sono stati osservati:

```text
VDC   PREF       ILREF_A   DUTY_RAW  DUTY_MAP  DUTY_APPLIED
6 V   7.039 W    0.587 A   0.000     1.000     0.200
8 V   3.961 W    0.330 A   0.000     1.000     0.200
10 V  0 W        0 A       0.600     0.400     0.200
12 V  0 W        0 A       0.500     0.500     0.200
```

Fatti osservati:

- durante il sintetico `VALID=2047` e `MISSING=0`;
- `PREF` e `ILREF_A` diminuiscono all'avvicinarsi al riferimento VDC;
- con `VDC >= VDCref`, `PREF=0` e `ILREF_A=0`;
- `OUT_EN=0`, `BBC_ENABLED=0` e `DOCK_TEST_ENABLED=0` per tutta la prova;
- dopo il disarmo, la maschera reale torna a `VALID=29`, `MISSING=2018` e
  tutte le uscite diagnostiche tornano a zero;
- con `UNIPD_bbcDutyMappingMode=1`, tutti i punti osservati producono un duty
  mappato maggiore di 0.20 e vengono quindi limitati a
  `DUTY_APPLIED=0.20`.

Interpretazione:
l'anello esterno UniPD reagisce nel verso previsto nel profilo SOURCE e il
percorso di inibizione mantiene il BBC spento. Il duty matematico UniPD a
corrente richiesta nulla coincide con il termine `VBAT/VDC`; il mapping
complementare `1-duty` puo' quindi rappresentare intenzionalmente il comando
fisico BOOST. Il test non consente pero' di validarlo perche' il limite 0.20
satura tutti i punti scelti e rimuove l'autorita' di regolazione osservabile.

Esito: PARTIAL PASS per anello esterno, maschere, reset e inibizione; NO-GO per
l'abilitazione perche' il profilo low-voltage/duty non e' ancora coerente e il
mapping fisico non e' discriminato. Le uscite di potenza non devono essere
abilitate prima della definizione di un punto operativo entro l'autorita' del
duty e di un nuovo dry-run PASS.

### Test 35.1 - Discriminazione mapping e autorita' duty

Il dry-run e' stato esteso mantenendo `OUT_EN=0`, `BBC_ENABLED=0` e
`DOCK_TEST_ENABLED=0`. Sono stati aggiunti controlli diagnostici per mapping,
duty massimo e riferimento VDC.

Punto neutro:

```text
VBAT=6 V
VDC=7 V
VDCref=7 V
PREF=0
ILREF=0
duty max diagnostico=0.50
```

Risultati:

```text
mapping diretto       DUTY_RAW=0.857142  DUTY_MAP=0.857142  APPLIED=0.500000
mapping complementare DUTY_RAW=0.857142  DUTY_MAP=0.142857  APPLIED=0.142857
```

Il mapping complementare produce esattamente `1 - VBAT/VDC = 1 - 6/7`,
coerente con la convenzione del comando BOOST. Il mapping diretto non e'
coerente con tale interpretazione.

Con mapping complementare e riferimento 7 V:

```text
VDC=6 V: PREF=1.430 W, ILREF_A=0.119 A, DUTY_MAP=0.694201
VDC=8 V: PREF=0 W,     ILREF_A=0 A,     DUTY_MAP=0.250000
```

Entrambi i valori superano il limite operativo corrente `duty max=0.20`.
Pertanto il mapping e' discriminato, ma i punti scelti non dispongono di
autorita' di regolazione completa con il limite corrente.

E' stato inoltre osservato che una riga UART composta lunga non viene applicata
quando supera la lunghezza utile del command buffer; il test e' stato ripetuto
con token separati.

Condizione finale verificata:

```text
synthetic test = 0
valid mask reale = 29
missing mask = 2018
mapping mode = 1
duty max = 0.20
OUT_EN = 0
BBC_ENABLED = 0
DOCK_TEST_ENABLED = 0
```

Esito: PASS per la discriminazione del mapping complementare e per il ripristino
sicuro; NO-GO invariato per l'abilitazione fisica finche' non viene approvato un
profilo duty/punto operativo coerente.

## Test 36 - Baseline asset modificato e trip VDC UniPD

Obiettivo:
verificare con la sola alimentazione logica l'inibizione del BBC, la misura del
DCLINK scarico e il comportamento del trip VDC latched prima della prova BOOST.

Setup:

```text
controlCARD VEHICLE montata sull'asset TI modificato
logica 12 V, current limit 1 A
VBATT 6 V predisposta ma uscita non abilitata
resistenza DCLINK1 1.2 kohm
carichi 83 ohm e 2.2 ohm non inseriti
UART COM25
HFC OFF
```

Baseline osservata:

```text
VDC reale circa 62 mV
synthetic test=0
valid mask=29
missing mask=2018
OUT_EN=0
BBC_ENABLED=0
DOCK_TEST_ENABLED=0
FSM=DISCOVERY
RadioLink=FAIL, atteso senza controparte attiva
```

Sequenza sintetica trip:

```text
VDC=9 V  -> trip latch=0
VDC=10 V -> trip latch=1, capture=10 V
VDC=6 V  -> trip latch resta 1
UF=1     -> reset accettato sotto soglia, latch=0, capture=0
UT=0     -> sintetico disarmato
```

Durante tutta la sequenza `OUT_EN=0`, `BBC_ENABLED=0` e
`DOCK_TEST_ENABLED=0`. Dopo il disarmo la misura reale e le maschere tornano ai
valori di baseline.

Esito: PASS. Il trip interviene alla soglia di 10 V, resta latched dopo la
rimozione della sovratensione sintetica e puo' essere resettato solo con VDC
sotto soglia. Nessuna uscita di potenza e' stata abilitata.

### Test 36.1 - Profilo ibrido con misure ADC reali

Obiettivo:
completare gli ingressi UniPD mancanti con riferimenti diagnostici SOURCE,
mantenendo reali VDC, correnti induttori e corrente bobina, senza abilitare il
BBC.

Profilo:

```text
VBAT sintetica=6 V
VDCref sintetica=7 V
Ibat min/max sintetiche=0/1 A
tx_rx mode=SOURCE
VDC, IL_A, IL_B, Icoil da ADC reali
OUT_EN=0
```

Con la sola alimentazione logica e corrente fisica nulla sono stati osservati:

```text
VDC circa 0.06...0.19 V
IL_A circa +1.20 A
IL_B circa -1.18 A
Icoil circa +68.98 A
```

Il profilo porta correttamente la maschera a `VALID=2047`, `MISSING=0`, ma le
misure a zero non sono compatibili con la chiusura del loop a limite batteria
1 A. `Icoil` e' inoltre prossimo al fondo scala firmware di circa 69.12 A e
supera il limite UniPD di 50 A.

Il profilo ibrido e' stato disarmato immediatamente. Condizione finale:

```text
synthetic test=0
valid=29
missing=2018
OUT_EN=0
BBC_ENABLED=0
DOCK_TEST_ENABLED=0
```

Esito: FAIL/NO-GO per le misure di corrente a zero; PASS per maschera ibrida,
inibizione e ripristino. Prima della prova BOOST occorre calibrare gli offset
IL_A/IL_B e sostituire o escludere esplicitamente Icoil finche' HFC resta OFF.

### Test 36.2 - Calibrazione offset IL e nuovo profilo ibrido

Con VBATT OFF sono stati acquisiti 30 campioni prima della calibrazione:

```text
IL_A medio +1.195 A, min +1.143 A, max +1.250 A
IL_B medio -1.197 A, min -1.246 A, max -1.129 A
OUT_EN=BBC_ENABLED=DOCK_TEST_ENABLED=0
```

Applicando la formula di conversione firmware sono stati impostati:

```text
TTPLPFC_IL1_OFFSET_PU=0.50944
TTPLPFC_IL2_OFFSET_PU=0.49244
```

Dopo clean build completa, flash e nuova acquisizione di 30 campioni:

```text
IL_A medio +0.0036 A, min -0.071 A, max +0.055 A
IL_B medio +0.0033 A, min -0.069 A, max +0.077 A
VDC medio 0.091 V
OUT_EN=BBC_ENABLED=DOCK_TEST_ENABLED=0
```

Nel profilo BOOST con HFC OFF, `Icoil` e' stato sostituito con zero sintetico,
mentre VDC e le correnti IL restano reali. Il profilo ibrido porta correttamente
la maschera a `VALID=2047`, `MISSING=0`, con `VBAT=6 V`, `VDCref=7 V` e
limite batteria 1 A. Gli enable fisici sono rimasti a zero. Il profilo e' stato
successivamente disarmato e le maschere sono tornate a 29/2018.

Esito: PASS per calibrazione a zero, profilo ibrido e ripristino sicuro. Il
test non ha abilitato VBATT o PWM.

### Test 36.3 - Reiezione dell'abilitazione fuori profilo

Condizioni: asset TI modificato, logica alimentata, VBATT OFF, DCLINK con
resistenza permanente da 1.2 kOhm, profilo ibrido disarmato.

Dopo flash della build con gate runtime sono stati inviati in sequenza
`UE=1`, `UT=99`, `UE=0`, `UT=99`. La richiesta `UE=1`, priva delle
precondizioni del profilo ibrido, non ha abilitato lo stadio. Entrambe le
diagnostiche hanno riportato:

```text
OUT_EN=0
BBC_ENABLED=0
DOCK_TEST_ENABLED=0
DUTY_APPLIED_A=0
VDC=0.063 V e 0.095 V
trip latched=0
```

Esito: PASS. Il percorso di reiezione e il disarmo esplicito mantengono le
uscite fisiche disabilitate. Non e' stato provato il percorso di accettazione
di `UE=1`, che richiede review esplicita prima di generare PWM.

### Test 36.4 - Trip sintetico correnti induttori

Condizioni: logica alimentata, VBATT OFF, BBC e profilo ibrido disabilitati.
Il vettore completamente sintetico e' stato impostato a VDC=VBAT=6 V; il
comando diagnostico `UI` forza correnti opposte sui due induttori e disabilita
preventivamente ogni uscita.

Sequenza ed esito:

```text
UI=500 -> IL_A=+0.500 A, IL_B=-0.500 A, latch=0
UI=800 -> IL_A=+0.800 A, IL_B=-0.800 A, latch=1
           capture A=+0.800 A, capture B=-0.800 A
UI=0   -> IL_A=IL_B=0, latch resta 1
UF=1   -> sotto soglia: latch=0, capture A/B=0
UT=0   -> vettore sintetico disarmato
```

Durante tutta la sequenza `OUT_EN=0`, `BBC_ENABLED=0` e
`DOCK_TEST_ENABLED=0`. La soglia configurata riportata dalla diagnostica e'
0.750 A.

Esito: PASS per non intervento sotto soglia, intervento oltre soglia,
persistenza, cattura e reset condizionato. Nessun PWM e nessuna alimentazione
di potenza sono stati abilitati.

### Test 36.5 - Riconferma trip IL con filtro temporale

Con VBATT OFF e uscite disabilitate e' stata ripetuta l'iniezione sintetica
dopo l'introduzione della conferma di 22 cicli, circa 1 ms. Lo stimolo
persistente `IL_A=+0.800 A`, `IL_B=-0.800 A` ha attivato il latch con catture
corrette. Dopo `UI=0`, `UF=1` ha azzerato latch e catture. La diagnostica ha
confermato duty cap 0.05 e uscite fisiche sempre disabilitate.

Esito: PASS. Il filtro non impedisce l'intervento sulla sovracorrente
persistente; nessun PWM e nessuna alimentazione di potenza sono stati attivati.

### Test 36.6 - Smoke test BOOST open-loop di confronto

Su richiesta e' stato ripetuto il punto open-loop gia' caratterizzato, con
VBATT=6 V, carico DCLINK 1.2 kOhm, HFC OFF, entrambe le leg BOOST e duty fisso
0.05. Il punto e' rimasto attivo per 10 s senza trip; l'oscilloscopio ha
misurato DCLINK=8.07 V, coerente con il precedente valore di circa 7.8 V.

Esito: PASS di non regressione del BOOST e del comando PWM open-loop. Il test
ha evidenziato che la differenza rispetto al run UniPD non appartiene allo
stadio BOOST di base.

### Test 36.7 - Run UniPD dopo configurazione PWM, acquisizione invalidata

Dopo la correzione della configurazione PWM SOURCE e' stato richiesto un run
di 3 s. Tutte le acquisizioni hanno riportato `OUT_EN=0`, `BBC_ENABLED=0`, VDC
circa 5.02...5.12 V e correnti prossime a zero. L'analisi della sequenza ha
mostrato che lo script interrogava `UT=99`, comando che revoca intenzionalmente
l'abilitazione prima di trasmettere la diagnostica.

Esito: INVALID/NO-POWER per la verifica del controllo corretto; PASS per il
fail-safe del comando diagnostico. Nessuna risposta del BOOST e' stata
osservata. Il test deve essere ripetuto usando una query read-only.

### Test 36.8 - Run UniPD BOOST con PWM dedicato e query read-only

Condizioni: VBATT=6 V con limite 1 A, carico DCLINK 1.2 kOhm, HFC OFF,
VDCref=7 V, limite batteria UniPD 0.2 A, duty cap 0.05, trip VDC 10 V e trip
IL 0.75 A. Il monitoraggio usa esclusivamente `UQ?` e non revoca l'enable.

Durante tre run da 3 s il BBC e' rimasto abilitato, DCLINK e' salita in modo
regolare e non sono intervenuti trip VDC o IL. Nel run acquisito con finestra
oscilloscopio estesa:

```text
VDC max firmware = 7.546 V
VDC max oscilloscopio = 7.59 V
scarto = 0.044 V, circa 0.6%
tempo di salita osservato = 5.5 divisioni
con 500 ms/div: circa 2.75 s
IBATT osservata nei run = circa 0.02 A
```

Esito: PASS per configurazione PWM SOURCE dedicata, applicazione fisica del
comando UniPD, coerenza ADC/oscilloscopio, arresto e interlock. Il punto a 3 s
resta sopra VDCref e con duty al limite 0.05: la regolazione a regime e la
desaturazione del controllore non sono ancora validate.

### Test 36.9 - Run UniPD BOOST closed-loop esteso a 10 s

Con le stesse condizioni e protezioni del Test 36.8, il run e' stato esteso a
10 s e monitorato con `UQ?`. Non sono intervenuti trip e `OUT_EN`/BBC sono
rimasti attivi fino all'arresto comandato.

Andamento principale:

```text
t~0 s: VDC=5.02 V, duty comandato=0.050
t~1 s: VDC=7.23 V, duty comandato=0.050
t~4 s: VDC=7.45 V, duty comandato=0.050
t~5 s: VDC=7.48 V, duty comandato=0.031
t~6 s: VDC=7.04 V, duty comandato=0.011
t~7 s: VDC=6.84 V, duty comandato=0.038
t~8 s: VDC=7.16 V, duty comandato=0.029
t~9 s: VDC=6.97 V, duty comandato=0.017
VDC max firmware=7.515 V
```

Esito: PASS funzionale per desaturazione, azione closed-loop, mantenimento
enable, interlock e arresto. Il loop converge nell'intorno del riferimento di
7 V ma presenta una dinamica lenta e un'oscillazione a bassa frequenza circa
6.84...7.16 V nel tratto osservato. FAIL/OPEN per validazione prestazionale e
tuning definitivo dei parametri low-voltage.

### Test 36.10 - UniPD BOOST con carico 83 Ohm e duty cap 0.05

Con la resistenza da 1.2 kOhm rimossa e il solo carico da 83 Ohm, VBATT=6 V,
VDCref=7 V e limite batteria 0.2 A, il run di 10 s ha prodotto:

```text
VDC iniziale=3.133 V
VDC massimo=3.645 V
VDC a regime circa 3.55...3.58 V
duty comandato=0.05 per tutto il run
potenza richiesta circa 1.2 W, al clamp
trip VDC/IL=0
```

Esito: PASS per stabilita', saturazione coerente, interlock e arresto; INCONCLUSIVE
per regolazione a 7 V. Il duty cap 0.05 non fornisce autorita' sufficiente con
83 Ohm e deve essere aumentato progressivamente prima di valutare il tuning.

### Test 36.11 - UniPD BOOST con carico 83 Ohm e duty cap 0.10

Due run da 10 s con VBATT=6 V, VDCref=7 V e limite batteria 0.2 A hanno
raggiunto un punto stabile con comando saturato a 0.10:

```text
VDC firmware max=3.997 V
VDC oscilloscopio max=4.09 V
scarto=0.093 V, circa 2.3%
IBATT osservata circa 0.06 A
trip VDC/IL=0
```

Esito: PASS per stabilita', coerenza ADC/oscilloscopio, progressione rispetto
al cap 0.05 e protezioni; INCONCLUSIVE per regolazione a 7 V perche' il comando
resta al clamp. Autorizzato il successivo gradino duty cap 0.20.

### Test 36.12 - UniPD BOOST con carico 83 Ohm e duty cap 0.20

Il run da 10 s e' stato ripetuto fino a ottenere un'acquisizione completa con
oscilloscopio a 1 s/div. Condizioni: VBATT=6 V, VDCref=7 V, limite batteria
0.2 A, carico 83 Ohm e duty cap 0.20.

```text
VDC firmware max=5.020 V
VDC oscilloscopio max=5.02 V
IBATT osservata circa 0.09 A
duty comandato=0.20 per tutto il run
trip VDC/IL=0
```

Esito: PASS per stabilita', corrispondenza ADC/oscilloscopio, incremento
coerente del punto operativo e protezioni; INCONCLUSIVE per regolazione a 7 V,
poiche' il comando resta saturato al cap. Il successivo gradino proposto e'
0.30, mantenendo invariati limite batteria e trip.

### Test 36.13 - UniPD BOOST con carico 83 Ohm e duty cap 0.30

Con VBATT=6 V, VDCref=7 V, limite batteria 0.2 A e carico 83 Ohm, due run da
10 s hanno raggiunto un punto stabile con comando saturato a 0.30:

```text
VDC firmware max=6.843...6.875 V
VDC oscilloscopio max=6.9 V
scarto rispetto al firmware circa 0.057 V, 0.8%
IBATT osservata circa 0.15 A
trip VDC/IL=0
```

Esito: PASS per stabilita', coerenza ADC/oscilloscopio, progressione del punto
operativo e protezioni. Il riferimento di 7 V non viene ancora attraversato e
il duty resta al clamp; proposto cap 0.35 per fornire margine di regolazione.

### Test 36.14 - UniPD BOOST con carico 83 Ohm e duty cap 0.35

Con il primo cap sufficiente ad attraversare VDCref=7 V, il run da 10 s ha
mostrato una dinamica fortemente oscillante:

```text
VDC firmware max=7.642 V
VDC oscilloscopio max=7.7 V
oscillazione osservata quasi 4 Vpp
IBATT max osservata circa 0.18 A
duty desaturato fino a circa 0.19...0.21 e poi tornato al cap 0.35
trip VDC/IL=0
```

Esito: PASS per autorita' sufficiente, attraversamento del riferimento,
coerenza ADC/oscilloscopio e protezioni; FAIL prestazionale per oscillazione
ampia. Non autorizzato alcun ulteriore aumento del duty. L'analisi del wrapper
ha evidenziato che la rampa limita gli incrementi ma applica immediatamente le
riduzioni di duty, dinamica asimmetrica coerente con il ciclo osservato.

### Test 36.15 - Cap 0.35 con rampa duty bidirezionale

Ripetendo il run da 10 s dopo aver limitato anche la discesa del duty allo
stesso slew-rate della salita sono stati osservati:

```text
VDC firmware max=7.642 V
VDC firmware min dopo la salita=5.340 V
oscillazione firmware circa 2.30 Vpp
oscillazione oscilloscopio circa 2.5 Vpp
IBATT max osservata circa 0.18 A
trip VDC/IL=0
```

Esito: PASS comparativo per la rampa bidirezionale, che riduce l'oscillazione
da quasi 4 Vpp a circa 2.5 Vpp senza aumentare la corrente; FAIL prestazionale
ancora confermato. La potenza richiesta continua a variare quasi fra 0 e il
clamp 1.2 W, indicando guadagno eccessivo del loop VDC alla scala low-voltage.

### Test 36.16 - Primo startup con rampa limitata alla fase iniziale

Dopo il ripristino dei coefficienti UniPD originali e la separazione della
rampa di startup, il run e' stato arrestato immediatamente dal trip IL:

```text
IL_A capture circa 0 A
IL_B capture=1.141 A
VDC firmware max=4.445 V
VDC oscilloscopio max circa 4.93 V
durata visibile dello spike/scarica circa 200 ms
IBATT non osservabile sul display alimentatore
handoff non raggiunto
```

Esito: FAIL startup, PASS protezione. L'oscilloscopio conferma un impulso reale
e non un solo artefatto ADC. Non viene autorizzato blanking del trip; richiesta
correzione della sequenza compare-shadow/OST prima di ripetere.

### Test 36.17 - Ripristino della rampa continua di riferimento

E' stata ripristinata la configurazione nota funzionante con coefficienti
UniPD originali, rampa duty bidirezionale continua, step 0.00001 pu/ciclo,
duty cap 0.35, VBATT=6 V e carico DCLINK=83 Ohm. Run ripetuti da 10 s hanno
fornito risultati coerenti:

```text
VDC firmware max=7.642 V
VDC oscilloscopio max circa 7.0...7.7 V
ripple oscilloscopio circa 2.6 Vpp
IBATT max osservata circa 0.18 A
trip VDC/IL=0
```

Le iniziali mancate acquisizioni in modalita' SINGLE sono risultate dovute
alla configurazione del trigger troppo vicina alla baseline DCLINK di circa
3.12 V. In modalita' RUN la rampa era visibile; spostando il trigger sopra la
baseline il SINGLE ha acquisito l'intero evento.

Esito: PASS per ripetibilita' della configurazione di riferimento, attivazione
fisica del BOOST, coerenza fra ADC e oscilloscopio e protezioni. Resta FAIL
prestazionale per ripple ampio rispetto a una regolazione stabile a 7 V.

### Test 36.18 - Scansione dello slew-rate della rampa duty

Con condizioni hardware e controllo invariati e' stato variato esclusivamente
lo step della rampa bidirezionale continua. Ogni run e' durato 10 s; il valore
effettivo dello step e' stato verificato nella telemetria UART. Criteri di
arresto: VDC >=9.5 V, IBATT >=0.5 A, trip VDC/IL oppure ripple >=4 Vpp come
limite prestazionale.

```text
step [micro-pu/ciclo]  rampa 0->0.35     VMAX scope   ripple scope
10                     circa 1.6 s       7.0...7.7 V  2.6 Vpp
20                     circa 0.8 s       7.7 V        3.2 Vpp
40                     circa 0.4 s       7.74 V       3.13 Vpp
80                     circa 0.2 s       7.7 V        3.8 Vpp
160                    circa 0.1 s       7.7 V        3.7 Vpp
320                    circa 0.05 s      7.65 V       3.65 Vpp
500                    circa 0.03 s      7.66 V       3.5 Vpp
```

Per tutti i run: duty max=0.35, trip VDC/IL=0 e nessun arresto anticipato.
IBATT max e' rimasta circa 0.18 A ai primi punti ed e' salita fino a 0.21 A a
step 320/500. Il run a step 500 e' stato ripetuto con risultato equivalente.

Esito: PASS per interlock, ripetibilita' e completamento della scansione; FAIL
prestazionale per ripple ancora pari a 2.6...3.8 Vpp. La frequenza del ripple
aumenta riducendo la durata della rampa, mentre l'ampiezza non e' monotona e
tende a variare poco agli step maggiori. Il punto 10 minimizza il ripple; il
punto 40 offre il miglior compromesso osservato fra durata della rampa e
ripple, da confermare prima di adottarlo come parametro operativo.

### Test 36.19 - BOOST con riferimento 9 V e confronto rampa

Con VBATT=6 V, carico 83 Ohm, I_bat_max=0.2 A, I_bat_min=0 A, duty cap 0.35
e HFC spento, il riferimento DCLINK e' stato portato da 7 V a 9 V. Il trip VDC
e' rimasto a 10 V e lo script avrebbe arrestato il run a 9.5 V.

Con `US=40`, run ripetuti hanno mostrato:

```text
VDC firmware max=7.642...7.706 V
VDC oscilloscopio max=7.87 V
IBATT max osservata=0.18 A
salita circa 300 ms
plateau iniziale circa 3 s
ripple successivo circa 0.7 Vpp
duty max=0.35
trip VDC/IL=0
```

Il riferimento 9 V non viene raggiunto e il duty resta al clamp. La prova e'
quindi una caratterizzazione in saturazione, non una regolazione a 9 V.

Sono stati poi confrontati step piu' rapidi mantenendo invariato tutto il resto:

```text
US=160  VMAX scope=7.87 V, forma simile a US=40, nessun trip
US=500  VMAX scope=7.8 V, ripple circa 1.5 Vpp, nessun plateau,
        salita piu' corta ma poco lineare, nessun trip
US=320  VMAX scope=7.87 V, salita rapida e definita, plateau circa 8 s,
        ripple non apprezzabile allo scope, nessun trip
```

Il run `US=320` e' stato ripetuto acquisendo separatamente il plateau dopo 2 s:

```text
VDC firmware complessiva=3.229...7.674 V
VDC firmware dopo 2 s=7.546...7.674 V
ripple UART sul plateau circa 0.128 Vpp
duty dopo 2 s=0.35 fisso
IBATT osservata=0.18 A costante
corrente interna max=0.155 A
trip VDC/IL=0
```

Esito: PASS per ripetibilita', interlock e stabilita' del punto saturo con
`US=320`. `US=500` e' respinto come candidato per fronte meno regolare e ripple
maggiore. Il test non valida `US=320` nel caso regolato a 7 V: il plateau a 9 V
e' determinato dal clamp duty e non dall'inseguimento del riferimento. Il
confronto indica che il grande ripple osservato a Vref=7 V e' associato
all'interazione del loop di tensione durante l'attraversamento del riferimento,
non a un ripple intrinseco del BOOST a duty costante.

### Test 36.20 - Estensione duty e limite corrente con Vref 9 V

Dopo review esplicita, il massimo armabile via UART e' stato esteso a 0.50,
mantenendo default 0.35, Vref=9 V, VBATT=6 V, carico 83 Ohm, US=320, trip VDC
10 V e arresto script 9.5 V. La scansione iniziale e' stata eseguita con
I_bat_max=0.2 A:

```text
UD=0.375  VMAX scope=8.11 V, IBATT max=0.19 A, ripple circa 0.7 Vpp
UD=0.400  VMAX scope=8.35 V, IBATT=0.20 A, salita circa 200 ms,
          plateau stabile e nessun ripple apprezzabile
UD=0.425  VMAX scope=8.35 V, IBATT=0.20 A, leggero ripple
UD=0.450  VMAX scope=8.51 V, IBATT=0.21 A, ripple contenuto e glitch iniziale
```

Con UD=0.45 il limite corrente e' stato poi aumentato per gradini:

```text
I_bat_max=0.3 A  VMAX scope=8.60 V, IBATT max=0.23 A,
                 plateau piu' stabile, glitch circa 0.5 V per 400 ms
I_bat_max=0.5 A  VMAX scope=8.68 V, IBATT max=0.23 A,
                 nessun glitch
```

Poiche' il duty restava al clamp, con I_bat_max=0.5 A sono stati provati:

```text
UD=0.475  VMAX scope=8.84 V, IBATT max=0.22 A
UD=0.500  VMAX scope=9.08 V, IBATT max=0.24 A,
          forma stabile ma glitch iniziale
```

Infine, mantenendo UD=0.50, I_bat_max e' stato portato a 0.6 A:

```text
VMAX scope=9.16 V
VMAX firmware=8.986 V
IBATT=0.24 A stabile
glitch assente
forma d'onda stabile
duty dopo 2 s=0.50 fisso
trip VDC/IL=0
```

Esito: PASS per progressione, interlock e raggiungimento fisico di 9 V. Il
limite 0.6 A rimuove il glitch senza aumentare la corrente a regime rispetto
al punto 0.5 A. Il loop non e' ancora dimostrato in regolazione desaturata:
il firmware misura al massimo 8.986 V e mantiene il duty al clamp 0.50. Non
sono autorizzati ulteriori aumenti di duty o corrente; il test successivo deve
usare un riferimento leggermente inferiore e ad alta risoluzione.

## BOOST UniPD closed-loop - scalabilita' 6 V -> 60 V (20 luglio 2026)

Setup comune: HFC disabilitato, carico resistivo 82/83 ohm sul DCLINK,
alimentazione logica 12 V, controllo UniPD originale, mapping BOOST
low-side complementare, duty cap 0.50 e ramp step 500 micro-pu/ciclo. I run
utili sono stati acquisiti per 10 o 20 s e terminati con disarmo UART.

Risultati consolidati:

- 6 V -> 8 V: baseline ripetibile; massimo firmware 8.698 V, regime
  7.866..8.122 V;
- 7 V -> 9.5 V: stabile; VMAX scope 10.67 V, IBATT 0.23 A;
- 8 V -> 11 V: stabile anche per 20 s; VMAX scope 12.6 V, IBATT 0.25 A;
- 9 V -> 12 V: stabile con UA=0.6 A; VMAX 14.55 V, IBATT max circa
  0.30 A e 0.26 A a regime;
- 10 V -> 13.5 V: stabile per 20 s; VMAX 16.5 V, IBATT 0.29 A;
- 11 V -> 14.75 V: stabile per 20 s; VMAX 18.14 V, ripple circa 0.5 V;
- 11.5 V -> 15.375 V: stabile ma con transitorio di circa 4 s, overshoot
  circa 4 V e undershoot circa 2.5 V;
- 12 V -> 16 V con UA=0.6 A: oscillazione persistente circa 7..8.5 Vpp;
  l'aumento del limite alimentatore da 1 A a 2 A non elimina il fenomeno;
- 12 V -> 16 V con UA=0.8 A: stabile e ripetibile; VMAX 20.34 V,
  IBATT max >0.4 A e 0.33 A a regime. Il precedente limit cycle era
  associato al clamp Pbat=VBAT*Ibat_max;
- 15 V -> 20 V con UA=1.2 A: instabile per perdita di margine quando il
  minimo DCLINK attraversa VBATT;
- 15 V -> 22 V con UA=1.2 A: stabile e ripetibile; VMAX 26.4 V, settling
  circa 1.2 s, IBATT max 0.50 A e 0.47 A a regime;
- 20 V -> 30 V con UA=1.5 A e trip rami 2.5 A: stabile; VMAX 36.4 V,
  settling circa 1.1 s, IBATT 0.63 A a regime;
- 30 V -> 45 V con UA=3 A e trip rami 4.5 A: stabile; VMAX 53.2 V,
  settling circa 1.1 s, IBATT max 1.44 A e 0.90 A a regime;
- 45 V -> 68 V con UA=4 A e trip rami 6 A: stabile; VMAX 76.9 V,
  settling circa 1 s, IBATT 1.32 A a regime. Il limite alimentatore a 3 A
  riduce la deformazione dello startup rispetto a 2 A;
- 60 V -> 90 V con UA=4 A, trip rami 6 A e alimentatore limitato a 5 A:
  PASS per 20 s; VMAX 97 V, regime UART 89.892..90.148 V, IBATT 1.7 A,
  Pbat_rif massimo 190.125 W, correnti di ramo campionate 0.904/0.907 A.
  Il carico ha superato 70 gradi C; VBATT e DCLINK sono stati portati a zero
  immediatamente dopo il run.

Prove ramp step a 9 V -> 12 V: US=500, 1000 e 2000 non modificano in modo
sostanziale VMAX; valori piu' rapidi aumentano il picco di corrente allo
startup e richiedono soglie di interlock maggiori. Non sono stati modificati
i guadagni UniPD.

Criterio conclusivo: il BOOST UniPD closed-loop e' stato validato fino al
massimo ingresso disponibile (60 V) e a 90 V/99 W circa sul carico. I limiti
Ibat_max e di corrente di ramo devono essere dimensionati sul punto operativo;
un clamp troppo stretto puo' generare un limit cycle pur con hardware e
controllore funzionanti.

### Diagnostica clamp inferiore al punto 60 V -> 90 V

E' stato ripetuto un run di 20 s per verificare se l'undershoot piu'
pronunciato dell'overshoot fosse correlato al clamp Pbat_rif=0. Risultati:

- DCLINK UART 70.928..95.968 V;
- Pbat_rif 1.261..213.587 W;
- 0 campioni su 215 con Pbat_rif prossimo a zero;
- 0 campioni con duty applicato prossimo a zero;
- 0 campioni con duty applicato al cap 0.50;
- IBATT max 2.85 A, 1.70 A a regime;
- forma scope sostanzialmente identica ai run precedenti;
- nessun fault.

Esito: l'ipotesi di undershoot causato direttamente dal clamp inferiore
Ibat_min=0 non e' confermata alla risoluzione della telemetria UART. Restano
plausibili l'asimmetria energetica del BOOST non rigenerativo e la dinamica
della cascata di controllo, ma non e' stato osservato un clamp esplicito di
potenza o duty durante il transitorio.

## 2026-07-20 - UniPD BUCK su STATION / scheda 2

### Setup

- probe/controlCARD `STAT0001`, UART COM26, firmware STATION;
- alimentazione logica 12 V, assorbimento circa 0.2 A;
- alimentatore collegato a DCLINK;
- carico 83 ohm collegato a VBATT;
- HFC non comandato e carico DCLINK da 1.2 kohm rimosso;
- multimetro e oscilloscopio collegati ai capi del carico;
- nessun commit eseguito.

### Verifica logica BUCK e convenzione di corrente

Con potenza disabilitata, `UH=2` ha selezionato `tx_1_rx_0=0`, mapping duty
diretto e modalita' BUCK. `UA=500` ha prodotto i limiti effettivi
`I_bat_rif_min=-500 mA` e `I_bat_rif_max=0`, confermando la convenzione UniPD
indicata dal DT. `UE=0` e BBC disabilitato sono rimasti verificati durante la
configurazione.

Le prime letture a corrente nulla erano circa IL_A=-0.65 A e IL_B=+1.11 A e
causavano il latch del trip da 0.75 A. Dopo la calibrazione esplicita a corrente
nulla, le letture residue sono rientrate tipicamente nell'ordine di alcune
decine di mA e il latch e' stato azzerato. Le prove powered successive sono
state eseguite soltanto dopo questa verifica.

### Smoke test BUCK open-loop a DCLINK 12 V

Il primo run closed-loop a duty cap 0.10 non ha prodotto un evento osservabile
perche' le correnti non calibrate falsavano gli anelli. Dopo calibrazione e'
stata verificata separatamente la modalita' open-loop BUCK su entrambi i rami.

Risultati consolidati:

| Duty | Baseline scope VBATT | Multimetro VBATT | IDCLINK | Esito |
|---:|---:|---:|---:|---|
| 0.02 | circa 1.8 V | circa 2.0 V | non risolta | PASS |
| 0.05 | circa 2.25 V | circa 2.5 V | circa 0.01 A | PASS, ripetibile |
| 0.10 | circa 3.6 V | circa 3.8 V | circa 0.02 A | PASS, ripetibile |

La risposta e' monotona ma non segue la relazione ideale Vout=D*Vin. Il dato e'
coerente con il funzionamento discontinuo, i condensatori di uscita e il ripple
osservato. La prova ha validato mapping high-side, propagazione UART del duty,
driver PWM e percorso DCLINK -> BUCK -> carico; non e' stata usata per ricavare
un modello statico del convertitore.

### Closed-loop a DCLINK 12 V

Con riferimento DCLINK 10 V, alimentatore rigido a 12 V e limite iniziale
`I_bat=[-0.2,0] A`, il punto con duty cap 0.10 ha dato VBATT circa 3.6 V e
IDCLINK circa 0.02 A, con salita pulita nell'ordine di 10 ms. Il risultato era
coincidente con l'open-loop perche' il duty era al cap.

Con `UA=100 mA`, `UW=8.3 V` e duty cap 0.30, la telemetria attiva ha mostrato:

- duty A circa 0.232 e duty B circa 0.054, entrambi fuori clamp;
- IL_A circa -68 mA e IL_B circa -19 mA;
- riferimento totale circa -86 mA;
- VBATT circa 6 V e IDCLINK circa 0.04 A;
- nessun trip e uscita stabile.

Il successivo punto `UA=50 mA` non e' stato considerato validante: VBATT e
IDCLINK sono rimasti circa 6.1 V e 0.04 A e le derive/resoluzione degli offset
IL erano comparabili con il riferimento richiesto.

### Closed-loop BUCK a DCLINK 40 V

Configurazione:

- DCLINK 40 V / limite alimentatore 2 A;
- riferimento DCLINK 38 V;
- `UW=33 V`, `UA=400 mA`, limite effettivo [-0.4,0] A;
- duty cap 0.80, rampa 500 micro-pu/ciclo;
- trip DCLINK 48 V, trip rami 1.5 A / 22 cicli;
- carico 83 ohm.

Risultati ripetuti su run fino a 10 s:

- VBATT 31..31.8 V;
- IDCLINK 0.30 A;
- riferimento per ramo circa -196 mA;
- a regime IL_A e IL_B circa -195 mA;
- duty A/B stabilizzati sotto il cap;
- transitorio circa 80 ms;
- nessun trip.

La potenza indicativa era circa 12 W in ingresso e 11.6..12.2 W sul carico. Il
calcolo di efficienza non e' formale a causa della risoluzione degli strumenti.

### Closed-loop BUCK a DCLINK 60 V

Configurazione:

- DCLINK 60 V / limite alimentatore 2 A;
- riferimento DCLINK 57 V;
- `UW=50 V`, `UA=600 mA`, limite effettivo [-0.6,0] A;
- duty cap 0.85, trip DCLINK 70 V, trip rami 2 A;
- carico 83 ohm.

Risultati su run da 5 s e 15 s:

- VBATT circa 47 V;
- IDCLINK circa 0.45 A;
- potenza richiesta UniPD circa -29.5..-29.8 W;
- riferimento per ramo circa -297..-300 mA;
- a 7.5 s IL_A e IL_B circa -312 mA;
- duty A/B circa 0.567/0.560, fuori clamp e quasi bilanciati;
- nessun trip;
- ripple/inviluppo visibile in riduzione dopo circa 3 s, circa 2.6 Vpp nella
  misura a finestra larga.

La potenza indicativa era 27 W in ingresso e 26.6 W sul carico. L'efficienza
derivata e' soltanto indicativa. La misura ad alta frequenza del ripple e'
rimandata: servira' una cattura separata in RUN/Normal a 5..10 us/div dopo
l'assestamento, per distinguere PWM, ringing e aliasing.

### Esito e limite della validazione

Il percorso UniPD -> mapping BUCK -> PWM high-side -> stadio -> carico, la
convenzione negativa e i due controllori di corrente indipendenti sono PASS
fino a circa 27 W sul banco disponibile. Il test a 60 V ha mostrato correnti di
ramo convergenti, duty fuori clamp, uscita stabile e nessun fault.

L'anello esterno di regolazione DCLINK non e' ancora validato perche'
l'alimentatore impone rigidamente la tensione del bus. La prova successiva
prevede una resistenza da 2.2 ohm in serie al positivo dell'alimentatore, con
negativo invariato, per introdurre una caratteristica di droop controllabile.
# Test VBATT ADC - mapping J26.39 e verifica agli estremi (2026-07-21)

Obiettivo: validare senza stadio di potenza il percorso fisico e software usato
per fornire a UniPD la misura reale di VBATT/VIN.

Setup:

- controlCARD STATION `STAT0001` su docking;
- PWM forzato OFF mediante `UB=0`, `UE=0`, `UH=0`;
- ingresso analogico J26.39 pilotato rispetto ad AGND;
- letture acquisite mediante `UQ?`;
- nessuna alimentazione applicata a DCLINK o allo stadio di potenza.

Una prima configurazione errata su ADCINC14 non rispondeva né alla massa né a
3,297 V. La rilettura dello schema controlCARD Rev.B ha mostrato che, a causa
della numerazione alternata delle due colonne del connettore, J26.39 corrisponde
ad ADCA3/ADCB9/ADCC7 e non ad ADCINC14. Il firmware è stato corretto per usare
ADCC7 su SOC13.

Risultati dopo la correzione:

| Ingresso J26.39 | Raw ADC | Conversione nominale VBATT | Esito |
|---:|---:|---:|---|
| AGND / 0 V | 0 count; sporadico 1 LSB fra letture asincrone | 0--0,040 V | PASS |
| 1,650 V | 2038--2051 count; centro circa 2047 | 82,2--82,8 V | PASS |
| 3,297 V | 4087--4094 count | 164,5--164,8 V | PASS |

Il valore teorico corrispondente a 3,297 V con rapporto nominale 0,02 è
164,85 V. Il mapping `J26.39 -> ADCC7 -> SOC13 -> firmware` è quindi validato.
Il punto intermedio teorico è 2048 count e 82,5 V equivalenti. La risposta ADC
è pertanto coerente e lineare sui tre punti di docking. Resta da validare il
rapporto reale del condizionamento `VBATT -> VIN_ADC` sulla scheda modificata.
# BOOST UniPD con VIN fisica - Scheda 2 / VEHI0001 (2026-07-21)

Obiettivo: ripetere la baseline BOOST 6 V -> 8 V eliminando l'override sintetico
`UW` sulla tensione batteria e usando `VIN_ADC` reale.

Setup:

- controlCARD `VEHI0001`, firmware VEHICLE, montata sulla Scheda TI 2;
- VIN=6 V, limite alimentatore 1 A;
- carico 83 ohm su DCLINK;
- HFC disabilitato;
- `UH=1`, `UV=8000`, `UA=600`, `UD=500`, `US=500`;
- trip DCLINK 10 V; trip correnti 1,5 A per 22 cicli;
- offset correnti ricalibrati a VIN OFF mediante `UF=2`;
- durata run 10 s e arresto UART automatico.

Il firmware è stato corretto prima del run per escludere `V_BAT` dalla maschera
degli override sintetici usata da `UH=1/2`. La telemetria ha quindi usato la
misura ADCC7 reale; `UW` è rimasto un valore configurato ma non applicato.

Risultati:

```text
VIN fisica UART             5,80...6,00 V
DCLINK a regime UART        7,83...8,12 V
Vmax scope                  8,81 V
overshoot                   +0,81 V, circa 10,1%
undershoot                  circa -2,25 V
settling time               circa 1,5 s
ripple a regime scope       circa 0,5 Vpp
IBATT a regime              circa 0,20 A
IBATT massima               non risolta dall'alimentatore
Pbat_rif                    circa 1,45...1,66 W
Iref per ramo               circa 0,12...0,15 A
duty BOOST applicato        circa 0,28...0,32
duty cap                    0,50
trip VDC / correnti         nessuno
```

Il duty è rimasto nettamente fuori dal clamp; la prova valida pertanto una vera
zona di regolazione e non un semplice punto saturo. Il massimo è vicino alla
baseline precedente (8,698 V, differenza circa 1,3%). Esito: PASS per
acquisizione VIN fisica, integrazione UniPD, controllo BOOST desaturato,
interlock e ripetibilità della baseline 6 V -> 8 V.

Una prova precedente con 1,2 kohm ha raggiunto ripetibilmente il trip DCLINK a
10,01 V: il carico quasi nullo non forniva sufficiente capacità di scarica a un
BOOST unidirezionale. Il risultato non è stato attribuito a VIN_ADC.

## Confronto VIN_ADC Scheda 1 / Scheda 2

Usando la stessa controlCARD `VEHI0001` e lo stesso firmware:

| VIN fisica | Scheda 1, VBATT firmware | Scheda 2, VBATT firmware |
|---:|---:|---:|
| 0 V | circa 14,4 V | circa 0 V |
| 6 V | circa 14,5 V | 5,88...5,96 V |
| 15 V | circa 14,7 V | non provato |
| 19,93 V | circa 14,8 V | non provato |

Il problema segue la Scheda 1 e non la controlCARD. Sono esclusi come causa
ADCC7, SOC13 e la conversione firmware. Resta da diagnosticare sulla Scheda 1
il circuito `VIN_ADC`, la sua alimentazione/massa, i componenti del
condizionamento e la continuità verso J26.39.

Le letture ottenute durante un tentativo con polarità VIN invertita per errore
di cablaggio sono invalide e non sono usate per deduzioni circuitali.
## Test HFC diretto DCLINK-to-DCLINK con rettificatore passivo - 2026-07-22

Setup:

```text
SOURCE: scheda TI 2 + VEHI001, alimentatore su DCLINK
LOAD:   scheda TI 1 + STAT0001, 83 ohm su DCLINK
VBATT:  scollegate su entrambe le schede
Bobine: collegate
HFC SOURCE: open-loop TI, 85 kHz, duty 0.5, phase shift 0
HFC LOAD: non comandato, rettificatore passivo
BOOST/BUCK: non comandati
Durata run: 5 s
```

Risultati:

| DCLINK SOURCE | Limite PSU | IDCLINK regime | VLOAD max/plateau | P ingresso | P carico 83 ohm | Rendimento apparente | Esito |
|---:|---:|---:|---:|---:|---:|---:|:---:|
| 12 V | 1.0 A | 0.85 A | 20.6 V | 10.2 W | 5.11 W | 50% | PASS |
| 15 V | 1.5 A | 1.11 A | 27.6 V | 16.65 W | 9.18 W | 55% | PASS |
| 16 V | 1.5 A | 1.20 A (1.38 A max) | 29.9 V | 19.2 W | 10.77 W | 56% | PASS |

In entrambi i punti la risposta e' a gradino, con salita di circa 40 ms,
plateau ben definito e senza oscillazioni evidenti, e discesa asintotica di
circa 500 ms. La forma e' risultata ripetibile aumentando la durata da 1 s a
5 s. Il comando UART ha arrestato automaticamente l'HFC al termine dei run.

Durante un run a 12 V, quattro campioni UART nel plateau hanno restituito
`ITANK_MOD raw` compreso tra 233 e 294 e corrente convertita tra 4.134 A e
4.809 A. Questi valori sono dello stesso ordine della baseline a HFC spento
(raw circa 250, corrente convertita circa 4.4 A): la misura non e' quindi
ancora validata come rappresentazione della corrente fisica del tank e non e'
stata usata per controllare il ponte. `ITANK_PHS` non e' disponibile ed e'
mantenuto sintetico.

Il punto a 16 V raggiunge il target di circa 10 W indicato dal DT. Il picco di
corrente SOURCE resta inferiore al limite impostato di 1.5 A e la forma d'onda
rimane identica ai punti precedenti. La validazione HFC diretta a questo livello
di potenza e' pertanto conclusa con esito positivo; non e' necessario aumentare
ulteriormente il DCLINK per questo obiettivo.

### Integrazione BOOST closed-loop e HFC

Setup invariato lato LOAD; lato SOURCE l'alimentatore e' stato spostato da
DCLINK a VIN e il BOOST UniPD e' stato configurato per 12 V -> 16 V. Per evitare
l'overshoot a vuoto, la sequenza validante e' stata HFC ON, attesa 1 s, BOOST ON,
BOOST OFF e HFC OFF dopo altri 0.5 s.

Un tentativo BOOST-only con solo bleeder da 1.2 kohm ha raggiunto il trip DCLINK
di 24 V (capture firmware 24.016 V) ed e' stato arrestato. La sequenza con HFC
precaricato non ha invece prodotto trip.

Nel plateau combinato:

```text
VIN impostata                 12 V
VIN fisica UART               11.64..11.84 V
Corrente VIN strumentale      2.0..2.2 A
DCLINK SOURCE strumentale     circa 16 V
DCLINK SOURCE UART            15.41..16.12 V
VLOAD strumentale             circa 27 V
VMAX scope                    circa 30 V
Carico                        83 ohm
Potenza LOAD al plateau       circa 8.78 W
Potenza ingresso              circa 24..26.4 W
IL_A                          1.03..1.14 A
IL_B                          1.07..1.23 A
Duty applicato                circa 0.386..0.429
Trip                          nessuno
```

La forma d'onda separa chiaramente le fasi comandate: primo plateau di circa
10 V con solo HFC e bus passivo, salita asintotica a 27..30 V quando il BOOST
entra in regolazione, ritorno a circa 10 V allo spegnimento BOOST e decadimento
a zero dopo HFC OFF. Esito integrazione BOOST closed-loop + HFC: PASS al primo
livello di potenza.

### Criticita' della sequenza di attivazione HFC/BOOST a VIN 20 V

Portando VIN a 20 V, l'attivazione HFC seguita da 1 s di attesa prima del BOOST
ha prodotto una condizione iniziale sbilanciata. Con BOOST PWM OFF e solo HFC
attivo sono stati misurati:

```text
VIN                         circa 19.6 V
DCLINK SOURCE               circa 16.0..16.2 V
Corrente alimentatore       1.22 A costanti
IL_A firmware               circa 1.15..1.30 A
IL_B firmware               circa -0.05..+0.14 A
```

La corrispondenza tra corrente alimentatore e IL_A conferma un percorso di
conduzione passivo quasi interamente sul ramo A quando HFC carica il DCLINK con
BOOST disabilitato. Abilitando successivamente il BOOST, sono stati osservati
trip IL_A con capture 7.06..7.53 A, mentre IL_B restava prossimo a zero. In
altri run il DCLINK ha invece superato il riferimento di 30/40 V fino ai trip
42/50/55 V; il massimo DCLINK di 49.41 V e' stato confermato da multimetro e il
VLOAD massimo e' stato circa 50 V. Variazioni di `UA`, `UD` e `US` non hanno
eliminato la criticita' mantenendo l'anticipo HFC di 1 s.

Con avvio quasi simultaneo `HFC=1` / `UE=1`, VIN=20 V, Vref=30 V, UA=1.8 A,
UD=0.50 e US=500:

```text
DCLINK SOURCE UART          18.96..19.28 V
Pbat_rif                    34.6..34.9 W (clamp)
IL_A                        0.88..0.97 A
IL_B                        0.82..1.02 A
Duty applicato              circa 0.13..0.17
Trip                        nessuno
VLOAD max scope             38 V
```

La sequenza simultanea elimina lo sbilanciamento iniziale e rende stabile il
punto, pur lasciando il DCLINK sotto riferimento per il clamp di potenza. La
sequenza di attivazione HFC/BOOST e' quindi un requisito funzionale da integrare
nella futura orchestrazione FSM: non deve essere assunto che HFC possa essere
precaricato per un tempo arbitrario prima del BOOST.

La ripetizione per 10 s dello stesso punto, con avvio quasi simultaneo, ha
confermato l'assenza di trip e la stabilita' del funzionamento. Durante il run
sono stati rilevati:

```text
DCLINK SOURCE UART          18.39..18.58 V
VIN fisica UART             19.66..19.78 V
Corrente ingresso strument. 1.5..1.8 A
Pbat_rif                    35.2..35.5 W (clamp)
IL_A                        0.65..0.90 A
IL_B                        0.75..0.78 A
Duty ramo A                 circa 0.102..0.111
Duty ramo B                 circa 0.107..0.108
HFC                         85 kHz, duty 0.50
Trip                        nessuno
```

La corrente d'ingresso strumentale e' coerente con il limite di potenza e con
la somma delle correnti dei due rami. Il run conferma la ripetibilita' della
sequenza simultanea; non costituisce ancora verifica del raggiungimento dei
30 V, impedito in questo punto dal clamp impostato a `UA=1.8 A`.

Un successivo incremento del limite a `UA=3.0 A`, mantenendo invariati
Vref=30 V, UD=0.50, US=500, carico da 83 ohm e avvio quasi simultaneo, ha
prodotto una sovratensione reale ed e' stato arrestato automaticamente:

```text
Trip DCLINK impostato        42.000 V
Capture DCLINK firmware      42.020 V
VMAX LOAD scope              50.9 V
Trip corrente ramo           nessuno registrato
Esito                        FAIL per sovratensione DCLINK
```

Il valore strumentale sul LOAD e la capture firmware concordano sulla natura
fisica del transitorio. La soglia di protezione non e' stata aumentata. Il
passaggio diretto da UA=1.8 A a UA=3.0 A non e' quindi accettabile in questa
configurazione; occorre cercare progressivamente il limite stabile con valori
intermedi.

Con `UA=2.2 A` e gli altri parametri invariati sono stati eseguiti un run da
5 s e una ripetizione da 10 s per consentire le letture manuali. Entrambi i run
sono terminati senza trip:

```text
DCLINK SOURCE UART          21.01..21.94 V
VIN fisica UART             19.42..19.54 V
Corrente ingresso strument. 1.7..2.4 A
Pbat_rif                    42.1..43.0 W (clamp)
IL_A                        0.81..1.17 A
IL_B                        0.89..1.08 A
Duty applicato              circa 0.19..0.23
VMAX LOAD scope             42.4 V
Trip                        nessuno
```

Esito del punto `UA=2.2 A`: PASS per stabilita', bilanciamento dei rami e
assenza di trip. Il DCLINK resta inferiore al riferimento di 30 V, per cui il
punto non valida ancora il raggiungimento del setpoint.

Il gradino successivo `UA=2.4 A`, eseguito per 10 s con gli altri parametri e
la sequenza simultanea invariati, ha fornito:

```text
DCLINK SOURCE UART          22.35..22.55 V
VIN fisica UART             circa 19.46 V
Corrente ingresso strument. 2.0..2.5 A
Pbat_rif                    circa 46.3 W (clamp)
IL_A                        1.13..1.34 A
IL_B                        0.99..1.30 A
Duty applicato              circa 0.225..0.262
VMAX LOAD scope             44.8 V
Trip                        nessuno
```

Esito del punto `UA=2.4 A`: PASS. La crescita rispetto a UA=2.2 A e' coerente,
i rami restano sostanzialmente bilanciati e non sono intervenute protezioni;
il DCLINK resta tuttavia inferiore al riferimento di 30 V.

Il gradino `UA=2.6 A`, eseguito per 10 s con configurazione invariata, ha
fornito strumentalmente:

```text
Corrente ingresso strument. 2.2..2.88 A
VMAX LOAD scope             47.3 V
Trip al termine             nessuno
```

La telemetria UART richiesta durante il run non e' stata ricevuta; a stadi
disabilitati e' stato comunque confermato che entrambi i latch erano liberi.
Il punto e' classificato PASS strumentale, ma senza caratterizzazione delle
correnti di ramo e dei duty a regime. Tale limitazione e' mantenuta esplicita
e i valori interni non vengono inferiti dai punti adiacenti.

Il gradino `UA=2.8 A`, mantenuto per 10 s, ha fornito:

```text
DCLINK SOURCE UART          23.98..24.50 V
VIN fisica UART             19.42..19.62 V
Corrente ingresso strument. 2.5..3.0 A
Pbat_rif                    53.6..54.5 W (clamp)
IL_A                        1.20..1.36 A
IL_B                        1.22..1.51 A
Duty applicato              circa 0.275..0.304
VMAX LOAD scope             48.5 V
Trip                        nessuno
```

Esito del punto `UA=2.8 A`: PASS. Le grandezze crescono in modo coerente, i
rami restano bilanciati e non sono state osservate protezioni; il DCLINK resta
ancora inferiore al riferimento di 30 V.

Il punto di confine `UA=2.9 A`, mantenuto per 10 s, ha fornito:

```text
DCLINK SOURCE UART          24.30..24.56 V
VIN fisica UART             19.46..19.54 V
Corrente ingresso strument. 2.5..3.11 A
Pbat_rif                    54.9..55.9 W (clamp)
IL_A                        1.53..1.64 A
IL_B                        1.51..1.59 A
Duty applicato              circa 0.304..0.328
VLOAD al plateau            circa 46 V
VMAX LOAD scope             50.1 V
Trip                        nessuno
```

Esito del punto `UA=2.9 A`: PASS. Il plateau LOAD e' ben definito, i due rami
sono molto ben bilanciati e il punto e' stabile per l'intera durata. Il forte
contrasto con la sovratensione osservata nel precedente run a `UA=3.0 A`
richiede una ripetizione mirata dello stesso punto a 3.0 A, senza modificare
sequenza o soglie, per distinguere una discontinuita' ripetibile da un evento
isolato.

La ripetizione a `UA=3.0 A` ha riprodotto il trip DCLINK con capture firmware
pari a 42.020 V. Dopo l'arresto automatico il DCLINK SOURCE e' rimasto carico:
anche dopo il reset del latch la diagnostica riportava ancora circa 35.8 V,
anziche' il valore passivo di circa 19.4 V. La prova successiva a UA=3.1 A e'
stata quindi sospesa fino alla scarica manuale del bus.

Nei run terminati normalmente BOOST viene disabilitato circa 250 ms prima
dell'HFC e il DCLINK ritorna al valore passivo. Nel trip i due stadi vengono
invece inibiti dalla protezione e non e' stata osservata una scarica analoga.
Fatto consolidato: il reset del latch non garantisce la rimozione dell'energia
residua dal DCLINK. E' richiesta una funzione software dedicata di scarica
controllata post-trip; architettura, percorso di potenza e condizioni di
sicurezza restano da definire e validare separatamente.

La prova diagnostica successiva a `UA=3.1 A`, eseguita dopo avere riportato
manualmente il DCLINK a circa 19.4 V, ha prodotto lo stesso esito:

```text
Capture DCLINK firmware      42.020 V
VMAX LOAD scope              50.9 V
Trip corrente ramo           nessuno
DCLINK residuo dopo il trip  circa 40.6 V (UART)
Esito                        FAIL per sovratensione DCLINK
```

Il risultato esclude una singolarita' limitata al valore esatto UA=3.0 A. Con
questa configurazione il confine osservato e' compreso tra UA=2.9 A, stabile
per 10 s, e UA=3.0 A, che riproduce il trip di sovratensione.

### Verifica a potenza disabilitata della cattura pre-trip

Il firmware VEHICLE release 1002, programmato su VEHI001, e' stato verificato
con BOOST e HFC disabilitati. Dopo calibrazione offset `UF=2`, `CAP=1` ha
riempito il buffer circolare di 48 campioni; `CAP=0` e `CAPD?` hanno confermato
freeze manuale, formato CSV e ordinamento cronologico.

Per verificare il trigger senza applicare PWM, la soglia DCLINK e' stata
temporaneamente portata sotto la tensione fisica presente. La cattura si e'
congelata automaticamente con `trigger=1` e 48 campioni. Un tentativo di riarmo
con latch attivo e' stato rifiutato con `C,FAULT`. Al termine la soglia DCLINK
e' stata ripristinata a 42 V e il latch e' stato azzerato. Esito parser,
buffer, freeze e protezione dal riarmo ambiguo: PASS. La verifica quantitativa
del budget temporale ISR resta pendente prima dell'uso in potenza.

### Prima cattura pre-trip in potenza a UA=3.0 A

Un primo tentativo, con cattura armata prima dell'accensione VIN, si e'
congelato su un inrush passivo precedente all'abilitazione BOOST. Sono stati
osservati IL_A=10.17 A e IL_B=6.84 A con duty rampati ancora nulli; il latch
corrente riportava IL_A=7.373 A. Il tentativo non e' stato classificato come
prova BOOST+HFC. La sequenza corretta richiede VIN stabile, reset del latch e
solo successivamente arm e comando di potenza.

Ripetendo con VIN stabile, il buffer si e' congelato correttamente sul trip
DCLINK a 42.020 V (`trigger=1`), senza trip corrente. I 48 campioni precedenti
hanno mostrato:

```text
Vdc                         35.688 -> 42.020 V
Vbat                        circa 19.62..19.86 V
Pbat_rif                    27.433 -> 1.705 W
IL_A                        circa 0.36..0.97 A
IL_B                        circa 0.27..1.09 A
Duty raw A                  circa 0.433 -> 0.386
Duty raw B                  circa 0.466 -> 0.399
Duty mapped A               circa 0.567 -> 0.614
Duty mapped B               circa 0.534 -> 0.601
Duty applied A/B            0.500 (clamp)
Duty ramped A/B             0.500 dal quarto campione osservato
```

Fatto: nel tratto finale della sovratensione il limite `UD=0.50` e' attivo su
entrambi i rami. Mentre Vdc cresce e Pbat_rif viene quasi azzerata, i duty raw
UniPD diminuiscono ma la mappatura BOOST `1-duty` produce duty crescenti oltre
0.50, mantenuti al massimo dal clamp. Il trip non e' associato a sovracorrente.

Inferenza: il FAIL e' legato alla combinazione uscita UniPD, inversione della
mappatura BOOST e clamp del duty. La cattura non dimostra ancora, da sola, se
l'inversione sia circuitalmente errata in assoluto; dimostra pero' che nel
regime osservato essa trasforma la correzione UniPD in un comando fisico che
resta saturo al massimo durante la crescita del bus.

Una seconda cattura, estesa con riferimenti ed errori interni, ha raffinato
l'interpretazione. Con VIN gia' stabile e sequenza valida, il trip DCLINK e'
stato riprodotto a 42.052 V senza trip corrente; lo scope sul LOAD ha misurato
VMAX=50.5 V. Nei 32 campioni su 480 cicli ISR precedenti sono stati osservati:

```text
Vdc                         35.752 -> 42.052 V
I_L_rif A/B                 0.676 -> 0.027 A
IL_A                        circa 0.30..1.02 A
IL_B                        circa 0.35..1.03 A
Errore IL_A                 +0.217 -> -0.344 A
Errore IL_B                 -0.378 -> -0.412 A
V_L_rif_A                   4.180 -> 3.597 V
V_L_rif_B                   2.841 -> 2.564 V
Pbat_rif                    26.589 -> 1.072 W
delta UniPD A               0.433 -> 0.387
delta UniPD B               0.470 -> 0.412
1-delta mappato A           0.567 -> 0.613
1-delta mappato B           0.530 -> 0.588
Duty applicato/rampato      0.500 su entrambi i rami
```

La relazione documentale `delta=(Vbat-V_L_rif)/Vdc` e la mappatura low-side
BOOST `1-delta` risultano numericamente coerenti. Il clamp UD=0.50 limita un
comando richiesto ancora maggiore e non e' la sorgente della crescita.

L'anello esterno riduce correttamente Pbat_rif e I_L_rif quasi a zero; l'anello
di corrente rileva correnti superiori al riferimento, come mostrano gli errori
negativi, ma V_L_rif rimane positivo e decade troppo lentamente rispetto alla
salita del bus. La frequenza non e' errata: ISR2 e coefficienti UniPD sono
entrambi definiti per 21.25 kHz (Ts circa 47 us). L'anello di corrente UniPD e'
documentato con banda 85 Hz. La finestra catturata copre circa 22.6 ms, durante
i quali il plant supera la soglia prima che lo stato del regolatore possa
portare V_L_rif a zero o negativo.

Inferenza aggiornata: l'inversione complementare e' coerente con il convertitore
sincrono. Il confine UA=2.9/3.0 A corrisponde verosimilmente al passaggio da un
equilibrio sotto setpoint, limitato dalla potenza disponibile, a un regime che
attraversa il setpoint; nel plant di prova l'energia continua a caricare il bus
piu' rapidamente della risposta a 85 Hz dell'anello interno. Non e' stata
osservata saturazione di V_L_rif ai limiti teorici, quindi questa cattura non
indica un classico wind-up sul clamp di V_L.

## Verifica logica controllo WPT distribuito - firmware 1003

Prova eseguita con entrambe le controlCARD alimentate, VEHICLE su COM25 e
STATION su COM26. `WLESS_SM_POWER_CONTROL_ENABLE` e' rimasto a zero; non sono
stati applicati comandi ai PWM.

Risultati:

- programmazione flash completata con successo su entrambe le schede;
- `FW?` ha restituito `FW=1003` su COM25 e COM26;
- `WPT?` a reset ha restituito `EN=0`, ruolo nullo e uscite neutre su entrambe;
- i comandi `WPTSRC`, `WPTLOAD`, `WPTIMAX` e `WPT=1` sono stati acquisiti con
  i valori richiesti;
- `MANUAL;SOURCE` ha assegnato SOURCE alla VEHICLE; la STATION ha assunto LOAD
  tramite il collegamento radio operativo;
- con riferimento SOURCE ridotto a 1 V, il valore locale `Pwr2Ld=1` della
  VEHICLE e' stato ricevuto sulla STATION come `PREM_W=1` e usato come
  `PREF_W=1`: verificato quindi il verso SOURCE -> LOAD con scala 1 W/LSB;
- dopo `INITOK` su entrambe le schede, le FSM hanno raggiunto rispettivamente
  `PRECHARGE_SOURCE` e `PRECHARGE_LOAD`; la STATION ha pubblicato
  `IERR_cA` circa -6890 e la VEHICLE lo ha ricevuto come `IREM_cA=-6886`:
  verificato quindi anche il verso LOAD -> SOURCE con scala 0.01 A/LSB;
- al termine `WPT=0` ha riportato a zero potenza, errore e uscite logiche su
  entrambe le schede.

Esito: PASS per programmazione, diagnostica UART, selezione dei ruoli e
trasporto numerico bidirezionale SOURCE <-> LOAD. NON TESTATA l'applicazione
del comando HFC. Nessuna conclusione sul comportamento di potenza closed-loop
deriva da questa prova.

Criticita' rilevata a HFC non pilotato: `HFC?` ha riportato sulla VEHICLE
`ITANK_MOD raw=268`, convertito in circa 4.555 A, e sulla STATION
`ITANK_MOD raw=4090`, convertito in circa 68.845 A. Una ripetizione dopo lo
stop ha confermato rispettivamente circa 4.488 A e 68.929 A. Il valore STATION
e' prossimo al fondo scala ADC ed e' incompatibile con una corrente di bobina
nulla. In conseguenza di questo ingresso, il controllore LOAD genera un errore
fortemente negativo e il comando SOURCE rimane clampato a zero.

Esito del prerequisito analogico: FAIL. Prima di collegare l'uscita UniPD al
phase shift HFC devono essere verificati elettricamente lo zero di `ITANK_MOD`
sulle due schede e la relativa catena J26.31/ADCC11. Non e' stata applicata
alcuna correzione software o compensazione sintetica.

### Discriminazione scheda/controlCARD mediante scambio

Le controlCARD VEHI001 e STAT0001 sono state scambiate tra le due schede,
lasciando HFC e stadi di potenza disabilitati. Dopo la nuova enumerazione UART,
tre letture `HFC?` per scheda hanno fornito:

```text
Scheda TI 1 + VEHI001 / COM25: raw 4092, 4092, 4090
                                69.064, 68.895, 68.946 A
Scheda TI 2 + STAT0001 / COM26: raw 248, 261, 238
                                4.134, 4.235, 4.100 A
```

Fatto: prima dello scambio la scheda TI 1 con STAT0001 era prossima al fondo
scala, mentre la scheda TI 2 con VEHI001 riportava circa 4.5 A. Dopo lo scambio
i due comportamenti sono rimasti associati alle rispettive schede TI.

Conclusione: la saturazione di `ITANK_MOD` segue la scheda TI 1 e non la
controlCARD. La prova esclude come causa primaria specifica la controlCARD e la
build VEHICLE/STATION. Anche lo zero della scheda TI 2, circa 4.1 A dopo lo
scambio, richiede verifica/calibrazione prima dell'impiego closed-loop.

## Prima applicazione controllata HFC da anello WPT - firmware 1004

Setup:

- Scheda TI 1 + VEHI001 nel ruolo SOURCE;
- Scheda TI 2 + STAT0001 nel ruolo LOAD;
- DCLINK SOURCE alimentato a 12 V con limite 1 A;
- carico 83 ohm sul DCLINK LOAD;
- VBATT scollegate e bobine collegate;
- corrente di bobina SOURCE sintetica impostata a zero;
- misura `ITANK_MOD` LOAD compensata mediante offset;
- riferimenti bus SOURCE e LOAD pari a 12 V;
- limite corrente bobina 2 A;
- limite phase shift HFC 0.020 pu e rampa 1 micro-pu/ciclo ISR;
- finestra di comando HFC pari a 5 s.

La diagnostica iniziale ha confermato attuatore abilitato, nessun fault e
richiesta limitata a 0.020 pu. Al termine dei 5 s il comando e' stato
disabilitato e i PWM sono stati posti in trip. Lo scope sul carico ha misurato
`VMAX = 21 V`; il transitorio iniziale non e' visibile nella cattura a causa
del ritardo di acquisizione. Non sono stati acquisiti valori affidabili delle
correnti durante questa esecuzione. Dopo lo spegnimento sono stati confermati
DCLINK SOURCE = 0 V e VLOAD = 0 V.

Esito: **FAIL per sovratensione LOAD**, avendo superato il limite di arresto
preventivamente fissato a 15 V. Il test non deve essere ripetuto con gli stessi
parametri. La forma del transitorio e le correnti restano non caratterizzate.

### Ripetizione con limite phase shift ridotto - firmware 1005

Prima della prova e' stato corretto lo stop dell'attuatore affinche' azzeri
anche i riferimenti interni di phase shift. Il limite predefinito e' stato
ridotto da 0.020 pu a 0.005 pu. Build complete VEHICLE e STATION, flash e
verifica `FW=1005` sono risultati PASS; a riposo `HENA=0`, `HAPP=0` e il
riferimento CLLLC era zero.

Con lo stesso setup fisico della prova precedente e finestra nominale di 5 s,
lo scope sul carico da 83 ohm ha misurato `VMAX = 20.8 V`. Al termine e' stato
osservato `HFLT=1`: il wrapper aveva quindi rilevato un interlock durante la
finestra e non e' dimostrato che il pilotaggio sia rimasto attivo per tutti i
5 s. Dopo lo spegnimento sono stati confermati DCLINK SOURCE = 0 V e VLOAD =
0 V.

Esito: **FAIL per sovratensione LOAD**. La riduzione del limite phase shift di
un fattore quattro non ha prodotto una riduzione significativa di VMAX (21.0
V contro 20.8 V). Questo confronto non dimostra da solo che il comando phase
shift non venga applicato: in un ponte a duty fisso 50% il phase shift limita
principalmente l'energia trasferibile, mentre la tensione di picco a carico
leggero puo' restare prossima al rapporto di trasformazione. Restano da
separare sperimentalmente risposta a phase shift fisso, comando dinamico UniPD
e arresto dell'attuatore per interlock distribuito.

### Verifica differenziale ai terminali della bobina SOURCE

Con firmware 1006, DCLINK SOURCE a 6 V / 0.5 A e bobine accoppiate, due sonde
passive 10x sono state collegate ai terminali esposti della bobina SOURCE con
entrambe le masse sullo stesso DCLINK-. Lo scope ha calcolato
`MATH = CH1 - CH2`.

Durante finestre ripetute da circa 10 s la diagnostica latched ha confermato
phase shift richiesto, applicato e massimo pari a zero, `F=0` e circa 215 mila
cicli attivi. Le misure dello scope sono state:

```text
MATH forma                 sinusoidale, circa centrata
MATH frequenza             85 kHz
MATH picco-picco           158 Vpp
fase CH1 rispetto a CH2    circa 180 gradi
variazione misura fase     179.5...180.4 gradi, con wrapping +/-180 gradi
```

Esito: **FAIL del mapping HFC/ePWM a comando nullo**. La misura diretta prova
che, quando UniPD fornisce il punto neutro `PSA=PSB=0.5`, il wrapper applica
phase shift zero al driver TI e le uscite fisiche risultano in opposizione di
fase, generando piena eccitazione differenziale del tank. Il Matlab definisce
`PSA=0.5+duty` e `PSB=0.5-duty`: sono i due comandi di ramo, non un singolo
phase shift con origine neutra. Il trasferimento osservato non e' imputabile
alla dinamica del controllore UniPD.
## Caratterizzazione fase HFC e correzione caricamento EPWM2

Con DCLINK SOURCE a 6 V, limite 0.5 A, bobine collegate e carico da 83 ohm sul
DCLINK LOAD, CH1 e CH2 sono stati acquisiti sui due terminali della bobina
SOURCE e MATH e' stata impostata a CH1-CH2. Prima della correzione, variazioni
del riferimento software non modificavano la forma fisica: la diagnostica
aggiunta ha mostrato tick calcolati non nulli ma `PHS2=0`.

Dopo aver reso indipendente da `CLLLC_SECONDARY_ENABLED` la scrittura del
`TBPHS` del secondo ramo primario, il dry-run ha verificato corrispondenza tra
riferimento e registro EPWM2. La caratterizzazione manuale, con run nominali di
10 s, ha fornito:

```text
fase TI [pu]   MATH Vpp   fase scope     forma
0.125          144 V      circa 166 deg  quasi sinusoidale
0.250          114 V      circa 164 deg  sinusoidale
0.375           67 V      circa 152 deg  quasi triangolare
0.425           50 V      circa 136 deg  non annotata
0.450           41 V      70..90 deg     circa triangolare
0.475           31 V      circa 47 deg   non annotata
0.490           26 V      circa 5 deg    non annotata
0.500           19 V      0 deg           onda quadra residua
```

I primi tentativi a 0.425 pu sono stati interrotti prima della misura da un
campo `remoteRole` temporaneamente invalido; non sono inclusi nella tabella. Il
run valido a 0.425 pu e i punti successivi sono terminati senza fault. Al punto
0.500 pu la corrente DCLINK non era osservabile (0 A). La combinazione di Vpp
minima, fase misurata nulla e corrente nulla identifica 0.500 pu come neutro
fisico del driver TI. Le misure di fase dello scope ai punti non sinusoidali
sono riportate come dati accessori e non come fase fondamentale affidabile.

### Verifica automatica del punto neutro e criticita' FSM osservate

Con firmware VEHICLE 1014 e STATION 1007, DCLINK SOURCE inizialmente spento,
sono stati configurati riferimenti SOURCE e LOAD pari a 12 V, limite corrente
2 A, corrente SOURCE sintetica nulla e compensazione dello zero LOAD pari a
4.3 A. Le FSM distribuite hanno raggiunto `PRECHARGE_SOURCE` e
`PRECHARGE_LOAD`; la diagnostica UART riportava ruoli locali SOURCE/LOAD e
`RadioLink=OK` su entrambe le schede.

Il dry-run automatico a DCLINK spento ha prodotto:

```text
modalita' phase shift       automatica (`MAN=0`)
comandi UniPD               PSA=0.500, PSB=0.500
TBPHS EPWM1                 0
TBPHS EPWM2                 46129408 (0.500 pu)
fault                       0
cicli attivi                16785
```

Esito dry-run: PASS per applicazione ePWM del punto neutro automatico.

Con DCLINK SOURCE a circa 6 V sono stati quindi eseguiti tre tentativi dopo
configurazione; prima dell'ultimo entrambe le schede sono state sottoposte a
power-cycle e le FSM sono state riconfigurate. In ogni tentativo l'abilitazione
iniziale e' stata accettata con `HFLT=0`, ma durante la finestra il wrapper ha
registrato in modo ripetibile:

```text
fault latched               F=3 (ruolo remoto invalido)
campioni ruolo non valido   RRINV=2125
stato locale prima del run  PRECHARGE_SOURCE
stato remoto prima del run  PRECHARGE_LOAD
radio prima del run         OK su entrambe le schede
```

Il power-cycle non ha modificato l'esito. La finestra UART richiesta era di
10 s, ma l'attivazione fisica e' stata interrotta anticipatamente
dall'interlock; pertanto questi tentativi non costituiscono run HFC continui da
10 s.

Nell'ultimo tentativo lo scope ha misurato `MATH=19 Vpp`, forma quadra e fase
CH1-CH2 pari a 359.8 gradi, equivalente al punto di fase nullo entro la
risoluzione della misura. La diagnostica finale confermava `PSA=PSB=0.500`,
`PHS2=46129408`, richiesta e picco applicato nulli e DCLINK acquisito pari a
5.980 V. Esito: PASS per conferma elettrica del punto neutro automatico; non e'
stato osservato trasferimento comandato.

Fatti relativi alla state machine/interfaccia distribuita:

- prima di ogni run i ruoli locali e gli stati FSM risultavano coerenti con
  SOURCE/LOAD e PRECHARGE_SOURCE/PRECHARGE_LOAD;
- il collegamento radio risultava operativo su entrambe le schede;
- durante la commutazione il campo di ruolo remoto e' risultato non valido per
  2125 campioni consecutivi, provocando sempre il fault 3;
- la stessa anomalia era gia' comparsa sporadicamente durante la
  caratterizzazione manuale a 0.425 pu;
- il reset completo e la riconfigurazione hanno ripristinato temporaneamente
  `F=0` e `RRINV=0`, senza impedire la ricomparsa durante il run successivo.

Inferenza limitata ai dati di test: la criticita' e' associata alla
disponibilita'/coerenza runtime del ruolo remoto durante l'attivita' HFC, non
alla configurazione iniziale dei ruoli locali. Queste prove non identificano
la causa interna alla FSM o al trasporto radio e non definiscono modifiche al
codice della state machine.

## Verifica diagnostica atomica WPT - firmware 1020

Condizioni: DCLINK OFF, VLOAD 0 V, controllo di potenza non attivo.

Entrambe le immagini sono state prodotte con full clean, programmate e
interrogate sulle rispettive UART:

- COM25: `FW=1020`; `WPTSNAP?` ha restituito una riga completa;
- COM26: `FW=1020`; `WPTSNAP?` ha restituito una riga completa.

PASS limitato all'installazione del firmware e all'operativita' del nuovo
comando diagnostico. Le letture a riposo hanno confermato il canale fisico
SOURCE anomalo (circa 69 A equivalenti) e che, dopo la programmazione, l'offset
LOAD deve essere nuovamente configurato prima delle prove. Non e' stato
abilitato alcun PWM e non e' stata eseguita una prova di potenza.

### HFC closed-loop a DCLINK 30 V e riferimento LOAD 15 V

Setup: DCLINK SOURCE 30 V / 5 A, carico LOAD 4.6 ohm, corrente SOURCE
sintetica nulla, corrente LOAD fisica con offset 4.300 A, limite corrente
UniPD 5 A, limite phase shift 0.500 pu, rampa 100 micro-pu/ciclo e seed del
limite di potenza SOURCE pari a 90 W.

Nel primo tentativo un `WPTSNAP?` atomico acquisito a circa 3 s ha riportato:

```text
IPHY       4.539 A
IZERO      4.300 A
IUSE       0.239 A
IREF       4.25 A
IERR       4.02 A
PREM       15 W
PREF       2 W
radio      OK, NOACK=0
```

La relazione `IREF - IUSE` e' coerente con `IERR` entro la risoluzione della
telemetria. Il run comandato e' durato 10.015 s, ma la FSM e' transitata in
scarica/arresto durante la finestra e il wrapper ha concluso con `HFLT=6`;
non costituisce quindi un'attivazione HFC continua di 10 s.

Dopo reset e riconfigurazione completa e' stato eseguito un secondo run senza
diagnostica UART durante la finestra. Durata comando: 10.022 s. Misure:

```text
VLOAD multimetro             circa 14 V
plateau scope                circa 15.5 V
VMAX scope                   17.7 V
IDCLINK osservata            circa 2.8 A
forma                        plateau con diversi dropout
stato finale                 RadioLink=FAIL, HFLT=5
```

Con 4.6 ohm, i valori strumentali corrispondono indicativamente a 42.6 W sul
carico a 14 V e 52.2 W al plateau di 15.5 V. L'ingresso indicativo e' 84 W
usando 30 V e 2.8 A; questi valori non costituiscono una misura di rendimento
sincronizzata.

Esito parziale: PASS per raggiungimento sostanziale del riferimento LOAD di
15 V; FAIL per continuita' del trasferimento, essendo intervenuti dropout e
perdita del collegamento nRF. Al termine sono stati confermati HFC OFF e
VLOAD=0 V.

Dopo ulteriori reset e ripetizioni e' stato ottenuto un run completo senza
arresto HFC, della durata di 10.026 s:

```text
DCLINK SOURCE                30 V
riferimento LOAD             15 V
VLOAD multimetro             circa 14 V
plateau scope                circa 15.5 V
VMAX scope                   16.9 V
IDCLINK                      circa 2.8 A
fault HFC finale             0
```

Le potenze indicative, non sincronizzate, sono circa 84 W in ingresso,
42.6 W sul carico usando la lettura multimetro di 14 V e 52.2 W usando il
plateau scope di 15.5 V.

In un run precedente allo stesso punto operativo, concluso senza fault, lo
snapshot atomico LOAD a circa 3 s aveva misurato `IPHY=4.100 A`,
`IZERO=4.300 A` e `IUSE=0 A`. Il valore fisico era quindi inferiore all'offset
software ed e' stato clampato a zero, nonostante il trasferimento di potenza
osservato. Fatto: l'attuale combinazione di catena analogica, conversione e
offset software non fornisce una misura utile di corrente di bobina in questo
punto operativo. Questa osservazione non dimostra da sola quale contributo
derivi dall'hardware analogico, dalla calibrazione o dalla variabilita'
temporale del segnale.

Al termine del run sono stati confermati DCLINK OFF e VLOAD=0 V.

## Caratterizzazione statica HFC manuale sotto carico

Obiettivo: verificare sperimentalmente il segno e l'autorita' statica della
catena `phase shift -> ponte HFC -> bobine -> rectifier passivo -> carico`,
escludendo il regolatore UniPD dalla generazione del comando HFC.

Setup:

- SOURCE: scheda 1 con controlCARD VEHI001;
- LOAD: scheda 2 con controlCARD STAT001;
- DCLINK SOURCE: 20 V, limite alimentatore 10 A;
- carico sul DCLINK LOAD: 2.2 ohm / 250 W;
- BOOST e BUCK non attivi;
- HFC in modalita' phase shift manuale tramite `WPTHFCPH`;
- durata di ogni run valido: circa 10 s;
- scope sul lato SOURCE, CH1 e CH2 riferiti a DCLINK-, MATH=CH1-CH2;
- frequenza MATH misurata: circa 85 kHz.

La convenzione verificata e' `500 mpu = punto neutro` e valori decrescenti
corrispondono a eccitazione crescente. I valori consolidati sono:

```text
WPTHFCPH  MATH ampl.  MATH Vpp  fase CH1-CH2  VLOAD    IDCLINK
[mpu]     [V]         [V]        [deg]          [V]      [A]
500       28.2        40.2       0              0        0.01
490       10.6        65         4              0.002    0.01
475       29.8        83        -10             0.014    0.02
450       85.2        114        90             0.06     0.02
425       78.8        >131       113            0.40     0.10
400       110         135        167            0.90     0.22
375       129         129        181            1.30     0.35
325       115         131       -176            2.10     0.60
250       128         141        178            3.12     0.98
125       165         n.d.       177..180       4.30     1.50
0         217         217        176            4.80     1.73
```

Le letture automatiche iniziali di fase pari a 260 gradi a 375 mpu e
122 gradi a 325 mpu non sono risultate ripetibili. Le ripetizioni hanno
fornito rispettivamente 181 gradi e -176 gradi; le prime letture sono state
classificate come errori di aggancio della misura automatica dello scope su
forme non sinusoidali. I valori prossimi a +180 e -180 gradi sono equivalenti
considerando il wrapping della misura.

Fatti di test:

- la tensione e la corrente DC sul carico aumentano riducendo il comando da
  500 a 0 mpu;
- il mapping ePWM applica i punti manuali richiesti, verificati anche tramite
  il registro diagnostico `PHS2`;
- non sono intervenuti fault HFC durante i run validi riportati;
- al massimo comando manuale, VLOAD=4.8 V e IDCLINK=1.73 A coincidono
  sostanzialmente con il limite osservato nelle prove closed-loop sullo stesso
  setup;
- al punto massimo le potenze indicative, non sincronizzate, sono circa
  34.6 W in ingresso (`20 V * 1.73 A`) e 10.5 W sul carico
  (`4.8 V^2 / 2.2 ohm`);
- gli snapshot LOAD di `ITANK_MOD`, acquisiti in singoli istanti dei diversi
  run, non hanno mostrato una crescita monotona con VLOAD e IDCLINK e sono
  risultati spesso inferiori all'offset software di 4.300 A, quindi clampati
  a zero.

Esito: PASS per il segno del plant e per la monotonia della relazione
`comando phase shift -> VLOAD/IDCLINK`. Il mancato raggiungimento del
riferimento LOAD di 15 V con 20 V e 2.2 ohm non e' attribuibile a una
inversione del mapping del phase shift: il limite closed-loop coincide con
il massimo ottenibile mediante comando manuale nel setup provato.

Al termine dell'ultimo run HFC e' stato disabilitato senza fault. Il DCLINK
SOURCE e' rimasto alimentato; non e' stata dichiarata la conclusione in
sicurezza del setup.

## Verifica FW1024: capture ISR WPT e shadow HFC

Condizioni: DCLINK SOURCE OFF; nessun trasferimento di potenza.

Le immagini VEHICLE e STATION sono state generate con full clean, programmate
e interrogate sulle UART:

```text
COM25 FW=1024
COM26 FW=1024
```

### Buffer locale sincronizzato

Su entrambe le schede e' stata impostata una decimazione pari a 213 cicli
ISR2. Dopo arm, breve acquisizione e stop:

```text
VEHICLE COUNT=36, LEN=96, DEC=213, FROZEN=1
STATION COUNT=37, LEN=96, DEC=213, FROZEN=1
```

`WPTCAPD?` ha restituito i campioni in ordine cronologico su entrambe le
UART. Il dump e' stato eseguito solo dopo il congelamento del buffer. Sono
risultati presenti i campi previsti per ruolo, stato, link, VDC/VLOAD,
ITANK_MOD fisica e usata, PREF, IREF, IERR, VAC, HREQ, HAPP, HAUTO, HPHY,
modalita' manuale e fault.

Esito: PASS per arm, campionamento ISR locale, stop/freeze, conteggio,
decimazione configurabile e dump UART differito.

### Shadow manuale

Dopo reset e configurazione valida della FSM sono stati verificati:

```text
SOURCE State=SOURCE_ON, Role=SOURCE, RadioLink=OK
STATION State=LOAD_ON, Role=LOAD, RadioLink=OK
```

Sul SOURCE e' stato impostato `WPTHFCPH=325`, il buffer e' stato armato con
decimazione 21 e HFC e' stato abilitato per circa 80 ms con DCLINK OFF. Il
buffer e' stato congelato prima della disabilitazione HFC.

Nei campioni con attuatore attivo:

```text
MAN=1
HAUTO=500 mpu
HPHY=325 mpu
HREQ=0 mpu
HAPP=0 mpu
FAULT=0
```

Fatto: UniPD ha mantenuto esposte richiesta e fase automatica calcolata,
mentre la fase fisica applicata e' rimasta quella manuale. Esito: PASS per
la separazione diagnostica shadow e per il mantenimento del comando manuale.
La prova e' limitata a richiesta automatica nulla e DCLINK OFF; il confronto
dinamico con HREQ non nulla richiede una successiva prova operativa.

Al termine HFC e' stato disabilitato senza fault e DCLINK e' rimasto OFF.

### Shadow manuale sotto potenza reale

Setup:

- SOURCE: scheda 1 / VEHI001, DCLINK alimentato a 20 V con limite 10 A;
- LOAD: scheda 2 / STAT001, carico resistivo 2.2 ohm;
- bobine collegate, BOOST e BUCK non coinvolti;
- phase shift fisico manuale `WPTHFCPH=250`;
- seed SOURCE `WPTPLIMINIT=120 W`;
- seed LOAD `WPTPLOADINIT=60 W`;
- capture locale su entrambe le schede, lunghezza 96 campioni e
  decimazione 213 cicli ISR2;
- durata effettiva del comando HFC: 5.046 s.

Prima dell'avvio HFC, la diagnostica ha confermato:

```text
SOURCE: PLIM=120 W
LOAD:   PREM=120 W, PREF=68 W, IREF=10.00 A
```

I due buffer sono stati congelati prima della disabilitazione HFC. Entrambi
hanno restituito `COUNT=96`, `FROZEN=1`.

Misure esterne dichiarate dall'operatore:

```text
IDCLINK = 0.98 A
VLOAD   = 3.0 V
VMAX    = 4.12 V (oscilloscopio)
durata  = 5 s
forma   = rettangolare, plateau circa 3.25 V
```

Intervalli osservati nel capture locale:

```text
SOURCE:
  VDC       = 19.922 ... 20.114 V
  HREQ      = 453 ... 500 mpu
  HAPP      = 494 ... 500 mpu
  HAUTO     = 0 ... 5 mpu
  HPHY      = 250 mpu costante
  MAN       = 1
  FAULT     = 0

LOAD:
  VDC/VLOAD = 3.101 ... 3.293 V
  PREF      = 7 ... 13 W
  IREF      = 3.80 ... 6.92 A
  IUSE      = 0 ... 0.660 A
  FAULT     = 0
```

Fatti:

- la misura VLOAD del capture e' coerente con i 3.0 V misurati esternamente;
- il plateau osservato sullo scope, circa 3.25 V, e' coerente con
  l'intervallo 3.101...3.293 V del capture; VMAX=4.12 V rappresenta il
  massimo transitorio e non il valore di regime;
- il comando fisico e' rimasto a 250 mpu per tutta la finestra;
- contemporaneamente UniPD ha portato la propria richiesta HFC vicino alla
  saturazione e la fase automatica calcolata fino a 494...500 mpu;
- la conversione diagnostica `HAUTO=500-HAPP` ha prodotto 0...5 mpu;
- nessun fault e' stato registrato;
- la lettura fisica SOURCE `IPHY` e' rimasta non valida, circa 64...69 A,
  ed e' stata esclusa dal controllo (`IUSE=0`);
- sul LOAD la corrente compensata usata dal controllo e' risultata rumorosa
  e discontinua, 0...0.660 A, molto inferiore al riferimento calcolato.

Esito: PASS per il capture ISR sincronizzato sotto potenza reale e per la
modalita' shadow. La prova dimostra che richiesta UniPD, attuazione automatica
calcolata e phase shift fisico manuale sono osservabili separatamente durante
un trasferimento reale. La saturazione di HREQ/HAPP, con HPHY mantenuto a
250 mpu, non costituisce una validazione delle prestazioni closed-loop ma
fornisce il confronto richiesto fra comando calcolato e comando imposto.

Al termine i buffer sono stati congelati, quindi HFC e' stato disabilitato
senza fault. Il DCLINK SOURCE e' rimasto alimentato a 20 V.
