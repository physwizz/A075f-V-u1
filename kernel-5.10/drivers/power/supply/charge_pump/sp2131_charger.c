/*
 * Copyright (C) 2023 Nuvolta Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
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
#include <linux/version.h>

//#include "../../common/inc/nvt_charger_class.h"
//#include "../charger_class/hq_cp_class.h"
#include "sp2131_charger.h"
#include "sp2131_reg.h"
#include "pd_policy_manager.h"
#include <linux/power/gxy_psy_sysfs.h>
/************************************************************************/
/*HS07 code for SR-AL7761A-01-167 by yexuedong at 20250307 start*/
extern void gxy_bat_set_cpinfo(enum gxy_bat_cp_info pinfo_data);
extern void gxy_bat_set_cp_sts(bool cp_status);
struct sp2131 *g_sp2131 = NULL;
extern struct gxy_cp_ops g_gxy_cp_ops;
/*HS07 code for SR-AL7761A-01-167 by yexuedong at 20250307 end*/

static int __sp2131_read_byte(struct sp2131 *chip, u8 reg, u8 *data)
{
	s32 ret;

	ret = i2c_smbus_read_byte_data(chip->client, reg);
	if (ret < 0) {
		nu_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		return ret;
	}

	*data = (u8) ret;

	return 0;
}

static int __sp2131_write_byte(struct sp2131 *chip, int reg, u8 val)
{
	s32 ret;

	ret = i2c_smbus_write_byte_data(chip->client, reg, val);
	if (ret < 0) {
		nu_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
				val, reg, ret);
		return ret;
	}
	return 0;
}

static int sp2131_read_byte(struct sp2131 *chip, u8 reg, u8 *data)
{
	int ret;

	if (chip->skip_reads) {
		*data = 0;
		return 0;
	}

	mutex_lock(&chip->i2c_rw_lock);
	ret = __sp2131_read_byte(chip, reg, data);
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

static int sp2131_write_byte(struct sp2131 *chip, u8 reg, u8 data)
{
	int ret;

	if (chip->skip_writes)
		return 0;

	mutex_lock(&chip->i2c_rw_lock);
	ret = __sp2131_write_byte(chip, reg, data);
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

static int sp2131_update_bits(struct sp2131 *chip, u8 reg,
		u8 mask, u8 data)
{
	int ret;
	u8 tmp;

	if (chip->skip_reads || chip->skip_writes)
		return 0;

	mutex_lock(&chip->i2c_rw_lock);
	ret = __sp2131_read_byte(chip, reg, &tmp);
	if (ret) {
		nu_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= data & mask;

	ret = __sp2131_write_byte(chip, reg, tmp);
	if (ret)
		nu_err("Failed: reg=%02X, ret=%d\n", reg, ret);

out:
	mutex_unlock(&chip->i2c_rw_lock);
	return ret;
}
/*********************************************************************/


static int sp2131_set_errhi_status_mask(struct sp2131 *chip, bool en)
{
	int ret = 0;
	u8 val;

	if (en)
		val = SP2131_VBUS_ERRHI_MASK_ENABLE;
	else
		val = SP2131_VBUS_ERRHI_MASK_DISABLE;

	val <<= SP2131_VBUS_ERRHI_MASK_SHIFT;

	nu_info("sp2131_set_errhi_status_mask, val = %d\n", val);

	ret = sp2131_update_bits(chip, SP2131_REG_17, SP2131_VBUS_ERRHI_MASK_MASK, val);
	return ret;
}

static int sp2131_set_errlo_status_mask(struct sp2131 *chip, bool en)
{
	int ret = 0;
	u8 val;

	if (en)
		val = SP2131_VBUS_ERRLO_MASK_ENABLE;
	else
		val = SP2131_VBUS_ERRLO_MASK_DISABLE;

	val <<= SP2131_VBUS_ERRLO_MASK_SHIFT;

	nu_info("sp2131_set_errlo_status_mask, val = %d\n", val);

	ret = sp2131_update_bits(chip, SP2131_REG_17, SP2131_VBUS_ERRLO_MASK_MASK, val);
	return ret;
}


void sp2131_dump(struct sp2131 *chip)
{
	u8 addr;
	u8 val;
	int ret;
	for (addr = 0x0; addr <= 0x2B; addr++) {
		ret = sp2131_read_byte(chip, addr, &val);
		if (ret == 0) {
			nu_err("sp2131 reg[%02X]=%02X \n", addr, val);
		}
	}
}


static int _sp2131_enable_otg(struct sp2131 *chip, bool en)
{
	int ret = 0;
	u8 val;

	if (en)
		val = SP2131_OTG_ENABLE;
	else
		val = SP2131_OTG_DISABLE;

	val <<= SP2131_EN_OTG_SHIFT;

	nu_info("_sp2131_enable_otg, val = %d\n", val);

	ret = sp2131_update_bits(chip, SP2131_REG_0C, SP2131_EN_OTG_MASK, val);

	chip->otg_enable = en;

	return ret;
}


static int sp2131_get_adc_data(struct sp2131 *chip, int channel, int *data);
void sp2131_check_fault_status(struct sp2131 *chip);
static void sp2131_wireless_work(struct work_struct *work)
{
	struct sp2131 *chip = container_of(work, struct sp2131, wireless_work.work);
	int ret;
	int result;
	//mutex_lock(&chip->wireless_chg_lock);
#if 1
	dev_info(chip->dev, "[sp2131] [%s] \n", __func__);
#endif

	ret = sp2131_get_adc_data(chip, ADC_IBUS, &result);
	ret = sp2131_get_adc_data(chip, ADC_VBUS, &result);
	ret = sp2131_get_adc_data(chip, ADC_VAC, &result);
	ret = sp2131_get_adc_data(chip, ADC_VOUT, &result);
	ret = sp2131_get_adc_data(chip, ADC_VBAT, &result);
	ret = sp2131_get_adc_data(chip, ADC_TDIE, &result);
	ret = sp2131_get_adc_data(chip, ADC_TS, &result);

	sp2131_check_fault_status(chip);

	schedule_delayed_work(&chip->wireless_work, msecs_to_jiffies(5000));

	//mutex_unlock(&chip->wireless_chg_lock);

	return;
}

static int sp2131_enable_wdt(struct sp2131 *chip, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = SP2131_WATCHDOG_1S;//1s
	else
		val = SP2131_WATCHDOG_DIS;

	val <<= SP2131_WATCHDOG_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_0B,
			SP2131_WATCHDOG_MASK, val);

	return ret;
}

static int sp2131_set_reg_reset(struct sp2131 *chip)
{
	int ret;
	u8 val = 1;

	val = SP2131_REG_RST_ENABLE;

	val <<= SP2131_REG_RST_DISABLE;

	ret = sp2131_update_bits(chip, SP2131_REG_08,
			SP2131_REG_RST_MASK, val);
	return ret;
}

static int sp2131_enable_batovp(struct sp2131 *chip, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = SP2131_BAT_OVP_DISABLE;
	else
		val = SP2131_BAT_OVP_ENABLE;

	val <<= SP2131_BAT_OVP_DIS_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_02,
			SP2131_BAT_OVP_DIS_MASK, val);
	return ret;
}

static int sp2131_set_batovp_th(struct sp2131 *chip, int threshold)
{
	int ret;
	u8 val;

	if (threshold < SP2131_BAT_OVP_BASE)
		threshold = SP2131_BAT_OVP_BASE;

	val = (threshold - SP2131_BAT_OVP_BASE) / SP2131_BAT_OVP_LSB;

	val <<= SP2131_BAT_OVP_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_00,
			SP2131_BAT_OVP_MASK, val);
	return ret;
}


