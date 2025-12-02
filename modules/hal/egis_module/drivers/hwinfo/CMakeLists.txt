# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_HWINFO_ET171)
  zephyr_library_sources(hwinfo_et171.c)
  zephyr_library_link_libraries(ET171HAL)
endif()
