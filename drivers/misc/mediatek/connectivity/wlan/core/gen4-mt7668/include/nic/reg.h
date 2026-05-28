/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "reg.h"
 *   \brief  The common register definition of MT6630
 *
 *   N/A
 */

#ifndef _REG_H
#define _REG_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

extern struct hif_driver_data driver_data_mt6632;
extern struct hif_driver_data driver_data_mt7668;

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/* 1 MT6630 MCR Definition */

/* 2 Host Interface */

/* #define TOP_CFG_BASE 0x0000 */
#define TOP_CFG_BASE                 0x80020000
#define TOP_RGU_BASE                 0x81020000
#define TOP_CFG_AON_base             0x81021000
#define MCU_CFG_BASE                 0x80000000

#define TOP_HVR                  (TOP_CFG_BASE + 0x1000)
#define HW_VER_MASK              (0xffff)
#define GET_HW_VER(p)          (((p) & HW_VER_MASK))
#define RESC_CID_MASK                (0xf << 28)
#define GET_RESC_CID(p)        (((p) & RESC_CID_MASK) >> 28)

#define TOP_FVR                  (TOP_CFG_BASE + 0x1004)
#define FW_VER_MASK              (0xff)
#define GET_FW_VER(p)          (((p) & FW_VER_MASK))

#define TOP_HCR                  (TOP_CFG_BASE + 0x1008)
#define HW_CODE_MASK                 (0xffff)
#define GET_HW_CODE(p)         (((p) & HW_CODE_MASK))

#define STRAP_STA                (TOP_CFG_BASE + 0x1010)
#define XTAL_SEL_MASK                (0x3)
#define GET_XTAL_SEL(p)        (((p) & XTAL_SEL_MASK))
#define EEPROM_SEL               (1 << 2)
#define GET_EEPROM_SEL(p)      (((p) & EEPROM_SEL) >> 2)
#define CO_CLOCK_SEL                 (1 << 8)
#define GET_CO_CLOCK_SEL(p)    (((p) & CO_CLOCK_SEL) >> 8)
#define ONE_ANT                  (1 << 24)
#define GET_ONE_ANT(p)         (((p) & ONE_ANT) >> 24)
#define USB_MODE                 (1 << 25)
#define GET_USB_MODE(p)        (((p) & USB_MODE) >> 25)

#define TOP_MISC2                (TOP_CFG_BASE + 0x1134)

#define TOP_CKGEN2_CR_PMIC_CK_MANUAL         (TOP_CFG_AON_base + 0x00000108)
#define TOP_CKGEN2_CR_PMIC_CK_MANUAL_MASK    0x00080000

#define MTK_CHIP_REV                 0x00006632

#define WIFI_RGU_SW_SYNC0            (TOP_RGU_BASE + 0x1250)
#define WIFI_RGU_SYNC0_RDY_OFFSET        (0)
#define MCU_CFG_PCIE_REMAP1          (MCU_CFG_BASE + 0x0500)
#define PCIE_REMAP1_OFFSET           (18)
#define PCIE_REMAP1_MASK             (BITS(18, 31))
#define PCIE_REMAP1_BUS_ADDR             (0x40000)
#define MCU_CFG_PCIE_REMAP2          (MCU_CFG_BASE + 0x0504)
#define PCIE_REMAP2_OFFSET           (19)
#define PCIE_REMAP2_MASK             (BITS(19, 31))
#define PCIE_REMAP2_BUS_ADDR             (0x80000)

/* UMAC Register */
#define UMAC_PLE_CR_CFG_BASE_ADDR        0x82060000
#define UMAC_PSE_CR_CFG_BASE_ADDR        0x82068000

#define UMAC_PSE_PLE_CR_ADDR_DIFF \
    (UMAC_PSE_CR_CFG_BASE_ADDR - UMAC_PLE_CR_CFG_BASE_ADDR)
#define UMAC_PSE_CR_BITMAP_OFFSET        15
#define UMAC_PSE_PLE_ADDR_DIFF_MAR(_x)    (_x << UMAC_PSE_CR_BITMAP_OFFSET)

#define UMAC_PLE_BASE_ADDRESS            (0xa << 28)

#define UMAC_PSE_BASE_ADDRESS            (0xb << 28)

#define UMAC_FID_SHIFT_16_BIT_VM_MAP         16

#define UMAC_BASE(_x) \
    (UMAC_PLE_CR_CFG_BASE_ADDR | (_x << UMAC_PSE_CR_BITMAP_OFFSET))

