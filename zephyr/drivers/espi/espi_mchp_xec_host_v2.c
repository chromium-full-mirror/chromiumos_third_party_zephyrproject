/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_espi_host_dev

#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/espi/mchp_xec_espi.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include "espi_utils.h"
#include "espi_mchp_xec_v2.h"

#define CONNECT_IRQ_MBOX0	NULL
#define CONNECT_IRQ_KBC0	NULL
#define CONNECT_IRQ_ACPI_EC0	NULL
#define CONNECT_IRQ_ACPI_EC1	NULL
#define CONNECT_IRQ_ACPI_EC2	NULL
#define CONNECT_IRQ_ACPI_EC3	NULL
#define CONNECT_IRQ_ACPI_EC4	NULL
#define CONNECT_IRQ_ACPI_PM1	NULL
#define CONNECT_IRQ_EMI0	NULL
#define CONNECT_IRQ_EMI1	NULL
#define CONNECT_IRQ_EMI2	NULL
#define CONNECT_IRQ_RTC0	NULL
#define CONNECT_IRQ_P80BD0	NULL

#define INIT_MBOX0		NULL
#define INIT_KBC0		NULL
#define INIT_ACPI_EC0		NULL
#define INIT_ACPI_EC1		NULL
#define INIT_ACPI_EC2		NULL
#define INIT_ACPI_EC3		NULL
#define INIT_ACPI_EC4		NULL
#define INIT_ACPI_PM1		NULL
#define INIT_EMI0		NULL
#define INIT_EMI1		NULL
#define INIT_EMI2		NULL
#define INIT_RTC0		NULL
#define INIT_P80BD0		NULL
#define	INIT_UART0		NULL
#define	INIT_UART1		NULL

/* BARs as defined in LPC spec chapter 11 */
#define ESPI_XEC_KBC_BAR_ADDRESS    0x00600000
#define ESPI_XEC_UART0_BAR_ADDRESS  0x03F80000
#define ESPI_XEC_MBOX_BAR_ADDRESS   0x03600000
#define ESPI_XEC_PORT80_BAR_ADDRESS 0x00800000
#define ESPI_XEC_PORT81_BAR_ADDRESS 0x00810000
#define ESPI_XEC_ACPI_EC0_BAR_ADDRESS   0x00620000

/* Espi peripheral has 3 uart ports */
#define ESPI_PERIPHERAL_UART_PORT0  0
#define ESPI_PERIPHERAL_UART_PORT1  1

#define UART_DEFAULT_IRQ_POS	    2u
#define UART_DEFAULT_IRQ	    BIT(UART_DEFAULT_IRQ_POS)

/* PCR */
#define XEC_PCR_REG_BASE						\
	((struct pcr_regs *)(DT_REG_ADDR(DT_NODELABEL(pcr))))

#define MCHP_P80_MAX_BYTE_COUNT  4
#define MCHP_P80_FIFO_READ_COUNT 8

struct xec_espi_host_sram_config {
	uint32_t host_sram1_base;
	uint32_t host_sram2_base;
	uint16_t ec_sram1_ofs;
	uint16_t ec_sram2_ofs;
	uint8_t sram1_acc_size;
	uint8_t sram2_acc_size;
};

struct xec_espi_host_dev_config {
	const struct device *parent;
	uint32_t reg_base;		/* logical device registers */
	uint32_t host_mem_base;		/* 32-bit host memory address */
	uint16_t host_io_base;		/* 16-bit host I/O address */
	uint8_t ldn;			/* Logical device number */
	uint8_t num_ecia;
	uint32_t *girqs;
};

struct xec_acpi_ec_config {
	uintptr_t regbase;
	uint32_t ibf_ecia_info;
	uint32_t obe_ecia_info;
};

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
static uint8_t ec_host_cmd_sram[CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE +
			CONFIG_ESPI_XEC_PERIPHERAL_ACPI_SHD_MEM_SIZE] __aligned(8);
#else
static uint8_t ec_host_cmd_sram[CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE] __aligned(8);
#endif

#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */

#ifdef CONFIG_ESPI_PERIPHERAL_XEC_MAILBOX

#define XEC_MBOX0_NODE DT_NODELABEL(mbox0)

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(XEC_MBOX0_NODE), "XEC mbox0 DT node is disabled!");

#define XEC_MBOX_H2EC_OFS      0x100U
#define XEC_MBOX_EC2H_OFS      0x104U
#define XEC_MBOX_SMI_SRC_OFS   0x108U
#define XEC_MBOX_SMI_MSK_OFS   0x10cU
/* 32 mailbox data registers. 0 < n < 32 */
#define XEC_MBOX_NUM_8BIT_DATA 32U
#define XEC_MBOX_DATA_OFS(n)   (0x110u + (uint32_t)(n))

#define XEC_MBOX_SIRQ_EV_SLOT_VAL  CONFIG_ESPI_PERIPHERAL_XEC_MAILBOX_SIRQ_SLOT
#define XEC_MBOX_SIRQ_SMI_SLOT_VAL CONFIG_ESPI_PERIPHERAL_XEC_MAILBOX_SMI_SIRQ_SLOT

#define XEC_MBOX_GET_DATA 0
#define XEC_MBOX_SET_DATA 1U

struct xec_mbox_config {
	uintptr_t regbase;
	uint32_t ecia_info;
	uint32_t host_mem_addr;
	uint16_t host_io_addr;
};

static const struct xec_mbox_config xec_mbox0_cfg = {
	.regbase = (uintptr_t)DT_REG_ADDR(XEC_MBOX0_NODE),
	.ecia_info = (uint32_t)DT_PROP_BY_IDX(XEC_MBOX0_NODE, girqs, 0),
	.host_io_addr = (uint32_t)DT_PROP_OR(XEC_MBOX0_NODE, host_io, UINT16_MAX),
	.host_mem_addr = (uint32_t)DT_PROP_OR(XEC_MBOX0_NODE, host_mem, UINT32_MAX),
};

