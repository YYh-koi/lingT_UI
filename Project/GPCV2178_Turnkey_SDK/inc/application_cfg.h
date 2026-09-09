/*****************************************************************************
 *               Copyright Generalplus Corp. All Rights Reserved.
 *
 * FileName:       application_cfg.h
 * Author:         Lichuanyue
 * Description:    Created
 * Version:        1.0
 * Function List:
 *                 Null
 * History:
 *                 1>. 2008/7/15 Created
 *****************************************************************************/
#ifndef __APPLICATION_CFG_H__
#define __APPLICATION_CFG_H__

#define _DPF_SUPPORT_ENGLISH    1
#define _DPF_SUPPORT_TCHINESE   1
#define _DPF_SUPPORT_SCHINESE   1
#define _DPF_SUPPORT_JAPANESE   0
#define _DPF_SUPPORT_FRENCH     0
#define _DPF_SUPPORT_GERMAN     0
#define _DPF_SUPPORT_KOREA      0
#define _DPF_SUPPORT_ITALIAN    0
#define _DPF_SUPPORT_PORTUGUESE 0
#define _DPF_SUPPORT_RUSSIAN    0
#define _DPF_SUPPORT_SPANISH    0
#define _DPF_SUPPORT_ARABIC     0

/*
*  Language code
*/
typedef enum
{
#if _DPF_SUPPORT_ENGLISH == 1
    LCD_EN = 0, //English
#endif
#if _DPF_SUPPORT_TCHINESE == 1
    LCD_TCH,    //Traditional Chinese
#endif
#if _DPF_SUPPORT_SCHINESE == 1
    LCD_SCH,    //Simple Chinese
#endif
#if _DPF_SUPPORT_JAPANESE == 1
    LCD_JAP,    //Japanese
#endif
#if _DPF_SUPPORT_FRENCH == 1
    LCD_FR,     //French
#endif
#if _DPF_SUPPORT_GERMAN == 1
    LCD_GER,    //German
#endif
#if _DPF_SUPPORT_KOREA == 1
    LCD_KOR,    //Korea
#endif
#if _DPF_SUPPORT_ITALIAN == 1
    LCD_IT,     //Italian
#endif
#if _DPF_SUPPORT_PORTUGUESE == 1
    LCD_PT,     //Portuguese
#endif
#if _DPF_SUPPORT_RUSSIAN == 1
    LCD_RU,     //Russian
#endif
#if _DPF_SUPPORT_SPANISH == 1
    LCD_SP,     //Spanish
#endif
#if _DPF_SUPPORT_ARABIC == 1
    LCD_ARA,    //Arabic
#endif
    LCD_MAX
} _DPF_LCD;

//define audio algorithm
#define APP_UP_SAMPLE_EN	0
#endif // __APPLICATION_CFG_H__
