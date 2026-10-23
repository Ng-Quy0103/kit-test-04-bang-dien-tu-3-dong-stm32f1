#ifndef MODULERF_H
#define MODULERF_H

#include "stm32f1xx.h"
#include <stdint.h>

/*
 * ModuleRF - Thu vien doc tin hieu tu module thu RF / RFID 12 phim
 *
 * Phan cung (4 chan data tu IC giai ma RF dua vao MCU):
 *      PA0 = bit 3 (MSB)
 *      PB3 = bit 2
 *      PB4 = bit 1
 *      PB5 = bit 0 (LSB)
 *
 * Nguyen ly:
 *      - Khi bam 1 phim tren remote, 4 chan PA0, PB3, PB4, PB5 xuat ra ma nhi phan 4 bit (1..14).
 *      - Dung bang tra KEY_MAP[16] de doi ma 4 bit sang so phim tu 1 den 12.
 *      - Ho tro phan biet 2 kieu bam:
 *          + Bam nha nhanh (< 1 giay) -> isLong = false (phat sinh su kien khi nha phim)
 *          + Bam giu (>= 1 giay)      -> isLong = true  (phat sinh su kien ngay khi du 1 giay)
 */

// Cau truc luu 1 su kien phim bam
struct RfEvent {
    uint8_t key;        // So thu tu phim: 1..12
    bool    isLong;     // true = bam giu (>= 1s), false = bam nha nhanh
};

class ModuleRF {
public:
    static const uint8_t  KEY_NONE          = 0;        // Khong co phim nao duoc bam
    static const uint8_t  KEY_BACK          = 11;       // Phim 11: lui con tro khi cai dat
    static const uint8_t  KEY_FORWARD       = 12;       // Phim 12: tien con tro khi cai dat
    static const uint32_t LONG_PRESS_MS     = 1000;     // Giu >= 1000ms (1 giay) thi tinh la bam giu
    static const uint32_t DROP_TOLERANCE_MS = 150;      // Thoi gian bu rot song RF tam thoi (150ms) de chong doi phim

    ModuleRF();
    void init();                                        // Cau hinh thanh ghi GPIO cho PA0, PB3, PB4, PB5
    bool update(uint32_t now, RfEvent& ev);             // Quet phim lien tuc (tra ve true khi co su kien phim moi)

private:
    static const uint8_t KEY_MAP[16];                   // Bang doi ma 4 bit sang phim 1..12

    uint8_t  currentKey;        // Phim dang duoc nhan giu hien tai (0 = khong co)
    uint32_t pressMs;           // Thoi diem bat dau nhan phim (dung de tinh 1 giay bam giu)
    uint32_t lastValidKeyMs;    // Thoi diem cuoi cung con thu duoc song RF cua phim
    bool     longSent;          // Co danh dau da bao su kien bam giu (tranh bao lap lai)

    uint8_t readRaw();          // Doc trang thai 4 chan PA0, PB3, PB4, PB5 thanh so 4 bit (0..15)
};

#endif
