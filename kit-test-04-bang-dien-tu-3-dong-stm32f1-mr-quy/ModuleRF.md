1. Chức năng chính
Nhận tín hiệu: Tiếp nhận dữ liệu điều khiển từ Remote RF.
Phân loại thao tác: Phân biệt rõ hai trạng thái Bấm nhả nhanh và Bấm giữ lì (>1 giây).
Chống nhiễu: Tích hợp thuật toán chống dội phím (anti-bouncing) và cơ chế tránh kẹt nút hiệu quả.
2.Cách sử dụng
Thêm thư viện: Sao chép hai tệp ModuleRF.h và ModuleRF.cpp vào thư mục Core/Src của dự án.
Khai báo: Thêm #include "ModuleRF.h" và khởi tạo đối tượng ModuleRF rf;.
Khởi tạo: Gọi hàm rf.init(); một lần trước khi vào vòng lặp chính.
Xử lý sự kiện: Trong vòng lặp while(1), gọi hàm rf.update(...) để bắt sự kiện. Hàm sẽ trả về thông tin phím được bấm và kiểu thao tác tương ứng.