/* dev is a pointer to espi0 (parent) device
 * Interrupt on Host writing a byte to Host-to-EC mailbox register.
 * EC clears interrupt line by writing 0xFF to Host-to-EC mailbox register.
 * This also clears Host-to-EC to 0. Host can poll Host-to-EC to know
 * EC has processed command. EC may generate an interrupt to the Host
 * by writing a non-zero value to the EC-to-Host mailbox register.
 * EC may choose to enable EC-to-Host Serial-IRQ and/or EC-to-Host SMI delivered
 * by Serial-IRQ.
 * We do not write 0xFF to the Host-to-EC register to clear the interrupt status because
 * the Host may interpret claring of Host-to-EC as the EC has completed the requested
 * operation. Instead, we disable the mailbox interrupt. The application can re-enable the
 * interrupt after writing a value to Host-to-EC via our side-band API.
 */
static void mbox0_isr(const struct device *dev)
{
	struct espi_xec_data *const data = dev->data;
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		MCHP_XEC_ESPI_PERIPHERAL_MAILBOX,
		ESPI_PERIPHERAL_NODATA,
	};

	evt.evt_data = sys_read8(mregbase + XEC_MBOX_H2EC_OFS);

	xec_ecia_info_girq_ctrl(xec_mbox0_cfg.ecia_info, MCHP_MEC_ECIA_GIRQ_DIS);
	xec_ecia_info_girq_src_clear(xec_mbox0_cfg.ecia_info);

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static int connect_irq_mbox0(const struct device *dev)
{
	xec_ecia_info_girq_src_clear(xec_mbox0_cfg.ecia_info);

	IRQ_CONNECT(DT_IRQN(XEC_MBOX0_NODE), DT_IRQ(XEC_MBOX0_NODE, priority), mbox0_isr,
		    DEVICE_DT_GET(XEC_ESPI0_NODE), 0);
	irq_enable(DT_IRQN(XEC_MBOX0_NODE));

	/* enable GIRQ source */
	xec_ecia_info_girq_ctrl(xec_mbox0_cfg.ecia_info, MCHP_MEC_ECIA_GIRQ_EN);

	return 0;
}

/* Called by eSPI Bus init, eSPI reset de-assertion, and eSPI Platform Reset
 * de-assertion.
 */
static int init_mbox0(const struct device *dev)
{
	const struct espi_xec_config *devcfg = dev->config;
	struct xec_espi_ioc_cfg_regs *regs =
		(struct xec_espi_ioc_cfg_regs *)(devcfg->ioc_base_addr + MCHP_ESPI_IO_CFG_OFS);
	uint32_t bar_val = 0;

#ifdef CONFIG_ESPI_PERIPHERAL_XEC_MAILBOX_PORT_NUM
	bar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(CONFIG_ESPI_PERIPHERAL_XEC_MAILBOX_PORT_NUM);

	regs->IOHBAR[IOB_MBOX] = bar_val | MCHP_ESPI_IO_BAR_HOST_VALID;
#else
	bar_val = xec_mbox0_cfg.host_io_addr;

	if (bar_val != UINT16_MAX) {
		bar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(bar_val);
		regs->IOHBAR[IOB_MBOX] = bar_val | MCHP_ESPI_IO_BAR_HOST_VALID;
	}

	bar_val = xec_mbox0_cfg.host_mem_addr;
	if (bar_val != UINT32_MAX) {
		xec_espi_mbar_host_set(devcfg->mc_base_addr, MEMB_MBOX, bar_val, true);
	}
#endif

	/* Serial IRQ */
	regs->SIRQ[MCHP_ESPI_SIRQ_MBOX_SIRQ] = XEC_MBOX_SIRQ_EV_SLOT_VAL;
	regs->SIRQ[MCHP_ESPI_SIRQ_MBOX_SMI] = XEC_MBOX_SIRQ_SMI_SLOT_VAL;

	return 0;
}

/* Write all 32 8-bit mailbox data register as 32-bit words. These registers were designed
 * to be accessible as 32-bit words.
 */
static int mbox0_data_set_all(const struct device *dev, const uint32_t *src)
{
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	uint32_t n = 0;

	if (src == NULL) {
		return -EINVAL;
	}

	for (n = 0; n < (XEC_MBOX_NUM_8BIT_DATA / 4U); n += 4U) {
		sys_write32(src[n], mregbase + XEC_MBOX_DATA_OFS(n));
	}

	return 0;
}

/* Get all 32 8-bit mailbox data register as 32-bit words. These registers were designed
 * to be accessible as 32-bit words.
 */
static int mbox0_data_get_all(const struct device *dev, uint32_t *dest)
{
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	uint32_t n = 0;

	if (dest == NULL) {
		return -EINVAL;
	}

	for (n = 0; n < (XEC_MBOX_NUM_8BIT_DATA / 4U); n += 4U) {
		dest[n] = sys_read32(mregbase + XEC_MBOX_DATA_OFS(n));
	}

	return 0;
}

int mchp_xec_espi_pc_mailbox_get(const struct device *dev, uint8_t mbox_idx, uint8_t num_mboxes,
				 uint8_t *dest)
{
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	uint8_t nb = 0;

	if ((dev == NULL) || (dest == NULL) || (mbox_idx >= MCHP_XEC_MAX_MAILBOX_INDEX)) {
		return -EINVAL;
	}

	mregbase += XEC_MBOX_DATA_OFS(0);

	for (uint8_t idx = mbox_idx; idx < MCHP_XEC_MAX_MAILBOX_INDEX; idx++) {
		if (idx >= num_mboxes) {
			break;
		}

		dest[nb++] = sys_read8(mregbase + idx);
	}

	return 0;
}

