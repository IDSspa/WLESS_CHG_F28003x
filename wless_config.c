#include <string.h>

#include "driverlib.h"
#include "device.h"
#include "FlashAPI/F021.h"
#include "wless_config.h"
#include "clllc/clllc.h"
#include "ttplpfc/ttplpfc.h"

#define WLESS_CONFIG_FLASH_A_ADDRESS       0x09E000UL
#define WLESS_CONFIG_FLASH_B_ADDRESS       0x09F000UL
#define WLESS_CONFIG_MAGIC_0               0x574CUL
#define WLESS_CONFIG_MAGIC_1               0x4346UL
#define WLESS_CONFIG_COMMIT                0xA55AU
#define WLESS_CONFIG_RECORD_WORDS          40U
#define WLESS_CONFIG_CRC_INDEX             30U
#define WLESS_CONFIG_COMMIT_INDEX          32U
#define WLESS_CONFIG_FLASH_TIMEOUT         2000000UL

#define WLESS_CONFIG_DEFAULT_ITANK_OFFSET_MV       120U
#define WLESS_CONFIG_DEFAULT_ITANK_SENS_MV_A       573U
#define WLESS_CONFIG_DEFAULT_ITANK_LIMIT_MA        5000U
#define WLESS_CONFIG_DEFAULT_ILA_OFFSET_RAW        2048U
#define WLESS_CONFIG_DEFAULT_ILB_OFFSET_RAW        2048U

WLESS_Config WLESS_Config_active;
WLESS_Config WLESS_Config_pending;
uint16_t WLESS_Config_source;
uint32_t WLESS_Config_sequence;

static uint16_t WLESS_Config_crc16(const uint16_t *data, uint16_t words)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;
    uint16_t bit;

    for(i = 0U; i < words; i++)
    {
        crc ^= data[i];
        for(bit = 0U; bit < 16U; bit++)
        {
            crc = (crc & 1U) != 0U ?
                    (uint16_t)((crc >> 1U) ^ 0xA001U) :
                    (uint16_t)(crc >> 1U);
        }
    }
    return crc;
}

static void WLESS_Config_encode(uint16_t *record, uint32_t sequence)
{
    uint16_t i;

    for(i = 0U; i < WLESS_CONFIG_RECORD_WORDS; i++)
    {
        record[i] = 0xFFFFU;
    }
    record[0] = WLESS_CONFIG_MAGIC_0;
    record[1] = WLESS_CONFIG_MAGIC_1;
    record[2] = WLESS_CONFIG_SCHEMA_VERSION;
    record[3] = WLESS_CONFIG_RECORD_WORDS;
    record[4] = (uint16_t)sequence;
    record[5] = (uint16_t)(sequence >> 16U);
    record[6] = WLESS_Config_active.iTankOffset_mV;
    record[7] = WLESS_Config_active.iTankSensitivity_mV_A;
    record[8] = WLESS_Config_active.iTankLimit_mA;
    record[9] = WLESS_Config_active.iLAOffsetRaw;
    record[10] = WLESS_Config_active.iLBOffsetRaw;
    record[WLESS_CONFIG_CRC_INDEX] =
            WLESS_Config_crc16(record, WLESS_CONFIG_CRC_INDEX);
    record[WLESS_CONFIG_COMMIT_INDEX] = WLESS_CONFIG_COMMIT;
}

static uint16_t WLESS_Config_decode(uint32_t address, WLESS_Config *config,
                                    uint32_t *sequence)
{
    const uint16_t *record = (const uint16_t *)address;

    if((record[0] != WLESS_CONFIG_MAGIC_0) ||
       (record[1] != WLESS_CONFIG_MAGIC_1) ||
       (record[2] != WLESS_CONFIG_SCHEMA_VERSION) ||
       (record[3] != WLESS_CONFIG_RECORD_WORDS) ||
       (record[WLESS_CONFIG_COMMIT_INDEX] != WLESS_CONFIG_COMMIT) ||
       (record[WLESS_CONFIG_CRC_INDEX] !=
               WLESS_Config_crc16(record, WLESS_CONFIG_CRC_INDEX)))
    {
        return 0U;
    }

    config->iTankOffset_mV = record[6];
    config->iTankSensitivity_mV_A = record[7];
    config->iTankLimit_mA = record[8];
    config->iLAOffsetRaw = record[9];
    config->iLBOffsetRaw = record[10];
    *sequence = (uint32_t)record[4] | ((uint32_t)record[5] << 16U);
    return WLESS_Config_validate(config);
}

