/* HS07 code for SR-AL7761A-01-164 by lina at 20250314 start */
// SPDX-License-Identifier: GPL-2.0
/**
 * @file   tcpc_scv89601p_pd.c
 * @author <boyu-wen@southchip>
 * @date   Feb 25 2025
 * @brief  Copyright (c) 20XX-2025 Southchip Semiconductor Technology(Shanghai) Co.,Ltd.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/pm_runtime.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/cpu.h>
#include <linux/version.h>
#include <linux/sched/clock.h>
#include <linux/power/gxy_psy_sysfs.h>
#include "inc/pd_dbg_info.h"
#include "inc/tcpci.h"
#include "inc/scv89601p_pd.h"
#ifdef SUPPORT_SOUTHCHIP_CPU_BOOST
//#include "../../cpu_boost.h"
#endif

#ifdef CONFIG_RT_REGMAP
#include "inc/rt-regmap.h"
#endif /* CONFIG_RT_REGMAP */

#define SCV89601P_PD_DRV_VER      "1.0"
#define SCV89601P_PD_IRQ_WAKE_TIME        (500) // ms

#ifndef CONFIG_SUPPORT_SOUTHCHIP_PDPHY
#define SOUTHCHIP_PD_VID        0x311C
#define SCV89601P_PD_PID        0x6610
#endif

#define SCV89601P_PD_ERR(fmt, ...)       pr_err("SCV89601P_PD_ERR:" fmt, ##__VA_ARGS__)
#define SCV89601P_PD_INFO(fmt, ...)      pr_err("SCV89601P_PD_INFO:" fmt, ##__VA_ARGS__)

#ifndef __maybe_unused
#define __maybe_unused
#endif

/*HS07 code for SR-AL7761A-01-20 by lina at 20250415 start*/
struct scv89601p_pd {
    struct i2c_client *client;
    struct i2c_adapter *adp;
    struct device *dev;
    #ifdef CONFIG_RT_REGMAP
    struct rt_regmap_device *m_dev;
#endif /* CONFIG_RT_REGMAP */
    struct tcpc_desc *tcpc_desc;
    struct tcpc_device *tcpc;

    int irq_gpio;
    int irq;
    int chip_id;
    uint16_t chip_pid;
    uint16_t chip_vid;
    bool vbus_present_status;
    bool vbus_present_alert;
    bool vbus_detect_thread;
    struct delayed_work vbus_detect_dwork;
    /* HST11 code for AX7800A-2024 by zhangziyi at 20250611 start */
    #if defined(CONFIG_CUSTOM_PROJECT_HST11)
    bool is_shutdown_flag;
    struct mutex pd_intr_lock;
    #endif //CONFIG_CUSTOM_PROJECT_HST11
    /* HST11 code for AX7800A-2024 by zhangziyi at 20250611 end */
};
/*HS07 code for SR-AL7761A-01-20 by lina at 20250415 end*/

