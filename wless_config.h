#ifndef WLESS_CONFIG_H
#define WLESS_CONFIG_H

#include <stdint.h>

/* Limiti della catena ADC ITANK_MOD, espressi in unita' fisiche. */
#define WLESS_CONFIG_ITANK_ADC_FULL_SCALE_MV       3300U
#define WLESS_CONFIG_ITANK_OFFSET_MAX_MV             500U
/* 1 mV/A impedisce soltanto la divisione per zero; il controllo span ADC
 * sottostante limita ogni combinazione sensibilita'/corrente non fisica. */
#define WLESS_CONFIG_ITANK_SENSITIVITY_MIN_MV_A       1U
#define WLESS_CONFIG_ITANK_SENSITIVITY_MAX_MV_A    1000U
#define WLESS_CONFIG_ITANK_LIMIT_MAX_MA           37500U

/* Incrementare quando cambiano i default compilati: un record flash di schema
 * precedente non deve prevalere sulla calibrazione associata alla nuova build. */
#define WLESS_CONFIG_SCHEMA_VERSION       2U
#define WLESS_CONFIG_SOURCE_DEFAULT       0U
#define WLESS_CONFIG_SOURCE_FLASH_A       1U
#define WLESS_CONFIG_SOURCE_FLASH_B       2U

typedef struct
{
    uint16_t iTankOffset_mV;
    uint16_t iTankSensitivity_mV_A;
    uint16_t iTankLimit_mA;
    uint16_t iLAOffsetRaw;
    uint16_t iLBOffsetRaw;
    uint16_t wptLoadCoilOffset_mA;
} WLESS_Config;

extern WLESS_Config WLESS_Config_active;
extern WLESS_Config WLESS_Config_pending;
extern uint16_t WLESS_Config_source;
extern uint32_t WLESS_Config_sequence;

void WLESS_Config_init(void);
uint16_t WLESS_Config_validate(const WLESS_Config *config);
uint16_t WLESS_Config_apply(void);
void WLESS_Config_loadDefaults(void);
uint16_t WLESS_Config_reload(void);
uint16_t WLESS_Config_save(void);
void WLESS_Config_applyCurrentOffsets(uint16_t iLAOffsetRaw,
                                      uint16_t iLBOffsetRaw);

#endif
