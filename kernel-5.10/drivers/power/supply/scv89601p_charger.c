/* HS07 code for SR-AL7761A-01-164 by lina at 20250314 start */
/*HS07 code for HS07-188 by xiongxiaoliang at 20250320 start*/
// SPDX-License-Identifier: GPL-2.0
/**
 * @file   scv89601p_charger.c
 * @author <boyu-wen@southchip>
 * @date   Feb 25 2025
 * @brief  Copyright (c) 20XX-2025 Southchip Semiconductor Technology(Shanghai) Co.,Ltd.
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
#define CONFIG_MTK_CLASS
#ifdef CONFIG_MTK_CLASS
#include "charger_class.h"
#include "mtk_charger.h"
#ifdef CONFIG_MTK_CHARGER_V4P19
#include "charger_type.h"
#include "mtk_charger_intf.h"
#endif /*CONFIG_MTK_CHARGER_V4P19*/
#endif /*CONFIG_MTK_CLASS*/
#if IS_ENABLED(CONFIG_CHARGER_CP_PPS)
#include "charge_pump/pd_policy_manager.h"
extern struct gxy_cp_info g_cp_charging_lmt;
#endif

#define SCV89601P_DRV_VERSION         "1.0"
#define SCV89601P_ERR(fmt, ...)       pr_err("SCV89601P_ERR:" fmt, ##__VA_ARGS__)
#define SCV89601P_INFO(fmt, ...)      pr_info("SCV89601P_INFO:" fmt, ##__VA_ARGS__)

#ifndef __maybe_unused
#define __maybe_unused
#endif

#define VINDPM_BEFORCE_BC12         0
#define VIMDPM_DEFAULT_IN_BC12      4800
#define DEFAULT_HIZ_CUT_TIME_EXPRIE 300UL

extern void gxy_bat_set_chginfo(enum gxy_bat_chg_info cinfo_data);
/*HS07 code for HS07-3330 by xiongxiaoliang at 20250522 start*/
#if !defined(CONFIG_ODM_CUSTOM_FACTORY_BUILD)
extern bool gxy_check_batt_protection_status(void);
#endif
/*HS07 code for HS07-3330 by xiongxiaoliang at 20250522 end*/
/*HS07 code for HS07-3571 by yexuedong at 20250530 start*/
#if IS_ENABLED(CONFIG_CUSTOM_PROJECT_HS07)
extern void usbpd_setvacovp(int enable);
#endif
/*HS07 code for HS07-3571 by yexuedong at 20250530 end*/

/* HS07 code for HS07-1480 by lina at 20250425 start */
#if IS_ENABLED(CONFIG_CUSTOM_USBIF)
struct sc_tag_bootmode {
	u32 size;
	u32 tag;
	u32 bootmode;
	u32 boottype;
};
#endif//CONFIG_CUSTOM_USBIF
/* HS07 code for HS07-1480 by lina at 20250425 end */

enum scv89601p_part_no {
    SC89601P_PN_NUM = 0x0C,
};

enum scv89601p_vbus_stat {
    VBUS_STAT_NO_INPUT = 0,
    VBUS_STAT_SDP,
    VBUS_STAT_CDP,
    VBUS_STAT_DCP,
    VBUS_STAT_HVDCP,
    VBUS_STAT_UNKOWN,
    VBUS_STAT_NONSTAND,
    VBUS_STAT_OTG,
};

enum scv89601p_chg_stat {
    CHG_STAT_NOT_CHARGE = 0,
    CHG_STAT_TRICKLE_CHARGE,
    CHG_STAT_PRE_CHARGE,
    CHG_STAT_FAST_CHARGE_CCMODE,
    CHG_STAT_FAST_CHARGE_CVMODE,
    CHG_STAT_CHARGE_DONE,
};

enum scv89601p_adc_channel {
    SCV89601P_ADC_IBUS,
    SCV89601P_ADC_VBUS,
    SCV89601P_ADC_VBAT,
    SCV89601P_ADC_IBAT,
    SCV89601P_ADC_TSBUS,
    SCV89601P_ADC_TDIE,
};

enum vindpm_track {
    SCV89601P_TRACK_DIS,
    SCV89601P_TRACK_200,
    SCV89601P_TRACK_250,
    SCV89601P_TRACK_300,
};

enum scv89601p_dpdm_drive {
    DPDM_HIZ,
    DPDM_20K_PULL_DOWN,
    DPDM_0P6,
    DPDM_2P0,
    DPDM_2P7,
    DPDM_3P3,
};

enum scv89601p_dpdm_stat {
    DPDM_0P325V,
    DPDM_1V,
    DPDM_1P35V,
    DPDM_2P2V,
    DPDM_3V,
    DPDM_0P425V,
};

enum attach_type {
    ATTACH_TYPE_NONE,
    ATTACH_TYPE_PWR_RDY,
    ATTACH_TYPE_TYPEC,
    ATTACH_TYPE_PD,
    ATTACH_TYPE_PD_SDP,
    ATTACH_TYPE_PD_DCP,
    ATTACH_TYPE_PD_NONSTD,
};

enum scv89601p_fields {
    /* house keeping register */
    F_VBUS_OVP_DIS, F_VBUS_OVP, /* reg04 */
    F_WD_TIMER_RST, F_WD_TIMEOUT, /* reg07 */
    F_REG_RST, F_VBUS_PD_30MA, F_PMID_PD_EN,
    F_VSYS_PD_EN, F_TSHUT_DIS, /* reg08 */
    F_ADC_DONE_STAT, F_REGN_OK_STAT, F_VBUS_UVLZ_STAT, F_VBUS_PRESENT_STAT, /* reg09 */
    F_ADC_DONE_FLAG, F_REGN_OK_FLAG, F_VBUS_UVLZ_FLAG, F_VBUS_PRESENT_FLAG, /* reg0A */
    F_PN, /* reg0B */
    F_WD_TIMEOUT_STAT, F_TSHUT_STAT, F_TSBAT_HOT_STAT,
    F_TSBAT_COLD_STAT, F_VBUS_OVP_STAT, /* reg0C */
    F_WD_TIMEOUT_FLAG, F_TSHUT_FLAG, F_TSBAT_HOT_FLAG,
    F_TSBAT_COLD_FLAG, F_VBUS_OVP_FLAG, /* reg0D */
    F_WD_TIMEOUT_MASK, F_TSHUT_MASK, F_TSBAT_HOT_MASK,
    F_TSBAT_COLD_MASK, F_VBUS_OVP_MASK, /* reg0E */
    F_ADC_EN, F_ADC_RATE, F_FORCE_REGN_BYPASS, /* reg0F */
    F_IBUS_ADC, /* reg12 */
    F_VBUS_ADC, /* reg14 */
    F_VBAT_ADC, /* reg1A */
    F_IBAT_ADC, /* reg1C */
    F_TSBAT_ADC, /* reg22 */
    F_TDIE_ADC, /* reg24 */
    F_BGOK_BLOCK_ANA_EN, /* reg2F */
    F_ADC_DONE_MASK, F_REGN_OK_MASK, F_VBUS_UVLZ_MASK, F_VBUS_PRESENT_MASK, /* reg5F */

    /* buck charger register */
    F_SYS_MIN, /* reg30 */
    F_VBAT_REG, /* reg31 */
    F_ICC, /* reg32 */
    F_VINDPM_TRACK, F_VINDPM_FUNC_DIS, F_VINDPM, /* reg33 */
    F_IINDPM_FUNC_DIS, F_IINDPM_UPDATE_DIS, F_IINDPM, /* reg34 */
    F_VBAT_PRECHG_THRE, F_IPRECHG, /* reg36 */
    F_TERM_EN, F_ITERM, /* reg37 */
    F_JEITA_OTG_HOT_TEMP, F_JEITA_OTG_COLD_TEMP, F_RECHG_DIS, F_VRECHG, /* reg38 */
    F_VBOOST, F_IBOOST_LIMIT, /* reg39 */
    F_VBOOST_HIGHEST_BIT, F_TSBAT_JEITA_EN, F_IBAT_OCP_DIS,
    F_VPMID_OVP_OTG_DIS, F_VBAT_OVP_BUCK_DIS, /* reg3A */
    F_BATFET_DIS_DELAY_STAT, F_BATFET_RST_EN, F_BATFET_DLY, F_BATFET_DIS, /* reg3B */
    F_HIZ_EN, F_DIS_BUCKCHG_PATH, F_SLEEP_FOR_OTG, F_QB_EN, F_BOOST_EN, F_CHG_EN, /* reg3C */
    F_IBAT_OCP_DG, F_IBAT_OCP, F_OTG_DPDM_DIS, F_VSYSOVP_DIS, F_VSYSOCP_THRE, /* reg3D */
    F_BATFET_CTRL_WVBUS, F_NTC_CHG_COOL_VSET, F_JEITA_ISET_WARM,
    F_JEITA_ISET_COOL, F_JEITA_VSET_WARM, /* reg3E */
    F_TMR2X_EN, F_CHG_TIMER_EN, F_CHG_TIMER,
    F_TDIE_REG_DIS, F_TDIE_REG, F_PFM_DIS, /* reg3F */
    F_VBAT_LOW_OTG, F_BOOST_FREQ, F_BUCK_FREQ, F_BAT_LOAD_EN, /* reg40 */
    F_VSYS_SHORT_STAT, F_VSLEEP_BUCK_STAT, F_VBAT_DPL_STAT,
    F_VBAT_LOW_BOOST_STAT, F_VBUS_GOOD_STAT, /* reg41 */
    F_CHG_STAT, F_BOOST_OK_STAT, F_VSYSMIN_REG_STAT, F_QB_ON_STAT, /* reg42 */
    F_TSBAT_COOL_STAT, F_TDIE_REG_STAT, F_TSBAT_WARM_STAT,
    F_IINDPM_STAT, F_VINDPM_STAT, /* reg43 */
    F_VSYS_SHORT_FLAG, F_VSLEEP_BUCK_FLAG, F_VBAT_DPL_FLAG,
    F_VBAT_LOW_BOOST_FLAG, F_VBUS_GOOD_FLAG, /* reg44 */
    F_CHG_FLAG, F_BOOST_OK_FLAG, F_VSYSMIN_REG_FLAG, F_QB_EN_FLAG, /* reg45 */
    F_TSBAT_COOL_FLAG, F_TDIE_REG_FLAG, F_TSBAT_WARM_FLAG,
    F_IINDPM_FLAG, F_VINDPM_FLAG, /* reg46 */
    F_VSYS_SHORT_MASK, F_VSLEEP_BUCK_MASK, F_VBAT_DPL_MASK,
    F_VBAT_LOW_BOOST_MASK, F_VBUS_GOOD_MASK, /* reg47 */
    F_CHG_MASK, F_BOOST_OK_MASK, F_VSYSMIN_REG_MASK, F_QB_EN_MASK, /* reg48 */
    F_TSBAT_COOL_MASK, F_TDIE_REG_MASK, F_TSBAT_WARM_MASK,
    F_IINDPM_MASK, F_VINDPM_MASK, /* reg49 */
    F_OTG_CONV_OCP_HICCUP_DIS, F_CONV_OCP_THRE, F_SET_SW_SLEW_RATE_OTG,
    F_SET_SW_SLEW_RATE_FWD0, F_OTG_SOFT_START_FAST, /* reg4D */
    F_TSHIPMODE_EXIT_SET, F_EOC_DPM_CTRL, F_TRICKLE_CURRENT, /* reg4E */
    F_CONV_OCP_STAT, F_VSYS_OVP_STAT, F_IBAT_OCP_STAT, F_VBAT_OVP_BUCK_STAT, /* reg50 */
    F_OTG_VBUS_STAT, F_CHG_TIMEOUT_STAT, F_VPMID_OVP_OTG_STAT, /* reg51 */
    F_CONV_OCP_FLAG, F_VSYS_OVP_FLAG, F_IBAT_OCP_FLAG, F_VBAT_OVP_BUCK_FLAG, /* reg52 */
    F_OTG_VBUS_FLAG, F_CHG_TIMEOUT_FLAG, F_VPMID_OVP_OTG_FLAG, /* reg53 */
    F_CONV_OCP_MASK, F_VSYS_OVP_MASK, F_IBAT_OCP_MASK, F_VBAT_OVP_BUCK_MASK, /* reg54 */
    F_OTG_VBUS_MASK, F_CHG_TIMEOUT_MASK, F_VPMID_OVP_OTG_MASK, /* reg55 */
    F_JEITA_WARM_TEMP, F_OTG_PMID_UV, /* reg56 */
    F_EOC_STAT, F_BOOST_FAULT_STAT, /* reg57 */
    F_EOC_FLAG, F_BOOST_FAULT_FLAG, /* reg58 */
    F_EOC_MASK, F_BOOST_FAULT_MASK, /* reg59 */
    F_JEITA_COOL_TEMP, F_OTG_PMID_UV_DG, /* reg5A */
    F_JEITA_HOT_TEMP, F_OTG_OVP, /* reg5B */
    F_JEITA_COLD_TEMP, F_VBAT_UVLZ_DPLZ_FALL_DG, /* reg5C */
    F_ICHG_FOLD, F_SET_SW_SLEW_RATE_FWD1, /* reg5D */

    /*DPDM register*/
    F_FORCE_INDET_EN, F_AUTO_INDET_EN, F_HVDCP_EN,
    F_IN_DPDM_DET, F_DPDM_INDET_SUPPORT, /* reg90 */
    F_DP_DRIVE, F_DM_DRIVE, F_BC12_VDAT_REF_SET, F_BC12_DPDM_SINK_CAP, /* reg91 */
    F_VBUS_STAT, F_INPUT_DET_DONE_FLAG, F_DP_OVP_FLAG, F_DM_OVP_FLAG, /* reg94 */
    F_INPUT_DET_DONE_MASK, F_DP_OVP_MASK, F_DM_OVP_MASK, /* reg95 */
    F_DP_OVP_STAT, F_DP_IN5, F_DP_IN4, F_DP_IN3, F_DP_IN2, F_DP_IN1, F_DP_IN0, /* reg98 */
    F_DM_OVP_STAT, F_DM_IN5, F_DM_IN4, F_DM_IN3, F_DM_IN2, F_DM_IN1, F_DM_IN0, /* reg99 */
    F_DPDM_OVP_EN, F_DPDM_POLLING_EN, F_NONSTD_PRI_HIGH, F_DPDM_VTH_REF, /* reg9A */
    F_DM_500K_PD_EN, F_DP_500_PD_EN, F_DM_SINK_EN, F_DP_SINK_EN,
    F_DP_SRC_10UA, F_VBUS_PLUGIN_RST_DPDM_DIS, F_VBUS_PLUGOUT_RST_DPDM_DIS, F_DPDM_EN, /* reg9D */
    F_DPDM_STAT, F_DM_SRC_10UA, F_NONSTD_FLAG, /* reg9E */

    F_MAX_FIELDS,
};

enum scv89601p_reg_range {
    SCV89601P_IINDPM,
    SCV89601P_ICHG,
    SCV89601P_IBOOST,
    SCV89601P_VBOOST,
    SCV89601P_VBAT_REG,
    SCV89601P_VINDPM,
    SCV89601P_ITERM,
    SCV89601P_IPRECHG,
    SCV89601P_IBUS,
    SCV89601P_VBUS,
    SCV89601P_VBAT,
    SCV89601P_IBAT,
    SCV89601P_TSBAT,
    SCV89601P_TDIE,
};

struct reg_range {
    uint32_t min;
    uint32_t max;
    uint32_t step;
    uint32_t offset;
    const uint32_t *table;
    uint16_t num_table;
    bool round_up;
};

struct scv89601p_param_e {
    const char *chg_name;
    int pfm_dis;
    int vsys_min;
    int vbat_low_otg;
    int pre_chg_curr;
    int itrickle;
    int term_chg_curr;
    int vbat_cv;
    int vrechg_volt;
    int term_en;
    int hvdcp_en;
    int wdt_timer;
    int en_chg_saft_timer;
    int charge_timer;
    int vac_ovp;
    int iboost;
    int vboost;
    int vindpm_track;
    int vindpm_int_mask;
    int iindpm_int_mask;
    int auto_dpdm_en;
    int iindpm_update_dis;
};

/* These default values will be applied if there's no property in dts */
static struct scv89601p_param_e scv89601p_default_cfg = {
    .chg_name = "primary_chg",
    .pfm_dis = 0,
    .vsys_min = 5,
    .vbat_low_otg = 0,
    .pre_chg_curr = 1,
    .term_chg_curr = 1,
    .vbat_cv = 70,
    .vrechg_volt = 0,
    .term_en = 1,
    .hvdcp_en = 0,
    .wdt_timer = 0,
    .en_chg_saft_timer = 1,
    .charge_timer = 0,
    .vac_ovp = 0,
    .iboost = 1,
    .vboost = 3,
    .vindpm_track = 0,
    .vindpm_int_mask = 1,
    .iindpm_int_mask = 1,
    .auto_dpdm_en = 0,
    .iindpm_update_dis = 0,
};

