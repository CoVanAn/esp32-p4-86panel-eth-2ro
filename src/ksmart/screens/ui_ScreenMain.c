#include "../ui_custom.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "../services/relay/ks_relay_service.h"

/* ================================================================
 * Layout hằng số — map từ bản template 720x720 (RelayBoxScreen.tsx).
 * Các hằng số đặt tại đây để dễ chỉnh khi cần tinh chỉnh lại UI.
 * ================================================================ */
#define SCREEN_W 720
#define SCREEN_H 720

#define HEADER_H 72
#define STATS_ROW_Y 82
#define STATS_ROW_H 70
#define GRID_Y 160
#define GRID_H (SCREEN_H - GRID_Y)

#define RELAY_GRID_COLS 4
#define RELAY_GRID_GAP 12
#define RELAY_GRID_PAD_X 20
#define RELAY_GRID_PAD_TOP 10
#define RELAY_GRID_PAD_BOTTOM 16

/* Ô relay vuông; tính sao cho 4 ô + 3 khoảng cách vừa khít chiều ngang. */
#define RELAY_CARD_SIZE                                                        \
  ((SCREEN_W - 2 * RELAY_GRID_PAD_X -                                          \
    (RELAY_GRID_COLS - 1) * RELAY_GRID_GAP) /                                  \
   RELAY_GRID_COLS)

/* Cache object của toàn bộ màn chính; index 0 dành cho nút "Bật/Tắt tất cả"
 * trên stats row. */
static lv_obj_t *relay_cards[KS_UI_RELAY_TOTAL_COUNT];
static lv_obj_t *relay_icon_bubbles[KS_UI_RELAY_TOTAL_COUNT];
static lv_obj_t *relay_icon_labels[KS_UI_RELAY_TOTAL_COUNT];
static lv_obj_t *relay_name_labels[KS_UI_RELAY_TOTAL_COUNT];
static int relay_event_indices[KS_UI_RELAY_TOTAL_COUNT];
/* -1 = chưa render lần nào; 0/1 = trạng thái đã render gần nhất. Dùng để skip
 * lv_obj_set_style_* lặp khi state không đổi (LVGL không tự diff). */
static int relay_last_rendered_state[KS_UI_RELAY_TOTAL_COUNT];

// Nếu biến g_hide_device_id_0 = 1 thì bỏ qua relay 0 và 1
int g_hide_device_id_0 = 0;
// Biến lưu số lượng kênh sử dụng (0 = lấy toàn bộ)
int g_used_channels_count = 0;

static lv_obj_t *stats_count_label;
static lv_obj_t *toggle_all_on_label;
static lv_obj_t *toggle_all_off_label;
static lv_obj_t *btn_all_on;
static lv_obj_t *btn_all_off;
static lv_obj_t *relay_discovery_chip;
static lv_obj_t *relay_discovery_label;

static lv_obj_t *main_lock_overlay;
static lv_obj_t *main_unlock_button;
static char relay_display_names[KS_UI_RELAY_TOTAL_COUNT][MAX_RELAY_NAME_LEN];
static bool relay_display_names_ready = false;

/* Lazy-create card cho relay vật lý: mặc định 0, chỉ tạo card khi đã biết số
 * relay thật (qua ks_relay_service_get_active_count) để không lãng phí 64 ô
 * placeholder lúc init. Grid object được giữ lại để grow thêm card khi
 * active_count tăng (ví dụ RS485 discovery thêm board mid-run). */
static lv_obj_t *relay_grid_obj;
static int g_relay_card_count;
static bool main_screen_locked = false;
static lv_timer_t *relay_names_json_timer;
static time_t relay_names_json_last_mtime;

static void ensure_relay_display_names_ready(void);
static void normalize_relay_display_name(int relay_index, const char *name,
                                         char *normalized_name,
                                         size_t normalized_name_size);
static void load_relay_display_names_from_storage(void);
static int save_relay_display_names_to_storage(void);
static int import_relay_display_names_from_json(int update_labels);
static void relay_names_json_timer_cb(lv_timer_t *timer);
static void main_refresh_lock_overlay(void);
static void main_create_unlock_button(lv_obj_t *parent);
static void create_main_lock_overlay(void);
static void main_event_lock_overlay(lv_event_t *e);
static void main_event_unlock_button(lv_event_t *e);
static void create_header_device_title(void);
static int load_header_device_display_name(char *out, size_t out_size);
static int save_header_device_display_name_file(const char *name);
static void create_main_ambient_layers(void);
static void main_reset_cached_objects(void);
static void main_event_screen_delete(lv_event_t *e);
static void create_relay_card(lv_obj_t *parent, int relay_index);
static void create_stats_row(void);
static void main_apply_active_relay_visibility(void);
static void toggle_all_event_cb(lv_event_t *e);
static void ensure_relay_cards_created(int target_active_count);

/* Các relay GPIO (theo KS_GPIO_RELAY_COUNT) dùng chip màu vàng như template
 * RelayBoxScreen. */
static bool relay_is_gpio(int relay_index) {
  return relay_index >= 1 && relay_index <= KS_GPIO_RELAY_COUNT;
}

static const char *relay_type_text(int relay_index) {
  if (relay_index <= 0) {
    return "ALL";
  }
  return relay_is_gpio(relay_index) ? "GPIO" : "RS485";
}

static time_t get_file_mtime_seconds(const char *path) {
  if (path == NULL)
    return 0;
  return (time_t)ks_vfs_get_mtime(path);
}

static char *read_text_file(const char *path, size_t *out_size) {
  if (path == NULL)
    return NULL;
  return ks_vfs_read_text_file(path, out_size);
}

static const char *find_substr_in_range(const char *start, const char *end,
                                        const char *needle) {
  size_t needle_len;
  const char *p;

  if (start == NULL || end == NULL || needle == NULL) {
    return NULL;
  }

  needle_len = strlen(needle);
  if (needle_len == 0) {
    return start;
  }

  for (p = start; p + needle_len <= end; p++) {
    if (*p == *needle && memcmp(p, needle, needle_len) == 0) {
      return p;
    }
  }

  return NULL;
}

static const char *find_matching_brace(const char *start, const char *end) {
  int depth = 0;
  int in_string = 0;
  int escape = 0;
  const char *p;

  if (start == NULL || end == NULL || start >= end || *start != '{') {
    return NULL;
  }

  for (p = start; p < end; p++) {
    char c = *p;

    if (in_string) {
      if (escape) {
        escape = 0;
        continue;
      }
      if (c == '\\') {
        escape = 1;
        continue;
      }
      if (c == '"') {
        in_string = 0;
      }
      continue;
    }

    if (c == '"') {
      in_string = 1;
      continue;
    }

    if (c == '{') {
      depth++;
      continue;
    }

    if (c == '}') {
      depth--;
      if (depth == 0) {
        return p;
      }
    }
  }

  return NULL;
}

static int parse_json_int_value(const char *json, const char *key_quoted,
                                int *out_value) {
  const char *p;
  const char *colon;
  char *endptr;
  long v;

  if (json == NULL || key_quoted == NULL || out_value == NULL) {
    return -1;
  }

  p = strstr(json, key_quoted);
  if (p == NULL) {
    return -1;
  }

  colon = strchr(p, ':');
  if (colon == NULL) {
    return -1;
  }

  p = colon + 1;
  while (*p != '\0' && isspace((unsigned char)*p)) {
    p++;
  }

  if (*p == '"') {
    p++;
  }

  errno = 0;
  v = strtol(p, &endptr, 10);
  if (errno != 0 || endptr == p) {
    return -1;
  }

  *out_value = (int)v;
  return 0;
}

