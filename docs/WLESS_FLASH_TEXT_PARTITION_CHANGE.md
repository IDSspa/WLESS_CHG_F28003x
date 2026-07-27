# Estensione immediata della partizione `.text`

## Problema

Il linker confinava la sezione `.text` all'intervallo
`0x082000–0x085FFF`, corrispondente ai settori Flash Bank 0 SEC2–SEC5.
Il limite riguardava quindi la singola regione assegnata e non la capacità
Flash complessiva del TMS320F28003x.

## Verifica del partizionamento

Nel linker corrente:

- SEC0 contiene l'entry point;
- SEC1 contiene inizializzazione, funzioni copiate in RAM e DCL;
- SEC2–SEC5 contengono `.text` e il load image delle costanti CLA;
- SEC6 contiene costanti e programma CLA;
- SEC7 contiene stringhe UART e load image delle ISR;
- SEC8–SEC9 contengono il codice UART;
- SEC10–SEC15 non avevano alcuna sezione assegnata;
- Flash Bank 1 e Bank 2 non avevano alcuna sezione assegnata.

Nel progetto non sono state trovate configurazioni di boot o programmazione
che limitino il caricamento a SEC0–SEC9.

## Modifica

La sezione `.text` può ora essere distribuita su:

```text
Bank 0 SEC2–SEC5
Bank 0 SEC10
Bank 0 SEC11
```

Non vengono modificate le sezioni specializzate SEC6–SEC9.

## Margine preservato

Restano intenzionalmente non assegnati:

- Bank 0 SEC12–SEC15;
- tutti i settori di Bank 1;
- tutti i settori di Bank 2.

La scelta non definisce ancora la futura area di configurazione persistente.
Quella partizione dovrà essere stabilita insieme alla strategia di emulazione
EEPROM, aggiornamento firmware e recupero da scrittura interrotta.

