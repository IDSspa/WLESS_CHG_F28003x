//#############################################################################
//
// FILE:   ttplpfc_hal.h
//
// TITLE: This is the solution header file.
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

#ifndef TTPLPFC_HAL_H
#define TTPLPFC_HAL_H

#ifdef __cplusplus

extern "C" {
#endif


//
// the includes
//

//
// Include files for device support, F2806x in this case
//
#include <stdint.h>
#include "inc/hw_types.h"
#include "driverlib.h"
#include "device.h"
#include <ttplpfc_settings.h>

//
// typedefs
//

typedef union {
    struct {
        float32_t TTPLPFC_PH1_H_TEMP;
        float32_t TTPLPFC_PH1_L_TEMP;
        float32_t TTPLPFC_PH2_L_TEMP;
        float32_t TTPLPFC_PH2_H_TEMP;
        float32_t CLLLC_PRIM_LEG1_H_TEMP;
        float32_t CLLLC_PRIM_LEG1_L_TEMP;
        float32_t CLLLC_PRIM_LEG2_L_TEMP;
        float32_t CLLLC_PRIM_LEG2_H_TEMP;
        float32_t CLLLC_SEC_LEG1_H_TEMP;
        float32_t CLLLC_SEC_LEG1_L_TEMP;
        float32_t CLLLC_SEC_LEG2_L_TEMP;
        float32_t CLLLC_SEC_LEG2_H_TEMP;
    } device;
    float32_t array[TTPLPFC_TEMP_CHAN];
} TTPLPFC_HAL_GaNTemp_Struct;

extern TTPLPFC_HAL_GaNTemp_Struct TTPLPFC_HAL_GaNTemp_C;

void TTPLPFC_HAL_setupDevice(void);
void TTPLPFC_HAL_setupPWM(uint16_t pwm_period_ticks,
                           uint16_t pwm_dbred_ticks, uint16_t pwm_dbfed_ticks);

void TTPLPFC_HAL_setupADC(void);

void TTPLPFC_HALconfigurePWM1chUpCnt(uint32_t n, uint16_t period);
void TTPLPFC_HAL_setupPWMforTrip(uint32_t base);
void TTPLPFC_HAL_setupBoardProtection(float32_t current_limit,
                          float32_t current_max_sense);
void TTPLPFC_HAL_setupCMPSS(uint32_t base1, float32_t current_limit,
                            float32_t current_max_sense );

void TTPLPFC_calibrateOffset(volatile float32_t *ac_cur_sensedOffset,
                                   volatile float32_t *IL1c_sensedOffset,
                                   volatile float32_t *IL2c_sensedOffset ,
                                   volatile float32_t *IL3c_sensedOffset ,
                                   float32_t k1, float32_t k2);
void TTPLPFC_HAL_setupProfilingGPIO(void);
void TTPLPFC_HAL_setupRelayGPIO(void);
void TTPLPFC_HAL_toggleLED(void);
void TTPLPFC_HAL_setupLEDGPIO(void);
void TTPLPFC_HAL_disablePWMCLKCounting(void);
void TTPLPFC_HAL_enablePWMCLKCounting(void);
void TTPLPFC_HAL_setupTrigForADC(void);

void TTPLPFC_HAL_setupECAPforDutyCapture(uint32_t base);
void TTPLPFC_HAL_setupECAPforGaNTemp();
void TTPLPFC_HAL_setupTempMuxSelGPIO(void);

//
//void updateBoardStatus(void);
//
void TTPLPFC_HAL_setupGainChangeGPIO(void);

void TTPLPFC_HAL_setupEPWMtoTriggerADCSOC(uint32_t base);

void TTPLPFC_HAL_setPinsAsPWM();

void TTPLPFC_HAL_setupDAC(void);

void TTPLPFC_HAL_setupCLA(void);

//
// Function Prototypes
//
//
//CLA C Tasks defined in Cla1Tasks_C.cla
//
__attribute__((interrupt))  void Cla1Task1();
__attribute__((interrupt))  void Cla1Task2();
__attribute__((interrupt))  void Cla1Task3();
__attribute__((interrupt))  void Cla1Task4();
__attribute__((interrupt))  void Cla1Task5();
__attribute__((interrupt))  void Cla1Task6();
__attribute__((interrupt))  void Cla1Task7();
__attribute__((interrupt))  void Cla1BackgroundTask();

extern uint16_t Cla1ProgLoadStart;
extern uint16_t Cla1ProgLoadEnd;
extern uint16_t Cla1ProgLoadSize;
extern uint16_t Cla1ProgRunStart;
extern uint16_t Cla1ProgRunEnd;
extern uint16_t Cla1ProgRunSize;

//
// ISR related
//
extern void ISR2_primToSecPowerFlow(void);

#if TTPLPFC_CONTROL_RUNNING_ON == C28x_CORE

#ifndef __TMS320C28XX_CLA__
    #pragma INTERRUPT (ISR1, HPI)
    #pragma CODE_SECTION(ISR1,"isrcodefuncs");
    interrupt void ISR1(void);
    static inline void TTPLPFC_HAL_clearISR1InterruptFlag(
            uint16_t pie_group_no);
    static inline void TTPLPFC_HAL_setupInterrupt(void);
#endif

#endif

#if TTPLPFC_INSTRUMENTATION_ISR_RUNNING_ON == C28x_CORE

#ifndef __TMS320C28XX_CLA__
    #pragma CODE_SECTION(ISR2,"ramfuncs");
    interrupt void ISR2(void);
    static inline void TTPLPFC_HAL_clearISR1InterruptFlag(
            uint16_t pie_group_no);
    static inline void TTPLPFC_HAL_setupInterrupt(void);
#endif

#endif

//
// Inline functions
//
#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_EPWM_setCounterCompareValueOptimized)
static inline void TTPLPFC_HAL_EPWM_setCounterCompareValueOptimized(
        uint32_t base, EPWM_CounterCompareModule compModule, uint16_t compCount)
{
    uint32_t registerOffset;

    //
    // Get the register offset for the Counter compare
    //
    registerOffset = EPWM_O_CMPA + (uint16_t)compModule;

    //
    // Write to the counter compare registers.
    //
    #pragma diag_suppress = 770
    #pragma diag_suppress = 173
    if((compModule == EPWM_COUNTER_COMPARE_A) ||
    (compModule == EPWM_COUNTER_COMPARE_B))
    {
        //
        // write to CMPA or COMPB bits
        //
        HWREGH(base + registerOffset + 1) = (uint16_t)compCount;
    }
    else
    {
        //
        // write to COMPC or COMPD bits
        //
        HWREGH(base + registerOffset) = compCount;
    }
    #pragma diag_warning = 770
    #pragma diag_warning = 173
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_setupECAPforProfilingCode)
static inline void TTPLPFC_HAL_setupECAPforProfilingCode(void)
{

    //
    // Route profiling GPIO through INPUTXBAR
    //


    XBAR_setInputPin(INPUTXBAR_BASE, TTPLPFC_PROFILING1_INPUTXBAR, TTPLPFC_GPIO_PROFILING1);
    XBAR_setInputPin(INPUTXBAR_BASE, TTPLPFC_PROFILING2_INPUTXBAR, TTPLPFC_GPIO_PROFILING2);

    //
    // Configure ECAP to measure the rising to falling edge pulse
    //
    ECAP_disableTimeStampCapture(TTPLPFC_PROFILING1_ECAP);

    ECAP_stopCounter(TTPLPFC_PROFILING1_ECAP);
    ECAP_enableCaptureMode(TTPLPFC_PROFILING1_ECAP);

    ECAP_setCaptureMode(TTPLPFC_PROFILING1_ECAP, ECAP_CONTINUOUS_CAPTURE_MODE,
                        ECAP_EVENT_4);

    ECAP_setEventPolarity(TTPLPFC_PROFILING1_ECAP, ECAP_EVENT_1,
                          ECAP_EVNT_FALLING_EDGE);
    ECAP_setEventPolarity(TTPLPFC_PROFILING1_ECAP, ECAP_EVENT_2,
                          ECAP_EVNT_RISING_EDGE);
    ECAP_setEventPolarity(TTPLPFC_PROFILING1_ECAP, ECAP_EVENT_3,
                          ECAP_EVNT_FALLING_EDGE);
    ECAP_setEventPolarity(TTPLPFC_PROFILING1_ECAP, ECAP_EVENT_4,
                          ECAP_EVNT_RISING_EDGE);

    ECAP_enableCounterResetOnEvent(TTPLPFC_PROFILING1_ECAP, ECAP_EVENT_1);
    ECAP_enableCounterResetOnEvent(TTPLPFC_PROFILING1_ECAP, ECAP_EVENT_2);
    ECAP_enableCounterResetOnEvent(TTPLPFC_PROFILING1_ECAP, ECAP_EVENT_3);
    ECAP_enableCounterResetOnEvent(TTPLPFC_PROFILING1_ECAP, ECAP_EVENT_4);

    ECAP_startCounter(TTPLPFC_PROFILING1_ECAP);
    ECAP_enableTimeStampCapture(TTPLPFC_PROFILING1_ECAP);
    ECAP_reArm(TTPLPFC_PROFILING1_ECAP);


    ECAP_disableTimeStampCapture(TTPLPFC_PROFILING2_ECAP);

    ECAP_stopCounter(TTPLPFC_PROFILING2_ECAP);
    ECAP_enableCaptureMode(TTPLPFC_PROFILING2_ECAP);

    ECAP_setCaptureMode(TTPLPFC_PROFILING2_ECAP, ECAP_CONTINUOUS_CAPTURE_MODE,
                        ECAP_EVENT_4);

    ECAP_setEventPolarity(TTPLPFC_PROFILING2_ECAP, ECAP_EVENT_1,
                          ECAP_EVNT_FALLING_EDGE);
    ECAP_setEventPolarity(TTPLPFC_PROFILING2_ECAP, ECAP_EVENT_2,
                          ECAP_EVNT_RISING_EDGE);
    ECAP_setEventPolarity(TTPLPFC_PROFILING2_ECAP, ECAP_EVENT_3,
                          ECAP_EVNT_FALLING_EDGE);
    ECAP_setEventPolarity(TTPLPFC_PROFILING2_ECAP, ECAP_EVENT_4,
                          ECAP_EVNT_RISING_EDGE);

    ECAP_enableCounterResetOnEvent(TTPLPFC_PROFILING2_ECAP, ECAP_EVENT_1);
    ECAP_enableCounterResetOnEvent(TTPLPFC_PROFILING2_ECAP, ECAP_EVENT_2);
    ECAP_enableCounterResetOnEvent(TTPLPFC_PROFILING2_ECAP, ECAP_EVENT_3);
    ECAP_enableCounterResetOnEvent(TTPLPFC_PROFILING2_ECAP, ECAP_EVENT_4);

    ECAP_startCounter(TTPLPFC_PROFILING2_ECAP);
    ECAP_enableTimeStampCapture(TTPLPFC_PROFILING2_ECAP);
    ECAP_reArm(TTPLPFC_PROFILING2_ECAP);

    //
    // Recognize the XBAR as ECAP input
    //
    ECAP_selectECAPInput(TTPLPFC_PROFILING1_ECAP,
                       (ECAP_InputCaptureSignals)TTPLPFC_PROFILING1_ECAP_XBAR_MUX);
    ECAP_selectECAPInput(TTPLPFC_PROFILING2_ECAP,
                       (ECAP_InputCaptureSignals)TTPLPFC_PROFILING2_ECAP_XBAR_MUX);

}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_updateDAC)
static inline void TTPLPFC_HAL_updateDAC(float32_t value)
{
    //
    // Use the DAC to output an analog value propotional to the paremeter passed
    // the following code achieves the same function as the below driverlib call
    // DAC_setShadowValue(DCAEF_INJ_PD_LEVEL_DAC_BASE,
    //                (int32_t)((((float32_t)DCAEF_inj_pd_level_pu*4096.0f))));
    //
    HWREGH(TTPLPFC_DAC_BASE + DAC_O_VALS) =
            (int32_t)((float32_t)value * (4096.0f-200.0f)) + 100;
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_updatePWM)
static inline void TTPLPFC_HAL_updatePWM(uint32_t base, float32_t duty)
{
    uint32_t dutyPWMReg;

    dutyPWMReg = (uint32_t)((float32_t)(TTPLPFC_PWM_PERIOD / 2.0) *
            fabsf(duty));

#if TTPLPFC_TOTAL_NO_PHASES == 1 || TTPLPFC_TOTAL_NO_PHASES == 2 || TTPLPFC_TOTAL_NO_PHASES == 3
    TTPLPFC_HAL_EPWM_setCounterCompareValueOptimized(base,
                   EPWM_COUNTER_COMPARE_A, (uint32_t)((float32_t)dutyPWMReg));
    //
    // Optimized lab4 left others labs "as is"
    //
    //    HWREGH(base + EPWM_O_CMPA + EPWM_COUNTER_COMPARE_A + 1) = (uint32_t)((float32_t)dutyPWMReg);

    #endif

}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_enableHighSideHFPWM)
static inline void TTPLPFC_HAL_enableHighSideHFPWM()
{
    EALLOW;
    HWREG(GPIOCTRL_BASE + GPIO_O_GPAMUX1) = (HWREG(GPIO_O_GPAGMUX1) | 0x01100000);
    EDIS;
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_disableLowSideHFPWM)
static inline void TTPLPFC_HAL_disableLowSideHFPWM()
{
    EALLOW;
    HWREG(GPIOCTRL_BASE + GPIO_O_GPAMUX1) = (HWREG(GPIO_O_GPAGMUX1) & 0xF33FFFFF);
    EDIS;
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_enableLowSideHFPWM)
static inline void TTPLPFC_HAL_enableLowSideHFPWM()
{
    EALLOW;
    HWREG(GPIOCTRL_BASE + GPIO_O_GPAMUX1) = (HWREG(GPIO_O_GPAGMUX1) | 0x04400000);
    EDIS;
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_disableHighSideHFPWM)
static inline void TTPLPFC_HAL_disableHighSideHFPWM()
{
    EALLOW;
    HWREG(GPIOCTRL_BASE + GPIO_O_GPAMUX1) = (HWREG(GPIO_O_GPAGMUX1) & 0xFCCFFFFF);
    EDIS;
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_closeRelay)
static inline void TTPLPFC_HAL_closeRelay(void)
{
    HWREG(GPIODATA_BASE  + GPIO_O_GPASET ) = TTPLPFC_GPIO_RELAY_SET;
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_openRelay)
static inline void TTPLPFC_HAL_openRelay(void)
{
    HWREG(GPIODATA_BASE  + GPIO_O_GPACLEAR ) = TTPLPFC_GPIO_RELAY_CLEAR;
}

