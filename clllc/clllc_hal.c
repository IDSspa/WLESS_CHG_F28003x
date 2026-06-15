//#############################################################################
//
// FILE:   clllc_hal.c
//
// TITLE: solution hardware abstraction layer,
//        This file consists of common variables and functions
//        for a particular hardware board, like functions to read current
//        and voltage signals on the board and functions to setup the
//        basic peripherals of the board
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

#include "clllc_hal.h"

#include "sfo_v8.h"

//
// Global variables used that are applicable to this board
//
// Global variable used by the SFO library
// Result can be used for all HRPWM channels
// This variable is also copied to HRMSTEP
// register by SFO(0) function.
//
int16_t MEP_ScaleFactor;

volatile uint32_t ePWM[9] = {0,
                             EPWM1_BASE,
                             EPWM2_BASE,
                             EPWM3_BASE,
                             EPWM4_BASE,
                             EPWM5_BASE,
                             EPWM6_BASE,
                             EPWM7_BASE,
                             EPWM8_BASE};


CLLLC_HAL_GaNTemp_Struct CLLLC_HAL_GaNTemp_C;

//
//  This routine sets up the basic device configuration such as initializing PLL
//  CPU timers and copying code from FLASH to RAM
//
void CLLLC_HAL_setupDevice(void)
{

    //
    // Initialize device clock and peripherals
    //
    Device_init();

    //
    // Disable pin locks and enable internal pull-ups.
    //
    Device_initGPIO();

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // initialize CPU timers
    // Initialize timer period to rate at which they will be used
    // to slice of task
    // TASK A FREQUENCY
    //
    CPUTimer_setPeriod(CLLLC_TASKA_CPUTIMER_BASE,
                       DEVICE_SYSCLK_FREQ / CLLLC_TASKA_FREQ_HZ);

    //
    // TASK B FREQUENCY
    //
    CPUTimer_setPeriod(CLLLC_TASKB_CPUTIMER_BASE,
                       DEVICE_SYSCLK_FREQ / CLLLC_TASKB_FREQ_HZ);

    //
    // TASK C FREQUENCY
    //
    CPUTimer_setPeriod(CLLLC_TASKC_CPUTIMER_BASE,
                       DEVICE_SYSCLK_FREQ / CLLLC_TASKC_FREQ_HZ);

    //
    // Initialize pre-scale counter to divide by 1 (SYSCLKOUT)
    //
    CPUTimer_setPreScaler(CLLLC_TASKA_CPUTIMER_BASE, 0);

    //
    // Initialize pre-scale counter to divide by 1 (SYSCLKOUT)
    //
    CPUTimer_setPreScaler(CLLLC_TASKB_CPUTIMER_BASE, 0);

    //
    // Initialize pre-scale counter to divide by 1 (SYSCLKOUT)
    //
    CPUTimer_setPreScaler(CLLLC_TASKC_CPUTIMER_BASE, 0);

    //
    // Make sure timer is stopped
    //
    CPUTimer_stopTimer(CLLLC_TASKA_CPUTIMER_BASE);
    CPUTimer_stopTimer(CLLLC_TASKB_CPUTIMER_BASE);
    CPUTimer_stopTimer(CLLLC_TASKC_CPUTIMER_BASE);

    CPUTimer_setEmulationMode(CLLLC_TASKA_CPUTIMER_BASE,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    CPUTimer_setEmulationMode(CLLLC_TASKB_CPUTIMER_BASE,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    CPUTimer_setEmulationMode(CLLLC_TASKC_CPUTIMER_BASE,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);

    CPUTimer_startTimer(CLLLC_TASKA_CPUTIMER_BASE);
    CPUTimer_startTimer(CLLLC_TASKB_CPUTIMER_BASE);
    CPUTimer_startTimer(CLLLC_TASKC_CPUTIMER_BASE);

}

void CLLLC_HAL_setupADC(void)
{





    //#####BUILD_F28003X#####
    ADC_setVREF(ADCA_BASE, ADC_REFERENCE_INTERNAL, ADC_REFERENCE_3_3V);
    ADC_setVREF(ADCB_BASE, ADC_REFERENCE_INTERNAL, ADC_REFERENCE_3_3V);
    ADC_setVREF(ADCC_BASE, ADC_REFERENCE_INTERNAL, ADC_REFERENCE_3_3V);

    ADC_setPrescaler(ADCA_BASE, ADC_CLK_DIV_2_0);
    ADC_setPrescaler(ADCB_BASE, ADC_CLK_DIV_2_0);
    ADC_setPrescaler(ADCC_BASE, ADC_CLK_DIV_2_0);

    ADC_enableConverter(ADCA_BASE);
    ADC_enableConverter(ADCB_BASE);
    ADC_enableConverter(ADCC_BASE);

    //#####BUILD_F28003X#####



    DEVICE_DELAY_US(1000);

    //
    //ISEC
    //
    ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
                 CLLLC_ISEC_ADC_SOC_NO_1,
                 CLLLC_ISEC_ADC_TRIG_SOURCE_1,
                 CLLLC_ISEC_ADC_PIN,
                 CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);

    #if CLLLC_OVERSAMPLING_ENABLED == 1
        ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
                     CLLLC_ISEC_ADC_SOC_NO_2,
                     CLLLC_ISEC_ADC_TRIG_SOURCE_1,
                     CLLLC_ISEC_ADC_PIN,
                     CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
                     CLLLC_ISEC_ADC_SOC_NO_3,
                     CLLLC_ISEC_ADC_TRIG_SOURCE_1,
                     CLLLC_ISEC_ADC_PIN,
                     CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
                     CLLLC_ISEC_ADC_SOC_NO_4,
                     CLLLC_ISEC_ADC_TRIG_SOURCE_1,
                     CLLLC_ISEC_ADC_PIN,
                     CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
                     CLLLC_ISEC_ADC_SOC_NO_5,
                     CLLLC_ISEC_ADC_TRIG_SOURCE_1,
                     CLLLC_ISEC_ADC_PIN,
                     CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
                     CLLLC_ISEC_ADC_SOC_NO_6,
                     CLLLC_ISEC_ADC_TRIG_SOURCE_1,
                     CLLLC_ISEC_ADC_PIN,
                     CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
                     CLLLC_ISEC_ADC_SOC_NO_7,
                     CLLLC_ISEC_ADC_TRIG_SOURCE_2,
                     CLLLC_ISEC_ADC_PIN,
                     CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
                     CLLLC_ISEC_ADC_SOC_NO_8,
                     CLLLC_ISEC_ADC_TRIG_SOURCE_2,
                     CLLLC_ISEC_ADC_PIN,
                     CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
                     CLLLC_ISEC_ADC_SOC_NO_9,
                     CLLLC_ISEC_ADC_TRIG_SOURCE_2,
                     CLLLC_ISEC_ADC_PIN,
                     CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
                     CLLLC_ISEC_ADC_SOC_NO_10,
                     CLLLC_ISEC_ADC_TRIG_SOURCE_2,
                     CLLLC_ISEC_ADC_PIN,
                     CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
                     CLLLC_ISEC_ADC_SOC_NO_11,
                     CLLLC_ISEC_ADC_TRIG_SOURCE_2,
                     CLLLC_ISEC_ADC_PIN,
                     CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);

//        ADC_setupSOC(CLLLC_ISEC_ADC_MODULE,
//                     CLLLC_ISEC_ADC_SOC_NO_12,
//                     CLLLC_ISEC_ADC_TRIG_SOURCE_2,
//                     CLLLC_ISEC_ADC_PIN,
//                     CLLLC_ISEC_ADC_ACQPS_SYS_CLKS);
    #endif

    //
    //VPRIM
    //
    ADC_setupSOC(CLLLC_VPRIM_ADC_MODULE,
                 CLLLC_VPRIM_ADC_SOC_NO_1,
                 CLLLC_VPRIM_ADC_TRIG_SOURCE_1,
                 CLLLC_VPRIM_ADC_PIN,
                 CLLLC_VPRIM_ADC_ACQPS_SYS_CLKS);

    #if CLLLC_OVERSAMPLING_ENABLED == 1
        ADC_setupSOC(CLLLC_VPRIM_ADC_MODULE,
                     CLLLC_VPRIM_ADC_SOC_NO_2,
                     CLLLC_VPRIM_ADC_TRIG_SOURCE_2,
                     CLLLC_VPRIM_ADC_PIN,
                     CLLLC_VPRIM_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VPRIM_ADC_MODULE,
                     CLLLC_VPRIM_ADC_SOC_NO_3,
                     CLLLC_VPRIM_ADC_TRIG_SOURCE_3,
                     CLLLC_VPRIM_ADC_PIN,
                     CLLLC_VPRIM_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VPRIM_ADC_MODULE,
                     CLLLC_VPRIM_ADC_SOC_NO_4,
                     CLLLC_VPRIM_ADC_TRIG_SOURCE_4,
                     CLLLC_VPRIM_ADC_PIN,
                     CLLLC_VPRIM_ADC_ACQPS_SYS_CLKS);
    #endif


    //
    //VSEC
    //
    ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                 CLLLC_VSEC_ADC_SOC_NO_1,
                 CLLLC_VSEC_ADC_TRIG_SOURCE_1,
                 CLLLC_VSEC_ADC_PIN,
                 CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

    #if CLLLC_OVERSAMPLING_ENABLED == 1

        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                     CLLLC_VSEC_ADC_SOC_NO_2,
                     CLLLC_VSEC_ADC_TRIG_SOURCE_1,
                     CLLLC_VSEC_ADC_PIN,
                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                     CLLLC_VSEC_ADC_SOC_NO_2,
                     CLLLC_VSEC_ADC_TRIG_SOURCE_1,
                     CLLLC_VSEC_ADC_PIN,
                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                     CLLLC_VSEC_ADC_SOC_NO_3,
                     CLLLC_VSEC_ADC_TRIG_SOURCE_1,
                     CLLLC_VSEC_ADC_PIN,
                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                     CLLLC_VSEC_ADC_SOC_NO_4,
                     CLLLC_VSEC_ADC_TRIG_SOURCE_1,
                     CLLLC_VSEC_ADC_PIN,
                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                     CLLLC_VSEC_ADC_SOC_NO_5,
                     CLLLC_VSEC_ADC_TRIG_SOURCE_1,
                     CLLLC_VSEC_ADC_PIN,
                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                     CLLLC_VSEC_ADC_SOC_NO_6,
                     CLLLC_VSEC_ADC_TRIG_SOURCE_1,
                     CLLLC_VSEC_ADC_PIN,
                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                     CLLLC_VSEC_ADC_SOC_NO_7,
                     CLLLC_VSEC_ADC_TRIG_SOURCE_2,
                     CLLLC_VSEC_ADC_PIN,
                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                     CLLLC_VSEC_ADC_SOC_NO_8,
                     CLLLC_VSEC_ADC_TRIG_SOURCE_2,
                     CLLLC_VSEC_ADC_PIN,
                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                     CLLLC_VSEC_ADC_SOC_NO_9,
                     CLLLC_VSEC_ADC_TRIG_SOURCE_2,
                     CLLLC_VSEC_ADC_PIN,
                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                     CLLLC_VSEC_ADC_SOC_NO_10,
                     CLLLC_VSEC_ADC_TRIG_SOURCE_2,
                     CLLLC_VSEC_ADC_PIN,
                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                     CLLLC_VSEC_ADC_SOC_NO_11,
                     CLLLC_VSEC_ADC_TRIG_SOURCE_2,
                     CLLLC_VSEC_ADC_PIN,
                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

//        ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
//                     CLLLC_VSEC_ADC_SOC_NO_12,
//                     CLLLC_VSEC_ADC_TRIG_SOURCE_2,
//                     CLLLC_VSEC_ADC_PIN,
//                     CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);
    #endif

    //
    // IPRIM
    //
    ADC_setupSOC(CLLLC_IPRIM_ADC_MODULE,
                 CLLLC_IPRIM_ADC_SOC_NO,
                 CLLLC_IPRIM_ADC_TRIG_SOURCE,
                 CLLLC_IPRIM_ADC_PIN,
                 CLLLC_IPRIM_ADC_ACQPS_SYS_CLKS);

    //
    // setup another slow ADC conversion for ISR3 trigger
    //
    ADC_setupSOC(CLLLC_VSEC_ADC_MODULE,
                 CLLLC_VSEC_ADC_SOC_NO_13,
                 CLLLC_ADC_SOC_TRIG5,
                 CLLLC_VSEC_ADC_PIN,
                 CLLLC_VSEC_ADC_ACQPS_SYS_CLKS);

}

void CLLLC_HAL_setupIprimSensedSignalChain(void)
{
}

