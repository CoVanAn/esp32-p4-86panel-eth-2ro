#include "ks_device_service.h"
#include <string.h>
#include <stdio.h>
#include "esp_mac.h"
#include "../../core/ks_app_config.h"

void system_get_device_model(char *model) {
    if (model != NULL) {
        // Có thể lấy tên từ config hoặc hardcode
        strcpy(model, "ESP32-P4-86-Panel-ETH-2RO");
    }
}

int ks_device_service_get_primary_mac(char *mac_address, size_t mac_size) {
    uint8_t mac[6];
    
    if (mac_address == NULL || mac_size < 18) {
        return -1;
    }

    // Thử lấy MAC của Ethernet trước
    if (esp_read_mac(mac, ESP_MAC_ETH) == ESP_OK) {
        snprintf(mac_address, mac_size, "%02X:%02X:%02X:%02X:%02X:%02X", 
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return 0;
    }

    // Nếu không có, lấy MAC của WiFi Station
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
        snprintf(mac_address, mac_size, "%02X:%02X:%02X:%02X:%02X:%02X", 
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return 0;
    }

    return -1;
}
