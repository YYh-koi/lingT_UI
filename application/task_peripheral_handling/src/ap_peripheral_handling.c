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
#include "application.h"

#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
#include "state_lvgl.h"
#endif

#define BACK_SENSOR_STABLE_THRESHOLD	4
#define BACK_A_CAR_STABLE_THRESHOLD     4


static INT8U back_a_car_on_flag = 0;
static ap_peripheral_call_key_handler_t call_key_handler;
static ap_peripheral_media_key_handler_t media_key_handler;

INT8U	auto_off_force_disable = 0;

static INT16U config_cnt;

static INT8U gsensor_lock_flag = 0;
INT8U gsensor_data[2][32];

static INT16U		adc_battery_value_new, adc_battery_value_old;
static INT32U		battery_stable_cnt = 0;

// Defines threshold number that AD value is deemed stable
#define C_BATTERY_STABLE_THRESHOLD	2

#define ADC_CHANNEL_MASK	0xF
static INT16U		ad_value;

#if C_BATTERY_DETECT
//Note :	BATTERY_CNT_WAY = 0 -> send result once ,after count and average
//			BATTERY_CNT_WAY = 1 -> average evey time and send result
#define BATTERY_CNT_WAY 		1
static INT32U		battery_value_sum = 0;
static INT8U		bat_ck_cnt = 0;
static BAT_LVL		bat_lvl_cal_bak = BAT_LVL_NUM - 1;
#endif // #if C_BATTERY_DETECT

#if C_BATTERY_LOW_POWER_OFF
static INT16U		low_voltage_cnt, /*core_power_cnt=0,*/ core_power_low_power_flag = 0;
#define 			LOW_VOL_TRESHOLD					3
#define 			CORE_POWER_THRESHOLD_CNT			5
#define				CORE_POWER_THRESHOLD				1400	// 1.10v / 3.3v * 4096
#define             CORE_POWER_DETECT_PIN	            9       //ADC_LINE_8
#endif

static INT8U		ad_detect_timerid;

#if ADKEY_NUM
static KEYSTATUS	ad_key_map[ADKEY_NUM];
#endif // #if ADKEY_NUM

static INT8U		ad_key_num=0;

static INT16S		ad_key_thsr_l = 0x7FFF;
static INT16S		ad_key_thsr_h = 0x0;

static INT8U		ad_line_count= 0;
static INT8U		core_power_cnt= 0;
static INT16U		adc_key_release_value_old, adc_key_release_value_new, adc_key_release_value_stable;
static INT32U		key_release_stable_cnt = 0;

static INT32U		ad_time_stamp;

void ap_peripheral_call_key_handler_register(
	ap_peripheral_call_key_handler_t handler)
{
	call_key_handler = handler;
}

static INT8U ap_peripheral_call_key_dispatch(
	ap_peripheral_call_key_action_t action)
{
	if (call_key_handler == NULL)
	{
		return 0;
	}
	return call_key_handler(action);
}
void ap_peripheral_media_key_handler_register(
	ap_peripheral_media_key_handler_t handler)
{
	media_key_handler = handler;
}

static INT8U ap_peripheral_media_key_dispatch(
	ap_peripheral_media_key_action_t action)
{
	if (media_key_handler == NULL)
	{
		return 0;
	}
	return media_key_handler(action);
}

#define C_RESISTOR_ACCURACY				5	// 2% accuracy
#define C_KEY_STABLE_THRESHOLD			2	// Defines threshold number that AD value of key is deemed stable
#define C_KEY_FAST_JUDGE_THRESHOLD		40	// Defines threshold number that key is should be judge before it is release. 0=Disable
#define C_KEY_RELEASE_STABLE_THRESHOLD	4	// Defines threshold number that AD value is deemed stable
INT16U				adc_key_value;

//static INT8U  ad_value_cnt ;
static INT32U	key_pressed_cnt;
static INT8U	fast_key_exec_flag;
static INT8U	normal_key_exec_flag;
static INT8U	long_key_exec_flag;

#if defined(IOKEY_NUM) && IOKEY_NUM > 0
static KEYSTATUS	io_key_map[IOKEY_NUM];
static INT8U		io_key_num=0;
#endif

static INT32U key_active_cnt;

static INT8U power_off_timerid;
static INT8U usbd_detect_io_timerid;
static INT8U key_detect_timerid;

#define USBD_DETECTION_DEBOUNCE		16	/* 32 = 1 second */
#define USBD_BATTERY_SHOW_WAIT_CNT	64	/* 32 = 1 second */
static INT8U usbd_connected_state = USBD_CONNECTED_EMPTY;

INT8U s_usbd_pin;

#if CRAZY_KEY_TEST
INT32U	crazy_key_enable;
INT32U	crazy_io_back_plug_in;
#endif // #if CRAZY_KEY_TEST

static INT8U lcd_bl_sts=0;

INT8U tp_status = 0;

#if C_AEAWB_INFO_STAMP
INT8U vid_info_flag = 1;
#endif

#if C_ADKEY_CALIBRATION
const INT16U adkey_cali_num = 100;
INT16U adkey_cali_cnt;
INT16U adkey_cali_min;
INT16U adkey_cali_max;
INT32U adkey_cali_avg;

#define adkey_statistic_max 4096
#define adkey_statistic_unit 16
INT8U adkey_statistic_cnt[adkey_statistic_max / adkey_statistic_unit];
#endif // #if C_ADKEY_CALIBRATION




/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

#if C_MOTION_DETECTION
#define	SENSETIVE_HIGHT		10
#define SENSETIVE_NORMAL	150
#define SENSETIVE_LOW		300

#define MDBLKCNT			SENSETIVE_LOW

