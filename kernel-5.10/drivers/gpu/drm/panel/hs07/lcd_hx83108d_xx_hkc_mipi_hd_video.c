// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/backlight.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_modes.h>
#include <linux/delay.h>
#include <drm/drm_connector.h>
#include <drm/drm_device.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/string.h>

#define CONFIG_MTK_PANEL_EXT
#if defined(CONFIG_MTK_PANEL_EXT)
#include "../mediatek/mediatek_v2/mtk_panel_ext.h"
#include "../mediatek/mediatek_v2/mtk_drm_graphics_base.h"
#endif     // CONFIG_MTK_PANEL_EXT
/*HS07 code for HS07-528 by huangyin at 20250418 start*/
#define BACKLIGHT_MAX_REG_VAL   19
#define BACKLIGHT_MAX_APP_VAL   23
/*HS07 code for HS07-528 by huangyin at 20250418 end*/
#include "./panel-bias.h"
/*HS07 code for HS07-354 by huangyin at 20250407 start*/
#include <linux/pinctrl/pinctrl.h>
static struct pinctrl *pinctl_rst = NULL;
static struct pinctrl_state *tp_prst_low = NULL;
static struct pinctrl_state *tp_prst_high = NULL;
/*HS07 code for HS07-354 by huangyin at 20250407 end*/
/*HS07 code for SR-AL7761A-01-584 by huangyin at 20250321 start*/
#define HFP_SUPPORT 1
#define HAC (720)
/*HS07 code for HS07-2870 by huangyin at 20250515 start*/
/*HS07 code for HS07-941 by huangyin at 20250422 start*/
#define HFP (136)
#define HSA (14)
#define HBP (125)
/*HS07 code for HS07-941 by huangyin at 20250422 end*/
/*HS07 code for HS07-2870 by huangyin at 20250515 end*/
#define VAC (1600)
#define VFP (1206)
#define VFP_90 (262)
#define VSA (4)
#define VBP (28)
#define PHYSICAL_WIDTH                  (70308)
#define PHYSICAL_HEIGHT                 (156240)
/*HS07 code for SR-AL7761A-01-584 by huangyin at 20250321 end*/
#define CURRENT_FPS                     60
#define HTOTAL                          (HAC + HFP + HSA + HBP)
#define VTOTAL                          (VAC + VFP+  VSA + VBP)
#define HTOTAL_90                  (HAC + HFP + HSA + HBP)
#define VTOTAL_90                   (VAC + VFP_90+  VSA + VBP)
#define FPS_60  60
#define FPS_90  90

static int current_fps = 60;

struct lcm {
    struct device *dev;
    struct drm_panel panel;
    struct backlight_device *backlight;
    struct gpio_desc *reset_gpio;
    struct gpio_desc *bias_pos;
    struct gpio_desc *bias_neg;
    bool prepared;
    bool enabled;
    unsigned int gate_ic;
    int error;
};

#define lcm_dcs_write_seq(ctx, seq...)                                         \
    ({                                                                     \
        const u8 d[] = { seq };                                        \
        BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
                 "DCS sequence too big for stack");           \
        lcm_dcs_write(ctx, d, ARRAY_SIZE(d));                         \
    })

#define lcm_dcs_write_seq_static(ctx, seq...)                                  \
    ({                                                                     \
        static const u8 d[] = { seq };                                 \
        lcm_dcs_write(ctx, d, ARRAY_SIZE(d));                         \
    })

static inline struct lcm *panel_to_lcm(struct drm_panel *panel)
{
    return container_of(panel, struct lcm, panel);
}

static void lcm_dcs_write(struct lcm *ctx, const void *data, size_t len)
{
    struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
    ssize_t ret = 0;
    char *addr = NULL;

    if (ctx->error < 0) {
        return;
    }

    addr = (char *)data;
    if ((int)*addr < 0xB0) {
        ret = mipi_dsi_dcs_write_buffer(dsi, data, len);
    } else {
        ret = mipi_dsi_generic_write(dsi, data, len);
    }

    if (ret < 0) {
        dev_err(ctx->dev, "error %zd writing seq: %ph\n", ret, data);
        ctx->error = ret;
    }
}