int mchp_xec_espi_pc_mailbox_set(const struct device *dev, uint8_t mbox_idx, uint8_t num_mboxes,
				 const uint8_t *src)
{
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	uint8_t nb = 0;

	if ((dev == NULL) || (src == NULL) || (mbox_idx >= MCHP_XEC_MAX_MAILBOX_INDEX)) {
		return -EINVAL;
	}

	mregbase += XEC_MBOX_DATA_OFS(0);

	for (uint8_t idx = mbox_idx; idx < MCHP_XEC_MAX_MAILBOX_INDEX; idx++) {
		if (idx >= num_mboxes) {
			break;
		}

		sys_write8(src[nb], mregbase + idx);
	}

	return 0;
}

int mchp_xec_espi_pc_mailbox_get_all(const struct device *dev, uint32_t *dest)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	return mbox0_data_get_all(dev, dest);
}

int mchp_xec_espi_pc_mailbox_set_all(const struct device *dev, const uint32_t *src)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	return mbox0_data_set_all(dev, src);
}

int mchp_xec_espi_pc_mailbox_cmd(const struct device *dev, enum mchp_xec_espi_pc_mbox_cmd_id cmd_id,
				 uint8_t cmd)
{
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	uint32_t ofs = 0;

	if ((dev == NULL) || (cmd_id >= MCHP_XEC_ESPI_PC_MBOX_CMD_ID_MAX)) {
		return -EINVAL;
	}

	switch (cmd_id) {
	case MCHP_XEC_ESPI_PC_MBOX_CMD_ID_HOST_TO_EC:
		ofs = XEC_MBOX_H2EC_OFS;
		break;
	case MCHP_XEC_ESPI_PC_MBOX_CMD_ID_EC_TO_HOST:
		ofs = XEC_MBOX_EC2H_OFS;
		break;
	case MCHP_XEC_ESPI_PC_MBOX_CMD_ID_SMI_SRC:
		ofs = XEC_MBOX_SMI_SRC_OFS;
		break;
	case MCHP_XEC_ESPI_PC_MBOX_CMD_ID_SMI_MASK:
		ofs = XEC_MBOX_SMI_MSK_OFS;
		break;
	default:
		return -EINVAL;
	}

	sys_write8(cmd, mregbase + ofs);

	return 0;
}

/** @brief Write command to specified mailbox box command register
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param id enum mchp_xec_espi_pc_mbox_isrc specifying the interrupt (internal EC or serial IRQ)
 * @param enable a zero value is disable else enable
 *
 * @retval 0 success
 * @retval -EINVAL if dev is NULL or id is invalid
 */
int mchp_xec_espi_pc_mailbox_ictrl(const struct device *dev, enum mchp_xec_espi_pc_mbox_isrc id,
				   uint8_t enable)
{
	const struct espi_xec_config *espi_drv_cfg = dev->config;
	struct xec_espi_ioc_cfg_regs *regs =
		(struct xec_espi_ioc_cfg_regs *)(espi_drv_cfg->ioc_base_addr +
						 MCHP_ESPI_IO_CFG_OFS);
	uint8_t mbox_sirq_slot_val = MCHP_ESPI_IO_SIRQ_DIS;

	if (dev == NULL) {
		return -EINVAL;
	}

	switch (id) {
	case MCHP_XEC_ESPI_PC_MBOX_ISRC_EC:
		xec_ecia_info_girq_ctrl(xec_mbox0_cfg.ecia_info, enable);
		break;
	case MCHP_XEC_ESPI_PC_MBOX_ISRC_SIRQ:
		if (enable != 0) {
			mbox_sirq_slot_val = (uint8_t)(XEC_MBOX_SIRQ_EV_SLOT_VAL);
		}
		regs->SIRQ[MCHP_ESPI_SIRQ_MBOX_SIRQ] = mbox_sirq_slot_val;
		break;
	case MCHP_XEC_ESPI_PC_MBOX_ISRC_SIRQ_SMI:
		if (enable != 0) {
			mbox_sirq_slot_val = (uint8_t)(XEC_MBOX_SIRQ_SMI_SLOT_VAL);
		}
		regs->SIRQ[MCHP_ESPI_SIRQ_MBOX_SMI] = mbox_sirq_slot_val;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#undef	CONNECT_IRQ_MBOX0
#define	CONNECT_IRQ_MBOX0	connect_irq_mbox0
#undef	INIT_MBOX0
#define	INIT_MBOX0		init_mbox0

#endif /* CONFIG_ESPI_PERIPHERAL_XEC_MAILBOX */

#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(kbc0)),
	     "XEC kbc0 DT node is disabled!");

struct xec_kbc0_config {
	uintptr_t regbase;
	uint32_t ibf_ecia_info;
	uint32_t obe_ecia_info;
};

static const struct xec_kbc0_config xec_kbc0_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(kbc0)),
	.ibf_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(kbc0), girqs, 1),
	.obe_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(kbc0), girqs, 0),
};

static void kbc0_ibf_isr(const struct device *dev)
{
	struct kbc_regs *kbc_hw = (struct kbc_regs *)xec_kbc0_cfg.regbase;
	struct espi_xec_data *const data =
		(struct espi_xec_data *const)dev->data;

#ifdef CONFIG_ESPI_PERIPHERAL_KBC_IBF_EVT_DATA
	/* Chrome solution */
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_8042_KBC,
		ESPI_PERIPHERAL_NODATA,
	};
	struct espi_evt_data_kbc *kbc_evt =
				(struct espi_evt_data_kbc *)&evt.evt_data;
	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	kbc_evt->type = kbc_hw->EC_KBC_STS & MCHP_KBC_STS_CD ? 1 : 0;
	/* The data in KBC Input Buffer */
	kbc_evt->data = kbc_hw->EC_DATA;
	/* KBC Input Buffer Full event */
	kbc_evt->evt = HOST_KBC_EVT_IBF;
