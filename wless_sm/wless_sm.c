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
#pragma RETAIN(WLESS_SM_bootSourceMode)
#pragma RETAIN(WLESS_SM_localEnergyEncoded)
#pragma RETAIN(WLESS_SM_remoteEnergyEncoded)
#pragma RETAIN(WLESS_SM_vBus_V)
#pragma RETAIN(WLESS_SM_bootSourceVBusMin_V)
#pragma RETAIN(WLESS_SM_bootLoadVBusMin_V)
#pragma RETAIN(WLESS_SM_bootBbcDutyMax_mpu)
#pragma RETAIN(WLESS_SM_iBat_mA)
#pragma RETAIN(WLESS_SM_iCoil_mA)
#pragma RETAIN(WLESS_SM_powerToLoad)
#pragma RETAIN(WLESS_SM_remotePowerToLoad)
#pragma RETAIN(WLESS_SM_iCoilErr)
#pragma RETAIN(WLESS_SM_remoteICoilErr)
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
#pragma RETAIN(WLESS_SM_bootTraceState)
#pragma RETAIN(WLESS_SM_bootTraceNextState)
#pragma RETAIN(WLESS_SM_bootTraceVBus_V)
#pragma RETAIN(WLESS_SM_bootTraceRemoteCtrl)
#pragma RETAIN(WLESS_SM_bootTracePhase_mpu)
#pragma RETAIN(WLESS_SM_bootTraceReason)
#pragma RETAIN(WLESS_SM_bootHfcHandoffActive)
#pragma RETAIN(WLESS_SM_bootHfcHandoffPhase_mpu)
#pragma RETAIN(WLESS_SM_bootHfcHandoffLimit_mpu)

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
volatile WLESS_SM_BootSourceMode WLESS_SM_bootSourceMode =
        WLESS_SM_BOOT_SOURCE_EXTERNAL;

volatile int16_t WLESS_SM_localEnergyEncoded;
volatile int16_t WLESS_SM_remoteEnergyEncoded;
volatile uint16_t WLESS_SM_vBus_V;
volatile uint16_t WLESS_SM_iBat_mA;
volatile uint16_t WLESS_SM_iCoil_mA;
volatile uint16_t WLESS_SM_vBusMin_V = WLESS_SM_VBUS_MIN_V;
volatile uint16_t WLESS_SM_bootSourceVBusMin_V =
        WLESS_SM_BOOT_SOURCE_VBUS_MIN_V;
volatile uint16_t WLESS_SM_bootLoadVBusMin_V =
        WLESS_SM_BOOT_LOAD_VBUS_MIN_V;
volatile uint16_t WLESS_SM_bootBbcDutyMax_mpu =
        WLESS_SM_BOOT_BBC_DUTY_MAX_MPU;
volatile uint16_t WLESS_SM_iBatMin_mA = WLESS_SM_IBAT_MIN_MA;
volatile uint16_t WLESS_SM_iCoilMin_mA = WLESS_SM_ICOIL_MIN_MA;
volatile int16_t WLESS_SM_powerToLoad;
volatile int16_t WLESS_SM_remotePowerToLoad;
volatile int16_t WLESS_SM_iCoilErr;
volatile int16_t WLESS_SM_remoteICoilErr;
volatile uint16_t WLESS_SM_noAckCount;
volatile uint16_t WLESS_SM_noAckMaxCount;

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
volatile uint16_t WLESS_SM_bootTraceState;
volatile uint16_t WLESS_SM_bootTraceNextState;
volatile uint16_t WLESS_SM_bootTraceVBus_V;
volatile uint16_t WLESS_SM_bootTraceRemoteCtrl;
volatile uint16_t WLESS_SM_bootTracePhase_mpu;
volatile uint16_t WLESS_SM_bootTraceReason;
volatile uint16_t WLESS_SM_bootHfcHandoffActive;
volatile uint16_t WLESS_SM_bootHfcHandoffPhase_mpu;
volatile uint16_t WLESS_SM_bootHfcHandoffLimit_mpu;

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
static WLESS_SM_ConditionFilter WLESS_SM_bootLoadVBusFilter;
static WLESS_SM_ConditionFilter WLESS_SM_iBatFilter;
static WLESS_SM_ConditionFilter WLESS_SM_iCoilFilter;
static WLESS_SM_State WLESS_SM_filterState;
static WLESS_SM_Role WLESS_SM_filterRole;

/*
 * Il tick normale della FSM e' diagnostico (1 Hz). Il bootstrap deve invece
 * reagire alla corrente coil e al consenso VDC LOAD alla cadenza operativa.
 */
static volatile uint16_t WLESS_SM_bootstrapStepPending;
static volatile uint16_t WLESS_SM_bootHfcSoftCurrentTrip;
static volatile uint16_t WLESS_SM_bootLoadOverVoltage;
static uint16_t WLESS_SM_bootSourceReadyTicks;
static uint16_t WLESS_SM_bootHfcRampTicks;
static float WLESS_SM_bootHfcTargetPhase_pu;
static uint16_t WLESS_SM_bootHfcHandoffRampTicks;
static float WLESS_SM_bootHfcConfiguredPhaseMax_pu;
static uint16_t WLESS_SM_bootBbcControlTicks;
static uint16_t WLESS_SM_bootBbcDutyArmTicks;
static uint16_t WLESS_SM_bootBbcTargetReadyTicks;

#if WLESS_SM_POWER_CONTROL_ENABLE == 1
static bool WLESS_SM_confirmBootSourceVBusReady(void);
static bool WLESS_SM_confirmBoostGovernorReady(void);
static bool WLESS_SM_confirmSourceBootstrapReady(void);
static bool WLESS_SM_boostGovernorAtTarget(void);

// Isteresi di mantenimento VBUS per il bootstrap HFC
static uint16_t WLESS_SM_getBootSourceVBusHoldHyst_V(void)
{
    const uint32_t scaledHyst_V =
            (((uint32_t)WLESS_SM_bootSourceVBusMin_V *
              WLESS_SM_BOOT_SOURCE_VBUS_HOLD_HYST_PERMILLE) + 999U) / 1000U;

    return (scaledHyst_V > WLESS_SM_BOOT_SOURCE_VBUS_HOLD_HYST_MIN_V) ?
            (uint16_t)scaledHyst_V :
            WLESS_SM_BOOT_SOURCE_VBUS_HOLD_HYST_MIN_V;
}

// Cattura lo stato della FSM al momento del bootstrap per la diagnostica
static void WLESS_SM_captureBootstrapTrace(WLESS_SM_State state)
{
    WLESS_SM_bootTraceState = (uint16_t)state;
    WLESS_SM_bootTraceVBus_V = WLESS_SM_vBus_V;
    WLESS_SM_bootTraceRemoteCtrl = (uint16_t)WLESS_SM_remoteCtrlState;
    WLESS_SM_bootTracePhase_mpu = (uint16_t)(UNIPD_wptHfcManualPhase_pu * 1000.0f);
}
#endif

