#ifndef WLESS_STANDARD_PROFILE_H_
#define WLESS_STANDARD_PROFILE_H_

/*
 * Profilo standard del banco per BOOST + HFC + BUCK closed-loop.
 *
 * Questa e' la sola sorgente dei valori nominali usati dagli script UART
 * BOOST_HFC_BUCK_BOOT_GOVERNOR_SYNC_*.wcx.  I valori sono organizzati per
 * grandezza fisica e ruolo runtime, non per porta COM o posizione della
 * scheda: una scheda puo' assumere SOURCE oppure LOAD in sessioni diverse.
 *
 * UE, UF, WPT, INITOK e STOP sono azioni di sessione e non sono parametri di
 * profilo; restano quindi intenzionalmente fuori da questo file.
 */

/* Guardie FSM comuni a entrambi i ruoli. */
#define WLESS_PROFILE_FSM_VBUS_MIN_V                 5U      /* V */
#define WLESS_PROFILE_FSM_IBAT_MIN_MA                 1U      /* mA */

/* Soglia locale di corrente coil per completare la chiusura FSM.
 * Dipende dall'offset della catena analogica della scheda, non dal ruolo. */
#define WLESS_PROFILE_LOCAL_ICOIL_MIN_VEHICLE_MA    100U      /* mA */
#define WLESS_PROFILE_LOCAL_ICOIL_MIN_STATION_MA    400U      /* mA */

/* Riferimenti UniPD/WPT condivisi dalla coppia SOURCE/LOAD. */
#define WLESS_PROFILE_WPT_VDC_SOURCE_REF_V          20.0f     /* V */
#define WLESS_PROFILE_WPT_VDC_LOAD_REF_V            20.0f     /* V */
#define WLESS_PROFILE_WPT_COIL_CURRENT_MAX_A         5.0f     /* A */
#define WLESS_PROFILE_WPT_SOURCE_POWER_SEED_W       15.0f     /* W */
#define WLESS_PROFILE_WPT_HFC_PHASE_LIMIT_MPU       400U      /* milli-pu */
#define WLESS_PROFILE_WPT_HFC_BOOT_PHASE_MPU        470U      /* milli-pu */
#define WLESS_PROFILE_WPT_PHASE_CLAMP_ENABLE          0U      /* boolean */
#define WLESS_PROFILE_WPT_PHASE_RAMP_UPU               0U      /* micro-pu/ciclo */

/* SOURCE: bootstrap BOOST e hand-off HFC. */
#define WLESS_PROFILE_SOURCE_BOOT_MODE                 1U     /* 1=BOOST */
#define WLESS_PROFILE_SOURCE_BOOT_VBUS_MIN_V          23U     /* V */
#define WLESS_PROFILE_SOURCE_BOOT_VBUS_HOLD_MIN_V     14U     /* V */
#define WLESS_PROFILE_SOURCE_BBC_ARM_VBUS_MIN_V       10U     /* V */
#define WLESS_PROFILE_SOURCE_BOOT_DUTY_MAX_MPU       500U     /* milli-pu */
#define WLESS_PROFILE_SOURCE_HFC_DUTY_MAX_MPU        525U     /* milli-pu */
#define WLESS_PROFILE_SOURCE_HANDOFF_DUTY_MAX_MPU    600U     /* milli-pu */
#define WLESS_PROFILE_SOURCE_HFC_SOFT_COIL_LIMIT_MA 4500U     /* mA */

/* LOAD: soglie locali di bootstrap del BUCK. */
#define WLESS_PROFILE_LOAD_BOOT_VBUS_MIN_V            15U     /* V */
#define WLESS_PROFILE_LOAD_BOOT_VBUS_MAX_V            30U     /* V */

/* Profilo BBC applicabile soltanto dopo l'assegnazione del ruolo SOURCE. */
#define WLESS_PROFILE_SOURCE_BBC_HYBRID_MODE           1U     /* UH */
#define WLESS_PROFILE_SOURCE_BBC_VBAT_DIAG_MV      12000U     /* mV */
#define WLESS_PROFILE_SOURCE_BBC_VDC_REF_V            24U     /* V */
#define WLESS_PROFILE_SOURCE_BBC_VDC_TRIP_V           50U     /* V */
#define WLESS_PROFILE_SOURCE_BBC_CURRENT_LIMIT_MA  10000U     /* mA */
#define WLESS_PROFILE_SOURCE_BBC_IL_TRIP_MA        10000U     /* mA, per ramo */
#define WLESS_PROFILE_SOURCE_BBC_DUTY_MAX_MPU        950U     /* milli-pu */
#define WLESS_PROFILE_SOURCE_BBC_DUTY_RAMP_UPU       500U     /* micro-pu/ciclo */
/* Mantiene VBAT virtuale a 12 V nel profilo BOOST del banco; VDC e correnti
 * di ramo restano sempre fisici. */
#define WLESS_PROFILE_SOURCE_BBC_VBAT_OVERRIDE_ENABLE  1U     /* boolean */

/* Profilo BBC applicabile soltanto dopo l'assegnazione del ruolo LOAD. */
#define WLESS_PROFILE_LOAD_BBC_HYBRID_MODE             2U     /* UH */
#define WLESS_PROFILE_LOAD_BBC_CURRENT_POLARITY_MASK   0U     /* UP */
#define WLESS_PROFILE_LOAD_BBC_VDC_REF_MV          20000U     /* mV */
#define WLESS_PROFILE_LOAD_BBC_CURRENT_LIMIT_MA     4000U     /* mA */
#define WLESS_PROFILE_LOAD_BBC_VDC_TRIP_MV         30000U     /* mV */
#define WLESS_PROFILE_LOAD_BBC_IL_TRIP_MA           6000U     /* mA, per ramo */
#define WLESS_PROFILE_LOAD_BBC_DUTY_MAX_MPU          950U     /* milli-pu */
#define WLESS_PROFILE_LOAD_BBC_DUTY_RAMP_UPU        2000U     /* micro-pu/ciclo */

/* Protezione BBC comune ai due profili di ruolo. */
#define WLESS_PROFILE_BBC_IL_TRIP_CONFIRM_CYCLES      22U     /* cicli ISR */

/* Default sicuri prima che sia applicato un profilo di ruolo.  Sono mantenuti
 * separati dai valori SOURCE/LOAD per non introdurre una scelta implicita del
 * ruolo al boot. */
#define WLESS_PROFILE_BBC_SAFE_DUTY_MAX_MPU          350U     /* milli-pu */
#define WLESS_PROFILE_BBC_SAFE_VDC_REF_MV           7000U     /* mV */
#define WLESS_PROFILE_BBC_SAFE_CURRENT_LIMIT_MA      200U     /* mA */
#define WLESS_PROFILE_BBC_SAFE_VBAT_MV              6000U     /* mV */
#define WLESS_PROFILE_BBC_SAFE_VDC_TRIP_MV         10000U     /* mV */
#define WLESS_PROFILE_BBC_SAFE_IL_TRIP_MA            750U     /* mA, per ramo */
#define WLESS_PROFILE_BBC_SAFE_DUTY_RAMP_UPU         500U     /* micro-pu/ciclo */
#define WLESS_PROFILE_BBC_DUTY_RAMP_ENABLE             1U     /* boolean */
#define WLESS_PROFILE_BBC_VDC_TRIP_ENABLE              1U     /* boolean */
#define WLESS_PROFILE_BBC_IL_TRIP_ENABLE               1U     /* boolean */
#define WLESS_PROFILE_BBC_CURRENT_POLARITY_MASK        0U     /* bit mask */

#endif
