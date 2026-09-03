# Modifica clamp UniPD HFC e bypass rampa

## Scopo

Ripristinare la semantica firmata del riferimento AC prevista da
`Controllo_Sistema_WPT_funzione_v14.m`, mantenendo esterna all'algoritmo la
protezione unidirezionale del banco, e permettere di disabilitare la rampa HFC.

## Modifiche funzionali

- `Vac_rif` UniPD e' limitata in `[-Vac_max,+Vac_max]` per default;
- il limite di corrente locale opera su `abs(Vac_rif)` e conserva il segno;
- il wrapper continua a limitare la richiesta fisica negativa al punto neutro;
- `WPTCLAMP=1` abilita il clamp inferiore legacy a zero;
- `WPTCLAMP=0` seleziona la trasposizione firmata, default corrente;
- `WPTHFCRAMP=0` applica direttamente `HAPP=HREQ`;
- `WPT?` espone `CLAMP`, `VACRAW_mV` e `VAC_mV`;
- il capture WPT riporta come `VACRAW_mV` l'uscita dell'equazione prima dei
  clamp e mantiene separati comando UniPD, mapping, richiesta e applicazione.

I comandi di configurazione disabilitano HFC prima di cambiare clamp o rampa.
La modalita' firmata non abilita il trasferimento fisico inverso.

## Validazione richiesta

La modifica richiede clean build e flash di entrambi i ruoli, quindi confronto
closed-loop a parita' di setup fra:

1. `WPTCLAMP=1`, rampa corrente;
2. `WPTCLAMP=0`, rampa corrente;
3. `WPTCLAMP=0`, `WPTHFCRAMP=0`.

Durante i run acquisire almeno `IERR`, `VACRAW`, `VAC`, `HREQ`, `HAPP`, stato
FSM e grandezze fisiche. Non e' stata eseguita alcuna build o prova hardware
nell'ambito di questa modifica.
