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

	cmd = 0xf0;
	data[0] = 0xC3;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	cmd = 0xf0;
	data[0] = 0x96;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	cmd = 0x36;
	data[0] = 0x68;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	/* Memory Access Control */
	cmd = ILI9340_CMD_MEM_ACCESS_CTRL;
	data[0] = ILI9340_DATA_MEM_ACCESS_CTRL_MX |
		  ILI9340_DATA_MEM_ACCESS_CTRL_MY |
		  ILI9340_DATA_MEM_ACCESS_CTRL_MV |
		  ILI9340_DATA_MEM_ACCESS_CTRL_BGR;
	ili9486_transmit(p_ili9486, cmd, data, 1);

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

	cmd = ILI9340_CMD_INTF_MODE_CTRL;
	data[0] = 0x80U;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	cmd = ILI9340_CMD_DISPLAY_FUNCTION_CTRL;
	data[0] = 0x20;
	data[1] = 0x02;
	ili9486_transmit(p_ili9486, cmd, data, 2);

	cmd = 0xb5;
	data[0] = 0x02;
	data[1] = 0x02;
	data[2] = 0x00;
	data[3] = 0x04;
	ili9486_transmit(p_ili9486, cmd, data, 4);

	cmd = ILI9340_CMD_FRAME_CTRL_NORMAL_MODE;
	data[0] = 0x80;
	data[1] = 0x10;
	ili9486_transmit(p_ili9486, cmd, data, 2);


	cmd = 0xb4;
	data[0] = 0x00;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	cmd = 0xb7;
	data[0] = 0xc6;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	cmd = ILI9340_CMD_VCOM_CTRL_1;
	data[0] = 0x5d;
	ili9486_transmit(p_ili9486, cmd, data, 1);


	cmd = 0xe4;
	data[0] = 0x31;
	ili9486_transmit(p_ili9486, cmd, data, 1);


	cmd = 0xe8;
	data[0] = 0x40;
	data[1] = 0x8a;
	data[2] = 0x00;
	data[3] = 0x00;
	data[4] = 0x29;
	data[5] = 0x19;
	data[6] = 0xa5;
	data[7] = 0x33;
	ili9486_transmit(p_ili9486, cmd, data, 8);

	cmd = ILI9486_CMD_POWER_CTRL_3;
	ili9486_transmit(p_ili9486, cmd, NULL, 0);

	cmd = 0xa7;
	ili9486_transmit(p_ili9486, cmd, NULL, 0);

	cmd = ILI9340_CMD_POSITIVE_GAMMA_CORRECTION;
	data[0] = 0xf0;
	data[1] = 0x09;
	data[2] = 0x13;
	data[3] = 0x12;
	data[4] = 0x12;
	data[5] = 0x2b;
	data[6] = 0x3c;
	data[7] = 0x44;
	data[8] = 0x4b;
	data[9] = 0x1b;
	data[10] = 0x18;
	data[11] = 0x17;
	data[12] = 0x1d;
	data[13] = 0x21;
	ili9486_transmit(p_ili9486, cmd, data, 14);

	cmd = ILI9340_CMD_NEGATIVE_GAMMA_CORRECTION;
	data[0] = 0xf0;
	data[1] = 0x09;
	data[2] = 0x13;
	data[3] = 0x0c;
	data[4] = 0x0d;
	data[5] = 0x27;
	data[6] = 0x3b;
	data[7] = 0x44;
	data[8] = 0x4b;
	data[9] = 0x0b;
	data[10] = 0x17;
	data[11] = 0x17;
	data[12] = 0x1d;
	data[13] = 0x21;
	ili9486_transmit(p_ili9486, cmd, data, 14);

	cmd = 0xf0;
	data[0] = 0x3c;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	cmd = 0xf0;
	data[0] = 0x69;
	ili9486_transmit(p_ili9486, cmd, data, 1);

	cmd = 0x13;
	ili9486_transmit(p_ili9486, cmd, NULL, 0);

	/* Sleep Out */
	cmd = ILI9340_CMD_EXIT_SLEEP;
	ili9486_transmit(p_ili9486, cmd, NULL, 0);

	k_sleep(120);

	/* Display ON */
	cmd = ILI9340_CMD_DISPLAY_ON;
	ili9486_transmit(p_ili9486, cmd, NULL, 0);

}
