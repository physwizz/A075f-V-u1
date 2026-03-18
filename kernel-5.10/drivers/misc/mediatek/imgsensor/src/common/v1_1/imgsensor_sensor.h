/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __IMGSENSOR_SENSOR_H__
#define __IMGSENSOR_SENSOR_H__

#include "kd_camera_feature.h"
#include "kd_imgsensor_define.h"
#include "imgsensor_i2c.h"

enum IMGSENSOR_STATE {
	IMGSENSOR_STATE_CLOSE,
	IMGSENSOR_STATE_OPEN
};

struct IMGSENSOR_SENSOR_STATUS {
	u32 reserved:28;
	u32 arch:4;
};

struct IMGSENSOR_SENSOR_INST {
/* HS07 V code for HS07-19  by xuyunhui at 2025/03/10 start */
#if defined (CONFIG_CUSTOM_PROJECT_HS07)
	char *psensor_name;
#endif
/* HS07 V code for HS07-19  by xuyunhui at 2025/03/10 end */
	enum IMGSENSOR_STATE state;
	enum IMGSENSOR_SENSOR_IDX sensor_idx;
	struct IMGSENSOR_I2C_CFG i2c_cfg;
	struct IMGSENSOR_SENSOR_STATUS status;
	struct IMGSENSOR_SENSOR_LIST *psensor_list;
	struct mutex sensor_mutex;
	struct timespec64 profile_time;
};

struct IMGSENSOR_SENSOR {
	struct IMGSENSOR_SENSOR_INST  inst;
	struct SENSOR_FUNCTION_STRUCT *pfunc;
};

#endif

