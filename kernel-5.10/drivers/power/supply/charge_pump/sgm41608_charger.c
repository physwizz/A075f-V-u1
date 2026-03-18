// SPDX-License-Identifier: GPL-2.0
/*
* Copyright (c) 2025 Sgmicro TechnologyCo., Ltd.
*/

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/err.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/debugfs.h>
#include <linux/bitops.h>
#include <linux/math64.h>
#include <linux/regmap.h>

#include "pd_policy_manager.h"

#include <linux/power/gxy_psy_sysfs.h>
//#define CONFIG_MTK_CHARGER_V4P19 1

#define CONFIG_MTK_CHARGER_V5P10	1

#ifdef CONFIG_MTK_CLASS
#include "charger_class.h"
#ifdef CONFIG_MTK_CHARGER_V4P19
#include "mtk_charger_intf.h"
#endif /*CONFIG_MTK_CHARGER_V4P19*/
#endif /*CONFIG_MTK_CLASS*/

#ifdef CONFIG_SGM_DVCHG_CLASS
//#include "dvchg_class.h"
#endif /*CONFIG_SGM_DVCHG_CLASS*/

#define SGM41608_DRV_VERSION		"V1.1"
#define	SGM_INFO(fmt, args...) printk("[SGMICRO] %s() line=%d." fmt, __func__, __LINE__, ##args)
extern void gxy_bat_set_cpinfo(enum gxy_bat_cp_info pinfo_data);
extern void gxy_bat_set_cp_sts(bool cp_status);
static bool g_is_sgm41608_prob_success = false;

enum {
    SGM41608_STANDALONG = 0,
    SGM41608_SECONDARY,
    SGM41608_PRIMARY,
};

//static const char* sgm41608_psy_name[] = {
//    [SGM41608_STANDALONG] = "sgm-cp-standalone",
//    [SGM41608_SECONDARY] = "sgm-cp-second",
//    [SGM41608_PRIMARY] = "sgm-cp-primary",
//};

static const char* sgm41608_irq_name[] = {
    [SGM41608_STANDALONG] = "sgm41608-standalone-irq",
    [SGM41608_SECONDARY] = "sgm41608-second-irq",
    [SGM41608_PRIMARY] = "sgm41608-primary-irq",
};

static int sgm41608_mode_data[] = {
    [SGM41608_STANDALONG] = SGM41608_STANDALONG,
    [SGM41608_SECONDARY] = SGM41608_SECONDARY,
    [SGM41608_PRIMARY] = SGM41608_PRIMARY,
};

enum {
	ADC_IBUS = 0,
	ADC_VBUS,
	ADC_VAC,
	ADC_VOUT,
    ADC_VBAT,
    ADC_IBAT,
    ADC_TSBAT,
    ADC_TDIE,
    ADC_MAX_NUM,
}SGM41608_ADC_CH;

enum sgm41608_notify {
    SGM41608_NOTIFY_OTHER = 0,
	SGM41608_NOTIFY_IBUSOCP,
	SGM41608_NOTIFY_VBUSOVP,
	SGM41608_NOTIFY_IBATOCP,
	SGM41608_NOTIFY_VOUTOVP,
	SGM41608_NOTIFY_VBATOVP,
	SGM41608_NOTIFY_TDIE,
	SGM41608_NOTIFY_VAC_OVP,
};

struct flag_bit {
    int notify;
    int mask;
    char *name;
};

struct intr_flag {
    int reg;
    int len;
    struct flag_bit bit[8];
};

static struct intr_flag cp_intr_flag[] = {
	{ .reg = 0x15, .len = 8, .bit = {
			{.mask = BIT(0), .name = "VOUT_INSERT flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(1), .name = "VAC_INSERT flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(2), .name = "VBUS_PRESENT flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(3), .name = "VOUT_TH_CHG_EN flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(4), .name = "VOUT_TH_REV_EN flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(5), .name = "PEAK_OCP_RVS flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(6), .name = "CONV_OCP flag", .notify = SGM41608_NOTIFY_OTHER},
        	{.mask = BIT(7), .name = "CP_SWITCHING flag", .notify = SGM41608_NOTIFY_OTHER},
        },
    },
    { .reg = 0x16, .len = 6, .bit = {
			{.mask = BIT(1), .name = "RVS_SSOK flag", .notify = SGM41608_NOTIFY_OTHER},
    		{.mask = BIT(2), .name = "VBUS_ERRLO flag", .notify = SGM41608_NOTIFY_OTHER},
    		{.mask = BIT(3), .name = "VBUS_ERRHI flag", .notify = SGM41608_NOTIFY_OTHER},
    		{.mask = BIT(4), .name = "VBUS_SHORT flag", .notify = SGM41608_NOTIFY_OTHER,},
    		{.mask = BIT(5), .name = "IBUS_UCP_RISE flag", .notify = SGM41608_NOTIFY_OTHER},
    		{.mask = BIT(6), .name = "PIN_DIAG_FAIL flag", .notify = SGM41608_NOTIFY_OTHER},
            {.mask = BIT(7), .name = "ADC_DONE flag", .notify = SGM41608_NOTIFY_OTHER},
    	},
    },
    { .reg = 0x17, .len = 8, .bit = {
			{.mask = BIT(0), .name = "VBAT_OVP flag", .notify = SGM41608_NOTIFY_VBATOVP},
			{.mask = BIT(1), .name = "VOUT_OVP flag", .notify = SGM41608_NOTIFY_VOUTOVP},
			{.mask = BIT(2), .name = "VBUS_OVP flag", .notify = SGM41608_NOTIFY_VBUSOVP},
			{.mask = BIT(3), .name = "IBAT_OCP flag", .notify = SGM41608_NOTIFY_IBATOCP},
			{.mask = BIT(4), .name = "IBUS_OCP flag", .notify = SGM41608_NOTIFY_IBUSOCP},
			{.mask = BIT(5), .name = "IBUS_UCP_FALL flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(6), .name = "VAC_OVP flag", .notify = SGM41608_NOTIFY_VAC_OVP},
            {.mask = BIT(7), .name = "POR_FLAG", .notify = SGM41608_NOTIFY_VAC_OVP},
    	},
    },
    { .reg = 0x18, .len = 8, .bit = {
			{.mask = BIT(0), .name = "WDT TIMEOUT flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(1), .name = "SS_TIMEOUT flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(2), .name = "TSBAT_FLT flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(3), .name = "TSHUT flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(4), .name = "PMID2VOUT_OVP flag", .notify = SGM41608_NOTIFY_OTHER},
			{.mask = BIT(5), .name = "PMID2VOUT_UVP flag", .notify = SGM41608_NOTIFY_OTHER},
		},
	},
};

/************************************************************************/
#define SGM41608_DEVICE_ID			0x91
#define SGM41608_REGMAX				0x2E
#define SGM41608_STAT1_REG			0x11
#define SGM41608_STAT2_REG			0x12
#define SGM41608_STAT3_REG			0x13
#define SGM41608_STAT4_REG			0x14

#define SGM41608_ADC_START_REG		0x1F


#define SGM41608_CHARGE_MODE_F21     	0
#define SGM41608_CHARGE_MODE_F11     	1
#define SGM41608_CHARGE_MODE_R12		2
#define SGM41608_CHARGE_MODE_R11		3

enum sgm41608_reg_range {
    SGM41608_VBAT_OVP,
    SGM41608_IBAT_OCP,
    SGM41608_VBUS_OVP11,
    SGM41608_VBUS_OVP21,
    SGM41608_IBUS_OCP,
    SGM41608_VOUT_OVP,
    SGM41608_PMID2VOUT_OVP,
    SGM41608_PMID2VOUT_UVP,
};

struct reg_range {
    u32 min;
    u32 max;
    u32 step;
    u32 offset;
    const u32 *table;
    u16 num_table;
    bool round_up;
};

#define SGM41608_CHG_RANGE(_min, _max, _step, _offset, _ru) \
{ \
    .min = _min, \
    .max = _max, \
    .step = _step, \
    .offset = _offset, \
    .round_up = _ru, \
}

#define SGM41608_CHG_RANGE_T(_table, _ru) \
    { .table = _table, .num_table = ARRAY_SIZE(_table), .round_up = _ru, }


static const struct reg_range sgm41608_reg_range[] = {
    [SGM41608_VBAT_OVP]     = SGM41608_CHG_RANGE(4040, 5110, 10, 3840, false),
    [SGM41608_IBAT_OCP]     = SGM41608_CHG_RANGE(500, 12700, 100, 0, false),
    [SGM41608_VBUS_OVP11]   = SGM41608_CHG_RANGE(5250, 7000, 250, 5250, false),
    [SGM41608_VBUS_OVP21]   = SGM41608_CHG_RANGE(10500, 14000, 500, 10500, false),
    [SGM41608_IBUS_OCP]     = SGM41608_CHG_RANGE(1000, 6350, 50, 0, false),
    [SGM41608_VOUT_OVP]     = SGM41608_CHG_RANGE(4700, 5000, 100, 4700, false),
    [SGM41608_PMID2VOUT_OVP] = SGM41608_CHG_RANGE(400, 1100, 100, 400, false),
    [SGM41608_PMID2VOUT_UVP] = SGM41608_CHG_RANGE(100, 450, 50, 100, false),
};


enum sgm41608_fields {
    BAT_OVP_DIS,BAT_OVP, //0x00
    BAT_OCP_DIS,BAT_OCP,//0x01
	BUS_OVP_DIS,VAC_OVP_DIS,VAC_OVP,BUS_OVP,//0x02
	SGM_DEVICE_ID,//0x03
	BUS_OCP_DIS,BUS_OCP,//0x04
	PMID2VOUT_OVP_DIS,PMID2VOUT_OVP_CFG,PMID2VOUT_OVP,//0x05
	PMID2VOUT_UVP_DIS,PMID2VOUT_UVP,//0x06
	BUS_UCP,PEAK_OCP_SET,PEAK_OCP_RVS,PEAK_OCP_DIS,EN_HIZ,
	TSBAT_FLT_DIS,TSBAT_FLT,//0x08
	IBAT_RSNS,VAC_PDN_EN,PMID_PD_EN,VBUS_PD_EN,REG_RST,VOUT_OVP_DIS,OP_MODE,//0x09
	SS_TIMEOUT,FORCE_VAC_OK,WDT_TIMER,//0x0A
	FREQ_DIV,FREQ_SHIFT,FSW_SET,//0x0B
	CHG_EN,VBUS_SHORT_DIS,PIN_DIAG_EN,OVPGATE_ISET,IBATSNS_HS_EN,EN_IBATSNS,//0x0C
	WDT_DIS,ACDRV_MANUAL_EN,ACDRV_EN,EN_OTG,DIS_ACDRV,TSHUT_DIS,BUSUCP_DIS,
	SYNC_FUNCTION_EN,SYNC_MASTER_EN,VBUS_ERRLO_DIS,VBUS_ERRHI_DIS,VOUT_OVP,//0x0E
	PMID2VOUT_UVP_DEG,PMID2VOUT_OVP_DEG,IBAT_OCP_DEG,VBUS_OVP_DEG,VOUT_OVP_DEG,//0x0F
	IBUSUCP_FALL_DEG,IBUSOCP_DEG,VBAT_OVP_DEG,//0x10
	CP_SWITCH_STAT,CONV_OCP_STAT,PEAK_OCP_STAT,VOUT_TH_RVS_EN_STAT,VOUT_TH_CHG_EN_STAT,VBUS_PRESENT_STAT,VAC_INSERT_STAT,VOUT_INSERT_STAT,//0x11
	ADC_DONE_STAT,PIN_DIAG_FALL_STAT,IBUS_UCP_RISE_STAT,VBUS_SHORT_STAT,VBUS_ERRHI_STAT,VBUS_ERRLO_STAT,RVS_SSOK_STAT,//0x12
	VAC_OVP_STAT,IBUS_UCP_FALL_STAT,IBUS_OCP_STAT,IBAT_OCP_STAT,VBUS_OVP_STAT,VOUT_OVP_STAT,VBAT_OVP_STAT,//0x13
	PMID2VOUT_UVP_STAT,PMID2VOUT_OVP_STAT,TSHUT_STAT,TSBAT_FLT_STAT,SS_TIMEOUT_STAT,WD_TIMEOUT_STAT,//0x14
	CP_SWITCHING_MASK,CONV_OCP_MASK,PEAK_OCP_RVS_MASK,VOUT_TH_REV_EN_MASK,VOUT_TH_CHG_EN_MASK,//0x19
	VBUS_PRESENT_MASK,VAC_INSERT_MASK,VOUT_INSERT_MASK,
	ADC_DONE_MASK,PIN_DIAG_FAIL_MASK,IBUS_UCP_RISE_MASK,VBUS_SHORT_MASK,VBUS_ERRLO_MASK,VBUS_ERRHI_MASK,RVS_SSOK_MASK,//0x1A
	POR_MASK,VAC_OVP_MASK,IBUS_UCP_FALL_MASK,IBUS_OCP_MASK,IBAT_OCP_MASK,VBUS_OVP_MASK,VOUT_OVP_MASK,VBAT_OVP_MASK,//0x1B
	PMID2VOUT_UVP_MASK,PMID2VOUT_OVP_MASK,TSHUT_MASK,TSBAT_FLT_MASK,SS_TIMEOUT_MASK,WD_TIMEOUT_MASK, //0x1C

