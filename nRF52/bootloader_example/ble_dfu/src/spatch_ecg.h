/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_SPATCH_ECG_H_
#define BT_SPATCH_ECG_H_

/**@file
 * @defgroup bt_ECG LED Button Service API
 * @{
 * @brief API for the LED Button Service (ECG).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

/** @brief ECG Service UUID. */
#define BT_UUID_ECG_VAL                                                        \
  BT_UUID_128_ENCODE(0x66900001, 0xDA64, 0x5A97, 0x8C4F, 0x04B8593FF99B)

/** @brief ECG Control Pointer Characteristic UUID. */
#define BT_UUID_ECG_CONTROL_POINTER_VAL                                        \
  BT_UUID_128_ENCODE(0x66900002, 0xDA64, 0x5A97, 0x8C4F, 0x04B8593FF99B)

/** @brief ECG Live Characteristic UUID. */
#define BT_UUID_ECG_LIVE_VAL                                                   \
  BT_UUID_128_ENCODE(0x66900003, 0xDA64, 0x5A97, 0x8C4F, 0x04B8593FF99B)

/** @brief ECG DB Characteristic UUID. */
#define BT_UUID_DB_LIVE_VAL                                                    \
  BT_UUID_128_ENCODE(0x66900005, 0xDA64, 0x5A97, 0x8C4F, 0x04B8593FF99B)

/** @brief IMU Characteristic UUID. */
#define BT_UUID_ECG_MOTION_DATA_VAL                                            \
  BT_UUID_128_ENCODE(0x66900006, 0xDA64, 0x5A97, 0x8C4F, 0x04B8593FF99B)

#define BT_UUID_ECG BT_UUID_DECLARE_128(BT_UUID_ECG_VAL)
#define BT_UUID_ECG_CONTROL_POINTER                                            \
  BT_UUID_DECLARE_128(BT_UUID_ECG_CONTROL_POINTER_VAL)
#define BT_UUID_ECG_LIVE BT_UUID_DECLARE_128(BT_UUID_ECG_LIVE_VAL)
#define BT_UUID_ECG_DB BT_UUID_DECLARE_128(BT_UUID_DB_LIVE_VAL)
#define BT_UUID_ECG_MOTION BT_UUID_DECLARE_128(BT_UUID_ECG_MOTION_DATA_VAL)

/** @brief Callback type for when an LED state change is received. */
typedef void (*led_cb_t)(const bool led_state);

/** @brief Callback type for when the button state is pulled. */
typedef uint8_t (*live_cb_t)(void);
typedef uint8_t (*db_cb_t)(void);
typedef uint8_t (*motion_cb_t)(void);

/** @brief Callback struct used by the ECG Service. */
struct bt_ECG_cb {
  /** LED state change callback. */
  led_cb_t led_cb;
  live_cb_t live_data_cb;
  db_cb_t db_data_cb;
  motion_cb_t motion_data_cb;
};

/** @brief Initialize the ECG Service.
 *
 * This function registers a GATT service with two characteristics: Button
 * and LED.
 * Send notifications for the Button Characteristic to let connected peers know
 * when the button state changes.
 * Write to the LED Characteristic to change the state of the LED on the
 * board.
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *			used by the service. This pointer can be NULL
 *			if no callback functions are defined.
 *
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ECG_init(struct bt_ECG_cb *callbacks);

/** @brief Send the button state.
 *
 * This function sends a binary state, typically the state of a
 * button, to all connected peers.
 *
 * @param[in] button_state The state of the button.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ECG_send_live_data(uint8_t button_state);
int bt_ECG_send_db_data(uint8_t button_state);
int bt_ECG_send_motion_data(uint8_t button_state);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_SPATCH_ECG_H_ */
