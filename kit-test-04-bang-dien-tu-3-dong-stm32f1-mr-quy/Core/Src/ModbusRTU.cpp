#include "ModbusRTU.h"

// Chon ngoai vi USART va chan GPIO tuong ung voi RS485_UART_NUM
#if (RS485_UART_NUM == 1)
    #define MB_USART        USART1
    #define MB_IRQn         USART1_IRQn
#elif (RS485_UART_NUM == 2)
    #define MB_USART        USART2
    #define MB_IRQn         USART2_IRQn
#else
    #define MB_USART        USART3
    #define MB_IRQn         USART3_IRQn
#endif

// Con tro tinh de goi nhanh trong ham ngat USARTx_IRQHandler
static ModbusRTU* s_modbusInstance = 0;

ModbusRTU::ModbusRTU() {
    ui                 = 0;
    rxLen              = 0;
    lastRxCycle        = 0;
    silenceCycles      = 0;
    txLen              = 0;
    txIdx              = 0;
    txState            = TX_IDLE;
    lastFpsMs          = 0;
    fpsCounter         = 0;
    currentFps         = 0;
    totalSuccessFrames = 0;
}

// Dieu khien chan DE/RE cua IC thu phat RS485 (MAX485 / SP3485):
//   tx = true  -> Keo chan DE len muc 1 (che do Phat - Transmit)
//   tx = false -> Keo chan DE xuong muc 0 (che do Nhan - Receive)
void ModbusRTU::setDeTransmit(bool tx) {
#if (RS485_UART_NUM == 1)
    // PA8 lam chan DE/RE (hoac PA12 cung duoc kich song song de tuong thich nhieu phien ban mach)
    if (tx) GPIOA->BSRR = (1UL << 8);
    else    GPIOA->BSRR = (1UL << (8 + 16));
#elif (RS485_UART_NUM == 2)
    // PA1 lam chan DE/RE
    if (tx) GPIOA->BSRR = (1UL << 1);
    else    GPIOA->BSRR = (1UL << (1 + 16));
#else
    // PB12 lam chan DE/RE
    if (tx) GPIOB->BSRR = (1UL << 12);
    else    GPIOB->BSRR = (1UL << (12 + 16));
#endif
}

void ModbusRTU::init(SystemUI* uiPtr, uint32_t nowMs) {
    ui               = uiPtr;
    lastFpsMs        = nowMs;
    s_modbusInstance = this;

    // 1. Bat bo dem chu ky CPU (DWT->CYCCNT) de do khoang lang ket thuc khung Modbus RTU
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    silenceCycles = (SystemCoreClock / 1000000UL) * SILENCE_US;

#if (RS485_UART_NUM == 1)
    // --- CAU HINH USART1: TX = PA9, RX = PA10, DE/RE = PA8 ---
    // Bat clock Port A va USART1 tren bus APB2 (72MHz)
    RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN);

    // PA8 (DE): Output Push-Pull 50MHz (MODE8=11, CNF8=00 -> 0x3)
    // PA9 (TX): Alternate Function Push-Pull 50MHz (MODE9=11, CNF9=10 -> 0xB)
    // PA10 (RX): Input Pull-Up (MODE10=00, CNF10=10 -> 0x8)
    GPIOA->CRH &= ~(0xFFFUL << 0);      // Xoa 12 bit cua PA8, PA9, PA10 (bit 0..11)
    GPIOA->CRH |=  (0x8B3UL << 0);      // PA10=0x8, PA9=0xB, PA8=0x3
    GPIOA->BSRR =  (1UL << 10);         // Keo PA10 (RX) len muc cao (Pull-Up) de chong nhieu khi ho day

    // Tinh thanh ghi BRR cho toc do 115200 tai PCLK2 = 72MHz: 72000000 / 115200 = 625 (0x0271)
    USART1->BRR = SystemCoreClock / BAUDRATE;

#elif (RS485_UART_NUM == 2)
    // --- CAU HINH USART2: TX = PA2, RX = PA3, DE/RE = PA1 ---
    // Bat clock Port A (APB2) va USART2 (APB1 = 36MHz)
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // PA1 (DE): Output Push-Pull 50MHz (0x3)
    // PA2 (TX): AF Push-Pull 50MHz (0xB)
    // PA3 (RX): Input Pull-Up (0x8)
    GPIOA->CRL &= ~(0xFFFUL << 4);      // Xoa 12 bit cua PA1, PA2, PA3 (bit 4..15)
    GPIOA->CRL |=  (0x8B3UL << 4);
    GPIOA->BSRR =  (1UL << 3);          // Pull-up cho PA3 (RX)

    // PCLK1 = SystemCoreClock / 2 = 36MHz
    USART2->BRR = (SystemCoreClock / 2UL) / BAUDRATE;

