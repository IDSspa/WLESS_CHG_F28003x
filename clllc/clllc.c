//#############################################################################
//
// FILE:   clllc.c
//
// TITLE: This is the solution file.
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


//*****************************************************************************
// the includes
//*****************************************************************************

#include "clllc.h"

//
//--- System Related Globals ---
// Put the variables that are specific to control in the below section
// For example SFRA cannot run on CLA hence it must not be placed
// in the below section, the control verification using SFRA can only
// be carried out on the C28x
// Control Variables
//
#pragma SET_DATA_SECTION("controlVariables")

CLLLC_Lab_EnumType CLLLC_Lab;

CLLLC_TripFlag_EnumType CLLLC_tripFlag;

CLLLC_PwmSwState_EnumType CLLLC_pwmSwStateActive, CLLLC_pwmSwState;

CLLLC_PowerFlowState_EnumType CLLLC_powerFlowStateActive, CLLLC_powerFlowState;

CLLLC_CommandSentTo_AC_DC_EnumType CLLLC_commandSentTo_AC_DC;

CLLLC_ModulationMode_EnumType CLLLC_ModulationMode;

CLLLC_GI CLLLC_gi;
float32_t CLLLC_giOut;
float32_t CLLLC_giError;
float32_t CLLLC_giPartialComputedValue;

CLLLC_GI CLLLC_gv;
float32_t CLLLC_gvOut;
float32_t CLLLC_gvError;
float32_t CLLLC_gvPartialComputedValue;

//
// Flags for clearing trips and closing the loop
//
#pragma RETAIN (CLLLC_closeGiLoop)
#pragma RETAIN (CLLLC_closeGvLoop)
#pragma RETAIN (CLLLC_clearTrip)
#pragma RETAIN (CLLLC_hfcReceiverTestEnable)
#pragma RETAIN (CLLLC_hfcReceiverTestRun)
#pragma RETAIN (CLLLC_hfcReceiverTestActive)
#pragma RETAIN (CLLLC_hfcReceiverTestMode)
#pragma RETAIN (CLLLC_hfcReceiverTestSyncEnable)
#pragma RETAIN (CLLLC_hfcReceiverTestSyncConfigured)
#pragma RETAIN (CLLLC_hfcReceiverTestReady)
#pragma RETAIN (CLLLC_hfcReceiverTestInhibit)
#pragma RETAIN (CLLLC_hfcReceiverTestMinITankModRaw)
#pragma RETAIN (CLLLC_hfcReceiverTestDuty_pu)
#pragma RETAIN (CLLLC_hfcReceiverTestPhaseShiftPrimLegs_pu)
#pragma RETAIN (CLLLC_hfcReceiverTestPhaseShiftSeq)

int32_t CLLLC_closeGiLoop;
int32_t CLLLC_closeGvLoop;
int32_t CLLLC_clearTrip;
volatile uint16_t CLLLC_hfcReceiverTestEnable;
volatile uint16_t CLLLC_hfcReceiverTestRun;
volatile uint16_t CLLLC_hfcReceiverTestActive;
volatile uint16_t CLLLC_hfcReceiverTestMode;
volatile uint16_t CLLLC_hfcReceiverTestSyncEnable;
volatile uint16_t CLLLC_hfcReceiverTestSyncConfigured;
volatile uint16_t CLLLC_hfcReceiverTestReady;
volatile uint16_t CLLLC_hfcReceiverTestInhibit;
volatile uint16_t CLLLC_hfcReceiverTestMinITankModRaw;
volatile float32_t CLLLC_hfcReceiverTestDuty_pu;
volatile float32_t CLLLC_hfcReceiverTestPhaseShiftPrimLegs_pu;
volatile uint16_t CLLLC_hfcReceiverTestPhaseShiftSeq;

#pragma RETAIN (CLLLC_hfcGanFaultGpioLevel)
#pragma RETAIN (CLLLC_hfcGanFaultActiveLow)
#pragma RETAIN (CLLLC_hfcGanFaultXbarFlag)
#pragma RETAIN (CLLLC_epwm1TzFlgDebug)
#pragma RETAIN (CLLLC_epwm2TzFlgDebug)
#pragma RETAIN (CLLLC_epwm3TzFlgDebug)
#pragma RETAIN (CLLLC_epwm1TzOstFlgDebug)
#pragma RETAIN (CLLLC_epwm2TzOstFlgDebug)
#pragma RETAIN (CLLLC_epwm3TzOstFlgDebug)
#pragma RETAIN (CLLLC_epwm1GanFaultOst2Latched)
#pragma RETAIN (CLLLC_epwm2GanFaultOst2Latched)
#pragma RETAIN (CLLLC_epwm3GanFaultOst2Latched)

volatile uint16_t CLLLC_hfcGanFaultGpioLevel;
volatile uint16_t CLLLC_hfcGanFaultActiveLow;
volatile uint16_t CLLLC_hfcGanFaultXbarFlag;
volatile uint16_t CLLLC_epwm1TzFlgDebug;
volatile uint16_t CLLLC_epwm2TzFlgDebug;
volatile uint16_t CLLLC_epwm3TzFlgDebug;
volatile uint16_t CLLLC_epwm1TzOstFlgDebug;
volatile uint16_t CLLLC_epwm2TzOstFlgDebug;
volatile uint16_t CLLLC_epwm3TzOstFlgDebug;
volatile uint16_t CLLLC_epwm1GanFaultOst2Latched;
volatile uint16_t CLLLC_epwm2GanFaultOst2Latched;
volatile uint16_t CLLLC_epwm3GanFaultOst2Latched;


#pragma RETAIN (CLLLC_pwmFrequency_Hz)
#pragma RETAIN (CLLLC_pwmFrequencyPrev_Hz)

float32_t CLLLC_pwmFrequency_Hz;
float32_t CLLLC_pwmFrequencyPrev_Hz;


#pragma RETAIN (CLLLC_pwmPeriodRef_pu)
float32_t CLLLC_pwmPeriodRef_pu;
float32_t CLLLC_pwmPeriod_pu;
#pragma RETAIN (CLLLC_pwmPeriodSlewed_pu)
float32_t CLLLC_pwmPeriodSlewed_pu;
float32_t CLLLC_pwmPeriodMin_pu;
float32_t CLLLC_pwmPeriodMax_pu;
float32_t CLLLC_pwmPeriodMax_ticks;
uint32_t CLLLC_pwmPeriod_ticks;

//
// 1- Primary Side (PFC-Inv/Bus)
//
float32_t CLLLC_iPrimSensed_Amps;
#pragma RETAIN (CLLLC_iPrimSensed_pu)
float32_t CLLLC_iPrimSensed_pu;
float32_t CLLLC_iPrimSensedOffset_pu;
float32_t CLLLC_iPrimSensedCalIntercept_pu;
float32_t CLLLC_iPrimSensedCalXvariable_pu;
EMAVG CLLLC_iPrimSensedAvg_pu;

