#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "app_wifi_scanner.h"
#include "app_rtos_task.h"


#define TAG "wifi_scanner"
#define SCAN_DONE_BIT BIT0

typedef struct {
    wifi_scan_config_t scan_config;
    wifi_ap_record_t ap_record_list[CONFIG_WIFI_SCAN_LIST_SIZE];
    wifi_scan_result_t scan_result[CONFIG_WIFI_SCAN_LIST_SIZE];
    volatile bool scan_task_running;
    app_wifi_scanner_callback_t scanner_callback;
    EventGroupHandle_t scan_done_event_group;
    TaskHandle_t scan_task_handle;
    esp_netif_t *sta_netif;
} app_wifi_scanner_context_t;

// Module context: context dynamically allocated, internal fields statically resident in same memory block
static app_wifi_scanner_context_t *s_ctx = NULL;


// Event handler function
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    app_wifi_scanner_context_t *ctx = (app_wifi_scanner_context_t *)arg;
    if (ctx == NULL) {
        return;
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        // Scan completed, notify main task
        xEventGroupSetBits(ctx->scan_done_event_group, SCAN_DONE_BIT);
    }
}

/**
 * @brief WiFi scan result processing task, process results and notify via callback in this task
 * 
 * @param pvParameters 
 */
static void wifi_scan_task(void *pvParameters) {
    app_wifi_scanner_context_t *ctx = (app_wifi_scanner_context_t *)pvParameters;
    if (ctx == NULL) {
        vTaskDelete(NULL);
        return;
    }
    esp_err_t err;

    ESP_LOGI(TAG, "Wifi scan and show list task started");
    while (ctx->scan_task_running) {
        // Get scan records
        uint16_t number = CONFIG_WIFI_SCAN_LIST_SIZE;
        // Wait for scan completion event
        EventBits_t bits = xEventGroupWaitBits(ctx->scan_done_event_group, SCAN_DONE_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        // Prevent task from executing subsequent logic right after waking if stop() called
        if (!ctx->scan_task_running) {
            break;
        }
        // Handle scan completion event
        if (bits & SCAN_DONE_BIT) {
            err = esp_wifi_scan_get_ap_records(&number, ctx->ap_record_list);
            if (err != ESP_OK) {
                break;
            }
            for (int i = 0; i < number; i++) {
                // Extract fields of interest, put in result buffer
                ctx->scan_result[i].ecn = ctx->ap_record_list[i].authmode;
                memcpy(ctx->scan_result[i].ssid, ctx->ap_record_list[i].ssid, sizeof(ctx->scan_result[i].ssid));
                ctx->scan_result[i].rssi = ctx->ap_record_list[i].rssi;
                memcpy(ctx->scan_result[i].mac, ctx->ap_record_list[i].bssid, sizeof(ctx->scan_result[i].mac));
                ctx->scan_result[i].channel = ctx->ap_record_list[i].primary;
                ctx->scan_result[i].pairwise_cipher = ctx->ap_record_list[i].pairwise_cipher;
                ctx->scan_result[i].group_cipher = ctx->ap_record_list[i].group_cipher;
                ctx->scan_result[i].is11b = ctx->ap_record_list[i].phy_11b;
                ctx->scan_result[i].is11g = ctx->ap_record_list[i].phy_11g;
                ctx->scan_result[i].is11n = ctx->ap_record_list[i].phy_11n;
                ctx->scan_result[i].is11lr = ctx->ap_record_list[i].phy_lr;
                ctx->scan_result[i].is11ax = ctx->ap_record_list[i].phy_11ax;
                ctx->scan_result[i].is11a = ctx->ap_record_list[i].phy_11a;
                ctx->scan_result[i].is11ac = ctx->ap_record_list[i].phy_11ac;
                ctx->scan_result[i].wps = ctx->ap_record_list[i].wps;
            }
            // If valid APs found and scan callback set, notify results via callback
            if (number > 0 && ctx->scanner_callback != NULL) {
                ctx->scanner_callback(ctx->scan_result, number);
            }
            // Ensure scanning should continue; if external stop called, don't execute start after finishing current scan data
            if (ctx->scan_task_running) {
                // Restart non-blocking scan
                err = esp_wifi_scan_start(&ctx->scan_config, false);
                if (err != ESP_OK) {
                    break;
                }
            }
        }
    }

    ESP_LOGI(TAG, "wifi_scan_task exit.");
    ctx->scan_task_handle = NULL;
    ctx->scan_task_running = false;
    vTaskDelete(NULL);
}

/**
 * @brief Internal unified cleanup function (used for rollback on init failure)
 */
static void cleanup_on_init_failure(app_wifi_scanner_context_t *ctx) {
    if (ctx == NULL) {
        return;
    }
    // If WiFi not started yet, may have inited or not; for safety call both
    // esp_wifi_stop usually returns ERR when not started, can ignore
    esp_wifi_stop();
    esp_wifi_deinit();

    if (ctx->sta_netif) {
        esp_netif_destroy_default_wifi(ctx->sta_netif);
        ctx->sta_netif = NULL;
    }
    if (ctx->scan_done_event_group) {
        vEventGroupDelete(ctx->scan_done_event_group);
        ctx->scan_done_event_group = NULL;
    }

    esp_event_loop_delete_default();
    esp_netif_deinit();

    free(ctx);
    if (s_ctx == ctx) {
        s_ctx = NULL;
    }
}

/**
 * @brief Initialize WiFi scanner
 * 
 * @return esp_err_t 
 */
esp_err_t app_wifi_scanner_init(void) {
    if (s_ctx != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    app_wifi_scanner_context_t *ctx = (app_wifi_scanner_context_t *)calloc(1, sizeof(app_wifi_scanner_context_t));
    if (ctx == NULL) {
        return ESP_ERR_NO_MEM;
    }
    s_ctx = ctx;

    esp_err_t err;

    err = esp_netif_init();
    if (err != ESP_OK) {
        cleanup_on_init_failure(ctx);
        return err;
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        cleanup_on_init_failure(ctx);
        return err;
    }
    ctx->scan_done_event_group = xEventGroupCreate();
    if (ctx->scan_done_event_group == NULL) {
        cleanup_on_init_failure(ctx);
        return ESP_ERR_NO_MEM;
    }

    ctx->sta_netif = esp_netif_create_default_wifi_sta();
    assert(ctx->sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        cleanup_on_init_failure(ctx);
        return err;
    }

    err = esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &wifi_event_handler, ctx, NULL);
    if (err != ESP_OK) {
        cleanup_on_init_failure(ctx);
        return err;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        cleanup_on_init_failure(ctx);
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        cleanup_on_init_failure(ctx);
        return err;
    }

    // Initialize default scan configuration
    ctx->scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE; // Default to active scan
    ctx->scan_config.show_hidden = true;
    ctx->scan_config.scan_time.active.min = 120;
    ctx->scan_config.scan_time.active.max = 120;
    ctx->scan_config.channel_bitmap.ghz_2_channels = 0;

    return ESP_OK;
}

/**
 * @brief Start WiFi scan
 * 
 * @return esp_err_t 
 */
esp_err_t app_wifi_scanner_start(void) {
    if (s_ctx == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!s_ctx->scan_task_running) {
        s_ctx->scan_task_running = true;
        
        esp_err_t err = esp_wifi_scan_start(&s_ctx->scan_config, false);
        if (err != ESP_OK) {
            s_ctx->scan_task_running = false;
            return err;
        }

        xTaskCreate(wifi_scan_task, "wifiScanTask", 4096, s_ctx, 8, &s_ctx->scan_task_handle);
        if (s_ctx->scan_task_handle == NULL) {
            s_ctx->scan_task_running = false;
            esp_wifi_scan_stop();
            return ESP_ERR_NO_MEM;
        }
        
        ESP_LOGI(TAG, "Free Heap(after app_wifi_scanner_start): %ld", esp_get_free_heap_size());
        ESP_LOGI(TAG, "WiFi Scanner started");
    }

    return ESP_OK;
}

/**
 * @brief Stop WiFi scan
 * 
 * @return esp_err_t 
 */
esp_err_t app_wifi_scanner_stop(void) {
    esp_err_t err = ESP_OK;

    if (s_ctx == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_ctx->scan_task_running) {
        s_ctx->scan_task_running = false;

        // Stop underlying scan
        // According to ESP-IDF docs, esp_wifi_scan_stop() automatically triggers WIFI_EVENT_SCAN_DONE
        // scan_task_running is already set to false before this call.
        // xEventGroupWaitBits in the scan task should exit after scan stop completes.
        err = esp_wifi_scan_stop();
        
        // Wait for scan task to complete
        wait_for_rtos_task_exit(2000, &s_ctx->scan_task_handle);
        ESP_LOGI(TAG, "WiFi Scanner stoped");
    }

    return err;
}

/**
 * @brief Deinitialize WiFi scanner
 * 
 * @return esp_err_t 
 */
esp_err_t app_wifi_scanner_deinit(void) {
    if (s_ctx == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Deinitializing WiFi Scanner...");

    // 1. Stop task
    app_wifi_scanner_stop();

    // 2. Clean up WiFi driver
    esp_wifi_stop();
    esp_wifi_deinit();

    if (s_ctx->sta_netif) {
        esp_netif_destroy_default_wifi(s_ctx->sta_netif);
        s_ctx->sta_netif = NULL;
    }

    esp_event_loop_delete_default();
    esp_netif_deinit();

    // 3. Clean up RTOS objects
    if (s_ctx->scan_done_event_group) {
        vEventGroupDelete(s_ctx->scan_done_event_group);
        s_ctx->scan_done_event_group = NULL;
    }

    // 4. Release context
    free(s_ctx);
    s_ctx = NULL;

    ESP_LOGI(TAG, "WiFi Scanner deinitialized");
    return ESP_OK;
}

/**
 * @brief Get scan config; note this function must be called after successful app_wifi_scanner_init, else returned pointer will be NULL
 * And don't continue using returned parameter after calling app_wifi_scanner_deinit
 * 
 * @return esp_err_t Return ESP_OK on successful config reference get; if WiFi device state wrong or config reference released, return ESP_ERR_INVALID_STATE
 */
esp_err_t app_wifi_scanner_get_scan_config(wifi_scan_config_t **pcfg) {
    if (pcfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ctx) {
        *pcfg = &s_ctx->scan_config;
        return ESP_OK;
    }
    return ESP_ERR_INVALID_STATE;
}

/**
 * @brief Get scan state to check if WiFi scan is running
 * 
 * @return uint8_t 0 means not running, 1 means running
 */
uint8_t app_wifi_scanner_get_status(void) {
    if (s_ctx == NULL) {
        return 0;
    }
    return s_ctx->scan_task_running;
}

/**
 * @brief Set WiFi scan result processing callback
 * 
 * @param callback Callback function reference
 */
void app_wifi_scanner_set_callback(app_wifi_scanner_callback_t callback) {
    if (s_ctx == NULL) {
        return;
    }
    s_ctx->scanner_callback = callback;
}