#else
    // --- CAU HINH USART3: TX = PB10, RX = PB11, DE/RE = PB12 ---
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;

    // PB10 (TX): AF Push-Pull 50MHz (0xB), PB11 (RX): Input Pull-Up (0x8), PB12 (DE): Output Push-Pull (0x3)
    GPIOB->CRH &= ~(0xFFFUL << 8);      // Xoa 12 bit cua PB10, PB11, PB12 (bit 8..19)
    GPIOB->CRH |=  (0x38BUL << 8);
    GPIOB->BSRR =  (1UL << 11);         // Pull-up cho PB11 (RX)

    USART3->BRR = (SystemCoreClock / 2UL) / BAUDRATE;
#endif

    // Dat IC RS485 o che do Nhan (DE = 0)
    setDeTransmit(false);

    // Cau hinh 8N1 (M = 0: 8 data bits, PCE = 0: No parity, STOP = 00: 1 stop bit)
    // Bat UART (UE), bat Phat (TE), bat Nhan (RE) va bat Ngat nhan (RXNEIE)
    MB_USART->CR2 = 0;
    MB_USART->CR3 = 0;
    MB_USART->CR1 = (USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE);

    // Cho phep ngat USART trong NVIC
    NVIC_SetPriority(MB_IRQn, 1);
    NVIC_EnableIRQ(MB_IRQn);
}

// Ham doc byte tu thanh ghi DR khi co co RXNE (goi trong ngat hoac trong vong lap)
void ModbusRTU::onRxIrq() {
    uint32_t sr = MB_USART->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE)) {
        uint8_t b = (uint8_t)(MB_USART->DR & 0xFF);     // Doc DR se tu dong xoa co RXNE va ORE

        // Chi nhan khi khong dang phat va co khong bi loi khung (FE)
        if (txState == TX_IDLE && !(sr & USART_SR_FE)) {
            if (rxLen < BUF_SIZE) {
                rxBuf[rxLen++] = b;
            }
            lastRxCycle = DWT->CYCCNT;                  // Ghi nho thoi diem nhan byte cuoi
        }
    }
}

void ModbusRTU::pollRxByte() {
    if (MB_USART->SR & (USART_SR_RXNE | USART_SR_ORE)) {
        onRxIrq();
    }
}

// Vong lap xu ly Modbus RTU (non-blocking, khong dung delay)
void ModbusRTU::process(uint32_t nowMs) {
    // 1. Quet them co RXNE de dam bao hoat dong ca khi ngat bi tat
    pollRxByte();

    // 2. Cap nhat thong so FPS (Frame per second) moi 1000ms (1 giay)
    if ((uint32_t)(nowMs - lastFpsMs) >= 1000UL) {
        lastFpsMs  = nowMs;
        currentFps = fpsCounter;
        fpsCounter = 0;
    }

    // 3. Xu ly may trang thai Phat (TX) khong chan CPU
    if (txState == TX_SENDING) {
        // Kiem tra co TXE (Transmit Data Register Empty - thanh ghi DR da trong)
        if (MB_USART->SR & USART_SR_TXE) {
            if (txIdx < txLen) {
                MB_USART->DR = txBuf[txIdx++];
            } else {
                // Da nap het cac byte vao DR, chuyen sang cho bit cuoi cung ra khoi duong day (co TC)
                txState = TX_WAIT_TC;
            }
        }
        return;
    } else if (txState == TX_WAIT_TC) {
        // Khi co TC (Transmission Complete) = 1 -> toan bo khung phan hoi da gui xong
        if (MB_USART->SR & USART_SR_TC) {
            setDeTransmit(false);           // Chuyen IC RS485 ve che do Nhan ngay lap tuc
            txState = TX_IDLE;

            // Moi lan truyen nhan thanh cong giua Master va Slave thi tinh la +1 lan thanh cong
            fpsCounter++;
            totalSuccessFrames++;
        }
        return;
    }

    // 4. Kiem tra xem da ket thuc 1 khung nhan Modbus RTU chua (khoang lang >= 2ms)
    if (rxLen > 0) {
        uint32_t elapsedCycles = (uint32_t)(DWT->CYCCNT - lastRxCycle);
        if (elapsedCycles >= silenceCycles) {
            // Copy khung nhan ra bo dem tam de xu ly an toan
            uint8_t localFrame[BUF_SIZE];
            __disable_irq();
            uint8_t len = rxLen;
            for (uint8_t i = 0; i < len; i++) localFrame[i] = rxBuf[i];
            rxLen = 0;
            __enable_irq();

            handleFrame(localFrame, len);
        }
    }
}