#ifdef CONFIG_RT_REGMAP
RT_REG_DECL(TCPC_V10_REG_VID, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_PID, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_DID, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_TYPEC_REV, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_PD_REV, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_PDIF_REV, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_ALERT, 2, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_ALERT_MASK, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_POWER_STATUS_MASK, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_FAULT_STATUS_MASK, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_TCPC_CTRL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_ROLE_CTRL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_FAULT_CTRL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_POWER_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_CC_STATUS, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_POWER_STATUS, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_FAULT_STATUS, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_COMMAND, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_MSG_HDR_INFO, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_RX_DETECT, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_RX_BYTE_CNT, 4, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_RX_DATA, 28, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_TRANSMIT, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_TX_BYTE_CNT, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_TX_HDR, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_TX_DATA, 28, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(SCV89601P_PD_REG_ANA_CTRL1, 1, RT_VOLATILE, {});
RT_REG_DECL(SCV89601P_PD_REG_ANA_STATUS, 1, RT_VOLATILE, {});
RT_REG_DECL(SCV89601P_PD_REG_ANA_INT, 1, RT_VOLATILE, {});
RT_REG_DECL(SCV89601P_PD_REG_ANA_MASK, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(SCV89601P_PD_REG_ANA_CTRL2, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(SCV89601P_PD_REG_ANA_CTRL3, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(SCV89601P_PD_REG_RST_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(SCV89601P_PD_REG_DRP_CTRL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(SCV89601P_PD_REG_DRP_DUTY_CTRL, 2, RT_NORMAL_WR_ONCE, {});

static const rt_register_map_t scv89601p_pd_regmap[] = {
    RT_REG(TCPC_V10_REG_VID),
    RT_REG(TCPC_V10_REG_PID),
    RT_REG(TCPC_V10_REG_DID),
    RT_REG(TCPC_V10_REG_TYPEC_REV),
    RT_REG(TCPC_V10_REG_PD_REV),
    RT_REG(TCPC_V10_REG_PDIF_REV),
    RT_REG(TCPC_V10_REG_ALERT),
    RT_REG(TCPC_V10_REG_ALERT_MASK),
    RT_REG(TCPC_V10_REG_POWER_STATUS_MASK),
    RT_REG(TCPC_V10_REG_FAULT_STATUS_MASK),
    RT_REG(TCPC_V10_REG_TCPC_CTRL),
    RT_REG(TCPC_V10_REG_ROLE_CTRL),
    RT_REG(TCPC_V10_REG_FAULT_CTRL),
    RT_REG(TCPC_V10_REG_POWER_CTRL),
    RT_REG(TCPC_V10_REG_CC_STATUS),
    RT_REG(TCPC_V10_REG_POWER_STATUS),
    RT_REG(TCPC_V10_REG_FAULT_STATUS),
    RT_REG(TCPC_V10_REG_COMMAND),
    RT_REG(TCPC_V10_REG_MSG_HDR_INFO),
    RT_REG(TCPC_V10_REG_RX_DETECT),
    RT_REG(TCPC_V10_REG_RX_BYTE_CNT),
    RT_REG(TCPC_V10_REG_RX_DATA),
    RT_REG(TCPC_V10_REG_TRANSMIT),
    RT_REG(TCPC_V10_REG_TX_BYTE_CNT),
    RT_REG(TCPC_V10_REG_TX_HDR),
    RT_REG(TCPC_V10_REG_TX_DATA),
    RT_REG(SCV89601P_PD_REG_ANA_CTRL1),
    RT_REG(SCV89601P_PD_REG_ANA_STATUS),
    RT_REG(SCV89601P_PD_REG_ANA_INT),
    RT_REG(SCV89601P_PD_REG_ANA_MASK),
    RT_REG(SCV89601P_PD_REG_ANA_CTRL2),
    RT_REG(SCV89601P_PD_REG_ANA_CTRL3),
    RT_REG(SCV89601P_PD_REG_RST_CTRL),
    RT_REG(SCV89601P_PD_REG_DRP_CTRL),
    RT_REG(SCV89601P_PD_REG_DRP_DUTY_CTRL),
};
#define SCV89601P_PD_REGMAP_SIZE ARRAY_SIZE(scv89601p_pd_regmap)

#endif /* CONFIG_RT_REGMAP */

extern void gxy_bat_set_tcpcinfo(enum gxy_bat_tcpc_info tinfo_data);
static int scv89601p_pd_read_device(void *client, u32 reg, int len, void *dst)
{
    struct i2c_client *i2c = client;
    int ret = 0, count = 5;
    u64 t1 = 0, t2 = 0;

    while (1) {
        t1 = local_clock();
        ret = i2c_smbus_read_i2c_block_data(i2c, reg, len, dst);
        t2 = local_clock();
#if ENABLE_SCV89601P_PD_DBG
        SCV89601P_PD_INFO("%s del = %lluus, reg = 0x%02X, len = %d, val = 0x%08X\n",
                __func__, (t2 - t1) / NSEC_PER_USEC, reg, len, *(u16 *)dst);
#endif /* ENABLE_SCV89601P_PD_DBG */
        if (ret < 0 && count > 1)
            count--;
        else
            break;
        udelay(100);
    }
    return ret;
}


static int scv89601p_pd_write_device(void *client, u32 reg, int len, const void *src)
{
    struct i2c_client *i2c = client;
    int ret = 0, count = 5;
    u64 t1 = 0, t2 = 0;

    while (1) {
        t1 = local_clock();
        ret = i2c_smbus_write_i2c_block_data(i2c, reg, len, src);
        t2 = local_clock();
#if ENABLE_SCV89601P_PD_DBG
        SCV89601P_PD_INFO("%s del = %lluus, reg = %02X, len = %d, val = 0x%08X\n",
                __func__, (t2 - t1) / NSEC_PER_USEC, reg, len, *(u8 *)src);
#endif /* ENABLE_SCV89601P_PD_DBG */
        if (ret < 0 && count > 1)
            count--;
        else
            break;
        udelay(100);
    }
    return ret;
}

static int scv89601p_pd_reg_read(struct i2c_client *i2c, u8 reg)
{
    struct scv89601p_pd *sc = i2c_get_clientdata(i2c);
    u8 val = 0;
    int ret = 0;

#ifdef CONFIG_RT_REGMAP
    ret = rt_regmap_block_read(sc->m_dev, reg, 1, &val);
#else
    ret = scv89601p_pd_read_device(sc->client, reg, 1, &val);
#endif /* CONFIG_RT_REGMAP */
    if (ret < 0) {
        SCV89601P_PD_ERR("scv89601p_pd reg read fail\n");
        return ret;
    }
    return val;
}

static int scv89601p_pd_reg_write(struct i2c_client *i2c, u8 reg, const u8 data)
{
    struct scv89601p_pd *sc = i2c_get_clientdata(i2c);
    int ret = 0;

#ifdef CONFIG_RT_REGMAP
    ret = rt_regmap_block_write(sc->m_dev, reg, 1, &data);
#else
    ret = scv89601p_pd_write_device(sc->client, reg, 1, &data);
#endif /* CONFIG_RT_REGMAP */
    if (ret < 0)
        SCV89601P_PD_ERR("scv89601p_pd reg write fail\n");
    return ret;
}

static int scv89601p_pd_block_read(struct i2c_client *i2c,
            u8 reg, int len, void *dst)
{
    struct scv89601p_pd *sc = i2c_get_clientdata(i2c);
    int ret = 0;
#ifdef CONFIG_RT_REGMAP
    ret = rt_regmap_block_read(sc->m_dev, reg, len, dst);
#else
    ret = scv89601p_pd_read_device(sc->client, reg, len, dst);
#endif /* #ifdef CONFIG_RT_REGMAP */
    if (ret < 0)
        SCV89601P_PD_ERR("scv89601p_pd block read fail\n");
    return ret;
}

static int scv89601p_pd_block_write(struct i2c_client *i2c,
            u8 reg, int len, const void *src)
{
    struct scv89601p_pd *sc = i2c_get_clientdata(i2c);
    int ret = 0;
#ifdef CONFIG_RT_REGMAP
    ret = rt_regmap_block_write(sc->m_dev, reg, len, src);
#else
    ret = scv89601p_pd_write_device(sc->client, reg, len, src);
#endif /* #ifdef CONFIG_RT_REGMAP */
    if (ret < 0)
        SCV89601P_PD_ERR("scv89601p_pd block write fail\n");
    return ret;
}


static int32_t scv89601p_pd_write_word(struct i2c_client *client,
                    uint8_t reg_addr, uint16_t data)
{
    int ret;

    /* don't need swap */
    ret = scv89601p_pd_block_write(client, reg_addr, 2, (uint8_t *)&data);
    return ret;
}

static int32_t scv89601p_pd_read_word(struct i2c_client *client,
                    uint8_t reg_addr, uint16_t *data)
{
    int ret;

    /* don't need swap */
    ret = scv89601p_pd_block_read(client, reg_addr, 2, (uint8_t *)data);
    return ret;
}

static inline int scv89601p_pd_i2c_write8(
    struct tcpc_device *tcpc, u8 reg, const u8 data)
{
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);

    return scv89601p_pd_reg_write(sc->client, reg, data);
}

static inline int scv89601p_pd_i2c_write16(
        struct tcpc_device *tcpc, u8 reg, const u16 data)
{
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);

    return scv89601p_pd_write_word(sc->client, reg, data);
}

static inline int scv89601p_pd_i2c_read8(struct tcpc_device *tcpc, u8 reg)
{
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);

    return scv89601p_pd_reg_read(sc->client, reg);
}

static inline int scv89601p_pd_i2c_read16(
    struct tcpc_device *tcpc, u8 reg)
{
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);
    u16 data;
    int ret;

    ret = scv89601p_pd_read_word(sc->client, reg, &data);
    if (ret < 0)
        return ret;
    return data;
}

#ifdef CONFIG_RT_REGMAP
static struct rt_regmap_fops scv89601p_pd_regmap_fops = {
    .read_device = scv89601p_pd_read_device,
    .write_device = scv89601p_pd_write_device,
};
#endif /* CONFIG_RT_REGMAP */

static int scv89601p_pd_regmap_init(struct scv89601p_pd *sc)
{
#ifdef CONFIG_RT_REGMAP
    struct rt_regmap_properties *props;
    char name[32];
    int len;

    props = devm_kzalloc(sc->dev, sizeof(*props), GFP_KERNEL);
    if (!props)
        return -ENOMEM;

    props->register_num = SCV89601P_PD_REGMAP_SIZE;
    props->rm = scv89601p_pd_regmap;

    props->rt_regmap_mode = RT_MULTI_BYTE |
                RT_IO_PASS_THROUGH | RT_DBG_SPECIAL;
    snprintf(name, sizeof(name), "scv89601p_pd-%02x", sc->client->addr);

    len = strlen(name);
    props->name = kzalloc(len+1, GFP_KERNEL);
    props->aliases = kzalloc(len+1, GFP_KERNEL);

    if ((!props->name) || (!props->aliases))
        return -ENOMEM;

    strlcpy((char *)props->name, name, len+1);
    strlcpy((char *)props->aliases, name, len+1);
    props->io_log_en = 0;

    sc->m_dev = rt_regmap_device_register(props,
            &scv89601p_pd_regmap_fops, sc->dev, sc->client, sc);
    if (!sc->m_dev) {
        SCV89601P_PD_ERR("scv89601p_pd rt_regmap register fail\n");
        return -EINVAL;
    }
#endif
    return 0;
}

static int scv89601p_pd_regmap_deinit(struct scv89601p_pd *sc)
{
#ifdef CONFIG_RT_REGMAP
    rt_regmap_device_unregister(sc->m_dev);
#endif
    return 0;
}

__maybe_unused
static int scv89601p_pd_test_mode(struct scv89601p_pd *sc)
{
    return 0;
}

static inline int scv89601p_pd_software_reset(struct tcpc_device *tcpc)
{
    int ret = scv89601p_pd_i2c_write8(tcpc, SCV89601P_PD_REG_RST_CTRL, 1);
#ifdef CONFIG_RT_REGMAP
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);
#endif /* CONFIG_RT_REGMAP */

    if (ret < 0)
        return ret;
#ifdef CONFIG_RT_REGMAP
    rt_regmap_cache_reload(sc->m_dev);
#endif /* CONFIG_RT_REGMAP */
    usleep_range(1000, 2000);
    return 0;
}

static inline int scv89601p_pd_command(struct tcpc_device *tcpc, uint8_t cmd)
{
    return scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_COMMAND, cmd);
}

