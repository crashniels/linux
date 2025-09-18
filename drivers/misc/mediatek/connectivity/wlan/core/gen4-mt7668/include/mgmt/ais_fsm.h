/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   ais_fsm.h
 *  \brief  Declaration of functions and finite state machine for AIS Module.
 *
 *  Declaration of functions and finite state machine for AIS Module.
 */

#ifndef _AIS_FSM_H
#define _AIS_FSM_H

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

#define AIS_BG_SCAN_INTERVAL_MIN_SEC         2  /* 30 // exponential to 960 */
#define AIS_BG_SCAN_INTERVAL_MAX_SEC         2  /* 960 // 16min */
#if CFG_SUPPORT_ROAMING
#define AIS_DELAY_TIME_OF_DISCONNECT_SEC \
    0  /* 100 Milli-seconds - Immediate indication to reduce inter-SSID \
        * roam time \
        */
#else
#define AIS_DELAY_TIME_OF_DISCONNECT_SEC     5  /* 10 */
#endif
#define AIS_IBSS_ALONE_TIMEOUT_SEC       20  /* seconds */

#define AIS_BEACON_TIMEOUT_COUNT_ADHOC       30
#define AIS_BEACON_TIMEOUT_COUNT_INFRA       10
#define AIS_BEACON_TIMEOUT_GUARD_TIME_SEC    1  /* Second */

#define AIS_BEACON_MAX_TIMEOUT_TU        100
#define AIS_BEACON_MIN_TIMEOUT_TU        5
#define AIS_BEACON_MAX_TIMEOUT_VALID         true
#define AIS_BEACON_MIN_TIMEOUT_VALID         true

#define AIS_BMC_MAX_TIMEOUT_TU           100
#define AIS_BMC_MIN_TIMEOUT_TU           5
#define AIS_BMC_MAX_TIMEOUT_VALID        true
#define AIS_BMC_MIN_TIMEOUT_VALID        true

#define AIS_JOIN_CH_GRANT_THRESHOLD      10
#define AIS_JOIN_CH_REQUEST_INTERVAL         6000

#ifdef CFG_SUPPORT_ADJUST_JOIN_CH_REQ_INTERVAL
#define AIS_JOIN_CH_REQUEST_MAX_INTERVAL     4000
#endif

#define AIS_SCN_DONE_TIMEOUT_SEC         15  /* 15 for 2.4G + 5G */ /* 5 */

#define AIS_DEFAULT_INDEX            (0)
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

typedef enum _ENUM_AIS_STATE_T {
    AIS_STATE_IDLE = 0,
    AIS_STATE_SEARCH,
    AIS_STATE_SCAN,
    AIS_STATE_ONLINE_SCAN,
    AIS_STATE_LOOKING_FOR,
    AIS_STATE_WAIT_FOR_NEXT_SCAN,
    AIS_STATE_REQ_CHANNEL_JOIN,
    AIS_STATE_JOIN,
    AIS_STATE_JOIN_FAILURE,
    AIS_STATE_IBSS_ALONE,
    AIS_STATE_IBSS_MERGE,
    AIS_STATE_NORMAL_TR,
    AIS_STATE_DISCONNECTING,
    AIS_STATE_REQ_REMAIN_ON_CHANNEL,
    AIS_STATE_REMAIN_ON_CHANNEL,
    AIS_STATE_NUM
} ENUM_AIS_STATE_T;

typedef struct _MSG_AIS_ABORT_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucReasonOfDisconnect;
    u8 fgDelayIndication;
} MSG_AIS_ABORT_T, *P_MSG_AIS_ABORT_T;

typedef struct _MSG_AIS_IBSS_PEER_FOUND_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucBssIndex;
    u8 fgIsMergeIn;  /* true: Merge In, false: Merge Out */
    P_STA_RECORD_T prStaRec;
} MSG_AIS_IBSS_PEER_FOUND_T, *P_MSG_AIS_IBSS_PEER_FOUND_T;

typedef enum _ENUM_AIS_REQUEST_TYPE_T {
    AIS_REQUEST_SCAN,
    AIS_REQUEST_RECONNECT,
    AIS_REQUEST_ROAMING_SEARCH,
    AIS_REQUEST_ROAMING_CONNECT,
    AIS_REQUEST_REMAIN_ON_CHANNEL,
    AIS_REQUEST_NUM
} ENUM_AIS_REQUEST_TYPE_T;

typedef struct _AIS_REQ_HDR_T {
    LINK_ENTRY_T rLinkEntry;
    ENUM_AIS_REQUEST_TYPE_T eReqType;
} AIS_REQ_HDR_T, *P_AIS_REQ_HDR_T;

