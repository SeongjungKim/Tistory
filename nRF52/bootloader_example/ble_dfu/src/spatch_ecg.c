/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief LED Button Service (ECG) sample
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/byteorder.h>
#include <sys/printk.h>
#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>

#include "spatch_ecg.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(bt_ECG, LOG_LEVEL_DBG);

static bool cp_indicate_enabled;
static bool live_notify_enabled, db_notify_enabled, motion_notify_enabled;
static uint8_t db_data, live_data, motion_data;
static struct bt_ECG_cb ECG_cb;

static void ECG_cp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                   uint16_t value) {
  cp_indicate_enabled = (value == BT_GATT_CCC_INDICATE);
  LOG_DBG("%s",
          cp_indicate_enabled ? "cp indicate enabled" : "cp indicate disabled");
}

static void ECG_live_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                     uint16_t value) {
  live_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
  LOG_DBG("%s", live_notify_enabled ? "live notificate enabled"
                                    : "live notificate disabled");
}

static void ECG_db_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                   uint16_t value) {
  db_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
  LOG_DBG("%s", db_notify_enabled ? "db notificate enabled"
                                  : "db notificate disabled");
}

static void ECG_motion_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                       uint16_t value) {
  motion_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
  LOG_DBG("%s", motion_notify_enabled ? "motion notificate enabled"
                                      : "motion notificate disabled");
}

static ssize_t write_led(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         const void *buf, uint16_t len, uint16_t offset,
                         uint8_t flags) {
  LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle, (void *)conn);

  if (len != 1U) {
    LOG_DBG("Write led: Incorrect data length%s", "");
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
  }

  if (offset != 0) {
    LOG_DBG("Write led: Incorrect data offset%s", "");
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }

  if (ECG_cb.led_cb) {
    uint8_t val = *((uint8_t *)buf);

    if (val == 0x00 || val == 0x01) {
      ECG_cb.led_cb(val ? true : false);
    } else {
      LOG_DBG("Write led: Incorrect value%s", "");
      return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }
  }

  return len;
}

static ssize_t read_live(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         void *buf, uint16_t len, uint16_t offset) {
  const char *value = attr->user_data;

  LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

  if (ECG_cb.live_data_cb) {
    live_data = ECG_cb.live_data_cb();
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             sizeof(*value));
  }

  return 0;
}

static ssize_t read_db(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                       void *buf, uint16_t len, uint16_t offset) {
  const char *value = attr->user_data;

  LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

  if (ECG_cb.db_data_cb) {
    db_data = ECG_cb.db_data_cb();
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             sizeof(*value));
  }

  return 0;
}

static ssize_t read_motion(struct bt_conn *conn,
                           const struct bt_gatt_attr *attr, void *buf,
                           uint16_t len, uint16_t offset) {
  const char *value = attr->user_data;

  LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

  if (ECG_cb.motion_data_cb) {
    motion_data = ECG_cb.motion_data_cb();
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             sizeof(*value));
  }

  return 0;
}

/* LED Button Service Declaration */
BT_GATT_SERVICE_DEFINE(
    ECG_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_ECG),
    BT_GATT_CHARACTERISTIC(BT_UUID_ECG_CONTROL_POINTER,
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_WRITE | BT_GATT_PERM_READ, NULL,
                           write_led, NULL),
    BT_GATT_CCC(ECG_cp_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_ECG_LIVE, BT_GATT_CHRC_NOTIFY, NULL,
                           read_live, NULL, &live_data),
    BT_GATT_CCC(ECG_live_ccc_cfg_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_ECG_DB, BT_GATT_CHRC_NOTIFY, NULL, read_db,
                           NULL, &db_data),
    BT_GATT_CCC(ECG_db_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_ECG_MOTION, BT_GATT_CHRC_NOTIFY, NULL,
                           read_motion, NULL, &motion_data),
    BT_GATT_CCC(ECG_motion_ccc_cfg_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), );

int bt_ECG_init(struct bt_ECG_cb *callbacks) {
  if (callbacks) {
    ECG_cb.led_cb = callbacks->led_cb;
    ECG_cb.live_data_cb = callbacks->live_data_cb;
    ECG_cb.db_data_cb = callbacks->db_data_cb;
    ECG_cb.motion_data_cb = callbacks->motion_data_cb;
  }

  return 0;
}

int bt_ECG_send_live_data(uint8_t live_data) {
  if (!live_notify_enabled) {
    return -EACCES;
  }

  return bt_gatt_notify(NULL, &ECG_svc.attrs[5], &live_data, sizeof(live_data));
}

int bt_ECG_send_db_data(uint8_t db_data) {
  if (!db_notify_enabled) {
    return -EACCES;
  }

  return bt_gatt_notify(NULL, &ECG_svc.attrs[8], &db_data, sizeof(db_data));
}

int bt_ECG_send_motion_data(uint8_t motion_data) {
  if (!motion_notify_enabled) {
    return -EACCES;
  }

  return bt_gatt_notify(NULL, &ECG_svc.attrs[11], &motion_data,
                        sizeof(motion_data));
}