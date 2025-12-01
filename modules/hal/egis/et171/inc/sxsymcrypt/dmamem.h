/** Memory fit for DMA
 *
 * @file
 * @copyright Copyright (c) 2019-2021 Silex Insight. All Rights reserved.
 */

#ifndef DMAMEM_HEADER_FILE
#define DMAMEM_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

/** Returns memory useable as input and output for DMA
 *
 * That memory is shared between all users and all hardware instances.
 *
 * @param sz size of the memory to be reserved
 * @return address of the reserved memory
 *
 * @remark - this function is platform specific and can be optional for some
 *           platforms (example: baremetal platform with no reserved DMA memory).
 */
char *sx_alloc_global_dmamem(size_t sz);

#ifdef __cplusplus
}
#endif

#endif
