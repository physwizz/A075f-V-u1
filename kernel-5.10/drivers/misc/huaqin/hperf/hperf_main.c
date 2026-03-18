#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include "log.h"
#include "hperf.h"

static struct proc_dir_entry *hperf_proc_entry;
static struct hperf_info hperf_data = {
    .version = 1,
    .name = "hperf_module",
};

static int hperf_proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "hperf Module:\n");
    seq_printf(m, "Version: %d\n", hperf_data.version);
    seq_printf(m, "Name: %s\n", hperf_data.name);
    return 0;
}

static int hperf_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, hperf_proc_show, NULL);
}
struct hperf_process hperf_process;

static long hperf_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct hperf_info info;
    /* HS07 code for HS07-899 by hujingcheng at 20250621 start*/
    struct hperf_cpu_mask cpu_mask;
    /* HS07 code for HS07-899 by hujingcheng at 20250621 end*/
    /* HST11 code for AX7800A-1996 by wangletian at 20250613 start */
    struct hperf_pinfo hperf_pinfo;
    /* HST11 code for AX7800A-1996 by wangletian at 20250613 end */
    int ret = 0;

    Logi("E cmd 0x%x", cmd);
    switch (cmd) {
        case HPERF_GET_INFO:
            if (copy_to_user((void __user *)arg, &hperf_data, sizeof(struct hperf_info))) {
                ret = -EFAULT;
            }
            break;
        case HPERF_SET_INFO:
            if (copy_from_user(&info, (void __user *)arg, sizeof(struct hperf_info))) {
                ret = -EFAULT;
            } else {
                hperf_data.version = info.version;
                strncpy(hperf_data.name, info.name, sizeof(hperf_data.name) - 1);
                hperf_data.name[sizeof(hperf_data.name) - 1] = '\0';
            }
            break;
        case HPERF_IOC_PROCESS_CTRL:
             if (copy_from_user(&hperf_process, (void __user *)arg, sizeof(struct hperf_process))) {
                ret = -EFAULT;
                break;
            }
            process_signal_ctrl(&hperf_process);
            break;
        case HPERF_IOC_PROCESS_CTRL_REPLY:
            if (copy_to_user((void __user *)arg, &hperf_process, sizeof(struct hperf_process))) {
                ret = -EFAULT;
            }
            break;
        case HPERF_IOC_CPU_BOOST:
            // if (copy_to_user((void __user *)arg, &hperf_process, sizeof(struct hperf_process))) {
            //     ret = -EFAULT;
            // }
            Logi("HPERF_IOC_CPU_BOOST");
            set_max_freq_init();
            break;
        /* HST11 code for AX7800A-1996 by wangletian at 20250613 start */
        case HPERF_IOC_GET_PID_BY_PNAME:
            {
                if (copy_from_user(&hperf_pinfo, (void __user *)arg, sizeof(struct hperf_pinfo))) {
                    Loge("getpid cpfu error");
                    ret = -EFAULT;
                    break;
                }
                get_pids_by_pname(hperf_pinfo.pname, hperf_pinfo.pids, &(hperf_pinfo.pids_cnt));
                if (copy_to_user((void __user *)arg, &hperf_pinfo, sizeof(struct hperf_pinfo))) {
                    Loge("getpid cptu error");
                    ret = -EFAULT;
                }
                break;
            }
        /* HST11 code for AX7800A-1996 by wangletian at 20250613 end */
         /* HS07 code for HS07-899 by hujingcheng at 20250621 start*/
        case HPERF_IOC_CPU_BIND:
             if (copy_from_user(&cpu_mask, (void __user *)arg, sizeof(struct hperf_cpu_mask))) {
                ret = -EFAULT;
                break;
            }
            Logi("HPERF_IOC_CPU_BIND");
            bind_process_core(&cpu_mask);
            break;
         /* HS07 code for HS07-899 by hujingcheng at 20250621 end*/
        default:
            ret = -ENOTTY;
    }
    return ret;
}

static const struct proc_ops hperf_proc_fops = {
    .proc_open = hperf_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
    .proc_ioctl = hperf_ioctl,
};

static int __init hperf_init(void) {
    // create /proc/hperf
    hperf_proc_entry = proc_create("hperf", 0666, NULL, &hperf_proc_fops);
    if (!hperf_proc_entry) {
        Loge("Failed to create /proc/hperf\n");
        return -ENOMEM;
    }
    Logi("hperf module loaded\n");
    return 0;
}

static void __exit hperf_exit(void) {
    /* HS07 code for HS07-899 by hujingcheng at 20250626 start*/
    hmem_exit();
    /* HS07 code for HS07-899 by hujingcheng at 20250626 end*/
    proc_remove(hperf_proc_entry);
    Logi("hperf module unloaded\n");
}

module_init(hperf_init);
module_exit(hperf_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hperf driver");
