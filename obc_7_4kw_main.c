//#############################################################################
//
// FILE:   obc_7_4kw_main.c
//
// TITLE: This is the main file for the OBC solution, following is the
//         obc_7_4kw.c -> solution source file
//         obc_7_4kw.h -> solution header file
//         obc_7_4kw_settings.h -> powerSUITE generated settings
//           (any changes to this file will be overwritten by the GUI)
//         obc_7_4kw_user_settings.h -> user configurable settings not set
//                                      by the GUI
//
//        The OBC solution draws on two modules: CLLLC and PFC
//         clllc.c / ttplpfc.c -> module source file
//         clllc.h / ttplpfc.h -> module header file
//         clllc_settings.h / ttplpfc_settings.h ->
//             powerSUITE generated settings
//             (any changes to these files will be overwritten by the GUI)
//         clllc_user_settings.h / ttplpfc_user_settings.h ->
//             user configurable settings not set by the GUI
//
//         clllc_hal.c / ttplpfc.c -> board and device drivers source file
//         clllc_hal.h / ttplpfc.h -> board and device drivers header file
//
//         <optional>
//         <solution>_cla.cla -> cla task file
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
#include "obc_7_4kw.h"
#include "clllc.h"
#include "ttplpfc.h"
#include "unipd/unipd_control.h"
#include "wless_sm/wless_sm.h"
#include "wless_uart/wless_uart.h"
#include "wless_radio/wless_nrf24.h"

//
//---  State Machine Related ---
//
int16_t vTimer0[4];         // Virtual Timers slaved off CPU Timers (A events)
int16_t vTimer1[4];         // Virtual Timers slaved off CPU Timers (B events)

//
// Variable declarations for state machine
//
void (*Alpha_State_Ptr)(void);  // Base States pointer
void (*A_Task_Ptr)(void);       // State pointer A branch
void (*B_Task_Ptr)(void);       // State pointer B branch
void (*C_Task_Ptr)(void);       // State pointer C branch

//
// State Machine function prototypes
//------------------------------------
// Alpha states
//
void A0(void);  //state A0
void B0(void);  //state B0

//
// A branch states
//
void A1(void);  //state A1

//
// B branch states
//
void B1(void);  //state B1
void B2(void);  //state B2
void B3(void);  //state B3

uint32_t  CLLLC_ISR2cap1Count;
uint32_t  CLLLC_ISR2cap2Count;
uint32_t  CLLLC_ISR2cap3Count;
uint32_t  CLLLC_ISR2cap4Count;

uint32_t  CLLLC_ISR3cap1Count;
uint32_t  CLLLC_ISR3cap2Count;
uint32_t  CLLLC_ISR3cap3Count;
uint32_t  CLLLC_ISR3cap4Count;

float32_t CLLLC_ISR2_Loading;
float32_t CLLLC_ISR2_LoadingMax;
float32_t CLLLC_ISR3_Loading;
float32_t CLLLC_ISR3_LoadingAvg;

#if Profiling
uint32_t OBC_7_4KW_ISR2_nestingCounter;
uint32_t OBC_7_4KW_ISR2_nests;
uint32_t OBC_7_4KW_ISR2_nestsMax;
float32_t OBC_7_4KW_ISR2_nestsAvg;
float32_t OBC_7_4KW_ISR3_LoadingAvg_accountingForNesting;
float32_t OBC_7_4KW_ISR3_LoadingAvgMax_accountingForNesting;
#endif

volatile uint32_t logger1[20];
volatile float32_t logger2[20];
volatile uint32_t index = 0;

