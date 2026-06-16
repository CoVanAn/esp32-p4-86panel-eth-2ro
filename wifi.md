# Kế Hoạch Lập Trình Lại Chức Năng Wi-Fi (Chi tiết Codebase)

Do phần cứng đã được hỗ trợ sẵn qua thư viện (hoặc core), chúng ta sẽ không bàn về lý thuyết phần cứng (ESP32-P4/C6) nữa. Dưới đây là kế hoạch **code chi tiết** để lập trình lại phần mạng trong hệ thống.

## 1. Tầng Backend: `ks_network_service.cpp`

### 1.1. Khởi tạo và Bắt Sự Kiện (Event Driven)
- **Kích hoạt lại Wi-Fi:** Mở lại `#include <WiFi.h>`.
- **Hàm `ks_network_manager_init()`:**
  - Giữ nguyên khởi tạo Ethernet (`ETH.begin(...)`).
  - Thêm khởi tạo Wi-Fi: `WiFi.mode(WIFI_STA);`
- **Hàm `NetworkEvent()`:** Bổ sung xử lý sự kiện Wi-Fi bên cạnh sự kiện ETH hiện tại:
  - `ARDUINO_EVENT_WIFI_STA_GOT_IP`: Đặt `current_state = NET_WIFI_CONNECTED`, cập nhật biến `current_ip`.
  - `ARDUINO_EVENT_WIFI_STA_DISCONNECTED`: Nếu mất Wi-Fi, kiểm tra xem ETH có đang cắm không để đổi IP về lại ETH, nếu không thì báo mất kết nối.

### 1.2. Thuật Toán Quét Mạng Bất Đồng Bộ (Async Scanning)
Để UI không bị "đóng băng" khi quét Wi-Fi (tốn vài giây):
- **Cờ trạng thái:** Bổ sung biến `static bool is_scanning_wifi = false;`.
- **Hàm `wifi_scanning_ssid()`:**
  - Chỉ gọi lệnh: `WiFi.scanNetworks(true);` (Tham số `true` báo cho thư viện quét ở luồng nền).
  - Đặt `is_scanning_wifi = true;` và hàm return ngay lập tức để không chặn UI.
- **Hàm `ks_network_manager_loop()` (Được gọi liên tục):**
  - Kiểm tra: `if (is_scanning_wifi && WiFi.scanComplete() >= 0)`
  - Nếu đúng (đã quét xong):
    - Đọc số lượng mạng: `network_count = WiFi.scanComplete();`
    - Dùng vòng lặp copy `WiFi.SSID(i)` và `WiFi.RSSI(i)` vào mảng toàn cục `networks[]`.
    - Xóa cache quét: `WiFi.scanDelete();`
    - Kích hoạt một cờ để báo cho LVGL (UI) cập nhật Dropdown danh sách mạng.
    - Đặt `is_scanning_wifi = false;`

### 1.3. Kết Nối Mạng Bất Đồng Bộ
- **Hàm `wifi_connect_async(const char* ssid, const char* pass)`:**
  - Thay vì dùng vòng lặp `while (WiFi.status() != WL_CONNECTED)`, chỉ cần gọi:
    `WiFi.disconnect();`
    `WiFi.begin(ssid, pass);`
  - Giao diện sẽ chỉ hiển thị "Đang kết nối...". Khi có sự kiện `ARDUINO_EVENT_WIFI_STA_GOT_IP`, UI sẽ tự động được làm mới thông qua hàm `sync_ui_state()`.

## 2. Tầng Giao Diện: UI (LVGL)

- **Màn hình Cài Đặt (Ví dụ: `ui_ScreenSettingsAdv` hoặc màn hình Network):**
  - Khi người dùng nhấn nút "Quét Mạng": Hiển thị một icon Loading (Spinner), gọi `wifi_scanning_ssid()`.
  - Cần có một Timer trong LVGL (`lv_timer_create`) hoặc check trong hàm loop của UI: Nếu thấy cờ dữ liệu đã quét xong từ tầng Backend, lập tức đọc mảng `networks[]`, tạo chuỗi định dạng bằng `\n` và đẩy vào widget Dropdown của LVGL. Sau đó ẩn icon Loading.
  - Khi người dùng chọn mạng, nhập Pass và nhấn "Kết nối": Gọi `wifi_connect_async()`, hiển thị thông báo "Đang xử lý kết nối...".

## 3. Tầng Web: `ks_web_html.h` & Routing

- Cập nhật trang quản trị Web LAN để hiển thị thêm trạng thái mạng Wi-Fi (Cường độ tín hiệu, SSID đang kết nối).
- Thêm một Form cấu hình Wi-Fi trên Web, gọi API (ví dụ `POST /api/wifi/connect`) chứa JSON `{"ssid": "...", "password": "..."}` để cấu hình thiết bị từ xa qua cổng mạng Ethernet, giúp dự phòng mạng.
