// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "kd_imgsensor.h"


#include "imgsensor_hw.h"
#include "imgsensor_cfg_table.h"

/* Legacy design */
struct IMGSENSOR_HW_POWER_SEQ sensor_power_sequence[] = {
/* Tab A11 code for bring up by liugang at 20250331 start */
#if defined(CONFIG_CUSTOM_PROJECT_HST11)
#if defined(A11YYSTBACKSC820CS_MIPI_RAW)
	{
		SENSOR_DRVNAME_A11YYSTBACKSC820CS_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{RST, Vol_High, 7},
			{SensorMCLK, Vol_High,5}
		},
	},
#endif
#if defined(A11YYLYFRONTGC05A2_MIPI_RAW)
	{
		SENSOR_DRVNAME_A11YYLYFRONTGC05A2_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{AVDD1, Vol_2800, 1},
			{RST, Vol_High, 7},
			{SensorMCLK, Vol_High,5}
		},
	},
#endif
#if defined(A1101CXTBACKGC08A8_MIPI_RAW)
	{
		SENSOR_DRVNAME_A1101CXTBACKGC08A8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High,5},
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{AFVDD, Vol_2800, 5},
			{RST, Vol_High, 7},
		},
	},
#endif
#if defined(A1102HDYBACKGC08A8_MIPI_RAW)
	{
		SENSOR_DRVNAME_A1102HDYBACKGC08A8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High,5},
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
        /* HST11  code  for AX7800A-160 by lidong at 20250512 start */
			{AFVDD, Vol_2800, 4},
        /* HST11  code  for AX7800A-160 by lidong at 20250512 end */
			{RST, Vol_High, 7},
		},
	},
#endif
/* Tab A11 code for bring up by jiangwenhan at 20250425 start */
#if defined(A1103CXTBACKSC820CS_MIPI_RAW)
	{
		SENSOR_DRVNAME_A1103CXTBACKSC820CS_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
/* HST11  code  for AX7800A-562 by lidong at 20250617 start */
			{AFVDD, Vol_2800, 4},
/* HST11  code  for AX7800A-562 by lidong at 20250617 end */
			{RST, Vol_High, 7},
			{SensorMCLK, Vol_High,5}
		},
	},
#endif
/* Tab A11 code for SR-AL7761A-01-376 by xuyunhui at 2025/04/25 start */
#if defined(A1101CXTFRONTGC05A2_MIPI_RAW)
	{
		SENSOR_DRVNAME_A1101CXTFRONTGC05A2_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{AVDD1, Vol_2800, 1},
			{RST, Vol_High, 7},
			{SensorMCLK, Vol_High,5}
		},
	},
#endif
#if defined(A1102WXFRONTSC521CS_MIPI_RAW)
	{
		SENSOR_DRVNAME_A1102WXFRONTSC521CS_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{AVDD1, Vol_2800, 1},
			{RST, Vol_High, 7},
			{SensorMCLK, Vol_High,5}
		},
	},
#endif
#if defined(A1103HDYFRONTGC05A2_MIPI_RAW)
	{
		SENSOR_DRVNAME_A1103HDYFRONTGC05A2_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{AVDD1, Vol_2800, 1},
			{RST, Vol_High, 7},
			{SensorMCLK, Vol_High,5}
		},
	},
#endif
/* Tab A11 code for SR-AL7761A-01-376 by xuyunhui at 2025/04/25 end */
#endif
/* Tab A11 code for bring up by liugang at 20250331 end */
#if defined(CONFIG_CUSTOM_PROJECT_OT11)
/* Tab A9 code for SR-AX6739A-01-762 by rongyi at 20230628 start */
/* Tab A9 code for SR-AX6739A-01-762 by xuyunhui at 20250618 start */
#if defined(A0901BACKSC800CSLY_MIPI_RAW)
	{
		SENSOR_DRVNAME_A0901BACKSC800CSLY_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{AFVDD, Vol_2800, 5},
			{RST, Vol_High, 7},
			{SensorMCLK, Vol_High,5}
		},

	},
