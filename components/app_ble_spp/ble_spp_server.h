/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef H_BLESPPSERVER_
#define H_BLESPPSERVER_

#include <stdbool.h>
#include "nimble/ble.h"
#include "modlog/modlog.h"
#include "esp_peripheral.h"
#ifdef __cplusplus
extern "C" {
#endif

/* 16 Bit SPP Service UUID */
#define BLE_SVC_SPP_UUID16                                  0xAE86

/* 16 Bit SPP Service Characteristic UUID */
#define BLE_SVC_SPP_CHR_UUID16                              0xAE88

/* Standard Battery Service UUID */
#define BLE_SVC_BAS_UUID16                                  0x180F

/* Standard Battery Level Characteristic UUID */
#define BLE_SVC_BAS_CHR_UUID16                              0x2A19

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

void ble_store_config_init(void);

#ifdef __cplusplus
}
#endif

#endif
