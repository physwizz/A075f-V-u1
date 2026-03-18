#include <linux/module.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>

#include <sound/control.h>
#include <sound/soc.h>
#include <uapi/sound/asound.h>

#include "analog_pa.h"
#if IS_ENABLED(CONFIG_SND_SOC_AW87XXX)
#include "../aw87xxx.h"
#endif

#define PA_OFF_MODE 0

struct analog_pa_t g_analog_pa;

static DEFINE_SPINLOCK(s_pa_lock);

#if IS_ENABLED(CONFIG_SND_SOC_AW87XXX)
static char *aw_profile[] = {"Music", "Off"};/*aw87xxx_acf.bin*/
enum aw87xxx_dev_index {
    AW_DEV_0 = 0,
};
#endif

static factory_spk_choose_t s_spk_choose = DUAL_SPK;
int enable_analog_pa(bool enable)
{
    uint32_t mode = enable ? g_analog_pa.mode : PA_OFF_MODE;
    uint32_t compat_mode = PA_OFF_MODE;
    int spk1 = g_analog_pa.ctrl_spk1;
    int spk2 = g_analog_pa.ctrl_spk2;

    switch (g_analog_pa.type) {
        case GPIO:
            /*Tab A11 code for AX7800A-1982 by hujincan at 20250606 start*/
            if (!g_analog_pa.hac_enable) {
                if (g_analog_pa.compat) {
                    compat_mode = enable ? g_analog_pa.compat_mode : PA_OFF_MODE;
                    if (g_analog_pa.num == 1) {
                        set_pa_compat_mode(mode, compat_mode, spk1);
                    } else if (g_analog_pa.num == 2) {
                        if (s_spk_choose == DUAL_SPK) { // DUAL SPK
                            set_pa_compat_mode(mode, compat_mode, spk1);
                            set_pa_compat_mode(mode, compat_mode, spk2);
                        } else if (s_spk_choose == SPKL) { // SPKL-SPK2-UP
                            set_pa_compat_mode(mode, compat_mode, spk2);
                            set_pa_compat_mode(PA_OFF_MODE, PA_OFF_MODE, spk1);
                        } else if (s_spk_choose == SPKR) { // SPKR-SPK1-DOWN
                            set_pa_compat_mode(mode, compat_mode, spk1);
                            set_pa_compat_mode(PA_OFF_MODE, PA_OFF_MODE, spk2);
                        }
                    }
                } else {
                    if (g_analog_pa.num == 1) {
                        set_pa_mode(mode, spk1);
                    } else if (g_analog_pa.num == 2) {
                        if (s_spk_choose == DUAL_SPK) { // DUAL SPK
                            set_pa_mode(mode, spk1);
                            set_pa_mode(mode, spk2);
                        } else if (s_spk_choose == SPKL) { // SPKL-SPK2-UP
                            set_pa_mode(mode, spk2);
                            set_pa_mode(PA_OFF_MODE, spk1);
                        } else if (s_spk_choose == SPKR) { // SPKR-SPK1-DOWN
                            set_pa_mode(mode, spk1);
                            set_pa_mode(PA_OFF_MODE, spk2);
                        }
                    }
                }
            } else {
                set_pa_mode(mode, g_analog_pa.ctrl_hac);
            }
            /*Tab A11 code for AX7800A-1982 by hujincan at 20250606 end*/
            break;
        case I2C:
            switch (g_analog_pa.model) {
                case AW87394FCR:
                    if (enable) {
#if IS_ENABLED(CONFIG_SND_SOC_AW87XXX)
                        aw87xxx_set_profile(AW_DEV_0, aw_profile[0]); // MUSIC
#endif
                    } else {
#if IS_ENABLED(CONFIG_SND_SOC_AW87XXX)
                        aw87xxx_set_profile(AW_DEV_0, aw_profile[1]); // OFF
#endif
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return 0;
}
EXPORT_SYMBOL(enable_analog_pa);

int set_pa_mode(int mode, uint32_t mod_pin)
{
    const struct analog_pa_info_t *info = &g_analog_pa_info_table[g_analog_pa.model];
    unsigned long gpio_flag;
    int count;
    int ret = 0;

    pr_info("%s(),pa mode:%d", __func__, mode);
    if (mode > info->max_mode || mode < 0) {
        // invalid mode
        pr_err("%s(),invalid mode:%d", __func__, mode);
        return -1;
    }

    // switch mode online, need shut down pa firstly
    spin_lock_irqsave(&s_pa_lock, gpio_flag);
    gpio_set_value(mod_pin, 0);
    udelay(info->t_pwd);
    spin_unlock_irqrestore(&s_pa_lock, gpio_flag);

    if (mode == PA_OFF_MODE) {
        return 0;
    }

    // enable pa into work mode
    // make sure idle mode: gpio output low
    gpio_direction_output(mod_pin, 0);
    spin_lock_irqsave(&s_pa_lock, gpio_flag);

    // 1. send T-sta
    if (info->t_start > 0) {
        gpio_set_value(mod_pin, 1);
        udelay(info->t_start);
        gpio_set_value(mod_pin, 0);
        udelay(info->pulse_delay_us);
    }

    // 2. send mode
    if (info->model == FS1512GN) {
        count = (mode < 3) ? mode - 1 : mode + 1;
    } else {
        count = mode - 1;
    }
    while (count > 0) { // count of pulse
        gpio_set_value(mod_pin, 1);
        udelay(info->pulse_delay_us);
        gpio_set_value(mod_pin, 0);
        udelay(info->pulse_delay_us);
        count--;
    }

    // 3. pull up gpio and delay, enable pa
    gpio_set_value(mod_pin, 1);
    spin_unlock_irqrestore(&s_pa_lock, gpio_flag);
    udelay(info->t_work);

    return ret;
}

int set_pa_compat_mode(int fsm_mode, int aw_mode, uint32_t mod_pin)
{
    const struct analog_pa_info_t *info = &g_analog_pa_info_table[g_analog_pa.model];
    const struct analog_pa_info_t *compat_info = &g_analog_pa_info_table[g_analog_pa.compat_model];
    unsigned long gpio_flag;
    int count;
    int ret = 0;

    pr_info("%s(),fsm_mode:%d ,aw_mode:%d", __func__, fsm_mode, aw_mode);
    if (fsm_mode > info->max_mode || fsm_mode < 0 ||
        aw_mode > compat_info->max_mode || aw_mode < 0) {
        // invalid mode
        pr_err("%s(),invalid mode, fsm_mode:%d, aw_mode:%d", __func__, fsm_mode, aw_mode);
        return -EINVAL;
    }

    // switch mode online, need shut down pa firstly
    spin_lock_irqsave(&s_pa_lock, gpio_flag);
    gpio_set_value(mod_pin, 0);
    udelay(info->t_pwd);
    spin_unlock_irqrestore(&s_pa_lock, gpio_flag);

    if ((fsm_mode == 0) && (aw_mode == 0)) {
        return 0;
    }

    // enable pa into work mode
    // make sure idle mode: gpio output low
    gpio_direction_output(mod_pin, 0);
    spin_lock_irqsave(&s_pa_lock, gpio_flag);

    // awinic pa sequential logic
    gpio_set_value(mod_pin, 1);
    count = aw_mode - 1;
    while (count > 0) {
        udelay(compat_info->pulse_delay_us);
        gpio_set_value(mod_pin, 0);
        udelay(compat_info->pulse_delay_us);
        gpio_set_value(mod_pin, 1);
        count--;
    }

    // 1. send T-sta
    gpio_set_value(mod_pin, 1);
    udelay(info->t_start);
    gpio_set_value(mod_pin, 0);
    udelay(info->pulse_delay_us);

    // 2. send mode
    count = fsm_mode - 1;
    while (count > 0) { // count of pulse
        gpio_set_value(mod_pin, 1);
        udelay(info->pulse_delay_us);
        gpio_set_value(mod_pin, 0);
        udelay(info->pulse_delay_us);
        count--;
    }

    // 3. pull up gpio and delay, enable pa
    gpio_set_value(mod_pin, 1);
    spin_unlock_irqrestore(&s_pa_lock, gpio_flag);
    udelay(info->t_work);

    return ret;
}

static const char *const s_spk_mode_str[] = { "DUAL SPK", "SPKL", "SPKR" };
static const char *const s_audio_switch_str[] = { "SPK", "RCV" };
static const char *const s_hac_mode_str[] = { "OFF", "ON" };

static const struct soc_enum s_analog_pa_operation_enum[] = {
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(s_spk_mode_str),
                s_spk_mode_str),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(s_audio_switch_str),
                s_audio_switch_str),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(s_hac_mode_str),
                s_hac_mode_str),
};

