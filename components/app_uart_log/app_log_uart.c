#include <stdlib.h>
#include "app_log_uart.h"


typedef struct {
    volatile bool logging_in_progress;      // Whether currently outputting logs
    char *log_buffer;                       // Log formatting buffer, dynamically allocated at start
    volatile bool log_uart_started;         // Whether log forwarding to instruction UART has been started
    app_log_uart_tx_callback_t tx_callback; // Log forwarding callback function
} app_log_uart_ctx_t;

static app_log_uart_ctx_t *s_ctx = NULL;


/**
 * @brief Custom vprintf function to send logs to instruction UART
 * Also call original vprintf to output logs to default terminal as backup, avoid losing logs completely
 * 
 * @param fmt Format string
 * @param args Variadic argument list
 * @return int Number of output characters
 */
static int custom_vprintf(const char *fmt, va_list args) {
    // Prevent recursive calls and check if log forwarding started; if not, call original vprintf to output logs
    if (s_ctx == NULL || s_ctx->logging_in_progress || !s_ctx->log_uart_started) {
        // If already printing and lower layer triggers log, discard to avoid infinite loop
        return vprintf(fmt, args); // Directly call original vprintf to output logs, print logs on default terminal, at least there's a backup output, avoid completely losing logs
    }
    // Set flag to indicate currently outputting logs
    s_ctx->logging_in_progress = true;
    // Format string
    int len = vsnprintf(s_ctx->log_buffer, CONFIG_CMD_PAYLOAD_SIZE, fmt, args);
    if (len > 0) {
        // Limit length to not exceed buffer
        if (len >= CONFIG_CMD_PAYLOAD_SIZE) {
            len = CONFIG_CMD_PAYLOAD_SIZE - 1;
        }
        // Call callback to send log data to instruction UART
        if (s_ctx->tx_callback) {
            s_ctx->tx_callback((uint8_t*)s_ctx->log_buffer, len);
        }
    }
    // Finally clear flag
    s_ctx->logging_in_progress = false;
    return vprintf(fmt, args); // Continue calling original vprintf to output logs to default terminal as backup, avoid losing logs completely
}

/**
 * @brief Initialize log forwarding module, replace default log output function with custom function
 * 
 * @return esp_err_t 
 */
esp_err_t app_log_uart_init(void) {
    if (s_ctx != NULL) {
        return ESP_OK; // Already initialized
    }

    s_ctx = malloc(sizeof(app_log_uart_ctx_t));
    if (s_ctx == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_ctx->logging_in_progress = false;
    s_ctx->log_buffer = NULL;       // Buffer allocated at start
    s_ctx->log_uart_started = false;
    s_ctx->tx_callback = NULL;

    // Replace default log output function with custom function
    esp_log_set_vprintf(custom_vprintf);
    return ESP_OK;
}

/**
 * @brief Deinitialize log forwarding module, restore default log output to original vprintf
 * 
 * @return esp_err_t 
 */
esp_err_t app_log_uart_deinit(void) {
    if (s_ctx == NULL) {
        return ESP_OK;
    }

    // Stop log forwarding
    if (s_ctx->log_uart_started) {
        app_log_uart_stop();
    }

    // Restore default log output function
    esp_log_set_vprintf(vprintf);

    // Release context
    free(s_ctx);
    s_ctx = NULL;
    return ESP_OK;
}

/**
 * @brief Start log forwarding to instruction UART, logs will be forwarded to instruction UART afterwards
 * 
 * @return esp_err_t 
 */
esp_err_t app_log_uart_start(void) {
    if (s_ctx == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_ctx->log_uart_started) {
        return ESP_OK; // Already started
    }

    // Allocate log_buffer at startup
    s_ctx->log_buffer = malloc(CONFIG_CMD_PAYLOAD_SIZE);
    if (s_ctx->log_buffer == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_ctx->log_uart_started = true;
    return ESP_OK;
}

/**
 * @brief Stop log forwarding to instruction UART, logs won't be forwarded afterwards
 * 
 * @return esp_err_t 
 */
esp_err_t app_log_uart_stop(void) {
    if (s_ctx == NULL) {
        return ESP_OK;
    }

    s_ctx->log_uart_started = false;

    // Free log_buffer at stop
    if (s_ctx->log_buffer != NULL) {
        free(s_ctx->log_buffer);
        s_ctx->log_buffer = NULL;
    }

    return ESP_OK;
}

/**
 * @brief Get log forwarding to instruction UART enable status; status=1 means started, 0 means not started
 * 
 * @param status 
 * @return esp_err_t 
 */
esp_err_t app_log_uart_get_status(uint8_t *status) {
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ctx == NULL) {
        *status = 0;
        return ESP_OK;
    }
    *status = s_ctx->log_uart_started ? 1 : 0;
    return ESP_OK;
}

/**
 * @brief Set log forwarding to instruction UART send callback
 * 
 * @param callback Callback function pointer
 * @return esp_err_t 
 */
esp_err_t app_log_uart_set_tx_callback(app_log_uart_tx_callback_t callback) {
    if (s_ctx == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    s_ctx->tx_callback = callback;
    return ESP_OK;
}
