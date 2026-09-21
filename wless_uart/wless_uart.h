#ifndef WLESS_UART_H_
#define WLESS_UART_H_

#include <stdint.h>

#define WLESS_UART_ENABLE          1
#define WLESS_UART_BAUDRATE        115200UL
#define WLESS_UART_CMD_MAX_LEN     16U

/*
 * Temporary BOOST diagnosis: in UH=1, allow UW=<mV> to replace only the
 * VBAT input of UniPD.  VDC and both BBC branch-current inputs remain
 * physical, so this cannot turn the power-stage test into a full synthetic
 * simulation.  Set back to 0 after the VIN scaling investigation.
 */
#define WLESS_UART_UNIPD_VBAT_DIAG_OVERRIDE_ENABLE   1U

extern const int FIRMWARE_RELEASE;

extern volatile uint32_t WLESS_UART_rxByteCount;
extern volatile uint32_t WLESS_UART_rxErrorCount;
extern volatile uint32_t WLESS_UART_rxOverflowCount;
extern volatile uint32_t WLESS_UART_commandCount;
extern volatile uint32_t WLESS_UART_commandErrorCount;

void WLESS_UART_init(void);
void WLESS_UART_process(void);
void WLESS_UART_sendStatus(void);
void WLESS_UART_sendByte(uint16_t data);
void WLESS_UART_sendString(const char *str);
void WLESS_UART_sendInt(int32_t value);

#if WLESS_UART_ENABLE == 0
#define WLESS_UART_INIT_IF_ENABLED()       ((void)0)
#define WLESS_UART_PROCESS_IF_ENABLED()    ((void)0)
#else
#define WLESS_UART_INIT_IF_ENABLED()       WLESS_UART_init()
#define WLESS_UART_PROCESS_IF_ENABLED()    WLESS_UART_process()
#endif

#endif
