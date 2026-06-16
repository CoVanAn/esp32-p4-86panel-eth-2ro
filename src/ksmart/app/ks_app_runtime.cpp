#include "../services/device/ks_device_service.h"
#include "../services/network/ks_network_service.h"
#include "../services/relay/ks_relay_service.h"
#include "../services/web/ks_web_service.h"
#include "../app/ks_app_runtime.h"
#include "../ui.h"
#include "../core/ks_path_config.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// --- Types & Globals ---

typedef enum {
    UI_MSG_RELAY_STATE_CHANGE,
    UI_MSG_RELAY_NAME_SYNC,
    UI_MSG_DEVICE_INFO_SYNC,
    UI_MSG_ALL_RELAYS_SEQUENCE
} UIMessageType;

typedef struct {
    UIMessageType type;
    int relay_index;
    bool state;
    char text_data[64];
} UIMessage;

static QueueHandle_t g_ui_queue = NULL;

lv_timer_t *ui_Timer = NULL;
int timer_count = 0;

static int g_all_relay_delay_ms = 300;
static int g_screen_sleep_ms = 60000;

static lv_timer_t *sleep_check_timer = NULL;
static lv_timer_t *seq_toggle_timer = NULL;
static int seq_toggle_index = 0;
static bool seq_toggle_target = false;

extern int g_hide_device_id_0;

// --- Forward Declarations ---
static void process_ui_queue(void);


// --- Timers ---

static void sleep_check_timer_cb(lv_timer_t *timer) {
    (void)timer;
    uint32_t inactive_time = lv_disp_get_inactive_time(NULL);
    int sleep_ms = g_screen_sleep_ms;
    
    if (sleep_ms > 0 && inactive_time >= (uint32_t)sleep_ms) {
        if (!ui_is_screen_locked()) {
            ui_set_screen_locked(true);
        }
    }

    // Process UI messages from Socket task
    process_ui_queue();
}

static void seq_toggle_timer_cb(lv_timer_t *timer) {
    int active_count = ks_relay_service_get_active_count();
    int start_index = (g_hide_device_id_0 == 1) ? 2 : 0;

    if (seq_toggle_target) {
        // Bật tất cả: chạy từ trên xuống
        int found_idx = -1;
        for (int i = seq_toggle_index; i < active_count; i++) {
            if (i < start_index) continue;
            if (ks_relay_service_get_cached_state(i) == false) {
                found_idx = i;
                break;
            }
        }

        if (found_idx != -1) {
            ks_relay_service_set(found_idx, true);
            ui_apply_relay_state_change(found_idx + 1, true);
            seq_toggle_index = found_idx + 1;
        } else {
            lv_timer_delete(timer);
            seq_toggle_timer = NULL;
        }
    } else {
        // Tắt tất cả: chạy từ dưới lên
        int found_idx = -1;
        for (int i = seq_toggle_index; i >= start_index; i--) {
            if (i >= active_count) continue;
            if (ks_relay_service_get_cached_state(i) == true) {
                found_idx = i;
                break;
            }
        }

        if (found_idx != -1) {
            ks_relay_service_set(found_idx, false);
            ui_apply_relay_state_change(found_idx + 1, false);
            seq_toggle_index = found_idx - 1;
        } else {
            lv_timer_delete(timer);
            seq_toggle_timer = NULL;
        }
    }
}

// --- UI Queue Processor (Runs in LVGL Core 0) ---

static void process_ui_queue(void) {
    if (g_ui_queue == NULL) return;
    
    UIMessage msg;
    while (xQueueReceive(g_ui_queue, &msg, 0) == pdTRUE) {
        switch (msg.type) {
            case UI_MSG_RELAY_STATE_CHANGE:
                ui_apply_relay_state_change(msg.relay_index + 1, msg.state);
                break;
            case UI_MSG_RELAY_NAME_SYNC:
                ui_set_relay_display_name(msg.relay_index + 1, msg.text_data);
                ks_relay_service_save_to_json(RELAY_NAME_JSON_PATH);
                break;
            case UI_MSG_DEVICE_INFO_SYNC:
                ui_set_header_device_display_name(msg.text_data);
                break;
            case UI_MSG_ALL_RELAYS_SEQUENCE:
                ks_app_runtime_schedule_all_relays_from_ui(msg.state);
                break;
        }
    }
}