#else
	/* Windows solution */
	/* The high byte contains information from the host,
	 * and the lower byte specifies if the host sent
	 * a command or data. 1 = Command.
	 */
	uint32_t isr_data = ((kbc_hw->EC_KBC_STS & MCHP_KBC_STS_CD) <<
				E8042_ISR_CMD_DATA_POS);
	isr_data |= ((kbc_hw->EC_DATA & 0xFF) << E8042_ISR_DATA_POS);

	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_8042_KBC,
		.evt_data = isr_data
	};
#endif
	espi_send_callbacks(&data->callbacks, dev, evt);

	mchp_xec_ecia_info_girq_src_clr(xec_kbc0_cfg.ibf_ecia_info);
}

static void kbc0_obe_isr(const struct device *dev)
{
#ifdef CONFIG_ESPI_PERIPHERAL_KBC_OBE_CBK
	/* Chrome solution */
	struct espi_xec_data *const data =
		(struct espi_xec_data *const)dev->data;

	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_8042_KBC,
		ESPI_PERIPHERAL_NODATA,
	};
	struct espi_evt_data_kbc *kbc_evt =
				(struct espi_evt_data_kbc *)&evt.evt_data;

	/* Disable KBC OBE interrupt first */
	mchp_xec_ecia_info_girq_src_dis(xec_kbc0_cfg.obe_ecia_info);

	/*
	 * Notify application that host already read out data. The application
	 * might need to clear status register via espi_api_lpc_write_request()
	 * with E8042_CLEAR_FLAG opcode in callback.
	 */
	kbc_evt->evt = HOST_KBC_EVT_OBE;
	kbc_evt->data = 0;
	kbc_evt->type = 0;

	espi_send_callbacks(&data->callbacks, dev, evt);
#else
	/* Windows solution */
	/* disable and clear GIRQ interrupt and status */
	mchp_xec_ecia_info_girq_src_dis(xec_kbc0_cfg.obe_ecia_info);
#endif
	mchp_xec_ecia_info_girq_src_clr(xec_kbc0_cfg.obe_ecia_info);
}

/* dev is a pointer to espi0 device */
static int kbc0_rd_req(const struct device *dev, enum lpc_peripheral_opcode op,
		       uint32_t *data)
{
	struct kbc_regs *kbc_hw = (struct kbc_regs *)xec_kbc0_cfg.regbase;

	ARG_UNUSED(dev);

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		/* Make sure kbc 8042 is on */
		if (!(kbc_hw->KBC_CTRL & MCHP_KBC_CTRL_OBFEN)) {
			return -ENOTSUP;
		}

		switch (op) {
		case E8042_OBF_HAS_CHAR:
			/* EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = kbc_hw->EC_KBC_STS & MCHP_KBC_STS_OBF ? 1 : 0;
			break;
		case E8042_IBF_HAS_CHAR:
			*data = kbc_hw->EC_KBC_STS & MCHP_KBC_STS_IBF ? 1 : 0;
			break;
		case E8042_READ_KB_STS:
			*data = kbc_hw->EC_KBC_STS;
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

/* dev is a pointer to espi0 device */
static int kbc0_wr_req(const struct device *dev, enum lpc_peripheral_opcode op,
		       uint32_t *data)
{
	struct kbc_regs *kbc_hw = (struct kbc_regs *)xec_kbc0_cfg.regbase;

	volatile uint32_t __attribute__((unused)) dummy;

	ARG_UNUSED(dev);

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		/* Make sure kbc 8042 is on */
		if (!(kbc_hw->KBC_CTRL & MCHP_KBC_CTRL_OBFEN)) {
			return -ENOTSUP;
		}

		switch (op) {
		case E8042_WRITE_KB_CHAR:
			kbc_hw->EC_DATA = *data & 0xff;
			break;
		case E8042_WRITE_MB_CHAR:
			kbc_hw->EC_AUX_DATA = *data & 0xff;
			break;
		case E8042_RESUME_IRQ:
			mchp_xec_ecia_info_girq_src_clr(
				xec_kbc0_cfg.ibf_ecia_info);
			mchp_xec_ecia_info_girq_src_en(
				xec_kbc0_cfg.ibf_ecia_info);
			break;
		case E8042_PAUSE_IRQ:
			mchp_xec_ecia_info_girq_src_dis(
				xec_kbc0_cfg.ibf_ecia_info);
			break;
		case E8042_CLEAR_OBF:
			dummy = kbc_hw->HOST_AUX_DATA;
			break;
		case E8042_SET_FLAG:
			/* FW shouldn't modify these flags directly */
			*data &= ~(MCHP_KBC_STS_OBF | MCHP_KBC_STS_IBF |
				   MCHP_KBC_STS_AUXOBF);
			kbc_hw->EC_KBC_STS |= *data;
			break;
		case E8042_CLEAR_FLAG:
			/* FW shouldn't modify these flags directly */
			*data |= (MCHP_KBC_STS_OBF | MCHP_KBC_STS_IBF |
				  MCHP_KBC_STS_AUXOBF);
			kbc_hw->EC_KBC_STS &= ~(*data);
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int connect_irq_kbc0(const struct device *dev)
{
	/* clear GIRQ source */
	mchp_xec_ecia_info_girq_src_clr(xec_kbc0_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_clr(xec_kbc0_cfg.obe_ecia_info);

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_ibf, priority),
		    kbc0_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_ibf, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_obe, priority),
		    kbc0_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_obe, irq));

	/* enable GIRQ sources */
	mchp_xec_ecia_info_girq_src_en(xec_kbc0_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_en(xec_kbc0_cfg.obe_ecia_info);

	return 0;
}

static int init_kbc0(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;
	struct kbc_regs *kbc_hw = (struct kbc_regs *)xec_kbc0_cfg.regbase;

	kbc_hw->KBC_CTRL |= MCHP_KBC_CTRL_AUXH;
	kbc_hw->KBC_CTRL |= MCHP_KBC_CTRL_OBFEN;
	/* This is the activate register, but the HAL has a funny name */
	kbc_hw->KBC_PORT92_EN = MCHP_KBC_PORT92_EN;
	regs->IOHBAR[IOB_KBC] = ESPI_XEC_KBC_BAR_ADDRESS |
				MCHP_ESPI_IO_BAR_HOST_VALID;

	return 0;
}

#undef	CONNECT_IRQ_KBC0
#define	CONNECT_IRQ_KBC0	connect_irq_kbc0
#undef	INIT_KBC0
#define	INIT_KBC0		init_kbc0

#endif /* CONFIG_ESPI_PERIPHERAL_8042_KBC */

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO

static const struct xec_acpi_ec_config xec_acpi_ec0_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(acpi_ec0)),
	.ibf_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(acpi_ec0), girqs, 0),
	.obe_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(acpi_ec0), girqs, 1),
};