struct scv89601p_chip {
    struct device *dev;
    struct i2c_client *client;
#ifdef CONFIG_MTK_CLASS
    struct charger_device *chg_dev;
    struct regulator_dev *otg_rdev;
#ifdef CONFIG_MTK_CHARGER_V4P19
    enum charger_type adp_type;
    struct power_supply *chg_psy;
    struct delayed_work psy_dwork;
#endif /*CONFIG_MTK_CHARGER_V4P19*/
#endif /*CONFIG_MTK_CLASS */

    int chg_type;
    int psy_usb_type;

    int irq_gpio;
    int irq;

    struct delayed_work force_detect_dwork;
    int force_detect_count;
    int dpdm_done_flag;

    int power_good;
    int vbus_good;
    uint8_t dev_id;
    /*Tab A9 code for AX6739A-516 | OT11BSP-42 by qiaodan at 20230613 start*/
    int typec_attached;
    /*Tab A9 code for AX6739A-516 | OT11BSP-42 by qiaodan at 20230613 end*/
    bool chg_config;

    /* HS07 code for SR-AL7761A-01-85 by lina at 20250317 start */
    bool shipmode_wr_value;
    /* HS07 code for SR-AL7761A-01-85 by lina at 20250317 end */
    bool bc12_detect;
    struct mutex bc_detect_lock;
    struct delayed_work bc12_timeout_dwork;
    bool bc12_recovery;
    /*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 start*/
    struct delayed_work hvdcp_done_work;
    bool hvdcp_done;
    /*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 end*/
    /* HS07 code for SR-AL7761A-01-155|HS07-184 by lina at 20250319 start */
    bool is_recharging;
    bool force_dpdm;
    /* HS07 code for SR-AL7761A-01-155|HS07-184 by lina at 20250319 end */

    struct power_supply_desc psy_desc;
    struct scv89601p_param_e *cfg;
    struct power_supply *psy;

    int term_curr;
    int icc_curr;

    struct delayed_work hiz_cut_dwork;
    bool hiz_cut_flag;

    int vindpm_value;
    /* HS07 code for HS07-1480 by lina at 20250425 start */
    #if IS_ENABLED(CONFIG_CUSTOM_USBIF)
    struct delayed_work set_input_volt_lim_work;
    bool is_first_plugin;
    u32 bootmode;
    /*HS07 code for HS07-752 by xiongxiaoliang at 20250514 start*/
    bool pd_set_vindpm_sts;
    /*HS07 code for HS07-752 by xiongxiaoliang at 20250514 end*/
    #endif//CONFIG_CUSTOM_USBIF
    /* HS07 code for HS07-1480 by lina at 20250425 end */
};

static struct charger_device *s_chg_dev_otg;

static const uint32_t scv89601p_iboost[] = {
    500, 900, 1300, 1500, 2100, 2500, 2900, 3000
};

static const uint32_t scv89601p_vindpm[] = {
    4000, 4100, 4200, 4300, 4400, 4500, 4600, 4700,
    4800, 7600, 8200, 8400, 8600, 10000, 10500, 10700
};

#define SC8962x_CHG_RANGE(_min, _max, _step, _offset, _ru) \
{ \
    .min = _min, \
    .max = _max, \
    .step = _step, \
    .offset = _offset, \
    .round_up = _ru, \
}

#define SC8962x_CHG_RANGE_T(_table, _ru) \
    { .table = _table, .num_table = ARRAY_SIZE(_table), .round_up = _ru, }

#define DEVIDE_10(x) (x / 10)
#define DEVIDE_1000(x) (x / 1000)

static struct reg_range scv89601p_reg_range_ary[] = {
    [SCV89601P_IINDPM]    = SC8962x_CHG_RANGE(100, 3200, 100, 100, false),
    [SCV89601P_ICHG]      = SC8962x_CHG_RANGE(0, 3780, 60, 0, false),
    [SCV89601P_IBOOST]    = SC8962x_CHG_RANGE_T(scv89601p_iboost, false),
    [SCV89601P_VBOOST]    = SC8962x_CHG_RANGE(3900, 7000, 100, 3900, false),
    [SCV89601P_VBAT_REG]  = SC8962x_CHG_RANGE(3530, 4800, 10, 3530, false),
    [SCV89601P_VINDPM]    = SC8962x_CHG_RANGE_T(scv89601p_vindpm, false),
    [SCV89601P_ITERM]     = SC8962x_CHG_RANGE(60, 960, 60, 60, false),
    [SCV89601P_IPRECHG]   = SC8962x_CHG_RANGE(60, 1920, 60, 60, false),
    [SCV89601P_IBUS]      = SC8962x_CHG_RANGE(0, 2907, DEVIDE_10(114), 0, false),
    [SCV89601P_VBUS]      = SC8962x_CHG_RANGE(0, 15360, 60, 0, false),
    [SCV89601P_VBAT]      = SC8962x_CHG_RANGE(0, 4590, 18, 0, false),
    [SCV89601P_IBAT]      = SC8962x_CHG_RANGE(0, 3070, 12, 0, false),
    [SCV89601P_TSBAT]     = SC8962x_CHG_RANGE(25, 85, DEVIDE_1000(195), 25, false),
    [SCV89601P_TDIE]      = SC8962x_CHG_RANGE(-40, 150, DEVIDE_10(35), 0, false),
};

/* Optimized loop response */
/*#define SCV89601P_SPECIAL_CODE_REG_1 0x91
#define SCV89601P_SPECIAL_CODE_REG_2 0x92
#define SCV89601P_SPECIAL_CODE_VAL_1 0x9D
#define SCV89601P_SPECIAL_CODE_VAL_2 0x40*/

/*#define TREG_JUDGE_CURR 500
#define TREG_CURR_ADJ_STEP 200
#define TREG_CURR_ADJ_TIME_STEP 7*/

struct sc_reg_field {
    uint32_t reg;
    uint32_t lsb;
    uint32_t msb;
    bool force_write;
};

#define SC_REG_FIELD(_reg, _lsb, _msb) {           \
                    .reg = _reg,                \
                    .lsb = _lsb,                \
                    .msb = _msb,                \
                    }

#define SC_REG_FIELD_FORCE_WRITE(_reg, _lsb, _msb) {           \
                    .reg = _reg,                \
                    .lsb = _lsb,                \
                    .msb = _msb,                \
                    .force_write = true,        \
                    }

