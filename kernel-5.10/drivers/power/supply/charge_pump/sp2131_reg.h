#ifndef __SP2131_HEADER__
#define __SP2131_HEADER__

/* Register 00h */
#define SP2131_REG_00					0x00
#define SP2131_BAT_OVP_MASK				0xFF
#define SP2131_BAT_OVP_SHIFT				0
#define SP2131_BAT_OVP_BASE				2700
/*HS07 code for HS07-191 by yexuedong at 20250327 start*/
#define SP2131_BAT_OVP_LSB				10
/*HS07 code for HS07-191 by yexuedong at 20250327 end*/

/* Register 01h */
#define SP2131_REG_01					0x01
#define	SP2131_BAT_OCP_DIS_MASK			        0x80
#define	SP2131_BAT_OCP_DIS_SHIFT		        7
#define SP2131_BAT_OCP_ENABLE			        0
#define SP2131_BAT_OCP_DISABLE			        1

#define SP2131_BAT_OCP_MASK				0x7F
#define SP2131_BAT_OCP_SHIFT			        0
#define SP2131_BAT_OCP_BASE				1000
#define SP2131_BAT_OCP_LSB				100

/* Register 02h */
#define SP2131_REG_02					0x02
#define	SP2131_BAT_OVP_DIS_MASK			        0x80
#define	SP2131_BAT_OVP_DIS_SHIFT		        7
#define	SP2131_BAT_OVP_ENABLE			        0
#define	SP2131_BAT_OVP_DISABLE			        1

#define SP2131_VOUT_OVP_DIS_MASK			0x40
#define SP2131_VOUT_OVP_DIS_SHIFT			6
#define SP2131_VOUT_OVP_ENABLE			        0
#define SP2131_VOUT_OVP_DISABLE			        1

#define SP2131_VOUT_OVP_SET_MASK			0x20
#define SP2131_VOUT_OVP_SET_SHIFT			5
#define SP2131_VOUT_OVP_SET_4P9V			0
#define SP2131_VOUT_OVP_SET_5P4V			1

#define SP2131_VAC_OVP_DIS_MASK		                0x10
#define SP2131_VAC_OVP_DIS_SHIFT                        4
#define SP2131_VAC_OVP_ENABLE                           0
#define SP2131_VAC_OVP_DISABLE                          1

#define SP2131_AC_OVP_MASK				0x0f
#define SP2131_AC_OVP_SHIFT				0
#define SP2131_AC_OVP_BASE				6000//6V
#define SP2131_AC_OVP_6P5V				6500
#define SP2131_AC_OVP_LSB				1000//1V

/* Register 03h */
#define SP2131_REG_03					0x03
#define	SP2131_BUS_OVP_DIS_MASK			        0x80
#define	SP2131_BUS_OVP_DIS_SHIFT		        7
#define SP2131_BUS_OVP_ENABLE			        0
#define SP2131_BUS_OVP_DISABLE			        1

#define SP2131_BUS_OVP_MASK				0x7F
#define SP2131_BUS_OVP_SHIFT				0
#define SP2131_BUS_OVP_BASE				5400//5.4V
#define SP2131_BUS_OVP_LSB				100//100mV

/* Register 04h */
#define SP2131_REG_04					0x04
#define SP2131_BUS_SCP_DIS_MASK			        0x80
#define SP2131_BUS_SCP_DIS_SHIFT			7
#define	SP2131_BUS_SCP_ENABLE				0
#define	SP2131_BUS_SCP_DISABLE				1

#define SP2131_BUS_OCP_DIS_MASK			        0x40
#define SP2131_BUS_OCP_DIS_SHIFT			6
#define	SP2131_BUS_OCP_ENABLE				0
#define	SP2131_BUS_OCP_DISABLE				1

#define SP2131_BUS_OCP_MASK				0x3F
#define SP2131_BUS_OCP_SHIFT				0
#define SP2131_BUS_OCP_BASE				500//0.5A
#define SP2131_BUS_OCP_LSB				100//100mA

/* Register 05h */
#define SP2131_REG_05					0x05
/*HS07 code for HS07-191 by yexuedong at 20250327 start*/
#define SP2131_PMID2VOUT_OVP_MASK		        0x30
#define SP2131_PMID2VOUT_OVP_SHIFT		        4
#define SP2131_PMID2VOUT_OVP_BASE		        250//0.5V
#define SP2131_PMID2VOUT_OVP_LSB			    100//100mAV

#define SP2131_PMID2VOUT_UVP_MASK		        0x03
/*HS07 code for HS07-191 by yexuedong at 20250327 end*/
#define SP2131_PMID2VOUT_UVP_SHIFT		        0
#define SP2131_PMID2VOUT_UVP_BASE		        (-100)//-0.1V
#define SP2131_PMID2VOUT_UVP_LSB			(-25)//-25mAV

/* Register 06h */
#define SP2131_REG_06					0x06
#define SP2131_TS_FLT_MASK			        0xFF
#define SP2131_TS_FLT_SHIFT			        0
#define SP2131_TS_FLT_BASE			        0
#define SP2131_TS_FLT_LSB			        10//10mV
#define SP2131_TS_FLT_DISABLE                           0

/* Register 07h */
#define SP2131_REG_07					0x07
#define SP2131_CHG_EN_MASK				0x80
#define SP2131_CHG_EN_SHIFT				7
#define SP2131_CHG_ENABLE				1
#define SP2131_CHG_DISABLE				0
//reversed bit6
#define SP2131_FSW_LOWER_EN_MASK		        0x20
#define SP2131_FSW_LOWER_EN_SHIFT			5
#define SP2131_FSW_LOWER_ENABLE				1
#define SP2131_FSW_LOWER_DISABLE			0

