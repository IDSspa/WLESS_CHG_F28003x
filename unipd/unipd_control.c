#include "unipd_control.h"
#include "clllc.h"
#include "ttplpfc.h"
#include "wless_sm/wless_sm.h"

#define UNIPD_V_BAT_MAX_ALG        (218.0f)
#define UNIPD_V_BAT_MIN_ALG        (37.5f)
#define UNIPD_V_DC_MAX_ALG         (500.0f)
#define UNIPD_V_DC_MIN_ALG         (62.5f)

#ifndef UNIPD_LOW_VOLTAGE_TEST_LIMITS
#define UNIPD_LOW_VOLTAGE_TEST_LIMITS 1
#endif

#if UNIPD_LOW_VOLTAGE_TEST_LIMITS
#define UNIPD_V_BAT_MIN_TEST_ALG   (1.0f)
#define UNIPD_V_DC_MIN_TEST_ALG    (1.0f)
#else
#define UNIPD_V_BAT_MIN_TEST_ALG   UNIPD_V_BAT_MIN_ALG
#define UNIPD_V_DC_MIN_TEST_ALG    UNIPD_V_DC_MIN_ALG
#endif
#define UNIPD_PI                   (3.14159265358979323846f)
#define UNIPD_FOUR_OVER_PI         (4.0f / UNIPD_PI)

#define UNIPD_K_I_L_ERR            (0.533694299666651f)
#define UNIPD_K_I_L_ERR_P          (-0.526383425589328f)

#define UNIPD_K_V_DC_ERR           (0.046777788128766f)
#define UNIPD_K_V_DC_ERR_P         (-0.046768069962416f)

#define UNIPD_K_I_COIL_ERR         (0.002436907237795f)
#define UNIPD_K_I_COIL_ERR_P       (0.004873814475590f)
#define UNIPD_K_I_COIL_ERR_PP      (0.002436907237795f)
#define UNIPD_K_V_COIL_RIF_P       (1.901603650787137f)
#define UNIPD_K_V_COIL_RIF_PP      (-0.901603650787137f)

#define UNIPD_I_COIL_LOC_LIM       (50.0f)
#define UNIPD_K_I_COIL_LOC_ERR     (0.436953635294118f)
#define UNIPD_K_I_COIL_LOC_ERR_P   (-0.406566364705882f)
#define UNIPD_K_V_COIL_LIM_P       (1.0f)

/*
 * Param_Contr(12..17), generated from p_schema_completo_generale_v2.m
 * (nominal Vdc=250 V, Cdc=1.5 mF, fs=85 kHz, Ts=4/fs, Tustin).
 * Difference equation: y[k] = A1*y[k-1] + B0*e[k] + B1*e[k-1].
 */
#define UNIPD_RX_PTRASF_A1          (1.0f)
#define UNIPD_RX_PTRASF_B0          (0.014046111269511f)
#define UNIPD_RX_PTRASF_B1          (-0.014044685364921f)
#define UNIPD_TX_PTRASF_A1          (1.0f)
#define UNIPD_TX_PTRASF_B0          (0.014071403798379f)
#define UNIPD_TX_PTRASF_B1          (-0.014070190824372f)

#define UNIPD_PWM_ENABLE_DELAY     (100U)

static UNIPD_DcBusControlState unipd_dc_bus_state;
static UNIPD_RemoteCoilLimitControlState unipd_remote_coil_limit_state;
static UNIPD_TransferredPowerControlState unipd_rx_transferred_power_state;
static UNIPD_TransferredPowerControlState unipd_tx_transferred_power_state;
static UNIPD_DcBusCoilControlState unipd_dc_bus_coil_state;

UNIPD_BbcIntegrationInputs UNIPD_bbcInputs;
UNIPD_DcBusCoilControlOutput UNIPD_bbcOutput;
unsigned int UNIPD_bbcSignalValidMask;
unsigned int UNIPD_bbcSignalMissingMask = UNIPD_BBC_REQUIRED_SIGNAL_MASK;
volatile float UNIPD_vBatMinAlg_Volts = UNIPD_V_BAT_MIN_TEST_ALG;
volatile float UNIPD_vBatMaxAlg_Volts = UNIPD_V_BAT_MAX_ALG;
volatile float UNIPD_vDcMinAlg_Volts = UNIPD_V_DC_MIN_TEST_ALG;
volatile float UNIPD_vDcMaxAlg_Volts = UNIPD_V_DC_MAX_ALG;
volatile float UNIPD_vdcControllerGainScale = 1.0f;
volatile unsigned int UNIPD_bbcCurrentPolarityMask;
volatile float UNIPD_bbcILRawA_Amps;
volatile float UNIPD_bbcILRawB_Amps;
volatile unsigned int UNIPD_bbcPowerOutputEnable;
volatile float UNIPD_bbcPowerOutputDutyMax_pu = 0.35f;
volatile unsigned int UNIPD_bbcDutyMappingMode = 1U;
volatile float UNIPD_bbcMappedDutyA_pu;
volatile float UNIPD_bbcMappedDutyB_pu;
volatile float UNIPD_bbcAppliedDutyA_pu;
volatile float UNIPD_bbcAppliedDutyB_pu;
volatile unsigned int UNIPD_bbcPowerOutputDutyRampEnable = 1U;
volatile float UNIPD_bbcPowerOutputDutyRampStep_pu = 0.00002f;
volatile float UNIPD_bbcRampedDutyA_pu;
volatile float UNIPD_bbcRampedDutyB_pu;
volatile unsigned int UNIPD_resetControlStatesCommand;
volatile unsigned int UNIPD_bbcVdcTripEnable = 1U;
volatile float UNIPD_bbcVdcTripThreshold_Volts = 10.0f;
volatile unsigned int UNIPD_bbcVdcTripLatched;
volatile float UNIPD_bbcVdcTripCapture_Volts;
volatile unsigned int UNIPD_bbcVdcTripResetCommand;
volatile unsigned int UNIPD_bbcILTripEnable = 1U;
volatile float UNIPD_bbcILTripThreshold_Amps = 0.75f;
volatile unsigned int UNIPD_bbcILTripLatched;
volatile float UNIPD_bbcILTripCaptureA_Amps;
volatile float UNIPD_bbcILTripCaptureB_Amps;
static unsigned int UNIPD_bbcILTripConfirmCount;
volatile unsigned int UNIPD_bbcILTripConfirmCycles = 22U;
volatile float UNIPD_bbcDebugILerrA;
volatile float UNIPD_bbcDebugILerrB;
volatile float UNIPD_bbcDebugVLrifMin;
volatile float UNIPD_bbcDebugVLrifMax;
volatile float UNIPD_bbcDebugVLrifA;
volatile float UNIPD_bbcDebugVLrifB;
volatile float UNIPD_bbcDebugDutyRawA;
volatile float UNIPD_bbcDebugDutyRawB;
volatile unsigned int UNIPD_bbcSyntheticTestEnable;
volatile unsigned int UNIPD_bbcSyntheticValidMask = UNIPD_BBC_REQUIRED_SIGNAL_MASK;
volatile unsigned int UNIPD_bbcSyntheticOverrideMask =
        UNIPD_BBC_REQUIRED_SIGNAL_MASK &
        ~(UNIPD_BBC_SIGNAL_V_DC |
          UNIPD_BBC_SIGNAL_I_L_A |
          UNIPD_BBC_SIGNAL_I_L_B |
          UNIPD_BBC_SIGNAL_I_COIL_LOC);
volatile UNIPD_BbcIntegrationInputs UNIPD_bbcSyntheticInputs =
{
    300.0f,
    96.0f,
    0.0f,
    0.0f,
    10.0f,
    5.0f,
    12.0f,
    320.0f,
    5.0f,
    -5.0f,
    1U,
    UNIPD_BBC_REQUIRED_SIGNAL_MASK
};

volatile unsigned int UNIPD_bbcCaptureArmed;
volatile unsigned int UNIPD_bbcCaptureFrozen;
volatile unsigned int UNIPD_bbcCaptureCount;
volatile unsigned int UNIPD_bbcCaptureTriggerReason;
static volatile unsigned int UNIPD_bbcCaptureWriteIndex;
static volatile unsigned int UNIPD_bbcCaptureDecimationCount;
static UNIPD_BbcCaptureSample
        UNIPD_bbcCaptureBuffer[UNIPD_BBC_CAPTURE_LENGTH];
volatile unsigned int UNIPD_wptCaptureArmed;
volatile unsigned int UNIPD_wptCaptureFrozen;
volatile unsigned int UNIPD_wptCaptureCount;
volatile unsigned int UNIPD_wptCaptureDecimation =
        UNIPD_WPT_CAPTURE_DECIMATION_DEFAULT;
static volatile unsigned int UNIPD_wptCaptureWriteIndex;
static volatile unsigned int UNIPD_wptCaptureDecimationCount;
#pragma DATA_SECTION(UNIPD_wptCaptureBuffer, "wptCapture")
static UNIPD_WptCaptureSample
        UNIPD_wptCaptureBuffer[UNIPD_WPT_CAPTURE_LENGTH];

static float unipd_clampf(float value, float min_value, float max_value);
static void unipd_captureWptCycle(void);