// --- Initialization ---

void custom_init(void) {
    g_ui_queue = xQueueCreate(20, sizeof(UIMessage));

    ks_relay_service_init();

    // Khởi tạo và cắm Web Server chạy nền
    ks_web_service_init();
}

void main_timer_init(void) {
    if (sleep_check_timer == NULL) {
        // Reduced to 100ms so the queue is processed quickly
        sleep_check_timer = lv_timer_create(sleep_check_timer_cb, 100, NULL);
    }
}

void main_timer_release(void) {
    if (sleep_check_timer != NULL) {
        lv_timer_delete(sleep_check_timer);
        sleep_check_timer = NULL;
    }
}

// --- App Runtime Functions ---

void ks_app_runtime_schedule_all_relays(bool enabled) {
    UIMessage msg;
    msg.type = UI_MSG_ALL_RELAYS_SEQUENCE;
    msg.state = enabled;
    xQueueSend(g_ui_queue, &msg, 0);
}

void ks_app_runtime_schedule_all_relays_from_ui(bool enabled) {
    if (seq_toggle_timer != NULL) {
        lv_timer_delete(seq_toggle_timer);
    }
    
    int active_count = ks_relay_service_get_active_count();
    seq_toggle_target = enabled;
    
    if (enabled) {
        seq_toggle_index = (g_hide_device_id_0 == 1) ? 2 : 0;
    } else {
        seq_toggle_index = active_count - 1;
    }
    
    int delay_ms = g_all_relay_delay_ms;
    if (delay_ms < 50) delay_ms = 50;
    
    seq_toggle_timer = lv_timer_create(seq_toggle_timer_cb, delay_ms, NULL);
    seq_toggle_timer_cb(seq_toggle_timer);
}

void ks_app_runtime_cancel_all_relays(void) {
    if (seq_toggle_timer != NULL) {
        lv_timer_delete(seq_toggle_timer);
        seq_toggle_timer = NULL;
    }
}

void ks_app_runtime_set_all_relay_delay_ms(int ms) {
    g_all_relay_delay_ms = ms;
}

int ks_app_runtime_get_all_relay_delay_ms(void) {
    return g_all_relay_delay_ms;
}

void ks_app_runtime_set_screen_sleep_ms(int ms) {
    g_screen_sleep_ms = ms;
}

int ks_app_runtime_get_screen_sleep_ms(void) {
    return g_screen_sleep_ms;
}

void ks_app_runtime_load_settings(void) {
    if (!LittleFS.exists(RELAY_NAME_JSON_PATH)) {
        return;
    }
    File file = LittleFS.open(RELAY_NAME_JSON_PATH, "r");
    if (!file) {
        return;
    }
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (!error) {
        if (doc.containsKey("all_relay_delay_ms")) {
            g_all_relay_delay_ms = doc["all_relay_delay_ms"];
            Serial.printf("[Runtime] Đã nạp lại độ trễ All Relay: %d ms\n", g_all_relay_delay_ms);
        }
        if (doc.containsKey("screen_sleep_seconds")) {
            int sleep_sec = doc["screen_sleep_seconds"];
            g_screen_sleep_ms = sleep_sec * 1000;
            Serial.printf("[Runtime] Đã nạp lại thời gian tắt màn hình: %d ms\n", g_screen_sleep_ms);
        }
    } else {
        Serial.println("[Runtime] Lỗi phân giải cấu hình JSON để nạp cài đặt!");
    }
}

void ks_app_runtime_push_ui_sync_event(void) {
    // Web Server gọi hàm này sau khi nó set state (tắt/bật relay)
    // Để đẩy trạng thái mới nhất lên giao diện màn hình LVGL, ta quét lại toàn bộ:
    int active = ks_relay_service_get_active_count();
    for (int i = 0; i < active; i++) {
        bool state = ks_relay_service_get_cached_state(i);
        UIMessage msg;
        msg.type = UI_MSG_RELAY_STATE_CHANGE;
        msg.relay_index = i;
        msg.state = state;
        xQueueSend(g_ui_queue, &msg, 0);
    }
}
