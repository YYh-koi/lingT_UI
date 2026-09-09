/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2005 by Sunplus Technology Co., Ltd.             *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  Technology Co., Ltd. All rights are reserved by Sunplus Technology    *
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus Technology Co., Ltd. reserves the right to modify this        *
 *  software without notice.                                              *
 *                                                                        *
 *  Sunplus Technology Co., Ltd.                                          *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 *  Author: Yi-Ming Kao                                                   *
 *                                                                        *
 *  Reviewer:                                                             *
 *                                                                        *
 **************************************************************************/
#ifndef __VID_MP4_PACKER_H__
#define __VID_MP4_PACKER_H__

#include "typedef.h"
#include "gp_avcodec.h"
#include "gp_mux.h"
#include <stdio.h>
#include <cmsis_os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NEW_HEADER	1
#define SHOW_WRITE_SPEED    0

#define WRITE_SD_THREAD     1

#if WRITE_SD_THREAD
#define WRITE_SD_TASK_QUEUE_SIZE    (5)
#define WRITE_SD_NOTIFY             0x0001
#define WRITE_SD_FLUSH              0x0002
#define WRITE_SD_WAIT_EXIT          0x0003
#endif

/**************************************************************************
 *                   G E N E R A L    C O N S T A N T S                   *
 **************************************************************************/
#define VIDEO_TRAK              1
#define AUDIO_TRAK              2
#define INFO_TRAK               3

#define IDX_NUM                 9
#define IDX_VID_STTS            0
#define IDX_VID_STSZ            1
#define IDX_VID_STCO            2
#define IDX_VID_STSS            3
#define IDX_VID_STSC            4
#define IDX_AUD_STSZ            5
#define IDX_AUD_STCO            6
#define IDX_AUD_STSC            7
#define IDX_AUD_STTS            8

#define FIRST_CHUNK_OFFSET      0x20
#define FTYP_BOX_SIZE           0x18
#define BOX_HEADER_SIZE         8

#define TIME_ZONE_OFFSET		(8) //UTC time offset (-12)~(12) in hours

#define _3GP_ID_FTYP            0x66747970
#define _3GP_ID_MDAT            0x6D646174
#define _3GP_ID_MOOV            0x6D6F6F76
#define _3GP_ID_MVHD            0x6D766864
#define _3GP_ID_TKHD            0x746B6864
#define _3GP_ID_TRAK            0x7472616B
#define _3GP_ID_MDIA            0x6D646961
#define _3GP_ID_MDHD            0x6D646864
#define _3GP_ID_MINF            0x6D696E66
#define _3GP_ID_VMHD            0x766D6864
#define _3GP_ID_SMHD            0x736D6864
#define _3GP_ID_STBL            0x7374626C
#define _3GP_ID_STSD            0x73747364
#define _3GP_ID_STSC            0x73747363
#define _3GP_ID_STTS            0x73747473
#define _3GP_ID_STSZ            0x7374737A
#define _3GP_ID_STCO            0x7374636F
#define _3GP_ID_STSS            0x73747373
#define _3GP_ID_HMHD            0x6E6D6864
#define _3GP_ID_NMHD            0x686D6864

#define LONG_FILENAME_LEN       1024

//#define ONLY_ONE_HOUR_REC

#define MODE_MP4  0x01
#define MODE_MOV  0x02

