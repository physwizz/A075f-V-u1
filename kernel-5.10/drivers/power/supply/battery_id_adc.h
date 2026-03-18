/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020.
 */
#ifndef __BATTERY_ID_ADC__
#define __BATTERY_ID_ADC__

/*HS07 code for HS07-10 by xiongxiaoliang at 20250221 start*/
/* HST11 code for AX7800A-473 by zhangziyi at 20250421 start */
#if defined(CONFIG_CUSTOM_PROJECT_OT11)
#define    BATTERY_ATL_ID_VOLT_HIGH            990
#define    BATTERY_ATL_ID_VOLT_LOW             810
/*Tab A9 code for AX6739A-1103 by lina at 20230613 start*/
#define    BATTERY_BYD_ID_VOLT_HIGH            1289
#define    BATTERY_BYD_ID_VOLT_LOW             1059
/*Tab A9 code for AX6739A-1103 by lina at 20230613 end*/
#endif

#if defined(CONFIG_CUSTOM_PROJECT_HS07)
#define    BATTERY_SDI_ID_VOLT_HIGH            627
#define    BATTERY_SDI_ID_VOLT_LOW             513
#define    BATTERY_BYD_ID_VOLT_HIGH            1320
#define    BATTERY_BYD_ID_VOLT_LOW             1080
#define    BATTERY_ATL_ID_VOLT_HIGH            990
#define    BATTERY_ATL_ID_VOLT_LOW             810
#define    BATTERY_GF_ID_VOLT_HIGH            400
#define    BATTERY_GF_ID_VOLT_LOW             250
#endif
/*HS07 code for HS07-10 by xiongxiaoliang at 20250221 end*/

#if defined(CONFIG_CUSTOM_PROJECT_HST11)
#define    BATTERY_ATL_ID_VOLT_HIGH            1000
#define    BATTERY_ATL_ID_VOLT_LOW             800
#define    BATTERY_BYD_ID_VOLT_HIGH            1300
#define    BATTERY_BYD_ID_VOLT_LOW             1100
#endif
/* HST11 code for AX7800A-473 by zhangziyi at 20250421 end */

#ifdef CONFIG_MEDIATEK_MT6577_AUXADC
int bat_id_get_adc_num(void);
int bat_id_get_adc_val(void);
int battery_get_profile_id(void);
#endif

//static int bat_id_get_adc_info(struct device *dev);
int bat_id_get_adc_num(void);
signed int battery_get_bat_id_voltage(void);
int battery_get_profile_id(void);

#endif
