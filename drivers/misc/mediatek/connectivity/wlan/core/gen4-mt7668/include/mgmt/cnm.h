/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "cnm.h"
 *    \brief
 */

#ifndef _CNM_H
#define _CNM_H

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

#if CFG_SUPPORT_DBDC
#define DBDC_5G_WMM_INDEX                0
#define DBDC_2G_WMM_INDEX                1
#endif
#define MAX_HW_WMM_INDEX                 (HW_WMM_NUM - 1)
#define DEFAULT_HW_WMM_INDEX                 (MAX_HW_WMM_INDEX - 1)

/* Define DBDC connection time */
#if CFG_SUPPORT_DBDC
#define DBDC_SWITCH_GUARD_TIME               (4 * 1000)  /*ms*/
#define DBDC_DISABLE_COUNTDOWN_TIME          (2 * 1000)  /*ms*/
#if CFG_SUPPORT_DBDC_TC6
#define DBDC_AIS_BCN_TIMEOUT_RECONNECT_COUNTDOWN_TIME    (10 * 1000)  /*ms*/
#define DBDC_AIS_REASSOC_COUNTDOWN_TIME          (10 * 1000)  /*ms*/
#define DBDC_AIS_CONNECT_COUNTDOWN_TIME          (10 * 1000)  /*ms*/
#define DBDC_AIS_REASSOC_DELAY_DISCONNECT_EVENT_TIME     (15 * 1000)  /*ms*/
#endif
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

typedef enum _ENUM_CH_REQ_TYPE_T {
    CH_REQ_TYPE_JOIN,
    CH_REQ_TYPE_P2P_LISTEN,
    CH_REQ_TYPE_OFFCHNL_TX,
    CH_REQ_TYPE_GO_START_BSS,
#if (CFG_SUPPORT_DFS_MASTER == 1)
    CH_REQ_TYPE_DFS_CAC,
#endif
    CH_REQ_TYPE_NUM
} ENUM_CH_REQ_TYPE_T,
*P_ENUM_CH_REQ_TYPE_T;

typedef struct _MSG_CH_REQ_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucBssIndex;
    u8 ucTokenID;
    u8 ucPrimaryChannel;
    ENUM_CHNL_EXT_T eRfSco;
    ENUM_BAND_T eRfBand;
    ENUM_CHANNEL_WIDTH_T eRfChannelWidth;  /* To support 80/160MHz bandwidth
                                            */
    u8 ucRfCenterFreqSeg1;  /* To support 80/160MHz bandwidth */
    u8 ucRfCenterFreqSeg2;  /* To support 80/160MHz bandwidth */
    ENUM_CH_REQ_TYPE_T eReqType;
    u32 u4MaxInterval;  /* In unit of ms */
    ENUM_DBDC_BN_T eDBDCBand;
} MSG_CH_REQ_T, *P_MSG_CH_REQ_T;

typedef struct _MSG_CH_ABORT_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucBssIndex;
    u8 ucTokenID;
    ENUM_DBDC_BN_T eDBDCBand;
} MSG_CH_ABORT_T, *P_MSG_CH_ABORT_T;

typedef struct _MSG_CH_GRANT_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucBssIndex;
    u8 ucTokenID;
    u8 ucPrimaryChannel;
    ENUM_CHNL_EXT_T eRfSco;
    ENUM_BAND_T eRfBand;
    ENUM_CHANNEL_WIDTH_T eRfChannelWidth;  /* To support 80/160MHz bandwidth
                                            */
    u8 ucRfCenterFreqSeg1;  /* To support 80/160MHz bandwidth */
    u8 ucRfCenterFreqSeg2;  /* To support 80/160MHz bandwidth */
    ENUM_CH_REQ_TYPE_T eReqType;
    u32 u4GrantInterval;  /* In unit of ms */
    ENUM_DBDC_BN_T eDBDCBand;
} MSG_CH_GRANT_T, *P_MSG_CH_GRANT_T;

typedef struct _MSG_CH_REOCVER_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucBssIndex;
    u8 ucTokenID;
    u8 ucPrimaryChannel;
    ENUM_CHNL_EXT_T eRfSco;
    ENUM_BAND_T eRfBand;
    ENUM_CHANNEL_WIDTH_T eRfChannelWidth;  /* To support 80/160MHz bandwidth
                                            */
    u8 ucRfCenterFreqSeg1;  /* To support 80/160MHz bandwidth */
    u8 ucRfCenterFreqSeg2;  /* To support 80/160MHz bandwidth */
    ENUM_CH_REQ_TYPE_T eReqType;
} MSG_CH_RECOVER_T, *P_MSG_CH_RECOVER_T;