/* HST11 code for AX7800A-2331 by zhangziyi at 20250618 start */
#if defined(CONFIG_CUSTOM_PROJECT_HST11)
static int scv89601p_set_key(struct scv89601p_pd *sc, bool en)
{
    struct i2c_msg msg;
    uint8_t buf[2];
    int ret;
    int i = 0;
    uint8_t values[] = {0x44, 0x49, 0x53, 0x43, 0x4f, 0x56,
                     0x45, 0x52, 0x59, 0x50};

    buf[0] = 0x5d;

    if (en) {
        for (i = 0; i < ARRAY_SIZE(values); i++)
        {
            buf[1] = values[i];
            msg.addr = 0x6b;
            msg.flags = 0;
            msg.buf = buf;
            msg.len = 2;
            ret = i2c_transfer(sc->adp, &msg, 1);
        }
    } else {
            buf[1] = 0x04;
            msg.addr = 0x6b;
            msg.flags = 0;
            msg.buf = buf;
            msg.len = 2;
            ret = i2c_transfer(sc->adp, &msg, 1);
    }

    if (ret != 1) {
        pr_err("Transfer failed: %d\n", ret);
        return (ret < 0) ? ret : -EIO;
    }
    return 0;
}

static int scv89601p_set_bmc(struct scv89601p_pd *sc)
{
    uint8_t data[5] = {78, 134, 61, 60, 22};
    uint8_t val;
    scv89601p_set_key(sc, true);
    scv89601p_pd_write_device(sc->client, 0xE7, 1, &data[0]);
    scv89601p_pd_write_device(sc->client, 0xE8, 1, &data[1]);
    scv89601p_pd_write_device(sc->client, 0xE9, 1, &data[2]);
    scv89601p_pd_write_device(sc->client, 0xEA, 1, &data[3]);
    scv89601p_pd_write_device(sc->client, 0xEB, 1, &data[4]);
    scv89601p_pd_read_device(sc->client, 0xE8, 1, &val);
    pr_info("%s %x", __func__, val);
    scv89601p_set_key(sc, false);
    return 0;
}
#endif //CONFIG_CUSTOM_PROJECT_HST11
/* HST11 code for AX7800A-2331 by zhangziyi at 20250618 end */

static int scv89601p_pd_init_alert_mask(struct tcpc_device *tcpc)
{
    uint16_t mask;

    mask = TCPC_V10_REG_ALERT_CC_STATUS | TCPC_V10_REG_ALERT_POWER_STATUS;

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
    /* Need to handle RX overflow */
    mask |= TCPC_V10_REG_ALERT_TX_SUCCESS | TCPC_V10_REG_ALERT_TX_DISCARDED
            | TCPC_V10_REG_ALERT_TX_FAILED
            | TCPC_V10_REG_ALERT_RX_HARD_RST
            | TCPC_V10_REG_ALERT_RX_STATUS
            | TCPC_V10_REG_RX_OVERFLOW;
#endif

    mask |= TCPC_REG_ALERT_FAULT;

    return scv89601p_pd_i2c_write16(tcpc, TCPC_V10_REG_ALERT_MASK, mask);
}

static int scv89601p_pd_init_power_status_mask(struct tcpc_device *tcpc)
{
    const uint8_t mask = TCPC_V10_REG_POWER_STATUS_VBUS_PRES;

    return scv89601p_pd_i2c_write8(tcpc,
            TCPC_V10_REG_POWER_STATUS_MASK, mask);
}

static int scv89601p_pd_init_fault_mask(struct tcpc_device *tcpc)
{
    const uint8_t mask =
        TCPC_V10_REG_FAULT_STATUS_VCONN_OV |
        TCPC_V10_REG_FAULT_STATUS_VCONN_OC;

    return scv89601p_pd_i2c_write8(tcpc,
            TCPC_V10_REG_FAULT_STATUS_MASK, mask);
}

static inline int scv89601p_pd_init_prv_mask(struct tcpc_device *tcpc)
{
    return scv89601p_pd_i2c_write8(tcpc, SCV89601P_PD_REG_ANA_MASK, SCV89601P_PD_REG_MASK_VBUS_80);
}

/*HS07 code for SR-AL7761A-01-20 by lina at 20250415 start*/
static int scv89601p_get_adc_en(struct scv89601p_pd *sc, int *en)
{
    int ret = 0;
    unsigned char adc_en;
    unsigned char adc_reg = 0x0F;
    struct i2c_msg xfer[2];

    xfer[0].addr    = 0x6b;
    xfer[0].flags   = 0;
    xfer[0].len     = 1;
    xfer[0].buf     = &adc_reg;

    xfer[1].addr    = 0x6b;
    xfer[1].flags   = I2C_M_RD;
    xfer[1].len     = 1;
    xfer[1].buf     = &adc_en;
    ret = i2c_transfer(sc->adp, xfer, 2);
    if (ret < 0) {
        pr_info("Error: %d\n", ret);
    }
    *en = adc_en;
    return ret;
}

static int scv89601p_set_adc_en(struct scv89601p_pd *sc, bool en)
{
    int ret = 0;
    uint8_t en_adc[] = {0x0F, 0x80};
    uint8_t dis_adc[] = {0x0F, 0x00};
    int adc_reg_val = 0;
    struct i2c_msg msg = {
        .addr = 0x6b,
        .flags = 0,
    };

    /* HST11 code for AX7800A-2024 by zhangziyi at 20250611 start */
    #if defined(CONFIG_CUSTOM_PROJECT_HST11)
    if (sc->is_shutdown_flag) {
        SCV89601P_PD_INFO("will shut down, return\n");
        return ret;
    }
    #endif //CONFIG_CUSTOM_PROJECT_HST11
    /* HST11 code for AX7800A-2024 by zhangziyi at 20250611 end */

    scv89601p_get_adc_en(sc, &adc_reg_val);
    if (en) {
        if (!(adc_reg_val & 0x80)) {
            msg.len = sizeof(en_adc);
            msg.buf = en_adc;
            ret = i2c_transfer(sc->adp, &msg, 1);
            pr_info("%s %d", __func__, en);
        }
    } else {
        if (adc_reg_val & 0x80) {
            msg.len = sizeof(dis_adc);
            msg.buf = dis_adc;
            ret = i2c_transfer(sc->adp, &msg, 1);
            pr_info("%s %d", __func__, en);
        }
    }
    return ret;
}

static int scv89601p_get_vbus_adc(struct scv89601p_pd *sc, int *vbus)
{
    int ret = 0;
    unsigned char vbus_reg_val;
    unsigned char vbus_reg = 0x14;
    struct i2c_msg xfer[2];

    xfer[0].addr    = 0x6b;
    xfer[0].flags   = 0;
    xfer[0].len     = 1;
    xfer[0].buf     = &vbus_reg;

    xfer[1].addr    = 0x6b;
    xfer[1].flags   = I2C_M_RD;
    xfer[1].len     = 1;
    xfer[1].buf     = &vbus_reg_val;
    ret = i2c_transfer(sc->adp, xfer, 2);
    if (ret < 0) {
        pr_info("Error: %d\n", ret);
    }
    *vbus = vbus_reg_val*60;
    return ret;
}

/* HST11 code for AX7800A-2024 by zhangziyi at 20250611 start */
static irqreturn_t scv89601p_pd_intr_handler(int irq, void *data)
{
    struct scv89601p_pd *sc = data;
    int cc_status, power_status;
    int vbus = 0;
    bool vbus_stat = false;

    #if defined(CONFIG_CUSTOM_PROJECT_HST11)
    mutex_lock(&sc->pd_intr_lock);
    #endif //CONFIG_CUSTOM_PROJECT_HST11

#ifdef SUPPORT_SOUTHCHIP_CPU_BOOST
    //sc_cpufreq_update(CPU_CHG_FREQ_STAT_UP);
#endif
    pm_wakeup_event(sc->dev, SCV89601P_PD_IRQ_WAKE_TIME);
    /*HS07 code for HS07-2865 by lina at 20250512 start*/
    tcpci_lock_typec(sc->tcpc);
    tcpci_alert(sc->tcpc);
    tcpci_unlock_typec(sc->tcpc);
    /*HS07 code for HS07-2865 by lina at 20250512 end*/

    cc_status = scv89601p_pd_i2c_read8(sc->tcpc, TCPC_V10_REG_CC_STATUS);
    power_status = scv89601p_pd_i2c_read8(sc->tcpc, TCPC_V10_REG_POWER_STATUS);

    if ((cc_status & 0x0F) && (!(power_status & 0x04))&&(!(cc_status&0x20))) {
        sc->vbus_detect_thread = true;
        scv89601p_set_adc_en(sc, true);
        scv89601p_get_vbus_adc(sc, &vbus);
        vbus_stat = vbus > 3900 ? true : false;
        if (sc->vbus_present_status != vbus_stat) {
            sc->vbus_present_status = vbus_stat;
            sc->vbus_present_alert = true;

            SCV89601P_PD_INFO("scv89601p_pd_set_vbus_present: %d\n", sc->vbus_present_status);
        }
        schedule_delayed_work(&sc->vbus_detect_dwork, msecs_to_jiffies(0));
    } else {
        sc->vbus_detect_thread = false;
        sc->vbus_present_status = false;
        /*HS07 code for HS07-445 by lina at 20250417 start*/
        sc->vbus_present_alert = true;
        /*HS07 code for HS07-445 by lina at 20250417 end*/
        scv89601p_set_adc_en(sc, false);
    }

    #if defined(CONFIG_CUSTOM_PROJECT_HST11)
    mutex_unlock(&sc->pd_intr_lock);
    #endif //CONFIG_CUSTOM_PROJECT_HST11

    return IRQ_HANDLED;
}
/* HST11 code for AX7800A-2024 by zhangziyi at 20250611 end */