#if WLESS_SM_POWER_CONTROL_ENABLE == 1
static bool WLESS_SM_isBootstrapState(WLESS_SM_State state)
{
    return (state == WLESS_SM_STATE_PRECHARGE_SOURCE) ||
           (state == WLESS_SM_STATE_HFC_BOOT_SOURCE) ||
           (state == WLESS_SM_STATE_WAIT_VBUS_LOAD) ||
           (state == WLESS_SM_STATE_WAIT_RUN_LOAD);
}

/*
 * Per il SOURCE alimentato dal BOOST, SMBOOTSRC esprime la soglia minima di
 * sicurezza, non il punto operativo. L'HFC puo' iniziare soltanto dopo che
 * il governor BBC ha raggiunto il riferimento VDC finale e il bus lo segue.
 * Il duty-cap resta invece intenzionalmente inserito fino a WPTREADY: prima
 * di quel consenso il LOAD non costituisce ancora un pozzo energetico certo.
 */
static bool WLESS_SM_boostGovernorAtTarget(void)
{
    const float configuredVdcRef_V = UNIPD_bbcSyntheticInputs.v_dc_pbat_rif;
    const float requiredVdc_V = configuredVdcRef_V -
            (float)WLESS_SM_BOOT_BBC_VDC_TRACK_MARGIN_V;

    if((configuredVdcRef_V <= 0.0f) ||
       (WLESS_SM_vBus_V < (uint16_t)((requiredVdc_V > 0.0f) ?
                                     requiredVdc_V : 0.0f)) ||
       ((UNIPD_bbcBootstrapVdcReferenceLimitEnable != 0U) &&
        ((UNIPD_bbcBootstrapVdcReferenceLimit_Volts + 0.001f) <
         configuredVdcRef_V)))
    {
        return false;
    }

    return true;
}

static bool WLESS_SM_confirmBoostGovernorReady(void)
{
    if(!WLESS_SM_boostGovernorAtTarget())
    {
        WLESS_SM_bootBbcTargetReadyTicks = 0U;
        return false;
    }

    if(WLESS_SM_bootBbcTargetReadyTicks <
            WLESS_SM_BOOT_BBC_TARGET_CONFIRM_TICKS)
    {
        WLESS_SM_bootBbcTargetReadyTicks++;
    }

    return WLESS_SM_bootBbcTargetReadyTicks >=
            WLESS_SM_BOOT_BBC_TARGET_CONFIRM_TICKS;
}

static bool WLESS_SM_confirmSourceBootstrapReady(void)
{
    if(WLESS_SM_bootSourceMode == WLESS_SM_BOOT_SOURCE_BOOST)
    {
        return WLESS_SM_confirmBoostGovernorReady();
    }

    return WLESS_SM_confirmBootSourceVBusReady();
}

