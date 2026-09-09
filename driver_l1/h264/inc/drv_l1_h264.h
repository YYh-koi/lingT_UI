/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2021 by Generalplus Inc.                         *
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

#ifndef __drv_l1_H264_H__
#define __drv_l1_H264_H__

/******************************************************************************
 * Constant Definitions
 *****************************************************************************/

#define BIT_FRAME_END		0x1
#define BIT_MB_STOP			0x2
#define BIT_INPUT_FIFO_STOP	0x4
#define BIT_SCAN2MB_END		0x8
#define BIT_FBC_ERROR		0x10000
#define BIT_FIFO_DONE		0x20000
#define BIT_FIFO_ERROR		0x40000
//0x14
#define BIT_ENC_TRIGGER         0x1
#define BIT_DEC_TRIGGER         0x2
#define BIT_ENC_CONT_TRIGGER	0x4
#define BIT_CODEC_ENGINE_RESET  0x10

//0x20
#define REF_FRAME       0x0
#define NON_REF_FRAME   0x2
#define I_FRAME         0x1
#define P_FRAME         0x0

//0x28
#define PPS_EN          0x1
#define SPS_EN          0x2
#define IDR_FLAG        0x4
#define CABAC_EN		0x8

//0x90
#define ENABLE_MB_STOP			(0x1 << 31)
#define ENABLE_ROI				(0x1 << 30)
#define ENABLE_VUI				(0x1 << 29)
#define DISABLE_COEFF_COST		(0x1 << 28)
#define NEW_OP_2_QUANT_TABLE    (0x1 << 26)
#define I16_COST_TWEAK          (0x1 << 25)
#define FOURCE_INTRA_AS_SCN_CHG (0x1 << 24)

//0xc0160038 FBC
#define FBC_INT_MAXSIZE			0x1
#define FBC_INT_DEC_ERR			0x2

#define FRAME_SKIP              (0x1 << 30)

#ifndef H264_FMT_GP420
	#define H264_FMT_GP420	0
	#define H264_FMT_YUYV	1
	#define H264_FMT_YVYU	2
	#define H264_FMT_UYVY	3
	#define H264_FMT_VYUY	4
#endif

/*******************************************************************************
 * Data Types
 ******************************************************************************/

/*typedef struct H264Enc_Reg_s
{
	INT32U FrameDataSize;         //0x1c
	INT32U PictureType;           //0x20
	INT32U QP;                    //0x24
	INT32U MiscSetting;           //0x28
	INT32U ProfileLevel;          //0x30
	INT32U BitstreamStartAddr;    //0x34
	INT32U RawFrameAddr;          //0x40
	INT32U RecLumaAddr;           //0x60
	INT32U RecChromaAddr;         //0x64
	INT32U RecMbYOffset;          //0x68
	INT32U RefMbYOffset;          //0x6c
	INT32U MaxMbYOffset;          //0x70
	INT32U MEDynamicSearch;       //0x90
	INT32U FunctionControl;       //0x94
	INT32U FrameCropOffset;       //0xb0
} H264Enc_Reg_t;*/

typedef struct h264_buffers
{
	INT32U LumaAddr0;
	INT32U ChromaAddr0;
	INT32U LumaAddr;
	INT32U ChromaAddr;
	INT32U RecMBYOffset;
	INT32U RefMBYOffset;
	INT32U MB2ScanYOffset;
	INT32U NotUsedMBYOffset;
	INT32U MaxMBYOffset;
	INT32U FbcAddr;
	INT32U FbcBuf[2];
	INT32U DivOutLumaAddr0;
	INT32U DivOutChromaAddr0;
	INT32U DivOutLumaAddr[3];
	INT32U DivOutChromaAddr[3];
	INT16U DivOutIdx;
	INT32U width;
	INT32U height;
	INT8U* ArTable;
	INT8U  BufNum;
	INT8U  FbcRefReady;
    INT32U extBuf;
} h264_buffers_t;

typedef struct h264_statistic
{
	INT32U cost;
	INT32U cnt_byte;
	INT16U cnt_i_mb;
	INT16U cnt_skip_mb;
	INT16U cnt_p_mb;
} h264_stat_t;

/******************************************************************************
 * Function Prototypes
 *****************************************************************************/
void H264_EnableCoreClock();
void H264_DisableCoreClock();
void H264Enc_EnablePeripheralClock();
void H264Enc_DisablePeripheralClock();
void H264Dec_EnablePeripheralClock();
void H264Dec_DisablePeripheralClock();

/* HW Control *************************************************************/

/**
 * @brief   Reset H.264 module
 */
int H264_ResetEngine();

/**
 * @brief   Soft Reset H.264 module
 */
void H264_SoftReset(void);

/**
 * @brief   Trigger H.264 FBC for Encoding
 */
void H264Enc_FBC_Run();

/**
 * @brief   Trigger H.264 Encoding
 */
void H264Enc_Trigger();

/**
 * @brief   Trigger H.264 continue encoding
 * @note 	obsolete, use in MB mode encoding
 */
void H264Enc_Continue();

/**
 * @brief   Trigger H.264 FBC for Decoding
 */
 void H264Dec_FBC_Run();

/**
 * @brief   Save H264 Decoder Status
 */
void H264Dec_SaveRegStatus(INT32U* status);

/**
 * @brief   Restore H264 Decoder Status
 */
void H264Dec_RestoreRegStatus(INT32U* status);

/**
 * @brief   Trigger H.264 decoding
 */
