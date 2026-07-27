#include "wless_sm.h"

#if WLESS_SM_TICK_GPIO_TEST_ENABLE == 1
#include "driverlib.h"
#endif

#if WLESS_SM_POWER_CONTROL_ENABLE == 1
#include "clllc.h"
#include "ttplpfc.h"
#include "unipd/unipd_control.h"
#endif

#pragma RETAIN(WLESS_SM_state)
#pragma RETAIN(WLESS_SM_localRole)
#pragma RETAIN(WLESS_SM_remoteRole)
#pragma RETAIN(WLESS_SM_localCtrlState)
#pragma RETAIN(WLESS_SM_remoteCtrlState)
#pragma RETAIN(WLESS_SM_localAbort)
#pragma RETAIN(WLESS_SM_remoteAbort)
#pragma RETAIN(WLESS_SM_radioLink)
#pragma RETAIN(WLESS_SM_opMode)
#pragma RETAIN(WLESS_SM_wptState)
#pragma RETAIN(WLESS_SM_powerCommand)
#pragma RETAIN(WLESS_SM_localEnergyEncoded)
#pragma RETAIN(WLESS_SM_remoteEnergyEncoded)
#pragma RETAIN(WLESS_SM_vBus_V)
#pragma RETAIN(WLESS_SM_iBat_mA)
#pragma RETAIN(WLESS_SM_iCoil_mA)
#pragma RETAIN(WLESS_SM_powerToLoad)
#pragma RETAIN(WLESS_SM_iCoilErr)
#pragma RETAIN(WLESS_SM_noAckCount)
#pragma RETAIN(WLESS_SM_initOkCommand)
#pragma RETAIN(WLESS_SM_stopCommand)
#pragma RETAIN(WLESS_SM_diagnosticMessagePending)
#pragma RETAIN(WLESS_SM_operationMessagePending)
#pragma RETAIN(WLESS_SM_statusUpdatePending)
#pragma RETAIN(WLESS_SM_pendingTicks)
#pragma RETAIN(WLESS_SM_pendingTicksMax)
#pragma RETAIN(WLESS_SM_ticksDropped)
#pragma RETAIN(WLESS_SM_sourceTickAccumulator)
#pragma RETAIN(WLESS_SM_tickCounter)
#pragma RETAIN(WLESS_SM_stateStepCounter)

volatile WLESS_SM_State WLESS_SM_state;
volatile WLESS_SM_Role WLESS_SM_localRole;
volatile WLESS_SM_Role WLESS_SM_remoteRole;
volatile WLESS_SM_ControllerState WLESS_SM_localCtrlState;
volatile WLESS_SM_ControllerState WLESS_SM_remoteCtrlState;
volatile WLESS_SM_Abort WLESS_SM_localAbort;
volatile WLESS_SM_Abort WLESS_SM_remoteAbort;
volatile WLESS_SM_RadioLink WLESS_SM_radioLink;
volatile WLESS_SM_OpMode WLESS_SM_opMode;
volatile WLESS_SM_WptState WLESS_SM_wptState;
volatile WLESS_SM_PowerCommand WLESS_SM_powerCommand;

volatile int16_t WLESS_SM_localEnergyEncoded;
volatile int16_t WLESS_SM_remoteEnergyEncoded;
volatile uint16_t WLESS_SM_vBus_V;
volatile uint16_t WLESS_SM_iBat_mA;
volatile uint16_t WLESS_SM_iCoil_mA;
volatile uint16_t WLESS_SM_vBusMin_V = WLESS_SM_VBUS_MIN_V;
volatile uint16_t WLESS_SM_iBatMin_mA = WLESS_SM_IBAT_MIN_MA;
volatile uint16_t WLESS_SM_iCoilMin_mA = WLESS_SM_ICOIL_MIN_MA;
volatile int16_t WLESS_SM_powerToLoad;
volatile int16_t WLESS_SM_iCoilErr;
volatile uint16_t WLESS_SM_noAckCount;

volatile uint16_t WLESS_SM_initOkCommand;
volatile uint16_t WLESS_SM_stopCommand;
volatile uint16_t WLESS_SM_diagnosticMessagePending;
volatile uint16_t WLESS_SM_operationMessagePending;
volatile uint16_t WLESS_SM_statusUpdatePending;
volatile uint16_t WLESS_SM_pendingTicks;
volatile uint16_t WLESS_SM_pendingTicksMax;
volatile uint16_t WLESS_SM_ticksDropped;
volatile uint32_t WLESS_SM_sourceTickAccumulator;
volatile uint32_t WLESS_SM_tickCounter;
volatile uint32_t WLESS_SM_stateStepCounter;

typedef struct
{
    uint16_t count;
    uint32_t lastStep;
    bool latched;
} WLESS_SM_ConditionFilter;

