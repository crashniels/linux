/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "cnm_mem.h"
 *    \brief  In this file we define the structure of the control unit of
 *     packet buffer and MGT/MSG Memory Buffer.
 */

#ifndef _CNM_MEM_H
#define _CNM_MEM_H

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

#ifndef POWER_OF_2
#define POWER_OF_2(n)    BIT(n)
#endif

/* Size of a basic management buffer block in power of 2 */
#define MGT_BUF_BLOCK_SIZE_IN_POWER_OF_2    7  /* 7 to the power of 2 = 128 */
#define MSG_BUF_BLOCK_SIZE_IN_POWER_OF_2    5  /* 5 to the power of 2 = 32 */

/* Size of a basic management buffer block */
#define MGT_BUF_BLOCK_SIZE          POWER_OF_2( \
        MGT_BUF_BLOCK_SIZE_IN_POWER_OF_2)
#define MSG_BUF_BLOCK_SIZE          POWER_OF_2( \
        MSG_BUF_BLOCK_SIZE_IN_POWER_OF_2)

/* Total size of (n) basic management buffer blocks */
#define MGT_BUF_BLOCKS_SIZE(n)    ((u32)(n) << MGT_BUF_BLOCK_SIZE_IN_POWER_OF_2)
#define MSG_BUF_BLOCKS_SIZE(n)    ((u32)(n) << MSG_BUF_BLOCK_SIZE_IN_POWER_OF_2)

/* Number of management buffer block */
#define MAX_NUM_OF_BUF_BLOCKS           32  /* Range: 1~32 */

/* Size of overall management frame buffer */
#define MGT_BUFFER_SIZE             (MAX_NUM_OF_BUF_BLOCKS * \
                                     MGT_BUF_BLOCK_SIZE)
#define MSG_BUFFER_SIZE             (MAX_NUM_OF_BUF_BLOCKS * \
                                     MSG_BUF_BLOCK_SIZE)

/* STA_REC related definitions */
#define STA_REC_INDEX_BMCAST            0xFF
#define STA_REC_INDEX_NOT_FOUND         0xFE
#define STA_WAIT_QUEUE_NUM \
    5  /* Number of SW queues in each STA_REC: AC0~AC4 \
        */
#define SC_CACHE_INDEX_NUM \
    5  /* Number of SC caches in each STA_REC: AC0~AC4 \
        */

/* P2P related definitions */
#ifdef CFG_ENABLE_WIFI_DIRECT
/* Moved from p2p_fsm.h */
#define WPS_ATTRI_MAX_LEN_DEVICE_NAME       32  /* 0x1011 */
#define P2P_GC_MAX_CACHED_SEC_DEV_TYPE_COUNT    8  /* NOTE(Kevin): Shall <= 16 */
#endif

/* Define the argument of cnmStaFreeAllStaByNetwork when all station records
 * will be free. No one will be free
 */
#define STA_REC_EXCLUDE_NONE            CFG_STA_REC_NUM

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

#if ((MAX_NUM_OF_BUF_BLOCKS > 32) || (MAX_NUM_OF_BUF_BLOCKS <= 0))
#error > #define MAX_NUM_OF_MGT_BUF_BLOCKS : Out of boundary !
#elif MAX_NUM_OF_BUF_BLOCKS > 16
typedef u32 BUF_BITMAP;
#elif MAX_NUM_OF_BUF_BLOCKS > 8
typedef u16 BUF_BITMAP;
#else
typedef u8 BUF_BITMAP;
#endif

/* Control variable of TX management memory pool */
typedef struct _BUF_INFO_T {
    u8 *pucBuf;

#if CFG_DBG_MGT_BUF
    u32 u4AllocCount;
    u32 u4FreeCount;
    u32 u4AllocNullCount;
#endif

    BUF_BITMAP rFreeBlocksBitmap;
    u8 aucAllocatedBlockNum[MAX_NUM_OF_BUF_BLOCKS];
} BUF_INFO_T, *P_BUF_INFO_T;

/* Wi-Fi divides RAM into three types
 * MSG:     Mailbox message (Small size)
 * BUF:     HW DMA buffers (HIF/MAC)
 */
typedef enum _ENUM_RAM_TYPE_T {
    RAM_TYPE_MSG = 0,
    RAM_TYPE_BUF
} ENUM_RAM_TYPE_T,
P_ENUM_RAM_TYPE_T;