volatile unsigned int UNIPD_wptIntegrationEnable;
volatile float UNIPD_wptVdcSourceRef_Volts = 12.0f;
volatile float UNIPD_wptVdcLoadRef_Volts = 12.0f;
volatile float UNIPD_wptCoilCurrentMax_Amps = 40.0f;
volatile float UNIPD_wptCoilCurrentMin_Amps;
volatile float UNIPD_wptTxPowerLimit_Watts;
volatile float UNIPD_wptRemotePowerLimit_Watts;
volatile float UNIPD_wptRemoteCoilErr_Amps;
volatile unsigned int UNIPD_wptTxPowerSeedPending;
volatile float UNIPD_wptTxPowerSeed_Watts;
volatile unsigned int UNIPD_wptLoadPowerSeedPending;
volatile float UNIPD_wptLoadPowerSeed_Watts;
volatile unsigned int UNIPD_wptSourceCoilSyntheticEnable;
volatile float UNIPD_wptSourceCoilSynthetic_Amps;
volatile unsigned int UNIPD_wptLoadCoilSyntheticEnable;
volatile float UNIPD_wptLoadCoilSynthetic_Amps;
volatile float UNIPD_wptLoadCoilOffset_Amps;
volatile float UNIPD_wptLocalCoilPhysical_Amps;
volatile float UNIPD_wptLocalCoilUsed_Amps;
volatile unsigned int UNIPD_wptHfcActuatorEnable;
volatile unsigned int UNIPD_wptHfcActuatorFault;
volatile unsigned int UNIPD_wptHfcManualPhaseEnable;
volatile float UNIPD_wptHfcManualPhase_pu = 0.25f;
volatile uint32_t UNIPD_wptHfcRemoteRoleInvalidCycles;
volatile float UNIPD_wptHfcPhaseMax_pu = 0.005f;
volatile float UNIPD_wptHfcPhaseRampStep_pu = 0.000001f;
volatile float UNIPD_wptHfcPhaseRequested_pu;
volatile float UNIPD_wptHfcPhaseApplied_pu;
volatile float UNIPD_wptHfcAutoHardwarePhase_pu = 0.5f;
volatile float UNIPD_wptHfcHardwarePhase_pu = 0.5f;
volatile float UNIPD_wptHfcPhasePeak_pu;
volatile float UNIPD_wptHfcPhaseLast_pu;
volatile int32_t UNIPD_wptHfcPhaseTicksLast;
volatile uint32_t UNIPD_wptHfcActiveCycles;
volatile uint32_t UNIPD_wptHfcEpwm1TbphsRawLast;
volatile uint32_t UNIPD_wptHfcEpwm2TbphsRawLast;
volatile uint16_t UNIPD_wptHfcEpwm1TbprdLast;
volatile uint16_t UNIPD_wptHfcEpwm2TbprdLast;
volatile uint16_t UNIPD_wptHfcEpwm1TbctlLast;
volatile uint16_t UNIPD_wptHfcEpwm2TbctlLast;
volatile float UNIPD_wptHfcFaultVdc_Volts;
UNIPD_RxTransferredPowerControlOutput UNIPD_wptRxOutput;
UNIPD_RemoteCoilLimitControlOutput UNIPD_wptHfcOutput;

static int16_t unipd_encodeSigned16(float value)
{
    value = unipd_clampf(value, -32768.0f, 32767.0f);
    value += (value >= 0.0f) ? 0.5f : -0.5f;
    return (int16_t)value;
}

void UNIPD_disableWptHfcActuator(void)
{
    UNIPD_wptHfcActuatorEnable = 0U;
    UNIPD_wptHfcPhaseRequested_pu = 0.0f;
    UNIPD_wptHfcPhaseApplied_pu = 0.0f;
    UNIPD_wptHfcAutoHardwarePhase_pu = 0.5f;
    UNIPD_wptHfcHardwarePhase_pu = 0.5f;
    CLLLC_hfcReceiverTestRun = 0U;
    CLLLC_hfcReceiverTestEnable = 0U;
    CLLLC_hfcReceiverTestPhaseShiftPrimLegs_pu = 0.0f;
    CLLLC_pwmPhaseShiftPrimLegsRef_pu = 0.0f;
    CLLLC_pwmPhaseShiftPrimLegs_pu = 0.0f;
    CLLLC_FORCE_PWM_OST_TRIP(CLLLC_PRIM_LEG1_PWM_BASE);
    CLLLC_FORCE_PWM_OST_TRIP(CLLLC_PRIM_LEG2_PWM_BASE);
}

static void unipd_updateWptHfcActuator(unsigned int role)
{
    float requested;
    float hardwarePhase;
    unsigned int fault = 0U;

    if(UNIPD_wptHfcActuatorEnable == 0U)
    {
        return;
    }

    if(WLESS_SM_remoteRole == WLESS_SM_ROLE_LOAD)
    {
        UNIPD_wptHfcRemoteRoleInvalidCycles = 0UL;
    }
    else if(UNIPD_wptHfcRemoteRoleInvalidCycles <
            (uint32_t)(CLLLC_ISR2_FREQUENCY_HZ / 10U))
    {
        UNIPD_wptHfcRemoteRoleInvalidCycles++;
    }

    if(UNIPD_wptIntegrationEnable == 0U)
    {
        fault = 1U;
    }
    else if(role != (unsigned int)WLESS_SM_ROLE_SOURCE)
    {
        fault = 2U;
    }
    else if(WLESS_SM_radioLink != WLESS_SM_LINK_OK)
    {
        fault = 4U;
    }
    else if(WLESS_SM_noAckCount >= 15U)
    {
        fault = 5U;
    }
    else if((WLESS_SM_state != WLESS_SM_STATE_PRECHARGE_SOURCE) &&
            (WLESS_SM_state != WLESS_SM_STATE_SOURCE_ON))
    {
        fault = 6U;
    }
    if(fault != 0U)
    {
        UNIPD_wptHfcActuatorFault = fault;
        UNIPD_wptHfcPhaseLast_pu = UNIPD_wptHfcPhaseApplied_pu;
        UNIPD_wptHfcPhaseTicksLast = CLLLC_pwmPhaseShiftPrimLegs_ticks;
        UNIPD_wptHfcFaultVdc_Volts = UNIPD_bbcInputs.v_dc;
        UNIPD_disableWptHfcActuator();
        return;
    }

    requested = fabsf(UNIPD_wptHfcOutput.duty_cycle_ps_a - 0.5f);
    requested = unipd_clampf(requested, 0.0f, UNIPD_wptHfcPhaseMax_pu);
    UNIPD_wptHfcPhaseRequested_pu = requested;

    if(UNIPD_wptHfcPhaseApplied_pu < requested)
    {
        UNIPD_wptHfcPhaseApplied_pu += UNIPD_wptHfcPhaseRampStep_pu;
        if(UNIPD_wptHfcPhaseApplied_pu > requested)
        {
            UNIPD_wptHfcPhaseApplied_pu = requested;
        }
    }
    else if(UNIPD_wptHfcPhaseApplied_pu > requested)
    {
        UNIPD_wptHfcPhaseApplied_pu -= UNIPD_wptHfcPhaseRampStep_pu;
        if(UNIPD_wptHfcPhaseApplied_pu < requested)
        {
            UNIPD_wptHfcPhaseApplied_pu = requested;
        }
    }

    /*
     * UniPD expresses the differential modulation as the deviation d from
     * PSA/PSB = 0.5.  Bench characterization of the retained TI up-down PWM
     * driver established 0.5 pu as the physical inter-leg neutral point and
     * 0.0 pu as full differential excitation.  Therefore phase_TI = 0.5 - d.
     */
    if(UNIPD_wptHfcManualPhaseEnable != 0U)
    {
        hardwarePhase = unipd_clampf(UNIPD_wptHfcManualPhase_pu,
                                     0.0f, 0.5f);
    }
    else
    {
        hardwarePhase = 0.5f - UNIPD_wptHfcPhaseApplied_pu;
    }
    UNIPD_wptHfcAutoHardwarePhase_pu =
            0.5f - UNIPD_wptHfcPhaseApplied_pu;
    UNIPD_wptHfcHardwarePhase_pu = hardwarePhase;
    CLLLC_hfcReceiverTestPhaseShiftPrimLegs_pu = hardwarePhase;
    UNIPD_wptHfcPhaseLast_pu = UNIPD_wptHfcPhaseApplied_pu;
    if(UNIPD_wptHfcPhaseApplied_pu > UNIPD_wptHfcPhasePeak_pu)
    {
        UNIPD_wptHfcPhasePeak_pu = UNIPD_wptHfcPhaseApplied_pu;
    }
    UNIPD_wptHfcPhaseTicksLast = CLLLC_pwmPhaseShiftPrimLegs_ticks;
    UNIPD_wptHfcEpwm1TbphsRawLast =
            HWREG(CLLLC_PRIM_LEG1_PWM_BASE + HRPWM_O_TBPHS);
    UNIPD_wptHfcEpwm2TbphsRawLast =
            HWREG(CLLLC_PRIM_LEG2_PWM_BASE + HRPWM_O_TBPHS);
    UNIPD_wptHfcEpwm1TbprdLast =
            HWREGH(CLLLC_PRIM_LEG1_PWM_BASE + EPWM_O_TBPRD);
    UNIPD_wptHfcEpwm2TbprdLast =
            HWREGH(CLLLC_PRIM_LEG2_PWM_BASE + EPWM_O_TBPRD);
    UNIPD_wptHfcEpwm1TbctlLast =
            HWREGH(CLLLC_PRIM_LEG1_PWM_BASE + EPWM_O_TBCTL);
    UNIPD_wptHfcEpwm2TbctlLast =
            HWREGH(CLLLC_PRIM_LEG2_PWM_BASE + EPWM_O_TBCTL);
    UNIPD_wptHfcActiveCycles++;
}

