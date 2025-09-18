/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "scan.h"
 *    \brief
 *
 */

#ifndef _SCAN_H
#define _SCAN_H

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

/*! Maximum buffer size of SCAN list */
#define SCN_MAX_BUFFER_SIZE    (CFG_MAX_NUM_BSS_LIST * \
                                ALIGN_4(sizeof(BSS_DESC_T)))

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
#define SCN_ROAM_MAX_BUFFER_SIZE \
    (CFG_MAX_NUM_ROAM_BSS_LIST * ALIGN_4(sizeof(ROAM_BSS_DESC_T)))
#endif

#define SCN_RM_POLICY_EXCLUDE_SPECIFIC_SSID \
    BIT(4)  /* Remove SCAN result except the connected one. */

#define SCN_RM_POLICY_EXCLUDE_CONNECTED \
    BIT(0)  /* Remove SCAN result except the connected one. */
#define SCN_RM_POLICY_TIMEOUT               BIT(1)  /* Remove the timeout one */
#define SCN_RM_POLICY_OLDEST_HIDDEN \
    BIT(2)  /* Remove the oldest one with hidden ssid */
#define SCN_RM_POLICY_SMART_WEAKEST \
    BIT(3)  /* If there are more than half BSS which has the \
             * same ssid as connection setting, remove the \
             * weakest one from them \
             * Else remove the weakest one. \
             */
#define SCN_RM_POLICY_ENTIRE                BIT(5)  /* Remove entire SCAN result */

#define SCN_BSS_DESC_SAME_SSID_THRESHOLD \
    3  /* This is used by POLICY SMART WEAKEST, \
        * If exceed this value, remove weakest BSS_DESC_T \
        * with same SSID first in large network. \
        */
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
#define REMOVE_TIMEOUT_TWO_DAY              (60 * 60 * 24 * 2)
#endif

#define SCN_BSS_DESC_REMOVE_TIMEOUT_SEC         30
#define SCN_BSS_DESC_STALE_SEC              20  /* Scan Request Timeout */

#define SCN_PROBE_DELAY_MSEC                0

#define SCN_ADHOC_BSS_DESC_TIMEOUT_SEC          5  /* Second. */

#define SCN_NLO_NETWORK_CHANNEL_NUM         (4)

#define SCAN_DONE_DIFFERENCE                3

/*----------------------------------------------------------------------------*/
/* MSG_SCN_SCAN_REQ                                                           */
/*----------------------------------------------------------------------------*/
#define SCAN_REQ_SSID_WILDCARD              BIT(0)
#define SCAN_REQ_SSID_P2P_WILDCARD          BIT(1)
#define SCAN_REQ_SSID_SPECIFIED             BIT(2)

/*----------------------------------------------------------------------------*/
/* Support Multiple SSID SCAN                                                 */
/*----------------------------------------------------------------------------*/
#define SCN_SSID_MAX_NUM                CFG_SCAN_SSID_MAX_NUM
#define SCN_SSID_MATCH_MAX_NUM              CFG_SCAN_SSID_MATCH_MAX_NUM

#define SCN_AGPS_AP_LIST_MAX_NUM            32

#define SCN_BSS_JOIN_FAIL_THRESOLD          5
#define SCN_BSS_JOIN_FAIL_CNT_RESET_SEC         15
#define SCN_BSS_JOIN_FAIL_RESET_STEP            2

#if CFG_SUPPORT_BATCH_SCAN
/*----------------------------------------------------------------------------*/
/* SCAN_BATCH_REQ                                                             */
/*----------------------------------------------------------------------------*/
#define SCAN_BATCH_REQ_START                BIT(0)
#define SCAN_BATCH_REQ_STOP             BIT(1)
#define SCAN_BATCH_REQ_RESULT               BIT(2)
#endif

#define SCAN_NLO_CHECK_SSID_ONLY            0x00000001
#define SCAN_NLO_DEFAULT_INTERVAL           30000

