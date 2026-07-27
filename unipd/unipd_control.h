#ifndef UNIPD_CONTROL_H
#define UNIPD_CONTROL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Existing nRF24 int16 payload field scales used by the UniPD integration. */
#define UNIPD_WPT_POWER_W_PER_LSB       (1.0f)
#define UNIPD_WPT_COIL_ERR_A_PER_LSB    (0.01f)

typedef struct
{
    float i_l_err_a_p;
    float i_l_err_b_p;
    float v_l_rif_a_p;
    float v_l_rif_b_p;
    float v_dc_2_err_p;
    float p_bat_rif_p;
} UNIPD_DcBusControlState;

typedef struct
{
    float p_bat_rif;
    float i_l_rif_a;
    float i_l_rif_b;
    float duty_cycle_a;
    float duty_cycle_b;
} UNIPD_DcBusControlOutput;

typedef struct
{
    float i_coil_rem_err_p;
    float i_coil_rem_err_pp;
    float v_ac_rif_p;
    float v_ac_rif_pp;
    float i_coil_loc_err_p;
    float v_ac_rif_lim_p;
} UNIPD_RemoteCoilLimitControlState;

typedef struct
{
    float duty_cycle_ps_a;
    float duty_cycle_ps_b;
    float v_ac_rif;
    float v_ac_rif_lim;
} UNIPD_RemoteCoilLimitControlOutput;

/* Outer WPT power-transfer loops from Controllo_Sistema_WPT_funzione_v14.m. */
typedef struct
{
    float v_dc_2_ptrasf_err_p;
    float p_trasf_rif_p;
} UNIPD_TransferredPowerControlState;

typedef struct
{
    float p_trasf_rif;
    float i_coil_rif;
    float i_coil_err;
} UNIPD_RxTransferredPowerControlOutput;

typedef struct
{
    unsigned int counter;
    float i_l_err_a_p;
    float i_l_err_b_p;
    float v_l_rif_a_p;
    float v_l_rif_b_p;
    float v_dc_2_pbat_err_p;
    float p_bat_rif_p;
    float i_coil_rem_err_p;
    float i_coil_rem_err_pp;
    float v_ac_rif_p;
    float v_ac_rif_pp;
} UNIPD_DcBusCoilControlState;

typedef struct
{
    float i_coil_loc_err;
    float duty_cycle_pwm_a;
    float duty_cycle_pwm_b;
    unsigned int abilita_pwm;
    float duty_cycle_ps_a;
    float duty_cycle_ps_b;
    unsigned int abilita_ps;
    float p_bat_rif;
    float i_l_rif_a;
    float i_l_rif_b;
    float v_ac_rif;
} UNIPD_DcBusCoilControlOutput;

#define UNIPD_BBC_SIGNAL_V_DC             (1U << 0)
#define UNIPD_BBC_SIGNAL_V_BAT            (1U << 1)
#define UNIPD_BBC_SIGNAL_I_L_A            (1U << 2)
#define UNIPD_BBC_SIGNAL_I_L_B            (1U << 3)
#define UNIPD_BBC_SIGNAL_I_COIL_LOC       (1U << 4)
#define UNIPD_BBC_SIGNAL_I_COIL_REM_ERR   (1U << 5)
#define UNIPD_BBC_SIGNAL_I_COIL_LOC_REF   (1U << 6)
#define UNIPD_BBC_SIGNAL_V_DC_PBAT_REF    (1U << 7)
#define UNIPD_BBC_SIGNAL_I_BAT_MAX        (1U << 8)
#define UNIPD_BBC_SIGNAL_I_BAT_MIN        (1U << 9)
#define UNIPD_BBC_SIGNAL_TX_RX_MODE       (1U << 10)

#define UNIPD_BBC_REQUIRED_SIGNAL_MASK    \
    (UNIPD_BBC_SIGNAL_V_DC |              \
     UNIPD_BBC_SIGNAL_V_BAT |             \
     UNIPD_BBC_SIGNAL_I_L_A |             \
     UNIPD_BBC_SIGNAL_I_L_B |             \
     UNIPD_BBC_SIGNAL_I_COIL_LOC |        \
     UNIPD_BBC_SIGNAL_I_COIL_REM_ERR |    \
     UNIPD_BBC_SIGNAL_I_COIL_LOC_REF |    \
     UNIPD_BBC_SIGNAL_V_DC_PBAT_REF |     \
     UNIPD_BBC_SIGNAL_I_BAT_MAX |         \
     UNIPD_BBC_SIGNAL_I_BAT_MIN |         \
     UNIPD_BBC_SIGNAL_TX_RX_MODE)

