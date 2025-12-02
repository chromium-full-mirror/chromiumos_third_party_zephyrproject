# Copyright (c) 2025 Egis Technology Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/usb/udc/)

if(CONFIG_UDC_ET171)
    zephyr_library_sources(udc_et171.c)
    zephyr_library_link_libraries(ET171HAL)
endif()
