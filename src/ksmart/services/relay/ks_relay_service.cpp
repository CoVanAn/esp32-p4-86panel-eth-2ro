#include "ks_relay_service.h"
#include "../../core/ks_app_config.h"
#include "../../core/ks_path_config.h"
#include "ks_relay_discovery_service.h"
#include "../../app/ks_app_runtime.h"
#include "../../ui.h"

#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <esp_ldo_regulator.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

SemaphoreHandle_t g_rs485_mutex = NULL;

#define KS_RS485_TIMEOUT_MS RELAY_RS485_TIMEOUT_MS
#define KS_RS485_WRITE_RESPONSE_LEN 8
#define KS_RELAY_ERROR_LEN 160
#define KS_RS485_MAX_BOARDS 32

typedef struct {
    uint8_t slave_id;
    int relay_count;
    int rs485_offset;
} ks_rs485_board_t;

typedef struct {
    uint8_t slave_id;
    uint16_t coil;
} ks_rs485_relay_map_item_t;

// ESP32-P4 board relays
static const int gpio_relay_pins[KS_GPIO_RELAY_COUNT] = {32, 46}; 

static bool relay_states[KS_RELAY_TOTAL_COUNT];
static bool rs485_cache_ready = false;
static char relay_last_error[KS_RELAY_ERROR_LEN];
static int actual_rs485_relay_count = 0;
static ks_rs485_board_t rs485_boards[KS_RS485_MAX_BOARDS];
static int rs485_board_count = 0;
static ks_rs485_relay_map_item_t rs485_relay_map[KS_RS485_RELAY_COUNT];

static void set_last_errorf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(relay_last_error, sizeof(relay_last_error), format, args);
    va_end(args);
}

static uint16_t modbus_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static int is_valid_relay_index(int index) {
    if (index < 0 || index >= KS_RELAY_TOTAL_COUNT) {
        set_last_errorf("Chỉ số relay %d nằm ngoài dải hợp lệ (0-%d)", index, KS_RELAY_TOTAL_COUNT - 1);
        return 0;
    }
    return 1;
}

static void discover_rs485_boards_and_build_map(void) {
    ks_relay_discovery_item_t items[KS_RS485_MAX_BOARDS];
    int count = ks_relay_discovery_service_scan_sync(RELAY_RS485_SCAN_START_ID, RELAY_RS485_SCAN_END_ID, items, KS_RS485_MAX_BOARDS);
    
    rs485_board_count = 0;
    actual_rs485_relay_count = 0;

    if (count > 0) {
        for (int i = 0; i < count; i++) {
            rs485_boards[i].slave_id = items[i].slave_id;
            rs485_boards[i].relay_count = items[i].relay_count;
            rs485_boards[i].rs485_offset = actual_rs485_relay_count;
            
            for (int coil = 0; coil < items[i].relay_count; coil++) {
                if (actual_rs485_relay_count < KS_RS485_RELAY_COUNT) {
                    rs485_relay_map[actual_rs485_relay_count].slave_id = items[i].slave_id;
                    rs485_relay_map[actual_rs485_relay_count].coil = coil;
                    actual_rs485_relay_count++;
                }
            }
            rs485_board_count++;
        }
    }
}

static void ensure_relay_map_json_exists(void) {
    if (LittleFS.exists(RELAY_NAME_JSON_PATH)) {
        return;
    }

    File f = LittleFS.open(RELAY_NAME_JSON_PATH, FILE_WRITE);
    if (!f) return;

    JsonDocument doc;
    doc["all_relay_delay_ms"] = 1000;
    doc["screen_sleep_seconds"] = 300;
    
    JsonObject devices = doc["devices"].to<JsonObject>();
    JsonObject device0 = devices["0"].to<JsonObject>();
    JsonObject relays0 = device0["relays"].to<JsonObject>();
    
    relays0["0"] = "Relay 1";
    relays0["1"] = "Relay 2";

    int global_idx = 1;
    for (int i = 0; i < rs485_board_count; i++) {
        JsonObject dev = devices[String(rs485_boards[i].slave_id)].to<JsonObject>();
        JsonObject rls = dev["relays"].to<JsonObject>();
        for (int c = 0; c < rs485_boards[i].relay_count; c++) {
            rls[String(c)] = "Relay " + String(global_idx++);
        }
    }

    doc["hide-device-id-0"] = 1;
    serializeJson(doc, f);
    f.close();
}

