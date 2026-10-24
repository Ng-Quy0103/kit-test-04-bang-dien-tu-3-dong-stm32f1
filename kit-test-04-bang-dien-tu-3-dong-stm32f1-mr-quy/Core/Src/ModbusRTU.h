#ifndef MODBUSRTU_H
#define MODBUSRTU_H

#include "stm32f1xx.h"
#include <stdint.h>
#include "SystemUI.h"

/*
 * ============================================================================
 * ModbusRTU - Thu vien giao tiep RS485 Modbus RTU Slave bang DMA + Ngat IDLE
 * (Tham khao truc tiep tu ModbusDMA.cpp tren bo mach Kit Test 04 STM32F103RCT6)
 *
 * Ho tro DONG THOI ca 2 cong RS485 tren bo mach (cam day vao Domino nao cung chay):
 *   1. Cong RS485 so 1 (USART3):
 *        - TX = PB10 (AF Push-Pull 50MHz), RX = PB11 (Input Floating)
 *        - DE/RE = PA6 (Output Push-Pull 50MHz: 1 = Phat TX, 0 = Nhan RX)
 *        - DMA TX = DMA1_Channel2, DMA RX = DMA1_Channel3
 *   2. Cong RS485 so 2 (USART2 - Domino A2, B2):
 *        - TX = PA2 (AF Push-Pull 50MHz), RX = PA3 (Input Floating)
 *        - DE/RE = PA5 (Output Push-Pull 50MHz: 1 = Phat TX, 0 = Nhan RX)
 *        - DMA TX = DMA1_Channel7, DMA RX = DMA1_Channel6
 *
 * Thong so giao tiep theo README.md:
 *   - Chuan giao tiep : Modbus RTU Slave
 *   - Dia chi Slave   : ID = 5
 *   - Toc do truyen   : 115200 - 8N1
 *   - Ho tro ma ham   : 0x03 (Read Holding), 0x04 (Read Input),
 *                       0x06 (Write Single), 0x10 (Write Multiple)
 *   - Tinh toc do FPS : Frame per second (so lan truyen nhan thanh cong / giay)
 * ============================================================================
 */
class ModbusRTU {
public:
    static const uint8_t  SLAVE_ID   = 5;           // Dia chi Slave = 5 theo README.md
    static const uint32_t BAUDRATE   = 115200UL;    // Toc do truyen 115200-8N1
    static const uint16_t REG_COUNT  = 20;          // 20 thanh ghi Holding (0..19)
    static const uint16_t BUF_SIZE   = 256;         // Bo dem DMA 256 byte

    // Danh sach 20 thanh ghi Modbus (Holding Registers 0..19)
    enum RegAddr {
        REG_FPS           = 0,  // Thanh ghi 0 : Toc do truyen thong FPS (Frame per second)
        REG_YEAR          = 1,  // Thanh ghi 1 : Nam thoi gian thuc (2000..2099)
        REG_MONTH         = 2,  // Thanh ghi 2 : Thang thoi gian thuc (1..12)
        REG_DATE          = 3,  // Thanh ghi 3 : Ngay thoi gian thuc (1..31)
        REG_HOUR          = 4,  // Thanh ghi 4 : Gio thoi gian thuc (0..23)
        REG_MINUTE        = 5,  // Thanh ghi 5 : Phut thoi gian thuc (0..59)
        REG_SECOND        = 6,  // Thanh ghi 6 : Giay thoi gian thuc (0..59)
        REG_TARGET_DAY    = 7,  // Thanh ghi 7 : Muc tieu ngay - Dong 1 (Doc/Ghi)
        REG_TARGET_NOW    = 8,  // Thanh ghi 8 : Muc tieu hien tai realtime - Dong 2 (Chi Doc)
        REG_ACTUAL_PROD   = 9,  // Thanh ghi 9 : San luong hien tai - Dong 3 (Doc/Ghi)
        REG_S1_START      = 10, // Thanh ghi 10: Ca 1 Bat dau dang HHMM (VD: 600 = 06:00)
        REG_S1_END        = 11, // Thanh ghi 11: Ca 1 Ket thuc dang HHMM (VD: 1400 = 14:00)
        REG_S2_START      = 12, // Thanh ghi 12: Ca 2 Bat dau dang HHMM (VD: 1400 = 14:00)
        REG_S2_END        = 13, // Thanh ghi 13: Ca 2 Ket thuc dang HHMM (VD: 2200 = 22:00)
        REG_S3_START      = 14, // Thanh ghi 14: Ca 3 Bat dau dang HHMM (VD: 2200 = 22:00)
        REG_S3_END        = 15, // Thanh ghi 15: Ca 3 Ket thuc dang HHMM (VD: 600 = 06:00)
        REG_FACTOR        = 16, // Thanh ghi 16: He so cong khi cam bien dem (1..999)
        REG_TOTAL_PACKETS = 17, // Thanh ghi 17: Tong so lan truyen nhan thanh cong tich luy
        REG_RESERVED_18   = 18, // Thanh ghi 18: Du phong
        REG_RESERVED_19   = 19  // Thanh ghi 19: Du phong
    };

