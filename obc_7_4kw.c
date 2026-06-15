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

#include "obc_7_4kw.h"
#include "clllc.h"
#include "ttplpfc.h"

//
//--- System Related Globals ---
// Put the variables that are specific to control in the below section
// For example SFRA cannot run on CLA hence it must not be placed
// in the below section, the control verification using SFRA can only
// be carried out on the C28x
// Control Variables
//
#pragma SET_DATA_SECTION("controlVariables")



#pragma SET_DATA_SECTION()
void OBC_7_4KW_setupDevice()
{
    //
    // An interchangeable function exists in the PFC module
    // TTPLPFC_HAL_setupDevice();
    //
    CLLLC_HAL_setupDevice();
}

void OBC_7_4KW_initGlobalVariables()
{
    //
    // Initialize global variables
    //
    TTPLPFC_globalVariablesInit();
    TTPLPFC_setLabIndicatorVariable();

    //
    // Offset Calibration Routine
    //
//    TTPLPFC_calibrateOffset(&TTPLPFC_ac_cur_senseOffset_pu,
//                            &TTPLPFC_iL1_senseOffset_pu,
//                            &TTPLPFC_iL2_senseOffset_pu,
//                            &TTPLPFC_iL3_senseOffset_pu , k1 , k2);

    //
    // note the value below is determined in Lab1 Check 7
    //
    TTPLPFC_iL1_senseOffset_pu = TTPLPFC_IL1_OFFSET_PU;
    TTPLPFC_iL2_senseOffset_pu = TTPLPFC_IL2_OFFSET_PU;
    TTPLPFC_ac_vol_senseOffset_pu = TTPLPFC_VAC_OFFSET_PU;

    CLLLC_initGlobalVariables();
//
//    
//     set's a global variable that indicates which build level is running
//     This variable can be viewed in the expressions view.
//     Changes to this variable through the expressions view has no effect
//    
    CLLLC_setBuildLevelIndicatorVariable();
}

void OBC_7_4KW_disablePWMClkCounting()
{
    //
    // An interchangeable function exists in the PFC module
    // TTPLPFC_HAL_disablePWMClkCounting();
    //
    CLLLC_HAL_disablePWMClkCounting();
}

void OBC_7_4KW_enablePWMClkCounting()
{
    //
    // An interchangeable function exists in the PFC module
    // TTPLPFC_HAL_enablePWMClkCounting();
    //
    CLLLC_HAL_enablePWMClkCounting();
}

void OBC_7_4KW_setupActiveSyncRectCLLLC()
{
    //
    // setup CMPSS for synchRect
    //
    CLLLC_HAL_setupSynchronousRectificationAction(
            CLLLC_powerFlowStateActive.CLLLC_PowerFlowState_Enum);

    //
    // brings out the blanked CMPSS signal on GPIO for debug
    //
    CLLLC_HAL_setupSynchronousRectificationActionDebug(
            CLLLC_powerFlowStateActive.CLLLC_PowerFlowState_Enum);
}

void OBC_7_4KW_setupAEF()
{

#if TTPLPFC_AEF_ENABLED == 1
    //
    // Active EMI based code
    //
    AEF_configure();
#endif


}

void OBC_7_4KW_setupPWM()
{

    TTPLPFC_HAL_setupPWM(TTPLPFC_PWM_PERIOD,
                         TTPLPFC_HIGH_FREQ_PWM_DEADBAND_RED_COUNT,
                         TTPLPFC_HIGH_FREQ_PWM_DEADBAND_FED_COUNT);

    //
    // safe to setup PWM pins
    //
    TTPLPFC_HAL_setPinsAsPWM();

    //
    // Sets up the PWMs for the CLLC prim and sec bridges
    // by default the PWMs are set as battery charging mode
    //
    CLLLC_HAL_setupPWM(CLLLC_powerFlowStateActive.CLLLC_PowerFlowState_Enum);

    EALLOW;

    //
    // Set global load to one-shot mode
    //
    HWREGH(CLLLC_PRIM_LEG1_PWM_BASE + EPWM_O_GLDCTL) = 0xA1;
    HWREGH(CLLLC_PRIM_LEG2_PWM_BASE + EPWM_O_GLDCTL) = 0xA7;

    //
    // Link EPWM2 to EPWM1
    //
    HWREG(CLLLC_PRIM_LEG2_PWM_BASE + EPWM_O_XLINK) &= ~(0xF0000000);

    EDIS;

    //
    // setup PWM pins
    //
    CLLLC_HAL_setupPWMpins(
            pwmSwState_synchronousRectification_active);
}