static void unipd_clearTransferredPowerIntegration(void)
{
    UNIPD_resetTransferredPowerControl(&unipd_rx_transferred_power_state);
    UNIPD_resetTransferredPowerControl(&unipd_tx_transferred_power_state);
    UNIPD_resetRemoteCoilLimitControl(&unipd_remote_coil_limit_state);
    UNIPD_wptTxPowerLimit_Watts = 0.0f;
    UNIPD_wptRemotePowerLimit_Watts = 0.0f;
    UNIPD_wptRemoteCoilErr_Amps = 0.0f;
    UNIPD_wptRxOutput.p_trasf_rif = 0.0f;
    UNIPD_wptRxOutput.i_coil_rif = 0.0f;
    UNIPD_wptRxOutput.i_coil_err = 0.0f;
    UNIPD_wptHfcOutput.duty_cycle_ps_a = 0.5f;
    UNIPD_wptHfcOutput.duty_cycle_ps_b = 0.5f;
    UNIPD_wptHfcOutput.v_ac_rif = 0.0f;
    UNIPD_wptHfcOutput.v_ac_rif_lim = 0.0f;
}

void UNIPD_armBbcCapture(void)
{
    UNIPD_bbcCaptureArmed = 1U;
    UNIPD_bbcCaptureFrozen = 0U;
    UNIPD_bbcCaptureCount = 0U;
    UNIPD_bbcCaptureTriggerReason = 0U;
    UNIPD_bbcCaptureWriteIndex = 0U;
    UNIPD_bbcCaptureDecimationCount = 0U;
}

void UNIPD_stopBbcCapture(void)
{
    UNIPD_bbcCaptureArmed = 0U;
    UNIPD_bbcCaptureFrozen = 1U;
}

unsigned int UNIPD_getBbcCaptureSample(unsigned int chronologicalIndex,
                                       UNIPD_BbcCaptureSample *sample)
{
    unsigned int physicalIndex;
    unsigned int startIndex;

    if((sample == 0) || (chronologicalIndex >= UNIPD_bbcCaptureCount))
    {
        return 0U;
    }

    startIndex = (UNIPD_bbcCaptureCount < UNIPD_BBC_CAPTURE_LENGTH) ?
            0U : UNIPD_bbcCaptureWriteIndex;
    physicalIndex = startIndex + chronologicalIndex;
    if(physicalIndex >= UNIPD_BBC_CAPTURE_LENGTH)
    {
        physicalIndex -= UNIPD_BBC_CAPTURE_LENGTH;
    }
    *sample = UNIPD_bbcCaptureBuffer[physicalIndex];
    return 1U;
}

void UNIPD_armWptCapture(void)
{
    UNIPD_wptCaptureArmed = 1U;
    UNIPD_wptCaptureFrozen = 0U;
    UNIPD_wptCaptureCount = 0U;
    UNIPD_wptCaptureWriteIndex = 0U;
    UNIPD_wptCaptureDecimationCount = 0U;
}

void UNIPD_stopWptCapture(void)
{
    UNIPD_wptCaptureArmed = 0U;
    UNIPD_wptCaptureFrozen = 1U;
}

unsigned int UNIPD_getWptCaptureSample(unsigned int chronologicalIndex,
                                       UNIPD_WptCaptureSample *sample)
{
    unsigned int physicalIndex;
    unsigned int startIndex;

    if((sample == 0) || (chronologicalIndex >= UNIPD_wptCaptureCount))
    {
        return 0U;
    }

    startIndex = (UNIPD_wptCaptureCount < UNIPD_WPT_CAPTURE_LENGTH) ?
            0U : UNIPD_wptCaptureWriteIndex;
    physicalIndex = startIndex + chronologicalIndex;
    if(physicalIndex >= UNIPD_WPT_CAPTURE_LENGTH)
    {
        physicalIndex -= UNIPD_WPT_CAPTURE_LENGTH;
    }
    *sample = UNIPD_wptCaptureBuffer[physicalIndex];
    return 1U;
}

static void unipd_captureWptCycle(void)
{
    UNIPD_WptCaptureSample *sample;

    if((UNIPD_wptCaptureArmed == 0U) ||
       (UNIPD_wptCaptureFrozen != 0U))
    {
        return;
    }

    UNIPD_wptCaptureDecimationCount++;
    if(UNIPD_wptCaptureDecimationCount < UNIPD_wptCaptureDecimation)
    {
        return;
    }
    UNIPD_wptCaptureDecimationCount = 0U;

    sample = &UNIPD_wptCaptureBuffer[UNIPD_wptCaptureWriteIndex];
    sample->tick = WLESS_SM_tickCounter;
    sample->v_dc = UNIPD_bbcInputs.v_dc;
    sample->i_coil_physical = UNIPD_wptLocalCoilPhysical_Amps;
    sample->i_coil_used = UNIPD_wptLocalCoilUsed_Amps;
    sample->p_ref = UNIPD_wptRxOutput.p_trasf_rif;
    sample->i_ref = UNIPD_wptRxOutput.i_coil_rif;
    sample->i_err = UNIPD_wptRxOutput.i_coil_err;
    sample->v_ac_ref = UNIPD_wptHfcOutput.v_ac_rif_lim;
    sample->hfc_request = UNIPD_wptHfcPhaseRequested_pu;
    sample->hfc_applied = UNIPD_wptHfcPhaseApplied_pu;
    sample->hfc_auto_hardware = UNIPD_wptHfcAutoHardwarePhase_pu;
    sample->hfc_physical = UNIPD_wptHfcHardwarePhase_pu;
    sample->role = (unsigned int)WLESS_SM_localRole;
    sample->state = (unsigned int)WLESS_SM_state;
    sample->link = (unsigned int)WLESS_SM_radioLink;
    sample->hfc_fault = UNIPD_wptHfcActuatorFault;
    sample->manual = UNIPD_wptHfcManualPhaseEnable;

    UNIPD_wptCaptureWriteIndex++;
    if(UNIPD_wptCaptureWriteIndex >= UNIPD_WPT_CAPTURE_LENGTH)
    {
        UNIPD_wptCaptureWriteIndex = 0U;
    }
    if(UNIPD_wptCaptureCount < UNIPD_WPT_CAPTURE_LENGTH)
    {
        UNIPD_wptCaptureCount++;
    }
}

static void unipd_captureBbcCycle(void)
{
    UNIPD_BbcCaptureSample *sample;
    unsigned int triggerReason = 0U;

    if((UNIPD_bbcVdcTripLatched != 0U) ||
       (UNIPD_bbcILTripLatched != 0U))
    {
        triggerReason = ((UNIPD_bbcVdcTripLatched != 0U) ? 1U : 0U) |
                        ((UNIPD_bbcILTripLatched != 0U) ? 2U : 0U);
    }

    if((UNIPD_bbcCaptureArmed == 0U) ||
       (UNIPD_bbcCaptureFrozen != 0U))
    {
        return;
    }

    UNIPD_bbcCaptureDecimationCount++;
    if((UNIPD_bbcCaptureDecimationCount < UNIPD_BBC_CAPTURE_DECIMATION) &&
       (triggerReason == 0U))
    {
        return;
    }
    UNIPD_bbcCaptureDecimationCount = 0U;

    sample = &UNIPD_bbcCaptureBuffer[UNIPD_bbcCaptureWriteIndex];
    sample->v_dc = UNIPD_bbcInputs.v_dc;
    sample->v_bat = UNIPD_bbcInputs.v_bat;
    sample->i_l_a = UNIPD_bbcInputs.i_l_a;
    sample->i_l_b = UNIPD_bbcInputs.i_l_b;
    sample->i_l_rif_a = UNIPD_bbcOutput.i_l_rif_a;
    sample->i_l_rif_b = UNIPD_bbcOutput.i_l_rif_b;
    sample->i_l_err_a = UNIPD_bbcDebugILerrA;
    sample->i_l_err_b = UNIPD_bbcDebugILerrB;
    sample->v_l_rif_a = UNIPD_bbcDebugVLrifA;
    sample->v_l_rif_b = UNIPD_bbcDebugVLrifB;
    sample->p_bat_rif = UNIPD_bbcOutput.p_bat_rif;
    sample->duty_raw_a = UNIPD_bbcOutput.duty_cycle_pwm_a;
    sample->duty_raw_b = UNIPD_bbcOutput.duty_cycle_pwm_b;
    sample->duty_internal_a = UNIPD_bbcDebugDutyRawA;
    sample->duty_internal_b = UNIPD_bbcDebugDutyRawB;
    sample->duty_mapped_a = UNIPD_bbcMappedDutyA_pu;
    sample->duty_mapped_b = UNIPD_bbcMappedDutyB_pu;
    sample->duty_applied_a = UNIPD_bbcAppliedDutyA_pu;
    sample->duty_applied_b = UNIPD_bbcAppliedDutyB_pu;
    sample->duty_ramped_a = UNIPD_bbcRampedDutyA_pu;
    sample->duty_ramped_b = UNIPD_bbcRampedDutyB_pu;

    UNIPD_bbcCaptureWriteIndex++;
    if(UNIPD_bbcCaptureWriteIndex >= UNIPD_BBC_CAPTURE_LENGTH)
    {
        UNIPD_bbcCaptureWriteIndex = 0U;
    }
    if(UNIPD_bbcCaptureCount < UNIPD_BBC_CAPTURE_LENGTH)
    {
        UNIPD_bbcCaptureCount++;
    }

    if(triggerReason != 0U)
    {
        UNIPD_bbcCaptureTriggerReason = triggerReason;
        UNIPD_bbcCaptureFrozen = 1U;
        UNIPD_bbcCaptureArmed = 0U;
    }
}

