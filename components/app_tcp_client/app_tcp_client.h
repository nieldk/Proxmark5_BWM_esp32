#ifndef APP_TCP_CLIENT_H_
#define APP_TCP_CLIENT_H_

#include <stdint.h>
#include "esp_err.h"

// Data receive callback
typedef void (*app_tcp_client_rx_callback_t)(uint8_t *data, uint16_t length);

esp_err_t app_tcp_client_init(void);
esp_err_t app_tcp_client_deinit(void);
esp_err_t app_tcp_client_start(void);
esp_err_t app_tcp_client_stop(void);

esp_err_t app_tcp_client_send(uint8_t *data, size_t length);
esp_err_t app_tcp_client_set_rx_callback(app_tcp_client_rx_callback_t callback);
esp_err_t app_tcp_client_get_state(uint8_t *state);

esp_err_t app_tcp_client_set_ip(const char *ip_str);
esp_err_t app_tcp_client_set_port(uint16_t port);
esp_err_t app_tcp_client_set_so_linger(int so_linger);
esp_err_t app_tcp_client_set_tcp_nodelay(uint8_t enable);
esp_err_t app_tcp_client_set_so_sndtimeo(int sndtimeo);
esp_err_t app_tcp_client_set_keep_alive(uint8_t enable, int keep_idle, int keep_interval, int keep_count);

esp_err_t app_tcp_client_get_ip(char *ip_buf, size_t buf_len);
esp_err_t app_tcp_client_get_port(uint16_t *port);
esp_err_t app_tcp_client_get_so_linger(int *so_linger);
esp_err_t app_tcp_client_get_tcp_nodelay(uint8_t *enable);
esp_err_t app_tcp_client_get_so_sndtimeo(int *sndtimeo);
esp_err_t app_tcp_client_get_keep_alive(uint8_t *enable, int *keep_idle, int *keep_interval, int *keep_count);

#endif /* APP_TCP_CLIENT_H_ */
