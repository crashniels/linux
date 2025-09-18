/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

#ifndef _P2P_ROLE_H
#define _P2P_ROLE_H

typedef enum _ENUM_BUFFER_TYPE_T {
    ENUM_FRAME_TYPE_EXTRA_IE_BEACON,
    ENUM_FRAME_TYPE_EXTRA_IE_ASSOC_RSP,
    ENUM_FRAME_TYPE_EXTRA_IE_PROBE_RSP,
    ENUM_FRAME_TYPE_PROBE_RSP_TEMPLATE,
    ENUM_FRAME_TYPE_BEACON_TEMPLATE,
    ENUM_FRAME_IE_NUM
} ENUM_BUFFER_TYPE_T,
*P_ENUM_BUFFER_TYPE_T;

typedef enum _ENUM_HIDDEN_SSID_TYPE_T {
    ENUM_HIDDEN_SSID_NONE,
    ENUM_HIDDEN_SSID_LEN,
    ENUM_HIDDEN_SSID_ZERO_CONTENT,
    ENUM_HIDDEN_SSID_NUM
} ENUM_HIDDEN_SSID_TYPE_T,
*P_ENUM_HIDDEN_SSID_TYPE_T;

typedef struct _P2P_BEACON_UPDATE_INFO_T {
    u8 *pucBcnHdr;
    u32 u4BcnHdrLen;
    u8 *pucBcnBody;
    u32 u4BcnBodyLen;
} P2P_BEACON_UPDATE_INFO_T, *P_P2P_BEACON_UPDATE_INFO_T;

typedef struct _P2P_PROBE_RSP_UPDATE_INFO_T {
    P_MSDU_INFO_T prProbeRspMsduTemplate;
} P2P_PROBE_RSP_UPDATE_INFO_T, *P_P2P_PROBE_RSP_UPDATE_INFO_T;

typedef struct _P2P_ASSOC_RSP_UPDATE_INFO_T {
    u8 *pucAssocRspExtIE;
    u16 u2AssocIELen;
} P2P_ASSOC_RSP_UPDATE_INFO_T, *P_P2P_ASSOC_RSP_UPDATE_INFO_T;

typedef struct _AP_CRYPTO_SETTINGS_T {
    u32 u4WpaVersion;
    u32 u4CipherGroup;
    s32 i4NumOfCiphers;
    u32 aucCiphersPairwise[5];
    s32 i4NumOfAkmSuites;
    u32 aucAkmSuites[2];
    u8 fgIsControlPort;
    u16 u2ControlPortBE;
    u8 fgIsControlPortEncrypt;
} AP_CRYPTO_SETTINGS_T, *P_AP_CRYPTO_SETTINGS_T;

/* ////////////////////////////////// Message
 * /////////////////////////////////// */

typedef struct _MSG_P2P_BEACON_UPDATE_T {
    MSG_HDR_T rMsgHdr;
    u8 ucRoleIndex;
    u32 u4BcnHdrLen;
    u32 u4BcnBodyLen;
    u32 u4AssocRespLen;
    u8 *pucBcnHdr;
    u8 *pucBcnBody;
    u8 *pucAssocRespIE;
    u8 fgIsWepCipher;
    u8 aucBuffer[1];  /* Header & Body & Extra IEs are put here. */
} MSG_P2P_BEACON_UPDATE_T, *P_MSG_P2P_BEACON_UPDATE_T;

typedef struct _MSG_P2P_MGMT_FRAME_UPDATE_T {
    MSG_HDR_T rMsgHdr;
    ENUM_BUFFER_TYPE_T eBufferType;
    u32 u4BufferLen;
    u8 aucBuffer[1];
} MSG_P2P_MGMT_FRAME_UPDATE_T, *P_MSG_P2P_MGMT_FRAME_UPDATE_T;

typedef struct _MSG_P2P_SWITCH_OP_MODE_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    ENUM_OP_MODE_T eOpMode;
    u8 ucRoleIdx;
} MSG_P2P_SWITCH_OP_MODE_T, *P_MSG_P2P_SWITCH_OP_MODE_T;