static const float unipd_arcsin_table[101] = {
    0.000000000000000f, 0.003183151915873f, 0.006366622213270f, 0.009550729560432f,
    0.012735793199755f, 0.015922133236660f, 0.019110070930640f, 0.022299928989202f,
    0.025492031865477f, 0.028686706060258f, 0.031884280429260f, 0.035085086496430f,
    0.038289458774147f, 0.041497735091205f, 0.044710256929508f, 0.047927369770437f,
    0.051149423451922f, 0.054376772537300f, 0.057609776697097f, 0.060848801104951f,
    0.064094216848975f, 0.067346401359940f, 0.070605738857752f, 0.073872620817828f,
    0.077147446459050f, 0.080430623255166f, 0.083722567471605f, 0.087023704729853f,
    0.090334470601733f, 0.093655311236095f, 0.096986684020678f, 0.100329058282131f,
    0.103682916027458f, 0.107048752730465f, 0.110427078167105f, 0.113818417304015f,
    0.117223311244961f, 0.120642318240358f, 0.124076014765585f, 0.127524996674404f,
    0.130989880434455f, 0.134471304452546f, 0.137969930498342f, 0.141486445235958f,
    0.145021561874111f, 0.148576021946683f, 0.152150597236966f, 0.155746091860452f,
    0.159363344522883f, 0.163003230972354f, 0.166666666666667f, 0.170354609679922f,
    0.174068063875524f, 0.177808082376468f, 0.181575771368100f, 0.185372294273510f,
    0.189198876347599f, 0.193056809742680f, 0.196947459106530f, 0.200872267783327f,
    0.204832764699133f, 0.208830572026983f, 0.212867413742587f, 0.216945125200853f,
    0.221065663886429f, 0.225231121519470f, 0.229443737731699f, 0.233705915569418f,
    0.238020239131091f, 0.242389493710302f, 0.246816688893365f, 0.251305085159287f,
    0.255858224653840f, 0.260479966967229f, 0.265174530946820f, 0.269946543837384f,
    0.274801099381575f, 0.279743826961257f, 0.284780974456357f, 0.289919508299921f,
    0.295167235300867f, 0.300532952314905f, 0.306026631958643f, 0.311659655572965f,
    0.317445109006172f, 0.323398163238602f, 0.329536571616801f, 0.335881330554762f,
    0.342457574575956f, 0.349295816015145f, 0.356433706871294f, 0.363918619603224f,
    0.371811566302050f, 0.380193416581985f, 0.389175313395541f, 0.398917375895740f,
    0.409665529398267f, 0.421834068069044f, 0.436231439141480f, 0.454946586355588f,
    0.500000000000000f
};

static float unipd_clampf(float value, float min_value, float max_value)
{
    if(value > max_value)
    {
        value = max_value;
    }
    if(value < min_value)
    {
        value = min_value;
    }
    return value;
}

static void unipd_clearDcBusCoilOutput(UNIPD_DcBusCoilControlOutput *output)
{
    output->i_coil_loc_err = 0.0f;
    output->duty_cycle_pwm_a = 0.0f;
    output->duty_cycle_pwm_b = 0.0f;
    output->abilita_pwm = 0U;
    output->duty_cycle_ps_a = 0.5f;
    output->duty_cycle_ps_b = 0.5f;
    output->abilita_ps = 0U;
    output->p_bat_rif = 0.0f;
    output->i_l_rif_a = 0.0f;
    output->i_l_rif_b = 0.0f;
    output->v_ac_rif = 0.0f;
}

static float unipd_arcsin_over_pi_interpolated(float argument)
{
    float table_index;
    unsigned int lower_index;
    float delta_index;
    float duty_lower;
    float duty_upper;

    argument = unipd_clampf(argument, 0.0f, 1.0f);
    table_index = argument * 100.0f;
    lower_index = (unsigned int)table_index;
    if(lower_index >= 100U)
    {
        lower_index = 99U;
    }

    delta_index = table_index - (float)lower_index;
    duty_lower = unipd_arcsin_table[lower_index];
    duty_upper = unipd_arcsin_table[lower_index + 1U];

    return duty_lower + (delta_index * (duty_upper - duty_lower));
}

static void unipd_calculatePhaseShiftDuty(float v_ac_rif,
                                          float v_ac_rif_max,
                                          float *duty_cycle_ps_a,
                                          float *duty_cycle_ps_b)
{
    float argument;
    float duty;

    if(v_ac_rif_max <= 0.0f)
    {
        *duty_cycle_ps_a = 0.5f;
        *duty_cycle_ps_b = 0.5f;
        return;
    }

    argument = v_ac_rif / v_ac_rif_max;
    duty = unipd_arcsin_over_pi_interpolated(argument);
    *duty_cycle_ps_a = 0.5f + duty;
    *duty_cycle_ps_b = 0.5f - duty;
}

void UNIPD_resetDcBusControl(UNIPD_DcBusControlState *state)
{
    if(state != 0)
    {
        state->i_l_err_a_p = 0.0f;
        state->i_l_err_b_p = 0.0f;
        state->v_l_rif_a_p = 0.0f;
        state->v_l_rif_b_p = 0.0f;
        state->v_dc_2_err_p = 0.0f;
        state->p_bat_rif_p = 0.0f;
    }
}

void UNIPD_resetRemoteCoilLimitControl(UNIPD_RemoteCoilLimitControlState *state)
{
    if(state != 0)
    {
        state->i_coil_rem_err_p = 0.0f;
        state->i_coil_rem_err_pp = 0.0f;
        state->v_ac_rif_p = 0.0f;
        state->v_ac_rif_pp = 0.0f;
        state->i_coil_loc_err_p = 0.0f;
        state->v_ac_rif_lim_p = 0.0f;
    }
}

void UNIPD_resetTransferredPowerControl(
    UNIPD_TransferredPowerControlState *state)
{
    if(state != 0)
    {
        state->v_dc_2_ptrasf_err_p = 0.0f;
        state->p_trasf_rif_p = 0.0f;
    }
}

void UNIPD_resetDcBusCoilControl(UNIPD_DcBusCoilControlState *state)
{
    if(state != 0)
    {
        state->counter = 0U;
        state->i_l_err_a_p = 0.0f;
        state->i_l_err_b_p = 0.0f;
        state->v_l_rif_a_p = 0.0f;
        state->v_l_rif_b_p = 0.0f;
        state->v_dc_2_pbat_err_p = 0.0f;
        state->p_bat_rif_p = 0.0f;
        state->i_coil_rem_err_p = 0.0f;
        state->i_coil_rem_err_pp = 0.0f;
        state->v_ac_rif_p = 0.0f;
        state->v_ac_rif_pp = 0.0f;
    }
}

void UNIPD_resetAllControls(void)
{
    UNIPD_resetDcBusControl(&unipd_dc_bus_state);
    UNIPD_resetRemoteCoilLimitControl(&unipd_remote_coil_limit_state);
    UNIPD_resetTransferredPowerControl(&unipd_rx_transferred_power_state);
    UNIPD_resetTransferredPowerControl(&unipd_tx_transferred_power_state);
    UNIPD_resetDcBusCoilControl(&unipd_dc_bus_coil_state);
}

void UNIPD_controlDcBus(UNIPD_DcBusControlState *state,
                        float v_dc_rif,
                        float v_dc,
                        float v_bat,
                        float i_l_a,
                        float i_l_b,
                        float i_bat_rif_max,
                        float i_bat_rif_min,
                        UNIPD_DcBusControlOutput *output)
{
    float p_bat_rif_max;
    float p_bat_rif_min;
    float v_dc_2_err;
    float i_bat_rif;
    float i_l_err_a;
    float i_l_err_b;
    float v_l_rif_max;
    float v_l_rif_min;
    float v_l_a_rif;
    float v_l_b_rif;

    if((state == 0) || (output == 0))
    {
        return;
    }

    v_bat = unipd_clampf(v_bat, UNIPD_vBatMinAlg_Volts, UNIPD_vBatMaxAlg_Volts);
    v_dc = unipd_clampf(v_dc, UNIPD_vDcMinAlg_Volts, UNIPD_vDcMaxAlg_Volts);

    p_bat_rif_max = v_bat * i_bat_rif_max;
    p_bat_rif_min = v_bat * i_bat_rif_min;
    v_dc_2_err = (v_dc_rif * v_dc_rif) - (v_dc * v_dc);
    output->p_bat_rif = state->p_bat_rif_p +
                         (UNIPD_vdcControllerGainScale *
                          UNIPD_K_V_DC_ERR * v_dc_2_err) +
                         (UNIPD_vdcControllerGainScale *
                          UNIPD_K_V_DC_ERR_P * state->v_dc_2_err_p);
    output->p_bat_rif = unipd_clampf(output->p_bat_rif, p_bat_rif_min, p_bat_rif_max);

    i_bat_rif = output->p_bat_rif / v_bat;
    output->i_l_rif_a = 0.5f * i_bat_rif;
    output->i_l_rif_b = output->i_l_rif_a;

    i_l_err_a = output->i_l_rif_a - i_l_a;
    i_l_err_b = output->i_l_rif_b - i_l_b;

    v_l_rif_max = v_bat;
    v_l_rif_min = v_bat - v_dc;
    v_l_a_rif = state->v_l_rif_a_p +
                (UNIPD_K_I_L_ERR * i_l_err_a) +
                (UNIPD_K_I_L_ERR_P * state->i_l_err_a_p);
    v_l_b_rif = state->v_l_rif_b_p +
                (UNIPD_K_I_L_ERR * i_l_err_b) +
                (UNIPD_K_I_L_ERR_P * state->i_l_err_b_p);
    v_l_a_rif = unipd_clampf(v_l_a_rif, v_l_rif_min, v_l_rif_max);
    v_l_b_rif = unipd_clampf(v_l_b_rif, v_l_rif_min, v_l_rif_max);

    output->duty_cycle_a = (v_bat - v_l_a_rif) / v_dc;
    output->duty_cycle_b = (v_bat - v_l_b_rif) / v_dc;

    state->i_l_err_a_p = i_l_err_a;
    state->i_l_err_b_p = i_l_err_b;
    state->v_l_rif_a_p = v_l_a_rif;
    state->v_l_rif_b_p = v_l_b_rif;
    state->v_dc_2_err_p = v_dc_2_err;
    state->p_bat_rif_p = output->p_bat_rif;
}