#endif
#if defined(A0902BACKHI846ST_MIPI_RAW)
{
	SENSOR_DRVNAME_A0902BACKHI846ST_MIPI_RAW,
	{
		{RST, Vol_Low, 1},
		{DOVDD, Vol_1800, 1},
		{AVDD, Vol_2800, 1},
		{DVDD, Vol_1200, 1},
		{AFVDD, Vol_2800, 5},
		{SensorMCLK, Vol_High,1},
		{RST, Vol_High, 4}
	},
},
#endif
#if defined(A0903BACKC8490XSJ_MIPI_RAW)
	{
		SENSOR_DRVNAME_A0903BACKC8490XSJ_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{RST, Vol_High, 4},
			{SensorMCLK, Vol_High,8}
		},
	},
#endif
#if defined(A0904BACKSC800CSST_MIPI_RAW)
	{
		SENSOR_DRVNAME_A0904BACKSC800CSST_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{AFVDD, Vol_2800, 5},
			{RST, Vol_High, 7},
			{SensorMCLK, Vol_High,5}
		},

	},
#endif
#if defined(A0904BACKSC820CSST_MIPI_RAW)
	{
		SENSOR_DRVNAME_A0904BACKSC820CSST_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{AFVDD, Vol_2800, 5},
			{RST, Vol_High, 7},
			{SensorMCLK, Vol_High,5}
		},
	},
#endif
/* Tab A9 code for SR-AX6739A-01-762 by xuyunhui at 20250618 end */
#if defined(A0901FRONTSC201CSLY_MIPI_RAW)
	{
		SENSOR_DRVNAME_A0901FRONTSC201CSLY_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{RST, Vol_High, 5},
			{SensorMCLK, Vol_High, 1}
		},
	},
#endif
#if defined(A0902FRONTC2599CXT_MIPI_RAW)
	{
		SENSOR_DRVNAME_A0902FRONTC2599CXT_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 4},
			{RST, Vol_High, 3},
			{SensorMCLK, Vol_High,8}
		},
	},
#endif
#if defined(A0903FRONTBF2257XSJ_MIPI_RAW)
	{
		SENSOR_DRVNAME_A0903FRONTBF2257XSJ_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{RST, Vol_High, 2},
			{SensorMCLK, Vol_High, 3}
		},
	},
#endif
/* Tab A9 code for SR-AX6739A-01-762 by rongyi at 20230628 end */
#endif

#if defined(CONFIG_CUSTOM_PROJECT_HS07)
#if defined(A07YYXLBACKS5KJN1_MIPI_RAW)
	{
		SENSOR_DRVNAME_A07YYXLBACKS5KJN1_MIPI_RAW,
		{
			{SensorMCLK, Vol_Low, 3},
			{PDN, Vol_Low, 3},
			{RST, Vol_Low, 3},
			{DOVDD, Vol_High, 5},
			{DVDD, Vol_High, 5},
			{AVDD, Vol_High, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317start */
			{AFVDD, Vol_2800, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317end */
			{SensorMCLK, Vol_High, 10},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 10},
		},
	},
#endif
#if defined(A07YYCXTFRONTGC08A8_MIPI_RAW)
	{
		SENSOR_DRVNAME_A07YYCXTFRONTGC08A8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High,5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_High, 1},
			{DVDD, Vol_High, 1},
			{AVDD, Vol_High, 1},
			/* Hs07 code  for HS07-260 by zhangpengfei at 20250409start */
			{AVDD1, Vol_High, 1},
			/* Hs07 code  for HS07-260 by zhangpengfei at 20250409end */
			{RST, Vol_High, 7},
		},
	},
