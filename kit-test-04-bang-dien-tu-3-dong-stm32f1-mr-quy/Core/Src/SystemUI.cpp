#include "SystemUI.h"

SystemUI::SystemUI() {
    indicator = 0;
}

void SystemUI::init(Indicator* ind, uint32_t now) {
    indicator = ind;
    nowMs     = now;

    // Gia tri mac dinh khi khoi dong lan dau (neu chua co du lieu luu khi mat dien)
    targetDay = 1000;       // muc tieu ngay mac dinh = 1000
    count     = 0;          // san luong bat dau tu 0
    factor    = 1;          // moi lan cam bien thay san pham thi +1
    shiftStart[0] = 6 * 60;   shiftEnd[0] = 14 * 60;    // Ca 1: 06:00 - 14:00
    shiftStart[1] = 14 * 60;  shiftEnd[1] = 22 * 60;    // Ca 2: 14:00 - 22:00
    shiftStart[2] = 22 * 60;  shiftEnd[2] = 6 * 60;     // Ca 3: 22:00 - 06:00 (qua nua dem)

    mode         = MODE_NORMAL;
    clockValid   = false;
    clock.hour   = 0; clock.minute = 0; clock.second = 0;
    clock.date   = 1; clock.month  = 1; clock.year   = 26;
    targetNow    = 0;
    normalDirty  = true;    // ve man hinh chinh ngay lan dau
    dateKnown    = false;
    lastDate     = 0;
    flashPending = false;
    lastChangeMs = now;
    cur          = 0;
    testDigit    = 0;
    testMs       = now;
    for (uint8_t i = 0; i < MAX_EDIT_CELLS; i++) buf[i] = 0;

    disp.init();
    rf.init();
    rtc.init();

    // Khoi phuc cac thong so cai dat va bo dem da luu truoc khi mat dien
    loadSavedState();

    lastRtcMs = now;
    readClock();            // doc gio lan dau va kiem tra sang ngay moi
}

// Dong goi cac thong so hien tai vao cau truc SavedParams
void SystemUI::packState(SavedParams& p) const {
    p.magic      = FlashStorage::MAGIC_CODE;
    p.targetDay  = targetDay;
    p.count      = count;
    p.factor     = factor;
    p.reserved   = 0;
    for (uint8_t i = 0; i < 3; i++) {
        p.shiftStart[i] = shiftStart[i];
        p.shiftEnd[i]   = shiftEnd[i];
    }
    p.lastDate   = lastDate;
    p.checksum   = FlashStorage::calcChecksum(p);
}

// Khoi phuc cac thong so tu cau truc SavedParams hop le
void SystemUI::unpackState(const SavedParams& p) {
    targetDay = clampU32(p.targetDay, 0, MAX_VALUE);
    count     = clampU32(p.count,     0, MAX_VALUE);
    factor    = (uint16_t)clampU32(p.factor, 1, 999);
    for (uint8_t i = 0; i < 3; i++) {
        shiftStart[i] = (p.shiftStart[i] < 1440) ? p.shiftStart[i] : (uint16_t)(6 * 60);
        shiftEnd[i]   = (p.shiftEnd[i]   < 1440) ? p.shiftEnd[i]   : (uint16_t)(14 * 60);
    }
    if (p.lastDate > 0) {
        lastDate  = p.lastDate;
        dateKnown = true;
    }
}

// Doc du lieu luu tru khi moi cap dien: uu tien RAM co pin nuoi cua DS1307, sau do den Flash noi STM32
void SystemUI::loadSavedState() {
    SavedParams p;
    if (rtc.readRam(0, (uint8_t*)&p, sizeof(SavedParams)) && FlashStorage::isValid(p)) {
        unpackState(p);
        return;
    }
    if (flash.load(p)) {
        unpackState(p);
        // Dong bo nguoc len RAM DS1307
        rtc.writeRam(0, (const uint8_t*)&p, sizeof(SavedParams));
    }
}