static void acpi_ec0_ibf_isr(const struct device *dev)
{
	struct espi_xec_data *const data =
		(struct espi_xec_data *const)dev->data;
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_HOST_IO, ESPI_PERIPHERAL_NODATA
	};
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA
	struct acpi_ec_regs *acpi_ec0_hw = (struct acpi_ec_regs *)xec_acpi_ec0_cfg.regbase;

	/* Updates to fit Chrome shim layer design */
	struct espi_evt_data_acpi *acpi_evt =
				(struct espi_evt_data_acpi *)&evt.evt_data;

	/* Host put data on input buffer of ACPI EC0 channel */
	if (acpi_ec0_hw->EC_STS & MCHP_ACPI_EC_STS_IBF) {
		/* Set processing flag before reading command byte */
		acpi_ec0_hw->EC_STS |= MCHP_ACPI_EC_STS_UD1A;
		/*
		 * Indicates if the host sent a command or data.
		 * 0 = data
		 * 1 = Command.
		 */
		acpi_evt->type = acpi_ec0_hw->EC_STS & MCHP_ACPI_EC_STS_CMD ? 1 : 0;
		acpi_evt->data = acpi_ec0_hw->OS2EC_DATA;
	}
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA */

	espi_send_callbacks(&data->callbacks, dev, evt);

	/* clear GIRQ status */
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec0_cfg.ibf_ecia_info);
}

static void acpi_ec0_obe_isr(const struct device *dev)
{
	/* disable and clear GIRQ status */
	mchp_xec_ecia_info_girq_src_dis(xec_acpi_ec0_cfg.obe_ecia_info);
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec0_cfg.obe_ecia_info);
}

static int eacpi_rd_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	struct acpi_ec_regs *acpi_ec0_hw = (struct acpi_ec_regs *)xec_acpi_ec0_cfg.regbase;

	ARG_UNUSED(dev);

	switch (op) {
	case EACPI_OBF_HAS_CHAR:
		/* EC has written data back to host. OBF is
		 * automatically cleared after host reads
		 * the data
		 */
		*data = acpi_ec0_hw->EC_STS & MCHP_ACPI_EC_STS_OBF ? 1 : 0;
		break;
	case EACPI_IBF_HAS_CHAR:
		*data = acpi_ec0_hw->EC_STS & MCHP_ACPI_EC_STS_IBF ? 1 : 0;
		break;
	case EACPI_READ_STS:
		*data = acpi_ec0_hw->EC_STS;
		break;
#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	case EACPI_GET_SHARED_MEMORY:
		*data = (uint32_t)ec_host_cmd_sram + CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE;
		break;
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */
	default:
		return -EINVAL;
	}

	return 0;
}

static int eacpi_wr_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	struct acpi_ec_regs *acpi_ec0_hw = (struct acpi_ec_regs *)xec_acpi_ec0_cfg.regbase;

	ARG_UNUSED(dev);

	switch (op) {
	case EACPI_WRITE_CHAR:
		acpi_ec0_hw->EC2OS_DATA = (*data & 0xff);
		break;
	case EACPI_WRITE_STS:
		acpi_ec0_hw->EC_STS = (*data & 0xff);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int connect_irq_acpi_ec0(const struct device *dev)
{
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec0_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec0_cfg.obe_ecia_info);

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_ibf, priority),
		    acpi_ec0_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_ibf, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_obe, priority),
		    acpi_ec0_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_obe, irq));

	mchp_xec_ecia_info_girq_src_en(xec_acpi_ec0_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_en(xec_acpi_ec0_cfg.obe_ecia_info);

	return 0;
}

static int init_acpi_ec0(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;

	regs->IOHBAR[IOB_ACPI_EC0] = ESPI_XEC_ACPI_EC0_BAR_ADDRESS |
				MCHP_ESPI_IO_BAR_HOST_VALID;

	return 0;
}

#undef	CONNECT_IRQ_ACPI_EC0
#define	CONNECT_IRQ_ACPI_EC0	connect_irq_acpi_ec0
#undef	INIT_ACPI_EC0
#define	INIT_ACPI_EC0		init_acpi_ec0

#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO */

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || \
	defined(CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT)

static const struct xec_acpi_ec_config xec_acpi_ec1_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(acpi_ec1)),
	.ibf_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(acpi_ec1), girqs, 0),
	.obe_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(acpi_ec1), girqs, 1),
};