#define UMAC_RESET(_x)          (UMAC_BASE(_x) + 0x00000000)

#define UMAC_INT_CR4_EN_MASK(_x)    (UMAC_BASE(_x) + 0x00000004)
#define UMAC_INT_CR4_STS(_x)        (UMAC_BASE(_x) + 0x00000008)
#define UMAC_INT_CR4_ERR_RES(_x)    (UMAC_BASE(_x) + 0x0000000C)
#define UMAC_INT_CR4_ERR_MASK(_x)   (UMAC_BASE(_x) + 0x00000010)

#define UMAC_PBUF_CTRL(_x)      (UMAC_BASE(_x) + 0x00000014)

#define UMAC_CHIP_ID_VER(_x)        (UMAC_BASE(_x) + 0x00000018)

#define UMAC_TIMER_CNF(_x)      (UMAC_BASE(_x) + 0x0000001C)

#define UMAC_INT_N9_EN_MASK(_x)     (UMAC_BASE(_x) + 0x00000020)
#define UMAC_INT_N9_STS(_x)     (UMAC_BASE(_x) + 0x00000024)
#define UMAC_INT_N9_ERR_STS(_x)     (UMAC_BASE(_x) + 0x00000028)
#define UMAC_INT_N9_ERR_MASK(_x)    (UMAC_BASE(_x) + 0x0000002C)
#define UMAC_IGNORE_BUSY_EN_MASK(_x)    (UMAC_BASE(_x) + 0x0000038C)

#define UMAC_RELEASE_CTRL(_x)       (UMAC_BASE(_x) + 0x00000030)

#define UMAC_HIF_REPROT(_x)     (UMAC_BASE(_x) + 0x00000034)

#define UMAC_C_GET_FID_0(_x)        (UMAC_BASE(_x) + 0x00000040)
#define UMAC_C_GET_FID_1(_x)        (UMAC_BASE(_x) + 0x00000044)

#define UMAC_C_EN_QUEUE_0(_x)       (UMAC_BASE(_x) + 0x00000060)
#define UMAC_C_EN_QUEUE_1(_x)       (UMAC_BASE(_x) + 0x00000064)
#define UMAC_C_EN_QUEUE_2(_x)       (UMAC_BASE(_x) + 0x00000068)

#define UMAC_C_DE_QUEUE_0(_x)       (UMAC_BASE(_x) + 0x00000080)
#define UMAC_C_DE_QUEUE_1(_x)       (UMAC_BASE(_x) + 0x00000084)
#define UMAC_C_DE_QUEUE_2(_x)       (UMAC_BASE(_x) + 0x00000088)
#define UMAC_C_DE_QUEUE_3(_x)       (UMAC_BASE(_x) + 0x0000008C)

#define UMAC_ALLOCATE_0(_x)     (UMAC_BASE(_x) + 0x000000A0)
#define UMAC_ALLOCATE_1(_x)     (UMAC_BASE(_x) + 0x000000A4)
#define UMAC_ALLOCATE_2(_x)     (UMAC_BASE(_x) + 0x000000A8)

#define UMAC_QUEUE_EMPTY(_x)        (UMAC_BASE(_x) + 0x000000B0)

/* 0x820680B4 QUEUE_EMPTY_MAS
 * Queue empty status mask register, 7615 E3 eco
 * item only PSE with this setting, but ple
 */

#define UMAC_QUEUE_EMPTY_MASK    (UMAC_BASE(UMAC_PSE_CFG_POOL_INDEX) + \
                                  0x000000B4)

#define UMAC_TO_CR4_INT(_x)     (UMAC_BASE(_x) + 0x000000e0)
#define UMAC_TO_N9_INT(_x)      (UMAC_BASE(_x) + 0x000000f0)

#define UMAC_FREEPG_CNT(_x)     (UMAC_BASE(_x) + 0x00000100)

#define UMAC_FREEPG_HEAD_TAIL(_x)   (UMAC_BASE(_x) + 0x00000104)

#define UMAC_PG_HIF0_GROUP(_x)      (UMAC_BASE(_x) + 0x00000110)
#define UMAC_HIF0_PG_INFO(_x)       (UMAC_BASE(_x) + 0x00000114)

#define UMAC_PG_HIF1_GROUP(_x)      (UMAC_BASE(_x) + 0x00000118)
#define UMAC_HIF1_PG_INFO(_x)       (UMAC_BASE(_x) + 0x0000011C)