static int pick_first_device_id(const char *json, int *out_device_id) {
  const char *p;
  const char *brace;
  const char *json_end;
  char *endptr;
  long v;

  if (json == NULL || out_device_id == NULL) {
    return -1;
  }

  json_end = json + strlen(json);
  p = strstr(json, "\"devices\"");
  if (p == NULL) {
    return -1;
  }

  brace = strchr(p, '{');
  if (brace == NULL) {
    return -1;
  }

  p = strchr(brace, '"');
  if (p == NULL || p >= json_end) {
    return -1;
  }
  p++;

  errno = 0;
  v = strtol(p, &endptr, 10);
  if (errno != 0 || endptr == p) {
    return -1;
  }

  *out_device_id = (int)v;
  return 0;
}

static int parse_json_string_value_quoted(const char *p, const char *end,
                                          char *out, size_t out_size,
                                          const char **out_next) {
  size_t i = 0;
  int escape = 0;

  if (out != NULL && out_size > 0) {
    out[0] = '\0';
  }

  if (p == NULL || end == NULL || out == NULL || out_size == 0) {
    return -1;
  }

  if (p >= end || *p != '"') {
    return -1;
  }
  p++;

  while (p < end && *p != '\0') {
    char c = *p;

    if (escape) {
      switch (c) {
      case '"':
        c = '"';
        break;
      case '\\':
        c = '\\';
        break;
      case 'n':
        c = '\n';
        break;
      case 'r':
        c = '\r';
        break;
      case 't':
        c = '\t';
        break;
      default:
        /* Keep unknown escapes as-is. */
        break;
      }
      escape = 0;
      if (i + 1 < out_size) {
        out[i++] = c;
      }
      p++;
      continue;
    }

    if (c == '\\') {
      escape = 1;
      p++;
      continue;
    }

    if (c == '"') {
      out[i] = '\0';
      if (out_next != NULL) {
        *out_next = p + 1;
      }
      return 0;
    }

    if (i + 1 < out_size) {
      out[i++] = c;
    }
    p++;
  }

  return -1;
}

static int import_relay_display_names_from_json(int update_labels) {
  char *json_buf;
  size_t json_len;
  const char *json_end;
  int target_device_id = -1;
  const char *devices_key;
  const char *devices_start;
  const char *devices_end;
  const char *device_key;
  const char *device_colon;
  const char *device_start;
  const char *device_end;
  const char *relays_key;
  const char *relays_start;
  const char *relays_end;
  const char *p;
  int updated = 0;
  time_t current_mtime;

  json_buf = read_text_file(RELAY_NAME_JSON_PATH, &json_len);
  if (json_buf == NULL) {
    return 0;
  }

  json_end = json_buf + json_len;

  int delay_val;
  if (parse_json_int_value(json_buf, "\"all_relay_delay_ms\"", &delay_val) ==
      0) {
    fprintf(stderr, "[ui-json] Found all_relay_delay_ms: %d\n", delay_val);
    ks_app_runtime_set_all_relay_delay_ms(delay_val);
  }

  int sleep_val;
  if (parse_json_int_value(json_buf, "\"screen_sleep_seconds\"", &sleep_val) ==
      0) {
    fprintf(stderr, "[ui-json] Found screen_sleep_seconds: %d\n", sleep_val);
    ks_app_runtime_set_screen_sleep_ms(sleep_val * 1000);
  }

  // Nếu có biến g_hide_device_id_0 = 1 thì bỏ qua relay 0 và 1
  int hide_dev0;
  if (parse_json_int_value(json_buf, "\"hide-device-id-0\"", &hide_dev0) == 0) {
    g_hide_device_id_0 = hide_dev0;
  }

  int used_ch;
  if (parse_json_int_value(json_buf, "\"used_channels\"", &used_ch) == 0) {
    g_used_channels_count = used_ch;
  }

  devices_key = strstr(json_buf, "\"devices\"");
  if (devices_key == NULL) {
    free(json_buf);
    return 0;
  }

  devices_start = strchr(devices_key, '{');
  if (devices_start == NULL || devices_start >= json_end) {
    free(json_buf);
    return 0;
  }

  devices_end = find_matching_brace(devices_start, json_end);
  if (devices_end == NULL) {
    free(json_buf);
    return 0;
  }

  p = devices_start + 1;
  while (p < devices_end) {
    char *endptr;
    int slave_id;
    int board_offset;
    const char *device_obj_end;
    const char *relays_key_local;
    const char *relays_start_local;
    const char *relays_end_local;
    const char *rp;

    fprintf(stderr, "[ui-json] Checking device at p offset %ld\n",
            (long)(p - json_buf));
    p = strchr(p, '"');
    if (p == NULL || p >= devices_end)
      break;
    p++;

    errno = 0;
    slave_id = (int)strtol(p, &endptr, 10);
    if (errno != 0 || endptr == p || *endptr != '"') {
      fprintf(stderr, "[ui-json] Skipping invalid device key at p offset %ld\n",
              (long)(p - json_buf));
      p = strchr(p, ':');
      if (p == NULL)
        break;
      p = strchr(p, '{');
      if (p == NULL)
        break;
      device_obj_end = find_matching_brace(p, devices_end + 1);
      if (device_obj_end == NULL)
        break;
      p = device_obj_end + 1;
      continue;
    }
    p = endptr + 1;

    fprintf(stderr, "[ui-json] Found slave_id %d\n", slave_id);
    p = strchr(p, ':');
    if (p == NULL || p >= devices_end)
      break;
    p = strchr(p, '{');
    if (p == NULL || p >= devices_end)
      break;

    device_obj_end = find_matching_brace(p, devices_end + 1);
    if (device_obj_end == NULL)
      break;

    board_offset = ks_relay_service_get_board_offset(slave_id);
    fprintf(stderr, "[ui-json] slave_id %d -> board_offset %d\n", slave_id,
            board_offset);

    relays_key_local =
        find_substr_in_range(p, device_obj_end + 1, "\"relays\"");
    if (relays_key_local != NULL) {
      relays_start_local = strchr(relays_key_local, '{');
      if (relays_start_local != NULL && relays_start_local < device_obj_end) {
        relays_end_local =
            find_matching_brace(relays_start_local, device_obj_end + 1);
        if (relays_end_local != NULL) {
          rp = relays_start_local + 1;
          while (rp < relays_end_local) {
            long coil;
            char name_buf[MAX_RELAY_NAME_LEN];
            char normalized_name[MAX_RELAY_NAME_LEN];
            int relay_index;

            while (rp < relays_end_local &&
                   (*rp == ',' || isspace((unsigned char)*rp)))
              rp++;
            if (rp >= relays_end_local || *rp == '}')
              break;
            if (*rp != '"')
              break;
            rp++;

            errno = 0;
            coil = strtol(rp, &endptr, 10);
            if (errno != 0 || endptr == rp)
              break;

            rp = endptr;
            if (rp >= relays_end_local || *rp != '"')
              break;
            rp++;

            while (rp < relays_end_local && isspace((unsigned char)*rp))
              rp++;
            if (rp >= relays_end_local || *rp != ':') {
              const char *maybe_colon =
                  memchr(rp, ':', (size_t)(relays_end_local - rp));
              if (maybe_colon == NULL)
                break;
              rp = maybe_colon;
            }
            rp++;
            while (rp < relays_end_local && isspace((unsigned char)*rp))
              rp++;
            if (rp >= relays_end_local || *rp != '"')
              break;

            if (parse_json_string_value_quoted(rp, relays_end_local, name_buf,
                                               sizeof(name_buf), &rp) != 0)
              break;

            if (board_offset >= 0) {
              relay_index = board_offset + (int)coil + 1;
              fprintf(
                  stderr,
                  "[ui-json] dev %d, coil %ld -> relay_index %d, name: %s\n",
                  slave_id, coil, relay_index, name_buf);
              if (relay_index >= 1 && relay_index < KS_UI_RELAY_TOTAL_COUNT) {
                normalize_relay_display_name(relay_index, name_buf,
                                             normalized_name,
                                             sizeof(normalized_name));
                if (strncmp(relay_display_names[relay_index], normalized_name,
                            sizeof(relay_display_names[relay_index])) != 0) {
                  snprintf(relay_display_names[relay_index],
                           sizeof(relay_display_names[relay_index]), "%s",
                           normalized_name);
                  updated++;
                }

                if (update_labels &&
                    ui_obj_is_ready(relay_name_labels[relay_index])) {
                  lv_label_set_text(relay_name_labels[relay_index],
                                    relay_display_names[relay_index]);
                  lv_obj_set_style_text_font(relay_name_labels[relay_index],
                                             &lv_font_montserrat_18,
                                             LV_PART_MAIN | LV_STATE_DEFAULT);
                }
              }
            }
          }
        }
      }
    }
    p = device_obj_end + 1;
  }

  free(json_buf);

  if (updated > 0) {
    (void)save_relay_display_names_to_storage();
  }

  current_mtime = get_file_mtime_seconds(RELAY_NAME_JSON_PATH);
  if (current_mtime != 0) {
    relay_names_json_last_mtime = current_mtime;
  }

  return updated;
}

