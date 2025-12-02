/*
 * Copyright 2025 Egis Techonlogy Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>

#if DT_NODE_EXISTS(DT_NODELABEL(ext_wdt))
#include <et171_hal/et171.h>
#include <et171_hal/et171_hal_smu.h>
#endif

#include <pinctrl_soc.h>

#if DT_NODE_EXISTS(DT_NODELABEL(ext_wdt))
static void init_external_wdt()
{
	uint8_t pwm_ch = DT_PROP(DT_NODELABEL(ext_wdt), pwm_channel);

	int key = arch_irq_lock();

	const uint32_t tout = 100;
	const uint32_t cycle_per_ms = HAL_SMU_GetClock(SMU_CLK_PITPWM_EXT) / 1000;
	uint32_t push_cycle = (cycle_per_ms >> 2) | 1;
	uint32_t pull_cycle = cycle_per_ms * tout - push_cycle;

	if (pull_cycle > 0xFFFF) {
		pull_cycle = 0xFFFF;
	}
	if (push_cycle > 0xFFFF) {
		push_cycle = 0xFFFF;
	}

	AE350_PIT->CHANNEL[pwm_ch].CTRL = 4;
	AE350_PIT->CHANNEL[pwm_ch].RELOAD = ((push_cycle << 16) | pull_cycle);
	AE350_PIT->CHNEN |= (BIT(3) << (4 * (pwm_ch)));

	PINCTRL_DT_DEFINE(DT_NODELABEL(ext_wdt));
	const struct pinctrl_dev_config *pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(ext_wdt));
	pinctrl_apply_state(pincfg, PINCTRL_STATE_DEFAULT);

	arch_irq_unlock(key);
}
#endif /* DT_NODE_EXISTS(DT_NODELABEL(ext_wdt)) */

static int board_init(void)
{
#if DT_NODE_EXISTS(DT_NODELABEL(ext_wdt))
	init_external_wdt();
#endif
	return 0;
}

SYS_INIT(board_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);