//REGISTER
static const struct sc_reg_field scv89601p_reg_fields[] = {
    /* house keeping register */
    /*reg04*/
    [F_VBUS_OVP_DIS] = SC_REG_FIELD(0x04, 2, 2),
    [F_VBUS_OVP] = SC_REG_FIELD(0x04, 0, 1),
    /*reg07*/
    [F_WD_TIMER_RST] = SC_REG_FIELD(0x07, 3, 3),
    [F_WD_TIMEOUT] = SC_REG_FIELD(0x07, 0, 1),
    /*reg08*/
    [F_REG_RST] = SC_REG_FIELD(0x08, 7, 7),
    [F_VBUS_PD_30MA] = SC_REG_FIELD(0x08, 6, 6),
    [F_PMID_PD_EN] = SC_REG_FIELD(0x08, 4, 4),
    [F_VSYS_PD_EN] = SC_REG_FIELD(0x08, 3, 3),
    [F_TSHUT_DIS] = SC_REG_FIELD(0x08, 1, 1),
    /*reg09*/
    [F_ADC_DONE_STAT] = SC_REG_FIELD(0x09, 4, 4),
    [F_REGN_OK_STAT] = SC_REG_FIELD(0x09, 3, 3),
    [F_VBUS_UVLZ_STAT] = SC_REG_FIELD(0x09, 2, 2),
    [F_VBUS_PRESENT_STAT] = SC_REG_FIELD(0x09, 1, 1),
    /*reg0A*/
    [F_ADC_DONE_FLAG] = SC_REG_FIELD(0x0A, 4, 4),
    [F_REGN_OK_FLAG] = SC_REG_FIELD(0x0A, 3, 3),
    [F_VBUS_UVLZ_FLAG] = SC_REG_FIELD(0x0A, 2, 2),
    [F_VBUS_PRESENT_FLAG] = SC_REG_FIELD(0x0A, 1, 1),
    /*reg0B*/
    [F_PN] = SC_REG_FIELD(0x0B, 3, 6),
    /*reg0C*/
    [F_WD_TIMEOUT_STAT] = SC_REG_FIELD(0x0C, 6, 6),
    [F_TSHUT_STAT] = SC_REG_FIELD(0x0C, 5, 5),
    [F_TSBAT_HOT_STAT] = SC_REG_FIELD(0x0C, 4, 4),
    [F_TSBAT_COLD_STAT] = SC_REG_FIELD(0x0C, 3, 3),
    [F_VBUS_OVP_STAT] = SC_REG_FIELD(0x0C, 1, 1),
    /*reg0D*/
    [F_WD_TIMEOUT_FLAG] = SC_REG_FIELD(0x0D, 6, 6),
    [F_TSHUT_FLAG] = SC_REG_FIELD(0x0D, 5, 5),
    [F_TSBAT_HOT_FLAG] = SC_REG_FIELD(0x0D, 4, 4),
    [F_TSBAT_COLD_FLAG] = SC_REG_FIELD(0x0D, 3, 3),
    [F_VBUS_OVP_FLAG] = SC_REG_FIELD(0x0D, 1, 1),
    /*reg0E*/
    [F_WD_TIMEOUT_MASK] = SC_REG_FIELD(0x0E, 6, 6),
    [F_TSHUT_MASK] = SC_REG_FIELD(0x0E, 5, 5),
    [F_TSBAT_HOT_MASK] = SC_REG_FIELD(0x0E, 4, 4),
    [F_TSBAT_COLD_MASK] = SC_REG_FIELD(0x0E, 3, 3),
    [F_VBUS_OVP_MASK] = SC_REG_FIELD(0x0E, 1, 1),
    /*reg0F*/
    [F_ADC_EN] = SC_REG_FIELD(0x0F, 7, 7),
    [F_ADC_RATE] = SC_REG_FIELD(0x0F, 6, 6),
    [F_FORCE_REGN_BYPASS] = SC_REG_FIELD(0x0F, 4, 4),
    /*reg12*/
    [F_IBUS_ADC] = SC_REG_FIELD(0x12, 0, 7),
    /*reg14*/
    [F_VBUS_ADC] = SC_REG_FIELD(0x14, 0, 7),
    /*reg1A*/
    [F_VBAT_ADC] = SC_REG_FIELD(0x1A, 0, 7),
    /*reg1C*/
    [F_IBAT_ADC] = SC_REG_FIELD(0x1C, 0, 7),
    /*reg22*/
    [F_TSBAT_ADC] = SC_REG_FIELD(0x22, 0, 7),
    /*reg24*/
    [F_TDIE_ADC] = SC_REG_FIELD(0x24, 0, 7),
    /*reg2F*/
    [F_BGOK_BLOCK_ANA_EN] = SC_REG_FIELD(0x2F, 0, 0),
    /*reg5F*/
    [F_ADC_DONE_MASK] = SC_REG_FIELD(0x5F, 4, 4),
    [F_REGN_OK_MASK] = SC_REG_FIELD(0x5F, 3, 3),
    [F_VBUS_UVLZ_MASK] = SC_REG_FIELD(0x5F, 2, 2),
    [F_VBUS_PRESENT_MASK] = SC_REG_FIELD(0x5F, 1, 1),

    /* buck charger register */
    /*reg30*/
    [F_SYS_MIN] = SC_REG_FIELD(0x30, 0, 2),
    /*reg31*/
    [F_VBAT_REG] = SC_REG_FIELD(0x31, 0, 6),
    /*reg32*/
    [F_ICC] = SC_REG_FIELD(0x32, 0, 5),
    /*reg33*/
    [F_VINDPM_TRACK] = SC_REG_FIELD(0x33, 5, 6),
    [F_VINDPM_FUNC_DIS] = SC_REG_FIELD(0x33, 4, 4),
    [F_VINDPM] = SC_REG_FIELD(0x33, 0, 3),
    /*reg34*/
    [F_IINDPM_FUNC_DIS] = SC_REG_FIELD(0x34, 7, 7),
    [F_IINDPM_UPDATE_DIS] = SC_REG_FIELD(0x34, 6, 6),
    [F_IINDPM] = SC_REG_FIELD(0x34, 0, 4),
    /*reg36*/
    [F_VBAT_PRECHG_THRE] = SC_REG_FIELD(0x36, 5, 6),
    [F_IPRECHG] = SC_REG_FIELD(0x36, 0, 4),
    /*reg37*/
    [F_TERM_EN] = SC_REG_FIELD(0x37, 7, 7),
    [F_ITERM] = SC_REG_FIELD(0x37, 0, 3),
    /*reg38*/
    [F_JEITA_OTG_HOT_TEMP] = SC_REG_FIELD(0x38, 6, 7),
    [F_JEITA_OTG_COLD_TEMP] = SC_REG_FIELD(0x38, 5, 5),
    [F_RECHG_DIS] = SC_REG_FIELD(0x38, 4, 4),
    [F_VRECHG] = SC_REG_FIELD(0x38, 0, 1),
    /*reg39*/
    [F_VBOOST] = SC_REG_FIELD(0x39, 3, 7),
    [F_IBOOST_LIMIT] = SC_REG_FIELD(0x39, 0, 2),
    /*reg3A*/
    [F_VBOOST_HIGHEST_BIT] = SC_REG_FIELD(0x3A, 7, 7),
    [F_TSBAT_JEITA_EN] = SC_REG_FIELD(0x3A, 3, 3),
    [F_IBAT_OCP_DIS] = SC_REG_FIELD(0x3A, 2, 2),
    [F_VPMID_OVP_OTG_DIS] = SC_REG_FIELD(0x3A, 1, 1),
    [F_VBAT_OVP_BUCK_DIS] = SC_REG_FIELD(0x3A, 0, 0),
    /*reg3B*/
    [F_BATFET_DIS_DELAY_STAT] = SC_REG_FIELD(0x3B, 6, 6),
    [F_BATFET_RST_EN] = SC_REG_FIELD(0x3B, 3, 3),
    [F_BATFET_DLY] = SC_REG_FIELD(0x3B, 2, 2),
    [F_BATFET_DIS] = SC_REG_FIELD(0x3B, 1, 1),
    /*reg3C*/
    [F_HIZ_EN] = SC_REG_FIELD(0x3C, 7, 7),
    [F_DIS_BUCKCHG_PATH] = SC_REG_FIELD(0x3C, 5, 5),
    [F_SLEEP_FOR_OTG] = SC_REG_FIELD(0x3C, 4, 4),
    [F_QB_EN] = SC_REG_FIELD(0x3C, 2, 2),
    [F_BOOST_EN] = SC_REG_FIELD(0x3C, 1, 1),
    [F_CHG_EN] = SC_REG_FIELD(0x3C, 0, 0),
    /*reg3D*/
    [F_IBAT_OCP_DG] = SC_REG_FIELD(0x3D, 6, 7),
    [F_IBAT_OCP] = SC_REG_FIELD(0x3D, 4, 4),
    [F_OTG_DPDM_DIS] = SC_REG_FIELD(0x3D, 3, 3),
    [F_VSYSOVP_DIS] = SC_REG_FIELD(0x3D, 2, 2),
    [F_VSYSOCP_THRE] = SC_REG_FIELD(0x3D, 0, 1),
    /*reg3E*/
    [F_BATFET_CTRL_WVBUS] = SC_REG_FIELD(0x3E, 7, 7),
    [F_NTC_CHG_COOL_VSET] = SC_REG_FIELD(0x3E, 6, 6),
    [F_JEITA_ISET_WARM] = SC_REG_FIELD(0x3E, 4, 5),
    [F_JEITA_ISET_COOL] = SC_REG_FIELD(0x3E, 2, 3),
    [F_JEITA_VSET_WARM] = SC_REG_FIELD(0x3E, 0, 1),
    /*reg3F*/
    [F_TMR2X_EN] = SC_REG_FIELD(0x3F, 7, 7),
    [F_CHG_TIMER_EN] = SC_REG_FIELD(0x3F, 6, 6),
    [F_CHG_TIMER] = SC_REG_FIELD(0x3F, 4, 5),
    [F_TDIE_REG_DIS] = SC_REG_FIELD(0x3F, 3, 3),
    [F_TDIE_REG] = SC_REG_FIELD(0x3F, 1, 2),
    [F_PFM_DIS] = SC_REG_FIELD(0x3F, 0, 0),
    /*reg40*/
    [F_VBAT_LOW_OTG] = SC_REG_FIELD(0x40, 5, 5),
    [F_BOOST_FREQ] = SC_REG_FIELD(0x40, 3, 4),
    [F_BUCK_FREQ] = SC_REG_FIELD(0x40, 1, 2),
    [F_BAT_LOAD_EN] = SC_REG_FIELD(0x40, 0, 0),
    /*reg41*/
    [F_VSYS_SHORT_STAT] = SC_REG_FIELD(0x41, 4, 4),
    [F_VSLEEP_BUCK_STAT] = SC_REG_FIELD(0x41, 3, 3),
    [F_VBAT_DPL_STAT] = SC_REG_FIELD(0x41, 2, 2),
    [F_VBAT_LOW_BOOST_STAT] = SC_REG_FIELD(0x41, 1, 1),
    [F_VBUS_GOOD_STAT] = SC_REG_FIELD(0x41, 0, 0),
    /*reg42*/
    [F_CHG_STAT] = SC_REG_FIELD(0x42, 5, 7),
    [F_BOOST_OK_STAT] = SC_REG_FIELD(0x42, 4, 4),
    [F_VSYSMIN_REG_STAT] = SC_REG_FIELD(0x42, 3, 3),
    [F_QB_ON_STAT] = SC_REG_FIELD(0x42, 2, 2),
    /*reg43*/
    [F_TSBAT_COOL_STAT] = SC_REG_FIELD(0x43, 7, 7),
    [F_TDIE_REG_STAT] = SC_REG_FIELD(0x43, 6, 6),
    [F_TSBAT_WARM_STAT] = SC_REG_FIELD(0x43, 4, 4),
    [F_IINDPM_STAT] = SC_REG_FIELD(0x43, 1, 1),
    [F_VINDPM_STAT] = SC_REG_FIELD(0x43, 0, 0),
    /*reg44*/
    [F_VSYS_SHORT_FLAG] = SC_REG_FIELD(0x44, 4, 4),
    [F_VSLEEP_BUCK_FLAG] = SC_REG_FIELD(0x44, 3, 3),
    [F_VBAT_DPL_FLAG] = SC_REG_FIELD(0x44, 2, 2),
    [F_VBAT_LOW_BOOST_FLAG] = SC_REG_FIELD(0x44, 1, 1),
    [F_VBUS_GOOD_FLAG] = SC_REG_FIELD(0x44, 0, 0),
    /*reg45*/
    [F_CHG_FLAG] = SC_REG_FIELD(0x45, 5, 5),
    [F_BOOST_OK_FLAG] = SC_REG_FIELD(0x45, 4, 4),
    [F_VSYSMIN_REG_FLAG] = SC_REG_FIELD(0x45, 3, 3),
    [F_QB_EN_FLAG] = SC_REG_FIELD(0x45, 2, 2),
    /*reg46*/
    [F_TSBAT_COOL_FLAG] = SC_REG_FIELD(0x46, 7, 7),
    [F_TDIE_REG_FLAG] = SC_REG_FIELD(0x46, 6, 6),
    [F_TSBAT_WARM_FLAG] = SC_REG_FIELD(0x46, 4, 4),
    [F_IINDPM_FLAG] = SC_REG_FIELD(0x46, 1, 1),
    [F_VINDPM_FLAG] = SC_REG_FIELD(0x46, 0, 0),
    /*reg47*/
    [F_VSYS_SHORT_MASK] = SC_REG_FIELD(0x47, 4, 4),
    [F_VSLEEP_BUCK_MASK] = SC_REG_FIELD(0x47, 3, 3),
    [F_VBAT_DPL_MASK] = SC_REG_FIELD(0x47, 2, 2),
    [F_VBAT_LOW_BOOST_MASK] = SC_REG_FIELD(0x47, 1, 1),
    [F_VBUS_GOOD_MASK] = SC_REG_FIELD(0x47, 0, 0),
    /*reg48*/
    [F_CHG_MASK] = SC_REG_FIELD(0x48, 5, 5),
    [F_BOOST_OK_MASK] = SC_REG_FIELD(0x48, 4, 4),
    [F_VSYSMIN_REG_MASK] = SC_REG_FIELD(0x48, 3, 3),
    [F_QB_EN_MASK] = SC_REG_FIELD(0x48, 2, 2),
    /*reg49*/
    [F_TSBAT_COOL_MASK] = SC_REG_FIELD(0x49, 7, 7),
    [F_TDIE_REG_MASK] = SC_REG_FIELD(0x49, 6, 6),
    [F_TSBAT_WARM_MASK] = SC_REG_FIELD(0x49, 4, 4),
    [F_IINDPM_MASK] = SC_REG_FIELD(0x49, 1, 1),
    [F_VINDPM_MASK] = SC_REG_FIELD(0x49, 0, 0),
    /*reg4D*/
    [F_OTG_CONV_OCP_HICCUP_DIS] = SC_REG_FIELD(0x4D, 5, 5),
    [F_CONV_OCP_THRE] = SC_REG_FIELD(0x4D, 3, 4),
    [F_SET_SW_SLEW_RATE_OTG] = SC_REG_FIELD(0x4D, 2, 2),
    [F_SET_SW_SLEW_RATE_FWD0] = SC_REG_FIELD(0x4D, 1, 1),
    [F_OTG_SOFT_START_FAST] = SC_REG_FIELD(0x4D, 0, 0),
    /*reg4E*/
    [F_TSHIPMODE_EXIT_SET] = SC_REG_FIELD(0x4E, 7, 7),
    [F_EOC_DPM_CTRL] = SC_REG_FIELD(0x4E, 5, 5),
    [F_TRICKLE_CURRENT] = SC_REG_FIELD(0x4E, 2, 2),
    /*reg50*/
    [F_CONV_OCP_STAT] = SC_REG_FIELD(0x50, 4, 4),
    [F_VSYS_OVP_STAT] = SC_REG_FIELD(0x50, 3, 3),
    [F_IBAT_OCP_STAT] = SC_REG_FIELD(0x50, 1, 1),
    [F_VBAT_OVP_BUCK_STAT] = SC_REG_FIELD(0x50, 0, 0),
    /*reg51*/
    [F_OTG_VBUS_STAT] = SC_REG_FIELD(0x51, 4, 4),
    [F_CHG_TIMEOUT_STAT] = SC_REG_FIELD(0x51, 2, 2),
    [F_VPMID_OVP_OTG_STAT] = SC_REG_FIELD(0x51, 0, 0),
    /*reg52*/
    [F_CONV_OCP_FLAG] = SC_REG_FIELD(0x52, 4, 4),
    [F_VSYS_OVP_FLAG] = SC_REG_FIELD(0x52, 3, 3),
    [F_IBAT_OCP_FLAG] = SC_REG_FIELD(0x52, 1, 1),
    [F_VBAT_OVP_BUCK_FLAG] = SC_REG_FIELD(0x52, 0, 0),
    /*reg53*/
    [F_OTG_VBUS_FLAG] = SC_REG_FIELD(0x53, 4, 4),
    [F_CHG_TIMEOUT_FLAG] = SC_REG_FIELD(0x53, 2, 2),
    [F_VPMID_OVP_OTG_FLAG] = SC_REG_FIELD(0x53, 0, 0),
    /*reg54*/
    [F_CONV_OCP_MASK] = SC_REG_FIELD(0x54, 4, 4),
    [F_VSYS_OVP_MASK] = SC_REG_FIELD(0x54, 3, 3),
    [F_IBAT_OCP_MASK] = SC_REG_FIELD(0x54, 1, 1),
    [F_VBAT_OVP_BUCK_MASK] = SC_REG_FIELD(0x54, 0, 0),
    /*reg55*/
    [F_OTG_VBUS_MASK] = SC_REG_FIELD(0x55, 4, 4),
    [F_CHG_TIMEOUT_MASK] = SC_REG_FIELD(0x55, 2, 2),
    [F_VPMID_OVP_OTG_MASK] = SC_REG_FIELD(0x55, 0, 0),
    /*reg56*/
    [F_JEITA_WARM_TEMP] = SC_REG_FIELD(0x56, 2, 3),
    [F_OTG_PMID_UV] = SC_REG_FIELD(0x56, 0, 0),
    /*reg57*/
    [F_EOC_STAT] = SC_REG_FIELD(0x57, 1, 1),
    [F_BOOST_FAULT_STAT] = SC_REG_FIELD(0x57, 0, 0),
    /*reg58*/
    [F_EOC_FLAG] = SC_REG_FIELD(0x58, 1, 1),
    [F_BOOST_FAULT_FLAG] = SC_REG_FIELD(0x58, 0, 0),
    /*reg59*/
    [F_EOC_MASK] = SC_REG_FIELD(0x59, 1, 1),
    [F_BOOST_FAULT_MASK] = SC_REG_FIELD(0x59, 0, 0),
    /*reg5A*/
    [F_JEITA_COOL_TEMP] = SC_REG_FIELD(0x5A, 2, 3),
    [F_OTG_PMID_UV_DG] = SC_REG_FIELD(0x5A, 1, 1),
    /*reg5B*/
    [F_JEITA_HOT_TEMP] = SC_REG_FIELD(0x5B, 2, 2),
    [F_OTG_OVP] = SC_REG_FIELD(0x5B, 0, 0),
    /*reg5C*/
    [F_JEITA_COLD_TEMP] = SC_REG_FIELD(0x5C, 2, 2),
    [F_VBAT_UVLZ_DPLZ_FALL_DG] = SC_REG_FIELD(0x5C, 0, 1),
    /*reg5D*/
    [F_ICHG_FOLD] = SC_REG_FIELD(0x5D, 4, 4),
    [F_SET_SW_SLEW_RATE_FWD1] = SC_REG_FIELD(0x5D, 2, 2),

    /* dpdm register */
    /*reg90*/
    [F_FORCE_INDET_EN] = SC_REG_FIELD_FORCE_WRITE(0x90, 7, 7),
    [F_AUTO_INDET_EN] = SC_REG_FIELD(0x90, 6, 6),
    [F_HVDCP_EN] = SC_REG_FIELD(0x90, 5, 5),
    [F_IN_DPDM_DET] = SC_REG_FIELD(0x90, 2, 2),
    [F_DPDM_INDET_SUPPORT] = SC_REG_FIELD(0x90, 1, 1),
    /*reg91*/
    [F_DP_DRIVE] = SC_REG_FIELD(0x91, 5, 7),
    [F_DM_DRIVE] = SC_REG_FIELD(0x91, 2, 4),
    [F_BC12_VDAT_REF_SET] = SC_REG_FIELD(0x91, 1, 1),
    [F_BC12_DPDM_SINK_CAP] = SC_REG_FIELD(0x91, 0, 0),
    /*reg94*/
    [F_VBUS_STAT] = SC_REG_FIELD(0x94, 5, 7),
    [F_INPUT_DET_DONE_FLAG] = SC_REG_FIELD(0x94, 2, 2),
    [F_DP_OVP_FLAG] = SC_REG_FIELD(0x94, 1, 1),
    [F_DM_OVP_FLAG] = SC_REG_FIELD(0x94, 0, 0),
    /*reg95*/
    [F_INPUT_DET_DONE_MASK] = SC_REG_FIELD(0x95, 5, 7),
    [F_DP_OVP_MASK] = SC_REG_FIELD(0x95, 2, 2),
    [F_DM_OVP_MASK] = SC_REG_FIELD(0x95, 1, 1),
    /*reg98*/
    [F_DP_OVP_STAT] = SC_REG_FIELD(0x98, 7, 7),
    [F_DP_IN5] = SC_REG_FIELD(0x98, 5, 5),
    [F_DP_IN4] = SC_REG_FIELD(0x98, 4, 4),
    [F_DP_IN3] = SC_REG_FIELD(0x98, 3, 3),
    [F_DP_IN2] = SC_REG_FIELD(0x98, 2, 2),
    [F_DP_IN1] = SC_REG_FIELD(0x98, 1, 1),
    [F_DP_IN0] = SC_REG_FIELD(0x98, 0, 0),
    /*reg99*/
    [F_DM_OVP_STAT] = SC_REG_FIELD(0x99, 7, 7),
    [F_DM_IN5] = SC_REG_FIELD(0x99, 5, 5),
    [F_DM_IN4] = SC_REG_FIELD(0x99, 4, 4),
    [F_DM_IN3] = SC_REG_FIELD(0x99, 3, 3),
    [F_DM_IN2] = SC_REG_FIELD(0x99, 2, 2),
    [F_DM_IN1] = SC_REG_FIELD(0x99, 1, 1),
    [F_DM_IN0] = SC_REG_FIELD(0x99, 0, 0),
    /*reg9A*/
    [F_DPDM_OVP_EN] = SC_REG_FIELD(0x9A, 7, 7),
    [F_DPDM_POLLING_EN] = SC_REG_FIELD(0x9A, 5, 5),
    [F_NONSTD_PRI_HIGH] = SC_REG_FIELD(0x9A, 3, 3),
    [F_DPDM_VTH_REF] = SC_REG_FIELD(0x9A, 0, 2),
    /*reg9D*/
    [F_DM_500K_PD_EN] = SC_REG_FIELD(0x9D, 7, 7),
    [F_DP_500_PD_EN] = SC_REG_FIELD(0x9D, 6, 6),
    [F_DM_SINK_EN] = SC_REG_FIELD(0x9D, 5, 5),
    [F_DP_SINK_EN] = SC_REG_FIELD(0x9D, 4, 4),
    [F_DP_SRC_10UA] = SC_REG_FIELD(0x9D, 3, 3),
    [F_VBUS_PLUGIN_RST_DPDM_DIS] = SC_REG_FIELD(0x9D, 2, 2),
    [F_VBUS_PLUGOUT_RST_DPDM_DIS] = SC_REG_FIELD(0x9D, 1, 1),
    [F_DPDM_EN] = SC_REG_FIELD(0x9D, 0, 0),
    /*reg9E*/
    [F_DPDM_STAT] = SC_REG_FIELD(0x9E, 4, 7),
    [F_DM_SRC_10UA] = SC_REG_FIELD(0x9E, 3, 3),
    [F_NONSTD_FLAG] = SC_REG_FIELD(0x9E, 0, 2),

};

/********************COMMON API***********************/
static uint8_t val2reg(enum scv89601p_reg_range id, uint32_t val) {
    int i;
    uint8_t reg;
    const struct reg_range *range = &scv89601p_reg_range_ary[id];

    if (!range)
        return val;

    if (range->table) {
        if (val <= range->table[0])
            return 0;
        for (i = 0; i < range->num_table - 1; i++) {
            if (val == range->table[i]) {
                return i;
            }
            if (val > range->table[i] && val < range->table[i + 1]) {
                return range->round_up ? i + 1 : i;
            }
        }
        return range->num_table - 1;
    }
    if (val <= range->min)
        reg = (range->min - range->offset) / range->step;
    else if (val >= range->max)
        reg = (range->max - range->offset) / range->step;
    else if (range->round_up)
        reg = (val - range->offset) / range->step + 1;
    else
        reg = (val - range->offset) / range->step;
    return reg;
}

static uint32_t reg2val(enum scv89601p_reg_range id, uint8_t reg) {
    const struct reg_range *range = &scv89601p_reg_range_ary[id];
    if (!range)
        return reg;
    return range->table ? range->table[reg] : range->offset + range->step * reg;
}

/*********************I2C API*********************/
static int scv89601p_i2c_write_bytes(struct scv89601p_chip *sc, uint8_t reg, uint8_t len, uint8_t *val)
{
    struct i2c_client *i2c = to_i2c_client(sc->dev);

    return i2c_smbus_write_i2c_block_data(i2c, reg, len, val);
}

static int scv89601p_i2c_read_bytes(struct scv89601p_chip *sc, uint8_t reg, uint8_t len, uint8_t *val)
{
    struct i2c_client *i2c = to_i2c_client(sc->dev);

    return i2c_smbus_read_i2c_block_data(i2c, reg, len, val);
}

static int scv89601p_i2c_write_byte(struct scv89601p_chip *sc, uint8_t reg, uint8_t val)
{
    return scv89601p_i2c_write_bytes(sc, reg, 1, &val);
}

/* HS07 code for HS07-223 by lina at 20250328 start */
static int scv89601p_i2c_read_byte(struct scv89601p_chip *sc, uint8_t reg, uint8_t *val)
{
    int ret = scv89601p_i2c_read_bytes(sc, reg, 1, val);
    if (reg == 0x94 && (*val & BIT(2))) {
        sc->dpdm_done_flag = 1;
    }
    return ret;
}
/* HS07 code for HS07-223 by lina at 20250328 end */

static int scv89601p_field_read(struct scv89601p_chip *sc,
                            enum scv89601p_fields field_id, int *val)
{
    int ret;
    uint8_t reg_val = 0;
    uint8_t mask = GENMASK(scv89601p_reg_fields[field_id].msb, scv89601p_reg_fields[field_id].lsb);

    ret = scv89601p_i2c_read_byte(sc, scv89601p_reg_fields[field_id].reg, &reg_val);
    if (ret < 0) {
        SCV89601P_ERR("scv89601p read field %d fail: %d\n", field_id, ret);
        return ret;
    }

    reg_val &= mask;
    reg_val >>= scv89601p_reg_fields[field_id].lsb;

    *val = reg_val;

    return ret;
}

static int scv89601p_field_write(struct scv89601p_chip *sc,
                            enum scv89601p_fields field_id, int val)
{
    int ret;
    uint8_t reg_val = 0, tmp = 0;
    uint8_t mask = GENMASK(scv89601p_reg_fields[field_id].msb, scv89601p_reg_fields[field_id].lsb);

    ret = scv89601p_i2c_read_byte(sc, scv89601p_reg_fields[field_id].reg, &reg_val);
    if (ret < 0) {
        goto out;
    }

