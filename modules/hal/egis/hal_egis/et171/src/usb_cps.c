/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#ifdef CONFIG_DCACHE
#include <zephyr/cache.h>
#endif

#include <et171_usb/cps.h>

#ifdef CONFIG_XIP
#define cps_attr __ramfunc
#else
#define cps_attr
#endif

cps_attr
uint8_t CPS_UncachedRead8(volatile uint8_t* address) {
    return *address;
}

cps_attr
uint16_t CPS_UncachedRead16(volatile uint16_t* address) {
    return *address;
}

cps_attr
uint32_t CPS_UncachedRead32(volatile uint32_t* address) {
    return *address;
}

cps_attr
void CPS_UncachedWrite8(volatile uint8_t* address, uint8_t value) {
    *address = value;
}

cps_attr
void CPS_UncachedWrite16(volatile uint16_t* address, uint16_t value) {
    *address = value;
}

cps_attr
void CPS_UncachedWrite32(volatile uint32_t* address, uint32_t value) {
    *address = value;
}

cps_attr
void CPS_CacheInvalidate(uintptr_t address, size_t size, uintptr_t devInfo) {
#ifdef CONFIG_DCACHE
    cache_data_invd_range((void*)address, size);
#endif
    return;
}

cps_attr
void CPS_CacheFlush(uintptr_t address, size_t size, uintptr_t devInfo) {
#ifdef CONFIG_DCACHE
    cache_data_flush_range((void*)address, size);
#endif
    return;
}
