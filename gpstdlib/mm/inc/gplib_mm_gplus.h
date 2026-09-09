#ifndef __GPLIB_MM_GPLUS_H__
#define __GPLIB_MM_GPLUS_H__

#include "Typedef.h"

//===========================================================================================//
// Memory management module
#define GPLIB_MEMORY_MANAGEMENT_EN      1
#define C_MM_IRAM_LINK_NUM              20
#define C_MM_16BYTE_NUM                 32
#define C_MM_64BYTE_NUM                 16
#define C_MM_256BYTE_NUM                12
#define C_MM_1024BYTE_NUM               18
#define C_MM_4096BYTE_NUM               30

#if (GPLIB_MEMORY_MANAGEMENT_EN == 1)

#include "Typedef.h"

extern void gp_mm_init(INT32U free_memory_start, INT32U free_memory_end);
extern void * gp_malloc_align(INT32U size, INT32U align);		// SDRAM allocation. Align value must be power of 2, eg: 2, 4, 8, 16, 32...
extern void * gp_malloc(INT32U size);							// SDRAM allocation
extern void * gp_iram_malloc_align(INT32U size, INT32U align);	// IRAM allocation. Align value must be power of 2, eg: 2, 4, 8, 16, 32...
extern void * gp_iram_malloc(INT32U size);						// IRAM allocation
extern void gp_free(void *ptr);									// Both SDRAM and IRAM can be freed by this function
extern void * gp_realloc(void *ptr, INT32U size);

#endif

#endif 		// __GPLIB_MM_GPLUS_H__