//static inline void TTPLPFC_HAL_gainHighSetforLowCurrent(void)
//{
//    GPIO_writePin(TTPLPFC_GPIO_I_SENSE_GAIN_ADJUST, 1);
//}
//
//static inline void TTPLPFC_HAL_gainLowSetforHighCurrent(void)
//{
//    GPIO_writePin(TTPLPFC_GPIO_I_SENSE_GAIN_ADJUST, 0);
//}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_setProfilingGPIO)
static inline void TTPLPFC_HAL_setProfilingGPIO(void)
{
    #pragma diag_suppress = 770
    #pragma diag_suppress = 173
    HWREG(GPIODATA_BASE + TTPLPFC_GPIO_PROFILING1_SET_REG ) = TTPLPFC_GPIO_PROFILING1_SET;
    #pragma diag_warning = 770
    #pragma diag_warning = 173
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_resetProfilingGPIO)
static inline void TTPLPFC_HAL_resetProfilingGPIO(void)
{
    #pragma diag_suppress = 770
    #pragma diag_suppress = 173
    HWREG(GPIODATA_BASE + TTPLPFC_GPIO_PROFILING1_CLEAR_REG ) = TTPLPFC_GPIO_PROFILING1_CLEAR;
    #pragma diag_warning = 770
    #pragma diag_warning = 173
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_setProfilingGPIO2)
static inline void TTPLPFC_HAL_setProfilingGPIO2(void)
{
    #pragma diag_suppress = 770
    #pragma diag_suppress = 173
    HWREG(GPIODATA_BASE + TTPLPFC_GPIO_PROFILING2_SET_REG ) = TTPLPFC_GPIO_PROFILING2_SET;
    #pragma diag_warning = 770
    #pragma diag_warning = 173
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_resetProfilingGPIO2)
static inline void TTPLPFC_HAL_resetProfilingGPIO2(void)
{
    #pragma diag_suppress = 770
    #pragma diag_suppress = 173
    HWREG(GPIODATA_BASE + TTPLPFC_GPIO_PROFILING2_CLEAR_REG ) = TTPLPFC_GPIO_PROFILING2_CLEAR;
    #pragma diag_warning = 770
    #pragma diag_warning = 173
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_clearPWMTripFlags)
static inline void TTPLPFC_HAL_clearPWMTripFlags(uint32_t base)
{
    //
    // clear all the configured trip sources for the PWM module
    //
    EPWM_clearTripZoneFlag(base,
                           ( EPWM_TZ_INTERRUPT_OST |
                             EPWM_TZ_INTERRUPT_CBC |
                             EPWM_TZ_INTERRUPT_DCAEVT1 )
                           );
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_clearOSTPWMTripFlag)
static inline void TTPLPFC_HAL_clearOSTPWMTripFlag(uint32_t base)
{
    //
    // clear all the configured trip sources for the PWM module
    //
    EPWM_clearTripZoneFlag(base, EPWM_TZ_INTERRUPT_OST);
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_forceOSTPWMTrip_onAllPWM)
static inline void TTPLPFC_HAL_forceOSTPWMTrip_onAllPWM()
{
    EPWM_forceTripZoneEvent(TTPLPFC_LOW_FREQ_PWM_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_forceTripZoneEvent(TTPLPFC_HIGH_FREQ_PWM1_BASE, EPWM_TZ_FORCE_EVENT_OST);

#if TTPLPFC_TOTAL_NO_PHASES >= 2
    EPWM_forceTripZoneEvent(TTPLPFC_HIGH_FREQ_PWM2_BASE, EPWM_TZ_FORCE_EVENT_OST);
#endif

    EPWM_setActionQualifierContSWForceAction(TTPLPFC_LOW_FREQ_PWM_BASE,
                                            EPWM_AQ_OUTPUT_A ,
                                            EPWM_AQ_SW_OUTPUT_LOW);
    EPWM_setActionQualifierContSWForceAction(TTPLPFC_LOW_FREQ_PWM_BASE,
                                            EPWM_AQ_OUTPUT_B ,
                                            EPWM_AQ_SW_OUTPUT_LOW);


}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_forceOSTPWMTrip)
static inline void TTPLPFC_HAL_forceOSTPWMTrip(uint32_t base)
{
    EPWM_forceTripZoneEvent(base, EPWM_TZ_FORCE_EVENT_OST);
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_clearPWMInterruptFlag)
static inline void TTPLPFC_HAL_clearPWMInterruptFlag(uint32_t base)
{
    EPWM_clearEventTriggerInterruptFlag(base);
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_enablePWMInterruptGeneration)
static inline void TTPLPFC_HAL_enablePWMInterruptGeneration(uint32_t base)
{
    EPWM_setCounterCompareValue(base, EPWM_COUNTER_COMPARE_B, TTPLPFC_AC_L_ADC_ACQPS_SYS_CLKS);
    EPWM_setInterruptSource(base, EPWM_INT_TBCTR_D_CMPB);
    EPWM_setInterruptEventCount(base, TTPLPFC_CNTRL_ISR_FREQ_RATIO);
    EPWM_enableInterrupt(base);
    EPWM_clearEventTriggerInterruptFlag(base);
}
//
// This function reads the positive duty captured by an ECAP module
// and re-arms the ECAP for the next sample
//
#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_readDuty)
static inline float32_t TTPLPFC_HAL_readDuty(uint32_t base)
{
    //
    // Variables for the ECAP timestamps
    //
    static uint32_t t1, t2, t3;

    //
    // Timestamps:
    // t1 = ECAP_EVENT_1 first rising edge
    // t2 = ECAP_EVENT_2 falling edge
    // ECAP_EVENT_3 second rising edge
    //
    // period = t3 - t1
    // positive pulse width = t2 - t1
    // duty = pulse width / period
    //
    t1 = ECAP_getEventTimeStamp(base, ECAP_EVENT_1);
    t2 = ECAP_getEventTimeStamp(base, ECAP_EVENT_2);
    t3 = ECAP_getEventTimeStamp(base, ECAP_EVENT_3);

    //
    // Return the measured duty
    //
    return ((float32_t)(t2-t1) / (t3-t1));
}

