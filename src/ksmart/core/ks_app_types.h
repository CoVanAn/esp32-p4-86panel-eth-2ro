#ifndef KS_APP_TYPES_H
#define KS_APP_TYPES_H

#include "ks_app_config.h"

/* Thông tin một mạng Wi-Fi lấy từ kết quả scan */
typedef struct {
    char ssid[MAX_CONF_LEN];
    int signal_level;
    char flags[MAX_CONF_LEN];
} wifi_network;

/* Dữ liệu truyền vào luồng kết nối Wi-Fi nền */
typedef struct {
    char ssid[MAX_CONF_LEN];
    char passwd[MAX_CONF_LEN];
} wifi_connect_info_t;

#endif
