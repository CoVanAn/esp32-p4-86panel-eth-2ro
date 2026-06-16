#include "../services/network/ks_network_service.h"
#include "../ui_custom.h"

#include <stdio.h>
#include <string.h>

static lv_obj_t *wpa_status_card;
static lv_obj_t *wpa_status_state_label;
static lv_obj_t *wpa_status_ssid_label;
static lv_obj_t *wpa_network_count_label;
static lv_obj_t *wpa_network_list_card;
static lv_obj_t *wpa_network_rows;
static lv_obj_t *wpa_modal_overlay;
static lv_obj_t *wpa_modal_card;
static lv_obj_t *wpa_modal_ssid_label;
static lv_obj_t *wpa_modal_toggle_btn;
static lv_obj_t *wpa_modal_toggle_label;

static int last_network_count = -1;

static int wpa_row_indices[MAX_NETWORKS];
static int wpa_selected_network_index = -1;
static bool wpa_password_visible = false;
static bool wpa_connected = false;
static bool wpa_connecting = false;
static int wpa_connecting_timeout = 0;
static int wpa_connected_signal = 0;
static char wpa_connected_ssid[MAX_CONF_LEN];
static lv_timer_t *wpa_poll_timer = NULL;

static lv_obj_t *wpa_status_icon_wifi = NULL;
static lv_obj_t *wpa_status_btn_discon = NULL;
static lv_obj_t *wpa_status_lbl_discon = NULL;

static void wpa_reset_cached_objects(void);
static void wpa_event_screen_delete(lv_event_t *e);
static void wpa_apply_screen_background(void);
static void wpa_create_header(void);
static void wpa_create_status_section(void);
static void wpa_create_network_section(void);
static void wpa_create_password_modal(void);
static void wpa_refresh_network_rows(void);
static void wpa_refresh_network_count(void);
static void wpa_refresh_status_card(void);
static void wpa_sync_connected_from_system(void);
static int wpa_signal_to_percent(int raw_signal);
static int wpa_network_is_open(const wifi_network *network);
static const char *wpa_network_security_text(const wifi_network *network);
static void wpa_show_password_modal(int network_index);
static void wpa_hide_password_modal(void);
static void wpa_connect_selected_network(const char *password);

static void wpa_event_scan(lv_event_t *e);
static void wpa_event_disconnect(lv_event_t *e);
static void wpa_event_network_row_click(lv_event_t *e);
static void wpa_event_modal_cancel(lv_event_t *e);
static void wpa_event_modal_toggle_password(lv_event_t *e);
static void wpa_event_modal_connect(lv_event_t *e);

static int wpa_signal_to_percent(int raw_signal) {
  if (raw_signal < 0) {
    int percent = (raw_signal + 100) * 2;
    if (percent < 0) {
      return 0;
    }
    if (percent > 100) {
      return 100;
    }
    return percent;
  }

  if (raw_signal > 100) {
    return 100;
  }
  if (raw_signal < 0) {
    return 0;
  }
  return raw_signal;
}

static int wpa_network_is_open(const wifi_network *network) {
  if (network == NULL) {
    return 0;
  }

  if (strstr(network->flags, "WEP") != NULL) {
    return 0;
  }
  if (strstr(network->flags, "WPA") != NULL) {
    return 0;
  }

  return 1;
}

static const char *wpa_network_security_text(const wifi_network *network) {
  if (network == NULL) {
    return "";
  }

  if (wpa_network_is_open(network)) {
    return "mo";
  }
  if (strstr(network->flags, "WPA3") != NULL) {
    return "wpa3";
  }
  if (strstr(network->flags, "WPA2") != NULL) {
    return "wpa2";
  }
  if (strstr(network->flags, "WPA") != NULL) {
    return "wpa";
  }
  if (strstr(network->flags, "WEP") != NULL) {
    return "wep";
  }

  return "bao mat";
}