	ADC_EN,ADC_RATE,ADC_AVG,ADC_AVG_INIT,//0x1D
	IBUS_ADC_DIS,VBUS_ADC_DIS,VAC_ADC_DIS,VOUT_ADC_DIS,VBAT_ADC_DIS,IBAT_ADC_DIS,TSBAT_ADC_DIS,TDIE_ADC_DIS,//0x1E

	F_MAX_FIELDS,
};


//REGISTER
static const struct reg_field sgm41608_reg_fields[] = {
    /*reg00*/
	[BAT_OVP_DIS] = REG_FIELD(0x00, 7, 7),
    [BAT_OVP] = REG_FIELD(0x00, 0, 6),
    /*reg01*/
    [BAT_OCP_DIS] = REG_FIELD(0x01, 7, 7),
    [BAT_OCP] = REG_FIELD(0x01, 0, 6),
    /*reg02*/
    [BUS_OVP_DIS] = REG_FIELD(0x02, 7, 7),
    [VAC_OVP_DIS] = REG_FIELD(0x02, 6, 6),
    [VAC_OVP] = REG_FIELD(0x02, 3, 5),
    [BUS_OVP] = REG_FIELD(0x02, 0, 2),
    /*reg03*/
    [SGM_DEVICE_ID] = REG_FIELD(0x03, 0, 7),
    /*reg04*/
    [BUS_OCP_DIS] = REG_FIELD(0x04, 7, 7),
    [BUS_OCP] = REG_FIELD(0x04, 0, 6),
    /*reg05*/
    [PMID2VOUT_OVP_DIS] = REG_FIELD(0x05, 7, 7),
    [PMID2VOUT_OVP_CFG] = REG_FIELD(0x05, 5, 5),
    [PMID2VOUT_OVP] = REG_FIELD(0x05, 0, 2),
    /*reg06*/
    [PMID2VOUT_UVP_DIS] = REG_FIELD(0x06, 7, 7),
    [PMID2VOUT_UVP] = REG_FIELD(0x06, 0, 2),
    /*reg07*/
    [BUS_UCP] = REG_FIELD(0x07, 5, 5),
    [PEAK_OCP_SET] = REG_FIELD(0x07, 3, 4),
    [PEAK_OCP_RVS] = REG_FIELD(0x07, 2, 2),
    [PEAK_OCP_DIS] = REG_FIELD(0x07, 1, 1),
    [EN_HIZ] = REG_FIELD(0x07, 0, 0),
    /*reg08*/
    [TSBAT_FLT_DIS] = REG_FIELD(0x08, 7, 7),
    [TSBAT_FLT] = REG_FIELD(0x08, 0, 5),
    /*reg09*/
    [IBAT_RSNS] = REG_FIELD(0x09, 7, 7),
    [VAC_PDN_EN] = REG_FIELD(0x09, 6, 6),
    [PMID_PD_EN] = REG_FIELD(0x09, 5, 5),
    [VBUS_PD_EN] = REG_FIELD(0x09, 4, 4),
    [REG_RST] = REG_FIELD(0x09, 3, 3),
    [VOUT_OVP_DIS] = REG_FIELD(0x09, 2, 2),
    [OP_MODE] = REG_FIELD(0x09, 0, 1),
    /*reg0a*/
    [SS_TIMEOUT] = REG_FIELD(0x0a, 5, 7),
    [FORCE_VAC_OK] = REG_FIELD(0x0a, 2, 2),
    [WDT_TIMER] = REG_FIELD(0x0a, 0, 1),
	/*reg0b*/
	[FREQ_DIV] = REG_FIELD(0x0b, 6, 6),
	[FREQ_SHIFT] = REG_FIELD(0x0b, 4, 5),
	[FSW_SET] = REG_FIELD(0x0b, 0, 3),
	/*reg0c*/
    [CHG_EN] = REG_FIELD(0x0c, 7, 7),
    [VBUS_SHORT_DIS] = REG_FIELD(0x0c, 5, 5),
    [PIN_DIAG_EN] = REG_FIELD(0x0c, 4, 4),
    [OVPGATE_ISET] = REG_FIELD(0x0c, 2, 3),
    [IBATSNS_HS_EN] = REG_FIELD(0x0c, 1, 1),
    [EN_IBATSNS] = REG_FIELD(0x0c, 0, 0),
	/*reg0d*/
   	[WDT_DIS] = REG_FIELD(0x0d, 7, 7),
   	[ACDRV_MANUAL_EN] = REG_FIELD(0x0d, 6, 6),
   	[ACDRV_EN] = REG_FIELD(0x0d, 5, 5),
   	[EN_OTG] = REG_FIELD(0x0d, 4, 4),
   	[DIS_ACDRV] = REG_FIELD(0x0d, 2, 2),
   	[TSHUT_DIS] = REG_FIELD(0x0d, 1, 1),
   	[BUSUCP_DIS] = REG_FIELD(0x0d, 0, 0),
	/*reg0e*/
    [SYNC_FUNCTION_EN] = REG_FIELD(0x0e, 7, 7),
    [SYNC_MASTER_EN] = REG_FIELD(0x0e, 6, 6),
    [VBUS_ERRLO_DIS] = REG_FIELD(0x0e, 5, 5),
    [VBUS_ERRHI_DIS] = REG_FIELD(0x0e, 4, 4),
    [VOUT_OVP] = REG_FIELD(0x0e, 2, 3),
	/*reg0F*/
    [PMID2VOUT_UVP_DEG] = REG_FIELD(0x0F, 7, 7),
    [PMID2VOUT_OVP_DEG] = REG_FIELD(0x0F, 6, 6),
    [IBAT_OCP_DEG] = REG_FIELD(0x0F, 4, 5),
    [VBUS_OVP_DEG] = REG_FIELD(0x0F, 2, 2),
    [VOUT_OVP_DEG] = REG_FIELD(0x0F, 1, 1),
	/*reg10*/
    [IBUSUCP_FALL_DEG] = REG_FIELD(0x10, 6, 7),
    [IBUSOCP_DEG] = REG_FIELD(0x10, 4, 5),
    [VBAT_OVP_DEG] = REG_FIELD(0x10, 2, 3),
	//reg11
	[CP_SWITCH_STAT] = REG_FIELD(0x11, 7, 7),
    [CONV_OCP_STAT] = REG_FIELD(0x11, 6, 6),
    [PEAK_OCP_STAT] = REG_FIELD(0x11, 5, 5),
    [VOUT_TH_RVS_EN_STAT] = REG_FIELD(0x11, 4, 4),
    [VOUT_TH_CHG_EN_STAT] = REG_FIELD(0x11, 3, 3),
    [VBUS_PRESENT_STAT] = REG_FIELD(0x11, 2, 2),
    [VAC_INSERT_STAT] = REG_FIELD(0x11, 1, 1),
	[VOUT_INSERT_STAT] = REG_FIELD(0x11, 0, 0),
	//reg12
	[ADC_DONE_STAT] = REG_FIELD(0x12, 7, 7),
    [PIN_DIAG_FALL_STAT] = REG_FIELD(0x12, 6, 6),
    [IBUS_UCP_RISE_STAT] = REG_FIELD(0x12, 5, 5),
    [VBUS_SHORT_STAT] = REG_FIELD(0x12, 4, 4),
    [VBUS_ERRHI_STAT] = REG_FIELD(0x12, 3, 3),
    [VBUS_ERRLO_STAT] = REG_FIELD(0x12, 2, 2),
    [RVS_SSOK_STAT] = REG_FIELD(0x12, 1, 1),
	//reg13
    [VAC_OVP_STAT] = REG_FIELD(0x13, 6, 6),
    [IBUS_UCP_FALL_STAT] = REG_FIELD(0x13, 5, 5),
    [IBUS_OCP_STAT] = REG_FIELD(0x13, 4, 4),
    [IBAT_OCP_STAT] = REG_FIELD(0x13, 3, 3),
    [VBUS_OVP_STAT] = REG_FIELD(0x13, 2, 2),
    [VOUT_OVP_STAT] = REG_FIELD(0x13, 1, 1),
    [VBAT_OVP_STAT] = REG_FIELD(0x13, 0, 0),
	//reg14
    [PMID2VOUT_UVP_STAT] = REG_FIELD(0x14, 5, 5),
    [PMID2VOUT_OVP_STAT] = REG_FIELD(0x14, 4, 4),
    [TSHUT_STAT] = REG_FIELD(0x14, 3, 3),
    [TSBAT_FLT_STAT] = REG_FIELD(0x14, 2, 2),
    [SS_TIMEOUT_STAT] = REG_FIELD(0x14, 1, 1),
    [WD_TIMEOUT_STAT] = REG_FIELD(0x14, 0, 0),
    /*reg19*/
    [CP_SWITCHING_MASK] = REG_FIELD(0x19, 7, 7),
    [CONV_OCP_MASK] = REG_FIELD(0x19, 6, 6),
    [PEAK_OCP_RVS_MASK] = REG_FIELD(0x19, 5, 5),
    [VOUT_TH_REV_EN_MASK] = REG_FIELD(0x19, 4, 4),
    [VOUT_TH_CHG_EN_MASK] = REG_FIELD(0x19, 3, 3),
    [VBUS_PRESENT_MASK] = REG_FIELD(0x19, 2, 2),
    [VAC_INSERT_MASK] = REG_FIELD(0x19, 1, 1),
	[VOUT_INSERT_MASK] = REG_FIELD(0x19, 0, 0),

	/*reg1A*/
    [ADC_DONE_MASK] = REG_FIELD(0x1A, 7, 7),
    [PIN_DIAG_FAIL_MASK] = REG_FIELD(0x1A, 6, 6),
    [IBUS_UCP_RISE_MASK] = REG_FIELD(0x1A, 5, 5),
    [VBUS_SHORT_MASK] = REG_FIELD(0x1A, 4, 4),
    [VBUS_ERRLO_MASK] = REG_FIELD(0x1A, 3, 3),
    [VBUS_ERRHI_MASK] = REG_FIELD(0x1A, 2, 2),
    [RVS_SSOK_MASK] = REG_FIELD(0x1A, 1, 1),

	/*reg1B*/
    [POR_MASK] = REG_FIELD(0x1B, 7, 7),
    [VAC_OVP_MASK] = REG_FIELD(0x1B, 6, 6),
    [IBUS_UCP_FALL_MASK] = REG_FIELD(0x1B, 5, 5),
    [IBUS_OCP_MASK] = REG_FIELD(0x1B, 4, 4),
    [IBAT_OCP_MASK] = REG_FIELD(0x1B, 3, 3),
    [VBUS_OVP_MASK] = REG_FIELD(0x1B, 2, 2),
    [VOUT_OVP_MASK] = REG_FIELD(0x1B, 1, 1),
	[VBAT_OVP_MASK] = REG_FIELD(0x1B, 0, 0),

	/*reg1C*/
    [PMID2VOUT_UVP_MASK] = REG_FIELD(0x1C, 5, 5),
    [PMID2VOUT_OVP_MASK] = REG_FIELD(0x1C, 4, 4),
    [TSHUT_MASK] = REG_FIELD(0x1C, 3, 3),
    [TSBAT_FLT_MASK] = REG_FIELD(0x1C, 2, 2),
    [SS_TIMEOUT_MASK] = REG_FIELD(0x1C, 1, 1),
	[WD_TIMEOUT_MASK] = REG_FIELD(0x1C, 0, 0),

	/*reg1D*/
    [ADC_EN] = REG_FIELD(0x1D, 7, 7),
    [ADC_RATE] = REG_FIELD(0x1D, 6, 6),
    [ADC_AVG] = REG_FIELD(0x1D, 5, 5),
    [ADC_AVG_INIT] = REG_FIELD(0x1D, 4, 4),

	/*reg1E*/
	[IBUS_ADC_DIS] = REG_FIELD(0x1E, 7, 7),
	[VBUS_ADC_DIS] = REG_FIELD(0x1E, 6, 6),
	[VAC_ADC_DIS] = REG_FIELD(0x1E, 5, 5),
	[VOUT_ADC_DIS] = REG_FIELD(0x1E, 4, 4),
	[VBAT_ADC_DIS] = REG_FIELD(0x1E, 3, 3),
	[IBAT_ADC_DIS] = REG_FIELD(0x1E, 2, 2),
	[TSBAT_ADC_DIS] = REG_FIELD(0x1E, 1, 1),
	[TDIE_ADC_DIS] = REG_FIELD(0x1E, 0, 0),

};

static const struct regmap_config sgm41608_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,