#define SP2131_FREQ_DETHER_MASK		                0x10
#define SP2131_FREQ_DETHER_SHIFT			4
#define SP2131_FREQ_DETHER_NORMINAL                     0
#define SP2131_FREQ_DETHER_SPREAD_SPECTRUM	        1

#define SP2131_FSW_SET_MASK				0x0F
#define SP2131_FSW_SET_SHIFT				0
#define SP2131_FSW_SET_500KHZ				0
#define SP2131_FSW_SET_533KHZ				1
#define SP2131_FSW_SET_571KHZ				2
#define SP2131_FSW_SET_615KHZ				3
#define SP2131_FSW_SET_667KHZ				4
#define SP2131_FSW_SET_727KHZ				5
#define SP2131_FSW_SET_800KHZ				6
#define SP2131_FSW_SET_889KHZ				7
#define SP2131_FSW_SET_1000KHZ				8
#define SP2131_FSW_SET_1143KHZ				9
#define SP2131_FSW_SET_1333KHZ				10
#define SP2131_FSW_SET_1600KHZ				11
#define SP2131_FSW_SET_2000KHZ				12


/* Register 08h */
#define SP2131_REG_08					0x08
#define SP2131_CHG_MODE_MASK				0x80
#define SP2131_CHG_MODE_SHIFT				7
#define SP2131_1_1_MODE					1
#define SP2131_2_1_MODE					0

#define SP2131_REVERSE_MODE_MASK			0x40
#define SP2131_REVERSE_MODE_SHIFT			6
#define SP2131_FORWARD_MODE				1
#define SP2131_REVERSE_MODE				0

/*HS07 code for HS07-191 by yexuedong at 20250327 start*/
#define SP2131_MS_MASK			                0x30
#define SP2131_MS_SHIFT			                4
#define SP2131_MS_STANDALONE                    0
#define SP2131_MS_SLAVE                         1
#define SP2131_MS_MASTER                        2
#define SP2131_MS_STANDALONE1                   3
/*HS07 code for HS07-191 by yexuedong at 20250327 end*/

#define SP2131_REG_RST_MASK				0x08
#define SP2131_REG_RST_SHIFT				3
#define SP2131_REG_RST_ENABLE				1
#define SP2131_REG_RST_DISABLE				0

#define SP2131_FORCE_SLEEP_MASK		                0x04
#define SP2131_FORCE_SLEEP_SHIFT                        2
#define SP2131_FORCE_SLEEP_ENABLE                       1
#define SP2131_FORCE_SLEEP_DISABLE                      0

#define SP2131_CFLY_PRECHG_TIMEOUT_MASK		        0x03
#define SP2131_CFLY_PRECHG_TIMEOUT_SHIFT                0
#define SP2131_CFLY_PRECHG_TIMEOUT_10MS                 0
#define SP2131_CFLY_PRECHG_TIMEOUT_20MS                 1
#define SP2131_CFLY_PRECHG_TIMEOUT_40MS                 2
#define SP2131_CFLY_PRECHG_TIMEOUT_80MS                 3


/* Register 09h */
#define SP2131_REG_09					0x09
#define SP2131_SS_TIMEOUT_SET_MASK			0xE0
#define SP2131_SS_TIMEOUT_SET_SHIFT			5
#define SP2131_SS_TIMEOUT_DISABLE			0
#define SP2131_SS_TIMEOUT_40MS			        1
#define SP2131_SS_TIMEOUT_80MS				2
#define SP2131_SS_TIMEOUT_320MS				3
#define SP2131_SS_TIMEOUT_1280MS			4
#define SP2131_SS_TIMEOUT_5120MS			5
#define SP2131_SS_TIMEOUT_20480MS			6
#define SP2131_SS_TIMEOUT_81920MS			7


#define SP2131_BUS_UCP_DIS_MASK			        0x10
#define SP2131_BUS_UCP_DIS_SHIFT			4
#define	SP2131_BUS_UCP_ENABLE				0
#define	SP2131_BUS_UCP_DISABLE				1

#define SP2131_SET_BUS_UCP_TH_MASK			0x08
#define SP2131_SET_BUS_UCP_TH_SHIFT			3
#define SP2131_SET_BUS_UCP_TH_150_300			0
#define SP2131_SET_BUS_UCP_TH_250_500			1

#define SP2131_SET_IBAT_SNS_RES_MASK			0x03
#define SP2131_SET_IBAT_SNS_RES_SHIFT			0
#define SP2131_SET_IBAT_SNS_RES_2MHM			0
#define SP2131_SET_IBAT_SNS_RES_5MHM			1
#define SP2131_SET_IBAT_SNS_RES_1MHM			2
#define SP2131_SET_IBAT_SNS_RES_2P0MHM			3


/* Register 0Ah */
#define SP2131_REG_0A					0x0A
#define SP2131_PMID2VOUT_OVP_DIS_MASK		        0x80
#define SP2131_PMID2VOUT_OVP_DIS_SHIFT                  7
#define SP2131_PMID2VOUT_OVP_ENABLE                     0
#define SP2131_PMID2VOUT_OVP_DISABLE                    1

#define SP2131_PMID2VOUT_UVP_DIS_MASK		        0x40
#define SP2131_PMID2VOUT_UVP_DIS_SHIFT                  6
#define SP2131_PMID2VOUT_UVP_ENABLE                     0
#define SP2131_PMID2VOUT_UVP_DISABLE                    1

