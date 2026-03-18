#ifdef    CONFIG_CTS_I2C_HOST
#define LOG_TAG         "I2CDrv"
#else
#define LOG_TAG         "SPIDrv"
#endif

#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"
#include "cts_sysfs.h"
#include "cts_charger_detect.h"
#include "cts_earjack_detect.h"
#include "cts_oem.h"
#include "mtk_disp_notify.h"
#include "cts_tcs.h"
/*HS07 code for SR-AL7761A-01-224 by guyuming at 20250317 start*/
#include "../touch_common/touch_feature.h"
/*HS07 code for SR-AL7761A-01-224 by guyuming at 20250317 end*/

static void cts_resume_work_func(struct work_struct *work);
#ifdef CFG_CTS_DRM_NOTIFIER
#include <drm/drm_panel.h>
static struct drm_panel *active_panel;
static int check_dt(struct device_node *np);
#endif
bool cts_show_debug_log;
#ifdef CTS_MTK_GET_PANEL
static char *active_panel_name;
#endif

module_param_named(debug_log, cts_show_debug_log, bool, 0660);
MODULE_PARM_DESC(debug_log, "Show debug log control");

/*HS07 code for SR-AL7761A-01-224 by guyuming at 20250317 start*/
struct chipone_ts_data *g_cts_data = NULL;
/*HS07 code for SR-AL7761A-01-224 by guyuming at 20250317 end*/

int cts_suspend(struct chipone_ts_data *cts_data)
{
    int ret;

    cts_info("Suspend");

    cts_lock_device(&cts_data->cts_dev);
    ret = cts_suspend_device(&cts_data->cts_dev);
    cts_unlock_device(&cts_data->cts_dev);

    if (ret)
        cts_err("Suspend device failed %d", ret);

    ret = cts_stop_device(&cts_data->cts_dev);
    if (ret) {
        cts_err("Stop device failed %d", ret);
        return ret;
    }
#ifdef CFG_CTS_GESTURE
    /* Enable IRQ wake if gesture wakeup enabled */
    if (cts_is_gesture_wakeup_enabled(&cts_data->cts_dev)) {
        ret = cts_plat_enable_irq_wake(cts_data->pdata);
        if (ret) {
            cts_err("Enable IRQ wake failed %d", ret);
            return ret;
        }
        ret = cts_plat_enable_irq(cts_data->pdata);
        if (ret) {
            cts_err("Enable IRQ failed %d", ret);
            return ret;
        }
    }
#endif /* CFG_CTS_GESTURE */

/** - To avoid waking up while not sleeping,
 *delay 20ms to ensure reliability
 */
    msleep(20);

    return 0;
}

int cts_resume(struct chipone_ts_data *cts_data)
{
    int ret;

    cts_info("Resume");

    /*HS07 code for HS07-227 by guyuming at 20250403 start*/
    cts_data->cts_dev.enabled = false;
    /*HS07 code for HS07-227 by guyuming at 20250403 end*/

#ifdef CFG_CTS_GESTURE
    if (cts_is_gesture_wakeup_enabled(&cts_data->cts_dev)) {
        ret = cts_plat_disable_irq_wake(cts_data->pdata);
        if (ret)
            cts_warn("Disable IRQ wake failed %d", ret);
        ret = cts_plat_disable_irq(cts_data->pdata);
        if (ret < 0)
            cts_err("Disable IRQ failed %d", ret);
    }
#endif /* CFG_CTS_GESTURE */

    cts_lock_device(&cts_data->cts_dev);
    ret = cts_resume_device(&cts_data->cts_dev);
    cts_unlock_device(&cts_data->cts_dev);
    if (ret) {
        cts_warn("Resume device failed %d", ret);
        return ret;
    }

    ret = cts_start_device(&cts_data->cts_dev);
    if (ret) {
        cts_err("Start device failed %d", ret);
        return ret;
    }

    return 0;
}

static void cts_resume_work_func(struct work_struct *work)
{
    struct chipone_ts_data *cts_data =
    container_of(work, struct chipone_ts_data, ts_resume_work);
    cts_info("%s", __func__);
    cts_resume(cts_data);
}

#ifdef CONFIG_CTS_PM_FB_NOTIFIER
#ifdef CFG_CTS_DRM_NOTIFIER
static int fb_notifier_callback(struct notifier_block *nb,
        unsigned long action, void *data)
{
    volatile int blank;
    const struct cts_platform_data *pdata =
    container_of(nb, struct cts_platform_data, fb_notifier);
    struct chipone_ts_data *cts_data =
    container_of(pdata->cts_dev, struct chipone_ts_data, cts_dev);
    int *evdata = (int *)data;

    cts_info("FB notifier callback");
    if (!evdata || !cts_data)
        return 0;

    blank = *(int *)evdata;
    cts_info("action=%lu, blank=%d\n", action, blank);
    /*HS07 code for HS07-2495 by guyuming at 20250429 start*/
    if ((action == MTK_DISP_EARLY_EVENT_BLANK || action == MTK_DISP_ESD_RECOVERY_SUSPEND) && blank == MTK_DISP_BLANK_POWERDOWN) {
        cts_suspend(cts_data);
    } else if (evdata) {
        blank = *(int *)evdata;
        if ((action == MTK_DISP_EARLY_EVENT_BLANK || action == MTK_DISP_ESD_RECOVERY_RESUME) && blank == MTK_DISP_BLANK_UNBLANK) {
            /* cts_resume(cts_data); */
            queue_work(cts_data->workqueue, &cts_data->ts_resume_work);
        }
    }
    /*HS07 code for HS07-2495 by guyuming at 20250429 end*/
    return 0;
}
#else
static int fb_notifier_callback(struct notifier_block *nb,
        unsigned long action, void *data)
{
    volatile int blank;
    const struct cts_platform_data *pdata =
    container_of(nb, struct cts_platform_data, fb_notifier);
    struct chipone_ts_data *cts_data =
    container_of(pdata->cts_dev, struct chipone_ts_data, cts_dev);
    struct fb_event *evdata = data;

    cts_info("FB notifier callback");

    if (evdata && evdata->data) {
        if (action == FB_EVENT_BLANK) {
            blank = *(int *)evdata->data;
            if (blank == FB_BLANK_UNBLANK) {
                /* cts_resume(cts_data); */
                queue_work(cts_data->workqueue,
                    &cts_data->ts_resume_work);
                return NOTIFY_OK;
            }
        } else if (action == FB_EARLY_EVENT_BLANK) {
            blank = *(int *)evdata->data;
            if (blank == FB_BLANK_POWERDOWN) {
                cts_suspend(cts_data);
                return NOTIFY_OK;
            }
        }
    }

    return NOTIFY_DONE;
}
#endif