    .max_register = SGM41608_REGMAX,
};

/************************************************************************/

struct sgm41608_cfg_e {
    int vbat_ovp_dis;
	int ibat_ocp_dis;
	int ibus_ucp_dis;
	int ibus_rcp_dis;
	int vbus_low_dis;
	int vbus_hi_dis;

	int vbat_ovp;
    int ibat_ocp;
	int ibat_ocp_alm;
	int ibat_ucp_alm;
	int bus_ucp;
	int bus_rcp;
	int vbus_ovp;
	int vbus_ovp_alm;
	int ibus_ocp;
	int ibus_ocp_alm;
	int vac_ovp;
	int vout_ovp;

	int fsw_set;
	int wdt_dis;
    int wd_timeout;
	int rsns;
	int operation_mode;

	int vbus_low;
	int vbus_hi;

};

struct sgm41608_chip {
    struct device *dev;
    struct i2c_client *client;
    struct regmap *regmap;
    struct regmap_field *rmap_fields[F_MAX_FIELDS];

    struct sgm41608_cfg_e cfg;
    int irq_gpio;
    int lpm_gpio;
    int irq;
    int mode;

    bool charge_enabled;
    int usb_present;
    int vbus_volt;
    int ibus_curr;
    int vbat_volt;
    int ibat_curr;
    int die_temp;

#ifdef CONFIG_MTK_CLASS
    struct charger_device *chg_dev;
#endif /*CONFIG_MTK_CLASS*/

#ifdef CONFIG_SGM_DVCHG_CLASS
    struct dvchg_dev *charger_pump;
#endif /*CONFIG_SGM_DVCHG_CLASS*/

    const char *chg_dev_name;
	bool otg_tx_mode;

    struct power_supply_desc psy_desc;
    struct power_supply_config psy_cfg;
    struct power_supply *psy;
};

#ifdef CONFIG_MTK_CLASS
static const struct charger_properties sgm41608_chg_props = {
	.alias_name = "sgm41608_chg",
};
#endif /*CONFIG_MTK_CLASS*/

struct sgm41608_chip *g_sgm41608 = NULL;
extern struct gxy_cp_ops g_gxy_cp_ops;