static WLESS_SM_ConditionFilter WLESS_SM_sourceEmptyFilter;
static WLESS_SM_ConditionFilter WLESS_SM_loadFullFilter;
static WLESS_SM_ConditionFilter WLESS_SM_energyBalanceFilter;
static WLESS_SM_ConditionFilter WLESS_SM_vBusFilter;
static WLESS_SM_ConditionFilter WLESS_SM_iBatFilter;
static WLESS_SM_ConditionFilter WLESS_SM_iCoilFilter;
static WLESS_SM_State WLESS_SM_filterState;
static WLESS_SM_Role WLESS_SM_filterRole;

static void WLESS_SM_resetFilter(WLESS_SM_ConditionFilter *filter)
{
    filter->count = 0U;
    filter->lastStep = 0UL;
    filter->latched = false;
}

static bool WLESS_SM_updateFilteredCondition(WLESS_SM_ConditionFilter *filter,
                                             bool tripCondition,
                                             bool releaseCondition,
                                             bool latchResult)
{
    if(filter->latched)
    {
        if(releaseCondition)
        {
            WLESS_SM_resetFilter(filter);
        }
        return filter->latched;
    }

    if(!tripCondition)
    {
        filter->count = 0U;
        return false;
    }

    if(filter->lastStep != WLESS_SM_stateStepCounter)
    {
        filter->lastStep = WLESS_SM_stateStepCounter;
        if(filter->count < WLESS_SM_ANALOG_CONFIRM_SAMPLES)
        {
            filter->count++;
        }
    }

    if(filter->count >= WLESS_SM_ANALOG_CONFIRM_SAMPLES)
    {
        filter->count = 0U;
        if(latchResult)
        {
            filter->latched = true;
        }
        return true;
    }

    return false;
}

static bool WLESS_SM_confirmVBusReady(void)
{
    return WLESS_SM_updateFilteredCondition(&WLESS_SM_vBusFilter,
            WLESS_SM_vBus_V > WLESS_SM_vBusMin_V, false, false);
}

static bool WLESS_SM_confirmIBatLow(void)
{
    return WLESS_SM_updateFilteredCondition(&WLESS_SM_iBatFilter,
            WLESS_SM_iBat_mA < WLESS_SM_iBatMin_mA, false, false);
}

static bool WLESS_SM_confirmICoilLow(void)
{
    return WLESS_SM_updateFilteredCondition(&WLESS_SM_iCoilFilter,
            WLESS_SM_iCoil_mA < WLESS_SM_iCoilMin_mA, false, false);
}

static bool WLESS_SM_radioLinkKo(void)
{
    if(WLESS_SM_noAckCount >= 15U)
    {
        WLESS_SM_radioLink = WLESS_SM_LINK_FAIL;
        return true;
    }

    WLESS_SM_radioLink = WLESS_SM_LINK_OK;
    return false;
}

int16_t WLESS_SM_encodeCapacityWh(int32_t capacityWh)
{
    const int32_t den = WLESS_SM_CAPACITY_MAX_WH - WLESS_SM_CAPACITY_THRESHOLD_WH;
    int64_t num = (int64_t)(capacityWh - WLESS_SM_CAPACITY_THRESHOLD_WH) * 32767L;
    int32_t encoded;

    if(num >= 0)
    {
        num += den / 2;
    }
    else
    {
        num -= den / 2;
    }

    encoded = (int32_t)(num / den);

    if(encoded > 32767L)
    {
        encoded = 32767L;
    }
    if(encoded < -32768L)
    {
        encoded = -32768L;
    }

    return (int16_t)encoded;
}

int32_t WLESS_SM_decodeCapacityWh(int16_t encodedCapacity)
{
    int64_t num;
    int32_t decoded;

    if(encodedCapacity >= 32767)
    {
        return WLESS_SM_CAPACITY_MAX_WH;
    }
    if(encodedCapacity <= -32768)
    {
        return WLESS_SM_CAPACITY_MIN_WH;
    }

    num = (int64_t)encodedCapacity *
          (WLESS_SM_CAPACITY_MAX_WH - WLESS_SM_CAPACITY_THRESHOLD_WH);

    if(num >= 0)
    {
        num += 32767L / 2L;
    }
    else
    {
        num -= 32767L / 2L;
    }

    decoded = (int32_t)(num / 32767L) + WLESS_SM_CAPACITY_THRESHOLD_WH;
    return decoded;
}

static bool WLESS_SM_externalStopCommand(void)
{
    if(WLESS_SM_stopCommand != 0U)
    {
        WLESS_SM_stopCommand = 0U;
        return true;
    }

    return false;
}

