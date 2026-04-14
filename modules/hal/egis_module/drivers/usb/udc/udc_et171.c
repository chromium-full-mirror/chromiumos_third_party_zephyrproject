/**
 * Copyright (c) 2025 Egis Technology Inc
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  udc_et171.c
 * @brief Egis USB device controller (UDC) driver
 */

#include <string.h>
#include <stdio.h>

//#include <soc.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/pm/policy.h>

#include "udc_common.h"

#include <assert.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_DCACHE
#	ifndef CONFIG_DCACHE_LINE_SIZE 
#		error *** The line size of dcache must be declared !! ***
#	endif
#endif

LOG_MODULE_REGISTER(udc_et171, CONFIG_UDC_DRIVER_LOG_LEVEL);

/* USB device controller access from devicetree */
#define DT_DRV_COMPAT egis_et171_usbd

#define LOCK_PROCESS(lock_session) \
	int32_t __##lock_session = arch_irq_lock()
#define UNLOCK_PROCESS(lock_session) \
	arch_irq_unlock(__##lock_session)

//================================================================== ET171 SMU

// et171 core
#include <et171_hal/et171.h>
#include <et171_hal/et171_hal_smu.h>

#define IN_REG(x)     sys_read32((uint32_t)&((AOSMU_RegDef*)AOSMU_BASE)->x)
#define OUT_REG(x, d) sys_write32(d, (uint32_t)&((AOSMU_RegDef*)AOSMU_BASE)->x)

//==================================================================

#include <et171_usb/cusbd_if.h>
#include <et171_usb/cusb_dma_if.h>
#include <et171_usb/byteorder.h>
#include <et171_usb/sduc_regs.h>
#include <usb_mem.h>
//==================================================================

//DT_N_S_soc_S_usb_e8100000_P_reg
#define USB_REGS_BASE DT_REG_ADDR_BY_IDX(DT_NODELABEL(usb), 0)

static CUSBD_Config usbd_config = {
    .regBase = USB_REGS_BASE, // address where USB core is mapped
    .epIN = {
		{.bufferingValue = 1, .maxPacketSize = 64, .startBuf = 0},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 0},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 4},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 8},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 12},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 16},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 20},
		{.bufferingValue = 1, .maxPacketSize = 512 , .startBuf = 64 + 512 * 24},
	},
    .epOUT = {
		{.bufferingValue = 1, .maxPacketSize = 64, .startBuf = 0},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 2},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 6},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 10},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 14},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 18},
		{.bufferingValue = 1, .maxPacketSize = 1024, .startBuf = 64 + 512 * 22},
		{.bufferingValue = 1, .maxPacketSize = 512 , .startBuf = 64 + 512 * 25},
	},
    .dmultEnabled = 1, // set to 1 if scatter/gather DMA available
    .dmaInterfaceWidth = CUSBD_DMA_32_WIDTH,
    .dmaSupport = 1,
    .isOtg = 0,
    .isDevice = 1
};

struct udc_et171_work_event {
	sys_snode_t _node;
	enum {
		WORK_EVT_SETUP,
		WORK_EVT_COMPLETE,
	} type;
	union {
		struct {
			void *privateData;
			CH9_UsbSetup ctrl;
		} setup;
		struct {
			struct CUSBD_Ep *ep;
			struct CUSBD_Req *req;
		} complete;
	};
};
#define endof(s,m) (offsetof(s,m) + sizeof(((s*)0)->m)) 

#define SNODE_TO_EVENT(node) CONTAINER_OF((node), struct udc_et171_work_event, _node)

static struct udc_et171_device {
	struct udc_data _data;
	struct k_work _work;
	sys_slist_t _slist;
	uint8_t _usbd_config_init_flag;
#if defined(CONFIG_PM)
	uint8_t _pm_policy_locked;
#endif
} udc_et171_device_inst;

#define DEV_TO_INST(dev) CONTAINER_OF((dev)->data, struct udc_et171_device, _data)
#define WORK_TO_INST(work) CONTAINER_OF((work), struct udc_et171_device, _work)