typedef struct _AIS_REQ_CHNL_INFO {
    ENUM_BAND_T eBand;
    ENUM_CHNL_EXT_T eSco;
    u8 ucChannelNum;
    u32 u4DurationMs;
    u64 u8Cookie;
} AIS_REQ_CHNL_INFO, *P_AIS_REQ_CHNL_INFO;

typedef struct _AIS_MGMT_TX_REQ_INFO_T {
    u8 fgIsMgmtTxRequested;
    P_MSDU_INFO_T prMgmtTxMsdu;
    u64 u8Cookie;
} AIS_MGMT_TX_REQ_INFO_T, *P_AIS_MGMT_TX_REQ_INFO_T;

typedef struct _AIS_FSM_INFO_T {
    ENUM_AIS_STATE_T ePreviousState;
    ENUM_AIS_STATE_T eCurrentState;

    u8 fgTryScan;

    u8 fgIsInfraChannelFinished;
    u8 fgIsChannelRequested;
    u8 fgIsChannelGranted;

#if CFG_SUPPORT_ROAMING
    u8 fgIsRoamingScanPending;
#endif

    u8 ucAvailableAuthTypes;  /* Used for AUTH_MODE_AUTO_SWITCH */

    P_BSS_DESC_T prTargetBssDesc;  /* For destination */

    P_STA_RECORD_T prTargetStaRec;  /* For JOIN Abort */

    u32 u4SleepInterval;

    TIMER_T rBeaconLostTimer;

    TIMER_T rBGScanTimer;

    TIMER_T rIbssAloneTimer;

    TIMER_T rIndicationOfDisconnectTimer;

    TIMER_T rJoinTimeoutTimer;

    TIMER_T rChannelTimeoutTimer;

    TIMER_T rScanDoneTimer;

    TIMER_T rDeauthDoneTimer;

    u8 ucSeqNumOfReqMsg;
    u8 ucSeqNumOfChReq;
    u8 ucSeqNumOfScanReq;

    u32 u4ChGrantedInterval;

    u8 ucConnTrialCount;

    u8 ucScanSSIDNum;
    PARAM_SSID_T arScanSSID[SCN_SSID_MAX_NUM];

    u32 u4ScanIELength;
    u8 aucScanIEBuf[MAX_IE_LENGTH];

#if CFG_SCAN_CHANNEL_SPECIFIED
    u8 ucScanChannelListNum;
    RF_CHANNEL_INFO_T arScanChnlInfoList[MAXIMUM_OPERATION_CHANNEL_LIST];
#endif
    u8 fgIsScanOidAborted;

    /* Pending Request List */
    LINK_T rPendingReqList;

    /* Join Request Timestamp */
    u32 rJoinReqTime;

    /* for cfg80211 REMAIN_ON_CHANNEL support */
    AIS_REQ_CHNL_INFO rChReqInfo;

    /* Mgmt tx related. */
    AIS_MGMT_TX_REQ_INFO_T rMgmtTxInfo;

    /* Packet filter for AIS module. */
    u32 u4AisPacketFilter;

    /* for roaming target */
    PARAM_SSID_T rRoamingSSID;
} AIS_FSM_INFO_T, *P_AIS_FSM_INFO_T;

enum WNM_AIS_BSS_TRANSITION {
    BSS_TRANSITION_NO_MORE_ACTION,
    BSS_TRANSITION_REQ_ROAMING,
    BSS_TRANSITION_DISASSOC,
    BSS_TRANSITION_MAX_NUM
};

typedef struct _MSG_AIS_BSS_TRANSITION_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucToken;
    u8 fgNeedResponse;
    u8 ucValidityInterval;
    enum WNM_AIS_BSS_TRANSITION eTransitionType;
    u16 u2CandListLen;
    u8 *pucCandList;
} MSG_AIS_BSS_TRANSITION_T, *P_MSG_AIS_BSS_TRANSITION_T;

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

#define aisChangeMediaState(_prAdapter, _eNewMediaState) \
    (_prAdapter->prAisBssInfo->eConnectionState = (_eNewMediaState))

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void aisInitializeConnectionSettings(IN P_ADAPTER_T prAdapter,
                                     IN P_REG_INFO_T prRegInfo);

void aisFsmInit(IN P_ADAPTER_T prAdapter);

void aisFsmUninit(IN P_ADAPTER_T prAdapter);

void aisFsmStateInit_JOIN(IN P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc);

u8 aisFsmStateInit_RetryJOIN(IN P_ADAPTER_T prAdapter,
                             IN P_STA_RECORD_T prStaRec);

void aisFsmStateInit_IBSS_ALONE(IN P_ADAPTER_T prAdapter);

void aisFsmStateInit_IBSS_MERGE(IN P_ADAPTER_T prAdapter,
                                P_BSS_DESC_T prBssDesc);

