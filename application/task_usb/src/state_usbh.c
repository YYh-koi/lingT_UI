#include "application.h"

#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
#include <stdlib.h>
#include "project.h"
#include "cmsis_os.h"
#include "drv_l1_scaler.h"
#include "drv_l2_display.h"
#include "drv_l1_timer.h"
#include "usbh_uvc.h"
#include "drv_l1_usbh.h"
#include "drv_l1_usbh_iso.h"
#include "drv_l2_usbh.h"
#include "drv_l1_gpio.h"
#include "drv_l1_arm_cp15.h"
#include "drv_l1_usbh_ehci.h"
#include "drv_l1_usbh_ohci.h"
#include "drv_l2_usbh_uvc.h"

#include "uvc_func_api.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define STACKSIZE                           8192
#define USB_HOST_UVC_TASK_QUEUE_MAX         64

/**************************************************************************
 *                           M A C R O S                                  *
 **************************************************************************/
#define DELETE_MSG(msg)\
{\
    if(msg){\
        xQueueReset(msg);\
        vQueueDelete(msg);\
        msg = 0;\
    }\
}

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern void drv_l1_usbh_ehci_set_iso_hw_frame_buf(INT32U hostid, uint8_t * abuf, uint8_t * bbuf);
extern INT32U drv_l2_usbh_enumeration(INT32U hostid);
extern void drv_l2_usbh_release(INT32U hostid);
extern unsigned usbh_interface_set(INT32U hostid, INT32U *maxpkt, INT32U frame_width, INT32U frame_height, INT32U frame_rate);
extern INT32U drv_l2_wait_hci_halt(INT32U hostid);
extern INT32U drv_l2_usbh_chk_port_connect_status(INT32U hostid);

extern INT32S uvc_q_get_idle(INT32U host_id,INT32U *buf);
extern INT32S uvc_q_put_idle(INT32U host_id,INT32U buf);
extern INT32S uvc_q_put_ready(INT32U host_id,INT32U buf,INT32U size);

extern 	INT8U	power_on_auto_record;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
uint32_t SystemCoreClock;
osThreadId hci_uvc_main_task_ID[2]  = {NULL, NULL};
osThreadId id_usb_host_uvc_entry[2] = {NULL, NULL};
osThreadId id_usb_host_uvc_disp_task[2] = {NULL, NULL};
xQueueHandle hUSBHostUVCTaskQ[2];
xQueueHandle hUSBHostIntrQ[2];
uint32_t hci_dev_connect_status[2] = {0, 0}, uvc_disp_halted, udev_ins_debcnt[2]={0, 0}, udev_rmv_debcnt[2]={0, 0};

INT32U MaxPacket;
INT32U g_uvc_frame_buf_size;
extern INT8U *g_dummybuf_m[HOSTNUM];
extern INT8U *g_dummybuf[HOSTNUM];
extern USBH_DEV_INFO *g_stDev[HOSTNUM];

extern INT8U *gUSBH_setup_buf_m[HOSTNUM];
extern INT8U *gUSBH_setup_buf[HOSTNUM];
UVC_HW_FRAMEBUF_INFO_T uvc_hw_framebuf_info[HOSTNUM];

uint32_t hci_dev_connect_retry[2] = {0, 0};
static INT32U pre_parking_status = 0;
static INT32U cur_parking_status = 0;


typedef struct FRAME_INFO_ARGS_S
{
	INT32U	frame_width;
	INT32U	frame_height;
} FRAME_INFO_ARGS;

FRAME_INFO_ARGS usbh_uvc_frame_info[2];

void usbh_uvc_mem_free(INT32U hostid)
{
    drv_l2_wait_hci_halt(hostid);

	/*free uvc terminal link*/
	drv_l2_usbh_free_uvc_ctrl_units(hostid);

    if(g_dummybuf_m[hostid] != NULL)
    {
		gp_free(g_dummybuf_m[hostid]);
		g_dummybuf[hostid] = NULL;
		g_dummybuf_m[hostid] = NULL;
    }

    if(g_stDev[hostid] != NULL)
    {
		if(g_stDev[hostid]->para[0] != NULL)
		{
	        gp_free(g_stDev[hostid]->para[0]);
	        g_stDev[hostid]->para[0] = NULL;
		}
	
        gp_free(g_stDev[hostid]);
        g_stDev[hostid] = NULL;
    }

    if(gUSBH_setup_buf_m[hostid] != NULL)
    {
        gp_free(gUSBH_setup_buf_m[hostid]);
        gUSBH_setup_buf_m[hostid] = NULL;
        gUSBH_setup_buf[hostid] = NULL;
    }

}

