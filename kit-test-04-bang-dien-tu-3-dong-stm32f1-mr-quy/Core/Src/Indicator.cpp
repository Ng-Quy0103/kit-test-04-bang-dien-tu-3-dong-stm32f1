#include "Indicator.h"

Indicator::Indicator() {
    preLed_millis = 0;        // Gan truc tiep, khong khai bao lai uint32_t
    prevBuzzerMillis = 0;
    beepCount = 2;            // coi keu 2 lan khoi dong
    buzState = BEEP_ON;
}

void Indicator::init() {
    RCC->APB2ENR |= (RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPDEN); // Cau hinh CLK cho port B, D
    
    // CAU HINH PUSH-PULL OUTPUT CHO PORTB2
    GPIOB->CRL &= ~(GPIO_CRL_MODE2 | GPIO_CRL_CNF2);
    GPIOB->CRL |= GPIO_CRL_MODE2; 
    
    // CAU HINH PUSH PULL OUTPUT PORTD2
    GPIOD->CRL &= ~(GPIO_CRL_MODE2 | GPIO_CRL_CNF2);
    GPIOD->CRL |= GPIO_CRL_MODE2;
    
    // CAU HINH BUZZER TAT BAN DAU
    GPIOD->BSRR = GPIO_BSRR_BR2;
}

void Indicator::update(uint32_t currentMillis) { // Them Indicator::
    if(currentMillis - preLed_millis >= 100) {
        preLed_millis = currentMillis;
        GPIOB->ODR ^= GPIO_ODR_ODR2; // dao trang thai chan khi dat 100ms 
    }
    
    switch (buzState) {
        case BEEP_ON:
            GPIOD->BSRR = GPIO_BSRR_BS2; // Bat coi chan PD2 len muc 1 
            if (currentMillis - prevBuzzerMillis >= 50) {
                prevBuzzerMillis = currentMillis;
                buzState = BEEP_OFF;
            }
            break;
            
        case BEEP_OFF:
            GPIOD->BSRR = GPIO_BSRR_BR2; // Tat coi chan PD2
            if (currentMillis - prevBuzzerMillis >= 100) {
                prevBuzzerMillis = currentMillis;
                beepCount--; // Keu xong 1 nhip thi tru di 1
                
                if (beepCount > 0) {
                    buzState = BEEP_ON; // Chua het so lan thi quay lai bat coi
                } else {
                    buzState = IDLE;    // Da het so lan thi ve IDLE
                }
            }
            break;
            
        case IDLE:
            break;
    }
}

void Indicator::triggerSingleBeep(uint32_t currentMillis) {
    if(buzState == IDLE) {
        beepCount = 1; 
        buzState = BEEP_ON;
        prevBuzzerMillis = currentMillis; 
    }
}