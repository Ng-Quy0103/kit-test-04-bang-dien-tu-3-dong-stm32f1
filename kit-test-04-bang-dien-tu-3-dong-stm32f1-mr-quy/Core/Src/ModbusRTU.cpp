#include "ModbusRTU.h"

// ============================================================================
// DINH NGHIA MA HAM & MA LOI MODBUS 
// ============================================================================
enum MbFunc {
    MB_FUNC_READ_HOLDING   = 0x03,
    MB_FUNC_READ_INPUT     = 0x04,
    MB_FUNC_WRITE_SINGLE   = 0x06,
    MB_FUNC_WRITE_MULTIPLE = 0x10
};

enum MbException {
    MB_EX_ILLEGAL_FUNCTION  = 0x01,
    MB_EX_ILLEGAL_DATA_ADDR = 0x02,
    MB_EX_ILLEGAL_DATA_VAL  = 0x03
};

ModbusRTU::ModbusRTU()
    : m_ui(0),
      m_rxLen3(0),
      m_frameReceived3(false),
      m_rxLen2(0),
      m_frameReceived2(false),
      m_packetCounter(0),
      m_lastPacketCount(0),
      m_lastFpsTime(0),
      m_currentFps(0)
{
    for (uint16_t i = 0; i < REG_COUNT; i++) {
        m_holdingRegisters[i] = 0;
    }
}

// ============================================================================
// KHOI TAO DONG THOI CA 2 CONG RS485 TREN BO MACH STM32F103RCT6:
//   - Cong 1 (USART3): PB10 (TX), PB11 (RX), PA6 (DE), DMA1_Ch2 (TX), DMA1_Ch3 (RX)
//   - Cong 2 (USART2): PA2  (TX), PA3  (RX), PA1 (DE), DMA1_Ch7 (TX), DMA1_Ch6 (RX)
// ============================================================================
void ModbusRTU::init(SystemUI* uiPtr, uint32_t nowMs) {
    m_ui             = uiPtr;
    m_lastFpsTime    = nowMs;
    m_rxLen3         = 0;
    m_frameReceived3 = false;
    m_rxLen2         = 0;
    m_frameReceived2 = false;

    // 1. Bat Clock cho GPIOA, GPIOB, AFIO, DMA1, USART2 va USART3
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    RCC->AHBENR  |= RCC_AHBENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN;

    // Lay xung nhip APB1 thuc te (36MHz khi chay HSE 72MHz, hoac 32MHz khi fallback HSI 64MHz)
    uint32_t pclk = (SystemCoreClock >= 36000000UL) ? (SystemCoreClock / 2UL) : 36000000UL;
    uint32_t brr  = (pclk + (BAUDRATE / 2UL)) / BAUDRATE;

    // ========================================================================
    // CAU HINH CONG RS485 SO 1: USART3 (PB10/PB11, DE = PA6)
    // ========================================================================
    // PA6 (DE/RE): Output Push-Pull 50MHz (0x3 tai bit 24..27 cua GPIOA->CRL)
    GPIOA->CRL &= ~(0xFUL << 24);
    GPIOA->CRL |=  (0x3UL << 24);
    setModeRX3();

    // PB10 (USART3_TX): AF Push-Pull 50MHz (0xB), PB11 (USART3_RX): Input Floating (0x4)
    GPIOB->CRH &= ~((0xFUL << 8) | (0xFUL << 12));
    GPIOB->CRH |=  ((0xBUL << 8) | (0x4UL << 12));

    USART3->BRR = brr;
    USART3->CR2 = 0;
    USART3->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_IDLEIE;
    USART3->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;

    // DMA1_Channel3 cho USART3_RX
    DMA1_Channel3->CCR  &= ~DMA_CCR_EN;
    DMA1_Channel3->CPAR  = (uint32_t)&(USART3->DR);
    DMA1_Channel3->CMAR  = (uint32_t)m_rxBuf3;
    DMA1_Channel3->CNDTR = BUF_SIZE;
    DMA1_Channel3->CCR   = DMA_CCR_PL_1 | DMA_CCR_MINC | DMA_CCR_EN;

    // DMA1_Channel2 cho USART3_TX
    DMA1_Channel2->CCR  &= ~DMA_CCR_EN;
    DMA1_Channel2->CPAR  = (uint32_t)&(USART3->DR);
    DMA1_Channel2->CMAR  = (uint32_t)m_txBuf3;
    DMA1_Channel2->CCR   = DMA_CCR_PL_0 | DMA_CCR_MINC | DMA_CCR_DIR;

    NVIC_SetPriority(USART3_IRQn, 0);
    NVIC_EnableIRQ(USART3_IRQn);
    NVIC_SetPriority(DMA1_Channel2_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);

    volatile uint32_t tmp = USART3->SR;
    tmp = USART3->DR;
    USART3->CR1 |= USART_CR1_UE;

    // ========================================================================
    // CAU HINH CONG RS485 SO 2: USART2 (PA2/PA3, DE = PA5 - Domino A2, B2)
    // ========================================================================
    // PA5 (DE/RE): Output Push-Pull 50MHz (0x3 tai bit 20..23), PA2 (TX): AF Push-Pull (0xB tai bit 8..11), PA3 (RX): Input Floating (0x4 tai bit 12..15)
    GPIOA->CRL &= ~((0xFUL << 8) | (0xFUL << 12) | (0xFUL << 20));
    GPIOA->CRL |=  ((0xBUL << 8) | (0x4UL << 12) | (0x3UL << 20));
    setModeRX2();

    USART2->BRR = brr;
    USART2->CR2 = 0;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_IDLEIE;
    USART2->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;

    // DMA1_Channel6 cho USART2_RX
    DMA1_Channel6->CCR  &= ~DMA_CCR_EN;
    DMA1_Channel6->CPAR  = (uint32_t)&(USART2->DR);
    DMA1_Channel6->CMAR  = (uint32_t)m_rxBuf2;
    DMA1_Channel6->CNDTR = BUF_SIZE;
    DMA1_Channel6->CCR   = DMA_CCR_PL_1 | DMA_CCR_MINC | DMA_CCR_EN;

    // DMA1_Channel7 cho USART2_TX
    DMA1_Channel7->CCR  &= ~DMA_CCR_EN;
    DMA1_Channel7->CPAR  = (uint32_t)&(USART2->DR);
    DMA1_Channel7->CMAR  = (uint32_t)m_txBuf2;
    DMA1_Channel7->CCR   = DMA_CCR_PL_0 | DMA_CCR_MINC | DMA_CCR_DIR;

    NVIC_SetPriority(USART2_IRQn, 0);
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_SetPriority(DMA1_Channel7_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);

    tmp = USART2->SR;
    tmp = USART2->DR;
    (void)tmp;
    USART2->CR1 |= USART_CR1_UE;

    // Dong bo trang thai ban dau tu SystemUI vao bang thanh ghi Modbus
    syncFromUI();
}