    tmp = reg_val & ~mask;
    val <<= scv89601p_reg_fields[field_id].lsb;
    tmp |= val  & mask;

    if (scv89601p_reg_fields[field_id].force_write || tmp != reg_val) {
        ret = scv89601p_i2c_write_byte(sc, scv89601p_reg_fields[field_id].reg, tmp);
    }

out:
    if (ret < 0) {
        SCV89601P_ERR("scv89601p write field %d fail: %d\n", field_id, ret);
    }
    return ret;
}

/*********************CHIP API*********************/
__maybe_unused
static int scv89601p_set_vindpm_track(struct scv89601p_chip *sc,
                                     enum vindpm_track track)
{
    int reg_val;

    scv89601p_field_write(sc, F_VINDPM_TRACK, track);
    scv89601p_field_read(sc, F_VINDPM_TRACK, &reg_val);
    SCV89601P_INFO("%s: successful set F_VINDPM track as 0x%x\n",
                           __func__, reg_val);

    return 0;
}

__maybe_unused
static int scv89601p_set_hvdcp_en(struct scv89601p_chip *sc)
{
    return scv89601p_field_write(sc, F_HVDCP_EN, 1);
}

__maybe_unused
static int scv89601p_set_hvdcp_9v(struct scv89601p_chip *sc)
{
    int ret;

    ret = scv89601p_field_write(sc, F_DP_DRIVE, 0x05);
    ret |= scv89601p_field_write(sc, F_DM_DRIVE, 0x02);
    pr_err("%s \n", __func__);

    return ret;
}

__maybe_unused
static int scv89601p_set_hvdcp_5v(struct scv89601p_chip *sc)
{
    int ret;

    ret = scv89601p_field_write(sc, F_DP_DRIVE, 0x02);
    ret |= scv89601p_field_write(sc, F_DM_DRIVE, 0x01);

    return ret;
}

__maybe_unused
static int scv89601p_set_soft_hvdcp(struct scv89601p_chip *sc)
{
    int ret = 0;

    ret = scv89601p_field_write(sc, F_DP_DRIVE, DPDM_0P6);
    ret |= scv89601p_field_write(sc, F_DM_DRIVE, DPDM_HIZ);
    pr_err("%s \n", __func__);
    /*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 start*/
    schedule_delayed_work(&sc->hvdcp_done_work, msecs_to_jiffies(1500));
    /*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 end*/

    return ret;
}

/*HS07 code for HS07-188 by lina at 20250320 start*/
__maybe_unused
static int scv89601p_hk_buck_irq_mask(struct scv89601p_chip *sc)
{
    int ret;

    ret = scv89601p_field_write(sc, F_TSHUT_MASK, 1);
    ret |= scv89601p_field_write(sc, F_TSBAT_HOT_MASK, 1);
    ret |= scv89601p_field_write(sc, F_TSBAT_COLD_MASK, 1);
    ret |= scv89601p_field_write(sc, F_ADC_DONE_MASK, 1);
    ret |= scv89601p_field_write(sc, F_REGN_OK_MASK, 1);
    ret |= scv89601p_field_write(sc, F_TSBAT_COOL_MASK, 1);
    ret |= scv89601p_field_write(sc, F_TDIE_REG_MASK, 1);
    ret |= scv89601p_field_write(sc, F_TSBAT_WARM_MASK, 1);
    ret |= scv89601p_field_write(sc, F_IINDPM_MASK, 1);
    ret |= scv89601p_field_write(sc, F_VINDPM_MASK, 1);
    ret |= scv89601p_field_write(sc, F_CHG_MASK, 1);
    ret |= scv89601p_field_write(sc, F_QB_EN_MASK, 1);

    return ret;
}
/*HS07 code for HS07-188 by lina at 20250320 end*/

static int scv89601p_set_adc_conv_rate(struct scv89601p_chip *sc, int val)
{
    int reg_val = val;
    return scv89601p_field_write(sc, F_ADC_RATE, reg_val);
}

static int scv89601p_set_iindpm(struct scv89601p_chip *sc, int curr_ma) {
    int reg_val = val2reg(SCV89601P_IINDPM, curr_ma);

    return scv89601p_field_write(sc, F_IINDPM, reg_val);
}

static int scv89601p_get_iindpm(struct scv89601p_chip *sc, int *curr_ma) {
    int ret, reg_val;

    ret = scv89601p_field_read(sc, F_IINDPM, &reg_val);
    if (ret < 0) {
        SCV89601P_ERR("read iindpm failed(%d)\n", ret);
        return ret;
    }

    *curr_ma = reg2val(SCV89601P_IINDPM, reg_val);

    return ret;
}

__maybe_unused
static int scv89601p_set_batfet_dis(struct scv89601p_chip *sc, bool en)
{
    int ret = 0;

    if (en) {
        ret = scv89601p_field_write(sc, F_BATFET_DIS, 1);
    } else {
        ret = scv89601p_field_write(sc, F_BATFET_DIS, 0);
    }

    return ret;
}

static int scv89601p_set_dpdm_hiz(struct scv89601p_chip *sc)
{
    int ret = 0;
    ret = scv89601p_field_write(sc, F_DP_DRIVE, 0);
    ret |= scv89601p_field_write(sc, F_DM_DRIVE, 0);
    return ret;
}

__maybe_unused
static int scv89601p_reset_wdt(struct scv89601p_chip *sc) {
    return scv89601p_field_write(sc, F_WD_TIMER_RST, 1);
}

static int scv89601p_set_chg_enable(struct scv89601p_chip *sc, bool enable) {
    int reg_val = enable ? 1 : 0;

    return scv89601p_field_write(sc, F_CHG_EN, reg_val);
}

__maybe_unused
static int scv89601p_check_chg_enabled(struct scv89601p_chip *sc,
                                                    bool * enable) {
    int ret, reg_val;

    ret = scv89601p_field_read(sc, F_CHG_EN, &reg_val);
    if (ret < 0) {
        SCV89601P_ERR("read charge enable failed(%d)\n", ret);
        return ret;
    }
    *enable = !!reg_val;

    return ret;
}

__maybe_unused
static int scv89601p_set_otg_enable(struct scv89601p_chip *sc,
                                                bool enable) {
    int reg_val = enable ? 1 : 0;
    int ret;

    ret = scv89601p_field_write(sc, F_BOOST_EN, reg_val);

    return ret;
}

__maybe_unused
static int scv89601p_batfet_rst_en(struct scv89601p_chip *sc,
                                                bool enable) {
    int reg_val = enable ? 1 : 0;

    return scv89601p_field_write(sc, F_BATFET_RST_EN, reg_val);
}

__maybe_unused
static int scv89601p_set_iboost(struct scv89601p_chip *sc,
                                            int curr_ma) {
    int reg_val = val2reg(SCV89601P_IBOOST, curr_ma);

    return scv89601p_field_write(sc, F_IBOOST_LIMIT, reg_val);
}

__maybe_unused
static int scv89601p_set_vboost(struct scv89601p_chip *sc,
                                            int volt_mv) {
    int reg_val = val2reg(SCV89601P_VBOOST, volt_mv);
    int vboost_highest_bit = 0;
    int vboost;
    int ret = 0;

    vboost_highest_bit = (reg_val & 0x20) >> 5;
    vboost = reg_val & 0x1F;

    ret = scv89601p_field_write(sc, F_VBOOST_HIGHEST_BIT, vboost_highest_bit);
    ret |= scv89601p_field_write(sc, F_VBOOST, vboost);
    return ret;
}

static int scv89601p_set_ichg(struct scv89601p_chip *sc, int curr_ma) {
    int reg_val = val2reg(SCV89601P_ICHG, curr_ma);

    return scv89601p_field_write(sc, F_ICC, reg_val);
}

static int scv89601p_get_ichg(struct scv89601p_chip *sc, int *curr_ma) {
    int ret, reg_val;
    ret = scv89601p_field_read(sc, F_ICC, &reg_val);
    if (ret < 0) {
        SCV89601P_ERR("read icc failed(%d)\n", ret);
        return ret;
    }

    *curr_ma = reg2val(SCV89601P_ICHG, reg_val);

    return ret;
}

static int scv89601p_set_term_curr(struct scv89601p_chip *sc, int curr_ma) {
    int reg_val = val2reg(SCV89601P_ITERM, curr_ma);

    return scv89601p_field_write(sc, F_ITERM, reg_val);
}

static int scv89601p_get_term_curr(struct scv89601p_chip *sc, int *curr_ma) {
    int ret, reg_val;

    ret = scv89601p_field_read(sc, F_ITERM, &reg_val);
    if (ret < 0)
        return ret;

    *curr_ma = reg2val(SCV89601P_ITERM, reg_val);

    return ret;
}

__maybe_unused
static int scv89601p_set_safet_timer(struct scv89601p_chip *sc,
                                                bool enable) {
    int reg_val = enable ? 1 : 0;

    return scv89601p_field_write(sc, F_CHG_TIMER_EN, reg_val);
}

/* HST11 code for AX7800A-625 by zhangziyi at 20250429 start */
__maybe_unused
static int scv89601p_ocp_irq_mask(struct scv89601p_chip *sc)
{
    int ret = 0;

    ret = scv89601p_field_write(sc, F_CONV_OCP_MASK, 0x01);
    if (ret < 0) {
        SCV89601P_ERR("write F_CONV_OCP_MASK failed(%d)\n", ret);
    }

    ret = scv89601p_i2c_write_byte(sc, 0x4D, 0x37);
    if (ret < 0) {
        SCV89601P_ERR("write 0x4D failed(%d)\n", ret);
    }

    return ret;
}
/* HST11 code for AX7800A-625 by zhangziyi at 20250429 end */

__maybe_unused
static int scv89601p_check_safet_timer(struct scv89601p_chip *sc,
                                                    bool * enabled) {
    int ret, reg_val;

    ret = scv89601p_field_read(sc, F_CHG_TIMER_EN, &reg_val);
    if (ret < 0) {
        SCV89601P_ERR("read ICEN_TIMERC failed(%d)\n", ret);
        return ret;
    }

    *enabled = reg_val ? true : false;

    return ret;
}

static int scv89601p_set_vbat(struct scv89601p_chip *sc, int volt_mv)
{
    int reg_val = val2reg(SCV89601P_VBAT_REG, volt_mv);

    return scv89601p_field_write(sc, F_VBAT_REG, reg_val);
}

static int scv89601p_get_vbat(struct scv89601p_chip *sc, int *volt_mv)
{
    int ret, reg_val;

    ret = scv89601p_field_read(sc, F_VBAT_REG, &reg_val);
    if (ret < 0) {
        SCV89601P_ERR("read vbat reg failed(%d)\n", ret);
        return ret;
    }

    *volt_mv = reg2val(SCV89601P_VBAT_REG, reg_val);

    return ret;
}

static int scv89601p_set_vindpm(struct scv89601p_chip *sc, int volt_mv)
{
    int reg_val = 0;

    /* HS07 code for HS07-1480 by lina at 20250425 start */
    #if IS_ENABLED(CONFIG_CUSTOM_USBIF)
    if (sc->is_first_plugin == true) {
        volt_mv = 7600;
        /*HS07 code for HS07-2435 by xiongxiaoliang at 20250429 start*/
        SCV89601P_INFO("is_first_plugin:%d\n", sc->is_first_plugin);
        /*HS07 code for HS07-2435 by xiongxiaoliang at 20250429 end*/
    }
    /*HS07 code for HS07-752 by xiongxiaoliang at 20250514 start*/
    if (sc->pd_set_vindpm_sts == true) {
        volt_mv = 10700;
    }
    /*HS07 code for HS07-752 by xiongxiaoliang at 20250514 end*/
    #endif//CONFIG_CUSTOM_USBIF
    /* HS07 code for HS07-1480 by lina at 20250425 end */
    if (volt_mv == VINDPM_BEFORCE_BC12 && sc->bc12_detect) {
        volt_mv = VIMDPM_DEFAULT_IN_BC12;
        SCV89601P_INFO("bc12 running, set vindpm to 4800, avoid adapter fall\n");
    } else {
        sc->vindpm_value = volt_mv;
        SCV89601P_INFO("update vindpm value = %d\n", sc->vindpm_value);
    }
    reg_val = val2reg(SCV89601P_VINDPM, volt_mv);

    return scv89601p_field_write(sc, F_VINDPM, reg_val);

}

/* HS07 code for HS07-1480 by lina at 20250425 start */
#if IS_ENABLED(CONFIG_CUSTOM_USBIF)
void scv89601p_set_input_volt_lim(struct scv89601p_chip *sc, bool enable)
{
    unsigned int vindpm = 0;
    if (enable == true) {
        vindpm = 4600;
    } else {
        vindpm = 10700;
    }
    if (sc != NULL) {
        scv89601p_set_vindpm(sc, vindpm);
    } else {
        pr_err("[%s] sc is NULL", __func__);
    }
}

void scv89601p_set_input_volt_lim_pd(struct scv89601p_chip *sc, bool enable)
{
    unsigned int vindpm = 0;
    if (enable == true) {
        vindpm = 4600;
    } else {
        vindpm = 7600;
    }
    if (sc != NULL) {
        scv89601p_set_vindpm(sc, vindpm);
    } else {
        pr_err("[%s] sc is NULL", __func__);
    }
}

void scv89601p_set_input_volt_limit_plugin(struct scv89601p_chip *sc)
{
    if (sc != NULL) {
        scv89601p_set_input_volt_lim_pd(sc, false);
        schedule_delayed_work(&sc->set_input_volt_lim_work,
            msecs_to_jiffies(1500));
    } else {
        pr_err("[%s] sc is NULL", __func__);
    }
}

static void scv89601p_delay_set_input_volt_lim(struct work_struct *work)
{
    struct delayed_work *set_input_volt_lim_work = NULL;
    struct scv89601p_chip *sc = NULL;
    set_input_volt_lim_work = container_of(work, struct delayed_work, work);

    if (set_input_volt_lim_work == NULL) {
        pr_err("Cann't get set_input_volt_lim_work\n");
        return;
    }
    sc = container_of(set_input_volt_lim_work, struct scv89601p_chip,
                       set_input_volt_lim_work);
    if (sc == NULL) {
        pr_err("Cann't get scv89601p_chip\n");
        return;
    }
    sc->is_first_plugin = false;
    scv89601p_set_input_volt_lim_pd(sc, true);
    pr_err("%s end\n", __func__);
}

static int scv89601p_set_vindpm_pd(struct charger_device *chg_dev, bool enable)
{
    struct scv89601p_chip *sc = charger_get_data(chg_dev);
    /*HS07 code for HS07-752 by xiongxiaoliang at 20250514 start*/
    sc->pd_set_vindpm_sts = enable;
    pr_info("%s value = %d, pd_set_vindpm_sts = %d\n", __func__, enable, sc->pd_set_vindpm_sts);
    /*HS07 code for HS07-752 by xiongxiaoliang at 20250514 end*/
    if (enable) {
        scv89601p_set_input_volt_lim(sc, false);
    } else {
        scv89601p_set_input_volt_lim(sc, true);
    }

    return 0;

}
#endif//CONFIG_CUSTOM_USBIF
/* HS07 code for HS07-1480 by lina at 20250425 end */

static int scv89601p_get_vindpm(struct scv89601p_chip *sc, int *volt_mv)
{
    int ret, reg_val;

    ret = scv89601p_field_read(sc, F_VINDPM, &reg_val);
    if (ret < 0)
        return ret;

    *volt_mv = reg2val(SCV89601P_VINDPM, reg_val);

    return ret;
}

__maybe_unused
static int scv89601p_set_hiz(struct scv89601p_chip *sc, bool enable)
{
    int reg_val = enable ? 1 : 0;
    scv89601p_set_iindpm(sc, 500);
    scv89601p_set_ichg(sc, 500);
    if (enable == false) {
        schedule_delayed_work(&sc->hiz_cut_dwork,msecs_to_jiffies(DEFAULT_HIZ_CUT_TIME_EXPRIE));
        sc->hiz_cut_flag = true;
    } else {
        if (sc->hiz_cut_flag) {
            SCV89601P_INFO("disable hiz not end, after 300ms retry\n");
            return -EBUSY;
        }
    }

    pr_err ("%s enable %d\n",__func__ , enable);

    return scv89601p_field_write(sc, F_HIZ_EN, reg_val);
}

/* HS07 code for HS07-184 by lina at 20250319 start */
__maybe_unused
static int scv89601p_set_dis_buck(struct scv89601p_chip *sc, bool enable)
{
    int reg_val = enable ? 1 : 0;

    /*HS07 code for HS07-188 by lina at 20250320 start*/
    /*HS07 code for HS07-793 by xiongxiaoliang at 20250417 start*/
    sc->hvdcp_done = false;
    /*HS07 code for HS07-793 by xiongxiaoliang at 20250417 end*/
    /*HS07 code for HS07-188 by lina at 20250320 end*/
    pr_err ("%s enable %d\n",__func__ , enable);

    return scv89601p_field_write(sc, F_DIS_BUCKCHG_PATH, reg_val);
}

