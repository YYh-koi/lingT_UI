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

/*******************************************************
    Include file
*******************************************************/
#include "drv_l2.h"
#include "application.h"

#if (defined(LCD_DRV_EN_HDMI) && LCD_DRV_EN_HDMI)

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define SP_HDCP_KEY     		0
#define MONITOR_BLOCK_LIST		0
#define DBG_EDID_BLOCK			1
#define EDID_DETECT_FORMAT		1

#if SP_HDCP_KEY
#include "n007685.h"
#endif

#define HDMI_640X480P60_BIT     0x00000001
#define HDMI_720X480P60_BIT     0x00000002
#define HDMI_1280X720P60_BIT    0x00000004

#define D1_MODE	                2
#define TFT_1280x720_MODE		6

#define HDMI_SEGMENT_ID	        0x60
#define HDMI_DDC_ID_0		    0xA0
#define HDMI_DDC_ID_1		    0xA1
#define HDMI_HDCP_ID_0		    0x74
#define HDMI_HDCP_ID_1		    0x76
#define RETRY_CNT               5

#if 1 //sccb
static void *ddc_handle;
#if HDCP_SUPPORT == 1
static void *hdcp_handle;
#endif
static void *ext_seg_handle;
#else //hardware I2C
static drv_l1_i2c_bus_handle_t hHdmiDDC;
static drv_l1_i2c_bus_handle_t hHdmiDDCSegment;
#endif

//HDMI EDID check ----------------------------------------------------------------------------
enum {
	HDMI_640X480P60 = 0,
	HDMI_720X480P60,
	HDMI_1280X720P60,
	HDMI_VIDEO_TIMING_NUM
};

typedef struct
{
	/*	Progressive: 0, Interlaced: 1	*/
	/*	Negative: 0, Positive: 1	*/
	short	PixelClock,	HFreq,	VFreq;
} HDMI_TIMMING_TAB;

static const HDMI_TIMMING_TAB HdmiTiming[HDMI_VIDEO_TIMING_NUM] =
{
	//	PxlClk	HFreq	VFreq	HRes	VRes	Intrlac	HTotal	HBlank	HFPorch	HSWidth	HBPorch	HPol	VTotal	VBlank	VFPorch	VSWidth	VBPorch	VPol	Oversmp	ID	IsPC	PHY1		PHY2
	{	2475,	315,	60	}, 	// 640 x 480 x60
	{	2700,	315,	60	},	// 480  p60
	{	7425,	450,	60	},	// 720  p60
};

typedef struct
{
    unsigned short PixelClock;                                     // in 10khz
    unsigned char HorizontalAddressableVideo;                      // in pixels
    unsigned char HorizontalBlanking;                              // in pixels
    unsigned char HorizontalExtraInfo;
    unsigned char VerticalAddressableVideo;
    unsigned char VerticalBlanking;
    unsigned char VerticalExtraInfo;

    unsigned char HorizontalFrontPorch;                            //  in pixels
    unsigned char HorizontalSyncPulseWidth;
    unsigned char VerticalSync;
    unsigned char HV_Info;

    unsigned char HorizontalAddressableVideoImageSize;              // in mm
    unsigned char VerticalAddressableVideoImageSize;                // in mm
    unsigned char HV_AddressableVideoImageSize;						// in mm
    unsigned char RightHorizontalBorderOrLeftHorizontalBorder;      // in pixels
    unsigned char TopVerticalBorderOrBottomVerticalBorder;          // in Lines
    unsigned char Info;
} VESA_E_EDID_TIMING_MODE;


typedef struct
{
    unsigned long Indicator;
    unsigned char DisplayRangeLimitsOffsets;
    unsigned char MinimumVerticalRate;
    unsigned char MaximumVerticalRate;
    unsigned char MinimumHorizontalRate;
    unsigned char MaximumHorizontalRate;
    unsigned char MaximumPixelClock;
    unsigned char VideoTimingSupportFlags;
    unsigned char VideoTimingData[7];
} VESA_E_EDID_DISPLAY_RANGE_LIMITS;

typedef struct
{
    int Size, Blanking, FrontPorch, PluseWidth;
} TIMING;

typedef struct
{
    int PixelClock;
    int hSize, hBlanking, hFrontPorch, hPluseWidth;
    int vSize, vBlanking, vFrontPorch, vPluseWidth;
} FRAME_TIMING;

typedef struct
{
    int maxPixelClock;          // MHz
    int hMaxRate, hMinRate;     // kHz
    int vMaxRate, vMinRate;     // Hz
} TIMING_LIMITS;

#if MONITOR_BLOCK_LIST == 1
#define MAX_HDMI_BLACKLIST_NUM  2

static INT8S hdmi_monitor_blacklist[MAX_HDMI_BLACKLIST_NUM][11] = {
    {0x4D, 0xD9, 0x01, 0x5D, 0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x12},
    {0x4D, 0xD9, 0x01, 0x5D, 0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x12}
};
static INT32U hdmi_monitor_blacklist_flag = 0;
#endif

static INT8U g_hdmi_video_format = HDMI_VID_1080P;
static INT8U g_hdmi_video_support_format = HDMI_VID_1080P;
static INT8U g_hdmi_audio_format = HDMI_AUD_44K;
static INT8U g_hdmi_mode = DEVICE_HDMI;//HDMI or DVI
static INT32U g_cec_phyical_addr = 0;
#if MONITOR_BLOCK_LIST == 1
INT32U drv_l2_hdmi_get_blacklist_flag()
{
    return hdmi_monitor_blacklist_flag;
}
#endif