static void scv89601p_vbus_detect_work(struct work_struct *work)
{
    struct scv89601p_pd *sc = container_of(work,
                                    struct scv89601p_pd, vbus_detect_dwork.work);
    int vbus = 0;
    bool vbus_stat = false;
    if (sc->vbus_detect_thread == false) {
        /*HS07 code for HS07-445 by lina at 20250417 start*/
        goto out;
        /*HS07 code for HS07-445 by lina at 20250417 end*/
    }

    scv89601p_get_vbus_adc(sc, &vbus);
    vbus_stat = vbus > 3900 ? true : false;

    if (sc->vbus_present_status == vbus_stat) {
        goto out;
    }

    sc->vbus_present_status = vbus_stat;
    sc->vbus_present_alert = true;

    SCV89601P_PD_INFO("scv89601p_pd_set_vbus_present %d\n", sc->vbus_present_status);
    scv89601p_pd_intr_handler(sc->irq, (void *)sc);
out:
    if (sc->vbus_detect_thread) {
        schedule_delayed_work(&sc->vbus_detect_dwork, msecs_to_jiffies(5));
    } else {
        sc->vbus_present_status = false;
    }
}
/*HS07 code for SR-AL7761A-01-20 by lina at 20250415 end*/

static int scv89601p_pd_init_alert(struct tcpc_device *tcpc)
{
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);
    int ret = 0;
    char *name = NULL;

    /* Clear Alert Mask & Status */
    scv89601p_pd_write_word(sc->client, TCPC_V10_REG_ALERT_MASK, 0);
    scv89601p_pd_write_word(sc->client, TCPC_V10_REG_ALERT, 0xffff);
    scv89601p_pd_reg_write(sc->client, SCV89601P_PD_REG_ANA_MASK, 0);
    scv89601p_pd_reg_write(sc->client, SCV89601P_PD_REG_ANA_INT, 0xff);
    scv89601p_pd_reg_write(sc->client, TCPC_V10_REG_FAULT_STATUS, 0xff);

    name = devm_kasprintf(sc->dev, GFP_KERNEL, "%s-IRQ",
                sc->tcpc_desc->name);
    if (!name)
        return -ENOMEM;

    SCV89601P_PD_INFO("%s name = %s, gpio = %d\n",
                __func__, sc->tcpc_desc->name, sc->irq_gpio);

    ret = devm_gpio_request(sc->dev, sc->irq_gpio, name);

    if (ret < 0) {
        SCV89601P_PD_ERR("%s request GPIO fail(%d)\n",
                    __func__, ret);
        return ret;
    }

    ret = gpio_direction_input(sc->irq_gpio);
    if (ret < 0) {
        SCV89601P_PD_ERR("%s set GPIO fail(%d)\n", __func__, ret);
        return ret;
    }

    ret = gpio_to_irq(sc->irq_gpio);
    if (ret < 0) {
        SCV89601P_PD_ERR("%s gpio to irq fail(%d)",
                    __func__, ret);
        return ret;
    }
    sc->irq = ret;

    SCV89601P_PD_INFO("%s IRQ number = %d\n", __func__, sc->irq);

    ret = devm_request_threaded_irq(sc->dev, sc->irq, NULL,
                    scv89601p_pd_intr_handler,
                    IRQF_TRIGGER_LOW | IRQF_ONESHOT,
                    name, sc);
    if (ret < 0) {
        SCV89601P_PD_ERR("%s request irq fail(%d)\n",
                    __func__, ret);
        return ret;
    }
    device_init_wakeup(sc->dev, true);

    return 0;
}

/*HS07 code for SR-AL7761A-01-20 by lina at 20250415 start*/
int scv89601p_pd_alert_status_clear(struct tcpc_device *tcpc, uint32_t mask)
{
    int ret;
    uint16_t mask_t1;
    uint8_t mask_t2;
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);

    mask_t1 = mask;
    if (mask_t1) {
        ret = scv89601p_pd_i2c_write16(tcpc, TCPC_V10_REG_ALERT, mask_t1);
        if (ret < 0)
            return ret;
    }

    if (sc->vbus_present_alert) {
        sc->vbus_present_alert = false;
    }

    mask_t2 = mask >> 16;
    if (mask_t2) {
        ret = scv89601p_pd_i2c_write8(tcpc, SCV89601P_PD_REG_ANA_INT, mask_t2);
        if (ret < 0)
            return ret;
    }
    return 0;
}
/*HS07 code for SR-AL7761A-01-20 by lina at 20250415 end*/

static int scv89601p_pd_tcpc_init(struct tcpc_device *tcpc, bool sw_reset)
{
    int ret;

    if (sw_reset) {
        ret = scv89601p_pd_software_reset(tcpc);
        if (ret < 0)
            return ret;
    }
    //scv89601p_pd_init_iicrst(tcpc);
    /* UFP Both RD setting */
    /* DRP = 0, RpVal = 0 (Default), Rd, Rd */
    scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_ROLE_CTRL,
        TCPC_V10_REG_ROLE_CTRL_RES_SET(0, 0, CC_RD, CC_RD));

    scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_COMMAND,
        TCPM_CMD_ENABLE_VBUS_DETECT);

    /*
    * DRP Toggle Cycle : 51.2 + 6.4*val ms
    * DRP Duty Ctrl : dcSRC / 1024
    */
    scv89601p_pd_i2c_write8(tcpc, SCV89601P_PD_REG_DRP_CTRL, 4);
    scv89601p_pd_i2c_write16(tcpc,
        SCV89601P_PD_REG_DRP_DUTY_CTRL, TCPC_NORMAL_RP_DUTY);

    scv89601p_pd_alert_status_clear(tcpc, 0xffffffff);

    scv89601p_pd_init_power_status_mask(tcpc);
    scv89601p_pd_init_alert_mask(tcpc);
    scv89601p_pd_init_fault_mask(tcpc);
    scv89601p_pd_init_prv_mask(tcpc);

    /* shutdown off */
    scv89601p_pd_i2c_write8(tcpc, SCV89601P_PD_REG_ANA_CTRL2, SCV89601P_PD_REG_SHUTDOWN_OFF);
    mdelay(1);

    return 0;
}

#ifdef CONFIG_SUPPORT_SOUTHCHIP_PDPHY
static int scv89601p_pd_get_chip_id(struct tcpc_device *tcpc,uint32_t *id)
{
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);
    *id = sc->chip_id;
    return 0;
}

static int scv89601p_pd_get_chip_pid(struct tcpc_device *tcpc,uint32_t *pid)
{
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);
    *pid = sc->chip_pid;
    return 0;
}

static int scv89601p_pd_get_chip_vid(struct tcpc_device *tcpc,uint32_t *vid)
{
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);
    *vid = sc->chip_vid;
    return 0;
}
#endif /* CONFIG_SUPPORT_SOUTHCHIP_PDPHY */