// Luu ngay vao RAM DS1307 (chong mat dien tuc thi) va luu vao Flash noi STM32
void SystemUI::saveStateNow(bool writeFlash) {
    SavedParams p;
    packState(p);
    rtc.writeRam(0, (const uint8_t*)&p, sizeof(SavedParams));

    if (writeFlash) {
        flash.save(p);
        flashPending = false;
    } else {
        flashPending = true;
        lastChangeMs = nowMs;
    }
}

// Ham nay goi lien tuc trong while(1), KHONG chan, KHONG delay
void SystemUI::process(uint32_t now) {
    nowMs = now;

    // 1. Cu 1 giay doc DS1307 1 lan
    if ((uint32_t)(now - lastRtcMs) >= RTC_PERIOD_MS) {
        lastRtcMs = now;
        readClock();
    }

    // 2. Doc remote RF, co phim thi xu ly
    RfEvent ev;
    if (rf.update(now, ev)) {
        handleKey(ev);
    }

    // 3. Che do test LED: moi 0.5 giay doi sang so tiep theo, het so 9 thi tu thoat
    if (mode == MODE_TEST_LED && (uint32_t)(now - testMs) >= TEST_STEP_MS) {
        testMs = now;
        if (++testDigit > 9) leaveMode(false);
        else                 disp.fillAll(testDigit, true);
    }

    // 4. Neu co thay doi bo dem san pham, sau 2s ngung dem thi chot luon xuong Flash noi
    if (flashPending && (uint32_t)(now - lastChangeMs) >= FLASH_SAVE_DELAY_MS) {
        SavedParams p;
        packState(p);
        flash.save(p);
        flashPending = false;
    }

    // 5. Man hinh chinh chi ve lai khi co thay doi
    if (mode == MODE_NORMAL && normalDirty) renderNormal();

    // 6. Day du lieu ra LED
    disp.process(now);
}

// Cam bien phat hien san pham: san luong tang them 'he so' (mac dinh = 1) va luu lai ngay
void SystemUI::onProduct() {
    count += factor;
    if (count > MAX_VALUE) count = MAX_VALUE;   // khong vuot qua 99999 (5 so LED)
    normalDirty = true;
    saveStateNow(false);                        // ghi ngay vao RAM DS1307, cho 2s ghi Flash
}

// Cac ham cap nhat thong so tu Modbus RTU (Master ghi xuong mach)
void SystemUI::setTargetDay(uint32_t v) {
    targetDay = clampU32(v, 0, MAX_VALUE);
    recalcTargetNow();
    normalDirty = true;
    saveStateNow(true);
}

void SystemUI::setCount(uint32_t v) {
    count = clampU32(v, 0, MAX_VALUE);
    normalDirty = true;
    saveStateNow(true);
}

void SystemUI::setFactor(uint16_t v) {
    factor = (uint16_t)clampU32(v, 1, 999);
    saveStateNow(true);
}

void SystemUI::setShiftStart(uint8_t shift, uint16_t minutesFromMidnight) {
    shiftStart[shift % 3] = (uint16_t)(minutesFromMidnight % 1440);
    recalcTargetNow();
    normalDirty = true;
    saveStateNow(true);
}

void SystemUI::setShiftEnd(uint8_t shift, uint16_t minutesFromMidnight) {
    shiftEnd[shift % 3] = (uint16_t)(minutesFromMidnight % 1440);
    recalcTargetNow();
    normalDirty = true;
    saveStateNow(true);
}

bool SystemUI::setTime(const RtcTime& tIn) {
    RtcTime t = tIn;
    t.year   = (uint8_t)(t.year % 100);
    t.month  = (uint8_t)clampU32(t.month, 1, 12);
    t.date   = (uint8_t)clampU32(t.date, 1, daysInMonth(t.month, t.year));
    t.hour   = (uint8_t)clampU32(t.hour, 0, 23);
    t.minute = (uint8_t)clampU32(t.minute, 0, 59);
    t.second = (uint8_t)clampU32(t.second, 0, 59);

    if (!rtc.writeTime(t)) return false;
    clock      = t;
    clockValid = true;
    dateKnown  = true;
    lastDate   = (uint32_t)t.year * 10000UL + t.month * 100UL + t.date;
    recalcTargetNow();
    normalDirty = true;
    saveStateNow(true);
    return true;
}

