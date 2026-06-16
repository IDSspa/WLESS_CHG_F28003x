#include "unipd_control.h"
#include "clllc.h"
#include "ttplpfc.h"

#define UNIPD_V_BAT_MAX_ALG        (218.0f)
#define UNIPD_V_BAT_MIN_ALG        (37.5f)
#define UNIPD_V_DC_MAX_ALG         (500.0f)
#define UNIPD_V_DC_MIN_ALG         (62.5f)
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

#define UNIPD_PWM_ENABLE_DELAY     (100U)

static UNIPD_DcBusControlState unipd_dc_bus_state;
static UNIPD_RemoteCoilLimitControlState unipd_remote_coil_limit_state;
static UNIPD_DcBusCoilControlState unipd_dc_bus_coil_state;

UNIPD_BbcIntegrationInputs UNIPD_bbcInputs;
UNIPD_DcBusCoilControlOutput UNIPD_bbcOutput;
unsigned int UNIPD_bbcSignalValidMask;
unsigned int UNIPD_bbcSignalMissingMask = UNIPD_BBC_REQUIRED_SIGNAL_MASK;

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

    v_bat = unipd_clampf(v_bat, UNIPD_V_BAT_MIN_ALG, UNIPD_V_BAT_MAX_ALG);
    v_dc = unipd_clampf(v_dc, UNIPD_V_DC_MIN_ALG, UNIPD_V_DC_MAX_ALG);

    p_bat_rif_max = v_bat * i_bat_rif_max;
    p_bat_rif_min = v_bat * i_bat_rif_min;
    v_dc_2_err = (v_dc_rif * v_dc_rif) - (v_dc * v_dc);
    output->p_bat_rif = state->p_bat_rif_p +
                         (UNIPD_K_V_DC_ERR * v_dc_2_err) +
                         (UNIPD_K_V_DC_ERR_P * state->v_dc_2_err_p);
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

    v_dc = unipd_clampf(v_dc, UNIPD_V_DC_MIN_ALG, UNIPD_V_DC_MAX_ALG);
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

    v_bat = unipd_clampf(v_bat, UNIPD_V_BAT_MIN_ALG, UNIPD_V_BAT_MAX_ALG);
    v_dc = unipd_clampf(v_dc, UNIPD_V_DC_MIN_ALG, UNIPD_V_DC_MAX_ALG);

    p_bat_rif_max = v_bat * i_bat_rif_max;
    p_bat_rif_min = v_bat * i_bat_rif_min;
    v_dc_pbat_2_err = (v_dc_pbat_rif * v_dc_pbat_rif) - (v_dc * v_dc);
    output->p_bat_rif = state->p_bat_rif_p +
                         (UNIPD_K_V_DC_ERR * v_dc_pbat_2_err) +
                         (UNIPD_K_V_DC_ERR_P * state->v_dc_2_pbat_err_p);
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
    CLLLC_readPrimaryTankMagnitudeAndPhase();

    input->v_dc = TTPLPFC_vBus_sensed_pu * TTPLPFC_VDCBUS_MAX_SENSE;
    input->i_l_a = TTPLPFC_iL1_sensed_pu * TTPLPFC_IL_MAX_SENSE;
    input->i_l_b = TTPLPFC_iL2_sensed_pu * TTPLPFC_IL_MAX_SENSE;
    input->i_coil_loc =
            CLLLC_iPrimTankModSensed_pu * CLLLC_IPRIM_TANK_MAX_SENSE_AMPS;

    input->valid_mask |= UNIPD_BBC_SIGNAL_V_DC |
                         UNIPD_BBC_SIGNAL_I_L_A |
                         UNIPD_BBC_SIGNAL_I_L_B |
                         UNIPD_BBC_SIGNAL_I_COIL_LOC;
}

void OBC_7_4KW_runUnipdBbcControl(void)
{
    UNIPD_collectBbcIntegrationInputs(&UNIPD_bbcInputs);

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
       (UNIPD_bbcOutput.abilita_pwm != 0U))
    {
        TTPLPFC_BBC_setMode(UNIPD_bbcInputs.tx_1_rx_0 != 0U ?
                            TTPLPFC_BBC_MODE_BOOST :
                            TTPLPFC_BBC_MODE_BUCK);
        TTPLPFC_BBC_setDuty(UNIPD_bbcOutput.duty_cycle_pwm_a,
                            UNIPD_bbcOutput.duty_cycle_pwm_b);
        TTPLPFC_BBC_enable();
    }
    else
#endif
    {
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