static int cts_init_pm_fb_notifier(struct chipone_ts_data *cts_data)
{
    cts_info("Init FB notifier");

    cts_data->pdata->fb_notifier.notifier_call = fb_notifier_callback;

#ifdef CFG_CTS_DRM_NOTIFIER
    {
        int ret = -ENODEV;

        ret = mtk_disp_notifier_register("chipone Touch",
                &cts_data->pdata->fb_notifier);
            if (ret)
                cts_err("register drm_notifier failed. ret=%d\n", ret);
        return ret;
    }
#else
    return fb_register_client(&cts_data->pdata->fb_notifier);
#endif
}

static int cts_deinit_pm_fb_notifier(struct chipone_ts_data *cts_data)
{
    cts_info("Deinit FB notifier");
#ifdef CFG_CTS_DRM_NOTIFIER
    {
        int ret = 0;

        if (active_panel) {
            ret = mtk_disp_notifier_unregister(&cts_data->pdata->fb_notifier);
                 //drm_panel_notifier_unregister(active_panel,
                 //   &cts_data->pdata->fb_notifier);
            if (ret)
                cts_err("Error occurred while unregistering drm_notifier.\n");
        }
        return ret;
    }
#else
    return fb_unregister_client(&cts_data->pdata->fb_notifier);
#endif
}
#endif /* CONFIG_CTS_PM_FB_NOTIFIER */

/*HS07 code for HS07-2659 by guyuming at 20250508 start*/
/*HS07 code for HS07-2980 by guyuming at 20250515 start*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
static int chipone_get_tp_module(void)
{
    int fw_num = 0;

    if (tp_choose_panel("nl9916x_txd_boe_mipi")) {
        fw_num = MODEL_TXD_BOE;
    } else if (tp_choose_panel("nl9916x_ld_hkc")) {
        fw_num = MODEL_LD_HKC;
    } else if (tp_choose_panel("nl9916x_xx_tm")) {
        fw_num = MODEL_XX_TM;
    } else if (tp_choose_panel("nl9916x_txd_csot")) {
        fw_num = MODEL_TXD_CSOT;
    } else if (tp_choose_panel("nl9916x_txd_boe_hh")) {
        fw_num = MODEL_TXD_BOE_HH;
    } else {
        fw_num = MODEL_DEFAULT;
    }

    return fw_num;
}

#else//LINUX_VERSION_CODE
static int chipone_get_tp_module(void)
{
    int fw_num = 0;
    const char *panel_name = NULL;

    panel_name = tp_choose_panel();

    if (NULL != strstr(panel_name, "nl9916x_txd_boe_mipi")) {
        fw_num = MODEL_TXD_BOE;
    } else if (NULL != strstr(panel_name, "nl9916x_ld_hkc")) {
        fw_num = MODEL_LD_HKC;
    } else if (NULL != strstr(panel_name, "nl9916x_xx_tm")) {
        fw_num = MODEL_XX_TM;
    } else if (NULL != strstr(panel_name, "nl9916x_txd_csot")) {
        fw_num = MODEL_TXD_CSOT;
    } else if (NULL != strstr(panel_name, "nl9916x_txd_boe_hh")) {
        fw_num = MODEL_TXD_BOE_HH;
    } else {
        fw_num = MODEL_DEFAULT;
    }

    return fw_num;
}
#endif//LINUX_VERSION_CODE
/*HS07 code for HS07-2980 by guyuming at 20250515 end*/

static void chipone_update_module_info(void)
{
    int module = 0;
    struct chipone_ts_data *cts_data = g_cts_data;

    module = chipone_get_tp_module();

    switch (module) {
        case MODEL_TXD_BOE:
            strcpy(cts_data->module_name,"nl9916x_txd_boe");
            strcpy(cts_data->cts_firmware_filename, "nl9916x_txd_boe_firmware.bin");
            break;
        case MODEL_LD_HKC:
            strcpy(cts_data->module_name,"nl9916x_ld_hkc");
            strcpy(cts_data->cts_firmware_filename, "nl9916x_ld_hkc_firmware.bin");
            break;
        case MODEL_XX_TM:
            strcpy(cts_data->module_name,"nl9916x_xx_tm");
            strcpy(cts_data->cts_firmware_filename, "nl9916x_xx_tm_firmware.bin");
            break;
        case MODEL_TXD_CSOT:
            strcpy(cts_data->module_name,"nl9916x_txd_csot");
            strcpy(cts_data->cts_firmware_filename, "nl9916x_txd_csot_firmware.bin");
            break;
        case MODEL_TXD_BOE_HH:
            strcpy(cts_data->module_name,"nl9916x_txd_boe_hh");
            strcpy(cts_data->cts_firmware_filename, "nl9916x_txd_boe_hh_firmware.bin");
            break;
        default:
            strcpy(cts_data->module_name,"unknown");
            strcpy(cts_data->cts_firmware_filename, "unknown");
            break;

    }
}
/*HS07 code for HS07-2659 by guyuming at 20250508 end*/

