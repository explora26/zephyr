/*
 * Copyright (c) 2020 NXP
 * Copyright (c) 2020 Mark Olsson <mark@markolsson.se>
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2020 Harry Jiang <explora26@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xptek_xpt2046

#include <drivers/kscan.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(xpt2046, CONFIG_KSCAN_LOG_LEVEL);

/* XPT2046 used registers */
#define REG_TD_STATUS		0x02U
#define REG_P1_XH		0x03U

/* REG_TD_STATUS: Touch points. */
#define TOUCH_POINTS_POS	0U
#define TOUCH_POINTS_MSK	0x0FU

/* REG_Pn_XH: Events. */
#define EVENT_POS		6U
#define EVENT_MSK		0x03U

#define EVENT_PRESS_DOWN	0x00U
#define EVENT_LIFT_UP		0x01U
#define EVENT_CONTACT		0x02U
#define EVENT_NONE		0x03U

/* REG_Pn_XH: Position */
#define POSITION_H_MSK		0x0FU


#define CMD_X_READ  0x91
#define CMD_Y_READ  0xd1
#define CMD_Y_PD0_READ  0xd0
#define CMD_Z1_READ  0xb1
#define CMD_Z2_READ  0xc1
#define XPT2046_HOR_RES     480
#define XPT2046_VER_RES     320
#define XPT2046_X_MIN       400
#define XPT2046_Y_MIN       400
#define XPT2046_X_MAX       3800
#define XPT2046_Y_MAX       3800
#define XPT2046_AVG         4
#define XPT2046_INV         0
#define XPT2046_X_INV       0
#define XPT2046_Y_INV       1
#define Z_THRESHOLD     200
int16_t avg_buf_x[XPT2046_AVG];
int16_t avg_buf_y[XPT2046_AVG];
uint8_t avg_last;
static int16_t last_x = 0;
static int16_t last_y = 0;


/** XPT2046 configuration (DT). */
struct xpt2046_config {
	const char *gpio_port;
	uint8_t gpio_pin;
	gpio_dt_flags_t gpio_flags;
	const char *spi_port;
	gpio_pin_t spi_cs_pin;
	gpio_dt_flags_t spi_cs_dt_flags;
	const char *spi_cs_port;
	uint32_t spi_freq;
	uint8_t spi_slave;
};

/** XPT2046 data. */
struct xpt2046_data {
	/** Device pointer. */
	const struct device *dev;
	/** SPI controller device. */
	const struct device *gpio;
	const struct device *spi;
	struct spi_cs_control spi_cs;
	struct spi_config spi_cfg;
	struct gpio_callback gpio_cb;
	/** KSCAN Callback. */
	kscan_callback_t callback;
	/** Work queue (for deferred read). */
	struct k_work work;
};

static void xpt2046_corr(int16_t * x, int16_t * y)
{
#if XPT2046_XY_SWAP != 0
	int16_t swap_tmp;
	swap_tmp = *x;
	*x = *y;
	*y = swap_tmp;
#endif

	if((*x) > XPT2046_X_MIN)(*x) -= XPT2046_X_MIN;
	else(*x) = 0;

	if((*y) > XPT2046_Y_MIN)(*y) -= XPT2046_Y_MIN;
	else(*y) = 0;

	(*x) = (uint32_t)((uint32_t)(*x) * XPT2046_HOR_RES) /
		   (XPT2046_X_MAX - XPT2046_X_MIN);

	(*y) = (uint32_t)((uint32_t)(*y) * XPT2046_VER_RES) /
		   (XPT2046_Y_MAX - XPT2046_Y_MIN);

#if XPT2046_X_INV != 0
	(*x) =  XPT2046_HOR_RES - (*x);
#endif

#if XPT2046_Y_INV != 0
	(*y) =  XPT2046_VER_RES - (*y);
#endif
}

static void xpt2046_avg(int16_t * x, int16_t * y)
{
	/*Shift out the oldest data*/
	uint8_t i;
	for(i = XPT2046_AVG - 1; i > 0 ; i--) {
		avg_buf_x[i] = avg_buf_x[i - 1];
		avg_buf_y[i] = avg_buf_y[i - 1];
	}

	/*Insert the new point*/
	avg_buf_x[0] = *x;
	avg_buf_y[0] = *y;
	if(avg_last < XPT2046_AVG) avg_last++;

	/*Sum the x and y coordinates*/
	int32_t x_sum = 0;
	int32_t y_sum = 0;
	for(i = 0; i < avg_last ; i++) {
		x_sum += avg_buf_x[i];
		y_sum += avg_buf_y[i];
	}

	/*Normalize the sums*/
	(*x) = (int32_t)x_sum / avg_last;
	(*y) = (int32_t)y_sum / avg_last;
}