float32_t CLLLC_iPrimTankSensed_Amps;
float32_t CLLLC_iPrimTankSensed_pu;
float32_t CLLLC_iPrimTankSensedOffset_pu;
float32_t CLLLC_iPrimTankSensedCalIntercept_pu;
float32_t CLLLC_iPrimTankSensedCalXvariable_pu;
EMAVG CLLLC_iPrimTankSensedAvg_pu;

void CLLLC_applyITankCmpssDebugSelection(void)
{
    if((CLLLC_iTankCmpssDebugCmpssNo != CLLLC_iTankCmpssDebugAppliedCmpssNo) ||
       (CLLLC_iTankCmpssDebugMuxValue != CLLLC_iTankCmpssDebugAppliedMuxValue))
    {
        CLLLC_HAL_configureITankCmpssDebug(CLLLC_iTankCmpssDebugCmpssNo,
                                           CLLLC_iTankCmpssDebugMuxValue);
        CLLLC_iTankCmpssDebugAppliedCmpssNo = CLLLC_iTankCmpssDebugCmpssNo;
        CLLLC_iTankCmpssDebugAppliedMuxValue = CLLLC_iTankCmpssDebugMuxValue;
    }
}

void CLLLC_applyITankCmpssDebugThresholds(void)
{
    if((CLLLC_iTankCmpssDebugDacHigh !=
            CLLLC_iTankCmpssDebugAppliedDacHigh) ||
       (CLLLC_iTankCmpssDebugDacLow !=
            CLLLC_iTankCmpssDebugAppliedDacLow))
    {
        CLLLC_HAL_setITankCmpssDebugThresholds(
                                               CLLLC_iTankCmpssDebugAppliedCmpssNo,
                                               CLLLC_iTankCmpssDebugDacHigh,
                                               CLLLC_iTankCmpssDebugDacLow);
        CLLLC_iTankCmpssDebugAppliedDacHigh = CLLLC_iTankCmpssDebugDacHigh;
        CLLLC_iTankCmpssDebugAppliedDacLow = CLLLC_iTankCmpssDebugDacLow;
    }
}

void CLLLC_applyHfcReceiverTestConfig(void)
{
    if(CLLLC_hfcReceiverTestMode >
            CLLLC_HFC_RECEIVER_TEST_MODE_MANUAL_NEGATIVE)
    {
        CLLLC_hfcReceiverTestMode =
                CLLLC_HFC_RECEIVER_TEST_MODE_MONITOR_ONLY;
    }

    if((CLLLC_hfcReceiverTestEnable == 0U) ||
       (CLLLC_hfcReceiverTestSyncEnable == 0U) ||
       ((CLLLC_hfcReceiverTestMode !=
             CLLLC_HFC_RECEIVER_TEST_MODE_SYNC_RECTIFIER) &&
        (CLLLC_hfcReceiverTestMode !=
             CLLLC_HFC_RECEIVER_TEST_MODE_FORCE_POSITIVE) &&
        (CLLLC_hfcReceiverTestMode !=
             CLLLC_HFC_RECEIVER_TEST_MODE_FORCE_NEGATIVE)))
    {
        if(CLLLC_hfcReceiverTestSyncConfigured != 0U)
        {
            CLLLC_HAL_resetSynchronousRectifierTripAction(
                    CLLLC_POWER_FLOW_SEC_PRIM);
            CLLLC_hfcReceiverTestSyncConfigured = 0U;
        }
        CLLLC_hfcReceiverTestReady =
                ((CLLLC_hfcReceiverTestEnable != 0U) &&
                  (CLLLC_tripFlag.CLLLC_TripFlag_Enum == noTrip)) ? 1U : 0U;
        if((CLLLC_hfcReceiverTestEnable != 0U) &&
           ((CLLLC_hfcReceiverTestMode ==
                  CLLLC_HFC_RECEIVER_TEST_MODE_FIXED_SAFE_PWM) ||
            (CLLLC_hfcReceiverTestMode ==
                  CLLLC_HFC_RECEIVER_TEST_MODE_MANUAL_POSITIVE) ||
            (CLLLC_hfcReceiverTestMode ==
                  CLLLC_HFC_RECEIVER_TEST_MODE_MANUAL_NEGATIVE)))
        {
            CLLLC_HAL_setupPWMpins(pwmSwState_synchronousRectification_fixedDuty);
            CLLLC_pwmSwState.CLLLC_PwmSwState_Enum =
                    pwmSwState_synchronousRectification_fixedDuty;
            CLLLC_pwmSwStateActive.CLLLC_PwmSwState_Enum =
                    pwmSwState_synchronousRectification_fixedDuty;
        }
        return;
    }

    if(CLLLC_hfcReceiverTestSyncConfigured == 0U)
    {
        CLLLC_HAL_setupSynchronousRectificationAction(
                CLLLC_POWER_FLOW_SEC_PRIM);
        CLLLC_HAL_setupPWMpins(pwmSwState_synchronousRectification_active);
        CLLLC_pwmSwState.CLLLC_PwmSwState_Enum =
                pwmSwState_synchronousRectification_active;
        CLLLC_pwmSwStateActive.CLLLC_PwmSwState_Enum =
                pwmSwState_synchronousRectification_active;
        CLLLC_hfcReceiverTestSyncConfigured = 1U;
    }

    if(CLLLC_hfcReceiverTestMode ==
            CLLLC_HFC_RECEIVER_TEST_MODE_FORCE_POSITIVE)
    {
        CLLLC_HAL_setITankCmpssDebugThresholds(
                CLLLC_iTankCmpssDebugAppliedCmpssNo, 0U, 0U);
        CLLLC_iTankCmpssDebugAppliedDacHigh = 0U;
        CLLLC_iTankCmpssDebugAppliedDacLow = 0U;
    }
    else if(CLLLC_hfcReceiverTestMode ==
            CLLLC_HFC_RECEIVER_TEST_MODE_FORCE_NEGATIVE)
    {
        CLLLC_HAL_setITankCmpssDebugThresholds(
                CLLLC_iTankCmpssDebugAppliedCmpssNo, 4095U, 4095U);
        CLLLC_iTankCmpssDebugAppliedDacHigh = 4095U;
        CLLLC_iTankCmpssDebugAppliedDacLow = 4095U;
    }

    CLLLC_hfcReceiverTestReady =
            (CLLLC_tripFlag.CLLLC_TripFlag_Enum == noTrip) ? 1U : 0U;
}