#if defined(CONFIG_UDC_ET171_WORKQUEUE)
K_KERNEL_STACK_DEFINE(udc_et171_work_q_stack, CONFIG_UDC_ET171_WORKQUEUE_STACK_SIZE);
static struct k_work_q udc_et171_work_q;
static int udc_et171_work_q_init(void)
{
	k_work_queue_start(&udc_et171_work_q,
			   udc_et171_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(udc_et171_work_q_stack),
			   CONFIG_UDC_ET171_WORKQUEUE_PRIORITY, NULL);
	k_thread_name_set(&udc_et171_work_q.thread, "udc_work_q");

	return 0;
}
#define udc_et171_get_work_q() (&udc_et171_work_q)
#else
#define udc_et171_get_work_q() udc_get_work_q()
#endif

static inline void put_work_event(struct udc_et171_device *inst, struct udc_et171_work_event *evt) {
	int32_t key = arch_irq_lock();
	sys_slist_append(&inst->_slist, &evt->_node);
	arch_irq_unlock(key);
}
static inline struct udc_et171_work_event* get_work_event(struct udc_et171_device *inst) {
	int32_t key = arch_irq_lock();
	sys_snode_t *node = sys_slist_get(&inst->_slist);
	arch_irq_unlock(key);
	return node ? SNODE_TO_EVENT(node) : NULL;
}

#if defined(CONFIG_PM)
static void lock_pm_policy(const struct device * dev, uint8_t lock)
{
	struct udc_et171_device *dev_inst = DEV_TO_INST(dev);

	unsigned int irq_lock_key = arch_irq_lock();
	if (dev_inst->_pm_policy_locked != lock)
	{
		const struct pm_state_info *cpu_states;
		uint32_t num_cpu_states = pm_state_cpu_get_all(0, &cpu_states);

		if (lock) {
			for (uint32_t i = 0; i < num_cpu_states; i++) {
				pm_policy_state_lock_get(cpu_states[i].state, PM_ALL_SUBSTATES);
			}
		} else {
			for (uint32_t i = 0; i < num_cpu_states; i++) {
				pm_policy_state_lock_put(cpu_states[i].state, PM_ALL_SUBSTATES);
			}
		}
		dev_inst->_pm_policy_locked = lock;
	}
    arch_irq_unlock(irq_lock_key);
}
#else
#define lock_pm_policy(dev, lock);
#endif

static void isr_cb_connect(void *privateData)  {
	struct device* const dev = ((struct device**)privateData)[-1];
	LOG_DBG("%s", __func__);
	udc_submit_event(dev, UDC_EVT_RESET, 0);
}

static uint32_t isr_cb_setup(void *privateData, CH9_UsbSetup *ctrl)  {

	struct device* const dev = ((struct device**)privateData)[-1];
	struct udc_et171_device* inst = DEV_TO_INST(dev);
	struct udc_et171_work_event *evt =
		(struct udc_et171_work_event*)USB_mem_alloc(endof(struct udc_et171_work_event, setup));

	if (evt) {
		evt->type = WORK_EVT_SETUP;
		evt->setup.privateData = privateData;
		evt->setup.ctrl = *ctrl;
		put_work_event(inst, evt);
		k_work_submit_to_queue(udc_et171_get_work_q(), &inst->_work);
		return 0;
	}

	return -1;
}
static uint32_t work_cb_setup(void *privateData, CH9_UsbSetup *ctrl)  {

	struct device* const dev = ((struct device**)privateData)[-1];

	LOG_INF("setup( Type = %02X, Req = %02X, Value = %04X, Index = %04X, Len = %04X )", ctrl->bmRequestType, ctrl->bRequest, ctrl->wValue, ctrl->wIndex, ctrl->wLength);

	udc_setup_received(dev, ctrl);

	return 0;
}
static void isr_cb_suspend(void *privateData)  {
	struct device* const dev = ((struct device**)privateData)[-1];
	LOG_DBG("%s", __func__);
	udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
	lock_pm_policy(dev, false);
}
static void isr_cb_resume(void *privateData)  {
	struct device* const dev = ((struct device**)privateData)[-1];
	LOG_DBG("%s", __func__);
	OUT_REG(USB2PHY, IN_REG(USB2PHY) & ~SMU_USB2PHY_WAKEUP);
	udc_submit_event(dev, UDC_EVT_RESUME, 0);
	lock_pm_policy(dev, true);
}
static void * mem_cb_alloc(void * privateData, uint32_t requireSize)  {
    return USB_mem_alloc(requireSize);
}
static void mem_cb_free(void * privateData, void* usbRequest)  {
    if (usbRequest) {
        USB_mem_free(usbRequest);
    }
}
static CUSBD_Callbacks callback = {
	.connect = isr_cb_connect,
	.setup = isr_cb_setup,
	.suspend = isr_cb_suspend,
	.resume = isr_cb_resume,
	.usbRequestMemAlloc = mem_cb_alloc,
	.usbRequestMemFree = mem_cb_free
};
#define NUM_OF_EP_MAX		DT_INST_PROP(0, num_bidir_endpoints)

