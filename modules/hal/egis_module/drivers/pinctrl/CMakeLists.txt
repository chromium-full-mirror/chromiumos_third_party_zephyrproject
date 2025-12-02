# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_PINCTRL_EGIS_ET171) 
  zephyr_library_sources(pinctrl_egis_et171.c)
  zephyr_library_link_libraries(ET171HAL)
endif()
