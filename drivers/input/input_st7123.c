/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sitronix_st7123_touch

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_touch.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(st7123_touch, CONFIG_INPUT_LOG_LEVEL);

#define ST7123_REG_FW_VERSION    0x0000U
#define ST7123_REG_MAX_X_H       0x0005U
#define ST7123_REG_MAX_TOUCHES   0x0009U
#define ST7123_REG_ADV_INFO      0x0010U
#define ST7123_REG_REPORT_COORD  0x0014U

/* ST7123_REG_ADV_INFO: coordinate reports are present */
#define ST7123_ADV_INFO_WITH_COORD BIT(3)

/* Coordinate report layout */
#define ST7123_REPORT_SIZE   7U
#define ST7123_REPORT_VALID  BIT(7)
#define ST7123_REPORT_X_H    GENMASK(5, 0)
#define ST7123_MAX_REPORTS   10U

struct st7123_config {
	struct input_touchscreen_common_config common;
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
#ifdef CONFIG_INPUT_ST7123_INTERRUPT
	struct gpio_dt_spec int_gpio;
#endif
};

struct st7123_data {
	const struct device *dev;
	struct k_work work;
#ifdef CONFIG_INPUT_ST7123_INTERRUPT
	struct gpio_callback int_gpio_cb;
#else
	struct k_timer timer;
#endif
	bool pressed;
};

INPUT_TOUCH_STRUCT_CHECK(struct st7123_config);

static int st7123_read(const struct device *dev, uint16_t reg, uint8_t *buf, size_t len)
{
	const struct st7123_config *config = dev->config;
	uint8_t addr[2];

	sys_put_be16(reg, addr);

	return i2c_write_read_dt(&config->bus, addr, sizeof(addr), buf, len);
}

static int st7123_process(const struct device *dev)
{
	struct st7123_data *data = dev->data;
	uint8_t reports[ST7123_MAX_REPORTS * ST7123_REPORT_SIZE];
	uint8_t adv_info;
	uint8_t count = 0U;
	int ret;

	ret = st7123_read(dev, ST7123_REG_ADV_INFO, &adv_info, sizeof(adv_info));
	if (ret < 0) {
		return ret;
	}

	if ((adv_info & ST7123_ADV_INFO_WITH_COORD) != 0U) {
		ret = st7123_read(dev, ST7123_REG_MAX_TOUCHES, &count, sizeof(count));
		if (ret < 0) {
			return ret;
		}

		count = MIN(count, ST7123_MAX_REPORTS);
		if (count > 0U) {
			ret = st7123_read(dev, ST7123_REG_REPORT_COORD, reports,
					  (size_t)count * ST7123_REPORT_SIZE);
			if (ret < 0) {
				return ret;
			}
		}
	}

	/* Report the first valid contact */
	for (uint8_t i = 0U; i < count; i++) {
		const uint8_t *report = &reports[i * ST7123_REPORT_SIZE];

		if ((report[0] & ST7123_REPORT_VALID) == 0U) {
			continue;
		}

		uint16_t x = (FIELD_GET(ST7123_REPORT_X_H, report[0]) << 8) | report[1];
		uint16_t y = sys_get_be16(&report[2]);

		input_touchscreen_report_pos(dev, x, y, K_FOREVER);
		input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
		data->pressed = true;

		return 0;
	}

	if (data->pressed) {
		input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
		data->pressed = false;
	}

	return 0;
}

static void st7123_work_handler(struct k_work *work)
{
	struct st7123_data *data = CONTAINER_OF(work, struct st7123_data, work);
	int ret;

	ret = st7123_process(data->dev);
	if (ret < 0) {
		LOG_DBG("Failed to read touch data (%d)", ret);
	}
}

#ifdef CONFIG_INPUT_ST7123_INTERRUPT
static void st7123_isr_handler(const struct device *port, struct gpio_callback *cb,
			       uint32_t pins)
{
	struct st7123_data *data = CONTAINER_OF(cb, struct st7123_data, int_gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}
#else
static void st7123_timer_handler(struct k_timer *timer)
{
	struct st7123_data *data = CONTAINER_OF(timer, struct st7123_data, timer);

	k_work_submit(&data->work);
}
#endif

static int st7123_log_info(const struct device *dev)
{
	uint8_t version;
	uint8_t info[5];
	int ret;

	ret = st7123_read(dev, ST7123_REG_FW_VERSION, &version, sizeof(version));
	if (ret < 0) {
		return ret;
	}

	ret = st7123_read(dev, ST7123_REG_MAX_X_H, info, sizeof(info));
	if (ret < 0) {
		return ret;
	}

	LOG_INF("Firmware version %u, max X %u, max Y %u, max touches %u", version,
		sys_get_be16(&info[0]), sys_get_be16(&info[2]), info[4]);

	return 0;
}

static int st7123_init(const struct device *dev)
{
	const struct st7123_config *config = dev->config;
	struct st7123_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR_DEVICE_NOT_READY(config->bus.bus);
		return -ENODEV;
	}

	data->dev = dev;
	k_work_init(&data->work, st7123_work_handler);

	if (config->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(config->reset_gpio.port);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}
		k_msleep(10);

		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret < 0) {
			return ret;
		}
		k_msleep(10);
	}

	ret = st7123_log_info(dev);
	if (ret < 0) {
		LOG_ERR("Failed to read controller information (%d)", ret);
		return ret;
	}

#ifdef CONFIG_INPUT_ST7123_INTERRUPT
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(config->int_gpio.port);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO (%d)", ret);
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, st7123_isr_handler, BIT(config->int_gpio.pin));
	ret = gpio_add_callback_dt(&config->int_gpio, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not add interrupt callback (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt (%d)", ret);
		return ret;
	}
#else
	k_timer_init(&data->timer, st7123_timer_handler, NULL);
	k_timer_start(&data->timer, K_MSEC(CONFIG_INPUT_ST7123_PERIOD_MS),
		      K_MSEC(CONFIG_INPUT_ST7123_PERIOD_MS));
#endif

	return 0;
}

#define ST7123_DEFINE(inst)                                                                        \
	static const struct st7123_config st7123_config_##inst = {                                \
		.common = INPUT_TOUCH_DT_INST_COMMON_CONFIG_INIT(inst),                            \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		IF_ENABLED(CONFIG_INPUT_ST7123_INTERRUPT,                                          \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),))                  \
	};                                                                                         \
	static struct st7123_data st7123_data_##inst;                                              \
	DEVICE_DT_INST_DEFINE(inst, st7123_init, NULL, &st7123_data_##inst,                        \
			      &st7123_config_##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,      \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(ST7123_DEFINE)
