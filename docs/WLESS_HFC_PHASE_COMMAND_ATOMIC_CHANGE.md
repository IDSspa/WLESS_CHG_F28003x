# Pubblicazione atomica del comando di phase-shift HFC

## Motivazione

Il comando `CLLLC_hfcReceiverTestPhaseShiftPrimLegs_pu` e' un valore floating
point a 32 bit scritto dal controllo UniPD a 1 kHz e letto dall'ISR HFC a
80 kHz. Sul C28x l'accesso alla memoria dati avviene a parole di 16 bit: senza
coordinamento il lettore puo' osservare una scrittura parziale. Inoltre l'ISR
riscriveva la stessa variabile dopo il clamp, creando due scrittori sullo
stesso dato condiviso.

## Modifica

La pubblicazione e' ora protetta da un sequence lock a 16 bit:

- il writer rende la sequenza dispari, scrive il `float`, quindi rende la
  sequenza nuovamente pari;
- l'ISR legge sequenza, dato e sequenza;
- se la sequenza e' dispari o e' cambiata, l'ISR conserva l'ultimo comando
  valido invece di attendere;
- l'ISR non riscrive piu' la variabile pubblicata.

La soluzione non disabilita gli interrupt, non contiene cicli di attesa e
aggiunge soltanto pochi accessi a 16 bit al percorso real-time.

## Stato di verifica

La modifica e' stata verificata sull'hardware, ma da sola non ha eliminato i
dropout ciclici osservati nel closed-loop HFC. Ha comunque rimosso una corsa
reale fra il writer a 1 kHz e il reader ISR a 80 kHz e resta quindi parte della
baseline corretta. La successiva stabilizzazione del run e' stata ottenuta con
la correzione della pipeline ACK descritta nel relativo change document.
