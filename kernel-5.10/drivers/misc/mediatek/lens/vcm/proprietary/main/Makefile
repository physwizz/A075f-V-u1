#
# Copyright (C) 2015 MediaTek Inc.
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

#
# Makefile for misc devices that really don't fit anywhere else.
#

MTK_CUSTOM_PATH  := $(srctree)/drivers/misc/mediatek


subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/lens/vcm/proprietary/main/inc
subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/lens/vcm/proprietary/main/common/lc898122af
subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/lens/vcm/proprietary/main/common/lc898212xdaf/inc
ifeq ($(CONFIG_MTK_LENS_BU63165AF_SUPPORT), y)
  subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/lens/vcm/proprietary/main/common/bu63165af/inc
else
  subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/lens/vcm/proprietary/main/common/bu63169af/inc
endif
#Tab A9 code for SR-AX6739A-01-214 by rongyi at 20230516 start


obj-$(CONFIG_MTK_LENS)    += mainaf.o

mainaf-y  += main_lens.o
# HS07 V code for HS07-19  by liugang at 2025/02/28 start
# Hs07 code  for HS07-19  AF by yujiahui at 20250317start
ifeq ($(CONFIG_CUSTOM_PROJECT_HS07), y)
mainaf-y  += common/a0701gt9778af/A0701GT9778AF.o
mainaf-y  += common/a0702pd9402af/A0702PD9402AF.o
mainaf-y  += common/a0703pd9402af/A0703PD9402AF.o
mainaf-y  += common/a0704gt9778af/A0704GT9778AF.o
mainaf-y  += common/a0705cn3968af/A0705CN3968AF.o
endif
#Hs07 code  for HS07-19  AF by yujiahui at 20250317end
# HS07 V code for HS07-19  by liugang at 2025/02/28 end
# HST11  code  for AX7800A-160 by lidong at 20250512 start
ifeq ($(CONFIG_CUSTOM_PROJECT_HST11), y)
mainaf-y  += common/a1101cn3927af/A1101CN3927AF.o
mainaf-y  += common/a1102cn3927af/A1102CN3927AF.o
# HST11  code  for AX7800A-158 by lixiaoqi at 20250513 start
mainaf-y  += common/a1103pd9215bl/A1103PD9215BL.o
# HST11  code  for AX7800A-158 by lixiaoqi at 20250513 end
endif
# HST11  code  for AX7800A-160 by lidong at 20250512 end
mainaf-y  += common/fp5510e2af/FP5510E2AF.o
mainaf-y  += common/bu6424af/BU6424AF.o
mainaf-y  += common/cn3927vaf/CN3927VAF.o
mainaf-y  += common/cn3927af/CN3927AF.o
mainaf-y  += common/cn3929af/CN3929AF.o
mainaf-y  += common/aw8601xaf/AW8601XAF.o
mainaf-y  += common/bu6429af/BU6429AF.o
mainaf-y  += common/bu64748af/bu64748_function.o
mainaf-y  += common/bu64748af/bu64748af.o
mainaf-y  += common/dw9714af/DW9714AF.o
mainaf-y  += common/dw9800waf/DW9800WAF.o
mainaf-y  += common/dw9814af/DW9814AF.o
mainaf-y  += common/dw9718af/DW9718AF.o
mainaf-y  += common/dw9718saf/DW9718SAF.o
mainaf-y  += common/dw9718taf/DW9718TAF.o
mainaf-y  += common/dw9719taf/DW9719TAF.o
mainaf-y  += common/dw9763af/DW9763AF.o
mainaf-y  += common/dw9839af/DW9839AF.o
mainaf-y  += common/gt9764af/GT9764AF.o
mainaf-y  += common/lc898212af/LC898212AF.o
mainaf-y  += common/lc898212xdaf/LC898212XDAF.o
mainaf-y  += common/lc898212xdaf/AfInter.o
mainaf-y  += common/lc898212xdaf/AfInit.o
mainaf-y  += common/lc898212xdaf/AfSTMV.o
mainaf-y  += common/lc898214af/LC898214AF.o
mainaf-y  += common/lc898217af/LC898217AF.o
mainaf-y  += common/lc898217afa/LC898217AFA.o
mainaf-y  += common/lc898217afb/LC898217AFB.o
mainaf-y  += common/lc898217afc/LC898217AFC.o
mainaf-y  += common/lc898229af/LC898229AF.o
mainaf-y  += common/lc898122af/LC898122AF.o
mainaf-y  += common/lc898122af/OisCmd.o
mainaf-y  += common/lc898122af/OisIni.o
mainaf-y  += common/wv511aaf/WV511AAF.o
mainaf-y  += common/ak7371af/AK7371AF.o
mainaf-y  += common/bu63169af/BU63169AF.o
mainaf-y  += common/bu63169af/OIS_func.o
mainaf-y  += common/bu63169af/OIS_main.o
mainaf-y  += common/bu63169af/OIS_user.o
mainaf-y  += common/bu64253gwzaf/BU64253GWZAF.o

#Tab A9 code for SR-AX6739A-01-214 by rongyi at 20230516 end


