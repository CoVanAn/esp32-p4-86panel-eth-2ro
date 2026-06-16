/*********************
 *      INCLUDES
 *********************/
#include "ui.h"
#include "ui_custom.h"
#include "app/ks_app_runtime.h"
#include "services/relay/ks_relay_service.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *  GLOBAL VARIABLES
 *********************/
/* Model thiết bị đọc từ /proc/device-tree/model */
char device_model[32];



/* Tên mặc định cho card UI: index 0 là card "Tất cả", còn lại là "Relay N".
 * Dùng buffer static để có thể trả con trỏ. Tên thật do
 * RELAY_NAME_JSON_PATH/RELAY_NAME_CONFIG_PATH ghi đè ở runtime. */
const char *ui_get_relay_default_title(int relay_index)
{
    static char fallback_titles[KS_UI_RELAY_TOTAL_COUNT][16];
    static bool fallback_ready = false;

    if (!fallback_ready)
    {
        int index;

        snprintf(fallback_titles[0], sizeof(fallback_titles[0]), "%s", "Tất cả");
        for (index = 1; index < KS_UI_RELAY_TOTAL_COUNT; index++)
        {
            snprintf(fallback_titles[index], sizeof(fallback_titles[index]), "Relay %d", index);
        }
        fallback_ready = true;
    }

    if (relay_index < 0 || relay_index >= KS_UI_RELAY_TOTAL_COUNT)
    {
        return "";
    }

    return fallback_titles[relay_index];
}

const char *ui_get_relay_source(int relay_index)
{
    if (relay_index < 0 || relay_index >= KS_UI_RELAY_TOTAL_COUNT)
    {
        return "";
    }

    if (relay_index == 0)
    {
        return "ALL";
    }

    if (relay_index <= KS_GPIO_RELAY_COUNT)
    {
        return "GPIO";
    }

    return "RS485";
}

/*SCREEN: ui_ScreenMain*/
lv_obj_t *ui_ScreenMain;
lv_obj_t *ui_ImageHeaderNetwork;
lv_obj_t *ui_PanelHeaderNetworkDot;
lv_obj_t *ui_LabelHeaderNetworkStatus;
lv_obj_t *ui_ButtonSettings;
lv_obj_t *ui_LabelSettingsIcon;
lv_obj_t *ui_LabelDate;
lv_obj_t *ui_LabelTime;
lv_obj_t *ui_PanelRelay0;
lv_obj_t *ui_LabelRelay0;
lv_obj_t *ui_PanelRelay1;
lv_obj_t *ui_LabelRelay1;
lv_obj_t *ui_LabelLogo;
lv_obj_t *ui_LabelHeaderDeviceName;

/*SCREEN: ui_ScreenSettings*/
lv_obj_t *ui_ScreenSettings;
lv_obj_t *ui_ScreenSettingsNetwork;
lv_obj_t *ui_ScreenSettingsCh;

lv_obj_t *ui_PanelSettingsHeader;
lv_obj_t *ui_ButtonSettingsBack;
lv_obj_t *ui_LabelSettingsBack;
lv_obj_t *ui_LabelSettingsTitle;
lv_obj_t *ui_PanelWifi;
lv_obj_t *ui_ImageWifi;
lv_obj_t *ui_LabelWifiName;
lv_obj_t *ui_LabelWifiIP;
lv_obj_t *ui_LabelWIP;
lv_obj_t *ui_PanelEth;
lv_obj_t *ui_ImageEth;
lv_obj_t *ui_LabelEth;
lv_obj_t *ui_LabelEthIP;
lv_obj_t *ui_LabelNetIP;