static int hdmi_check_supported(unsigned char *edid, unsigned long *flag_supported, unsigned int offset)
{
    int i;
    const unsigned char *ptr;
    TIMING_LIMITS Limits={0};
    int flag_timing_limits = 0;

    DBG_PRINT("Manufacturer ID: 0x%02x%02x\r\n", edid[8], edid[9]);
    DBG_PRINT("Manufacturer Product Code: 0x%02x%02x\r\n", edid[11], edid[10]);
    DBG_PRINT("Serial Number: %d\r\n", (edid[12]<<24)|(edid[13]<<16)|(edid[14]<<8)|edid[15]);
    DBG_PRINT("Manufacture Date: %d %d\r\n", edid[0x10], edid[0x11]+1990);
    DBG_PRINT("EDID Version = %d.%d\r\n", edid[0x12], edid[0x13]);

#if MONITOR_BLOCK_LIST == 1
    //check black list
    for(i=0;i<MAX_HDMI_BLACKLIST_NUM;i++)
    {
        if(gp_strncmp( (INT8S*)(edid+8), hdmi_monitor_blacklist[i], 10) == 0)
        {
            hdmi_monitor_blacklist_flag |= 0x1<<i;
            DBG_PRINT("Black List ID %d\r\n",i);
            break;
        }
    }
#endif
    ptr = edid + offset;

    for(i=0;i<4;i++)
    {

        if( ptr[0] != 0 ||
                ptr[1] != 0) // check if timing descriptor
        {
            const VESA_E_EDID_TIMING_MODE *VesaTiming = (const VESA_E_EDID_TIMING_MODE*)ptr;
            FRAME_TIMING Timing;
			unsigned char HorizontalBlanking_upper = VesaTiming->HorizontalExtraInfo & 0x0F;
			unsigned char HorizontalAddressableVideo_upper = (VesaTiming->HorizontalExtraInfo & 0xF0)>>4;
			unsigned char VerticalBlanking_upper = VesaTiming->VerticalExtraInfo & 0x0F;
			unsigned char VerticalAddressableVideo_upper = (VesaTiming->VerticalExtraInfo & 0xF0)>>4;
			unsigned char VerticalSyncPulseWidth = VesaTiming->VerticalSync & 0x0F;
			unsigned char VerticalFrontPorch = (VesaTiming->VerticalSync & 0xF0)>>4;
			unsigned char VerticalSyncPulseWidth_upper = VesaTiming->HV_Info & 0x3;
			unsigned char VerticalFrontPorch_upper = (VesaTiming->HV_Info & 0x0C)>>2;
			unsigned char HorizontalSyncPulseWidth_upper = (VesaTiming->HV_Info & 0x30)>>4;
			unsigned char HorizontalFrontPorch_upper = (VesaTiming->HV_Info & 0xC0)>>6;

            Timing.PixelClock   = VesaTiming->PixelClock;

            Timing.hSize        = VesaTiming->HorizontalAddressableVideo | (HorizontalAddressableVideo_upper << 8);
            Timing.hBlanking    = VesaTiming->HorizontalBlanking | (HorizontalBlanking_upper << 8);
            Timing.hFrontPorch  = VesaTiming->HorizontalFrontPorch | (HorizontalFrontPorch_upper << 8);
            Timing.hPluseWidth  = VesaTiming->HorizontalSyncPulseWidth | (HorizontalSyncPulseWidth_upper << 8);

            Timing.vSize        = VesaTiming->VerticalAddressableVideo | (VerticalAddressableVideo_upper << 8);
            Timing.vBlanking    = VesaTiming->VerticalBlanking | (VerticalBlanking_upper << 8);
            Timing.vFrontPorch  = VerticalFrontPorch | (VerticalFrontPorch_upper << 4);
            Timing.vPluseWidth  = VerticalSyncPulseWidth | (VerticalSyncPulseWidth_upper << 4);

            if(i==0) DBG_PRINT("========= Preffered Timing =========\r\n");
            else     DBG_PRINT("============ Timing #%d =============\r\n", i);
            DBG_PRINT("Pixel clock = 0x%x\r\n",Timing.PixelClock);
            DBG_PRINT("           Addressable Blanking FrontPorch PulseWidth \r\n");
            DBG_PRINT("Horizontal    %4d        %4d     %4d      %4d    (pixels)\r\n",Timing.hSize,  Timing.hBlanking,   Timing.hFrontPorch, Timing.hPluseWidth);
            DBG_PRINT("Vertical      %4d        %4d     %4d      %4d    (lines)\r\n",Timing.vSize,  Timing.vBlanking,   Timing.vFrontPorch,     Timing.vPluseWidth);
        }
        else if(ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 0)
        {
            const VESA_E_EDID_DISPLAY_RANGE_LIMITS *VesaLimits = (const VESA_E_EDID_DISPLAY_RANGE_LIMITS*)ptr;
            int hMaxOffset = 0, hMinOffset = 0;
            int vMaxOffset = 0, vMinOffset = 0;
            switch(ptr[3])
            {
                case 0xFD:  // Display Range Limits
                    if(flag_timing_limits == 0)
                    {
                        if(VesaLimits->DisplayRangeLimitsOffsets & 1) vMinOffset = 255;
                        if(VesaLimits->DisplayRangeLimitsOffsets & 2) vMaxOffset = 255;
                        if(VesaLimits->DisplayRangeLimitsOffsets & 4) hMinOffset = 255;
                        if(VesaLimits->DisplayRangeLimitsOffsets & 8) hMaxOffset = 255;


                        Limits.maxPixelClock    = VesaLimits->MaximumPixelClock * 10;
                        Limits.hMaxRate         = VesaLimits->MaximumHorizontalRate + hMaxOffset;
                        Limits.hMinRate         = VesaLimits->MinimumHorizontalRate + hMinOffset;
                        Limits.vMaxRate         = VesaLimits->MaximumVerticalRate + vMaxOffset;
                        Limits.vMinRate         = VesaLimits->MinimumVerticalRate + vMinOffset;

                        DBG_PRINT("========= Timing Limits =========\r\n");
                        DBG_PRINT("Vertical Rate       = %3d ~ %3d Hz\r\n", Limits.vMinRate, Limits.vMaxRate);
                        DBG_PRINT("Horizontal Rate     = %3d ~ %3d kHz\r\n", Limits.hMinRate, Limits.hMaxRate);
                        DBG_PRINT("Maximum Pixel Clock = %3d MHz\r\n", Limits.maxPixelClock);
                        flag_timing_limits = 1;
                    }
                break;
                case 0xFC: //Display name
                    DBG_PRINT("========= Display Name =========\r\n");
                    DBG_PRINT("%s\r\n", (INT8S*)(ptr+5));
                break;
                case 0xFF: //Display serial number
                    DBG_PRINT("========= Display Serial Number: =========\r\n");
                    DBG_PRINT("%s\r\n", (INT8S*)(ptr+5));
                break;
                default:break;
            }
        }
        ptr += 18;
    }

    if(flag_timing_limits)
    {
        int val = 0;
        if(flag_supported)
        {
            for(i=0;i<sizeof(HdmiTiming)/sizeof(HdmiTiming[0]);i++)
            {
                if( HdmiTiming[i].PixelClock <= Limits.maxPixelClock * 100 &&
                        HdmiTiming[i].HFreq <= Limits.hMaxRate * 10 &&
                        HdmiTiming[i].HFreq >= Limits.hMinRate * 10 &&
                        HdmiTiming[i].VFreq <= Limits.vMaxRate &&
                        HdmiTiming[i].VFreq >= Limits.vMinRate)
                    val |= 1 << i;
            }
            *flag_supported = val;
        }

        return 0;
    }
    else
    {
        return -1;
    }
}

static INT32U ddc_init_flag = 0;
INT32S ddc_sccb_open(void)
{
    sccb_config_t cfg;

    if(ddc_init_flag == 0)
    {
    #if 1
        cfg.scl_port = IO_C12;
        cfg.scl_drv = IOC_DRV_30mA;
        cfg.sda_port = IO_C13;
        cfg.sda_drv = IOC_DRV_30mA;
    #else
	cfg.scl_port = IO_B4;
	cfg.scl_drv = IOB_DRV_30mA;
	cfg.sda_port = IO_B5;
	cfg.sda_drv = IOB_DRV_30mA;
    #endif
        cfg.pwdn_port = 0;
        cfg.pwdn_drv = 0;
        cfg.have_pwdn = 0;
        cfg.RegBits = 8;
        cfg.DataBits = 8;
        cfg.slaveAddr = HDMI_DDC_ID_0;
        cfg.timeout = 0x20000;
        cfg.clock_rate = 100;

        ddc_handle = drv_l2_sccb_open_ext(&cfg);

        if(ddc_handle == 0) {
            DBG_PRINT("ddc Sccb open fail.\r\n");
            return STATUS_FAIL;
        }

        cfg.slaveAddr = HDMI_SEGMENT_ID;

        ext_seg_handle = drv_l2_sccb_open_ext(&cfg);

        if(ext_seg_handle == 0) {
            DBG_PRINT("ext seg Sccb open fail.\r\n");
            if(ddc_handle)
                gp_free((void *)ddc_handle);

            return STATUS_FAIL;
        }
        ddc_init_flag = 1;
    }
	return STATUS_OK;
}

int hdmi_read_block(INT8U index, INT8U *p, INT16U length)
{
    return drv_l2_sccb_continue_read(ddc_handle,index,p,length);
}

int ext_seg_read_block(INT8U addr1, INT8U addr2, INT8U *p, INT16U length)
{
    return drv_l2_sccb_seg_p_edid(ext_seg_handle, ddc_handle, addr1, addr2, p, length);
}

int hdmi_read_EDID(INT8U *p)
{
    return drv_l2_sccb_continue_read(ddc_handle, 0x00, p, 128);
}

#if HDCP_SUPPORT == 1
#define HDCP_STACKSIZE                          8192
#define HDCP_QUEUE_MAX                          5
#define HDCP_KEY_CHECK                          1
#define HDCP_STOP_STATE                         2
#define TASK_EXIT_ACK_OK                        0x23242134
#define TASK_EXIT_STATE                         0x19921215

static osThreadId hdcp_id;
static xQueueHandle hdcp_process_queue = NULL;

/**************************************************************************
 *                              Queue Get/Post                            *
 **************************************************************************/
static INT32U process_queue_buf_get(xQueueHandle queue, INT32U wait)
{
    osEvent result;

    if(wait == 1)
        result = osMessageGet(queue, osWaitForever);
    else if(wait)
        result = osMessageGet(queue, wait);
    else
        result = osMessageGet(queue, 1);

    if(result.status != osEventMessage)
    {
    #if 0
        return PRCESS_DUMMY_ADDR;
    #else
        return 0;
    #endif
    }
    else
        return result.value.v;
}

static INT32S process_queue_buf_put(xQueueHandle queue, INT32U buf, INT32U wait)
{
    if(wait == 1)
        osMessagePut(queue, (INT32U)&buf, osWaitForever);
    else if(wait)
        osMessagePut(queue, (INT32U)&buf, wait);
    else
        osMessagePut(queue, (INT32U)&buf, 1);

    return 0;
}

