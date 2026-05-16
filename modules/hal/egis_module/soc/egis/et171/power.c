/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/pm.h>
#include <zephyr/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#include <et171_hal/et171.h>
#include <et171_hal/et171_hal_smu.h>

#include <andes_csr.h>
#include <zephyr/arch/riscv/csr.h>
#define MMISC_CTL_BRPE_EN BIT(3)

#ifndef CONFIG_64BIT
typedef union union64_t {
	struct {
		uint32_t l32;
		uint32_t h32;
	};
	uint64_t v64;
} union64_t;
#endif

static ALWAYS_INLINE uint64_t read_mtime64()
{
#ifdef CONFIG_64BIT
	return sys_read64(0xE6000000);
#else
	union64_t v;
	do {
		v.h32 = sys_read32(0xE6000004);
		v.l32 = sys_read32(0xE6000000);
		if (v.h32 == sys_read32(0xE6000004))
			return v.v64;
	} while(1);
#endif
}

static ALWAYS_INLINE void write_mtime(unsigned long long mtime)
{
#ifdef CONFIG_64BIT
	sys_write64(mtime, 0xE6000000); 
#else
	const union64_t v = { .v64 = mtime };
	sys_write32(0, 0xE6000000); // to avoid carry
	sys_write32(v.h32, 0xE6000004); // update high word first
	sys_write32(v.l32, 0xE6000000); 
#endif
}

static ALWAYS_INLINE uint64_t read_mtimecmp()
{
	return sys_read64(0xE6000008);
}

static ALWAYS_INLINE void write_mtimecmp(unsigned long long mtime)
{
#ifdef CONFIG_64BIT
	sys_write64(mtime, 0xE6000008); 
#else
	const union64_t v = { .v64 = mtime };
	sys_write32(0xFFFFFFFF, 0xE600000C);
	sys_write32(v.l32, 0xE6000008); // update high word first 
	sys_write32(v.h32, 0xE600000C);
#endif
}

static ALWAYS_INLINE uint32_t get_active_interrupt_devices()
{
	// const uint32_t plic_int_enable_bits = sys_read32(0xe4002000);
	// #pragma push_macro("irq_is_enabled")
	// #define irq_is_enabled(irqn) (0 != (plic_int_enable_bits & BIT(irqn)))
	uint32_t active_ip = 0;
#if DT_NODE_EXISTS(DT_NODELABEL(ext_wdt))
	 active_ip |= SMU_IPSEL_PITPWM;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usb), okay)
	if (irq_is_enabled(DT_IRQN(DT_NODELABEL(usb)))) active_ip |= SMU_IPSEL_USB2;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
	if (irq_is_enabled(DT_IRQN(DT_NODELABEL(uart0)))) active_ip |= SMU_IPSEL_UART;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
	if (irq_is_enabled(DT_IRQN(DT_NODELABEL(gpio0)))) active_ip |= SMU_IPSEL_GPIO;
#endif
	// #pragma pop_macro("irq_is_enabled")
	return active_ip;
}

__ramfunc
#ifndef __clang__
__attribute__((optimize("O2")))
#endif
static void suspend()
{
	extern uint32_t __g_et171_exten_clock;
	const uint32_t lo_freq = __g_et171_exten_clock * 3 / 2;
	const uint32_t hi_freq = HAL_SMU_GetClock(SMU_CLK_APB);
	const uint32_t clk_setting = sys_read32(0xF0100004);
	const uint32_t clk_en = sys_read32(0xF0100014);
	const uint32_t clk_off = ~0x00181DFF | get_active_interrupt_devices();

	/*== turn off periphery clock =====================================================*/
	sys_write32(clk_en & clk_off, 0xF0100014); /* Remain USB, SPIM, SRAM */
	__asm__ volatile ("" ::: "memory");

	const bool switch_clock = true
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay)
		/* Don't switch clock when SPI1 is still busy */
		&& !(sys_read32(0xF0F00038) & BIT(4))
#endif
#if CONFIG_PM_SOC_ET171_PREVENT_UART_RX_INCORRECT
		/* Don't switch clock when UART RX is ready for receiving */
		&& !(sys_read32(0xF0200024) & BIT(0))
#endif
		;
	if (!switch_clock)
	{
		__asm__ volatile("wfi");
	}
	else
	{
#if defined(CONFIG_DCACHE) && defined(CONFIG_XIP)
		/* Prevent the i-cache to pre-fetch instructions from flash after clock switching. */
		const unsigned int mmisc_ctl = csr_read_clear(NDS_MMISC_CTL, MMISC_CTL_BRPE_EN);
		__asm__ volatile ("" ::: "memory");
#endif

		/*== switch to external clock =====================================================*/
		uint64_t begin_time = read_mtime64();
		__asm__ volatile ("" ::: "memory");
		sys_write32(0x00000000, 0xF0100004);
		__asm__ volatile ("" ::: "memory");

		/*== turn off internal clock ======================================================*/
		const uint32_t osc360m = sys_read32(0xF0100210);
		sys_write32(osc360m & ~BIT(30), 0xF0100210);
		__asm__ volatile ("" ::: "memory");

		/*== update mtimecmp ==============================================================*/
		const uint64_t cmp_time = read_mtimecmp();
		const bool cmp_check = cmp_time > begin_time;
		if (cmp_check) {
			const uint64_t new_cmp_time = begin_time + (cmp_time - begin_time) * lo_freq / hi_freq;
			write_mtimecmp(new_cmp_time);
		}

		/*== idle =========================================================================*/
		__asm__ volatile("wfi");
		__asm__ volatile ("" ::: "memory");

		/*== restore mtimecmp =============================================================*/
		if (cmp_check) {
			write_mtimecmp(cmp_time);
		}

		/*== turn on internal clock =======================================================*/
		sys_write32(osc360m, 0xF0100210);
		__asm__ volatile ("" ::: "memory");

		/*== switch to internal clock =====================================================*/
		uint64_t end_time = read_mtime64();
		__asm__ volatile ("" ::: "memory");
		sys_write32(clk_setting, 0xF0100004);
		__asm__ volatile ("" ::: "memory");

#if defined(CONFIG_DCACHE) && defined(CONFIG_XIP)
		if (mmisc_ctl & MMISC_CTL_BRPE_EN) {
			csr_set(NDS_MMISC_CTL, MMISC_CTL_BRPE_EN);
		}
		__asm__ volatile ("" ::: "memory");
#endif

		/*== update mtime =================================================================*/
		uint64_t adj_time = (end_time - begin_time) * hi_freq / lo_freq;
		__asm__ volatile ("" ::: "memory");
		write_mtime(read_mtime64() + adj_time);
		__asm__ volatile ("" ::: "memory");

		/*== Check mtime ==========================================================*/
		if (cmp_check) {
			if (!(csr_read(mip) & MIP_MTIP)) {
				uint64_t check_time = read_mtime64();
				if (check_time > cmp_time) {
					/* MIP_MTIP is read only, try to set MIP_MTIP by write_mtimecmp() */
					write_mtimecmp(check_time + 32);
				}
			}
		}
	} /* switch_clock */

	/*== turn on periphery clock ======================================================*/
	sys_write32(clk_en, 0xF0100014);
}

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
		while ((sys_read32(0xF0200034) & BIT(5)) == 0) ;
#endif
		suspend();
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		irq_unlock(MSTATUS_IEN);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}
