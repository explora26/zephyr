.. zephyr:board:: m5stack_atoms3r

Overview
********

M5Stack AtomS3R is an ESP32-S3 based development board from M5Stack with
integrated PSRAM.

Hardware
********

The board peripherals:

- 8MB of Flash
- 8MB of PSRAM
- LCD IPS TFT 0.85", 128x128 px screen (ST7735S / GC9107 compatible)
- 6-axis IMU MPU6886
- Infrared emitter
- Grove Port A (I2C)

.. include:: ../../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

.. include:: ../../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

M5Stack AtomS3R debugging is not supported due to pinout limitations.

Related Documents
*****************

.. target-notes::

.. _`M5Stack AtomS3R documentation`: https://docs.m5stack.com/en/core/AtomS3R
