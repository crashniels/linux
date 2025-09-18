/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */
#ifndef _P2P_DEV_STATE_H
#define _P2P_DEV_STATE_H

u8 p2pDevStateInit_IDLE(IN P_ADAPTER_T prAdapter,
                        IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo,
                        OUT P_ENUM_P2P_DEV_STATE_T peNextState);

void p2pDevStateAbort_IDLE(IN P_ADAPTER_T prAdapter);

u8 p2pDevStateInit_REQING_CHANNEL(IN P_ADAPTER_T prAdapter,
                                  IN u8 ucBssIdx,
                                  IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo,
                                  OUT P_ENUM_P2P_DEV_STATE_T peNextState);

void p2pDevStateAbort_REQING_CHANNEL(IN P_ADAPTER_T prAdapter,
                                     IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo,
                                     IN ENUM_P2P_DEV_STATE_T eNextState);

void p2pDevStateInit_CHNL_ON_HAND(IN P_ADAPTER_T prAdapter,
                                  IN P_BSS_INFO_T prP2pBssInfo,
                                  IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo,
                                  IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo);

void p2pDevStateAbort_CHNL_ON_HAND(IN P_ADAPTER_T prAdapter,
                                   IN P_BSS_INFO_T prP2pBssInfo,
                                   IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo,
                                   IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo);

void p2pDevStateInit_SCAN(IN P_ADAPTER_T prAdapter,
                          IN u8 ucBssIndex,
                          IN P_P2P_SCAN_REQ_INFO_T prScanReqInfo);

void p2pDevStateAbort_SCAN(IN P_ADAPTER_T prAdapter,
                           IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo);

u8 p2pDevStateInit_OFF_CHNL_TX(IN P_ADAPTER_T prAdapter,
                               IN P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo,
                               IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo,
                               IN P_P2P_MGMT_TX_REQ_INFO_T prP2pMgmtTxInfo,
                               OUT P_ENUM_P2P_DEV_STATE_T peNextState);

void p2pDevStateAbort_OFF_CHNL_TX(IN P_ADAPTER_T prAdapter,
                                  IN P_P2P_MGMT_TX_REQ_INFO_T prP2pMgmtTxInfo,
                                  IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo,
                                  IN ENUM_P2P_DEV_STATE_T eNextState);

#endif
