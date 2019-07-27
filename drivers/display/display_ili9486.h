/*
 * Copyright (c) 2019 Harry Jiang
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9486_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9486_H_

#include <zephyr.h>
#include "display_ili9340.h"

#define ILI9486_CMD_POWER_CTRL_3 0xC2

struct ili9486_data;

/**
 * Send data to ILI9486 display controller
 *
 * @param data Device data structure
 * @param cmd Command to send to display controller
 * @param tx_data Data to transmit to the display controller
 * In case no data should be transmitted pass a NULL pointer
 * @param tx_len Number of bytes in tx_data buffer
 *
 */
void ili9486_transmit(struct ili9486_data *data, u8_t cmd, void *tx_data,
		      size_t tx_len);

/**
 * Perform LCD specific initialization
 *
 * @param data Device data structure
 */
void ili9486_lcd_init(struct ili9486_data *data);

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9486_H_ */
