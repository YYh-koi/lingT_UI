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

#include "drv_l1_lbp.h"


/**************************************************************************
 *                           F U N C T I O N S                            *
 **************************************************************************/

/**
 * @brief   H/W LBP generator mode
 * @param   inImg: input image start address
 * @param   outImg: output image start address
 * @param   imageWidth: input image width
 * @param   imageHeight: input image Height
 * @param   format_mode: input color format
 * @return 	none
 */
void LBPGenMode_HW(void *inImg, void *outImg, int imageWidth, int imageHeight, int threshold, int format_mode)
{
	int yuv_mode=0;

	rLBP_CTRL &= 0xf8;


	rIMG_STR_ADDR = (unsigned int) inImg;
	rIMG_SIZE = (imageWidth << 16) | (imageHeight);
	rLBPGEN_OB_ADDR =  (unsigned int) outImg;

	if(format_mode == IMG_FMT_GRAY)
		yuv_mode = YUV_FMT_YONLY;
	else if(format_mode == IMG_FMT_YUYV || format_mode == IMG_FMT_YCbYCr)
		yuv_mode = YUV_FMT_YUYV;
	else if(format_mode == IMG_FMT_UYVY || format_mode == IMG_FMT_CbYCrY)
		yuv_mode = YUV_FMT_UYVY;
	else if(format_mode == IMG_FMT_GP420)
		yuv_mode = GP_FMT_420;

	rLBP_CTRL = yuv_mode | (threshold << 8) | LBP_GEN_EN_ON;

	while(1)
	{
		if(rLBP_STATUS & LBP_GEN_DONE) break;
	}

	rLBP_STATUS = LBP_GEN_DONE;

}



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
void LBPAutoMode_HW(void *inImg, void *outImg, void *HammImg, int imageWidth, int imageHeight,  int threshold, int format_mode)
{
	int yuv_mode;

	rLBP_CTRL &= 0xf8;

	rIMG_STR_ADDR = (unsigned int) inImg;
	rIMG_SIZE = (imageWidth << 16) | (imageHeight);
	rLBPGEN_OB_ADDR =  (unsigned int) outImg;
	rLBPCMP_OB_ADDR =  (unsigned int) HammImg;


	if(format_mode == IMG_FMT_GRAY)
		yuv_mode = YUV_FMT_YONLY;
	else if(format_mode == IMG_FMT_YUYV || format_mode == IMG_FMT_YCbYCr)
		yuv_mode = YUV_FMT_YUYV;
	else if(format_mode == IMG_FMT_UYVY || format_mode == IMG_FMT_CbYCrY)
		yuv_mode = YUV_FMT_UYVY;
	else if(format_mode == IMG_FMT_GP420)
		yuv_mode = GP_FMT_420;
    else
        yuv_mode = 0;

	rLBP_CTRL = yuv_mode | (threshold << 8) | LBP_AUTO_EN_ON;

	while(1)
	{
		if(rLBP_STATUS & LBP_AUTO_DONE) break;
	}

	rLBP_STATUS = LBP_AUTO_DONE;

}



/**
 * @brief   H/W LBP compare mode
 * @param   srcImg: source image information
 * @param   dstImg: destination image information
 * @param   srcRoi: source ROI information
 * @param   dstRoi: destination ROI information
 * @return 	total sum
 */

