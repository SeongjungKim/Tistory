/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <drivers/gpio.h>
#include <errno.h>
#include <soc.h>
#include <stddef.h>
#include <string.h>
#include <sys/byteorder.h>
#include <sys/printk.h>
#include <zephyr.h>
#include <zephyr/types.h>

#include "img_mgmt/img_mgmt.h"
#include "os_mgmt/os_mgmt.h"
#include <mgmt/mcumgr/smp_bt.h>

#include "spatch_ecg.h"
#include "spatch_hr.h"
#include "spatch_battery.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <settings/settings.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

#define USER_LED DK_LED3

#define USER_BUTTON DK_BTN1_MSK

static uint8_t app_live_state;
static uint8_t app_db_state;
static uint8_t app_motion_state;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, 0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b,
                  0x86, 0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
};

static void connected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    printk("Connection failed (err %u)\n", err);
    return;
  }

  printk("Connected\n");

  dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
  printk("Disconnected (reason %u)\n", reason);

  dk_set_led_off(CON_STATUS_LED);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  if (!err) {
    printk("Security changed: %s level %u\n", addr, level);
  } else {
    printk("Security failed: %s level %u err %d\n", addr, level, err);
  }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  printk("Pairing cancelled: %s\n", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  printk("Pairing failed conn: %s, reason %d\n", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .cancel = auth_cancel,
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed};

static void app_led_cb(bool led_state) { dk_set_led(USER_LED, led_state); }

static uint8_t app_live_cb(void) { return app_live_state; }

static uint8_t app_db_cb(void) { return app_db_state; }

static uint8_t app_motion_cb(void) { return app_motion_state; }

static struct bt_ECG_cb ECG_callbacs = {
    .led_cb = app_led_cb,
    .live_data_cb = app_live_cb,
    .db_data_cb = app_db_cb,
    .motion_data_cb = app_motion_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed) {
  if (has_changed & USER_BUTTON) {
    uint8_t user_button_state = button_state & USER_BUTTON;

    bt_ECG_send_live_data(user_button_state + 1);
    bt_ECG_send_db_data(user_button_state + 2);
    bt_ECG_send_motion_data(user_button_state + 3);
    
    app_live_state = user_button_state;
    app_db_state = user_button_state;
    app_motion_state = user_button_state;
  }
}

static int init_button(void) {
  int err;

  err = dk_buttons_init(button_changed);
  if (err) {
    printk("Cannot init buttons (err: %d)\n", err);
  }

  return err;
}

void main(void) {
  printk("build time: " __DATE__ " " __TIME__ "\n");
  os_mgmt_register_group();
  img_mgmt_register_group();
  smp_bt_register();

  int blink_status = 0;
  int err;

  printk("Starting Bluetooth Peripheral ECG example\n");

  err = dk_leds_init();
  if (err) {
    printk("LEDs init failed (err %d)\n", err);
    return;
  }

  err = init_button();
  if (err) {
    printk("Button init failed (err %d)\n", err);
    return;
  }

  //if (IS_ENABLED(CONFIG_BT_ECG_SECURITY_ENABLED)) {
    bt_conn_auth_cb_register(&conn_auth_callbacks);
  //}

  err = bt_enable(NULL);
  if (err) {
    printk("Bluetooth init failed (err %d)\n", err);
    return;
  }

  printk("Bluetooth initialized\n");

  if (IS_ENABLED(CONFIG_SETTINGS)) {
    settings_load();
  }

  err = bt_ECG_init(&ECG_callbacs);
  if (err) {
    printk("Failed to init ECG (err:%d)\n", err);
    return;
  }

  err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
  if (err) {
    printk("Advertising failed to start (err %d)\n", err);
    return;
  }

  printk("Advertising successfully started\n");

  for (;;) {
    dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
    k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));

    hrs_notify();
    bas_notify();
  }
}
