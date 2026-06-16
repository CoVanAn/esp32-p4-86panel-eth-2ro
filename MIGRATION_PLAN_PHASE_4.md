# Kế Hoạch Chi Tiết Triển Khai Giai Đoạn 4 (Phase 4): Network & Sync trên ESP32-P4

Sau khi hoàn thành Giai đoạn 3 (Quản lý thiết bị & Local Relay), Giai đoạn 4 sẽ tập trung vào việc kết nối thiết bị với hệ thống mạng và giao tiếp thời gian thực với server (KSmartZoneX) thông qua TCP Socket. 

Dựa trên thực tế hoạt động của firmware KSmartZoneX trên LUCKFOX (Linux), quá trình chuyển đổi sang FreeRTOS (ESP32) đòi hỏi kiến trúc phải chặt chẽ hơn rất nhiều để tránh rủi ro về bộ nhớ và xung đột đa luồng. Do đó, Phase 4 được chia thành **6 module cốt lõi** và **1 bước tích hợp UI**.

---

## 4.1. Network Manager (Quản lý Mạng)

Không chỉ đơn thuần là gọi hàm kết nối WiFi hay Ethernet, module này đóng vai trò là một **State Machine (Máy trạng thái)** cung cấp thông tin xuyên suốt cho toàn hệ thống.

*   **Quản lý Trạng thái Mạng:**
    *   `DISCONNECTED` $\rightarrow$ `ETH_CONNECTED` $\rightarrow$ `WIFI_CONNECTED` $\rightarrow$ `INTERNET_OK`
*   **Hỗ trợ Cả 2 Giao Thức (Lưu ý Phần cứng ESP32-P4):** 
    *   **Ethernet (Ưu tiên số 1):** Cấu hình `ETH.begin()` cho LAN8720/RMII. Cổng LAN được kết nối trực tiếp vào ESP32-P4.
    *   **Wi-Fi (Đã hoãn):** Chip ESP32-P4 không có tích hợp MAC/Radio Wi-Fi. Mạch sử dụng chip `ESP32-C6-MINI-1U` làm Co-processor. Do đó, việc gọi `#include <WiFi.h>` sẽ gây xung đột GPIO với mạch RS485 và Relay. Tạm thời vô hiệu hóa thư viện `<WiFi.h>` và tập trung chạy Socket trên **Ethernet** trước.
*   **APIs Cần Thiết:**
    *   `bool ks_network_is_connected();`
    *   `bool ks_network_has_internet();` (Có thể ping 8.8.8.8 hoặc rely vào NTP/Socket)
    *   `String ks_network_get_ip();`

## 4.2. Device Identity Service (Định Danh Thiết Bị)

Trên Linux (LUCKFOX), KSmartZoneX đọc các thông số định danh từ hệ điều hành. Trên ESP32, chúng ta cần một service riêng biệt để cung cấp thông tin này cho Server khi xác thực Socket.

*   **Nhiệm vụ:** Quản lý và cung cấp cấu trúc thông tin thiết bị:
    ```json
    {
      "device_id": "ESP32-P4-...",
      "device_name": "KSmart Node",
      "mac": "AA:BB:CC:DD:EE:FF",
      "firmware": "v1.0.0-esp32",
      "ip": "192.168.1.x"
    }
    ```
*   **APIs Cần Thiết:**
    *   Trích xuất MAC Address thực tế của ESP32 qua `esp_read_mac()`.
    *   Tạo Device UUID độc nhất dựa trên chip ID.

## 4.3. Socket Service (Giao Tiếp TCP Thời Gian Thực)

Tái sử dụng tối đa logic `LwIP BSD Sockets` của C cũ (chạy trên FreeRTOS Task), nhưng phải nâng cấp độ tin cậy.

*   **FreeRTOS Task:** Chạy `xTaskCreatePinnedToCore` để tách riêng luồng mạng (Core 1) khỏi luồng UI LVGL (Core 0).
*   **Connection State Machine:** Thay vì vòng lặp `while(true) { connect(); }` thủ công, cần máy trạng thái rõ ràng:
    `IDLE` $\rightarrow$ `CONNECTING` $\rightarrow$ `AUTH` $\rightarrow$ `CONNECTED` $\rightarrow$ `RECONNECT`
*   **Cơ Chế Heartbeat (Ping/Pong):**
    *   Gửi gói tin `{"cmd":"heartbeat"}` mỗi 30 giây để báo cho Server biết Node vẫn còn sống.
    *   Nếu Socket `recv()` bị timeout quá 3 lần mất Heartbeat, tự động chuyển về trạng thái `RECONNECT`.

