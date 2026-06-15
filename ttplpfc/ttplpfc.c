//#############################################################################
//
// FILE:   ttplpfc.c
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

//
// the includes
//

#include "ttplpfc.h"

//
// System Related
//
#pragma SET_DATA_SECTION("controlVariables")

//
// RAMP to generate forced angle when grid is not presentting
//
RAMPGEN TTPLPFC_rgen;

TTPLPFC_GI_STRUCT TTPLPFC_gi1;
TTPLPFC_GI_STRUCT TTPLPFC_gi2;
TTPLPFC_GV_STRUCT TTPLPFC_gv;

#pragma RETAIN (TTPLPFC_gi1_out_ff_scale)
float32_t TTPLPFC_gi1_out_ff_scale;
float32_t TTPLPFC_gi_Kp;
float32_t TTPLPFC_gi_Ki;
float32_t TTPLPFC_gi_Kp_Ref;
float32_t TTPLPFC_gi_Ki_Ref;
uint32_t TTPLPFC_updateCoefficientsCommand;

SPLL_1PH_SOGI TTPLPFC_spll1;
SPLL_1PH_NOTCH TTPLPFC_spll2;
SPLL_1PH_SOGI_FLL TTPLPFC_spll3;

TTPLPFC_NOTCH_2P2Z_STRUCT TTPLPFC_notch1;
TTPLPFC_NOTCH_2P2Z_STRUCT TTPLPFC_notch2;
TTPLPFC_NOTCH_2P2Z_STRUCT TTPLPFC_notch_50Hz;
TTPLPFC_NOTCH_2P2Z_STRUCT TTPLPFC_notch_60Hz;
TTPLPFC_NOTCH_2P2Z_STRUCT TTPLPFC_spll_notch_50Hz;
TTPLPFC_NOTCH_2P2Z_STRUCT TTPLPFC_spll_notch_60Hz;

//
// Sine analyzer block for RMS Volt, Curr and Power measurements
//
POWER_MEAS_SINE_ANALYZER TTPLPFC_sine_mains;

uint16_t TTPLPFC_guiPowerStageStart;
uint16_t TTPLPFC_guiPowerStageStop;

#pragma RETAIN(TTPLPFC_lab)
#pragma RETAIN(TTPLPFC_board_State)
#pragma RETAIN(TTPLPFC_board_Status)
TTPLPFC_Lab_EnumType TTPLPFC_lab;
TTPLPFC_boardState TTPLPFC_board_State;
TTPLPFC_boardStatus TTPLPFC_board_Status;

//
// Inductor Current Measurement
//
float32_t TTPLPFC_iL1_sensed_pu;
float32_t TTPLPFC_iL2_sensed_pu;

float32_t TTPLPFC_iL1_senseOffset_pu;
float32_t TTPLPFC_iL2_senseOffset_pu;

float32_t TTPLPFC_iL1_sensed_Amps;
float32_t TTPLPFC_iL2_sensed_Amps;

//
// AC Current Measurement
//
float32_t TTPLPFC_ac_cur_sensed_pu;
float32_t TTPLPFC_ac_cur_sensed_Amps;

//
// AC Grid Sync offset
//
float32_t TTPLPFC_thetaOffset_pu;

//
// Output Voltage measurement
//
#pragma RETAIN (TTPLPFC_vBus_sensed_pu)
#pragma RETAIN (TTPLPFC_vBus_sensedOffset_pu)
#pragma RETAIN (TTPLPFC_vBus_sensed_Volts)
float32_t TTPLPFC_vBus_sensed_pu;
float32_t TTPLPFC_vBus_sensedOffset_pu;
float32_t TTPLPFC_vBus_sensed_Volts;

//
// Input voltage measurement
//
#pragma RETAIN (TTPLPFC_ac_L_sensed_pu)
#pragma RETAIN (TTPLPFC_ac_N_sensed_pu)
#pragma RETAIN (TTPLPFC_ac_vol_sensed_pu)
#pragma RETAIN (TTPLPFC_ac_vol_sensed_Volts)
#pragma RETAIN (TTPLPFC_ac_L_senseOffset_pu)
#pragma RETAIN (TTPLPFC_ac_N_senseOffset_pu)
#pragma RETAIN (TTPLPFC_ac_vol_senseOffset_pu)
float32_t TTPLPFC_ac_L_sensed_pu;
float32_t TTPLPFC_ac_N_sensed_pu;
float32_t TTPLPFC_ac_vol_sensed_pu;
float32_t TTPLPFC_ac_vol_sensed_Volts;
float32_t TTPLPFC_ac_L_senseOffset_pu;
float32_t TTPLPFC_ac_N_senseOffset_pu;
float32_t TTPLPFC_ac_vol_senseOffset_pu;

#pragma RETAIN (TTPLPFC_system_temp_pu)
#pragma RETAIN (TTPLPFC_system_vref_165_pu)
#pragma RETAIN (TTPLPFC_vbus2_pu)
#pragma RETAIN (TTPLPFC_vol2_pu)
float32_t TTPLPFC_system_temp_pu;
float32_t TTPLPFC_system_vref_165_pu;
float32_t TTPLPFC_vbus2_pu;
float32_t TTPLPFC_vol2_pu;

//
// Filtered DC bus measurement
//
#pragma RETAIN (TTPLPFC_vBusAvg_pu)
#pragma RETAIN (TTPLPFC_vRmsAvg_pu)
float32_t TTPLPFC_vBusAvg_pu;
float32_t TTPLPFC_vRmsAvg_pu;

#pragma RETAIN (TTPLPFC_threshold_PZC1)
#pragma RETAIN (TTPLPFC_threshold_PZC2)
#pragma RETAIN (TTPLPFC_threshold_NZC1)
#pragma RETAIN (TTPLPFC_threshold_NZC2)
float32_t TTPLPFC_threshold_PZC1;
float32_t TTPLPFC_threshold_PZC2;
float32_t TTPLPFC_threshold_NZC1;
float32_t TTPLPFC_threshold_NZC2;

//
// Current set point
//
#pragma RETAIN (TTPLPFC_ac_cur_ref_pu)
float32_t TTPLPFC_ac_cur_ref_pu;
float32_t TTPLPFC_ac_cur_ref_inst_pu;
float32_t TTPLPFC_ac_cur_ref_inst_prev_pu;
#pragma RETAIN (TTPLPFC_inductor_voltage_drop_feedforward)
float32_t TTPLPFC_inductor_voltage_drop_feedforward;

//
// Voltage set point
//
#pragma RETAIN (TTPLPFC_vBusRef_pu)
#pragma RETAIN (TTPLPFC_vBusRefSlewed_pu)
float32_t TTPLPFC_vBusRef_pu;
float32_t TTPLPFC_vBusRefSlewed_pu;

//
// Adaptive Dead Time variables
// 10 corresponds to 100ns dead time
//
uint32_t TTPLPFC_dbRED_SetValue_ticks;
uint32_t TTPLPFC_dbRED_SetValue_Temp_ticks;
uint32_t TTPLPFC_dbFED_SetValue_ticks;
uint32_t TTPLPFC_dbRED_SetValue1_ticks;
uint32_t TTPLPFC_dbRED_SetValue_Temp1_ticks;
uint32_t TTPLPFC_dbRED_SetValue2_ticks;
uint32_t TTPLPFC_dbRED_SetValue_Temp2_ticks;
uint32_t TTPLPFC_dbRED_SetValue3_ticks;
uint32_t TTPLPFC_dbRED_SetValue_Temp3_ticks;
uint32_t TTPLPFC_dbRED_Min_ticks;
uint32_t TTPLPFC_dbRED_Max_ticks;

