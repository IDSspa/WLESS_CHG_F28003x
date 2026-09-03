#include <stdbool.h>
#include <stddef.h>

#include "driverlib.h"
#include "device.h"
#include "wless_sm/wless_sm.h"
#include "wless_nrf24.h"

#define NRF_CMD_R_REGISTER        0x00U
#define NRF_CMD_W_REGISTER        0x20U
#define NRF_CMD_ACTIVATE          0x50U
#define NRF_CMD_FLUSH_TX          0xE1U
#define NRF_CMD_FLUSH_RX          0xE2U
#define NRF_CMD_NOP               0xFFU
#define NRF_CMD_R_RX_PAYLOAD      0x61U
#define NRF_CMD_W_TX_PAYLOAD      0xA0U
#define NRF_CMD_W_ACK_PAYLOAD_P0  0xA8U

#define NRF_REG_CONFIG            0x00U
#define NRF_REG_EN_AA             0x01U
#define NRF_REG_EN_RXADDR         0x02U
#define NRF_REG_SETUP_AW          0x03U
#define NRF_REG_SETUP_RETR        0x04U
#define NRF_REG_RF_CH             0x05U
#define NRF_REG_RF_SETUP          0x06U
#define NRF_REG_STATUS            0x07U
#define NRF_REG_RX_ADDR_P0        0x0AU
#define NRF_REG_TX_ADDR           0x10U
#define NRF_REG_RX_PW_P0          0x11U
#define NRF_REG_FIFO_STATUS       0x17U
#define NRF_REG_DYNPD             0x1CU
#define NRF_REG_FEATURE           0x1DU

#define NRF_CONFIG_RX             0x3BU
#define NRF_CONFIG_TX             0x2AU
#define NRF_STATUS_IRQ_MASK       0x70U
#define NRF_STATUS_RX_DR          0x40U
#define NRF_STATUS_TX_DS          0x20U
#define NRF_STATUS_MAX_RT         0x10U
#define NRF_FIFO_TX_FULL          0x20U
#define NRF_FIFO_TX_EMPTY         0x10U
#define NRF_FIFO_RX_EMPTY         0x01U
#define NRF_PAYLOAD_LENGTH        13U
#define NRF_MAX_TRANSFER          32U

volatile uint16_t WLESS_NRF24_initOk;
volatile uint16_t WLESS_NRF24_irqPending;
volatile uint32_t WLESS_NRF24_irqCount;
volatile uint8_t WLESS_NRF24_lastStatus;
volatile uint8_t WLESS_NRF24_lastConfig;
volatile uint8_t WLESS_NRF24_lastFifoStatus;
volatile uint32_t WLESS_NRF24_txCount;
volatile uint32_t WLESS_NRF24_rxCount;
volatile uint32_t WLESS_NRF24_ackCount;
volatile uint32_t WLESS_NRF24_maxRtCount;
volatile uint32_t WLESS_NRF24_validPayloadCount;
volatile uint32_t WLESS_NRF24_invalidPayloadCount;
volatile uint16_t WLESS_NRF24_invalidPayloadRunCount;
volatile uint16_t WLESS_NRF24_invalidPayloadMaxRunCount;
volatile uint16_t WLESS_NRF24_txBusy;
volatile uint32_t WLESS_NRF24_txBusyRejectCount;
volatile uint32_t WLESS_NRF24_txFifoRejectCount;
volatile uint16_t WLESS_NRF24_ackRefreshPending;
volatile uint16_t WLESS_NRF24_lastTxSequence;
volatile uint16_t WLESS_NRF24_lastRxSequence;
volatile uint32_t WLESS_NRF24_initFailMask;
volatile int16_t WLESS_NRF24_lastTxPowerToLoad;
volatile int16_t WLESS_NRF24_lastRxPowerToLoad;
volatile uint16_t WLESS_NRF24_txPowerZeroLatched;
volatile uint16_t WLESS_NRF24_rxPowerZeroLatched;
volatile uint16_t WLESS_NRF24_lastTxAppSequence;
volatile uint16_t WLESS_NRF24_lastRxAppSequence;
volatile uint16_t WLESS_NRF24_lastRxAppSequenceDelta;
volatile uint32_t WLESS_NRF24_appCrcErrorCount;
volatile uint32_t WLESS_NRF24_appSequenceAnomalyCount;
volatile uint16_t WLESS_NRF24_rxPowerZeroSequence;
volatile uint16_t WLESS_NRF24_rxPowerZeroSequenceDelta;
static uint16_t WLESS_NRF24_txPowerValidSeen;
static uint16_t WLESS_NRF24_rxPowerValidSeen;
static uint16_t WLESS_NRF24_payloadTxSequence;
static uint16_t WLESS_NRF24_rxAppSequenceSeen;