static int refresh_rs485_states(void) {
    if (rs485_board_count == 0) return 0;

    for (int i = 0; i < rs485_board_count; i++) {
        uint8_t request[8] = {rs485_boards[i].slave_id, 0x01, 0x00, 0x00, 
                              (uint8_t)(rs485_boards[i].relay_count >> 8), 
                              (uint8_t)(rs485_boards[i].relay_count & 0xFF), 0, 0};
        uint16_t crc = modbus_crc16(request, 6);
        request[6] = crc & 0xFF;
        request[7] = (crc >> 8) & 0xFF;

        xSemaphoreTake(g_rs485_mutex, portMAX_DELAY);
        while (Serial1.available()) Serial1.read();
        Serial1.write(request, 8);
        Serial1.flush();

        delay(20);
        uint8_t response[64];
        Serial1.setTimeout(KS_RS485_TIMEOUT_MS);
        
        int expected_bytes = (rs485_boards[i].relay_count + 7) / 8;
        int expected_len = expected_bytes + 5;
        
        size_t read_len = Serial1.readBytes(response, expected_len);
        xSemaphoreGive(g_rs485_mutex);

        if (read_len == expected_len && response[0] == rs485_boards[i].slave_id && response[1] == 0x01) {
            uint16_t rx_crc = modbus_crc16(response, expected_len - 2);
            if (response[expected_len - 2] == (rx_crc & 0xFF) && response[expected_len - 1] == ((rx_crc >> 8) & 0xFF)) {
                for (int r = 0; r < rs485_boards[i].relay_count; r++) {
                    int byte_idx = r / 8;
                    int bit_idx = r % 8;
                    int global_rs485_idx = rs485_boards[i].rs485_offset + r;
                    if (global_rs485_idx < actual_rs485_relay_count) {
                        relay_states[KS_GPIO_RELAY_COUNT + global_rs485_idx] = (response[3 + byte_idx] >> bit_idx) & 0x01;
                    }
                }
            }
        }
    }
    rs485_cache_ready = true;
    return 0;
}

void ks_relay_service_init(void) {
    if (g_rs485_mutex == NULL) {
        g_rs485_mutex = xSemaphoreCreateMutex();
    }

    memset(relay_states, 0, sizeof(relay_states));
    relay_last_error[0] = '\0';
    rs485_cache_ready = false;

    // Power up LDO VO4 to 3.3V (voltage domain for GPIO46, 47, 48)
    esp_ldo_channel_handle_t ldo4_handle = NULL;
    esp_ldo_channel_config_t ldo_vo4_config = {
        .chan_id = 4,
        .voltage_mv = 3300,
    };
    esp_err_t err = esp_ldo_acquire_channel(&ldo_vo4_config, &ldo4_handle);
    if (err != ESP_OK) {
        Serial.printf("Failed to set LDO VO4 to 3.3V: %d\n", err);
    }

    // GPIO Init
    for (int i = 0; i < KS_GPIO_RELAY_COUNT; i++) {
        pinMode(gpio_relay_pins[i], OUTPUT);
        digitalWrite(gpio_relay_pins[i], LOW);
    }

    // RS485 Init
    Serial1.begin(RELAY_RS485_BAUD, SERIAL_8N1, 48, 47);

    ks_relay_discovery_service_init();
    discover_rs485_boards_and_build_map();
    refresh_rs485_states();
    ensure_relay_map_json_exists();
}

