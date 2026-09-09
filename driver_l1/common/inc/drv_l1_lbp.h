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


#ifndef __DRV_L1_LBP_H__
#define __DRV_L1_LBP_H__

# include "typedef.h"
#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define IMG_FMT_GRAY	        0
#define IMG_FMT_YUYV	        1
#define IMG_FMT_YCbYCr	        2
#define IMG_FMT_RGB		        3
#define IMG_FMT_UYVY	        4
#define IMG_FMT_CbYCrY	        5
#define IMG_FMT_INTEGRAL_Y      6
#define IMG_FMT_YUV444          7
#define IMG_FMT_GP420           8

/*
#ifndef NULL
#define NULL 0
#endif
*/

#define LBP_BASE 		  	0xD1800000

#define rLBP_CTRL			(*(volatile unsigned *)(LBP_BASE+0x00))
#define rLBP_STATUS			(*(volatile unsigned *)(LBP_BASE+0x04))
#define rIMG_STR_ADDR		(*(volatile unsigned *)(LBP_BASE+0x08))
#define	rIMG_SIZE			(*(volatile unsigned *)(LBP_BASE+0x0C))
#define rLBPGEN_OB_ADDR		(*(volatile unsigned *)(LBP_BASE+0x10))
#define rLBPCMP_OB_ADDR		(*(volatile unsigned *)(LBP_BASE+0x14))
#define rLBP_OCODE_ADDR			(*(volatile unsigned *)(LBP_BASE+0x18))
#define	rLBP_NCODE_ADDR		(*(volatile unsigned *)(LBP_BASE+0x1C))
#define rLBP_IMG_SIZE		(*(volatile unsigned *)(LBP_BASE+0x20))
#define rLBP_BLK_SIZE			(*(volatile unsigned *)(LBP_BASE+0x24))
#define rTOTAL_SUM			(*(volatile unsigned *)(LBP_BASE+0x28))


#define LBP_AUTO_EN_ON		0x00000001
#define LBP_GEN_EN_ON		0x00000002
#define LBP_CMP_EN_ON		0x00000004

#define YUV_FMT_YONLY		0x00000000
//#define YUV_FMT_YUYV		0x00010000
//#define YUV_FMT_UYVY		0x00020000
#define YUV_FMT_YUYV		0x00020000      //old 0x00010000  unify YUYV define (GP format Y1U0Y0V0)
#define YUV_FMT_UYVY		0x00010000      //old 0x00020000

#define GP_FMT_420  		0x00030000

#define LBP_AUTO_DONE		0x00000001
#define LBP_GEN_DONE		0x00000002
#define LBP_CMP_DONE		0x00000004
#define LBP_CMP_CLEAR		0x00000008

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#define MAX(x, y)	((x) >= (y) ? (x) : (y))
#define MIN(x, y)	((x) <  (y) ? (x) : (y))
#define ABS(x)		((x) >= 0 ? (x) : -(x))
#define ROUND(x)	((int)((x)+0.5))


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


typedef struct
{
	INT16S width;
	INT16S height;
    INT16S widthStep;
	INT8S ch;
	INT8S format;
	INT8U *ptr;
	INT8U *ptr_u;
	INT8U *ptr_v;
} gpImage;

typedef struct
{
    INT16S x;
    INT16S y;
    INT16S width;
    INT16S height;
} gpRect;

typedef struct
{
    INT16S width;
    INT16S height;
} gpSize;

typedef struct {
	char *classify_name;
	void *data;

	int obj_type;

	int stage_of_classify;
	int num_of_classify;
	int size_of_classify;
} ClassifyData;


/**************************************************************************
 *                            E X T E R N A L                             *
 **************************************************************************/

extern void print_string(char *fmt, ...);

extern int cache_invalid_range(unsigned int addr, unsigned int size);


/**
 * @brief   H/W LBP generator mode
 * @param   inImg: input image start address
 * @param   outImg: output image start address
 * @param   imageWidth: input image width
 * @param   imageHeight: input image Height
 * @param   format_mode: input color format
 * @return 	none
 */
extern void LBPGenMode_HW(void *inImg, void *outImg, int imageWidth, int imageHeight, int threshold, int format_mode);

/**
 * @brief   H/W LBP auto mode
 * @param   inImg: input image start address
 * @param   outImg: output image start address
 * @param   HammImg: Hamm image start address
 * @param   imageWidth: input image width
 * @param   imageHeight: input image Height
 * @param   threshold: threshold
 * @param   format_mode: input color format
 * @return 	none
 */
extern void LBPAutoMode_HW(void *inImg, void *outImg, void *HammImg, int imageWidth, int imageHeight, int threshold, int format_mode);

/**
 * @brief   H/W LBP compare mode
 * @param   srcImg: source image information
 * @param   dstImg: destination image information
 * @param   srcRoi: source ROI information
 * @param   dstRoi: destination ROI information
 * @return 	total sum
 */
extern int LBPCmpMode_HW(gpImage *srcImg, gpImage *dstImg, gpRect *srcRoi, gpRect *dstRoi);





/**
 * @brief   S/W LBP generator mode
 * @param   inImg: input image start address
 * @param   outImg: output image start address
 * @param   imageWidth: input image width
 * @param   imageHeight: input image Height
 * @param   format_mode: input color format
 * @return 	none
 */
extern void LBPGenMode(void *inputImage_adr, void *outputImage_adr, int imageWidth, int imageHeight, int threshold, int format_mode);

/**
 * @brief   S/W LBP auto mode
 * @param   inImg: input image start address
 * @param   outImg: output image start address
 * @param   HammImg: Hamm image start address
 * @param   imageWidth: input image width
 * @param   imageHeight: input image Height
 * @param   threshold: threshold
 * @param   format_mode: input color format
 * @return 	none
 */
extern void LBPAutoMode(void *inputImage_adr, void *outputImage_adr, void *hammingDist_adr, int imageWidth, int imageHeight, int threshold, int format_mode);

/**
 * @brief   S/W LBP compare mode
 * @param   srcImg: source image information
 * @param   dstImg: destination image information
 * @param   srcRoi: source ROI information
 * @param   dstRoi: destination ROI information
 * @return 	total sum
 */
extern int LBPCmpMode(gpImage *srcImg, gpImage *dstImg, gpRect *srcRoi, gpRect *dstRoi);


#ifdef __cplusplus
}
#endif


#endif