typedef struct _CNM_INFO_T {
    u8 fgChGranted;
    u8 ucBssIndex;
    u8 ucTokenID;
#if CFG_SUPPORT_DBDC_TC6
    LINK_T
        rDbdcSwitchGuradPendingReqList;  /* Pending Request List for DBDC switch
                                          * guard */
    u8 fgSkipDbdcDisable;  /* Skip DBDC diable to keep DBDC status for BCN
                            * timeout */
#endif
} CNM_INFO_T, *P_CNM_INFO_T;

#if CFG_ENABLE_WIFI_DIRECT
/* Moved from p2p_fsm.h */
typedef struct _DEVICE_TYPE_T {
    u16 u2CategoryId;  /* Category ID */
    u8 aucOui[4];  /* OUI */
    u16 u2SubCategoryId;  /* Sub Category ID */
} __KAL_ATTRIB_PACKED__ DEVICE_TYPE_T, *P_DEVICE_TYPE_T;
#endif

#if CFG_SUPPORT_DBDC
typedef struct _CNM_DBDC_CAP_T {
    u8 ucBssIndex;
    u8 ucNss;
    u8 ucWmmSetIndex;
} CNM_DBDC_CAP_T, *P_CNM_DBDC_CAP_T;

typedef enum _ENUM_CNM_DBDC_MODE_T {
    DBDC_MODE_DISABLED,  /* A/G traffic separate by WMM, but both WMM TRX on
                          * band 0, CANNOT enable DBDC */
    DBDC_MODE_STATIC,  /* A/G traffic separate by WMM, WMM0/1 TRX on band
                        * 0/1, CANNOT disable DBDC */
    DBDC_MODE_DYNAMIC,  /* Automatically enable/disable DBDC, setting just
                         * like static/disable mode */
    DBDC_MODE_NUM
} ENUM_CNM_DBDC_MODE_T,
*P_ENUM_CNM_DBDC_MODE_T;

typedef enum _ENUM_CNM_DBDC_SWITCH_MECHANISM_T {  /* When DBDC available in dynamic DBDC */
    DBDC_SWITCH_MECHANISM_LATENCY_MODE,  /* Switch to DBDC when available (less
                                          * latency) */
    DBDC_SWITCH_MECHANISM_THROUGHPUT_MODE,  /* Switch to DBDC when DBDC T-put > MCC
                                             * T-put */
    DBDC_SWITCH_MECHANISM_NUM
} ENUM_CNM_DBDC_SWITCH_MECHANISM_T,
*P_ENUM_CNM_DBDC_SWITCH_MECHANISM_T;

/* Define DBDC Decision timer  */
typedef enum _ENUM_CNM_DBDC_DECISION_TIMER_T {
    DBDC_DECISION_TIMER_SWITCH_GUARD_TIME,
    DBDC_DECISION_TIMER_DISABLE_COUNT_DOWN,
#if CFG_SUPPORT_DBDC_TC6
    DBDC_DECISION_TIMER_RECONNECT_COUNT_DOWN,
    DBDC_DECISION_TIMER_AIS_CONNECT_COUNT_DOWN,
    DBDC_DECISION_AIS_RECONNECT,
#endif
    DBDC_DECISION_STATE_NUM
} ENUM_CNM_DBDC_DECISION_TIMER_T,
*P_ENUM_CNM_DBDC_DECISION_TIMER_T;

#endif

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

#define CNM_CH_GRANTED_FOR_BSS(_prAdapter, _ucBssIndex) \
    ((_prAdapter)->rCnmInfo.fgChGranted &&              \
     (_prAdapter)->rCnmInfo.ucBssIndex == (_ucBssIndex))

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void cnmInit(P_ADAPTER_T prAdapter);

void cnmChMngrRequestPrivilege(P_ADAPTER_T prAdapter, P_MSG_HDR_T prMsgHdr);

void cnmChMngrAbortPrivilege(P_ADAPTER_T prAdapter, P_MSG_HDR_T prMsgHdr);

void cnmChMngrHandleChEvent(P_ADAPTER_T prAdapter,
                            P_WIFI_EVENT_T prEvent,
                            u32 u4EventBufLen);

#if (CFG_SUPPORT_DFS_MASTER == 1)
void cnmRadarDetectEvent(P_ADAPTER_T prAdapter,
                         P_WIFI_EVENT_T prEvent,
                         u32 u4EventBufLen);

void cnmCsaDoneEvent(P_ADAPTER_T prAdapter,
                     P_WIFI_EVENT_T prEvent,
                     u32 u4EventBufLen);
#endif

u8 cnmPreferredChannel(P_ADAPTER_T prAdapter,
                       P_ENUM_BAND_T prBand,
                       u8 *pucPrimaryChannel,
                       P_ENUM_CHNL_EXT_T prBssSCO);

u8 cnmAisInfraChannelFixed(P_ADAPTER_T prAdapter,
                           P_ENUM_BAND_T prBand,
                           u8 *pucPrimaryChannel);

