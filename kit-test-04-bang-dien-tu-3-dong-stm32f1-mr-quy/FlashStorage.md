Hướng Dẫn Cơ Bản Thư Viện FlashStorage

1. Chức năng chính

Lưu trữ an toàn: Ghi nhớ vĩnh viễn các thông số quan trọng (sản lượng, mục tiêu, hệ số, giờ ca làm) vào bộ nhớ Flash của chip để không bị reset khi mất điện đột ngột.

Bảo vệ dữ liệu: Sử dụng mã nhận diện riêng (Magic code) và cơ chế tính toán tổng (Checksum) để đảm bảo dữ liệu đọc lên luôn toàn vẹn, không bị lỗi hỏng.

Tối ưu tuổi thọ: Tích hợp thuật toán thông minh tự động so sánh dữ liệu; chỉ thực hiện lệnh xóa/ghi khi thực sự có sự thay đổi, giúp bảo vệ tuổi thọ của bộ nhớ Flash.

2. Cách sử dụng

Thêm thư viện: Sao chép hai tệp FlashStorage.h và FlashStorage.cpp vào thư mục mã nguồn của dự án (thường là src hoặc Core/Src).

Khai báo: Thêm #include "FlashStorage.h" ở đầu file. Khai báo biến lưu trữ SavedParams params; và khởi tạo đối tượng FlashStorage flash;.

Khởi tạo (Đọc lên): Gọi hàm flash.load(params); một lần duy nhất ở hàm main() (trước khi vào vòng lặp) để khôi phục lại dữ liệu từ lần tắt máy trước.

Xử lý sự kiện (Lưu xuống): Bất cứ khi nào hệ thống có sự thay đổi thông số (ví dụ: bấm remote cài đặt lại), cập nhật các giá trị mới vào biến params, sau đó gọi hàm flash.save(params); để lưu vĩnh viễn.