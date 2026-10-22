#include "led7seg.h"

// Ma Hex cho Catot chung (Bit 7 DP = 0 de tat dau cham)
const uint8_t Led7Seg::fontCathode[16] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 
    0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71
};

// Ma Hex cho Anot chung (Bit 7 DP = 1 de tat dau cham)
const uint8_t Led7Seg::fontAnode[16] = {
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 
    0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E
};

// Tao do tre doc bit de dam bao IC doi cu bat kip (100 chu ky NOP = khoang 1.4 us)
void delay_nop() {
    for(volatile int j = 0; j < 100; j++) {
        __NOP();
    }
}

void Led7Seg::init(bool cathodeMode) {
    isCathode = cathodeMode;

    RCC->APB2ENR |= (1UL << 4);

    // KET LIEU LOI 8.8.8.8 TAI DAY: 
    // Dung cau hinh Output 2MHz (0x222) thay vi 50MHz (0x333) de triet tieu nhieu day cap
    GPIOC->CRH &= ~(0xFFFUL << 8);
    GPIOC->CRH |= (0x222UL << 8);

    // Xoa dien ap ve 0V ban dau
    GPIOC->BRR = (1UL << PIN_STR_BIT) | (1UL << PIN_CLK_BIT) | (1UL << PIN_DATA_BIT);
}

void Led7Seg::shiftOut(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        // Chuan bi muc dien ap cho chan DATA 
        if (data & (0x80 >> i)) {
            GPIOC->BSRR = (1UL << PIN_DATA_BIT);
        } else {
            GPIOC->BRR = (1UL << PIN_DATA_BIT);
        }
        delay_nop(); 
        
        // Danh xung CLOCK de doc bit
        GPIOC->BSRR = (1UL << PIN_CLK_BIT);
        delay_nop(); 
        
        // Ha xung CLOCK xuong
        GPIOC->BRR = (1UL << PIN_CLK_BIT);
        delay_nop(); 
    }
}

void Led7Seg::displayRows(uint32_t row1, uint32_t row2, uint32_t row3) {
    uint8_t digits[15];
    uint32_t temp;

    // Tach 5 so Hang 1
    temp = row1;
    for (int i = 4; i >= 0; i--) { digits[i] = temp % 10; temp /= 10; }

    // Tach 5 so Hang 2
    temp = row2;
    for (int i = 9; i >= 5; i--) { digits[i] = temp % 10; temp /= 10; }

    // Tach 5 so Hang 3
    temp = row3;
    for (int i = 14; i >= 10; i--) { digits[i] = temp % 10; temp /= 10; }

    // Truyen 15 byte ra mach (Dich tu cuoi len dau cho chuoi daisy-chain)
    for (int i = 14; i >= 0; i--) {
        uint8_t num = digits[i];
        uint8_t code = isCathode ? fontCathode[num] : fontAnode[num];
        shiftOut(code);
    }

    // Phat xung LATCH (Strobe) de hien thi toan bo hinh anh
    GPIOC->BSRR = (1UL << PIN_STR_BIT);
    delay_nop(); 
    GPIOC->BRR = (1UL << PIN_STR_BIT);
    delay_nop();
}