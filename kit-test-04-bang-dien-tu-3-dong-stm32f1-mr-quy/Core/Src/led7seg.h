#ifndef LED7SEG_H
#define LED7SEG_H

#include "stm32f1xx.h"
#include <stdint.h>

/*
 * Led7Seg - Driver bang LED 7 doan 3 dong, dich bit noi tiep (74HC595 / CD4094 daisy-chain)
 *
 * Phan cung (khop voi so do mach cua kit):
 *      PC10 = CK  (CLOCK - xung dich bit)
 *      PC11 = ST  (STROBE / LATCH - chot du lieu ra LED)
 *      PC12 = DT  (DATA)
 *
 * Cach dung:
 *      myLed.init();                   // 1 lan luc khoi dong
 *      myLed.setNumber(0, 1000);       // Dong 1
 *      myLed.setNumber(1, 500);        // Dong 2
 *      myLed.setNumber(2, count);      // Dong 3
 *      while(1) { myLed.process(); }   // goi lien tuc trong vong lap chinh (khong chan, khong delay)
 */
class Led7Seg {
public:
    static const uint8_t ROWS   = 3;    // So dong
    static const uint8_t DIGITS = 5;    // So con LED tren moi dong (bang thuc te = 5)

    static const uint8_t SEG_BLANK = 0x00;  // Tat het cac doan
    static const uint8_t SEG_DOT   = 0x08;  // Bit dieu khien dau cham

    void init();

    // Hien thi so can phai (leadingZero = false: bo cac so 0 o dau)
    void setNumber(uint8_t row, uint32_t value, bool leadingZero = false);

    // Hien thi 1 chu so (0-9) tai vi tri (row, col)
    void setDigit(uint8_t row, uint8_t col, uint8_t digit);

    // Bat/tat dau cham tai vi tri (row, col)
    void setDot(uint8_t row, uint8_t col, bool on);

    // Tat toan bo LED
    void clear();

    // Goi lien tuc trong while(1): tu dong day du lieu ra LED khi co thay doi (khong chan CPU)
    void process();

    // true khi dang trong qua trinh day du lieu
    bool isBusy() const { return state != ST_IDLE; }

private:
    // Cac buoc cua may trang thai day du lieu
    enum State {
        ST_IDLE,        // Khong lam gi
        ST_SETUP,       // Dat muc DATA, keo CLOCK xuong
        ST_CLOCK,       // Keo CLOCK len (IC doc bit)
        ST_LATCH_LOW,   // Ha STROBE xuong
        ST_LATCH_HIGH   // Keo STROBE len -> chot du lieu ra LED
    };

    static const uint8_t TOTAL_BYTES = ROWS * DIGITS;

    static const uint8_t FONT[10];                  // Ma 7 doan cua so 0-9
    static const bool    FLIP_MAP[ROWS][DIGITS];    // true = module LED nay dau day nguoc 180 do

    uint8_t  seg[ROWS][DIGITS];     // Ma doan (chua dao) cua tung con LED
    uint8_t  frame[TOTAL_BYTES];    // Khung du lieu se day ra (da dao neu can)
    bool     dirty;                 // Co thay doi can cap nhat ra LED
    State    state;
    uint8_t  byteIdx;
    uint8_t  bitIdx;
    uint32_t lastCycle;             // Moc CYCCNT cua buoc truoc
    uint32_t halfPeriodCycles;      // So chu ky CPU cho 1/2 chu ky xung CLOCK

    static uint8_t rotate180(uint8_t v);
    void buildFrame();
};

#endif
