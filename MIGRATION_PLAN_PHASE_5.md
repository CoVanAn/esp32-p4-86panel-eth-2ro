# Kế Hoạch Chuyển Đổi Phase 5: Local Web Control (Embedded Web Server)

## Mối cảnh và Hiện Trạng
Ở dự án gốc `KSmartZoneX-Node-Device` trên nền tảng Linux (Luckfox), hệ thống được chia làm hai tiến trình chạy song song:
1. **Tiến trình UI (C/LVGL):** Vẽ giao diện cảm ứng và thao tác phần cứng.
2. **Tiến trình Web Backend (Golang - `/go-services`):** Khởi tạo một HTTP Server ở cổng 80, chứa giao diện HTML/CSS, cung cấp các RESTful APIs và liên lạc nội bộ với tiến trình UI thông qua một TCP Socket (cổng 9000).

**Vấn đề trên ESP32:** ESP32 là vi điều khiển (RTOS) không chạy đa tiến trình như Linux. Chức năng `tcp_client` (TCP Socket cổng 9000) mà chúng ta viết ở Phase 4 vốn là để tiến trình UI kết nối sang tiến trình Golang. Nay Golang không còn, và `tcp_client` đã được bạn bóc tách riêng ra nhánh `fu/tcp_client` (điều này rất hợp lý).

Trong nhánh `fu/web_server` hiện tại, chúng ta sẽ hợp nhất chức năng của tiến trình UI và tiến trình Golang vào cùng một ứng dụng C++ duy nhất trên ESP32 bằng thư viện `WebServer.h`.

---

## Các Bước Triển Khai Chi Tiết (Phase 5)

### 5.1. Khởi tạo Web Service (Thay thế hàm `main` của Golang)
Tạo file mới `ks_web_service.cpp` và `ks_web_service.h`:
* Khởi tạo đối tượng `WebServer server(80)`.
* Tích hợp nó vào `loop()` của ứng dụng chính thông qua hàm `server.handleClient()`.
* Đảm bảo Web Server chạy không làm giật (block) LVGL rendering bằng cách cấu hình FreeRTOS Task riêng hoặc dùng `ESPAsyncWebServer` (nếu WebServer mặc định có dấu hiệu block). Ở đây ưu tiên dùng `<WebServer.h>` vì nó đã được tối ưu cực nhẹ.

### 5.2. Chuyển đổi Frontend (HTML/CSS/JS)
* **Trong file `main.go` cũ:** Giao diện Web được lưu ở dạng chuỗi nguyên thủy `const indexHTML`.
* **Trên ESP32:** Ta cũng sẽ lưu chuỗi HTML này y hệt vào C++ bằng định dạng:
  ```cpp
  const char indexHTML[] PROGMEM = R"=====(
    <!DOCTYPE html>...
  )=====";
  ```
  Sử dụng từ khoá `PROGMEM` để nạp chuỗi HTML dung lượng lớn thẳng vào bộ nhớ Flash, tránh làm đầy bộ nhớ RAM (Heap).
* Khai báo API `GET /`: `server.on("/", [](){ server.send(200, "text/html", indexHTML); });`

### 5.3. Porting các RESTful APIs (Thay thế Logic Backend Golang)
Chúng ta sẽ chuyển đổi chính xác các route API từ Golang sang C++:

1. **API `GET /api/relays`** (thay thế `handleGetRelays`)
   - Đọc trực tiếp file `relay_map.json` từ `LittleFS`.
   - Trả nội dung raw JSON về cho Client với Header `application/json`.

2. **API `POST /api/relays`** (thay thế `handlePostRelays`)
   - Nhận payload JSON cấu hình thiết bị (đổi tên).
   - Kiểm tra độ dài hợp lệ (1-30 ký tự) giống như Golang.
   - Ghi đè vào file `relay_map.json` trên `LittleFS`.

3. **API `POST /api/relay/control`** (thay thế `handleControlRelay`)
   - Nhận JSON `{ "channel": 1, "state": true }`.
   - Gọi trực tiếp hàm nội bộ `ks_relay_service_set(channel, state)` và đẩy sự kiện lên Queue để giao diện LVGL trên màn hình tự động bật/tắt công tắc đồng bộ với Web.

4. **API `GET /api/relay/status`** (thay thế `handleGetRelayStatus`)
   - Tạo ra một chuỗi JSON trả về trạng thái thời gian thực của toàn bộ Relay.

5. **API `/api/set-id` và `/api/reboot`**
   - **Set ID:** Gọi trực tiếp lệnh Modbus RS485 (FC 06) để đổi địa chỉ Slave ID.
   - **Reboot:** Gọi hàm `ESP.restart()`.

### 5.4. Chỉnh sửa tương thích với Hệ thống Cũ
* Trên hệ thống Golang cũ, khi một lệnh `/api/relay/control` được gọi, Golang sẽ bọc dữ liệu thành bản tin TCP `HUB_RELAY_COMMAND` để đẩy sang Socket.
* Trên ESP32, nhờ việc "hợp nhất" 2 tiến trình lại làm một, API Web sẽ gọi thẳng đến hàm đóng/mở GPIO hoặc Modbus (thông qua `ks_relay_service`) mà không cần đi qua tầng giao tiếp Socket (TCP) trung gian nào nữa. Tốc độ sẽ **nhanh hơn và không có độ trễ**.

---

## Kế hoạch hành động

1. **Bước 1:** Bóc tách và chuyển nguyên vẹn khối HTML/CSS khổng lồ từ `main.go` vào file `src/ksmart/ui_web_html.h`.
2. **Bước 2:** Viết khung logic `ks_web_service.cpp` và mapping các Endpoints.
3. **Bước 3:** Đấu nối API `control` và `status` với `ks_relay_service`.
4. **Bước 4:** Biên dịch và truy cập thử IP của màn hình qua điện thoại để thao tác bật/tắt thiết bị vật lý.
