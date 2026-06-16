#ifndef KS_NETWORK_SERVICE_H
#define KS_NETWORK_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "../../core/ks_app_types.h"

#define MAX_NETWORKS 20
#define IP_SIZE 16
#define WIFI_INTERFACE_NAME "WIFI"

extern wifi_network networks[MAX_NETWORKS];
extern int network_count;

// System APIs
void ks_network_manager_init(void);
void ks_network_manager_loop(void);

void ks_network_service_init_ui_state(void);
void ks_network_service_refresh(void);
void ks_network_service_process_ui_sync(void);
int ks_network_service_has_connectivity(void);
const char *ks_network_service_get_connection_type(void);

// Wi-Fi Control APIs
void wifi_connect(const char *ssid, const char *password);
int wifi_connect_async(const char *ssid, const char *password);
int wifi_disconnect(const char *interface);
void wifi_scr_init(void);
int wifi_scanning_ssid(void);

// Network state APIs
bool ks_network_is_connected(void);
bool ks_network_has_internet(void);
const char *ks_network_get_ip(void);
const char *ks_network_get_mac(void);

// Wi-Fi status APIs
const char* ks_network_get_wifi_ssid(void);
int ks_network_get_wifi_signal(void);
bool ks_network_is_scanning(void);

#ifdef __cplusplus
}
#endif

#endif // KS_NETWORK_SERVICE_H