static bool WLESS_SM_sourceIsEmpty(void)
{
    int32_t sourceEnergy;

    sourceEnergy = (WLESS_SM_localRole == WLESS_SM_ROLE_SOURCE) ?
            WLESS_SM_decodeCapacityWh(WLESS_SM_localEnergyEncoded) :
            WLESS_SM_decodeCapacityWh(WLESS_SM_remoteEnergyEncoded);

    return WLESS_SM_updateFilteredCondition(&WLESS_SM_sourceEmptyFilter,
            sourceEnergy <= WLESS_SM_CAPACITY_MIN_WH,
            sourceEnergy >= (WLESS_SM_CAPACITY_MIN_WH +
                             WLESS_SM_ENERGY_HYSTERESIS_WH),
            true);
}

static bool WLESS_SM_loadIsFull(void)
{
    int32_t loadEnergy;

    loadEnergy = (WLESS_SM_localRole == WLESS_SM_ROLE_SOURCE) ?
            WLESS_SM_decodeCapacityWh(WLESS_SM_remoteEnergyEncoded) :
            WLESS_SM_decodeCapacityWh(WLESS_SM_localEnergyEncoded);

    return WLESS_SM_updateFilteredCondition(&WLESS_SM_loadFullFilter,
            loadEnergy >= WLESS_SM_CAPACITY_MAX_WH,
            loadEnergy <= (WLESS_SM_CAPACITY_MAX_WH -
                           WLESS_SM_ENERGY_HYSTERESIS_WH),
            true);
}

static WLESS_SM_WptState WLESS_SM_runWptDecision(void)
{
#if WLESS_SM_BUILD_VEHICLE == 1
    int32_t loadEnergy;
    int32_t sourceEnergy;

    loadEnergy = (WLESS_SM_localRole == WLESS_SM_ROLE_SOURCE) ?
            WLESS_SM_decodeCapacityWh(WLESS_SM_remoteEnergyEncoded) :
            WLESS_SM_decodeCapacityWh(WLESS_SM_localEnergyEncoded);

    sourceEnergy = (WLESS_SM_localRole == WLESS_SM_ROLE_SOURCE) ?
            WLESS_SM_decodeCapacityWh(WLESS_SM_localEnergyEncoded) :
            WLESS_SM_decodeCapacityWh(WLESS_SM_remoteEnergyEncoded);

    if(WLESS_SM_externalStopCommand() ||
       WLESS_SM_sourceIsEmpty() ||
       WLESS_SM_loadIsFull() ||
       WLESS_SM_updateFilteredCondition(&WLESS_SM_energyBalanceFilter,
          (loadEnergy > WLESS_SM_CAPACITY_THRESHOLD_WH) &&
          (sourceEnergy <= WLESS_SM_CAPACITY_THRESHOLD_WH),
          (loadEnergy <= (WLESS_SM_CAPACITY_THRESHOLD_WH -
                          WLESS_SM_ENERGY_HYSTERESIS_WH)) ||
          (sourceEnergy > (WLESS_SM_CAPACITY_THRESHOLD_WH +
                           WLESS_SM_ENERGY_HYSTERESIS_WH)),
          true))
    {
        WLESS_SM_wptState = WLESS_SM_WPT_STOP;
        return WLESS_SM_WPT_STOP;
    }
#else
    if(WLESS_SM_externalStopCommand())
    {
        WLESS_SM_wptState = WLESS_SM_WPT_STOP;
        return WLESS_SM_WPT_STOP;
    }
#endif

    WLESS_SM_wptState = WLESS_SM_WPT_RUN;
    return WLESS_SM_WPT_RUN;
}

#if WLESS_SM_POWER_CONTROL_ENABLE == 1
/*
 * Prospective FSM -> UniPD power-control integration.
 *
 * IMPORTANT:
 * - this code is excluded from the current FW while
 *   WLESS_SM_POWER_CONTROL_ENABLE remains 0;
 * - it deliberately does not overwrite UART-configurable references, clamps
 *   or ramp parameters;
 * - every INTEGRATION TODO below must be reviewed before hardware enable.
 */
static void WLESS_SM_disableUnipdPowerPath(void)
{
    /*
     * Stop the active actuators first, then clear the integration state.
     * UNIPD_disableWptHfcActuator() also forces an OST trip on the HFC PWM.
     */
    UNIPD_bbcPowerOutputEnable = 0U;
    TTPLPFC_bbcDockTestEnable = 0;
    TTPLPFC_BBC_disable();
    UNIPD_disableWptHfcActuator();
    UNIPD_wptIntegrationEnable = 0U;
    UNIPD_resetControlStatesCommand = 1U;

    /*
     * INTEGRATION TODO: validate whether an active DCLINK discharge command is
     * required here. At present OFF removes power conversion but does not
     * guarantee a controlled discharge of the DC-link capacitors.
     */
}