static void WLESS_SM_serviceBootstrapFastGuards(void)
{
    if((WLESS_SM_state == WLESS_SM_STATE_PRECHARGE_SOURCE) &&
       (WLESS_SM_bootSourceMode == WLESS_SM_BOOT_SOURCE_BOOST) &&
       (UNIPD_bbcPowerOutputEnable == 0U) &&
       (WLESS_SM_vBus_V > WLESS_SM_BOOT_BBC_ARM_VBUS_MIN_V))
    {
        /*
         * Il VIN del SOURCE deve essere realmente presente prima di liberare
         * il PWM BOOST. In precedenza il consenso veniva dato quando la FSM
         * entrava in PRECHARGE_SOURCE, quindi anche a VIN OFF: i transitori
         * del ponte potevano latching il trip di corrente prima del run.
         *
         * Il DCLINK viene osservato con l'acquisizione fisica, prima di ogni
         * override UniPD; il percorso passivo del banco lo porta a ~11,5 V con
         * VIN=12 V. Il limite energetico da 0,5 A e' gia' predisposto.
         */
        UNIPD_resetControlStatesCommand = 1U;
        /* Il riferimento parte dalla tensione realmente gia' presente sul
         * condensatore: evita il gradino VDC~11 V -> UR=24 V. */
        UNIPD_bbcBootstrapVdcReferenceLimit_Volts =
                (float)WLESS_SM_vBus_V;
        UNIPD_bbcBootstrapVdcReferenceLimitEnable = 1U;
        WLESS_SM_bootBbcControlTicks = 0U;
        WLESS_SM_bootBbcDutyArmTicks = 0U;
        WLESS_SM_bootBbcTargetReadyTicks = 0U;
        UNIPD_bbcPowerOutputEnable = 1U;
    }

    if(((WLESS_SM_state == WLESS_SM_STATE_PRECHARGE_SOURCE) ||
        (WLESS_SM_state == WLESS_SM_STATE_HFC_BOOT_SOURCE) ||
        (WLESS_SM_state == WLESS_SM_STATE_SOURCE_ON)) &&
       (WLESS_SM_bootSourceMode == WLESS_SM_BOOT_SOURCE_BOOST) &&
        ((UNIPD_bbcBootstrapVdcReferenceLimitEnable != 0U) ||
         (UNIPD_bbcBootstrapCurrentLimitEnable != 0U)))
    {
        /*
         * Il cap BD e' applicato dopo l'uscita UniPD. Non puo' essere
         * sostituito con UD in un unico istante: il regolatore BBC puo' avere
         * gia' accumulato richiesta e il salto reale di PWM causerebbe un
         * overshoot VDC. La conferma iniziale abilita soltanto la rampa del
         * cap; il rilascio effettivo e' eseguito piu' sotto, un passo/s.
         */
        if(UNIPD_bbcBootstrapDutyLimitEnable != 0U)
        {
            if((float)WLESS_SM_vBus_V >=
                    (UNIPD_bbcBootstrapVdcReferenceLimit_Volts -
                     (float)WLESS_SM_BOOT_BBC_VDC_TRACK_MARGIN_V))
            {
                if(WLESS_SM_bootBbcDutyArmTicks <
                        WLESS_SM_BOOT_BBC_DUTY_ARM_CONFIRM_TICKS)
                {
                    WLESS_SM_bootBbcDutyArmTicks++;
                }
            }
            else
            {
                WLESS_SM_bootBbcDutyArmTicks = 0U;
            }

            /* Finche' il bus non ha seguito il punto passivo iniziale, BD
             * resta fermo e anche VDC/IA non possono avanzare. */
            if(WLESS_SM_bootBbcDutyArmTicks <
                    WLESS_SM_BOOT_BBC_DUTY_ARM_CONFIRM_TICKS)
            {
                return;
            }
        }

        if(WLESS_SM_bootBbcControlTicks < WLESS_SM_BOOT_BBC_CONTROL_STEP_TICKS)
        {
            WLESS_SM_bootBbcControlTicks++;
        }

        if((WLESS_SM_bootBbcControlTicks >= WLESS_SM_BOOT_BBC_CONTROL_STEP_TICKS) &&
           ((float)WLESS_SM_vBus_V >=
            (UNIPD_bbcBootstrapVdcReferenceLimit_Volts -
             (float)WLESS_SM_BOOT_BBC_VDC_TRACK_MARGIN_V)))
        {
            const bool vdcReferenceStepActive =
                    (UNIPD_bbcBootstrapVdcReferenceLimitEnable != 0U);

            /* Il riferimento e il limite di corrente avanzano insieme ogni
             * secondo, ma solo se il condensatore SOURCE ha seguito il passo
             * precedente. Se VDC si arresta, la richiesta UniPD resta al
             * livello gia' raggiunto: non viene liberato un gradino UR/UA. */
            WLESS_SM_bootBbcControlTicks = 0U;

            if(vdcReferenceStepActive)
            {
                const float configuredVdcRef_V =
                        UNIPD_bbcSyntheticInputs.v_dc_pbat_rif;

                UNIPD_bbcBootstrapVdcReferenceLimit_Volts +=
                        WLESS_SM_BOOT_BBC_VDC_REF_STEP_V;
                if(UNIPD_bbcBootstrapVdcReferenceLimit_Volts >=
                        configuredVdcRef_V)
                {
                    UNIPD_bbcBootstrapVdcReferenceLimit_Volts =
                            configuredVdcRef_V;
                    UNIPD_bbcBootstrapVdcReferenceLimitEnable = 0U;
                }
            }

            /*
             * Durante HFC_BOOT il LOAD non ha ancora dichiarato WPTREADY:
             * il budget IA acquisito nel precharge resta quindi congelato.
             * Rilasciarlo in questa fase avrebbe consentito UA completo pur
             * senza una controparte energeticamente pronta. Dopo SOURCE_ON,
             * invece, il rilascio riprende soltanto con VDC SOURCE in track.
             */
            if((UNIPD_bbcBootstrapCurrentLimitEnable != 0U) &&
               (vdcReferenceStepActive ||
                (WLESS_SM_state == WLESS_SM_STATE_SOURCE_ON)))
            {
                const float configuredCurrentLimit_A =
                        UNIPD_bbcSyntheticInputs.i_bat_rif_max;

                UNIPD_bbcBootstrapCurrentLimit_Amps +=
                        WLESS_SM_BOOT_BBC_CURRENT_STEP_A;
                if(UNIPD_bbcBootstrapCurrentLimit_Amps >=
                        configuredCurrentLimit_A)
                {
                    UNIPD_bbcBootstrapCurrentLimit_Amps =
                            configuredCurrentLimit_A;
                    UNIPD_bbcBootstrapCurrentLimitEnable = 0U;
                }
            }

            /* BD resta fisso durante PRECHARGE_SOURCE e HFC_BOOT_SOURCE:
             * VDC SOURCE gia' carico non implica che il LOAD possa ancora
             * assorbire energia. Il rilascio e' ammesso solo dopo WPTREADY,
             * cioe' in SOURCE_ON, e rimane graduale per evitare il gradino
             * verso UD che causerebbe il trip UY. */
            if((UNIPD_bbcBootstrapDutyLimitEnable != 0U) &&
               (WLESS_SM_state == WLESS_SM_STATE_SOURCE_ON))
            {
                const float configuredDutyLimit_pu =
                        UNIPD_bbcPowerOutputDutyMax_pu;
                const float dutyStep_pu =
                        ((float)WLESS_SM_BOOT_BBC_DUTY_STEP_MPU) * 0.001f;

                UNIPD_bbcBootstrapDutyLimit_pu += dutyStep_pu;
                if(UNIPD_bbcBootstrapDutyLimit_pu >=
                        configuredDutyLimit_pu)
                {
                    UNIPD_bbcBootstrapDutyLimit_pu =
                            configuredDutyLimit_pu;
                    UNIPD_bbcBootstrapDutyLimitEnable = 0U;
                }
            }
        }
    }
    else
    {
        WLESS_SM_bootBbcControlTicks = 0U;
    }

    /* La parte specifica della rampa HFC resta confinata a HFC_BOOT. */
    if((WLESS_SM_state == WLESS_SM_STATE_PRECHARGE_SOURCE) ||
       (WLESS_SM_state == WLESS_SM_STATE_HFC_BOOT_SOURCE) ||
       (WLESS_SM_state == WLESS_SM_STATE_SOURCE_ON))
    {
        const bool sourceVbusReady = WLESS_SM_confirmBootSourceVBusReady();
        const uint16_t sourceHoldHyst_V =
                WLESS_SM_getBootSourceVBusHoldHyst_V();
        const uint16_t sourceHoldMin_V =
                (WLESS_SM_bootSourceVBusMin_V >
                 sourceHoldHyst_V) ?
                (WLESS_SM_bootSourceVBusMin_V -
                 sourceHoldHyst_V) : 0U;

        if(WLESS_SM_state == WLESS_SM_STATE_HFC_BOOT_SOURCE)
        {
            if(WLESS_SM_iCoil_mA >= WLESS_SM_BOOT_HFC_SOFT_COIL_LIMIT_MA)
            {
                /* Arresto prima del trip HFC fisico; WPTOFF/Abort coordinano il peer. */
                WLESS_SM_bootHfcSoftCurrentTrip = 1U;
                WLESS_SM_localAbort = WLESS_SM_ABORT_ENABLED;
                UNIPD_disableWptHfcActuator();
            }
            else if(WLESS_SM_vBus_V <= sourceHoldMin_V)
            {
                /*
                 * Il LOAD non ha ancora un bus autonomo: se l'HFC scarica il
                 * SOURCE sotto il margine di mantenimento, non proseguire verso
                 * una fase piu' energetica. Tornare lentamente al neutro lascia
                 * al BOOST il tempo di ricostituire VDC senza un OFF brusco.
                 */
                WLESS_SM_bootTraceReason = WLESS_SM_BOOT_TRACE_SOURCE_VBUS_HOLD;
                WLESS_SM_bootSourceReadyTicks = 0U;
                WLESS_SM_bootHfcRampTicks++;
                if(WLESS_SM_bootHfcRampTicks >= WLESS_SM_BOOT_HFC_PHASE_STEP_TICKS)
                {
                    const float phaseStep =
                            ((float)WLESS_SM_BOOT_HFC_PHASE_STEP_MPU) * 0.001f;

                    WLESS_SM_bootHfcRampTicks = 0U;
                    UNIPD_wptHfcManualPhase_pu += phaseStep;
                    if(UNIPD_wptHfcManualPhase_pu > 0.5f)
                    {
                        UNIPD_wptHfcManualPhase_pu = 0.5f;
                    }
                }
            }
            else if(WLESS_SM_vBus_V <= WLESS_SM_bootSourceVBusMin_V)
            {
                /* Sopra l'isteresi ma sotto il consenso: congelare la rampa e
                 * richiedere nuovamente una permanenza stabile sopra SMBOOTSRC. */
                WLESS_SM_bootSourceReadyTicks = 0U;
                WLESS_SM_bootHfcRampTicks = 0U;
            }
            /*
             * Il primo WPTBOOT ricevuto dal LOAD e' il consenso che la controparte
             * e' entrata nel proprio precharge. Prima di tale ACK il SOURCE resta
             * neutro: non esiste ancora un destinatario che possa fermare una
             * rampa HFC troppo energica.
             */
            else if((WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTBOOT) &&
                    sourceVbusReady &&
                    (UNIPD_wptHfcManualPhase_pu > WLESS_SM_bootHfcTargetPhase_pu))
            {
                WLESS_SM_bootHfcRampTicks++;
                if(WLESS_SM_bootHfcRampTicks >= WLESS_SM_BOOT_HFC_PHASE_STEP_TICKS)
                {
                    const float phaseStep =
                            ((float)WLESS_SM_BOOT_HFC_PHASE_STEP_MPU) * 0.001f;

                    WLESS_SM_bootHfcRampTicks = 0U;
                    UNIPD_wptHfcManualPhase_pu -= phaseStep;
                    if(UNIPD_wptHfcManualPhase_pu < WLESS_SM_bootHfcTargetPhase_pu)
                    {
                        UNIPD_wptHfcManualPhase_pu = WLESS_SM_bootHfcTargetPhase_pu;
                    }
                }
            }
        }
    }

    if((WLESS_SM_state == WLESS_SM_STATE_WAIT_VBUS_LOAD) ||
       (WLESS_SM_state == WLESS_SM_STATE_WAIT_RUN_LOAD))
    {
        if(WLESS_SM_vBus_V >= WLESS_SM_BOOT_LOAD_VBUS_MAX_V)
        {
            /* Il LOAD ferma localmente il BUCK e richiede lo stop al SOURCE. */
            WLESS_SM_bootLoadOverVoltage = 1U;
            WLESS_SM_localAbort = WLESS_SM_ABORT_ENABLED;
            UNIPD_bbcPowerOutputEnable = 0U;
            TTPLPFC_BBC_disable();
        }
    }
}

