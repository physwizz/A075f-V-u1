#include "panel-bias.h"

static u8 gs_bias_reg = BIAS_IC_DETECT_REG;
static struct mutex gs_bias_read_lock;

static struct proc_dir_entry *gs_bias_info_proc_entry = NULL;
static bool gs_smart_wakeup_flag = false;

bool gs_smart_wakeup_flag_get(void)
{
    return gs_smart_wakeup_flag;
}
EXPORT_SYMBOL(gs_smart_wakeup_flag_get);

void gs_smart_wakeup_flag_set(bool flag)
{
    gs_smart_wakeup_flag = flag;
}
EXPORT_SYMBOL(gs_smart_wakeup_flag_set);

#if defined(BIAS_IC_NAME_DETECT_FUNC)
/**
 * R03=0x13, R04=0x00 -----LP62701HVF
 * R03=0x5b, R04=0x00 -----SM5109CF
 * R03=0x33, R04=0x0d -----OCP2131BWPAD
 */
#define BIAS_IC_LP62701HVF_R03   0x13
#define BIAS_IC_SM5109CF_R03     0x5b
#define BIAS_IC_OCP2131BWPAD_R03 0x33
static int bias_ic_detect(unsigned char bias_buf, char *name_buf, size_t buf_size)
{
    u8 value = (u8)bias_buf;

    const char *ic_name = "unknown";

    switch (value) {
        case BIAS_IC_LP62701HVF_R03:
            ic_name = "LP62701";
            break;
        case BIAS_IC_SM5109CF_R03:
            ic_name = "SM5109C";
            break;
        case BIAS_IC_OCP2131BWPAD_R03:
            ic_name = "OCP2131";
            break;
        default:
            ic_name = "unknown";
    }
    return snprintf(name_buf, buf_size, "%s", ic_name);
}

static ssize_t bias_info_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[BIAS_INFO_BUF_SIZE] = {0};
    char ic_name[BIAS_IC_NAME_LEN] = {0};
    unsigned char bias_buf = 0;
    int len = 0;

    if (gs_bias_info_proc_entry == NULL) {
        pr_err("%s():gs_bias_info_proc_entry is null\n",__func__);
        return -EFAULT;
    }

    mutex_lock(&gs_bias_read_lock);
    panel_bias_i2c_read_bytes(gs_bias_reg, &bias_buf);
    mutex_unlock(&gs_bias_read_lock);

    if (gs_bias_reg == BIAS_IC_DETECT_REG) {
        bias_ic_detect(bias_buf, ic_name, sizeof(ic_name));
    } else {
        snprintf(ic_name, sizeof(ic_name), "unknown");
    }

    len = scnprintf(buf, sizeof(buf), "INFO:0x%02x=0x%02x,IC=%s\n",gs_bias_reg, bias_buf, ic_name);
    if (len >= sizeof(buf) - 1) {
        pr_err("%s(): output truncated\n",__func__);
    }

    return simple_read_from_buffer(ubuf,count,ppos,buf,len);
}
#else//BIAS_IC_NAME_DETECT_FUNC
static ssize_t bias_info_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[BIAS_INFO_BUF_SIZE] = {0};
    unsigned char bias_buf = 0;
    int len = 0;

    if (gs_bias_info_proc_entry == NULL) {
        pr_err("%s():gs_bias_info_proc_entry is null\n",__func__);
        return -EFAULT;
    }

    mutex_lock(&gs_bias_read_lock);
    panel_bias_i2c_read_bytes(gs_bias_reg, &bias_buf);
    mutex_unlock(&gs_bias_read_lock);

    len = scnprintf(buf, sizeof(buf), "INFO:0x%02x=0x%02x\n",gs_bias_reg, bias_buf);
    if (len >= sizeof(buf) - 1) {
        pr_err("%s(): output truncated\n",__func__);
    }

    return simple_read_from_buffer(ubuf,count,ppos,buf,len);
}
#endif//BIAS_IC_NAME_DETECT_FUNC
static ssize_t bias_info_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[BIAS_INFO_WRITE_MAX_SIZE] = {0};
    ssize_t ret = 0;
    u8 reg = 0;

    if (count > sizeof(buf) - 1) {
        pr_err("%s too much data, length = %ld\n", __func__, count);
        return -EINVAL;
    }

    ret = simple_write_to_buffer(buf, sizeof(buf) - 1, ppos, ubuf, count);
    if (ret < 0) {
        return ret;
    }

    ret = kstrtou8(buf, 16, &reg);
    if (ret) {
        pr_err("%s invalid format:%s\n",__func__, buf);
        return -EINVAL;
    }
    gs_bias_reg = reg;

    return count;
}

static const struct proc_ops bias_info_proc_fops = {
    .proc_read = bias_info_read,
    .proc_write = bias_info_write,
};

static int32_t bias_proc_init(void)
{
    gs_bias_info_proc_entry = proc_create(BIAS_INFO_PROC_FILE, 0644, NULL, &bias_info_proc_fops);
    if (NULL == gs_bias_info_proc_entry) {
        pr_err("%s():Couldn't create proc entry!\n",__func__);
        return -EFAULT;
    } else {
        pr_info("%s():Create proc entry success\n",__func__);
    }
    return 0;
}

#define LCM_I2C_ID_NAME "I2C_LCD_BIAS"
static struct i2c_client *panel_bias_i2c_client;

