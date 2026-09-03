#ifndef WLESS_SM_H_
#define WLESS_SM_H_

#include <stdbool.h>
#include <stdint.h>

#include "wless_sm_settings.h"

typedef enum
{
    WLESS_SM_WPT_RUN = 0,
    WLESS_SM_WPT_STOP = 1
} WLESS_SM_WptState;

typedef enum
{
    WLESS_SM_STATE_STANDBY = 0,
    WLESS_SM_STATE_DISCOVERY,
    WLESS_SM_STATE_INIT_SOURCE,
    WLESS_SM_STATE_INIT_LOAD,
    WLESS_SM_STATE_WAIT_SOURCE,
    WLESS_SM_STATE_WAIT_LOAD,
    WLESS_SM_STATE_PRECHARGE_SOURCE,
    WLESS_SM_STATE_PRECHARGE_LOAD,
    WLESS_SM_STATE_SOURCE_ON,
    WLESS_SM_STATE_LOAD_ON,
    WLESS_SM_STATE_DISCHARGE_SOURCE,
    WLESS_SM_STATE_DISCHARGE_LOAD,
    WLESS_SM_STATE_SOURCE_OFF,
    WLESS_SM_STATE_LOAD_OFF,
    WLESS_SM_STATE_SOURCE_END,
    WLESS_SM_STATE_LOAD_END,
    WLESS_SM_STATE_ENDCHARGE_SOURCE,
    WLESS_SM_STATE_ENDCHARGE_LOAD
} WLESS_SM_State;

typedef enum
{
    WLESS_SM_ROLE_NONE = 0,
    WLESS_SM_ROLE_SOURCE = 1,
    WLESS_SM_ROLE_LOAD = 2
} WLESS_SM_Role;

typedef enum
{
    WLESS_SM_CTRL_IDLE = 0x00,
    WLESS_SM_CTRL_INIT = 0x01,
    WLESS_SM_CTRL_INITOK = 0x02,
    WLESS_SM_CTRL_WPTON = 0x03,
    WLESS_SM_CTRL_WPTOFF = 0x04,
    WLESS_SM_CTRL_WPTEND = 0x05,
    WLESS_SM_CTRL_WPTERR = 0x06
} WLESS_SM_ControllerState;

typedef enum
{
    WLESS_SM_ABORT_DISABLED = 0,
    WLESS_SM_ABORT_ENABLED = 1
} WLESS_SM_Abort;

typedef enum
{
    WLESS_SM_LINK_OK = 0,
    WLESS_SM_LINK_FAIL = 1
} WLESS_SM_RadioLink;

typedef enum
{
    WLESS_SM_MODE_AUTO = 0,
    WLESS_SM_MODE_MANUAL = 1
} WLESS_SM_OpMode;

typedef enum
{
    WLESS_SM_POWER_CMD_OFF = 0,
    WLESS_SM_POWER_CMD_SOURCE_PRECHARGE,
    WLESS_SM_POWER_CMD_SOURCE_ON,
    WLESS_SM_POWER_CMD_LOAD_PRECHARGE,
    WLESS_SM_POWER_CMD_LOAD_ON
} WLESS_SM_PowerCommand;

extern volatile WLESS_SM_State WLESS_SM_state;
extern volatile WLESS_SM_Role WLESS_SM_localRole;
extern volatile WLESS_SM_Role WLESS_SM_remoteRole;
extern volatile WLESS_SM_ControllerState WLESS_SM_localCtrlState;
extern volatile WLESS_SM_ControllerState WLESS_SM_remoteCtrlState;
extern volatile WLESS_SM_Abort WLESS_SM_localAbort;
extern volatile WLESS_SM_Abort WLESS_SM_remoteAbort;
extern volatile WLESS_SM_RadioLink WLESS_SM_radioLink;
extern volatile WLESS_SM_OpMode WLESS_SM_opMode;
extern volatile WLESS_SM_WptState WLESS_SM_wptState;
extern volatile WLESS_SM_PowerCommand WLESS_SM_powerCommand;

