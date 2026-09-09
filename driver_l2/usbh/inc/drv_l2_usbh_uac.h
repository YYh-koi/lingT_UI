#ifndef __drv_l2_USBH_UAC_H__
#define __drv_l2_USBH_UAC_H__

#define AC_HEADER_DESC		0x01
#define AC_IT_DESC			0x02
#define AC_OT_DESC			0x03
#define AC_MIXER_DESC		0x04
#define AC_FU_DESC			0x06

#define	AS_GENERAL			0x01
#define AS_FORMAT_TYPE		0x02
#define AS_FORMAT_SPECIFIC	0x03

/* Feature Unit Control Bits */
#define AUDIO_CONTROL_MUTE                      0x0001
#define AUDIO_CONTROL_VOLUME                    0x0002
#define AUDIO_CONTROL_BASS                      0x0004
#define AUDIO_CONTROL_MID                       0x0008
#define AUDIO_CONTROL_TREBLE                    0x0010
#define AUDIO_CONTROL_GRAPHIC_EQUALIZER         0x0020
#define AUDIO_CONTROL_AUTOMATIC_GAIN            0x0040
#define AUDIO_CONTROL_DEALY                     0x0080
#define AUDIO_CONTROL_BASS_BOOST                0x0100
#define AUDIO_CONTROL_LOUDNESS                  0x0200

#define UAC_ISO_IN			0
#define UAC_ISO_OUT			1

typedef struct
{
/*
	unsigned char		FormatTypeNum;
	unsigned char 		CaptureMode;
	ST_UAC_FORMAT_INFO	Format_INFO[UAC_MAX_FORMAT];
*/
	INT8U				NrChannels;
	INT8U				SubframeSize;
	INT8U				BitResolution;
	INT8U				SamFreqType;
	INT32U				*SamFreq;
}A_STRM_INFO;

//----- USB Audio Class Interface information -----
typedef struct
{
	INT8U			iso_dir;		/* Interface iso direction: in/out */
	INT8U			tLinkId;		/* Terminal Link ID	*/
	unsigned char 	IF;				/* The number of AudioStreaming interfaces*/
	unsigned char	NrAlt;			/* Alternative Interface Number */
	struct EP_INFO_S
	{
		A_STRM_INFO		STREAM_INFO;
		unsigned char 	Alt;		/* Alternative Interface Index */
		unsigned char 	Endp;		/* Endpoint */
		unsigned char   Multi;
		unsigned short	MaxPkt;		/* Maximum Packet Size */
	}alt[15];
#if 0
	struct							/* Interrupt Endpoint Information */
	{
		unsigned char 	Endp;		/* Endpoint */
		unsigned char 	Poll;		/* Polling Interval */
		unsigned short	MaxPkt;		/* Maximum Packet Size */
	}Int;
#endif
}ST_UAC_STREAM_IF_INFO;

typedef struct
{
	INT8U arr[4][4];
}FourByFour;

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
	FourByFour	bmaControls;
	//INT8U	OutID;
}UAC_TLINK;


typedef struct
{
	INT8U		Intf_Num;	//UAC Control Interface Number
	INT8U 		NrIntf;		//Total UAC streaming Interface number
	UAC_TLINK	*tLink;
}ST_UAC_CTRL_IF_INFO;

typedef struct
{
	//void					*terminal_link;
	//ST_UAC_STREAM_INFO		STREAM_INFO;
	//ST_UAC_IF_INFO			IF_INFO;

	ST_UAC_CTRL_IF_INFO		CTRL_IF_INFO;
	ST_UAC_STREAM_IF_INFO	*STREAM_IF_INFO;
}ST_UAC_INFO;


typedef struct
{
	INT8S*  ppcmbuf;
	INT32U  pcmframesize;
	//INT32U	pktsz;
	volatile INT32U	read_index;				/* For audio decode read */
	volatile INT32U	write_index;			/* For USBD write */
	INT32U	channel;
	INT32U	sample_rate;
	INT32U	datalen;

}USBH_UAC_CTRL;
#endif