__maybe_unused
static int scv89601p_get_dis_buck(struct scv89601p_chip *sc, bool *enable)
{
    int ret = 0;
    int reg_val = 0;

    ret = scv89601p_field_read(sc, F_DIS_BUCKCHG_PATH, &reg_val);
    if (ret < 0) {
        pr_err ("read F_DIS_BUCKCHG_PATH fail\n");
        return ret;
    }
    if (reg_val) {
        *enable = true;
    } else {
        *enable = false;
    }
    pr_err ("scv89601p_get_dis_buck enable %d\n", *enable);

    return ret;
}
/* HS07 code for HS07-184 by lina at 20250319 end */

/* HS07 code for SR-AL7761A-01-155|HS07-184 by lina at 20250319 start */
static int scv89601p_enable_port_charging(struct charger_device *chg_dev, bool enable)
{
    struct scv89601p_chip *sc = charger_get_data(chg_dev);
    int ret = 0;

    dev_info(sc->dev, "%s value = %d\n", __func__, enable);
    if (enable) {
        ret = scv89601p_set_dis_buck(sc, !enable);
        sc->is_recharging = true;
    } else {
        ret = scv89601p_set_dis_buck(sc, !enable);
        sc->is_recharging = false;
    }

    return ret;
}

static int scv89601p_is_port_charging_enabled(struct charger_device *chg_dev, bool *enable)
{
    struct scv89601p_chip *sc = charger_get_data(chg_dev);
    int ret = 0;
    bool hiz_enable = false;

    ret = scv89601p_get_dis_buck(sc, &hiz_enable);
    *enable = !hiz_enable;
    dev_info(sc->dev, "%s enable %d\n", __func__ , *enable);

    return ret;
}
/* HS07 code for SR-AL7761A-01-155|HS07-184 by lina at 20250319 end */

/* HS07 code for SR-AL7761A-01-85 by lina at 20250317 start */
__maybe_unused
static int scv89601p_set_shipmode(struct charger_device *chg_dev, bool en)
{
    int ret = 0;
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    sc->shipmode_wr_value = 0;
    if (en) {
        ret = scv89601p_set_hiz(sc, en);
        ret |= scv89601p_field_write(sc, F_BATFET_DLY, 1);
        ret |= scv89601p_field_write(sc, F_BATFET_DIS, 1);
    } else {
        ret = scv89601p_field_write(sc, F_BATFET_DIS, 0);
    }

    if(ret < 0) {
        pr_err("scv89601p set shipmode failed\n");
        return ret;
    }
    sc->shipmode_wr_value = en;
    pr_err ("%s shipmode %s \n", __func__, en ? "enable" : "disable");

    return ret;
}


__maybe_unused
static int scv89601p_get_shipmode(struct charger_device *chg_dev)
{
    int ret = 0;
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    ret = sc->shipmode_wr_value;
    pr_err("%s:shipmode_wr %s\n",__func__, ret ? "enabled" : "disabled");

    return ret;
}
/* HS07 code for SR-AL7761A-01-85 by lina at 20250317 end */

static int scv89601p_force_dpdm(struct scv89601p_chip *sc);
static void scv89601p_bc12_timeout_dwork_handler(struct work_struct *work)
{
    int ret;
    int vbus_stat = 0;
    uint8_t force_dpdm_stat = 0;
    int dpdm_done_state = 0;
    struct scv89601p_chip *sc = container_of(work,
                                        struct scv89601p_chip,
                                        bc12_timeout_dwork.work);

    ret = scv89601p_field_read(sc, F_VBUS_STAT, &vbus_stat);
    SCV89601P_INFO("vbus stat = %d\n", vbus_stat);
    dpdm_done_state = sc->dpdm_done_flag;
    ret = scv89601p_i2c_read_byte(sc, 0x90, &force_dpdm_stat);
    force_dpdm_stat &= 0x80;
    SCV89601P_INFO("force_dpdm = %d\n", force_dpdm_stat);

    mutex_lock(&sc->bc_detect_lock);
    sc->bc12_detect = false;
    mutex_unlock(&sc->bc_detect_lock);
    if (force_dpdm_stat && !vbus_stat && !dpdm_done_state && !sc->bc12_recovery) {
        SCV89601P_INFO("BC1.2 timeout\n");
        scv89601p_force_dpdm(sc);
        sc->bc12_recovery = true;
    }
}

static irqreturn_t scv89601p_irq_handler(int irq, void *data);
static void scv89601p_hiz_cut_dwork_handler(struct work_struct *work) {
    struct scv89601p_chip *sc = container_of(work,
                                    struct scv89601p_chip,
                                    hiz_cut_dwork.work);
    SCV89601P_INFO("disable hiz cut end\n");
    sc->hiz_cut_flag = false;
    scv89601p_irq_handler(sc->irq, (void *)sc);
}

static int scv89601p_force_dpdm(struct scv89601p_chip *sc)
{
    int ret;
    int bc12_expire_time;
    int hvdcp_en;

    SCV89601P_ERR("scv89601p_force_dpdm\n");
    mutex_lock(&sc->bc_detect_lock);
    if (sc->bc12_detect) {
        SCV89601P_ERR("bc12_detect is true, return!\n");
        mutex_unlock(&sc->bc_detect_lock);
        return -EBUSY;
    }
    sc->bc12_detect = true;
    mutex_unlock(&sc->bc_detect_lock);
    ret = scv89601p_field_read(sc, F_HVDCP_EN, &hvdcp_en);
    /* HS07 code for HS07-223 by lina at 20250328 start */
    if (hvdcp_en)
        bc12_expire_time = 3400;
    else
        bc12_expire_time = 1300;
    /* HS07 code for HS07-223 by lina at 20250328 end */
    scv89601p_set_vindpm(sc, 0);
    schedule_delayed_work(&sc->bc12_timeout_dwork,
                            msecs_to_jiffies(bc12_expire_time));

    ret = scv89601p_field_write(sc, F_FORCE_INDET_EN, true);

    sc->power_good = 0;
    sc->dpdm_done_flag = 0;

    return ret;
}

static int scv89601p_get_charge_stat(struct scv89601p_chip *sc, int *stat)
{
    int ret = 0;
    int chg_stat, vbus_stat;
    /* HS07 code for HS07-2633 by lina at 20250507 start */
    static int s_psy_stat_old = POWER_SUPPLY_STATUS_UNKNOWN;
    /* HS07 code for HS07-2633 by lina at 20250507 end */

    ret = scv89601p_field_read(sc, F_CHG_STAT, &chg_stat);
    ret |= scv89601p_field_read(sc, F_VBUS_STAT, &vbus_stat);
    if (ret < 0)
        return ret;
    /*HS07 code for HS07-3330 by xiongxiaoliang at 20250522 start*/
    dev_info(sc->dev, "%s chg_stat=%d,vbus_stat=%d,chg_config=%d\n", __func__, chg_stat, vbus_stat, sc->chg_config);
    /*HS07 code for HS07-3330 by xiongxiaoliang at 20250522 end*/
    /*Tab A9 code for P230609-02043 by wenyaqi at 20230612 start*/
    if (vbus_stat == VBUS_STAT_OTG || sc->chg_type == POWER_SUPPLY_TYPE_UNKNOWN) {
    /*Tab A9 code for P230609-02043 by wenyaqi at 20230612 end*/
        *stat = POWER_SUPPLY_STATUS_DISCHARGING;
    } else {
        switch (chg_stat) {
        case CHG_STAT_NOT_CHARGE:
            if (sc->chg_config) {
                *stat = POWER_SUPPLY_STATUS_CHARGING;
            } else {
                *stat = POWER_SUPPLY_STATUS_NOT_CHARGING;
            }
            break;
        case CHG_STAT_PRE_CHARGE:
        case CHG_STAT_FAST_CHARGE_CCMODE:
        case CHG_STAT_FAST_CHARGE_CVMODE:
            *stat = POWER_SUPPLY_STATUS_CHARGING;
            break;
        case CHG_STAT_CHARGE_DONE:
            *stat = POWER_SUPPLY_STATUS_FULL;
            break;
        default:
            *stat = POWER_SUPPLY_STATUS_UNKNOWN;
            break;
        }
    }
    /* HS07 code for HS07-2633 by lina at 20250507 start */
    if (s_psy_stat_old == POWER_SUPPLY_STATUS_NOT_CHARGING &&
        *stat == POWER_SUPPLY_STATUS_CHARGING) {
        power_supply_changed(sc->psy);
    }
    s_psy_stat_old = *stat;
    /* HS07 code for HS07-2633 by lina at 20250507 end */

    return ret;
}

__maybe_unused
static int scv89601p_check_charge_done(struct scv89601p_chip *sc,
                                                    bool * chg_done)
{
    int ret, reg_val;

    ret = scv89601p_field_read(sc, F_CHG_STAT, &reg_val);
    if (ret < 0) {
        SCV89601P_ERR("read charge stat failed(%d)\n", ret);
        return ret;
    }

    *chg_done = (reg_val == CHG_STAT_CHARGE_DONE) ? true : false;
    return ret;
}

static bool scv89601p_detect_device(struct scv89601p_chip *sc)
{
    int ret;
    int val;

    ret = scv89601p_field_read(sc, F_PN, &val);
    if (ret < 0 || !(val == SC89601P_PN_NUM)) {
        SCV89601P_ERR("not find scv89601p, part_no = %d\n", val);
        return false;
    }

    sc->dev_id = val;

    return true;
}

static int scv89601p_dump_register(struct scv89601p_chip *sc)
{
    int ret;
    int i;
    uint8_t val;

    for (i = 0x04; i <= 0x2F; i++) {
        ret = scv89601p_i2c_read_byte(sc, i, &val);
        if (ret < 0) {
            return ret;
        }
        SCV89601P_INFO("%s reg[0x%02x] = 0x%02x\n", __func__, i, val);
    }

    for (i = 0x30; i <= 0x5F; i++) {
        ret = scv89601p_i2c_read_byte(sc, i, &val);
        if (ret < 0) {
            return ret;
        }
        SCV89601P_INFO("%s reg[0x%02x] = 0x%02x\n", __func__, i, val);
    }

    for (i = 0x90; i <= 0x9E; i++) {
        ret = scv89601p_i2c_read_byte(sc, i, &val);
        if (ret < 0) {
            return ret;
        }
        SCV89601P_INFO("%s reg[0x%02x] = 0x%02x\n", __func__, i, val);
    }

    return 0;
}

#ifdef CONFIG_MTK_CLASS
/********************MTK OPS***********************/
static int scv89601p_plug_in(struct charger_device *chg_dev)
{
    int ret = 0;
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    ret = scv89601p_set_chg_enable(sc, true);
    if (ret < 0) {
        pr_err("Failed to enable charging:%d\n", ret);
    }

    SCV89601P_INFO("%s\n", __func__);

    return ret;
}

static int scv89601p_plug_out(struct charger_device *chg_dev)
{
    int ret = 0;
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    ret = scv89601p_set_chg_enable(sc, false);
    if (ret < 0) {
        pr_err("Failed to disable charging:%d\n", ret);
    }

    SCV89601P_INFO("%s\n", __func__);

    return ret;
}

static int scv89601p_enable(struct charger_device *chg_dev, bool en)
{
    int ret = 0;
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    ret = scv89601p_set_chg_enable(sc, en);

    if (ret < 0)
        SCV89601P_ERR("fail to enable charger\n");
    else
        SCV89601P_INFO("success to %s charger\n", en? "enable" : "disable");
    /*for test*/
    /*scv89601p_dump_register(sc);*/
    /* HS07 code for SR-AL7761A-01-155 by lina at 20250317 start */
    sc->chg_config = en;
    /* HS07 code for SR-AL7761A-01-155 by lina at 20250317 end */
    return ret;
}

static int scv89601p_is_enabled(struct charger_device *chg_dev, bool * enabled)
{
    int ret;
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    ret = scv89601p_check_chg_enabled(sc, enabled);
    SCV89601P_INFO("charger is %s\n",
            *enabled ? "charging" : "not charging");

    return ret;
}

static int scv89601p_get_charging_current(struct charger_device *chg_dev,
                                        uint32_t * curr)
{
    int ret = 0;
    int curr_ma;
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    ret = scv89601p_get_ichg(sc, &curr_ma);
    if (ret >= 0) {
        *curr = curr_ma * 1000;
    }

    return ret;
}

static int scv89601p_set_charging_current(struct charger_device *chg_dev,
                                        uint32_t curr)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    SCV89601P_INFO("%s: charge curr = %duA\n", __func__, curr);

    return scv89601p_set_ichg(sc, curr / 1000);
}

static int scv89601p_get_input_current(struct charger_device *chg_dev, uint32_t *ua)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);
    int ma;
    int ret;

    SCV89601P_INFO("%s\n", __func__);

    ret = scv89601p_get_iindpm(sc, &ma);
    if (ret >= 0) {
        *ua = ma * 1000;
    }

    return ret;
}

static int scv89601p_set_input_current(struct charger_device *chg_dev, uint32_t curr)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    #if IS_ENABLED(CONFIG_CHARGER_CP_PPS)
    if (g_cp_charging_lmt.cp_chg_enable) {
        curr = 200000;
        pr_err("cp open icl curr = %d\n", curr);
    }
    #endif

    SCV89601P_INFO("%s: F_IINDPM curr = %duA\n", __func__, curr);

    return scv89601p_set_iindpm(sc, curr / 1000);
}

static int scv89601p_get_constant_voltage(struct charger_device *chg_dev,
                                        uint32_t *uv)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);
    int mv;
    int ret;

    SCV89601P_INFO("%s\n", __func__);

    ret = scv89601p_get_vbat(sc, &mv);
    if (ret >= 0) {
        *uv = mv * 1000;
    }

    return ret;
}

static int scv89601p_set_constant_voltage(struct charger_device *chg_dev,
                                        uint32_t volt)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    /*HS07 code for HS07-3925 by yexuedong at 20250605 start*/
    #if defined(CONFIG_CUSTOM_PROJECT_HS07)
    if (volt == 4450000) {
        volt = 4440000;
    }
    #endif
    /*HS07 code for HS07-3925 by yexuedong at 20250605 end*/

    SCV89601P_INFO("%s: charge volt = %duV\n", __func__, volt);

    return scv89601p_set_vbat(sc, volt / 1000);
}

static int scv89601p_kick_wdt(struct charger_device *chg_dev)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    SCV89601P_INFO("%s\n", __func__);
    return scv89601p_reset_wdt(sc);
}

static int scv89601p_set_ivl(struct charger_device *chg_dev, uint32_t volt)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    SCV89601P_INFO("%s: F_VINDPM volt = %d\n", __func__, volt);

    return scv89601p_set_vindpm(sc, volt / 1000);
}

/*HS07 code for HS07-2435 by xiongxiaoliang at 20250429 start*/
static int scv89601p_get_ivl(struct charger_device *chg_dev, uint32_t *volt)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);
    int volt_mv;
    int ret;

    ret = scv89601p_get_vindpm(sc, &volt_mv);
    if (ret < 0) {
        SCV89601P_INFO("%s: get vindpm fail, ret:%d\n", __func__, ret);
        return ret;
    }
    *volt = volt_mv * 1000;

    SCV89601P_INFO("%s: F_VINDPM volt = %d\n", __func__, *volt);

    return ret;
}
/*HS07 code for HS07-2435 by xiongxiaoliang at 20250429 end*/

static int scv89601p_is_charging_done(struct charger_device *chg_dev, bool * done)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);
    int ret;

    ret = scv89601p_check_charge_done(sc, done);

    SCV89601P_INFO("%s: charge %s done\n", __func__, *done ? "is" : "not");
    return ret;
}

static int scv89601p_get_min_ichg(struct charger_device *chg_dev, uint32_t * curr)
{
    //struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    *curr = 60 * 1000;
    SCV89601P_ERR("%s\n", __func__);
    return 0;
}

static int scv89601p_dump_registers(struct charger_device *chg_dev)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    SCV89601P_INFO("%s\n", __func__);

    return scv89601p_dump_register(sc);
}

__attribute__((unused)) static int scv89601p_check_otg_enabled(struct scv89601p_chip *sc, bool *enable)
{
    int ret, reg_val;

    ret = scv89601p_field_read(sc, F_BOOST_EN, &reg_val);
    if (ret < 0) {
        dev_err(sc->dev, "read otg enable failed(%d)\n", ret);
        return ret;
    }
    *enable = !!reg_val;

    return ret;
}