/*
 * Il bootstrap usa una fase fisica, mentre UniPD usa la deviazione rispetto al
 * neutro. Al consenso WPTREADY la conversione d = 0.5 - phase fisica conserva
 * il punto elettrico raggiunto; il limite UniPD viene quindi aperto lentamente.
 */
static void WLESS_SM_startUnipdHfcHandoff(void)
{
    float handoffPhase = UNIPD_wptHfcManualPhase_pu;
    float handoffDeviation;

    if(handoffPhase < 0.0f)
    {
        handoffPhase = 0.0f;
    }
    else if(handoffPhase > 0.5f)
    {
        handoffPhase = 0.5f;
    }

    handoffDeviation = 0.5f - handoffPhase;
    if(handoffDeviation > WLESS_SM_bootHfcConfiguredPhaseMax_pu)
    {
        handoffDeviation = WLESS_SM_bootHfcConfiguredPhaseMax_pu;
    }

    UNIPD_wptHfcPhaseRequested_pu = handoffDeviation;
    UNIPD_wptHfcPhaseApplied_pu = handoffDeviation;
    UNIPD_wptHfcPhaseMax_pu = handoffDeviation;
    WLESS_SM_bootHfcHandoffPhase_mpu =
            (uint16_t)(handoffDeviation * 1000.0f);
    WLESS_SM_bootHfcHandoffLimit_mpu = WLESS_SM_bootHfcHandoffPhase_mpu;
    WLESS_SM_bootHfcHandoffRampTicks = 0U;
    WLESS_SM_bootHfcHandoffActive =
            (handoffDeviation < WLESS_SM_bootHfcConfiguredPhaseMax_pu) ? 1U : 0U;
}

static void WLESS_SM_serviceHfcHandoffLimit(void)
{
    const float phaseStep =
            ((float)WLESS_SM_BOOT_HFC_HANDOFF_STEP_MPU) * 0.001f;
    const uint16_t sourceHoldHyst_V =
            WLESS_SM_getBootSourceVBusHoldHyst_V();
    const uint16_t sourceHoldMin_V =
            (WLESS_SM_bootSourceVBusMin_V > sourceHoldHyst_V) ?
            (WLESS_SM_bootSourceVBusMin_V - sourceHoldHyst_V) : 0U;

    if((WLESS_SM_state != WLESS_SM_STATE_SOURCE_ON) ||
       (WLESS_SM_bootSourceMode != WLESS_SM_BOOT_SOURCE_BOOST) ||
       (UNIPD_wptHfcActuatorEnable == 0U))
    {
        return;
    }

    /*
     * Dopo WPTON l'HFC continua a essere attivo. Se il DCLINK SOURCE perde
     * il margine di bootstrap, non basta congelare l'apertura BBC: il tank
     * continuerebbe a sottrarre energia. Ridurre il limite differenziale
     * UniPD riporta gradualmente la fase fisica verso 500 mpu (zero
     * trasferimento); al recupero del bus lo stesso limite verra' riaperto
     * dalla rampa normale senza un gradino di PWM.
     */
    if(WLESS_SM_vBus_V <= sourceHoldMin_V)
    {
        WLESS_SM_bootHfcHandoffActive = 1U;
        WLESS_SM_bootHfcHandoffRampTicks++;
        if(WLESS_SM_bootHfcHandoffRampTicks <
                WLESS_SM_BOOT_HFC_HANDOFF_STEP_TICKS)
        {
            return;
        }

        WLESS_SM_bootHfcHandoffRampTicks = 0U;
        if(UNIPD_wptHfcPhaseMax_pu > phaseStep)
        {
            UNIPD_wptHfcPhaseMax_pu -= phaseStep;
        }
        else
        {
            UNIPD_wptHfcPhaseMax_pu = 0.0f;
        }
        WLESS_SM_bootHfcHandoffLimit_mpu =
                (uint16_t)(UNIPD_wptHfcPhaseMax_pu * 1000.0f);
        return;
    }

    if(WLESS_SM_bootHfcHandoffActive == 0U)
    {
        return;
    }

    WLESS_SM_bootHfcHandoffRampTicks++;
    if(WLESS_SM_bootHfcHandoffRampTicks <
            WLESS_SM_BOOT_HFC_HANDOFF_STEP_TICKS)
    {
        return;
    }

    WLESS_SM_bootHfcHandoffRampTicks = 0U;
    UNIPD_wptHfcPhaseMax_pu += phaseStep;
    if(UNIPD_wptHfcPhaseMax_pu >= WLESS_SM_bootHfcConfiguredPhaseMax_pu)
    {
        UNIPD_wptHfcPhaseMax_pu = WLESS_SM_bootHfcConfiguredPhaseMax_pu;
        WLESS_SM_bootHfcHandoffActive = 0U;
    }
    WLESS_SM_bootHfcHandoffLimit_mpu =
            (uint16_t)(UNIPD_wptHfcPhaseMax_pu * 1000.0f);
}

