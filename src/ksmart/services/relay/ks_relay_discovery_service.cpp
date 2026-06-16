#include "ks_relay_discovery_service.h"
#include "../../core/ks_app_config.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define KS_RELAY_DISCOVERY_MAX_RESULTS 247
#define KS_RELAY_DISCOVERY_ERROR_LEN 160
#define KS_RELAY_DISCOVERY_RX_BUFFER_LEN 260
#define KS_RELAY_DISCOVERY_INTER_FRAME_US 20000

extern SemaphoreHandle_t g_rs485_mutex;
static SemaphoreHandle_t g_relay_discovery_mutex = NULL;
static ks_relay_discovery_item_t g_relay_discovery_results[KS_RELAY_DISCOVERY_MAX_RESULTS];
static int g_relay_discovery_result_count = 0;
static bool g_relay_discovery_loading = false;
static int g_relay_discovery_start_id = RELAY_RS485_SCAN_START_ID;
static int g_relay_discovery_end_id = RELAY_RS485_SCAN_END_ID;
static char g_relay_discovery_last_error[KS_RELAY_DISCOVERY_ERROR_LEN];

typedef struct {
    int start_id;
    int end_id;
} ks_relay_discovery_request_t;

static void set_last_errorf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(g_relay_discovery_last_error, sizeof(g_relay_discovery_last_error), format, args);
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

static int send_modbus_read_request(uint8_t slave_id, uint8_t function_code, uint16_t register_address, uint16_t quantity, uint8_t *response, size_t response_size, int accept_exception_response) {
    uint8_t request[8];
    request[0] = slave_id;
    request[1] = function_code;
    request[2] = (register_address >> 8) & 0xFF;
    request[3] = register_address & 0xFF;
    request[4] = (quantity >> 8) & 0xFF;
    request[5] = quantity & 0xFF;
    uint16_t crc = modbus_crc16(request, 6);
    request[6] = crc & 0xFF;
    request[7] = (crc >> 8) & 0xFF;

    xSemaphoreTake(g_rs485_mutex, portMAX_DELAY);
    
    // Đợi 10ms (Inter-frame delay) để mạch rơ-le có đủ thời gian xử lý lệnh trước đó
    delay(10);

    // Flush RX buffer
    while (Serial1.available()) Serial1.read();

    Serial1.write(request, 8);
    Serial1.flush(); // wait for transmission to finish

    Serial1.setTimeout(RELAY_RS485_DISCOVERY_TIMEOUT_MS);
    size_t read_len = Serial1.readBytes(response, 3);
    
    // Ignore echo
    if (read_len >= 3 && response[0] == slave_id && response[1] == function_code && response[2] == request[2]) {
        Serial1.readBytes(response + 3, 5); // Read rest of echo
        read_len = Serial1.readBytes(response, 3); // Read real response
    }

    if (read_len < 3 || response[0] != slave_id) {
        xSemaphoreGive(g_rs485_mutex);
        return -1;
    }

    size_t expected_length;
    if (response[1] == (function_code | 0x80)) {
        expected_length = 5;
    } else if (response[1] == function_code) {
        expected_length = response[2] + 5;
    } else {
        xSemaphoreGive(g_rs485_mutex);
        return -1;
    }

    if (expected_length > response_size) {
        xSemaphoreGive(g_rs485_mutex);
        return -1;
    }

    if (expected_length > 3) {
        size_t rest_len = Serial1.readBytes(response + 3, expected_length - 3);
        if (rest_len < expected_length - 3) {
            xSemaphoreGive(g_rs485_mutex);
            return -1;
        }
    }
    
    xSemaphoreGive(g_rs485_mutex);

    uint16_t rx_crc = modbus_crc16(response, expected_length - 2);
    if (response[expected_length - 2] != (rx_crc & 0xFF) || response[expected_length - 1] != ((rx_crc >> 8) & 0xFF)) {
        return -1;
    }

    if (response[1] == (function_code | 0x80) && !accept_exception_response) {
        return -1;
    }
    return expected_length;
}

static int probe_slave_id(uint8_t slave_id) {
    uint8_t response[KS_RELAY_DISCOVERY_RX_BUFFER_LEN];
    return send_modbus_read_request(slave_id, 0x03, 0, 1, response, sizeof(response), 1) > 0;
}

static int detect_relay_count(uint8_t slave_id, int *relay_count_out) {
    static const uint16_t relay_count_candidates[] = {32, 16, 8, 4, 2, 1};
    uint8_t response[KS_RELAY_DISCOVERY_RX_BUFFER_LEN];

    if (send_modbus_read_request(slave_id, 0x01, 0, 1, response, sizeof(response), 0) <= 0) {
        return 0;
    }

    for (int i = 0; i < sizeof(relay_count_candidates) / sizeof(relay_count_candidates[0]); i++) {
        if (send_modbus_read_request(slave_id, 0x01, 0, relay_count_candidates[i], response, sizeof(response), 0) > 0) {
            *relay_count_out = relay_count_candidates[i];
            return 1;
        }
    }
    return 0;
}

void ks_relay_discovery_service_init(void) {
    if (g_relay_discovery_mutex == NULL) {
        g_relay_discovery_mutex = xSemaphoreCreateMutex();
    }
    xSemaphoreTake(g_relay_discovery_mutex, portMAX_DELAY);
    g_relay_discovery_result_count = 0;
    g_relay_discovery_loading = false;
    g_relay_discovery_start_id = RELAY_RS485_SCAN_START_ID;
    g_relay_discovery_end_id = RELAY_RS485_SCAN_END_ID;
    g_relay_discovery_last_error[0] = '\0';
    xSemaphoreGive(g_relay_discovery_mutex);
}