static struct udc_ep_config ep_cfg_out[NUM_OF_EP_MAX];
static struct udc_ep_config ep_cfg_in[NUM_OF_EP_MAX];

static struct CH9_UsbEndpointDescriptor ep_desc_out[NUM_OF_EP_MAX];
static struct CH9_UsbEndpointDescriptor ep_desc_in[NUM_OF_EP_MAX];

static CUSBD_Ep *udc_et171_get_usbd_ep(void *privateData, uint8_t ep_addr) {
	const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle
	CUSBD_Dev *usbd_dev;
	drv->getDevInstance(privateData, &usbd_dev);
	if (USB_EP_GET_IDX(ep_addr) == 0) {
		return usbd_dev->ep0;
	} else {
		for (CUSBD_ListHead *list = usbd_dev->epList.next; list != &usbd_dev->epList; list = list->next) {
			CUSBD_Ep *ep = CONTAINER_OF(list, CUSBD_Ep, epList);
			if (ep->address == ep_addr) {
				return ep;
			}
		}
	}
	return NULL;
}

static void udc_et171_ep0_transfer__cb_complete(struct CUSBD_Ep *ep, struct CUSBD_Req * req)  {
	const struct device *dev = req->context;
	__unused const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle
	__unused struct udc_data *data = dev->data;
	__unused void* pD = data->priv;

	assert(USB_EP_GET_IDX(ep->address) == 0); // control ep

	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, ep->address);
	// lock
	struct net_buf* buf = udc_buf_get(cfg);
	// unlock

	if (buf) {
		LOG_INF("ep%02X_complete( %p : %u ) ep = %p, req = %p", ep->address, buf, req->actual, ep, req);

		if (req->status != 0) {
			udc_submit_ep_event(dev, buf, -ECONNABORTED);
		} else {
			if (USB_EP_DIR_IS_OUT(ep->address)) {
				net_buf_add(buf, req->actual);
			}
			udc_submit_ep_event(dev, buf, 0);
		}
	}

	udc_ep_set_busy(cfg, false);
	// unlock
	drv->reqFree(pD, ep, req);
}

