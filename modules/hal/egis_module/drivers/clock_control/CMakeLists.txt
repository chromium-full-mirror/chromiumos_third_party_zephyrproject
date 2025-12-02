# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_CLOCK_CONTROL_ET171)
  zephyr_library_sources(clock_control_et171.c)
  zephyr_library_link_libraries(ET171HAL)
endif()

