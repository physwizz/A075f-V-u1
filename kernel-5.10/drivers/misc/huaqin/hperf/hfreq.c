/**
 *  Change Log:
 *
 *   20250621    add CPU bind function for HS07-899    hujingcheng
*/
#include "hperf.h"
#include "log.h"
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/thermal.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/slab.h>
#define NUM_CPUS 8

void kernel_print_binary(uint32_t val) {
    char buf[33];
    int i;
    int bits = 32;

    for (i = 0; i < bits; i++) {
        buf[i] = (val >> (bits - 1 - i)) & 1 ? '1' : '0';
    }
    buf[bits] = '\0'; 

    printk(KERN_INFO "Binary (%d bits): %s\n", bits, buf);
}

int  set_max_freq_init(void)
{
    int i;
    unsigned int max_freq;
    struct cpufreq_policy *policy;
    for (i = 0; i < NUM_CPUS; i++) {
        policy = cpufreq_cpu_get(i);
        if (!policy) {
            Loge("Failed to get CPU %d policy\n", i);
            continue;
        }
        policy->policy = CPUFREQ_POLICY_PERFORMANCE;
        max_freq = policy->cpuinfo.max_freq;
        if (cpufreq_driver_target(policy, max_freq, CPUFREQ_RELATION_H)) {
            Loge("Failed to set max frequency for CPU %d\n", i);
        } else {
            Logi("Set CPU %d to max frequency: %u KHz\n", i, max_freq);
        }
        cpufreq_cpu_put(policy);
    }
    return 0;
}

int bind_process_core(struct hperf_cpu_mask *cpu_mask) {
    int i = 0;
    int ret = -1;
    uint32_t cpu_id;
    struct cpumask mask;
    struct task_struct *task;
    
    if(!cpu_mask->cpu_mask) {
        Loge("cpu_mask is 0");
        return ret;
    }

    task =  find_task_by_pid(cpu_mask->pid);
    if (!task) {
        Loge("PID %d Not Found!", cpu_mask->pid);
        return -ESRCH;
    }

    cpumask_clear(&mask);

    Logi("E pid %d",cpu_mask->pid);
    kernel_print_binary(cpu_mask->cpu_mask);
    for (i = num_possible_cpus(); i > 0; i--){
        bool cpu_enable =  cpu_mask->cpu_mask >> (i-1) & 0x01;
        cpu_id = num_possible_cpus()-i;
        if(cpu_enable) {
            cpumask_set_cpu(cpu_id, &mask);
            Logi("cpu_enable %d cpu_id %d",cpu_enable,cpu_id);
        }
        
    }
    
    ret = set_cpus_allowed_ptr(task, &mask);
    if (ret == 0) {
        Loge("Process %d  bound success\n", cpu_mask->pid);
    } else {
        Loge("Process %d bound failed\n", cpu_mask->pid);
    }

    put_task_struct(task);
    return ret;
}