INT32U init_jpeg_buf(INT32U hostid)
{
    uvc_hw_framebuf_info[hostid].hw_abuf = (uint8_t *)DUMMY_BUFFER_ADDRS;
    uvc_hw_framebuf_info[hostid].hw_bbuf = (uint8_t *)DUMMY_BUFFER_ADDRS;

	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	if(hostid == HOST_UVC_PORT_FRONT)
	{
		g_uvc_frame_buf_size = HOST_USB_MJPEG_BUF_SIZE_FRONT;	//for each buff size
	}
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	if(hostid == HOST_UVC_PORT_REAR)
	{
		g_uvc_frame_buf_size = HOST_USB_MJPEG_BUF_SIZE_REAR;	//for each buff size
	}
	#endif

    return 1;
}

uint32_t check_jpeg_marker(uint8_t* jpg_buf, uint32_t jpg_len)
{
    uint8_t* jpg_bufp = jpg_buf;
    uint32_t jpg_length = jpg_len;

	/* Check start and end header of a JPEG buffer */
	drv_l1_cp15_clean_dcache_range((uint32_t)jpg_bufp, jpg_length);
	if(jpg_bufp[0] != 0xFF && jpg_bufp[1] != 0xD8)
	{
		DBG_PRINT("JPG Buffer 0x%x len 0x%x, [0]=0x%x, [1]=0x%x\r\n", jpg_bufp, jpg_length, jpg_bufp[0], jpg_bufp[1]);
		return 1;
	}
	return 0;
}

//For USB PHY setting
extern INT8U HOST_UPHY_REG[];
void usb_phy_i2c_read_cmd(INT32U usbid, INT8U device, INT8U addr, INT8U* data)
{
    USB20D_BASE_SEL(usbid);

	//DBG_PRINT("read, CDR = 0x%x, CCR = 0x%x\r\n", (phyaddr << 8), (I2C_SLAVE_ADDRESS << 2) | 0x1);
	/* set phy address */
	rUDPHYI2CDR = (addr << 8);
	/* set slave address ,read direction and enable I2C. [8:2] = 1, [1] = 0, [0] = 1 */
	rUDPHYI2CCR = (device << 2) | 0x1;
	/* wait read done */
	while((rUDPHYI2CCR & 0x01))
	{
	}

	*data = rUDPHYI2CDR & 0xFF;
}

/* excuted before drv_l2_usbh_init() */
void usb_config_phy(void)
{
	HOST_UPHY_REG[5] =  0x2b;	//0x06[4:3]	squelch, 0x23(140mv)/0x2b(115mv)/0x33(90mv)/0x3b(60mv)
	HOST_UPHY_REG[9] =  0x81;	//0x0C[2:0]	IPP, 0x80/0x81/0x82/0x83/0x84/0x85/0x86/0x87
	HOST_UPHY_REG[11] = 0x85;	//0x0E[3:0]	HS TX output current, 0x80~0x8F
	HOST_UPHY_REG[15] = 0x84;	//0x15[7:5]	TX DEMP, 0x04/0x24/0x44/0x64/0x84/0xA4/0xC4/0xE4
	HOST_UPHY_REG[16] = 0x42;	//0x16[1:0]	RX EQR, 0x40/0x41/0x42/0x43
}

