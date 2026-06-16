#include "ks_time_service.h"
#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include <Preferences.h>
#include "esp_sntp.h"
#include "../../ui.h"
#include <lvgl.h>

static char last_error[64] = "";
static Preferences preferences;
static bool time_synced = false;
static uint32_t last_nvs_save_time = 0;

static const char* get_vietnamese_wday(int wday) {
    switch(wday) {
        case 0: return "Chủ Nhật";
        case 1: return "Thứ Hai";
        case 2: return "Thứ Ba";
        case 3: return "Thứ Tư";
        case 4: return "Thứ Năm";
        case 5: return "Thứ Sáu";
        case 6: return "Thứ Bảy";
        default: return "--";
    }
}

#define TIME_SAVE_INTERVAL_MS 60000 // Lưu giờ vào NVS mỗi 60 giây

// Timezone Việt Nam (UTC+7)
// Trong POSIX, múi giờ GMT+7 được biểu diễn ngược dấu là "GMT-7"
#define TZ_INFO "GMT-7"

void time_sync_notification_cb(struct timeval *tv) {
    Serial.println("[Time] Đã đồng bộ thời gian thành công từ NTP Server!");
    time_synced = true;
}

int ks_time_service_sync_ntp(void) {
    preferences.begin("ks_time", false);
    uint32_t last_epoch = preferences.getUInt("last_epoch", 0);
    preferences.end();

    // Nếu trong NVS có lưu giờ cũ, nạp làm giờ khởi điểm
    if (last_epoch > 1700000000) { // Lớn hơn mốc cuối năm 2023
        struct timeval tv = { .tv_sec = (time_t)last_epoch, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        Serial.printf("[Time] Đã nạp thời gian khởi điểm từ NVS NVM: %u\n", last_epoch);
    } else {
        Serial.println("[Time] NVS trống hoặc không hợp lệ, sử dụng thời gian mặc định 1970.");
    }

    // Thiết lập NTP
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov", "time.windows.com");
    
    Serial.println("[Time] Đã gửi yêu cầu đồng bộ NTP...");
    return 0;
}

const char *ks_time_service_get_last_error(void) {
    if (strlen(last_error) == 0) {
        return "None";
    }
    return last_error;
}

void ks_time_service_refresh_labels(void) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Lưu giờ vào NVS định kỳ
    if (time_synced && (millis() - last_nvs_save_time >= TIME_SAVE_INTERVAL_MS)) {
        last_nvs_save_time = millis();
        preferences.begin("ks_time", false);
        preferences.putUInt("last_epoch", now);
        preferences.end();
    }

    // Cập nhật giao diện (UI) mỗi khi giây thay đổi
    static int last_sec = -1;
    if (timeinfo.tm_sec != last_sec) {
        last_sec = timeinfo.tm_sec;
        
        if (ui_LabelTime != NULL) {
            char time_str[16];
            snprintf(time_str, sizeof(time_str), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
            lv_label_set_text(ui_LabelTime, time_str);
        }
        if (ui_LabelDate != NULL) {
            char date_str[64];
            snprintf(date_str, sizeof(date_str), "%s, %02d/%02d/%04d", 
                     get_vietnamese_wday(timeinfo.tm_wday), 
                     timeinfo.tm_mday, 
                     timeinfo.tm_mon + 1, 
                     timeinfo.tm_year + 1900);
            lv_label_set_text(ui_LabelDate, date_str);
        }

        // Chỉ in ra Serial debug mỗi 10 giây
        if (timeinfo.tm_sec % 10 == 0) {
            char str[64];
            strftime(str, sizeof(str), "%Y-%m-%d %H:%M:%S", &timeinfo);
            Serial.printf("[Time] Hiện tại: %s (Synced: %s)\n", str, time_synced ? "YES" : "NO");
        }
    }
}