#ifdef CFG_CTS_DRM_NOTIFIER
static int check_dt(struct device_node *np)
{
    int i;
    int count;
    struct device_node *node;
    struct drm_panel *panel;

    count = of_count_phandle_with_args(np, "panel", NULL);
    if (count <= 0)
        return 0;

    for (i = 0; i < count; i++) {
        node = of_parse_phandle(np, "panel", i);
        panel = of_drm_find_panel(node);
        of_node_put(node);
        if (!IS_ERR(panel)) {
            cts_info("check active_panel");
            active_panel = panel;
            return 0;
        }
    }
    if (node)
        cts_err("%s: %s not actived", __func__, node->name);
    return -ENODEV;
}

static int check_default_tp(struct device_node *dt, const char *prop)
{
    const char *active_tp;
    const char *compatible;
    char *start;
    int ret;

    ret = of_property_read_string(dt->parent, prop, &active_tp);
    if (ret) {
        cts_err("%s:fail to read %s %d", __func__, prop, ret);
        return -ENODEV;
    }

    ret = of_property_read_string(dt, "compatible", &compatible);
    if (ret < 0) {
        cts_err("%s:fail to read %s %d", __func__, "compatible", ret);
        return -ENODEV;
    }

    start = strnstr(active_tp, compatible, strlen(active_tp));
    if (start == NULL) {
        cts_err("no match compatible, %s, %s", compatible, active_tp);
        ret = -ENODEV;
    }

    return ret;
}
#endif

#ifdef CTS_MTK_GET_PANEL
char panel_name[50] = { 0 };

static int cts_get_panel(void)
{
    int ret = -1;

    cts_info("Enter cts_get_panel");
    if (saved_command_line) {
        char *sub;
        char key_prefix[] = "mipi_mot_vid_";
        char ic_prefix[] = "icnl";

        cts_info("saved_command_line is %s", saved_command_line);
        sub = strstr(saved_command_line, key_prefix);
        if (sub) {
            char *d;
            int n, len, len_max = 50;

            d = strstr(sub, " ");
            if (d)
                n = strlen(sub) - strlen(d);
            else
                n = strlen(sub);

            if (n > len_max)
                len = len_max;
            else
                len = n;

            strncpy(panel_name, sub, len);
            active_panel_name = panel_name;
            if (strstr(active_panel_name, ic_prefix))
                cts_info("active_panel_name=%s", active_panel_name);
            else {
                cts_info("Not chipone panel!");
                return ret;
            }

        } else {
            cts_info("chipone active panel not found!");
            return ret;
        }
    } else {
        cts_info("saved_command_line null!");
        return ret;
    }

    return 0;
}
#endif

/*HS07 code for SR-AL7761A-01-265 by guyuming at 20250321 start*/
static void cts_charger_notify_work(struct work_struct *work)
{
    struct chipone_ts_data *cts_data = g_cts_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;

    int ret = 0;

    if (NULL == work) {
        cts_err("%s:  parameter work are null!", __func__);
        return;
    }

    if (cts_data->cts_dev.rtdata.suspended == false) {
        cts_info("enter charger_notify_work\n");
        cts_lock_device(cts_dev);
        ret = cts_tcs_set_charger_plug(cts_dev, cts_data->cts_charger_state);
        cts_unlock_device(cts_dev);

        if (ret) {
            cts_err("Set charger state failed %d", ret);
        }
    } else {
        cts_err("%s:is suspended,do nothing", __func__);
    }
}

static int cts_charger_notifier_callback(struct notifier_block *nb, unsigned long event, void *v)
{
    struct chipone_ts_data *cts_data = g_cts_data;

    if (event == TP_USB_PLUGIN_STATE) {
        cts_data->cts_charger_state = 1;
    } else if (event == TP_USB_PLUGOUT_STATE) {
        cts_data->cts_charger_state = 0;
    } else {
        return -EINVAL;
    }

    schedule_work(&cts_data->cts_charger_notify_wq);

    return 0;
}

void cts_charger_deinit(void)
{
    struct chipone_ts_data *cts_data = g_cts_data;
    g_tp_detect_usb_flag = 0;
    cancel_work_sync(&cts_data->cts_charger_notify_wq);
    tp_usb_notifier_unregister(&cts_data->charger_notify);
    cts_err("Unable to register cts_data->charger_notify");
}

void cts_earphone_workqueue(struct work_struct *work)
{
    struct chipone_ts_data *cts_data = g_cts_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    int ret = 0;
    if (cts_data->cts_dev.rtdata.suspended == false) {
        cts_info("enter headset_workqueue\n");
        cts_lock_device(cts_dev);
        ret = cts_tcs_set_earjack_plug(&cts_data->cts_dev, cts_data->earphone_status);
        cts_unlock_device(cts_dev);
        if (ret) {
            cts_err("%s:CTS headset mode set fail\n", __func__);
        }
    } else {
        cts_info("%s:is suspended,do nothing", __func__);
    }

    return;
}