void usb_read_phy_config(INT32U hostid)
{
	INT8U data;
	
	if(hostid == 0)
        drv_l1_clock_set_system_clk_en(CLK_EN1_USB20_DEVICE_A, 1);
    else if(hostid == 1)
        drv_l1_clock_set_system_clk_en(CLK_EN6_USB20_DEVICE_B, 1);

	data = 0;
	usb_phy_i2c_read_cmd(hostid, 0x01, 0x06, &data);	//squelch level, default = 0x23
	DBG_PRINT("USB PHY config[0x06] = 0x%x\r\n", data);
	data = 0;
	usb_phy_i2c_read_cmd(hostid, 0x01, 0x0C, &data);	//IPP, default = 0x81
	DBG_PRINT("USB PHY config[0x0C] = 0x%x\r\n", data);
	data = 0;
	usb_phy_i2c_read_cmd(hostid, 0x01, 0x0E, &data);	//TX DESR current, default = 0x85
	DBG_PRINT("USB PHY config[0x0E] = 0x%x\r\n", data);
	data = 0;
	usb_phy_i2c_read_cmd(hostid, 0x01, 0x15, &data);	//TX deamphasis, defaul[t = 0x84
	DBG_PRINT("USB PHY config[0x15] = 0x%x\r\n", data);
	data = 0;
	usb_phy_i2c_read_cmd(hostid, 0x01, 0x16, &data);	//RX equalizer, default = 0x42
	DBG_PRINT("USB PHY config[0x16] = 0x%x\r\n", data);	

	if(hostid == 0)
    	drv_l1_clock_set_system_clk_en(CLK_EN1_USB20_DEVICE_A, 0);
    else if(hostid == 1)
        drv_l1_clock_set_system_clk_en(CLK_EN6_USB20_DEVICE_B, 0);
}


void
do_MSG_USBH_HCI_DEV_INIT(
	INT32U hostid 
)
{
	//DBG_PRINT("==>MSG_USBH_HCI_DEV_INIT\r\n");
    udev_ins_debcnt[hostid] = 0;
    udev_rmv_debcnt[hostid] = 0;
	hci_dev_connect_status[hostid] = 0;
	usb_config_phy();
    drv_l2_usbh_init(hostid);
	usb_read_phy_config(hostid);
}

void 
usb_host_frame_info_set(
	INT32U hostid,
	INT32U frame_width,
	INT32U frame_height	
)
{
	usbh_uvc_frame_info[hostid].frame_width = frame_width;
	usbh_uvc_frame_info[hostid].frame_height = frame_height;
}