static void WLESS_SM_prepareUnipdBbcPath(void)
{
    /*
     * The docking-test path has ISR precedence over the UniPD BBC path and
     * must therefore be disabled before granting authority to UniPD.
     */
    TTPLPFC_bbcDockTestEnable = 0;
    TTPLPFC_bbcDockTestLegMode = TTPLPFC_BBC_DOCK_TEST_LEG_DISABLED;
    TTPLPFC_BBC_disable();

    UNIPD_disableWptHfcActuator();
    UNIPD_wptIntegrationEnable = 1U;
    UNIPD_resetControlStatesCommand = 1U;
    UNIPD_bbcPowerOutputEnable = 1U;

    /*
     * The ISR selects BOOST or BUCK from the UniPD tx_1_rx_0 input, which is
     * derived from WLESS_SM_localRole. Do not select the hardware mode here:
     * there must be one authority for the role -> converter-mode mapping.
     *
     * INTEGRATION TODO: define the policy for clearing latched VDC/IL faults.
     * They are intentionally not cleared automatically on a state transition.
     *
     * INTEGRATION TODO: validate references, current limits, duty clamp and
     * ramp parameters before setting UNIPD_bbcPowerOutputEnable.
     */
}

static void WLESS_SM_enableUnipdSourceHfc(void)
{
    /*
     * Reproduce the already tested automatic UniPD HFC actuator preparation,
     * but leave calculation of the phase shift to
     * UNIPD_runTransferredPowerIntegration().
     */
    UNIPD_wptHfcActuatorFault = 0U;

    CLLLC_hfcReceiverTestDuty_pu = 0.5f;
    CLLLC_hfcReceiverTestPhaseShiftPrimLegs_pu = 0.5f;
    CLLLC_pwmPhaseShiftPrimLegsRef_pu = 0.5f;
    CLLLC_pwmPhaseShiftPrimLegs_pu = 0.5f;
    CLLLC_hfcReceiverTestMode =
            CLLLC_HFC_RECEIVER_TEST_MODE_FIXED_SAFE_PWM;
    CLLLC_hfcReceiverTestSyncEnable = 0U;
    CLLLC_hfcReceiverTestEnable = 1U;
    CLLLC_hfcReceiverTestRun = 1U;
    CLLLC_clearTrip = 1;
    UNIPD_wptHfcActuatorEnable = 1U;

    /*
     * INTEGRATION TODO: before enabling this path on hardware, validate:
     * - SOURCE local role, LOAD remote role and radio link;
     * - availability/validity of local and remote ITANK_MOD;
     * - SOURCE synthetic-current policy while its physical channel is not
     *   validated;
     * - BOOST-ready sequencing and the minimum DCLINK condition;
     * - HFC trip-clear policy.
     */
}
#endif

static void WLESS_SM_setPowerCommand(WLESS_SM_PowerCommand command)
{
    WLESS_SM_powerCommand = command;

#if WLESS_SM_POWER_CONTROL_ENABLE == 1
    if(command == WLESS_SM_POWER_CMD_OFF)
    {
        WLESS_SM_disableUnipdPowerPath();
    }
    else if(command == WLESS_SM_POWER_CMD_SOURCE_PRECHARGE)
    {
        /*
         * SOURCE precharge: run the BBC UniPD path in BOOST mode while HFC
         * remains tripped. The FSM will request SOURCE_ON only after its
         * VBUS-ready condition.
         */
        WLESS_SM_prepareUnipdBbcPath();
    }
    else if(command == WLESS_SM_POWER_CMD_SOURCE_ON)
    {
        /*
         * Keep the BOOST loop running and add the transmitting HFC actuator.
         * Do not reset the BBC controller here: it has already reached the
         * precharge operating point.
         */
        WLESS_SM_enableUnipdSourceHfc();
    }
    else if(command == WLESS_SM_POWER_CMD_LOAD_PRECHARGE)
    {
        /*
         * LOAD precharge: UniPD selects BUCK from the local role. The HFC
         * bridge remains disabled because the LOAD rectifier is passive.
         */
        WLESS_SM_prepareUnipdBbcPath();
    }
    else if(command == WLESS_SM_POWER_CMD_LOAD_ON)
    {
        /*
         * Keep the BUCK UniPD loop and transferred-power calculation active.
         * They were enabled by LOAD_PRECHARGE; no active receiver bridge
         * command is issued.
         */
        UNIPD_disableWptHfcActuator();
    }
#endif
}

#if WLESS_SM_BUILD_VEHICLE == 0
static void WLESS_SM_resetLocalExchange(void)
{
    WLESS_SM_powerToLoad = 0;
    WLESS_SM_iCoilErr = 0;
    WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
}
#endif

