# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_ENTROPY_EGIS_ET171_TRNG)
  zephyr_library_sources(entropy_egis.c)
  zephyr_library_link_libraries(ET171HAL)
endif()