#pragma RETAIN (TTPLPFC_softstart_duty)
#pragma RETAIN (TTPLPFC_softStartDeadbandScalingFactor)
float32_t TTPLPFC_softstart_duty;
float32_t TTPLPFC_softStartDeadbandScalingFactor;

//
// Display Values
//

#pragma RETAIN (TTPLPFC_vBus_overVoltage_Volts)
#pragma RETAIN (TTPLPFC_vBus_underVoltage_Volts)
#pragma RETAIN (TTPLPFC_duty1_pu)
#pragma RETAIN (TTPLPFC_duty2_pu)
#pragma RETAIN (TTPLPFC_duty_clamp_high_pu)
#pragma RETAIN (TTPLPFC_duty_clamp_low_pu)
#pragma RETAIN (TTPLPFC_duty_DC_ref_pu)
#pragma RETAIN (TTPLPFC_duty_DC_ref_final_pu)
#pragma RETAIN (TTPLPFC_duty_ref_pu)
#pragma RETAIN (TTPLPFC_powerRms_Watts)
#pragma RETAIN (TTPLPFC_ac_curRms_sensed_Amps)
#pragma RETAIN (TTPLPFC_ac_volRms_sensed_Volts)
#pragma RETAIN (TTPLPFC_ac_volRms_overVoltage_Volts)
#pragma RETAIN (TTPLPFC_ac_volRms_underVoltage_Volts)
#pragma RETAIN (TTPLPFC_powerFactor)
#pragma RETAIN (TTPLPFC_apparentPower_VA)
#pragma RETAIN (TTPLPFC_acFreqAvg_Hz)
#pragma RETAIN (TTPLPFC_ac_volEma_sensed_Volts)
#pragma RETAIN (TTPLPFC_ac_volRmsEma_sensed_Volts)
#pragma RETAIN (TTPLPFC_autoStartSlew)
float32_t TTPLPFC_vBus_overVoltage_Volts;
float32_t TTPLPFC_vBus_underVoltage_Volts;
float32_t TTPLPFC_duty1_pu;
float32_t TTPLPFC_duty2_pu;
float32_t TTPLPFC_duty_clamp_high_pu;
float32_t TTPLPFC_duty_clamp_low_pu;
float32_t TTPLPFC_duty_DC_ref_pu;
float32_t TTPLPFC_duty_DC_ref_final_pu;
float32_t TTPLPFC_duty_ref_pu;
volatile TTPLPFC_BBC_Mode TTPLPFC_bbcMode;
volatile int32_t TTPLPFC_bbcEnabled;
float32_t TTPLPFC_powerRms_Watts;
float32_t TTPLPFC_ac_curRms_sensed_Amps;
float32_t TTPLPFC_ac_volRms_sensed_Volts;
float32_t TTPLPFC_ac_volRms_overVoltage_Volts;
float32_t TTPLPFC_ac_volRms_underVoltage_Volts;
float32_t TTPLPFC_powerFactor;
float32_t TTPLPFC_apparentPower_VA;
float32_t TTPLPFC_acFreqAvg_Hz;
float32_t TTPLPFC_ac_volEma_sensed_Volts;
float32_t TTPLPFC_ac_volRmsEma_sensed_Volts;
int32_t TTPLPFC_autoStartSlew;

//
// Phase-Shedding Variables
//
#pragma RETAIN (TTPLPFC_iref1_pu)
#pragma RETAIN (TTPLPFC_iref2_pu)
#pragma RETAIN (TTPLPFC_deltaIref_pu)
float32_t TTPLPFC_iref1_pu;
float32_t TTPLPFC_iref2_pu;
float32_t TTPLPFC_deltaIref_pu;

//
// per unit duty value
//
#pragma RETAIN (TTPLPFC_duty1_pu)
#pragma RETAIN (TTPLPFC_duty1_pu)
float32_t TTPLPFC_duty1_pu;
float32_t TTPLPFC_duty1_pu;

//
// Flags for clearing trips and closing the loops and the Relay
//
#pragma RETAIN (TTPLPFC_closeGiLoop)
#pragma RETAIN (TTPLPFC_closeGvLoop)
#pragma RETAIN (TTPLPFC_clearTrip)
#pragma RETAIN (TTPLPFC_rlyConnect)
#pragma RETAIN (TTPLPFC_firstTimeGvLoop)
int32_t TTPLPFC_closeGiLoop;
int32_t TTPLPFC_closeGvLoop;
int32_t TTPLPFC_clearTrip;
int32_t TTPLPFC_rlyConnect;
int32_t TTPLPFC_firstTimeGvLoop;

//
// Flags for detecting ZCD
//
#pragma RETAIN (TTPLPFC_acSine)
#pragma RETAIN (TTPLPFC_acSinePrev)
#pragma RETAIN (TTPLPFC_acCosine)
#pragma RETAIN (TTPLPFC_acCosinePrev)
#pragma RETAIN (TTPLPFC_zeroCrossDetectFlag)
float32_t TTPLPFC_acSine;
float32_t TTPLPFC_acSinePrev;
float32_t TTPLPFC_acCosine;
float32_t TTPLPFC_acCosinePrev;
int32_t TTPLPFC_zeroCrossDetectFlag;

TTPLPFC_pwmSwState TTPLPFC_pwm_SwState;
#pragma RETAIN (TTPLPFC_interleaving_State)
#pragma RETAIN (TTPLPFC_state_slew)
#pragma RETAIN (TTPLPFC_state_slew_max)

TTPLPFC_interleavingState TTPLPFC_interleaving_State;
int32_t TTPLPFC_state_slew;
int32_t TTPLPFC_state_slew_max;


#pragma RETAIN (TTPLPFC_gi1_out)
#pragma RETAIN (TTPLPFC_gi2_out)
#pragma RETAIN (TTPLPFC_gv_out)

 float32_t TTPLPFC_gi1_out;
 float32_t TTPLPFC_gi2_out;
 float32_t TTPLPFC_gv_out;


#pragma RETAIN (TTPLPFC_vBusSensedBuff)
#pragma RETAIN (TTPLPFC_vBus_sensedFiltered)
#pragma RETAIN (TTPLPFC_vBusSensedBuffIndex)

 float32_t TTPLPFC_vBusSensedBuff[10];
 float32_t TTPLPFC_vBus_sensedFiltered;
 int32_t TTPLPFC_vBusSensedBuffIndex = 0;


#pragma RETAIN (TTPLPFC_vBus_sensedFiltered_notch1)
#pragma RETAIN (TTPLPFC_vBus_sensedFiltered_notch2)

 float32_t TTPLPFC_vBus_sensedFiltered_notch1;
 float32_t TTPLPFC_vBus_sensedFiltered_notch2;


#pragma RETAIN (TTPLPFC_ac_vol_sensed_buff)
#pragma RETAIN (TTPLPFC_ac_vol_sensed_buff_index)
#pragma RETAIN (TTPLPFC_ac_sign_filtered)

 float32_t TTPLPFC_ac_vol_sensed_buff[5];
 int32_t TTPLPFC_ac_vol_sensed_buff_index = 0;
 int32_t TTPLPFC_ac_sign_filtered;


#pragma RETAIN (TTPLPFC_spll_sine)
#pragma RETAIN (TTPLPFC_spll_cosine)
#pragma RETAIN (TTPLPFC_voltage_error)

 float32_t TTPLPFC_spll_sine;
 float32_t TTPLPFC_spll_cosine;
 float32_t TTPLPFC_voltage_error;

//
// 1 if using non linear voltage loop, 0 otherwise
//
#pragma RETAIN (TTPLPFC_nonLinearVoltageLoopFlag)
#pragma RETAIN (TTPLPFC_nonLinearVoltageLoopFlagDelay)

 uint32_t TTPLPFC_nonLinearVoltageLoopFlag;
 uint32_t TTPLPFC_nonLinearVoltageLoopFlagDelay;