void SystemUI::beep() {
    if (indicator) indicator->triggerSingleBeep(nowMs);
}

// Xu ly 1 su kien phim tu remote RF
void SystemUI::handleKey(const RfEvent& ev) {
    switch (mode) {

    case MODE_NORMAL:
        // O man hinh chinh: phai GIU phim moi vao cai dat
        if (!ev.isLong) break;
        switch (ev.key) {
            case 1: enterMode(MODE_SET_TIME);   break;  // giu 1: cai dat thoi gian thuc
            case 2: enterMode(MODE_SET_TARGET); break;  // giu 2: cai dat muc tieu ngay
            case 3: enterMode(MODE_SET_COUNT);  break;  // giu 3: chinh san luong
            case 4: enterMode(MODE_SET_SHIFT1); break;  // giu 4: cai dat ca 1
            case 5: enterMode(MODE_SET_SHIFT2); break;  // giu 5: cai dat ca 2
            case 6: enterMode(MODE_SET_SHIFT3); break;  // giu 6: cai dat ca 3
            case 7: enterMode(MODE_SET_FACTOR); break;  // giu 7: cai dat he so
            case 8: enterMode(MODE_TEST_LED);   break;  // giu 8: test LED
            default: break;
        }
        break;

    case MODE_TEST_LED:
        // Giu phim bat ky de thoat test LED
        if (ev.isLong) leaveMode(false);
        break;

    default:
        // Dang trong che do cai dat
        if (ev.isLong) {
            // Giu phim 11 hoac 12: LUU va thoat ve man hinh chinh
            if (ev.key == 11 || ev.key == 12) leaveMode(true);
            break;
        }

        uint8_t n;
        cellsOf(n);     // lay so o nhap n cua che do hien tai
        if (ev.key >= 1 && ev.key <= 10) {
            buf[cur] = ev.key % 10;         // phim 1..9 = so 1..9, phim 10 = so 0 (10 % 10 = 0)
        } else if (ev.key == ModuleRF::KEY_BACK) {
            cur = (cur == 0) ? (n - 1) : (cur - 1);         // lui con tro (quay vong)
        } else if (ev.key == ModuleRF::KEY_FORWARD) {
            cur = (cur + 1 >= n) ? 0 : (cur + 1);           // tien con tro (quay vong)
        }
        renderEdit();
        break;
    }
}

void SystemUI::enterMode(Mode m) {
    mode = m;
    if (m == MODE_TEST_LED) {
        testDigit = 0;
        testMs    = nowMs;
        disp.fillAll(0, true);      // bat dau test bang so 0
        return;
    }
    cur = 0;            // con tro ve o dau tien
    loadBuffer();       // nap gia tri hien tai de nguoi dung sua
    renderEdit();
}

void SystemUI::leaveMode(bool save) {
    if (save) applyBuffer();        // luu gia tri vua nhap
    mode = MODE_NORMAL;
    disp.clearCursor();
    normalDirty = true;             // ve lai man hinh chinh
}

// Ve man hinh chinh theo dung README.md:
//   0 = Dong 1 (tren cung): Muc tieu can thuc hien duoc trong ngay hien tai (targetDay, mac dinh 1000)
//   1 = Dong 2 (o giua):    Muc tieu tai thoi diem hien tai can dat duoc - cap nhat realtime tu DS1307 (targetNow)
//   2 = Dong 3 (duoi cung): So luong san pham da san xuat duoc - tang +1 khi cam bien co tin hieu (count)
void SystemUI::renderNormal() {
    disp.clear();
    disp.setNumber(0, targetDay);   // Dong 1: Muc tieu ngay (1000)
    disp.setNumber(1, targetNow);   // Dong 2: Muc tieu tai thoi diem hien tai (tinh realtime theo gio DS1307)
    disp.setNumber(2, count);       // Dong 3: So luong san pham da san xuat duoc (tang khi cam bien dem)
    normalDirty = false;
}