void CLLLC_HAL_setupBoardProtection()
{
    //
    // Disable all the muxes first
    //
    XBAR_enableEPWMMux(XBAR_TRIP4, 0x00);

#if CLLLC_BOARD_PROTECTION_IPRIM == 1

    ASysCtl_selectCMPHPMux(CLLLC_IPRIM_CMPSS_ASYSCTRL_CMPHPMUX,
                           CLLLC_IPRIM_CMPSS_ASYSCTRL_MUX_VALUE);
    ASysCtl_selectCMPLPMux(CLLLC_IPRIM_CMPSS_ASYSCTRL_CMPLPMUX,
                           CLLLC_IPRIM_CMPSS_ASYSCTRL_MUX_VALUE);

    CLLLC_HAL_setupCMPSSHighLowLimit(CLLLC_IPRIM_CMPSS_BASE,
                                    CLLLC_IPRIM_TRIP_LIMIT_AMPS,
                                    CLLLC_IPRIM_MAX_SENSE_AMPS,
                                    CLLLC_CMPSS_HYSTERESIS,
                                    CLLLC_CMPSSS_FILTER_PRESCALAR,
                                    CLLLC_CMPSS_WINODW,
                                    CLLLC_CMPSS_THRESHOLD);

    XBAR_setEPWMMuxConfig(XBAR_TRIP4, CLLLC_IPRIM_CMPSS_XBAR_MUX_VAL);
    XBAR_enableEPWMMux(XBAR_TRIP4, CLLLC_IPRIM_CMPSS_XBAR_MUX);
    XBAR_clearInputFlag(CLLLC_IPRIM_CMPSS_XBAR_FLAG1);
    XBAR_clearInputFlag(CLLLC_IPRIM_CMPSS_XBAR_FLAG2);
#else

    #warning BOARD_PROTECTION_IPRIM is disabled --Not available on F28003x due to IPRIM_TANK CMPSS2 resource conflict

#endif

#if CLLLC_BOARD_PROTECTION_ISEC == 1

    ASysCtl_selectCMPHPMux(CLLLC_ISEC_CMPSS_ASYSCTRL_CMPHPMUX,
                           CLLLC_ISEC_CMPSS_ASYSCTRL_MUX_VALUE);
    ASysCtl_selectCMPLPMux(CLLLC_ISEC_CMPSS_ASYSCTRL_CMPLPMUX,
                           CLLLC_ISEC_CMPSS_ASYSCTRL_MUX_VALUE);

    CLLLC_HAL_setupCMPSSHighLowLimit(CLLLC_ISEC_CMPSS_BASE,
                CLLLC_ISEC_TRIP_LIMIT_AMPS,
                CLLLC_ISEC_MAX_SENSE_AMPS,
                CLLLC_CMPSS_HYSTERESIS,
                CLLLC_CMPSSS_FILTER_PRESCALAR,
                CLLLC_CMPSS_WINODW,
                CLLLC_CMPSS_THRESHOLD);

    XBAR_setEPWMMuxConfig(XBAR_TRIP4, CLLLC_ISEC_XBAR_MUX_VAL);
    XBAR_enableEPWMMux(XBAR_TRIP4, CLLLC_ISEC_XBAR_MUX);
    XBAR_clearInputFlag(CLLLC_ISEC_CMPSS_XBAR_FLAG1);
    XBAR_clearInputFlag(CLLLC_ISEC_CMPSS_XBAR_FLAG2);
#else
    #warning BOARD_PROTECTION_ISEC is disabled  --Not available on F28003x due to IPRIM_TANK CMPSS2 resource conflict
#endif

#if CLLLC_BOARD_PROTECTION_VSEC == 1

    ASysCtl_selectCMPHPMux(CLLLC_VSEC_CMPSS_ASYSCTRL_CMPHPMUX,
                           CLLLC_VSEC_CMPSS_ASYSCTRL_MUX_VALUE);
    ASysCtl_selectCMPLPMux(CLLLC_VSEC_CMPSS_ASYSCTRL_CMPLPMUX,
                           CLLLC_VSEC_CMPSS_ASYSCTRL_MUX_VALUE);

    CLLLC_HAL_setupCMPSSHighLimit(CLLLC_VSEC_CMPSS_BASE,
                CLLLC_VSEC_TRIP_LIMIT_VOLTS,
                CLLLC_VSEC_MAX_SENSE_VOLTS,
                CLLLC_CMPSS_HYSTERESIS,
                CLLLC_CMPSSS_FILTER_PRESCALAR,
                CLLLC_CMPSS_WINODW,
                CLLLC_CMPSS_THRESHOLD);

    XBAR_setEPWMMuxConfig(XBAR_TRIP4, CLLLC_VSEC_CMPSS_XBAR_MUX_VAL);
    XBAR_enableEPWMMux(XBAR_TRIP4, CLLLC_VSEC_CMPSS_XBAR_MUX);
    XBAR_clearInputFlag(CLLLC_VSEC_CMPSS_XBAR_FLAG1);
#else
    #warning BOARD_PROTECTION_VSEC is disabled --Not available on F28003x due to IPRIM_TANK CMPSS2 resource conflict
#endif

#if CLLLC_BOARD_PROTECTION_GANFAULT == 1
    GPIO_setDirectionMode(CLLLC_GANFAULTn_GPIO, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(CLLLC_GANFAULTn_GPIO, GPIO_QUAL_6SAMPLE);
    GPIO_setPinConfig(CLLLC_GANFAULTn_GPIO_PIN_CONFIG);
    GPIO_setPadConfig(CLLLC_GANFAULTn_XBAR_BASE, GPIO_PIN_TYPE_STD);

    XBAR_setInputPin(CLLLC_GANFAULTn_XBAR_BASE, CLLLC_GANFAULTn_XBAR_INPUT, CLLLC_GANFAULTn_GPIO);
    EPWM_clearOneShotTripZoneFlag(CLLLC_PRIM_LEG1_PWM_BASE, CLLLC_GANFAULTn_EPWM_FLAG);

    //
    // Enable the OSHT TZ2 trip
    //
    EPWM_enableTripZoneSignals(CLLLC_PRIM_LEG1_PWM_BASE,
                               EPWM_TZ_SIGNAL_OSHT2);
    EPWM_enableTripZoneSignals(CLLLC_PRIM_LEG2_PWM_BASE,
                               EPWM_TZ_SIGNAL_OSHT2);
    EPWM_enableTripZoneSignals(CLLLC_SEC_LEG1_PWM_BASE,
                               EPWM_TZ_SIGNAL_OSHT2);
    EPWM_enableTripZoneSignals(CLLLC_SEC_LEG2_PWM_BASE,
                               EPWM_TZ_SIGNAL_OSHT2);
#else
    #warning BOARD_PROTECTION_GANFAULT is disabled
#endif

#if CLLLC_BOARD_PROTECTION_IPRIM == 1 ||                                       \
    CLLLC_BOARD_PROTECTION_ISEC == 1  ||                                       \
    CLLLC_BOARD_PROTECTION_VSEC == 1

    //
    //Trip 4 is the input to the DCAHCOMPSEL
    //
    EPWM_selectDigitalCompareTripInput(CLLLC_PRIM_LEG1_PWM_BASE,
                                       EPWM_DC_TRIP_TRIPIN4,
                                       EPWM_DC_TYPE_DCAH);
    EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_PRIM_LEG1_PWM_BASE,
                                                 EPWM_TZ_DC_OUTPUT_A1,
                                                 EPWM_TZ_EVENT_DCXH_HIGH);
    EPWM_setDigitalCompareEventSource(CLLLC_PRIM_LEG1_PWM_BASE,
                                      EPWM_DC_MODULE_A,
                                      EPWM_DC_EVENT_1,
                                      EPWM_DC_EVENT_SOURCE_ORIG_SIGNAL);
    EPWM_setDigitalCompareEventSyncMode(CLLLC_PRIM_LEG1_PWM_BASE,
                                        EPWM_DC_MODULE_A ,
                                        EPWM_DC_EVENT_1,
                                        EPWM_DC_EVENT_INPUT_NOT_SYNCED);

    EPWM_selectDigitalCompareTripInput(CLLLC_PRIM_LEG2_PWM_BASE,
                                       EPWM_DC_TRIP_TRIPIN4,
                                       EPWM_DC_TYPE_DCAH);
    EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_PRIM_LEG2_PWM_BASE,
                                                 EPWM_TZ_DC_OUTPUT_A1,
                                                 EPWM_TZ_EVENT_DCXH_HIGH);
    EPWM_setDigitalCompareEventSource(CLLLC_PRIM_LEG2_PWM_BASE,
                                      EPWM_DC_MODULE_A,
                                      EPWM_DC_EVENT_1,
                                      EPWM_DC_EVENT_SOURCE_ORIG_SIGNAL);
    EPWM_setDigitalCompareEventSyncMode(CLLLC_PRIM_LEG2_PWM_BASE,
                                        EPWM_DC_MODULE_A ,
                                        EPWM_DC_EVENT_1,
                                        EPWM_DC_EVENT_INPUT_NOT_SYNCED);

    EPWM_selectDigitalCompareTripInput(CLLLC_SEC_LEG1_PWM_BASE,
                                       EPWM_DC_TRIP_TRIPIN4,
                                       EPWM_DC_TYPE_DCAH);
    EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_SEC_LEG1_PWM_BASE,
                                                 EPWM_TZ_DC_OUTPUT_A1,
                                                 EPWM_TZ_EVENT_DCXH_HIGH);
    EPWM_setDigitalCompareEventSource(CLLLC_SEC_LEG1_PWM_BASE,
                                      EPWM_DC_MODULE_A,
                                      EPWM_DC_EVENT_1,
                                      EPWM_DC_EVENT_SOURCE_ORIG_SIGNAL);
    EPWM_setDigitalCompareEventSyncMode(CLLLC_SEC_LEG1_PWM_BASE,
                                        EPWM_DC_MODULE_A ,
                                        EPWM_DC_EVENT_1,
                                        EPWM_DC_EVENT_INPUT_NOT_SYNCED);

    EPWM_selectDigitalCompareTripInput(CLLLC_SEC_LEG2_PWM_BASE,
                                       EPWM_DC_TRIP_TRIPIN4,
                                       EPWM_DC_TYPE_DCAH);
    EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_SEC_LEG2_PWM_BASE,
                                                 EPWM_TZ_DC_OUTPUT_A1,
                                                 EPWM_TZ_EVENT_DCXH_HIGH);
    EPWM_setDigitalCompareEventSource(CLLLC_SEC_LEG2_PWM_BASE,
                                      EPWM_DC_MODULE_A,
                                      EPWM_DC_EVENT_1,
                                      EPWM_DC_EVENT_SOURCE_ORIG_SIGNAL);
    EPWM_setDigitalCompareEventSyncMode(CLLLC_SEC_LEG2_PWM_BASE,
                                        EPWM_DC_MODULE_A ,
                                        EPWM_DC_EVENT_1,
                                        EPWM_DC_EVENT_INPUT_NOT_SYNCED);

    //
    // Enable the following trips DCAEVT1
    //
    EPWM_enableTripZoneSignals(CLLLC_PRIM_LEG1_PWM_BASE,
                               EPWM_TZ_SIGNAL_DCAEVT1);
    EPWM_enableTripZoneSignals(CLLLC_PRIM_LEG2_PWM_BASE,
                               EPWM_TZ_SIGNAL_DCAEVT1);
    EPWM_enableTripZoneSignals(CLLLC_SEC_LEG1_PWM_BASE,
                               EPWM_TZ_SIGNAL_DCAEVT1);
    EPWM_enableTripZoneSignals(CLLLC_SEC_LEG2_PWM_BASE,
                               EPWM_TZ_SIGNAL_DCAEVT1);

#else
    #warning All CLLLC voltage and current comparator-based protections are disabled