int cts_earphone_notifier_callback(struct notifier_block *nb, unsigned long event, void *ptr)
{
    struct chipone_ts_data *cts_data = g_cts_data;
    static int earphone_pre_status = 0;
    cts_info("in %s\n", __func__);
    /*HS07 code for HS07-3485 by guyuming at 20250527 start*/
    if (event == EARPHONE_PLUGIN_STATE) {
        cts_data->earphone_status = 1;
        cts_info("earphone in");
    } else if (event == EARPHONE_PLUGOUT_STATE) {
        if((g_tp_typec_earphone_in || g_tp_round_earphone_in) == 0){
            cts_data->earphone_status = 0;
            cts_info("earphone out");
        }
    } else {
        cts_info("Earphone Error\n");
        return -EINVAL;
    }
    cts_info("Current state: typec_plugin=%d, round_plugin=%d, earphone_status=%d\n",
    g_tp_typec_earphone_in, g_tp_round_earphone_in, cts_data->earphone_status);
    /*HS07 code for HS07-3485 by guyuming at 20250527 end*/
    if (earphone_pre_status != cts_data->earphone_status) {
        schedule_work(&cts_data->earphone_work_queue);
        earphone_pre_status = cts_data->earphone_status;
    }

    return 0;
}

void cts_earphone_deinit(void)
{
    struct chipone_ts_data *cts_data = g_cts_data;
    cancel_work_sync(&cts_data->earphone_work_queue);
    earphone_notifier_unregister(&cts_data->earphone_notify);
    cts_err("Unable to register earphone_notifier");
}
/*HS07 code for SR-AL7761A-01-265 by guyuming at 20250321 end*/

/*HS07 code for SR-AL7761A-01-263 by guyuming at 20250319 start*/
void tp_enable_gesture_wakeup(void)
{
    struct chipone_ts_data *cts_data = g_cts_data;
    cts_info("Enable gesture wakeup");
    cts_data->cts_dev.rtdata.gesture_wakeup_enabled = true;
    gs_smart_wakeup_flag_set(true);
}

void tp_disable_gesture_wakeup(void)
{
    struct chipone_ts_data *cts_data = g_cts_data;
    cts_info("Disable gesture wakeup");
    cts_data->cts_dev.rtdata.gesture_wakeup_enabled = false;
    gs_smart_wakeup_flag_set(false);
}
/*HS07 code for SR-AL7761A-01-263 by guyuming at 20250319 end*/

/*HS07 code for SR-AL7761A-01-224 by guyuming at 20250317 start*/
extern int cts_ito_test(void);
static void tp_feature_interface(void)
{
    struct chipone_ts_data *cts_data = g_cts_data;
    struct lcm_info *gs_cts_lcm_info = NULL;

    gs_cts_lcm_info = kzalloc(sizeof(*gs_cts_lcm_info), GFP_KERNEL);
    if (gs_cts_lcm_info == NULL) {
        kfree(gs_cts_lcm_info);
        cts_err("gs_cts_lcm_info kzalloc fail, and is NULL!");
        return;
    }

    gs_cts_lcm_info->module_name = cts_data->module_name;
    gs_cts_lcm_info->fw_version = &(cts_data->cts_dev.fwdata.version);
    /*HS07 code for SR-AL7761A-01-263 by guyuming at 20250319 start*/
    gs_cts_lcm_info->sec = cts_data->sec;
    gs_cts_lcm_info->gesture_wakeup_enable = tp_enable_gesture_wakeup;
    gs_cts_lcm_info->gesture_wakeup_disable = tp_disable_gesture_wakeup;
    /*HS07 code for SR-AL7761A-01-263 by guyuming at 20250319 end*/
    gs_cts_lcm_info->ito_test = cts_ito_test;
    cts_info("module_name=%s", gs_cts_lcm_info->module_name);

    tp_common_init(gs_cts_lcm_info);

}
/*HS07 code for SR-AL7761A-01-224 by guyuming at 20250317 end*/

#ifdef CONFIG_CTS_I2C_HOST
static int cts_driver_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
#else
static int cts_driver_probe(struct spi_device *client)
#endif
{
    struct chipone_ts_data *cts_data = NULL;
    int ret = 0;

#ifdef CTS_MTK_GET_PANEL
    ret = cts_get_panel();
    if (ret) {
        cts_info("MTK get chipone panel error");
        return ret;
    }
#endif

#ifdef CFG_CTS_DRM_NOTIFIER
    {
        struct device_node *dp = client->dev.of_node;

        if (check_dt(dp)) {
            if (!check_default_tp(dp, "qcom,i2c-touch-active"))
                ret = -EPROBE_DEFER;
            else
                ret = -ENODEV;

            cts_err("%s: %s not actived\n", __func__, dp->name);
            return ret;
        }
    }
#endif

#ifdef CONFIG_CTS_I2C_HOST
    cts_info("Probe i2c client: name='%s' addr=0x%02x flags=0x%02x irq=%d",
        client->name, client->addr, client->flags, client->irq);

#if !defined(CONFIG_MTK_PLATFORM)
    if (client->addr != CTS_DEV_NORMAL_MODE_I2CADDR) {
        cts_err("Probe i2c addr 0x%02x != driver config addr 0x%02x",
            client->addr, CTS_DEV_NORMAL_MODE_I2CADDR);
        return -ENODEV;
    };
#endif

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        cts_err("Check functionality failed");
        return -ENODEV;
    }
#endif

    cts_data = kzalloc(sizeof(struct chipone_ts_data), GFP_KERNEL);
    if (cts_data == NULL) {
        cts_err("Allocate chipone_ts_data failed");
        return -ENOMEM;
    }
    cts_data->pdata = kzalloc(sizeof(struct cts_platform_data), GFP_KERNEL);
    if (cts_data->pdata == NULL) {
        cts_err("Allocate cts_platform_data failed");
        ret = -ENOMEM;
        goto err_free_cts_data;
    }
