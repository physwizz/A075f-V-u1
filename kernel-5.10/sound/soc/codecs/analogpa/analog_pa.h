#ifndef __ANALOG_PA_H__
#define __ANALOG_PA_H__
#include <linux/types.h>

typedef enum {
    GPIO,
    I2C,
} pa_control_type_t;

enum {
    MODE1,
    MODE2,
    MODE_MAX,
};

typedef enum {
    DUAL_SPK,
    SPKL,
    SPKR,
} factory_spk_choose_t;

enum {
    SPK,
    RCV,
};

enum {
    OFF,
    ON,
};

typedef enum {
    FS1512N,
    FS1512GN,
    FS1506N,
    FS1507TN,
    AW87318AFCR,
    AW87394FCR,
    PA_MODEL_MAX,
} pa_model_t;

/**
 * This struct defines the time sequence information of an PA model.
 *
 * @model：PA model, such as: FS1512N, FS1512GN...
 * @t_start ：The duration required before the start of the time sequence.
 * @pulse_delay_us ：The duration of the pulse.
 * @t_work ：The duration required before starting PA work.
 * @t_pwd ：The duration required before turning off the PA.
 * @max_mode ：The max mode supported by the PA.
 */
struct analog_pa_info_t {
    const pa_model_t model;
    const int t_start;
    const int pulse_delay_us;
    const int t_work;
    const int t_pwd;
    const int max_mode;
};

const struct analog_pa_info_t g_analog_pa_info_table[] = {
    { FS1512N,     500, 10, 300, 5000, 10 },
    { FS1512GN,    220, 10, 820, 5000, 12 },
    { FS1506N,     500, 10, 820, 5000, 12 },
    { FS1507TN,    0,   10, 820, 5000, 14 },
    { AW87318AFCR, 0,   2,  820, 5000, 10 },
    { AW87394FCR,  0,   0,  0,   0,     0 },
};

/**
 * This struct defines the analog PA information of a project (obtained from the device tree).
 *
 * @model：PA model, such as: FS1512N, FS1512GN...
 * @compat_model：PA model that need to be compatible (If necessary).
 * @type ：The way to control the PA, such as GPIO or I2C.
 * @compat ：This project need to compatible PA or not.
 * @num ：The number of PA (SPK) for this project.
 * @mode ：The modes that need to be set for the PA of this project.
 * @compat_mode ：The modes that need to be set for the compatible PA of this project (If necessary).
 * @ctrl_spk1 ：The GPIO of spk1 PA.
 * @ctrl_spk2 ：The GPIO of spk2 PA.
 * @ctrl_hac ：The GPIO of HAC PA.
 * @ctrl_switch ：The GPIO of audio switch.
 * @hac_enable ：The flag of hac enable.
 */
struct analog_pa_t {
    pa_model_t model;
    pa_model_t compat_model;
    pa_control_type_t type;
    bool compat;
    uint32_t num;
    uint32_t mode;
    uint32_t compat_mode;
    int ctrl_spk1; // SPKR-SPK1-DOWN
    int ctrl_spk2; // SPKL-SPK2-UP
    int ctrl_hac;
    int ctrl_switch;
    /*Tab A11 code for AX7800A-1982 by hujincan at 20250606 start*/
    bool hac_enable;
    /*Tab A11 code for AX7800A-1982 by hujincan at 20250606 end*/
};

int set_pa_mode(int mode, uint32_t mod_pin);
int set_pa_compat_mode(int mode1, int mode2, uint32_t mod_pin);
extern int enable_analog_pa(bool enable);
extern int add_analog_pa_controls(struct snd_soc_component *cmpnt);

#endif //__ANALOG_PA_H__
