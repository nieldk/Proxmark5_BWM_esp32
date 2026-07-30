#ifndef WIFI_SCAN_
#define WIFI_SCAN_

#include "esp_err.h"
#include "esp_wifi.h"

typedef struct __attribute__((packed)) {
    /**
     * @brief Encryption method
     * Has following values:
     *  0: OPEN
     *  1: WEP
     *  2: WPA_PSK
     *  3: WPA2_PSK
     *  4: WPA_WPA2_PSK
     *  5: WPA2_ENTERPRISE
     *  6: WPA3_PSK
     *  7: WPA2_WPA3_PSK
     *  8: WAPI_PSK
     *  9: OWE
     *  10: WPA3_ENT_192
     *  11: WPA3_EXT_PSK
     *  12: WPA3_EXT_PSK_MIXED_MODE
     *  13: DPP
     *  14: WPA3_ENTERPRISE
     *  15: WPA2_WPA3_ENTERPRISE
     */
    uint8_t ecn;
    /**
     * @brief AP device SSID
     */
    uint8_t ssid[33];
    /**
     * @brief Signal strength
     * Access point (AP) signal strength. Note in rare cases of very strong signal, RSSI value may be slightly positive
     */
    int8_t  rssi;
    /**
     * @brief MAC address; when SSID repeats, BSSID is important parameter for connection
     */
    uint8_t mac[6];
    /**
     * @brief Channel number
     */
    uint8_t channel;
    /**
     * @brief Pairwise cipher type
     * Has following values:
     * 0: None
     * 1: WEP40
     * 2: WEP104
     * 3: TKIP
     * 4: CCMP
     * 5: TKIP and CCMP
     * 6: AES-CMAC-128
    * 7: Unknown
     */
    uint8_t pairwise_cipher;
    /**
     * @brief Group cipher type, same enum values as <pairwise_cipher> parameter
     */
    uint8_t group_cipher;
    /**
     * @brief Wi-Fi protocol standard; bit=1 means mode enabled, 0 means disabled
     *  bit 0: 802.11b mode enabled
     *  bit 1: 802.11g mode enabled
     *  bit 2: 802.11n mode enabled
     *  bit 3: 802.11 LR Espressif proprietary protocol
     *  bit 4: 802.11ax protocol standard
     *  bit 5: 802.11a protocol standard
     *  bit 6: 802.11ac protocol standard
     * Remaining 9 bits reserved
     */
    uint16_t is11b: 1;
    uint16_t is11g: 1;
    uint16_t is11n: 1;
    uint16_t is11lr: 1;
    uint16_t is11ax: 1;
    uint16_t is11a: 1;
    uint16_t is11ac: 1;
    uint16_t reserved: 9;
    /**
     * @brief WPS flag
     * 0: WPS not supported
     * 1: WPS supported
     */
    uint8_t wps;
} wifi_scan_result_t;

/*
 * Scan callback 
 */
typedef void (*app_wifi_scanner_callback_t)(wifi_scan_result_t *result, uint8_t count);

esp_err_t app_wifi_scanner_init(void);
esp_err_t app_wifi_scanner_start(void);
esp_err_t app_wifi_scanner_stop(void);
esp_err_t app_wifi_scanner_deinit(void);
esp_err_t app_wifi_scanner_get_scan_config(wifi_scan_config_t **pcfg);
uint8_t app_wifi_scanner_get_status(void);
void app_wifi_scanner_set_callback(app_wifi_scanner_callback_t callback);

#endif