void SystemUI::readClock() {
    RtcTime t;
    if (!rtc.readTime(t)) return;   // doc loi thi giu nguyen gia tri cu
    clock      = t;
    clockValid = true;
    checkNewDay();
    recalcTargetNow();
}

// Phat hien sang ngay moi (nam/thang/ngay doi) thi reset "San luong hien tai - Dong 3" ve 0 va luu lai
void SystemUI::checkNewDay() {
    uint32_t today = (uint32_t)clock.year * 10000UL + clock.month * 100UL + clock.date;   // dang YYMMDD
    if (!dateKnown) {
        // Lan doc dau tien (khi chua co ngay luu trong Flash/RAM): ghi nho ngay hien tai va luu lai
        dateKnown = true;
        lastDate  = today;
        saveStateNow(false);
        return;
    }
    if (today != lastDate) {
        lastDate    = today;
        count       = 0;            // Sang ngay moi: Reset "San luong hien tai - Dong 3" ve 0
        normalDirty = true;
        saveStateNow(true);         // Luu ngay trang thai moi xuong RAM DS1307 va Flash
    }
}

// Muc tieu tai thoi diem hien tai = Muc tieu ngay x (so phut da lam / tong so phut 3 ca)
// Ngay san xuat bat dau tu gio bat dau cua ca 1
uint32_t SystemUI::calcTargetNow() const {
    if (!clockValid) return 0;

    // Tinh thoi luong (phut) cua tung ca va tong 3 ca. "+ 1440" de ca qua nua dem van ra so duong
    uint32_t dur[3];
    uint32_t total = 0;
    for (uint8_t i = 0; i < 3; i++) {
        dur[i] = (uint32_t)(shiftEnd[i] + 1440 - shiftStart[i]) % 1440;
        total += dur[i];
    }
    if (total == 0) return 0;

    // t = so phut tu luc bat dau ca 1 den bay gio
    uint32_t nowMin = (uint32_t)clock.hour * 60 + clock.minute;
    uint32_t t = (nowMin + 1440 - shiftStart[0]) % 1440;

    // Cong don so phut da lam trong tung ca
    uint32_t worked = 0;
    for (uint8_t i = 0; i < 3; i++) {
        uint32_t off = (uint32_t)(shiftStart[i] + 1440 - shiftStart[0]) % 1440;   // ca i bat dau sau ca 1 bao nhieu phut
        if (t > off) {
            uint32_t el = t - off;          // so phut da troi qua trong ca i
            if (el > dur[i]) el = dur[i];   // toi da bang thoi luong ca
            worked += el;
        }
    }
    return clampU32(targetDay * worked / total, 0, MAX_VALUE);
}

// Tinh lai muc tieu hien tai, neu doi gia tri thi danh dau ve lai man hinh
void SystemUI::recalcTargetNow() {
    uint32_t v = calcTargetNow();
    if (v != targetNow) {
        targetNow   = v;
        normalDirty = true;
    }
}

