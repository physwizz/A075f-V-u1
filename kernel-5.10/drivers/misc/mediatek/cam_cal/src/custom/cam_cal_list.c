// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/* Tab A9 code for SR-AX6739A-01-762 by liluling at 20230703 start */
#include <linux/kernel.h>
#include "cam_cal_list.h"
#include "eeprom_i2c_common_driver.h"
#include "eeprom_i2c_custom_driver.h"
#include "kd_imgsensor.h"
/* Tab A11 code for bring up by liugang at 20250331 start */
#if defined(CONFIG_CUSTOM_PROJECT_HST11)
#endif
/* Tab A11 code for bring up by liugang at 20250331 end */
#if defined(CONFIG_CUSTOM_PROJECT_OT11)
#include "eeprom_i2c_a0902backhi846st_driver.h"
#include "eeprom_i2c_a0904backsc820csst_driver.h"
#endif
/* HS07 V code for HS07-19  by xuyunhui at 2025/03/10 start */
/* HS07 V code for SR-AL7761A-01-438  by jiangwenhan at 2025/03/17 start */
/* HS07 V code for HS07-198  by huabinchen at 2025/03/24 start */
#if defined (CONFIG_CUSTOM_PROJECT_HS07)
#include "eeprom_i2c_a0701cxtfrontgc08a8_driver.h"
#include "eeprom_i2c_a0702lyfrontmt815_driver.h"
#include "eeprom_i2c_a0703ddfrontsc820cs_driver.h"
/* HS07 V code for HS07-198  by huabinchen at 2025/03/24 end */
/* HS07 V code for SR-AL7761A-01-438  by jiangwenhan at 2025/03/17 end */
#endif
#define MAX_EEPROM_SIZE_32K 0x8000
#define MAX_EEPROM_SIZE_16K 0x4000

extern unsigned int a1102wxfrontsc521cs_read_region(struct i2c_client *client, unsigned int addr,
	unsigned char *data, unsigned int size);
/* Tab A11 code for AX7800A-159 by zhaoyue at 20250509 start */
extern unsigned int a1101cxtfrontgc05a2_read_region(struct i2c_client *client,unsigned int addr,
	unsigned char *data,unsigned int size);
extern unsigned int a1103hdyfrontgc05a2_read_region(struct i2c_client *client,unsigned int addr,
	unsigned char *data,unsigned int size);
extern unsigned int a1101cxtbackgc08a8_read_region(struct i2c_client *client,unsigned int addr,
	unsigned char *data,unsigned int size);
extern unsigned int a1102hdybackgc08a8_read_region(struct i2c_client *client,unsigned int addr,
	unsigned char *data,unsigned int size);
struct stCAM_CAL_LIST_STRUCT g_camCalList[] = {
/* Tab A11 code for bring up by lixiaoqi at 20250512 start */
#if defined(CONFIG_CUSTOM_PROJECT_HST11)
{A1103CXTBACKSC820CS_SENSOR_ID,  0xB0, Common_read_region},
{A1102WXFRONTSC521CS_SENSOR_ID,  0x20, a1102wxfrontsc521cs_read_region},
{A1101CXTFRONTGC05A2_SENSOR_ID, 0x6E, a1101cxtfrontgc05a2_read_region,MAX_EEPROM_SIZE_16K},
{A1103HDYFRONTGC05A2_SENSOR_ID, 0x6E, a1103hdyfrontgc05a2_read_region,MAX_EEPROM_SIZE_16K},
{A1101CXTBACKGC08A8_SENSOR_ID,     0x62, a1101cxtbackgc08a8_read_region, MAX_EEPROM_SIZE_16K},
{A1102HDYBACKGC08A8_SENSOR_ID,     0x62, a1102hdybackgc08a8_read_region, MAX_EEPROM_SIZE_16K},
#endif
/* Tab A11 code for AX7800A-159 by zhaoyue at 20250509 end */
/* Tab A11 code for bring up by lixiaoqi at 20250512 end */
#if defined(CONFIG_CUSTOM_PROJECT_OT11)
	/*Below is commom sensor */
	{A0902_BACK_HI846ST_SENSOR_ID,0x40,a0902backhi846st_read_region},
	{A0901_BACK_SC800CSLY_SENSOR_ID, 0xA0, Common_read_region},
	{A0903_BACK_C8490XSJ_SENSOR_ID, 0xA0, Common_read_region},
	{A0904_BACK_SC820CSST_SENSOR_ID, 0x6c, a0904backsc820csst_read_region},
#endif
#if defined (CONFIG_CUSTOM_PROJECT_HS07)
    {A07YYXLBACKS5KJN1_SENSOR_ID,       0xA0, Common_read_region},
/* HS07 V code for SR-AL7761A-01-373  by zhongbin at 2025/03/12 start */
/* HS07 V code for SR-AL7761A-01-373  by xuyunhui at 2025/03/27 start */
    {A0701TXDBACKS5KJN1_SENSOR_ID,       0xA0, Common_read_region},
    {A0702TXDBACKGC50E1_SENSOR_ID,       0xA0, Common_read_region},
    {A0703XLBACKGC50E1_SENSOR_ID,       0xA0, Common_read_region},
    {A0704XLBACKS5KJN1_SENSOR_ID,       0xA0, Common_read_region},
    {A0705LYBACKGC50E1_SENSOR_ID,       0xB0, Common_read_region},
/* HS07 V code for SR-AL7761A-01-373  by xuyunhui at 2025/03/27 end */
/* HS07 V code for SR-AL7761A-01-373  by zhongbin at 2025/03/12 end */
/* HS07 V code for SR-AL7761A-01-438  by jiangwenhan at 2025/03/17 start */
/* HS07 V code for HS07-198  by huabinchen at 2025/03/24 start */
    {A0701CXTFRONTGC08A8_SENSOR_ID,     0x62, a0701cxtfrontgc08a8_read_region},
	{A0702LYFRONTMT815_SENSOR_ID,     0x40, a0702lyfrontmt815_read_region},
/* HS07 V code for SR-AL7761A-01-383  by huabinchen at 2025/03/25 start */
	{A0703DDFRONTSC820CS_SENSOR_ID,     0xA2, Common_read_region/*a0703ddfrontsc820cs_read_region*/},
/* HS07 V code for SR-AL7761A-01-383  by huabinchen at 2025/03/25 end */
/* HS07 V code for HS07-198  by huabinchen at 2025/03/24 end */
/* HS07 V code for SR-AL7761A-01-438  by jiangwenhan at 2025/03/17 end */
#endif
/* HS07 V code for HS07-19  by xuyunhui at 2025/03/10 end */
	/*  ADD before this line */
	{0, 0, 0}       /*end of list */
};

/* Tab A9 code for SR-AX6739A-01-762 by liluling at 20230703 end */
unsigned int cam_cal_get_sensor_list(
	struct stCAM_CAL_LIST_STRUCT **ppCamcalList)
{
	if (ppCamcalList == NULL)
		return 1;

	*ppCamcalList = &g_camCalList[0];
	return 0;
}