uint16_t CLLLC_unusedAdca5SensedRaw;
uint16_t CLLLC_iTankSensedRaw;
uint16_t CLLLC_iTankModSensedRaw;
uint16_t CLLLC_iTankCmpssStatus;
uint16_t CLLLC_iTankCmpssHigh;
uint16_t CLLLC_iTankCmpssHighLatched;
uint16_t CLLLC_iTankCmpssLow;
uint16_t CLLLC_iTankCmpssLowLatched;
uint16_t CLLLC_iTankCmpssDebugCmpssNo;
uint16_t CLLLC_iTankCmpssDebugMuxValue;
uint16_t CLLLC_iTankCmpssDebugAppliedCmpssNo;
uint16_t CLLLC_iTankCmpssDebugAppliedMuxValue;
uint16_t CLLLC_iTankCmpssDebugDacHigh;
uint16_t CLLLC_iTankCmpssDebugDacLow;
uint16_t CLLLC_iTankCmpssDebugAppliedDacHigh;
uint16_t CLLLC_iTankCmpssDebugAppliedDacLow;
uint16_t CLLLC_iTankCmpssDebugResetCounts;
uint16_t CLLLC_iTankCmpssState;
uint16_t CLLLC_iTankCmpssLastState;
uint16_t CLLLC_iTankSensedRawMin;
uint16_t CLLLC_iTankSensedRawMax;
uint16_t CLLLC_iTankModSensedRawMin;
uint16_t CLLLC_iTankModSensedRawMax;
uint32_t CLLLC_iTankCmpssSampleCount;
uint32_t CLLLC_iTankCmpssHighOnlyCount;
uint32_t CLLLC_iTankCmpssLowOnlyCount;
uint32_t CLLLC_iTankCmpssBothCount;
uint32_t CLLLC_iTankCmpssNeitherCount;
uint32_t CLLLC_iTankCmpssTransitionCount;
float32_t CLLLC_unusedAdca5Sensed_pu;
float32_t CLLLC_iTankSensed_pu;
float32_t CLLLC_iTankModSensed_pu;
float32_t CLLLC_iTankModAmpsPerPu;
float32_t CLLLC_iTankModLimit_Amps;
float32_t CLLLC_unusedAdca5SensedOffset_pu;
float32_t CLLLC_iTankSensedOffset_pu;
float32_t CLLLC_iTankModSensedOffset_pu;

float32_t CLLLC_vPrimSensed_Volts;
float32_t CLLLC_vPrimSensed_pu;
float32_t CLLLC_vPrimSensedOffset_pu;
EMAVG CLLLC_vPrimSensedAvg_pu;

float32_t CLLLC_vPrimRef_Volts;
float32_t CLLLC_vPrimRef_pu;
float32_t CLLLC_vPrimRefSlewed_pu;

#pragma RETAIN (CLLLC_pwmDutyPrimRef_pu)
float32_t CLLLC_pwmDutyPrimRef_pu;
float32_t CLLLC_pwmDutyPrim_pu;
uint32_t CLLLC_pwmDutyAPrim_ticks;
uint32_t CLLLC_pwmDutyBPrim_ticks;

#pragma RETAIN (CLLLC_pwmDeadBandREDPrimRef_ns)
float32_t CLLLC_pwmDeadBandREDPrimRef_ns;
uint32_t CLLLC_pwmDeadBandREDPrim_ticks;

#pragma RETAIN (CLLLC_pwmDeadBandFEDPrimRef_ns)
float32_t CLLLC_pwmDeadBandFEDPrimRef_ns;
uint32_t CLLLC_pwmDeadBandFEDPrim_ticks;

#pragma RETAIN (CLLLC_startupDelay)
float32_t CLLLC_startupDelay = 2000;

//
// 2-Secondary side (Battery)
//
uint32_t CLLLC_iSecSenseDiag;

float32_t CLLLC_iSecSensed_Amps;
#pragma RETAIN (CLLLC_iSecSensed_pu)
float32_t CLLLC_iSecSensed_pu;
float32_t CLLLC_iSecSensedOffset_pu;
float32_t CLLLC_iSecSensedCalIntercept_pu;
float32_t CLLLC_iSecSensedCalXvariable_pu;
EMAVG CLLLC_iSecSensedAvg_pu;

#pragma RETAIN (CLLLC_iSecRef_Amps)
float32_t CLLLC_iSecRef_Amps;
float32_t CLLLC_iSecRef_pu;
float32_t CLLLC_iSecRefSlewed_pu;

float32_t CLLLC_vSecSensed_Volts;
float32_t CLLLC_vSecSensed_pu;
float32_t CLLLC_vSecSensedOffset_pu;

float32_t CLLLC_vSecRef_Volts;
float32_t CLLLC_vSecRef_pu;
float32_t CLLLC_vSecRefSlewed_pu;
EMAVG CLLLC_vSecSensedAvg_pu;

#pragma RETAIN (CLLLC_pwmDutySecRef_pu)
float32_t CLLLC_pwmDutySecRef_pu;
float32_t CLLLC_pwmDutySec_pu;
uint32_t CLLLC_pwmDutyASec_ticks;
uint32_t CLLLC_pwmDutyBSec_ticks;
#pragma RETAIN (CLLLC_pwmDutySecAdjust_pu)
float32_t CLLLC_pwmDutySecAdjust_pu;

float32_t CLLLC_pwmDeadBandREDSec_ns;
uint16_t CLLLC_pwmDeadBandREDSec_ticks;

float32_t CLLLC_pwmDeadBandFEDSec_ns;
uint16_t CLLLC_pwmDeadbandFEDSec_ticks;

#pragma RETAIN (CLLLC_pwmPhaseShiftPrimSecRef_ns)
float32_t CLLLC_pwmPhaseShiftPrimSecRef_ns;
float32_t CLLLC_pwmPhaseShiftPrimSec_ns;
int32_t CLLLC_pwmPhaseShiftPrimSec_ticks;
int16_t CLLLC_pwmPhaseShiftPrimSec_countDirection;

#pragma RETAIN (CLLLC_pwmPhaseShiftPrimLegsRef_pu)
float32_t CLLLC_pwmPhaseShiftPrimLegsRef_pu;
float32_t CLLLC_pwmPhaseShiftPrimLegs_pu;
#pragma RETAIN (CLLLC_pwmPhaseShiftPrimLegs_ns)
float32_t CLLLC_pwmPhaseShiftPrimLegs_ns;
int32_t CLLLC_pwmPhaseShiftPrimLegs_ticks;
#pragma RETAIN (CLLLC_pwmPhaseShiftPrimLegs_HiResticks)
uint16_t CLLLC_pwmPhaseShiftPrimLegs_HiResticks;
int16_t CLLLC_pwmPhaseShiftPrimLegs_countDirection;