void aisFsmStateAbort(IN P_ADAPTER_T prAdapter,
                      u8 ucReasonOfDisconnect,
                      u8 fgDelayIndication);

void aisFsmStateAbort_JOIN(IN P_ADAPTER_T prAdapter);

void aisFsmStateAbort_SCAN(IN P_ADAPTER_T prAdapter);

void aisFsmStateAbort_NORMAL_TR(IN P_ADAPTER_T prAdapter);

void aisFsmStateAbort_IBSS(IN P_ADAPTER_T prAdapter);

void aisFsmSteps(IN P_ADAPTER_T prAdapter, ENUM_AIS_STATE_T eNextState);

/*----------------------------------------------------------------------------*/
/* Mailbox Message Handling                                                   */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventScanDone(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);

void aisFsmRunEventAbort(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);

void aisFsmRunEventJoinComplete(IN P_ADAPTER_T prAdapter,
                                IN P_MSG_HDR_T prMsgHdr);

enum _ENUM_AIS_STATE_T aisFsmJoinCompleteAction(IN struct _ADAPTER_T *prAdapter,
                                                IN struct _MSG_HDR_T *prMsgHdr);

void aisFsmRunEventFoundIBSSPeer(IN P_ADAPTER_T prAdapter,
                                 IN P_MSG_HDR_T prMsgHdr);

void aisFsmRunEventRemainOnChannel(IN P_ADAPTER_T prAdapter,
                                   IN P_MSG_HDR_T prMsgHdr);

void aisFsmRunEventCancelRemainOnChannel(IN P_ADAPTER_T prAdapter,
                                         IN P_MSG_HDR_T prMsgHdr);

/*----------------------------------------------------------------------------*/
/* Handling for Ad-Hoc Network                                                */
/*----------------------------------------------------------------------------*/
void aisFsmCreateIBSS(IN P_ADAPTER_T prAdapter);