enum MovChannelLayoutTag {
    MOV_CH_LAYOUT_UNKNOWN               = 0xFFFF0000,
    MOV_CH_LAYOUT_USE_DESCRIPTIONS      = (  0 << 16) | 0,
    MOV_CH_LAYOUT_USE_BITMAP            = (  1 << 16) | 0,
    MOV_CH_LAYOUT_DISCRETEINORDER       = (147 << 16) | 0,
    MOV_CH_LAYOUT_MONO                  = (100 << 16) | 1,
    MOV_CH_LAYOUT_STEREO                = (101 << 16) | 2,
    MOV_CH_LAYOUT_STEREOHEADPHONES      = (102 << 16) | 2,
    MOV_CH_LAYOUT_MATRIXSTEREO          = (103 << 16) | 2,
    MOV_CH_LAYOUT_MIDSIDE               = (104 << 16) | 2,
    MOV_CH_LAYOUT_XY                    = (105 << 16) | 2,
    MOV_CH_LAYOUT_BINAURAL              = (106 << 16) | 2,
    MOV_CH_LAYOUT_AMBISONIC_B_FORMAT    = (107 << 16) | 4,
    MOV_CH_LAYOUT_QUADRAPHONIC          = (108 << 16) | 4,
    MOV_CH_LAYOUT_PENTAGONAL            = (109 << 16) | 5,
    MOV_CH_LAYOUT_HEXAGONAL             = (110 << 16) | 6,
    MOV_CH_LAYOUT_OCTAGONAL             = (111 << 16) | 8,
    MOV_CH_LAYOUT_CUBE                  = (112 << 16) | 8,
    MOV_CH_LAYOUT_MPEG_3_0_A            = (113 << 16) | 3,
    MOV_CH_LAYOUT_MPEG_3_0_B            = (114 << 16) | 3,
    MOV_CH_LAYOUT_MPEG_4_0_A            = (115 << 16) | 4,
    MOV_CH_LAYOUT_MPEG_4_0_B            = (116 << 16) | 4,
    MOV_CH_LAYOUT_MPEG_5_0_A            = (117 << 16) | 5,
    MOV_CH_LAYOUT_MPEG_5_0_B            = (118 << 16) | 5,
    MOV_CH_LAYOUT_MPEG_5_0_C            = (119 << 16) | 5,
    MOV_CH_LAYOUT_MPEG_5_0_D            = (120 << 16) | 5,
    MOV_CH_LAYOUT_MPEG_5_1_A            = (121 << 16) | 6,
    MOV_CH_LAYOUT_MPEG_5_1_B            = (122 << 16) | 6,
    MOV_CH_LAYOUT_MPEG_5_1_C            = (123 << 16) | 6,
    MOV_CH_LAYOUT_MPEG_5_1_D            = (124 << 16) | 6,
    MOV_CH_LAYOUT_MPEG_6_1_A            = (125 << 16) | 7,
    MOV_CH_LAYOUT_MPEG_7_1_A            = (126 << 16) | 8,
    MOV_CH_LAYOUT_MPEG_7_1_B            = (127 << 16) | 8,
    MOV_CH_LAYOUT_MPEG_7_1_C            = (128 << 16) | 8,
    MOV_CH_LAYOUT_EMAGIC_DEFAULT_7_1    = (129 << 16) | 8,
    MOV_CH_LAYOUT_SMPTE_DTV             = (130 << 16) | 8,
    MOV_CH_LAYOUT_ITU_2_1               = (131 << 16) | 3,
    MOV_CH_LAYOUT_ITU_2_2               = (132 << 16) | 4,
    MOV_CH_LAYOUT_DVD_4                 = (133 << 16) | 3,
    MOV_CH_LAYOUT_DVD_5                 = (134 << 16) | 4,
    MOV_CH_LAYOUT_DVD_6                 = (135 << 16) | 5,
    MOV_CH_LAYOUT_DVD_10                = (136 << 16) | 4,
    MOV_CH_LAYOUT_DVD_11                = (137 << 16) | 5,
    MOV_CH_LAYOUT_DVD_18                = (138 << 16) | 5,
    MOV_CH_LAYOUT_AUDIOUNIT_6_0         = (139 << 16) | 6,
    MOV_CH_LAYOUT_AUDIOUNIT_7_0         = (140 << 16) | 7,
    MOV_CH_LAYOUT_AUDIOUNIT_7_0_FRONT   = (148 << 16) | 7,
    MOV_CH_LAYOUT_AAC_6_0               = (141 << 16) | 6,
    MOV_CH_LAYOUT_AAC_6_1               = (142 << 16) | 7,
    MOV_CH_LAYOUT_AAC_7_0               = (143 << 16) | 7,
    MOV_CH_LAYOUT_AAC_OCTAGONAL         = (144 << 16) | 8,
    MOV_CH_LAYOUT_TMH_10_2_STD          = (145 << 16) | 16,
    MOV_CH_LAYOUT_TMH_10_2_FULL         = (146 << 16) | 21,
    MOV_CH_LAYOUT_AC3_1_0_1             = (149 << 16) | 2,
    MOV_CH_LAYOUT_AC3_3_0               = (150 << 16) | 3,
    MOV_CH_LAYOUT_AC3_3_1               = (151 << 16) | 4,
    MOV_CH_LAYOUT_AC3_3_0_1             = (152 << 16) | 4,
    MOV_CH_LAYOUT_AC3_2_1_1             = (153 << 16) | 4,
    MOV_CH_LAYOUT_AC3_3_1_1             = (154 << 16) | 5,
    MOV_CH_LAYOUT_EAC3_6_0_A            = (155 << 16) | 6,
    MOV_CH_LAYOUT_EAC3_7_0_A            = (156 << 16) | 7,
    MOV_CH_LAYOUT_EAC3_6_1_A            = (157 << 16) | 7,
    MOV_CH_LAYOUT_EAC3_6_1_B            = (158 << 16) | 7,
    MOV_CH_LAYOUT_EAC3_6_1_C            = (159 << 16) | 7,
    MOV_CH_LAYOUT_EAC3_7_1_A            = (160 << 16) | 8,
    MOV_CH_LAYOUT_EAC3_7_1_B            = (161 << 16) | 8,
    MOV_CH_LAYOUT_EAC3_7_1_C            = (162 << 16) | 8,
    MOV_CH_LAYOUT_EAC3_7_1_D            = (163 << 16) | 8,
    MOV_CH_LAYOUT_EAC3_7_1_E            = (164 << 16) | 8,
    MOV_CH_LAYOUT_EAC3_7_1_F            = (165 << 16) | 8,
    MOV_CH_LAYOUT_EAC3_7_1_G            = (166 << 16) | 8,
    MOV_CH_LAYOUT_EAC3_7_1_H            = (167 << 16) | 8,
    MOV_CH_LAYOUT_DTS_3_1               = (168 << 16) | 4,
    MOV_CH_LAYOUT_DTS_4_1               = (169 << 16) | 5,
    MOV_CH_LAYOUT_DTS_6_0_A             = (170 << 16) | 6,
    MOV_CH_LAYOUT_DTS_6_0_B             = (171 << 16) | 6,
    MOV_CH_LAYOUT_DTS_6_0_C             = (172 << 16) | 6,
    MOV_CH_LAYOUT_DTS_6_1_A             = (173 << 16) | 7,
    MOV_CH_LAYOUT_DTS_6_1_B             = (174 << 16) | 7,
    MOV_CH_LAYOUT_DTS_6_1_C             = (175 << 16) | 7,
    MOV_CH_LAYOUT_DTS_6_1_D             = (182 << 16) | 7,
    MOV_CH_LAYOUT_DTS_7_0               = (176 << 16) | 7,
    MOV_CH_LAYOUT_DTS_7_1               = (177 << 16) | 8,
    MOV_CH_LAYOUT_DTS_8_0_A             = (178 << 16) | 8,
    MOV_CH_LAYOUT_DTS_8_0_B             = (179 << 16) | 8,
    MOV_CH_LAYOUT_DTS_8_1_A             = (180 << 16) | 9,
    MOV_CH_LAYOUT_DTS_8_1_B             = (181 << 16) | 9,
};
/**************************************************************************
 *                             M A C R O S                                *
 **************************************************************************/


