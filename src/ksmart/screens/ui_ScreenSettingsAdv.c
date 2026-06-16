#include "../ui_custom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lv_obj_t *ui_ScreenSettingsAdv = NULL;
static lv_obj_t *ui_btn_sleep = NULL;
static lv_obj_t *ui_btn_delay = NULL;
static lv_obj_t *ui_btn_recover = NULL;
static lv_obj_t *ui_btn_used_channels = NULL;

static uint8_t active_slave_ids[32];
static int active_slave_id_count = 0;

static void adv_settings_event_back(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    ui_ensure_screen_ready(&ui_ScreenSettings, ui_ScreenSettings_screen_init);
    lv_scr_load_anim(ui_ScreenSettings, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, true);
  }
}

static void btn_cancel_cb(lv_event_t *e) {
  lv_obj_t *overlay = (lv_obj_t *)lv_event_get_user_data(e);
  lv_obj_del(overlay);
}

static void btn_reboot_cb(lv_event_t *e) { ks_device_reboot(); }

typedef struct {
  lv_obj_t *popup;
  lv_obj_t *device_container;
  lv_obj_t *numpad_container;
  lv_obj_t *lbl_status;
  lv_obj_t *lbl_title;
  lv_obj_t *btn_cancel;
  uint8_t selected_old_id;
  int typed_id;
} change_id_ctx_t;

static void update_title_with_typed_id(change_id_ctx_t *ctx) {
  char title_buf[128];
  if (ctx->typed_id > 0) {
    snprintf(title_buf, sizeof(title_buf), "Mạch %d -> ID Mới: %d",
             ctx->selected_old_id, ctx->typed_id);
  } else {
    snprintf(title_buf, sizeof(title_buf), "Mạch %d -> ID Mới: _",
             ctx->selected_old_id);
  }
  lv_label_set_text(ctx->lbl_title, title_buf);
}

static void device_btn_cb(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target(e);
  change_id_ctx_t *ctx = (change_id_ctx_t *)lv_event_get_user_data(e);

  uint32_t idx = lv_obj_get_index(btn);
  if (idx < active_slave_id_count) {
    ctx->selected_old_id = active_slave_ids[idx];
    ctx->typed_id = 0;

    update_title_with_typed_id(ctx);

    lv_obj_add_flag(ctx->device_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ctx->btn_cancel,
                    LV_OBJ_FLAG_HIDDEN); // Ẩn nút Hủy/Đóng ở dưới cùng
    lv_obj_clear_flag(ctx->numpad_container, LV_OBJ_FLAG_HIDDEN);

    // Đặt vị trí thông báo lỗi ở dưới bàn phím khi đang nhập
    lv_obj_set_pos(ctx->lbl_status, 30, 430);
    lv_label_set_text(ctx->lbl_status, "");
  }
}