void UNIPD_controlRemoteCoilCurrentAndLocalCoilLimit(
    UNIPD_RemoteCoilLimitControlState *state,
    float v_dc,
    float i_coil_rem_err,
    float i_coil_loc,
    UNIPD_RemoteCoilLimitControlOutput *output)
{
    float v_ac_rif_max;
    float i_coil_loc_err;

    if((state == 0) || (output == 0))
    {
        return;
    }

    v_dc = unipd_clampf(v_dc, UNIPD_vDcMinAlg_Volts, UNIPD_vDcMaxAlg_Volts);
    v_ac_rif_max = v_dc * UNIPD_FOUR_OVER_PI;

    output->v_ac_rif = (UNIPD_K_V_COIL_RIF_P * state->v_ac_rif_p) +
                       (UNIPD_K_V_COIL_RIF_PP * state->v_ac_rif_pp) +
                       (UNIPD_K_I_COIL_ERR * i_coil_rem_err) +
                       (UNIPD_K_I_COIL_ERR_P * state->i_coil_rem_err_p) +
                       (UNIPD_K_I_COIL_ERR_PP * state->i_coil_rem_err_pp);
    output->v_ac_rif = unipd_clampf(output->v_ac_rif, 0.0f, v_ac_rif_max);

    i_coil_loc_err = UNIPD_I_COIL_LOC_LIM - i_coil_loc;
    output->v_ac_rif_lim = (UNIPD_K_V_COIL_LIM_P * state->v_ac_rif_lim_p) +
                           (UNIPD_K_I_COIL_LOC_ERR * i_coil_loc_err) +
                           (UNIPD_K_I_COIL_LOC_ERR_P * state->i_coil_loc_err_p);
    output->v_ac_rif_lim = unipd_clampf(output->v_ac_rif_lim, 0.0f, v_ac_rif_max);

    if(output->v_ac_rif > output->v_ac_rif_lim)
    {
        output->v_ac_rif = output->v_ac_rif_lim;
    }
    if(output->v_ac_rif_lim > (output->v_ac_rif + 1.0f))
    {
        output->v_ac_rif_lim = output->v_ac_rif + 1.0f;
    }

    unipd_calculatePhaseShiftDuty(output->v_ac_rif,
                                  v_ac_rif_max,
                                  &output->duty_cycle_ps_a,
                                  &output->duty_cycle_ps_b);

    state->v_ac_rif_pp = state->v_ac_rif_p;
    state->v_ac_rif_p = output->v_ac_rif;
    state->i_coil_rem_err_pp = state->i_coil_rem_err_p;
    state->i_coil_rem_err_p = i_coil_rem_err;
    state->v_ac_rif_lim_p = output->v_ac_rif_lim;
    state->i_coil_loc_err_p = i_coil_loc_err;
}

void UNIPD_controlRxDcBusViaTransferredPower(
    UNIPD_TransferredPowerControlState *state,
    float i_coil_loc,
    float p_trasf_rem_rif_lim,
    float v_dc,
    float v_dc_ptrasf_rif,
    UNIPD_RxTransferredPowerControlOutput *output)
{
    float v_dc_2_ptrasf_err;
    float p_trasf_loc_max;

    if((state == 0) || (output == 0))
    {
        return;
    }

    v_dc = unipd_clampf(v_dc, UNIPD_vDcMinAlg_Volts,
                        UNIPD_vDcMaxAlg_Volts);
    p_trasf_loc_max = (p_trasf_rem_rif_lim > 0.0f) ?
            p_trasf_rem_rif_lim : 0.0f;
    v_dc_2_ptrasf_err = (v_dc_ptrasf_rif * v_dc_ptrasf_rif) -
                        (v_dc * v_dc);

    output->p_trasf_rif =
            (UNIPD_RX_PTRASF_A1 * state->p_trasf_rif_p) +
            (UNIPD_RX_PTRASF_B0 * v_dc_2_ptrasf_err) +
            (UNIPD_RX_PTRASF_B1 * state->v_dc_2_ptrasf_err_p);
    output->p_trasf_rif = unipd_clampf(output->p_trasf_rif,
                                       0.0f, p_trasf_loc_max);

    output->i_coil_rif = (0.5f * UNIPD_PI) *
                         output->p_trasf_rif / v_dc;
    output->i_coil_err = output->i_coil_rif - i_coil_loc;

    state->v_dc_2_ptrasf_err_p = v_dc_2_ptrasf_err;
    state->p_trasf_rif_p = output->p_trasf_rif;
}

float UNIPD_controlTxDcBusTransferredPowerLimit(
    UNIPD_TransferredPowerControlState *state,
    float v_dc,
    float v_dc_ptrasf_rif,
    float i_coil_amp_max,
    float i_coil_amp_min)
{
    float v_dc_2_ptrasf_err;
    float p_trasf_rif;
    float p_trasf_min;
    float p_trasf_max;

    if(state == 0)
    {
        return 0.0f;
    }

    v_dc = unipd_clampf(v_dc, UNIPD_vDcMinAlg_Volts,
                        UNIPD_vDcMaxAlg_Volts);
    i_coil_amp_min = (i_coil_amp_min > 0.0f) ? i_coil_amp_min : 0.0f;
    i_coil_amp_max = (i_coil_amp_max > i_coil_amp_min) ?
            i_coil_amp_max : i_coil_amp_min;
    p_trasf_min = v_dc * i_coil_amp_min * (2.0f / UNIPD_PI);
    p_trasf_max = v_dc * i_coil_amp_max * (2.0f / UNIPD_PI);

    /* Transferred power discharges the transmitter bus: reverse the error. */
    v_dc_2_ptrasf_err = (v_dc * v_dc) -
                        (v_dc_ptrasf_rif * v_dc_ptrasf_rif);
    p_trasf_rif =
            (UNIPD_TX_PTRASF_A1 * state->p_trasf_rif_p) +
            (UNIPD_TX_PTRASF_B0 * v_dc_2_ptrasf_err) +
            (UNIPD_TX_PTRASF_B1 * state->v_dc_2_ptrasf_err_p);
    p_trasf_rif = unipd_clampf(p_trasf_rif, p_trasf_min, p_trasf_max);

    state->v_dc_2_ptrasf_err_p = v_dc_2_ptrasf_err;
    state->p_trasf_rif_p = p_trasf_rif;
    return p_trasf_rif;
}

