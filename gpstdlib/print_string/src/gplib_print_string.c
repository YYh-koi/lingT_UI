#include "drv_l1.h"

#if GPLIB_PRINT_STRING_EN == 1

#if defined(DBG_PRINT_UART) && DBG_PRINT_UART == 0 && UART0_ENABLE == 1
	#define SEND_DATA(x)	drv_l1_uart0_data_send((x), 1)
	#define GET_DATA(x)	    drv_l1_uart0_data_get((x), 1)
#elif defined(DBG_PRINT_UART) && DBG_PRINT_UART == 1 && UART1_ENABLE == 1
	#define SEND_DATA(x)	drv_l1_uart1_data_send((x), 1)
	#define GET_DATA(x)	    drv_l1_uart1_data_get((x), 1)
#elif defined(DBG_PRINT_UART) && DBG_PRINT_UART == 2 && UART2_ENABLE == 1
	#define SEND_DATA(x)	drv_l1_uart2_data_send((x), 1)
	#define GET_DATA(x)	    drv_l1_uart2_data_get((x), 1)
#else
    #define SEND_DATA(x)
    #define GET_DATA(x)	(*(x) = '\r')
#endif

#if defined(USER_PRINT_UART) && USER_PRINT_UART == 0 && UART0_ENABLE == 1
	#define USER_SEND_DATA(x)	drv_l1_uart0_data_send((x), 1)
	#define USER_GET_DATA(x)	    drv_l1_uart0_data_get((x), 1)
#elif defined(USER_PRINT_UART) && USER_PRINT_UART == 1 && UART1_ENABLE == 1
	#define USER_SEND_DATA(x)	drv_l1_uart1_data_send((x), 1)
	#define USER_GET_DATA(x)	    drv_l1_uart1_data_get((x), 1)
#elif defined(USER_PRINT_UART) && USER_PRINT_UART == 2 && UART2_ENABLE == 1
	#define USER_SEND_DATA(x)	drv_l1_uart2_data_send((x), 1)
	#define USER_GET_DATA(x)	    drv_l1_uart2_data_get((x), 1)
#else
    #define USER_SEND_DATA(x)
    #define USER_GET_DATA(x)	(*(x) = '\r')
#endif



#define PRINT_OUTPUT_NONE	0x00
#define PRINT_OUTPUT_UART	0x01
#define PRINT_OUTPUT_USB	0x02

static INT32U print_output_type = PRINT_OUTPUT_UART;	/* 0 = NONE, 1 = UART, 2 = USB */

/* For USB logger used */
#if defined(C_RUN_ON_SPIFC) && (C_RUN_ON_SPIFC == 1)
#define USB_LOGGER_BUF_SIZE	512
#define USB_LOGGER_BUF_NUM	1
#define USB_LOGGER_BUF_TOTAL_SIZE	(USB_LOGGER_BUF_SIZE*USB_LOGGER_BUF_NUM)
#else
#define USB_LOGGER_BUF_SIZE	512
#define USB_LOGGER_BUF_NUM	10
#define USB_LOGGER_BUF_TOTAL_SIZE	(USB_LOGGER_BUF_SIZE*USB_LOGGER_BUF_NUM)
#endif

static CHAR usb_logger_buf[USB_LOGGER_BUF_TOTAL_SIZE] = {0};
CHAR* logger_read_ptr = usb_logger_buf;
CHAR* logger_write_ptr = usb_logger_buf;
//INT32U loggerwritecnt = 0;
//INT32U loggerreadcnt = 0;

#if defined(DBG_PRINT_UART)
static CHAR print_buf[PRINT_BUF_SIZE];
#endif

#if defined(USER_PRINT_UART)
static CHAR user_print_buf[PRINT_BUF_SIZE];
#endif

