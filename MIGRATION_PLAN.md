# Kế Hoạch Chuyển Đổi (Migration Plan): LUCKFOX KSmartZoneX -> ESP32-P4-86PANEL

Kế hoạch này vạch ra các bước chi tiết để clone giao diện và chức năng của phần mềm KSmartZoneX-Node-Device từ hệ điều hành Linux (board LUCKFOX) sang môi trường Arduino/FreeRTOS trên vi điều khiển **ESP32-P4-86PANEL-ETH-2RO**.

---

## 1. Phân Tích Sự Khác Biệt Nền Tảng (Platform Differences)

| Đặc Điểm | LUCKFOX KSmartZoneX (Linux) | ESP32-P4-86PANEL (Arduino / RTOS) | Hành Động Cần Thiết |
| :--- | :--- | :--- | :--- |
| **Hệ Điều Hành** | Linux (Ubuntu/Buildroot) | FreeRTOS (thông qua Arduino core) | Viết lại logic quản lý luồng (Threads -> FreeRTOS Tasks). |
| **Giao Diện (UI)** | LVGL (Linux Framebuffer/DRM) | LVGL (Arduino_GFX + MIPI DSI) | Tái sử dụng source code C của LVGL, thay đổi lớp Display & Touch input. |
| **Lưu Trữ (Storage)**| File System (ext4), đọc ghi JSON. | SPIFFS / LittleFS / NVS | Chuyển đổi mã đọc/ghi file `relay_map.json` sang LittleFS/SPIFFS. |
| **Mạng (Network)** | TCP/IP Socket chuẩn Linux, wpa_supplicant | WiFi.h, Ethernet.h (LwIP) | Viết lại module HTTP Client/Server và WebSocket cho ESP32. |
| **Phần Cứng** | Tương tác qua `/sys/class/gpio` hoặc thư viện ngoại vi. | RS485 (Hardware Serial), GPIO (digitalWrite) | Cấu hình lại các chân GPIO, UART cho RS485 và Relay. |

---

## 2. Các Giai Đoạn Triển Khai (Implementation Phases)

### Giai Đoạn 1: Chuẩn Bị & Khởi Tạo Nền Tảng (Đã hoàn thành một phần)
- **1.1. Cấu hình Display & Touch:** Đã tích hợp `Arduino_GFX` điều khiển panel qua MIPI DSI và GT911 (I2C) tại `controllrs485.ino`.
- **1.2. Cấu hình LVGL:** Đã thiết lập `lv_conf.h`, timer tick, và buffer màn hình.
- **1.3. Cấu trúc thư mục:** Source UI (SquareLine Studio export) đang đặt tại `src/ksmart/`.

### Giai Đoạn 2: Clone & Tích Hợp UI (Giao Diện)
- **2.1. Bê nguyên mã nguồn UI:** Cần copy đè các file từ thư mục `ui/` của LUCKFOX (bao gồm `ui.c`, `ui.h`, các thư mục `screens`, `images`, `fonts`) sang `src/ksmart/`.
- **2.2. Xử lý Font tiếng Việt:** Đảm bảo `lv_conf.h` bật các custom fonts hỗ trợ UTF-8 (để không bị lỗi hình chữ nhật). Add các file `.c` của font vào `src/fonts/`.
- **2.3. Khắc phục lỗi tương thích LVGL:** ESP32 sử dụng LVGL v8 hoặc v9. Kiểm tra phiên bản LVGL giữa 2 dự án. Nếu khác biệt, cần sửa lại các hàm API đã bị deprecate (ví dụ: `lv_disp_set_bg_color` -> `lv_obj_set_style_bg_color`).
- **2.4. Compile & Test UI:** Đảm bảo UI lên hình, cảm ứng mượt, chuyển trang (screens) thành công mà không bị crash do thiếu RAM.

### Giai Đoạn 3: Viết Lại Backend - Quản Lý Thiết Bị (Device Control)
Trong môi trường ESP32, các hành động trên UI sẽ kích hoạt callbacks, ta cần map chúng với các chức năng phần cứng:
- **3.1. RS485 Driver:**
  - Khởi tạo `HardwareSerial` cho RS485 (cần xác định TX, RX, và chân DE/RE control pin).
  - Viết module `RS485_Service`: Xử lý đóng gói/giải mã các frame lệnh Modbus/Custom Protocol để giao tiếp với các node con.
- **3.2. Relay Control:**
  - Khởi tạo GPIO điều khiển Relay (2 cổng RO).
  - Ánh xạ hành động nhấn nút trên UI sang việc gọi hàm `digitalWrite(RELAY_PIN, HIGH/LOW)`.
- **3.3. File System (Cài đặt CH):**
  - Khởi tạo `LittleFS` để lưu file cài đặt như `relay_map.json`.
  - Thay thế thư viện file I/O của Linux bằng thư viện `<ArduinoJson.h>` và `File` từ `<LittleFS.h>`.

### Giai Đoạn 4: Giao Tiếp Mạng & Đồng Bộ Dữ Liệu (Network & Sync)
- **4.1. Network Driver:**
  - Viết module `Network_Service`: Khởi tạo WiFi Station hoặc Ethernet LAN8720/RMII (tùy thiết kế mạch của ESP32-P4-ETH-2RO).
- **4.2. Giao tiếp API KSmartZoneX:**
  - Sử dụng thư viện `HTTPClient` để call API lấy lịch phát, lấy danh sách playlist, thông tin thiết bị.
  - Chuyển logic parse JSON từ LUCKFOX sang dùng `ArduinoJson`.
- **4.3. Đa luồng (FreeRTOS):**
  - Tách quá trình lấy data (Fetch API) ra một `Task` riêng trong FreeRTOS (`xTaskCreatePinnedToCore`) để không block UI tick của LVGL.
  - Sử dụng `Mutex` (Semaphore) bảo vệ dữ liệu khi ghi vào biến cấu trúc để tránh tranh chấp bộ nhớ giữa Task xử lý mạng và Task vẽ UI.

---

## 3. Quy Trình Làm Việc Đề Xuất (Next Steps)

1. **Copy UI:** Bạn hãy tiến hành copy (export) mã nguồn UI từ SquareLine / LUCKFOX và ghi đè vào thư mục `src/ksmart/` của dự án ESP32.
2. **Loại bỏ / Stub Code OS cũ:** Tôi sẽ giúp bạn comment/mock các đoạn code gọi đến hệ điều hành Linux (POSIX threads, files, sockets) để code có thể biên dịch thành công.
3. **Chạy thử UI trên ESP32:** Compile và nạp xuống ESP32-P4 để xem giao diện tĩnh.
4. **Viết class Wrapper cho ESP32:** Bắt đầu implement các driver thay thế cho WiFi, LittleFS, và Serial RS485.

Bạn muốn chúng ta bắt đầu bằng việc **đổ mã nguồn UI sang `src/ksmart`** hay tập trung vào **viết driver RS485 / Relay** trước?