#pragma RETAIN (TTPLPFC_softStartDeadBandFED)

 uint32_t TTPLPFC_softStartDeadBandFED;


#pragma RETAIN (TTPLPFC_manualStop)
#pragma RETAIN (TTPLPFC_timedRun_flag)
#pragma RETAIN (TTPLPFC_timedRun_ms)
#pragma RETAIN (TTPLPFC_timedRunCounter_ms)
#pragma RETAIN (TTPLPFC_dutySlew_pu)

 uint32_t TTPLPFC_manualStop;
 uint32_t TTPLPFC_timedRun_flag;
 float32_t TTPLPFC_timedRun_ms;
 float32_t TTPLPFC_timedRunCounter_ms;
 float32_t TTPLPFC_dutySlew_pu;

//
// Variables used to calibrate measurement offsets
// Offset filter coefficient K1: 0.05/(T+0.05);
//
float32_t k1 = 0.998f;

//
//Offset filter coefficient K2: T/(T+0.05)
//
float32_t k2 = 0.001999f;

#pragma RETAIN (TTPLPFC_phase1ScaleFactor)
#pragma RETAIN (TTPLPFC_phase2ScaleFactor)
#pragma RETAIN (TTPLPFC_phase3ScaleFactor)

 float32_t TTPLPFC_phase1ScaleFactor = 1.0f;
 float32_t TTPLPFC_phase2ScaleFactor = 1.0f;
 float32_t TTPLPFC_phase3ScaleFactor = 1.0f;

////
//// Variable to log how many times ISR1 is nested from
////
#pragma RETAIN (TTPLPFC_ISR1_nestingCounter)
#pragma RETAIN (TTPLPFC_ISR1_nests)
#pragma RETAIN (TTPLPFC_ISR1_nestsMax)
#pragma RETAIN (TTPLPFC_ISR1_nestsAvg)

 int32_t TTPLPFC_ISR1_nestingCounter;
 int32_t TTPLPFC_ISR1_nests;
 int32_t TTPLPFC_ISR1_nestsMax;
 float32_t TTPLPFC_ISR1_nestsAvg;

//
// eCAP Profiling related variables
//

//
// Variables to log the capture value for profiling ISR1
//
uint32_t  TTPLPFC_ISR1cap1Count;
uint32_t  TTPLPFC_ISR1cap2Count;
uint32_t  TTPLPFC_ISR1cap3Count;
uint32_t  TTPLPFC_ISR1cap4Count;

//
// Variable for logging the the max value and also computing the average for ISR1
//
uint32_t TTPLPFC_ISR1_capCountMax;
float32_t TTPLPFC_ISR1cap1CountAvg;
float32_t TTPLPFC_ISR2_LoadingAvg_accountingForNesting;

//
// Variables to compute the loading percentage for ISR1
//
float32_t TTPLPFC_ISR1_Loading;
float32_t TTPLPFC_ISR1_LoadingMax;

//
// Variables to log the capture value for profiling ISR2
//
uint32_t  TTPLPFC_ISR2cap1Count;
uint32_t  TTPLPFC_ISR2cap2Count;
uint32_t  TTPLPFC_ISR2cap3Count;
uint32_t  TTPLPFC_ISR2cap4Count;

//
// Variables to compute the loading percentage for ISR2
// subtracts the ISR1 nesting cycles
//
float32_t TTPLPFC_ISR2_Loading;
float32_t TTPLPFC_ISR2_LoadingAvg;

#pragma SET_DATA_SECTION()

//
// datalogger
//
DLOG_4CH TTPLPFC_dLog1;
float32_t TTPLPFC_dBuff1[100];
float32_t TTPLPFC_dBuff2[100];
float32_t TTPLPFC_dBuff3[100];
float32_t TTPLPFC_dBuff4[100];
float32_t TTPLPFC_dVal1;
float32_t TTPLPFC_dVal2;
float32_t TTPLPFC_dVal3;
float32_t TTPLPFC_dVal4;

//
// SFRA Related Variables
//
#if TTPLPFC_SFRA_TYPE != TTPLPFC_SFRA_DISABLED
float32_t TTPLPFC_plantMagVect[TTPLPFC_SFRA_FREQ_LENGTH];
float32_t TTPLPFC_plantPhaseVect[TTPLPFC_SFRA_FREQ_LENGTH];
float32_t TTPLPFC_olMagVect[TTPLPFC_SFRA_FREQ_LENGTH];
float32_t TTPLPFC_olPhaseVect[TTPLPFC_SFRA_FREQ_LENGTH];
float32_t TTPLPFC_freqVect[TTPLPFC_SFRA_FREQ_LENGTH];
#endif

SFRA_F32 TTPLPFC_sfra1;

#if TTPLPFC_SFRA_TYPE != TTPLPFC_SFRA_DISABLED
void TTPLPFC_setupSFRA(void)
{
#if    TTPLPFC_SFRA_TYPE == TTPLPFC_SFRA_CURRENT// current loop
    int16_t i;
#endif
    SFRA_F32_reset(&TTPLPFC_sfra1);

    SFRA_F32_config(&TTPLPFC_sfra1,
                    TTPLPFC_SFRA_ISR_FREQ,
                    TTPLPFC_SFRA_AMPLITUDE,
                    TTPLPFC_SFRA_FREQ_LENGTH,
                    TTPLPFC_SFRA_FREQ_START,
                    TTPLPFC_SFRA_FREQ_STEP_MULTIPLY,
                    TTPLPFC_plantMagVect,
                    TTPLPFC_plantPhaseVect,
                    TTPLPFC_olMagVect,
                    TTPLPFC_olPhaseVect,
                    NULL,
                    NULL,
                    TTPLPFC_freqVect,
                    1);

    SFRA_F32_resetFreqRespArray(&TTPLPFC_sfra1);

    //
    // Re-initialize the frequency array to make SFRA sweep go fast
    //


    #if TTPLPFC_SFRA_TYPE == TTPLPFC_SFRA_CURRENT// current loop
        i = 0;
        TTPLPFC_sfra1.freqVect[i++] = TTPLPFC_SFRA_FREQ_START;
        for(;i < TTPLPFC_sfra1.vecLength;i++)
        {
            if(TTPLPFC_sfra1.freqVect[i - 1] < 10)
                TTPLPFC_sfra1.freqVect[i] =
                        TTPLPFC_sfra1.freqVect[i - 1] + 2;
            else if(TTPLPFC_sfra1.freqVect[i - 1] < 50)
                TTPLPFC_sfra1.freqVect[i] =
                        TTPLPFC_sfra1.freqVect[i - 1] + 10;
            else
                TTPLPFC_sfra1.freqVect[i] =
                        TTPLPFC_sfra1.freqVect[i - 1] * TTPLPFC_sfra1.freqStep;
        }
    #else
        TTPLPFC_sfra1.freqVect[0] = 2;
        TTPLPFC_sfra1.freqVect[1] = 4;
        TTPLPFC_sfra1.freqVect[2] = 6;
        TTPLPFC_sfra1.freqVect[3] = 8;
        TTPLPFC_sfra1.freqVect[4] = 10;
        TTPLPFC_sfra1.freqVect[5] = 12;
        TTPLPFC_sfra1.freqVect[6] = 14;
        TTPLPFC_sfra1.freqVect[7] = 16;
        TTPLPFC_sfra1.freqVect[8] = 18;
        TTPLPFC_sfra1.freqVect[9] = 20;
        TTPLPFC_sfra1.freqVect[10] = 22;
        TTPLPFC_sfra1.freqVect[11] = 24;
        TTPLPFC_sfra1.freqVect[12] = 26;
        TTPLPFC_sfra1.freqVect[13] = 28;
        TTPLPFC_sfra1.freqVect[14] = 30;
        TTPLPFC_sfra1.freqVect[15] = 35;
        TTPLPFC_sfra1.freqVect[16] = 40;
        TTPLPFC_sfra1.freqVect[17] = 45;
        TTPLPFC_sfra1.freqVect[18] = 55;
        TTPLPFC_sfra1.freqVect[19] = 65;
        TTPLPFC_sfra1.freqVect[20] = 70;
        TTPLPFC_sfra1.freqVect[21] = 80;
        TTPLPFC_sfra1.freqVect[22] = 90;
        TTPLPFC_sfra1.freqVect[23] = 130;
        TTPLPFC_sfra1.freqVect[24] = 140;
        TTPLPFC_sfra1.freqVect[25] = 150;
        TTPLPFC_sfra1.freqVect[26] = 160;
        TTPLPFC_sfra1.freqVect[27] = 170;
        TTPLPFC_sfra1.freqVect[28] = 210;
        TTPLPFC_sfra1.freqVect[29] = 250;
    #endif

    //
    //configures the SCI channel for communication with SFRA host GUI
    //to change SCI channel change #defines in the settings.h file
    //the GUI also changes a LED status, this can also be changed with #define
    //in the file pointed to above
    //
    SFRA_GUI_config(TTPLPFC_SFRA_GUI_SCI_BASE,
                    TTPLPFC_SCI_VBUS_CLK,
                    TTPLPFC_SFRA_GUI_SCI_BAUDRATE,
                    TTPLPFC_SFRA_GUI_SCIRX_GPIO,
                    TTPLPFC_SFRA_GUI_SCIRX_GPIO_PIN_CONFIG,
                    TTPLPFC_SFRA_GUI_SCITX_GPIO,
                    TTPLPFC_SFRA_GUI_SCITX_GPIO_PIN_CONFIG,
                    TTPLPFC_SFRA_GUI_LED_INDICATOR,
                    TTPLPFC_SFRA_GUI_LED_GPIO,
                    TTPLPFC_SFRA_GUI_LED_GPIO_PIN_CONFIG,
                    &TTPLPFC_sfra1,
                    TTPLPFC_SFRA_GUI_PLOT_OL_PLANT);
}
#endif


