# Correzione pipeline ACK nRF introdotta dall'integrazione

## Scopo

Ridurre le anomalie macroscopiche osservate sul percorso ACK payload
STATION -> VEHICLE senza modificare il protocollo applicativo originale, la
state machine o gli algoritmi UniPD.

La modifica riguarda esclusivamente aspetti introdotti o rielaborati durante
l'integrazione del driver nRF nel firmware WLESS_CHG.

## Problema osservato

Nel port integrato l'aggiornamento dell'ACK payload STATION dipendeva dalla
seguente sequenza:

```text
ricezione payload
  -> ackRefreshPending
  -> attesa TX_DS
  -> CE OFF
  -> FLUSH_TX
  -> caricamento nuovo ACK payload
  -> CE ON
```

Con traffico operativo ad alta frequenza questa sequenza introduce una finestra
durante la quale una nuova interrogazione puo' trovare il FIFO TX vuoto o in
corso di modifica. Era inoltre presente un secondo percorso di recupero per il
caso `TX_EMPTY`, rendendo non univoco il momento di ricarica dell'ACK.

I test hanno mostrato una degradazione macroscopica, non riconducibile a un
errore radio occasionale: sulla VEHICLE sono stati osservati migliaia di
payload semanticamente invalidi e incrementi rilevanti di `MAXRT`, mentre la
STATION non registrava analoghi payload invalidi nel verso opposto.

Questa asimmetria rende prioritario il percorso ACK payload.

## Correzione applicata

La STATION usa ora una pipeline deterministica a un pacchetto:

1. l'ACK relativo al pacchetto corrente e' quello precaricato in precedenza;
2. `RX_DR` richiede l'aggiornamento senza modificare il FIFO TX;
3. `TX_DS` prova che la trasmissione dell'ACK corrente e' terminata;
4. il firmware verifica che il FIFO TX non sia pieno;
5. accoda direttamente un solo ACK aggiornato per la prossima interrogazione.

Durante il traffico normale non vengono piu' eseguiti:

- commutazione CE OFF/ON per aggiornare l'ACK;
- `FLUSH_TX` dopo `TX_DS`;
- percorso alternativo di ricarica dipendente da `ackRefreshPending`.

`FLUSH_TX` resta utilizzato in inizializzazione e nella gestione `MAX_RT`, dove
ha funzione di recupero esplicita.

## Coerenza del payload TX

La serializzazione acquisisce ora una sola volta ciascuna variabile condivisa
in una variabile locale prima di separare i valori a 16 bit nei byte alto e
basso. Questo impedisce che un aggiornamento ISR produca un valore composto da
due campioni differenti.

La modifica non crea uno snapshot temporale globale fra tutte le grandezze, ma
elimina la corruzione interna di ciascun valore a 16 bit.

## Elementi invariati

- payload applicativo originale da 9 byte;
- CRC hardware nRF;
- ruoli VEHICLE/STATION;
- semantica dei campi FSM;
- cadenza diagnostica e operativa;
- configurazione SPI a 1 MHz;
- watchdog e criteri `RadioLink`;
- FSM, UniPD, HFC e mapping ePWM.

## Diagnostica conservata

`RADIO?` continua a esporre:

- `RXVALID` e `RXINVALID`;
- `MAXRT` e `NOACK`;
- rifiuti per TX busy/FIFO;
- stato FIFO e registri nRF.

`ackRefreshPending=1` indica che e' stato ricevuto un nuovo campione e si attende
il completamento `TX_DS` dell'ACK corrente. Torna a zero dopo l'accodamento del
campione successivo. Non provoca attivita' SPI autonoma nel background.

## Esito intermedio FW1047

La prima variante FW1047 accodava il nuovo ACK immediatamente dopo aver letto
il FIFO RX. Lo stress senza e con HFC ha eliminato i payload invalidi, ma una
successiva inizializzazione ha mostrato che la VEHICLE poteva conservare
`REMOTE_ROLE=0` e `REMOTE_CTRL=0` per decine di migliaia di pacchetti prima di
ricevere lo stato LOAD/WPTON aggiornato.