static void hsmi_user_isr1(INT32U state)
{
    if(state & HDCP_128FRAME_ACTIVE)
    {
        process_queue_buf_put(hdcp_process_queue, HDCP_KEY_CHECK, 0);
    }
}

static INT32U hdcp_open_flag = 0;
static INT32U hdcp_task_flag = 0;
INT32S hdcp_sccb_open(void)
{
    sccb_config_t cfg;

    if(hdcp_open_flag == 0)
    {
    #if 1
        cfg.scl_port = IO_C12;
        cfg.scl_drv = IOC_DRV_30mA;
        cfg.sda_port = IO_C13;
        cfg.sda_drv = IOC_DRV_30mA;
    #else
        cfg.scl_port = IO_B4;
        cfg.scl_drv = IOB_DRV_30mA;
        cfg.sda_port = IO_B5;
        cfg.sda_drv = IOB_DRV_30mA;
    #endif
        cfg.pwdn_port = 0;
        cfg.pwdn_drv = 0;
        cfg.have_pwdn = 0;
        cfg.RegBits = 8;
        cfg.DataBits = 8;
        cfg.slaveAddr = HDMI_HDCP_ID_0;
        cfg.timeout = 0x20000;
        cfg.clock_rate = 100;

        hdcp_handle = drv_l2_sccb_open_ext(&cfg);

        if(hdcp_handle == 0) {
            DBG_PRINT("hdcp Sccb open fail.\r\n");
            hdcp_open_flag = 0;
            return STATUS_FAIL;
        }
        hdcp_open_flag = 1;
	}

	return STATUS_OK;
}

static int hdmi_write_HDCP_An(INT8U* An)
{
    DBG_PRINT("\r\nWrite An\r\n");
    return drv_l2_sccb_continue_write(hdcp_handle,0x18,An,8);
}

static int hdmi_write_HDCP_Aksv(INT8U* Aksv)
{
    DBG_PRINT("\r\nWrite AKsv\r\n");
    return drv_l2_sccb_continue_write(hdcp_handle,0x10,Aksv,5);
}

static int hdmi_write_HDCP_Ainfo(INT8U *p)
{
    return drv_l2_sccb_continue_write(hdcp_handle,0x15,p,1);
}

static int hdmi_read_HDCP_Ri(INT8U* Ri)
{
    return drv_l2_sccb_continue_read(hdcp_handle,0x8,Ri,2);
}

static int hdmi_read_HDCP_KSV_FIFO(INT8U *p, INT32U length)
{
#if 1
    INT32U i;
    INT32S ret;

    ret = drv_l2_sccb_continue_read(hdcp_handle, 0x43, p, length);
    if(ret == STATUS_OK)
    {
        for(i=0;i<length;i++)
        {
            if((i%5)==0)
                DBG_PRINT("\r\n0x%x ",i/5);
            DBG_PRINT("0x%x ",*(p+i));
        }
        DBG_PRINT("\r\n----\r\n");
    }

    return ret;
#else
    return drv_l2_sccb_continue_read(hdcp_handle, 0x43, p, length);
#endif
}

static int hdmi_read_HDCP_VH(INT8U *p)
{
    return drv_l2_sccb_continue_read(hdcp_handle, 0x20, p, 20);
}

static int hdmi_read_HDCP_bcap(INT8U *bcap)
{
    return drv_l2_sccb_continue_read(hdcp_handle,0x40,bcap,1);
}

static int hdmi_read_HDCP_Ainfo(INT8U *p)
{
    return drv_l2_sccb_continue_read(hdcp_handle,0x15,p,1);
}

static int hdmi_read_HDCP_bstatus(INT8U *bstatus)
{
    return drv_l2_sccb_continue_read(hdcp_handle,0x41,bstatus,2);
}

static int hdmi_read_HDCP_Bksv(INT8U *Bksv)
{
    return drv_l2_sccb_continue_read(hdcp_handle,0x00,Bksv,5);
}

static int hdmi_read_HDCP_reg(unsigned char *p)
{
    INT32U i;
    drv_l2_sccb_continue_read(hdcp_handle,0x00,p,128);
    for(i=0;i<128;i++)
    {
        if((i%16)==0)
            DBG_PRINT("\r\n%2x ",i/16);
        DBG_PRINT("%2x ",*(p+i));
    }
    DBG_PRINT("\r\n----\r\n");
    return 0;
}

INT32S drv_l2_hdmi_hdcp_retry(void)
{
	INT8U IsRxRepeater = 0;
	INT8U timeout = 0;
    INT8U HDCP_Ri[2];
    INT8U HDCP_An[8]={0x03,0x04,0x07,0x0C,0x13,0x1C,0x27,0x34};
    INT8U HDCP_An2[8]={0xe5,0x0f,0xd1,0x3a,0xa5,0x62,0x5e,0x44};
    INT8U HDCP_Aksv[5]={0xB7,0x03,0x61,0xf7,0x14};//its from customer key
    INT8U HDCP_bcap[1];
    INT8U HDCP_Bksv[5];
    INT8U HDCP_bstatus[2];
    INT8U *HDCP_KSV_FIFO;
    INT8U HDCP_VH[20];//={0xB2,0xEE,0xB1,0xD1,0x88};
    INT8U HDCP_reg[128];//={0xB2,0xEE,0xB1,0xD1,0x88};
    INT8U HDCP_Ainfo[1];
    INT16U TEMP_RI_TX, TEMP_RI_RX;
    INT16U cnt_ri_rdy = 0;
    INT32U read_HDCP_reg_flag = 0;

    drv_l1_hdmi_hdcp_enable(DISABLE, DISABLE);
    osDelay(10);

    //step 0: HDCP reset
    drv_l1_hdmi_hdcp_reset();
    osDelay(10);
/*
    HDCP_Ainfo[0] = 2;
    hdmi_write_HDCP_Ainfo(HDCP_Ainfo);
    osDelay(1);
    DBG_PRINT("Ainfo[W] = %x\r\n",HDCP_bcap[0]);
*/

    //step 1: read Bcaps
    hdmi_read_HDCP_bcap(HDCP_bcap);
	DBG_PRINT("bcap = %x\r\n",HDCP_bcap[0]);
	osDelay(1);
    if(HDCP_bcap[0]&0x40)
    {
		drv_l1_hdmi_hdcp_set_repeater_mode(ENABLE);
        IsRxRepeater = 1;
    }

    //step 2: read/write Ainfo
    hdmi_read_HDCP_Ainfo(HDCP_Ainfo);
    osDelay(1);
    DBG_PRINT("Ainfo[R] = %x\r\n",HDCP_bcap[0]);

    ///step 3: stop generate AN
    drv_l1_hdmi_hdcp_set_An_generate(DISABLE);

    //Step 4: Read AN
    drv_l1_hdmi_hdcp_read_An((INT16U *)&HDCP_An[0]);
    DBG_PRINT("An %x%x%x%x%x%x%x%x\r\n",HDCP_An[7],HDCP_An[6],HDCP_An[5],HDCP_An[4],HDCP_An[3],HDCP_An[2],HDCP_An[1],HDCP_An[0]);

    //step 5: Write An
	hdmi_write_HDCP_An(HDCP_An);
	osDelay(1);

    //step 6: Read/Write AKsv
    drv_l1_hdmi_hdcp_read_AKsv((INT8U *)&HDCP_Aksv[0]);
#if SP_HDCP_KEY// GP test only
    HDCP_Aksv[0] = setk[0];
    HDCP_Aksv[1] = setk[1];
    HDCP_Aksv[2] = setk[2];
    HDCP_Aksv[3] = setk[3];
    HDCP_Aksv[4] = setk[4];
#endif
    DBG_PRINT("Aksv %x%x%x%x%x\r\n",HDCP_Aksv[4],HDCP_Aksv[3],HDCP_Aksv[2],HDCP_Aksv[1],HDCP_Aksv[0]);
	hdmi_write_HDCP_Aksv(HDCP_Aksv);
    osDelay(1);

    //step 7: Read BKsv
	hdmi_read_HDCP_Bksv(HDCP_Bksv);
	DBG_PRINT("bksv = %02x%02x%02x%02x%02x\r\n",HDCP_Bksv[4],HDCP_Bksv[3],HDCP_Bksv[2],HDCP_Bksv[1],HDCP_Bksv[0]);
    osDelay(1);

    //step 8: Write BKsv
	//analyzer shown 88D1B1EEB2, read back Bksv = b2eeb1d188
	drv_l1_hdmi_hdcp_write_Bksv((INT16U *)&HDCP_Bksv[0]);

	//step 9: wait Ri Ready 0xD0D00040 [8]
	while(1)
    {
        TEMP_RI_TX = drv_l1_hdmi_hdcp_read_Ri();
        if(drv_l1_hdmi_hdcp_Ri_ready())
        {
            break;
        }
        cnt_ri_rdy++;
        osDelay(1);
    }
    osDelay(100);

    //step 10: Read Ri
    hdmi_read_HDCP_Ri(HDCP_Ri);
    DBG_PRINT("HDCP_Ri = [1]:%02x[0]:%02x\r\n",HDCP_Ri[1],HDCP_Ri[0]);
    osDelay(1);
    TEMP_RI_RX = (HDCP_Ri[1]<<8)|HDCP_Ri[0];

    //step 11: Check Ri, enable HDCP if they are the same
    DBG_PRINT("TEMP_RI_TX = 0x%x\r\n",TEMP_RI_TX);
    DBG_PRINT("TEMP_RI_RX = 0x%x\r\n",TEMP_RI_RX);
    if(TEMP_RI_RX == TEMP_RI_TX)
    {
        drv_l1_hdmi_inten_set(HDMI_INT_EN1, HDCP_128FRAME_ENABLE);
        drv_l1_hdmi_user_isr_register(1, hsmi_user_isr1);
        drv_l1_hdmi_hdcp_enable(ENABLE, 0);
        DBG_PRINT("HDCP retry enable !!! \r\n");
        return 0;
    }
    else
    {
        DBG_PRINT("HDCP retry R0 compared fail->>>R0_RX=0x%x, R0_TX=0x%x !!!\r\n",TEMP_RI_RX, TEMP_RI_TX);
        //drv_l1_hdmi_hdcp_enable(DISABLE);
        //drv_l1_disp_dev_en_set(TFT_2, 0);
        osDelay(16);
        process_queue_buf_put(hdcp_process_queue, HDCP_KEY_CHECK, 0);
        return -1;
    }
}