void UNIPD_runTransferredPowerIntegration(void)
{
    static unsigned int previousRole;
    const unsigned int role = (unsigned int)WLESS_SM_localRole;
    float localCoilCurrent;

    UNIPD_wptLocalCoilPhysical_Amps = UNIPD_bbcInputs.i_coil_loc;
    localCoilCurrent = UNIPD_wptLocalCoilPhysical_Amps;
    if((role == (unsigned int)WLESS_SM_ROLE_SOURCE) &&
       (UNIPD_wptSourceCoilSyntheticEnable != 0U))
    {
        localCoilCurrent = UNIPD_wptSourceCoilSynthetic_Amps;
    }
    else if((role == (unsigned int)WLESS_SM_ROLE_LOAD) &&
            (UNIPD_wptLoadCoilSyntheticEnable != 0U))
    {
        localCoilCurrent = UNIPD_wptLoadCoilSynthetic_Amps;
    }
    else if(role == (unsigned int)WLESS_SM_ROLE_LOAD)
    {
        localCoilCurrent -= UNIPD_wptLoadCoilOffset_Amps;
        if(localCoilCurrent < 0.0f)
        {
            localCoilCurrent = 0.0f;
        }
    }
    UNIPD_wptLocalCoilUsed_Amps = localCoilCurrent;

    if((UNIPD_wptIntegrationEnable == 0U) ||
       ((role != (unsigned int)WLESS_SM_ROLE_SOURCE) &&
        (role != (unsigned int)WLESS_SM_ROLE_LOAD)))
    {
        unipd_clearTransferredPowerIntegration();
        WLESS_SM_powerToLoad = 0;
        WLESS_SM_iCoilErr = 0;
        if(UNIPD_wptHfcActuatorEnable != 0U)
        {
            UNIPD_wptHfcActuatorFault = 7U;
            UNIPD_disableWptHfcActuator();
        }
        previousRole = role;
        unipd_captureWptCycle();
        return;
    }

    if(role != previousRole)
    {
        unipd_clearTransferredPowerIntegration();
        previousRole = role;
    }

    if(role == (unsigned int)WLESS_SM_ROLE_SOURCE)
    {
        if(UNIPD_wptTxPowerSeedPending != 0U)
        {
            float seedVdc = unipd_clampf(UNIPD_bbcInputs.v_dc,
                                        UNIPD_vDcMinAlg_Volts,
                                        UNIPD_vDcMaxAlg_Volts);
            float seedMax = seedVdc * UNIPD_wptCoilCurrentMax_Amps *
                            (2.0f / UNIPD_PI);
            unipd_tx_transferred_power_state.v_dc_2_ptrasf_err_p =
                    (seedVdc * seedVdc) -
                    (UNIPD_wptVdcSourceRef_Volts *
                     UNIPD_wptVdcSourceRef_Volts);
            unipd_tx_transferred_power_state.p_trasf_rif_p =
                    unipd_clampf(UNIPD_wptTxPowerSeed_Watts,
                                 0.0f, seedMax);
            UNIPD_wptTxPowerSeedPending = 0U;
        }
        UNIPD_wptRemoteCoilErr_Amps =
                (float)WLESS_SM_iCoilErr *
                UNIPD_WPT_COIL_ERR_A_PER_LSB;
        UNIPD_wptTxPowerLimit_Watts =
                UNIPD_controlTxDcBusTransferredPowerLimit(
                    &unipd_tx_transferred_power_state,
                    UNIPD_bbcInputs.v_dc,
                    UNIPD_wptVdcSourceRef_Volts,
                    UNIPD_wptCoilCurrentMax_Amps,
                    UNIPD_wptCoilCurrentMin_Amps);
        WLESS_SM_powerToLoad = unipd_encodeSigned16(
                UNIPD_wptTxPowerLimit_Watts /
                UNIPD_WPT_POWER_W_PER_LSB);

        UNIPD_controlRemoteCoilCurrentAndLocalCoilLimit(
                &unipd_remote_coil_limit_state,
                UNIPD_bbcInputs.v_dc,
                UNIPD_wptRemoteCoilErr_Amps,
                localCoilCurrent,
                &UNIPD_wptHfcOutput);
        unipd_updateWptHfcActuator(role);
    }
    else
    {
        UNIPD_wptRemotePowerLimit_Watts =
                (float)WLESS_SM_powerToLoad *
                UNIPD_WPT_POWER_W_PER_LSB;
        if((UNIPD_wptLoadPowerSeedPending != 0U) &&
           (UNIPD_wptRemotePowerLimit_Watts > 0.0f))
        {
            float seedVdc = unipd_clampf(UNIPD_bbcInputs.v_dc,
                                        UNIPD_vDcMinAlg_Volts,
                                        UNIPD_vDcMaxAlg_Volts);
            float seedMax = (UNIPD_wptRemotePowerLimit_Watts > 0.0f) ?
                    UNIPD_wptRemotePowerLimit_Watts : 0.0f;
            unipd_rx_transferred_power_state.v_dc_2_ptrasf_err_p =
                    (UNIPD_wptVdcLoadRef_Volts *
                     UNIPD_wptVdcLoadRef_Volts) -
                    (seedVdc * seedVdc);
            unipd_rx_transferred_power_state.p_trasf_rif_p =
                    unipd_clampf(UNIPD_wptLoadPowerSeed_Watts,
                                 0.0f, seedMax);
            UNIPD_wptLoadPowerSeedPending = 0U;
        }
        UNIPD_controlRxDcBusViaTransferredPower(
                &unipd_rx_transferred_power_state,
                localCoilCurrent,
                UNIPD_wptRemotePowerLimit_Watts,
                UNIPD_bbcInputs.v_dc,
                UNIPD_wptVdcLoadRef_Volts,
                &UNIPD_wptRxOutput);
        if(UNIPD_wptRxOutput.i_coil_rif > UNIPD_wptCoilCurrentMax_Amps)
        {
            UNIPD_wptRxOutput.i_coil_rif = UNIPD_wptCoilCurrentMax_Amps;
            UNIPD_wptRxOutput.i_coil_err =
                    UNIPD_wptRxOutput.i_coil_rif - localCoilCurrent;
        }
        WLESS_SM_iCoilErr = unipd_encodeSigned16(
                UNIPD_wptRxOutput.i_coil_err /
                UNIPD_WPT_COIL_ERR_A_PER_LSB);

        UNIPD_resetRemoteCoilLimitControl(&unipd_remote_coil_limit_state);
        UNIPD_wptHfcOutput.duty_cycle_ps_a = 0.5f;
        UNIPD_wptHfcOutput.duty_cycle_ps_b = 0.5f;
        UNIPD_wptHfcOutput.v_ac_rif = 0.0f;
        UNIPD_wptHfcOutput.v_ac_rif_lim = 0.0f;
    }
    unipd_captureWptCycle();
}

void UNIPD_controlDcBusAndCoilCurrent(UNIPD_DcBusCoilControlState *state,
                                      float i_coil_loc_rif,
                                      float i_coil_loc,
                                      float i_coil_rem_err,
                                      float v_dc_pbat_rif,
                                      float v_dc,
                                      float v_bat,
                                      float i_l_a,
                                      float i_l_b,
                                      float i_bat_rif_max,
                                      float i_bat_rif_min,
                                      unsigned int tx_1_rx_0,
                                      UNIPD_DcBusCoilControlOutput *output)
{
    float p_bat_rif_max;
    float p_bat_rif_min;
    float v_dc_pbat_2_err;
    float i_bat_rif;
    float i_l_err_a;
    float i_l_err_b;
    float v_l_rif_max;
    float v_l_rif_min;
    float v_l_a_rif;
    float v_l_b_rif;
    float v_ac_rif_max;

    if((state == 0) || (output == 0))
    {
        return;
    }

    if(state->counter < UNIPD_PWM_ENABLE_DELAY)
    {
        state->counter++;
        output->abilita_pwm = 0U;
        output->abilita_ps = 0U;
    }
    else
    {
        output->abilita_pwm = 1U;
        output->abilita_ps = (tx_1_rx_0 == 1U) ? 1U : 0U;
    }

    v_bat = unipd_clampf(v_bat, UNIPD_vBatMinAlg_Volts, UNIPD_vBatMaxAlg_Volts);
    v_dc = unipd_clampf(v_dc, UNIPD_vDcMinAlg_Volts, UNIPD_vDcMaxAlg_Volts);

    p_bat_rif_max = v_bat * i_bat_rif_max;
    p_bat_rif_min = v_bat * i_bat_rif_min;
    v_dc_pbat_2_err = (v_dc_pbat_rif * v_dc_pbat_rif) - (v_dc * v_dc);
    output->p_bat_rif = state->p_bat_rif_p +
                         (UNIPD_vdcControllerGainScale *
                          UNIPD_K_V_DC_ERR * v_dc_pbat_2_err) +
                         (UNIPD_vdcControllerGainScale *
                          UNIPD_K_V_DC_ERR_P * state->v_dc_2_pbat_err_p);
    output->p_bat_rif = unipd_clampf(output->p_bat_rif, p_bat_rif_min, p_bat_rif_max);

    i_bat_rif = output->p_bat_rif / v_bat;
    output->i_l_rif_a = 0.5f * i_bat_rif;
    output->i_l_rif_b = output->i_l_rif_a;

    i_l_err_a = output->i_l_rif_a - i_l_a;
    i_l_err_b = output->i_l_rif_b - i_l_b;

    v_l_rif_max = v_bat;
    v_l_rif_min = v_bat - v_dc;
    v_l_a_rif = state->v_l_rif_a_p +
                (UNIPD_K_I_L_ERR * i_l_err_a) +
                (UNIPD_K_I_L_ERR_P * state->i_l_err_a_p);
    v_l_b_rif = state->v_l_rif_b_p +
                (UNIPD_K_I_L_ERR * i_l_err_b) +
                (UNIPD_K_I_L_ERR_P * state->i_l_err_b_p);
    v_l_a_rif = unipd_clampf(v_l_a_rif, v_l_rif_min, v_l_rif_max);
    v_l_b_rif = unipd_clampf(v_l_b_rif, v_l_rif_min, v_l_rif_max);

    output->duty_cycle_pwm_a = (v_bat - v_l_a_rif) / v_dc;
    output->duty_cycle_pwm_b = (v_bat - v_l_b_rif) / v_dc;
    UNIPD_bbcDebugILerrA = i_l_err_a;
    UNIPD_bbcDebugILerrB = i_l_err_b;
    UNIPD_bbcDebugVLrifMin = v_l_rif_min;
    UNIPD_bbcDebugVLrifMax = v_l_rif_max;
    UNIPD_bbcDebugVLrifA = v_l_a_rif;
    UNIPD_bbcDebugVLrifB = v_l_b_rif;
    UNIPD_bbcDebugDutyRawA = output->duty_cycle_pwm_a;
    UNIPD_bbcDebugDutyRawB = output->duty_cycle_pwm_b;

    v_ac_rif_max = v_dc * UNIPD_FOUR_OVER_PI;
    output->i_coil_loc_err = i_coil_loc_rif - i_coil_loc;
    output->v_ac_rif = (UNIPD_K_V_COIL_RIF_P * state->v_ac_rif_p) +
                       (UNIPD_K_V_COIL_RIF_PP * state->v_ac_rif_pp) +
                       (UNIPD_K_I_COIL_ERR * i_coil_rem_err) +
                       (UNIPD_K_I_COIL_ERR_P * state->i_coil_rem_err_p) +
                       (UNIPD_K_I_COIL_ERR_PP * state->i_coil_rem_err_pp);
    output->v_ac_rif = unipd_clampf(output->v_ac_rif, 0.0f, v_ac_rif_max);

    unipd_calculatePhaseShiftDuty(output->v_ac_rif,
                                  v_ac_rif_max,
                                  &output->duty_cycle_ps_a,
                                  &output->duty_cycle_ps_b);

    if(state->counter == UNIPD_PWM_ENABLE_DELAY)
    {
        state->i_l_err_a_p = i_l_err_a;
        state->i_l_err_b_p = i_l_err_b;
        state->v_l_rif_a_p = v_l_a_rif;
        state->v_l_rif_b_p = v_l_b_rif;
        state->v_dc_2_pbat_err_p = v_dc_pbat_2_err;
        state->p_bat_rif_p = output->p_bat_rif;
        state->i_coil_rem_err_pp = state->i_coil_rem_err_p;
        state->i_coil_rem_err_p = i_coil_rem_err;
        state->v_ac_rif_pp = state->v_ac_rif_p;
        state->v_ac_rif_p = output->v_ac_rif;
    }
    else
    {
        state->i_l_err_a_p = i_l_err_a;
        state->i_l_err_b_p = i_l_err_b;
        state->v_l_rif_a_p = v_l_a_rif;
        state->v_l_rif_b_p = v_l_b_rif;
    }
}

