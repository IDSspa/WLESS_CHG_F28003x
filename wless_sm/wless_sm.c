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
#pragma RETAIN(WLESS_SM_bootSourceVBusHoldMin_V)
#pragma RETAIN(WLESS_SM_bootLoadVBusMin_V)
#pragma RETAIN(WLESS_SM_bootBbcArmVBusMin_V)
#pragma RETAIN(WLESS_SM_bootBbcDutyMax_mpu)
#pragma RETAIN(WLESS_SM_bootBbcHfcDutyMax_mpu)
#pragma RETAIN(WLESS_SM_bootBbcHfcHandoffDutyMax_mpu)
#pragma RETAIN(WLESS_SM_bootHfcTargetPhase_mpu)
#pragma RETAIN(WLESS_SM_bootHfcPhaseLimit_mpu)
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
volatile uint16_t WLESS_SM_bootSourceVBusHoldMin_V;
volatile uint16_t WLESS_SM_bootLoadVBusMin_V =
        WLESS_SM_BOOT_LOAD_VBUS_MIN_V;
volatile uint16_t WLESS_SM_bootBbcArmVBusMin_V =
        WLESS_SM_BOOT_BBC_ARM_VBUS_MIN_V;
volatile uint16_t WLESS_SM_bootBbcDutyMax_mpu =
        WLESS_SM_BOOT_BBC_DUTY_MAX_MPU;
volatile uint16_t WLESS_SM_bootBbcHfcDutyMax_mpu =
        WLESS_SM_BOOT_BBC_DUTY_MAX_MPU;
volatile uint16_t WLESS_SM_bootBbcHfcHandoffDutyMax_mpu =
        WLESS_SM_BOOT_BBC_HFC_HANDOFF_DUTY_MAX_MPU;
volatile uint16_t WLESS_SM_bootHfcTargetPhase_mpu =
        WLESS_SM_BOOT_HFC_PHASE_TARGET_MPU;
/* Il default replica il limite UniPD storico; WPTHFCLIM lo sovrascrive. */
volatile uint16_t WLESS_SM_bootHfcPhaseLimit_mpu = 5U;
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
/* Consenso alto iniziale: dopo la conferma resta valido fino alla soglia hold. */
static uint16_t WLESS_SM_bootSourceReadyLatched;
static uint16_t WLESS_SM_bootHfcRampTicks;
static float WLESS_SM_bootHfcTargetPhase_pu;
static uint16_t WLESS_SM_bootHfcHandoffRampTicks;
static float WLESS_SM_bootHfcConfiguredPhaseMax_pu;
/* Rilascio post-handoff del cap BOOST: distinto dalla rampa di fase HFC. */
static uint16_t WLESS_SM_bootBbcDutyReleaseActive;
static uint16_t WLESS_SM_bootBbcDutyReleaseConfirmTicks;
static uint16_t WLESS_SM_bootBbcDutyReleaseRampTicks;

#if WLESS_SM_POWER_CONTROL_ENABLE == 1
static bool WLESS_SM_confirmBootSourceVBusReady(void);
static bool WLESS_SM_confirmBoostGovernorReady(void);
static bool WLESS_SM_confirmSourceBootstrapReady(void);
static void WLESS_SM_servicePostHandoffBbcDutyRelease(void);

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

/*
 * La soglia alta SMBOOTSRC abilita HFC; la soglia bassa puo' essere fissata
 * dal profilo di banco per liberare il BOOST dopo il sag introdotto da HFC
 * neutro. Il valore zero conserva la legge storica percentuale/minima.
 */
static uint16_t WLESS_SM_getBootSourceVBusHoldMin_V(void)
{
    const uint16_t configuredHold_V = WLESS_SM_bootSourceVBusHoldMin_V;

    if((configuredHold_V != 0U) &&
       (configuredHold_V < WLESS_SM_bootSourceVBusMin_V))
    {
        return configuredHold_V;
    }

    {
        const uint16_t sourceHoldHyst_V =
                WLESS_SM_getBootSourceVBusHoldHyst_V();

        return (WLESS_SM_bootSourceVBusMin_V > sourceHoldHyst_V) ?
                (WLESS_SM_bootSourceVBusMin_V - sourceHoldHyst_V) : 0U;
    }
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
           (state == WLESS_SM_STATE_HFC_HANDOFF_SOURCE) ||
           (state == WLESS_SM_STATE_WAIT_VBUS_LOAD) ||
           (state == WLESS_SM_STATE_WAIT_RUN_LOAD);
}