static void lcm_deep_suspend_setting(struct lcm *ctx)
{
    lcm_dcs_write_seq_static(ctx, 0x28);
    mdelay(20);
    lcm_dcs_write_seq_static(ctx, 0x10);
    mdelay(120);
    lcm_dcs_write_seq_static(ctx, 0xB9, 0x83,0x10,0x8D);
    lcm_dcs_write_seq_static(ctx, 0xBD, 0x00);
    lcm_dcs_write_seq_static(ctx, 0xB1, 0x2D);
    mdelay(30);
};

static void lcm_aot_suspend_setting(struct lcm *ctx)
{
    lcm_dcs_write_seq_static(ctx, 0x28);
    mdelay(20);
    lcm_dcs_write_seq_static(ctx, 0x10);
    mdelay(120);
}
/*HS07 code for HS07-3390 by huangyin at 20250523 start*/
static void lcm_panel_init(struct lcm *ctx)
{
    pr_info("%s+\n", __func__);
    lcm_dcs_write_seq_static(ctx, 0xB9, 0x83,0x10,0x8D);
    lcm_dcs_write_seq_static(ctx, 0xBD, 0x00);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xC4);
    lcm_dcs_write_seq_static(ctx, 0xD9, 0xD8);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xB1, 0x2C,0x37,0x37,0x31,0x35,0x22,0x4D,0x39,0x59,0x1A,0x12,0xE8,0x21,0x11,0x95,0x5A,0x65,0x26,0x00,0x26,0x9E,0xA1,0x15,0x60);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xDD);
    lcm_dcs_write_seq_static(ctx, 0xB1, 0x4D,0x4D);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xB2, 0x00,0x26,0xD0,0x40,0x01,0x1E,0x04,0x20,0xB1,0x11,0x00,0x00,0x00,0x15,0x22,0xD7);
    lcm_dcs_write_seq_static(ctx, 0xB4, 0xB4,0x6A,0xB4,0x6A,0xB4,0x6A,0x76,0x76,0x76,0x76,0x02,0xFF,0x01,0xFF,0x00,0xFF,0x00,0xFF);
    //lcm_dcs_write_seq_static(ctx, 0xB6, 0x94,0x94,0x00);
    lcm_dcs_write_seq_static(ctx, 0xBA, 0x70,0x03,0xA8,0x92);
    lcm_dcs_write_seq_static(ctx, 0xB8, 0x40,0x20,0x00,0x00,0x00,0x00,0x1F,0x1F,0x0E,0x0C,0x1A);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xD0);
    lcm_dcs_write_seq_static(ctx, 0xBA, 0x05);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xBF, 0xFC,0x40);
    lcm_dcs_write_seq_static(ctx, 0xC0, 0x33,0x33,0x22,0x33,0xB3,0xCF,0x03,0x03,0x00,0x00,0x00,0x63,0x85,0x11,0x11,0x11);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xC5);
    lcm_dcs_write_seq_static(ctx, 0xC7, 0x80,0xCA,0x08,0x14,0x02,0x44,0x00,0x00);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xC6);
    lcm_dcs_write_seq_static(ctx, 0xC8, 0x87,0x13,0xFF);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xCC, 0x02);
    lcm_dcs_write_seq_static(ctx, 0xCB, 0x13,0x08,0xE0,0x0C,0xE3);
    lcm_dcs_write_seq_static(ctx, 0xD1, 0x27,0x02,0x2C);
    lcm_dcs_write_seq_static(ctx, 0xD2, 0x00,0x00,0x00,0x00);
    lcm_dcs_write_seq_static(ctx, 0xD3, 0xC0,0x00,0x10,0x10,0x00,0x00,0x7F,0x0F,0x88,0x84,0x11,0x11,0x11,0x11,0x32,0x10,0x0E,0x00,0x0E,0x32,0x16,0xBE,0x06,0xBE,0x32,0x10,0x0C,0x00,0x0C,0x00,0x00,0x00,0x00,0x00);
    lcm_dcs_write_seq_static(ctx, 0xD5, 0x18,0x18,0x18,0x18,0x18,0x18,0xBE,0xBE,0xBE,0xBE,0x18,0x18,0x18,0x18,0x18,0x18,0xBE,0xBE,0xBE,0xBE,0x01,0x00,0x03,0x02,0x05,0x04,0x07,0x06,0x09,0x08,0x0B,0x0A,0x0D,0x0C,0x0F,0x0E,0x23,0x22,0x21,0x20,0x18,0x18,0x18,0x18);
    lcm_dcs_write_seq_static(ctx, 0xD6, 0x18,0x18,0x18,0x18,0x18,0x18,0xBE,0xBE,0xBE,0xBE,0x18,0x18,0x18,0x18,0x18,0x18,0xBE,0xBE,0xBE,0xBE,0x0E,0x0F,0x0C,0x0D,0x0A,0x0B,0x08,0x09,0x06,0x07,0x04,0x05,0x02,0x03,0x00,0x01,0x20,0x21,0x22,0x23,0x18,0x18,0x18,0x18);
    lcm_dcs_write_seq_static(ctx, 0xD8, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
    lcm_dcs_write_seq_static(ctx, 0xE0, 0x04,0x08,0x0E,0x13,0x19,0x27,0x3E,0x43,0x4E,0x49,0x65,0x6C,0x73,0x85,0x84,0x8D,0x96,0xA8,0xA7,0x52,0x59,0x64,0x73,0x04,0x08,0x0F,0x13,0x19,0x27,0x3E,0x43,0x4E,0x49,0x65,0x6C,0x73,0x85,0x84,0x8D,0x96,0xA8,0xA7,0x52,0x59,0x64,0x73);
    lcm_dcs_write_seq_static(ctx, 0xE7, 0x00,0x00,0x00,0x01);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xD0);
    lcm_dcs_write_seq_static(ctx, 0xE7, 0xEA,0x0A,0x01,0x02);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xDD);
    lcm_dcs_write_seq_static(ctx, 0xE7, 0x32,0x02,0x04,0x08,0x01,0x69,0x56);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xBD, 0x01);
    lcm_dcs_write_seq_static(ctx, 0xB1, 0x01,0x1B,0x11);
    lcm_dcs_write_seq_static(ctx, 0xBA, 0x64,0x24,0x80,0x02,0x06,0x09,0x04,0x41);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xC9);
    lcm_dcs_write_seq_static(ctx, 0xBF, 0x08,0x20);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xCB, 0x00,0x35,0x01,0x01,0x03,0x00,0x00,0x00,0x77);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xC6);
    lcm_dcs_write_seq_static(ctx, 0xD2, 0x0B,0x28);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xC6);
    lcm_dcs_write_seq_static(ctx, 0xD3, 0x28);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xD8, 0x55,0x55,0x55,0x55,0x55,0x00,0x55,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
    lcm_dcs_write_seq_static(ctx, 0xE7, 0x01);
    lcm_dcs_write_seq_static(ctx, 0xBD, 0x02);
    lcm_dcs_write_seq_static(ctx, 0xBF, 0xF3,0x00,0x02);
    lcm_dcs_write_seq_static(ctx, 0xCB, 0x03,0x79,0x1E,0x0B,0x2E);
    lcm_dcs_write_seq_static(ctx, 0xD8, 0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0);
    lcm_dcs_write_seq_static(ctx, 0xBD, 0x03);
    lcm_dcs_write_seq_static(ctx, 0xD8, 0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x55,0x55,0x55,0x55,0x55,0x00,0x55,0x55,0x55,0x55,0x55,0x00);
    lcm_dcs_write_seq_static(ctx, 0xBD, 0x00);
    lcm_dcs_write_seq_static(ctx, 0xC9, 0x00,0x1E,0x0B,0xEA,0x01);//PWM=20KHZ
    lcm_dcs_write_seq_static(ctx, 0x51, 0x00,0x00);
    lcm_dcs_write_seq_static(ctx, 0x53, 0x2C);
    lcm_dcs_write_seq_static(ctx, 0x35, 0x00);
    //CRC start
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xC3);
    lcm_dcs_write_seq_static(ctx, 0xCD, 0x6D);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0xC5);
    lcm_dcs_write_seq_static(ctx, 0xCD, 0x1F, 0x39, 0x00);
    lcm_dcs_write_seq_static(ctx, 0xE9, 0x3F);
    lcm_dcs_write_seq_static(ctx, 0xCD, 0x81);
    // CRC end
    lcm_dcs_write_seq_static(ctx, 0x11);
    mdelay(120);
    lcm_dcs_write_seq_static(ctx, 0x29);
    mdelay(1);

    pr_info("%s-\n", __func__);
}
/*HS07 code for HS07-3390 by huangyin at 20250523 end*/
static int lcm_disable(struct drm_panel *panel)
{
    struct lcm *ctx = panel_to_lcm(panel);
    pr_info("%s\n", __func__);

    if (!ctx->enabled) {
        return 0;
    }

    if (ctx->backlight) {
        ctx->backlight->props.power = FB_BLANK_POWERDOWN;
        backlight_update_status(ctx->backlight);
    }

    ctx->enabled = false;

    return 0;
}
/*HS07 code for HS07-354 by huangyin at 20250407 start*/
static int lcm_unprepare(struct drm_panel *panel)
{
    struct lcm *ctx = panel_to_lcm(panel);
    int retval = 0;
    if (!ctx->prepared) {
        return 0;
    }

    pr_info("%s+\n", __func__);
    if (gs_smart_wakeup_flag_get()) {
        lcm_aot_suspend_setting(ctx);
    } else {
        lcm_deep_suspend_setting(ctx);
        if (pinctl_rst != NULL && tp_prst_low != NULL) {
            retval = pinctrl_select_state(pinctl_rst, tp_prst_low);
            if (retval < 0){
                pr_err("%s:set pinctl_rst failed,retval=%d", __func__, retval);
            }
        }
        ctx->bias_neg = devm_gpiod_get_index(ctx->dev, "bias", 1, GPIOD_OUT_HIGH);
        if (IS_ERR(ctx->bias_neg)) {
            dev_err(ctx->dev, "%s: cannot get bias_neg %ld\n", __func__, PTR_ERR(ctx->bias_neg));
            return PTR_ERR(ctx->bias_neg);
        }
        gpiod_set_value(ctx->bias_neg, 0);
        devm_gpiod_put(ctx->dev, ctx->bias_neg);

        mdelay(5);

        ctx->bias_pos = devm_gpiod_get_index(ctx->dev, "bias", 0, GPIOD_OUT_HIGH);
        if (IS_ERR(ctx->bias_pos)) {
            dev_err(ctx->dev, "%s: cannot get bias_pos %ld\n", __func__, PTR_ERR(ctx->bias_pos));
            return PTR_ERR(ctx->bias_pos);
        }
        gpiod_set_value(ctx->bias_pos, 0);
        devm_gpiod_put(ctx->dev, ctx->bias_pos);
    }
    ctx->error = 0;
    ctx->prepared = false;
    pr_info("%s-\n", __func__);
    return 0;
}
/*HS07 code for HS07-354 by huangyin at 20250407 end*/
static int lcm_prepare(struct drm_panel *panel)
{
    struct lcm *ctx = panel_to_lcm(panel);
    /*HS07 code for HS07-354 by huangyin at 20250407 start*/
    int ret = 0;
    int retval = 0;

    pr_info("%s+\n", __func__);
    if (ctx->prepared)
        return 0;
    if (pinctl_rst != NULL && tp_prst_high != NULL) {
        retval = pinctrl_select_state(pinctl_rst, tp_prst_high);
        if (retval < 0){
            pr_err("%s:set pinctl_rst failed,retval=%d", __func__, retval);
        }
    }
    mdelay(1);
    /*HS07 code for HS07-354 by huangyin at 20250407 end*/
    ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(ctx->reset_gpio)) {
        dev_err(ctx->dev, "cannot get lcd reset-gpios %ld\n", PTR_ERR(ctx->reset_gpio));
        return PTR_ERR(ctx->reset_gpio);
    }
    /*HS07 code for SR-AL7761A-01-584 by huangyin at 20250321 start*/
    gpiod_set_value(ctx->reset_gpio, 0);
    mdelay(5);

    ctx->bias_pos = devm_gpiod_get_index(ctx->dev, "bias", 0, GPIOD_OUT_HIGH);
    if (IS_ERR(ctx->bias_pos)) {
        dev_err(ctx->dev, "%s: cannot get bias_pos %ld\n", __func__, PTR_ERR(ctx->bias_pos));
        return PTR_ERR(ctx->bias_pos);
    }
    gpiod_set_value(ctx->bias_pos, 1);
    devm_gpiod_put(ctx->dev, ctx->bias_pos);
    panel_bias_i2c_write_bytes(0x0, 0x14);
    mdelay(5);

    ctx->bias_neg = devm_gpiod_get_index(ctx->dev, "bias", 1, GPIOD_OUT_HIGH);
    if (IS_ERR(ctx->bias_neg)) {
        dev_err(ctx->dev, "%s: cannot get bias_neg %ld\n", __func__, PTR_ERR(ctx->bias_neg));
        return PTR_ERR(ctx->bias_neg);
    }
    gpiod_set_value(ctx->bias_neg, 1);
    devm_gpiod_put(ctx->dev, ctx->bias_neg);
    panel_bias_i2c_write_bytes(0x1, 0x14);
    mdelay(5);

    gpiod_set_value(ctx->reset_gpio, 1);
    mdelay(5);
    gpiod_set_value(ctx->reset_gpio, 0);
    mdelay(5);
    gpiod_set_value(ctx->reset_gpio, 1);
    mdelay(50);
    /*HS07 code for SR-AL7761A-01-584 by huangyin at 20250321 end*/
    devm_gpiod_put(ctx->dev, ctx->reset_gpio);

    lcm_panel_init(ctx);

    ret = ctx->error;
    if (ret < 0)
        lcm_unprepare(panel);

    ctx->prepared = true;

    pr_info("%s-\n", __func__);
    return ret;
}

