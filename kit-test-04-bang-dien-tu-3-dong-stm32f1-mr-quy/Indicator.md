# Thu vien Indicator (LED & Buzzer)

## 1. Gioi thieu
Thu vien quan ly chuc nang nhay LED va keu Coi (Buzzer) su dung nguyen ly non-blocking (khong dung delay, dung millis).

## 2. Phan cung yeu cau
- MCU: STM32F103C8T6
- LED Main: Chan PB2 
- Buzzer: Chan PD2 

## 3. Cach tich hop
1. Add `Indicator.h` va `Indicator.cpp` vao project.
2. Goi `systemIndicator.init()` truoc vong lap while.
3. Goi `systemIndicator.update(HAL_GetTick())` trong vong lap while(1).