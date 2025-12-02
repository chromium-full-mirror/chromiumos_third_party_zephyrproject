/*
 * Copyright (c) 2025, Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT egis_et171_clock

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/clock_control/et171_clock_control.h>

/* ================================================================== ET171 HAL
 * et171 core
 */
#include <et171_hal/et171.h>
#include <et171_hal/et171_hal_smu.h>
#include <et171_hal/et171_hal_otp.h>

#define AHB_CLOCK_MASK (0          \
        | BIT(ET171_CLOCK_SYSRAM)  \
        | BIT(ET171_CLOCK_SYSRAM2) \
        | BIT(ET171_CLOCK_SYSRAM3) \
        | BIT(ET171_CLOCK_SPIM1)   \
        | BIT(ET171_CLOCK_SPIM2)   \
        | BIT(ET171_CLOCK_SPIM3)   \
        | BIT(ET171_CLOCK_SPIS)    \
        | BIT(ET171_CLOCK_CRYPTO)  \
        | BIT(ET171_CLOCK_DMAC)    \
        | BIT(ET171_CLOCK_USB2)    \
        | BIT(ET171_CLOCK_HWA)     \
        | BIT(ET171_CLOCK_HWA2)    \
        | BIT(ET171_CLOCK_AHB)     \
        | BIT(ET171_CLOCK_CPU)     \
    )
#define APB_CLOCK_MASK (0          \
        | BIT(ET171_CLOCK_GPIO)    \
        | BIT(ET171_CLOCK_PITPWM)  \
        | BIT(ET171_CLOCK_I2C)     \
        | BIT(ET171_CLOCK_UART)    \
        | BIT(ET171_CLOCK_OTPC)    \
        | BIT(ET171_CLOCK_WDT)     \
        | BIT(ET171_CLOCK_RTC)     \
        | BIT(ET171_CLOCK_APB)     \
    )

#define CTRL_CLOCK_MASK (0         \
        | BIT(ET171_CLOCK_SYSRAM)  \
        | BIT(ET171_CLOCK_SYSRAM2) \
        | BIT(ET171_CLOCK_SYSRAM3) \
        | BIT(ET171_CLOCK_SPIM1)   \
        | BIT(ET171_CLOCK_SPIM2)   \
        | BIT(ET171_CLOCK_SPIM3)   \
        | BIT(ET171_CLOCK_SPIS)    \
        | BIT(ET171_CLOCK_CRYPTO)  \
        | BIT(ET171_CLOCK_DMAC)    \
        | BIT(ET171_CLOCK_USB2)    \
        | BIT(ET171_CLOCK_HWA)     \
        | BIT(ET171_CLOCK_HWA2)    \
        | BIT(ET171_CLOCK_GPIO)    \
        | BIT(ET171_CLOCK_PITPWM)  \
        | BIT(ET171_CLOCK_I2C)     \
        | BIT(ET171_CLOCK_UART)    \
        | BIT(ET171_CLOCK_OTPC)    \
        | BIT(ET171_CLOCK_WDT)     \
        | BIT(ET171_CLOCK_RTC)     \
    )


/* ================================================================== driver instance */
struct et171_clock_data {
	struct k_mutex mutex;
};

static int et171_clock_control_on(const struct device *dev,
						clock_control_subsys_t sub_system)
{
	struct et171_clock_data *data = dev->data;
	const uint32_t clkid = 1 << (uint32_t)sub_system;

	if (clkid & CTRL_CLOCK_MASK) {
		k_mutex_lock(&data->mutex, K_FOREVER);
		HAL_SMU_PowerUpIP(clkid);
		k_mutex_unlock(&data->mutex);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int et171_clock_control_off(const struct device *dev,
						clock_control_subsys_t sub_system)
{
	struct et171_clock_data *data = dev->data;
	const uint32_t clkid = 1 << (uint32_t)sub_system;

	if (clkid & CTRL_CLOCK_MASK) {
		k_mutex_lock(&data->mutex, K_FOREVER);
		HAL_SMU_PowerDownIP(clkid);
		k_mutex_unlock(&data->mutex);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int et171_clock_control_get_rate(const struct device *dev,
						clock_control_subsys_t sub_system,
						uint32_t *rate)
{
	const uint32_t clkid = 1 << (uint32_t)sub_system;

	if (clkid & AHB_CLOCK_MASK) {
		*rate = HAL_SMU_GetClock(SMU_CLK_AHB);
	} else if (clkid & APB_CLOCK_MASK) {
		*rate = HAL_SMU_GetClock(SMU_CLK_APB);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int et171_clock_init(const struct device *dev)
{
	struct et171_clock_data *data = dev->data;

	k_mutex_init(&data->mutex);

	return 0;
}

static DEVICE_API(clock_control, et171_clock_control_api) = {
	.on = et171_clock_control_on,
	.off = et171_clock_control_off,
	.get_rate = et171_clock_control_get_rate,
};

static struct et171_clock_data clock_data;

DEVICE_DT_INST_DEFINE(0,
		    et171_clock_init,
		    NULL,
		    &clock_data, NULL,
		    PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &et171_clock_control_api);
