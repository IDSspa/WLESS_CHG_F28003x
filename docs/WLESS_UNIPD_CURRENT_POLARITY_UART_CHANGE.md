# Correzione UART della polarità delle correnti UniPD

## Scopo

La modifica permette di correggere a runtime la convenzione di segno delle
misure fisiche `IL_A` e `IL_B` fornite al controllo UniPD BOOST/BUCK.

Non modifica i coefficienti UniPD, la mappatura duty/PWM o la conversione
analogica. La configurazione non è persistente: dopo un reset torna al valore
predefinito `0`.

## Comandi

```text
UP?
UP=0
UP=1
UP=2
UP=3
```

Il valore è una maschera:

- `0`: nessuna inversione;
- `1`: inverte `IL_A`;
- `2`: inverte `IL_B`;
- `3`: inverte entrambe.

Valori maggiori di `3` producono `UP RANGE`.

Il comando di scrittura è rifiutato con `UP REJECTED ACTIVE` se il BBC,
il controllo di potenza UniPD o il docking test sono attivi. Questo evita
una variazione istantanea del segno della retroazione durante il pilotaggio.

## Punto di applicazione

Le grandezze raw sono calcolate dalle misure scalate:

```text
ADC -> conversione fisica -> IL raw -> correzione UP -> UniPD/trip/capture
```

Gli override sintetici sono applicati successivamente e quindi non sono
alterati dalla maschera, che riguarda esclusivamente la polarità della catena
fisica.

## Diagnostica

La risposta estesa `UQ?` mantiene `IL_A` e `IL_B` corretti nei campi esistenti
e aggiunge in coda:

```text
UP_mask, IL_A_raw_mA, IL_B_raw_mA
```

Il buffer `CAPD?` registra i valori corretti, cioè gli stessi valori usati dal
controllore e dalle protezioni software.