static void acpi_ec1_ibf_isr(const struct device *dev)
{
	struct espi_xec_data *const data =
		(struct espi_xec_data *const)dev->data;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
		.evt_details = ESPI_PERIPHERAL_EC_HOST_CMD,
#else
		.evt_details = ESPI_PERIPHERAL_HOST_IO_PVT,
#endif
		.evt_data = ESPI_PERIPHERAL_NODATA
	};
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA
	struct acpi_ec_regs *acpi_ec1_hw = (struct acpi_ec_regs *)xec_acpi_ec1_cfg.regbase;

	/* Updates to fit Chrome shim layer design.
	 * Host put data on input buffer of ACPI EC1 channel.
	 */
	if (acpi_ec1_hw->EC_STS & MCHP_ACPI_EC_STS_IBF) {
		/* Set processing flag before reading command byte */
		acpi_ec1_hw->EC_STS |= MCHP_ACPI_EC_STS_UD1A;
		/* Read out input data and clear IBF pending bit */
		evt.evt_data = acpi_ec1_hw->OS2EC_DATA;
	}
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA */

	espi_send_callbacks(&data->callbacks, dev, evt);

	/* clear GIRQ status */
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec1_cfg.ibf_ecia_info);
}

static void acpi_ec1_obe_isr(const struct device *dev)
{
	/* disable and clear GIRQ status */
	mchp_xec_ecia_info_girq_src_dis(xec_acpi_ec1_cfg.obe_ecia_info);
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec1_cfg.obe_ecia_info);
}

static int connect_irq_acpi_ec1(const struct device *dev)
{
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec1_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec1_cfg.obe_ecia_info);

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_ibf, priority),
		    acpi_ec1_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_ibf, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_obe, priority),
		    acpi_ec1_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_obe, irq));

	mchp_xec_ecia_info_girq_src_en(xec_acpi_ec1_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_en(xec_acpi_ec1_cfg.obe_ecia_info);

	return 0;
}

static int init_acpi_ec1(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	regs->IOHBAR[IOB_ACPI_EC1] =
				(CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM << 16) |
				MCHP_ESPI_IO_BAR_HOST_VALID;
#else
	regs->IOHBAR[IOB_ACPI_EC1] =
		CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT_PORT_NUM |
		MCHP_ESPI_IO_BAR_HOST_VALID;
	regs->IOHBAR[IOB_MBOX] = ESPI_XEC_MBOX_BAR_ADDRESS |
				 MCHP_ESPI_IO_BAR_HOST_VALID;
#endif

	return 0;
}

#undef	CONNECT_IRQ_ACPI_EC1
#define	CONNECT_IRQ_ACPI_EC1	connect_irq_acpi_ec1
#undef	INIT_ACPI_EC1
#define	INIT_ACPI_EC1		init_acpi_ec1

#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD || CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT */

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(emi0)),
	     "XEC EMI0 DT node is disabled!");

struct xec_emi_config {
	uintptr_t regbase;
};

static const struct xec_emi_config xec_emi0_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(emi0)),
};

static int init_emi0(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;
	struct emi_regs *emi_hw =
		(struct emi_regs *)xec_emi0_cfg.regbase;

	regs->IOHBAR[IOB_EMI0] =
				(CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM << 16) |
				MCHP_ESPI_IO_BAR_HOST_VALID;

	emi_hw->MEM_BA_0 = (uint32_t)ec_host_cmd_sram;
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	emi_hw->MEM_RL_0 = CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE +
						CONFIG_ESPI_XEC_PERIPHERAL_ACPI_SHD_MEM_SIZE;
#else
	emi_hw->MEM_RL_0 = CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE;
#endif
	emi_hw->MEM_WL_0 = CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE;

	return 0;
}

#undef	INIT_EMI0
#define	INIT_EMI0		init_emi0

#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */

#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE

static void host_cus_opcode_enable_interrupts(void);
static void host_cus_opcode_disable_interrupts(void);

static int ecust_rd_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	ARG_UNUSED(dev);

	switch (op) {
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY:
		*data = (uint32_t)ec_host_cmd_sram;
		break;
	case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE:
		*data = CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE;
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}

static int ecust_wr_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	struct acpi_ec_regs *acpi_ec1_hw = (struct acpi_ec_regs *)xec_acpi_ec1_cfg.regbase;

	ARG_UNUSED(dev);

	switch (op) {
	case ECUSTOM_HOST_SUBS_INTERRUPT_EN:
		if (*data != 0) {
			host_cus_opcode_enable_interrupts();
		} else {
			host_cus_opcode_disable_interrupts();
		}
		break;
	case ECUSTOM_HOST_CMD_SEND_RESULT:
		/*
		 * Write result to the data byte.  This sets the OBF
		 * status bit.
		 */
		acpi_ec1_hw->EC2OS_DATA = (*data & 0xff);
		/* Clear processing flag */
		acpi_ec1_hw->EC_STS &= ~MCHP_ACPI_EC_STS_UD1A;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)

static int eacpi_shm_rd_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	ARG_UNUSED(dev);

	switch (op) {
	case EACPI_GET_SHARED_MEMORY:
		*data = (uint32_t)&ec_host_cmd_sram[CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE];
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int eacpi_shm_wr_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -EINVAL;
}

#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */


#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80

struct xec_p80bd_config {
	uintptr_t regbase;
	uint32_t ecia_info;
};

static const struct xec_p80bd_config xec_p80bd0_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(p80bd0)),
	.ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(p80bd0), girqs, 0),
};

/*
 * MEC172x P80 BIOS Debug Port hardware captures writes to its 4-byte I/O range
 * Hardware provides status indicating byte lane(s) of each write.
 * We must decode the byte lane information and produce one or more
 * notification packets.
 */