INT32S drv_l2_hdmi_hdcp_stop(void)
{
    if(hdcp_process_queue)
        process_queue_buf_put(hdcp_process_queue, HDCP_STOP_STATE, 0);

    return 0;
}

extern INT32U show_frame_stop(void);
extern INT32U HPD_Mode_Get(INT32U cnt);
static void hdcp_task_entry(void const *param)
{
    INT8U HDCP_Ri[2];
    INT16U TEMP_RI_TX, TEMP_RI_RX;
    INT32U i,time_out;
    INT32U process_buf;
    INT32S retry_flag;
    osEvent result;

    DBG_PRINT("hdcp_task_entry start \r\n");
    retry_flag = 0;

    while(1)
    {
        result = osMessageGet(hdcp_process_queue, osWaitForever);
        process_buf = result.value.v;
        if((result.status != osEventMessage) || !process_buf) {
            continue;
        }
        //DBG_PRINT("H");
        //DBG_PRINT("hdcp_task_task_buf = 0x%x\r\n", process_buf);

        switch(process_buf)
        {
            case TASK_EXIT_STATE:
                DBG_PRINT("[TASK_EXIT_STATE]: hdcp_id !!! \r\n");
                drv_l1_disp_dev_en_set(TFT_2, 0);
                osThreadTerminate(hdcp_id);
                break;

            case HDCP_STOP_STATE:
                DBG_PRINT("[HDCP_STOP_STATE] !!! \r\n");
                retry_flag = 0;
                drv_l1_disp_dev_en_set(TFT_2, 0);
                drv_l1_hdmi_hdcp_enable(DISABLE, DISABLE);
                drv_l1_hdmi_inten_set(HDMI_INT_EN1, 0);
                drv_l1_hdmi_user_isr_register(1, 0);
                break;

            case HDCP_KEY_CHECK:
                if(retry_flag < 0)
                    goto _HDCP_RETRY;

                //step 9: wait Ri Ready 0xD0D00040 [8]
                time_out = 0;
                for(i=0;i<50;i++)
                {
                    TEMP_RI_TX = drv_l1_hdmi_hdcp_read_Ri();
                    if(drv_l1_hdmi_hdcp_Ri_ready())
                    {
                        time_out = 0;
                        break;
                    }
                    time_out++;
                    osDelay(1);
                }

                if(time_out)
                {
                    DBG_PRINT("HDCP_Ri_ready Time = %d ms !!!\r\n", time_out);
                }
                else
                {
                    retry_flag = HPD_Mode_Get(3);
                    if(retry_flag == 0)
                        goto _HPD_EXIT;

                    //step 10: Read Ri
                    HDCP_Ri[0] = 0;
                    HDCP_Ri[1] = 0;
                    hdmi_read_HDCP_Ri(HDCP_Ri);
                    osDelay(1);
                    TEMP_RI_RX = (HDCP_Ri[1]<<8)|HDCP_Ri[0];

                    //step 11: Check Ri, enable HDCP if they are the same
                    if(TEMP_RI_RX != TEMP_RI_TX)
                    {
                        DBG_PRINT("HDCP R0 compared fail->>>R0_RX=0x%x, R0_TX=0x%x !!!\r\n",TEMP_RI_RX, TEMP_RI_TX);
                    _HDCP_RETRY:
                        retry_flag = 0;
                        osDelay(2000);
                        retry_flag = HPD_Mode_Get(3);
                        if(retry_flag == 0)
                            goto _HPD_EXIT;
                        //step 10: Read Ri
                        HDCP_Ri[0] = 0;
                        HDCP_Ri[1] = 0;
                        hdmi_read_HDCP_Ri(HDCP_Ri);
                        osDelay(1);
                        TEMP_RI_RX = (HDCP_Ri[1]<<8)|HDCP_Ri[0];
                        if(TEMP_RI_RX != TEMP_RI_TX)
                        {
                            osDelay(500);
                            retry_flag = HPD_Mode_Get(3);
                            if(retry_flag == 0)
                                goto _HPD_EXIT;
                            //step 10: Read Ri
                            HDCP_Ri[0] = 0;
                            HDCP_Ri[1] = 0;
                            hdmi_read_HDCP_Ri(HDCP_Ri);
                            osDelay(1);
                            TEMP_RI_RX = (HDCP_Ri[1]<<8)|HDCP_Ri[0];
                            if(TEMP_RI_RX != TEMP_RI_TX)
                            {
                            #if 1
                            _HPD_EXIT:
                                show_frame_stop();
                                drv_l1_hdmi_inten_set(HDMI_INT_EN1, 0);
                                drv_l1_hdmi_user_isr_register(1, 0);
                                drv_l1_hdmi_hdcp_enable(DISABLE, DISABLE);
                            #else
                                retry_flag = drv_l2_hdmi_hdcp_retry();
                            #endif
                            }
                            else
                                DBG_PRINT("!#");
                        }
                        else
                            DBG_PRINT("!#");
                    }
                    else
                        DBG_PRINT("#");
                }
                break;

            default:

                break;
         }
     }
}