#define SP2131_VBUS_VALID_DIS_MASK		        0x20
#define SP2131_VBUS_VALID_DIS_SHIFT                     5
#define SP2131_VBUS_VALID_ENABLE                        0
#define SP2131_VBUS_VALID_DISABLE                       1

#define SP2131_VBUS_ERRORHI_TH_MASK		        0x10
#define SP2131_VBUS_ERRORHI_TH_SHIFT                    4
#define SP2131_VBUS_ERRORHI_TH_0P9V                     0
#define SP2131_VBUS_ERRORHI_TH_1P5V                     1

#define SP2131_VOUT_VALID_TH_MASK			0x0F
#define SP2131_VOUT_VALID_TH_SHIFT			0
#define SP2131_VOUT_VALID_TH_BASE			2600//2.6V
#define SP2131_VOUT_VALID_TH_LSB			100//0.1V


/* Register 0Bh */
#define SP2131_REG_0B					0xB
#define SP2131_WATCHDOG_MASK				0xE0
#define SP2131_WATCHDOG_SHIFT				5
#define SP2131_WATCHDOG_DIS				0
#define SP2131_WATCHDOG_0P2S				1
#define SP2131_WATCHDOG_0P5S				2
#define SP2131_WATCHDOG_1S				3
#define SP2131_WATCHDOG_5S				4
#define SP2131_WATCHDOG_30S				5
#define SP2131_WATCHDOG_60S				6
#define SP2131_WATCHDOG_120S				7
//reversed bit4
#define SP2131_VAC_PD_AUTO_EN_MASK		        0x08
#define SP2131_VAC_PD_AUTO_EN_SHIFT                     3
#define SP2131_VAC_PD_AUTO_ENABLE                       1
#define SP2131_VAC_PD_AUTO_DISABLE                      0

#define SP2131_VAC_PD_EN_MASK		                0x04
#define SP2131_VAC_PD_EN_SHIFT		                2
#define SP2131_VAC_PD_ENABLE                            1
#define SP2131_VAC_PD_DISABLE                           0

#define SP2131_VBUS_PD_AUTO_EN_MASK		        0x02
#define SP2131_VBUS_PD_AUTO_EN_SHIFT                    1
#define SP2131_VBUS_PD_AUTO_ENABLE                      1
#define SP2131_VBUS_PD_AUTO_DISABLE                     0

#define SP2131_VBUS_PD_EN_MASK		                0x01
#define SP2131_VBUS_PD_EN_SHIFT		                0
#define SP2131_VBUS_PD_ENABLE                           1
#define SP2131_VBUS_PD_DISABLE                          0


/* Register 0Ch */
#define SP2131_REG_0C					0x0C
#define SP2131_EN_OTG_MASK		                0x80
#define SP2131_EN_OTG_SHIFT	                        7
#define SP2131_OTG_ENABLE		                1
#define SP2131_OTG_DISABLE		                0

#define SP2131_OVPGATE_MANUAL_MASK		        0x40
#define SP2131_OVPGATE_MANUAL_SHIFT                     6
#define SP2131_OVPGATE_MANUAL_EN                        1
#define SP2131_OVPGATE_MANUAL_DIS                       0

#define SP2131_OVPGATE_EN_MASK		                0x20
#define SP2131_OVPGATE_EN_SHIFT                         5
#define SP2131_OVPGATE_EN_ENABLE                        1
#define SP2131_OVPGATE_EN_DISABLE                       0

#define SP2131_OVPGATE_DR_MASK		                0x10
#define SP2131_OVPGATE_DR_SHIFT                         4
#define SP2131_OVPGATE_DR_4P75V                         0
#define SP2131_OVPGATE_DR_10V                           1

#define SP2131_SKIP_Q1_OK_CHECK_EN_MASK		        0x08
#define SP2131_SKIP_Q1_OK_CHECK_EN_SHIFT                3
#define SP2131_SKIP_Q1_OK_CHECK_ENABLE                  1
#define SP2131_SKIP_Q1_OK_CHECK_DISABLE                 0

#define SP2131_SKIP_BSTCAP_OPEN_CHECK_EN_MASK		0x04
#define SP2131_SKIP_BSTCAP_OPEN_CHECK_EN_SHIFT          2
#define SP2131_SKIP_BSTCAP_OPEN_CHECK_ENABLE            1
#define SP2131_SKIP_BSTCAP_OPEN_CHECK_DISABLE           0

#define SP2131_1V2_PU_EN_MASK		                0x02
#define SP2131_1V2_PU_EN_SHIFT                          1
#define SP2131_1V2_PU_ENABLE                            1
#define SP2131_1V2_PU_DISABLE                           0
//reversed bit0


/* Register 0Dh */
#define SP2131_REG_0D					0x0D
#define SP2131_VBUS_OVP_DG_MASK		                0x80
#define SP2131_VBUS_OVP_DG_SHIFT	                7
#define SP2131_VBUS_OVP_DG_1US		                0
#define SP2131_VBUS_OVP_DG_10US		                1

#define SP2131_IBUS_OCP_DG_MASK		                0x60
#define SP2131_IBUS_OCP_DG_SHIFT	                5
#define SP2131_IBUS_OCP_DG_75US		                0
#define SP2131_IBUS_OCP_DG_400US		        1
#define SP2131_IBUS_OCP_DG_1MS		                2
#define SP2131_IBUS_OCP_DG_5MS		                3

