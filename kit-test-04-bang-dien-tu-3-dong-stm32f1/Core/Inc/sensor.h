#ifndef SENSOR_H
#define SENSOR_H

#include <stm32f1xx.h>
#include <stdint.h>

class sensor {
private:
    uint32_t productCount;     // Bien luu so luong san pham
    uint32_t debounceMillis;   // Moc thoi gian chong nhieu
    
    // May trang thai 3 buoc bat suon xung cam bien
    enum State {
        WAIT_OBJECT,           // Dang cho san pham di toi
        DEBOUNCE,              // Dang kiem tra chong nhieu (20ms)
        WAIT_LEAVE             // Da dem xong, cho san pham di qua han
    } sensorState;

public:
    sensor();                                  // Ham tao (Constructor)
    void init();                               // Cau hinh thanh ghi chan PC3
    bool update(uint32_t currentMillis);       // Quet cam bien (Tra ve true khi tang +1)
    uint32_t getCount();                       // Ham lay gia tri bo dem ra ngoai
};

#endif