#pragma RETAIN (CLLLC_modeChangeFlag)
uint16_t CLLLC_modeChangeFlag;

#pragma RETAIN (CLLLC_pwmISRTrig_ticks)
#pragma RETAIN (CLLLC_pwmISRTrig_ticks_prev)
uint16_t CLLLC_pwmISRTrig_ticks;
uint16_t CLLLC_pwmISRTrig_ticks_prev;

//#pragma RETAIN (CLLLC_cla_task_counter)
//uint32_t CLLLC_cla_task_counter;


#pragma RETAIN (CLLLC_enableLC)
uint16_t CLLLC_enableLC;

#pragma SET_DATA_SECTION()

uint32_t CLLLC_slewSCIcommand;

//
// datalogger
//
DLOG_4CH CLLLC_dLog1;
float32_t CLLLC_dBuff1[100];
float32_t CLLLC_dBuff2[100];
float32_t CLLLC_dBuff3[100];
float32_t CLLLC_dBuff4[100];
float32_t CLLLC_dVal1;
float32_t CLLLC_dVal2;
float32_t CLLLC_dVal3;
float32_t CLLLC_dVal4;
#pragma RETAIN (CLLLC_dlogTrigger)
volatile float32_t CLLLC_dlogTrigger;



//
//--- SFRA Related Variables ---
//
#if CLLLC_SFRA_TYPE != CLLLC_SFRA_DISABLED
float32_t CLLLC_plantMagVect[CLLLC_SFRA_FREQ_LENGTH];
float32_t CLLLC_plantPhaseVect[CLLLC_SFRA_FREQ_LENGTH];
float32_t CLLLC_olMagVect[CLLLC_SFRA_FREQ_LENGTH];
float32_t CLLLC_olPhaseVect[CLLLC_SFRA_FREQ_LENGTH];
float32_t CLLLC_freqVect[CLLLC_SFRA_FREQ_LENGTH];

SFRA_F32 CLLLC_sfra1;
#endif

void CLLLC_updateFaultDebugSignals(void)
{
    CLLLC_hfcGanFaultGpioLevel =
            (uint16_t)GPIO_readPin(CLLLC_GANFAULTn_GPIO);
    CLLLC_hfcGanFaultActiveLow =
            (CLLLC_hfcGanFaultGpioLevel == 0U) ? 1U : 0U;
    CLLLC_hfcGanFaultXbarFlag =
            (XBAR_getInputFlagStatus(XBAR_INPUT_FLG_INPUT2) == 1U) ? 1U : 0U;

    CLLLC_epwm1TzFlgDebug =
            EPWM_getTripZoneFlagStatus(CLLLC_PRIM_LEG1_PWM_BASE);
    CLLLC_epwm2TzFlgDebug =
            EPWM_getTripZoneFlagStatus(CLLLC_PRIM_LEG2_PWM_BASE);
    CLLLC_epwm3TzFlgDebug =
            EPWM_getTripZoneFlagStatus(CLLLC_SEC_LEG1_PWM_BASE);

    CLLLC_epwm1TzOstFlgDebug =
            EPWM_getOneShotTripZoneFlagStatus(CLLLC_PRIM_LEG1_PWM_BASE);
    CLLLC_epwm2TzOstFlgDebug =
            EPWM_getOneShotTripZoneFlagStatus(CLLLC_PRIM_LEG2_PWM_BASE);
    CLLLC_epwm3TzOstFlgDebug =
            EPWM_getOneShotTripZoneFlagStatus(CLLLC_SEC_LEG1_PWM_BASE);

    CLLLC_epwm1GanFaultOst2Latched =
            ((CLLLC_epwm1TzOstFlgDebug & CLLLC_GANFAULTn_EPWM_FLAG) != 0U) ?
                    1U : 0U;
    CLLLC_epwm2GanFaultOst2Latched =
            ((CLLLC_epwm2TzOstFlgDebug & CLLLC_GANFAULTn_EPWM_FLAG) != 0U) ?
                    1U : 0U;
    CLLLC_epwm3GanFaultOst2Latched =
            ((CLLLC_epwm3TzOstFlgDebug & CLLLC_GANFAULTn_EPWM_FLAG) != 0U) ?
                    1U : 0U;
}

