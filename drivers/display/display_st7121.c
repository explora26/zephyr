/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sitronix_st7121

#include <string.h>

#include <zephyr/display/mipi_display.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(st7121, CONFIG_DISPLAY_LOG_LEVEL);

/* Delay after each initialization command, as used by the vendor sequence */
#define ST7121_CMD_DELAY_MS 5U

struct st7121_init_cmd {
	uint8_t cmd;
	uint8_t len;
	uint16_t delay_ms;
	const uint8_t *data;
};

#define ST7121_CMD(_cmd, _delay, ...)                                                              \
	{                                                                                          \
		.cmd = (_cmd),                                                                     \
		.len = sizeof((const uint8_t[]){__VA_ARGS__}),                                    \
		.delay_ms = (_delay),                                                              \
		.data = (const uint8_t[]){__VA_ARGS__},                                           \
	}

#define ST7121_CMD_NO_DATA(_cmd, _delay)                                                           \
	{                                                                                          \
		.cmd = (_cmd),                                                                     \
		.len = 0U,                                                                         \
		.delay_ms = (_delay),                                                              \
		.data = NULL,                                                                      \
	}

/* Panel initialization sequence of the M5Stack Tab5 720x1280 panel */
static const struct st7121_init_cmd st7121_init_cmds[] = {
	ST7121_CMD(0x60, 0, 0x71, 0x21, 0xA2),
	ST7121_CMD(0x60, 0, 0x71, 0x21, 0xA3),
	ST7121_CMD(0x60, 0, 0x71, 0x21, 0xA4),
	ST7121_CMD(0x78, 0, 0x21),
	ST7121_CMD(0x79, 0, 0xEF),
	ST7121_CMD(0xA4, 0, 0x31),
	ST7121_CMD(0xB7, 0, 0x00, 0x00, 0x5F, 0x5F, 0x44, 0x1A),
	ST7121_CMD(0xB0, 0, 0x22, 0x6B, 0x11, 0x89, 0x25, 0x43, 0x43),
	ST7121_CMD(0xBF, 0, 0xA7, 0xA7),
	ST7121_CMD(0xA5, 0, 0xF0, 0x03),
	ST7121_CMD(0xD7, 0, 0x10, 0x2C, 0x14, 0x2A, 0x80, 0x80),
	ST7121_CMD(0x90, 0, 0x71, 0x23, 0x5A, 0x20, 0x24, 0x11, 0x21),
	ST7121_CMD(0xA3, 0, 0x80, 0x01, 0x8C, 0xFF, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46,
		   0x00, 0x00, 0x1E, 0x5C, 0x1E, 0x80, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00,
		   0x00, 0x46, 0x00, 0x00, 0x1E, 0x5C, 0x1E, 0x80, 0x10, 0xEF, 0x58, 0x00, 0x00,
		   0x00, 0xFF),
	ST7121_CMD(0xA6, 0, 0x0A, 0x00, 0x24, 0x71, 0x36, 0x00, 0x00, 0x00, 0x68, 0x68, 0x91,
		   0xFF, 0x00, 0x24, 0x71, 0x37, 0x00, 0x00, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x00,
		   0x24, 0x71, 0x00, 0x00, 0x00, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x00, 0x2C, 0x71,
		   0x00, 0x01, 0x00, 0x00, 0x68, 0x68, 0xFF, 0xFF, 0x00, 0x08, 0x80, 0x08, 0x80,
		   0x06, 0x00, 0x00, 0x00, 0x00),
	ST7121_CMD(0xA7, 0, 0x1A, 0x1A, 0xC0, 0x64, 0x40, 0x04, 0x15, 0x40, 0x00, 0x40, 0x00,
		   0x68, 0x68, 0x91, 0xFF, 0x08, 0x80, 0x64, 0x40, 0x26, 0x37, 0x40, 0x00, 0x00,
		   0x00, 0x68, 0x68, 0x91, 0xFF, 0x08, 0x80, 0x64, 0x40, 0x8C, 0x9D, 0x40, 0x00,
		   0x00, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x08, 0x80, 0x64, 0x40, 0xAE, 0xBF, 0x00,
		   0x00, 0x20, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x08, 0x80, 0x79),
	ST7121_CMD(0xAC, 0, 0x1D, 0x18, 0x19, 0x1D, 0x18, 0x19, 0x04, 0x1C, 0x1D, 0x08, 0x0A,
		   0x10, 0x12, 0x0C, 0x0E, 0x14, 0x16, 0x00, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x18,
		   0x19, 0x1D, 0x18, 0x19, 0x06, 0x1C, 0x1D, 0x09, 0x0B, 0x11, 0x13, 0x0D, 0x0F,
		   0x15, 0x17, 0x02, 0x1D, 0x1D, 0x1D, 0x1D),
	ST7121_CMD(0xAD, 0, 0x0C, 0x40, 0x46, 0x00, 0x07, 0x4B, 0x4B, 0xFF, 0xFF, 0xF0, 0x40,
		   0x0E, 0x01, 0x07, 0x42, 0x42, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
		   0xFF),
	ST7121_CMD(0xAE, 0, 0xF0, 0xFF, 0x03, 0xF0, 0xFF, 0x03, 0x00),
	ST7121_CMD(0xB2, 0, 0x15, 0x19, 0x05, 0x23, 0x49, 0x2D, 0x03, 0x2E, 0x5C, 0xD2, 0xFF,
		   0x10, 0x60, 0xFD, 0x20, 0xC0, 0x00),
	ST7121_CMD(0xE8, 0, 0x20, 0x60, 0x04, 0x8E, 0x8E, 0x3E, 0x04, 0xDC, 0xDC, 0x3E, 0x06,
		   0xFA, 0x26, 0x3E),
	ST7121_CMD(0x75, 0, 0x03, 0x04),
	ST7121_CMD(0xE7, 0, 0x4B, 0x00, 0x00, 0xBE, 0x4B, 0x8C, 0x20, 0x1A, 0xF0, 0x7D, 0x14,
		   0x7D, 0x14, 0x7D, 0x14, 0x7D, 0x14, 0xFF, 0x00, 0x32, 0x30, 0x73, 0x00, 0x00,
		   0xC8, 0x6A, 0xFF, 0x5A, 0x64, 0x38, 0x88, 0x15, 0xB1, 0x01, 0x01, 0x64, 0x01,
		   0x01, 0x7C, 0xFF, 0x1A, 0x51),
	ST7121_CMD(0xE1, 0, 0x0C, 0x0C),
	ST7121_CMD(0xEA, 0, 0x15, 0x00, 0x01),
	ST7121_CMD(0xC8, 0, 0x00, 0x00, 0x04, 0x08, 0x10, 0x00, 0x1F, 0x01, 0x39, 0x3E, 0x00,
		   0x78, 0x06, 0xE2, 0x02, 0x11, 0x33, 0x01, 0x7A, 0x0D, 0x21, 0xC4, 0x0B, 0x19,
		   0x08, 0x32, 0xA0, 0x08, 0x1A, 0x0A, 0xF3, 0x7F, 0x0E, 0xC5, 0xE8, 0x03, 0xFF),
	ST7121_CMD(0xC9, 0, 0x00, 0x00, 0x04, 0x08, 0x10, 0x00, 0x1F, 0x01, 0x39, 0x3E, 0x00,
		   0x78, 0x06, 0xE2, 0x02, 0x11, 0x33, 0x01, 0x7A, 0x0D, 0x21, 0xC4, 0x0B, 0x19,
		   0x08, 0x32, 0xA0, 0x08, 0x1A, 0x0A, 0xF3, 0x7F, 0x0E, 0xC5, 0xE8, 0x03, 0xFF),
	ST7121_CMD(0x60, 0, 0x71, 0x21, 0x00),
	ST7121_CMD_NO_DATA(MIPI_DCS_EXIT_SLEEP_MODE, 80),
	ST7121_CMD_NO_DATA(MIPI_DCS_SET_DISPLAY_ON, 800),
	ST7121_CMD(MIPI_DCS_SET_TEAR_ON, 0, 0x00),
	ST7121_CMD_NO_DATA(MIPI_DCS_EXIT_INVERT_MODE, 0),
	ST7121_CMD(MIPI_DCS_SET_ADDRESS_MODE, 0, 0x00),
};