#define SP2131_VOUT_OVP_DG_MASK		                0x10
#define SP2131_VOUT_OVP_DG_SHIFT	                4
#define SP2131_VOUT_OVP_DG_5US		                0
#define SP2131_VOUT_OVP_DG_5MS		                1

#define SP2131_IBAT_OCP_DG_MASK		                0x0C
#define SP2131_IBAT_OCP_DG_SHIFT	                2
#define SP2131_IBAT_OCP_DG_5US		                0
#define SP2131_IBAT_OCP_DG_500US		        1
#define SP2131_IBAT_OCP_DG_2MS		                2
#define SP2131_IBAT_OCP_DG_5MS		                3

#define SP2131_PMID2VOUT_OVP_DG_MASK		        0x02
#define SP2131_PMID2VOUT_OVP_DG_SHIFT	                1
#define SP2131_PMID2VOUT_OVP_DG_100NS		        0
#define SP2131_PMID2VOUT_OVP_DG_1US		        1

#define SP2131_PMID2VOUT_UVP_DG_MASK		        0x01
#define SP2131_PMID2VOUT_UVP_DG_SHIFT	                0
#define SP2131_PMID2VOUT_UVP_DG_100NS		        0
#define SP2131_PMID2VOUT_UVP_DG_1US		        1


/* Register 0Eh */
#define SP2131_REG_0E					0x0E
#define SP2131_VAC_GOOD_DEG_MASK		        0x80
#define SP2131_VAC_GOOD_DEG_SHIFT	                7
/*HS07 code for HS07-191 by yexuedong at 20250327 start*/
#define SP2131_VAC_GOOD_DEG_80MS		        0
#define SP2131_VAC_GOOD_DEG_280MS		        1

#define SP2131_FORCE_VBATSNS_ENABLE_MASK		    0x40
#define SP2131_FORCE_VBATSNS_ENABLE_SHIFT	        6
#define SP2131_FORCE_VBATSNS_DISABLE		        0
#define SP2131_FORCE_VBATSNS_ENABLE		            1

#define SP2131_VBAT_OVP_DEG_MASK		        0x30
#define SP2131_VBAT_OVP_DEG_SHIFT	                4
#define SP2131_VBAT_OVP_DEG_1US		                0
#define SP2131_VBAT_OVP_DEG_10US		            1
/*HS07 code for HS07-191 by yexuedong at 20250327 end*/
#define SP2131_VBAT_OVP_DEG_1MS		                2
#define SP2131_VBAT_OVP_DEG_5MS		                3

#define SP2131_VOUT_VALID_DEG_MASK		        0x08
#define SP2131_VOUT_VALID_DEG_SHIFT	                3
#define SP2131_VOUT_VALID_DEG_20MS		        0
#define SP2131_VOUT_VALID_DEG_100US		        1

#define SP2131_VBUS_ERR_DG_MASK		                0x04
#define SP2131_VBUS_ERR_DG_SHIFT	                2
#define SP2131_VBUS_ERR_DG_5MS_2MS	                0
#define SP2131_VBUS_ERR_DG_20MS_10MS		        1

#define SP2131_IBUS_UCP_FALL_DG_MASK		        0x03
#define SP2131_IBUS_UCP_FALL_DG_SHIFT                   0
#define SP2131_IBUS_UCP_FALL_DG_10US                    0
#define SP2131_IBUS_UCP_FALL_DG_5MS                     1
#define SP2131_IBUS_UCP_FALL_DG_50MS                    2
#define SP2131_IBUS_UCP_FALL_DG_150MS                   3


/* Register 0Fh */
#define SP2131_REG_0F					0x0F
#define	SP2131_VOUT_OVP_STAT_MASK		        0x80
#define	SP2131_VOUT_OVP_STAT_SHIFT		        7

#define SP2131_BAT_OVP_STAT_MASK			0x40
#define SP2131_BAT_OVP_STAT_SHIFT			6

#define SP2131_BAT_OCP_STAT_MASK			0x20
#define SP2131_BAT_OCP_STAT_SHIFT			5

#define SP2131_BUS_OVP_STAT_MASK			0x10
#define SP2131_BUS_OVP_STAT_SHIFT			4

#define SP2131_BUS_OCP_STAT_MASK			0x08
#define SP2131_BUS_OCP_STAT_SHIFT			3

#define SP2131_IBUS_UCP_FALL_STAT_MASK			0x04
#define SP2131_IBUS_UCP_FALL_STAT_SHIFT		        2

#define SP2131_ADAPTER_INSERT_STAT_MASK			0x02
#define SP2131_ADAPTER_INSERT_STAT_SHIFT	        1

#define SP2131_VBAT_INSERT_STAT_MASK			0x01
#define SP2131_VBAT_INSERT_STAT_SHIFT			0


/* Register 10h */
#define SP2131_REG_10					0x10
#define SP2131_TSD_STAT_MASK				0x80
#define SP2131_TSD_STAT_SHIFT			        7

#define SP2131_TS_FLT_STAT_MASK			        0x40
#define SP2131_TS_FLT_STAT_SHIFT			6

#define SP2131_PMID2VOUT_OVP_STAT_MASK		        0x20
#define SP2131_PMID2VOUT_OVP_STAT_SHIFT		        5

#define SP2131_PMID2VOUT_UVP_STAT_MASK		        0x10
#define SP2131_PMID2VOUT_UVP_STAT_SHIFT		        4

#define SP2131_VBUS_ERRHI_STAT_MASK		        0x08
#define SP2131_VBUS_ERRHI_STAT_SHIFT	                3

