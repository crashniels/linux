// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "scan.c"
 *    \brief  This file defines the scan profile and the processing function of
 *     scan result for SCAN Module.
 *
 *    The SCAN Profile selection is part of SCAN MODULE and responsible for
 * defining SCAN Parameters - e.g. MIN_CHANNEL_TIME, number of scan channels. In
 * this file we also define the process of SCAN Result including adding,
 * searching and removing SCAN record from the list.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "precomp.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define REPLICATED_BEACON_TIME_THRESHOLD (3000)
#define REPLICATED_BEACON_FRESH_PERIOD (10000)
#define REPLICATED_BEACON_STRENGTH_THRESHOLD (32)

#define ROAMING_NO_SWING_RCPI_STEP (10)

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
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used by SCN to initialize its variables
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scnInit(IN P_ADAPTER_T prAdapter)
{
    P_SCAN_INFO_T prScanInfo;
    P_BSS_DESC_T prBSSDesc;
    u8 *pucBSSBuff;
    u32 i;
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
    P_ROAM_BSS_DESC_T prRoamBSSDesc;
    u8 *pucRoamBSSBuff;
#endif

    ASSERT(prAdapter);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
    pucBSSBuff = &prScanInfo->aucScanBuffer[0];
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
    pucRoamBSSBuff = &prScanInfo->aucScanRoamBuffer[0];
#endif

    DBGLOG(SCN, INFO, "->scnInit()\n");

    /* 4 <1> Reset STATE and Message List */
    prScanInfo->eCurrentState = SCAN_STATE_IDLE;

    prScanInfo->rLastScanCompletedTime = (u32)0;

    LINK_INITIALIZE(&prScanInfo->rPendingMsgList);

    /* 4 <2> Reset link list of BSS_DESC_T */
    kalMemZero((void *)pucBSSBuff, SCN_MAX_BUFFER_SIZE);
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
    kalMemZero((void *)pucRoamBSSBuff, SCN_ROAM_MAX_BUFFER_SIZE);
#endif

    LINK_INITIALIZE(&prScanInfo->rFreeBSSDescList);
    LINK_INITIALIZE(&prScanInfo->rBSSDescList);
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
    LINK_INITIALIZE(&prScanInfo->rRoamFreeBSSDescList);
    LINK_INITIALIZE(&prScanInfo->rRoamBSSDescList);
#endif

    for (i = 0; i < CFG_MAX_NUM_BSS_LIST; i++) {
        prBSSDesc = (P_BSS_DESC_T)pucBSSBuff;

        LINK_INSERT_TAIL(&prScanInfo->rFreeBSSDescList, &prBSSDesc->rLinkEntry);

        pucBSSBuff += ALIGN_4(sizeof(BSS_DESC_T));
    }
    /* Check if the memory allocation consist with this initialization
     * function */
    ASSERT(((unsigned long)pucBSSBuff -
            (unsigned long)&prScanInfo->aucScanBuffer[0]) ==
           SCN_MAX_BUFFER_SIZE);

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
    for (i = 0; i < CFG_MAX_NUM_ROAM_BSS_LIST; i++) {
        prRoamBSSDesc = (P_ROAM_BSS_DESC_T)pucRoamBSSBuff;

        LINK_INSERT_TAIL(&prScanInfo->rRoamFreeBSSDescList,
                         &prRoamBSSDesc->rLinkEntry);

        pucRoamBSSBuff += ALIGN_4(sizeof(ROAM_BSS_DESC_T));
    }
    ASSERT(((unsigned long)pucRoamBSSBuff -
            (unsigned long)&prScanInfo->aucScanRoamBuffer[0]) ==
           SCN_ROAM_MAX_BUFFER_SIZE);
#endif
    /* reset freest channel information */
    prScanInfo->fgIsSparseChannelValid = false;

    /* reset NLO state */
    prScanInfo->fgNloScanning = false;
}