INT8S	hit=0;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void ap_peripheral_motion_detect_judge(void)
{
	INT32U	result;

	result = drv_l1_CdspGetResult();

	//DBG_PRINT("MD_result = 0x%x\r\n",result);

	if(result > MDBLKCNT)	//max_cnt:1800
	{
		//DBG_PRINT(" %d-%d",result, hit);
		hit++;
		if (hit>1)
		{
			hit = 0;
			app_msg_send(MSG_APQ_MOTION_DETECT_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
		}
	}
	else
	{
		hit--;
		if(hit<0){
			hit = 0;
		}
	}

}

void ap_peripheral_motion_detect_start(void)
{
	motion_detect_status_set(MOTION_DETECT_STATUS_START);
}

void ap_peripheral_motion_detect_stop(void)
{
	motion_detect_status_set(MOTION_DETECT_STATUS_STOP);
}

#endif // C_MOTION_DETECTION == CUSTOM_ON

void ap_peripheral_ad_detect_init(INT8U adc_channel, void (*ad_detect_isr) (INT16U data))
{
#if C_BATTERY_DETECT == CUSTOM_ON
	battery_value_sum = 0;
	bat_ck_cnt = 0;
#endif

	/* ADKEY1..6 use the board's ADC_LINE_0 resistor ladder.  Do not
	 * disable this when LVGL is selected: LVGL consumes the resulting key
	 * actions through the normal peripheral handlers. */
#if ADKEY_NUM
	//ad_value_cnt = 0;
	drv_l1_adc_init(0,0);
	drv_l1_adc_vref_enable_set(TRUE);
	drv_l1_adc_conv_time_sel(4);
	drv_l1_adc_manual_ch_set(adc_channel);
	drv_l1_adc_manual_callback_set(ad_detect_isr);
	DBG_PRINT("[KEY] ADKEY1-6 scan enabled on ADC%d\r\n", adc_channel);
#endif
	if(ad_detect_timerid == 0xFF)
	{
		ad_detect_timerid = AD_DETECT_TIMER_ID;
		drv_l2_sys_timer_set((void *) msgQSend, (void *) PeripheralTaskQ, MSG_PERIPHERAL_TASK_POLL_32HZ, ad_detect_timerid, PERI_TIME_INTERVAL_AD_DETECT);
	}
}

void ap_peripheral_ad_check_isr(INT16U value)
{
	ad_value = value;
}

void ap_peripheral_clr_screen_saver_timer(void)
{
	key_active_cnt = 0;
}

static short Median_Filter(short adc_value)
{
	// 使用 static 變數來記住歷史數據 (窗格)
	static short window[3] = {0, 0, 0};
	short a, b, c;

	// 將舊資料左移，並將新資料放入窗格
	window[0] = window[1];
	window[1] = window[2];
	window[2] = adc_value;

	// 為了不影響原始窗格數據，複製到臨時變數進行排序
	a = window[0];
	b = window[1];
	c = window[2];

	// 進行排序，找出中位數
	short mid;
	if ((a <= b && b <= c) || (c <= b && b <= a)) {
		mid = b;
	} else if ((b <= a && a <= c) || (c <= a && a <= b)) {
		mid = a;
	} else {
		mid = c;
	}

	return mid;
}

#if ADKEY_NUM
INT8U adc_key_judge(INT32U adc_value)
{
	INT32U	candidate_key;
	INT32U	i;

	candidate_key = ad_key_num;

	for(i = 0; i < ad_key_num; i++)
	{
		//DBG_PRINT("i=%d, adc_value=%d, L=%d, H=%d\r\n", i, adc_value, ad_key_map[i].key_val_L, ad_key_map[i].key_val_H);
		if((adc_value >= ad_key_map[i].key_val_L) && (adc_value < ad_key_map[i].key_val_H))
		{
			candidate_key = i;

		}
	}

	if(candidate_key < ad_key_num)
	{
		DBG_PRINT("\r\nKey %d", candidate_key + 1);

		#if C_SCREEN_SAVER
		//DBG_PRINT("adc_key_judge,C_SCREEN_SAVER\r\n");
		key_active_cnt = 0;

		ap_peripheral_lcd_backlight_set(BL_ON);
		#endif // #if C_SCREEN_SAVER
	}

	return candidate_key;
}
#endif // #if ADKEY_NUM

#if ADKEY_NUM
/* Dispatch on the first valid ADC sample.  The resistor ladder already
 * identifies the key range here, and the per-press flag suppresses repeats
 * until it is released. */
static void ap_peripheral_ad_key_press_dispatch(void)
{
	INT8U pressed_key;

	if (normal_key_exec_flag || fast_key_exec_flag || long_key_exec_flag ||
		(adc_key_value == 0u))
	{
		return;
	}

	pressed_key = adc_key_judge(adc_key_value);
	if ((pressed_key < ad_key_num) &&
		(ad_key_map[pressed_key].key_function != NULL))
	{
		ad_key_map[pressed_key].key_cnt = 1u;
		ad_key_map[pressed_key].key_function(&(ad_key_map[pressed_key].key_cnt));
		normal_key_exec_flag = 1u;
	}
}
#endif

void ap_peripheral_ad_key_judge(void)
{
	INT32U t,diff;
	INT16U adc_current_value,adc_core_powr_value;
	INT16U battery_data;
	static int wait_cnt=0;

	#if C_BATTERY_LOW_POWER_OFF
	//INT16U adc_core_powr_value;
	#endif

	//DBG_PRINT("ap_peripheral_ad_key_judge()~~~~\r\n");
	//DBG_PRINT(".");
	t = xTaskGetTickCount();
	if((t - ad_time_stamp) < 2)
	{
		return;
	}
	ad_time_stamp = t;


	//select NEXT ADC channel
	ad_line_count++;
	if(ad_line_count <=31)
	{
		drv_l1_adc_manual_ch_set(ADKEY_DETECT_PIN);
	}
	else if (ad_line_count ==32)
	{
		drv_l1_adc_manual_ch_set(BATTERY_DETECT_PIN);
	}
	else if(ad_line_count ==33)
	{
        drv_l1_adc_manual_ch_set(CORE_POWER_DETECT_PIN);		//core power line  for  3v3  low power detect
		ad_line_count = 0;
	}


	#if C_BATTERY_LOW_POWER_OFF
	//use CORE POWER (1.2v)	detect
	if ((ad_value & ADC_CHANNEL_MASK ) == CORE_POWER_DETECT_PIN)
	{
		adc_core_powr_value = (ad_value >>4);
		DBG_PRINT("adc_core_powr_value= %d  \r\n",adc_core_powr_value);
		if(adc_core_powr_value >= CORE_POWER_THRESHOLD)
		{

			core_power_cnt++;

			//DBG_PRINT("core_power_cnt = %d  \r\n",core_power_cnt);
			if(core_power_cnt > CORE_POWER_THRESHOLD_CNT)
			{
				core_power_low_power_flag = 1;	//send to  ap_peripheral_battery_check_calculate
			}

		}
		else
		{
			core_power_cnt = 0;
		}

		drv_l1_adc_manual_sample_start();
		return;
	}
	#endif

	//battery adc detection
	if((ad_value & ADC_CHANNEL_MASK ) == BATTERY_DETECT_PIN)
	{
		battery_data = (ad_value >>4);
        //DBG_PRINT("battery_value =%d \r\n",battery_data);
		adc_battery_value_old = adc_battery_value_new;
		adc_battery_value_new = battery_data;

		if(adc_battery_value_new >= adc_battery_value_old)
		{
			diff = adc_battery_value_new - adc_battery_value_old;
		}
		else
		{
			diff = adc_battery_value_old - adc_battery_value_new;
		}

		if(!diff || (100 * diff <= C_RESISTOR_ACCURACY * adc_battery_value_old))
		{
			if(battery_stable_cnt < C_BATTERY_STABLE_THRESHOLD)
				battery_stable_cnt++;
		}
		else
		{
			battery_stable_cnt = 1;

			#if C_BATTERY_DETECT
			bat_ck_cnt = 0;
			battery_value_sum = 0;
			#endif
		}

		#if C_BATTERY_DETECT
		if(battery_stable_cnt >= C_BATTERY_STABLE_THRESHOLD)
		{
			//DBG_PRINT("adc_battery_value_new=%d\r\n", adc_battery_value_new);
			ap_peripheral_battery_check_calculate(battery_data);
		}
		#endif // #if C_BATTERY_DETECT

		drv_l1_adc_manual_sample_start();
		return;
	}

	//ad key detection
	if ( (ad_value & ADC_CHANNEL_MASK) == ADKEY_DETECT_PIN)
	{
		// get the value ; use Median filter
		adc_current_value = Median_Filter((ad_value >> 4));
		//DBG_PRINT("adc =%d %d\r\n",ad_value,adc_current_value);

		#if C_ADKEY_CALIBRATION
		if(adc_current_value < adkey_statistic_max)
		{
			if(!adkey_cali_cnt)
			{
				INT16U a_idx;
				DBG_PRINT(
					"C_ADKEY_CALIBRATION(avg,min,max)=(%d,%d,%d)\r\n"
					, adkey_cali_avg / adkey_cali_num, adkey_cali_min, adkey_cali_max
				);

				for(a_idx = 0 ; a_idx < adkey_statistic_max / adkey_statistic_unit ; ++a_idx)
				{
					if(adkey_statistic_cnt[a_idx])
					{
						DBG_PRINT("[%4d-%4d]=%4d\r\n"
							, a_idx * adkey_statistic_unit
							, (a_idx + 1) * adkey_statistic_unit - 1
							, adkey_statistic_cnt[a_idx]
						);
					}
				}

				adkey_cali_cnt = adkey_cali_num;
				adkey_cali_min = 0xFFFF;
				adkey_cali_max = 0;
				adkey_cali_avg = 0;
				gp_memset((INT8S*)adkey_statistic_cnt, 0, sizeof(adkey_statistic_cnt)/sizeof(adkey_statistic_cnt[0]));
			}
			//else
			{
				//DBG_PRINT("%d,", adc_current_value);
				--adkey_cali_cnt;
				adkey_cali_avg += adc_current_value;
				if(adkey_cali_min > adc_current_value)	adkey_cali_min = adc_current_value;
				if(adkey_cali_max < adc_current_value)	adkey_cali_max = adc_current_value;
				if(adc_current_value >= adkey_statistic_max)
				{
					++adkey_statistic_cnt[(adkey_statistic_max - 1)/adkey_statistic_unit];
				}
				else
				{
					++adkey_statistic_cnt[(adc_current_value - 1)/adkey_statistic_unit];
				}
			}

			//DBG_PRINT("AD_KEY_STEP: %d\r\n",adc_current_value);	//get AD_KEY_STEP @customer.h

		}
		#endif // #if C_ADKEY_CALIBRATION


		#if ADKEY_NUM

		// Key released
		if((adc_current_value < ad_key_thsr_l) || (adc_current_value > ad_key_thsr_h))
		{
			//DBG_PRINT("   %d\r\n", adc_current_value);
			// adc_key_value: last ADC(KEY) value
			if(adc_key_value)
			{
				// Key released
				if(!fast_key_exec_flag && !normal_key_exec_flag &&
					!long_key_exec_flag && key_pressed_cnt >= C_KEY_STABLE_THRESHOLD)
				{
					INT8U pressed_key;

					pressed_key = adc_key_judge(adc_key_value);
					if(pressed_key < ad_key_num && ad_key_map[pressed_key].key_function)
					{
						ad_key_map[pressed_key].key_function(&(ad_key_map[pressed_key].key_cnt));

						//DBG_PRINT("key_function key%d \r\n",pressed_key);
					}
				}

				adc_key_value = 0;
				key_pressed_cnt = 0;
				fast_key_exec_flag = 0;
				normal_key_exec_flag = 0;
				long_key_exec_flag = 0;
			}

			adc_key_release_value_old = adc_key_release_value_new;
			adc_key_release_value_new = adc_current_value;

			if(adc_key_release_value_new >= adc_key_release_value_old)
			{
				diff = adc_key_release_value_new - adc_key_release_value_old;
			}
			else
			{
				diff = adc_key_release_value_old - adc_key_release_value_new;
			}

			if(!diff || (100 * diff <= C_RESISTOR_ACCURACY * adc_key_release_value_old))
			{
				key_release_stable_cnt++;
			}
			else
			{
				key_release_stable_cnt = 1;
			}

			if(key_release_stable_cnt >= C_KEY_RELEASE_STABLE_THRESHOLD)
			{
				adc_key_release_value_stable = (adc_key_release_value_new + adc_key_release_value_old) >> 1;

				//DBG_PRINT("%d,", adc_key_release_value_stable);
				key_release_stable_cnt = 1;
			}
		}

		// Key pressed
		else
		{

			//DBG_PRINT("   %d  key pressed \r\n", adc_current_value);
			if(adc_current_value >= adc_key_value)
			{
				diff = adc_current_value - adc_key_value;
			}
			else
			{
				diff = adc_key_value - adc_current_value;
			}

			if(!diff || (100 * diff <= C_RESISTOR_ACCURACY * adc_key_value))
			{
				key_pressed_cnt++;

				/* ADKEY1..6 are UI/media controls.  Dispatch once after two
				 * stable ADC samples instead of waiting for release, so a held
				 * button cannot accidentally become a long-press-only action. */
				if (!normal_key_exec_flag && !fast_key_exec_flag &&
					!long_key_exec_flag &&
					(key_pressed_cnt >= C_KEY_STABLE_THRESHOLD))
				{
					INT8U pressed_key = adc_key_judge(adc_key_value);

					if ((pressed_key < ad_key_num) &&
						(ad_key_map[pressed_key].key_function != NULL))
					{
						ad_key_map[pressed_key].key_cnt = (INT16U)key_pressed_cnt;
						ad_key_map[pressed_key].key_function(
							&(ad_key_map[pressed_key].key_cnt));
						normal_key_exec_flag = 1;
					}
				}

				// TBD: Handle zoom function
				//if (zoom_key_flag && ad_key_cnt>2 && (adkey_lvl == ADKEY_PREV_L || adkey_lvl == ADKEY_NEXT_L)) {

				#if C_KEY_FAST_JUDGE_THRESHOLD
				if(key_pressed_cnt == C_KEY_FAST_JUDGE_THRESHOLD)
				{
					// fast key(long pressed),more than 2 sec
					INT8U pressed_key;

					pressed_key = adc_key_judge(adc_key_value);
					if(pressed_key < ad_key_num && ad_key_map[pressed_key].fast_key_fun)
					{
						ad_key_map[pressed_key].fast_key_fun(&(ad_key_map[pressed_key].key_cnt));
						fast_key_exec_flag = 1;
					}
				}
				#endif // #if C_KEY_FAST_JUDGE_THRESHOLD

				//DBG_PRINT("key_pressed_cnt = %d  \r\n",key_pressed_cnt);
				//long key
				if((key_pressed_cnt == 28) && !normal_key_exec_flag)
				{
					INT8U pressed_key;

					pressed_key = adc_key_judge(adc_key_value);
					if(pressed_key < ad_key_num && ad_key_map[pressed_key].key_function)
					{
						ad_key_map[pressed_key].key_cnt = key_pressed_cnt;

						//DBG_PRINT("long key cnt = %d  \r\n",ad_key_map[pressed_key].key_cnt);
						ad_key_map[pressed_key].key_function(&(ad_key_map[pressed_key].key_cnt));

						long_key_exec_flag = 1;
						adc_key_value = 0;
						//key_pressed_cnt = 0; //long key exec only once
						fast_key_exec_flag = 0;
						normal_key_exec_flag = 0;

					}
				}

				// Key released or not stable
			}
			else
			{
				if(!fast_key_exec_flag && !normal_key_exec_flag)
				{
					adc_key_value = adc_current_value;
					key_pressed_cnt = 1u;
					ap_peripheral_ad_key_press_dispatch();
				}
			}
		}

		#endif // #if ADKEY_NUM
	}

	drv_l1_adc_manual_sample_start();
}

INT32U ap_peripheral_battery_level_get(void)
{
	#if C_BATTERY_DETECT
	return bat_lvl_cal_bak;
	#else
	return BAT_LVL_NUM - 1;
	#endif
}

#if C_BATTERY_DETECT
static BAT_LVL ap_peripheral_smith_trigger_battery_level(INT32U charging)
{
	static const INT32U battery_lvl_thsr[BAT_LVL_NUM] =
	{
		BATTERY_LVL_THSR_0,
		BATTERY_LVL_THSR_1,
		BATTERY_LVL_THSR_2,
		BATTERY_LVL_THSR_3
	};

	BAT_LVL	bat_lvl_cal = BAT_LVL_0;
	INT32U	i;

	#if BATTERY_CNT_WAY
	static INT8U	first_read_flag = 1;
	static INT32U	battery_low_pass[BATTERY_DET_CNT]={0};
	static INT32U	Bat_det_cnt_index = 0;
	#endif // #if BATTERY_CNT_WAY


	#if BATTERY_CNT_WAY
	if(first_read_flag == 1)
	{
		first_read_flag = 0;

		//first initial
		for(i = 0; i < BATTERY_DET_CNT; i++)
		{
			battery_low_pass[i] = battery_value_sum;
		}
	}
	else
	{
		battery_low_pass[Bat_det_cnt_index] =  battery_value_sum;
		Bat_det_cnt_index++;
		if(Bat_det_cnt_index >= BATTERY_DET_CNT)
		{
			Bat_det_cnt_index = 0;
		}
	}

	battery_value_sum = 0;
	for(i = 0; i < BATTERY_DET_CNT; i++)
	{
		battery_value_sum += battery_low_pass[i];

		//for debug
		//DBG_PRINT("battery_low_pass[%d]  = (%d)\r\n",i,battery_low_pass[i]);
	}

	//DBG_PRINT("\r\n");
	#endif // #if BATTERY_CNT_WAY

	//DBG_PRINT("(%d)\r\n", battery_value_sum);
	//DBG_PRINT("average battery value = %d\r\n", battery_value_sum/BATTERY_DET_CNT);

	for(i = 0; i < BAT_LVL_NUM; i++)
	{
		if(battery_value_sum > battery_lvl_thsr[i] * BATTERY_DET_CNT)
		{
			bat_lvl_cal = i;
		}
	}

	//high voltage to low voltage
	if((charging == 0) && (bat_lvl_cal > bat_lvl_cal_bak))
	{
		//for(i = BAT_LVL_NUM; i == 0; i--)
		for(i = 0; i < BAT_LVL_NUM ; i++)
		{
			if(battery_value_sum > battery_lvl_thsr[i] * BATTERY_DET_CNT + BATTERY_LVL_GAP * BATTERY_DET_CNT)
			{
				bat_lvl_cal = i;
			//	DBG_PRINT("LOOP high to low %d\r\n", bat_lvl_cal);
			}
		}
	}

	//low voltage to high voltage
	if((charging == 1) && (bat_lvl_cal < bat_lvl_cal_bak))
	{
		bat_lvl_cal = BAT_LVL_0;
		for(i = 1; i < BAT_LVL_NUM; i++)
		{

			if(battery_value_sum < battery_lvl_thsr[i] * BATTERY_DET_CNT  - BATTERY_LVL_GAP * BATTERY_DET_CNT)
			{
				bat_lvl_cal = i;
			//	DBG_PRINT("LOOP low to high %d\r\n", bat_lvl_cal);
			}
		}
	}

	bat_lvl_cal_bak = bat_lvl_cal;
	return bat_lvl_cal;
}

void ap_peripheral_battery_check_calculate(INT16U battery_value)
{
	static INT32U	keep_battery_value = (BATTERY_LVL_THSR_1 + BATTERY_LVL_THSR_2) / 2;
	static INT32U	battery_judge = 0;
	static INT8U	battery_low_flag = 0;
	static INT32U	charger = 0;
	//static INT32U	charger_sta = 0;
	//static INT32U	charger_cnt = 0;
	static BAT_LVL	bat_lvl_cal = BAT_LVL_UNDEF;

	#ifdef CHARGER_IO_LED
	INT8U charge_full_flag=0;
	#endif

	if(battery_low_flag)
	{
		return;
	}

	if(ap_peripheral_get_usbd_io_status())
	{
		/* Charger plug in */
		if(charger == 0)
		{
			//low voltage to high voltage
			charger = 1;

			#ifdef CHARGER_IO_LED
			#if (CHARGER_IO_ACTIVE == DATA_LOW)
			gpio_write_io(CHARGER_IO_LED, DATA_LOW);	//charger LED on
			#else
			gpio_write_io(CHARGER_IO_LED, DATA_HIGH);	//charger LED on
			#endif // #if (CHARGER_IO_ACTIVE == DATA_LOW)
			#endif // #ifdef CHARGER_IO_LED

			#if C_BATTERY_DETECT
			app_msg_send(MSG_APQ_BATTERY_CHARGED_SHOW, NULL, 0, MSG_PRI_NORMAL);
			#endif
		}
#if 0
		else
		{
			/* Check charging status, 0 = charging, 1= end of charging */
			if(drv_l1_sys_chr_get_status() && charger_sta == 0)
			{
				if(charger_cnt++ > 1)
				{
					DBG_PRINT("AP charger check, end of charging status\r\n");
					msgQSend(ApQ, MSG_APQ_BATTERY_CHARGED_CLEAR, NULL, 0, MSG_PRI_NORMAL);
					charger_sta = 1;
					charger_cnt = 0;
				}
			}
			else if(!drv_l1_sys_chr_get_status() && charger_sta == 1)
			{
				/* charger is charging */
				if(charger_cnt++ > 1)
				{
					DBG_PRINT("AP charger check, charging status\r\n");
					charger_sta = 0;
					charger_cnt = 0;
				}
			}
		}
#endif
	}
	else
	{
		//adaptor out state
		if(charger == 1)
		{
			#ifdef CHARGER_IO_LED
			#if (CHARGER_IO_ACTIVE == DATA_LOW)
			gpio_write_io(CHARGER_IO_LED, DATA_HIGH);	//charger LED off
			#else
			gpio_write_io(CHARGER_IO_LED, DATA_LOW);	//charger LED off
			#endif // #if (CHARGER_IO_ACTIVE == DATA_LOW)
			#endif // #ifdef CHARGER_IO_LED

			//high voltage to low voltage
			charger = 0;
			//charger_cnt = 0;
			//charger_sta = 0;

			#if C_BATTERY_DETECT
			app_msg_send(MSG_APQ_BATTERY_CHARGED_CLEAR, NULL, 0, MSG_PRI_NORMAL);
			#endif
		}
	}



	// judge ADC threshold
	#if BATTERY_CNT_WAY == 0

	battery_judge = battery_value;

	//check adkey pin is the same battery pin?
	if(ADKEY_DETECT_PIN == BATTERY_DETECT_PIN)
	{
		if(battery_judge < ad_key_thsr_l)
		{
			battery_value_sum += battery_judge;
		}
	}
	else
	{
		battery_value_sum += battery_judge;
	}

	//for Debug
	//DBG_PRINT("bat_ck_cnt=[%d]	battery_value =%d\r\n", bat_ck_cnt, battery_value );

	#else // #if BATTERY_CNT_WAY == 0

	battery_judge = battery_value;

	//for Debug
	//DBG_PRINT("bat_ck_cnt=[%d]	battery_value =%d\r\n", bat_ck_cnt, battery_value );

	if(ADKEY_DETECT_PIN == BATTERY_DETECT_PIN)		//check adkey pin is the same battery pin?
	{
		if(battery_judge < ad_key_thsr_l)
		{
			battery_value_sum = battery_judge;				//update new value
			keep_battery_value = battery_judge;
		}
		else
		{
			battery_value_sum = keep_battery_value ;		// Keep before value
		}
	}
	else
	{
		battery_value_sum = battery_judge;					//update new value
	}

	#endif // #if BATTERY_CNT_WAY == 0

	#ifdef CHARGER_IO_LED
	//get the charge value
	charge_full_flag = drv_l1_sys_chr_get_status_method(2);
	if(charger == 1)
	{
		if(charge_full_flag == 1)
		{
			// DBG_PRINT("charge full\r\n");
			#if (CHARGER_IO_ACTIVE == DATA_LOW)
			gpio_write_io(CHARGER_IO_LED, DATA_HIGH);	//charger LED off
			#else
			gpio_write_io(CHARGER_IO_LED, DATA_LOW);	//charger LED off
			#endif
		}
		else
		{
			// DBG_PRINT("charge battery\r\n");
			#if (CHARGER_IO_ACTIVE == DATA_LOW)
			gpio_write_io(CHARGER_IO_LED, DATA_LOW);	//charger LED on
			#else
			gpio_write_io(CHARGER_IO_LED, DATA_HIGH);	//charger LED on
			#endif
		}
	}
	#endif // #ifdef CHARGER_IO_LED


	#if BATTERY_CNT_WAY == 0
	if(battery_judge < ad_key_thsr_l)
	{
		bat_ck_cnt++;
	}
	#endif

	#if BATTERY_CNT_WAY == 0
	if(bat_ck_cnt >= BATTERY_DET_CNT)
	#endif
	{

		BAT_LVL bat_lvl_tmp = ap_peripheral_smith_trigger_battery_level(charger);
		//DBG_PRINT("bat_lvl_tmp=%d\r\n", bat_lvl_tmp);

		if(bat_lvl_cal != bat_lvl_tmp)
		{
			bat_lvl_cal = bat_lvl_tmp;
			//DBG_PRINT("bat_lvl_cal=%d\r\n", bat_lvl_cal);

			#if C_BATTERY_DETECT
			if(bat_lvl_cal < BAT_LVL_NUM)
			{
				app_msg_send(MSG_APQ_BATTERY_LVL_SHOW, &bat_lvl_cal, sizeof(BAT_LVL), MSG_PRI_NORMAL);
			}
			#endif
		}

		#if C_BATTERY_LOW_POWER_OFF
		//core power flag check
		if(core_power_low_power_flag == 1)
		{
			DBG_PRINT("3V3 low voltage.\r\n");
			battery_low_flag = 1;
		}

		//use VBAT voltage detect
		if(bat_lvl_cal == 0 && charger == 0)
		{
			//DBG_PRINT("LOW BATTERY CNT = %d  \r\n",low_voltage_cnt);
			if(++low_voltage_cnt > LOW_VOL_TRESHOLD)
				battery_low_flag = 1;
		}

		if(battery_low_flag)	//low power ! must shutdown .
		{
			//INT8U type = KEY_MODE_BETTERY_LOW;

			DBG_PRINT("LOW BATTERY or 3V3 low voltage & NO charger\r\n");

			storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_STOP, NULL, 0, MSG_PRI_NORMAL);
			//msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_KEY_REGISTER, &type, sizeof(INT8U), MSG_PRI_NORMAL);
			app_msg_send(MSG_APQ_BATTERY_LOW_SHOW, NULL, 0, MSG_PRI_NORMAL);
			//DBG_PRINT("send1 MSG_APQ_BATTERY_LOW_SHOW\r\n");
		}

		#endif // #if C_BATTERY_LOW_POWER_OFF


		#if BATTERY_CNT_WAY == 0
		bat_ck_cnt = 0;
		battery_value_sum = 0;
		#endif
	}

}
#endif // C_BATTERY_DETECT

