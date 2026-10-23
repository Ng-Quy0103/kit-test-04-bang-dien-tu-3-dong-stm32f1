Hướng dẫn tạo Project STM32F1 ban đầu
1. Công cụ cần thiết
STM32CubeMX: Dùng để cấu hình vi điều khiển, tạo khung project và sinh code khởi tạo ngoại vi ban đầu.
IAR Embedded Workbench for ARM (EWARM): Môi trường lập trình (IDE) dùng để viết code, biên dịch và nạp chương trình xuống vi điều khiển.
2. Tạo Project trên STM32CubeMX
Bước 1: Mở STM32CubeMX, chọn New Project và tìm kiếm vi điều khiển với mã STM32F103RET6.
Bước 2: Cấu hình các thông số hệ thống cơ bản:
Truy cập System Core $\rightarrow$ RCC $\rightarrow$ High Speed Clock (HSE): Chọn Crystal/Ceramic Resonator.
Truy cập System Core $\rightarrow$ SYS $\rightarrow$ Debug: Chọn Serial Wire (cho phép nạp/debug qua chuẩn SWD).
Bước 3: Chuyển sang tab Clock Configuration, nhập giá trị 72 vào ô HCLK (MHz) và nhấn Enter để tự động cấu hình bộ nhân tần số lên tốc độ tối đa.
Bước 4: Chuyển sang tab Project Manager:
Nhập tên dự án và đường dẫn lưu file.
Tại mục Toolchain / IDE, chọn EWARM.
Nhấn nút GENERATE CODE ở góc trên bên phải để khởi tạo thư mục dự án.
3. Đưa code C++ vào IAR
Bước 1: Mở thư mục dự án vừa khởi tạo và kích kép vào file .eww để mở dự án bằng IAR Embedded Workbench.
Bước 2: Chuyển đổi file khởi tạo sang C++:
Xóa file main.c mặc định khỏi cây thư mục quản lý của IAR.
Tạo hoặc đổi tên file thành main.cpp và thêm lại vào project.
Bước 3: Thêm các thư viện bên ngoài:
Sao chép toàn bộ các file thư viện mã nguồn (.h và .cpp) vào thư mục Core/Src trong dự án.
Trên giao diện IAR, nhấp chuột phải vào thư mục quản lý code, chọn Add Files và chọn toàn bộ các file vừa chép vào.
Bước 4: Biên dịch và nạp chương trình:
Nhấn phím F7 (hoặc chọn Make) để tiến hành Build toàn bộ dự án.
Nhấn tổ hợp phím Ctrl + D (Download and Debug) để nạp code xuống mạch và bắt đầu quá trình gỡ lỗi.

