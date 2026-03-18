ifeq ($(CONFIG_DRM_MEDIATEK), y)
ccflags-$(CONFIG_DRM_MEDIATEK) += -I$(srctree)/drivers/gpu/drm/mediatek/
endif
ccflags-$(CONFIG_DRM_MEDIATEK_V2) += -I$(srctree)/drivers/gpu/drm/mediatek/mediatek_v2/

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat
subdir-ccflags-y += -I$(srctree)/drivers/power/supply/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/eccci/inc/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/hs07/touch_common/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/extcon/
ccflags-y += -I$(srctree)/drivers/gpu/drm/panel/hs07/
#obj-$(CONFIG_TOUCHSCREEN_CHIPONE) += chipone-tddi.o
obj-m += chipone.o

chipone-objs := cts_driver.o
chipone-objs += cts_core.o
chipone-objs += cts_sfctrlv2.o
chipone-objs += cts_spi_flash.o
chipone-objs += cts_firmware.o
chipone-objs += cts_test.o
chipone-objs += cts_charger_detect.o
chipone-objs += cts_earjack_detect.o
chipone-objs += cts_tcs.o
chipone-objs += cts_platform.o
chipone-objs += cts_tool.o
chipone-objs += cts_sysfs.o
chipone-objs += cts_strerror.o
chipone-objs += cts_oem.o

