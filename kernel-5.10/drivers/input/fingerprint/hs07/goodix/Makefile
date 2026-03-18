#
# Makefile for the fingerprint device.
#
# HS07 code for HS07-955 by hehaoran at 20250418 start
ccflags-$(CONFIG_DRM_MEDIATEK_V2) += -I$(srctree)/drivers/gpu/drm/mediatek/mediatek_v2/
# HS07 code for HS07-955 by hehaoran at 20250418 end
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
obj-$(CONFIG_FINGERPRINT_GOODIX) := goodix_fp.o
goodix_fp-objs  := gf_spi_tee.o