void TTPLPFC_globalVariablesInit(void)
{

    TTPLPFC_gi1_out_ff_scale = 1.0f;
    RAMPGEN_reset(&TTPLPFC_rgen);
    RAMPGEN_config(&TTPLPFC_rgen,
                   TTPLPFC_CONTROL_ISR_FREQUENCY,
                   TTPLPFC_AC_FREQ);

    TTPLPFC_gi1.Kp = TTPLPFC_GI_PI_KP;
    TTPLPFC_gi1.Ki = TTPLPFC_GI_PI_KI;
    TTPLPFC_gi1.Umax = TTPLPFC_GI_PI_MAX;
    TTPLPFC_gi1.Umin = TTPLPFC_GI_PI_MIN;
    TTPLPFC_gi1.i10 = 0;
    TTPLPFC_gi1.i6 = 0;

    TTPLPFC_gi2.Kp = TTPLPFC_GI_PI_KP;
    TTPLPFC_gi2.Ki = TTPLPFC_GI_PI_KI;
    TTPLPFC_gi2.Umax = TTPLPFC_GI_PI_MAX;
    TTPLPFC_gi2.Umin = TTPLPFC_GI_PI_MIN;
    TTPLPFC_gi2.i10 = 0;
    TTPLPFC_gi2.i6 = 0;

    TTPLPFC_gv.Ki = TTPLPFC_GV_PI_KI;
    TTPLPFC_gv.Kp = TTPLPFC_GV_PI_KP;
    TTPLPFC_gv.Umax = TTPLPFC_GV_PI_MAX;
    TTPLPFC_gv.Umin = TTPLPFC_GV_PI_MIN;
    TTPLPFC_gv.i10 = 0;
    TTPLPFC_gv.i6 = 0;

    TTPLPFC_gi_Kp = TTPLPFC_GI_PI_KP;
    TTPLPFC_gi_Ki = TTPLPFC_GI_PI_KI;
    TTPLPFC_gi_Kp_Ref = TTPLPFC_GI_PI_KP;
    TTPLPFC_gi_Ki_Ref = TTPLPFC_GI_PI_KI;
    TTPLPFC_updateCoefficientsCommand = 0;

    SPLL_1PH_SOGI_reset(&TTPLPFC_spll1);
    SPLL_1PH_SOGI_config(&TTPLPFC_spll1,
                         TTPLPFC_AC_FREQ,
                         TTPLPFC_CONTROL_ISR_FREQUENCY,
                         (float32_t)(222.2862),
                         (float32_t)(-222.034));

    SPLL_1PH_NOTCH_reset(&TTPLPFC_spll2);
    SPLL_1PH_NOTCH_config(&TTPLPFC_spll2,
                          TTPLPFC_AC_FREQ,
                          TTPLPFC_CONTROL_ISR_FREQUENCY,
                          (float32_t)(222.2862),
                          (float32_t)(-222.034),
                          (float32_t) 0.25,
                          (float32_t) 0.00001);

    SPLL_1PH_SOGI_FLL_reset(&TTPLPFC_spll3);
    SPLL_1PH_SOGI_FLL_config(&TTPLPFC_spll3,
                             TTPLPFC_AC_FREQ,
                             TTPLPFC_CONTROL_ISR_FREQUENCY,
                             (float32_t)(222.2862),
                             (float32_t)(-222.034),
                             (float32_t) 0.5,
                             (float32_t) 20000);

    //
    // Sine analyzer initialization
    //
    POWER_MEAS_SINE_ANALYZER_reset(&TTPLPFC_sine_mains);
    POWER_MEAS_SINE_ANALYZER_config(&TTPLPFC_sine_mains,
                                    TTPLPFC_INSTRUMENTATION_ISR_FREQUENCY,
                                    (float32_t)0.08,
                                    (float32_t)TTPLPFC_GRID_MAX_FREQ,
                                    (float32_t)TTPLPFC_GRID_MIN_FREQ);

    TTPLPFC_computeNotchFltrCoeff(&TTPLPFC_notch1,
                           (float32_t)(TTPLPFC_INSTRUMENTATION_ISR_FREQUENCY),
                           (float32_t)(TTPLPFC_AC_FREQ*2.0) , 0.25f, 0.00001f);
    TTPLPFC_computeNotchFltrCoeff(&TTPLPFC_notch2,
                           (float32_t)(TTPLPFC_INSTRUMENTATION_ISR_FREQUENCY),
                           (float32_t)(TTPLPFC_AC_FREQ*2.0) , 0.25f, 0.00001f);
    TTPLPFC_notch1.x1 = 0;
    TTPLPFC_notch1.x2 = 0;
    TTPLPFC_notch2.x1 = 0;
    TTPLPFC_notch2.x2 = 0;

    DLOG_4CH_reset(&TTPLPFC_dLog1);
    DLOG_4CH_config(&TTPLPFC_dLog1,
                    &TTPLPFC_dVal1, &TTPLPFC_dVal2,
                    &TTPLPFC_dVal3, &TTPLPFC_dVal4,
                    TTPLPFC_dBuff1, TTPLPFC_dBuff2,
                    TTPLPFC_dBuff3, TTPLPFC_dBuff4,
                    100, 0.1f, 4);

    TTPLPFC_guiPowerStageStart = 0;
    TTPLPFC_guiPowerStageStop = 0;

    TTPLPFC_vBus_sensed_pu = 0;
    TTPLPFC_powerRms_Watts = 0;
    TTPLPFC_ac_curRms_sensed_Amps = 0;
    TTPLPFC_ac_volRms_sensed_Volts = 0;
    TTPLPFC_ac_volRms_overVoltage_Volts = 270;

    TTPLPFC_acFreqAvg_Hz = 0;
    TTPLPFC_ac_volEma_sensed_Volts = 0;
    TTPLPFC_vBus_overVoltage_Volts = 480;
    TTPLPFC_ac_volRmsEma_sensed_Volts = 0;

    //
    //A.D.T Variables
    //
    TTPLPFC_dbFED_SetValue_ticks =
            TTPLPFC_HIGH_FREQ_PWM_DEADBAND_FED_COUNT;

    TTPLPFC_dbRED_SetValue_ticks =
            TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_COUNT;
    TTPLPFC_dbRED_SetValue_Temp_ticks =
            TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_COUNT;

    TTPLPFC_dbRED_SetValue1_ticks =
            TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_COUNT;
    TTPLPFC_dbRED_SetValue_Temp1_ticks =
            TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_COUNT;

    TTPLPFC_dbRED_SetValue2_ticks =
            TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_COUNT;
    TTPLPFC_dbRED_SetValue_Temp2_ticks =
            TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_COUNT;

    TTPLPFC_dbRED_SetValue3_ticks =
            TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_COUNT;
    TTPLPFC_dbRED_SetValue_Temp3_ticks =
            TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_COUNT;

    TTPLPFC_dbRED_Min_ticks = TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_MIN_COUNT;
    TTPLPFC_dbRED_Max_ticks = TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_MAX_COUNT;

    //
    //Variable initialization
    //
    TTPLPFC_iL1_sensed_pu = 0;
    TTPLPFC_iL1_senseOffset_pu = 0;

    TTPLPFC_iL2_sensed_pu = 0;
    TTPLPFC_iL2_senseOffset_pu = 0;

    TTPLPFC_ac_cur_sensed_pu = 0;

    TTPLPFC_vBus_sensed_pu = 0;
    TTPLPFC_vBus_sensedOffset_pu = 0;
    TTPLPFC_vBusRef_pu = TTPLPFC_VBUS_TYPICAL / TTPLPFC_VDCBUS_MAX_SENSE;



    TTPLPFC_ac_L_sensed_pu = 0;
    TTPLPFC_ac_L_senseOffset_pu = 0;

    TTPLPFC_ac_N_sensed_pu = 0;
    TTPLPFC_ac_N_senseOffset_pu = 0;

    TTPLPFC_ac_vol_senseOffset_pu = 0.5;

    TTPLPFC_duty1_pu = 0;
    TTPLPFC_duty2_pu = 0;
    TTPLPFC_bbcMode = TTPLPFC_BBC_MODE_DISABLED;
    TTPLPFC_bbcEnabled = 0;

    TTPLPFC_closeGiLoop = 0;
    TTPLPFC_closeGvLoop = 0;
    TTPLPFC_clearTrip = 0;
    TTPLPFC_rlyConnect = 0;
    TTPLPFC_firstTimeGvLoop = 1;
    TTPLPFC_acSine = 0;
    TTPLPFC_acSinePrev = 0;
    TTPLPFC_zeroCrossDetectFlag = 0;

    TTPLPFC_vBus_sensedFiltered_notch1 = 0;
    TTPLPFC_vBus_sensedFiltered_notch2 = 0;

    TTPLPFC_threshold_PZC1 = TTPLPFC_PWM_SWITCHING_PZC1;
    TTPLPFC_threshold_PZC2 = TTPLPFC_PWM_SWITCHING_PZC2;
    TTPLPFC_threshold_NZC1 = TTPLPFC_PWM_SWITCHING_NZC1;
    TTPLPFC_threshold_NZC2 = TTPLPFC_PWM_SWITCHING_NZC2;


    TTPLPFC_autoStartSlew = 0;

    TTPLPFC_vRmsAvg_pu = 0;
    TTPLPFC_vBusAvg_pu = 0;

    TTPLPFC_duty_ref_pu = 0.5f;
    TTPLPFC_duty_DC_ref_pu = 0.5f;


#if TTPLPFC_DC_CHECK == 1
#if TTPLPFC_AC_HALF_CHECK == TTPLPFC_POSITIVE_HALF_DC_CHECK
    TTPLPFC_duty_clamp_high_pu = 1.0f;
    TTPLPFC_duty_clamp_low_pu = 0.0f;
    TTPLPFC_ac_cur_ref_pu = 0.03f;
    TTPLPFC_duty_DC_ref_pu = 0.5f;
#else
    TTPLPFC_duty_clamp_high_pu = 0.0f;
    TTPLPFC_duty_clamp_low_pu = -1.0f;
    TTPLPFC_ac_cur_ref_pu = -0.03f;
    TTPLPFC_duty_DC_ref_pu = -0.5f;
#endif
#else
    TTPLPFC_duty_clamp_high_pu = 1.0f;
    TTPLPFC_duty_clamp_low_pu = -1.0f;
    TTPLPFC_ac_cur_ref_pu = 0.03f;
#endif

    TTPLPFC_thetaOffset_pu = -0.02f;

    TTPLPFC_pwm_SwState.enum_pwmSwState = pwmSwState_defaultState;

    //
    // Interleaving variable and state Initilazation
    //
    TTPLPFC_iref1_pu =
            TTPLPFC_PHASE_SHEDDING_1PH_2PH_TRANSITION_CURRENT /
            TTPLPFC_IL_MAX_SENSE;
    TTPLPFC_iref2_pu =
            TTPLPFC_PHASE_SHEDDING_2PH_3PH_TRANSITION_CURRENT /
            TTPLPFC_IL_MAX_SENSE;
    TTPLPFC_deltaIref_pu =
            TTPLPFC_PHASE_SHEDDING_TRANSITION_CURRENT_HYSTERESIS /
            TTPLPFC_IL_MAX_SENSE;
    TTPLPFC_interleaving_State.enum_interleavingState = interleavingState_3ph;

    TTPLPFC_nonLinearVoltageLoopFlagDelay = 0;
    TTPLPFC_nonLinearVoltageLoopFlag = 0;


    //
    //
    //
#if TTPLPFC_TIMED_STARTUP == 1
    TTPLPFC_timedRun_flag = 1;
    TTPLPFC_timedRun_ms = TTPLPFC_STARTUP_TIME_MS;
#else
    TTPLPFC_timedRun_flag = 0;
#endif

    TTPLPFC_manualStop = 0;
    TTPLPFC_dutySlew_pu = 0.01f;

    TTPLPFC_state_slew = 0;
    TTPLPFC_state_slew_max = 10;
    TTPLPFC_softStartDeadbandScalingFactor = 150;
}