static INT32U hdcp_error_flag = 0;
INT32S drv_l2_hdmi_hdcp_init(INT32U cnt)
{
	INT8U IsRxRepeater = 0;
	INT8U timeout = 0;
    INT8U HDCP_Ri[2];
    INT8U HDCP_An[8]={0x03,0x04,0x07,0x0C,0x13,0x1C,0x27,0x34};
    INT8U HDCP_An2[8]={0xe5,0x0f,0xd1,0x3a,0xa5,0x62,0x5e,0x44};
    INT8U HDCP_Aksv[5]={0xB7,0x03,0x61,0xf7,0x14};//its from customer key
    INT8U HDCP_bcap[1];
    INT8U HDCP_Bksv[5];
    INT8U HDCP_bstatus[2];
    INT8U *HDCP_KSV_FIFO;
    INT8U HDCP_VH[20];//={0xB2,0xEE,0xB1,0xD1,0x88};
    INT8U HDCP_VH_1[20];//={0xB2,0xEE,0xB1,0xD1,0x88};
    INT8U HDCP_reg[128];//={0xB2,0xEE,0xB1,0xD1,0x88};
    INT8U HDCP_Ainfo[1];
    INT16U TEMP_RI_TX, TEMP_RI_RX;
    INT16U cnt_ri_rdy = 0;
    INT32U read_HDCP_reg_flag = 0;
    INT32U i,j,state_flag,hdcp_error;

#if 0 // efuse test
    INT32U i,j,*ptr;
    INT32U *efuse_value;

    /* efuse layout:
        total 4096 bit are spilt to 8 banks, so each bank has 512 bit

        Bit Location	Description	            Size

        Bit0	        Security Enable	        1-bit
        Bit1	        Reserved	            1-bit
        Bit[7:2]	    LDO XCKGEN Trim value   6-bit
        Bit[119:8]	    Function/IOSC trim/XCK LDO Trim/DDR trim	112-bit
        Bit[127:120]	DDR_ZQ_RES_TRIM	        8-bit
        Bit[255:128]	repair for Bit[127:0]   128-bit
        Bit[383:256]	Unique ID	            128-bit
        Bit[511:384]	Reserved	            128-bit
        Bit[767:512]	Reserved	            256-bit
        Bit[1023:768]	Public Key	            256-bit
        Bit[3583:1024]	HDCP Key	            2560-bit
        Bit[4095:3584]	2 256-bit AES Key	    512-bit
    */
    DBG_PRINT("efuse data !!! \r\n");
#if 1
    efuse_value = (INT32U *)&setk[0]; /* efuse data buffer for each bank */
    i = sizeof(setk);
#else
    R_EFUSE_BACKDOOR_EN = 1;
    R_EFUSE_BANK = 0; /* switch bank */
    R_EFUSE_DATA_BUF0 = 0;
    R_EFUSE_BACKDOOR_KEY = 0x53083181;

    ptr = (INT32U *)&setk[0]; /* efuse data buffer for each bank */
    for(i=0; i<128; i++) { /* 32*128 = 4096 bit */
        efuse_value[i] = *ptr;
        ptr++;
    }

    for (j=0;j<5;j++) {
        R_EFUSE_BANK = (j+2); /* switch bank */
        ptr = (INT32U *)R_EFUSE_DATA_BASE; /* efuse data buffer for each bank */
        for (i=0;i<16;i++) {
#if 1
            *ptr = efuse_value[j*16 + i];
#else
            *ptr = setk[j*16 + i];
#endif
            ptr++;
        }
        R_EFUSE_BACKDOOR_KEY = 0x53083181;
    }
#endif
    R_EFUSE_BACKDOOR_EN = 0;

    /* read all efuse data to check value */
    for(j=0;j<5;j++)
    {
        R_EFUSE_BANK = (j+2);
        ptr = (INT32U *)R_EFUSE_DATA_BASE;
        for(i=0;i<16;i++)
        {
            //DBG_PRINT("efuse data bank_%d, buf_%d = 0x%x\r\n", j, i, *ptr);
            if (*ptr != efuse_value[j*16 + i]) {
                if((j<=4)&&(i<7))
                    DBG_PRINT(" !!!Fail buf_%, read = 0x%x, expect = 0x%x\r\n", i, *ptr, efuse_value[i]);
            }
            ptr++;
        }
    }
#endif

#if 0
    //R_HDMICONFIG |= (0x1<<15)|(0x1<<14);
    ////CEC_Tx
    R_FUNPOS2 &= ~(0x7<<16);//IOC0 as CEC
    R_FUNPOS2 |= ((0x1<<17)|(0x1<<16)); // CEC enable

     //0.set 0xd0d001C0[10:0] =11¡¦hFA //cec_divisor how to decide this value?
    (*((volatile INT32U *) 0xd0d001C0)) = 0x548;
    //1.set 0xd0d001CC[0]    //cec initial enable :cec_out_en

    //2.set 0xd0d001D0=16'h241b    //initial int_en
    (*((volatile INT32U *) 0xd0d001D0)) = 0x241;
    //3.set 0xd0d001E4=16'h0700    //cec follower: [10]cec_in_en[9]cec_header_ack[8]cec_data_ack
    (*((volatile INT32U *) 0xd0d001E4)) = 0x700;
    //4.set 0xd0d001E8=16'h0300    //[9]sram_en[8]cec_rcv_frm_en
    (*((volatile INT32U *) 0xd0d001E8)) = 0x0300;
    //5.set 0xd0d001E0=0324 //[11:8]cec_addr [3:0]debounce_thd
    (*((volatile INT32U *) 0xd0d001E0)) = 0x124;



    //6.set 0xd0d001CC[0]=1'h1    //cec initial enable :cec_out_en
    (*((volatile INT32U *) 0xd0d001CC)) |= 0x1<<0;
    //7.set 0xd0d001D4[7:0]=8'h23
    (*((volatile INT32U *) 0xd0d001D4)) = 0x2300;
    //8.set 0xd0d00200[15:0]=16'h8331
    (*((volatile INT32U *) 0xd0d00200)) = 0x8310;//byte2:op code, byte1: addr[initiaior][rcv]
    //9.set 0xd0d00204[15:0]=16'h0034
    (*((volatile INT32U *) 0xd0d00204)) = 0x0034;
    //10.set 0xd0d001D4[1]=1'h1 //cec_tx start
    (*((volatile INT32U *) 0xd0d001D4)) |= 0x1<<1;
#endif

    if(0)//(drv_l1_hdmi_hot_plug_detect_get(1) == 0)
    {
        DBG_PRINT("HDMI Hot Plug Detect Fail !!!\r\n");

        return -1;
    }
    //drv_l1_hdmi_hot_plug_detect_get(1);

    hdcp_sccb_open();

    if(read_HDCP_reg_flag)
        hdmi_read_HDCP_reg((unsigned char *)&HDCP_reg[0]);

    state_flag = 0;
    for(i=0;i<cnt;i++)
    {
        //step 0: HDCP reset
        drv_l1_hdmi_hdcp_reset();
        osDelay(10);

    /*
        HDCP_Ainfo[0] = 2;
        hdmi_write_HDCP_Ainfo(HDCP_Ainfo);
        osDelay(1);
        DBG_PRINT("Ainfo[W] = %x\r\n",HDCP_Ainfo[0]);
    */

        //step 1: read Bcaps
	hdmi_read_HDCP_bcap(HDCP_bcap);
	DBG_PRINT("bcap = %x\r\n",HDCP_bcap[0]);
        osDelay(1);
    if(HDCP_bcap[0]&0x40)
    {
		drv_l1_hdmi_hdcp_set_repeater_mode(ENABLE);
        IsRxRepeater = 1;
    }

        //step 2: Read Ainfo
    hdmi_read_HDCP_Ainfo(HDCP_Ainfo);
        DBG_PRINT("Ainfo %x\r\n",HDCP_Ainfo[0]);
        osDelay(1);

        ///step 3: stop generate AN
    drv_l1_hdmi_hdcp_set_An_generate(DISABLE);

        //Step 4: Read AN
    drv_l1_hdmi_hdcp_read_An( (INT16U*)&HDCP_An[0]);
    DBG_PRINT("An %x%x%x%x%x%x%x%x\r\n",HDCP_An[7],HDCP_An[6],HDCP_An[5],HDCP_An[4],HDCP_An[3],HDCP_An[2],HDCP_An[1],HDCP_An[0]);

    //step 5: Write An
	hdmi_write_HDCP_An(HDCP_An);
	osDelay(1);

        //step 6: Read/Write AKsv
        drv_l1_hdmi_hdcp_read_AKsv((INT8U *)&HDCP_Aksv[0]);
    #if SP_HDCP_KEY// GP test only
    HDCP_Aksv[0] = setk[0];
    HDCP_Aksv[1] = setk[1];
    HDCP_Aksv[2] = setk[2];
    HDCP_Aksv[3] = setk[3];
    HDCP_Aksv[4] = setk[4];
#endif
        DBG_PRINT("Aksv %x%x%x%x%x\r\n",HDCP_Aksv[4],HDCP_Aksv[3],HDCP_Aksv[2],HDCP_Aksv[1],HDCP_Aksv[0]);
	hdmi_write_HDCP_Aksv(HDCP_Aksv);
        osDelay(1);

        hdcp_error = HPD_Mode_Get(3);
        if(hdcp_error == 0)
            goto _HPD_DISABLE;

    //step 7: Read BKsv
	hdmi_read_HDCP_Bksv(HDCP_Bksv);
        DBG_PRINT("bksv = %02x%02x%02x%02x%02x\r\n",HDCP_Bksv[4],HDCP_Bksv[3],HDCP_Bksv[2],HDCP_Bksv[1],HDCP_Bksv[0]);
        osDelay(1);
        hdcp_error = 0;
        for(i=0;i<5;i++)
        {
            if(HDCP_Bksv[i] == 0)
                hdcp_error++;
            else if(HDCP_Bksv[i] == 1)
                hdcp_error++;
        }

        if(hdcp_error >= 4)
        {
            for(j=0;j<2;j++)
            {
	osDelay(1);
                //step 7: Read BKsv
                hdmi_read_HDCP_Bksv(HDCP_Bksv);
	DBG_PRINT("bksv = %02x%02x%02x%02x%02x\r\n",HDCP_Bksv[4],HDCP_Bksv[3],HDCP_Bksv[2],HDCP_Bksv[1],HDCP_Bksv[0]);
    osDelay(1);
                hdcp_error = 0;
                for(i=0;i<5;i++)
                {
                    if(HDCP_Bksv[i] == 0)
                        hdcp_error++;
                    else if(HDCP_Bksv[i] == 0xFF)
                        hdcp_error++;
                }
                if(hdcp_error == 0)
                    break;
            }
            if(hdcp_error >= 4)
                goto _HDCP_DISABLE;
        }

        hdmi_read_HDCP_VH(HDCP_VH);
        for(j=0;j<5;j++)
        {
            DBG_PRINT("VH = %02x%02x%02x%02x\r\n",HDCP_VH[(j+3)],HDCP_VH[(j+2)],HDCP_VH[(j+1)],HDCP_VH[(j+0)]);
        }
        osDelay(1);

    //step 8: Write BKsv
	//analyzer shown 88D1B1EEB2, read back Bksv = b2eeb1d188
	drv_l1_hdmi_hdcp_write_Bksv( (INT16U*)&HDCP_Bksv[0]);

        // test
        hdmi_read_HDCP_bstatus(HDCP_bstatus);
        DBG_PRINT("bstatus = %02x%02x\r\n",HDCP_bstatus[1],HDCP_bstatus[0]);
        osDelay(1);

	//step 9: wait Ri Ready 0xD0D00040 [8]
	while(1)
    {
        TEMP_RI_TX = drv_l1_hdmi_hdcp_read_Ri();
        if(drv_l1_hdmi_hdcp_Ri_ready())
        {
            break;
        }
        cnt_ri_rdy++;
        osDelay(1);
    }

        if(hdcp_error_flag)
            osDelay(150);
        else
            osDelay(100);

        hdcp_error = HPD_Mode_Get(3);
        if(hdcp_error == 0)
            goto _HPD_DISABLE;

    //step 10: Read Ri
    hdmi_read_HDCP_Ri(HDCP_Ri);
        DBG_PRINT("HDCP_Ri = [1]:%02x[0]:%02x\r\n",HDCP_Ri[1],HDCP_Ri[0]);
    osDelay(1);
        TEMP_RI_RX = (HDCP_Ri[1]<<8)|HDCP_Ri[0];

        //step 11: Check Ri, enable HDCP if they are the same
        DBG_PRINT("TEMP_RI_TX = 0x%x\r\n",TEMP_RI_TX);
        DBG_PRINT("TEMP_RI_RX = 0x%x\r\n",TEMP_RI_RX);

        if(IsRxRepeater && (TEMP_RI_RX != TEMP_RI_TX))
        {
            IsRxRepeater = 0;
            for(j=0;j<500;j++)
            {
                hdmi_read_HDCP_bcap(HDCP_bcap);
                if(HDCP_bcap[0]&0x20)
                {
                    IsRxRepeater = 1;
                    break;
                }
                osDelay(1);
            }

            hdcp_error = HPD_Mode_Get(3);
            if(hdcp_error == 0)
                goto _HPD_DISABLE;

            if(IsRxRepeater)
                goto _RXREPEATER_START;
            else
            {
                hdmi_read_HDCP_bstatus(HDCP_bstatus);
                DBG_PRINT("bstatus = %02x%02x\r\n",HDCP_bstatus[1],HDCP_bstatus[0]);
                osDelay(1);
                if(HDCP_bstatus[0]&0x7f)
                {
                    IsRxRepeater = 1;
                    goto _RXREPEATER_START;
                }
                else
                {
                    HDCP_KSV_FIFO = (INT8U *)&HDCP_reg[0];
                    hdmi_read_HDCP_KSV_FIFO(HDCP_KSV_FIFO, 60);
                    hdcp_error = 0;
                    for(j=0;j<60;j++)
                    {
                        if(HDCP_KSV_FIFO[j] == 0)
                            hdcp_error++;
                    }

                    if(hdcp_error == 0)
                    {
                        IsRxRepeater = 1;
                        goto _RXREPEATER_START;
                    }
                    else
                    {
                        osDelay(600);
                        hdmi_read_HDCP_VH(HDCP_VH_1);
                        hdcp_error = 0;
                        for(j=0;j<5;j++)
                        {
                            if(HDCP_VH_1[i] == 0)
                                hdcp_error++;
                            DBG_PRINT("VH = %02x%02x%02x%02x\r\n",HDCP_VH_1[(j+3)],HDCP_VH_1[(j+2)],HDCP_VH_1[(j+1)],HDCP_VH_1[(j+0)]);
                        }
                        if(hdcp_error == 0)
                        {
                            IsRxRepeater = 1;
                            goto _RXREPEATER_START;
                        }
                    }
                }
            }
        }
        else if(IsRxRepeater && (TEMP_RI_RX == TEMP_RI_TX))
        {
            IsRxRepeater = 0;
            for(j=0;j<500;j++)
            {
                hdmi_read_HDCP_bcap(HDCP_bcap);
                if(HDCP_bcap[0]&0x20)
                {
                    IsRxRepeater = 1;
                    break;
                }
    osDelay(1);
            }

            if(IsRxRepeater == 0)
            {
                hdmi_read_HDCP_bstatus(HDCP_bstatus);
                DBG_PRINT("bstatus = %02x%02x\r\n",HDCP_bstatus[1],HDCP_bstatus[0]);
                osDelay(1);
                if(HDCP_bstatus[0]&0x7f)
                    IsRxRepeater = 1;
                else
                {
                    HDCP_KSV_FIFO = (INT8U *)&HDCP_reg[0];
                    hdmi_read_HDCP_KSV_FIFO(HDCP_KSV_FIFO, 60);
                    hdcp_error = 0;
                    for(j=0;j<60;j++)
                    {
                        if(HDCP_KSV_FIFO[j] == 0)
                            hdcp_error++;
                    }
                    if(hdcp_error == 0)
                      IsRxRepeater = 1;
                    else
                    {
                        osDelay(600);
                        hdmi_read_HDCP_VH(HDCP_VH_1);
                        hdcp_error = 0;
                        for(j=0;j<5;j++)
                        {
                            if(HDCP_VH_1[i] == 0)
                                hdcp_error++;
                            DBG_PRINT("VH = %02x%02x%02x%02x\r\n",HDCP_VH_1[(j+3)],HDCP_VH_1[(j+2)],HDCP_VH_1[(j+1)],HDCP_VH_1[(j+0)]);
                        }
                        if(hdcp_error == 0)
                          IsRxRepeater = 1;
                    }
                }
            }

        _RXREPEATER_START:
            hdcp_error = HPD_Mode_Get(3);
            if(hdcp_error == 0)
                goto _HPD_DISABLE;

            if(IsRxRepeater)
            {
                hdmi_read_HDCP_bstatus(HDCP_bstatus);
                DBG_PRINT("bstatus = %02x%02x\r\n",HDCP_bstatus[1],HDCP_bstatus[0]);
                osDelay(1);
                if(1)//(HDCP_bstatus[0]&0x8)
                {
                    INT8U device_cnt = ((HDCP_bstatus[0]&~0x80)*5);
                    INT8U device_depth = ((HDCP_bstatus[1]&~0x8)*600);

                    HDCP_KSV_FIFO = (INT8U *)&HDCP_reg[0];
                    if((HDCP_bstatus[0]&0x80)||(HDCP_bstatus[1]&0x8))
                    {
                        IsRxRepeater = 0xFF;
                        goto _KSV_DEVS_EXIT;
                    }

                    if(device_cnt == 0)
                    {
                        IsRxRepeater = 0xFF;
                        goto _KSV_DEVS_EXIT;
                    }

                    if(device_cnt>128)
                        device_cnt = 128;

                    hdmi_read_HDCP_KSV_FIFO(HDCP_KSV_FIFO, device_cnt);
                    hdcp_error = 0;
                    for(j=0;j<device_cnt;j++)
                    {
                        if(HDCP_KSV_FIFO[j] == 0)
                            hdcp_error++;
                    }

                    osDelay(device_depth);
                    if(hdcp_error >= (device_cnt-1))
                        IsRxRepeater = 0xFF;
                    else
                    {
                        hdmi_read_HDCP_bstatus(HDCP_bstatus);
                        DBG_PRINT("bstatus = %02x%02x\r\n",HDCP_bstatus[1],HDCP_bstatus[0]);
                        osDelay(1);
                        if(HDCP_bstatus[0]&0x8)
                        {
                            hdmi_read_HDCP_VH(HDCP_VH_1);
                            hdcp_error = 0;
                            for(j=0;j<5;j++)
                            {
                                if(HDCP_VH_1[i] == 0)
                                    hdcp_error++;
                                DBG_PRINT("VH = %02x%02x%02x%02x\r\n",HDCP_VH_1[(j+3)],HDCP_VH_1[(j+2)],HDCP_VH_1[(j+1)],HDCP_VH_1[(j+0)]);
                            }
                            osDelay(1);
                            if(hdcp_error >= (5-1))
                                IsRxRepeater = 0xFF;
                        }
                    }
                }
             //_KSV_EXIT:
                //step 10: Read Ri
                hdmi_read_HDCP_Ri(HDCP_Ri);
                DBG_PRINT("HDCP_Ri = [1]:%02x[0]:%02x\r\n",HDCP_Ri[1],HDCP_Ri[0]);
                osDelay(1);
    TEMP_RI_RX = (HDCP_Ri[1]<<8) | HDCP_Ri[0];

    //step 11: Check Ri, enable HDCP if they are the same
    DBG_PRINT("TEMP_RI_TX = 0x%x\r\n",TEMP_RI_TX);
                DBG_PRINT("TEMP_RI_RX = 0x%x\r\n",TEMP_RI_RX);
            }
            else
                IsRxRepeater = 0xFF;
        }

        if(IsRxRepeater == 0xFF)
    {
        _KSV_DEVS_EXIT:
            IsRxRepeater = 0;
    }
    else
            IsRxRepeater = 0x81;

        if((TEMP_RI_RX == TEMP_RI_TX) && (IsRxRepeater == 0x81))
        {
            osThreadDef_t hdcp_task = {"hdcp_task", hdcp_task_entry, osPriorityNormal, 1, HDCP_STACKSIZE};
            osMessageQDef_t disp_q = {HDCP_QUEUE_MAX, sizeof(INT32U), 0};

            if(hdcp_task_flag == 0)
    {
                if(hdcp_process_queue == NULL)
                {
                    hdcp_process_queue = osMessageCreate(&disp_q, NULL);
                    if(!hdcp_process_queue)
                    {
                        DBG_PRINT("hdcp_process_queue error\r\n");
                        while(1);
                    }
                    else
                        DBG_PRINT("hdcp_process_queue = 0x%x\r\n", hdcp_process_queue);
                }

                hdcp_id = osThreadCreate(&hdcp_task, (void *)NULL);
                if(hdcp_id == 0) {
                    DBG_PRINT("hdcp_task error\r\n");
        return -1;
    }
                else
                    osDelay(100);
                hdcp_task_flag = 1;
            }
            drv_l1_hdmi_inten_set(HDMI_INT_EN1, HDCP_128FRAME_ENABLE);
            drv_l1_hdmi_user_isr_register(1, hsmi_user_isr1);
            drv_l1_hdmi_hdcp_enable(ENABLE, 1);
            state_flag = 0;
            DBG_PRINT("HDCP enable !!! \r\n");
            break;
        }
        else
        {
            DBG_PRINT("HDCP R0 compared fail->>>R0_RX=0x%x, R0_TX=0x%x !!!\r\n",TEMP_RI_RX, TEMP_RI_TX);
         _HDCP_DISABLE:
            drv_l1_hdmi_hdcp_enable(DISABLE, DISABLE);
            //drv_l1_disp_dev_en_set(TFT_2, 0);
            state_flag++;
            hdcp_error_flag = 1;
            osDelay(300);
        }
    }

    hdcp_error_flag = 0;
    if(state_flag)
        return -1;
    else
        return 0;

_HPD_DISABLE:
    return -1;
}
#endif

