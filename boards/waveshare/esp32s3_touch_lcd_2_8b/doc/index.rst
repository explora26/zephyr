.. zephyr:board:: esp32s3_touch_lcd_2_8b

Overview
********

The `ESP32-S3-Touch-LCD-2.8B`_ is an ESP32-S3 development board from Waveshare with a 2.8-inch
480x640 IPS RGB LCD, capacitive touch panel, IMU, RTC, microSD card slot and Li-Po battery
charger.

The schematic can be found in the `ESP32-S3-Touch-LCD-2.8B Schematic`_ reference.

Hardware
********

- ESP32-S3R8 SoC (8 MB octal PSRAM) with 16 MB flash
- 2.8-inch 480x640 RGB LCD panel (ST7701S controller, initialized over 3-wire SPI)
- Capacitive touch panel (GT911)
- TCA9554 I2C I/O expander for reset, chip select and interrupt lines
- QMI8658C 6-axis IMU
- PCF85063A RTC
- microSD card slot (SPI mode, shared with the LCD configuration interface)
- Li-Po battery charger with battery voltage measurement
- Buzzer
- BOOT and RESET buttons
- USB Type-C connector wired to the ESP32-S3 USB Serial/JTAG controller

Pin mapping:

+------------------------+-----------------------------------------+
| Function               | GPIO                                    |
+========================+=========================================+
| I2C SDA / SCL          | GPIO15 / GPIO7                          |
+------------------------+-----------------------------------------+
| SPI SCK / MOSI / MISO  | GPIO2 / GPIO1 / GPIO42                  |
+------------------------+-----------------------------------------+
| Battery voltage (ADC)  | GPIO4 (ADC1 channel 3, 1/3 divider)     |
+------------------------+-----------------------------------------+
| Touch interrupt        | GPIO16                                  |
+------------------------+-----------------------------------------+
| LCD backlight (PWM)    | GPIO6                                   |
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

I2C devices: TCA9554 I/O expander (0x20), GT911 touch controller (0x5D), QMI8658C IMU (0x6A)
and PCF85063A RTC (0x51).

TCA9554 I/O expander pins:

+------+------------------------+
| Pin  | Function               |
+======+========================+
| P0   | LCD reset              |
+------+------------------------+
| P1   | Touch reset            |
+------+------------------------+
| P2   | LCD SPI chip select    |
+------+------------------------+
| P3   | SD card chip select    |
+------+------------------------+
| P4   | IMU INT1               |
+------+------------------------+
| P5   | IMU INT2               |
+------+------------------------+
| P6   | RTC INT                |
+------+------------------------+
| P7   | Buzzer                 |
+------+------------------------+

The TCA9554 interrupt output is not connected to the ESP32-S3, so the IMU and RTC interrupts
cannot be used.

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

The RGB LCD panel is not supported yet. The LCD backlight is available as the ``pwm-lcd0``
PWM LED.

Battery state-of-charge estimation is provided by the ``fuel_gauge`` node, which is disabled by
default. Enable it in an overlay and adapt ``charge-full-design-microamp-hours`` to the battery.

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
.. _`ESP32-S3-Touch-LCD-2.8B Schematic`: https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.8B/ESP32-S3-Touch-LCD-2.8B_schematic_diagram.pdf