static int xpt2046_process(const struct device *dev)
{
	const struct xpt2046_config *config = dev->config;
	struct xpt2046_data *data = dev->data;
	uint8_t buffer_tx[1];
	uint8_t buffer_rx[3];

	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = sizeof(buffer_tx)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf = {
		.buf = buffer_rx,
		.len = sizeof(buffer_rx)
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	int16_t x = 0;
	int16_t y = 0;
	int16_t z1 = 0;
	int16_t z2 = 0;
	uint32_t Rt = 0;

	bool pressed = false;

//========================================
//
	while (!gpio_pin_get(data->gpio, config->gpio_pin)) {

		buffer_tx[0] = CMD_Z1_READ;
		spi_transceive(data->spi, &data->spi_cfg, &tx, &rx);
		z1 = ((buffer_rx[1] << 8) | buffer_rx[2]) >> 3;
		LOG_DBG("Touch Z1:%d", z1);

		buffer_tx[0] = CMD_Z2_READ;
		spi_transceive(data->spi, &data->spi_cfg, &tx, &rx);
		LOG_DBG("Touch Z2:%d", ((buffer_rx[1] << 8) | buffer_rx[2]) >> 3);
		z2 = ((buffer_rx[1] << 8) | buffer_rx[2]) >> 3;

		buffer_tx[0] = CMD_X_READ;
		spi_transceive(data->spi, &data->spi_cfg, &tx, &rx);
		x = ((buffer_rx[1] << 8) | buffer_rx[2]) >> 3;

		buffer_tx[0] = CMD_X_READ;
		spi_transceive(data->spi, &data->spi_cfg, &tx, &rx);

		buffer_tx[0] = CMD_Y_READ;
		spi_transceive(data->spi, &data->spi_cfg, &tx, &rx);
		y = ((buffer_rx[1] << 8) | buffer_rx[2]) >> 3;
		LOG_DBG("Touch Y:%d", y);

		buffer_tx[0] = CMD_X_READ;
		spi_transceive(data->spi, &data->spi_cfg, &tx, &rx);
		x = ((buffer_rx[1] << 8) | buffer_rx[2]) >> 3;
		LOG_DBG("Touch X:%d", x);

		buffer_tx[0] = CMD_Y_PD0_READ;
		spi_transceive(data->spi, &data->spi_cfg, &tx, &rx);

		Rt = z2;
		Rt -= z1;
		Rt *= x;
		Rt *= 60;
		Rt /= z1;
		Rt = (Rt + 2047) >> 12;

		LOG_DBG("Touch Rt:%d", Rt);

		xpt2046_corr(&x, &y);
		xpt2046_avg(&x, &y);

		last_x = x;
		last_y = y;
		pressed = true;

		LOG_DBG("Touch X: %d Y: %d Z1: %d Z2: %d pressed:%d", x, y, z1, z2, pressed);
		data->callback(dev, x, y, pressed);

		k_msleep(10);
	}

	x = last_x;
	y = last_y;
	pressed = false;
	avg_last = 0;
	LOG_DBG("Touch X: %d Y: %d Z1: %d Z2: %d pressed:%d", x, y, z1, z2, pressed);
	data->callback(dev, x, y, pressed);


	return 0;
}

static void xpt2046_work_handler(struct k_work *work)
{
	struct xpt2046_data *data = CONTAINER_OF(work, struct xpt2046_data, work);
	const struct xpt2046_config *config = data->dev->config;

	xpt2046_process(data->dev);

	gpio_pin_interrupt_configure(data->gpio, config->gpio_pin,
					 GPIO_INT_EDGE_FALLING);
}

static void xpt2046_isr_handler(const struct device *dev,
			       struct gpio_callback *cb, uint32_t pins)
{
	struct xpt2046_data *data = CONTAINER_OF(cb, struct xpt2046_data, gpio_cb);
	const struct xpt2046_config *config = data->dev->config;

	gpio_pin_interrupt_configure(data->gpio, config->gpio_pin, GPIO_INT_DISABLE);

	k_work_submit(&data->work);
}

static int xpt2046_configure(const struct device *dev,
			    kscan_callback_t callback)
{
	struct xpt2046_data *data = dev->data;

	if (!callback) {
		LOG_ERR("Invalid callback (NULL)");
		return -EINVAL;
	}

	data->callback = callback;

	return 0;
}

static int xpt2046_enable_callback(const struct device *dev)
{
	const struct xpt2046_config *config = dev->config;
	struct xpt2046_data *data = dev->data;
	uint8_t buffer_tx[1];
	uint8_t buffer_rx[3];
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = sizeof(buffer_tx)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf = {
		.buf = buffer_rx,
		.len = sizeof(buffer_rx)
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	buffer_tx[0] = CMD_Y_PD0_READ;
	spi_transceive(data->spi, &data->spi_cfg, &tx, &rx);

	if (gpio_pin_interrupt_configure(data->gpio, config->gpio_pin, GPIO_INT_EDGE_FALLING)) {
		LOG_ERR("Could not configure interrupt GPIO pin");
		return -EINVAL;
	}

	return 0;
}

static int xpt2046_disable_callback(const struct device *dev)
{
	struct xpt2046_data *data = dev->data;
	const struct xpt2046_config *config = data->dev->config;

	gpio_pin_interrupt_configure(data->gpio, config->gpio_pin, GPIO_INT_DISABLE);

	return 0;
}

static int xpt2046_init(const struct device *dev)
{
	const struct xpt2046_config *config = dev->config;
	struct xpt2046_data *data = dev->data;

	data->spi_cfg.operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |
			       SPI_TRANSFER_MSB | SPI_LINES_SINGLE);
	data->spi_cfg.frequency = config->spi_freq;
	data->spi_cfg.slave = config->spi_slave;

	data->spi = device_get_binding(config->spi_port);
	if (!data->spi) {
		LOG_ERR("Could not find SPI controller");
		return -ENODEV;
	}

	data->spi_cs.gpio_dev = device_get_binding(config->spi_cs_port);
	if (!data->spi_cs.gpio_dev) {
		LOG_ERR("SPI CS port %s not found", config->spi_cs_port);
		return -EINVAL;
	}

	data->spi_cs.gpio_pin = config->spi_cs_pin;
	data->spi_cs.gpio_dt_flags = config->spi_cs_dt_flags;
	data->spi_cfg.cs = &data->spi_cs;
	gpio_pin_configure(data->spi_cs.gpio_dev, data->spi_cs.gpio_pin, GPIO_OUTPUT_ACTIVE);


	gpio_pin_set(data->spi_cs.gpio_dev, data->spi_cs.gpio_pin, 1);
	LOG_ERR("SPI CS pin: %d", data->spi_cs.gpio_pin);
	k_msleep(1000);

	/* Initialize GPIO */
	data->gpio = device_get_binding(config->gpio_port);
	if (!data->gpio) {
		LOG_ERR("GPIO port %s not found", config->gpio_port);
		return -EINVAL;
	}

	if (gpio_pin_configure(data->gpio, config->gpio_pin,
			       GPIO_INPUT | config->gpio_flags)) {
		LOG_ERR("Unable to configure GPIO pin %u", config->gpio_pin);
		return -EINVAL;
	}

	gpio_init_callback(&data->gpio_cb, xpt2046_isr_handler,
			   BIT(config->gpio_pin));

	gpio_add_callback(data->gpio, &data->gpio_cb);

	data->dev = dev;

	k_work_init(&data->work, xpt2046_work_handler);

	return 0;
}

static const struct kscan_driver_api xpt2046_driver_api = {
	.config = xpt2046_configure,
	.enable_callback = xpt2046_enable_callback,
	.disable_callback = xpt2046_disable_callback,
};

#define XPT2046_DEFINE_CONFIG(index)					       \
	static const struct xpt2046_config xpt2046_config_##index = {	       \
	.gpio_port = DT_INST_GPIO_LABEL(index, int_gpios),       \
	.gpio_pin = DT_INST_GPIO_PIN(index, int_gpios),       \
	.gpio_flags = DT_INST_GPIO_FLAGS(index, int_gpios),       \
	.spi_port = DT_INST_BUS_LABEL(index),       \
	.spi_freq  = DT_INST_PROP(index, spi_max_frequency),       \
	.spi_slave = DT_INST_REG_ADDR(index),       \
	.spi_cs_port = DT_INST_SPI_DEV_CS_GPIOS_LABEL(index),       \
	.spi_cs_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(index),       \
	.spi_cs_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(index),       \
	}

#define XPT2046_INIT(index)                                                     \
	XPT2046_DEFINE_CONFIG(index);					       \
	static struct xpt2046_data xpt2046_data_##index;			       \
	DEVICE_AND_API_INIT(xpt2046_##index, DT_INST_LABEL(index), xpt2046_init, \
			    &xpt2046_data_##index, &xpt2046_config_##index,      \
			    POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,	       \
			    &xpt2046_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XPT2046_INIT)
