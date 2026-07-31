//#############################################################################
//
// FILE:   clllc_settings.h
//
// TITLE: This is the settings.h file, which contains all the project level
//        settings, in case of powerSUITE , powerSUITE related settings are
//        in this file and the other settings are located in 
//        clllc_user_settings.h.
//        The User section is not over-written by powerSUITE
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

#ifndef _CLLLC_PROJSETTINGS_H
#define _CLLLC_PROJSETTINGS_H

#ifdef __cplusplus

extern "C" {
#endif

//#############################################################################
//
// Includes
//
//#############################################################################
#include <stdint.h>
#include <obc_7_4kw_user_settings.h>

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


//
// Defines
//

//
// Device Related Defines
//
#define CLLLC_CPU_SYS_CLOCK_FREQ_HZ ((float32_t)120*1000000)
#define CLLLC_PWMSYSCLOCK_FREQ_HZ   ((float32_t)120*1000000)
#define CLLLC_ECAPSYSCLOCK_FREQ_HZ  ((float32_t)120*1000000)

//
// Project Options
//

//
// CONTROL MODE , voltage or current
// 1 -> Voltage
// 2 -> Current
//
#define CLLLC_VOLTAGE_MODE 1
#define CLLLC_CURRENT_MODE 2

//
// POWER FLOW ,
// 1 -> PRIM to SEC
// 2 -> SEC to PRIM
//
#define CLLLC_POWER_FLOW_PRIM_SEC 1
#define CLLLC_POWER_FLOW_SEC_PRIM 2
#define CLLLC_POWER_FLOW_TRANSTION_STAGE 3

//
// PROTECTION  ,
// 0 -> DISABLED
// 1 -> ENABLED
//
#define CLLLC_PROTECTION_ENABLED 1
#define CLLLC_PROTECTION_DISABLED 0

//
// BUILD
// 1 ->  Open Loop Check
// 2 ->  Closed Loop Check
//
#define CLLLC_OPEN_LOOP_BUILD 1
#define CLLLC_CLOSED_LOOP_BUILD 2

//
// TEST, (which side is output depends on power flow)
// 0 ->  Test with Res load at the output
// 1 ->  Test with Res Load and Voltage source connected at output
//
#define CLLLC_TEST_SETUP_RES_LOAD 0
#define CLLLC_TEST_SETUP_EMULATED_BATTERY 1

//
// CORE running the control loop
// 1 -> C28x
// 2 -> CLA
//
#ifndef C28x_CORE
#define C28x_CORE 1
#define CLA_CORE 2
#endif

//
// SFRA Options
// 0 -> disabled
// 1 -> Current
// 2 -> Voltage
//
#define CLLLC_SFRA_DISABLED 0
#define CLLLC_SFRA_CURRENT 1
#define CLLLC_SFRA_VOLTAGE 2

//
// SFRA injection amplitude, use higher injection in open loop  because plant
// response is low
//
#define CLLLC_SFRA_INJECTION_AMPLITUDE_LEVEL1 0.001
#define CLLLC_SFRA_INJECTION_AMPLITUDE_LEVEL2 0.01
#define CLLLC_SFRA_INJECTION_AMPLITUDE_LEVEL3 0.015

//
// SFRA running on CLLLC
//
#define CLLLC_SFRA_ALLOWED 0

//
// if 1, the CLLLC module used as a stand alone project
// if 0, the CLLLC module used in PFC + CLLLC  project
//
//
#define CLLLC_STANDALONE_MODULE 1

//
// CLLLC LAB
// Power Flow Prim -> Sec
// 1 -> Open loop check for PWM drivers,
// 2 -> Open loop check for PWM drivers with protection,
// 3 -> Closed loop check with resistive load, voltage loop,
// 4 -> Closed loop check with resistive load, current loop
// 5 -> Closed loop check with battery emulated, current loop
// Power Flow Sec -> Prim
// 6 -> Open loop check for PWM driver,
// 7 -> Open loop check for PWM driver with protection,
// 8 -> Closed loop voltage with resistive load
//

#define CLLLC_LAB 1

#if CLLLC_LAB == 1
#define CLLLC_CONTROL_RUNNING_ON CLA_CORE
#define CLLLC_POWER_FLOW CLLLC_POWER_FLOW_PRIM_SEC
#define CLLLC_INCR_BUILD CLLLC_OPEN_LOOP_BUILD
#define CLLLC_TEST_SETUP CLLLC_TEST_SETUP_RES_LOAD
#define CLLLC_PROTECTION CLLLC_PROTECTION_ENABLED
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_DISABLED
#define CLLLC_SFRA_AMPLITUDE (float32_t)CLLLC_SFRA_INJECTION_AMPLITUDE_LEVEL2
#endif

#if CLLLC_LAB == 2
#define CLLLC_CONTROL_RUNNING_ON CLA_CORE
#define CLLLC_POWER_FLOW CLLLC_POWER_FLOW_PRIM_SEC
#define CLLLC_INCR_BUILD CLLLC_OPEN_LOOP_BUILD
#define CLLLC_TEST_SETUP CLLLC_TEST_SETUP_RES_LOAD
#define CLLLC_PROTECTION CLLLC_PROTECTION_ENABLED

#if CLLLC_SFRA_ALLOWED == 1
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_VOLTAGE
#else
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_DISABLED
#endif

#define CLLLC_SFRA_AMPLITUDE (float32_t)CLLLC_SFRA_INJECTION_AMPLITUDE_LEVEL2
#endif

#if CLLLC_LAB == 3
#define CLLLC_CONTROL_RUNNING_ON CLA_CORE
#define CLLLC_POWER_FLOW CLLLC_POWER_FLOW_PRIM_SEC
#define CLLLC_INCR_BUILD CLLLC_CLOSED_LOOP_BUILD
#define CLLLC_CONTROL_MODE CLLLC_VOLTAGE_MODE
#define CLLLC_TEST_SETUP CLLLC_TEST_SETUP_RES_LOAD
#define CLLLC_PROTECTION CLLLC_PROTECTION_ENABLED
#if CLLLC_SFRA_ALLOWED == 1
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_VOLTAGE
#else
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_DISABLED
#endif
#define CLLLC_SFRA_AMPLITUDE (float32_t)CLLLC_SFRA_INJECTION_AMPLITUDE_LEVEL1
#endif

#if CLLLC_LAB == 4
#define CLLLC_CONTROL_RUNNING_ON CLA_CORE
#define CLLLC_POWER_FLOW CLLLC_POWER_FLOW_PRIM_SEC
#define CLLLC_INCR_BUILD CLLLC_CLOSED_LOOP_BUILD
#define CLLLC_CONTROL_MODE CLLLC_CURRENT_MODE
#define CLLLC_TEST_SETUP CLLLC_TEST_SETUP_RES_LOAD
#define CLLLC_PROTECTION CLLLC_PROTECTION_ENABLED
#if CLLLC_SFRA_ALLOWED == 1
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_CURRENT
#else
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_DISABLED
#endif
#define CLLLC_SFRA_AMPLITUDE (float32_t)CLLLC_SFRA_INJECTION_AMPLITUDE_LEVEL1
#endif

#if CLLLC_LAB == 5
#error "untested CLLLC lab"
#define CLLLC_CONTROL_RUNNING_ON 1
#define CLLLC_POWER_FLOW CLLLC_POWER_FLOW_PRIM_SEC
#define CLLLC_INCR_BUILD CLLLC_CLOSED_LOOP_BUILD
#define CLLLC_CONTROL_MODE CLLLC_CURRENT_MODE
#define CLLLC_TEST_SETUP CLLLC_TEST_SETUP_EMULATED_BATTERY
#define CLLLC_PROTECTION CLLLC_PROTECTION_ENABLED
#if CLLLC_SFRA_ALLOWED == 1
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_CURRENT
#else
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_DISABLED
#endif
#define CLLLC_SFRA_AMPLITUDE (float32_t)CLLLC_SFRA_INJECTION_AMPLITUDE_LEVEL1
#endif


#if CLLLC_LAB == 6
#error "untested CLLLC lab"
#define CLLLC_CONTROL_RUNNING_ON 1
#define CLLLC_POWER_FLOW CLLLC_POWER_FLOW_SEC_PRIM
#define CLLLC_INCR_BUILD CLLLC_OPEN_LOOP_BUILD
#define CLLLC_CONTROL_MODE CLLLC_VOLTAGE_MODE
#define CLLLC_TEST_SETUP CLLLC_TEST_SETUP_RES_LOAD
#define CLLLC_PROTECTION CLLLC_PROTECTION_DISABLED
#if CLLLC_SFRA_ALLOWED == 1
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_DISABLED
#else
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_DISABLED
#endif
#define CLLLC_SFRA_AMPLITUDE (float32_t)CLLLC_SFRA_INJECTION_AMPLITUDE_LEVEL2
#endif

#if CLLLC_LAB == 7
#error "untested CLLLC lab"
#define CLLLC_CONTROL_RUNNING_ON 1
#define CLLLC_POWER_FLOW CLLLC_POWER_FLOW_SEC_PRIM
#define CLLLC_INCR_BUILD CLLLC_OPEN_LOOP_BUILD
#define CLLLC_CONTROL_MODE CLLLC_VOLTAGE_MODE
#define CLLLC_TEST_SETUP CLLLC_TEST_SETUP_RES_LOAD
#define CLLLC_PROTECTION CLLLC_PROTECTION_ENABLED
#if CLLLC_SFRA_ALLOWED == 1
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_DISABLED
#else
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_DISABLED
#endif
#define CLLLC_SFRA_AMPLITUDE (float32_t)CLLLC_SFRA_INJECTION_AMPLITUDE_LEVEL2
#endif

#if CLLLC_LAB == 8
#error "untested CLLLC lab"
#define CLLLC_CONTROL_RUNNING_ON 1
#define CLLLC_POWER_FLOW CLLLC_POWER_FLOW_SEC_PRIM
#define CLLLC_INCR_BUILD CLLLC_CLOSED_LOOP_BUILD
#define CLLLC_CONTROL_MODE CLLLC_VOLTAGE_MODE
#define CLLLC_TEST_SETUP CLLLC_TEST_SETUP_RES_LOAD
#define CLLLC_PROTECTION CLLLC_PROTECTION_ENABLED
#if CLLLC_SFRA_ALLOWED == 1
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_DISABLED
#else
#define CLLLC_SFRA_TYPE  CLLLC_SFRA_DISABLED
#endif
#define CLLLC_SFRA_AMPLITUDE (float32_t)CLLLC_SFRA_INJECTION_AMPLITUDE_LEVEL1
#endif


#define CLLLC_ISR1_RUNNING_ON CLLLC_CONTROL_RUNNING_ON

#define CLLLC_ISR2_FREQUENCY_HZ ((float32_t)21250)
#define CLLLC_ISR3_FREQUENCY_HZ ((float32_t)10000)
#define CLLLC_SFRA_ISR_FREQ_HZ       CLLLC_ISR2_FREQUENCY_HZ

//
// Power Stage Related Values
//
#define CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ ((float32_t)80*1000)
#define CLLLC_MAX_PWM_SWITCHING_FREQUENCY_HZ     ((float32_t)90*1000)
#define CLLLC_MIN_PWM_SWITCHING_FREQUENCY_HZ     ((float32_t)75*1000)

#define CLLLC_PRIM_PWM_DEADBAND_RED_NS ((float32_t)102.3)
#define CLLLC_PRIM_PWM_DEADBAND_FED_NS ((float32_t)102.3)
#define CLLLC_SEC_PWM_DEADBAND_RED_NS  ((float32_t)102)
#define CLLLC_SEC_PWM_DEADBAND_FED_NS  ((float32_t)102)

#define CLLLC_VPRIM_MAX_SENSE_VOLTS    ((float32_t)508.685)
#define CLLLC_VSEC_MAX_SENSE_VOLTS     ((float32_t)482.863)
#define CLLLC_VSEC_OPTIMAL_RANGE_VOLTS ((float32_t)450)
#define CLLLC_IPRIM_MAX_SENSE_AMPS    ((float32_t)55.000)
#define CLLLC_ISEC_MAX_SENSE_AMPS     ((float32_t)33.951)
//
// WLESS ITANK_MOD scaling:
// V_ITANK_MOD = (6/pi) * 0.025 V/A * |Icoil|.
// Therefore 3.3 V full-scale corresponds to about 69.12 A peak.
//
// DT calibration: V_ITANK_MOD_ADC = 0.573 * I_COIL + 0.120 V.
// This constant converts the ADC per-unit value, after offset removal, to A.
#define CLLLC_ITANK_MOD_MAX_SENSE_AMPS ((float32_t)(3.3f / 0.573f))
#define CLLLC_IPRIM_TANK_MAX_SENSE_AMPS CLLLC_ITANK_MOD_MAX_SENSE_AMPS
#define CLLLC_ISEC_TANK_MAX_SENSE_AMPS ((float32_t)42.375)

#define CLLLC_VSEC_NOMINAL_VOLTS ((float32_t)350)
#define CLLLC_VPRIM_NOMINAL_VOLTS ((float32_t)400)

#define CLLLC_IPRIM_TRIP_LIMIT_AMPS ((float32_t)30)
#define CLLLC_ISEC_TRIP_LIMIT_AMPS  ((float32_t)30)
#define CLLLC_IPRIM_TANK_TRIP_LIMIT_AMPS ((float32_t)30)
#define CLLLC_ISEC_TANK_TRIP_LIMIT_AMPS  ((float32_t)40)
#define CLLLC_VSEC_TRIP_LIMIT_VOLTS  ((float32_t)450)

//
// Control Loop Design
//

//
// Short PWM pulses will happen when phase makes transitions around 0.25,
// keep the clamp less than or equal to 0.24 to avoid this
//
#define CLLLC_PHASE_COMP_MAX        ((float32_t) 0.4)
#define CLLLC_PHASE_COMP_SCALING    ((float32_t) 1.25)

//
// LAB3
//
//ISR = 120.0 kHz, KDC = 200, fz0 = 0.1 kHz, fz1 = 1000.0 kHz, fp1 = 2.0 kHz
//#define CLLLC_GV1_2P2Z_A1    (float32_t) -1.9004905
//#define CLLLC_GV1_2P2Z_A2    (float32_t) 0.9004905
//#define CLLLC_GV1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GV1_2P2Z_B0    (float32_t) 0.0164854
//#define CLLLC_GV1_2P2Z_B1    (float32_t) -0.0011270
//#define CLLLC_GV1_2P2Z_B2    (float32_t) -0.0151926
//#define CLLLC_GV1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, Kdc = 500 ,fz0 = 0.01 kHz, fz1 = 1000.0 kHz, fp1 = 0.1 kHz
//#define CLLLC_GV1_2P2Z_A1    (float32_t) -1.9947777
//#define CLLLC_GV1_2P2Z_A2    (float32_t) 0.9947777
//#define CLLLC_GV1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GV1_2P2Z_B0    (float32_t) 0.0215783
//#define CLLLC_GV1_2P2Z_B1    (float32_t) -0.0015765
//#define CLLLC_GV1_2P2Z_B2    (float32_t) -0.0199800
//#define CLLLC_GV1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 5000, fz0 = 0.1 kHz, fz1 = 1000.0 kHz, fp1 = 2.0 kHz
#define CLLLC_GV1_2P2Z_A1    (float32_t) -1.9004905
#define CLLLC_GV1_2P2Z_A2    (float32_t) 0.9004905
#define CLLLC_GV1_2P2Z_A3    (float32_t) 0.0000000
#define CLLLC_GV1_2P2Z_B0    (float32_t) 0.4121353
#define CLLLC_GV1_2P2Z_B1    (float32_t) -0.0281741
#define CLLLC_GV1_2P2Z_B2    (float32_t) -0.3798149
#define CLLLC_GV1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 10000, fz0 = 0.1 kHz, fz1 = 1000.0 kHz, fp1 = 2.0 kHz
//#define CLLLC_GV1_2P2Z_A1    (float32_t) -1.9004905
//#define CLLLC_GV1_2P2Z_A2    (float32_t) 0.9004905
//#define CLLLC_GV1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GV1_2P2Z_B0    (float32_t) 0.8242706
//#define CLLLC_GV1_2P2Z_B1    (float32_t) -0.0563483
//#define CLLLC_GV1_2P2Z_B2    (float32_t) -0.7596299
//#define CLLLC_GV1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 13000.0, fz0 = 0.1 kHz, fz1 = 1000.0 kHz, fp1 = 2.0 kHz**
//#define CLLLC_GV1_2P2Z_A1    (float32_t) -1.9004905
//#define CLLLC_GV1_2P2Z_A2    (float32_t) 0.9004905
//#define CLLLC_GV1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GV1_2P2Z_B0    (float32_t) 1.0715518
//#define CLLLC_GV1_2P2Z_B1    (float32_t) -0.0732527
//#define CLLLC_GV1_2P2Z_B2    (float32_t) -0.9875189
//#define CLLLC_GV1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 20000, fz0 = 0.1 kHz, fz1 = 1000.0 kHz, fp1 = 2.0 kHz
//#define CLLLC_GV1_2P2Z_A1    (float32_t) -1.9004905
//#define CLLLC_GV1_2P2Z_A2    (float32_t) 0.9004905
//#define CLLLC_GV1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GV1_2P2Z_B0    (float32_t) 1.6485412
//#define CLLLC_GV1_2P2Z_B1    (float32_t) -0.1126965
//#define CLLLC_GV1_2P2Z_B2    (float32_t) -1.5192598
//#define CLLLC_GV1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 30000.0, fz0 = 0.1 kHz, fz1 = 1000.0 kHz, fp1 = 2.0 kHz
//#define CLLLC_GV1_2P2Z_A1    (float32_t) -1.9004905
//#define CLLLC_GV1_2P2Z_A2    (float32_t) 0.9004905
//#define CLLLC_GV1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GV1_2P2Z_B0    (float32_t) 2.4728118
//#define CLLLC_GV1_2P2Z_B1    (float32_t) -0.1690448
//#define CLLLC_GV1_2P2Z_B2    (float32_t) -2.2788897
//#define CLLLC_GV1_2P2Z_B3    (float32_t) 0.0000000

//
// LAB4
//
//#define CLLLC_GI1_2P2Z_A1    (float32_t) -1.8277396009
//#define CLLLC_GI1_2P2Z_A2    (float32_t) 0.8277396009
//#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000000
//#define CLLLC_GI1_2P2Z_B0    (float32_t) 1.2500036172
//#define CLLLC_GI1_2P2Z_B1    (float32_t) 0.2153188876
//#define CLLLC_GI1_2P2Z_B2    (float32_t) -1.0346715071
//#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000000

//ISR = 120.0 kHz, KDC = 300, fz0 = 3.6 kHz, fz1 = 1000.0 kHz, fp1 = 3.6 kHz
//#define CLLLC_GI1_2P2Z_A1    (float32_t) -1.8277396
//#define CLLLC_GI1_2P2Z_A2    (float32_t) 0.8277396
//#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GI1_2P2Z_B0    (float32_t) 0.0012977
//#define CLLLC_GI1_2P2Z_B1    (float32_t) 0.0001281
//#define CLLLC_GI1_2P2Z_B2    (float32_t) -0.0009952
//#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 3000, fz0 = 3.6 kHz, fz1 = 1000.0 kHz, fp1 = 3.6 kHz
//#define CLLLC_GI1_2P2Z_A1    (float32_t) -1.8277396
//#define CLLLC_GI1_2P2Z_A2    (float32_t) 0.8277396
//#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GI1_2P2Z_B0    (float32_t) 0.0129775
//#define CLLLC_GI1_2P2Z_B1    (float32_t) 0.0012806
//#define CLLLC_GI1_2P2Z_B2    (float32_t) -0.0099515
//#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 3000, fz0 = 0.2 kHz, fz1 = 1000.0 kHz, fp1 = 2.0 kHz
//#define CLLLC_GI1_2P2Z_A1    (float32_t) -1.9004905
//#define CLLLC_GI1_2P2Z_A2    (float32_t) 0.9004905
//#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GI1_2P2Z_B0    (float32_t) 0.1239634
//#define CLLLC_GI1_2P2Z_B1    (float32_t) -0.0078303
//#define CLLLC_GI1_2P2Z_B2    (float32_t) -0.1136454
//#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 9000, fz0 = 0.2 kHz, fz1 = 1000.0 kHz, fp1 = 2.0 kHz
#define CLLLC_GI1_2P2Z_A1    (float32_t) -1.9004905
#define CLLLC_GI1_2P2Z_A2    (float32_t) 0.9004905
#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000
#define CLLLC_GI1_2P2Z_B0    (float32_t) 0.3718903
#define CLLLC_GI1_2P2Z_B1    (float32_t) -0.0234909
#define CLLLC_GI1_2P2Z_B2    (float32_t) -0.3409362
#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 9000, fz0 = 3.6 kHz, fz1 = 1000.0 kHz, fp1 = 3.6 kHz
//#define CLLLC_GI1_2P2Z_A1    (float32_t) -1.8277396
//#define CLLLC_GI1_2P2Z_A2    (float32_t) 0.8277396
//#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GI1_2P2Z_B0    (float32_t) 0.0389324
//#define CLLLC_GI1_2P2Z_B1    (float32_t) 0.0038417
//#define CLLLC_GI1_2P2Z_B2    (float32_t) -0.0298546
//#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 3000, fz0 = 1000.0 kHz, fz1 = 1000.0 kHz, fp1 = 5.0 kHz
//#define CLLLC_GI1_2P2Z_A1    (float32_t) -1.7685034
//#define CLLLC_GI1_2P2Z_A2    (float32_t) 0.7685034
//#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GI1_2P2Z_B0    (float32_t) 0.0015595
//#define CLLLC_GI1_2P2Z_B1    (float32_t) 0.0028895
//#define CLLLC_GI1_2P2Z_B2    (float32_t) 0.0013384
//#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 30000, fz0 = 1000.0 kHz, fz1 = 1000.0 kHz, fp1 = 5.0 kHz
//#define CLLLC_GI1_2P2Z_A1    (float32_t) -1.7685034
//#define CLLLC_GI1_2P2Z_A2    (float32_t) 0.7685034
//#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GI1_2P2Z_B0    (float32_t) 0.0155950
//#define CLLLC_GI1_2P2Z_B1    (float32_t) 0.0288948
//#define CLLLC_GI1_2P2Z_B2    (float32_t) 0.0133843
//#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 30000, fz0 = 1000.0 kHz, fz1 = 1000.0 kHz, fp1 = 1000.0 kHz
//#define CLLLC_GI1_2P2Z_A1    (float32_t) -0.0735837
//#define CLLLC_GI1_2P2Z_A2    (float32_t) -0.9264163
//#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GI1_2P2Z_B0    (float32_t) 0.1297746
//#define CLLLC_GI1_2P2Z_B1    (float32_t) 0.2404507
//#define CLLLC_GI1_2P2Z_B2    (float32_t) 0.1113787
//#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 60000, fz0 = 1000.0 kHz, fz1 = 1000.0 kHz, fp1 = 1000.0 kHz
//#define CLLLC_GI1_2P2Z_A1    (float32_t) -0.0735837
//#define CLLLC_GI1_2P2Z_A2    (float32_t) -0.9264163
//#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GI1_2P2Z_B0    (float32_t) 0.2595493
//#define CLLLC_GI1_2P2Z_B1    (float32_t) 0.4809014
//#define CLLLC_GI1_2P2Z_B2    (float32_t) 0.2227575
//#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 120000, fz0 = 1000.0 kHz, fz1 = 1000.0 kHz, fp1 = 1000.0 kHz
//#define CLLLC_GI1_2P2Z_A1    (float32_t) -0.0735837
//#define CLLLC_GI1_2P2Z_A2    (float32_t) -0.9264163
//#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GI1_2P2Z_B0    (float32_t) 0.5190986
//#define CLLLC_GI1_2P2Z_B1    (float32_t) 0.9618028
//#define CLLLC_GI1_2P2Z_B2    (float32_t) 0.4455149
//#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000

//ISR = 120.0 kHz, KDC = 600000, fz0 = 1000.0 kHz, fz1 = 1000.0 kHz, fp1 = 1000.0 kHz
//#define CLLLC_GI1_2P2Z_A1    (float32_t) -0.0735837
//#define CLLLC_GI1_2P2Z_A2    (float32_t) -0.9264163
//#define CLLLC_GI1_2P2Z_A3    (float32_t) 0.0000000
//#define CLLLC_GI1_2P2Z_B0    (float32_t) 2.5954930
//#define CLLLC_GI1_2P2Z_B1    (float32_t) 4.8090141
//#define CLLLC_GI1_2P2Z_B2    (float32_t) 2.2275746
//#define CLLLC_GI1_2P2Z_B3    (float32_t) 0.0000000

//
// LAB5 
//
#define CLLLC_GI2_2P2Z_A1    (float32_t) 0.0341879720
#define CLLLC_GI2_2P2Z_A2    (float32_t) -0.7668017816
#define CLLLC_GI2_2P2Z_A3    (float32_t) -0.2673861903
#define CLLLC_GI2_2P2Z_B0    (float32_t) 1.3436620732
#define CLLLC_GI2_2P2Z_B1    (float32_t) 0.3459370813
#define CLLLC_GI2_2P2Z_B2    (float32_t) -0.7200660800
#define CLLLC_GI2_2P2Z_B3    (float32_t) -0.2790608258

//
// LAB8 
//
#define CLLLC_GV2_2P2Z_A1    (float32_t) -0.4829060140
#define CLLLC_GV2_2P2Z_A2    (float32_t) -0.5170939860
#define CLLLC_GV2_2P2Z_A3    (float32_t) 0.0000000000
#define CLLLC_GV2_2P2Z_B0    (float32_t) 1.3436620732
#define CLLLC_GV2_2P2Z_B1    (float32_t) -0.3488624959
#define CLLLC_GV2_2P2Z_B2    (float32_t) -0.5396713815
#define CLLLC_GV2_2P2Z_B3    (float32_t) 0.0000000000

//=============================================================================
// User code settings file
//=============================================================================
#include "clllc_user_settings.h"

#ifdef __cplusplus
}
#endif                                  /* extern "C" */

#endif //_PROJSETTINGS_H
