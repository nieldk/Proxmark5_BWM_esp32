/*
 * EXPERIMENTAL -- see app_zb_probe.h and docs/esp32c2_ble_phy_notes.md.
 * Built from static disassembly of espressif/esp32c2-bt-lib, never
 * verified against hardware before this was written. Expect it to need
 * iteration once real register dumps come back.
 */

#include <string.h>
#include "app_zb_probe.h"
#include "app_com_defs.h"
#include "app_cmd_uart.h"
#include "esp_bt.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "zb_probe";

/* Internal controller symbols. Prototypes are inferred from disassembly of
 * libble_app.a, not from a header -- if the real signatures differ this
 * will misbehave rather than fail to compile. */
extern void r_ble_phy_mode_write(uint8_t mode);
extern void r_ble_phy_config_access_addr(uint32_t access_addr);
extern void r_ble_phy_disable_whitening(void);
extern void r_ble_phy_enable_whitening(void);
extern void r_ble_phy_setchan(uint8_t chan);
extern void r_ble_phy_start_rx_immediately(void);
extern void r_ble_phy_disable(void);
// Confirmed against the public Apache Mynewt NimBLE controller (this library's
// upstream): matches ble_phy_global_rxbuf_get()'s two out-params exactly.
// Second param's meaning on this port isn't confirmed (len? index?); see header.
extern void r_ble_phy_global_rxbuf_get(uint32_t *rxbuf_ptr_out, uint32_t *aux_out);

#define BLE_REG_BASE   0x60045000u
#define REG(off)       (*(volatile uint32_t *)(BLE_REG_BASE + (off)))

// RB_FREQ_OFF is read back for the regs snapshot (whatever r_ble_phy_setchan()
// left there), never written directly anymore -- see app_zb_probe_start().
#define RB_FREQ_OFF    0x508u
#define RB_MODE_A_OFF  0x510u
#define RB_MODE_B_OFF  0x514u
#define RB_WHITEN_OFF  0x518u
#define RB_AA_LO_OFF   0x51Cu
#define RB_AA_HI_OFF   0x524u

// ESP32-C2 DRAM range (SRAM0+SRAM1 mapped as data memory); from Espressif's
// published memory map. Used only to decide whether rxbuf_ptr is safe to
// dereference, not to confirm it actually points at a received frame.
#define C2_DRAM_LO 0x3FCA0000u
#define C2_DRAM_HI 0x3FCE0000u

static bool s_running = false;
static uint32_t s_poll_count = 0;
static TaskHandle_t s_broadcast_task = NULL;
static int64_t s_deadline_us = 0;

// Guards every call into the controller/PHY registers below. GET_REGS and
// STOP are dispatched from the UART command task; broadcast_task is a
// separate task re-arming RX and reading the same registers every 500ms.
// Without this, the two can touch the radio at the same time.
static SemaphoreHandle_t s_lock = NULL;

static void lock_init_once(void) {
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
    }
}

void app_zb_probe_pack_regs(const app_zb_probe_regs_t *regs, uint8_t payload[APP_ZB_PROBE_WIRE_LEN]) {
    memcpy(payload +  0, &regs->reg_freq,   4);
    memcpy(payload +  4, &regs->reg_mode_a, 4);
    memcpy(payload +  8, &regs->reg_mode_b, 4);
    memcpy(payload + 12, &regs->reg_whiten, 4);
    memcpy(payload + 16, &regs->reg_aa_lo,  4);
    memcpy(payload + 20, &regs->reg_aa_hi,  4);
    memcpy(payload + 24, &regs->poll_count, 4);
    memcpy(payload + 28, &regs->rxbuf_ptr,  4);
    memcpy(payload + 32, &regs->rxbuf_aux,  4);
    payload[36] = regs->rxbuf_in_dram ? 1 : 0;
    memcpy(payload + 37, regs->rxbuf_snapshot, APP_ZB_PROBE_RXBUF_SNAPSHOT_LEN);
    payload[37 + APP_ZB_PROBE_RXBUF_SNAPSHOT_LEN] = regs->running ? 1 : 0;
}