void WLESS_SM_init(void)
{
#if WLESS_SM_TICK_GPIO_TEST_ENABLE == 1
    GPIO_setPinConfig(GPIO_30_GPIO30);
    GPIO_setControllerCore(WLESS_SM_TICK_GPIO_PIN, GPIO_CORE_CPU1);
    GPIO_setPadConfig(WLESS_SM_TICK_GPIO_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(WLESS_SM_TICK_GPIO_PIN, GPIO_DIR_MODE_OUT);
    GPIO_writePin(WLESS_SM_TICK_GPIO_PIN, 0U);
#endif

    WLESS_SM_state = WLESS_SM_STATE_STANDBY;
    WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
    WLESS_SM_remoteRole = WLESS_SM_ROLE_NONE;
    WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
    WLESS_SM_remoteCtrlState = WLESS_SM_CTRL_IDLE;
    WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
    WLESS_SM_remoteAbort = WLESS_SM_ABORT_DISABLED;
    WLESS_SM_radioLink = WLESS_SM_LINK_OK;
    WLESS_SM_opMode = WLESS_SM_MODE_AUTO;
    WLESS_SM_wptState = WLESS_SM_WPT_STOP;
    WLESS_SM_powerCommand = WLESS_SM_POWER_CMD_OFF;
    WLESS_SM_localEnergyEncoded = 0;
    WLESS_SM_remoteEnergyEncoded = 0;
    WLESS_SM_vBus_V = 0U;
    WLESS_SM_iBat_mA = 0U;
    WLESS_SM_iCoil_mA = 0U;
    WLESS_SM_powerToLoad = 0;
    WLESS_SM_iCoilErr = 0;
    WLESS_SM_noAckCount = 0U;
    WLESS_SM_initOkCommand = 0U;
    WLESS_SM_stopCommand = 0U;
    WLESS_SM_diagnosticMessagePending = 0U;
    WLESS_SM_operationMessagePending = 0U;
    WLESS_SM_statusUpdatePending = 1U;
    WLESS_SM_pendingTicks = 0U;
    WLESS_SM_pendingTicksMax = 0U;
    WLESS_SM_ticksDropped = 0U;
    WLESS_SM_sourceTickAccumulator = 0UL;
    WLESS_SM_tickCounter = 0UL;
    WLESS_SM_stateStepCounter = 0UL;
    WLESS_SM_resetFilter(&WLESS_SM_sourceEmptyFilter);
    WLESS_SM_resetFilter(&WLESS_SM_loadFullFilter);
    WLESS_SM_resetFilter(&WLESS_SM_energyBalanceFilter);
    WLESS_SM_resetFilter(&WLESS_SM_vBusFilter);
    WLESS_SM_resetFilter(&WLESS_SM_iBatFilter);
    WLESS_SM_resetFilter(&WLESS_SM_iCoilFilter);
    WLESS_SM_filterState = WLESS_SM_state;
    WLESS_SM_filterRole = WLESS_SM_localRole;
}

void WLESS_SM_onSourceTickIsr(void)
{
    WLESS_SM_sourceTickAccumulator += WLESS_SM_BASE_TICK_HZ;

    if(WLESS_SM_sourceTickAccumulator >= WLESS_SM_SOURCE_TICK_HZ)
    {
        WLESS_SM_sourceTickAccumulator -= WLESS_SM_SOURCE_TICK_HZ;

#if WLESS_SM_TICK_GPIO_TEST_ENABLE == 1
        GPIO_togglePin(WLESS_SM_TICK_GPIO_PIN);
#endif

        if(WLESS_SM_pendingTicks < WLESS_SM_PENDING_TICK_LIMIT)
        {
            WLESS_SM_pendingTicks++;

            if(WLESS_SM_pendingTicks > WLESS_SM_pendingTicksMax)
            {
                WLESS_SM_pendingTicksMax = WLESS_SM_pendingTicks;
            }
        }
        else
        {
            WLESS_SM_ticksDropped++;
        }
    }
}

void WLESS_SM_servicePendingTicks(void)
{
    while(WLESS_SM_pendingTicks != 0U)
    {
        WLESS_SM_pendingTicks--;
        WLESS_SM_runBackgroundTick();
    }
}

void WLESS_SM_runBackgroundTick(void)
{
#if WLESS_SM_BUILD_VEHICLE == 1
    bool operationStateActive;
#endif

    WLESS_SM_tickCounter++;

#if WLESS_SM_BUILD_VEHICLE == 1
    operationStateActive =
            (WLESS_SM_state == WLESS_SM_STATE_SOURCE_ON) ||
            (WLESS_SM_state == WLESS_SM_STATE_LOAD_ON) ||
            (WLESS_SM_state == WLESS_SM_STATE_PRECHARGE_LOAD) ||
            (WLESS_SM_state == WLESS_SM_STATE_DISCHARGE_SOURCE);

    if(operationStateActive &&
       ((WLESS_SM_tickCounter % WLESS_SM_OPERATION_TICKS) == 0UL))
    {
        WLESS_SM_operationMessagePending = 1U;
    }
#endif

    if((WLESS_SM_tickCounter % WLESS_SM_DIAGNOSTIC_TICKS) == 0UL)
    {
        WLESS_SM_diagnosticMessagePending = 1U;

#if WLESS_SM_BUILD_VEHICLE == 1
        if(!operationStateActive)
        {
            WLESS_SM_operationMessagePending = 1U;
        }
#else
        if(WLESS_SM_noAckCount < 15U)
        {
            WLESS_SM_noAckCount++;
        }
#endif
    }
}

#if WLESS_SM_BUILD_VEHICLE == 1
static void WLESS_SM_runVehicleStandbyOnly(void)
{
    if(WLESS_SM_radioLinkKo())
    {
        WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
        WLESS_SM_powerToLoad = 0;
        WLESS_SM_iCoilErr = 0;
        WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
        WLESS_SM_state = WLESS_SM_STATE_DISCOVERY;
    }
    else if(WLESS_SM_opMode == WLESS_SM_MODE_MANUAL)
    {
        if(WLESS_SM_localRole == WLESS_SM_ROLE_SOURCE)
        {
            WLESS_SM_localCtrlState = WLESS_SM_CTRL_INIT;
            WLESS_SM_powerToLoad = 0;
            WLESS_SM_iCoilErr = 0;
            WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
            WLESS_SM_state = WLESS_SM_STATE_INIT_SOURCE;
        }
        else if(WLESS_SM_localRole == WLESS_SM_ROLE_LOAD)
        {
            WLESS_SM_localCtrlState = WLESS_SM_CTRL_INIT;
            WLESS_SM_powerToLoad = 0;
            WLESS_SM_iCoilErr = 0;
            WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
            WLESS_SM_state = WLESS_SM_STATE_INIT_LOAD;
        }
    }
}

static void WLESS_SM_runVehicleStep(void)
{
    int32_t energyDifference;

    switch(WLESS_SM_state)
    {
        case WLESS_SM_STATE_STANDBY:
            if(WLESS_SM_radioLinkKo())
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_DISCOVERY;
            }
            else if(WLESS_SM_opMode == WLESS_SM_MODE_MANUAL)
            {
                if(WLESS_SM_localRole == WLESS_SM_ROLE_SOURCE)
                {
                    WLESS_SM_localCtrlState = WLESS_SM_CTRL_INIT;
                    WLESS_SM_powerToLoad = 0;
                    WLESS_SM_iCoilErr = 0;
                    WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                    WLESS_SM_state = WLESS_SM_STATE_INIT_SOURCE;
                }
                else if(WLESS_SM_localRole == WLESS_SM_ROLE_LOAD)
                {
                    WLESS_SM_localCtrlState = WLESS_SM_CTRL_INIT;
                    WLESS_SM_powerToLoad = 0;
                    WLESS_SM_iCoilErr = 0;
                    WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                    WLESS_SM_state = WLESS_SM_STATE_INIT_LOAD;
                }
            }
            break;

        case WLESS_SM_STATE_DISCOVERY:
            if(!WLESS_SM_radioLinkKo())
            {
                if(WLESS_SM_opMode == WLESS_SM_MODE_MANUAL)
                {
                    WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                    WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                    WLESS_SM_powerToLoad = 0;
                    WLESS_SM_iCoilErr = 0;
                    WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                    WLESS_SM_state = WLESS_SM_STATE_STANDBY;
                }
                else
                {
                    energyDifference =
                        WLESS_SM_decodeCapacityWh(WLESS_SM_localEnergyEncoded) -
                        WLESS_SM_decodeCapacityWh(WLESS_SM_remoteEnergyEncoded);
                    WLESS_SM_powerToLoad = 0;
                    WLESS_SM_iCoilErr = 0;
                    WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;

                    if(energyDifference >= WLESS_SM_ROLE_DEADBAND_WH)
                    {
                        WLESS_SM_localRole = WLESS_SM_ROLE_SOURCE;
                        WLESS_SM_localCtrlState = WLESS_SM_CTRL_INIT;
                        WLESS_SM_state = WLESS_SM_STATE_INIT_SOURCE;
                    }
                    else if(energyDifference <= -WLESS_SM_ROLE_DEADBAND_WH)
                    {
                        WLESS_SM_localRole = WLESS_SM_ROLE_LOAD;
                        WLESS_SM_localCtrlState = WLESS_SM_CTRL_INIT;
                        WLESS_SM_state = WLESS_SM_STATE_INIT_LOAD;
                    }
                    else
                    {
                        WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                        WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                    }
                }
            }
            break;

        case WLESS_SM_STATE_INIT_SOURCE:
            if(WLESS_SM_initOkCommand != 0U)
            {
                WLESS_SM_initOkCommand = 0U;
                WLESS_SM_localRole = WLESS_SM_ROLE_SOURCE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_INITOK;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_WAIT_LOAD;
            }
            break;

        case WLESS_SM_STATE_INIT_LOAD:
            if(WLESS_SM_initOkCommand != 0U)
            {
                WLESS_SM_initOkCommand = 0U;
                WLESS_SM_localRole = WLESS_SM_ROLE_LOAD;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_INITOK;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_WAIT_SOURCE;
            }
            break;

        case WLESS_SM_STATE_WAIT_LOAD:
            if(WLESS_SM_radioLinkKo())
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_DISCOVERY;
            }
            else if(WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_INITOK)
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_SOURCE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_PRECHARGE);
                WLESS_SM_state = WLESS_SM_STATE_PRECHARGE_SOURCE;
            }
            break;

        case WLESS_SM_STATE_WAIT_SOURCE:
            if(WLESS_SM_radioLinkKo())
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_DISCOVERY;
            }
            else if(WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTON)
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_LOAD;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_PRECHARGE);
                WLESS_SM_state = WLESS_SM_STATE_PRECHARGE_LOAD;
            }
            break;

        case WLESS_SM_STATE_PRECHARGE_SOURCE:
            if(WLESS_SM_radioLinkKo() ||
               (WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if(WLESS_SM_confirmVBusReady())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_ON);
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_ON;
            }
            break;

        case WLESS_SM_STATE_PRECHARGE_LOAD:
            if(WLESS_SM_radioLinkKo() ||
               (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_LOAD;
            }
            else if(WLESS_SM_confirmVBusReady())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_ON);
                WLESS_SM_state = WLESS_SM_STATE_LOAD_ON;
            }

            if(WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP)
            {
                WLESS_SM_localAbort = WLESS_SM_ABORT_ENABLED;
            }
            break;

        case WLESS_SM_STATE_SOURCE_ON:
            if(WLESS_SM_radioLinkKo() ||
               (WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_DISCHARGE_SOURCE;
            }
            break;

        case WLESS_SM_STATE_LOAD_ON:
            if((WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF) ||
               WLESS_SM_radioLinkKo())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_DISCHARGE_LOAD;
            }

            if(WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP)
            {
                WLESS_SM_localAbort = WLESS_SM_ABORT_ENABLED;
            }
            break;

        case WLESS_SM_STATE_DISCHARGE_SOURCE:
            if(WLESS_SM_confirmIBatLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_OFF;
            }
            break;

        case WLESS_SM_STATE_DISCHARGE_LOAD:
            if(WLESS_SM_confirmICoilLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_LOAD_OFF;
            }
            break;

