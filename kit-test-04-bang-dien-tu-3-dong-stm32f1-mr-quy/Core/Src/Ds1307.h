#ifndef DS1307_H
#define DS1307_H

#include "stm32f1xx.h"
#include <stdint.h>

/*
 * Ds1307 - Driver IC thoi gian thuc DS1307 (I2C gia lap bang 2 chan GPIO)
 *
 * Phan cung: PB6 = SCL, PB7 = SDA (open-drain, co dien tro keo len tren mach)
 * Dia chi I2C cua DS1307: 0x68 (byte ghi 0xD0, byte doc 0xD1)
 *
 * Luu y: cho 5us giua cac muc chan I2C dung bo dem chu ky CPU (DWT), khong dung HAL_Delay.
 * Moi lan doc/ghi thoi gian mat khoang 1ms, chi goi 1 lan/giay.
 */
struct RtcTime {
    uint8_t hour;       // 0..23
    uint8_t minute;     // 0..59
    uint8_t second;     // 0..59
    uint8_t date;       // ngay 1..31
    uint8_t month;      // thang 1..12
    uint8_t year;       // nam 0..99 (2000..2099)
};

class Ds1307 {
public:
    void init();
    bool readTime(RtcTime& t);          // false: DS1307 khong tra loi hoac du lieu sai
    bool writeTime(const RtcTime& t);   // false: DS1307 khong tra loi (NACK)

private:
    static const uint8_t ADDR_WRITE = 0xD0;
    static const uint8_t ADDR_READ  = 0xD1;

    static uint8_t bcdToDec(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }
    static uint8_t decToBcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

    void waitHalfBit();
    void start();
    void stop();
    bool writeByte(uint8_t d);          // true = slave ACK
    uint8_t readByte(bool ack);
};

#endif