static int sp2131_enable_batocp(struct sp2131 *chip, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = SP2131_BAT_OCP_DISABLE;
	else
		val = SP2131_BAT_OCP_ENABLE;

	val <<= SP2131_BAT_OCP_DIS_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_01,
			SP2131_BAT_OCP_DIS_MASK, val);
	return ret;
}

static int sp2131_set_batocp_th(struct sp2131 *chip, int threshold)
{
	int ret;
	u8 val;

	if (threshold < SP2131_BAT_OCP_BASE)
		threshold = SP2131_BAT_OCP_BASE;

	val = (threshold - SP2131_BAT_OCP_BASE) / SP2131_BAT_OCP_LSB;

	val <<= SP2131_BAT_OCP_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_01,
			SP2131_BAT_OCP_MASK, val);
	return ret;
}


static int sp2131_set_busovp_th(struct sp2131 *chip, int threshold)
{
	int ret;
	u8 val;

	if (threshold < SP2131_BUS_OVP_BASE)
		threshold = SP2131_BUS_OVP_BASE;

	val = (threshold - SP2131_BUS_OVP_BASE) / SP2131_BUS_OVP_LSB;

	val <<= SP2131_BUS_OVP_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_03,
			SP2131_BUS_OVP_MASK, val);
	return ret;
}


static int sp2131_enable_busocp(struct sp2131 *chip, bool enable)
{
	int ret = 0;
	u8 val;

	if (enable)
		val = SP2131_BUS_OCP_DISABLE;
	else
		val = SP2131_BUS_OCP_ENABLE;

	val <<= SP2131_BUS_OCP_DIS_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_04,
			SP2131_BUS_OCP_DIS_MASK, val);

	return ret;
}

static int sp2131_set_busocp_th(struct sp2131 *chip, int threshold)
{
	int ret;
	u8 val;

	if (threshold < SP2131_BUS_OCP_BASE)
		threshold = SP2131_BUS_OCP_BASE;

	val = (threshold - SP2131_BUS_OCP_BASE) / (SP2131_BUS_OCP_LSB);

	val <<= SP2131_BUS_OCP_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_04,
			SP2131_BUS_OCP_MASK, val);
	return ret;
}


static int sp2131_set_acovp_th(struct sp2131 *chip, int threshold)
{
	int ret = 0;
	u8 val;

	if (threshold < SP2131_AC_OVP_BASE)
		threshold = SP2131_AC_OVP_BASE;

	if (threshold == SP2131_AC_OVP_6P5V)
		val = 0x0f;
	else
		val = (threshold - SP2131_AC_OVP_BASE) /  SP2131_AC_OVP_LSB;

	val <<= SP2131_AC_OVP_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_02,
			SP2131_AC_OVP_MASK, val);

	return ret;
}


static int _sp2131_enable_charge(struct sp2131 *chip, bool en)
{
	int ret;
	u8 val;

	if (en) {
		val = SP2131_CHG_ENABLE;
	} else {
		val = SP2131_CHG_DISABLE;
	}

	val <<= SP2131_CHG_EN_SHIFT;

	nu_err("SP2131 charger %s\n", en == false ? "disable" : "enable");
	ret = sp2131_update_bits(chip, SP2131_REG_07,
			SP2131_CHG_EN_MASK, val);

	return ret;
}

static int _sp2131_adc_enable(struct sp2131 *chip, bool enable)
{
	int ret;
	u8 val;

	dev_err(chip->dev,"SP2131 set adc :%d\n", enable);

	if (enable)
		val = SP2131_ADC_ENABLE;
	else
		val = SP2131_ADC_DISABLE;

	val <<= SP2131_ADC_EN_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_19,
			SP2131_ADC_EN_MASK, val);
	return ret;
}


static int sp2131_check_charge_enabled(struct sp2131 *chip, bool *en)
{
	int ret;
	u8 val_reg07;
	u8 val_reg11;

	ret = sp2131_read_byte(chip, SP2131_REG_11, &val_reg11);//SP2131_CP_ACTIVE_STAT(bit4)
	nu_info(">>>reg [0x0C] = 0x%02x\n", val_reg11);

	ret = sp2131_read_byte(chip, SP2131_REG_07, &val_reg07);//chg en(bit7)
	nu_info(">>>reg [0x0E] = 0x%02x\n", val_reg07);

	*en = (!!(val_reg11 & SP2131_CP_ACTIVE_STAT_MASK) & !!(val_reg07 & SP2131_CHG_EN_MASK));

	return ret;
}


