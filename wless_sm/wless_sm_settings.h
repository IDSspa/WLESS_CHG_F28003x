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
#define WLESS_SM_POWER_CONTROL_ENABLE   0

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
#define WLESS_SM_IBAT_MIN_MA            1U
#define WLESS_SM_ICOIL_MIN_MA           1U

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