/* Nạp tên mặc định một lần để các màn hình cùng đọc chung một nguồn tên relay
 * runtime. */
static void ensure_relay_display_names_ready(void) {
  int relay_index;

  if (relay_display_names_ready) {
    return;
  }

  for (relay_index = 0; relay_index < KS_UI_RELAY_TOTAL_COUNT; relay_index++) {
    snprintf(relay_display_names[relay_index],
             sizeof(relay_display_names[relay_index]), "%s",
             ui_get_relay_default_title(relay_index));
  }

  load_relay_display_names_from_storage();
  /* Optional override from JSON mapping (easier to edit without changing code).
   */
  (void)import_relay_display_names_from_json(0);
  relay_display_names_ready = true;
}

static void relay_names_json_timer_cb(lv_timer_t *timer) {
  time_t mtime;

  (void)timer;

  mtime = get_file_mtime_seconds(RELAY_NAME_JSON_PATH);
  if (mtime == 0) {
    return;
  }

  if (relay_names_json_last_mtime != 0 &&
      mtime == relay_names_json_last_mtime) {
    return;
  }

  (void)import_relay_display_names_from_json(1);
}

/* Cắt khoảng trắng đầu/cuối để tên nhập tay gọn hơn và tự rơi về tên mặc định
 * khi input rỗng. */
static void normalize_relay_display_name(int relay_index, const char *name,
                                         char *normalized_name,
                                         size_t normalized_name_size) {
  const char *start;
  const char *end;
  size_t copy_length;

  if (normalized_name == NULL || normalized_name_size == 0 || relay_index < 0 ||
      relay_index >= KS_UI_RELAY_TOTAL_COUNT) {
    return;
  }

  if (name == NULL) {
    snprintf(normalized_name, normalized_name_size, "%s",
             ui_get_relay_default_title(relay_index));
    return;
  }

  start = name;
  while (*start != '\0' && isspace((unsigned char)*start)) {
    start++;
  }

  end = name + strlen(name);
  while (end > start && isspace((unsigned char)*(end - 1))) {
    end--;
  }

  copy_length = (size_t)(end - start);
  if (copy_length == 0) {
    snprintf(normalized_name, normalized_name_size, "%s",
             ui_get_relay_default_title(relay_index));
    return;
  }

  if (copy_length >= normalized_name_size) {
    copy_length = normalized_name_size - 1;
  }

  memcpy(normalized_name, start, copy_length);
  normalized_name[copy_length] = '\0';
}

/* Đọc file cấu hình tên relay nếu có để khôi phục tên đã đổi ở lần chạy trước.
 */
static void load_relay_display_names_from_storage(void) {
  FILE *config_file;
  char line_buffer[MAX_LINE_LEN];

  config_file = fopen(RELAY_NAME_CONFIG_PATH, "r");
  if (config_file == NULL) {
    if (errno != ENOENT) {
      fprintf(stderr, "Không thể mở file tên relay %s: %s\n",
              RELAY_NAME_CONFIG_PATH, strerror(errno));
    }
    return;
  }

  while (fgets(line_buffer, sizeof(line_buffer), config_file) != NULL) {
    char *separator;
    char *name_text;
    char normalized_name[MAX_RELAY_NAME_LEN];
    long relay_index;

    separator = strchr(line_buffer, '\t');
    if (separator == NULL) {
      continue;
    }

    *separator = '\0';
    name_text = separator + 1;
    name_text[strcspn(name_text, "\r\n")] = '\0';
    relay_index = strtol(line_buffer, NULL, 10);
    if (relay_index < 0 || relay_index >= KS_UI_RELAY_TOTAL_COUNT) {
      continue;
    }

    normalize_relay_display_name((int)relay_index, name_text, normalized_name,
                                 sizeof(normalized_name));
    snprintf(relay_display_names[relay_index],
             sizeof(relay_display_names[relay_index]), "%s", normalized_name);
  }

  fclose(config_file);
}

/* Ghi toàn bộ tên relay ra file cấu hình để đổi tên không bị mất sau khi
 * reboot. */
static int save_relay_display_names_to_storage(void) {
  FILE *config_file;
  int relay_index;

  config_file = fopen(RELAY_NAME_CONFIG_TMP_PATH, "w");
  if (config_file == NULL) {
    fprintf(stderr, "Không thể tạo file tạm tên relay %s: %s\n",
            RELAY_NAME_CONFIG_TMP_PATH, strerror(errno));
    return -1;
  }

  for (relay_index = 0; relay_index < KS_UI_RELAY_TOTAL_COUNT; relay_index++) {
    if (fprintf(config_file, "%d\t%s\n", relay_index,
                relay_display_names[relay_index]) < 0) {
      fprintf(stderr, "Không thể ghi tên relay vào file tạm %s: %s\n",
              RELAY_NAME_CONFIG_TMP_PATH, strerror(errno));
      fclose(config_file);
      return -1;
    }
  }

  if (fclose(config_file) != 0) {
    fprintf(stderr, "Không thể đóng file tạm tên relay %s: %s\n",
            RELAY_NAME_CONFIG_TMP_PATH, strerror(errno));
    return -1;
  }

  if (rename(RELAY_NAME_CONFIG_TMP_PATH, RELAY_NAME_CONFIG_PATH) != 0) {
    fprintf(stderr, "Không thể cập nhật file tên relay %s: %s\n",
            RELAY_NAME_CONFIG_PATH, strerror(errno));
    return -1;
  }

  return 0;
}

/* Trả về tên hiển thị hiện tại của relay để các màn hình dựng UI theo cùng một
 * dữ liệu runtime. */
const char *ui_get_relay_display_name(int relay_index) {
  ensure_relay_display_names_ready();

  if (relay_index < 0 || relay_index >= KS_UI_RELAY_TOTAL_COUNT) {
    return "";
  }

  return relay_display_names[relay_index];
}

/* Cập nhật tên relay runtime và đồng bộ ngay vào label trên màn hình chính nếu
 * object đã tồn tại. */
