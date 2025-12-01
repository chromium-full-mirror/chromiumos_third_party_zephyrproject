/**
 ******************************************************************************
 * @file           : et171_type.h
 * @brief          : general type define
 ******************************************************************************
 * @attention
 * Copyright (c) 2025 Egis Technology Inc. All Rights Reserved.
 ******************************************************************************
 */

#ifndef __ET171_TYPE_H__
#define __ET171_TYPE_H__

#include <stdint.h>
#include <stddef.h>

/*****************************************************************************
 * System clock
 ****************************************************************************/
 #define KHz                     1000
 #define MHz                     1000000

/*****************************************************************************
 * Device Specific Peripheral Registers structures
 ****************************************************************************/

#define __I                     volatile const	/* 'read only' permissions      */
#define __O                     volatile        /* 'write only' permissions     */
#define __IO                    volatile        /* 'read / write' permissions   */

typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;

typedef uint8_t BOOL;
#define TRUE    (1==1)
#define FALSE   (!TRUE)

#define LOBYTE(w)           ((uint8_t)((w) & 0xFF))
#define HIBYTE(w)           ((uint8_t)(((w) >> 8) & 0xFF))

/* bit operation */
#ifndef BIT
#define BIT(n)  ((unsigned int) 1 << (n))
#endif

#define BITS2(m,n)                  (BIT(m) | BIT(n) )

/* bits range: for example BITS(16,23) = 0xFF0000
 *   ==>  (BIT(m)-1)   = 0x0000FFFF     ~(BIT(m)-1)   => 0xFFFF0000
 *   ==>  (BIT(n+1)-1) = 0x00FFFFFF
 */
#define BITS(m,n)                   (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))


#define BITS_MASK(msb, lsb)         BITS(lsb, msb)

#endif	//__ET171_TYPE_H__
