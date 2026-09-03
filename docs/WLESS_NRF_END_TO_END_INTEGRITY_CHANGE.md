# Integrita' end-to-end del payload operativo nRF

## Scopo

La modifica introduce una diagnostica end-to-end per distinguere payload
duplicati o precedenti da alterazioni avvenute prima o dopo il tratto radio.
Non modifica gli algoritmi UniPD, la FSM o il comando HFC.

## Formato

Il payload operativo passa da 9 a 13 byte. Ai nove byte esistenti vengono
aggiunti:

- sequenza applicativa a 16 bit;
- CRC16-CCITT, polinomio `0x1021`, inizializzazione `0xFFFF`, calcolato sui
  primi 11 byte.

La transazione SPI risultante e' di 14 byte, inferiore al FIFO SPI da 16 word.
Entrambe le immagini VEHICLE e STATION devono usare la stessa release.

## Comportamento

Un payload con CRC applicativo errato viene scartato e non aggiorna i campi
remoti. Le anomalie di sequenza vengono contate ma il payload con CRC valido
resta pubblicato, così da osservare l'eventuale correlazione con i dropout.

`RADIO?` aggiunge:

- `ATX`, `ARX`: ultima sequenza applicativa trasmessa/ricevuta;
- `ADEL`: delta dell'ultima sequenza ricevuta;
- `ACRC`: errori CRC applicativi;
- `ASEQERR`: delta di sequenza diverso da uno;
- `ZSEQ`, `ZDEL`: sequenza e delta associati all'ultimo zero di potenza
  ricevuto dopo un valore operativo.

## Correzione refresh ACK payload (release 1064)

La configurazione PRX `0x3B` maschera l'interrupt `TX_DS`. Dopo `RX_DR`, il
completamento dell'ACK puo' quindi non produrre una nuova transizione sul pin
IRQ. La prima integrazione attendeva correttamente `TX_DS` prima di ricaricare
il FIFO ACK, ma non richiamava piu' il servizio radio quando rimaneva pendente
soltanto tale aggiornamento. Il FIFO si esauriva e la VEHICLE conservava dati
remoti obsoleti, fino a bloccare anche la sequenza `INITOK`.

Dalla release 1064 la STATION continua a interrogare `STATUS` mentre
`WLESS_NRF24_ackRefreshPending` e' attivo. Quando osserva `TX_DS`, accoda il
nuovo payload senza toggle di CE e senza flush del FIFO. La modifica interessa
solo il servizio in background e non aggiunge lavoro alle ISR.

### Esito banco e revisione 1065

La verifica a freddo della 1064 ha mostrato `ACKP=1` e `TX=0`: nel PRX usato
sul banco `TX_DS` non viene osservato come conferma dell'ACK automatico. La
1064 non completa quindi il refresh ed e' da considerare non valida per i test
distribuiti.

La release 1065 ricarica il payload ACK subito dopo la lettura del pacchetto RX,
ma soltanto dopo avere verificato che il TX FIFO non sia pieno. Restano vietati
toggle di CE e `FLUSH_TX` durante il traffico. Snapshot, sequenza e CRC
end-to-end restano invariati.

### Esito closed-loop FW1065

Nel banco a DCLINK 24 V, carico 8 ohm e riferimenti SOURCE/LOAD pari a 10 V,
la release 1065 ha completato un run closed-loop HFC di 30 s senza dropout
fisici osservabili. VLOAD e' rimasta circa 9,9...10 V, ILOAD circa 1,25 A e
IDCLINK ha mostrato un lento assestamento circa 1,30...1,12 A con lieve
successiva risalita.

Il risultato rende la correzione della pipeline ACK il principale elemento
correlato alla rimozione dei dropout nel run verificato. Restano tuttavia
elevati i conteggi CRC applicativi scartati sul verso ACK: poiche' i payload
non validi non vengono pubblicati e non hanno prodotto dropout fisici nel run,
questa anomalia diagnostica resta aperta e non va confusa con una validazione
completa dell'integrita' del link.