void TTPLPFC_computeNotchFltrCoeff(DCL_DF22 *coeff, float32_t Fs,
                                   float32_t notch_freq,
                                   float32_t c1, float32_t c2)
{
    float32_t temp1;
    float32_t temp2;
    float32_t wn2;
    float32_t Ts;
    Ts = 1 / Fs;

    //
    // pre warp the notch frequency
    //
    wn2 = 2 * Fs * tanf(notch_freq * TTPLPFC_PI_VALUE * Ts);

    temp1 = 4 * Fs * Fs + 4 * wn2 * c2 * Fs + wn2 * wn2;
    temp2 = 1 / ( 4 * Fs * Fs + 4 * wn2 * c1 * Fs + wn2 * wn2);

    coeff->b0 = temp1 * temp2;
    coeff->b1 = (-8 * Fs * Fs + 2 * wn2 * wn2) * temp2;
    coeff->b2 = (4 * Fs * Fs - 4 * wn2 * c2 * Fs + wn2 * wn2) * temp2;
    coeff->a1 = (-8 * Fs * Fs + 2 * wn2 * wn2) * temp2;
    coeff->a2 = (4 * Fs * Fs - 4 * wn2 * c1 * Fs + wn2 * wn2) * temp2;


}

void TTPLPFC_updateBoardStatus()
{
    if(TTPLPFC_board_Status.enum_boardStatus == boardStatus_NoFault)
    {
        if((TTPLPFC_PWM_TRIP_STATUS(EPWM1_BASE) & EPWM_TZ_INTERRUPT_CBC) != 0 )
            TTPLPFC_board_Status.enum_boardStatus =
                    boardStatus_EmulatorStopTrip;
    }

}