static inline int scv89601p_pd_fault_status_vconn_ov(struct tcpc_device *tcpc)
{
    int ret;

    ret = scv89601p_pd_i2c_read8(tcpc, SCV89601P_PD_REG_ANA_CTRL1);
    if (ret < 0)
        return ret;

    ret &= ~SCV89601P_PD_REG_VCONN_DISCHARGE_EN;
    return scv89601p_pd_i2c_write8(tcpc, SCV89601P_PD_REG_ANA_CTRL1, ret);
}

static int scv89601p_pd_set_vconn(struct tcpc_device *tcpc, int enable);
static int scv89601p_pd_fault_status_clear(struct tcpc_device *tcpc, uint8_t status)
{
    int ret;

    if (status & TCPC_V10_REG_FAULT_STATUS_VCONN_OV)
        ret = scv89601p_pd_fault_status_vconn_ov(tcpc);
    if (status & TCPC_V10_REG_FAULT_STATUS_VCONN_OC)
        ret = scv89601p_pd_set_vconn(tcpc, false);

    scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_FAULT_STATUS, status);
    return 0;
}

static int scv89601p_pd_get_alert_mask(struct tcpc_device *tcpc, uint32_t *mask)
{
    int ret;
    uint8_t v2;

    ret = scv89601p_pd_i2c_read16(tcpc, TCPC_V10_REG_ALERT_MASK);
    if (ret < 0)
        return ret;

    *mask = (uint16_t) ret;

    ret = scv89601p_pd_i2c_read8(tcpc, SCV89601P_PD_REG_ANA_MASK);
    if (ret < 0)
        return ret;

    v2 = (uint8_t) ret;
    *mask |= v2 << 16;

    return 0;
}

/*HS07 code for SR-AL7761A-01-20 by lina at 20250415 start*/
int scv89601p_pd_get_alert_status(struct tcpc_device *tcpc, uint32_t *alert)
{
    int ret;
    uint8_t v2;
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);

    ret = scv89601p_pd_i2c_read16(tcpc, TCPC_V10_REG_ALERT);
    if (ret < 0)
        return ret;

    *alert = (uint16_t) ret;

    ret = scv89601p_pd_i2c_read8(tcpc, SCV89601P_PD_REG_ANA_INT);
    if (ret < 0)
        return ret;

    v2 = (uint8_t) ret;
    *alert |= v2 << 16;

    if (sc->vbus_present_alert) {
        *alert |= TCPC_V10_REG_ALERT_POWER_STATUS;
    }

    return 0;
}

static int scv89601p_pd_get_power_status(
        struct tcpc_device *tcpc, uint16_t *pwr_status)
{
    int ret;
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);

    ret = scv89601p_pd_i2c_read8(tcpc, TCPC_V10_REG_POWER_STATUS);
    if (ret < 0)
        return ret;

    *pwr_status = 0;

    if ((ret & TCPC_V10_REG_POWER_STATUS_VBUS_PRES) || sc->vbus_present_status == true)
        *pwr_status |= TCPC_REG_POWER_STATUS_VBUS_PRES;

    ret = scv89601p_pd_i2c_read8(tcpc, SCV89601P_PD_REG_ANA_STATUS);
    if (ret < 0)
        return ret;

    if (ret & SCV89601P_PD_REG_VBUS_80){
        *pwr_status |= TCPC_REG_POWER_STATUS_EXT_VSAFE0V;

        sc->vbus_present_status = false;
    }
    pr_info("vbus_present_status %d pwr_status: %2x",sc->vbus_present_status, *pwr_status);
    return 0;
}
/*HS07 code for SR-AL7761A-01-20 by lina at 20250415 end*/

int scv89601p_pd_get_fault_status(struct tcpc_device *tcpc, uint8_t *status)
{
    int ret;

    ret = scv89601p_pd_i2c_read8(tcpc, TCPC_V10_REG_FAULT_STATUS);
    if (ret < 0)
        return ret;
    *status = (uint8_t) ret;
    return 0;
}

static int scv89601p_pd_get_cc(struct tcpc_device *tcpc, int *cc1, int *cc2)
{
    int status, role_ctrl, cc_role;
    bool act_as_sink, act_as_drp;

    status = scv89601p_pd_i2c_read8(tcpc, TCPC_V10_REG_CC_STATUS);
    if (status < 0)
        return status;

    role_ctrl = scv89601p_pd_i2c_read8(tcpc, TCPC_V10_REG_ROLE_CTRL);
    if (role_ctrl < 0)
        return role_ctrl;

    if (status & TCPC_V10_REG_CC_STATUS_DRP_TOGGLING) {
        *cc1 = TYPEC_CC_DRP_TOGGLING;
        *cc2 = TYPEC_CC_DRP_TOGGLING;
        return 0;
    }

    *cc1 = TCPC_V10_REG_CC_STATUS_CC1(status);
    *cc2 = TCPC_V10_REG_CC_STATUS_CC2(status);

    act_as_drp = TCPC_V10_REG_ROLE_CTRL_DRP & role_ctrl;

    if (act_as_drp) {
        act_as_sink = TCPC_V10_REG_CC_STATUS_DRP_RESULT(status);
    } else {
        if (tcpc->typec_polarity)
            cc_role = TCPC_V10_REG_CC_STATUS_CC2(role_ctrl);
        else
            cc_role = TCPC_V10_REG_CC_STATUS_CC1(role_ctrl);
        if (cc_role == TYPEC_CC_RP)
            act_as_sink = false;
        else
            act_as_sink = true;
    }

    /**
    * cc both connect
    */
    if (act_as_drp && act_as_sink) {
        if ((*cc1 + *cc2) > (2 * TYPEC_CC_VOLT_RA)) {
            if (*cc1 == TYPEC_CC_VOLT_RA)
                *cc1 = TYPEC_CC_VOLT_OPEN;
            if (*cc2 == TYPEC_CC_VOLT_RA)
                *cc2 = TYPEC_CC_VOLT_OPEN;
        }
    }

    if (*cc1 != TYPEC_CC_VOLT_OPEN)
        *cc1 |= (act_as_sink << 2);

    if (*cc2 != TYPEC_CC_VOLT_OPEN)
        *cc2 |= (act_as_sink << 2);

    return 0;
}

static int scv89601p_pd_enable_vsafe0v_detect(
    struct tcpc_device *tcpc, bool enable)
{
    int ret = scv89601p_pd_i2c_read8(tcpc, SCV89601P_PD_REG_ANA_MASK);

    if (ret < 0)
        return ret;

    if (enable)
        ret |= SCV89601P_PD_REG_MASK_VBUS_80;
    else
        ret &= ~SCV89601P_PD_REG_MASK_VBUS_80;

    return scv89601p_pd_i2c_write8(tcpc, SCV89601P_PD_REG_ANA_MASK, (uint8_t) ret);
}

static int scv89601p_pd_set_cc(struct tcpc_device *tcpc, int pull)
{
    int ret = 0;
    uint8_t data = 0, old_data = 0;
    int rp_lvl = TYPEC_CC_PULL_GET_RP_LVL(pull), pull1, pull2;

    pull = TYPEC_CC_PULL_GET_RES(pull);

    old_data = scv89601p_pd_i2c_read8(tcpc, TCPC_V10_REG_ROLE_CTRL);

    if (pull == TYPEC_CC_DRP) {
        data = TCPC_V10_REG_ROLE_CTRL_RES_SET(
                1, rp_lvl, TYPEC_CC_RP, TYPEC_CC_RP);

        if (old_data != data) {
            ret = scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_ROLE_CTRL, data);
        }

        if (ret == 0) {
            scv89601p_pd_enable_vsafe0v_detect(tcpc, false);
            ret = scv89601p_pd_command(tcpc, TCPM_CMD_LOOK_CONNECTION);
        }
    } else {
        pull1 = pull2 = pull;

        if (pull == TYPEC_CC_RP && tcpc->typec_is_attached_src) {
            if (tcpc->typec_polarity)
                pull1 = TYPEC_CC_OPEN;
            else
                pull2 = TYPEC_CC_OPEN;
        }
        data = TCPC_V10_REG_ROLE_CTRL_RES_SET(0, rp_lvl, pull1, pull2);
        if (old_data != data) {
            ret = scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_ROLE_CTRL, data);
        }
    }

    return 0;
}

