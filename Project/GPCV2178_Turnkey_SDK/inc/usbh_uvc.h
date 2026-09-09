#ifndef __USBH_UVC_H__
#define __USBH_UVC_H__

#define UVC_FUNC_CTRL				0
//#define USB_DEBUG
#ifndef UVC_USING_YUV_FORMAT
#define UVC_USING_YUV_FORMAT        0
#endif

#define UVC_YUV_FORMAT_RECORD       0

#if UVC_USING_YUV_FORMAT == 1
#define FRAME_WIDTH                           640
#define FRAME_HEIGHT                          480

#define sFRAME_WIDTH                          640
#define sFRAME_HEIGHT                         480
#else
#define FRAME_WIDTH                          1280//1920
#define FRAME_HEIGHT                          720//1080

#define sFRAME_WIDTH                          640//1280
#define sFRAME_HEIGHT                         480//720
#endif

#define UVC_JPGDEC_BUF_MQUEUE_MAX 12//6
#define UVC_JPGSTDEC_QUEUE_MAX    12
#define UVC_JPGDEC_BUF_SIZE (150*1024)



#define BACK_SENSOR_FRAME_RATE_5     2000000
#define BACK_SENSOR_FRAME_RATE_10    1000000
#define BACK_SENSOR_FRAME_RATE_15    666666
#define BACK_SENSOR_FRAME_RATE_20    500000
#define BACK_SENSOR_FRAME_RATE_24    0x65B9A
#define BACK_SENSOR_FRAME_RATE_25    0x61A80//0x07A120////400000
#define BACK_SENSOR_FRAME_RATE_30    0x51615
#define BACK_SENSOR_FRAME_RATE       BACK_SENSOR_FRAME_RATE_25//BACK_SENSOR_FRAME_RATE_24

#define GUID_YUV2_0 0x32595559
#define GUID_YUV2_1 0x0000
#define GUID_YUV2_2 0x0010
#define GUID_YUV2_3 0x8000
#define GUID_YUV2_4 0x00AA00
#define GUID_YUV2_5 0x389B71

#define GUID_NV12_0 0x3231564E
#define GUID_NV12_1 0x0000
#define GUID_NV12_2 0x0010
#define GUID_NV12_3 0x8000
#define GUID_NV12_4 0x00AA00
#define GUID_NV12_5 0x389B71

/* Values for bmHeaderInfo (Video and Still Image Payload Headers, 2.4.3.3) */
#define UVC_STREAM_EOH	(1 << 7)
#define UVC_STREAM_ERR	(1 << 6)
#define UVC_STREAM_STI	(1 << 5)
#define UVC_STREAM_RES	(1 << 4)
#define UVC_STREAM_SCR	(1 << 3)
#define UVC_STREAM_PTS	(1 << 2)
#define UVC_STREAM_EOF	(1 << 1)
#define UVC_STREAM_FID	(1 << 0)

/* Video Class-Specific Request Codes, (USB_Video_Class_1.1.pdf, A.8 Video Class-Specific Request Codes) */
#define RC_UNDEFINED                               0x00
#define SET_CUR                                    0x01
#if 0
#define GET_CUR                                    0x81
#define GET_MIN                                    0x82
#define GET_MAX                                    0x83
#define GET_RES                                    0x84
#define GET_LEN                                    0x85
#define GET_INFO                                   0x86
#define GET_DEF                                    0x87
#endif
/* VideoStreaming Interface Control Selectors, (USB_Video_Class_1.1.pdf, A.9.7 VideoStreaming Interface Control Selectors) */
#define VS_CONTROL_UNDEFINED             	       0x00
#define VS_PROBE_CONTROL                 	       0x01
#define VS_COMMIT_CONTROL                     	   0x02
#define VS_STILL_PROBE_CONTROL               	   0x03
#define VS_STILL_COMMIT_CONTROL                    0x04
#define VS_STILL_IMAGE_TRIGGER_CONTROL      	   0x05
#define VS_STREAM_ERROR_CODE_CONTROL       	       0x06
#define VS_GENERATE_KEY_FRAME_CONTROL     	       0x07
#define VS_UPDATE_FRAME_SEGMENT_CONTROL    	       0x08

// Video Class-Specific VideoStreaming Interface Descriptor Subtypes, (USB_Video_Class_1.1.pdf, A.6 Video Class-Specific VS Interface Descriptor Subtypes) */
#define VS_UNDEFINED                               0x00
#define VS_INPUT_HEADER                            0x01
#define VS_OUTPUT_HEADER                           0x02
#define VS_STILL_IMAGE_FRAME                       0x03
#define VS_FORMAT_UNCOMPRESSED                     0x04
#define VS_FRAME_UNCOMPRESSED                      0x05
#define VS_FORMAT_MJPEG                            0x06
#define VS_FRAME_MJPEG                             0x07
#define VS_FORMAT_MPEG2TS                          0x0A
#define VS_FORMAT_DV                               0x0C
#define VS_COLORFORMAT                             0x0D
#define VS_FORMAT_FRAME_BASED                      0x10
#define VS_FRAME_FRAME_BASED                       0x11
#define VS_FORMAT_STREAM_BASED                     0x12