/* HST11 code for AX7800A-28 by zhangziyi at 20250408 start */
#if defined(CONFIG_CUSTOM_PROJECT_HS07)
/*HS07 code for HS07-46 by lina at 20250228 start*/
extern void usbpd_pm_set_otg_txmode(int enable);
/*HS07 code for HS07-46 by lina at 20250228 end*/
#endif
/*Tab A9 code for SR-AX6739A-01-487 by qiaodan at 20230515 start*/
static int scv89601p_enable_vbus(struct regulator_dev *rdev)
{
    struct scv89601p_chip *sc = charger_get_data(s_chg_dev_otg);
    int ret = 0;
    /* HST11 code for AX7800A-874 by wenyaqi at 20250522 start */
    bool hiz_enable = false;
    /* HST11 code for AX7800A-874 by wenyaqi at 20250522 end */

    pr_notice("%s enter\n", __func__);
    #if defined(CONFIG_CUSTOM_PROJECT_HS07)
    /*HS07 code for HS07-46 by lina at 20250228 start*/
    usbpd_pm_set_otg_txmode(true);
    /*HS07 code for HS07-46 by lina at 20250228 end*/
    #endif
    /*we should ensure that the powerpath is enabled before enable OTG*/
    /* HST11 code for AX7800A-874 by wenyaqi at 20250522 start */
    ret = scv89601p_get_dis_buck(sc, &hiz_enable);
    if (ret >= 0 && hiz_enable == true) {
    /* HST11 code for AX7800A-874 by wenyaqi at 20250522 end */
        SCV89601P_INFO("use otg func in dis buck mode, need exit dis buck\n");
        ret = scv89601p_set_dis_buck(sc, false);
        if (ret < 0) {
            SCV89601P_INFO("%s exit dis buck failed\n" ,__func__);
        }
    }

    ret |= scv89601p_set_otg_enable(sc, true);
    ret |= scv89601p_set_chg_enable(sc, false);

    return ret;
}
/*Tab A9 code for SR-AX6739A-01-487 by qiaodan at 20230515 end*/

static int scv89601p_disable_vbus(struct regulator_dev *rdev)
{
    struct scv89601p_chip *sc = charger_get_data(s_chg_dev_otg);
    int ret = 0;

    pr_notice("%s enter\n", __func__);
    #if defined(CONFIG_CUSTOM_PROJECT_HS07)
    /*HS07 code for HS07-46 by lina at 20250228 start*/
    usbpd_pm_set_otg_txmode(false);
    /*HS07 code for HS07-46 by lina at 20250228 end*/
    #endif
    ret = scv89601p_set_otg_enable(sc, false);
    ret |= scv89601p_set_chg_enable(sc, true);

    return ret;
}
/* HST11 code for AX7800A-28 by zhangziyi at 20250408 end */

static int scv89601p_is_enabled_vbus(struct regulator_dev *rdev)
{
    struct scv89601p_chip *sc = charger_get_data(s_chg_dev_otg);
    bool otg_en  = false;

    pr_notice("%s enter\n", __func__);

    scv89601p_check_otg_enabled(sc, &otg_en);

    return otg_en? 1 : 0;
}

static int scv89601p_set_otg(struct charger_device *chg_dev, bool enable)
{
    int ret;
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    ret = scv89601p_set_otg_enable(sc, enable);

    SCV89601P_INFO("%s OTG %s\n", enable ? "enable" : "disable",
            !ret ? "successfully" : "failed");

    return ret;
}

static int scv89601p_set_safety_timer(struct charger_device *chg_dev, bool enable)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    SCV89601P_INFO("%s  %s\n", __func__, enable ? "enable" : "disable");

    return scv89601p_set_safet_timer(sc, enable);
}

static int scv89601p_is_safety_timer_enabled(struct charger_device *chg_dev,
                                        bool * enabled)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    return scv89601p_check_safet_timer(sc, enabled);
}

static int scv89601p_set_boost_ilmt(struct charger_device *chg_dev, uint32_t curr)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    SCV89601P_INFO("%s otg curr = %d\n", __func__, curr);
    return scv89601p_set_iboost(sc, curr / 1000);
}

static int scv89601p_do_event(struct charger_device *chg_dev, uint32_t event, uint32_t args)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);
    SCV89601P_INFO("%s\n", __func__);

#ifdef CONFIG_MTK_CHARGER_V4P19
    switch (event) {
    case EVENT_EOC:
        charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_EOC);
    break;
    case EVENT_RECHARGE:
        charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_RECHG);
    break;
    default:
    break;
    }
#else
    switch (event) {
    case EVENT_FULL:
    case EVENT_RECHARGE:
    case EVENT_DISCHARGE:
        power_supply_changed(sc->psy);
        break;
    default:
        break;
    }
#endif /*CONFIG_MTK_CHARGER_V4P19*/

    return 0;
}

static int scv89601p_enable_hz(struct charger_device *chg_dev, bool enable)
{
    struct scv89601p_chip *sc = dev_get_drvdata(&chg_dev->dev);

    SCV89601P_INFO("%s %s\n", __func__, enable ? "enable" : "disable");

    /* HST11 code for AX7800A-31 by zhangziyi at 20250415 start */
    return scv89601p_set_dis_buck(sc, enable);
    /* HST11 code for AX7800A-31 by zhangziyi at 20250415 end */
}

/*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 start*/
static int scv89601p_get_hvdcp_status(struct charger_device *chg_dev)
{
    int ret = 0;
    struct scv89601p_chip *sc = charger_get_data(chg_dev);

    if (sc == NULL) {
        pr_info("[%s] scv89601p_get_hvdcp_status fail\n", __func__);
        return ret;
    }

    ret = (sc->hvdcp_done ? 1 : 0);

    return ret;
}
/*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 end*/

static struct regulator_ops scv89601p_vbus_ops = {
    .enable = scv89601p_enable_vbus,
    .disable = scv89601p_disable_vbus,
    .is_enabled = scv89601p_is_enabled_vbus,
};

static const struct regulator_desc scv89601p_otg_rdesc = {
    .of_match = "usb-otg-vbus",
    .name = "usb-otg-vbus",
    .ops = &scv89601p_vbus_ops,
    .owner = THIS_MODULE,
    .type = REGULATOR_VOLTAGE,
    .fixed_uV = 5000000,
    .n_voltages = 1,
};

static int scv89601p_vbus_regulator_register(struct scv89601p_chip *sc)
{
    struct regulator_config config = {};
    int ret = 0;
    /* otg regulator */
    config.dev = sc->dev;
    config.driver_data = sc;
    sc->otg_rdev = devm_regulator_register(sc->dev,
                        &scv89601p_otg_rdesc, &config);
    sc->otg_rdev->constraints->valid_ops_mask |= REGULATOR_CHANGE_STATUS;
    if (IS_ERR(sc->otg_rdev)) {
        ret = PTR_ERR(sc->otg_rdev);
        pr_info("%s: register otg regulator failed (%d)\n", __func__, ret);
    }
    return ret;
}

static struct charger_ops scv89601p_chg_ops = {
    /* Normal charging */
    .plug_in = scv89601p_plug_in,
    .plug_out = scv89601p_plug_out,
    .enable = scv89601p_enable,
    .is_enabled = scv89601p_is_enabled,
    .get_charging_current = scv89601p_get_charging_current,
    .set_charging_current = scv89601p_set_charging_current,
    .get_input_current = scv89601p_get_input_current,
    .set_input_current = scv89601p_set_input_current,
    .get_constant_voltage = scv89601p_get_constant_voltage,
    .set_constant_voltage = scv89601p_set_constant_voltage,
    .kick_wdt = scv89601p_kick_wdt,
    .set_mivr = scv89601p_set_ivl,
    /*HS07 code for HS07-2435 by xiongxiaoliang at 20250429 start*/
    .get_mivr = scv89601p_get_ivl,
    /*HS07 code for HS07-2435 by xiongxiaoliang at 20250429 end*/
    .is_charging_done = scv89601p_is_charging_done,
    .get_min_charging_current = scv89601p_get_min_ichg,
    .dump_registers = scv89601p_dump_registers,

    /* Safety timer */
    .enable_safety_timer = scv89601p_set_safety_timer,
    .is_safety_timer_enabled = scv89601p_is_safety_timer_enabled,

    /* Power path */
    //.enable_powerpath = scv89601p_enable_powerpath,
    //.is_powerpath_enabled = scv89601p_is_powerpath_enabled,

    /* OTG */
    .enable_otg = scv89601p_set_otg,
    .set_boost_current_limit = scv89601p_set_boost_ilmt,
    .enable_discharge = NULL,

    /* ADC */
    //.get_adc = scv89601p_get_adc,
    //.get_vbus_adc = scv89601p_get_vbus,
    //.get_ibus_adc = scv89601p_get_ibus,
    //.get_ibat_adc = scv89601p_get_ibat,

    .event = scv89601p_do_event,
    .enable_hz = scv89601p_enable_hz,
    /*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 start*/
    .get_hvdcp_status = scv89601p_get_hvdcp_status,
    /*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 end*/
    /* HS07 code for SR-AL7761A-01-85 by lina at 20250317 start */
    .get_ship_mode = scv89601p_get_shipmode,
    .set_ship_mode = scv89601p_set_shipmode,
    /* HS07 code for SR-AL7761A-01-85 by lina at 20250317 end */
    /* HS07 code for SR-AL7761A-01-155 by lina at 20250317 start */
    .enable_port_charging = scv89601p_enable_port_charging,
    .is_port_charging_enabled= scv89601p_is_port_charging_enabled,
    /* HS07 code for SR-AL7761A-01-155 by lina at 20250317 end */
    /* HS07 code for HS07-1480 by lina at 20250425 start */
    #if IS_ENABLED(CONFIG_CUSTOM_USBIF)
    .set_vindpm = scv89601p_set_vindpm_pd,
    #endif//CONFIG_CUSTOM_USBIF
    /* HS07 code for HS07-1480 by lina at 20250425 end */
};

static const struct charger_properties scv89601p_chg_props = {
    .alias_name = "scv89601p_chg",
};

#ifdef CONFIG_MTK_CHARGER_V4P19
static void SCV89601P_INFOrm_psy_dwork_handler(struct work_struct *work)
{
    int ret = 0;
    union power_supply_propval propval;
    struct scv89601p_chip *sc = container_of(work,
                                        struct scv89601p_chip,
                                        psy_dwork.work);
    if (!sc->chg_psy) {
        sc->chg_psy = power_supply_get_by_name("charger");
        if (!sc->chg_psy) {
            pr_err("%s get power supply fail\n", __func__);
            mod_delayed_work(system_wq, &sc->psy_dwork,
                    msecs_to_jiffies(2000));
            return ;
        }
    }

    if (sc->adp_type != CHARGER_UNKNOWN)
        propval.intval = 1;
    else
        propval.intval = 0;

    ret = power_supply_set_property(sc->chg_psy, POWER_SUPPLY_PROP_ONLINE,
                    &propval);

    if (ret < 0)
        pr_notice("inform power supply online failed:%d\n", ret);

    propval.intval = sc->adp_type;

    ret = power_supply_set_property(sc->chg_psy,
                    POWER_SUPPLY_PROP_CHARGE_TYPE,
                    &propval);
    if (ret < 0)
        pr_notice("inform power supply charge type failed:%d\n", ret);

}
#endif /*CONFIG_MTK_CHARGER_V4P19*/

#endif /*CONFIG_MTK_CLASS */

static int scv89601p_chg_attach_pre_process(struct scv89601p_chip *sc, int attach)
{
    sc->typec_attached = attach;
    /*HS07 code for HS07-3571 by yexuedong at 20250530 start*/
    #if IS_ENABLED(CONFIG_CUSTOM_PROJECT_HS07)
    if((attach == ATTACH_TYPE_PD_DCP) || (attach == ATTACH_TYPE_PD_NONSTD)) {
        usbpd_setvacovp(0);
    }
    #endif
    /*HS07 code for HS07-3571 by yexuedong at 20250530 end*/
    switch(attach) {
        case ATTACH_TYPE_TYPEC:
            /*Tab A9 code for AX6739A-2077 | AX6739A-2728 by wenyaqi at 20230906 start*/
            schedule_delayed_work(&sc->force_detect_dwork, msecs_to_jiffies(150));
            /*Tab A9 code for AX6739A-2077 | AX6739A-2728 by wenyaqi at 20230906 end*/
            break;
        case ATTACH_TYPE_PD_SDP:
            sc->chg_type = POWER_SUPPLY_TYPE_USB;
            sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_SDP;
            break;
        case ATTACH_TYPE_PD_DCP:
            sc->chg_type = POWER_SUPPLY_TYPE_USB_DCP;
            sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_DCP;
            break;
        case ATTACH_TYPE_PD_NONSTD:
            sc->chg_type = POWER_SUPPLY_TYPE_USB;
            sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_DCP;
            break;
        case ATTACH_TYPE_NONE:
            sc->chg_type = POWER_SUPPLY_TYPE_UNKNOWN;
            sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
            break;
        default:
            dev_info(sc->dev, "%s: using tradtional bc12 flow!\n", __func__);
            break;
    }
    /* HS07 code for HS07-1480 by lina at 20250425 start */
    #if IS_ENABLED(CONFIG_CUSTOM_USBIF)
    if (sc->bootmode == 0) {
        if (attach == ATTACH_TYPE_NONE) {
            sc->is_first_plugin = false;
        } else {
            sc->is_first_plugin = true;
        }
    }
    pr_err("is_first_plugin:%d\n", sc->is_first_plugin);
    #endif//CONFIG_CUSTOM_USBIF
    //* HS07 code for HS07-1480 by lina at 20250425 end */

    power_supply_changed(sc->psy);
    dev_err(sc->dev, "%s: type(%d %d),attach:%d\n", __func__,
        sc->chg_type, sc->psy_usb_type, attach);

    return 0;
}

/**********************interrupt*********************/
static void scv89601p_force_detection_dwork_handler(struct work_struct *work)
{
    int ret;
    struct scv89601p_chip *sc = container_of(work,
                                        struct scv89601p_chip,
                                        force_detect_dwork.work);

    /*Tab A9 code for AX6739A-2077 | AX6739A-2728 by wenyaqi at 20230906 start*/
    if ((sc->vbus_good == false) && (sc->typec_attached == ATTACH_TYPE_TYPEC)) {
        dev_err(sc->dev, "%s: wait vbus to be good\n", __func__);
        schedule_delayed_work(&sc->force_detect_dwork, msecs_to_jiffies(10));
        return;
    } else if (sc->vbus_good == false) {
        dev_err(sc->dev, "%s: TypeC has been plug out\n", __func__);
        return;
    }

    msleep(100);
    /*Tab A9 code for AX6739A-2077 | AX6739A-2728 by wenyaqi at 20230906 end*/
    ret = scv89601p_force_dpdm(sc);
    /* HS07 code for SR-AL7761A-01-155 by lina at 20250317 start */
    sc->force_dpdm = true;
    /* HS07 code for SR-AL7761A-01-155 by lina at 20250317 end */
    if (ret < 0) {
        SCV89601P_ERR("%s: force dpdm failed(%d)\n", __func__, ret);
        return;
    }

    sc->force_detect_count++;
}

static int scv89601p_get_charger_type(struct scv89601p_chip *sc)
{
    int ret = 0;
    int reg_val = 0;
    if (sc->typec_attached > ATTACH_TYPE_NONE && sc->vbus_good) {
        switch(sc->typec_attached) {
            case ATTACH_TYPE_PD_SDP:
            case ATTACH_TYPE_PD_DCP:
            case ATTACH_TYPE_PD_NONSTD:
                dev_info(sc->dev, "%s: Attach PD_TYPE, skip bc12 flow!\n", __func__);
                return ret;
            default:
                break;
        }
    }
    if(sc->dpdm_done_flag == 0) {
        sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
        sc->chg_type = POWER_SUPPLY_TYPE_UNKNOWN;
        return ret;
    }

    ret = scv89601p_field_read(sc, F_VBUS_STAT, &reg_val);
    if (ret < 0) {
        return ret;
    }

    switch (reg_val) {
    case VBUS_STAT_NO_INPUT:
        sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
        sc->chg_type = POWER_SUPPLY_TYPE_UNKNOWN;
        pr_info("charger type: none\n");
        break;
    case VBUS_STAT_SDP:
        sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_SDP;
        sc->chg_type = POWER_SUPPLY_TYPE_USB;
        /*HS07 code for HS07-3571 by yexuedong at 20250530 start*/
        #if IS_ENABLED(CONFIG_CUSTOM_PROJECT_HS07)
        usbpd_setvacovp(1);
        #endif
        /*HS07 code for HS07-3571 by yexuedong at 20250530 end*/
        pr_info("charger type: SDP\n");
        break;
    case VBUS_STAT_CDP:
        sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_CDP;
        sc->chg_type = POWER_SUPPLY_TYPE_USB_CDP;
        pr_info("charger type: CDP\n");
        break;
    case VBUS_STAT_DCP:
    case VBUS_STAT_HVDCP:
        sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_DCP;
        sc->chg_type = POWER_SUPPLY_TYPE_USB_DCP;
        pr_info("charger type: DCP\n");
        break;
    case VBUS_STAT_UNKOWN:
        sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_SDP;
        sc->chg_type = POWER_SUPPLY_TYPE_USB;
        /*HS07 code for HS07-3571 by yexuedong at 20250530 start*/
        #if IS_ENABLED(CONFIG_CUSTOM_PROJECT_HS07)
        usbpd_setvacovp(1);
        #endif
        /*HS07 code for HS07-3571 by yexuedong at 20250530 end*/
        pr_info("charger type: unknown adapter\n");
        if (sc->force_detect_count < 10) {
            schedule_delayed_work(&sc->force_detect_dwork,
                                msecs_to_jiffies(2000));
        }
        break;
    case VBUS_STAT_NONSTAND:
        sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_DCP;
        sc->chg_type = POWER_SUPPLY_TYPE_USB_DCP;
        pr_info("charger type: non-std charger\n");
        break;
    default:
        sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
        sc->chg_type = POWER_SUPPLY_TYPE_UNKNOWN;
        pr_info("charger type: invalid value\n");
        break;
    }
    /* HS07 code for SR-AL7761A-01-155|HS07-184 by lina at 20250319 start */
    /*HS07 code for HS07-793 by xiongxiaoliang at 20250417 start*/
    if (sc->chg_type == POWER_SUPPLY_TYPE_USB_DCP && sc->hvdcp_done != true) {
    /*HS07 code for HS07-793 by xiongxiaoliang at 20250417 end*/
    /* HS07 code for SR-AL7761A-01-155|HS07-184 by lina at 20250319 end */
        ret = scv89601p_set_soft_hvdcp(sc);
        if (ret < 0) {
            pr_err("Failed to set hvdcp, ret = %d\n", ret);
        }
    }

    power_supply_changed(sc->psy);
    SCV89601P_INFO("%s vbus stat: 0x%02x\n", __func__, reg_val);

    return ret;
}

