#ifndef __HUAQIN_HPERF_H
#define __HUAQIN_HPERF_H
#include <linux/types.h>
#include <linux/sched.h>

/* HST11 code for AX7800A-1996 by wangletian at 20250613 start */
#define MAX_PNAME_SIZE 256
/* HST11 code for AX7800A-1996 by wangletian at 20250613 end */
#define CPU_NUMBER 8

/* HS07 code for HS07-899 by hujingcheng at 20250626 start */
#define MAX_PIDS 64
#define SIG_PROC_STAT_CTRL 129
#define SIG_FREEZE_EXCEPT_WHITELIST 130
#define SIG_UNFREEZE_EXCEPT_WHITELIST 131
/* SIGSTOP AUTO RECOVERY TIME: 500ms */
#define FREEZE_TIME 500
/* SIGSTOP AUTO SENT INTERVAL: 50ms */
#define FREEZE_INTERVAL 50
/* Freeze Thread Timeout: 10s */
#define FREEZE_THREAD_MAX_RUNNING 10000
/* HS07 code for HS07-899 by hujingcheng at 20250626 end */

#define HPERF_IOC_MAGIC 'h'
#define HPERF_GET_INFO _IOR(HPERF_IOC_MAGIC, 1, struct hperf_info)
#define HPERF_SET_INFO _IOW(HPERF_IOC_MAGIC, 2, struct hperf_info)

#define HPERF_IOC_PROCESS_CTRL _IOW(HPERF_IOC_MAGIC, 3, struct hperf_process)
#define HPERF_IOC_PROCESS_CTRL_REPLY _IOR(HPERF_IOC_MAGIC, 4, struct hperf_process)

// CPU Boost
#define HPERF_IOC_CPU_BOOST _IOW(HPERF_IOC_MAGIC, 5,struct hperf_cpu_boost)
#define HPERF_IOC_CPU_BOOST_REPLY _IOR(HPERF_IOC_MAGIC, 6, struct hperf_cpu_boost)

// GPU Boost
#define HPERF_IOC_GPU_BOOST _IOW(HPERF_IOC_MAGIC, 7,struct hperf_gpu_boost)
#define HPERF_IOC_GPU_BOOST_REPLY _IOR(HPERF_IOC_MAGIC, 8, struct hperf_gpu_boost)

// DDR Boost
#define HPERF_IOC_DDR_BOOST _IOW(HPERF_IOC_MAGIC, 9,struct hperf_ddr_boost)
#define HPERF_IOC_DDR_BOOST_REPLY _IOR(HPERF_IOC_MAGIC, 10, struct hperf_ddr_boost)

// CPU BIND
#define HPERF_IOC_CPU_BIND _IOW(HPERF_IOC_MAGIC, 11,struct hperf_cpu_mask)
#define HPERF_IOC_CPU_BIND_REPLY _IOR(HPERF_IOC_MAGIC, 12, struct hperf_cpu_mask)

/* HST11 code for AX7800A-1996 by wangletian at 20250613 start */
// get pids by pname
#define HPERF_IOC_GET_PID_BY_PNAME _IOW(HPERF_IOC_MAGIC, 13, struct hperf_pinfo)
/* HST11 code for AX7800A-1996 by wangletian at 20250613 end */

struct hperf_info {
    int version;
    char name[32];
};

struct hperf_cpu_boost {
    int pid;
    int max_cpu_freq;
    int min_cpu_freq;
    int cpu_policy;
    char cpu_mask;// which cpus supports, 1 support 0 not support eg 00000001 cpu7 support
};

/* HS07 code for HS07-899 by hujingcheng at 20250616 start */
struct hperf_process {
    int signal;
    int oom_score_adj;
    int priority;
    int pids[MAX_PIDS];
};
/* HS07 code for HS07-899 by hujingcheng at 20250616 end */

struct hperf_gpu_boost {
    int max_gpu_freq;
};

struct hperf_ddr_boost {
    int max_ddr_freq;
};

 /* HS07 code for HS07-899 by hujingcheng at 20250621 start*/
struct hperf_cpu_mask {
    pid_t pid;
    uint32_t cpu_mask; //0000 0000 ,eg if CPU0 support,set 1000 0000
};
 /* HS07 code for HS07-899 by hujingcheng at 20250621 start*/

/* HST11 code for AX7800A-1996 by wangletian at 20250613 start */
struct hperf_pinfo {
    char pname[MAX_PNAME_SIZE];
    int pids_cnt;
    int pids[MAX_PIDS];
};

int get_pids_by_pname(const char *pname, int *pids, int *pids_cnt);
/* HST11 code for AX7800A-1996 by wangletian at 20250613 end */

 /* HS07 code for HS07-899 by hujingcheng at 20250626 start*/
int send_signal_by_pid(int target_pid, int signal);
int process_signal_ctrl(struct hperf_process *hperf_sig_process);
struct task_struct *find_task_by_pid(pid_t pid);
void hmem_exit(void);
 int get_mm_cmdline(struct task_struct *task, char *buffer, int buflen);
int bind_process_core(struct hperf_cpu_mask *cpu_mask);
 /* HS07 code for HS07-899 by hujingcheng at 20250626 end*/
int  set_max_freq_init(void);

#endif // __HUAQIN_HPERF_H