void CLLLC_runISR3(void)
{
    CLLLC_updateFaultDebugSignals();

    CLLLC_iSecSenseDiag = CLLLC_readSecSenseDiag();

    EMAVG_run(&CLLLC_iSecSensedAvg_pu, CLLLC_iSecSensed_pu);
    EMAVG_run(&CLLLC_iPrimSensedAvg_pu, CLLLC_iPrimSensed_pu);
    EMAVG_run(&CLLLC_vSecSensedAvg_pu, CLLLC_vSecSensed_pu);
    EMAVG_run(&CLLLC_vPrimSensedAvg_pu, CLLLC_vPrimSensed_pu);

    CLLLC_vPrimSensed_Volts = CLLLC_vPrimSensedAvg_pu.out *
                             CLLLC_VPRIM_MAX_SENSE_VOLTS;
    CLLLC_vSecSensed_Volts = CLLLC_vSecSensedAvg_pu.out *
                             CLLLC_VSEC_OPTIMAL_RANGE_VOLTS;
    CLLLC_iPrimSensed_Amps = CLLLC_iPrimSensedAvg_pu.out *
                             CLLLC_IPRIM_MAX_SENSE_AMPS;
    CLLLC_iSecSensed_Amps = CLLLC_iSecSensedAvg_pu.out *
                            CLLLC_ISEC_MAX_SENSE_AMPS;

    #if CLLLC_CONTROL_MODE == CLLLC_VOLTAGE_MODE

        #if CLLLC_POWER_FLOW == CLLLC_POWER_FLOW_PRIM_SEC
            CLLLC_vSecRef_pu = CLLLC_vSecRef_Volts /
                               CLLLC_VSEC_OPTIMAL_RANGE_VOLTS;

            if((CLLLC_vSecRef_pu - CLLLC_vSecRefSlewed_pu) >
                (2.0f * CLLLC_VOLTS_PER_SECOND_SLEW /
                        CLLLC_VSEC_OPTIMAL_RANGE_VOLTS) *
                (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ))
            {
                CLLLC_vSecRefSlewed_pu = CLLLC_vSecRefSlewed_pu +
                        ((CLLLC_VOLTS_PER_SECOND_SLEW /
                                CLLLC_VSEC_OPTIMAL_RANGE_VOLTS) *
                      (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ));
            }
            else if((CLLLC_vSecRef_pu - CLLLC_vSecRefSlewed_pu) <
                    - (2.0f * CLLLC_VOLTS_PER_SECOND_SLEW /
                            CLLLC_VSEC_OPTIMAL_RANGE_VOLTS)
                    * (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ))
            {
                CLLLC_vSecRefSlewed_pu = CLLLC_vSecRefSlewed_pu -
                        ((CLLLC_VOLTS_PER_SECOND_SLEW /
                                CLLLC_VSEC_OPTIMAL_RANGE_VOLTS) *
                     (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ));
            }
            else
            {
                CLLLC_vSecRefSlewed_pu = CLLLC_vSecRef_pu;
            }
        #elif CLLLC_POWER_FLOW == CLLLC_POWER_FLOW_SEC_PRIM
            CLLLC_vPrimRef_pu = CLLLC_vPrimRef_Volts /
                    CLLLC_VPRIM_MAX_SENSE_VOLTS;

            if((CLLLC_vPrimRef_pu - CLLLC_vPrimRefSlewed_pu) >
                (2.0f * CLLLC_VOLTS_PER_SECOND_SLEW /
                        CLLLC_VPRIM_MAX_SENSE_VOLTS)
                * (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ))
            {
                CLLLC_vPrimRefSlewed_pu = CLLLC_vPrimRefSlewed_pu +
                        ((CLLLC_VOLTS_PER_SECOND_SLEW /
                                CLLLC_VPRIM_MAX_SENSE_VOLTS) *
                      (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ));
            }
            else if((CLLLC_vPrimRef_pu - CLLLC_vPrimRefSlewed_pu) <
                    - (2.0f * CLLLC_VOLTS_PER_SECOND_SLEW /
                            CLLLC_VPRIM_MAX_SENSE_VOLTS)
                 * (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ))
            {
                CLLLC_vPrimRefSlewed_pu = CLLLC_vPrimRefSlewed_pu -
                        ((CLLLC_VOLTS_PER_SECOND_SLEW /
                                CLLLC_VPRIM_MAX_SENSE_VOLTS) *
                     (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ));
            }
            else
            {
                CLLLC_vPrimRefSlewed_pu = CLLLC_vPrimRef_pu;
            }
        #endif
    #else
        CLLLC_iSecRef_pu = CLLLC_iSecRef_Amps / CLLLC_ISEC_MAX_SENSE_AMPS;

        if((CLLLC_iSecRef_pu - CLLLC_iSecRefSlewed_pu) >
            (2.0f * CLLLC_AMPS_PER_SECOND_SLEW / CLLLC_ISEC_MAX_SENSE_AMPS) *
            (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ))
        {
            CLLLC_iSecRefSlewed_pu = CLLLC_iSecRefSlewed_pu +
              ((CLLLC_AMPS_PER_SECOND_SLEW / CLLLC_ISEC_MAX_SENSE_AMPS) *
               (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ));
        }
        else if((CLLLC_iSecRef_pu - CLLLC_iSecRefSlewed_pu) <
             - (2.0f * CLLLC_AMPS_PER_SECOND_SLEW /
                     CLLLC_ISEC_MAX_SENSE_AMPS) *
               (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ))
        {
            CLLLC_iSecRefSlewed_pu = CLLLC_iSecRefSlewed_pu -
                 ((CLLLC_AMPS_PER_SECOND_SLEW / CLLLC_ISEC_MAX_SENSE_AMPS) *
                 (1.0f / (float32_t)CLLLC_ISR3_FREQUENCY_HZ));
        }
        else
        {
            CLLLC_iSecRefSlewed_pu = CLLLC_iSecRef_pu;
        }
    #endif


    CLLLC_calculatePWMDeadBandPrimTicks();

    CLLLC_HAL_updatePWMDeadBandPrim(CLLLC_pwmDeadBandREDPrim_ticks,
                                    CLLLC_pwmDeadBandFEDPrim_ticks);

    CLLLC_HAL_updateGaNTemp();
}