typedef enum _ENUM_BUFFER_SOURCE_T {
    BUFFER_SOURCE_HIF_TX0 = 0,
    BUFFER_SOURCE_HIF_TX1,
    BUFFER_SOURCE_MAC_RX,
    BUFFER_SOURCE_MNG,
    BUFFER_SOURCE_BCN,
    BUFFER_SOURCE_NUM
} ENUM_BUFFER_SOURCE_T,
*P_ENUM_BUFFER_SOURCE_T;

typedef enum _ENUM_SEC_STATE_T {
    SEC_STATE_INIT,
    SEC_STATE_INITIATOR_PORT_BLOCKED,
    SEC_STATE_RESPONDER_PORT_BLOCKED,
    SEC_STATE_CHECK_OK,
    SEC_STATE_SEND_EAPOL,
    SEC_STATE_SEND_DEAUTH,
    SEC_STATE_COUNTERMEASURE,
    SEC_STATE_NUM
} ENUM_SEC_STATE_T;

typedef struct _TSPEC_ENTRY_T {
    u8 ucStatus;
    u8 ucToken;  /* Dialog Token in ADDTS_REQ or ADDTS_RSP */
    u16 u2MediumTime;
    u32 u4TsInfo;
    /* PARAM_QOS_TS_INFO rParamTsInfo; */
    /* Add other retained QoS parameters below */
} TSPEC_ENTRY_T, *P_TSPEC_ENTRY_T, TSPEC_TABLE_ENTRY_T, *P_TSPEC_TABLE_ENTRY_T;

#define MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS    3

#define UPDATE_BSS_RSSI_INTERVAL_SEC           3  /* Seconds */

/* Fragment information structure */
typedef struct _FRAG_INFO_T {
    u16 u2SeqNo;
    u8 ucNextFragNo;
#if CFG_SUPPORT_FRAG_ATTACK_DETECTION
    u8 ucSecMode;
    u64 u8NextPN;
#endif
    u8 *pucNextFragStart;
    P_SW_RFB_T pr1stFrag;
    u32 rReceiveLifetimeLimit;  /* The receive time of 1st fragment */
} FRAG_INFO_T, *P_FRAG_INFO_T;

#if CFG_SUPPORT_802_11W
/* AP PMF */
struct AP_PMF_CFG {
    u8 fgMfpc;
    u8 fgMfpr;
    u8 fgSha256;
    u8 fgAPApplyPmfReq;
    u8 fgBipKeyInstalled;
};

struct STA_PMF_CFG {
    u8 fgMfpc;
    u8 fgMfpr;
    u8 fgSha256;
    u8 fgApplyPmf;
    u8 fgBipKeyInstalled;
    u8 fgRxDeauthResp;  /* for certification 4.3.3.1, 4.3.3.2 TX unprotected
                         * deauth */

    /* For PMF SA query TX request retry a timer */
    u32 u4SAQueryStart;  /* record the start time of 1st SAQ request */
    u32 u4SAQueryCount;
    u8 ucSAQueryTimedOut;  /* retry more than 1000ms */
    TIMER_T rSAQueryTimer;
    u16 u2TransactionID;
};
#endif

/* Define STA record structure */
struct _STA_RECORD_T {
    LINK_ENTRY_T rLinkEntry;
    u8 ucIndex;  /* Not modify it except initializing */
    u8 ucWlanIndex;  /* WLAN table index */

    /* u8 ucBMCWlanIndex; */ /* The BSS STA Rx WLAN index,
     * IBSS Rx BC WLAN table index,
     * work at IBSS Open and WEP
     */

    u8 fgIsInUse;  /* Indicate if this entry is in use or not */
    u8 aucMacAddr[MAC_ADDR_LEN];  /* MAC address */

    /* SAA/AAA */
    ENUM_AA_STATE_T eAuthAssocState;  /* Store STATE Value used in SAA/AAA */
    u8 ucAuthAssocReqSeqNum;

    ENUM_STA_TYPE_T eStaType;  /* Indicate the role of this STA in
                                * the network (for example, P2P GO)
                                */

    u8 ucBssIndex;  /* BSS_INFO_I index */

    u8 ucStaState;  /* STATE_1,2,3 */