void TTPLPFC_runSFRABackGroundTasks(void)
{

    SFRA_F32_runBackgroundTask(&TTPLPFC_sfra1);
    SFRA_GUI_runSerialHostComms(&TTPLPFC_sfra1);

}


void TTPLPFC_setLabIndicatorVariable(void)
{
#if TTPLPFC_LAB == 1
    #if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab1_CLA;
    #else
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab1;
    #endif
#elif TTPLPFC_LAB == 2
    #if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab2_CLA;
    #else
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab2;
    #endif
#elif TTPLPFC_LAB == 3
    #if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab3_CLA;
    #else
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab3;
    #endif
#elif TTPLPFC_LAB == 4
    #if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab4_CLA;
    #else
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab4;
    #endif
#elif TTPLPFC_LAB == 5
    #if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab5_CLA;
    #else
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab5;
    #endif
#elif TTPLPFC_LAB == 6
    #if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab6_CLA;
    #else
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab6;
    #endif
#elif TTPLPFC_LAB == 7
    #if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab7_CLA;
    #else
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab7;
    #endif
#elif TTPLPFC_LAB == 8
    #if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab8_CLA;
    #else
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab8;
    #endif
#elif TTPLPFC_LAB == 9
    #if TTPLPFC_CONTROL_RUNNING_ON == CLA_CORE
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab9_CLA;
    #else
        TTPLPFC_lab.enum_lab = TTPLPFC_Lab9;
    #endif
#endif

}

