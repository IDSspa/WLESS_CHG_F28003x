# Versionamento e identificazione delle risposte UART

## Motivazione

Nel formato precedente più messaggi con significato differente condividevano
lo stesso prefisso:

- `UQ?` produceva `U,...`;
- `CAP?` produceva `C,...`;
- le righe di `CAPD?` producevano anch'esse `C,...`.

Una riga isolata non era quindi sempre interpretabile senza ricordare il
comando trasmesso, contare i campi o aver ricevuto correttamente
l'intestazione precedente. Questo è fragile in presenza di output asincrono,
perdita di byte o estensioni future.

## Convenzione

Dal firmware release 1027 le risposte strutturate adottano:

```text
TIPO,VERSIONE,<payload>
```

La versione iniziale dello schema è `1`. Il comando trasmesso non cambia.

| Comando | Prefisso della risposta |
|---|---|
| `FW?` | `FW,1,RELEASE=` |
| `ROLE?` | `ROLE,1,BUILD=` |
| `UP?` | `UP,1,MASK=` |
| `WH?` | `WH,1,VALUE=` |
| `UQ?` | `UQ,1,` |
| `HFC?` | `HFC,1,` |
| `SMTHR?` | `SMTHR,1,` |
| `VARS?` | `VARS,1,` |
| `RADIO?` | `RADIO,1,` |
| `RADIOPING` | `RADIOPING,1,` |
| `WPT?` | `WPT,1,` |
| `WPTSNAP?` | `WPTSNAP,1,` |
| `WPTCAP?` | `WPTCAP,1,` |
| `CAP?` | `CAPS,1,` |
| `CAPD?`, intestazione | `CAPD_COLUMNS,1,` |
| `CAPD?`, campione | `CAPD,1,` |
| `WPTCAPD?`, intestazione | `WPTCAPD_COLUMNS,1,` |
| `WPTCAPD?`, campione | `WPTCAPD,1,` |

`CAPS` identifica lo stato del capture BBC e non può più essere confuso con
un campione `CAPD`.

## Regole per il parser host

1. Separare il flusso in righe terminate da `CRLF`.
2. Ignorare, se desiderato, l'eco esatta del comando trasmesso.
3. Leggere il primo campo completo come tipo del record.
4. Accettare il record solo se la versione dello schema è supportata.
5. Non identificare mai una risposta usando soltanto il primo carattere.
6. Trattare gli status asincroni `VH ...` e `ST ...` come record separati.

## Compatibilità

La modifica è intenzionalmente incompatibile con parser che si aspettano i
vecchi prefissi `U,` o `C,`. I comandi in ingresso e il significato fisico dei
campi rimangono invariati.