static int spk_mode_get(struct snd_kcontrol *kcontrol,
          struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.integer.value[0] = s_spk_choose;
    return 0;
}
/*Tab A11 code for P250623-05904 by huangzhixian at 20250625 start*/
static int spk_mode_set(struct snd_kcontrol *kcontrol,
          struct snd_ctl_elem_value *ucontrol)
{
    if (ucontrol->value.integer.value[0] < 0 ||
        ucontrol->value.integer.value[0] > 3) {
        pr_err("%s() ucontrol->value.integer.value[0] out of range\n", __func__);
        return -EINVAL;
    }

    s_spk_choose = ucontrol->value.integer.value[0];
    pr_info("%s() spk mode = %s\n ", __func__,
        s_spk_mode_str[s_spk_choose]);

    return 0;
}
/*Tab A11 code for P250623-05904 by huangzhixian at 20250625 end*/
/*Tab A11 code for P250623-05904 by huangzhixian at 20250625 start*/
static int spk_mode_immediately_set(struct snd_kcontrol *kcontrol,
          struct snd_ctl_elem_value *ucontrol)
{
    uint32_t mode = g_analog_pa.mode;
    uint32_t compat_mode = g_analog_pa.compat_mode;
    int spk1 = g_analog_pa.ctrl_spk1;
    int spk2 = g_analog_pa.ctrl_spk2;

