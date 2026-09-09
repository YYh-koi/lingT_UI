#include "project.h"

extern void USB20_IRQHandler(INT32U usb_id);

void USB_A_IRQHandler(void)
{
    USB20_IRQHandler(0);    /*0: USB_A*/
}

void USB_B_IRQHandler(void)
{
    USB20_IRQHandler(1);    /*1: USB_B*/
}
