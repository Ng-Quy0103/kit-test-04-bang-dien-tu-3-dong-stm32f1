# Thu vien Indicator (LED Main & Coi Buzzer)

## 1. Chuc nang cua thu vien
- Dieu khien **LED Main** tren mach nhay deu dan chu ky 100ms SANG / 100ms TAT de bao mach dang chay.
- Dieu khien **Coi chip (Buzzer)**:
  + Khi moi cap dien khoi dong: coi tu dong keu 2 lan (50ms ON, 100ms OFF).
  + Khi goi ham `triggerSingleBeep()`: coi keu 1 lan (50ms ON, 100ms OFF).
- Viet bang thanh ghi va dung `millis` (khong dung `delay`), khong lam dung chuong trinh.

## 2. Phan cung ket noi
- **LED Main**: nối chan **PB2** (Output Push-Pull).
- **Coi Buzzer**: noi chan **PD2** (Output Push-Pull, muc 1 = keu, muc 0 = tat).

## 3. Cac buoc su dung thu vien
- **Buoc 1**: Copy 2 file `Indicator.h` va `Indicator.cpp` vao thu muc `Core/Src` va Add vao project tren IAR.
- **Buoc 2**: Khai bao `#include "Indicator.h"` va tao doi tuong `Indicator systemIndicator;` o dau file `main.cpp`.
- **Buoc 3**: Goi `systemIndicator.init();` 1 lan truoc vong lap `while(1)` de cau hinh chan PB2 va PD2.
- **Buoc 4**: Trong vong lap `while(1)`, goi `systemIndicator.update(HAL_GetTick());` lien tuc de cap nhat trang thai nhay LED va coi.
- **Buoc 5**: Khi can coi keu 1 tieng (vi du khi cam bien dem duoc san pham), goi `systemIndicator.triggerSingleBeep(HAL_GetTick());`.

## 4. Code mau su dung trong `main.cpp`
```cpp
#include "main.h"
#include "Indicator.h"

Indicator systemIndicator;

int main(void) {
    HAL_Init();
    SystemClock_Config();

    // Buoc 3: Khoi tao chan PB2 (LED) va PD2 (Coi)
    systemIndicator.init();

    while (1) {
        uint32_t current_time = HAL_GetTick();

        // Buoc 4: Cap nhat trang thai LED va Coi
        systemIndicator.update(current_time);
    }
}
```