typedef struct _MSG_P2P_MGMT_FRAME_REGISTER_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u16 u2FrameType;
    u8 fgIsRegister;
} MSG_P2P_MGMT_FRAME_REGISTER_T, *P_MSG_P2P_MGMT_FRAME_REGISTER_T;

typedef struct _MSG_P2P_CHNL_ABORT_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u64 u8Cookie;
} MSG_P2P_CHNL_ABORT_T, *P_MSG_P2P_CHNL_ABORT_T;

typedef struct _MSG_P2P_CONNECTION_REQUEST_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucRoleIdx;
    P2P_SSID_STRUCT_T rSsid;
    u8 aucBssid[MAC_ADDR_LEN];
    u8 aucSrcMacAddr[MAC_ADDR_LEN];
    ENUM_CHNL_EXT_T eChnlSco;
    RF_CHANNEL_INFO_T rChannelInfo;
    u32 u4IELen;
    u8 aucIEBuf[1];
    /* TODO: Auth Type, OPEN, SHARED, FT, EAP... */
} MSG_P2P_CONNECTION_REQUEST_T, *P_MSG_P2P_CONNECTION_REQUEST_T;

typedef struct _MSG_P2P_CONNECTION_ABORT_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member. */
    u8 ucRoleIdx;
    u8 aucTargetID[MAC_ADDR_LEN];
    u16 u2ReasonCode;
    u8 fgSendDeauth;
} MSG_P2P_CONNECTION_ABORT_T, *P_MSG_P2P_CONNECTION_ABORT_T;

typedef struct _MSG_P2P_START_AP_T {
    MSG_HDR_T rMsgHdr;
    u32 u4DtimPeriod;
    u32 u4BcnInterval;
    u8 aucSsid[32];
    u16 u2SsidLen;
    u8 ucHiddenSsidType;
    u8 fgIsPrivacy;
    u8 ucRoleIdx;
    AP_CRYPTO_SETTINGS_T rEncryptionSettings;
    s32 i4InactiveTimeout;
} MSG_P2P_START_AP_T, *P_MSG_P2P_START_AP_T;

#if (CFG_SUPPORT_DFS_MASTER == 1)
typedef struct _MSG_P2P_DFS_CAC_T {
    MSG_HDR_T rMsgHdr;
    ENUM_CHANNEL_WIDTH_T eChannelWidth;
    u8 ucRoleIdx;
} MSG_P2P_DFS_CAC_T, *P_MSG_P2P_DFS_CAC_T;

typedef struct _MSG_P2P_RADAR_DETECT_T {
    MSG_HDR_T rMsgHdr;
    u8 ucBssIndex;
} MSG_P2P_RADAR_DETECT_T, *P_MSG_P2P_RADAR_DETECT_T;

struct P2P_RADAR_INFO {
    u8 ucRadarReportMode;  /*0: Only report radar detected; 1:  Add parameter
                            * reports*/
    u8 ucRddIdx;
    u8 ucLongDetected;
    u8 ucPeriodicDetected;
    u8 ucLPBNum;
    u8 ucPPBNum;
    u8 ucLPBPeriodValid;
    u8 ucLPBWidthValid;
    u8 ucPRICountM1;
    u8 ucPRICountM1TH;
    u8 ucPRICountM2;
    u8 ucPRICountM2TH;
    u32 u4PRI1stUs;
    LONG_PULSE_BUFFER_T arLpbContent[LPB_SIZE];
    PERIODIC_PULSE_BUFFER_T arPpbContent[PPB_SIZE];
};

typedef struct _MSG_P2P_SET_NEW_CHANNEL_T {
    MSG_HDR_T rMsgHdr;
    ENUM_CHANNEL_WIDTH_T eChannelWidth;
    u8 ucRoleIdx;
    u8 ucBssIndex;
} MSG_P2P_SET_NEW_CHANNEL_T, *P_MSG_P2P_SET_NEW_CHANNEL_T;

