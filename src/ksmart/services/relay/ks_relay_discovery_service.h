#ifndef KS_RELAY_DISCOVERY_SERVICE_H
#define KS_RELAY_DISCOVERY_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

    typedef struct
    {
        int slave_id;
        int relay_count;
    } ks_relay_discovery_item_t;

    /* Khởi tạo trạng thái service quét relay theo ID trên bus RS485. */
    void ks_relay_discovery_service_init(void);

    /* Quét dải ID mặc định trong cấu hình để lấy danh sách board relay đang phản hồi. */
    void ks_relay_discovery_service_start_default_scan(void);

    /* Bắt đầu một phiên quét ID mới; trả 0 nếu tạo được luồng quét nền. */
    int ks_relay_discovery_service_start_scan(int start_id, int end_id);

    /* Trả true khi service còn đang quét danh sách relay. */
    bool ks_relay_discovery_service_is_loading(void);

    /* Trả số lượng relay board đã phát hiện được ở lần quét gần nhất. */
    int ks_relay_discovery_service_get_result_count(void);

    /* Sao chép danh sách kết quả quét vào buffer caller truyền vào. */
    int ks_relay_discovery_service_copy_results(ks_relay_discovery_item_t *out_items, int max_items);

    /* Quét đồng bộ (blocking) dải ID để lấy danh sách relay board.
     * Trả về số item copy vào out_items (0..max_items) hoặc -1 nếu lỗi. */
    int ks_relay_discovery_service_scan_sync(int start_id, int end_id,
                                             ks_relay_discovery_item_t *out_items,
                                             int max_items);

    /* Trả về dải ID của phiên quét gần nhất để UI hiển thị ngữ cảnh. */
    void ks_relay_discovery_service_get_last_range(int *start_id, int *end_id);

    /* Trả về thông điệp lỗi cuối cùng nếu quét thất bại. */
    const char *ks_relay_discovery_service_get_last_error(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
