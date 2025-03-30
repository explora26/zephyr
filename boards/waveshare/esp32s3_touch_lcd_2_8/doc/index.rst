.. _esp32s3_touch_lcd_2_b:

Waveshare ESP32-S3-Touch-LCD-2.8
#################################

Overview
********

ESP32-S3-Touch-LCD-2.8 is a microcontroller development board with 2.4GHz WiFi and Bluetooth BLE 5 
support. It integrates a 2.8-inch LCD screen with capacitive touch functionality, 16MB Flash and 8MB PSRAM. 
The board can smoothly run GUI programs and is suitable for HMI applications.

Features
========

* Processor: ESP32-S3, Xtensa 32-bit LX7 dual-core processor, up to 240 MHz
* Memory: 512KB SRAM, 384KB ROM, 16MB Flash, 8MB PSRAM
* Wireless: 2.4GHz Wi-Fi (802.11 b/g/n) and Bluetooth 5 (LE), with onboard antenna
* Display: 2.8-inch LCD display, 240×320 resolution, 262K color
* Touch: Capacitive touch with 5-point detection via I2C interface
* Peripherals:
   * QMI8658C 6-axis sensor (gyroscope and accelerometer)
   * PCF85063 RTC chip
   * PCM5101 audio decoder
   * TF card slot
   * Battery recharge management module
* Interfaces: UART, I2C, USB Type-C
* Buttons: BOOT Button, RESET Button
* Power: USB Type-C or 3.7V Lithium battery

.. figure:: img/esp32s3_touch_lcd_2_8.jpg
   :align: center
   :alt: ESP32-S3-Touch-LCD-2.8

   ESP32-S3-Touch-LCD-2.8 Development Board (Credit: Waveshare)

Hardware
********

The ESP32-S3 series is equipped with a Xtensa dual-core 32-bit LX7 microprocessor, operating at up to 240 MHz.
The ESP32-S3-Touch-LCD-2.8 board has 16MB external flash and 8MB PSRAM. 

The board features a 2.8-inch 240×320 LCD display with a ST7789 controller and capacitive touch control
with a CST328 controller.

Additionally, the board includes multiple sensors and peripherals:
- QMI8658C 6-axis IMU (accelerometer and gyroscope)
- PCF85063 RTC chip
- PCM5101 audio decoder
- TF card slot
- Battery management

.. figure:: img/esp32s3_touch_lcd_2_8_pinout.jpg
   :align: center
   :alt: ESP32-S3-Touch-LCD-2.8 Pinout

   ESP32-S3-Touch-LCD-2.8 Pinout (Credit: Waveshare)

For more details please refer to the `ESP32-S3-Touch-LCD-2.8 Wiki page`_.

Supported Features
=================

The ESP32-S3-Touch-LCD-2.8 board configuration supports the following hardware features:

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
| DISPLAY   | ST7789     | display                             |
+-----------+------------+-------------------------------------+
| Touch     | CST328     | touch controller                    |
+-----------+------------+-------------------------------------+
| IMU       | QMI8658C   | 6-axis sensor                       |
+-----------+------------+-------------------------------------+
| RTC       | PCF85063   | Real-time clock                     |
+-----------+------------+-------------------------------------+

System requirements
*******************

Prerequisites
============

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

Building & Flashing
*******************

ESP-IDF bootloader
==================

The board is using the ESP-IDF bootloader as the default 2nd stage bootloader.
It is build as a subproject at each application build. No further attention
is expected from the user.

MCUboot bootloader
=================

User may choose to use MCUboot bootloader instead. In that case the bootloader
must be build (and flash) at least once.

There are two options to be used when building an application:

1. Sysbuild
~~~~~~~~~~~

.. code-block:: console

   # West
   west build -b esp32s3_touch_lcd_2_8/esp32s3/procpu --sysbuild samples/hello_world
   # Sysbuild build system explicitly use --sysbuild
   west build --sysbuild -b esp32s3_touch_lcd_2_8/esp32s3/procpu samples/hello_world

2. Traditional build
~~~~~~~~~~~~~~~~~~~

.. code-block:: console

   west build -b esp32s3_touch_lcd_2_8/esp32s3/procpu samples/hello_world

From there, uses shall be able to easily flash using the command:

.. code-block:: console

   west flash

Debugging
*********

ESP32-S3-Touch-LCD-2.8 debugging is supported through the ESP-IDF monitor:

.. code-block:: console
   
   west build -b esp32s3_touch_lcd_2_8/esp32s3/procpu samples/hello_world
   west flash
   west espressif monitor

References
**********

.. target-notes::

.. _`ESP32-S3-Touch-LCD-2.8 Wiki page`: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.8 
