/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_EGIS_ET171_PINCTRL_SOC_H__
#define __SOC_EGIS_ET171_PINCTRL_SOC_H__

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/egis-et171-pinctrl.h>

/**
 * @brief pin type.
 */
typedef uint32_t pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx) \
	DT_PROP(DT_PROP_BY_IDX(node_id, state_prop, idx), pinmux),

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) \
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

#endif  /* __SOC_EGIS_ET171_PINCTRL_SOC_H__ */
