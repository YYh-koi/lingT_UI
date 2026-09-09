#include "project.h"

#define bit		INT8U
const INT8U LFSR_LUT[16] =
{
    0x0E, 0x47, 0xDB, 0x46, 0x46, 0x8D, 0x38, 0xE5,
    0xFC, 0x52, 0x7A, 0xDE, 0x6F, 0xC5, 0x05, 0xE6
};

INT8U g_LFSRseed[4];
INT8U g_LFSR_Key[6]; //first 4 bytes are random device seed
INT8U Gen_LFSR_32Bit_Key(void)
{
    INT8U tc_period, q;
    INT16U tw_LFSR_Key;
    INT16U tw_LFSR_MagicNum;

    g_LFSR_Key[0] = (INT8U)(*((volatile INT32U *) 0xD0500380));
    g_LFSR_Key[1] = (INT8U)(*((volatile INT32U *) 0xD0500384));
    g_LFSR_Key[2] = (INT8U)(*((volatile INT32U *) 0xD0500380));
    g_LFSR_Key[3] = (INT8U)(*((volatile INT32U *) 0xD0500384));

    ((INT8U*)(&tw_LFSR_MagicNum))[0] = g_LFSRseed[0]^g_LFSR_Key[0];
    ((INT8U*)(&tw_LFSR_MagicNum))[1] = g_LFSRseed[1]^g_LFSR_Key[1];
    ((INT8U*)(&tw_LFSR_Key))[0] = g_LFSRseed[2]^g_LFSR_Key[2];
    ((INT8U*)(&tw_LFSR_Key))[1] = g_LFSRseed[3]^g_LFSR_Key[3];

    tc_period = (((INT8U) tw_LFSR_MagicNum)^((INT8U)(tw_LFSR_MagicNum >>8)))&0x0F;
    tc_period = ((LFSR_LUT[tc_period]^g_LFSRseed[0])+(LFSR_LUT[15-tc_period]^g_LFSRseed[1]));
    for(q=0; q<tc_period; q++)
    {
        bit tb_bit = (bit)((tw_LFSR_Key ^ (tw_LFSR_Key >> 2) ^ (tw_LFSR_Key >> 3) ^ \
                            (tw_LFSR_Key >> 5)) & 0x01);
        tw_LFSR_Key = (((INT16U)tb_bit)<<15) | (tw_LFSR_Key >> 1);
    }

    /*if (((*((volatile INT32U *) 0xC015000C)) & 0x00000200) == 0)
    {
        print_string("???Gen_LFSR_32Bit_Key device\r\n");
        // patch area
        tw_LFSR_Key += 77;
        g_LFSR_Key[4] = ((INT8U*)(&tw_LFSR_Key))[0];
        g_LFSR_Key[5] = ((INT8U*)(&tw_LFSR_Key))[1];
    	 g_LFSR_Key[1] += 0x77;
    	 g_LFSR_Key[2] -= 0x77;

    }
    else*/
    {
        g_LFSR_Key[4] = ((INT8U*)(&tw_LFSR_Key))[0];
        g_LFSR_Key[5] = ((INT8U*)(&tw_LFSR_Key))[1];
    }

    //DBG_PRINT("\ndrv_usbd_uac_gen_LFSR_32Bit_Key:\r\n");

    /*DBG_PRINT("in: 0x%.2x 0x%.2x 0x%.2x 0x%.2x\r\n",g_LFSRseed[0],g_LFSRseed[1],g_LFSRseed[2],g_LFSRseed[3]);
    DBG_PRINT("device %x %x %x %x\r\n",g_LFSR_Key[0],g_LFSR_Key[1],g_LFSR_Key[2],g_LFSR_Key[3]);
    DBG_PRINT("out:0x%.2x 0x%.2x\r\n",g_LFSR_Key[4],g_LFSR_Key[5]);
    DBG_PRINT("tc_period: 0x%.2x\r\n",tc_period);*/

    return 0;
}