void TTPLPFC_autoStart(void)
{
    #if TTPLPFC_LAB >= 5
        {
            #if TTPLPFC_LAB == 9
            {
                TTPLPFC_vBus_underVoltage_Volts = 340;
                TTPLPFC_ac_volRms_underVoltage_Volts = 75;
            }
            #else
            {
                TTPLPFC_vBus_underVoltage_Volts = 110;
                TTPLPFC_ac_volRms_underVoltage_Volts = 0;
            }
            #endif
        }
    #endif

    #if TTPLPFC_DC_CHECK == 1
    if(TTPLPFC_autoStartSlew < 100)
    {
        TTPLPFC_autoStartSlew++;

        #if TTPLPFC_RAMPED_DUTY_STARTUP == 1
        //
        // start with a  low duty cycle value on the active FET
        // note the duty cycle is for the synchronous FET
        // hence for a low duty cycle on the active fet a high value needs to be
        // written to duty cycle command
        //

        //
        // commenting out polarity protection
        //
        //            if(TTPLPFC_ac_vol_sensed_pu > 0.0f)
        //            {
        //                //
        //                // Use 0.97 to startup the duty, this will give ~3% on the duty command
        //                // which after deadband will correspond ~2% duty cycle pulse
        //                //
        //                TTPLPFC_duty1_DC_ref_pu = 1.0 - 0.03f;
        //                TTPLPFC_duty2_DC_ref_pu = 1.0 - 0.03f;
        //
        //                TTPLPFC_duty1_DC_ref_final_pu = 0.5f;
        //
        //            }
        //            else
        //            {
        //                //
        //                // Use -0.97 to startup the duty, this will give ~3% on the duty command
        //                // which after deadband will correspond ~2% duty cycle pulse
        //                //
        //                TTPLPFC_duty1_DC_ref_pu = -1.0f * (1.0 - 0.03f);
        //                TTPLPFC_duty2_DC_ref_pu = -1.0f * (1.0 - 0.03f);
        //
        //                TTPLPFC_duty1_DC_ref_final_pu = -0.5f;
        //            }


            #if TTPLPFC_AC_HALF_CHECK == TTPLPFC_POSITIVE_HALF_DC_CHECK
            //
            // for positive half
            //
            TTPLPFC_duty_DC_ref_pu = 1.0 - 0.03f;
            TTPLPFC_duty_DC_ref_pu = 1.0 - 0.03f;

            TTPLPFC_duty_DC_ref_final_pu = 0.5f;
            #else
            //
            // for negative half
            //
            TTPLPFC_duty_DC_ref_pu = -1.0f * (1.0 - 0.03f);
            TTPLPFC_duty_DC_ref_pu = -1.0f * (1.0 - 0.03f);

            TTPLPFC_duty_DC_ref_final_pu = -0.5f;

            #endif
         #endif

        }
    #else

        //
        // Auto Start for Inverter mode
        //
        #if TTPLPFC_LAB >= 5
            if(TTPLPFC_vBus_sensed_Volts >= TTPLPFC_vBus_underVoltage_Volts &&
               TTPLPFC_ac_volRms_sensed_Volts >=
               TTPLPFC_ac_volRms_underVoltage_Volts)
            {
                if(TTPLPFC_autoStartSlew < 5)
                {
                    TTPLPFC_autoStartSlew++;
                }

                if(TTPLPFC_autoStartSlew == 2)
                {
                    DINT;

                    if(TTPLPFC_sine_mains.acFreq > 55)
                    {
                        TTPLPFC_sine_mains.acFreq = 60;
                    }
                    else
                    {
                        TTPLPFC_sine_mains.acFreq = 50;
                    }

                    SPLL_1PH_SOGI_config(&TTPLPFC_spll1,
                                         TTPLPFC_sine_mains.acFreq,
                                         TTPLPFC_CONTROL_ISR_FREQUENCY,
                                         (float32_t)(222.2862),
                                         (float32_t)(-222.034));

                    SPLL_1PH_NOTCH_config(&TTPLPFC_spll2,
                                          TTPLPFC_sine_mains.acFreq,
                                          TTPLPFC_CONTROL_ISR_FREQUENCY,
                                          (float32_t)(222.2862),
                                          (float32_t)(-222.034),
                                          (float32_t) 0.25,
                                          (float32_t) 0.00001);

                    SPLL_1PH_SOGI_FLL_config(&TTPLPFC_spll3,
                                             TTPLPFC_sine_mains.acFreq,
                                             TTPLPFC_CONTROL_ISR_FREQUENCY,
                                             (float32_t)(222.2862),
                                             (float32_t)(-222.034),
                                             (float32_t) 0.5,
                                             (float32_t) 20000);

                    TTPLPFC_computeNotchFltrCoeff(&TTPLPFC_notch1,
                             (float32_t)(TTPLPFC_INSTRUMENTATION_ISR_FREQUENCY),
                             (float32_t)(TTPLPFC_sine_mains.acFreq*2.0) ,
                                           0.25f,
                                           0.00001f);

                    TTPLPFC_computeNotchFltrCoeff(&TTPLPFC_notch2,
                             (float32_t)(TTPLPFC_INSTRUMENTATION_ISR_FREQUENCY),
                             (float32_t)(TTPLPFC_sine_mains.acFreq*2.0),
                                           0.25f,
                                           0.00001f);

                    TTPLPFC_duty1_pu = 0.01f;

#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
                    TTPLPFC_HAL_AQ_SW_FORCE_PWMxA_LOW_PWMxB_LOW(
                            TTPLPFC_LOW_FREQ_PWM_BASE);
#endif

                    TTPLPFC_HAL_SET_PWM_DBRED(TTPLPFC_HIGH_FREQ_PWM1_BASE,
                                              TTPLPFC_PWM_PERIOD);
                    TTPLPFC_HAL_SET_PWM_DBFED(TTPLPFC_HIGH_FREQ_PWM1_BASE,
                                              TTPLPFC_PWM_PERIOD);

                    TTPLPFC_HAL_SET_PWM_DBRED(TTPLPFC_HIGH_FREQ_PWM2_BASE,
                                              TTPLPFC_PWM_PERIOD);
                    TTPLPFC_HAL_SET_PWM_DBFED(TTPLPFC_HIGH_FREQ_PWM2_BASE,
                                              TTPLPFC_PWM_PERIOD);

                    EINT;

                    TTPLPFC_nonLinearVoltageLoopFlag = 0;

                }

                if(TTPLPFC_autoStartSlew == 3)
                {
                    if(TTPLPFC_ac_volRms_sensed_Volts < 150 )
                    {
                        TTPLPFC_thetaOffset_pu =
                                TTPLPFC_LOW_LINE_INPUT_CAP_COMP_ADJUST;
                    }
                    else
                    {
                        TTPLPFC_thetaOffset_pu =
                                TTPLPFC_HIGH_LINE_INPUT_CAP_COMP_ADJUST;
                    }

                    DINT;
#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
                    TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_LOW_FREQ_PWM_BASE);
#endif
                    TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_HIGH_FREQ_PWM1_BASE);
                    TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_HIGH_FREQ_PWM2_BASE);
                    TTPLPFC_board_Status.enum_boardStatus = boardStatus_NoFault;
                    EINT;

                }
                if(TTPLPFC_autoStartSlew == 1)
                {
                    TTPLPFC_board_Status.enum_boardStatus =
                            boardStatus_Idle;
                    TTPLPFC_pwm_SwState.enum_pwmSwState =
                            pwmSwState_defaultState;
                }



            }
            else
            {
                TTPLPFC_rlyConnect = 0;
                TTPLPFC_autoStartSlew = 0;
                TTPLPFC_pwm_SwState.enum_pwmSwState = pwmSwState_defaultState;
                TTPLPFC_closeGiLoop = 0;
                TTPLPFC_closeGvLoop = 0;

            }

        //
        // Auto Start for PFC mode
        //
        #else
            if(TTPLPFC_ac_volRms_sensed_Volts > 75.0f )
            {
                if(TTPLPFC_autoStartSlew < 5)
                {
                    TTPLPFC_autoStartSlew++;
                }

                if(TTPLPFC_autoStartSlew == 2)
                {


                    DINT;

                    if(TTPLPFC_sine_mains.acFreq > 55)
                    {
                        TTPLPFC_sine_mains.acFreq = 60;
                    }
                    else
                    {
                        TTPLPFC_sine_mains.acFreq = 50;
                    }

                    SPLL_1PH_SOGI_config(&TTPLPFC_spll1,
                                         TTPLPFC_sine_mains.acFreq,
                                         TTPLPFC_CONTROL_ISR_FREQUENCY,
                                         (float32_t)(222.2862),
                                         (float32_t)(-222.034));

                    SPLL_1PH_NOTCH_config(&TTPLPFC_spll2,
                                          TTPLPFC_sine_mains.acFreq,
                                          TTPLPFC_CONTROL_ISR_FREQUENCY,
                                          (float32_t)(222.2862),
                                          (float32_t)(-222.034),
                                          (float32_t) 0.25,
                                          (float32_t) 0.00001);

                    SPLL_1PH_SOGI_FLL_config(&TTPLPFC_spll3,
                                             TTPLPFC_sine_mains.acFreq,
                                             TTPLPFC_CONTROL_ISR_FREQUENCY,
                                             (float32_t)(222.2862),
                                             (float32_t)(-222.034),
                                             (float32_t) 0.5,
                                             (float32_t) 20000);

                    TTPLPFC_computeNotchFltrCoeff(&TTPLPFC_notch1,
                             (float32_t)(TTPLPFC_INSTRUMENTATION_ISR_FREQUENCY),
                             (float32_t)(TTPLPFC_sine_mains.acFreq*2.0) ,
                                           0.25f,
                                           0.00001f);

                    TTPLPFC_computeNotchFltrCoeff(&TTPLPFC_notch2,
                             (float32_t)(TTPLPFC_INSTRUMENTATION_ISR_FREQUENCY),
                             (float32_t)(TTPLPFC_sine_mains.acFreq*2.0),
                                           0.25f,
                                           0.00001f);

                    TTPLPFC_duty1_pu = 0.01f;
                    TTPLPFC_duty2_pu = 0.01f;

#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
                    TTPLPFC_HAL_AQ_SW_FORCE_PWMxA_LOW_PWMxB_LOW(
                            TTPLPFC_LOW_FREQ_PWM_BASE);
#endif

                    TTPLPFC_HAL_SET_PWM_DBRED(TTPLPFC_HIGH_FREQ_PWM1_BASE,
                                              TTPLPFC_PWM_PERIOD);
                    TTPLPFC_HAL_SET_PWM_DBFED(TTPLPFC_HIGH_FREQ_PWM1_BASE,
                                              TTPLPFC_PWM_PERIOD);

                    TTPLPFC_HAL_SET_PWM_DBRED(TTPLPFC_HIGH_FREQ_PWM2_BASE,
                                              TTPLPFC_PWM_PERIOD);
                    TTPLPFC_HAL_SET_PWM_DBFED(TTPLPFC_HIGH_FREQ_PWM2_BASE,
                                              TTPLPFC_PWM_PERIOD);
#if TTPLPFC_TOTAL_NO_PHASES == 3
                    TTPLPFC_HAL_SET_PWM_DBRED(TTPLPFC_HIGH_FREQ_PWM3_BASE,
                                              TTPLPFC_PWM_PERIOD);
                    TTPLPFC_HAL_SET_PWM_DBFED(TTPLPFC_HIGH_FREQ_PWM3_BASE,
                                              TTPLPFC_PWM_PERIOD);
#endif
                    EINT;

                    TTPLPFC_nonLinearVoltageLoopFlag = 0;

                }

                if(TTPLPFC_autoStartSlew == 3)
                {
                    if(TTPLPFC_ac_volRms_sensed_Volts < 150 )
                    {
                        TTPLPFC_thetaOffset_pu =
                                TTPLPFC_LOW_LINE_INPUT_CAP_COMP_ADJUST;
                    }
                    else
                    {
                        TTPLPFC_thetaOffset_pu =
                                TTPLPFC_HIGH_LINE_INPUT_CAP_COMP_ADJUST;
                    }

                    DINT;
#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
                    TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_LOW_FREQ_PWM_BASE);
#endif
                    TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_HIGH_FREQ_PWM1_BASE);
                    TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_HIGH_FREQ_PWM2_BASE);
#if TTPLPFC_TOTAL_NO_PHASES == 3
                    TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_HIGH_FREQ_PWM3_BASE);
#endif
                    TTPLPFC_board_Status.enum_boardStatus = boardStatus_NoFault;
                    EINT;

                }
                if(TTPLPFC_autoStartSlew == 1)
                {
                    TTPLPFC_board_Status.enum_boardStatus =
                            boardStatus_Idle;
                    TTPLPFC_pwm_SwState.enum_pwmSwState =
                            pwmSwState_defaultState;
                }



            }
            else
            {
                //
                // If Lab1 and AC check then clear the trip
                // even if AC is not present to see the PWM modulation
                //
                #if (TTPLPFC_LAB == 1 && TTPLPFC_PWM_AC_CHECK == 1)
                if (TTPLPFC_clearTrip == 1)
                    {
#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
                    TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_LOW_FREQ_PWM_BASE);
#endif
                    TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_HIGH_FREQ_PWM1_BASE);
                    TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_HIGH_FREQ_PWM2_BASE);