void ui_set_relay_display_name(int relay_index, const char *name) {
  char normalized_name[MAX_RELAY_NAME_LEN];

  if (relay_index < 0 || relay_index >= KS_UI_RELAY_TOTAL_COUNT) {
    return;
  }

  ensure_relay_display_names_ready();
  normalize_relay_display_name(relay_index, name, normalized_name,
                               sizeof(normalized_name));
  snprintf(relay_display_names[relay_index],
           sizeof(relay_display_names[relay_index]), "%s", normalized_name);
  if (save_relay_display_names_to_storage() != 0) {
    fprintf(stderr, "Không thể lưu tên Relay %d vào cấu hình\n",
            relay_index + 1);
  }

  if (ui_obj_is_ready(relay_name_labels[relay_index])) {
    lv_label_set_text(relay_name_labels[relay_index],
                      relay_display_names[relay_index]);
  }
}

/* Khôi phục tên mặc định của một relay khi người dùng muốn bỏ tên tùy chỉnh. */
void ui_reset_relay_display_name(int relay_index) {
  if (relay_index < 0 || relay_index >= KS_UI_RELAY_TOTAL_COUNT) {
    return;
  }

  ui_set_relay_display_name(relay_index,
                            ui_get_relay_default_title(relay_index));
}

static void main_reset_cached_objects(void) {
  int relay_index;

  ui_ScreenMain = NULL;
  ui_ImageHeaderNetwork = NULL;
  ui_PanelHeaderNetworkDot = NULL;
  ui_LabelHeaderNetworkStatus = NULL;
  ui_ButtonSettings = NULL;
  ui_LabelSettingsIcon = NULL;
  ui_LabelDate = NULL;
  ui_LabelTime = NULL;
  ui_PanelRelay0 = NULL;
  ui_LabelRelay0 = NULL;
  ui_PanelRelay1 = NULL;
  ui_LabelRelay1 = NULL;
  ui_LabelLogo = NULL;
  ui_LabelHeaderDeviceName = NULL;

  for (relay_index = 0; relay_index < KS_UI_RELAY_TOTAL_COUNT; relay_index++) {
    relay_cards[relay_index] = NULL;
    relay_icon_bubbles[relay_index] = NULL;
    relay_icon_labels[relay_index] = NULL;
    relay_name_labels[relay_index] = NULL;
    relay_last_rendered_state[relay_index] = -1;
  }

  stats_count_label = NULL;
  btn_all_on = NULL;
  btn_all_off = NULL;
  toggle_all_on_label = NULL;
  toggle_all_off_label = NULL;
  relay_discovery_chip = NULL;
  relay_discovery_label = NULL;
  main_lock_overlay = NULL;
  main_unlock_button = NULL;
  relay_grid_obj = NULL;
  g_relay_card_count = 0;
}

static void main_event_screen_delete(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_DELETE ||
      lv_event_get_target(e) != ui_ScreenMain) {
    return;
  }

  if (relay_names_json_timer != NULL) {
    lv_timer_del(relay_names_json_timer);
    relay_names_json_timer = NULL;
  }

  main_reset_cached_objects();
}

/* Trạng thái khóa màn hình được giữ ở runtime để menu cài đặt và màn chính cùng
 * nhìn một nguồn dữ liệu. */
bool ui_is_screen_locked(void) { return main_screen_locked; }

/* Bật/tắt khóa màn hình và đồng bộ lại overlay nếu màn chính đã được dựng. */
void ui_set_screen_locked(bool locked) {
  main_screen_locked = locked;
  main_refresh_lock_overlay();
}

/* Chỉ khi đang khóa mới hiện lớp phủ; mỗi lần khóa lại sẽ ẩn nút mở để người
 * dùng phải xác nhận bằng một chạm mới. */