/********************COMMON API***********************/
__maybe_unused static u8 val2reg(enum sgm41608_reg_range id, u32 val)
{
    int i;
    u8 reg;
    const struct reg_range *range= &sgm41608_reg_range[id];

    if (!range)
        return val;

    if (range->table) {
        if (val <= range->table[0])
            return 0;
        for (i = 1; i < range->num_table - 1; i++) {
            if (val == range->table[i])
                return i;
            if (val > range->table[i] &&
                val < range->table[i + 1])
                return range->round_up ? i + 1 : i;
        }
        return range->num_table - 1;
    }
    if (val <= range->min)
        reg = 0;
    else if (val >= range->max)
        reg = (range->max - range->offset) / range->step;
    else if (range->round_up)
        reg = (val - range->offset) / range->step + 1;
    else
        reg = (val - range->offset) / range->step;
    return reg;
}

__maybe_unused static u32 reg2val(enum sgm41608_reg_range id, u8 reg)
{
    const struct reg_range *range= &sgm41608_reg_range[id];
    if (!range)
        return reg;
    return range->table ? range->table[reg] :
                  range->offset + range->step * reg;
}
/*********************************************************/
static int sgm41608_field_read(struct sgm41608_chip *sgm,
                enum sgm41608_fields field_id, int *val)
{
    int ret;

    ret = regmap_field_read(sgm->rmap_fields[field_id], val);
    if (ret < 0) {
        SGM_INFO("i2c read field %d fail: %d\n", field_id, ret);
    }

    return ret;
}

static int sgm41608_field_write(struct sgm41608_chip *sgm,
                enum sgm41608_fields field_id, int val)
{
    int ret;

    ret = regmap_field_write(sgm->rmap_fields[field_id], val);
    if (ret < 0) {
        SGM_INFO("i2c write field %d fail: %d\n", field_id, ret);
    }

    return ret;
}

static int sgm41608_read_block(struct sgm41608_chip *sgm,
                int reg, uint8_t *val, int len)
{
    int ret;

    ret = regmap_bulk_read(sgm->regmap, reg, val, len);
    if (ret < 0) {
        SGM_INFO("i2c read %02x block failed %d\n", reg, ret);
    }

    return ret;
}

/*******************************************************/
__maybe_unused static int sgm41608_detect_device(struct sgm41608_chip *sgm)
{
    int ret;
    int val;

    ret = sgm41608_field_read(sgm, SGM_DEVICE_ID, &val);
    if (ret < 0) {
        return ret;
    }

    if (val != SGM41608_DEVICE_ID) {
        SGM_INFO("not find SGM41608, ID = 0x%02x\n", val);
        return -EINVAL;
    }
	SGM_INFO("Found SGM41608 dev, ID = 0x%02x\n", val);

    return ret;
}

__maybe_unused static int sgm41608_reg_reset(struct sgm41608_chip *sgm)
{
    return sgm41608_field_write(sgm, REG_RST, 1);
}

__maybe_unused static int sgm41608_dump_reg(struct sgm41608_chip *sgm)
{
    int ret;
    int i;
    int val;

    for (i = 0; i <= SGM41608_REGMAX; i++) {
        ret = regmap_read(sgm->regmap, i, &val);
        SGM_INFO("reg[0x%02x] = 0x%02x\n", i, val);
    }

    return ret;
}

__maybe_unused static int sgm41608_enable_charge(struct sgm41608_chip *sgm, bool en)
{
    int ret;

    SGM_INFO("chg enable: %d\n", en);

	if (en)
    	ret = sgm41608_field_write(sgm, CHG_EN, 1);
	else
		ret = sgm41608_field_write(sgm, CHG_EN, 0);

	sgm41608_dump_reg(sgm);
    return ret;
}

__maybe_unused static int sgm41608_check_charge_enabled(struct sgm41608_chip *sgm, bool *enabled)
{
    int ret, val;

    ret = sgm41608_field_read(sgm, CHG_EN, &val);

	if (val)
    	*enabled = true;
	else
		*enabled = false;

     SGM_INFO("chg enable: %d\n", val);

    return ret;
}

__maybe_unused static int sgm41608_get_status(struct sgm41608_chip *sgm, uint32_t *status)
{
    int ret, val;
    *status = 0;

	ret = sgm41608_read_block(sgm,SGM41608_STAT2_REG, (uint8_t *)&val, 1);
    if (ret < 0) {
        return ret;
    }
    if (val & BIT(3)) //VBUS_ERRHI_STAT
        *status |= BIT(3);

    if (val & BIT(2)) //VBUS_ERRLO_STAT
        *status |= BIT(2);

    return ret;
}

__maybe_unused static int sgm41608_enable_adc(struct sgm41608_chip *sgm, bool en)
{
    SGM_INFO("set adc enable: %d\n", en);
    return sgm41608_field_write(sgm, ADC_EN, !!en);
}

__maybe_unused static int sgm41608_set_adc_scanrate(struct sgm41608_chip *sgm, bool oneshot)
{
    SGM_INFO("adc oneshot enable: %d\n", oneshot);
    return sgm41608_field_write(sgm, ADC_RATE, !!oneshot);
}

static int sgm41608_get_adc_data(struct sgm41608_chip *sgm,
            int channel, int *result)
{
    uint8_t val[2] = {0};
    int ret;
	int step = 0;

    if(channel >= ADC_MAX_NUM)
        return -EINVAL;

    ret = sgm41608_read_block(sgm, SGM41608_ADC_START_REG + (channel << 1), val, 2);
    if (ret < 0) {
        return ret;
    }

	switch(channel)
	{
	case 0:	//IBUS step = 2.5mA
		step = 25;
		*result = (val[1] | ((val[0] & 0x0F) << 8)) * step / 10;
		break;
	case 1:	//VBUS step = 3.75mV
		step = 375;
		*result = (val[1] | ((val[0] & 0x0F) << 8)) * step / 100;
		break;
	case 2:	//VAC
		step = 5;
		*result = (val[1] | ((val[0] & 0x0F) << 8)) * step;
		break;
	case 3:	//VOUT step = 1.25mV
		step = 125;
		*result = (val[1] | ((val[0] & 0x0F) << 8)) * step / 100;
		break;
	case 4:	//VBAT step = 1.25mV
		step = 125;
		*result = (val[1] | ((val[0] & 0x0F) << 8)) * step / 100;
		break;
	case 5:	//IBAT step = 3.125mA
		step = 3125;
		*result = (val[1] | ((val[0] & 0x0F) << 8)) * step / 1000;
		break;
	case 6:	//TSBAT step = 0.44mV
		step = 44;
		*result = (val[1] | ((val[0] & 0x0F) << 8)) * step / 100;
		break;
	case 7:	//TDIE step = 0.5
		step = 5;
		*result = (val[1] | ((val[0] & 0x01) << 8)) * step / 10;
		break;
	default:
		break;
	}

    SGM_INFO("adc channel %d = %d\n", channel, *result);

    return ret;
}

__maybe_unused static int sgm41608_set_busovp_th(struct sgm41608_chip *sgm, int threshold)
{
	u8 val = 0;
	int ret = 0;

	if (sgm->cfg.operation_mode == SGM41608_CHARGE_MODE_F11 ||
		sgm->cfg.operation_mode == SGM41608_CHARGE_MODE_R11) {
		val = val2reg(SGM41608_VBUS_OVP11, threshold);
	} else {
		val = val2reg(SGM41608_VBUS_OVP21, threshold);
	}
	ret = sgm41608_field_write(sgm, BUS_OVP, val);

	if (ret)
		SGM_INFO("set threshold: %d-%#x\n", threshold, val);
	else
		SGM_INFO("i2c error, set threshold: %d-%#x\n", threshold, val);
	return ret;
}

__maybe_unused static int sgm41608_set_busocp_th(struct sgm41608_chip *sgm, int threshold)
{
	u8 val;

	val = val2reg(SGM41608_IBUS_OCP, threshold);
	SGM_INFO("threshold: %d-%#x\n", threshold, val);

    return sgm41608_field_write(sgm, BUS_OCP, val);
}

__maybe_unused static int sgm41608_set_batovp_th(struct sgm41608_chip *sgm, int threshold)
{
	u8 val;

	val = val2reg(SGM41608_VBAT_OVP, threshold);
	SGM_INFO("threshold: %d-%#x\n", threshold, val);

    return sgm41608_field_write(sgm, BAT_OVP, val);
}

__maybe_unused static int sgm41608_set_batocp_th(struct sgm41608_chip *sgm, int threshold)
{
	u8 val;

	val = val2reg(SGM41608_VBAT_OVP, threshold);
	SGM_INFO("threshold: %d-%#x\n", threshold, val);

	return sgm41608_field_write(sgm, BAT_OCP, val);
}

__maybe_unused static int sgm41608_is_vbuslowerr(struct sgm41608_chip *sgm, bool *err)
{
    int ret;
    int val;

	ret = sgm41608_field_read(sgm, VBUS_ERRHI_STAT, &val);
    if(ret < 0) {
        return ret;
    }

	SGM_INFO("vbuslowerr reg: %#x.\n", val);
	*err = (bool)val;

    return ret;
}
__maybe_unused static int sgm41608_get_vbus_present_status(struct sgm41608_chip *sgm)
{
    int ret;
    int val;
    ret = sgm41608_field_read(sgm, VBUS_PRESENT_STAT, &val);
    if(ret < 0) {
        return ret;
    }
    SGM_INFO("vbus status:%d",val);
    return val;
}

