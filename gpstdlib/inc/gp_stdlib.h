/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2022 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#ifndef __GP_STD_LIB_H__
#define __GP_STD_LIB_H__

#include "project.h"

// Memory management
#include "gplib_mm_gplus.h"

#include "gplib_print_string.h"


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern INT32S gp_strcmp(INT8S *s,INT8S *t);
extern INT8S *gp_strcpy(INT8S *t,INT8S *s);
extern INT32S gp_strlen(INT8S *s);
extern INT32S gp_strncmp(INT8S *dest,INT8S *src,INT32U Len);
extern INT8S gp_toupper(INT8S c);
extern INT8S *gp_strcat(INT8S *s,INT8S *t);
extern INT32S gp_memcpy(INT8S *dest, INT8S *src, INT32U Len);
extern INT32S gp_memmove(INT8S *dest, INT8S *src, INT32U Len);
extern INT32S gp_memset(INT8S *dest,INT8U byte, INT32U Len);
extern INT32S gp_memcmp(INT8S* dest, INT8S* src, INT32U Len);
extern INT32S gp_memset_32(INT32S *dest, INT32U byte, INT32U Len);

#endif
