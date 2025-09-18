/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "que_mgt.h"
 *  \brief  TX/RX queues management header file
 *
 *  The main tasks of queue management include TC-based HIF TX flow control,
 *  adaptive TC quota adjustment, HIF TX grant scheduling, Power-Save
 *  forwarding control, RX packet reordering, and RX BA agreement management.
 */

#ifndef _QUE_MGT_H
#define _QUE_MGT_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

extern u8 g_arTdlsLink[MAXNUM_TDLS_PEER];
extern const u8 *apucACI2Str[4];
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/* Queue Manager Features */
#define QM_BURST_END_INFO_ENABLED \
    0  /* 1: Indicate the last TX packet to the FW for each burst */
#define QM_FORWARDING_FAIRNESS \
    1  /* 1: To fairly share TX resource among active STAs */

#define QM_ADAPTIVE_TC_RESOURCE_CTRL \
    1  /* 1: To adaptively adjust resource for each TC */
#define QM_FAST_TC_RESOURCE_CTRL \
    1  /* 1: To fast adjust resource for EMPTY TC (assigned resource is 0) \
        */
#define QM_PRINT_TC_RESOURCE_CTRL \
    0  /* 1: To print TC resource adjustment results */
/* 1: If pkt with SSN is missing, auto advance the RX reordering window */
#define QM_RX_WIN_SSN_AUTO_ADVANCING        1
/* 1: Indicate the packets falling behind to OS before the frame with SSN is
 * received */
#define QM_RX_INIT_FALL_BEHIND_PASS     1
#define QM_TC_RESOURCE_EMPTY_COUNTER \
    1  /* 1: Count times of TC resource empty happened */

/* Parameters */
#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
/* p: Update queue lengths when p TX packets are enqueued */
#define QM_INIT_TIME_TO_UPDATE_QUE_LEN      128
#else
#define QM_INIT_TIME_TO_UPDATE_QUE_LEN \
    256  /* p: Update queue lengths when p TX packets are enqueued */
#endif
#define QM_INIT_TIME_TO_ADJUST_TC_RSC \
    2  /* s: Adjust the TC resource every s updates of queue lengths  */
#define QM_QUE_LEN_MOVING_AVE_FACTOR        3  /* Factor for Que Len averaging */

#define QM_MIN_RESERVED_TC0_RESOURCE        0
#define QM_MIN_RESERVED_TC1_RESOURCE        1
#define QM_MIN_RESERVED_TC2_RESOURCE        0
#define QM_MIN_RESERVED_TC3_RESOURCE        1
#define QM_MIN_RESERVED_TC4_RESOURCE \
    2  /* Resource for TC4 is not adjustable \
        */
#define QM_MIN_RESERVED_TC5_RESOURCE        0

#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
#define QM_MIN_RESERVED_TC6_RESOURCE        0
#define QM_MIN_RESERVED_TC7_RESOURCE        0
#define QM_MIN_RESERVED_TC8_RESOURCE        0
#define QM_MIN_RESERVED_TC9_RESOURCE        0
#define QM_MIN_RESERVED_TC10_RESOURCE       0
#define QM_MIN_RESERVED_TC11_RESOURCE       0
#define QM_MIN_RESERVED_TC12_RESOURCE       0
#define QM_MIN_RESERVED_TC13_RESOURCE       0
#define QM_MIN_RESERVED_TC14_RESOURCE       0
#define QM_MIN_RESERVED_TC15_RESOURCE       0
#define QM_MIN_RESERVED_TC16_RESOURCE       0
#endif

#define QM_GUARANTEED_TC0_RESOURCE      4
#define QM_GUARANTEED_TC1_RESOURCE      4
#define QM_GUARANTEED_TC2_RESOURCE      9
#define QM_GUARANTEED_TC3_RESOURCE      11
#define QM_GUARANTEED_TC4_RESOURCE      2  /* Resource for TC4 is not adjustable */
#define QM_GUARANTEED_TC5_RESOURCE      4

#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
#define QM_GUARANTEED_TC6_RESOURCE      4
#define QM_GUARANTEED_TC7_RESOURCE      4
#define QM_GUARANTEED_TC8_RESOURCE      9
#define QM_GUARANTEED_TC9_RESOURCE      11
#define QM_GUARANTEED_TC10_RESOURCE     0
#define QM_GUARANTEED_TC11_RESOURCE     4
#define QM_GUARANTEED_TC12_RESOURCE     4
#define QM_GUARANTEED_TC13_RESOURCE     9
#define QM_GUARANTEED_TC14_RESOURCE     11
#define QM_GUARANTEED_TC15_RESOURCE     0
#define QM_GUARANTEED_TC16_RESOURCE     11
#endif

#define QM_EXTRA_RESERVED_RESOURCE_WHEN_BUSY    0

#define QM_AVERAGE_TC_RESOURCE          6

#define QM_ACTIVE_TC_NUM            TC_NUM

#define QM_MGMT_QUEUED_THRESHOLD        6
#define QM_CMD_RESERVED_THRESHOLD       4
#define QM_MGMT_QUEUED_TIMEOUT          1000  /* ms */

#define QM_TEST_MODE                0
#define QM_TEST_TRIGGER_TX_COUNT        50
#define QM_TEST_STA_REC_DETERMINATION       0
#define QM_TEST_STA_REC_DEACTIVATION        0
#define QM_TEST_FAIR_FORWARDING         0