// Tra ve danh sach o LED (dong, cot) cua che do cai dat hien tai, n = so o.
// Thu tu trong danh sach = thu tu cac chu so trong buf[]
// Quy uoc dong: 0 = Dong 1 (tren cung), 1 = Dong 2 (giua), 2 = Dong 3 (duoi cung)
const SystemUI::Cell* SystemUI::cellsOf(uint8_t& n) const {
    // Cai dat thoi gian thuc (12 o tren 3 dong):
    //   Dong 1 (row 0): Gio.Phut (HH.MM)
    //   Dong 2 (row 1): Ngay.Thang (DD.MM)
    //   Dong 3 (row 2): Nam (20YY)
    static const Cell cTime[12]   = {
        {0,1},{0,2},{0,3},{0,4},  // Dong 1: Gio.Phut (HH.MM)
        {1,1},{1,2},{1,3},{1,4},  // Dong 2: Ngay.Thang (DD.MM)
        {2,1},{2,2},{2,3},{2,4}   // Dong 3: Nam (20YY)
    };
    static const Cell cRow1[5]    = { {0,0},{0,1},{0,2},{0,3},{0,4} }; // 5 so o Dong 1 (Muc tieu ngay)
    static const Cell cRow3[5]    = { {2,0},{2,1},{2,2},{2,3},{2,4} }; // 5 so o Dong 3 (So luong san pham hien tai)
    static const Cell cShift[8]   = {
        {0,1},{0,2},{0,3},{0,4},  // Dong 1: Gio bat dau ca (HH.MM)
        {1,1},{1,2},{1,3},{1,4}   // Dong 2: Gio ket thuc ca (HH.MM)
    };
    static const Cell cFactor[3]  = { {0,2},{0,3},{0,4} };             // 3 so o Dong 1 (He so cong khi cam bien dem)

    switch (mode) {
        case MODE_SET_TIME:   n = 12; return cTime;
        case MODE_SET_TARGET: n = 5;  return cRow1;     // Phim 2: sua Muc tieu ngay tren Dong 1
        case MODE_SET_COUNT:  n = 5;  return cRow3;     // Phim 3: sua So luong san pham hien tai tren Dong 3
        case MODE_SET_SHIFT1:
        case MODE_SET_SHIFT2:
        case MODE_SET_SHIFT3: n = 8;  return cShift;    // Phim 4,5,6: Dong 1 = Bat dau, Dong 2 = Ket thuc, Dong 3 = So ca
        case MODE_SET_FACTOR: n = 3;  return cFactor;   // Phim 7: sua He so cong tren Dong 1
        default:              n = 5;  return cRow1;
    }
}

// Tach so v thanh n chu so, luu vao b[0..n-1] (b[0] la chu so cao nhat)
void SystemUI::numToBuf(uint32_t v, uint8_t* b, uint8_t n) {
    for (int i = n - 1; i >= 0; i--) {
        b[i] = (uint8_t)(v % 10);
        v /= 10;
    }
}

// Ghep n chu so trong b[0..n-1] thanh 1 so
uint32_t SystemUI::bufToNum(const uint8_t* b, uint8_t n) {
    uint32_t v = 0;
    for (uint8_t i = 0; i < n; i++) v = v * 10 + b[i];
    return v;
}

