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
#ifndef __EXIF_INFO_DEFINE_H__
#define __EXIF_INFO_DEFINE_H__

#include "application.h"
#include "exif.h"

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/



/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
INT32S exif_info_init(INT32U src,INT32U thumbnail_img_en);
INT32S exif_info_uninit(INT32U src);
void exif_tag_set(INT32U src,ExifIfd ifd_id,ExifTag tag,ExifArgs* arg);


/* Store raw EXIF data representing the ExifInfo/ExifGPSIfdTagInfo structure into a memory
 * buffer
 */
INT32S exif_data_content_save(INT32U src,INT32U thumbnail_size,INT8U** ret_exif_data,INT32U* ret_exif_data_len);


#endif //__EXIF_INFO_DEFINE_H__