#define QM_DEBUG_COUNTER            0

/* Per-STA Queues: [0] AC0, [1] AC1, [2] AC2, [3] AC3 */
/* Per-Type Queues: [0] BMCAST */
#define NUM_OF_PER_STA_TX_QUEUES        4
#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
/*Each wmm set has its own BCM_IDX tx queue*/
#define NUM_OF_PER_TYPE_TX_QUEUES       HIF_WMM_SET_NUM
#else
#define NUM_OF_PER_TYPE_TX_QUEUES       1
#endif

/* TX Queue Index */
/* Per-Type */
#define TX_QUEUE_INDEX_BMCAST           0
#define TX_QUEUE_INDEX_NO_STA_REC       0

/* Per-STA */
#define TX_QUEUE_INDEX_AC0          0
#define TX_QUEUE_INDEX_AC1          1
#define TX_QUEUE_INDEX_AC2          2
#define TX_QUEUE_INDEX_AC3          3
#define TX_QUEUE_INDEX_NON_QOS          TX_QUEUE_INDEX_AC1

#define QM_DEFAULT_USER_PRIORITY        0

#define QM_STA_FORWARD_COUNT_UNLIMITED      0xFFFFFFFF
#define QM_FWD_PKT_QUE_THRESHOLD        128

/* 1 WMM-related */
/* WMM FLAGS */
#define WMM_FLAG_SUPPORT_WMM            BIT(0)
#define WMM_FLAG_SUPPORT_WMMSA          BIT(1)
#define WMM_FLAG_AC_PARAM_PRESENT       BIT(2)
#define WMM_FLAG_SUPPORT_UAPSD          BIT(3)

/* WMM Admission Control Mandatory FLAGS */
#define ACM_FLAG_ADM_NOT_REQUIRED       0
#define ACM_FLAG_ADM_GRANTED            BIT(0)
#define ACM_FLAG_ADM_REQUIRED           BIT(1)

/* WMM Power Saving FLAGS */
#define AC_FLAG_TRIGGER_ENABLED         BIT(1)
#define AC_FLAG_DELIVERY_ENABLED        BIT(2)

/* WMM-2.2.1 WMM Information Element */
#define ELEM_MAX_LEN_WMM_INFO           7

/* WMM-2.2.2 WMM Parameter Element */
#define ELEM_MAX_LEN_WMM_PARAM          24

/* WMM-2.2.1 WMM QoS Info field */
#define WMM_QOS_INFO_PARAM_SET_CNT      BITS(0, 3)  /* Sent by AP */
#define WMM_QOS_INFO_UAPSD          BIT(7)

#define WMM_QOS_INFO_VO_UAPSD           BIT(0)  /* Sent by non-AP STA */
#define WMM_QOS_INFO_VI_UAPSD           BIT(1)
#define WMM_QOS_INFO_BK_UAPSD           BIT(2)
#define WMM_QOS_INFO_BE_UAPSD           BIT(3)
#define WMM_QOS_INFO_MAX_SP_LEN_MASK        BITS(5, 6)
#define WMM_QOS_INFO_MAX_SP_ALL         0
#define WMM_QOS_INFO_MAX_SP_2           BIT(5)
#define WMM_QOS_INFO_MAX_SP_4           BIT(6)
#define WMM_QOS_INFO_MAX_SP_6           BITS(5, 6)

/* -- definitions for Max SP length field */
#define WMM_MAX_SP_LENGTH_ALL           0
#define WMM_MAX_SP_LENGTH_2         2
#define WMM_MAX_SP_LENGTH_4         4
#define WMM_MAX_SP_LENGTH_6         6

/* WMM-2.2.2 WMM ACI/AIFSN field */
/* -- subfields in the ACI/AIFSN field */
#define WMM_ACIAIFSN_AIFSN          BITS(0, 3)
#define WMM_ACIAIFSN_ACM            BIT(4)
#define WMM_ACIAIFSN_ACI            BITS(5, 6)
#define WMM_ACIAIFSN_ACI_OFFSET         5

/* -- definitions for ACI field */
#define WMM_ACI_AC_BE               0
#define WMM_ACI_AC_BK               BIT(5)
#define WMM_ACI_AC_VI               BIT(6)
#define WMM_ACI_AC_VO               BITS(5, 6)

#define WMM_ACI(_AC)    (_AC << WMM_ACIAIFSN_ACI_OFFSET)

/* -- definitions for ECWmin/ECWmax field */
#define WMM_ECW_WMIN_MASK           BITS(0, 3)
#define WMM_ECW_WMAX_MASK           BITS(4, 7)
#define WMM_ECW_WMAX_OFFSET         4

#define TXM_DEFAULT_FLUSH_QUEUE_GUARD_TIME  0  /* Unit: 64 us */

#ifdef CFG_RX_BA_ENTRY_MISS_TIMEOUT
#define QM_RX_BA_ENTRY_MISS_TIMEOUT_MS      (CFG_RX_BA_ENTRY_MISS_TIMEOUT)
#else
#define QM_RX_BA_ENTRY_MISS_TIMEOUT_MS      (200)
#endif

#define QM_DEQUE_PERCENT_VHT80_NSS1     75  /* BW80 NSS1 rate: MCS9 433 Mbps */
#define QM_DEQUE_PERCENT_VHT40_NSS1     35  /* BW40 NSS1 Max rate: 200 Mbps */
#define QM_DEQUE_PERCENT_VHT20_NSS1     15  /* BW20 NSS1 Max rate: 86.7Mbps */

