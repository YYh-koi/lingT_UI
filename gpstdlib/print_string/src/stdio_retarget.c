/*standard I/O hook functions*/
#include "drv_l1.h"
#include <sys/errno.h>

#if DBG_PRINT_UART == 0
	#define SEND_DATA(x)	drv_l1_uart0_data_send((x), 1)
	#define GET_DATA(x)	    drv_l1_uart0_data_get((x), 1)
#elif DBG_PRINT_UART == 1
	#define SEND_DATA(x)	drv_l1_uart1_data_send((x), 1)
	#define GET_DATA(x)	    drv_l1_uart1_data_get((x), 1)
#elif DBG_PRINT_UART == 2
	#define SEND_DATA(x)	drv_l1_uart2_data_send((x), 1)
	#define GET_DATA(x)	    drv_l1_uart2_data_get((x), 1)
#endif

ALIGN32 INT8U __attribute__((section(".uninit"))) end[16384];

INT8U io_putchar(INT8U ch)
{
    SEND_DATA(ch);
    return ch;
}

INT8U io_getchar(void)
{
    INT8U ch;
	GET_DATA(&ch);
	return ch;
}


long
_write_r(	struct _reent *r,
			int fd,
			const void *buf,
			size_t cnt )
{
	const unsigned char *p = (const unsigned char*) buf;
	int i;

	for (i = 0; i < cnt; i++)
	{
		if (*p == '\n' )
			io_putchar('\r');
		io_putchar(*p++);
	}
	return cnt;
}


long
_read_r(	struct _reent *r,
			int fd,
			char *buf,
			size_t cnt )
{
	unsigned char *p = (unsigned char*) buf;
	char c;
	int  i;

	for (i = 0; i < cnt; i++)
	{
		c = io_getchar();

		*p++ = c;
		io_putchar(c);

		if (c == '\r' && i <= (cnt - 2))
		{
			*p = '\n';
			io_putchar(*p);
			return i + 2;
		}
	}
	return i;
}

void _ttywrch(int c)
{
  /* Write one char "ch" to the default console
   * Need implementing with UART here. */
   io_putchar(c);
}
