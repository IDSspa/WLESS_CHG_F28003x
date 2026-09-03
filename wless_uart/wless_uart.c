#include <stddef.h>
#include <string.h>

#include "driverlib.h"
#include "device.h"
#include "ring_buffer.h"
#include "wless_uart.h"
#include "wless_sm/wless_sm.h"
#include "wless_radio/wless_nrf24.h"
#include "unipd/unipd_control.h"
#include "ttplpfc/ttplpfc.h"
#include "clllc/clllc.h"
#include "wless_config.h"

#define WLESS_UART_TOKEN_SEP       ';'

const int FIRMWARE_RELEASE = 1065;

#define WLESS_UART_KEY_WH          (1U << 0)
#define WLESS_UART_KEY_VB          (1U << 1)
#define WLESS_UART_KEY_IB          (1U << 2)
#define WLESS_UART_KEY_IC          (1U << 3)
#define WLESS_UART_KEY_SOURCE      (1U << 4)
#define WLESS_UART_KEY_LOAD        (1U << 5)
#define WLESS_UART_KEY_AUTO        (1U << 6)
#define WLESS_UART_KEY_MANUAL      (1U << 7)
#define WLESS_UART_KEY_STOP        (1U << 8)
#define WLESS_UART_KEY_INITOK      (1U << 9)
#define WLESS_UART_KEY_WH_QUERY    (1U << 10)
#define WLESS_UART_KEY_VARS_QUERY  (1U << 11)
#define WLESS_UART_KEY_RADIO_QUERY (1U << 12)
#define WLESS_UART_KEY_RADIO_PING  (1U << 13)
#define WLESS_UART_KEY_UTEST       (1U << 14)
#define WLESS_UART_KEY_UMAP        (1UL << 15)
#define WLESS_UART_KEY_UDMAX       (1UL << 16)
#define WLESS_UART_KEY_UREF        (1UL << 17)
#define WLESS_UART_KEY_UFAULT      (1UL << 18)
#define WLESS_UART_KEY_UHYBRID     (1UL << 19)
#define WLESS_UART_KEY_UENABLE     (1UL << 20)
#define WLESS_UART_KEY_UILTEST     (1UL << 21)
#define WLESS_UART_KEY_UBOOST      (1UL << 22)
#define WLESS_UART_KEY_UQUERY      (1UL << 23)
#define WLESS_UART_KEY_USTEP       (1UL << 24)
#define WLESS_UART_KEY_UIBATMAX    (1UL << 25)
#define WLESS_UART_KEY_UREF_MV     (1UL << 26)
#define WLESS_UART_KEY_UVBAT_MV    (1UL << 27)
#define WLESS_UART_KEY_UVTRIP_MV   (1UL << 28)
#define WLESS_UART_KEY_UITRIP_MA   (1UL << 29)
#define WLESS_UART_KEY_UITRIP_N    (1UL << 30)
#define WLESS_UART_KEY_UVTRIP_V    (1UL << 31)

typedef enum
{
    WLESS_UART_TYPE_VALUE,
    WLESS_UART_TYPE_FLAG,
    WLESS_UART_TYPE_QUERY
} WLESS_UART_EntryType;

typedef struct
{
    const char *key;
    uint8_t keyLen;
    uint16_t *destination;
    uint32_t id;
    WLESS_UART_EntryType type;
} WLESS_UART_ParseEntry;

typedef struct
{
    uint32_t mask;
    uint16_t tokens;
    uint16_t matches;
} WLESS_UART_ParseResult;

typedef struct
{
    uint32_t tickCounter;
    uint32_t stateStepCounter;
    uint16_t localRole;
    uint16_t state;
    uint16_t radioLink;
    uint16_t noAckCount;
    float coilPhysical_Amps;
    float coilOffset_Amps;
    float coilUsed_Amps;
    float remotePowerLimit_Watts;
    float transferredPowerRef_Watts;
    float coilCurrentRef_Amps;
    float coilCurrentErr_Amps;
} WLESS_UART_WptSnapshot;

static WLESS_RingBuffer WLESS_UART_rxBuffer;
static char WLESS_UART_commandBuffer[WLESS_UART_CMD_MAX_LEN];
static uint16_t WLESS_UART_commandIndex;
static uint16_t WLESS_UART_wh;
static uint16_t WLESS_UART_vBus;
static uint16_t WLESS_UART_iBat;
static uint16_t WLESS_UART_iCoil;
static uint16_t WLESS_UART_unipdTest;
static uint16_t WLESS_UART_unipdMap;
static uint16_t WLESS_UART_unipdDutyMaxMilli = 350U;
static uint16_t WLESS_UART_unipdVdcRef;
static uint16_t WLESS_UART_unipdVdcRefConfig = 7U;
static uint16_t WLESS_UART_unipdVdcRefMilli;
static uint32_t WLESS_UART_unipdVdcRefMilliConfig = 7000UL;
static uint16_t WLESS_UART_unipdIBatMaxMilli;
static uint16_t WLESS_UART_unipdIBatMaxMilliConfig = 200U;
static uint16_t WLESS_UART_unipdVBatMilli;
static uint16_t WLESS_UART_unipdVBatMilliConfig = 6000U;
static uint16_t WLESS_UART_unipdVdcTripMilli;
static uint16_t WLESS_UART_unipdILTripMilli;
static uint16_t WLESS_UART_unipdILTripCycles;
static uint16_t WLESS_UART_unipdVdcTripVolts;
static uint16_t WLESS_UART_unipdFaultReset;
static uint16_t WLESS_UART_unipdHybrid;
static uint16_t WLESS_UART_unipdEnable;
static uint16_t WLESS_UART_unipdILTestMilli;
static uint16_t WLESS_UART_unipdOpenLoopBoost;
static uint16_t WLESS_UART_unipdRampStepMicro;
static uint16_t WLESS_UART_unipdHybridActive;

volatile uint32_t WLESS_UART_rxByteCount;
volatile uint32_t WLESS_UART_rxErrorCount;
volatile uint32_t WLESS_UART_rxOverflowCount;
volatile uint32_t WLESS_UART_commandCount;
volatile uint32_t WLESS_UART_commandErrorCount;

static const WLESS_UART_ParseEntry WLESS_UART_parseTable[] =
{
    {"WH?",    3U, NULL,                WLESS_UART_KEY_WH_QUERY, WLESS_UART_TYPE_QUERY},
    {"VARS?",  5U, NULL,                WLESS_UART_KEY_VARS_QUERY, WLESS_UART_TYPE_QUERY},
    {"RADIO?", 6U, NULL,                WLESS_UART_KEY_RADIO_QUERY, WLESS_UART_TYPE_QUERY},
    {"RADIOPING", 9U, NULL,             WLESS_UART_KEY_RADIO_PING,  WLESS_UART_TYPE_FLAG},
    {"UT=",    3U, &WLESS_UART_unipdTest, WLESS_UART_KEY_UTEST,     WLESS_UART_TYPE_VALUE},
    {"UM=",    3U, &WLESS_UART_unipdMap, WLESS_UART_KEY_UMAP,       WLESS_UART_TYPE_VALUE},
    {"UD=",    3U, &WLESS_UART_unipdDutyMaxMilli, WLESS_UART_KEY_UDMAX, WLESS_UART_TYPE_VALUE},
    {"UR=",    3U, &WLESS_UART_unipdVdcRef, WLESS_UART_KEY_UREF,    WLESS_UART_TYPE_VALUE},
    {"UV=",    3U, &WLESS_UART_unipdVdcRefMilli, WLESS_UART_KEY_UREF_MV, WLESS_UART_TYPE_VALUE},
    {"UA=",    3U, &WLESS_UART_unipdIBatMaxMilli, WLESS_UART_KEY_UIBATMAX, WLESS_UART_TYPE_VALUE},
    {"UW=",    3U, &WLESS_UART_unipdVBatMilli, WLESS_UART_KEY_UVBAT_MV, WLESS_UART_TYPE_VALUE},
    {"UX=",    3U, &WLESS_UART_unipdVdcTripMilli, WLESS_UART_KEY_UVTRIP_MV, WLESS_UART_TYPE_VALUE},
    {"UC=",    3U, &WLESS_UART_unipdILTripMilli, WLESS_UART_KEY_UITRIP_MA, WLESS_UART_TYPE_VALUE},
    {"UN=",    3U, &WLESS_UART_unipdILTripCycles, WLESS_UART_KEY_UITRIP_N, WLESS_UART_TYPE_VALUE},
    {"UY=",    3U, &WLESS_UART_unipdVdcTripVolts, WLESS_UART_KEY_UVTRIP_V, WLESS_UART_TYPE_VALUE},
    {"UF=",    3U, &WLESS_UART_unipdFaultReset, WLESS_UART_KEY_UFAULT, WLESS_UART_TYPE_VALUE},
    {"UH=",    3U, &WLESS_UART_unipdHybrid, WLESS_UART_KEY_UHYBRID, WLESS_UART_TYPE_VALUE},
    {"UE=",    3U, &WLESS_UART_unipdEnable, WLESS_UART_KEY_UENABLE, WLESS_UART_TYPE_VALUE},
    {"UI=",    3U, &WLESS_UART_unipdILTestMilli, WLESS_UART_KEY_UILTEST, WLESS_UART_TYPE_VALUE},
    {"UB=",    3U, &WLESS_UART_unipdOpenLoopBoost, WLESS_UART_KEY_UBOOST, WLESS_UART_TYPE_VALUE},
    {"US=",    3U, &WLESS_UART_unipdRampStepMicro, WLESS_UART_KEY_USTEP, WLESS_UART_TYPE_VALUE},
    {"UQ?",    3U, NULL, WLESS_UART_KEY_UQUERY, WLESS_UART_TYPE_QUERY},
    {"WH=",    3U, &WLESS_UART_wh,      WLESS_UART_KEY_WH,       WLESS_UART_TYPE_VALUE},
    {"VB=",    3U, &WLESS_UART_vBus,    WLESS_UART_KEY_VB,       WLESS_UART_TYPE_VALUE},
    {"IB=",    3U, &WLESS_UART_iBat,    WLESS_UART_KEY_IB,       WLESS_UART_TYPE_VALUE},
    {"IC=",    3U, &WLESS_UART_iCoil,   WLESS_UART_KEY_IC,       WLESS_UART_TYPE_VALUE},
    {"SOURCE", 6U, NULL,                WLESS_UART_KEY_SOURCE,   WLESS_UART_TYPE_FLAG},
    {"LOAD",   4U, NULL,                WLESS_UART_KEY_LOAD,     WLESS_UART_TYPE_FLAG},
    {"AUTO",   4U, NULL,                WLESS_UART_KEY_AUTO,     WLESS_UART_TYPE_FLAG},
    {"MANUAL", 6U, NULL,                WLESS_UART_KEY_MANUAL,   WLESS_UART_TYPE_FLAG},
    {"STOP",   4U, NULL,                WLESS_UART_KEY_STOP,     WLESS_UART_TYPE_FLAG},
    {"INITOK", 6U, NULL,                WLESS_UART_KEY_INITOK,   WLESS_UART_TYPE_FLAG}
};