#define QM_DEQUE_PERCENT_HT40_NSS1 \
    25  /* BW40 NSS1 Max rate: 150 Mbps (MCS9 200Mbps)*/
#define QM_DEQUE_PERCENT_HT20_NSS1 \
    12  /* BW20 NSS1 Max rate: 72.2Mbps (MCS8 86.7Mbps)*/

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

enum {
    QM_DBG_CNT_00 = 0,
    QM_DBG_CNT_01,
    QM_DBG_CNT_02,
    QM_DBG_CNT_03,
    QM_DBG_CNT_04,
    QM_DBG_CNT_05,
    QM_DBG_CNT_06,
    QM_DBG_CNT_07,
    QM_DBG_CNT_08,
    QM_DBG_CNT_09,
    QM_DBG_CNT_10,
    QM_DBG_CNT_11,
    QM_DBG_CNT_12,
    QM_DBG_CNT_13,
    QM_DBG_CNT_14,
    QM_DBG_CNT_15,
    QM_DBG_CNT_16,
    QM_DBG_CNT_17,
    QM_DBG_CNT_18,
    QM_DBG_CNT_19,
    QM_DBG_CNT_20,
    QM_DBG_CNT_21,
    QM_DBG_CNT_22,
    QM_DBG_CNT_23,
    QM_DBG_CNT_24,
    QM_DBG_CNT_25,
    QM_DBG_CNT_26,
    QM_DBG_CNT_27,
    QM_DBG_CNT_28,
    QM_DBG_CNT_29,
    QM_DBG_CNT_30,
    QM_DBG_CNT_31,
    QM_DBG_CNT_NUM
};

/* Used for MAC TX */
typedef enum _ENUM_MAC_TX_QUEUE_INDEX_T {
    MAC_TX_QUEUE_AC0_INDEX = 0,
    MAC_TX_QUEUE_AC1_INDEX,
    MAC_TX_QUEUE_AC2_INDEX,
    MAC_TX_QUEUE_AC3_INDEX,
    MAC_TX_QUEUE_AC4_INDEX,
    MAC_TX_QUEUE_AC5_INDEX,
    MAC_TX_QUEUE_AC6_INDEX,
    MAC_TX_QUEUE_BCN_INDEX,
    MAC_TX_QUEUE_BMC_INDEX,
    MAC_TX_QUEUE_NUM
} ENUM_MAC_TX_QUEUE_INDEX_T;

typedef struct _RX_BA_ENTRY_T {
    u8 fgIsValid;
    QUE_T rReOrderQue;
    u16 u2WinStart;
    u16 u2WinEnd;
    u16 u2WinSize;

    /* For identifying the RX BA agreement */
    u8 ucStaRecIdx;
    u8 ucTid;

    u8 fgIsWaitingForPktWithSsn;

    TIMER_T rReorderBubbleTimer;
    u16 u2FirstBubbleSn;
    u8 fgHasBubble;

    /* u8                  ucTxBufferSize; */
    /* u8                    fgIsAcConstrain; */
    /* u8                    fgIsBaEnabled; */
#if CFG_SUPPORT_RX_AMSDU
    /* RX reorder for one MSDU in AMSDU issue */
    /* P_SW_RFB_T prMpduSwRfb; */
    u32 u4SeqNo;  /* for statistic */
    u8 fgAmsduNeedLastFrame;  /* for statistic */
    u8 u8LastAmsduSubIdx;
    u8 fgIsAmsduDuplicated;
#endif
    u8 fgFirstSnToWinStart;
} RX_BA_ENTRY_T, *P_RX_BA_ENTRY_T;

typedef u32 (*PFN_DEQUEUE_FUNCTION)(IN P_ADAPTER_T prAdapter,
                                    OUT P_QUE_T prQue,
                                    IN u8 ucTC,
                                    IN u32 u4CurrentQuota,
                                    IN u32 u4TotalQuota);

/* The mailbox message (could be used for Host-To-Device or Device-To-Host
 * Mailbox) */
typedef struct _MAILBOX_MSG_T {
    u32 u4Msg[2];  /* [0]: D2HRM0R or H2DRM0R, [1]: D2HRM1R or H2DRM1R */
} MAILBOX_MSG_T, *P_MAILBOX_MSG_T;

/* Used for adaptively adjusting TC resources */
typedef struct _TC_RESOURCE_CTRL_T {
    /* TC0, TC1, TC2, TC3, TC5 */
    u32 au4AverageQueLen[TC_NUM - 1];
} TC_RESOURCE_CTRL_T, *P_TC_RESOURCE_CTRL_T;