static void udc_et171_epX_transfer__cb_complete(struct CUSBD_Ep *ep, struct CUSBD_Req * req)  {
	const struct device *dev = req->context;
	const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	void* pD = data->priv;

	assert(USB_EP_GET_IDX(ep->address) != 0); // not control ep

	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, ep->address);
	// lock
	struct net_buf* buf = udc_buf_get(cfg);
	// unlock

	while (buf) {
		LOG_INF("ep%02X_complete( %p : %u ) ep = %p, req = %p", ep->address, buf, req->actual, ep, req);
		if (req->status == 0) {
			if (USB_EP_DIR_IS_OUT(ep->address)) {
				net_buf_add(buf, req->actual);
			}
			udc_submit_ep_event(dev, buf, 0);
			break;
		}
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
		// lock
		buf = udc_buf_get(cfg); // drop next
		// unlock
	}

	// lock
	buf = udc_buf_peek(cfg);
	if (buf) {
		size_t transfer_size = USB_EP_DIR_IS_OUT(ep->address) ? net_buf_tailroom(buf) : buf->len;
		// unlock
		req->buf = buf->data;
		req->dma = (uintptr_t)buf->data;
		req->length = transfer_size;
		req->streamId = 0;

		LOG_INF("ep%02X_complete( %p : %u )::reqQueue( ep = %p, req = %p)", ep->address, buf, buf->len, ep, req);

		LOCK_PROCESS(queue_transfer);
		ep->ops->reqQueue(pD, ep, req);
		UNLOCK_PROCESS(queue_transfer);

	} else {
		udc_ep_set_busy(cfg, false);
		// unlock
		drv->reqFree(pD, ep, req);
	}
}

static void udc_et171_transfer__cb_complete(struct CUSBD_Ep *ep, struct CUSBD_Req * req)  {
	const struct device *dev = req->context;
	struct udc_et171_work_event *evt =
		(struct udc_et171_work_event*)USB_mem_alloc(endof(struct udc_et171_work_event, complete));

	if (evt) {
		struct udc_et171_device* inst = DEV_TO_INST(dev);

		evt->type = WORK_EVT_COMPLETE;
		evt->complete.ep = ep;
		evt->complete.req = req;
		put_work_event(inst, evt);
		k_work_submit_to_queue(udc_et171_get_work_q(), &inst->_work);
	} else {
		struct udc_data *data = dev->data;
		void* pD = data->priv;
		const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle

		LOG_ERR("ep%02X_complete( %p ) out of memory", ep->address, req);
		drv->reqFree(pD, ep, req);
	}
}

static void work_cb_complete(struct CUSBD_Ep *ep, struct CUSBD_Req * req) {
	if (USB_EP_GET_IDX(ep->address) == 0) { // controller
		udc_et171_ep0_transfer__cb_complete(ep, req);
	} else {
		udc_et171_epX_transfer__cb_complete(ep, req);
	}
}

static int udc_et171_ep_enqueue(const struct device *dev, struct udc_ep_config *cfg, struct net_buf *buf)
{
	LOG_INF("ep%02X_enqueue( %p : %u )", cfg->addr, buf, buf->len);

	const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	void* pD = data->priv;

	if (USB_EP_GET_IDX(cfg->addr) == 0) { // control ep
		struct udc_buf_info *buf_info = udc_get_buf_info(buf);

		if (buf_info->setup) {
			udc_buf_put(cfg, buf);
			return 0;
		}

		// lock
		if (udc_ep_is_busy(cfg)) {
			// unlock
			return -EBUSY;
		}
		else if (udc_get_buf_info(buf)->status) {
			// unlock
			// skip status, because caps.out_ack == true;
			assert(buf->len == 0);
			// status stage, finished
			LOG_DBG("%s(%p) finish", "ep0_complete already", buf);
			udc_submit_ep_event(dev, buf, 0);
			return 0;
		}
		udc_ep_set_busy(cfg, true);
		udc_buf_put(cfg, buf);
		// unlock
	} else {
		// lock
		udc_buf_put(cfg, buf);

		if (udc_ep_is_busy(cfg)) {
			// unlock
			return 0;
		}
		udc_ep_set_busy(cfg, true);
		// unlock
	}

	int ret = 0;
	uint32_t status;
	uint32_t transfer_size = USB_EP_DIR_IS_OUT(cfg->addr) ? net_buf_tailroom(buf) : buf->len;
	CUSBD_Ep *ep = udc_et171_get_usbd_ep(pD, cfg->addr);
	CUSBD_Req* req = NULL;
	drv->reqAlloc(pD, ep, &req);
	if (!req) {
		ret = -ENOMEM;
		goto exit;
	}

	req->buf = buf->data;
	req->dma = (uintptr_t)buf->data;
	req->context = (void*)dev;
	req->complete = udc_et171_transfer__cb_complete;
	req->length = transfer_size;
	req->streamId = 0;

	LOG_INF("ep%02X_enqueue()::reqQueue( ep = %p, req = %p)", cfg->addr, ep, req);

	LOCK_PROCESS(queue_transfer);
	status = ep->ops->reqQueue(pD, ep, req);
	UNLOCK_PROCESS(queue_transfer);
	
	if (status != 0) {
		LOG_ERR("%s:%u Queue fail, status = %d\n", __func__, __LINE__, status);
		drv->reqFree(pD, ep, req);
		ret = -EINVAL;
		goto exit;
	}

exit:
	if (ret != 0) {
		udc_buf_get(cfg);
		udc_ep_set_busy(cfg, false);
		udc_submit_ep_event(dev, buf, ret);
		// forword the 'ret' to ep_event handler only
	}

	return 0;
}