void main(void)
{
    //
    // This routine sets up the basic device configuration such as
    // initializing PLL, copying code from FLASH to RAM,
    // this routine will also initialize the CPU timers that are used in
    // the background 1, 2 & 3) task for this system (CPU time)
    //
    OBC_7_4KW_setupDevice();

    OBC_7_4KW_initGlobalVariables();

    //
    // Stop all PWM mode clock
    //
    OBC_7_4KW_disablePWMClkCounting();

    //
    // Setup peripherals
    //
    OBC_7_4KW_setupADC();
    OBC_7_4KW_setupGPIO();

#if Profiling
    OBC_7_4KW_setupProfiling();
#warning Temperature reading disabled when profiling ISRs with eCAP
#else
    OBC_7_4KW_setupGaNTempReading();
#endif

    OBC_7_4KW_setupBoardProtection();
    OBC_7_4KW_setupPWM();

//    OBC_7_4KW_setupAEF();
    OBC_7_4KW_setupActiveSyncRectCLLLC();

    //
    // Enable PWM Clocks
    //
    OBC_7_4KW_enablePWMClkCounting();

    OBC_7_4KW_setupInterrupt();

//    GPIO_setPinConfig(GPIO_10_GPIO10);  // Disable Phase 1
//    GPIO_setPinConfig(GPIO_11_GPIO11);  // Disable Phase 1
//    GPIO_setPinConfig(GPIO_12_GPIO12);  // Disable Phase 2
//    GPIO_setPinConfig(GPIO_13_GPIO13);  // Disable Phase 2

    //
    // Tasks State-machine init
    //
    Alpha_State_Ptr = &A0;
    A_Task_Ptr = &A1;
    B_Task_Ptr = &B1;
    WLESS_SM_INIT_IF_ENABLED();
    WLESS_UART_INIT_IF_ENABLED();
    WLESS_NRF24_INIT_IF_ENABLED();

    //
    // IDLE loop. Just sit and loop forever, periodically will branch into
    // A0-A3, B0-B3, C0-C3 tasks
    // Frequency of this branching is set in setupDevice routine:
    //
    for(;;)
    {
        //
        // State machine entry & exit point
        //===========================================================
        //
        (*Alpha_State_Ptr)();   // jump to an Alpha state (A0,B0,...)
        //
        //===========================================================
        //
    }
} //END MAIN CODE

//
// ISRs are named by the priority
// ISR1 is the highest priority
// ISR2 has the next highest and so forth
//

#if CLLLC_ISR1_RUNNING_ON == C28x_CORE
interrupt void ISR1(void)
{

    CLLLC_HAL_setProfilingGPIO1();

    CLLLC_runISR1();

    CLLLC_HAL_clearISR1InterruputFlag();

    CLLLC_HAL_resetProfilingGPIO1();

}
#endif


