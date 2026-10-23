#include "led7seg.h"

// Vi tri chan (Port C) - theo so do mach
#define PIN_CK   10UL   // PC10: CLOCK
#define PIN_ST   11UL   // PC11: STROBE / LATCH
#define PIN_DT   12UL   // PC12: DATA

// Thoi gian 1/2 chu ky xung CLOCK (us). 5us -> CLOCK ~100kHz: du cham de chay on dinh qua day cap
#define HALF_PERIOD_US  5UL

// Ma 7 doan cua tung so 0-9 (bit = 1 la sang). Gan bit -> doan theo mach cua kit:
//   bit0 = c, bit1 = d, bit2 = e, bit3 = dau cham (dp)
//   bit4 = b, bit5 = a, bit6 = f, bit7 = g

const uint8_t Led7Seg::FONT[10] = {
    0x77,   // 0
    0x11,   // 1
    0xB6,   // 2
    0xB3,   // 3
    0xD1,   // 4
    0xE3,   // 5
    0xE7,   // 6
    0x31,   // 7
    0xF7,   // 8
    0xF3    // 9
};

// Neu mot con LED hien sai hinh (vi du so 1 bi hien o ben trai, so 7 bi lat) thi
// module do dang bi noi nguoc 180 do -> dat true tai vi tri do [dong][cot].
// Mac dinh tat ca = false.
const bool Led7Seg::FLIP_MAP[Led7Seg::ROWS][Led7Seg::DIGITS] = {
    { false, false, false, false, false },  // Dong 1
    { false, false, false, false, false },  // Dong 2
    { false, false, false, false, false }   // Dong 3
};

// Dao 180 do: a<->d, b<->e, c<->f; g va dp giu nguyen
uint8_t Led7Seg::rotate180(uint8_t v) {
    uint8_t r = v & 0x88;               // giu nguyen g (bit7) va dp (bit3)
    if (v & 0x20) r |= 0x02;            // a (bit5) -> d (bit1)
    if (v & 0x02) r |= 0x20;            // d (bit1) -> a (bit5)
    if (v & 0x10) r |= 0x04;            // b (bit4) -> e (bit2)
    if (v & 0x04) r |= 0x10;            // e (bit2) -> b (bit4)
    if (v & 0x01) r |= 0x40;            // c (bit0) -> f (bit6)
    if (v & 0x40) r |= 0x01;            // f (bit6) -> c (bit0)
    return r;
}

void Led7Seg::init() {
    // Bat clock Port C (IOPCEN = bit 4 cua APB2ENR)
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    // PC10, PC11, PC12 -> Output push-pull 2MHz (CNF = 00, MODE = 10 -> 0x2 moi chan)
    // PC10 -> CRH bit 8-11, PC11 -> bit 12-15, PC12 -> bit 16-19
    GPIOC->CRH &= ~(0xFFFUL << 8);
    GPIOC->CRH |=  (0x222UL << 8);

    // Trang thai nghi: CLOCK = 0, DATA = 0, STROBE = 1 (muc cao, chot bang suon len)
    GPIOC->BSRR = (1UL << PIN_ST) | (1UL << (PIN_CK + 16)) | (1UL << (PIN_DT + 16));

    // Bo dem chu ky CPU (DWT) dung de dinh thoi gian khong can delay
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    halfPeriodCycles = (SystemCoreClock / 1000000UL) * HALF_PERIOD_US;

    for (uint8_t r = 0; r < ROWS; r++)
        for (uint8_t c = 0; c < DIGITS; c++)
            seg[r][c] = SEG_BLANK;

    state     = ST_IDLE;
    byteIdx   = 0;
    bitIdx    = 0;
    lastCycle = 0;
    dirty     = true;   // day khung rong ra LED ngay lan process() dau tien
}

void Led7Seg::setDigit(uint8_t row, uint8_t col, uint8_t digit) {
    if (row >= ROWS || col >= DIGITS) return;
    uint8_t base = (digit <= 9) ? FONT[digit] : SEG_BLANK;      // digit > 9 => o trong
    uint8_t v = base | (seg[row][col] & SEG_DOT);               // giu nguyen dau cham
    if (seg[row][col] != v) {
        seg[row][col] = v;
        dirty = true;
    }
}

void Led7Seg::setDot(uint8_t row, uint8_t col, bool on) {
    if (row >= ROWS || col >= DIGITS) return;
    uint8_t v = on ? (seg[row][col] | SEG_DOT) : (seg[row][col] & (uint8_t)~SEG_DOT);
    if (seg[row][col] != v) {
        seg[row][col] = v;
        dirty = true;
    }
}