#if 0
static int sp2131_check_vbus_error_status(struct sp2131 *chip)
{
	int ret;
	u8 data;
	u8 temp = 0;

	ret = sp2131_read_byte(chip, SP2131_REG_10, &data);
	if(ret == 0){
		chip->vbus_error_low = !!(data & (1 << SP2131_VBUS_ERRLO_STAT_SHIFT));
		chip->vbus_error_high = !!(data & (1 << SP2131_VBUS_ERRHI_STAT_SHIFT));
		nu_info("vbus_error_low:%d, vbus_error_high:%d \n", chip->vbus_error_low, chip->vbus_error_high);
	}

	if (chip->vbus_error_low != 0)
		temp |= BIT(VBUS_ERROR_L);

	if (chip->vbus_error_high != 0)
		temp |= BIT(VBUS_ERROR_H);

	return temp;
}

static int sp2131_enable_charge(struct charger_device *cp_dev, bool enable)
{
	struct sp2131 *chip = charger_get_data(cp_dev);
	return _sp2131_enable_charge(chip, enable);
}

static int SP2131_ADC_enable(struct charger_device *cp_dev, bool en)
{
	struct sp2131 *chip = charger_get_data(cp_dev);
	return _sp2131_adc_enable(chip, en);
}

static int sp2131_enable_otg(struct charger_device *cp_dev, bool en)
{
	struct sp2131 *chip = charger_get_data(cp_dev);
	return _sp2131_enable_otg(chip, en);
}

static int sp2131_get_is_enable(struct charger_device *cp_dev)
{
	struct sp2131 *chip = charger_get_data(cp_dev);
	return sp2131_check_charge_enabled(chip);
}

static int sp2131_get_vbus_status(struct charger_device *cp_dev)
{
	struct sp2131 *chip = charger_get_data(cp_dev);
	return sp2131_check_vbus_error_status(chip);
}
#endif

static int sp2131_set_adc_scanrate(struct sp2131 *chip, bool oneshot)
{
	int ret;
	u8 val;

	if (oneshot)
		val = SP2131_ADC_RATE_ONESHOT;
	else
		val = SP2131_ADC_RATE_CONTINOUS;

	val <<= SP2131_ADC_RATE_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_19,
			SP2131_ADC_RATE_MASK, val);
	return ret;
}

static int sp2131_get_adc_data(struct sp2131 *chip, int channel, int *data)
{
	int ret;
	u8 val_l, val_h;
	u16 val;

	//nu_err("[sp2131_get_adc_data]: channel = %d", channel);

	if(channel >= ADC_MAX_NUM) return -EINVAL;

	ret = sp2131_read_byte(chip, ADC_REG_BASE + (channel << 1), &val_h);
	ret = sp2131_read_byte(chip, ADC_REG_BASE + (channel << 1) + 1, &val_l);

	if (ret < 0)
		return ret;
	val = (val_h << 8) | val_l;

	nu_err("   [sp2131_get_adc_data]: val = %d", val);

	if(channel == ADC_TS)
		val = val * 25/100;
	else if(channel == ADC_TDIE)
		val = val * 5/10;

	*data = val;

	return ret;
}

static int sp2131_set_adc_scan(struct sp2131 *chip, int channel, bool enable)
{
	int ret;
	u8 reg;
	u8 mask;
	u8 shift;
	u8 val;

	if (channel > ADC_MAX_NUM)
		return -EINVAL;

	if (channel == ADC_IBUS) {
		reg = SP2131_REG_1A;
		shift = SP2131_IBUS_ADC_DIS_SHIFT;
		mask = SP2131_IBUS_ADC_DIS_MASK;
	} else if (channel == ADC_VBUS) {
		reg = SP2131_REG_1A;
		shift = SP2131_VBUS_ADC_DIS_SHIFT;
		mask = SP2131_VBUS_ADC_DIS_MASK;
	} else if (channel == ADC_VAC) {
		reg = SP2131_REG_1A;
		shift = SP2131_VAC_ADC_DIS_SHIFT;
		mask = SP2131_VAC_ADC_DIS_MASK;
	} else if (channel == ADC_VOUT) {
		reg = SP2131_REG_1A;
		shift = SP2131_VOUT_ADC_DIS_SHIFT;
		mask = SP2131_VOUT_ADC_DIS_MASK;
	} else if (channel == ADC_VBAT) {
		reg = SP2131_REG_1A;
		shift = SP2131_VBAT_ADC_DIS_SHIFT;
		mask = SP2131_VBAT_ADC_DIS_MASK;
	} else if (channel == ADC_IBAT) {
		reg = SP2131_REG_1A;
		shift = SP2131_IBAT_ADC_DIS_SHIFT;
		mask = SP2131_IBAT_ADC_DIS_MASK;
	} else if (channel == ADC_TDIE) {
		reg = SP2131_REG_1A;
		shift = SP2131_TDIE_ADC_DIS_SHIFT;
		mask = SP2131_TDIE_ADC_DIS_MASK;
	} else if (channel == ADC_TS) {
		reg = SP2131_REG_1A;
		shift = SP2131_TS_ADC_DIS_SHIFT;
		mask = SP2131_TS_ADC_DIS_MASK;
	} else {
		return -EINVAL;
	}

	if (enable)
		val = 0 << shift;
	else
		val = 1 << shift;

	ret = sp2131_update_bits(chip, reg, mask, val);

	return ret;
}

static int sp2131_disable_cp_ts_detect(struct sp2131 *chip)
{
	int ret;

	nu_err("sp2131_disable_cp_ts_flt_detect\n");
	ret = sp2131_write_byte(chip, SP2131_REG_06, SP2131_TS_FLT_DISABLE);//0x00

	return ret;
}

