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

Condizione di test:

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

Verifica dinamica:
- modificando `TTPLPFC_bbcDockTestDuty1_pu`, cambia il duty di EPWM6
- modificando `TTPLPFC_bbcDockTestDuty2_pu`, cambia il duty di EPWM7
- riportando `TTPLPFC_bbcDockTestEnable = 0`, i quattro segnali EPWM6A/B ed EPWM7A/B spariscono

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

Stato:
Da eseguire.

Mapping previsto:

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

## Test 11 - Synthetic UniPD Docking Test

Obiettivo:
Simulare gli input UniPD mancanti tramite variabili modificabili da CCS, portare `UNIPD_bbcSignalMissingMask` a zero e verificare la risposta dell'algoritmo senza hardware di potenza.

Stato:
Da progettare/eseguire dopo il Test 10.

