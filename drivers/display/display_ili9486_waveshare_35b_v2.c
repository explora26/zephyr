/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <display.h>
#include "display_ili9486.h"

void ili9486_lcd_init(struct ili9486_data *p_ili9486)
{
	u8_t cmd;
	u16_t data[15];

	/* Software reset */
	cmd = ILI9340_CMD_SOFTWARE_RESET;
	ili9486_transmit(p_ili9486, cmd, NULL, 0);

	k_sleep(5);

	cmd = ILI9340_CMD_INTF_MODE_CTRL;
	data[0] = 0x00U;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	/* Sleep Out */
	cmd = ILI9340_CMD_EXIT_SLEEP;
	ili9486_transmit(p_ili9486, cmd, NULL, 0);

	k_sleep(255);

	cmd = ILI9340_CMD_INVER_ON;
	ili9486_transmit(p_ili9486, cmd, NULL, 0);


	/* Pixel Format Set */
	cmd = ILI9340_CMD_PIXEL_FORMAT_SET;
#ifdef CONFIG_ILI9486_RGB565
	data[0] = ILI9340_DATA_PIXEL_FORMAT_MCU_16_BIT |
		  ILI9340_DATA_PIXEL_FORMAT_RGB_16_BIT;
#else
	data[0] = ILI9340_DATA_PIXEL_FORMAT_MCU_18_BIT |
		  ILI9340_DATA_PIXEL_FORMAT_RGB_18_BIT;
#endif
	ili9486_transmit(p_ili9486, cmd, data, 1);

	cmd = ILI9486_CMD_POWER_CTRL_3;
	data[0] = 0x33U;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	/* VCM control */
	cmd = ILI9340_CMD_VCOM_CTRL_1;
	data[0] = 0x00U;
	data[1] = 0x1EU;
	data[2] = 0x80U;
	ili9486_transmit(p_ili9486, cmd, data, 3);

	/* Memory Access Control */
	cmd = ILI9340_CMD_MEM_ACCESS_CTRL;
	data[0] = ILI9340_DATA_MEM_ACCESS_CTRL_MY |
		  ILI9340_DATA_MEM_ACCESS_CTRL_MV |
		  ILI9340_DATA_MEM_ACCESS_CTRL_ML |
		  ILI9340_DATA_MEM_ACCESS_CTRL_BGR;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	/* Frame Rate */
	cmd = ILI9340_CMD_FRAME_CTRL_NORMAL_MODE;
	data[0] = 0xB0U;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	/* Positive Gamma Correction */
	cmd = ILI9340_CMD_POSITVE_GAMMA_CORRECTION;
	data[0] = 0x00U;
	data[1] = 0x13U;
	data[2] = 0x18U;
	data[3] = 0x04U;
	data[4] = 0x0FU;
	data[5] = 0x06U;
	data[6] = 0x3AU;
	data[7] = 0x56U;
	data[8] = 0x4DU;
	data[9] = 0x03U;
	data[10] = 0x0AU;
	data[11] = 0x06U;
	data[12] = 0x30U;
	data[13] = 0x3EU;
	data[14] = 0x0FU;
	ili9486_transmit(p_ili9486, cmd, data, 15);

	/* Negative Gamma Correction */
	cmd = ILI9340_CMD_NEGATIVE_GAMMA_CORRECTION;
	data[0] = 0x00U;
	data[1] = 0x13U;
	data[2] = 0x18U;
	data[3] = 0x01U;
	data[4] = 0x11U;
	data[5] = 0x06U;
	data[6] = 0x3BU;
	data[7] = 0x34U;
	data[8] = 0x4DU;
	data[9] = 0x06U;
	data[10] = 0x0DU;
	data[11] = 0x0BU;
	data[12] = 0x31U;
	data[13] = 0x37U;
	data[14] = 0x0FU;
	ili9486_transmit(p_ili9486, cmd, data, 15);


	/* Sleep Out */
	cmd = ILI9340_CMD_EXIT_SLEEP;
	ili9486_transmit(p_ili9486, cmd, NULL, 0);

	k_sleep(120);

	/* Display ON */
	cmd = ILI9340_CMD_DISPLAY_ON;
	ili9486_transmit(p_ili9486, cmd, NULL, 0);

}
