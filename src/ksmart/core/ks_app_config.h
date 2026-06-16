#ifndef KS_APP_CONFIG_H
#define KS_APP_CONFIG_H

#include "ks_path_config.h"

/* Cấu hình địa chỉ TCP thô cho hub socket, ví dụ `tcp://192.168.1.21:9000`; để
 * rỗng nếu không dùng socket. */
#ifndef KS_SOCKET_URL
#define KS_SOCKET_URL "tcp://127.0.0.1:9000"
#endif

/* Device model */
#define LUCKFOX_PICO_86PANEL_W "Luckfox Pico 86Panel W"

#ifndef KS_APP_VERSION
#define KS_APP_VERSION "0.0.0"
#endif

/* Network interfaces */
#define WIFI_INTERFACE_NAME "wlan0"
#define ETH_INTERFACE_NAME "eth0"

/* Runtime defaults */
#define NTP_SERVER_ADDRESS "202.120.2.101"

/* Múi giờ của ứng dụng: Giờ Việt Nam (UTC+7) */
#define APP_TIMEZONE "UTC-7"

/* Cấu hình mặc định cho board relay Modbus RTU qua RS485. */
#define RELAY_RS485_DEVICE "/dev/ttyS4"
#define RELAY_RS485_SLAVE_ID 5
#define RELAY_RS485_BAUD 9600
#define RELAY_RS485_PARITY 'N'
#define RELAY_RS485_STOP_BITS 1
#define RELAY_RS485_TIMEOUT_MS 200

/* Dải ID Modbus mặc định để quét danh sách relay trên bus RS485. */
#ifndef RELAY_RS485_SCAN_START_ID
#define RELAY_RS485_SCAN_START_ID 1
#endif
#ifndef RELAY_RS485_SCAN_END_ID
#define RELAY_RS485_SCAN_END_ID 32
#endif

/* Timeout riêng cho tác vụ quét ID relay, bám theo tool Python để giảm thời
 * gian chờ. */
#ifndef RELAY_RS485_DISCOVERY_TIMEOUT_MS
#define RELAY_RS485_DISCOVERY_TIMEOUT_MS 80
#endif

/* Tổng số relay vật lý: 2 GPIO cục bộ + 14 RS485 = 16 relay. */
#ifndef KS_GPIO_RELAY_COUNT
#define KS_GPIO_RELAY_COUNT 2
#endif
#ifndef KS_RELAY_MAX_TOTAL_COUNT
/* Tổng số relay vật lý tối đa mà firmware/UI hỗ trợ (không tính card "Tất cả").
 * Yêu cầu bài toán: tối đa 64 relay. */
#define KS_RELAY_MAX_TOTAL_COUNT 64
#endif
#ifndef KS_RS485_RELAY_COUNT
/* Số relay RS485 tối đa có thể ánh xạ lên UI. Thực tế đang hoạt động sẽ được
 * dò động qua quét Modbus ID (có thể gom nhiều board: 4/8/16/32 kênh).
 * Mặc định giới hạn theo tổng max 64 relay của hệ thống. */
#define KS_RS485_RELAY_COUNT (KS_RELAY_MAX_TOTAL_COUNT - KS_GPIO_RELAY_COUNT)
#endif

/* Buffer sizes */
#define IP_SIZE 64
#define MAX_CONF_LEN 64
#define MAX_LINE_LEN 256
#define KS_HUB_URL_LEN 256
#define KS_HUB_TOKEN_LEN 1536
#define KS_HUB_TOPIC_LEN 256
#define KS_HUB_JSON_MAX_LEN 16384
#define KS_HUB_HTTP_RESPONSE_MAX_LEN 32768
#define KS_HUB_NONCE_LEN 64
#define KS_SOCKET_URL_LEN 256
#define MAX_NETWORKS 10
#define MAX_CMD_LEN 256
/* Giới hạn tên hiển thị của relay để UI nhập tay và hiển thị không bị tràn bộ
 * nhớ. */
#define MAX_RELAY_NAME_LEN 64

/* Tên thiết bị trên header màn chính (đồng bộ từ CMS qua HUB_DEVICE_INFO_SYNC).
 */
#ifndef KS_DEVICE_DISPLAY_NAME_MAX_LEN
#define KS_DEVICE_DISPLAY_NAME_MAX_LEN 64
#endif
#ifndef KS_DEVICE_DISPLAY_NAME_PATH
#define KS_DEVICE_DISPLAY_NAME_PATH "/ksmart_device_display_name.txt"
#define KS_DEVICE_DISPLAY_NAME_TMP_PATH                                        \
  "/ksmart_device_display_name.txt.tmp"
#endif

/* Tổng số card relay trên UI (1 card "Tất cả" + các relay vật lý).
 * Tên hiển thị/nguồn điều khiển không hardcode ở đây nữa: tên đọc qua
 * RELAY_NAME_JSON_PATH/RELAY_NAME_CONFIG_PATH và rơi về "Relay N", còn nguồn
 * suy ra từ chỉ số (ALL/GPIO/RS485) trong ui_get_relay_source(). */
#define KS_UI_RELAY_TOTAL_COUNT (KS_RELAY_TOTAL_COUNT + 1)

/* Khoảng chờ giữa hai relay khi chạy chuỗi "Tất cả"; tính theo milliseconds. */
#ifndef KS_RELAY_ALL_STEP_DELAY_MS
#define KS_RELAY_ALL_STEP_DELAY_MS 2000
#endif

#endif
