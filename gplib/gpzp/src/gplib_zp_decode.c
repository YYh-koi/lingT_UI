/*
* Description: Power Performance Version GPZIP
*
* Author: Dominantyang
*
* Date: 2009/11/27
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 2.00
*/
#include "gplib.h"

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#if (defined GPLIB_GPZP_EN) && (GPLIB_GPZP_EN == 1) //

//================================================================//
#define dumpbits(p, n)	(((p)->r) += (n))

typedef struct
{
	INT8U	*p;
	INT32U	r;	// gpzip total bit offset
} t_getbit;

static void		getbits_init(t_getbit *p, const void *ptr);
INT32S			gunzip(t_getbit *gb, INT8U *op);
static INT32U	getbits(t_getbit *p, INT32S n);
static INT32S	inflate_fixed(t_getbit *gb, INT8U *op);
INT32U			revn(INT32U u, INT32S n);
INT32U			showbit(t_getbit *p);
INT32U			revn_showbit(t_getbit *p, INT32S n);
INT32U			revn_bitget(t_getbit *p, INT32S n);
INT32U			revn_showbit(t_getbit *p, INT32S n);

//
// gunzip inflate-fixed lookup tables
//
// i = idx-257	(idx: 257~285 escape-code)
// length: 3 + tbl_lel3[] + getbits(gb,tbl_lee[])
// tbl_le3[]: length offset-3	(-3 to compress it into 8-bit table)
#if 0
const INT8U		tbl_lel3[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 255 };
#endif

// the sum of this value and the extra bits.
const INT16U	tbl_lel[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };

// tbl_lee[]: length extra bits
// The number of extra bits for code-257
const INT8U		tbl_lee[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };

// f = getbits(gb, 5)
// distance = tbl_dd[f] + getbits(gb, tbl_de[f])
// tbl_dd[]: distance offset
//distance_base
const INT16U	tbl_dd[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };

// tbl_de[]: distance extra-bits
// distance_extra
const INT8U		tbl_de[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

const INT16U	mask_bits[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff, 0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff };

t_getbit		gb;

INT32S gpzp_decode(INT8U *input_buffer, INT8U *output_buffer)
{
	getbits_init(&gb, input_buffer);
	return gunzip(&gb, output_buffer);
}

// getbits_init()
static void getbits_init(t_getbit *p, const void *ptr)
{
	p->p = (INT8U *) ptr;	// initialize input pointer
	p->r = 0;				// initialize input bit-pointer
}

// gunzip()
INT32S gunzip(t_getbit *gb, INT8U *op)
{
	INT32U	w;
	INT32U	u;
	INT32S	v;

	w = 0;
	do
	{
		u = getbits(gb, 3);
		if((u & 0x06) == 0x02)
		{
			v = inflate_fixed(gb, op + w);
			if(v < 0)
				return v;
			w += v;
		}
		else
		{
			return -1;
		}
	} while(!(u & 0x01));
	return w;
}

static INT32U getbits(t_getbit *p, INT32S n)
{
	INT32U	w;
	INT8U	*q;

	q = p->p + ((p->r) >> 3);
	w = (((q[0] | (q[1] << 8) | (q[2] << 16)) >> ((p->r) & 0x7)) & mask_bits[n]);
	dumpbits(p, n);
	return w;
}

static INT32S inflate_fixed(t_getbit *gb, INT8U *op)
{
	INT32S	w;
	INT32U	tt;
	w = 0;
	while(1)
	{
		INT32U	u, v;
		v = revn_showbit(gb, 9);
		if(v < 0x60)			//96  v<=0x5f
		{
			u = 256 + (v >> 2);
			dumpbits(gb, 7);
		}
		else
		if(v < 0x190)			//400
		{
			v >>= 1;
			if(v < 0xc0)
				u = v - 0x30;
			else
				u = v - 0xc0 + 280;
			dumpbits(gb, 8);
		}
		else
		{
			u = v - 0x190 + 144;
			dumpbits(gb, 9);
		}

		// LZ77 Start
		if(u == 256)
		{
			break;				// eob
		}
		else
		if(u < 256)
		{
			op[w++] = u;		// literal
		}
		else
		if(u <= 285)
		{
			INT32S	l, d, e;	// len/dist/extra
			u -= 257;
			e = tbl_lee[u];
			l = tbl_lel[u] + getbits(gb, e);	// extra
			u = revn_bitget(gb, 5);
			e = tbl_de[u];
			d = tbl_dd[u] + getbits(gb, e);		// extra
			tt = w - d;
			do
			{
				op[w] = op[tt++];
				w++;
			} while(--l > 0);
		}
		else
		{
			return -3;
		}
	}

	return w;
}

INT32U revn(INT32U u, INT32S n)
{
	INT32U	v = 0;
	for(; n > 0; n--)
	{
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);
	}

	return v;
}