static int lcm_enable(struct drm_panel *panel)
{
    struct lcm *ctx = panel_to_lcm(panel);
    pr_info("%s\n", __func__);

    if (ctx->enabled) {
        return 0;
    }

    if (ctx->backlight) {
        ctx->backlight->props.power = FB_BLANK_UNBLANK;
        backlight_update_status(ctx->backlight);
    }

    ctx->enabled = true;

    return 0;
}

static const struct drm_display_mode default_mode = {
    .clock = HTOTAL * VTOTAL * FPS_60 / 1000,
    .hdisplay = HAC,
    .hsync_start = HAC + HFP,
    .hsync_end = HAC + HFP + HSA,
    .htotal = HAC + HFP + HSA + HBP,
    .vdisplay = VAC,
    .vsync_start = VAC + VFP,
    .vsync_end = VAC + VFP + VSA,
    .vtotal = VAC + VFP + VSA + VBP,
    .width_mm = PHYSICAL_WIDTH / 1000,
    .height_mm = PHYSICAL_HEIGHT / 1000,
};

static const struct drm_display_mode performance_mode_90hz = {
    .clock = HTOTAL_90 * VTOTAL_90 * FPS_90 / 1000,
    .hdisplay = HAC,
    .hsync_start = HAC + HFP,
    .hsync_end = HAC + HFP + HSA,
    .htotal = HAC + HFP + HSA + HBP,
    .vdisplay = VAC,
    .vsync_start = VAC + VFP_90,
    .vsync_end = VAC + VFP_90 + VSA,
    .vtotal = VAC + VFP_90 + VSA + VBP,
    .width_mm = PHYSICAL_WIDTH / 1000,
    .height_mm = PHYSICAL_HEIGHT / 1000,
};