#define SCN_CTRL_SCAN_CHANNEL_LISTEN_TIME_ENABLE    BIT(1)
#define SCN_CTRL_IGNORE_AIS_FIX_CHANNEL         BIT(1)
#define SCN_CTRL_ENABLE                 BIT(0)

#define SCN_CTRL_DEFAULT_SCAN_CTRL \
    SCN_CTRL_IGNORE_AIS_FIX_CHANNEL

#define SCN_SCAN_DONE_PRINT_BUFFER_LENGTH       200
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

typedef enum _ENUM_SCAN_TYPE_T {
    SCAN_TYPE_PASSIVE_SCAN = 0,
    SCAN_TYPE_ACTIVE_SCAN,
    SCAN_TYPE_NUM
} ENUM_SCAN_TYPE_T,
*P_ENUM_SCAN_TYPE_T;

typedef enum _ENUM_SCAN_STATE_T {
    SCAN_STATE_IDLE = 0,
    SCAN_STATE_SCANNING,
    SCAN_STATE_NUM
} ENUM_SCAN_STATE_T;

typedef enum _ENUM_FW_SCAN_STATE_T {
    FW_SCAN_STATE_IDLE = 0,  /* 0 */
    FW_SCAN_STATE_SCAN_START,  /* 1 */
    FW_SCAN_STATE_REQ_CHANNEL,  /* 2 */
    FW_SCAN_STATE_SET_CHANNEL,  /* 3 */
    FW_SCAN_STATE_DELAYED_ACTIVE_PROB_REQ,  /* 4 */
    FW_SCAN_STATE_ACTIVE_PROB_REQ,  /* 5 */
    FW_SCAN_STATE_LISTEN,  /* 6 */
    FW_SCAN_STATE_SCAN_DONE,  /* 7 */
    FW_SCAN_STATE_NLO_START,  /* 8 */
    FW_SCAN_STATE_NLO_HIT_CHECK,  /* 9 */
    FW_SCAN_STATE_NLO_STOP,  /* 10 */
    FW_SCAN_STATE_BATCH_START,  /* 11 */
    FW_SCAN_STATE_BATCH_CHECK,  /* 12 */
    FW_SCAN_STATE_BATCH_STOP,  /* 13 */
    FW_SCAN_STATE_NUM  /* 14 */
} ENUM_FW_SCAN_STATE_T;

typedef enum _ENUM_SCAN_CHANNEL_T {
    SCAN_CHANNEL_FULL = 0,
    SCAN_CHANNEL_2G4,
    SCAN_CHANNEL_5G,
    SCAN_CHANNEL_P2P_SOCIAL,
    SCAN_CHANNEL_SPECIFIED,
    SCAN_CHANNEL_NUM
} ENUM_SCAN_CHANNEL,
*P_ENUM_SCAN_CHANNEL;

typedef struct _MSG_SCN_FSM_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u32 u4Dummy;
} MSG_SCN_FSM_T, *P_MSG_SCN_FSM_T;

/*----------------------------------------------------------------------------*/
/* BSS Descriptors                                                            */
/*----------------------------------------------------------------------------*/
struct _BSS_DESC_T {
    LINK_ENTRY_T rLinkEntry;

    u8 aucBSSID[MAC_ADDR_LEN];
    u8 aucSrcAddr[MAC_ADDR_LEN];  /* For IBSS, the SrcAddr is different from
                                   * BSSID */

    u8 fgIsConnecting;  /* If we are going to connect to this BSS
                         * (JOIN or ROAMING to another BSS), don't
                         * remove this record from BSS List.
                         */
    u8 fgIsConnected;  /* If we have connected to this BSS (NORMAL_TR),
                        * don't removed this record from BSS list.
                        */

    u8 fgIsHiddenSSID;  /* When this flag is true, means the SSID
                         * of this BSS is not known yet.
                         */
    u8 ucSSIDLen;
    u8 aucSSID[ELEM_MAX_LEN_SSID];