static uint8_t WLESS_NRF24_command(uint8_t command,
                                   const uint8_t *txData,
                                   uint8_t *rxData,
                                   uint16_t length);

static uint16_t WLESS_NRF24_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFFU;
    uint16_t index;
    uint16_t bit;

    for(index = 0U; index < length; index++)
    {
        crc ^= (uint16_t)data[index] << 8U;
        for(bit = 0U; bit < 8U; bit++)
        {
            crc = ((crc & 0x8000U) != 0U) ?
                    (uint16_t)((crc << 1U) ^ 0x1021U) :
                    (uint16_t)(crc << 1U);
        }
    }
    return crc;
}

static void WLESS_NRF24_recordInvalidPayload(void)
{
    WLESS_NRF24_invalidPayloadCount++;
    if(WLESS_NRF24_invalidPayloadRunCount < UINT16_MAX)
    {
        WLESS_NRF24_invalidPayloadRunCount++;
    }
    if(WLESS_NRF24_invalidPayloadRunCount >
       WLESS_NRF24_invalidPayloadMaxRunCount)
    {
        WLESS_NRF24_invalidPayloadMaxRunCount =
                WLESS_NRF24_invalidPayloadRunCount;
    }
}

static void WLESS_NRF24_makeOperationalPayload(uint8_t *payload)
{
    /*
     * Acquisire una sola volta ogni variabile condivisa. ISR1 puo' aggiornare
     * le grandezze di controllo mentre il background serializza il payload:
     * senza snapshot i byte alto e basso potrebbero appartenere a due campioni
     * diversi. I singoli accessi a 16 bit sono atomici sul C28x.
     */
    const int16_t energy = WLESS_SM_localEnergyEncoded;
    const uint16_t role = (uint16_t)WLESS_SM_localRole;
    const uint16_t ctrlState = (uint16_t)WLESS_SM_localCtrlState;
    const int16_t powerToLoad = WLESS_SM_powerToLoad;
    const int16_t iCoilErr = WLESS_SM_iCoilErr;
    const uint16_t abortState = (uint16_t)WLESS_SM_localAbort;
    const uint16_t sequence = ++WLESS_NRF24_payloadTxSequence;
    uint16_t crc;

    WLESS_NRF24_lastTxPowerToLoad = powerToLoad;
    if(powerToLoad >= 10)
    {
        WLESS_NRF24_txPowerValidSeen = 1U;
    }
    else if((WLESS_NRF24_txPowerValidSeen != 0U) && (powerToLoad == 0))
    {
        WLESS_NRF24_txPowerZeroLatched = 1U;
    }

    payload[0] = (uint8_t)((uint16_t)energy >> 8U);
    payload[1] = (uint8_t)energy;
    payload[2] = (uint8_t)role;
    payload[3] = (uint8_t)ctrlState;
    payload[4] = (uint8_t)((uint16_t)powerToLoad >> 8U);
    payload[5] = (uint8_t)powerToLoad;
    payload[6] = (uint8_t)((uint16_t)iCoilErr >> 8U);
    payload[7] = (uint8_t)iCoilErr;
    payload[8] = (uint8_t)abortState;
    payload[9] = (uint8_t)(sequence >> 8U);
    payload[10] = (uint8_t)sequence;
    crc = WLESS_NRF24_crc16(payload, 11U);
    payload[11] = (uint8_t)(crc >> 8U);
    payload[12] = (uint8_t)crc;
    WLESS_NRF24_lastTxAppSequence = sequence;
}

