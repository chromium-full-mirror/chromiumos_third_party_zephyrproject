/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#ifdef CONFIG_DCACHE
#include <zephyr/cache.h>
#endif

#include <sxsymcrypt/hw.h>

#ifdef CONFIG_XIP
#define sx_attr __ramfunc
#else
#define sx_attr
#endif

#ifndef SX_CM_REGS_ADDR
#define SX_CM_REGS_ADDR ((char*)0xE8000000) //((char*)0x000C0000)
#endif


#ifndef SX_ADDR2BUS
#define SX_ADDR2BUS 0x00000000
#endif

#ifndef SX_TRNG_REGS_OFFSET
#define SX_TRNG_REGS_OFFSET 0x1000
#endif

#define ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))

#ifndef SX_DMAMEM_RESERVE_MEM_SZ
//#define SX_DMAMEM_RESERVE_MEM_SZ (16*1024)//2048
#endif

#if SX_DMAMEM_RESERVE_MEM_SZ != 0
static char global_dmamem[SX_DMAMEM_RESERVE_MEM_SZ];
#endif

struct sx_regs {
    char *devmem;
    int slotidx;
};

#define cmb() __asm__ volatile ("":::"memory")
#define wmb() __asm__ volatile ("":::"memory")
#define rmb() __asm__ volatile ("":::"memory")

static const struct sx_regs hwregs[] = {
    {
        .devmem = (char*)(SX_CM_REGS_ADDR),
        .slotidx = 0,
    },
    /* CUSTOMIZATION: For more than one instance of CryptoMaster,
     * add entries with the base address of the registers and
     * incremented slot index. */
};


static const struct sx_regs trngregs[] = {
    {
        .devmem = (char*)(SX_CM_REGS_ADDR) + (SX_TRNG_REGS_OFFSET),
    },
};

static inline void sx_wrregx(struct sx_regs *regs, uint32_t addr, uint32_t val)
{
    volatile uint32_t *p = (uint32_t*)(regs->devmem + addr);

    wmb();
    *p = val;
    rmb();
}

static inline void sx_wrregx_addr(struct sx_regs *regs, uint32_t addr, size_t p)
{
    volatile size_t *d = (volatile size_t*)(regs->devmem + addr);

    wmb();
    *d = p;
    rmb();
}

static inline uint32_t sx_rdregx(struct sx_regs *regs, uint32_t addr)
{
    volatile uint32_t *p = (uint32_t*)(regs->devmem + addr);
    uint32_t v;

    wmb();
    v = *p;
    rmb();

    return v;
}

sx_attr
struct sx_regs *sx_hw_find_regs(unsigned int idx)
{
    if (idx >= ARRAY_COUNT(hwregs))
        return NULL;

    return (struct sx_regs *) &hwregs[idx];
}

sx_attr
int sx_hw_idx_of_regs(struct sx_regs *regs)
{
    return regs - hwregs;
}

sx_attr
struct sx_regs *sx_hw_find_trng_regs(unsigned int idx)
{
    if (idx >= ARRAY_COUNT(trngregs))
        return NULL;

    return (struct sx_regs *) &trngregs[idx];
}

sx_attr
void sx_wrreg(struct sx_regs *regs, uint32_t addr, uint32_t val)
{
    sx_wrregx(regs, addr, val);
}

sx_attr
void sx_wrreg_addr(struct sx_regs *regs, uint32_t addr, struct sxdesc *p)
{
    sx_wrregx_addr(regs, addr, (size_t)p);
}

sx_attr
char *sx_map_internal(struct sx_regs *regs, char *dma)
{
    (void)regs;
    return (char*)(dma) + (SX_ADDR2BUS);
}

sx_attr
char *sx_map_usrdata(char *s)
{
    return s + (SX_ADDR2BUS);
}

sx_attr
char *sx_map_dmadata(char *s)
{
    return s - (SX_ADDR2BUS);
}

sx_attr
void sx_flush_tohw(struct sx_regs *regs, char *cpumem, size_t sz)
{
    (void)regs;
    (void)cpumem;
    (void)sz;
#ifdef CONFIG_DCACHE
    cache_data_flush_range((void*)cpumem, sz);
#endif
}

sx_attr
void sx_flush_fromhw(struct sx_regs *regs, char *cpumem, size_t offset,
    size_t sz)
{
    (void)regs;
    (void)cpumem;
    (void)offset;
    (void)sz;
#ifdef CONFIG_DCACHE
    cache_data_flush_and_invd_range((void*)cpumem, sz);
#endif
}

sx_attr
uint32_t sx_rdreg(struct sx_regs *regs, uint32_t addr)
{
    return sx_rdregx(regs, addr);
}

#if SX_DMAMEM_RESERVE_MEM_SZ != 0
sx_attr
char *sx_alloc_global_dmamem(size_t sz)
{
    if (sz > sizeof(global_dmamem))
        return NULL;

    return global_dmamem;
}
#endif

sx_attr
void sx_cmdma_wait(struct sx_regs *regs)
{
    (void)regs;
    /* CUSTOMIZATION: write custom wait on interrupts here. This is also
     * where custom code can be included to move the CPU to a lower power
     * mode. */

    //__asm__ __volatile__("wfi");
}