/**************************************************************************
 *                         D A T A   T Y P E S                            *
 **************************************************************************/
typedef enum
{
	Mp4DataHeader = 0,
	Mp4DataNormal,
	Mp4DataAppendMovEnd,
	Mp4DataAppendFileEnd,
} mp4DataBoxType_t;

/**************************************************************************
 *                        G L O B A L   D A T A                           *
 **************************************************************************/


/**************************************************************************
 *                 E X T E R N A L   R E F E R E N C E S                  *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N   D E C L A R A T I O N S                *
 **************************************************************************/

#if MP4_RECOVER
//ytliao 2014.08 Mux Info data to insert into video file.
typedef struct MP4MuxInfoBox_s
{
	vidVideoType_t vidType;       /* e.g. MPEG4, MJPG, etc */
	audAudioType_t audType;       /* e.g. AMR, AAC, etc */
	INT32U width;         /* pixels in x-axis */
	INT32U height;        /* pixels in y-axis */
	INT32U frmRate;       /* frame rate (fps) */

	INT32U vidExtraLen;

	INT16U audSR;         /* audio sample rate */
	INT8U  audChannel;    /* audio channel */
	INT8U  audBits;       /* Bits of per audio sample */

	INT32U gVidRecSize;
	INT32U gVidFrmNo;
	INT32U gAudRecSize;
	INT32U gAudFrmNo;

	INT32U gSTTSEntryCnt;

	INT32U gPackAudDuration;
	INT32U gPackVidDuration;
	INT32U gVidIFrmNo;

	INT32U prevOffset;
	INT32U gInfoBoxSize;
}MP4MuxInfoBox_t;
#endif

typedef struct MP4MuxDataBox_s
{
	INT32U name;
	INT32U size;
	INT32U buffer_size;
	INT32U level;
	mp4DataBoxType_t type;
	INT8U* data;
} MP4MuxDataBox_t;

typedef struct MP4DataBoxInfo_s {
	INT32U name;
	INT32U level;
	INT32S type;
} MP4DataBoxInfo_t;

typedef struct MP4MetaItem_s {
	INT8U* tag;
	INT8U* value;
} MP4MetaItem_t;

