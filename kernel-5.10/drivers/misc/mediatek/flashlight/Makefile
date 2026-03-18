#
# Makefile for the kernel flashlight device drivers.
#

flashlight-y := flashlight-core.o
flashlight-y += flashlight-device.o

obj-$(CONFIG_MTK_FLASHLIGHT)             += flashlight.o
obj-$(CONFIG_MTK_V4L2_FLASHLIGHT)        += mtk-composite.o
obj-$(CONFIG_MTK_FLASHLIGHT_LED191)      += flashlights-led191.o
obj-$(CONFIG_MTK_FLASHLIGHT_LM3642)      += flashlights-lm3642.o
obj-$(CONFIG_MTK_FLASHLIGHT_LM3643)      += flashlights-lm3643.o
obj-$(CONFIG_MTK_FLASHLIGHT_LM3644)      += flashlights-lm3644.o
obj-$(CONFIG_MTK_FLASHLIGHT_RT4505)      += flashlights-rt4505.o
obj-$(CONFIG_MTK_FLASHLIGHT_RT5081)      += flashlights-rt5081.o
obj-$(CONFIG_MTK_FLASHLIGHT_MT6336)      += flashlights-mt6336.o
obj-$(CONFIG_MTK_FLASHLIGHT_MT6370)      += flashlights-mt6370.o
obj-$(CONFIG_MTK_FLASHLIGHT_MT6360)      += flashlights-mt6360.o
obj-$(CONFIG_MTK_FLASHLIGHT_AW3644)      += flashlights-aw3644.o
obj-$(CONFIG_MTK_FLASHLIGHT_DUMMY)       += flashlights-dummy.o
obj-$(CONFIG_MTK_FLASHLIGHT_DUMMY_GPIO)  += flashlights-dummy-gpio.o
#HS07 V code for HS07-19  by zhongbin at 2025/02/28 start
ifeq ($(CONFIG_CUSTOM_PROJECT_HS07),y)
obj-$(CONFIG_MTK_FLASHLIGHT_SGM3785_GPIO)      += flashlights-sgm3785-gpio.o
endif
$(info CONFIG_MTK_FLASHLIGHT_SGM3785_GPIO = $(CONFIG_MTK_FLASHLIGHT_SGM3785_GPIO))
#HS07 V code for HS07-19  by zhongbin at 2025/02/28 end
ccflags-$(CONFIG_MTK_FLASHLIGHT_DEBUG)     := -DDEBUG
ccflags-$(CONFIG_MTK_FLASHLIGHT_PT)        += -DCONFIG_MTK_FLASHLIGHT_PT
ccflags-$(CONFIG_MTK_FLASHLIGHT_PT_STRICT) += -DCONFIG_MTK_FLASHLIGHT_PT_STRICT
ccflags-$(CONFIG_MTK_FLASHLIGHT_DLPT)      += -DCONFIG_MTK_FLASHLIGHT_DLPT

MTK_CUSTOM_PATH := $(srctree)/drivers/misc/mediatek

ifeq ($(CONFIG_MTK_FLASHLIGHT_AW3644),y)
ccflags-y += -DCONFIG_MTK_FLASHLIGHT_AW3644
endif
subdir-ccflags-y += \
	-I$(MTK_CUSTOM_PATH)/include/mt-plat \
	-I$(MTK_CUSTOM_PATH)/include/mt-plat/$(MTK_PLATFORM)/include/mach \
	-I$(MTK_CUSTOM_PATH)/pmic/include \
	-I$(MTK_CUSTOM_PATH)/pmic \
	-I$(MTK_CUSTOM_PATH)/power_throttling
subdir-ccflags-y += \
    -I$(MTK_CUSTOM_PATH)/pbm
