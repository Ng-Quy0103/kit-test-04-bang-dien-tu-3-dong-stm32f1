#ifndef FLASHSTORAGE_H
#define FLASHSTORAGE_H

#include "stm32f1xx.h"
#include <stdint.h>

/*
 * FlashStorage - Thu vien luu tru thong so cai dat va bo dem vao bo nho Flash noi cua STM32F1
 *
 * Muc dich:
 *      Giup luu lai cac thong so (Muc tieu ngay, So luong san pham, He so cong,
 *      Thoi gian 3 ca lam viec, Ngay hien tai) khi mat dien dot ngot.
 *
 * Nguyen ly:
 *      - Su dung 1 trang Flash o dia chi 0x0800F800 (vung 62KB, nam ngoai vung code chuong trinh).
 *      - Truy cap truc tiep thanh ghi FLASH (FLASH->KEYR, FLASH->SR, FLASH->CR, FLASH->AR),
 *        khong su dung thu vien HAL.
 *      - Co ma nhanarea MAGIC (0x564E4134) va Checksum de kiem tra du lieu hop le khi doc len.
 */

// Cau truc du lieu luu tru (kich thuoc chan 32 byte = 16 half-word 16-bit)
struct SavedParams {
    uint32_t magic;             // Ma danh dau du lieu hop le (0x564E4134 = 'VNA4')
    uint32_t targetDay;         // Muc tieu ngay (Dong 1)
    uint32_t count;             // So luong san pham hien tai (Dong 3)
    uint16_t factor;            // He so cong khi cam bien dem
    uint16_t reserved;          // Dem cho chan byte
    uint16_t shiftStart[3];     // Gio bat dau 3 ca (tinh bang phut 0..1439)
    uint16_t shiftEnd[3];       // Gio ket thuc 3 ca (tinh bang phut 0..1439)
    uint32_t lastDate;          // Ngay cuoi cung (YYMMDD) de kiem tra sang ngay moi
    uint32_t checksum;          // Ma kiem tra loi du lieu
};

class FlashStorage {
public:
    static const uint32_t FLASH_PAGE_ADDR = 0x0800F800UL;   // Dia chi trang Flash luu du lieu (62KB)
    static const uint32_t MAGIC_CODE      = 0x564E4134UL;   // Hang so nhan dien 'VNA4'

    // Tinh ma kiem tra checksum cho cau truc SavedParams
    static uint32_t calcChecksum(const SavedParams& p);

    // Kiem tra 1 ban ghi SavedParams co hop le khong (dung magic va dung checksum)
    static bool isValid(const SavedParams& p);

    // Doc thong so tu Flash noi STM32 (tra ve true neu du lieu hop le)
    bool load(SavedParams& p);

    // Ghi thong so xuong Flash noi STM32 bang thanh ghi (tra ve true neu ghi thanh cong)
    bool save(SavedParams& p);

private:
    void unlock();              // Mo khoa thanh ghi FLASH->CR
    void lock();                // Khoa lai thanh ghi FLASH->CR
    bool waitDone();            // Cho co BSY (Busy) trong FLASH->SR tat
    bool erasePage(uint32_t pageAddr);                      // Xoa 1 trang Flash
    bool writeHalfWord(uint32_t addr, uint16_t data);       // Ghi 2 byte (16-bit) vao Flash
};

#endif