#define UMAC_PG_CPU_GROUP(_x)       (UMAC_BASE(_x) + 0x00000150)
#define UMAC_CPU_PG_INFO(_x)        (UMAC_BASE(_x) + 0x00000154)

#define UMAC_PG_LMAC0_GROUP(_x)     (UMAC_BASE(_x) + 0x00000170)
#define UMAC_LMAC0_PG_INFO(_x)      (UMAC_BASE(_x) + 0x00000174)

#define UMAC_PG_LMAC1_GROUP(_x)     (UMAC_BASE(_x) + 0x00000178)
#define UMAC_LMAC1_PG_INFO(_x)      (UMAC_BASE(_x) + 0x0000017C)

#define UMAC_PG_LMAC2_GROUP(_x)     (UMAC_BASE(_x) + 0x00000180)
#define UMAC_LMAC2_PG_INFO(_x)      (UMAC_BASE(_x) + 0x00000184)

#define UMAC_PG_PLE_GROUP(_x)       (UMAC_BASE(_x) + 0x00000190)
#define UMAC_PLE_PG_INFO(_x)        (UMAC_BASE(_x) + 0x00000194)

#define UMAC_RL_BUF_CTRL_0(_x)      (UMAC_BASE(_x) + 0x000001A0)
#define UMAC_RL_BUF_CTRL_1(_x)      (UMAC_BASE(_x) + 0x000001A4)

#define UMAC_FL_QUE_CTRL_0(_x)      (UMAC_BASE(_x) + 0x000001B0)
#define UMAC_FL_QUE_CTRL_1(_x)      (UMAC_BASE(_x) + 0x000001B4)
#define UMAC_FL_QUE_CTRL_2(_x)      (UMAC_BASE(_x) + 0x000001B8)
#define UMAC_FL_QUE_CTRL_3(_x)      (UMAC_BASE(_x) + 0x000001BC)

#define UMAC_HIF_ENQ_PKT_NUM(_x)    (UMAC_BASE(_x) + 0x000001F0)
#define UMAC_CPU_ENQ_PKT_NUM(_x)    (UMAC_BASE(_x) + 0x000001F4)
#define UMAC_RLS_MSDU_PKT_NUM(_x)   (UMAC_BASE(_x) + 0x000001F8)
#define UMAC_HOST_REPORT_NUM(_x)    (UMAC_BASE(_x) + 0x000001FC)

#define UMAC_PL_QUE_CTRL_0(_x)      (UMAC_BASE(_x) + 0x000001C0)

#define UMAC_TP_HIF_EN(_x)      (UMAC_BASE(_x) + 0x00000200)

#define UMAC_TP_HIF_Q_CTRL0(_x)     (UMAC_BASE(_x) + 0x00000204)
#define UMAC_TP_HIF_Q_CTRL1(_x)     (UMAC_BASE(_x) + 0x00000208)
#define UMAC_TP_HIF_Q_CTRL2(_x)     (UMAC_BASE(_x) + 0x0000020C)

#define UMAC_TP_HIF_ALLOC0(_x)      (UMAC_BASE(_x) + 0x00000210)
#define UMAC_TP_HIF_ALLOC1(_x)      (UMAC_BASE(_x) + 0x00000214)

#define UMAC_TP_HIF_OPER0(_x)       (UMAC_BASE(_x) + 0x00000218)
#define UMAC_TP_HIF_OPER1(_x)       (UMAC_BASE(_x) + 0x0000021C)

#define UMAC_TP_LMAC_EN(_x)     (UMAC_BASE(_x) + 0x00000220)

#define UMAC_TP_LMAC_Q_CTRL0(_x)    (UMAC_BASE(_x) + 0x00000224)
#define UMAC_TP_LMAC_Q_CTRL1(_x)    (UMAC_BASE(_x) + 0x00000228)
#define UMAC_TP_LMAC_Q_CTRL2(_x)    (UMAC_BASE(_x) + 0x0000022C)

#define UMAC_TP_LMAC_ALLOC0(_x)     (UMAC_BASE(_x) + 0x00000230)
#define UMAC_TP_LMAC_ALLOC1(_x)     (UMAC_BASE(_x) + 0x00000234)

#define UMAC_TP_LMAC_OPER0(_x)      (UMAC_BASE(_x) + 0x00000238)
#define UMAC_TP_LMAC_OPER1(_x)      (UMAC_BASE(_x) + 0x0000023C)