#define WLESS_UART_PARSE_ENTRIES   \
    ((uint16_t)(sizeof(WLESS_UART_parseTable) / \
                sizeof(WLESS_UART_parseTable[0])))

__interrupt void WLESS_UART_rxIsr(void);
static void WLESS_UART_sendVars(void);
static void WLESS_UART_sendRadio(void);
static void WLESS_UART_sendUnipd(void);
static void WLESS_UART_setUnipdTestVector(float vDc);
static void WLESS_UART_applyUnipdDirectionalCurrentLimit(void);
static void WLESS_UART_sendBbcCaptureStatus(void);
static void WLESS_UART_sendBbcCaptureDump(void);
static void WLESS_UART_sendWptIntegration(void);
static void WLESS_UART_sendWptSnapshot(void);
static void WLESS_UART_sendWptCaptureStatus(void);
static void WLESS_UART_sendWptCaptureDump(void);
static uint16_t WLESS_UART_isDigit(char value);

static uint16_t WLESS_UART_parseExactUnsigned(const char *command,
                                               const char *prefix,
                                               uint32_t *value)
{
    const size_t prefixLength = strlen(prefix);
    const char *p;
    uint32_t parsed = 0UL;

    if(strncmp(command, prefix, prefixLength) != 0)
    {
        return 0U;
    }

    p = command + prefixLength;
    if(!WLESS_UART_isDigit(*p))
    {
        return 0U;
    }

    while(WLESS_UART_isDigit(*p))
    {
        const uint32_t digit = (uint32_t)(*p - '0');
        if(parsed > 429496728UL)
        {
            return 0U;
        }
        parsed = (parsed * 10UL) + digit;
        p++;
    }
    if(*p != '\0')
    {
        return 0U;
    }

    *value = parsed;
    return 1U;
}

static void WLESS_UART_applyUnipdDirectionalCurrentLimit(void)
{
    const float currentLimit =
            ((float)WLESS_UART_unipdIBatMaxMilliConfig) * 0.001f;

    if(WLESS_UART_unipdHybridActive == 2U)
    {
        UNIPD_bbcSyntheticInputs.i_bat_rif_max = 0.0f;
        UNIPD_bbcSyntheticInputs.i_bat_rif_min = -currentLimit;
    }
    else
    {
        UNIPD_bbcSyntheticInputs.i_bat_rif_max = currentLimit;
        UNIPD_bbcSyntheticInputs.i_bat_rif_min = 0.0f;
    }
}

static uint16_t WLESS_UART_isDigit(char value)
{
    return (uint16_t)((value >= '0') && (value <= '9'));
}

static uint16_t WLESS_UART_isSpace(char value)
{
    return (uint16_t)((value == ' ') || (value == '\t'));
}

static void WLESS_UART_toUpper(char *str)
{
    while(*str != '\0')
    {
        if((*str >= 'a') && (*str <= 'z'))
        {
            *str = (char)(*str - ('a' - 'A'));
        }
        str++;
    }
}

static int16_t WLESS_UART_parseToken(const char *token, uint16_t length,
                                     uint32_t *mask)
{
    uint16_t i;

    for(i = 0U; i < WLESS_UART_PARSE_ENTRIES; i++)
    {
        const WLESS_UART_ParseEntry *entry = &WLESS_UART_parseTable[i];
        const char *p;
        const char *end;
        uint16_t value = 0U;

        if((length < entry->keyLen) ||
           (strncmp(token, entry->key, entry->keyLen) != 0))
        {
            continue;
        }

        if((entry->type == WLESS_UART_TYPE_FLAG) ||
           (entry->type == WLESS_UART_TYPE_QUERY))
        {
            if(length != entry->keyLen)
            {
                continue;
            }

            *mask |= entry->id;
            return 0;
        }

        p = token + entry->keyLen;
        end = token + length;

        while((p < end) && WLESS_UART_isSpace(*p))
        {
            p++;
        }

        if((p >= end) || !WLESS_UART_isDigit(*p))
        {
            return -2;
        }

        while((p < end) && WLESS_UART_isDigit(*p))
        {
            uint16_t digit = (uint16_t)(*p - '0');

            if((value > 6553U) ||
               ((value == 6553U) && (digit > 5U)))
            {
                return -3;
            }

            value = (uint16_t)(value * 10U + digit);
            p++;
        }

        while(p < end)
        {
            if(!WLESS_UART_isSpace(*p))
            {
                return -4;
            }
            p++;
        }

        *entry->destination = value;
        *mask |= entry->id;
        return 0;
    }

    return -1;
}

static WLESS_UART_ParseResult WLESS_UART_parseAll(const char *str)
{
    WLESS_UART_ParseResult result = {0U, 0U, 0U};
    const char *p = str;

    while(*p != '\0')
    {
        const char *start = p;
        uint32_t oldMask;
        uint16_t length;

        while((*p != '\0') && (*p != WLESS_UART_TOKEN_SEP))
        {
            p++;
        }

        length = (uint16_t)(p - start);
        if(length > 0U)
        {
            result.tokens++;
            oldMask = result.mask;
            (void)WLESS_UART_parseToken(start, length, &result.mask);
            if(result.mask != oldMask)
            {
                result.matches++;
            }
        }

        if(*p == WLESS_UART_TOKEN_SEP)
        {
            p++;
        }
    }

    return result;
}