void usb_host_uvc_entry(void const *parm)
{
    INT32U hostid = *((INT32U*)parm);
	INT32U frame_buf;
    INT32U msg_id, ret;
	INT32U jpeg_buf,jpeg_len;

	DBG_PRINT("==Run:%s==hostid:%d==\r\n",__FUNCTION__,hostid);

	while(1)
	{
        if(hUSBHostUVCTaskQ[hostid] && (xQueueReceive(hUSBHostUVCTaskQ[hostid], &msg_id, portMAX_DELAY) == pdTRUE))
        {
			//DBG_PRINT("==Run:%s==msg_id:0x%x==\r\n",__FUNCTION__,msg_id);


            switch (msg_id)
            {
                case MSG_USBH_HCI_DEV_CONN:
                    while((drv_l2_usbh_chk_port_connect_status(hostid) == DEV_CONNECT) && (hci_dev_connect_status[hostid] == 0))
                    {
                        if(++udev_ins_debcnt[hostid]>5)
                        {
                            DBG_PRINT("usb dev insert+\r\n");
                            udev_ins_debcnt[hostid] = 0;
                            //============================================================
	                        if(drv_l2_usbh_hosttype(hostid) == TYPE_OHCI)
	                        {
                            	drv_l1_usbh_issue_reset(hostid, 30);
                            	DBG_PRINT(">>Full Speed device connected!\r\n");
	                        }

	                        if(drv_l2_usbh_hosttype(hostid) == TYPE_EHCI)
	                        {
								if(drv_l1_usbh_ehci_speed_enum(hostid) == 0)
								{
									break;
								}
								DBG_PRINT(">>High Speed device connected!\r\n");
	                        }
                            //============================================================
                            hci_dev_connect_status[hostid] = 1;
                            msg_id = MSG_USB_HOST_UVC_PLUG_IN;
                            xQueueSend(hUSBHostUVCTaskQ[hostid], &msg_id, 0);
                            break;
                        }
                    }
                    udev_ins_debcnt[hostid] = 0;
                break;

                case MSG_USBH_HCI_DEV_DISCONN:
                    while((drv_l2_usbh_chk_port_connect_status(hostid) == DEV_DISCONNECT) && (hci_dev_connect_status[hostid] == 1))
                    {
                        if(++udev_rmv_debcnt[hostid]>5)
                        {
                            DBG_PRINT("usb dev removed-\r\n");
							udev_rmv_debcnt[hostid] = 0;
                            hci_dev_connect_status[hostid] = 0;
                            msg_id = MSG_USB_HOST_UVC_PLUG_OUT;
                            xQueueSend(hUSBHostUVCTaskQ[hostid], &msg_id, 0);
                            break;
                        }
                    }

                break;

                case MSG_USB_HOST_UVC_PLUG_IN:
                    DBG_PRINT("->UVC plug in->\r\n");
                    if(drv_l2_usbh_enumeration(hostid)==FALSE)
                    {
                        drv_l2_usbh_release(hostid);
                        drv_l2_usbh_uninit(hostid);
                        usbh_uvc_mem_free(hostid);
                        DBG_PRINT("USB HOST UVC initial fail!\r\n");

						/*
							Check USB device IO exist, retry it
							Not exist, go to init function
						*/
						if((++hci_dev_connect_retry[hostid] < 6) || ap_peripheral_get_host_io_status())
						{
							if(hci_dev_connect_retry[hostid] > 3)
							{
								#if defined(BACK_SENSOR_PWR)
								gpio_write_io(BACK_SENSOR_PWR, 0);
								osDelay(300);
								gpio_write_io(BACK_SENSOR_PWR, 1);
								#endif
							}
						}
						osDelay(100);
						do_MSG_USBH_HCI_DEV_INIT(hostid);
                        //DBG_PRINT("USB HOST UVC reinitial:%d \r\n",hci_dev_connect_retry[hostid]);
                    }
                    else
                    {
                        DBG_PRINT("usbh enum ok!\r\n");
                        #if UVC_FUNC_CTRL == 1
                        ST_UVC_INFO *uvc = (ST_UVC_INFO*)g_stDev[hostid]->para[0];
                        uvch_func_setup(hostid, uvc);
                        #endif

                        ret = usbh_interface_set(hostid, &MaxPacket, usbh_uvc_frame_info[hostid].frame_width, usbh_uvc_frame_info[hostid].frame_height, BACK_SENSOR_FRAME_RATE);
                        if(ret == FALSE)
                        {
                            drv_l2_usbh_release(hostid);
                            drv_l2_usbh_uninit(hostid);
                            usbh_uvc_mem_free(hostid);
                            DBG_PRINT("USB HOST UVC initial fail b!\r\n");

							osDelay(100);
							do_MSG_USBH_HCI_DEV_INIT(hostid);
						}
                        else
                        {
                            init_jpeg_buf(hostid);
                            drv_l1_cp15_clean_dcache_range((INT32U)(&uvc_hw_framebuf_info[hostid]), sizeof(UVC_HW_FRAMEBUF_INFO_T));
                            //---------------
                            if(drv_l2_usbh_hosttype(hostid) == TYPE_EHCI)
                                drv_l1_usbh_ehci_set_iso_hw_frame_buf(hostid, uvc_hw_framebuf_info[hostid].hw_abuf, uvc_hw_framebuf_info[hostid].hw_bbuf);
                            else
                                drv_l1_usbh_ohci_set_iso_hw_frame_buf(hostid, uvc_hw_framebuf_info[hostid].hw_abuf, uvc_hw_framebuf_info[hostid].hw_bbuf);
                            //---------------
                            if(hUSBHostUVCTaskQ[hostid])
                            {
                                msg_id = MSG_USB_HOST_UVC_STARTGETISODATA;
                                xQueueSend(hUSBHostUVCTaskQ[hostid], &msg_id, 0);
                            }
                            DBG_PRINT("uvc init ok \r\n");
                        }
                    }
                break;

                case MSG_USB_HOST_UVC_STARTGETISODATA:
                    DBG_PRINT("uvc start getting data!\r\n");
                    vTaskDelay(30);
                    //----------------------------------------
                    #if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
					bsd_q_reset();
					#endif
                    if(drv_l2_usbh_hosttype(hostid) == TYPE_EHCI)
                        ret = drv_l2_usbh_ehci_iso_in_trans(hostid);
                    else
                        ret = drv_l2_usbh_ohci_iso_in_trans(hostid);

                    if(ret == FALSE)
                    {
                        drv_l2_usbh_release(hostid);
                        drv_l2_usbh_uninit(hostid);
                        usbh_uvc_mem_free(hostid);
                        DBG_PRINT("USB HOST UVC initial fail c!\r\n");

						osDelay(100);
						do_MSG_USBH_HCI_DEV_INIT(hostid);
						break;
                    }
                    //----------------------------------------
                    #if (_SENSOR_USB_MJPEG_CAM_REAR)
					if(hostid == HOST_UVC_PORT_REAR)
					{
		                while(osOK != app_msg_send(MSG_APQ_BACK_SENSOR_PLUG_IN, NULL, 0, MSG_PRI_NORMAL))
		                {
		                    osDelay(1);
		                }

						if(power_on_auto_record)
						{
							if(osOK == app_msg_send(MSG_APQ_POWER_ON_AUTO_RECORD, NULL, 0, MSG_PRI_NORMAL))
							{
								power_on_auto_record = 0;
							}
						}
					}
					#endif
                break;

                case MSG_USB_HOST_UVC_JPEGBUFA_DONE:
                case MSG_USB_HOST_UVC_JPEGBUFA_OVERBOUND:
					/* Get a new buf */
					if(uvc_q_get_idle(hostid,&frame_buf) == STATUS_FAIL)
					{
						frame_buf = DUMMY_BUFFER_ADDRS;
					}

                    if(drv_l2_usbh_hosttype(hostid) == TYPE_EHCI)
                    {
						drv_l1_usbh_ehci_iso_hw_set_new_bufa(hostid, frame_buf);
                    }
					else
					{
						drv_l1_usbh_ohci_iso_hw_set_new_bufa(hostid, frame_buf);
					}

					jpeg_len = uvc_hw_framebuf_info[hostid].hw_prevabuf_size;
                    cache_invalid_range((INT32U)(&jpeg_len), sizeof(INT32U));
					
					if(msg_id == MSG_USB_HOST_UVC_JPEGBUFA_OVERBOUND)
                    {
                        DBG_PRINT("A buf overbound!\r\n");
						jpeg_len = 0;
                    }
                    else
                    {
						//DBG_PRINT("[host:%d]A buf 0x%x done 0x%x size 0x%x done!\r\n",hostid,uvc_hw_framebuf_info[hostid].hw_abuf, uvc_hw_framebuf_info[hostid].hw_preabuf, uvc_hw_framebuf_info[hostid].hw_prevabuf_size);
                    }

					/* return previous done buf */
					jpeg_buf = (INT32U)uvc_hw_framebuf_info[hostid].hw_preabuf;
					if((jpeg_buf != DUMMY_BUFFER_ADDRS) && (jpeg_buf != 0))
					{
						if(jpeg_len == 0)
						{
							uvc_q_put_idle(hostid,jpeg_buf);
						}
						else
						{
	                        cache_invalid_range(jpeg_buf,jpeg_len);
							uvc_q_put_ready(hostid,jpeg_buf,jpeg_len);
						}
					}
					uvc_hw_framebuf_info[hostid].hw_preabuf = (uint8_t *)DUMMY_BUFFER_ADDRS;
					uvc_hw_framebuf_info[hostid].hw_prevabuf_size = 0;
				break;

                case MSG_USB_HOST_UVC_JPEGBUFB_DONE:
                case MSG_USB_HOST_UVC_JPEGBUFB_OVERBOUND:
					/* Get b new buf */
					if(uvc_q_get_idle(hostid,&frame_buf) == STATUS_FAIL)
					{
						frame_buf = DUMMY_BUFFER_ADDRS;
					}

                    if(drv_l2_usbh_hosttype(hostid) == TYPE_EHCI)
                    {
						drv_l1_usbh_ehci_iso_hw_set_new_bufb(hostid, frame_buf);
                    }
					else
					{
						drv_l1_usbh_ohci_iso_hw_set_new_bufb(hostid, frame_buf);
					}

					jpeg_len = uvc_hw_framebuf_info[hostid].hw_prevbbuf_size;
                    cache_invalid_range((INT32U)(&jpeg_len), sizeof(INT32U));

                    if(msg_id == MSG_USB_HOST_UVC_JPEGBUFB_OVERBOUND)
                    {
                        DBG_PRINT("B buf overbound!\r\n");
						jpeg_len = 0;
                    }
                    else
                    {
						//DBG_PRINT("[host:%d]B buf 0x%x done 0x%x size 0x%x done!\r\n",hostid,uvc_hw_framebuf_info[hostid].hw_bbuf, uvc_hw_framebuf_info[hostid].hw_prebbuf, uvc_hw_framebuf_info[hostid].hw_prevbbuf_size);
                    }

					/* return previous done buf */
					jpeg_buf = (INT32U)uvc_hw_framebuf_info[hostid].hw_prebbuf;
					if((jpeg_buf != DUMMY_BUFFER_ADDRS) && (jpeg_buf != 0))
					{
						if(jpeg_len == 0)
						{
							uvc_q_put_idle(hostid,jpeg_buf);
						}
						else
						{
	                        cache_invalid_range(jpeg_buf,jpeg_len);
							uvc_q_put_ready(hostid,jpeg_buf,jpeg_len);
						}
					}
					uvc_hw_framebuf_info[hostid].hw_prebbuf = (uint8_t *)DUMMY_BUFFER_ADDRS;
					uvc_hw_framebuf_info[hostid].hw_prevbbuf_size = 0;
				break;

				case MSG_USBH_HCI_DEV_ISR_HANG:
                case MSG_USB_HOST_UVC_PLUG_OUT:
                    DBG_PRINT("UVC PLUG OUT!\r\n");
					#if 0// defined(BACK_SENSOR_PWR)
					if(gpio_read_io(BACK_SENSOR_PWR) == 0)
					{
						osDelay(300);
						gpio_write_io(BACK_SENSOR_PWR, 1);			
					}
					#endif

					/* return UVC hardware buffer */
					if(uvc_hw_framebuf_info[hostid].hw_abuf && uvc_hw_framebuf_info[hostid].hw_abuf != (uint8_t *)DUMMY_BUFFER_ADDRS)
					{
						uvc_q_put_idle(hostid,(INT32U)uvc_hw_framebuf_info[hostid].hw_abuf);
						uvc_hw_framebuf_info[hostid].hw_abuf = (uint8_t *)DUMMY_BUFFER_ADDRS;
					}
					if(uvc_hw_framebuf_info[hostid].hw_bbuf && uvc_hw_framebuf_info[hostid].hw_bbuf != (uint8_t *)DUMMY_BUFFER_ADDRS)
					{
						uvc_q_put_idle(hostid,(INT32U)uvc_hw_framebuf_info[hostid].hw_bbuf);
						uvc_hw_framebuf_info[hostid].hw_bbuf =(uint8_t *)DUMMY_BUFFER_ADDRS;
					}

					#if (_SENSOR_USB_MJPEG_CAM_REAR)
					if(hostid == HOST_UVC_PORT_REAR)
					{
		                while(osOK != app_msg_send(MSG_APQ_BACK_SENSOR_PLUG_OUT, NULL, 0, MSG_PRI_NORMAL)) {
		                    osDelay(1);
		                }
					}
					#endif

                    drv_l2_usbh_release(hostid);
                    drv_l2_usbh_uninit(hostid);
                    usbh_uvc_mem_free(hostid);
					drv_l2_usbh_release_reverse_parking(hostid);
					uvc_release_keep_buf(hostid);

					do_MSG_USBH_HCI_DEV_INIT(hostid);

                    hci_dev_connect_status[hostid] = 0;//(JC)
					hci_dev_connect_retry[hostid] = 0;
					cur_parking_status = 0;
					pre_parking_status = 0;
				break;

				case MSG_USB_HOST_UVC_RLS:
                    DBG_PRINT("uvc plugout!\r\n");
                    usbh_uvc_mem_free(hostid);
                    hci_dev_connect_status[hostid] = 0;
                    msg_id = MSG_USBH_HCI_EXIT;
                break;

				case MSG_USB_HOST_UVC_CHK_RP_STA:
					if(hci_dev_connect_status[hostid])
					{
						#if (_SENSOR_USB_MJPEG_CAM_FRONT)
						if(hostid == HOST_UVC_PORT_FRONT)
						{
							drv_l2_usbh_chk_reverse_parking(hostid);
							break;
						}
						#endif
					
						#if (_SENSOR_USB_MJPEG_CAM_REAR)
						if(hostid == HOST_UVC_PORT_REAR)
						{
							cur_parking_status = drv_l2_usbh_chk_reverse_parking(hostid);
							if(cur_parking_status == 0xFF) // means "Not Ready Yet, Try Again Later"
							{
								break;
							}

							//DBG_PRINT("=>%d,%d\r\n",cur_parking_status,pre_parking_status);
							if(cur_parking_status != pre_parking_status)
							{
								pre_parking_status = cur_parking_status;
								if(pre_parking_status)
								{
			                        //DBG_PRINT("REVERSE PARKING.....\r\n");
									app_msg_send(MSG_APQ_BACK_A_CAR_ON, NULL, 0, MSG_PRI_NORMAL);
								}
								else
								{
			                        //DBG_PRINT("NORMAL DRIVING.....\r\n");
									app_msg_send(MSG_APQ_BACK_A_CAR_OFF, NULL, 0, MSG_PRI_NORMAL);
								}
							}
						}
						#endif
					}
	               break;

                default:
					DBG_PRINT("usb_host_uvc_entry default msg_id 0x%x\r\n", msg_id);
                break;
            }
        }
    }
}