uint32_t SystemUI::clampU32(uint32_t v, uint32_t lo, uint32_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// So ngay cua thang. Nam nhuan (nam chia het cho 4) thi thang 2 co 29 ngay
uint8_t SystemUI::daysInMonth(uint8_t month, uint8_t year) {
    static const uint8_t d[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && (year % 4) == 0) return 29;
    return d[(month - 1) % 12];
}

// Nap gia tri hien tai vao buf[] de nguoi dung sua tung chu so
void SystemUI::loadBuffer() {
    switch (mode) {
    case MODE_SET_TIME:
        // Dong 1 (buf[0..3]): Gio.Phut (HH.MM)
        numToBuf(clock.hour,  &buf[0], 2);
        numToBuf(clock.minute,&buf[2], 2);
        // Dong 2 (buf[4..7]): Ngay.Thang (DD.MM)
        numToBuf(clock.date,  &buf[4], 2);
        numToBuf(clock.month, &buf[6], 2);
        // Dong 3 (buf[8..11]): Nam (20YY)
        buf[8] = 2; buf[9] = 0;                 // 2 chu so dau cua nam: "20"
        numToBuf(clock.year,  &buf[10], 2);     // 2 chu so cuoi cua nam
        break;

    case MODE_SET_TARGET: numToBuf(targetDay, buf, 5); break;
    case MODE_SET_COUNT:  numToBuf(count,     buf, 5); break;

    case MODE_SET_SHIFT1:
    case MODE_SET_SHIFT2:
    case MODE_SET_SHIFT3: {
        uint8_t i = (uint8_t)(mode - MODE_SET_SHIFT1);      // ca 0, 1 hoac 2
        numToBuf(shiftStart[i] / 60, &buf[0], 2); numToBuf(shiftStart[i] % 60, &buf[2], 2);
        numToBuf(shiftEnd[i] / 60,   &buf[4], 2); numToBuf(shiftEnd[i] % 60,   &buf[6], 2);
        break;
    }
    case MODE_SET_FACTOR: numToBuf(factor, buf, 3); break;
    default: break;
    }
}

// Ghi gia tri nguoi dung vua nhap (trong buf[]) vao thong so that va luu xuong Flash + RAM DS1307
void SystemUI::applyBuffer() {
    switch (mode) {
    case MODE_SET_TIME: {
        RtcTime t;
        t.hour   = (uint8_t)clampU32(bufToNum(&buf[0], 2), 0, 23);          // Dong 1: Gio
        t.minute = (uint8_t)clampU32(bufToNum(&buf[2], 2), 0, 59);          // Dong 1: Phut
        t.date   = (uint8_t)clampU32(bufToNum(&buf[4], 2), 1, 31);          // Dong 2: Ngay
        t.month  = (uint8_t)clampU32(bufToNum(&buf[6], 2), 1, 12);          // Dong 2: Thang
        t.year   = (uint8_t)bufToNum(&buf[10], 2);                          // Dong 3: 2 so cuoi cua Nam (20YY)

        // Gioi han ngay theo dung thang (VD thang 2 chi co 28/29 ngay)
        t.date   = (uint8_t)clampU32(t.date, 1, daysInMonth(t.month, t.year));
        t.second = 0;       // luu gio thi giay dat ve 0

        if (rtc.writeTime(t)) {
            clock      = t;
            clockValid = true;
            // Nguoi dung tu chinh ngay thi KHONG tinh la sang ngay moi (khong reset san luong)
            dateKnown  = true;
            lastDate   = (uint32_t)t.year * 10000UL + t.month * 100UL + t.date;
        }
        break;
    }
    case MODE_SET_TARGET: targetDay = bufToNum(buf, 5); break;
    case MODE_SET_COUNT:  count     = bufToNum(buf, 5); break;
    case MODE_SET_SHIFT1:
    case MODE_SET_SHIFT2:
    case MODE_SET_SHIFT3: {
        uint8_t i = (uint8_t)(mode - MODE_SET_SHIFT1);
        shiftStart[i] = (uint16_t)(clampU32(bufToNum(&buf[0], 2), 0, 23) * 60 + clampU32(bufToNum(&buf[2], 2), 0, 59));
        shiftEnd[i]   = (uint16_t)(clampU32(bufToNum(&buf[4], 2), 0, 23) * 60 + clampU32(bufToNum(&buf[6], 2), 0, 59));
        break;
    }
    case MODE_SET_FACTOR:
        factor = (uint16_t)clampU32(bufToNum(buf, 3), 1, 999);      // he so toi thieu = 1
        break;
    default: break;
    }
    recalcTargetNow();      // thong so doi thi muc tieu hien tai co the doi
    saveStateNow(true);     // luu ngay thong so moi vao RAM DS1307 va Flash noi STM32
}

// Ve man hinh cai dat: hien cac chu so trong buf[] len dung o LED + con tro nhap nhay
void SystemUI::renderEdit() {
    uint8_t n;
    const Cell* c = cellsOf(n);

    disp.clear();
    for (uint8_t i = 0; i < n; i++) disp.setDigit(c[i].row, c[i].col, buf[i]);

    switch (mode) {
    case MODE_SET_TIME:
        disp.setDot(0, 2, true);  // dau cham giua Gio.Phut (Dong 1)
        disp.setDot(1, 2, true);  // dau cham giua Ngay.Thang (Dong 2)
        break;
    case MODE_SET_SHIFT1:
    case MODE_SET_SHIFT2:
    case MODE_SET_SHIFT3:
        disp.setDot(0, 2, true);  // dau cham Gio.Phut bat dau (Dong 1)
        disp.setDot(1, 2, true);  // dau cham Gio.Phut ket thuc (Dong 2)
        disp.setDigit(2, 4, (uint8_t)(mode - MODE_SET_SHIFT1 + 1));  // hien so ca (1, 2, 3) o Dong 3
        break;
    default: break;
    }

    disp.setCursor(c[cur].row, c[cur].col);     // con tro nhap nhay tai o dang sua
}
