# Configurazione persistente delle calibrazioni

## Scopo

Questa modifica sostituisce, per le calibrazioni analogiche coinvolte, i soli
valori fissati a compile time con una configurazione validata a run time e
memorizzabile nella Flash interna del TMS320F280039C.

La prima versione gestisce:

- offset di `ITANK_MOD`, in mV;
- sensibilita' di `ITANK_MOD`, in mV/A;
- limite fisico di corrente di bobina, in mA;
- offset raw ADC delle correnti `IL_A` e `IL_B`.

Il record contiene campi riservati e una versione di schema, così da poter
aggiungere in seguito altre calibrazioni senza cambiare il meccanismo di base.

## Valori di default

I default recepiscono l'ultima caratterizzazione fornita dal DT:

```text
V_ITANK_MOD_ADC = 0.573 * I_COIL + 0.120 V
I_COIL_MAX      = 5 A
IL_A_OFFSET     = 2048 count ADC
IL_B_OFFSET     = 2048 count ADC
```

Ne segue:

```text
I_COIL = (V_ITANK_MOD_ADC - offset) / sensibilita'
```

Il limite a 5 A non altera la misura: se viene superato durante l'attuazione
HFC, il firmware disabilita l'attuatore e segnala il fault HFC `8`.

## Organizzazione Flash

Sono riservati due settori indipendenti:

| Copia | Banco/settore | Indirizzo C28x |
|---|---|---:|
| A | Bank 1, Sector 14 | `0x09E000` |
| B | Bank 1, Sector 15 | `0x09F000` |

Il salvataggio avviene sulla copia inattiva. Il record comprende:

- magic e versione dello schema;
- numero di sequenza monotono;
- valori di configurazione;
- CRC16;
- marker di commit programmato per ultimo.

All'avvio viene scelta la copia valida con sequenza più recente. Se entrambe
sono assenti o corrotte vengono applicati i default. Un'interruzione
dell'alimentazione durante la scrittura non invalida la copia precedente.

Un'operazione di programmazione con cancellazione completa del dispositivo
elimina anche le calibrazioni. La normale programmazione dei soli settori
occupati dal file `.out` deve lasciare esclusi i due settori riservati.

## Comandi UART

I comandi sono volutamente brevi per contenere l'occupazione `.text`.

| Comando | Funzione |
|---|---|
| `CQ?` | configurazione attiva, sorgente e sequenza |
| `IO?`, `IO=<mV>` | offset `ITANK_MOD` pendente |
| `IG?`, `IG=<mV/A>` | sensibilita' `ITANK_MOD` pendente |
| `IM?`, `IM=<mA>` | limite corrente bobina pendente |
| `AO=<raw>` | offset ADC `IL_A` pendente |
| `BO=<raw>` | offset ADC `IL_B` pendente |
| `CV?` | valida l'intera configurazione pendente |
| `CA` | applica la configurazione pendente alla RAM |
| `CS` | salva in Flash la configurazione attiva |
| `CR` | ricarica e applica la configurazione dalla Flash |
| `CD` | copia i default nella configurazione pendente |

`CA`, `CS` e `CR` sono rifiutati se il BUCK/BOOST o l'attuatore HFC sono
attivi. `CD` non modifica il controllo finché non viene eseguito `CA`.

Sequenza tipica:

```text
IO=120
IG=573
IM=5000
AO=2048
BO=2048
CV?
CA
CS
CQ?
```

Risultati attesi:

```text
CV,1,1
CA,1,OK
CS,1,OK
CQ,1,<source>,<sequence>,120,573,5000,2048,2048
```

La calibrazione automatica esistente `UF=2` continua ad aggiornare gli offset
di corrente in RAM e nella configurazione pendente. Il salvataggio permanente
rimane esplicito tramite `CS`, evitando usure Flash involontarie.

## Stato di verifica

Fatti:

- clean build VEHICLE completata;
- clean build STATION completata;
- Flash API e funzione di salvataggio sono eseguite da RAM;
- i settori A/B non sono utilizzati dal linker per codice o dati;
- il firmware usa i parametri attivi nelle conversioni `ITANK_MOD`, `IL_A` e
  `IL_B`.

Da verificare su controlCARD:

1. avvio senza record e caricamento dei default;
2. salvataggio, power cycle e rilettura dello stesso record;
3. alternanza corretta A/B su salvataggi successivi;
4. fallback alla copia precedente dopo corruzione/interruzione simulata;
5. corrispondenza tra corrente nota e valore diagnostico `ITANK_MOD`;
6. arresto HFC al superamento della soglia configurata.