#endif
#if defined(A07YYWXDEPTHSC201CS_MIPI_RAW)
    {
        SENSOR_DRVNAME_A07YYWXDEPTHSC201CS_MIPI_RAW,
        {
            {RST, Vol_Low, 1},
            {DOVDD, Vol_High, 1},
            {AVDD, Vol_High, 1},
            /* Hs07 code  for HS07-260 by zhangpengfei at 20250409start */
            {AVDD1, Vol_High, 1},
            /* Hs07 code  for HS07-260 by zhangpengfei at 20250409end */
            //{DVDD, Vol_High, 1},
            {RST, Vol_High, 1},
            {RST, Vol_Low, 1},
            {RST, Vol_High, 0},
            {SensorMCLK, Vol_High, 5},
        },
     },
#endif
/* HS07 V code for SR-AL7761A-01-373  by zhongbin at 2025/03/24 start */
#if defined(A0701TXDBACKS5KJN1_MIPI_RAW)
	{
        SENSOR_DRVNAME_A0701TXDBACKS5KJN1_MIPI_RAW,
        {
            {SensorMCLK, Vol_Low, 3},
            {RST, Vol_Low, 3},
            {DOVDD, Vol_High, 5},
            {PDN, Vol_High, 2, Vol_Low, 2},
            {DVDD, Vol_High, 5},
            {AVDD, Vol_High, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317start */
            {AFVDD, Vol_2800, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317end*/
/* HS07 V code for P250626-01124 by xuyunhui at 2025/07/01 start */
            {RST, Vol_High, 1},
            {SensorMCLK, Vol_High, 10},
/* HS07 V code for P250626-01124 by xuyunhui at 2025/07/01 end */
		},
	},
#endif
#if defined(A0702TXDBACKGC50E1_MIPI_RAW)
	{
        SENSOR_DRVNAME_A0702TXDBACKGC50E1_MIPI_RAW,
          {
            {SensorMCLK, Vol_Low, 3},
            {RST, Vol_Low, 3},
            {DOVDD, Vol_High, 5},
            {PDN, Vol_High, 2, Vol_Low, 2},	/* used for front camera dvdd select 1.1V */
            {DVDD, Vol_High, 5},
            {AVDD, Vol_High, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317start */
            {AFVDD, Vol_2800, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317end*/
            {SensorMCLK, Vol_High, 10},
            {RST, Vol_High, 10},
	  },
	},
#endif
#if defined(A0703XLBACKGC50E1_MIPI_RAW)
	{
        SENSOR_DRVNAME_A0703XLBACKGC50E1_MIPI_RAW,
        {
            {SensorMCLK, Vol_Low, 3},
            {RST, Vol_Low, 3},
            {DOVDD, Vol_High, 5},
            {PDN, Vol_High, 2, Vol_Low, 2},	/* used for front camera dvdd select 1.1V */
            {DVDD, Vol_High, 5},
            {AVDD, Vol_High, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317start */
            {AFVDD, Vol_2800, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317end*/
            {SensorMCLK, Vol_High, 10},
            {RST, Vol_High, 10},
		},
	},
#endif
#if defined(A0704XLBACKS5KJN1_MIPI_RAW)
	{
        SENSOR_DRVNAME_A0704XLBACKS5KJN1_MIPI_RAW,
        {
            {SensorMCLK, Vol_Low, 3},
            {RST, Vol_Low, 3},
            {DOVDD, Vol_High, 5},
            {PDN, Vol_High, 2, Vol_Low, 2},	/* used for front camera dvdd select 1.1V */
            {DVDD, Vol_High, 5},
            {AVDD, Vol_High, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317start */
            {AFVDD, Vol_2800, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317end*/
/* HS07 V code for P250626-01124 by xuyunhui at 2025/07/01 start */
            {RST, Vol_High, 1},
            {SensorMCLK, Vol_High, 10},
/* HS07 V code for P250626-01124 by xuyunhui at 2025/07/01 end */
		},
	},
#endif
#if defined(A0705LYBACKGC50E1_MIPI_RAW)
	{
        SENSOR_DRVNAME_A0705LYBACKGC50E1_MIPI_RAW,
        {
            {SensorMCLK, Vol_Low, 3},
            {RST, Vol_Low, 3},
            {DOVDD, Vol_High, 5},
            {PDN, Vol_High, 2, Vol_Low, 2},	/* used for front camera dvdd select 1.1V */
            {DVDD, Vol_High, 5},
            {AVDD, Vol_High, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317start */
            {AFVDD, Vol_2800, 5},
/* Hs07 code  for HS07-19  AF by yujiahui at 20250317end*/
            {SensorMCLK, Vol_High, 10},
            {RST, Vol_High, 10},
		},
	},
#endif
/* HS07 V code for SR-AL7761A-01-373  by zhongbin at 2025/03/24 end */
/* HS07 V code for SR-AL7761A-01-438  by jiangwenhan at 2025/03/17 start */
/* HS07 V code for HS07-198  by huabinchen at 2025/03/24 start */
#if defined(A0701CXTFRONTGC08A8_MIPI_RAW)
	{
		SENSOR_DRVNAME_A0701CXTFRONTGC08A8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High,5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_High, 1},
			{DVDD, Vol_High, 1},
			{AVDD, Vol_High, 1},
			/* Hs07 code  for HS07-260 by zhangpengfei at 20250409start */
			{AVDD1, Vol_High, 1},
			/* Hs07 code  for HS07-260 by zhangpengfei at 20250409end */
			{RST, Vol_High, 7},
		},
	},
#endif
#if defined(A0702LYFRONTMT815_MIPI_RAW)
    {
        SENSOR_DRVNAME_A0702LYFRONTMT815_MIPI_RAW,
        {
/* HS07 V code for SR-AL7761A-01-496 by jiangwenhan at 2025/04/08 start */
            {DVDD, Vol_High, 1},
            {DOVDD, Vol_High, 1},
            {AVDD, Vol_High, 1},
            /* Hs07 code  for HS07-260 by zhangpengfei at 20250409start */
            {AVDD1, Vol_High, 1},
            /* Hs07 code  for HS07-260 by zhangpengfei at 20250409end */
/* HS07 V code for SR-AL7761A-01-496 by jiangwenhan at 2025/04/08 end */
            {SensorMCLK, Vol_High, 5},
            {RST, Vol_Low, 1},
/* HS07 V code for SR-AL7761A-01-496 by jiangwenhan at 2025/04/15 start */
            {RST, Vol_High, 3},
/* HS07 V code for SR-AL7761A-01-496 by jiangwenhan at 2025/04/15 end */
        },
     },
#endif
#if defined(A0703DDFRONTSC820CS_MIPI_RAW)
	{
		SENSOR_DRVNAME_A0703DDFRONTSC820CS_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_High, 1},
			{DVDD, Vol_High, 1},
			{AVDD, Vol_High, 1},
			/* Hs07 code  for HS07-260 by zhangpengfei at 20250409start */
			{AVDD1, Vol_High, 1},
			/* Hs07 code  for HS07-260 by zhangpengfei at 20250409end */
			{RST, Vol_High, 7},
			{SensorMCLK, Vol_High,5}
		},
	},
#endif
/* HS07 V code for HS07-198  by huabinchen at 2025/03/24 end */
/* HS07 V code for SR-AL7761A-01-438  by jiangwenhan at 2025/03/17 end */
/* HS07 V code for SR-AL7761A-01-376 by xuyunhui at 2025/03/13 start */
#if defined(A0701CXTDEPTHSC201CS_MIPI_RAW)
    {
        SENSOR_DRVNAME_A0701CXTDEPTHSC201CS_MIPI_RAW,
        {
            {RST, Vol_Low, 1},
            {DOVDD, Vol_High, 1},
            {AVDD, Vol_High, 1},
            /* Hs07 code  for HS07-260 by zhangpengfei at 20250409start */
            {AVDD1, Vol_High, 1},
            /* Hs07 code  for HS07-260 by zhangpengfei at 20250409end */
            //{DVDD, Vol_High, 1},
            {RST, Vol_High, 1},
            {RST, Vol_Low, 1},
            {RST, Vol_High, 0},
            {SensorMCLK, Vol_High, 5},
        },
     },
#endif
#if defined(A0702WXDEPTHOV02F_MIPI_RAW)
    {
        SENSOR_DRVNAME_A0702WXDEPTHOV02F_MIPI_RAW,
        {
           {RST, Vol_Low, 1},
           {DOVDD, Vol_High, 2},
           {AVDD, Vol_High, 6},
            /* Hs07 code  for HS07-260 by zhangpengfei at 20250409start */
            {AVDD1, Vol_High, 1},
            /* Hs07 code  for HS07-260 by zhangpengfei at 20250409end */
           {PDN, Vol_High, 1},
           {SensorMCLK, Vol_High, 10},
           {RST, Vol_High, 11},
        },
     },
#endif
/* HS07 V code for SR-AL7761A-01-376 by xuyunhui at 2025/03/14 end */
#endif

#if defined(IMX586_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX586_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 0},
			{AVDD1, Vol_1800, 0},
			{AFVDD, Vol_2800, 0},
			{DVDD, Vol_1100, 0},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 3}
		},
	},
#endif
#if defined(IMX319_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX319_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 1},
			{AVDD, Vol_2800, 0},
			{AFVDD, Vol_2800, 0},
			{DVDD, Vol_1100, 0},
			{DOVDD, Vol_1800, 1},
			{SensorMCLK, Vol_High, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(S5K3M5SX_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3M5SX_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DVDD, Vol_1100, 1},
			{AVDD, Vol_2800, 1},
			{AFVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 1},
			{RST, Vol_High, 2},
			{SensorMCLK, Vol_High, 1}
		},
	},
#endif
#if defined(GC02M0_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC02M0_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{DOVDD, Vol_1800, 1},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(IMX519_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX519_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_2800, 0},
			{AFVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{DOVDD, Vol_1800, 1},
			{SensorMCLK, Vol_High, 5},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(IMX519DUAL_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX519DUAL_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_2800, 0},
			{AFVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{DOVDD, Vol_1800, 1},
			{SensorMCLK, Vol_High, 5},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(IMX499_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX499_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 1},
			{SensorMCLK, Vol_High, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 10}
		},
	},
#endif
#if defined(IMX481_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX481_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_1800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1800, 2},
			{AFVDD, Vol_2800, 1},
			{SensorMCLK, Vol_High, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 10}
		},
	},
