cmake_minimum_required(VERSION 3.16)
### set the project name ###
project(ET171HAL VERSION 1.0.0 LANGUAGES C ASM)
set(TARGET ET171HAL)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "${CMAKE_BUILD_TYPE}")

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if("${CMAKE_BUILD_TYPE_UPPER}" STREQUAL "DEBUG")
    set(OPTIM " -Og")
else()
    set(OPTIM " -O3")
endif()

set(HAL_INCS
    inc/et171_hal
    )

set(HAL_SRCS
    src/et171_hal_smu.c
    src/et171_hal_otp.c
    src/et171_hal_utils.c
    )

set(HAL_C_FLAGS "-Wall -fomit-frame-pointer -fno-strict-aliasing -fno-builtin -ffunction-sections -fdata-sections -Wno-implicit-function-declaration -Wno-address-of-packed-member -mcmodel=medium ${OPTIM}")

add_library(${TARGET} STATIC ${HAL_SRCS})

include_directories(${HAL_INCS})
target_include_directories(${TARGET} PRIVATE internal)
add_compile_options(${HAL_C_FLAGS})
if(DEFINED CONFIG_FLOAT_HARD)
    target_compile_options(${TARGET} PRIVATE "-march=rv32imafc_zicsr_zifencei -mabi=ilp32f -mdiv -mfdiv")
else()
    target_compile_options(${TARGET} PRIVATE "-march=rv32imac_zicsr_zifencei -mabi=ilp32 -mdiv")
endif()