/*SCREEN: ui_ScreenWpa*/
lv_obj_t *ui_ScreenWpa;
lv_obj_t *ui_PanelList;
lv_obj_t *ui_DropdownSSID;
lv_obj_t *ui_LabelWLAN;
lv_obj_t *ui_PanelBtn;
lv_obj_t *ui_ButtonDiscon;
lv_obj_t *ui_ImageDiscon;
lv_obj_t *ui_ButtonScan;
lv_obj_t *ui_ImageScan;
lv_obj_t *ui_ButtonBack;
lv_obj_t *ui_ImageBack;
lv_obj_t *ui_ButtonConnect;
lv_obj_t *ui_ImageConnect;
lv_obj_t *ui_LabelMGMT;
lv_obj_t *ui_LabelPW;
lv_obj_t *ui_TextAreaPW;
lv_obj_t *ui_LabelRSSI;
lv_obj_t *ui_LabelSSID;
lv_obj_t *ui_TextAreaSSID;
lv_obj_t *ui_TextAreaRSSI;
lv_obj_t *ui_TextAreaMgnt;
lv_obj_t *ui_Keyboard1;

lv_obj_t *ui_initial_actions0;

static bool scan_is_button_pressed = false;

/**********************
 *  STATIC FUNCTIONS
 **********************/
/* Bàn phím WPA được tạo muộn để lúc mở màn không phải dựng thêm object nặng ngoài nhu cầu thật. */
static void ensure_wpa_keyboard_ready(void)
{
    if (ui_ScreenWpa == NULL || ui_Keyboard1 != NULL)
    {
        return;
    }

    ui_Keyboard1 = lv_keyboard_create(ui_ScreenWpa);
    lv_obj_set_size(ui_Keyboard1, 720, 236);
    lv_obj_align(ui_Keyboard1, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(ui_Keyboard1, lv_color_hex(0x08131F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Keyboard1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Keyboard1, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Keyboard1, lv_color_hex(0x22384E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_Keyboard1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_Keyboard1, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Keyboard1, &lv_font_montserrat_24, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_keyboard_set_popovers(ui_Keyboard1, true);
    lv_obj_add_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(ui_Keyboard1, ui_event_Keyboard1, LV_EVENT_ALL, NULL);
}

/* Gom thao tác bật bàn phím về một chỗ để SSID và mật khẩu dùng cùng luồng hiển thị. */
static void show_wpa_keyboard_for(lv_obj_t *textarea)
{
    ensure_wpa_keyboard_ready();
    if (ui_Keyboard1 == NULL || textarea == NULL)
    {
        return;
    }

    lv_keyboard_set_textarea(ui_Keyboard1, textarea);
    lv_obj_clear_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ui_Keyboard1);
}

/* Đồng bộ dữ liệu WPA sau khi screen đã lên để tránh dồn thêm tác vụ lúc đang chuyển màn. */
static void wpa_prepare_screen_async(void *user_data)
{
    (void)user_data;

    if (ui_ScreenWpa == NULL || lv_scr_act() != ui_ScreenWpa)
    {
        return;
    }

    // wifi_scr_init();
}

/* Hai ô thông tin chỉ đọc ở màn WPA đã đổi sang label để giảm tải object khi init. */
static void wpa_set_status_text(lv_obj_t *label, const char *text)
{
    if (label == NULL)
    {
        return;
    }

    lv_label_set_text(label, text != NULL ? text : "");
}

bool ui_obj_is_ready(lv_obj_t *obj)
{
    return obj != NULL && lv_obj_is_valid(obj);
}

bool ui_screen_is_ready(lv_obj_t *screen)
{
    return ui_obj_is_ready(screen);
}

void ui_ensure_screen_ready(lv_obj_t **screen, void (*screen_init)(void))
{
    if (screen == NULL || screen_init == NULL)
    {
        return;
    }

    if (ui_screen_is_ready(*screen))
    {
        return;
    }

    *screen = NULL;
    screen_init();
}

/* Chuyển màn hình với animation và khởi tạo lại screen đích nếu cần. */
static void _ui_screen_change(lv_obj_t **target, lv_scr_load_anim_t fademode, int spd, int delay,
                              bool auto_delete_current, void (*target_init)(void))
{
    ui_ensure_screen_ready(target, target_init);
    if (!ui_screen_is_ready(*target))
    {
        return;
    }

    lv_scr_load_anim(*target, fademode, spd, delay, auto_delete_current);
}

static void toggle_relay_from_ui(int relay_index)
{
    int physical_relay_index;
    bool enabled_after = false;
    bool target_all_enabled = false;
    int index;

    if (relay_index < 0 || relay_index >= KS_UI_RELAY_TOTAL_COUNT)
    {
        return;
    }

    if (relay_index == 0)
    {
        /* Relay "Tất cả": nếu đang có relay nào tắt thì bật tất cả, ngược lại tắt tất cả. */
        int active_count = ks_relay_service_get_active_count();
        for (index = 0; index < active_count; index++)
        {
            if (!ks_relay_service_get_cached_state(index))
            {
                target_all_enabled = true;
                break;
            }
        }

        /* Relay "Tất cả" chạy tuần tự theo KS_RELAY_ALL_STEP_DELAY_SEC. */
        ks_app_runtime_schedule_all_relays_from_ui(target_all_enabled);
        return;
    }

    physical_relay_index = relay_index - 1;
    ks_app_runtime_cancel_all_relays();

    if (ks_relay_service_toggle(physical_relay_index, &enabled_after) != 0)
    {
        printf("Không thể điều khiển Relay %d: %s\n",
               physical_relay_index + 1, ks_relay_service_get_last_error());
        return;
    }

    ui_apply_relay_state_change(relay_index, enabled_after);
}

/**********************
 *  GLOBAL FUNCTIONS
 **********************/
/* Tắt gesture trên màn hình chính vì luồng mở player cũ không còn được sử dụng. */
void ui_event_ScreenMain(lv_event_t *e)
{
    (void)e;
}

/* Mở màn hình cài đặt khi nhấn nút bánh răng. */
void ui_event_ButtonSettings(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        ui_refresh_settings_screen_state();
        _ui_screen_change(&ui_ScreenSettings, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, true,
                          &ui_ScreenSettings_screen_init);
    }
}

/* Luồng mở nhanh từ chip trạng thái mạng đã bị tắt; màn network chỉ mở từ menu Cài đặt. */
void ui_event_HeaderNetworkChip(lv_event_t *e)
{
    (void)e;
}

/* Quay lại màn hình chính từ màn hình cài đặt. */
void ui_event_SettingsBack(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        _ui_screen_change(&ui_ScreenMain, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, true,
                          &ui_ScreenMain_screen_init);
    }
}

/* Gesture ở network settings đã bị tắt; điều hướng chỉ còn bằng nút/back từ menu Cài đặt. */
void ui_event_ScreenSettingsNetwork(lv_event_t *e)
{
    (void)e;
}

/* Mở màn hình cấu hình Wi-Fi và nạp sẵn thông tin đã lưu. */
void ui_event_PanelWifi(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        _ui_screen_change(&ui_ScreenWpa, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, true,
                          &ui_ScreenWpa_screen_init);
        lv_async_call(wpa_prepare_screen_async, NULL);
    }
}