#endif
#if defined(IMX576_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX576_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 1}, /*data sheet 1050*/
			{SensorMCLK, Vol_High, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 8}
		},
	},
#endif
#if defined(IMX350_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX350_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1200, 5},
			{SensorMCLK, Vol_High, 5},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(IMX398_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX398_MIPI_RAW,
		{
			{SensorMCLK, Vol_Low, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 1},
		},
	},
#endif
#if defined(OV23850_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV23850_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 2},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(OV16885_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV16885_MIPI_RAW,
		{
			{PDN, Vol_Low, 1},
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 5},
			{PDN, Vol_High, 2},
			{RST, Vol_High, 2},
		},
	},
#endif
#if defined(OV05A20_MIPI_RAW)
		{
			SENSOR_DRVNAME_OV05A20_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 1},
				{RST, Vol_Low, 1},
				{AVDD, Vol_2800, 10},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{RST, Vol_High, 15}
			},
		},
#endif

#if defined(IMX386_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX386_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1100, 0},
			{DOVDD, Vol_1800, 1},
			{AFVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 2},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 10},
		},
	},
#endif
#if defined(IMX386_MIPI_MONO)
	{
		SENSOR_DRVNAME_IMX386_MIPI_MONO,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1100, 0},
			{DOVDD, Vol_1800, 1},
			{AFVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 2},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 10},
		},
	},
