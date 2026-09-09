#ifndef __DRV_L2_USER_CALIBRATION_H__
#define __DRV_L2_USER_CALIBRATION_H__

#include "drv_l2_sensor.h"

#if (defined _SENSOR_GC2053_CDSP_MIPI) && (_SENSOR_GC2053_CDSP_MIPI == 1)
    #include "lens/gp_gc2053_calibration.h"
#elif (defined _SENSOR_GC2083_CDSP_MIPI) && (_SENSOR_GC2083_CDSP_MIPI == 1)
    #include "lens/gp_gc2083_calibration.h"
#elif (defined _SENSOR_H42_CDSP_MIPI) && (_SENSOR_H42_CDSP_MIPI == 1)
    #include "lens/gp_jxh42_calibration.h"
#elif (defined _SENSOR_H62_CDSP_MIPI) && (_SENSOR_H62_CDSP_MIPI == 1)
    #include "lens/gp_jxh62_calibration.h"
#elif (defined _SENSOR_GC1054_CDSP_MIPI) && (_SENSOR_GC1054_CDSP_MIPI == 1)
    #include "lens/gp_gc1054_calibration.h"
#elif (defined _SENSOR_GC1064_CDSP_MIPI) && (_SENSOR_GC1064_CDSP_MIPI == 1)
    #include "lens/gp_gc1064_calibration.h"
#elif (defined _SENSOR_GC4653_CDSP_MIPI) && (_SENSOR_GC4653_CDSP_MIPI == 1)
    #include "lens/gp_gc4653_calibration.h"
#elif (defined _SENSOR_GC5025_CDSP_MIPI) && (_SENSOR_GC5025_CDSP_MIPI == 1)
    #include "lens/gp_gc5025_calibration.h"
#elif (defined _SENSOR_IMX307_CDSP_MIPI) && (_SENSOR_IMX307_CDSP_MIPI == 1)
    #include "lens/gp_imx307_calibration.h"
#elif (defined _SENSOR_IMX376_CDSP_MIPI) && (_SENSOR_IMX376_CDSP_MIPI == 1)
    #include "lens/gp_imx376_calibration.h"    
#else
    #include "lens/gp_calibration.h"
#endif

#endif
