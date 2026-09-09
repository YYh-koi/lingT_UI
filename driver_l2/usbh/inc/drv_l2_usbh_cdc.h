#ifndef __drv_l2_USBH_CDC_H__
#define __drv_l2_USBH_CDC_H__

#define MSG_USB_HOST_CDC_DEV_PLUG_IN        1
#define MSG_USBH_CDC_DEV_PROCESS            2
#define MSG_USB_HOST_CDC_DEV_PLUG_OUT       3
#define MSG_USBH_CDC_DEV_STOP               4
#define MSG_USBH_CDC_DEV_WRITE_COMPLETE     5
#define MSG_USBH_CDC_DEV_READ_COMPLETE      6

typedef enum
{
    CDC_REQ_SendEncapsulatedCommand = 0x00,
    CDC_REQ_GetEncapsulatedResponse = 0x01,
    CDC_REQ_SetLineEncoding         = 0x20,
    CDC_REQ_GetLineEncoding         = 0x21,
    CDC_REQ_SetControlLineState     = 0x22,
    CDC_REQ_SendBreak               = 0x23
}CDC_ClassRequest;

typedef struct
{
	uint8_t Size;
	uint8_t Type;

}PACKED USB_Descriptor_Header_t;


typedef struct
{
	USB_Descriptor_Header_t Header;

	uint8_t FirstInterface;
	uint8_t InterfaceCnt;
	uint8_t FuncClass;
	uint8_t FuncSubClass;
	uint8_t FuncProtocol;
	uint8_t FuncStrIndex;

}PACKED USB_Descriptor_IAD_t;


typedef struct
{
	USB_Descriptor_Header_t Header;

	uint8_t InterfaceNumber;
	uint8_t AlternateSetting;
	uint8_t TotalEndpoints;
	uint8_t Class;
	uint8_t SubClass;
	uint8_t Protocol;
	uint8_t InterfaceStrIndex;

}PACKED USB_Descriptor_Interface_t;


typedef struct
{
	USB_Descriptor_Header_t Header;

	uint8_t  EndpointAddress;
	uint8_t  Attributes;
	uint16_t EndpointSize;
    uint8_t  PollingInterval;

}PACKED USB_Descriptor_Endpoint_t;

typedef struct
{
	USB_Descriptor_Header_t Header;
    uint16_t UnicodeString[64];
}PACKED USB_Descriptor_String_t;

typedef struct
{
	uint32_t DTERate;
	uint8_t  CharFormat;
	uint8_t  ParityType;
	uint8_t  DataBits;

}PACKED CDC_LineCoding_t;


typedef struct
{
		uint8_t  DataIN_EPNumber[6];
		uint16_t DataIN_EPSize[6];
		uint8_t  DataIN_pInterval[6];
		uint8_t  DataOUT_EPNumber[6];
		uint16_t DataOUT_EPSize[6];
		uint8_t  DataOUT_pInterval[6];
		uint8_t  Notification_EPNumber[6];
		uint16_t Notification_EPSize[6];
		uint8_t  Notification_pInterval[6];

		uint8_t  ControlInterfaceNumber[6];
		uint8_t  DataInterfaceNumber[6];
		uint8_t  iad_function_str_desc_index[6];
		uint8_t  control_interface_str_desc_index[6];
		uint8_t  data_interface_str_desc_index[6];

		uint8_t	 DATAIN_toggle[6];
        uint8_t	 DATAOUT_toggle[6];
        uint8_t	 Notification_in_toggle[6];

		CDC_LineCoding_t LineCoding[6];

} USB_CDCACMClassInfo_t;

typedef	INT32U (*CDC_CALLBACK_FUNC)(INT32U);
typedef uint8_t (* FindDescriptor)(void * const);
extern USB_CDCACMClassInfo_t CDCACMClassInfo[];
extern xQueueHandle hUSBHostCDCPETaskQ[];

#define DESCRIPTOR_PCAST(DescriptorPtr, DescType)   ((DescType *)(DescriptorPtr))
#define DESCRIPTOR_CAST(DescriptorPtr, DescType)  (*DESCRIPTOR_PCAST(DescriptorPtr, DescType))
#define GETDESCRIPTOR_TYPE(DescriptorPtr)      DESCRIPTOR_PCAST(DescriptorPtr, USB_Descriptor_Header_t)->Type

INT32U drv_l2_usbh_cdc_set_line_coding(INT32U hostid, USBH_DEV_INFO *dev, INT8U fn);

INT32U drv_l2_usbh_cdc_get_line_coding(INT32U hostid, USBH_DEV_INFO *dev, INT8U fn);

INT32U drv_l2_usbh_cdc_set_control_line_state(INT32U hostid, USBH_DEV_INFO *dev, INT8U fn);

INT32U drv_l2_usbh_cdc_sendbreak(INT32U hostid, USBH_DEV_INFO *dev, \
                                 INT8U BreakDuration, INT8U fn);

void drv_l2_usbh_cdc_set_line_DTERate(INT32U hostid, INT32U DTERate, INT8U fn);

INT32U drv_l2_usbh_cdc_read(
    INT32U hostid,
	USBH_DEV_INFO *dev,
	INT32U len,
	void * rbuf,
	INT32U *act_length,
	INT8U* pErrorCode,
	INT8U fn);

INT32U drv_l2_usbh_cdc_write(
    INT32U hostid,
	USBH_DEV_INFO *dev,
	INT32U len,
	void * wbuf,
	INT8U* pErrorCode,
	INT8U fn);

INT32U drv_l2_usbh_cdc_notification(
    INT32U hostid,
	USBH_DEV_INFO *dev,
	INT32U len,
	void * nbuf,
	INT8U* pErrorCode,
	INT8U fn);
extern unsigned drv_l2_usbh_parse_cdc_descriptor(INT32U hostid, void* pConfigData, unsigned ConfigLen);
extern INT32U drv_l2_usbh_cdcacm_enumeration(INT32U hostid);

#endif
