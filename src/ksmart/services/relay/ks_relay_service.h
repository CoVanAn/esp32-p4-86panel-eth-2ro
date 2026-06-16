#ifndef KS_RELAY_SERVICE_H
#define KS_RELAY_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "../../core/ks_app_config.h"

#define KS_RELAY_TOTAL_COUNT (KS_GPIO_RELAY_COUNT + KS_RS485_RELAY_COUNT)

    /* Khởi tạo relay GPIO cục bộ và đồng bộ trạng thái cache ban đầu cho toàn bộ relay. */
    void ks_relay_service_init(void);

    /* Ghi trạng thái mong muốn cho relay theo chỉ số UI 0..(KS_RELAY_TOTAL_COUNT-1). */
    int ks_relay_service_set(int relay_index, bool enabled);

    /* Đảo trạng thái relay theo cache hiện tại và trả về trạng thái mới nếu thành công. */
    int ks_relay_service_toggle(int relay_index, bool *enabled_after);

    /* Đọc lại trạng thái thực tế của GPIO và relay RS485 để cập nhật cache. */
    int ks_relay_service_refresh_cached_states(void);

    /* Lấy trạng thái cache hiện tại để lớp UI dựng lại giao diện. */
    bool ks_relay_service_get_cached_state(int relay_index);

    /* Trả về tổng số relay thực tế đang hoạt động (đã qua giới hạn của người dùng). */
    int ks_relay_service_get_active_count(void);

    /* Trả về tổng số relay thực tế phần cứng đang có (gồm GPIO và số lượng RS485 dò được). */
    int ks_relay_service_get_physical_count(void);

    /* Trả về offset của relay (chỉ số phần cứng tổng) cho một slave_id cụ thể (0 cho Luckfox). */
    int ks_relay_service_get_board_offset(int slave_id);

    /* Trả về thông điệp lỗi cuối cùng của service relay. */
    const char *ks_relay_service_get_last_error(void);

    /* Trả về số lượng board RS485 đang hoạt động. */
    int ks_relay_service_get_board_count(void);

    /* Lấy thông tin chi tiết của một board RS485. */
    int ks_relay_service_get_board_info(int board_idx, uint8_t *slave_id, int *relay_count, int *rs485_offset);

    /* Lưu lại bản đồ relay display names từ runtime vào file JSON. */
    int ks_relay_service_save_to_json(const char *path);

    /* Bật/Tắt tất cả relay (GPIO + RS485). */
    int ks_relay_service_set_all(bool enabled);

    /* Đổi ID của board RS485 cả trên phần cứng (Modbus) và phần mềm (file JSON). */
    int ks_relay_service_change_slave_id(uint8_t old_id, uint8_t new_id);

    /* Khởi động lại thiết bị (ESP32). */
    void ks_device_reboot(void);

    /* Quét và phục hồi baud rate mạch RS485 */
    int ks_relay_service_recover_rs485(void);

    /* VFS Wrappers cho file tĩnh trên LittleFS (để code C thuần dùng được). */
    char *ks_vfs_read_text_file(const char *path, size_t *out_size);
    long ks_vfs_get_mtime(const char *path);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