#ifdef CONFIG_CTS_I2C_HOST
    i2c_set_clientdata(client, cts_data);
    cts_data->i2c_client = client;
    cts_data->device = &client->dev;
#else
    spi_set_drvdata(client, cts_data);
    cts_data->spi_client = client;
    cts_data->device = &client->dev;
#endif

    cts_init_platform_data(cts_data->pdata, client);

    cts_data->cts_dev.pdata = cts_data->pdata;
    cts_data->pdata->cts_dev = &cts_data->cts_dev;

    g_cts_data = cts_data;

    cts_data->workqueue =
    create_singlethread_workqueue(CFG_CTS_DEVICE_NAME "-workqueue");
    if (cts_data->workqueue == NULL) {
        cts_err("Create workqueue failed");
        ret = -ENOMEM;
        goto err_deinit_platform_data;
    }
#ifdef CONFIG_CTS_ESD_PROTECTION
    cts_data->esd_workqueue =
    create_singlethread_workqueue(CFG_CTS_DEVICE_NAME "-esd_workqueue");
    if (cts_data->esd_workqueue == NULL) {
        cts_err("Create esd workqueue failed");
        ret = -ENOMEM;
        goto err_destroy_workqueue;
    }
#endif

    /*HS07 code for SR-AL7761A-01-224 by guyuming at 20250317 start*/
    chipone_update_module_info();
    /*HS07 code for SR-AL7761A-01-224 by guyuming at 20250317 end*/

#ifdef CFG_CTS_HEARTBEAT_MECHANISM
    cts_data->heart_workqueue =
        create_singlethread_workqueue(CFG_CTS_DEVICE_NAME "-heart_workqueue");
    if (cts_data->heart_workqueue == NULL) {
        cts_err("Create heart workqueue failed");
        ret = -ENOMEM;
        goto err_destroy_esd_workqueue;
    }
#endif

    ret = cts_plat_request_resource(cts_data->pdata);
    if (ret < 0) {
        cts_err("Request resource failed %d", ret);
        goto err_destroy_heart_workqueue;
    }

    ret = cts_reset_device(&cts_data->cts_dev);
    if (ret < 0) {
        cts_err("Reset device failed %d", ret);
        goto err_free_resource;
    }

    ret = cts_probe_device(&cts_data->cts_dev);
    if (ret) {
        cts_err("Probe device failed %d", ret);
        goto err_free_resource;
    }

    ret = cts_plat_init_touch_device(cts_data->pdata);
    if (ret < 0) {
        cts_err("Init touch device failed %d", ret);
        goto err_free_resource;
    }

    ret = cts_plat_init_vkey_device(cts_data->pdata);
    if (ret < 0) {
        cts_err("Init vkey device failed %d", ret);
        goto err_deinit_touch_device;
    }

    ret = cts_plat_init_gesture(cts_data->pdata);
    if (ret < 0) {
        cts_err("Init gesture failed %d", ret);
        goto err_deinit_vkey_device;
    }

    cts_init_esd_protection(cts_data);

    ret = cts_tool_init(cts_data);
    if (ret < 0)
        cts_warn("Init tool node failed %d", ret);

    /*HS07 code for SR-AL7761A-01-265 by guyuming at 20250321 start*/
    INIT_WORK(&cts_data->earphone_work_queue, cts_earphone_workqueue);
    cts_data->earphone_notify.notifier_call = cts_earphone_notifier_callback;
    ret = earphone_notifier_register(&cts_data->earphone_notify);
    if (ret) {
        /*HS07 code for HS07-3485 by guyuming at 20250527 start*/
        g_tp_detect_typec_flag = 0;
        cts_err("Unable to register earphone_notifier: %d", ret);
        goto err_register_earphone_notify_failed;
    } else {
        g_tp_detect_typec_flag = 1;
        cts_err("Able to register earphone_notifier: %d",ret);
        /*HS07 code for HS07-3485 by guyuming at 20250527 end*/
    }

    INIT_WORK(&cts_data->cts_charger_notify_wq, cts_charger_notify_work);
    cts_data->charger_notify.notifier_call = cts_charger_notifier_callback;
    ret = tp_usb_notifier_register(&cts_data->charger_notify);
    if (ret) {
        g_tp_detect_usb_flag = 0;
        goto err_register_usb_notify_failed;
        cts_err("Unable to register charger_notifier: %d",ret);
    } else {
        g_tp_detect_usb_flag = 1;
    }
    /*HS07 code for SR-AL7761A-01-265 by guyuming at 20250321 end*/

    ret = cts_sysfs_add_device(&client->dev);
    if (ret < 0)
        cts_warn("Add sysfs entry for device failed %d", ret);

#ifdef CONFIG_CTS_PM_FB_NOTIFIER
    ret = cts_init_pm_fb_notifier(cts_data);
    if (ret) {
        cts_err("Init FB notifier failed %d", ret);
        goto err_deinit_sysfs;
    }
#endif

    ret = cts_plat_request_irq(cts_data->pdata);
    if (ret < 0) {
        cts_err("Request IRQ failed %d", ret);
        goto err_register_fb;
    }

#ifdef CONFIG_CTS_CHARGER_DETECT
    ret = cts_charger_detect_init(cts_data);
    if (ret)
        cts_err("Init charger detect failed %d", ret);
        /* Ignore this error */
#endif

