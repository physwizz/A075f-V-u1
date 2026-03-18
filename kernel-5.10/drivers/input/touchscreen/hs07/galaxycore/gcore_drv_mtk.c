/*
 * GalaxyCore touchscreen driver
 *
 * Copyright (C) 2021 GalaxyCore Incorporated
 *
 * Copyright (C) 2021 Neo Chen <neo_chen@gcoreinc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "gcore_drv_common.h"
#ifdef CONFIG_DRM
#include <drm/drm_panel.h>
#endif

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
static int tpd_local_init(void);
static void tpd_suspend(struct device *h);
static void tpd_resume(struct device *h);

static struct tpd_driver_t tpd_device_driver = {
    .tpd_device_name = "gcore",
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
};
#endif

#ifdef CONFIG_ENABLE_PROXIMITY_TP_SCREEN_OFF
#ifdef CONFIG_MTK_PROXIMITY_TP_SCREEN_ON_ALSPS
static int mtk_gcore_proximity_local_init(void);
static int mtk_gcore_proximity_local_uninit(void);
static int tpd_get_ps_value(void);


static struct alsps_init_info mtk_gcore_proximity_info = {
    .name = TP_PS_DEVICE,
    .init = mtk_gcore_proximity_local_init,
    .uninit = mtk_gcore_proximity_local_uninit,
};

static int ps_open_report_data(int open)
{
    /* should queuq work to report event if  is_report_input_direct=true */
    return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL*/
static int ps_enable_nodata(int en)
{
    tpd_enable_ps(en);

    return 0;
}

static int ps_set_delay(u64 ns)
{
    return 0;
}

static int ps_get_data(int *value, int *status)
{

    *value = tpd_get_ps_value();
    *status = SENSOR_STATUS_ACCURACY_MEDIUM;

    return 0;
}
/*
static int ps_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
    int value = 0;

    value = (int)samplingPeriodNs / 1000 / 1000;
    //FIX  ME

    return 0;
}

static int ps_flush(void)
{
    return ps_flush_report();
}
*/
static int tpd_get_ps_value(void)
{
    if(false == fn_data.gdev->tel_screen_off){
        return 0;       
    }else{
        return 10;
    }
}


static int mtk_gcore_proximity_local_init(void)
{
    int nErr;

    struct ps_control_path ps_ctl = { 0 };
    struct ps_data_path ps_data = { 0 };

    ps_ctl.open_report_data = ps_open_report_data;
    ps_ctl.enable_nodata = ps_enable_nodata;
    ps_ctl.set_delay = ps_set_delay;
    ps_ctl.is_report_input_direct = true;
    //ps_ctl.batch = ps_batch;
    //ps_ctl.flush = ps_flush;
    ps_ctl.is_support_batch = false;

    GTP_DEBUG("proximity function start init");
    nErr = ps_register_control_path(&ps_ctl);
    if(nErr){
        GTP_ERROR("ps_register_control_path() failed = %d", nErr);
    }

    ps_data.get_data = ps_get_data;
    ps_data.vender_div = 100;
    nErr = ps_register_data_path(&ps_data);
    if (nErr) {
        GTP_ERROR("ps_register_data_path() failed = %d", nErr);
    }
    
    return 0;
}

/*----------------------------------------------------------------------------*/

static int mtk_gcore_proximity_local_uninit(void)
{
    return 0;
}
#endif
#endif

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
static int tpd_local_init(void)
{
    GTP_DEBUG("tpd_local_init start.");

    if (gcore_touch_bus_init()) {
        GTP_ERROR("bus init fail!");
        return -EPERM;
    }

    if (tpd_load_status == 0) {
        GTP_ERROR("add error touch panel driver.");
        gcore_touch_bus_exit();
        return -EPERM;
    }

    GTP_DEBUG("end %s, %d\n", __func__, __LINE__);
    tpd_type_cap = 1;

    return 0;
}
#endif
static void tpd_resume(struct device *h)
{
    GTP_DEBUG("gcore resume start...");
    
#ifdef    CONFIG_ENABLE_PROXIMITY_TP_SCREEN_OFF
        if(fn_data.gdev->PS_Enale == true){
            tpd_enable_ps(1);
        }
    
#endif


#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if(fn_data.gdev->gesture_wakeup_en) {
        disable_irq_wake(fn_data.gdev->touch_irq);
    }
#endif
/*HS07 code for HS07-1477 by huangyin at 20250425 start*/
#ifdef CONFIG_GCORE_AUTO_UPDATE_FW_HOSTDOWNLOAD
    if(fn_data.gdev->ts_stat == TS_MPTEST){
        GTP_DEBUG("ITO test is runing");
        queue_delayed_work(fn_data.gdev->fwu_workqueue, &fn_data.gdev->fwu_work, \
                            msecs_to_jiffies(2000));
    }else{
        //gcore_request_firmware_update_work(NULL);
        queue_delayed_work(fn_data.gdev->fwu_workqueue, &fn_data.gdev->fwu_work, \
                            msecs_to_jiffies(20));
    }
