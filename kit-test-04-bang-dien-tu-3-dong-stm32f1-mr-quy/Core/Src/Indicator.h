#ifndef INDICATOR_H
#define INDICATOR_H

#include <stm32f1xx.h>
#include <stdint.h> 

class Indicator {
private:
    uint32_t preLed_millis;
    uint32_t prevBuzzerMillis;
    uint8_t beepCount;
    
    enum Buzz_state {
        IDLE,
        BEEP_ON,
        BEEP_OFF
    }buzState;
    

public :
    Indicator(); // ham carry cac thong so
    void init(); // ham init
    void update(uint32_t currentMillis); // ham update led & buzzer
    void triggerSingleBeep(uint32_t currentMillis);  /* ham coi keu */
};

#endif