INT8U	rev_table[16] = { 0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF };

INT32U revn_showbit(t_getbit *p, INT32S n)
{
	INT32U	v = 0;
	INT32U	u;
	INT8U	*q;

	//   INT8U i;
	//r = p->r;
	q = p->p + ((p->r) >> 3);					// q 為目前指標所指之byte offset位址, 所要讀取之 bit 數之所屬 int 之起始 byte

	//v = q[0] | (q[1]<<8) | (q[2]<<16); // | (q[3]<<24);
	u = (q[0] | (q[1] << 8)) >> ((p->r) & 0x7); // 保證所要的第一個 bit 一定可以被 show bit 呈現出來 , r&7 為 r mod 7 之意思, 為餘數 bit
#if 0
	for(; n > 0; n--)
	{
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);
	}

#else
	switch(n)
	{
	case 13:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 12:
		v = (v << 4) | rev_table[u & 0xF];
		u = (u >> 4);
		v = (v << 4) | rev_table[u & 0xF];
		u = (u >> 4);
		v = (v << 4) | rev_table[u & 0xF];
		break;

	case 11:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 10:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 9:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 8:
		v = (v << 4) | rev_table[u & 0xF];
		u = (u >> 4);
		v = (v << 4) | rev_table[u & 0xF];
		break;

	case 7:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 6:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 5:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 4:
		v = (v << 4) | rev_table[u & 0xF];
		break;

	case 3:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 2:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 1:
		v = (v << 1) | (u & 0x01);
		break;

	case 0:
	default:
		v = 0;
		break;
	}

#endif
	return v;
}

INT32U revn_bitget(t_getbit *p, INT32S n)
{
	INT32U	v = 0;
	INT32U	u;
	INT8U	*q;

	//    INT8U i;
	q = p->p + ((p->r) >> 3);					// q 為目前指標所指之byte offset位址, 所要讀取之 bit 數之所屬 int 之起始 byte

	//v = q[0] | (q[1]<<8) | (q[2]<<16); // | (q[3]<<24);
	//w = ((v>>((p->r)&0x7))&mask_bits[n]);
	u = (((q[0] | (q[1] << 8)) >> ((p->r) & 0x7)) & mask_bits[n]);
	dumpbits(p, n);				//bit offset 增加
#if 0
	for(; n > 0; n--)
	{
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);
	}

#else
	switch(n)
	{
	case 13:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 12:
		v = (v << 4) | rev_table[u & 0xF];
		u = (u >> 4);
		v = (v << 4) | rev_table[u & 0xF];
		u = (u >> 4);
		v = (v << 4) | rev_table[u & 0xF];
		break;

	case 11:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 10:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 9:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 8:
		v = (v << 4) | rev_table[u & 0xF];
		u = (u >> 4);
		v = (v << 4) | rev_table[u & 0xF];
		break;

	case 7:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 6:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 5:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 4:
		v = (v << 4) | rev_table[u & 0xF];
		break;

	case 3:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 2:
		v = (v << 1) | (u & 0x01);
		u = (u >> 1);

	case 1:
		v = (v << 1) | (u & 0x01);
		break;

	case 0:
	default:
		v = 0;
		break;
	}

#endif
	return v;
}

INT32U showbit(t_getbit *p)
{
	INT8U	*q;

	//INT32U v;
	//INT32U r;
	//r = p->r;
	q = p->p + ((p->r) >> 3);	// q 為目前指標所指之byte offset位址, 所要讀取之 bit 數之所屬 int 之起始 byte

	//v = q[0] | (q[1]<<8) | (q[2]<<16); // | (q[3]<<24);
	return(q[0] | (q[1] << 8)) >> ((p->r) & 0x7);	// 保證所要的第一個 bit 一定可以被 show bit 呈現出來 , r&7 為 r mod 7 之意思, 為餘數 bit
}

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#endif //(defined GPLIB_GPZP_EN) && (GPLIB_GPZP_EN == 1)          //

//================================================================//