#ifndef CONFIG_ESPI_XEC_P80_MULTIBYTE
static void p80bd0_isr(const struct device *dev)
{
	struct espi_xec_data *const data =
		(struct espi_xec_data *const)dev->data;
	struct p80bd_regs *p80regs =
		(struct p80bd_regs *)xec_p80bd0_cfg.regbase;
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION, 0,
				  ESPI_PERIPHERAL_NODATA };
	int count = MCHP_P80_FIFO_READ_COUNT; /* limit ISR to 8 bytes */
	uint32_t dattr = p80regs->EC_DA;

	/* b[7:0]=8-bit value written, b[15:8]=attributes */
	while ((dattr & MCHP_P80BD_ECDA_NE) && (count--)) { /* Not empty? */
		/* espi_event protocol No Data value is 0 so pick a bit and
		 * set it. This depends on the application.
		 */
		evt.evt_data = (dattr & 0xffu) | BIT(16);
		switch (dattr & MCHP_P80BD_ECDA_LANE_MSK) {
		case MCHP_P80BD_ECDA_LANE_0:
			evt.evt_details |= (ESPI_PERIPHERAL_INDEX_0 << 16) |
					   ESPI_PERIPHERAL_DEBUG_PORT80;
			break;
		case MCHP_P80BD_ECDA_LANE_1:
			evt.evt_details |= (ESPI_PERIPHERAL_INDEX_1 << 16) |
					   ESPI_PERIPHERAL_DEBUG_PORT80;
			break;
		case MCHP_P80BD_ECDA_LANE_2:
			break;
		case MCHP_P80BD_ECDA_LANE_3:
			break;
		default:
			break;
		}

		if (evt.evt_details) {
			espi_send_callbacks(&data->callbacks, dev, evt);
			evt.evt_details = 0;
		}
		dattr = p80regs->EC_DA;
	}

	/* clear GIRQ status */
	mchp_xec_ecia_info_girq_src_clr(xec_p80bd0_cfg.ecia_info);
}
#else
static void p80bd0_isr(const struct device *dev)
{
	struct espi_xec_data *const data =
		(struct espi_xec_data *const)dev->data;
	struct p80bd_regs *p80regs =
		(struct p80bd_regs *)xec_p80bd0_cfg.regbase;
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION, 0,
				  ESPI_PERIPHERAL_NODATA };
	int count = MCHP_P80_FIFO_READ_COUNT; /* limit ISR to 8 bytes */
	uint32_t dattr = p80regs->EC_DA;
	uint8_t byte_count;
	uint8_t shift;

	/* b[7:0]=8-bit value written, b[15:8]=attributes */
	while ((dattr & MCHP_P80BD_ECDA_NE) && (count--)) { /* Not empty? */
		byte_count = 0;
		evt.evt_data = 0;

		byte_count = BIT((dattr & MCHP_P80BD_ECDA_LEN_MSK) >> MCHP_P80BD_ECDA_LEN_POS);
		if (byte_count <= MCHP_P80_MAX_BYTE_COUNT) {
			shift = 0;
			while (byte_count) {
				evt.evt_data |= (dattr & 0xffu) << (shift * 8);
				byte_count--;
				shift++;
				if (byte_count) {
					dattr = p80regs->EC_DA;
				}
			}

			evt.evt_details = ESPI_PERIPHERAL_DEBUG_PORT80;
			espi_send_callbacks(&data->callbacks, dev, evt);
		}
		dattr = p80regs->EC_DA;
	}

	/* clear GIRQ status */
	mchp_xec_ecia_info_girq_src_clr(xec_p80bd0_cfg.ecia_info);
}
#endif

static int connect_irq_p80bd0(const struct device *dev)
{
	mchp_xec_ecia_info_girq_src_clr(xec_p80bd0_cfg.ecia_info);

	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(p80bd0)),
		    DT_IRQ(DT_NODELABEL(acpi_ec1), priority),
		    p80bd0_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQN(DT_NODELABEL(p80bd0)));

	mchp_xec_ecia_info_girq_src_en(xec_p80bd0_cfg.ecia_info);

	return 0;
}

static int init_p80bd0(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;
	struct p80bd_regs *p80bd_hw =
		(struct p80bd_regs *)xec_p80bd0_cfg.regbase;

	regs->IOHBAR[IOB_P80BD] = ESPI_XEC_PORT80_BAR_ADDRESS |
				  MCHP_ESPI_IO_BAR_HOST_VALID;

	p80bd_hw->ACTV = 1;
	p80bd_hw->STS_IEN = MCHP_P80BD_SI_THR_IEN;

	return 0;
}

#undef	CONNECT_IRQ_P80BD0
#define	CONNECT_IRQ_P80BD0	connect_irq_p80bd0
#undef	INIT_P80BD0
#define	INIT_P80BD0		init_p80bd0

#endif /* CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80 */

#ifdef CONFIG_ESPI_PERIPHERAL_UART

#if CONFIG_ESPI_PERIPHERAL_UART_SOC_MAPPING == 0
int init_uart0(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;

	regs->IOHBAR[IOB_UART0] = ESPI_XEC_UART0_BAR_ADDRESS |
				  MCHP_ESPI_IO_BAR_HOST_VALID;

	return 0;
}

#undef	INIT_UART0
#define	INIT_UART0		init_uart0

#elif CONFIG_ESPI_PERIPHERAL_UART_SOC_MAPPING == 1
int init_uart1(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;

	regs->IOHBAR[IOB_UART1] = ESPI_XEC_UART0_BAR_ADDRESS |
				  MCHP_ESPI_IO_BAR_HOST_VALID;

	return 0;
}

#undef	INIT_UART1
#define	INIT_UART1		init_uart1
#endif /* CONFIG_ESPI_PERIPHERAL_UART_SOC_MAPPING */
#endif /* CONFIG_ESPI_PERIPHERAL_UART */

typedef int (*host_dev_irq_connect)(const struct device *dev);

static const host_dev_irq_connect hdic_tbl[] = {
	CONNECT_IRQ_MBOX0,
	CONNECT_IRQ_KBC0,
	CONNECT_IRQ_ACPI_EC0,
	CONNECT_IRQ_ACPI_EC1,
	CONNECT_IRQ_ACPI_EC2,
	CONNECT_IRQ_ACPI_EC3,
	CONNECT_IRQ_ACPI_EC4,
	CONNECT_IRQ_ACPI_PM1,
	CONNECT_IRQ_EMI0,
	CONNECT_IRQ_EMI1,
	CONNECT_IRQ_EMI2,
	CONNECT_IRQ_RTC0,
	CONNECT_IRQ_P80BD0,
};

