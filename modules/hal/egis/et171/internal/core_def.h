/*
 * Copyright (c) 2025 Egistec Technology Inc.
 * All rights reserved.
 *
 */

#ifndef __CORE_DEF_H__
#define __CORE_DEF_H__

#include <stdint.h>


#ifndef __CORE_V5_H__

#define MSTATUS_MIE             0x00000008

#endif // #ifndef __CORE_V5_H__

 
#ifndef _NDS_INTRINSIC_H

#define NDS_MSTATUS             0x300
#define NDS_MIE                 0x304
#define NDS_MCYCLE              0xB00
#define NDS_MCYCLEH             0xB80

static inline unsigned long read_csr(unsigned long srname) {
    register unsigned long ret;
    switch(srname) {
        case NDS_MSTATUS:
            asm volatile("csrr %0, %1" : "=r"(ret) : "I"(NDS_MSTATUS));
            break;
        case NDS_MIE:
            asm volatile("csrr %0, %1" : "=r"(ret) : "I"(NDS_MIE));
            break;
        case NDS_MCYCLE:
            asm volatile("csrr %0, mcycle" : "=r"(ret));
            break;
        case NDS_MCYCLEH:
            asm volatile("csrr %0, mcycleh" : "=r"(ret));
            break;
        default:
            // ASSERT(0);
            break;
    }
    return ret;
}
static inline unsigned long set_csr(unsigned long val, unsigned long srname) {
    register unsigned long ret;
    switch(srname) {
        case NDS_MSTATUS:
            asm volatile("csrrs %0, %1, %2" : "=r"(ret) : "I"(NDS_MSTATUS), "r"(val));
            break;
        case NDS_MIE:
            asm volatile("csrrs %0, %1, %2" : "=r"(ret) : "I"(NDS_MIE), "r"(val));
            break;
        default:
            // ASSERT(0);
            break;
    }
    return ret;
}

static inline unsigned long clear_csr(unsigned long val, unsigned long srname) {
    register unsigned long ret;
    switch(srname) {
        case NDS_MSTATUS:
            asm volatile("csrrc %0, %1, %2" : "=r"(ret) : "I"(NDS_MSTATUS), "r"(val));
            break;
        case NDS_MIE:
            asm volatile("csrrc %0, %1, %2" : "=r"(ret) : "I"(NDS_MIE), "r"(val));
            break;
        default:
            // ASSERT(0);
            break;
    }
    return ret;
}

#endif // #ifndef _NDS_INTRINSIC_H


#endif /* __CORE_DEF_H__ */
