#ifndef WLESS_SM_SETTINGS_H_
#define WLESS_SM_SETTINGS_H_

//
// Integrazione della state machine del caricatore wireless.
//
// Mantenere il modulo disabilitato per default durante il bring-up hardware.
// L'abilitazione esegue soltanto la state machine di supervisione; i comandi
// allo stadio di potenza restano bloccati finche' non viene abilitato
// esplicitamente WLESS_SM_POWER_CONTROL_ENABLE.
//
#define WLESS_SM_ENABLE                 1
#define WLESS_SM_POWER_CONTROL_ENABLE   1

//
// Isolamento diagnostico del bring-up. Se abilitato, esegue solo WLESS_SM_init()
// e mantiene disabilitati l'hook del tick ISR2 e l'elaborazione in background A1
// della state machine.
//
#define WLESS_SM_TEST_INIT_ONLY         0
#define WLESS_SM_TEST_ISR_TICK_ONLY     0
#define WLESS_SM_TEST_TICK_SERVICE_ONLY 0
#define WLESS_SM_TEST_RUN_DISPATCH_ONLY 0
#define WLESS_SM_TEST_VEHICLE_STANDBY_ONLY 0
#define WLESS_SM_TEST_VEHICLE_OPERATION_STATES_ONLY 0

//
// Ruolo di build. Il progetto di comunicazione UniPD originale usa VEHICLE come
// selettore compile-time. Gli script di build sovrascrivono questo default dalla
// riga di comando del compilatore, evitando che la selezione del ruolo modifichi
// un file sorgente tracciato.
//
#ifndef WLESS_SM_BUILD_VEHICLE
#define WLESS_SM_BUILD_VEHICLE          0
#endif

//
// La demo nRF originale usa una base tempi a 1 kHz. Mantenere Timer1 libero per
// il firmware TI e derivare la temporizzazione della state machine dalla sorgente
// ISR2 esistente. Il codice della state machine continua a decimare il traffico
// diagnostico a circa 1 Hz quando non e' attivo alcun trasferimento di energia.
//
#define WLESS_SM_SOURCE_TICK_HZ         21250U
#define WLESS_SM_BASE_TICK_HZ           1000U
#define WLESS_SM_DIAGNOSTIC_TICKS       WLESS_SM_BASE_TICK_HZ
#define WLESS_SM_OPERATION_TICKS        1U
#define WLESS_SM_PENDING_TICK_LIMIT     8U

#define WLESS_SM_CAPACITY_MAX_WH        4800L
#define WLESS_SM_CAPACITY_MIN_WH        960L
#define WLESS_SM_CAPACITY_THRESHOLD_WH  2880L

#define WLESS_SM_VBUS_MIN_V             50U
/*
 * Soglie distinte dalla guardia FSM SMVBUS. Devono essere configurate per il
 * punto di prova prima di WPT; il LOAD richiede margine rispetto a VBAT BUCK.
 */
#define WLESS_SM_BOOT_SOURCE_VBUS_MIN_V  50U
#define WLESS_SM_BOOT_LOAD_VBUS_MIN_V    50U
/*
 * Bootstrap HFC: l'inverter parte dal punto neutro (500 mpu) e si muove
 * lentamente verso la fase configurata con WPTHFCPH. Queste guardie sono
 * inferiori ai trip hardware e proteggono il transitorio di pre-carica.
 */
#define WLESS_SM_BOOT_HFC_PHASE_START_MPU       500U
#define WLESS_SM_BOOT_HFC_PHASE_STEP_MPU          1U
/* 1 mpu ogni 100 ms: 500 -> 400 mpu richiede circa 10 s. */
#define WLESS_SM_BOOT_HFC_PHASE_STEP_TICKS      100U
/*
 * Dopo WPTREADY UniPD riceve la fase realmente applicata dal bootstrap.
 * Il suo limite differenziale viene poi rilasciato con la stessa cadenza,
 * evitando un salto alla massima escursione ammessa al primo tick automatico.
 */
#define WLESS_SM_BOOT_HFC_HANDOFF_STEP_MPU        1U
#define WLESS_SM_BOOT_HFC_HANDOFF_STEP_TICKS    100U
#define WLESS_SM_BOOT_HFC_SOFT_COIL_LIMIT_MA   4500U
/*
 * Il BOOST deve sostenere il proprio DCLINK prima che l'HFC inizi a sottrarre
 * energia. La conferma evita che un superamento transitorio di SMBOOTSRC
 * abiliti l'inverter; l'isteresi di mantenimento riporta gradualmente HFC al
 * neutro se il SOURCE perde il margine durante il bootstrap.
 */
