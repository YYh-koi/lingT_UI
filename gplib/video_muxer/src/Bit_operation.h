/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2004 by Sunplus Technology Co., Ltd.             *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  Technology Co., Ltd. All rights are reserved by Sunplus Technology    *
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus Technology Co., Ltd. reserves the right to modify this        *
 *  software without notice.                                              *
 *                                                                        *
 *  Sunplus Technology Co., Ltd.                                          *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 *                                                                        *
 **************************************************************************/
#ifndef __BIT_OPERATION_H__
#define __BIT_OPERATION_H__

#include "typedef.h"

/******************************************************************
                         C O N S T A N T S
******************************************************************/

#define NEG_SSR32(a,s) (((INT32S)(a))>>(32-(s)))
#define NEG_USR32(a,s) (((INT32U)(a))>>(32-(s)))

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

#define AV_RB16(x)  ((((INT8U*)(x))[0] << 8) | ((INT8U*)(x))[1])
#define AV_RB32(x)  ((((INT8U*)(x))[0] << 24) | \
	                 (((INT8U*)(x))[1] << 16) | \
	                 (((INT8U*)(x))[2] <<  8) | \
                      ((INT8U*)(x))[3])

#define OPEN_READER(name, gb)\
	    INT32S name##_index= (gb)->index;\
        INT32S name##_cache= 0;\

#define UPDATE_CACHE(name, gb)\
        name##_cache= AV_RB32( ((const INT8U *)(gb)->buffer)+(name##_index>>3) ) << (name##_index&0x07);\

#define SHOW_UBITS(name, gb, num)     NEG_USR32(name##_cache, num)

#define SKIP_COUNTER(name, gb, num)	  name##_index += (num);

#define LAST_SKIP_BITS(name, gb, num) SKIP_COUNTER(name, gb, num)

#define CLOSE_READER(name, gb)        (gb)->index= name##_index;
/******************************************************************
              S T R U C T U R E   D E C L A R A T I O N
******************************************************************/

/* bit input */
/* buffer, buffer_end and size_in_bits must be present and used by every reader */
typedef struct GetBitContext_s
{
	const INT8U *buffer;
	const INT8U *buffer_end;
	INT32S index;
	INT32S size_in_bits;
} GetBitContext_t;
/******************************************************************
           V A R I A B L E   D E C L A R A T I O N S
******************************************************************/



/******************************************************************
        F U N C T I O N    D E C L A R A T I O N S
******************************************************************/

INT16U read12(void *, INT32U);
INT16U read16(void *);
INT32U LREAD32(void *);
INT32U BREAD32(void *);
INT64U read64(void *);
void   write12(void *, INT16U, INT32U);
void   write16(void *, INT16U);
void   write32(void *, INT32U);
void   write64(void *, INT64U);

void   lwrite(void *, INT32U, INT32U);
INT32U lread(void *, INT32U);

INT32U parser_get_bits(GetBitContext_t *s, INT32S n);
INT32U parser_get_bits1(GetBitContext_t *s);
void   parser_skip_bits(GetBitContext_t *s, int n);
void   init_get_bits(GetBitContext_t *s,
						  const INT8U *buffer, INT32S bit_size);
INT32S  get_bits_count(GetBitContext_t *s);
void   align_get_bits(GetBitContext_t *s);

#endif