static uint16_t WLESS_Config_sequenceNewer(uint32_t a, uint32_t b)
{
    return ((int32_t)(a - b) > 0) ? 1U : 0U;
}

static void WLESS_Config_applyToControl(void)
{
    float32_t offsetPu;
    float32_t ampsPerPu;

    offsetPu = ((float32_t)WLESS_Config_active.iTankOffset_mV) / 3300.0f;
    ampsPerPu = 3300.0f /
            (float32_t)WLESS_Config_active.iTankSensitivity_mV_A;
    CLLLC_iTankModSensedOffset_pu = offsetPu;
    CLLLC_iTankModAmpsPerPu = ampsPerPu;
    CLLLC_iTankModLimit_Amps =
            ((float32_t)WLESS_Config_active.iTankLimit_mA) * 0.001f;

    TTPLPFC_iL1_senseOffset_pu =
            ((float32_t)WLESS_Config_active.iLAOffsetRaw) / 4096.0f;
    TTPLPFC_iL2_senseOffset_pu =
            ((float32_t)WLESS_Config_active.iLBOffsetRaw) / 4096.0f;
}

void WLESS_Config_loadDefaults(void)
{
    WLESS_Config_pending.iTankOffset_mV =
            WLESS_CONFIG_DEFAULT_ITANK_OFFSET_MV;
    WLESS_Config_pending.iTankSensitivity_mV_A =
            WLESS_CONFIG_DEFAULT_ITANK_SENS_MV_A;
    WLESS_Config_pending.iTankLimit_mA =
            WLESS_CONFIG_DEFAULT_ITANK_LIMIT_MA;
    WLESS_Config_pending.iLAOffsetRaw =
            WLESS_CONFIG_DEFAULT_ILA_OFFSET_RAW;
    WLESS_Config_pending.iLBOffsetRaw =
            WLESS_CONFIG_DEFAULT_ILB_OFFSET_RAW;
}

uint16_t WLESS_Config_validate(const WLESS_Config *config)
{
    if((config->iTankOffset_mV > 500U) ||
       (config->iTankSensitivity_mV_A < 100U) ||
       (config->iTankSensitivity_mV_A > 1000U) ||
       (config->iTankLimit_mA == 0U) ||
       (config->iTankLimit_mA > 5000U) ||
       (config->iLAOffsetRaw < 1500U) ||
       (config->iLAOffsetRaw > 2600U) ||
       (config->iLBOffsetRaw < 1500U) ||
       (config->iLBOffsetRaw > 2600U))
    {
        return 0U;
    }
    return 1U;
}

uint16_t WLESS_Config_apply(void)
{
    if(WLESS_Config_validate(&WLESS_Config_pending) == 0U)
    {
        return 0U;
    }
    WLESS_Config_active = WLESS_Config_pending;
    WLESS_Config_applyToControl();
    return 1U;
}

uint16_t WLESS_Config_reload(void)
{
    WLESS_Config configA;
    WLESS_Config configB;
    uint32_t sequenceA = 0UL;
    uint32_t sequenceB = 0UL;
    uint16_t validA;
    uint16_t validB;

    validA = WLESS_Config_decode(WLESS_CONFIG_FLASH_A_ADDRESS,
                                 &configA, &sequenceA);
    validB = WLESS_Config_decode(WLESS_CONFIG_FLASH_B_ADDRESS,
                                 &configB, &sequenceB);
    if((validA == 0U) && (validB == 0U))
    {
        WLESS_Config_loadDefaults();
        WLESS_Config_active = WLESS_Config_pending;
        WLESS_Config_source = WLESS_CONFIG_SOURCE_DEFAULT;
        WLESS_Config_sequence = 0UL;
        WLESS_Config_applyToControl();
        return 0U;
    }
    if((validB != 0U) &&
       ((validA == 0U) || (WLESS_Config_sequenceNewer(sequenceB,
                                                       sequenceA) != 0U)))
    {
        WLESS_Config_active = configB;
        WLESS_Config_source = WLESS_CONFIG_SOURCE_FLASH_B;
        WLESS_Config_sequence = sequenceB;
    }
    else
    {
        WLESS_Config_active = configA;
        WLESS_Config_source = WLESS_CONFIG_SOURCE_FLASH_A;
        WLESS_Config_sequence = sequenceA;
    }
    WLESS_Config_pending = WLESS_Config_active;
    WLESS_Config_applyToControl();
    return 1U;
}

void WLESS_Config_init(void)
{
    (void)WLESS_Config_reload();
}