## 4.4. Message Queue (Hàng Đợi Thông Điệp FreeRTOS)

Để thay thế cho cơ chế `dirty flag` thô sơ và không an toàn (có thể gây Guru Meditation Error do xung đột), hệ thống bắt buộc sử dụng hàng đợi chuẩn của FreeRTOS.

*   **Cơ chế:**
    *   Khởi tạo: `QueueHandle_t g_ui_queue = xQueueCreate(10, sizeof(UIMessage));`
    *   **Từ Socket Task:** Khi nhận lệnh thay đổi từ Server, đóng gói thành cấu trúc `UIMessage` và gọi `xQueueSend()`.
    *   **Từ UI Task (Main Loop):** Kiểm tra `xQueueReceive()`. Nếu có thông điệp mới, gọi hàm API của LVGL tương ứng để thay đổi giao diện một cách an toàn nhất.

## 4.5. Time Service & RTC Cache (Đồng Bộ Thời Gian)

Hệ thống có các chức năng cực kỳ quan trọng là Lịch Phát, Timer Relay và Scheduler. Nếu mất mạng, các tính năng này vẫn phải hoạt động.

*   **NTP Sync:** Sử dụng `configTzTime` để đồng bộ SNTP qua mạng.
*   **RTC Cache qua NVS:**
    *   Định kỳ lưu `last_ntp_epoch` vào bộ nhớ Non-Volatile Storage (NVS) hoặc LittleFS.
    *   Khi khởi động lại mà không có Internet (mất mạng), ESP32 vẫn có thể lấy lại giờ cuối cùng đã lưu và tiếp tục đếm giờ qua RTC nội bộ để chạy Lịch Phát (Scheduler).

## 4.6. Sync Engine (Trái Tim Của Hệ Thống)

Đây là module liên kết tất cả các thành phần lại với nhau. Socket chỉ có nhiệm vụ Gửi/Nhận chuỗi JSON. `ks_sync_engine` sẽ đọc JSON và ra quyết định.

*   **Đồng bộ Relay:**
    *   Server gửi lệnh: `{"cmd": "relay", "port": 5, "state": "ON"}` $\rightarrow$ Engine gọi `ks_relay_service` để gửi lệnh Modbus qua RS485 $\rightarrow$ Engine đẩy vào `Message Queue` báo UI cập nhật nút bấm số 5 thành màu xanh.
*   **Đồng bộ Cài Đặt (Settings):**
    *   Server đổi tên: `{"cmd": "rename", "port": 1, "name": "Đèn Sân"}` $\rightarrow$ Engine lưu vào `relay_map.json` (Phase 3) $\rightarrow$ Engine đẩy Queue báo UI cập nhật Text Label.
*   **Đồng bộ Thiết bị (Discovery):**
    *   Server yêu cầu: `scan_rs485` $\rightarrow$ Engine gọi `ks_relay_discovery_service_scan_sync()` $\rightarrow$ Trả kết quả JSON ngược lại qua Socket.

---

## 4.7. Thứ Tự Triển Khai Thực Tế

Khuyến nghị triển khai lần lượt từng module và test độc lập trước khi ráp lại:

1.  **Network Manager**: [HOÀN THÀNH] Đã cấu hình chạy ổn định Ethernet IP101 cho board Waveshare P4, hiển thị IP tức thời lên trang Cài đặt, tự động chuyển màu icon.
2.  **Device Identity Service**: [HOÀN THÀNH] Đọc địa chỉ MAC (ưu tiên Ethernet) làm UUID định danh cho thiết bị qua `esp_read_mac`.
3.  **Time Service**: [HOÀN THÀNH] Đồng bộ NTP thành công, hiển thị giờ (HH:MM) và ngày tháng tiếng Việt (Thứ..., DD/MM/YYYY) trực tiếp lên trang chính. Đã cấu hình NVS cache lưu mốc thời gian để khôi phục khi mất điện không có mạng.
4.  **Socket Service**: [HOÀN THÀNH] Kết nối TCP Client với Server sử dụng LwIP BSD Sockets, gửi chuỗi định danh Identity JSON và duy trì kết nối ở Core 1.
5.  **Message Queue**: [HOÀN THÀNH] Tạo Queue FreeRTOS để gửi tin nhắn điều khiển từ Socket Task sang UI Task (Core 0) an toàn, tránh Guru Meditation crash.
6.  **Sync Engine**: [HOÀN THÀNH] Xử lý lệnh JSON nhận được từ Server để đóng/mở Relay local hoặc Relay RS485.
7.  **UI Integration**: [HOÀN THÀNH] Đồng bộ chấm tròn xanh/đỏ góc trên trang chính tương ứng với trạng thái kết nối mạng thực tế (Connected/Disconnected).