typedef struct _QUE_MGT_T {  /* Queue Management Control Info */
    /* Per-Type Queues: [0] BMCAST or UNKNOWN-STA packets */
    QUE_T arTxQueue[NUM_OF_PER_TYPE_TX_QUEUES];

    /* Reordering Queue Parameters */
    RX_BA_ENTRY_T arRxBaTable[CFG_NUM_OF_RX_BA_AGREEMENTS];

    /* Current number of activated RX BA agreements <=
     * CFG_NUM_OF_RX_BA_AGREEMENTS */
    u8 ucRxBaCount;

#if QM_TEST_MODE
    u32 u4PktCount;
    P_ADAPTER_T prAdapter;

#if QM_TEST_FAIR_FORWARDING
    u32 u4CurrentStaRecIndexToEnqueue;
#endif
#endif

#if QM_FORWARDING_FAIRNESS
    /* The current resource used count for a STA with respect to a TC index
     */
#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
    u32 au4ResourceUsedCount[TC_NUM];
#else
    u32 au4ResourceUsedCount[NUM_OF_PER_STA_TX_QUEUES];
#endif

    /* The current serving STA with respect to a TC index */
#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
    u8 au4HeadStaRecIndex[TC_NUM];
#else
    u32 au4HeadStaRecIndex[NUM_OF_PER_STA_TX_QUEUES];
#endif

    /* For TC5 only */
    u8 fgGlobalQFirst;
    u32 u4GlobalResourceUsedCount;
#endif

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
    u32 au4AverageQueLen[TC_NUM];
    u32 au4CurrentTcResource[TC_NUM];
    u32 au4MinReservedTcResource[TC_NUM];  /* The minimum amount of resource
                                            * no matter busy or idle */
    u32 au4GuaranteedTcResource[TC_NUM];  /* The minimum amount of resource
                                           * when extremely busy */

    u32 u4TimeToAdjustTcResource;
    u32 u4TimeToUpdateQueLen;

    u32 u4QueLenMovingAverage;
    u32 u4ExtraReservedTcResource;
    u32 u4ResidualTcResource;

    /* Set to true if the last TC adjustment has not been completely applied
     * (i.e., waiting more TX-Done events */
    /* to align the TC quotas to the TC resource assignment) */
    u8 fgTcResourcePostAnnealing;

#if QM_FAST_TC_RESOURCE_CTRL
    u8 fgTcResourceFastReaction;
#endif
#endif

#if QM_DEBUG_COUNTER
    u32 au4QmDebugCounters[QM_DBG_CNT_NUM];
#endif

    u32 u4TxAllowedStaCount;

#if QM_TC_RESOURCE_EMPTY_COUNTER
    u32 au4QmTcResourceEmptyCounter[MAX_BSS_INDEX][TC_NUM];
#endif

    u32 u4MaxForwardBufferCount;

    u32 rLastTxPktDumpTime;
    u8 fgIsTxResrouceControlEn;
} QUE_MGT_T, *P_QUE_MGT_T;

typedef struct _EVENT_RX_ADDBA_T {
    /* Fields not present in the received ADDBA_REQ */
    u8 ucStaRecIdx;

    /* Fields that are present in the received ADDBA_REQ */
    u8 ucDialogToken;  /* Dialog Token chosen by the sender */
    u16 u2BAParameterSet;  /* BA policy, TID, buffer size */
    u16 u2BATimeoutValue;
    u16 u2BAStartSeqCtrl;  /* SSN */
} EVENT_RX_ADDBA_T, *P_EVENT_RX_ADDBA_T;

typedef struct _EVENT_RX_DELBA_T {
    /* Fields not present in the received ADDBA_REQ */
    u8 ucStaRecIdx;
    u8 ucTid;
    u8 aucReserved[2];
} EVENT_RX_DELBA_T, *P_EVENT_RX_DELBA_T;

typedef struct _EVENT_BSS_ABSENCE_PRESENCE_T {
    /* Event Body */
    u8 ucBssIndex;
    u8 ucIsAbsent;
    u8 ucBssFreeQuota;
    u8 aucReserved[1];
} EVENT_BSS_ABSENCE_PRESENCE_T, *P_EVENT_BSS_ABSENCE_PRESENCE_T;

typedef struct _EVENT_STA_CHANGE_PS_MODE_T {
    /* Event Body */
    u8 ucStaRecIdx;
    u8 ucIsInPs;
    u8 ucUpdateMode;
    u8 ucFreeQuota;
} EVENT_STA_CHANGE_PS_MODE_T, *P_EVENT_STA_CHANGE_PS_MODE_T;

/* The free quota is used by PS only now */
/* The event may be used by per STA flow conttrol in general */
typedef struct _EVENT_STA_UPDATE_FREE_QUOTA_T {
    /* Event Body */
    u8 ucStaRecIdx;
    u8 ucUpdateMode;
    u8 ucFreeQuota;
    u8 aucReserved[1];
} EVENT_STA_UPDATE_FREE_QUOTA_T, *P_EVENT_STA_UPDATE_FREE_QUOTA_T;

typedef struct _EVENT_CHECK_REORDER_BUBBLE_T {
    /* Event Body */
    u8 ucStaRecIdx;
    u8 ucTid;
} EVENT_CHECK_REORDER_BUBBLE_T, *P_EVENT_CHECK_REORDER_BUBBLE_T;

/* WMM-2.2.1 WMM Information Element */
typedef struct _IE_WMM_INFO_T {
    u8 ucId;  /* Element ID */
    u8 ucLength;  /* Length */
    u8 aucOui[3];  /* OUI */
    u8 ucOuiType;  /* OUI Type */
    u8 ucOuiSubtype;  /* OUI Subtype */
    u8 ucVersion;  /* Version */
    u8 ucQosInfo;  /* QoS Info field */
    u8 ucDummy[3];  /* Dummy for pack */
} IE_WMM_INFO_T, *P_IE_WMM_INFO_T;

typedef struct _WMM_AC_PARAM_T {
    u8 ucAciAifsn;
    u8 ucEcw;
    u16 u2TxopLimit;
} WMM_AC_PARAM_T, *P_WMM_AC_PARAM_T;