/*
* Function Name :  get_usb_logger_data
*
* Syntax : INT32U get_usb_logger_data(void)
*
* Purpose :  get usb log data from usb_logger_buf
*
* Parameters : <IN> none
*              <OUT> none
* Return : INT32U: logger_read_ptr
*
* Note :
*
*/
INT32U get_usb_logger_data(void)
{
	INT32U ret = 0;

	/* read operation */
#if _OPERATING_SYSTEM != _OS_NONE	// Soft Protect for critical section
	#if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedLock();
	#elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSuspendAll();
	#endif
#endif
	ret = (INT32U)logger_read_ptr;
	//memcpy((CHAR*)dataptr, usb_read_ptr, USB_LOGGER_BUF_SIZE);

	if(logger_read_ptr == logger_write_ptr)
	{
		logger_read_ptr[0] = 0;
	}
	else
	{
		if(logger_read_ptr < (usb_logger_buf + USB_LOGGER_BUF_TOTAL_SIZE - USB_LOGGER_BUF_SIZE))
	    	logger_read_ptr += USB_LOGGER_BUF_SIZE;
	    else
	    	logger_read_ptr = usb_logger_buf;
    }

	//loggerwritecnt = (logger_write_ptr - usb_logger_buf) >> 9;
    //loggerreadcnt = (logger_read_ptr - usb_logger_buf) >> 9;

#if _OPERATING_SYSTEM != _OS_NONE	// Soft Protect for critical section
	#if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedUnlock();
	#elif _OPERATING_SYSTEM == _OS_FREERTOS
		xTaskResumeAll();
	#endif
#endif

    return ret;
}
/*
* Function Name :  print_string
*
* Syntax : void print_string(CHAR *fmt, ...)
*
* Purpose :  print debug message via uart or usb
*
* Parameters : <IN> *fmt, ...:  character string
*              <OUT> none
* Return : none
*
* Note :
*
*/
void print_string(CHAR *fmt, ...)
{
    va_list v_list;
    INT32S ret;

	if(print_output_type & PRINT_OUTPUT_USB)
	{

	#if _OPERATING_SYSTEM != _OS_NONE	// Soft Protect for critical section
		#if _OPERATING_SYSTEM == _OS_UCOS2
			OSSchedLock();
		#elif _OPERATING_SYSTEM == _OS_FREERTOS
			vTaskSuspendAll();
		#endif
	#endif
		/* redirect print_string to usb_logger_buf, write operation */
		va_start(v_list, fmt);
		ret = vsnprintf(logger_write_ptr, USB_LOGGER_BUF_SIZE, fmt, v_list);
		if(ret < 0)
		{
			return;
		}
	    //vsprintf(usb_logger_buf, fmt, v_list);
	    va_end(v_list);


	    /* Update write pointer */
	    if(logger_write_ptr < (usb_logger_buf + USB_LOGGER_BUF_TOTAL_SIZE - USB_LOGGER_BUF_SIZE ))
	    	logger_write_ptr += USB_LOGGER_BUF_SIZE;
	    else
	    	logger_write_ptr = usb_logger_buf;

	    /* Check if write ptr is equal to read ptr, if yes update read ptr */
	    if(logger_write_ptr == logger_read_ptr)
	    {
	    	if(logger_read_ptr < (usb_logger_buf + USB_LOGGER_BUF_TOTAL_SIZE - USB_LOGGER_BUF_SIZE ))
	    		logger_read_ptr += USB_LOGGER_BUF_SIZE;
	    	else
	    		logger_read_ptr = usb_logger_buf;
		}

	    //loggerwritecnt = (logger_write_ptr - usb_logger_buf) >> 9;
	    //loggerreadcnt = (logger_read_ptr - usb_logger_buf) >> 9;
	#if _OPERATING_SYSTEM != _OS_NONE	// Soft Protect for critical section
		#if _OPERATING_SYSTEM == _OS_UCOS2
			OSSchedUnlock();
		#elif _OPERATING_SYSTEM == _OS_FREERTOS
			xTaskResumeAll();
		#endif
	#endif
	}

    #if defined(DBG_PRINT_UART)
	if(print_output_type & PRINT_OUTPUT_UART)
	{
		CHAR *pt;
	    va_start(v_list, fmt);
	    ret = vsnprintf(print_buf, PRINT_BUF_SIZE, fmt, v_list);
		if(ret < 0)
		{
			return;
		}
	    //vsprintf(print_buf, fmt, v_list);
	    va_end(v_list);

	    print_buf[PRINT_BUF_SIZE-1] = 0;
	    pt = print_buf;
	    while (*pt) {
			SEND_DATA(*pt);
			pt++;
		}
	}
    #endif
}