/*******************************************************************************
 *                   usb host controller uvc main entry
 *******************************************************************************/
void state_usbh_uvc_entry(void const *pvParameters)
{
	INT32U hostid;
	osThreadDef_t usb_host_uvc_task = {"usb_host_uvc_task", usb_host_uvc_entry, osPriorityRealtime, 1, STACKSIZE};

	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	hostid = HOST_UVC_PORT_FRONT;
    hUSBHostUVCTaskQ[hostid] = xQueueCreate(USB_HOST_UVC_TASK_QUEUE_MAX, sizeof(INT32U));
    hUSBHostIntrQ[hostid] = xQueueCreate(3, sizeof(INT32U));
    id_usb_host_uvc_entry[hostid] = osThreadCreate(&usb_host_uvc_task, (void*)(&g_usbh_id[hostid]));
	if(id_usb_host_uvc_entry[hostid] == 0)
	{
	    ERR_PRINT("[%d]osThreadCreate fail\r\n",hostid);
	}
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	hostid = HOST_UVC_PORT_REAR;	
    hUSBHostUVCTaskQ[hostid] = xQueueCreate(USB_HOST_UVC_TASK_QUEUE_MAX, sizeof(INT32U));
    hUSBHostIntrQ[hostid] = xQueueCreate(3, sizeof(INT32U));
    id_usb_host_uvc_entry[hostid] = osThreadCreate(&usb_host_uvc_task, (void*)(&g_usbh_id[hostid]));
	if(id_usb_host_uvc_entry[hostid] == 0)
	{
	    ERR_PRINT("[%d]osThreadCreate fail\r\n",hostid);
	}
	#endif
		
	return;
}