#if defined(CONFIG_MTK_PANEL_EXT)
/*HS07 code for HS07-2870 by huangyin at 20250515 start*/
/*HS07 code for HS07-941 by huangyin at 20250422 start*/
static struct mtk_panel_params ext_params = {
    .pll_clk = 550,
    .vfp_low_power = VFP,//60hz vfp
    .cust_esd_check = 1,
    .esd_check_enable = 1,
    .lcm_esd_check_table[0] = {
        .cmd = 0x0A,
        .count = 1,
        .para_list[0] = 0x9D,
    },
    /*HS07 code for HS07-302 by huangyin at 20250401 start*/
    .lcm_esd_check_table[1] = {
        .cmd = 0x09,
        .count = 3,
        .para_list[0] = 0x80,
        .para_list[1] = 0x73,
        .para_list[2] = 0x06,
    },
    /*HS07 code for HS07-302 by huangyin at 20250401 end*/
    .physical_width_um = PHYSICAL_WIDTH,
    .physical_height_um = PHYSICAL_HEIGHT,

    .data_rate = 1100,
    .dyn_fps = {
        .switch_en = 1,
        .vact_timing_fps = 90,
    },
};

static struct mtk_panel_params ext_params_90hz = {
    .pll_clk = 550,
    .vfp_low_power = VFP,//60hz vfp
    .cust_esd_check = 1,
    .esd_check_enable = 1,
    .lcm_esd_check_table[0] = {
        .cmd = 0x0A,
        .count = 1,
        .para_list[0] = 0x9D,
    },
    /*HS07 code for HS07-302 by huangyin at 20250401 start*/
    .lcm_esd_check_table[1] = {
        .cmd = 0x09,
        .count = 3,
        .para_list[0] = 0x80,
        .para_list[1] = 0x73,
        .para_list[2] = 0x06,
    },
    /*HS07 code for HS07-302 by huangyin at 20250401 end*/
    .physical_width_um = PHYSICAL_WIDTH,
    .physical_height_um = PHYSICAL_HEIGHT,

