#ifndef APP_NVS_RW_
#define APP_NVS_RW_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

// Define enum to represent supported NVS data types
typedef enum {
    APP_NVS_RW_TYPE_I8 = 0U,
    APP_NVS_RW_TYPE_U8,
    APP_NVS_RW_TYPE_I16,
    APP_NVS_RW_TYPE_U16,
    APP_NVS_RW_TYPE_I32,
    APP_NVS_RW_TYPE_U32,
    APP_NVS_RW_TYPE_I64,
    APP_NVS_RW_TYPE_U64,
    APP_NVS_RW_TYPE_STR,
    APP_NVS_RW_TYPE_BLOB,
    APP_NVS_RW_TYPE_MAX,
} app_nvs_rw_type_t;

// Read item structure: contains key, type, output buffer, default value, etc.
typedef struct {
    const char *key;            // NVS key name, **must** end with '\0', length (excluding '\0') must be within NVS_KEY_NAME_MAX_SIZE
    app_nvs_rw_type_t type;     // Data type, determines how data is interpreted
    void *data;                 // Numeric type: point directly to output variable (e.g. uint16_t *)
                                // STR/BLOB type: point to external pointer variable (void **), library auto-allocates and writes pointer to *data, caller must free after successful read
    size_t out_len;             // Actual data length read
    uint64_t default_value;     // Numeric default value (only for numeric types, STR/BLOB ignored)
} app_nvs_rw_read_item_t;

// Write item structure: contains key, type, input data pointer and length
typedef struct {
    const char *key;            // NVS key name, **must** end with '\0', length (excluding '\0') must be within NVS_KEY_NAME_MAX_SIZE
    app_nvs_rw_type_t type;     // Data type, determines how data is interpreted
    const void *data;           // Input data pointer, must point to valid memory, length specified by length field
    size_t length;              // Input data length
} app_nvs_rw_write_item_t;


esp_err_t app_nvs_rw_read(const char *namespace_name, app_nvs_rw_read_item_t *items, size_t item_count);
esp_err_t app_nvs_rw_write(const char *namespace_name, const app_nvs_rw_write_item_t *items, size_t item_count);
esp_err_t app_nvs_rw_erase(const char *namespace_name, const char *key);


/**
 * @brief Auto-calculate items array element count to simplify read calls; note items must be array not pointer
 * 
 * @param namespace_name NVS namespace
 * @param items Read/write item array, must be array not pointer for sizeof to calculate element count
 */
#define APP_NVS_RW_READ(namespace_name, items) \
    app_nvs_rw_read(namespace_name, items, sizeof(items) / sizeof(items[0]))
    

/**
 * @brief Auto-calculate items array element count to simplify write calls; note items must be array not pointer
 * 
 * @param namespace_name NVS namespace
 * @param items Read/write item array, must be array not pointer for sizeof to calculate element count
 */
#define APP_NVS_RW_WRITE(namespace_name, items) \
    app_nvs_rw_write(namespace_name, items, sizeof(items) / sizeof(items[0]))

#endif