static int scv89601p_pd_set_polarity(struct tcpc_device *tcpc, int polarity)
{
    int data;

    data = scv89601p_pd_i2c_read8(tcpc, TCPC_V10_REG_TCPC_CTRL);
    if (data < 0)
        return data;

    data &= ~TCPC_V10_REG_TCPC_CTRL_PLUG_ORIENT;
    data |= polarity ? TCPC_V10_REG_TCPC_CTRL_PLUG_ORIENT : 0;

    return scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_TCPC_CTRL, data);
}

static int scv89601p_pd_set_low_rp_duty(struct tcpc_device *tcpc, bool low_rp)
{
    uint16_t duty = low_rp ? TCPC_LOW_RP_DUTY : TCPC_NORMAL_RP_DUTY;

    return scv89601p_pd_i2c_write16(tcpc, SCV89601P_PD_REG_DRP_DUTY_CTRL, duty);
}

static int scv89601p_pd_set_vconn(struct tcpc_device *tcpc, int enable)
{
    int rv;
    int data;

    data = scv89601p_pd_i2c_read8(tcpc, TCPC_V10_REG_POWER_CTRL);
    if (data < 0)
        return data;

    data &= ~TCPC_V10_REG_POWER_CTRL_VCONN;
    data |= enable ? TCPC_V10_REG_POWER_CTRL_VCONN : 0;

    rv = scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_POWER_CTRL, data);
    if (rv < 0)
        return rv;

    return rv;
}

#ifdef CONFIG_TCPC_LOW_POWER_MODE
static int scv89601p_pd_is_low_power_mode(struct tcpc_device *tcpc)
{
    int rv = scv89601p_pd_i2c_read8(tcpc, SCV89601P_PD_REG_ANA_CTRL1);
    if (rv < 0)
        return rv;

    return (rv & SCV89601P_PD_REG_LPM_EN) != 0;
}

static int scv89601p_pd_set_low_power_mode(
        struct tcpc_device *tcpc, bool en, int pull)
{
    uint8_t data = 0;

    scv89601p_pd_enable_vsafe0v_detect(tcpc, !en);
    data = scv89601p_pd_i2c_read8(tcpc, SCV89601P_PD_REG_ANA_CTRL1);
    if(data < 0) {
        return data;
    }
    if(en) {
        data |= SCV89601P_PD_REG_LPM_EN;
    } else {
        data &= ~SCV89601P_PD_REG_LPM_EN;
    }

    return scv89601p_pd_i2c_write8(tcpc, SCV89601P_PD_REG_ANA_CTRL1, data);
}
#endif	/* CONFIG_TCPC_LOW_POWER_MODE */

#ifdef CONFIG_TCPC_WATCHDOG_EN
int scv89601p_pd_set_watchdog(struct tcpc_device *tcpc, bool en)
{
    uint8_t data = 0;

    data = scv89601p_pd_i2c_read8(tcpc, TCPC_V10_REG_TCPC_CTRL);
    if (data < 0)
        return data;
    if (en) {
        data |= TCPC_V10_REG_TCPC_CTRL_EN_WDT;
    } else {
        data &= (~TCPC_V10_REG_TCPC_CTRL_EN_WDT);
    }
    SCV89601P_PD_INFO("%s set watchdog %d\n", __func__, en);
    return scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_TCPC_CTRL, data);
}
#endif	/* CONFIG_TCPC_WATCHDOG_EN */

static int scv89601p_pd_tcpc_deinit(struct tcpc_device *tcpc)
{
#ifdef CONFIG_RT_REGMAP
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);
#endif /* CONFIG_RT_REGMAP */

#ifdef CONFIG_TCPC_SHUTDOWN_CC_DETACH
    scv89601p_pd_set_cc(tcpc, TYPEC_CC_DRP);
    scv89601p_pd_set_cc(tcpc, TYPEC_CC_OPEN);
    mdelay(150);
    scv89601p_pd_i2c_write8(tcpc, SCV89601P_PD_REG_RST_CTRL, 1);
#else
    scv89601p_pd_i2c_write8(tcpc, SCV89601P_PD_REG_RST_CTRL, 1);
#endif	/* CONFIG_TCPC_SHUTDOWN_CC_DETACH */
#ifdef CONFIG_RT_REGMAP
    rt_regmap_cache_reload(sc->m_dev);
#endif /* CONFIG_RT_REGMAP */

    return 0;
}

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
static int scv89601p_pd_set_msg_header(
    struct tcpc_device *tcpc, uint8_t power_role, uint8_t data_role)
{
    uint8_t msg_hdr = TCPC_V10_REG_MSG_HDR_INFO_SET(data_role, power_role);

    return scv89601p_pd_i2c_write8(
        tcpc, TCPC_V10_REG_MSG_HDR_INFO, msg_hdr);
}

static int scv89601p_pd_set_rx_enable(struct tcpc_device *tcpc, uint8_t enable)
{
    return scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_RX_DETECT, enable);
}

static int scv89601p_pd_set_bist_test_mode(struct tcpc_device *tcpc, bool en);
static int scv89601p_pd_get_message(struct tcpc_device *tcpc, uint32_t *payload,
            uint16_t *msg_head, enum tcpm_transmit_type *frame_type)
{
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);
    int rv = 0;
    uint8_t cnt = 0, buf[4];

    rv = scv89601p_pd_block_read(sc->client, TCPC_V10_REG_RX_BYTE_CNT, 4, buf);
    if (rv < 0)
        return rv;

    cnt = buf[0];
    *frame_type = buf[1];
    *msg_head = le16_to_cpu(*(uint16_t *)&buf[2]);

#ifndef CONFIG_USB_PD_ONLY_PRINT_SYSTEM_BUSY
    if (PD_DATA_BIST == PD_HEADER_TYPE(*msg_head) && cnt > 3){
        scv89601p_pd_set_bist_test_mode(tcpc, true);
        SCV89601P_PD_INFO("%s \n", __func__);
    }

#endif /* CONFIG_USB_PD_ONLY_PRINT_SYSTEM_BUSY */

    /* TCPC 1.0 ==> no need to subtract the size of msg_head */
    if (cnt > 3) {
        cnt -= 3; /* MSG_HDR */
        rv = scv89601p_pd_block_read(sc->client, TCPC_V10_REG_RX_DATA, cnt,
                        payload);
    }

    return rv;
}

#pragma pack(push, 1)
struct tcpc_transmit_packet {
    uint8_t cnt;
    uint16_t msg_header;
    uint8_t data[sizeof(uint32_t)*7];
};
#pragma pack(pop)

static int scv89601p_pd_transmit(struct tcpc_device *tcpc,
    enum tcpm_transmit_type type, uint16_t header, const uint32_t *data)
{
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);
    int rv;
    int data_cnt;
    struct tcpc_transmit_packet packet;

    if (type < TCPC_TX_HARD_RESET) {
        data_cnt = sizeof(uint32_t) * PD_HEADER_CNT(header);

        packet.cnt = data_cnt + sizeof(uint16_t);
        packet.msg_header = header;

        packet.cnt += 4;

        if (data_cnt > 0)
            memcpy(packet.data, (uint8_t *) data, data_cnt);
        /* Length need add CRC(4)*/
        rv = scv89601p_pd_block_write(sc->client,
                TCPC_V10_REG_TX_BYTE_CNT,
                packet.cnt - 3, (uint8_t *) &packet);
        if (rv < 0)
            return rv;
        SCV89601P_PD_INFO("%s data_cnt:%d header:%04X data:%08X\n", __func__, packet.cnt, header, *packet.data);
    }
    return scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_TRANSMIT,
            TCPC_V10_REG_TRANSMIT_SET(
            tcpc->pd_retry_count, type));
}

