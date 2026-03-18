#
# Copyright (C) 2018 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

MTK_CUSTOM_PATH := $(srctree)/drivers/misc/mediatek
CAM_CAL_SRC_PATH := $(MTK_CUSTOM_PATH)/cam_cal/src
CAM_CAL_CUSTOM_SRC_PATH := $(CAM_CAL_SRC_PATH)/custom
CAM_CAL_CUSTOM_SRC_LAYOUT_PATH := $(CAM_CAL_CUSTOM_SRC_PATH)/layout
COMMON_VERSION := v2

subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/imgsensor/inc \
                    -I$(MTK_CUSTOM_PATH)/cam_cal/inc \
                    -I$(CAM_CAL_CUSTOM_SRC_PATH) \
                    -I$(CAM_CAL_SRC_PATH)/common/$(COMMON_VERSION)


# Common code
obj-$(CONFIG_MTK_CAM_CAL) += camera_eeprom.o
camera_eeprom-objs := ../common/$(COMMON_VERSION)/eeprom_driver.o
camera_eeprom-objs += ../common/$(COMMON_VERSION)/eeprom_i2c_common_driver.o
camera_eeprom-objs += ../common/$(COMMON_VERSION)/eeprom_utils.o

# Platform code
camera_eeprom-objs += eeprom_i2c_custom_driver.o
camera_eeprom-objs += cam_cal_list.o
# Tab A9 code for SR-AX6739A-01-762 by liluling at 20230703 start
# HS07 V code for HS07-19  by xuyunhui at 2025/03/10 start
ifeq ($(CONFIG_CUSTOM_PROJECT_OT11),y)
camera_eeprom-objs += eeprom_i2c_a0902backhi846st_driver.o
camera_eeprom-objs += eeprom_i2c_a0904backsc820csst_driver.o
else ifeq ($(CONFIG_CUSTOM_PROJECT_HST11),y)
camera_eeprom-objs += eeprom_i2c_a0902backhi846st_driver.o
camera_eeprom-objs += eeprom_i2c_a0904backsc820csst_driver.o
endif
# Tab A9 code for SR-AX6739A-01-762 by liluling at 20230703 end
ifeq ($(CONFIG_CUSTOM_PROJECT_HS07), y)
camera_eeprom-objs +=eeprom_i2c_a0701cxtfrontgc08a8_driver.o
camera_eeprom-objs +=eeprom_i2c_a0702lyfrontmt815_driver.o
# HS07 V code for SR-AL7761A-01-383  by huabinchen at 2025/03/25 start
#camera_eeprom-objs +=eeprom_i2c_a0703ddfrontsc820cs_driver.o
# HS07 V code for SR-AL7761A-01-383  by huabinchen at 2025/03/25 end
endif
# HS07 V code for HS07-19  by xuyunhui at 2025/03/10 end
comma := ,
empty :=
space := $(empty) $(empty)

KERNEL_CAM_CAL_SRC := $(notdir $(wildcard $(CAM_CAL_CUSTOM_SRC_LAYOUT_PATH)/*.c))
KERNEL_CAM_CAL_OBJ := $(addprefix layout/,$(patsubst %.c, %.o, $(KERNEL_CAM_CAL_SRC)))
KERNEL_CAM_CAL := $(basename $(KERNEL_CAM_CAL_SRC))
CAM_CAL_CONFIG_LIST := $(subst $(space),$(comma),$(foreach e,$(subst ",,$(KERNEL_CAM_CAL)),E($(e))))
subdir-ccflags-y += -D'CAM_CAL_CONFIG_LIST=$(CAM_CAL_CONFIG_LIST)'

camera_eeprom-objs += cam_cal_config.o
camera_eeprom-objs += $(KERNEL_CAM_CAL_OBJ)