static int udc_et171_ep_dequeue(const struct device *dev, struct udc_ep_config *cfg)
{
	const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	void* pD = data->priv;
	CUSBD_Ep *ep = udc_et171_get_usbd_ep(pD, cfg->addr);

	LOCK_PROCESS(dequeue);
	udc_ep_cancel_queued(dev, cfg);
	UNLOCK_PROCESS(dequeue);
	if (drv->reqDequeue(pD, ep, NULL) == 0) {
		while (drv->reqDequeue(pD, ep, NULL) == 0);
		drv->epFifoFlush(pD, ep);
	}

	return 0;
}

static int udc_et171_ep_enable(const struct device *dev, struct udc_ep_config *cfg)
{
	__unused const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	void* pD = data->priv;

	LOG_INF("Enable ep 0x%02x", cfg->addr);

	if (USB_EP_GET_IDX(cfg->addr) == 0) { // control ep
		// do nothing
	} else {
		CUSBD_Ep *ep = udc_et171_get_usbd_ep(pD, cfg->addr);
		if (ep) {
			CH9_UsbEndpointDescriptor *desc = USB_EP_DIR_IS_OUT(cfg->addr)
				? &ep_desc_out[USB_EP_GET_IDX(cfg->addr)]
				: &ep_desc_in[USB_EP_GET_IDX(cfg->addr)];
			desc->bLength = CH9_USB_DS_ENDPOINT;
			desc->bDescriptorType = CH9_USB_DT_ENDPOINT;
			desc->bEndpointAddress = cfg->addr;
			desc->bmAttributes = cfg->attributes;
			desc->wMaxPacketSize = cpuToLe16(cfg->mps);
			desc->bInterval = cfg->interval;
			ep->ops->epEnable(pD, ep, desc);
		}
	}
	return 0;
}

static int udc_et171_ep_disable(const struct device *dev, struct udc_ep_config *cfg)
{
	__unused const CUSBD_OBJ *drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	void* pD = data->priv;

	LOG_INF("Disable ep 0x%02x", cfg->addr);

	if (USB_EP_GET_IDX(cfg->addr) == 0) { // control ep
		// do nothing
	} else {
		CUSBD_Ep *ep = udc_et171_get_usbd_ep(pD, cfg->addr);
		if (ep) {
			ep->ops->epDisable(pD, ep);
		}
	}

	return 0;
}

static int udc_et171_ep_set_halt(const struct device *dev, struct udc_ep_config *cfg)
{
	__unused const CUSBD_OBJ *drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	void* pD = data->priv;

	LOG_INF("Set halt ep 0x%02x", cfg->addr);

	CUSBD_Ep *ep = udc_et171_get_usbd_ep(pD, cfg->addr);
	if (ep) {
		ep->ops->epSetHalt(pD, ep, 1);
	}

	return 0;
}

static int udc_et171_ep_clear_halt(const struct device *dev, struct udc_ep_config *cfg)
{
	__unused const CUSBD_OBJ *drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	void* pD = data->priv;

	LOG_INF("Clear halt ep 0x%02x", cfg->addr);

	CUSBD_Ep *ep = udc_et171_get_usbd_ep(pD, cfg->addr);
	if (ep) {
		ep->ops->epSetHalt(pD, ep, 0);
	}

	return 0;
}

