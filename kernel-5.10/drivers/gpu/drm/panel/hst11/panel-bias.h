/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _PANEL_BIAS_H_
#define _PANEL_BIAS_H_

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>

/*_FUNC_*/
#define BIAS_IC_NAME_DETECT_FUNC 1

#define BIAS_INFO_BUF_SIZE 32
#define BIAS_INFO_WRITE_MAX_SIZE 4
#define BIAS_INFO_PROC_FILE "bias_info" //proc/bias_info
#define BIAS_IC_NAME_LEN 16

#define BIAS_IC_DETECT_REG       0x03

bool gs_smart_wakeup_flag_get(void);
void gs_smart_wakeup_flag_set(bool flag);
/*HST11 code for AX7800A-2752 by yubo at 20250620 start*/
int hbm_map_level(int level, int normal_max_bl, int hbm_max_bl);
/*HST11 code for AX7800A-2752 by yubo at 20250620 end*/
int panel_bias_i2c_write_bytes(unsigned char addr, unsigned char value);
int panel_bias_i2c_read_bytes(unsigned char addr, unsigned char *returnData);
int panel_bias_i2c_write_multiple_bytes(unsigned char addr, unsigned char *value, unsigned int size);
#endif//_PANEL_BIAS_H_