    if (ucontrol->value.integer.value[0] < 0 ||
        ucontrol->value.integer.value[0] > 3) {
        pr_err("%s() ucontrol->value.integer.value[0] out of range\n", __func__);
        return -EINVAL;
    }

    s_spk_choose = ucontrol->value.integer.value[0];

    pr_info("%s() spk mode = %s\n ", __func__,
        s_spk_mode_str[s_spk_choose]);
    /* Open the PA and set the mode */
    if (g_analog_pa.compat) {
        if (s_spk_choose == DUAL_SPK) { // DUAL SPK
            set_pa_compat_mode(mode, compat_mode, spk1);
            set_pa_compat_mode(mode, compat_mode, spk2);
        } else if (s_spk_choose == SPKL) { // SPKL-SPK2-UP
            set_pa_compat_mode(mode, compat_mode, spk2);
            set_pa_compat_mode(PA_OFF_MODE, PA_OFF_MODE, spk1);
        } else if (s_spk_choose == SPKR) { // SPKR-SPK1-DOWN
            set_pa_compat_mode(mode, compat_mode, spk1);
            set_pa_compat_mode(PA_OFF_MODE, PA_OFF_MODE, spk2);
        }
    } else {
        if (s_spk_choose == DUAL_SPK) { // DUAL SPK
            set_pa_mode(mode, spk1);
            set_pa_mode(mode, spk2);
        } else if (s_spk_choose == SPKL) { // SPKL-SPK2-UP
            set_pa_mode(mode, spk2);
            set_pa_mode(PA_OFF_MODE, spk1);
        } else if (s_spk_choose == SPKR) { // SPKR-SPK1-DOWN
            set_pa_mode(mode, spk1);
            set_pa_mode(PA_OFF_MODE, spk2);
        }
    }
    return 0;
}
/*Tab A11 code for P250623-05904 by huangzhixian at 20250625 end*/
static int s_audio_switch_value = SPK; //Default SPK
static int rcv_spk_audio_switch_get(struct snd_kcontrol *kcontrol,
                    struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.integer.value[0] = s_audio_switch_value;
    return 0;
}
/*Tab A11 code for P250623-05904 by huangzhixian at 20250625 start*/
static int rcv_spk_audio_switch_set(struct snd_kcontrol *kcontrol,
                    struct snd_ctl_elem_value *ucontrol)
{
    if (ucontrol->value.integer.value[0] < 0 ||
        ucontrol->value.integer.value[0] > 2) {
        pr_err("%s() ucontrol->value.integer.value[0] out of range\n", __func__);
        return -EINVAL;
    }
    s_audio_switch_value = ucontrol->value.integer.value[0];
    pr_info("%s() audio switch = %s\n ", __func__,
        s_audio_switch_str[s_audio_switch_value]);
    if (s_audio_switch_value == SPK) {
        gpio_set_value(g_analog_pa.ctrl_switch, 0);
    } else if (s_audio_switch_value == RCV) {
        gpio_set_value(g_analog_pa.ctrl_switch, 1);
    }
    return 0;
}
/*Tab A11 code for P250623-05904 by huangzhixian at 20250625 end*/
static int s_hac_mode_value = OFF; //Default OFF
static int hac_mode_get(struct snd_kcontrol *kcontrol,
                    struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.integer.value[0] = s_hac_mode_value;
    return 0;
}
/*Tab A11 code for P250623-05904 by huangzhixian at 20250625 start*/
/*Tab A11 code for AX7800A-1982 by hujincan at 20250606 start*/
static int hac_mode_set(struct snd_kcontrol *kcontrol,
                    struct snd_ctl_elem_value *ucontrol){
    if (ucontrol->value.integer.value[0] < 0 ||
        ucontrol->value.integer.value[0] > 2) {
        pr_err("%s() ucontrol->value.integer.value[0] out of range\n", __func__);
        return -EINVAL;
    }
    s_hac_mode_value = ucontrol->value.integer.value[0];
    pr_info("%s() hac mode = %s\n ", __func__,
        s_hac_mode_str[s_audio_switch_value]);
    if (s_hac_mode_value == OFF){
        g_analog_pa.hac_enable = false;
    } else if (s_hac_mode_value == ON) {
        g_analog_pa.hac_enable = true;
    }
    return 0;
}
/*Tab A11 code for AX7800A-1982 by hujincan at 20250606 end*/
/*Tab A11 code for P250623-05904 by huangzhixian at 20250625 end*/

