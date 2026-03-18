/**
 *  Change Log:
 *
 *   20250626    add Process Freeze Whitelist Mode for HS07-899    hujingcheng
*/
#include "hperf.h"
#include "log.h"
#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/signal.h>
#include <linux/pid_namespace.h>
#include <linux/pid.h>
#include <linux/oom.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/kthread.h>

#define PNAME_SIZE_MAX 1024

struct zygote_child {
    struct list_head list;
    struct task_struct *task;
};

static ktime_t freeze_start_time;

enum freeze_thread_state {
    FREEZE_THREAD_STOPPED,
    FREEZE_THREAD_RUNNING
};

static struct task_struct *freeze_kthread = NULL;
static enum freeze_thread_state freeze_state = FREEZE_THREAD_STOPPED;

struct target_info {
    const char *name;
    pid_t pid;
};

static struct target_info zygote_proc[] = {
    {"/system/bin/app_process64", -1},
    {"/system/bin/app_process32", -1},
    {NULL, -1}
};

static struct target_info app_white_list[] = {
    {"system_server", -1},
    {"webview_zygote", -1},
    {"com.android.systemui", -1},
    {"com.android.phone", -1},
    {"com.android.launcher3", -1},
    {"com.android.permissioncontroller", -1},
    {"com.sec.android.app.camerasaver", -1},
    {"com.sec.android.app.camera", -1},
    {"com.android.providers.media.module", -1},
    {"android.process.media", -1},
    {"com.android.se", -1},
    {"com.android.traceur", -1},
    {"com.debug.loggerui", -1},
    // sec apps
    {"com.salab.act_agent", -1}, /* sec test agent */
    {"com.sec.android.app.launcher", -1}, /* sec launcher */
    {"com.google.android.permissioncontroller", -1}, /* google permission controller */
    {"com.sec.android.gallery3d", -1}, /* sec gallery */
    {"com.sec.epdg", -1}, /* sec app */
    {"com.google.android.providers.media.module", -1}, /* google media provider */
    {"com.google.android.googlequicksearchbox:search", -1}, /* sec launcher related */
    {"com.sec.phone", -1}, /* sec phone */
    {"com.android.bluetooth", -1}, /* bluetooth */
    {"com.sec.android.diagmonagent", -1}, /* sec diagmonagent */
    {"com.samsung.cmh", -1}, /* sec cmh */
    {NULL, -1}
};

/**
 *  get task_struct by pid
 * */
struct task_struct *find_task_by_pid(pid_t pid) {
    struct pid *pid_struct = NULL;
    struct task_struct *task = NULL;

    pid_struct = find_get_pid(pid);
    if (!pid_struct) {
        return NULL;
    }
    task =  pid_task(pid_struct, PIDTYPE_PID);

    if (task) {
        get_task_struct(task);
    }

    put_pid(pid_struct);
    return task;
}

void my_sleep(long timeout_ms) {
    long timeout_jiffies = msecs_to_jiffies(timeout_ms);
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(timeout_jiffies);
}

/**
*  find zygote pid by:
*  exe_file == system/bin/app_process[32|64] && ppid == 1
*/
static int find_zygote_pid(void) {
    struct target_info *t;
    int cnt = 0;
    struct task_struct *task = NULL;
    struct mm_struct *mm;
    char* exe_path;
    char buf[PATH_MAX];
    struct path *exe_path_ptr;

    // Zygote process doesn't change if no soft reboot, so use cache firstly.
    for(t = zygote_proc; t->name; t++) {
        if(t->pid > 0 && t->pid < PID_MAX_LIMIT) {
            Logi("zygote process found %d (Cached)", t->pid);
            cnt++;
        }
    }
    if (cnt == 2) {
        return cnt;
    } else {
        Logi("Zygote Cache incomplete,  count %d, check directly!", cnt);
        cnt = 0;
    }

    /* check by traversal */
    rcu_read_lock();
    for_each_process(task) {
        mm = get_task_mm(task);
        if(!mm) {
            continue;
        }
        if (!mm->exe_file) {
            mmput(mm);
            continue;
        }

        if (!task->real_parent || task->real_parent->pid != 1) {
            mmput(mm);
            continue;
        }

        exe_path_ptr = &mm->exe_file->f_path;
        exe_path = d_path(exe_path_ptr, buf, sizeof(buf));
        if (exe_path) {
            for(t = zygote_proc; t->name; t++) {
                if (strcmp(t->name, exe_path) == 0) {
                    t->pid = task->pid;
                    cnt++;
                    Logi("find zygote process %d\n", t->pid);
                    break;
                }
            }
        }

        mmput(mm);
    }
    rcu_read_unlock();
    return cnt;
}