typedef struct
{
    float v_dc;
    float v_bat;
    float i_l_a;
    float i_l_b;
    float i_coil_loc;
    float i_coil_rem_err;
    float i_coil_loc_rif;
    float v_dc_pbat_rif;
    float i_bat_rif_max;
    float i_bat_rif_min;
    unsigned int tx_1_rx_0;
    unsigned int valid_mask;
} UNIPD_BbcIntegrationInputs;

#define UNIPD_BBC_CAPTURE_LENGTH       32U
#define UNIPD_BBC_CAPTURE_DECIMATION   15U
#define UNIPD_WPT_CAPTURE_LENGTH       96U
#define UNIPD_WPT_CAPTURE_DECIMATION_DEFAULT 21U

typedef struct
{
    float v_dc;
    float v_bat;
    float i_l_a;
    float i_l_b;
    float i_l_rif_a;
    float i_l_rif_b;
    float i_l_err_a;
    float i_l_err_b;
    float v_l_rif_a;
    float v_l_rif_b;
    float p_bat_rif;
    float duty_raw_a;
    float duty_raw_b;
    float duty_internal_a;
    float duty_internal_b;
    float duty_mapped_a;
    float duty_mapped_b;
    float duty_applied_a;
    float duty_applied_b;
    float duty_ramped_a;
    float duty_ramped_b;
} UNIPD_BbcCaptureSample;

typedef struct
{
    uint32_t tick;
    float v_dc;
    float i_coil_physical;
    float i_coil_used;
    float p_ref;
    float i_ref;
    float i_err;
    float v_ac_ref;
    float hfc_request;
    float hfc_applied;
    float hfc_auto_hardware;
    float hfc_physical;
    unsigned int role;
    unsigned int state;
    unsigned int link;
    unsigned int hfc_fault;
    unsigned int manual;
} UNIPD_WptCaptureSample;

extern UNIPD_BbcIntegrationInputs UNIPD_bbcInputs;
extern UNIPD_DcBusCoilControlOutput UNIPD_bbcOutput;
extern unsigned int UNIPD_bbcSignalValidMask;
extern unsigned int UNIPD_bbcSignalMissingMask;
extern volatile float UNIPD_vBatMinAlg_Volts;
extern volatile float UNIPD_vBatMaxAlg_Volts;
extern volatile float UNIPD_vDcMinAlg_Volts;
extern volatile float UNIPD_vDcMaxAlg_Volts;
extern volatile float UNIPD_vdcControllerGainScale;
extern volatile unsigned int UNIPD_bbcPowerOutputEnable;
extern volatile float UNIPD_bbcPowerOutputDutyMax_pu;
extern volatile unsigned int UNIPD_bbcDutyMappingMode;
extern volatile float UNIPD_bbcMappedDutyA_pu;
extern volatile float UNIPD_bbcMappedDutyB_pu;
extern volatile float UNIPD_bbcAppliedDutyA_pu;
extern volatile float UNIPD_bbcAppliedDutyB_pu;
extern volatile unsigned int UNIPD_bbcPowerOutputDutyRampEnable;
extern volatile float UNIPD_bbcPowerOutputDutyRampStep_pu;
extern volatile float UNIPD_bbcRampedDutyA_pu;
extern volatile float UNIPD_bbcRampedDutyB_pu;
extern volatile unsigned int UNIPD_resetControlStatesCommand;
extern volatile unsigned int UNIPD_bbcVdcTripEnable;
extern volatile float UNIPD_bbcVdcTripThreshold_Volts;
extern volatile unsigned int UNIPD_bbcVdcTripLatched;
extern volatile float UNIPD_bbcVdcTripCapture_Volts;
extern volatile unsigned int UNIPD_bbcVdcTripResetCommand;
extern volatile unsigned int UNIPD_bbcILTripEnable;
extern volatile float UNIPD_bbcILTripThreshold_Amps;
extern volatile unsigned int UNIPD_bbcILTripConfirmCycles;
extern volatile unsigned int UNIPD_bbcILTripLatched;
extern volatile float UNIPD_bbcILTripCaptureA_Amps;
extern volatile float UNIPD_bbcILTripCaptureB_Amps;
extern volatile float UNIPD_bbcDebugILerrA;
extern volatile float UNIPD_bbcDebugILerrB;
extern volatile float UNIPD_bbcDebugVLrifMin;
extern volatile float UNIPD_bbcDebugVLrifMax;
extern volatile float UNIPD_bbcDebugVLrifA;
extern volatile float UNIPD_bbcDebugVLrifB;
extern volatile float UNIPD_bbcDebugDutyRawA;
extern volatile float UNIPD_bbcDebugDutyRawB;
extern volatile unsigned int UNIPD_bbcSyntheticTestEnable;
extern volatile unsigned int UNIPD_bbcSyntheticValidMask;
extern volatile unsigned int UNIPD_bbcSyntheticOverrideMask;
extern volatile UNIPD_BbcIntegrationInputs UNIPD_bbcSyntheticInputs;
extern volatile unsigned int UNIPD_bbcCaptureArmed;
extern volatile unsigned int UNIPD_bbcCaptureFrozen;
extern volatile unsigned int UNIPD_bbcCaptureCount;
extern volatile unsigned int UNIPD_bbcCaptureTriggerReason;