    u8 ucPhyTypeSet;  /* Available PHY Type Set of this peer
                       * (may deduced from received BSS_DESC_T)
                       */
    u8 ucVhtCapNumSoundingDimensions;  /* record from bcn or probe response*/

    u8 ucDesiredPhyTypeSet;  /* The match result by AND operation of peer's
                              * PhyTypeSet and ours.
                              */
    u8 fgHasBasicPhyType;  /* A flag to indicate a Basic Phy Type which
                            * is used to generate some Phy Attribute IE
                            * (e.g. capability, MIB) during association.
                            */
    u8 ucNonHTBasicPhyType;  /* The Basic Phy Type chosen among the
                              * ucDesiredPhyTypeSet.
                              */

    u16 u2HwDefaultFixedRateCode;

    u16 u2CapInfo;  /* For Infra Mode, to store Capability Info. from
                     * Association Resp(SAA). For AP Mode, to store
                     * Capability Info. from Association Req(AAA).
                     */
    u16 u2AssocId;  /* For Infra Mode, to store AID from Association
                     * Resp(SAA). For AP Mode, to store the Assigned
                     * AID(AAA).
                     */

    u16 u2ListenInterval;  /* Listen Interval from STA(AAA) */

    u16 u2DesiredNonHTRateSet;  /* Our Current Desired Rate Set after
                                 * match with STA's Operational Rate Set
                                 */

    u16 u2OperationalRateSet;  /* Operational Rate Set of peer BSS */
    u16 u2BSSBasicRateSet;  /* Basic Rate Set of peer BSS */

    u8 fgIsMerging;  /* For IBSS Mode, to indicate that Merge is ongoing */

    u8 fgDiagnoseConnection;  /* For Infra/AP Mode, to diagnose the
                               * Connection with */
    /* this peer by sending ProbeReq/Null frame */

    /*------------------------------------------------------------------------------------------*/
    /* 802.11n HT capabilities when (prStaRec->ucPhyTypeSet &
     * PHY_TYPE_BIT_HT) is true          */
    /* They have the same definition with fields of information element */
    /*------------------------------------------------------------------------------------------*/
    u8 ucMcsSet;  /* MCS0~7 rate set of peer BSS */
    u8 fgSupMcs32;  /* MCS32 is supported by peer BSS */
    u8 aucRxMcsBitmask[SUP_MCS_RX_BITMASK_OCTET_NUM];
    u16 u2RxHighestSupportedRate;
    u32 u4TxRateInfo;
    u16 u2HtCapInfo;  /* HT cap info field by HT cap IE */
    u8 ucAmpduParam;  /* Field A-MPDU Parameters in HT cap IE */
    u16 u2HtExtendedCap;  /* HT extended cap field by HT cap IE */
    u32 u4TxBeamformingCap;  /* TX beamforming cap field by HT cap IE */
    u8 ucAselCap;  /* ASEL cap field by HT cap IE */
    /* CFG_SUPPORT_802_11AC */
    /*------------------------------------------------------------------------------------------*/
    /* 802.11ac  VHT capabilities when (prStaRec->ucPhyTypeSet &
     * PHY_TYPE_BIT_VHT) is true          */
    /* They have the same definition with fields of information element */
    /*------------------------------------------------------------------------------------------*/
    u32 u4VhtCapInfo;
    u16 u2VhtRxMcsMap;
    u16 u2VhtRxHighestSupportedDataRate;
    u16 u2VhtTxMcsMap;
    u16 u2VhtTxHighestSupportedDataRate;
    u8 ucVhtOpMode;

    u8 ucRCPI;  /* RCPI of peer */

    u8 ucDTIMPeriod;  /* Target BSS's DTIM Period, we use this
                       * value for setup Listen Interval
                       * TODO(Kevin): TBD
                       */
    u8 ucAuthAlgNum;  /* For Infra/AP Mode, the Auth Algorithm Num used in
                       * Authentication(SAA/AAA) */
    u8 fgIsReAssoc;  /* For Infra/AP Mode, to indicate ReAssoc Frame was in
                      * used(SAA/AAA) */

    u8 ucTxAuthAssocRetryCount;  /* For Infra Mode, the Retry Count of TX
                                  * Auth/Assod Frame(SAA) */
    u8 ucTxAuthAssocRetryLimit;  /* For Infra Mode, the Retry Limit of TX
                                  * Auth/Assod Frame(SAA) */