void scnFreeAllPendingScanRquests(IN P_ADAPTER_T prAdapter)
{
    P_SCAN_INFO_T prScanInfo;
    P_MSG_HDR_T prMsgHdr;
    P_MSG_SCN_SCAN_REQ prScanReqMsg;

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    /* check for pending scanning requests */
    while (!LINK_IS_EMPTY(&(prScanInfo->rPendingMsgList))) {
        /* load next message from pending list as scan parameters */
        LINK_REMOVE_HEAD(&(prScanInfo->rPendingMsgList), prMsgHdr, P_MSG_HDR_T);
        if (prMsgHdr) {
            prScanReqMsg = (P_MSG_SCN_SCAN_REQ)prMsgHdr;
            DBGLOG(SCN, INFO,
                   "free scan request eMsgId[%d] ucSeqNum [%d] BSSID[%d]!!\n",
                   prMsgHdr->eMsgId, prScanReqMsg->ucSeqNum,
                   prScanReqMsg->ucBssIndex);
            cnmMemFree(prAdapter, prMsgHdr);
        } else {
            /* should not deliver to this function */
            ASSERT(0);
        }
        /* switch to next state */
    }

    DBGLOG(SCN, INFO, "%s()\n", __func__);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used by SCN to uninitialize its variables
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scnUninit(IN P_ADAPTER_T prAdapter)
{
    P_SCAN_INFO_T prScanInfo;

    ASSERT(prAdapter);
    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    DBGLOG(SCN, INFO, "%s()\n", __func__);

    scnFreeAllPendingScanRquests(prAdapter);

    /* 4 <1> Reset STATE and Message List */
    prScanInfo->eCurrentState = SCAN_STATE_IDLE;

    prScanInfo->rLastScanCompletedTime = (u32)0;

    /* NOTE(Kevin): Check rPendingMsgList ? */

    /* 4 <2> Reset link list of BSS_DESC_T */
    LINK_INITIALIZE(&prScanInfo->rFreeBSSDescList);
    LINK_INITIALIZE(&prScanInfo->rBSSDescList);
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
    LINK_INITIALIZE(&prScanInfo->rRoamFreeBSSDescList);
    LINK_INITIALIZE(&prScanInfo->rRoamBSSDescList);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to given BSSID
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] aucBSSID           Given BSSID.
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T scanSearchBssDescByBssid(IN P_ADAPTER_T prAdapter,
                                      IN u8 aucBSSID[])
{
    return scanSearchBssDescByBssidAndSsid(prAdapter, aucBSSID, false, NULL);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to given BSSID
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] aucBSSID           Given BSSID.
 * @param[in] fgCheckSsid        Need to check SSID or not. (for multiple SSID
 * with single BSSID cases)
 * @param[in] prSsid             Specified SSID
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T
scanSearchBssDescByBssidAndSsid(IN P_ADAPTER_T prAdapter, IN u8 aucBSSID[],
                                IN u8 fgCheckSsid, IN P_PARAM_SSID_T prSsid)
{
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prBSSDescList;
    P_BSS_DESC_T prBssDesc;
    P_BSS_DESC_T prDstBssDesc = (P_BSS_DESC_T)NULL;

    ASSERT(prAdapter);
    ASSERT(aucBSSID);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    prBSSDescList = &prScanInfo->rBSSDescList;

    /* Search BSS Desc from current SCAN result list. */
    LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T){
        if (!(EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID))) {
            continue;
        }
        if (fgCheckSsid == false || prSsid == NULL) {
            return prBssDesc;
        }

        if (EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
                       prSsid->aucSsid, prSsid->u4SsidLen)) {
            return prBssDesc;
        }
        if (prDstBssDesc == NULL && prBssDesc->fgIsHiddenSSID == true) {
            prDstBssDesc = prBssDesc;
            continue;
        }
        if (prBssDesc->eBSSType == BSS_TYPE_P2P_DEVICE) {
            /* 20120206 frog: Equal BSSID but not SSID,
             * SSID not hidden, SSID must be updated.
             */
            COPY_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen, prSsid->aucSsid,
                      (u8)(prSsid->u4SsidLen));
            return prBssDesc;
        }
    }

    return prDstBssDesc;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor
 *        according to given BSSID & ChanNum
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] aucBSSID           Given BSSID.
 * @param[in] fgCheckChanNum     Need to check ChanNum or not.
 * @param[in] ucChannelNum       Specified Channel Num
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T scanSearchBssDescByBssidAndChanNum(IN P_ADAPTER_T prAdapter,
                                                IN u8 aucBSSID[],
                                                IN u8 fgCheckChanNum,
                                                IN u8 ucChannelNum)
{
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prBSSDescList;
    P_BSS_DESC_T prBssDesc = (P_BSS_DESC_T)NULL;

    ASSERT(prAdapter);
    ASSERT(aucBSSID);
    ASSERT(ucChannelNum);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    prBSSDescList = &prScanInfo->rBSSDescList;

    /* Search BSS Desc from current SCAN result list. */
    LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T){
        if (!(EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID))) {
            continue;
        }
        if (fgCheckChanNum == false || ucChannelNum == 0) {
            return prBssDesc;
        }

        if (prBssDesc->ucChannelNum == ucChannelNum) {
            return prBssDesc;
        }
    }

    return prBssDesc;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to given Transmitter
 * Address.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] aucSrcAddr         Given Source Address(TA).
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T scanSearchBssDescByTA(IN P_ADAPTER_T prAdapter,
                                   IN u8 aucSrcAddr[])
{
    return scanSearchBssDescByTAAndSsid(prAdapter, aucSrcAddr, false, NULL);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to given Transmitter
 * Address.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] aucSrcAddr         Given Source Address(TA).
 * @param[in] fgCheckSsid        Need to check SSID or not. (for multiple SSID
 * with single BSSID cases)
 * @param[in] prSsid             Specified SSID
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T
scanSearchBssDescByTAAndSsid(IN P_ADAPTER_T prAdapter, IN u8 aucSrcAddr[],
                             IN u8 fgCheckSsid, IN P_PARAM_SSID_T prSsid)
{
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prBSSDescList;
    P_BSS_DESC_T prBssDesc;
    P_BSS_DESC_T prDstBssDesc = (P_BSS_DESC_T)NULL;

    ASSERT(prAdapter);
    ASSERT(aucSrcAddr);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    prBSSDescList = &prScanInfo->rBSSDescList;

    /* Search BSS Desc from current SCAN result list. */
    LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T){
        if (EQUAL_MAC_ADDR(prBssDesc->aucSrcAddr, aucSrcAddr)) {
            if (fgCheckSsid == false || prSsid == NULL) {
                return prBssDesc;
            }

            if (EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
                           prSsid->aucSsid, prSsid->u4SsidLen)) {
                return prBssDesc;
            } else if (prDstBssDesc == NULL &&
                       prBssDesc->fgIsHiddenSSID == true) {
                prDstBssDesc = prBssDesc;
            }
        }
    }

    return prDstBssDesc;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to
 *        given eBSSType, BSSID and Transmitter Address
 *
 * @param[in] prAdapter  Pointer to the Adapter structure.
 * @param[in] eBSSType   BSS Type of incoming Beacon/ProbeResp frame.
 * @param[in] aucBSSID   Given BSSID of Beacon/ProbeResp frame.
 * @param[in] aucSrcAddr Given source address (TA) of Beacon/ProbeResp frame.
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T
scanSearchExistingBssDesc(IN P_ADAPTER_T prAdapter, IN ENUM_BSS_TYPE_T eBSSType,
                          IN u8 aucBSSID[], IN u8 aucSrcAddr[])
{
    return scanSearchExistingBssDescWithSsid(prAdapter, eBSSType, aucBSSID,
                                             aucSrcAddr, false, NULL);
}

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
void scanRemoveRoamBssDescsByTime(IN P_ADAPTER_T prAdapter,
                                  IN u32 u4RemoveTime)
{
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prRoamBSSDescList;
    P_LINK_T prRoamFreeBSSDescList;
    P_ROAM_BSS_DESC_T prRoamBssDesc;
    P_ROAM_BSS_DESC_T prRoamBSSDescNext;
    u32 rCurrentTime;

    ASSERT(prAdapter);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
    prRoamBSSDescList = &prScanInfo->rRoamBSSDescList;
    prRoamFreeBSSDescList = &prScanInfo->rRoamFreeBSSDescList;

    GET_CURRENT_SYSTIME(&rCurrentTime);

    LINK_FOR_EACH_ENTRY_SAFE(prRoamBssDesc, prRoamBSSDescNext,
                             prRoamBSSDescList, rLinkEntry, ROAM_BSS_DESC_T){
        if (CHECK_FOR_TIMEOUT(rCurrentTime, prRoamBssDesc->rUpdateTime,
                              SEC_TO_SYSTIME(u4RemoveTime))) {
            LINK_REMOVE_KNOWN_ENTRY(prRoamBSSDescList, prRoamBssDesc);
            LINK_INSERT_TAIL(prRoamFreeBSSDescList, &prRoamBssDesc->rLinkEntry);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
P_ROAM_BSS_DESC_T
scanSearchRoamBssDescBySsid(IN P_ADAPTER_T prAdapter,
                            IN P_BSS_DESC_T prBssDesc)
{
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prRoamBSSDescList;
    P_ROAM_BSS_DESC_T prRoamBssDesc;

    ASSERT(prAdapter);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    prRoamBSSDescList = &prScanInfo->rRoamBSSDescList;

    /* Search BSS Desc from current SCAN result list. */
    LINK_FOR_EACH_ENTRY(prRoamBssDesc, prRoamBSSDescList, rLinkEntry,
                        ROAM_BSS_DESC_T){
        if (EQUAL_SSID(prRoamBssDesc->aucSSID, prRoamBssDesc->ucSSIDLen,
                       prBssDesc->aucSSID, prBssDesc->ucSSIDLen)) {
            return prRoamBssDesc;
        }
    }

    return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
P_ROAM_BSS_DESC_T scanAllocateRoamBssDesc(IN P_ADAPTER_T prAdapter)
{
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prRoamFreeBSSDescList;
    P_ROAM_BSS_DESC_T prRoamBssDesc = NULL;

    ASSERT(prAdapter);
    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    prRoamFreeBSSDescList = &prScanInfo->rRoamFreeBSSDescList;

    LINK_REMOVE_HEAD(prRoamFreeBSSDescList, prRoamBssDesc, P_ROAM_BSS_DESC_T);

    if (prRoamBssDesc) {
        P_LINK_T prRoamBSSDescList;

        kalMemZero(prRoamBssDesc, sizeof(ROAM_BSS_DESC_T));

        prRoamBSSDescList = &prScanInfo->rRoamBSSDescList;

        LINK_INSERT_HEAD(prRoamBSSDescList, &prRoamBssDesc->rLinkEntry);
    }

    return prRoamBssDesc;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
void scanAddToRoamBssDesc(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBssDesc)
{
    P_ROAM_BSS_DESC_T prRoamBssDesc;

    prRoamBssDesc = scanSearchRoamBssDescBySsid(prAdapter, prBssDesc);

    if (prRoamBssDesc == NULL) {
        u32 u4RemoveTime = REMOVE_TIMEOUT_TWO_DAY;

        do {
            prRoamBssDesc = scanAllocateRoamBssDesc(prAdapter);
            if (prRoamBssDesc) {
                break;
            }
            scanRemoveRoamBssDescsByTime(prAdapter, u4RemoveTime);
            u4RemoveTime = u4RemoveTime / 2;
        } while (u4RemoveTime > 0);

        if (prRoamBssDesc != NULL) {
            COPY_SSID(prRoamBssDesc->aucSSID, prRoamBssDesc->ucSSIDLen,
                      prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
        }
    }

    if (prRoamBssDesc != NULL) {
        GET_CURRENT_SYSTIME(&prRoamBssDesc->rUpdateTime);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
void scanSearchBssDescOfRoamSsid(IN P_ADAPTER_T prAdapter)
{
#define SSID_ONLY_EXIST_ONE_AP \
    1  /* If only exist one same ssid AP, avoid unnecessary scan */

    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prBSSDescList;
    P_BSS_DESC_T prBssDesc;
    P_BSS_INFO_T prAisBssInfo;
    u32 u4SameSSIDCount = 0;

    prAisBssInfo = prAdapter->prAisBssInfo;

    /* XXX: wlan0(AP mode) + p2p0 occurs exception. */
    if (prAisBssInfo == NULL) {
        return;
    }

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
    prBSSDescList = &prScanInfo->rBSSDescList;

    if (prAisBssInfo->eConnectionState != PARAM_MEDIA_STATE_CONNECTED) {
        return;
    }

    LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T){
        if (EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
                       prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen)) {
            u4SameSSIDCount++;
            if (u4SameSSIDCount > SSID_ONLY_EXIST_ONE_AP) {
                scanAddToRoamBssDesc(prAdapter, prBssDesc);
                break;
            }
        }
    }
}

#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to
 *        given eBSSType, BSSID and Transmitter Address
 *
 * @param[in] prAdapter  Pointer to the Adapter structure.
 * @param[in] eBSSType   BSS Type of incoming Beacon/ProbeResp frame.
 * @param[in] aucBSSID   Given BSSID of Beacon/ProbeResp frame.
 * @param[in] aucSrcAddr Given source address (TA) of Beacon/ProbeResp frame.
 * @param[in] fgCheckSsid Need to check SSID or not. (for multiple SSID with
 * single BSSID cases)
 * @param[in] prSsid     Specified SSID
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T
scanSearchExistingBssDescWithSsid(IN P_ADAPTER_T prAdapter,
                                  IN ENUM_BSS_TYPE_T eBSSType, IN u8 aucBSSID[],
                                  IN u8 aucSrcAddr[], IN u8 fgCheckSsid,
                                  IN P_PARAM_SSID_T prSsid)
{
    P_SCAN_INFO_T prScanInfo;
    P_BSS_DESC_T prBssDesc, prIBSSBssDesc;
    /* CASE III */
    P_LINK_T prBSSDescList;
    P_LINK_T prFreeBSSDescList;

    ASSERT(prAdapter);
    ASSERT(aucSrcAddr);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    if (eBSSType == BSS_TYPE_P2P_DEVICE) {
        fgCheckSsid = false;
        // No break; intentional fall-through
    } else if (eBSSType == BSS_TYPE_INFRASTRUCTURE) {
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
        scanSearchBssDescOfRoamSsid(prAdapter);
        // No break; intentional fall-through
#endif
    } else if (eBSSType == BSS_TYPE_BOW_DEVICE) {
        prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter, aucBSSID,
                                                    fgCheckSsid, prSsid);
        return prBssDesc;
    } else if (eBSSType == BSS_TYPE_IBSS) {
        prIBSSBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter, aucBSSID,
                                                        fgCheckSsid, prSsid);
        prBssDesc = scanSearchBssDescByTAAndSsid(prAdapter, aucSrcAddr,
                                                 fgCheckSsid, prSsid);

        if (prBssDesc) {
            if ((!prIBSSBssDesc) || (prBssDesc == prIBSSBssDesc)) {
                return prBssDesc;
            }

            prBSSDescList = &prScanInfo->rBSSDescList;
            prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;

            LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDesc);
            LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDesc->rLinkEntry);

            return prIBSSBssDesc;
        }

        if (prIBSSBssDesc) {
            return prIBSSBssDesc;
        }
    } else {
    }

    return (P_BSS_DESC_T)NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief bypass BSS Descriptors from current list according to specific BSSID.
 *
 * @param[in] prAdapter  Pointer to the Adapter structure.
 * @param[in] aucBSSID   Given BSSID.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
u8 scanByPassRemoveBssDesc(IN P_ADAPTER_T prAdapter, IN P_BSS_DESC_T prBssDesc)
{
    P_SCAN_INFO_T prScanInfo;
    P_SCAN_PARAM_T prScanParam;
    u8 ucIndex = 0;
    u8 fgIsByPassRemove = false;

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
    prScanParam = &prScanInfo->rScanParam;

    for (ucIndex = 0; ucIndex < prScanParam->ucSSIDNum; ucIndex++) {
        if (EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
                       prScanParam->aucSpecifiedSSID[ucIndex],
                       prScanParam->ucSpecifiedSSIDLen[ucIndex])) {
            fgIsByPassRemove = true;
            DBGLOG(INIT, INFO, "scanByPassRemoveBssDesc %s | %s\n",
                   prBssDesc->aucSSID, prScanParam->aucSpecifiedSSID[ucIndex]);
            break;
        }
    }
    return fgIsByPassRemove;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Delete BSS Descriptors from current list according to given Remove
 * Policy.
 *
 * @param[in] u4RemovePolicy     Remove Policy.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanRemoveBssDescsByPolicy(IN P_ADAPTER_T prAdapter,
                                IN u32 u4RemovePolicy)
{
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prBSSDescList;
    P_LINK_T prFreeBSSDescList;
    P_BSS_DESC_T prBssDesc;

    ASSERT(prAdapter);

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
    prBSSDescList = &prScanInfo->rBSSDescList;
    prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;

    /* DBGLOG(SCN, TRACE, ("Before Remove - Number Of SCAN Result = %ld\n",
     */
    /* prBSSDescList->u4NumElem)); */

    if (u4RemovePolicy & SCN_RM_POLICY_TIMEOUT) {
        P_BSS_DESC_T prBSSDescNext;
        u32 rCurrentTime;

        GET_CURRENT_SYSTIME(&rCurrentTime);

        /* Search BSS Desc from current SCAN result list. */
        LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext, prBSSDescList,
                                 rLinkEntry, BSS_DESC_T){
            if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED) &&
                (prBssDesc->fgIsConnected || prBssDesc->fgIsConnecting)) {
                /* Don't remove the one currently we are
                 * connected. */
                continue;
            }

            if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_SPECIFIC_SSID) &&
                scanByPassRemoveBssDesc(prAdapter, prBssDesc)) {
                /* Don't remove the one currently we are looking
                 * for specifi SSID. */
                continue;
            }

            if (CHECK_FOR_TIMEOUT(
                    rCurrentTime, prBssDesc->rUpdateTime,
                    SEC_TO_SYSTIME(SCN_BSS_DESC_REMOVE_TIMEOUT_SEC))) {
                /* DBGLOG(SCN, TRACE, ("Remove TIMEOUT BSS
                 * DESC(%#x): MAC: "MACSTR", Current Time =
                 * %08lx, Update Time = %08lx\n",
                 */
                /* prBssDesc, MAC2STR(prBssDesc->aucBSSID),
                 * rCurrentTime, prBssDesc->rUpdateTime)); */

                /* Remove this BSS Desc from the BSS Desc list
                 */
                LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDesc);

                /* Return this BSS Desc to the free BSS Desc
                 * list. */
                LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDesc->rLinkEntry);
            }
        }
    }
    if (u4RemovePolicy & SCN_RM_POLICY_OLDEST_HIDDEN) {
        P_BSS_DESC_T prBssDescOldest = (P_BSS_DESC_T)NULL;

        /* Search BSS Desc from current SCAN result list. */
        LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T){
            if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED) &&
                (prBssDesc->fgIsConnected || prBssDesc->fgIsConnecting)) {
                /* Don't remove the one currently we are
                 * connected. */
                continue;
            }
            if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_SPECIFIC_SSID) &&
                scanByPassRemoveBssDesc(prAdapter, prBssDesc)) {
                /* Don't remove the one currently we are looking
                 * for specifi SSID. */
                continue;
            }

            if (!prBssDesc->fgIsHiddenSSID) {
                continue;
            }

            if (!prBssDescOldest) {  /* 1st element */
                prBssDescOldest = prBssDesc;
                continue;
            }

            if (TIME_BEFORE(prBssDesc->rUpdateTime,
                            prBssDescOldest->rUpdateTime)) {
                prBssDescOldest = prBssDesc;
            }
        }

        if (prBssDescOldest) {
            /* DBGLOG(SCN, TRACE,
             * ("Remove OLDEST HIDDEN BSS DESC(%#x): MAC: "MACSTR",
             * Update Time = %08lx\n",
             */
            /* prBssDescOldest, MAC2STR(prBssDescOldest->aucBSSID),
             * prBssDescOldest->rUpdateTime)); */

            /* Remove this BSS Desc from the BSS Desc list */
            LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDescOldest);

            /* Return this BSS Desc to the free BSS Desc list. */
            LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDescOldest->rLinkEntry);
        }
    }
    if (u4RemovePolicy & SCN_RM_POLICY_SMART_WEAKEST) {
        P_BSS_DESC_T prBssDescWeakest = (P_BSS_DESC_T)NULL;
        P_BSS_DESC_T prBssDescWeakestSameSSID = (P_BSS_DESC_T)NULL;
        u32 u4SameSSIDCount = 0;

        /* Search BSS Desc from current SCAN result list. */
        LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T){
            if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED) &&
                (prBssDesc->fgIsConnected || prBssDesc->fgIsConnecting)) {
                /* Don't remove the one currently we are
                 * connected. */
                continue;
            }
            if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_SPECIFIC_SSID) &&
                scanByPassRemoveBssDesc(prAdapter, prBssDesc)) {
                /* Don't remove the one currently we are looking
                 * for specifi SSID. */
                continue;
            }

            if ((!prBssDesc->fgIsHiddenSSID) &&
                (EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
                            prConnSettings->aucSSID,
                            prConnSettings->ucSSIDLen))) {
                u4SameSSIDCount++;

                if (!prBssDescWeakestSameSSID) {
                    prBssDescWeakestSameSSID = prBssDesc;
                } else if (prBssDesc->ucRCPI <
                           prBssDescWeakestSameSSID->ucRCPI) {
                    prBssDescWeakestSameSSID = prBssDesc;
                }
            }

            if (!prBssDescWeakest) {  /* 1st element */
                prBssDescWeakest = prBssDesc;
                continue;
            }

            if (prBssDesc->ucRCPI < prBssDescWeakest->ucRCPI) {
                prBssDescWeakest = prBssDesc;
            }
        }

        if ((u4SameSSIDCount >= SCN_BSS_DESC_SAME_SSID_THRESHOLD) &&
            (prBssDescWeakestSameSSID)) {
            prBssDescWeakest = prBssDescWeakestSameSSID;
        }

        if (prBssDescWeakest) {
            /* DBGLOG(SCN, TRACE, ("Remove WEAKEST BSS DESC(%#x):
             * MAC: "MACSTR", Update Time = %08lx\n", */
            /* prBssDescOldest, MAC2STR(prBssDescOldest->aucBSSID),
             * prBssDescOldest->rUpdateTime)); */

            /* Remove this BSS Desc from the BSS Desc list */
            LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDescWeakest);

            /* Return this BSS Desc to the free BSS Desc list. */
            LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDescWeakest->rLinkEntry);
        }
    }
    if (u4RemovePolicy & SCN_RM_POLICY_ENTIRE) {
        P_BSS_DESC_T prBSSDescNext;

        LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext, prBSSDescList,
                                 rLinkEntry, BSS_DESC_T){
            if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED) &&
                (prBssDesc->fgIsConnected || prBssDesc->fgIsConnecting)) {
                /* Don't remove the one currently we are
                 * connected. */
                continue;
            }
            if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_SPECIFIC_SSID) &&
                scanByPassRemoveBssDesc(prAdapter, prBssDesc)) {
                /* Don't remove the one currently we are looking
                 * for specifi SSID. */
                continue;
            }

            /* Remove this BSS Desc from the BSS Desc list */
            LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDesc);

            /* Return this BSS Desc to the free BSS Desc list. */
            LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDesc->rLinkEntry);
        }
    }

    return;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Delete BSS Descriptors from current list according to given BSSID.
 *
 * @param[in] prAdapter  Pointer to the Adapter structure.
 * @param[in] aucBSSID   Given BSSID.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanRemoveBssDescByBssid(IN P_ADAPTER_T prAdapter, IN u8 aucBSSID[])
{
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prBSSDescList;
    P_LINK_T prFreeBSSDescList;
    P_BSS_DESC_T prBssDesc = (P_BSS_DESC_T)NULL;
    P_BSS_DESC_T prBSSDescNext;

    ASSERT(prAdapter);
    ASSERT(aucBSSID);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
    prBSSDescList = &prScanInfo->rBSSDescList;
    prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;

    /* Check if such BSS Descriptor exists in a valid list */
    LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext, prBSSDescList,
                             rLinkEntry, BSS_DESC_T){
        if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID)) {
            /* Remove this BSS Desc from the BSS Desc list */
            LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDesc);

            /* Return this BSS Desc to the free BSS Desc list. */
            LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDesc->rLinkEntry);

            /* BSSID is not unique, so need to traverse whols
             * link-list */
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Delete BSS Descriptors from current list according to given band
 * configuration
 *
 * @param[in] prAdapter  Pointer to the Adapter structure.
 * @param[in] eBand      Given band
 * @param[in] ucBssIndex     AIS - Remove IBSS/Infrastructure BSS
 *                           BOW - Remove BOW BSS
 *                           P2P - Remove P2P BSS
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanRemoveBssDescByBandAndNetwork(IN P_ADAPTER_T prAdapter,
                                       IN ENUM_BAND_T eBand, IN u8 ucBssIndex)
{
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prBSSDescList;
    P_LINK_T prFreeBSSDescList;
    P_BSS_DESC_T prBssDesc = (P_BSS_DESC_T)NULL;
    P_BSS_DESC_T prBSSDescNext;
    u8 fgToRemove;

    ASSERT(prAdapter);
    ASSERT(eBand <= BAND_NUM);
    ASSERT(ucBssIndex <= MAX_BSS_INDEX);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
    prBSSDescList = &prScanInfo->rBSSDescList;
    prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;

    if (eBand == BAND_NULL) {
        return;  /* no need to do anything, keep all scan result */
    }
    /* Check if such BSS Descriptor exists in a valid list */
    LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext, prBSSDescList,
                             rLinkEntry, BSS_DESC_T){
        fgToRemove = false;

        if (prBssDesc->eBand == eBand) {
            switch (
                GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType) {
            case NETWORK_TYPE_AIS:
                if ((prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE) ||
                    (prBssDesc->eBSSType == BSS_TYPE_IBSS)) {
                    fgToRemove = true;
                }
                break;

            case NETWORK_TYPE_P2P:
                if (prBssDesc->eBSSType == BSS_TYPE_P2P_DEVICE) {
                    fgToRemove = true;
                }
                break;

            case NETWORK_TYPE_BOW:
                if (prBssDesc->eBSSType == BSS_TYPE_BOW_DEVICE) {
                    fgToRemove = true;
                }
                break;

            default:
                ASSERT(0);
                break;
            }
        }

        if (fgToRemove == true) {
            /* Remove this BSS Desc from the BSS Desc list */
            LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDesc);

            /* Return this BSS Desc to the free BSS Desc list. */
            LINK_INSERT_TAIL(prFreeBSSDescList, &prBssDesc->rLinkEntry);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Clear the CONNECTION FLAG of a specified BSS Descriptor.
 *
 * @param[in] aucBSSID   Given BSSID.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanRemoveConnFlagOfBssDescByBssid(IN P_ADAPTER_T prAdapter,
                                        IN u8 aucBSSID[])
{
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prBSSDescList;
    P_BSS_DESC_T prBssDesc = (P_BSS_DESC_T)NULL;

    ASSERT(prAdapter);
    ASSERT(aucBSSID);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
    prBSSDescList = &prScanInfo->rBSSDescList;

    /* Search BSS Desc from current SCAN result list. */
    LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T){
        if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID)) {
            prBssDesc->fgIsConnected = false;
            prBssDesc->fgIsConnecting = false;

            /* BSSID is not unique, so need to traverse whols
             * link-list */
        }
    }

    return;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Allocate new BSS_DESC_T
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 *
 * @return   Pointer to BSS Descriptor, if has free space. NULL, if has no
 * space.
 */
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T scanAllocateBssDesc(IN P_ADAPTER_T prAdapter)
{
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prFreeBSSDescList;
    P_BSS_DESC_T prBssDesc;

    ASSERT(prAdapter);
    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;

    LINK_REMOVE_HEAD(prFreeBSSDescList, prBssDesc, P_BSS_DESC_T);

    if (prBssDesc) {
        P_LINK_T prBSSDescList;

        kalMemZero(prBssDesc, sizeof(BSS_DESC_T));

#if CFG_ENABLE_WIFI_DIRECT
        LINK_INITIALIZE(&(prBssDesc->rP2pDeviceList));
        prBssDesc->fgIsP2PPresent = false;
#endif

        prBSSDescList = &prScanInfo->rBSSDescList;

        /* NOTE(Kevin): In current design, this new empty BSS_DESC_T
         * will be inserted to BSSDescList immediately.
         */
        LINK_INSERT_TAIL(prBSSDescList, &prBssDesc->rLinkEntry);
    }

    return prBssDesc;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This API parses Beacon/ProbeResp frame and insert extracted BSS_DESC_T
 *        with IEs into prAdapter->rWifiVar.rScanInfo.aucScanBuffer
 *
 * @param[in] prAdapter      Pointer to the Adapter structure.
 * @param[in] prSwRfb        Pointer to the receiving frame buffer.
 *
 * @return   Pointer to BSS Descriptor
 *           NULL if the Beacon/ProbeResp frame is invalid
 */
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T scanAddToBssDesc(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
    P_BSS_DESC_T prBssDesc = NULL;
    u16 u2CapInfo;
    ENUM_BSS_TYPE_T eBSSType = BSS_TYPE_INFRASTRUCTURE;

    u8 *pucIE;
    u16 u2IELength;
    u16 u2Offset = 0;

    P_WLAN_BEACON_FRAME_T prWlanBeaconFrame = (P_WLAN_BEACON_FRAME_T)NULL;
    P_IE_SSID_T prIeSsid = (P_IE_SSID_T)NULL;
    P_IE_SUPPORTED_RATE_IOT_T prIeSupportedRate =
        (P_IE_SUPPORTED_RATE_IOT_T)NULL;
    P_IE_EXT_SUPPORTED_RATE_T prIeExtSupportedRate =
        (P_IE_EXT_SUPPORTED_RATE_T)NULL;
    u8 ucHwChannelNum = 0;
    u8 ucIeDsChannelNum = 0;
    u8 ucIeHtChannelNum = 0;
    u8 fgIsValidSsid = false, fgEscape = false, fgIsCopy = false;
    PARAM_SSID_T rSsid;
    u64 u8Timestamp;
    u8 fgIsNewBssDesc = false;

    u32 i;
    u8 ucSSIDChar;
    /* u8 * pucDumpIE; */

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prWlanBeaconFrame = (P_WLAN_BEACON_FRAME_T)prSwRfb->pvHeader;

    WLAN_GET_FIELD_16(&prWlanBeaconFrame->u2CapInfo, &u2CapInfo);
    WLAN_GET_FIELD_64(&prWlanBeaconFrame->au4Timestamp[0], &u8Timestamp);

    /* decide BSS type */
    switch (u2CapInfo & CAP_INFO_BSS_TYPE) {
    case CAP_INFO_ESS:
        /* It can also be Group Owner of P2P Group. */
        eBSSType = BSS_TYPE_INFRASTRUCTURE;
        break;

    case CAP_INFO_IBSS:
        eBSSType = BSS_TYPE_IBSS;
        break;

    case 0:
        /* The P2P Device shall set the ESS bit of the Capabilities
         * field in the Probe Response fame to 0 and IBSS bit to 0.
         * (3.1.2.1.1)
         */
        eBSSType = BSS_TYPE_P2P_DEVICE;
        break;

    default:
        return NULL;
    }

    /* 4 <1.1> Pre-parse SSID IE */
    pucIE = prWlanBeaconFrame->aucInfoElem;
    u2IELength = (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
                 (u16)OFFSET_OF(WLAN_BEACON_FRAME_BODY_T, aucInfoElem[0]);

    IE_FOR_EACH(pucIE, u2IELength, u2Offset){
        switch (IE_ID(pucIE)) {
        case ELEM_ID_SSID:
            if (IE_LEN(pucIE) <= ELEM_MAX_LEN_SSID) {
                ucSSIDChar = '\0';

                /* D-Link DWL-900AP+ */
                if (IE_LEN(pucIE) == 0) {
                    fgIsValidSsid = false;
                }
                /* Cisco AP1230A - (IE_LEN(pucIE) == 1) &&
                 * (SSID_IE(pucIE)->aucSSID[0] == '\0') */
                /* Linksys WRK54G/WL520g - (IE_LEN(pucIE) == n)
                 * && (SSID_IE(pucIE)->aucSSID[0~(n-1)] == '\0')
                 */
                else {
                    for (i = 0; i < IE_LEN(pucIE); i++)
                        ucSSIDChar |= SSID_IE(pucIE)->aucSSID[i];

                    if (ucSSIDChar) {
                        fgIsValidSsid = true;
                    }
                }

                /* Update SSID to BSS Descriptor only if SSID is
                 * not hidden. */
                if (fgIsValidSsid == true) {
                    COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
                              SSID_IE(pucIE)->aucSSID,
                              SSID_IE(pucIE)->ucLength);
                }
            }
            fgEscape = true;
            break;

        default:
            break;
        }

        if (fgEscape == true) {
            break;
        }
    }

    /* 4 <1.2> Replace existing BSS_DESC_T or allocate a new one */
    prBssDesc = scanSearchExistingBssDescWithSsid(
        prAdapter, eBSSType, (u8 *)prWlanBeaconFrame->aucBSSID,
        (u8 *)prWlanBeaconFrame->aucSrcAddr, fgIsValidSsid,
        fgIsValidSsid == true ? &rSsid : NULL);

    if (prBssDesc == (P_BSS_DESC_T)NULL) {
        fgIsNewBssDesc = true;

        do {
            /* 4 <1.2.1> First trial of allocation */
            prBssDesc = scanAllocateBssDesc(prAdapter);
            if (prBssDesc) {
                break;
            }
            /* 4 <1.2.2> Hidden is useless, remove the oldest hidden
             * ssid. (for passive scan) */
            scanRemoveBssDescsByPolicy(
                prAdapter,
                (SCN_RM_POLICY_EXCLUDE_CONNECTED | SCN_RM_POLICY_OLDEST_HIDDEN |
                 SCN_RM_POLICY_TIMEOUT | SCN_RM_POLICY_EXCLUDE_SPECIFIC_SSID));

            /* 4 <1.2.3> Second tail of allocation */
            prBssDesc = scanAllocateBssDesc(prAdapter);
            if (prBssDesc) {
                break;
            }
            /* 4 <1.2.4> Remove the weakest one */
            /* If there are more than half of BSS which has the same
             * ssid as connection setting, remove the weakest one
             * from them. Else remove the weakest one.
             */
            scanRemoveBssDescsByPolicy(prAdapter,
                                       (SCN_RM_POLICY_EXCLUDE_CONNECTED |
                                        SCN_RM_POLICY_SMART_WEAKEST |
                                        SCN_RM_POLICY_EXCLUDE_SPECIFIC_SSID));

            /* 4 <1.2.5> reallocation */
            prBssDesc = scanAllocateBssDesc(prAdapter);
            if (prBssDesc) {
                break;
            }

            // DBGLOG(SCN, WARN, "Allocate Bss Desc failed\n");

            /* 4 <1.2.6> no space, should not happen */
            /* ASSERT(0); // still no space available ? */
            return NULL;
        } while (false);
    } else {
        u32 rCurrentTime;

        /* WCXRP00000091 */
        /* if the received strength is much weaker than the original
         * one, */
        /* ignore it due to it might be received on the folding
         * frequency */

        GET_CURRENT_SYSTIME(&rCurrentTime);

        ASSERT(prSwRfb->prRxStatusGroup3);

        if (prBssDesc->eBSSType != eBSSType) {
            prBssDesc->eBSSType = eBSSType;
        } else if (HAL_RX_STATUS_GET_CHNL_NUM(prSwRfb->prRxStatus) !=
                   prBssDesc->ucChannelNum &&
                   prBssDesc->ucRCPI >
                   nicRxGetRcpiValueFromRxv(RCPI_MODE_WF0, prSwRfb)) {
            /* for signal strength is too much weaker and previous
             * beacon is not stale */
            ASSERT(prSwRfb->prRxStatusGroup3);
            if ((prBssDesc->ucRCPI -
                 nicRxGetRcpiValueFromRxv(RCPI_MODE_WF0, prSwRfb)) >=
                REPLICATED_BEACON_STRENGTH_THRESHOLD &&
                rCurrentTime - prBssDesc->rUpdateTime <=
                REPLICATED_BEACON_FRESH_PERIOD) {
                return prBssDesc;
            }
            /* for received beacons too close in time domain */
            else if (rCurrentTime - prBssDesc->rUpdateTime <=
                     REPLICATED_BEACON_TIME_THRESHOLD) {
                return prBssDesc;
            }
        }

        /* if Timestamp has been reset, re-generate BSS DESC 'cause AP
         * should have reset itself */
        if (prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE &&
            u8Timestamp < prBssDesc->u8TimeStamp.QuadPart) {
            u8 fgIsConnected, fgIsConnecting;

            /* set flag for indicating this is a new BSS-DESC */
            fgIsNewBssDesc = true;

            /* backup 2 flags for APs which reset timestamp
             * unexpectedly */
            fgIsConnected = prBssDesc->fgIsConnected;
            fgIsConnecting = prBssDesc->fgIsConnecting;
            scanRemoveBssDescByBssid(prAdapter, prBssDesc->aucBSSID);

            prBssDesc = scanAllocateBssDesc(prAdapter);
            if (!prBssDesc) {
                // DBGLOG(SCN, WARN, "Allocate Bss Desc failed\n");
                return NULL;
            }

            /* restore */
            prBssDesc->fgIsConnected = fgIsConnected;
            prBssDesc->fgIsConnecting = fgIsConnecting;
        }
    }

    /* 2021/04/18 frog: Only update IE when in scan state. */
    /* Driver would still RX BCN/Probe RSP under other state. */
    /* It would cause driver cache some scan result till next scan done. */
    if (scnFsmIsScanning(prAdapter)) {
        /* 2018/04/17 frog: always update IE is not a good choice. */
        /* Because of not considering hidden BSS.                  */
        /* Hidden BSS Beacon v.s. hidden BSS probe response.       */
        if ((prBssDesc->u2RawLength == 0) || (fgIsValidSsid)) {
            prBssDesc->u2RawLength = prSwRfb->u2PacketLen;
            if (prBssDesc->u2RawLength > CFG_RAW_BUFFER_SIZE) {
                prBssDesc->u2RawLength = CFG_RAW_BUFFER_SIZE;
            }
            kalMemCopy(prBssDesc->aucRawBuf, prWlanBeaconFrame,
                       prBssDesc->u2RawLength);
            fgIsCopy = true;
        }
    } else {
        prBssDesc->u2RawLength = 0;
    }

    /* NOTE: Keep consistency of Scan Record during JOIN process */
    if (fgIsNewBssDesc == false && prBssDesc->fgIsConnecting) {
        return prBssDesc;
    }

    /* 4 <2> Get information from Fixed Fields */
    prBssDesc->eBSSType = eBSSType;  /* Update the latest BSS type
                                      * information. */

    COPY_MAC_ADDR(prBssDesc->aucSrcAddr, prWlanBeaconFrame->aucSrcAddr);

    COPY_MAC_ADDR(prBssDesc->aucBSSID, prWlanBeaconFrame->aucBSSID);

    prBssDesc->u8TimeStamp.QuadPart = u8Timestamp;

    WLAN_GET_FIELD_16(&prWlanBeaconFrame->u2BeaconInterval,
                      &prBssDesc->u2BeaconInterval);

    prBssDesc->u2CapInfo = u2CapInfo;

    /* 4 <2.1> Retrieve IEs for later parsing */
    u2IELength = (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
                 (u16)OFFSET_OF(WLAN_BEACON_FRAME_BODY_T, aucInfoElem[0]);

    if (fgIsCopy) {
        prBssDesc->u2IELength = u2IELength;

        if (prBssDesc->u2IELength > CFG_IE_BUFFER_SIZE) {
            prBssDesc->u2IELength = CFG_IE_BUFFER_SIZE;
            prBssDesc->fgIsIEOverflow = true;
        } else {
            prBssDesc->fgIsIEOverflow = false;
        }

        kalMemCopy(prBssDesc->aucIEBuf, prWlanBeaconFrame->aucInfoElem,
                   prBssDesc->u2IELength);
    }

    /* 4 <2.2> reset prBssDesc variables in case that AP has been
     * reconfigured */
    prBssDesc->fgIsERPPresent = false;
    prBssDesc->fgIsHTPresent = false;
    prBssDesc->fgIsVHTPresent = false;
    prBssDesc->eSco = CHNL_EXT_SCN;
    prBssDesc->fgIEWAPI = false;
    prBssDesc->fgIERSN = false;
    prBssDesc->fgIEWPA = false;
    prBssDesc->eChannelWidth = CW_20_40MHZ;  /*Reset VHT OP IE relative
                                              * settings */
    prBssDesc->ucCenterFreqS1 = 0;
    prBssDesc->ucCenterFreqS2 = 0;

    /* 4 <3.1> Full IE parsing on SW_RFB_T */
    pucIE = prWlanBeaconFrame->aucInfoElem;
    /* pucDumpIE = pucIE; */

    IE_FOR_EACH(pucIE, u2IELength, u2Offset){
        switch (IE_ID(pucIE)) {
        case ELEM_ID_SSID:
            if ((!prIeSsid) &&  /* NOTE(Kevin): for Atheros IOT #1 */
                (IE_LEN(pucIE) <= ELEM_MAX_LEN_SSID)) {
                u8 fgIsHiddenSSID = false;

                ucSSIDChar = '\0';

                prIeSsid = (P_IE_SSID_T)pucIE;

                /* D-Link DWL-900AP+ */
                if (IE_LEN(pucIE) == 0) {
                    fgIsHiddenSSID = true;
                }
                /* Cisco AP1230A - (IE_LEN(pucIE) == 1) &&
                 * (SSID_IE(pucIE)->aucSSID[0] == '\0') */
                /* Linksys WRK54G/WL520g - (IE_LEN(pucIE) == n)
                 * && (SSID_IE(pucIE)->aucSSID[0~(n-1)] == '\0')
                 * =
                 */
                else {
                    for (i = 0; i < IE_LEN(pucIE); i++)
                        ucSSIDChar |= SSID_IE(pucIE)->aucSSID[i];

                    if (!ucSSIDChar) {
                        fgIsHiddenSSID = true;
                    }
                }

                /* Update SSID to BSS Descriptor only if SSID is
                 * not hidden. */
                if (!fgIsHiddenSSID) {
                    COPY_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
                              SSID_IE(pucIE)->aucSSID,
                              SSID_IE(pucIE)->ucLength);
                }
            }
            break;

        case ELEM_ID_SUP_RATES:
            /* NOTE(Kevin): Buffalo WHR-G54S's supported rate set IE
             * exceed 8. IE_LEN(pucIE) == 12, "1(B), 2(B), 5.5(B),
             * 6(B), 9(B), 11(B), 12(B), 18(B), 24(B), 36(B), 48(B),
             * 54(B)"
             */
            /* TP-LINK will set extra and incorrect ie with
             * ELEM_ID_SUP_RATES */
            if ((!prIeSupportedRate) && (IE_LEN(pucIE) <= RATE_NUM_SW)) {
                prIeSupportedRate = SUP_RATES_IOT_IE(pucIE);
            }
            break;

        case ELEM_ID_DS_PARAM_SET:
            if (IE_LEN(pucIE) == ELEM_MAX_LEN_DS_PARAMETER_SET) {
                ucIeDsChannelNum = DS_PARAM_IE(pucIE)->ucCurrChnl;
            }
            break;

        case ELEM_ID_TIM:
            if (IE_LEN(pucIE) <= ELEM_MAX_LEN_TIM) {
                prBssDesc->ucDTIMPeriod = TIM_IE(pucIE)->ucDTIMPeriod;
            }
            break;

        case ELEM_ID_IBSS_PARAM_SET:
            if (IE_LEN(pucIE) == ELEM_MAX_LEN_IBSS_PARAMETER_SET) {
                prBssDesc->u2ATIMWindow = IBSS_PARAM_IE(pucIE)->u2ATIMWindow;
            }
            break;

        case ELEM_ID_ERP_INFO:
            if (IE_LEN(pucIE) == ELEM_MAX_LEN_ERP) {
                prBssDesc->fgIsERPPresent = true;
            }
            break;

        case ELEM_ID_EXTENDED_SUP_RATES:
            if (!prIeExtSupportedRate) {
                prIeExtSupportedRate = EXT_SUP_RATES_IE(pucIE);
            }
            break;

        case ELEM_ID_RSN:
            if (rsnParseRsnIE(prAdapter, RSN_IE(pucIE), &prBssDesc->rRSNInfo)) {
                prBssDesc->fgIERSN = true;
                prBssDesc->u2RsnCap = prBssDesc->rRSNInfo.u2RsnCap;
                if (prAdapter->rWifiVar.rConnSettings.eAuthMode ==
                    AUTH_MODE_WPA2) {
                    rsnCheckPmkidCache(prAdapter, prBssDesc);
                }
            }
            break;

        case ELEM_ID_HT_CAP:
            prBssDesc->fgIsHTPresent = true;
            break;

        case ELEM_ID_HT_OP:
            if (IE_LEN(pucIE) != (sizeof(IE_HT_OP_T) - 2)) {
                break;
            }

            if ((((P_IE_HT_OP_T)pucIE)->ucInfo1 & HT_OP_INFO1_SCO) !=
                CHNL_EXT_RES) {
                prBssDesc->eSco =
                    (ENUM_CHNL_EXT_T)(((P_IE_HT_OP_T)pucIE)->ucInfo1 &
                                      HT_OP_INFO1_SCO);
            }
            ucIeHtChannelNum = ((P_IE_HT_OP_T)pucIE)->ucPrimaryChannel;

            break;

        case ELEM_ID_VHT_CAP:
            prBssDesc->fgIsVHTPresent = true;
#if CFG_SUPPORT_BFEE
            prBssDesc->ucVhtCapNumSoundingDimensions =
                ((((P_IE_VHT_CAP_T)pucIE)->u4VhtCapInfo) &
                 VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS) >>
                VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS_OFFSET;
#endif
            break;

        case ELEM_ID_VHT_OP:
            if (IE_LEN(pucIE) != (sizeof(IE_VHT_OP_T) - 2)) {
                break;
            }

            prBssDesc->eChannelWidth =
                (ENUM_CHANNEL_WIDTH_T)(((P_IE_VHT_OP_T)pucIE)
                                       ->ucVhtOperation[0]);
            prBssDesc->ucCenterFreqS1 =
                (ENUM_CHANNEL_WIDTH_T)(((P_IE_VHT_OP_T)pucIE)
                                       ->ucVhtOperation[1]);
            prBssDesc->ucCenterFreqS2 =
                (ENUM_CHANNEL_WIDTH_T)(((P_IE_VHT_OP_T)pucIE)
                                       ->ucVhtOperation[2]);

            /*add IEEE BW160 patch*/
            rlmModifyVhtBwPara(&prBssDesc->ucCenterFreqS1,
                               &prBssDesc->ucCenterFreqS2,
                               (u8 *)&prBssDesc->eChannelWidth);

            break;

        case ELEM_ID_VENDOR:  /* ELEM_ID_P2P, ELEM_ID_WMM */
        {
            u8 ucOuiType;
            u16 u2SubTypeVersion;

            if (rsnParseCheckForWFAInfoElem(prAdapter, pucIE, &ucOuiType,
                                            &u2SubTypeVersion)) {
                if ((ucOuiType == VENDOR_OUI_TYPE_WPA) &&
                    (u2SubTypeVersion == VERSION_WPA) &&
                    (rsnParseWpaIE(prAdapter, WPA_IE(pucIE),
                                   &prBssDesc->rWPAInfo))) {
                    prBssDesc->fgIEWPA = true;
                }
            }
#if CFG_ENABLE_WIFI_DIRECT
            if (prAdapter->fgIsP2PRegistered) {
                if ((p2pFuncParseCheckForP2PInfoElem(prAdapter, pucIE,
                                                     &ucOuiType)) &&
                    (ucOuiType == VENDOR_OUI_TYPE_P2P)) {
                    prBssDesc->fgIsP2PPresent = true;
                }
            }
#endif
        } break;

#if CFG_SUPPORT_802_11K
        case ELEM_ID_RRM_ENABLED_CAP:
            /* RRM Capability IE is always in length 5 bytes */
            kalMemZero(prBssDesc->aucRrmCap, sizeof(prBssDesc->aucRrmCap));
            kalMemCopy(prBssDesc->aucRrmCap, pucIE + 2,
                       sizeof(prBssDesc->aucRrmCap));
            break;
#endif
            /* no default */
        }
    }

    /* 4 <3.2> Save information from IEs - SSID */
    /* Update Flag of Hidden SSID for used in SEARCH STATE. */

    /* NOTE(Kevin): in current driver, the ucSSIDLen == 0 represent
     * all cases of hidden SSID.
     * If the fgIsHiddenSSID == true, it means we didn't get the ProbeResp
     * with valid SSID.
     */
    if (prBssDesc->ucSSIDLen == 0) {
        prBssDesc->fgIsHiddenSSID = true;
    } else {
        prBssDesc->fgIsHiddenSSID = false;
    }

    /* 4 <3.3> Check rate information in related IEs. */
    if (prIeSupportedRate || prIeExtSupportedRate) {
        rateGetRateSetFromIEs(prIeSupportedRate, prIeExtSupportedRate,
                              &prBssDesc->u2OperationalRateSet,
                              &prBssDesc->u2BSSBasicRateSet,
                              &prBssDesc->fgIsUnknownBssBasicRate);
    }

    /* 4 <4> Update information from HIF RX Header */
    {
        P_HW_MAC_RX_DESC_T prRxStatus;
        u8 ucRxRCPI;

        prRxStatus = prSwRfb->prRxStatus;
        ASSERT(prRxStatus);

        /* 4 <4.1> Get TSF comparison result */
        prBssDesc->fgIsLargerTSF = HAL_RX_STATUS_GET_TCL(prRxStatus);

        /* 4 <4.2> Get Band information */
        prBssDesc->eBand = HAL_RX_STATUS_GET_RF_BAND(prRxStatus);

        /* 4 <4.2> Get channel and RCPI information */
        ucHwChannelNum = HAL_RX_STATUS_GET_CHNL_NUM(prRxStatus);

        ASSERT(prSwRfb->prRxStatusGroup3);
        ucRxRCPI = nicRxGetRcpiValueFromRxv(RCPI_MODE_WF0, prSwRfb);
        if (prBssDesc->eBand == BAND_2G4) {
            /* Update RCPI if in right channel */

            if (ucIeDsChannelNum >= 1 && ucIeDsChannelNum <= 14) {
                /* Receive Beacon/ProbeResp frame from adjacent
                 * channel. */
                if ((ucIeDsChannelNum == ucHwChannelNum) ||
                    (ucRxRCPI > prBssDesc->ucRCPI)) {
                    prBssDesc->ucRCPI = ucRxRCPI;
                }
                /* trust channel information brought by IE */
                prBssDesc->ucChannelNum = ucIeDsChannelNum;
            } else if (ucIeHtChannelNum >= 1 && ucIeHtChannelNum <= 14) {
                /* Receive Beacon/ProbeResp frame from adjacent
                 * channel. */
                if ((ucIeHtChannelNum == ucHwChannelNum) ||
                    (ucRxRCPI > prBssDesc->ucRCPI)) {
                    prBssDesc->ucRCPI = ucRxRCPI;
                }
                /* trust channel information brought by IE */
                prBssDesc->ucChannelNum = ucIeHtChannelNum;
            } else {
                prBssDesc->ucRCPI = ucRxRCPI;

                prBssDesc->ucChannelNum = ucHwChannelNum;
            }
        }
        /* 5G Band */
        else {
            if (ucIeHtChannelNum >= 1 && ucIeHtChannelNum < 200) {
                /* Receive Beacon/ProbeResp frame from adjacent
                 * channel. */
                if ((ucIeHtChannelNum == ucHwChannelNum) ||
                    (ucRxRCPI > prBssDesc->ucRCPI)) {
                    prBssDesc->ucRCPI = ucRxRCPI;
                }
                /* trust channel information brought by IE */
                prBssDesc->ucChannelNum = ucIeHtChannelNum;
            } else {
                /* Always update RCPI */
                prBssDesc->ucRCPI = ucRxRCPI;

                prBssDesc->ucChannelNum = ucHwChannelNum;
            }
        }
    }

    /* 4 <5> Check IE information corret or not */
    if (!rlmDomainIsValidRfSetting(
            prAdapter, prBssDesc->eBand, prBssDesc->ucChannelNum,
            prBssDesc->eSco, prBssDesc->eChannelWidth,
            prBssDesc->ucCenterFreqS1, prBssDesc->ucCenterFreqS2)) {
        /* Dump IE Inforamtion */
        /* DBGLOG(RLM, WARN, "ScanAddToBssDesc IE Information\n"); */
        /* DBGLOG(RLM, WARN, "IE Length = %d\n", u2IELength); */
        /* DBGLOG_MEM8(RLM, WARN, pucDumpIE, u2IELength); */

        /* Error Handling for Non-predicted IE - Fixed to set 20MHz */
        prBssDesc->eChannelWidth = CW_20_40MHZ;
        prBssDesc->ucCenterFreqS1 = 0;
        prBssDesc->ucCenterFreqS2 = 0;
        prBssDesc->eSco = CHNL_EXT_SCN;
    }

    /* 4 <6> PHY type setting */
    prBssDesc->ucPhyTypeSet = 0;

    if (prBssDesc->eBand == BAND_2G4) {
        /* check if support 11n */
        if (prBssDesc->fgIsHTPresent) {
            prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_HT;
        }

        /* if not 11n only */
        if (!(prBssDesc->u2BSSBasicRateSet & RATE_SET_BIT_HT_PHY)) {
            /* check if support 11g */
            if ((prBssDesc->u2OperationalRateSet & RATE_SET_OFDM) ||
                prBssDesc->fgIsERPPresent) {
                prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_ERP;
            }

            /* if not 11g only */
            if (!(prBssDesc->u2BSSBasicRateSet & RATE_SET_OFDM)) {
                /* check if support 11b */
                if ((prBssDesc->u2OperationalRateSet & RATE_SET_HR_DSSS)) {
                    prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_HR_DSSS;
                }
            }
        }
    } else {  /* (BAND_5G == prBssDesc->eBande) */
        /* check if support 11n */
        if (prBssDesc->fgIsVHTPresent) {
            prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;
        }

        if (prBssDesc->fgIsHTPresent) {
            prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_HT;
        }

        /* if not 11n only */
        if (!(prBssDesc->u2BSSBasicRateSet & RATE_SET_BIT_HT_PHY)) {
            /* Support 11a definitely */
            prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_OFDM;

            /* ASSERT(!(prBssDesc->u2OperationalRateSet &
             * RATE_SET_HR_DSSS)); */
        }
    }

    /* 4 <7> Update BSS_DESC_T's Last Update TimeStamp. */
    GET_CURRENT_SYSTIME(&prBssDesc->rUpdateTime);

    return prBssDesc;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Convert the Beacon or ProbeResp Frame in SW_RFB_T to scan result for
 * query
 *
 * @param[in] prSwRfb            Pointer to the receiving SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS   It is a valid Scan Result and been sent to the
 * host.
 * @retval WLAN_STATUS_FAILURE   It is not a valid Scan Result.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS scanAddScanResult(IN P_ADAPTER_T prAdapter,
                              IN P_BSS_DESC_T prBssDesc, IN P_SW_RFB_T prSwRfb)
{
    P_SCAN_INFO_T prScanInfo;
    u8 aucRatesEx[PARAM_MAX_LEN_RATES_EX];
    P_WLAN_BEACON_FRAME_T prWlanBeaconFrame;
    PARAM_MAC_ADDRESS rMacAddr;
    PARAM_SSID_T rSsid;
    ENUM_PARAM_NETWORK_TYPE_T eNetworkType;
    PARAM_802_11_CONFIG_T rConfiguration;
    ENUM_PARAM_OP_MODE_T eOpMode;
    u8 ucRateLen = 0;
    u32 i;

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    if (prBssDesc->eBand == BAND_2G4) {
        if ((prBssDesc->u2OperationalRateSet & RATE_SET_OFDM) ||
            prBssDesc->fgIsERPPresent) {
            eNetworkType = PARAM_NETWORK_TYPE_OFDM24;
        } else {
            eNetworkType = PARAM_NETWORK_TYPE_DS;
        }
    } else {
        ASSERT(prBssDesc->eBand == BAND_5G);
        eNetworkType = PARAM_NETWORK_TYPE_OFDM5;
    }

    if (prBssDesc->eBSSType == BSS_TYPE_P2P_DEVICE) {
        /* NOTE(Kevin): Not supported by WZC(TBD) */
        return WLAN_STATUS_FAILURE;
    }

    prWlanBeaconFrame = (P_WLAN_BEACON_FRAME_T)prSwRfb->pvHeader;
    COPY_MAC_ADDR(rMacAddr, prWlanBeaconFrame->aucBSSID);
    COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen, prBssDesc->aucSSID,
              prBssDesc->ucSSIDLen);

    rConfiguration.u4Length = sizeof(PARAM_802_11_CONFIG_T);
    rConfiguration.u4BeaconPeriod = (u32)prWlanBeaconFrame->u2BeaconInterval;
    rConfiguration.u4ATIMWindow = prBssDesc->u2ATIMWindow;
    rConfiguration.u4DSConfig = nicChannelNum2Freq(prBssDesc->ucChannelNum);
    rConfiguration.rFHConfig.u4Length = sizeof(PARAM_802_11_CONFIG_FH_T);

    rateGetDataRatesFromRateSet(prBssDesc->u2OperationalRateSet, 0, aucRatesEx,
                                &ucRateLen);

    /* NOTE(Kevin): Set unused entries, if any, at the end of the array to
     * 0. from OID_802_11_BSSID_LIST
     */
    for (i = ucRateLen; i < ARRAY_SIZE(aucRatesEx); i++)
        aucRatesEx[i] = 0;

    switch (prBssDesc->eBSSType) {
    case BSS_TYPE_IBSS:
        eOpMode = NET_TYPE_IBSS;
        break;

    case BSS_TYPE_INFRASTRUCTURE:
    case BSS_TYPE_P2P_DEVICE:
    case BSS_TYPE_BOW_DEVICE:
    default:
        eOpMode = NET_TYPE_INFRA;
        break;
    }

    DBGLOG(SCN, TRACE, "ind %s %d %d\n", prBssDesc->aucSSID,
           prBssDesc->ucChannelNum, prBssDesc->ucRCPI);

    kalIndicateBssInfo(prAdapter->prGlueInfo, (u8 *)prSwRfb->pvHeader,
                       prSwRfb->u2PacketLen, prBssDesc->ucChannelNum,
                       RCPI_TO_dBm(prBssDesc->ucRCPI));

    nicAddScanResult(
        prAdapter, rMacAddr, &rSsid,
        prWlanBeaconFrame->u2CapInfo & CAP_INFO_PRIVACY ? 1 : 0,
        RCPI_TO_dBm(prBssDesc->ucRCPI), eNetworkType, &rConfiguration, eOpMode,
        aucRatesEx, prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen,
        (u8 *)((unsigned long)(prSwRfb->pvHeader) + WLAN_MAC_MGMT_HEADER_LEN));

    return WLAN_STATUS_SUCCESS;
}

u8 scanCheckBssIsLegal(IN P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
    u8 fgAddToScanResult = false;
    ENUM_BAND_T eBand;
    u8 ucChannel;

    ASSERT(prAdapter);
    /* check the channel is in the legal doamin */
    if (rlmDomainIsLegalChannel(prAdapter, prBssDesc->eBand,
                                prBssDesc->ucChannelNum) == true) {
        /* check ucChannelNum/eBand for adjacement channel filtering */
        if (cnmAisInfraChannelFixed(prAdapter, &eBand, &ucChannel) == true &&
            (eBand != prBssDesc->eBand ||
             ucChannel != prBssDesc->ucChannelNum)) {
            fgAddToScanResult = false;
        } else {
            fgAddToScanResult = true;
        }
    }

    return fgAddToScanResult;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Parse the content of given Beacon or ProbeResp Frame.
 *
 * @param[in] prSwRfb            Pointer to the receiving SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS           if not report this SW_RFB_T to host
 * @retval WLAN_STATUS_PENDING           if report this SW_RFB_T to host as scan
 * result
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS scanProcessBeaconAndProbeResp(IN P_ADAPTER_T prAdapter,
                                          IN P_SW_RFB_T prSwRfb)
{
    P_SCAN_INFO_T prScanInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_BSS_DESC_T prBssDesc = (P_BSS_DESC_T)NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    P_BSS_INFO_T prAisBssInfo;
    P_WLAN_BEACON_FRAME_T prWlanBeaconFrame = (P_WLAN_BEACON_FRAME_T)NULL;

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    /* 4 <0> Ignore invalid Beacon Frame */
    if ((prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) <
        (TIMESTAMP_FIELD_LEN + BEACON_INTERVAL_FIELD_LEN +
         CAP_INFO_FIELD_LEN)) {
#ifndef _lint
        ASSERT(0);
#endif
        return rStatus;
    }

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prAisBssInfo = prAdapter->prAisBssInfo;
    prWlanBeaconFrame = (P_WLAN_BEACON_FRAME_T)prSwRfb->pvHeader;

    /* 4 <1> Parse and add into BSS_DESC_T */
    prBssDesc = scanAddToBssDesc(prAdapter, prSwRfb);

    if (prBssDesc) {
#if CFG_SUPPORT_BEACON_CHANGE_DETECTION
        /* 4 <1.1> Beacon Change Detection for Connected BSS */
        if ((prAisBssInfo != NULL) &&
            (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) &&
            ((prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE &&
              prConnSettings->eOPMode != NET_TYPE_IBSS) ||
             (prBssDesc->eBSSType == BSS_TYPE_IBSS &&
              prConnSettings->eOPMode != NET_TYPE_INFRA)) &&
            EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prAisBssInfo->aucBSSID) &&
            EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
                       prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen)) {
            u8 fgNeedDisconnect = false;

            /* <1.1.2> check if supported rate differs */
            if (prAisBssInfo->u2OperationalRateSet !=
                prBssDesc->u2OperationalRateSet) {
                fgNeedDisconnect = true;
            }

            /* <1.1.3> beacon content change detected, disconnect
             * immediately */
            if (fgNeedDisconnect == true) {
                aisBssBeaconTimeout(prAdapter, 0);
            }
        }
#endif
        /* 4 <1.1> Update AIS_BSS_INFO */
        if ((prAisBssInfo != NULL) &&
            (((prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE) &&
              (prConnSettings->eOPMode != NET_TYPE_IBSS)) ||
             (prBssDesc->eBSSType == BSS_TYPE_IBSS &&
              prConnSettings->eOPMode != NET_TYPE_INFRA))) {
            if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
                /**not* checking prBssDesc->fgIsConnected
                 * anymore, due to Linksys AP uses " " as hidden
                 * SSID, and would have different BSS descriptor
                 */
                if ((!prAisBssInfo->ucDTIMPeriod) &&
                    EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
                                   prAisBssInfo->aucBSSID) &&
                    (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) &&
                    ((prWlanBeaconFrame->u2FrameCtrl & MASK_FRAME_TYPE) ==
                     MAC_FRAME_BEACON)) {
                    prAisBssInfo->ucDTIMPeriod = prBssDesc->ucDTIMPeriod;

                    /* sync with firmware for beacon
                     * information */
                    nicPmIndicateBssConnected(prAdapter,
                                              prAisBssInfo->ucBssIndex);
                }
            }
#if CFG_SUPPORT_ADHOC
            if (EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
                           prConnSettings->aucSSID,
                           prConnSettings->ucSSIDLen) &&
                (prBssDesc->eBSSType == BSS_TYPE_IBSS) &&
                (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS)) {
                ASSERT(prSwRfb->prRxStatusGroup3);

                ibssProcessMatchedBeacon(prAdapter, prAisBssInfo, prBssDesc,
                                         nicRxGetRcpiValueFromRxv(RCPI_MODE_WF0,
                                                                  prSwRfb));
            }