void OBC_7_4KW_setupADC()
{

    //
    // power up ADC on the device
    //
    TTPLPFC_HAL_setupADC();

    TTPLPFC_HAL_setupTrigForADC();

    //
    // power up ADC on the device
    //
    CLLLC_HAL_setupADC();

    //
    // setup trigger for the ADC conversions
    //
    CLLLC_HAL_setupTrigForADC();



}

void OBC_7_4KW_setupGPIO()
{
    //
    //configure LED GPIO
    //
//    TTPLPFC_HAL_setupLEDGPIO();

    //
    //configure Relay GPIO
    //
    TTPLPFC_HAL_setupRelayGPIO();

    //
    // Setup GPIOs for ISR profiling, LEDs, commutation inductor, etc.
    //
    CLLLC_HAL_setupGPIOs();
}

void OBC_7_4KW_setupProfiling()
{
    //
    //profiling eCAP
    //
//    TTPLPFC_HAL_setupECAPforProfilingCode();
    CLLLC_HAL_setupECAPforProfilingCode();

    //
    // Profiling GPIO
    //
//    TTPLPFC_HAL_setupProfilingGPIO();
}

void OBC_7_4KW_setupGaNTempReading()
{
//
//setup ECAP to read GaN temperature
//
//    TTPLPFC_HAL_setupECAPforGaNTemp();
    CLLLC_HAL_setupECAPforGaNTemp();
}

void OBC_7_4KW_setupBoardProtection()
{
    //
    // clear any spurious flags in the SDFM module
    // setup protection and trips for the board
    //
    TTPLPFC_HAL_setupBoardProtection(TTPLPFC_IL_TRIP_LIMIT,
            TTPLPFC_IL_MAX_SENSE);

    //
    // clear any spurious flags
    // setup protection and trips for the board
    //
    CLLLC_HAL_setupBoardProtection();
}

void OBC_7_4KW_setupInterrupt()
{
    //
    // ISR Mapping
    //

//    TTPLPFC_HAL_setupInterrupt();

    //
    // as LLC is resonant and frequency changes,
    // for ISR separate fixed frequency PWM is configured
    //
    CLLLC_HAL_setupPWMinUpDownCountMode(CLLLC_ISR2_PWM_BASE,
                               CLLLC_ISR2_FREQUENCY_HZ,
                               CLLLC_PWMSYSCLOCK_FREQ_HZ);
    CLLLC_HAL_setupECAPinPWMMode(CLLLC_ISR2_ECAP_BASE,
                                 CLLLC_ISR2_FREQUENCY_HZ,
                                 CLLLC_PWMSYSCLOCK_FREQ_HZ);

    //
    // ISR Mapping
    //
    CLLLC_HAL_setupInterrupt(
            CLLLC_powerFlowStateActive.CLLLC_PowerFlowState_Enum);

}

void OBC_7_4KW_setupSFRA()
{
//
// If both SFRA routines are enabled, throw an error
//
#if((TTPLPFC_SFRA_TYPE != TTPLPFC_SFRA_DISABLED) && (CLLLC_SFRA_TYPE != CLLLC_SFRA_DISABLED))
#error "The SFRA option cannot be simultaneously enabled for both the PFC and CLLLC!"
#endif

//
// If TTPLPFC routine is enabled, include the setup function
//
#if(TTPLPFC_SFRA_TYPE == TTPLPFC_SFRA_DISABLED)
#else
    TTPLPFC_setupSFRA();
#endif

//
// If CLLLC routine is enabled, include the setup function
//
#if(CLLLC_SFRA_TYPE == CLLLC_SFRA_DISABLED)
#else
    CLLLC_setupSFRA();
#endif
}

//
//===========================================================================
// No more.
//===========================================================================
//
