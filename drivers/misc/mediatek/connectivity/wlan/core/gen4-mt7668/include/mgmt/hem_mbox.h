/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   hem_mbox.h
 *    \brief
 *
 */

#ifndef _HEM_MBOX_H
#define _HEM_MBOX_H

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
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* Message IDs */
typedef enum _ENUM_MSG_ID_T {
    MID_MNY_CNM_CH_REQ,  /* MANY notify CNM to obtain channel privilege */
    MID_MNY_CNM_CH_ABORT,  /* MANY notify CNM to abort/release channel
                            * privilege */

    MID_CNM_AIS_CH_GRANT,  /* CNM notify AIS for indicating channel granted
                            */
    MID_CNM_P2P_CH_GRANT,  /* CNM notify P2P for indicating channel granted
                            */
    MID_CNM_BOW_CH_GRANT,  /* CNM notify BOW for indicating channel granted
                            */

#if (CFG_SUPPORT_DFS_MASTER == 1)
    MID_CNM_P2P_RADAR_DETECT,
    MID_CNM_P2P_CSA_DONE,
#endif
    /*--------------------------------------------------*/
    /* SCN Module Mailbox Messages                      */
    /*--------------------------------------------------*/
    MID_AIS_SCN_SCAN_REQ,  /* AIS notify SCN for starting scan */
    MID_AIS_SCN_SCAN_REQ_V2,  /* AIS notify SCN for starting scan with
                               * multiple SSID support */
    MID_AIS_SCN_SCAN_CANCEL,  /* AIS notify SCN for cancelling scan */
    MID_P2P_SCN_SCAN_REQ,  /* P2P notify SCN for starting scan */
    MID_P2P_SCN_SCAN_REQ_V2,  /* P2P notify SCN for starting scan with
                               * multiple SSID support */
    MID_P2P_SCN_SCAN_CANCEL,  /* P2P notify SCN for cancelling scan */
    MID_BOW_SCN_SCAN_REQ,  /* BOW notify SCN for starting scan */
    MID_BOW_SCN_SCAN_REQ_V2,  /* BOW notify SCN for starting scan with
                               * multiple SSID support */
    MID_BOW_SCN_SCAN_CANCEL,  /* BOW notify SCN for cancelling scan */
    MID_RLM_SCN_SCAN_REQ,  /* RLM notify SCN for starting scan (OBSS-SCAN) */
    MID_RLM_SCN_SCAN_REQ_V2,  /* RLM notify SCN for starting scan (OBSS-SCAN)
                               * with multiple SSID support */
    MID_RLM_SCN_SCAN_CANCEL,  /* RLM notify SCN for cancelling scan
                               * (OBSS-SCAN) */
    MID_SCN_AIS_SCAN_DONE,  /* SCN notify AIS for scan completion */
    MID_SCN_P2P_SCAN_DONE,  /* SCN notify P2P for scan completion */
    MID_SCN_BOW_SCAN_DONE,  /* SCN notify BOW for scan completion */
    MID_SCN_RLM_SCAN_DONE,  /* SCN notify RLM for scan completion (OBSS-SCAN)
                             */

    /*--------------------------------------------------*/
    /* AIS Module Mailbox Messages                      */
    /*--------------------------------------------------*/
    MID_OID_AIS_FSM_JOIN_REQ,  /* OID/IOCTL notify AIS for join */
    MID_OID_AIS_FSM_ABORT,  /* OID/IOCTL notify AIS for abort */
    MID_AIS_SAA_FSM_START,  /* AIS notify SAA for Starting
                             * authentication/association fsm */
    MID_AIS_SAA_FSM_ABORT,  /* AIS notify SAA for Aborting
                             * authentication/association fsm */
    MID_SAA_AIS_JOIN_COMPLETE,  /* SAA notify AIS for indicating join
                                 * complete */

#if CFG_ENABLE_WIFI_DIRECT
    /*--------------------------------------------------*/
    /* P2P Module Mailbox Messages                      */
    /*--------------------------------------------------*/
    MID_P2P_SAA_FSM_START,  /* P2P notify SAA for Starting
                             * authentication/association fsm */
    MID_P2P_SAA_FSM_ABORT,  /* P2P notify SAA for Aborting
                             * authentication/association fsm */
    MID_SAA_P2P_JOIN_COMPLETE,  /* SAA notify P2P for indicating join
                                 * complete */

    MID_MNY_P2P_FUN_SWITCH,  /* Enable P2P FSM. */
    MID_MNY_P2P_DEVICE_DISCOVERY,  /* Start device discovery. */
    MID_MNY_P2P_CONNECTION_REQ,  /* Connection request. */
    MID_MNY_P2P_CONNECTION_ABORT,  /* Abort connection request, P2P FSM
                                    * return to IDLE. */
    MID_MNY_P2P_BEACON_UPDATE,
    MID_MNY_P2P_STOP_AP,
    MID_MNY_P2P_CHNL_REQ,
    MID_MNY_P2P_CHNL_ABORT,
    MID_MNY_P2P_MGMT_TX,
    MID_MNY_P2P_GROUP_DISSOLVE,
    MID_MNY_P2P_MGMT_FRAME_REGISTER,
    MID_MNY_P2P_NET_DEV_REGISTER,
    MID_MNY_P2P_START_AP,
    MID_MNY_P2P_DEL_IFACE,
    MID_MNY_P2P_MGMT_FRAME_UPDATE,
#if (CFG_SUPPORT_DFS_MASTER == 1)
    MID_MNY_P2P_DFS_CAC,
    MID_MNY_P2P_SET_NEW_CHANNEL,
#endif
#if CFG_SUPPORT_WFD
    MID_MNY_P2P_WFD_CFG_UPDATE,
#endif
    MID_MNY_P2P_ACTIVE_BSS,
#endif

#if CFG_SUPPORT_ADHOC
    MID_SCN_AIS_FOUND_IBSS,  /* SCN notify AIS that an IBSS Peer has been
                              * found and can merge into */
#endif

    MID_SAA_AIS_FSM_ABORT,  /* SAA notify AIS for indicating
                             * deauthentication/disassociation */

    /*--------------------------------------------------*/
    /* AIS MGMT-TX Support                              */
    /*--------------------------------------------------*/
    MID_MNY_AIS_REMAIN_ON_CHANNEL,
    MID_MNY_AIS_CANCEL_REMAIN_ON_CHANNEL,
    MID_MNY_AIS_MGMT_TX,
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
    MID_WNM_AIS_BSS_TRANSITION,
#endif
    MID_TOTAL_NUM
} ENUM_MSG_ID_T,
*P_ENUM_MSG_ID_T;