interrupt void ISR2_primToSecPowerFlow(void)
{
    //
    // enable group 3 interrupt only to interrupt ISR2
    //
    IER |= 0x4;
    IER &= 0x4;
    EINT;
#if Profiling
    OBC_7_4KW_ISR2_nestingCounter++;
#endif

//
//GPIO write doesn't work when assigning GPIO to CLA, remove at compile time to make this clear
//
#if Profiling
#if CLLLC_ISR1_RUNNING_ON == C28x_CORE
    CLLLC_HAL_setProfilingGPIO2();
#endif
#endif
#if TTPLPFC_EPWM67_ACTIVE_CONTROL == TTPLPFC_EPWM67_CONTROL_BBC
    if(TTPLPFC_BBC_runDockingTest() == 0)
    {
        OBC_7_4KW_runUnipdBbcControl();
    }
    else
    {
        UNIPD_runBbcDockingDiagnostics();
    }
    UNIPD_runTransferredPowerIntegration();
#elif TTPLPFC_EPWM67_ACTIVE_CONTROL == TTPLPFC_EPWM67_CONTROL_LEGACY_PFC
#if TTPLPFC_LAB == 1
    TTPLPFC_runISR1_lab1();
#elif TTPLPFC_LAB == 2
    TTPLPFC_runISR1_lab2();
#elif TTPLPFC_LAB == 3
    TTPLPFC_runISR1_lab3();
#elif TTPLPFC_LAB == 4
    TTPLPFC_runISR1_lab4();
#elif TTPLPFC_LAB == 5
#error "TTPLPFC Lab 5 has not been validated"
    TTPLPFC_runISR1_lab5();
#elif TTPLPFC_LAB == 6
#error "TTPLPFC Lab 6 has not been validated"
    TTPLPFC_runISR1_lab6();
#elif TTPLPFC_LAB == 7
#error "TTPLPFC Lab 7 has not been validated"
    TTPLPFC_runISR1_lab7();
#elif TTPLPFC_LAB == 8
#error "TTPLPFC Lab 8 has not been validated"
    TTPLPFC_runISR1_lab8();
#elif TTPLPFC_LAB == 9
#error "TTPLPFC Lab 9 has not been validated"
    TTPLPFC_runISR1_lab9();
#else
#warning "undefined TTPLPFC lab"
#endif
#else
#error "Invalid EPWM6/EPWM7 control selection"
#endif


//
//    Remove CLLLC Code from C28xISR when CLLLC is running on CLA
//
#if CLLLC_ISR2_RUNNING_ON == C28x_CORE
#if(CLLLC_LAB == 1 || CLLLC_LAB == 2)
    CLLLC_runOpenLoop();
#elif(CLLLC_LAB == 3)
    CLLLC_runVoltageLoop();
#elif(CLLLC_LAB == 4)
    CLLLC_runCurrentLoop();
#else
#warning "undefined CLLLC lab"
#endif
#endif

    WLESS_SM_SOURCE_TICK_ISR_IF_ENABLED();

//
//GPIO write doesn't work when assigning GPIO to CLA, remove at compile time to make this clear
//

#if CLLLC_ISR1_RUNNING_ON == C28x_CORE
    CLLLC_HAL_resetProfilingGPIO2();
#endif

//
//    CLLLC HAL functions used to reset ISR2 interrupt flags, not specific to CLLLC
//

    DINT;
    CLLLC_HAL_clearISR2PeripheralInterruptFlag();
    CLLLC_HAL_clearISR2InterruputFlag();
}

interrupt void ISR2_secToPrimPowerFlow(void)
{
    //
    // enable group 3 interrupt only to interrupt ISR2
    //
    IER |= 0x4;
    IER &= 0x4;
    EINT;
#if Profiling
    OBC_7_4KW_ISR2_nestingCounter++;
#endif

//
//    TTPLPFC secToPrim Code
//



//
//    Remove CLLLC Code from C28xISR when CLLLC is running on CLA
//
#if CLLLC_ISR2_RUNNING_ON == C28x_CORE

    CLLLC_HAL_setProfilingGPIO2();

    CLLLC_runISR2_secToPrimPowerFlow();
#endif

    WLESS_SM_SOURCE_TICK_ISR_IF_ENABLED();

    CLLLC_HAL_resetProfilingGPIO2();


    DINT;
    CLLLC_HAL_clearISR2InterruputFlag();
}


interrupt void ISR3(void)
{
    EINT;

#if Profiling
    //
    // Reset the ISR2 nesting counter to zero to log the number of nests
    // of ISR2 in ISR3
    //
    OBC_7_4KW_ISR2_nestingCounter = 0;
#endif


    CLLLC_HAL_setProfilingGPIO3();


#if TTPLPFC_EPWM67_ACTIVE_CONTROL == TTPLPFC_EPWM67_CONTROL_LEGACY_PFC
    TTPLPFC_runISR2();
#endif
    CLLLC_runISR3();

    CLLLC_HAL_resetProfilingGPIO3();

    DINT;

#if Profiling
    //
    // Log the number of nests, the max and compute the average number of nests
    //
    OBC_7_4KW_ISR2_nests = OBC_7_4KW_ISR2_nestingCounter;

    if(OBC_7_4KW_ISR2_nests > OBC_7_4KW_ISR2_nestsMax)
    {
       OBC_7_4KW_ISR2_nestsMax = OBC_7_4KW_ISR2_nests;
    }

    //
    // Take nesting count average over 100 cycles
    //
    OBC_7_4KW_ISR2_nestsAvg = OBC_7_4KW_ISR2_nestsAvg +
           0.01f * (float32_t)(OBC_7_4KW_ISR2_nests - OBC_7_4KW_ISR2_nestsAvg);
#endif


    CLLLC_HAL_clearISR3InterruputFlag();
}