int LBPCmpMode_HW(gpImage *srcImg, gpImage *dstImg, gpRect *srcRoi, gpRect *dstRoi)
{
	int ret;
	int yuv_mode;
	unsigned char *srcImg_ptr, *dstImg_ptr;
	//int *srcImg_ptr2, *dstImg_ptr2;


	rLBP_CTRL &= 0xf8;

	if(srcImg->format == IMG_FMT_GRAY)
		yuv_mode = YUV_FMT_YONLY;
	else if(srcImg->format == IMG_FMT_YUYV || srcImg->format == IMG_FMT_YCbYCr)
		yuv_mode = YUV_FMT_YUYV;
	else if(srcImg->format == IMG_FMT_UYVY || srcImg->format == IMG_FMT_CbYCrY)
		yuv_mode = YUV_FMT_UYVY;
	else if(srcImg->format == IMG_FMT_GP420)
		yuv_mode = GP_FMT_420;
    else
        return 0;

	srcImg_ptr = srcImg->ptr + srcRoi->y * srcImg->widthStep + srcRoi->x;
	dstImg_ptr = dstImg->ptr + dstRoi->y * dstImg->widthStep + dstRoi->x;


	rLBP_OCODE_ADDR = (int) srcImg_ptr;
	rLBP_NCODE_ADDR = (int) dstImg_ptr;
	rLBP_IMG_SIZE = (srcImg->width << 16) | srcImg->height;
	rLBP_BLK_SIZE = (srcRoi->width << 16) | srcRoi->height;

	rLBP_CTRL = (yuv_mode << 16) | (0 << 8) | LBP_CMP_EN_ON;

	// wait for total sum done
	while(1) {
		if(rLBP_STATUS & LBP_CMP_DONE) break;
	}
	ret = rTOTAL_SUM;
	rLBP_STATUS = LBP_CMP_DONE;

	return ret;
}


/**
 * @brief   S/W LBP generator mode
 * @param   inImg: input image start address
 * @param   outImg: output image start address
 * @param   imageWidth: input image width
 * @param   imageHeight: input image Height
 * @param   format_mode: input color format
 * @return 	none
 */
void LBPGenMode(void *inputImage_adr, void *outputImage_adr, int imageWidth, int imageHeight, int threshold, int format_mode)
{
	int shiftBit_x[8] = { -1,  0,  1, -1,  1, -1,  0, 1};
	int shiftBit_y[8] = { -1, -1, -1,  0,  0,  1,  1, 1};
	int i, j, n;
	unsigned char *inputImage_UCAdr = (unsigned char *)inputImage_adr;
	unsigned char *outputImage_UCAdr = (unsigned char *)outputImage_adr;
	int shiftP;


	//cache_invalid_range((unsigned int) inputImage_adr, imageWidth * imageHeight);

	if(format_mode == IMG_FMT_GRAY)
	{
		shiftP = 1;
	}
	else if(format_mode == IMG_FMT_YUYV || format_mode == IMG_FMT_YCbYCr)
	{
		shiftP = 2;
	}
	else if(format_mode == IMG_FMT_UYVY || format_mode == IMG_FMT_CbYCrY)
	{
		shiftP = 2;
		inputImage_UCAdr ++;
	}else
        shiftP = 0;



	for(j = 0 ; j  < imageHeight; j++)
	{
		for(i = 0 ; i  < imageWidth; i++)
		{
			if(i == 0 || j == 0 || i == imageWidth-1 || j == imageHeight-1)
			{
				*outputImage_UCAdr = 0;
				outputImage_UCAdr++;
				inputImage_UCAdr += shiftP;
			}
			else
			{
				int shiftBit = 0;
				unsigned int LBPValue =0 , centrolBit = 0 , aroundBit = 0 ;

				centrolBit = *inputImage_UCAdr;

				centrolBit += threshold;
				//print_string("[%d, %d] => %x:  ", i, j, centrolBit);
				for(n = 0; n < 8; n++)
				{

					aroundBit = *(inputImage_UCAdr+ (shiftBit_x[n]*shiftP) + (shiftBit_y[n]*imageWidth*shiftP));

					shiftBit  <<=  1;
					if(aroundBit > centrolBit)
						shiftBit++;
				}
				//print_string(" %x,\r\n", aroundBit);
				//print_string("\r\n");

				inputImage_UCAdr += shiftP;

				LBPValue = (unsigned char) shiftBit;
				*outputImage_UCAdr = LBPValue;
				outputImage_UCAdr++;
			}
		}
	}
}


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