void CLLLC_initGlobalVariables(void)
{
    CLLLC_enableLC = 1;


    #if CLLLC_POWER_FLOW == CLLLC_POWER_FLOW_SEC_PRIM
        CLLLC_powerFlowStateActive.CLLLC_PowerFlowState_Enum =
                powerFlow_SecToPrim;
        CLLLC_powerFlowState.CLLLC_PowerFlowState_Enum =
                powerFlow_SecToPrim;
    #else
        CLLLC_powerFlowStateActive.CLLLC_PowerFlowState_Enum =
                powerFlow_PrimToSec;
        CLLLC_powerFlowState.CLLLC_PowerFlowState_Enum =
                powerFlow_PrimToSec;
    #endif

    DCL_resetDF13(&CLLLC_gi);
    #if CLLLC_SEC_CONNECTED_IN_BATTERY_EMULATION_MODE == 1
        CLLLC_gi.a1 = CLLLC_GI2_2P2Z_A1;
        CLLLC_gi.a2 = CLLLC_GI2_2P2Z_A2;
        CLLLC_gi.a3 = CLLLC_GI2_2P2Z_A3;
        CLLLC_gi.b0 = CLLLC_GI2_2P2Z_B0;
        CLLLC_gi.b1 = CLLLC_GI2_2P2Z_B1;
        CLLLC_gi.b2 = CLLLC_GI2_2P2Z_B2;
        CLLLC_gi.b3 = CLLLC_GI2_2P2Z_B3;
    #else
        CLLLC_gi.a1 = CLLLC_GI1_2P2Z_A1;
        CLLLC_gi.a2 = CLLLC_GI1_2P2Z_A2;
        CLLLC_gi.a3 = CLLLC_GI1_2P2Z_A3;
        CLLLC_gi.b0 = CLLLC_GI1_2P2Z_B0;
        CLLLC_gi.b1 = CLLLC_GI1_2P2Z_B1;
        CLLLC_gi.b2 = CLLLC_GI1_2P2Z_B2;
        CLLLC_gi.b3 = CLLLC_GI1_2P2Z_B3;
    #endif

    DCL_resetDF13(&CLLLC_gv);
    #if CLLLC_POWER_FLOW == CLLLC_POWER_FLOW_PRIM_SEC
        CLLLC_gv.a1 = CLLLC_GV1_2P2Z_A1;
        CLLLC_gv.a2 = CLLLC_GV1_2P2Z_A2;
        CLLLC_gv.a3 = CLLLC_GV1_2P2Z_A3;
        CLLLC_gv.b0 = CLLLC_GV1_2P2Z_B0;
        CLLLC_gv.b1 = CLLLC_GV1_2P2Z_B1;
        CLLLC_gv.b2 = CLLLC_GV1_2P2Z_B2;
        CLLLC_gv.b3 = CLLLC_GV1_2P2Z_B3;
    #elif CLLLC_POWER_FLOW == CLLLC_POWER_FLOW_SEC_PRIM
        CLLLC_gv.a1 = CLLLC_GV2_2P2Z_A1;
        CLLLC_gv.a2 = CLLLC_GV2_2P2Z_A2;
        CLLLC_gv.a3 = CLLLC_GV2_2P2Z_A3;
        CLLLC_gv.b0 = CLLLC_GV2_2P2Z_B0;
        CLLLC_gv.b1 = CLLLC_GV2_2P2Z_B1;
        CLLLC_gv.b2 = CLLLC_GV2_2P2Z_B2;
        CLLLC_gv.b3 = CLLLC_GV2_2P2Z_B3;
    #endif

    DLOG_4CH_reset(&CLLLC_dLog1);
    DLOG_4CH_config(&CLLLC_dLog1,
                    &CLLLC_dVal1, &CLLLC_dVal2, &CLLLC_dVal3, &CLLLC_dVal4,
                    CLLLC_dBuff1, CLLLC_dBuff2, CLLLC_dBuff3, CLLLC_dBuff4,
                    100, 0.5, 1);

    CLLLC_dlogTrigger = 0;

    EMAVG_reset(&CLLLC_iSecSensedAvg_pu);
    EMAVG_config(&CLLLC_iSecSensedAvg_pu, 0.01f);

    EMAVG_reset(&CLLLC_iPrimSensedAvg_pu);
    EMAVG_config(&CLLLC_iPrimSensedAvg_pu, 0.01f);

    EMAVG_reset(&CLLLC_iPrimTankSensedAvg_pu);
    EMAVG_config(&CLLLC_iPrimTankSensedAvg_pu, 0.01f);

    EMAVG_reset(&CLLLC_vPrimSensedAvg_pu);
    EMAVG_config(&CLLLC_vPrimSensedAvg_pu, 0.01f);

    EMAVG_reset(&CLLLC_vSecSensedAvg_pu);
    EMAVG_config(&CLLLC_vSecSensedAvg_pu, 0.01f);

    CLLLC_iPrimSensed_Amps = 0;
    CLLLC_unusedAdca5SensedRaw = 0;
    CLLLC_iTankSensedRaw = 0;
    CLLLC_iTankModSensedRaw = 0;
    CLLLC_iTankCmpssStatus = 0;
    CLLLC_iTankCmpssHigh = 0;
    CLLLC_iTankCmpssHighLatched = 0;
    CLLLC_iTankCmpssLow = 0;
    CLLLC_iTankCmpssLowLatched = 0;
    CLLLC_iTankCmpssDebugCmpssNo = 2;
    CLLLC_iTankCmpssDebugMuxValue = 1;
    CLLLC_iTankCmpssDebugAppliedCmpssNo = 2;
    CLLLC_iTankCmpssDebugAppliedMuxValue = 1;
    CLLLC_iTankCmpssDebugDacHigh = CLLLC_IPRIM_TANK_DACHVAL;
    CLLLC_iTankCmpssDebugDacLow = CLLLC_IPRIM_TANK_DACLVAL;
    CLLLC_iTankCmpssDebugAppliedDacHigh = CLLLC_IPRIM_TANK_DACHVAL;
    CLLLC_iTankCmpssDebugAppliedDacLow = CLLLC_IPRIM_TANK_DACLVAL;
    CLLLC_iTankCmpssDebugResetCounts = 0;
    CLLLC_iTankCmpssState = 0;
    CLLLC_iTankCmpssLastState = 0xFFFFU;
    CLLLC_iTankSensedRawMin = 0xFFFFU;
    CLLLC_iTankSensedRawMax = 0;
    CLLLC_iTankModSensedRawMin = 0xFFFFU;
    CLLLC_iTankModSensedRawMax = 0;
    CLLLC_iTankCmpssSampleCount = 0;
    CLLLC_iTankCmpssHighOnlyCount = 0;
    CLLLC_iTankCmpssLowOnlyCount = 0;
    CLLLC_iTankCmpssBothCount = 0;
    CLLLC_iTankCmpssNeitherCount = 0;
    CLLLC_iTankCmpssTransitionCount = 0;
    CLLLC_unusedAdca5Sensed_pu = 0.0f;
    CLLLC_iTankSensed_pu = 0.0f;
    CLLLC_iTankModSensed_pu = 0.0f;
    CLLLC_iTankModAmpsPerPu = CLLLC_ITANK_MOD_MAX_SENSE_AMPS;
    CLLLC_iTankModLimit_Amps = 5.0f;
    CLLLC_unusedAdca5SensedOffset_pu = 0.0f;
    CLLLC_iTankSensedOffset_pu = 0.5f;
    CLLLC_iTankModSensedOffset_pu = 0.120f / 3.3f;
    CLLLC_vPrimSensed_Volts = 0;
    CLLLC_iSecSensed_Amps = 0;
    CLLLC_vSecSensed_Volts = 0;

    CLLLC_vSecRef_Volts = CLLLC_VSEC_NOMINAL_VOLTS;
    CLLLC_vSecRef_pu = CLLLC_VSEC_NOMINAL_VOLTS /
                       CLLLC_VSEC_OPTIMAL_RANGE_VOLTS;
    CLLLC_vSecRefSlewed_pu = 0;

    CLLLC_iSecRef_Amps = 9.7f;
    CLLLC_vPrimRef_Volts = CLLLC_VPRIM_NOMINAL_VOLTS;
    CLLLC_vPrimRef_pu = CLLLC_VPRIM_NOMINAL_VOLTS /
                        CLLLC_VPRIM_MAX_SENSE_VOLTS;

    CLLLC_pwmPeriod_pu = (CLLLC_MIN_PWM_SWITCHING_FREQUENCY_HZ /
                          CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ);
    //
    // CLLLC_pwmPeriodSlewed_pu is written with a slightly different value
    // to trigger the PWM update ISR
    //
    CLLLC_pwmPeriodSlewed_pu = CLLLC_pwmPeriod_pu - 0.1f;
    CLLLC_pwmPeriodRef_pu = CLLLC_pwmPeriod_pu;
    CLLLC_pwmPeriodMin_pu = (CLLLC_MIN_PWM_SWITCHING_FREQUENCY_HZ /
                            CLLLC_MAX_PWM_SWITCHING_FREQUENCY_HZ);
    CLLLC_pwmPeriodMax_ticks = CLLLC_PWMSYSCLOCK_FREQ_HZ /
                                CLLLC_MIN_PWM_SWITCHING_FREQUENCY_HZ;

    CLLLC_pwmFrequency_Hz = CLLLC_NOMINAL_PWM_SWITCHING_FREQUENCY_HZ;
    CLLLC_pwmFrequencyPrev_Hz = CLLLC_pwmFrequency_Hz - 1.0f;

    CLLLC_pwmPhaseShiftPrimSec_ns = 81;
    CLLLC_pwmPhaseShiftPrimSecRef_ns = 150;


    CLLLC_pwmDeadBandREDPrimRef_ns = CLLLC_PRIM_PWM_DEADBAND_RED_NS;
    CLLLC_pwmDeadBandFEDPrimRef_ns = CLLLC_PRIM_PWM_DEADBAND_FED_NS;

    CLLLC_modeChangeFlag = 0;

    CLLLC_iPrimSensed_pu = 0;
    CLLLC_iPrimTankSensed_pu = 0;
    CLLLC_iPrimSensedOffset_pu = 0.5f;
    CLLLC_iPrimTankSensedOffset_pu = 0.5f;
    CLLLC_vPrimSensed_pu = 0;
    CLLLC_vPrimSensedOffset_pu = 0;
    CLLLC_iSecSensed_pu = 0;
    CLLLC_iSecSensedOffset_pu = 0.5f;
    CLLLC_vSecSensed_pu = 0;
    CLLLC_vSecSensedOffset_pu = 0;

    if(CLLLC_powerFlowState.CLLLC_PowerFlowState_Enum ==
            powerFlow_PrimToSec)
    {
        CLLLC_pwmDutyPrim_pu = 0.5f;
        CLLLC_pwmDutyPrimRef_pu = 0.5f;
        CLLLC_pwmDutySec_pu = 0.45f;
        CLLLC_pwmDutySecRef_pu = 0.45f;
    }
    else if(CLLLC_powerFlowState.CLLLC_PowerFlowState_Enum ==
            powerFlow_SecToPrim)
    {
        CLLLC_pwmDutyPrim_pu = 0.45f;
        CLLLC_pwmDutyPrimRef_pu = 0.45f;
        CLLLC_pwmDutySec_pu = 0.5f;
        CLLLC_pwmDutySecRef_pu = 0.5f;
    }

    CLLLC_iSecSensedCalIntercept_pu = 0.0;
    CLLLC_iSecSensedCalXvariable_pu = 1.0f;

    CLLLC_iPrimSensedCalIntercept_pu = 0.0;
    CLLLC_iPrimSensedCalXvariable_pu = 1.0f;

    CLLLC_iPrimTankSensedCalIntercept_pu = 0.0;
    CLLLC_iPrimTankSensedCalXvariable_pu = 1.0f;

    CLLLC_pwmSwState.CLLLC_PwmSwState_Enum =
            pwmSwState_synchronousRectification_active;
    CLLLC_pwmSwStateActive.CLLLC_PwmSwState_Enum =
            pwmSwState_synchronousRectification_active;

    CLLLC_commandSentTo_AC_DC.CLLLC_CommandSentTo_AC_DC_Enum = ac_dc_OFF;
    CLLLC_tripFlag.CLLLC_TripFlag_Enum = noTrip;

    CLLLC_slewSCIcommand = 0;
    CLLLC_vPrimRef_Volts = 400;

    CLLLC_closeGiLoop = 0;
    CLLLC_closeGvLoop = 0;
    CLLLC_clearTrip = 0;
    CLLLC_hfcReceiverTestEnable = 0;
    CLLLC_hfcReceiverTestRun = 0;
    CLLLC_hfcReceiverTestActive = 0;
    CLLLC_hfcReceiverTestMode = CLLLC_HFC_RECEIVER_TEST_MODE_MONITOR_ONLY;
    CLLLC_hfcReceiverTestSyncEnable = 0;
    CLLLC_hfcReceiverTestSyncConfigured = 0;
    CLLLC_hfcReceiverTestReady = 0;
    CLLLC_hfcReceiverTestInhibit = 0;
    CLLLC_hfcReceiverTestMinITankModRaw = 0;
    CLLLC_hfcReceiverTestDuty_pu = 0.1f;
    CLLLC_hfcReceiverTestPhaseShiftSeq = 0U;
    CLLLC_publishHfcReceiverTestPhaseShift(0.0f);

//    CLLLC_cla_task_counter = 0;

    CLLLC_ModulationMode.CLLLC_ModulationMode_Enum = freqMode;

}

