/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_lcd_cam_rgb

#include <string.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/dt-bindings/display/panel.h>
#include <zephyr/kernel.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/sys/util.h>

#include <esp_clk_tree.h>
#include <esp_rom_sys.h>
#include <hal/lcd_hal.h>
#include <hal/lcd_ll.h>
#include <soc/clk_tree_defs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_esp32_lcd_cam_rgb, CONFIG_DISPLAY_LOG_LEVEL);

/* There is a single LCD_CAM peripheral */
#define LCD_BUS_ID 0

/* The GDMA reads PSRAM in bursts of up to 64 bytes */
#define FB_ALIGN 64

/* Only 16-bit RGB565 is streamed, over a 16-bit data bus */
#define BITS_PER_PIXEL  16
#define BYTES_PER_PIXEL (BITS_PER_PIXEL / 8)

/* How long a write waits for the buffer it reuses to leave the screen */
#define FRAME_WAIT_MS 100

struct display_esp32_rgb_config {
	const struct device *dma_dev;
	uint8_t dma_channel;
	int irq_source;
	int irq_priority;
	int irq_flags;
	uint16_t width;
	uint16_t height;
	uint32_t pclk_hz;
	uint16_t hsync_len;
	uint16_t hback_porch;
	uint16_t hfront_porch;
	uint16_t vsync_len;
	uint16_t vback_porch;
	uint16_t vfront_porch;
	bool hsync_active_high;
	bool vsync_active_high;
	bool de_active_high;
	bool pclk_drive_rising;
};

struct display_esp32_rgb_data {
	lcd_hal_context_t hal;
	struct k_spinlock lock;
	struct k_sem frame_sem;
	uint8_t *fb[CONFIG_DISPLAY_ESP32_LCD_CAM_RGB_FB_NUM];
	/* Bytes of pixel data in a frame, the buffers may be larger */
	size_t frame_size;
	/* Buffer on screen */
	uint8_t active_fb;
	/* Buffer to put on screen at the next frame start, or -1 */
	int8_t pending_fb;
	/* Buffer the next write draws into */
	uint8_t draw_fb;
	/* Whether the draw buffer holds the last presented frame */
	bool draw_fb_seeded;
};

static int display_esp32_rgb_dma_load(const struct device *dev, const uint8_t *fb)
{
	const struct display_esp32_rgb_config *cfg = dev->config;
	struct display_esp32_rgb_data *data = dev->data;
	int ret;

	ret = dma_reload(cfg->dma_dev, cfg->dma_channel, (uint32_t)fb, 0, data->frame_size);
	if (ret < 0) {
		return ret;
	}

	return dma_start(cfg->dma_dev, cfg->dma_channel);
}

/*
 * Each frame is one DMA transfer. The LCD generates the next frame on its own
 * once a frame ends, so the transfer is restarted in the vertical blanking,
 * from the buffer to show next.
 */
static void display_esp32_rgb_isr(void *arg)
{
	const struct device *dev = arg;
	struct display_esp32_rgb_data *data = dev->data;
	uint32_t status = lcd_ll_get_interrupt_status(data->hal.dev);
	k_spinlock_key_t key;

	lcd_ll_clear_interrupt_status(data->hal.dev, status);

	if ((status & LCD_LL_EVENT_VSYNC_END) == 0U) {
		return;
	}

	key = k_spin_lock(&data->lock);
	if (data->pending_fb >= 0) {
		data->active_fb = (uint8_t)data->pending_fb;
		data->pending_fb = -1;
	}
	k_spin_unlock(&data->lock, key);

	(void)display_esp32_rgb_dma_load(dev, data->fb[data->active_fb]);

	k_sem_give(&data->frame_sem);
}

static int display_esp32_rgb_wait_buffer_free(struct display_esp32_rgb_data *data, uint8_t index)
{
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(FRAME_WAIT_MS));

	while (true) {
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		bool busy = (index == data->active_fb) || (index == data->pending_fb);

		k_spin_unlock(&data->lock, key);

		if (!busy) {
			return 0;
		}

		if (k_sem_take(&data->frame_sem, sys_timepoint_timeout(end)) != 0 &&
		    sys_timepoint_expired(end)) {
			return -EAGAIN;
		}
	}
}