    ModbusRTU();

    // Khoi tao USART3 + USART2 cung cac kenh DMA va chan DE/RE (PA6, PA1)
    void init(SystemUI* uiPtr, uint32_t nowMs);

    // Goi lien tuc trong while(1): xu ly khung nhan DMA, dong bo thanh ghi va tinh FPS
    void process(uint32_t nowMs);

    // Ngat IDLE va DMA TX cho USART3 (PB10/PB11, DE = PA6)
    void irqHandlerIdle3();
    void irqHandlerTx3();

    // Ngat IDLE va DMA TX cho USART2 (PA2/PA3, DE = PA1)
    void irqHandlerIdle2();
    void irqHandlerTx2();

    uint16_t getFps() const { return m_currentFps; }
    uint32_t getPacketCounter() const { return m_packetCounter; }

private:
    SystemUI* m_ui;

    uint16_t  m_holdingRegisters[REG_COUNT];

    // Bo dem DMA cho USART3 (PB10/PB11, DE = PA6)
    uint8_t           m_rxBuf3[BUF_SIZE];
    uint8_t           m_txBuf3[BUF_SIZE];
    volatile uint16_t m_rxLen3;
    volatile bool     m_frameReceived3;

    // Bo dem DMA cho USART2 (PA2/PA3, DE = PA1)
    uint8_t           m_rxBuf2[BUF_SIZE];
    uint8_t           m_txBuf2[BUF_SIZE];
    volatile uint16_t m_rxLen2;
    volatile bool     m_frameReceived2;

    uint32_t          m_packetCounter;
    uint32_t          m_lastPacketCount;
    uint32_t          m_lastFpsTime;
    uint16_t          m_currentFps;

    // Dieu khien chan DE/RE cua USART3 (PA6) va USART2 (PA5)
    inline void setModeTX3() { GPIOA->BSRR = (1UL << 6); }
    inline void setModeRX3() { GPIOA->BRR  = (1UL << 6); }

    inline void setModeTX2() { GPIOA->BSRR = (1UL << 5); }
    inline void setModeRX2() { GPIOA->BRR  = (1UL << 5); }

    void sendDMA(uint8_t port, uint8_t* buf, uint16_t len);
    void sendException(uint8_t port, uint8_t* txBuf, uint8_t func, uint8_t code);
    void processFrame(uint8_t port, const uint8_t* rxBuf, uint16_t rxLen, uint8_t* txBuf);

    void syncFromUI();                          // Lay thong so tu SystemUI nap vao m_holdingRegisters[]
    void applyToUI(uint16_t addr, uint16_t val);// Ghi thong so tu Master xuong SystemUI

    bool checkLimit(uint16_t addr, uint16_t val);
    static uint16_t calculateCRC(const uint8_t* buf, uint16_t len);
    static uint16_t minToHhmm(uint16_t minFromMidnight);
    static bool     hhmmToMin(uint16_t val, uint16_t& outMin);
};

#endif
