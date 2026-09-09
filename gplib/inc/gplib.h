#ifndef __GPLIB_H__
#define __GPLIB_H__

#include "project.h"
#include "gplib_cfg.h"
#include "gp_stdlib.h"

//Time and data reminder
typedef struct
{
	INT32S	tm_sec;		/* 0-59 */
	INT32S	tm_min;		/* 0-59 */
	INT32S	tm_hour;	/* 0-23 */
	INT32S	tm_mday;	/* 1-31 */
	INT32S	tm_mon;		/* 1-12 */
	INT32S	tm_year;
	INT32S	tm_wday;	/* 0-6 Sunday,Monday,Tuesday,Wednesday,Thursday,Friday,Saturday */
} TIME_T;

// Component layer functions
extern void gplib_init(INT32U free_memory_start, INT32U free_memory_end);

/* WAVE */
#include "gplib_wav.h"

#include "storages.h"

#include "gplib_fs.h"

#include "gplib_calendar.h"

#include "gplib_zp_decode.h"

#endif // __GPLIB_H__