void ap_peripheral_auto_off_force_disable_set(INT8U auto_off_disable)
{
	auto_off_force_disable = auto_off_disable;
}

void ap_peripheral_lcd_backlight_set(INT8U type)
{
	if(type == BL_ON)
	{
		if(lcd_bl_sts)
		{
			lcd_bl_sts = 0;
			drv_l2_tft_backlight_en_set(TRUE);
			DBG_PRINT("\r\nLCD ON");
		}
	}
	else
	{
		if(!lcd_bl_sts)
		{
			lcd_bl_sts = 1;
			drv_l2_tft_backlight_en_set(FALSE);
			DBG_PRINT("\r\nLCD OFF");
		}
	}
}

INT32U ap_peripheral_lcd_backlight_get(void)
{
	return lcd_bl_sts;
}

void ap_peripheral_night_mode_set(INT8U flash_light_switch)
{
	// Note : Use 2 IO drving IR LED

	#ifdef C_FLASH_LIGHT
	gpio_init_io(FLASH_LIGHT, GPIO_OUTPUT);		//IOC14
	gpio_init_io(FLASH_LIGHT_2, GPIO_OUTPUT);	//IOC15

	gpio_set_port_attribute(FLASH_LIGHT, ATTRIBUTE_HIGH);
	gpio_set_port_attribute(FLASH_LIGHT_2, ATTRIBUTE_HIGH);

	if(flash_light_switch==0x1)
	{
		DBG_PRINT("FLASH LIGHT ON \r\n");
		if(FLASH_LIGHT_ACTIVE==DATA_LOW)
		{
			gpio_write_io(FLASH_LIGHT, DATA_LOW);
			gpio_write_io(FLASH_LIGHT_2, DATA_LOW);

			DBG_PRINT("R_IOC_O_DATA = 0x%x \r\n",R_IOC_O_DATA);
			DBG_PRINT("R_IOC_DIR = 0x%x \r\n",R_IOC_DIR);
			DBG_PRINT("R_IOC_ATT = 0x%x \r\n",R_IOC_ATT);
			DBG_PRINT("R_IOC_DRV = 0x%x \r\n",R_IOC_DRV);

		}
		else
		{
			gpio_write_io(FLASH_LIGHT, DATA_HIGH);
			gpio_write_io(FLASH_LIGHT_2, DATA_HIGH);
		}
	}
	else
	{
		DBG_PRINT("FLASH LIGHT OFF \r\n");
		if(FLASH_LIGHT_ACTIVE==DATA_LOW)
		{
			gpio_write_io(FLASH_LIGHT, DATA_HIGH);
			gpio_write_io(FLASH_LIGHT_2, DATA_HIGH);
		}
		else
		{
			gpio_write_io(FLASH_LIGHT, DATA_LOW);
			gpio_write_io(FLASH_LIGHT_2, DATA_LOW);
		}
	}
	#endif // #ifdef C_FLASH_LIGHT
}

