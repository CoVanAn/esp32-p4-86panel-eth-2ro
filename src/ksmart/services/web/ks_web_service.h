#ifndef KS_WEB_SERVICE_H
#define KS_WEB_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Khởi tạo Web Server cho màn hình ESP32. Sẽ mở cổng 80 và chạy trên một Task riêng biệt */
void ks_web_service_init(void);

#ifdef __cplusplus
}
#endif

#endif // KS_WEB_SERVICE_H