struct st7121_config {
	const struct device *mipi_dsi;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec bl_gpio;
	uint8_t data_lanes;
	uint8_t pixel_format;
	uint16_t width;
	uint16_t height;
	uint8_t channel;
	uint32_t hbp;
	uint32_t hsync;
	uint32_t hfp;
	uint32_t vbp;
	uint32_t vsync;
	uint32_t vfp;
};

static int st7121_blanking_off(const struct device *dev)
{
	const struct st7121_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 1);
	}

	return 0;
}

static int st7121_blanking_on(const struct device *dev)
{
	const struct st7121_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 0);
	}

	return 0;
}

static enum display_pixel_format st7121_display_format(uint8_t pixfmt)
{
	if (pixfmt == MIPI_DSI_PIXFMT_RGB565) {
		return PIXEL_FORMAT_RGB_565;
	}

	return PIXEL_FORMAT_RGB_888;
}

static void st7121_get_capabilities(const struct device *dev,
				    struct display_capabilities *capabilities)
{
	const struct st7121_config *config = dev->config;
	enum display_pixel_format format = st7121_display_format(config->pixel_format);

	memset(capabilities, 0, sizeof(*capabilities));
	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->supported_pixel_formats = format;
	capabilities->current_pixel_format = format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int st7121_set_pixel_format(const struct device *dev,
				   const enum display_pixel_format pixel_format)
{
	const struct st7121_config *config = dev->config;

	if (pixel_format == st7121_display_format(config->pixel_format)) {
		return 0;
	}

	return -ENOTSUP;
}

static int st7121_set_orientation(const struct device *dev,
				  const enum display_orientation orientation)
{
	ARG_UNUSED(dev);

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	return -ENOTSUP;
}

static DEVICE_API(display, st7121_api) = {
	.blanking_on = st7121_blanking_on,
	.blanking_off = st7121_blanking_off,
	.get_capabilities = st7121_get_capabilities,
	.set_pixel_format = st7121_set_pixel_format,
	.set_orientation = st7121_set_orientation,
};

static int st7121_reset(const struct device *dev)
{
	const struct st7121_config *config = dev->config;
	ssize_t ret;

	if (config->reset_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO (%d)", (int)ret);
			return (int)ret;
		}
		k_msleep(5);
		gpio_pin_set_dt(&config->reset_gpio, 1);
		k_msleep(10);
		gpio_pin_set_dt(&config->reset_gpio, 0);
	} else {
		ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel, MIPI_DCS_SOFT_RESET,
					 NULL, 0);
		if (ret < 0) {
			LOG_ERR("Soft reset failed (%d)", (int)ret);
			return (int)ret;
		}
	}

	k_msleep(120);

	return 0;
}