static void numpad_btn_cb(lv_event_t *e) {
  change_id_ctx_t *ctx = (change_id_ctx_t *)lv_event_get_user_data(e);
  lv_obj_t *btn = lv_event_get_target(e);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  const char *txt = lv_label_get_text(label);

  if (strcmp(txt, "Hủy") == 0) {
    // Quay lại chọn mạch
    lv_obj_add_flag(ctx->numpad_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ctx->device_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ctx->btn_cancel,
                      LV_OBJ_FLAG_HIDDEN); // Hiện lại nút Hủy/Đóng
    lv_label_set_text(ctx->lbl_title, "Đổi ID Mạch RS485");

    // Đặt vị trí thông báo về lại chỗ cũ
    lv_obj_set_pos(ctx->lbl_status, 30, 350);
    lv_label_set_text(ctx->lbl_status, "");
    return;
  }

  if (strcmp(txt, "Lưu") == 0) {
    if (ctx->typed_id <= 0 || ctx->typed_id > 247) {
      lv_label_set_text(ctx->lbl_status, "Lỗi: ID phải từ 1 đến 247!");
      lv_obj_set_style_text_color(ctx->lbl_status, lv_color_hex(0xEF4444),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
      return;
    }

    int new_id_val = ctx->typed_id;
    for (int i = 0; i < active_slave_id_count; i++) {
      if (active_slave_ids[i] == new_id_val &&
          active_slave_ids[i] != ctx->selected_old_id) {
        lv_label_set_text(ctx->lbl_status, "Lỗi: ID này đã tồn tại!");
        lv_obj_set_style_text_color(ctx->lbl_status, lv_color_hex(0xEF4444),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
      }
    }

    lv_label_set_text(ctx->lbl_status,
                      "Đang đổi ID... Vui lòng không tắt nguồn.");
    lv_obj_set_style_text_color(ctx->lbl_status, lv_color_hex(0x3B82F6),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_refr_now(NULL);

    int res =
        ks_relay_service_change_slave_id(ctx->selected_old_id, new_id_val);
    if (res == 0) {
      lv_obj_add_flag(ctx->numpad_container, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ctx->btn_cancel, LV_OBJ_FLAG_HIDDEN);

      // Khi đổi ID thành công, căn giữa thông báo thành công và nút Reboot
      lv_obj_set_pos(ctx->lbl_status, 30, 180);
      lv_label_set_text(
          ctx->lbl_status,
          "Đổi ID thành công!\nVui lòng nhấp Reboot để khởi động lại.");
      lv_obj_set_style_text_color(ctx->lbl_status, lv_color_hex(0x10B981),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);

      lv_obj_t *btn_reboot = lv_btn_create(ctx->popup);
      lv_obj_set_size(btn_reboot, 460, 48);
      lv_obj_set_pos(btn_reboot, 30, 320);
      lv_obj_set_style_radius(btn_reboot, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(btn_reboot, lv_color_hex(0x10B981),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_add_event_cb(btn_reboot, btn_reboot_cb, LV_EVENT_CLICKED, NULL);

      lv_obj_t *lbl_reboot = lv_label_create(btn_reboot);
      lv_obj_center(lbl_reboot);
      lv_label_set_text(lbl_reboot, "Khởi động lại (Reboot)");
      lv_obj_set_style_text_font(lbl_reboot, &lv_font_montserrat_18,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_color(lbl_reboot, lv_color_hex(0xFFFFFF),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
      char err_msg[128];
      snprintf(err_msg, sizeof(err_msg), "Lỗi: %s",
               ks_relay_service_get_last_error());
      lv_label_set_text(ctx->lbl_status, err_msg);
      lv_obj_set_style_text_color(ctx->lbl_status, lv_color_hex(0xEF4444),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    return;
  }

  // It's a number
  int digit = atoi(txt);
  int new_val = ctx->typed_id * 10 + digit;
  if (new_val <= 247) {
    ctx->typed_id = new_val;
    update_title_with_typed_id(ctx);
  }
}

static void free_ctx_cb(lv_event_t *e) {
  change_id_ctx_t *ctx = (change_id_ctx_t *)lv_event_get_user_data(e);
  free(ctx);
}

static void open_change_id_modal(lv_obj_t *parent) {
  int boards = ks_relay_service_get_board_count();
  if (boards <= 0) {
    lv_obj_t *overlay = lv_obj_create(parent);
    lv_obj_set_size(overlay, 720, 720);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(overlay, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *popup = lv_obj_create(overlay);
    lv_obj_set_size(popup, 500, 240);
    lv_obj_center(popup);
    lv_obj_set_style_radius(popup, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(popup, lv_color_hex(0x0F172A),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(popup, lv_color_hex(0x334155),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(popup, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(popup);
    lv_label_set_text(title, "Thông báo");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *desc = lv_label_create(popup);
    lv_label_set_text(desc, "Không tìm thấy thiết bị RS485 nào!");
    lv_obj_align(desc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_18,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(desc, lv_color_hex(0xEF4444),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *btn_close = lv_btn_create(popup);
    lv_obj_set_size(btn_close, 180, 44);
    lv_obj_align(btn_close, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_radius(btn_close, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x334155),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_close, btn_cancel_cb, LV_EVENT_CLICKED, overlay);

    lv_obj_t *lbl_close = lv_label_create(btn_close);
    lv_obj_center(lbl_close);
    lv_label_set_text(lbl_close, "Đóng");
    return;
  }

  active_slave_id_count = 0;
  for (int i = 0; i < boards; i++) {
    uint8_t slave_id = 0;
    int relay_count = 0;
    if (ks_relay_service_get_board_info(i, &slave_id, &relay_count, NULL) ==
        0) {
      active_slave_ids[active_slave_id_count++] = slave_id;
    }
  }

  lv_obj_t *overlay = lv_obj_create(parent);
  lv_obj_set_size(overlay, 720, 720);
  lv_obj_set_pos(overlay, 0, 0);
  lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(overlay, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

  change_id_ctx_t *ctx = (change_id_ctx_t *)malloc(sizeof(change_id_ctx_t));
  memset(ctx, 0, sizeof(change_id_ctx_t));
  lv_obj_add_event_cb(overlay, free_ctx_cb, LV_EVENT_DELETE, ctx);

  lv_obj_t *popup = lv_obj_create(overlay);
  lv_obj_set_size(popup, 520, 500);
  lv_obj_center(popup);
  lv_obj_set_style_radius(popup, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(popup, lv_color_hex(0x0F172A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(popup, lv_color_hex(0x334155),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(popup, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(popup, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);
  ctx->popup = popup;

  lv_obj_t *lbl_title = lv_label_create(popup);
  lv_label_set_text(lbl_title, "Đổi ID Mạch RS485");
  lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  ctx->lbl_title = lbl_title;

  lv_obj_t *device_container = lv_obj_create(popup);
  lv_obj_set_size(device_container, 460, 260);
  lv_obj_set_pos(device_container, 30, 90);
  lv_obj_set_style_bg_opa(device_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(device_container, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(device_container, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_flex_flow(device_container, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(device_container, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(device_container, 10,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(device_container, 10,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  ctx->device_container = device_container;

  for (int i = 0; i < active_slave_id_count; i++) {
    lv_obj_t *btn = lv_btn_create(device_container);
    lv_obj_set_size(btn, 140, 100);
    lv_obj_set_style_radius(btn, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E293B),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn, device_btn_cb, LV_EVENT_CLICKED, ctx);

    lv_obj_t *icon = lv_label_create(btn);
    lv_label_set_text(icon, LV_SYMBOL_DRIVE);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(icon, lv_color_hex(0x60A5FA),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text_fmt(lbl, "ID: %d", active_slave_ids[i]);
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF8FAFC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  lv_obj_t *numpad_container = lv_obj_create(popup);
  lv_obj_set_size(numpad_container, 460, 270);
  // Căn giữa bàn phím theo cả chiều dọc và ngang trong popup
  lv_obj_align(numpad_container, LV_ALIGN_CENTER, 0, 35);
  lv_obj_set_style_bg_opa(numpad_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(numpad_container, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(numpad_container, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(numpad_container, 8,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(numpad_container, 8,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_layout(numpad_container, LV_LAYOUT_GRID);
  static lv_coord_t col_dsc[] = {148, 148, 148, LV_GRID_TEMPLATE_LAST};
  static lv_coord_t row_dsc[] = {58, 58, 58, 58, LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(numpad_container, col_dsc, row_dsc);
  lv_obj_add_flag(numpad_container, LV_OBJ_FLAG_HIDDEN);
  ctx->numpad_container = numpad_container;

  const char *btn_map[] = {"1", "2", "3", "4",   "5", "6",
                           "7", "8", "9", "Hủy", "0", "Lưu"};

  for (int i = 0; i < 12; i++) {
    int r = i / 3;
    int c = i % 3;

    lv_obj_t *btn = lv_btn_create(numpad_container);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, c, 1,
                         LV_GRID_ALIGN_STRETCH, r, 1);
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (i == 9) { // Hủy
      lv_obj_set_style_bg_color(btn, lv_color_hex(0xEF4444),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    } else if (i == 11) { // Lưu
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x10B981),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x334155),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_add_event_cb(btn, numpad_btn_cb, LV_EVENT_CLICKED, ctx);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, btn_map[i]);
    lv_obj_center(lbl);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF8FAFC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  lv_obj_t *lbl_status = lv_label_create(popup);
  lv_label_set_text(lbl_status, "");
  lv_obj_set_pos(lbl_status, 30, 350);
  lv_obj_set_width(lbl_status, 460);
  lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(lbl_status, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  ctx->lbl_status = lbl_status;

  lv_obj_t *btn_cancel = lv_btn_create(popup);
  lv_obj_set_size(btn_cancel, 400, 48);
  lv_obj_set_pos(btn_cancel, 60, 420);
  lv_obj_set_style_radius(btn_cancel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x334155),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_cancel, btn_cancel_cb, LV_EVENT_CLICKED, overlay);
  ctx->btn_cancel = btn_cancel;

  lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
  lv_obj_center(lbl_cancel);
  lv_label_set_text(lbl_cancel, "Đóng / Hủy");
  lv_obj_set_style_text_font(lbl_cancel, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_cancel, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void adv_settings_event_change_id(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    open_change_id_modal(ui_ScreenSettingsAdv);
  }
}

static void btn_confirm_recover_cb(lv_event_t *e) {
  lv_obj_t *overlay = (lv_obj_t *)lv_event_get_user_data(e);
  lv_obj_del(overlay);

  if (ui_btn_recover) {
    lv_obj_t *lbl = lv_obj_get_child(ui_btn_recover, 2);
    if (lbl) {
      lv_label_set_text(lbl, "Đang quét các tốc độ...");
      lv_refr_now(NULL);
    }

    int result = ks_relay_service_recover_rs485();

    if (result > 0) {
      int recovered_id = result & 0xFF;
      int recovered_baud = result >> 8;
      if (lbl) {
        lv_label_set_text_fmt(lbl,
                              "Đã khôi phục ID: %d (%d bps). Vui lòng Reboot!",
                              recovered_id, recovered_baud);
      }
    } else {
      if (lbl) {
        lv_label_set_text(lbl, "Không tìm thấy mạch RS485 nào bị kẹt.");
      }
    }
  }
}

static void open_recover_confirm_modal(lv_obj_t *parent) {
  lv_obj_t *overlay = lv_obj_create(parent);
  lv_obj_set_size(overlay, 720, 720);
  lv_obj_set_pos(overlay, 0, 0);
  lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(overlay, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *popup = lv_obj_create(overlay);
  lv_obj_set_size(popup, 520, 360);
  lv_obj_center(popup);
  lv_obj_set_style_radius(popup, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(popup, lv_color_hex(0x0F172A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(popup, lv_color_hex(0x334155),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(popup, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(popup);
  lv_label_set_text(title, "Xác nhận Phục hồi RS485");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *desc = lv_label_create(popup);
  lv_obj_set_width(desc, 460);
  lv_label_set_text(
      desc, "Sử dụng khi mạch rơ-le bị mất kết nối (sai ID hoặc sai tốc độ "
            "Baudrate).\n\nHệ thống sẽ quét mọi tốc độ truyền, tự động đưa "
            "mạch về ID: 1 và Baudrate: 9600 để khôi phục kết nối.");
  lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 70);
  lv_obj_set_style_text_font(desc, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(desc, lv_color_hex(0x94A3B8),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  // Nút Hủy
  lv_obj_t *btn_cancel = lv_btn_create(popup);
  lv_obj_set_size(btn_cancel, 160, 48);
  lv_obj_set_pos(btn_cancel, 50, 250);
  lv_obj_set_style_radius(btn_cancel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x334155),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_cancel, btn_cancel_cb, LV_EVENT_CLICKED, overlay);

  lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
  lv_obj_center(lbl_cancel);
  lv_label_set_text(lbl_cancel, "Hủy");
  lv_obj_set_style_text_font(lbl_cancel, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_cancel, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  // Nút Xác nhận
  lv_obj_t *btn_confirm = lv_btn_create(popup);
  lv_obj_set_size(btn_confirm, 160, 48);
  lv_obj_set_pos(btn_confirm, 275, 250);
  lv_obj_set_style_radius(btn_confirm, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_confirm, lv_color_hex(0x3B82F6),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_confirm, btn_confirm_recover_cb, LV_EVENT_CLICKED,
                      overlay);

  lv_obj_t *lbl_confirm = lv_label_create(btn_confirm);
  lv_obj_center(lbl_confirm);
  lv_label_set_text(lbl_confirm, "Xác nhận");
  lv_obj_set_style_text_font(lbl_confirm, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_confirm, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void open_recover_error_modal(lv_obj_t *parent, int board_count) {
  lv_obj_t *overlay = lv_obj_create(parent);
  lv_obj_set_size(overlay, 720, 720);
  lv_obj_set_pos(overlay, 0, 0);
  lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(overlay, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *popup = lv_obj_create(overlay);
  lv_obj_set_size(popup, 520, 360);
  lv_obj_center(popup);
  lv_obj_set_style_radius(popup, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(popup, lv_color_hex(0x0F172A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(popup, lv_color_hex(0x334155),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(popup, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(popup);
  lv_label_set_text(title, "Không thể phục hồi");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(title, lv_color_hex(0xEF4444),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *desc = lv_label_create(popup);
  lv_obj_set_width(desc, 460);
  lv_label_set_text_fmt(
      desc,
      "Chức năng này chỉ khả dụng khi có duy nhất 1 mạch RS485 được kết "
      "nối.\n\n"
      "Hiện tại hệ thống đang phát hiện %d mạch. Vui lòng ngắt kết nối các "
      "thiết bị đang hoạt động, chỉ giữ lại DUY NHẤT 1 mạch cần phục hồi "
      "và thử lại.",
      board_count);
  lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 70);
  lv_obj_set_style_text_font(desc, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(desc, lv_color_hex(0x94A3B8),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  // Nút Đóng
  lv_obj_t *btn_close = lv_btn_create(popup);
  lv_obj_set_size(btn_close, 160, 48);
  lv_obj_align(btn_close, LV_ALIGN_BOTTOM_MID, 0, -30);
  lv_obj_set_style_radius(btn_close, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_close, lv_color_hex(0x334155),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_close, btn_cancel_cb, LV_EVENT_CLICKED, overlay);

  lv_obj_t *lbl_close = lv_label_create(btn_close);
  lv_obj_center(lbl_close);
  lv_label_set_text(lbl_close, "Đóng");
  lv_obj_set_style_text_font(lbl_close, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_close, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void adv_settings_event_recover_rs485(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    int board_count = ks_relay_service_get_board_count();
    if (board_count > 1) {
      open_recover_error_modal(ui_ScreenSettingsAdv, board_count);
    } else {
      open_recover_confirm_modal(ui_ScreenSettingsAdv);
    }
  }
}

static void update_adv_settings_subtitles(void) {
  if (ui_btn_sleep) {
    int sleep_sec = ks_app_runtime_get_screen_sleep_ms() / 1000;
    lv_obj_t *lbl = lv_obj_get_child(ui_btn_sleep, 2);
    if (lbl) {
      if (sleep_sec == 0) {
        lv_label_set_text(lbl, "Thời gian chờ tắt: Không bao giờ");
      } else {
        int mins = sleep_sec / 60;
        int secs = sleep_sec % 60;
        if (secs == 0) {
          lv_label_set_text_fmt(lbl, "Thời gian chờ tắt: %d phút", mins);
        } else if (mins == 0) {
          lv_label_set_text_fmt(lbl, "Thời gian chờ tắt: %d giây", secs);
        } else {
          lv_label_set_text_fmt(lbl, "Thời gian chờ tắt: %dp %dg", mins, secs);
        }
      }
    }
  }

  if (ui_btn_delay) {
    int delay_ms = ks_app_runtime_get_all_relay_delay_ms();
    lv_obj_t *lbl = lv_obj_get_child(ui_btn_delay, 2);
    if (lbl) {
      if (delay_ms >= 1000) {
        float secs = (float)delay_ms / 1000.0f;
        if ((int)(secs * 10) % 10 == 0) {
          lv_label_set_text_fmt(lbl, "Độ trễ kích hoạt: %d giây", (int)secs);
        } else {
          lv_label_set_text_fmt(lbl, "Độ trễ kích hoạt: %.1f giây", secs);
        }
      } else {
        lv_label_set_text_fmt(lbl, "Độ trễ kích hoạt: %d ms", delay_ms);
      }
    }
  }

  if (ui_btn_used_channels) {
    extern int g_used_channels_count;
    lv_obj_t *lbl = lv_obj_get_child(ui_btn_used_channels, 2);
    if (lbl) {
      int physical_count = ks_relay_service_get_physical_count();
      if (g_used_channels_count <= 0) {
        lv_label_set_text_fmt(lbl, "Số cổng hiển thị: Tất cả (%d/%d)", physical_count, physical_count);
      } else {
        int display_val = g_used_channels_count;
        if (display_val > physical_count) {
          display_val = physical_count;
        }
        lv_label_set_text_fmt(lbl, "Số cổng hiển thị: %d/%d", display_val, physical_count);
      }
    }
  }
}

typedef struct {
  lv_obj_t *overlay;
  void (*callback)(int value);
  int values[12];
} options_modal_ctx_t;

static void free_options_ctx_cb(lv_event_t *e) {
  options_modal_ctx_t *c = (options_modal_ctx_t *)lv_event_get_user_data(e);
  free(c);
}

static void option_btn_cb(lv_event_t *e) {
  options_modal_ctx_t *ctx = (options_modal_ctx_t *)lv_event_get_user_data(e);
  lv_obj_t *btn = lv_event_get_target(e);
  uint32_t idx = lv_obj_get_index(btn);

  int val = ctx->values[idx];
  ctx->callback(val);

  lv_obj_del(ctx->overlay);
}

static void open_options_modal(lv_obj_t *parent, const char *title,
                               const char *btn_labels[], const int values[],
                               int count, int current_value,
                               void (*callback)(int)) {
  lv_obj_t *overlay = lv_obj_create(parent);
  lv_obj_set_size(overlay, 720, 720);
  lv_obj_set_pos(overlay, 0, 0);
  lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(overlay, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

  options_modal_ctx_t *ctx =
      (options_modal_ctx_t *)malloc(sizeof(options_modal_ctx_t));
  ctx->overlay = overlay;
  ctx->callback = callback;
  for (int i = 0; i < count && i < 12; i++) {
    ctx->values[i] = values[i];
  }

  lv_obj_add_event_cb(overlay, free_options_ctx_cb, LV_EVENT_DELETE, ctx);

  lv_obj_t *popup = lv_obj_create(overlay);
  lv_obj_set_size(popup, 520, 480);
  lv_obj_center(popup);
  lv_obj_set_style_radius(popup, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(popup, lv_color_hex(0x0F172A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(popup, lv_color_hex(0x334155),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(popup, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(popup, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl_title = lv_label_create(popup);
  lv_label_set_text(lbl_title, title);
  lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 25);
  lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  // Grid container
  lv_obj_t *grid = lv_obj_create(popup);
  lv_obj_set_size(grid, 460, 260);
  lv_obj_align(grid, LV_ALIGN_CENTER, 0, -5);
  lv_obj_set_style_bg_opa(grid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(grid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(grid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(grid, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(grid, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_layout(grid, LV_LAYOUT_GRID);

  static lv_coord_t col_dsc[] = {224, 224, LV_GRID_TEMPLATE_LAST};
  static lv_coord_t row_dsc_3[] = {70, 70, 70, LV_GRID_TEMPLATE_LAST};
  static lv_coord_t row_dsc_4[] = {55, 55, 55, 55, LV_GRID_TEMPLATE_LAST};

  if (count <= 6) {
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc_3);
  } else {
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc_4);
  }

  for (int i = 0; i < count; i++) {
    int r = i / 2;
    int c = i % 2;
    int col_span = 1;

    // If it's the last item and count is odd, span 2 columns
    if (i == count - 1 && (count % 2) != 0) {
      c = 0;
      col_span = 2;
    }

    lv_obj_t *btn = lv_btn_create(grid);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, c, col_span,
                         LV_GRID_ALIGN_STRETCH, r, 1);
    lv_obj_set_style_radius(btn, 16, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (values[i] == current_value) {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x2563EB),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E293B),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_add_event_cb(btn, option_btn_cb, LV_EVENT_CLICKED, ctx);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, btn_labels[i]);
    lv_obj_center(lbl);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF8FAFC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  // Cancel button at bottom
  lv_obj_t *btn_cancel = lv_btn_create(popup);
  lv_obj_set_size(btn_cancel, 460, 48);
  lv_obj_set_pos(btn_cancel, 30, 400);
  lv_obj_set_style_radius(btn_cancel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x334155),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_cancel, btn_cancel_cb, LV_EVENT_CLICKED, overlay);

  lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
  lv_obj_center(lbl_cancel);
  lv_label_set_text(lbl_cancel, "Đóng / Hủy");
  lv_obj_set_style_text_font(lbl_cancel, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_cancel, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void on_sleep_selected(int val) {
  ks_app_runtime_set_screen_sleep_ms(val * 1000);
  ks_relay_service_save_to_json("/relay_map.json");
  update_adv_settings_subtitles();
}

static void on_delay_selected(int val) {
  ks_app_runtime_set_all_relay_delay_ms(val);
  ks_relay_service_save_to_json("/relay_map.json");
  update_adv_settings_subtitles();
}

static void adv_settings_event_sleep(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    const char *labels[] = {"30 giây", "1 phút", "2 phút", "3 phút",
                            "4 phút",  "5 phút", "10 phút"};
    int values[] = {30, 60, 120, 180, 240, 300, 600};
    int current = ks_app_runtime_get_screen_sleep_ms() / 1000;
    open_options_modal(ui_ScreenSettingsAdv, "Thời gian tắt màn hình", labels,
                       values, 7, current, on_sleep_selected);
  }
}

static void adv_settings_event_delay(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    const char *labels[] = {"0.1 giây", "0.3 giây", "0.5 giây",
                            "1.0 giây", "2.0 giây", "5.0 giây"};
    int values[] = {100, 300, 500, 1000, 2000, 5000};
    int current = ks_app_runtime_get_all_relay_delay_ms();
    open_options_modal(ui_ScreenSettingsAdv, "Độ trễ Bật/Tắt tất cả", labels,
                       values, 6, current, on_delay_selected);
  }
}

typedef struct {
  lv_obj_t *overlay;
  lv_obj_t *lbl_title;
  lv_obj_t *lbl_status;
  int typed_val;
  int max_val;
} set_ch_ctx_t;

static void free_set_ch_ctx_cb(lv_event_t *e) {
  set_ch_ctx_t *ctx = (set_ch_ctx_t *)lv_event_get_user_data(e);
  free(ctx);
}

static void update_set_ch_title(set_ch_ctx_t *ctx) {
  char title_buf[128];
  if (ctx->typed_val > 0) {
    snprintf(title_buf, sizeof(title_buf), "Nhập số cổng: %d", ctx->typed_val);
  } else if (ctx->typed_val == 0) {
    snprintf(title_buf, sizeof(title_buf), "Nhập số cổng: Tất cả (0)");
  } else {
    snprintf(title_buf, sizeof(title_buf), "Nhập số cổng: _");
  }
  lv_label_set_text(ctx->lbl_title, title_buf);
}

static void set_ch_numpad_btn_cb(lv_event_t *e) {
  set_ch_ctx_t *ctx = (set_ch_ctx_t *)lv_event_get_user_data(e);
  lv_obj_t *btn = lv_event_get_target(e);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  const char *txt = lv_label_get_text(label);

  if (strcmp(txt, "Hủy") == 0) {
    lv_obj_del(ctx->overlay);
    return;
  }

  if (strcmp(txt, "Lưu") == 0) {
    if (ctx->typed_val > ctx->max_val && ctx->typed_val != 0) {
      char err_msg[128];
      snprintf(err_msg, sizeof(err_msg), "Lỗi: Số cổng max là %d!", ctx->max_val);
      lv_label_set_text(ctx->lbl_status, err_msg);
      lv_obj_set_style_text_color(ctx->lbl_status, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
      return;
    }

    extern int g_used_channels_count;
    g_used_channels_count = ctx->typed_val;
    ks_relay_service_save_to_json("/relay_map.json");
    update_adv_settings_subtitles();
    lv_obj_del(ctx->overlay);
    return;
  }

  // Xử lý phím xoá
  if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
    if (ctx->typed_val > 0) {
      ctx->typed_val = ctx->typed_val / 10;
      update_set_ch_title(ctx);
    }
    return;
  }

  // Nhập số
  int digit = atoi(txt);
  int new_val = ctx->typed_val * 10 + digit;
  if (new_val <= 999) {
    ctx->typed_val = new_val;
    update_set_ch_title(ctx);
  }
}

static void open_set_used_channels_modal(lv_obj_t *parent) {
  int physical_count = ks_relay_service_get_physical_count();
  
  lv_obj_t *overlay = lv_obj_create(parent);
  lv_obj_set_size(overlay, 720, 720);
  lv_obj_set_pos(overlay, 0, 0);
  lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(overlay, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

  set_ch_ctx_t *ctx = (set_ch_ctx_t *)malloc(sizeof(set_ch_ctx_t));
  memset(ctx, 0, sizeof(set_ch_ctx_t));
  extern int g_used_channels_count;
  
  int init_val = g_used_channels_count;
  if (init_val > physical_count) {
    init_val = physical_count;
  }
  ctx->typed_val = init_val;
  ctx->max_val = physical_count;
  ctx->overlay = overlay;
  lv_obj_add_event_cb(overlay, free_set_ch_ctx_cb, LV_EVENT_DELETE, ctx);

  lv_obj_t *popup = lv_obj_create(overlay);
  lv_obj_set_size(popup, 520, 500);
  lv_obj_center(popup);
  lv_obj_set_style_radius(popup, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(popup, lv_color_hex(0x0F172A), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(popup, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(popup, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(popup, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl_title = lv_label_create(popup);
  lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xF8FAFC), LV_PART_MAIN | LV_STATE_DEFAULT);
  ctx->lbl_title = lbl_title;
  update_set_ch_title(ctx);

  lv_obj_t *numpad_container = lv_obj_create(popup);
  lv_obj_set_size(numpad_container, 460, 270);
  lv_obj_align(numpad_container, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_bg_opa(numpad_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(numpad_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(numpad_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(numpad_container, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(numpad_container, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_layout(numpad_container, LV_LAYOUT_GRID);
  
  static lv_coord_t col_dsc[] = {148, 148, 148, LV_GRID_TEMPLATE_LAST};
  static lv_coord_t row_dsc[] = {58, 58, 58, 58, LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(numpad_container, col_dsc, row_dsc);

  const char *btn_map[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", LV_SYMBOL_BACKSPACE, "0", "Lưu"};

  for (int i = 0; i < 12; i++) {
    int r = i / 3;
    int c = i % 3;

    lv_obj_t *btn = lv_btn_create(numpad_container);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, c, 1, LV_GRID_ALIGN_STRETCH, r, 1);
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (i == 9) { // Xoá
      lv_obj_set_style_bg_color(btn, lv_color_hex(0xEF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else if (i == 11) { // Lưu
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x10B981), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_add_event_cb(btn, set_ch_numpad_btn_cb, LV_EVENT_CLICKED, ctx);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, btn_map[i]);
    lv_obj_center(lbl);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF8FAFC), LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  lv_obj_t *lbl_status = lv_label_create(popup);
  lv_label_set_text_fmt(lbl_status, "Hệ thống đang có tối đa %d cổng", physical_count);
  lv_obj_set_pos(lbl_status, 30, 420);
  lv_obj_set_width(lbl_status, 460);
  lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(lbl_status, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x94A3B8), LV_PART_MAIN | LV_STATE_DEFAULT);
  ctx->lbl_status = lbl_status;

  lv_obj_t *btn_cancel = lv_btn_create(popup);
  lv_obj_set_size(btn_cancel, 80, 48);
  lv_obj_align(btn_cancel, LV_ALIGN_TOP_RIGHT, -15, 20);
  lv_obj_set_style_radius(btn_cancel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_cancel, btn_cancel_cb, LV_EVENT_CLICKED, overlay);

  lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
  lv_obj_center(lbl_cancel);
  lv_label_set_text(lbl_cancel, "Đóng");
  lv_obj_set_style_text_font(lbl_cancel, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_cancel, lv_color_hex(0xF8FAFC), LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void adv_settings_event_used_channels(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    open_set_used_channels_modal(ui_ScreenSettingsAdv);
  }
}

static lv_obj_t *create_adv_card(lv_obj_t *parent, lv_coord_t y,
                                 const char *icon_symbol, const char *title,
                                 const char *subtitle, lv_color_t icon_color) {
  lv_obj_t *card = lv_btn_create(parent);
  lv_obj_set_size(card, 656, 104);
  lv_obj_set_x(card, 32);
  lv_obj_set_y(card, y);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(card, 26, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x0F1722),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(card, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(card, lv_color_hex(0x22384E),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *icon_bubble = lv_obj_create(card);
  lv_obj_set_size(icon_bubble, 60, 60);
  lv_obj_set_pos(icon_bubble, 20, 22);
  lv_obj_clear_flag(icon_bubble, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(icon_bubble, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(icon_bubble, lv_color_hex(0x112131),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(icon_bubble, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(icon_bubble, lv_color_hex(0x334155),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *icon_label = lv_label_create(icon_bubble);
  lv_label_set_text(icon_label, icon_symbol);
  lv_obj_center(icon_label);
  lv_obj_set_style_text_color(icon_label, icon_color,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *title_label = lv_label_create(card);
  lv_obj_set_pos(title_label, 98, 20);
  lv_label_set_text(title_label, title);
  lv_obj_set_style_text_color(title_label, lv_color_hex(0xF8FAFC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *sub_label = lv_label_create(card);
  lv_obj_set_pos(sub_label, 98, 56);
  lv_label_set_text(sub_label, subtitle);
  lv_obj_set_style_text_color(sub_label, lv_color_hex(0x94A3B8),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(sub_label, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *chevron = lv_label_create(card);
  lv_obj_align(chevron, LV_ALIGN_RIGHT_MID, -24, 0);
  lv_label_set_text(chevron, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(chevron, lv_color_hex(0x64748B),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(chevron, &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  return card;
}

void ui_ScreenSettingsAdv_screen_init(void) {
  ui_ScreenSettingsAdv = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_ScreenSettingsAdv, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_ScreenSettingsAdv, lv_color_hex(0x0C1A3E),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_color(ui_ScreenSettingsAdv, lv_color_hex(0x040D1A),
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_dir(ui_ScreenSettingsAdv, LV_GRAD_DIR_VER,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Header */
  lv_obj_t *header = lv_obj_create(ui_ScreenSettingsAdv);
  lv_obj_set_size(header, 720, 72);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(header, lv_color_hex(0x0B1622),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(header, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *btn_back = lv_btn_create(header);
  lv_obj_set_size(btn_back, 124, 48);
  lv_obj_set_pos(btn_back, 16, 12);
  lv_obj_set_style_radius(btn_back, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x123049),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn_back, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(btn_back, lv_color_hex(0x2E84B8),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(btn_back, adv_settings_event_back, LV_EVENT_ALL, NULL);

  lv_obj_t *lbl_back = lv_label_create(btn_back);
  lv_obj_center(lbl_back);
  lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " Quay lại");
  lv_obj_set_style_text_color(lbl_back, lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *lbl_title = lv_label_create(header);
  lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(lbl_title, "Cài đặt nâng cao");
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xE0F2FE),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_28,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Container cuộn chứa các lựa chọn */
  lv_obj_t *container = lv_obj_create(ui_ScreenSettingsAdv);
  lv_obj_set_size(container, 720, 648);
  lv_obj_set_pos(container, 0, 72);
  lv_obj_set_style_bg_opa(container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Thêm các tuỳ chọn */

  ui_btn_sleep =
      create_adv_card(container, 268, LV_SYMBOL_IMAGE, "Thời gian tắt màn hình",
                      "Đang tải...", lv_color_hex(0x10B981));
  ui_btn_delay =
      create_adv_card(container, 386, LV_SYMBOL_LIST, "Độ trễ Bật/Tắt tất cả",
                      "Đang tải...", lv_color_hex(0xF59E0B));
  ui_btn_used_channels =
      create_adv_card(container, 504, LV_SYMBOL_SAVE, "Giới hạn cổng hiển thị",
                      "Đang tải...", lv_color_hex(0x8B5CF6));
  lv_obj_t *btn_change_id = create_adv_card(
      container, 32, LV_SYMBOL_DRIVE, "Đổi mã thiết bị (RS485 ID)",
      "Đổi ID vật lý của mạch rơ-le", lv_color_hex(0xFFFFFF));
  ui_btn_recover = create_adv_card(
      container, 150, LV_SYMBOL_REFRESH, "Phục hồi RS485",
      "Quét & sửa lỗi mất kết nối RS485", lv_color_hex(0x3B82F6));
  lv_obj_add_event_cb(ui_btn_sleep, adv_settings_event_sleep, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_add_event_cb(ui_btn_delay, adv_settings_event_delay, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_add_event_cb(ui_btn_used_channels, adv_settings_event_used_channels,
                      LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(btn_change_id, adv_settings_event_change_id,
                      LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_btn_recover, adv_settings_event_recover_rs485,
                      LV_EVENT_CLICKED, NULL);

  update_adv_settings_subtitles();
}