typedef struct _MSG_P2P_CSA_DONE_T {
    MSG_HDR_T rMsgHdr;
    u8 ucBssIndex;
} MSG_P2P_CSA_DONE_T, *P_MSG_P2P_CSA_DONE_T;
#endif

typedef struct _MSG_P2P_DEL_IFACE_T {
    MSG_HDR_T rMsgHdr;
    u8 ucRoleIdx;
} MSG_P2P_DEL_IFACE_T, *P_MSG_P2P_DEL_IFACE_T;

typedef struct _P2P_STATION_INFO_T {
    u32 u4InactiveTime;
    u32 u4RxBytes;  /* TODO: */
    u32 u4TxBytes;  /* TODO: */
    u32 u4RxPackets;  /* TODO: */
    u32 u4TxPackets;  /* TODO: */
    /* TODO: Add more for requirement. */
} P2P_STATION_INFO_T, *P_P2P_STATION_INFO_T;

/* 3  --------------- WFA P2P Attributes Handler prototype --------------- */
typedef u32 (*PFN_APPEND_ATTRI_FUNC)(P_ADAPTER_T, u8, u8, u16 *, u8 *, u16);

typedef u32 (*PFN_CALCULATE_VAR_ATTRI_LEN_FUNC)(P_ADAPTER_T, P_STA_RECORD_T);

typedef struct _APPEND_VAR_ATTRI_ENTRY_T {
    u16 u2EstimatedFixedAttriLen;  /* For fixed length */
    PFN_CALCULATE_VAR_ATTRI_LEN_FUNC pfnCalculateVariableAttriLen;
    PFN_APPEND_ATTRI_FUNC pfnAppendAttri;
} APPEND_VAR_ATTRI_ENTRY_T, *P_APPEND_VAR_ATTRI_ENTRY_T;

/* //////////////////////////////////////////////////////////////// */

typedef enum _ENUM_P2P_ROLE_STATE_T {
    P2P_ROLE_STATE_IDLE = 0,
    P2P_ROLE_STATE_SCAN,
    P2P_ROLE_STATE_REQING_CHANNEL,
    P2P_ROLE_STATE_AP_CHNL_DETECTION,  /* Requesting Channel to Send Specific
                                        * Frame. */
    P2P_ROLE_STATE_GC_JOIN,
#if (CFG_SUPPORT_DFS_MASTER == 1)
    P2P_ROLE_STATE_DFS_CAC,
    P2P_ROLE_STATE_SWITCH_CHANNEL,
#endif
    P2P_ROLE_STATE_NUM
} ENUM_P2P_ROLE_STATE_T,
*P_ENUM_P2P_ROLE_STATE_T;

typedef enum _ENUM_P2P_CONNECTION_TYPE_T {
    P2P_CONNECTION_TYPE_IDLE = 0,
    P2P_CONNECTION_TYPE_GO,
    P2P_CONNECTION_TYPE_GC,
    P2P_CONNECTION_TYPE_PURE_AP,
    P2P_CONNECTION_TYPE_NUM
} ENUM_P2P_CONNECTION_TYPE_T,
*P_ENUM_P2P_CONNECTION_TYPE_T;

typedef struct _P2P_JOIN_INFO_T {
    u8 ucSeqNumOfReqMsg;
    u8 ucAvailableAuthTypes;
    P_STA_RECORD_T prTargetStaRec;
    P_BSS_DESC_T prTargetBssDesc;
    u8 fgIsJoinComplete;
    /* For ASSOC Rsp. */
    u32 u4BufLength;
    u8 aucIEBuf[MAX_IE_LENGTH];
} P2P_JOIN_INFO_T, *P_P2P_JOIN_INFO_T;

