.. _m5stack_dinmeter:

M5Stack DinMeter
################

Overview
********

The `M5Stack DinMeter`_ is a 1/32 DIN standard embedded development module
based on the M5StampS3A (ESP32-S3FN8). It features a 1.14-inch ST7789 LCD,
a rotary encoder with push button, an onboard WS2812 RGB LED, a buzzer,
a BM8563 RTC, and supports 6-36V DC input power.

.. figure:: img/m5stack_dinmeter.webp
   :width: 400px
   :align: center
   :alt: M5Stack DinMeter

Hardware
********

- ESP32-S3FN8 dual-core Xtensa LX7 @ 240 MHz
- 8 MB Flash, no PSRAM
- 1.14-inch ST7789 LCD (135x240), SPI
- Rotary encoder (A=GPIO40, B=GPIO41) with push button (GPIO42)
- WS2812 RGB LED (GPIO21)
- Buzzer / PWM (GPIO3)
- BM8563 RTC (I2C1, address 0x51)
- Internal I2C1: SDA=GPIO11, SCL=GPIO12
- External I2C0 / PORT.A: SDA=GPIO13, SCL=GPIO15
- UART0: TX=GPIO43, RX=GPIO44
- Power hold: GPIO46 (must be HIGH to stay powered)
- USB (ESP32-S3 native USB)
- Wi-Fi 2.4 GHz 802.11 b/g/n, BLE 5.0

Supported Features
******************

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| PWM/LEDC  | on-chip    | pwm (backlight, buzzer)             |
+-----------+------------+-------------------------------------+
| Display   | ST7789     | mipi-dbi / display                  |
+-----------+------------+-------------------------------------+
| RTC       | BM8563     | rtc (nxp,pcf8563)                   |
+-----------+------------+-------------------------------------+
| Encoder   | PCNT       | sensor (quadrature decoder)         |
+-----------+------------+-------------------------------------+
| LED Strip | WS2812     | led_strip (worldsemi,ws2812-spi)     |
+-----------+------------+-------------------------------------+
| Wi-Fi     | on-chip    | wifi                                |
+-----------+------------+-------------------------------------+
| Bluetooth | on-chip    | BLE                                 |
+-----------+------------+-------------------------------------+

Pin Mapping
***********

Display (SPI2)
==============

+-------+--------+--------------------------------------+
| Name  | GPIO   | Description                          |
+=======+========+======================================+
| MOSI  | GPIO5  | SPI2 MOSI                            |
+-------+--------+--------------------------------------+
| SCLK  | GPIO6  | SPI2 clock                           |
+-------+--------+--------------------------------------+
| CS    | GPIO7  | SPI2 chip select                     |
+-------+--------+--------------------------------------+
| DC    | GPIO4  | Data/Command select                  |
+-------+--------+--------------------------------------+
| RST   | GPIO8  | Reset (active low)                   |
+-------+--------+--------------------------------------+
| BL    | GPIO9  | Backlight (LEDC channel 0)           |
+-------+--------+--------------------------------------+

Encoder
=======

+--------+--------+--------------------------------------+
| Name   | GPIO   | Description                          |
+========+========+======================================+
| A      | GPIO40 | PCNT0 CH0 signal                     |
+--------+--------+--------------------------------------+
| B      | GPIO41 | PCNT0 CH0 control                    |
+--------+--------+--------------------------------------+
| Button | GPIO42 | Encoder push button (active low)     |
+--------+--------+--------------------------------------+

Programming and Debugging
*************************

Build the application for the ``m5stack_dinmeter/esp32s3/procpu`` target:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: m5stack_dinmeter/esp32s3/procpu
   :goals: build flash

References
**********

.. _M5Stack DinMeter:
   https://docs.m5stack.com/en/core/M5DinMeter