/* Distributed WPT integration. Computation only; no HFC PWM write is made. */
extern volatile unsigned int UNIPD_wptIntegrationEnable;
extern volatile float UNIPD_wptVdcSourceRef_Volts;
extern volatile float UNIPD_wptVdcLoadRef_Volts;
extern volatile float UNIPD_wptCoilCurrentMax_Amps;
extern volatile float UNIPD_wptCoilCurrentMin_Amps;
extern volatile float UNIPD_wptTxPowerLimit_Watts;
extern volatile float UNIPD_wptRemotePowerLimit_Watts;
extern volatile float UNIPD_wptRemoteCoilErr_Amps;
extern volatile unsigned int UNIPD_wptTxPowerSeedPending;
extern volatile float UNIPD_wptTxPowerSeed_Watts;
extern volatile unsigned int UNIPD_wptLoadPowerSeedPending;
extern volatile float UNIPD_wptLoadPowerSeed_Watts;
extern volatile unsigned int UNIPD_wptSourceCoilSyntheticEnable;
extern volatile float UNIPD_wptSourceCoilSynthetic_Amps;
extern volatile unsigned int UNIPD_wptLoadCoilSyntheticEnable;
extern volatile float UNIPD_wptLoadCoilSynthetic_Amps;
extern volatile float UNIPD_wptLoadCoilOffset_Amps;
extern volatile float UNIPD_wptLocalCoilPhysical_Amps;
extern volatile float UNIPD_wptLocalCoilUsed_Amps;
extern volatile unsigned int UNIPD_wptHfcActuatorEnable;
extern volatile unsigned int UNIPD_wptHfcActuatorFault;
extern volatile unsigned int UNIPD_wptHfcManualPhaseEnable;
extern volatile float UNIPD_wptHfcManualPhase_pu;
extern volatile uint32_t UNIPD_wptHfcRemoteRoleInvalidCycles;
extern volatile float UNIPD_wptHfcPhaseMax_pu;
extern volatile float UNIPD_wptHfcPhaseRampStep_pu;
extern volatile float UNIPD_wptHfcPhaseRequested_pu;
extern volatile float UNIPD_wptHfcPhaseApplied_pu;
extern volatile float UNIPD_wptHfcAutoHardwarePhase_pu;
extern volatile float UNIPD_wptHfcHardwarePhase_pu;
extern volatile float UNIPD_wptHfcPhasePeak_pu;
extern volatile float UNIPD_wptHfcPhaseLast_pu;
extern volatile int32_t UNIPD_wptHfcPhaseTicksLast;
extern volatile uint32_t UNIPD_wptHfcActiveCycles;
extern volatile uint32_t UNIPD_wptHfcEpwm1TbphsRawLast;
extern volatile uint32_t UNIPD_wptHfcEpwm2TbphsRawLast;
extern volatile uint16_t UNIPD_wptHfcEpwm1TbprdLast;
extern volatile uint16_t UNIPD_wptHfcEpwm2TbprdLast;
extern volatile uint16_t UNIPD_wptHfcEpwm1TbctlLast;
extern volatile uint16_t UNIPD_wptHfcEpwm2TbctlLast;
extern volatile float UNIPD_wptHfcFaultVdc_Volts;
extern UNIPD_RxTransferredPowerControlOutput UNIPD_wptRxOutput;
extern UNIPD_RemoteCoilLimitControlOutput UNIPD_wptHfcOutput;
extern volatile unsigned int UNIPD_wptCaptureArmed;
extern volatile unsigned int UNIPD_wptCaptureFrozen;
extern volatile unsigned int UNIPD_wptCaptureCount;
extern volatile unsigned int UNIPD_wptCaptureDecimation;