/*HS07 code for HS07-191 by yexuedong at 20250327 start*/
/*static int sp2131_set_fault_int_mask(struct sp2131 *chip, u8 mask1, u8 mask2, u8 mask3)
{
	int ret;
	u8 val;

	ret = sp2131_read_byte(chip, SP2131_REG_16, &val);
	if (ret)
		return ret;
	val |= mask1;
	ret = sp2131_write_byte(chip, SP2131_REG_16, val);

	ret = sp2131_read_byte(chip, SP2131_REG_17, &val);
	if (ret)
		return ret;
	val |= mask2;
	ret = sp2131_write_byte(chip, SP2131_REG_17, val);

	ret = sp2131_read_byte(chip, SP2131_REG_18, &val);
	if (ret)
		return ret;
	val |= mask3;
	ret = sp2131_write_byte(chip, SP2131_REG_18, val);

	return ret;
}*/
/*HS07 code for HS07-191 by yexuedong at 20250327 end*/


static int sp2131_set_sense_resistor(struct sp2131 *chip, int r_mohm)
{
	int ret = 0;

	u8 val;

	if (r_mohm == 2)
		val = SP2131_SET_IBAT_SNS_RES_2MHM;//0
	else if (r_mohm == 5)
		val = SP2131_SET_IBAT_SNS_RES_5MHM;//1
	else if (r_mohm == 1)
		val = SP2131_SET_IBAT_SNS_RES_1MHM;//2
	else
		return -EINVAL;

	val <<= SP2131_SET_IBAT_SNS_RES_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_09,
			SP2131_SET_IBAT_SNS_RES_MASK,
			val);

	return ret;
}

/*HS07 code for HS07-191 by yexuedong at 20250327 start*/
static int sp2131_set_pmid2vout_ovp_uvp(struct sp2131 *chip, int ovp_th, int uvp_th)
{
	int ret =0;
	u8 val;

	//pmid2vout_ovp setting
	if (ovp_th < SP2131_PMID2VOUT_OVP_BASE)//250mV
		ovp_th = SP2131_PMID2VOUT_OVP_BASE;

	val = (ovp_th - SP2131_PMID2VOUT_OVP_BASE) / (SP2131_PMID2VOUT_OVP_LSB);//250/100

	val <<= SP2131_PMID2VOUT_OVP_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_05,
			SP2131_PMID2VOUT_OVP_MASK, val);

	//pmid2vout_uvp setting
	if (uvp_th > SP2131_PMID2VOUT_UVP_BASE)//-100mV
		uvp_th = SP2131_PMID2VOUT_UVP_BASE;

	val = (uvp_th - SP2131_PMID2VOUT_UVP_BASE) / (SP2131_PMID2VOUT_UVP_LSB);//(-100)/(-25)

	val <<= SP2131_PMID2VOUT_UVP_SHIFT;

	ret = sp2131_update_bits(chip, SP2131_REG_05,
			SP2131_PMID2VOUT_UVP_MASK, val);

	return ret;
}

static int sp2131_set_switching_freq(struct sp2131 *chip, int freq)
{
	int ret =0;
	u8 val;

	switch (freq) {
		case SP2131_FSW_SET_500KHZ:
			val = SP2131_FSW_SET_500KHZ;
			break;
		case SP2131_FSW_SET_533KHZ:
			val = SP2131_FSW_SET_533KHZ;
			break;
		case SP2131_FSW_SET_571KHZ:
			val = SP2131_FSW_SET_571KHZ;
			break;
		case SP2131_FSW_SET_615KHZ:
			val = SP2131_FSW_SET_615KHZ;
			break;
		case SP2131_FSW_SET_667KHZ:
			val = SP2131_FSW_SET_667KHZ;
			break;
		case SP2131_FSW_SET_727KHZ:
			val = SP2131_FSW_SET_727KHZ;
			break;
		case SP2131_FSW_SET_800KHZ:
			val = SP2131_FSW_SET_800KHZ;
			break;
		case SP2131_FSW_SET_889KHZ:
			val = SP2131_FSW_SET_889KHZ;
			break;
		case SP2131_FSW_SET_1000KHZ:
			val = SP2131_FSW_SET_1000KHZ;
			break;
		case SP2131_FSW_SET_1143KHZ:
			val = SP2131_FSW_SET_1143KHZ;
			break;
		case SP2131_FSW_SET_1333KHZ:
			val = SP2131_FSW_SET_1333KHZ;
			break;
		case SP2131_FSW_SET_1600KHZ:
			val = SP2131_FSW_SET_1600KHZ;
			break;
		case SP2131_FSW_SET_2000KHZ:
			val = SP2131_FSW_SET_2000KHZ;
			break;
		default:
			val = SP2131_FSW_SET_500KHZ;
			break;
	}

	val <<= SP2131_FSW_SET_SHIFT;;

	ret = sp2131_update_bits(chip, SP2131_REG_07,
			SP2131_FSW_SET_MASK,
			val);

	return ret;
}
/*HS07 code for HS07-191 by yexuedong at 20250327 end*/

static int sp2131_set_ss_timeout(struct sp2131 *chip, int timeout)
{
	int ret =0;
	u8 val;

	switch (timeout) {
		case 0:
			val = SP2131_SS_TIMEOUT_DISABLE;
			break;
		case 40:
			val = SP2131_SS_TIMEOUT_40MS;
			break;
		case 80:
			val = SP2131_SS_TIMEOUT_80MS;
			break;
		case 320:
			val = SP2131_SS_TIMEOUT_320MS;
			break;
		case 1280:
			val = SP2131_SS_TIMEOUT_1280MS;
			break;
		case 5120:
			val = SP2131_SS_TIMEOUT_5120MS;
			break;
		case 20480:
			val = SP2131_SS_TIMEOUT_20480MS;
			break;
		case 81920:
			val = SP2131_SS_TIMEOUT_81920MS;
			break;
		default:
			val = SP2131_SS_TIMEOUT_DISABLE;
			break;
	}

	val <<= SP2131_SS_TIMEOUT_SET_SHIFT;;

	ret = sp2131_update_bits(chip, SP2131_REG_09,
			SP2131_SS_TIMEOUT_SET_MASK,
			val);

	return ret;
}

