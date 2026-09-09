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
#ifndef __USBD_UVC_H__
#define __USBD_UVC_H__

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define UVC_H264            0x01
#define UVC_MJPEG           0x02
#define UVC_UNCOMPRESSED    0x04
#define UVC_FORMAT          UVC_MJPEG

#define UVC_FR			    25		// this value should be: 15/20/25/30

#define SPR				    16000
#define FPS				    UVC_FR

#if UVC_FR == 30
#define UVC_FRM_INTVL       UVC_FRM_INTVL_30FPS
#elif UVC_FR == 25
#define UVC_FRM_INTVL       UVC_FRM_INTVL_25FPS
#elif UVC_FR == 20
#define UVC_FRM_INTVL       UVC_FRM_INTVL_20FPS
#elif UVC_FR == 15
#define UVC_FRM_INTVL       UVC_FRM_INTVL_15FPS
#elif UVC_FR == 10
#define UVC_FRM_INTVL       UVC_FRM_INTVL_10FPS
#elif UVC_FR == 5
#define UVC_FRM_INTVL       UVC_FRM_INTVL_5FPS
#elif UVC_FR == 2
#define UVC_FRM_INTVL       UVC_FRM_INTVL_2FPS
#elif UVC_FR == 60
#define UVC_FRM_INTVL       UVC_FRM_INTVL_60FPS
#endif

/*Target Resolution*/
#define UVC_TAR_WIDTH_H264		1280        /*UVC Target Width for H264 */
#define UVC_TAR_HEIGHT_H264		720         /*UVC Target Height for H264 */

#define UVC_TAR_WIDTH_MJPEG     1280        /*UVC Target Width for MJPEG */
#define UVC_TAR_HEIGHT_MJPEG    720         /*UVC Target Height for MJPEG */

#define UVC_TAR_WIDTH_YUY2       640         /*UVC Target Width for YUV */
#define UVC_TAR_HEIGHT_YUY2      480         /*UVC Target Height for YUV */


#define H264_TAR_BITRATE	7200
#define H264_GOP_LENGTH		10
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
#endif
