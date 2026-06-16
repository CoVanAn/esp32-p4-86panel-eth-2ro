#include "../services/relay/ks_relay_service.h"
#include "../ui_custom.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

extern int g_hide_device_id_0;

static lv_obj_t *settings_ch_list_container = NULL;
static lv_obj_t *edit_overlay = NULL;
static lv_obj_t *edit_dialog = NULL;
static lv_obj_t *edit_textarea = NULL;
static lv_obj_t *edit_keyboard = NULL;
// static lv_obj_t *edit_hint_label = NULL;

static int selected_relay_index = -1;
static int selected_ch_num = -1;
static lv_obj_t *ch_name_labels[KS_UI_RELAY_TOTAL_COUNT];

static void settings_ch_reset_cached_objects(void);
static void settings_ch_event_back(lv_event_t *e);
static void settings_ch_event_save(lv_event_t *e);
static void settings_ch_event_edit_click(lv_event_t *e);
static void settings_ch_event_keyboard(lv_event_t *e);
static void settings_ch_event_cancel_edit(lv_event_t *e);
static void settings_ch_event_apply_edit(lv_event_t *e);
static void settings_ch_event_screen_delete(lv_event_t *e);
static void settings_ch_normalize_name(const char *name, char *normalized,
                                       size_t size);

static void settings_ch_reset_cached_objects(void) {
  settings_ch_list_container = NULL;
  edit_overlay = NULL;
  edit_dialog = NULL;
  edit_textarea = NULL;
  edit_keyboard = NULL;
  //   edit_hint_label = NULL;
  selected_relay_index = -1;
  selected_ch_num = -1;
  memset(ch_name_labels, 0, sizeof(ch_name_labels));
}

static void settings_ch_event_screen_delete(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_DELETE) {
    settings_ch_reset_cached_objects();
  }
}

static void settings_ch_event_back(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    // Quay lại màn hình Cài đặt chính
    ui_ensure_screen_ready(&ui_ScreenSettings, ui_ScreenSettings_screen_init);
    if (ui_screen_is_ready(ui_ScreenSettings)) {
      lv_scr_load_anim(ui_ScreenSettings, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, true);
    }
  }
}

static void settings_ch_event_save(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    // Gọi service để ghi đè file relay_map.json
    if (ks_relay_service_save_to_json(RELAY_NAME_JSON_PATH) == 0) {
      fprintf(stderr, "[ui-ch] Đã lưu cài đặt đổi tên cổng thành công\n");
    } else {
      fprintf(stderr, "[ui-ch] Lỗi khi lưu cài đặt đổi tên cổng\n");
    }

    // Quay lại màn hình Cài đặt chính
    ui_ensure_screen_ready(&ui_ScreenSettings, ui_ScreenSettings_screen_init);
    if (ui_screen_is_ready(ui_ScreenSettings)) {
      lv_scr_load_anim(ui_ScreenSettings, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, true);
    }
  }
}

static void settings_ch_normalize_name(const char *name, char *normalized,
                                       size_t size) {
  if (name == NULL || size == 0) {
    return;
  }

  const char *start = name;
  while (*start != '\0' && isspace((unsigned char)*start)) {
    start++;
  }

  const char *end = name + strlen(name);
  while (end > start && isspace((unsigned char)*(end - 1))) {
    end--;
  }

  size_t copy_length = (size_t)(end - start);
  if (copy_length >= size) {
    copy_length = size - 1;
  }

  if (copy_length == 0) {
    normalized[0] = '\0';
  } else {
    memcpy(normalized, start, copy_length);
    normalized[copy_length] = '\0';
  }
}