static bool WLESS_SM_confirmBoostGovernorReady(void)
{
    /*
     * Il consenso HFC e' una soglia fisica di VDC SOURCE (SMBOOTSRC), non il
     * raggiungimento del riferimento finale BBC. Il cap duty resta attivo
     * fino al primo trasferimento HFC, quando esiste un carico reale.
     */
    return WLESS_SM_confirmBootSourceVBusReady();
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
       (WLESS_SM_vBus_V > WLESS_SM_bootBbcArmVBusMin_V))
    {
        /*
         * Il VIN del SOURCE deve essere realmente presente prima di liberare
         * il PWM BOOST. In precedenza il consenso veniva dato quando la FSM
         * entrava in PRECHARGE_SOURCE, quindi anche a VIN OFF: i transitori
         * del ponte potevano latching il trip di corrente prima del run.
         *
         * Il DCLINK viene osservato con l'acquisizione fisica, prima di ogni
         * override UniPD. La soglia SMBOOTARM e' un consenso di presenza VIN,
         * non un riferimento di regolazione: deve restare sopra il rumore a
         * VIN assente e sotto il VDC passivo del profilo di banco configurato.
         * Il limite energetico iniziale e' gia' predisposto separatamente.
         */
        UNIPD_resetControlStatesCommand = 1U;
        /*
         * Il precharge BBC e' governato dal solo cap duty. Un cap VDC o
         * corrente sovrapposto altera il regolatore UniPD e, a DCLINK aperto,
         * puo' creare un rilascio temporizzato non correlato al carico HFC.
         */
        UNIPD_bbcBootstrapVdcReferenceLimitEnable = 0U;
        UNIPD_bbcBootstrapCurrentLimitEnable = 0U;
        UNIPD_bbcBootstrapDutyLimit_pu =
                ((float)WLESS_SM_bootBbcDutyMax_mpu) * 0.001f;
        UNIPD_bbcBootstrapDutyLimitEnable = 1U;
        UNIPD_bbcPowerOutputEnable = 1U;
    }

    /* La parte specifica della rampa HFC resta confinata a HFC_BOOT. */
    if((WLESS_SM_state == WLESS_SM_STATE_PRECHARGE_SOURCE) ||
       (WLESS_SM_state == WLESS_SM_STATE_HFC_BOOT_SOURCE) ||
       (WLESS_SM_state == WLESS_SM_STATE_SOURCE_ON))
    {
        const uint16_t sourceHoldMin_V =
                WLESS_SM_getBootSourceVBusHoldMin_V();

        if(WLESS_SM_state == WLESS_SM_STATE_HFC_BOOT_SOURCE)
        {
            /*
             * WPTBOOT conferma che il bridge HFC neutro e' ormai parte del
             * circuito. Da qui al WPTLOADREADY del LOAD il BOOST non passa mai
             * dall'esiguo cap di precharge al duty ordinario in un solo tick:
             * usa invece il secondo ceiling configurabile SMBHD. Il controllo
             * UniPD continua a calcolare normalmente il suo comando, ma il
             * duty fisico resta limitato per contenere l'energia nel DCLINK.
             */
            if(WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTBOOT)
            {
                UNIPD_bbcBootstrapDutyLimit_pu =
                        ((float)WLESS_SM_bootBbcHfcDutyMax_mpu) * 0.001f;
                UNIPD_bbcBootstrapDutyLimitEnable = 1U;
            }

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
            /*
             * Il primo WPTBOOT ricevuto dal LOAD conferma che HFC neutro ha
             * introdotto il carico iniziale. Sopra la soglia bassa il BOOST
             * deve recuperare autorita' anche se VDC e' temporaneamente sotto
             * SMBOOTSRC; la rampa HFC resta pero' ferma finche' non e' tornato
             * sopra la soglia alta.
             */
            else if((WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTBOOT) &&
                    (WLESS_SM_vBus_V > sourceHoldMin_V) &&
                    (UNIPD_wptHfcManualPhase_pu > WLESS_SM_bootHfcTargetPhase_pu))
            {
                if(WLESS_SM_vBus_V <= WLESS_SM_bootSourceVBusMin_V)
                {
                    WLESS_SM_bootSourceReadyTicks = 0U;
                    WLESS_SM_bootHfcRampTicks = 0U;
                }
                else
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
            else if(WLESS_SM_vBus_V <= WLESS_SM_bootSourceVBusMin_V)
            {
                /* Sopra hold ma sotto consenso: HFC resta neutro e in attesa. */
                WLESS_SM_bootSourceReadyTicks = 0U;
                WLESS_SM_bootHfcRampTicks = 0U;
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
 * neutro. Al consenso WPTLOADREADY la conversione d = 0.5 - phase fisica conserva
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
    WLESS_SM_bootBbcDutyReleaseActive = 0U;
    WLESS_SM_bootBbcDutyReleaseConfirmTicks = 0U;
    WLESS_SM_bootBbcDutyReleaseRampTicks = 0U;
}

static void WLESS_SM_serviceHfcHandoffLimit(void)
{
    const float phaseStep =
            ((float)WLESS_SM_BOOT_HFC_HANDOFF_STEP_MPU) * 0.001f;
    const uint16_t sourceHoldMin_V =
            WLESS_SM_getBootSourceVBusHoldMin_V();

    if(((WLESS_SM_state != WLESS_SM_STATE_HFC_HANDOFF_SOURCE) &&
        (WLESS_SM_state != WLESS_SM_STATE_SOURCE_ON)) ||
       (WLESS_SM_bootSourceMode != WLESS_SM_BOOT_SOURCE_BOOST) ||
       (UNIPD_wptHfcActuatorEnable == 0U))
    {
        return;
    }

    /*
     * Se il DCLINK SOURCE scende sotto la hold durante l'hand-off, non
     * richiudere l'HFC verso il neutro: il ceiling BBC segueva quella stessa
     * chiusura e lasciava il BOOST senza autorita' per ripristinare il bus.
     * Congelare invece il limite HFC al valore gia' raggiunto conserva un
     * carico noto sul BOOST; serviceHfcHandoffBbcDutyLimit() puo' quindi
     * innalzare il solo ceiling BBC fino al valore di recovery SMBHH.
     *
     * La variabile fisica bloccata qui e' l'apertura differenziale massima
     * dell'HFC, non la fase istantanea ne' il duty del BOOST.
     */
    if(WLESS_SM_vBus_V <= sourceHoldMin_V)
    {
        WLESS_SM_bootHfcHandoffActive = 1U;
        /* Il prossimo incremento HFC riparte soltanto dopo il recupero VDC. */
        WLESS_SM_bootHfcHandoffRampTicks = 0U;
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
 * Durante HFC_HANDOFF_SOURCE il BOOST non deve restare bloccato a SMBHD
 * mentre il tank aumenta gradualmente il proprio assorbimento. Normalmente il
 * ceiling BBC accompagna l'apertura HFC da SMBHD a SMBHH. Se VDC SOURCE perde
 * la hold, l'HFC viene congelato ma il ceiling BBC deve acquisire autorita'
 * di recovery: viene quindi promosso a SMBHH e non puo' piu' ridursi durante
 * lo stesso hand-off. L'ordinario UD resta escluso fino a WPTON.
 *
 * La variabile fisica limitata e' il duty BBC applicato al BOOST.
 */
static void WLESS_SM_serviceHfcHandoffBbcDutyLimit(void)
{
    const float initialPhase =
            ((float)WLESS_SM_bootHfcHandoffPhase_mpu) * 0.001f;
    const float finalPhase = WLESS_SM_bootHfcConfiguredPhaseMax_pu;
    const uint16_t sourceHoldMin_V =
            WLESS_SM_getBootSourceVBusHoldMin_V();
    float dutyMin = ((float)WLESS_SM_bootBbcHfcDutyMax_mpu) * 0.001f;
    float dutyMax =
            ((float)WLESS_SM_bootBbcHfcHandoffDutyMax_mpu) * 0.001f;
    float progress;
    float requestedDutyLimit;

    if((WLESS_SM_state != WLESS_SM_STATE_HFC_HANDOFF_SOURCE) ||
       (WLESS_SM_bootSourceMode != WLESS_SM_BOOT_SOURCE_BOOST) ||
       (UNIPD_bbcBootstrapDutyLimitEnable == 0U))
    {
        return;
    }

    if(dutyMin > dutyMax)
    {
        dutyMin = dutyMax;
    }

    if(finalPhase <= initialPhase)
    {
        progress = 1.0f;
    }
    else
    {
        progress = (UNIPD_wptHfcPhaseMax_pu - initialPhase) /
                   (finalPhase - initialPhase);
        if(progress < 0.0f)
        {
            progress = 0.0f;
        }
        else if(progress > 1.0f)
        {
            progress = 1.0f;
        }
    }

    requestedDutyLimit = dutyMin + ((dutyMax - dutyMin) * progress);

    /*
     * Recovery anti-deadlock: con VDC sotto hold la fase HFC resta congelata,
     * ma il BOOST deve poter aumentare il proprio duty fino al ceiling
     * intermedio sicuro SMBHH. Non richiudere il ceiling in seguito: farlo
     * reintrodurrebbe il deadlock quando la tensione attraversa la soglia.
     */
    if(WLESS_SM_vBus_V <= sourceHoldMin_V)
    {
        requestedDutyLimit = dutyMax;
    }
    if(requestedDutyLimit > UNIPD_bbcBootstrapDutyLimit_pu)
    {
        UNIPD_bbcBootstrapDutyLimit_pu = requestedDutyLimit;
    }
}

/*
 * WPTON certifica che il controllo distribuito e' entrato in regolazione,
 * ma non autorizza un salto istantaneo del duty BOOST da SMBHH a UD. Il salto
 * aveva portato il DCLINK oltre UY nel run con SMBHH=350. Conserviamo quindi
 * il cap dell'hand-off e lo rilasciamo solo dopo una conferma di VDC SOURCE
 * sopra SMBOOTSRC; la rampa agisce sul ceiling del duty fisicamente applicato,
 * non sul riferimento UniPD, sulle correnti o sulla fase HFC.
 */
static void WLESS_SM_servicePostHandoffBbcDutyRelease(void)
{
    float ordinaryDutyLimit = UNIPD_bbcPowerOutputDutyMax_pu;
    const float dutyStep =
            ((float)WLESS_SM_BOOT_BBC_DUTY_RELEASE_STEP_MPU) * 0.001f;

    if(ordinaryDutyLimit < 0.0f)
    {
        ordinaryDutyLimit = 0.0f;
    }
    else if(ordinaryDutyLimit > 0.95f)
    {
        ordinaryDutyLimit = 0.95f;
    }

    if((WLESS_SM_bootBbcDutyReleaseActive == 0U) ||
       (WLESS_SM_state != WLESS_SM_STATE_SOURCE_ON) ||
       (WLESS_SM_bootSourceMode != WLESS_SM_BOOT_SOURCE_BOOST) ||
       (UNIPD_bbcBootstrapDutyLimitEnable == 0U))
    {
        return;
    }

    /* Un sag sotto SMBOOTSRC non riporta indietro il cap: sospende soltanto
     * il rilascio, evitando sia il gradino sia il deadlock gia' osservato. */
    if(WLESS_SM_vBus_V < WLESS_SM_bootSourceVBusMin_V)
    {
        WLESS_SM_bootBbcDutyReleaseConfirmTicks = 0U;
        WLESS_SM_bootBbcDutyReleaseRampTicks = 0U;
        return;
    }

    if(WLESS_SM_bootBbcDutyReleaseConfirmTicks <
            WLESS_SM_BOOT_BBC_DUTY_RELEASE_CONFIRM_TICKS)
    {
        WLESS_SM_bootBbcDutyReleaseConfirmTicks++;
        return;
    }

    WLESS_SM_bootBbcDutyReleaseRampTicks++;
    if(WLESS_SM_bootBbcDutyReleaseRampTicks <
            WLESS_SM_BOOT_BBC_DUTY_RELEASE_STEP_TICKS)
    {
        return;
    }
    WLESS_SM_bootBbcDutyReleaseRampTicks = 0U;

    UNIPD_bbcBootstrapDutyLimit_pu += dutyStep;
    if(UNIPD_bbcBootstrapDutyLimit_pu >= ordinaryDutyLimit)
    {
        UNIPD_bbcBootstrapDutyLimit_pu = ordinaryDutyLimit;
        UNIPD_bbcBootstrapDutyLimitEnable = 0U;
        WLESS_SM_bootBbcDutyReleaseActive = 0U;
    }
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
         (WLESS_SM_state == WLESS_SM_STATE_HFC_HANDOFF_SOURCE) ||
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
    const uint16_t sourceHoldMin_V = WLESS_SM_getBootSourceVBusHoldMin_V();

    /*
     * Il consenso alto serve solo a dimostrare che il BOOST ha costituito il
     * bus prima di armare HFC. Dopo tale prova, HFC puo' introdurre un sag
     * fisiologico: richiedere nuovamente 500 tick alla soglia alta crea un
     * deadlock. Il consenso decade quindi soltanto sotto la soglia hold.
     */
    if(WLESS_SM_bootSourceReadyLatched != 0U)
    {
        if(WLESS_SM_vBus_V <= sourceHoldMin_V)
        {
            WLESS_SM_bootSourceReadyLatched = 0U;
            WLESS_SM_bootSourceReadyTicks = 0U;
            return false;
        }

        return true;
    }

    /*
     * VDC e' rappresentata in volt interi. SMBOOTSRC e' una tensione minima
     * valida: un campione ADC uguale alla soglia e' quindi valido; la
     * conferma di 500 tick sottostante rigetta un attraversamento transitorio.
     */
    if(WLESS_SM_vBus_V < WLESS_SM_bootSourceVBusMin_V)
    {
        WLESS_SM_bootSourceReadyTicks = 0U;
        return false;
    }

    if(WLESS_SM_bootSourceReadyTicks <
            WLESS_SM_BOOT_SOURCE_READY_CONFIRM_TICKS)
    {
        WLESS_SM_bootSourceReadyTicks++;
    }

    if(WLESS_SM_bootSourceReadyTicks >=
            WLESS_SM_BOOT_SOURCE_READY_CONFIRM_TICKS)
    {
        WLESS_SM_bootSourceReadyLatched = 1U;
        return true;
    }

    return false;
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
    WLESS_SM_bootBbcDutyReleaseActive = 0U;
    WLESS_SM_bootBbcDutyReleaseConfirmTicks = 0U;
    WLESS_SM_bootBbcDutyReleaseRampTicks = 0U;
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
    WLESS_SM_bootBbcDutyReleaseActive = 0U;
    WLESS_SM_bootBbcDutyReleaseConfirmTicks = 0U;
    WLESS_SM_bootBbcDutyReleaseRampTicks = 0U;
    /*
     * Nel bootstrap HFC+BUCK con PSU esterna il SOURCE non deve mai accendere
     * il BOOST. Il percorso UniPD/WPT resta invece attivo per HFC e radio.
     */
    if(enableBbc)
    {
        /* Solo il cap duty condiziona il precharge BOOST. */
        UNIPD_bbcBootstrapCurrentLimitEnable = 0U;
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
     * non puo' alterare la fase fisica fino al CTRL_WPTLOADREADY remoto.
     */
    if(bootstrap)
    {
        /*
         * WPTHFCPH resta il target di bootstrap, non il primo comando fisico.
         * Partire a 500 mpu evita il gradino 500 -> target che ha portato il
         * tank oltre 5,5 A prima che il LOAD potesse pubblicare WPTLOADREADY.
         */
        /*
         * Non ricavare il target dalla fase istantanea: questa funzione la
         * porta deliberatamente a 500 mpu per il precharge. In precedenza il
         * run successivo catturava quindi 500 mpu come nuovo target e l'HFC
         * restava neutro. Il target configurato sopravvive al ciclo precedente.
         */
        WLESS_SM_bootHfcTargetPhase_pu =
                ((float)WLESS_SM_bootHfcTargetPhase_mpu) * 0.001f;
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
        /*
         * UNIPD_wptHfcPhaseMax_pu e' il ceiling operativo: l'hand-off lo
         * riduce temporaneamente al valore fisico da cui riparte. Non usarlo
         * come configurazione del run successivo, altrimenti WPTHFCLIM=200
         * puo' degradare persistentemente, per esempio, a 54 mpu.
         */
        WLESS_SM_bootHfcConfiguredPhaseMax_pu =
                ((float)WLESS_SM_bootHfcPhaseLimit_mpu) * 0.001f;
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
        /*
         * WPTLOADREADY certifica soltanto VDC LOAD pronto. Conservare il cap
         * intermedio SMBHD durante l'hand-off: in questa chiamata HFC passa dal
         * punto manuale al proprio limite UniPD iniziale, quindi il carico puo'
         * essere ancora troppo piccolo per assorbire un BOOST senza cap.
         * Il cap viene rilasciato nel punto che pubblica WPTON, dopo il
         * completamento della rampa HFC.
         */
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
         * Il precharge del LOAD puo' avere gia' avviato il BUCK al consenso
         * SMBOOTLOAD. WPTON deve allora mantenere lo stato del regolatore:
         * resettarlo qui reintrodurrebbe un gradino di potenza proprio durante
         * l'hand-off HFC. Al primo armamento, invece, il reset e' necessario.
         */
        UNIPD_disableWptHfcActuator();
        if(UNIPD_bbcPowerOutputEnable == 0U)
        {
            UNIPD_resetControlStatesCommand = 1U;
        }
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
    WLESS_SM_bootSourceVBusHoldMin_V = 0U;
    WLESS_SM_bootLoadVBusMin_V = WLESS_SM_BOOT_LOAD_VBUS_MIN_V;
    WLESS_SM_bootBbcArmVBusMin_V = WLESS_SM_BOOT_BBC_ARM_VBUS_MIN_V;
    WLESS_SM_bootBbcDutyMax_mpu = WLESS_SM_BOOT_BBC_DUTY_MAX_MPU;
    WLESS_SM_bootBbcHfcDutyMax_mpu = WLESS_SM_BOOT_BBC_DUTY_MAX_MPU;
    WLESS_SM_bootBbcHfcHandoffDutyMax_mpu =
            WLESS_SM_BOOT_BBC_HFC_HANDOFF_DUTY_MAX_MPU;
    WLESS_SM_bootHfcTargetPhase_mpu = WLESS_SM_BOOT_HFC_PHASE_TARGET_MPU;
    WLESS_SM_bootHfcPhaseLimit_mpu = 5U;
    WLESS_SM_bootstrapStepPending = 0U;
    WLESS_SM_bootHfcSoftCurrentTrip = 0U;
    WLESS_SM_bootLoadOverVoltage = 0U;
    WLESS_SM_bootSourceReadyTicks = 0U;
    WLESS_SM_bootSourceReadyLatched = 0U;
    WLESS_SM_bootHfcRampTicks = 0U;
    WLESS_SM_bootHfcTargetPhase_pu = 0.5f;
    WLESS_SM_bootHfcHandoffRampTicks = 0U;
#if WLESS_SM_POWER_CONTROL_ENABLE == 1
    WLESS_SM_bootHfcConfiguredPhaseMax_pu = UNIPD_wptHfcPhaseMax_pu;
#endif
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
    WLESS_SM_serviceHfcHandoffBbcDutyLimit();
    WLESS_SM_servicePostHandoffBbcDutyRelease();
    WLESS_SM_serviceBootstrapLed();
#endif

#if WLESS_SM_BUILD_VEHICLE == 1
    operationStateActive =
            (WLESS_SM_state == WLESS_SM_STATE_HFC_HANDOFF_SOURCE) ||
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
                WLESS_SM_bootSourceReadyLatched = 0U;
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

#if WLESS_SM_POWER_CONTROL_ENABLE == 1
        case WLESS_SM_STATE_HFC_BOOT_SOURCE:
            if(WLESS_SM_radioLinkKo())
            {
                WLESS_SM_bootTraceReason = WLESS_SM_BOOT_TRACE_RADIO;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if((WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP) ||
                    (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF))
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
            else if((WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTLOADREADY) &&
                    WLESS_SM_confirmBootSourceVBusReady())
            {
                /*
                 * Il LOAD dichiara solo il proprio DCLINK pronto. Il BUCK e'
                 * ancora spento: prima completiamo il passaggio bumpless HFC
                 * e rilasciamo il BOOST, poi pubblicheremo WPTON.
                 */
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTBOOT;
                WLESS_SM_startUnipdHfcHandoff();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_ON);
                WLESS_SM_state = WLESS_SM_STATE_HFC_HANDOFF_SOURCE;
            }
            break;

        case WLESS_SM_STATE_HFC_HANDOFF_SOURCE:
            if(WLESS_SM_radioLinkKo() ||
               (WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP) ||
               (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF) ||
               (WLESS_SM_remoteAbort == WLESS_SM_ABORT_ENABLED))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if(WLESS_SM_bootHfcHandoffActive == 0U)
            {
                /*
                 * La rampa HFC ha raggiunto il limite UniPD configurato: il
                 * carico HFC e' ormai disponibile, ma il BOOST non deve
                 * riacquisire UD in un singolo tick. Conserva SMBHH in WPTON
                 * e avvia il rilascio condizionato su VDC SOURCE stabile.
                 */
                WLESS_SM_bootBbcDutyReleaseActive = 1U;
                WLESS_SM_bootBbcDutyReleaseConfirmTicks = 0U;
                WLESS_SM_bootBbcDutyReleaseRampTicks = 0U;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
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
                /*
                 * Ripristina il precharge BUCK gia' validato: al superamento
                 * di SMBOOTLOAD il carico controllato evita che il DCLINK
                 * LOAD rimanga esclusivamente capacitivo durante l'hand-off.
                 * I limiti ordinari UV/UA/UC/UD/US restano quelli configurati
                 * via UART; WPTON completera' soltanto il protocollo remoto.
                 */
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_ON);
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTLOADREADY;
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
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_ON);
                WLESS_SM_state = WLESS_SM_STATE_LOAD_ON;
            }
            if(WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP)
            {
                WLESS_SM_localAbort = WLESS_SM_ABORT_ENABLED;
            }
            break;
#endif

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
               (WLESS_SM_runWptDecision() == WLESS_SM_WPT_STOP) ||
               (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF) ||
               (WLESS_SM_remoteAbort == WLESS_SM_ABORT_ENABLED))
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
                WLESS_SM_bootSourceReadyLatched = 0U;
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

#if WLESS_SM_POWER_CONTROL_ENABLE == 1
        case WLESS_SM_STATE_HFC_BOOT_SOURCE:
            if(WLESS_SM_radioLinkKo() ||
                (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF) ||
                (WLESS_SM_remoteAbort == WLESS_SM_ABORT_ENABLED) ||
                (WLESS_SM_bootHfcSoftCurrentTrip != 0U))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if((WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTLOADREADY) &&
                    WLESS_SM_confirmBootSourceVBusReady())
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTBOOT;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_startUnipdHfcHandoff();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_SOURCE_ON);
                WLESS_SM_state = WLESS_SM_STATE_HFC_HANDOFF_SOURCE;
            }
            break;

        case WLESS_SM_STATE_HFC_HANDOFF_SOURCE:
            if(WLESS_SM_radioLinkKo() ||
               (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF) ||
               (WLESS_SM_remoteAbort == WLESS_SM_ABORT_ENABLED))
            {
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTOFF;
                WLESS_SM_resetLocalExchange();
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_OFF);
                WLESS_SM_state = WLESS_SM_STATE_ENDCHARGE_SOURCE;
            }
            else if(WLESS_SM_bootHfcHandoffActive == 0U)
            {
                /* Mantiene anche qui SMBHH e rilascia UD soltanto tramite la
                 * rampa post-handoff condizionata su VDC SOURCE stabile. */
                WLESS_SM_bootBbcDutyReleaseActive = 1U;
                WLESS_SM_bootBbcDutyReleaseConfirmTicks = 0U;
                WLESS_SM_bootBbcDutyReleaseRampTicks = 0U;
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTON;
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
                /* Stesso precharge BUCK del build VEHICLE: la soglia e' locale
                 * al LOAD, mentre WPTON rimane l'ack distribuito dell'handoff. */
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_ON);
                WLESS_SM_localCtrlState = WLESS_SM_CTRL_WPTLOADREADY;
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
                WLESS_SM_setPowerCommand(WLESS_SM_POWER_CMD_LOAD_ON);
                WLESS_SM_state = WLESS_SM_STATE_LOAD_ON;
            }
            break;
#endif

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
               (WLESS_SM_remoteCtrlState == WLESS_SM_CTRL_WPTOFF) ||
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
