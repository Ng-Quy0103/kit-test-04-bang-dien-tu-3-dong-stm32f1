#include "Ds1307.h"

#define PIN_SCL  6UL    // PB6
#define PIN_SDA  7UL    // PB7

#define SCL_HIGH()  (GPIOB->BSRR = (1UL << PIN_SCL))
#define SCL_LOW()   (GPIOB->BSRR = (1UL << (PIN_SCL + 16)))
#define SDA_HIGH()  (GPIOB->BSRR = (1UL << PIN_SDA))
#define SDA_LOW()   (GPIOB->BSRR = (1UL << (PIN_SDA + 16)))
#define SDA_READ()  ((GPIOB->IDR & (1UL << PIN_SDA)) != 0)

// Cho 5us bang bo dem chu ky CPU (DWT->CYCCNT). DS1307 toi da 100kHz => moi nua bit >= 4.7us
void Ds1307::waitHalfBit() {
    uint32_t cycles = (SystemCoreClock / 1000000UL) * 5UL;
    uint32_t t0 = DWT->CYCCNT;
    while ((uint32_t)(DWT->CYCCNT - t0) < cycles) { }
}

void Ds1307::init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    // PB6 (CRL bit 24-27), PB7 (bit 28-31): Output open-drain 2MHz (CNF = 01, MODE = 10 -> 0x6)
    GPIOB->CRL &= ~(0xFFUL << 24);
    GPIOB->CRL |=  (0x66UL << 24);

    // Bo dem chu ky CPU (dung chung voi led7seg)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    SCL_HIGH();
    SDA_HIGH();     // nha 2 duong len muc cao (bus ranh)
}

void Ds1307::start() {
    SDA_HIGH(); SCL_HIGH(); waitHalfBit();
    SDA_LOW();  waitHalfBit();
    SCL_LOW();  waitHalfBit();
}

void Ds1307::stop() {
    SDA_LOW();  waitHalfBit();
    SCL_HIGH(); waitHalfBit();
    SDA_HIGH(); waitHalfBit();
}

bool Ds1307::writeByte(uint8_t d) {
    for (int i = 0; i < 8; i++) {
        if (d & 0x80) SDA_HIGH(); else SDA_LOW();
        d <<= 1;
        waitHalfBit();
        SCL_HIGH(); waitHalfBit();
        SCL_LOW();  waitHalfBit();
    }
    // Bit thu 9: nha SDA, slave keo xuong = ACK
    SDA_HIGH(); waitHalfBit();
    SCL_HIGH(); waitHalfBit();
    bool ack = !SDA_READ();
    SCL_LOW();  waitHalfBit();
    return ack;
}

uint8_t Ds1307::readByte(bool ack) {
    uint8_t d = 0;
    SDA_HIGH();     // nha SDA cho slave dieu khien
    for (int i = 0; i < 8; i++) {
        waitHalfBit();
        SCL_HIGH(); waitHalfBit();
        d <<= 1;
        if (SDA_READ()) d |= 1;
        SCL_LOW();
    }
    // Master tra loi: ACK (keo SDA xuong) neu con doc tiep, NACK neu la byte cuoi
    if (ack) SDA_LOW(); else SDA_HIGH();
    waitHalfBit();
    SCL_HIGH(); waitHalfBit();
    SCL_LOW();  waitHalfBit();
    SDA_HIGH();
    return d;
}

bool Ds1307::readTime(RtcTime& t) {
    // Dat con tro thanh ghi ve 0x00
    start();
    if (!writeByte(ADDR_WRITE)) { stop(); return false; }
    if (!writeByte(0x00))       { stop(); return false; }

    // Doc 7 thanh ghi lien tiep: giay, phut, gio, thu, ngay, thang, nam
    start();
    if (!writeByte(ADDR_READ))  { stop(); return false; }
    uint8_t sec   = readByte(true);
    uint8_t min   = readByte(true);
    uint8_t hour  = readByte(true);
    (void)readByte(true);               // thu (khong dung)
    uint8_t date  = readByte(true);
    uint8_t month = readByte(true);
    uint8_t year  = readByte(false);
    stop();

    // Bit 7 cua thanh ghi giay la co dung dao dong (CH) -> bo di
    sec &= 0x7F;
    hour &= 0x3F;       // che do 24h

    // Kiem tra du lieu BCD hop le (bus ho -> doc ra 0xFF)
    if ((sec & 0x0F) > 9 || (min & 0x0F) > 9 || (hour & 0x0F) > 9 ||
        (date & 0x0F) > 9 || (month & 0x0F) > 9 || (year & 0x0F) > 9 ||
        (sec >> 4) > 5 || (min >> 4) > 5 || (year >> 4) > 9) return false;

    uint8_t h = bcdToDec(hour), mo = bcdToDec(month), d = bcdToDec(date);
    if (h > 23 || mo < 1 || mo > 12 || d < 1 || d > 31) return false;

    t.second = bcdToDec(sec);
    t.minute = bcdToDec(min);
    t.hour   = h;
    t.date   = d;
    t.month  = mo;
    t.year   = bcdToDec(year);
    return true;
}

bool Ds1307::writeTime(const RtcTime& t) {
    start();
    if (!writeByte(ADDR_WRITE)) { stop(); return false; }
    bool ok = writeByte(0x00);                      // bat dau ghi tu thanh ghi giay
    ok &= writeByte(decToBcd(t.second) & 0x7F);     // bit CH = 0 => dong ho chay
    ok &= writeByte(decToBcd(t.minute));
    ok &= writeByte(decToBcd(t.hour) & 0x3F);       // bit 6 = 0 => che do 24h
    ok &= writeByte(1);                             // thu (khong dung)
    ok &= writeByte(decToBcd(t.date));
    ok &= writeByte(decToBcd(t.month));
    ok &= writeByte(decToBcd(t.year));
    stop();
    return ok;
}

// Doc 'len' byte tu bo nho RAM cua DS1307 (bat dau tu thanh ghi 0x08 + offset)
bool Ds1307::readRam(uint8_t offset, uint8_t* data, uint8_t len) {
    if (len == 0 || (uint16_t)offset + len > 56) return false;

    start();
    if (!writeByte(ADDR_WRITE))           { stop(); return false; }
    if (!writeByte((uint8_t)(0x08 + offset))) { stop(); return false; }

    start();
    if (!writeByte(ADDR_READ))            { stop(); return false; }
    for (uint8_t i = 0; i < len; i++) {
        data[i] = readByte(i + 1 < len);  // byte cuoi cung gui NACK (false)
    }
    stop();
    return true;
}

// Ghi 'len' byte vao bo nho RAM co pin nuoi cua DS1307 (bat dau tu thanh ghi 0x08 + offset)
bool Ds1307::writeRam(uint8_t offset, const uint8_t* data, uint8_t len) {
    if (len == 0 || (uint16_t)offset + len > 56) return false;

    start();
    if (!writeByte(ADDR_WRITE))           { stop(); return false; }
    if (!writeByte((uint8_t)(0x08 + offset))) { stop(); return false; }
    bool ok = true;
    for (uint8_t i = 0; i < len; i++) {
        ok &= writeByte(data[i]);
    }
    stop();
    return ok;
}