#define MSG_BACK_GET_DATA_N_DECODE                 0x00100000
#define IMAGE_DECODE_OUTBUF_NUM                    3  /*(JC)maximum jpeg decode output buffer number*/

typedef struct st_EHCD_ITD
{
	/*---------- Word 1 ----------*/
	uint32_t Horizontal;
	/*---------- Word 2-9 ----------*/
    uint32_t Transaction[8];
	/*---------- Word 10-16 ----------*/
	uint32_t BufferPointer[7];
} PACKED EITD, *PEITD;


typedef struct
{
  INT8U id;				//0 means there's no this function
  INT8U type;
  INT8U interface;
  INT8U cs;
  INT32U trans_len;
}UVCH_UNIT_INFO_t;

typedef struct{
  INT32S cur;   //current value
  INT32S min;   //minimum value
  INT32S max;   //maximum value
  INT32U res;   //resolution
  INT32U len;   //length
  INT32U info;  //info
  INT32S def;   //default value
}FUNC_ATTR_t;

typedef struct{
  FUNC_ATTR_t attr;
  UVCH_UNIT_INFO_t unit;
}FUNC_t;

typedef struct{
  FUNC_t auto_exposure_mode;
  FUNC_t exposure_time;
  FUNC_t brightness;
  FUNC_t contrast_auto;
  FUNC_t contrast;
  FUNC_t hue_auto;
  FUNC_t hue;
  FUNC_t saturation;
  FUNC_t sharpness;
  FUNC_t gamma;
  FUNC_t wb_temp;
  FUNC_t wb_temp_auto;
  //FUNC_t wb_component;
  FUNC_t bklight_comp;
  FUNC_t gain;
  FUNC_t pwrline_freq;
}CAMERA_FUNC_t;

typedef enum{
  CT_AE_MODE_CTRL = 0x02,
  CT_EXPO_TIME_ABS_CTRL = 0x04
}CT_CONTROL_SELECTORS;

#if 0
typedef enum{
  PU_CONTROL_UNDEFINED = 0,
  PU_BKLIGNT_COMPEN_CTRL,       //0x01
  PU_BRIGHTNESS_CTRL,           //0x02
  PU_CONTRAST_CTRL,             //0x03
  PU_GAIN_CTRL,                 //0x04
  PU_PWR_LINE_FREQ_CTRL,        //0x05
  PU_HUE_CTRL,                  //0x06
  PU_SATURATION_CTRL,           //0x07
  PU_SHARPNESS_CTRL,            //0x08
  PU_GAMMA_CTRL,                //0x09
  PU_WB_TEMP_CTRL,              //0x0A
  PU_WB_TEMP_AUTO_CTRL,         //0x0B
  PU_WB_COMPONENT_CTRL,         //0x0C
  PU_WB_COMPONENT_AUTO_CTRL,    //0X0D
  PU_DIGI_MULTIPLIER_CTRL,      //0X0E
  PU_DIGI_MULTIPLIER_LIMIT_CTRL,//0X0F
  PU_HUE_AUTO_CTRL,             //0X10
  PU_ANALOG_VIDEO_STANDARD_CTRL,//0X11
  PU_ANALOG_LOCK_STATUS_CTRL,   //0X12
  PU_CONTRAST_AUTO_CTRL         //0X13
}PU_CTRL_SELECTORS;
#endif

enum{
  INPUT_TERMINAL = 2,
  OUTPUT_TERMINAL,
  SELECTOR_UNIT,
  PROCESSING_UNIT,
  EXTENSION_UNIT
};

enum UVCH_ErrorCodes_t{
  ERROR_FUNC_VALUE = 5,
  ERROR_UNIT_INFO,
  ERROR_FUNC,
  ERROR_ATTR,
  ERROR_FUNC_AUTO_EN,
  HOST_ENUMERROR_ControlError
};

#if 0
typedef enum{
  GET_CUR  = 0x81,
  GET_MIN, //0x82
  GET_MAX, //0x83
  GET_RES, //0x84
  GET_LEN, //0x85
  GET_INFO,//0x86
  GET_DEF  //0x87
}GET_ATTR_E;
#endif