    u32 rUpdateTime;

    ENUM_BSS_TYPE_T eBSSType;

    u16 u2CapInfo;

    u16 u2BeaconInterval;
    u16 u2ATIMWindow;

    u16 u2OperationalRateSet;
    u16 u2BSSBasicRateSet;
    u8 fgIsUnknownBssBasicRate;

    u8 fgIsERPPresent;
    u8 fgIsHTPresent;
    u8 fgIsVHTPresent;

    u8 ucPhyTypeSet;  /* Available PHY Type Set of this BSS */
    u8 ucVhtCapNumSoundingDimensions;  /* record from bcn or probe response*/

    u8 ucChannelNum;

    ENUM_CHNL_EXT_T eSco;  /* Record bandwidth for association process */
    /*Some AP will send association resp by 40MHz BW */
    ENUM_CHANNEL_WIDTH_T eChannelWidth;  /*VHT operation ie */
    u8 ucCenterFreqS1;
    u8 ucCenterFreqS2;
    ENUM_BAND_T eBand;

    u8 ucDTIMPeriod;

    u8 fgIsLargerTSF;  /* This BSS's TimeStamp is larger than us(TCL == 1 in
                        * RX_STATUS_T) */

    u8 ucRCPI;

    u8 ucWmmFlag;  /* A flag to indicate this BSS's WMM capability */

    /*! \brief The srbiter Search State will matched the scan result,
     *   and saved the selected cipher and akm, and report the score,
     *   for arbiter join state, join module will carry this target BSS
     *   to rsn generate ie function, for gen wpa/rsn ie
     */
    u32 u4RsnSelectedGroupCipher;
    u32 u4RsnSelectedPairwiseCipher;
    u32 u4RsnSelectedAKMSuite;

    u16 u2RsnCap;

    RSN_INFO_T rRSNInfo;
    RSN_INFO_T rWPAInfo;

    WAPI_INFO_T rIEWAPI;
    u8 fgIEWAPI;

    u8 fgIERSN;
    u8 fgIEWPA;

    /*! \brief RSN parameters selected for connection */
    /*! \brief The Select score for final AP selection,
     *  0, no sec, 1,2,3 group cipher is WEP, TKIP, CCMP
     */
    u8 ucEncLevel;

#if CFG_ENABLE_WIFI_DIRECT
    u8 fgIsP2PPresent;
    u8 fgIsP2PReport;  /* true: report to upper layer */
    P_P2P_DEVICE_DESC_T prP2pDesc;

    u8 aucIntendIfAddr[MAC_ADDR_LEN];  /* For IBSS, the SrcAddr is different
                                        * from BSSID */
    /* u8 ucDevCapabilityBitmap; */ /* Device Capability Attribute.
     *                                 (P2P_DEV_CAPABILITY_XXXX)
     */
    /* u8 ucGroupCapabilityBitmap; */ /* Group Capability Attribute.
     *                                   (P2P_GROUP_CAPABILITY_XXXX)
     */

    LINK_T rP2pDeviceList;

    /* P_LINK_T prP2pDeviceList; */

    /* For
     *    1. P2P Capability.
     *    2. P2P Device ID. ( in aucSrcAddr[] )
     *    3. NOA   (TODO:)
     *    4. Extend Listen Timing. (Probe Rsp)  (TODO:)
     *    5. P2P Device Info. (Probe Rsp)
     *    6. P2P Group Info. (Probe Rsp)
     */
#endif

    u8 fgIsIEOverflow;  /* The received IE length exceed the maximum IE
                         * buffer size */
    u16 u2RawLength;  /* The byte count of aucRawBuf[] */
    u16 u2IELength;  /* The byte count of aucIEBuf[] */