    /* Record what we sent for retry TX Auth/Assoc without SAA FSM */
    enum ENUM_AA_SENT_T eAuthAssocSent;

    u16 u2StatusCode;  /* Status of Auth/Assoc Req */
    u16 u2ReasonCode;  /* Reason that been Deauth/Disassoc */
    u8 fgIsLocallyGenerated;

    /* Point to an allocated buffer for storing Challenge */
    /* Text for Shared Key Authentication */
    P_IE_CHALLENGE_TEXT_T prChallengeText;

    /* For Infra Mode, a timer used to send a timeout event
     * while waiting for TX request done or RX response.
     */
    TIMER_T rTxReqDoneOrRxRespTimer;

    /* For Infra Mode, a timer used to avoid the Deauth frame
     * not be sent
     */
    TIMER_T rDeauthTxDoneTimer;

    /*------------------------------------------------------------------------------------------*/
    /* Power Management related fields  (for STA/ AP/ P2P/ BOW power saving
     * mode)           */
    /*------------------------------------------------------------------------------------------*/
    u8 fgSetPwrMgtBit;  /* For Infra Mode, to indicate that outgoing frame
                         * need toggle the Pwr Mgt Bit in its Frame Control
                         * Field.
                         */

    u8 fgIsInPS;  /* For AP Mode, to indicate the client PS state(PM).
                   * true: In PS Mode; false: In Active Mode.
                   */

    u8 fgIsInPsPollSP;  /* For Infra Mode, to indicate we've sent a PS POLL
                         * to AP and start the PS_POLL Service Period(LP)
                         */

    u8 fgIsInTriggerSP;  /* For Infra Mode, to indicate we've sent a Trigger
                          * Frame to AP and start the Delivery Service
                          * Period(LP)
                          */

    u8 ucBmpDeliveryAC;  /* 0: AC0, 1: AC1, 2: AC2, 3: AC3 */

    u8 ucBmpTriggerAC;  /* 0: AC0, 1: AC1, 2: AC2, 3: AC3 */

    u8 ucUapsdSp;  /* Max SP length */

    /*------------------------------------------------------------------------------------------*/

    u8 fgIsRtsEnabled;

    u32 rUpdateTime;  /* (4) System Timestamp of Successful TX and RX  */

    u32 rLastJoinTime;  /* (4) System Timestamp of latest JOIN process */

    u8 ucJoinFailureCount;  /* Retry Count of JOIN process */

    LINK_T arStaWaitQueue[STA_WAIT_QUEUE_NUM];  /* For TXM to defer pkt
                                                 * forwarding to MAC TX DMA
                                                 */

    /* Duplicate removal for HT STA on a
     * per-TID basis ("+1" is for MMPDU and non-QoS)
     */
    u16 au2CachedSeqCtrl[TID_NUM + 1];

    u8 afgIsIgnoreAmsduDuplicate[TID_NUM + 1];

#if CFG_SUPPORT_AMSDU_ATTACK_DETECTION
    u16 au2AmsduInvalidSN[TID_NUM + 1];
    u8 afgIsAmsduInvalid[TID_NUM + 1];
#endif

    FRAG_INFO_T rFragInfo[MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS];

    /* SEC_INFO_T              rSecInfo; */ /* The security state machine */

#if CFG_SUPPORT_ADHOC
    u8 fgAdhocRsnBcKeyExist[2];  /* Ad-hoc RSN Rx BC key exist flag,
                                  * only reserved two entry for each peer
                                  */
    u8 ucAdhocRsnBcWlanIndex[2];  /* Ad-hoc RSN Rx BC wlan index */
#endif

    u8 fgPortBlock;  /* The 802.1x Port Control flag */

    u8 fgTransmitKeyExist;  /* Unicast key exist for this STA */

    u8 fgTxAmpduEn;  /* Enable TX AMPDU for this Peer */
    u8 fgRxAmpduEn;  /* Enable RX AMPDU for this Peer */

    u8 *pucAssocReqIe;
    u16 u2AssocReqIeLen;

    WIFI_WMM_AC_STAT_T
        arLinkStatistics[WMM_AC_INDEX_NUM];  /*link layer satatistics */

    /*------------------------------------------------------------------------------------------*/
    /* WMM/QoS related fields */
    /*------------------------------------------------------------------------------------------*/
    u8 fgIsQoS;  /* If the STA is associated as a QSTA or QAP (for TX/RX) */
    u8 fgIsWmmSupported;  /* If the peer supports WMM, set to true (for
                           * association) */
    u8 fgIsUapsdSupported;  /* Set according to the scan result (for
                             * association) */

