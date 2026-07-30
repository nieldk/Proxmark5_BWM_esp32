#ifndef WIFI_CONNECT_
#define WIFI_CONNECT_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_netif.h"


// Enumeration of WiFi connection event callback types
typedef enum {
    APP_WIFI_CONNECT_CALLBACK_GOTIP,
    APP_WIFI_CONNECT_CALLBACK_DISCONN,
} app_wifi_connect_callback_type_t;

// Callback invoked when WiFi connects successfully and an IP address is obtained
typedef void (*app_wifi_connect_callback_gotip_t)(bool ip_changed);
// Callback invoked when WiFi disconnects
typedef void (*app_wifi_disconnect_callback_t)(void);

esp_err_t app_wifi_connect_init(void);
esp_err_t app_wifi_connect_deinit(void);
esp_err_t app_wifi_connect_start(void);
esp_err_t app_wifi_connect_stop(void);

esp_err_t app_wifi_connect_wait_finish(uint8_t timeout, uint8_t *result, uint8_t *err_reason);
esp_err_t app_wifi_connect_get_status(uint8_t *status);
esp_err_t app_wifi_connect_set_callback(app_wifi_connect_callback_type_t type, void* callback);

esp_err_t app_wifi_connect_set_ssid(const char* ssid, uint8_t length);
esp_err_t app_wifi_connect_set_password(const char* password, uint8_t length);
esp_err_t app_wifi_connect_set_bssid(const uint8_t* bssid, uint8_t length);
esp_err_t app_wifi_connect_set_authmode(uint8_t authmode);
esp_err_t app_wifi_connect_set_listen_interval(uint16_t listen_interval);
esp_err_t app_wifi_connect_set_scan_mode(uint8_t scan_mode);
esp_err_t app_wifi_connect_set_pmf_mode(uint8_t pmf_mode);
esp_err_t app_wifi_connect_set_reconnect_interval(uint16_t interval);

esp_err_t app_wifi_connect_get_ssid(char* ssid, uint8_t* length);
esp_err_t app_wifi_connect_get_password(char* password, uint8_t* length);
esp_err_t app_wifi_connect_get_bssid(uint8_t* bssid, uint8_t* length, uint8_t* bssid_set);
esp_err_t app_wifi_connect_get_authmode(uint8_t* authmode);
esp_err_t app_wifi_connect_get_listen_interval(uint16_t* listen_interval);
esp_err_t app_wifi_connect_get_scan_mode(uint8_t* scan_mode);
esp_err_t app_wifi_connect_get_pmf_mode(uint8_t* pmf_mode);
esp_err_t app_wifi_connect_get_reconnect_interval(uint16_t* interval);

esp_err_t app_wifi_connect_restore_reconnect_interval(void);

#endif
