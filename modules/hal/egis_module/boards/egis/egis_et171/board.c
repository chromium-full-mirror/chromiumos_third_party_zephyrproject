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

#include <et171_hal/et171.h>
#include <et171_hal/et171_hal_smu.h>

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

#ifndef __clang__
__attribute__((optimize("O2")))
#endif
static inline void disable_unused_pad()
{
	#define CONVERT_GPIO_TO_PAD(node_id, prop, idx) \
		SMU_GPIO_NUM_TO_PAD(DT_GPIO_PIN_BY_IDX(node_id, prop, idx))

	uint32_t used_pad = 0
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spis), okay)
		| PAD_SPIS_MODE0
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi0), okay)
		| PAD_SPIM1_MODE0
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay)
		| PAD_SPIM2
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi2), okay)
		| PAD_SPIM3
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fp_sensor))
		| SMU_GPIO_NUM_TO_PAD(DT_GPIO_PIN_BY_IDX(DT_NODELABEL(fp_sensor), reset_gpios, 0))
		| SMU_GPIO_NUM_TO_PAD(DT_GPIO_PIN_BY_IDX(DT_NODELABEL(fp_sensor), irq_gpios, 0))
#endif
		;

#if CONFIG_PINCTRL

	#define Z_LISTIFY_PINS_DEFINE(state_idx, node_id, F, sep, ...)	\
		Z_PINCTRL_STATE_PINS_DEFINE(state_idx, node_id);			\
		COND_CODE_1(Z_PINCTRL_SKIP_STATE(state_idx, node_id), (), (	\
			LISTIFY(												\
				DT_PROP_LEN(node_id, pinctrl_##state_idx),			\
				F,													\
				sep,												\
				Z_PINCTRL_STATE_PINS_NAME(state_idx, node_id),		\
				__VA_ARGS__											\
			)														\
		))

	#define APPEND_USED_PAD(i, soc_pin, result) \
		result |= (1U << ET171_PINMUX_TO_PADNUM(soc_pin[i]))

#endif // #if CONFIG_PINCTRL

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
#if CONFIG_PINCTRL && DT_NODE_HAS_PROP(DT_NODELABEL(uart0), pinctrl_names)
	Z_LISTIFY_PINS_DEFINE(0, DT_NODELABEL(uart0), APPEND_USED_PAD, (;), used_pad);
#else
	used_pad |= (PAD13_UART_RX | PAD14_UART_TX);
#endif
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay) */

#if DT_NODE_EXISTS(DT_NODELABEL(ext_wdt))
	Z_LISTIFY_PINS_DEFINE(0, DT_NODELABEL(ext_wdt), APPEND_USED_PAD, (;), used_pad);
#endif

    ET171_AOSMU->PAD_IE = used_pad
#ifdef CONFIG_XIP
		| PAD_SPIM1_MODE0
#endif
#ifdef CONFIG_EGIS_JTAG
		| PAD_JTAG
#endif
		;
}

static int board_init(void)
{
	disable_unused_pad();

#if DT_NODE_EXISTS(DT_NODELABEL(ext_wdt))
	init_external_wdt();
#endif

	return 0;
}

SYS_INIT(board_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