static void scv89601p_inserted_irq(struct scv89601p_chip *sc)
{
    SCV89601P_INFO("%s: adapter/usb inserted\n", __func__);

    /*HS07 code for HS07-3477 by xiongxiaoliang at 20250526 start*/
    scv89601p_set_vindpm_track(sc, SCV89601P_TRACK_DIS);
    /*HS07 code for HS07-3477 by xiongxiaoliang at 20250526 end*/
    scv89601p_set_vindpm(sc, 4600);
    sc->force_detect_count = 0;
    scv89601p_field_write(sc, F_ADC_RATE, true);
    /* HS07 code for HS07-1480 by lina at 20250425 start */
    #if IS_ENABLED(CONFIG_CUSTOM_USBIF)
    if (sc->bootmode == 0) {
        scv89601p_set_input_volt_limit_plugin(sc);
    }
    #endif//CONFIG_CUSTOM_USBIF
    /* HS07 code for HS07-1480 by lina at 20250425 end */
}

static void scv89601p_removed_irq(struct scv89601p_chip *sc)
{
    SCV89601P_INFO("%s: adapter/usb removed\n", __func__);
    cancel_delayed_work_sync(&sc->force_detect_dwork);
    cancel_delayed_work_sync(&sc->bc12_timeout_dwork);
    /*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 start*/
    cancel_delayed_work_sync(&sc->hvdcp_done_work);
    sc->hvdcp_done = false;
    /*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 end*/

    mutex_lock(&sc->bc_detect_lock);
    sc->bc12_detect = false;
    sc->bc12_recovery = false;
    mutex_unlock(&sc->bc_detect_lock);
    scv89601p_set_dpdm_hiz(sc);
    scv89601p_set_iindpm(sc, 500);
    scv89601p_set_ichg(sc, 500);
    scv89601p_field_write(sc, F_ADC_RATE, false);
    sc->dpdm_done_flag = 0;
    /*HS07 code for HS07-3571 by yexuedong at 20250530 start*/
    #if IS_ENABLED(CONFIG_CUSTOM_PROJECT_HS07)
    usbpd_setvacovp(0);
    #endif
    /*HS07 code for HS07-3571 by yexuedong at 20250530 end*/
}

/*HS07 code for HS07-188 by lina at 20250320 start*/
static irqreturn_t scv89601p_irq_handler(int irq, void *data)
{
    int ret;
    int reg_val;
    bool prev_vbus_gd;
    int true_power_good = 0;
    struct scv89601p_chip *sc = (struct scv89601p_chip *)data;
    int vbus_present_stat;
    int otg_stat;

    SCV89601P_INFO("%s: scv89601p_irq_handler\n", __func__);
    if (sc->hiz_cut_flag) {
        SCV89601P_INFO("%s: scv89601p in the process of disable hiz\n", __func__);
        return IRQ_HANDLED;
    }

    //ret = scv89601p_field_read(sc, F_VBUS_GOOD_STAT, &reg_val);
    ret = scv89601p_field_read(sc, F_VBUS_PRESENT_STAT, &vbus_present_stat);
    ret |= scv89601p_field_read(sc, F_BOOST_EN, &otg_stat);
    if (ret < 0) {
        goto out;
    }
    prev_vbus_gd = sc->vbus_good;
    SCV89601P_INFO("%s: vbus_present = %d otg_stat = %d\n", __func__, vbus_present_stat, otg_stat);
    reg_val = otg_stat ? 0 : vbus_present_stat;
    sc->vbus_good = !!reg_val;

    if (!prev_vbus_gd && sc->vbus_good) {
        /*HS07 code for HS07-3330 by xiongxiaoliang at 20250522 start*/
        #if !defined(CONFIG_ODM_CUSTOM_FACTORY_BUILD)
        sc->chg_config = !gxy_check_batt_protection_status();
        #endif
        /*HS07 code for HS07-3330 by xiongxiaoliang at 20250522 end*/
        scv89601p_inserted_irq(sc);
    } else if (prev_vbus_gd && !sc->vbus_good) {
        scv89601p_removed_irq(sc);
        /* HS07 code for SR-AL7761A-01-155 by lina at 20250317 start */
        sc->force_dpdm = false;
        sc->is_recharging = false;
        /* HS07 code for SR-AL7761A-01-155 by lina at 20250317 end */
    /*HS07 code for P250512-06665 by jiashixian at 20250513 start*/
    } else if (prev_vbus_gd && sc->vbus_good) {
        #if IS_ENABLED(CONFIG_CUSTOM_USBIF)
        if (sc->bootmode == 0) {
            sc->is_first_plugin = false;
        }
        #endif
    }
    /*HS07 code for P250512-06665 by jiashixian at 20250513 end*/
    ret = scv89601p_field_read(sc, F_INPUT_DET_DONE_FLAG, &true_power_good);
    if (ret < 0) {
        goto out;
    }
    /* HS07 code for HS07-223 by lina at 20250328 start */
    true_power_good = sc->dpdm_done_flag;
    /* HS07 code for HS07-223 by lina at 20250328 start */

    SCV89601P_INFO("%s: true_power_good:%d, pre_pg:%d\n", __func__, true_power_good, sc->power_good);
    if (!sc->power_good && true_power_good) {
        cancel_delayed_work_sync(&sc->bc12_timeout_dwork);
        scv89601p_set_iindpm(sc, 500);
        mutex_lock(&sc->bc_detect_lock);
        sc->bc12_detect = false;
        sc->bc12_recovery = false;
        mutex_unlock(&sc->bc_detect_lock);
        scv89601p_set_vindpm(sc, sc->vindpm_value);
    }

    sc->power_good = true_power_good;
    /* HS07 code for SR-AL7761A-01-155 by lina at 20250317 start */
    if ((sc->force_dpdm == true || sc->is_recharging == true) && (sc->dpdm_done_flag == true)) {
        pr_notice("start get chgtype!\n");
        scv89601p_get_charger_type(sc);
        sc->force_dpdm = false;
        sc->is_recharging = false;
    }
    /* HS07 code for SR-AL7761A-01-155 by lina at 20250317 end */

    scv89601p_dump_register(sc);
out:
    return IRQ_HANDLED;
}
/*HS07 code for HS07-188 by lina at 20250320 end*/

/*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 start*/
static void hvdcp_done_work_func(struct work_struct *work)
{
    struct delayed_work *hvdcp_done_work = NULL;
    struct scv89601p_chip *sc = NULL;

    hvdcp_done_work = container_of(work, struct delayed_work, work);
    if (hvdcp_done_work == NULL) {
        pr_err("Cann't get hvdcp_done_work\n");
        return;
    }

    sc = container_of(hvdcp_done_work, struct scv89601p_chip,
                      hvdcp_done_work);
    if (sc == NULL) {
        pr_err("Cann't get scv89601p_chip\n");
        return;
    }

    sc->hvdcp_done = true;
    power_supply_changed(sc->psy);
    pr_info("%s HVDCP end\n", __func__);
}
/*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 end*/

/**********************system*********************/
static int scv89601p_parse_dt(struct scv89601p_chip *sc)
{
    struct device_node *np = sc->dev->of_node;
    int i;
    int ret = 0;
    /* HS07 code for HS07-1480 by lina at 20250425 start */
    #if IS_ENABLED(CONFIG_CUSTOM_USBIF)
    struct device_node *boot_node = NULL;
    struct sc_tag_bootmode *tag = NULL;
    #endif//CONFIG_CUSTOM_USBIF
    /* HS07 code for HS07-1480 by lina at 20250425 end */
    struct {
        char *name;
        int *conv_data;
    } props[] = {
        {"scv89601p,pfm-dis", &(sc->cfg->pfm_dis)},
        {"scv89601p,vsys-min", &(sc->cfg->vsys_min)},
        {"scv89601p,vbat-low-otg", &(sc->cfg->vbat_low_otg)},
        {"scv89601p,ipre-chg", &(sc->cfg->pre_chg_curr)},
        {"scv89601p,itrickle", &(sc->cfg->itrickle)},
        {"scv89601p,itermination", &(sc->cfg->term_chg_curr)},
        {"scv89601p,vbat-volt", &(sc->cfg->vbat_cv)},
        {"scv89601p,vrechg-volt", &(sc->cfg->vrechg_volt)},
        {"scv89601p,en-termination", &(sc->cfg->term_en)},
        {"scv89601p,hvdcp-en", &(sc->cfg->hvdcp_en)},
        {"scv89601p,wdt-timer", &(sc->cfg->wdt_timer)},
        {"scv89601p,en-safety-timer", &(sc->cfg->en_chg_saft_timer)},
        {"scv89601p,charge-timer", &(sc->cfg->charge_timer)},
        {"scv89601p,vac-ovp", &(sc->cfg->vac_ovp)},
        {"scv89601p,iboost", &(sc->cfg->iboost)},
        {"scv89601p,vboost", &(sc->cfg->vboost)},
        {"scv89601p,vindpm-track", &(sc->cfg->vindpm_track)},
        {"scv89601p,vindpm-int-mask", &(sc->cfg->vindpm_int_mask)},
        {"scv89601p,iindpm-int-mask", &(sc->cfg->iindpm_int_mask)},
        {"scv89601p,auto-dpdm-en", &(sc->cfg->auto_dpdm_en)},
        {"scv89601p,iindpm-update-dis", &(sc->cfg->iindpm_update_dis)}};

    /* HS07 code for HS07-1480 by lina at 20250425 start */
    #if IS_ENABLED(CONFIG_CUSTOM_USBIF)
    boot_node = of_find_node_by_path("/chosen");
    if (!boot_node) {
        boot_node = of_find_node_by_path("/chosen@0");
    }
    if (boot_node) {
        tag = (struct sc_tag_bootmode *)of_get_property(boot_node,
                                "atag,boot", NULL);
        if (!tag)
            pr_err("%s: failed to get atag,boot\n", __func__);
        else {
            pr_err("%s: size:0x%x tag:0x%x bootmode:0x%x boottype:0x%x\n",
                             __func__, tag->size, tag->tag, tag->bootmode, tag->boottype);
            sc->bootmode = tag->bootmode;
        }
    } else {
        pr_err("%s: failed to get /chosen and /chosen@0\n", __func__);
    }
    #endif//CONFIG_CUSTOM_USBIF
    /* HS07 code for HS07-1480 by lina at 20250425 end */

    sc->irq_gpio = of_get_named_gpio(np, "scv89601p,intr-gpio", 0);
    if (sc->irq_gpio < 0)
        SCV89601P_ERR("%s scv89601p,intr-gpio is not available\n", __func__);

    if (of_property_read_string(np, "scv89601p,charger_name", &sc->cfg->chg_name) < 0)
        SCV89601P_ERR("%s no charger name\n", __func__);

    /* initialize data for optional properties */
    for (i = 0; i < ARRAY_SIZE(props); i++) {
        ret = of_property_read_u32(np, props[i].name, props[i].conv_data);
        if (ret < 0) {
            SCV89601P_ERR("%s not find\n", props[i].name);
            continue;
        }
    }

    return 0;
}

static int scv89601p_init_device(struct scv89601p_chip *sc) {
    int ret = 0;
    int i;
    struct {
        enum scv89601p_fields field_id;
        int conv_data;
    } props[] = {
        {F_PFM_DIS, sc->cfg->pfm_dis},
        {F_SYS_MIN, sc->cfg->vsys_min},
        {F_VBAT_LOW_OTG, sc->cfg->vbat_low_otg},
        {F_IPRECHG, sc->cfg->pre_chg_curr},
        {F_TRICKLE_CURRENT, sc->cfg->itrickle},
        {F_ITERM, sc->cfg->term_chg_curr},
        {F_VBAT_REG, sc->cfg->vbat_cv},
        {F_VRECHG, sc->cfg->vrechg_volt},
        {F_TERM_EN, sc->cfg->term_en},
        {F_HVDCP_EN, sc->cfg->hvdcp_en},
        {F_WD_TIMEOUT, sc->cfg->wdt_timer},
        {F_CHG_TIMER_EN, sc->cfg->en_chg_saft_timer},
        {F_CHG_TIMER, sc->cfg->charge_timer},
        {F_VBUS_OVP, sc->cfg->vac_ovp},
        {F_IBOOST_LIMIT, sc->cfg->iboost},
        {F_VBOOST, sc->cfg->vboost},
        {F_VINDPM_TRACK, sc->cfg->vindpm_track},
        {F_VINDPM_MASK, sc->cfg->vindpm_int_mask},
        {F_IINDPM_MASK, sc->cfg->iindpm_int_mask},
        {F_AUTO_INDET_EN, sc->cfg->auto_dpdm_en},
        {F_IINDPM_UPDATE_DIS, sc->cfg->iindpm_update_dis},};

    //reg reset;
    scv89601p_field_write(sc, F_REG_RST, 1);
    msleep(10);

    for (i = 0; i < ARRAY_SIZE(props); i++) {
        if (props[i].field_id == F_VBOOST) {
            scv89601p_set_vboost(sc, sc->cfg->vboost);
        } else {
            SCV89601P_INFO("%d--->%d\n", props[i].field_id, props[i].conv_data);
            ret = scv89601p_field_write(sc, props[i].field_id, props[i].conv_data);
        }
    }

    scv89601p_set_iindpm(sc, 500);
    scv89601p_set_ichg(sc, 500);
    scv89601p_set_adc_conv_rate(sc, 1);
    scv89601p_hk_buck_irq_mask(sc);
    /*HS07 code for HS07-432 by lina at 20250409 start*/
    ret = scv89601p_set_safet_timer(sc, false);
    if (ret < 0) {
        dev_err(sc->dev, "Failed to set safety_timer stop\n");
    }
    /*HS07 code for HS07-432 by lina at 20250409 end*/

    /* HST11 code for AX7800A-625 by zhangziyi at 20250429 start */
    ret = scv89601p_ocp_irq_mask(sc);
    if (ret < 0) {
        dev_err(sc->dev, "Failed to mask ocp irq\n");
    }
    /* HST11 code for AX7800A-625 by zhangziyi at 20250429 end */

    /*HS07 code for HS07-4586 by lina at 20250616 start*/
    scv89601p_batfet_rst_en(sc,false);
    if (ret < 0) {
        dev_err(sc->dev, "Failed to set BATFET_RST_EN fail\n");
    }
    /*HS07 code for HS07-4586 by lina at 20250616 end*/

    return scv89601p_dump_register(sc);
}

static int scv89601p_register_interrupt(struct scv89601p_chip *sc)
{
    int ret = 0;

    ret = devm_gpio_request(sc->dev, sc->irq_gpio, "chr-irq");
    if (ret < 0) {
        SCV89601P_ERR("failed to request GPIO%d ; ret = %d", sc->irq_gpio, ret);
        return ret;
    }

    ret = gpio_direction_input(sc->irq_gpio);
    if (ret < 0) {
        SCV89601P_ERR("failed to set GPIO%d ; ret = %d", sc->irq_gpio, ret);
        return ret;
    }

    sc->irq = gpio_to_irq(sc->irq_gpio);
    if (ret < 0) {
        SCV89601P_ERR("failed gpio to irq GPIO%d ; ret = %d", sc->irq_gpio, ret);
        return ret;
    }

    ret = devm_request_threaded_irq(sc->dev, sc->irq, NULL,
                                    scv89601p_irq_handler,
                                    IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                                    "chr_irq", sc);
    if (ret < 0) {
        SCV89601P_ERR("request thread irq failed:%d\n", ret);
        return ret;
    } else {
        SCV89601P_ERR("request thread irq pass:%d  sc->irq =%d\n", ret, sc->irq);
    }

    enable_irq_wake(sc->irq);

    return 0;
}

static void determine_initial_status(struct scv89601p_chip *sc)
{
    scv89601p_irq_handler(sc->irq, (void *)sc);
}