//HDMI initial at drv_l2_display.c and check EDID from TV/Monitor then get supported format
//drv_l2_hdmi_set_video() set the target resolution, and check with supported format
//and final set the HDMI/DVI timing
void drv_l2_hdmi_set_video(INT8U resolution)
{
    g_hdmi_video_format = resolution;
}

INT8U drv_l2_hdmi_get_video(void)
{
    DBG_PRINT("video_format=%d\r\n",g_hdmi_video_format);
    DBG_PRINT("supported_format=%d\r\n",g_hdmi_video_support_format);
    if(g_hdmi_video_format > g_hdmi_video_support_format)
        g_hdmi_video_format = g_hdmi_video_support_format;
    return g_hdmi_video_format;
}

INT8U drv_l2_hdmi_get_supported_resolution(void)
{
    return g_hdmi_video_support_format;
}

void drv_l2_hdmi_set_supported_format(INT8U resolution)
{
    g_hdmi_video_support_format = resolution;
}

void drv_l2_hdmi_set_audio(INT8U sample_rate)
{
    g_hdmi_audio_format = sample_rate;
}

INT8U drv_l2_hdmi_get_audio(void)
{
    DBG_PRINT("audio_format=%d\r\n",g_hdmi_audio_format);
    DBG_PRINT("hdmi_mode=%d\r\n",g_hdmi_mode);
    if(g_hdmi_mode == DEVICE_DVI)
        g_hdmi_audio_format = HDMI_AUD_OFF;

    return g_hdmi_audio_format;
}