//
// This function updates the GPIOs to cycle to the next mux selection
// for the GaN temperature feedback
//
#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_updateGaNTempMux)
static inline void TTPLPFC_HAL_updateGaNTempMux(uint32_t sel)
{
    GPIO_writePin(TTPLPFC_GPIO_TEMP_MUX_SEL2, (sel & 0x4) >> 2);
    GPIO_writePin(TTPLPFC_GPIO_TEMP_MUX_SEL1, (sel & 0x2) >> 1);
    GPIO_writePin(TTPLPFC_GPIO_TEMP_MUX_SEL0, (sel & 0x1));
}

//
// This function reads the duty cycle on each of two ECAP pins,
// averages it over a number of cycles, and stores the converted
// result in an array of temperature readings
//
#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_updateGaNTemp)
static inline void TTPLPFC_HAL_updateGaNTemp()
{
    //
    // Running totals for averaging multiple duty readings
    //
    static float32_t sum1=0, sum2=0;

    //
    // Number of samples in each sum
    //
    static uint32_t samples1 = 0, samples2 = 0;

    //
    // Timeout variable to prevent the function from getting
    // stalled if there is no duty modulated signal on the
    // ECAP pin
    //
    static uint32_t timeout = 5*TTPLPFC_TEMP_OVERSAMPLE;

    //
    // Index used to determine mux selection and record
    // results in the temperature array
    //
    static uint32_t tempIndex = 0;

    //
    // If the oversampling requirement has been met, or the timeout has expired,
    // record the results in the temperature array, reset variables, and move to
    // the next mux selection
    //
    if( ( (samples1 >= TTPLPFC_TEMP_OVERSAMPLE) && (samples2 >= TTPLPFC_TEMP_OVERSAMPLE) ) || timeout == 0)
    {
        //
        // If either of the sum variables is equal to zero, then no data was recorded,
        // so store an invalid value in the array. Otherwise, converter the averaged duty
        // measurement to a temperature and store it in the array.
        //
        if(sum1 == 0)
        {
            TTPLPFC_HAL_GaNTemp_C.array[2*tempIndex] = -1;
        }
        else
        {
            TTPLPFC_HAL_GaNTemp_C.array[2*tempIndex] = (sum1 / samples1) * 162.3 + 20.12;
        }

        if(sum2 == 0)
        {
            TTPLPFC_HAL_GaNTemp_C.array[2*tempIndex+1] = -1;
        }
        else
        {
            TTPLPFC_HAL_GaNTemp_C.array[2*tempIndex+1] = (sum2 / samples2) * 162.3 + 20.12;
        }

        //
        // Reset the sampling variables
        //
        sum1 = 0;
        sum2 = 0;
        samples1 = 0;
        samples2 = 0;
        timeout = 5*TTPLPFC_TEMP_OVERSAMPLE;

        //
        // Move the mux to the next selection
        //
        tempIndex++;
        if(tempIndex >= (TTPLPFC_TEMP_CHAN / 2))
        {
            tempIndex = 0;
        }
        TTPLPFC_HAL_updateGaNTempMux(tempIndex);

        //
        // Re-arm the ECAPs now that the new mux selection has been made
        //
        ECAP_reArm(TTPLPFC_TEMP_MUX_OUT1_ECAP_BASE);
        ECAP_reArm(TTPLPFC_TEMP_MUX_OUT2_ECAP_BASE);
    }

    //
    // If the oversampling requirement has not been met yet and the timeout
    // has not expired, continue collecting samples.
    //
    else
    {
        //
        // If either ECAP has a sample ready accumulate that sample into the
        // respective running total
        //
        if(ECAP_getModuloCounterStatus(TTPLPFC_TEMP_MUX_OUT1_ECAP_BASE) == ECAP_EVENT_4)
        {
            sum1 += TTPLPFC_HAL_readDuty(TTPLPFC_TEMP_MUX_OUT1_ECAP_BASE);
            samples1++;

            //
            // Make the ECAP ready for the next sample
            //
            ECAP_reArm(TTPLPFC_TEMP_MUX_OUT1_ECAP_BASE);
        }

        if(ECAP_getModuloCounterStatus(TTPLPFC_TEMP_MUX_OUT2_ECAP_BASE) == ECAP_EVENT_4)
        {
            sum2 += TTPLPFC_HAL_readDuty(TTPLPFC_TEMP_MUX_OUT2_ECAP_BASE);
            samples2++;

            //
            // Make the ECAP ready for the next sample
            //
            ECAP_reArm(TTPLPFC_TEMP_MUX_OUT2_ECAP_BASE);
        }
    }

    timeout--;
}