int ks_relay_service_set(int relay_index, bool enabled) {
    if (!is_valid_relay_index(relay_index)) return -1;

    if (relay_index < KS_GPIO_RELAY_COUNT) {
        digitalWrite(gpio_relay_pins[relay_index], enabled ? HIGH : LOW);
        relay_states[relay_index] = enabled;
        return 0;
    }

    int rs485_index = relay_index - KS_GPIO_RELAY_COUNT;
    if (rs485_index >= actual_rs485_relay_count) return -1;

    ks_rs485_relay_map_item_t item = rs485_relay_map[rs485_index];
    uint8_t request[8] = {item.slave_id, 0x05, 
                          (uint8_t)(item.coil >> 8), (uint8_t)(item.coil & 0xFF), 
                          enabled ? (uint8_t)0xFF : (uint8_t)0x00, 0x00, 0, 0};
    uint16_t crc = modbus_crc16(request, 6);
    request[6] = crc & 0xFF;
    request[7] = (crc >> 8) & 0xFF;

    xSemaphoreTake(g_rs485_mutex, portMAX_DELAY);
    while (Serial1.available()) Serial1.read();
    Serial1.write(request, 8);
    Serial1.flush();

    delay(20);
    uint8_t response[8];
    Serial1.setTimeout(KS_RS485_TIMEOUT_MS);
    bool success = (Serial1.readBytes(response, 8) == 8);
    xSemaphoreGive(g_rs485_mutex);

    if (success) {
        relay_states[relay_index] = enabled;
        return 0;
    }

    set_last_errorf("Timeout ghi RS485");
    return -1;
}

int ks_relay_service_toggle(int relay_index, bool *enabled_after) {
    if (!is_valid_relay_index(relay_index)) return -1;
    bool new_state = !relay_states[relay_index];
    int res = ks_relay_service_set(relay_index, new_state);
    if (res == 0 && enabled_after) {
        *enabled_after = new_state;
    }
    return res;
}

int ks_relay_service_refresh_cached_states(void) {
    for (int i = 0; i < KS_GPIO_RELAY_COUNT; i++) {
        relay_states[i] = digitalRead(gpio_relay_pins[i]);
    }
    return refresh_rs485_states();
}

bool ks_relay_service_get_cached_state(int relay_index) {
    if (!is_valid_relay_index(relay_index)) return false;
    return relay_states[relay_index];
}

extern "C" {
    extern int g_used_channels_count;
    extern int g_hide_device_id_0;
}

int ks_relay_service_get_active_count(void) {
    int max_count = KS_GPIO_RELAY_COUNT + actual_rs485_relay_count;
    
    if (g_used_channels_count > 0) {
        int target_active = g_used_channels_count;
        if (g_hide_device_id_0 == 1) {
            target_active += KS_GPIO_RELAY_COUNT;
        }
        if (target_active < max_count) {
            return target_active;
        }
    }
    return max_count;
}

int ks_relay_service_get_physical_count(void) {
    int count = KS_GPIO_RELAY_COUNT + actual_rs485_relay_count;
    if (g_hide_device_id_0 == 1) {
        count -= KS_GPIO_RELAY_COUNT;
    }
    return count;
}

int ks_relay_service_get_board_offset(int slave_id) {
    if (slave_id == 0) return 0;
    for (int i = 0; i < rs485_board_count; i++) {
        if (rs485_boards[i].slave_id == slave_id) {
            return KS_GPIO_RELAY_COUNT + rs485_boards[i].rs485_offset;
        }
    }
    return -1;
}

const char *ks_relay_service_get_last_error(void) {
    return relay_last_error;
}

int ks_relay_service_get_board_count(void) {
    return rs485_board_count;
}

