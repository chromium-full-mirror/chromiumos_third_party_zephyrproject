/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <pinctrl_soc.h>
#include <zephyr/drivers/pinctrl.h>

// et171 core
#include <et171_hal/et171.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++)
    {
		const uint32_t pin_cfg = pins[i];
		const uint32_t reg_sel = (pin_cfg >> 24) & 0xFF;
		const uint32_t pos_offset = (pin_cfg >> 16) & 0xFF;
		const uint32_t mux_sel = (pin_cfg >> 8) & 0xFF;
		__maybe_unused const uint32_t mux_pad = pin_cfg & 0xFF;
        __IO unsigned int * const mux_reg =
            (reg_sel == 0) ? &ET171_SMU2->PAD_MUXA : &ET171_SMU2->PAD_MUXB;

        *mux_reg = (*mux_reg & ~(0x03 << pos_offset)) | mux_sel << pos_offset;
	}

	return 0;
}