static uint16_t WLESS_UART_handleCommand(const char *command)
{
    WLESS_UART_ParseResult result;
    uint32_t wptValue;
    uint16_t wptHfcInhibitMask;

    WLESS_UART_sendString(command);
    WLESS_UART_sendString("\r\n");

    if(strcmp(command, "CQ?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendString("CQ,1,");
        WLESS_UART_sendInt((int32_t)WLESS_Config_source);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)WLESS_Config_sequence);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)WLESS_Config_active.iTankOffset_mV);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt(
                (int32_t)WLESS_Config_active.iTankSensitivity_mV_A);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)WLESS_Config_active.iTankLimit_mA);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)WLESS_Config_active.iLAOffsetRaw);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)WLESS_Config_active.iLBOffsetRaw);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }
    if(strcmp(command, "IO?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendString("IO,1,");
        WLESS_UART_sendInt((int32_t)WLESS_Config_pending.iTankOffset_mV);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }
    if(strcmp(command, "IG?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendString("IG,1,");
        WLESS_UART_sendInt(
                (int32_t)WLESS_Config_pending.iTankSensitivity_mV_A);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }
    if(strcmp(command, "IM?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendString("IM,1,");
        WLESS_UART_sendInt((int32_t)WLESS_Config_pending.iTankLimit_mA);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }
    if(strcmp(command, "CV?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendString("CV,1,");
        WLESS_UART_sendInt((int32_t)WLESS_Config_validate(
                &WLESS_Config_pending));
        WLESS_UART_sendString("\r\n");
        return 0U;
    }
    if(strcmp(command, "CA") == 0)
    {
        WLESS_UART_commandCount++;
        if((UNIPD_bbcPowerOutputEnable != 0U) ||
           (TTPLPFC_bbcEnabled != 0U) ||
           (TTPLPFC_bbcDockTestEnable != 0U) ||
           (UNIPD_wptHfcActuatorEnable != 0U))
        {
            WLESS_UART_sendString("CA ACTIVE\r\n");
            return 1U;
        }
        WLESS_UART_sendString((WLESS_Config_apply() != 0U) ?
                "CA,1,OK\r\n" : "CA,1,INVALID\r\n");
        return 0U;
    }
    if(strcmp(command, "CS") == 0)
    {
        WLESS_UART_commandCount++;
        if((UNIPD_bbcPowerOutputEnable != 0U) ||
           (TTPLPFC_bbcEnabled != 0U) ||
           (TTPLPFC_bbcDockTestEnable != 0U) ||
           (UNIPD_wptHfcActuatorEnable != 0U))
        {
            WLESS_UART_sendString("CS ACTIVE\r\n");
            return 1U;
        }
        WLESS_UART_sendString((WLESS_Config_save() != 0U) ?
                "CS,1,OK\r\n" : "CS,1,FAIL\r\n");
        return 0U;
    }
    if(strcmp(command, "CR") == 0)
    {
        WLESS_UART_commandCount++;
        if((UNIPD_bbcPowerOutputEnable != 0U) ||
           (TTPLPFC_bbcEnabled != 0U) ||
           (TTPLPFC_bbcDockTestEnable != 0U) ||
           (UNIPD_wptHfcActuatorEnable != 0U))
        {
            WLESS_UART_sendString("CR ACTIVE\r\n");
            return 1U;
        }
        WLESS_UART_sendString((WLESS_Config_reload() != 0U) ?
                "CR,1,FLASH\r\n" : "CR,1,DEFAULT\r\n");
        return 0U;
    }
    if(strcmp(command, "CD") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_Config_loadDefaults();
        WLESS_UART_sendString("CD,1,OK\r\n");
        return 0U;
    }
    if(WLESS_UART_parseExactUnsigned(command, "IO=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 500UL)
        {
            WLESS_UART_sendString("IO RANGE\r\n");
            return 1U;
        }
        WLESS_Config_pending.iTankOffset_mV = (uint16_t)wptValue;
        WLESS_UART_sendString("IO,1,");
        WLESS_UART_sendInt((int32_t)WLESS_Config_pending.iTankOffset_mV);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }
    if(WLESS_UART_parseExactUnsigned(command, "IG=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if((wptValue < 100UL) || (wptValue > 1000UL))
        {
            WLESS_UART_sendString("IG RANGE\r\n");
            return 1U;
        }
        WLESS_Config_pending.iTankSensitivity_mV_A = (uint16_t)wptValue;
        WLESS_UART_sendString("IG,1,");
        WLESS_UART_sendInt(
                (int32_t)WLESS_Config_pending.iTankSensitivity_mV_A);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }
    if(WLESS_UART_parseExactUnsigned(command, "IM=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if((wptValue == 0UL) || (wptValue > 5000UL))
        {
            WLESS_UART_sendString("IM RANGE\r\n");
            return 1U;
        }
        WLESS_Config_pending.iTankLimit_mA = (uint16_t)wptValue;
        WLESS_UART_sendString("IM,1,");
        WLESS_UART_sendInt((int32_t)WLESS_Config_pending.iTankLimit_mA);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }
    if(WLESS_UART_parseExactUnsigned(command, "AO=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if((wptValue < 1500UL) || (wptValue > 2600UL))
        {
            WLESS_UART_sendString("AO RANGE\r\n");
            return 1U;
        }
        WLESS_Config_pending.iLAOffsetRaw = (uint16_t)wptValue;
        WLESS_UART_sendString("AO,1,");
        WLESS_UART_sendInt((int32_t)WLESS_Config_pending.iLAOffsetRaw);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }
    if(WLESS_UART_parseExactUnsigned(command, "BO=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if((wptValue < 1500UL) || (wptValue > 2600UL))
        {
            WLESS_UART_sendString("BO RANGE\r\n");
            return 1U;
        }
        WLESS_Config_pending.iLBOffsetRaw = (uint16_t)wptValue;
        WLESS_UART_sendString("BO,1,");
        WLESS_UART_sendInt((int32_t)WLESS_Config_pending.iLBOffsetRaw);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }

    if(strcmp(command, "CAP=1") == 0)
    {
        WLESS_UART_commandCount++;
        if((UNIPD_bbcVdcTripLatched != 0U) ||
           (UNIPD_bbcILTripLatched != 0U))
        {
            WLESS_UART_sendString("CAPS,1,FAULT\r\n");
            WLESS_UART_sendBbcCaptureStatus();
            return 0U;
        }
        UNIPD_armBbcCapture();
        WLESS_UART_sendBbcCaptureStatus();
        return 0U;
    }
    if(strcmp(command, "CAP=0") == 0)
    {
        WLESS_UART_commandCount++;
        UNIPD_stopBbcCapture();
        WLESS_UART_sendBbcCaptureStatus();
        return 0U;
    }
    if(strcmp(command, "CAP?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendBbcCaptureStatus();
        return 0U;
    }
    if(strcmp(command, "CAPD?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendBbcCaptureDump();
        return 0U;
    }

    /* Keep the read-only release query outside the already full 32-bit
     * command mask.  Exact matching also ensures that FW=... is rejected. */
    if(strcmp(command, "FW?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendString("FW,1,RELEASE=");
        WLESS_UART_sendInt((int32_t)FIRMWARE_RELEASE);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }

    /* Report the compile-time role explicitly.  Runtime Role from VARS?/VH
     * may legitimately be NOTHING while the distributed FSM is in DISCOVERY,
     * so it must not be used to identify the firmware image. */
    if(strcmp(command, "ROLE?") == 0)
    {
        WLESS_UART_commandCount++;
#if WLESS_SM_BUILD_VEHICLE == 1
        WLESS_UART_sendString("ROLE,1,BUILD=VEHICLE\r\n");
#else
        WLESS_UART_sendString("ROLE,1,BUILD=STATION\r\n");
#endif
        return 0U;
    }

    /*
     * Correct the physical IL_A/IL_B sign convention without consuming a bit
     * in the already full command mask.  Changing polarity while the BBC is
     * active would instantaneously change controller feedback, so reject it.
     * UP bit 0 = invert IL_A; bit 1 = invert IL_B.
     */
    if(strcmp(command, "UP?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendString("UP,1,MASK=");
        WLESS_UART_sendInt((int32_t)UNIPD_bbcCurrentPolarityMask);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }
    if(WLESS_UART_parseExactUnsigned(command, "UP=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 3UL)
        {
            WLESS_UART_sendString("UP RANGE\r\n");
            WLESS_UART_commandErrorCount++;
            return 1U;
        }
        if((UNIPD_bbcPowerOutputEnable != 0U) ||
           (TTPLPFC_bbcEnabled != 0U) ||
           (TTPLPFC_bbcDockTestEnable != 0U))
        {
            WLESS_UART_sendString("UP REJECTED ACTIVE\r\n");
            WLESS_UART_commandErrorCount++;
            return 1U;
        }
        UNIPD_bbcCurrentPolarityMask = (unsigned int)wptValue;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendString("UP=");
        WLESS_UART_sendInt((int32_t)UNIPD_bbcCurrentPolarityMask);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }

    /* Explicit HFC source bridge control for bench tests.  The receiver test
     * path remains disabled: the remote bridge is used as a passive rectifier. */
    if(strcmp(command, "HFC=0") == 0)
    {
        WLESS_UART_commandCount++;
        CLLLC_clearTrip = 0;
        CLLLC_hfcReceiverTestRun = 0U;
        CLLLC_hfcReceiverTestEnable = 0U;
        CLLLC_FORCE_PWM_OST_TRIP(CLLLC_PRIM_LEG1_PWM_BASE);
        CLLLC_FORCE_PWM_OST_TRIP(CLLLC_PRIM_LEG2_PWM_BASE);
#if (CLLLC_PWM3_SYNC90_ENABLED == 1) && (CLLLC_SECONDARY_ENABLED == 0)
        CLLLC_FORCE_PWM_OST_TRIP(CLLLC_SEC_LEG1_PWM_BASE);
#endif
        WLESS_UART_sendString("HFC=0\r\n");
        return 0U;
    }

    if(strcmp(command, "HFC=1") == 0)
    {
        WLESS_UART_commandCount++;
        CLLLC_hfcReceiverTestRun = 0U;
        CLLLC_hfcReceiverTestEnable = 0U;
        CLLLC_clearTrip = 1;
        WLESS_UART_sendString("HFC=1\r\n");
        return 0U;
    }

    if(strcmp(command, "HFC?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendString("HFC,1,");
        WLESS_UART_sendInt((int32_t)CLLLC_tripFlag.CLLLC_TripFlag_Enum);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)CLLLC_hfcGanFaultActiveLow);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)CLLLC_pwmFrequency_Hz);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(CLLLC_pwmDutyPrimRef_pu * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(CLLLC_pwmPhaseShiftPrimLegsRef_pu *
                                    1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)CLLLC_iTankModSensedRaw);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(CLLLC_iTankModSensed_pu *
                                     CLLLC_iTankModAmpsPerPu *
                                     1000.0f));
        WLESS_UART_sendString(",F=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcActuatorFault);
        WLESS_UART_sendString(",LAST_mpu=");
        WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcPhaseLast_pu * 1000.0f));
        WLESS_UART_sendString(",PEAK_mpu=");
        WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcPhasePeak_pu * 1000.0f));
        WLESS_UART_sendString(",TICKS=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcPhaseTicksLast);
        WLESS_UART_sendString(",CYC=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcActiveCycles);
        WLESS_UART_sendString(",FVD_mV=");
        WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcFaultVdc_Volts * 1000.0f));
        WLESS_UART_sendString(",PHS1=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcEpwm1TbphsRawLast);
        WLESS_UART_sendString(",PHS2=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcEpwm2TbphsRawLast);
        WLESS_UART_sendString(",PRD1=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcEpwm1TbprdLast);
        WLESS_UART_sendString(",PRD2=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcEpwm2TbprdLast);
        WLESS_UART_sendString(",CTL1=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcEpwm1TbctlLast);
        WLESS_UART_sendString(",CTL2=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcEpwm2TbctlLast);
        WLESS_UART_sendString(",MAN=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcManualPhaseEnable);
        WLESS_UART_sendString(",MPH_mpu=");
        WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcManualPhase_pu * 1000.0f));
        WLESS_UART_sendString(",MAPINV=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcPhaseMapInvert);
        WLESS_UART_sendString(",RRINV=");
        WLESS_UART_sendInt((int32_t)UNIPD_wptHfcRemoteRoleInvalidCycles);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }

    if(strcmp(command, "WPTISRC=OFF") == 0)
    {
        WLESS_UART_commandCount++;
        UNIPD_wptIntegrationEnable = 0U;
        UNIPD_wptSourceCoilSyntheticEnable = 0U;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(strcmp(command, "WPTILOAD=OFF") == 0)
    {
        WLESS_UART_commandCount++;
        UNIPD_wptIntegrationEnable = 0U;
        UNIPD_wptLoadCoilSyntheticEnable = 0U;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(strcmp(command, "SMTHR?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendString("SMTHR,1,VBUS_MIN_V=");
        WLESS_UART_sendInt((int32_t)WLESS_SM_vBusMin_V);
        WLESS_UART_sendString(",IBAT_MIN_mA=");
        WLESS_UART_sendInt((int32_t)WLESS_SM_iBatMin_mA);
        WLESS_UART_sendString(",ICOIL_MIN_mA=");
        WLESS_UART_sendInt((int32_t)WLESS_SM_iCoilMin_mA);
        WLESS_UART_sendString("\r\n");
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "SMVBUS=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 65535UL)
        {
            WLESS_UART_sendString("SMVBUS RANGE\r\n");
            return 1U;
        }
        WLESS_SM_vBusMin_V = (uint16_t)wptValue;
        WLESS_UART_sendString("SMVBUS OK\r\n");
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "SMIBAT=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 65535UL)
        {
            WLESS_UART_sendString("SMIBAT RANGE\r\n");
            return 1U;
        }
        WLESS_SM_iBatMin_mA = (uint16_t)wptValue;
        WLESS_UART_sendString("SMIBAT OK\r\n");
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "SMICOIL=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 65535UL)
        {
            WLESS_UART_sendString("SMICOIL RANGE\r\n");
            return 1U;
        }
        WLESS_SM_iCoilMin_mA = (uint16_t)wptValue;
        WLESS_UART_sendString("SMICOIL OK\r\n");
        return 0U;
    }

    if(strcmp(command, "WPTHFC=0") == 0)
    {
        WLESS_UART_commandCount++;
        UNIPD_disableWptHfcActuator();
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTIRINV=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 1UL)
        {
            WLESS_UART_sendString("WPTIRINV RANGE\r\n");
            return 1U;
        }
        UNIPD_disableWptHfcActuator();
        UNIPD_wptRemoteCoilErrInvert = (unsigned int)wptValue;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTHFCINV=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 1UL)
        {
            WLESS_UART_sendString("WPTHFCINV RANGE\r\n");
            return 1U;
        }
        UNIPD_disableWptHfcActuator();
        UNIPD_wptHfcPhaseMapInvert = (unsigned int)wptValue;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTCLAMP=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 1UL)
        {
            WLESS_UART_sendString("WPTCLAMP RANGE\r\n");
            return 1U;
        }
        /* Le modifiche del controllore sono applicate soltanto con HFC fermo. */
        UNIPD_disableWptHfcActuator();
        UNIPD_wptHfcLegacyVacClampEnable = (unsigned int)wptValue;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(strcmp(command, "WPTHFCPHAUTO") == 0)
    {
        WLESS_UART_commandCount++;
        UNIPD_disableWptHfcActuator();
        UNIPD_wptHfcManualPhaseEnable = 0U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTHFCPH=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        UNIPD_disableWptHfcActuator();
        if(wptValue > 500UL)
        {
            WLESS_UART_sendString("ERR RANGE\r\n");
            return 0U;
        }
        UNIPD_wptHfcManualPhase_pu = (float)wptValue * 0.001f;
        UNIPD_wptHfcManualPhaseEnable = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(strcmp(command, "WPTHFC=1") == 0)
    {
        WLESS_UART_commandCount++;
        wptHfcInhibitMask = 0U;
        if(UNIPD_wptIntegrationEnable == 0U)
        {
            wptHfcInhibitMask |= 1U;
        }
        if(WLESS_SM_localRole != WLESS_SM_ROLE_SOURCE)
        {
            wptHfcInhibitMask |= 2U;
        }
        if(WLESS_SM_remoteRole != WLESS_SM_ROLE_LOAD)
        {
            wptHfcInhibitMask |= 4U;
        }
        if(WLESS_SM_radioLink != WLESS_SM_LINK_OK)
        {
            wptHfcInhibitMask |= 8U;
        }
        if((UNIPD_wptSourceCoilSyntheticEnable == 0U) &&
           ((UNIPD_bbcInputs.valid_mask &
             UNIPD_BBC_SIGNAL_I_COIL_LOC) == 0U))
        {
            wptHfcInhibitMask |= 16U;
        }
        if(wptHfcInhibitMask != 0U)
        {
            WLESS_UART_sendString("WPTHFC INHIBIT,");
            WLESS_UART_sendInt((int32_t)wptHfcInhibitMask);
            WLESS_UART_sendString("\r\n");
            WLESS_UART_sendWptIntegration();
            return 1U;
        }
        UNIPD_wptHfcActuatorFault = 0U;
        UNIPD_wptHfcPhaseRequested_pu = 0.0f;
        UNIPD_wptHfcPhaseApplied_pu = 0.0f;
        UNIPD_wptHfcPhasePeak_pu = 0.0f;
        UNIPD_wptHfcPhaseLast_pu = 0.0f;
        UNIPD_wptHfcPhaseTicksLast = 0;
        UNIPD_wptHfcActiveCycles = 0UL;
        UNIPD_wptHfcRemoteRoleInvalidCycles = 0UL;
        UNIPD_wptHfcFaultVdc_Volts = 0.0f;
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
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTHFCLIM=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        UNIPD_disableWptHfcActuator();
        if((wptValue < 1UL) || (wptValue > 500UL))
        {
            WLESS_UART_sendString("WPTHFCLIM RANGE\r\n");
            return 1U;
        }
        UNIPD_wptHfcPhaseMax_pu = (float)wptValue * 0.001f;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTHFCRAMP=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        UNIPD_disableWptHfcActuator();
        if(wptValue > 100UL)
        {
            WLESS_UART_sendString("WPTHFCRAMP RANGE\r\n");
            return 1U;
        }
        UNIPD_wptHfcPhaseRampStep_pu = (float)wptValue * 0.000001f;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTISRC=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 100000UL)
        {
            WLESS_UART_sendString("WPTISRC RANGE\r\n");
            return 1U;
        }
        UNIPD_wptIntegrationEnable = 0U;
        UNIPD_wptSourceCoilSynthetic_Amps = (float)wptValue * 0.001f;
        UNIPD_wptSourceCoilSyntheticEnable = 1U;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTILOAD=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 100000UL)
        {
            WLESS_UART_sendString("WPTILOAD RANGE\r\n");
            return 1U;
        }
        UNIPD_wptIntegrationEnable = 0U;
        UNIPD_wptLoadCoilSynthetic_Amps = (float)wptValue * 0.001f;
        UNIPD_wptLoadCoilSyntheticEnable = 1U;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTPLIMINIT=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 10000UL)
        {
            WLESS_UART_sendString("WPTPLIMINIT RANGE\r\n");
            return 1U;
        }
        UNIPD_disableWptHfcActuator();
        UNIPD_wptIntegrationEnable = 0U;
        UNIPD_wptTxPowerSeed_Watts = (float)wptValue;
        UNIPD_wptTxPowerSeedPending = 1U;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTPLOADINIT=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 10000UL)
        {
            WLESS_UART_sendString("WPTPLOADINIT RANGE\r\n");
            return 1U;
        }
        UNIPD_disableWptHfcActuator();
        UNIPD_wptIntegrationEnable = 0U;
        UNIPD_wptLoadPowerSeed_Watts = (float)wptValue;
        UNIPD_wptLoadPowerSeedPending = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTIZERO=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 100000UL)
        {
            WLESS_UART_sendString("WPTIZERO RANGE\r\n");
            return 1U;
        }
        UNIPD_wptIntegrationEnable = 0U;
        UNIPD_wptLoadCoilOffset_Amps = (float)wptValue * 0.001f;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    /* UniPD distributed WPT computation only. This enable never writes PWM. */
    if(strcmp(command, "WPT=0") == 0)
    {
        WLESS_UART_commandCount++;
        UNIPD_wptIntegrationEnable = 0U;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }
    if(strcmp(command, "WPT=1") == 0)
    {
        WLESS_UART_commandCount++;
        UNIPD_resetControlStatesCommand = 1U;
        UNIPD_wptIntegrationEnable = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }
    if(strcmp(command, "WPT?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }
    if(strcmp(command, "WPTSNAP?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendWptSnapshot();
        return 0U;
    }

    if(strcmp(command, "WPTCAP=1") == 0)
    {
        WLESS_UART_commandCount++;
        UNIPD_armWptCapture();
        WLESS_UART_sendWptCaptureStatus();
        return 0U;
    }

    if(strcmp(command, "WPTCAP=2") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_NRF24_resetPowerTrace();
        UNIPD_armWptCaptureDropTrigger();
        WLESS_UART_sendWptCaptureStatus();
        return 0U;
    }

    if(strcmp(command, "WPTCAP=0") == 0)
    {
        WLESS_UART_commandCount++;
        UNIPD_stopWptCapture();
        WLESS_UART_sendWptCaptureStatus();
        return 0U;
    }

    if(strcmp(command, "WPTCAP?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendWptCaptureStatus();
        return 0U;
    }

    if(strcmp(command, "WPTCAPD?") == 0)
    {
        WLESS_UART_commandCount++;
        WLESS_UART_sendWptCaptureDump();
        return 0U;
    }

    if(WLESS_UART_parseExactUnsigned(command, "WPTCAPDEC=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if((wptValue == 0UL) || (wptValue > 65535UL) ||
           (UNIPD_wptCaptureArmed != 0U))
        {
            WLESS_UART_sendString("WPTCAPDEC REJECTED\r\n");
            return 0U;
        }
        UNIPD_wptCaptureDecimation = (unsigned int)wptValue;
        WLESS_UART_sendWptCaptureStatus();
        return 0U;
    }
    if(WLESS_UART_parseExactUnsigned(command, "WPTSRC=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if((wptValue < 1000UL) || (wptValue > 300000UL))
        {
            WLESS_UART_sendString("WPTSRC RANGE\r\n");
            return 1U;
        }
        UNIPD_wptIntegrationEnable = 0U;
        UNIPD_wptVdcSourceRef_Volts = (float)wptValue * 0.001f;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }
    if(WLESS_UART_parseExactUnsigned(command, "WPTLOAD=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if((wptValue < 1000UL) || (wptValue > 300000UL))
        {
            WLESS_UART_sendString("WPTLOAD RANGE\r\n");
            return 1U;
        }
        UNIPD_wptIntegrationEnable = 0U;
        UNIPD_wptVdcLoadRef_Volts = (float)wptValue * 0.001f;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }
    if(WLESS_UART_parseExactUnsigned(command, "WPTIMAX=", &wptValue) != 0U)
    {
        WLESS_UART_commandCount++;
        if(wptValue > 100000UL)
        {
            WLESS_UART_sendString("WPTIMAX RANGE\r\n");
            return 1U;
        }
        UNIPD_wptIntegrationEnable = 0U;
        UNIPD_wptCoilCurrentMax_Amps = (float)wptValue * 0.001f;
        UNIPD_resetControlStatesCommand = 1U;
        WLESS_UART_sendWptIntegration();
        return 0U;
    }

    result = WLESS_UART_parseAll(command);
    if(result.tokens == 0U)
    {
        return 0U;
    }
    if(result.matches == 0U)
    {
        WLESS_UART_sendString("Wrong command\r\n");
        WLESS_UART_commandErrorCount++;
        return 1U;
    }

    WLESS_UART_commandCount++;

    if((result.mask & WLESS_UART_KEY_WH) != 0U)
    {
        WLESS_SM_localEnergyEncoded = WLESS_SM_encodeCapacityWh(WLESS_UART_wh);
    }
    if((result.mask & WLESS_UART_KEY_WH_QUERY) != 0U)
    {
        WLESS_UART_sendString("WH,1,VALUE=");
        WLESS_UART_sendInt(WLESS_SM_decodeCapacityWh(WLESS_SM_localEnergyEncoded));
        WLESS_UART_sendString("\r\n");
    }
    if((result.mask & WLESS_UART_KEY_VARS_QUERY) != 0U)
    {
        WLESS_UART_sendVars();
    }
    if((result.mask & WLESS_UART_KEY_RADIO_QUERY) != 0U)
    {
        WLESS_UART_sendRadio();
    }
    if((result.mask & WLESS_UART_KEY_RADIO_PING) != 0U)
    {
        WLESS_UART_sendString(WLESS_NRF24_sendDiagnosticPing() ?
                              "RADIOPING,1,OK\r\n" :
                              "RADIOPING,1,REJECTED\r\n");
    }
    if((result.mask & WLESS_UART_KEY_UTEST) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        if((WLESS_UART_unipdTest > 0U) &&
           (WLESS_UART_unipdTest <= 20U))
        {
            WLESS_UART_setUnipdTestVector((float)WLESS_UART_unipdTest);
        }
        else if(WLESS_UART_unipdTest == 0U)
        {
            UNIPD_bbcSyntheticTestEnable = 0U;
            UNIPD_resetControlStatesCommand = 1U;
        }
        WLESS_UART_sendUnipd();
    }
    if((result.mask & WLESS_UART_KEY_UQUERY) != 0U)
    {
        WLESS_UART_sendUnipd();
    }
    if((result.mask & WLESS_UART_KEY_UMAP) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        UNIPD_bbcDutyMappingMode = (WLESS_UART_unipdMap != 0U) ? 1U : 0U;
        UNIPD_resetControlStatesCommand = 1U;
    }
    if((result.mask & WLESS_UART_KEY_UDMAX) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        if(WLESS_UART_unipdDutyMaxMilli > 950U)
        {
            WLESS_UART_unipdDutyMaxMilli = 950U;
        }
        UNIPD_bbcPowerOutputDutyMax_pu =
                ((float)WLESS_UART_unipdDutyMaxMilli) * 0.001f;
    }
    if((result.mask & WLESS_UART_KEY_USTEP) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if((WLESS_UART_unipdRampStepMicro >= 1U) &&
           (WLESS_UART_unipdRampStepMicro <= 2000U))
        {
            UNIPD_bbcPowerOutputDutyRampStep_pu =
                    ((float)WLESS_UART_unipdRampStepMicro) * 0.000001f;
        }
    }
    if((result.mask & WLESS_UART_KEY_UREF) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if((WLESS_UART_unipdVdcRef > 0U) &&
           (WLESS_UART_unipdVdcRef <= 300U))
        {
            WLESS_UART_unipdVdcRefConfig = WLESS_UART_unipdVdcRef;
            WLESS_UART_unipdVdcRefMilliConfig =
                    ((uint32_t)WLESS_UART_unipdVdcRef) * 1000UL;
            UNIPD_bbcSyntheticInputs.v_dc_pbat_rif =
                    ((float)WLESS_UART_unipdVdcRefMilliConfig) * 0.001f;
            UNIPD_resetControlStatesCommand = 1U;
        }
    }
    if((result.mask & WLESS_UART_KEY_UREF_MV) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if((WLESS_UART_unipdVdcRefMilli >= 1000U) &&
           (WLESS_UART_unipdVdcRefMilli <= 60000U))
        {
            WLESS_UART_unipdVdcRefMilliConfig = WLESS_UART_unipdVdcRefMilli;
            WLESS_UART_unipdVdcRefConfig =
                    (WLESS_UART_unipdVdcRefMilliConfig + 500U) / 1000U;
            UNIPD_bbcSyntheticInputs.v_dc_pbat_rif =
                    ((float)WLESS_UART_unipdVdcRefMilliConfig) * 0.001f;
            UNIPD_resetControlStatesCommand = 1U;
        }
    }
    if((result.mask & WLESS_UART_KEY_UIBATMAX) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if(WLESS_UART_unipdIBatMaxMilli <= 20000U)
        {
            WLESS_UART_unipdIBatMaxMilliConfig =
                    WLESS_UART_unipdIBatMaxMilli;
            WLESS_UART_applyUnipdDirectionalCurrentLimit();
            UNIPD_resetControlStatesCommand = 1U;
        }
    }
    if((result.mask & WLESS_UART_KEY_UVBAT_MV) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if((WLESS_UART_unipdVBatMilli >= 1000U) &&
           (WLESS_UART_unipdVBatMilli <= 60000U))
        {
            WLESS_UART_unipdVBatMilliConfig = WLESS_UART_unipdVBatMilli;
            UNIPD_bbcSyntheticInputs.v_bat =
                    ((float)WLESS_UART_unipdVBatMilliConfig) * 0.001f;
            UNIPD_resetControlStatesCommand = 1U;
        }
    }
    if((result.mask & WLESS_UART_KEY_UVTRIP_MV) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if((WLESS_UART_unipdVdcTripMilli >= 2000U) &&
           (WLESS_UART_unipdVdcTripMilli <= 60000U))
        {
            UNIPD_bbcVdcTripThreshold_Volts =
                    ((float)WLESS_UART_unipdVdcTripMilli) * 0.001f;
            UNIPD_resetControlStatesCommand = 1U;
        }
    }
    if((result.mask & WLESS_UART_KEY_UVTRIP_V) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if((WLESS_UART_unipdVdcTripVolts >= 2U) &&
           (WLESS_UART_unipdVdcTripVolts <= 300U))
        {
            UNIPD_bbcVdcTripThreshold_Volts =
                    (float)WLESS_UART_unipdVdcTripVolts;
            UNIPD_bbcVdcTripResetCommand = 1U;
        }
    }
    if((result.mask & WLESS_UART_KEY_UITRIP_MA) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if((WLESS_UART_unipdILTripMilli >= 100U) &&
           (WLESS_UART_unipdILTripMilli <= 10000U))
        {
            UNIPD_bbcILTripThreshold_Amps =
                    ((float)WLESS_UART_unipdILTripMilli) * 0.001f;
            UNIPD_bbcVdcTripResetCommand = 1U;
        }
    }
    if((result.mask & WLESS_UART_KEY_UITRIP_N) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if((WLESS_UART_unipdILTripCycles >= 1U) &&
           (WLESS_UART_unipdILTripCycles <= 1000U))
        {
            UNIPD_bbcILTripConfirmCycles = WLESS_UART_unipdILTripCycles;
            UNIPD_bbcVdcTripResetCommand = 1U;
        }
    }
    if((result.mask & WLESS_UART_KEY_UFAULT) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if(WLESS_UART_unipdFaultReset == 2U)
        {
            /*
             * Zero-current calibration for the two BBC branch sensors.
             * TTPLPFC_read_individualCurrent() returns
             * 2 * (raw_pu - offset_pu), therefore half of the measured
             * zero-current value is the offset correction.
             */
            TTPLPFC_read_individualCurrent();
            TTPLPFC_iL1_senseOffset_pu += 0.5f * TTPLPFC_iL1_sensed_pu;
            TTPLPFC_iL2_senseOffset_pu += 0.5f * TTPLPFC_iL2_sensed_pu;
            TTPLPFC_read_individualCurrent();
            WLESS_Config_applyCurrentOffsets(
                    (uint16_t)(TTPLPFC_iL1_senseOffset_pu * 4096.0f +
                               0.5f),
                    (uint16_t)(TTPLPFC_iL2_senseOffset_pu * 4096.0f +
                               0.5f));
            UNIPD_resetControlStatesCommand = 1U;
            UNIPD_bbcVdcTripResetCommand = 1U;
        }
        else if(WLESS_UART_unipdFaultReset != 0U)
        {
            UNIPD_bbcVdcTripResetCommand = 1U;
        }
    }
    if((result.mask & WLESS_UART_KEY_UILTEST) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if((UNIPD_bbcSyntheticTestEnable != 0U) &&
           (UNIPD_bbcSyntheticOverrideMask ==
            UNIPD_BBC_REQUIRED_SIGNAL_MASK) &&
           (WLESS_UART_unipdILTestMilli <= 2000U))
        {
            const float testCurrent =
                    ((float)WLESS_UART_unipdILTestMilli) * 0.001f;
            UNIPD_bbcSyntheticInputs.i_l_a = testCurrent;
            UNIPD_bbcSyntheticInputs.i_l_b = -testCurrent;
        }
    }
    if((result.mask & WLESS_UART_KEY_UBOOST) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        if(WLESS_UART_unipdOpenLoopBoost == 0U)
        {
            TTPLPFC_bbcDockTestEnable = 0;
            TTPLPFC_bbcDockTestLegMode = TTPLPFC_BBC_DOCK_TEST_LEG_DISABLED;
            TTPLPFC_BBC_disable();
        }
        else if((WLESS_UART_unipdOpenLoopBoost == 1U) ||
                (WLESS_UART_unipdOpenLoopBoost == 2U))
        {
            TTPLPFC_BBC_disable();
            TTPLPFC_bbcDockTestEnable = 0;
            TTPLPFC_bbcDockTestMode =
                    (WLESS_UART_unipdOpenLoopBoost == 1U) ?
                    TTPLPFC_BBC_MODE_BOOST : TTPLPFC_BBC_MODE_BUCK;
            TTPLPFC_bbcDockTestLegMode = TTPLPFC_BBC_DOCK_TEST_LEG_BOTH;
            TTPLPFC_bbcDockTestDuty1_pu =
                    UNIPD_bbcPowerOutputDutyMax_pu;
            TTPLPFC_bbcDockTestDuty2_pu =
                    UNIPD_bbcPowerOutputDutyMax_pu;
            TTPLPFC_bbcDockTestDutyRamp1_pu = 0.0f;
            TTPLPFC_bbcDockTestDutyRamp2_pu = 0.0f;
            TTPLPFC_bbcDockTestDutyRampStep_pu =
                    UNIPD_bbcPowerOutputDutyRampStep_pu;
            TTPLPFC_bbcDockTestDutyRampEnable = 1;
            TTPLPFC_bbcDockTestVbusTrip_Volts =
                    UNIPD_bbcVdcTripThreshold_Volts;
            TTPLPFC_bbcDockTestVbusTripLatched = 0;
            TTPLPFC_bbcDockTestVbusTripCapture_Volts = 0.0f;
            TTPLPFC_bbcDockTestVbusMax_Volts = 0.0f;
            TTPLPFC_bbcDockTestEnable = 1;
        }
        else
        {
            TTPLPFC_bbcDockTestEnable = 0;
            TTPLPFC_bbcDockTestLegMode = TTPLPFC_BBC_DOCK_TEST_LEG_DISABLED;
            TTPLPFC_BBC_disable();
        }
    }
    if((result.mask & WLESS_UART_KEY_UHYBRID) != 0U)
    {
        UNIPD_bbcPowerOutputEnable = 0U;
        TTPLPFC_BBC_disable();
        if((WLESS_UART_unipdHybrid == 1U) ||
           (WLESS_UART_unipdHybrid == 2U))
        {
            UNIPD_bbcSyntheticInputs.v_bat =
                    ((float)WLESS_UART_unipdVBatMilliConfig) * 0.001f;
            UNIPD_bbcSyntheticInputs.i_coil_loc = 0.0f;
            UNIPD_bbcSyntheticInputs.i_coil_rem_err = 0.0f;
            UNIPD_bbcSyntheticInputs.i_coil_loc_rif = 0.0f;
            UNIPD_bbcSyntheticInputs.v_dc_pbat_rif =
                    ((float)WLESS_UART_unipdVdcRefMilliConfig) * 0.001f;
            WLESS_UART_unipdHybridActive = WLESS_UART_unipdHybrid;
            WLESS_UART_applyUnipdDirectionalCurrentLimit();
            UNIPD_bbcSyntheticInputs.tx_1_rx_0 =
                    (WLESS_UART_unipdHybridActive == 1U) ? 1U : 0U;
#if TTPLPFC_BBC_COMPLEMENTARY_PWM_ENABLE
            //
            // In synchronous mode UniPD delta always drives the high side;
            // EPWM dead-band hardware produces the low-side complement.
            //
            UNIPD_bbcDutyMappingMode = 0U;
#else
            UNIPD_bbcDutyMappingMode =
                    (WLESS_UART_unipdHybridActive == 1U) ? 1U : 0U;
#endif
            UNIPD_bbcSyntheticValidMask = UNIPD_BBC_REQUIRED_SIGNAL_MASK;
            UNIPD_bbcSyntheticOverrideMask =
                    UNIPD_BBC_REQUIRED_SIGNAL_MASK &
                    ~(UNIPD_BBC_SIGNAL_V_DC |
                      UNIPD_BBC_SIGNAL_V_BAT |
                      UNIPD_BBC_SIGNAL_I_L_A |
                      UNIPD_BBC_SIGNAL_I_L_B);
            UNIPD_bbcSyntheticTestEnable = 1U;
        }
        else
        {
            UNIPD_bbcSyntheticTestEnable = 0U;
            WLESS_UART_unipdHybridActive = 0U;
        }
        UNIPD_resetControlStatesCommand = 1U;
    }
    if((result.mask & WLESS_UART_KEY_UENABLE) != 0U)
    {
        if(WLESS_UART_unipdEnable == 0U)
        {
            UNIPD_bbcPowerOutputEnable = 0U;
            TTPLPFC_BBC_disable();
        }
        else if(((WLESS_UART_unipdHybridActive == 1U) ||
                 (WLESS_UART_unipdHybridActive == 2U)) &&
                 (UNIPD_bbcSyntheticTestEnable != 0U) &&
#if TTPLPFC_BBC_COMPLEMENTARY_PWM_ENABLE
                 (UNIPD_bbcDutyMappingMode == 0U) &&
#else
                 (((WLESS_UART_unipdHybridActive == 1U) &&
                   (UNIPD_bbcDutyMappingMode == 1U)) ||
                  ((WLESS_UART_unipdHybridActive == 2U) &&
                   (UNIPD_bbcDutyMappingMode == 0U))) &&
#endif
                 /*
                  * UD is entered and range-checked as integer milli-pu.
                  * Comparing its float conversion with 0.95f rejected the
                  * nominal boundary UD=950 on the target.
                  */
                 (WLESS_UART_unipdDutyMaxMilli <= 950U) &&
                 (UNIPD_bbcVdcTripEnable != 0U) &&
                 (UNIPD_bbcVdcTripLatched == 0U) &&
                 (UNIPD_bbcILTripEnable != 0U) &&
                 (UNIPD_bbcILTripLatched == 0U) &&
                (UNIPD_bbcInputs.v_dc <
                 UNIPD_bbcVdcTripThreshold_Volts))
        {
            UNIPD_resetControlStatesCommand = 1U;
            UNIPD_bbcRampedDutyA_pu = 0.0f;
            UNIPD_bbcRampedDutyB_pu = 0.0f;
            UNIPD_bbcPowerOutputEnable = 1U;
        }
        else
        {
            WLESS_UART_sendString("UE REJECTED CONFIG\r\n");
        }
    }

#if WLESS_SM_BUILD_VEHICLE == 1
    if((result.mask & WLESS_UART_KEY_SOURCE) != 0U)
    {
        WLESS_SM_localRole = WLESS_SM_ROLE_SOURCE;
    }
    if((result.mask & WLESS_UART_KEY_LOAD) != 0U)
    {
        WLESS_SM_localRole = WLESS_SM_ROLE_LOAD;
    }
    if((result.mask & WLESS_UART_KEY_AUTO) != 0U)
    {
        WLESS_SM_opMode = WLESS_SM_MODE_AUTO;
    }
    if((result.mask & WLESS_UART_KEY_MANUAL) != 0U)
    {
        WLESS_SM_opMode = WLESS_SM_MODE_MANUAL;
    }
    if((result.mask & WLESS_UART_KEY_STOP) != 0U)
    {
        WLESS_SM_stopCommand = 1U;
    }
#endif

    if((result.mask & WLESS_UART_KEY_VB) != 0U)
    {
        WLESS_SM_vBus_V = WLESS_UART_vBus;
    }
    if((result.mask & WLESS_UART_KEY_IB) != 0U)
    {
        WLESS_SM_iBat_mA = WLESS_UART_iBat;
    }
    if((result.mask & WLESS_UART_KEY_IC) != 0U)
    {
        WLESS_SM_iCoil_mA = WLESS_UART_iCoil;
    }
    if((result.mask & WLESS_UART_KEY_INITOK) != 0U)
    {
        WLESS_SM_initOkCommand = 1U;
    }

    WLESS_SM_statusUpdatePending = 1U;
    return 0U;
}

static void WLESS_UART_sendBbcCaptureStatus(void)
{
    WLESS_UART_sendString("CAPS,1,");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcCaptureArmed);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcCaptureFrozen);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcCaptureCount);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_BBC_CAPTURE_LENGTH);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_BBC_CAPTURE_DECIMATION);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcCaptureTriggerReason);
    WLESS_UART_sendString("\r\n");
}

static void WLESS_UART_sendBbcCaptureDump(void)
{
    UNIPD_BbcCaptureSample sample;
    unsigned int index;

    if(UNIPD_bbcCaptureArmed != 0U)
    {
        WLESS_UART_sendString("CAPD,1,BUSY\r\n");
        return;
    }

    WLESS_UART_sendString("CAPD_COLUMNS,1,n,vdc_mV,vbat_mV,ila_mA,ilb_mA,ilrefA_mA,ilrefB_mA,ilerrA_mA,ilerrB_mA,vlrefA_mV,vlrefB_mV,pbat_mW,rawA_u,rawB_u,intA_u,intB_u,mapA_u,mapB_u,appA_u,appB_u,rampA_u,rampB_u\r\n");
    for(index = 0U; index < UNIPD_bbcCaptureCount; index++)
    {
        if(UNIPD_getBbcCaptureSample(index, &sample) == 0U)
        {
            break;
        }
        WLESS_UART_sendString("CAPD,1,");
        WLESS_UART_sendInt((int32_t)index);
#define WLESS_UART_CAP_FIELD(value, scale) \
        WLESS_UART_sendString(","); \
        WLESS_UART_sendInt((int32_t)((value) * (scale)))
        WLESS_UART_CAP_FIELD(sample.v_dc, 1000.0f);
        WLESS_UART_CAP_FIELD(sample.v_bat, 1000.0f);
        WLESS_UART_CAP_FIELD(sample.i_l_a, 1000.0f);
        WLESS_UART_CAP_FIELD(sample.i_l_b, 1000.0f);
        WLESS_UART_CAP_FIELD(sample.i_l_rif_a, 1000.0f);
        WLESS_UART_CAP_FIELD(sample.i_l_rif_b, 1000.0f);
        WLESS_UART_CAP_FIELD(sample.i_l_err_a, 1000.0f);
        WLESS_UART_CAP_FIELD(sample.i_l_err_b, 1000.0f);
        WLESS_UART_CAP_FIELD(sample.v_l_rif_a, 1000.0f);
        WLESS_UART_CAP_FIELD(sample.v_l_rif_b, 1000.0f);
        WLESS_UART_CAP_FIELD(sample.p_bat_rif, 1000.0f);
        WLESS_UART_CAP_FIELD(sample.duty_raw_a, 1000000.0f);
        WLESS_UART_CAP_FIELD(sample.duty_raw_b, 1000000.0f);
        WLESS_UART_CAP_FIELD(sample.duty_internal_a, 1000000.0f);
        WLESS_UART_CAP_FIELD(sample.duty_internal_b, 1000000.0f);
        WLESS_UART_CAP_FIELD(sample.duty_mapped_a, 1000000.0f);
        WLESS_UART_CAP_FIELD(sample.duty_mapped_b, 1000000.0f);
        WLESS_UART_CAP_FIELD(sample.duty_applied_a, 1000000.0f);
        WLESS_UART_CAP_FIELD(sample.duty_applied_b, 1000000.0f);
        WLESS_UART_CAP_FIELD(sample.duty_ramped_a, 1000000.0f);
        WLESS_UART_CAP_FIELD(sample.duty_ramped_b, 1000000.0f);
#undef WLESS_UART_CAP_FIELD
        WLESS_UART_sendString("\r\n");
    }
    WLESS_UART_sendBbcCaptureStatus();
}

void WLESS_UART_init(void)
{
    WLESS_Config_init();
    WLESS_RB_init(&WLESS_UART_rxBuffer);
    WLESS_UART_commandIndex = 0U;
    WLESS_UART_rxByteCount = 0UL;
    WLESS_UART_rxErrorCount = 0UL;
    WLESS_UART_rxOverflowCount = 0UL;
    WLESS_UART_commandCount = 0UL;
    WLESS_UART_commandErrorCount = 0UL;

    GPIO_setPinConfig(GPIO_28_SCIA_RX);
    GPIO_setPinConfig(GPIO_29_SCIA_TX);
    GPIO_setPadConfig(28U, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
    GPIO_setPadConfig(29U, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(28U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(29U, GPIO_QUAL_ASYNC);

    SCI_performSoftwareReset(SCIA_BASE);
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, WLESS_UART_BAUDRATE,
                  SCI_CONFIG_WLEN_8 | SCI_CONFIG_STOP_ONE |
                  SCI_CONFIG_PAR_NONE);
    SCI_disableLoopback(SCIA_BASE);
    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    SCI_clearOverflowStatus(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_RXFF | SCI_INT_RXERR);
    SCI_setFIFOInterruptLevel(SCIA_BASE, SCI_FIFO_TX0, SCI_FIFO_RX1);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);

    Interrupt_register(INT_SCIA_RX, &WLESS_UART_rxIsr);
    Interrupt_enable(INT_SCIA_RX);
    SCI_enableInterrupt(SCIA_BASE, SCI_INT_RXFF);

    WLESS_SM_statusUpdatePending = 1U;
}

void WLESS_UART_process(void)
{
    uint16_t data;

    while(WLESS_RB_read(&WLESS_UART_rxBuffer, &data))
    {
        char character = (char)data;

        if((character == '\n') || (character == '\r'))
        {
            if(WLESS_UART_commandIndex > 0U)
            {
                WLESS_UART_commandBuffer[WLESS_UART_commandIndex] = '\0';
                WLESS_UART_toUpper(WLESS_UART_commandBuffer);
                (void)WLESS_UART_handleCommand(WLESS_UART_commandBuffer);
                WLESS_UART_commandIndex = 0U;
            }
        }
        else if(WLESS_UART_commandIndex < (WLESS_UART_CMD_MAX_LEN - 1U))
        {
            WLESS_UART_commandBuffer[WLESS_UART_commandIndex++] = character;
        }
        else
        {
            WLESS_UART_commandIndex = 0U;
            WLESS_UART_commandErrorCount++;
        }
    }

    if(WLESS_SM_statusUpdatePending != 0U)
    {
        WLESS_SM_statusUpdatePending = 0U;
        WLESS_UART_sendStatus();
    }
}

void WLESS_UART_sendByte(uint16_t data)
{
    while(SCI_getTxFIFOStatus(SCIA_BASE) == SCI_FIFO_TX16)
    {
    }
    SCI_writeCharNonBlocking(SCIA_BASE, data);
}

void WLESS_UART_sendString(const char *str)
{
    while(*str != '\0')
    {
        WLESS_UART_sendByte((uint16_t)*str);
        str++;
    }
}

void WLESS_UART_sendInt(int32_t value)
{
    char buffer[12];
    uint16_t index = 0U;
    uint32_t magnitude;

    if(value < 0)
    {
        WLESS_UART_sendByte('-');
        magnitude = (uint32_t)(-(value + 1L)) + 1UL;
    }
    else
    {
        magnitude = (uint32_t)value;
    }

    if(magnitude == 0UL)
    {
        WLESS_UART_sendByte('0');
        return;
    }

    while(magnitude != 0UL)
    {
        buffer[index++] = (char)('0' + (magnitude % 10UL));
        magnitude /= 10UL;
    }

    while(index > 0U)
    {
        WLESS_UART_sendByte((uint16_t)buffer[--index]);
    }
}

static const char *WLESS_UART_roleString(WLESS_SM_Role role)
{
    if(role == WLESS_SM_ROLE_SOURCE) return "SOURCE";
    if(role == WLESS_SM_ROLE_LOAD) return "LOAD";
    return "NOTHING";
}

static const char *WLESS_UART_ctrlString(WLESS_SM_ControllerState state)
{
    switch(state)
    {
        case WLESS_SM_CTRL_IDLE: return "IDLE";
        case WLESS_SM_CTRL_INIT: return "INIT";
        case WLESS_SM_CTRL_INITOK: return "INITOK";
        case WLESS_SM_CTRL_WPTON: return "WPTON";
        case WLESS_SM_CTRL_WPTOFF: return "WPTOFF";
        case WLESS_SM_CTRL_WPTEND: return "WPTEND";
        case WLESS_SM_CTRL_WPTERR: return "WPTERR";
        default: return "UNDEFINED";
    }
}

static const char *WLESS_UART_stateString(WLESS_SM_State state)
{
    static const char *const names[] =
    {
        "STANDBY", "DISCOVERY", "INIT_SOURCE", "INIT_LOAD",
        "WAIT_SOURCE", "WAIT_LOAD", "PRECHARGE_SOURCE",
        "PRECHARGE_LOAD", "SOURCE_ON", "LOAD_ON",
        "DISCHARGE_SOURCE", "DISCHARGE_LOAD", "SOURCE_OFF",
        "LOAD_OFF", "SOURCE_END", "LOAD_END", "ENDCHARGE_SOURCE",
        "ENDCHARGE_LOAD"
    };

    if((uint16_t)state < (uint16_t)(sizeof(names) / sizeof(names[0])))
    {
        return names[(uint16_t)state];
    }
    return "UNDEFINED";
}

static void WLESS_UART_sendWptIntegration(void)
{
    WLESS_UART_sendString("WPT,1,EN=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptIntegrationEnable);
    WLESS_UART_sendString(",ROLE=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_localRole);
    WLESS_UART_sendString(",SRCREF_mV=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptVdcSourceRef_Volts * 1000.0f));
    WLESS_UART_sendString(",LOADREF_mV=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptVdcLoadRef_Volts * 1000.0f));
    WLESS_UART_sendString(",IMAX_mA=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptCoilCurrentMax_Amps * 1000.0f));
    WLESS_UART_sendString(",ISYN=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptSourceCoilSyntheticEnable);
    WLESS_UART_sendString(",ILSYN=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptLoadCoilSyntheticEnable);
    WLESS_UART_sendString(",ILSYN_mA=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptLoadCoilSynthetic_Amps * 1000.0f));
    WLESS_UART_sendString(",IPHY_mA=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptLocalCoilPhysical_Amps * 1000.0f));
    WLESS_UART_sendString(",IZERO_mA=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptLoadCoilOffset_Amps * 1000.0f));
    WLESS_UART_sendString(",IUSE_mA=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptLocalCoilUsed_Amps * 1000.0f));
    WLESS_UART_sendString(",PLIM_W=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptTxPowerLimit_Watts);
    WLESS_UART_sendString(",PSEED_W=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptTxPowerSeed_Watts);
    WLESS_UART_sendString(",PSEEDP=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptTxPowerSeedPending);
    WLESS_UART_sendString(",LPSEED_W=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptLoadPowerSeed_Watts);
    WLESS_UART_sendString(",LPSEEDP=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptLoadPowerSeedPending);
    WLESS_UART_sendString(",PREM_W=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptRemotePowerLimit_Watts);
    WLESS_UART_sendString(",PREF_W=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptRxOutput.p_trasf_rif);
    WLESS_UART_sendString(",IREF_cA=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptRxOutput.i_coil_rif * 100.0f));
    WLESS_UART_sendString(",IERR_cA=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptRxOutput.i_coil_err * 100.0f));
    WLESS_UART_sendString(",IREM_cA=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptRemoteCoilErr_Amps * 100.0f));
    WLESS_UART_sendString(",RINV=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptRemoteCoilErrInvert);
    WLESS_UART_sendString(",CLAMP=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptHfcLegacyVacClampEnable);
    WLESS_UART_sendString(",VACRAW_mV=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcOutput.v_ac_rif_raw * 1000.0f));
    WLESS_UART_sendString(",VAC_mV=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcOutput.v_ac_rif * 1000.0f));
    WLESS_UART_sendString(",HENA=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptHfcActuatorEnable);
    WLESS_UART_sendString(",HFLT=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptHfcActuatorFault);
    WLESS_UART_sendString(",HREQ_mpu=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcPhaseRequested_pu * 1000.0f));
    WLESS_UART_sendString(",HAPP_mpu=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcPhaseApplied_pu * 1000.0f));
    WLESS_UART_sendString(",HLIM_mpu=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcPhaseMax_pu * 1000.0f));
    WLESS_UART_sendString(",HRAMP_upc=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcPhaseRampStep_pu * 1000000.0f));
    WLESS_UART_sendString(",PSA_mpu=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcOutput.duty_cycle_ps_a * 1000.0f));
    WLESS_UART_sendString(",PSB_mpu=");
    WLESS_UART_sendInt((int32_t)(UNIPD_wptHfcOutput.duty_cycle_ps_b * 1000.0f));
    WLESS_UART_sendString("\r\n");
}

static void WLESS_UART_sendWptSnapshot(void)
{
    WLESS_UART_WptSnapshot snapshot;

    /*
     * Freeze only long enough to copy the ISR-owned values.  Conversion and
     * UART transmission deliberately remain outside the critical section.
     */
    DINT;
    snapshot.tickCounter = WLESS_SM_tickCounter;
    snapshot.stateStepCounter = WLESS_SM_stateStepCounter;
    snapshot.localRole = (uint16_t)WLESS_SM_localRole;
    snapshot.state = (uint16_t)WLESS_SM_state;
    snapshot.radioLink = (uint16_t)WLESS_SM_radioLink;
    snapshot.noAckCount = WLESS_SM_noAckCount;
    snapshot.coilPhysical_Amps = UNIPD_wptLocalCoilPhysical_Amps;
    snapshot.coilOffset_Amps = UNIPD_wptLoadCoilOffset_Amps;
    snapshot.coilUsed_Amps = UNIPD_wptLocalCoilUsed_Amps;
    snapshot.remotePowerLimit_Watts = UNIPD_wptRemotePowerLimit_Watts;
    snapshot.transferredPowerRef_Watts = UNIPD_wptRxOutput.p_trasf_rif;
    snapshot.coilCurrentRef_Amps = UNIPD_wptRxOutput.i_coil_rif;
    snapshot.coilCurrentErr_Amps = UNIPD_wptRxOutput.i_coil_err;
    EINT;

    WLESS_UART_sendString("WPTSNAP,1,TICK=");
    WLESS_UART_sendInt((int32_t)snapshot.tickCounter);
    WLESS_UART_sendString(",STEP=");
    WLESS_UART_sendInt((int32_t)snapshot.stateStepCounter);
    WLESS_UART_sendString(",ROLE=");
    WLESS_UART_sendInt((int32_t)snapshot.localRole);
    WLESS_UART_sendString(",STATE=");
    WLESS_UART_sendInt((int32_t)snapshot.state);
    WLESS_UART_sendString(",LINK=");
    WLESS_UART_sendInt((int32_t)snapshot.radioLink);
    WLESS_UART_sendString(",NOACK=");
    WLESS_UART_sendInt((int32_t)snapshot.noAckCount);
    WLESS_UART_sendString(",IPHY_mA=");
    WLESS_UART_sendInt((int32_t)(snapshot.coilPhysical_Amps * 1000.0f));
    WLESS_UART_sendString(",IZERO_mA=");
    WLESS_UART_sendInt((int32_t)(snapshot.coilOffset_Amps * 1000.0f));
    WLESS_UART_sendString(",IUSE_mA=");
    WLESS_UART_sendInt((int32_t)(snapshot.coilUsed_Amps * 1000.0f));
    WLESS_UART_sendString(",PREM_W=");
    WLESS_UART_sendInt((int32_t)snapshot.remotePowerLimit_Watts);
    WLESS_UART_sendString(",PREF_W=");
    WLESS_UART_sendInt((int32_t)snapshot.transferredPowerRef_Watts);
    WLESS_UART_sendString(",IREF_cA=");
    WLESS_UART_sendInt((int32_t)(snapshot.coilCurrentRef_Amps * 100.0f));
    WLESS_UART_sendString(",IERR_cA=");
    WLESS_UART_sendInt((int32_t)(snapshot.coilCurrentErr_Amps * 100.0f));
    WLESS_UART_sendString("\r\n");
}

static void WLESS_UART_sendWptCaptureStatus(void)
{
    WLESS_UART_sendString("WPTCAP,1,ARM=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptCaptureArmed);
    WLESS_UART_sendString(",FROZEN=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptCaptureFrozen);
    WLESS_UART_sendString(",COUNT=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptCaptureCount);
    WLESS_UART_sendString(",LEN=");
    WLESS_UART_sendInt((int32_t)UNIPD_WPT_CAPTURE_LENGTH);
    WLESS_UART_sendString(",DEC=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptCaptureDecimation);
    WLESS_UART_sendString(",DROPTRIG=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptCaptureDropTriggerEnable);
    WLESS_UART_sendString(",REASON=");
    WLESS_UART_sendInt((int32_t)UNIPD_wptCaptureTriggerReason);
    WLESS_UART_sendString("\r\n");
}

static void WLESS_UART_sendWptCaptureDump(void)
{
    UNIPD_WptCaptureSample sample;
    unsigned int index;

    if(UNIPD_wptCaptureArmed != 0U)
    {
        WLESS_UART_sendString("WPTCAPD,1,REJECTED_ARMED\r\n");
        return;
    }

    WLESS_UART_sendString(
        "WPTCAPD_COLUMNS,2,IDX,TICK,ROLE,STATE,LINK,VDC_mV,IPHY_mA,IUSE_mA,"
        "PREM_mW,PREF_mW,PPREV_mW,VERR2_x1000,VERR2P_x1000,"
        "REMOTE_RAW,SEEDP,CLEARSEQ,"
        "IREF_cA,IERR_cA,IREM_cA,VACRAW_mV,HRAW_mpu,HMAP_mpu,"
        "HREQ_mpu,HAPP_mpu,"
        "HAUTO_mpu,HPHY_mpu,MAN,FAULT\r\n");
    for(index = 0U; index < UNIPD_wptCaptureCount; index++)
    {
        if(UNIPD_getWptCaptureSample(index, &sample) == 0U)
        {
            break;
        }
        WLESS_UART_sendString("WPTCAPD,2,");
        WLESS_UART_sendInt((int32_t)index);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)sample.tick);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)sample.role);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)sample.state);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)sample.link);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.v_dc * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.i_coil_physical * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.i_coil_used * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.v_ac_ref * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.p_ref * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.hfc_unipd_signed * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.hfc_mapped * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.hfc_request * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)sample.hfc_applied);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)sample.hfc_auto_hardware);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)sample.hfc_physical);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.i_ref * 100.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.i_err * 100.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.i_remote_err * 100.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.v_ac_ref * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.hfc_unipd_signed * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.hfc_mapped * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.hfc_request * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.hfc_applied * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.hfc_auto_hardware * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)(sample.hfc_physical * 1000.0f));
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)sample.manual);
        WLESS_UART_sendString(",");
        WLESS_UART_sendInt((int32_t)sample.hfc_fault);
        WLESS_UART_sendString("\r\n");
    }
    WLESS_UART_sendWptCaptureStatus();
}

void WLESS_UART_sendStatus(void)
{
#if WLESS_SM_BUILD_VEHICLE == 1
    WLESS_UART_sendString("VH State=");
#else
    WLESS_UART_sendString("ST State=");
#endif
    WLESS_UART_sendString(WLESS_UART_stateString(WLESS_SM_state));
    WLESS_UART_sendString(", Energy[Wh]=");
    WLESS_UART_sendInt(WLESS_SM_decodeCapacityWh(WLESS_SM_localEnergyEncoded));
    WLESS_UART_sendString(", Role=");
    WLESS_UART_sendString(WLESS_UART_roleString(WLESS_SM_localRole));
    WLESS_UART_sendString(", WPTState=");
    WLESS_UART_sendString((WLESS_SM_wptState == WLESS_SM_WPT_RUN) ? "RUN" : "STOP");
    WLESS_UART_sendString(", CtrlState=");
    WLESS_UART_sendString(WLESS_UART_ctrlString(WLESS_SM_localCtrlState));
    WLESS_UART_sendString(", Pwr2Ld=");
    WLESS_UART_sendInt(WLESS_SM_powerToLoad);
    WLESS_UART_sendString(", IcoilErr=");
    WLESS_UART_sendInt(WLESS_SM_iCoilErr);
    WLESS_UART_sendString(", AbortState=");
    WLESS_UART_sendString((WLESS_SM_localAbort == WLESS_SM_ABORT_ENABLED) ?
                          "ENABLED" : "DISABLED");
    WLESS_UART_sendString(", RadioLink=");
    WLESS_UART_sendString((WLESS_SM_radioLink == WLESS_SM_LINK_FAIL) ?
                          "FAIL" : "OK");
    WLESS_UART_sendString("\r\n");
}

static void WLESS_UART_sendVars(void)
{
    WLESS_UART_sendString("VARS,1,WH=");
    WLESS_UART_sendInt(WLESS_SM_decodeCapacityWh(WLESS_SM_localEnergyEncoded));
    WLESS_UART_sendString(", VB=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_vBus_V);
    WLESS_UART_sendString(", IB=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_iBat_mA);
    WLESS_UART_sendString(", IC=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_iCoil_mA);
    WLESS_UART_sendString(", VBMIN=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_vBusMin_V);
    WLESS_UART_sendString(", IBMIN=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_iBatMin_mA);
    WLESS_UART_sendString(", ICMIN=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_iCoilMin_mA);
    WLESS_UART_sendString(", MODE=");
    WLESS_UART_sendString((WLESS_SM_opMode == WLESS_SM_MODE_MANUAL) ?
                          "MANUAL" : "AUTO");
    WLESS_UART_sendString(", STOP=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_stopCommand);
    WLESS_UART_sendString(", INITOK=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_initOkCommand);
    WLESS_UART_sendString("\r\n");
}

static void WLESS_UART_sendRadio(void)
{
#if WLESS_NRF24_ENABLE == 1
    // Read live values so repeated RADIO? commands also exercise the SPI link.
    WLESS_NRF24_lastStatus = WLESS_NRF24_readRegister(0x07U);
    WLESS_NRF24_lastConfig = WLESS_NRF24_readRegister(0x00U);
    WLESS_NRF24_lastFifoStatus = WLESS_NRF24_readRegister(0x17U);
#endif
    WLESS_UART_sendString("RADIO,1,EN=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_ENABLE);
    WLESS_UART_sendString(", INIT=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_initOk);
    WLESS_UART_sendString(", INITERR=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_initFailMask);
    WLESS_UART_sendString(", STATUS=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_lastStatus);
    WLESS_UART_sendString(", CONFIG=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_lastConfig);
    WLESS_UART_sendString(", FIFO=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_lastFifoStatus);
    WLESS_UART_sendString(", IRQ=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_irqCount);
    WLESS_UART_sendString(", TX=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_txCount);
    WLESS_UART_sendString(", RX=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_rxCount);
    WLESS_UART_sendString(", ACK=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_ackCount);
    WLESS_UART_sendString(", MAXRT=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_maxRtCount);
    WLESS_UART_sendString(", RXVALID=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_validPayloadCount);
    WLESS_UART_sendString(", RXINVALID=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_invalidPayloadCount);
    WLESS_UART_sendString(", RXIRUN=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_invalidPayloadRunCount);
    WLESS_UART_sendString(", RXIMAX=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_invalidPayloadMaxRunCount);
    WLESS_UART_sendString(", TXBUSY=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_txBusy);
    WLESS_UART_sendString(", BUSYREJ=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_txBusyRejectCount);
    WLESS_UART_sendString(", FIFOREJ=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_txFifoRejectCount);
    WLESS_UART_sendString(", ACKP=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_ackRefreshPending);
    WLESS_UART_sendString(", TXSEQ=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_lastTxSequence);
    WLESS_UART_sendString(", RXSEQ=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_lastRxSequence);
    WLESS_UART_sendString(", NOACK=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_noAckCount);
    WLESS_UART_sendString(", NOACKMAX=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_noAckMaxCount);
    WLESS_UART_sendString(", TXP=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_lastTxPowerToLoad);
    WLESS_UART_sendString(", RXP=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_lastRxPowerToLoad);
    WLESS_UART_sendString(", TXP0=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_txPowerZeroLatched);
    WLESS_UART_sendString(", RXP0=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_rxPowerZeroLatched);
    WLESS_UART_sendString(", ATX=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_lastTxAppSequence);
    WLESS_UART_sendString(", ARX=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_lastRxAppSequence);
    WLESS_UART_sendString(", ADEL=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_lastRxAppSequenceDelta);
    WLESS_UART_sendString(", ACRC=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_appCrcErrorCount);
    WLESS_UART_sendString(", ASEQERR=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_appSequenceAnomalyCount);
    WLESS_UART_sendString(", ZSEQ=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_rxPowerZeroSequence);
    WLESS_UART_sendString(", ZDEL=");
    WLESS_UART_sendInt((int32_t)WLESS_NRF24_rxPowerZeroSequenceDelta);
    WLESS_UART_sendString(", REMOTE_E=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_remoteEnergyEncoded);
    WLESS_UART_sendString(", REMOTE_ROLE=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_remoteRole);
    WLESS_UART_sendString(", REMOTE_CTRL=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_remoteCtrlState);
    WLESS_UART_sendString(", REMOTE_ABORT=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_remoteAbort);
    WLESS_UART_sendString(", S=");
    WLESS_UART_sendInt((int32_t)WLESS_SM_stateStepCounter);
    WLESS_UART_sendString("\r\n");
}

static void WLESS_UART_setUnipdTestVector(float vDc)
{
    UNIPD_bbcPowerOutputEnable = 0U;
    UNIPD_bbcSyntheticInputs.v_dc = vDc;
    UNIPD_bbcSyntheticInputs.v_bat = 6.0f;
    UNIPD_bbcSyntheticInputs.i_l_a = 0.0f;
    UNIPD_bbcSyntheticInputs.i_l_b = 0.0f;
    UNIPD_bbcSyntheticInputs.i_coil_loc = 0.0f;
    UNIPD_bbcSyntheticInputs.i_coil_rem_err = 0.0f;
    UNIPD_bbcSyntheticInputs.i_coil_loc_rif = 0.0f;
    UNIPD_bbcSyntheticInputs.v_dc_pbat_rif = 10.0f;
    UNIPD_bbcSyntheticInputs.i_bat_rif_max = 2.0f;
    UNIPD_bbcSyntheticInputs.i_bat_rif_min = 0.0f;
    UNIPD_bbcSyntheticInputs.tx_1_rx_0 = 1U;
    UNIPD_bbcSyntheticValidMask = UNIPD_BBC_REQUIRED_SIGNAL_MASK;
    UNIPD_bbcSyntheticOverrideMask = UNIPD_BBC_REQUIRED_SIGNAL_MASK;
    UNIPD_bbcSyntheticTestEnable = 1U;
    UNIPD_resetControlStatesCommand = 1U;
}

static void WLESS_UART_sendUnipd(void)
{
    WLESS_UART_sendString("UQ,1,");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcSyntheticTestEnable);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcSignalValidMask);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcSignalMissingMask);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcInputs.v_dc * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcInputs.v_bat * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)TTPLPFC_vBatAdcRaw);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(TTPLPFC_vBatSensed_Volts * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcOutput.p_bat_rif * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcOutput.i_l_rif_a * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcInputs.i_l_a * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcInputs.i_l_b * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcInputs.i_coil_loc * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcOutput.duty_cycle_pwm_a * 1000000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcMappedDutyA_pu * 1000000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcAppliedDutyA_pu * 1000000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcOutput.abilita_pwm);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcPowerOutputEnable);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)TTPLPFC_bbcEnabled);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)TTPLPFC_bbcDockTestEnable);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcDutyMappingMode);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcPowerOutputDutyMax_pu * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcInputs.v_dc_pbat_rif * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcVdcTripLatched);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcVdcTripThreshold_Volts * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcILTripThreshold_Amps * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcILTripConfirmCycles);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcVdcTripCapture_Volts * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcILTripLatched);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcILTripThreshold_Amps * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcILTripCaptureA_Amps * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcILTripCaptureB_Amps * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_vdcControllerGainScale * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcRampedDutyA_pu * 1000000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcPowerOutputDutyRampStep_pu *
                                 1000000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)WLESS_UART_unipdVdcRefConfig);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)WLESS_UART_unipdIBatMaxMilliConfig);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)WLESS_UART_unipdVdcRefMilliConfig);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)WLESS_UART_unipdVBatMilliConfig);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcVdcTripThreshold_Volts * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)WLESS_UART_unipdHybridActive);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcSyntheticInputs.i_bat_rif_min *
                                 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcSyntheticInputs.i_bat_rif_max *
                                 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcOutput.duty_cycle_pwm_b *
                                 1000000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcMappedDutyB_pu * 1000000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcAppliedDutyB_pu * 1000000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcRampedDutyB_pu * 1000000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcDebugILerrA * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcDebugILerrB * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)UNIPD_bbcCurrentPolarityMask);
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcILRawA_Amps * 1000.0f));
    WLESS_UART_sendString(",");
    WLESS_UART_sendInt((int32_t)(UNIPD_bbcILRawB_Amps * 1000.0f));
    WLESS_UART_sendString("\r\n");
}

__interrupt void WLESS_UART_rxIsr(void)
{
    uint32_t status = SCI_getRxStatus(SCIA_BASE);

    if((status & SCI_RXSTATUS_ERROR) != 0UL)
    {
        WLESS_UART_rxErrorCount++;
        SCI_clearOverflowStatus(SCIA_BASE);
        SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_RXERR);
    }

    while(SCI_getRxFIFOStatus(SCIA_BASE) != SCI_FIFO_RX0)
    {
        uint16_t data = SCI_readCharNonBlocking(SCIA_BASE);
        WLESS_UART_rxByteCount++;
        if(!WLESS_RB_write(&WLESS_UART_rxBuffer, data))
        {
            WLESS_UART_rxOverflowCount++;
        }
    }

    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_RXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}
