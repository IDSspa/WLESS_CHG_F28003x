//#############################################################################
//
// FILE:   ttplpfc_hal.c
//
// TITLE: solution hardware abstraction layer header file
//        This file consists of common variables and functions
//        for a particular hardware board, like functions to read current
//        and voltage signals on the board and functions to setup the
//        basic peripherals of the device
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

#include <ttplpfc_hal.h>


TTPLPFC_HAL_GaNTemp_Struct TTPLPFC_HAL_GaNTemp_C;

//
// This routine sets up the basic device configuration such as initializing PLL
// copying code from FLASH to RAM
//
void TTPLPFC_HAL_setupDevice(void)
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
    //

    //
    // Initialize timer period to maximum
    //
    CPUTimer_setPeriod(CPUTIMER0_BASE, DEVICE_SYSCLK_FREQ / TTPLPFC_TASKA_FREQ_HZ );

    //
    // Initialize timer period to maximum
    //
    CPUTimer_setPeriod(CPUTIMER1_BASE, DEVICE_SYSCLK_FREQ / TTPLPFC_TASKB_FREQ_HZ );

    //
    // Initialize timer period to maximum
    //
    CPUTimer_setPeriod(CPUTIMER2_BASE, DEVICE_SYSCLK_FREQ / 10000 );

    //
    // Initialize pre-scale counter to divide by 1 (SYSCLKOUT)
    //
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0);

    //
    // Initialize pre-scale counter to divide by 1 (SYSCLKOUT)
    //
    CPUTimer_setPreScaler(CPUTIMER1_BASE, 0);

    //
    // Initialize pre-scale counter to divide by 1 (SYSCLKOUT)
    //
    CPUTimer_setPreScaler(CPUTIMER2_BASE, 0);

    //
    // Make sure timer is stopped
    //
    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_stopTimer(CPUTIMER1_BASE);
    CPUTimer_stopTimer(CPUTIMER2_BASE);
    CPUTimer_setEmulationMode(CPUTIMER0_BASE,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    CPUTimer_setEmulationMode(CPUTIMER1_BASE,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    CPUTimer_setEmulationMode(CPUTIMER2_BASE,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    //
    // Reload all counter register with period value
    //
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER1_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER2_BASE);

    CPUTimer_resumeTimer(CPUTIMER0_BASE);
    CPUTimer_resumeTimer(CPUTIMER1_BASE);
    CPUTimer_resumeTimer(CPUTIMER2_BASE);

}

void TTPLPFC_HAL_setupADC(void)
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
    // setup ADC conversions for current and voltage signals
    //

    //
    //IL1
    //
    ADC_setupSOC(TTPLPFC_IL1_ADC_MODULE, TTPLPFC_IL1_ADC_SOC_NO,
                 TTPLPFC_IL1_ADC_TRIG_SOURCE,
                 TTPLPFC_IL1_ADC_PIN, TTPLPFC_IL1_ADC_ACQPS_SYS_CLKS);

    ADC_setupSOC(TTPLPFC_IL1_ADC_MODULE, TTPLPFC_IL1_ADC_SOC_NO_2,
                 TTPLPFC_IL1_ADC_TRIG_SOURCE_2,
                 TTPLPFC_IL1_ADC_PIN, TTPLPFC_IL1_ADC_ACQPS_SYS_CLKS);
    //
    //IL2
    //
    ADC_setupSOC(TTPLPFC_IL2_ADC_MODULE, TTPLPFC_IL2_ADC_SOC_NO,
                 TTPLPFC_IL2_ADC_TRIG_SOURCE,
                 TTPLPFC_IL2_ADC_PIN, TTPLPFC_IL2_ADC_ACQPS_SYS_CLKS);

    ADC_setupSOC(TTPLPFC_IL2_ADC_MODULE, TTPLPFC_IL2_ADC_SOC_NO_2,
                  TTPLPFC_IL2_ADC_TRIG_SOURCE_2,
                  TTPLPFC_IL2_ADC_PIN, TTPLPFC_IL2_ADC_ACQPS_SYS_CLKS);

    //
    // VAC
    //
    ADC_setupSOC(TTPLPFC_VAC_ADC_MODULE, TTPLPFC_VAC_ADC_SOC_NO,
                 TTPLPFC_VAC_ADC_TRIG_SOURCE,
                 TTPLPFC_VAC_ADC_PIN, TTPLPFC_VAC_ADC_ACQPS_SYS_CLKS);

    ADC_setupSOC(TTPLPFC_VAC_ADC_MODULE, TTPLPFC_VAC_ADC_SOC_NO_2,
                  TTPLPFC_VAC_ADC_TRIG_SOURCE_2,
                  TTPLPFC_VAC_ADC_PIN, TTPLPFC_VAC_ADC_ACQPS_SYS_CLKS);

    //
    // AC_N
    //
    ADC_setupSOC(TTPLPFC_AC_N_ADC_MODULE, TTPLPFC_AC_N_ADC_SOC_NO,
                 TTPLPFC_AC_N_ADC_TRIG_SOURCE,
                 TTPLPFC_AC_N_ADC_PIN, TTPLPFC_AC_N_ADC_ACQPS_SYS_CLKS);

    //
    // AC_L
    //
    ADC_setupSOC(TTPLPFC_AC_L_ADC_MODULE, TTPLPFC_AC_L_ADC_SOC_NO,
                 TTPLPFC_AC_L_ADC_TRIG_SOURCE,
                 TTPLPFC_AC_L_ADC_PIN, TTPLPFC_AC_L_ADC_ACQPS_SYS_CLKS);

    //
    // VBUS
    //
    ADC_setupSOC(TTPLPFC_VBUS_ADC_MODULE, TTPLPFC_VBUS_ADC_SOC_NO,
                 TTPLPFC_VBUS_ADC_TRIG_SOURCE,
                 TTPLPFC_VBUS_ADC_PIN, TTPLPFC_VBUS_ADC_ACQPS_SYS_CLKS);

    ADC_setupSOC(TTPLPFC_VBUS_ADC_MODULE, TTPLPFC_VBUS_ADC_SOC_NO_2,
                 TTPLPFC_VBUS_ADC_TRIG_SOURCE_2,
                 TTPLPFC_VBUS_ADC_PIN, TTPLPFC_VBUS_ADC_ACQPS_SYS_CLKS);

    //
    // SYSTEM_TEMP
    //
    ADC_setupSOC(TTPLPFC_SYSTEM_TEMP_ADC_MODULE, TTPLPFC_SYSTEM_TEMP_ADC_SOC_NO,
                TTPLPFC_SYSTEM_TEMP_ADC_TRIG_SOURCE,
                TTPLPFC_SYSTEM_TEMP_ADC_PIN, TTPLPFC_SYSTEM_TEMP_ADC_ACQPS_SYS_CLKS);

    //
    // SYSTEM_VREF_165
    //
    ADC_setupSOC(TTPLPFC_SYSTEM_VREF_165_ADC_MODULE, TTPLPFC_SYSTEM_VREF_165_ADC_SOC_NO,
                TTPLPFC_SYSTEM_VREF_165_ADC_TRIG_SOURCE,
                TTPLPFC_SYSTEM_VREF_165_ADC_PIN, TTPLPFC_SYSTEM_VREF_165_ADC_ACQPS_SYS_CLKS);

    //
    // VBUS2
    //
    ADC_setupSOC(TTPLPFC_VBUS2_ADC_MODULE, TTPLPFC_VBUS2_ADC_SOC_NO,
                TTPLPFC_VBUS2_ADC_TRIG_SOURCE,
                TTPLPFC_VBUS2_ADC_PIN, TTPLPFC_VBUS2_ADC_ACQPS_SYS_CLKS);

}

