#ifndef APP_LOG_UART_H
#define APP_LOG_UART_H

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_system.h"
#include "esp_log.h"

// Callback function for log forwarding to UART TX implementation
// After setting via app_log_uart_set_tx_callback, when calling ESP_LOGx() to output logs
// Will call this callback to send log data; callback must send log data to instruction UART
// Parameter data is log data pointer, length is log data length
typedef void (*app_log_uart_tx_callback_t)(uint8_t *data, uint16_t length);


esp_err_t app_log_uart_init(void);
esp_err_t app_log_uart_deinit(void);
esp_err_t app_log_uart_start(void);
esp_err_t app_log_uart_stop(void);
esp_err_t app_log_uart_get_status(uint8_t *status);
esp_err_t app_log_uart_set_tx_callback(app_log_uart_tx_callback_t callback);


#endif // APP_LOG_UART_H
