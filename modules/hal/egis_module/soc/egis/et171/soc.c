/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/devicetree.h>

/* ================================================================== ET171 HAL
 * et171 core
 */
#include <et171_hal/et171.h>
#include <et171_hal/et171_hal_smu.h>
#include <et171_hal/et171_hal_otp.h>

static inline void disable_unused_device()
{
	const uint32_t unused_ip = 0
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(usb), okay) || 1 /* Egis's usb driver will handle it by itself */
		| SMU_RST_USB2
#endif
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(spis), okay)
		| SMU_RST_SPIS
#endif
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(spi0), okay) && !CONFIG_XIP
		| SMU_RST_SPIM1
#endif
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay)
		| SMU_RST_SPIM2
#endif
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(spi2), okay)
		| SMU_RST_SPIM3
#endif
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(rtc0), okay)
		| SMU_RST_RTC
#endif
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(pit0), okay) && !DT_NODE_EXISTS(DT_NODELABEL(ext_wdt))
		| SMU_RST_PITPWM
#endif
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay) && 0 /* keep enable, i2c is using to workaround for WFI issue */
		| SMU_RST_I2C
#endif
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
		| SMU_RST_UART
#endif
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
		| SMU_RST_GPIO
#endif
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(wdt), okay)
		| SMU_RST_WDT
#endif
#if !CONFIG_HAS_EGIS_ET171_SXSMCRYPT
		| SMU_RST_CRYPTO
#endif
		;
	if (unused_ip)
		HAL_SMU_ResetLow(unused_ip);
}