    .data_rate = 1100,
    .dyn_fps = {
        .switch_en = 1,
        .vact_timing_fps = 90,
    },
};
/*HS07 code for HS07-941 by huangyin at 20250422 end*/
/*HS07 code for HS07-2870 by huangyin at 20250515 end*/

static int lcm_setbacklight_cmdq(void *dsi, dcs_write_gce cb, void *handle,
                 unsigned int level)
{
    /*HS07 code for SR-AL7761A-01-584 by huangyin at 20250321 start*/
    char bl_tb0[] = {0x51, 0x0f, 0xff};
    /*HS07 code for HS07-2936 by huangyin at 20250513 start*/
    char bl_tb1[] = {0x53, 0x24};
    /*HS07 code for HS07-2936 by huangyin at 20250513 end*/
    /*HS07 code for SR-AL7761A-01-584 by huangyin at 20250321 end*/
    if (level == 0) {
        cb(dsi, handle, bl_tb1, ARRAY_SIZE(bl_tb1));
        pr_info("close diming\n");
    }
    /*HS07 code for HS07-528 by huangyin at 20250418 start*/
    level = mult_frac(level, BACKLIGHT_MAX_REG_VAL, BACKLIGHT_MAX_APP_VAL);
    /*HS07 code for HS07-528 by huangyin at 20250418 end*/
    bl_tb0[1] = level >> 8;
    bl_tb0[2] = level & 0xFF;

    pr_info("%s bl_tb0[1] = 0x%x,bl_tb0[2]= 0x%x\n", __func__, bl_tb0[1], bl_tb0[2]);

    if (!cb) {
        return -1;
        pr_err("cb error\n");
    }

    cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));
    return 0;
}