static int sp2131_detect_device(struct sp2131 *chip)
{
	int ret;
	u8 data;

	ret = sp2131_read_byte(chip, SP2131_REG_2B, &data);
	if (ret == 0) {
		chip->part_no = (data & SP2131_DEV_ID_MASK);
		chip->part_no >>= SP2131_DEV_ID_SHIFT;
	}

	return ret;
}

void sp2131_check_fault_flag(struct sp2131 *chip)
{
	int ret;
	u8 data = 0;

	mutex_lock(&chip->data_lock);
	ret = sp2131_read_byte(chip, SP2131_REG_12, &data);
	if (!ret && data)
		nu_err("reg_12 = 0x%02X\n", data);

	ret = sp2131_read_byte(chip, SP2131_REG_13, &data);
	if (!ret && data)
		nu_err("reg_13 = 0x%02X\n", data);

	ret = sp2131_read_byte(chip, SP2131_REG_14, &data);
	if (!ret && data)
		nu_err("reg_14 = 0x%02X\n", data);

	/*HS07 code for HS07-191 by yexuedong at 20250327 start*/
	ret = sp2131_read_byte(chip, SP2131_REG_15, &data);
	if (!ret && data)
		nu_err("reg_15 = 0x%02X\n", data);
	/*HS07 code for HS07-191 by yexuedong at 20250327 end*/

	mutex_unlock(&chip->data_lock);
}

void sp2131_check_fault_status(struct sp2131 *chip)
{
	int ret;
	u8 data = 0;

	mutex_lock(&chip->data_lock);
	ret = sp2131_read_byte(chip, SP2131_REG_0F, &data);
	//if (!ret && data)
	dev_err(chip->dev,"**reg_0F = 0x%02X\n", data);

	ret = sp2131_read_byte(chip, SP2131_REG_10, &data);
	//if (!ret && data)
	dev_err(chip->dev,"**reg_10 = 0x%02X\n", data);

	ret = sp2131_read_byte(chip, SP2131_REG_11, &data);
	//if (!ret && data)
	dev_err(chip->dev,"**reg_11 = 0x%02X\n", data);
	mutex_unlock(&chip->data_lock);
}


static ssize_t sp2131_show_registers(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sp2131 *chip = dev_get_drvdata(dev);
	u8 addr;
	u8 val;
	u8 tmpbuf[400];
	int len;
	int idx = 0;
	int ret;

	idx = snprintf(buf, PAGE_SIZE, "%s:\n", "sp2131");
	for (addr = 0x0; addr <= 0x2B; addr++) {
		ret = sp2131_read_byte(chip, addr, &val);
		if (ret == 0) {
			len = snprintf(tmpbuf, PAGE_SIZE - idx,
					"Reg[%.2X] = 0x%.2x\n", addr, val);
			memcpy(&buf[idx], tmpbuf, len);
			idx += len;
		}
	}

	return idx;
}

static ssize_t sp2131_store_register(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sp2131 *chip = dev_get_drvdata(dev);
	int ret;
	unsigned int reg;
	unsigned int val;

	ret = sscanf(buf, "%x %x", &reg, &val);
	if (ret == 2 && reg <= 0x2B)
		sp2131_write_byte(chip, (unsigned char)reg, (unsigned char)val);

	return count;
}

static DEVICE_ATTR(registers, 0660, sp2131_show_registers, sp2131_store_register);

static void sp2131_create_device_node(struct device *dev)
{
	device_create_file(dev, &dev_attr_registers);
}

static int sp2131_init_adc(struct sp2131 *chip)
{
	sp2131_set_adc_scanrate(chip, false);
	sp2131_set_adc_scan(chip, ADC_IBUS, true);
	sp2131_set_adc_scan(chip, ADC_VBUS, true);
	sp2131_set_adc_scan(chip, ADC_VOUT, true);
	sp2131_set_adc_scan(chip, ADC_VBAT, true);
	sp2131_set_adc_scan(chip, ADC_IBAT, true);
	sp2131_set_adc_scan(chip, ADC_TS, true);
	sp2131_set_adc_scan(chip, ADC_TDIE, true);
	sp2131_set_adc_scan(chip, ADC_VAC, true);

	_sp2131_adc_enable(chip, true);

	return 0;
}

/*HS07 code for HS07-191 by yexuedong at 20250327 start*/
/*static int sp2131_init_int_src(struct sp2131 *chip)
{
	int ret = 0;

	ret = sp2131_set_fault_int_mask(chip, 0, (SP2131_VBUS_ERRLO_MASK_MASK| SP2131_VBUS_ERRHI_MASK_MASK | SP2131_TS_FLT_MASK_MASK), 0);
	if (ret) {
		nu_err("failed to set fault mask:%d\n", ret);
		return ret;
	}
	return ret;
}*/
/*HS07 code for HS07-191 by yexuedong at 20250327 end*/

static int sp2131_init_protection(struct sp2131 *chip)
{
	int ret;

	ret = sp2131_enable_batovp(chip, chip->cfg->bat_ovp_disable);
	nu_err("%s bat ovp %s\n",
			chip->cfg->bat_ovp_disable ? "disable" : "enable",
			!ret ? "successfullly" : "failed");

	ret = sp2131_enable_batocp(chip, chip->cfg->bat_ocp_disable);
	nu_err("%s bat ocp %s\n",
			chip->cfg->bat_ocp_disable ? "disable" : "enable",
			!ret ? "successfullly" : "failed");

	ret = sp2131_enable_busocp(chip, chip->cfg->bus_ocp_disable);
	nu_err("%s bus ocp %s\n",
			chip->cfg->bus_ocp_disable ? "disable" : "enable",
			!ret ? "successfullly" : "failed");

	ret = sp2131_set_batovp_th(chip, chip->cfg->bat_ovp_th);
	nu_err("set bat ovp th %d %s\n", chip->cfg->bat_ovp_th,
			!ret ? "successfully" : "failed");

	ret = sp2131_set_batocp_th(chip, chip->cfg->bat_ocp_th);
	nu_err("set bat ocp threshold %d %s\n", chip->cfg->bat_ocp_th,
			!ret ? "successfully" : "failed");

	ret = sp2131_set_busovp_th(chip, chip->cfg->bus_ovp_th);
	nu_err("set bus ovp threshold %d %s\n", chip->cfg->bus_ovp_th,
			!ret ? "successfully" : "failed");

	ret = sp2131_set_busocp_th(chip, chip->cfg->bus_ocp_th);
	nu_err("set bus ocp threshold %d %s\n", chip->cfg->bus_ocp_th,
			!ret ? "successfully" : "failed");

	ret = sp2131_set_acovp_th(chip, chip->cfg->ac_ovp_th);
	nu_err("set ac ovp threshold %d %s\n", chip->cfg->ac_ovp_th,
			!ret ? "successfully" : "failed");

	return 0;
}