#if TTPLPFC_TOTAL_NO_PHASES == 3
                    TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_HIGH_FREQ_PWM3_BASE);
#endif
                    TTPLPFC_board_Status.enum_boardStatus = boardStatus_NoFault;

                    TTPLPFC_clearTrip = 0;

                    TTPLPFC_pwm_SwState.enum_pwmSwState = pwmSwState_normalOperation;
                    }
                #else
                    TTPLPFC_rlyConnect = 0;
                    TTPLPFC_autoStartSlew = 0;
                    TTPLPFC_pwm_SwState.enum_pwmSwState =
                            pwmSwState_defaultState;
                    TTPLPFC_closeGiLoop = 0;
                    TTPLPFC_closeGvLoop = 0;
                #endif
            }
        #endif
    #endif

#if TTPLPFC_DC_CHECK == 1
    if(TTPLPFC_autoStartSlew == 100)
#else
    if(TTPLPFC_autoStartSlew == 4)
#endif
    {

#if TTPLPFC_RAMPED_DUTY_STARTUP == 1

        //
        // If debug startup is ON then always disable the sync FET
        //
//        if(TTPLPFC_ac_vol_sensed_pu > 0.0f)
//        {
//            GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM1_H_GPIO, GPIO_DIR_MODE_OUT);
//            GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM1_H_GPIO , GPIO_PIN_TYPE_STD);
//            GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM1_H_PHASE_SHEDDING_GPIO_PIN_CONFIG);
//            GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM1_H_GPIO, 0);
//
//            GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM2_H_GPIO, GPIO_DIR_MODE_OUT);
//            GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM2_H_GPIO , GPIO_PIN_TYPE_STD);
//            GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM2_H_PHASE_SHEDDING_GPIO_PIN_CONFIG);
//            GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM2_H_GPIO, 0);
//        }
//        else
//        {
//            GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM1_L_GPIO, GPIO_DIR_MODE_OUT);
//            GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM1_L_GPIO , GPIO_PIN_TYPE_STD);
//            GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM1_L_PHASE_SHEDDING_GPIO_PIN_CONFIG);
//            GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM1_L_GPIO, 0);
//
//            GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM2_L_GPIO, GPIO_DIR_MODE_OUT);
//            GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM2_L_GPIO , GPIO_PIN_TYPE_STD);
//            GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM2_L_PHASE_SHEDDING_GPIO_PIN_CONFIG);
//            GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM2_L_GPIO, 0);
//        }

#if TTPLPFC_AC_HALF_CHECK == TTPLPFC_POSITIVE_HALF_DC_CHECK
        //
        // for positive half
        //
        GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM1_H_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM1_H_GPIO , GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM1_H_PHASE_SHEDDING_GPIO_PIN_CONFIG);
        GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM1_H_GPIO, 0);

        GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM2_H_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM2_H_GPIO , GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM2_H_PHASE_SHEDDING_GPIO_PIN_CONFIG);
        GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM2_H_GPIO, 0);
#else
        //
        // for negative half
        //
        GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM1_L_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM1_L_GPIO , GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM1_L_PHASE_SHEDDING_GPIO_PIN_CONFIG);
        GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM1_L_GPIO, 0);

        GPIO_setDirectionMode(TTPLPFC_HIGH_FREQ_PWM2_L_GPIO, GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(TTPLPFC_HIGH_FREQ_PWM2_L_GPIO , GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(TTPLPFC_HIGH_FREQ_PWM2_L_PHASE_SHEDDING_GPIO_PIN_CONFIG);
        GPIO_writePin(TTPLPFC_HIGH_FREQ_PWM2_L_GPIO, 0);
#endif

        TTPLPFC_timedRunCounter_ms = 0.0f;
        TTPLPFC_manualStop = 0;
        TTPLPFC_timedRun_flag = TTPLPFC_TIMED_STARTUP;
#endif

        DINT;

        TTPLPFC_vBusRefSlewed_pu = 0;

        TTPLPFC_gv.i6 = 0;
        TTPLPFC_gv.i10 = 0;

        TTPLPFC_gi1.i10 = 0;
        TTPLPFC_gi1.i6 = 0;

        TTPLPFC_gi2.i10 = 0;
        TTPLPFC_gi2.i6 = 0;

        TTPLPFC_closeGiLoop = 0;
        TTPLPFC_closeGvLoop = 0;

        TTPLPFC_clearTrip = 1;
        TTPLPFC_firstTimeGvLoop = 1;

        TTPLPFC_state_slew = 0;
        TTPLPFC_softstart_duty = 0;

        TTPLPFC_vBusRef_pu = TTPLPFC_VBUS_TYPICAL / TTPLPFC_VDCBUS_MAX_SENSE;

        TTPLPFC_ac_cur_ref_pu = 0.03f;

        TTPLPFC_ac_cur_ref_inst_pu = 0;
        TTPLPFC_ac_cur_ref_inst_prev_pu = 0;

        TTPLPFC_acSine = 0;
        TTPLPFC_acSinePrev = 0;
        TTPLPFC_zeroCrossDetectFlag = 0;

        #if TTPLPFC_LAB >= 5

        //
        // In inverter mode, it will not automatically start dc/ac conversion
        // pwmSwState has to be manually changed from defaultState
        // to normalOperation by user
        //
            TTPLPFC_pwm_SwState.enum_pwmSwState =
                    pwmSwState_defaultState;

        //
        // The negative sign in ac_cur_ref represents the reverse power flow
        // in inverter mode
        //
            TTPLPFC_ac_cur_ref_pu = -0.03f;
        #else

        //
        // In PFC mode, it will automatically start ac/dc conversion
        //
            TTPLPFC_pwm_SwState.enum_pwmSwState = pwmSwState_normalOperation;
        #endif

        EINT;

        TTPLPFC_autoStartSlew++;

#if TTPLPFC_DC_CHECK == 1

#if TTPLPFC_LOW_FREQ_PWM_ENABLED == 1
        TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_LOW_FREQ_PWM_BASE);
#endif
        TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_HIGH_FREQ_PWM1_BASE);
        TTPLPFC_HAL_clearPWMTripFlags(TTPLPFC_HIGH_FREQ_PWM2_BASE);

        TTPLPFC_rlyConnect = 1;
        TTPLPFC_closeGiLoop = 1;

#endif

    }


}

//
// If the update command is set to 1, the coefficients will be updated with the Kp Ref and Ki Ref values
//
void TTPLPFC_runCoefficientsUpdate()
{
    if(TTPLPFC_updateCoefficientsCommand == 1)
    {
        TTPLPFC_updateCoefficientsCommand = 0;
        DINT;
        TTPLPFC_gi_Kp = TTPLPFC_gi_Kp_Ref;
        TTPLPFC_gi_Ki = TTPLPFC_gi_Ki_Ref;
        #if TTPLPFC_DC_CHECK == 1
            TTPLPFC_gi1.Kp = TTPLPFC_gi_Kp;
            TTPLPFC_gi1.Ki = TTPLPFC_gi_Ki;
            TTPLPFC_gi2.Kp = TTPLPFC_gi_Kp;
            TTPLPFC_gi2.Ki = TTPLPFC_gi_Ki;
        #else
            //
            // for AC closed loop the state machine will auto update the compensators
            // hence no need to do that here
            //
        #endif
        EINT;
    }
}