static void unipd_applySyntheticBbcIntegrationInputs(
        UNIPD_BbcIntegrationInputs *input)
{
    const unsigned int overrideMask =
            UNIPD_bbcSyntheticOverrideMask & UNIPD_bbcSyntheticValidMask;

    if((overrideMask & UNIPD_BBC_SIGNAL_V_DC) != 0U)
    {
        input->v_dc = UNIPD_bbcSyntheticInputs.v_dc;
    }
    if((overrideMask & UNIPD_BBC_SIGNAL_V_BAT) != 0U)
    {
        input->v_bat = UNIPD_bbcSyntheticInputs.v_bat;
    }
    if((overrideMask & UNIPD_BBC_SIGNAL_I_L_A) != 0U)
    {
        input->i_l_a = UNIPD_bbcSyntheticInputs.i_l_a;
    }
    if((overrideMask & UNIPD_BBC_SIGNAL_I_L_B) != 0U)
    {
        input->i_l_b = UNIPD_bbcSyntheticInputs.i_l_b;
    }
    if((overrideMask & UNIPD_BBC_SIGNAL_I_COIL_LOC) != 0U)
    {
        input->i_coil_loc = UNIPD_bbcSyntheticInputs.i_coil_loc;
    }
    if((overrideMask & UNIPD_BBC_SIGNAL_I_COIL_REM_ERR) != 0U)
    {
        input->i_coil_rem_err = UNIPD_bbcSyntheticInputs.i_coil_rem_err;
    }
    if((overrideMask & UNIPD_BBC_SIGNAL_I_COIL_LOC_REF) != 0U)
    {
        input->i_coil_loc_rif = UNIPD_bbcSyntheticInputs.i_coil_loc_rif;
    }
    if((overrideMask & UNIPD_BBC_SIGNAL_V_DC_PBAT_REF) != 0U)
    {
        input->v_dc_pbat_rif = UNIPD_bbcSyntheticInputs.v_dc_pbat_rif;
    }
    if((overrideMask & UNIPD_BBC_SIGNAL_I_BAT_MAX) != 0U)
    {
        input->i_bat_rif_max = UNIPD_bbcSyntheticInputs.i_bat_rif_max;
    }
    if((overrideMask & UNIPD_BBC_SIGNAL_I_BAT_MIN) != 0U)
    {
        input->i_bat_rif_min = UNIPD_bbcSyntheticInputs.i_bat_rif_min;
    }
    if((overrideMask & UNIPD_BBC_SIGNAL_TX_RX_MODE) != 0U)
    {
        input->tx_1_rx_0 = UNIPD_bbcSyntheticInputs.tx_1_rx_0;
    }

    input->valid_mask |= UNIPD_bbcSyntheticValidMask;
}

void UNIPD_collectBbcIntegrationInputs(UNIPD_BbcIntegrationInputs *input)
{
    input->v_dc = 0.0f;
    input->v_bat = 0.0f;
    input->i_l_a = 0.0f;
    input->i_l_b = 0.0f;
    input->i_coil_loc = 0.0f;
    input->i_coil_rem_err = 0.0f;
    input->i_coil_loc_rif = 0.0f;
    input->v_dc_pbat_rif = 0.0f;
    input->i_bat_rif_max = 0.0f;
    input->i_bat_rif_min = 0.0f;
    input->tx_1_rx_0 = 0U;
    input->valid_mask = 0U;

    /*
     * These reads currently use the existing ADC result registers. The final
     * control entry point should be moved to an ADC-EOC ISR once the complete
     * synchronized conversion burst has been defined.
     */
    TTPLPFC_read_individualCurrent();
    TTPLPFC_read_busVoltage();
    TTPLPFC_read_systemSignals();
    CLLLC_readWirelessTankAnalogSignals();

    input->v_dc = TTPLPFC_vBus_sensed_pu * TTPLPFC_VDCBUS_MAX_SENSE *
                  TTPLPFC_WLESS_VBUS_SENSE_CORRECTION;
    input->v_bat = TTPLPFC_vBatSensed_Volts;
    UNIPD_bbcILRawA_Amps =
            TTPLPFC_iL1_sensed_pu * TTPLPFC_IL_MAX_SENSE;
    UNIPD_bbcILRawB_Amps =
            TTPLPFC_iL2_sensed_pu * TTPLPFC_IL_MAX_SENSE;
    input->i_l_a = ((UNIPD_bbcCurrentPolarityMask & 1U) != 0U) ?
                   -UNIPD_bbcILRawA_Amps : UNIPD_bbcILRawA_Amps;
    input->i_l_b = ((UNIPD_bbcCurrentPolarityMask & 2U) != 0U) ?
                   -UNIPD_bbcILRawB_Amps : UNIPD_bbcILRawB_Amps;
    input->i_coil_loc =
            CLLLC_iTankModSensed_pu * CLLLC_IPRIM_TANK_MAX_SENSE_AMPS;

    input->valid_mask |= UNIPD_BBC_SIGNAL_V_DC |
                         UNIPD_BBC_SIGNAL_V_BAT |
                         UNIPD_BBC_SIGNAL_I_L_A |
                         UNIPD_BBC_SIGNAL_I_L_B |
                         UNIPD_BBC_SIGNAL_I_COIL_LOC;

    if(UNIPD_bbcSyntheticTestEnable != 0U)
    {
        unipd_applySyntheticBbcIntegrationInputs(input);
    }
}

void UNIPD_runBbcDockingDiagnostics(void)
{
    /*
     * The docking-test path has ISR precedence over the UniPD controller.
     * Keep the physical measurements and the diagnostic capture alive without
     * executing the controller or writing any additional PWM command.
     */
    UNIPD_collectBbcIntegrationInputs(&UNIPD_bbcInputs);
    UNIPD_bbcSignalValidMask = UNIPD_bbcInputs.valid_mask;
    UNIPD_bbcSignalMissingMask =
            UNIPD_BBC_REQUIRED_SIGNAL_MASK & ~UNIPD_bbcSignalValidMask;

    /*
     * During docking tests these fields describe the duty actually handed to
     * the BBC adapter. Controller references/errors remain untouched so that
     * no open-loop sample can alter the UniPD control state.
     */
    UNIPD_bbcMappedDutyA_pu = TTPLPFC_duty1_pu;
    UNIPD_bbcMappedDutyB_pu = TTPLPFC_duty2_pu;
    UNIPD_bbcAppliedDutyA_pu = TTPLPFC_duty1_pu;
    UNIPD_bbcAppliedDutyB_pu = TTPLPFC_duty2_pu;
    UNIPD_bbcRampedDutyA_pu = TTPLPFC_duty1_pu;
    UNIPD_bbcRampedDutyB_pu = TTPLPFC_duty2_pu;

    unipd_captureBbcCycle();
}