#endif
        }

        rlmProcessBcn(
            prAdapter, prSwRfb,
            ((P_WLAN_BEACON_FRAME_T)(prSwRfb->pvHeader))->aucInfoElem,
            (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
            (u16)(OFFSET_OF(WLAN_BEACON_FRAME_BODY_T, aucInfoElem[0])));

        mqmProcessBcn(
            prAdapter, prSwRfb,
            ((P_WLAN_BEACON_FRAME_T)(prSwRfb->pvHeader))->aucInfoElem,
            (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
            (u16)(OFFSET_OF(WLAN_BEACON_FRAME_BODY_T, aucInfoElem[0])));

        /* 4 <3> Send SW_RFB_T to HIF when we perform SCAN for HOST */
        if (prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE ||
            prBssDesc->eBSSType == BSS_TYPE_IBSS) {
            /* for AIS, send to host */
            if (prConnSettings->fgIsScanReqIssued) {
                u8 fgAddToScanResult;

                fgAddToScanResult = scanCheckBssIsLegal(prAdapter, prBssDesc);

                if (fgAddToScanResult == true) {
                    rStatus = scanAddScanResult(prAdapter, prBssDesc, prSwRfb);
                }
            }
        }
#if CFG_ENABLE_WIFI_DIRECT
        if (prAdapter->fgIsP2PRegistered) {
            scanP2pProcessBeaconAndProbeResp(prAdapter, prSwRfb, &rStatus,
                                             prBssDesc, prWlanBeaconFrame);
        }
#endif
    }

    return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Search the Candidate of BSS Descriptor for JOIN(Infrastructure) or
 *        MERGE(AdHoc) according to current Connection Policy.
 *
 * \return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
P_BSS_DESC_T scanSearchBssDescByPolicy(IN P_ADAPTER_T prAdapter,
                                       IN u8 ucBssIndex)
{
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_BSS_INFO_T prBssInfo;
    P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo;
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prBSSDescList;
    P_BSS_DESC_T prBssDesc = (P_BSS_DESC_T)NULL;
    P_BSS_DESC_T prPrimaryBssDesc = (P_BSS_DESC_T)NULL;
    P_BSS_DESC_T prCandidateBssDesc = (P_BSS_DESC_T)NULL;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;
    P_STA_RECORD_T prPrimaryStaRec;
    P_STA_RECORD_T prCandidateStaRec = (P_STA_RECORD_T)NULL;
    u32 rCurrentTime;
    u8 fgIsFindFirst = (u8)false;
    u8 fgIsFindBestRSSI = (u8)false;
    u8 fgIsFixedChannel;
    ENUM_BAND_T eBand;
    u8 ucChannel;
    P_BSS_INFO_T prAisBssInfo;

    ASSERT(prAdapter);

    prAisBssInfo = prAdapter->prAisBssInfo;

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    prAisSpecBssInfo = &(prAdapter->rWifiVar.rAisSpecificBssInfo);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
    prBSSDescList = &prScanInfo->rBSSDescList;

    GET_CURRENT_SYSTIME(&rCurrentTime);

    /* check for fixed channel operation */
    if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
#if CFG_SUPPORT_CHNL_CONFLICT_REVISE
        fgIsFixedChannel =
            cnmAisDetectP2PChannel(prAdapter, &eBand, &ucChannel);
#else
        fgIsFixedChannel =
            cnmAisInfraChannelFixed(prAdapter, &eBand, &ucChannel);
#endif
    } else {
        fgIsFixedChannel = false;
    }

#if DBG
    if (prConnSettings->ucSSIDLen < ELEM_MAX_LEN_SSID) {
        prConnSettings->aucSSID[prConnSettings->ucSSIDLen] = '\0';
    }
#endif

    DBGLOG(SCN, INFO,
           "SEARCH: Num Of BSS_DESC_T = %d, Look for SSID: %s "
           "prConnSettings->fgIsConnByBssidIssued %x "
           "prConnSettings->eConnectionPolicy %x\n",
           prBSSDescList->u4NumElem, prConnSettings->aucSSID,
           prConnSettings->fgIsConnByBssidIssued,
           prConnSettings->eConnectionPolicy);

    /* 4 <1> The outer loop to search for a candidate. */
    LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T){
        /* TODO(Kevin): Update Minimum Channel Load Information here */

        DBGLOG(SCN, INFO, "SEARCH: [" MACSTR "], SSID:%s\n",
               MAC2STR(prBssDesc->aucBSSID), prBssDesc->aucSSID);

        /* 4 <2> Check PHY Type and attributes */
        /* 4 <2.1> Check Unsupported BSS PHY Type */
        if (!(prBssDesc->ucPhyTypeSet &
              (prAdapter->rWifiVar.ucAvailablePhyTypeSet))) {
            DBGLOG(SCN, INFO, "SEARCH: Ignore unsupported ucPhyTypeSet = %x\n",
                   prBssDesc->ucPhyTypeSet);
            continue;
        }
        /* 4 <2.2> Check if has unknown NonHT BSS Basic Rate Set. */
        if (prBssDesc->fgIsUnknownBssBasicRate) {
            DBGLOG(SCN, LOUD, "SEARCH: Ignore Unknown Bss Basic Rate\n");
            continue;
        }
        /* 4 <2.3> Check if fixed operation cases should be aware */
        if (fgIsFixedChannel == true &&
            (prBssDesc->eBand != eBand ||
             prBssDesc->ucChannelNum != ucChannel)) {
            DBGLOG(SCN, LOUD,
                   "SEARCH: Ignore BssBand[%d] != FixBand[%d] or BssCH[%d] != "
                   "FixCH[%d]\n",
                   prBssDesc->eBand, eBand, prBssDesc->ucChannelNum, ucChannel);
            continue;
        }
        /* 4 <2.4> Check if the channel is legal under regulatory domain
         */
        if (rlmDomainIsLegalChannel(prAdapter, prBssDesc->eBand,
                                    prBssDesc->ucChannelNum) == false) {
            DBGLOG(SCN, LOUD, "SEARCH: Ignore illegal CH Band[%d] CH[%d]\n",
                   prBssDesc->eBand, prBssDesc->ucChannelNum);
            continue;
        }
        /* 4 <2.5> Check if this BSS_DESC_T is stale */
        if (CHECK_FOR_TIMEOUT(rCurrentTime, prBssDesc->rUpdateTime,
                              SEC_TO_SYSTIME(SCN_BSS_DESC_STALE_SEC))) {
            if (prAisBssInfo->fgReConnBypassScan &&
                EQUAL_MAC_ADDR(prConnSettings->aucBSSID, prBssDesc->aucBSSID) &&
                prAisBssInfo->eConnectionState ==
                PARAM_MEDIA_STATE_DISCONNECTED) {
                prAisBssInfo->fgReConnBypassScan = 0;
                DBGLOG(SCN, LOUD,
                       "SEARCH: Found target Bss but skip stale state for DBDC "
                       "reconnect\n");
            } else {
                DBGLOG(
                    SCN, LOUD,
                    "SEARCH: Ignore stale Bss, CurrTime[%ld] BssUpdateTime[%ld]\n",
                    rCurrentTime, prBssDesc->rUpdateTime);
                continue;
            }
        }
        /* 4 <3> Check if reach the excessive join retry limit */
        /* NOTE(Kevin): STA_RECORD_T is recorded by TA. */
        prStaRec =
            cnmGetStaRecByAddress(prAdapter, ucBssIndex, prBssDesc->aucSrcAddr);

        if (prStaRec) {
            /* NOTE(Kevin):
             * The Status Code is the result of a Previous
             * Connection Request, we use this as SCORE for choosing
             * a proper candidate (Also used for compare see <6>)
             * The Reason Code is an indication of the reason why AP
             * reject us, we use this Code for "Reject" a SCAN
             * result to become our candidate(Like a blacklist).
             */

            if (prStaRec->u2StatusCode != STATUS_CODE_SUCCESSFUL) {
                /* NOTE(Kevin): greedy association - after
                 * timeout, we'll still try to associate to the
                 * AP whose STATUS of conection attempt was not
                 * success. We may also use (ucJoinFailureCount
                 * x JOIN_RETRY_INTERVAL_SEC) for time bound.
                 */
                if ((prStaRec->ucJoinFailureCount <
                     JOIN_MAX_RETRY_FAILURE_COUNT) ||
                    (CHECK_FOR_TIMEOUT(
                         rCurrentTime, prStaRec->rLastJoinTime,
                         SEC_TO_SYSTIME(JOIN_RETRY_INTERVAL_SEC)))) {
                    /* NOTE(Kevin): Every
                     * JOIN_RETRY_INTERVAL_SEC interval, we
                     * can retry
                     * JOIN_MAX_RETRY_FAILURE_COUNT times.
                     */
                    if (prStaRec->ucJoinFailureCount >=
                        JOIN_MAX_RETRY_FAILURE_COUNT) {
                        prStaRec->ucJoinFailureCount = 0;
                    }
                    DBGLOG(
                        SCN, INFO,
                        "SEARCH:Try to join BSS again,Status Code=%d(Curr=%ld/Last "
                        "Join=%ld)\n",
                        prStaRec->u2StatusCode, rCurrentTime,
                        prStaRec->rLastJoinTime);
                } else {
                    DBGLOG(
                        SCN, INFO,
                        "SEARCH: Ignore BSS which reach maximum Join Retry Count = %d\n",
                        JOIN_MAX_RETRY_FAILURE_COUNT);
                    continue;
                }
            }
        }

        /* 4 <4> Check for various NETWORK conditions */
        if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
            /* 4 <4.1> Check BSS Type for the corresponding
             * Operation Mode in Connection Setting */
            /* NOTE(Kevin): For NET_TYPE_AUTO_SWITCH, we will always
             * pass following check. */
            if (((prConnSettings->eOPMode == NET_TYPE_INFRA) &&
                 (prBssDesc->eBSSType != BSS_TYPE_INFRASTRUCTURE)) ||
                ((prConnSettings->eOPMode == NET_TYPE_IBSS ||
                  prConnSettings->eOPMode == NET_TYPE_DEDICATED_IBSS) &&
                 (prBssDesc->eBSSType != BSS_TYPE_IBSS))) {
                DBGLOG(SCN, INFO, "SEARCH: Ignore eBSSType = %s\n",
                       ((prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE) ?
                        "INFRASTRUCTURE" :
                        "IBSS"));
                continue;
            }
            /* 4 <4.2> Check AP's BSSID if OID_802_11_BSSID has been
             * set. */
            if ((prConnSettings->fgIsConnByBssidIssued) &&
                (prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE)) {
                if (UNEQUAL_MAC_ADDR(prConnSettings->aucBSSID,
                                     prBssDesc->aucBSSID)) {
                    DBGLOG(SCN, INFO,
                           "SEARCH: Ignore due to BSSID was not matched!\n");
                    continue;
                }
            }

            /* 4 <4.3> Check for AdHoc Mode */
            if (prBssDesc->eBSSType == BSS_TYPE_IBSS) {
                u32 rCurrentTime;

                /* 4 <4.3.1> Check if this SCAN record has been
                 * updated recently for IBSS. */
                /* NOTE(Kevin): Because some STA may change its
                 * BSSID frequently after it create the IBSS -
                 * e.g. IPN2220, so we need to make sure we get
                 * the new one. For BSS, if the old record was
                 * matched, however it won't be able to pass the
                 * Join Process later.
                 */
                GET_CURRENT_SYSTIME(&rCurrentTime);
                if (CHECK_FOR_TIMEOUT(
                        rCurrentTime, prBssDesc->rUpdateTime,
                        SEC_TO_SYSTIME(SCN_ADHOC_BSS_DESC_TIMEOUT_SEC))) {
                    DBGLOG(
                        SCN, LOUD,
                        "SEARCH: Skip old record of BSS Descriptor - BSSID:[" MACSTR
                        "]\n\n",
                        MAC2STR(
                            prBssDesc->aucBSSID));
                    continue;
                }
                /* 4 <4.3.2> Check Peer's capability */
                if (ibssCheckCapabilityForAdHocMode(prAdapter, prBssDesc) ==
                    WLAN_STATUS_FAILURE) {
                    DBGLOG(
                        SCN, INFO,
                        "SEARCH: Ignore BSS DESC MAC: " MACSTR
                        ", Capability is not supported for current AdHoc Mode.\n",
                        MAC2STR(prPrimaryBssDesc->aucBSSID));

                    continue;
                }

                /* 4 <4.3.3> Compare TSF */
                if (prBssInfo->fgIsBeaconActivated &&
                    UNEQUAL_MAC_ADDR(prBssInfo->aucBSSID,
                                     prBssDesc->aucBSSID)) {
                    DBGLOG(SCN, LOUD, "SEARCH: prBssDesc->fgIsLargerTSF = %d\n",
                           prBssDesc->fgIsLargerTSF);

                    if (!prBssDesc->fgIsLargerTSF) {
                        DBGLOG(SCN, INFO,
                               "SEARCH: Ignore BSS DESC MAC: [" MACSTR
                               "], Smaller TSF\n",
                               MAC2STR(prBssDesc->aucBSSID));
                        continue;
                    }
                }
            }
        }

        prPrimaryBssDesc = (P_BSS_DESC_T)NULL;

        /* 4 <6> Check current Connection Policy. */
        switch (prConnSettings->eConnectionPolicy) {
        case CONNECT_BY_SSID_BEST_RSSI:
            /* Choose Hidden SSID to join only if the
             * `fgIsEnableJoin...` is true */
            if (prAdapter->rWifiVar.fgEnableJoinToHiddenSSID &&
                prBssDesc->fgIsHiddenSSID) {
                /* NOTE(Kevin): following if () statement means
                 * that If Target is hidden, then we won't
                 * connect when user specify SSID_ANY policy.
                 */
                if (prConnSettings->ucSSIDLen) {
                    prPrimaryBssDesc = prBssDesc;

                    fgIsFindBestRSSI = true;
                }
            } else if (EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
                                  prConnSettings->aucSSID,
                                  prConnSettings->ucSSIDLen)) {
                prPrimaryBssDesc = prBssDesc;

                fgIsFindBestRSSI = true;
                DBGLOG(SCN, LOUD,
                       "SEARCH: Found BSS by SSID, [" MACSTR "], SSID:%s\n",
                       MAC2STR(prBssDesc->aucBSSID), prBssDesc->aucSSID);
            }
            break;

        case CONNECT_BY_SSID_ANY:
            /* NOTE(Kevin): In this policy, we don't know the
             * desired SSID from user, so we should exclude the
             * Hidden SSID from scan list. And because we refuse to
             * connect to Hidden SSID node at the beginning, so when
             * the JOIN Module deal with a BSS_DESC_T which has
             * fgIsHiddenSSID == true, then the Connection Settings
             * must be valid without doubt.
             */
            if (!prBssDesc->fgIsHiddenSSID) {
                prPrimaryBssDesc = prBssDesc;

                fgIsFindFirst = true;
            }
            break;

        case CONNECT_BY_BSSID:
            if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prConnSettings->aucBSSID)) {
                /* Make sure to match with SSID if supplied.
                 * Some dual band APs share a single BSSID among
                 * different BSSes.
                 */
                if ((prBssDesc->ucSSIDLen > 0 &&
                     prConnSettings->ucSSIDLen > 0 &&
                     EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
                                prConnSettings->aucSSID,
                                prConnSettings->ucSSIDLen)) ||
                    prConnSettings->ucSSIDLen == 0) {
                    if (prBssDesc->ucChannelNum ==
                        prConnSettings->ucChannelNum) {
                        prPrimaryBssDesc = prBssDesc;
                        fgIsFindFirst = true;
                    }
                }
            }
            break;

        default:
            break;
        }

        /* Primary Candidate was not found */
        if (prPrimaryBssDesc == NULL) {
            continue;
        }

        prPrimaryStaRec = prStaRec;

        /* 4 <8> Compare the Candidate and the Primary Scan Record. */
        if (!prCandidateBssDesc) {
            prCandidateBssDesc = prPrimaryBssDesc;
            prCandidateStaRec = prPrimaryStaRec;

            /* 4 <8.1> Condition - Get the first matched one. */
            if (fgIsFindFirst) {
                break;
            }
        } else {
            /* 4 <6D> Condition - Visible SSID win Hidden SSID. */
            if (prCandidateBssDesc->fgIsHiddenSSID) {
                if (!prPrimaryBssDesc->fgIsHiddenSSID) {
                    prCandidateBssDesc = prPrimaryBssDesc;  /* The non
                                                             * Hidden SSID
                                                             * win. */
                    prCandidateStaRec = prPrimaryStaRec;
                    continue;
                }
            } else {
                if (prPrimaryBssDesc->fgIsHiddenSSID) {
                    continue;
                }
            }

            /* 4 <6E> Condition - Choose the one with better
             * RCPI(RSSI). */
            if (fgIsFindBestRSSI) {
                /* TODO(Kevin): We shouldn't compare the actual
                 * value, we should allow some acceptable
                 * tolerance of some RSSI percentage here.
                 */
                DBGLOG(SCN, TRACE,
                       "Candidate [" MACSTR
                       "]: RCPI = %d, joinFailCnt=%d, Primary [" MACSTR
                       "]: RCPI = %d, joinFailCnt=%d\n",
                       MAC2STR(prCandidateBssDesc->aucBSSID),
                       prCandidateBssDesc->ucRCPI,
                       prCandidateBssDesc->ucJoinFailureCount,
                       MAC2STR(prPrimaryBssDesc->aucBSSID),
                       prPrimaryBssDesc->ucRCPI,
                       prPrimaryBssDesc->ucJoinFailureCount);

                ASSERT(!(prCandidateBssDesc->fgIsConnected &&
                         prPrimaryBssDesc->fgIsConnected));
                if (prPrimaryBssDesc->ucJoinFailureCount >
                    SCN_BSS_JOIN_FAIL_THRESOLD) {
                    /* give a chance to do join if join fail
                     * before
                     * SCN_BSS_DECRASE_JOIN_FAIL_CNT_SEC
                     * seconds
                     */
                    if (CHECK_FOR_TIMEOUT(
                            rCurrentTime, prBssDesc->rJoinFailTime,
                            SEC_TO_SYSTIME(SCN_BSS_JOIN_FAIL_CNT_RESET_SEC))) {
                        prBssDesc->ucJoinFailureCount -=
                            SCN_BSS_JOIN_FAIL_RESET_STEP;
                        DBGLOG(AIS, INFO,
                               "decrease join fail count for Bss " MACSTR
                               " to %u, timeout second %d\n",
                               MAC2STR(prBssDesc->aucBSSID),
                               prBssDesc->ucJoinFailureCount,
                               SCN_BSS_JOIN_FAIL_CNT_RESET_SEC);
                    }
                }
                /* NOTE: To prevent SWING, we do roaming only if
                 * target AP has at least 5dBm larger than us.
                 */
                if (prCandidateBssDesc->fgIsConnected) {
                    if ((prCandidateBssDesc->ucRCPI +
                         ROAMING_NO_SWING_RCPI_STEP <=
                         prPrimaryBssDesc->ucRCPI) &&
                        prPrimaryBssDesc->ucJoinFailureCount <=
                        SCN_BSS_JOIN_FAIL_THRESOLD) {
                        prCandidateBssDesc = prPrimaryBssDesc;
                        prCandidateStaRec = prPrimaryStaRec;
                        continue;
                    }
                } else if (prPrimaryBssDesc->fgIsConnected) {
                    if ((prCandidateBssDesc->ucRCPI <
                         prPrimaryBssDesc->ucRCPI +
                         ROAMING_NO_SWING_RCPI_STEP) ||
                        (prCandidateBssDesc->ucJoinFailureCount >
                         SCN_BSS_JOIN_FAIL_THRESOLD)) {
                        prCandidateBssDesc = prPrimaryBssDesc;
                        prCandidateStaRec = prPrimaryStaRec;
                        continue;
                    }
                } else if (prPrimaryBssDesc->ucJoinFailureCount >
                           SCN_BSS_JOIN_FAIL_THRESOLD) {
                    continue;
                } else if (prCandidateBssDesc->ucJoinFailureCount >
                           SCN_BSS_JOIN_FAIL_THRESOLD ||
                           prCandidateBssDesc->ucRCPI <
                           prPrimaryBssDesc->ucRCPI) {
                    prCandidateBssDesc = prPrimaryBssDesc;
                    prCandidateStaRec = prPrimaryStaRec;
                    continue;
                }
            }
        }
    }

    return prCandidateBssDesc;
}

