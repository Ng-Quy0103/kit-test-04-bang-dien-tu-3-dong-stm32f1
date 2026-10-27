#include "sensor.h"

sensor::sensor() {
    productCount = 0;
    debounceMillis = 0;
    sensorState = WAIT_OBJECT;
}

void sensor::init() {
    // 1. Bat xung nhip Clock cho Port C (Bit IOPCEN - Trang 114 RM0008)
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    // 2. Xoa sach 4 bit cau hinh cua chan PC3 ve 0000 (MODE3 = 00 -> Input mode)
    GPIOC->CRL &= ~(GPIO_CRL_MODE3 | GPIO_CRL_CNF3);

    // 3. Dat CNF3 = 10 (Che do Input with pull-up / pull-down - Trang 161, 171)
    GPIOC->CRL |= GPIO_CRL_CNF3_1;

    // 4. Keo chan PC3 len muc cao 3.3V (Pull-Up) thong qua thanh ghi BSRR (Trang 161, 173)
    GPIOC->BSRR = GPIO_BSRR_BS3;
}

// Ham quet cam bien: Tra ve true dung 1 lan duy nhat moi khi co 1 san pham hop le
bool sensor::update(uint32_t currentMillis) {
    bool isNewProduct = false;

    // Doc chan PC3 tu thanh ghi IDR (Trang 172):
    // Cam bien tiem can kich muc THAP (0V khi co vat). 
    bool hasSignal = ((GPIOC->IDR & GPIO_IDR_IDR3) == 0);

    switch (sensorState) {
        case WAIT_OBJECT:
            if (hasSignal) {
                debounceMillis = currentMillis; // Chot moc thoi gian vua thay vat
                sensorState = DEBOUNCE;         // Chuyen sang buoc loc nhieu
            }
            break;

        case DEBOUNCE:
            if (!hasSignal) {
                // Tin hieu bien mat truoc 20ms -> Chi la nhieu dien, quay ve cho lai
                sensorState = WAIT_OBJECT;
            } 
            else if (currentMillis - debounceMillis >= 20) {
                // Tin hieu giu on dinh suot 20ms -> Xac nhan co san pham that!
                productCount++;                 // Tang bo dem len 01 don vi
                isNewProduct = true;            // Bao co de kich coi keu 1 lan
                sensorState = WAIT_LEAVE;       // Chuyen sang cho vat di qua khoi cam bien
            }
            break;

        case WAIT_LEAVE:
            // Giu nguyen o day chung nao vat van con che truoc cam bien (chong cong don)
            // Khi vat da di qua han (!hasSignal), moi mo khoa de don san pham tiep theo
            if (!hasSignal) {
                sensorState = WAIT_OBJECT;
            }
            break;
    }

    return isNewProduct;
}

uint32_t sensor::getCount() {
    return productCount;
}