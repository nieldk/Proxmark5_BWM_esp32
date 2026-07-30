#ifndef UDP_CLIENT_H_
#define UDP_CLIENT_H_

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lwip/sockets.h"

#define UDP_CLIENT_IP_STR_LEN      64

typedef void (*app_udp_client_rx_callback_t)(uint8_t *data, uint16_t length);

esp_err_t app_udp_client_init(void);
esp_err_t app_udp_client_deinit(void);
esp_err_t app_udp_client_start(void);
esp_err_t app_udp_client_stop(void);

esp_err_t app_udp_client_send(uint8_t *data, size_t length);
esp_err_t app_udp_client_set_rx_callback(app_udp_client_rx_callback_t callback);
esp_err_t app_udp_client_get_state(uint8_t *state);

esp_err_t app_udp_client_set_ip_mode(uint8_t mode);
esp_err_t app_udp_client_set_local_port(uint16_t port);
esp_err_t app_udp_client_set_so_sndtimeo(int sndtimeo);
esp_err_t app_udp_client_set_server_ip(const char *ip_str);
esp_err_t app_udp_client_set_server_port(uint16_t port);

esp_err_t app_udp_client_get_ip_mode(uint8_t *mode);
esp_err_t app_udp_client_get_local_port(uint16_t *port);
esp_err_t app_udp_client_get_so_sndtimeo(int *sndtimeo);
esp_err_t app_udp_client_get_server_ip(char *ip_str, size_t max_len);
esp_err_t app_udp_client_get_server_port(uint16_t *port);

#endif // UDP_CLIENT_H_