void scanReportBss2Cfg80211(IN P_ADAPTER_T prAdapter,
                            IN ENUM_BSS_TYPE_T eBSSType,
                            IN P_BSS_DESC_T SpecificprBssDesc)
{
    P_SCAN_INFO_T prScanInfo = NULL;
    P_LINK_T prBSSDescList = NULL;
    P_BSS_DESC_T prBssDesc = NULL;
    RF_CHANNEL_INFO_T rChannelInfo;

    ASSERT(prAdapter);

    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

    prBSSDescList = &prScanInfo->rBSSDescList;

    DBGLOG(SCN, TRACE, "scanReportBss2Cfg80211\n");

    if (SpecificprBssDesc) {
        {
            /* check BSSID is legal channel */
            if (!scanCheckBssIsLegal(prAdapter, SpecificprBssDesc)) {
                DBGLOG(SCN, TRACE, "Remove specific SSID[%s %d]\n",
                       SpecificprBssDesc->aucSSID,
                       SpecificprBssDesc->ucChannelNum);
                return;
            }

            DBGLOG(SCN, TRACE, "Report Specific SSID[%s]\n",
                   SpecificprBssDesc->aucSSID);
            if (eBSSType == BSS_TYPE_INFRASTRUCTURE) {
                kalIndicateBssInfo(prAdapter->prGlueInfo,
                                   (u8 *)SpecificprBssDesc->aucRawBuf,
                                   SpecificprBssDesc->u2RawLength,
                                   SpecificprBssDesc->ucChannelNum,
                                   RCPI_TO_dBm(SpecificprBssDesc->ucRCPI));
            } else {
                rChannelInfo.ucChannelNum = SpecificprBssDesc->ucChannelNum;
                rChannelInfo.eBand = SpecificprBssDesc->eBand;
                kalP2PIndicateBssInfo(prAdapter->prGlueInfo,
                                      (u8 *)SpecificprBssDesc->aucRawBuf,
                                      SpecificprBssDesc->u2RawLength,
                                      &rChannelInfo,
                                      RCPI_TO_dBm(SpecificprBssDesc->ucRCPI));
            }

            SpecificprBssDesc->fgIsP2PReport = false;
        }
    } else {
#if CFG_AUTO_CHANNEL_SEL_SUPPORT
        /* Clear old ACS data (APNum, Dirtiness, ...) and initialize the
         * ch number */
        kalMemZero(&(prAdapter->rWifiVar.rChnLoadInfo),
                   sizeof(prAdapter->rWifiVar.rChnLoadInfo));
        wlanInitChnLoadInfoChannelList(prAdapter);
#endif

        /* Search BSS Desc from current SCAN result list. */
        LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T){
#if CFG_AUTO_CHANNEL_SEL_SUPPORT
            /* Record channel loading with channel's AP number */
            u8 ucIdx = wlanGetChannelIndex(prBssDesc->ucChannelNum);

            if (ucIdx < MAX_CHN_NUM) {
                prAdapter->rWifiVar.rChnLoadInfo.rEachChnLoad[ucIdx].u2APNum++;
            }
#endif

            /* check BSSID is legal channel */
            if (!scanCheckBssIsLegal(prAdapter, prBssDesc)) {
                DBGLOG(SCN, TRACE, "Remove SSID[%s %d]\n", prBssDesc->aucSSID,
                       prBssDesc->ucChannelNum);
                continue;
            }

            if ((prBssDesc->eBSSType == eBSSType) ||
                ((eBSSType == BSS_TYPE_P2P_DEVICE) &&
                 (prBssDesc->fgIsP2PReport == true &&
                  prAdapter->p2p_scan_report_all_bss))) {
                DBGLOG(SCN, TRACE, "Report ALL SSID[%s %d]\n",
                       prBssDesc->aucSSID, prBssDesc->ucChannelNum);

                if (eBSSType == BSS_TYPE_INFRASTRUCTURE) {
                    if (prBssDesc->u2RawLength != 0) {
                        kalIndicateBssInfo(prAdapter->prGlueInfo,
                                           (u8 *)prBssDesc->aucRawBuf,
                                           prBssDesc->u2RawLength,
                                           prBssDesc->ucChannelNum,
                                           RCPI_TO_dBm(prBssDesc->ucRCPI));
                        kalMemZero(prBssDesc->aucRawBuf, CFG_RAW_BUFFER_SIZE);
                        prBssDesc->u2RawLength = 0;
                        prBssDesc->fgIsP2PReport = false;
                    }
                } else {
                    if ((prBssDesc->fgIsP2PReport == true &&
                         prAdapter->p2p_scan_report_all_bss) &&
                        prBssDesc->u2RawLength != 0) {
                        rChannelInfo.ucChannelNum = prBssDesc->ucChannelNum;
                        rChannelInfo.eBand = prBssDesc->eBand;

                        kalP2PIndicateBssInfo(prAdapter->prGlueInfo,
                                              (u8 *)prBssDesc->aucRawBuf,
                                              prBssDesc->u2RawLength,
                                              &rChannelInfo,
                                              RCPI_TO_dBm(prBssDesc->ucRCPI));

                        /* do not clear it then we can
                         * pass the bss in Specific
                         * report */
                        /* kalMemZero(prBssDesc->aucRawBuf,CFG_RAW_BUFFER_SIZE);
                         */

                        /*
                         *  the BSS entry will not be
                         * cleared after scan done. So
                         * if we dont receive the BSS in
                         * next scan, we cannot pass it.
                         * We use u2RawLength for the
                         * purpose.
                         */
                        /* prBssDesc->u2RawLength=0; */
                        prBssDesc->fgIsP2PReport = false;
                    }
                }
            } else {
                prBssDesc->u2RawLength = 0;
            }
        }
#if CFG_AUTO_CHANNEL_SEL_SUPPORT
        wlanCalculateAllChannelDirtiness(prAdapter);
        wlanSortChannel(prAdapter);

        prAdapter->rWifiVar.rChnLoadInfo.fgDataReadyBit = true;
#endif
    }
}