int ks_relay_service_get_board_info(int board_idx, uint8_t *slave_id, int *relay_count, int *rs485_offset) {
    if (board_idx < 0 || board_idx >= rs485_board_count) return -1;
    if (slave_id) *slave_id = rs485_boards[board_idx].slave_id;
    if (relay_count) *relay_count = rs485_boards[board_idx].relay_count;
    if (rs485_offset) *rs485_offset = rs485_boards[board_idx].rs485_offset;
    return 0;
}

int ks_relay_service_save_to_json(const char *path) {
    File f = LittleFS.open(path, FILE_WRITE);
    if (!f) return -1;
    
    JsonDocument doc;
    doc["all_relay_delay_ms"] = ks_app_runtime_get_all_relay_delay_ms();
    doc["screen_sleep_seconds"] = ks_app_runtime_get_screen_sleep_ms() / 1000;
    doc["hide-device-id-0"] = g_hide_device_id_0;
    doc["used_channels"] = g_used_channels_count;
    
    JsonObject devices = doc["devices"].to<JsonObject>();
    JsonObject device0 = devices["0"].to<JsonObject>();
    JsonObject relays0 = device0["relays"].to<JsonObject>();
    
    for (int coil = 0; coil < KS_GPIO_RELAY_COUNT; coil++) {
        relays0[String(coil)] = ui_get_relay_display_name(coil + 1);
    }

    for (int i = 0; i < rs485_board_count; i++) {
        JsonObject dev = devices[String(rs485_boards[i].slave_id)].to<JsonObject>();
        JsonObject rls = dev["relays"].to<JsonObject>();
        int board_offset = KS_GPIO_RELAY_COUNT + rs485_boards[i].rs485_offset;
        for (int c = 0; c < rs485_boards[i].relay_count; c++) {
            rls[String(c)] = ui_get_relay_display_name(board_offset + c + 1);
        }
    }

    serializeJson(doc, f);
    f.close();
    return 0;
}

int ks_relay_service_set_all(bool enabled) {
    int active_count = ks_relay_service_get_active_count();
    
    // Nếu thiết lập hide-device-id-0, active_count chỉ bao gồm các cổng ko bị ẩn
    // Nhưng vì UI loop qua `1 -> active_count`, ta cần đảm bảo index chạy tới đúng số cổng vật lý
    // Thực tế ks_relay_service_get_active_count() đã cộng thêm KS_GPIO_RELAY_COUNT nếu hide_device_id_0 == 1
    int errors = 0;
    
    for (int i = 0; i < active_count; i++) {
        if (g_hide_device_id_0 == 1 && i < KS_GPIO_RELAY_COUNT) {
            continue; // Bỏ qua 2 cổng GPIO nếu ẩn
        }
        if (ks_relay_service_set(i, enabled) != 0) {
            errors++;
        }
    }
    
    return (errors == 0) ? 0 : -1;
}

char *ks_vfs_read_text_file(const char *path, size_t *out_size) {
    if (!LittleFS.exists(path)) {
        if (out_size) *out_size = 0;
        return NULL;
    }
    File f = LittleFS.open(path, "r");
    if (!f) return NULL;
    size_t size = f.size();
    char *buf = (char *)malloc(size + 1);
    if (!buf) {
        f.close();
        return NULL;
    }
    f.readBytes(buf, size);
    buf[size] = '\0';
    f.close();
    if (out_size) *out_size = size;
    return buf;
}

long ks_vfs_get_mtime(const char *path) {
    if (!LittleFS.exists(path)) return 0;
    File f = LittleFS.open(path, "r");
    if (!f) return 0;
    long t = f.getLastWrite();
    f.close();
    return t;
}