    /*------------------------------------------------------------------------------------------*/
    /* P2P related fields */
    /*------------------------------------------------------------------------------------------*/
#if CFG_ENABLE_WIFI_DIRECT
    u8 u2DevNameLen;
    u8 aucDevName[WPS_ATTRI_MAX_LEN_DEVICE_NAME];

    u8 aucDevAddr[MAC_ADDR_LEN];  /* P2P Device Address */

    u16 u2ConfigMethods;

    u8 ucDeviceCap;

    u8 ucSecondaryDevTypeCount;

    DEVICE_TYPE_T rPrimaryDevTypeBE;

    DEVICE_TYPE_T arSecondaryDevTypeBE[P2P_GC_MAX_CACHED_SEC_DEV_TYPE_COUNT];
#endif

    /*------------------------------------------------------------------------------------------*/
    /* QM related fields */
    /*------------------------------------------------------------------------------------------*/

    u8 ucFreeQuota;  /* Per Sta flow controal. Valid when fgIsInPS is true.
                      * Change it for per Queue flow control
                      */
    /* u8                  aucFreeQuotaPerQueue[NUM_OF_PER_STA_TX_QUEUES];
     */ /* used in future */
    u8 ucFreeQuotaForDelivery;
    u8 ucFreeQuotaForNonDelivery;

    /*------------------------------------------------------------------------------------------*/
    /* TXM related fields */
    /*------------------------------------------------------------------------------------------*/
    void *aprTxDescTemplate[TX_DESC_TID_NUM];

#if CFG_ENABLE_PKT_LIFETIME_PROFILE && CFG_ENABLE_PER_STA_STATISTICS
    u32 u4TotalTxPktsNumber;
    u32 u4TotalTxPktsTime;
    u32 u4TotalRxPktsNumber;
    u32 u4MaxTxPktsTime;
    u32 u4ThresholdCounter;
#endif

    /*------------------------------------------------------------------------------------------*/
    /* To be removed, this is to make que_mgt compilation success only */
    /*------------------------------------------------------------------------------------------*/
    /* When this STA_REC is in use, set to true. */
    u8 fgIsValid;

    /* TX key is ready */
    u8 fgIsTxKeyReady;

    /* When the STA is connected or TX key is ready */
    u8 fgIsTxAllowed;

    /* Per-STA Queues: [0] AC0, [1] AC1, [2] AC2, [3] AC3 */
    QUE_T arTxQueue[NUM_OF_PER_STA_TX_QUEUES];

    /* Per-STA Pending Queues: [0] AC0, [1] AC1, [2] AC2, [3] AC3 */
    /* This queue is for Tx packet in protected BSS before key is set */
    QUE_T arPendingTxQueue[NUM_OF_PER_STA_TX_QUEUES];

    /* Tx packet target queue pointer. Select between arTxQueue &
     * arPendingTxQueue */
    P_QUE_T aprTargetQueue[NUM_OF_PER_STA_TX_QUEUES];

    /* Reorder Parameter reference table */
    P_RX_BA_ENTRY_T aprRxReorderParamRefTbl[CFG_RX_MAX_BA_TID_NUM];

#if CFG_SUPPORT_802_11V_TIMING_MEASUREMENT
    TIMINGMSMT_PARAM_T rWNMTimingMsmt;
#endif
    u8 ucTrafficDataType;  /* 0: auto 1: data 2: video 3: voice */
    u8 ucTxGfMode;  /* 0: auto 1:Force enable 2: Force disable 3: enable by
                     * peer */
    u8 ucTxSgiMode;  /* 0: auto 1:Force enable 2: Force disable 3: enable by
                      * peer */
    u8 ucTxStbcMode;  /* 0: auto 1:Force enable 2: Force disable 3: enable by
                       * peer */
    u32 u4FixedPhyRate;  /**/
    u16 u2MaxLinkSpeed;  /* unit is 0.5 Mbps */
    u16 u2MinLinkSpeed;
    u32 u4Flags;  /* reserved for MTK Synergies */

#if CFG_SUPPORT_TDLS
    u8 fgTdlsIsProhibited;  /* true: AP prohibits TDLS links */
    u8 fgTdlsIsChSwProhibited;  /* true: AP prohibits TDLS chan switch */

