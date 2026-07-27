#ifndef WLESS_NRF24_H_
#define WLESS_NRF24_H_

#include <stdbool.h>
#include <stdint.h>
#include "wless_nrf24_settings.h"

extern volatile uint16_t WLESS_NRF24_initOk;
extern volatile uint16_t WLESS_NRF24_irqPending;
extern volatile uint32_t WLESS_NRF24_irqCount;
extern volatile uint8_t WLESS_NRF24_lastStatus;
extern volatile uint8_t WLESS_NRF24_lastConfig;
extern volatile uint8_t WLESS_NRF24_lastFifoStatus;
extern volatile uint32_t WLESS_NRF24_txCount;
extern volatile uint32_t WLESS_NRF24_rxCount;
extern volatile uint32_t WLESS_NRF24_ackCount;
extern volatile uint32_t WLESS_NRF24_maxRtCount;
extern volatile uint16_t WLESS_NRF24_lastTxSequence;
extern volatile uint16_t WLESS_NRF24_lastRxSequence;
extern volatile uint32_t WLESS_NRF24_initFailMask;

void WLESS_NRF24_init(void);
void WLESS_NRF24_service(void);
uint8_t WLESS_NRF24_readRegister(uint8_t reg);
bool WLESS_NRF24_sendDiagnosticPing(void);
void WLESS_NRF24_irqIsr(void);

#if WLESS_NRF24_ENABLE == 1
#define WLESS_NRF24_INIT_IF_ENABLED()     WLESS_NRF24_init()
#define WLESS_NRF24_SERVICE_IF_ENABLED()  WLESS_NRF24_service()
#else
#define WLESS_NRF24_INIT_IF_ENABLED()     ((void)0)
#define WLESS_NRF24_SERVICE_IF_ENABLED()  ((void)0)
#endif

#endif