    ULARGE_INTEGER
        u8TimeStamp;  /* Place u8TimeStamp before aucIEBuf[1] to force DW align
                       */
    u8 aucRawBuf[CFG_RAW_BUFFER_SIZE];
    u8 aucIEBuf[CFG_IE_BUFFER_SIZE];
    u8 ucJoinFailureCount;
    u32 rJoinFailTime;
#if CFG_SUPPORT_802_11K
    u8 aucRrmCap[5];
#endif
};

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
struct _ROAM_BSS_DESC_T {
    LINK_ENTRY_T rLinkEntry;
    u8 ucSSIDLen;
    u8 aucSSID[ELEM_MAX_LEN_SSID];
    u32 rUpdateTime;
};
#endif

typedef struct _SCHED_SCAN_PARAM {  /* Used by SCAN FSM */
    u8 ucSeqNum;
    u8 ucBssIndex;  /* Network Type */
    u8 fgStopAfterIndication;  /* always false */
    u8 ucMatchSSIDNum;  /* Match SSID */
    P_BSS_DESC_T aprPendingBssDescToInd[SCN_SSID_MATCH_MAX_NUM];
} SCHED_SCAN_PARAM_T, *P_SCHED_SCAN_PARAM_T;

typedef struct _SCAN_PARAM_T {  /* Used by SCAN FSM */
    /* Active or Passive */
    ENUM_SCAN_TYPE_T eScanType;

    /* Network Type */
    u8 ucBssIndex;

    /* Specified SSID Type */
    u8 ucSSIDType;
    u8 ucSSIDNum;

    /* Length of Specified SSID */
    u8 ucSpecifiedSSIDLen[SCN_SSID_MAX_NUM];

    /* Specified SSID */
    u8 aucSpecifiedSSID[SCN_SSID_MAX_NUM][ELEM_MAX_LEN_SSID];

#if CFG_ENABLE_WIFI_DIRECT
    u8 fgFindSpecificDev;  /* P2P: Discovery Protocol */
    u8 aucDiscoverDevAddr[MAC_ADDR_LEN];
    u8 fgIsDevType;
    P2P_DEVICE_TYPE_T rDiscoverDevType;

    /* TODO: Find Specific Device Type. */
#endif

    u16 u2ChannelDwellTime;
    u16 u2TimeoutValue;

    u8 fgIsObssScan;
    u8 fgIsScanV2;

    /* Run time flags */
    u16 u2ProbeDelayTime;

    /* channel information */
    ENUM_SCAN_CHANNEL eScanChannel;
    u8 ucChannelListNum;
    RF_CHANNEL_INFO_T arChnlInfoList[MAXIMUM_OPERATION_CHANNEL_LIST];

    /* Feedback information */
    u8 ucSeqNum;

    /* Information Element */
    u16 u2IELen;
    u8 aucIE[MAX_IE_LENGTH];
} SCAN_PARAM_T, *P_SCAN_PARAM_T;

typedef struct _NLO_PARAM_T {  /* Used by SCAN FSM */
    SCAN_PARAM_T rScanParam;

    /* NLO */
    u8 fgStopAfterIndication;
    u8 ucFastScanIteration;
    u16 u2FastScanPeriod;
    u16 u2SlowScanPeriod;

    /* Match SSID */
    u8 ucMatchSSIDNum;
    u8 ucMatchSSIDLen[SCN_SSID_MATCH_MAX_NUM];
    u8 aucMatchSSID[SCN_SSID_MATCH_MAX_NUM][ELEM_MAX_LEN_SSID];

    u8 aucCipherAlgo[SCN_SSID_MATCH_MAX_NUM];
    u16 au2AuthAlgo[SCN_SSID_MATCH_MAX_NUM];
    u8 aucChannelHint[SCN_SSID_MATCH_MAX_NUM][SCN_NLO_NETWORK_CHANNEL_NUM];
} NLO_PARAM_T, *P_NLO_PARAM_T;