INT32S
state_usbh_uvc_do_init(
	void
)
{
	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	drv_l2_usbh_register_host_operation_table(HOST_UVC_PORT_FRONT,HOST_USB_TYPE_FRONT);
	do_MSG_USBH_HCI_DEV_INIT(HOST_UVC_PORT_FRONT);
	#if HOST_USB_TYPE_FRONT == HOST_USB_EHCI
	usb_host_frame_info_set(HOST_UVC_PORT_FRONT,USB_EHCI_OUT_WIDTH_FRONT,USB_EHCI_OUT_HEIGHT_FRONT);
	#else
	usb_host_frame_info_set(HOST_UVC_PORT_FRONT,USB_OHCI_OUT_WIDTH_FRONT,USB_OHCI_OUT_HEIGHT_FRONT);
	#endif	
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	drv_l2_usbh_register_host_operation_table(HOST_UVC_PORT_REAR,HOST_USB_TYPE_REAR);
	do_MSG_USBH_HCI_DEV_INIT(HOST_UVC_PORT_REAR);	
	#if HOST_USB_TYPE_REAR == HOST_USB_EHCI
	usb_host_frame_info_set(HOST_UVC_PORT_REAR,USB_EHCI_OUT_WIDTH_REAR,USB_EHCI_OUT_HEIGHT_REAR);
	#else
	usb_host_frame_info_set(HOST_UVC_PORT_REAR,USB_OHCI_OUT_WIDTH_REAR,USB_OHCI_OUT_HEIGHT_REAR);
	#endif	
		
		if(drv_l2_usbh_chk_port_connect_status(HOST_UVC_PORT_REAR) == DEV_DISCONNECT)
		{
			if(power_on_auto_record)
			{
				if(osOK == app_msg_send(MSG_APQ_POWER_ON_AUTO_RECORD, NULL, 0, MSG_PRI_NORMAL))
				{
					power_on_auto_record = 0;
				}
			}	
		}
	#endif
	
	return STATUS_OK;
}
#endif //#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)

