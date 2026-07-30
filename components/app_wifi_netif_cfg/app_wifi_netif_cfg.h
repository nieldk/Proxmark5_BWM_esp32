#ifndef WIFI_CFG_
#define WIFI_CFG_

#include "esp_err.h"
#include "esp_wifi.h"

esp_err_t app_wifi_cfg_get_dhcp_status(esp_netif_dhcp_status_t *status);
esp_err_t app_wifi_cfg_set_dhcp_enable(bool enable);
esp_err_t app_wifi_cfg_get_ipv4(uint32_t *ipaddr, uint32_t *gwaddr, uint32_t *netmask);
esp_err_t app_wifi_cfg_set_ipv4(uint32_t ipaddr, uint32_t gwaddr, uint32_t netmask);
esp_err_t app_wifi_cfg_get_host_name(const char **hostname);
esp_err_t app_wifi_cfg_set_host_name(const char *hostname);

#endif
