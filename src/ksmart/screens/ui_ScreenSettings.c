#include "../services/device/ks_device_service.h"
#include "../services/network/ks_network_service.h"
#include "../ui_custom.h"

#include <stdio.h>

static void settings_menu_reset_cached_objects(void);
static void settings_menu_load_screen(lv_obj_t **target,
                                      void (*target_init)(void),
                                      bool auto_delete_current);
static void settings_menu_event_screen_delete(lv_event_t *e);
static void settings_menu_event_open_network(lv_event_t *e);
static void settings_menu_event_open_channels(lv_event_t *e);
static lv_obj_t *settings_menu_create_info_card(lv_obj_t *parent, lv_coord_t y,
                                                const char *label,
                                                const char *value,
                                                lv_color_t value_color);

/* Màn settings mới không còn trạng thái lock card; hàm này giữ lại để tương
 * thích luồng điều hướng cũ. */
void ui_refresh_settings_screen_state(void) {
  if (!ui_screen_is_ready(ui_ScreenSettings)) {
    ui_ScreenSettings = NULL;
    settings_menu_reset_cached_objects();
  }
}

static void settings_menu_reset_cached_objects(void) {
  ui_ScreenSettings = NULL;
  ui_PanelSettingsHeader = NULL;
  ui_ButtonSettingsBack = NULL;
  ui_LabelSettingsBack = NULL;
  ui_LabelSettingsTitle = NULL;

  ui_PanelWifi = NULL;
  ui_ImageWifi = NULL;
  ui_LabelWifiName = NULL;
  ui_LabelWifiIP = NULL;
  ui_LabelWIP = NULL;

  ui_PanelEth = NULL;
  ui_ImageEth = NULL;
  ui_LabelEth = NULL;
  ui_LabelEthIP = NULL;
  ui_LabelNetIP = NULL;
}

/* Gom thao tác mở màn hình con về một chỗ để card Wi-Fi chỉ cần quan tâm target
 * cần chuyển tới. */
static void settings_menu_load_screen(lv_obj_t **target,
                                      void (*target_init)(void),
                                      bool auto_delete_current) {
  if (target == NULL || target_init == NULL) {
    return;
  }

  ui_ensure_screen_ready(target, target_init);
  if (!ui_screen_is_ready(*target)) {
    return;
  }

  lv_scr_load_anim(*target, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0,
                   auto_delete_current);
}

static void settings_menu_event_screen_delete(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_DELETE ||
      lv_event_get_target(e) != ui_ScreenSettings) {
    return;
  }

  settings_menu_reset_cached_objects();
}

/* Card Wi-Fi mở thẳng màn WPA để khớp flow RelayBoxSettingsScreen ->
 * RelayBoxWifiConnectScreen. */
static void settings_menu_event_open_network(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    settings_menu_load_screen(&ui_ScreenWpa, ui_ScreenWpa_screen_init, true);
  }
}

static void settings_menu_event_open_channels(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    settings_menu_load_screen(&ui_ScreenSettingsCh,
                              ui_ScreenSettingsCh_screen_init, true);
  }
}

void ui_event_SettingsAdv(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    settings_menu_load_screen(&ui_ScreenSettingsAdv,
                              ui_ScreenSettingsAdv_screen_init, true);
  }
}

