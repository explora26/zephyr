/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pwm.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(board, CONFIG_LOG_DEFAULT_LEVEL);

/* The display drivers do not control the backlight, turn it fully on */
void board_late_init_hook(void)
{
	const struct pwm_dt_spec backlight = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_lcd0));
	int ret;

	if (!pwm_is_ready_dt(&backlight)) {
		LOG_ERR("Backlight PWM is not ready");
		return;
	}

	ret = pwm_set_pulse_dt(&backlight, backlight.period);
	if (ret < 0) {
		LOG_ERR("Failed to turn on the backlight (%d)", ret);
	}
}
