/* FreeRTOS includes. */
#ifndef OS_API_H
#define OS_API_H

#ifdef ATBM_GENERALPLUS_SDIO
#undef ATBM_GENERALPLUS_SDIO
#endif
#define ATBM_GENERALPLUS_SDIO 1

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#if !ATBM_GENERALPLUS_SDIO
#include "semaphore.h"
#endif
#include "event_groups.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if ATBM_GENERALPLUS_SDIO
#include "gplib_print_string.h"
#else
#include <sdio.h>
#include "sys.h"
#endif
#include <timers.h>
#include <task.h>
//#include <kwrap/flag.h>


#if ATBM_GENERALPLUS_SDIO
#define iot_printf print_string
#else
int printf(const char *format, ...);

#define iot_printf printf
#endif
#ifndef ZEROSIZE
#define ZEROSIZE 1
#endif

#define BITS_PER_LONG 32
#define BIT_MASK(nr)		(1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)
#define max(a, b) ((a)>(b) ? (a) : (b))
#define min(a, b) ((a)<(b) ? (a) : (b))
#define ffz(x)  __ffs(~(x))

#define  rcu_read_lock()
#define  rcu_read_unlock()

#endif
