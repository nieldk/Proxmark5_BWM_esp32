#ifndef APP_ZB_PROBE_H_
#define APP_ZB_PROBE_H_

/*
 * EXPERIMENTAL. See README section "Zigbee/802.15.4 probe (app_zb_probe)".
 *
 * Drives the ESP32-C2 BLE controller's internal PHY primitives (normally
 * owned exclusively by the controller's own scheduler) to test the WazaBee
 * technique against real Zigbee traffic: 2M GFSK, whitening off, access
 * address 0x9b3af703 (the MSK encoding of the Zigbee preamble's repeated
 * 0000 symbol -- Cayre/Galtier/Auriol/Nicomette/Marconato, "WazaBee",
 * SSTIC 2020; confirmed on nRF52832 and CC1352-R1, not yet on this chip).
 * WazaBee needs BLE's 2M PHY, which ESP-IDF's own docs list as supported
 * on ESP32-C2 -- the same gate the SSTIC/WOOT23 papers say rules out the
 * original ESP32 (1M only) but allows ESP32-S3/C3.
 *
 * This takes the BLE controller away from its normal job while running.
 * BLE SPP (and so the pm5<->BWM wireless link, if that's how the host is
 * talking to this module) will NOT work while the probe is active. Run
 * this over the USB/UART production header, not over a BLE link you need
 * to stay up.
 *
 * Channel tuning uses r_ble_phy_setchan(ble_channel), which is a real,
 * public BLE Core Spec RF channel index (f = 2402 + 2*ch MHz for ch 0-39,
 * not a guess) -- the frequency itself is therefore exact. The caller
 * converts a known 802.15.4 channel to the nearest BLE channel index
 * (only every other 802.15.4 channel lands on the 2MHz BLE grid exactly;
 * see `hw bwm zbprobe`'s --zbchan for the table). What's still unverified:
 *
 * Known unknowns, not resolved before this had to stop:
 *   - CRC and frame-length are NOT reconfigured here. The ESP32-S3/C3 port
 *     of WazaBee (WOOT23, "ESPwn32") disables CRC checking and raises the
 *     max frame length to 255 bytes via a named register, BLE_RWBLECNTL_REG
 *     -- without that, the controller's own CRC gate may flush anything
 *     that isn't a valid BLE frame before it ever reaches rxbuf_ptr below,
 *     independent of whether sync detection itself works. Checked and
 *     ruled out as a quick fix: Espressif does not publish this
 *     controller's register map in any public SVD for the RISC-V chips
 *     (C2/C3), which is why WOOT23 needed Ghidra for S3/C3 in the first
 *     place; the original Xtensa ESP32's SVD does expose a same-family
 *     block (btdm.rs: BLECNTL/BLECONF/... at 0x200-0x2A0, matching the
 *     RW-BLE-LC layout Dialog's DA1468x documents), but that's a
 *     different base and a different-generation controller, so reusing
 *     its offsets on C2 without confirmation would be a guess, not a fix.
 *     Next step is empirical, not more disassembly: on real hardware,
 *     `hw bwm zbprobe --watch` already dumps reg_mode_a/reg_mode_b/
 *     reg_whiten every poll -- if a real Zigbee transmitter is active
 *     on the tuned channel, compare register snapshots against known-
 *     good BLE traffic, or (since whitening's bit 25 at 0x518 is already
 *     confirmed) try nearby bits in reg_mode_a/reg_mode_b and watch
 *     rxbuf_snapshot for a change in behavior rather than guessing a
 *     bit from static analysis.
 *   - Which r_ble_phy_mode_write() argument truly selects 2M PHY, versus
 *     1M/coded -- inferred from branch structure only, not confirmed.
 *   - rxbuf_ptr below is read via r_ble_phy_global_rxbuf_get(), confirmed
 *     (by cross-reference against the public Apache Mynewt NimBLE
 *     controller, whose ble_ll/ble_phy source this library is a port of)
 *     to be the controller's raw PHY receive buffer accessor. What it
 *     points at while the probe's non-standard PHY config is active is
 *     NOT confirmed -- it's the controller's normal single-packet RX
 *     buffer, which this probe is deliberately using out of its normal
 *     context. rxbuf_snapshot only gets read when the pointer lands in
 *     the chip's actual DRAM range, as a safety check, not a correctness
 *     one -- a plausible-looking pointer can still hold stale or garbage
 *     data.
 *
 * See docs/esp32c2_ble_phy_notes.md for the disassembly this is based on.
 */

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define APP_ZB_PROBE_RXBUF_SNAPSHOT_LEN 32

typedef struct {
    uint32_t reg_freq;     // 0x60045508 -- candidate channel/frequency register
    uint32_t reg_mode_a;   // 0x60045510 -- PHY mode selector (meaning of value not fully confirmed)
    uint32_t reg_mode_b;   // 0x60045514 -- paired with reg_mode_a
    uint32_t reg_whiten;   // 0x60045518 -- bit 25 = whitening enable
    uint32_t reg_aa_lo;    // 0x6004551C -- access address, low part
    uint32_t reg_aa_hi;    // 0x60045524 -- access address, high byte in low bits
    uint32_t poll_count;   // how many times this snapshot has been taken since start
    uint32_t rxbuf_ptr;    // first output of r_ble_phy_global_rxbuf_get(); candidate RX buffer address
    uint32_t rxbuf_aux;    // second output of the same call; meaning not confirmed (len? index?)
    bool rxbuf_in_dram;    // true if rxbuf_ptr fell inside ESP32-C2 DRAM (0x3FCA0000-0x3FCE0000) and was read
    uint8_t rxbuf_snapshot[APP_ZB_PROBE_RXBUF_SNAPSHOT_LEN]; // bytes at rxbuf_ptr, only valid if rxbuf_in_dram
    bool running;          // true if the probe is active
} app_zb_probe_regs_t;

// Starts the probe: brings up the BLE controller, forces 2M PHY + whitening
// off + the given access address, tunes to the given real BLE RF channel
// (0-39, f = 2402 + 2*ble_channel MHz), and re-arms RX continuously for up
// to duration_s seconds (clamped to 300; 0 also means 300 -- this never
// runs unattended forever). Auto-stops and releases the BLE controller
// when the window elapses.
// Takes over the BLE controller; stop it first if BLE SPP is running.
esp_err_t app_zb_probe_start(uint32_t access_addr, uint8_t ble_channel, uint32_t duration_s);

// Stops the probe and disables the PHY. Does NOT automatically restart
// BLE SPP -- the caller (main.c) re-enables it if the host wants it back.
esp_err_t app_zb_probe_stop(void);

// Snapshot of the registers and candidate RX buffer listed above, taken right now.
esp_err_t app_zb_probe_get_regs(app_zb_probe_regs_t *out);

// True once app_zb_probe_start() has completed controller bring-up.
bool app_zb_probe_is_running(void);

// Wire payload size for APP_CMD_ZB_PROBE_GET_REGS / APP_BROADCAST_ZB_PROBE_REGS:
// 6 regs + poll_count + rxbuf_ptr + rxbuf_aux (9 x uint32) + rxbuf_in_dram(u8) +
// rxbuf_snapshot(32) + running(u8) = 36 + 1 + 32 + 1 = 70 bytes.
#define APP_ZB_PROBE_WIRE_LEN 70

// Packs a snapshot into the wire format documented above and in app_com_defs.h.
void app_zb_probe_pack_regs(const app_zb_probe_regs_t *regs, uint8_t payload[APP_ZB_PROBE_WIRE_LEN]);

#endif