#define UMAC_STATE_IDLE_CTL(_x)     (UMAC_BASE(_x) + 0x0000024C)

#define UMAC_DIS_STA_MAP0(_x)       (UMAC_BASE(_x) + 0x00000260)
#define UMAC_DIS_STA_MAP1(_x)       (UMAC_BASE(_x) + 0x00000264)
#define UMAC_DIS_STA_MAP2(_x)       (UMAC_BASE(_x) + 0x00000268)
#define UMAC_DIS_STA_MAP3(_x)       (UMAC_BASE(_x) + 0x0000026c)

#define UMAC_AC0_QUEUE_EMPTY0(_x)   (UMAC_BASE(_x) + 0x00000300)
#define UMAC_AC0_QUEUE_EMPTY1(_x)   (UMAC_BASE(_x) + 0x00000304)
#define UMAC_AC0_QUEUE_EMPTY2(_x)   (UMAC_BASE(_x) + 0x00000308)
#define UMAC_AC0_QUEUE_EMPTY3(_x)   (UMAC_BASE(_x) + 0x0000030C)

#define UMAC_AC1_QUEUE_EMPTY0(_x)   (UMAC_BASE(_x) + 0x00000310)
#define UMAC_AC1_QUEUE_EMPTY1(_x)   (UMAC_BASE(_x) + 0x00000314)
#define UMAC_AC1_QUEUE_EMPTY2(_x)   (UMAC_BASE(_x) + 0x00000318)
#define UMAC_AC1_QUEUE_EMPTY3(_x)   (UMAC_BASE(_x) + 0x0000031C)

#define UMAC_AC2_QUEUE_EMPTY0(_x)   (UMAC_BASE(_x) + 0x00000320)
#define UMAC_AC2_QUEUE_EMPTY1(_x)   (UMAC_BASE(_x) + 0x00000324)
#define UMAC_AC2_QUEUE_EMPTY2(_x)   (UMAC_BASE(_x) + 0x00000328)
#define UMAC_AC2_QUEUE_EMPTY3(_x)   (UMAC_BASE(_x) + 0x0000032C)

#define UMAC_AC3_QUEUE_EMPTY0(_x)   (UMAC_BASE(_x) + 0x00000330)
#define UMAC_AC3_QUEUE_EMPTY1(_x)   (UMAC_BASE(_x) + 0x00000334)
#define UMAC_AC3_QUEUE_EMPTY2(_x)   (UMAC_BASE(_x) + 0x00000338)
#define UMAC_AC3_QUEUE_EMPTY3(_x)   (UMAC_BASE(_x) + 0x0000033C)

#define UMAC_QUEUE_EMPTY_MASK        (UMAC_BASE(UMAC_PSE_CFG_POOL_INDEX) + \
                                      0x000000B4)

/* BSS PS INT */
#define UMAC_N9_BSS_PS_INT_EN        (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x000000F4)
#define UMAC_N9_BSS_PS_INT_STS       (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x000000F8)

/* CR for VoW and BW Ctrl */

#define UMAC_DRR_TABLE_CTRL0         (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000388)

#define UMAC_DRR_TABLE_WDATA0        (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000340)
#define UMAC_DRR_TABLE_WDATA1        (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000344)
#define UMAC_DRR_TABLE_WDATA2        (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000348)
#define UMAC_DRR_TABLE_WDATA3        (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x0000034C)

#define UMAC_DRR_TABLE_RDATA0        (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000350)
#define UMAC_DRR_TABLE_RDATA1        (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000354)
#define UMAC_DRR_TABLE_RDATA2        (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000358)
#define UMAC_DRR_TABLE_RDATA3        (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x0000035C)

#define UMAC_STATION_PAUSE_0         (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000360)
#define UMAC_STATION_PAUSE_1         (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000364)
#define UMAC_STATION_PAUSE_2         (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000368)
#define UMAC_STATION_PAUSE_3         (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x0000036C)
#define UMAC_VOW_ENABLE          (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                  0x00000370)
#define UMAC_AIR_TIME_DRR_SIZE       (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000374)
#define UMAC_CHECK_TIME_TOKEN        (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x00000378)
#define UMAC_CHECK_LENGTH_TOKEN \
    (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + 0x0000037C)
#define UMAC_WDRR0           (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                              0x00000380)
#define UMAC_WDRR1           (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                              0x00000384)