//
//=============================================================================
//  STATE-MACHINE SEQUENCING AND SYNCRONIZATION FOR SLOW BACKGROUND TASKS
//=============================================================================
//
//
//--------------------------------- FRAME WORK --------------------------------
//
void A0(void)
{
    //
    // loop rate synchronizer for A-tasks
    //
    if(CLLLC_GET_TASKA_TIMER_OVERFLOW_STATUS == 1)
    {
        CLLLC_CLEAR_TASKA_TIMER_OVERFLOW_FLAG;    // clear flag

        //
        // jump to an A Task (A1,A2,A3,...)
        //
        (*A_Task_Ptr)();

        vTimer0[0]++;           // virtual timer 0, instance 0 (spare)
    }
    Alpha_State_Ptr = &B0;      // Comment out to allow only A tasks
}

void B0(void)
{
    //
    // loop rate synchronizer for B-tasks
    //
    if(CLLLC_GET_TASKB_TIMER_OVERFLOW_STATUS  == 1)
    {
        CLLLC_CLEAR_TASKB_TIMER_OVERFLOW_FLAG;                // clear flag

        //
        // jump to a B Task (B1,B2,B3,...)
        //
        (*B_Task_Ptr)();
        CLLLC_applyITankCmpssDebugSelection();
        CLLLC_applyITankCmpssDebugThresholds();
        CLLLC_applyHfcReceiverTestConfig();

        vTimer1[0]++;           // virtual timer 1, instance 0 (spare)
    }

    //
    // Allow A state tasks
    //
    Alpha_State_Ptr = &A0;
}

//
//=============================================================================
//  A - TASKS (executed in every 1 msec)
//=============================================================================
//

void A1(void)
{
    WLESS_SM_RUN_TICK_IF_ENABLED();
    WLESS_SM_RUN_IF_ENABLED();
    WLESS_UART_PROCESS_IF_ENABLED();
    WLESS_NRF24_SERVICE_IF_ENABLED();

#if CLLLC_SECONDARY_ENABLED == 1
    CLLLC_changeSynchronousRectifierPwmBehavior(CLLLC_POWER_FLOW);
#endif

    //
    //the next time CpuTimer0 'counter' reaches Period value go to A2
    //
    A_Task_Ptr = &A1;

}

//
//=============================================================================
//  B - TASKS (executed in every 5 msec)
//=============================================================================
//

