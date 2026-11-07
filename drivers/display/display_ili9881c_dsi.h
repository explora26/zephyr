/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_ILITEK_ILI9881C_H_
#define ZEPHYR_DRIVERS_DISPLAY_ILITEK_ILI9881C_H_

#include <zephyr/device.h>

/* Indexes of the init-sequence devicetree property values */
#define ILI9881C_INIT_SEQUENCE_RTKAPPLCDMS02001BE 0
#define ILI9881C_INIT_SEQUENCE_M5STACK_TAB5       1

/**
 * @brief Run the ILI9881C power-on command sequence.
 *
 * @param dev ILI9881C device instance
 * @return 0 on success, errno otherwise.
 */
int ili9881c_dsi_regs_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_DISPLAY_ILITEK_ILI9881C_H_ */
