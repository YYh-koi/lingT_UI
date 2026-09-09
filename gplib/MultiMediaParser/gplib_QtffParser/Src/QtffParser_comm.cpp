#include "QtffParser.h"
#include "QtffParser_comm.h"

#ifdef ENABLE_DEBUG_INFO
const char *indent(int i)
{
	const char *ind = "                                ";
	return ind + 32 - (i<<1);
}

static char buf[5] = {0,0,0,0,0};
const char *fourcc_to_str(long fourcc)
{
	*(long*)buf = fourcc;
	return buf;
}
#endif

void QtffParser_Struct_ByteReverse(void *_ptr, int cnt, const char *fmt)
{
	int a;
	unsigned long *ptr = (unsigned long *)_ptr;
	/*register*/ unsigned long t, prev_t = 0;
	/*register*/ const unsigned long mask8H = 0xFF00FF00;
	/*register*/ const unsigned long mask8L = 0x00FF00FF;

	a = *fmt++;
	while(cnt>0 && a)
	{
		a -=  '0';
		if(a!=1)
		{
			t = *ptr;
			if(a==2)
			{
				t = (mask8L & (t >> 8))  | (mask8H & (t << 8));
			}
			else if(a==4)
			{
				t = (mask8L & (t >> 8))  | (mask8H & (t << 8));
				t = (t >> 16) | (t << 16);
			}
			else if (a==8)
			{
				t = (mask8L & (t >> 8))  | (mask8H & (t << 8));
				t = (t >> 16) | (t << 16);
				prev_t = t;
				ptr++;
				a = (a+1) + '0';
				cnt--;
				continue;
			}
			else if (a==9)
			{
				t = (mask8L & (t >> 8))  | (mask8H & (t << 8));
				t = (t >> 16) | (t << 16);
				*(ptr-1) = t;
				t = prev_t;
			}
			else
			{
				while(1);
			}
			*ptr = t;
		}
		ptr++;
		a = *fmt++;
		cnt--;
	}
}

void QtffParser_ByteSwap(void *_ptr, int cnt)
{
	int i;
	unsigned long *ptr = (unsigned long *)_ptr;
	/*register*/ unsigned long t;
	/*register*/ const unsigned long mask8H = 0xFF00FF00;
	/*register*/ const unsigned long mask8L = 0x00FF00FF;

	cnt >>= 2;
	for(i=0;i<cnt;i++)
	{
		t = *ptr;
		t = (mask8L & (t >> 8))  | (mask8H & (t << 8));
		t = (t >> 16) | (t << 16);
		*ptr++ = t;
	}
}