void OBC_7_4KW_runUnipdBbcControl(void)
{
    if(UNIPD_resetControlStatesCommand != 0U)
    {
        UNIPD_resetAllControls();
        UNIPD_resetControlStatesCommand = 0U;
    }

    UNIPD_collectBbcIntegrationInputs(&UNIPD_bbcInputs);

    if(UNIPD_bbcVdcTripResetCommand != 0U)
    {
        if((UNIPD_bbcInputs.v_dc < UNIPD_bbcVdcTripThreshold_Volts) &&
           (UNIPD_bbcInputs.i_l_a < UNIPD_bbcILTripThreshold_Amps) &&
           (UNIPD_bbcInputs.i_l_a > -UNIPD_bbcILTripThreshold_Amps) &&
           (UNIPD_bbcInputs.i_l_b < UNIPD_bbcILTripThreshold_Amps) &&
           (UNIPD_bbcInputs.i_l_b > -UNIPD_bbcILTripThreshold_Amps))
        {
            UNIPD_bbcVdcTripLatched = 0U;
            UNIPD_bbcVdcTripCapture_Volts = 0.0f;
            UNIPD_bbcILTripLatched = 0U;
            UNIPD_bbcILTripCaptureA_Amps = 0.0f;
            UNIPD_bbcILTripCaptureB_Amps = 0.0f;
            UNIPD_bbcILTripConfirmCount = 0U;
        }
        UNIPD_bbcVdcTripResetCommand = 0U;
    }

    if((UNIPD_bbcILTripEnable != 0U) &&
       ((UNIPD_bbcInputs.i_l_a >= UNIPD_bbcILTripThreshold_Amps) ||
        (UNIPD_bbcInputs.i_l_a <= -UNIPD_bbcILTripThreshold_Amps) ||
        (UNIPD_bbcInputs.i_l_b >= UNIPD_bbcILTripThreshold_Amps) ||
        (UNIPD_bbcInputs.i_l_b <= -UNIPD_bbcILTripThreshold_Amps)))
    {
        if(UNIPD_bbcILTripConfirmCount < UNIPD_bbcILTripConfirmCycles)
        {
            UNIPD_bbcILTripConfirmCount++;
        }
        if(UNIPD_bbcILTripConfirmCount >= UNIPD_bbcILTripConfirmCycles)
        {
            UNIPD_bbcILTripLatched = 1U;
            UNIPD_bbcILTripCaptureA_Amps = UNIPD_bbcInputs.i_l_a;
            UNIPD_bbcILTripCaptureB_Amps = UNIPD_bbcInputs.i_l_b;
            UNIPD_bbcPowerOutputEnable = 0U;
        }
    }
    else
    {
        UNIPD_bbcILTripConfirmCount = 0U;
    }

    if((UNIPD_bbcVdcTripEnable != 0U) &&
       (UNIPD_bbcInputs.v_dc >= UNIPD_bbcVdcTripThreshold_Volts))
    {
        if(UNIPD_bbcVdcTripLatched == 0U)
        {
            UNIPD_bbcVdcTripCapture_Volts = UNIPD_bbcInputs.v_dc;
        }
        UNIPD_bbcVdcTripLatched = 1U;
        UNIPD_bbcPowerOutputEnable = 0U;
    }

    unipd_captureBbcCycle();

    UNIPD_bbcSignalValidMask = UNIPD_bbcInputs.valid_mask;
    UNIPD_bbcSignalMissingMask =
            UNIPD_BBC_REQUIRED_SIGNAL_MASK & ~UNIPD_bbcSignalValidMask;

    if(UNIPD_bbcSignalMissingMask == 0U)
    {
        UNIPD_controlDcBusAndCoilCurrent(&unipd_dc_bus_coil_state,
                                         UNIPD_bbcInputs.i_coil_loc_rif,
                                         UNIPD_bbcInputs.i_coil_loc,
                                         UNIPD_bbcInputs.i_coil_rem_err,
                                         UNIPD_bbcInputs.v_dc_pbat_rif,
                                         UNIPD_bbcInputs.v_dc,
                                         UNIPD_bbcInputs.v_bat,
                                         UNIPD_bbcInputs.i_l_a,
                                         UNIPD_bbcInputs.i_l_b,
                                         UNIPD_bbcInputs.i_bat_rif_max,
                                         UNIPD_bbcInputs.i_bat_rif_min,
                                         UNIPD_bbcInputs.tx_1_rx_0,
                                         &UNIPD_bbcOutput);
    }
    else
    {
        UNIPD_resetDcBusCoilControl(&unipd_dc_bus_coil_state);
        unipd_clearDcBusCoilOutput(&UNIPD_bbcOutput);
    }

#if UNIPD_BBC_ENABLE_POWER_OUTPUTS
    if((UNIPD_bbcSignalMissingMask == 0U) &&
       (UNIPD_bbcOutput.abilita_pwm != 0U) &&
       (UNIPD_bbcVdcTripLatched == 0U) &&
       (UNIPD_bbcILTripLatched == 0U))
    {
        const float dutyMax = unipd_clampf(UNIPD_bbcPowerOutputDutyMax_pu,
                                           0.0f,
                                           0.95f);
#if TTPLPFC_BBC_COMPLEMENTARY_PWM_ENABLE
        //
        // UniPD defines delta as the high-side duty for both power-flow
        // directions. The dead-band module generates the complementary
        // low-side command. Do not apply the asynchronous BOOST 1-delta map.
        //
        UNIPD_bbcMappedDutyA_pu = UNIPD_bbcOutput.duty_cycle_pwm_a;
        UNIPD_bbcMappedDutyB_pu = UNIPD_bbcOutput.duty_cycle_pwm_b;
#else
        UNIPD_bbcMappedDutyA_pu =
                (UNIPD_bbcDutyMappingMode != 0U) ?
                (1.0f - UNIPD_bbcOutput.duty_cycle_pwm_a) :
                UNIPD_bbcOutput.duty_cycle_pwm_a;
        UNIPD_bbcMappedDutyB_pu =
                (UNIPD_bbcDutyMappingMode != 0U) ?
                (1.0f - UNIPD_bbcOutput.duty_cycle_pwm_b) :
                UNIPD_bbcOutput.duty_cycle_pwm_b;
#endif

        UNIPD_bbcAppliedDutyA_pu =
                unipd_clampf(UNIPD_bbcMappedDutyA_pu, 0.0f, dutyMax);
        UNIPD_bbcAppliedDutyB_pu =
                unipd_clampf(UNIPD_bbcMappedDutyB_pu, 0.0f, dutyMax);

        if(UNIPD_bbcPowerOutputEnable != 0U)
        {
#if TTPLPFC_BBC_COMPLEMENTARY_PWM_ENABLE
            //
            // Ramping the high-side duty from zero is not a safe soft start
            // for a synchronous half bridge: delta=0 commands the low side
            // almost continuously. Apply the controller duty directly; a
            // validated synchronous startup/precharge policy is still needed.
            //
            UNIPD_bbcRampedDutyA_pu = UNIPD_bbcAppliedDutyA_pu;
            UNIPD_bbcRampedDutyB_pu = UNIPD_bbcAppliedDutyB_pu;
#else
            if(UNIPD_bbcPowerOutputDutyRampEnable != 0U)
            {
                const float rampStep =
                        unipd_clampf(UNIPD_bbcPowerOutputDutyRampStep_pu,
                                     0.0f,
                                     dutyMax);

                if(UNIPD_bbcRampedDutyA_pu < UNIPD_bbcAppliedDutyA_pu)
                {
                    UNIPD_bbcRampedDutyA_pu += rampStep;
                    if(UNIPD_bbcRampedDutyA_pu > UNIPD_bbcAppliedDutyA_pu)
                    {
                        UNIPD_bbcRampedDutyA_pu = UNIPD_bbcAppliedDutyA_pu;
                    }
                }
                else
                {
                    UNIPD_bbcRampedDutyA_pu -= rampStep;
                    if(UNIPD_bbcRampedDutyA_pu < UNIPD_bbcAppliedDutyA_pu)
                    {
                        UNIPD_bbcRampedDutyA_pu = UNIPD_bbcAppliedDutyA_pu;
                    }
                }

                if(UNIPD_bbcRampedDutyB_pu < UNIPD_bbcAppliedDutyB_pu)
                {
                    UNIPD_bbcRampedDutyB_pu += rampStep;
                    if(UNIPD_bbcRampedDutyB_pu > UNIPD_bbcAppliedDutyB_pu)
                    {
                        UNIPD_bbcRampedDutyB_pu = UNIPD_bbcAppliedDutyB_pu;
                    }
                }

                else
                {
                    UNIPD_bbcRampedDutyB_pu -= rampStep;
                    if(UNIPD_bbcRampedDutyB_pu < UNIPD_bbcAppliedDutyB_pu)
                    {
                        UNIPD_bbcRampedDutyB_pu = UNIPD_bbcAppliedDutyB_pu;
                    }
                }
            }
            else
            {
                UNIPD_bbcRampedDutyA_pu = UNIPD_bbcAppliedDutyA_pu;
                UNIPD_bbcRampedDutyB_pu = UNIPD_bbcAppliedDutyB_pu;
            }
#endif

            const TTPLPFC_BBC_Mode requestedMode =
                    (UNIPD_bbcInputs.tx_1_rx_0 != 0U) ?
                    TTPLPFC_BBC_MODE_BOOST : TTPLPFC_BBC_MODE_BUCK;
            const unsigned int modeChanged =
                    (TTPLPFC_bbcMode != requestedMode) ? 1U : 0U;

            TTPLPFC_BBC_setMode(requestedMode);
            if(modeChanged != 0U)
            {
                if(requestedMode == TTPLPFC_BBC_MODE_BOOST)
                {
                    TTPLPFC_HAL_setupBBCBoostLowSidePWM(
                            TTPLPFC_BBC_LEG1_PWM_BASE);
                    TTPLPFC_HAL_setupBBCBoostLowSidePWM(
                            TTPLPFC_BBC_LEG2_PWM_BASE);
                }
                else
                {
                    TTPLPFC_HAL_setupBBCBuckHighSidePWM(
                            TTPLPFC_BBC_LEG1_PWM_BASE);
                    TTPLPFC_HAL_setupBBCBuckHighSidePWM(
                            TTPLPFC_BBC_LEG2_PWM_BASE);
                }

            }
            TTPLPFC_BBC_setDuty(UNIPD_bbcRampedDutyA_pu,
                                UNIPD_bbcRampedDutyB_pu);
            TTPLPFC_BBC_enable();
        }
        else
        {
            UNIPD_bbcRampedDutyA_pu = 0.0f;
            UNIPD_bbcRampedDutyB_pu = 0.0f;
            TTPLPFC_BBC_setMode(TTPLPFC_BBC_MODE_DISABLED);
            TTPLPFC_BBC_disable();
        }
    }
    else
#endif
    {
        UNIPD_bbcMappedDutyA_pu = 0.0f;
        UNIPD_bbcMappedDutyB_pu = 0.0f;
        UNIPD_bbcAppliedDutyA_pu = 0.0f;
        UNIPD_bbcAppliedDutyB_pu = 0.0f;
        UNIPD_bbcRampedDutyA_pu = 0.0f;
        UNIPD_bbcRampedDutyB_pu = 0.0f;
        TTPLPFC_BBC_setMode(TTPLPFC_BBC_MODE_DISABLED);
        TTPLPFC_BBC_disable();
    }
}

void OBC_7_4KW_runUnipdControlHooks(void)
{
#if UNIPD_CONTROL_ENABLE_RUNTIME_HOOK
    OBC_7_4KW_runUnipdBbcControl();
#endif
}