struct drm_display_mode *get_mode_by_id_hfp(struct drm_connector *connector,
    unsigned int mode)
{
    struct drm_display_mode *m = NULL;
    unsigned int i = 0;

    list_for_each_entry(m, &connector->modes, head) {
        if (i == mode) {
            return m;
        }
        i++;
    }
    return NULL;
}

static int mtk_panel_ext_param_set(struct drm_panel *panel,
            struct drm_connector *connector, unsigned int mode)
{
    struct mtk_panel_ext *ext = find_panel_ext(panel);
    int ret = 0;
    struct drm_display_mode *m = get_mode_by_id_hfp(connector, mode);

    if (m == NULL) {
        pr_err("%s: invalid display_mode\n", __func__);
        return -1;
    }

    if (drm_mode_vrefresh(m) == 60)
        ext->params = &ext_params;
    else if (drm_mode_vrefresh(m) == 90)
        ext->params = &ext_params_90hz;
    else
        ret = 1;

    if (!ret) {
        current_fps = drm_mode_vrefresh(m);
    }
    return ret;
}

static int panel_ext_reset(struct drm_panel *panel, int on)
{
    struct lcm *ctx = panel_to_lcm(panel);

    ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
    gpiod_set_value(ctx->reset_gpio, on);
    devm_gpiod_put(ctx->dev, ctx->reset_gpio);

    return 0;
}

static struct mtk_panel_funcs ext_funcs = {
    .reset = panel_ext_reset,
    .set_backlight_cmdq = lcm_setbacklight_cmdq,
    .ext_param_set = mtk_panel_ext_param_set,
};
#endif    // CONFIG_MTK_PANEL_EXT

struct panel_desc {
    const struct drm_display_mode *modes;
    unsigned int num_modes;

    unsigned int bpc;

    struct {
        unsigned int width;
        unsigned int height;
    } size;