void B1(void)
{

    TTPLPFC_updateBoardStatus();
    CLLLC_updateBoardStatus();
#if Profiling
    //
    // Get the capture counts for the ISR1 and ISR2 for the last two
    // ISRs, these are logged by reading the capture values
    //
    CLLLC_ISR2cap1Count = HWREG(CLLLC_PROFILING1_ECAP + ECAP_O_CAP1);
    CLLLC_ISR2cap2Count = HWREG(CLLLC_PROFILING1_ECAP + ECAP_O_CAP2);
    CLLLC_ISR2cap3Count = HWREG(CLLLC_PROFILING1_ECAP + ECAP_O_CAP3);
    CLLLC_ISR2cap4Count = HWREG(CLLLC_PROFILING1_ECAP + ECAP_O_CAP4);

    CLLLC_ISR3cap1Count = HWREG(CLLLC_PROFILING2_ECAP + ECAP_O_CAP1);
    CLLLC_ISR3cap2Count = HWREG(CLLLC_PROFILING2_ECAP + ECAP_O_CAP2);
    CLLLC_ISR3cap3Count = HWREG(CLLLC_PROFILING2_ECAP + ECAP_O_CAP3);
    CLLLC_ISR3cap4Count = HWREG(CLLLC_PROFILING2_ECAP + ECAP_O_CAP4);

    //
    // Capture ISR2 loading and log max value
    // look back at the last 2 captures to do the same
    //
    CLLLC_ISR2_Loading =  ((float32_t)CLLLC_ISR2cap1Count /
            ((float32_t)CLLLC_ISR2cap1Count + (float32_t)CLLLC_ISR2cap2Count) );

    if( CLLLC_ISR2_Loading > CLLLC_ISR2_LoadingMax )
    {
        CLLLC_ISR2_LoadingMax = CLLLC_ISR2_Loading;
    }

    CLLLC_ISR2_Loading =  ((float32_t)CLLLC_ISR2cap3Count /
            ((float32_t)CLLLC_ISR2cap3Count + (float32_t)CLLLC_ISR2cap4Count) );

    if( CLLLC_ISR2_Loading > CLLLC_ISR2_LoadingMax )
    {
        CLLLC_ISR2_LoadingMax = CLLLC_ISR2_Loading;
    }

    //
    // Average last 2 captures of ISR3
    //
    CLLLC_ISR3_Loading = ( ((float32_t)CLLLC_ISR3cap1Count /
             ((float32_t)CLLLC_ISR3cap1Count + (float32_t)CLLLC_ISR3cap2Count) ) +
                          ((float32_t)CLLLC_ISR3cap3Count /
             ((float32_t)CLLLC_ISR3cap3Count + (float32_t)CLLLC_ISR3cap4Count) ) )
                     * 0.5f;

    //
    // Further average over 100 samples of the average of 2 captures
    //
    CLLLC_ISR3_LoadingAvg = CLLLC_ISR3_LoadingAvg +
                      (CLLLC_ISR3_Loading - CLLLC_ISR3_LoadingAvg) * 0.01f ;

    //
    // Subtract the average loading of the nested ISR2 interrupts from ISR3 Loading
    //
    OBC_7_4KW_ISR3_LoadingAvg_accountingForNesting =  CLLLC_ISR3_LoadingAvg
            -( OBC_7_4KW_ISR2_nestsAvg * CLLLC_ISR2_Loading *
              (float32_t)TTPLPFC_INSTRUMENTATION_ISR_FREQUENCY/(float32_t)TTPLPFC_CONTROL_ISR_FREQUENCY );

    //
    //Track greatest average ISR3 loading, Note not the greatest instantaneous loading
    //
    if( OBC_7_4KW_ISR3_LoadingAvg_accountingForNesting > OBC_7_4KW_ISR3_LoadingAvgMax_accountingForNesting )
        {
        OBC_7_4KW_ISR3_LoadingAvgMax_accountingForNesting = OBC_7_4KW_ISR3_LoadingAvg_accountingForNesting;
        }
#endif
    //
    // The next time CpuTimer1 'counter' reaches Period value go to B2
    //
    B_Task_Ptr = &B2;
}

void B2(void)
{
#if (TTPLPFC_AUTO_START == 1) && \
    (TTPLPFC_EPWM67_ACTIVE_CONTROL == TTPLPFC_EPWM67_CONTROL_LEGACY_PFC)
    TTPLPFC_autoStart();
#endif


    //
    //the next time CpuTimer1 'counter' reaches Period value go to B3
    //
    B_Task_Ptr = &B3;

}

void B3(void)
{
//    TTPLPFC_HAL_toggleLED();
#if Profiling
    TTPLPFC_computeISRloading();
#endif
#if TTPLPFC_EPWM67_ACTIVE_CONTROL == TTPLPFC_EPWM67_CONTROL_LEGACY_PFC
    TTPLPFC_runCoefficientsUpdate();
#endif

    CLLLC_HAL_toggleLED1();

    //
    //the next time CpuTimer1 'counter' reaches Period value go to B1
    //
    B_Task_Ptr = &B1;

}

//
// No more.
//