    u8 flgTdlsIsInitiator;  /* true: the peer is the initiator */
    IE_HT_CAP_T rTdlsHtCap;  /* temp to queue HT capability element */
    PARAM_KEY_T rTdlsKeyTemp;  /* temp to queue the key information */
    u8 ucTdlsIndex;
#endif
#if CFG_SUPPORT_TX_BF
    TXBF_PFMU_STA_INFO rTxBfPfmuStaInfo;
#endif
#if CFG_SUPPORT_MSP
    u32 u4RxVector0;
    u32 u4RxVector1;
    u32 u4RxVector2;
    u32 u4RxVector3;
    u32 u4RxVector4;
#endif
#if CFG_SUPPORT_LAST_SEC_MCS_INFO
    u32 au4RxVect0Que[MCS_INFO_SAMPLE_CNT];
    u32 au4RxVect1Que[MCS_INFO_SAMPLE_CNT];
#endif
    u8 ucSmDialogToken;  /* Spectrum Mngt Dialog Token */
    u8 ucSmMsmtRequestMode;  /* Measurement Request Mode */
    u8 ucSmMsmtToken;  /* Measurement Request Token */
#if CFG_SUPPORT_802_11W
    /* AP PMF */
    struct STA_PMF_CFG rPmfCfg;
#endif
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
    u8 fgSupportBTM;  /* Indicates whether to support BTM */
#endif
};

typedef enum _ENUM_STA_REC_CMD_ACTION_T {
    STA_REC_CMD_ACTION_STA             = 0,
    STA_REC_CMD_ACTION_BSS             = 1,
    STA_REC_CMD_ACTION_BSS_EXCLUDE_STA = 2
} ENUM_STA_REC_CMD_ACTION_T,
*P_ENUM_STA_REC_CMD_ACTION_T;

#if CFG_SUPPORT_TDLS

/* TDLS FSM */
typedef struct _CMD_PEER_ADD_T {
    u8 aucPeerMac[6];
    ENUM_STA_TYPE_T eStaType;
} CMD_PEER_ADD_T;

typedef struct _CMD_PEER_UPDATE_HT_CAP_MCS_INFO_T {
    u8 arRxMask[SUP_MCS_RX_BITMASK_OCTET_NUM];
    u16 u2RxHighest;
    u8 ucTxParams;
    u8 Reserved[3];
} CMD_PEER_UPDATE_HT_CAP_MCS_INFO_T;

typedef struct _CMD_PEER_UPDATE_VHT_CAP_MCS_INFO_T {
    u16 u2RxMcsMap;
    u16 u2RxHighest;
    u16 u2TxMcsMap;
    u16 u2TxHighest;
} CMD_PEER_UPDATE_VHT_CAP_MCS_INFO_T;

typedef struct _CMD_PEER_UPDATE_HT_CAP_T {
    u16 u2CapInfo;
    u8 ucAmpduParamsInfo;

    /* 16 bytes MCS information */
    CMD_PEER_UPDATE_HT_CAP_MCS_INFO_T rMCS;

    u16 u2ExtHtCapInfo;
    u32 u4TxBfCapInfo;
    u8 ucAntennaSelInfo;
} CMD_PEER_UPDATE_HT_CAP_T;

typedef struct _CMD_PEER_UPDATE_VHT_CAP_T {
    u32 u4CapInfo;
    /* 16 bytes MCS information */
    CMD_PEER_UPDATE_VHT_CAP_MCS_INFO_T rVMCS;
} CMD_PEER_UPDATE_VHT_CAP_T;

typedef struct _CMD_PEER_UPDATE_T {
    u8 aucPeerMac[6];

#define CMD_PEER_UPDATE_SUP_CHAN_MAX      50
    u8 aucSupChan[CMD_PEER_UPDATE_SUP_CHAN_MAX];

    u16 u2StatusCode;

#define CMD_PEER_UPDATE_SUP_RATE_MAX      50
    u8 aucSupRate[CMD_PEER_UPDATE_SUP_RATE_MAX];
    u16 u2SupRateLen;

    u8 UapsdBitmap;
    u8 UapsdMaxSp;  /* MAX_SP */

    u16 u2Capability;
#define CMD_PEER_UPDATE_EXT_CAP_MAXLEN    5
    u8 aucExtCap[CMD_PEER_UPDATE_EXT_CAP_MAXLEN];
    u16 u2ExtCapLen;

    CMD_PEER_UPDATE_HT_CAP_T rHtCap;
    CMD_PEER_UPDATE_VHT_CAP_T rVHtCap;

    u8 fgIsSupHt;
    u8 fgIsSupVht;
    ENUM_STA_TYPE_T eStaType;
} CMD_PEER_UPDATE_T;

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

