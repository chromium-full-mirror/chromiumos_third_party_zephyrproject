# SPDX-License-Identifier: Apache-2.0

string(TOLOWER "${BOARD}" BOARD_LOWER)

if(${BOARD_LOWER} STREQUAL "egis_et171")
    zephyr_library()
    zephyr_library_sources(board.c)
    zephyr_library_link_libraries(ET171HAL)
endif()