static lv_obj_t *settings_menu_create_info_card(lv_obj_t *parent, lv_coord_t y,
                                                const char *label,
                                                const char *value,
                                                lv_color_t value_color) {
  lv_obj_t *card;
  lv_obj_t *label_obj;
  lv_obj_t *value_obj;

  card = lv_obj_create(parent);
  lv_obj_set_size(card, 656, 72);
  lv_obj_set_x(card, 32);
  lv_obj_set_y(card, y);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(card, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x0F1E30),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(card, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(card, lv_color_hex(0x2B445D),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(card, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  label_obj = lv_label_create(card);
  lv_obj_set_pos(label_obj, 24, 12);
  lv_label_set_text(label_obj, label);
  lv_obj_set_style_text_color(label_obj, lv_color_hex(0x8DB6CF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(label_obj, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  value_obj = lv_label_create(card);
  lv_obj_set_pos(value_obj, 24, 38);
  lv_obj_set_width(value_obj, 610);
  lv_label_set_long_mode(value_obj, LV_LABEL_LONG_DOT);
  lv_label_set_text(value_obj, value);
  lv_obj_set_style_text_color(value_obj, value_color,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(value_obj, &lv_font_montserrat_20,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  return value_obj;
}

static void create_main_background(void) {
  lv_obj_set_style_bg_color(ui_ScreenMain, lv_color_hex(0x0C1A3E),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_color(ui_ScreenMain, lv_color_hex(0x040D1A),
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_dir(ui_ScreenMain, LV_GRAD_DIR_VER,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ScreenMain, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/* Màn Cài đặt theo bố cục RelayBoxSettingsScreen.tsx: header + network cards +
 * device info. */
void ui_ScreenSettings_screen_init(void) {
  lv_obj_t *header_logo;
  lv_obj_t *chevron_label;
  lv_obj_t *info_version_value;
  lv_obj_t *info_device_value;
  char mac[24];
  char version_text[96];
  char device_text[128];

  lv_obj_t *ui_PanelCh;
  lv_obj_t *ui_LabelCh;
  lv_obj_t *ui_LabelChSub;
  lv_obj_t *ui_LabelChIcon;

  settings_menu_reset_cached_objects();

  ui_ScreenSettings = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_ScreenSettings, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_ScreenSettings, 0,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_ScreenSettings, lv_color_hex(0x0C1A3E),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_color(ui_ScreenSettings, lv_color_hex(0x040D1A),
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_dir(ui_ScreenSettings, LV_GRAD_DIR_VER,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ScreenSettings, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_ScreenSettings, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_ScreenSettings, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(ui_ScreenSettings, settings_menu_event_screen_delete,
                      LV_EVENT_DELETE, NULL);

  ui_PanelSettingsHeader = lv_obj_create(ui_ScreenSettings);
  lv_obj_set_size(ui_PanelSettingsHeader, 720, 72);
  lv_obj_clear_flag(ui_PanelSettingsHeader, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_PanelSettingsHeader, 0,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_PanelSettingsHeader, lv_color_hex(0x0B1622),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_PanelSettingsHeader, 220,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_PanelSettingsHeader, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_PanelSettingsHeader, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_ButtonSettingsBack = lv_btn_create(ui_PanelSettingsHeader);
  lv_obj_set_size(ui_ButtonSettingsBack, 124, 48);
  lv_obj_set_x(ui_ButtonSettingsBack, 16);
  lv_obj_set_y(ui_ButtonSettingsBack, 12);
  lv_obj_clear_flag(ui_ButtonSettingsBack, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_ButtonSettingsBack, 12,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_ButtonSettingsBack, lv_color_hex(0x123049),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ButtonSettingsBack, 190,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_ButtonSettingsBack, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_ButtonSettingsBack, lv_color_hex(0x2E84B8),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_ButtonSettingsBack, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_ButtonSettingsBack, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_LabelSettingsBack = lv_label_create(ui_ButtonSettingsBack);
  lv_obj_center(ui_LabelSettingsBack);
  lv_label_set_text(ui_LabelSettingsBack, LV_SYMBOL_LEFT " Quay lại");
  lv_obj_set_style_text_color(ui_LabelSettingsBack, lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelSettingsBack, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_LabelSettingsTitle = lv_label_create(ui_PanelSettingsHeader);
  lv_obj_set_x(ui_LabelSettingsTitle, 310);
  lv_obj_set_y(ui_LabelSettingsTitle, 20);
  lv_label_set_text(ui_LabelSettingsTitle, "Cài đặt");
  lv_obj_set_style_text_color(ui_LabelSettingsTitle, lv_color_hex(0xE0F2FE),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelSettingsTitle, &lv_font_montserrat_28,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  header_logo = lv_label_create(ui_PanelSettingsHeader);
  lv_obj_align(header_logo, LV_ALIGN_RIGHT_MID, -20, 0);
  lv_label_set_text(header_logo, "KSMART");
  lv_obj_set_style_text_color(header_logo, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(header_logo, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_PanelWifi = lv_btn_create(ui_ScreenSettings);
  lv_obj_set_size(ui_PanelWifi, 320, 104);
  lv_obj_set_x(ui_PanelWifi, 32);
  lv_obj_set_y(ui_PanelWifi, 96);
  lv_obj_clear_flag(ui_PanelWifi, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_PanelWifi, 26, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_PanelWifi, lv_color_hex(0x0F1722),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_PanelWifi, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_set_style_border_width(ui_PanelWifi, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_PanelWifi, lv_color_hex(0x22384E),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_PanelWifi, 255,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_PanelWifi, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_PanelWifi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(ui_PanelWifi, settings_menu_event_open_network,
                      LV_EVENT_ALL, NULL);

  {
    lv_obj_t *icon_bubble = lv_obj_create(ui_PanelWifi);
    lv_obj_set_size(icon_bubble, 60, 60);
    lv_obj_set_x(icon_bubble, 20);
    lv_obj_set_y(icon_bubble, 22);
    lv_obj_clear_flag(icon_bubble, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(icon_bubble, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(icon_bubble, true, LV_PART_MAIN);
    lv_obj_set_style_bg_color(icon_bubble, lv_color_hex(0x112131),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(icon_bubble, 1,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(icon_bubble, lv_color_hex(0x334155),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(icon_bubble, 0,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ImageWifi = lv_img_create(icon_bubble);
    lv_img_set_src(ui_ImageWifi, &ui_img_icon_wifi_off_png);
    lv_obj_center(ui_ImageWifi);
  }

  ui_LabelWifiName = lv_label_create(ui_PanelWifi);
  lv_obj_set_x(ui_LabelWifiName, 98);
  lv_obj_set_y(ui_LabelWifiName, 20);
  lv_label_set_text(ui_LabelWifiName, "Wi-Fi");
  lv_obj_set_style_text_color(ui_LabelWifiName, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelWifiName, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_LabelWifiIP = lv_label_create(ui_PanelWifi);
  lv_obj_set_x(ui_LabelWifiIP, 98);
  lv_obj_set_y(ui_LabelWifiIP, 56);

  lv_label_set_text(ui_LabelWifiIP, "No IP");
  lv_obj_set_style_text_color(ui_LabelWifiIP, lv_color_hex(0x94A3B8),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelWifiIP, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  chevron_label = lv_label_create(ui_PanelWifi);
  lv_obj_align(chevron_label, LV_ALIGN_RIGHT_MID, -24, 0);
  lv_label_set_text(chevron_label, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(chevron_label, lv_color_hex(0x64748B),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(chevron_label, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_PanelEth = lv_obj_create(ui_ScreenSettings);
  lv_obj_set_size(ui_PanelEth, 320, 104);
  lv_obj_set_x(ui_PanelEth, 368);
  lv_obj_set_y(ui_PanelEth, 96);
  lv_obj_clear_flag(ui_PanelEth, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_PanelEth, 26, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_PanelEth, lv_color_hex(0x0F1722),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_PanelEth, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_PanelEth, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_PanelEth, lv_color_hex(0x22384E),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_PanelEth, 255,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_PanelEth, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_PanelEth, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  {
    lv_obj_t *icon_bubble = lv_obj_create(ui_PanelEth);
    lv_obj_set_size(icon_bubble, 60, 60);
    lv_obj_set_x(icon_bubble, 20);
    lv_obj_set_y(icon_bubble, 22);
    lv_obj_clear_flag(icon_bubble, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(icon_bubble, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(icon_bubble, true, LV_PART_MAIN);
    lv_obj_set_style_bg_color(icon_bubble, lv_color_hex(0x112131),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(icon_bubble, 1,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(icon_bubble, lv_color_hex(0x334155),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(icon_bubble, 0,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ImageEth = lv_img_create(icon_bubble);
    lv_img_set_src(ui_ImageEth, &ui_img_icon_eth_off_png);
    lv_obj_center(ui_ImageEth);
  }

  ui_LabelEth = lv_label_create(ui_PanelEth);
  lv_obj_set_x(ui_LabelEth, 98);
  lv_obj_set_y(ui_LabelEth, 20);
  lv_label_set_text(ui_LabelEth, "Ethernet");
  lv_obj_set_style_text_color(ui_LabelEth, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelEth, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_LabelEthIP = lv_label_create(ui_PanelEth);
  lv_obj_set_x(ui_LabelEthIP, 98);
  lv_obj_set_y(ui_LabelEthIP, 56);
  lv_label_set_text(ui_LabelEthIP, "No IP");
  lv_obj_set_style_text_color(ui_LabelEthIP, lv_color_hex(0x94A3B8),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelEthIP, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Card Cài đặt CH (Đổi tên cổng) */
  ui_PanelCh = lv_btn_create(ui_ScreenSettings);
  lv_obj_set_size(ui_PanelCh, 656, 104);
  lv_obj_set_x(ui_PanelCh, 32);
  lv_obj_set_y(ui_PanelCh, 216);
  lv_obj_clear_flag(ui_PanelCh, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_PanelCh, 26, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_PanelCh, lv_color_hex(0x0F1722),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_PanelCh, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_PanelCh, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_PanelCh, lv_color_hex(0x22384E),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_PanelCh, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_PanelCh, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_PanelCh, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(ui_PanelCh, settings_menu_event_open_channels,
                      LV_EVENT_ALL, NULL);

  {
    lv_obj_t *icon_bubble = lv_obj_create(ui_PanelCh);
    lv_obj_set_size(icon_bubble, 60, 60);
    lv_obj_set_x(icon_bubble, 20);
    lv_obj_set_y(icon_bubble, 22);
    lv_obj_clear_flag(icon_bubble, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(icon_bubble, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(icon_bubble, true, LV_PART_MAIN);
    lv_obj_set_style_bg_color(icon_bubble, lv_color_hex(0x112131),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(icon_bubble, 1,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(icon_bubble, lv_color_hex(0x334155),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(icon_bubble, 0,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelChIcon = lv_label_create(icon_bubble);
    lv_label_set_text(ui_LabelChIcon, LV_SYMBOL_EDIT);
    lv_obj_center(ui_LabelChIcon);
    lv_obj_set_style_text_color(ui_LabelChIcon, lv_color_hex(0x3AAED8),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelChIcon, &lv_font_montserrat_24,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  ui_LabelCh = lv_label_create(ui_PanelCh);
  lv_obj_set_x(ui_LabelCh, 98);
  lv_obj_set_y(ui_LabelCh, 20);
  lv_label_set_text(ui_LabelCh, "Cài đặt kênh");
  lv_obj_set_style_text_color(ui_LabelCh, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelCh, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_LabelChSub = lv_label_create(ui_PanelCh);
  lv_obj_set_x(ui_LabelChSub, 98);
  lv_obj_set_y(ui_LabelChSub, 56);
  lv_label_set_text(ui_LabelChSub, "Đổi tên các cổng thiết bị");
  lv_obj_set_style_text_color(ui_LabelChSub, lv_color_hex(0x94A3B8),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelChSub, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  chevron_label = lv_label_create(ui_PanelCh);
  lv_obj_align(chevron_label, LV_ALIGN_RIGHT_MID, -24, 0);
  lv_label_set_text(chevron_label, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(chevron_label, lv_color_hex(0x64748B),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(chevron_label, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Card Cài đặt Nâng cao */
  extern void ui_event_SettingsAdv(lv_event_t *e);
  lv_obj_t *ui_PanelAdv = lv_btn_create(ui_ScreenSettings);
  lv_obj_set_size(ui_PanelAdv, 656, 104);
  lv_obj_set_x(ui_PanelAdv, 32);
  lv_obj_set_y(ui_PanelAdv, 336);
  lv_obj_clear_flag(ui_PanelAdv, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_PanelAdv, 26, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_PanelAdv, lv_color_hex(0x0F1722),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_PanelAdv, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_PanelAdv, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_PanelAdv, lv_color_hex(0x22384E),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_PanelAdv, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_PanelAdv, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_PanelAdv, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(ui_PanelAdv, ui_event_SettingsAdv, LV_EVENT_ALL, NULL);

  {
    lv_obj_t *icon_bubble = lv_obj_create(ui_PanelAdv);
    lv_obj_set_size(icon_bubble, 60, 60);
    lv_obj_set_x(icon_bubble, 20);
    lv_obj_set_y(icon_bubble, 22);
    lv_obj_clear_flag(icon_bubble, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(icon_bubble, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(icon_bubble, true, LV_PART_MAIN);
    lv_obj_set_style_bg_color(icon_bubble, lv_color_hex(0x112131),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(icon_bubble, 1,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(icon_bubble, lv_color_hex(0x334155),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(icon_bubble, 0,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *adv_icon = lv_label_create(icon_bubble);
    lv_label_set_text(adv_icon, LV_SYMBOL_SETTINGS);
    lv_obj_center(adv_icon);
    lv_obj_set_style_text_color(adv_icon, lv_color_hex(0xEAB308),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(adv_icon, &lv_font_montserrat_24,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  lv_obj_t *adv_label = lv_label_create(ui_PanelAdv);
  lv_obj_set_x(adv_label, 98);
  lv_obj_set_y(adv_label, 20);
  lv_label_set_text(adv_label, "Cài đặt nâng cao");
  lv_obj_set_style_text_color(adv_label, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(adv_label, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *adv_sub = lv_label_create(ui_PanelAdv);
  lv_obj_set_x(adv_sub, 98);
  lv_obj_set_y(adv_sub, 56);
  lv_label_set_text(adv_sub, "Đổi ID Modbus, Sleep, ...");
  lv_obj_set_style_text_color(adv_sub, lv_color_hex(0x94A3B8),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(adv_sub, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *chevron_adv = lv_label_create(ui_PanelAdv);
  lv_obj_align(chevron_adv, LV_ALIGN_RIGHT_MID, -24, 0);
  lv_label_set_text(chevron_adv, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(chevron_adv, lv_color_hex(0x64748B),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(chevron_adv, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  snprintf(version_text, sizeof(version_text), "%s", KS_APP_VERSION);
  info_version_value = settings_menu_create_info_card(
      ui_ScreenSettings, 456, "Phiên bản ứng dụng", version_text,
      lv_color_hex(0xFBBF24));

  if (ks_device_service_get_primary_mac(mac, sizeof(mac)) == 0) {
    snprintf(device_text, sizeof(device_text), "%s", mac);
  } else {
    snprintf(device_text, sizeof(device_text), "--");
  }

  info_device_value =
      settings_menu_create_info_card(ui_ScreenSettings, 540, "Mã thiết bị",
                                     device_text, lv_color_hex(0x7DD3FC));
  lv_obj_set_style_text_font(info_version_value, &lv_font_montserrat_20,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(info_device_value, &lv_font_montserrat_20,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Tránh service chạm nhầm các object của màn WPA khi đang ở Settings. */
  ui_LabelWLAN = NULL;
  ui_LabelWIP = NULL;
  ui_LabelNetIP = NULL;

  ks_network_service_init_ui_state();
  lv_obj_add_event_cb(ui_ButtonSettingsBack, ui_event_SettingsBack,
                      LV_EVENT_ALL, NULL);
}
