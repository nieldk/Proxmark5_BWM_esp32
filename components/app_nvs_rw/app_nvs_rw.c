#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "nvs.h"
#include "app_nvs_rw.h"


#define TAG "app_nvs_rw"


// Byte width mapping for numeric types, used to fill out_len after successful read
static size_t app_nvs_type_size(app_nvs_rw_type_t type) {
    switch (type) {
        case APP_NVS_RW_TYPE_I8:
        case APP_NVS_RW_TYPE_U8:
            return 1;
        case APP_NVS_RW_TYPE_I16:
        case APP_NVS_RW_TYPE_U16:
            return 2;
        case APP_NVS_RW_TYPE_I32:
        case APP_NVS_RW_TYPE_U32:
            return 4;
        case APP_NVS_RW_TYPE_I64:
        case APP_NVS_RW_TYPE_U64:
            return 8;
        default:
            return 0;
    }
}

// Unified fallback when key doesn't exist: fill numeric types with default_value, set *data to NULL for STR/BLOB
static esp_err_t app_nvs_apply_default(app_nvs_rw_read_item_t *item) {
    if ((item == NULL) || (item->data == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if ((item->type == APP_NVS_RW_TYPE_STR) || (item->type == APP_NVS_RW_TYPE_BLOB)) {
        *(void **)item->data = NULL;
        item->out_len = 0;
        return ESP_OK;
    }

    switch (item->type) {
        case APP_NVS_RW_TYPE_I8:  *(int8_t  *)item->data = (int8_t )item->default_value; break;
        case APP_NVS_RW_TYPE_U8:  *(uint8_t *)item->data = (uint8_t)item->default_value; break;
        case APP_NVS_RW_TYPE_I16: *(int16_t *)item->data = (int16_t)item->default_value; break;
        case APP_NVS_RW_TYPE_U16: *(uint16_t*)item->data = (uint16_t)item->default_value; break;
        case APP_NVS_RW_TYPE_I32: *(int32_t *)item->data = (int32_t)item->default_value; break;
        case APP_NVS_RW_TYPE_U32: *(uint32_t*)item->data = (uint32_t)item->default_value; break;
        case APP_NVS_RW_TYPE_I64: *(int64_t *)item->data = (int64_t)item->default_value; break;
        case APP_NVS_RW_TYPE_U64: *(uint64_t*)item->data = (uint64_t)item->default_value; break;
        default: return ESP_ERR_INVALID_ARG;
    }

    item->out_len = app_nvs_type_size(item->type);
    return ESP_OK;
}

// Read scalar types (i8/u8/i16/u16/i32/u32/i64/u64)
// Convention: if key doesn't exist, use default_value and return success
static esp_err_t app_nvs_read_number(nvs_handle_t handle, app_nvs_rw_type_t type, app_nvs_rw_read_item_t *item) {
    esp_err_t err = ESP_OK;

    if ((item == NULL) || (item->data == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    switch (type) {
        case APP_NVS_RW_TYPE_I8:
            err = nvs_get_i8(handle, item->key, (int8_t *)item->data);
            break;
        case APP_NVS_RW_TYPE_U8:
            err = nvs_get_u8(handle, item->key, (uint8_t *)item->data);
            break;
        case APP_NVS_RW_TYPE_I16:
            err = nvs_get_i16(handle, item->key, (int16_t *)item->data);
            break;
        case APP_NVS_RW_TYPE_U16:
            err = nvs_get_u16(handle, item->key, (uint16_t *)item->data);
            break;
        case APP_NVS_RW_TYPE_I32:
            err = nvs_get_i32(handle, item->key, (int32_t *)item->data);
            break;
        case APP_NVS_RW_TYPE_U32:
            err = nvs_get_u32(handle, item->key, (uint32_t *)item->data);
            break;
        case APP_NVS_RW_TYPE_I64:
            err = nvs_get_i64(handle, item->key, (int64_t *)item->data);
            break;
        case APP_NVS_RW_TYPE_U64:
            err = nvs_get_u64(handle, item->key, (uint64_t *)item->data);
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return app_nvs_apply_default(item);
    }

    if (err == ESP_OK) {
        item->out_len = app_nvs_type_size(type);
    }

    return err;
}

// Read string or blob
// data is treated as void ** (address of external pointer); library allocates memory and writes to *data, caller must free
// Convention:
// 1) key exists: probe length → malloc → read → write to *data, out_len = actual length
// 2) key doesn't exist: *data = NULL, out_len = 0, return success (STR/BLOB don't support defaults)
static esp_err_t app_nvs_read_string_or_blob(nvs_handle_t handle, app_nvs_rw_type_t type, app_nvs_rw_read_item_t *item) {
    esp_err_t err;
    size_t read_len = 0;
    void *buffer = NULL;
    void **out_ptr;

    if ((item == NULL) || (item->data == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    // data is address of external pointer variable (void **)
    out_ptr = (void **)item->data;

    // First probe required length
    if (type == APP_NVS_RW_TYPE_STR) {
        err = nvs_get_str(handle, item->key, NULL, &read_len);
    } else {
        err = nvs_get_blob(handle, item->key, NULL, &read_len);
    }

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return app_nvs_apply_default(item);
    }

    if (err != ESP_OK) {
        return err;
    }

    // Allocate based on probed length, then read actual data
    buffer = malloc(read_len);
    if (buffer == NULL) {
        return ESP_ERR_NO_MEM;
    }

    if (type == APP_NVS_RW_TYPE_STR) {
        err = nvs_get_str(handle, item->key, (char *)buffer, &read_len);
    } else {
        err = nvs_get_blob(handle, item->key, buffer, &read_len);
    }

    if (err != ESP_OK) {
        free(buffer);
        return err;
    }

    *out_ptr = buffer;
    item->out_len = read_len;
    return ESP_OK;
}

// Single read dispatch: enter scalar or string/blob read path based on type
static esp_err_t app_nvs_read_one(nvs_handle_t handle, app_nvs_rw_read_item_t *item) {
    if ((item == NULL) || (item->key == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (item->type >= APP_NVS_RW_TYPE_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if ((item->type == APP_NVS_RW_TYPE_STR) || (item->type == APP_NVS_RW_TYPE_BLOB)) {
        return app_nvs_read_string_or_blob(handle, item->type, item);
    }

    return app_nvs_read_number(handle, item->type, item);
}

// Single write dispatch: call corresponding nvs_set_xxx based on type
static esp_err_t app_nvs_write_one(nvs_handle_t handle, const app_nvs_rw_write_item_t *item) {
    if ((item == NULL) || (item->key == NULL) || (item->data == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (item->type >= APP_NVS_RW_TYPE_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    switch (item->type) {
        case APP_NVS_RW_TYPE_I8:
            return nvs_set_i8(handle, item->key, *(const int8_t *)item->data);
        case APP_NVS_RW_TYPE_U8:
            return nvs_set_u8(handle, item->key, *(const uint8_t *)item->data);
        case APP_NVS_RW_TYPE_I16:
            return nvs_set_i16(handle, item->key, *(const int16_t *)item->data);
        case APP_NVS_RW_TYPE_U16:
            return nvs_set_u16(handle, item->key, *(const uint16_t *)item->data);
        case APP_NVS_RW_TYPE_I32:
            return nvs_set_i32(handle, item->key, *(const int32_t *)item->data);
        case APP_NVS_RW_TYPE_U32:
            return nvs_set_u32(handle, item->key, *(const uint32_t *)item->data);
        case APP_NVS_RW_TYPE_I64:
            return nvs_set_i64(handle, item->key, *(const int64_t *)item->data);
        case APP_NVS_RW_TYPE_U64:
            return nvs_set_u64(handle, item->key, *(const uint64_t *)item->data);
        case APP_NVS_RW_TYPE_STR:
            return nvs_set_str(handle, item->key, (const char *)item->data);
        case APP_NVS_RW_TYPE_BLOB:
            return nvs_set_blob(handle, item->key, item->data, item->length);
        default:
            return ESP_ERR_INVALID_ARG;
    }
}

// Batch read interface: open namespace, read each item, abort immediately on failure
// Note: close handle even on failure to avoid resource leak
esp_err_t app_nvs_rw_read(const char *namespace_name, app_nvs_rw_read_item_t *items, size_t item_count) {
    esp_err_t err = ESP_OK;
    nvs_handle_t handle;

    if ((namespace_name == NULL) || (items == NULL) || (item_count == 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    err = nvs_open(namespace_name, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Namespace not created yet, equivalent to all keys don't exist, apply defaults to each item and return normally
        ESP_LOGW(TAG, "nvs_open(read): ns=%s not found, applying defaults", namespace_name);
        for (size_t i = 0; i < item_count; ++i) {
            items[i].out_len = 0;
            esp_err_t def_err = app_nvs_apply_default(&items[i]);
            if (def_err != ESP_OK) {
                return def_err;
            }
        }
        return ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(read) failed, ns=%s, err=%s", namespace_name, esp_err_to_name(err));
        return err;
    }

    for (size_t i = 0; i < item_count; ++i) {
        items[i].out_len = 0;
        err = app_nvs_read_one(handle, &items[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "read failed, key=%s, type=%d, err=%s", items[i].key, (int)items[i].type, esp_err_to_name(err));
            break;
        }
    }

    nvs_close(handle);
    return err;
}

// Batch write interface: open namespace, write each item, abort on failure
// Only commit when all entries write successfully
esp_err_t app_nvs_rw_write(const char *namespace_name, const app_nvs_rw_write_item_t *items, size_t item_count) {
    esp_err_t err = ESP_OK;
    nvs_handle_t handle;

    if ((namespace_name == NULL) || (items == NULL) || (item_count == 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(write) failed, ns=%s, err=%s", namespace_name, esp_err_to_name(err));
        return err;
    }

    for (size_t i = 0; i < item_count; ++i) {
        err = app_nvs_write_one(handle, &items[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "write failed, key=%s, type=%d, err=%s", items[i].key, (int)items[i].type, esp_err_to_name(err));
            break;
        }
    }

    if (err == ESP_OK) {
        err = nvs_commit(handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "nvs_commit failed, ns=%s, err=%s", namespace_name, esp_err_to_name(err));
        }
    }

    nvs_close(handle);
    return err;
}

// Interface to erase specified key: open namespace, erase key, commit changes, close handle
esp_err_t app_nvs_rw_erase(const char *namespace_name, const char *key) {
    esp_err_t err = ESP_OK;
    nvs_handle_t handle;

    if ((namespace_name == NULL) || (key == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(erase) failed, ns=%s, err=%s", namespace_name, esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_key(handle, key);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = ESP_OK;
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "nvs_commit failed, ns=%s, err=%s", namespace_name, esp_err_to_name(err));
        }
    }

    nvs_close(handle);
    return err;
}