Il risultato dimostra che la scrittura eseguita dopo `RX_DR` poteva precedere il
completamento fisico dell'ACK. La variante FW1048 introduce pertanto la
sincronizzazione `RX_DR -> pending -> TX_DS -> W_ACK_PAYLOAD`, senza ripristinare
il flush o la commutazione CE.

## Verifiche richieste

### Regressione software, stadio HFC disabilitato

1. power cycle corretto di entrambe le schede;
2. verificare `FW? = 1048` e `ROLE?`;
3. acquisire `RADIO?` iniziale;
4. predisporre il DCLINK al minimo valore necessario per soddisfare in modo
   controllato la guardia VBUS e raggiungere `SOURCE_ON`;
5. completare la sequenza FSM che abilita la cadenza operativa a 1 kHz;
6. mantenere `WPTHFC=0`, carico e conversione di potenza disabilitati;
7. osservare il solo traffico operativo per almeno 60 s;
8. acquisire `RADIO?` finale su entrambe e porre DCLINK OFF.

Il DCLINK non puo' essere semplicemente lasciato a 0 V in questa prova: con la
guardia analogica reale la SOURCE non raggiungerebbe `SOURCE_ON` e la cadenza
radio resterebbe diagnostica, circa 1 Hz. Una prova a DCLINK OFF richiede invece
uno stimolo radio dedicato equivalente e non valida direttamente lo scheduler
operativo della FSM.

Criterio preliminare: nessuna crescita macroscopica di `RXINVALID`, `MAXRT`,
`NOACK` o rifiuti FIFO. Un evento isolato deve essere annotato ma non viene
equiparato alla degradazione precedentemente osservata.

### Discriminazione elettrica SPI/RF

Solo se la regressione precedente fallisce:

1. ripetere senza modificare cablaggi con `WLESS_NRF24_SPI_BITRATE` ridotto da
   1 MHz a 500 kHz;
2. confrontare separatamente `RXINVALID` e `MAXRT`;
3. se possibile osservare SCLK, MISO, CSN e alimentazione 3,3 V del modulo;
4. ripetere con HFC e DCLINK disabilitati, quindi con DCLINK alimentato ma HFC
   disabilitato.

Interpretazione:

- forte riduzione di `RXINVALID` a SPI piu' lenta: sospetto timing/integrita'
  elettrica SPI;
- crescita di `MAXRT` senza `RXINVALID`: sospetto prevalentemente RF;
- errori correlati all'abilitazione HFC: sospetto EMI/alimentazione;
- errori gia' presenti con DCLINK e HFC OFF: causa non dipendente dallo stadio
  di potenza.

## Stato

Implementato nel sorgente, firmware release 1048. Build, flash e validazione
hardware ancora da eseguire.

## Diagnostica burst FW1049

La release 1049 aggiunge esclusivamente contatori diagnostici; non modifica il
payload radio, il watchdog o il controllo HFC:

- `RXIRUN`: numero corrente di payload semanticamente invalidi consecutivi;
- `RXIMAX`: massimo numero di invalidi consecutivi osservato dall'avvio;
- `NOACKMAX`: massimo valore raggiunto da `WLESS_SM_noAckCount` dall'avvio.

Un payload valido azzera `RXIRUN`, mentre `RXIMAX` resta latched. `NOACKMAX`
viene aggiornato sia dal percorso VEHICLE `MAX_RT` sia dal watchdog diagnostico
STATION. I tre valori sono pubblicati da `RADIO?` e vengono azzerati soltanto
dall'inizializzazione nRF/FSM conseguente a reset o power cycle.

Questi campi permettono di distinguere errori isolati da raffiche senza
inferire la distribuzione temporale dal solo totale `RXINVALID`.