// ============================================================================
// CAC HAM TRO GIUP CRC16, CHUYEN DOI GIO CA LAM VIEC & KIEM TRA GIOI HAN
// ============================================================================
uint16_t ModbusRTU::calculateCRC(const uint8_t* buf, uint16_t len) {
    uint16_t crc = 0xFFFFU;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1U) {
                crc >>= 1;
                crc ^= 0xA001U;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

uint16_t ModbusRTU::minToHhmm(uint16_t minFromMidnight) {
    uint16_t m = minFromMidnight % 1440U;
    return (uint16_t)((m / 60U) * 100U + (m % 60U));
}

bool ModbusRTU::hhmmToMin(uint16_t val, uint16_t& outMin) {
    uint16_t hh = val / 100U;
    uint16_t mm = val % 100U;
    if (hh <= 23U && mm <= 59U) {
        outMin = (uint16_t)(hh * 60U + mm);
        return true;
    }
    if (val < 1440U) {
        outMin = val;
        return true;
    }
    return false;
}

bool ModbusRTU::checkLimit(uint16_t addr, uint16_t val) {
    switch (addr) {
        case REG_YEAR:
            return (val >= 2000U && val <= 2099U) || (val <= 99U);
        case REG_MONTH:
            return (val >= 1U && val <= 12U);
        case REG_DATE:
            return (val >= 1U && val <= 31U);
        case REG_HOUR:
            return (val <= 23U);
        case REG_MINUTE:
        case REG_SECOND:
            return (val <= 59U);
        case REG_S1_START:
        case REG_S1_END:
        case REG_S2_START:
        case REG_S2_END:
        case REG_S3_START:
        case REG_S3_END: {
            uint16_t dummy = 0;
            return hhmmToMin(val, dummy);
        }
        case REG_FACTOR:
            return (val >= 1U && val <= 999U);
        default:
            return true;
    }
}

// ============================================================================
// GUI KHUNG PHAN HOI BANG DMA TREN CONG TUONG UNG (USART3 hoac USART2)
// ============================================================================
void ModbusRTU::sendDMA(uint8_t port, uint8_t* buf, uint16_t len) {
    if (port == 3) {
        setModeTX3(); // PA6 = 1
        for (volatile int i = 0; i < 100; i++) __NOP();

        USART3->SR = ~USART_SR_TC;
        DMA1->IFCR = (0xFUL << 4); // Clear flags Channel 2

        DMA1_Channel2->CCR  &= ~DMA_CCR_EN;
        DMA1_Channel2->CMAR  = (uint32_t)buf;
        DMA1_Channel2->CNDTR = len;
        DMA1_Channel2->CCR  |= DMA_CCR_TCIE | DMA_CCR_EN;
    } else {
        setModeTX2(); // PA1 = 1
        for (volatile int i = 0; i < 100; i++) __NOP();

        USART2->SR = ~USART_SR_TC;
        DMA1->IFCR = (0xFUL << 24); // Clear flags Channel 7

        DMA1_Channel7->CCR  &= ~DMA_CCR_EN;
        DMA1_Channel7->CMAR  = (uint32_t)buf;
        DMA1_Channel7->CNDTR = len;
        DMA1_Channel7->CCR  |= DMA_CCR_TCIE | DMA_CCR_EN;
    }
}

void ModbusRTU::sendException(uint8_t port, uint8_t* txBuf, uint8_t func, uint8_t code) {
    txBuf[0] = SLAVE_ID;
    txBuf[1] = (uint8_t)(func | 0x80U);
    txBuf[2] = code;
    uint16_t crc = calculateCRC(txBuf, 3);
    txBuf[3] = (uint8_t)(crc & 0xFFU);
    txBuf[4] = (uint8_t)(crc >> 8);
    sendDMA(port, txBuf, 5);
}

// ============================================================================
// DONG BO DU LIEU GIUA SystemUI VA BANG THANH GHI MODBUS (0..19)
// ============================================================================
void ModbusRTU::syncFromUI() {
    if (!m_ui) return;

    const RtcTime& clk = m_ui->getClock();

    m_holdingRegisters[REG_FPS]           = m_currentFps;
    m_holdingRegisters[REG_YEAR]          = (uint16_t)(2000U + clk.year);
    m_holdingRegisters[REG_MONTH]         = clk.month;
    m_holdingRegisters[REG_DATE]          = clk.date;
    m_holdingRegisters[REG_HOUR]          = clk.hour;
    m_holdingRegisters[REG_MINUTE]        = clk.minute;
    m_holdingRegisters[REG_SECOND]        = clk.second;
    m_holdingRegisters[REG_TARGET_DAY]    = (uint16_t)(m_ui->getTargetDay() > 65535UL ? 65535U : m_ui->getTargetDay());
    m_holdingRegisters[REG_TARGET_NOW]    = (uint16_t)(m_ui->getTargetNow() > 65535UL ? 65535U : m_ui->getTargetNow());
    m_holdingRegisters[REG_ACTUAL_PROD]   = (uint16_t)(m_ui->getCount()     > 65535UL ? 65535U : m_ui->getCount());
    m_holdingRegisters[REG_S1_START]      = minToHhmm(m_ui->getShiftStart(0));
    m_holdingRegisters[REG_S1_END]        = minToHhmm(m_ui->getShiftEnd(0));
    m_holdingRegisters[REG_S2_START]      = minToHhmm(m_ui->getShiftStart(1));
    m_holdingRegisters[REG_S2_END]        = minToHhmm(m_ui->getShiftEnd(1));
    m_holdingRegisters[REG_S3_START]      = minToHhmm(m_ui->getShiftStart(2));
    m_holdingRegisters[REG_S3_END]        = minToHhmm(m_ui->getShiftEnd(2));
    m_holdingRegisters[REG_FACTOR]        = m_ui->getFactor();
    m_holdingRegisters[REG_TOTAL_PACKETS] = (uint16_t)(m_packetCounter & 0xFFFFU);
}

void ModbusRTU::applyToUI(uint16_t addr, uint16_t val) {
    if (!m_ui) return;

    switch (addr) {
        case REG_YEAR: {
            uint8_t y = (val >= 2000U) ? (uint8_t)(val - 2000U) : (uint8_t)val;
            RtcTime t = m_ui->getClock();
            if (t.year != y) { t.year = y; m_ui->setTime(t); }
            break;
        }
        case REG_MONTH: {
            RtcTime t = m_ui->getClock();
            if (t.month != (uint8_t)val) { t.month = (uint8_t)val; m_ui->setTime(t); }
            break;
        }
        case REG_DATE: {
            RtcTime t = m_ui->getClock();
            if (t.date != (uint8_t)val) { t.date = (uint8_t)val; m_ui->setTime(t); }
            break;
        }
        case REG_HOUR: {
            RtcTime t = m_ui->getClock();
            if (t.hour != (uint8_t)val) { t.hour = (uint8_t)val; m_ui->setTime(t); }
            break;
        }
        case REG_MINUTE: {
            RtcTime t = m_ui->getClock();
            if (t.minute != (uint8_t)val) { t.minute = (uint8_t)val; m_ui->setTime(t); }
            break;
        }
        case REG_SECOND: {
            RtcTime t = m_ui->getClock();
            if (t.second != (uint8_t)val) { t.second = (uint8_t)val; m_ui->setTime(t); }
            break;
        }
        case REG_TARGET_DAY:
            if (val != (uint16_t)m_ui->getTargetDay()) {
                m_ui->setTargetDay(val);
            }
            break;

        case REG_ACTUAL_PROD:
            if (val != (uint16_t)m_ui->getCount()) {
                m_ui->setCount(val);
            }
            break;

        case REG_S1_START: {
            uint16_t m = 0;
            if (hhmmToMin(val, m) && m != m_ui->getShiftStart(0)) m_ui->setShiftStart(0, m);
            break;
        }
        case REG_S1_END: {
            uint16_t m = 0;
            if (hhmmToMin(val, m) && m != m_ui->getShiftEnd(0)) m_ui->setShiftEnd(0, m);
            break;
        }
        case REG_S2_START: {
            uint16_t m = 0;
            if (hhmmToMin(val, m) && m != m_ui->getShiftStart(1)) m_ui->setShiftStart(1, m);
            break;
        }
        case REG_S2_END: {
            uint16_t m = 0;
            if (hhmmToMin(val, m) && m != m_ui->getShiftEnd(1)) m_ui->setShiftEnd(1, m);
            break;
        }
        case REG_S3_START: {
            uint16_t m = 0;
            if (hhmmToMin(val, m) && m != m_ui->getShiftStart(2)) m_ui->setShiftStart(2, m);
            break;
        }
        case REG_S3_END: {
            uint16_t m = 0;
            if (hhmmToMin(val, m) && m != m_ui->getShiftEnd(2)) m_ui->setShiftEnd(2, m);
            break;
        }
        case REG_FACTOR:
            if (val != m_ui->getFactor()) {
                m_ui->setFactor(val);
            }
            break;

        case REG_TOTAL_PACKETS:
            if (val == 0) {
                m_packetCounter   = 0;
                m_lastPacketCount = 0;
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// XU LY VONG LAP CHINH & GIAI MA KHUNG MODBUS RTU TREN CA 2 CONG
// ============================================================================
void ModbusRTU::process(uint32_t nowMs) {
    // 1. Moi 1000ms (1 giay) chot toc do truyen thong FPS (Frame per second)
    if ((uint32_t)(nowMs - m_lastFpsTime) >= 1000UL) {
        m_lastFpsTime     = nowMs;
        uint32_t c        = m_packetCounter;
        m_currentFps      = (uint16_t)(c - m_lastPacketCount);
        m_lastPacketCount = c;
    }

    // 2. Kiem tra khung nhan tren Cong 1 (USART3: PB10/PB11, DE = PA6)
    if (m_frameReceived3) {
        syncFromUI();
        processFrame(3, m_rxBuf3, m_rxLen3, m_txBuf3);
        m_frameReceived3 = false;

        if ((DMA1_Channel3->CCR & DMA_CCR_EN) == 0) {
            DMA1_Channel3->CNDTR = BUF_SIZE;
            DMA1_Channel3->CCR  |= DMA_CCR_EN;
        }
    }

    // 3. Kiem tra khung nhan tren Cong 2 (USART2: PA2/PA3, DE = PA1)
    if (m_frameReceived2) {
        syncFromUI();
        processFrame(2, m_rxBuf2, m_rxLen2, m_txBuf2);
        m_frameReceived2 = false;

        if ((DMA1_Channel6->CCR & DMA_CCR_EN) == 0) {
            DMA1_Channel6->CNDTR = BUF_SIZE;
            DMA1_Channel6->CCR  |= DMA_CCR_EN;
        }
    }
}

void ModbusRTU::processFrame(uint8_t port, const uint8_t* rxBuf, uint16_t rxLen, uint8_t* txBuf) {
    // Kiem tra do dai toi thieu va Slave ID = 5 (hoac ID = 1)
    if (rxLen < 4) return;
    uint8_t reqId = rxBuf[0];
    if (reqId != SLAVE_ID && reqId != 1) return;

    // Kiem tra CRC16
    uint16_t receivedCRC = ((uint16_t)rxBuf[rxLen - 1] << 8) | rxBuf[rxLen - 2];
    if (receivedCRC != calculateCRC(rxBuf, rxLen - 2)) return;

    uint8_t  funcCode  = rxBuf[1];
    uint16_t startAddr = ((uint16_t)rxBuf[2] << 8) | rxBuf[3];

    switch (funcCode) {
        case MB_FUNC_READ_HOLDING:
        case MB_FUNC_READ_INPUT: {
            if (rxLen < 8) return;
            uint16_t count = ((uint16_t)rxBuf[4] << 8) | rxBuf[5];
            if (count > 0 && (startAddr + count) <= REG_COUNT) {
                m_packetCounter++;
                m_holdingRegisters[REG_TOTAL_PACKETS] = (uint16_t)(m_packetCounter & 0xFFFFU);
                m_holdingRegisters[REG_FPS]           = m_currentFps;

                txBuf[0] = reqId;
                txBuf[1] = funcCode;
                txBuf[2] = (uint8_t)(count * 2);
                for (uint16_t i = 0; i < count; i++) {
                    txBuf[3 + i * 2]     = (uint8_t)(m_holdingRegisters[startAddr + i] >> 8);
                    txBuf[3 + i * 2 + 1] = (uint8_t)(m_holdingRegisters[startAddr + i] & 0xFFU);
                }
                uint16_t txLen = 3 + count * 2;
                uint16_t crc   = calculateCRC(txBuf, txLen);
                txBuf[txLen]     = (uint8_t)(crc & 0xFFU);
                txBuf[txLen + 1] = (uint8_t)(crc >> 8);
                sendDMA(port, txBuf, txLen + 2);
            } else {
                sendException(port, txBuf, funcCode, MB_EX_ILLEGAL_DATA_ADDR);
            }
            break;
        }

        case MB_FUNC_WRITE_SINGLE: {
            if (rxLen < 8) return;
            uint16_t val = ((uint16_t)rxBuf[4] << 8) | rxBuf[5];
            if (startAddr < REG_COUNT && checkLimit(startAddr, val)) {
                m_holdingRegisters[startAddr] = val;
                m_packetCounter++;

                for (int i = 0; i < 8; i++) {
                    txBuf[i] = rxBuf[i];
                }
                sendDMA(port, txBuf, 8);

                applyToUI(startAddr, val);
                if (m_ui) m_ui->beep();
            } else {
                sendException(port, txBuf, funcCode, (startAddr < REG_COUNT) ? MB_EX_ILLEGAL_DATA_VAL : MB_EX_ILLEGAL_DATA_ADDR);
            }
            break;
        }

        case MB_FUNC_WRITE_MULTIPLE: {
            if (rxLen < 9) return;
            uint16_t count = ((uint16_t)rxBuf[4] << 8) | rxBuf[5];
            if (count > 0 && (startAddr + count) <= REG_COUNT && rxLen >= (9 + count * 2)) {
                bool dataValid = true;
                for (uint16_t i = 0; i < count; i++) {
                    uint16_t val = ((uint16_t)rxBuf[7 + i * 2] << 8) | rxBuf[7 + i * 2 + 1];
                    if (!checkLimit(startAddr + i, val)) {
                        dataValid = false;
                        break;
                    }
                }
                if (dataValid) {
                    m_packetCounter++;

                    txBuf[0] = reqId;
                    txBuf[1] = MB_FUNC_WRITE_MULTIPLE;
                    txBuf[2] = rxBuf[2];
                    txBuf[3] = rxBuf[3];
                    txBuf[4] = rxBuf[4];
                    txBuf[5] = rxBuf[5];
                    uint16_t crc = calculateCRC(txBuf, 6);
                    txBuf[6] = (uint8_t)(crc & 0xFFU);
                    txBuf[7] = (uint8_t)(crc >> 8);
                    sendDMA(port, txBuf, 8);

                    for (uint16_t i = 0; i < count; i++) {
                        uint16_t val = ((uint16_t)rxBuf[7 + i * 2] << 8) | rxBuf[7 + i * 2 + 1];
                        m_holdingRegisters[startAddr + i] = val;
                        applyToUI(startAddr + i, val);
                    }
                    if (m_ui) m_ui->beep();
                } else {
                    sendException(port, txBuf, funcCode, MB_EX_ILLEGAL_DATA_VAL);
                }
            } else {
                sendException(port, txBuf, funcCode, MB_EX_ILLEGAL_DATA_ADDR);
            }
            break;
        }

        default:
            sendException(port, txBuf, funcCode, MB_EX_ILLEGAL_FUNCTION);
            break;
    }
}

// ============================================================================
// TRINH PHUC VU NGAT CHO CA USART3 VA USART2
// ============================================================================
void ModbusRTU::irqHandlerIdle3() {
    if (USART3->SR & USART_SR_IDLE) {
        volatile uint32_t tmp = USART3->SR;
        tmp = USART3->DR;
        (void)tmp;

        DMA1_Channel3->CCR &= ~DMA_CCR_EN;
        uint16_t received = BUF_SIZE - DMA1_Channel3->CNDTR;
        if (received > 0) {
            m_rxLen3         = received;
            m_frameReceived3 = true;
        }

        DMA1_Channel3->CNDTR = BUF_SIZE;
        DMA1_Channel3->CCR  |= DMA_CCR_EN;
    }
}

void ModbusRTU::irqHandlerTx3() {
    uint32_t tcifFlag = (1UL << 5); // TCIF2 (DMA1_Channel2)
    if (DMA1->ISR & tcifFlag) {
        DMA1->IFCR = tcifFlag;
        DMA1_Channel2->CCR &= ~DMA_CCR_EN;

        while (!(USART3->SR & USART_SR_TC));
        setModeRX3();
    }
}

void ModbusRTU::irqHandlerIdle2() {
    if (USART2->SR & USART_SR_IDLE) {
        volatile uint32_t tmp = USART2->SR;
        tmp = USART2->DR;
        (void)tmp;

        DMA1_Channel6->CCR &= ~DMA_CCR_EN;
        uint16_t received = BUF_SIZE - DMA1_Channel6->CNDTR;
        if (received > 0) {
            m_rxLen2         = received;
            m_frameReceived2 = true;
        }

        DMA1_Channel6->CNDTR = BUF_SIZE;
        DMA1_Channel6->CCR  |= DMA_CCR_EN;
    }
}

void ModbusRTU::irqHandlerTx2() {
    uint32_t tcifFlag = (1UL << 25); // TCIF7 (DMA1_Channel7)
    if (DMA1->ISR & tcifFlag) {
        DMA1->IFCR = tcifFlag;
        DMA1_Channel7->CCR &= ~DMA_CCR_EN;

        while (!(USART2->SR & USART_SR_TC));
        setModeRX2();
    }
}

extern ModbusRTU modbus;

extern "C" {
    void USART3_IRQHandler(void) {
        modbus.irqHandlerIdle3();
    }

    void DMA1_Channel2_IRQHandler(void) {
        modbus.irqHandlerTx3();
    }

    void USART2_IRQHandler(void) {
        modbus.irqHandlerIdle2();
    }

    void DMA1_Channel7_IRQHandler(void) {
        modbus.irqHandlerTx2();
    }
}