__maybe_unused static int sgm41608_get_wdt_status(struct sgm41608_chip *sgm)
{
    int ret;
    int val;
    ret = sgm41608_field_read(sgm, WDT_DIS, &val);
    if(ret < 0) {
        return ret;
    }
    SGM_INFO("wdt status:%d",val);
    return val;
}

__maybe_unused static int sgm41608_disable_watch_dog(struct sgm41608_chip *sgm, bool en)
{
    SGM_INFO("wdt status:%d", en);
    return sgm41608_field_write(sgm, WDT_DIS, !!en);
}

__maybe_unused static int sgm41608_init_device(struct sgm41608_chip *sgm)
{
    int ret = 0;
    int i;
    struct {
        enum sgm41608_fields field_id;
        int conv_data;
    } props[] = {
		{WDT_DIS, sgm->cfg.wdt_dis},
        {BAT_OVP_DIS, sgm->cfg.vbat_ovp_dis},
        //{BAT_OCP_DIS, sgm->cfg.ibat_ocp_dis},
        {BUSUCP_DIS, sgm->cfg.ibus_ucp_dis},
		{VBUS_ERRLO_DIS, sgm->cfg.vbus_low_dis},
        {VBUS_ERRHI_DIS, sgm->cfg.vbus_hi_dis},

		{BAT_OVP, sgm->cfg.vbat_ovp},
        {BAT_OCP, sgm->cfg.ibat_ocp},
        //{BUS_UCP, sgm->cfg.bus_ucp},
        {BUS_OVP, sgm->cfg.vbus_ovp},
		{BUS_OCP, sgm->cfg.ibus_ocp},
		{VAC_OVP, sgm->cfg.vac_ovp},
        //{VOUT_OVP, sgm->cfg.vout_ovp},
        {FSW_SET, sgm->cfg.fsw_set},
        {WDT_TIMER, sgm->cfg.wd_timeout},
        {OP_MODE, sgm->cfg.operation_mode},
 		{IBAT_RSNS, sgm->cfg.rsns},
    };

    ret = sgm41608_reg_reset(sgm);
    if (ret < 0) {
        SGM_INFO("Failed to reset registers(%d)\n", ret);
    }
    msleep(10);

    for (i = 0; i < ARRAY_SIZE(props); i++) {
        ret = sgm41608_field_write(sgm, props[i].field_id, props[i].conv_data);
    }

    sgm41608_field_write(sgm, TSBAT_FLT_DIS, 1);

    sgm41608_enable_adc(sgm, true);
    sgm41608_dump_reg(sgm);

    return ret;
}


/*********************mtk charger interface start**********************************/
#ifdef CONFIG_MTK_CLASS
static inline int to_sgm41608_adc(enum adc_channel chan)
{
	switch (chan) {
	case ADC_CHANNEL_VBUS:
		return ADC_VBUS;
	case ADC_CHANNEL_VBAT:
		return ADC_VBAT;
	case ADC_CHANNEL_IBUS:
		return ADC_IBUS;
	case ADC_CHANNEL_IBAT:
		return ADC_IBAT;
	case ADC_CHANNEL_TEMP_JC:
		return ADC_TDIE;
	default:
		break;
	}
	return ADC_MAX_NUM;
}


static int mtk_sgm41608_is_chg_enabled(struct charger_device *chg_dev, bool *en)
{
    struct sgm41608_chip *sgm = charger_get_data(chg_dev);
    int ret;

    ret = sgm41608_check_charge_enabled(sgm, en);

    return ret;
}


static int mtk_sgm41608_enable_chg(struct charger_device *chg_dev, bool en)
{
    struct sgm41608_chip *sgm = charger_get_data(chg_dev);
    int ret;

    ret = sgm41608_enable_charge(sgm, en);

    return ret;
}
static int mtk_sgm41608_set_mode(struct charger_device *chg_dev, u32 mode)
{
    struct sgm41608_chip *sgm = charger_get_data(chg_dev);
    int val,ret;

	ret = sgm41608_field_write(sgm, OP_MODE, mode);
	sgm->cfg.operation_mode = mode;

    ret = sgm41608_field_read(sgm, OP_MODE, &val);

    SGM_INFO("set op mode = %#x.\n",val);

    return ret;
}

static int mtk_sgm41608_set_vbusovp(struct charger_device *chg_dev, u32 uV)
{
    struct sgm41608_chip *sgm = charger_get_data(chg_dev);

    return sgm41608_set_busovp_th(sgm, uV / 1000);
}

static int mtk_sgm41608_set_ibusocp(struct charger_device *chg_dev, u32 uA)
{
    struct sgm41608_chip *sgm = charger_get_data(chg_dev);

    return sgm41608_set_busocp_th(sgm, uA / 1000);
}

static int mtk_sgm41608_set_vbatovp(struct charger_device *chg_dev, u32 uV)
{
    struct sgm41608_chip *sgm = charger_get_data(chg_dev);
    int ret;

    ret = sgm41608_set_batovp_th(sgm, uV / 1000);
    if (ret < 0)
        return ret;

    return ret;
}

static int mtk_sgm41608_set_ibatocp(struct charger_device *chg_dev, u32 uA)
{
    struct sgm41608_chip *sgm = charger_get_data(chg_dev);
    int ret;

    ret = sgm41608_set_batocp_th(sgm, uA / 1000);
    if (ret < 0)
        return ret;

    return ret;
}


static int mtk_sgm41608_get_adc(struct charger_device *chg_dev, enum adc_channel chan,
			  int *min, int *max)
{
    struct sgm41608_chip *sgm = charger_get_data(chg_dev);

    sgm41608_get_adc_data(sgm, to_sgm41608_adc(chan), max);

    if(chan != ADC_CHANNEL_TEMP_JC)
        *max = *max * 1000;

    if (min != max)
		*min = *max;
    return 0;
}

static int mtk_sgm41608_get_adc_accuracy(struct charger_device *chg_dev,
				   enum adc_channel chan, int *min, int *max)
{
    return 0;
}

static int mtk_sgm41608_is_vbuslowerr(struct charger_device *chg_dev, bool *err)
{
    struct sgm41608_chip *sgm = charger_get_data(chg_dev);

    return sgm41608_is_vbuslowerr(sgm, err);
}

static int mtk_sgm41608_set_vbatovp_alarm(struct charger_device *chg_dev, u32 uV)
{
    return 0;
}

static int mtk_sgm41608_reset_vbatovp_alarm(struct charger_device *chg_dev)
{
    return 0;
}

static int mtk_sgm41608_set_vbusovp_alarm(struct charger_device *chg_dev, u32 uV)
{
    return 0;
}

static int mtk_sgm41608_reset_vbusovp_alarm(struct charger_device *chg_dev)
{
    return 0;
}

static int mtk_sgm41608_init_chip(struct charger_device *chg_dev)
{
    struct sgm41608_chip *sgm = charger_get_data(chg_dev);

    return sgm41608_init_device(sgm);
}

static const struct charger_ops sgm41608_chg_ops = {
	 .enable = mtk_sgm41608_enable_chg,
	 .is_enabled = mtk_sgm41608_is_chg_enabled,
	 .get_adc = mtk_sgm41608_get_adc,
     .get_adc_accuracy = mtk_sgm41608_get_adc_accuracy,
	 .set_vbusovp = mtk_sgm41608_set_vbusovp,
	 .set_ibusocp = mtk_sgm41608_set_ibusocp,
	 .set_vbatovp = mtk_sgm41608_set_vbatovp,
	 .set_ibatocp = mtk_sgm41608_set_ibatocp,
	 .init_chip = mtk_sgm41608_init_chip,
     .is_vbuslowerr = mtk_sgm41608_is_vbuslowerr,
	 .set_vbatovp_alarm = mtk_sgm41608_set_vbatovp_alarm,
	 .reset_vbatovp_alarm = mtk_sgm41608_reset_vbatovp_alarm,
	 .set_vbusovp_alarm = mtk_sgm41608_set_vbusovp_alarm,
	 .reset_vbusovp_alarm = mtk_sgm41608_reset_vbusovp_alarm,
	 .set_cp_mode = mtk_sgm41608_set_mode,
};
#endif /*CONFIG_MTK_CLASS*/
/********************mtk charger interface end*************************************************/

#ifdef CONFIG_SGM_DVCHG_CLASS
static int sc_sgm41608_set_enable(struct dvchg_dev *charger_pump, bool enable)
{
    struct sgm41608_chip *sgm = dvchg_get_private(charger_pump);

    return sgm41608_enable_charge(sgm,enable);
}

static int sc_sgm41608_get_is_enable(struct dvchg_dev *charger_pump, bool *enable)
{
    struct sgm41608_chip *sgm = dvchg_get_private(charger_pump);

    return sgm41608_check_charge_enabled(sgm, enable);
}

static int sc_sgm41608_get_status(struct dvchg_dev *charger_pump, uint32_t *status)
{
    struct sgm41608_chip *sgm = dvchg_get_private(charger_pump);

    return sgm41608_get_status(sgm, status);
}

static int sc_sgm41608_get_adc_value(struct dvchg_dev *charger_pump, enum sc_adc_channel ch, int *value)
{
    struct sgm41608_chip *sgm = dvchg_get_private(charger_pump);

    return sgm41608_get_adc_data(sgm, ch, value);
}

