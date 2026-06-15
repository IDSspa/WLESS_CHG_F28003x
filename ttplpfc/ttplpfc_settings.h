//#############################################################################
//
// FILE:   ttplpfc_settings.h
//
// TITLE:  ttplpfc_settings.h
//
//#############################################################################
// $TI Release: obc_7_4kw_ttplpfc_clllc v2.00.03.00 $
// $Release Date: Tue Aug 26 14:08:13 CDT 2025 $
// $Copyright:
// Copyright (C) 2025 Texas Instruments Incorporated - http://www.ti.com/
//
// ALL RIGHTS RESERVED
// $
//#############################################################################

#ifndef _PROJSETTINGS_H
#define _PROJSETTINGS_H

#ifdef __cplusplus

extern "C" {
#endif

//#############################################################################
//
// Macro Definitions
//
//#############################################################################
#ifndef C2000_IEEE754_TYPES
    #define C2000_IEEE754_TYPES
    #ifdef __TI_EABI__
        typedef float         float32_t;
        typedef double        float64_t;
    #else // TI COFF
        typedef float         float32_t;
        typedef long double   float64_t;
    #endif // __TI_EABI__
#endif // C2000_IEEE754_TYPES


//*****************************************************************************
//defines
//*****************************************************************************

/* Device Related Defines */
#define CPU_SYS_CLOCK (120*1000000)
#define PWMSYSCLOCK_FREQ (120*1000000)
#define ECAPSYSCLOCK_FREQ	(120*1000000)

//
// Project Options
// Labs for System check-out
// LAB 1:PFC Open Loop Check (DC Input , with auto start)
//
//       PFC Check AC voltage sensing under AC condition (without auto start)
// LAB 2:PFC Closed Current Loop Check (DC Input)
// LAB 3:PFC Closed Current Loop Check (AC Input)
// LAB 4:PFC Closed Voltage Loop Check (AC Input)
// LAB 5:INV Open Loop Check (DC Output)
// LAB 6:INV Open Loop Check (AC Output)
// LAB 7:INV Closed Current Loop Check (DC Output)
// LAB 8:INV Closed Current Loop Check (AC Output)
// LAB 9:INV Closed Current Loop Check (Grid Connection)
//
#define TTPLPFC_LAB 4

//
// 1 means control runs on C28x , otherwise it will run on CLA
//
#define C28x_CORE 1
#define CLA_CORE 2
#define TTPLPFC_CONTROL_RUNNING_ON 1

//
//Enable Disable Adaptive Deadtime
//
#define TTPLPFC_ADAPTIVE_DEADTIME 0

//
// set 1 to enable phase shedding 0 to disable
//
#define TTPLPFC_PHASE_SHEDDING_ENABLED 0

//
// set 1 to enable non linear voltage loop 0 to disable
//
#define TTPLPFC_NON_LINEAR_VOLTAGE_LOOP 0

//
// The module support phases 1,2 and 3
//
#define TTPLPFC_TOTAL_NO_PHASES 2

//
// Options of PLL modules
//
#define TTPLPFC_SPLL_1PH_NOTCH_SEL 1
#define TTPLPFC_SPLL_1PH_SOGI_SEL 2
#define TTPLPFC_SPLL_1PH_SOGI_FLL_SEL 3
#define TTPLPFC_AC_FREQ 60

#define TTPLPFC_POSITIVE_HALF_DC_CHECK 1
#define TTPLPFC_NEGATIVE_HALF_DC_CHECK 2


//
// if 1, the TTPLPFC module used as a stand alone project
// if 0, the TTPLPFC module used in PFC + CLLLC  project
//
//
#define TTPLPFC_STANDALONE_MODULE 0

//
// SFRA running on TTPLPFC
//
#define TTPLPFC_SFRA_ALLOWED 1 //OBC_7_4KW_RUN_SFRA_ON_PFC

#if TTPLPFC_LAB == 1
#define TTPLPFC_DC_CHECK 1
#define TTPLPFC_PWM_AC_CHECK 0
#define TTPLPFC_PWM_AC_SR_OFF 0
#define TTPLPFC_AUTO_START 1
//
// If the TTPLPFC_TIMED_STARTUP is 1, then only an initial burst of PWM is issued
// the burst of PWM lasts for TTPLPFC_STARTUP_TIME_MS
// the code trips the PWM after the time specified in TTPLPFC_STARTUP_TIME_MS
//
#define TTPLPFC_TIMED_STARTUP 0
#define TTPLPFC_STARTUP_TIME_MS 20
//
// The below option will switch off the SRs and slowly increase the duty cycle
// the SRs are enabled once the duty reaches the final value
// this happens when startup time hits 10mS, if the startup is less than 10mS the SRs will remain OFF during the full burst
// if TIMED_STARTUP is 0, this variable must be zero
//
#define TTPLPFC_RAMPED_DUTY_STARTUP 0
//
// during DC test, we may want to test with positive and negative DC input
// setting the below specifies which half is being tested
//
#define TTPLPFC_AC_HALF_CHECK TTPLPFC_POSITIVE_HALF_DC_CHECK
#define TTPLPFC_IL1_PROTECTION_ENABLE 1
#define TTPLPFC_IL2_PROTECTION_ENABLE 1
#define TTPLPFC_FAULTn_PROTECTION_ENABLE 1
#define TTPLPFC_SPLL_METHOD_SELECT TTPLPFC_SPLL_1PH_SOGI_SEL
#define TTPLPFC_CONTROL_RUNNING_ON 1
#define TTPLPFC_CURRENT_LOOP_CLOSED 0
#define TTPLPFC_VOLTAGE_LOOP_CLOSED 0
#if TTPLPFC_SFRA_ALLOWED == 1
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_CURRENT
#else
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_DISABLED
#endif
#define TTPLPFC_ADAPTIVE_DEADTIME 0
#define TTPLPFC_PHASE_SHEDDING_ENABLED 0
#endif

#if TTPLPFC_LAB == 2
#define TTPLPFC_AC_HALF_CHECK TTPLPFC_POSITIVE_HALF_DC_CHECK
#define TTPLPFC_DC_CHECK 1
#define TTPLPFC_PWM_AC_CHECK 0
#define TTPLPFC_AUTO_START 1
#define TTPLPFC_IL1_PROTECTION_ENABLE 1
#define TTPLPFC_IL2_PROTECTION_ENABLE 1
#define TTPLPFC_FAULTn_PROTECTION_ENABLE 1
#define TTPLPFC_SPLL_METHOD_SELECT TTPLPFC_SPLL_1PH_SOGI_SEL
#define TTPLPFC_CONTROL_RUNNING_ON 1
#define TTPLPFC_CURRENT_LOOP_CLOSED 0
#define TTPLPFC_VOLTAGE_LOOP_CLOSED 0
#if TTPLPFC_SFRA_ALLOWED == 1
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_CURRENT
#else
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_DISABLED
#endif
#define TTPLPFC_ADAPTIVE_DEADTIME 0
#define TTPLPFC_PHASE_SHEDDING_ENABLED 0
#endif

#if TTPLPFC_LAB == 3
#define TTPLPFC_DC_CHECK 0
#define TTPLPFC_PWM_AC_CHECK 0
#define TTPLPFC_PWM_AC_SR_OFF 0
#define TTPLPFC_AUTO_START 0
#define TTPLPFC_IL1_PROTECTION_ENABLE 1
#define TTPLPFC_IL2_PROTECTION_ENABLE 1
#define TTPLPFC_FAULTn_PROTECTION_ENABLE 1
#define TTPLPFC_SPLL_METHOD_SELECT TTPLPFC_SPLL_1PH_SOGI_SEL
#define TTPLPFC_CONTROL_RUNNING_ON 1
#define TTPLPFC_CURRENT_LOOP_CLOSED 1
#define TTPLPFC_VOLTAGE_LOOP_CLOSED 0
#if TTPLPFC_SFRA_ALLOWED == 1
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_CURRENT
#else
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_DISABLED
#endif
#define TTPLPFC_ADAPTIVE_DEADTIME 0
#define TTPLPFC_PHASE_SHEDDING_ENABLED 0
#endif

#if TTPLPFC_LAB == 4
#define TTPLPFC_DC_CHECK 0
#define TTPLPFC_PWM_AC_CHECK 0
#define TTPLPFC_AUTO_START 0
#define TTPLPFC_PWM_AC_SR_OFF 0
#define TTPLPFC_IL1_PROTECTION_ENABLE 1
#define TTPLPFC_IL2_PROTECTION_ENABLE 1
#define TTPLPFC_FAULTn_PROTECTION_ENABLE 1
#define TTPLPFC_SPLL_METHOD_SELECT TTPLPFC_SPLL_1PH_SOGI_SEL
#define TTPLPFC_CONTROL_RUNNING_ON 1
#define TTPLPFC_CURRENT_LOOP_CLOSED 1
#define TTPLPFC_VOLTAGE_LOOP_CLOSED 1
#if TTPLPFC_SFRA_ALLOWED == 1
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_VOLTAGE
#else
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_DISABLED
#endif
#define TTPLPFC_ADAPTIVE_DEADTIME 0
#define TTPLPFC_PHASE_SHEDDING_ENABLED 0
#endif

#if TTPLPFC_LAB == 5
#error "TTPLPFC Lab 5 has not been validated"
#define TTPLPFC_DC_CHECK 1
#define TTPLPFC_PWM_AC_CHECK 0
#define TTPLPFC_AUTO_START 1
#define TTPLPFC_IL1_PROTECTION_ENABLE 1
#define TTPLPFC_IL2_PROTECTION_ENABLE 1
#define TTPLPFC_FAULTn_PROTECTION_ENABLE 1
#define TTPLPFC_SPLL_METHOD_SELECT TTPLPFC_SPLL_1PH_SOGI_SEL
#define TTPLPFC_CONTROL_RUNNING_ON 1
#define TTPLPFC_CURRENT_LOOP_CLOSED 0
#define TTPLPFC_VOLTAGE_LOOP_CLOSED 0
#if TTPLPFC_SFRA_ALLOWED == 1
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_CURRENT
#else
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_DISABLED
#endif
#define TTPLPFC_ADAPTIVE_DEADTIME 0
#define TTPLPFC_PHASE_SHEDDING_ENABLED 0

#endif

#if TTPLPFC_LAB == 6
#error "TTPLPFC Lab 6 has not been validated"
#define TTPLPFC_CONTROL_RUNNING_ON 1
#define TTPLPFC_DC_CHECK 0
#define TTPLPFC_CURRENT_LOOP_CLOSED 0
#define TTPLPFC_VOLTAGE_LOOP_CLOSED 0
#if TTPLPFC_SFRA_ALLOWED == 1
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_CURRENT
#else
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_DISABLED
#endif
#define TTPLPFC_AUTO_START 1
#define TTPLPFC_IL1_PROTECTION_ENABLE 0
#define TTPLPFC_IL2_PROTECTION_ENABLE 0
#define TTPLPFC_FAULTn_PROTECTION_ENABLE 0
#define TTPLPFC_SPLL_METHOD_SELECT TTPLPFC_SPLL_1PH_SOGI_SEL
#endif

#if TTPLPFC_LAB == 7
#error "TTPLPFC Lab 7 has not been validated"
#define TTPLPFC_CONTROL_RUNNING_ON 1
#define TTPLPFC_DC_CHECK 1
#define TTPLPFC_CURRENT_LOOP_CLOSED 1
#define TTPLPFC_VOLTAGE_LOOP_CLOSED 0
#if TTPLPFC_SFRA_ALLOWED == 1
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_CURRENT
#else
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_DISABLED
#endif
#define TTPLPFC_AUTO_START 1
#define TTPLPFC_DEBUG_STARTUP 1
#define TTPLPFC_IL1_PROTECTION_ENABLE 0
#define TTPLPFC_IL2_PROTECTION_ENABLE 0
#define TTPLPFC_FAULTn_PROTECTION_ENABLE 0
#define TTPLPFC_SPLL_METHOD_SELECT TTPLPFC_SPLL_1PH_SOGI_SEL
#endif

#if TTPLPFC_LAB == 8
#error "TTPLPFC Lab 8 has not been validated"
#define TTPLPFC_CONTROL_RUNNING_ON 1
#define TTPLPFC_DC_CHECK 0
#define TTPLPFC_CURRENT_LOOP_CLOSED 1
#define TTPLPFC_VOLTAGE_LOOP_CLOSED 0
#if TTPLPFC_SFRA_ALLOWED == 1
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_CURRENT
#else
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_DISABLED
#endif
#define TTPLPFC_AUTO_START 1
#define TTPLPFC_IL1_PROTECTION_ENABLE 0
#define TTPLPFC_IL2_PROTECTION_ENABLE 0
#define TTPLPFC_FAULTn_PROTECTION_ENABLE 0
#define TTPLPFC_SPLL_METHOD_SELECT TTPLPFC_SPLL_1PH_SOGI_SEL
#endif

#if TTPLPFC_LAB == 9
#error "TTPLPFC Lab 9 has not been validated"
#define TTPLPFC_CONTROL_RUNNING_ON 1
#define TTPLPFC_DC_CHECK 0
#define TTPLPFC_CURRENT_LOOP_CLOSED 1
#define TTPLPFC_VOLTAGE_LOOP_CLOSED 0
#if TTPLPFC_SFRA_ALLOWED == 1
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_CURRENT
#else
#define TTPLPFC_SFRA_TYPE  TTPLPFC_SFRA_DISABLED
#endif
#define TTPLPFC_AUTO_START 1
#define TTPLPFC_IL1_PROTECTION_ENABLE 0
#define TTPLPFC_IL2_PROTECTION_ENABLE 0
#define TTPLPFC_FAULTn_PROTECTION_ENABLE 0
#define TTPLPFC_SPLL_METHOD_SELECT TTPLPFC_SPLL_1PH_SOGI_SEL
#endif

//
//Power Stage Related Values
//
#define TTPLPFC_PWM_SWITCHING_FREQUENCY ((float)120*1000)
#define TTPLPFC_PWM_PERIOD ((PWMSYSCLOCK_FREQ) / (TTPLPFC_PWM_SWITCHING_FREQUENCY))

#define TTPLPFC_DEADBAND_RED_HS_US 0.1
#define TTPLPFC_DEADBAND_FED_HS_US 0.1

#define TTPLPFC_HIGH_FREQ_PWM_DEADBAND_FED_COUNT (int)((float32_t)TTPLPFC_DEADBAND_FED_HS_US * (float32_t)PWMSYSCLOCK_FREQ * (float32_t)0.000001)
#define TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_COUNT (int)((float32_t)TTPLPFC_DEADBAND_RED_HS_US * (float32_t)PWMSYSCLOCK_FREQ * (float32_t)0.000001)

#define TTPLPFC_VAC_MAX_SENSE       (float32_t) 519.18
#define TTPLPFC_VAC_L_N_MAX_SENSE   (float32_t) 508.68
#define TTPLPFC_VDCBUS_MAX_SENSE    (float32_t) 508.68
#define TTPLPFC_IL_MAX_SENSE       (float32_t) 30.303
#define TTPLPFC_IL_TRIP_LIMIT        (float32_t) 30
#define TTPLPFC_VAC_TYPICAL         (float32_t) 120

#define TTPLPFC_VBUS_TYPICAL        (float32_t) 400

#define TTPLPFC_INDUCTOR_VALUE  ((float32_t) 0.077*0.001)

//
// Control Loop Design
//
#define TTPLPFC_CNTRL_ISR_FREQ_RATIO 1
#define TTPLPFC_CONTROL_ISR_FREQUENCY ((TTPLPFC_PWM_SWITCHING_FREQUENCY) / (TTPLPFC_CNTRL_ISR_FREQ_RATIO))
#define TTPLPFC_VOLTAGE_LOOP_RUN_RATIO 1

//
// SFRA Options
//
#define TTPLPFC_SFRA_DISABLED 0
#define TTPLPFC_SFRA_CURRENT 1
#define TTPLPFC_SFRA_VOLTAGE 2

#if TTPLPFC_SFRA_TYPE == TTPLPFC_SFRA_CURRENT
#define TTPLPFC_SFRA_ISR_FREQ       TTPLPFC_CONTROL_ISR_FREQUENCY
#else
#define TTPLPFC_SFRA_ISR_FREQ       TTPLPFC_INSTRUMENTATION_ISR_FREQUENCY
#endif

#define TTPLPFC_PI_VALUE 3.141592653589

//#define TTPLPFC_GI_PI_KP (float) 0.1//0.3496503407
//#define TTPLPFC_GI_PI_KI (float) 0.002//0.0020000510

//Kdc = 0.02, fz0 = 1, ISR fs = 120 kHz, fbw ~ 2 kHz at 150 V / 400 V / 100 Ohm
//#define TTPLPFC_GI_PI_KP (float) 0.0194764
//#define TTPLPFC_GI_PI_KI (float) 0.0537675
//#define TTPLPFC_GI_PI_KP (float) 0.25
//#define TTPLPFC_GI_PI_KI (float) 0.07
#define TTPLPFC_GI_PI_KP (float) 0.3
#define TTPLPFC_GI_PI_KI (float) 0.03
//#define TTPLPFC_GI_PI_KI (float) 0.05

//#define TTPLPFC_GV_PI_KP (float) 0.6004499882
//#define TTPLPFC_GV_PI_KI (float) 0.0001000236
#define TTPLPFC_GV_PI_KP (float) 0.6
#define TTPLPFC_GV_PI_KI (float) 0.0001
//
// User code settings file
//
#include <ttplpfc_user_settings.h>

#ifdef __cplusplus
}
#endif

#endif