#if WLESS_SM_TEST_VEHICLE_OPERATION_STATES_ONLY == 0
        case WLESS_SM_STATE_SOURCE_OFF:
            if(WLESS_SM_confirmICoilLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_END;
            }
            break;

        case WLESS_SM_STATE_LOAD_OFF:
            if(WLESS_SM_confirmIBatLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTEND;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_LOAD_END;
            }
            break;

        case WLESS_SM_STATE_SOURCE_END:
            if(WLESS_SM_radioLinkKo())
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_DISCOVERY;
            }
            else if(WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTEND)
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_STANDBY;
            }
            break;

        case WLESS_SM_STATE_LOAD_END:
            if(WLESS_SM_radioLinkKo())
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_DISCOVERY;
            }
            else if(WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_IDLE)
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_STANDBY;
            }
            break;

        case WLESS_SM_STATE_ENDCHARGE_SOURCE:
            if(WLESS_SM_confirmIBatLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_END;
            }
            break;

        case WLESS_SM_STATE_ENDCHARGE_LOAD:
            if(WLESS_SM_confirmICoilLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTEND;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_state = WLESS_SM_STATE_LOAD_END;
            }
            break;
#endif
    }
}
#else
static void WLESS_SM_runStationStep(void)
{
    switch(WLESS_SM_state)
    {
        case WLESS_SM_STATE_STANDBY:
            if((WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_INIT) &&
               (WLESS_SM_remoteRole == WLESS_SM_ROLE_LOAD))
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_SOURCE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_INIT;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_INIT_SOURCE;
            }
            else if((WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_INIT) &&
                    (WLESS_SM_remoteRole == WLESS_SM_ROLE_SOURCE))
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_LOAD;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_INIT;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_INIT_LOAD;
            }
            break;

        case WLESS_SM_STATE_INIT_SOURCE:
            if(WLESS_SM_initOkCommand != 0U)
            {
                WLESS_SM_initOkCommand = 0U;
                WLESS_SM_localRole = WLESS_SM_ROLE_SOURCE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_INITOK;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_WAIT_LOAD;
            }
            break;

        case WLESS_SM_STATE_INIT_LOAD:
            if(WLESS_SM_initOkCommand != 0U)
            {
                WLESS_SM_initOkCommand = 0U;
                WLESS_SM_localRole = WLESS_SM_ROLE_LOAD;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_INITOK;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_WAIT_SOURCE;
            }
            break;

        case WLESS_SM_STATE_WAIT_LOAD:
            if(WLESS_SM_radioLinkKo())
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_STANDBY;
            }
            else if(WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_INITOK)
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_SOURCE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_PRECHARGE);
                WLESS_SM_state = WLESS_SM_STATE_PRECHARGE_SOURCE;
            }
            break;

        case WLESS_SM_STATE_WAIT_SOURCE:
            if(WLESS_SM_radioLinkKo())
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_STANDBY;
            }
            else if(WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTON)
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_LOAD;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_PRECHARGE);
                WLESS_SM_state = WLESS_SM_STATE_PRECHARGE_LOAD;
            }
            break;

        case WLESS_SM_STATE_PRECHARGE_SOURCE:
            if(WLESS_SM_radioLinkKo() ||
               (WLESS_SM_remoteAbort == WLESS_SM_ABORT_ENABLED))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if(WLESS_SM_confirmVBusReady())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_ON);
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_ON;
            }
            break;

        case WLESS_SM_STATE_PRECHARGE_LOAD:
            if(WLESS_SM_radioLinkKo() ||
               (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_LOAD;
            }
            else if(WLESS_SM_confirmVBusReady())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_ON);
                WLESS_SM_state = WLESS_SM_STATE_LOAD_ON;
            }
            break;

        case WLESS_SM_STATE_SOURCE_ON:
            if(WLESS_SM_radioLinkKo() ||
               (WLESS_SM_remoteAbort == WLESS_SM_ABORT_ENABLED))
            {
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_DISCHARGE_SOURCE;
            }
            break;

        case WLESS_SM_STATE_LOAD_ON:
            if((WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF) ||
               WLESS_SM_radioLinkKo())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_DISCHARGE_LOAD;
            }
            break;

        case WLESS_SM_STATE_DISCHARGE_SOURCE:
            if(WLESS_SM_confirmIBatLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_OFF;
            }
            break;

        case WLESS_SM_STATE_DISCHARGE_LOAD:
            if(WLESS_SM_confirmICoilLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_LOAD_OFF;
            }
            break;

        case WLESS_SM_STATE_SOURCE_OFF:
            if(WLESS_SM_confirmICoilLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_END;
            }
            break;

        case WLESS_SM_STATE_LOAD_OFF:
            if(WLESS_SM_confirmIBatLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTEND;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_LOAD_END;
            }
            break;

        case WLESS_SM_STATE_SOURCE_END:
            if(WLESS_SM_radioLinkKo() ||
               (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTEND))
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_STANDBY;
            }
            break;

        case WLESS_SM_STATE_LOAD_END:
            if(WLESS_SM_radioLinkKo() ||
               (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_IDLE))
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_NONE;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_IDLE;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_STANDBY;
            }
            break;

        case WLESS_SM_STATE_ENDCHARGE_SOURCE:
            if(WLESS_SM_confirmIBatLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_END;
            }
            break;

        case WLESS_SM_STATE_ENDCHARGE_LOAD:
            if(WLESS_SM_confirmICoilLow())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTEND;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_LOAD_END;
            }
            break;

        default:
            break;
    }
}
#endif