    /**
     * @prepare: the time (in milliseconds) that it takes for the panel to
     *       become ready and start receiving video data
     * @enable: the time (in milliseconds) that it takes for the panel to
     *      display the first valid frame after starting to receive
     *      video data
     * @disable: the time (in milliseconds) that it takes for the panel to
     *       turn the display off (no content is visible)
     * @unprepare: the time (in milliseconds) that it takes for the panel
     *         to power itself down completely
     */
    struct {
        unsigned int prepare;
        unsigned int enable;
        unsigned int disable;
        unsigned int unprepare;
    } delay;
};

static int lcm_get_modes(struct drm_panel *panel,
                    struct drm_connector *connector)
{
    struct drm_display_mode *mode = NULL;
    struct drm_display_mode *mode2 = NULL;

    mode = drm_mode_duplicate(connector->dev, &default_mode);
    if (!mode) {
        dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
             default_mode.hdisplay, default_mode.vdisplay,
             drm_mode_vrefresh(&default_mode));
        return -ENOMEM;
    }

    drm_mode_set_name(mode);
    mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    drm_mode_probed_add(connector, mode);

    mode2 = drm_mode_duplicate(connector->dev, &performance_mode_90hz);
    if (!mode2) {
        dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
            performance_mode_90hz.hdisplay, performance_mode_90hz.vdisplay,
             drm_mode_vrefresh(&performance_mode_90hz));
        return -ENOMEM;
    }

    drm_mode_set_name(mode2);
    mode2->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    drm_mode_probed_add(connector, mode2);

    connector->display_info.width_mm = PHYSICAL_WIDTH / 1000;
    connector->display_info.height_mm = PHYSICAL_HEIGHT / 1000;

    return 2;
}

static const struct drm_panel_funcs lcm_drm_funcs = {
    .disable = lcm_disable,
    .unprepare = lcm_unprepare,
    .prepare = lcm_prepare,
    .enable = lcm_enable,
    .get_modes = lcm_get_modes,
};

static int lcm_probe(struct mipi_dsi_device *dsi)
{
    struct device *dev = &dsi->dev;
    struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
    struct lcm *ctx = NULL;
    struct device_node *backlight = NULL;
    int ret = 0;

    pr_info("%s+ lcm,hx83108d_xx_hkc\n", __func__);
    if (lcm_detect_panel("hx83108d_xx_hkc")) {
        pr_err("%s- current panel is not loaded\n", __func__);
        return -ENODEV;
    }
    dsi_node = of_get_parent(dev->of_node);
    if (dsi_node) {
        endpoint = of_graph_get_next_endpoint(dsi_node, NULL);
        if (endpoint) {
            remote_node = of_graph_get_remote_port_parent(endpoint);
            if (!remote_node) {
                pr_err("No panel connected,skip probe lcm\n");
                return -ENODEV;
            }
            pr_err("device node name:%s\n", remote_node->name);
        }
    }
    if (remote_node != dev->of_node) {
        pr_err("%s+ skip probe due to not current lcm\n", __func__);
        return -ENODEV;
    }

    ctx = devm_kzalloc(dev, sizeof(struct lcm), GFP_KERNEL);
    if (!ctx) {
        return -ENOMEM;
    }

    mipi_dsi_set_drvdata(dsi, ctx);

    ctx->dev = dev;
    dsi->lanes = 4;
    dsi->format = MIPI_DSI_FMT_RGB888;
    dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
            MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_EOT_PACKET | MIPI_DSI_MODE_VIDEO_BURST|
            MIPI_DSI_CLOCK_NON_CONTINUOUS;

    backlight = of_parse_phandle(dev->of_node, "backlight", 0);
    if (backlight) {
        ctx->backlight = of_find_backlight_by_node(backlight);
        of_node_put(backlight);

        if (!ctx->backlight) {
            return -EPROBE_DEFER;
        }
    }

    ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(ctx->reset_gpio)) {
        dev_err(dev, "cannot get lcd reset-gpios %ld\n",
             PTR_ERR(ctx->reset_gpio));
        return PTR_ERR(ctx->reset_gpio);
    }
    devm_gpiod_put(dev, ctx->reset_gpio);
    /*HS07 code for HS07-354 by huangyin at 20250407 start*/
    pinctl_rst = devm_pinctrl_get(ctx->dev);
    if (IS_ERR(pinctl_rst)) {
        pr_info("pinctl_tprst =%d\n",PTR_ERR(pinctl_rst));
        pinctl_rst = NULL;
    } else {
        tp_prst_low = pinctrl_lookup_state(pinctl_rst, "ctp_rst_pin_low");
        if (IS_ERR(tp_prst_low)) {
            pr_err("lookup state tp_prst_low failed\n");
            devm_pinctrl_put(pinctl_rst);
            pinctl_rst = NULL;
            return -ENODEV;
        }
        tp_prst_high = pinctrl_lookup_state(pinctl_rst, "ctp_rst_pin_high");
        if (IS_ERR(tp_prst_high)) {
            pr_err("lookup state tp_prst_high failed\n");
            devm_pinctrl_put(pinctl_rst);
            pinctl_rst = NULL;
            tp_prst_low = NULL;
            return -ENODEV;
        }
    }
    /*HS07 code for HS07-354 by huangyin at 20250407 start*/
    ctx->bias_pos = devm_gpiod_get_index(dev, "bias", 0, GPIOD_OUT_HIGH);
    if (IS_ERR(ctx->bias_pos)) {
        dev_err(dev, "cannot get bias-gpios 0 %ld\n",
                PTR_ERR(ctx->bias_pos));
        return PTR_ERR(ctx->bias_pos);
    }
    devm_gpiod_put(dev, ctx->bias_pos);

    ctx->bias_neg = devm_gpiod_get_index(dev, "bias", 1, GPIOD_OUT_HIGH);
    if (IS_ERR(ctx->bias_neg)) {
        dev_err(dev, "cannot get bias-gpios 1 %ld\n",
                PTR_ERR(ctx->bias_neg));
        return PTR_ERR(ctx->bias_neg);
    }
    devm_gpiod_put(dev, ctx->bias_neg);


    ctx->prepared = true;
    ctx->enabled = true;
    drm_panel_init(&ctx->panel, dev, &lcm_drm_funcs, DRM_MODE_CONNECTOR_DSI);

    drm_panel_add(&ctx->panel);

    ret = mipi_dsi_attach(dsi);
    if (ret < 0) {
        drm_panel_remove(&ctx->panel);
    }