void CLLLC_updateBoardStatus(void)
{
    int16_t tripStatusRead;
    tripStatusRead = CLLLC_HAL_readTripFlags();

    if(CLLLC_tripFlag.CLLLC_TripFlag_Enum == noTrip)
    {
        if(tripStatusRead == (int16_t)primOverCurrentTrip)
        {
            CLLLC_tripFlag.CLLLC_TripFlag_Enum = primOverCurrentTrip;
        }
        else if(tripStatusRead == (int16_t)secOverCurrentTrip)
        {
            CLLLC_tripFlag.CLLLC_TripFlag_Enum = secOverCurrentTrip;
        }
        else if(tripStatusRead == (int16_t)primTankOverCurrentTrip)
        {
            CLLLC_tripFlag.CLLLC_TripFlag_Enum = primTankOverCurrentTrip;
        }
        else if(tripStatusRead == (int16_t)secOverVoltageTrip)
        {
            CLLLC_tripFlag.CLLLC_TripFlag_Enum = secOverVoltageTrip;
        }
        else if(tripStatusRead == (int16_t)ganFaultTrip)
        {
            CLLLC_tripFlag.CLLLC_TripFlag_Enum = ganFaultTrip;
        }
    }
}

#if CLLLC_SFRA_TYPE != CLLLC_SFRA_DISABLED
void CLLLC_runSFRABackGroundTasks(void)
{

    SFRA_F32_runBackgroundTask(&CLLLC_sfra1);
    SFRA_GUI_runSerialHostComms(&CLLLC_sfra1);

}
#endif



