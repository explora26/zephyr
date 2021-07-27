/*
 * Copyright (c) 2020 Google LLC.
 * Copyright (c) 2021 Harry Jiang
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int board_pinmux_init(const struct device *dev)
{
	__unused const struct device *muxa = DEVICE_DT_GET(DT_NODELABEL(pinmux_a));
	__unused const struct device *muxb = DEVICE_DT_GET(DT_NODELABEL(pinmux_b));
	__unused const struct device *muxc = DEVICE_DT_GET(DT_NODELABEL(pinmux_c));

	__ASSERT_NO_MSG(device_is_ready(muxa));
	__ASSERT_NO_MSG(device_is_ready(muxb));

	ARG_UNUSED(dev);

#if ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
	pinmux_pin_set(muxb, 26, PINMUX_FUNC_C);
	pinmux_pin_set(muxb, 27, PINMUX_FUNC_C);
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif

#if ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif

#if defined(CONFIG_PWM_SAM0_TCC)
#if ATMEL_SAM0_DT_TCC_CHECK(0, atmel_sam0_tcc_pwm)
	/* TCC0/WO[2] on PA22 (LED) */
	pinmux_pin_set(muxa, 22, PINMUX_FUNC_G);
#endif
#if ATMEL_SAM0_DT_TCC_CHECK(1, atmel_sam0_tcc_pwm)
	/* TCC1/WO[2] on PA18 (D7) */
	pinmux_pin_set(muxa, 18, PINMUX_FUNC_F);
	/* TCC1/WO[3] on PA19 (D9) */
	pinmux_pin_set(muxa, 19, PINMUX_FUNC_F);
#endif
#endif

	if (IS_ENABLED(CONFIG_USB_DC_SAM0)) {
		/* USB DP on PA25, USB DM on PA24 */
		pinmux_pin_set(muxa, 25, PINMUX_FUNC_H);
		pinmux_pin_set(muxa, 24, PINMUX_FUNC_H);
	}

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