/* WMM-2.2.2 WMM Parameter Element */
typedef struct _IE_WMM_PARAM_T {
    u8 ucId;  /* Element ID */
    u8 ucLength;  /* Length */

    /* IE Body */
    u8 aucOui[3];  /* OUI */
    u8 ucOuiType;  /* OUI Type */
    u8 ucOuiSubtype;  /* OUI Subtype */
    u8 ucVersion;  /* Version */

    /* WMM IE Body */
    u8 ucQosInfo;  /* QoS Info field */
    u8 ucReserved;

    /* AC Parameters */
    WMM_AC_PARAM_T arAcParam[4];
} IE_WMM_PARAM_T, *P_IE_WMM_PARAM_T;

typedef struct _IE_WMM_TSPEC_T {
    u8 ucId;  /* Element ID */
    u8 ucLength;  /* Length */
    u8 aucOui[3];  /* OUI */
    u8 ucOuiType;  /* OUI Type */
    u8 ucOuiSubtype;  /* OUI Subtype */
    u8 ucVersion;  /* Version */
    /* WMM TSPEC body */
    u8 aucTsInfo[3];  /* TS Info */
    u8 aucTspecBodyPart[1];  /* Note: Utilize PARAM_QOS_TSPEC to fill (memory
                              * copy) */
} IE_WMM_TSPEC_T, *P_IE_WMM_TSPEC_T;

typedef struct _IE_WMM_HDR_T {
    u8 ucId;  /* Element ID */
    u8 ucLength;  /* Length */
    u8 aucOui[3];  /* OUI */
    u8 ucOuiType;  /* OUI Type */
    u8 ucOuiSubtype;  /* OUI Subtype */
    u8 ucVersion;  /* Version */
    u8 aucBody[1];  /* IE body */
} IE_WMM_HDR_T, *P_IE_WMM_HDR_T;

typedef struct _AC_QUE_PARMS_T {
    u16 u2CWmin;  /*!< CWmin */
    u16 u2CWmax;  /*!< CWmax */
    u16 u2TxopLimit;  /*!< TXOP limit */
    u16 u2Aifsn;  /*!< AIFSN */
    u8 ucGuradTime;  /*!< GuardTime for STOP/FLUSH. */
    u8 ucIsACMSet;
} AC_QUE_PARMS_T, *P_AC_QUE_PARMS_T;

/* WMM ACI (AC index) */
typedef enum _ENUM_WMM_ACI_T {
    WMM_AC_BE_INDEX = 0,
    WMM_AC_BK_INDEX,
    WMM_AC_VI_INDEX,
    WMM_AC_VO_INDEX,
    WMM_AC_INDEX_NUM
} ENUM_WMM_ACI_T,
*P_ENUM_WMM_ACI_T;

/* Used for CMD Queue Operation */
typedef enum _ENUM_FRAME_ACTION_T {
    FRAME_ACTION_DROP_PKT = 0,
    FRAME_ACTION_QUEUE_PKT,
    FRAME_ACTION_TX_PKT,
    FRAME_ACTION_NUM
} ENUM_FRAME_ACTION_T;

typedef enum _ENUM_FRAME_TYPE_IN_CMD_Q_T {
    FRAME_TYPE_802_1X = 0,
    FRAME_TYPE_MMPDU,
    FRAME_TYPE_NUM
} ENUM_FRAME_TYPE_IN_CMD_Q_T;

typedef enum _ENUM_FREE_QUOTA_MODET_T {
    FREE_QUOTA_UPDATE_MODE_INIT = 0,
    FREE_QUOTA_UPDATE_MODE_OVERWRITE,
    FREE_QUOTA_UPDATE_MODE_INCREASE,
    FREE_QUOTA_UPDATE_MODE_DECREASE
} ENUM_FREE_QUOTA_MODET_T,
*P_ENUM_FREE_QUOTA_MODET_T;

typedef struct _CMD_UPDATE_WMM_PARMS_T {
    AC_QUE_PARMS_T arACQueParms[AC_NUM];
    u8 ucBssIndex;
    u8 fgIsQBSS;
    u8 ucWmmSet;
    u8 aucReserved;
} CMD_UPDATE_WMM_PARMS_T, *P_CMD_UPDATE_WMM_PARMS_T;

typedef struct _CMD_TX_AMPDU_T {
    u8 fgEnable;
    u8 aucReserved[3];
} CMD_TX_AMPDU_T, *P_CMD_TX_AMPDU_T;

typedef struct _CMD_ADDBA_REJECT {
    u8 fgEnable;
    u8 aucReserved[3];
} CMD_ADDBA_REJECT_T, *P_CMD_ADDBA_REJECT_T;

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

#define QM_TX_SET_NEXT_MSDU_INFO(_prMsduInfoPreceding, _prMsduInfoNext) \
    ((((_prMsduInfoPreceding)->rQueEntry).prNext) =                     \
         (P_QUE_ENTRY_T)(_prMsduInfoNext))

#define QM_TX_SET_NEXT_SW_RFB(_prSwRfbPreceding, _prSwRfbNext) \
    ((((_prSwRfbPreceding)->rQueEntry).prNext) =               \
         (P_QUE_ENTRY_T)(_prSwRfbNext))

#define QM_TX_GET_NEXT_MSDU_INFO(_prMsduInfo) \
    ((P_MSDU_INFO_T)(((_prMsduInfo)->rQueEntry).prNext))