void scanReportScanResultToAgps(P_ADAPTER_T prAdapter)
{
    P_LINK_T prBSSDescList = &prAdapter->rWifiVar.rScanInfo.rBSSDescList;
    P_BSS_DESC_T prBssDesc = NULL;
    P_AGPS_AP_LIST_T prAgpsApList =
        kalMemAlloc(sizeof(AGPS_AP_LIST_T), VIR_MEM_TYPE);
    P_AGPS_AP_INFO_T prAgpsInfo = &prAgpsApList->arApInfo[0];
    P_SCAN_INFO_T prScanInfo = &prAdapter->rWifiVar.rScanInfo;
    u8 ucIndex = 0;

    LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry, BSS_DESC_T){
        if (prBssDesc->rUpdateTime < prScanInfo->rLastScanCompletedTime) {
            continue;
        }
        COPY_MAC_ADDR(prAgpsInfo->aucBSSID, prBssDesc->aucBSSID);
        prAgpsInfo->ePhyType = AGPS_PHY_G;
        prAgpsInfo->u2Channel = prBssDesc->ucChannelNum;
        prAgpsInfo->i2ApRssi = RCPI_TO_dBm(prBssDesc->ucRCPI);
        prAgpsInfo++;
        ucIndex++;
        if (ucIndex == SCN_AGPS_AP_LIST_MAX_NUM) {
            break;
        }
    }
    prAgpsApList->ucNum = ucIndex;
    GET_CURRENT_SYSTIME(&prScanInfo->rLastScanCompletedTime);
    /* DBGLOG(SCN, INFO, ("num of scan list:%d\n", ucIndex)); */
    kalIndicateAgpsNotify(prAdapter, AGPS_EVENT_WLAN_AP_LIST,
                          (u8 *)prAgpsApList, sizeof(AGPS_AP_LIST_T));
    kalMemFree(prAgpsApList, VIR_MEM_TYPE, sizeof(AGPS_AP_LIST_T));
}