#if defined(CONFIG_MTK_PANEL_EXT)
    mtk_panel_tch_handle_reg(&ctx->panel);
    ret = mtk_panel_ext_create(dev, &ext_params, &ext_funcs, &ctx->panel);
    if (ret < 0) {
        return ret;
    }

#endif    // CONFIG_MTK_PANEL_EXT

    pr_info("%s- lcm,hx83108d_xx_hkc\n", __func__);

    return ret;
}

static int lcm_remove(struct mipi_dsi_device *dsi)
{
    struct lcm *ctx = mipi_dsi_get_drvdata(dsi);
#if defined(CONFIG_MTK_PANEL_EXT)
    struct mtk_panel_ctx *ext_ctx = find_panel_ctx(&ctx->panel);
#endif    // CONFIG_MTK_PANEL_EXT
    /*HS07 code for HS07-354 by huangyin at 20250407 start*/
    devm_pinctrl_put(pinctl_rst);
    pinctl_rst = NULL;
    tp_prst_low = NULL;
    tp_prst_high = NULL;
    /*HS07 code for HS07-354 by huangyin at 20250407 end*/
    mipi_dsi_detach(dsi);
    drm_panel_remove(&ctx->panel);
#if defined(CONFIG_MTK_PANEL_EXT)
    mtk_panel_detach(ext_ctx);
    mtk_panel_remove(ext_ctx);
#endif    // CONFIG_MTK_PANEL_EXT

    return 0;
}

static const struct of_device_id lcm_of_match[] = {
    {
        .compatible = "panel,lcm,video",
    },
    {}
};

MODULE_DEVICE_TABLE(of, lcm_of_match);

static struct mipi_dsi_driver lcm_driver = {
    .probe = lcm_probe,
    .remove = lcm_remove,
    .driver = {
        .name = "lcd_hx83108d_xx_hkc_mipi_hd_video",
        .owner = THIS_MODULE,
        .of_match_table = lcm_of_match,
    },
};

module_mipi_dsi_driver(lcm_driver);

MODULE_AUTHOR("Elon Hsu <elon.hsu@mediatek.com>");
MODULE_DESCRIPTION("lcm hx83108d xx hkc Panel Driver");
MODULE_LICENSE("GPL v2");
