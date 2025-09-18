/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "swcr.h"
 *    \brief
 */

/*
 *
 */

#ifndef _SWCR_H
#define _SWCR_H

#include "nic_cmd_event.h"

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

#define TEST_PS        1

#define SWCR_VAR(x) ((void *)&x)
#define SWCR_FUNC(x)    ((void *)x)

#define SWCR_T_FUNC    BIT(7)

#define SWCR_L_32      3
#define SWCR_L_16      2
#define SWCR_L_8       1

#define SWCR_READ      0
#define SWCR_WRITE     1

#define SWCR_MAP_NUM(x)    (ARRAY_SIZE(x))

#define SWCR_CR_NUM    7

#define SWCR_GET_RW_INDEX(action, rw, index) \
    do {                                     \
        index = action & 0x7F;               \
        rw = action >> 7;                    \
    } while (0)

extern u32 g_au4SwCr[];  /*: 0: command other: data */

typedef void (*PFN_SWCR_RW_T)(P_ADAPTER_T prAdapter,
                              u8 ucRead,
                              u16 u2Addr,
                              u32 *pu4Data);
typedef void (*PFN_CMD_RW_T)(P_ADAPTER_T prAdapter,
                             u8 ucCate,
                             u8 ucAction,
                             u8 ucOpt0,
                             u8 ucOpt1);

typedef struct _SWCR_MAP_ENTRY_T {
    u16 u2Type;
    void *u4Addr;
} SWCR_MAP_ENTRY_T, *P_SWCR_MAP_ENTRY_T;

typedef struct _SWCR_MOD_MAP_ENTRY_T {
    u8 ucMapNum;
    P_SWCR_MAP_ENTRY_T prSwCrMap;
} SWCR_MOD_MAP_ENTRY_T, *P_SWCR_MOD_MAP_ENTRY_T;

typedef enum _ENUM_SWCR_DBG_TYPE_T {
    SWCR_DBG_TYPE_ALL = 0,
    SWCR_DBG_TYPE_TXRX,
    SWCR_DBG_TYPE_RX_RATES,
    SWCR_DBG_TYPE_PS,
    SWCR_DBG_TYPE_NUM
} ENUM_SWCR_DBG_TYPE_T;

typedef enum _ENUM_SWCR_DBG_ALL_T {
    SWCR_DBG_ALL_TX_CNT = 0,
    SWCR_DBG_ALL_TX_BCN_CNT,
    SWCR_DBG_ALL_TX_FAILED_CNT,
    SWCR_DBG_ALL_TX_RETRY_CNT,
    SWCR_DBG_ALL_TX_AGING_TIMEOUT_CNT,
    SWCR_DBG_ALL_TX_PS_OVERFLOW_CNT,
    SWCR_DBG_ALL_TX_MGNT_DROP_CNT,
    SWCR_DBG_ALL_TX_ERROR_CNT,

    SWCR_DBG_ALL_RX_CNT,
    SWCR_DBG_ALL_RX_DROP_CNT,
    SWCR_DBG_ALL_RX_DUP_DROP_CNT,
    SWCR_DBG_ALL_RX_TYPE_ERROR_DROP_CNT,
    SWCR_DBG_ALL_RX_CLASS_ERROR_DROP_CNT,
    SWCR_DBG_ALL_RX_AMPDU_ERROR_DROP_CNT,

    SWCR_DBG_ALL_RX_STATUS_ERROR_DROP_CNT,
    SWCR_DBG_ALL_RX_FORMAT_ERROR_DROP_CNT,
    SWCR_DBG_ALL_RX_ICV_ERROR_DROP_CNT,
    SWCR_DBG_ALL_RX_KEY_ERROR_DROP_CNT,
    SWCR_DBG_ALL_RX_TKIP_ERROR_DROP_CNT,
    SWCR_DBG_ALL_RX_MIC_ERROR_DROP_CNT,
    SWCR_DBG_ALL_RX_BIP_ERROR_DROP_CNT,

    SWCR_DBG_ALL_RX_FCSERR_CNT,
    SWCR_DBG_ALL_RX_FIFOFULL_CNT,
    SWCR_DBG_ALL_RX_PFDROP_CNT,

    SWCR_DBG_ALL_PWR_PS_POLL_CNT,
    SWCR_DBG_ALL_PWR_TRIGGER_NULL_CNT,
    SWCR_DBG_ALL_PWR_BCN_IND_CNT,
    SWCR_DBG_ALL_PWR_BCN_TIMEOUT_CNT,
    SWCR_DBG_ALL_PWR_PM_STATE0,
    SWCR_DBG_ALL_PWR_PM_STATE1,
    SWCR_DBG_ALL_PWR_CUR_PS_PROF0,
    SWCR_DBG_ALL_PWR_CUR_PS_PROF1,

    SWCR_DBG_ALL_AR_STA0_RATE,
    SWCR_DBG_ALL_AR_STA0_BWGI,
    SWCR_DBG_ALL_AR_STA0_RX_RATE_RCPI,

    SWCR_DBG_ALL_ROAMING_ENABLE,
    SWCR_DBG_ALL_ROAMING_ROAM_CNT,
    SWCR_DBG_ALL_ROAMING_INT_CNT,

    SWCR_DBG_ALL_BB_RX_MDRDY_CNT,
    SWCR_DBG_ALL_BB_RX_FCSERR_CNT,
    SWCR_DBG_ALL_BB_CCK_PD_CNT,
    SWCR_DBG_ALL_BB_OFDM_PD_CNT,
    SWCR_DBG_ALL_BB_CCK_SFDERR_CNT,
    SWCR_DBG_ALL_BB_CCK_SIGERR_CNT,
    SWCR_DBG_ALL_BB_OFDM_TAGERR_CNT,
    SWCR_DBG_ALL_BB_OFDM_SIGERR_CNT,

    SWCR_DBG_ALL_NUM
} ENUM_SWCR_DBG_ALL_T;

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

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