void LBPAutoMode(void *inputImage_adr, void *outputImage_adr, void *hammingDist_adr, int imageWidth, int imageHeight, int threshold, int format_mode)
{

	int shiftBit_x[8] = { -1,  0,  1, -1,  1, -1,  0, 1};
	int shiftBit_y[8] = { -1, -1, -1,  0,  0,  1,  1, 1};
	unsigned char *inputImage_UCAdr = (unsigned char *)inputImage_adr;
	unsigned char *outputImage_UCAdr = (unsigned char *)outputImage_adr;
	unsigned char *hammingDist_UCAdr = (unsigned char *)hammingDist_adr;
	int i, j, n;
	int bitOneCnt;
	unsigned char LBPValueOld;
	unsigned char XORValue;
	int shiftP;


	if(format_mode == IMG_FMT_GRAY)
	{
		shiftP = 1;
	}
	else if(format_mode == IMG_FMT_YUYV || format_mode == IMG_FMT_YCbYCr)
	{
		shiftP = 2;
	}
	else if(format_mode == IMG_FMT_UYVY || format_mode == IMG_FMT_CbYCrY)
	{
		shiftP = 2;
		inputImage_UCAdr ++;
	}else
        shiftP = 0;


	for(j = 0 ; j  < imageHeight; j++)
	{
		for(i = 0 ; i  < imageWidth; i++)
		{
			if(i == 0 || j == 0 || i == imageWidth-1 || j == imageHeight-1)
			{
				*outputImage_UCAdr = 0;
				outputImage_UCAdr++;
				*hammingDist_UCAdr = 0;
				hammingDist_UCAdr++;
				inputImage_UCAdr += shiftP;
			}
			else
			{
				int shiftBit = 0;
				unsigned char LBPValue =0 , centrolBit = 0 , aroundBit = 0 ;

				centrolBit = *inputImage_UCAdr;

				for(n = 0; n < 8; n++)
				{

					aroundBit = *(inputImage_UCAdr+ (shiftBit_x[n]*shiftP) + (shiftBit_y[n]*imageWidth*shiftP));
					shiftBit  <<=  1;
					if(aroundBit > centrolBit + threshold)
						shiftBit++;
				}

				inputImage_UCAdr += shiftP;

				LBPValue = (unsigned char) shiftBit;
				LBPValueOld = *outputImage_UCAdr;

				XORValue = LBPValue ^ LBPValueOld;


				bitOneCnt = 0;
				for( n = 0 ; n < 8; n ++)
				{
					if(XORValue%2 == 1)
						bitOneCnt++;

					XORValue >>= 1;
				}

				*hammingDist_UCAdr = (unsigned char)bitOneCnt;
				hammingDist_UCAdr++;
				*outputImage_UCAdr = LBPValue;
				outputImage_UCAdr++;
			}
		}
	}
}



/**
 * @brief   S/W LBP compare mode
 * @param   srcImg: source image information
 * @param   dstImg: destination image information
 * @param   srcRoi: source ROI information
 * @param   dstRoi: destination ROI information
 * @return 	total sum
 */
int LBPCmpMode(gpImage *srcImg, gpImage *dstImg, gpRect *srcRoi, gpRect *dstRoi)
{
	int i, j;
	unsigned char *srcImg_ptr, *dstImg_ptr;
	int XORValue, total_sum;

	srcImg_ptr = srcImg->ptr + srcRoi->y * srcImg->widthStep + srcRoi->x;
	dstImg_ptr = dstImg->ptr + dstRoi->y * dstImg->widthStep + dstRoi->x;

	//print_string("\r\n\r\n");

	total_sum = 0;
	j = srcRoi->height;
	do {
		unsigned char *p_src, *p_dst;

		p_src = srcImg_ptr;
		p_dst = dstImg_ptr;

		i = srcRoi->width;
		do {
			unsigned char new_code, old_code;
			int k = 0;

			new_code = *p_src++;
			old_code = *p_dst++;

			XORValue = new_code ^ old_code;

			k += XORValue & 0x1;
			k += (XORValue >> 1) & 0x1;
			k += (XORValue >> 2) & 0x1;
			k += (XORValue >> 3) & 0x1;
			k += (XORValue >> 4) & 0x1;
			k += (XORValue >> 5) & 0x1;
			k += (XORValue >> 6) & 0x1;
			k += (XORValue >> 7) & 0x1;

			//print_string("src: %x, dst: %x, xor_val = %x, dist = %d\r\n", new_code, old_code, XORValue, k);

			total_sum += k;

			i--;
		} while(i != 0);

		//print_string("src+1: %x, dst+1: %x\r\n",*p_src++, *p_dst++);

		srcImg_ptr += srcImg->widthStep;
		dstImg_ptr += dstImg->widthStep;
		j--;
	} while(j != 0);
	//print_string("\r\n\r\n");

	return total_sum;
}
//#endif