static void wpa_reset_cached_objects(void) {
  if (wpa_poll_timer != NULL) {
    lv_timer_del(wpa_poll_timer);
    wpa_poll_timer = NULL;
  }

  ui_ScreenWpa = NULL;
  ui_PanelList = NULL;
  ui_DropdownSSID = NULL;
  ui_LabelWLAN = NULL;
  ui_PanelBtn = NULL;
  ui_ButtonDiscon = NULL;
  ui_ImageDiscon = NULL;
  ui_ButtonScan = NULL;
  ui_ImageScan = NULL;
  ui_ButtonBack = NULL;
  ui_ImageBack = NULL;
  ui_ButtonConnect = NULL;
  ui_ImageConnect = NULL;
  ui_LabelMGMT = NULL;
  ui_LabelPW = NULL;
  ui_TextAreaPW = NULL;
  ui_LabelRSSI = NULL;
  ui_LabelSSID = NULL;
  ui_TextAreaSSID = NULL;
  ui_TextAreaRSSI = NULL;
  ui_TextAreaMgnt = NULL;
  ui_Keyboard1 = NULL;

  wpa_status_card = NULL;
  wpa_status_state_label = NULL;
  wpa_status_ssid_label = NULL;
  wpa_network_count_label = NULL;
  wpa_network_list_card = NULL;
  wpa_network_rows = NULL;
  wpa_modal_overlay = NULL;
  wpa_modal_card = NULL;
  wpa_modal_ssid_label = NULL;
  wpa_modal_toggle_btn = NULL;
  wpa_modal_toggle_label = NULL;

  wpa_selected_network_index = -1;
  wpa_password_visible = false;
  wpa_connected = false;
  wpa_connecting = false;
  wpa_connecting_timeout = 0;
  wpa_connected_signal = 0;
  wpa_connected_ssid[0] = '\0';
  wpa_status_icon_wifi = NULL;
  wpa_status_btn_discon = NULL;
  wpa_status_lbl_discon = NULL;
}

static void wpa_event_screen_delete(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_DELETE ||
      lv_event_get_target(e) != ui_ScreenWpa) {
    return;
  }

  wpa_reset_cached_objects();
}

