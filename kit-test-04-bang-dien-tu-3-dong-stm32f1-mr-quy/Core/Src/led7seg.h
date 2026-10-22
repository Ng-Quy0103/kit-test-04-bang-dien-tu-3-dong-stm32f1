#ifndef LED7SEG_H
#define LED7SEG_H

#include "stm32f1xx.h"
#include <stdint.h>

// CHOT CAU HINH CHUAN TRUONG HOP 6
#define PIN_STR_BIT   10UL  // Chan PC10: LATCH / STROBE
#define PIN_CLK_BIT   11UL  // Chan PC11: CLOCK 
#define PIN_DATA_BIT  12UL  // Chan PC12: DATA 

class Led7Seg {
private:
    static const uint8_t fontCathode[16];
    static const uint8_t fontAnode[16];

    bool isCathode; 
    void shiftOut(uint8_t data); 

public:
    void init(bool cathodeMode = true); 
    void displayRows(uint32_t row1, uint32_t row2, uint32_t row3); 
};

#endif