#endif
#if defined(IMX376_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX376_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(IMX338_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX338_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 1},
			{AVDD, Vol_2500, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(S5K2LQSX_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2LQSX_MIPI_RAW,
		{
			{PDN, Vol_Low, 1},
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 4},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 1},
			{DOVDD, Vol_1800, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0},
		},
	},
#endif
#if defined(S5K4H7_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K4H7_MIPI_RAW,
		{
			{PDN, Vol_Low, 1},
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 4},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 1},
			{DOVDD, Vol_1800, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0},
		},
	},
#endif
#if defined(S5K4E6_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K4E6_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2900, 0},
			{DVDD, Vol_1200, 2},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K2T7SP_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2T7SP_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{SensorMCLK, Vol_High, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 2},
		},
	},
#endif
#if defined(S5K3P8SP_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3P8SP_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
		},
	},
#endif
#if defined(S5K3P8SX_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3P8SX_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{SensorMCLK, Vol_High, 1},
			{DVDD, Vol_1000, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 2},
		},
	},
#endif
#if defined(S5K3M2_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3M2_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K3P3SX_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3P3SX_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K5E2YA_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K5E2YA_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K4ECGX_MIPI_YUV)
	{
		SENSOR_DRVNAME_S5K4ECGX_MIPI_YUV,
		{
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{DOVDD, Vol_1800, 1},
			{AFVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(OV16880_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV16880_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(S5K2P7_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2P7_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1000, 1},
			{DOVDD, Vol_1800, 1},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_Low, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
		},
	},
#endif
#if defined(S5K2P8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2P8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX258_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX258_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(IMX258_MIPI_MONO)
	{
		SENSOR_DRVNAME_IMX258_MIPI_MONO,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(IMX377_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX377_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(OV8858_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV8858_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(OV8856_MIPI_RAW)
	{SENSOR_DRVNAME_OV8856_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 2},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(S5K2X8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2X8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX214_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX214_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(IMX214_MIPI_MONO)
	{
		SENSOR_DRVNAME_IMX214_MIPI_MONO,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(IMX230_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX230_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K3L8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3L8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX362_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX362_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K2L7_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2L7_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 3},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(IMX318_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX318_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{SensorMCLK, Vol_High, 5},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(OV8865_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV8865_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(IMX219_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX219_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1000, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K3M3_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3M3_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(OV5670_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV5670_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(OV5670_MIPI_RAW_2)
	{
		SENSOR_DRVNAME_OV5670_MIPI_RAW_2,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(OV2281_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV2281_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 0},
			{RST, Vol_Low, 10},
			{RST, Vol_High, 5},
			{PDN, Vol_Low, 5},
			{PDN, Vol_High, 5},
		},
	},
#endif
#if defined(OV20880_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV20880_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{RST, Vol_Low, 1},
			{AVDD, Vol_2800, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1100, 1},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(OV5645_MIPI_YUV)
	{
		SENSOR_DRVNAME_OV5645_MIPI_YUV,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 5},
			{PDN, Vol_High, 1},
			{RST, Vol_High, 10}
		},
	},
#endif
#if defined(S5K5E9_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K5E9_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 2},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 10}
		},
	},
#endif
#if defined(S5KGD1SP_MIPI_RAW)
		{
			SENSOR_DRVNAME_S5KGD1SP_MIPI_RAW,
			{
				{RST, Vol_Low, 1},
				{AVDD, Vol_2800, 0},
				{DVDD, Vol_1100, 0},
				{DOVDD, Vol_1800, 1},
				{SensorMCLK, Vol_High, 1},
				{RST, Vol_High, 2}
			},
		},
#endif
#if defined(HI846_MIPI_RAW)
		{
			SENSOR_DRVNAME_HI846_MIPI_RAW,
			{
				{RST, Vol_Low, 1},
				{AVDD, Vol_2800, 0},
				{DVDD, Vol_1200, 0},
				{DOVDD, Vol_1800, 1},
				{SensorMCLK, Vol_High, 1},
				{RST, Vol_High, 2}
			},
		},
#endif
#if defined(GC02M0_MIPI_RAW)
		{
			SENSOR_DRVNAME_GC02M0_MIPI_RAW,
			{
				{RST, Vol_Low, 1},
				{AVDD, Vol_2800, 0},
				{DVDD, Vol_1200, 0},
				{DOVDD, Vol_1800, 1},
				{SensorMCLK, Vol_High, 1},
				{RST, Vol_High, 2}
			},
		},
#endif
#if defined(OV02A10_MIPI_MONO)
		{
			SENSOR_DRVNAME_OV02A10_MIPI_MONO,
			{
				{RST, Vol_High, 1},
				{AVDD, Vol_2800, 0},
			/*main3 has no dvdd, compatible with sub2*/
				{DVDD, Vol_1200, 0},
				{DOVDD, Vol_1800, 0},
				{SensorMCLK, Vol_High, 5},
				{RST, Vol_Low, 9}
			},
		},
#endif
#if defined(IMX686_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX686_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{AVDD, Vol_2900, 0},
		/*in alph.dts file, pin avdd controls two gpio pins*/
			/*{AVDD_1, Vol_1800, 0},*/
			{DVDD, Vol_1100, 0},
			{DOVDD, Vol_1800, 1},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(IMX616_MIPI_RAW)
		{
			SENSOR_DRVNAME_IMX616_MIPI_RAW,
			{
				{RST, Vol_Low, 1},
				{AVDD, Vol_2900, 0},
				{DVDD, Vol_1100, 0},
				{DOVDD, Vol_1800, 1},
				{SensorMCLK, Vol_High, 1},
				{RST, Vol_High, 2}
			},
		},
#endif
#if defined(IMX355_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX355_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(OV13B10_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV13B10_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{DOVDD, Vol_1800, 1},
			{RST, Vol_High, 5},
			{SensorMCLK, Vol_High, 1},
		},
	},
#endif
#if defined(OV48C_MIPI_RAW)
		{
			SENSOR_DRVNAME_OV48C_MIPI_RAW,
			{
				{RST, Vol_Low, 1},
				{SensorMCLK, Vol_High, 0},
				{DOVDD, Vol_1800, 0},
				{AVDD, Vol_2800, 0},
				{DVDD, Vol_1200, 5},
				{RST, Vol_High, 5},
			},
		},
#endif
#if defined(OV16A10_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV16A10_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{DOVDD, Vol_1800, 1},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 2},
		},
	},
#endif
#if defined(OV48B_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV48B_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 2},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(S5K3P9SP_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3P9SP_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{RST, Vol_Low, 1},
			{DVDD, Vol_1100, 1},
			{AVDD, Vol_2800, 1},
			{DOVDD, Vol_1800, 3},
			{AFVDD, Vol_2800, 5},
			{RST, Vol_High, 2},
		},
	},