/* For STA & AP mode. */
typedef struct _P2P_CONNECTION_REQ_INFO_T {
    ENUM_P2P_CONNECTION_TYPE_T eConnRequest;
    P2P_SSID_STRUCT_T rSsidStruct;
    u8 aucBssid[MAC_ADDR_LEN];

    /* AP preferred channel. */
    RF_CHANNEL_INFO_T rChannelInfo;
    ENUM_CHNL_EXT_T eChnlExt;

    /* To record channel bandwidth from CFG80211 */
    ENUM_MAX_BANDWIDTH_SETTING eChnlBw;

    /* To record primary channel frequency (MHz) from CFG80211 */
    u16 u2PriChnlFreq;

    /* To record Channel Center Frequency Segment 0 (MHz) from CFG80211 */
    u32 u4CenterFreq1;

    /* To record Channel Center Frequency Segment 1 (MHz) from CFG80211 */
    u32 u4CenterFreq2;

#if (CFG_SUPPORT_DFS_MASTER == 1)
    /* To record Channel DFS State */
    u32 u4ChnlDfsState;
#endif

    /* For ASSOC Req. */
    u32 u4BufLength;
    u8 aucIEBuf[MAX_IE_LENGTH];
} P2P_CONNECTION_REQ_INFO_T, *P_P2P_CONNECTION_REQ_INFO_T;

#define P2P_ROLE_INDEX_2_ROLE_FSM_INFO(_prAdapter, _RoleIndex) \
    ((_prAdapter)->rWifiVar.aprP2pRoleFsmInfo[_RoleIndex])

struct _P2P_ROLE_FSM_INFO_T {
    u8 ucRoleIndex;

    u8 ucBssIndex;

    /* State related. */
    ENUM_P2P_ROLE_STATE_T eCurrentState;

    /* Channel related. */
    P2P_CHNL_REQ_INFO_T rChnlReqInfo;

    /* Scan related. */
    P2P_SCAN_REQ_INFO_T rScanReqInfo;

    /* FSM Timer */
    TIMER_T rP2pRoleFsmTimeoutTimer;

#if (CFG_SUPPORT_DFS_MASTER == 1)
    TIMER_T rDfsShutDownTimer;
#if CFG_SUPPORT_DBDC
    TIMER_T rDfsStartCacTimer;
    ENUM_CHANNEL_WIDTH_T rChannelWidth;
#endif
#endif

    /* Packet filter for P2P module. */
    u32 u4P2pPacketFilter;

    /* GC Join related. */
    P2P_JOIN_INFO_T rJoinInfo;

    /* Connection related. */
    P2P_CONNECTION_REQ_INFO_T rConnReqInfo;

    /* Beacon Information. */
    P2P_BEACON_UPDATE_INFO_T rBeaconUpdateInfo;
};

/*========================= Initial ============================*/

u8 p2pRoleFsmInit(IN P_ADAPTER_T prAdapter, IN u8 ucRoleIdx);

void p2pRoleFsmUninit(IN P_ADAPTER_T prAdapter, IN u8 ucRoleIdx);

/*================== Message Event ==================*/

void p2pRoleFsmRunEventAbort(IN P_ADAPTER_T prAdapter,
                             IN P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo);

