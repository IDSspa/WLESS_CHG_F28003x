# Cattura diagnostica UniPD pre-trip

## Scopo

La modifica aggiunge una cattura firmware ad alta frequenza delle grandezze
BOOST immediatamente precedenti al trip. Non modifica algoritmi, coefficienti,
clamp, protezioni, frequenza dei loop o pilotaggio PWM.

## Implementazione

La cattura e' un buffer circolare di 32 campioni aggiornato nel medesimo hook
ISR che esegue il controllo UniPD. Il campionamento ordinario e' decimato di un
fattore 15; il campione che osserva il latch DCLINK o corrente viene sempre
salvato e congela il buffer. `TIMER1` non viene utilizzato.

Ogni campione contiene:

- Vdc e Vbat fisiche;
- IL_A e IL_B;
- riferimenti ed errori IL_A/IL_B;
- riferimenti `V_L_rif_A/B`;
- Pbat_rif;
- duty UniPD raw A/B;
- duty interni di debug A/B;
- duty mappati A/B;
- duty limitati A/B;
- duty rampati e realmente richiesti al wrapper PWM A/B.

La cattura avviene dopo l'acquisizione e la valutazione dei trip ma prima del
nuovo calcolo UniPD del ciclo corrente. Pertanto ingressi e stato di trip sono
del ciclo corrente, mentre riferimenti e duty rappresentano il comando
calcolato nel ciclo immediatamente precedente e ancora applicato all'ingresso
del ciclo. Questa relazione temporale e' intenzionale per identificare il
comando che precede il superamento della soglia.

## Comandi UART

```text
CAP=1   azzera e arma il buffer
CAP=0   congela manualmente il buffer
CAP?    stato: C,armed,frozen,count,length,decimation,trigger
CAPD?   dump cronologico CSV; rifiutato con C,BUSY se ancora armato
```

`CAP=1` viene rifiutato con `C,FAULT` se un latch DCLINK o corrente e' gia'
attivo: il fault deve essere prima verificato e azzerato esplicitamente, per
evitare che il nuovo buffer congeli sullo stato storico.

`trigger` vale 0 senza trip, 1 per DCLINK, 2 per corrente e 3 se entrambi sono
presenti. Le tensioni e correnti del dump sono in mV/mA, Pbat in mW e i duty in
milionesimi di per-unit.

Il firmware release e' incrementato da 1001 a 1002.

## Risorse e vincoli

La prima versione a 48 campioni occupava `0x4E0` word. L'estensione mantiene
costante la finestra a 480 cicli ISR usando 32 campioni con decimazione 15 e
aggiunge le variabili necessarie a discriminare segno, saturazione e mappatura.
L'occupazione finale viene verificata nel map file della build.

La build estesa occupa `0x540` word per il buffer e lascia `0x1FE` word libere
in RAMGS2. La sezione UART usa `0xFFE` word in FLASH_BANK0_SEC8 e `0x3B2` word
in SEC9. Build e link sono completati senza errori.

Prima dell'uso in prova di potenza devono essere verificati a potenza
disabilitata parser, arm, freeze manuale, ordine cronologico del dump e release.
Resta inoltre necessaria una verifica quantitativa che il costo aggiunto nel
percorso ISR sia compatibile con il budget temporale del controllo.

La verifica a potenza disabilitata ha confermato `FW=1002`, arm, riempimento dei
48 campioni, freeze manuale e dump cronologico. Dopo calibrazione corrente
`UF=2`, le correnti catturate a PWM spento sono rimaste prossime a zero. Il test
automatico del freeze e' stato eseguito abbassando temporaneamente la soglia
DCLINK sotto la tensione fisica a PWM spento: il buffer si e' congelato con 48
campioni e `trigger=1`; un nuovo `CAP=1` e' stato correttamente rifiutato con
`C,FAULT`. La soglia e' stata ripristinata a 42 V e il latch azzerato. Resta
pendente la verifica temporale ISR.

La prima verifica in potenza ha confermato il freeze automatico sul trip DCLINK
reale e ha reso disponibili tutti i 48 campioni pre-trip. Il contenuto e
l'interpretazione del test sono riportati esclusivamente nel test report.