/* Đồng bộ SSID đã chọn từ dropdown sang các ô thông tin chi tiết. */
void ui_event_DropdownSSID(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    /* Chỉ style list dropdown khi người dùng thực sự mở nó để tránh ép tạo object từ lúc init. */
    if (event_code == LV_EVENT_CLICKED)
    {
        lv_obj_t *dropdown_list = lv_dropdown_get_list(target);

        if (dropdown_list != NULL)
        {
            lv_obj_set_style_bg_color(dropdown_list, lv_color_hex(0x08131F),
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(dropdown_list, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(dropdown_list, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(dropdown_list, lv_color_hex(0x22384E),
                                          LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(dropdown_list, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(dropdown_list, lv_color_hex(0xF5FBFF),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(dropdown_list, &lv_font_montserrat_22,
                                       LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(dropdown_list, lv_color_hex(0xFFFFFF),
                                        LV_PART_SELECTED | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(dropdown_list, lv_color_hex(0x1E6B8F),
                                      LV_PART_SELECTED | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(dropdown_list, 255, LV_PART_SELECTED | LV_STATE_DEFAULT);
        }
    }

    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        char ssid[MAX_CONF_LEN];
        memset(ssid, 0, MAX_CONF_LEN);
        lv_dropdown_get_selected_str(ui_DropdownSSID, ssid, MAX_CONF_LEN);
        lv_textarea_set_text(ui_TextAreaSSID, ssid);
        wpa_set_status_text(ui_TextAreaMgnt, "---");
        wpa_set_status_text(ui_TextAreaRSSI, "-- dBm");

        /* Tìm bản ghi mạng tương ứng để hiển thị RSSI và kiểu bảo mật. */
        for (int i = 0; i < network_count; i++)
        {
            if (strcmp(networks[i].ssid, ssid) == 0)
            {
                char signal_level_str[20];
                snprintf(signal_level_str, sizeof(signal_level_str), "%d dBm", networks[i].signal_level);
                wpa_set_status_text(ui_TextAreaMgnt, networks[i].flags);
                wpa_set_status_text(ui_TextAreaRSSI, signal_level_str);
                break;
            }
        }
    }
}

/* Xóa thông tin hiển thị và yêu cầu ngắt kết nối Wi-Fi. */
void ui_event_ButtonDiscon(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_RELEASED)
    {
        lv_textarea_set_text(ui_TextAreaSSID, "");
        lv_textarea_set_text(ui_TextAreaPW, "");
        wpa_set_status_text(ui_TextAreaMgnt, "---");
        wpa_set_status_text(ui_TextAreaRSSI, "-- dBm");
        // wifi_disconnect("wlan0");
    }
}

/* Chống bấm scan lặp khi thao tác liên tiếp trên nút quét Wi-Fi. */
void ui_event_ButtonScan(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_RELEASED && scan_is_button_pressed == false)
    {
        scan_is_button_pressed = true;
        // wifi_scanning_ssid();
        scan_is_button_pressed = false;
    }
}

/* Quay lại từ WPA: ưu tiên về Settings (flow mới), fallback về Network nếu Settings chưa sẵn sàng. */
void ui_event_ButtonBack(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        if (ui_obj_is_ready(ui_Keyboard1))
        {
            lv_obj_add_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
        }

        // Luôn gọi ui_ScreenSettings_screen_init để đảm bảo component được dựng lại đúng layout template
        _ui_screen_change(&ui_ScreenSettings, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, true,
                          &ui_ScreenSettings_screen_init);
    }
}

/* Gắn bàn phím ảo cho ô nhập mật khẩu và hiển thị bàn phím. */
void ui_event_TextAreaPW(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        show_wpa_keyboard_for(ui_TextAreaPW);
    }
}

/* Gắn bàn phím ảo cho ô nhập SSID và hiển thị bàn phím. */
void ui_event_TextAreaSSID(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        show_wpa_keyboard_for(ui_TextAreaSSID);
    }
}

/* Ẩn bàn phím khi người dùng xác nhận hoặc hủy nhập liệu. */
void ui_event_Keyboard1(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_READY || event_code == LV_EVENT_CANCEL)
    {
        lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);
    }
}

/* Bật hoặc tắt Relay 0, đồng thời đổi giao diện để phản ánh trạng thái hiện tại. */
void ui_event_PanelRelay0(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        toggle_relay_from_ui(0);
    }
}

