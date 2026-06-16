#ifndef KS_APP_RUNTIME_H
#define KS_APP_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>

extern lv_timer_t *ui_Timer;
extern int timer_count;

void custom_init(void);
void main_timer_init(void);
void main_timer_release(void);
void ks_app_runtime_schedule_all_relays(bool enabled);
void ks_app_runtime_schedule_all_relays_from_ui(bool enabled);
void ks_app_runtime_cancel_all_relays(void);
void ks_app_runtime_set_all_relay_delay_ms(int ms);
int ks_app_runtime_get_all_relay_delay_ms(void);
void ks_app_runtime_set_screen_sleep_ms(int ms);
int ks_app_runtime_get_screen_sleep_ms(void);
void ks_app_runtime_load_settings(void);
void ks_app_runtime_push_ui_sync_event(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
