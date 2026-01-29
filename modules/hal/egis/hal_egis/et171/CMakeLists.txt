#
# Copyright 2025 Egis Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

set(CURRENT_MODULE_DIR "../..")
if(CONFIG_HAS_EGIS_ET171_HAL)

  add_library(ET171HAL INTERFACE)
  target_include_directories(ET171HAL INTERFACE ${CURRENT_MODULE_DIR}/et171/inc)
  target_link_directories(ET171HAL INTERFACE ${CURRENT_MODULE_DIR}/et171/lib)

  if(DEFINED CONFIG_FLOAT_HARD)
    target_link_libraries(ET171HAL INTERFACE et171hal_ilp32f)
  else()
    target_link_libraries(ET171HAL INTERFACE et171hal_ilp32)
  endif()

  if(CONFIG_HAS_EGIS_ET171_SXSMCRYPT)
    if(DEFINED CONFIG_FLOAT_HARD)
      target_link_libraries(ET171HAL INTERFACE sxsymcrypt_ilp32f)
    else()
      target_link_libraries(ET171HAL INTERFACE sxsymcrypt_ilp32)
    endif()
  endif() # CONFIG_HAS_EGIS_ET171_SXSMCRYPT

  if(CONFIG_HAS_EGIS_ET171_USBCORE)
    target_include_directories(ET171HAL INTERFACE inc)
    if(DEFINED CONFIG_FLOAT_HARD)
      target_link_libraries(ET171HAL INTERFACE usbcore_ilp32f)
    else()
      target_link_libraries(ET171HAL INTERFACE usbcore_ilp32)
    endif()
  endif() # CONFIG_HAS_EGIS_ET171_USBCORE

  if(CONFIG_HAS_EGIS_ET171_SXSMCRYPT OR CONFIG_HAS_EGIS_ET171_USBCORE)
    zephyr_library()
    zephyr_compile_options(-O2)
    zephyr_library_include_directories(inc ${CURRENT_MODULE_DIR}/et171/inc)
    if(CONFIG_HAS_EGIS_ET171_SXSMCRYPT)
      zephyr_library_sources( src/sxs_baremetal.c )
    endif()
    if(CONFIG_HAS_EGIS_ET171_USBCORE)
      if(CONFIG_XIP)
        zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x1 src/usb_mem.ld)
      else()
        # The section `.usb_mem` must be localed before 0x20000.
        zephyr_linker_sources(ROM_START SORT_KEY 0x1 src/usb_mem.ld)
      endif()
      zephyr_library_sources( src/usb_mem.c
                              src/usb_cps.c )
    endif()
  endif()

endif()