static const struct snd_kcontrol_new spk_mode_controls[] = {
    SOC_ENUM_EXT("SPK_MODE_Switch", s_analog_pa_operation_enum[0],
             spk_mode_get, spk_mode_set),
    SOC_ENUM_EXT("SPK_MODE_IMMEDIATELY_Switch", s_analog_pa_operation_enum[0],
             NULL, spk_mode_immediately_set),
};
static const struct snd_kcontrol_new audio_switch_controls[] = {
    SOC_ENUM_EXT("RCV_SPK_Audio_Switch", s_analog_pa_operation_enum[1],
             rcv_spk_audio_switch_get, rcv_spk_audio_switch_set),
};
static const struct snd_kcontrol_new hac_mode_controls[] = {
    SOC_ENUM_EXT("HAC_MODE_Switch", s_analog_pa_operation_enum[2],
             hac_mode_get, hac_mode_set),
};

int add_analog_pa_controls(struct snd_soc_component *cmpnt)
{
    int ret = 0;
    if (g_analog_pa.num == 2) {
        ret = snd_soc_add_component_controls(cmpnt,
                        spk_mode_controls,
                        ARRAY_SIZE(spk_mode_controls));
    }
    if (gpio_is_valid(g_analog_pa.ctrl_switch)) {
        ret = snd_soc_add_component_controls(cmpnt,
                        audio_switch_controls,
                        ARRAY_SIZE(audio_switch_controls));
    }
    if (gpio_is_valid(g_analog_pa.ctrl_hac)) {
        ret = snd_soc_add_component_controls(cmpnt,
                        hac_mode_controls,
                        ARRAY_SIZE(hac_mode_controls));
    }
#if IS_ENABLED(CONFIG_SND_SOC_AW87XXX)
	ret = aw87xxx_add_codec_controls((void *)cmpnt);
	if (ret < 0) {
		pr_err("%s: add_codec_controls failed, err %d\n" , __func__ , ret);
	}
#endif
    return ret;
}
EXPORT_SYMBOL(add_analog_pa_controls);