void Led7Seg::setNumber(uint8_t row, uint32_t value, bool leadingZero) {
    if (row >= ROWS) return;

    // Tach tung chu so tu phai sang trai (hang don vi o cot cuoi cung)
    uint8_t digit[DIGITS];
    for (int c = DIGITS - 1; c >= 0; c--) {
        digit[c] = (uint8_t)(value % 10);
        value /= 10;
    }

    bool show = leadingZero;
    for (uint8_t c = 0; c < DIGITS; c++) {
        // Bat dau hien thi tu so khac 0 dau tien; luon hien chu so cuoi cung (de hien so 0)
        if (digit[c] != 0 || c == DIGITS - 1) show = true;

        uint8_t v = show ? FONT[digit[c]] : SEG_BLANK;
        v |= (seg[row][c] & SEG_DOT);   // giu nguyen dau cham dang co
        if (seg[row][c] != v) {
            seg[row][c] = v;
            dirty = true;
        }
    }
}

void Led7Seg::clear() {
    for (uint8_t r = 0; r < ROWS; r++) {
        for (uint8_t c = 0; c < DIGITS; c++) {
            if (seg[r][c] != SEG_BLANK) {
                seg[r][c] = SEG_BLANK;
                dirty = true;
            }
        }
    }
}

// Xep du lieu theo thu tu day ra: byte gui DAU TIEN di xa nhat chuoi IC => nam o dong tren cung cua bang.
// Thu tu gui: dong 1 -> dong 2 -> dong 3 (dong 1 = hang tren cung), trong moi dong gui cot 0 -> cot cuoi.
// (Da kiem tra thuc te tren bang: neu gui nguoc dong 3 -> dong 1 thi bang bi dao cho dong 1 va dong 3.)
void Led7Seg::buildFrame() {
    uint8_t n = 0;
    for (uint8_t r = 0; r < ROWS; r++) {
        for (uint8_t c = 0; c < DIGITS; c++) {
            uint8_t v = seg[r][c];
            if (FLIP_MAP[r][c]) v = rotate180(v);
            frame[n++] = v;
        }
    }
}

// May trang thai khong chan: moi lan goi chi lam toi da 1 buoc (1 thay doi muc chan),
// va chi lam khi da du HALF_PERIOD_US ke tu buoc truoc.
// Moi bit gom 2 buoc: SETUP (dat DATA, CLOCK=0) -> CLOCK (CLOCK=1). Cuoi cung: STROBE 1->0->1.
void Led7Seg::process() {
    uint32_t now = DWT->CYCCNT;

    if (state == ST_IDLE) {
        if (!dirty) return;
        buildFrame();
        dirty     = false;
        byteIdx   = 0;
        bitIdx    = 0;
        state     = ST_SETUP;
        lastCycle = now - halfPeriodCycles;     // cho phep chay buoc dau tien ngay
    }

    if ((uint32_t)(now - lastCycle) < halfPeriodCycles) return;
    lastCycle = now;

    switch (state) {
    case ST_SETUP:
        // Gui MSB truoc. Dat DATA va keo CLOCK xuong trong cung 1 lenh ghi BSRR
        if (frame[byteIdx] & (0x80 >> bitIdx))
            GPIOC->BSRR = (1UL << PIN_DT) | (1UL << (PIN_CK + 16));
        else
            GPIOC->BSRR = (1UL << (PIN_DT + 16)) | (1UL << (PIN_CK + 16));
        state = ST_CLOCK;
        break;

    case ST_CLOCK:
        GPIOC->BSRR = (1UL << PIN_CK);      // suon len: IC doc bit
        if (++bitIdx >= 8) {
            bitIdx = 0;
            byteIdx++;
        }
        state = (byteIdx >= TOTAL_BYTES) ? ST_LATCH_LOW : ST_SETUP;
        break;

    case ST_LATCH_LOW:
        GPIOC->BSRR = (1UL << (PIN_ST + 16));
        state = ST_LATCH_HIGH;
        break;

    case ST_LATCH_HIGH:
        // STROBE len cao: chot toan bo du lieu ra LED. Dua CLOCK, DATA ve 0 cho lan sau
        GPIOC->BSRR = (1UL << PIN_ST) | (1UL << (PIN_CK + 16)) | (1UL << (PIN_DT + 16));
        state = ST_IDLE;
        break;

    default:
        state = ST_IDLE;
        break;
    }
}
