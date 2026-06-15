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

void OBC_7_4KW_runUnipdControlHooks(void);

#ifdef __cplusplus
}
#endif

#endif /* UNIPD_CONTROL_H */