void prase_pa_model(uint32_t model)
{
    uint8_t i = 0;
    if (model & (model - 1)) {
        g_analog_pa.compat = true;
    } else {
        g_analog_pa.compat = false;
    }

    for (i = 0; i < PA_MODEL_MAX; i++) {
        if (model % 2 == 1) {
            if (g_analog_pa.compat) {
                if (model == 1) {
                    g_analog_pa.compat_model = i;
                    break;
                } else {
                    g_analog_pa.model = i;
                }
            } else {
                g_analog_pa.model = i;
                break;
            }
        }
        model /= 2;
    }
}

static int analog_pa_parse_dt(struct device *dev)
{
    int ret = 0;
    struct device_node *np = NULL;
    uint32_t pa_model = 0;

    if (!dev) {
        return -ENODEV;
    }
    np = dev->of_node;

    ret = of_property_read_u32(np, "pa-model", &pa_model);
    if (ret < 0 || pa_model <= 0) {
        pr_err("%s(), get pa-model fail!\n", __func__);
        return ret;
    }
    prase_pa_model(pa_model);

    ret = of_property_read_u32(np, "pa-control-type", &g_analog_pa.type);
    if (ret < 0) {
        pr_err("%s(), get pa-control-type fail!\n", __func__);
        return ret;
    }

    ret = of_property_read_u32(np, "pa-mum", &g_analog_pa.num);
    if (ret < 0) {
        pr_err("%s(), get pa-mum fail!\n", __func__);
        return ret;
    }

    if (g_analog_pa.type == GPIO) {
        ret = of_property_read_u32(np, "pa-mode", &g_analog_pa.mode);
        if (ret < 0) {
            pr_err("%s(), get pa-mode fail!\n", __func__);
            return ret;
        }

        if (g_analog_pa.compat) {
            ret = of_property_read_u32(np, "pa-compat-mode", &g_analog_pa.compat_mode);
            if (ret < 0) {
                pr_err("%s(), get pa-compat-mode fail!\n", __func__);
                return ret;
            }
        }

        g_analog_pa.ctrl_spk1 = of_get_named_gpio(np, "ctrl-mod-r-down", 0);
        if (g_analog_pa.ctrl_spk1 < 0) {
            pr_err("%s(), get ctrl-mod-r-down fail!\n", __func__);
        } else if (gpio_is_valid(g_analog_pa.ctrl_spk1)) {
            if (gpio_request(g_analog_pa.ctrl_spk1, "ctrl-mod-r-down") < 0) {
                pr_err("%s(), gpio_request ctrl-mod-r-down fail\n", __func__);
            }
        } else {
            pr_err("%s(), ctrl-mod-r-down is Invalid GPIO!\n", __func__);
        }

        if (g_analog_pa.num == 2) {
            g_analog_pa.ctrl_spk2 = of_get_named_gpio(np, "ctrl-mod-l-up", 0);
            if (g_analog_pa.ctrl_spk2 < 0) {
                pr_err("%s(), get ctrl-mod-l-up fail!\n", __func__);
            } else if (gpio_is_valid(g_analog_pa.ctrl_spk2)) {
                if (gpio_request(g_analog_pa.ctrl_spk2, "ctrl-mod-l-up") < 0) {
                    pr_err("%s(), gpio_request ctrl-mod-l-up fail\n", __func__);
                }
            } else {
                pr_err("%s(), ctrl-mod-l-up is Invalid GPIO!\n", __func__);
            }
        }
    }

    g_analog_pa.ctrl_switch = of_get_named_gpio(np, "audio-switch", 0);
    if (g_analog_pa.ctrl_switch < 0) {
        pr_err("%s(), get audio-switch fail!\n", __func__);
    } else if (gpio_is_valid(g_analog_pa.ctrl_switch)) {
        if (gpio_request(g_analog_pa.ctrl_switch, "audio-switch") < 0) {
            pr_err("%s(), gpio_request audio-switch fail\n", __func__);
        }
    } else {
        pr_err("%s(), audio-switch is Invalid GPIO!\n", __func__);
    }

    g_analog_pa.ctrl_hac = of_get_named_gpio(np, "ctrl-mod-hac", 0);
    if (g_analog_pa.ctrl_hac < 0) {
        pr_err("%s(), get ctrl-mod-hac fail!\n", __func__);
    } else if (gpio_is_valid(g_analog_pa.ctrl_hac)) {
        if (gpio_request(g_analog_pa.ctrl_hac, "ctrl-mod-hac") < 0) {
            pr_err("%s(), gpio_request ctrl-mod-hac fail\n", __func__);
        }
    } else {
        pr_err("%s(), ctrl-mod-hac is Invalid GPIO!\n", __func__);
    }

    return ret;
}