static bool WLESS_NRF24_consumeOperationalPayload(const uint8_t *payload)
{
    const int16_t energy =
        (int16_t)(((uint16_t)payload[0] << 8U) | payload[1]);
    const uint16_t role = payload[2];
    const uint16_t ctrlState = payload[3];
    const int16_t powerToLoad =
        (int16_t)(((uint16_t)payload[4] << 8U) | payload[5]);
    const int16_t iCoilErr =
        (int16_t)(((uint16_t)payload[6] << 8U) | payload[7]);
    const uint16_t abortState = payload[8];
    const uint16_t sequence =
        (uint16_t)(((uint16_t)payload[9] << 8U) | payload[10]);
    const uint16_t receivedCrc =
        (uint16_t)(((uint16_t)payload[11] << 8U) | payload[12]);
    const uint16_t calculatedCrc = WLESS_NRF24_crc16(payload, 11U);
    uint16_t sequenceDelta = 0U;

    if(receivedCrc != calculatedCrc)
    {
        WLESS_NRF24_appCrcErrorCount++;
        WLESS_NRF24_recordInvalidPayload();
        return false;
    }

    if(WLESS_NRF24_rxAppSequenceSeen != 0U)
    {
        sequenceDelta = (uint16_t)(sequence - WLESS_NRF24_lastRxAppSequence);
        if(sequenceDelta != 1U)
        {
            WLESS_NRF24_appSequenceAnomalyCount++;
        }
    }
    WLESS_NRF24_lastRxAppSequenceDelta = sequenceDelta;
    WLESS_NRF24_lastRxAppSequence = sequence;
    WLESS_NRF24_rxAppSequenceSeen = 1U;

    /*
     * Il CRC hardware nRF protegge il tratto radio, ma non la successiva
     * lettura SPI e la decodifica applicativa. Pubblicare il campione solo se
     * tutti i campi enumerativi hanno un valore ammesso evita che un payload
     * impossibile alteri parzialmente la state machine o il controllo UniPD.
     */
    if((role > (uint16_t)WLESS_SM_ROLE_LOAD) ||
       (ctrlState > (uint16_t)WLESS_SM_CTRL_WPTERR) ||
       (abortState > (uint16_t)WLESS_SM_ABORT_ENABLED))
    {
        WLESS_NRF24_recordInvalidPayload();
        return false;
    }

    WLESS_NRF24_invalidPayloadRunCount = 0U;
    WLESS_NRF24_lastRxPowerToLoad = powerToLoad;
    if(powerToLoad >= 10)
    {
        WLESS_NRF24_rxPowerValidSeen = 1U;
    }
    else if((WLESS_NRF24_rxPowerValidSeen != 0U) && (powerToLoad == 0))
    {
        WLESS_NRF24_rxPowerZeroLatched = 1U;
        WLESS_NRF24_rxPowerZeroSequence = sequence;
        WLESS_NRF24_rxPowerZeroSequenceDelta = sequenceDelta;
    }
    WLESS_SM_remoteEnergyEncoded = energy;
    WLESS_SM_remoteRole = (WLESS_SM_Role)role;
    WLESS_SM_remoteCtrlState = (WLESS_SM_ControllerState)ctrlState;
    /*
     * Conservare separati il comando locale trasmesso e il limite ricevuto.
     * La FSM LOAD azzera legittimamente WLESS_SM_powerToLoad nei propri stati;
     * usare la stessa variabile per il dato remoto cancellava il limite SOURCE
     * tra due payload e rendeva pulsante il controllo distribuito.
     */
    WLESS_SM_remotePowerToLoad = powerToLoad;
    WLESS_SM_remoteICoilErr = iCoilErr;
    WLESS_SM_remoteAbort = (WLESS_SM_Abort)abortState;
    WLESS_NRF24_validPayloadCount++;
    return true;
}