#ifdef CONFIG_CTS_EARJACK_DETECT
    ret = cts_earjack_detect_init(cts_data);
    if (ret) {
        cts_err("Init earjack detect failed %d", ret);
        // Ignore this error
    }
#endif

    ret = cts_oem_init(cts_data);
    if (ret < 0) {
        cts_warn("Init oem specific faild %d", ret);
        goto err_deinit_oem;
    }

#ifdef CFG_CTS_HEARTBEAT_MECHANISM
    INIT_DELAYED_WORK(&cts_data->heart_work, cts_heartbeat_mechanism_work);
#endif

    /* Init firmware upgrade work and schedule */
    INIT_DELAYED_WORK(&cts_data->fw_upgrade_work, cts_firmware_upgrade_work);
    queue_delayed_work(cts_data->workqueue, &cts_data->fw_upgrade_work,
        /*HS07 code for HS07-227 by guyuming at 20250403 start*/
        msecs_to_jiffies(1 * 1000));
        /*HS07 code for HS07-227 by guyuming at 20250403 end*/

    INIT_WORK(&cts_data->ts_resume_work, cts_resume_work_func);

    /*HS07 code for SR-AL7761A-01-224 by guyuming at 20250317 start*/
    tp_feature_interface();
    /*HS07 code for SR-AL7761A-01-224 by guyuming at 20250317 end*/
    return 0;

err_deinit_oem:
    cts_oem_deinit(cts_data);

#ifdef CONFIG_CTS_EARJACK_DETECT
    cts_earjack_detect_deinit(cts_data);
#endif
#ifdef CONFIG_CTS_CHARGER_DETECT
    cts_charger_detect_deinit(cts_data);
#endif
    cts_plat_free_irq(cts_data->pdata);

err_register_fb:
#ifdef CONFIG_CTS_PM_FB_NOTIFIER
    cts_deinit_pm_fb_notifier(cts_data);
err_deinit_sysfs:
#endif
    cts_sysfs_remove_device(&client->dev);
#ifdef CONFIG_CTS_LEGACY_TOOL
    cts_tool_deinit(cts_data);
#endif

#ifdef CONFIG_CTS_ESD_PROTECTION
    cts_deinit_esd_protection(cts_data);
#endif

#ifdef CFG_CTS_GESTURE
    cts_plat_deinit_gesture(cts_data->pdata);
#endif
/*HS07 code for SR-AL7761A-01-265 by guyuming at 20250321 start*/
    tp_usb_notifier_unregister(&cts_data->charger_notify);
err_register_usb_notify_failed:
    cancel_work_sync(&cts_data->cts_charger_notify_wq);

    earphone_notifier_unregister(&cts_data->earphone_notify);
err_register_earphone_notify_failed:
    cancel_work_sync(&cts_data->earphone_work_queue);
/*HS07 code for SR-AL7761A-01-265 by guyuming at 20250321 ends*/

err_deinit_vkey_device:
#ifdef CONFIG_CTS_VIRTUALKEY
    cts_plat_deinit_vkey_device(cts_data->pdata);
#endif

err_deinit_touch_device:
    cts_plat_deinit_touch_device(cts_data->pdata);

err_free_resource:
    cts_plat_free_resource(cts_data->pdata);

err_destroy_heart_workqueue:
#ifdef CFG_CTS_HEARTBEAT_MECHANISM
    destroy_workqueue(cts_data->heart_workqueue);
err_destroy_esd_workqueue:
#endif

#ifdef CONFIG_CTS_ESD_PROTECTION
    destroy_workqueue(cts_data->esd_workqueue);
err_destroy_workqueue:
#endif
    destroy_workqueue(cts_data->workqueue);
err_deinit_platform_data:
    cts_deinit_platform_data(cts_data->pdata);
    kfree(cts_data->pdata);
err_free_cts_data:
    kfree(cts_data);

    cts_err("Probe failed %d", ret);

    return ret;
}

