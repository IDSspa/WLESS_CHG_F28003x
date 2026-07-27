# WLESS UniPD - correzione limite duty UART

## Scopo

Correggere il rifiuto del comando `UE=1` osservato con il valore nominalmente
valido:

```text
UD=950
```

La modifica non aumenta il limite massimo del duty e non cambia il pilotaggio
BUCK/BOOST.

## Problema osservato

Con firmware 1024:

1. `UD=950` veniva accettato dal parser;
2. `UQ?` riportava duty massimo pari a 950 mpu;
3. il successivo `UE=1` non abilitava il controllo di potenza;
4. non veniva restituito alcun messaggio di rifiuto;
5. `UD=949` consentiva invece l'abilitazione.

Il consenso all'abilitazione confrontava il valore floating-point derivato da
`UD` con `0.95f`. Il valore di configurazione UART era gia' disponibile come
intero in milli-pu e gia' limitato a 950.

## Modifica

- il consenso usa ora il valore intero
  `WLESS_UART_unipdDutyMaxMilli <= 950U`;
- la variabile UART viene inizializzata a 350 mpu, coerentemente con il valore
  iniziale del limite UniPD;
- se `UE` non soddisfa le condizioni di consenso viene emesso:

```text
UE REJECTED CONFIG
```

- `FIRMWARE_RELEASE` viene incrementato da 1024 a 1025.

Il clamp fisico applicato dal percorso UniPD resta pari a 0.95.

## Verifiche richieste

1. clean build completa VEHICLE;
2. programmazione della controlCARD;
3. verifica `FW? -> FW=1025`;
4. con uscita disabilitata, impostazione `UD=950`;
5. verifica `UQ?` del limite;
6. prova controllata `UE=1`;
7. verifica che PWM/BBC risultino abilitati e arresto con `UE=0`;
8. controprova di un rifiuto configurativo e presenza del messaggio
   `UE REJECTED CONFIG`.

## Verifica hardware

La build VEHICLE e' stata eseguita dopo clean completa e programmata sulla
controlCARD `VEHI0001`.

Dopo power-cycle:

```text
FW?    -> FW=1025
UP?    -> UP=0
UD=950 -> accettato
```

Con DCLINK a circa 20 V, carico da 83 ohm su VBATT, `UH=2`, `UA=400`,
`UV=18000`, `UX=24000`, `UC=1000` e `UN=22`, il comando `UE=1` ha prodotto:

```text
VDC                = 20.082 V
VBATT              = 19.376 V
PowerOutputEnable  = 1
BBC enabled        = 1
duty max           = 0.950
duty applicati     = 0.950 / 0.950
duty rampati       = 0.950 / 0.950
fault VDC/IL       = 0 / 0
```

Misure esterne:

```text
VBATT multimetro = 19.6 V
IDCLINK          = 0.23 A
VMAX scope       = 20.4 V
Vpp complessivo  = 21.1 V
plateau          = stabile
```

Il run e' stato arrestato mediante `UE=0` senza fault.

Esito: PASS per l'abilitazione al limite nominale `UD=950`.

La controprova del messaggio `UE REJECTED CONFIG` resta separata e non e'
necessaria per dimostrare la correzione del bordo 0.95.
