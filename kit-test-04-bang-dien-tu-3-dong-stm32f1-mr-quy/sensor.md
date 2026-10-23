1. Chức năng chính
Bắt tín hiệu: Nhận biết chính xác khi có vật cản (sản phẩm) đi qua cảm biến.
Bộ lọc nhiễu 20ms: Tích hợp cơ chế khử nhiễu tín hiệu (debouncing) 20ms. Tín hiệu phải duy trì ổn định đủ 20ms mới ghi nhận là 1 lần kích hoạt.
Chống lặp dữ liệu: Đảm bảo mỗi sản phẩm đi qua chỉ được đếm duy nhất 1 lần, loại bỏ hoàn toàn hiện tượng sản phẩm đứng yên trước cảm biến gây tăng số đếm liên tục.
2.  Cách sử dụng
Bước 1: Sao chép 2 tệp tin sensor.h và sensor.cpp vào thư mục Core/Src của dự án.
Bước 2: Khai báo thư viện #include "sensor.h" và khởi tạo đối tượng sensor proxSensor;.
Bước 3: Gọi hàm proxSensor.init(); trong phần khởi tạo hệ thống (trước vòng lặp chính).
Bước 4: Trong vòng lặp while(1), gọi liên tục hàm proxSensor.update(HAL_GetTick()). Khi hàm trả về giá trị true, một sản phẩm hợp lệ vừa được nhận diện.
