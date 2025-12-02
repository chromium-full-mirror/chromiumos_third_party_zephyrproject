# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(soc.c)

if(CONFIG_PM)
  zephyr_sources(power.c)
endif()
zephyr_library_link_libraries(ET171HAL)