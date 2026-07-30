#ifndef UDP_SERVER_H_
#define UDP_SERVER_H_

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lwip/sockets.h"

#define UDP_SERVER_IP_STR_LEN      64

typedef void (*app_udp_server_rx_callback_t)(uint8_t *data, uint16_t length);

esp_err_t app_udp_server_init(void);
esp_err_t app_udp_server_deinit(void);
esp_err_t app_udp_server_start(void);
esp_err_t app_udp_server_stop(void);

esp_err_t app_udp_server_send(uint8_t *data, size_t length);
esp_err_t app_udp_server_set_rx_callback(app_udp_server_rx_callback_t callback);
esp_err_t app_udp_server_get_state(uint8_t *state);

esp_err_t app_udp_server_set_ip_mode(uint8_t mode);
esp_err_t app_udp_server_get_ip_mode(uint8_t *mode);
esp_err_t app_udp_server_set_port(uint16_t port);
esp_err_t app_udp_server_get_port(uint16_t *port);
esp_err_t app_udp_server_set_so_sndtimeo(int sndtimeo);
esp_err_t app_udp_server_get_so_sndtimeo(int *sndtimeo);

esp_err_t app_udp_server_set_client_ip(const char *ip_str);
esp_err_t app_udp_server_get_client_ip(char *ip_str, size_t max_len);
esp_err_t app_udp_server_set_client_port(uint16_t port);
esp_err_t app_udp_server_get_client_port(uint16_t *port);

#endif
