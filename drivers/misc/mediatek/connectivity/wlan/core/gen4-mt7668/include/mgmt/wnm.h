/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file  wnm.h
 *    \brief This file contains the IEEE 802.11 family related 802.11v network
 * management for MediaTek 802.11 Wireless LAN Adapters.
 */

#ifndef _WNM_H
#define _WNM_H

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

#define BTM_REQ_MODE_CAND_INCLUDED_BIT            BIT(0)
#define BTM_REQ_MODE_ABRIDGED                 BIT(1)
#define BTM_REQ_MODE_DISC_IMM                 BIT(2)
#define BTM_REQ_MODE_BSS_TERM_INCLUDE             BIT(3)
#define BTM_REQ_MODE_ESS_DISC_IMM             BIT(4)

#define BSS_TRANSITION_MGT_STATUS_ACCEPT          0
#define BSS_TRANSITION_MGT_STATUS_UNSPECIFIED         1
#define BSS_TRANSITION_MGT_STATUS_NEED_SCAN       2
#define BSS_TRANSITION_MGT_STATUS_CAND_NO_CAPACITY    3
#define BSS_TRANSITION_MGT_STATUS_TERM_UNDESIRED      4
#define BSS_TRANSITION_MGT_STATUS_TERM_DELAY_REQUESTED    5
#define BSS_TRANSITION_MGT_STATUS_CAND_LIST_PROVIDED      6
#define BSS_TRANSITION_MGT_STATUS_CAND_NO_CANDIDATES      7
#define BSS_TRANSITION_MGT_STATUS_LEAVING_ESS         8

/* 802.11v: define Transtion and Transition Query reasons */
#define BSS_TRANSITION_BETTER_AP_FOUND            6
#define BSS_TRANSITION_LOW_RSSI               16
#define BSS_TRANSITION_INCLUDE_PREFER_CAND_LIST       19
#define BSS_TRANSITION_LEAVING_ESS            20

/*******************************************************************************
 *                         D A T A   T Y P E S
 *******************************************************************************
 */

typedef struct _TIMINGMSMT_PARAM_T {
    u8 fgInitiator;
    u8 ucTrigger;
    u8 ucDialogToken;  /* Dialog Token */
    u8 ucFollowUpDialogToken;  /* Follow Up Dialog Token */
    u32 u4ToD;  /* Timestamp of Departure [10ns] */
    u32 u4ToA;  /* Timestamp of Arrival [10ns] */
} TIMINGMSMT_PARAM_T, *P_TIMINGMSMT_PARAM_T;

typedef struct _BSS_TRANSITION_MGT_PARAM_T {
    /* for Query */
    u8 ucDialogToken;
    u8 ucQueryReason;
    /* for Request */
    u8 ucRequestMode;
    u16 u2DisassocTimer;
    u16 u2TermDuration;
    u8 aucTermTsf[8];
    u8 ucSessionURLLen;
    u8 aucSessionURL[255];
    /* for Respone */
    u8 fgPendingResponse : 1;
    u8 fgUnsolicitedReq : 1;
    u8 fgReserved : 6;
    u8 ucStatusCode;
    u8 ucTermDelay;
    u8 aucTargetBssid[MAC_ADDR_LEN];
    u8 *pucOurNeighborBss;
    u16 u2OurNeighborBssLen;
} BSS_TRANSITION_MGT_PARAM_T, *P_BSS_TRANSITION_MGT_PARAM_T;

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

void wnmWNMAction(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

void wnmReportTimingMeas(IN P_ADAPTER_T prAdapter,
                         IN u8 ucStaRecIndex,
                         IN u32 u4ToD,
                         IN u32 u4ToA);

#define WNM_UNIT_TEST    1

#if WNM_UNIT_TEST
void wnmTimingMeasUnitTest1(P_ADAPTER_T prAdapter, u8 ucStaRecIndex);
#endif

u8 wnmGetBtmToken(void);

void wnmSendBTMQueryFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

void wnmSendBTMResponseFrame(IN P_ADAPTER_T prAdapter,
                             IN P_STA_RECORD_T prStaRec);

void wnmRecvBTMRequest(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