#else
/*HS07 code for HS07-1477 by huangyin at 20250425 end*/
#if CONFIG_GCORE_RESUME_EVENT_NOTIFY
        queue_delayed_work(fn_data.gdev->gtp_workqueue, &fn_data.gdev->resume_notify_work, msecs_to_jiffies(200));
#endif
    gcore_touch_release_all_point(fn_data.gdev->input_device);
#endif
    fn_data.gdev->ts_stat = TS_NORMAL;

    GTP_DEBUG("gcore resume end.");
}

static void tpd_suspend(struct device *h)
{
    GTP_DEBUG("gcore suspend start...");
    /*HS07 code for HS07-249 by huangyin at 20250331 start*/
    gcore_modify_fw_event_cmd(DRIVER_REGISTER_END);
    /*HS07 code for HS07-249 by huangyin at 20250331 end*/

#ifdef    CONFIG_ENABLE_PROXIMITY_TP_SCREEN_OFF
    if(gcore_tpd_proximity_flag && gcore_tpd_proximity_flag_off){
        fn_data.gdev->ts_stat = TS_SUSPEND;
        GTP_DEBUG("Proximity TP Now.");
        return ;
    }

#endif


#if GCORE_WDT_RECOVERY_ENABLE
    cancel_delayed_work_sync(&fn_data.gdev->wdt_work);
#endif

    cancel_delayed_work_sync(&fn_data.gdev->fwu_work);

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if(fn_data.gdev->gesture_wakeup_en) {
        enable_irq_wake(fn_data.gdev->touch_irq);
    }
#endif
#ifdef CONFIG_SAVE_CB_CHECK_VALUE
        fn_data.gdev->CB_ckstat = false;
#endif

    fn_data.gdev->ts_stat = TS_SUSPEND;

    gcore_touch_release_all_point(fn_data.gdev->input_device);

    GTP_DEBUG("gcore suspend end.");
}


#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
static int __init touch_driver_init(void)
{
    GTP_DEBUG("mtk touch_driver_init start.");
    tpd_get_dts_info();
    if (tpd_driver_add(&tpd_device_driver) < 0) {
        GTP_ERROR("add generic driver failed\n");
    }
#ifdef CONFIG_ENABLE_PROXIMITY_TP_SCREEN_OFF
#ifdef CONFIG_MTK_PROXIMITY_TP_SCREEN_ON_ALSPS
    alsps_driver_add(&mtk_gcore_proximity_info);
#endif
#endif

    return 0;
}

/* should never be called */
static void __exit touch_driver_exit(void)
{
    GTP_DEBUG("mtk touch_driver_exit exit\n");
    tpd_driver_remove(&tpd_device_driver);
}

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
int gcore_ts_drm_notifier_callback(struct notifier_block *self,
        unsigned long event, void *v)
{
    const unsigned long event_enum[2] = {MTK_DISP_EARLY_EVENT_BLANK, MTK_DISP_EVENT_BLANK};
    const int blank_enum[2] = {MTK_DISP_BLANK_POWERDOWN, MTK_DISP_BLANK_UNBLANK};
    int blank_value = *((int *)v);

    GTP_DEBUG("notifier,event:%lu,blank:%d", event, blank_value);
    if ((blank_enum[1] == blank_value) && (event_enum[1] == event)) {
       tpd_resume(NULL);
    } else if ((blank_enum[0] == blank_value) && (event_enum[0] == event)) {
        tpd_suspend(NULL);
    } else {
        GTP_DEBUG("notifier,event:%lu,blank:%d, not care", event, blank_value);
    }
    return 0;
}
static int __init touch_driver_init(void)
{
    GTP_DEBUG("mtk platform use qcom setting, touch driver init.");
    if (gcore_touch_bus_init()) {
        GTP_ERROR("bus init fail!");
        return -EPERM;
    }
    return 0;
}
static void __exit touch_driver_exit(void)
{
    gcore_touch_bus_exit();
}
#endif

module_init(touch_driver_init);
module_exit(touch_driver_exit);

#if GKI_CLOSE_CAN_SAVE_FILE
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
MODULE_IMPORT_NS(ANDROID_GKI_VFS_EXPORT_ONLY);
#endif

MODULE_AUTHOR("GalaxyCore, Inc.");
MODULE_DESCRIPTION("GalaxyCore Touch Main Mudule");
MODULE_LICENSE("GPL");
