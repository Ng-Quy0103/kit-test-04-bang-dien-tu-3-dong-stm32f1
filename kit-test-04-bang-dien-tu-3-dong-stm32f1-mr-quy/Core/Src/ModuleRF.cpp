#include "ModuleRF.h"

// Bang tra ma 4 bit (0..15) tu 4 chan PA0, PB3, PB4, PB5 sang so phim tren remote (1..12)
// Gia tri 0 nghia la khong co phim hoac ma khong hop le
const uint8_t ModuleRF::KEY_MAP[16] = { 0, 8, 4, 12, 2, 10, 6, 0, 1, 9, 5, 11, 3, 11, 7, 0 };

ModuleRF::ModuleRF() {
    currentKey     = KEY_NONE;
    pressMs        = 0;
    lastValidKeyMs = 0;
    longSent       = false;
}

void ModuleRF::init() {
    // 1. Bat xung clock cho AFIO, Port A va Port B tren bus APB2 (RM0008 trang 114)
    RCC->APB2ENR |= (RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN);

    // 2. Tat JTAG-DP, chi giu lai SW-DP (SWJ_CFG = 010) de giai phong chan PB3, PB4 lam GPIO thuong
    //    (Mac dinh khi reset, PB3 la JTDO va PB4 la JNTRST cua mach nap JTAG)
    uint32_t mapr = AFIO->MAPR;
    mapr &= ~AFIO_MAPR_SWJ_CFG;
    mapr |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
    AFIO->MAPR = mapr;

    // 3. Cau hinh PA0, PB3, PB4, PB5 la Input with pull-up / pull-down (CNF = 10, MODE = 00 -> 0x8)
    //    - PA0 nam o 4 bit thap (bit 0..3) cua thanh ghi GPIOA->CRL
    GPIOA->CRL &= ~(0xFUL << 0);
    GPIOA->CRL |=  (0x8UL << 0);
    //    - PB3 (bit 12..15), PB4 (bit 16..19), PB5 (bit 20..23) cua thanh ghi GPIOB->CRL
    GPIOB->CRL &= ~(0xFFFUL << 12);
    GPIOB->CRL |=  (0x888UL << 12);

    // 4. Keo tat ca 4 chan xuong muc thap (Pull-Down: ghi 1 vao bit BRx o nua cao thanh ghi BSRR)
    //    Khi khong bam phim, cac chan o muc 0; khi bam phim, module RF day chan len muc 1 (3.3V)
    GPIOA->BSRR = (1UL << (0 + 16));
    GPIOB->BSRR = (1UL << (3 + 16)) | (1UL << (4 + 16)) | (1UL << (5 + 16));
}

// Doc trang thai 4 chan GPIO tu thanh ghi IDR va ghep thanh ma 4 bit (0..15)
uint8_t ModuleRF::readRaw() {
    uint8_t k = 0;
    if (GPIOA->IDR & (1UL << 0)) k |= 8;    // PA0 -> bit 3
    if (GPIOB->IDR & (1UL << 3)) k |= 4;    // PB3 -> bit 2
    if (GPIOB->IDR & (1UL << 4)) k |= 2;    // PB4 -> bit 1
    if (GPIOB->IDR & (1UL << 5)) k |= 1;    // PB5 -> bit 0
    return k;
}

// Quet phim khong chan (non-blocking), goi lien tuc trong while(1)
bool ModuleRF::update(uint32_t now, RfEvent& ev) {
    uint8_t raw = readRaw();
    uint8_t k = KEY_MAP[raw & 0x0F];

    if (k != KEY_NONE) {
        // Dang thu duoc tin hieu phim k
        lastValidKeyMs = now;   // Cap nhat moc thoi gian cuoi cung con thay song RF

        if (k != currentKey) {
            // Bat dau nhan mot phim moi
            currentKey = k;
            pressMs    = now;
            longSent   = false;
        } else {
            // Vua giu nguyen phim do: kiem tra xem da du 1000ms (1 giay) chua
            if (!longSent && (now - pressMs >= LONG_PRESS_MS)) {
                longSent  = true;           // Danh dau da gui su kien bam giu
                ev.key    = currentKey;
                ev.isLong = true;           // Bao su kien BAM GIU
                return true;
            }
        }
    } else {
        // Khong doc duoc tin hieu (raw = 0)
        if (currentKey != KEY_NONE) {
            // Cho 150ms xem co phai chi bi rot song RF tam thoi hay nguoi dung da nha tay that
            if (now - lastValidKeyMs > DROP_TOLERANCE_MS) {
                uint8_t releasedKey = currentKey;
                bool    wasLong     = longSent;

                // Reset trang thai ve khong co phim
                currentKey = KEY_NONE;
                longSent   = false;

                // Neu truoc do chua du 1 giay (chua phat su kien bam giu) -> tinh la BAM NHA NHANH
                if (!wasLong) {
                    ev.key    = releasedKey;
                    ev.isLong = false;      // Bao su kien BAM NGAN
                    return true;
                }
            }
        }
    }
    return false;
}