/* Bật hoặc tắt Relay 1, đồng thời đổi giao diện để phản ánh trạng thái hiện tại. */
void ui_event_PanelRelay1(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        toggle_relay_from_ui(1);
    }
}

/* Các relay RS485 dùng cùng callback và lấy chỉ số relay từ user_data của từng card. */
void ui_event_PanelRelayRs485(lv_event_t *e)
{
    const int *relay_index = (const int *)lv_event_get_user_data(e);
    lv_event_code_t event_code = lv_event_get_code(e);

    if (event_code == LV_EVENT_CLICKED && relay_index != NULL)
    {
        toggle_relay_from_ui(*relay_index);
    }
}

/* Khởi tạo theme, đọc model thiết bị, dựng các màn hình và nạp screen đầu tiên. */
void ui_init(void)
{
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                              false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    snprintf(device_model, sizeof(device_model), "ESP32-P4-86PANEL");

    /* Khởi tạo runtime trước để màn hình chính có thể dùng dữ liệu và callback nền ngay khi lên app. */
    custom_init();
    ui_ScreenMain_screen_init();

    /* Tạo object rỗng theo cấu trúc cũ của project rồi khởi động timer nền. */
    ui_initial_actions0 = lv_obj_create(NULL);
    main_timer_init();
    lv_disp_load_scr(ui_ScreenMain);
}