static struct dvchg_ops sc_sgm41608_dvchg_ops = {
    .set_enable = sc_sgm41608_set_enable,
    .get_status = sc_sgm41608_get_status,
    .get_is_enable = sc_sgm41608_get_is_enable,
    .get_adc_value = sc_sgm41608_get_adc_value,
};
#endif /*CONFIG_SGM_DVCHG_CLASS*/

/********************creat devices note start*************************************************/
static ssize_t sgm41608_show_registers(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    struct sgm41608_chip *sgm = dev_get_drvdata(dev);
    u8 addr;
    int val;
    u8 tmpbuf[300];
    int len;
    int idx = 0;
    int ret;

    idx = snprintf(buf, PAGE_SIZE, "%s:\n", "sgm41608");
    for (addr = 0x0; addr <= SGM41608_REGMAX; addr++) {
        ret = regmap_read(sgm->regmap, addr, &val);
        if (ret == 0) {
            len = snprintf(tmpbuf, PAGE_SIZE - idx,
                    "Reg[%.2X] = 0x%.2x\n", addr, val);
            memcpy(&buf[idx], tmpbuf, len);
            idx += len;
        }
    }

    return idx;
}

static ssize_t sgm41608_store_register(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct sgm41608_chip *sgm = dev_get_drvdata(dev);
    int ret;
    unsigned int reg;
    unsigned int val;

    ret = sscanf(buf, "%x %x", &reg, &val);
    if (ret == 2 && reg <= SGM41608_REGMAX)
        regmap_write(sgm->regmap, (unsigned char)reg, (unsigned char)val);

    return count;
}

static DEVICE_ATTR(registers, 0660, sgm41608_show_registers, sgm41608_store_register);

static void sgm41608_create_device_node(struct device *dev)
{
    device_create_file(dev, &dev_attr_registers);
}
/********************creat devices note end*************************************************/

/*
* interrupt does nothing, just info event chagne, other module could get info
* through power supply interface
*/
#ifdef CONFIG_MTK_CLASS
static inline int status_reg_to_charger(enum sgm41608_notify notify)
{
	switch (notify) {
    case SGM41608_NOTIFY_IBUSOCP:
		return CHARGER_DEV_NOTIFY_IBUS_OCP;
    case SGM41608_NOTIFY_VBUSOVP:
		return CHARGER_DEV_NOTIFY_VBUS_OVP;
    case SGM41608_NOTIFY_IBATOCP:
		return CHARGER_DEV_NOTIFY_IBAT_OCP;
    case SGM41608_NOTIFY_VOUTOVP:
		return CHARGER_DEV_NOTIFY_BAT_OVP;
	default:
        return -EINVAL;
		break;
	}
	return -EINVAL;
}
#endif /*CONFIG_MTK_CLASS*/
static void sgm41608_check_fault_status(struct sgm41608_chip *sgm)
{
    int ret;
    u8 flag = 0;
    int i,j;

#ifdef CONFIG_MTK_CLASS
    int noti;
#endif /*CONFIG_MTK_CLASS*/

    for (i = 0;i < ARRAY_SIZE(cp_intr_flag);i++) {
        ret = sgm41608_read_block(sgm, cp_intr_flag[i].reg, &flag, 1);
        for (j=0; j <  cp_intr_flag[i].len; j++) {
            if (flag & cp_intr_flag[i].bit[j].mask) {
                SGM_INFO("trigger: %s\n", cp_intr_flag[i].bit[j].name);
#ifdef CONFIG_MTK_CLASS
                noti = status_reg_to_charger(cp_intr_flag[i].bit[j].notify);
                if(noti >= 0 && cp_intr_flag[i].reg == 0x17 && j!= 5) { // notify irq but ibus ucp
                    charger_dev_notify(sgm->chg_dev, noti);
                }
#endif /*CONFIG_MTK_CLASS*/
            }
        }
    }
}
__maybe_unused static void sgm41608_get_stat_register(struct sgm41608_chip *sgm)
{
    int ret;
    u8 stat[4] = {0};

	ret = sgm41608_read_block(sgm, SGM41608_STAT1_REG, stat, 4);
	if (!ret){
		SGM_INFO("stat1 = %#x,stat2 = %#x,stat3 = %#x,stat4 = %#x.\n",stat[0],stat[1],stat[2],stat[3]);
	} else {
		SGM_INFO("get error.\n");
	}
}

static irqreturn_t sgm41608_irq_handler(int irq, void *data)
{
    struct sgm41608_chip *sgm = data;
	int result = 0;

    SGM_INFO("INT OCCURED\n");

    sgm41608_field_read(sgm, EN_OTG, &result);
    if (sgm->otg_tx_mode && !result) {
        sgm41608_field_write(sgm, EN_OTG, true);
        /*HS07 code for HS07-4379 by yexuedong at 20250610 start*/
        sgm41608_field_write(sgm, ACDRV_MANUAL_EN, true);
        sgm41608_field_write(sgm, ACDRV_EN, true);
        /*HS07 code for HS07-4379 by yexuedong at 20250610 end*/
        SGM_INFO("set irq otg mode = 1\n");
    }

    sgm41608_check_fault_status(sgm);

    if (!sgm41608_get_wdt_status(sgm)) {
        sgm41608_disable_watch_dog(sgm, true);
    }

    return IRQ_HANDLED;
}

static int sgm41608_register_interrupt(struct sgm41608_chip *sgm)
{
    int ret;

    if (gpio_is_valid(sgm->irq_gpio)) {
        ret = gpio_request_one(sgm->irq_gpio, GPIOF_DIR_IN,"sgm41608_irq");
        if (ret) {
            SGM_INFO("failed to request sgm41608_irq\n");
            return -EINVAL;
        }
        sgm->irq = gpio_to_irq(sgm->irq_gpio);
        if (sgm->irq < 0) {
            SGM_INFO("sgm41608 failed to gpio_to_irq\n");
            return -EINVAL;
        }
    } else {
        SGM_INFO("sgm41608 irq gpio not provided\n");
        return -EINVAL;
    }

    if (sgm->irq) {
        ret = devm_request_threaded_irq(&sgm->client->dev, sgm->irq,
                NULL, sgm41608_irq_handler,
                IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                sgm41608_irq_name[sgm->mode], sgm);

        if (ret < 0) {
            SGM_INFO("request irq for irq=%d failed, ret =%d\n", sgm->irq, ret);
            return ret;
        }
        enable_irq_wake(sgm->irq);
    }
	SGM_INFO("irq register success.\n");
    return ret;
}
static int sgm41608_set_present(struct sgm41608_chip *chip, bool present)
{
    chip->usb_present = present;

    if (present)
    sgm41608_init_device(chip);
    return 0;
}
/********************interrupte end*************************************************/
static void sgm41608_set_otg_txmode(struct sgm41608_chip *sgm, int enable)
{
    sgm->otg_tx_mode = enable;
    if (!enable) {
        sgm41608_field_write(sgm, EN_OTG, false);
    }
}
/******************External interface************/
int sgm41608_usbpd_get_present(int *value)
{
    if (g_sgm41608 != NULL) {
        *value = g_sgm41608->usb_present;
        return true;
    } else {
        return false;
    }
}

int sgm41608_usbpd_set_present(int *value)
{
    if (g_sgm41608 != NULL) {
        sgm41608_set_present(g_sgm41608, !!(*value));
        return true;
    } else {
        return false;
    }
}

int sgm41608_usbpd_get_charging_enabled(bool *value)
{
    int ret = 0;
    bool result = false;

    if (g_sgm41608 != NULL) {
        ret = sgm41608_check_charge_enabled(g_sgm41608, &result);
        if (!ret) {
            g_sgm41608->charge_enabled = result;
        }

        *value = g_sgm41608->charge_enabled;
        return true;
    } else {
        return false;
    }
}

int sgm41608_usbpd_set_charging_enabled(bool *value)
{
    if (g_sgm41608 != NULL) {
        sgm41608_enable_charge(g_sgm41608, !!(*value));
        sgm41608_check_charge_enabled(g_sgm41608, &g_sgm41608->charge_enabled);
        gxy_bat_set_cp_sts(*value);
        return true;
    } else {
        return false;
    }
}

int sgm41608_usbpd_get_vbus(int *value)
{
    int ret = 0;
    int result = 0;

    if (g_sgm41608 != NULL) {
        ret = sgm41608_get_adc_data(g_sgm41608, ADC_VBUS, &result);
        if (!ret) {
            g_sgm41608->vbus_volt = result;
        }

        *value = g_sgm41608->vbus_volt;
        return true;
    } else {
        return false;
    }
}

int sgm41608_usbpd_get_ibus(int *value)
{
    int ret = 0;
    int result = 0;

    if (g_sgm41608 != NULL) {
        ret = sgm41608_get_adc_data(g_sgm41608, ADC_IBUS, &result);
        if (!ret) {
            g_sgm41608->ibus_curr = result;
        }

        *value = g_sgm41608->ibus_curr;
        return true;
    } else {
        return false;
    }
}

int sgm41608_usbpd_get_vbat(int *value)
{
    int ret = 0;
    int result = 0;

    if (g_sgm41608 != NULL) {
        ret = sgm41608_get_adc_data(g_sgm41608, ADC_VBAT, &result);
        if (!ret) {
            g_sgm41608->vbat_volt = result;
        }

        *value = g_sgm41608->vbat_volt;
        return true;
    } else {
        return false;
    }
}