#ifdef CONFIG_CTS_I2C_HOST
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
static void cts_driver_remove(struct i2c_client *client)
#else
static int cts_driver_remove(struct i2c_client *client)
#endif
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
static void cts_driver_remove(struct spi_device *client)
#else
static int cts_driver_remove(struct spi_device *client)
#endif
#endif
{
    struct chipone_ts_data *cts_data;
    int ret = 0;

    cts_info("Remove");

#ifdef CONFIG_CTS_I2C_HOST
    cts_data = (struct chipone_ts_data *)i2c_get_clientdata(client);
#else
    cts_data = (struct chipone_ts_data *)spi_get_drvdata(client);
#endif
    if (cts_data) {
        ret = cts_stop_device(&cts_data->cts_dev);
        if (ret)
            cts_warn("Stop device failed %d", ret);

#ifdef CONFIG_CTS_CHARGER_DETECT
        cts_charger_detect_deinit(cts_data);
#endif
        /*HS07 code for SR-AL7761A-01-265 by guyuming at 20250321 start*/
        cts_earphone_deinit();
        cts_charger_deinit();
        /*HS07 code for SR-AL7761A-01-265 by guyuming at 20250321 end*/

#ifdef CONFIG_CTS_EARJACK_DETECT
        cts_earjack_detect_deinit(cts_data);
#endif

        cts_plat_free_irq(cts_data->pdata);

#ifdef CONFIG_CTS_PM_FB_NOTIFIER
        cts_deinit_pm_fb_notifier(cts_data);
#endif

        cts_tool_deinit(cts_data);

        cts_sysfs_remove_device(&client->dev);

        cts_deinit_esd_protection(cts_data);

        cts_plat_deinit_touch_device(cts_data->pdata);

        cts_plat_deinit_vkey_device(cts_data->pdata);

        cts_plat_deinit_gesture(cts_data->pdata);

        cts_plat_free_resource(cts_data->pdata);

        cts_oem_deinit(cts_data);

#ifdef CFG_CTS_HEARTBEAT_MECHANISM
        if (cts_data->heart_workqueue)
            destroy_workqueue(cts_data->heart_workqueue);
#endif

#ifdef CONFIG_CTS_ESD_PROTECTION
        if (cts_data->esd_workqueue)
            destroy_workqueue(cts_data->esd_workqueue);
#endif

        if (cts_data->workqueue)
            destroy_workqueue(cts_data->workqueue);

        cts_deinit_platform_data(cts_data->pdata);

        if (cts_data->pdata)
            kfree(cts_data->pdata);
        kfree(cts_data);
    } else {
        cts_warn("Chipone i2c driver remove while NULL chipone_ts_data");
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
        #else
        return -EINVAL;
        #endif
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
#else
    return ret;
#endif
}

#ifdef CONFIG_CTS_PM_LEGACY
static int cts_i2c_driver_suspend(struct device *dev, pm_message_t state)
{
    cts_info("Suspend by legacy power management");
    return cts_suspend(dev_get_drvdata(dev));
}

static int cts_i2c_driver_resume(struct device *dev)
{
    cts_info("Resume by legacy power management");
    return cts_resume(dev_get_drvdata(dev));
}
#endif /* CONFIG_CTS_PM_LEGACY */

#ifdef CONFIG_CTS_PM_GENERIC
static int cts_i2c_driver_pm_suspend(struct device *dev)
{
    cts_info("Suspend by bus power management");
    return cts_suspend(dev_get_drvdata(dev));
}

static int cts_i2c_driver_pm_resume(struct device *dev)
{
    cts_info("Resume by bus power management");
    return cts_resume(dev_get_drvdata(dev));
}

/* bus control the suspend/resume procedure */
static const struct dev_pm_ops cts_i2c_driver_pm_ops = {
    .suspend = cts_i2c_driver_pm_suspend,
    .resume = cts_i2c_driver_pm_resume,
};
#endif /* CONFIG_CTS_PM_GENERIC */

#ifdef CONFIG_CTS_SYSFS
static ssize_t reset_pin_show(struct device_driver *driver, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "CFG_CTS_HAS_RESET_PIN: %c\n",
#ifdef CFG_CTS_HAS_RESET_PIN
            'Y'
#else
            'N'
#endif
    );
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(reset_pin, S_IRUGO, reset_pin_show, NULL);
#else
static DRIVER_ATTR_RO(reset_pin);
#endif

static ssize_t swap_xy_show(struct device_driver *dev, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "CFG_CTS_SWAP_XY: %c\n",
#ifdef CFG_CTS_SWAP_XY
            'Y'
#else
            'N'
#endif
    );
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(swap_xy, S_IRUGO, swap_xy_show, NULL);
#else
static DRIVER_ATTR_RO(swap_xy);
#endif

static ssize_t wrap_x_show(struct device_driver *dev, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "CFG_CTS_WRAP_X: %c\n",
#ifdef CFG_CTS_WRAP_X
            'Y'
#else
            'N'
#endif
    );
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(wrap_x, S_IRUGO, wrap_x_show, NULL);
#else
static DRIVER_ATTR_RO(wrap_x);
#endif

static ssize_t wrap_y_show(struct device_driver *dev, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "CFG_CTS_WRAP_Y: %c\n",
#ifdef CFG_CTS_WRAP_Y
            'Y'
#else
            'N'
#endif
    );
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(wrap_y, S_IRUGO, wrap_y_show, NULL);
#else
static DRIVER_ATTR_RO(wrap_y);
#endif

static ssize_t force_update_show(struct device_driver *dev, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "CFG_CTS_HAS_RESET_PIN: %c\n",
#ifdef CFG_CTS_FIRMWARE_FORCE_UPDATE
            'Y'
#else
            'N'
#endif
    );
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(force_update, S_IRUGO, force_update_show, NULL);
#else
static DRIVER_ATTR_RO(force_update);
#endif

static ssize_t max_touch_num_show(struct device_driver *dev, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "CFG_CTS_MAX_TOUCH_NUM: %d\n",
            CFG_CTS_MAX_TOUCH_NUM);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(max_touch_num, S_IRUGO, max_touch_num_show, NULL);
#else
static DRIVER_ATTR_RO(max_touch_num);
#endif

static ssize_t vkey_show(struct device_driver *dev, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "CONFIG_CTS_VIRTUALKEY: %c\n",
#ifdef CONFIG_CTS_VIRTUALKEY
            'Y'
#else
            'N'
#endif
    );
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(vkey, S_IRUGO, vkey_show, NULL);
#else
static DRIVER_ATTR_RO(vkey);
#endif

static ssize_t gesture_show(struct device_driver *dev, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "CFG_CTS_GESTURE: %c\n",
#ifdef CFG_CTS_GESTURE
            'Y'
#else
            'N'
#endif
    );
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(gesture, S_IRUGO, gesture_show, NULL);
#else
static DRIVER_ATTR_RO(gesture);
#endif

static ssize_t esd_protection_show(struct device_driver *dev, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "CONFIG_CTS_ESD_PROTECTION: %c\n",
#ifdef CONFIG_CTS_ESD_PROTECTION
            'Y'
#else
            'N'
#endif
    );
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(esd_protection, S_IRUGO, esd_protection_show, NULL);
#else
static DRIVER_ATTR_RO(esd_protection);
#endif

static ssize_t slot_protocol_show(struct device_driver *dev, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "CONFIG_CTS_SLOTPROTOCOL: %c\n",
#ifdef CONFIG_CTS_SLOTPROTOCOL
            'Y'
#else
            'N'
#endif
    );
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(slot_protocol, S_IRUGO, slot_protocol_show, NULL);
#else
static DRIVER_ATTR_RO(slot_protocol);
#endif

static ssize_t max_xfer_size_show(struct device_driver *dev, char *buf)
{
#ifdef CONFIG_CTS_I2C_HOST
    return snprintf(buf, PAGE_SIZE, "CFG_CTS_MAX_I2C_XFER_SIZE: %d\n",
            CFG_CTS_MAX_I2C_XFER_SIZE);
#else
    return snprintf(buf, PAGE_SIZE, "CFG_CTS_MAX_SPI_XFER_SIZE: %d\n",
            CFG_CTS_MAX_SPI_XFER_SIZE);
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(max_xfer_size, S_IRUGO, max_xfer_size_show, NULL);
#else
static DRIVER_ATTR_RO(max_xfer_size);
#endif

static ssize_t driver_info_show(struct device_driver *dev, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "Driver version: %s\n",
            CFG_CTS_DRIVER_VERSION);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static DRIVER_ATTR(driver_info, S_IRUGO, driver_info_show, NULL);
#else
static DRIVER_ATTR_RO(driver_info);
#endif

static struct attribute *cts_i2c_driver_config_attrs[] = {
    &driver_attr_reset_pin.attr,
    &driver_attr_swap_xy.attr,
    &driver_attr_wrap_x.attr,
    &driver_attr_wrap_y.attr,
    &driver_attr_force_update.attr,
    &driver_attr_max_touch_num.attr,
    &driver_attr_vkey.attr,
    &driver_attr_gesture.attr,
    &driver_attr_esd_protection.attr,
    &driver_attr_slot_protocol.attr,
    &driver_attr_max_xfer_size.attr,
    &driver_attr_driver_info.attr,
    NULL
};

static const struct attribute_group cts_i2c_driver_config_group = {
    .name = "config",
    .attrs = cts_i2c_driver_config_attrs,
};

static const struct attribute_group *cts_i2c_driver_config_groups[] = {
    &cts_i2c_driver_config_group,
    NULL,
};
#endif /* CONFIG_CTS_SYSFS */

#ifdef CONFIG_CTS_OF
static const struct of_device_id cts_i2c_of_match_table[] = {
    {.compatible = CFG_CTS_OF_DEVICE_ID_NAME, },
    { },
};

MODULE_DEVICE_TABLE(of, cts_i2c_of_match_table);
#endif /* CONFIG_CTS_OF */

#ifdef CONFIG_CTS_I2C_HOST
static const struct i2c_device_id cts_device_id_table[] = {
    { CFG_CTS_DEVICE_NAME, 0 },
    { }
};
#else
static const struct spi_device_id cts_device_id_table[] = {
    { CFG_CTS_DEVICE_NAME, 0 },
    { }
};
#endif

#ifdef CONFIG_CTS_I2C_HOST
static struct i2c_driver cts_i2c_driver = {
#else
static struct spi_driver cts_spi_driver = {
#endif
    .probe = cts_driver_probe,
    .remove = cts_driver_remove,
    .driver = {
        .name = CFG_CTS_DRIVER_NAME,
        .owner = THIS_MODULE,
#ifdef CONFIG_CTS_OF
        .of_match_table = of_match_ptr(cts_i2c_of_match_table),
#endif /* CONFIG_CTS_OF */
#ifdef CONFIG_CTS_SYSFS
        .groups = cts_i2c_driver_config_groups,
#endif /* CONFIG_CTS_SYSFS */
#ifdef CONFIG_CTS_PM_LEGACY
        .suspend = cts_i2c_driver_suspend,
        .resume = cts_i2c_driver_resume,
#endif /* CONFIG_CTS_PM_LEGACY */
#ifdef CONFIG_CTS_PM_GENERIC
        .pm = &cts_i2c_driver_pm_ops,
#endif /* CONFIG_CTS_PM_GENERIC */

        },
    .id_table = cts_device_id_table,
};

static int __init cts_driver_init(void)
{
    cts_info("2024/8/28 17:30 Chipone touch driver init, version: "CFG_CTS_DRIVER_VERSION);

#ifdef CONFIG_CTS_I2C_HOST
    cts_info(" - Register i2c driver");
    return i2c_add_driver(&cts_i2c_driver);
#else
    /*HS07 code for SR-AL7761A-01-276 by guyuming at 20250321 start*/
    if (tp_detect_panel("nl9916x") == 0) {
        cts_info(" - Register spi driver");
        return spi_register_driver(&cts_spi_driver);
    } else {
        return -EFAULT;
    }
    /*HS07 code for SR-AL7761A-01-276 by guyuming at 20250321 end*/
#endif
}

static void __exit cts_driver_exit(void)
{
    cts_info("Exit");

#ifdef CONFIG_CTS_I2C_HOST
    i2c_del_driver(&cts_i2c_driver);
#else
    spi_unregister_driver(&cts_spi_driver);
#endif
}

module_init(cts_driver_init);
module_exit(cts_driver_exit);

#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif
MODULE_DESCRIPTION("Chipone TDDI touchscreen Driver for QualComm platform");
MODULE_VERSION(CFG_CTS_DRIVER_VERSION);
MODULE_AUTHOR("Miao Defang <dfmiao@chiponeic.com>");
MODULE_LICENSE("GPL");