void WLESS_NRF24_resetPowerTrace(void)
{
    WLESS_NRF24_lastTxPowerToLoad = 0;
    WLESS_NRF24_lastRxPowerToLoad = 0;
    WLESS_NRF24_txPowerZeroLatched = 0U;
    WLESS_NRF24_rxPowerZeroLatched = 0U;
    WLESS_NRF24_txPowerValidSeen = 0U;
    WLESS_NRF24_rxPowerValidSeen = 0U;
    WLESS_NRF24_lastRxAppSequenceDelta = 0U;
    WLESS_NRF24_appCrcErrorCount = 0UL;
    WLESS_NRF24_appSequenceAnomalyCount = 0UL;
    WLESS_NRF24_rxPowerZeroSequence = 0U;
    WLESS_NRF24_rxPowerZeroSequenceDelta = 0U;
    WLESS_NRF24_rxAppSequenceSeen = 0U;
}

static void WLESS_NRF24_loadAckPayload(uint16_t sequence)
{
    uint8_t payload[NRF_PAYLOAD_LENGTH];
    (void)sequence;
    WLESS_NRF24_makeOperationalPayload(payload);
    (void)WLESS_NRF24_command(NRF_CMD_W_ACK_PAYLOAD_P0,
                              payload, NULL, NRF_PAYLOAD_LENGTH);
}

static void WLESS_NRF24_ce(bool high)
{
    GPIO_writePin(WLESS_NRF24_GPIO_CE, high ? 1U : 0U);
}

static uint8_t WLESS_NRF24_command(uint8_t command,
                                    const uint8_t *txData,
                                    uint8_t *rxData,
                                    uint16_t length)
{
    uint16_t i;
    uint16_t transferLength = length + 1U;
    uint8_t received;
    uint8_t status = 0xFFU;

    if(transferLength > NRF_MAX_TRANSFER)
    {
        transferLength = NRF_MAX_TRANSFER;
    }

    SPI_resetTxFIFO(SPIA_BASE);
    SPI_resetRxFIFO(SPIA_BASE);
    SPI_writeDataNonBlocking(SPIA_BASE, ((uint16_t)command << 8U));
    for(i = 1U; i < transferLength; i++)
    {
        uint8_t value = (txData != NULL) ? txData[i - 1U] : NRF_CMD_NOP;
        SPI_writeDataNonBlocking(SPIA_BASE, ((uint16_t)value << 8U));
    }
    while((uint16_t)SPI_getRxFIFOStatus(SPIA_BASE) < transferLength)
    {
    }
    for(i = 0U; i < transferLength; i++)
    {
        received = (uint8_t)(SPI_readDataNonBlocking(SPIA_BASE) & 0x00FFU);
        if(i == 0U)
        {
            status = received;
        }
        else if(rxData != NULL)
        {
            rxData[i - 1U] = received;
        }
    }
    return status;
}

static void WLESS_NRF24_writeRegister(uint8_t reg, uint8_t value)
{
    (void)WLESS_NRF24_command((uint8_t)(NRF_CMD_W_REGISTER | (reg & 0x1FU)),
                              &value, NULL, 1U);
}

static bool WLESS_NRF24_writeVerify(uint8_t reg, uint8_t value)
{
    WLESS_NRF24_writeRegister(reg, value);
    return WLESS_NRF24_readRegister(reg) == value;
}

static bool WLESS_NRF24_writeAddressVerify(uint8_t reg)
{
    static const uint8_t address[3] = {0xE7U, 0xE7U, 0xE7U};
    uint8_t readback[3];

    (void)WLESS_NRF24_command((uint8_t)(NRF_CMD_W_REGISTER | reg),
                              address, NULL, 3U);
    (void)WLESS_NRF24_command((uint8_t)(NRF_CMD_R_REGISTER | reg),
                              NULL, readback, 3U);
    return (readback[0] == address[0]) &&
           (readback[1] == address[1]) &&
           (readback[2] == address[2]);
}

uint8_t WLESS_NRF24_readRegister(uint8_t reg)
{
    uint8_t value;
    (void)WLESS_NRF24_command((uint8_t)(NRF_CMD_R_REGISTER | (reg & 0x1FU)),
                              NULL, &value, 1U);
    return value;
}