/*
* Function Name :  user_print_string
*
* Syntax : void user_print_string(CHAR *fmt, ...)
*
* Purpose :  print user message via uart
*
* Parameters : <IN> *fmt, ...:  character string
*              <OUT> none
* Return : none
*
* Note :
*
*/

void user_print_string(CHAR *fmt, ...)
{
    #if defined(USER_PRINT_UART)
    va_list v_list;
    INT32S ret;
    CHAR *pt;
    va_start(v_list, fmt);
    ret = vsnprintf(user_print_buf, PRINT_BUF_SIZE, fmt, v_list);
    if(ret < 0)
    {
        return;
    }
    //vsprintf(user_print_buf, fmt, v_list);
    va_end(v_list);

    user_print_buf[PRINT_BUF_SIZE-1] = 0;
    pt = user_print_buf;
    while (*pt) {
        USER_SEND_DATA(*pt);
        pt++;
    }
    #endif
}
/*
* Function Name :  get_string
*
* Syntax : void get_string(CHAR *s)
*
* Purpose :  get character string from uart
*
* Parameters : <IN>  *s:  character string
*              <OUT> none
* Return : none
*
* Note :
*
*/
void get_string(CHAR *s)
{
    INT8U temp;

    while (1) {
        GET_DATA(&temp);
        SEND_DATA(temp);
        if (temp == '\r') {
            *s = 0;
            return;
        }
        *s++ = (CHAR) temp;
    }
}

/*
* Function Name :  user_get_string
*
* Syntax : void user_get_string(CHAR *s)
*
* Purpose :  get character string from uart
*
* Parameters : <IN>  *s:  character string
*              <OUT> none
* Return : none
*
* Note :
*
*/

void user_get_string(CHAR *s)
{
    INT8U temp;

    while (1) {
        USER_GET_DATA(&temp);
        USER_SEND_DATA(temp);
        if (temp == '\r') {
            *s = 0;
            return;
        }
        *s++ = (CHAR) temp;
    }
}
/*
* Function Name :  get_string_with_size
*
* Syntax : void get_string_with_size(CHAR *s, INT32U size)
*
* Purpose :  get character string from uart
*
* Parameters : <IN>  *s:  character string
*              <IN>  size: avialable buffer size
*              <OUT> none
* Return : 0: success   1: out of buffer size
*
* Note :
*
*/
INT32S get_string_with_size(CHAR *s, INT32U size)
{
    INT8U temp;

    while (1) {
        GET_DATA(&temp);
        SEND_DATA(temp);
        if (temp == '\r') {
            *s = 0;
            return 0;
        }
        *s++ = (CHAR) temp;
        size -= 1;
        if(size <= 0 )
            return -1;
    }
}
/*
* Function Name :  user_get_string_with_size
*
* Syntax : void user_get_string_with_size(CHAR *s, INT32U size)
*
* Purpose :  get character string from uart
*
* Parameters : <IN>  *s:  character string
*              <IN>  size: avialable buffer size
*              <OUT> none
* Return : 0: success   1: out of buffer size
*
* Note :
*
*/
INT32S user_get_string_with_size(CHAR *s, INT32U size)
{
    INT8U temp;

    while (1) {
        USER_GET_DATA(&temp);
        USER_SEND_DATA(temp);
        if (temp == '\r') {
            *s = 0;
            return 0;
        }
        *s++ = (CHAR) temp;
        size -= 1;
        if(size <= 0 )
            return -1;
    }
}
/*
* Function Name :  set_print_output_type
*
* Syntax : void set_print_output_type(INT32U type)
*
* Purpose :  set the output type of debug message
*
* Parameters : <IN> type:  output type of debug message
*													#define PRINT_OUTPUT_NONE	0x00
*													#define PRINT_OUTPUT_UART	0x01
*													#define PRINT_OUTPUT_USB	0x02
*              <OUT> none
* Return : none
*
* Note :
*
*/
void set_print_output_type(INT32U type)
{
	print_output_type = type;
}

#else

void print_string(CHAR *fmt, ...)
{
}

void get_string(CHAR *s)
{
}

INT32U get_usb_logger_data(void)
{
}


void set_print_output_type(INT32U type)
{

}
#endif //GPLIB_PRINT_STRING_EN

