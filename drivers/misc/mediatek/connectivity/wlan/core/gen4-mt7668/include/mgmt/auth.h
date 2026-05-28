/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file  auth.h
 *    \brief This file contains the authentication REQ/RESP of
 *    IEEE 802.11 family for MediaTek 802.11 Wireless LAN Adapters.
 */

#ifndef _AUTH_H
#define _AUTH_H

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
 *                         D A T A   T Y P E S
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
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/* Routines in auth.c                                                         */
/*----------------------------------------------------------------------------*/
void authAddIEChallengeText(IN P_ADAPTER_T prAdapter,
                            IN OUT P_MSDU_INFO_T prMsduInfo);

#if !CFG_SUPPORT_AAA
WLAN_STATUS authSendAuthFrame(IN P_ADAPTER_T prAdapter,
                              IN P_STA_RECORD_T prStaRec,
                              IN u16 u2TransactionSeqNum);
#else
WLAN_STATUS
authSendAuthFrame(IN P_ADAPTER_T prAdapter,
                  IN P_STA_RECORD_T prStaRec,
                  IN u8 uBssIndex,
                  IN P_SW_RFB_T prFalseAuthSwRfb,
                  IN u16 u2TransactionSeqNum,
                  IN u16 u2StatusCode);
#endif

WLAN_STATUS authCheckTxAuthFrame(IN P_ADAPTER_T prAdapter,
                                 IN P_MSDU_INFO_T prMsduInfo,
                                 IN u16 u2TransactionSeqNum);

WLAN_STATUS authCheckRxAuthFrameTransSeq(IN P_ADAPTER_T prAdapter,
                                         IN P_SW_RFB_T prSwRfb);

WLAN_STATUS
authCheckRxAuthFrameStatus(IN P_ADAPTER_T prAdapter,
                           IN P_SW_RFB_T prSwRfb,
                           IN u16 u2TransactionSeqNum,
                           OUT u16 *pu2StatusCode);

void authHandleIEChallengeText(P_ADAPTER_T prAdapter,
                               P_SW_RFB_T prSwRfb,
                               P_IE_HDR_T prIEHdr);

WLAN_STATUS authProcessRxAuth2_Auth4Frame(IN P_ADAPTER_T prAdapter,
                                          IN P_SW_RFB_T prSwRfb);

WLAN_STATUS
authSendDeauthFrame(IN P_ADAPTER_T prAdapter,
                    IN P_BSS_INFO_T prBssInfo,
                    IN P_STA_RECORD_T prStaRec,
                    IN P_SW_RFB_T prClassErrSwRfb,
                    IN u16 u2ReasonCode,
                    IN PFN_TX_DONE_HANDLER pfTxDoneHandler);

WLAN_STATUS authProcessRxDeauthFrame(IN P_SW_RFB_T prSwRfb,
                                     IN u8 aucBSSID[],
                                     OUT u16 *pu2ReasonCode);

WLAN_STATUS
authProcessRxAuth1Frame(IN P_ADAPTER_T prAdapter,
                        IN P_SW_RFB_T prSwRfb,
                        IN u8 aucExpectedBSSID[],
                        IN u16 u2ExpectedAuthAlgNum,
                        IN u16 u2ExpectedTransSeqNum,
                        OUT u16 *pu2ReturnStatusCode);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
