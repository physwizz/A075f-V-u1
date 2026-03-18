#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/sched/task.h>
#include <linux/sched/signal.h>
#include <linux/mm_types.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include "hperf.h"
#include "log.h"

#define PNAME_SIZE_MAX 1024

void get_pname_from_path(const char *path, char *pname, size_t pname_size) {
    const char *p, *start;
    size_t name_len;
    if (!path || !pname || pname_size == 0) {
        if (pname && pname_size > 0) pname[0] = '\0';
        return;
    }
    p = path + strlen(path);
    while (p > path && *(p - 1) == '/') { p--; }
    start = p;
    while (start > path && *(start - 1) != '/') { start--; }
    name_len = p - start;
    if (name_len == 0 && pname_size > 1) {
        strncpy(pname, "/", pname_size);
        pname[pname_size - 1] = '\0';
        return;
    }
    if (name_len >= pname_size) {
        name_len = pname_size - 1;
    }
    strncpy(pname, start, name_len);
    pname[name_len] = '\0';
}

int get_mm_cmdline(struct task_struct *task, char *buffer, int buflen) {
    int res = 0;
    unsigned int len;
    struct mm_struct *mm;
    unsigned long arg_start, arg_end, env_start, env_end;
    mm = get_task_mm(task);
    if (!mm) {
        if (task->state == EXIT_ZOMBIE || task->state == EXIT_DEAD) {
            Loge("pid(%d) is exiting", task->pid);
        } else {
            Loge("pid(%d) unknown reason for missing mm", task->pid);
        }
        goto out;
    }
    if (!mm->arg_end) {
        Loge("mm arg_end is null");
        goto out_mm;
    }

    spin_lock(&mm->arg_lock);
    arg_start = mm->arg_start;
    arg_end = mm->arg_end;
    env_start = mm->env_start;
    env_end = mm->env_end;
    spin_unlock(&mm->arg_lock);

    len = arg_end - arg_start;

    if (len > buflen) {
        len = buflen;
    }

    res = access_process_vm(task, arg_start, buffer, len, FOLL_FORCE);

    /*
    * If the nul at the end of args has been overwritten, then
    * assume application is using setproctitle(3).
    */
    if (res > 0 && buffer[res-1] != '\0' && len < buflen) {
        len = strnlen(buffer, res);
        if (len < res) {
            res = len;
        } else {
            len = env_end - env_start;
            if (len > buflen - res) {
                len = buflen - res;
            }
            res += access_process_vm(task, env_start, buffer+res, len, FOLL_FORCE);
            res = strnlen(buffer, res);
        }
    }
out_mm:
    mmput(mm);
out:
    return res;
}

int get_pids_by_pname(const char *pname, int *pids, int *pids_cnt) {
    struct task_struct *task = NULL;
    char cmdline[PNAME_SIZE_MAX];
    char pname_from_cmd[PNAME_SIZE_MAX];
    int len = 0;
    int cnt = 0;
    Logd("E");
    read_lock(&tasklist_lock);
    for_each_process(task) {
        if (task->flags & PF_KTHREAD) {
            if (strncmp(task->comm, pname, PNAME_SIZE_MAX) == 0) {
                if (MAX_PIDS == cnt) {
                    Loge("pids is full, cant add any more");
                    break;
                }
                pids[cnt] = task->pid;
                Logd("[kt]: pname=%s, pids[%d]=%d", task->comm, cnt, task->pid);
                cnt++;
            }
        } else {
            len = get_mm_cmdline(task, cmdline, sizeof(cmdline));
            if (len > 0) {
                get_pname_from_path(cmdline, pname_from_cmd, PNAME_SIZE_MAX);
                if (strncmp(pname_from_cmd, pname, PNAME_SIZE_MAX) == 0) {
                    if (MAX_PIDS == cnt) {
                        Loge("pids is full, cant add any more");
                        break;
                    }
                    pids[cnt] = task->pid;
                    Logd("[ut]: pname=%s, pids[%d]=%d", pname_from_cmd, cnt, task->pid);
                    cnt++;
                }
            }
        }
    }
    *pids_cnt = cnt;
    read_unlock(&tasklist_lock);
    Logd("X, pids_cnt is %d", *pids_cnt);
    return 0;
}

