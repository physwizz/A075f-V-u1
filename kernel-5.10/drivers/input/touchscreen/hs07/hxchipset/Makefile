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
ccflags-y += -I$(srctree)/drivers/gpu/drm/panel/hs07/

obj-m += hxchipset.o
hxchipset-y    +=  himax_debug.o
hxchipset-y    +=  himax_inspection.o
hxchipset-y    +=  himax_ic_HX83108d.o
hxchipset-y    +=  himax_ic_core.o
hxchipset-y    +=  himax_common.o
hxchipset-y    +=  himax_platform.o