extern INT32U *uac_in_itd_tbl;
extern INT32U *uac_in_itd_tbl_m;
extern INT32U *uac_out_itd_tbl;
extern INT32U *uac_out_itd_tbl_m;
extern PEITD  uac_in_itd_tbl_Addr[];
extern PEITD  uac_out_itd_tbl_Addr[];
extern INT8U *g_buf;
extern INT8U *g_buf_m;
extern INT8U *uac_in_buf;
extern INT8U *uac_in_buf_m;
extern INT8U *uac_out_buf;
extern INT8U *uac_out_buf_m;
extern volatile INT32U g_usbh_int_queue[];
extern INT8U *gUVC_setup_buf;
extern INT8U giTD_buf_not_clear[];
extern INT8U giTD_buf_use[];

#undef min
#define min(a,b) (((a)<(b))?(a):(b))
#undef max
#define max(a,b) (((a)>(b))?(a):(b))

enum
{
    MSG_HOST_UVC_DISP_INIT = 1,
//  MSG_UVC_DISP_RUN,
    MSG_HOST_UVC_DISP_DECODE_SCALE,
    MSG_HOST_UVC_DISP_DECODE_SCALE_A,
    MSG_HOST_UVC_DISP_DECODE_SCALE_B,
    MSG_HOST_UVC_DISP_HALT
};

//----- main uvc task control message -----
typedef enum
{
	MSG_UVC_INIT = 1,
	MSG_UVC_PREVIEW_EN,
	MSG_UVC_PREVIEW_DIS,
	MSG_UVC_RECORD_EN,
	MSG_UVC_RECORD_DIS,
	MSG_UVC_FORCE_CAPTURE_EN,
	MSG_UVC_CAPTURE_EN,
	MSG_UVC_CLOSE,
	MSG_UVC_SOF
}UVC_MSG_CMD;
//----- main uvc task status message -----
typedef enum
{
	MSG_INIT_SUCCESS=1,
	MSG_INIT_FAIL,
	MSG_USB_FAIL,
	MSG_PREVIEW_INIT_SUCCESS,
	MSG_PREVIEW_INIT_FAIL,
	MSG_PREVIEW_FRAME_END,
	MSG_CAPTURE_FAIL,
	MSG_CAPTURE_SUCCESS
}MSG_UVC_STATUS;


typedef struct
{
	unsigned char format_index;
	unsigned char frame_index;
	unsigned	buf_num;
	unsigned char	**buf;
	unsigned	ln;
}ST_UVC_PREVIEW_CTRL;

typedef struct
{
	unsigned type;
	unsigned char* buf;
	unsigned ln;
}ST_UVC_CAPTURE;

typedef struct
{
	unsigned Buf_Num;
	unsigned ln;
}ST_UVC_FRAME_PASER;

/* USB class code */
/*typedef enum
{
	UC_DEVICE 	= 0x00,
	UC_AUDIO	= 0x01,
	UC_CDC_CTRL	= 0x02,
	UC_HID		= 0x03,
	UC_PHYSICAL	= 0x05,
	UC_IMAGE	= 0x06,
	UC_PRINTER	= 0x07,
	UC_MSC		= 0x08,
	UC_HUB		= 0x09,
	UC_CDC		= 0x0A,
	UC_CCID		= 0x0B,
	UC_CS		= 0x0D,
	UC_VIDEO	= 0x0E
}USB_CLASS_CODES;*/

/* USB descriptor types */
/*typedef enum
{
	VS_UNDEFINED			= 0x00,
	VS_INPUT_HEADER			= 0x01,
	VS_OUTPUT_HEADER 		= 0x02,
	VS_STILL_IMAGE_FRAME	= 0x03,
	VS_FORMAT_UNCOMPRESSED	= 0x04,
	VS_FRAME_UNCOMPRESSED	= 0x05,
	VS_FORMAT_MJPEG			= 0x06,
	VS_FRAME_MJPEG			= 0x07,
	VS_FORMAT_MPEG2TS		= 0x0A,
	VS_FORMAT_DV			= 0x0C,
	VS_COLORFPRMAT			= 0x0D,
	VS_FORMAT_FRAME_BASED	= 0x10,
	VS_FRAME_FRAME_BASED	= 0x11,
	VS_FORMAT_STREAM_BASED	= 0x12
}UVC_VS_SUBTYPE;*/
//----- USB Video Class Specific Request Codes -----
/*typedef enum
{
	RC_UNDEFINED	= 0x00,
	SET_CUR			= 0x01,
	GET_CUR			= 0x81,
	GET_MIN			= 0x82,
	GET_MAX			= 0x83,
	GET_RES			= 0x84,
	GET_LEN			= 0x85,
	GET_INFO		= 0x86,
	GET_DEF			= 0x87
}UVC_CS_REQ;*/
//----- UVC Video Streaming Interface Control Selector -----
/*typedef enum
{
	VS_CONTROL_UNDEFINED	= 0x00,
	VS_PROBE_CONTROL,
	VS_COMMIT_CONTROL,
	VS_STILL_PROBE_CONTROL,
	VS_STILL_COMMIT_CONTROL,
	VS_STILL_IMAGE_TRIGGER_CONTROL,
	VS_STREAM_ERROR_CODE_CONTROL,
	VS_GENERATE_KEY_FRAME_CONTROL,
	VS_UPDATE_FRAME_SEGMENT_CONTROL,
	VS_SYNCH_DELAY_CONTROL
}UVC_VS_REQ;*/