#define UMAC_VOW_CTRL1           (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                  0x0000038C)

#define UMAC_VOW_DBG_MUX         (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                  0x000003A0)
#define UMAC_AIRTIME_DBG_INFO0       (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x000003A4)
#define UMAC_AIRTIME_DBG_INFO1       (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                      0x000003A8)

#define UMAC_BW_DBG_INFO         (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                  0x000003AC)

#define UMAC_BW_WDRR0            (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                  0x000003B0)
#define UMAC_BW_WDRR1            (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                  0x000003B4)
#define UMAC_BW_WDRR2            (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                  0x000003B8)
#define UMAC_BW_WDRR3            (UMAC_BASE(UMAC_PLE_CFG_POOL_INDEX) + \
                                  0x000003BC)
/* UMAC Register */

#define WIFI_CFG_SW_SYNC0        0
#define WIFI_CFG_SYNC0_RDY_OFFSET    0

/* 4 CHIP ID Register */
#define MCR_WCIR             0x0000

/* 4 HIF Low Power Control  Register */
#define MCR_WHLPCR           0x0004

/* 4 Control  Status Register */
#define MCR_WSDIOCSR             0x0008

/* 4 HIF Control Register */
#define MCR_WHCR             0x000C

/* 4 HIF Interrupt Status  Register */
#define MCR_WHISR            0x0010

/* 4 HIF Interrupt Enable  Register */
#define MCR_WHIER            0x0014

/* 4 Abnormal Status Register */
#define MCR_WASR             0x0020

/* 4 WLAN Software Interrupt Control Register */
#define MCR_WSICR            0x0024

/* 4 WLAN TX Data Register 1 */
#define MCR_WTDR1            0x0034

/* 4 WLAN RX Data Register 0 */
#define MCR_WRDR0            0x0050

/* 4 WLAN RX Data Register 1 */
#define MCR_WRDR1            0x0054

/* 4 Host to Device Send Mailbox 0 Register */
#define MCR_H2DSM0R          0x0070

/* 4 Host to Device Send Mailbox 1 Register */
#define MCR_H2DSM1R          0x0074

/* 4 Device to Host Receive Mailbox 0 Register */
#define MCR_D2HRM0R          0x0078

/* 4 Device to Host Receive Mailbox 1 Register */
#define MCR_D2HRM1R          0x007c

/* 4 WLAN RX Packet Length Register */
#define MCR_WRPLR            0x0090

/* 4 Test Mode Data Port */
#define MCR_WTMDR            0x00b0

/* 4 Test Mode Control Register */
#define MCR_WTMCR            0x00b4

/* 4 Test Mode Data Pattern Control Register #0 */
#define MCR_WTMDPCR0             0x00b8

/* 4 Test Mode Data Pattern Control Register #1 */
#define MCR_WTMDPCR1             0x00bc

/* 4 WLAN Packet Length Report Control Register */
#define MCR_WPLRCR           0x00d4

/* 4 WLAN Snapshot Register */
#define MCR_WSR              0x00D8

/* 4 Clock Pad Macro IO Control Register */
#define MCR_CLKIOCR          0x0100

/* 4 Command Pad Macro IO Control Register */
#define MCR_CMDIOCR          0x0104

/* 4 Data 0 Pad Macro IO Control Register */
#define MCR_DAT0IOCR             0x0108

/* 4 Data 1 Pad Macro IO Control Register */
#define MCR_DAT1IOCR             0x010C

/* 4 Data 2 Pad Macro IO Control Register */
#define MCR_DAT2IOCR             0x0110

/* 4 Data 3 Pad Macro IO Control Register */
#define MCR_DAT3IOCR             0x0114

/* 4 Clock Pad Macro Delay Chain Control Register */
#define MCR_CLKDLYCR             0x0118

/* 4 Command Pad Macro Delay Chain Control Register */
#define MCR_CMDDLYCR             0x011C

/* 4 SDIO Output Data Delay Chain Control Register */
#define MCR_ODATDLYCR            0x0120

/* 4 SDIO Input Data Delay Chain Control Register 1 */
#define MCR_IDATDLYCR1           0x0124

/* 4 SDIO Input Data Delay Chain Control Register 2 */
#define MCR_IDATDLYCR2           0x0128

/* 4 SDIO Input Data Latch Time Control Register */
#define MCR_ILCHCR           0x012C