---

## 4.8. Nhật Ký Tiến Độ Thực Tế (Cập Nhật 12/06/2026)

### Những hạng mục đã hoàn thành:
- [x] **Ethernet IP101 Driver:** Khắc phục lỗi cấu hình PHY Address, MDC/MDIO và cấp xung Clock giúp board Waveshare nhận IP trực tiếp từ cổng LAN thành công.
- [x] **Đồng bộ Giao diện Cài Đặt:** Loại bỏ hoàn toàn độ trễ 2 giây khi chuyển đổi trang Cài đặt, hiển thị IP ngay lập tức khi mở trang.
- [x] **Time Service & NVS Cache:** Hoàn thành đồng bộ SNTP tự động. Triển khai định dạng thời gian Việt hóa và đẩy trực tiếp lên màn hình chính. Hoàn thiện lưu giờ vào NVS NVM để backup.
- [x] **Device Identity Service:** Trích xuất MAC Address thành công làm định danh UUID.
- [x] **Module 4.3: Socket Service:** Porting và tối ưu hóa socket từ Linux sang ESP32/FreeRTOS chạy trên Core 1.
- [x] **Module 4.4 & 4.6:** Triển khai `xQueue` FreeRTOS để giao tiếp an toàn đa nhân và viết Sync Engine phân tích lệnh điều khiển.
- [x] **Module 4.7: UI Integration:** Hoàn thành đồng bộ trạng thái mạng lên góc main UI (chấm tròn xanh/đỏ và text trạng thái).
### Những hạng mục tiếp theo cần xử lý:
- [ ] Không còn hạng mục nào trong Phase 4. Sẵn sàng chuyển sang Phase 5 (OTA, Cấu hình nâng cao) sau khi test phần cứng thành công.

---

## 4.9. Các Hạng Mục Khó & Xem Xét Sau (Deferred & Advanced Tasks)

Các hạng mục dưới đây có độ phức tạp cao về mặt phần cứng và tích hợp hệ thống. Để tránh làm tắc nghẽn quá trình di cư các tính năng cốt lõi (Socket, Relay, Sync), chúng ta thống nhất **hoãn lại và sẽ xử lý sau khi toàn bộ hệ thống đã chạy ổn định trên Ethernet**:

### 1. Wi-Fi Co-Processor Integration (Tích hợp Wi-Fi qua ESP32-C6)
*   **Thách thức:** ESP32-P4 không có bộ thu phát sóng Wi-Fi tích hợp. Board Waveshare sử dụng chip phụ `ESP32-C6-MINI-1U` kết nối qua giao tiếp SDIO/SPI. Để chạy được Wi-Fi trong môi trường Arduino, cần nạp firmware phụ (ESP-Hosted hoặc AT firmware) cho ESP32-C6 và tích hợp driver tương ứng trên P4. Điều này cực kỳ dễ gây xung đột GPIO với các chip RS485 và Relay nếu cấu hình sai.
*   **Giải pháp tạm thời:** Vô hiệu hóa hoàn toàn bộ quét Wi-Fi trong UI, stub các hàm gọi Wi-Fi, và chỉ sử dụng kết nối Ethernet ổn định làm kênh truyền dữ liệu chính.
*   **Kế hoạch xem xét:** Sau khi hoàn thành Phase 4 trên cổng LAN, chúng ta sẽ quay lại nghiên cứu tài liệu driver của Waveshare để viết hoặc cấu hình lại driver Wi-Fi SDIO/AT.

### 2. OTA (Over-The-Air) Firmware Update (Cập nhật phần mềm qua mạng)
*   **Thách thức:** Cần phân vùng lại bộ nhớ Flash trên ESP32-P4 để hỗ trợ 2 phân vùng OTA (OTA0, OTA1) nhằm nâng cấp firmware từ xa một cách an toàn mà không cần cắm cáp USB.
*   **Kế hoạch xem xét:** Sẽ triển khai sau khi Socket và Sync Engine đã hoạt động ổn định.

### 3. Đồng bộ Trạng thái Network trên Header UI
*   **Mô tả:** Chấm tròn xanh/đỏ biểu thị trạng thái kết nối mạng thực tế ở góc trên cùng bên phải màn hình chính (`ui_PanelHeaderNetworkDot` và `ui_LabelHeaderNetworkStatus`) hiện đang hiển thị tĩnh "Đã kết nối".
*   **Kế hoạch xem xét:** Sẽ được liên kết trực tiếp vào trạng thái thực tế của Ethernet (đọc từ `ks_network_is_connected`) trong bước UI Integration cuối cùng.