static int udc_et171_set_address(const struct device *dev, const uint8_t addr)
{
	__unused const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle
	__unused struct udc_data *data = dev->data;
	__unused void* pD = data->priv;

	ARG_UNUSED(dev);
	ARG_UNUSED(addr);

	// ((UDC_Device*)pD)->deviceAddress = setup->wValue & 0x7F;
	// ((UDC_Device*)pD)->usbDev.state = CH9_USB_STATE_ADDRESS;

	return 0;
}

static int udc_et171_host_wakeup(const struct device *dev)
{
	LOG_DBG("Host wakeup request");

	ARG_UNUSED(dev);

#ifdef ENABLE_USB_PLL_EN
	OUT_REG(USB2PHY, IN_REG(USB2PHY) | SMU_USB2PHY_PLL_EN);
#endif // ENABLE_USB_PLL_EN
	OUT_REG(USB2PHY, IN_REG(USB2PHY) | SMU_USB2PHY_WAKEUP);

	return 0;
}

static void udc_et171_irq_handler(const struct device *dev)
{
	const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	void* pD = data->priv;

	drv->isr(pD);
}

static int udc_et171_enable(const struct device *dev)
{
	const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	void* pD = data->priv;

	// start
	drv->start(pD);

	// enable_usb_int
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), udc_et171_irq_handler, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	lock_pm_policy(dev, true);

	return 0;
}

static int udc_et171_disable(const struct device *dev)
{
	const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	void* pD = data->priv;

	// disable_usb_int
	irq_disable(DT_INST_IRQN(0));

	// stop all.
	drv->stop(pD);

	lock_pm_policy(dev, false);

	return 0;
}

static int udc_et171_init(const struct device *dev)
{
	const CUSBD_OBJ *drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	struct CUSBD_Config *config = (struct CUSBD_Config*)dev->config;
	int ret;
	LOG_INF("Initialized");

#ifdef ENABLE_USB_PLL_EN
	OUT_REG(USB2PHY, IN_REG(USB2PHY) | SMU_USB2PHY_PLL_EN);
#else // ENABLE_USB_PLL_EN
	OUT_REG(USB2PHY, IN_REG(USB2PHY) & ~SMU_USB2PHY_PLL_EN);
#endif //ENABLE_USB_PLL_EN

	HAL_SMU_ResetIP(SMU_RST_USB2);

    CUSBD_SysReq sysReq; // returns driver requirements

	ret = drv->probe(config, &sysReq);
	if (ret != 0) {
		return -EIO;
	}

	void *p_dev = USB_mem_alloc(sizeof(struct device **) + sysReq.privDataSize);
	*(const struct device**)p_dev = dev;

	void* pD = (void*)((uint8_t *)p_dev + sizeof(dev));

    config->trbAddr = (uint8_t *)SRAM2NONCACHE_ADDR(USB_mem_alloc(sysReq.trbMemSize));

	config->trbDmaAddr = (uintptr_t)config->trbAddr;

	ret = drv->init(pD, config, &callback);
	if (ret != 0) {
		return -EIO;
	}
	data->priv = pD;

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control out endpoint");
		return -EIO;
	}
	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL,	64, 0)) {
		LOG_ERR("Failed to enable control in endpoint");
		return -EIO;
	}

	return 0;
}

static int udc_et171_shutdown(const struct device *dev)
{
	const CUSBD_OBJ * drv = CUSBD_GetInstance(); // get driver handle
	struct udc_data *data = dev->data;
	struct CUSBD_Config *config = (struct CUSBD_Config*)dev->config;
	void* pD = data->priv;
	
	LOG_INF("shutdown");

	udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT);
	udc_ep_disable_internal(dev, USB_CONTROL_EP_IN);

	if (pD) {
		drv->destroy(pD);
    
		struct device** p_dev = (struct device**)pD - 1;
    	USB_mem_free((void*)p_dev);

		data->priv = NULL;
	}

    if (config->trbAddr)
    {
        USB_mem_free((uint8_t*)SRAM2CACHE_ADDR(config->trbAddr));
        config->trbAddr = NULL;
    }

    HAL_SMU_ResetLow(SMU_RST_USB2);

	return 0;
}