typedef int (*host_dev_init)(const struct device *dev);

static const host_dev_init hd_init_tbl[] = {
	INIT_MBOX0,
	INIT_KBC0,
	INIT_ACPI_EC0,
	INIT_ACPI_EC1,
	INIT_ACPI_EC2,
	INIT_ACPI_EC3,
	INIT_ACPI_EC4,
	INIT_ACPI_PM1,
	INIT_EMI0,
	INIT_EMI1,
	INIT_EMI2,
	INIT_RTC0,
	INIT_P80BD0,
	INIT_UART0,
	INIT_UART1,
};

int xec_host_dev_connect_irqs(const struct device *dev)
{
	int ret = 0;

	for (int i = 0; i < ARRAY_SIZE(hdic_tbl); i++) {
		if (hdic_tbl[i] == NULL) {
			continue;
		}

		ret = hdic_tbl[i](dev);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

int xec_host_dev_init(const struct device *dev)
{
	int ret = 0;

	for (int i = 0; i < ARRAY_SIZE(hd_init_tbl); i++) {
		if (hd_init_tbl[i] == NULL) {
			continue;
		}

		ret = hd_init_tbl[i](dev);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL

typedef int (*xec_lpc_req)(const struct device *,
			   enum lpc_peripheral_opcode,
			   uint32_t *);

struct espi_lpc_req {
	uint16_t opcode_start;
	uint16_t opcode_max;
	xec_lpc_req rd_req;
	xec_lpc_req wr_req;
};

static const struct espi_lpc_req espi_lpc_req_tbl[] = {
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	{ E8042_START_OPCODE, E8042_MAX_OPCODE, kbc0_rd_req, kbc0_wr_req },
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	{ EACPI_START_OPCODE, EACPI_MAX_OPCODE, eacpi_rd_req, eacpi_wr_req },
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	{ EACPI_GET_SHARED_MEMORY, EACPI_GET_SHARED_MEMORY, eacpi_shm_rd_req, eacpi_shm_wr_req},
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	{ ECUSTOM_START_OPCODE, ECUSTOM_MAX_OPCODE, ecust_rd_req, ecust_wr_req},
#endif
};

static int espi_xec_lpc_req(const struct device *dev,
			    enum lpc_peripheral_opcode op,
			    uint32_t  *data, uint8_t write)
{
	ARG_UNUSED(dev);

	for (int i = 0; i < ARRAY_SIZE(espi_lpc_req_tbl); i++) {
		const struct espi_lpc_req *req = &espi_lpc_req_tbl[i];

		if ((op >= req->opcode_start) && (op <= req->opcode_max)) {
			if (write) {
				return req->wr_req(dev, op, data);
			} else {
				return req->rd_req(dev, op, data);
			}
		}
	}

	return -ENOTSUP;
}

/* dev = pointer to espi0 device */
int espi_xec_read_lpc_request(const struct device *dev,
			      enum lpc_peripheral_opcode op,
			      uint32_t  *data)
{
	return espi_xec_lpc_req(dev, op, data, 0);
}

int espi_xec_write_lpc_request(const struct device *dev,
			       enum lpc_peripheral_opcode op,
			       uint32_t  *data)
{
	return espi_xec_lpc_req(dev, op, data, 1);
}
#else
int espi_xec_write_lpc_request(const struct device *dev,
				      enum lpc_peripheral_opcode op,
				      uint32_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -ENOTSUP;
}

int espi_xec_read_lpc_request(const struct device *dev,
				     enum lpc_peripheral_opcode op,
				     uint32_t  *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -ENOTSUP;
}
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)
static void host_cus_opcode_enable_interrupts(void)
{
	/* Enable host KBC sub-device interrupt */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_8042_KBC)) {
		mchp_xec_ecia_info_girq_src_en(xec_kbc0_cfg.ibf_ecia_info);
		mchp_xec_ecia_info_girq_src_en(xec_kbc0_cfg.obe_ecia_info);
	}

	/* Enable host ACPI EC0 (Host IO) and
	 * ACPI EC1 (Host CMD) sub-device interrupt
	 */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO) ||
	    IS_ENABLED(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)) {
		mchp_xec_ecia_info_girq_src_en(xec_acpi_ec0_cfg.ibf_ecia_info);
		mchp_xec_ecia_info_girq_src_en(xec_acpi_ec0_cfg.obe_ecia_info);
		mchp_xec_ecia_info_girq_src_en(xec_acpi_ec1_cfg.ibf_ecia_info);
	}

	/* Enable host Port80 sub-device interrupt installation */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)) {
		mchp_xec_ecia_info_girq_src_en(xec_p80bd0_cfg.ecia_info);
	}
}

static void host_cus_opcode_disable_interrupts(void)
{
	/* Disable host KBC sub-device interrupt */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_8042_KBC)) {
		mchp_xec_ecia_info_girq_src_dis(xec_kbc0_cfg.ibf_ecia_info);
		mchp_xec_ecia_info_girq_src_dis(xec_kbc0_cfg.obe_ecia_info);
	}

	/* Disable host ACPI EC0 (Host IO) and
	 * ACPI EC1 (Host CMD) sub-device interrupt
	 */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO) ||
		IS_ENABLED(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)) {
		mchp_xec_ecia_info_girq_src_dis(xec_acpi_ec0_cfg.ibf_ecia_info);
		mchp_xec_ecia_info_girq_src_dis(xec_acpi_ec0_cfg.obe_ecia_info);
		mchp_xec_ecia_info_girq_src_dis(xec_acpi_ec1_cfg.ibf_ecia_info);
	}

	/* Disable host Port80 sub-device interrupt installation */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)) {
		mchp_xec_ecia_info_girq_src_dis(xec_p80bd0_cfg.ecia_info);
	}
}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
