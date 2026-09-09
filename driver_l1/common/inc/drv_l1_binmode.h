
#define BINMODE_BUF_MASK    0x0FFFFFFE //BIT0 is stand for the address updating which hardware is using it
#define BINMODE_BUF_FROMA   0x40000000
#define BINMODE_BUF_FROMB   0x80000000
#define BINMODE_BUF_PIP     0xC0000000
#define BINMODE_BUF_AB      0xE0000000
#define BINMODE_TAG_MASK    0xC0000000

#define BINMODE_FLAG_BUFA   0x00000010
#define BINMODE_FLAG_BUFB   0x00000020
#define BINMODE_FLAG_END    0x00000001
#define BINMODE_FLAG_OF     0x00010000
#define BINMODE_FLAG_MASK   0x00010031
#define BINMODE_FLAG_RETURN 0x00100000

//for api
#define BINCFG_BY2          0x0000
#define BINCFG_BY4          0x0100
#define BINCFG_AVG          0x0000
#define BINCFG_SKIP         0x0010
#define BINCFG_BYPASS       0x0020
#define BINCFG_RAW8         0x0000
#define BINCFG_RAW10        0x4000
#define BINCFG_CROP         0x1000

#define BINMODE_SYSFIFO     SYSTEM_FIFO_ADDRS //0:mease not use system fifo
#define BINMODE_B_NUM       4

typedef enum
{
    BINMODE_A,
	BINMODE_B,
	BINMODE_B1,
	BINMODE_B2,
	BINMODE_B3,
    BINMODE_MAX_NUMS
} BINMODE_NUMBER;

typedef struct {
    INT32U  buf_addr;
    INT32U  width;
    INT32U  height;
    INT8U   id;
} BINMODE_ARGUMENT;

typedef struct {
	INT32U  bit_mode;//8bit 10bit
	INT32U  pixwidth10Pad;
	INT32U  mode;//bypass, avg, skip
	INT32U  by_m;//by2, by4
	INT32U  crop_enable;
	INT32U  crop_x;
	INT32U  crop_y;
	INT32U  crop_width; //crop then binning
	INT32U  crop_height;
	INT32U  sensor_width; //binmode input size
	INT32U  sensor_height;
	INT32U  cdsp_width; //binmode output > cdsp input/output size
	INT32U  cdsp_height;
	INT32U  pscaler_width; //for pscaler size
	INT32U  pscaler_height;
	INT32U  _4pixel_mode;
} BINMODE_CDSP_CFG, *P_BINMODE_CDSP_CFG;

typedef struct {
	volatile INT32U	CTRL;           //0x00
	volatile INT32U	CROP_START;     //0x04
	volatile INT32U	CROP_SIZE;      //0x08
	volatile INT32U	IN_SIZE;        //0x0C
	volatile INT32U	FRAME_A_BUF;    //0x10
	volatile INT32U	FRAME_B_BUF;    //0x14
	volatile INT32U	INT_EN;         //0x18
	volatile INT32U RESERVE0;       //0x1C
	volatile INT32U SOFT_RST;       //0x20
	volatile INT32U FRAME_STS;      //0x24
	volatile INT32U RESERVE1;       //0x28
	volatile INT32U RESERVE2;       //0x2C
	volatile INT32U RESERVE3;       //0x30
	volatile INT32U INT_FLAG;       //0x34
	volatile INT32U OUT_SIZE;       //0x38
} binmodeReg_t;

extern INT32U drv_l1_binmode_int_flag_get(INT8U binmode_num);
extern INT32S drv_l1_binmode_int_flag_set(INT8U binmode_num, INT32U int_flag);
extern INT32S drv_l1_binmode_interrupt_set(INT8U binmode_num, INT32U int_en);
extern INT32S drv_l1_binmode_interrupt_register(INT8U binmode_num, void (*user_isr_callback)(INT32U dev_no, INT32U event));
extern INT32S drv_l1_binmode_config(INT8U binmode_num, P_BINMODE_CDSP_CFG config);
extern INT32S drv_l1_binmode_sysfifo_set(INT8U binmode_num, INT8U enable);
extern void drv_l1_binmode_framebuf_A_set(INT8U binmode_num, INT32U buffer);
extern void drv_l1_binmode_framebuf_B_set(INT8U binmode_num, INT32U buffer);
extern INT32U drv_l1_binmode_framebuf_A_get(INT8U binmode_num);
extern INT32U drv_l1_binmode_framebuf_B_get(INT8U binmode_num);
extern INT32U drv_l1_binmode_get_cfg_out_width(INT8U binmode_num);
extern INT32U drv_l1_binmode_get_cfg_out_height(INT8U binmode_num);
extern INT32S drv_l1_binmode_disable(INT8U binmode_num);
extern INT32S drv_l1_binmode_enable(INT8U binmode_num);
extern INT32U drv_l1_binmode_get_ctrl(INT8U binmode_num);
extern INT32S drv_l1_binmode_set_ctrl(INT8U binmode_num, INT32U ctrl);
extern INT32S drv_l1_binmode_init(INT8U binmode_num, INT8U vic_num);
extern INT32S drv_l1_binmode_uninit(INT8U binmode_num, INT8U vic_num);
extern void drv_l1_binmode_set_4pixel_mode(INT8U binmode_num, INT8U enable);
