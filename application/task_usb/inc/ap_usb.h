#ifndef __AP_USBD_H__
#define __AP_USBD_H__

typedef INT32S (*UVC_SEND_BUF_DONE)(INT32U src, INT32U data, INT32U frameType);

typedef enum
{
	/* 0 for system DMA done */
	USBD_START_VIDEO_EVENT		= 1,
	USBD_STOP_VIDEO_EVENT,
	USBD_SEND_VIDEO_EVENT,
	USBD_SEND_VIDEO_DONE_EVENT,
	USBD_START_AUDIO_EVENT,
	USBD_STOP_AUDIO_EVENT,
	USBD_SEND_AUDIO_EVENT,
	USBD_SEND_AUDIO_DONE_EVENT,
	USBD_SET_INTERFACE_EVENT,
	USBD_UAC_TIMER_DONE,
	USBD_NO_EVENT
} UVC_TASK_EVENT_E;

extern INT32U   usb_device_is_connected_to_pc(void);
extern INT32S	usb_mode_detect(void);

/*****  MSDC *****/
extern INT32S	usb_msdc_init(void);
extern void		usb_msdc_exit(void);

/*****  UVC *****/
extern INT32S	usb_uvc_init(void);
extern void		usb_uvc_exit(void);
extern INT32S	usb_jpeg_send(INT32U buf, INT32U size, INT32U ret_buf);

extern INT32S	usb_webcam_start(void);
extern INT32S	usb_webcam_stop(void);
extern INT32U   usbd_uvc_display_width_get(void);
extern INT32U   usbd_uvc_display_height_get(void);

extern void state_usbd_status_set(INT8U status);
extern INT8U state_usbd_status_get(void);

extern INT32U uvc_encode_type_get(void);
extern INT32U uvc_display_buf_size_get(void);
extern INT32U uvc_display_format_get(void);
extern INT32U usbd_uvc_display_height_get(void);
extern INT32U usbd_uvc_display_width_get(void);
extern INT8U state_usbd_status_get(void);
extern void state_usbd_status_set(INT8U status);

/*****  UAC *****/
extern INT32S usb_audio_send(INT32U buf,INT32U size,INT32U ret_buf);
 
#endif // __AP_USBD_H__
