#ifndef KS_DEVICE_SERVICE_H
#define KS_DEVICE_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

void system_get_device_model(char *model);
int ks_device_service_get_primary_mac(char *mac_address, size_t mac_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif