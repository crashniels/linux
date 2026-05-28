/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "rlm.h"
 *  \brief
 */

#ifndef _P2P_RLM_H
#define _P2P_RLM_H

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

#define CHANNEL_SPAN_20    20

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
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
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void rlmBssInitForAP(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);

u8 rlmUpdateBwByChListForAP(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);

void rlmUpdateParamsForAP(P_ADAPTER_T prAdapter,
                          P_BSS_INFO_T prBssInfo,
                          u8 fgUpdateBeacon);

void rlmFuncInitialChannelList(IN P_ADAPTER_T prAdapter);

void rlmFuncCommonChannelList(IN P_ADAPTER_T prAdapter,
                              IN P_CHANNEL_ENTRY_FIELD_T prChannelEntryII,
                              IN u8 ucChannelListSize);

u8 rlmFuncFindOperatingClass(IN P_ADAPTER_T prAdapter, IN u8 ucChannelNum);

u8 rlmFuncFindAvailableChannel(IN P_ADAPTER_T prAdapter,
                               IN u8 ucCheckChnl,
                               IN u8 *pucSuggestChannel,
                               IN u8 fgIsSocialChannel,
                               IN u8 fgIsDefaultChannel);

ENUM_CHNL_EXT_T rlmDecideScoForAP(P_ADAPTER_T prAdapter,
                                  P_BSS_INFO_T prBssInfo);

ENUM_CHNL_EXT_T rlmGetScoForAP(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);

u8 rlmGetVhtS1ForAP(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);

#endif