#endif

    //
    // Enable the CBC Emulator Stop trip
    //
    EPWM_enableTripZoneSignals(CLLLC_PRIM_LEG1_PWM_BASE,
                               EPWM_TZ_SIGNAL_CBC6);
    EPWM_enableTripZoneSignals(CLLLC_PRIM_LEG2_PWM_BASE,
                               EPWM_TZ_SIGNAL_CBC6);
    EPWM_enableTripZoneSignals(CLLLC_SEC_LEG1_PWM_BASE,
                               EPWM_TZ_SIGNAL_CBC6);
    EPWM_enableTripZoneSignals(CLLLC_SEC_LEG2_PWM_BASE,
                               EPWM_TZ_SIGNAL_CBC6);

    //
    // What do we want the OST/CBC events to do?
    // TZA events can force EPWMxA
    // TZB events can force EPWMxB
    //
    EPWM_setTripZoneAction(CLLLC_PRIM_LEG1_PWM_BASE,
                           EPWM_TZ_ACTION_EVENT_TZA,
                           EPWM_TZ_ACTION_LOW);
    EPWM_setTripZoneAction(CLLLC_PRIM_LEG1_PWM_BASE,
                           EPWM_TZ_ACTION_EVENT_TZB,
                           EPWM_TZ_ACTION_LOW);
    EPWM_setTripZoneAction(CLLLC_PRIM_LEG2_PWM_BASE,
                           EPWM_TZ_ACTION_EVENT_TZA,
                           EPWM_TZ_ACTION_LOW);
    EPWM_setTripZoneAction(CLLLC_PRIM_LEG2_PWM_BASE,
                           EPWM_TZ_ACTION_EVENT_TZB,
                           EPWM_TZ_ACTION_LOW);
    EPWM_setTripZoneAction(CLLLC_SEC_LEG1_PWM_BASE,
                           EPWM_TZ_ACTION_EVENT_TZA,
                           EPWM_TZ_ACTION_LOW);
    EPWM_setTripZoneAction(CLLLC_SEC_LEG1_PWM_BASE,
                           EPWM_TZ_ACTION_EVENT_TZB,
                           EPWM_TZ_ACTION_LOW);
    EPWM_setTripZoneAction(CLLLC_SEC_LEG2_PWM_BASE,
                           EPWM_TZ_ACTION_EVENT_TZA,
                           EPWM_TZ_ACTION_LOW);
    EPWM_setTripZoneAction(CLLLC_SEC_LEG2_PWM_BASE,
                           EPWM_TZ_ACTION_EVENT_TZB,
                           EPWM_TZ_ACTION_LOW);

    //
    // Clear any spurious trip
    //
    EPWM_clearTripZoneFlag(CLLLC_PRIM_LEG1_PWM_BASE ,
                           (EPWM_TZ_INTERRUPT_OST | EPWM_TZ_INTERRUPT_DCAEVT1));
    EPWM_clearTripZoneFlag(CLLLC_PRIM_LEG2_PWM_BASE ,
                           (EPWM_TZ_INTERRUPT_OST | EPWM_TZ_INTERRUPT_DCAEVT1));
    EPWM_clearTripZoneFlag(CLLLC_SEC_LEG1_PWM_BASE ,
                           (EPWM_TZ_INTERRUPT_OST | EPWM_TZ_INTERRUPT_DCAEVT1));
    EPWM_clearTripZoneFlag(CLLLC_SEC_LEG2_PWM_BASE ,
                           (EPWM_TZ_INTERRUPT_OST | EPWM_TZ_INTERRUPT_DCAEVT1));

    EPWM_forceTripZoneEvent(CLLLC_PRIM_LEG1_PWM_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_forceTripZoneEvent(CLLLC_PRIM_LEG2_PWM_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_forceTripZoneEvent(CLLLC_SEC_LEG1_PWM_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_forceTripZoneEvent(CLLLC_SEC_LEG2_PWM_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

void CLLLC_HAL_setupSynchronousRectificationAction(uint16_t powerFlow)
{
    if(powerFlow == CLLLC_POWER_FLOW_PRIM_SEC)
    {

        ASysCtl_selectCMPHPMux(CLLLC_ISEC_TANK_CMPSS_ASYSCTRL_CMPHPMUX,
                               CLLLC_ISEC_TANK_CMPSS_ASYSCTRL_MUX_VALUE);

        ASysCtl_selectCMPLPMux(CLLLC_ISEC_TANK_CMPSS_ASYSCTRL_CMPLPMUX,
                               CLLLC_ISEC_TANK_CMPSS_ASYSCTRL_MUX_VALUE);

        //
        //Enable CMPSS
        //
        CMPSS_enableModule(CLLLC_ISEC_TANK_CMPSS_BASE);

        //
        //Use VDDA as the reference for comparator DACs
        //
        CMPSS_configDAC(CLLLC_ISEC_TANK_CMPSS_BASE,
                       CMPSS_DACVAL_SYSCLK | CMPSS_DACREF_VDDA
                       | CMPSS_DACSRC_SHDW);

        //
        // set DAC H and L values
        //
        CMPSS_setDACValueHigh(CLLLC_ISEC_TANK_CMPSS_BASE,
                              CLLLC_ISEC_TANK_DACHVAL);
        CMPSS_setDACValueLow(CLLLC_ISEC_TANK_CMPSS_BASE,
                             CLLLC_ISEC_TANK_DACLVAL);

        //
        // CMPH comparison is inverted because we want to trip for xA
        // when this signal goes below zero, as the pin is connected to + sign
        // the output needs to be inverted to be the right logical level
        //
        CMPSS_configHighComparator(CLLLC_ISEC_TANK_CMPSS_BASE,
                                   CMPSS_INSRC_DAC | CMPSS_INV_INVERTED);

        //
        // CMPL is not inverted because we want to trip when the
        // signal goes above zero, CMPSS pin is connected to + sign of
        // the comparator, hence no sign inversion required
        //
        CMPSS_configLowComparator(CLLLC_ISEC_TANK_CMPSS_BASE, CMPSS_INSRC_DAC );

        //
        // configure SEC PWM LEG1 PWM to issue the blanking signal
        //
        CMPSS_configBlanking(CLLLC_ISEC_TANK_CMPSS_BASE, CLLLC_SEC_LEG1_PWM_NO);

        CMPSS_enableBlanking(CLLLC_ISEC_TANK_CMPSS_BASE);

        CMPSS_configLatchOnPWMSYNC(CLLLC_ISEC_TANK_CMPSS_BASE, TRUE, TRUE);



        EALLOW;
        HWREGH(CLLLC_ISEC_TANK_CMPSS_BASE + CMPSS_O_COMPDACCTL) |= 0x04;//todo verify assigned value


        EDIS;

        //
        // configure the filter to the lowest setting
        //
        CMPSS_configFilterHigh(CLLLC_ISEC_TANK_CMPSS_BASE, 0, 1, 1);
        CMPSS_configFilterLow(CLLLC_ISEC_TANK_CMPSS_BASE, 0, 1, 1);

        //
        //Reset filter logic & start filtering
        //
        CMPSS_initFilterHigh(CLLLC_ISEC_TANK_CMPSS_BASE);
        CMPSS_initFilterLow(CLLLC_ISEC_TANK_CMPSS_BASE);


        CMPSS_configOutputsHigh(CLLLC_ISEC_TANK_CMPSS_BASE,
                                CMPSS_TRIP_LATCH | CMPSS_TRIPOUT_LATCH);
        CMPSS_configOutputsLow(CLLLC_ISEC_TANK_CMPSS_BASE,
                               CMPSS_TRIP_LATCH | CMPSS_TRIPOUT_LATCH);

        //
        //Comparator hysteresis control , set to 2x typical value
        //
        CMPSS_setHysteresis(CLLLC_ISEC_TANK_CMPSS_BASE, 2);

        //
        // Clear the latched comparator events
        //
        CMPSS_clearFilterLatchHigh(CLLLC_ISEC_TANK_CMPSS_BASE);
        CMPSS_clearFilterLatchLow(CLLLC_ISEC_TANK_CMPSS_BASE);

        XBAR_setEPWMMuxConfig(XBAR_TRIP5,
                              CLLLC_ISEC_TANK_H_PWM_XBAR_MUX_VAL);
        XBAR_enableEPWMMux(XBAR_TRIP5,
                           CLLLC_ISEC_TANK_H_XBAR_MUX);

        XBAR_setEPWMMuxConfig(XBAR_TRIP7,
                              CLLLC_ISEC_TANK_L_PWM_XBAR_MUX_VAL);
        XBAR_enableEPWMMux(XBAR_TRIP7,
                           CLLLC_ISEC_TANK_L_XBAR_MUX);

        XBAR_clearInputFlag(CLLLC_ISEC_TANK_H_CMPSS_XBAR_FLAG);
        XBAR_clearInputFlag(CLLLC_ISEC_TANK_L_CMPSS_XBAR_FLAG);

        //
        // configure EPWM to issue blanking pulse
        //

        EPWM_setDigitalCompareBlankingEvent(CLLLC_SEC_LEG1_PWM_BASE,
                                        EPWM_DC_WINDOW_START_TBCTR_ZERO_PERIOD);

        EPWM_setDigitalCompareWindowOffset(CLLLC_SEC_LEG1_PWM_BASE,
                                                0);

        EPWM_setDigitalCompareWindowLength(CLLLC_SEC_LEG1_PWM_BASE, 25);

        EPWM_enableDigitalCompareBlankingWindow(CLLLC_SEC_LEG1_PWM_BASE);

        //
        // Now also program the behavior of the PWM to accept
        // the TRIP5 and 7 that are generated by the CMPSS
        // Qualify TRIP5 as DCAL event
        // Qualify TRIP7 as DCBL event
        //

        EPWM_selectDigitalCompareTripInput(CLLLC_SEC_LEG1_PWM_BASE,
                                           EPWM_DC_TRIP_TRIPIN5,
                                           EPWM_DC_TYPE_DCAL);

        EPWM_selectDigitalCompareTripInput(CLLLC_SEC_LEG1_PWM_BASE,
                                           EPWM_DC_TRIP_TRIPIN7,
                                           EPWM_DC_TYPE_DCBL);

        EPWM_selectDigitalCompareTripInput(CLLLC_SEC_LEG2_PWM_BASE,
                                           EPWM_DC_TRIP_TRIPIN7,
                                           EPWM_DC_TYPE_DCAL);

        EPWM_selectDigitalCompareTripInput(CLLLC_SEC_LEG2_PWM_BASE,
                                           EPWM_DC_TRIP_TRIPIN5,
                                           EPWM_DC_TYPE_DCBL);

        //
        // Qualify DCAEVT2 to be when DCAL is high
        // Qualify DCBEVT2 to be when DCBL is high
        //
        EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_SEC_LEG1_PWM_BASE,
                                          EPWM_TZ_DC_OUTPUT_A2,
                                          EPWM_TZ_EVENT_DCXL_HIGH);

        EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_SEC_LEG1_PWM_BASE,
                                          EPWM_TZ_DC_OUTPUT_B2,
                                          EPWM_TZ_EVENT_DCXL_HIGH);

        EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_SEC_LEG2_PWM_BASE,
                                          EPWM_TZ_DC_OUTPUT_A2,
                                          EPWM_TZ_EVENT_DCXL_HIGH);

        EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_SEC_LEG2_PWM_BASE,
                                          EPWM_TZ_DC_OUTPUT_B2,
                                          EPWM_TZ_EVENT_DCXL_HIGH);


        //
        // sets the ETZE bit to 1, to enable advanced actions on the PWM
        //
        EPWM_enableTripZoneAdvAction(CLLLC_SEC_LEG1_PWM_BASE);
        EPWM_enableTripZoneAdvAction(CLLLC_SEC_LEG2_PWM_BASE);

        //
        // set PWM trip behavior to force low on trip action
        //
#if CLLLC_TRANSFORMER_POLARITY == CLLLC_POSITIVE_POLARITY
        //The following code should be used when the transformer polarity matches the schematic.
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_D, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_U, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_D, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_U, EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_D, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_U, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_D, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_U, EPWM_TZ_ADV_ACTION_LOW);
#else
        //The following code should be used when the transformer polarity does not match the schematic.
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_U, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_D, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_U, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_D, EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_U, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_D, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_U, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_SEC_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_D, EPWM_TZ_ADV_ACTION_LOW);
#endif

        //
        // first set all the TZCTLDCX registers to do nothing
        //
        EALLOW;
        HWREGH(CLLLC_SEC_LEG1_PWM_BASE + EPWM_O_TZCTLDCA) = 0xFFFF;
        HWREGH(CLLLC_SEC_LEG1_PWM_BASE + EPWM_O_TZCTLDCB) = 0xFFFF;
        HWREGH(CLLLC_SEC_LEG2_PWM_BASE + EPWM_O_TZCTLDCA) = 0xFFFF;
        HWREGH(CLLLC_SEC_LEG2_PWM_BASE + EPWM_O_TZCTLDCB) = 0xFFFF;
        EDIS;

        //
        // no describe the behavior in case when DCAEVT2 and
        //