void UNIPD_armBbcCapture(void);
void UNIPD_stopBbcCapture(void);
unsigned int UNIPD_getBbcCaptureSample(unsigned int chronologicalIndex,
                                       UNIPD_BbcCaptureSample *sample);
void UNIPD_armWptCapture(void);
void UNIPD_stopWptCapture(void);
unsigned int UNIPD_getWptCaptureSample(unsigned int chronologicalIndex,
                                       UNIPD_WptCaptureSample *sample);
void UNIPD_runTransferredPowerIntegration(void);
void UNIPD_disableWptHfcActuator(void);

void UNIPD_resetDcBusControl(UNIPD_DcBusControlState *state);
void UNIPD_resetRemoteCoilLimitControl(UNIPD_RemoteCoilLimitControlState *state);
void UNIPD_resetTransferredPowerControl(
    UNIPD_TransferredPowerControlState *state);
void UNIPD_resetDcBusCoilControl(UNIPD_DcBusCoilControlState *state);
void UNIPD_resetAllControls(void);

void UNIPD_controlDcBus(UNIPD_DcBusControlState *state,
                        float v_dc_rif,
                        float v_dc,
                        float v_bat,
                        float i_l_a,
                        float i_l_b,
                        float i_bat_rif_max,
                        float i_bat_rif_min,
                        UNIPD_DcBusControlOutput *output);

void UNIPD_controlRemoteCoilCurrentAndLocalCoilLimit(
    UNIPD_RemoteCoilLimitControlState *state,
    float v_dc,
    float i_coil_rem_err,
    float i_coil_loc,
    UNIPD_RemoteCoilLimitControlOutput *output);

void UNIPD_controlRxDcBusViaTransferredPower(
    UNIPD_TransferredPowerControlState *state,
    float i_coil_loc,
    float p_trasf_rem_rif_lim,
    float v_dc,
    float v_dc_ptrasf_rif,
    UNIPD_RxTransferredPowerControlOutput *output);

float UNIPD_controlTxDcBusTransferredPowerLimit(
    UNIPD_TransferredPowerControlState *state,
    float v_dc,
    float v_dc_ptrasf_rif,
    float i_coil_amp_max,
    float i_coil_amp_min);

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
                                      UNIPD_DcBusCoilControlOutput *output);

/*
 * ISR hook prepared for integration with the original firmware.
 * Keep UNIPD_CONTROL_ENABLE_RUNTIME_HOOK at 0 until the application variables
 * carrying ADC-scaled measurements, BMS limits, radio data and PWM writes are
 * mapped in this function.
 */
#ifndef UNIPD_CONTROL_ENABLE_RUNTIME_HOOK
#define UNIPD_CONTROL_ENABLE_RUNTIME_HOOK 0
#endif

/*
 * Keep the power-output application disabled until the ADC synchronization,
 * BMS/radio data path and gate mapping have been verified on hardware.
 */
#ifndef UNIPD_BBC_ENABLE_POWER_OUTPUTS
#define UNIPD_BBC_ENABLE_POWER_OUTPUTS 1
#endif

void UNIPD_collectBbcIntegrationInputs(UNIPD_BbcIntegrationInputs *input);
void OBC_7_4KW_runUnipdBbcControl(void);
void OBC_7_4KW_runUnipdControlHooks(void);

#ifdef __cplusplus
}
#endif

#endif /* UNIPD_CONTROL_H */
