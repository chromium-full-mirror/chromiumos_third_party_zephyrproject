/*
 * usb_mem.h
 *
 *  Created on: Feb 8, 2021
 *      Author: kelvin.lin
 */

#ifndef BSP_USB_INCLUDE_USB_MEM_H_
#define BSP_USB_INCLUDE_USB_MEM_H_

// #include <soc.h>
#include <et171_usb/cdn_stdtypes.h>

void *USB_mem_alloc(size_t size);
void USB_mem_free(void* ptr);


#endif /* BSP_USB_INCLUDE_USB_MEM_H_ */