static int scv89601p_pd_set_bist_test_mode(struct tcpc_device *tcpc, bool en)
{
    int data = 0;

    data = scv89601p_pd_i2c_read8(tcpc, TCPC_V10_REG_TCPC_CTRL);
    if (data < 0)
        return data;

    data &= ~TCPC_V10_REG_TCPC_CTRL_BIST_TEST_MODE;
    data |= en ? TCPC_V10_REG_TCPC_CTRL_BIST_TEST_MODE : 0;

    return scv89601p_pd_i2c_write8(tcpc, TCPC_V10_REG_TCPC_CTRL, data);
}
#endif /* CONFIG_USB_POWER_DELIVERY */

static struct tcpc_ops scv89601p_pd_tcpc_ops = {
    .init = scv89601p_pd_tcpc_init,
    .alert_status_clear = scv89601p_pd_alert_status_clear,
    .fault_status_clear = scv89601p_pd_fault_status_clear,
    .get_alert_mask = scv89601p_pd_get_alert_mask,
    .get_alert_status = scv89601p_pd_get_alert_status,
    .get_power_status = scv89601p_pd_get_power_status,
    .get_fault_status = scv89601p_pd_get_fault_status,
#ifdef CONFIG_SUPPORT_SOUTHCHIP_PDPHY
    .get_chip_id = scv89601p_pd_get_chip_id,
    .get_chip_vid = scv89601p_pd_get_chip_vid,
    .get_chip_pid = scv89601p_pd_get_chip_pid,
#endif /* CONFIG_SUPPORT_SOUTHCHIP_PDPHY */
    .get_cc = scv89601p_pd_get_cc,
    .set_cc = scv89601p_pd_set_cc,
    .set_polarity = scv89601p_pd_set_polarity,
    .set_low_rp_duty = scv89601p_pd_set_low_rp_duty,
    .set_vconn = scv89601p_pd_set_vconn,
    .deinit = scv89601p_pd_tcpc_deinit,

#ifdef CONFIG_TCPC_LOW_POWER_MODE
    .is_low_power_mode = scv89601p_pd_is_low_power_mode,
    .set_low_power_mode = scv89601p_pd_set_low_power_mode,
#endif	/* CONFIG_TCPC_LOW_POWER_MODE */

#ifdef CONFIG_TCPC_WATCHDOG_EN
    .set_watchdog = scv89601p_pd_set_watchdog,
#endif	/* CONFIG_TCPC_WATCHDOG_EN */

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
    .set_msg_header = scv89601p_pd_set_msg_header,
    .set_rx_enable = scv89601p_pd_set_rx_enable,
    .get_message = scv89601p_pd_get_message,
    .transmit = scv89601p_pd_transmit,
    .set_bist_test_mode = scv89601p_pd_set_bist_test_mode,
#endif	/* CONFIG_USB_POWER_DELIVERY */
};

static int scv89601p_pd_parse_dt(struct scv89601p_pd *sc, struct device *dev)
{
    struct device_node *np = dev->of_node;
    struct tcpc_desc *desc;
    u32 val, len;
    const char *name = "default";
    int ret = 0;

    desc = devm_kzalloc(dev, sizeof(*desc), GFP_KERNEL);
    if (!desc)
        return -ENOMEM;

    sc->irq_gpio = of_get_named_gpio(np, "scv89601p_pd,intr-gpio", 0);

    if (ret < 0)
        SCV89601P_PD_ERR("%s no intr_gpio info\n", __func__);

    SCV89601P_PD_INFO("tcpc irq = %d\n", sc->irq_gpio);

    if (of_property_read_u32(np, "sc-tcpc,role_def", &val) >= 0) {
        if (val >= TYPEC_ROLE_NR)
            desc->role_def = TYPEC_ROLE_DRP;
        else
            desc->role_def = val;
    } else {
        SCV89601P_PD_INFO("use default Role DRP\n");
        desc->role_def = TYPEC_ROLE_DRP;
    }

    if (of_property_read_u32(np, "sc-tcpc,rp_level", &val) >= 0) {
        switch (val) {
        case 0: /* RP Default */
            desc->rp_lvl = TYPEC_CC_RP_DFT;
            break;
        case 1: /* RP 1.5V */
            desc->rp_lvl = TYPEC_CC_RP_1_5;
            break;
        case 2: /* RP 3.0V */
            desc->rp_lvl = TYPEC_CC_RP_3_0;
            break;
        default:
            break;
        }
    }

#ifdef CONFIG_TCPC_VCONN_SUPPLY_MODE
    if (of_property_read_u32(np, "sc-tcpc,vconn_supply", &val) >= 0) {
        if (val >= TCPC_VCONN_SUPPLY_NR)
            desc->vconn_supply = TCPC_VCONN_SUPPLY_ALWAYS;
        else
            desc->vconn_supply = val;
    } else {
        SCV89601P_PD_INFO("use default VconnSupply\n");
        desc->vconn_supply = TCPC_VCONN_SUPPLY_ALWAYS;
    }
#endif	/* CONFIG_TCPC_VCONN_SUPPLY_MODE */

    if (of_property_read_string(np, "sc-tcpc,name",
                (char const **)&name) < 0) {
        SCV89601P_PD_INFO("use default name\n");
    }

    len = strlen(name);
    desc->name = kzalloc(len+1, GFP_KERNEL);
    if (!desc->name)
        return -ENOMEM;

    strlcpy((char *)desc->name, name, len+1);

    sc->tcpc_desc = desc;

    return ret < 0 ? ret : 0;
}

static int scv89601p_pd_tcpcdev_init(struct scv89601p_pd *sc, struct device *dev)
{
    //struct device_node *np = dev->of_node;

    SCV89601P_PD_INFO("%s\n", __func__);

    sc->tcpc = tcpc_device_register(dev,
            sc->tcpc_desc, &scv89601p_pd_tcpc_ops, sc);
    if (IS_ERR_OR_NULL(sc->tcpc))
        return -EINVAL;

#if CONFIG_USB_PD_DISABLE_PE
    sc->tcpc->disable_pe =
            of_property_read_bool(np, "sc-tcpc,disable_pe");
#endif	/* CONFIG_USB_PD_DISABLE_PE */

    sc->tcpc->tcpc_flags = TCPC_FLAGS_LPM_WAKEUP_WATCHDOG |
            TCPC_FLAGS_VCONN_SAFE5V_ONLY;

#ifdef CONFIG_USB_PD_REV30
    sc->tcpc->tcpc_flags |= TCPC_FLAGS_PD_REV30;
    SCV89601P_PD_INFO("PD_REV30\n");
#endif	/* CONFIG_USB_PD_REV30 */
    sc->tcpc->tcpc_flags |= TCPC_FLAGS_ALERT_V10;

    return 0;
}

static inline int scv89601p_pd_check_revision(struct i2c_client *client)
{
    u16 vid, pid, did;
    int ret;
    u8 data = 1;
    struct scv89601p_pd *sc = i2c_get_clientdata(client);

    ret = scv89601p_pd_read_device(client, TCPC_V10_REG_VID, 2, &vid);
    if (ret < 0) {
        SCV89601P_PD_ERR("read chip ID fail(%d)\n", ret);
        return -EIO;
    }

    if (vid != SOUTHCHIP_PD_VID) {
        SCV89601P_PD_INFO("%s failed, VID=0x%04x\n", __func__, vid);
        return -ENODEV;
    }
    sc->chip_vid = vid;

    ret = scv89601p_pd_read_device(client, TCPC_V10_REG_PID, 2, &pid);
    if (ret < 0) {
        SCV89601P_PD_ERR("read product ID fail(%d)\n", ret);
        return -EIO;
    }

    if (pid != SCV89601P_PD_PID) {
        SCV89601P_PD_INFO("%s failed, PID=0x%04x\n", __func__, pid);
        return -ENODEV;
    }
    sc->chip_pid = pid;

    // close watchdog
    ret = scv89601p_pd_write_device(client, TCPC_V10_REG_TCPC_CTRL, 1, &data);
    ret |= scv89601p_pd_write_device(client, SCV89601P_PD_REG_RST_CTRL, 1, &data);
    if (ret < 0)
        return ret;

    usleep_range(1000, 2000);

    ret = scv89601p_pd_read_device(client, TCPC_V10_REG_DID, 2, &did);
    if (ret < 0) {
        SCV89601P_PD_ERR("read device ID fail(%d)\n", ret);
        return -EIO;
    }

    return did;
}

