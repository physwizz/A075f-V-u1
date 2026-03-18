#ifndef __HUAQIN_HPERF_LOG_H
#define __HUAQIN_HPERF_LOG_H

#include <linux/kernel.h>

#define LOG_TAG "hperf"

#define __LOG(level, fmt, ...) \
    pr_##level(                \
        "[%s:%s:%d] " fmt,     \
        LOG_TAG,               \
        __func__,              \
        __LINE__,              \
        ##__VA_ARGS__          \
    )

#define Logd(fmt, ...) __LOG(debug, fmt, ##__VA_ARGS__)
#define Logi(fmt, ...) __LOG(info, fmt, ##__VA_ARGS__)
#define Logw(fmt, ...) __LOG(warning, fmt, ##__VA_ARGS__)
#define Loge(fmt, ...) __LOG(err, fmt, ##__VA_ARGS__)

#endif // __HUAQIN_HPERF_LOG_H