typedef struct _SCAN_INFO_T {
    ENUM_SCAN_STATE_T
        eCurrentState;  /* Store the STATE variable of SCAN FSM */

    u32 rLastScanCompletedTime;

    SCAN_PARAM_T rScanParam;
    NLO_PARAM_T rNloParam;

    u32 u4NumOfBssDesc;

    u8 aucScanBuffer[SCN_MAX_BUFFER_SIZE];

    LINK_T rBSSDescList;

    LINK_T rFreeBSSDescList;

    LINK_T rPendingMsgList;
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
    u8 aucScanRoamBuffer[SCN_ROAM_MAX_BUFFER_SIZE];
    LINK_T rRoamFreeBSSDescList;
    LINK_T rRoamBSSDescList;
#endif
    /* Sparse Channel Detection */
    u8 fgIsSparseChannelValid;
    RF_CHANNEL_INFO_T rSparseChannel;

    /* NLO scanning state tracking */
    u8 fgNloScanning;

    /*channel idle count # Mike */
    u8 ucSparseChannelArrayValidNum;
    u8 aucReserved[3];
    u8 aucChannelNum[64];
    u16 au2ChannelIdleTime[64];
    u8 aucChannelFlag[64];
    u8 aucChannelMDRDYCnt[64];

    SCHED_SCAN_PARAM_T rSchedScanParam;
} SCAN_INFO_T, *P_SCAN_INFO_T;

/* Incoming Mailbox Messages */
typedef struct _MSG_SCN_SCAN_REQ_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucSeqNum;
    u8 ucBssIndex;
    ENUM_SCAN_TYPE_T eScanType;
    u8 ucSSIDType;  /* BIT(0) wildcard / BIT(1) P2P-wildcard / BIT(2)
                     * specific */
    u8 ucSSIDLength;
    u8 aucSSID[PARAM_MAX_LEN_SSID];
    u16 u2ChannelDwellTime;  /* ms unit */
    u16 u2TimeoutValue;  /* ms unit */
    ENUM_SCAN_CHANNEL eScanChannel;
    u8 ucChannelListNum;
    RF_CHANNEL_INFO_T arChnlInfoList[MAXIMUM_OPERATION_CHANNEL_LIST];
    u16 u2IELen;
    u8 aucIE[MAX_IE_LENGTH];
} MSG_SCN_SCAN_REQ, *P_MSG_SCN_SCAN_REQ;

typedef struct _MSG_SCN_SCAN_REQ_V2_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucSeqNum;
    u8 ucBssIndex;
    ENUM_SCAN_TYPE_T eScanType;
    u8 ucSSIDType;  /* BIT(0) wildcard / BIT(1) P2P-wildcard / BIT(2)
                     * specific */
    u8 ucSSIDNum;
    P_PARAM_SSID_T prSsid;
    u16 u2ProbeDelay;
    u16 u2ChannelDwellTime;  /* In TU. 1024us. */
    u16 u2TimeoutValue;  /* ms unit */
    ENUM_SCAN_CHANNEL eScanChannel;
    u8 ucChannelListNum;
    RF_CHANNEL_INFO_T arChnlInfoList[MAXIMUM_OPERATION_CHANNEL_LIST];
    u16 u2IELen;
    u8 aucIE[MAX_IE_LENGTH];
} MSG_SCN_SCAN_REQ_V2, *P_MSG_SCN_SCAN_REQ_V2;

typedef struct _MSG_SCN_SCAN_CANCEL_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucSeqNum;
    u8 ucBssIndex;
    u8 fgIsChannelExt;
    u8 fgIsOidRequest;
} MSG_SCN_SCAN_CANCEL, *P_MSG_SCN_SCAN_CANCEL;