static void display_esp32_rgb_present(struct display_esp32_rgb_data *data, uint8_t index)
{
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->pending_fb = (int8_t)index;
	k_spin_unlock(&data->lock, key);
}

static int display_esp32_rgb_write(const struct device *dev, const uint16_t x, const uint16_t y,
				   const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct display_esp32_rgb_config *cfg = dev->config;
	struct display_esp32_rgb_data *data = dev->data;
	const size_t src_pitch = (size_t)desc->pitch * BYTES_PER_PIXEL;
	const size_t dst_pitch = (size_t)cfg->width * BYTES_PER_PIXEL;
	const bool multi = CONFIG_DISPLAY_ESP32_LCD_CAM_RGB_FB_NUM > 1;
	uint8_t *fb;
	int ret;

	if (buf == NULL) {
		return -EINVAL;
	}

	if ((uint32_t)x + desc->width > cfg->width || (uint32_t)y + desc->height > cfg->height) {
		LOG_ERR("Write %ux%u at (%u,%u) exceeds %ux%u panel", desc->width, desc->height, x,
			y, cfg->width, cfg->height);
		return -EINVAL;
	}

	if (desc->pitch < desc->width || src_pitch * desc->height > desc->buf_size) {
		LOG_ERR("Invalid buffer descriptor");
		return -EINVAL;
	}

	/* A frame drawn directly into a framebuffer from display_get_framebuffer() */
	for (uint8_t i = 0; i < ARRAY_SIZE(data->fb); i++) {
		if (buf != data->fb[i]) {
			continue;
		}

		if (x != 0U || y != 0U || desc->width != cfg->width ||
		    desc->height != cfg->height || desc->pitch != cfg->width) {
			return -EINVAL;
		}

		sys_cache_data_flush_range(data->fb[i], data->frame_size);
		if (multi) {
			display_esp32_rgb_present(data, i);
			data->draw_fb = (i + 1U) % ARRAY_SIZE(data->fb);
			data->draw_fb_seeded = false;
		}

		return 0;
	}

	fb = data->fb[data->draw_fb];

	if (multi && !data->draw_fb_seeded) {
		/* Let the buffer leave the screen, then bring it up to date with
		 * the frame on screen, so a partial update keeps the rest of it.
		 */
		ret = display_esp32_rgb_wait_buffer_free(data, data->draw_fb);
		if (ret < 0) {
			LOG_DBG("Timed out waiting for framebuffer %u", data->draw_fb);
			return ret;
		}

		memcpy(fb, data->fb[data->active_fb], data->frame_size);
		data->draw_fb_seeded = true;
	}

	for (uint16_t row = 0; row < desc->height; row++) {
		memcpy(fb + (size_t)(y + row) * dst_pitch + (size_t)x * BYTES_PER_PIXEL,
		       (const uint8_t *)buf + (size_t)row * src_pitch,
		       (size_t)desc->width * BYTES_PER_PIXEL);
	}

	if (multi && data->draw_fb_seeded && desc->frame_incomplete) {
		return 0;
	}

	/* The DMA reads the framebuffer from memory, not through the cache */
	if (multi) {
		sys_cache_data_flush_range(fb, data->frame_size);
		display_esp32_rgb_present(data, data->draw_fb);
		data->draw_fb = (data->draw_fb + 1U) % ARRAY_SIZE(data->fb);
		data->draw_fb_seeded = false;
	} else {
		sys_cache_data_flush_range(fb + (size_t)y * dst_pitch,
					   (size_t)desc->height * dst_pitch);
	}

	return 0;
}