static void wpa_apply_screen_background(void) {
  lv_obj_set_style_radius(ui_ScreenWpa, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_ScreenWpa, lv_color_hex(0x0C1A3E),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_color(ui_ScreenWpa, lv_color_hex(0x040D1A),
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_dir(ui_ScreenWpa, LV_GRAD_DIR_VER,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ScreenWpa, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
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

static void wpa_create_header(void) {
  lv_obj_t *header;
  lv_obj_t *title;

  header = lv_obj_create(ui_ScreenWpa);
  lv_obj_set_size(header, 720, 72);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(header, lv_color_hex(0x0B1622),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(header, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(header, lv_color_hex(0x0EA5E9),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(header, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_ButtonBack = lv_btn_create(header);
  lv_obj_set_size(ui_ButtonBack, 124, 48);
  lv_obj_set_pos(ui_ButtonBack, 16, 12);
  lv_obj_clear_flag(ui_ButtonBack, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_ButtonBack, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_ButtonBack, lv_color_hex(0x123049),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ButtonBack, 190, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_ButtonBack, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_ButtonBack, lv_color_hex(0x2E84B8),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_ButtonBack, 255,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_ButtonBack, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_ImageBack = lv_label_create(ui_ButtonBack);
  lv_obj_center(ui_ImageBack);
  lv_label_set_text(ui_ImageBack, LV_SYMBOL_LEFT " Quay lại");
  lv_obj_set_style_text_color(ui_ImageBack, lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_ImageBack, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  title = lv_label_create(header);
  lv_obj_set_pos(title, 324, 20);
  lv_label_set_text(title, "Wi-Fi");
  lv_obj_set_style_text_color(title, lv_color_hex(0xE0F2FE),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_28,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_ButtonScan = lv_btn_create(header);
  lv_obj_set_size(ui_ButtonScan, 112, 44);
  lv_obj_set_pos(ui_ButtonScan, 594, 14);
  lv_obj_clear_flag(ui_ButtonScan, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_ButtonScan, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_ButtonScan, lv_color_hex(0x12384A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ButtonScan, 210, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_ButtonScan, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_ButtonScan, lv_color_hex(0x2DD4BF),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_ButtonScan, 255,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_ButtonScan, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_ImageScan = lv_label_create(ui_ButtonScan);
  lv_obj_center(ui_ImageScan);
  lv_label_set_text(ui_ImageScan, "Quét lại");
  lv_obj_set_style_text_color(ui_ImageScan, lv_color_hex(0x2DD4BF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_ImageScan, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_add_event_cb(ui_ButtonBack, ui_event_ButtonBack, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui_ButtonScan, wpa_event_scan, LV_EVENT_CLICKED, NULL);
}

static void wpa_create_status_section(void) {
  lv_obj_t *section_label;
  lv_obj_t *icon_wrap;

  wpa_status_card = lv_obj_create(ui_ScreenWpa);
  lv_obj_set_size(wpa_status_card, 660, 150);
  lv_obj_set_pos(wpa_status_card, 30, 84);
  lv_obj_clear_flag(wpa_status_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(wpa_status_card, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(wpa_status_card, lv_color_hex(0x10223A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(wpa_status_card, 214,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(wpa_status_card, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(wpa_status_card, lv_color_hex(0x244663),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(wpa_status_card, 255,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(wpa_status_card, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  icon_wrap = lv_obj_create(wpa_status_card);
  lv_obj_set_size(icon_wrap, 52, 52);
  lv_obj_clear_flag(icon_wrap, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(icon_wrap, 12, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_set_style_clip_corner(icon_wrap, true,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(icon_wrap, lv_color_hex(0x12384A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(icon_wrap, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(icon_wrap, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(icon_wrap, lv_color_hex(0x2DD4BF),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(icon_wrap, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_ImageWifi = lv_img_create(icon_wrap);
  lv_img_set_src(ui_ImageWifi, &ui_img_icon_wifi_on_png);
  lv_obj_center(ui_ImageWifi);
  wpa_status_icon_wifi = ui_ImageWifi;

  ui_LabelWLAN = lv_label_create(wpa_status_card);
  lv_obj_set_pos(ui_LabelWLAN, 60, 0);
  lv_label_set_text(ui_LabelWLAN, "Đang kết nối");
  lv_obj_set_style_text_color(ui_LabelWLAN, lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelWLAN, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  wpa_status_state_label = ui_LabelWLAN;

  wpa_status_ssid_label = lv_label_create(wpa_status_card);
  lv_obj_set_pos(wpa_status_ssid_label, 60, 28);
  lv_obj_set_width(wpa_status_ssid_label, 420);
  lv_label_set_long_mode(wpa_status_ssid_label, LV_LABEL_LONG_DOT);
  lv_label_set_text(wpa_status_ssid_label, "--");
  lv_obj_set_style_text_color(wpa_status_ssid_label, lv_color_hex(0xE0F2FE),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(wpa_status_ssid_label, &lv_font_montserrat_20,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_ButtonDiscon = lv_btn_create(wpa_status_card);
  lv_obj_set_size(ui_ButtonDiscon, 620, 42);
  lv_obj_set_pos(ui_ButtonDiscon, 0, 70);
  lv_obj_clear_flag(ui_ButtonDiscon, LV_OBJ_FLAG_SCROLLABLE);
  wpa_status_btn_discon = ui_ButtonDiscon;
  lv_obj_set_style_radius(ui_ButtonDiscon, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_ButtonDiscon, lv_color_hex(0x22334E),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ButtonDiscon, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_ButtonDiscon, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_ButtonDiscon, lv_color_hex(0xEF4444),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_ButtonDiscon, 180,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_ButtonDiscon, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_ImageDiscon = lv_label_create(ui_ButtonDiscon);
  lv_obj_center(ui_ImageDiscon);
  lv_label_set_text(ui_ImageDiscon, "Ngắt kết nối");
  wpa_status_lbl_discon = ui_ImageDiscon;
  lv_obj_set_style_text_color(ui_ImageDiscon, lv_color_hex(0xF87171),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_ImageDiscon, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_add_event_cb(ui_ButtonDiscon, wpa_event_disconnect, LV_EVENT_CLICKED,
                      NULL);
}

static void wpa_create_network_section(void) {
  lv_obj_t *section_label;

  section_label = lv_label_create(ui_ScreenWpa);
  lv_obj_set_pos(section_label, 30, 260);
  lv_label_set_text(section_label, "MẠNG KHẢ DỤNG");
  lv_obj_set_style_text_color(section_label, lv_color_hex(0x5FA8C8),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(section_label, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  wpa_network_count_label = lv_label_create(ui_ScreenWpa);
  lv_obj_set_pos(wpa_network_count_label, 610, 260);
  lv_label_set_text(wpa_network_count_label, "0 mạng");
  lv_obj_set_style_text_color(wpa_network_count_label, lv_color_hex(0x7DA7BC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(wpa_network_count_label, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  wpa_network_list_card = lv_obj_create(ui_ScreenWpa);
  lv_obj_set_size(wpa_network_list_card, 660, 398);
  lv_obj_set_pos(wpa_network_list_card, 30, 300);
  lv_obj_clear_flag(wpa_network_list_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(wpa_network_list_card, 18,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  // 🔥 FIX LỖI BO GÓC
  lv_obj_set_style_clip_corner(wpa_network_list_card, true,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(wpa_network_list_card, lv_color_hex(0x0D2236),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(wpa_network_list_card, 220,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(wpa_network_list_card, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(wpa_network_list_card, lv_color_hex(0x1E4A69),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(wpa_network_list_card, 160,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(wpa_network_list_card, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(wpa_network_list_card, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);

  wpa_network_rows = lv_obj_create(wpa_network_list_card);
  lv_obj_set_size(wpa_network_rows, 660, 398);
  lv_obj_set_pos(wpa_network_rows, 0, 0);
  lv_obj_set_scroll_dir(wpa_network_rows, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(wpa_network_rows, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_flex_flow(wpa_network_rows, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(wpa_network_rows, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_bg_opa(wpa_network_rows, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(wpa_network_rows, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(wpa_network_rows, 0,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(wpa_network_rows, 0,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(wpa_network_rows, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(wpa_network_rows, 0,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(wpa_network_rows, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_PanelList = wpa_network_list_card;
}

static void textarea_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = lv_event_get_target(e);

  if (code == LV_EVENT_FOCUSED) {
    lv_keyboard_set_textarea(ui_Keyboard1, ta);
    lv_obj_clear_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
  } else if (code == LV_EVENT_DEFOCUSED) {
    lv_obj_add_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
  }
}

static void keyboard_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *kb = lv_event_get_target(e);

  if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(kb, NULL);
  }
}

static void wpa_create_password_modal(void) {
  lv_obj_t *title;
  lv_obj_t *ssid_hint;
  lv_obj_t *pwd_label;
  lv_obj_t *cancel_btn;
  lv_obj_t *cancel_label;

  ui_Keyboard1 = lv_keyboard_create(ui_ScreenWpa);
  lv_obj_set_size(ui_Keyboard1, 720, 236);
  lv_obj_align(ui_Keyboard1, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(ui_Keyboard1, lv_color_hex(0x08131F),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_Keyboard1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_Keyboard1, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_Keyboard1, lv_color_hex(0x22384E),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_Keyboard1, 255,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(ui_Keyboard1, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_Keyboard1, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_keyboard_set_popovers(ui_Keyboard1, true);

  // Ẩn mặc định
  lv_obj_add_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);

  lv_obj_add_event_cb(ui_Keyboard1, keyboard_event_cb, LV_EVENT_ALL, NULL);

  wpa_modal_overlay = lv_obj_create(ui_ScreenWpa);
  lv_obj_set_size(wpa_modal_overlay, 720, 720);
  lv_obj_set_pos(wpa_modal_overlay, 0, 0);
  lv_obj_clear_flag(wpa_modal_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(wpa_modal_overlay, 0,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(wpa_modal_overlay, lv_color_hex(0x040A1A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(wpa_modal_overlay, 170,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(wpa_modal_overlay, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(wpa_modal_overlay, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  wpa_modal_card = lv_obj_create(wpa_modal_overlay);
  lv_obj_set_size(wpa_modal_card, 500, 250);
  lv_obj_center(wpa_modal_card);
  lv_obj_clear_flag(wpa_modal_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(wpa_modal_card, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(wpa_modal_card, lv_color_hex(0x0C1A3E),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_color(wpa_modal_card, lv_color_hex(0x062030),
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_dir(wpa_modal_card, LV_GRAD_DIR_VER,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(wpa_modal_card, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(wpa_modal_card, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(wpa_modal_card, lv_color_hex(0x2DD4BF),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(wpa_modal_card, 140,
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  title = lv_label_create(wpa_modal_card);
  lv_obj_t *icon = lv_label_create(wpa_modal_card);
  lv_obj_set_pos(icon, 0, 0);
  lv_label_set_text(icon, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_color(icon, lv_color_hex(0x7DA7BC), 0);
  lv_obj_set_style_text_font(icon, &lv_font_montserrat_18, 0);

  wpa_modal_ssid_label = lv_label_create(wpa_modal_card);
  lv_obj_set_pos(wpa_modal_ssid_label, 30, -3);
  lv_obj_set_width(wpa_modal_ssid_label, 452);
  lv_label_set_long_mode(wpa_modal_ssid_label, LV_LABEL_LONG_DOT);
  lv_label_set_text(wpa_modal_ssid_label, "");
  lv_obj_set_style_text_color(wpa_modal_ssid_label, lv_color_hex(0xE0F2FE),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(wpa_modal_ssid_label, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ssid_hint = lv_label_create(wpa_modal_card);
  lv_obj_set_pos(ssid_hint, 0, 40);
  lv_label_set_text(ssid_hint, "Mật khẩu");
  lv_obj_set_style_text_color(ssid_hint, lv_color_hex(0x7DA7BC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ssid_hint, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *pwd_container = lv_obj_create(wpa_modal_card);
  lv_obj_set_size(pwd_container, 452, 56);
  lv_obj_set_pos(pwd_container, 0, 75);
  lv_obj_set_style_radius(pwd_container, 12, 0);
  lv_obj_set_style_bg_color(pwd_container, lv_color_hex(0x08131F), 0);
  lv_obj_set_style_border_width(pwd_container, 1, 0);
  lv_obj_set_style_border_color(pwd_container, lv_color_hex(0x27506F), 0);
  lv_obj_clear_flag(pwd_container, LV_OBJ_FLAG_SCROLLABLE);

  ui_TextAreaPW = lv_textarea_create(pwd_container);
  lv_obj_set_size(ui_TextAreaPW, 452, 56);
  lv_obj_set_pos(ui_TextAreaPW, -10, -20);

  lv_textarea_set_one_line(ui_TextAreaPW, true);
  lv_textarea_set_password_mode(ui_TextAreaPW, true);

  lv_obj_set_style_bg_opa(ui_TextAreaPW, 0,
                          0); // trong suốt để dùng bg container
  lv_obj_set_style_border_width(ui_TextAreaPW, 0, 0);
  lv_obj_set_style_text_color(ui_TextAreaPW, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_add_event_cb(ui_TextAreaPW, textarea_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_set_style_text_font(ui_TextAreaPW, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // 👇 QUAN TRỌNG: chừa chỗ bên phải cho nút "Hiện"
  wpa_modal_toggle_btn = lv_btn_create(pwd_container);
  lv_obj_set_size(wpa_modal_toggle_btn, 60, 40);
  lv_obj_align(wpa_modal_toggle_btn, LV_ALIGN_RIGHT_MID, 10, 0);

  // ❌ bỏ hết background + border
  lv_obj_set_style_bg_opa(wpa_modal_toggle_btn, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(wpa_modal_toggle_btn, 0, 0);
  lv_obj_set_style_shadow_width(wpa_modal_toggle_btn, 0, 0);
  lv_obj_set_style_outline_width(wpa_modal_toggle_btn, 0, 0);

  // (optional) bỏ luôn padding nếu thấy lệch
  lv_obj_set_style_pad_all(wpa_modal_toggle_btn, 0, 0);

  wpa_modal_toggle_label = lv_label_create(wpa_modal_toggle_btn);
  lv_obj_center(wpa_modal_toggle_label);
  lv_label_set_text(wpa_modal_toggle_label, "Hiện");

  lv_obj_set_style_text_color(wpa_modal_toggle_label, lv_color_hex(0x7DD3FC),
                              0);
  lv_obj_set_style_text_font(wpa_modal_toggle_label, &lv_font_montserrat_18, 0);

  cancel_btn = lv_btn_create(wpa_modal_card);
  lv_obj_set_size(cancel_btn, 150, 46);
  lv_obj_set_pos(cancel_btn, 0, 150);
  lv_obj_clear_flag(cancel_btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(cancel_btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x17263A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(cancel_btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(cancel_btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(cancel_btn, lv_color_hex(0x334155),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  cancel_label = lv_label_create(cancel_btn);
  lv_obj_center(cancel_label);
  lv_label_set_text(cancel_label, "Hủy");
  lv_obj_set_style_text_color(cancel_label, lv_color_hex(0xCBD5E1),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(cancel_label, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_ButtonConnect = lv_btn_create(wpa_modal_card);
  lv_obj_set_size(ui_ButtonConnect, 200, 46);
  lv_obj_set_pos(ui_ButtonConnect, 254, 150);
  lv_obj_clear_flag(ui_ButtonConnect, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_ButtonConnect, 12,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_ButtonConnect, lv_color_hex(0x1E6B8F),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ButtonConnect, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_ButtonConnect, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_ButtonConnect, lv_color_hex(0x2DD4BF),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_ImageConnect = lv_label_create(ui_ButtonConnect);
  lv_obj_center(ui_ImageConnect);
  lv_label_set_text(ui_ImageConnect, "Kết nối");
  lv_obj_set_style_text_color(ui_ImageConnect, lv_color_hex(0xE6FFFA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_ImageConnect, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_add_event_cb(cancel_btn, wpa_event_modal_cancel, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_add_event_cb(wpa_modal_toggle_btn, wpa_event_modal_toggle_password,
                      LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_ButtonConnect, wpa_event_modal_connect,
                      LV_EVENT_CLICKED, NULL);

  lv_obj_add_flag(wpa_modal_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void wpa_refresh_status_card(void) {
  if (!ui_obj_is_ready(wpa_status_card) ||
      !ui_obj_is_ready(wpa_status_state_label) ||
      !ui_obj_is_ready(wpa_status_ssid_label) ||
      !ui_obj_is_ready(wpa_status_btn_discon) ||
      !ui_obj_is_ready(wpa_status_lbl_discon) ||
      !ui_obj_is_ready(wpa_status_icon_wifi)) {
    return;
  }

  if (wpa_connecting) {
    lv_label_set_text(wpa_status_state_label, "Đang kết nối...");
    lv_obj_set_style_text_font(wpa_status_state_label, &lv_font_montserrat_18,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(wpa_status_ssid_label, wpa_connected_ssid[0] != '\0'
                                                 ? wpa_connected_ssid
                                                 : "--");
    lv_obj_set_style_bg_color(wpa_status_card, lv_color_hex(0x10303F),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(wpa_status_card, lv_color_hex(0xF6AC05),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(wpa_status_card, 120,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_img_set_src(wpa_status_icon_wifi, &ui_img_icon_wifi_on_png);
    lv_obj_set_style_text_color(wpa_status_state_label, lv_color_hex(0xF6AC05),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_state(wpa_status_btn_discon, LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(wpa_status_btn_discon, 140,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(wpa_status_lbl_discon, 140,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  } else if (wpa_connected) {
    lv_label_set_text(wpa_status_state_label, "Đã kết nối");
    lv_obj_set_style_text_font(wpa_status_state_label, &lv_font_montserrat_18,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(wpa_status_ssid_label, wpa_connected_ssid[0] != '\0'
                                                 ? wpa_connected_ssid
                                                 : "--");
    lv_obj_set_style_bg_color(wpa_status_card, lv_color_hex(0x10303F),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(wpa_status_card, lv_color_hex(0x2DD4BF),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(wpa_status_card, 120,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_img_set_src(wpa_status_icon_wifi, &ui_img_icon_wifi_on_png);
    lv_obj_set_style_text_color(wpa_status_state_label, lv_color_hex(0x2DD4BF),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_state(wpa_status_btn_discon, LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(wpa_status_btn_discon, 255,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(wpa_status_lbl_discon, 255,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  } else {
    lv_label_set_text(wpa_status_state_label, "Chưa kết nối");
    lv_obj_set_style_text_font(wpa_status_state_label, &lv_font_montserrat_18,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(wpa_status_ssid_label, "--");
    lv_obj_set_style_bg_color(wpa_status_card, lv_color_hex(0x221A2A),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(wpa_status_card, lv_color_hex(0xEF4444),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(wpa_status_card, 80,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_img_set_src(wpa_status_icon_wifi, &ui_img_icon_wifi_off_png);
    lv_obj_set_style_text_color(wpa_status_state_label, lv_color_hex(0xF87171),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_state(wpa_status_btn_discon, LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(wpa_status_btn_discon, 140,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(wpa_status_lbl_discon, 140,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static void wpa_refresh_network_count(void) {
  char count_text[32];

  if (!ui_obj_is_ready(wpa_network_count_label)) {
    return;
  }

  snprintf(count_text, sizeof(count_text), "%d mạng", network_count);
  lv_obj_set_style_text_font(wpa_network_count_label, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(wpa_network_count_label, count_text);
}

static void wpa_refresh_network_rows(void) {
  int i;

  if (!ui_obj_is_ready(wpa_network_rows)) {
    return;
  }

  lv_obj_clean(wpa_network_rows);

  if (network_count <= 0) {
    lv_obj_t *empty_label = lv_label_create(wpa_network_rows);
    lv_obj_set_width(empty_label, lv_pct(100));
    lv_label_set_long_mode(empty_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(empty_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(empty_label, 150, 0);
    if (ks_network_is_scanning()) {
      lv_label_set_text(empty_label, "Đang quét mạng...");
    } else {
      lv_label_set_text(empty_label, "Không tìm thấy mạng. Bấm Quét lại.");
    }
    lv_obj_set_style_text_color(empty_label, lv_color_hex(0x7DA7BC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(empty_label, &lv_font_montserrat_18,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    return;
  }

  for (i = 0; i < network_count && i < MAX_NETWORKS; i++) {
    lv_obj_t *row;
    lv_obj_t *ssid_label;
    lv_obj_t *signal_label;
    lv_obj_t *security_chip;
    lv_obj_t *security_label;
    int signal_percent;
    char signal_text[24];
    const char *security_text;
    bool is_active;

    wpa_row_indices[i] = i;
    signal_percent = wpa_signal_to_percent(networks[i].signal_level);
    snprintf(signal_text, sizeof(signal_text), "%d%%", signal_percent);
    security_text = wpa_network_security_text(&networks[i]);
    is_active =
        wpa_connected && strcmp(wpa_connected_ssid, networks[i].ssid) == 0;

    row = lv_btn_create(wpa_network_rows);
    lv_obj_set_size(row, 660, 80);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(
        row, is_active ? lv_color_hex(0x10303F) : lv_color_hex(0x0D2236),
        LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(row, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(row, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(row, lv_color_hex(0x1E4A69),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(row, 120, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ssid_label = lv_label_create(row);
    lv_obj_set_pos(ssid_label, 18, 28);
    lv_obj_set_width(ssid_label, 320);
    lv_label_set_long_mode(ssid_label, LV_LABEL_LONG_DOT);
    lv_label_set_text(ssid_label, networks[i].ssid);
    lv_obj_set_style_text_color(
        ssid_label, is_active ? lv_color_hex(0xE0F2FE) : lv_color_hex(0xC7DBE8),
        LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_18,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

    signal_label = lv_label_create(row);
    lv_obj_set_pos(signal_label, 450, 28);
    lv_label_set_text(signal_label, signal_text);
    lv_obj_set_style_text_color(signal_label, lv_color_hex(0x7DD3FC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(signal_label, &lv_font_montserrat_16,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

    security_chip = lv_obj_create(row);
    lv_obj_set_size(security_chip, 88, 30);
    lv_obj_set_pos(security_chip, 540, 24);
    lv_obj_clear_flag(security_chip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(security_chip, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(security_chip,
                              strcmp(security_text, "mo") == 0
                                  ? lv_color_hex(0x3A151A)
                                  : lv_color_hex(0x123049),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(security_chip, 255,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(security_chip, 1,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(security_chip,
                                  strcmp(security_text, "mo") == 0
                                      ? lv_color_hex(0xEF4444)
                                      : lv_color_hex(0x2E84B8),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(security_chip, 160,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(security_chip, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    security_label = lv_label_create(security_chip);
    lv_obj_center(security_label);
    lv_label_set_text(security_label, security_text);
    lv_obj_set_style_text_color(security_label,
                                strcmp(security_text, "mo") == 0
                                    ? lv_color_hex(0xF87171)
                                    : lv_color_hex(0x7DD3FC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(security_label, &lv_font_montserrat_14,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(row, wpa_event_network_row_click, LV_EVENT_CLICKED,
                        &wpa_row_indices[i]);
  }
}

static void wpa_show_password_modal(int network_index) {
  if (!ui_obj_is_ready(wpa_modal_overlay) ||
      !ui_obj_is_ready(wpa_modal_ssid_label) ||
      !ui_obj_is_ready(ui_TextAreaPW) || network_index < 0 ||
      network_index >= network_count) {
    return;
  }

  wpa_selected_network_index = network_index;
  wpa_password_visible = false;
  lv_textarea_set_password_mode(ui_TextAreaPW, true);
  lv_label_set_text(wpa_modal_toggle_label, "Hiện");

  lv_label_set_text(wpa_modal_ssid_label, networks[network_index].ssid);
  lv_textarea_set_text(ui_TextAreaPW, "");

  lv_obj_add_state(ui_TextAreaPW, LV_STATE_FOCUSED);

  lv_obj_clear_flag(wpa_modal_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(wpa_modal_overlay);

  lv_obj_move_foreground(ui_Keyboard1);
}

static void wpa_hide_password_modal(void) {
  if (!ui_obj_is_ready(wpa_modal_overlay)) {
    return;
  }

  lv_obj_add_flag(wpa_modal_overlay, LV_OBJ_FLAG_HIDDEN);
  wpa_selected_network_index = -1;
}

static void wpa_connect_selected_network(const char *password) {
  const char *ssid;

  if (wpa_selected_network_index < 0 ||
      wpa_selected_network_index >= network_count) {
    return;
  }

  ssid = networks[wpa_selected_network_index].ssid;

#ifdef KS_DESKTOP_SIMULATOR
  (void)password;
#else
  /* Chạy nền để không khoá UI 7s do sleep/system() trong wifi_connect; worker
   * sẽ tự kick network refresh để snapshot ghi đè trạng thái lạc quan dưới đây
   * nếu kết nối thực tế thất bại. */
  (void)wifi_connect_async(ssid, password);
#endif

  wpa_connecting = true;
  wpa_connected = false;
  wpa_connecting_timeout = 20; // ~20 seconds timeout if polled every 2s
  wpa_connected_signal =
      wpa_signal_to_percent(networks[wpa_selected_network_index].signal_level);
  snprintf(wpa_connected_ssid, sizeof(wpa_connected_ssid), "%s", ssid);

  wpa_refresh_status_card();
  wpa_refresh_network_rows();
}

static void wpa_sync_connected_from_system(void) {
#ifdef KS_DESKTOP_SIMULATOR
  wpa_connected = true;
  wpa_connecting = false;
  wpa_connected_signal = 92;
  snprintf(wpa_connected_ssid, sizeof(wpa_connected_ssid), "KSmart-Office");
  return;
#endif

#ifdef ARDUINO
  const char *ssid = ks_network_get_wifi_ssid();
  if (ssid != NULL && ssid[0] != '\0') {
    wpa_connected = true;
    wpa_connecting = false;
    wpa_connected_signal = wpa_signal_to_percent(ks_network_get_wifi_signal());
    snprintf(wpa_connected_ssid, sizeof(wpa_connected_ssid), "%s", ssid);
  } else {
    wpa_connected = false;
    // Nếu không đang connecting hoặc hết timeout thì xoá ssid cũ
    if (!wpa_connecting) {
      wpa_connected_ssid[0] = '\0';
      wpa_connected_signal = 0;
    }
  }
  return;
#endif

  FILE *fp;
  char line[MAX_LINE_LEN];
  bool found = false;
  bool is_completed = false;
  char temp_ssid[MAX_CONF_LEN] = {0};

  fp = popen("wpa_cli -i " WIFI_INTERFACE_NAME " status", "r");
  if (fp == NULL) {
    return;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    if (strncmp(line, "wpa_state=COMPLETED", 19) == 0) {
      is_completed = true;
    }
    if (strncmp(line, "ssid=", 5) == 0) {
      line[strcspn(line, "\r\n")] = '\0';
      snprintf(temp_ssid, sizeof(temp_ssid), "%s", line + 5);
      found = temp_ssid[0] != '\0';
    }
  }

  (void)pclose(fp);

  if (found && is_completed) {
    wpa_connected = true;
    wpa_connecting = false;
    snprintf(wpa_connected_ssid, sizeof(wpa_connected_ssid), "%s", temp_ssid);
  } else {
    wpa_connected = false;
    // Nếu không đang connecting hoặc hết timeout thì xoá ssid cũ
    if (!wpa_connecting) {
      wpa_connected_ssid[0] = '\0';
      wpa_connected_signal = 0;
    }
  }
}

static void wpa_event_scan(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  wifi_scanning_ssid();
  wpa_refresh_network_count();
  wpa_refresh_network_rows();
}

static void wpa_event_disconnect(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  // Force reset state ngay lập tức để UI phản hồi tức thì
  wpa_connected = false;
  wpa_connecting = false;
  wpa_connected_ssid[0] = '\0';

#ifndef KS_DESKTOP_SIMULATOR
  (void)wifi_disconnect(WIFI_INTERFACE_NAME);
#endif

  wpa_refresh_status_card();
  wpa_refresh_network_rows();
}

static void wpa_event_network_row_click(lv_event_t *e) {
  int network_index;

  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  if (lv_event_get_user_data(e) == NULL) {
    return;
  }

  network_index = *((int *)lv_event_get_user_data(e));
  if (network_index < 0 || network_index >= network_count) {
    return;
  }

  if (wpa_network_is_open(&networks[network_index])) {
    wpa_selected_network_index = network_index;
    wpa_connect_selected_network("");
    wpa_hide_password_modal();
    return;
  }

  wpa_show_password_modal(network_index);
}

static void wpa_event_modal_cancel(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  wpa_hide_password_modal();
}

static void wpa_event_modal_toggle_password(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED ||
      !ui_obj_is_ready(ui_TextAreaPW) ||
      !ui_obj_is_ready(wpa_modal_toggle_label)) {
    return;
  }

  wpa_password_visible = !wpa_password_visible;
  lv_textarea_set_password_mode(ui_TextAreaPW, !wpa_password_visible);
  lv_label_set_text(wpa_modal_toggle_label,
                    wpa_password_visible ? "Ẩn" : "Hiện");
}

static void wpa_event_modal_connect(lv_event_t *e) {
  const char *password;
  size_t password_len;

  if (lv_event_get_code(e) != LV_EVENT_CLICKED ||
      !ui_obj_is_ready(ui_TextAreaPW)) {
    return;
  }

  password = lv_textarea_get_text(ui_TextAreaPW);
  if (password == NULL) {
    return;
  }

  /* WPA2-PSK yêu cầu passphrase 8..63 ký tự; nếu ngoài khoảng,
   * wpa_supplicant sẽ reject silently → giữ modal mở để user nhập lại. */
  password_len = strlen(password);
  if (password_len < 8 || password_len > 63) {
    return;
  }

  wpa_connect_selected_network(password);
  wpa_hide_password_modal();
}

static void wpa_poll_timer_cb(lv_timer_t *timer) {
  bool old_connected = wpa_connected;
  bool old_connecting = wpa_connecting;
  char old_ssid[MAX_CONF_LEN];

  snprintf(old_ssid, sizeof(old_ssid), "%s", wpa_connected_ssid);

  if (wpa_connecting) {
    wpa_connecting_timeout -= 2;
    if (wpa_connecting_timeout <= 0) {
      wpa_connecting = false;
    }
  }

  wpa_sync_connected_from_system();

  // Luôn force refresh card (chỉ đổi text/style nên rất nhẹ) để UI không kẹt
  wpa_refresh_status_card();

  if (old_connected != wpa_connected || old_connecting != wpa_connecting ||
      strcmp(old_ssid, wpa_connected_ssid) != 0 ||
      last_network_count != network_count) {
    last_network_count = network_count;
    wpa_refresh_network_count();
    wpa_refresh_network_rows();
  }
}

/* Màn kết nối Wi-Fi theo bố cục RelayBoxWifiConnectScreen.tsx (header + status
 * + list + modal mật khẩu). */
void ui_ScreenWpa_screen_init(void) {
  wpa_reset_cached_objects();
  last_network_count = -1;

  ui_ScreenWpa = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_ScreenWpa, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(ui_ScreenWpa, wpa_event_screen_delete, LV_EVENT_DELETE,
                      NULL);

  wpa_apply_screen_background();
  wpa_create_header();
  wpa_create_status_section();
  wpa_create_network_section();
  wpa_create_password_modal();

  wifi_scanning_ssid();
  wpa_sync_connected_from_system();
  wpa_refresh_network_count();
  wpa_refresh_status_card();
  wpa_refresh_network_rows();

  wpa_poll_timer = lv_timer_create(wpa_poll_timer_cb, 2000, NULL);
}