void WLESS_Config_applyCurrentOffsets(uint16_t iLAOffsetRaw,
                                      uint16_t iLBOffsetRaw)
{
    WLESS_Config_active.iLAOffsetRaw = iLAOffsetRaw;
    WLESS_Config_active.iLBOffsetRaw = iLBOffsetRaw;
    WLESS_Config_pending.iLAOffsetRaw = iLAOffsetRaw;
    WLESS_Config_pending.iLBOffsetRaw = iLBOffsetRaw;
}

#pragma CODE_SECTION(WLESS_Config_save, ".TI.ramfunc");
uint16_t WLESS_Config_save(void)
{
    uint16_t record[WLESS_CONFIG_RECORD_WORDS];
    uint32_t targetAddress;
    uint32_t nextSequence;
    uint16_t index;
    uint32_t timeout;
    bool interruptsWereDisabled;
    Fapi_StatusType status;

    if(WLESS_Config_validate(&WLESS_Config_active) == 0U)
    {
        return 0U;
    }

    targetAddress = (WLESS_Config_source == WLESS_CONFIG_SOURCE_FLASH_A) ?
            WLESS_CONFIG_FLASH_B_ADDRESS : WLESS_CONFIG_FLASH_A_ADDRESS;
    nextSequence = WLESS_Config_sequence + 1UL;
    WLESS_Config_encode(record, nextSequence);

    interruptsWereDisabled = Interrupt_disableGlobal();
    status = Fapi_initializeAPI(F021_CPU0_BASE_ADDRESS,
                                (uint32_t)(DEVICE_SYSCLK_FREQ / 1000000UL));
    if(status == Fapi_Status_Success)
    {
        status = Fapi_setActiveFlashBank(Fapi_FlashBank1);
    }
    if(status == Fapi_Status_Success)
    {
        status = Fapi_issueAsyncCommandWithAddress(
                Fapi_EraseSector, (uint32_t *)targetAddress);
        timeout = WLESS_CONFIG_FLASH_TIMEOUT;
        while((Fapi_checkFsmForReady() != Fapi_Status_FsmReady) &&
              (timeout != 0UL))
        {
            timeout--;
        }
        if((timeout == 0UL) || (Fapi_getFsmStatus() != 0U))
        {
            status = Fapi_Error_Fail;
        }
    }

    for(index = 0U;
        (status == Fapi_Status_Success) &&
        (index < WLESS_CONFIG_COMMIT_INDEX);
        index += 8U)
    {
        uint16_t count = (uint16_t)(WLESS_CONFIG_COMMIT_INDEX - index);
        if(count > 8U)
        {
            count = 8U;
        }
        status = Fapi_issueProgrammingCommand(
                (uint32_t *)(targetAddress + index),
                &record[index], count, 0, 0, Fapi_AutoEccGeneration);
        timeout = WLESS_CONFIG_FLASH_TIMEOUT;
        while((Fapi_checkFsmForReady() != Fapi_Status_FsmReady) &&
              (timeout != 0UL))
        {
            timeout--;
        }
        if((timeout == 0UL) || (Fapi_getFsmStatus() != 0U))
        {
            status = Fapi_Error_Fail;
        }
    }

    if(status == Fapi_Status_Success)
    {
        status = Fapi_issueProgrammingCommand(
                (uint32_t *)(targetAddress + WLESS_CONFIG_COMMIT_INDEX),
                &record[WLESS_CONFIG_COMMIT_INDEX], 1U,
                0, 0, Fapi_AutoEccGeneration);
        timeout = WLESS_CONFIG_FLASH_TIMEOUT;
        while((Fapi_checkFsmForReady() != Fapi_Status_FsmReady) &&
              (timeout != 0UL))
        {
            timeout--;
        }
        if((timeout == 0UL) || (Fapi_getFsmStatus() != 0U))
        {
            status = Fapi_Error_Fail;
        }
    }
    if(interruptsWereDisabled == false)
    {
        (void)Interrupt_enableGlobal();
    }

    if((status != Fapi_Status_Success) ||
       (WLESS_Config_decode(targetAddress, &WLESS_Config_pending,
                            &nextSequence) == 0U))
    {
        return 0U;
    }
    WLESS_Config_active = WLESS_Config_pending;
    WLESS_Config_sequence = nextSequence;
    WLESS_Config_source =
            (targetAddress == WLESS_CONFIG_FLASH_A_ADDRESS) ?
            WLESS_CONFIG_SOURCE_FLASH_A : WLESS_CONFIG_SOURCE_FLASH_B;
    return 1U;
}
