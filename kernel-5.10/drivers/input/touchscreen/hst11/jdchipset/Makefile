
ifeq ($(CONFIG_DRM_MEDIATEK), y)
ccflags-$(CONFIG_DRM_MEDIATEK) += -I$(srctree)/drivers/gpu/drm/mediatek/
endif
ccflags-$(CONFIG_DRM_MEDIATEK_V2) += -I$(srctree)/drivers/gpu/drm/mediatek/mediatek_v2/

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat
subdir-ccflags-y += -I$(srctree)/drivers/power/supply/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/extcon/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/eccci/inc/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/hst11/touch_common/
# HST11 code for AX7800A-354 by huangpengyu at 20250421 start
ccflags-y += -I$(srctree)/drivers/gpu/drm/panel/hst11/
# HST11 code for AX7800A-354 by huangpengyu at 20250421 end

ifeq ($(TARGET_BUILD_VARIANT), user)
	ccflags-y += -D__JADARD_GKI__
endif

ccflags-y += -DCONFIG_TOUCHSCREEN_JADARD_DEBUG=y
ccflags-y += -DCONFIG_TOUCHSCREEN_JADARD_SORTING=y
ccflags-y += -D__JADARD_KMODULE__

obj-m += jdchipset.o
jdchipset-y += jadard_module.o
jdchipset-y += jadard_ic_JD9366T.o
jdchipset-y += jadard_common.o
jdchipset-y += jadard_platform.o
jdchipset-y += jadard_debug.o
jdchipset-y += jadard_sorting.o