void TTPLPFC_HALconfigurePWM1chUpCnt(uint32_t base1, uint16_t period)
{

    //
    // Time Base SubModule Registers
    //
    EPWM_setPeriodLoadMode(base1, EPWM_PERIOD_DIRECT_LOAD);
    EPWM_setTimeBasePeriod(base1, period - 1);
    EPWM_setTimeBaseCounter(base1, 0);
    EPWM_setPhaseShift(base1, 0);
    EPWM_setTimeBaseCounterMode(base1, EPWM_COUNTER_MODE_UP);
    EPWM_setClockPrescaler(base1, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);

    //
    // configure PWM 1 as master
    //
    EPWM_disablePhaseShiftLoad(base1);



    EPWM_enableSyncOutPulseSource(base1,
                                  EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);
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
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_A ,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(base1, EPWM_AQ_OUTPUT_A ,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);

}

void TTPLPFC_HAL_setupCMPSS(
        uint32_t base1, float32_t current_limit, float32_t current_max_sense )
{
    //
    //Enable CMPSS1
    //
    CMPSS_enableModule(base1);

    //
    //Use VDDA as the reference for comparator DACs
    //
    CMPSS_configDAC(base1,
                    CMPSS_DACVAL_SYSCLK | CMPSS_DACREF_VDDA |
                    CMPSS_DACSRC_SHDW);

    //
    //Set DAC to H~75% and L ~25% values
    //
    CMPSS_setDACValueHigh(base1,
                          2048 +
                          (int16_t)((float32_t)current_limit * (float32_t)2048.0 /
                                  (float32_t)current_max_sense));
    CMPSS_setDACValueLow(base1,
                         2048 -
                         (int16_t)((float32_t)current_limit * (float32_t)2048.0 /
                                 (float32_t)current_max_sense));

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

    CMPSS_configFilterHigh(base1, 2, 10, 7);
    CMPSS_configFilterLow(base1, 2, 10, 7);

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
    CMPSS_setHysteresis(base1, 2);

    //
    // Clear the latched comparator events
    //
    CMPSS_clearFilterLatchHigh(base1);
    CMPSS_clearFilterLatchLow(base1);
}