int sgm41608_usbpd_get_ibat(int *value)
{
    int ret = 0;
    int result = 0;

    if (g_sgm41608 != NULL) {
        ret = sgm41608_get_adc_data(g_sgm41608, ADC_IBAT, &result);
        if (!ret) {
            g_sgm41608->ibat_curr = result;
        }

        *value = g_sgm41608->ibat_curr;
        return true;
    } else {
        return false;
    }
}

int sgm41608_usbpd_get_temp(int *value)
{
    int ret = 0;
    int result = 0;

    if (g_sgm41608 != NULL) {
        ret = sgm41608_get_adc_data(g_sgm41608, ADC_TDIE, &result);
        if (!ret) {
            g_sgm41608->die_temp = result;
        }

        *value = g_sgm41608->die_temp;
        return true;
    } else {
        return false;
    }
}

int sgm41608_usbpd_set_otg_txmode(int *value)
{
    int enable = *value;

    if (g_sgm41608 != NULL) {
        sgm41608_set_otg_txmode(g_sgm41608, enable);
        return true;
    } else {
        return false;
    }
}

int sgm41608_usbpd_get_otg_txmode(int *value)
{
    if (g_sgm41608 != NULL) {
        *value = g_sgm41608->otg_tx_mode;
        return true;
    } else {
        return false;
    }
}

int sgm41608_usbpd_pm_set_shipmode_adc(int *value)
{
    int enable = *value;

    if (g_sgm41608 != NULL) {
        sgm41608_enable_adc(g_sgm41608, enable);
        return true;
    } else {
        return false;
    }
}

/************************psy start**************************************/
// static enum power_supply_property sgm41608_charger_props[] = {
//     POWER_SUPPLY_PROP_ONLINE,
//     POWER_SUPPLY_PROP_PRESENT,
//     POWER_SUPPLY_PROP_VOLTAGE_NOW,
//     POWER_SUPPLY_PROP_CURRENT_NOW,
//     POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
//     POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
//     POWER_SUPPLY_PROP_TEMP,
// };

// static int sgm41608_charger_get_property(struct power_supply *psy,
//                 enum power_supply_property psp,
//                 union power_supply_propval *val)
// {
//     struct sgm41608_chip *sgm = power_supply_get_drvdata(psy);
//     int result;
//     int ret;

//     switch (psp) {
//     case POWER_SUPPLY_PROP_ONLINE:
//         sgm41608_check_charge_enabled(sgm, &sgm->charge_enabled);
//         val->intval = sgm->charge_enabled;
//         break;
//     case POWER_SUPPLY_PROP_VOLTAGE_NOW:
//         ret = sgm41608_get_adc_data(sgm, ADC_VBUS, &result);
//         if (!ret)
//             sgm->vbus_volt = result;
//         val->intval = sgm->vbus_volt;
//         break;
//     case POWER_SUPPLY_PROP_CURRENT_NOW:
//         ret = sgm41608_get_adc_data(sgm, ADC_IBUS, &result);
//         if (!ret)
//             sgm->ibus_curr = result;
//         val->intval = sgm->ibus_curr;
//         break;
//     case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
//         ret = sgm41608_get_adc_data(sgm, ADC_VBAT, &result);
//         if (!ret)
//             sgm->vbat_volt = result;
//         val->intval = sgm->vbat_volt;
//         break;
//     case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
//         ret = sgm41608_get_adc_data(sgm, ADC_IBAT, &result);
//         if (!ret)
//             sgm->ibat_curr = result;
//         val->intval = sgm->ibat_curr;
//         break;
//     case POWER_SUPPLY_PROP_TEMP:
//         ret = sgm41608_get_adc_data(sgm, ADC_TDIE, &result);
//         if (!ret)
//             sgm->die_temp = result;
//         val->intval = sgm->die_temp;
//         break;
//     default:
//         return -EINVAL;
//     }

//     return 0;
// }

// static int sgm41608_charger_set_property(struct power_supply *psy,
//                     enum power_supply_property prop,
//                     const union power_supply_propval *val)
// {
//     struct sgm41608_chip *sgm = power_supply_get_drvdata(psy);

//     switch (prop) {
//     case POWER_SUPPLY_PROP_ONLINE:
//         sgm41608_enable_charge(sgm, val->intval);
//         SGM_INFO("POWER_SUPPLY_PROP_ONLINE: %s\n", val->intval ? "enable" : "disable");
//         break;
//     default:
//         return -EINVAL;
//     }

//     return 0;
// }

// static int sgm41608_charger_is_writeable(struct power_supply *psy,
//                     enum power_supply_property prop)
// {
//     switch (prop) {
// 	case POWER_SUPPLY_PROP_ONLINE:
// 		return true;
// 	default:
// 		return false;
// 	}
//     return 0;
// }

// static int sgm41608_psy_register(struct sgm41608_chip *sgm)
// {
//     sgm->psy_cfg.drv_data = sgm;
//     sgm->psy_cfg.of_node = sgm->dev->of_node;

//     sgm->psy_desc.name = sgm41608_psy_name[sgm->mode];

//     sgm->psy_desc.type = POWER_SUPPLY_TYPE_MAINS;
//     sgm->psy_desc.properties = sgm41608_charger_props;
//     sgm->psy_desc.num_properties = ARRAY_SIZE(sgm41608_charger_props);
//     sgm->psy_desc.get_property = sgm41608_charger_get_property;
//     sgm->psy_desc.set_property = sgm41608_charger_set_property;
//     sgm->psy_desc.property_is_writeable = sgm41608_charger_is_writeable;

//     sgm->psy = devm_power_supply_register(sgm->dev, 
//             &sgm->psy_desc, &sgm->psy_cfg);
//     if (IS_ERR(sgm->psy)) {
//         SGM_INFO("failed to register psy\n");
//         return PTR_ERR(sgm->psy);
//     }

//     SGM_INFO("%s power supply register successfully\n", sgm->psy_desc.name);

//     return 0;
// }


/************************psy end**************************************/

static int sgm41608_set_work_mode(struct sgm41608_chip *sgm, int mode)
{
    sgm->mode = mode;
    SGM_INFO("SGM41608 work mode is %s\n", sgm->mode == SGM41608_STANDALONG 
        ? "standalone" : (sgm->mode == SGM41608_SECONDARY ? "Secondary" : "Primary"));

    return 0;
}

static int sgm41608_parse_dt(struct sgm41608_chip *sgm, struct device *dev)
{
    struct device_node *np = dev->of_node;
    int i;
    int ret;
    struct {
        char *name;
        int *conv_data;
    } props[] = {
		{"sgm,sgm41608,wdt-dis", &(sgm->cfg.wdt_dis)},
        {"sgm,sgm41608,vbat-ovp-dis", &(sgm->cfg.vbat_ovp_dis)},
        {"sgm,sgm41608,ibat-ocp-dis", &(sgm->cfg.ibat_ocp_dis)},
        //{"sgm,sgm41608,ibus-ucp-dis", &(sgm->cfg.ibus_ucp_dis)},
        {"sgm,sgm41608,vbus-error-low-dis", &(sgm->cfg.vbus_low_dis)},
        {"sgm,sgm41608,vbus-error-high-dis", &(sgm->cfg.vbus_hi_dis)},
    
        {"sgm,sgm41608,vbat-ovp", &(sgm->cfg.vbat_ovp)},
        {"sgm,sgm41608,ibat-ocp", &(sgm->cfg.ibat_ocp)},
        //{"sgm,sgm41608,ibus-ucp", &(sgm->cfg.bus_ucp)},
        {"sgm,sgm41608,vbus-ovp", &(sgm->cfg.vbus_ovp)},
        {"sgm,sgm41608,ibus-ocp", &(sgm->cfg.ibus_ocp)},
        {"sgm,sgm41608,vac-ovp", &(sgm->cfg.vac_ovp)},
        //{"sgm,sgm41608,vout-ovp", &(sgm->cfg.vout_ovp)},
		{"sgm,sgm41608,fsw-set", &(sgm->cfg.fsw_set)},
        {"sgm,sgm41608,wd-timeout", &(sgm->cfg.wd_timeout)},
        {"sgm,sgm41608,op-mode", &(sgm->cfg.operation_mode)},
        {"sgm,sgm41608,rsns", &(sgm->cfg.rsns)},
    };

    /* initialize data for optional properties */
    for (i = 0; i < ARRAY_SIZE(props); i++) {
        ret = of_property_read_u32(np, props[i].name,
                        props[i].conv_data);
        if (ret < 0) {
            SGM_INFO("can not read %s \n", props[i].name);
            return ret;
        }
    }

    sgm->irq_gpio = of_get_named_gpio(np, "sgm41608,intr_gpio", 0);
    if (!gpio_is_valid(sgm->irq_gpio)) {
        SGM_INFO("fail to valid gpio : %d\n", sgm->irq_gpio);
        return -EINVAL;
    }

#ifdef CONFIG_MTK_CHARGER_V5P10
    if (of_property_read_string(np, "charger_name", &sgm->chg_dev_name) < 0) {
        sgm->chg_dev_name = "charger";
        SGM_INFO("sgm41608 no charger name\n");
    }
#elif defined(CONFIG_MTK_CHARGER_V4P19)
    if (of_property_read_string(np, "charger_name_v4_19", &sgm->chg_dev_name) < 0) {
        sgm->chg_dev_name = "charger";
        SGM_INFO("sgm41608 no charger name\n");
    }
#endif /*CONFIG_MTK_CHARGER_V4P19*/
	SGM_INFO("end.\n");
    return 0;
}