/**
* freeze/unfreeze java app not in whitelist
*/
static int ctrl_zygote_fork_proc(int signal) {
    struct task_struct *child = NULL;
    struct target_info *t;
    struct list_head zygote_children;
    struct zygote_child *entry = NULL;
    struct zygote_child *temp_entry = NULL;
    int ret = 0;

    char *cmdline;

    Logi("ctrl_zygote_fork_proc xxx E");
    cmdline = kmalloc(PNAME_SIZE_MAX, GFP_KERNEL);
    if (!cmdline) {
        Loge("Failed to allocate cmdline\n");
        return -ENOMEM;
    }

    INIT_LIST_HEAD(&zygote_children);

    for_each_process(child) {
        for (t = zygote_proc; t->name; t++) {
            if (t->pid > 0 && child->real_parent && t->pid == child->real_parent->pid) {
                if (child->exit_state != 0 || (child->flags & PF_EXITING)) {
                    continue;
                }

                entry = kmalloc(sizeof(*entry), GFP_ATOMIC);
                if (!entry) {
                    Loge("Failed to allocate zygote_child entry\n");
                    continue;
                }

                entry->task = get_task_struct(child);
                list_add_tail(&entry->list, &zygote_children);
                break;
            }
        }
    }

    list_for_each_entry(entry, &zygote_children, list) {
        struct target_info *t_white;
        struct mm_struct *mm;
        int len;
        bool white_list_detected = false;

        struct task_struct* pos = entry->task;

        if (pos->exit_state != 0 || (pos->flags & PF_EXITING)) {
            put_task_struct(pos);
            continue;
        }

        mm = pos->mm;
        if (!mm) {
            put_task_struct(pos);
            continue;
        }

        len = get_mm_cmdline(pos, cmdline, PNAME_SIZE_MAX);

        for (t_white = app_white_list; t_white->name; t_white++) {
            const size_t white_len = strlen(t_white->name);
            if (len < white_len || white_len == 0) {
                continue;
            }
            if (strncmp(cmdline, t_white->name, white_len) == 0 && cmdline[white_len] == '\0') {
                //Logd("whitelist proc detect %s, skip\n", cmdline);
                white_list_detected = true;
                break;
            }
        }

        if (!white_list_detected) {
            send_sig(signal, pos, 0);
            Logi("Not whitelist %d (%s), send signal %d !\n", pos->pid, cmdline, signal);
        }

        put_task_struct(pos);
    }

    list_for_each_entry_safe(entry, temp_entry, &zygote_children, list) {
        list_del(&entry->list);
        kfree(entry);
    }
    kfree(cmdline);

    Logi("ctrl_zygote_fork_proc xxx X");
    return ret;
}

/**
* clean work when exit
*/
void hmem_exit() {
    if (freeze_kthread && freeze_state == FREEZE_THREAD_RUNNING) {
        kthread_stop(freeze_kthread);
        freeze_kthread = NULL;
        freeze_state = FREEZE_THREAD_STOPPED;
        Logi("hmem_exit - Freeze thread stopped\n");
    }
}