//reigster
static ssize_t scv89601p_show_registers(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
    struct scv89601p_chip *sc = dev_get_drvdata(dev);
    uint8_t addr;
    uint8_t val;
    uint8_t tmpbuf[300];
    int len;
    int idx = 0;
    int ret;

    idx = snprintf(buf, PAGE_SIZE, "%s:\n", "scv89601p");
    for (addr = 0x0; addr <= 0x9E; addr++) {
        ret = scv89601p_i2c_read_byte(sc, addr, &val);
        if (ret == 0) {
            len = snprintf(tmpbuf, PAGE_SIZE - idx,
                        "Reg[%.2X] = 0x%.2x\n", addr, val);
            memcpy(&buf[idx], tmpbuf, len);
            idx += len;
        }
    }

    return idx;
}

static ssize_t scv89601p_store_register(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count) {
    struct scv89601p_chip *sc = dev_get_drvdata(dev);
    int ret;
    uint8_t val;
    uint8_t reg;
    ret = sscanf(buf, "%hhu %hhu", &reg, &val);
    if (ret == 2 && reg <= 0x9E)
        scv89601p_i2c_write_byte(sc, reg, val);

    return count;
}

static ssize_t scv89601p_test_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    struct scv89601p_chip *sc = dev_get_drvdata(dev);
    int ret;
    int val;
    ret = sscanf(buf, "%d", &val);
    if (ret < 0) {
        dev_err(dev, "get parameters fail\n");
        return -EINVAL;
    }

    switch (val) {
    case 1:
        scv89601p_dump_register(sc);
        scv89601p_set_hiz(sc, true);
        break;
    case 2:
        scv89601p_dump_register(sc);
        scv89601p_set_dis_buck(sc, true);
        break;
    case 3:
        scv89601p_dump_register(sc);
        scv89601p_set_hiz(sc, false);
        break;
    case 4:
        scv89601p_dump_register(sc);
        scv89601p_set_dis_buck(sc, false);
        break;
    case 5:
        scv89601p_set_otg_enable(sc, true);
        break;
    case 6:
        scv89601p_set_otg_enable(sc, false);
        break;
    default:
        break;
    }

    return count;
}

static DEVICE_ATTR(registers, 0660, scv89601p_show_registers,
                scv89601p_store_register);
static DEVICE_ATTR(scv89601p_test, 0660, NULL,
                scv89601p_test_store);

static void scv89601p_create_device_node(struct device *dev) {
    device_create_file(dev, &dev_attr_registers);
    device_create_file(dev, &dev_attr_scv89601p_test);
}

static enum power_supply_property scv89601p_chg_psy_properties[] = {
    POWER_SUPPLY_PROP_MANUFACTURER,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
    POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
    POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT,
    POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
    POWER_SUPPLY_PROP_USB_TYPE,
    POWER_SUPPLY_PROP_CURRENT_MAX,
    POWER_SUPPLY_PROP_VOLTAGE_MAX,
};

static enum power_supply_usb_type scv89601p_chg_psy_usb_types[] = {
    POWER_SUPPLY_USB_TYPE_UNKNOWN,
    POWER_SUPPLY_USB_TYPE_SDP,
    POWER_SUPPLY_USB_TYPE_CDP,
    POWER_SUPPLY_USB_TYPE_DCP,
};

static int scv89601p_chg_property_is_writeable(struct power_supply *psy,
                                            enum power_supply_property psp) {
    int ret;

    switch (psp) {
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
    case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
    case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
    case POWER_SUPPLY_PROP_STATUS:
    case POWER_SUPPLY_PROP_ONLINE:
    case POWER_SUPPLY_PROP_ENERGY_EMPTY:
        ret = 1;
        break;

    default:
        ret = 0;
    }

    return ret;
}

static int scv89601p_chg_get_property(struct power_supply *psy,
                                    enum power_supply_property psp,
                                    union power_supply_propval *val) {
    struct scv89601p_chip *sc = power_supply_get_drvdata(psy);
    int ret = 0;
    int data = 0;

    if (!sc) {
        SCV89601P_ERR("%s:line%d: NULL pointer!!!\n", __func__, __LINE__);
        return -EINVAL;
    }

    switch (psp) {
    case POWER_SUPPLY_PROP_MANUFACTURER:
        val->strval = "SouthChip";
        break;
    case POWER_SUPPLY_PROP_ONLINE:
        /*Tab A9 code for AX6739A-516 | P230609-02043 | OT11BSP-42 by qiaodan at 20230613 start*/
        val->intval = sc->vbus_good | (!!sc->typec_attached);
        if (sc->chg_type == POWER_SUPPLY_TYPE_UNKNOWN) {
            val->intval = 0;
        }
        /*Tab A9 code for AX6739A-516 | P230609-02043 | OT11BSP-42 by qiaodan at 20230613 end*/
        break;
    case POWER_SUPPLY_PROP_STATUS:
        ret = scv89601p_get_charge_stat(sc, &val->intval);
        if (ret < 0)
            break;
        break;
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
        ret = scv89601p_get_ichg(sc, &data);
        if (ret < 0)
            break;
        val->intval = data * 1000;
        break;
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
        ret = scv89601p_get_vbat(sc, &data);
        if (ret < 0)
            break;
        /*Tab A9 code for AX6739A-765 by hualei at 20230612 start*/
        val->intval = data;
        /*Tab A9 code for AX6739A-765 by hualei at 20230612 end*/
        break;
    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
        ret = scv89601p_get_iindpm(sc, &data);
        if (ret < 0)
            break;
        val->intval = data * 1000;
        break;
    case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
        ret = scv89601p_get_vindpm(sc, &data);
        if (ret < 0)
            break;
        val->intval = data * 1000;
        break;
    case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
        ret = scv89601p_get_term_curr(sc, &data);
        if (ret < 0)
            break;
        val->intval = data * 1000;
        break;
    case POWER_SUPPLY_PROP_USB_TYPE:
        val->intval = sc->psy_usb_type;
        break;
    case POWER_SUPPLY_PROP_CURRENT_MAX:
        if (sc->psy_usb_type == POWER_SUPPLY_USB_TYPE_SDP)
            val->intval = 500000;
        else
            val->intval = 1500000;
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_MAX:
        if (sc->psy_usb_type == POWER_SUPPLY_USB_TYPE_SDP)
            val->intval = 5000000;
        else
            val->intval = 12000000;
        break;
    case POWER_SUPPLY_PROP_TYPE:
        val->intval = sc->chg_type;
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static int scv89601p_chg_set_property(struct power_supply *psy,
                                    enum power_supply_property psp,
                                    const union power_supply_propval *val) {
    struct scv89601p_chip *sc = power_supply_get_drvdata(psy);
    int ret = 0;

    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:
        //Need notice
        dev_info(sc->dev, "%s  %d\n", __func__, val->intval);
        ret = scv89601p_chg_attach_pre_process(sc, val->intval);
        break;
    case POWER_SUPPLY_PROP_STATUS:
        ret = scv89601p_set_chg_enable(sc, !!val->intval);
        if (val->intval == POWER_SUPPLY_STATUS_DISCHARGING){
            scv89601p_set_otg_enable(sc, true);
            scv89601p_set_chg_enable(sc, false);
        }
        break;
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
        ret = scv89601p_set_ichg(sc, val->intval / 1000);
        break;
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
        ret = scv89601p_set_vbat(sc, val->intval / 1000);
        break;
    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
        ret = scv89601p_set_iindpm(sc, val->intval / 1000);
        break;
    case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
        ret = scv89601p_set_vindpm(sc, val->intval / 1000);
        break;
    case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
        ret = scv89601p_set_term_curr(sc, val->intval / 1000);
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static char *scv89601p_psy_supplied_to[] = {
    "battery",
    "mtk-master-charger",
};

static const struct power_supply_desc scv89601p_psy_desc = {
    .name = "charger",
    /*Tab A9 code for SR-AX6739A-01-493 by wenyaqi at 20230503 start*/
    // avoid healthd chg=au
    .type = POWER_SUPPLY_TYPE_UNKNOWN,
    /*Tab A9 code for SR-AX6739A-01-493 by wenyaqi at 20230503 end*/
    .usb_types = scv89601p_chg_psy_usb_types,
    .num_usb_types = ARRAY_SIZE(scv89601p_chg_psy_usb_types),
    .properties = scv89601p_chg_psy_properties,
    .num_properties = ARRAY_SIZE(scv89601p_chg_psy_properties),
    .property_is_writeable = scv89601p_chg_property_is_writeable,
    .get_property = scv89601p_chg_get_property,
    .set_property = scv89601p_chg_set_property,
};

static int scv89601p_psy_register(struct scv89601p_chip *sc)
{
    struct power_supply_config cfg = {
        .drv_data = sc,
        .of_node = sc->dev->of_node,
        .supplied_to = scv89601p_psy_supplied_to,
        .num_supplicants = ARRAY_SIZE(scv89601p_psy_supplied_to),
    };

    memcpy(&sc->psy_desc, &scv89601p_psy_desc, sizeof(sc->psy_desc));
    sc->psy = devm_power_supply_register(sc->dev, &sc->psy_desc, &cfg);
    return IS_ERR(sc->psy) ? PTR_ERR(sc->psy) : 0;
}

static int scv89601p_charger_probe(struct i2c_client *client,
                                const struct i2c_device_id *id)
{
    struct scv89601p_chip *sc;
    int ret = 0;
    /*Tab A9 code for AX6739A-2077 | AX6739A-2728 by wenyaqi at 20230906 start*/
    int reg_val = 0;
    /*Tab A9 code for AX6739A-2077 | AX6739A-2728 by wenyaqi at 20230906 end*/

    SCV89601P_ERR("%s (%s)\n", __func__, SCV89601P_DRV_VERSION);

    sc = devm_kzalloc(&client->dev, sizeof(struct scv89601p_chip), GFP_KERNEL);
    if (!sc)
        return -ENOMEM;

    client->addr = 0x6B;
    sc->dev = &client->dev;
    sc->client = client;
    i2c_set_clientdata(client, sc);
    if (!scv89601p_detect_device(sc)) {
        ret = -ENODEV;
        goto err_nodev;
    }

    scv89601p_create_device_node(&(client->dev));

    gxy_bat_set_chginfo(GXY_BAT_CHG_INFO_SCV89601P);

    INIT_DELAYED_WORK(&sc->force_detect_dwork,
                        scv89601p_force_detection_dwork_handler);
    INIT_DELAYED_WORK(&sc->bc12_timeout_dwork,
                        scv89601p_bc12_timeout_dwork_handler);
    INIT_DELAYED_WORK(&sc->hiz_cut_dwork,
                        scv89601p_hiz_cut_dwork_handler);
    /*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 start*/
    INIT_DELAYED_WORK(&sc->hvdcp_done_work, hvdcp_done_work_func);
    sc->hvdcp_done = false;
    /*Tab A9 code for AX6739A-409 by wenyaqi at 20230530 end*/
    /* HS07 code for HS07-1480 by lina at 20250425 start */
    #if IS_ENABLED(CONFIG_CUSTOM_USBIF)
    INIT_DELAYED_WORK(&sc->set_input_volt_lim_work, scv89601p_delay_set_input_volt_lim);
    #endif//CONFIG_CUSTOM_USBIF
    /* HS07 code for HS07-1480 by lina at 20250425 end */

    sc->cfg = &scv89601p_default_cfg;
    ret = scv89601p_parse_dt(sc);
    if (ret < 0) {
        SCV89601P_ERR("parse dt fail(%d)\n", ret);
        goto err_parse_dt;
    }

    mutex_init(&sc->bc_detect_lock);
    sc->bc12_detect = false;
    sc->bc12_recovery = false;
    sc->hiz_cut_flag = false;
    sc->power_good = 1;
    sc->dpdm_done_flag = 0;
    ret = scv89601p_init_device(sc);
    if (ret < 0) {
        SCV89601P_ERR("init device fail(%d)\n", ret);
        goto err_init_dev;
    }

    ret = scv89601p_psy_register(sc);
    if (ret) {
        SCV89601P_ERR("%s psy register fail(%d)\n", __func__, ret);
        goto err_psy;
    }
    ret = scv89601p_register_interrupt(sc);
    if (ret < 0) {
        SCV89601P_ERR("%s register irq fail(%d)\n", __func__, ret);
        goto err_register_irq;
    }
#ifdef CONFIG_MTK_CLASS
#ifdef CONFIG_MTK_CHARGER_V4P19
    INIT_DELAYED_WORK(&sc->psy_dwork,
                        SCV89601P_INFOrm_psy_dwork_handler);
#endif /*CONFIG_MTK_CHARGER_V4P19*/
    /* Register charger device */
    sc->chg_dev = charger_device_register(sc->cfg->chg_name,
                                        sc->dev, sc, &scv89601p_chg_ops,
                                        &scv89601p_chg_props);
    if (IS_ERR_OR_NULL(sc->chg_dev)) {
        ret = PTR_ERR(sc->chg_dev);
        dev_notice(sc->dev, "%s register chg dev fail(%d)\n", __func__, ret);
        goto err_register_chg_dev;
    }
#endif /* CONFIG_MTK_CLASS */
    device_init_wakeup(sc->dev, 1);

    determine_initial_status(sc);
    scv89601p_dump_register(sc);

    /* otg regulator */
    s_chg_dev_otg=sc->chg_dev;
    scv89601p_vbus_regulator_register(sc);

    /*Tab A9 code for AX6739A-2077 | AX6739A-2728 by wenyaqi at 20230906 start*/
    ret = scv89601p_field_read(sc, F_VBUS_GOOD_STAT, &reg_val);
    if (ret < 0) {
        dev_err(sc->dev, "%s get vbus_gd failed(%d)\n", __func__, ret);
    }
    sc->vbus_good = !!reg_val;
    dev_info(sc->dev, "%s get vbus_good=%d\n", __func__, sc->vbus_good);
    /*Tab A9 code for AX6739A-2077 | AX6739A-2728 by wenyaqi at 20230906 end*/
    SCV89601P_INFO("scv89601p probe successfully\n!");
    return 0;

err_register_irq:
#ifdef CONFIG_MTK_CLASS
err_register_chg_dev:
#endif                          /*CONFIG_MTK_CLASS */
err_psy:
err_init_dev:
err_parse_dt:
err_nodev:
    SCV89601P_ERR("scv89601p probe failed!\n");
    devm_kfree(&client->dev, sc);
    return -ENODEV;
}

static int scv89601p_charger_remove(struct i2c_client *client)
{
    struct scv89601p_chip *sc = i2c_get_clientdata(client);

    if (sc) {
        SCV89601P_INFO("%s\n", __func__);
        scv89601p_field_write(sc, F_REG_RST, 1);
#ifdef CONFIG_MTK_CLASS
        charger_device_unregister(sc->chg_dev);
#endif /*CONFIG_MTK_CLASS */
        power_supply_put(sc->psy);
    }
    return 0;
}

/*HS07 code for HS07-199 by lina at 20250324 start*/
static void scv89601p_charger_shutdown(struct i2c_client *client)
{
    struct scv89601p_chip *sc = i2c_get_clientdata(client);
    int shipmode_flag = 0;

    if (sc) {
        scv89601p_field_read(sc, F_BATFET_DIS_DELAY_STAT, &shipmode_flag);
        if (!shipmode_flag)
            scv89601p_field_write(sc, F_REG_RST, 1);
    }
}
/*HS07 code for HS07-199 by lina at 20250324 end*/

#ifdef CONFIG_PM_SLEEP
static int scv89601p_suspend(struct device *dev)
{
    struct scv89601p_chip *sc = dev_get_drvdata(dev);

    SCV89601P_INFO("%s\n", __func__);
    if (device_may_wakeup(dev))
        enable_irq_wake(sc->irq);
    disable_irq(sc->irq);

    return 0;
}

static int scv89601p_resume(struct device *dev)
{
    struct scv89601p_chip *sc = dev_get_drvdata(dev);

    SCV89601P_INFO("%s\n", __func__);
    enable_irq(sc->irq);
    if (device_may_wakeup(dev))
        disable_irq_wake(sc->irq);

    return 0;
}

static const struct dev_pm_ops scv89601p_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(scv89601p_suspend, scv89601p_resume)
};
#endif /* CONFIG_PM_SLEEP */

static struct of_device_id scv89601p_of_device_id[] = {
    {.compatible = "sc,scv89601p",},
    {},
};

static struct i2c_driver scv89601p_charger_driver = {
    .driver = {
        .name = "scv89601p",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(scv89601p_of_device_id),
#ifdef CONFIG_PM_SLEEP
        .pm = &scv89601p_pm_ops,
#endif
    },
    .probe = scv89601p_charger_probe,
    .remove = scv89601p_charger_remove,
    .shutdown = scv89601p_charger_shutdown,
};

module_i2c_driver(scv89601p_charger_driver);

MODULE_DESCRIPTION("Southchip SCV89601P Charger Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("South Chip <boyu-wen@southchip.com>");

/**
 * Release Version
 * 1.0
 * (1) add scv89601p charger dirver, compile pass
 */
/*HS07 code for HS07-188 by xiongxiaoliang at 20250320 end*/
/* HS07 code for SR-AL7761A-01-164 by lina at 20250314 end */