static int send_modbus_write_register(uint8_t slave_id, uint16_t reg_addr, uint16_t value) {
    uint8_t request[8];
    request[0] = slave_id;
    request[1] = 0x06; // Function Code 06: Write Single Register
    request[2] = (reg_addr >> 8) & 0xFF;
    request[3] = reg_addr & 0xFF;
    request[4] = (value >> 8) & 0xFF;
    request[5] = value & 0xFF;
    uint16_t crc = modbus_crc16(request, 6);
    request[6] = crc & 0xFF;
    request[7] = (crc >> 8) & 0xFF;

    xSemaphoreTake(g_rs485_mutex, portMAX_DELAY);
    while (Serial1.available()) Serial1.read();
    Serial1.write(request, 8);
    Serial1.flush();

    delay(30); // wait for slave to process write & EEPROM write

    uint8_t response[8];
    Serial1.setTimeout(KS_RS485_TIMEOUT_MS);
    size_t read_len = Serial1.readBytes(response, 8);
    xSemaphoreGive(g_rs485_mutex);

    if (read_len < 8) {
        return -1; // timeout
    }
    
    // Check response CRC
    uint16_t rx_crc = modbus_crc16(response, 6);
    if (response[6] != (rx_crc & 0xFF) || response[7] != ((rx_crc >> 8) & 0xFF)) {
        return -2; // CRC error
    }

    // Response of FC 06 should be an echo of the query
    if (response[0] != slave_id || response[1] != 0x06 || 
        response[2] != request[2] || response[3] != request[3] ||
        response[4] != request[4] || response[5] != request[5]) {
        return -3; // invalid echo
    }

    return 0; // success
}

int ks_relay_service_change_slave_id(uint8_t old_id, uint8_t new_id) {
    if (old_id == new_id) return 0;
    if (new_id < 1 || new_id > 247) {
        set_last_errorf("ID mới %d không hợp lệ (1-247)", new_id);
        return -1;
    }

    // 1. Gửi lệnh Modbus FC06 xuống mạch để đổi ID vật lý.
    // Ưu tiên 0x4000 và 0x00FF (tuyệt đối không dùng 0x0000 vì hay trùng với Baud Rate)
    uint16_t reg_addresses[] = {0x4000, 0x00FF};
    bool hw_success = false;
    int err_code = 0;

    for (int i = 0; i < 2; i++) {
        uint16_t reg = reg_addresses[i];
        int res = send_modbus_write_register(old_id, reg, new_id);
        if (res == 0) {
            hw_success = true;
            break;
        } else {
            err_code = res;
        }
    }

    if (!hw_success) {
        set_last_errorf("Không thể đổi ID trên phần cứng (Modbus error %d)", err_code);
        return -2;
    }

    // 2. Đổi ID trên phần mềm (cập nhật file json)
    size_t size = 0;
    char *buf = ks_vfs_read_text_file(RELAY_NAME_JSON_PATH, &size);
    if (!buf) {
        set_last_errorf("Không thể đọc file relay_map.json");
        return -4;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, buf);
    free(buf); // giải phóng bộ nhớ ngay sau khi parse
    if (err) {
        set_last_errorf("Lỗi parse JSON file cấu hình: %s", err.c_str());
        return -5;
    }

    JsonObject devices = doc["devices"].as<JsonObject>();
    String old_key = String(old_id);
    String new_key = String(new_id);

    if (devices.containsKey(old_key)) {
        // Copy node cũ sang node mới
        devices[new_key] = devices[old_key];
        // Xoá node cũ
        devices.remove(old_key);
    } else {
        // Tạo node mới nếu chưa tồn tại
        JsonObject dev = devices[new_key].to<JsonObject>();
        JsonObject rls = dev["relays"].to<JsonObject>();
        int relay_count = 8;
        for (int i = 0; i < rs485_board_count; i++) {
            if (rs485_boards[i].slave_id == old_id) {
                relay_count = rs485_boards[i].relay_count;
                break;
            }
        }
        for (int c = 0; c < relay_count; c++) {
            rls[String(c)] = "Relay " + String(c + 1);
        }
    }

    // Lưu lại JSON
    File f = LittleFS.open(RELAY_NAME_JSON_PATH, FILE_WRITE);
    if (!f) {
        set_last_errorf("Không thể mở file relay_map.json để ghi");
        return -6;
    }
    serializeJson(doc, f);
    f.close();

    return 0; // Thành công
}

