1. Chức năng chính
   LED Main: Tự động nhấp nháy đều đặn báo hiệu mạch đang hoạt động bình thường.   
   Còi Buzzer: Kêu 2 tiếng khi khởi động mạch, kêu 1 tiếng khi có sự kiện       (ví dụ: đếm được 1 sản phẩm).   
   Chạy ngầm tự động hoàn toàn không dùng delay.  
3. Cách sử dụng
   Bước 1: Copy Indicator.h và Indicator.cpp vào thư mục Core/Src.   
   Bước 2: Khai báo #include "Indicator.h" và tạo đối tượng Indicator 	  	   sysInd;.   
   Bước 3: Gọi sysInd.init(); 1 lần trước vòng lặp chính.   
   Bước 4: Trong while(1), gọi liên tục sysInd.update(HAL_GetTick()); để duy     	   trì nhịp nháy LED.   
   Bước 5: Gọi sysInd.triggerSingleBeep(HAL_GetTick()); bất cứ khi nào muốn  	   còi kêu 1 tiếng.   