static void WLESS_NRF24_initGpioSpiXint(void)
{
    GPIO_setPinConfig(GPIO_16_SPIA_SIMO);
    GPIO_setPinConfig(GPIO_17_SPIA_SOMI);
    GPIO_setPinConfig(GPIO_56_SPIA_CLK);
    GPIO_setPinConfig(GPIO_57_SPIA_STE);
    GPIO_setPinConfig(GPIO_61_GPIO61);
    GPIO_setPinConfig(GPIO_33_GPIO33);

    GPIO_setPadConfig(WLESS_NRF24_GPIO_MOSI, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(WLESS_NRF24_GPIO_MISO, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
    GPIO_setPadConfig(WLESS_NRF24_GPIO_SCLK, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(WLESS_NRF24_GPIO_MOSI, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(WLESS_NRF24_GPIO_MISO, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(WLESS_NRF24_GPIO_SCLK, GPIO_QUAL_ASYNC);

    GPIO_setPadConfig(WLESS_NRF24_GPIO_CSN, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(WLESS_NRF24_GPIO_CSN, GPIO_QUAL_ASYNC);

    WLESS_NRF24_ce(false);
    GPIO_setPadConfig(WLESS_NRF24_GPIO_CE, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(WLESS_NRF24_GPIO_CE, GPIO_DIR_MODE_OUT);
    GPIO_setControllerCore(WLESS_NRF24_GPIO_CE, GPIO_CORE_CPU1);

    GPIO_setPadConfig(WLESS_NRF24_GPIO_IRQ, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(WLESS_NRF24_GPIO_IRQ, GPIO_QUAL_3SAMPLE);
    GPIO_setDirectionMode(WLESS_NRF24_GPIO_IRQ, GPIO_DIR_MODE_IN);
    GPIO_setControllerCore(WLESS_NRF24_GPIO_IRQ, GPIO_CORE_CPU1);

    SPI_disableModule(SPIA_BASE);
    SPI_setConfig(SPIA_BASE, DEVICE_LSPCLK_FREQ, SPI_PROT_POL0PHA1,
                  SPI_MODE_CONTROLLER, WLESS_NRF24_SPI_BITRATE, 8U);
    SPI_setPTESignalPolarity(SPIA_BASE, SPI_PTE_ACTIVE_LOW);
    SPI_enableFIFO(SPIA_BASE);
    SPI_resetTxFIFO(SPIA_BASE);
    SPI_resetRxFIFO(SPIA_BASE);
    SPI_disableLoopback(SPIA_BASE);
    SPI_setEmulationMode(SPIA_BASE, SPI_EMULATION_STOP_AFTER_TRANSMIT);
    SPI_enableModule(SPIA_BASE);

    GPIO_setInterruptPin(WLESS_NRF24_GPIO_IRQ, GPIO_INT_XINT1);
    GPIO_setInterruptType(GPIO_INT_XINT1, GPIO_INT_TYPE_FALLING_EDGE);
    GPIO_disableInterrupt(GPIO_INT_XINT1);
    Interrupt_register(INT_XINT1, &WLESS_NRF24_irqIsr);
    Interrupt_enable(INT_XINT1);
}

void WLESS_NRF24_init(void)
{
    bool ok = true;
    uint8_t activate = 0x73U;
    uint8_t config;

    WLESS_NRF24_initOk = 0U;
    WLESS_NRF24_irqPending = 0U;
    WLESS_NRF24_irqCount = 0UL;
    WLESS_NRF24_lastStatus = 0U;
    WLESS_NRF24_lastConfig = 0U;
    WLESS_NRF24_lastFifoStatus = 0U;
    WLESS_NRF24_txCount = 0UL;
    WLESS_NRF24_rxCount = 0UL;
    WLESS_NRF24_ackCount = 0UL;
    WLESS_NRF24_maxRtCount = 0UL;
    WLESS_NRF24_validPayloadCount = 0UL;
    WLESS_NRF24_invalidPayloadCount = 0UL;
    WLESS_NRF24_invalidPayloadRunCount = 0U;
    WLESS_NRF24_invalidPayloadMaxRunCount = 0U;
    WLESS_NRF24_txBusy = 0U;
    WLESS_NRF24_txBusyRejectCount = 0UL;
    WLESS_NRF24_txFifoRejectCount = 0UL;
    WLESS_NRF24_ackRefreshPending = 0U;
    WLESS_NRF24_lastTxSequence = 0U;
    WLESS_NRF24_lastRxSequence = 0U;
    WLESS_NRF24_initFailMask = 0UL;
    WLESS_NRF24_lastTxAppSequence = 0U;
    WLESS_NRF24_lastRxAppSequence = 0U;
    WLESS_NRF24_lastRxAppSequenceDelta = 0U;
    WLESS_NRF24_appCrcErrorCount = 0UL;
    WLESS_NRF24_appSequenceAnomalyCount = 0UL;
    WLESS_NRF24_rxPowerZeroSequence = 0U;
    WLESS_NRF24_rxPowerZeroSequenceDelta = 0U;
    WLESS_NRF24_payloadTxSequence = 0U;
    WLESS_NRF24_rxAppSequenceSeen = 0U;

    WLESS_NRF24_initGpioSpiXint();
    DEVICE_DELAY_US(20000U);

    (void)WLESS_NRF24_command(NRF_CMD_ACTIVATE, &activate, NULL, 1U);
    if(!WLESS_NRF24_writeVerify(NRF_REG_DYNPD, 0x01U)) WLESS_NRF24_initFailMask |= (1UL << 0U);
    if(!WLESS_NRF24_writeVerify(NRF_REG_FEATURE, 0x06U)) WLESS_NRF24_initFailMask |= (1UL << 1U);
    if(!WLESS_NRF24_writeVerify(NRF_REG_EN_AA, 0x01U)) WLESS_NRF24_initFailMask |= (1UL << 2U);
    if(!WLESS_NRF24_writeVerify(NRF_REG_EN_RXADDR, 0x01U)) WLESS_NRF24_initFailMask |= (1UL << 3U);
    if(!WLESS_NRF24_writeVerify(NRF_REG_SETUP_AW, 0x01U)) WLESS_NRF24_initFailMask |= (1UL << 4U);
    if(!WLESS_NRF24_writeVerify(NRF_REG_SETUP_RETR, 0x10U)) WLESS_NRF24_initFailMask |= (1UL << 5U);
    if(!WLESS_NRF24_writeVerify(NRF_REG_RF_CH, 0x02U)) WLESS_NRF24_initFailMask |= (1UL << 6U);
    if(!WLESS_NRF24_writeVerify(NRF_REG_RF_SETUP, 0x08U)) WLESS_NRF24_initFailMask |= (1UL << 7U);
    if(!WLESS_NRF24_writeAddressVerify(NRF_REG_RX_ADDR_P0)) WLESS_NRF24_initFailMask |= (1UL << 8U);
#if WLESS_SM_BUILD_VEHICLE == 0
    if(!WLESS_NRF24_writeAddressVerify(NRF_REG_TX_ADDR)) WLESS_NRF24_initFailMask |= (1UL << 9U);
#endif
    if(!WLESS_NRF24_writeVerify(NRF_REG_RX_PW_P0, NRF_PAYLOAD_LENGTH)) WLESS_NRF24_initFailMask |= (1UL << 10U);

    config = (WLESS_SM_BUILD_VEHICLE == 1) ? NRF_CONFIG_TX : NRF_CONFIG_RX;
    if(!WLESS_NRF24_writeVerify(NRF_REG_CONFIG, config)) WLESS_NRF24_initFailMask |= (1UL << 11U);
    ok = (WLESS_NRF24_initFailMask == 0UL);
    WLESS_NRF24_writeRegister(NRF_REG_STATUS, NRF_STATUS_IRQ_MASK);
    (void)WLESS_NRF24_command(NRF_CMD_FLUSH_TX, NULL, NULL, 0U);
    (void)WLESS_NRF24_command(NRF_CMD_FLUSH_RX, NULL, NULL, 0U);

    WLESS_NRF24_lastStatus = WLESS_NRF24_readRegister(NRF_REG_STATUS);
    WLESS_NRF24_lastConfig = WLESS_NRF24_readRegister(NRF_REG_CONFIG);
    WLESS_NRF24_lastFifoStatus = WLESS_NRF24_readRegister(NRF_REG_FIFO_STATUS);
    WLESS_NRF24_initOk = ok ? 1U : 0U;

    if(ok)
    {
#if WLESS_SM_BUILD_VEHICLE == 0
        WLESS_NRF24_loadAckPayload(0U);
#endif
        WLESS_NRF24_ce(true);
        GPIO_enableInterrupt(GPIO_INT_XINT1);
    }
}

void WLESS_NRF24_service(void)
{
    // The original ISR sampled STATUS immediately. In this port SPI is kept
    // outside the ISR, therefore also service an IRQ that is still asserted
    // low even if no new XINT edge can be generated.
    if((WLESS_NRF24_irqPending != 0U) ||
       (GPIO_readPin(WLESS_NRF24_GPIO_IRQ) == 0U)
#if WLESS_SM_BUILD_VEHICLE == 1
       || (WLESS_NRF24_txBusy != 0U)
#endif
      )
    {
        WLESS_NRF24_irqPending = 0U;
        uint8_t payload[NRF_PAYLOAD_LENGTH];
        WLESS_NRF24_lastStatus = WLESS_NRF24_readRegister(NRF_REG_STATUS);
        WLESS_NRF24_lastFifoStatus = WLESS_NRF24_readRegister(NRF_REG_FIFO_STATUS);
        /*
         * Acknowledge only the flags captured above, and do it before the
         * potentially longer FIFO/ACK service. Any event asserted afterwards
         * remains set and causes a subsequent pass instead of being lost.
         */
        if((WLESS_NRF24_lastStatus & NRF_STATUS_IRQ_MASK) != 0U)
        {
            WLESS_NRF24_writeRegister(
                NRF_REG_STATUS,
                (uint8_t)(WLESS_NRF24_lastStatus & NRF_STATUS_IRQ_MASK));
        }
        if(((WLESS_NRF24_lastStatus & NRF_STATUS_RX_DR) != 0U) ||
           ((WLESS_NRF24_lastFifoStatus & NRF_FIFO_RX_EMPTY) == 0U))
        {
            uint16_t payloads = 0U;
            do
            {
                (void)WLESS_NRF24_command(NRF_CMD_R_RX_PAYLOAD, NULL,
                                          payload, NRF_PAYLOAD_LENGTH);
#if WLESS_SM_BUILD_VEHICLE == 1
                if(WLESS_NRF24_consumeOperationalPayload(payload))
                {
                    WLESS_SM_noAckCount = 0U;
                    WLESS_NRF24_ackCount++;
                    WLESS_NRF24_lastRxSequence++;
                }
#else
                if(WLESS_NRF24_consumeOperationalPayload(payload))
                {
                    WLESS_SM_noAckCount = 0U;
                    WLESS_NRF24_rxCount++;
                    WLESS_NRF24_lastRxSequence++;
                }
#endif
                payloads++;
                WLESS_NRF24_lastFifoStatus =
                    WLESS_NRF24_readRegister(NRF_REG_FIFO_STATUS);
            }
            while(((WLESS_NRF24_lastFifoStatus & NRF_FIFO_RX_EMPTY) == 0U) &&
                  (payloads < 3U));
#if WLESS_SM_BUILD_VEHICLE == 0
            /*
             * Il payload ACK usato per questa ricezione era gia' nel FIFO.
             * Dopo aver svuotato RX, verificare lo slot e accodare direttamente
             * il campione successivo. In PRX non possiamo dipendere da TX_DS:
             * sul banco il flag non viene osservato per l'ACK automatico e il
             * refresh resterebbe pendente fino all'esaurimento del FIFO.
             * Nessun CE toggle e nessun FLUSH_TX durante il traffico.
             */
            WLESS_NRF24_ackRefreshPending = 1U;
            WLESS_NRF24_lastFifoStatus =
                WLESS_NRF24_readRegister(NRF_REG_FIFO_STATUS);
            if((WLESS_NRF24_lastFifoStatus & NRF_FIFO_TX_FULL) == 0U)
            {
                WLESS_NRF24_loadAckPayload(0U);
                WLESS_NRF24_ackRefreshPending = 0U;
            }
            else
            {
                WLESS_NRF24_txFifoRejectCount++;
            }
#endif
        }
#if WLESS_SM_BUILD_VEHICLE == 1
        /*
         * In PTX mode an empty TX FIFO while our single in-flight flag is set
         * is also definitive evidence of a completed, acknowledged transfer:
         * the radio removes the payload from TX FIFO only after success. This
         * recovers safely if TX_DS was cleared or was not observed by software.
         */
        if((WLESS_NRF24_txBusy != 0U) &&
           ((WLESS_NRF24_lastStatus & NRF_STATUS_MAX_RT) == 0U) &&
           (((WLESS_NRF24_lastStatus & NRF_STATUS_TX_DS) != 0U) ||
            ((WLESS_NRF24_lastFifoStatus & NRF_FIFO_TX_EMPTY) != 0U)))
        {
            WLESS_NRF24_txCount++;
            WLESS_NRF24_txBusy = 0U;
        }
#else
        if((WLESS_NRF24_lastStatus & NRF_STATUS_TX_DS) != 0U)
        {
            WLESS_NRF24_txCount++;
        }
#endif
        if((WLESS_NRF24_lastStatus & NRF_STATUS_MAX_RT) != 0U)
        {
            WLESS_NRF24_maxRtCount++;
#if WLESS_SM_BUILD_VEHICLE == 1
            if(WLESS_SM_noAckCount < 15U)
            {
                WLESS_SM_noAckCount++;
                if(WLESS_SM_noAckCount > WLESS_SM_noAckMaxCount)
                {
                    WLESS_SM_noAckMaxCount = WLESS_SM_noAckCount;
                }
            }
            WLESS_NRF24_txBusy = 0U;
#endif
            (void)WLESS_NRF24_command(NRF_CMD_FLUSH_TX, NULL, NULL, 0U);
        }
    }

#if WLESS_SM_BUILD_VEHICLE == 1
    if((WLESS_SM_operationMessagePending != 0U) &&
       (WLESS_NRF24_txBusy == 0U))
    {
        /*
         * Clear before starting SPI. If ISR2 requests a newer sample while
         * this payload is being built/queued, it sets the flag again and that
         * request is preserved. Restore it if the radio did not accept TX.
         */
        WLESS_SM_operationMessagePending = 0U;
        if(!WLESS_NRF24_sendDiagnosticPing())
        {
            WLESS_SM_operationMessagePending = 1U;
        }
    }
#endif
}

bool WLESS_NRF24_sendDiagnosticPing(void)
{
#if WLESS_SM_BUILD_VEHICLE == 1
    uint8_t payload[NRF_PAYLOAD_LENGTH];
    if(WLESS_NRF24_initOk == 0U) return false;

    if(WLESS_NRF24_txBusy != 0U)
    {
        WLESS_NRF24_txBusyRejectCount++;
        return false;
    }

    WLESS_NRF24_lastFifoStatus =
        WLESS_NRF24_readRegister(NRF_REG_FIFO_STATUS);
    if((WLESS_NRF24_lastFifoStatus & NRF_FIFO_TX_FULL) != 0U)
    {
        WLESS_NRF24_txFifoRejectCount++;
        return false;
    }

    WLESS_NRF24_makeOperationalPayload(payload);
    (void)WLESS_NRF24_command(NRF_CMD_W_TX_PAYLOAD, payload, NULL,
                              NRF_PAYLOAD_LENGTH);
    WLESS_NRF24_txBusy = 1U;
    WLESS_NRF24_lastTxSequence++;
    return true;
#else
    return false;
#endif
}

__interrupt void WLESS_NRF24_irqIsr(void)
{
    WLESS_NRF24_irqPending = 1U;
    WLESS_NRF24_irqCount++;
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