/*HS07 code for HS07-191 by yexuedong at 20250327 start*/
static int sp2131_init_device(struct sp2131 *chip)
{
	sp2131_set_reg_reset(chip);
	sp2131_enable_wdt(chip, false);
	sp2131_disable_cp_ts_detect(chip);

	sp2131_set_errhi_status_mask(chip, false);
	sp2131_set_errlo_status_mask(chip, false);


	sp2131_set_ss_timeout(chip, 1280);//1.28s
	sp2131_set_sense_resistor(chip, 5);

	sp2131_init_protection(chip);
	sp2131_init_adc(chip);

	sp2131_set_pmid2vout_ovp_uvp(chip, 250, (-100));//SP2131_REG_05 //PMID2VOUT OVP/UVP:250mV/-100mV
	sp2131_set_switching_freq(chip, SP2131_FSW_SET_615KHZ);

	return 0;
}
/*HS07 code for HS07-191 by yexuedong at 20250327 end*/

static irqreturn_t sp2131_irq_handler(int irq, void *data)
{
	struct sp2131 *chip = data;

	dev_err(chip->dev,"INT OCCURED\n");

	sp2131_check_fault_flag(chip);
	sp2131_check_fault_status(chip);


	return IRQ_HANDLED;
}

static int SP2131_REGister_interrupt(struct sp2131 *chip)
{
	int ret = 0;

	if (gpio_is_valid(chip->irq_gpio)) {
		//ret = gpio_request_one(chip->irq_gpio, GPIOF_DIR_IN,"sp2131_irq");
		gpio_direction_input(chip->irq_gpio);
		if (ret) {
			dev_err(chip->dev,"failed to request so2131_irq\n");
			return -EINVAL;
		}
		chip->irq = gpio_to_irq(chip->irq_gpio);
		if (chip->irq < 0) {
			dev_err(chip->dev,"failed to gpio_to_irq\n");
			return -EINVAL;
		}
	} else {
		dev_err(chip->dev,"irq gpio not provided\n");
		return -EINVAL;
	}

	if (chip->irq) {
		ret = devm_request_threaded_irq(&chip->client->dev, chip->irq,
				NULL, sp2131_irq_handler,
				IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				sp2131_irq_name[chip->mode], chip);

		if (ret < 0) {
			dev_err(chip->dev,"request irq for irq=%d failed, ret =%d\n",
							chip->irq, ret);
			return ret;
		}
		enable_irq_wake(chip->irq);
	}

	return ret;
}
/********************interrupte end*************************************************/


/*
static int sp2131_get_adc_value(struct charger_device *cp_dev, u8 ch, int *value)
{
	struct sp2131 *chip = charger_get_data(cp_dev);
	return sp2131_get_adc_data(chip, ch, value);
}*/


/*
static int sp2131_get_chip_id(struct charger_device *cp_dev, int *value)
{
	struct sp2131 *chip = charger_get_data(cp_dev);

	*value = chip->part_no;
	return 0;
}*/

/*HS07 code for SR-AL7761A-01-167 by yexuedong at 20250307 start*/
static int sp2131_set_present(struct sp2131 *sp, bool present)
{
    int ret = 0;

    sp->usb_present = present;
    if (present) {
        ret = sp2131_init_device(sp);
    }
    return ret;
}

/******************External interface************/
int sp2131_usbpd_get_present(int *value)
{
    if (g_sp2131 != NULL) {
        *value = g_sp2131->usb_present;
        return true;
    } else {
        return false;
    }
}

int sp2131_usbpd_set_present(int *value)
{
    if (g_sp2131 != NULL) {
        sp2131_set_present(g_sp2131, !!(*value));
        return true;
    } else {
        return false;
    }
}

int sp2131_usbpd_get_charging_enabled(bool *value)
{
    int ret = 0;
    bool result = false;

    if (g_sp2131 != NULL) {
        ret = sp2131_check_charge_enabled(g_sp2131, &result);
        if (!ret) {
            g_sp2131->charge_enabled = !!result;
        }

        *value = g_sp2131->charge_enabled;
        return true;
    } else {
        return false;
    }
}

int sp2131_usbpd_set_charging_enabled(bool *value)
{
    if (g_sp2131 != NULL) {
        _sp2131_enable_charge(g_sp2131, !!(*value));
        gxy_bat_set_cp_sts(*value);
        return true;
    } else {
        return false;
    }
}

int sp2131_usbpd_get_vbus(int *value)
{
    int ret = 0;
    int result = 0;

    if (g_sp2131 != NULL) {
        ret = sp2131_get_adc_data(g_sp2131, ADC_VBUS, &result);
        if (!ret) {
            g_sp2131->vbus_volt = result;
        }

        *value = g_sp2131->vbus_volt;
        return true;
    } else {
        return false;
    }
}

int sp2131_usbpd_get_ibus(int *value)
{
    int ret = 0;
    int result = 0;

    if (g_sp2131 != NULL) {
        ret = sp2131_get_adc_data(g_sp2131, ADC_IBUS, &result);
        if (!ret) {
            g_sp2131->ibus_curr = result;
        }

        *value = g_sp2131->ibus_curr;
        return true;
    } else {
        return false;
    }
}