#define SP2131_VBUS_ERRLO_STAT_MASK		        0x04
#define SP2131_VBUS_ERRLO_STAT_SHIFT	                2
//reserved bit1
#define SP2131_AC_OVP_STAT_MASK			        0x01
#define SP2131_AC_OVP_STAT_SHIFT			0


/* Register 11h */
#define SP2131_REG_11					0x11
//reserved bit7
#define SP2131_VOUT_INVALID_REVERSE_STAT_MASK		0x40
#define SP2131_VOUT_INVALID_REVERSE_STAT_SHIFT	        6

#define SP2131_IBUS_UCP_RISE_STAT_MASK			0x20
#define SP2131_IBUS_UCP_RISE_STAT_SHIFT		        5

#define SP2131_CP_ACTIVE_STAT_MASK		        0x10
#define SP2131_CP_ACTIVE_STAT_SHIFT		        4

#define SP2131_VBUS_GOOD_STAT_MASK		        0x08
#define SP2131_VBUS_GOOD_STAT_SHIFT		        3

#define SP2131_ADC_DONE_STAT_MASK			0x04
#define SP2131_ADC_DONE_STAT_SHIFT			2
//reserved bit0-1


/* Register 12h */
#define SP2131_REG_12					0x12
#define	SP2131_VOUT_OVP_FLAG_MASK		        0x80
#define	SP2131_VOUT_OVP_FLAG_SHIFT		        7

#define SP2131_BAT_OVP_FLT_FLAG_MASK			0x40
#define SP2131_BAT_OVP_FLT_FLAG_SHIFT			6

#define SP2131_BAT_OCP_FLT_FLAG_MASK			0x20
#define SP2131_BAT_OCP_FLT_FLAG_SHIFT			5

#define SP2131_BUS_OVP_FLT_FLAG_MASK			0x10
#define SP2131_BUS_OVP_FLT_FLAG_SHIFT			4

#define SP2131_BUS_OCP_FLT_FLAG_MASK			0x08
#define SP2131_BUS_OCP_FLT_FLAG_SHIFT			3

#define SP2131_IBUS_UCP_FALL_FLAG_MASK			0x04
#define SP2131_IBUS_UCP_FALL_FLAG_SHIFT		        2

#define SP2131_ADAPTER_INSERT_FLAG_MASK			0x02
#define SP2131_ADAPTER_INSERT_FLAG_SHIFT	        1

#define SP2131_VBAT_INSERT_FLAG_MASK			0x01
#define SP2131_VBAT_INSERT_FLAG_SHIFT			0


/* Register 13h */
#define SP2131_REG_13					0x13
#define SP2131_TSD_FLAG_MASK				0x80
#define SP2131_TSD_FLAG_SHIFT			        7

#define SP2131_TS_FLT_FLAG_MASK			        0x40
#define SP2131_TS_FLT_FLAG_SHIFT			6

#define SP2131_PMID2VOUT_OVP_FLAG_MASK		        0x20
#define SP2131_PMID2VOUT_OVP_FALG_SHIFT		        5

#define SP2131_PMID2VOUT_UVP_FLAG_MASK		        0x10
#define SP2131_PMID2VOUT_UVP_FALG_SHIFT		        4

#define SP2131_VBUS_ERRHI_FLAG_MASK		        0x08
#define SP2131_VBUS_ERRHI_FLAG_SHIFT	                3

#define SP2131_VBUS_ERRLO_FLAG_MASK		        0x04
#define SP2131_VBUS_ERRLO_FLAG_SHIFT	                2
//reserved bit1
#define SP2131_AC_OVP_FLAG_MASK			        0x01
#define SP2131_AC_OVP_FLAG_SHIFT			0


/* Register 14h */
#define SP2131_REG_14					0x14
//reserved bit7
#define SP2131_VOUT_INVALID_REVERSE_FLAG_MASK		0x40
#define SP2131_VOUT_INVALID_REVERSE_FLAG_SHIFT	        6

#define SP2131_IBUS_UCP_RISE_FLAG_MASK			0x20
#define SP2131_IBUS_UCP_RISE_FLAG_SHIFT		        5

#define SP2131_CP_ACTIVE_FLAG_MASK		        0x10
#define SP2131_CP_ACTIVE_FLAG_SHIFT		        4

#define SP2131_VBUS_GOOD_FLAG_MASK		        0x08
#define SP2131_VBUS_GOOD_FLAG_SHIFT		        3

#define SP2131_ADC_DONE_FLAG_MASK			0x04
#define SP2131_ADC_DONE_FLAG_SHIFT			2
//reserved bit0-1


/* Register 15h */
#define SP2131_REG_15					0x15
//reserved bit7
#define SP2131_VOUT_UVLO_FALL_FLAG_MASK		        0x40
#define SP2131_VOUT_UVLO_FALL_FLAG_SHIFT	        6

#define SP2131_VBUS_UVLO_FALL_FLAG_MASK		        0x20
#define SP2131_VBUS_UVLO_FALL_FLAG_SHIFT	        5

#define SP2131_POWER_NG_FLAG_MASK		        0x10
#define SP2131_POWER_NG_FLAG_SHIFT                      4

#define SP2131_WD_TIMEOUT_FLAG_MASK			0x08
#define SP2131_WD_TIMEOUT_SHIFT			        3

#define SP2131_SS_TIMEOUT_FLAG_MASK			0x04
#define SP2131_SS_TIMEOUT_FLAG_SHIFT			2
//reserved bit1
#define SP2131_PIN_DIAG_FALL_FLAG_MASK			0x01
#define SP2131_PIN_DIAG_FALL_FLAG_SHIFT		        0