INT32U drv_l2_hdmi_get_cec_phy_addr(void)
{
    return g_cec_phyical_addr;
}

INT32U drv_l2_check_EDID(void)
{
	INT32U	i, j, k, t;
	INT8U	*p_extb_buffer = NULL;
	INT8U	*ex;
	INT8U	DTD_offset;
	INT8U	find_offset;
	INT8U	block_tag;
	INT8U	block_length;
    INT8U	edid_buf[128];
#if EDID_DETECT_FORMAT == 1
    unsigned long flag = 0;
#endif

	g_hdmi_mode = DEVICE_DVI;
	if(ddc_sccb_open() == STATUS_OK)
	{
		if(hdmi_read_block(0, edid_buf, 128) == STATUS_OK)
		{
			DBG_PRINT( "hdmi_read_block() STATUS_OK\r\n" );
			if( (edid_buf[0] != 0x00) || (edid_buf[1] != 0xff) || (edid_buf[2] != 0xff)|| (edid_buf[3] != 0xff)
			 || (edid_buf[4] != 0xff) || (edid_buf[5] != 0xff) || (edid_buf[6] != 0xff)|| (edid_buf[7] != 0x00) )
			{
				DBG_PRINT( "EDID HEADER ERROR\r\n" );
				return STATUS_FAIL;
			}
#if EDID_DETECT_FORMAT == 1
            hdmi_check_supported(edid_buf, &flag, 0x36);
            if (flag == 0)
                hdmi_check_supported(edid_buf, &flag, 0x35);
            if (flag&HDMI_720X480P60_BIT) {
                DBG_PRINT("HDMI_720X480P60_BIT\r\n");
            }
            if (flag&HDMI_1280X720P60_BIT) {
                DBG_PRINT("HDMI_1280X720P60_BIT\r\n");
            }
            if (flag == 0) {
                DBG_PRINT("DDC doesn't contain Monitor Range Limits Information\r\n");
            }
#endif

#if DBG_EDID_BLOCK == 1//print EDID block
            for( i = 0; i < 128; i++ ) {
                if(i%8 == 0)
                    DBG_PRINT("\r\n");
                DBG_PRINT("%02X ",edid_buf[i]);
            }
            DBG_PRINT("\r\n-----\r\n");
#endif
			{
				INT8U	sum = 0;
				for( i = 0; i < 127; i++ )
					sum += edid_buf[i];
				DBG_PRINT( "CHECKSUM %02x\r\n", edid_buf[127] );
				sum += edid_buf[127];
				if( sum != 0x00 )
				{
					DBG_PRINT( "EDID CHECKSUM ERROR\r\n" );
					return STATUS_FAIL;
				}
			}

			{
				INT8S chk = FALSE;

				for( t = 54; t < 126; t+=18 )
				{
					for( i = 0; i < 18; i++ )
						DBG_PRINT( " %02x", edid_buf[t+i] );
					DBG_PRINT( "\r\n" );
					if( (edid_buf[t] == 0x00) && (edid_buf[t+1] == 0x00) && (edid_buf[t+2] == 0x00) && (edid_buf[t+4] == 0x00) )
					{
						;
					}
					else
					{
						if( !chk )
						{
							INT16U	ha, hb, va, vb;
							ha = (edid_buf[t+2])|(((INT16U)edid_buf[t+4]&0xf0)<<4);
							hb = (edid_buf[t+3])|(((INT16U)edid_buf[t+4]&0x0f)<<8);
							va = (edid_buf[t+5])|(((INT16U)edid_buf[t+7]&0xf0)<<4);
							vb = (edid_buf[t+6])|(((INT16U)edid_buf[t+7]&0x0f)<<8);
							DBG_PRINT( " => %d %d %d %d\r\n", ha, hb, va, vb );
							//possible720P = FALSE;
							if( (ha >= 1280) && (va >= 720) ) {
								//possible720P = TRUE;
								drv_l2_hdmi_set_supported_format(HDMI_VID_720P);
                                chk = TRUE;
							}
							if( (ha >= 1920) && (va >= 1080) ) {
								//possible720P = TRUE;
								drv_l2_hdmi_set_supported_format(HDMI_VID_1080P);
                                chk = TRUE;
							}
						}//end !chk
					}
				}
			}

			DBG_PRINT("number of extend block 0x%x\r\n", edid_buf[126] );
			// check extend block table // 2 not allow
			if( (edid_buf[126] != 0) && (edid_buf[126] != 2) )
			{
                p_extb_buffer = (INT8U *)gp_malloc_align(128 * edid_buf[126], 32);
                if(p_extb_buffer == NULL)
				{
                    DBG_PRINT( "HDMI extend block allocate fail\r\n" );
                    return STATUS_FAIL;
                }
				// HDMI Failed item 7-1
				if( edid_buf[126] > 2 ) {
                    for(i = 1 ; i <= edid_buf[126] ; ++i )
                    {
                        INT8U	adr1, adr2;
                        adr1 = i >> 1;
                        adr2 = (i & 0x01) << 7;		// Base Address
                        if( ext_seg_read_block( adr1, adr2, (INT8U *)(p_extb_buffer + ((i - 1) * 128)), 128 ) != STATUS_OK )
                        {
                            DBG_PRINT( "ext_seg_read_block fail\r\n" );
                            gp_free( (void *)p_extb_buffer );
                            return STATUS_FAIL;
                        }
                    }
				} else {
                    if(hdmi_read_block(0x80, p_extb_buffer, 128) != STATUS_OK)
                    {
                        DBG_PRINT( "hdmi_read_block fail\r\n" );
                        gp_free( (void *)p_extb_buffer );
                        return STATUS_FAIL;
                    }
				}
				#if DBG_EDID_BLOCK == 1//print EDID block
					for( j = 0; j < edid_buf[126]; j++) {
                        for( i = 0; i < 128; i++ ) {

                            if(i%8 == 0)
                                DBG_PRINT("\r\n");
                            DBG_PRINT("%02X ", *(p_extb_buffer+i+j*128) );
                        }
                        DBG_PRINT("\r\n-----\r\n");
                    }
                    DBG_PRINT("\r\n-----\r\n");
                #endif

				ex = p_extb_buffer;
				for( i = 0 ; i < edid_buf[126] ; ++i )
				{
					if( *(ex + 0) == 0x02 )
					{
                        if( *(ex + 1) == 0x03 ) { //version 3
                            DTD_offset = *(ex + 2);
                            find_offset = 4;	//first block
                            while( find_offset < DTD_offset )
                            {
                                block_tag = (*(ex + find_offset) & 0xE0) >> 5;
                                block_length = (*(ex + find_offset) & 0x1F) + 1;
                                DBG_PRINT( "[%02d][%02x %x %02x]", find_offset, *(ex + find_offset), block_tag, block_length );

                                switch( block_tag )
                                {
                                    case 2:		// video data block
                                        DBG_PRINT("video DB >");
                                            {
                                                INT8U VIC_data, prefer;
                                                for(k=1;k<block_length+1;k++)
                                                {
                                                    VIC_data = p_extb_buffer[find_offset+k]&0x7F;
                                                    prefer = p_extb_buffer[find_offset+k]&0x80;
                                                    DBG_PRINT("0x%x ",VIC_data);
                                                    if(prefer == 0x80)
                                                        DBG_PRINT("*");
                                                    if(VIC_data == 0x2)
                                                        DBG_PRINT("[480P@59.94Hz]");
                                                    if(VIC_data == 0x4) {
                                                        DBG_PRINT("[720P@60Hz]");
                                                        //possible720P = TRUE;
                                                    }
                                                }
                                            }
                                            DBG_PRINT("\r\n");
                                        break;
                                    case 1:		// Audio data block
                                        DBG_PRINT( "Audio DB\r\n" );
                                        break;
                                    case 4:		// speaker allocatn data block
                                        DBG_PRINT( "speaker DB\r\n" );
                                        break;
                                    case 3:		// VSDB
                                        DBG_PRINT( "VSDB\r\n" );
                                        //for CEC
                                        g_cec_phyical_addr = ((INT32U)*(ex + (find_offset + 4)) << 8) | (INT32U)*(ex + (find_offset + 5));// physical address
                                        // HDMI Failed item 7-33
                                        // check VSDB 0x000C03 ==> HDMI including the IEEE registered, registered identifier: 0x000C03
                                        if( (*(ex + (find_offset + 1)) == 0x03) &&
                                            (*(ex + (find_offset + 2)) == 0x0C) &&
                                            (*(ex + (find_offset + 3)) == 0x00) &&
                                            (block_length >= 6) )
                                        {
                                            g_hdmi_mode = DEVICE_HDMI;
                                        }
                                        break;
                                    case 7:		// colorimetry/video cap
                                        DBG_PRINT( "COL/VIDCAP\r\n" );
                                        break;
                                    default:
                                        DBG_PRINT( "unknown\r\n" );
                                        break;
                                }
                                find_offset += block_length;
                            }
						} else if( *(ex + 1) == 0x01 ) { //version 1

                            INT16U	ha, hb, va, vb, dtd_str, t ;
                            dtd_str = *(ex + 2);
                            if(dtd_str >= 4) {
                                for(t = dtd_str; t<127; t+=18) {
                                    ha = (*(ex+2+t))|(((*(ex+4+t))&0xf0)<<4);
                                    hb = (*(ex+3+t))|(((*(ex+4+t))&0x0f)<<8);
                                    va = (*(ex+5+t))|(((*(ex+7+t))&0xf0)<<4);
                                    vb = (*(ex+6+t))|(((*(ex+7+t))&0x0f)<<8);
                                    DBG_PRINT( " ==> %d %d %d %d\r\n", ha, hb, va, vb );
                                    //possible720P = FALSE;
                                    if( (ha >= 1280) && (va >= 720) ) {
                                        drv_l2_hdmi_set_supported_format(HDMI_VID_720P);
                                        //chk = TRUE;
                                    }
                                    if( (ha >= 1920) && (va >= 1080) ) {
                                        drv_l2_hdmi_set_supported_format(HDMI_VID_1080P);
                                        //chk = TRUE;
                                    }
                                }
							}
						}
					}//
					ex += 128;	// next block
				}
				if( p_extb_buffer )
					gp_free( (void*)p_extb_buffer );
			}
			return STATUS_OK;
		}
	}
	return STATUS_FAIL;
}

#endif //#if (defined(LCD_DRV_EN_HDMI) && LCD_DRV_EN_HDMI)