void aisFsmMergeIBSS(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

/*----------------------------------------------------------------------------*/
/* Handling of Incoming Mailbox Message from CNM                              */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventChGrant(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);

/*----------------------------------------------------------------------------*/
/* Generating Outgoing Mailbox Message to CNM                                 */
/*----------------------------------------------------------------------------*/
void aisFsmReleaseCh(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* Event Indication                                                           */
/*----------------------------------------------------------------------------*/
void aisIndicationOfMediaStateToHost(IN P_ADAPTER_T prAdapter,
                                     ENUM_PARAM_MEDIA_STATE_T eConnectionState,
                                     u8 fgDelayIndication);

void aisPostponedEventOfDisconnTimeout(IN P_ADAPTER_T prAdapter,
                                       unsigned long ulParamPtr);

void aisUpdateBssInfoForJOIN(IN P_ADAPTER_T prAdapter,
                             P_STA_RECORD_T prStaRec,
                             P_SW_RFB_T prAssocRspSwRfb);

void aisUpdateBssInfoForCreateIBSS(IN P_ADAPTER_T prAdapter);

void aisUpdateBssInfoForMergeIBSS(IN P_ADAPTER_T prAdapter,
                                  IN P_STA_RECORD_T prStaRec);

u8 aisValidateProbeReq(IN P_ADAPTER_T prAdapter,
                       IN P_SW_RFB_T prSwRfb,
                       OUT u32 *pu4ControlFlags);

WLAN_STATUS
aisFsmRunEventMgmtFrameTxDone(IN P_ADAPTER_T prAdapter,
                              IN P_MSDU_INFO_T prMsduInfo,
                              IN ENUM_TX_RESULT_CODE_T rTxDoneStatus);

/*----------------------------------------------------------------------------*/
/* Disconnection Handling                                                     */
/*----------------------------------------------------------------------------*/
void aisFsmDisconnect(IN P_ADAPTER_T prAdapter, IN u8 fgDelayIndication);

/*----------------------------------------------------------------------------*/
/* Event Handling                                                             */
/*----------------------------------------------------------------------------*/
void aisBssBeaconTimeout(IN P_ADAPTER_T prAdapter, IN u8 ucReasonCode);

void aisBssLinkDown(IN P_ADAPTER_T prAdapter);

#if CFG_SUPPORT_DBDC_TC6
u8 aisBssChangeNSS(IN P_ADAPTER_T prAdapter, IN u8 fgDbdcEn);
#endif

WLAN_STATUS
aisDeauthXmitComplete(IN P_ADAPTER_T prAdapter,
                      IN P_MSDU_INFO_T prMsduInfo,
                      IN ENUM_TX_RESULT_CODE_T rTxDoneStatus);

#if CFG_SUPPORT_ROAMING
void aisFsmRunEventRoamingDiscovery(IN P_ADAPTER_T prAdapter, u32 u4ReqScan);

ENUM_AIS_STATE_T aisFsmRoamingScanResultsUpdate(IN P_ADAPTER_T prAdapter);

void aisFsmRoamingDisconnectPrevAP(IN P_ADAPTER_T prAdapter,
                                   IN P_STA_RECORD_T prTargetStaRec);
#endif

#if CFG_SUPPORT_ROAMING || CFG_SUPPORT_SAME_BSS_REASSOC
void aisUpdateBssInfoForRoamingAP(IN P_ADAPTER_T prAdapter,
                                  IN P_STA_RECORD_T prStaRec,
                                  IN P_SW_RFB_T prAssocRspSwRfb);
#endif

/*----------------------------------------------------------------------------*/
/* Timeout Handling                                                           */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventBGSleepTimeOut(IN P_ADAPTER_T prAdapter,
                                  unsigned long ulParamPtr);

void aisFsmBeaconLostTimeOut(IN P_ADAPTER_T prAdapter,
                             unsigned long ulParamPtr);

void aisFsmRunEventIbssAloneTimeOut(IN P_ADAPTER_T prAdapter,
                                    unsigned long ulParamPtr);

void aisFsmRunEventJoinTimeout(IN P_ADAPTER_T prAdapter,
                               unsigned long ulParamPtr);

void aisFsmRunEventChannelTimeout(IN P_ADAPTER_T prAdapter,
                                  unsigned long ulParamPtr);

void aisFsmRunEventDeauthTimeout(IN P_ADAPTER_T prAdapter,
                                 unsigned long ulParamPtr);

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
void aisRxMcsCollectionTimeout(IN P_ADAPTER_T prAdapter,
                               unsigned long ulParamPtr);
#endif

/*----------------------------------------------------------------------------*/
/* OID/IOCTL Handling                                                         */
/*----------------------------------------------------------------------------*/
void aisFsmScanRequest(IN P_ADAPTER_T prAdapter,
                       IN P_PARAM_SSID_T prSsid,
                       IN u8 *pucIe,
                       IN u32 u4IeLength);

void aisFsmScanRequestAdv(IN P_ADAPTER_T prAdapter,
                          IN u8 ucSsidNum,
                          IN P_PARAM_SSID_T prSsid,
                          IN u8 ucChannelListNum,
                          IN P_RF_CHANNEL_INFO_T prChnlInfoList,
                          IN u8 *pucIe,
                          IN u32 u4IeLength);

/*----------------------------------------------------------------------------*/
/* Internal State Checking                                                    */
/*----------------------------------------------------------------------------*/
u8 aisFsmIsRequestPending(IN P_ADAPTER_T prAdapter,
                          IN ENUM_AIS_REQUEST_TYPE_T eReqType,
                          IN u8 bRemove);

P_AIS_REQ_HDR_T aisFsmGetNextRequest(IN P_ADAPTER_T prAdapter);

u8 aisFsmInsertRequest(IN P_ADAPTER_T prAdapter,
                       IN ENUM_AIS_REQUEST_TYPE_T eReqType);

void aisFsmFlushRequest(IN P_ADAPTER_T prAdapter);

WLAN_STATUS
aisFuncTxMgmtFrame(IN P_ADAPTER_T prAdapter,
                   IN P_AIS_MGMT_TX_REQ_INFO_T prMgmtTxReqInfo,
                   IN P_MSDU_INFO_T prMgmtTxMsdu,
                   IN u64 u8Cookie);

void aisFsmRunEventMgmtFrameTx(IN P_ADAPTER_T prAdapter,
                               IN P_MSG_HDR_T prMsgHdr);

void aisFuncValidateRxActionFrame(IN P_ADAPTER_T prAdapter,
                                  IN P_SW_RFB_T prSwRfb);

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
void aisFsmRunEventBssTransition(IN P_ADAPTER_T prAdapter,
                                 IN P_MSG_HDR_T prMsgHdr);
#endif

#if CFG_SUPPORT_802_11K
void aisSendNeighborRequest(IN P_ADAPTER_T prAdapter);
#endif

#if CFG_SUPPORT_802_11K || CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
void aisResetNeighborApList(IN P_ADAPTER_T prAdapter);

void aisCollectNeighborAP(IN P_ADAPTER_T prAdapter,
                          u8 *pucApBuf,
                          u16 u2ApBufLen,
                          u8 ucValidInterval);
#endif

enum _ENUM_AIS_STATE_T aisFsmStateSearchAction(IN struct _ADAPTER_T *prAdapter,
                                               u8 ucPhase);
#if defined(CFG_TEST_MGMT_FSM) && (CFG_TEST_MGMT_FSM != 0)
void aisTest(void);
#endif
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
