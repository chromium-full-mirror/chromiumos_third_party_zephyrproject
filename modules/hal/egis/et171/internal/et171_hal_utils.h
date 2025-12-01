/*
 * Copyright (c) 2025 Egistec Technology Inc.
 * All rights reserved.
 *
 */

/**
 * \file
 * \brief ET171 HAL utilities
 */
#ifndef __ET171_HAL_UTILS_H__
#define __ET171_HAL_UTILS_H__

#include "et171.h"
#include "et171_hal_smu.h"

#ifndef MIN
#define MIN(x, y) (((x) < (y))? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) (((x) > (y))? (x) : (y))
#endif


#define le2be16(x)  ((((x) & 0xff00U) >> 8) | (((x) & 0x00ffU) << 8))
#define le2be32(x)  ((((x) & 0x000000ffU) << 24) | \
                     (((x) & 0x0000ff00U) << 8)  | \
                     (((x) & 0x00ff0000U) >> 8)  | \
                     (((x) & 0xff000000U) >> 24))

#define be2le16(x)  le2be16(x)
#define be2le32(x)  le2be32(x)

#define ms_to_tick(x)      ((x)*((HAL_SMU_GetClock(SMU_CLK_CPU)+500)/1000))
#define us_to_tick(x)      ((x)*((HAL_SMU_GetClock(SMU_CLK_CPU)+500000)/1000000))

#define tick_to_ms(x)      ((x)*1000/HAL_SMU_GetClock(SMU_CLK_CPU))
#define tick_to_us(x)      ((x)*1000000/HAL_SMU_GetClock(SMU_CLK_CPU))

/**
 *  \brief read machine cycles (64bit value)
 *
 *  \returns machine cycles
 */
uint64_t HAL_read_mcycle64(void);

/**
 *  \brief read machine cycles (lower 32bit value)
 *
 *  \returns machine cycles
 */
uint32_t HAL_read_mcycle32();

/**
 *  \brief delay milliseconds
 *
 *  \param[in]      ms          delay time in ms
 *
 *  \returns none
 */
void HAL_delay(uint32_t ms);

/**
 *  \brief delay microseconds
 *
 *  \param[in]      microsec    delay time in us
 *
 *  \returns none
 */
void HAL_delay_microsecond(uint32_t microsec);

/**
 *  \brief get milliseconds
 *
 *  \returns current machine time in ms
 */
uint32_t HAL_milliseconds();

/**
 *  \brief get microseconds
 *
 *  \returns current machine time in us
 */
uint32_t HAL_microseconds();


// platform mtime utility
#define ms_to_mtime(x)      ((x)*((HAL_SMU_GetClock(SMU_CLK_APB)+500)/1000))
#define us_to_mtime(x)      ((x)*((HAL_SMU_GetClock(SMU_CLK_APB)+500000)/1000000))
#define mtime_to_ms(x)      ((uint32_t)((float)(x)*1000/HAL_SMU_GetClock(SMU_CLK_APB)+0.5f))
#define mtime_to_us(x)      ((uint32_t)((float)(x)*1000000/HAL_SMU_GetClock(SMU_CLK_APB)+0.5f))

void HAL_write_mtime(unsigned long long mtime);
uint32_t HAL_read_mtime32();
uint64_t HAL_read_mtime64();

/**
 *  \brief       Measure extern clock frequence
 *  \param[out]  result: frequence Hz
 *  \return      HAL_STATUS
 */
HAL_STATUS HAL_measure_ext_clock(uint32_t* result);


#endif /* __ET171_HAL_UTILS_H__ */
