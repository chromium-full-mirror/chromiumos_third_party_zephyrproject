/*
 * Copyright (c) 2025 Egistec Technology Inc.
 * All rights reserved.
 *
 */

#include <core_def.h>
#include <et171.h>
#include <et171_hal_utils.h>

uint64_t HAL_read_mcycle64(void)
{
#if __riscv_xlen == 32
    do {
            unsigned long hi = read_csr(NDS_MCYCLEH);
            unsigned long lo = read_csr(NDS_MCYCLE);

            if (hi == read_csr(NDS_MCYCLEH))
                return ((unsigned long long)hi << 32) | lo;
    } while(1);
#else
    return (unsigned long long)read_csr(NDS_MCYCLE);
#endif
}

uint32_t HAL_read_mcycle32()
{
    return read_csr(NDS_MCYCLE);
}

#ifndef ROM_HAL
// delay milliseconds
void HAL_delay(uint32_t ms)
{
    uint32_t timeout = ms_to_tick(ms);
    uint32_t start = HAL_read_mcycle32();
    while (HAL_read_mcycle32() - start < timeout)
    {
    }
}

// delay microseconds
void HAL_delay_microsecond(uint32_t microsec)
{
    uint32_t timeout = us_to_tick(microsec);
    uint32_t start = HAL_read_mcycle32();
    while (HAL_read_mcycle32() - start < timeout)
    {
    }
}
#endif // ROM_HAL

// get milliseconds
uint32_t HAL_milliseconds()
{
    const union {
        struct {
            uint32_t l32;
            uint32_t h32;
        };
        uint64_t v64;
    } mtime = {.v64 = HAL_read_mtime64()};
    const uint32_t freq = HAL_SMU_GetClock(SMU_CLK_APB);

    if (mtime.h32 == 0 && freq >= ((1 << 8) * 1000)) {
        // The CPU has a 32bits FPU, so divded by float is more fast.
        // Because of float has 24bit accuracy, thus if the denominator is > 8bits, then we don't need to care about error.
        return (uint32_t)(mtime.l32 / (freq / 1000.0f));
    }
    else if (mtime.h32 < ((uint32_t)~0 / 1000)) {
        // < 5.845 years when PLTM @ 100Mhz
        return (uint32_t)(mtime.v64 * 1000 / freq);
    }
    else {
        // we don't care about the quotient, because of it's already over 32bits.
        uint64_t tmp = ((uint64_t)mtime.h32 * 1000 % freq) << 32;
        return (uint32_t)((tmp + (uint64_t)mtime.l32 * 1000) / freq);
    }
}

// get microseconds
uint32_t HAL_microseconds()
{
    const union {
        struct {
            uint32_t l32;
            uint32_t h32;
        };
        uint64_t v64;
    } mtime = {.v64 = HAL_read_mtime64()};
    const uint32_t freq = HAL_SMU_GetClock(SMU_CLK_APB);
    if (mtime.h32 == 0 && freq >= ((1 << 8) * 1000000)) {
        // The CPU has a 32bits FPU, so divded by float is more fast.
        // Because of float has 24bit accuracy, thus if the denominator is > 8bits, then we don't need to care about error.
        return (uint32_t)(mtime.l32 / (freq / 1000000.0f));
    }
    else if (mtime.h32 < ((uint32_t)~0 / 1000000)) {
        // < 21.350 days when PLTM @ 100Mhz
        return (uint32_t)(mtime.v64 * 1000000 / freq);
    }
    else { 
        // we don't care about the quotient, because of it's already over 32bits.
        uint64_t tmp = ((uint64_t)mtime.h32 * 1000000 % freq) << 32;
        return (uint32_t)((tmp + (uint64_t)mtime.l32 * 1000000) / freq);
    }
}

void HAL_write_mtime(unsigned long long mtime)
{
#if __riscv_xlen == 32
    const union {
        struct {
            unsigned long l32;
            unsigned long h32;
        };
        unsigned long long v64;
    } v = { .v64 = mtime };
    const long mstatus = clear_csr(NDS_MSTATUS, MSTATUS_MIE);
    ((volatile unsigned long*)&AE350_PLMT->MTIME)[0] = 0; // to avoid carry
    ((volatile unsigned long*)&AE350_PLMT->MTIME)[1] = v.h32; // update high word first
    ((volatile unsigned long*)&AE350_PLMT->MTIME)[0] = v.l32;
    if (mstatus & MSTATUS_MIE) set_csr(NDS_MSTATUS, MSTATUS_MIE);
#else
    AE350_PLMT->MTIME = mtime;
#endif
}

uint32_t HAL_read_mtime32()
{
#if __riscv_xlen == 32
    return (uint32_t)*(volatile unsigned long*)&AE350_PLMT->MTIME;
#else
    return (uint32_t)AE350_PLMT->MTIME;
#endif
}

uint64_t HAL_read_mtime64()
{
#if __riscv_xlen == 32
    union {
        struct {
            unsigned long l32;
            unsigned long h32;
        };
        unsigned long long v64;
    } v;
    do {
        v.h32 = ((volatile unsigned long*)&AE350_PLMT->MTIME)[1];
        v.l32 = ((volatile unsigned long*)&AE350_PLMT->MTIME)[0];
        if (v.h32 == ((volatile unsigned long*)&AE350_PLMT->MTIME)[1])
            return v.v64;
    } while(1);
#else
    return AE350_PLMT->MTIME;
#endif
}
