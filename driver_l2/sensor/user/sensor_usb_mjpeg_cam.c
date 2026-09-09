/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2022 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "drv_l2.h"

#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#if (_SENSOR_USB_MJPEG_CAM_FRONT)
static drv_l2_sensor_info_t* usb_mjpeg_cam_get_info_front(INT32U index);

const drv_l2_sensor_ops_t usb_mjpeg_cam_ops_front =
{
	SOURCE_USB_CAM,	                /* sensor name */
	NULL,
	NULL,
	NULL,
	NULL,
	usb_mjpeg_cam_get_info_front,
	NULL,
	NULL,
    NULL,
    NULL,
	{
		#if (HOST_USB_TYPE_FRONT== HOST_USB_OHCI)
		/* 1st info */
		{
			MCLK_NONE,					/* MCLK */
			V4L2_PIX_FMT_YUYV,			/* input format */
			V4L2_PIX_FMT_YUYV,			/* output format */
			25,							/* FPS in sensor */
			USB_OHCI_OUT_WIDTH_FRONT,	/* target width */
			USB_OHCI_OUT_HEIGHT_FRONT,	/* target height */
			USB_OHCI_OUT_WIDTH_FRONT,	/* sensor width */
			USB_OHCI_OUT_HEIGHT_FRONT,	/* sensor height */
			0,							/* sensor h offset */
			0,                          /* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
		#endif
		#if (HOST_USB_TYPE_FRONT== HOST_USB_EHCI)
		{
			MCLK_NONE,					/* MCLK */
			V4L2_PIX_FMT_YUYV,			/* input format */
			V4L2_PIX_FMT_YUYV,			/* output format */
			25,							/* FPS in sensor */
			USB_EHCI_OUT_WIDTH_FRONT,	/* target width */
			USB_EHCI_OUT_HEIGHT_FRONT,	/* target height */
			USB_EHCI_OUT_WIDTH_FRONT,	/* sensor width */
			USB_EHCI_OUT_HEIGHT_FRONT,	/* sensor height */
			0,							/* sensor h offset */
			0,                          /* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
		#endif
	}
};

static drv_l2_sensor_info_t* usb_mjpeg_cam_get_info_front(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1))
	{
		return NULL;
	}
	else
	{
		return (drv_l2_sensor_info_t*)&usb_mjpeg_cam_ops_front.info[index];
	}
}
#endif

#if (_SENSOR_USB_MJPEG_CAM_REAR)
static drv_l2_sensor_info_t* usb_mjpeg_cam_get_info_rear(INT32U index);

const drv_l2_sensor_ops_t usb_mjpeg_cam_ops_rear =
{
	SOURCE_USB_CAM,	                /* sensor name */
	NULL,
	NULL,
	NULL,
	NULL,
	usb_mjpeg_cam_get_info_rear,
	NULL,
	NULL,
    NULL,
    NULL,
	{
		#if (HOST_USB_TYPE_REAR == HOST_USB_OHCI)
		/* 1st info */
		{
			MCLK_NONE,					/* MCLK */
			V4L2_PIX_FMT_YUYV,			/* input format */
			V4L2_PIX_FMT_YUYV,			/* output format */
			25,							/* FPS in sensor */
			USB_OHCI_OUT_WIDTH_REAR,	/* target width */
			USB_OHCI_OUT_HEIGHT_REAR,	/* target height */
			USB_OHCI_OUT_WIDTH_REAR,	/* sensor width */
			USB_OHCI_OUT_HEIGHT_REAR,	/* sensor height */
			0,							/* sensor h offset */
			0,                          /* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
		#endif
		#if (HOST_USB_TYPE_REAR == HOST_USB_EHCI)
		{
			MCLK_NONE,					/* MCLK */
			V4L2_PIX_FMT_YUYV,			/* input format */
			V4L2_PIX_FMT_YUYV,			/* output format */
			25,							/* FPS in sensor */
			USB_EHCI_OUT_WIDTH_REAR,	/* target width */
			USB_EHCI_OUT_HEIGHT_REAR,	/* target height */
			USB_EHCI_OUT_WIDTH_REAR,	/* sensor width */
			USB_EHCI_OUT_HEIGHT_REAR,	/* sensor height */
			0,							/* sensor h offset */
			0,                          /* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
		#endif
	}
};

static drv_l2_sensor_info_t* usb_mjpeg_cam_get_info_rear(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1))
	{
		return NULL;
	}
	else
	{
		return (drv_l2_sensor_info_t*)&usb_mjpeg_cam_ops_rear.info[index];
	}
}
#endif

#endif //(defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