static int recovery_probe_slave(uint8_t slave_id, int timeout_ms) {
    uint8_t request[8] = {slave_id, 0x03, 0x00, 0x00, 0x00, 0x01, 0, 0};
    uint16_t crc = modbus_crc16(request, 6);
    request[6] = crc & 0xFF;
    request[7] = (crc >> 8) & 0xFF;

    xSemaphoreTake(g_rs485_mutex, portMAX_DELAY);
    while (Serial1.available()) Serial1.read();
    Serial1.write(request, 8);
    Serial1.flush();

    Serial1.setTimeout(timeout_ms);
    uint8_t response[16];
    size_t read_len = Serial1.readBytes(response, 3);
    
    // Check if this is an echo of our request (FC03, reg 0x0000)
    if (read_len >= 3 && response[0] == slave_id && response[1] == 0x03 && response[2] == 0x00) {
        // Read the remaining 5 bytes of the echo
        Serial1.readBytes(response + 3, 5);
        // Now wait for the real response from the slave
        read_len = Serial1.readBytes(response, 3);
    }
    
    if (read_len < 3 || response[0] != slave_id) {
        xSemaphoreGive(g_rs485_mutex);
        return 0;
    }

    size_t expected_length;
    if (response[1] == 0x83) { // Exception
        expected_length = 5;
    } else if (response[1] == 0x03) { // Normal response
        expected_length = response[2] + 5;
    } else {
        xSemaphoreGive(g_rs485_mutex);
        return 0;
    }

    if (expected_length > sizeof(response)) {
        xSemaphoreGive(g_rs485_mutex);
        return 0;
    }

    if (expected_length > 3) {
        size_t rest_len = Serial1.readBytes(response + 3, expected_length - 3);
        if (rest_len < expected_length - 3) {
            xSemaphoreGive(g_rs485_mutex);
            return 0;
        }
    }
    xSemaphoreGive(g_rs485_mutex);

    uint16_t rx_crc = modbus_crc16(response, expected_length - 2);
    if (response[expected_length - 2] != (rx_crc & 0xFF) || response[expected_length - 1] != ((rx_crc >> 8) & 0xFF)) {
        return 0;
    }

    return 1;
}

int ks_relay_service_recover_rs485(void) {
    int baud_rates[] = {9600, 4800, 2400, 19200, 115200, 1200};
    int recovered_id = -1;
    
    for (int b = 0; b < 6; b++) {
        Serial1.updateBaudRate(baud_rates[b]);
        delay(50);
        int timeout_ms = 100000 / baud_rates[b] + 30; // Dynamic timeout based on baud rate
        
        for (int id = 1; id <= 255; id++) {
            if (recovery_probe_slave(id, timeout_ms)) {
                recovered_id = id;
                
                // Khôi phục baud rate về 9600 nếu đang bị sai
                if (baud_rates[b] != 9600) {
                    send_modbus_write_register(id, 0x4001, 3); // R4D3B16
                    delay(30);
                    send_modbus_write_register(id, 0x0001, 3); // Boards khác
                    delay(30);
                    send_modbus_write_register(id, 0x0000, 3); // Nếu 0x0000 là baud rate (3 = 9600)
                    delay(30);
                }
                
                // Ép luôn ID về 1 để đảm bảo mạch hoạt động bình thường ở ID mặc định
                send_modbus_write_register(id, 0x4000, 1);
                delay(30);
                send_modbus_write_register(id, 0x00FF, 1);
                delay(30);
                
                recovered_id = 1;
                
                Serial1.updateBaudRate(9600);
                delay(100);
                return (baud_rates[b] << 8) | recovered_id;
            }
        }
    }
    
    Serial1.updateBaudRate(9600);
    delay(100);
    return recovered_id;
}

void ks_device_reboot(void) {
    ESP.restart();
}