static void main_refresh_lock_overlay(void) {
  if (main_lock_overlay == NULL || main_unlock_button == NULL) {
    return;
  }

  if (!main_screen_locked) {
    lv_obj_add_flag(main_lock_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(main_unlock_button, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  lv_obj_clear_flag(main_lock_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(main_unlock_button, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(main_lock_overlay);
}

/* Nút mở khóa dùng layout hàng ngang để icon khóa tự vẽ nằm trước text mà không
 * phụ thuộc symbol mặc định của LVGL. */
static void main_create_unlock_button(lv_obj_t *parent) {
  lv_obj_t *label;

  main_unlock_button = lv_btn_create(parent);
  lv_obj_set_size(main_unlock_button, 320, 76);
  lv_obj_center(main_unlock_button);
  lv_obj_clear_flag(main_unlock_button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(main_unlock_button, 24,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(main_unlock_button, lv_color_hex(0xFFFFFF),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(main_unlock_button, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(main_unlock_button, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(main_unlock_button, lv_color_hex(0xD0D7DE),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(main_unlock_button, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  label = lv_label_create(main_unlock_button);
  lv_obj_center(label);
  lv_label_set_text(label, "Nhan de mo khoa");
  lv_obj_set_style_text_color(label, lv_color_hex(0x111111),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_22,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_add_flag(main_unlock_button, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(main_unlock_button, main_event_unlock_button,
                      LV_EVENT_ALL, NULL);
}

/* Overlay phủ toàn bộ màn chính để chặn thao tác relay và chỉ nhả ra khi người
 * dùng chủ động mở khóa. */
static void create_main_lock_overlay(void) {
  main_lock_overlay = lv_obj_create(ui_ScreenMain);
  lv_obj_set_size(main_lock_overlay, SCREEN_W, SCREEN_H);
  lv_obj_set_pos(main_lock_overlay, 0, 0);
  lv_obj_clear_flag(main_lock_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(main_lock_overlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(main_lock_overlay, 0,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(main_lock_overlay, lv_color_hex(0x000000),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(main_lock_overlay, 220,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(main_lock_overlay, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(main_lock_overlay, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(main_lock_overlay, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);

  main_create_unlock_button(main_lock_overlay);
  lv_obj_add_flag(main_lock_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(main_lock_overlay, main_event_lock_overlay, LV_EVENT_ALL,
                      NULL);
}

static void main_event_lock_overlay(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED && main_screen_locked) {
    ui_set_screen_locked(false);
  }
}

static void main_event_unlock_button(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    ui_set_screen_locked(false);
  }
}

/* Nền solid (template ocean-dark); không vẽ bg_img để nhẹ CPU/GPU trên Luckfox
 * 86 panel. */
static void create_main_background(void) {
  lv_obj_set_style_bg_color(ui_ScreenMain, lv_color_hex(0x0C1A3E),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_color(ui_ScreenMain, lv_color_hex(0x040D1A),
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_dir(ui_ScreenMain, LV_GRAD_DIR_VER,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ScreenMain, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/* Tạo 2 lớp ambient nhẹ để màn chính gần với visual của template 720x720. */
static void create_main_ambient_layers(void) {
  lv_obj_t *glow_top;
  lv_obj_t *glow_bottom;

  glow_top = lv_obj_create(ui_ScreenMain);
  lv_obj_set_size(glow_top, 360, 240);
  lv_obj_set_pos(glow_top, -80, -36);
  lv_obj_clear_flag(glow_top, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(glow_top, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(glow_top, LV_RADIUS_CIRCLE,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(glow_top, lv_color_hex(0x0EA476),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(glow_top, 32, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(glow_top, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(glow_top, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(glow_top, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  glow_bottom = lv_obj_create(ui_ScreenMain);
  lv_obj_set_size(glow_bottom, 300, 220);
  lv_obj_set_pos(glow_bottom, 500, 520);
  lv_obj_clear_flag(glow_bottom, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(glow_bottom, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(glow_bottom, LV_RADIUS_CIRCLE,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(glow_bottom, lv_color_hex(0x06B6D4),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(glow_bottom, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(glow_bottom, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(glow_bottom, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(glow_bottom, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static int load_header_device_display_name(char *out, size_t out_size) {
  FILE *config_file;
  size_t n;

  if (out == NULL || out_size == 0) {
    return -1;
  }
  out[0] = '\0';
  config_file = fopen(KS_DEVICE_DISPLAY_NAME_PATH, "r");
  if (config_file == NULL) {
    return -1;
  }
  if (fgets(out, (int)out_size, config_file) == NULL) {
    (void)fclose(config_file);
    return -1;
  }
  (void)fclose(config_file);
  n = strcspn(out, "\r\n");
  out[n] = '\0';
  return 0;
}

static int save_header_device_display_name_file(const char *name) {
  FILE *config_file;

  if (name == NULL) {
    return -1;
  }
  config_file = fopen(KS_DEVICE_DISPLAY_NAME_TMP_PATH, "w");
  if (config_file == NULL) {
    return -1;
  }
  if (fprintf(config_file, "%s\n", name) < 0) {
    (void)fclose(config_file);
    return -1;
  }
  if (fclose(config_file) != 0) {
    return -1;
  }
  if (rename(KS_DEVICE_DISPLAY_NAME_TMP_PATH, KS_DEVICE_DISPLAY_NAME_PATH) !=
      0) {
    return -1;
  }
  return 0;
}

static void create_header_device_title(void) {
  char initial[KS_DEVICE_DISPLAY_NAME_MAX_LEN];

  initial[0] = '\0';
  if (load_header_device_display_name(initial, sizeof(initial)) != 0 ||
      initial[0] == '\0') {
    (void)strncpy(initial, "HMI", sizeof(initial) - 1);
    initial[sizeof(initial) - 1] = '\0';
  }

  ui_LabelHeaderDeviceName = lv_label_create(ui_ScreenMain);
  lv_obj_set_width(ui_LabelHeaderDeviceName, 280);
  lv_label_set_long_mode(ui_LabelHeaderDeviceName, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(ui_LabelHeaderDeviceName, 20, 26);
  lv_label_set_text(ui_LabelHeaderDeviceName, initial);
  lv_obj_set_style_text_color(ui_LabelHeaderDeviceName, lv_color_hex(0xE0F2FE),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelHeaderDeviceName, &lv_font_montserrat_22,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_set_header_device_display_name(const char *name) {
  char trimmed[KS_DEVICE_DISPLAY_NAME_MAX_LEN];
  const char *start;
  const char *end;
  size_t len;
  const char *use;

  if (name == NULL) {
    use = "HMI";
  } else {
    start = name;
    while (*start != '\0' && isspace((unsigned char)*start)) {
      start++;
    }
    end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) {
      end--;
    }
    len = (size_t)(end - start);
    if (len == 0) {
      use = "HMI";
    } else {
      if (len >= sizeof(trimmed)) {
        len = sizeof(trimmed) - 1;
      }
      memcpy(trimmed, start, len);
      trimmed[len] = '\0';
      use = trimmed;
    }
  }

  if (ui_obj_is_ready(ui_LabelHeaderDeviceName)) {
    lv_label_set_text(ui_LabelHeaderDeviceName, use);
  }
  if (save_header_device_display_name_file(use) != 0) {
    fprintf(stderr, "Không thể lưu tên thiết bị header vào file\n");
  }
}

/* Chip trạng thái mạng hiển thị dot + text ngắn, không còn mở màn network trực
 * tiếp. */
static void create_header_status_chip(void) {
  lv_obj_t *status_chip = lv_obj_create(ui_ScreenMain);

  lv_obj_set_size(status_chip, 168, 38);
  lv_obj_set_x(status_chip, 480);
  lv_obj_set_y(status_chip, 22);
  lv_obj_clear_flag(status_chip, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(status_chip, 19, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(status_chip, lv_color_hex(0x0B2943),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(status_chip, 170, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(status_chip, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(status_chip, lv_color_hex(0x2E84B8),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(status_chip, 170,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(status_chip, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(status_chip, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  ui_ImageHeaderNetwork = NULL;

  ui_PanelHeaderNetworkDot = lv_obj_create(status_chip);
  lv_obj_set_size(ui_PanelHeaderNetworkDot, 8, 8);
  lv_obj_set_x(ui_PanelHeaderNetworkDot, 14);
  lv_obj_set_y(ui_PanelHeaderNetworkDot, 15);
  lv_obj_clear_flag(ui_PanelHeaderNetworkDot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_PanelHeaderNetworkDot, LV_RADIUS_CIRCLE,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_PanelHeaderNetworkDot, lv_color_hex(0x22C55E),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_PanelHeaderNetworkDot, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_PanelHeaderNetworkDot, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_PanelHeaderNetworkDot, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_LabelHeaderNetworkStatus = lv_label_create(status_chip);
  lv_obj_set_x(ui_LabelHeaderNetworkStatus, 30);
  lv_obj_set_y(ui_LabelHeaderNetworkStatus, 6);
  lv_label_set_text(ui_LabelHeaderNetworkStatus, "Đã kết nối");
  lv_obj_set_style_text_color(ui_LabelHeaderNetworkStatus,
                              lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelHeaderNetworkStatus,
                             &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
}

/* Số lượng relay vật lý đang bật — dùng cho stats row và nút toggle all. */
static int count_physical_relays_on(void) {
  int on = 0;
  int i;
  int active_count = ks_relay_service_get_active_count();
  for (i = 0; i < active_count; i++) {
    if (g_hide_device_id_0 == 1 && (i == 0 || i == 1)) {
      continue;
    }
    if (ks_relay_service_get_cached_state(i)) {
      on++;
    }
  }
  return on;
}

/* Helper nội bộ để tránh đếm relays_on hai lần khi caller đã có sẵn số liệu. */
static void refresh_main_stats_row_with(int on, int active_count) {
  char buf[48];
  int pct;

  if (!ui_obj_is_ready(stats_count_label) && !ui_obj_is_ready(btn_all_on)) {
    return;
  }

  pct = (active_count > 0) ? ((on * 100) / active_count) : 0;

  if (ui_obj_is_ready(stats_count_label)) {
    snprintf(buf, sizeof(buf), "%d/%d bật · %d%%", on, active_count, pct);
    lv_label_set_text(stats_count_label, buf);
  }

  if (ui_obj_is_ready(btn_all_on) && ui_obj_is_ready(btn_all_off)) {
    /* Logic: Highlight "Tắt" only when all are on, otherwise highlight "Bật"
     * (if any are off). */
    bool all_on = (on == active_count && active_count > 0);
    ui_update_relay_card_visual(0, all_on);
  }
}

void ui_refresh_main_stats_row(void) {

  // Nếu có biến g_hide_device_id_0 = 1 thì bỏ qua relay 0 và 1
  int active_count = ks_relay_service_get_active_count();
  if (g_hide_device_id_0 == 1 && active_count >= 2) {
    active_count -= 2;
  }
  refresh_main_stats_row_with(count_physical_relays_on(), active_count);
}
/* Callback cho nút "Bật/Tắt tất cả" trên stats row; vẫn dùng chỉ số 0 = relay
 * "Tất cả". */
static void toggle_all_on_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    ks_app_runtime_schedule_all_relays_from_ui(true);
  }
}

static void toggle_all_off_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    ks_app_runtime_schedule_all_relays_from_ui(false);
  }
}

/* Tạo stats row: label trái + time/date center + nút toggle all phải. */
static void create_stats_row(void) {
  lv_obj_t *divider;
  lv_obj_t *stats_row;

  /* Divider mảnh tách header và nội dung. */
  divider = lv_obj_create(ui_ScreenMain);
  lv_obj_set_size(divider, SCREEN_W - 40, 1);
  lv_obj_set_pos(divider, 20, HEADER_H);
  lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(divider, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(divider, lv_color_hex(0x0EA5E9),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(divider, 90, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Container trong suốt để dễ canh lề; không cần background riêng. */
  stats_row = lv_obj_create(ui_ScreenMain);
  lv_obj_set_size(stats_row, SCREEN_W, STATS_ROW_H);
  lv_obj_set_pos(stats_row, 0, STATS_ROW_Y);
  lv_obj_clear_flag(stats_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(stats_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(stats_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(stats_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Label "X/N bật · P%" bên trái. */
  stats_count_label = lv_label_create(stats_row);
  lv_obj_set_pos(stats_count_label, 20, 16);
  lv_label_set_text(stats_count_label, "0/0 bật · 0%");
  lv_obj_set_style_text_color(stats_count_label, lv_color_hex(0x2DD4BF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(stats_count_label, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Đồng hồ lớn trung tâm: dùng font lớn sẵn có của project. */
  ui_LabelTime = lv_label_create(stats_row);
  lv_obj_align(ui_LabelTime, LV_ALIGN_TOP_MID, 0, 0);
  lv_label_set_text(ui_LabelTime, "00:00");
  lv_obj_set_style_text_color(ui_LabelTime, lv_color_hex(0xE0F2FE),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelTime, &lv_font_montserrat_36,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_LabelDate = lv_label_create(stats_row);
  lv_obj_align(ui_LabelDate, LV_ALIGN_TOP_MID, 0, 48);
  lv_label_set_text(ui_LabelDate, "--, --/--/----");
  lv_obj_set_style_text_color(ui_LabelDate, lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelDate, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Label "Tất cả:" */
  lv_obj_t *label_all = lv_label_create(stats_row);
  lv_obj_set_pos(label_all, SCREEN_W - 245, 28);
  lv_label_set_text(label_all, "Tất cả:");
  lv_obj_set_style_text_color(label_all, lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(label_all, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Nút "Bật" */
  btn_all_on = lv_btn_create(stats_row);
  lv_obj_set_size(btn_all_on, 75, 44);
  lv_obj_set_pos(btn_all_on, SCREEN_W - 170, 18);
  lv_obj_clear_flag(btn_all_on, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(btn_all_on, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_all_on, lv_color_hex(0x0B2B4A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(btn_all_on, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn_all_on, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(btn_all_on, lv_color_hex(0x0EA5E9),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(btn_all_on, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  toggle_all_on_label = lv_label_create(btn_all_on);
  lv_obj_center(toggle_all_on_label);
  lv_label_set_text(toggle_all_on_label, "Bật");
  lv_obj_set_style_text_color(toggle_all_on_label, lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(toggle_all_on_label, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Nút "Tắt" */
  btn_all_off = lv_btn_create(stats_row);
  lv_obj_set_size(btn_all_off, 75, 44);
  lv_obj_set_pos(btn_all_off, SCREEN_W - 85, 18);
  lv_obj_clear_flag(btn_all_off, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(btn_all_off, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_all_off, lv_color_hex(0x0B2B4A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(btn_all_off, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn_all_off, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(btn_all_off, lv_color_hex(0x0EA5E9),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(btn_all_off, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  toggle_all_off_label = lv_label_create(btn_all_off);
  lv_obj_center(toggle_all_off_label);
  lv_label_set_text(toggle_all_off_label, "Tắt");
  lv_obj_set_style_text_color(toggle_all_off_label, lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(toggle_all_off_label, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Compat: một số service cũ tham chiếu ui_PanelRelay0/ui_LabelRelay0 nên giữ
   * cho ánh xạ tới nút bật. */
  relay_cards[0] = btn_all_on;
  ui_PanelRelay0 = btn_all_on;
  ui_LabelRelay0 = toggle_all_on_label;

  lv_obj_add_event_cb(btn_all_on, toggle_all_on_event_cb, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_add_event_cb(btn_all_off, toggle_all_off_event_cb, LV_EVENT_CLICKED,
                      NULL);
}

/*
 * Tạo một card relay trong lưới 4 cột. Tối ưu số object cho Luckfox Pico:
 *   - card (bg + border)
 *     - chip "GPIO"/"RS485" (bg + label)
 *     - bubble tròn (1 object) + icon label
 *     - name label
 *     - state dot
 * = 6 object × 16 relay = 96 object, vẫn nhẹ cho FBdev/SDL ở layout 720x720.
 */
static void create_relay_card(lv_obj_t *parent, int relay_index) {
  lv_obj_t *type_chip;
  lv_obj_t *type_label;

  /* Thân card. Kích thước được flex-layout của grid tự đặt nhưng vẫn khai báo
   * rõ để layout ổn định. */
  relay_cards[relay_index] = lv_obj_create(parent);
  lv_obj_set_size(relay_cards[relay_index], RELAY_CARD_SIZE, RELAY_CARD_SIZE);
  lv_obj_clear_flag(relay_cards[relay_index], LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(relay_cards[relay_index], LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(relay_cards[relay_index], 20,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(relay_cards[relay_index], 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(relay_cards[relay_index], 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(relay_cards[relay_index], 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);

  // Nếu có biến g_hide_device_id_0 = 1 thì bỏ qua relay 0 và 1
  if (g_hide_device_id_0 == 1 && (relay_index == 1 || relay_index == 2)) {
    lv_obj_add_flag(relay_cards[relay_index], LV_OBJ_FLAG_HIDDEN);
  }

  /* Chip loại relay góc trên trái — dùng một label có bg để gọn đối tượng. */
  // type_chip = lv_obj_create(relay_cards[relay_index]);
  // lv_obj_set_size(type_chip, 42, 18);
  // lv_obj_set_pos(type_chip, 10, 10);
  // lv_obj_clear_flag(type_chip, LV_OBJ_FLAG_SCROLLABLE);
  // lv_obj_clear_flag(type_chip, LV_OBJ_FLAG_CLICKABLE);
  // lv_obj_set_style_radius(type_chip, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
  // lv_obj_set_style_border_width(type_chip, 0, LV_PART_MAIN |
  // LV_STATE_DEFAULT); lv_obj_set_style_shadow_width(type_chip, 0, LV_PART_MAIN
  // | LV_STATE_DEFAULT); lv_obj_set_style_pad_all(type_chip, 0, LV_PART_MAIN |
  // LV_STATE_DEFAULT); if (relay_is_gpio(relay_index)) {
  //   lv_obj_set_style_bg_color(type_chip, lv_color_hex(0xFACC15),
  //                             LV_PART_MAIN | LV_STATE_DEFAULT);
  //   lv_obj_set_style_bg_opa(type_chip, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
  // } else {
  //   lv_obj_set_style_bg_color(type_chip, lv_color_hex(0x0EA5E9),
  //                             LV_PART_MAIN | LV_STATE_DEFAULT);
  //   lv_obj_set_style_bg_opa(type_chip, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
  // }

  // type_label = lv_label_create(type_chip);
  // lv_obj_center(type_label);
  // lv_label_set_text(type_label, relay_type_text(relay_index));
  // lv_obj_set_style_text_color(type_label,
  //                             relay_is_gpio(relay_index)
  //                                 ? lv_color_hex(0xFBBF24)
  //                                 : lv_color_hex(0x7DD3FC),
  //                             LV_PART_MAIN | LV_STATE_DEFAULT);
  // lv_obj_set_style_text_font(type_label, &lv_font_montserrat_12,
  //                            LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Bubble tròn giữa card. */
  relay_icon_bubbles[relay_index] = lv_obj_create(relay_cards[relay_index]);
  lv_obj_set_size(relay_icon_bubbles[relay_index], 54, 54);
  lv_obj_align(relay_icon_bubbles[relay_index], LV_ALIGN_TOP_MID, 0, 30);
  lv_obj_clear_flag(relay_icon_bubbles[relay_index], LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(relay_icon_bubbles[relay_index], LV_RADIUS_CIRCLE,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(relay_icon_bubbles[relay_index], 2,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(relay_icon_bubbles[relay_index], 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(relay_icon_bubbles[relay_index], 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(relay_icon_bubbles[relay_index], LV_OBJ_FLAG_EVENT_BUBBLE);

  relay_icon_labels[relay_index] =
      lv_label_create(relay_icon_bubbles[relay_index]);
  lv_obj_center(relay_icon_labels[relay_index]);
  lv_label_set_text(relay_icon_labels[relay_index], LV_SYMBOL_POWER);
  lv_obj_set_style_text_font(relay_icon_labels[relay_index],
                             &lv_font_montserrat_24,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(relay_icon_labels[relay_index], LV_OBJ_FLAG_EVENT_BUBBLE);

  /* Tên relay ở cuối card. */
  relay_name_labels[relay_index] = lv_label_create(relay_cards[relay_index]);
  lv_obj_set_width(relay_name_labels[relay_index], RELAY_CARD_SIZE - 16);
  lv_obj_align(relay_name_labels[relay_index], LV_ALIGN_TOP_MID, 0, 100);
  lv_label_set_long_mode(relay_name_labels[relay_index], LV_LABEL_LONG_WRAP);
  lv_label_set_text(relay_name_labels[relay_index],
                    ui_get_relay_display_name(relay_index));
  lv_obj_set_style_text_align(relay_name_labels[relay_index],
                              LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(relay_name_labels[relay_index],
                             &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Relay 1/2 là GPIO, cần callback riêng theo code hiện hữu; còn lại dùng
   * chung RS485 handler và truyền chỉ số. */
  if (relay_index == 1) {
    ui_PanelRelay1 = relay_cards[relay_index];
    ui_LabelRelay1 = relay_name_labels[relay_index];
    lv_obj_add_event_cb(relay_cards[relay_index], ui_event_PanelRelay1,
                        LV_EVENT_ALL, NULL);
  } else {
    relay_event_indices[relay_index] = relay_index;
    lv_obj_add_event_cb(relay_cards[relay_index], ui_event_PanelRelayRs485,
                        LV_EVENT_ALL, &relay_event_indices[relay_index]);
  }
}

/*
 * Cập nhật giao diện một thẻ relay theo trạng thái bật/tắt.
 * - relay_index = 0: nút "Bật/Tắt tất cả" trên stats row → chỉ cập nhật màu +
 * text.
 * - relay_index > 0: card trong lưới → cập nhật nền/viền/chấm trạng thái.
 */
void ui_update_relay_card_visual(int relay_index, bool enabled) {
  if (relay_index < 0 || relay_index >= KS_UI_RELAY_TOTAL_COUNT ||
      !ui_obj_is_ready(relay_cards[relay_index])) {
    return;
  }

  if (relay_last_rendered_state[relay_index] == (int)enabled) {
    return;
  }
  relay_last_rendered_state[relay_index] = (int)enabled;

  if (relay_index == 0) {
    /* Nút All On/Off trên stats row: highlight nút Tắt nếu tất cả đang bật,
     * ngược lại highlight nút Bật. */
    if (ui_obj_is_ready(btn_all_on)) {
      lv_obj_set_style_bg_color(btn_all_on,
                                !enabled ? lv_color_hex(0x0284C7)
                                         : lv_color_hex(0x0B2B4A),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(btn_all_on, !enabled ? 200 : 170,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_color(btn_all_on,
                                    !enabled ? lv_color_hex(0x67D8FF)
                                             : lv_color_hex(0x0EA5E9),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      if (ui_obj_is_ready(toggle_all_on_label)) {
        lv_obj_set_style_text_color(toggle_all_on_label,
                                    !enabled ? lv_color_hex(0xE0F2FE)
                                             : lv_color_hex(0x7DD3FC),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      }
    }
    if (ui_obj_is_ready(btn_all_off)) {
      lv_obj_set_style_bg_color(btn_all_off,
                                enabled ? lv_color_hex(0x0284C7)
                                        : lv_color_hex(0x0B2B4A),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(btn_all_off, enabled ? 200 : 170,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_border_color(btn_all_off,
                                    enabled ? lv_color_hex(0x67D8FF)
                                            : lv_color_hex(0x0EA5E9),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      if (ui_obj_is_ready(toggle_all_off_label)) {
        lv_obj_set_style_text_color(toggle_all_off_label,
                                    enabled ? lv_color_hex(0xE0F2FE)
                                            : lv_color_hex(0x7DD3FC),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
      }
    }
    return;
  }

  if (!ui_obj_is_ready(relay_icon_bubbles[relay_index]) ||
      !ui_obj_is_ready(relay_icon_labels[relay_index]) ||
      !ui_obj_is_ready(relay_name_labels[relay_index])) {
    return;
  }

  /* Card: màu nền đơn (bỏ gradient); chỉ thêm glow khi bật để tiết kiệm CPU. */
  lv_obj_set_style_bg_color(relay_cards[relay_index],
                            enabled ? lv_color_hex(0x0F3A58)
                                    : lv_color_hex(0x0A1B2E),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(relay_cards[relay_index], 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(relay_cards[relay_index],
                                enabled ? lv_color_hex(0x0EA5E9)
                                        : lv_color_hex(0x1F4A6D),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(relay_cards[relay_index], enabled ? 220 : 120,
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Bubble tròn: nền + viền đổi theo state. Không dùng shadow để tiết kiệm CPU
   * khi có nhiều card. */
  lv_obj_set_style_bg_color(relay_icon_bubbles[relay_index],
                            enabled ? lv_color_hex(0x0EA5E9)
                                    : lv_color_hex(0x0B233A),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(relay_icon_bubbles[relay_index], 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(relay_icon_bubbles[relay_index],
                                enabled ? lv_color_hex(0x67D8FF)
                                        : lv_color_hex(0x2B678B),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(relay_icon_bubbles[relay_index],
                              enabled ? 255 : 150,
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_set_style_text_color(relay_icon_labels[relay_index],
                              enabled ? lv_color_hex(0xFFFFFF)
                                      : lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_set_style_text_color(relay_name_labels[relay_index],
                              enabled ? lv_color_hex(0xBAE6FD)
                                      : lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(relay_name_labels[relay_index], enabled ? 255 : 180,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
}

/* Tạo bổ sung card cho relay 1-based đến target_active_count nếu chưa có.
 * Mặc định g_relay_card_count = 0 (chưa biết số relay vật lý); mỗi lần caller
 * có active_count mới (từ ks_relay_service_get_active_count), gọi hàm này để
 * grow tới đúng số đó — tránh tạo trước 64 placeholder. */
static void ensure_relay_cards_created(int target_active_count) {
  int relay_index;
  int max_count = KS_UI_RELAY_TOTAL_COUNT - 1;

  if (relay_grid_obj == NULL) {
    return;
  }
  if (target_active_count > max_count) {
    target_active_count = max_count;
  }
  if (target_active_count <= g_relay_card_count) {
    return;
  }

  for (relay_index = g_relay_card_count + 1; relay_index <= target_active_count;
       relay_index++) {
    create_relay_card(relay_grid_obj, relay_index);
  }
  g_relay_card_count = target_active_count;
}

/* Cập nhật visual cho 1 relay vật lý (1-based) + nút toggle all + stats row.
 * Dùng thay cho ui_refresh_relay_card_states() khi caller đã biết relay nào
 * vừa đổi để khỏi quét toàn bộ 16 card. */
void ui_apply_relay_state_change(int relay_index_1based, bool enabled) {
  int active_count = ks_relay_service_get_active_count();
  int on;

  if (active_count > (KS_UI_RELAY_TOTAL_COUNT - 1)) {
    active_count = KS_UI_RELAY_TOTAL_COUNT - 1;
  }

  ensure_relay_cards_created(active_count);

  if (relay_index_1based >= 1 && relay_index_1based <= active_count) {
    ui_update_relay_card_visual(relay_index_1based, enabled);
  }

  on = count_physical_relays_on();

  // Nếu có biến g_hide_device_id_0 = 1 thì bỏ qua relay 0 và 1
  int display_active_count = active_count;
  if (g_hide_device_id_0 == 1 && display_active_count >= 2) {
    display_active_count -= 2;
  }
  ui_update_relay_card_visual(0, display_active_count > 0 &&
                                     on == display_active_count);
  refresh_main_stats_row_with(on, display_active_count);
}

void ui_refresh_relay_card_states(void) {
  int relay_index;
  int on = count_physical_relays_on();
  int active_count = ks_relay_service_get_active_count();

  if (active_count > (KS_UI_RELAY_TOTAL_COUNT - 1)) {
    active_count = KS_UI_RELAY_TOTAL_COUNT - 1;
  }

  /* RS485 discovery có thể tăng active_count sau init: grow card list rồi mới
   * áp visibility/màu. */
  ensure_relay_cards_created(active_count);

  /* Trong một số trường hợp init/scan hoàn tất muộn, card có thể bị ẩn theo
   * active_count cũ. Luôn áp lại visibility trước khi tô màu. */
  main_apply_active_relay_visibility();

  // Nếu có biến g_hide_device_id_0 = 1 thì bỏ qua relay 0 và 1
  int display_active_count = active_count;
  if (g_hide_device_id_0 == 1 && display_active_count >= 2) {
    display_active_count -= 2;
  }
  ui_update_relay_card_visual(0, display_active_count > 0 &&
                                     on == display_active_count);

  for (relay_index = 1; relay_index <= active_count; relay_index++) {
    ui_update_relay_card_visual(
        relay_index, ks_relay_service_get_cached_state(relay_index - 1));
  }
  refresh_main_stats_row_with(on, display_active_count);
}

static void main_apply_active_relay_visibility(void) {
  int relay_index;
  int active_ui_count = ks_relay_service_get_active_count() + 1;

  if (active_ui_count < 1) {
    active_ui_count = 1;
  }
  if (active_ui_count > KS_UI_RELAY_TOTAL_COUNT) {
    active_ui_count = KS_UI_RELAY_TOTAL_COUNT;
  }

  /* Chỉ duyệt những card đã tạo (g_relay_card_count) thay vì 64 placeholder.
   * Edge case: nếu active_count giảm (ví dụ tháo board RS485), card thừa được
   * hide; còn tăng thì ensure_relay_cards_created đã grow trước khi gọi đây. */
  for (relay_index = 1; relay_index <= g_relay_card_count; relay_index++) {
    if (!ui_obj_is_ready(relay_cards[relay_index])) {
      continue;
    }

    if (relay_index < active_ui_count) {
      // Nếu có biến g_hide_device_id_0 = 1 thì bỏ qua relay 0 và 1
      if (g_hide_device_id_0 == 1 && (relay_index == 1 || relay_index == 2)) {
        lv_obj_add_flag(relay_cards[relay_index], LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_obj_clear_flag(relay_cards[relay_index], LV_OBJ_FLAG_HIDDEN);
      }
    } else {
      lv_obj_add_flag(relay_cards[relay_index], LV_OBJ_FLAG_HIDDEN);
    }
  }
}

/* Khởi tạo lại màn hình chính theo bố cục 720x720 của template RelayBoxScreen.
 */
void ui_ScreenMain_screen_init(void) {
  lv_obj_t *relay_grid;

  ensure_relay_display_names_ready();
  main_reset_cached_objects();

  if (relay_names_json_timer == NULL) {
    relay_names_json_timer =
        lv_timer_create(relay_names_json_timer_cb, 1000, NULL);
  }

  /* ===== Screen gốc ===== */
  ui_ScreenMain = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_ScreenMain, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_ScreenMain, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_ScreenMain, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_ScreenMain, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(ui_ScreenMain, main_event_screen_delete, LV_EVENT_DELETE,
                      NULL);

  create_main_background();
  create_header_device_title();
  create_header_status_chip();

  /* Logo KSMART giữa header. */
  ui_LabelLogo = lv_label_create(ui_ScreenMain);
  lv_obj_align(ui_LabelLogo, LV_ALIGN_TOP_MID, 0, 24);
  lv_label_set_text(ui_LabelLogo, "KSMART");
  lv_obj_set_style_text_color(ui_LabelLogo, lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelLogo, &lv_font_montserrat_22,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_letter_space(ui_LabelLogo, 6,
                                     LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Nút settings. */
  ui_ButtonSettings = lv_btn_create(ui_ScreenMain);
  lv_obj_set_size(ui_ButtonSettings, 44, 38);
  lv_obj_set_pos(ui_ButtonSettings, 658, 22);
  lv_obj_clear_flag(ui_ButtonSettings, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(ui_ButtonSettings, 12,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_ButtonSettings, lv_color_hex(0x0B2943),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_ButtonSettings, 170,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_ButtonSettings, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui_ButtonSettings, lv_color_hex(0x0EA5E9),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_ButtonSettings, 150,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(ui_ButtonSettings, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(ui_ButtonSettings, 0,
                           LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_LabelSettingsIcon = lv_label_create(ui_ButtonSettings);
  lv_obj_center(ui_LabelSettingsIcon);
  lv_label_set_text(ui_LabelSettingsIcon, LV_SYMBOL_SETTINGS);
  lv_obj_set_style_text_color(ui_LabelSettingsIcon, lv_color_hex(0x7DD3FC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_LabelSettingsIcon, &lv_font_montserrat_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  /* ===== Stats row (count · time/date · toggle all) ===== */
  create_stats_row();
  /* ===== Grid relay cuộn dọc ===== */
  relay_grid = lv_obj_create(ui_ScreenMain);
  lv_obj_set_size(relay_grid, SCREEN_W, GRID_H);
  lv_obj_set_pos(relay_grid, 0, GRID_Y);
  lv_obj_set_scroll_dir(relay_grid, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(relay_grid, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(relay_grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(relay_grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_bg_opa(relay_grid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(relay_grid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(relay_grid, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(relay_grid, RELAY_GRID_PAD_X,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(relay_grid, RELAY_GRID_PAD_X,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(relay_grid, RELAY_GRID_PAD_TOP,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(relay_grid, RELAY_GRID_PAD_BOTTOM,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(relay_grid, RELAY_GRID_GAP,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(relay_grid, RELAY_GRID_GAP,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(relay_grid, lv_color_hex(0x0EA5E9),
                            LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(relay_grid, 150,
                          LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
  lv_obj_set_style_width(relay_grid, 4, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(relay_grid, 2, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);

  /* Lưu lại grid để các lần grow card sau (RS485 discovery thêm board) còn
   * gắn được vào đúng parent. Index 0 = toggle all trên stats row; grid chỉ
   * render relay vật lý 1..N. */
  relay_grid_obj = relay_grid;

  /* ===== Đồng bộ dữ liệu runtime trước khi tạo card =====
   * Khi vào đây discovery RS485 có thể đã chạy xong (chạy trong app init);
   * refresh để chốt active_count rồi mới tạo đúng số card cần thiết. */
  if (ks_relay_service_refresh_cached_states() != 0) {
    fprintf(stderr,
            "Không thể đồng bộ trạng thái relay khi dựng màn hình chính: %s\n",
            ks_relay_service_get_last_error());
  }

  ensure_relay_cards_created(ks_relay_service_get_active_count());
  ui_refresh_relay_card_states();
  create_main_lock_overlay();
  main_refresh_lock_overlay();

  lv_obj_add_event_cb(ui_ButtonSettings, ui_event_ButtonSettings, LV_EVENT_ALL,
                      NULL);
}