static int display_esp32_rgb_read(const struct device *dev, const uint16_t x, const uint16_t y,
				  const struct display_buffer_descriptor *desc, void *buf)
{
	const struct display_esp32_rgb_config *cfg = dev->config;
	struct display_esp32_rgb_data *data = dev->data;
	const size_t src_pitch = (size_t)cfg->width * BYTES_PER_PIXEL;
	const size_t dst_pitch = (size_t)desc->pitch * BYTES_PER_PIXEL;
	const uint8_t *fb;
	k_spinlock_key_t key;

	if (buf == NULL) {
		return -EINVAL;
	}

	if ((uint32_t)x + desc->width > cfg->width || (uint32_t)y + desc->height > cfg->height ||
	    desc->pitch < desc->width || dst_pitch * desc->height > desc->buf_size) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	fb = data->fb[data->active_fb];
	k_spin_unlock(&data->lock, key);

	for (uint16_t row = 0; row < desc->height; row++) {
		memcpy((uint8_t *)buf + (size_t)row * dst_pitch,
		       fb + (size_t)(y + row) * src_pitch + (size_t)x * BYTES_PER_PIXEL,
		       (size_t)desc->width * BYTES_PER_PIXEL);
	}

	return 0;
}

static void *display_esp32_rgb_get_framebuffer(const struct device *dev)
{
	struct display_esp32_rgb_data *data = dev->data;

	return data->fb[data->draw_fb];
}

