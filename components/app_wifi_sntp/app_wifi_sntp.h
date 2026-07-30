#ifndef APP_WIFI_SNTP_H_
#define APP_WIFI_SNTP_H_

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t app_wifi_sntp_init(void);
esp_err_t app_wifi_sntp_deinit(void);

esp_err_t app_wifi_sntp_start(void);
esp_err_t app_wifi_sntp_stop(void);

esp_err_t app_wifi_sntp_set_enable(uint8_t enable);
esp_err_t app_wifi_sntp_get_enable(uint8_t *enable);

esp_err_t app_wifi_sntp_set_server(const char *server);
esp_err_t app_wifi_sntp_get_server(const char **server);

esp_err_t app_wifi_sntp_set_interval(uint32_t interval_ms);
esp_err_t app_wifi_sntp_get_interval(uint32_t *interval_ms);

esp_err_t app_wifi_sntp_get_sync_status(uint8_t *status);

#endif // APP_WIFI_SNTP_H_
