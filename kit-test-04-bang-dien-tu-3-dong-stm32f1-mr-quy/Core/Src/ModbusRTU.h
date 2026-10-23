#ifndef MODBUSRTU_H
#define MODBUSRTU_H

#include "stm32f1xx.h"
#include <stdint.h>
#include "SystemUI.h"

/*
 * ============================================================================
 * CAU HINH PHAN CUNG RS485 (Chon cổng UART theo schematic cua mach):
 *   Dat RS485_UART_NUM = 1 : USART1 (TX = PA9,  RX = PA10, DE/RE = PA8)  [Mac dinh]
 *   Dat RS485_UART_NUM = 2 : USART2 (TX = PA2,  RX = PA3,  DE/RE = PA1)
 *   Dat RS485_UART_NUM = 3 : USART3 (TX = PB10, RX = PB11, DE/RE = PB12)
 * ============================================================================
 */
#define RS485_UART_NUM  1

/*
 * ModbusRTU - Thu vien giao tiep RS485 theo chuan Modbus RTU Slave
 *
 * Thong so yeu cau (theo README.md):
 *      - Chuan giao tiep: Modbus RTU
 *      - Mach la Slave, ID = 5
 *      - Toc do truyen: 115200 - 8N1 (115200 baud, 8 data bits, No parity, 1 stop bit)
 *      - Ho tro Master doc/ghi tat ca cac thong so cua mach:
 *          + Ma ham 0x03 (Read Holding Registers) va 0x04 (Read Input Registers)
 *          + Ma ham 0x06 (Write Single Register)
 *          + Ma ham 0x10 (Write Multiple Registers)
 *      - Tinh toan toc do truyen thong FPS (Frame per second - so lan truyen nhan
 *        thanh cong trong 1 giay) va cho phep Master doc thong so nay.
 *      - Viet bang thanh ghi, khong dung delay (non-blocking).
 */
class ModbusRTU {
public:
    static const uint8_t  SLAVE_ID      = 5;        // Dia chi Slave = 5 theo yeu cau
    static const uint32_t BAUDRATE      = 115200UL; // Toc do baud 115200-8N1
    static const uint8_t  TOTAL_REGS    = 18;       // Tong so thanh ghi Modbus (0..17)
    static const uint8_t  BUF_SIZE      = 64;       // Kich thuoc bo dem khung truyen/nhan
    static const uint32_t SILENCE_US    = 2000UL;   // Khoang lang 2ms (2000us) ket thuc 1 khung RTU

    // Danh sach dia chi cac thanh ghi Modbus (Holding Registers)
    enum RegAddr {
        REG_TARGET_DAY   = 0,   // Dong 1: Muc tieu ngay (Doc/Ghi)
        REG_TARGET_NOW   = 1,   // Dong 2: Muc tieu tai thoi diem hien tai (Doc)
        REG_COUNT        = 2,   // Dong 3: So luong san pham hien tai (Doc/Ghi)
        REG_FACTOR       = 3,   // He so cong khi cam bien phat hien san pham (Doc/Ghi)
        REG_FPS          = 4,   // Toc do truyen thong FPS - Frame per second (Doc)
        REG_HOUR         = 5,   // Thoi gian thuc: Gio 0..23 (Doc/Ghi)
        REG_MINUTE       = 6,   // Thoi gian thuc: Phut 0..59 (Doc/Ghi)
        REG_SECOND       = 7,   // Thoi gian thuc: Giay 0..59 (Doc/Ghi)
        REG_DATE         = 8,   // Thoi gian thuc: Ngay 1..31 (Doc/Ghi)
        REG_MONTH        = 9,   // Thoi gian thuc: Thang 1..12 (Doc/Ghi)
        REG_YEAR         = 10,  // Thoi gian thuc: Nam 2000..2099 (Doc/Ghi)
        REG_SHIFT1_START = 11,  // Ca 1: Gio bat dau (tinh bang phut tu 0h00, VD 06:00 = 360) (Doc/Ghi)
        REG_SHIFT1_END   = 12,  // Ca 1: Gio ket thuc (phut tu 0h00, VD 14:00 = 840) (Doc/Ghi)
        REG_SHIFT2_START = 13,  // Ca 2: Gio bat dau (phut tu 0h00, VD 14:00 = 840) (Doc/Ghi)
        REG_SHIFT2_END   = 14,  // Ca 2: Gio ket thuc (phut tu 0h00, VD 22:00 = 1320) (Doc/Ghi)
        REG_SHIFT3_START = 15,  // Ca 3: Gio bat dau (phut tu 0h00, VD 22:00 = 1320) (Doc/Ghi)
        REG_SHIFT3_END   = 16,  // Ca 3: Gio ket thuc (phut tu 0h00, VD 06:00 = 360) (Doc/Ghi)
        REG_TOTAL_OK     = 17   // Tong so lan truyen nhan thanh cong tich luy (Doc)
    };

    ModbusRTU();

    // Khoi tao thanh ghi UART, GPIO va chan dieu khien DE/RE (goi SAU SystemClock_Config)
    void init(SystemUI* uiPtr, uint32_t nowMs);

    // Goi lien tuc trong vong lap while(1): nhan khung, xu ly Modbus, gui phan hoi, tinh FPS
    void process(uint32_t nowMs);

    // Goi trong ham ngat USARTx_IRQHandler de bat byte nhan tuc thi (khong bao gio sot byte)
    void onRxIrq();

    // Lay gia tri FPS hien tai (so khung truyen nhan thanh cong trong 1 giay)
    uint16_t getFps() const { return currentFps; }
    uint32_t getTotalSuccess() const { return totalSuccessFrames; }

private:
    enum TxState {
        TX_IDLE,        // Dang o che do Nhan (DE = 0)
        TX_SENDING,     // Dang day tung byte ra thanh ghi DR (DE = 1)
        TX_WAIT_TC      // Da day het byte, cho co TC (truyen xong bit cuoi) roi ha DE = 0
    };

    SystemUI* ui;

    volatile uint8_t  rxBuf[BUF_SIZE];  // Bo dem nhan khung Modbus
    volatile uint8_t  rxLen;            // So byte da nhan trong khung hien tai
    volatile uint32_t lastRxCycle;      // Moc DWT->CYCCNT cua byte nhan cuoi cung
    uint32_t          silenceCycles;    // So chu ky CPU tuong ung voi SILENCE_US (2ms)

    uint8_t  txBuf[BUF_SIZE];           // Bo dem phat phan hoi Modbus
    uint8_t  txLen;                     // Tong so byte can phat
    uint8_t  txIdx;                     // Vi tri byte dang phat
    TxState  txState;                   // Trang thai may phat non-blocking

    uint32_t lastFpsMs;                 // Moc thoi gian 1000ms de chot FPS
    uint16_t fpsCounter;                // Dem so khung thanh cong trong giay hien tai
    uint16_t currentFps;                // Gia tri FPS cua giay vua qua
    uint32_t totalSuccessFrames;        // Tong so khung thanh cong tu luc khoi dong

    void setDeTransmit(bool tx);        // Dieu khien chan DE/RE cua IC RS485 (true = Phat, false = Nhan)
    void pollRxByte();                  // Kiem tra co RXNE (phong truong hop khong dung ngat)
    void handleFrame(const uint8_t* frame, uint8_t len);
    void sendResponse(uint8_t len);
    void sendException(uint8_t funcCode, uint8_t exCode);

    uint16_t readReg(uint16_t addr) const;
    bool     writeReg(uint16_t addr, uint16_t val);

    static uint16_t calcCrc16(const uint8_t* data, uint8_t len);
};

#endif