/*
 * Indicatore visivo del solo SOURCE per le prove di bootstrap.  Il LED non
 * partecipa a decisioni di controllo: rende osservabile senza traffico UART
 * se UniPD dispone gia' di tutti i margini configurati.
 *
 *  OFF:   nessun run SOURCE attivo;
 *  BLINK: precharge/bootstrap o almeno un vincolo BBC/HFC ancora attivo;
 *  ON:    SOURCE_ON con BBC e HFC interamente consegnati a UniPD.
 *
 * In particolare WPTON da solo non basta: puo' essere pubblicato mentre il
 * cap duty BBC e/o il limite differenziale HFC sono ancora in rilascio.
 */
static void WLESS_SM_serviceBootstrapLed(void)
{
    bool sourceRun = false;
    bool bootstrapLimited = false;
    bool bbcVdcLimited = false;
    bool bbcCurrentLimited = false;
    bool bbcDutyLimited = false;
    uint16_t ledState = 0U;

    if((WLESS_SM_localRole == WLESS_SM_ROLE_SOURCE) &&
       ((WLESS_SM_state == WLESS_SM_STATE_PRECHARGE_SOURCE) ||
        (WLESS_SM_state == WLESS_SM_STATE_HFC_BOOT_SOURCE) ||
        (WLESS_SM_state == WLESS_SM_STATE_SOURCE_ON)))
    {
        sourceRun = true;
        /* Un flag bootstrap puo' restare armato anche quando il suo valore
         * ha ormai superato il riferimento/limite normale. Per il LED conta
         * soltanto una restrizione effettiva, altrimenti UR=24 non potrebbe
         * mai dichiarare finito un bootstrap il cui ceiling formale e' 60 V. */
        bbcVdcLimited =
                (UNIPD_bbcBootstrapVdcReferenceLimitEnable != 0U) &&
                ((UNIPD_bbcBootstrapVdcReferenceLimit_Volts + 0.001f) <
                 UNIPD_bbcSyntheticInputs.v_dc_pbat_rif);
        bbcCurrentLimited =
                (UNIPD_bbcBootstrapCurrentLimitEnable != 0U) &&
                ((UNIPD_bbcBootstrapCurrentLimit_Amps + 0.001f) <
                 UNIPD_bbcSyntheticInputs.i_bat_rif_max);
        bbcDutyLimited =
                (UNIPD_bbcBootstrapDutyLimitEnable != 0U) &&
                ((UNIPD_bbcBootstrapDutyLimit_pu + 0.0001f) <
                 UNIPD_bbcPowerOutputDutyMax_pu);
        bootstrapLimited =
                (WLESS_SM_state != WLESS_SM_STATE_SOURCE_ON) ||
                bbcVdcLimited ||
                bbcCurrentLimited ||
                bbcDutyLimited ||
                (WLESS_SM_bootHfcHandoffActive != 0U);
    }

    if(sourceRun && bootstrapLimited)
    {
        /* 2 Hz: visibile a occhio e privo di qualsiasi impatto sul tick PWM. */
        ledState = ((WLESS_SM_tickCounter / 250UL) & 1UL) ? 1U : 0U;
    }
    else if(sourceRun)
    {
        ledState = 1U;
    }

    /* LED della controlCARD attivo basso: il segnale logico resta espresso
     * nella semantica descritta sopra, l'inversione e' soltanto di I/O. */
    GPIO_writePin(CLLLC_GPIO_LED1, (ledState == 0U) ? 1U : 0U);
}
#endif

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

static bool WLESS_SM_confirmBootSourceVBusReady(void)
{
    if(WLESS_SM_vBus_V <= WLESS_SM_bootSourceVBusMin_V)
    {
        WLESS_SM_bootSourceReadyTicks = 0U;
        return false;
    }

    if(WLESS_SM_bootSourceReadyTicks <
            WLESS_SM_BOOT_SOURCE_READY_CONFIRM_TICKS)
    {
        WLESS_SM_bootSourceReadyTicks++;
    }

    return WLESS_SM_bootSourceReadyTicks >=
            WLESS_SM_BOOT_SOURCE_READY_CONFIRM_TICKS;
}

