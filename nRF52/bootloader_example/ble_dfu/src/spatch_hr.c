/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Peripheral Heart Rate over LE Coded PHY sample
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/printk.h>
#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/hrs.h>
#include <bluetooth/uuid.h>

#include "spatch_hr.h"

void hrs_notify(void) {
  static uint8_t heartrate = 90U;

  /* Heartrate measurements simulation */
  heartrate++;
  if (heartrate == 160U) {
    heartrate = 90U;
  }

  bt_hrs_notify(heartrate);
}