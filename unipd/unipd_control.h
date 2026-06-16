#ifndef UNIPD_CONTROL_H
#define UNIPD_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

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

extern UNIPD_BbcIntegrationInputs UNIPD_bbcInputs;
extern UNIPD_DcBusCoilControlOutput UNIPD_bbcOutput;
extern unsigned int UNIPD_bbcSignalValidMask;
extern unsigned int UNIPD_bbcSignalMissingMask;

void UNIPD_resetDcBusControl(UNIPD_DcBusControlState *state);
void UNIPD_resetRemoteCoilLimitControl(UNIPD_RemoteCoilLimitControlState *state);
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
#define UNIPD_BBC_ENABLE_POWER_OUTPUTS 0
#endif

void UNIPD_collectBbcIntegrationInputs(UNIPD_BbcIntegrationInputs *input);
void OBC_7_4KW_runUnipdBbcControl(void);
void OBC_7_4KW_runUnipdControlHooks(void);

#ifdef __cplusplus
}
#endif

#endif /* UNIPD_CONTROL_H */
