# SPDX-License-Identifier: Apache-2.0

board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
if(CONFIG_STM32_MEMMAP OR CONFIG_BOOTLOADER_MCUBOOT)
  board_runner_args(stm32cubeprogrammer "--extload=mini_stm32h750_ext_loader.stldr")
endif()

board_runner_args(stlink_gdbserver "--extload=mini_stm32h750_ext_loader.stldr")
board_runner_args(jlink "--device=STM32H735IG" "--speed=4000")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