static void settings_ch_event_edit_click(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  // Lấy thông tin relay index từ user_data
  intptr_t data = (intptr_t)lv_event_get_user_data(e);
  int r_idx = (int)(data & 0xFFFF);
  int ch_num = (int)((data >> 16) & 0xFFFF);

  selected_relay_index = r_idx;
  selected_ch_num = ch_num;

  if (edit_overlay != NULL && edit_textarea != NULL) {
    //   edit_hint_label != NULL) {
    // Hiển thị overlay sửa tên
    lv_obj_clear_flag(edit_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(edit_overlay);

    // Lấy tên hiện tại đổ vào textarea
    const char *curr_name = ui_get_relay_display_name(r_idx);
    lv_textarea_set_text(edit_textarea, curr_name ? curr_name : "");
    lv_textarea_set_cursor_pos(edit_textarea, LV_TEXTAREA_CURSOR_LAST);

    // Hiển thị bàn phím ảo
    if (edit_keyboard != NULL) {
      lv_keyboard_set_textarea(edit_keyboard, edit_textarea);
      lv_obj_clear_flag(edit_keyboard, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(edit_keyboard);
    }
  }
}

static void settings_ch_event_cancel_edit(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    if (edit_overlay != NULL) {
      lv_obj_add_flag(edit_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    if (edit_keyboard != NULL) {
      lv_obj_add_flag(edit_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void settings_ch_event_apply_edit(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED ||
      lv_event_get_code(e) == LV_EVENT_READY) {
    if (selected_relay_index >= 0 &&
        selected_relay_index < KS_UI_RELAY_TOTAL_COUNT) {
      char raw_name[MAX_RELAY_NAME_LEN];
      char clean_name[MAX_RELAY_NAME_LEN];

      snprintf(raw_name, sizeof(raw_name), "%s",
               lv_textarea_get_text(edit_textarea));
      settings_ch_normalize_name(raw_name, clean_name, sizeof(clean_name));

      // Nếu rỗng, fallback về mặc định "Relay X"
      if (clean_name[0] == '\0') {
        snprintf(clean_name, sizeof(clean_name), "Relay %d",
                 selected_relay_index);
      }

      // Lưu tạm vào RAM UI
      ui_set_relay_display_name(selected_relay_index, clean_name);

      // Cập nhật label hiển thị trên danh sách
      if (ch_name_labels[selected_relay_index] != NULL) {
        lv_label_set_text(ch_name_labels[selected_relay_index], clean_name);
      }
    }

    // Ẩn overlay và bàn phím
    if (edit_overlay != NULL) {
      lv_obj_add_flag(edit_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    if (edit_keyboard != NULL) {
      lv_obj_add_flag(edit_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void settings_ch_event_keyboard(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) {
    settings_ch_event_apply_edit(e);
  } else if (code == LV_EVENT_CANCEL) {
    settings_ch_event_cancel_edit(e);
  }
}

void ui_ScreenSettingsCh_screen_init(void) {
  lv_obj_t *ui_PanelChHeader;
  lv_obj_t *ui_ButtonChBack;
  lv_obj_t *ui_LabelChBack;
  lv_obj_t *ui_LabelChTitle;
  lv_obj_t *ui_ButtonChSave;
  lv_obj_t *ui_LabelChSave;

  settings_ch_reset_cached_objects();

  // Màn hình chính
  ui_ScreenSettingsCh = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_ScreenSettingsCh, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_ScreenSettingsCh, lv_color_hex(0x0C1A3E),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_color(ui_ScreenSettingsCh, lv_color_hex(0x040D1A),
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_dir(ui_ScreenSettingsCh, LV_GRAD_DIR_VER,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ScreenSettingsCh, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_ScreenSettingsCh, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_ScreenSettingsCh, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(ui_ScreenSettingsCh, settings_ch_event_screen_delete,
                      LV_EVENT_DELETE, NULL);

  // Header Panel
  ui_PanelChHeader = lv_obj_create(ui_ScreenSettingsCh);
  lv_obj_set_size(ui_PanelChHeader, 720, 72);
  lv_obj_clear_flag(ui_PanelChHeader, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_PanelChHeader, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_PanelChHeader, lv_color_hex(0x0B1622),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_PanelChHeader, 220,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_PanelChHeader, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_PanelChHeader, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);

  // Nút Quay lại bên trái
  ui_ButtonChBack = lv_btn_create(ui_PanelChHeader);
  lv_obj_set_size(ui_ButtonChBack, 124, 48);
  lv_obj_set_x(ui_ButtonChBack, 16);
  lv_obj_set_y(ui_ButtonChBack, 12);
  lv_obj_clear_flag(ui_ButtonChBack, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_ButtonChBack, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_ButtonChBack, lv_color_hex(0x123049),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ButtonChBack, 190,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_ButtonChBack, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_ButtonChBack, lv_color_hex(0x2E84B8),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_ButtonChBack, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_ButtonChBack, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(ui_ButtonChBack, settings_ch_event_back, LV_EVENT_CLICKED,
                      NULL);

  ui_LabelChBack = lv_label_create(ui_ButtonChBack);
  lv_obj_center(ui_LabelChBack);
  lv_label_set_text(ui_LabelChBack, LV_SYMBOL_LEFT " Quay lại");
  lv_obj_set_style_text_color(ui_LabelChBack, lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelChBack, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Tiêu đề ở giữa
  ui_LabelChTitle = lv_label_create(ui_PanelChHeader);
  lv_obj_set_x(ui_LabelChTitle, 260);
  lv_obj_set_y(ui_LabelChTitle, 20);
  lv_label_set_text(ui_LabelChTitle, "Đổi tên cổng");
  lv_obj_set_style_text_color(ui_LabelChTitle, lv_color_hex(0xE0F2FE),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelChTitle, &lv_font_montserrat_28,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Nút Lưu cài đặt bên phải
  ui_ButtonChSave = lv_btn_create(ui_PanelChHeader);
  lv_obj_set_size(ui_ButtonChSave, 134, 48);
  lv_obj_set_x(ui_ButtonChSave, 570);
  lv_obj_set_y(ui_ButtonChSave, 12);
  lv_obj_clear_flag(ui_ButtonChSave, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_ButtonChSave, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_ButtonChSave, lv_color_hex(0x16A34A),
                            LV_PART_MAIN | LV_STATE_DEFAULT); // Xanh lá
  lv_obj_set_style_bg_opa(ui_ButtonChSave, 220,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_ButtonChSave, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_ButtonChSave, lv_color_hex(0x4ADE80),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_ButtonChSave, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_ButtonChSave, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(ui_ButtonChSave, settings_ch_event_save, LV_EVENT_CLICKED,
                      NULL);

  ui_LabelChSave = lv_label_create(ui_ButtonChSave);
  lv_obj_center(ui_LabelChSave);
  lv_label_set_text(ui_LabelChSave, "Lưu cài đặt");
  lv_obj_set_style_text_color(ui_LabelChSave, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelChSave, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Container cuộn danh sách các CH
  settings_ch_list_container = lv_obj_create(ui_ScreenSettingsCh);
  lv_obj_set_size(settings_ch_list_container, 720, 638);
  lv_obj_set_pos(settings_ch_list_container, 0, 80);
  lv_obj_set_scroll_dir(settings_ch_list_container, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(settings_ch_list_container, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_bg_opa(settings_ch_list_container, 0,
                          LV_PART_MAIN | LV_STATE_DEFAULT); // Nền trong suốt
  lv_obj_set_style_border_width(settings_ch_list_container, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(settings_ch_list_container, 34,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(settings_ch_list_container, 34,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(settings_ch_list_container, 10,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(settings_ch_list_container, 30,
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  // Đẩy layout các cổng vào container
  lv_coord_t cur_y = 10;

  // 1. Cổng trên bo mạch (GPIO)
  if (g_hide_device_id_0 == 0) {
    lv_obj_t *title_gpio = lv_label_create(settings_ch_list_container);
    lv_obj_set_pos(title_gpio, 0, cur_y);
    lv_label_set_text(title_gpio, "Cổng trên bo mạch");
    lv_obj_set_style_text_color(title_gpio, lv_color_hex(0x7DD3FC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title_gpio, &lv_font_montserrat_18,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

    cur_y += 34;

    // Vẽ 2 card GPIO trên 1 dòng
    for (int i = 0; i < 2; i++) {
      int r_idx = i + 1;  // 1-based index cho ui display name
      int ch_num = i + 1; // Đánh số CH1, CH2 cho device 0

      lv_obj_t *card = lv_obj_create(settings_ch_list_container);
      lv_obj_set_size(card, 316, 72);
      lv_obj_set_pos(card, (i == 0) ? 0 : 336, cur_y);
      lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_radius(card, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(card, lv_color_hex(0x0F1E30),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(card, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_color(card, lv_color_hex(0x2B445D),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_opa(card, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

      // Click vào card cũng mở popup sửa
      intptr_t user_data = (intptr_t)r_idx | ((intptr_t)ch_num << 16);
      lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(card, settings_ch_event_edit_click, LV_EVENT_CLICKED,
                          (void *)user_data);

      // Label CH
      lv_obj_t *lbl_ch = lv_label_create(card);
      lv_obj_set_pos(lbl_ch, 16, 12);
      lv_label_set_text_fmt(lbl_ch, "CH%d", ch_num);
      lv_obj_set_style_text_color(lbl_ch, lv_color_hex(0x7DD3FC),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_font(lbl_ch, &lv_font_montserrat_16,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);

      // Label Tên relay
      const char *curr_name = ui_get_relay_display_name(r_idx);
      ch_name_labels[r_idx] = lv_label_create(card);
      lv_obj_set_pos(ch_name_labels[r_idx], 16, 38);
      lv_obj_set_width(ch_name_labels[r_idx], 220);
      lv_label_set_long_mode(ch_name_labels[r_idx], LV_LABEL_LONG_DOT);
      lv_label_set_text(ch_name_labels[r_idx], curr_name ? curr_name : "");
      lv_obj_set_style_text_color(ch_name_labels[r_idx], lv_color_hex(0xF8FAFC),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_font(ch_name_labels[r_idx], &lv_font_montserrat_18,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);

      // Nút bút chì bên phải card
      lv_obj_t *btn_edit = lv_btn_create(card);
      lv_obj_set_size(btn_edit, 40, 40);
      lv_obj_align(btn_edit, LV_ALIGN_RIGHT_MID, -12, 0);
      lv_obj_clear_flag(btn_edit, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_radius(btn_edit, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(btn_edit, lv_color_hex(0x112131),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(btn_edit, 1,
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_color(btn_edit, lv_color_hex(0x22384E),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_width(btn_edit, 0,
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(btn_edit, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_add_event_cb(btn_edit, settings_ch_event_edit_click,
                          LV_EVENT_CLICKED, (void *)user_data);

      lv_obj_t *lbl_edit = lv_label_create(btn_edit);
      lv_label_set_text(lbl_edit, LV_SYMBOL_EDIT);
      lv_obj_center(lbl_edit);
      lv_obj_set_style_text_color(lbl_edit, lv_color_hex(0x3AAED8),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_font(lbl_edit, &lv_font_montserrat_18,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    cur_y += 72 + 20;
  }

  // 2. Cổng mở rộng (RS485)
  int board_count = ks_relay_service_get_board_count();
  for (int b = 0; b < board_count; b++) {
    uint8_t slave_id = 0;
    int relay_count = 0;
    int rs485_offset = 0;

    if (ks_relay_service_get_board_info(b, &slave_id, &relay_count,
                                        &rs485_offset) != 0) {
      continue;
    }

    // Tiêu đề của board
    lv_obj_t *title_rs485 = lv_label_create(settings_ch_list_container);
    lv_obj_set_pos(title_rs485, 0, cur_y);
    lv_label_set_text_fmt(title_rs485, "Cổng mở rộng (ID %d)", slave_id);
    lv_obj_set_style_text_color(title_rs485, lv_color_hex(0x7DD3FC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title_rs485, &lv_font_montserrat_18,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

    cur_y += 34;

    // Tính offset thực tế trong mảng display name của UI
    int board_offset = KS_GPIO_RELAY_COUNT + rs485_offset;

    // Vẽ các CH thuộc board này (2 card trên 1 dòng)
    for (int r = 0; r < relay_count; r++) {
      int r_idx = board_offset + r + 1; // 1-based UI relay index
      int ch_num = r + 1;               // Cổng của board này: CH1, CH2, CH3...
      int col = r % 2;
      int row = r / 2;

      lv_obj_t *card = lv_obj_create(settings_ch_list_container);
      lv_obj_set_size(card, 316, 72);
      lv_obj_set_pos(card, (col == 0) ? 0 : 336, cur_y + (row * 84));
      lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_radius(card, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(card, lv_color_hex(0x0F1E30),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(card, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_color(card, lv_color_hex(0x2B445D),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_opa(card, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

      // Click vào card
      intptr_t user_data = (intptr_t)r_idx | ((intptr_t)ch_num << 16);
      lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(card, settings_ch_event_edit_click, LV_EVENT_CLICKED,
                          (void *)user_data);

      // Label CH
      lv_obj_t *lbl_ch = lv_label_create(card);
      lv_obj_set_pos(lbl_ch, 16, 12);
      lv_label_set_text_fmt(lbl_ch, "CH%d", ch_num);
      lv_obj_set_style_text_color(lbl_ch, lv_color_hex(0x7DD3FC),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_font(lbl_ch, &lv_font_montserrat_16,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);

      // Label Tên relay
      const char *curr_name = ui_get_relay_display_name(r_idx);
      ch_name_labels[r_idx] = lv_label_create(card);
      lv_obj_set_pos(ch_name_labels[r_idx], 16, 38);
      lv_obj_set_width(ch_name_labels[r_idx], 220);
      lv_label_set_long_mode(ch_name_labels[r_idx], LV_LABEL_LONG_DOT);
      lv_label_set_text(ch_name_labels[r_idx], curr_name ? curr_name : "");
      lv_obj_set_style_text_color(ch_name_labels[r_idx], lv_color_hex(0xF8FAFC),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_font(ch_name_labels[r_idx], &lv_font_montserrat_18,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);

      // Nút bút chì bên phải card
      lv_obj_t *btn_edit = lv_btn_create(card);
      lv_obj_set_size(btn_edit, 40, 40);
      lv_obj_align(btn_edit, LV_ALIGN_RIGHT_MID, -12, 0);
      lv_obj_clear_flag(btn_edit, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_radius(btn_edit, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(btn_edit, lv_color_hex(0x112131),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_width(btn_edit, 1,
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_color(btn_edit, lv_color_hex(0x22384E),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_width(btn_edit, 0,
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_pad_all(btn_edit, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_add_event_cb(btn_edit, settings_ch_event_edit_click,
                          LV_EVENT_CLICKED, (void *)user_data);

      lv_obj_t *lbl_edit = lv_label_create(btn_edit);
      lv_label_set_text(lbl_edit, LV_SYMBOL_EDIT);
      lv_obj_center(lbl_edit);
      lv_obj_set_style_text_color(lbl_edit, lv_color_hex(0x3AAED8),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_font(lbl_edit, &lv_font_montserrat_18,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    int rows = (relay_count + 1) / 2;
    cur_y += (rows * 84) + 20;
  }

  // 3. Đăng ký overlay sửa đổi tên (ẩn mặc định)
  edit_overlay = lv_obj_create(ui_ScreenSettingsCh);
  lv_obj_set_size(edit_overlay, 720, 720);
  lv_obj_set_pos(edit_overlay, 0, 0);
  lv_obj_clear_flag(edit_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(edit_overlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(edit_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(edit_overlay, lv_color_hex(0x020617),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(edit_overlay, 168, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(edit_overlay, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(edit_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(edit_overlay, LV_OBJ_FLAG_HIDDEN);
  // Click ra ngoài overlay cũng tắt popup
  lv_obj_add_event_cb(edit_overlay, settings_ch_event_cancel_edit,
                      LV_EVENT_CLICKED, NULL);

  // Dialog Box sửa đổi
  edit_dialog = lv_obj_create(edit_overlay);
  lv_obj_set_size(edit_dialog, 500, 260);
  lv_obj_center(edit_dialog);
  lv_obj_clear_flag(edit_dialog, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(edit_dialog,
                    LV_OBJ_FLAG_CLICKABLE); // Tránh click vào dialog bị trôi sự
                                            // kiện ra overlay
  lv_obj_set_style_radius(edit_dialog, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(edit_dialog, lv_color_hex(0x0C1724),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(edit_dialog, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(edit_dialog, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(edit_dialog, lv_color_hex(0x22384E),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(edit_dialog, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Tiêu đề dialog
  lv_obj_t *dlg_title = lv_label_create(edit_dialog);
  lv_obj_set_x(dlg_title, 20);
  lv_obj_set_y(dlg_title, 20);
  lv_label_set_text(dlg_title, "Đổi tên thiết bị");
  lv_obj_set_style_text_color(dlg_title, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(dlg_title, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Hint label
  //   edit_hint_label = lv_label_create(edit_dialog);
  //   lv_obj_set_x(edit_hint_label, 20);
  //   lv_obj_set_y(edit_hint_label, 52);
  //   lv_label_set_text(edit_hint_label, "Nhập tên mới cho cổng:");
  //   lv_obj_set_style_text_color(edit_hint_label, lv_color_hex(0x8BA7BD),
  //                               LV_PART_MAIN | LV_STATE_DEFAULT);
  //   lv_obj_set_style_text_font(edit_hint_label, &lv_font_montserrat_18,
  //                              LV_PART_MAIN | LV_STATE_DEFAULT);

  // TextArea để nhập text
  edit_textarea = lv_textarea_create(edit_dialog);
  lv_obj_set_size(edit_textarea, 460, 52);
  lv_obj_set_x(edit_textarea, 20);
  lv_obj_set_y(edit_textarea, 90);
  lv_textarea_set_one_line(edit_textarea, true);
  lv_textarea_set_max_length(edit_textarea, MAX_RELAY_NAME_LEN - 1);
  lv_obj_set_style_radius(edit_textarea, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(edit_textarea, lv_color_hex(0x09111B),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(edit_textarea, lv_color_hex(0x22384E),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(edit_textarea, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(edit_textarea, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Nút Hủy
  lv_obj_t *btn_cancel = lv_btn_create(edit_dialog);
  lv_obj_set_size(btn_cancel, 140, 48);
  lv_obj_set_x(btn_cancel, 20);
  lv_obj_set_y(btn_cancel, 180);
  lv_obj_set_style_radius(btn_cancel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x1E293B),
                            LV_PART_MAIN | LV_STATE_DEFAULT); // Xám tối
  lv_obj_set_style_shadow_width(btn_cancel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_cancel, settings_ch_event_cancel_edit,
                      LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
  lv_label_set_text(lbl_cancel, "Hủy");
  lv_obj_center(lbl_cancel);
  lv_obj_set_style_text_color(lbl_cancel, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(lbl_cancel, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Nút Áp dụng
  lv_obj_t *btn_apply = lv_btn_create(edit_dialog);
  lv_obj_set_size(btn_apply, 140, 48);
  lv_obj_set_x(btn_apply, 340);
  lv_obj_set_y(btn_apply, 180);
  lv_obj_set_style_radius(btn_apply, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_apply, lv_color_hex(0x1E6B8F),
                            LV_PART_MAIN | LV_STATE_DEFAULT); // Xanh dương
  lv_obj_set_style_border_width(btn_apply, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(btn_apply, lv_color_hex(0x3AAED8),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn_apply, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_apply, settings_ch_event_apply_edit, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_t *lbl_apply = lv_label_create(btn_apply);
  lv_label_set_text(lbl_apply, "Áp dụng");
  lv_obj_center(lbl_apply);
  lv_obj_set_style_text_color(lbl_apply, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(lbl_apply, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Bàn phím ảo (Lazy creation)
  edit_keyboard = lv_keyboard_create(ui_ScreenSettingsCh);
  lv_obj_set_size(edit_keyboard, 720, 236);
  lv_obj_align(edit_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(edit_keyboard, lv_color_hex(0x08131F),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(edit_keyboard, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(edit_keyboard, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(edit_keyboard, lv_color_hex(0x22384E),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(edit_keyboard, 255,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(edit_keyboard, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(edit_keyboard, &lv_font_montserrat_24,
                             LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_keyboard_set_popovers(edit_keyboard, true);
  lv_obj_add_flag(edit_keyboard, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(edit_keyboard, settings_ch_event_keyboard, LV_EVENT_ALL,
                      NULL);
}