/* Register 16h */
#define SP2131_REG_16					0x16
#define	SP2131_VOUT_OVP_MASK_MASK		        0x80
#define	SP2131_VOUT_OVP_MASK_SHIFT		        7
#define SP2131_VOUT_OVP_MASK_ENABLE		        1
#define SP2131_VOUT_OVP_MASK_DISABLE	                0

#define SP2131_BAT_OVP_MASK_MASK			0x40
#define SP2131_BAT_OVP_MASK_SHIFT			6
#define SP2131_BAT_OVP_MASK_ENABLE		        1
#define SP2131_BAT_OVP_MASK_DISABLE	                0

#define SP2131_BAT_OCP_MASK_MASK			0x20
#define SP2131_BAT_OCP_MASK_SHIFT			5
#define SP2131_BAT_OCP_MASK_ENABLE		        1
#define SP2131_BAT_OCP_MASK_DISABLE	                0

#define SP2131_BUS_OVP_MASK_MASK			0x10
#define SP2131_BUS_OVP_MASK_SHIFT			4
#define SP2131_BUS_OVP_MASK_ENABLE		        1
#define SP2131_BUS_OVP_MASK_DISABLE	                0

#define SP2131_BUS_OCP_MASK_MASK			0x08
#define SP2131_BUS_OCP_MASK_SHIFT			3
#define SP2131_BUS_OCP_MASK_ENABLE		        1
#define SP2131_BUS_OCP_MASK_DISABLE	                0

#define SP2131_IBUS_UCP_FALL_MASK_MASK			0x04
#define SP2131_IBUS_UCP_FALL_MASK_SHIFT		        2
#define SP2131_IBUS_UCP_FALL_MASK_ENABLE		1
#define SP2131_IBUS_UCP_FALL_MASK_DISABLE	        0

#define SP2131_ADAPTER_INSERT_MASK_MASK			0x02
#define SP2131_ADAPTER_INSERT_MASK_SHIFT	        1
#define SP2131_ADAPTER_INSERT_MASK_ENABLE		1
#define SP2131_ADAPTER_INSERT_MASK_DISABLE	        0

#define SP2131_VBAT_INSERT_MASK_MASK			0x01
#define SP2131_VBAT_INSERT_MASK_SHIFT	                0
#define SP2131_VBAT_INSERT_MASK_ENABLE		        1
#define SP2131_VBAT_INSERT_MASK_DISABLE	                0


/* Register 17h */
#define SP2131_REG_17					0x17
//reserved bit7
#define SP2131_TS_FLT_MASK_MASK			        0x40
#define SP2131_TS_FLT_MASK_SHIFT			6
#define SP2131_TS_FLT_MASK_ENABLE			1
#define SP2131_TS_FLT_MASK_DISABLE			0

#define SP2131_PMID2VOUT_OVP_MASK_MASK		        0x20
#define SP2131_PMID2VOUT_OVP_MASK_SHIFT		        5
#define SP2131_PMID2VOUT_OVP_MASK_ENABLE		1
#define SP2131_PMID2VOUT_OVP_MASK_DISABLE		0

#define SP2131_PMID2VOUT_UVP_MASK_MASK		        0x10
#define SP2131_PMID2VOUT_UVP_MASK_SHIFT		        4
#define SP2131_PMID2VOUT_UVP_MASK_ENABLE                1
#define SP2131_PMID2VOUT_UVP_MASK_DISABLE               0

#define SP2131_VBUS_ERRHI_MASK_MASK	                0x08
#define SP2131_VBUS_ERRHI_MASK_SHIFT	                3
#define SP2131_VBUS_ERRHI_MASK_ENABLE                   1
#define SP2131_VBUS_ERRHI_MASK_DISABLE                  0

#define SP2131_VBUS_ERRLO_MASK_MASK	                0x04
#define SP2131_VBUS_ERRLO_MASK_SHIFT                    2
#define SP2131_VBUS_ERRLO_MASK_ENABLE                   1
#define SP2131_VBUS_ERRLO_MASK_DISABLE                  0
//reserved bit1
#define SP2131_AC_OVP_MASK_MASK			        0x01
#define SP2131_AC_OVP_MASK_SHIFT			0
#define SP2131_AC_OVP_MASK_ENABLE                       1
#define SP2131_AC_OVP_MASK_DISABLE                      0


/* Register 18h */
#define SP2131_REG_18					0x18
//reserved bit7
#define SP2131_VOUT_INVALID_REVERSE_MASK_MASK		0x40
#define SP2131_VOUT_INVALID_REVERSE_MASK_SHIFT	        6
#define SP2131_VOUT_INVALID_REVERSE_MASK_ENABLE         1
#define SP2131_VOUT_INVALID_REVERSE_MASK_DISABLE        0

#define SP2131_IBUS_UCP_RISE_MASK_MASK			0x20
#define SP2131_IBUS_UCP_RISE_MASK_SHIFT		        5
#define SP2131_IBUS_UCP_RISE_MASK_ENABLE                1
#define SP2131_IBUS_UCP_RISE_MASK_DISABLE               0

#define SP2131_CP_ACTIVE_MASK_MASK		        0x10
#define SP2131_CP_ACTIVE_MASK_SHIFT		        4

#define SP2131_VBUS_GOOD_MASK_MASK		        0x08
#define SP2131_VBUS_GOOD_MASK_SHIFT		        3
#define SP2131_VBUS_GOOD_MASK_ENABLE                    1
#define SP2131_VBUS_GOOD_MASK_DISABLE                   0

