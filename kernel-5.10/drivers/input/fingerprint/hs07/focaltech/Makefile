# Makefile for the focaltech fingerprint driver.
# HS07 code for HS07-955 by hehaoran at 20250418 start
ccflags-$(CONFIG_DRM_MEDIATEK_V2) += -I$(srctree)/drivers/gpu/drm/mediatek/mediatek_v2/
# HS07 code for HS07-955 by hehaoran at 20250418 end
obj-$(CONFIG_FINGERPRINT_FOCALTECH) := focaltech_fp.o
focaltech_fp-objs  := ff_core.o  ff_spi.o