// Tinh ma kiem tra loi CRC-16/MODBUS (da thuc 0xA001, gia tri khoi tao 0xFFFF)
uint16_t ModbusRTU::calcCrc16(const uint8_t* data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)data[pos];
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// Doc gia tri 1 thanh ghi Modbus theo dia chi addr (0..17)
uint16_t ModbusRTU::readReg(uint16_t addr) const {
    if (!ui) return 0;
    const RtcTime& clk = ui->getClock();

    switch (addr) {
        case REG_TARGET_DAY:   return (uint16_t)(ui->getTargetDay() & 0xFFFF);
        case REG_TARGET_NOW:   return (uint16_t)(ui->getTargetNow() & 0xFFFF);
        case REG_COUNT:        return (uint16_t)(ui->getCount() & 0xFFFF);
        case REG_FACTOR:       return ui->getFactor();
        case REG_FPS:          return (currentFps > 0) ? currentFps : fpsCounter;
        case REG_HOUR:         return clk.hour;
        case REG_MINUTE:       return clk.minute;
        case REG_SECOND:       return clk.second;
        case REG_DATE:         return clk.date;
        case REG_MONTH:        return clk.month;
        case REG_YEAR:         return (uint16_t)(2000 + clk.year);
        case REG_SHIFT1_START: return ui->getShiftStart(0);
        case REG_SHIFT1_END:   return ui->getShiftEnd(0);
        case REG_SHIFT2_START: return ui->getShiftStart(1);
        case REG_SHIFT2_END:   return ui->getShiftEnd(1);
        case REG_SHIFT3_START: return ui->getShiftStart(2);
        case REG_SHIFT3_END:   return ui->getShiftEnd(2);
        case REG_TOTAL_OK:     return (uint16_t)(totalSuccessFrames & 0xFFFF);
        default:               return 0;
    }
}

// Ghi gia tri val xuong 1 thanh ghi Modbus theo dia chi addr
bool ModbusRTU::writeReg(uint16_t addr, uint16_t val) {
    if (!ui) return false;
    RtcTime clk = ui->getClock();

    switch (addr) {
        case REG_TARGET_DAY:
            ui->setTargetDay(val);
            return true;

        case REG_COUNT:
            ui->setCount(val);
            return true;

        case REG_FACTOR:
            if (val < 1 || val > 999) return false;
            ui->setFactor(val);
            return true;

        case REG_HOUR:
            if (val > 23) return false;
            clk.hour = (uint8_t)val;
            ui->setTime(clk);
            return true;

        case REG_MINUTE:
            if (val > 59) return false;
            clk.minute = (uint8_t)val;
            ui->setTime(clk);
            return true;

        case REG_SECOND:
            if (val > 59) return false;
            clk.second = (uint8_t)val;
            ui->setTime(clk);
            return true;

        case REG_DATE:
            if (val < 1 || val > 31) return false;
            clk.date = (uint8_t)val;
            ui->setTime(clk);
            return true;

        case REG_MONTH:
            if (val < 1 || val > 12) return false;
            clk.month = (uint8_t)val;
            ui->setTime(clk);
            return true;

        case REG_YEAR:
            clk.year = (uint8_t)(val % 100);
            ui->setTime(clk);
            return true;

        case REG_SHIFT1_START: ui->setShiftStart(0, val); return true;
        case REG_SHIFT1_END:   ui->setShiftEnd(0,   val); return true;
        case REG_SHIFT2_START: ui->setShiftStart(1, val); return true;
        case REG_SHIFT2_END:   ui->setShiftEnd(1,   val); return true;
        case REG_SHIFT3_START: ui->setShiftStart(2, val); return true;
        case REG_SHIFT3_END:   ui->setShiftEnd(2,   val); return true;

        default:
            return false;
    }
}

// Gan CRC16 vao cuoi txBuf va bat dau day khung phan hoi ve Master
void ModbusRTU::sendResponse(uint8_t len) {
    uint16_t crc = calcCrc16(txBuf, len);
    txBuf[len++] = (uint8_t)(crc & 0xFF);           // Byte thap CRC (LSB) gui truoc
    txBuf[len++] = (uint8_t)((crc >> 8) & 0xFF);    // Byte cao CRC (MSB) gui sau

    txLen   = len;
    txIdx   = 0;
    txState = TX_SENDING;

    // Xoa co TC cu va bat chan DE = 1 de chuyen IC RS485 sang che do Phat
    MB_USART->SR &= ~USART_SR_TC;
    setDeTransmit(true);
}