void p2pRoleFsmRunEventStartAP(IN P_ADAPTER_T prAdapter,
                               IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventDelIface(IN P_ADAPTER_T prAdapter,
                                IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventStopAP(IN P_ADAPTER_T prAdapter,
                              IN P_MSG_HDR_T prMsgHdr);

#if (CFG_SUPPORT_DFS_MASTER == 1)
void p2pRoleFsmRunEventDfsCac(IN P_ADAPTER_T prAdapter,
                              IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventRadarDet(IN P_ADAPTER_T prAdapter,
                                IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventSetNewChannel(IN P_ADAPTER_T prAdapter,
                                     IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventCsaDone(IN P_ADAPTER_T prAdapter,
                               IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventDfsShutDownTimeout(IN P_ADAPTER_T prAdapter,
                                          IN unsigned long ulParamPtr);
#endif

#if CFG_SUPPORT_DBDC_TC6
void p2pRoleFsmRunEventStartDfsCacTimeout(IN P_ADAPTER_T prAdapter,
                                          IN unsigned long ulParamPtr);
#endif

void p2pRoleFsmRunEventScanRequest(IN P_ADAPTER_T prAdapter,
                                   IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventScanDone(IN P_ADAPTER_T prAdapter,
                                IN P_MSG_HDR_T prMsgHdr,
                                IN P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo);

void p2pRoleFsmRunEventJoinComplete(IN P_ADAPTER_T prAdapter,
                                    IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventTimeout(IN P_ADAPTER_T prAdapter,
                               IN unsigned long ulParamPtr);

void p2pRoleFsmDeauthTimeout(IN P_ADAPTER_T prAdapter,
                             IN unsigned long ulParamPtr);

void p2pRoleFsmRunEventBeaconTimeout(IN P_ADAPTER_T prAdapter,
                                     IN P_BSS_INFO_T prP2pBssInfo);

void p2pRoleUpdateACLEntry(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx);

u8 p2pRoleProcessACLInspection(IN P_ADAPTER_T prAdapter,
                               IN u8 *pMacAddr,
                               IN u8 ucBssIdx);

WLAN_STATUS
p2pRoleFsmRunEventAAAComplete(IN P_ADAPTER_T prAdapter,
                              IN P_STA_RECORD_T prStaRec,
                              IN P_BSS_INFO_T prP2pBssInfo);

WLAN_STATUS
p2pRoleFsmRunEventAAASuccess(IN P_ADAPTER_T prAdapter,
                             IN P_STA_RECORD_T prStaRec,
                             IN P_BSS_INFO_T prP2pBssInfo);

void p2pRoleFsmRunEventAAATxFail(IN P_ADAPTER_T prAdapter,
                                 IN P_STA_RECORD_T prStaRec,
                                 IN P_BSS_INFO_T prP2pBssInfo);

void p2pRoleFsmRunEventConnectionRequest(IN P_ADAPTER_T prAdapter,
                                         IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventConnectionAbort(IN P_ADAPTER_T prAdapter,
                                       IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventChnlGrant(IN P_ADAPTER_T prAdapter,
                                 IN P_MSG_HDR_T prMsgHdr,
                                 IN P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo);

WLAN_STATUS
p2pRoleFsmRunEventDeauthTxDone(IN P_ADAPTER_T prAdapter,
                               IN P_MSDU_INFO_T prMsduInfo,
                               IN ENUM_TX_RESULT_CODE_T rTxDoneStatus);

void p2pRoleFsmRunEventRxDeauthentication(IN P_ADAPTER_T prAdapter,
                                          IN P_STA_RECORD_T prStaRec,
                                          IN P_SW_RFB_T prSwRfb);

void p2pRoleFsmRunEventRxDisassociation(IN P_ADAPTER_T prAdapter,
                                        IN P_STA_RECORD_T prStaRec,
                                        IN P_SW_RFB_T prSwRfb);

/* //////////////////////// TO BE REFINE ///////////////////// */
void p2pRoleFsmRunEventSwitchOPMode(IN P_ADAPTER_T prAdapter,
                                    IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventBeaconUpdate(IN P_ADAPTER_T prAdapter,
                                    IN P_MSG_HDR_T prMsgHdr);

void p2pRoleFsmRunEventDissolve(IN P_ADAPTER_T prAdapter,
                                IN P_MSG_HDR_T prMsgHdr);

void p2pProcessEvent_UpdateNOAParam(IN P_ADAPTER_T prAdapter,
                                    IN u8 ucBssIdx,
                                    IN P_EVENT_UPDATE_NOA_PARAMS_T
                                    prEventUpdateNoaParam);

void p2pRoleFsmRunEventScanAbort(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx);

void p2pRoleFsmStateTransition(IN P_ADAPTER_T prAdapter,
                               IN P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo,
                               IN ENUM_P2P_ROLE_STATE_T eNextState);

#endif
