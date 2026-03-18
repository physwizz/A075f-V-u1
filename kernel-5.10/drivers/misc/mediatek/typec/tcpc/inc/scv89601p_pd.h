/* HS07 code for SR-AL7761A-01-164 by lina at 20250314 start */
#ifndef __LINUX_SCV89601P_PD_H__
#define __LINUX_SCV89601P_PD_H__

#include "std_tcpci_v10.h"
#include "pd_dbg_info.h"

#define ENABLE_SCV89601P_PD_DBG   0

#define SUPPORT_SOUTHCHIP_CPU_BOOST 0

#define SCV89601P_PD_REG_ANA_CTRL1        0x90
#define SCV89601P_PD_REG_VCONN_OCP_CTRL   0x93
#define SCV89601P_PD_REG_ANA_STATUS       0x97
#define SCV89601P_PD_REG_ANA_INT          0x98
#define SCV89601P_PD_REG_ANA_MASK         0x99
#define SCV89601P_PD_REG_ANA_CTRL2        0x9B
#define SCV89601P_PD_REG_ANA_CTRL3        0x9E

#define SCV89601P_PD_REG_RST_CTRL         0xA0
#define SCV89601P_PD_REG_DRP_CTRL         0xA2
#define SCV89601P_PD_REG_DRP_DUTY_CTRL    0xA3

/**
 * SCV89601P_PD_REG_ANA_CTRL1             (0x90)
 */
#define SCV89601P_PD_REG_VCONN_DISCHARGE_EN       (1 << 5)
#define SCV89601P_PD_REG_LPM_EN                   (1 << 3)

/**
 * SCV89601P_PD_REG_ANA_STATUS        (0x97) 
 */
#define SCV89601P_PD_REG_VBUS_80      (1 << 1)

/**
 * SCV89601P_PD_REG_ANA_INT        (0x98) 
 */
#define SCV89601P_PD_REG_INT_HDRST          (1 << 7)
#define SCV89601P_PD_REG_INT_RA_DATECH    (1 << 5)
#define SCV89601P_PD_REG_INT_CC_OVP       (1 << 2)
#define SCV89601P_PD_REG_INT_VBUS_80      (1 << 1)

/**
 * SCV89601P_PD_REG_ANA_MASK          (0x99)
 */
#define SCV89601P_PD_REG_MASK_HDRST          (1 << 7)
#define SCV89601P_PD_REG_MASK_RA_DATECH    (1 << 5)
#define SCV89601P_PD_REG_MASK_CC_OVP       (1 << 2)
#define SCV89601P_PD_REG_MASK_VBUS_80      (1 << 1)

/**
 * SCV89601P_PD_REG_ANA_CTRL2          (0x9B)
 */
#define SCV89601P_PD_REG_SHUTDOWN_OFF         (1 << 5)

/**
 * SCV89601P_PD_REG_ANA_CTRL3          (0x9E)
 */
#define SCV89601P_PD_IICRST_300        (1 << 0)
#define SCV89601P_PD_IICRST_EN        (1 << 7)

#endif /* __LINUX_SCV89601P_PD_H__ */
/* HS07 code for SR-AL7761A-01-164 by lina at 20250314 end */