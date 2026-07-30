#ifndef APP_OTA_OPS_H_
#define APP_OTA_OPS_H_

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t app_ota_begin(uint32_t firmware_size);
esp_err_t app_ota_write(const uint8_t *data, size_t length);
esp_err_t app_ota_end(void);

#endif