/* 4 WLAN TXQ Count Register 0 */
#define MCR_WTQCR0           0x0130

/* 4 WLAN TXQ Count Register 1 */
#define MCR_WTQCR1           0x0134

/* 4 WLAN TXQ Count Register 2 */
#define MCR_WTQCR2           0x0138

/* 4 WLAN TXQ Count Register 3 */
#define MCR_WTQCR3           0x013C

/* 4 WLAN TXQ Count Register 4 */
#define MCR_WTQCR4           0x0140

/* 4 WLAN TXQ Count Register 5 */
#define MCR_WTQCR5           0x0144

/* 4 WLAN TXQ Count Register 6 */
#define MCR_WTQCR6           0x0148

/* 4 WLAN TXQ Count Register 7 */
#define MCR_WTQCR7           0x014C

/* WLAN/Common PC value Debug registre */
#define MCR_SWPCDBGR             0x0154

/* #if CFG_SDIO_INTR_ENHANCE */
typedef struct _ENHANCE_MODE_DATA_STRUCT_T {
    u32 u4WHISR;
    union {
        struct {
            u16 u2TQ0Cnt;
            u16 u2TQ1Cnt;
            u16 u2TQ2Cnt;
            u16 u2TQ3Cnt;
            u16 u2TQ4Cnt;
            u16 u2TQ5Cnt;
            u16 u2TQ6Cnt;
            u16 u2TQ7Cnt;
            u16 u2TQ8Cnt;
            u16 u2TQ9Cnt;
            u16 u2TQ10Cnt;
            u16 u2TQ11Cnt;
            u16 u2TQ12Cnt;
            u16 u2TQ13Cnt;
            u16 u2TQ14Cnt;
            u16 u2TQ15Cnt;
        } u;
        u32 au4WTSR[8];
    } rTxInfo;
    union {
        struct {
            u16 u2NumValidRx0Len;
            u16 u2NumValidRx1Len;
            u16 au2Rx0Len[16];
            u16 au2Rx1Len[16];
        } u;
        u32 au4RxStatusRaw[17];
    } rRxInfo;
    u32 u4RcvMailbox0;
    u32 u4RcvMailbox1;
} ENHANCE_MODE_DATA_STRUCT_T, *P_ENHANCE_MODE_DATA_STRUCT_T;
/* #endif */ /* ENHANCE_MODE_DATA_STRUCT_T */

/* 2 Definition in each register */
/* 3 WCIR 0x0000 */
#define WCIR_WLAN_READY           BIT(21)
#define WCIR_POR_INDICATOR        BIT(20)
#define WCIR_REVISION_ID          BITS(16, 19)
#define WCIR_CHIP_ID              BITS(0, 15)

#define MTK_CHIP_REV              0x00006632
#define MTK_CHIP_MP_REVERSION_ID      0x0

/* 3 WHLPCR 0x0004 */
#define WHLPCR_FW_OWN_REQ_CLR         BIT(9)
#define WHLPCR_FW_OWN_REQ_SET         BIT(8)
#define WHLPCR_IS_DRIVER_OWN          BIT(8)
#define WHLPCR_INT_EN_CLR         BIT(1)
#define WHLPCR_INT_EN_SET         BIT(0)

/* 3 WSDIOCSR 0x0008 */
#define WSDIOCSR_DB_CMD7_RESELECT_DIS     BIT(4)
#define WSDIOCSR_DB_WR_BUSY_EN        BIT(3)
#define WSDIOCSR_DB_RD_BUSY_EN        BIT(2)
#define WSDIOCSR_SDIO_INT_CTL         BIT(1)
#define WSDIOCSR_SDIO_RE_INIT_EN      BIT(0)

/* 3 WHCR 0x000C */
#define WHCR_RX_ENHANCE_MODE_EN       BIT(16)
#define WHCR_MAX_HIF_RX_LEN_NUM       BITS(8, 13)
#define WHCR_RPT_OWN_RX_PACKET_LEN    BIT(3)
#define WHCR_RECV_MAILBOX_RD_CLR_EN   BIT(2)
#define WHCR_W_INT_CLR_CTRL       BIT(1)
#define WHCR_MCU_DBG_EN           BIT(0)
#define WHCR_OFFSET_MAX_HIF_RX_LEN_NUM    8

