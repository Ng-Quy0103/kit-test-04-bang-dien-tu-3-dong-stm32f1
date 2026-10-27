## 1. MÔ TẢ
Bài tập nhằm mục đích kiểm tra khả năng nghiên cứu và tích hợp các module cơ bản thành 01 project lớn hơn.

## 2. PHẦN CỨNG YÊU CẦU

- 01 Mạch điều khiển + 01 mạch nạp
- 01 bảng điện tử 3 dòng, mỗi dòng 4 số led
- 01 module RFID
- 01 cảm biến đếm số lượng
- 01 nguồn 9 VDC
- 01 module rs485

## 3. YÊU CẦU CHUNG
- Tất cả code được viết trên IDE IAR (có thể sử dụng CubeMX để gen code ban đầu)
- Viết code kiểu truy cập thanh ghi (**hạn chế** sử dụng thư viện HAL)
- Không được sử dụng delay trong code
- Code bắt buộc phải viết thành kiểu **class** để có thể tái sử dụng sau này
- **Led main** trên mạch bắt buộc phải nháy với chu kỳ 100ms ON/ 100ms OFF trong bất kỳ trường hợp nào để biết được là mạch đang hoạt động.
- Khi hệ thống mới khởi động: còi chip cần kêu 2 lần (50ms ON, 100ms OFF) để báo hiệu là mạch bắt đầu hoạt động

## 4. TÍNH NĂNG CẦN CÓ
- Cảm biến: khi có tín hiệu từ cảm biến thì bộ đếm của mạch tăng 01 đơn vị và còi chip kêu 01 lần
- Module led 7 đoạn: 
    + Dòng 1: Mục tiêu cần thực hiện được trong ngày hiện tại
    + Dòng 2: Mục tiêu tại thời điểm hiện tại cần đạt được (cập nhật realtime)
    + Dòng 3: Số lượng sản phẩm đã sản xuất được
- Module RFID: sử dụng để cài đặt thông số
    - Cài đặt thời gian thực
    - Cài đặt "Mục tiêu ngày" cần đạt được
    - Chỉnh sửa thông số "Số lượng sản phẩm hiện tại"
    - Cài đặt thời gian Bắt đầu và Kết thúc của các ca làm việc (có tổng 3 ca làm việc)
    - Hệ số cộng khi cảm biến phát hiện sản phẩm (ví dụ cài đặt là 2 thì mỗi lần phát hiện sẽ tăng 2 sản phẩm)
    - Chế độ test led: led nháy tuần tự từ số 0->9
    - Bấm giữ 1 số để vào chế độ cài đặt. Trong chế độ cài đặt thì bấm giữ số 11 hoặc 12 để thoát chế độ cài đặt ra ngoài màn hình chính
    - Trong chế độ cài đặt: bấm 1->9 tương ứng với số 1->9. Bấm 10 tương ứng số 0. Số 11 để lùi con trỏ, số 12 để tiến con trỏ.
- Module RS485:
    - Chuẩn giao tiếp modbus RTU
    - Mạch là slave, id = 5
    - Cấu hình thông số giao tiếp với Master (tool test trên PC) là 115200-8N1
    - Master có thể đọc/ghi các thông số xuống mạch (trong project có thông số nào thì cần phải đọc/ghi được thông số đó qua modbus)
    - Tính toán tốc độ truyền thông giữa master và slave. Và cho phép master đọc thông số này (FPS - Frame per second). Mỗi lần truyền nhận thành công thì tính là 1 lần thành công
- Những thông số cài đặt, bộ đếm cần được lưu lại khi mất điện đột ngột
- Sang ngày mới thì "Sản lượng hiện tại - Dòng 3" cần được Reset về 0