// Internal, lock already held. Re-arms RX (if running) and fills *out.
static void get_regs_locked(app_zb_probe_regs_t *out) {
    if (s_running) {
        // Single-packet DTM-style RX arm, not a free-running receiver
        // (confirmed by its DTM call site in the disassembly) -- re-issue
        // on every read so the radio keeps listening between polls.
        r_ble_phy_start_rx_immediately();
    }

    out->reg_freq   = REG(RB_FREQ_OFF);
    out->reg_mode_a = REG(RB_MODE_A_OFF);
    out->reg_mode_b = REG(RB_MODE_B_OFF);
    out->reg_whiten = REG(RB_WHITEN_OFF);
    out->reg_aa_lo  = REG(RB_AA_LO_OFF);
    out->reg_aa_hi  = REG(RB_AA_HI_OFF);
    out->poll_count = s_running ? ++s_poll_count : s_poll_count;

    out->rxbuf_ptr = 0;
    out->rxbuf_aux = 0;
    out->rxbuf_in_dram = false;
    memset(out->rxbuf_snapshot, 0, sizeof(out->rxbuf_snapshot));
    if (s_running) {
        uint32_t ptr = 0, aux = 0;
        r_ble_phy_global_rxbuf_get(&ptr, &aux);
        out->rxbuf_ptr = ptr;
        out->rxbuf_aux = aux;
        // Bounds check only -- a pointer inside DRAM is safe to read, not
        // proof it holds a received frame. See header for why.
        if (ptr >= C2_DRAM_LO && ptr <= C2_DRAM_HI - APP_ZB_PROBE_RXBUF_SNAPSHOT_LEN) {
            memcpy(out->rxbuf_snapshot, (const void *)ptr, APP_ZB_PROBE_RXBUF_SNAPSHOT_LEN);
            out->rxbuf_in_dram = true;
        }
    }
    // Registers above are read unconditionally even when not running --
    // harmless (just whatever the BLE controller's hardware block last
    // held) as long as the chip's BT clock domain is still powered. After
    // esp_bt_controller_deinit() that may not hold; callers should treat a
    // snapshot with running=false as informational only, not reliable.
    out->running = s_running;
}

// Internal, lock already held.
static void stop_locked(void) {
    if (!s_running) {
        return;
    }
    r_ble_phy_disable();
    r_ble_phy_enable_whitening(); // restore, in case the controller is reused normally after this
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    s_running = false;
    ESP_LOGI(TAG, "probe stopped, controller deinitialized");
}

static void broadcast_task(void *arg) {
    for (;;) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        if (!s_running) {
            xSemaphoreGive(s_lock);
            break;
        }
        if (esp_timer_get_time() >= s_deadline_us) {
            ESP_LOGI(TAG, "probe duration elapsed, auto-stopping");
            stop_locked();
            xSemaphoreGive(s_lock);
            break;
        }

        app_zb_probe_regs_t regs;
        get_regs_locked(&regs);
        xSemaphoreGive(s_lock);

        uint8_t payload[APP_ZB_PROBE_WIRE_LEN];
        app_zb_probe_pack_regs(&regs, payload);
        app_uart_send_broadcast(APP_BROADCAST_ZB_PROBE_REGS, payload, sizeof(payload));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    s_broadcast_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t app_zb_probe_start(uint32_t access_addr, uint8_t ble_channel, uint32_t duration_s) {
    lock_init_once();
    if (s_lock == NULL) {
        ESP_LOGE(TAG, "mutex create failed");
        return ESP_ERR_NO_MEM;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);

    if (s_running) {
        ESP_LOGW(TAG, "already running");
        xSemaphoreGive(s_lock);
        return ESP_ERR_INVALID_STATE;
    }
    if (ble_channel > 39) {
        xSemaphoreGive(s_lock);
        return ESP_ERR_INVALID_ARG;
    }
    if (duration_s == 0 || duration_s > 300) {
        duration_s = 300; // self-limiting: never left running unattended indefinitely
    }

    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_bt_controller_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) { // INVALID_STATE = already initialized, fine
        ESP_LOGE(TAG, "controller_init failed: %d", err);
        xSemaphoreGive(s_lock);
        return err;
    }
    err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "controller_enable failed: %d", err);
        xSemaphoreGive(s_lock);
        return err;
    }
    ESP_LOGI(TAG, "controller enabled, probing BLE ch %u (%u MHz) aa=0x%08x for %us",
             ble_channel, 2402 + 2 * ble_channel, (unsigned)access_addr, (unsigned)duration_s);

    // ble_channel is a real BLE Core Spec RF channel index (f = 2402 + 2*ch
    // MHz, confirmed public spec, not a guess) -- the caller picks it to
    // match a known 802.15.4 channel; see `hw bwm zbprobe`'s --zbchan.
    r_ble_phy_setchan(ble_channel);
    r_ble_phy_mode_write(2); // candidate for 2M PHY; unconfirmed, see header
    r_ble_phy_disable_whitening();
    r_ble_phy_config_access_addr(access_addr);
    r_ble_phy_start_rx_immediately();

    s_running = true;
    s_poll_count = 0;
    s_deadline_us = esp_timer_get_time() + (int64_t)duration_s * 1000000;

    BaseType_t ok = xTaskCreate(broadcast_task, "zb_probe_bcast", 3072, NULL, 4, &s_broadcast_task);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "failed to start broadcast task, rolling back");
        stop_locked(); // leaves s_running false; nothing is left running unattended
        xSemaphoreGive(s_lock);
        return ESP_ERR_NO_MEM;
    }

    xSemaphoreGive(s_lock);
    return ESP_OK;
}

esp_err_t app_zb_probe_stop(void) {
    lock_init_once();
    if (s_lock == NULL) {
        return ESP_ERR_NO_MEM;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    stop_locked();
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

esp_err_t app_zb_probe_get_regs(app_zb_probe_regs_t *out) {
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    lock_init_once();
    if (s_lock == NULL) {
        return ESP_ERR_NO_MEM;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    get_regs_locked(out);
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

bool app_zb_probe_is_running(void) {
    return s_running;
}