typedef struct _tagOFFLOAD_NETWORK {
    u8 aucSsid[ELEM_MAX_LEN_SSID];
    u8 ucSsidLen;
    u8 ucUnicastCipher;  /* ENUM_NLO_CIPHER_ALGORITHM */
    u16 u2AuthAlgo;  /* ENUM_NLO_AUTH_ALGORITHM */
    u8 aucChannelList[SCN_NLO_NETWORK_CHANNEL_NUM];
} OFFLOAD_NETWORK, *P_OFFLOAD_NETWORK;

typedef struct _MSG_SCN_NLO_REQ_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 fgStopAfterIndication;
    u8 ucSeqNum;
    u8 ucBssIndex;
    u32 u4FastScanPeriod;
    u32 u4FastScanIterations;
    u32 u4SlowScanPeriod;
    u32 u4NumOfEntries;
    OFFLOAD_NETWORK arNetwork[CFG_SCAN_SSID_MAX_NUM];
} MSG_SCN_NLO_REQ, *P_MSG_SCN_NLO_REQ;

typedef struct _MSG_SCN_NLO_CANCEL_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucSeqNum;
    u8 ucBssIndex;
} MSG_SCN_NLO_CANCEL, *P_MSG_SCN_NLO_CANCEL;

/* Outgoing Mailbox Messages */
typedef enum _ENUM_SCAN_STATUS_T {
    SCAN_STATUS_DONE = 0,
    SCAN_STATUS_CANCELLED,
    SCAN_STATUS_FAIL,
    SCAN_STATUS_BUSY,
    SCAN_STATUS_NUM
} ENUM_SCAN_STATUS,
*P_ENUM_SCAN_STATUS;

typedef struct _MSG_SCN_SCAN_DONE_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucSeqNum;
    u8 ucBssIndex;
    ENUM_SCAN_STATUS eScanStatus;
} MSG_SCN_SCAN_DONE, *P_MSG_SCN_SCAN_DONE;

typedef enum {
    AGPS_PHY_A,
    AGPS_PHY_B,
    AGPS_PHY_G,
} AP_PHY_TYPE;

typedef struct _AGPS_AP_INFO_T {
    u8 aucBSSID[MAC_ADDR_LEN];
    s16 i2ApRssi;  /* -127..128 */
    u16 u2Channel;  /* 0..256 */
    AP_PHY_TYPE ePhyType;
} AGPS_AP_INFO_T, *P_AGPS_AP_INFO_T;

typedef struct _AGPS_AP_LIST_T {
    u8 ucNum;
    AGPS_AP_INFO_T arApInfo[SCN_AGPS_AP_LIST_MAX_NUM];
} AGPS_AP_LIST_T, *P_AGPS_AP_LIST_T;

typedef enum _ENUM_NLO_STATUS_T {
    NLO_STATUS_FOUND = 0,
    NLO_STATUS_CANCELLED,
    NLO_STATUS_FAIL,
    NLO_STATUS_BUSY,
    NLO_STATUS_NUM
} ENUM_NLO_STATUS,
*P_ENUM_NLO_STATUS;

typedef struct _MSG_SCN_NLO_DONE_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucSeqNum;
    u8 ucBssIndex;
    ENUM_NLO_STATUS eNloStatus;
} MSG_SCN_NLO_DONE, *P_MSG_SCN_NLO_DONE;

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

/*----------------------------------------------------------------------------*/
/* Routines in scan.c                                                         */
/*----------------------------------------------------------------------------*/
void scnInit(IN P_ADAPTER_T prAdapter);

void scnUninit(IN P_ADAPTER_T prAdapter);

/* BSS-DESC Search */
P_BSS_DESC_T scanSearchBssDescByBssid(IN P_ADAPTER_T prAdapter,
                                      IN u8 aucBSSID[]);

P_BSS_DESC_T
scanSearchBssDescByBssidAndSsid(IN P_ADAPTER_T prAdapter,
                                IN u8 aucBSSID[],
                                IN u8 fgCheckSsid,
                                IN P_PARAM_SSID_T prSsid);