// Gui khung bao loi Modbus Exception (ma ham | 0x80)
void ModbusRTU::handleFrame(const uint8_t* frame, uint8_t len) {
    // Khung Modbus RTU toi thieu phai co 4 byte (ID + Func + Data + 2 byte CRC)
    if (len < 4) return;

    // 1. Kiem tra Slave ID (chi xu ly khi dung ID = 5)
    uint8_t id = frame[0];
    if (id != SLAVE_ID) return;

    // 2. Kiem tra CRC-16 (2 byte cuoi khung)
    uint16_t rxCrc   = (uint16_t)frame[len - 2] | ((uint16_t)frame[len - 1] << 8);
    uint16_t calcCrc = calcCrc16(frame, (uint8_t)(len - 2));
    if (rxCrc != calcCrc) return;

    uint8_t func = frame[1];

    // --- MA HAM 0x03 (Read Holding Registers) hoac 0x04 (Read Input Registers) ---
    if (func == 0x03 || func == 0x04) {
        if (len < 8) return;
        uint16_t startAddr = ((uint16_t)frame[2] << 8) | frame[3];
        uint16_t regCount  = ((uint16_t)frame[4] << 8) | frame[5];

        if (regCount == 0 || regCount > TOTAL_REGS || (startAddr + regCount) > TOTAL_REGS) {
            sendException(func, 0x02);      // Loi 0x02: Illegal Data Address
            return;
        }

        txBuf[0] = SLAVE_ID;
        txBuf[1] = func;
        txBuf[2] = (uint8_t)(regCount * 2); // So byte du lieu tra ve

        uint8_t idx = 3;
        for (uint16_t i = 0; i < regCount; i++) {
            uint16_t val = readReg(startAddr + i);
            txBuf[idx++] = (uint8_t)((val >> 8) & 0xFF);    // Byte cao (Big-Endian)
            txBuf[idx++] = (uint8_t)(val & 0xFF);           // Byte thap
        }
        sendResponse(idx);
    }
    // --- MA HAM 0x06 (Write Single Register - Ghi 1 thanh ghi) ---
    else if (func == 0x06) {
        if (len < 8) return;
        uint16_t regAddr = ((uint16_t)frame[2] << 8) | frame[3];
        uint16_t regVal  = ((uint16_t)frame[4] << 8) | frame[5];

        if (regAddr >= TOTAL_REGS) {
            sendException(func, 0x02);      // Illegal Data Address
            return;
        }
        if (!writeReg(regAddr, regVal)) {
            sendException(func, 0x03);      // Illegal Data Value
            return;
        }

        // Phan hoi cua 0x06 la lap lai y nguyen 6 byte dau cua Master + CRC
        for (uint8_t i = 0; i < 6; i++) txBuf[i] = frame[i];
        sendResponse(6);
    }
    // --- MA HAM 0x10 (Write Multiple Registers - Ghi nhieu thanh ghi lien tiep) ---
    else if (func == 0x10) {
        if (len < 9) return;
        uint16_t startAddr = ((uint16_t)frame[2] << 8) | frame[3];
        uint16_t regCount  = ((uint16_t)frame[4] << 8) | frame[5];
        uint8_t  byteCount = frame[6];

        if (regCount == 0 || regCount > TOTAL_REGS || (startAddr + regCount) > TOTAL_REGS ||
            byteCount != (regCount * 2) || len < (uint8_t)(9 + byteCount)) {
            sendException(func, 0x02);
            return;
        }

        for (uint16_t i = 0; i < regCount; i++) {
            uint16_t val = ((uint16_t)frame[7 + i * 2] << 8) | frame[8 + i * 2];
            writeReg(startAddr + i, val);
        }

        // Phan hoi cua 0x10 gom 6 byte dau (ID, Func, StartAddr, RegCount) + CRC
        for (uint8_t i = 0; i < 6; i++) txBuf[i] = frame[i];
        sendResponse(6);
    }
    else {
        // Ma ham khong ho tro
        sendException(func, 0x01);          // Illegal Function
    }
}

void ModbusRTU::sendException(uint8_t funcCode, uint8_t exCode) {
    txBuf[0] = SLAVE_ID;
    txBuf[1] = (uint8_t)(funcCode | 0x80);
    txBuf[2] = exCode;
    sendResponse(3);
}

// Trinh phuc vu ngat USART (khai bao extern "C" de bang vector ngat C goi duoc)
extern "C" {
#if (RS485_UART_NUM == 1)
void USART1_IRQHandler(void) {
    if (s_modbusInstance) s_modbusInstance->onRxIrq();
}
#elif (RS485_UART_NUM == 2)
void USART2_IRQHandler(void) {
    if (s_modbusInstance) s_modbusInstance->onRxIrq();
}
#else
void USART3_IRQHandler(void) {
    if (s_modbusInstance) s_modbusInstance->onRxIrq();
}
#endif
}
