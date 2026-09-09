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
#include "gp_stdlib.h"

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
INT32S gp_strcmp(INT8S *s,INT8S *t)
{
    return strcmp((const char *)s, (const char *)t);
}

INT8S *gp_strcpy(INT8S *t,INT8S *s)
{
    return (INT8S *)strcpy((char *)t, (char *)s);
}

INT32S gp_strlen(INT8S *s)
{
    return strlen((const char *)s);
}

INT32S gp_strncmp(INT8S *dest,INT8S *src,INT32U Len)
{
    return strncmp((const char *)dest, (const char *)src, Len);
}

INT8S gp_toupper(INT8S c)
{
    return toupper(c);
}

INT8S *gp_strcat(INT8S *s,INT8S *t)
{
    return (INT8S *)strcat((char *)s, (const char *)t);
}

INT32S gp_memcpy(INT8S *dest, INT8S *src, INT32U Len)
{
    return (INT32S)memcpy(dest, src, Len);
}

INT32S gp_memmove(INT8S *dest, INT8S *src, INT32U Len)
{
	return (INT32S)memmove(dest, src, Len);
}

INT32S gp_memset(INT8S *dest,INT8U byte, INT32U Len)
{
    return (INT32S)memset(dest, byte, Len);
}

INT32S gp_memcmp(INT8S* dest, INT8S* src, INT32U Len)
{
    return memcmp(dest, src, Len);
}

INT32S gp_memset_32(INT32S *dest, INT32U byte, INT32U Len)
{
	INT32S	i = 0;
    Len = Len>>2;
	for(i = 0; i < Len; i++)
	{
		dest[i] = byte;
	}

	return(Len);
}
