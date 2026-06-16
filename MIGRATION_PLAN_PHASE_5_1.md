# Kế hoạch thực thi Phase 5.1: Giao diện Cài đặt nâng cao & Đổi ID Modbus RS485

## 1. Mục tiêu
Tái cấu trúc lại giao diện màn hình `Cài đặt` (Local UI trên ESP32) để bố trí lại các nút bấm hợp lý hơn, đồng thời phát triển thêm màn hình `Cài đặt nâng cao` cho phép thiết lập trực tiếp các thông số hệ thống và đặc biệt là ép đổi ID vật lý của các mạch RS485 bằng giao thức Modbus FC06.

## 2. Thiết kế Giao diện (UI)

### 2.1. Cập nhật Màn hình Cài đặt (`ui_ScreenSettings`)
Thay đổi bố cục các nút bấm hiện tại thành dạng danh sách lưới từ trên xuống dưới:
- **Hàng 1 (Chia đôi):** Nút **Wi-Fi** (trái, 50% chiều rộng) và Nút **Ethernet** (phải, 50% chiều rộng).
- **Hàng 2 (Full):** Nút **Cài đặt Kênh** (100% chiều rộng).
- **Hàng 3 (Full):** Nút **Cài đặt nâng cao** (100% chiều rộng) -> Điều hướng sang màn hình mới.

### 2.2. Tạo Màn hình mới: Cài đặt nâng cao (`ui_ScreenSettingsAdv`)
Màn hình này sẽ hiển thị dưới dạng danh sách (list) hoặc các thẻ cấu hình bao gồm:
1. **Đổi mã thiết bị (RS485 ID):** Nhấn vào mở ra giao diện thay đổi ID (Sẽ ưu tiên làm trước).
2. **Độ trễ Bật/Tắt tất cả:** Cho phép chọn delay tính bằng mili-giây.
3. **Thời gian tắt màn hình:** Cho phép chọn số giây màn hình sẽ tự động tắt để tiết kiệm điện.

### 2.3. Quy trình của tính năng "Đổi mã thiết bị"
- Khi người dùng chọn "Đổi mã thiết bị", màn hình hiển thị danh sách các ID của mạch RS485 đang được ESP32 nhận diện (ví dụ: ID 1, ID 2).
- Người dùng chọn một ID (ví dụ ID 1), sau đó nhập/chọn ID mới muốn đổi (từ 1 đến 247).
- Bấm nút **Lưu (Save)**, màn hình hiện Modal chờ và thực hiện 2 thao tác dưới nền:
  1. **Đổi ID phần cứng:** Gửi lệnh Modbus FC 06 (Write Single Register) xuống mạch RS485 có ID cũ để ép nó nhận ID mới.
  2. **Đổi ID phần mềm:** Đọc file `relay_map.json`, tìm object `"1"` đổi tên key thành `"ID_Mới"`, sau đó lưu lại file.
- Thành công: Thông báo yêu cầu Khởi động lại (Reboot) để mạch RS485 nhận ID mới và ESP32 quét lại toàn mạng.

## 3. Kiến trúc Mã nguồn (Logic)

### 3.1. Viết lệnh Modbus FC06 trong `ks_relay_discovery_service.cpp`
- **Hàm cần viết:** `int send_modbus_write_register(uint8_t slave_id, uint16_t reg_addr, uint16_t value)`
- Chức năng: Gửi bản tin Modbus chuẩn: `[Slave_ID] [0x06] [Reg_High] [Reg_Low] [Val_High] [Val_Low] [CRC_L] [CRC_H]`.
- *Lưu ý quan trọng:* Địa chỉ thanh ghi (Register Address) dùng để đổi ID RS485 thay đổi tuỳ theo hãng sản xuất thiết bị (Thường là `0x0000`, `0x4000`, `0x00FF`...). Cần thiết lập biến này ở dạng Macro hoặc thử nghiệm để tìm ra thanh ghi chuẩn của loại mạch đang dùng.

### 3.2. Cập nhật `ks_relay_service.cpp`
- **Hàm cần viết:** `ks_relay_service_change_slave_id(uint8_t old_id, uint8_t new_id)`
- Quy trình:
  1. Cập nhật `relay_map.json`: Lấy node của thiết bị cũ, tạo node mới bằng ID mới, copy dữ liệu (tên các relay) sang, xoá node cũ.
  2. Gọi lệnh Modbus xuống tầng vật lý.

### 3.3. Tích hợp giao diện LVGL
- Cập nhật `ui_ScreenSettings.c` để vẽ lại Layout 3 hàng.
- Sinh mới file `ui_ScreenSettingsAdv.c` và màn hình `ui_ScreenSettingsAdv`.
- Vẽ các Modal chọn ID cũ và nhập ID mới.

## 4. Các bước thực hiện
1. **Bước 1:** Thiết kế lại màn hình `ui_ScreenSettings` (hàng Wi-Fi/ETH chia đôi, thêm nút Nâng cao).
2. **Bước 2:** Xây dựng màn hình `ui_ScreenSettingsAdv` (Giao diện tĩnh).
3. **Bước 3:** Viết hàm Modbus vật lý FC 06 và hàm cập nhật JSON trong Backend.
4. **Bước 4:** Đấu nối giao diện tính năng "Đổi ID" với Backend, thêm thông báo popup và khởi động lại.
5. **Bước 5:** Hoàn thiện 2 chức năng phụ (Đổi thời gian sleep màn hình và Thời gian delay relay) - **[ĐÃ HOÀN THÀNH]**
   - Thiết kế Popup Modal đa lựa chọn (grid 3x2) cho cả 2 cài đặt.
   - Đồng bộ giá trị trực tiếp vào hệ thống runtime (RAM) và tự động ghi đè lưu trữ xuống `/relay_map.json` (Flash).
   - Tự động hiển thị và cập nhật giá trị hiện tại của cấu hình lên phần mô tả (subtitle) của từng Card một cách trực quan.