/* 3 WHISR 0x0010 */
#define WHISR_D2H_SW_INT          BITS(8, 31)
#define WHISR_D2H_SW_ASSERT_INFO_INT      BIT(31)
#define WHISR_D2H_WKUP_BY_RX_PACKET   BIT(30)
#define WHISR_D2H_SW_RD_MAILBOX_INT   BIT(29)
#define WHISR_FW_OWN_BACK_INT         BIT(7)
#define WHISR_ABNORMAL_INT        BIT(6)
#define WHISR_RX1_DONE_INT        BIT(2)
#define WHISR_RX0_DONE_INT        BIT(1)
#define WHISR_TX_DONE_INT         BIT(0)

/* 3 WHIER 0x0014 */
#define WHIER_D2H_SW_INT          BITS(8, 31)
#define WHIER_FW_OWN_BACK_INT_EN      BIT(7)
#define WHIER_ABNORMAL_INT_EN         BIT(6)
#define WHIER_RX1_DONE_INT_EN         BIT(2)
#define WHIER_RX0_DONE_INT_EN         BIT(1)
#define WHIER_TX_DONE_INT_EN          BIT(0)
#define WHIER_DEFAULT                                \
    (WHIER_RX0_DONE_INT_EN | WHIER_RX1_DONE_INT_EN | \
     WHIER_TX_DONE_INT_EN | WHIER_ABNORMAL_INT_EN | WHIER_D2H_SW_INT)

/* 3 WASR 0x0020 */
#define WASR_FW_OWN_INVALID_ACCESS    BIT(16)
#define WASR_RX1_UNDER_FLOW       BIT(9)
#define WASR_RX0_UNDER_FLOW       BIT(8)
#define WASR_TX1_OVER_FLOW        BIT(1)

/* 3 WSICR 0x0024 */
#define WSICR_H2D_SW_INT_SET          BITS(16, 31)

/* 3 WRPLR 0x0090 */
#define WRPLR_RX1_PACKET_LENGTH       BITS(16, 31)
#define WRPLR_RX0_PACKET_LENGTH       BITS(0, 15)

/* 3 WTMCR 0x00b4 */
#define WMTCR_TEST_MODE_FW_OWN        BIT(24)
#define WMTCR_PRBS_INIT_VAL       BITS(16, 23)
#define WMTCR_TEST_MODE_STATUS        BIT(8)
#define WMTCR_TEST_MODE_SELECT        BITS(0, 1)

/* Support features */
/* Options for VLAN-over-ethernet pkt to/from 802.11 LLC VLAN pkt.
 * This should depend on the configurations of HW-header-translation.
 */
#define FEAT_BITS_LLC_VLAN_TX         BIT(0)
#define FEAT_BITS_LLC_VLAN_RX         BIT(1)

/* Support features API */
#define FEAT_SUP_LLC_VLAN_TX(__chip_info) \
    ((__chip_info)->features & FEAT_BITS_LLC_VLAN_TX)
#define FEAT_SUP_LLC_VLAN_RX(__chip_info) \
    ((__chip_info)->features & FEAT_BITS_LLC_VLAN_RX)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

typedef enum _ENUM_WIFI_FUNC_T {
    WIFI_FUNC_INIT_DONE  = BIT(0),
    WIFI_FUNC_N9_DONE    = BIT(1),
    WIFI_FUNC_CR4_READY  = BIT(2),
    WIFI_FUNC_READY_BITS = BITS(0, 2),
    WIFI_FUNC_DUMMY_REQ  = BIT(3)
} ENUM_WIFI_FUNC_T;

enum enum_chip {
    CHIP_6632 = 0,
    CHIP_7666,
    CHIP_7668,
    CHIP_NUM
};

struct chip_info {
    const unsigned int chip_id;  /* chip id */
    const unsigned int sw_sync0;  /* sw_sync0 address */
    const unsigned int sw_ready_bit_offset;  /* sw_sync0 ready bit offset */
    const unsigned int patch_addr;  /* patch download start address */
    const unsigned char is_pcie_32dw_read;  /* diff PDMA config */

    const P_ECO_INFO_T eco_info;  /* chip version table */
    u8 eco_ver;  /* chip version */

    void (*constructFirmwarePrio)(P_GLUE_INFO_T prGlueInfo,
                                  u8 **apucNameTable,
                                  u8 **apucName,
                                  u8 *pucNameIdx,
                                  u8 ucMaxNameIdx);

    const u32 features;  /* feature bits */
};

struct hif_driver_data {
    struct chip_info *chip_info;
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