#define WLESS_SM_BOOT_SOURCE_READY_CONFIRM_TICKS       500U
/*
 * La banda di hold deve scalare con il DCLINK di bootstrap: 2 V restano il
 * minimo anti-rumore per il banco low-voltage, mentre il 10 % di SMBOOTSRC
 * evita una banda trascurabile nei futuri profili ad alta tensione.
 */
#define WLESS_SM_BOOT_SOURCE_VBUS_HOLD_HYST_MIN_V       2U
#define WLESS_SM_BOOT_SOURCE_VBUS_HOLD_HYST_PERMILLE  100U
/* Primo tratto del BOOST automatico: limita la richiesta UniPD a 6 W con
 * VBAT=12 V, prima che il DCLINK SOURCE superi SMBOOTSRC. */
#define WLESS_SM_BOOT_BBC_PRECHARGE_CURRENT_A    (0.5f)     //TODO: da validare con il banco definitivo
/* Limite iniziale di duty del solo bootstrap BOOST. 150 mpu e' il punto
 * validato sul banco 12 V -> 24 V; profili diversi lo impostano a runtime
 * tramite SMBOOTDUTY prima del run. Non sostituisce UD. */
#define WLESS_SM_BOOT_BBC_DUTY_MAX_MPU            150U
/* BD e' il limite iniziale del governor PWM BOOST. Rimane inserito dopo
 * l'armamento e viene poi rilasciato in passi discreti, con VDC in tracking:
 * non e' lecito sostituirlo con UD in un unico istante. */
#define WLESS_SM_BOOT_BBC_DUTY_ARM_CONFIRM_TICKS  500U
/* Il servizio bootstrap segue il tick operativo a 1 kHz. Il riferimento BBC
 * avanza di 1 V ogni secondo, non ogni 100 ms: la rampa deve governare
 * l'energia immessa nel DCLINK e non limitarsi a frammentare un gradino UR. */
#define WLESS_SM_BOOT_BBC_CONTROL_STEP_TICKS     1000U
#define WLESS_SM_BOOT_BBC_CURRENT_STEP_A          (0.1f)
/* Incremento del cap BD per ciascun passo di controllo (1 s). 25 mpu/s
 * richiede circa 32 s per il tratto 150 -> 950 del banco 12 V -> 24 V. */
#define WLESS_SM_BOOT_BBC_DUTY_STEP_MPU            25U
#define WLESS_SM_BOOT_BBC_CURRENT_RELEASE_A       (10.0f)

/* Permanenza al riferimento BOOST effettivo prima dell'abilitazione HFC.
 * Unita': tick del servizio bootstrap a 1 kHz. */
#define WLESS_SM_BOOT_BBC_TARGET_CONFIRM_TICKS    500U
/* Il BOOST non viene mai armato a VIN assente. Sul banco il percorso passivo
 * porta VDC SOURCE a circa 11,5 V con VIN=12 V; 10 V e' quindi un consenso
 * fisico affidabile prima di rilasciare il PWM. */
#define WLESS_SM_BOOT_BBC_ARM_VBUS_MIN_V         10U        //TODO: da validare con il banco definitivo
/* Il riferimento VDC puo' avanzare soltanto quando la misura fisica lo segue
 * entro questa fascia. E' un margine di tracking, non una soglia VDC assoluta. */
#define WLESS_SM_BOOT_BBC_VDC_TRACK_MARGIN_V      1U
#define WLESS_SM_BOOT_BBC_VDC_REF_STEP_V          (1.0f)
/* Oltre questo limite la guardia non puo' restringere nessun UR ammesso. */
#define WLESS_SM_BOOT_BBC_VDC_REF_RELEASE_V       (60.0f)
#define WLESS_SM_BOOT_LOAD_VBUS_MAX_V            30U        //TODO: da validare con il banco definitivo
#define WLESS_SM_IBAT_MIN_MA            1U                  //TODO: da validare con il banco definitivo
#define WLESS_SM_ICOIL_MIN_MA           1U                  //TODO: da validare con il banco definitivo

// Valori ingegneristici iniziali; mantenerli configurabili fino alla validazione
// con i sensori definitivi e la dinamica dello stadio di potenza.
#define WLESS_SM_ENERGY_HYSTERESIS_WH   50L
#define WLESS_SM_ROLE_DEADBAND_WH        50L
#define WLESS_SM_ANALOG_CONFIRM_SAMPLES  2U

// Test dello scheduler destinato al solo oscilloscopio. Commutare GPIO30 a ogni
// tick logico generato. Disabilitare nuovamente dopo la misura quantitativa su docking.
#define WLESS_SM_TICK_GPIO_TEST_ENABLE    0
#define WLESS_SM_TICK_GPIO_PIN            30U

#endif
