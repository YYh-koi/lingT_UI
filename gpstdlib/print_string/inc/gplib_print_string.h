#ifndef __GPLIB_PRINT_STRING_H__
#define __GPLIB_PRINT_STRING_H__

#define GPLIB_PRINT_STRING_EN           1

#include "Typedef.h"

#define PRINT_BUF_SIZE                  1024

#if GPLIB_PRINT_STRING_EN == 1
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
extern void print_string(CHAR *fmt, ...);
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
extern void user_print_string(CHAR *fmt, ...);
#endif
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
extern void get_string(CHAR *s);

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
extern void user_get_string(CHAR *s);
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

extern INT32S user_get_string_with_size(CHAR *s, INT32U size);
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

extern INT32S user_get_string_with_size(CHAR *s, INT32U size);

#endif 		// __GPLIB_PRINT_STRING_H__
