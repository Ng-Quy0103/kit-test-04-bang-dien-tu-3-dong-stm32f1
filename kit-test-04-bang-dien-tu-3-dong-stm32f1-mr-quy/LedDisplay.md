1. Chức năng chính
Xử lý việc hiển thị các con số lên đúng dòng, đúng vị trí.
Hỗ trợ hiển thị dấu chấm (ví dụ: ngày.tháng, giờ.phút).
Tự động tạo hiệu ứng nhấp nháy con trỏ khi cài đặt thông số.
Chế độ Test LED (Sáng đồng loạt một số).
2. Cách sử dụng
Bước 1: Đảm bảo project đã có led7seg và copy thêm LedDisplay.h, LedDisplay.cpp vào thư mục Core/Src.
Bước 2: Khai báo #include "LedDisplay.h" và tạo đối tượng LedDisplay disp;.
Bước 3: Khởi tạo disp.init();.
Bước 4: Sử dụng các hàm:
disp.setNumber(dong, gia_tri);: In số lên 1 dòng.
disp.setDot(dong, cot, true);: Bật dấu chấm.
disp.setCursor(dong, cot);: Làm nhấp nháy 1 vị trí.
Bước 5: Gọi disp.process(HAL_GetTick())trước while(1).
