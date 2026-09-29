.. zephyr:board:: esp32s3_touch_lcd_2_8b

Overview
********

The `ESP32-S3-Touch-LCD-2.8B`_ is an ESP32-S3 development board from Waveshare with a 2.8-inch
480x640 IPS RGB LCD, capacitive touch panel, IMU, RTC, microSD card slot and Li-Po battery
charger.

Hardware
********

- ESP32-S3R8 SoC (8 MB octal PSRAM) with 16 MB flash
- 2.8-inch 480x640 RGB LCD panel (ST7701S controller, initialized over 3-wire SPI)
- Capacitive touch panel (GT911)
- TCA9554 I2C I/O expander (LCD reset, touch reset, LCD SPI chip select)
- QMI8658 6-axis IMU
- PCF85063 RTC
- microSD card slot
- BOOT and RESET buttons
- USB Type-C connector wired to the ESP32-S3 USB Serial/JTAG controller

Pin mapping:

+------------------------+-----------------------------------------+
| Function               | GPIO                                    |
+========================+=========================================+
| I2C SDA / SCL          | GPIO15 / GPIO7                          |
+------------------------+-----------------------------------------+
| Touch interrupt        | GPIO16                                  |
+------------------------+-----------------------------------------+
| LCD backlight (PWM)    | GPIO6                                   |
+------------------------+-----------------------------------------+
| LCD SPI SCK / SDA      | GPIO2 / GPIO1                           |
+------------------------+-----------------------------------------+
| LCD DE / HSYNC / VSYNC | GPIO40 / GPIO38 / GPIO39                |
+------------------------+-----------------------------------------+
| LCD PCLK               | GPIO41                                  |
+------------------------+-----------------------------------------+
| LCD R0-R4              | GPIO46, GPIO3, GPIO8, GPIO18, GPIO17    |
+------------------------+-----------------------------------------+
| LCD G0-G5              | GPIO14, GPIO13, GPIO12, GPIO11, GPIO10, |
|                        | GPIO9                                   |
+------------------------+-----------------------------------------+
| LCD B0-B4              | GPIO5, GPIO45, GPIO48, GPIO47, GPIO21   |
+------------------------+-----------------------------------------+
| UART0 TX / RX          | GPIO43 / GPIO44                         |
+------------------------+-----------------------------------------+
| BOOT button            | GPIO0                                   |
+------------------------+-----------------------------------------+

The TCA9554 I/O expander is at I2C address 0x20 and the GT911 touch controller at 0x5D.

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

The RGB LCD panel is not supported yet. The LCD backlight is available as the ``pwm-lcd0``
PWM LED.

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`ESP32-S3-Touch-LCD-2.8B`: https://docs.waveshare.com/ESP32-S3-Touch-LCD-2.8B
