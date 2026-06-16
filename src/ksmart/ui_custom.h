#ifndef _UI_CUSTOM_H
#define _UI_CUSTOM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>

/*********************
 *      INCLUDES
 *********************/
#include "ui.h"
#include "app/ks_app_runtime.h"
#include "services/device/ks_device_service.h"
#include "services/gpio/ks_gpio_service.h"
#include "services/network/ks_network_service.h"
#include "services/relay/ks_relay_discovery_service.h"
#include "services/relay/ks_relay_service.h"

// LVGL 9 compatibility macros for screens
#define lv_obj_set_style_bg_img_opa lv_obj_set_style_bg_image_opa

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
