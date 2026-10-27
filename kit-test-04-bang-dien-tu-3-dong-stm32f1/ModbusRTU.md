1. Chức năng chính

Nhận tín hiệu: Tiếp nhận và phản hồi dữ liệu với máy tính (PC/PLC) theo chuẩn giao tiếp công nghiệp RS485 Modbus RTU Slave (Mặc định ID = 5, Tốc độ 115200).

Phân loại thao tác: Tự động phân tách và xử lý chính xác các nhóm lệnh đọc/ghi khác nhau từ phần mềm Master (mã 0x03, 0x04, 0x06, 0x10).

Chống nhiễu & Chống treo: Sử dụng bộ điều khiển truy cập nhớ trực tiếp (DMA) kết hợp Ngắt nhàn rỗi (IDLE) chạy ngầm trên 2 cổng vật lý cùng lúc, giúp vi điều khiển nhận luồng dữ liệu liên tục mà không bị treo hay giật lag vòng lặp chính.

2. Cách sử dụng

Thêm thư viện: Sao chép hai tệp ModbusRTU.h và ModbusRTU.cpp vào thư mục mã nguồn của dự án (thường là src hoặc Core/Src).

Khai báo: Thêm #include "ModbusRTU.h" và khởi tạo đối tượng ModbusRTU modbus;.

Khởi tạo: Gọi hàm modbus.init(&ui, millis()); một lần trước khi vào vòng lặp chính để tự động cấu hình các chân giao tiếp RS485 và DMA.

Xử lý sự kiện: Trong vòng lặp while(1), gọi hàm modbus.process(millis());. Hàm này sẽ ngầm tự động bắt các gói tin đến, đồng bộ thanh ghi, trả lời máy tính và tự động tính toán tốc độ truyền tải (FPS) mà không cần can thiệp thêm.

3. Hướng dẫn sử dụng phần mềm PC

Để giao tiếp và kiểm tra tốc độ mạng thực tế (FPS > 100) của mạch, khuyến nghị sử dụng phần mềm kiểm thử tiêu chuẩn công nghiệp Modbus Poll.

Cài đặt phần mềm:

Truy cập trang chủ modbustools.com và tải phiên bản Modbus Poll mới nhất (chọn bản 32-bit hoặc 64-bit phù hợp với Windows đang dùng).

Cài đặt phần mềm (có thể sử dụng ngay phiên bản dùng thử miễn phí).

Cấu hình kết nối vật lý (Nhấn F3 - Connection Setup):

Connection: Chọn Serial Port (Chọn đúng cổng COM của cáp USB-RS485 CH340).

Thông số: Baud Rate: 115200 | Data bits: 8 | Parity: None | Stop Bit: 1.

Response Timeout: 200 [ms].

Delay Between Polls: 0 hoặc 10 [ms] (Ép độ trễ giữa 2 lần gửi lệnh về mức thấp nhất).

Nhấn OK để kết nối.

Cấu hình khung truyền dữ liệu (Nhấn F8 - Read/Write Definition):

Slave ID: 5 (Bắt buộc phải khớp với địa chỉ quy định trong code).

Function: Chọn 03 Read Holding Registers (4x).

Address: 0 | Quantity: 20 (Đọc toàn bộ 20 thanh ghi của hệ thống).

Scan Rate: Đặt là 10 [ms] (Yêu cầu PC gửi lệnh 100 lần/giây để test sức chịu tải tối đa).

Trong mục View, chọn số dòng (Rows) là 20 để hiển thị hết bảng.

Nhấn OK. Lúc này, thanh ghi số 0 (REG_FPS) sẽ liên tục nhảy số, báo hiệu số lượng gói tin xử lý thành công trong 1 giây thực tế.