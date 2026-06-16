#ifndef KS_HUB_SERVICE_H
#define KS_HUB_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void ks_hub_service_init(void);
void ks_hub_service_process_ui_sync(void);
/* Giữ lại service này chỉ để chuyển dữ liệu socket sang luồng UI một cách an toàn. */
void ks_hub_service_queue_relay_display_name(int channel, const char *name);
void ks_hub_service_queue_device_display_name(const char *name);
const char *ks_hub_service_get_last_error(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
