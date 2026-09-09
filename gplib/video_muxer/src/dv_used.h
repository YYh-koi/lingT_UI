
/***************************************************************************
 * Name: dv_used.h
 *
 * Purpose:
 *
 * Developer:
 *     zhoulu, 2010-12-13
 *
 * Copyright (c) 2010-2011 by Sunplus mMobile Inc.
 ***************************************************************************/

#ifndef __DV_USED_H__
#define __DV_USED_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************
 * Header Files
 ***************************************************************************/
#include <typedef.h>
//#include <mem.h>
//#include <libmem.h>
//#include <chunkmem.h>

/***************************************************************************
 * Constants
 ***************************************************************************/

/***************************************************************************
 * Macros
 ***************************************************************************/

//#define USE_SYSTEM_FS

/* define for malloc */
//#define osMalloc(x)  gp_malloc(x) /*!< Macro for doing dynamic memory manageemnt. */
//#define osFree(x)  gp_free((void *)(x)) /*!< Macro for freeing memory. */



/* define for auido codec debuf */
#ifdef MCP_DEBUG
//#include "diag.h"
#define diag_printf(args...) print_string(args)
#else
#define diag_printf(args...)
#endif
/***************************************************************************
 * Data Types
 ***************************************************************************/

#ifndef S32
#define S64 SINT64
#define S32 INT32S
#define S16 INT16S
#define S08 SINT8
#define U64 INT64U
#define U32 INT32U
#define U16 INT16U
#define U08 INT8U
#endif
/***************************************************************************
 * Function Declarations
 ***************************************************************************/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DV_USED_H__ */




