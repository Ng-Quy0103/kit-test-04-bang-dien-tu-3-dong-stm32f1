#include "LedDisplay.h"

void LedDisplay::init() {
    led.init();
    cursorOn     = false;
    curRow       = 0;
    curCol       = 0;
    blinkVisible = true;
    lastBlink    = 0;
    lastNow      = 0;
    clear();
}

void LedDisplay::clear() {
    for (uint8_t r = 0; r < ROWS; r++) {
        for (uint8_t c = 0; c < COLS; c++) {
            digit[r][c] = BLANK;
            dot[r][c]   = false;
        }
    }
    cursorOn = false;
    dirty    = true;
}

void LedDisplay::setDigit(uint8_t row, uint8_t col, uint8_t d) {
    if (row >= ROWS || col >= COLS) return;
    digit[row][col] = (d <= 9) ? d : BLANK;
    dirty = true;
}

void LedDisplay::setDot(uint8_t row, uint8_t col, bool on) {
    if (row >= ROWS || col >= COLS) return;
    dot[row][col] = on;
    dirty = true;
}

void LedDisplay::setField(uint8_t row, uint8_t firstCol, uint8_t count, uint32_t value, bool zeroPad) {
    if (row >= ROWS || count == 0 || count > 8 || (firstCol + count) > COLS) return;

    uint8_t d[8];
    for (int i = count - 1; i >= 0; i--) {      // tach tung chu so tu phai sang trai
        d[i]  = (uint8_t)(value % 10);
        value /= 10;
    }

    bool show = zeroPad;
    for (uint8_t i = 0; i < count; i++) {
        if (d[i] != 0 || i == count - 1) show = true;       // luon hien chu so cuoi (de hien so 0)
        digit[row][firstCol + i] = show ? d[i] : BLANK;
    }
    dirty = true;
}

void LedDisplay::setNumber(uint8_t row, uint32_t value, bool leadingZero) {
    setField(row, 0, COLS, value, leadingZero);
}

void LedDisplay::setHourMinute(uint8_t row, uint8_t hour, uint8_t minute) {
    setField(row, 1, 2, hour,   true);
    setField(row, 3, 2, minute, true);
    setDot(row, 2, true);
}

void LedDisplay::fillAll(uint8_t d, bool dots) {
    for (uint8_t r = 0; r < ROWS; r++) {
        for (uint8_t c = 0; c < COLS; c++) {
            digit[r][c] = (d <= 9) ? d : BLANK;
            dot[r][c]   = dots;
        }
    }
    cursorOn = false;
    dirty    = true;
}

void LedDisplay::setCursor(uint8_t row, uint8_t col) {
    if (row >= ROWS || col >= COLS) return;
    cursorOn     = true;
    curRow       = row;
    curCol       = col;
    blinkVisible = true;        // vua di chuyen con tro thi cho sang ngay
    lastBlink    = lastNow;
    dirty        = true;
}

void LedDisplay::clearCursor() {
    cursorOn = false;
    dirty    = true;
}

void LedDisplay::render() {
    for (uint8_t r = 0; r < ROWS; r++) {
        for (uint8_t c = 0; c < COLS; c++) {
            uint8_t v = digit[r][c];
            if (cursorOn && !blinkVisible && r == curRow && c == curCol) v = BLANK;     // pha tat cua nhap nhay
            led.setDigit(r, c, v);
            led.setDot(r, c, dot[r][c]);
        }
    }
}

void LedDisplay::process(uint32_t now) {
    lastNow = now;

    if (cursorOn && (uint32_t)(now - lastBlink) >= BLINK_MS) {
        lastBlink    = now;
        blinkVisible = !blinkVisible;
        dirty        = true;
    }

    if (dirty) {
        render();
        dirty = false;
    }

    led.process();      // day du lieu ra LED neu co thay doi (khong chan)
}