#define SP2131_ADC_DONE_MASK_MASK			0x04
#define SP2131_ADC_DONE_MASK_SHIFT			2
#define SP2131_ADC_DONE_MASK_ENABLE                     1
#define SP2131_ADC_DONE_MASK_DISABLE                    0
//reserved bit0-1


/* Register 19h */
#define SP2131_REG_19					0x19
#define SP2131_ADC_EN_MASK				0x80
#define SP2131_ADC_EN_SHIFT				7
#define SP2131_ADC_ENABLE				1
#define SP2131_ADC_DISABLE				0

#define SP2131_ADC_RATE_MASK				0x40
#define SP2131_ADC_RATE_SHIFT				6
#define SP2131_ADC_RATE_CONTINOUS			0
#define SP2131_ADC_RATE_ONESHOT			        1

#define SP2131_ADC_CLOCK_SET_MASK		        0x20
#define SP2131_ADC_CLOCK_SET_SHIFT			5
#define SP2131_ADC_CLOCK_SET_470KHZ			0
#define SP2131_ADC_CLOCK_SET_727KHZ			1

#define SP2131_ADC_SAMPLE_TIME_SET_MASK		        0x18
#define SP2131_ADC_SAMPLE_TIME_SET_SHIFT		3
#define SP2131_ADC_SAMPLE_TIME_SET_1024			0
#define SP2131_ADC_SAMPLE_TIME_SET_1280			1
#define SP2131_ADC_SAMPLE_TIME_SET_1536			2
#define SP2131_ADC_SAMPLE_TIME_SET_1792			3
//reserved bit0-2


/* Register 1Ah */
#define SP2131_REG_1A					0x1A
#define SP2131_TS_ADC_DIS_MASK			        0x80
#define SP2131_TS_ADC_DIS_SHIFT			        7
#define SP2131_TS_ADC_ENABLE				0
#define SP2131_TS_ADC_DISABLE			        1

#define SP2131_VBUS_ADC_DIS_MASK			0x40
#define SP2131_VBUS_ADC_DIS_SHIFT			6
#define SP2131_VBUS_ADC_ENABLE				0
#define SP2131_VBUS_ADC_DISABLE			        1

#define SP2131_VAC_ADC_DIS_MASK			        0x20
#define SP2131_VAC_ADC_DIS_SHIFT			5
#define SP2131_VAC_ADC_ENABLE				0
#define SP2131_VAC_ADC_DISABLE			        1

#define SP2131_VOUT_ADC_DIS_MASK			0x10
#define SP2131_VOUT_ADC_DIS_SHIFT			4
#define SP2131_VOUT_ADC_ENABLE				0
#define SP2131_VOUT_ADC_DISABLE			        1

#define SP2131_VBAT_ADC_DIS_MASK			0x08
#define SP2131_VBAT_ADC_DIS_SHIFT			3
#define SP2131_VBAT_ADC_ENABLE				0
#define SP2131_VBAT_ADC_DISABLE			        1

#define SP2131_IBAT_ADC_DIS_MASK			0x04
#define SP2131_IBAT_ADC_DIS_SHIFT			2
#define SP2131_IBAT_ADC_ENABLE				0
#define SP2131_IBAT_ADC_DISABLE			        1

#define SP2131_IBUS_ADC_DIS_MASK			0x02
#define SP2131_IBUS_ADC_DIS_SHIFT			1
#define SP2131_IBUS_ADC_ENABLE				0
#define SP2131_IBUS_ADC_DISABLE			        1

#define SP2131_TDIE_ADC_DIS_MASK			0x01
#define SP2131_TDIE_ADC_DIS_SHIFT			0
#define SP2131_TDIE_ADC_ENABLE				0
#define SP2131_TDIE_ADC_DISABLE			        1


/* Register 1Bh */
#define SP2131_REG_1B					0x1B
#define SP2131_IBUS_POL_MASK				0x80
#define SP2131_IBUS_POL_SHIFT				7
#define SP2131_IBUS_POL_POSITIVE			0
#define SP2131_IBUS_POL_NAGETIVE			1

#define SP2131_IBUS_ADC1_MASK				0x1F
#define SP2131_IBUS_ADC1_SHIFT				0
#define SP2131_IBUS_ADC1_BASE				0
#define SP2131_IBUS_ADC1_LSB				256


/* Register 1Ch */
#define SP2131_REG_1C					0x1C
#define SP2131_IBUS_ADC0_MASK				0xFF
#define SP2131_IBUS_ADC0_SHIFT				0
#define SP2131_IBUS_ADC0_BASE				0
#define SP2131_IBUS_ADC0_LSB				1


/* Register 1Dh */
#define SP2131_REG_1D					0x1D
#define SP2131_VBUS_POL_MASK				0x80
#define SP2131_VBUS_POL_SHIFT				7
#define SP2131_VBUS_POL_POSITIVE			0
#define SP2131_VBUS_POL_NEGATIVE			1

#define SP2131_VBUS_ADC1_MASK				0x3F
#define SP2131_VBUS_ADC1_SHIFT				0
#define SP2131_VBUS_ADC1_BASE				0
#define SP2131_VBUS_ADC1_LSB				256


/* Register 1Eh */
#define SP2131_REG_1E					0x1E
#define SP2131_VBUS_ADC0_MASK				0xFF
#define SP2131_VBUS_ADC0_SHIFT				0
#define SP2131_VBUS_ADC0_BASE				0
#define SP2131_VBUS_ADC0_LSB				1


