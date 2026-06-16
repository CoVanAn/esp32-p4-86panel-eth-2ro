#include "ks_network_service.h"
#include "../../ui.h"
#include <ETH.h>
#include <Network.h>
#include <WiFi.h>

typedef enum {
  NET_DISCONNECTED,
  NET_ETH_CONNECTED,
  NET_WIFI_CONNECTED,
  NET_INTERNET_OK
} ks_network_state_t;

static ks_network_state_t current_state = NET_DISCONNECTED;
static char current_ip[IP_SIZE] = "0.0.0.0";
static char current_mac[18] = "00:00:00:00:00:00";

int network_count = 0;
wifi_network networks[MAX_NETWORKS];
static bool is_scanning_wifi = false;

static void sync_ui_state(void);

static void update_network_state(void) {
  bool eth_up = ETH.linkUp();
  bool wifi_up = (WiFi.status() == WL_CONNECTED);

  if (eth_up) {
    current_state = NET_ETH_CONNECTED;
    strncpy(current_ip, ETH.localIP().toString().c_str(), IP_SIZE);
    strncpy(current_mac, ETH.macAddress().c_str(), sizeof(current_mac));
    Serial.printf("[Network] ETH Connected. IP: %s, MAC: %s\n", current_ip,
                  current_mac);
  } else if (wifi_up) {
    current_state = NET_WIFI_CONNECTED;
    strncpy(current_ip, WiFi.localIP().toString().c_str(), IP_SIZE);
    strncpy(current_mac, WiFi.macAddress().c_str(), sizeof(current_mac));
    Serial.printf("[Network] WiFi Connected. IP: %s, MAC: %s\n", current_ip,
                  current_mac);
  } else {
    if (current_state != NET_DISCONNECTED) {
      Serial.println("[Network] Disconnected.");
    }
    current_state = NET_DISCONNECTED;
    strncpy(current_ip, "0.0.0.0", IP_SIZE);
  }

  // LUÔN CẬP NHẬT UI KHI TRẠNG THÁI MẠNG THAY ĐỔI
  sync_ui_state();
}

void NetworkEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
  case ARDUINO_EVENT_ETH_START:
    Serial.println("[Network] ETH Started");
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    Serial.println("[Network] ETH Cable Plugged In");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    update_network_state();
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    update_network_state();
    // Bật WiFi nếu mất mạng dây
    Serial.println("[Network] Mất mạng dây, ưu tiên kết nối lại WiFi...");
    WiFi.begin();
    break;
  case ARDUINO_EVENT_ETH_STOP:
    Serial.println("[Network] ETH Stopped");
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    update_network_state();
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    update_network_state();
    break;
  default:
    break;
  }
}

void ks_network_manager_init(void) {
  Network.onEvent(NetworkEvent);

  // Khởi tạo Ethernet
  Serial.println("[Network] Khởi tạo Ethernet (IP101)...");
  if (!ETH.begin(ETH_PHY_IP101, 1, 31, 52, 51, EMAC_CLK_EXT_IN)) {
    Serial.println("[Network] LỖI: Không thể khởi tạo Ethernet!");
  }

  // Khởi tạo WiFi
  Serial.println("[Network] Khởi tạo WiFi...");
  WiFi.mode(WIFI_STA);
  // Cố gắng kết nối WiFi ở chế độ nền (lấy cấu hình đã lưu nếu có)
  WiFi.begin();
}

void ks_network_manager_loop(void) {
  // Xử lý quét WiFi bất đồng bộ
  if (is_scanning_wifi) {
    int16_t scan_result = WiFi.scanComplete();
    if (scan_result == WIFI_SCAN_FAILED) {
      Serial.println("[Network] WiFi Scan Failed!");
      is_scanning_wifi = false;
    } else if (scan_result >= 0) {
      Serial.printf("[Network] WiFi Scan Complete: %d networks found\n",
                    scan_result);
      network_count = scan_result < MAX_NETWORKS ? scan_result : MAX_NETWORKS;
      for (int i = 0; i < network_count; ++i) {
        strncpy(networks[i].ssid, WiFi.SSID(i).c_str(),
                sizeof(networks[i].ssid) - 1);
        networks[i].ssid[sizeof(networks[i].ssid) - 1] = '\0';
        networks[i].signal_level = WiFi.RSSI(i);
        if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) {
          strncpy(networks[i].flags, "WPA2", sizeof(networks[i].flags) - 1);
        } else {
          networks[i].flags[0] = '\0';
        }
      }
      WiFi.scanDelete();
      is_scanning_wifi = false;
    }
  }

  // Cập nhật UI an toàn trong vòng lặp chính của LVGL
  static uint32_t last_ui_update = 0;
  if (millis() - last_ui_update > 2000) {
    last_ui_update = millis();
    sync_ui_state();
  }
}