void ap_peripheral_key_judge(void)
{
#if defined(IOKEY_NUM) && IOKEY_NUM > 0
	const INT32U long_press_interval = 128 / PERI_TIME_INTERVAL_AD_DETECT;
	static INT8U long_pw_key_pressed[IOKEY_NUM] = {0};
	INT32U i, j;
#if C_SCREEN_SAVER
	INT8U key_press = 0;
#endif
	static INT8U median[IOKEY_NUM][5] = {{0}};
	for(i = 0; i < io_key_num; i++)
	{
		INT32U	status, sum = 0;
#if C_SCREEN_SAVER
		key_press = 0;
#endif
		for(j = sizeof(median[0]) / sizeof(median[0][0]) - 1; j > 0; j--){
			median[i][j] = median[i][j - 1];
			sum += median[i][j];
		}
		// read from pwr pin
		if((PW_KEY_TYPE == READ_FROM_PIN) && (io_key_map[i].key_val_L == PW_KEY))
		{
			#if PW_KEY_ACTIVE == DATA_HIGH
			median[i][0] = drv_l1_sys_pwr_key_read(io_key_map[i].key_val_L);
			#else
			median[i][0] = !drv_l1_sys_pwr_key_read(io_key_map[i].key_val_L);
			#endif
		}
		// read from io
		else
		{
			median[i][0] = gpio_read_io(io_key_map[i].key_val_L);
		}

		if((sum + median[i][0]) >= (sizeof(median[0]) / sizeof(median[0][0]) / 2 + 1)){
			status = 1;
		}
		else{
			status = 0;
		}

		if(status)
		{
			io_key_map[i].key_cnt += 1;
			if(io_key_map[i].key_cnt > long_press_interval)
			{
				long_pw_key_pressed[i] = 1;
				io_key_map[i].key_function(&(io_key_map[i].key_cnt));
				io_key_map[i].key_cnt = 0;
			}
		}
		else
		{
			if(io_key_map[i].key_cnt >= 3 && !long_pw_key_pressed[i])
			{
				io_key_map[i].key_function(&(io_key_map[i].key_cnt));
#if C_SCREEN_SAVER
				key_press = 1;
#endif
			}

			long_pw_key_pressed[i] = 0;

			io_key_map[i].key_cnt = 0;

			#if C_SCREEN_SAVER
			{
				static INT8U auto_off_force_disable_bak = 0;

				INT32U screen_auto_off_TFT_BL;
				INT32U cnt_sec;
				INT32U screen_auto_off, md_status;
				INT32U cnt_clr_flag = 0;

				if(auto_off_force_disable != auto_off_force_disable_bak)
				{
					// video recording & video playing
					key_active_cnt = 0;
					auto_off_force_disable_bak = auto_off_force_disable;
				}

				screen_auto_off = ap_state_config_auto_off_get();
				md_status = ap_state_config_md_get();

				if((screen_auto_off != 0) && !auto_off_force_disable && !s_usbd_pin && !md_status && !tp_status)
				{
					//don't auto off under following conditions:
					// 1. recording & playing avi files(by auto_off_force_disable)
					// 2. usb connecting
					// 3. motion detect on
					if(screen_auto_off == 1)
					{
						// 1 min
						screen_auto_off = 1;
					}
					else
					if(screen_auto_off == 2)
					{
						// 3 min
						screen_auto_off = 3;
					}

					if(cnt_clr_flag == 0)
					{
						key_active_cnt += PERI_TIME_INTERVAL_AD_DETECT;
					}

					cnt_sec = (key_active_cnt >> 7) / io_key_num;
					if(cnt_sec > screen_auto_off * 60)
					{
						key_active_cnt = 0;

						if(screen_saver_enable)
						{
							//DBG_PRINT("screen_auto_off:MSG_APQ_KEY_WAKE_UP\r\n");
							screen_saver_enable = 0;
							app_msg_send(MSG_APQ_KEY_WAKE_UP, NULL, 0, MSG_PRI_NORMAL);
						}

						#if C_IDLE_POWER_OFF

						DBG_PRINT("\r\nNO active Auto Power off\r\n");

						#if CRAZY_KEY_TEST
						if(!crazy_key_enable)
						{
							app_msg_send(MSG_APQ_POWER_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
						}
						#else
						app_msg_send(MSG_APQ_POWER_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
						#endif

						#endif // #if C_IDLE_POWER_OFF
					}

					cnt_clr_flag++;
				}

				// 0=>off, 1=>3min, 2=>5min, 3=>10min
				screen_auto_off_TFT_BL = ap_state_config_auto_off_TFT_BL_get();
				if((screen_saver_enable == 0) && (screen_auto_off_TFT_BL != 0))
				{
					if(screen_auto_off_TFT_BL == 1)
					{ // 3min
						screen_auto_off_TFT_BL = 3;
					}
					else
					if(screen_auto_off_TFT_BL == 2)
					{ // 5 min
						screen_auto_off_TFT_BL = 5;
					}
					else
					if(screen_auto_off_TFT_BL == 3)
					{ // 10min
						screen_auto_off_TFT_BL = 10;
					}

					if(cnt_clr_flag == 0)
					{
						key_active_cnt += PERI_TIME_INTERVAL_AD_DETECT;
					}

					cnt_sec = (key_active_cnt >> 7) / io_key_num;

					if(cnt_sec > screen_auto_off_TFT_BL * 60)
					{
						key_active_cnt = 0;

						// trun off Backlight
						ap_state_handling_lcd_backlight_switch(0);
						msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_SCREEN_SAVER_ENABLE, NULL, 0, MSG_PRI_NORMAL);
						DBG_PRINT("\r\nTurn off Backlight");
					}

					cnt_clr_flag++;
				}
				//  DBG_PRINT("SAVER = %d  tp_status=%d \r\n",screen_saver_enable,tp_status);
				if((screen_saver_enable & key_press)|| (screen_saver_enable & tp_status))
				{
                    if(io_key_map[i].key_val_L != PW_KEY)
                    {
                        screen_saver_enable = 0;
                        app_msg_send(MSG_APQ_KEY_WAKE_UP, NULL, 0, MSG_PRI_NORMAL);
                    }
				}

				if(cnt_clr_flag == 0)
				{
					key_active_cnt = 0;
				}
				tp_status = 0; // clr touch panel  status
			}
			#endif // #if C_SCREEN_SAVER

		}
	}

#endif // defined(IOKEY_NUM) && IOKEY_NUM > 0
}

int ap_peripheral_power_key_read(int pin)
{
	INT32U	status = 0;
	status = gpio_read_io(pin);

	if(status != 0)
		return 1;
	else
		return 0;
}

void ap_peripheral_mode_key_exe(INT16U *tick_cnt_ptr)
{
	INT16U tick_cnt = *tick_cnt_ptr;

	*tick_cnt_ptr = 0;

	DBG_PRINT("mode_Key\r\n");

	if(tick_cnt > 24)
	{
		;
	}
	else
	{
		/* KEY3 (MODE_KEY): when AVRCP is connected it advances the track.
		 * If Bluetooth does not claim the key, preserve the original page
		 * switching behaviour below. */
		if (ap_peripheral_media_key_dispatch(
			AP_PERIPHERAL_MEDIA_KEY_NEXT))
		{
			return;
		}
		app_msg_send(MSG_APQ_AUDIO_EFFECT_MODE, NULL, 0, MSG_PRI_NORMAL);

        #if C_SCREEN_SAVER
        if(screen_saver_enable)
        {
            screen_saver_enable = 0;
            app_msg_send(MSG_APQ_KEY_WAKE_UP, NULL, 0, MSG_PRI_NORMAL);
            return;
        }
        #endif

		DBG_PRINT("MODE_ACTIVE\r\n");
		DBG_PRINT("tick_cnt=%d\r\n", tick_cnt);
		app_msg_send(MSG_APQ_MODE, NULL, 0, MSG_PRI_NORMAL);
	}

}

void ap_peripheral_next_key_exe(INT16U *tick_cnt_ptr)
{
	INT16U	tick_cnt = *tick_cnt_ptr;
	INT8U	data = 0;

	*tick_cnt_ptr = 0;

	if(tick_cnt > 24)
	{
		#if C_AEAWB_INFO_STAMP
		if (vid_info_flag != 1)
		{
			vid_info_flag =1;
		}
		else
		{
			vid_info_flag = 0;
		}

		DBG_PRINT("video info en:%d\r\n", vid_info_flag);
		app_msg_send(MSG_APQ_AUDIO_EFFECT_OK, NULL, 0, MSG_PRI_NORMAL);
		#endif // #if C_AEAWB_INFO_STAMP

		app_msg_send(MSG_APQ_FORWARD_FAST_PLAY, &data, sizeof(INT8U), MSG_PRI_NORMAL);
	}
	else
	{
		if (ap_peripheral_media_key_dispatch(
			AP_PERIPHERAL_MEDIA_KEY_NEXT))
		{
			return;
		}        app_msg_send(MSG_APQ_AUDIO_EFFECT_DOWN, NULL, 0, MSG_PRI_NORMAL);

        #if C_SCREEN_SAVER
        if(screen_saver_enable)
        {
            screen_saver_enable = 0;
            app_msg_send(MSG_APQ_KEY_WAKE_UP, NULL, 0, MSG_PRI_NORMAL);
            return;
        }
        #endif

		app_msg_send(MSG_APQ_NEXT_KEY_ACTIVE, &data, sizeof(INT8U), MSG_PRI_NORMAL);
	}
}

void ap_peripheral_prev_key_exe(INT16U *tick_cnt_ptr)
{
	INT16U tick_cnt = *tick_cnt_ptr;
	INT8U data = 0;

	*tick_cnt_ptr = 0;
    DBG_PRINT("ap_peripheral_prev_key_exe\r\n");
	if(tick_cnt > 24)
	{
		#if C_WIFI_ON_OFF_KEY
		#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
		if((ap_state_handling_display_path_get() == DISPLAY_PATH_NORMAL) && present_state <= STATE_BROWSE)
		#else
		if(present_state <= STATE_BROWSE)
		#endif
		{
			if(state_wifi_status_get() & FLAG_CONNECT)
			{
				state_wifi_disconnect();
				DBG_PRINT("==DISABLE WIFI==\r\n");
			}
			else
			{
				if(present_state < STATE_BROWSE)
				{
					state_wifi_connect();
					DBG_PRINT("==ENABLE WIFI==\r\n");
				}
			}
		}
		#endif
		app_msg_send(MSG_APQ_BACKWORD_FAST_PLAY, &data, sizeof(INT8U), MSG_PRI_NORMAL);
	}
	else
	{
		/* KEY1 (PRE_KEY): previous Bluetooth track when AVRCP is available. */
		if (ap_peripheral_media_key_dispatch(
			AP_PERIPHERAL_MEDIA_KEY_PREVIOUS))
		{
			return;
		}
        app_msg_send(MSG_APQ_AUDIO_EFFECT_UP, NULL, 0, MSG_PRI_NORMAL);

        #if C_SCREEN_SAVER
        if(screen_saver_enable)
        {
            screen_saver_enable = 0;
            app_msg_send(MSG_APQ_KEY_WAKE_UP, NULL, 0, MSG_PRI_NORMAL);
            return;
        }
        #endif

		#if C_ASYN_DBLE_TFT  /* customize */
		if(present_state <= STATE_PHOTO_CAPTURE)	/* limiting of tft switching  in Photo or Record mode */
		{
				DBG_PRINT("MSG_DISPLAY_DUAL_SCREEN_SWITCH\r\n");
			display_task_msg_send(MSG_DISPLAY_DUAL_SCREEN_SWITCH, NULL, 0, MSG_PRI_NORMAL);
			return;		/* do not execute the MSG_APQ_NEXT_KEY_ACTIVE */
		}
		#endif

		app_msg_send(MSG_APQ_PREV_KEY_ACTIVE, &data, sizeof(INT8U), MSG_PRI_NORMAL);
	}

}

void ap_peripheral_ok_key_exe(INT16U *tick_cnt_ptr)
{
	INT16U tick_cnt = *tick_cnt_ptr;

	*tick_cnt_ptr = 0;

	if(tick_cnt > 24)
	{
		#if CRAZY_KEY_TEST
		crazy_key_enable= !crazy_key_enable;
		if(crazy_key_enable)
		{
			DBG_PRINT("Crazy key enable!!!\r\n");
		}
		else
		{
			DBG_PRINT("Crazy key disable!!!\r\n");
		}
		#endif
	}
	else
	{
		/* KEY2 (OK_KEY): Bluetooth play / pause when AVRCP is available. */
		if (ap_peripheral_media_key_dispatch(
			AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE))
		{
			return;
		}
        app_msg_send(MSG_APQ_AUDIO_EFFECT_OK, NULL, 0, MSG_PRI_NORMAL);

        #if C_SCREEN_SAVER
        if(screen_saver_enable)
        {
            screen_saver_enable = 0;
            app_msg_send(MSG_APQ_KEY_WAKE_UP, NULL, 0, MSG_PRI_NORMAL);
            return;
        }
        #endif

		app_msg_send(MSG_APQ_FUNCTION_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
	}
}

void ap_peripheral_sos_key_exe(INT16U *tick_cnt_ptr)
{
	INT16U tick_cnt = *tick_cnt_ptr;

	*tick_cnt_ptr = 0;

	#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
	/* ADKEY6: move right from the dashboard to the music page. */
	if ((tick_cnt <= 24u) && lvgl_page_show_music())
	{
		return;
	}
	#endif

	if (ap_peripheral_call_key_dispatch(
		AP_PERIPHERAL_CALL_KEY_HANGUP))
	{
		return;
	}

	#if C_SCREEN_SAVER
	if(screen_saver_enable)
	{
		screen_saver_enable = 0;
		app_msg_send(MSG_APQ_KEY_WAKE_UP, NULL, 0, MSG_PRI_NORMAL);
		return;
	}
	#endif

	if(tick_cnt > 24)
	{
	}
	else
	{
		app_msg_send(MSG_APQ_SOS_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
	}
}

void ap_peripheral_pw_key_exe(INT16U *tick_cnt_ptr)
{
	INT16U tick_cnt = *tick_cnt_ptr;

	*tick_cnt_ptr = 0;


	#if C_SCREEN_SAVER
	if(screen_saver_enable)
	{
		screen_saver_enable = 0;
		app_msg_send(MSG_APQ_KEY_WAKE_UP, NULL, 0, MSG_PRI_NORMAL);
		return;
	}
	#endif

	if(tick_cnt > 24)
	{
		DBG_PRINT("Power key OFF\r\n");
		app_msg_send(MSG_APQ_POWER_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
	}
	else
	{
		app_msg_send(MSG_APP_SCREEN_BACKLIGHT_SWITCH, NULL, 0, MSG_PRI_NORMAL);

		/*#if C_GSENSOR_FUNCTION
		app_msg_send(MSG_APQ_PARK_MODE_SET, NULL, 0, MSG_PRI_NORMAL);
		#endif*/
	}
}

void ap_peripheral_menu_key_exe(INT16U *tick_cnt_ptr)
{
	INT16U tick_cnt = *tick_cnt_ptr;

	*tick_cnt_ptr = 0;

	if(tick_cnt > 24)
	{
		#if C_SUPORT_GET_JPG_BUF
		app_msg_send( MSG_APQ_GET_JPEG_ON_RECORDING, NULL, 0, MSG_PRI_NORMAL);
		#endif
	}
	else
	{
		#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
		/* ADKEY5: move left from the music page back to the dashboard. */
		if (lvgl_page_show_dashboard())
		{
			return;
		}
		#endif

		if (ap_peripheral_call_key_dispatch(
			AP_PERIPHERAL_CALL_KEY_ANSWER))
		{
			return;
		}
		app_msg_send(MSG_APQ_AUDIO_EFFECT_MENU, NULL, 0, MSG_PRI_NORMAL);

		#if C_SCREEN_SAVER
		if(screen_saver_enable)
		{
			screen_saver_enable = 0;
			app_msg_send(MSG_APQ_KEY_WAKE_UP, NULL, 0, MSG_PRI_NORMAL);
			return;
		}
		#endif

		app_msg_send(MSG_APQ_MENU_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
	}

}

void ap_peripheral_cam_key_exe(INT16U *tick_cnt_ptr)
{
	INT16U tick_cnt = *tick_cnt_ptr;
	INT8U data = 0;

	*tick_cnt_ptr = 0;

	app_msg_send(MSG_APQ_AUDIO_EFFECT_UP, NULL, 0, MSG_PRI_NORMAL);

	#if C_SCREEN_SAVER
	if(screen_saver_enable)
	{
		screen_saver_enable = 0;
		app_msg_send(MSG_APQ_KEY_WAKE_UP, NULL, 0, MSG_PRI_NORMAL);
		return;
	}
	#endif

	if(tick_cnt > 24)
	{
	}
	else
	{
		app_msg_send(MSG_APQ_CAM_MODE_CHANGE, &data, sizeof(INT8U), MSG_PRI_NORMAL);
	}

}

INT8U ap_peripheral_get_usbd_io_status(void)
{
#if USBD_DET_PIN_TYPE == READ_FROM_PIN
	#if USBD_DET_PIN_ACTIVE == DATA_HIGH
	return (drv_l1_sys_pwr_key_read(USBD_DET_PIN)?1:0);
	#else
	return (drv_l1_sys_pwr_key_read(USBD_DET_PIN)?0:1);
	#endif
#else
	#if USBD_DET_PIN_ACTIVE == DATA_HIGH
	return (gpio_read_io(USBD_DET_PIN)?1:0);
	#else
	return (gpio_read_io(USBD_DET_PIN)?0:1);
	#endif
#endif
}

INT8U ap_peripheral_get_usbd_connection_state(void)
{
	return usbd_connected_state;
}

void ap_peripheral_set_usbd_connection_state(INT8U state)
{
	usbd_connected_state = state;
}

void ap_peripheral_usbd_io_detection(void)
{
	static INT32U cnt = 0;
#if defined(C_CARCAM_UVC_DISPLAY) && C_CARCAM_UVC_DISPLAY == CUSTOM_OFF
	static INT32U power_off_cnt = 0;
#endif
	INT32U cur_key_state = 0;
	static INT32U pre_key_state = 0;
	INT32U key_changed = 0;

	cur_key_state = ap_peripheral_get_usbd_io_status();

	//DBG_PRINT("cur_key_state = %d\r\n",cur_key_state);
	if(pre_key_state != cur_key_state)
	{
		if(cnt++ > USBD_DETECTION_DEBOUNCE)
		{
			pre_key_state = cur_key_state;
			key_changed = 1;
#if defined(C_CARCAM_UVC_DISPLAY) && C_CARCAM_UVC_DISPLAY == CUSTOM_OFF
			if (cur_key_state==0)
			{
				power_off_cnt = 1;		// start counting
			}
			else
			{
				power_off_cnt = 0;		// reset counting
			}
#endif
			cnt = 0;
		}
	}
	else
	{
		if(cnt > 0){
			cnt--;
		}
	}

    #if defined(C_CARCAM_UVC_DISPLAY) && C_CARCAM_UVC_DISPLAY == CUSTOM_OFF
	if (power_off_cnt) // It means power off cnt is starting
	{
        //DBG_PRINT("power_off_cnt = %d\r\n",power_off_cnt);
		if (power_off_cnt++>PERI_ADP_OUT_PWR_OFF_TIME)
		{
			ap_peripheral_pw_key_exe((INT16U*)(&power_off_cnt));
		}
	}
	#endif

	switch(usbd_connected_state)
	{
		case USBD_CONNECTED_EMPTY:
			if(key_changed && pre_key_state)
			{
				DBG_PRINT("Send MSG_USBD_INITIAL to USB device\r\n");

				state_usbd_task_msg_send(MSG_USBD_INITIAL, NULL, 0, MSG_PRI_NORMAL);
				usbd_connected_state = USBD_CONNECTED_TO_CHARGER;

				#if C_SCREEN_SAVER
				if(screen_saver_enable)
				{
					screen_saver_enable = 0;
					ap_state_handling_lcd_backlight_switch(1);
				}
				#endif // #if C_SCREEN_SAVER
			}
			break;

		case USBD_CONNECTED_TO_CHARGER:
			if(pre_key_state && usb_device_is_connected_to_pc() && usbd_connected_state == USBD_CONNECTED_TO_CHARGER)
			{
				usbd_connected_state = USBD_CONNECTED_TO_PC;
				DBG_PRINT("USBD_CONNECTED_TO_PC\r\n");
			}
			else if(key_changed && !pre_key_state)
			{
				usbd_connected_state = USBD_CONNECTED_EMPTY;
				DBG_PRINT("USBD_CONNECTED_TO_CHARGER but IO plug out\r\n");

			}
			break;

		case USBD_CONNECTED_TO_PC:
			if(key_changed)
			{
				if(!pre_key_state)
				{
					usbd_connected_state = USBD_CONNECTED_EMPTY;
					app_msg_send(MSG_APQ_DISCONNECT_TO_PC, NULL, 0, MSG_PRI_NORMAL);
					DBG_PRINT("USBD adaptor plug out\r\n");
				}
			}
			break;
	}
}

static void ap_adkey_action_init(void)
{
	ad_key_num = 0;
	ad_key_thsr_l = 0x7FFF;
	ad_key_thsr_h = 0x0;
}

static void ap_IOkey_action_init(void)
{
	#if defined(IOKEY_NUM) && IOKEY_NUM > 0
	io_key_num = 0;
	#endif
}


static void ap_adkey_action_register(KEYFUNC key_func, INT16S val_l, INT16S val_h)
{
	INT16U tmp = 1 + ad_key_num;

	#if ADKEY_NUM
	if(tmp <= ADKEY_NUM)
	{
		KEYSTATUS* key_map = &ad_key_map[ad_key_num];

		if(val_l < ad_key_thsr_l)
		{
			ad_key_thsr_l = val_l;
		}

		if(val_h > ad_key_thsr_h)
		{
			ad_key_thsr_h = val_h;
		}

		key_map->key_val_L = val_l;
		key_map->key_val_H = val_h;
		key_map->key_function = key_func;

		ad_key_num = tmp;
	}
	#endif // #if ADKEY_NUM
}

void ap_peripheral_key_register(INT8U type)
{

	ap_IOkey_action_init();

	ap_adkey_action_init();

	switch(type)
	{
		case KEY_MODE_DISABLE:
		{
			//disable all key
		}
		break;
#if C_WIFI_ON_OFF_KEY
		case WIFI_MODE_KEY:
		{
			#if defined(PW_KEY)
			io_key_map[io_key_num].key_val_L = PW_KEY;
			io_key_map[io_key_num++].key_function = ap_peripheral_pw_key_exe;
			#endif

			#if (defined(ADKEY_PREV_L) && defined(ADKEY_PREV_H))
			ap_adkey_action_register(
				ap_peripheral_prev_key_exe
				, ADKEY_PREV_L
				, ADKEY_PREV_H
			);
			#endif
		}
		break;
#endif

		case KEY_MODE_NORMAL:
		{
			#if defined(PW_KEY)
			io_key_map[io_key_num].key_val_L = PW_KEY;
			io_key_map[io_key_num++].key_function = ap_peripheral_pw_key_exe;
			#endif

			#if defined(CAM_KEY)
			io_key_map[io_key_num].key_val_L = CAM_KEY;
			io_key_map[io_key_num++].key_function = ap_peripheral_cam_key_exe;
			#endif

#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
			/* Keep GPIO controls in the same user-visible order as the
			 * resistor-key board: KEY1 previous, KEY2 play/pause, KEY3 next. */
			#if defined(OK_KEY)
			io_key_map[io_key_num].key_val_L = OK_KEY;
			io_key_map[io_key_num++].key_function = ap_peripheral_prev_key_exe;
            #endif

   			#if defined(PRE_KEY)
			io_key_map[io_key_num].key_val_L = PRE_KEY;
			io_key_map[io_key_num++].key_function = ap_peripheral_ok_key_exe;
            #endif

            #if defined(MODE_KEY)
			io_key_map[io_key_num].key_val_L = MODE_KEY;
			io_key_map[io_key_num++].key_function = ap_peripheral_mode_key_exe;
            #endif
#endif

			#if (defined(ADKEY_OK_L) && defined(ADKEY_OK_H))
			ap_adkey_action_register(
				/* ADKEY1: previous track */
				ap_peripheral_prev_key_exe
				, ADKEY_OK_L
				, ADKEY_OK_H
			);
			#endif
			#if (defined(ADKEY_PREV_L) && defined(ADKEY_PREV_H))
			ap_adkey_action_register(
				/* ADKEY2: play/pause */
				ap_peripheral_ok_key_exe
				, ADKEY_PREV_L
				, ADKEY_PREV_H
			);
			#endif
			#if (defined(ADKEY_NEXT_L) && defined(ADKEY_NEXT_H))
			ap_adkey_action_register(
				ap_peripheral_next_key_exe
				, ADKEY_NEXT_L
				, ADKEY_NEXT_H
			);
			#endif
			#if (defined(ADKEY_MODE_L) && defined(ADKEY_MODE_H))
			ap_adkey_action_register(
				ap_peripheral_mode_key_exe
				, ADKEY_MODE_L
				, ADKEY_MODE_H
			);
			#endif
			#if (defined(ADKEY_MENU_L) && defined(ADKEY_MENU_H))
			ap_adkey_action_register(
				ap_peripheral_menu_key_exe
				, ADKEY_MENU_L
				, ADKEY_MENU_H
			);
			#endif
			#if (defined(ADKEY_SOS_L) && defined(ADKEY_SOS_H))
			ap_adkey_action_register(
				ap_peripheral_sos_key_exe
				, ADKEY_SOS_L
				, ADKEY_SOS_H
			);
			#endif
		}
		break;

		#if C_BATTERY_LOW_POWER_OFF
		case KEY_MODE_BETTERY_LOW:
		{
			io_key_map[io_key_num].key_val_L = PW_KEY;
			io_key_map[io_key_num++].key_function = ap_peripheral_pw_key_exe;
		}
		break;
		#endif // #if C_BATTERY_LOW_POWER_OFF

		case KEY_MODE_BACK_A_CAR:
		{
			#if defined(IOKEY_NUM) && IOKEY_NUM > 0
			io_key_map[io_key_num].key_val_L = PW_KEY;
			io_key_map[io_key_num++].key_function = ap_peripheral_pw_key_exe;
			#endif

			#if (defined(ADKEY_PREV_L) && defined(ADKEY_PREV_H))
			ap_adkey_action_register(
				ap_peripheral_prev_key_exe
				, ADKEY_PREV_L
				, ADKEY_PREV_H
			);
			#endif
			#if (defined(ADKEY_NEXT_L) && defined(ADKEY_NEXT_H))
			ap_adkey_action_register(
				ap_peripheral_next_key_exe
				, ADKEY_NEXT_L
				, ADKEY_NEXT_H
			);
			#endif
		}
		break;

		default:
		{
			DBG_PRINT("[WARN]invalid key type=%d\r\n", type);
			ap_peripheral_key_register(KEY_MODE_NORMAL);
		}
		break;
	}
}

void ap_peripheral_init(void)
{
	power_off_timerid = usbd_detect_io_timerid = 0xFF;
	key_detect_timerid = 0xFF;

	ap_peripheral_key_register(KEY_MODE_NORMAL);

	ad_detect_timerid = 0xFF;
	ap_peripheral_ad_detect_init(ADKEY_DETECT_PIN, ap_peripheral_ad_check_isr);

	config_cnt = 0;
	gsensor_lock_flag = 0;
}

INT32U ap_peripheral_get_host_io_status(void)
{
	INT32U ret = 0;

	#if defined(BACK_SENSOR_DET)
	if(BACK_SENSOR_DET_ACTIVE)
	{
		ret = gpio_read_io(BACK_SENSOR_DET)? 1: 0;
	}
	else
	{
		ret = gpio_read_io(BACK_SENSOR_DET)? 0: 1;
	}
	#endif

	#if 0 && CRAZY_KEY_TEST // Function not yet implemented
	if(ret && crazy_key_enable)
	{
		ret = crazy_io_back_plug_in;
	}
	#endif

	return ret;
}

INT32U ap_peripheral_get_back_a_car_status(void)
{
	INT32U ret = 0;

	#if defined(BACK_A_CAR_DET)
	if(BACK_A_CAR_DET_ACTIVE)
	{
		ret = gpio_read_io(BACK_A_CAR_DET)? 1: 0;
	}
	else
	{
		ret = gpio_read_io(BACK_A_CAR_DET)? 0: 1;
	}
	#endif

	return ret;
}

INT32S ap_peripheral_back_detect(void)
{
    static INT8U back_sensor_plug_in	= 0;
    static INT8U back_debounce_cnt		= 0;

    INT32U temp;
    INT32U rc = STATUS_FAIL;

    #if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_OFF))
    if(s_usbd_pin)
    {
        // If device connected to PC, ignore host detection
        return  rc;
    }
	#endif

    temp = ap_peripheral_get_host_io_status();
    if(temp != back_sensor_plug_in)
    {
        if(++back_debounce_cnt > BACK_SENSOR_STABLE_THRESHOLD)
        {
            back_sensor_plug_in = temp;
            back_debounce_cnt = 0;
            if(back_sensor_plug_in)
            {
                while(osOK != app_msg_send(MSG_APQ_BACK_SENSOR_PLUG_IN, NULL, 0, MSG_PRI_NORMAL))
                {
                    osDelay(1);
                }
            }
            else
            {
                while(osOK != app_msg_send(MSG_APQ_BACK_SENSOR_PLUG_OUT, NULL, 0, MSG_PRI_NORMAL))
                {
                    osDelay(1);
                }
            }
        }
    }
    else
    {
        back_debounce_cnt = 0;
        rc = STATUS_OK;
    }

	if(back_sensor_plug_in)
	{
        static BOOLEAN back_a_car = 0;
        static INT16U back_a_car_debounce_cnt = 0;

        temp = ap_peripheral_get_back_a_car_status();
		if(temp != back_a_car)
		{
            if(++back_a_car_debounce_cnt > BACK_A_CAR_STABLE_THRESHOLD)
			{
                back_a_car_debounce_cnt = 0;
				back_a_car = temp;
				if(back_a_car)
				{
                    while(osOK != app_msg_send(MSG_APQ_BACK_A_CAR_ON, NULL, 0, MSG_PRI_NORMAL))
                    {
                        osDelay(1);
                    }
				}
				else
				{
                    #if C_SCREEN_SAVER == CUSTOM_ON
					ap_state_handling_lcd_backlight_switch(1);
                    #endif
                    while(osOK != app_msg_send(MSG_APQ_BACK_A_CAR_OFF, NULL, 0, MSG_PRI_NORMAL))
                    {
                        osDelay(1);
                    }
				}
			}
		}
		else
		{
			back_a_car_debounce_cnt = 0;
		}
    }

	return rc;
}

INT8U back_a_car_status_get(void)
{
	return back_a_car_on_flag;
}

void back_a_car_status_set(INT8U val)
{
	back_a_car_on_flag = val;
}

#if C_GSENSOR_FUNCTION == CUSTOM_ON
void ap_peripheral_read_gsensor(void)
{
	INT16U			temp;

	if(gsensor_lock_flag)
		return;
	gsensor_lock_flag = 1;

	temp = drv_l2_gsensor_int_active_get();

	if((temp != 0xff) && (temp & 0x04)) //active int flag
	{
		drv_l2_gsensor_int_flag_clear();

		if(ap_state_config_record_G_sensor_get())
		{
			app_msg_send(MSG_APQ_SOS_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
		}
		DBG_PRINT("gsensor int actived, flag:%d\r\n",temp);
	}

	gsensor_lock_flag = 0;

	//DBG_PRINT("gsensor chipid = 0x%x\r\n", temp);
}
#endif

void ap_peripheral_config_store(void)
{
	if(config_cnt++ == PERI_COFING_STORE_INTERVAL)
	{
		config_cnt = 0;
		app_msg_send(MSG_APQ_USER_CONFIG_STORE, NULL, 0, MSG_PRI_NORMAL);
	}
}

#if C_RTC_REBOOT

INT8U gWDog_Reset;
static INT8U gRTC_Set_Alarm=0;

void ext_rtc_alarm_isr_callback(void)
{
	DBG_PRINT("Ext RTC Alarm interrupt\r\n");
}

void ext_rtc_wakeup_isr_callback(void)
{
	DBG_PRINT("Ext RTC wakeup interrupt\r\n");
}

void ap_peripheral_rtc_alarm_disable(void)
{
	gRTC_Set_Alarm =0;
	ext_rtc_alarm_enable(NULL, 0, NULL);
	ext_rtc_wakeup_int_enable(0, NULL);
	ext_rtc_write(0x80, 0);   // clear rtc reset count
	ext_rtc_write(0x81, 0);   // clear rtc reset index
	ext_rtc_write(0x82, 0);   // clear rtc reset index
}

INT32S ap_peripheral_rtc_alarm_set(void)
{
	t_rtc	alarm_time;
	INT32U	alarm_sec;

	// not set alarm
	if(gRTC_Set_Alarm == 0)
	{
		return STATUS_FAIL;
	}

	if(drv_l1_sys_wdt_enable_get() == SYS_WDT_EN_OFF)
	{
		return STATUS_FAIL;
	}
	///DBG_PRINT("set alarm\r\n");

	ext_rtc_set_reliable(0x55);
	ext_rtc_time_get(&alarm_time);
	///DBG_PRINT("start - %d: %d: %d: \r\n", time.rtc_hour, time.rtc_min, time.rtc_sec);

	/* next alarm time = now + polling time + wdt period + power off time */
	// polling time
	alarm_sec = 1;

	// wdt period
	switch(drv_l1_sys_wdt_period_get())
	{
		case SYS_WDT_2T0_SEC:	alarm_sec += 2; break;
		case SYS_WDT_4T0_SEC:	alarm_sec += 4; break;
		case SYS_WDT_8T0_SEC:	alarm_sec += 8; break;
		case SYS_WDT_16T0_SEC:	alarm_sec += 16; break;
		default:				alarm_sec += 1; break;
	}

	// power off time
	alarm_sec += 2;

	alarm_time.rtc_sec	+= alarm_sec;
	alarm_time.rtc_min	+= alarm_time.rtc_sec / 60;
	alarm_time.rtc_sec	%= 60;
	alarm_time.rtc_hour	+= alarm_time.rtc_min / 60;
	alarm_time.rtc_min	%= 60;
	alarm_time.rtc_day	+= alarm_time.rtc_hour / 24;
	alarm_time.rtc_hour	%= 24;

	///DBG_PRINT("alarm - %d: %d: %d: \r\n", alarm_time.rtc_hour, alarm_time.rtc_min, alarm_time.rtc_sec);
	ext_rtc_alarm_enable(&alarm_time, 1, ext_rtc_alarm_isr_callback);
	ext_rtc_wakeup_int_enable(1, ext_rtc_wakeup_isr_callback);
	rtc_int_set(GPX_RTC_HR_IEN|GPX_RTC_MIN_IEN|GPX_RTC_SEC_IEN|GPX_RTC_HALF_SEC_IEN,
		(RTC_DIS&GPX_RTC_HR_IEN)|(RTC_DIS&GPX_RTC_MIN_IEN)|(RTC_DIS&GPX_RTC_SEC_IEN)|(RTC_DIS&GPX_RTC_HALF_SEC_IEN));
	rtc_schedule_disable();
	ext_rtc_write(0x81, 0x55);   // set rtc reset index
	ext_rtc_write(0x82, 0xAA);   // set rtc reset index

	return STATUS_OK;
}

void ap_peripheral_rtc_reboot_init(void)
{
	INT8U temp=0,tempa=0,tempb=0;
	INT64U alarm_time=0, now_time=0;
	t_rtc time;

	ext_rtc_read(0x81, &tempa);   // read rtc reset index
	ext_rtc_read(0x82, &tempb);   // read rtc reset index
	if((tempa == 0x55) && (tempb == 0xAA))
	{
		gWDog_Reset = 1;

		ext_rtc_set_reliable(0x55);
		ext_rtc_time_get(&time);
		now_time = (time.rtc_day * 60 * 60 * 24) +
				(time.rtc_hour * 60 * 60) +
				(time.rtc_min * 60) + time.rtc_sec;

		ext_rtc_read(0x21, &temp);   // read rtc reset index
		alarm_time |= temp;
		ext_rtc_read(0x22, &temp);   // read rtc reset index
		alarm_time |= (temp<<8);
		ext_rtc_read(0x23, &temp);   // read rtc reset index
		alarm_time |= (temp<<16);
		ext_rtc_read(0x24, &temp);   // read rtc reset index
		alarm_time |= (temp<<24);
		ext_rtc_read(0x25, &temp);   // read rtc reset index
		alarm_time |= ((INT64U)temp<<32);
		alarm_time >>= 7;
		DBG_PRINT("watchdog alarm time:%lx\r\n",alarm_time);
		DBG_PRINT("watchdog now time:%lx\r\n",now_time);

		ext_rtc_read(0x80, &temp);   // read rtc reset count
		DBG_PRINT("rtc reset cnt:%x\r\n",(temp & 0xF));
		if((temp & 0xF) > 3)	// reset over 3 times, stop reset
		{
			gRTC_Set_Alarm = 0;
			ext_rtc_write(0x80, 0);   // clear rtc reset count
			ext_rtc_write(0x81, 0);   // clear rtc reset index
			ext_rtc_write(0x82, 0);   // clear rtc reset index
		}
		else
		{
			if(now_time > (alarm_time + 20))   // normal power up, show logo
			{
				gWDog_Reset = 0;
				gRTC_Set_Alarm = 1;
				ext_rtc_write(0x80, 0);   // clear rtc reset count
				ext_rtc_write(0x81, 0);   // clear rtc reset index
				ext_rtc_write(0x82, 0);   // clear rtc reset index
			}
			else
			{
				gRTC_Set_Alarm = 1;
				temp += 1;
				ext_rtc_write(0x80, temp);   // write rtc reset count
			}
		}
	}
	// normal power off and power on
	else
	{
		gRTC_Set_Alarm = 1;
		ext_rtc_write(0x80, 0);   // clear rtc reset count
		ext_rtc_write(0x81, 0);   // clear rtc reset index
		ext_rtc_write(0x82, 0);   // clear rtc reset index
	}

	drv_l1_sys_wdt_clear();  /* clear watchdog flag */
	R_SYSTEM_RESET_FLAG = 0x0010;  /* reset watchdog timeout flag */
}
#endif



#if CRAZY_KEY_TEST
void ap_peripheral_crazy_key_test(void)
{
	static INT32U	crazy_key_cnt = 0;
	static INT32U	next_interval = (32>>1);

	// 1s = 128 / 4 = 32 (  #define PERI_TIME_INTERVAL_AD_DETECT		4)
	if(crazy_key_cnt < next_interval)
	{
		crazy_key_cnt++;
	}
	else
	{
		INT32U crazy_key;

		#if defined(IOKEY_NUM) && IOKEY_NUM > 0
		INT32U total_test_case = 1 + ad_key_num + io_key_num;
		#else
		INT32U total_test_case = 1 + ad_key_num;
		#endif

		crazy_key_cnt = 0;
		key_active_cnt = 0;

		// 1x0.5 ~ 10x0.5 random time
		next_interval = 1 + (R_RANDOM1 % 10);
		next_interval *= (32>>1);

		switch(crazy_key = (R_RANDOM0 % total_test_case))
		{
		// other event
		case 0:
			// back camera plug in/out
			//crazy_io_back_plug_in = !crazy_io_back_plug_in;
			//DBG_PRINT("CRAZY_KEY=BACK PLUG %s\r\n", crazy_io_back_plug_in ? "IN" : "OUT");
			break;

		// adkey event
		default:
			{
				crazy_key -= 1;

				if(crazy_key< ad_key_num)		/* ad key */
				{
					// break some ad key
					if(ad_key_map[crazy_key].key_function == ap_peripheral_sos_key_exe)			break;
					//if(ad_key_map[crazy_key].key_function == ap_peripheral_mode_key_exe)		break;
					//if(ad_key_map[crazy_key].key_function == ap_peripheral_next_key_exe)		break;
					//if(ad_key_map[crazy_key].key_function == ap_peripheral_prev_key_exe)		break;
					//if(ad_key_map[crazy_key].key_function == ap_peripheral_ok_key_exe)		break;
					if(ad_key_map[crazy_key].key_function == ap_peripheral_menu_key_exe)		break;
				}

				#if defined(IOKEY_NUM) && IOKEY_NUM > 0
				else if(crazy_key>=ad_key_num)	/* io key */
				{
					// break some io key
					//if(io_key_map[crazy_key-ad_key_num].key_function == ap_peripheral_pw_key_exe)		break;
				}
				#endif

				if(crazy_key< ad_key_num)	/* ad key */
				{
					if(ad_key_map[crazy_key].key_function)
					{
#if 0
					// 1/26 probability trigger long pressed event
					INT16U	key_tick_cnt = R_RANDOM1 % (1 + 25);
#else
						INT16U	key_tick_cnt = 0;
#endif
						DBG_PRINT("\r\nAD_CRAZY_KEY=%d key_tick_cnt = %d \r\n", crazy_key,key_tick_cnt);
						ad_key_map[crazy_key].key_function(&key_tick_cnt);
					}
				}
				#if defined(IOKEY_NUM) && IOKEY_NUM > 0
				else if(crazy_key>=ad_key_num)	/* io key */
				{

					if(io_key_map[crazy_key-ad_key_num].key_function)
					{
#if 0
						// 1/26 probability trigger long pressed event
						INT16U	key_tick_cnt = R_RANDOM1 % (1 + 25);
#else
						INT16U	key_tick_cnt = 0;
#endif
						DBG_PRINT("\r\nIO_CRAZY_KEY=%d key_tick_cnt = %d \r\n", crazy_key-ad_key_num,key_tick_cnt);
						io_key_map[crazy_key-ad_key_num].key_function(&key_tick_cnt);
					}
				}
				#endif

			}

			break;
		}
	}
}
#endif // CRAZY_KEY_TEST == 1

#define LED_STATUS_OFF   0
#define LED_STATUS_ON   1
#define LED_STATUS_BLINK	2
static INT8U led_status = 0;
static INT32U led_cnt = 0;
void ap_peripheral_led_blink_set(INT8U state)
{
    switch(state)
    {
        case 0:     // led off
            gpio_write_io(LED, DATA_HIGH);
	     led_status = LED_STATUS_OFF;
            break;

	 case 1:    // led on
	     gpio_write_io(LED, DATA_LOW);
	     led_status = LED_STATUS_ON;
	     break;

        case 2:    // led blink
	     gpio_write_io(LED, DATA_HIGH);
	     led_status = LED_STATUS_BLINK;
	     break;
    }
}

void LED_service(void)
{
    INT8U io_status;

    led_cnt += 1;
    if((led_cnt & 0x1) == 0)    // 0.5s
        return;

    if(led_status == LED_STATUS_BLINK)
    {
        io_status = gpio_read_io(LED);
	 if(io_status == 0)
	 {
	     gpio_write_io(LED, DATA_HIGH);
	 }
	 else
	 {
	     gpio_write_io(LED, DATA_LOW);
	 }
    }
}


static TIME_T sys_osd_time;

void ap_peripheral_sys_osd_time_set(void)
{
    TIME_T  time;

	cal_time_get(&time);

	osSuspend();
	sys_osd_time = time;
	osResume();
}

void ap_peripheral_osd_time_get(TIME_T *osd_time)
{
    if(osd_time)
    {
        osSuspend();
        *osd_time = sys_osd_time;
        osResume();
    }
}

#ifdef SPEAKER_EN_PIN
void ap_peripheral_ext_amp_enable(INT8U en)
{
	if(en)
	{
		gpio_write_io(SPEAKER_EN_PIN, SPEAKER_EN_PIN_ACTIVE);
		osDelay(50);
	}
	else
	{
		gpio_write_io(SPEAKER_EN_PIN, !SPEAKER_EN_PIN_ACTIVE);
	}
}
#endif // #ifdef SPEAKER_EN_PIN

/*
	0: Turn off speaker
	1: Turn on speaker
*/
void
ap_peripheral_speaker_ctrl(
	INT8U on_or_off
)
{
	if(on_or_off)
	{ // Turn on
		drv_l1_dac_audop_speaker_en(1);
	#ifdef SPEAKER_EN_PIN
		ap_peripheral_ext_amp_enable(1);
	#endif
		//DBG_PRINT("==>Speaker On<==\r\n");
	}
	else
	{// Turn off
	#ifdef SPEAKER_EN_PIN
		if(present_state == STATE_BROWSE)
		{
			return;
		}

		ap_peripheral_ext_amp_enable(0);
	#endif
		drv_l1_dac_audop_speaker_en(0);
		//DBG_PRINT("==>Speaker Off<==\r\n");
	}

}

#if defined(HDMI_HPD_PIN) && (LCD_DRV_EN_HDMI==1)
#define HDMI_PLUGIN_DEBOUNCE			(4)		/* this function is placed in MSG_PERIPHERAL_TASK_POLL_32HZ , thus  (4)*(1/32) = 1/8 sec */
INT32U hdmi_plugin_debounce_cnt=0;			/* IO debounce count */
INT32U hdmi_plugin_status=0;					/* 0:unpluged ; 1:pluged */
void ap_peripheral_hdmi_hpd_detect(void)
{
	INT32U read_status=0;

	read_status=(gpio_read_io(HDMI_HPD_PIN)?1:0);
	if(read_status!=hdmi_plugin_status)	/* device changed */
	{
		hdmi_plugin_debounce_cnt++;
		//DBG_PRINT("hdmi_cnt%d\r\n",hdmi_plugin_debounce_cnt);

		if(hdmi_plugin_debounce_cnt>=HDMI_PLUGIN_DEBOUNCE)
		{
			hdmi_plugin_debounce_cnt=0;		/* clear count */
			hdmi_plugin_status=read_status;
			if(hdmi_plugin_status)	/* HDMI plug in */
			{
				DBG_PRINT("HDMI plugin\r\n");
				#if C_ASYN_DBLE_TFT
				drv_l2_asyn_multi_tft_hdmi_plug_status_set(1);	/* HDMI plug in */
				#endif
			}
			else	/* HDMI plug out */
			{
				DBG_PRINT("HDMI plugout\r\n");
				#if C_ASYN_DBLE_TFT
				drv_l2_asyn_multi_tft_hdmi_plug_status_set(0);	/* HDMI plug out */
				#endif
			}

			#if C_ASYN_DBLE_TFT
			/* sent switch key */
			display_task_msg_send(MSG_DISPLAY_DUAL_SCREEN_SWITCH, NULL, 0, MSG_PRI_NORMAL);
			#endif
		}
	}
}

#endif