static bool WLESS_SM_confirmBootLoadVBusReady(void)
{
    return WLESS_SM_updateFilteredCondition(&WLESS_SM_bootLoadVBusFilter,
            WLESS_SM_vBus_V > WLESS_SM_bootLoadVBusMin_V, false, false);
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
 * Integrazione prospettica FSM -> controllo di potenza UniPD.
 *
 * IMPORTANTE:
 * - questo codice e' escluso dal firmware corrente finche'
 *   WLESS_SM_POWER_CONTROL_ENABLE resta a 0;
 * - intenzionalmente non sovrascrive riferimenti, clamp o parametri di rampa
 *   configurabili tramite UART;
 * - ogni TODO DI INTEGRAZIONE seguente deve essere riesaminato prima
 *   dell'abilitazione hardware.
 */
static void WLESS_SM_disableUnipdPowerPath(void)
{
    /*
     * Arrestare prima gli attuatori attivi, quindi azzerare lo stato
     * dell'integrazione. UNIPD_disableWptHfcActuator() forza anche un trip OST
     * sul PWM HFC.
     */
    UNIPD_bbcPowerOutputEnable = 0U;
    UNIPD_bbcBootstrapCurrentLimitEnable = 0U;
    UNIPD_bbcBootstrapVdcReferenceLimitEnable = 0U;
    UNIPD_bbcBootstrapDutyLimitEnable = 0U;
    TTPLPFC_bbcDockTestEnable = 0;
    TTPLPFC_BBC_disable();
    UNIPD_disableWptHfcActuator();
    UNIPD_wptIntegrationEnable = 0U;
    UNIPD_resetControlStatesCommand = 1U;
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
    WLESS_SM_bootHfcHandoffActive = 0U;
#endif

    /*
     * TODO DI INTEGRAZIONE: validare se qui serva un comando attivo di scarica
     * del DCLINK. Attualmente OFF interrompe la conversione di potenza, ma non
     * garantisce la scarica controllata dei condensatori del DC-link.
     */
}

static void WLESS_SM_prepareUnipdBbcPath(bool enableBbc)
{
    /*
     * Il percorso di docking test ha precedenza nell'ISR sul percorso BBC
     * UniPD e deve quindi essere disabilitato prima di assegnare autorita' a UniPD.
     */
    TTPLPFC_bbcDockTestEnable = 0;
    TTPLPFC_bbcDockTestLegMode = TTPLPFC_BBC_DOCK_TEST_LEG_DISABLED;
    TTPLPFC_BBC_disable();

    UNIPD_disableWptHfcActuator();
    UNIPD_wptIntegrationEnable = 1U;
    UNIPD_resetControlStatesCommand = 1U;
    WLESS_SM_bootBbcControlTicks = 0U;
    WLESS_SM_bootBbcDutyArmTicks = 0U;
    WLESS_SM_bootBbcTargetReadyTicks = 0U;
    /*
     * Nel bootstrap HFC+BUCK con PSU esterna il SOURCE non deve mai accendere
     * il BOOST. Il percorso UniPD/WPT resta invece attivo per HFC e radio.
     */
    if(enableBbc)
    {
        /*
         * Precharge BOOST a richiesta energetica limitata. Diversamente il
         * controllore, vedendo VDC~0 e UR=24 V, richiede subito UA completo e
         * puo' superare il trip dei due rami prima che la rampa duty abbia
         * effetto misurabile. Il rilascio prosegue gradualmente anche dopo
         * l'ingresso nel bootstrap HFC, con DCLINK gia' qualificato sopra
         * SMBOOTSRC.
         */
        UNIPD_bbcBootstrapCurrentLimit_Amps =
                WLESS_SM_BOOT_BBC_PRECHARGE_CURRENT_A;
        UNIPD_bbcBootstrapCurrentLimitEnable = 1U;
        UNIPD_bbcBootstrapVdcReferenceLimitEnable = 0U;
        UNIPD_bbcBootstrapDutyLimit_pu =
                ((float)WLESS_SM_bootBbcDutyMax_mpu) * 0.001f;
        UNIPD_bbcBootstrapDutyLimitEnable = 1U;
        /* Il consenso PWM e' dato solo da serviceBootstrapFastGuards() dopo
         * la presenza fisica di VIN, non all'ingresso nello stato FSM. */
        UNIPD_bbcPowerOutputEnable = 0U;
    }
    else
    {
        UNIPD_bbcBootstrapCurrentLimitEnable = 0U;
        UNIPD_bbcBootstrapDutyLimitEnable = 0U;
        UNIPD_bbcPowerOutputEnable = 0U;
    }

    /*
     * L'ISR seleziona BOOST o BUCK dall'ingresso UniPD tx_1_rx_0, derivato da
     * WLESS_SM_localRole. Non selezionare qui la modalita' hardware: deve
     * esistere una sola autorita' per la mappatura ruolo -> modalita' convertitore.
     *
     * TODO DI INTEGRAZIONE: definire la strategia di cancellazione dei fault
     * VDC/IL latched. Intenzionalmente non vengono cancellati in automatico
     * durante una transizione di stato.
     *
     * TODO DI INTEGRAZIONE: validare riferimenti, limiti di corrente, clamp del
     * duty e parametri di rampa prima di impostare UNIPD_bbcPowerOutputEnable.
     */
}

static void WLESS_SM_enableUnipdSourceHfc(bool bootstrap)
{
    /*
     * Riprodurre la preparazione automatica dell'attuatore HFC UniPD gia'
     * verificata, lasciando pero' il calcolo del phase-shift a
     * UNIPD_runTransferredPowerIntegration().
     */
    UNIPD_wptHfcActuatorFault = 0U;
    /* Il rilascio di UA resta graduale e prosegue anche in HFC_BOOT/SOURCE_ON. */

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
    /*
     * In bootstrap il phase target e' quello preconfigurato con WPTHFCPH.
     * Il normale controllore di potenza continua a raccogliere le misure ma
     * non puo' alterare la fase fisica fino al CTRL_WPTREADY remoto.
     */
    if(bootstrap)
    {
        /*
         * WPTHFCPH resta il target di bootstrap, non il primo comando fisico.
         * Partire a 500 mpu evita il gradino 500 -> target che ha portato il
         * tank oltre 5,5 A prima che il LOAD potesse pubblicare WPTREADY.
         */
        WLESS_SM_bootHfcTargetPhase_pu = UNIPD_wptHfcManualPhase_pu;
        if(WLESS_SM_bootHfcTargetPhase_pu < 0.0f)
        {
            WLESS_SM_bootHfcTargetPhase_pu = 0.0f;
        }
        else if(WLESS_SM_bootHfcTargetPhase_pu > 0.5f)
        {
            WLESS_SM_bootHfcTargetPhase_pu = 0.5f;
        }
        UNIPD_wptHfcManualPhase_pu =
                ((float)WLESS_SM_BOOT_HFC_PHASE_START_MPU) * 0.001f;
        WLESS_SM_bootHfcSoftCurrentTrip = 0U;
        WLESS_SM_bootHfcRampTicks = 0U;
        WLESS_SM_bootHfcConfiguredPhaseMax_pu = UNIPD_wptHfcPhaseMax_pu;
        WLESS_SM_bootHfcHandoffActive = 0U;
        WLESS_SM_bootHfcHandoffPhase_mpu = WLESS_SM_BOOT_HFC_PHASE_START_MPU;
        WLESS_SM_bootHfcHandoffLimit_mpu =
                (uint16_t)(UNIPD_wptHfcPhaseMax_pu * 1000.0f);
    }
    UNIPD_wptHfcManualPhaseEnable = bootstrap ? 1U : 0U;
    UNIPD_wptHfcActuatorEnable = 1U;

    /*
     * TODO DI INTEGRAZIONE: prima di abilitare questo percorso sull'hardware, validare:
     * - ruolo locale SOURCE, ruolo remoto LOAD e link radio;
     * - disponibilita'/validita' di ITANK_MOD locale e remoto;
     * - strategia della corrente sintetica SOURCE finche' il relativo canale
     *   fisico non e' validato;
     * - sequenza BOOST-ready e condizione minima del DCLINK;
     * - strategia di cancellazione del trip HFC.
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
         * Precharge SOURCE: eseguire il percorso BBC UniPD in modalita' BOOST
         * mantenendo HFC in trip. La FSM richiedera' SOURCE_ON solo dopo il
         * raggiungimento della propria condizione VBUS-ready.
         */
        WLESS_SM_prepareUnipdBbcPath(
                WLESS_SM_bootSourceMode == WLESS_SM_BOOT_SOURCE_BOOST);
    }
    else if(command == WLESS_SM_POWER_CMD_SOURCE_HFC_BOOT)
    {
        WLESS_SM_enableUnipdSourceHfc(true);
    }
    else if(command == WLESS_SM_POWER_CMD_SOURCE_ON)
    {
        /*
         * Mantenere attivo l'anello BOOST e aggiungere l'attuatore HFC
         * trasmittente. Non resettare qui il controllore BBC: ha gia'
         * raggiunto il punto operativo di precharge. In particolare, il
         * passaggio HFC_BOOT_SOURCE -> SOURCE_ON segue
         * WLESS_SM_startUnipdHfcHandoff(), che ha appena inizializzato la
         * fase UniPD al punto fisico corrente: un reset in questo punto
         * reintrodurrebbe un gradino nel comando e annullerebbe l'hand-off.
         */
        /* Il cap bootstrap resta attivo oltre WPTON e viene rilasciato dalla
         * guardia a passi discreti; UD resta il limite ordinario immutato. */
        WLESS_SM_enableUnipdSourceHfc(false);
    }
    else if(command == WLESS_SM_POWER_CMD_LOAD_PRECHARGE)
    {
        /*
         * Precharge LOAD: UniPD seleziona BUCK in base al ruolo locale. Il ponte
         * HFC resta disabilitato poiche' il rectifier LOAD e' passivo.
         */
        /* Il rectifier resta passivo: il BUCK non puo' partire in precharge. */
        WLESS_SM_prepareUnipdBbcPath(false);
    }
    else if(command == WLESS_SM_POWER_CMD_LOAD_ON)
    {
        /*
         * Mantenere attivi l'anello BUCK UniPD e il calcolo della potenza
         * trasferita. Sono stati abilitati da LOAD_PRECHARGE; non viene emesso
         * alcun comando attivo per il ponte ricevente.
         */
        UNIPD_disableWptHfcActuator();
        UNIPD_resetControlStatesCommand = 1U;
        UNIPD_bbcPowerOutputEnable = 1U;
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
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
    /* Il GPIO e' configurato dal setup CLLLC; LED controlCARD attivo basso. */
    GPIO_writePin(CLLLC_GPIO_LED1, 1U);
#endif

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
    WLESS_SM_bootSourceMode = WLESS_SM_BOOT_SOURCE_EXTERNAL;
    WLESS_SM_localEnergyEncoded = 0;
    WLESS_SM_remoteEnergyEncoded = 0;
    WLESS_SM_vBus_V = 0U;
    WLESS_SM_iBat_mA = 0U;
    WLESS_SM_iCoil_mA = 0U;
    WLESS_SM_bootSourceVBusMin_V = WLESS_SM_BOOT_SOURCE_VBUS_MIN_V;
    WLESS_SM_bootLoadVBusMin_V = WLESS_SM_BOOT_LOAD_VBUS_MIN_V;
    WLESS_SM_bootBbcDutyMax_mpu = WLESS_SM_BOOT_BBC_DUTY_MAX_MPU;
    WLESS_SM_bootstrapStepPending = 0U;
    WLESS_SM_bootHfcSoftCurrentTrip = 0U;
    WLESS_SM_bootLoadOverVoltage = 0U;
    WLESS_SM_bootSourceReadyTicks = 0U;
    WLESS_SM_bootHfcRampTicks = 0U;
    WLESS_SM_bootHfcTargetPhase_pu = 0.5f;
    WLESS_SM_bootHfcHandoffRampTicks = 0U;
    WLESS_SM_bootHfcConfiguredPhaseMax_pu = UNIPD_wptHfcPhaseMax_pu;
    WLESS_SM_bootHfcHandoffActive = 0U;
    WLESS_SM_bootHfcHandoffPhase_mpu = 0U;
    WLESS_SM_bootHfcHandoffLimit_mpu = 0U;
    WLESS_SM_bootTraceState = (uint16_t)WLESS_SM_STATE_STANDBY;
    WLESS_SM_bootTraceNextState = (uint16_t)WLESS_SM_STATE_STANDBY;
    WLESS_SM_bootTraceVBus_V = 0U;
    WLESS_SM_bootTraceRemoteCtrl = (uint16_t)WLESS_SM_CTRL_IDLE;
    WLESS_SM_bootTracePhase_mpu = WLESS_SM_BOOT_HFC_PHASE_START_MPU;
    WLESS_SM_bootTraceReason = WLESS_SM_BOOT_TRACE_NONE;
    WLESS_SM_powerToLoad = 0;
    WLESS_SM_remotePowerToLoad = 0;
    WLESS_SM_iCoilErr = 0;
    WLESS_SM_remoteICoilErr = 0;
    WLESS_SM_noAckCount = 0U;
    WLESS_SM_noAckMaxCount = 0U;
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
    WLESS_SM_resetFilter(&WLESS_SM_bootLoadVBusFilter);
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

#if WLESS_SM_POWER_CONTROL_ENABLE == 1
    if(WLESS_SM_isBootstrapState(WLESS_SM_state) ||
       ((WLESS_SM_state == WLESS_SM_STATE_SOURCE_ON) &&
        (WLESS_SM_bootSourceMode == WLESS_SM_BOOT_SOURCE_BOOST) &&
        ((UNIPD_bbcBootstrapVdcReferenceLimitEnable != 0U) ||
         (UNIPD_bbcBootstrapCurrentLimitEnable != 0U) ||
         /* Il duty-cap ha una propria rampa di rilascio. Se VDC e corrente
          * sono gia' liberi, deve comunque continuare a essere servita. */
         (UNIPD_bbcBootstrapDutyLimitEnable != 0U))))
    {
        WLESS_SM_bootstrapStepPending = 1U;
        WLESS_SM_serviceBootstrapFastGuards();
    }
    WLESS_SM_serviceHfcHandoffLimit();
    WLESS_SM_serviceBootstrapLed();
#endif

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
        /* Azzerato dal driver esclusivamente dopo un payload valido. */
        if(WLESS_SM_noAckCount < 15U)
        {
            WLESS_SM_noAckCount++;
            if(WLESS_SM_noAckCount > WLESS_SM_noAckMaxCount)
            {
                WLESS_SM_noAckMaxCount = WLESS_SM_noAckCount;
            }
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
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                /* INITOK negozia la sessione; il bootstrap non e' ancora RUN. */
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_INITOK;
#else
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
#endif
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_bootSourceReadyTicks = 0U;
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
            else if(WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF)
            {
                /*
                 * Il SOURCE puo' annullare il run prima di pubblicare
                 * WPTBOOT. Il LOAD era gia' in WAIT_SOURCE: deve comunque
                 * completare il protocollo di spegnimento e pubblicare
                 * WPTEND, altrimenti il SOURCE resta in SOURCE_END.
                 */
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_LOAD;
            }
            else if(WLESS_SM_remoteCtrlState ==
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                    WLESS_SM_CTRL_WPTBOOT)
#else
                    WLESS_SM_CTRL_WPTON)
#endif
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_LOAD;
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTBOOT;
#else
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
#endif
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_PRECHARGE);
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                WLESS_SM_bootLoadOverVoltage = 0U;
                WLESS_SM_state = WLESS_SM_STATE_WAIT_VBUS_LOAD;
#else
                WLESS_SM_state = WLESS_SM_STATE_PRECHARGE_LOAD;
#endif
            }
            break;

        case WLESS_SM_STATE_PRECHARGE_SOURCE:
            if(WLESS_SM_radioLinkKo())
            {
                WLESS_SM_bootTraceReason = WLESS_SM_BOOT_TRACE_RADIO;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if(WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP)
            {
                WLESS_SM_bootTraceReason = WLESS_SM_BOOT_TRACE_WPT_DECISION;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_powerToLoad = 0;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if(
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                    WLESS_SM_confirmSourceBootstrapReady())
#else
                    WLESS_SM_confirmVBusReady())
#endif
            {
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTBOOT;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_HFC_BOOT);
                WLESS_SM_state = WLESS_SM_STATE_HFC_BOOT_SOURCE;
#else
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_iCoilErr = 0;
                WLESS_SM_localAbort = WLESS_SM_ABORT_DISABLED;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_ON);
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_ON;
#endif
            }
            break;

        case WLESS_SM_STATE_HFC_BOOT_SOURCE:
            if(WLESS_SM_radioLinkKo())
            {
                WLESS_SM_bootTraceReason = WLESS_SM_BOOT_TRACE_RADIO;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if(WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP)
            {
                WLESS_SM_bootTraceReason = WLESS_SM_BOOT_TRACE_WPT_DECISION;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if(WLESS_SM_remoteAbort == WLESS_SM_ABORT_ENABLED)
            {
                WLESS_SM_bootTraceReason = WLESS_SM_BOOT_TRACE_REMOTE_ABORT;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if(WLESS_SM_bootHfcSoftCurrentTrip != 0U)
            {
                WLESS_SM_bootTraceReason = WLESS_SM_BOOT_TRACE_SOFT_COIL;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if((WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTREADY) &&
                    WLESS_SM_confirmBootSourceVBusReady())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_startUnipdHfcHandoff();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_ON);
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_ON;
            }
            break;

        case WLESS_SM_STATE_WAIT_VBUS_LOAD:
            if((WLESS_SM_bootLoadOverVoltage != 0U) ||
                WLESS_SM_radioLinkKo() ||
                (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_LOAD;
            }
            else if(WLESS_SM_confirmBootLoadVBusReady())
            {
                /* Il LOAD e' l'unica autorita' sulla propria VDC fisica. */
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_ON);
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTREADY;
                WLESS_SM_state = WLESS_SM_STATE_WAIT_RUN_LOAD;
            }
            if(WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP)
            {
                /* In AUTO il LOAD propaga lo stop al SOURCE durante il boot. */
                WLESS_SM_localAbort = WLESS_SM_ABORT_ENABLED;
            }
            break;

        case WLESS_SM_STATE_WAIT_RUN_LOAD:
            if((WLESS_SM_bootLoadOverVoltage != 0U) ||
                WLESS_SM_radioLinkKo() ||
                (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_LOAD;
            }
            else if(WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTON)
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_state = WLESS_SM_STATE_LOAD_ON;
            }
            if(WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP)
            {
                WLESS_SM_localAbort = WLESS_SM_ABORT_ENABLED;
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
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_INITOK;
#else
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
#endif
                WLESS_SM_resetLocalExchange();
                WLESS_SM_bootSourceReadyTicks = 0U;
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
            else if(WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF)
            {
                /*
                 * Chiusura ordinata di un run annullato dal SOURCE prima di
                 * WPTBOOT: il WPTEND del LOAD sblocca SOURCE_END remoto.
                 */
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_LOAD;
            }
            else if(WLESS_SM_remoteCtrlState ==
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                    WLESS_SM_CTRL_WPTBOOT)
#else
                    WLESS_SM_CTRL_WPTON)
#endif
            {
                WLESS_SM_localRole = WLESS_SM_ROLE_LOAD;
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTBOOT;
#else
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
#endif
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_PRECHARGE);
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                WLESS_SM_bootLoadOverVoltage = 0U;
                WLESS_SM_state = WLESS_SM_STATE_WAIT_VBUS_LOAD;
#else
                WLESS_SM_state = WLESS_SM_STATE_PRECHARGE_LOAD;
#endif
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
            else if(
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                    WLESS_SM_confirmSourceBootstrapReady())
#else
                    WLESS_SM_confirmVBusReady())
#endif
            {
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTBOOT;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_HFC_BOOT);
                WLESS_SM_state = WLESS_SM_STATE_HFC_BOOT_SOURCE;
#else
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_ON);
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_ON;
#endif
            }
            break;

        case WLESS_SM_STATE_HFC_BOOT_SOURCE:
            if(WLESS_SM_radioLinkKo() ||
                (WLESS_SM_remoteAbort == WLESS_SM_ABORT_ENABLED) ||
                (WLESS_SM_bootHfcSoftCurrentTrip != 0U))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if((WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTREADY) &&
                    WLESS_SM_confirmBootSourceVBusReady())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_startUnipdHfcHandoff();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_ON);
                WLESS_SM_state = WLESS_SM_STATE_SOURCE_ON;
            }
            break;

        case WLESS_SM_STATE_WAIT_VBUS_LOAD:
            if((WLESS_SM_bootLoadOverVoltage != 0U) ||
                WLESS_SM_radioLinkKo() ||
                (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_LOAD;
            }
            else if(WLESS_SM_confirmBootLoadVBusReady())
            {
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_ON);
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTREADY;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_state = WLESS_SM_STATE_WAIT_RUN_LOAD;
            }
            break;

        case WLESS_SM_STATE_WAIT_RUN_LOAD:
            if((WLESS_SM_bootLoadOverVoltage != 0U) ||
                WLESS_SM_radioLinkKo() ||
                (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_LOAD;
            }
            else if(WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTON)
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
                WLESS_SM_state = WLESS_SM_STATE_LOAD_ON;
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
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
    bool bootstrapStep = false;

    if(WLESS_SM_bootstrapStepPending != 0U)
    {
        WLESS_SM_bootstrapStepPending = 0U;
        bootstrapStep = true;
    }
#endif

    if((WLESS_SM_diagnosticMessagePending == 0U)
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
       && !bootstrapStep
#endif
      )
    {
        return;
    }

    if(WLESS_SM_diagnosticMessagePending != 0U)
    {
        WLESS_SM_diagnosticMessagePending = 0U;
    }
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

#if WLESS_SM_POWER_CONTROL_ENABLE == 1
    if(WLESS_SM_isBootstrapState(previousState))
    {
        WLESS_SM_captureBootstrapTrace(previousState);
    }
#endif

#if WLESS_SM_TEST_RUN_DISPATCH_ONLY == 1
    // Isolamento diagnostico: verificare scheduling e bookkeeping di
    // WLESS_SM_run() senza entrare nelle implementazioni FSM specifiche dei ruoli.
#elif WLESS_SM_TEST_VEHICLE_STANDBY_ONLY == 1
    WLESS_SM_runVehicleStandbyOnly();
#elif WLESS_SM_BUILD_VEHICLE == 1
    WLESS_SM_runVehicleStep();
#else
    WLESS_SM_runStationStep();
#endif

    if(WLESS_SM_state != previousState)
    {
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
        if((WLESS_SM_state == WLESS_SM_STATE_PRECHARGE_SOURCE) ||
           (WLESS_SM_state == WLESS_SM_STATE_WAIT_VBUS_LOAD))
        {
            WLESS_SM_bootTraceReason = WLESS_SM_BOOT_TRACE_NONE;
        }
        if(WLESS_SM_isBootstrapState(previousState))
        {
            WLESS_SM_bootTraceNextState = (uint16_t)WLESS_SM_state;
        }
#endif
        WLESS_SM_statusUpdatePending = 1U;
    }
}
