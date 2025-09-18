/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_qa_agent.h
 *    \brief  This file includes private ioctl support.
 */

#ifndef _GL_QA_AGENT_H
#define _GL_QA_AGENT_H
#if CFG_SUPPORT_QA_TOOL
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

#define HQA_CMD_MAGIC_NO    0x18142880
#define HQA_CHIP_ID_6632    0x6632
#define HQA_CHIP_ID_7668    0x7668

#if CFG_SUPPORT_TX_BF
#define HQA_BF_STR_SIZE     512
#endif

#define HQA_RX_STATISTIC_NUM    66
#define BUFFER_BIN_MODE     0x0
#define EFUSE_MODE      0x2

extern u8 uacEEPROMImage[MAX_EEPROM_BUFFER_SIZE];

typedef struct _PARAM_RX_STAT_T {
    u32 MAC_FCS_Err;  /* b0 */
    u32 MAC_Mdrdy;  /* b0 */
    u32 FCSErr_CCK;
    u32 FCSErr_OFDM;
    u32 CCK_PD;
    u32 OFDM_PD;
    u32 CCK_SIG_Err;
    u32 CCK_SFD_Err;
    u32 OFDM_SIG_Err;
    u32 OFDM_TAG_Err;
    u32 WB_RSSI0;
    u32 IB_RSSI0;
    u32 WB_RSSI1;
    u32 IB_RSSI1;
    u32 PhyMdrdyCCK;
    u32 PhyMdrdyOFDM;
    u32 DriverRxCount;
    u32 RCPI0;
    u32 RCPI1;
    u32 FreqOffsetFromRX;
    u32 RSSI0;
    u32 RSSI1;  /* insert new member here */
    u32 OutOfResource;  /* MT7615 begin here */
    u32 LengthMismatchCount_B0;
    u32 MAC_FCS_Err1;  /* b1 */
    u32 MAC_Mdrdy1;  /* b1 */
    u32 FAGCRssiIBR0;
    u32 FAGCRssiIBR1;
    u32 FAGCRssiIBR2;
    u32 FAGCRssiIBR3;
    u32 FAGCRssiWBR0;
    u32 FAGCRssiWBR1;
    u32 FAGCRssiWBR2;
    u32 FAGCRssiWBR3;

    u32 InstRssiIBR0;
    u32 InstRssiIBR1;
    u32 InstRssiIBR2;
    u32 InstRssiIBR3;
    u32 InstRssiWBR0;
    u32 InstRssiWBR1;
    u32 InstRssiWBR2;
    u32 InstRssiWBR3;
    u32 ACIHitLower;
    u32 ACIHitUpper;
    u32 DriverRxCount1;
    u32 RCPI2;
    u32 RCPI3;
    u32 RSSI2;
    u32 RSSI3;
    u32 SNR0;
    u32 SNR1;
    u32 SNR2;
    u32 SNR3;
    u32 OutOfResource1;
    u32 LengthMismatchCount_B1;
    u32 CCK_PD_Band1;
    u32 OFDM_PD_Band1;
    u32 CCK_SIG_Err_Band1;
    u32 CCK_SFD_Err_Band1;
    u32 OFDM_SIG_Err_Band1;
    u32 OFDM_TAG_Err_Band1;
    u32 PHY_CCK_MDRDY_Band1;
    u32 PHY_OFDM_MDRDY_Band1;
    u32 CCK_FCS_Err_Band1;
    u32 OFDM_FCS_Err_Band1;
    u32 MRURxCount;
    u32 SIGMCS;
    u32 SINR;
    u32 RXVRSSI;
    u32 Reserved[184];
    u32 PHY_Mdrdy;
    u32 Noise_Floor;
    u32 AllLengthMismatchCount_B0;
    u32 AllLengthMismatchCount_B1;
    u32 AllMacMdrdy0;
    u32 AllMacMdrdy1;
    u32 AllFCSErr0;
    u32 AllFCSErr1;
    u32 RXOK0;
    u32 RXOK1;
    u32 PER0;
    u32 PER1;
} PARAM_RX_STAT_T, *P_PARAM_RX_STAT_T;
extern PARAM_RX_STAT_T g_HqaRxStat;

typedef struct _HQA_CMD_FRAME {
    u32 MagicNo;
    u16 Type;
    u16 Id;
    u16 Length;
    u16 Sequence;
    u8 Data[2000];
} __packed HQA_CMD_FRAME;

typedef s32 (*HQA_CMD_HANDLER)(struct net_device *prNetDev,
                               IN union iwreq_data *prIwReqData,
                               HQA_CMD_FRAME *HqaCmdFrame);

typedef struct _HQA_CMD_TABLE {
    HQA_CMD_HANDLER *CmdSet;
    u32 CmdSetSize;
    u32 CmdOffset;
} HQA_CMD_TABLE;

int HQA_CMDHandler(struct net_device *prNetDev,
                   IN union iwreq_data *prIwReqData,
                   HQA_CMD_FRAME *HqaCmdFrame);

int priv_qa_agent(IN struct net_device *prNetDev,
                  IN struct iw_request_info *prIwReqInfo,
                  IN union iwreq_data *prIwReqData,
                  IN char *pcExtra);

int priv_set_eeprom_mode(IN u32 u4Mode);
#endif
#endif