void ks_relay_discovery_service_start_default_scan(void) {
    ks_relay_discovery_service_start_scan(RELAY_RS485_SCAN_START_ID, RELAY_RS485_SCAN_END_ID);
}

static void relay_discovery_task(void *pvParameters) {
    ks_relay_discovery_request_t *req = (ks_relay_discovery_request_t *)pvParameters;
    int start_id = req->start_id;
    int end_id = req->end_id;
    free(req);

    ks_relay_discovery_item_t local_results[KS_RELAY_DISCOVERY_MAX_RESULTS];
    int local_count = 0;

    for (int id = start_id; id <= end_id; id++) {
        int relay_count = 0;
        if (!probe_slave_id(id)) continue;
        
        if (!detect_relay_count(id, &relay_count)) {
            relay_count = 8;
        }

        if (local_count < KS_RELAY_DISCOVERY_MAX_RESULTS) {
            local_results[local_count].slave_id = id;
            local_results[local_count].relay_count = relay_count;
            local_count++;
        }
    }

    xSemaphoreTake(g_relay_discovery_mutex, portMAX_DELAY);
    g_relay_discovery_result_count = local_count;
    memcpy(g_relay_discovery_results, local_results, local_count * sizeof(ks_relay_discovery_item_t));
    g_relay_discovery_loading = false;
    g_relay_discovery_last_error[0] = '\0';
    xSemaphoreGive(g_relay_discovery_mutex);

    vTaskDelete(NULL);
}

int ks_relay_discovery_service_start_scan(int start_id, int end_id) {
    if (start_id < 1 || end_id > 247 || start_id > end_id) return -1;

    xSemaphoreTake(g_relay_discovery_mutex, portMAX_DELAY);
    if (g_relay_discovery_loading) {
        set_last_errorf("Đang quét danh sách relay, vui lòng chờ hoàn tất");
        xSemaphoreGive(g_relay_discovery_mutex);
        return -1;
    }
    g_relay_discovery_loading = true;
    g_relay_discovery_result_count = 0;
    g_relay_discovery_start_id = start_id;
    g_relay_discovery_end_id = end_id;
    g_relay_discovery_last_error[0] = '\0';
    xSemaphoreGive(g_relay_discovery_mutex);

    ks_relay_discovery_request_t *req = (ks_relay_discovery_request_t *)malloc(sizeof(ks_relay_discovery_request_t));
    req->start_id = start_id;
    req->end_id = end_id;

    if (xTaskCreate(relay_discovery_task, "relay_scan", 4096, req, 5, NULL) != pdPASS) {
        free(req);
        xSemaphoreTake(g_relay_discovery_mutex, portMAX_DELAY);
        g_relay_discovery_loading = false;
        set_last_errorf("Không thể tạo luồng quét relay");
        xSemaphoreGive(g_relay_discovery_mutex);
        return -1;
    }
    return 0;
}

bool ks_relay_discovery_service_is_loading(void) {
    bool loading;
    xSemaphoreTake(g_relay_discovery_mutex, portMAX_DELAY);
    loading = g_relay_discovery_loading;
    xSemaphoreGive(g_relay_discovery_mutex);
    return loading;
}

int ks_relay_discovery_service_get_result_count(void) {
    int count;
    xSemaphoreTake(g_relay_discovery_mutex, portMAX_DELAY);
    count = g_relay_discovery_result_count;
    xSemaphoreGive(g_relay_discovery_mutex);
    return count;
}

int ks_relay_discovery_service_copy_results(ks_relay_discovery_item_t *out_items, int max_items) {
    int copied = 0;
    if (!out_items || max_items <= 0) return 0;
    xSemaphoreTake(g_relay_discovery_mutex, portMAX_DELAY);
    for (int i = 0; i < g_relay_discovery_result_count && copied < max_items; i++) {
        out_items[copied++] = g_relay_discovery_results[i];
    }
    xSemaphoreGive(g_relay_discovery_mutex);
    return copied;
}

void ks_relay_discovery_service_get_last_range(int *start_id, int *end_id) {
    xSemaphoreTake(g_relay_discovery_mutex, portMAX_DELAY);
    if (start_id) *start_id = g_relay_discovery_start_id;
    if (end_id) *end_id = g_relay_discovery_end_id;
    xSemaphoreGive(g_relay_discovery_mutex);
}

const char *ks_relay_discovery_service_get_last_error(void) {
    static char snapshot[KS_RELAY_DISCOVERY_ERROR_LEN];
    xSemaphoreTake(g_relay_discovery_mutex, portMAX_DELAY);
    if (g_relay_discovery_last_error[0] == '\0') {
        strcpy(snapshot, "Không có lỗi");
    } else {
        strcpy(snapshot, g_relay_discovery_last_error);
    }
    xSemaphoreGive(g_relay_discovery_mutex);
    return snapshot;
}

int ks_relay_discovery_service_scan_sync(int start_id, int end_id, ks_relay_discovery_item_t *out_items, int max_items) {
    if (start_id < 1 || end_id > 247 || start_id > end_id || !out_items || max_items <= 0) return -1;

    int copied = 0;
    for (int id = start_id; id <= end_id; id++) {
        int relay_count = 0;
        if (!probe_slave_id(id)) continue;
        
        // Nếu mạch phản hồi nhưng không dò được số relay (có thể do lỗi cấu trúc modbus hoặc không hỗ trợ FC01 ở địa chỉ 0),
        // mặc định gán cho nó là 8 relay để không bị bỏ qua.
        if (!detect_relay_count(id, &relay_count)) {
            relay_count = 8;
        }

        if (copied < max_items) {
            out_items[copied].slave_id = id;
            out_items[copied].relay_count = relay_count;
            copied++;
        }
    }
    return copied;
}
