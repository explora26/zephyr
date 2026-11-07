.. zephyr:board:: m5stack_tab5

Overview
********

M5Stack Tab5 is a portable development terminal built around the ESP32-P4 SoC. It
pairs the SoC with 16 MB of flash and 32 MB of PSRAM, a 5-inch 1280x720 MIPI DSI
touch display, a 2 MP MIPI CSI camera, an ES8388 audio codec with an ES7210 microphone
front end, and an ESP32-C6-MINI-1U module for Wi-Fi 6 and Bluetooth LE. See the
`M5Stack Tab5 documentation`_ for details.

This board definition provides the high-performance (HP) core target. The Zephyr
console is routed to the ESP32-P4 USB Serial/JTAG controller.

Hardware
********

- ESP32-P4 SoC (pre-v3 silicon) with 32 MB PSRAM and 16 MB NOR flash
- 5-inch 720x1280 IPS display over 2-lane MIPI DSI; the display controller and touch
  controller depend on the board revision (see `Board Revisions`_)
- SC2356 2 MP camera over MIPI CSI
- ES8388 audio codec and ES7210 microphone ADC on I2S0 (MCLK=GPIO30, BCLK=GPIO27,
  WS=GPIO29, DOUT=GPIO26, DIN=GPIO28)
- BMI270 6-axis IMU, RX8130CE real-time clock and INA226 power monitor
- Two PI4IOE5V6408 I/O expanders (I2C addresses 0x43 and 0x44) that switch the
  display, touch, camera, speaker amplifier, ESP32-C6 and USB-A power rails
- MicroSD card slot (4-bit SDMMC: CLK=GPIO43, CMD=GPIO44, D0-D3=GPIO39-GPIO42),
  powered by the on-chip LDO_VO4 regulator
- ESP32-C6-MINI-1U wireless co-processor
- USB-C and USB-A ports, RS-485, Grove, M5-Bus and GPIO extension headers
- Reset and Boot buttons
- NP-F550 battery holder

The internal I2C bus (I2C0, SDA=GPIO31, SCL=GPIO32) carries the I/O expanders, the
IMU, the RTC, the audio codecs and the touch controller.

The camera, audio and the ESP32-C6 wireless co-processor are not enabled in this board
definition.

Board Revisions
===============

M5Stack has shipped the Tab5 with three display assemblies. They are described as board
revisions:

.. list-table::
   :header-rows: 1

   * - Revision
     - Display controller
     - Touch controller
     - Zephyr support
   * - ``1``
     - Ilitek ILI9881C
     - Goodix GT911 (I2C 0x14)
     - Display and touch not enabled
   * - ``2``
     - Sitronix ST7123
     - Integrated in ST7123 (I2C 0x55)
     - Display and touch not enabled
   * - ``3`` (default)
     - Sitronix ST7121
     - Integrated in ST7121 (I2C 0x55)
     - Display and touch

Select the revision with the board target, for example ``m5stack_tab5@1/esp32p4/hpcore``.
The ESP-IDF board support package reports the revision at boot as ``board version``.

On revision 3 the display runs in RGB565 over two data lanes at 965 Mbit/s per lane, with
the backlight on GPIO22 and the touch interrupt on GPIO23. The panel and touch supplies
are switched on by I/O expander 0x43 pins 4 and 5, and LDO_VO3 supplies the MIPI D-PHY
at 2.5 V.

.. include:: ../../../espressif/common/soc-esp32p4-features.rst
   :start-after: espressif-soc-esp32p4-features

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

To enter download mode, hold the Boot button while pressing Reset.

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`M5Stack Tab5 documentation`: https://docs.m5stack.com/en/core/Tab5
