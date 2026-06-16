#ifndef KS_TIME_SERVICE_H
#define KS_TIME_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

    int ks_time_service_sync_ntp(void);
    const char *ks_time_service_get_last_error(void);
    void ks_time_service_refresh_labels(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