P_BSS_DESC_T
scanSearchBssDescByBssidAndChanNum(IN P_ADAPTER_T prAdapter,
                                   IN u8 aucBSSID[],
                                   IN u8 fgCheckChanNum,
                                   IN u8 ucChannelNum);

P_BSS_DESC_T scanSearchBssDescByTA(IN P_ADAPTER_T prAdapter,
                                   IN u8 aucSrcAddr[]);

P_BSS_DESC_T
scanSearchBssDescByTAAndSsid(IN P_ADAPTER_T prAdapter,
                             IN u8 aucSrcAddr[],
                             IN u8 fgCheckSsid,
                             IN P_PARAM_SSID_T prSsid);

/* BSS-DESC Search - Alternative */
P_BSS_DESC_T
scanSearchExistingBssDesc(IN P_ADAPTER_T prAdapter,
                          IN ENUM_BSS_TYPE_T eBSSType,
                          IN u8 aucBSSID[],
                          IN u8 aucSrcAddr[]);

P_BSS_DESC_T
scanSearchExistingBssDescWithSsid(IN P_ADAPTER_T prAdapter,
                                  IN ENUM_BSS_TYPE_T eBSSType,
                                  IN u8 aucBSSID[],
                                  IN u8 aucSrcAddr[],
                                  IN u8 fgCheckSsid,
                                  IN P_PARAM_SSID_T prSsid);

/* BSS-DESC Allocation */
P_BSS_DESC_T scanAllocateBssDesc(IN P_ADAPTER_T prAdapter);

/* BSS-DESC Removal */
void scanRemoveBssDescsByPolicy(IN P_ADAPTER_T prAdapter,
                                IN u32 u4RemovePolicy);

void scanRemoveBssDescByBssid(IN P_ADAPTER_T prAdapter, IN u8 aucBSSID[]);

void scanRemoveBssDescByBandAndNetwork(IN P_ADAPTER_T prAdapter,
                                       IN ENUM_BAND_T eBand,
                                       IN u8 ucBssIndex);

/* BSS-DESC State Change */
void scanRemoveConnFlagOfBssDescByBssid(IN P_ADAPTER_T prAdapter,
                                        IN u8 aucBSSID[]);