static void udc_et171_work_handler(struct k_work *item)
{
	struct udc_et171_device *inst = WORK_TO_INST(item);
	struct k_mutex *mutex = &inst->_data.mutex;
	struct udc_et171_work_event *evt;

	while ((evt = get_work_event(inst)) != NULL) {
		k_mutex_lock(mutex, K_FOREVER); // udc_lock_internal(dev, K_FOREVER);
		switch(evt->type)
		{
		case WORK_EVT_SETUP:
			work_cb_setup(evt->setup.privateData, &evt->setup.ctrl);
			break;
		case WORK_EVT_COMPLETE:
			work_cb_complete(evt->complete.ep, evt->complete.req);
			break;
		}
		k_mutex_unlock(mutex); // udc_unlock_internal(dev);
		USB_mem_free(evt);
	}
}

static int udc_et171_driver_init(const struct device *dev)
{
    int ret;
	LOG_INF("Preinit");

	struct udc_data *data = dev->data;
	struct udc_et171_device *inst = DEV_TO_INST(dev);

	k_mutex_init(&data->mutex);
	sys_slist_init(&inst->_slist);
	k_work_init(&inst->_work, udc_et171_work_handler);

#if defined(CONFIG_UDC_ET171_WORKQUEUE)
	udc_et171_work_q_init();
#endif

	for (int i = 0; i < ARRAY_SIZE(ep_cfg_out); i++) {
		ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			ep_cfg_out[i].caps.control = 1;
			ep_cfg_out[i].caps.mps = 64;
		} else {
			ep_cfg_out[i].caps.bulk = 1;
			ep_cfg_out[i].caps.interrupt = 1;
			ep_cfg_out[i].caps.iso = 1;
			ep_cfg_out[i].caps.mps = 64;
		}

		ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		ret = udc_register_ep(dev, &ep_cfg_out[i]);
		if (ret != 0) {
			LOG_ERR("Failed to register endpoint");
			return ret;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(ep_cfg_in); i++) {
		ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			ep_cfg_in[i].caps.control = 1;
			ep_cfg_in[i].caps.mps = 64;
		} else {
			ep_cfg_in[i].caps.bulk = 1;
			ep_cfg_in[i].caps.interrupt = 1;
			ep_cfg_in[i].caps.iso = 1;
			ep_cfg_in[i].caps.mps = 64;
		}

		ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		ret = udc_register_ep(dev, &ep_cfg_in[i]);
		if (ret != 0) {
			LOG_ERR("Failed to register endpoint");
			return ret;
		}
	}

	data->caps.rwup = true;
	data->caps.out_ack = true;
	data->caps.mps0 = UDC_MPS0_64;

	return 0;
}

static void udc_et171_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_et171_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static const struct udc_api udc_et171_api = {
	.device_speed = NULL,
	.ep_enqueue = udc_et171_ep_enqueue,
	.ep_dequeue = udc_et171_ep_dequeue,
	.ep_set_halt = udc_et171_ep_set_halt,
	.ep_clear_halt = udc_et171_ep_clear_halt,
	.ep_try_config = NULL,
	.ep_enable = udc_et171_ep_enable,
	.ep_disable = udc_et171_ep_disable,
	.host_wakeup = udc_et171_host_wakeup,
	.set_address = udc_et171_set_address,
	.enable = udc_et171_enable,
	.disable = udc_et171_disable,
	.init = udc_et171_init,
	.shutdown = udc_et171_shutdown,
	.lock = udc_et171_lock,
	.unlock = udc_et171_unlock,
};

DEVICE_DT_INST_DEFINE(0, udc_et171_driver_init, NULL,
		      &udc_et171_device_inst._data, &usbd_config,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &udc_et171_api);