/* The scanout never stops, blanking is left to the panel and its backlight */
static int display_esp32_rgb_blanking_off(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int display_esp32_rgb_blanking_on(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static void display_esp32_rgb_get_capabilities(const struct device *dev,
					       struct display_capabilities *caps)
{
	const struct display_esp32_rgb_config *cfg = dev->config;

	memset(caps, 0, sizeof(*caps));
	caps->x_resolution = cfg->width;
	caps->y_resolution = cfg->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	caps->current_pixel_format = PIXEL_FORMAT_RGB_565;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	if (CONFIG_DISPLAY_ESP32_LCD_CAM_RGB_FB_NUM > 1) {
		caps->screen_info = SCREEN_INFO_DOUBLE_BUFFER;
	}
}

static int display_esp32_rgb_set_pixel_format(const struct device *dev,
					      const enum display_pixel_format pixel_format)
{
	ARG_UNUSED(dev);

	return pixel_format == PIXEL_FORMAT_RGB_565 ? 0 : -ENOTSUP;
}

static int display_esp32_rgb_set_orientation(const struct device *dev,
					     const enum display_orientation orientation)
{
	ARG_UNUSED(dev);

	return orientation == DISPLAY_ORIENTATION_NORMAL ? 0 : -ENOTSUP;
}

static DEVICE_API(display, display_esp32_rgb_api) = {
	.blanking_on = display_esp32_rgb_blanking_on,
	.blanking_off = display_esp32_rgb_blanking_off,
	.write = display_esp32_rgb_write,
	.read = display_esp32_rgb_read,
	.get_framebuffer = display_esp32_rgb_get_framebuffer,
	.get_capabilities = display_esp32_rgb_get_capabilities,
	.set_pixel_format = display_esp32_rgb_set_pixel_format,
	.set_orientation = display_esp32_rgb_set_orientation,
};

static int display_esp32_rgb_alloc(const struct device *dev)
{
	const struct display_esp32_rgb_config *cfg = dev->config;
	struct display_esp32_rgb_data *data = dev->data;
	size_t fb_size;

	data->frame_size = (size_t)cfg->width * cfg->height * BYTES_PER_PIXEL;
	fb_size = ROUND_UP(data->frame_size, FB_ALIGN);

	for (uint8_t i = 0; i < ARRAY_SIZE(data->fb); i++) {
		data->fb[i] =
			shared_multi_heap_aligned_alloc(SMH_REG_ATTR_EXTERNAL, FB_ALIGN, fb_size);
		if (data->fb[i] == NULL) {
			LOG_ERR("Failed to allocate framebuffer %u (%zu bytes)", i, fb_size);
			return -ENOMEM;
		}

		memset(data->fb[i], 0, fb_size);
		sys_cache_data_flush_range(data->fb[i], fb_size);
	}

	return 0;
}

static int display_esp32_rgb_setup_lcd(const struct device *dev)
{
	const struct display_esp32_rgb_config *cfg = dev->config;
	struct display_esp32_rgb_data *data = dev->data;
	lcd_hal_context_t *hal = &data->hal;
	uint32_t src_hz;
	uint32_t pclk_div;
	uint32_t clk_div;
	esp_err_t err;

	lcd_ll_enable_bus_clock(LCD_BUS_ID, true);
	lcd_ll_reset_register(LCD_BUS_ID);
	hal->dev = LCD_LL_GET_HW(LCD_BUS_ID);
	lcd_ll_enable_clock(hal->dev, true);

	err = esp_clk_tree_src_get_freq_hz((soc_module_clk_t)LCD_CLK_SRC_DEFAULT,
					   ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &src_hz);
	if (err != ESP_OK) {
		LOG_ERR("Failed to get the LCD clock source frequency (%d)", err);
		return -EINVAL;
	}

	/*
	 * PCLK = source / (clk_div * pclk_div). The pixel clock divider is kept
	 * at 2 when the LCD clock divider can reach the rate, as the ESP-IDF
	 * RGB panel driver does.
	 */
	pclk_div = MAX(2U, DIV_ROUND_UP(src_hz, cfg->pclk_hz * LCD_LL_CLK_FRAC_DIV_N_MAX));
	clk_div = DIV_ROUND_UP(src_hz, cfg->pclk_hz * pclk_div);
	if (pclk_div > LCD_LL_PCLK_DIV_MAX || clk_div < 2U) {
		LOG_ERR("Unsupported pixel clock %u Hz", cfg->pclk_hz);
		return -EINVAL;
	}

	lcd_ll_select_clk_src(hal->dev, LCD_CLK_SRC_DEFAULT);
	lcd_ll_set_group_clock_coeff(hal->dev, clk_div, 0, 0);
	lcd_ll_set_pixel_clock_prescale(hal->dev, pclk_div);
	LOG_DBG("Pixel clock %u Hz", src_hz / (clk_div * pclk_div));

	lcd_ll_set_clock_idle_level(hal->dev, false);
	lcd_ll_set_pixel_clock_edge(hal->dev, !cfg->pclk_drive_rising);

	lcd_ll_enable_rgb_mode(hal->dev, true);
	lcd_ll_enable_color_convert(hal->dev, false);
	lcd_ll_set_dma_read_stride(hal->dev, BITS_PER_PIXEL);
	lcd_ll_set_data_wire_width(hal->dev, BITS_PER_PIXEL);
	/* Data phase only, its length follows the frame timing */
	lcd_ll_set_phase_cycles(hal->dev, 0, 0, 1);
	lcd_ll_enable_output_always_on(hal->dev, true);

	lcd_ll_set_idle_level(hal->dev, !cfg->hsync_active_high, !cfg->vsync_active_high,
			      !cfg->de_active_high);
	lcd_ll_set_blank_cycles(hal->dev, 1, 1);
	lcd_ll_set_horizontal_timing(hal->dev, cfg->hsync_len, cfg->hback_porch, cfg->width,
				     cfg->hfront_porch);
	lcd_ll_set_vertical_timing(hal->dev, cfg->vsync_len, cfg->vback_porch, cfg->height,
				   cfg->vfront_porch);
	lcd_ll_enable_output_hsync_in_porch_region(hal->dev, true);
	lcd_ll_set_hsync_position(hal->dev, 0);
	lcd_ll_enable_auto_next_frame(hal->dev, true);

	lcd_ll_reset(hal->dev);
	lcd_ll_fifo_reset(hal->dev);

	return 0;
}

static int display_esp32_rgb_init(const struct device *dev)
{
	const struct display_esp32_rgb_config *cfg = dev->config;
	struct display_esp32_rgb_data *data = dev->data;
	struct dma_block_config dma_blk = {0};
	struct dma_config dma_cfg = {0};
	int ret;

	k_sem_init(&data->frame_sem, 0, K_SEM_MAX_LIMIT);
	data->pending_fb = -1;
	data->active_fb = 0;
	data->draw_fb = ARRAY_SIZE(data->fb) - 1U;

	if (!device_is_ready(cfg->dma_dev)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}

	ret = display_esp32_rgb_alloc(dev);
	if (ret < 0) {
		return ret;
	}

	ret = display_esp32_rgb_setup_lcd(dev);
	if (ret < 0) {
		return ret;
	}

	dma_blk.block_size = data->frame_size;
	dma_blk.source_address = (uint32_t)data->fb[data->active_fb];
	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.dma_slot = ESP_GDMA_TRIG_PERIPH_LCD0;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;

	ret = dma_config(cfg->dma_dev, cfg->dma_channel, &dma_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure DMA channel %u (%d)", cfg->dma_channel, ret);
		return ret;
	}

	ret = esp_intr_alloc_intrstatus(
		cfg->irq_source,
		ESP_PRIO_TO_FLAGS(cfg->irq_priority) | ESP_INT_FLAGS_CHECK(cfg->irq_flags),
		(uint32_t)lcd_ll_get_interrupt_status_reg(data->hal.dev), LCD_LL_EVENT_VSYNC_END,
		display_esp32_rgb_isr, (void *)dev, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to allocate interrupt (%d)", ret);
		return ret;
	}

	lcd_ll_clear_interrupt_status(data->hal.dev, UINT32_MAX);
	lcd_ll_enable_interrupt(data->hal.dev, LCD_LL_EVENT_VSYNC_END, true);

	ret = dma_start(cfg->dma_dev, cfg->dma_channel);
	if (ret < 0) {
		LOG_ERR("Failed to start DMA channel %u (%d)", cfg->dma_channel, ret);
		return ret;
	}

	/* Let the DMA fill the LCD FIFO before the first line goes out */
	esp_rom_delay_us(1);
	lcd_ll_start(data->hal.dev);

	return 0;
}

#define TIMINGS DT_INST_CHILD(0, display_timings)

BUILD_ASSERT(DT_INST_PROP(0, pixel_format) == PANEL_PIXEL_FORMAT_RGB_565,
	     "Only the RGB565 pixel format is supported");
BUILD_ASSERT(!DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lcd_cam_disp)),
	     "The LCD_CAM RGB and MIPI DBI interfaces cannot be used together");