int panel_bias_i2c_read_bytes(unsigned char addr, unsigned char *returnData)
{
    char cmd_buf[2] = { 0x00, 0x00 };
    int ret = 0;
    struct i2c_client *client = panel_bias_i2c_client;

    if (client == NULL) {
        pr_err("%s: ERROR!! panel_bias_i2c_client is null\n", __func__);
        return 0;
    }

    cmd_buf[0] = addr;
    ret = i2c_master_send(client, &cmd_buf[0], 1);
    ret = i2c_master_recv(client, &cmd_buf[1], 1);
    if (ret < 0) {
        pr_err("%s: ERROR read 0x%x fail %d\n", __func__, addr, ret);
    }

    *returnData = cmd_buf[1];

    return ret;
}
EXPORT_SYMBOL(panel_bias_i2c_read_bytes);

int panel_bias_i2c_write_multiple_bytes(unsigned char addr, unsigned char *value,
    unsigned int size)
{
    int ret = 0, i = 0;
    struct i2c_client *client = panel_bias_i2c_client;
    char *write_data = NULL;

    if (client == NULL) {
        pr_err("%s: ERROR!! panel_bias_i2c_client is null\n", __func__);
        return 0;
    }

    if (IS_ERR_OR_NULL(value) || size == 0) {
        pr_err("%s: ERROR!! value is null, size:%u\n", __func__, size);
        return -EINVAL;
    }
    write_data = kzalloc(roundup(size + 1, 4), GFP_KERNEL);
    if (IS_ERR_OR_NULL(write_data)) {
        pr_err("%s: ERROR!! allocate buffer, size:%u\n", __func__, size);
        return -ENOMEM;
    }

    write_data[0] = addr;
    for (i = 0; i < size; i++) {
        write_data[i + 1] = value[i];
    }

    ret = i2c_master_send(client, write_data, size + 1);
    if (ret < 0) {
        pr_err("%s: ERROR i2c write 0x%x fail %d\n", __func__, ret, addr);
    }

    kfree(write_data);
    write_data = NULL;
    return ret;
}
EXPORT_SYMBOL(panel_bias_i2c_write_multiple_bytes);

int panel_bias_i2c_write_bytes(unsigned char addr, unsigned char value)
{
    int ret = 0;
    struct i2c_client *client = panel_bias_i2c_client;
    char write_data[2] = { 0 };

    if (client == NULL) {
        pr_err("ERROR!! panel_bias_i2c_client is null\n");
        return 0;
    }

    write_data[0] = addr;
    write_data[1] = value;
    ret = i2c_master_send(client, write_data, 2);
    if (ret < 0) {
        pr_err("[ERROR] panel_bias_i2c write data fail !!\n");
    }

    return ret;
}
EXPORT_SYMBOL(panel_bias_i2c_write_bytes);

#if defined(BIAS_IC_NAME_DETECT_FUNC)
static void panel_bias_i2c_init_detect(void)
{
    char buf[BIAS_INFO_BUF_SIZE] = {0};
    char ic_name[BIAS_IC_NAME_LEN] = {0};
    unsigned char bias_buf = 0;
    int len = 0;

    mutex_lock(&gs_bias_read_lock);
    panel_bias_i2c_read_bytes(gs_bias_reg, &bias_buf);
    mutex_unlock(&gs_bias_read_lock);

    if (gs_bias_reg == BIAS_IC_DETECT_REG) {
        bias_ic_detect(bias_buf, ic_name, sizeof(ic_name));
    } else {
        snprintf(ic_name, sizeof(ic_name), "unknown");
    }

    len = scnprintf(buf, sizeof(buf), "INFO:0x%02x=0x%02x,IC=%s\n",gs_bias_reg, bias_buf, ic_name);
    if (len >= sizeof(buf) - 1) {
        pr_err("%s(): output truncated\n",__func__);
    }

    pr_info("%s: %s",__func__, buf);
}
#endif//BIAS_IC_NAME_DETECT_FUNC

static int panel_bias_i2c_probe(struct i2c_client *client,
              const struct i2c_device_id *id)
{
    pr_info("[LCM][I2C] bias %s\n", __func__);
    pr_info("[LCM][I2C] bias:name=%s addr=0x%x\n", client->name, client->addr);
    panel_bias_i2c_client = client;
    mutex_init(&gs_bias_read_lock);
    bias_proc_init();
    #if defined(BIAS_IC_NAME_DETECT_FUNC)
    panel_bias_i2c_init_detect();
    #endif//BIAS_IC_NAME_DETECT_FUNC
    return 0;
}

static int panel_bias_i2c_remove(struct i2c_client *client)
{
    pr_info("[LCM][I2C] bias %s\n", __func__);
    if (gs_bias_info_proc_entry) {
        proc_remove(gs_bias_info_proc_entry);
    }
    panel_bias_i2c_client = NULL;
    i2c_unregister_device(client);
    return 0;
}

static const struct of_device_id panel_bias_i2c_of_match[] = {
    {
        .compatible = "mediatek,i2c_lcd_bias",
    },
    {}
};
MODULE_DEVICE_TABLE(of, panel_bias_i2c_of_match);

static const struct i2c_device_id panel_bias_i2c_id[] = {
    { LCM_I2C_ID_NAME, 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, panel_bias_i2c_id);

static struct i2c_driver panel_bias_i2c_driver = {
    .id_table = panel_bias_i2c_id,
    .probe = panel_bias_i2c_probe,
    .remove = panel_bias_i2c_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = LCM_I2C_ID_NAME,
        .of_match_table = panel_bias_i2c_of_match,
    },
};

module_i2c_driver(panel_bias_i2c_driver);

MODULE_AUTHOR("bias");
MODULE_DESCRIPTION("i2c driver");
MODULE_LICENSE("GPL v2");