/**
* hperf_freeze thread
*/
static int freeze_thread_func(void *data) {
    ktime_t current_time;
    unsigned long elapsed_ms;

    freeze_start_time = ktime_get_boottime();

    while (!kthread_should_stop()) {
        current_time = ktime_get_boottime();
        elapsed_ms = ktime_to_ms(ktime_sub(current_time, freeze_start_time));
        if (elapsed_ms >= FREEZE_THREAD_MAX_RUNNING) {
            Logi("Freeze thread timed out (elapsed: %lu ms), exit!\n", elapsed_ms);
            freeze_state = FREEZE_THREAD_STOPPED;
            return 0;
        }

        ctrl_zygote_fork_proc(SIGSTOP);
        my_sleep(FREEZE_TIME);

        ctrl_zygote_fork_proc(SIGCONT);
        my_sleep(FREEZE_INTERVAL);
    }

    ctrl_zygote_fork_proc(SIGCONT);
    Logi("Freeze thread exited\n");
    return 0;
}

int send_signal_by_pid(int target_pid, int signal) {
    struct task_struct *task;
    Logi("E");

    if (target_pid <=0 || target_pid >= PID_MAX_LIMIT) {
        Loge("PID %ld out of range (1-%d)\n", target_pid, PID_MAX_LIMIT - 1);
        return -EINVAL;
    }

    task = find_task_by_pid(target_pid);

    if (!task) {
        Loge("Process %d not found\n", target_pid);
        return -ESRCH;
    }

    if (task->exit_state != 0 || task->flags & PF_EXITING) {
        Loge("Process %d already exited or existing\n", target_pid);
        put_task_struct(task);
        return -EALREADY;
    }

    if (task->flags & PF_KTHREAD) {
        Loge("Process %d is kernel thread, skip\n", target_pid);
        put_task_struct(task);
        return -EOPNOTSUPP;
    }

    if (!task->signal) {
        Loge("Process %d  task->signal not found\n", target_pid);
        put_task_struct(task);
        return -ESRCH;
    }

    send_sig(signal, task,0);
    Logi("Sent Signal %d to PID %d (%s) oom %d\n", signal, target_pid, task->comm, task->signal->oom_score_adj);
    put_task_struct(task);
    Logi("X");
    return 0;
}

int process_signal_ctrl(struct  hperf_process*  hperf_sig_process) {
    int hperf_signal, i;

    Logi("E");
    if (!hperf_sig_process) {
        Loge("Invalid hperf_process sent!\n");
        return -EINVAL;
    }

    hperf_signal = hperf_sig_process->signal;
    Logi("hperf_signal get %d!\n", hperf_signal);

    switch (hperf_signal) {
        case SIGKILL:
        case SIGSTOP:
        case SIGCONT:
            for (i = 0; i < MAX_PIDS; i++) {
                if (hperf_sig_process->pids[i] > 0) {
                    send_signal_by_pid(hperf_sig_process->pids[i], hperf_signal);
                }
            }
            break;
        case SIG_FREEZE_EXCEPT_WHITELIST:
            if (find_zygote_pid() < 1) {
                Loge("zygote process not found!");
                break;
            }

            if (freeze_state == FREEZE_THREAD_STOPPED) {
                freeze_kthread = kthread_run(freeze_thread_func, NULL, "hperf_freeze");
                if (!IS_ERR(freeze_kthread)) {
                    freeze_state = FREEZE_THREAD_RUNNING;
                    Logi("Freeze thread started\n");
                } else {
                    Loge("Failed to start freeze thread: %ld\n", PTR_ERR(freeze_kthread));
                }
            } else {
                freeze_start_time = ktime_get_boottime();
                Logi("Freeze thread timeout reset\n");
            }
            break;
        case SIG_UNFREEZE_EXCEPT_WHITELIST:
            if (freeze_state == FREEZE_THREAD_RUNNING) {
                kthread_stop(freeze_kthread);
                freeze_kthread = NULL;
                freeze_state = FREEZE_THREAD_STOPPED;
                Logi("Freeze thread stopped\n");
            }
            break;
        default:
            Loge("Unsupported Signal %d!\n", hperf_signal);
            return -EINVAL;
    }
    Logi("X");
    return 0;
}