void swCrReadWriteCmd(P_ADAPTER_T prAdapter, u8 ucRead, u16 u2Addr,
                      u32 *pu4Data);

/* Debug Support */
void swCrFrameCheckEnable(P_ADAPTER_T prAdapter, u32 u4DumpType);
void swCrDebugInit(P_ADAPTER_T prAdapter);
void swCrDebugCheckEnable(P_ADAPTER_T prAdapter,
                          u8 fgIsEnable,
                          u8 ucType,
                          u32 u4Timeout);
void swCrDebugUninit(P_ADAPTER_T prAdapter);

#if CFG_SUPPORT_SWCR
void swCtrlCmdCategory0(P_ADAPTER_T prAdapter,
                        u8 ucCate,
                        u8 ucAction,
                        u8 ucOpt0,
                        u8 ucOpt1);
void swCtrlCmdCategory1(P_ADAPTER_T prAdapter,
                        u8 ucCate,
                        u8 ucAction,
                        u8 ucOpt0,
                        u8 ucOpt1);
#if TEST_PS
void testPsCmdCategory0(P_ADAPTER_T prAdapter,
                        u8 ucCate,
                        u8 ucAction,
                        u8 ucOpt0,
                        u8 ucOpt1);
void testPsCmdCategory1(P_ADAPTER_T prAdapter,
                        u8 ucCate,
                        u8 ucAction,
                        u8 ucOpt0,
                        u8 ucOpt1);
#endif
#if CFG_SUPPORT_802_11V
#if (CFG_SUPPORT_802_11V_TIMING_MEASUREMENT == 1) && (WNM_UNIT_TEST == 1)
void testWNMCmdCategory0(P_ADAPTER_T prAdapter,
                         u8 ucCate,
                         u8 ucAction,
                         u8 ucOpt0,
                         u8 ucOpt1);
#endif
#endif
void swCtrlSwCr(P_ADAPTER_T prAdapter, u8 ucRead, u16 u2Addr, u32 *pu4Data);

/* Support Debug */
void swCrDebugCheck(P_ADAPTER_T prAdapter, P_CMD_SW_DBG_CTRL_T prCmdSwCtrl);
void swCrDebugCheckTimeout(IN P_ADAPTER_T prAdapter, unsigned long ulParamPtr);
void swCrDebugQuery(IN P_ADAPTER_T prAdapter,
                    IN P_CMD_INFO_T prCmdInfo,
                    IN u8 *pucEventBuf,
                    IN u32 u4EventBufLen);
void swCrDebugQueryTimeout(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo);
#endif

#endif
