#include "ks_web_service.h"
#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "ks_web_html.h"
#include "../relay/ks_relay_service.h"
#include "../network/ks_network_service.h"
#include "../../app/ks_app_runtime.h"

// Web Server lắng nghe ở cổng HTTP mặc định (80)
static WebServer server(80);

static void set_cors_headers() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

static void handle_options() {
    set_cors_headers();
    server.send(204);
}

// Kiểm tra trạng thái mạng. Nếu mất kết nối mạng LAN thì lập tức trả về lỗi 404 bảo mật.
static bool check_network() {
    if (!ks_network_is_connected()) {
        set_cors_headers();
        server.send(404, "text/plain", "404 Not Found");
        return false;
    }
    return true;
}

// 1. Phục vụ giao diện HTML tĩnh (Trang chủ)
static void handle_root() {
    if (!check_network()) return;
    set_cors_headers();
    server.send_P(200, "text/html", indexHTML);
}

// 2. Lấy danh sách thiết bị và cấu hình từ file JSON (thay thế handleGetRelays)
static void handle_get_relays() {
    if (!check_network()) return;
    set_cors_headers();
    if (!LittleFS.exists("/relay_map.json")) {
        server.send(200, "application/json", "{\"devices\":{}}");
        return;
    }

    File file = LittleFS.open("/relay_map.json", "r");
    if (!file) {
        server.send(500, "text/plain", "Lỗi đọc cấu hình hệ thống");
        return;
    }

    server.streamFile(file, "application/json");
    file.close();
}

// 3. Cập nhật tên Relay và cấu hình vào file JSON (thay thế handlePostRelays)
static void handle_post_relays() {
    if (!check_network()) return;
    set_cors_headers();
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Thiếu nội dung JSON");
        return;
    }

    String body = server.arg("plain");

    // Mở file để ghi trực tiếp xuống Flash
    File file = LittleFS.open("/relay_map.json", "w");
    if (!file) {
        server.send(500, "text/plain", "Lỗi ghi cấu hình hệ thống");
        return;
    }
    file.print(body);
    file.close();

    // Để cập nhật ngay lập tức giao diện màn hình, tải lại runtime
    ks_app_runtime_load_settings();

    server.send(200, "application/json", "{\"status\":\"success\"}");
}

// 4. Lấy trạng thái Bật/Tắt thời gian thực của toàn bộ Relay (thay thế handleGetRelayStatus)
static void handle_relay_status() {
    if (!check_network()) return;
    set_cors_headers();
    
    // Yêu cầu lấy thông tin trạng thái mới nhất từ Modbus/GPIO
    ks_relay_service_refresh_cached_states();
    
    // Trả về JSON map kiểu: {"1": true, "2": false, ...}
    DynamicJsonDocument doc(1024);
    
    int active_count = ks_relay_service_get_active_count();
    // Vòng lặp lấy trạng thái (relay_index 0-based mapped to 1-based channel on Web UI)
    for (int i = 0; i < KS_RELAY_TOTAL_COUNT; i++) {
        // Chỉ lấy trạng thái các relay nhỏ hơn tổng số active (hoặc tất cả nếu fix cứng UI)
        bool state = ks_relay_service_get_cached_state(i);
        doc[String(i + 1)] = state;
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// 5. Điều khiển Bật/Tắt một hoặc tất cả Relay từ điện thoại (thay thế handleControlRelay)
static void handle_relay_control() {
    if (!check_network()) return;
    set_cors_headers();
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Thiếu nội dung");
        return;
    }

    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "text/plain", "JSON sai định dạng");
        return;
    }

    int channel = doc["channel"] | -1;
    bool state = doc["state"] | false;

    if (channel == 0) {
        // Tắt/Bật TẤT CẢ theo trình tự thời gian cấu hình
        ks_app_runtime_schedule_all_relays(state);
    } else if (channel > 0 && channel <= KS_RELAY_TOTAL_COUNT) {
        // Điều khiển một Relay (Cấu trúc channel 1-based -> relay_index 0-based)
        ks_relay_service_set(channel - 1, state);
        // Đẩy thông điệp ra giao diện màn hình LVGL để nhảy đèn LED
        ks_app_runtime_push_ui_sync_event();
    } else {
        server.send(400, "text/plain", "Kênh không hợp lệ");
        return;
    }

    server.send(200, "application/json", "{\"status\":\"success\"}");
}