extern volatile int16_t WLESS_SM_localEnergyEncoded;
extern volatile int16_t WLESS_SM_remoteEnergyEncoded;
extern volatile uint16_t WLESS_SM_vBus_V;
extern volatile uint16_t WLESS_SM_iBat_mA;
extern volatile uint16_t WLESS_SM_iCoil_mA;
extern volatile uint16_t WLESS_SM_vBusMin_V;
extern volatile uint16_t WLESS_SM_iBatMin_mA;
extern volatile uint16_t WLESS_SM_iCoilMin_mA;
extern volatile int16_t WLESS_SM_powerToLoad;
extern volatile int16_t WLESS_SM_remotePowerToLoad;
extern volatile int16_t WLESS_SM_iCoilErr;
extern volatile int16_t WLESS_SM_remoteICoilErr;
extern volatile uint16_t WLESS_SM_noAckCount;
extern volatile uint16_t WLESS_SM_noAckMaxCount;

extern volatile uint16_t WLESS_SM_initOkCommand;
extern volatile uint16_t WLESS_SM_stopCommand;
extern volatile uint16_t WLESS_SM_diagnosticMessagePending;
extern volatile uint16_t WLESS_SM_operationMessagePending;
extern volatile uint16_t WLESS_SM_statusUpdatePending;
extern volatile uint16_t WLESS_SM_pendingTicks;
extern volatile uint16_t WLESS_SM_pendingTicksMax;
extern volatile uint16_t WLESS_SM_ticksDropped;
extern volatile uint32_t WLESS_SM_sourceTickAccumulator;
extern volatile uint32_t WLESS_SM_tickCounter;
extern volatile uint32_t WLESS_SM_stateStepCounter;

void WLESS_SM_init(void);
void WLESS_SM_onSourceTickIsr(void);
void WLESS_SM_servicePendingTicks(void);
void WLESS_SM_runBackgroundTick(void);
void WLESS_SM_run(void);
int16_t WLESS_SM_encodeCapacityWh(int32_t capacityWh);
int32_t WLESS_SM_decodeCapacityWh(int16_t encodedCapacity);

#if WLESS_SM_ENABLE == 0
#define WLESS_SM_INIT_IF_ENABLED()          ((void)0)
#define WLESS_SM_SOURCE_TICK_ISR_IF_ENABLED() ((void)0)
#define WLESS_SM_RUN_TICK_IF_ENABLED()      ((void)0)
#define WLESS_SM_RUN_IF_ENABLED()           ((void)0)
#elif WLESS_SM_TEST_INIT_ONLY == 1
#define WLESS_SM_INIT_IF_ENABLED()          WLESS_SM_init()
#define WLESS_SM_SOURCE_TICK_ISR_IF_ENABLED() ((void)0)
#define WLESS_SM_RUN_TICK_IF_ENABLED()      ((void)0)
#define WLESS_SM_RUN_IF_ENABLED()           ((void)0)
#elif WLESS_SM_TEST_ISR_TICK_ONLY == 1
#define WLESS_SM_INIT_IF_ENABLED()          WLESS_SM_init()
#define WLESS_SM_SOURCE_TICK_ISR_IF_ENABLED() WLESS_SM_onSourceTickIsr()
#define WLESS_SM_RUN_TICK_IF_ENABLED()      ((void)0)
#define WLESS_SM_RUN_IF_ENABLED()           ((void)0)
#elif WLESS_SM_TEST_TICK_SERVICE_ONLY == 1
#define WLESS_SM_INIT_IF_ENABLED()          WLESS_SM_init()
#define WLESS_SM_SOURCE_TICK_ISR_IF_ENABLED() WLESS_SM_onSourceTickIsr()
#define WLESS_SM_RUN_TICK_IF_ENABLED()      WLESS_SM_servicePendingTicks()
#define WLESS_SM_RUN_IF_ENABLED()           ((void)0)
#else
#define WLESS_SM_INIT_IF_ENABLED()          WLESS_SM_init()
#define WLESS_SM_SOURCE_TICK_ISR_IF_ENABLED() WLESS_SM_onSourceTickIsr()
#define WLESS_SM_RUN_TICK_IF_ENABLED()      WLESS_SM_servicePendingTicks()
#define WLESS_SM_RUN_IF_ENABLED()           WLESS_SM_run()
#endif

#endif
