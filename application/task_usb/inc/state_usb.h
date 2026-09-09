#ifndef __STATE_USBD_H__
#define __STATE_USBD_H__

extern void usb_phy_lock(void);
extern void usb_phy_unlock(void);

extern void usbd_state_task_init(void);
extern void state_usb_device_entry(void const *arg);
extern void state_usbh_uvc_entry(void const *pvParameters);

extern INT32S state_usbd_task_msg_send(INT32U msg, void *para, INT32U bytes, INT32U prio);

#endif /*__STATE_CALENDAR_H__*/