void H264Dec_Trigger();
INT32U H264_Wait_Complete();
INT32U H264_FIFO_Wait_Complete();
INT32U SCAN2MB_Wait_Complete();
int H264_Set_Stop_MB(INT8U MB_X, INT8U MB_Y);
int H264_SetFrameSize(int width, int height);
void H264Enc_SetFrame_I(int idr_pic_id, int ref, int sps_pps);
void H264Enc_SetFrame_P(INT8U fnum, INT8U poc, INT8U ref);
int H264_GetFrameSize(int* width, int* height);
void H264Enc_SetFrame_Skip(INT8U fnum, INT8U poc);
int H264_GetFrameType();
int H264_SetQP(int qp);
int H264_GetQP();
void H264_SetLevel(int level);
void H264_GetLevel(int * level);
void H264_SetFrameAddr(INT32U addr);
void H264Enc_SetBitstreamAddr(INT32U addr, INT32U base, INT32U max);
void H264Dec_SetBitstreamAddr(INT32U addr, INT32U base, INT32U max);
void H264Enc_Enable_MBStop(INT8U enable);
void H264Enc_Enable_ROI(INT8U enable);
void H264Enc_Enable_VUI(INT8U enable);
void H264Enc_SetMEDynamicSearchWindow(int rangeX, int rangeY);
void H264_SetFrameCropOffset(int left, int right, int top, int bottom);
void H264_GetFrameCropOffset(int* left, int* right, int* top, int* bottom);
INT32U H264_GetBistreamLength();
INT32U H264_GetBistreamBypassLength();
void H264_Get_MB_Status(INT8U* Info_MB_X, INT8U* Info_MB_Y, INT8U* Process_MB_X, INT8U* Process_MB_Y);
INT32S H264Enc_Get_Statistic( h264_stat_t* stat);
INT32S H264Enc_Enable_ROI_Data(INT8U idx, INT8U en);
INT32S H264Enc_Set_ROI_Data(INT8U idx, INT8U qp, INT8U mb_x, INT8U mb_y, INT8U mb_x1, INT8U mb_y1);
INT32S H264Enc_Clear_ROI_Data(INT8U idx);
INT32S H264Enc_Get_ROI_Stat(INT8U idx, h264_stat_t* stat);
void H264_RestoreBuffers(INT32U param);
INT32U H264Enc_InitBuffers(int width, int height, int BufferNum, int FBC);
void H264Enc_UpdateBuffers(INT32U param, int update_offset);
INT32U H264Dec_InitBuffers(int width, int height, int out_div, int BufferNum, int FBC, void* ext_buf);
void H264Dec_UpdateBuffers(INT32U param, int update_offset);
void H264_ReleaseBuffers(INT32U param);
void H264Dec_GetOutputBuffer(INT32U *LumaAddr, INT32U *ChromaAddr);
void H264_DumpRegister();
/* MB2Scan *************************************************************/

/**
 * @brief   MB2SCAN Drive Init/Uninit
 */
INT32S H264Dec_MB2SCAN_Init(void);
INT32S H264Dec_MB2SCAN_Uninit(void);

/**
 * @brief   MB2SCAN Reset
 */
void H264Dec_MB2ScanReset();

/**
 * @brief   trigger MB2SCAN to run
 * @param	patch_offset:	buffer offset of second pass when width >= 2048
 * @param	mb_offset:	offset of mb line to convert,
 * @note	MB2SCAN convert H.264 internal MB420 format to scanline format.
 */
int H264Dec_MB2Scan(int patch_offset, INT32U mb_offset);

/**
 * @brief   trigger MB2SCAN with H.264 scaler output
 * @param	LumaAddr:	luma address of h264 scaler output buffer
 * @param	ChromaAddr:	chroma address of h264 scaler output buffer
 * @param	div:		division value of h264 scaler
 * @param	return:		0: Success -1:Fail
 */
int H264Dec_MB2ScanDiv(INT32U LumaAddr, INT32U ChromaAddr, INT32U div);

/**
 * @brief   MB2SCAN Notify function
 * @note	notify driver to proceed next MB buffer, use in patch for GP329XXXA
 */
void H264Dec_MB2ScanNotify();

/* Scan2MB *************************************************************/

/**
 * @brief   trigger Scan2MB for H.264 encoding
 * @param	inAddr:		address of input frame buffer
 * @param	width:		frame width
 * @param	height:		frame height
 * @param	informat:	input format H264_FMT_G420,H264_FMT_YUYV~H264_FMT_VYUY
 * @param	inFifo:     input fifo line (16 align)
 */
void H264Enc_Scan2MB(INT32U inAddr, INT16 width, INT16 height, INT8U informat, INT32U inFifo);

/**
 * @brief   restart Scan2MB for H.264 encoding (fifo encoding)
 * @param	inAddr:		address of next input data
 */
void H264Enc_Scan2MB_Restart(INT32U inAddr);
/**
 * @brief   reset Scan2MB
 */
void H264Enc_Scan2MB_Reset();

/**
 * @brief   get Scan2MB status
 * @return	Scan2MB status
 */
INT32U H264Enc_Scan2MB_GetStatus();
INT32U H264Enc_Scan2MB_Wait();
void H264Enc_FIFO_Enable(INT8U enable);
void H264Enc_FIFO_Set(INT32U addr, INT32U size);
void H264Enc_FIFO_Start();
void H264_FIFO_Reset();
void drv_l1_H264_get_version(void);
#endif