void CLLLC_setBuildLevelIndicatorVariable(void)
{
    #if CLLLC_LAB == 1
        #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
           CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab1_CLA;
        #else
           CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab1;
        #endif
    #elif CLLLC_LAB == 2
        #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
           CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab2_CLA;
        #else
           CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab2;
        #endif
    #elif CLLLC_LAB == 3
        #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
           CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab3_CLA;
        #else
           CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab3;
        #endif
    #elif CLLLC_LAB == 4
        #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
           CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab4_CLA;
        #else
           CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab4;
        #endif
    #elif CLLLC_LAB == 5
        #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
           CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab5_CLA;
        #else
            CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab5;
        #endif
    #elif CLLLC_LAB == 6
        #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
           CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab6_CLA;
        #else
           CLLLC_Lab.CLLLC_Lab_Enum =
                    CLLLC_Lab6;
        #endif
    #elif CLLLC_LAB == 7
       #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
          CLLLC_Lab.CLLLC_Lab_Enum =
                   CLLLC_Lab7_CLA;
       #else
          CLLLC_Lab.CLLLC_Lab_Enum =
                   CLLLC_Lab7;
       #endif
    #elif CLLLC_LAB == 8
       #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
          CLLLC_Lab.CLLLC_Lab_Enum =
                  CLLLC_Lab8_CLA;
       #else
          CLLLC_Lab.CLLLC_Lab_Enum =
                  CLLLC_Lab8;
   #endif
   #else

           CLLLC_Lab.CLLLC_Lab_Enum = CLLLC_undefinedLab;
    #endif

//
//    #if CLLLC_INCR_BUILD == 1
//        #if CLLLC_SEC_CONNECTED_IN_BATTERY_EMULATION_MODE == 0
//            #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
//                CLLLC_buildLevel.CLLLC_BuildLevel_Enum = openLoopCheck_CLA;
//            #else
//                CLLLC_buildLevel.CLLLC_BuildLevel_Enum = openLoopCheck;
//            #endif
//        #else
//                CLLLC_buildLevel..CLLLC_BuildLevel_Enum = undefinedState;
//        #endif
//    #elif CLLLC_INCR_BUILD == 2
//        #if CLLLC_CONTROL_MODE == CLLLC_VOLTAGE_MODE
//            #if CLLLC_SEC_CONNECTED_IN_BATTERY_EMULATION_MODE == 0
//                #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
//                    CLLLC_buildLevel.CLLLC_BuildLevel_Enum =
//                            closedLoopCheck_Voltage_CLA;
//                #else
//                    CLLLC_buildLevel.CLLLC_BuildLevel_Enum =
//                            closedLoopCheck_Voltage;
//                #endif
//            #else
//                CLLLC_buildLevel.CLLLC_BuildLevel_Enum = undefinedState;
//            #endif
//
//        #elif CLLLC_CONTROL_MODE == CLLLC_CURRENT_MODE
//            #if CLLLC_SEC_CONNECTED_IN_BATTERY_EMULATION_MODE == 0
//                #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
//                    CLLLC_buildLevel.CLLLC_BuildLevel_Enum =
//                            closedLoopCheck_Current_CLA;
//                #else
//                    CLLLC_buildLevel.CLLLC_BuildLevel_Enum =
//                            closedLoopCheck_Current;
//                #endif
//            #else
//                #if CLLLC_CONTROL_RUNNING_ON == CLA_CORE
//                    CLLLC_buildLevel.CLLLC_BuildLevel_Enum =
//                            closedLoopCheck_Current_BatteryMode_CLA;
//                #else
//                    CLLLC_buildLevel.CLLLC_BuildLevel_Enum =
//                            closedLoopCheck_Current_BatteryMode;
//                #endif
//            #endif
//        #endif
//    #endif
//

}

void CLLLC_changeSynchronousRectifierPwmBehavior(uint16_t powerFlow)
{
    if(CLLLC_pwmSwState.CLLLC_PwmSwState_Enum !=
            CLLLC_pwmSwStateActive.CLLLC_PwmSwState_Enum)
    {

        if(CLLLC_pwmSwState.CLLLC_PwmSwState_Enum ==
                pwmSwState_synchronousRectification_fixedDuty)
        {
            CLLLC_HAL_resetSynchronousRectifierTripAction(powerFlow);

            CLLLC_pwmSwStateActive.CLLLC_PwmSwState_Enum =
                    pwmSwState_synchronousRectification_fixedDuty;

            //
            // the below is emperical value for this design
            // accounting for delay in digital isolators
            //

            CLLLC_pwmPhaseShiftPrimSecRef_ns = 81;
        }
        else if(CLLLC_pwmSwState.CLLLC_PwmSwState_Enum ==
                pwmSwState_synchronousRectification_active)
        {
            CLLLC_HAL_setupSynchronousRectifierTripAction(powerFlow);

            CLLLC_pwmSwStateActive.CLLLC_PwmSwState_Enum =
                    pwmSwState_synchronousRectification_active;

            //
            // CMPSS trip is cleared on PWM period and zero,
            // however this causes some delay in PWM coming out of trip
            // this is compensated by reducing the phase shift as needed
            // the below is emperical value for this design
            //
            CLLLC_pwmPhaseShiftPrimSecRef_ns = 81;
        }
        else
        {

            CLLLC_pwmSwStateActive.CLLLC_PwmSwState_Enum =
                    CLLLC_pwmSwState.CLLLC_PwmSwState_Enum;

           //
           // do nothing, simply change the IOs which is done in the
            // routine outside this if statement
           //
        }

        CLLLC_HAL_setupPWMpins(CLLLC_pwmSwStateActive.CLLLC_PwmSwState_Enum);
    }

}

#if CLLLC_SFRA_TYPE != CLLLC_SFRA_DISABLED
void CLLLC_setupSFRA(void)
{
    SFRA_F32_reset(&CLLLC_sfra1);
    SFRA_F32_config(&CLLLC_sfra1,
                    CLLLC_SFRA_ISR_FREQ_HZ,
                    CLLLC_SFRA_AMPLITUDE,
                    CLLLC_SFRA_FREQ_LENGTH,
                    CLLLC_SFRA_FREQ_START,
                    CLLLC_SFRA_FREQ_STEP_MULTIPLY,
                    CLLLC_plantMagVect,
                    CLLLC_plantPhaseVect,
                    CLLLC_olMagVect,
                    CLLLC_olPhaseVect,
                    NULL,
                    NULL,
                    CLLLC_freqVect,
                    1);

    SFRA_F32_resetFreqRespArray(&CLLLC_sfra1);

    SFRA_F32_initFreqArrayWithLogSteps(&CLLLC_sfra1,
                                       CLLLC_SFRA_FREQ_START,
                                       CLLLC_SFRA_FREQ_STEP_MULTIPLY);

    //
    //configures the SCI channel for communication with SFRA host GUI
    //to change SCI channel change #defines in the settings.h file
    //the GUI also changes a LED status, this can also be changed with #define
    //in the file pointed to above
    //
    SFRA_GUI_config(CLLLC_SFRA_GUI_SCI_BASE,
                    CLLLC_SCI_VBUS_CLK,
                    CLLLC_SFRA_GUI_SCI_BAUDRATE,
                    CLLLC_SFRA_GUI_SCIRX_GPIO,
                    CLLLC_SFRA_GUI_SCIRX_GPIO_PIN_CONFIG,
                    CLLLC_SFRA_GUI_SCITX_GPIO,
                    CLLLC_SFRA_GUI_SCITX_GPIO_PIN_CONFIG,
                    CLLLC_SFRA_GUI_LED_INDICATOR,
                    CLLLC_SFRA_GUI_LED_GPIO,
                    CLLLC_SFRA_GUI_LED_GPIO_PIN_CONFIG,
                    &CLLLC_sfra1,
                    1);


}
#endif

//
//===========================================================================
// No more.
//===========================================================================
//
