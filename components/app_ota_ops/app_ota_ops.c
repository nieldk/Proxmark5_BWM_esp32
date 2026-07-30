#include "app_ota_ops.h"

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"


typedef struct {
    esp_ota_handle_t handle;
    const esp_partition_t *partition;
    uint32_t total_size;
    uint32_t written_size;
    bool active;
} app_ota_ctx_t;

static const char *TAG = "app_ota_ops";
static app_ota_ctx_t s_ota_ctx = { 0 };

/**
 * @brief Reset OTA context state, clear all fields, restore to initial state
 * 
 */
static inline void app_ota_reset_context(void) {
    memset(&s_ota_ctx, 0, sizeof(s_ota_ctx));
}

/**
 * @brief Abort current OTA operation and reset context state, if no OTA in progress just reset context
 * 
 * @return esp_err_t 
 */
static esp_err_t app_ota_abort_and_reset(void) {
    esp_err_t err = ESP_OK;

    if (s_ota_ctx.active) {
        err = esp_ota_abort(s_ota_ctx.handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_abort failed: %s", esp_err_to_name(err));
        }
    }

    app_ota_reset_context();
    return err;
}

/**
 * @brief Start OTA operation, prepare to receive new firmware data and write to OTA partition
 * Note: this wrapper doesn't support resumable transfer; failure requires restart
 * 
 * @param firmware_size Total size of the new firmware image in bytes
 * @return esp_err_t Returns ESP_OK when OTA starts successfully; otherwise returns a corresponding error code
 */
esp_err_t app_ota_begin(uint32_t firmware_size) {
    esp_err_t err;

    // At least check firmware size, we don't allow 0 size as we need it to judge if write is complete
    if (firmware_size == 0) {
        return ESP_ERR_INVALID_SIZE;
    }

    // If OTA already started, can call esp_ota_abort to abort previous operation, ensure clean state, then start new one
    if (s_ota_ctx.active) {
        err = app_ota_abort_and_reset();
        if (err != ESP_OK) {
            return err;
        }
    }

    // Get next available OTA update partition, usually OTA_0 or OTA_1, depends on which partition is running
    // If OTA_0 is running, this function returns OTA_1 partition info
    // If OTA_1 is running, this function returns OTA_0 partition info
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (partition == NULL) {
        ESP_LOGE(TAG, "No OTA update partition found, please check partition table");
        return ESP_ERR_NOT_FOUND;
    }

    // Call esp_ota_begin to start OTA, pass target partition info and firmware size, function returns OTA handle for subsequent writes
    esp_ota_handle_t ota_handle = 0;
    err = esp_ota_begin(partition, firmware_size, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        return err;
    }

    // Record OTA context state: OTA handle, target partition info, total firmware size, written size (initially 0), active state
    s_ota_ctx.handle = ota_handle;
    s_ota_ctx.partition = partition;
    s_ota_ctx.total_size = firmware_size;
    s_ota_ctx.written_size = 0;
    s_ota_ctx.active = true;

    ESP_LOGI(TAG, "OTA begin on partition %s, size=%" PRIu32, partition->label, firmware_size);
    return ESP_OK;
}

/**
 * @brief Write OTA firmware data chunks to target partition, can be called multiple times until write complete
 * Note: no need to auto-pad firmware data, write as-is, lower-level OTA interface handles alignment and chunking
 * 
 * @param data Pointer to firmware chunk data to write, must not be NULL
 * @param length Length of firmware chunk, must be > 0 and not exceed remaining firmware size
 * @return esp_err_t Returns ESP_OK on success; otherwise returns a corresponding error code
 */
esp_err_t app_ota_write(const uint8_t *data, size_t length) {
    // Check if input data is valid
    if (data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    // Check if OTA has been started
    if (!s_ota_ctx.active) {
        return ESP_ERR_INVALID_STATE;
    }
    // Check if write would exceed total firmware size to prevent writing too much data
    if (length > (size_t)(s_ota_ctx.total_size - s_ota_ctx.written_size)) {
        ESP_LOGE(TAG, "OTA write overflow, written=%" PRIu32 ", incoming=%u, total=%" PRIu32,
                 s_ota_ctx.written_size, (unsigned int)length, s_ota_ctx.total_size);
        return ESP_ERR_INVALID_SIZE;
    }
    // Call esp_ota_write to write the chunk to the target partition
    esp_err_t err = esp_ota_write(s_ota_ctx.handle, data, length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
        return err;
    }
    // Update written size statistics to help judge if write is complete and monitor progress
    s_ota_ctx.written_size += (uint32_t)length;
    return ESP_OK;
}

/**
 * @brief End OTA operation, complete firmware write and set next boot partition
 * 
 * @return esp_err_t Returns ESP_OK when OTA is finalized successfully; otherwise returns a corresponding error code
 */
esp_err_t app_ota_end(void) {
    // If OTA not started, nothing to end, return error
    if (!s_ota_ctx.active) {
        return ESP_ERR_INVALID_STATE;
    }
    // Want to end but firmware not fully transferred? Host may have lost packets? Or other issues? Then we abort OTA and retry
    if (s_ota_ctx.written_size != s_ota_ctx.total_size) {
        ESP_LOGE(TAG, "OTA size mismatch, written=%" PRIu32 ", expected=%" PRIu32,
                 s_ota_ctx.written_size, s_ota_ctx.total_size);
        app_ota_abort_and_reset();
        return ESP_ERR_INVALID_SIZE;
    }
    // Call esp_ota_end to end OTA, pass OTA handle, function completes firmware write and closes handle
    esp_err_t err = esp_ota_end(s_ota_ctx.handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        app_ota_reset_context();
        return err;
    }
    // Call esp_ota_set_boot_partition to set next boot partition to newly written partition, device will boot from new partition after restart
    err = esp_ota_set_boot_partition(s_ota_ctx.partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        app_ota_reset_context();
        return err;
    }
    // By here OTA is OK, we can safely boot from new partition, don't forget to reset context
    // After esp_ota_end succeeds, s_ota_ctx.handle is already released
    ESP_LOGI(TAG, "OTA image stored in %s and marked for next boot", s_ota_ctx.partition->label);
    app_ota_reset_context();
    return ESP_OK;
}