static const struct display_esp32_rgb_config display_esp32_rgb_config = {
	.dma_dev = DEVICE_DT_GET(DT_DMAS_CTLR_BY_NAME(DT_INST_PARENT(0), tx)),
	.dma_channel = DT_DMAS_CELL_BY_NAME(DT_INST_PARENT(0), tx, channel),
	.irq_source = DT_IRQ_BY_IDX(DT_INST_PARENT(0), 0, irq),
	.irq_priority = DT_IRQ_BY_IDX(DT_INST_PARENT(0), 0, priority),
	.irq_flags = DT_IRQ_BY_IDX(DT_INST_PARENT(0), 0, flags),
	.width = DT_INST_PROP(0, width),
	.height = DT_INST_PROP(0, height),
	.pclk_hz = DT_PROP(TIMINGS, clock_frequency),
	.hsync_len = DT_PROP(TIMINGS, hsync_len),
	.hback_porch = DT_PROP(TIMINGS, hback_porch),
	.hfront_porch = DT_PROP(TIMINGS, hfront_porch),
	.vsync_len = DT_PROP(TIMINGS, vsync_len),
	.vback_porch = DT_PROP(TIMINGS, vback_porch),
	.vfront_porch = DT_PROP(TIMINGS, vfront_porch),
	.hsync_active_high = DT_PROP(TIMINGS, hsync_active) == 1,
	.vsync_active_high = DT_PROP(TIMINGS, vsync_active) == 1,
	.de_active_high = DT_PROP(TIMINGS, de_active) == 1,
	.pclk_drive_rising = DT_PROP(TIMINGS, pixelclk_active) == 1,
};

static struct display_esp32_rgb_data display_esp32_rgb_data;

DEVICE_DT_INST_DEFINE(0, display_esp32_rgb_init, NULL, &display_esp32_rgb_data,
		      &display_esp32_rgb_config, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,
		      &display_esp32_rgb_api);