void analog_pa_init(void)
{
    g_analog_pa.model = FS1512N;
    g_analog_pa.compat_model = FS1512N;
    g_analog_pa.type = GPIO;
    g_analog_pa.compat = false;
    g_analog_pa.num = 1;
    g_analog_pa.mode = 0;
    g_analog_pa.compat_mode = 0;
    g_analog_pa.ctrl_spk1 = 0;
    g_analog_pa.ctrl_spk2 = 0;
    g_analog_pa.ctrl_hac = 0;
    g_analog_pa.ctrl_switch = 0;
    /*Tab A11 code for AX7800A-1982 by hujincan at 20250606 start*/
    g_analog_pa.hac_enable = false;
    /*Tab A11 code for AX7800A-1982 by hujincan at 20250606 end*/
}

static int analog_pa_probe(struct platform_device *pdev)
{
    int ret = 0;

    pr_info("%s enter\n", __func__);
    analog_pa_init();

    if (pdev->dev.of_node) {
        ret = analog_pa_parse_dt(&pdev->dev);
        if (ret < 0) {
            pr_err("analog_pa_parse_dt fail...\n");
            return -EINVAL;
        }
    }
    pr_info("%s(), model:%d, compat_model:%d, type:%d, compat:%d, num:%d, mode:%d, compat_mode:%d\n", __func__,
        g_analog_pa.model, g_analog_pa.compat_model, g_analog_pa.type, g_analog_pa.compat,
        g_analog_pa.num, g_analog_pa.mode, g_analog_pa.compat_mode);
    return ret;
}

static int analog_pa_remove(struct platform_device *pdev)
{
    pr_info("analog pa driver remove...\n");

    return 0;
}

static const struct of_device_id analog_pa_dt_match[] = {
    {.compatible = "odm,analog-pa"},
    {},
};
MODULE_DEVICE_TABLE(of, analog_pa_dt_match);

static struct platform_driver analog_pa_driver = {
    .driver = {
        .name = "analog-pa",
        .of_match_table = analog_pa_dt_match,
    },
    .probe = analog_pa_probe,
    .remove = analog_pa_remove,
};

module_platform_driver(analog_pa_driver);

/* Module information */
MODULE_DESCRIPTION("Analog pa driver");
MODULE_LICENSE("GPL v2");