static int soc_pre_init(void)
{
#if CONFIG_XIP && CONFIG_ICACHE
	/* Since caching is enabled before z_data_copy(), RAM functions may still be cached
	 * in the d-cache instead of being written to SRAM. In this case, the i-cache will
	 * fetch the wrong content from SRAM. Thus, using "fence.i" to fix it.
	 */
	__asm__ volatile("fence.i" ::: "memory");
#endif

	/* Load Analog triming value from OTP */
	HAL_OTP_LoadAnalogConfig();

	/* Load Root clock triming value from OTP */
	HAL_OTP_GetRootClock();

#define AHB_BASE_CLOCK (200 * 1000000) /* 200MHz */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clock), okay)
#   if DT_NODE_HAS_PROP(DT_NODELABEL(clock), ahb_divide)
#         if DT_PROP(DT_NODELABEL(clock), ahb_divide) == 1
#           define SET_AHB_DIV ROOT_CLK_DIV_1
#       elif DT_PROP(DT_NODELABEL(clock), ahb_divide) == 2
#           define SET_AHB_DIV ROOT_CLK_DIV_2
#       elif DT_PROP(DT_NODELABEL(clock), ahb_divide) == 3
#           define SET_AHB_DIV ROOT_CLK_DIV_3
#       elif DT_PROP(DT_NODELABEL(clock), ahb_divide) == 4
#           define SET_AHB_DIV ROOT_CLK_DIV_4
#       elif DT_PROP(DT_NODELABEL(clock), ahb_divide) == 6
#           define SET_AHB_DIV ROOT_CLK_DIV_6
#       elif DT_PROP(DT_NODELABEL(clock), ahb_divide) == 8
#           define SET_AHB_DIV ROOT_CLK_DIV_8
#       elif DT_PROP(DT_NODELABEL(clock), ahb_divide) == 16
#           define SET_AHB_DIV ROOT_CLK_DIV_16
#       else
#           error "invalid property value of ahb-divide"
#       endif
#       define AHB_CLOCK (AHB_BASE_CLOCK / DT_PROP(DT_NODELABEL(clock), ahb_divide))
#   elif DT_NODE_HAS_PROP(DT_NODELABEL(clock), ahb_clock)
#       define AHB_CLOCK DT_PROP(DT_NODELABEL(clock), ahb_clock)
#         if AHB_CLOCK > ((AHB_BASE_CLOCK / 1) + (AHB_BASE_CLOCK / 2)) / 2
#           define SET_AHB_DIV ROOT_CLK_DIV_1
#       elif AHB_CLOCK > ((AHB_BASE_CLOCK / 2) + (AHB_BASE_CLOCK / 3)) / 2
#           define SET_AHB_DIV ROOT_CLK_DIV_2
#       elif AHB_CLOCK > ((AHB_BASE_CLOCK / 3) + (AHB_BASE_CLOCK / 4)) / 2
#           define SET_AHB_DIV ROOT_CLK_DIV_3
#       elif AHB_CLOCK > ((AHB_BASE_CLOCK / 4) + (AHB_BASE_CLOCK / 6)) / 2
#           define SET_AHB_DIV ROOT_CLK_DIV_4
#       elif AHB_CLOCK > ((AHB_BASE_CLOCK / 6) + (AHB_BASE_CLOCK / 8)) / 2
#           define SET_AHB_DIV ROOT_CLK_DIV_6
#       elif AHB_CLOCK > ((AHB_BASE_CLOCK / 8) + (AHB_BASE_CLOCK / 16)) / 2
#           define SET_AHB_DIV ROOT_CLK_DIV_8
#       else /* AHB_CLOCK > (AHB_BASE_CLOCK / 16) or others */
#           define SET_AHB_DIV ROOT_CLK_DIV_16
#       endif
#   endif
#endif
#ifndef SET_AHB_DIV
#   define SET_AHB_DIV ROOT_CLK_DIV_3 /* default */
#   define AHB_CLOCK (AHB_BASE_CLOCK / 3)
#endif

#define APB_BASE_CLOCK (AHB_CLOCK / 2)
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clock), okay)
#   if DT_NODE_HAS_PROP(DT_NODELABEL(clock), apb_divide)
#         if DT_PROP(DT_NODELABEL(clock), apb_divide) == 1
#           define SET_APB_DIV APB_CLK_DIV_1
#       elif DT_PROP(DT_NODELABEL(clock), apb_divide) == 2
#           define SET_APB_DIV APB_CLK_DIV_2
#       elif DT_PROP(DT_NODELABEL(clock), apb_divide) == 3
#           define SET_APB_DIV APB_CLK_DIV_3
#       elif DT_PROP(DT_NODELABEL(clock), apb_divide) == 4
#           define SET_APB_DIV APB_CLK_DIV_4
#       elif DT_PROP(DT_NODELABEL(clock), apb_divide) == 6
#           define SET_APB_DIV APB_CLK_DIV_6
#       elif DT_PROP(DT_NODELABEL(clock), apb_divide) == 8
#           define SET_APB_DIV APB_CLK_DIV_8
#       elif DT_PROP(DT_NODELABEL(clock), apb_divide) == 16
#           define SET_APB_DIV APB_CLK_DIV_16
#       else
#           error "invalid property value of apb-divide"
#       endif
#       define APB_CLOCK (APB_BASE_CLOCK / DT_PROP(DT_NODELABEL(clock), apb_divide))
#   elif DT_NODE_HAS_PROP(DT_NODELABEL(clock), apb_clock)
#       define APB_CLOCK DT_PROP(DT_NODELABEL(clock), apb_clock)
#         if AHB_CLOCK > ((APB_BASE_CLOCK / 1) + (APB_BASE_CLOCK / 2)) / 2
#           define SET_APB_DIV APB_CLK_DIV_1
#       elif AHB_CLOCK > ((APB_BASE_CLOCK / 2) + (APB_BASE_CLOCK / 3)) / 2
#           define SET_APB_DIV APB_CLK_DIV_2
#       elif AHB_CLOCK > ((APB_BASE_CLOCK / 3) + (APB_BASE_CLOCK / 4)) / 2
#           define SET_AHB_DIV ROOT_CLK_DIV_3
#       elif AHB_CLOCK > ((APB_BASE_CLOCK / 4) + (APB_BASE_CLOCK / 6)) / 2
#           define SET_AHB_DIV ROOT_CLK_DIV_4
#       elif AHB_CLOCK > ((APB_BASE_CLOCK / 6) + (APB_BASE_CLOCK / 8)) / 2
#           define SET_AHB_DIV ROOT_CLK_DIV_6
#       elif AHB_CLOCK > ((APB_BASE_CLOCK / 8) + (APB_BASE_CLOCK / 16)) / 2
#           define SET_AHB_DIV ROOT_CLK_DIV_8
#       else /* AHB_CLOCK > (APB_BASE_CLOCK / 16) or others */
#           define SET_AHB_DIV ROOT_CLK_DIV_16
#       endif
#   endif
#endif
#ifndef SET_APB_DIV
#   define SET_APB_DIV APB_CLK_DIV_2 /* default */
#   define APB_CLOCK (APB_BASE_CLOCK / 2)
#endif

	/* AHB ~200Mhz / 3 = 66MHz AHB , ~66Mhz / 2 / 2 = 18Mhz APB */
	HAL_SMU_SetRootClock(ROOT_CLK_250M, SET_AHB_DIV, SET_APB_DIV);

	/* Workaround WFI wakeup issue. AE350_I2C->INTEN = 1;       */
	sys_write32(1, 0xF0A00014U);

#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
	extern unsigned int z_clock_hw_cycles_per_sec;
	z_clock_hw_cycles_per_sec = HAL_SMU_GetClock(SMU_CLK_APB);
#endif

	disable_unused_device();

	return 0;
}

SYS_INIT(soc_pre_init, PRE_KERNEL_1, 0);
