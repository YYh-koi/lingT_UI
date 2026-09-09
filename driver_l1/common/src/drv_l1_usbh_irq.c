#include "drv_l1_usbh.h"
#include "drv_l1_usbh_ohci.h"

extern volatile uint8_t USB_HostType;
extern void EHCI_IRQHandler(INT32U hostid);

void drv_l1_usbh_hci_irqhandler(INT32U hostid)
{
    if(USB_HostType == TYPE_EHCI)
    {
        EHCI_IRQHandler(hostid);
    }
    else if(USB_HostType == TYPE_OHCI)
    {
        OHCI_IRQHandler(hostid);
    }
}


void USBH_EHCIA_IRQHandler(void)
{

    USB_HostType = TYPE_EHCI;
    drv_l1_usbh_hci_irqhandler(0);
}

void USBH_EHCIB_IRQHandler(void)
{

    USB_HostType = TYPE_EHCI;
    drv_l1_usbh_hci_irqhandler(1);
}

void USBH_OHCIA_IRQHandler(void)
{
//    DBG_PRINT("=usb IRQ=\r\n");
//    if(R_INT_USBIRQMASK&0x03)//host
    {
//        if(R_INT_USBIRQMASK&0x02)
        {
//            DBG_PRINT("intr of usb Ehci!\r\n");
            USB_HostType = TYPE_OHCI;
        }
//        else if(R_INT_USBIRQMASK&0x01)
//        {
//            DBG_PRINT("intr of usb Ohci!\r\n");
//            USB_HostType = TYPE_OHCI;
//        }
        drv_l1_usbh_hci_irqhandler(0);
    }
}

void USBH_OHCIB_IRQHandler(void)
{
//    DBG_PRINT("=usb IRQ=\r\n");
//    if(R_INT_USBIRQMASK&0x03)//host
    {
//        if(R_INT_USBIRQMASK&0x02)
        {
//            DBG_PRINT("intr of usb Ehci!\r\n");
            USB_HostType = TYPE_OHCI;
        }
//        else if(R_INT_USBIRQMASK&0x01)
//        {
//            DBG_PRINT("intr of usb Ohci!\r\n");
//            USB_HostType = TYPE_OHCI;
//        }
        drv_l1_usbh_hci_irqhandler(1);
    }
}