static struct of_device_id sgm41608_charger_match_table[] = {
    {   .compatible = "sgm,sgm41608-standalone",
        .data = &sgm41608_mode_data[SGM41608_STANDALONG], },
    {   .compatible = "sgm,sgm41608-master",
        .data = &sgm41608_mode_data[SGM41608_SECONDARY], },
    {   .compatible = "sgm,sgm41608-slave",
        .data = &sgm41608_mode_data[SGM41608_PRIMARY], },
    {},
};

static int sgm41608_charger_probe(struct i2c_client *client,
                    const struct i2c_device_id *id)
{
    struct sgm41608_chip *sgm;
    const struct of_device_id *match;
    struct device_node *node = client->dev.of_node;
    int ret, i;

	pr_err("sgm41608 driver version: %s\n", SGM41608_DRV_VERSION);

    sgm = devm_kzalloc(&client->dev, sizeof(struct sgm41608_chip), GFP_KERNEL);
    if (!sgm) {
        ret = -ENOMEM;
        goto err_kzalloc;
    }

    client->addr = 0x61;
    sgm->dev = &client->dev;
    sgm->client = client;

    sgm->regmap = devm_regmap_init_i2c(client,
                            &sgm41608_regmap_config);
    if (IS_ERR(sgm->regmap)) {
        SGM_INFO("Failed to initialize regmap\n");
        ret = PTR_ERR(sgm->regmap);
        goto err_regmap_init;
    }

    for (i = 0; i < ARRAY_SIZE(sgm41608_reg_fields); i++) {
        const struct reg_field *reg_fields = sgm41608_reg_fields;

        sgm->rmap_fields[i] =
            devm_regmap_field_alloc(sgm->dev,
                        sgm->regmap,
                        reg_fields[i]);
        if (IS_ERR(sgm->rmap_fields[i])) {
            SGM_INFO("cannot allocate regmap field.\n");
            ret = PTR_ERR(sgm->rmap_fields[i]);
            goto err_regmap_field;
        }
    }
    ret = sgm41608_parse_dt(sgm, &client->dev);
    if (ret < 0) {
        SGM_INFO("parse dt failed(%d)\n", ret);
        goto err_parse_dt;
    }
    ret = sgm41608_detect_device(sgm);
    if (ret < 0) {
        goto err_detect_dev;
    }

    i2c_set_clientdata(client, sgm);
    sgm41608_create_device_node(&(client->dev));

    match = of_match_node(sgm41608_charger_match_table, node);
    if (match == NULL) {
        SGM_INFO("device tree match not found!\n");
        goto err_match_node;
    }

    ret = sgm41608_set_work_mode(sgm, *(int *)match->data);
    if (ret) {
        SGM_INFO("Fail to set work mode!\n");
        goto err_set_mode;
    }

    ret = sgm41608_init_device(sgm);
    if (ret < 0) {
        SGM_INFO("init device failed(%d)\n", ret);
        goto err_init_device;
    }

   // ret = sgm41608_psy_register(sgm);
   // if (ret < 0) {
   //     SGM_INFO("psy register failed(%d)\n", ret);
   //     goto err_register_psy;
   // }

    ret = sgm41608_register_interrupt(sgm);
    if (ret < 0) {
        SGM_INFO("register irq fail(%d)\n", ret);
        goto err_register_irq;
    }

#ifdef CONFIG_MTK_CLASS
    sgm->chg_dev = charger_device_register(sgm->chg_dev_name,
					      &client->dev, sgm,
					      &sgm41608_chg_ops,
					      &sgm41608_chg_props);
	if (IS_ERR_OR_NULL(sgm->chg_dev)) {
		ret = PTR_ERR(sgm->chg_dev);
		SGM_INFO("Fail to register charger!\n");
        goto err_register_mtk_charger;
	}
#endif /*CONFIG_MTK_CLASS*/

#ifdef CONFIG_SGM_DVCHG_CLASS
    sgm->charger_pump = dvchg_register("sc_dvchg",
                             sgm->dev, &sc_sgm41608_dvchg_ops, sgm);
    if (IS_ERR_OR_NULL(sgm->charger_pump)) {
		ret = PTR_ERR(sgm->charger_pump);
		SGM_INFO("Fail to register charger!\n");
        goto err_register_sc_charger;
	}
#endif /* CONFIG_SGM_DVCHG_CLASS */

	g_is_sgm41608_prob_success = true;

    g_sgm41608 = sgm;
    g_gxy_cp_ops.get_present = sgm41608_usbpd_get_present;
    g_gxy_cp_ops.set_present = sgm41608_usbpd_set_present;
    g_gxy_cp_ops.set_charging_enabled = sgm41608_usbpd_set_charging_enabled;
    g_gxy_cp_ops.get_charging_enabled = sgm41608_usbpd_get_charging_enabled;
    g_gxy_cp_ops.get_vbus = sgm41608_usbpd_get_vbus;
    g_gxy_cp_ops.get_ibus = sgm41608_usbpd_get_ibus;
    g_gxy_cp_ops.get_vbat = sgm41608_usbpd_get_vbat;
    g_gxy_cp_ops.get_ibat = sgm41608_usbpd_get_ibat;
    g_gxy_cp_ops.get_temp = sgm41608_usbpd_get_temp;
    g_gxy_cp_ops.set_otg_txmode = sgm41608_usbpd_set_otg_txmode;
    g_gxy_cp_ops.get_otg_txmode = sgm41608_usbpd_get_otg_txmode;
    sgm->otg_tx_mode = false;
    g_gxy_cp_ops.set_shipmode_adc = sgm41608_usbpd_pm_set_shipmode_adc;

    gxy_bat_set_cpinfo(GXY_BAT_CP_INFO_SGM41608);

	SGM_INFO("probe success!\n");
    return 0;

err_register_irq:
#ifdef CONFIG_MTK_CLASS
err_register_mtk_charger:
#endif /*CONFIG_MTK_CLASS*/
#ifdef CONFIG_SGM_DVCHG_CLASS
err_register_sc_charger:
#endif /*CONFIG_SGM_DVCHG_CLASS*/
err_init_device:
    //power_supply_unregister(sgm->psy);
err_detect_dev:
err_match_node:
err_set_mode:
err_parse_dt:
err_regmap_init:
err_regmap_field:
    devm_kfree(&client->dev, sgm);
err_kzalloc:
    SGM_INFO("sgm41608 probe fail\n");
    return ret;
}


static int sgm41608_charger_remove(struct i2c_client *client)
{
    struct sgm41608_chip *sgm = i2c_get_clientdata(client);

	if (g_is_sgm41608_prob_success) {
        struct sgm41608_chip *sgm = i2c_get_clientdata(client);
        sgm41608_enable_adc(sgm, false);
    }
    //power_supply_unregister(sgm->psy);
    devm_kfree(&client->dev, sgm);
    return 0;
}
static void sgm41608_charger_shutdown(struct i2c_client *client)
{
    if (g_is_sgm41608_prob_success) {
        struct sgm41608_chip *sgm = i2c_get_clientdata(client);
        sgm41608_enable_adc(sgm, false);
    }
}

#ifdef CONFIG_PM_SLEEP
static int sgm41608_suspend(struct device *dev)
{
    SGM_INFO("Suspend successfully!\n");

	if (g_is_sgm41608_prob_success) {
        struct sgm41608_chip *sgm = dev_get_drvdata(dev);

        SGM_INFO("Suspend successfully!");
        if (device_may_wakeup(dev)) {
            enable_irq_wake(sgm->irq);
        }
        disable_irq(sgm->irq);

        if(!sgm41608_get_vbus_present_status(sgm)) {
            sgm41608_enable_adc(sgm, false);
        }

       // sgm41606S_disable_watch_dog(sgm, true);
    }
    //if (device_may_wakeup(dev))
    //    enable_irq_wake(sgm->irq);
    //disable_irq(sgm->irq);

    return 0;
}
static int sgm41608_resume(struct device *dev)
{
	if (g_is_sgm41608_prob_success) {
        struct sgm41608_chip *sgm = dev_get_drvdata(dev);
        SGM_INFO("Resume successfully!");
        if (device_may_wakeup(dev)) {
            disable_irq_wake(sgm->irq);
        }
        enable_irq(sgm->irq);
        sgm41608_enable_adc(sgm, true);
    }

    //struct sgm41608_chip *sgm = dev_get_drvdata(dev);

    //SGM_INFO("Resume successfully!\n");
    //if (device_may_wakeup(dev))
    //    disable_irq_wake(sgm->irq);
    //enable_irq(sgm->irq);

    return 0;
}

static const struct dev_pm_ops sgm41608_pm = {
    SET_SYSTEM_SLEEP_PM_OPS(sgm41608_suspend, sgm41608_resume)
};
#endif

static struct i2c_driver sgm41608_charger_driver = {
    .driver     = {
        .name   = "sgm41608-charger",
        .owner  = THIS_MODULE,
        .of_match_table = sgm41608_charger_match_table,
#ifdef CONFIG_PM_SLEEP
        .pm = &sgm41608_pm,
#endif
    },
    .probe      = sgm41608_charger_probe,
    .remove     = sgm41608_charger_remove,
    .shutdown    = sgm41608_charger_shutdown,
};

module_i2c_driver(sgm41608_charger_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SGM SGM41608 Driver");
MODULE_AUTHOR("mike_shi<mike_shi@sg-micro.com>");