/* BSS-DESC Insertion - ALTERNATIVE */
P_BSS_DESC_T scanAddToBssDesc(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

WLAN_STATUS scanProcessBeaconAndProbeResp(IN P_ADAPTER_T prAdapter,
                                          IN P_SW_RFB_T prSWRfb);

void scanBuildProbeReqFrameCommonIEs(IN P_MSDU_INFO_T prMsduInfo,
                                     IN u8 *pucDesiredSsid,
                                     IN u32 u4DesiredSsidLen,
                                     IN u16 u2SupportedRateSet);

WLAN_STATUS scanSendProbeReqFrames(IN P_ADAPTER_T prAdapter,
                                   IN P_SCAN_PARAM_T prScanParam);

void scanUpdateBssDescForSearch(IN P_ADAPTER_T prAdapter,
                                IN P_BSS_DESC_T prBssDesc);

P_BSS_DESC_T scanSearchBssDescByPolicy(IN P_ADAPTER_T prAdapter,
                                       IN u8 ucBssIndex);

WLAN_STATUS scanAddScanResult(IN P_ADAPTER_T prAdapter,
                              IN P_BSS_DESC_T prBssDesc,
                              IN P_SW_RFB_T prSwRfb);

void scanReportBss2Cfg80211(IN P_ADAPTER_T prAdapter,
                            IN ENUM_BSS_TYPE_T eBSSType,
                            IN P_BSS_DESC_T SpecificprBssDesc);

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
P_ROAM_BSS_DESC_T scanSearchRoamBssDescBySsid(IN P_ADAPTER_T prAdapter,
                                              IN P_BSS_DESC_T prBssDesc);
P_ROAM_BSS_DESC_T scanAllocateRoamBssDesc(IN P_ADAPTER_T prAdapter);
void scanAddToRoamBssDesc(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBssDesc);
void scanSearchBssDescOfRoamSsid(IN P_ADAPTER_T prAdapter);
void scanRemoveRoamBssDescsByTime(IN P_ADAPTER_T prAdapter,
                                  IN u32 u4RemoveTime);
#endif
/*----------------------------------------------------------------------------*/
/* Routines in scan_fsm.c                                                     */
/*----------------------------------------------------------------------------*/
void scnFsmSteps(IN P_ADAPTER_T prAdapter, IN ENUM_SCAN_STATE_T eNextState);

/*----------------------------------------------------------------------------*/
/* Command Routines                                                           */
/*----------------------------------------------------------------------------*/
void scnSendScanReq(IN P_ADAPTER_T prAdapter);

void scnSendScanReqV2(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* RX Event Handling                                                          */
/*----------------------------------------------------------------------------*/
void scnEventScanDone(IN P_ADAPTER_T prAdapter,
                      IN P_EVENT_SCAN_DONE prScanDone,
                      u8 fgIsNewVersion);

void scnEventNloDone(IN P_ADAPTER_T prAdapter, IN P_EVENT_NLO_DONE_T prNloDone);

/*----------------------------------------------------------------------------*/
/* Mailbox Message Handling                                                   */
/*----------------------------------------------------------------------------*/
void scnFsmMsgStart(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);

void scnFsmMsgAbort(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);

void scnFsmHandleScanMsg(IN P_ADAPTER_T prAdapter,
                         IN P_MSG_SCN_SCAN_REQ prScanReqMsg);

void scnFsmHandleScanMsgV2(IN P_ADAPTER_T prAdapter,
                           IN P_MSG_SCN_SCAN_REQ_V2 prScanReqMsg);

void scnFsmRemovePendingMsg(IN P_ADAPTER_T prAdapter,
                            IN u8 ucSeqNum,
                            IN u8 ucBssIndex);

void scnFsmNloMsgStart(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);

void scnFsmNloMsgAbort(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);

void scnFsmHandleNloMsg(IN P_ADAPTER_T prAdapter,
                        IN P_MSG_SCN_NLO_REQ prNloReqMsg);

/*----------------------------------------------------------------------------*/
/* Mailbox Message Generation                                                 */
/*----------------------------------------------------------------------------*/
void scnFsmGenerateScanDoneMsg(IN P_ADAPTER_T prAdapter,
                               IN u8 ucSeqNum,
                               IN u8 ucBssIndex,
                               IN ENUM_SCAN_STATUS eScanStatus);

/*----------------------------------------------------------------------------*/
/* Query for sparse channel                                                   */
/*----------------------------------------------------------------------------*/
u8 scnQuerySparseChannel(IN P_ADAPTER_T prAdapter,
                         P_ENUM_BAND_T prSparseBand,
                         u8 *pucSparseChannel);

/*----------------------------------------------------------------------------*/
/* OID/IOCTL Handling                                                         */
/*----------------------------------------------------------------------------*/
u8 scnFsmSchedScanRequest(IN P_ADAPTER_T prAdapter,
                          IN u8 ucSsidNum,
                          IN P_PARAM_SSID_T prSsid,
                          IN u32 u4IeLength,
                          IN u8 *pucIe,
                          IN u16 u2Interval);

u8 scnFsmSchedScanStopRequest(IN P_ADAPTER_T prAdapter);

void scanReportScanResultToAgps(P_ADAPTER_T prAdapter);

#if CFG_SCAN_CHANNEL_SPECIFIED
static inline bool is_valid_scan_chnl_cnt(u8 num)
{
    return num && num < MAXIMUM_OPERATION_CHANNEL_LIST;
}
#endif

u8 scnFsmIsScanning(IN P_ADAPTER_T prAdapter);

#endif