bool ks_network_is_connected(void) {
  return (current_state == NET_ETH_CONNECTED ||
          current_state == NET_WIFI_CONNECTED ||
          current_state == NET_INTERNET_OK);
}

bool ks_network_has_internet(void) { return ks_network_is_connected(); }

const char *ks_network_get_ip(void) { return current_ip; }
const char *ks_network_get_mac(void) { return current_mac; }

static char connected_wifi_ssid[33] = "";

const char *ks_network_get_wifi_ssid(void) {
  if (WiFi.status() == WL_CONNECTED) {
    strncpy(connected_wifi_ssid, WiFi.SSID().c_str(), 32);
    connected_wifi_ssid[32] = '\0';
    return connected_wifi_ssid;
  }
  return "";
}

int ks_network_get_wifi_signal(void) {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.RSSI();
  }
  return 0;
}

bool ks_network_is_scanning(void) { return is_scanning_wifi; }

static void sync_ui_state(void) {
  bool connected = ks_network_is_connected();

  if (ui_LabelEthIP != NULL) {
    if (ETH.linkUp()) {
      lv_label_set_text(ui_LabelEthIP, ETH.localIP().toString().c_str());
    } else {
      lv_label_set_text(ui_LabelEthIP, "No IP");
    }
    if (ui_ImageEth != NULL) {
      if (ETH.linkUp()) {
        lv_img_set_src(ui_ImageEth, &ui_img_icon_eth_on_png);
      } else {
        lv_img_set_src(ui_ImageEth, &ui_img_icon_eth_off_png);
      }
    }
  }

  if (ui_LabelWifiIP != NULL) {
    if (WiFi.status() == WL_CONNECTED) {
      String wifi_text =  WiFi.localIP().toString();
      lv_label_set_text(ui_LabelWifiIP, wifi_text.c_str());
      if (ui_ImageWifi != NULL) {
        lv_img_set_src(ui_ImageWifi, &ui_img_icon_wifi_on_png);
      }
    } else {
      lv_label_set_text(ui_LabelWifiIP, "Chưa kết nối");
      if (ui_ImageWifi != NULL) {
        lv_img_set_src(ui_ImageWifi, &ui_img_icon_wifi_off_png);
      }
    }
  }

  if (ui_PanelHeaderNetworkDot != NULL) {
    if (connected) {
      lv_obj_set_style_bg_color(ui_PanelHeaderNetworkDot,
                                lv_color_hex(0x22C55E), LV_PART_MAIN);
    } else {
      lv_obj_set_style_bg_color(ui_PanelHeaderNetworkDot,
                                lv_color_hex(0xEF4444), LV_PART_MAIN);
    }
  }

  if (ui_LabelHeaderNetworkStatus != NULL) {
    if (connected) {
      lv_label_set_text(ui_LabelHeaderNetworkStatus, "Đã kết nối");
    } else {
      lv_label_set_text(ui_LabelHeaderNetworkStatus, "Mất kết nối");
    }
  }
}

// ==========================================
// Legacy Compatibility APIs for Luckfox UI
// ==========================================
void ks_network_service_init_ui_state(void) { sync_ui_state(); }

void ks_network_service_refresh(void) {
  update_network_state();
  sync_ui_state();
}

void ks_network_service_process_ui_sync(void) { sync_ui_state(); }

int ks_network_service_has_connectivity(void) {
  return ks_network_is_connected() ? 1 : 0;
}

const char *ks_network_service_get_connection_type(void) {
  if (current_state == NET_ETH_CONNECTED)
    return "ETH";
  if (current_state == NET_WIFI_CONNECTED)
    return "WIFI";
  return "NONE";
}

int wifi_disconnect(const char *interface) {
  WiFi.disconnect();
  return 0;
}

void wifi_connect(const char *ssid, const char *password) {
  Serial.printf("[Network] Yêu cầu kết nối WiFi SSID: %s\n", ssid);
  WiFi.disconnect(false, true);
  delay(100);
  WiFi.mode(WIFI_STA);
  if (password != NULL && password[0] != '\0') {
    WiFi.begin(ssid, password);
  } else {
    WiFi.begin(ssid);
  }
}

int wifi_connect_async(const char *ssid, const char *password) {
  Serial.printf("[Network] Yêu cầu kết nối WiFi Async SSID: %s\n", ssid);
  WiFi.disconnect(false, true);
  delay(100);
  WiFi.mode(WIFI_STA);
  if (password != NULL && password[0] != '\0') {
    WiFi.begin(ssid, password);
  } else {
    WiFi.begin(ssid);
  }
  return 0;
}

void wifi_scr_init(void) {
  // Reserved
}

int wifi_scanning_ssid(void) {
  if (!is_scanning_wifi) {
    WiFi.scanNetworks(true); // tham số true = chạy ngầm (async)
    is_scanning_wifi = true;
    network_count = 0;
    Serial.println("[Network] Đang bắt đầu quét WiFi ở chế độ ngầm...");
  }
  return 0;
}