void TTPLPFC_HAL_setupPWMforTrip(uint32_t base)
{
#if TTPLPFC_IL1_PROTECTION_ENABLE == 1 || \
    TTPLPFC_IL2_PROTECTION_ENABLE == 1 || \
    TTPLPFC_IL3_PROTECTION_ENABLE == 1 || \
    TTPLPFC_FAULTn_PROTECTION_ENABLE == 1

    //
    // Select the TRIPIN input as DCAHCOMPSEL
    //
    EPWM_selectDigitalCompareTripInput(base,
                                       TTPLPFC_DC_TRIP_SELECT,
                                       EPWM_DC_TYPE_DCAH);
    EPWM_setTripZoneDigitalCompareEventCondition(base,
                                                 EPWM_TZ_DC_OUTPUT_A1,
                                                 EPWM_TZ_EVENT_DCXH_HIGH);
    EPWM_setDigitalCompareEventSource(base,
                                      EPWM_DC_MODULE_A,
                                      EPWM_DC_EVENT_1,
                                      EPWM_DC_EVENT_SOURCE_ORIG_SIGNAL);
    EPWM_setDigitalCompareEventSyncMode(base,
                                        EPWM_DC_MODULE_A ,
                                        EPWM_DC_EVENT_1,
                                        EPWM_DC_EVENT_INPUT_NOT_SYNCED);

    //
    // Enable the following trips Emulator Stop, TZ1-3 and DCAEVT1
    //
    EPWM_enableTripZoneSignals(base, EPWM_TZ_SIGNAL_DCAEVT1);

#else
    #warning All current comparator based protections are disabled
#endif

    //
    // Enable the following trips Emulator Stop
    //
    EPWM_enableTripZoneSignals(base, EPWM_TZ_SIGNAL_CBC6);

    //
    // What do we want the OST/CBC events to do?
    // TZA events can force EPWMxA
    // TZB events can force EPWMxB
    //
    EPWM_setTripZoneAction(base, EPWM_TZ_ACTION_EVENT_TZA, EPWM_TZ_ACTION_LOW);
    EPWM_setTripZoneAction(base, EPWM_TZ_ACTION_EVENT_TZB, EPWM_TZ_ACTION_LOW);

    //
    // Clear any spurious trip
    //
    EPWM_clearTripZoneFlag(base,
                           (EPWM_TZ_INTERRUPT_OST | EPWM_TZ_INTERRUPT_DCAEVT1));

    EPWM_forceTripZoneEvent(base, EPWM_TZ_FORCE_EVENT_OST);

}