/* Message header of inter-components */
struct _MSG_HDR_T {
    LINK_ENTRY_T rLinkEntry;
    ENUM_MSG_ID_T eMsgId;
};

typedef void (*PFN_MSG_HNDL_FUNC)(P_ADAPTER_T, P_MSG_HDR_T);

typedef struct _MSG_HNDL_ENTRY {
    ENUM_MSG_ID_T eMsgId;
    PFN_MSG_HNDL_FUNC pfMsgHndl;
} MSG_HNDL_ENTRY_T, *P_MSG_HNDL_ENTRY_T;

typedef enum _EUNM_MSG_SEND_METHOD_T {
    MSG_SEND_METHOD_BUF = 0,  /* Message is put in the queue and will be */
    /*executed when mailbox is checked. */
    MSG_SEND_METHOD_UNBUF  /* The handler function is called immediately */
    /* in the same context of the sender */
} EUNM_MSG_SEND_METHOD_T,
*P_EUNM_MSG_SEND_METHOD_T;

typedef enum _ENUM_MBOX_ID_T {
    MBOX_ID_0 = 0,
    MBOX_ID_TOTAL_NUM
} ENUM_MBOX_ID_T,
*P_ENUM_MBOX_ID_T;

/* Define Mailbox structure */
typedef struct _MBOX_T {
    LINK_T rLinkHead;
} MBOX_T, *P_MBOX_T;

typedef struct _MSG_SAA_FSM_START_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucSeqNum;
    P_STA_RECORD_T prStaRec;
} MSG_SAA_FSM_START_T, *P_MSG_SAA_FSM_START_T;

typedef struct _MSG_SAA_FSM_COMP_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucSeqNum;
    WLAN_STATUS rJoinStatus;
    P_STA_RECORD_T prStaRec;
    P_SW_RFB_T prSwRfb;
} MSG_SAA_FSM_COMP_T, *P_MSG_SAA_FSM_COMP_T;

typedef struct _MSG_SAA_FSM_ABORT_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucSeqNum;
    P_STA_RECORD_T prStaRec;
} MSG_SAA_FSM_ABORT_T, *P_MSG_SAA_FSM_ABORT_T;

typedef struct _MSG_CONNECTION_ABORT_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u8 ucNetTypeIndex;
} MSG_CONNECTION_ABORT_T, *P_MSG_CONNECTION_ABORT_T;

typedef struct _MSG_REMAIN_ON_CHANNEL_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    ENUM_BAND_T eBand;
    ENUM_CHNL_EXT_T eSco;
    u8 ucChannelNum;
    u32 u4DurationMs;
    u64 u8Cookie;
} MSG_REMAIN_ON_CHANNEL_T, *P_MSG_REMAIN_ON_CHANNEL_T;

typedef struct _MSG_CANCEL_REMAIN_ON_CHANNEL_T {
    MSG_HDR_T rMsgHdr;  /* Must be the first member */
    u64 u8Cookie;
} MSG_CANCEL_REMAIN_ON_CHANNEL_T, *P_MSG_CANCEL_REMAIN_ON_CHANNEL_T;

typedef struct _MSG_MGMT_TX_REQUEST_T {
    MSG_HDR_T rMsgHdr;
    P_MSDU_INFO_T prMgmtMsduInfo;
    u64 u8Cookie;  /* For indication. */
    u8 fgNoneCckRate;
    u8 fgIsWaitRsp;
} MSG_MGMT_TX_REQUEST_T, *P_MSG_MGMT_TX_REQUEST_T;

/* specific message data types */
typedef MSG_SAA_FSM_START_T MSG_JOIN_REQ_T, *P_MSG_JOIN_REQ_T;
typedef MSG_SAA_FSM_COMP_T MSG_JOIN_COMP_T, *P_MSG_JOIN_COMP_T;
typedef MSG_SAA_FSM_ABORT_T MSG_JOIN_ABORT_T, *P_MSG_JOIN_ABORT_T;

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
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void mboxSetup(IN P_ADAPTER_T prAdapter, IN ENUM_MBOX_ID_T eMboxId);

void mboxSendMsg(IN P_ADAPTER_T prAdapter,
                 IN ENUM_MBOX_ID_T eMboxId,
                 IN P_MSG_HDR_T prMsg,
                 IN EUNM_MSG_SEND_METHOD_T eMethod);

void mboxRcvAllMsg(IN P_ADAPTER_T prAdapter, IN ENUM_MBOX_ID_T eMboxId);

void mboxInitialize(IN P_ADAPTER_T prAdapter);

void mboxDestroy(IN P_ADAPTER_T prAdapter);

void mboxDummy(IN P_ADAPTER_T prAdapter, P_MSG_HDR_T prMsgHdr);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