static int st7121_init(const struct device *dev)
{
	const struct st7121_config *config = dev->config;
	struct mipi_dsi_device mdev = {0};
	ssize_t ret;

	if (!device_is_ready(config->mipi_dsi)) {
		LOG_ERR("MIPI DSI host not ready");
		return -ENODEV;
	}

	if (config->bl_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->bl_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure backlight GPIO (%d)", (int)ret);
			return (int)ret;
		}
	}

	ret = st7121_reset(dev);
	if (ret < 0) {
		return (int)ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(st7121_init_cmds); i++) {
		const struct st7121_init_cmd *c = &st7121_init_cmds[i];

		ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel, c->cmd, c->data,
					 c->len);
		if (ret < 0) {
			LOG_ERR("Init command 0x%02x failed (%d)", c->cmd, (int)ret);
			return (int)ret;
		}
		k_msleep(c->delay_ms + ST7121_CMD_DELAY_MS);
	}

	mdev.pixfmt = config->pixel_format;
	mdev.data_lanes = config->data_lanes;
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST;
	mdev.timings.hactive = config->width;
	mdev.timings.hfp = config->hfp;
	mdev.timings.hbp = config->hbp;
	mdev.timings.hsync = config->hsync;
	mdev.timings.vactive = config->height;
	mdev.timings.vfp = config->vfp;
	mdev.timings.vbp = config->vbp;
	mdev.timings.vsync = config->vsync;

	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Failed to attach to MIPI DSI host (%d)", (int)ret);
		return (int)ret;
	}

	return 0;
}

#define ST7121_TIMING(inst, prop) DT_PROP(DT_INST_CHILD(inst, display_timings), prop)

#define ST7121_DEFINE(inst)                                                                        \
	BUILD_ASSERT(DT_INST_PROP_BY_IDX(inst, data_lanes, 0) == 2,                               \
		     "The ST7121 initialization sequence configures two data lanes");              \
	static const struct st7121_config st7121_config_##inst = {                                \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(inst)),                                      \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.bl_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, bl_gpios, {0}),                          \
		.data_lanes = DT_INST_PROP_BY_IDX(inst, data_lanes, 0),                            \
		.pixel_format = DT_INST_PROP(inst, pixel_format),                                  \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.channel = DT_INST_REG_ADDR(inst),                                                 \
		.hbp = ST7121_TIMING(inst, hback_porch),                                           \
		.hsync = ST7121_TIMING(inst, hsync_len),                                           \
		.hfp = ST7121_TIMING(inst, hfront_porch),                                          \
		.vbp = ST7121_TIMING(inst, vback_porch),                                           \
		.vsync = ST7121_TIMING(inst, vsync_len),                                           \
		.vfp = ST7121_TIMING(inst, vfront_porch),                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, st7121_init, NULL, NULL, &st7121_config_##inst, POST_KERNEL,  \
			      CONFIG_ST7121_INIT_PRIORITY, &st7121_api);

DT_INST_FOREACH_STATUS_OKAY(ST7121_DEFINE)