#if CFG_DBG_MGT_BUF
#define cnmMgtPktAlloc(_prAdapter, _u4Length) \
    cnmPktAllocWrapper((_prAdapter), (_u4Length), (u8 *)__func__)

#define cnmMgtPktFree(_prAdapter, _prMsduInfo) \
    cnmPktFreeWrapper((_prAdapter), (_prMsduInfo), (u8 *)__func__)
#else
#define cnmMgtPktAlloc    cnmPktAlloc
#define cnmMgtPktFree     cnmPktFree
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

P_MSDU_INFO_T cnmPktAllocWrapper(IN P_ADAPTER_T prAdapter,
                                 IN u32 u4Length,
                                 IN u8 *pucStr);

void cnmPktFreeWrapper(IN P_ADAPTER_T prAdapter,
                       IN P_MSDU_INFO_T prMsduInfo,
                       IN u8 *pucStr);

P_MSDU_INFO_T cnmPktAlloc(IN P_ADAPTER_T prAdapter, IN u32 u4Length);

void cnmPktFree(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo);

void cnmMemInit(IN P_ADAPTER_T prAdapter);

void *cnmMemAlloc(IN P_ADAPTER_T prAdapter,
                  IN ENUM_RAM_TYPE_T eRamType,
                  IN u32 u4Length);

void cnmMemFree(IN P_ADAPTER_T prAdapter, IN void *pvMemory);

void cnmStaRecInit(IN P_ADAPTER_T prAdapter);

P_STA_RECORD_T
cnmStaRecAlloc(IN P_ADAPTER_T prAdapter,
               IN ENUM_STA_TYPE_T eStaType,
               IN u8 ucBssIndex,
               IN u8 *pucMacAddr);

void cnmStaRecFree(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

void cnmStaFreeAllStaByNetwork(P_ADAPTER_T prAdapter,
                               u8 ucBssIndex,
                               u8 ucStaRecIndexExcluded);

P_STA_RECORD_T cnmGetStaRecByIndex(IN P_ADAPTER_T prAdapter, IN u8 ucIndex);

P_STA_RECORD_T cnmGetStaRecByAddress(P_ADAPTER_T prAdapter,
                                     u8 ucBssIndex,
                                     u8 aucPeerMACAddress[]);

P_STA_RECORD_T cnmGetAnyStaRecByAddress(P_ADAPTER_T prAdapter,
                                        u8 *pucPeerMacAddr);

void cnmStaRecChangeState(IN P_ADAPTER_T prAdapter,
                          IN OUT P_STA_RECORD_T prStaRec,
                          IN u8 ucNewState);

void cnmDumpStaRec(IN P_ADAPTER_T prAdapter, IN u8 ucStaRecIdx);

u32 cnmDumpMemoryStatus(IN P_ADAPTER_T prAdapter, IN u8 *pucBuf, IN u32 u4Max);

#if CFG_SUPPORT_TDLS
WLAN_STATUS  /* TDLS_STATUS */
cnmPeerAdd(P_ADAPTER_T prAdapter,
           void *pvSetBuffer,
           u32 u4SetBufferLen,
           u32 *pu4SetInfoLen);

WLAN_STATUS  /* TDLS_STATUS */
cnmPeerUpdate(P_ADAPTER_T prAdapter,
              void *pvSetBuffer,
              u32 u4SetBufferLen,
              u32 *pu4SetInfoLen);

P_STA_RECORD_T cnmGetTdlsPeerByAddress(P_ADAPTER_T prAdapter,
                                       u8 ucBssIndex,
                                       u8 aucPeerMACAddress[]);
#endif

void cnmStaSendUpdateCmd(P_ADAPTER_T prAdapter,
                         P_STA_RECORD_T prStaRec,
                         P_TXBF_PFMU_STA_INFO prTxBfPfmuStaInfo,
                         u8 fgNeedResp);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