// 6. Tính năng khởi động lại thiết bị (thay thế handleReboot)
static void handle_reboot() {
    if (!check_network()) return;
    set_cors_headers();
    server.send(200, "application/json", "{\"status\":\"success\"}");
    
    // Chờ 1 giây để phản hồi HTTP kịp gửi đi trước khi reboot
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP.restart();
}

// 7. Thay đổi địa chỉ Slave ID của mạch Modbus (thay thế handleSetID)
static void handle_set_id() {
    if (!check_network()) return;
    set_cors_headers();
    server.send(501, "text/plain", "Đổi ID qua Web chưa hỗ trợ ở phiên bản này, vui lòng dùng màn hình!");
}

// 8. Lấy trạng thái Wi-Fi hiện tại
static void handle_wifi_status() {
    if (!check_network()) return;
    set_cors_headers();
    
    DynamicJsonDocument doc(256);
    doc["connected"] = (WiFi.status() == WL_CONNECTED);
    doc["ssid"] = ks_network_get_wifi_ssid();
    doc["signal"] = ks_network_get_wifi_signal();
    doc["ip"] = WiFi.localIP().toString();

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// 9. Yêu cầu kết nối Wi-Fi mới
static void handle_wifi_connect() {
    if (!check_network()) return;
    set_cors_headers();
    
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Thiếu nội dung JSON");
        return;
    }

    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "text/plain", "JSON sai định dạng");
        return;
    }

    const char* ssid = doc["ssid"] | "";
    const char* password = doc["password"] | "";

    if (strlen(ssid) == 0) {
        server.send(400, "text/plain", "Thiếu SSID");
        return;
    }

    // Gọi hàm kết nối Wi-Fi (lưu ý: hàm này đã được tối ưu bất đồng bộ trong ks_network_service)
    wifi_connect_async(ssid, password);

    server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Đang tiến hành kết nối\"}");
}

// Task chạy độc lập cho Web Server (Core 1 - Không làm giật màn hình)
static void web_server_task(void *pvParameters) {
    // Đăng ký các Routes
    server.on("/", HTTP_GET, handle_root);
    server.on("/api/relays", HTTP_GET, handle_get_relays);
    server.on("/api/relays", HTTP_POST, handle_post_relays);
    server.on("/api/relay/status", HTTP_GET, handle_relay_status);
    server.on("/api/relay/control", HTTP_POST, handle_relay_control);
    server.on("/api/reboot", HTTP_POST, handle_reboot);
    server.on("/api/set-id", HTTP_POST, handle_set_id);
    server.on("/api/wifi/status", HTTP_GET, handle_wifi_status);
    server.on("/api/wifi/connect", HTTP_POST, handle_wifi_connect);
    
    server.onNotFound([]() {
        if (server.method() == HTTP_OPTIONS) {
            handle_options();
        } else {
            set_cors_headers();
            server.send(404, "text/plain", "404 Not Found");
        }
    });

    Serial.println("[Web Service] Đang chờ mạng LAN...");
    while (!ks_network_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    server.begin();
    Serial.println("[Web Service] Đã chạy HTTP Server tại cổng 80");

    // Lặp vĩnh viễn để xử lý các yêu cầu từ điện thoại
    while (1) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10)); // Nhường CPU cho các Task khác
    }
}

// Khởi tạo Web Server (được gọi từ app_runtime)
extern "C" void ks_web_service_init(void) {
    xTaskCreatePinnedToCore(
        web_server_task,
        "web_task",
        8192,     // Stack size (8KB) để an toàn xử lý JSON lớn
        NULL,     // Không truyền tham số
        2,        // Priority 2
        NULL,     // Không giữ handle
        1         // Chạy trên Core 1 (cùng Core mạng)
    );
}