int sp2131_usbpd_get_vbat(int *value)
{
    int ret = 0;
    int result = 0;

    if (g_sp2131 != NULL) {
        ret = sp2131_get_adc_data(g_sp2131, ADC_VBAT, &result);
        if (!ret) {
            g_sp2131->vbat_volt = result;
        }

        *value = g_sp2131->vbat_volt;
        return true;
    } else {
        return false;
    }
}

int sp2131_usbpd_get_ibat(int *value)
{
    int ret = 0;
    int result = 0;

    if (g_sp2131 != NULL) {
        ret = sp2131_get_adc_data(g_sp2131, ADC_IBAT, &result);
        if (!ret) {
            g_sp2131->ibat_curr = result;
        }

        *value = g_sp2131->ibat_curr;
        return true;
    } else {
        return false;
    }
}

int sp2131_usbpd_get_temp(int *value)
{
    int ret = 0;
    int result = 0;

    if (g_sp2131 != NULL) {
        ret = sp2131_get_adc_data(g_sp2131, ADC_TDIE, &result);
        if (!ret) {
            g_sp2131->die_temp = result;
        }

        *value = g_sp2131->die_temp;
        return true;
    } else {
        return false;
    }
}

int sp2131_usbpd_set_otg_txmode(int *value)
{
    int enable = *value;

    if (g_sp2131 != NULL) {
        _sp2131_enable_otg(g_sp2131, enable);
        return true;
    } else {
        return false;
    }
}

int sp2131_usbpd_get_otg_txmode(int *value)
{
    if (g_sp2131 != NULL) {
        *value = g_sp2131->otg_enable;
        return true;
    } else {
        return false;
    }
}

int sp2131_usbpd_pm_set_shipmode_adc(int *value)
{
    int enable = *value;

    if (g_sp2131 != NULL) {
        _sp2131_adc_enable(g_sp2131, enable);
        return true;
    } else {
        return false;
    }
}
/***********************************************/
/*HS07 code for SR-AL7761A-01-167 by yexuedong at 20250307 end*/


static int sp2131_set_work_mode(struct sp2131 *chip, int mode)
{
	chip->mode = mode;

	dev_err(chip->dev,"work mode is %s\n", chip->mode == SP2131_STANDALONG
		? "standalone" : (chip->mode == SP2131_MASTER ? "master" : "slave"));

	return 0;
}


static int sp2131_parse_dt(struct sp2131 *chip, struct device *dev)
{
	int ret;
	struct device_node *np = dev->of_node;

	chip->cfg = devm_kzalloc(dev, sizeof(struct sp2131_cfg),
			GFP_KERNEL);

	if (!chip->cfg)
		return -ENOMEM;

	ret = of_property_read_u32(np, "nuvolta,sp2131,bat-ovp-disable",
			&chip->cfg->bat_ovp_disable);
	ret = of_property_read_u32(np, "nuvolta,sp2131,bat-ocp-disable",
			&chip->cfg->bat_ocp_disable);
	ret = of_property_read_u32(np, "nuvolta,sp2131,bus-ocp-disable",
			&chip->cfg->bus_ocp_disable);

	ret = of_property_read_u32(np, "nuvolta,sp2131,bat-ovp-threshold",
			&chip->cfg->bat_ovp_th);
	if (ret) {
		nu_err("failed to read bat-ovp-threshold\n");
		return ret;
	}
	ret = of_property_read_u32(np, "nuvolta,sp2131,bat-ocp-threshold",
			&chip->cfg->bat_ocp_th);
	if (ret) {
		nu_err("failed to read bat-ocp-threshold\n");
		return ret;
	}
	ret = of_property_read_u32(np, "nuvolta,sp2131,bus-ovp-threshold",
			&chip->cfg->bus_ovp_th);
	if (ret) {
		nu_err("failed to read bus-ovp-threshold\n");
		return ret;
	}

	ret = of_property_read_u32(np, "nuvolta,sp2131,bus-ocp-threshold",
			&chip->cfg->bus_ocp_th);
	if (ret) {
		nu_err("failed to read bus-ocp-threshold\n");
		return ret;
	}

	ret = of_property_read_u32(np, "nuvolta,sp2131,ac-ovp-threshold",
			&chip->cfg->ac_ovp_th);
	if (ret) {
		nu_err("failed to read ac-ovp-threshold\n");
		return ret;
	}

	chip->irq_gpio = of_get_named_gpio(np, "sp2131,intr_gpio", 0);
	if (!gpio_is_valid(chip->irq_gpio)) {
		dev_err(chip->dev,"fail to valid gpio : %d\n", chip->irq_gpio);
		return -EINVAL;
	}
	return 0;
}

static struct of_device_id sp2131_charger_match_table[] = {
	{
		.compatible = "nuvolta,sp2131-standalone",
		.data = &sp2131_mode_data[SP2131_STANDALONG],
	},
	{
		.compatible = "nuvolta,sp2131-master",
		.data = &sp2131_mode_data[SP2131_MASTER],
	},

	{
		.compatible = "nuvolta,sp2131-slave",
		.data = &sp2131_mode_data[SP2131_SLAVE],
	},
	{},
};
MODULE_DEVICE_TABLE(of, sp2131_charger_match_table);