typedef enum
{
	UVC_DATA_UNCOMPRESSED_YUV2 = 1,
	UVC_DATA_UNCOMPRESSED_NV12,
	UVC_DATA_MJPEG
}UVC_DATA_FORMAT;

#define UVC_MAX_FORMAT 4
#define UVC_MAX_FRAME  20//10

typedef struct
{
	unsigned short Width;
	unsigned short Height;
}ST_Resolution;

typedef struct
{
	unsigned short	IMG_NUM;
	ST_Resolution	res[UVC_MAX_FRAME];
}ST_UVC_STILL_INFO;

typedef struct
{
	unsigned char		FrameIndex;
	ST_Resolution		res;
	unsigned char		FrameInterType;
	unsigned 			MaxBuffer;
	unsigned            MinBitRate;
	unsigned            MaxBitRate;
	unsigned            DefaultFrameInterval;
	unsigned			MinFrameInter;
	unsigned			MaxFrameInter;
	unsigned			FrameStep;
	unsigned			FrameInter[UVC_MAX_FRAME];
}ST_UVC_FRAME_INFO;

typedef struct
{
	unsigned char		FormatIndex;
	unsigned char 		FormatType;
	unsigned char		FrameTypeNum;
	ST_UVC_FRAME_INFO	FRAME_INFO[UVC_MAX_FRAME];
	ST_UVC_STILL_INFO	STILL_INFO;
}ST_UVC_FORMAT_INFO;

typedef struct
{
	unsigned char		FormatTypeNum;
	unsigned char 		CaptureMode;
	ST_UVC_FORMAT_INFO	Format_INFO[UVC_MAX_FORMAT];
}ST_UVC_STREAM_INFO;
//----- USB Video Class Interface information -----
typedef struct
{
	unsigned char 	IF;				/* Interface Number */
	unsigned char	Alt_Num;		/* Alternative Interface Number */
	struct
	{
		unsigned char 	Alt;		/* Alternative Interface Index */
		unsigned char 	Endp;		/* Endpoint */
		unsigned char   Multi;
		unsigned short	MaxPkt;		/* Maximum Packet Size */
	}alt[15];
	struct							/* Interrupt Endpoint Information */
	{
		unsigned char 	Endp;		/* Endpoint */
		unsigned char 	Poll;		/* Polling Interval */
		unsigned short	MaxPkt;		/* Maximum Packet Size */
	}Int;
}ST_UVC_IF_INFO;

#if 0
typedef struct
{
	INT8U arr[4][4];
}FourByFour;
#endif

typedef struct
{
	void	*nextTerminal;
	INT8U	DescSubtype;
	INT8U	NrCh;
	INT8U	TID;
	INT8U	InID;
	INT8U	OutID;
	INT16U	tType;
	INT8U	bCtrlSz;
	INT8U	bmControls[4];
	INT8U	*ext;
}UVC_TLINK;

typedef struct
{
	INT8U		Intf_Num;	//UAC Control Interface Number
	INT8U 		NrIntf;		//Total UAC streaming Interface number
	UVC_TLINK	*tLink;
}ST_UVC_CTRL_IF_INFO;

typedef struct
{
	ST_UVC_CTRL_IF_INFO		CTRL_INFO;
	ST_UVC_STREAM_INFO		STREAM_INFO;
	ST_UVC_IF_INFO			IF_INFO;
	CAMERA_FUNC_t 			CAM_FUNC;
}ST_UVC_INFO;


enum jpegdec_buffer_state {
	BUF_STATE_IDLE	 = 0,
	BUF_STATE_QUEUED = 1,
	BUF_STATE_ACTIVE = 2,
	BUF_STATE_DONE	 = 3,
	BUF_STATE_ERROR	 = 4,
};

typedef struct
{
    INT8U jpegdecbuf_idx;
    char *bufhead;
    INT32U buflength;
    INT32U bytesused;
    INT32U total_len[UVC_JPGDEC_BUF_MQUEUE_MAX];
	enum jpegdec_buffer_state state;
	INT8U bufkey[UVC_JPGDEC_BUF_MQUEUE_MAX];
//    ALIGN64 char jpegdecbuf[UVC_JPGDEC_BUF_MQUEUE_MAX][UVC_JPGDEC_BUF_SIZE];
    char *jpegdecbufptr[UVC_JPGDEC_BUF_MQUEUE_MAX];
}PACKED UVC_DECBUF_INFO_T;
#endif // __USBH_UVC_H__