#define QM_RX_SET_NEXT_SW_RFB(_prSwRfbPreceding, _prSwRfbNext) \
    ((((_prSwRfbPreceding)->rQueEntry).prNext) =               \
         (P_QUE_ENTRY_T)(_prSwRfbNext))

#define QM_RX_GET_NEXT_SW_RFB(_prSwRfb) \
    ((P_SW_RFB_T)(((_prSwRfb)->rQueEntry).prNext))

#define QM_GET_STA_REC_PTR_FROM_INDEX(_prAdapter, _ucIndex) \
    cnmGetStaRecByIndex(_prAdapter, _ucIndex)

#define QM_TX_SET_MSDU_INFO_FOR_DATA_PACKET(_prMsduInfo, _ucTC, _ucPacketType, \
                                            _ucFormatID, _fgIs802_1x,          \
                                            _fgIs802_11, _ucPsForwardingType)  \
    {                                                                          \
        ASSERT(_prMsduInfo);                                                   \
        (_prMsduInfo)->ucTC = (_ucTC);                                         \
        (_prMsduInfo)->ucPacketType = (_ucPacketType);                         \
        (_prMsduInfo)->ucFormatID = (_ucFormatID);                             \
        (_prMsduInfo)->fgIs802_1x = (_fgIs802_1x);                             \
        (_prMsduInfo)->fgIs802_11 = (_fgIs802_11);                             \
        (_prMsduInfo)->ucPsForwardingType = (_ucPsForwardingType);             \
    }

#define QM_INIT_STA_REC(_prStaRec, _fgIsValid, _fgIsQoS, _pucMacAddr) \
    {                                                                 \
        ASSERT(_prStaRec);                                            \
        (_prStaRec)->fgIsValid = (_fgIsValid);                        \
        (_prStaRec)->fgIsQoS = (_fgIsQoS);                            \
        (_prStaRec)->fgIsInPS = false;                                \
        (_prStaRec)->ucPsSessionID = 0xFF;                            \
        COPY_MAC_ADDR((_prStaRec)->aucMacAddr, (_pucMacAddr));        \
    }

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
#define QM_GET_TX_QUEUE_LEN(_prAdapter, _u4QueIdx)       \
    (((_prAdapter)->rQM.au4AverageQueLen[(_u4QueIdx)] >> \
      (_prAdapter)->rQM.u4QueLenMovingAverage))
#endif

#define WMM_IE_OUI_TYPE(fp)   (((P_IE_WMM_HDR_T)(fp))->ucOuiType)
#define WMM_IE_OUI_SUBTYPE(fp)    (((P_IE_WMM_HDR_T)(fp))->ucOuiSubtype)
#define WMM_IE_OUI(fp)        (((P_IE_WMM_HDR_T)(fp))->aucOui)

#if QM_DEBUG_COUNTER
#define QM_DBG_CNT_INC(_prQM, _index)            \
    {                                            \
        (_prQM)->au4QmDebugCounters[(_index)]++; \
    }
#else
#define QM_DBG_CNT_INC(_prQM, _index) \
    {                                 \
    }
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/* Queue Management and STA_REC Initialization                                */
/*----------------------------------------------------------------------------*/

void qmInit(IN P_ADAPTER_T prAdapter, IN u8 isTxResrouceControlEn);

#if QM_TEST_MODE
void qmTestCases(IN P_ADAPTER_T prAdapter);
#endif

