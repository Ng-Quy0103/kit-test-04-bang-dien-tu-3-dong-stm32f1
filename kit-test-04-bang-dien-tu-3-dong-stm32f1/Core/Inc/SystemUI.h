#ifndef SYSTEMUI_H
#define SYSTEMUI_H

#include <stdint.h>
#include "ModuleRF.h"
#include "LedDisplay.h"
#include "Ds1307.h"
#include "Indicator.h"
#include "FlashStorage.h"

/*
 * SystemUI - Lop dieu phoi man hinh chinh, cai dat bang remote RF va luu tru khi mat dien.
 *
 * Man hinh chinh (3 dong x 5 so):
 *      Dong 1: Muc tieu can thuc hien duoc trong ngay hien tai (mac dinh 1000)
 *      Dong 2: Muc tieu tai thoi diem hien tai can dat duoc (cap nhat realtime)
 *      Dong 3: So luong san pham da san xuat duoc (reset ve 0 khi sang ngay moi)
 *
 * Cai dat bang remote RF: GIU 1 phim >= 1 giay o man hinh chinh de vao che do cai dat
 *      Phim 1 = Cai dat thoi gian thuc (gio.phut, ngay.thang, nam)
 *      Phim 2 = Cai dat "Muc tieu ngay" can dat duoc
 *      Phim 3 = Chinh sua "So luong san pham hien tai"
 *      Phim 4, 5, 6 = Cai dat gio Bat dau / Ket thuc cua Ca 1, Ca 2, Ca 3
 *      Phim 7 = Cai dat He so cong khi cam bien phat hien san pham
 *      Phim 8 = Che do Test LED (nhay 0 -> 9)
 */
class SystemUI {
public:
    static const uint32_t MAX_VALUE = 99999;    // Gia tri toi da hien thi tren 5 so LED

    SystemUI();
    void init(Indicator* ind, uint32_t now);    // Khoi tao LED, RF, RTC, khoi phuc du lieu mat dien
    void process(uint32_t now);                 // Goi lien tuc trong while(1): RTC, phim, man hinh, luu Flash
    void onProduct();                           // Cam bien phat hien san pham: san luong += he so

    // Cac ham lay gia tri hien tai (dung cho Modbus RTU doc thong so)
    uint32_t getCount() const { return count; }
    uint32_t getTargetDay() const { return targetDay; }
    uint32_t getTargetNow() const { return targetNow; }
    uint16_t getFactor()    const { return factor; }
    uint16_t getShiftStart(uint8_t shift) const { return shiftStart[shift % 3]; }   // phut tu 0h00 (0..1439)
    uint16_t getShiftEnd(uint8_t shift)   const { return shiftEnd[shift % 3]; }     // phut tu 0h00 (0..1439)
    const RtcTime& getClock() const { return clock; }
    const RtcTime& getTime()  const { return clock; }
    bool     isClockValid() const { return clockValid; }
    bool     isNormalMode() const { return mode == MODE_NORMAL; }

    // Cac ham ghi gia tri tu ben ngoai (dung cho Modbus RTU ghi thong so xuong mach)
    void setTargetDay(uint32_t v);
    void setCount(uint32_t v);
    void setFactor(uint16_t v);
    void setShiftStart(uint8_t shift, uint16_t minutesFromMidnight);
    void setShiftEnd(uint8_t shift, uint16_t minutesFromMidnight);
    bool setTime(const RtcTime& t);
    void beep();                        // keu coi 1 lan khi nhan lenh ghi tu Modbus hoac RF

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

    static const uint32_t RTC_PERIOD_MS       = 1000;   // cu 1 giay doc DS1307 1 lan
    static const uint32_t TEST_STEP_MS        = 500;    // moi so trong che do test hien 0.5 giay
    static const uint32_t FLASH_SAVE_DELAY_MS = 2000;   // cho 2 giay sau khi dem xong moi ghi Flash (bao ve Flash)
    static const uint8_t  MAX_EDIT_CELLS      = 12;     // so o nhap toi da (che do cai dat thoi gian)

    ModuleRF     rf;            // remote RF
    LedDisplay   disp;          // bang LED 3 dong
    Ds1307       rtc;           // IC thoi gian thuc (kem 56 byte RAM co pin nuoi)
    FlashStorage flash;         // luu tru Flash noi STM32 khi mat dien
    Indicator*   indicator;     // coi chip (con tro toi doi tuong dung chung o main)

    uint32_t targetDay;             // muc tieu ngay (dong 1)
    uint32_t count;                 // san luong hien tai (dong 3)
    uint16_t factor;                // he so cong moi lan cam bien phat hien san pham
    uint16_t shiftStart[3];         // gio bat dau 3 ca (phut tu 0h00)
    uint16_t shiftEnd[3];           // gio ket thuc 3 ca (phut tu 0h00)

    Mode     mode;                  // che do hien tai
    RtcTime  clock;                 // thoi gian doc tu DS1307
    bool     clockValid;            // true khi da doc duoc gio hop le
    uint32_t lastRtcMs;             // moc millis lan doc RTC truoc
    uint32_t nowMs;                 // millis hien tai
    uint32_t targetNow;             // muc tieu tai thoi diem hien tai (dong 2)
    bool     normalDirty;           // true = can ve lai man hinh chinh
    bool     dateKnown;             // da biet ngay hien tai chua
    uint32_t lastDate;              // ngay lan truoc (dang YYMMDD) de phat hien sang ngay moi

    bool     flashPending;          // co danh dau can luu xuong Flash
    uint32_t lastChangeMs;          // moc thoi gian thay doi bo dem gan nhat

    uint8_t  buf[MAX_EDIT_CELLS];   // bo dem cac chu so dang nhap khi cai dat
    uint8_t  cur;                   // vi tri con tro trong buf
    uint8_t  testDigit;             // so dang hien trong che do test LED
    uint32_t testMs;                // moc thoi gian doi so trong che do test LED

    void handleKey(const RfEvent& ev);  // xu ly 1 su kien phim

    void renderNormal();                // ve man hinh chinh
    void readClock();                   // doc DS1307 va cap nhat muc tieu hien tai
    void checkNewDay();                 // sang ngay moi thi reset san luong ve 0
    uint32_t calcTargetNow() const;     // tinh muc tieu tai thoi diem hien tai
    void recalcTargetNow();             // tinh lai va danh dau ve lai neu thay doi

    void loadSavedState();              // khoi phuc thong so va bo dem khi bat dien
    void saveStateNow(bool writeFlash); // luu thong so vao RAM DS1307 (va Flash neu writeFlash = true)
    void packState(SavedParams& p) const;
    void unpackState(const SavedParams& p);

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
