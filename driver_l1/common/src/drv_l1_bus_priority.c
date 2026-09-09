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
#include "drv_l1_bus_priority.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define BUS_PRIORITY_BASE  0xD020007C

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
/*
				// Start: 0xD020_007C
				// Offset
BUS_PRY_M1;		// 0x0000	// ISP aeW
BUS_PRY_M2;		// 0x0004   // pscaA mb
BUS_PRY_M3;		// 0x0008   // pscaA
BUS_PRY_M4;		// 0x000C   // sad
BUS_PRY_M5;		// 0x0010   // ISP MD
BUS_PRY_M6;		// 0x0014   // Binmode B
BUS_PRY_M7;		// 0x0018   // IPU 0
BUS_PRY_M8;		// 0x001C	// pscaB mb
BUS_PRY_M9;		// 0x0020	// pscaB
BUS_PRY_M10;	// 0x0024   // h264 scalar
BUS_PRY_M11;	// 0x0028   // IPU 1
BUS_PRY_M12;	// 0x002C   // Binmode B CH1
BUS_PRY_M13;	// 0x0030   // rot
BUS_PRY_M14;	// 0x0034   // dma
BUS_PRY_M15;	// 0x0038   // mp3
BUS_PRY_M16;	// 0x003C   // spu
BUS_PRY_M17;	// 0x0040   // scan2mb H264 M4
BUS_PRY_M18;	// 0x0044   // nfc
BUS_PRY_M19;	// 0x0048   // ppuppu
BUS_PRY_M20;	// 0x004C   // Binmode A
BUS_PRY_M21;	// 0x0050   // usb20Host or usb20
BUS_PRY_M22;	// 0x0054   // usb20 1
BUS_PRY_M23;	// 0x0058   // scalar
BUS_PRY_M24;	// 0x005C   // ISP W
BUS_PRY_M25;	// 0x0060   // PPU fb2
BUS_PRY_M26;	// 0x0064   // mb2scan
BUS_PRY_M27;	// 0x0068   // Binmode B CH2
BUS_PRY_M28;	// 0x006C   // hdmi
BUS_PRY_M29;	// 0x0070   // sen CSI
BUS_PRY_M30;	// 0x0074   // jpegB
BUS_PRY_M31;	// 0x0078   // lbp
BUS_PRY_M32;	// 0x007C   // ISP R
BUS_PRY_M33;	// 0x0080   // box
BUS_PRY_M34;	// 0x0084   // PPU spr
BUS_PRY_M35;	// 0x0088   // gpdla sdp
BUS_PRY_M36;	// 0x008C   // Binmode B CH3
BUS_PRY_M37;	// 0x0090   // IPU 2
BUS_PRY_M38;	// 0x0094   // H264 M1 bitstream
BUS_PRY_M39;	// 0x0098   // sysfifo 1
BUS_PRY_M40;	// 0x009C   // sysfifo
BUS_PRY_M41;	// 0x00A0   // H264 M0 table
BUS_PRY_M42;	// 0x00A4   // ENMAC lite
BUS_PRY_M43;	// 0x00A8   // rotator
BUS_PRY_M44;	// 0x00AC   // psca Yonly
BUS_PRY_M45;	// 0x00B0   // jpegA
BUS_PRY_M46;	// 0x00B4   // spifc
BUS_PRY_M47; 	// 0x00B8   // cpu conv420
BUS_PRY_M48; 	// 0x00BC   // sd30
BUS_PRY_M49;	// 0x00C0   // gpdla idma
BUS_PRY_M50; 	// 0x00C4   // fft
BUS_PRY_M51; 	// 0x00C8   // rotator LB
BUS_PRY_M52;	// 0x00CC   // ac
BUS_PRY_M53;	// 0x00D0   // ac W
BUS_PRY_M54;	// 0x00D4   // H264 M2 REF fbc dec
BUS_PRY_M55;	// 0x00D8   // sysfifo 2
BUS_PRY_M56;	// 0x00DC   // H264 M3 REC fbc enc
BUS_PRY_M57;	// 0x00E0   // ppuppu W
BUS_PRY_M58;	// 0x00E4   // ISP 3dW
BUS_PRY_M59;	// 0x00E8   // ISP 3DdR
BUS_PRY_M60;	// 0x00EC   // usb20hostB or usb20B
BUS_PRY_M61;	// 0x00F0   // usb20B 1
BUS_PRY_M62;	// 0x00F4   // sec
BUS_PRY_M63;	// 0x00F8   // 0
*/

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
drv_l1_bus_priority_set(
	BUS_PRY_ARGS* pBusPryArgs
)
{
	volatile INT32U* Bus_Pry_Reg_Addr;
	INT8U waiting_cnt;
	INT8U en_preempt;

	Bus_Pry_Reg_Addr = ((volatile INT32U *) (BUS_PRIORITY_BASE + (pBusPryArgs->BP_num << 2)));

	waiting_cnt = (pBusPryArgs->waiting_count & 0x7F);
	en_preempt = (pBusPryArgs->enable_preempt & 0x01);

	//DBG_PRINT("Addr:0x%x Value:0x%x\r\n",Bus_Pry_Reg_Addr,(en_preempt << 7 | waiting_cnt));

	*Bus_Pry_Reg_Addr = (en_preempt << 7 | waiting_cnt);
}
