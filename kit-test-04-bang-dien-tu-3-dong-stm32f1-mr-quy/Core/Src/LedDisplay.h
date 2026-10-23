#ifndef LEDDISPLAY_H
#define LEDDISPLAY_H

#include <stdint.h>
#include "led7seg.h"

/*
 * LedDisplay - Lop hien thi muc cao cho bang LED 3 dong x 5 so, nam tren Led7Seg.
 *
 * Giu 1 bo dem hinh anh (digit + dau cham), co them:
 *      - Ghi so / truong so (co hoac khong dem so 0 o dau), gio:phut co dau cham
 *      - Con tro nhap nhay (dung khi cai dat)
 *      - To day 1 chu so len toan bo LED (che do test)
 *
 * Cach dung:
 *      disp.init();
 *      disp.setNumber(0, 1000);
 *      while (1) { disp.process(millis()); }
 */
class LedDisplay {
public:
    static const uint8_t  ROWS     = Led7Seg::ROWS;
    static const uint8_t  COLS     = Led7Seg::DIGITS;
    static const uint8_t  BLANK    = 0xFF;      // gia tri "o trong" (tat LED)
    static const uint32_t BLINK_MS = 400;       // chu ky nhap nhay con tro (nua chu ky)

    void init();

    // Goi lien tuc trong vong lap chinh: cap nhat nhap nhay, ve lai khi co thay doi, day ra LED
    void process(uint32_t now);

    void clear();                                               // xoa het (digit + dau cham + con tro)
    void setDigit(uint8_t row, uint8_t col, uint8_t digit);     // 0..9, hoac BLANK
    void setDot(uint8_t row, uint8_t col, bool on);

    // Ghi so can phai tren ca dong (leadingZero = false: bo so 0 o dau)
    void setNumber(uint8_t row, uint32_t value, bool leadingZero = false);

    // Ghi so vao truong gom 'count' chu so bat dau tu cot firstCol
    void setField(uint8_t row, uint8_t firstCol, uint8_t count, uint32_t value, bool zeroPad);

    // Ghi gio:phut dang "HH.MM" vao cot 1..4 (dau cham sau HH)
    void setHourMinute(uint8_t row, uint8_t hour, uint8_t minute);

    // To tat ca LED cung 1 chu so (dung cho che do test)
    void fillAll(uint8_t digit, bool dots);

    void setCursor(uint8_t row, uint8_t col);   // con tro nhap nhay tai o (row, col)
    void clearCursor();

private:
    Led7Seg  led;
    uint8_t  digit[ROWS][COLS];
    bool     dot[ROWS][COLS];
    bool     cursorOn;
    uint8_t  curRow, curCol;
    bool     blinkVisible;
    uint32_t lastBlink;
    uint32_t lastNow;
    bool     dirty;

    void render();
};

#endif