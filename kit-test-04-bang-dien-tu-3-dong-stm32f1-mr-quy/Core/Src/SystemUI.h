#ifndef SYSTEMUI_H
#define SYSTEMUI_H

#include <stdint.h>
#include "ModuleRF.h"
#include "LedDisplay.h"
#include "Ds1307.h"
#include "Indicator.h"

/*
 * SystemUI - Lop dieu phoi man hinh chinh va cai dat bang remote RF.
 *
 * Man hinh chinh (3 dong x 5 so):
 *      Dong 1: Muc tieu can thuc hien duoc trong ngay hien tai (mac dinh 1000)
 *      Dong 2: Muc tieu tai thoi diem hien tai can dat duoc (cap nhat realtime)
 *      Dong 3: So luong san pham da san xuat duoc
 *
 * Cai dat bang remote RF: GIU 1 phim >= 1 giay o man hinh chinh de vao che do cai dat
 *      Phim 1 = Cai dat thoi gian thuc (nam, ngay.thang, gio.phut)
 *      Phim 2 = Cai dat "Muc tieu ngay" can dat duoc
 *      (Phim 3..8 = cac che do mo rong: san luong, 3 ca lam viec, he so cong, test LED)
 *      Trong che do cai dat:
 *          + Bam 1..9 tuong ung so 1..9; bam 10 tuong ung so 0
 *          + Bam 11 de lui con tro; bam 12 de tien con tro
 *          + Bam giu 11 hoac 12 (>= 1 giay) de luu va thoat ra man hinh chinh
 */
class SystemUI {
public:
    static const uint32_t MAX_VALUE = 99999;    // Gia tri toi da hien thi tren 5 so LED

    SystemUI();
    void init(Indicator* ind, uint32_t now);    // Khoi tao LED, RF, RTC (goi SAU SystemClock_Config)
    void process(uint32_t now);                 // Goi lien tuc trong while(1): RTC, phim, man hinh
    void onProduct();                           // Cam bien phat hien san pham: san luong += he so

    // Cac ham lay gia tri hien tai
    uint32_t getCount() const { return count; }
    uint32_t getTargetDay() const { return targetDay; }
    uint32_t getTargetNow() const { return targetNow; }
    uint16_t getFactor()    const { return factor; }
    uint16_t getShiftStart(uint8_t shift) const { return shiftStart[shift % 3]; }   // gio bat dau ca (tinh bang phut)
    uint16_t getShiftEnd(uint8_t shift)   const { return shiftEnd[shift % 3]; }     // gio ket thuc ca (tinh bang phut)
    bool     getTime(RtcTime& t) const { t = clock; return clockValid; }
    bool     isNormalMode() const { return mode == MODE_NORMAL; }

private:
    // Cac che do hoat dong cua giao dien
    enum Mode {
        MODE_NORMAL,        // man hinh chinh
        MODE_SET_TIME,      // cai dat thoi gian thuc
        MODE_SET_TARGET,    // cai dat muc tieu ngay
        MODE_SET_COUNT,     // chinh san luong hien tai
        MODE_SET_SHIFT1,    // cai dat ca 1
        MODE_SET_SHIFT2,    // cai dat ca 2
        MODE_SET_SHIFT3,    // cai dat ca 3
        MODE_SET_FACTOR,    // cai dat he so cong
        MODE_TEST_LED       // test LED: nhay tuan tu 0 -> 9
    };

    // Vi tri 1 o LED (dong, cot) dung cho con tro nhap nhay khi cai dat
    struct Cell { uint8_t row; uint8_t col; };

    static const uint32_t RTC_PERIOD_MS = 1000;     // cu 1 giay doc DS1307 1 lan
    static const uint32_t TEST_STEP_MS  = 500;      // moi so trong che do test hien 0.5 giay
    static const uint8_t  MAX_EDIT_CELLS = 12;      // so o nhap toi da (che do cai dat thoi gian)

    ModuleRF   rf;          // remote RF
    LedDisplay disp;        // bang LED 3 dong
    Ds1307     rtc;         // IC thoi gian thuc
    Indicator* indicator;   // coi chip (con tro toi doi tuong dung chung o main)

    uint32_t targetDay;             // muc tieu ngay (dong 1)
    uint32_t count;                 // san luong hien tai (dong 3)
    uint16_t factor;                // he so cong moi lan cam bien phat hien san pham
    uint16_t shiftStart[3];         // gio bat dau 3 ca (phut tu 0h00)
    uint16_t shiftEnd[3];           // gio ket thuc 3 ca (phut tu 0h00)

    Mode     mode;                  // che do hien tai
    RtcTime  clock;                 // thoi gian doc tu DS1307
    bool     clockValid;            // true khi da doc duoc gio hop le
    uint32_t lastRtcMs;             // moc millis lan doc RTC truoc
    uint32_t nowMs;                 // millis hien tai (luu lai de dung trong cac ham con)
    uint32_t targetNow;             // muc tieu tai thoi diem hien tai (dong 2)
    bool     normalDirty;           // true = can ve lai man hinh chinh
    bool     dateKnown;             // da biet ngay hien tai chua
    uint32_t lastDate;              // ngay lan truoc (dang YYMMDD) de phat hien sang ngay moi

    uint8_t  buf[MAX_EDIT_CELLS];   // bo dem cac chu so dang nhap khi cai dat
    uint8_t  cur;                   // vi tri con tro trong buf
    uint8_t  testDigit;             // so dang hien trong che do test LED
    uint32_t testMs;                // moc thoi gian doi so trong che do test LED

    void handleKey(const RfEvent& ev);  // xu ly 1 su kien phim
    void beep();                        // keu coi 1 lan

    void renderNormal();                // ve man hinh chinh
    void readClock();                   // doc DS1307 va cap nhat muc tieu hien tai
    void checkNewDay();                 // sang ngay moi thi reset san luong ve 0
    uint32_t calcTargetNow() const;     // tinh muc tieu tai thoi diem hien tai
    void recalcTargetNow();             // tinh lai va danh dau ve lai neu thay doi

    void enterMode(Mode m);             // vao che do cai dat
    void leaveMode(bool save);          // thoat che do cai dat (save = true: luu gia tri vua nhap)
    void loadBuffer();                  // nap gia tri hien tai vao buf de sua
    void applyBuffer();                 // ghi gia tri trong buf vao thong so that
    void renderEdit();                  // ve man hinh cai dat
    const Cell* cellsOf(uint8_t& n) const;  // danh sach o LED cua che do hien tai (n = so o)

    static void     numToBuf(uint32_t v, uint8_t* b, uint8_t n);        // so -> n chu so
    static uint32_t bufToNum(const uint8_t* b, uint8_t n);              // n chu so -> so
    static uint8_t  daysInMonth(uint8_t month, uint8_t year);           // so ngay cua thang (co nam nhuan)
    static uint32_t clampU32(uint32_t v, uint32_t lo, uint32_t hi);     // gioi han v trong [lo, hi]
};

#endif