void qmActivateStaRec(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

void qmDeactivateStaRec(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

void qmUpdateStaRec(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

/*----------------------------------------------------------------------------*/
/* TX-Related Queue Management                                                */
/*----------------------------------------------------------------------------*/

P_MSDU_INFO_T qmFlushTxQueues(IN P_ADAPTER_T prAdapter);

P_MSDU_INFO_T qmFlushStaTxQueues(IN P_ADAPTER_T prAdapter, IN u32 u4StaRecIdx);

P_MSDU_INFO_T qmEnqueueTxPackets(IN P_ADAPTER_T prAdapter,
                                 IN P_MSDU_INFO_T prMsduInfoListHead);

P_MSDU_INFO_T qmDequeueTxPackets(IN P_ADAPTER_T prAdapter,
                                 IN P_TX_TCQ_STATUS_T prTcqStatus);

P_MSDU_INFO_T qmDequeueTxPacketsMthread(IN P_ADAPTER_T prAdapter,
                                        IN P_TX_TCQ_STATUS_T prTcqStatus);

u8 qmAdjustTcQuotasMthread(IN P_ADAPTER_T prAdapter,
                           OUT P_TX_TCQ_ADJUST_T prTcqAdjust,
                           IN P_TX_TCQ_STATUS_T prTcqStatus);

u8 qmAdjustTcQuotas(IN P_ADAPTER_T prAdapter,
                    OUT P_TX_TCQ_ADJUST_T prTcqAdjust,
                    IN P_TX_TCQ_STATUS_T prTcqStatus);

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
void qmReassignTcResource(IN P_ADAPTER_T prAdapter);

void qmUpdateAverageTxQueLen(IN P_ADAPTER_T prAdapter);

void qmDoAdaptiveTcResourceCtrl(IN P_ADAPTER_T prAdapter);

void qmCheckForFastTcResourceCtrl(IN P_ADAPTER_T prAdapter, IN u8 ucTc);

#endif

void qmDetermineStaRecIndex(IN P_ADAPTER_T prAdapter,
                            IN P_MSDU_INFO_T prMsduInfo);

u32 qmDequeueTxPacketsFromPerStaQueues(IN P_ADAPTER_T prAdapter,
                                       OUT P_QUE_T prQue,
                                       IN u8 ucTC,
                                       IN u32 u4CurrentQuota,
                                       IN u32 u4TotalQuota);

void qmDequeueTxPacketsFromPerTypeQueues(IN P_ADAPTER_T prAdapter,
                                         OUT P_QUE_T prQue,
                                         IN u8 ucTC,
                                         IN u32 u4CurrentQuota,
                                         IN u32 u4TotalQuota);

u32 qmDequeueTxPacketsFromGlobalQueue(IN P_ADAPTER_T prAdapter,
                                      OUT P_QUE_T prQue,
                                      IN u8 ucTC,
                                      IN u32 u4CurrentQuota,
                                      IN u32 u4TotalQuota);

void qmSetStaRecTxAllowed(IN P_ADAPTER_T prAdapter,
                          IN P_STA_RECORD_T prStaRec,
                          IN u8 fgIsTxAllowed);

u32 gmGetDequeueQuota(IN P_ADAPTER_T prAdapter,
                      IN P_STA_RECORD_T prStaRec,
                      IN P_BSS_INFO_T prBssInfo,
                      IN u32 u4TotalQuota);

/*----------------------------------------------------------------------------*/
/* RX-Related Queue Management                                                */
/*----------------------------------------------------------------------------*/

void qmInitRxQueues(IN P_ADAPTER_T prAdapter);

P_SW_RFB_T qmFlushRxQueues(IN P_ADAPTER_T prAdapter);

P_QUE_T qmDetermineStaTxQueue(IN P_ADAPTER_T prAdapter,
                              IN P_MSDU_INFO_T prMsduInfo,
                              OUT u8 *pucTC);

void qmSetTxPacketDescTemplate(IN P_ADAPTER_T prAdapter,
                               IN P_MSDU_INFO_T prMsduInfo);

P_SW_RFB_T qmHandleRxPackets(IN P_ADAPTER_T prAdapter,
                             IN P_SW_RFB_T prSwRfbListHead);

void qmProcessPktWithReordering(IN P_ADAPTER_T prAdapter,
                                IN P_SW_RFB_T prSwRfb,
                                OUT P_QUE_T prReturnedQue);

void qmProcessBarFrame(IN P_ADAPTER_T prAdapter,
                       IN P_SW_RFB_T prSwRfb,
                       OUT P_QUE_T prReturnedQue);

void qmInsertReorderPkt(IN P_ADAPTER_T prAdapter,
                        IN P_SW_RFB_T prSwRfb,
                        IN P_RX_BA_ENTRY_T prReorderQueParm,
                        OUT P_QUE_T prReturnedQue);

void qmInsertFallWithinReorderPkt(IN P_ADAPTER_T prAdapter,
                                  IN P_SW_RFB_T prSwRfb,
                                  IN P_RX_BA_ENTRY_T prReorderQueParm,
                                  OUT P_QUE_T prReturnedQue);

void qmInsertFallAheadReorderPkt(IN P_ADAPTER_T prAdapter,
                                 IN P_SW_RFB_T prSwRfb,
                                 IN P_RX_BA_ENTRY_T prReorderQueParm,
                                 OUT P_QUE_T prReturnedQue);

void qmPopOutReorderPkt(IN P_ADAPTER_T prAdapter,
                        IN P_SW_RFB_T prSwRfb,
                        OUT P_QUE_T prReturnedQue,
                        IN ENUM_RX_STATISTIC_COUNTER_T eRxCounter);

void qmPopOutDueToFallWithin(IN P_ADAPTER_T prAdapter,
                             IN P_RX_BA_ENTRY_T prReorderQueParm,
                             OUT P_QUE_T prReturnedQue);

void qmPopOutDueToFallAhead(IN P_ADAPTER_T prAdapter,
                            IN P_RX_BA_ENTRY_T prReorderQueParm,
                            OUT P_QUE_T prReturnedQue);

void qmHandleReorderBubbleTimeout(IN P_ADAPTER_T prAdapter,
                                  IN unsigned long ulParamPtr);

void qmHandleEventCheckReorderBubble(IN P_ADAPTER_T prAdapter,
                                     IN P_WIFI_EVENT_T prEvent,
                                     IN u32 u4EventBufLen);

void qmHandleMailboxRxMessage(IN MAILBOX_MSG_T prMailboxRxMsg);

u8 qmCompareSnIsLessThan(IN u32 u4SnLess, IN u32 u4SnGreater);

void qmHandleEventRxAddBa(IN P_ADAPTER_T prAdapter,
                          IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen);

void qmHandleEventRxDelBa(IN P_ADAPTER_T prAdapter,
                          IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen);

P_RX_BA_ENTRY_T qmLookupRxBaEntry(IN P_ADAPTER_T prAdapter,
                                  IN u8 ucStaRecIdx,
                                  IN u8 ucTid);

u8 qmAddRxBaEntry(IN P_ADAPTER_T prAdapter,
                  IN u8 ucStaRecIdx,
                  IN u8 ucTid,
                  IN u16 u2WinStart,
                  IN u16 u2WinSize);

void qmDelRxBaEntry(IN P_ADAPTER_T prAdapter,
                    IN u8 ucStaRecIdx,
                    IN u8 ucTid,
                    IN u8 fgFlushToHost);

void mqmProcessAssocRsp(IN P_ADAPTER_T prAdapter,
                        IN P_SW_RFB_T prSwRfb,
                        IN u8 *pucIE,
                        IN u16 u2IELength);

void mqmProcessBcn(IN P_ADAPTER_T prAdapter,
                   IN P_SW_RFB_T prSwRfb,
                   IN u8 *pucIE,
                   IN u16 u2IELength);

u8 mqmParseEdcaParameters(IN P_ADAPTER_T prAdapter,
                          IN P_SW_RFB_T prSwRfb,
                          IN u8 *pucIE,
                          IN u16 u2IELength,
                          IN u8 fgForceOverride);

u8 mqmCompareEdcaParameters(IN P_IE_WMM_PARAM_T prIeWmmParam,
                            IN P_BSS_INFO_T prBssInfo);

void mqmFillAcQueParam(IN P_IE_WMM_PARAM_T prIeWmmParam,
                       IN u32 u4AcOffset,
                       OUT P_AC_QUE_PARMS_T prAcQueParams);

void mqmProcessScanResult(IN P_ADAPTER_T prAdapter,
                          IN P_BSS_DESC_T prScanResult,
                          OUT P_STA_RECORD_T prStaRec);

u32 mqmFillWmmInfoIE(u8 *pucOutBuf,
                     u8 fgSupportUAPSD,
                     u8 ucBmpDeliveryAC,
                     u8 ucBmpTriggerAC,
                     u8 ucUapsdSp);

u32 mqmGenerateWmmInfoIEByStaRec(P_ADAPTER_T prAdapter,
                                 P_BSS_INFO_T prBssInfo,
                                 P_STA_RECORD_T prStaRec,
                                 u8 *pucOutBuf);

void mqmGenerateWmmInfoIE(IN P_ADAPTER_T prAdapter,
                          IN P_MSDU_INFO_T prMsduInfo);

void mqmGenerateWmmParamIE(IN P_ADAPTER_T prAdapter,
                           IN P_MSDU_INFO_T prMsduInfo);

#if CFG_SUPPORT_TDLS

u32 mqmGenerateWmmParamIEByParam(P_ADAPTER_T prAdapter,
                                 P_BSS_INFO_T prBssInfo,
                                 u8 *pOutBuf);
#endif

ENUM_FRAME_ACTION_T qmGetFrameAction(IN P_ADAPTER_T prAdapter,
                                     IN u8 ucBssIndex,
                                     IN u8 ucStaRecIdx,
                                     IN P_MSDU_INFO_T prMsduInfo,
                                     IN ENUM_FRAME_TYPE_IN_CMD_Q_T eFrameType,
                                     IN u16 u2FrameLength);

void qmHandleEventBssAbsencePresence(IN P_ADAPTER_T prAdapter,
                                     IN P_WIFI_EVENT_T prEvent,
                                     IN u32 u4EventBufLen);

void qmHandleEventStaChangePsMode(IN P_ADAPTER_T prAdapter,
                                  IN P_WIFI_EVENT_T prEvent,
                                  IN u32 u4EventBufLen);

void mqmProcessAssocReq(IN P_ADAPTER_T prAdapter,
                        IN P_SW_RFB_T prSwRfb,
                        IN u8 *pucIE,
                        IN u16 u2IELength);

void qmHandleEventStaUpdateFreeQuota(IN P_ADAPTER_T prAdapter,
                                     IN P_WIFI_EVENT_T prEvent,
                                     IN u32 u4EventBufLen);

void qmUpdateFreeQuota(IN P_ADAPTER_T prAdapter,
                       IN P_STA_RECORD_T prStaRec,
                       IN u8 ucUpdateMode,
                       IN u8 ucFreeQuota);

void qmFreeAllByBssIdx(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex);

u32 qmGetRxReorderQueuedBufferCount(IN P_ADAPTER_T prAdapter);

u32 qmDumpQueueStatus(IN P_ADAPTER_T prAdapter, IN u8 *pucBuf, IN u32 u4MaxLen);

void qmResetTcControlResource(IN P_ADAPTER_T prAdapter);

#if (CFG_SUPPORT_REPLAY_DETECTION || CFG_SUPPORT_FRAG_ATTACK_DETECTION)
#define CCMPTSCPNNUM    6
u8 qmRxPNtoU64(u8 *pucPN, u8 uPNNum, u64 *pu64Rets);
#endif

#if CFG_SUPPORT_REPLAY_DETECTION
u8 qmHandleRxReplay(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb);
u8 qmRxDetectReplay(u8 *pucPNS, u8 *pucPNT);
#endif

#if CFG_SUPPORT_FAKE_EAPOL_DETECTION
u8 qmDetectRxInvalidEAPOL(P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);
#endif

#if CFG_SUPPORT_AMSDU_ATTACK_DETECTION
u8 qmAmsduAttackDetection(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);
#endif

#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT
void qmFuncChangeBmcTcIdx(u8 ucTc);
u8 qmFuncGetBmcTcIdx(u8 ucWmmIdx);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#if QM_TEST_MODE
extern QUE_MGT_T g_rQM;
#endif
extern const u8 aucTid2ACI[TX_DESC_TID_NUM];
extern u8 arNetwork2TcResource[HW_BSSID_NUM + 1][NET_TC_NUM];

#endif
