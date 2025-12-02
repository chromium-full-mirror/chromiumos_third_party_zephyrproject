# SPDX-License-Identifier: Apache-2.0

add_subdirectory(usb)
add_subdirectory_ifdef(CONFIG_CLOCK_CONTROL clock_control)
add_subdirectory_ifdef(CONFIG_CRYPTO crypto)
add_subdirectory_ifdef(CONFIG_ENTROPY_GENERATOR entropy)
add_subdirectory_ifdef(CONFIG_HWINFO hwinfo)
add_subdirectory_ifdef(CONFIG_PINCTRL pinctrl)
