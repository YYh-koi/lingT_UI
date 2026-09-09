#ifndef __DRV_L2_H__
#define __DRV_L2_H__

#include "drv_l1.h"
#include "board_config.h"
#include "gplib.h"

#include "drv_l2_h264dec.h"
#include "drv_l2_h264enc.h"
#include "drv_l2_gsensor.h"
#include "drv_l2_jpeg_version.h"
#include "drv_l2_jpeg.h"
#include "drv_l2_jpeg_api.h"
#include "drv_l2_jpeg_decode.h"
#include "drv_l2_jpeg_encode.h"
#include "drv_l2_display.h"
#include "drv_l2_system_timer.h"

#include "drv_l2_usbd_version.h"
#include "drv_l2_usbd.h"
#include "drv_l2_usbd_msdc.h"
#include "drv_l2_usbd_msdc_vendor.h"
#include "drv_l2_usbd_uvc_uc.h"
#include "drv_l2_usbd_uac.h"
#include "drv_l2_usbd_cdc.h"

#include "drv_l2_usbh.h"
#include "drv_l2_usbh_uvc.h"

#include "drv_l2_sensor.h"
#include "drv_l2_cdsp_config.h"
#include "drv_l2_actuator.h"

#include "drv_l2_csi.h"

#include "drv_l2_sdc_version.h"
#include "drv_l2_sdc.h"

#include "drv_l2_3a.h"
#include "drv_l2_iram.h"
#include "drv_l2_rotator.h"

#include "drv_l2_scaler_version.h"
#include "drv_l2_scaler.h"

#include "drv_l2_sccb.h"
#include "drv_l2_spi_flash.h"

#if (WIFI_VENDOR == 2)
#include "drv_l2_sdio_rtk.h"
#endif

#include "drv_l2_hdmi.h"

#include "drv_l2_crypto.h"

void drv_l2_init(void);

#endif		// __DRV_L2_H__