#ifndef __TMS320C28XX_CLA__
#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_clearISR1InterruptFlag)
static inline void TTPLPFC_HAL_clearISR1InterruptFlag(uint16_t pie_group_no)
{
    Interrupt_clearACKGroup(pie_group_no);
}

#pragma FUNC_ALWAYS_INLINE(TTPLPFC_HAL_setupInterrupt)
static inline void TTPLPFC_HAL_setupInterrupt(void)
{

#if TTPLPFC_CONTROL_RUNNING_ON == C28x_CORE
    Interrupt_register(TTPLPFC_C28x_CONTROL_ISR, &ISR2_primToSecPowerFlow);
    TTPLPFC_HAL_enablePWMInterruptGeneration(
            TTPLPFC_C28x_CONTROL_ISR_TRIG_BASE);
    TTPLPFC_HAL_clearISR1InterruptFlag(
            TTPLPFC_C28x_CONTROL_ISR_PIE_GROUP_NO);
    Interrupt_enable(TTPLPFC_C28x_CONTROL_ISR);
#endif



#if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE ||                                  \
    TTPLPFC_INSTRUMENTATION_ISR_RUNNING_ON == CLA_CORE
    TTPLPFC_HAL_setupCLA();
#endif

    EALLOW;

    //
    //Enable Global interrupt INTM
    //
    EINT;

    //
    //Enable Global realtime interrupt DBGM
    //
    ERTM;
    EDIS;
}

#endif

#ifdef __cplusplus
}
#endif


#endif
