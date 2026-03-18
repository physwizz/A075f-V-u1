
#ifndef __SP2131_H__
#define __SP2131_H__

//#define pr_fmt(fmt)	"[SP2131] %s: " fmt, __func__

enum {
	SP2131_STANDALONG = 0,
	SP2131_MASTER,
	SP2131_SLAVE,
};

static const char* sp2131_psy_name[] = {
	[SP2131_STANDALONG] = "sp2131-cp-standalone",
	[SP2131_MASTER] = "sp2131-cp-master",
	[SP2131_SLAVE] = "sp2131-cp-slave",
};

static const char* sp2131_irq_name[] = {
	[SP2131_STANDALONG] = "sp2131-standalone-irq",
	[SP2131_MASTER] = "sp2131-master-irq",
	[SP2131_SLAVE] = "sp2131-slave-irq",
};

static int sp2131_mode_data[] = {
	[SP2131_STANDALONG] = SP2131_STANDALONG,
	[SP2131_MASTER] = SP2131_MASTER,
	[SP2131_SLAVE] = SP2131_SLAVE,
};

typedef enum {
	ADC_IBUS,
	ADC_VBUS,
	ADC_VAC,
	ADC_VOUT,
	ADC_VBAT,
	ADC_IBAT,
	ADC_TDIE,
	ADC_TS,
	ADC_MAX_NUM,
}ADC_CH;

#define SP2131_ROLE_STDALONE   	        0
#define SP2131_ROLE_MASTER		1
#define SP2131_ROLE_SLAVE		2

//reg0f
#define VOUT_OCP_FAULT		BIT(7)
#define BAT_OVP_FAULT		BIT(6)
#define BAT_OCP_FAULT		BIT(5)
#define BUS_OVP_FAULT		BIT(4)
#define BUS_OCP_FAULT		BIT(3)
#define BUS_UCP_FAULT		BIT(2)

//reg10
#define	TS_DIE_FAULT		BIT(7)
#define TS_FAULT		BIT(6)
#define	VBUS_ERROR_H		(3)
#define	VBUS_ERROR_L		(2)

/*below used for comm with other module*/
#define	VOUT_OVP_FAULT_SHIFT			7
#define	BAT_OVP_FAULT_SHIFT			6
#define	BAT_OCP_FAULT_SHIFT			5
#define	BUS_OVP_FAULT_SHIFT			4
#define	BUS_OCP_FAULT_SHIFT			3
#define	VOUT_OVP_FAULT_MASK			(1 << VOUT_OVP_FAULT_SHIFT)
#define	BAT_OVP_FAULT_MASK			(1 << BAT_OVP_FAULT_SHIFT)
#define	BAT_OCP_FAULT_MASK			(1 << BAT_OCP_FAULT_SHIFT)
#define	BUS_OVP_FAULT_MASK			(1 << BUS_OVP_FAULT_SHIFT)
#define	BUS_OCP_FAULT_MASK			(1 << BUS_OCP_FAULT_SHIFT)

#define	TS_FAULT_SHIFT                  6
#define VBUS_ERROR_H_SHIFT              3
#define VBUS_ERROR_L_SHIFT              2
#define TS_FAULT_MASK		        (1 << TS_FAULT_SHIFT)
#define VBUS_ERROR_H_MASK       (1 << VBUS_ERROR_H_SHIFT)
#define VBUS_ERROR_L_MASK       (1 << VBUS_ERROR_L_SHIFT)

#define ADC_REG_BASE SP2131_REG_1B

#define nu_err(fmt, ...)								\
do {											\
	if (chip->mode == SP2131_ROLE_MASTER)						\
		printk(KERN_ERR "[sp2131-MASTER]:%s:" fmt, __func__, ##__VA_ARGS__);	\
	else if (chip->mode == SP2131_ROLE_SLAVE)					\
		printk(KERN_ERR "[sp2131-SLAVE]:%s:" fmt, __func__, ##__VA_ARGS__);	\
	else										\
		printk(KERN_ERR "[sp2131-STANDALONE]:%s:" fmt, __func__, ##__VA_ARGS__);\
} while(0);

#define nu_info(fmt, ...)								\
do {											\
	if (chip->mode == SP2131_ROLE_MASTER)						\
		printk(KERN_INFO "[sp2131-MASTER]:%s:" fmt, __func__, ##__VA_ARGS__);	\
	else if (chip->mode == SP2131_ROLE_SLAVE)					\
		printk(KERN_INFO "[sp2131-SLAVE]:%s:" fmt, __func__, ##__VA_ARGS__);	\
	else										\
		printk(KERN_INFO "[sp2131-STANDALONE]:%s:" fmt, __func__, ##__VA_ARGS__);\
} while(0);

#define nu_dbg(fmt, ...)								\
do {											\
	if (chip->mode == SP2131_ROLE_MASTER)						\
		printk(KERN_DEBUG "[sp2131-MASTER]:%s:" fmt, __func__, ##__VA_ARGS__);	\
	else if (chip->mode == SP2131_ROLE_SLAVE)					\
		printk(KERN_DEBUG "[sp2131-SLAVE]:%s:" fmt, __func__, ##__VA_ARGS__);	\
	else										\
		printk(KERN_DEBUG "[sp2131-STANDALONE]:%s:" fmt, __func__, ##__VA_ARGS__);\
} while(0);


struct sp2131_cfg {
	int bat_ovp_disable;
	int bat_ocp_disable;

	int bat_ovp_th;
	int bat_ocp_th;

	int bus_ocp_disable;

	int bus_ovp_th;
	int bus_ocp_th;

	int ac_ovp_th;

	bool bat_therm_disable;
	bool bus_therm_disable;
	bool die_therm_disable;

	int bat_therm_th; /*in %*/
	int bus_therm_th; /*in %*/
	int die_therm_th; /*in degC*/

	int sense_r_mohm;
};

struct sp2131 {
	struct device *dev;
	struct charger_device *cp_dev;
	struct i2c_client *client;

	int part_no;
	int revision;

	struct delayed_work    wireless_work;

	int mode;

	struct mutex data_lock;
	struct mutex i2c_rw_lock;

	bool batt_present;
	bool vbus_present;

	bool usb_present;
	bool charge_enabled;	/* Register bit status */

	bool acdrv1_enable;
	bool otg_enable;

	int  vbus_error_low;
	int  vbus_error_high;

	/* ADC reading */
	int vbat_volt;
	int vbus_volt;
	int vout_volt;
	int vac_volt;

	int ibat_curr;
	int ibus_curr;

	int bat_temp;
	int bus_temp;
	int die_temp;

	/* fault status */
	bool bat_ovp_fault;
	bool bat_ocp_fault;
	bool bus_ovp_fault;
	bool bus_ocp_fault;

	int  prev_fault;

	int chg_ma;
	int chg_mv;

	struct sp2131_cfg *cfg;
	int irq_gpio;
	int irq;

	int skip_writes;
	int skip_reads;

	struct delayed_work monitor_work;
	struct dentry *debug_root;

	struct charger_device *chg_dev;

	//#ifdef CONFIG_HUAQIN_CP_POLICY_MODULE
	struct charger_device *master_cp_chg;
	struct charger_device *slave_cp_chg;
	//#endif

	const char *chg_dev_name;

	struct power_supply_desc psy_desc;
	struct power_supply_config psy_cfg;
	struct power_supply *psy;
};

#endif /* __SP2131_H__ */
