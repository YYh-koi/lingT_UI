/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2019 by Generalplus Inc.                         *
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
#ifndef __UVC_FUNC_API_H__
#define __UVC_FUNC_API_H__
//#include "usbh_uvc.h"
#define SIGN_BIT                            0x80000000
#define BRIGHTNESS_STEP                     4
#define CONTRAST_STEP                       1
#define COMPATIABLE_SLOW_RESPONSE_CAM       1
#define DEBUG_PU_CTRL_FUNC
#ifdef DEBUG_PU_CTRL_FUNC
    #define PU_CTRL_FN_DBG                  DBG_PRINT
#else
    #define PU_CTRL_FN_DBG(...)
#endif


/* AUTO EXPOSURE MODE*/
/******************************************************
* @brief   get auto exposure mode value
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_auto_exposure_mode(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   enable auto exposure mode
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_enable_auto_exposure_mode(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   disable auto exposure mode
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_disable_auto_exposure_mode(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/* EXPOSURE TIME */
/******************************************************
* @brief   get current exposure time value and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_exposure_time(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   set current exposure time value and update to cur attribure
* @param   hostid: host id
*          *cam_func_hdl: point of the camera function handle
*          set_value: CUR value to set
* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_exposure_time(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl, INT32S value);

/* Brightness */
/******************************************************
* @brief   get current brightness value and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_brightness(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   set current brightness value and update to cur attribure
* @param   hostid: host id
*          *cam_func_hdl: point of the camera function handle
*          set_value: CUR value to set
* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_brightness(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl, INT32S value);

/* Contrast */
/******************************************************
* @brief   get Contrast auto value
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
INT32U uvch_cam_get_cur_contrast_auto(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   enable contrast auto
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
INT32U uvch_cam_enable_contrast_auto(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   disable contrast auto
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
INT32U uvch_cam_disable_contrast_auto(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   get current contrast value and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_contrast(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   set current contrast value and update to cur attribure
* @param   hostid: host id
*          *cam_func_hdl: point of the camera function handle
*          set_value: CUR value to set
* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_contrast(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl, INT32S value);

/*Hue*/
/******************************************************
* @brief   get Hue auto value
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
INT32U uvch_cam_get_cur_hue_auto(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   enable Hue auto
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
INT32U uvch_cam_enable_hue_auto(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   disable hue auto
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
INT32U uvch_cam_disable_hue_auto(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   get current hue value and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_hue(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   set current hue value and update to cur attribure
* @param   hostid: host id
*          *cam_func_hdl: point of the camera function handle
*          set_value: CUR value to set
* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_hue(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl, INT32S value);

/*Saturation*/
/******************************************************
* @brief   get current saturation value and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_saturation(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   set current saturation value and update to cur attribure
* @param   hostid: host id
*          *cam_func_hdl: point of the camera function handle
*          set_value: CUR value to set
* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_saturation(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl, INT32S value);

/* Sharpness */
/******************************************************
* @brief   get current sharpness value and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_sharpness(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   set current sharpness value and update to cur attribure
* @param   hostid: host id
*          *cam_func_hdl: point of the camera function handle
*          set_value: CUR value to set
* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_sharpness(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl, INT32S value);

/*Gamma*/
/******************************************************
* @brief   get current gamma value and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_gamma(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   set current gamma value and update to cur attribure
* @param   hostid: host id
*          *cam_func_hdl: point of the camera function handle
*          set_value: CUR value to set
* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_gamma(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl, INT32S value);

/*WB Temp auto*/
/******************************************************
* @brief   get WB Temp auto value
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_wb_temp_auto(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   enable WB Temp auto
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_enable_wb_temp_auto(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   disable WB Temp auto
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_disable_wb_temp_auto(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/*WB Temp*/
/******************************************************
* @brief   get current white balance temperature value and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_wb_temp(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   set current white balance temperature value and update to cur attribure
* @param   hostid: host id
*          *cam_func_hdl: point of the camera function handle
*          set_value: CUR value to set
* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_wb_temp(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl, INT32S value);

/*Bklight compensation*/
/******************************************************
* @brief   get current backlight compensation value and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_bklight_comp(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   set current backlight compensation value and update to cur attribure
* @param   hostid: host id
*          *cam_func_hdl: point of the camera function handle
*          set_value: CUR value to set
* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_bklight_comp(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl, INT32S value);

/*Gain*/
/******************************************************
* @brief   get current gain and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_gain(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);

/******************************************************
* @brief   set current gain and update to cur attribure
* @param   hostid: host id
*          *cam_func_hdl: point of the camera function handle
*          set_value: CUR value to set
* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_gain(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl, INT32S value);


/*Powerline frequency*/
/******************************************************
* @brief   get current Powerline frequency value and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_get_cur_pwrline_freq(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);
/******************************************************
* @brief   set Powerline frequency value and update to cur attribure
* @param   hostid: host id
*          *cam_func_hdl: point of the camera function handle
*          set_value: CUR value to set
* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_pwrline_freq(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl, INT32S value);

/******************************************************
* @brief   set all function cur value as default and update to cur attribure
* @param   hostid: host id
           *cam_func_hdl: point of the camera function handle

* @return 	0: sucess;
            others: refer to UVCH_ErrorCodes_t
                  & USB_Host_EnumerationErrorCodes_t
*******************************************************/
extern INT32U uvch_cam_set_attr_default_all(INT32U hostid, CAMERA_FUNC_t *cam_func_hdl);
extern void uvch_func_setup(INT32U hostid, ST_UVC_INFO *uvc);

#endif
