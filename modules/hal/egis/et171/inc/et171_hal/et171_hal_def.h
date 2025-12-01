/*! \file et171_hal_def.h 
 * Copyright (c) 2025 Egistec Technology Inc.
 * All rights reserved.
 *
 */

#ifndef __ET171_HAL_DEF_H__
#define __ET171_HAL_DEF_H__

/*! \enum HAL_STATUS
 *  \brief HAL function return status.
 */
typedef enum
{
    HAL_OK                  = 0,    /*!< \brief 0 */
    HAL_INVALID_CMD         = 1,    /*!< \brief 1 */
    HAL_INVALID_PARAM       = 2,    /*!< \brief 2 */
    HAL_ERROR               = 3,    /*!< \brief 3 */
    HAL_BUSY                = 4,    /*!< \brief 4 */
    HAL_TIMEOUT             = 5,    /*!< \brief 5 */
    HAL_SECURE_CHECK_FAIL   = 6,    /*!< \brief 6 */
    HAL_DFU_FW_VER_ERROR    = 7,    /*!< \brief 7 */
    HAL_DFU_FW_ADDR_ERROR   = 8,    /*!< \brief 8 */
    HAL_DFU_FW_SIZE_ERROR   = 9,    /*!< \brief 9 */
    HAL_OTP_LOCKED          = 10,   /*!< \brief 10 */
    HAL_DFU_WRITE_FLASH_FAIL = 11   /*!< \brief 11 */
} HAL_STATUS;

#define HAL_MAX_TIMEOUT         0xFFFFFFFF


#define MEMSET(s, c, n)         __builtin_memset ((s), (c), (n))
#define MEMCPY(des, src, n)     __builtin_memcpy((des), (src), (n))
#define MEMCMP(mem0, mem1, n)     __builtin_memcmp((mem0), (mem1), (n))

#endif /* __ET171_HAL_DEF_H__ */