static int sp2131_charger_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct sp2131 *chip;
	const struct of_device_id *match;
	struct device_node *node = client->dev.of_node;
	int ret;

	dev_err(&client->dev, "%s\n", __func__);

	chip =  devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		ret = -ENOMEM;
		goto err_kzalloc;
	}
	chip->dev = &client->dev;
	chip->client = client;

	mutex_init(&chip->i2c_rw_lock);
	mutex_init(&chip->data_lock);

	ret = sp2131_detect_device(chip);
	if (ret) {
		nu_err("No sp2131 device found!\n");
		ret = -ENODEV;
		goto err_detect_dev;
	}

	i2c_set_clientdata(client, chip);
	sp2131_create_device_node(&(client->dev));

	dev_err(chip->dev, "%s\n sp2131 ", __func__);
	match = of_match_node(sp2131_charger_match_table, node);
	if (match == NULL) {
		nu_err("device tree match not found!\n");
		goto err_match_node;
	}

	nu_err("sp2131 match->data = %d \n",*(int *)match->data);

	ret = sp2131_set_work_mode(chip, *(int *)match->data);
	if (ret) {
		dev_err(chip->dev,"Fail to set work mode!\n");
		goto err_set_mode;
	}

	ret = sp2131_parse_dt(chip, &client->dev);
	if (ret < 0) {
		dev_err(chip->dev, "%s parse dt failed(%d)\n", __func__, ret);
		goto err_parse_dt;
	}

	ret = sp2131_init_device(chip);
	if (ret < 0) {
		dev_err(chip->dev, "%s init device failed(%d)\n", __func__, ret);
		goto err_init_device;
	}

	/*ret = sp2131_psy_register(chip);
	if (ret < 0) {
		dev_err(chip->dev, "%s psy register failed(%d)\n", __func__, ret);
		goto err_register_psy;
	}*/

	ret = SP2131_REGister_interrupt(chip);
	if (ret < 0) {
		dev_err(chip->dev, "%s register irq fail(%d)\n",
					__func__, ret);
		goto err_register_irq;
	}

	INIT_DELAYED_WORK(&chip->wireless_work, sp2131_wireless_work);
	schedule_delayed_work(&chip->wireless_work, 0);

	sp2131_dump(chip);//colin

#if 0
	if (chip->mode == SP2131_MASTER) {
		chip->cp_dev = charger_device_register("master_cp_chg",
				chip->dev, chip, &sp2131_ops);
	} else {
		chip->cp_dev = charger_device_register("slave_cp_chg",
				chip->dev, chip, &sp2131_ops);
	}
#endif

	/*HS07 code for SR-AL7761A-01-167 by yexuedong at 20250307 start*/
	g_sp2131 = chip;

	g_gxy_cp_ops.get_present = sp2131_usbpd_get_present;
	g_gxy_cp_ops.set_present = sp2131_usbpd_set_present;
	g_gxy_cp_ops.set_charging_enabled = sp2131_usbpd_set_charging_enabled;
	g_gxy_cp_ops.get_charging_enabled = sp2131_usbpd_get_charging_enabled;
	g_gxy_cp_ops.get_vbus = sp2131_usbpd_get_vbus;
	g_gxy_cp_ops.get_ibus = sp2131_usbpd_get_ibus;
	g_gxy_cp_ops.get_vbat = sp2131_usbpd_get_vbat;
	g_gxy_cp_ops.get_ibat = sp2131_usbpd_get_ibat;
	g_gxy_cp_ops.get_temp = sp2131_usbpd_get_temp;
	g_gxy_cp_ops.set_otg_txmode = sp2131_usbpd_set_otg_txmode;
	g_gxy_cp_ops.get_otg_txmode = sp2131_usbpd_get_otg_txmode;
	chip->otg_enable = false;

	g_gxy_cp_ops.set_shipmode_adc = sp2131_usbpd_pm_set_shipmode_adc;
	gxy_bat_set_cpinfo(GXY_BAT_CP_INFO_SP2131);
	/*HS07 code for SR-AL7761A-01-167 by yexuedong at 20250307 end*/
	nu_err("sp2131 probe successfully, Part Num:%d\n!", chip->part_no);

	return 0;

err_register_irq:
err_init_device:
err_detect_dev:
err_match_node:
err_set_mode:
err_parse_dt:
	devm_kfree(&client->dev, chip);
err_kzalloc:
	dev_err(&client->dev,"sp2131 probe fail\n");
	return ret;
}


static int sp2131_charger_remove(struct i2c_client *client)
{
	struct sp2131 *chip = i2c_get_clientdata(client);


	_sp2131_adc_enable(chip, false);
	mutex_destroy(&chip->data_lock);
	mutex_destroy(&chip->i2c_rw_lock);

	devm_kfree(&client->dev, chip);

	return 0;
}

static void sp2131_charger_shutdown(struct i2c_client *client)
{
	struct sp2131 *chip = i2c_get_clientdata(client);

	_sp2131_adc_enable(chip, false);
}


static int sp2131_suspend(struct device *dev)
{
	struct sp2131 *chip = dev_get_drvdata(dev);

	_sp2131_adc_enable(chip, false);

	dev_info(chip->dev, "Suspend successfully!");
	if (device_may_wakeup(dev))
		enable_irq_wake(chip->irq);
	disable_irq(chip->irq);

	return 0;
}

static int sp2131_resume(struct device *dev)
{
	struct sp2131 *chip = dev_get_drvdata(dev);

	_sp2131_adc_enable(chip, true);

	dev_info(chip->dev, "Resume successfully!");
	if (device_may_wakeup(dev))
		disable_irq_wake(chip->irq);
	enable_irq(chip->irq);

	return 0;
}
static const struct dev_pm_ops sp2131_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sp2131_suspend, sp2131_resume)
};

static const struct i2c_device_id sp2131_charger_id[] = {
	{"sp2131-standalone", SP2131_STANDALONG},
	{},
};
MODULE_DEVICE_TABLE(i2c, sp2131_charger_id);

static struct i2c_driver sp2131_charger_driver = {
	.driver		= {
		.name	= "sp2131-charger",
		.owner	= THIS_MODULE,
		.of_match_table = sp2131_charger_match_table,
		.pm	= &sp2131_pm_ops,
	},
	.id_table	= sp2131_charger_id,

	.probe		= sp2131_charger_probe,
	.remove		= sp2131_charger_remove,
	.shutdown	= sp2131_charger_shutdown,
};

module_i2c_driver(sp2131_charger_driver);

MODULE_DESCRIPTION("Nuvolta SP2131 Charge Pump Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("mick.ye@nuvoltatech.com");