/*HS07 code for SR-AL7761A-01-20 by lina at 20250415 start*/
void scv89601p_pd_set_vbus_present(struct tcpc_device *tcpc, bool vbus_stat)
{
    struct scv89601p_pd *sc = tcpc_get_dev_data(tcpc);
    if (sc->vbus_present_status == vbus_stat) {
        return ;
    }

    sc->vbus_present_status = vbus_stat;
    sc->vbus_present_alert = true;
    SCV89601P_PD_INFO("scv89601p_pd_set_vbus_present\n");
    scv89601p_pd_intr_handler(sc->irq, (void *)sc);
}
EXPORT_SYMBOL(scv89601p_pd_set_vbus_present);
/*HS07 code for SR-AL7761A-01-20 by lina at 20250415 end*/

static int scv89601p_pd_i2c_probe(struct i2c_client *client,
                const struct i2c_device_id *id)
{
    struct scv89601p_pd *sc;
    int ret = 0, chip_id;
    bool use_dt = client->dev.of_node;

    SCV89601P_PD_INFO("%s (%s)\n", __func__, SCV89601P_PD_DRV_VER);
    if (i2c_check_functionality(client->adapter,
            I2C_FUNC_SMBUS_I2C_BLOCK | I2C_FUNC_SMBUS_BYTE_DATA))
        SCV89601P_PD_INFO("I2C functionality : OK...\n");
    else
        SCV89601P_PD_INFO("I2C functionality check : failuare...\n");

    sc = devm_kzalloc(&client->dev, sizeof(*sc), GFP_KERNEL);
    if (!sc)
        return -ENOMEM;

    sc->dev = &client->dev;
    sc->client = client;
    /*HS07 code for SR-AL7761A-01-20 by lina at 20250415 start*/
    sc->adp = client->adapter;
    /*HS07 code for SR-AL7761A-01-20 by lina at 20250415 end*/
    i2c_set_clientdata(client, sc);
    chip_id = scv89601p_pd_check_revision(client);
    if (chip_id < 0)
        return chip_id;

    sc->chip_id = chip_id;
    /* HST11 code for AX7800A-2024 by zhangziyi at 20250611 start */
    #if defined(CONFIG_CUSTOM_PROJECT_HST11)
    sc->is_shutdown_flag = false;
    mutex_init(&sc->pd_intr_lock);
    #endif //CONFIG_CUSTOM_PROJECT_HST11
    /* HST11 code for AX7800A-2024 by zhangziyi at 20250611 end */
    /*HS07 code for SR-AL7761A-01-20 by lina at 20250415 start*/
    INIT_DELAYED_WORK(&sc->vbus_detect_dwork, scv89601p_vbus_detect_work);
    /*HS07 code for SR-AL7761A-01-20 by lina at 20250415 end*/
    gxy_bat_set_tcpcinfo(GXY_BAT_TCPC_INFO_SCV89601P);
    SCV89601P_PD_INFO("chip info [0x%0x,0x%0x,0x%0x]\n", sc->chip_vid,
                            sc->chip_pid, sc->chip_id);
    if (use_dt) {
        ret = scv89601p_pd_parse_dt(sc, &client->dev);
        if (ret < 0)
            return ret;
    } else {
        SCV89601P_PD_ERR("no dts node\n");
        return -ENODEV;
    }

    ret = scv89601p_pd_regmap_init(sc);
    if (ret < 0) {
        SCV89601P_PD_ERR("scv89601p_pd regmap init fail\n");
        goto err_regmap_init;
    }

    ret = scv89601p_pd_tcpcdev_init(sc, &client->dev);
    if (ret < 0) {
        SCV89601P_PD_ERR("scv89601p_pd tcpc dev init fail\n");
        goto err_tcpc_reg;
    }

    /* HST11 code for AX7800A-2331 by zhangziyi at 20250618 start */
    #if defined(CONFIG_CUSTOM_PROJECT_HST11)
    scv89601p_set_bmc(sc);
    #endif //CONFIG_CUSTOM_PROJECT_HST11
    /* HST11 code for AX7800A-2331 by zhangziyi at 20250618 end */

    ret = scv89601p_pd_init_alert(sc->tcpc);
    if (ret < 0) {
        SCV89601P_PD_ERR("scv89601p_pd init alert fail\n");
        goto err_irq_init;
    }
    SCV89601P_PD_INFO("%s probe OK!\n", __func__);
    return 0;

err_irq_init:
    tcpc_device_unregister(sc->dev, sc->tcpc);
err_tcpc_reg:
    scv89601p_pd_regmap_deinit(sc);
err_regmap_init:
    return ret;
}

static int scv89601p_pd_i2c_remove(struct i2c_client *client)
{
    struct scv89601p_pd *sc = i2c_get_clientdata(client);

    if (sc) {
        tcpc_device_unregister(sc->dev, sc->tcpc);
        scv89601p_pd_regmap_deinit(sc);
    }

    return 0;
}

#ifdef CONFIG_PM
static int scv89601p_pd_i2c_suspend(struct device *dev)
{
    struct scv89601p_pd *sc = dev_get_drvdata(dev);

    SCV89601P_PD_INFO("%s\n", __func__);
    if (device_may_wakeup(dev))
        enable_irq_wake(sc->irq);
    disable_irq(sc->irq);

    return 0;
}

static int scv89601p_pd_i2c_resume(struct device *dev)
{
    struct scv89601p_pd *sc = dev_get_drvdata(dev);

    SCV89601P_PD_INFO("%s\n", __func__);
    enable_irq(sc->irq);
    if (device_may_wakeup(dev))
        disable_irq_wake(sc->irq);

    return 0;
}

static const struct dev_pm_ops scv89601p_pd_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(
            scv89601p_pd_i2c_suspend,
            scv89601p_pd_i2c_resume)
};
#endif /* CONFIG_PM */

static void scv89601p_pd_shutdown(struct i2c_client *client)
{
    struct scv89601p_pd *sc = i2c_get_clientdata(client);

    /* HST11 code for AX7800A-2024 by zhangziyi at 20250611 start */
    #if defined(CONFIG_CUSTOM_PROJECT_HST11)
    scv89601p_set_adc_en(sc, false);
    sc->is_shutdown_flag = true;
    #endif //CONFIG_CUSTOM_PROJECT_HST11
    /* HST11 code for AX7800A-2024 by zhangziyi at 20250611 end */

    /* Please reset IC here */
    if (sc != NULL) {
        if (sc->irq)
            disable_irq(sc->irq);
        tcpm_shutdown(sc->tcpc);
    } else {
        i2c_smbus_write_byte_data(
            client, SCV89601P_PD_REG_RST_CTRL, 0x01);
    }
}

static const struct i2c_device_id scv89601p_pd_id_table[] = {
    {"scv89601p_pd", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, scv89601p_pd_id_table);

static const struct of_device_id sc_match_table[] = {
    {.compatible = "southchip,scv89601p_pd",},
    {},
};

static struct i2c_driver scv89601p_pd_driver = {
    .driver = {
        .name = "scv89601p_pd-driver",
        .owner = THIS_MODULE,
        .of_match_table = sc_match_table,
        .pm = &scv89601p_pd_pm_ops,
    },
    .probe = scv89601p_pd_i2c_probe,
    .remove = scv89601p_pd_i2c_remove,
    .shutdown = scv89601p_pd_shutdown,
    .id_table = scv89601p_pd_id_table,
};

module_i2c_driver(scv89601p_pd_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Boyu Wen <boyu-wen@southchip.com>");
MODULE_DESCRIPTION("SCV89601P_PD TCPC Driver");
MODULE_VERSION(SCV89601P_PD_DRV_VER);

/**
 * Release Version
 * 1.0
 * (1) add scv89601p pd dirver, compile pass
 */
/* HS07 code for SR-AL7761A-01-164 by lina at 20250314 end */
