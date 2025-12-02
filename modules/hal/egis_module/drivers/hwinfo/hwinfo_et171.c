/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>

// et171 core
#include <et171_hal/et171.h>
#include <et171_hal/et171_hal_smu.h>
#include <et171_hal/et171_hal_otp.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint8_t otp_read_data[16] = {0};

	if (length < 9) {
		return -ENOSYS;
	}

	HAL_OTP_GetUID(otp_read_data);
	otp_read_data[0] = 0;
	if (0 == *(uint64_t*)&otp_read_data[0] && 0 == *(uint16_t*)&otp_read_data[8]) {
		return -ENOSYS;
	}

	int i = 1; // 1~3 hex, 4~9 asscii

	for (; i <= 9; i++) {
		*buffer++ = otp_read_data[i];
	}

	return 9;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;

	// ET171C reset event
	if (ET171_AOSMU->AO_REG & SMU_AO_RESET_EVENT_RSTN_PIN) {
		flags |= RESET_PIN;
	}
	if (ET171_AOSMU->AO_REG & SMU_AO_RESET_EVENT_POR) {
		flags |= RESET_POR;
	}
	if (ET171_AOSMU->AO_REG & SMU_AO_RESET_EVENT_WDT) {
		flags |= RESET_WATCHDOG;
	}
	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	ET171_AOSMU->AO_REG &= ~SMU_AO_RESET_EVENT_MASK;

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = ( 0
		| RESET_PIN
		| RESET_WATCHDOG
		| RESET_POR
	);

	return 0;
}