void TTPLPFC_HAL_setupBoardProtection(float32_t current_limit,
                          float32_t current_max_sense)
{
    //
    // Disable all the muxes for the TRIP signal that is going to the PWM modules
    //
    XBAR_enableEPWMMux(TTPLPFC_PWMXBAR_TRIP_NO, 0x00);

    //
    // Now select the trip sources un the Muxes based on the selection
    //

#if TTPLPFC_IL1_PROTECTION_ENABLE == 1
    TTPLPFC_HAL_setupCMPSS(TTPLPFC_IL1_PROTECTION_CMPSS_BASE,
               current_limit, current_max_sense );
    XBAR_setEPWMMuxConfig(TTPLPFC_PWMXBAR_TRIP_NO,
                          TTPLPFC_IL1_PROTECTION_CMPSS_XBAR_MUX_VAL);

    ASysCtl_selectCMPHPMux(TTPLPFC_IL1_PROTECTION_CMPSS_ASYSCTRL_CMPHPMUX,
                           TTPLPFC_IL1_PROTECTION_CMPSS_ASYSCTRL_MUX_VALUE);

    ASysCtl_selectCMPLPMux(TTPLPFC_IL1_PROTECTION_CMPSS_ASYSCTRL_CMPLPMUX,
                           TTPLPFC_IL1_PROTECTION_CMPSS_ASYSCTRL_MUX_VALUE);

    XBAR_enableEPWMMux(TTPLPFC_PWMXBAR_TRIP_NO, TTPLPFC_IL1_PROTECTION_CMPSS_XBAR_MUX);
#else
#endif

#if TTPLPFC_IL2_PROTECTION_ENABLE == 1
    TTPLPFC_HAL_setupCMPSS(TTPLPFC_IL2_PROTECTION_CMPSS_BASE,
               current_limit, current_max_sense );
    XBAR_setEPWMMuxConfig(TTPLPFC_PWMXBAR_TRIP_NO,
                          TTPLPFC_IL2_PROTECTION_CMPSS_XBAR_MUX_VAL);

    ASysCtl_selectCMPHPMux(TTPLPFC_IL2_PROTECTION_CMPSS_ASYSCTRL_CMPHPMUX,
                           TTPLPFC_IL2_PROTECTION_CMPSS_ASYSCTRL_MUX_VALUE);

    ASysCtl_selectCMPLPMux(TTPLPFC_IL2_PROTECTION_CMPSS_ASYSCTRL_CMPLPMUX,
                           TTPLPFC_IL2_PROTECTION_CMPSS_ASYSCTRL_MUX_VALUE);

    XBAR_enableEPWMMux(TTPLPFC_PWMXBAR_TRIP_NO, TTPLPFC_IL2_PROTECTION_CMPSS_XBAR_MUX);
#else
#endif

#if TTPLPFC_FAULTn_PROTECTION_ENABLE == 1
    GPIO_setDirectionMode(TTPLPFC_FAULTn_GPIO, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(TTPLPFC_FAULTn_GPIO, GPIO_QUAL_6SAMPLE);
    GPIO_setPinConfig(TTPLPFC_FAULTn_PIN_CONFIG);
    GPIO_setPadConfig(TTPLPFC_FAULTn_GPIO, GPIO_PIN_TYPE_INVERT
                              | GPIO_PIN_TYPE_STD );

    XBAR_setInputPin(INPUTXBAR_BASE, TTPLPFC_FAULTn_XBAR_INPUT, TTPLPFC_FAULTn_GPIO);

    XBAR_setEPWMMuxConfig(TTPLPFC_PWMXBAR_TRIP_NO,
                          TTPLPFC_FAULTn_XBAR_MUX_VAL);
    XBAR_enableEPWMMux(TTPLPFC_PWMXBAR_TRIP_NO, TTPLPFC_FAULTn_XBAR_MUX);

#else
#endif

    XBAR_clearInputFlag(TTPLPFC_IL1_PROTECTION_XBAR_FLAG1);
    XBAR_clearInputFlag(TTPLPFC_IL2_PROTECTION_XBAR_FLAG1);
    XBAR_clearInputFlag(TTPLPFC_IL1_PROTECTION_XBAR_FLAG2);
    XBAR_clearInputFlag(TTPLPFC_IL2_PROTECTION_XBAR_FLAG2);
    XBAR_clearInputFlag(TTPLPFC_FAULTn_XBAR_FLAG);

#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
    TTPLPFC_HAL_setupPWMforTrip(TTPLPFC_LOW_FREQ_PWM_BASE);
#endif
    TTPLPFC_HAL_setupPWMforTrip(TTPLPFC_HIGH_FREQ_PWM1_BASE);
    TTPLPFC_HAL_setupPWMforTrip(TTPLPFC_HIGH_FREQ_PWM2_BASE);

}

void TTPLPFC_HAL_setupTrigForADC()
{

    //
    // SOC from TTPLPFC_HIGH_FREQ_PWM1_BASE
    //

    //
    // TRIG1 -> Select SOCA from counter at ctr = CMPCU
    //
    EPWM_setADCTriggerSource(TTPLPFC_HIGH_FREQ_PWM1_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_U_CMPC );
    EPWM_setCounterCompareValue(TTPLPFC_HIGH_FREQ_PWM1_BASE, EPWM_COUNTER_COMPARE_C,
                                (uint16_t)(((uint16_t)TTPLPFC_PWM_PERIOD >> 1) - ((uint16_t)TTPLPFC_ADC_ACQPS_SYS_CLOCKS >> 1)));
    EPWM_setADCTriggerEventPrescale(TTPLPFC_HIGH_FREQ_PWM1_BASE, EPWM_SOC_A, 1);
    EPWM_enableADCTrigger(TTPLPFC_HIGH_FREQ_PWM1_BASE, EPWM_SOC_A);

    //
    // TRIG2 -> Select SOCB from counter at ctr = CMPCD
    //
    EPWM_setADCTriggerSource(TTPLPFC_HIGH_FREQ_PWM1_BASE, EPWM_SOC_B, EPWM_SOC_TBCTR_D_CMPD );
    EPWM_setCounterCompareValue(TTPLPFC_HIGH_FREQ_PWM1_BASE , EPWM_COUNTER_COMPARE_D, (uint16_t)TTPLPFC_ADC_ACQPS_SYS_CLOCKS >> 1);
    EPWM_setADCTriggerEventPrescale(TTPLPFC_HIGH_FREQ_PWM1_BASE , EPWM_SOC_B, 1);
    EPWM_enableADCTrigger(TTPLPFC_HIGH_FREQ_PWM1_BASE, EPWM_SOC_B);

    //
    // SOC from TTPLPFC_HIGH_FREQ_PWM2_BASE
    //

    //
    // Select SOCA from counter at ctr = CMPCU
    //
    EPWM_setADCTriggerSource(TTPLPFC_HIGH_FREQ_PWM2_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_U_CMPC );
    EPWM_setCounterCompareValue(TTPLPFC_HIGH_FREQ_PWM2_BASE, EPWM_COUNTER_COMPARE_C,
                                (uint16_t)(((uint16_t)TTPLPFC_PWM_PERIOD >> 2) - (uint16_t)TTPLPFC_ADC_ACQPS_SYS_CLOCKS >> 1));
    EPWM_setADCTriggerEventPrescale(TTPLPFC_HIGH_FREQ_PWM2_BASE, EPWM_SOC_A, 1);
    EPWM_enableADCTrigger(TTPLPFC_HIGH_FREQ_PWM2_BASE, EPWM_SOC_A);

    //
    // Select SOCB from counter at ctr =CMPCD
    //
    EPWM_setADCTriggerSource(TTPLPFC_HIGH_FREQ_PWM2_BASE, EPWM_SOC_B, EPWM_SOC_TBCTR_D_CMPD );
    EPWM_setCounterCompareValue(TTPLPFC_HIGH_FREQ_PWM2_BASE, EPWM_COUNTER_COMPARE_D,
                                (uint16_t)(((uint16_t)TTPLPFC_PWM_PERIOD >> 2) + (uint16_t)TTPLPFC_ADC_ACQPS_SYS_CLOCKS >> 1));
    EPWM_setADCTriggerEventPrescale(TTPLPFC_HIGH_FREQ_PWM2_BASE, EPWM_SOC_B, 1);
    EPWM_enableADCTrigger(TTPLPFC_HIGH_FREQ_PWM2_BASE, EPWM_SOC_B);

    //
    // Setup CPU Timer initiated ADC Trig
    //
    CPUTimer_enableInterrupt(TTPLPFC_INSTRUMENTATION_ISR_TIME_BASE);
    CPUTimer_clearOverflowFlag(TTPLPFC_INSTRUMENTATION_ISR_TIME_BASE);
    ADC_setInterruptSource(TTPLPFC_INSTRUMENTATION_ISR_TRIG_BASE,
                          ADC_INT_NUMBER2, TTPLPFC_AC_L_ADC_SOC_NO);
    ADC_enableInterrupt(TTPLPFC_INSTRUMENTATION_ISR_TRIG_BASE, ADC_INT_NUMBER2);
    ADC_enableContinuousMode(TTPLPFC_INSTRUMENTATION_ISR_TRIG_BASE, ADC_INT_NUMBER2);
    ADC_clearInterruptStatus(TTPLPFC_INSTRUMENTATION_ISR_TRIG_BASE, ADC_INT_NUMBER2);

}

void TTPLPFC_calibrateOffset(volatile float32_t *ac_cur_sensedOffset,
                     volatile float32_t *IL1c_sensedOffset,
                     volatile float32_t *IL2c_sensedOffset,
                     volatile float32_t *IL3c_sensedOffset,
                     float32_t k1, float32_t k2)
{
    int16_t offsetCalCounter = 0;

    TTPLPFC_HAL_enablePWMInterruptGeneration(
            TTPLPFC_C28x_CONTROL_ISR_TRIG_BASE);

    offsetCalCounter = 0;
    while(offsetCalCounter < 20000)
    {
        if(EPWM_getEventTriggerInterruptStatus(
                TTPLPFC_C28x_CONTROL_ISR_TRIG_BASE) == 1)
        {
            if(offsetCalCounter > 1000)
            {
                //
                // offset of the inductor current sense
                //
//                *ac_cur_sensedOffset =  k1 * (*ac_cur_sensedOffset) +
//                                        k2 * (TTPLPFC_AC_CUR_FB) *
//                                        TTPLPFC_ADC_PU_SCALE_FACTOR;
                *IL1c_sensedOffset =   k1 * (*IL1c_sensedOffset) +
                                        k2 * (TTPLPFC_IL1_FB) *
                                        TTPLPFC_ADC_PU_SCALE_FACTOR;
                *IL2c_sensedOffset =   k1 * (*IL2c_sensedOffset) +
                                        k2 * (TTPLPFC_IL2_FB) *
                                        TTPLPFC_ADC_PU_SCALE_FACTOR;
//                *IL3c_sensedOffset =   k1 * (*IL3c_sensedOffset) +
//                                        k2 * (TTPLPFC_IL3_FB) *
//                                        TTPLPFC_ADC_PU_SCALE_FACTOR;
            }
            EPWM_clearEventTriggerInterruptFlag(
                    TTPLPFC_C28x_CONTROL_ISR_TRIG_BASE);
            offsetCalCounter++;
        }
    }

    ADC_setPPBCalibrationOffset(ADCA_BASE, ADC_PPB_NUMBER1, (0*4096.0));
    ADC_setPPBCalibrationOffset(ADCA_BASE, ADC_PPB_NUMBER2, (0*4096.0));
    ADC_setPPBCalibrationOffset(ADCA_BASE, ADC_PPB_NUMBER3, (0*4096.0));
}

void TTPLPFC_HAL_disablePWMCLKCounting(void)
{
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
}

void TTPLPFC_HAL_enablePWMCLKCounting(void)
{
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
}

void TTPLPFC_HAL_setupLEDGPIO(void)
{
    GPIO_setDirectionMode(TTPLPFC_GPIO_LED1, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(TTPLPFC_GPIO_LED2, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(TTPLPFC_GPIO_LED1, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(TTPLPFC_GPIO_LED2, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(TTPLPFC_GPIO_LED1_PIN_CONFIG);
    GPIO_setPinConfig(TTPLPFC_GPIO_LED2_PIN_CONFIG);
}

void TTPLPFC_HAL_setupTempMuxSelGPIO(void)
{
    //
    // Setup GaN temperature mux select pins
    //
    GPIO_setDirectionMode(TTPLPFC_GPIO_TEMP_MUX_SEL0, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(TTPLPFC_GPIO_TEMP_MUX_SEL0, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(TTPLPFC_GPIO_TEMP_MUX_SEL0_PIN_CONFIG);

    GPIO_setDirectionMode(TTPLPFC_GPIO_TEMP_MUX_SEL1, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(TTPLPFC_GPIO_TEMP_MUX_SEL1, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(TTPLPFC_GPIO_TEMP_MUX_SEL1_PIN_CONFIG);

    GPIO_setDirectionMode(TTPLPFC_GPIO_TEMP_MUX_SEL2, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(TTPLPFC_GPIO_TEMP_MUX_SEL2, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(TTPLPFC_GPIO_TEMP_MUX_SEL2_PIN_CONFIG);
}

void TTPLPFC_HAL_toggleLED(void)
{
    static uint16_t ledCnt1 = 0;

    if(ledCnt1 == 0)
    {
        GPIO_togglePin(TTPLPFC_GPIO_LED1);
        ledCnt1 = 5;
    }
    else
    {
        ledCnt1--;
    }

}

void TTPLPFC_HAL_setupProfilingGPIO(void)
{
    GPIO_setDirectionMode(TTPLPFC_GPIO_PROFILING1, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(TTPLPFC_GPIO_PROFILING2, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(TTPLPFC_GPIO_PROFILING1, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(TTPLPFC_GPIO_PROFILING2, GPIO_QUAL_SYNC);
    GPIO_setPinConfig(TTPLPFC_GPIO_PROFILING1_PIN_CONFIG);
    GPIO_setPinConfig(TTPLPFC_GPIO_PROFILING2_PIN_CONFIG);

#if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE
    GPIO_setMasterCore(TTPLPFC_GPIO_PROFILING1, GPIO_CORE_CPU1_CLA1);
    GPIO_setMasterCore(TTPLPFC_GPIO_PROFILING2, GPIO_CORE_CPU1_CLA1);
#endif
}

void TTPLPFC_HAL_setupRelayGPIO(void)
{
    GPIO_setDirectionMode(TTPLPFC_GPIO_RELAY, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(TTPLPFC_GPIO_RELAY, GPIO_QUAL_SYNC);

    GPIO_setPinConfig(TTPLPFC_GPIO_RELAY_PIN_CONFIG);

#if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE
    GPIO_setMasterCore(TTPLPFC_GPIO_RELAY, GPIO_CORE_CPU1_CLA1);
#endif

}

void TTPLPFC_HAL_setupPWM_UpDownCountWithDeadband(uint32_t base, uint16_t pwm_period_ticks,
                                                  uint16_t pwm_db_ticks_red, uint16_t pwm_db_ticks_fed)
{
    EPWM_setPeriodLoadMode(base, EPWM_PERIOD_SHADOW_LOAD);
    EPWM_setTimeBasePeriod(base, pwm_period_ticks >> 1);
    EPWM_setTimeBaseCounter(base, 0);
    EPWM_setPhaseShift(base, 0);
    EPWM_setTimeBaseCounterMode(base, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_setClockPrescaler(base, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);

    //
    // set as immediate mode
    //
    EPWM_disableCounterCompareShadowLoadMode(base, EPWM_COUNTER_COMPARE_A);

    //
    // set duty 0% initially
    //
    EPWM_setCounterCompareValue(base, EPWM_COUNTER_COMPARE_A, 0);
    EPWM_setCounterCompareShadowLoadMode(base, EPWM_COUNTER_COMPARE_A,
                                        EPWM_COMP_LOAD_ON_CNTR_ZERO);
    //
    // set duty 0% initially
    //
    EPWM_setCounterCompareValue(base, EPWM_COUNTER_COMPARE_A, 0);
    EPWM_setCounterCompareShadowLoadMode(base, EPWM_COUNTER_COMPARE_A,
                                        EPWM_COMP_LOAD_ON_CNTR_ZERO);

    //
    // Action Qualifier SubModule Registers
    //
    HWREGH(base + EPWM_O_AQCTLA) = 0;

    //
    // CTR = CMPA@UP , set to 0
    //
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A ,
            EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);

    //
    // CTR = CMPA@Down , set to 1
    //
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A ,
            EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);

    //
    // setupdeadband module
    //
    EPWM_setDeadBandControlShadowLoadMode(base, EPWM_DB_LOAD_ON_CNTR_ZERO);


    EPWM_setRisingEdgeDelayCountShadowLoadMode(base,
                                              EPWM_RED_LOAD_ON_CNTR_PERIOD);

    EPWM_setFallingEdgeDelayCountShadowLoadMode(base,
                                               EPWM_FED_LOAD_ON_CNTR_ZERO);

    EPWM_setDeadBandCounterClock(base, EPWM_DB_COUNTER_CLOCK_FULL_CYCLE);
    EPWM_setRisingEdgeDelayCount(base, pwm_db_ticks_red);
    EPWM_setFallingEdgeDelayCount(base, pwm_db_ticks_fed);
    EPWM_setDeadBandDelayMode(base, EPWM_DB_RED, true);
    EPWM_setDeadBandDelayMode(base, EPWM_DB_FED, true);
    EPWM_setRisingEdgeDeadBandDelayInput(base, EPWM_DB_INPUT_EPWMA);
    EPWM_setFallingEdgeDeadBandDelayInput(base, EPWM_DB_INPUT_EPWMA);
    EPWM_setDeadBandDelayPolarity(base, EPWM_DB_FED,
                                  EPWM_DB_POLARITY_ACTIVE_LOW);
    EPWM_setDeadBandDelayPolarity(base, EPWM_DB_RED,
                                  EPWM_DB_POLARITY_ACTIVE_HIGH);

}


//void TTPLPFC_HAL_setupGainChangeGPIO(void)
//{
//    GPIO_setDirectionMode(TTPLPFC_GPIO_I_SENSE_GAIN_ADJUST, GPIO_DIR_MODE_OUT);
//    GPIO_setQualificationMode(TTPLPFC_GPIO_I_SENSE_GAIN_ADJUST, GPIO_QUAL_SYNC);
//    GPIO_setPinConfig(TTPLPFC_GPIO_I_SENSE_GAIN_ADJUST_PIN_CONFIG);
//}

void TTPLPFC_HAL_setupPWM(uint16_t pwm_period_ticks,
                           uint16_t pwm_db_ticks_red, uint16_t pwm_db_ticks_fed)
{

#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
    TTPLPFC_HAL_setupPWM_UpDownCountWithDeadband(TTPLPFC_LOW_FREQ_PWM_BASE, pwm_period_ticks, pwm_db_ticks_red, pwm_db_ticks_fed);
#endif
    TTPLPFC_HAL_setupPWM_UpDownCountWithDeadband(TTPLPFC_HIGH_FREQ_PWM1_BASE, pwm_period_ticks, pwm_db_ticks_red, pwm_db_ticks_fed);
    TTPLPFC_HAL_setupPWM_UpDownCountWithDeadband(TTPLPFC_HIGH_FREQ_PWM2_BASE, pwm_period_ticks, pwm_db_ticks_red, pwm_db_ticks_fed);

#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
    HWREGH(TTPLPFC_LOW_FREQ_PWM_BASE + EPWM_O_AQCTLA) = 0;
    EPWM_setDeadBandDelayMode(TTPLPFC_LOW_FREQ_PWM_BASE, EPWM_DB_RED, false);
    EPWM_setDeadBandDelayMode(TTPLPFC_LOW_FREQ_PWM_BASE, EPWM_DB_FED, false);
#endif

    //
    // Setup phase sync
    //
    //


#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
    EPWM_disablePhaseShiftLoad(TTPLPFC_LOW_FREQ_PWM_BASE);
    EPWM_enableSyncOutPulseSource(TTPLPFC_LOW_FREQ_PWM_BASE,
                                     EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);
#endif

    EPWM_enablePhaseShiftLoad(TTPLPFC_HIGH_FREQ_PWM1_BASE);
    EPWM_setSyncInPulseSource(TTPLPFC_HIGH_FREQ_PWM1_BASE, TTPLPFC_PWM_SYNC_SOURCE);
    EPWM_setPhaseShift(TTPLPFC_HIGH_FREQ_PWM1_BASE, 2 );
    EPWM_setCountModeAfterSync(TTPLPFC_HIGH_FREQ_PWM1_BASE, EPWM_COUNT_MODE_UP_AFTER_SYNC);

    #if TTPLPFC_TOTAL_NO_PHASES == 2
        EPWM_enablePhaseShiftLoad(TTPLPFC_HIGH_FREQ_PWM2_BASE);
        EPWM_setSyncInPulseSource(TTPLPFC_HIGH_FREQ_PWM2_BASE, TTPLPFC_PWM_SYNC_SOURCE);
        //
        //  180 Degree Phase Shift for 2 phase interleaving
        //
        EPWM_setPhaseShift(TTPLPFC_HIGH_FREQ_PWM2_BASE,
                    ((uint16_t)((float32_t)TTPLPFC_PWM_PERIOD/(float32_t)(2.0)))-2);
        EPWM_setCountModeAfterSync(TTPLPFC_HIGH_FREQ_PWM2_BASE,
                                EPWM_COUNT_MODE_DOWN_AFTER_SYNC);

    #elif TTPLPFC_TOTAL_NO_PHASES == 3
        EPWM_enablePhaseShiftLoad(TTPLPFC_HIGH_FREQ_PWM2_BASE);
        EPWM_setSyncInPulseSource(TTPLPFC_HIGH_FREQ_PWM2_BASE, TTPLPFC_PWM_SYNC_SOURCE);

        EPWM_enablePhaseShiftLoad(TTPLPFC_HIGH_FREQ_PWM3_BASE);
        EPWM_setSyncInPulseSource(TTPLPFC_HIGH_FREQ_PWM3_BASE, TTPLPFC_PWM_SYNC_SOURCE);

        EPWM_setPhaseShift(TTPLPFC_HIGH_FREQ_PWM2_BASE,
                           (2 + (uint16_t)((float32_t)pwm_period_ticks /
                                   (float32_t)(3.0))));
        EPWM_setCountModeAfterSync(TTPLPFC_HIGH_FREQ_PWM2_BASE, EPWM_COUNT_MODE_UP_AFTER_SYNC);

        EPWM_setSyncOutPulseMode(TTPLPFC_HIGH_FREQ_PWM3_BASE, EPWM_SYNC_OUT_PULSE_ON_SOFTWARE);
        EPWM_setPhaseShift(TTPLPFC_HIGH_FREQ_PWM3_BASE,
                          (2 + (uint16_t)((float32_t)pwm_period_ticks /
                                  (float32_t)(3.0))));
        EPWM_setCountModeAfterSync(TTPLPFC_HIGH_FREQ_PWM3_BASE, EPWM_COUNT_MODE_DOWN_AFTER_SYNC);

    #endif



#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
    EPWM_setActionQualifierContSWForceShadowMode(TTPLPFC_LOW_FREQ_PWM_BASE,
            EPWM_AQ_SW_SH_LOAD_ON_CNTR_ZERO);
#endif
    EPWM_setActionQualifierContSWForceShadowMode(TTPLPFC_HIGH_FREQ_PWM1_BASE,
            EPWM_AQ_SW_SH_LOAD_ON_CNTR_ZERO);
    EPWM_setActionQualifierContSWForceShadowMode(TTPLPFC_HIGH_FREQ_PWM2_BASE,
            EPWM_AQ_SW_SH_LOAD_ON_CNTR_ZERO);
#if TTPLPFC_TOTAL_NO_PHASES == 3
    EPWM_setActionQualifierContSWForceShadowMode(TTPLPFC_HIGH_FREQ_PWM3_BASE,
            EPWM_AQ_SW_SH_LOAD_ON_CNTR_ZERO);
#endif

}

void TTPLPFC_HAL_setPinsAsPWM()
{
    GPIO_writePin(TTPLPFC_LOW_FREQ_PWM_H_GPIO, 0);
    GPIO_writePin(TTPLPFC_LOW_FREQ_PWM_L_GPIO, 0);
    GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM1_H_GPIO, 0);
    GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM1_L_GPIO, 0);
    GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM2_H_GPIO, 0);
    GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM2_L_GPIO, 0);

    GPIO_setDirectionMode( TTPLPFC_LOW_FREQ_PWM_H_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(TTPLPFC_LOW_FREQ_PWM_H_GPIO, GPIO_PIN_TYPE_STD);
#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
    GPIO_setPinConfig(TTPLPFC_LOW_FREQ_PWM_H_GPIO_PIN_CONFIG );
#else
    GPIO_setPinConfig(TTPLPFC_LOW_FREQ_PWM_H_GPIO_DISABLED_CONFIG);
#endif

    GPIO_setDirectionMode(TTPLPFC_LOW_FREQ_PWM_L_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(TTPLPFC_LOW_FREQ_PWM_L_GPIO, GPIO_PIN_TYPE_STD);
#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
    GPIO_setPinConfig(TTPLPFC_LOW_FREQ_PWM_L_GPIO_PIN_CONFIG);
#else
    GPIO_setPinConfig(TTPLPFC_LOW_FREQ_PWM_L_GPIO_DISABLED_CONFIG);
#endif

    GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM1_H_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM1_H_GPIO , GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM1_H_GPIO_PIN_CONFIG );

    GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM1_L_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM1_L_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM1_L_GPIO_PIN_CONFIG );

    GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM2_H_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM2_H_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM2_H_GPIO_PIN_CONFIG);

    GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM2_L_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM2_L_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM2_L_GPIO_PIN_CONFIG);

#if TTPLPFC_TOTAL_NO_PHASES == 3

    GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM3_H_GPIO, 0);
    GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM3_L_GPIO, 0);

    GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM3_H_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM3_H_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM3_H_GPIO_PIN_CONFIG);

    GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM3_L_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM3_L_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM3_L_GPIO_PIN_CONFIG);
#endif
}

//
//
//
void TTPLPFC_HAL_setupECAPforDutyCapture(uint32_t base)
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

void TTPLPFC_HAL_setupECAPforGaNTemp()
{
    TTPLPFC_HAL_setupECAPforDutyCapture(TTPLPFC_TEMP_MUX_OUT1_ECAP_BASE);
    TTPLPFC_HAL_setupECAPforDutyCapture(TTPLPFC_TEMP_MUX_OUT2_ECAP_BASE);

    ECAP_selectECAPInput(TTPLPFC_TEMP_MUX_OUT1_ECAP_BASE, TTPLPFC_TEMP_MUX_OUT1_ECAP_INPUT);
    XBAR_setInputPin(INPUTXBAR_BASE, TTPLPFC_TEMP_MUX_OUT1_INPUTXBAR, TTPLPFC_GPIO_TEMP_MUX_OUT1);

    ECAP_selectECAPInput(TTPLPFC_TEMP_MUX_OUT2_ECAP_BASE, TTPLPFC_TEMP_MUX_OUT2_ECAP_INPUT);
    XBAR_setInputPin(INPUTXBAR_BASE, TTPLPFC_TEMP_MUX_OUT2_INPUTXBAR, TTPLPFC_GPIO_TEMP_MUX_OUT2);
}

void TTPLPFC_HAL_setupCLA(void)
{
    //
    // setup CLA to register an interrupt
    //
#if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE

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

    //
    //Used to debug, the below line is intentionally commented
    //CLA_forceTasks(CLA1_BASE,CLA_TASKFLAG_1);
    //
    CLA_enableHardwareTrigger(CLA1_BASE);
    CLA_setTriggerSource(CLA_TASK_8, TTPLPFC_CLA_INSTRUMENTATIONISR_TRIG);
    CLA_enableBackgroundTask(CLA1_BASE);

    CLA_setTriggerSource(CLA_TASK_1, TTPLPFC_CLA_CONTROLISR_TRIG);

    TTPLPFC_HAL_enablePWMInterruptGeneration(EPWM1_BASE);
    TTPLPFC_HAL_clearPWMInterruptFlag(
            TTPLPFC_C28x_CONTROLISR_INTERRUPT_TRIG_PWM_BASE);


#endif
}

//
// Setup DAC - Setup the reference voltage and output value for the DAC
//
void TTPLPFC_HAL_setupDAC(void)
{
    //
    // Set VDAC as the DAC reference voltage.
    // Edit here to use ADC VREF as the reference voltage.
    //
    DAC_setReferenceVoltage(TTPLPFC_DAC_BASE,  DAC_REF_ADC_VREFHI);

    //
    // Enable the DAC output
    //
    DAC_enableOutput(TTPLPFC_DAC_BASE);

    //
    // Set the DAC shadow output to 0
    //
    DAC_setShadowValue(TTPLPFC_DAC_BASE, 0);

    //
    // Delay for buffered DAC to power up
    //
    DEVICE_DELAY_US(10);
}