u8 cnmAisIbssIsPermitted(P_ADAPTER_T prAdapter);

u8 cnmP2PIsPermitted(P_ADAPTER_T prAdapter);

u8 cnmBowIsPermitted(P_ADAPTER_T prAdapter);

u8 cnmBss40mBwPermitted(P_ADAPTER_T prAdapter, u8 ucBssIndex);

u8 cnmBss80mBwPermitted(P_ADAPTER_T prAdapter, u8 ucBssIndex);

u8 cnmGetBssMaxBw(P_ADAPTER_T prAdapter, u8 ucBssIndex);

u8 cnmGetBssMaxBwToChnlBW(P_ADAPTER_T prAdapter, u8 ucBssIndex);

P_BSS_INFO_T cnmGetBssInfoAndInit(P_ADAPTER_T prAdapter,
                                  ENUM_NETWORK_TYPE_T eNetworkType,
                                  u8 fgIsP2pDevice);

void cnmFreeBssInfo(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);
#if CFG_SUPPORT_CHNL_CONFLICT_REVISE
u8 cnmAisDetectP2PChannel(P_ADAPTER_T prAdapter,
                          P_ENUM_BAND_T prBand,
                          u8 *pucPrimaryChannel);
#endif

#if (CFG_HW_WMM_BY_BSS == 1)
u8 cnmWmmIndexDecision(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo);
void cnmFreeWmmIndex(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo);
#endif

#if CFG_SUPPORT_DBDC_TC6
u8 cnmSapIsActive(IN P_ADAPTER_T prAdapter);

u8 cnmSapIsConcurrent(IN P_ADAPTER_T prAdapter);

P_BSS_INFO_T cnmGetp2pSapBssInfo(IN P_ADAPTER_T prAdapter);

void cnmSapChannelSwitchReq(IN P_ADAPTER_T prAdapter,
                            IN P_RF_CHANNEL_INFO_T prRfChannelInfo,
                            IN u8 ucRoleIdx);

u8 cnmIdcCsaReq(IN P_ADAPTER_T prAdapter, IN u8 ch_num, IN u8 ucRoleIdx);

#endif

#if CFG_SUPPORT_DBDC
void cnmInitDbdcSetting(IN P_ADAPTER_T prAdapter);

void cnmUpdateDbdcSetting(IN P_ADAPTER_T prAdapter, IN u8 fgDbdcEn);

void cnmGetDbdcCapability(IN P_ADAPTER_T prAdapter,
                          IN u8 ucBssIndex,
                          IN ENUM_BAND_T eRfBand,
                          IN u8 ucPrimaryChannel,
                          IN u8 ucNss,
                          OUT P_CNM_DBDC_CAP_T prDbdcCap);

void cnmDbdcEnableDecision(IN P_ADAPTER_T prAdapter,
                           IN u8 ucChangedBssIndex,
                           IN ENUM_BAND_T eRfBand);

void cnmDbdcDisableDecision(IN P_ADAPTER_T prAdapter, IN u8 ucChangedBssIndex);
void cnmDbdcDecision(IN P_ADAPTER_T prAdapter, IN unsigned long plParamPtr);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#ifndef _lint
/* We don't have to call following function to inspect the data structure.
 * It will check automatically while at compile time.
 * We'll need this to guarantee the same member order in different structures
 * to simply handling effort in some functions.
 */
static __KAL_INLINE__ void cnmMsgDataTypeCheck(void)
{
    DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(MSG_CH_GRANT_T, rMsgHdr) == 0);

    DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(MSG_CH_GRANT_T, rMsgHdr) ==
                                  OFFSET_OF(MSG_CH_RECOVER_T, rMsgHdr));

    DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(MSG_CH_GRANT_T, ucBssIndex) ==
                                  OFFSET_OF(MSG_CH_RECOVER_T, ucBssIndex));

    DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(MSG_CH_GRANT_T, ucTokenID) ==
                                  OFFSET_OF(MSG_CH_RECOVER_T, ucTokenID));

    DATA_STRUCT_INSPECTING_ASSERT(
        OFFSET_OF(MSG_CH_GRANT_T, ucPrimaryChannel) ==
        OFFSET_OF(MSG_CH_RECOVER_T, ucPrimaryChannel));

    DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(MSG_CH_GRANT_T, eRfSco) ==
                                  OFFSET_OF(MSG_CH_RECOVER_T, eRfSco));

    DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(MSG_CH_GRANT_T, eRfBand) ==
                                  OFFSET_OF(MSG_CH_RECOVER_T, eRfBand));

    DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(MSG_CH_GRANT_T, eReqType) ==
                                  OFFSET_OF(MSG_CH_RECOVER_T, eReqType));
}
#endif

#endif