typedef struct MP4MuxInfo_s
{
	vidVideoType_t vidType;       /* e.g. MPEG4, MJPG, etc */
	audAudioType_t audType;       /* e.g. AMR, AAC, etc */
	INT32U width;         /* pixels in x-axis */
	INT32U height;        /* pixels in y-axis */
	INT32U frmRate;       /* frame rate (fps) */
	INT32U dpEn;          /* Data Partitioned enable */
	INT32U gopLen;

	INT32U vidExtraLen;   /* The count in bytes of the size of  video extra information */
	INT8U* pVidExtraData; /* video extra information */
	INT32U audExtraLen;   /* The count in bytes of the size of  audio extra information */
	INT8U* pAudExtraData; /* audio extra information */
	INT32U vidStreamNum;  /* video stream number */
	INT32U audStreamNum;  /* audio stream number */

	INT32U audBitrate;    /* audio bitrate*/
	INT16U audSR;         /* audio sample rate */
	INT8U  audChannel;    /* audio channel */
	INT8U  audBits;       /* Bits of per audio sample */

	INT32U maxFileSize;
	INT32U clusterSize;     /* jerry: add for cluster alighment */
	INT8U  tempFile[LONG_FILENAME_LEN];

	INT8U* gpMP4PktBufStart;
	INT8U* gpMP4PktBufCurr;
#if WRITE_SD_THREAD
	INT8U* gpMP4PktBufReadCurr; //point to next data to write sd
#endif
	INT8U* gpMP4PktBufEnd;
	INT8U* gpMP4IdxBufStart[IDX_NUM];
	INT8U* gpMP4IdxBufCurr[IDX_NUM];
	INT8U* gpMP4IdxBufEnd[IDX_NUM];
	INT32U gpMP4IdxBufSize[IDX_NUM];
	INT8U* gpMP4HeaderStart;

	INT8U  gbIdxFd[IDX_NUM];
	INT16S gPackFd;
	INT32U gEstSize;

	INT32U gMP4CurOff;
	INT32U gPrevSampleTime;
	INT32U gVidRecSize;
	INT32U gVidFrmNo;
	INT32U gAudRecSize;
	INT32U gAudFrmNo;
	INT32U gThumbSize;

	INT32U gSTTSEntryCnt;

	INT32U gPackAudDuration;
	INT32U gPackAudDurationT;
	INT32U gPackVidDuration;
	INT8U  gbReserved;
	INT32U gVidIFrmNo;
	INT8U* gFreeBuf;

	INT32U mode;
	INT32U gFirstChunkOffset;

#if NEW_HEADER
#if WRITE_SD_THREAD
	INT32U gHeaderOffset;
	INT32U gHeaderEndOffset;
	INT32U gHeaderLastOffset;
	INT32U gPackLastIndexTime;
	INT32U gAlignBytesBak;
	INT32U gAlignBytesBak2;

	INT8U *gpMoovBufStart;
	INT8U *gpMoovBufEnd;
	INT32U gMoovBufSizeUsed;

	INT8U  bMoovBufReady;
#else
	INT32U gHeaderOffset;
	INT32U gHeaderEndOffset;
	INT32U gPackLastIndexTime;
	INT32U gHeaderLastOffset;
	INT16U gSectorAlignOffset;
#endif
#endif
	MP4MuxDataBox_t* dataBox;
	INT32U dataBoxNum;
	INT32U dataBoxSize;
	osMutexId pkt_mutex;
	INT8U metaNum;
	MP4MetaItem_t meta[16];
#if WRITE_SD_THREAD
	QueueHandle_t  WriteSdQ;
	QueueHandle_t  WriteSdAck;
    osThreadId WriteSdThreadId;
    INT8U write_sd_err;
    INT8U change_prio;
#endif
	INT32U buffer_size;

	MuxEncryptFunc	encryptFunc;
}MP4MuxInfo_t;

INT32S muxMP4Open(INT16S out,INT8U flag);
INT32S muxMOVOpen(INT16S out,INT8U flag);
INT32S muxMP4Close(INT32S hd);
INT32S muxMP4Pack(INT32S hd, gpMuxPkt_t *pkt, gpEsType_t type);
INT32S muxMP4Set(INT32S hd, INT32U param, INT32U value);
INT32S muxMP4Get(INT32S hd, INT32U param, INT32U *value);
void muxMP4SetDataBox(INT32S hd, void* box);
INT32S muxMP4AddMetaData(INT32S hd, const char* tag, const char* value);
INT32S muxMP4SetEncryptFunc(INT32S hd, MuxEncryptFunc func);

/************** Turnkey *************************/
INT32S muxMOVUninit(INT32S hd, INT8U flag);
extern INT32U MOOV_HEADER_SIZE(void);


#ifdef __cplusplus
}
#endif
#endif /*__VID_MP4_PACKER_H__*/


