#ifndef _LCUKFOX_86PANEL_UI_H
#define _LCUKFOX_86PANEL_UI_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "core/ks_app_config.h"
#include "core/ks_app_types.h"
#include <lvgl.h>

    /**********************
     *  GLOBAL VARIABLES
     **********************/
    /* Model thiết bị */
    extern char device_model[32];

    /* Helper trả về text mặc định an toàn (không bao giờ NULL): tên là
     * "Relay N" (tên runtime override qua RELAY_NAME_*); nguồn suy ra từ chỉ số
     * (ALL/GPIO/RS485). */
    const char *ui_get_relay_default_title(int relay_index);
    const char *ui_get_relay_source(int relay_index);

    /*SCREEN: ui_ScreenMain*/
    extern lv_obj_t *ui_ScreenMain;
    extern lv_obj_t *ui_ImageHeaderNetwork;
    extern lv_obj_t *ui_PanelHeaderNetworkDot;
    extern lv_obj_t *ui_LabelHeaderNetworkStatus;
    extern lv_obj_t *ui_ButtonSettings;
    extern lv_obj_t *ui_LabelSettingsIcon;
    extern lv_obj_t *ui_LabelDate;
    extern lv_obj_t *ui_LabelTime;
    extern lv_obj_t *ui_PanelRelay0;
    extern lv_obj_t *ui_LabelRelay0;
    extern lv_obj_t *ui_PanelRelay1;
    extern lv_obj_t *ui_LabelRelay1;
    extern lv_obj_t *ui_LabelLogo;
    extern lv_obj_t *ui_LabelHeaderDeviceName;

    /*SCREEN: ui_ScreenSettings*/
    extern lv_obj_t *ui_ScreenSettings;
    extern lv_obj_t *ui_ScreenSettingsNetwork;
    extern lv_obj_t *ui_ScreenSettingsCh;
    extern lv_obj_t *ui_ScreenSettingsAdv;

    extern lv_obj_t *ui_PanelSettingsHeader;
    extern lv_obj_t *ui_ButtonSettingsBack;
    extern lv_obj_t *ui_LabelSettingsBack;
    extern lv_obj_t *ui_LabelSettingsTitle;
    extern lv_obj_t *ui_PanelWifi;
    extern lv_obj_t *ui_ImageWifi;
    extern lv_obj_t *ui_LabelWifiName;
    extern lv_obj_t *ui_LabelWifiIP;
    extern lv_obj_t *ui_LabelWIP;
    extern lv_obj_t *ui_PanelEth;
    extern lv_obj_t *ui_ImageEth;
    extern lv_obj_t *ui_LabelEth;
    extern lv_obj_t *ui_LabelEthIP;
    extern lv_obj_t *ui_LabelNetIP;

    /*SCREEN: ui_ScreenWpa*/
    extern lv_obj_t *ui_ScreenWpa;
    extern lv_obj_t *ui_PanelList;
    extern lv_obj_t *ui_DropdownSSID;
    extern lv_obj_t *ui_LabelWLAN;
    extern lv_obj_t *ui_PanelBtn;
    extern lv_obj_t *ui_ButtonDiscon;
    extern lv_obj_t *ui_ImageDiscon;
    extern lv_obj_t *ui_ButtonScan;
    extern lv_obj_t *ui_ImageScan;
    extern lv_obj_t *ui_ButtonBack;
    extern lv_obj_t *ui_ImageBack;
    extern lv_obj_t *ui_ButtonConnect;
    extern lv_obj_t *ui_ImageConnect;
    extern lv_obj_t *ui_LabelMGMT;
    extern lv_obj_t *ui_LabelPW;
    extern lv_obj_t *ui_TextAreaPW;
    extern lv_obj_t *ui_LabelRSSI;
    extern lv_obj_t *ui_LabelSSID;
    extern lv_obj_t *ui_TextAreaSSID;
    extern lv_obj_t *ui_TextAreaRSSI;
    extern lv_obj_t *ui_TextAreaMgnt;
    extern lv_obj_t *ui_Keyboard1;

    extern lv_obj_t *ui_initial_actions0;

    /**********************
     *  ASSETS
     **********************/
    /* Asset hình ảnh và font dùng cho các màn hình */
    LV_IMG_DECLARE(ui__temporary_image);
    LV_IMG_DECLARE(ui_img_luckfox_logo_png);
    LV_IMG_DECLARE(ui_img_ksmart_logo_png);
    LV_IMG_DECLARE(ui_img_icon_wifi_on_png);
    LV_IMG_DECLARE(ui_img_icon_eth_off_png);
    LV_IMG_DECLARE(ui_img_icon_top17_png);
    LV_IMG_DECLARE(ui_img_discon_png);
    LV_IMG_DECLARE(ui_img_icon_wifi_off_png);
    LV_IMG_DECLARE(ui_img_scan_png);
    LV_IMG_DECLARE(ui_img_back_png);
    LV_IMG_DECLARE(ui_img_icon_eth_on_png);
    LV_IMG_DECLARE(ui_img_connect_png);

    LV_FONT_DECLARE(ui_font_HarmonyOS200);
    LV_FONT_DECLARE(ui_font_HarmonyOS48);

    /**********************
     *  FUNCTIONS
     **********************/
    /* Khởi tạo toàn bộ UI */
    void ui_init(void);

    /*SCREEN: ui_ScreenMain*/
    /* Hàm dựng screen và callback của màn hình chính */
    void ui_ScreenMain_screen_init(void);
    void ui_event_ScreenMain(lv_event_t *e);
    void ui_event_ButtonSettings(lv_event_t *e);
    void ui_event_HeaderNetworkChip(lv_event_t *e);
    void ui_event_PanelRelayRs485(lv_event_t *e);
    void ui_update_relay_card_visual(int relay_index, bool enabled);
    void ui_apply_relay_state_change(int relay_index_1based, bool enabled);
    void ui_refresh_relay_card_states(void);
    void ui_refresh_relay_discovery_status(void);
    void ui_refresh_main_stats_row(void);
    bool ui_is_screen_locked(void);
    void ui_set_screen_locked(bool locked);
    const char *ui_get_relay_display_name(int relay_index);
    void ui_set_relay_display_name(int relay_index, const char *name);
    void ui_reset_relay_display_name(int relay_index);
    void ui_set_header_device_display_name(const char *name);

    /*SCREEN: ui_ScreenSettings*/
    /* Hàm dựng screen và callback của màn hình cài đặt */
    void ui_ScreenSettings_screen_init(void);
    bool ui_obj_is_ready(lv_obj_t *obj);
    bool ui_screen_is_ready(lv_obj_t *screen);
    void ui_ensure_screen_ready(lv_obj_t **screen, void (*screen_init)(void));
    void ui_refresh_settings_screen_state(void);
    void ui_ScreenSettingsNetwork_screen_init(void);
    void ui_ScreenSettingsCh_screen_init(void);
    void ui_ScreenSettingsAdv_screen_init(void);
    void ui_event_SettingsAdv(lv_event_t *e);

    void ui_set_settings_network_back_to_menu(bool enabled);
    void ui_event_ScreenSettingsNetwork(lv_event_t *e);
    void ui_event_SettingsBack(lv_event_t *e);
    void ui_event_PanelWifi(lv_event_t *e);

    /*SCREEN: ui_ScreenWpa*/
    /* Hàm dựng screen và callback của màn hình cấu hình Wi-Fi */
    void ui_ScreenWpa_screen_init(void);
    void ui_event_DropdownSSID(lv_event_t *e);
    void ui_event_ButtonDiscon(lv_event_t *e);
    void ui_event_ButtonScan(lv_event_t *e);
    void ui_event_ButtonBack(lv_event_t *e);
    void ui_event_TextAreaPW(lv_event_t *e);
    void ui_event_TextAreaSSID(lv_event_t *e);
    void ui_event_Keyboard1(lv_event_t *e);

    void ui_event_PanelRelay0(lv_event_t *e);
    void ui_event_PanelRelay1(lv_event_t *e);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