#if CLLLC_TRANSFORMER_POLARITY == CLLLC_POSITIVE_POLARITY
        //The following code should be used when the transformer polarity matches the schematic.
        EPWM_setTripZoneAdvDigitalCompareActionA(CLLLC_SEC_LEG1_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_D,
                                        EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvDigitalCompareActionB(CLLLC_SEC_LEG1_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_U,
                                        EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvDigitalCompareActionA(CLLLC_SEC_LEG2_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_U,
                                        EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvDigitalCompareActionB(CLLLC_SEC_LEG2_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_D,
                                        EPWM_TZ_ADV_ACTION_LOW);
#else
        //The following code should be used when the transformer polarity does not match the schematic.
        EPWM_setTripZoneAdvDigitalCompareActionA(CLLLC_SEC_LEG1_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_U,
                                        EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvDigitalCompareActionB(CLLLC_SEC_LEG1_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_D,
                                        EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvDigitalCompareActionA(CLLLC_SEC_LEG2_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_D,
                                        EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvDigitalCompareActionB(CLLLC_SEC_LEG2_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_U,
                                        EPWM_TZ_ADV_ACTION_LOW);
#endif
        //
        //    clear the cycle by cycle trip on zero and period
        //
        EPWM_selectCycleByCycleTripZoneClearEvent(CLLLC_SEC_LEG1_PWM_BASE,
                                    EPWM_TZ_CBC_PULSE_CLR_CNTR_ZERO_PERIOD);
        EPWM_selectCycleByCycleTripZoneClearEvent(CLLLC_SEC_LEG2_PWM_BASE,
                                    EPWM_TZ_CBC_PULSE_CLR_CNTR_ZERO_PERIOD);
    }
    else if(powerFlow == CLLLC_POWER_FLOW_SEC_PRIM)
    {

        //
        //Enable CMPSS
        //
        CMPSS_enableModule(CLLLC_IPRIM_TANK_CMPSS_BASE);

        //
        //Use VDDA as the reference for comparator DACs
        //
        CMPSS_configDAC(CLLLC_IPRIM_TANK_CMPSS_BASE,
                       CMPSS_DACVAL_SYSCLK | CMPSS_DACREF_VDDA
                       | CMPSS_DACSRC_SHDW);

        //
        // set DAC H and L values
        //
        CMPSS_setDACValueHigh(CLLLC_IPRIM_TANK_CMPSS_BASE,
                              CLLLC_IPRIM_TANK_DACHVAL);
        CMPSS_setDACValueLow(CLLLC_IPRIM_TANK_CMPSS_BASE,
                             CLLLC_IPRIM_TANK_DACLVAL);

        //
        // CMPH comparison is inverted because we want to trip for xA
        // when this signal goes below zero, as the pin is connected to + sign
        // the output needs to be inverted to be the right logical level
        //
        CMPSS_configHighComparator(CLLLC_IPRIM_TANK_CMPSS_BASE,
                                   CMPSS_INSRC_DAC | CMPSS_INV_INVERTED);

        //
        // CMPL is not inverted because we want to trip when the
        // signal goes above zero, CMPSS pin is connected to + sign of
        // the comparator, hence no sign inversion required
        //
        CMPSS_configLowComparator(CLLLC_IPRIM_TANK_CMPSS_BASE,
                                  CMPSS_INSRC_DAC );

        //
        // configure PRIM PWM LEG1 PWM to issue the blanking signal
        //
        CMPSS_configBlanking(CLLLC_IPRIM_TANK_CMPSS_BASE,
                             CLLLC_PRIM_LEG1_PWM_NO);

        CMPSS_enableBlanking(CLLLC_IPRIM_TANK_CMPSS_BASE);

        CMPSS_configLatchOnPWMSYNC(CLLLC_IPRIM_TANK_CMPSS_BASE, TRUE, TRUE);



        EALLOW;
        HWREGH(CLLLC_IPRIM_TANK_CMPSS_BASE + CMPSS_O_COMPDACCTL) |= 0x00; //todo: verify value and function


        EDIS;

        //
        // configure the filter to the lowest setting
        //
        CMPSS_configFilterHigh(CLLLC_IPRIM_TANK_CMPSS_BASE, 0, 1, 1);
        CMPSS_configFilterLow(CLLLC_IPRIM_TANK_CMPSS_BASE, 0, 1, 1);

        //
        //Reset filter logic & start filtering
        //
        CMPSS_initFilterHigh(CLLLC_IPRIM_TANK_CMPSS_BASE);
        CMPSS_initFilterLow(CLLLC_IPRIM_TANK_CMPSS_BASE);


        CMPSS_configOutputsHigh(CLLLC_IPRIM_TANK_CMPSS_BASE,
                                CMPSS_TRIP_LATCH | CMPSS_TRIPOUT_LATCH);
        CMPSS_configOutputsLow(CLLLC_IPRIM_TANK_CMPSS_BASE,
                               CMPSS_TRIP_LATCH | CMPSS_TRIPOUT_LATCH);

        //
        //Comparator hysteresis control , set to 2x typical value
        //
        CMPSS_setHysteresis(CLLLC_IPRIM_TANK_CMPSS_BASE, 2);

        //
        // Clear the latched comparator events
        //
        CMPSS_clearFilterLatchHigh(CLLLC_IPRIM_TANK_CMPSS_BASE);
        CMPSS_clearFilterLatchLow(CLLLC_IPRIM_TANK_CMPSS_BASE);

        XBAR_setEPWMMuxConfig(XBAR_TRIP5,
                              CLLLC_IPRIM_TANK_H_PWM_XBAR_MUX_VAL);
        XBAR_enableEPWMMux(XBAR_TRIP5,
                           CLLLC_IPRIM_TANK_H_XBAR_MUX);

        XBAR_setEPWMMuxConfig(XBAR_TRIP7,
                              CLLLC_IPRIM_TANK_L_PWM_XBAR_MUX_VAL);
        XBAR_enableEPWMMux(XBAR_TRIP7,
                           CLLLC_IPRIM_TANK_L_XBAR_MUX);

        XBAR_clearInputFlag(CLLLC_IPRIM_TANK_CMPSS_XBAR_FLAG1);
        XBAR_clearInputFlag(CLLLC_IPRIM_TANK_CMPSS_XBAR_FLAG2);

        //
        // configure EPWM to issue blanking pulse
        //

        EPWM_setDigitalCompareBlankingEvent(CLLLC_PRIM_LEG1_PWM_BASE,
                                        EPWM_DC_WINDOW_START_TBCTR_ZERO_PERIOD);

        EPWM_setDigitalCompareWindowOffset(CLLLC_PRIM_LEG1_PWM_BASE,
                                                0);

        EPWM_setDigitalCompareWindowLength(CLLLC_PRIM_LEG1_PWM_BASE, 25);

        EPWM_enableDigitalCompareBlankingWindow(CLLLC_PRIM_LEG1_PWM_BASE);

        //
        // Now also program the behavior of the PWM to accept
        // the TRIP5 and 7 that are generated by the CMPSS
        // Qualify TRIP5 as DCAL event
        // Qualify TRIP7 as DCBL event
        //

        EPWM_selectDigitalCompareTripInput(CLLLC_PRIM_LEG1_PWM_BASE,
                                           EPWM_DC_TRIP_TRIPIN5,
                                           EPWM_DC_TYPE_DCAL);

        EPWM_selectDigitalCompareTripInput(CLLLC_PRIM_LEG1_PWM_BASE,
                                           EPWM_DC_TRIP_TRIPIN7,
                                           EPWM_DC_TYPE_DCBL);

        EPWM_selectDigitalCompareTripInput(CLLLC_PRIM_LEG2_PWM_BASE,
                                           EPWM_DC_TRIP_TRIPIN7,
                                           EPWM_DC_TYPE_DCAL);

        EPWM_selectDigitalCompareTripInput(CLLLC_PRIM_LEG2_PWM_BASE,
                                           EPWM_DC_TRIP_TRIPIN5,
                                           EPWM_DC_TYPE_DCBL);

        //
        // Qualify DCAEVT2 to be when DCAL is high
        // Qualify DCBEVT2 to be when DCBL is high
        //
        EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_PRIM_LEG1_PWM_BASE,
                                          EPWM_TZ_DC_OUTPUT_A2,
                                          EPWM_TZ_EVENT_DCXL_HIGH);

        EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_PRIM_LEG1_PWM_BASE,
                                          EPWM_TZ_DC_OUTPUT_B2,
                                          EPWM_TZ_EVENT_DCXL_HIGH);

        EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_PRIM_LEG2_PWM_BASE,
                                          EPWM_TZ_DC_OUTPUT_A2,
                                          EPWM_TZ_EVENT_DCXL_HIGH);

        EPWM_setTripZoneDigitalCompareEventCondition(CLLLC_PRIM_LEG2_PWM_BASE,
                                          EPWM_TZ_DC_OUTPUT_B2,
                                          EPWM_TZ_EVENT_DCXL_HIGH);


        //
        // sets the ETZE bit to 1, to enable advanced actions on the PWM
        //
        EPWM_enableTripZoneAdvAction(CLLLC_PRIM_LEG1_PWM_BASE);
        EPWM_enableTripZoneAdvAction(CLLLC_PRIM_LEG2_PWM_BASE);

        //
        // set PWM trip behavior to force low on trip action
        //
        EPWM_setTripZoneAdvAction(CLLLC_PRIM_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_D, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_PRIM_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_U, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_PRIM_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_D, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_PRIM_LEG1_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_U, EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvAction(CLLLC_PRIM_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_D, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_PRIM_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZA_U, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_PRIM_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_D, EPWM_TZ_ADV_ACTION_LOW);
        EPWM_setTripZoneAdvAction(CLLLC_PRIM_LEG2_PWM_BASE, EPWM_TZ_ADV_ACTION_EVENT_TZB_U, EPWM_TZ_ADV_ACTION_LOW);

        //
        // first set all the TZCTLDCX registers to do nothing
        //
        EALLOW;
        HWREGH(CLLLC_PRIM_LEG1_PWM_BASE + EPWM_O_TZCTLDCA) = 0xFFFF;
        HWREGH(CLLLC_PRIM_LEG1_PWM_BASE + EPWM_O_TZCTLDCB) = 0xFFFF;
        HWREGH(CLLLC_PRIM_LEG2_PWM_BASE + EPWM_O_TZCTLDCA) = 0xFFFF;
        HWREGH(CLLLC_PRIM_LEG2_PWM_BASE + EPWM_O_TZCTLDCB) = 0xFFFF;
        EDIS;

        //
        // now describe the behavior in case when DCAEVT2 and
        //
        EPWM_setTripZoneAdvDigitalCompareActionA(CLLLC_PRIM_LEG1_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_U,
                                        EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvDigitalCompareActionB(CLLLC_PRIM_LEG1_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_D,
                                        EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvDigitalCompareActionA(CLLLC_PRIM_LEG2_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_D,
                                        EPWM_TZ_ADV_ACTION_LOW);

        EPWM_setTripZoneAdvDigitalCompareActionB(CLLLC_PRIM_LEG2_PWM_BASE,
                                        EPWM_TZ_ADV_ACTION_EVENT_DCxEVT2_U,
                                        EPWM_TZ_ADV_ACTION_LOW);

        //
        //    clear the cycle by cycle trip on zero and period
        //
        EPWM_selectCycleByCycleTripZoneClearEvent(CLLLC_PRIM_LEG1_PWM_BASE,
                                    EPWM_TZ_CBC_PULSE_CLR_CNTR_ZERO_PERIOD);
        EPWM_selectCycleByCycleTripZoneClearEvent(CLLLC_PRIM_LEG2_PWM_BASE,
                                    EPWM_TZ_CBC_PULSE_CLR_CNTR_ZERO_PERIOD);
    }
    else
    {
        //
        // not a valid option, do nothing
        //

    }

}

void CLLLC_HAL_setupSynchronousRectificationActionDebug(uint16_t powerFlow)
{
    //
    // For debug purpose bring out the CTRIPHSEL on an output xBar
    //
    if(powerFlow == 1)
    {



        XBAR_setOutputMuxConfig(OUTPUTXBAR_BASE,
                                XBAR_OUTPUT3,
                                CLLLC_ISEC_TANK_H_OUT_XBAR_MUX_VAL);
        XBAR_enableOutputMux(OUTPUTXBAR_BASE,
                             XBAR_OUTPUT3,
                             CLLLC_ISEC_TANK_H_XBAR_MUX);
        XBAR_setOutputMuxConfig(OUTPUTXBAR_BASE,
                                XBAR_OUTPUT4,
                                CLLLC_ISEC_TANK_L_OUT_XBAR_MUX_VAL);
        XBAR_enableOutputMux(OUTPUTXBAR_BASE,
                             XBAR_OUTPUT4,
                             CLLLC_ISEC_TANK_L_XBAR_MUX);

    }
    else
    {


        XBAR_setOutputMuxConfig(OUTPUTXBAR_BASE,
                                XBAR_OUTPUT3,
                               CLLLC_IPRIM_TANK_H_OUT_XBAR_MUX_VAL);
        XBAR_enableOutputMux(OUTPUTXBAR_BASE,
                             XBAR_OUTPUT3,
                             CLLLC_IPRIM_TANK_H_XBAR_MUX);
        XBAR_setOutputMuxConfig(XBAR_BASE,
                                XBAR_OUTPUT4,
                               CLLLC_IPRIM_TANK_L_OUT_XBAR_MUX_VAL);
        XBAR_enableOutputMux(OUTPUTXBAR_BASE,
                             XBAR_OUTPUT4,
                             CLLLC_IPRIM_TANK_L_XBAR_MUX);
    }

}

void CLLLC_HAL_setupTrigForADC(void)
{

    //
    // Set ADC Trigger for CLLLC control variables to be synchronous to the CLLLC PWM
    //
    EPWM_setADCTriggerSource(CLLLC_PRIM_LEG1_PWM_BASE,
                                EPWM_SOC_A, EPWM_SOC_TBCTR_PERIOD);
    EPWM_setADCTriggerEventPrescale(CLLLC_PRIM_LEG1_PWM_BASE,
                                    EPWM_SOC_A, 1);
    EPWM_enableADCTrigger(CLLLC_PRIM_LEG1_PWM_BASE,
                          EPWM_SOC_A);

    EPWM_setADCTriggerSource(CLLLC_PRIM_LEG1_PWM_BASE,
                                EPWM_SOC_B, EPWM_SOC_TBCTR_ZERO);
    EPWM_setADCTriggerEventPrescale(CLLLC_PRIM_LEG1_PWM_BASE,
                                    EPWM_SOC_B, 1);
    EPWM_enableADCTrigger(CLLLC_PRIM_LEG1_PWM_BASE,
                          EPWM_SOC_B);

    //
    //PWM module is used to trigger the SOC in this application
    //As control is carried out in ISR2,
    //The PWM used for ISR2 is configured below
    //Select SOC from counter at ctr =CMPBD
    //
    EPWM_setADCTriggerSource(CLLLC_ISR2_PWM_BASE,
                             EPWM_SOC_A, EPWM_SOC_TBCTR_D_CMPB );
    //
    // set CMPB such that the triggers for the ADC are centered around zero
    // of the ISR generating timebase
    //
    EPWM_setCounterCompareValue(CLLLC_ISR2_PWM_BASE,
                                 EPWM_COUNTER_COMPARE_B,
                                 (CLLLC_IPRIM_ADC_ACQPS_SYS_CLKS *
                      ((float32_t)CLLLC_PWMSYSCLOCK_FREQ_HZ /
                       (float32_t)CLLLC_CPU_SYS_CLOCK_FREQ_HZ)));

    //
    // Generate pulse on 1st event
    //
    EPWM_setADCTriggerEventPrescale(CLLLC_ISR2_PWM_BASE,
                                    EPWM_SOC_A, 1);

    //
    // Enable SOC on A group
    //
    EPWM_enableADCTrigger(CLLLC_ISR2_PWM_BASE,
                          EPWM_SOC_A);


    //
    // SOC from CLLLC_ADC_TIGGER_PWM1_BASE
    //

    //
    // TRIG1 -> Select SOCA from counter at ctr = CMPCU
    //
    EPWM_setADCTriggerSource(CLLLC_ADC_TIGGER_PWM1_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_U_CMPC );
    EPWM_setCounterCompareValue(CLLLC_ADC_TIGGER_PWM1_BASE, EPWM_COUNTER_COMPARE_C,
                                (uint16_t)(((uint16_t)CLLLC_ADC_BASE_SAMPLING_PERIOD >> 1) - ((uint16_t)CLLLC_ADC_ACQPS_SYS_CLKS >> 1)));
    EPWM_setADCTriggerEventPrescale(CLLLC_ADC_TIGGER_PWM1_BASE, EPWM_SOC_A, 1);
    EPWM_enableADCTrigger(CLLLC_ADC_TIGGER_PWM1_BASE, EPWM_SOC_A);

    //
    // TRIG2 -> Select SOCB from counter at ctr = CMPCD
    //
    EPWM_setADCTriggerSource(CLLLC_ADC_TIGGER_PWM1_BASE, EPWM_SOC_B, EPWM_SOC_TBCTR_D_CMPD );
    EPWM_setCounterCompareValue(CLLLC_ADC_TIGGER_PWM1_BASE , EPWM_COUNTER_COMPARE_D, (uint16_t)CLLLC_ADC_ACQPS_SYS_CLKS >> 1);
    EPWM_setADCTriggerEventPrescale(CLLLC_ADC_TIGGER_PWM1_BASE , EPWM_SOC_B, 1);
    EPWM_enableADCTrigger(CLLLC_ADC_TIGGER_PWM1_BASE, EPWM_SOC_B);

    //
    // SOC from CLLLC_ADC_TIGGER_PWM2_BASE
    //

    //
    // Select SOCA from counter at ctr = CMPCU
    //
    EPWM_setADCTriggerSource(CLLLC_ADC_TIGGER_PWM2_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_U_CMPC );
    EPWM_setCounterCompareValue(CLLLC_ADC_TIGGER_PWM2_BASE, EPWM_COUNTER_COMPARE_C,
                                (uint16_t)(((uint16_t)CLLLC_ADC_BASE_SAMPLING_PERIOD >> 2) - (uint16_t)CLLLC_ADC_ACQPS_SYS_CLKS >> 1));
    EPWM_setADCTriggerEventPrescale(CLLLC_ADC_TIGGER_PWM2_BASE, EPWM_SOC_A, 1);
    EPWM_enableADCTrigger(CLLLC_ADC_TIGGER_PWM2_BASE, EPWM_SOC_A);

    //
    // Select SOCB from counter at ctr =CMPCD
    //
    EPWM_setADCTriggerSource(CLLLC_ADC_TIGGER_PWM2_BASE, EPWM_SOC_B, EPWM_SOC_TBCTR_D_CMPD );
    EPWM_setCounterCompareValue(CLLLC_ADC_TIGGER_PWM2_BASE, EPWM_COUNTER_COMPARE_D,
                                (uint16_t)(((uint16_t)CLLLC_ADC_BASE_SAMPLING_PERIOD >> 2) + (uint16_t)CLLLC_ADC_ACQPS_SYS_CLKS >> 1));
    EPWM_setADCTriggerEventPrescale(CLLLC_ADC_TIGGER_PWM2_BASE, EPWM_SOC_B, 1);
    EPWM_enableADCTrigger(CLLLC_ADC_TIGGER_PWM2_BASE, EPWM_SOC_B);


//    //
//    // for the faster signals such as shunt current sense, the PWM time base of
//    // the module used to control the power stage must be used.
//    // Also as we are interested in the peak value we must sample close to the
//    // falling edge of the leg1 H PWM
//    // Select SOC from counter at ctr =CMPA
//    //
//    EPWM_setADCTriggerSource(CLLLC_PRIM_LEG1_PWM_BASE,
//                             EPWM_SOC_A, EPWM_SOC_TBCTR_U_CMPD);
//    //
//    // CMPA is updated in the ISR so nothing to do here
//    // pre-scale the trigger of the ADC conversion
//    //
//    EPWM_setADCTriggerEventPrescale(CLLLC_PRIM_LEG1_PWM_BASE,
//                                        EPWM_SOC_A, 6);
//
//    EPWM_enableADCTrigger(CLLLC_PRIM_LEG1_PWM_BASE,
//                          EPWM_SOC_A);

}

void CLLLC_HAL_toggleLED1(void)
{
    static uint16_t ledCnt1 = 0;

    if(ledCnt1 == 0)
    {
        GPIO_togglePin(CLLLC_GPIO_LED1);
        ledCnt1 = 5;
    }
    else
    {
        ledCnt1--;
    }

}

void CLLLC_HAL_setupECAPforProfilingCode(void)
{

    //
    // Route profiling GPIO through INPUTXBAR
    //


    XBAR_setInputPin(INPUTXBAR_BASE, CLLLC_PROFILING1_INPUTXBAR, CLLLC_GPIO_PROFILING2);
    XBAR_setInputPin(INPUTXBAR_BASE, CLLLC_PROFILING2_INPUTXBAR, CLLLC_GPIO_PROFILING3);

    //
    // Configure ECAP to measure the rising to falling edge pulse
    //
    ECAP_disableTimeStampCapture(CLLLC_PROFILING1_ECAP);

    ECAP_stopCounter(CLLLC_PROFILING1_ECAP);
    ECAP_enableCaptureMode(CLLLC_PROFILING1_ECAP);

    ECAP_setCaptureMode(CLLLC_PROFILING1_ECAP, ECAP_CONTINUOUS_CAPTURE_MODE,
                        ECAP_EVENT_4);

    ECAP_setEventPolarity(CLLLC_PROFILING1_ECAP, ECAP_EVENT_1,
                          ECAP_EVNT_FALLING_EDGE);
    ECAP_setEventPolarity(CLLLC_PROFILING1_ECAP, ECAP_EVENT_2,
                          ECAP_EVNT_RISING_EDGE);
    ECAP_setEventPolarity(CLLLC_PROFILING1_ECAP, ECAP_EVENT_3,
                          ECAP_EVNT_FALLING_EDGE);
    ECAP_setEventPolarity(CLLLC_PROFILING1_ECAP, ECAP_EVENT_4,
                          ECAP_EVNT_RISING_EDGE);

    ECAP_enableCounterResetOnEvent(CLLLC_PROFILING1_ECAP, ECAP_EVENT_1);
    ECAP_enableCounterResetOnEvent(CLLLC_PROFILING1_ECAP, ECAP_EVENT_2);
    ECAP_enableCounterResetOnEvent(CLLLC_PROFILING1_ECAP, ECAP_EVENT_3);
    ECAP_enableCounterResetOnEvent(CLLLC_PROFILING1_ECAP, ECAP_EVENT_4);

    ECAP_startCounter(CLLLC_PROFILING1_ECAP);
    ECAP_enableTimeStampCapture(CLLLC_PROFILING1_ECAP);
    ECAP_reArm(CLLLC_PROFILING1_ECAP);


    ECAP_disableTimeStampCapture(CLLLC_PROFILING2_ECAP);

    ECAP_stopCounter(CLLLC_PROFILING2_ECAP);
    ECAP_enableCaptureMode(CLLLC_PROFILING2_ECAP);

    ECAP_setCaptureMode(CLLLC_PROFILING2_ECAP, ECAP_CONTINUOUS_CAPTURE_MODE,
                        ECAP_EVENT_4);

    ECAP_setEventPolarity(CLLLC_PROFILING2_ECAP, ECAP_EVENT_1,
                          ECAP_EVNT_FALLING_EDGE);
    ECAP_setEventPolarity(CLLLC_PROFILING2_ECAP, ECAP_EVENT_2,
                          ECAP_EVNT_RISING_EDGE);
    ECAP_setEventPolarity(CLLLC_PROFILING2_ECAP, ECAP_EVENT_3,
                          ECAP_EVNT_FALLING_EDGE);
    ECAP_setEventPolarity(CLLLC_PROFILING2_ECAP, ECAP_EVENT_4,
                          ECAP_EVNT_RISING_EDGE);

    ECAP_enableCounterResetOnEvent(CLLLC_PROFILING2_ECAP, ECAP_EVENT_1);
    ECAP_enableCounterResetOnEvent(CLLLC_PROFILING2_ECAP, ECAP_EVENT_2);
    ECAP_enableCounterResetOnEvent(CLLLC_PROFILING2_ECAP, ECAP_EVENT_3);
    ECAP_enableCounterResetOnEvent(CLLLC_PROFILING2_ECAP, ECAP_EVENT_4);

    ECAP_startCounter(CLLLC_PROFILING2_ECAP);
    ECAP_enableTimeStampCapture(CLLLC_PROFILING2_ECAP);
    ECAP_reArm(CLLLC_PROFILING2_ECAP);

    //
    // Recognize the XBAR as ECAP input
    //
    ECAP_selectECAPInput(CLLLC_PROFILING1_ECAP,
                       (ECAP_InputCaptureSignals)CLLLC_PROFILING1_ECAP_XBAR_MUX);
    ECAP_selectECAPInput(CLLLC_PROFILING2_ECAP,
                       (ECAP_InputCaptureSignals)CLLLC_PROFILING2_ECAP_XBAR_MUX);

}

//
//
//
void CLLLC_HAL_setupGPIOs()
{

    //
    // Setup ISR profiling GPIOs
    //
    GPIO_setDirectionMode(CLLLC_GPIO_PROFILING1, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(CLLLC_GPIO_PROFILING2, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(CLLLC_GPIO_PROFILING3, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(CLLLC_GPIO_STEPCHANGEFREQ, GPIO_DIR_MODE_OUT);

    GPIO_setQualificationMode(CLLLC_GPIO_PROFILING1, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(CLLLC_GPIO_PROFILING2, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(CLLLC_GPIO_PROFILING3, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(CLLLC_GPIO_STEPCHANGEFREQ, GPIO_QUAL_SYNC);

    GPIO_setPinConfig(CLLLC_GPIO_PROFILING1_PIN_CONFIG);
    GPIO_setPinConfig(CLLLC_GPIO_PROFILING2_PIN_CONFIG);
    GPIO_setPinConfig(CLLLC_GPIO_PROFILING3_PIN_CONFIG);
    GPIO_setPinConfig(CLLLC_GPIO_STEPCHANGEFREQ_PIN_CONFIG);

#if CLLLC_ISR1_RUNNING_ON == CLA_CORE
    GPIO_setMasterCore(CLLLC_GPIO_PROFILING1, GPIO_CORE_CPU1_CLA1);
#endif
#if CLLLC_ISR2_RUNNING_ON == CLA_CORE
    GPIO_setMasterCore(CLLLC_GPIO_PROFILING2, GPIO_CORE_CPU1_CLA1);
    GPIO_setMasterCore(CLLLC_GPIO_STEPCHANGEFREQ, GPIO_CORE_CPU1_CLA1);
#endif

    //
    // Setup LED1 GPIO
    //
    GPIO_setDirectionMode(CLLLC_GPIO_LED1, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(CLLLC_GPIO_LED1, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(CLLLC_GPIO_LED1_PIN_CONFIG);

    //
    // Setup commutation inductor GPIO
    //
    GPIO_setDirectionMode(CLLLC_GPIO_LC, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(CLLLC_GPIO_LC, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(CLLLC_GPIO_LC_PIN_CONFIG);

    //
    // Setup secondary sensing diagnostic pin
    //
    GPIO_setDirectionMode(CLLLC_GPIO_SECSENSEDIAG, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(CLLLC_GPIO_SECSENSEDIAG, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(CLLLC_GPIO_SECSENSEDIAG_PIN_CONFIG);

    //
    // Setup GaN temperature mux select pins
    //
    GPIO_setDirectionMode(CLLLC_GPIO_TEMP_MUX_SEL0, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(CLLLC_GPIO_TEMP_MUX_SEL0, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(CLLLC_GPIO_TEMP_MUX_SEL0_PIN_CONFIG);

    GPIO_setDirectionMode(CLLLC_GPIO_TEMP_MUX_SEL1, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(CLLLC_GPIO_TEMP_MUX_SEL1, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(CLLLC_GPIO_TEMP_MUX_SEL1_PIN_CONFIG);

    GPIO_setDirectionMode(CLLLC_GPIO_TEMP_MUX_SEL2, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(CLLLC_GPIO_TEMP_MUX_SEL2, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(CLLLC_GPIO_TEMP_MUX_SEL2_PIN_CONFIG);

    //
    // Setup GaN temperature ECAP pins
    //
    GPIO_setDirectionMode(CLLLC_GPIO_TEMP_MUX_OUT1, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(CLLLC_GPIO_TEMP_MUX_OUT1, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(CLLLC_GPIO_TEMP_MUX_OUT1_PIN_CONFIG);

    GPIO_setDirectionMode(CLLLC_GPIO_TEMP_MUX_OUT2, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(CLLLC_GPIO_TEMP_MUX_OUT2, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(CLLLC_GPIO_TEMP_MUX_OUT2_PIN_CONFIG);
}

void CLLLC_HAL_setupSCI(void)
{
    //
    // setup Gpio for SCI comms
    //
    GPIO_setPinConfig(GPIO_28_SCIA_RX);
    GPIO_setPinConfig(GPIO_29_SCIA_TX);
    GPIO_setQualificationMode(28, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(29, GPIO_QUAL_ASYNC);
    EDIS;

    //
    // Initialize SCIA and its FIFO.
    //
    SCI_performSoftwareReset(SCIA_BASE);

    //
    // 50000000 is the LSPCLK or the Clock used for the SCI Module
    // 57600 is the Baudrate desired of the SCI module
    // 1 stop bit,  No parity, 8 char bits,
    //
    SCI_setConfig(SCIA_BASE, 50000000, 57600,
                    (SCI_CONFIG_WLEN_8 |
                    SCI_CONFIG_STOP_ONE |
                    SCI_CONFIG_PAR_NONE));

    //
    //No loopback
    //
    SCI_disableLoopback(SCIA_BASE);

    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);

    SCI_enableInterrupt(SCIA_BASE, SCI_INT_RXRDY_BRKDT | SCI_INT_TXRDY);

    //
    // Disable RX ERR, SLEEP, TXWAKE
    //
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXRDY | SCI_INT_RXRDY_BRKDT );

}

//
// the following routines sends the command packet to the PFC stage
// the command packet is organized as follows
// /r/n{Mode}{Voltage ref}
// in total 4 hex values are transmitted ,
// first two are /r and /n which signify start of packet
// next is a hex ASCII for P or I depending on the mode ,
// if mode is 0 that is PFC/Inveretr is off O is transmitted
// if mode is 1 that is PFC then P is transmitted
// if mode is 2 that is Inverter I is transmitted
// the next value is hex that represents the voltage reference
// the hex value is determined as follows (voltage_ref - 400 +32)
// +30 is done to avoid special hex characters
// -400 is done to keep the range of this message in 8 bits
//
#pragma FUNC_ALWAYS_INLINE(CLLLC_HAL_sendCommandOverSCI)
#pragma FUNC_ALWAYS_INLINE(SCI_writeCharBlockingFIFO)
void CLLLC_HAL_sendCommandOverSCI(uint16_t mode, uint16_t voltage_ref)
{
    //
    // below is usefull for debug
    // return, 13 decimal
    //SCI_writeCharBlockingFIFO(SCIA_BASE, 13);
    // new line, 10 decimal
    //SCI_writeCharBlockingFIFO(SCIA_BASE, 10);
    //
    if(mode == 2)
    {
        //
        // Inverter Mode I-73 decimal
        //
        SCI_writeCharBlockingFIFO(SCIA_BASE, 2);
    }
    else if(mode == 1)
    {
        //
        // PFC mode P - 80 decimal
        //
        SCI_writeCharBlockingFIFO(SCIA_BASE, 1);
    }
    else
    {
        //
        // OFF 0 - 80 decimal
        //
        SCI_writeCharBlockingFIFO(SCIA_BASE, 0);
    }

    SCI_writeCharBlockingFIFO(SCIA_BASE, voltage_ref - 400 + 32);
}

//
// mode is 0: no PWM
// mode is 1: prim PWM on sec PWM off
// mode is 2: prim and sec PWM on
// mode is 3: prim and sec PWM on
//
void CLLLC_HAL_setupPWMpins(uint16_t mode)
{
    //
    // if mode is 0 then disable prim & sec PWMs
    //
    if(mode == 0)
    {
        GPIO_writePin(CLLLC_PRIM_LEG1_PWM_H_GPIO, 0);
        GPIO_writePin(CLLLC_PRIM_LEG1_PWM_L_GPIO, 0);
        GPIO_setPinConfig(CLLLC_PRIM_LEG1_PWM_H_DIS_GPIO_PIN_CONFIG);
        GPIO_setPinConfig(CLLLC_PRIM_LEG1_PWM_L_DIS_GPIO_PIN_CONFIG);

        GPIO_writePin(CLLLC_PRIM_LEG2_PWM_H_GPIO, 0);
        GPIO_writePin(CLLLC_PRIM_LEG2_PWM_L_GPIO, 0);
        GPIO_setPinConfig(CLLLC_PRIM_LEG2_PWM_H_DIS_GPIO_PIN_CONFIG);
        GPIO_setPinConfig(CLLLC_PRIM_LEG2_PWM_L_DIS_GPIO_PIN_CONFIG);

    }
    //
    // if mode is 0 or 1 then disable sec PWM
    //
    if(mode == 0 || mode == 1)
    {
        GPIO_writePin(CLLLC_SEC_LEG1_PWM_H_GPIO, 0);
        GPIO_writePin(CLLLC_SEC_LEG1_PWM_L_GPIO, 0);
        GPIO_setPinConfig(CLLLC_SEC_LEG1_PWM_H_DIS_GPIO_PIN_CONFIG);
        GPIO_setPinConfig(CLLLC_SEC_LEG1_PWM_L_DIS_GPIO_PIN_CONFIG);

        GPIO_writePin(CLLLC_SEC_LEG2_PWM_H_GPIO, 0);
        GPIO_writePin(CLLLC_SEC_LEG2_PWM_L_GPIO, 0);
        GPIO_setPinConfig(CLLLC_SEC_LEG2_PWM_H_DIS_GPIO_PIN_CONFIG);
        GPIO_setPinConfig(CLLLC_SEC_LEG2_PWM_L_DIS_GPIO_PIN_CONFIG);
    }
    //
    // if mode is 1 or 2 or 3 then enable prim PWM
    //
    if(mode == 1 || mode == 2 || mode == 3)
    {
        GPIO_setDirectionMode(CLLLC_PRIM_LEG1_PWM_H_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(CLLLC_PRIM_LEG1_PWM_H_GPIO, GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(CLLLC_PRIM_LEG1_PWM_H_GPIO_PIN_CONFIG );

        GPIO_setDirectionMode(CLLLC_PRIM_LEG1_PWM_L_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(CLLLC_PRIM_LEG1_PWM_L_GPIO, GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(CLLLC_PRIM_LEG1_PWM_L_GPIO_PIN_CONFIG );

        GPIO_setDirectionMode(CLLLC_PRIM_LEG2_PWM_H_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(CLLLC_PRIM_LEG2_PWM_H_GPIO, GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(CLLLC_PRIM_LEG2_PWM_H_GPIO_PIN_CONFIG );

        GPIO_setDirectionMode(CLLLC_PRIM_LEG2_PWM_L_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(CLLLC_PRIM_LEG2_PWM_L_GPIO, GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(CLLLC_PRIM_LEG2_PWM_L_GPIO_PIN_CONFIG );
    }
    //
    // if mode is 2 or 3 then enable sec PWM
    //
    if(mode == 2 || mode == 3)
    {

        GPIO_setDirectionMode(CLLLC_SEC_LEG1_PWM_L_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(CLLLC_SEC_LEG1_PWM_L_GPIO, GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(CLLLC_SEC_LEG1_PWM_L_GPIO_PIN_CONFIG );

        GPIO_setDirectionMode(CLLLC_SEC_LEG2_PWM_L_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(CLLLC_SEC_LEG2_PWM_L_GPIO, GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(CLLLC_SEC_LEG2_PWM_L_GPIO_PIN_CONFIG );

        GPIO_setDirectionMode(CLLLC_SEC_LEG1_PWM_H_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(CLLLC_SEC_LEG1_PWM_H_GPIO, GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(CLLLC_SEC_LEG1_PWM_H_GPIO_PIN_CONFIG );

        GPIO_setDirectionMode(CLLLC_SEC_LEG2_PWM_H_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(CLLLC_SEC_LEG2_PWM_H_GPIO, GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(CLLLC_SEC_LEG2_PWM_H_GPIO_PIN_CONFIG );
    }
}


void CLLLC_HAL_setupCLA(void)
{
    //
    // setup CLA to register an interrupt
    //
#if CLLLC_ISR1_RUNNING_ON == CLA_CORE

    memcpy((uint32_t *)&Cla1ProgRunStart, (uint32_t *)&Cla1ProgLoadStart,
            (uint32_t)&Cla1ProgLoadSize );

    //
    // first assign memory to CLA
    //
    MemCfg_setLSRAMMasterSel(MEMCFG_SECT_LS0, MEMCFG_LSRAMMASTER_CPU_CLA1);
    MemCfg_setLSRAMMasterSel(MEMCFG_SECT_LS1, MEMCFG_LSRAMMASTER_CPU_CLA1);
    MemCfg_setLSRAMMasterSel(MEMCFG_SECT_LS2, MEMCFG_LSRAMMASTER_CPU_CLA1);
    MemCfg_setLSRAMMasterSel(MEMCFG_SECT_LS3, MEMCFG_LSRAMMASTER_CPU_CLA1);
    MemCfg_setLSRAMMasterSel(MEMCFG_SECT_LS4, MEMCFG_LSRAMMASTER_CPU_CLA1);
    MemCfg_setLSRAMMasterSel(MEMCFG_SECT_LS5, MEMCFG_LSRAMMASTER_CPU_CLA1);

    MemCfg_setCLAMemType(MEMCFG_SECT_LS0, MEMCFG_CLA_MEM_DATA);
    MemCfg_setCLAMemType(MEMCFG_SECT_LS1, MEMCFG_CLA_MEM_DATA);
    MemCfg_setCLAMemType(MEMCFG_SECT_LS2, MEMCFG_CLA_MEM_PROGRAM);
    MemCfg_setCLAMemType(MEMCFG_SECT_LS3, MEMCFG_CLA_MEM_PROGRAM);
    MemCfg_setCLAMemType(MEMCFG_SECT_LS4, MEMCFG_CLA_MEM_PROGRAM);
    MemCfg_setCLAMemType(MEMCFG_SECT_LS5, MEMCFG_CLA_MEM_PROGRAM);

    //
    // Suppressing #770-D conversion from pointer to smaller integer
    // The CLA address range is 16 bits so the addresses passed to the MVECT
    // registers will be in the lower 64KW address space. Turn the warning
    // back on after the MVECTs are assigned addresses
    //
    #pragma diag_suppress = 770

    CLA_mapTaskVector(CLA1_BASE , CLA_MVECT_1, (uint16_t)&Cla1Task1);
    CLA_mapTaskVector(CLA1_BASE , CLA_MVECT_2, (uint16_t)&Cla1Task2);
    CLA_mapTaskVector(CLA1_BASE , CLA_MVECT_3, (uint16_t)&Cla1Task3);
    CLA_mapTaskVector(CLA1_BASE , CLA_MVECT_4, (uint16_t)&Cla1Task4);
    CLA_mapTaskVector(CLA1_BASE , CLA_MVECT_5, (uint16_t)&Cla1Task5);
    CLA_mapTaskVector(CLA1_BASE , CLA_MVECT_6, (uint16_t)&Cla1Task6);
    CLA_mapTaskVector(CLA1_BASE , CLA_MVECT_7, (uint16_t)&Cla1Task7);
    CLA_mapBackgroundTaskVector(CLA1_BASE, (uint16_t)&Cla1BackgroundTask);

    #pragma diag_warning = 770

    CLA_enableIACK(CLA1_BASE);
    CLA_enableTasks(CLA1_BASE, CLA_TASKFLAG_ALL);

    CLA_enableHardwareTrigger(CLA1_BASE);
    CLA_setTriggerSource(CLA_TASK_8, CLLLC_ISR2_TRIG_CLA);
    CLA_enableBackgroundTask(CLA1_BASE);

    CLA_setTriggerSource(CLA_TASK_1, CLLLC_ISR1_TRIG_CLA);
#endif
}

void CLLLC_HAL_setupPWM(uint16_t powerFlowDir)
{

    uint16_t status;

    status = SFO_INCOMPLETE;
    //
    // Calling SFO() updates the HRMSTEP register with calibrated
    // MEP_ScaleFactor.
    // HRMSTEP must be populated with a scale factor value prior to enabling
    // high resolution period control.
    //
    while(status == SFO_INCOMPLETE)
    {
       status = SFO();
       if(status == SFO_ERROR)
       {
            //
            // SFO function returns 2 if an error occurs & # of MEP
            // steps/coarse step exceeds maximum of 255.
            //
            // Stop here and handle error
            //
              ESTOP0;
       }
    }

    if(powerFlowDir == CLLLC_POWER_FLOW_PRIM_SEC)
    {
        //
        //setup the prim PWM
        //
        CLLLC_HAL_setupHRPWMinUpDownCountModeWithDeadBand(
                                   CLLLC_PRIM_LEG1_PWM_BASE,
                                   CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ,
                                   CLLLC_PWMSYSCLOCK_FREQ_HZ,
                                   CLLLC_PRIM_PWM_DEADBAND_RED_NS,
                                   CLLLC_PRIM_PWM_DEADBAND_FED_NS);
        CLLLC_HAL_setupHRPWMinUpDownCountModeWithDeadBand(
                                   CLLLC_PRIM_LEG2_PWM_BASE,
                                   CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ,
                                   CLLLC_PWMSYSCLOCK_FREQ_HZ,
                                   CLLLC_PRIM_PWM_DEADBAND_RED_NS,
                                   CLLLC_PRIM_PWM_DEADBAND_FED_NS);

        //
        //setup the sec PWM
        //
        CLLLC_HAL_setupHRPWMinUpDownCount2ChAsymmetricMode(
                                  CLLLC_SEC_LEG1_PWM_BASE,
                                  CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ,
                                  CLLLC_PWMSYSCLOCK_FREQ_HZ);
        CLLLC_HAL_setupHRPWMinUpDownCount2ChAsymmetricMode(
                                  CLLLC_SEC_LEG2_PWM_BASE,
                                  CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ,
                                  CLLLC_PWMSYSCLOCK_FREQ_HZ);

        //
        //setup phase shift behavior
        //

        EPWM_disablePhaseShiftLoad(CLLLC_PRIM_LEG1_PWM_BASE);



        EPWM_enableSyncOutPulseSource(CLLLC_PRIM_LEG1_PWM_BASE, EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);
        EPWM_disableOneShotSync(CLLLC_PRIM_LEG1_PWM_BASE);
        EPWM_enableSyncOutPulseSource(CLLLC_PRIM_LEG1_PWM_BASE,
                                 EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);
        //
        // Sync action for PRIM LEG2
        //
        EPWM_disablePhaseShiftLoad(CLLLC_PRIM_LEG2_PWM_BASE);

        //
        // Sync action for SEC LEG1
        //
        EPWM_enablePhaseShiftLoad(CLLLC_SEC_LEG1_PWM_BASE);
        EPWM_setSyncInPulseSource(CLLLC_SEC_LEG1_PWM_BASE, EPWM_SYNC_IN_PULSE_SRC_SYNCOUT_EPWM1);
        EPWM_setPhaseShift(CLLLC_SEC_LEG1_PWM_BASE, 2);
        EPWM_setCountModeAfterSync(CLLLC_SEC_LEG1_PWM_BASE,
                              EPWM_COUNT_MODE_UP_AFTER_SYNC);

        //
        // Sync action for SEC LEG2
        //
        EPWM_enablePhaseShiftLoad(CLLLC_SEC_LEG2_PWM_BASE);
        EPWM_setSyncInPulseSource(CLLLC_SEC_LEG2_PWM_BASE, EPWM_SYNC_IN_PULSE_SRC_SYNCOUT_EPWM1);
        EPWM_setPhaseShift(CLLLC_SEC_LEG2_PWM_BASE, 2);
        EPWM_setCountModeAfterSync(CLLLC_SEC_LEG2_PWM_BASE,
                              EPWM_COUNT_MODE_UP_AFTER_SYNC);

        //
        // used by blanking logic
        //
        EPWM_enableSyncOutPulseSource(CLLLC_SEC_LEG2_PWM_BASE,
                                      EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);


        //
        // Add workaround for when TBPHS skips over CMPA
        //
        EPWM_setActionQualifierAction(CLLLC_PRIM_LEG2_PWM_BASE, EPWM_AQ_OUTPUT_A ,
               EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
        EPWM_setActionQualifierAction(CLLLC_PRIM_LEG2_PWM_BASE, EPWM_AQ_OUTPUT_A ,
               EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);

        //
        // Enable swap outputs using the feature in deadband module
        // for PRIM LEG2 and SEC LEG1
        //
        HWREGH(CLLLC_PRIM_LEG2_PWM_BASE + EPWM_O_DBCTL) =
                (HWREGH(CLLLC_PRIM_LEG2_PWM_BASE + EPWM_O_DBCTL) | 0x3000);

        HWREGH(CLLLC_SEC_LEG1_PWM_BASE + EPWM_O_DBCTL) =
                (HWREGH(CLLLC_SEC_LEG1_PWM_BASE + EPWM_O_DBCTL) | 0x3000);


    }
    else
    {
        //
        // setup the prim PWM
        //
        CLLLC_HAL_setupHRPWMinUpDownCount2ChAsymmetricMode(
                                   CLLLC_PRIM_LEG1_PWM_BASE,
                                   CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ,
                                   CLLLC_PWMSYSCLOCK_FREQ_HZ);
        CLLLC_HAL_setupHRPWMinUpDownCount2ChAsymmetricMode(
                                   CLLLC_PRIM_LEG2_PWM_BASE,
                                   CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ,
                                   CLLLC_PWMSYSCLOCK_FREQ_HZ);


        //
        // setup the sec PWM
        //
        CLLLC_HAL_setupHRPWMinUpDownCountModeWithDeadBand(
                                   CLLLC_SEC_LEG1_PWM_BASE,
                                   CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ,
                                   CLLLC_PWMSYSCLOCK_FREQ_HZ,
                                   CLLLC_PRIM_PWM_DEADBAND_RED_NS,
                                   CLLLC_PRIM_PWM_DEADBAND_FED_NS);
        CLLLC_HAL_setupHRPWMinUpDownCountModeWithDeadBand(
                                   CLLLC_SEC_LEG2_PWM_BASE,
                                   CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ,
                                   CLLLC_PWMSYSCLOCK_FREQ_HZ,
                                   CLLLC_PRIM_PWM_DEADBAND_RED_NS,
                                   CLLLC_PRIM_PWM_DEADBAND_FED_NS);

        //
        // setup phase shift behavior
        //
        EPWM_disablePhaseShiftLoad(CLLLC_PRIM_LEG1_PWM_BASE);




        EPWM_disablePhaseShiftLoad(CLLLC_PRIM_LEG2_PWM_BASE);

        EPWM_enableSyncOutPulseSource(CLLLC_PRIM_LEG1_PWM_BASE,
                                      EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);

        EPWM_enablePhaseShiftLoad(CLLLC_SEC_LEG1_PWM_BASE);

        //
        // This basically is also SYNCIN pass, need to correct driver lib
        // comments
        //

        EPWM_setPhaseShift(CLLLC_SEC_LEG1_PWM_BASE, 2);
        EPWM_setCountModeAfterSync(CLLLC_SEC_LEG1_PWM_BASE,
                              EPWM_COUNT_MODE_UP_AFTER_SYNC);

        //
        // by default EPWM1 syncout is passed to EPWM4
        //

        EPWM_enablePhaseShiftLoad(CLLLC_SEC_LEG2_PWM_BASE);

        EPWM_setSyncInPulseSource(CLLLC_SEC_LEG1_PWM_BASE,
                                  EPWM_SYNC_IN_PULSE_SRC_SYNCOUT_EPWM1);

        EPWM_enableSyncOutPulseSource(CLLLC_PRIM_LEG2_PWM_BASE,
                                      EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);

        EPWM_setPhaseShift(CLLLC_SEC_LEG2_PWM_BASE, 2 );

        EPWM_setCountModeAfterSync(CLLLC_SEC_LEG2_PWM_BASE,
                              EPWM_COUNT_MODE_UP_AFTER_SYNC);




        //
        // Enable swap outputs using the feature in deadband module
        //
        HWREGH(CLLLC_PRIM_LEG2_PWM_BASE + EPWM_O_DBCTL) =
              (HWREGH(CLLLC_PRIM_LEG2_PWM_BASE + EPWM_O_DBCTL) | 0x3000);


        HWREGH(CLLLC_SEC_LEG2_PWM_BASE + EPWM_O_DBCTL) =
              (HWREGH(CLLLC_SEC_LEG1_PWM_BASE + EPWM_O_DBCTL) | 0x3000);

    }

    //
    // Now setup the PWM link for TBPRD between
    // PRIM_LEG1, PRIM LEG2, SEC LEG1/2
    //

    EPWM_setupEPWMLinks(CLLLC_PRIM_LEG2_PWM_BASE,
                      EPWM_LINK_WITH_EPWM_1,
                      EPWM_LINK_TBPRD);

    EPWM_setupEPWMLinks(CLLLC_SEC_LEG1_PWM_BASE,
                      EPWM_LINK_WITH_EPWM_1,
                      EPWM_LINK_TBPRD);

    EPWM_setupEPWMLinks(CLLLC_SEC_LEG2_PWM_BASE,
                      EPWM_LINK_WITH_EPWM_1,
                      EPWM_LINK_TBPRD);

    //
    // Now setup the PWM link for CMPA between PRIM LEG1 & 2
    //

    EPWM_setupEPWMLinks(CLLLC_PRIM_LEG2_PWM_BASE,
                      EPWM_LINK_WITH_EPWM_1,
                      EPWM_LINK_COMP_A);

    //
    // Now setup the PWM link for CMPB between PRIM LEG1 & 2
    //

    EPWM_setupEPWMLinks(CLLLC_PRIM_LEG2_PWM_BASE,
                      EPWM_LINK_WITH_EPWM_1,
                      EPWM_LINK_COMP_B);

    //
    // Now setup the PWM link for CMPA between SEC LEG1 & 2
    //

    EPWM_setupEPWMLinks(CLLLC_SEC_LEG2_PWM_BASE,
                      EPWM_LINK_WITH_EPWM_3,
                      EPWM_LINK_COMP_A);

    //
    // Now setup the PWM link for CMPB between SEC LEG1 & 2
    //
    EPWM_setupEPWMLinks(CLLLC_SEC_LEG2_PWM_BASE,
                      EPWM_LINK_WITH_EPWM_3,
                      EPWM_LINK_COMP_B);

    //
    // Now setup the PWM link for GLDCTL2 between PRIM LEG1/2 & SEC LEG1 & 2
    //
#if CLLLC_GLOBAL_LOAD_ENABLED == 1
    EPWM_setupEPWMLinks(CLLLC_PRIM_LEG2_PWM_BASE,
                      EPWM_LINK_WITH_EPWM_1,
                      EPWM_LINK_GLDCTL2);

    EPWM_setupEPWMLinks(CLLLC_SEC_LEG1_PWM_BASE,
                     EPWM_LINK_WITH_EPWM_1,
                     EPWM_LINK_GLDCTL2);

    EPWM_setupEPWMLinks(CLLLC_SEC_LEG2_PWM_BASE,
                     EPWM_LINK_WITH_EPWM_1,
                     EPWM_LINK_GLDCTL2);
#endif

    //
    // Issue a one time latch from PRIM_LEG1
    //

    EPWM_setGlobalLoadOneShotLatch(CLLLC_PRIM_LEG1_PWM_BASE);
}

void CLLLC_HAL_setupCMPSSHighLimit(uint32_t base1,
                                 float32_t setLimit,
                                 float32_t maxSense,
                                 uint16_t hysteresis,
                                 uint16_t filterClkPrescalar,
                                 uint16_t filterSampleWindow,
                                 uint16_t filterThreshold)
{
    //
    //Enable CMPSS
    //
    CMPSS_enableModule(base1);

    //
    //Use VDDA as the reference for comparator DACs
    //
    CMPSS_configDAC(base1,
                 CMPSS_DACVAL_SYSCLK | CMPSS_DACREF_VDDA | CMPSS_DACSRC_SHDW);

    //
    //Set DAC to H~75% and L ~25% values
    //
    CMPSS_setDACValueHigh(base1,
                          (int16_t)((float32_t)setLimit * (float32_t)4096.0 /
                                  (float32_t)maxSense));

    //
    // Make sure the asynchronous path compare high and low event
    // does not go to the OR gate with latched digital filter output
    // hence no additional parameter CMPSS_OR_ASYNC_OUT_W_FILT  is passed
    // comparator oputput is "not" inverted for high compare event
    //
    CMPSS_configHighComparator(base1, CMPSS_INSRC_DAC );

    //
    // The following sets the digital filter for the trip flag for high and low
    // the below configuration will buffer last 10 samples and flag a trip if
    // 7 of them are high
    //
    CMPSS_configFilterHigh(base1,
                           filterClkPrescalar,
                           filterSampleWindow,
                           filterThreshold);
    CMPSS_configFilterLow(base1,
                          filterClkPrescalar,
                          filterSampleWindow,
                          filterThreshold);

    //
    // Reset filter logic & start filtering
    //
    CMPSS_initFilterHigh(base1);

    //
    // Configure CTRIPOUT path
    //
    CMPSS_configOutputsHigh(base1, CMPSS_TRIP_FILTER | CMPSS_TRIP_FILTER);

    //
    // Comparator hysteresis control , set to 2x typical value
    //
    CMPSS_setHysteresis(base1, hysteresis);

    //
    // Clear the latched comparator events
    //
    CMPSS_clearFilterLatchHigh(base1);
}

void CLLLC_HAL_setupCMPSSHighLowLimit(uint32_t base1,
                                 float32_t currentLimit,
                                 float32_t currentMaxSense,
                                 uint16_t hysteresis,
                                 uint16_t filterClkPrescalar,
                                 uint16_t filterSampleWindow,
                                 uint16_t filterThreshold)
{
    //
    //Enable CMPSS1
    //
    CMPSS_enableModule(base1);

    //
    //Use VDDA as the reference for comparator DACs
    //
    CMPSS_configDAC(base1,
                 CMPSS_DACVAL_SYSCLK | CMPSS_DACREF_VDDA | CMPSS_DACSRC_SHDW);

    //
    //Set DAC to H~75% and L ~25% values
    //
    CMPSS_setDACValueHigh(base1,
                          2048 +
                          (int16_t)((float)currentLimit * (float)2048.0 /
                                  (float)currentMaxSense));
    CMPSS_setDACValueLow(base1,
                         2048 -
                         (int16_t)((float)currentLimit * (float)2048.0 /
                                 (float)currentMaxSense));

    //
    // Make sure the asynchronous path compare high and low event
    // does not go to the OR gate with latched digital filter output
    // hence no additional parameter CMPSS_OR_ASYNC_OUT_W_FILT  is passed
    // comparator oputput is "not" inverted for high compare event
    //
    CMPSS_configHighComparator(base1, CMPSS_INSRC_DAC );
    //
    // Comparator output is inverted for for low compare event
    //
    CMPSS_configLowComparator(base1, CMPSS_INSRC_DAC | CMPSS_INV_INVERTED);

    //
    // The following sets the digital filter for the trip flag for high and low
    // the below configuration will buffer last 10 samples and flag a trip if
    // 7 of them are high
    //
    CMPSS_configFilterHigh(base1,
                           filterClkPrescalar,
                           filterSampleWindow,
                           filterThreshold);
    CMPSS_configFilterLow(base1,
                          filterClkPrescalar,
                          filterSampleWindow,
                          filterThreshold);

    //
    //Reset filter logic & start filtering
    //
    CMPSS_initFilterHigh(base1);
    CMPSS_initFilterLow(base1);

    //
    // Configure CTRIPOUT path
    //
    CMPSS_configOutputsHigh(base1, CMPSS_TRIP_FILTER | CMPSS_TRIP_FILTER);
    CMPSS_configOutputsLow(base1, CMPSS_TRIP_FILTER | CMPSS_TRIP_FILTER);

    //
    //Comparator hysteresis control , set to 2x typical value
    //
    CMPSS_setHysteresis(base1, hysteresis);

    //
    // Clear the latched comparator events
    //
    CMPSS_clearFilterLatchHigh(base1);
    CMPSS_clearFilterLatchLow(base1);
}

void CLLLC_HAL_disablePWMClkCounting(void)
{
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
}

void CLLLC_HAL_enablePWMClkCounting(void)
{
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
}

void CLLLC_HAL_setupPWMinUpDownCountMode(uint32_t base1,
                            float32_t pwmFreq_Hz,
                            float32_t pwmSysClkFreq_Hz)
{

    uint32_t pwmPeriod_ticks;

    pwmPeriod_ticks = TICKS_IN_PWM_FREQUENCY(pwmFreq_Hz, pwmSysClkFreq_Hz);

    //
    // Time Base SubModule Registers
    //
    EPWM_setPeriodLoadMode(base1, EPWM_PERIOD_DIRECT_LOAD);
    EPWM_setTimeBasePeriod(base1, pwmPeriod_ticks >> 1);
    EPWM_setTimeBaseCounter(base1, 0);
    EPWM_setPhaseShift(base1, 0);
    EPWM_setTimeBaseCounterMode(base1, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_setClockPrescaler(base1, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);

    //
    // configure PWM 1 as master and Phase 2 as slaves and
    // let it pass the sync in pulse from PWM1
    //
    EPWM_disablePhaseShiftLoad(base1);


    EPWM_enableSyncOutPulseSource(base1, EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);

    //
    // Counter Compare Submodule Registers
    // set duty 0% initially
    //
    EPWM_setCounterCompareValue(base1, EPWM_COUNTER_COMPARE_A, 0);
    EPWM_setCounterCompareShadowLoadMode(base1, EPWM_COUNTER_COMPARE_A,
                                         EPWM_COMP_LOAD_ON_CNTR_ZERO);

    //
    // Action Qualifier SubModule Registers
    //
    HWREGH(base1 + EPWM_O_AQCTLA) = 0 ;
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);

}


void CLLLC_HAL_setupECAPinPWMMode(uint32_t base1,
                            float32_t pwmFreq_Hz,
                            float32_t pwmSysClkFreq_Hz)
{
    int32_t ecap_ticks = (int32_t) ((float32_t)(pwmSysClkFreq_Hz / pwmFreq_Hz));

    ECAP_enableAPWMMode(base1);
    ECAP_setAPWMPeriod(base1, ecap_ticks);
    ECAP_setAPWMCompare(base1, ecap_ticks >> 1);

    ECAP_setAPWMShadowPeriod(base1, ecap_ticks);
    ECAP_setAPWMShadowCompare(base1, ecap_ticks >> 1);

    ECAP_clearInterrupt(base1, 0xFF);
    ECAP_clearGlobalInterrupt(base1);

    ECAP_startCounter(base1);

}


void CLLLC_HAL_setupHRPWMinUpDownCountModeWithDeadBand(uint32_t base1,
                                float32_t pwmFreq_Hz,
                                float32_t pwmSysClkFreq_Hz,
                                float32_t red_ns,
                                float32_t fed_ns)
{
    uint32_t pwmPeriod_ticks;
    uint32_t dbFED_ticks, dbRED_ticks;

    pwmPeriod_ticks = (uint32_t)((pwmSysClkFreq_Hz *
                                (float32_t)TWO_RAISED_TO_THE_POWER_SIXTEEN) /
                                (float32_t)pwmFreq_Hz) >> 1;
    pwmPeriod_ticks = (pwmPeriod_ticks & 0xFFFFFF00);

    dbRED_ticks = ((uint32_t)(red_ns *
                             (float32_t)TWO_RAISED_TO_THE_POWER_SIXTEEN *
                           ((float32_t)ONE_NANO_SEC) * pwmSysClkFreq_Hz * 2.0f));
    dbRED_ticks = ( dbRED_ticks & 0xFFFFFE00);

    dbFED_ticks = ((uint32_t)(red_ns *
                             (float32_t)TWO_RAISED_TO_THE_POWER_SIXTEEN *
                           ((float32_t)ONE_NANO_SEC) * pwmSysClkFreq_Hz * 2.0f));
    dbFED_ticks = ( dbFED_ticks & 0xFFFFFE00);

    //
    // Time Base SubModule Registers
    //
    EPWM_setPeriodLoadMode(base1, EPWM_PERIOD_SHADOW_LOAD);
    HWREG(base1 + HRPWM_O_TBPRDHR) = pwmPeriod_ticks;

    EPWM_setTimeBaseCounter(base1, 0);
    EPWM_setPhaseShift(base1, 0);
    EPWM_setTimeBaseCounterMode(base1, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_setClockPrescaler(base1, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);

    //
    // Counter Compare Submodule Registers
    // set duty 50% initially
    //
    HWREG(base1 + HRPWM_O_CMPA) = pwmPeriod_ticks >> 1;

    //
    // set as shadow mode
    //
    EPWM_setCounterCompareShadowLoadMode(base1, EPWM_COUNTER_COMPARE_A,
                                    EPWM_COMP_LOAD_ON_CNTR_ZERO_PERIOD);

    //
    // set as shadow mode
    //
    EPWM_setCounterCompareShadowLoadMode(base1, EPWM_COUNTER_COMPARE_B,
                                    EPWM_COMP_LOAD_ON_CNTR_ZERO_PERIOD);

    HWREG(base1 + HRPWM_O_CMPB) = pwmPeriod_ticks >> 1;

    EPWM_disableCounterCompareShadowLoadMode(base1, EPWM_COUNTER_COMPARE_C);

    EALLOW;
    //
    // Clear AQCTLA, B and Deadband settings settings
    //
    HWREGH(base1 + EPWM_O_AQCTLA) = 0x0000;
    HWREGH(base1 + EPWM_O_AQCTLB) = 0x0000;

    HWREGH(base1 + EPWM_O_DCBCTL) = 0x0000;
    EDIS;

    //
    // Action Qualifier SubModule Registers
    // CTR = CMPA@UP , xA set to 1
    //
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_A ,
           EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);

    //
    // CTR = CMPA@Down , xA set to 0
    //
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_A ,
           EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);

    //
    // Active high complementary PWMs - Set up the deadband
    //

    EPWM_setRisingEdgeDelayCountShadowLoadMode(base1,
                                              EPWM_RED_LOAD_ON_CNTR_ZERO);
    EPWM_setFallingEdgeDelayCountShadowLoadMode(base1,
                                              EPWM_FED_LOAD_ON_CNTR_ZERO);
    EPWM_setDeadBandCounterClock(base1, EPWM_DB_COUNTER_CLOCK_HALF_CYCLE);

    EPWM_setDeadBandDelayMode(base1, EPWM_DB_RED, true);
    EPWM_setDeadBandDelayMode(base1, EPWM_DB_FED, true);
    EPWM_setRisingEdgeDeadBandDelayInput(base1, EPWM_DB_INPUT_EPWMA);
    EPWM_setFallingEdgeDeadBandDelayInput(base1, EPWM_DB_INPUT_EPWMA);
    EPWM_setDeadBandDelayPolarity(base1, EPWM_DB_FED,
                                 EPWM_DB_POLARITY_ACTIVE_LOW);
    EPWM_setDeadBandDelayPolarity(base1, EPWM_DB_RED,
                                 EPWM_DB_POLARITY_ACTIVE_HIGH);

    HWREG(base1 + HRPWM_O_DBFEDHR) = dbFED_ticks;
    HWREG(base1 + HRPWM_O_DBREDHR) = dbRED_ticks;

    //
    // Hi-res PWM
    // MEP control on both edges.
    //
    HRPWM_setMEPEdgeSelect(base1, HRPWM_CHANNEL_A,
                           HRPWM_MEP_CTRL_RISING_AND_FALLING_EDGE);
    HRPWM_setCounterCompareShadowLoadEvent(base1, HRPWM_CHANNEL_A,
                                           HRPWM_LOAD_ON_CNTR_ZERO_PERIOD);
    HRPWM_setMEPEdgeSelect(base1, HRPWM_CHANNEL_B,
                           HRPWM_MEP_CTRL_RISING_AND_FALLING_EDGE);
    HRPWM_setCounterCompareShadowLoadEvent(base1, HRPWM_CHANNEL_B,
                                           HRPWM_LOAD_ON_CNTR_ZERO_PERIOD);

    HRPWM_setMEPControlMode(base1, HRPWM_CHANNEL_A, HRPWM_MEP_DUTY_PERIOD_CTRL);
    HRPWM_setMEPControlMode(base1, HRPWM_CHANNEL_B, HRPWM_MEP_DUTY_PERIOD_CTRL);

    HRPWM_setDeadbandMEPEdgeSelect(base1, HRPWM_DB_MEP_CTRL_RED_FED);

    //
    // Enable autoconversion
    //
    HRPWM_enableAutoConversion(base1);

    //
    // Turn on high-resolution period control.
    //
    HRPWM_enablePeriodControl(base1);


}

void CLLLC_HAL_setupHRPWMinUpDownCount2ChAsymmetricMode(uint32_t base1,
                            float32_t pwmFreq_Hz,
                            float32_t pwmSysClkFreq_Hz)
{
    uint32_t pwmPeriod_ticks;

    pwmPeriod_ticks = TICKS_IN_PWM_FREQUENCY(pwmFreq_Hz, pwmSysClkFreq_Hz);

    //
    // Time Base SubModule Registers
    //
    EPWM_setPeriodLoadMode(base1, EPWM_PERIOD_SHADOW_LOAD);
    EPWM_setTimeBasePeriod(base1, pwmPeriod_ticks >> 1);
    EPWM_setTimeBaseCounter(base1, 0);
    EPWM_setPhaseShift(base1, 0);
    EPWM_setTimeBaseCounterMode(base1, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_setClockPrescaler(base1, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);

    //
    // Counter Compare Submodule Registers
    // set duty 50% initially
    //
    EPWM_setCounterCompareValue(base1, EPWM_COUNTER_COMPARE_A,
                                pwmPeriod_ticks >> 2);
    //
    // set as shadow mode
    //
    EPWM_setCounterCompareShadowLoadMode(base1, EPWM_COUNTER_COMPARE_A,
                                EPWM_COMP_LOAD_ON_CNTR_ZERO_PERIOD);


    EPWM_setCounterCompareValue(base1, EPWM_COUNTER_COMPARE_B,
                                   pwmPeriod_ticks >> 2);

    EALLOW;
    //
    // Clear AQCTLA, B and Deadband settings settings
    //
    HWREGH(base1 + EPWM_O_AQCTLA) = 0x0000;
    HWREGH(base1 + EPWM_O_AQCTLB) = 0x0000;

    HWREGH(base1 + EPWM_O_DCBCTL) = 0x0000;
    EDIS;

    //
    // Action Qualifier SubModule Registers
#if CLLLC_TRANSFORMER_POLARITY == CLLLC_POSITIVE_POLARITY
    //The following code should be used when the transformer polarity matches the schematic.
    // CTR = CMPA@0 , xA set to 1
    //
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_A ,
       EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO );
    //
    // CTR = CMPA@Up , xA set to 0
    //
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_A ,
       EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);

    //
    // CTR = CMPB@PERIOD, xB set to 1
    //
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_B ,
       EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);
    //
    // CTR = CMPB@DOWN , xB set to 0
    //
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_B ,
       EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);
#else
    //The following code should be used when the transformer polarity does not match the schematic.
    // CTR = CMPA@period , xA set to 1
    //
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_A ,
       EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD );
    //
    // CTR = CMPA@Down , xA set to 0
    //
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_A ,
       EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);

    //
    // CTR = CMPB@0, xB set to 1
    //
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_B ,
       EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    //
    // CTR = CMPB@UP , xB set to 0
    //
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_B ,
       EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
#endif

   //
   // Hi-res PWM
   // MEP control on both edges.
   //
   HRPWM_setMEPEdgeSelect(base1, HRPWM_CHANNEL_A,
                          HRPWM_MEP_CTRL_RISING_AND_FALLING_EDGE);
   HRPWM_setCounterCompareShadowLoadEvent(base1, HRPWM_CHANNEL_A,
                                          HRPWM_LOAD_ON_CNTR_ZERO_PERIOD);
   HRPWM_setMEPEdgeSelect(base1, HRPWM_CHANNEL_B,
                          HRPWM_MEP_CTRL_RISING_AND_FALLING_EDGE);
   HRPWM_setCounterCompareShadowLoadEvent(base1, HRPWM_CHANNEL_B,
                                          HRPWM_LOAD_ON_CNTR_ZERO_PERIOD);

   HRPWM_setMEPControlMode(base1, HRPWM_CHANNEL_A, HRPWM_MEP_DUTY_PERIOD_CTRL);
   HRPWM_setMEPControlMode(base1, HRPWM_CHANNEL_B, HRPWM_MEP_DUTY_PERIOD_CTRL);

   //
   // Enable autoconversion
   //
   HRPWM_enableAutoConversion(base1);

   //
   // Enable TBPHSHR sync (required for updwn count HR control)
   //
   HRPWM_enablePhaseShiftLoad(base1);

   //
   // Turn on high-resolution period control.
   //
   HRPWM_enablePeriodControl(base1);

}

void CLLLC_HAL_setupECAPforGaNTemp()
{
    CLLLC_HAL_setupECAPforDutyCapture(CLLLC_TEMP_MUX_OUT1_ECAP_BASE);
    CLLLC_HAL_setupECAPforDutyCapture(CLLLC_TEMP_MUX_OUT2_ECAP_BASE);

    ECAP_selectECAPInput(CLLLC_TEMP_MUX_OUT1_ECAP_BASE, CLLLC_TEMP_MUX_OUT1_ECAP_INPUT);
    XBAR_setInputPin(INPUTXBAR_BASE, CLLLC_TEMP_MUX_OUT1_INPUTXBAR, CLLLC_GPIO_TEMP_MUX_OUT1);

    ECAP_selectECAPInput(CLLLC_TEMP_MUX_OUT2_ECAP_BASE, CLLLC_TEMP_MUX_OUT2_ECAP_INPUT);
    XBAR_setInputPin(INPUTXBAR_BASE, CLLLC_TEMP_MUX_OUT2_INPUTXBAR, CLLLC_GPIO_TEMP_MUX_OUT2);
}

//
//
//
void CLLLC_HAL_setupECAPforDutyCapture(uint32_t base)
{
    ECAP_enableCaptureMode(base);
    ECAP_setCaptureMode(base, ECAP_ONE_SHOT_CAPTURE_MODE, ECAP_EVENT_3);

    ECAP_enableCounterResetOnEvent(base, ECAP_EVENT_3);

    ECAP_enableTimeStampCapture(base);

    ECAP_setEventPolarity(base, ECAP_EVENT_1, ECAP_EVNT_RISING_EDGE);
    ECAP_setEventPolarity(base, ECAP_EVENT_2, ECAP_EVNT_FALLING_EDGE);
    ECAP_setEventPolarity(base, ECAP_EVENT_3, ECAP_EVNT_RISING_EDGE);

    ECAP_startCounter(base);
}


//
// Setup DAC - Setup the reference voltage and output value for the DAC
//
void CLLLC_HAL_setupDAC(void)
{
    //
    // Set VDAC as the DAC reference voltage.
    // Edit here to use ADC VREF as the reference voltage.
    //
    DAC_setReferenceVoltage(CLLLC_DAC_BASE,  DAC_REF_ADC_VREFHI);

    //
    // Enable the DAC output
    //
    DAC_enableOutput(CLLLC_DAC_BASE);

    //
    // Set the DAC shadow output to 0
    //
    DAC_setShadowValue(CLLLC_DAC_BASE, 0);

    //
    // Delay for buffered DAC to power up
    //
    DEVICE_DELAY_US(10);
}
