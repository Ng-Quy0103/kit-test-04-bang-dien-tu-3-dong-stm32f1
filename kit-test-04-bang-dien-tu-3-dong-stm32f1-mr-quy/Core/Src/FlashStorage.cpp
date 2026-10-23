#include "FlashStorage.h"

// Tinh checksum bang phep cong + XOR cac truong du lieu
uint32_t FlashStorage::calcChecksum(const SavedParams& p) {
    uint32_t sum = p.magic ^ 0xA5A5A5A5UL;
    sum += p.targetDay;
    sum += p.count;
    sum += p.factor;
    for (uint8_t i = 0; i < 3; i++) {
        sum += ((uint32_t)p.shiftStart[i] << 16) | p.shiftEnd[i];
    }
    sum += p.lastDate;
    return sum;
}

bool FlashStorage::isValid(const SavedParams& p) {
    if (p.magic != MAGIC_CODE) return false;
    return (p.checksum == calcChecksum(p));
}

// Mo khoa bo nho Flash bang cach ghi 2 KEY vao thanh ghi FLASH->KEYR (RM0008 trang 59)
void FlashStorage::unlock() {
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123UL;     // KEY1
        FLASH->KEYR = 0xCDEF89ABUL;     // KEY2
    }
}

// Khoa lai bo nho Flash de tranh ghi nham
void FlashStorage::lock() {
    FLASH->CR |= FLASH_CR_LOCK;
}

// Cho thao tac Flash hoan tat (co BSY = 0) co gioi han vong lap de khong treo may
bool FlashStorage::waitDone() {
    uint32_t timeout = 200000UL;
    while ((FLASH->SR & FLASH_SR_BSY) && (--timeout > 0)) { }
    if (timeout == 0) return false;

    // Kiem tra co loi ghi (PGERR hoac WRPRTERR)
    if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) {
        FLASH->SR = (FLASH_SR_PGERR | FLASH_SR_WRPRTERR);   // Xoa co loi bang cach ghi 1
        return false;
    }
    return true;
}

// Xoa 1 trang Flash tai dia chi pageAddr
bool FlashStorage::erasePage(uint32_t pageAddr) {
    if (!waitDone()) return false;

    FLASH->CR |= FLASH_CR_PER;          // Bat bit Page Erase (PER)
    FLASH->AR  = pageAddr;              // Chon dia chi trang can xoa
    FLASH->CR |= FLASH_CR_STRT;         // Bat dau xoa (STRT = 1)

    bool ok = waitDone();
    FLASH->CR &= ~FLASH_CR_PER;         // Tat bit PER sau khi xoa xong
    return ok;
}

// Ghi 1 half-word (16-bit) vao dia chi addr trong Flash
bool FlashStorage::writeHalfWord(uint32_t addr, uint16_t data) {
    if (!waitDone()) return false;

    FLASH->CR |= FLASH_CR_PG;           // Bat che do Programming (PG = 1)
    *(__IO uint16_t*)addr = data;       // Ghi 16-bit xuong dia chi Flash

    bool ok = waitDone();
    FLASH->CR &= ~FLASH_CR_PG;          // Tat bit PG
    return ok;
}

// Doc cau truc SavedParams truc tiep tu dia chi Flash
bool FlashStorage::load(SavedParams& p) {
    const SavedParams* flashPtr = (const SavedParams*)FLASH_PAGE_ADDR;
    p = *flashPtr;
    return isValid(p);
}

// Luu cau truc SavedParams xuong Flash noi STM32
bool FlashStorage::save(SavedParams& p) {
    p.magic    = MAGIC_CODE;
    p.reserved = 0;
    p.checksum = calcChecksum(p);

    // Neu du lieu trong Flash da giong het thi khong can xoa/ghi lai (bao ve tuoi tho Flash)
    const SavedParams* currentInFlash = (const SavedParams*)FLASH_PAGE_ADDR;
    if (isValid(*currentInFlash) &&
        currentInFlash->targetDay     == p.targetDay     &&
        currentInFlash->count         == p.count         &&
        currentInFlash->factor        == p.factor        &&
        currentInFlash->shiftStart[0] == p.shiftStart[0] &&
        currentInFlash->shiftStart[1] == p.shiftStart[1] &&
        currentInFlash->shiftStart[2] == p.shiftStart[2] &&
        currentInFlash->shiftEnd[0]   == p.shiftEnd[0]   &&
        currentInFlash->shiftEnd[1]   == p.shiftEnd[1]   &&
        currentInFlash->shiftEnd[2]   == p.shiftEnd[2]   &&
        currentInFlash->lastDate      == p.lastDate) {
        return true;
    }

    unlock();
    bool ok = erasePage(FLASH_PAGE_ADDR);
    if (ok) {
        const uint16_t* src = (const uint16_t*)&p;
        uint32_t addr = FLASH_PAGE_ADDR;
        uint8_t halfWords = sizeof(SavedParams) / 2;

        for (uint8_t i = 0; i < halfWords; i++) {
            if (!writeHalfWord(addr, src[i])) {
                ok = false;
                break;
            }
            addr += 2;
        }
    }
    lock();
    return ok;
}
