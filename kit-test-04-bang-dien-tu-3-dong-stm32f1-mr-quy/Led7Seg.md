1. Chức năng chính
Gửi dữ liệu ra 15 bóng LED 7 đoạn thông qua IC ghi dịch 74HC595.
Chuyển đổi số (0-9) sang mã hiển thị tương ứng một cách tự động.
Tối ưu hóa hiệu năng: Hoạt động mượt mà không dùng hàm delay, không gây nghẽn hoặc treo hệ thống.
2. Cách sử dụng
Bước 1: Copy file led7seg.h và led7seg.cpp vào thư mục Core/Src của project trong IAR Workbench.
Bước 2: Khai báo thư viện #include "led7seg.h" và khởi tạo đối tượng Led7Seg myLed;.
Bước 3: Gọi hàm myLed.init(); SAU khi hệ thống hoàn tất cấu hình Clock.
Bước 4: Để hiển thị số, sử dụng hàm myLed.setNumber(dong, gia_tri); (Trong đó: Dòng 0 là trên cùng, Dòng 2 là dưới cùng).
Bước 5: Bắt buộc gọi hàm myLed.process() trước vòng lặp vô hạn while(1).