#endif
#if defined(GC8054_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC8054_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{RST, Vol_Low,  1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{RST, Vol_High, 1},
			//{AFVDD, Vol_Low, 5}
		},
	},
#endif
#if defined(GC02M0B_MIPI_MONO)
	{
		SENSOR_DRVNAME_GC02M0B_MIPI_MONO,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(GC02M1B_MIPI_MONO)
	{
		SENSOR_DRVNAME_GC02M1B_MIPI_MONO,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(GC02M0B_MIPI_MONO1)
	{
		SENSOR_DRVNAME_GC02M0B_MIPI_MONO1,
		{
			{RST, Vol_Low, 1},
			//{DVDD, Vol_1200, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(GC02M0B_MIPI_MONO2)
	{
		SENSOR_DRVNAME_GC02M0B_MIPI_MONO2,
		{
			{RST, Vol_Low, 1},
			//{DVDD, Vol_1200, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(GC02K0B_MIPI_MONO)
	{
		SENSOR_DRVNAME_GC02K0B_MIPI_MONO,
		{
			{RST, Vol_Low, 1},
			//{DVDD, Vol_1200, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(OV02B10_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV02B10_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 9},
			{RST, Vol_High, 1}
		},
	},
#endif

	/* add new sensor before this line */
	{NULL,},
};