void WLESS_SM_run(void)
{
    WLESS_SM_State previousState;

    if(WLESS_SM_diagnosticMessagePending == 0U)
    {
        return;
    }

    WLESS_SM_diagnosticMessagePending = 0U;
    WLESS_SM_stateStepCounter++;

    if(WLESS_SM_filterState != WLESS_SM_state)
    {
        WLESS_SM_resetFilter(&WLESS_SM_vBusFilter);
        WLESS_SM_resetFilter(&WLESS_SM_iBatFilter);
        WLESS_SM_resetFilter(&WLESS_SM_iCoilFilter);
        WLESS_SM_filterState = WLESS_SM_state;
    }

    if(WLESS_SM_filterRole != WLESS_SM_localRole)
    {
        WLESS_SM_resetFilter(&WLESS_SM_sourceEmptyFilter);
        WLESS_SM_resetFilter(&WLESS_SM_loadFullFilter);
        WLESS_SM_resetFilter(&WLESS_SM_energyBalanceFilter);
        WLESS_SM_filterRole = WLESS_SM_localRole;
    }

    previousState = WLESS_SM_state;

#if WLESS_SM_TEST_RUN_DISPATCH_ONLY == 1
    // Diagnostic isolation: verify WLESS_SM_run() scheduling and bookkeeping
    // without entering either role-specific state-machine implementation.
#elif WLESS_SM_TEST_VEHICLE_STANDBY_ONLY == 1
    WLESS_SM_runVehicleStandbyOnly();
#elif WLESS_SM_BUILD_VEHICLE == 1
    WLESS_SM_runVehicleStep();
#else
    WLESS_SM_runStationStep();
#endif

    if(WLESS_SM_state != previousState)
    {
        WLESS_SM_statusUpdatePending = 1U;
    }
}
