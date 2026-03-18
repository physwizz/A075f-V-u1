# Makefile for the Galaxycore touchscreen drivers.

ifeq ($(CONFIG_DRM_MEDIATEK), y)
ccflags-$(CONFIG_DRM_MEDIATEK) += -I$(srctree)/drivers/gpu/drm/mediatek/
endif
ccflags-$(CONFIG_DRM_MEDIATEK_V2) += -I$(srctree)/drivers/gpu/drm/mediatek/mediatek_v2/

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/eccci/inc/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/hs07/touch_common/
subdir-ccflags-y += -I$(srctree)/drivers/power/supply/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/extcon/
# HS07 code for SR-AL7761A-01-230 by hehaoran at 20250306 start
ccflags-y += -I$(srctree)/drivers/gpu/drm/panel/hs07/
# HS07 code for SR-AL7761A-01-230 by hehaoran at 20250306 end
obj-m += galaxycore.o
galaxycore-y    +=  gcore_drv_main.o
galaxycore-y    +=  gcore_drv_mtk.o
galaxycore-y    +=  gcore_drv_fw_update.o
galaxycore-y    +=  gcore_drv_interface.o
galaxycore-y    +=  gcore_drv_mp_test.o