/* Register 1Fh */
#define SP2131_REG_1F					0x1F
#define SP2131_VAC_POL_MASK				0x80
#define SP2131_VAC_POL_SHIFT				7
#define SP2131_VAC_POL_POSITIVE			        0
#define SP2131_VAC_POL_NEGATIVE			        1

#define SP2131_VAC_ADC1_MASK				0x3F
#define SP2131_VAC_ADC1_SHIFT				0
#define SP2131_VAC_ADC1_BASE				0
#define SP2131_VAC_ADC1_LSB				256


/* Register 20h */
#define SP2131_REG_20					0x20
#define SP2131_VAC_ADC0_MASK				0xFF
#define SP2131_VAC_ADC0_SHIFT				0
#define SP2131_VAC_ADC0_BASE				0
#define SP2131_VAC_ADC0_LSB				1


/* Register 21h */
#define SP2131_REG_21					0x21
#define SP2131_VOUT_POL_MASK				0x80
#define SP2131_VOUT_POL_SHIFT				7
#define SP2131_VOUT_POL_POSITIVE			0
#define SP2131_VOUT_POL_NEGATIVE			1

/*HS07 code for HS07-191 by yexuedong at 20250327 start*/
#define SP2131_VOUT_ADC1_MASK				0x1F
/*HS07 code for HS07-191 by yexuedong at 20250327 end*/
#define SP2131_VOUT_ADC1_SHIFT				0
#define SP2131_VOUT_ADC1_BASE				0
#define SP2131_VOUT_ADC1_LSB				256


/* Register 22h */
#define SP2131_REG_22					0x22
#define SP2131_VOUT_ADC0_MASK				0xFF
#define SP2131_VOUT_ADC0_SHIFT				0
#define SP2131_VOUT_ADC0_BASE				0
#define SP2131_VOUT_ADC0_LSB				1


/* Register 23h */
#define SP2131_REG_23					0x23
#define SP2131_VBAT_POL_MASK				0x80
#define SP2131_VBAT_POL_SHIFT				7
#define SP2131_VBAT_POL_POSITIVE			0
#define SP2131_VBAT_POL_NEGATIVE			1

/*HS07 code for HS07-191 by yexuedong at 20250327 start*/
#define SP2131_VBAT_ADC1_MASK				0x1F
/*HS07 code for HS07-191 by yexuedong at 20250327 end*/
#define SP2131_VBAT_ADC1_SHIFT				0
#define SP2131_VBAT_ADC1_BASE				0
#define SP2131_VBAT_ADC1_LSB				256


/* Register 24h */
#define SP2131_REG_24					0x24
#define SP2131_VBAT_ADC0_MASK				0xFF
#define SP2131_VBAT_ADC0_SHIFT				0
#define SP2131_VBAT_ADC0_BASE				0
#define SP2131_VBAT_ADC0_LSB				1


/* Register 25h */
#define SP2131_REG_25					0x25
#define SP2131_IBAT_POL_MASK				0x80
#define SP2131_IBAT_POL_SHIFT				7
#define SP2131_IBAT_POL_POSITIVE			0
#define SP2131_IBAT_POL_NEGATIVE			1

#define SP2131_IBAT_ADC1_MASK				0x3F
#define SP2131_IBAT_ADC1_SHIFT				0
#define SP2131_IBAT_ADC1_BASE				0
#define SP2131_IBAT_ADC1_LSB				256


/* Register 26h */
#define SP2131_REG_26					0x26
#define SP2131_IBAT_ADC0_MASK				0xFF
#define SP2131_IBAT_ADC0_SHIFT				0
#define SP2131_IBAT_ADC0_BASE				0
#define SP2131_IBAT_ADC0_LSB				1


/* Register 27h */
#define SP2131_REG_27					0x27
#define SP2131_TDIE_POL_MASK				0x80
#define SP2131_TDIE_POL_SHIFT				7
#define SP2131_TDIE_POL_POSITIVE			0
#define SP2131_TDIE_POL_NEGATIVE			1

#define SP2131_TDIE_ADC1_MASK				0x01
#define SP2131_TDIE_ADC1_SHIFT				0
#define SP2131_TDIE_ADC1_BASE				0
#define SP2131_TDIE_ADC1_LSB				128


/* Register 28h */
#define SP2131_REG_28					0x28
#define SP2131_TDIE_ADC0_MASK				0xFF
#define SP2131_TDIE_ADC0_SHIFT				0
#define SP2131_TDIE_ADC0_BASE				0
#define SP2131_TDIE_ADC0_LSB				0.5


/* Register 29h */
#define SP2131_REG_29					0x29
#define SP2131_TS_POL_MASK				0x80
#define SP2131_TS_POL_SHIFT				7
#define SP2131_TS_POL_POSITIVE			        0
#define SP2131_TS_POL_NEGATIVE			        1

#define SP2131_TS_ADC1_MASK				0x10
#define SP2131_TS_ADC1_SHIFT			        0
#define SP2131_TS_ADC1_BASE				0
#define SP2131_TS_ADC1_LSB				64


/* Register 2Ah */
#define SP2131_REG_2A					0x2A
#define SP2131_TS_ADC0_MASK				0xFF
#define SP2131_TS_ADC0_SHIFT			        0
#define SP2131_TS_ADC0_BASE				0
#define SP2131_TS_ADC0_LSB				0.25


/* Register 2Bh */
#define SP2131_REG_2B					0x2B
#define SP2131_DEV_ID_MASK				0xFF
#define SP2131_DEV_ID_SHIFT				0

#endif
