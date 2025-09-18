// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */
#include "precomp.h"

APPEND_VAR_ATTRI_ENTRY_T txAssocRspAttributesTable[] = {
    { (P2P_ATTRI_HDR_LEN + P2P_ATTRI_MAX_LEN_STATUS), NULL,
      p2pFuncAppendAttriStatusForAssocRsp }
    /* 0 */ /* Status */
    ,
    { (P2P_ATTRI_HDR_LEN + P2P_ATTRI_MAX_LEN_EXT_LISTEN_TIMING), NULL,
      p2pFuncAppendAttriExtListenTiming }
};

APPEND_VAR_IE_ENTRY_T txProbeRspIETable[] = {
    { (ELEM_HDR_LEN + (RATE_NUM_SW - ELEM_MAX_LEN_SUP_RATES)), NULL,
      bssGenerateExtSuppRate_IE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_ERP), NULL, rlmRspGenerateErpIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP), NULL, rlmRspGenerateHtCapIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_OP), NULL, rlmRspGenerateHtOpIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_RSN), NULL, rsnGenerateRSNIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_OBSS_SCAN), NULL, rlmRspGenerateObssScanIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_EXT_CAP), NULL, rlmRspGenerateExtCapIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL, rsnGenerateWpaNoneIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_WMM_PARAM), NULL, mqmGenerateWmmParamIE }
#if CFG_SUPPORT_802_11AC
    ,
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_CAP), NULL,
      rlmRspGenerateVhtCapIE }  /*191 */
    ,
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_OP), NULL,
      rlmRspGenerateVhtOpIE }  /*192 */
    ,
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_OP_MODE_NOTIFICATION), NULL,
      rlmRspGenerateVhtOpNotificationIE }  /*199 */
#endif
#if CFG_SUPPORT_MTK_SYNERGY
    ,
    { (ELEM_HDR_LEN + ELEM_MIN_LEN_MTK_OUI), NULL, rlmGenerateMTKOuiIE }
#endif
};

#if (CFG_SUPPORT_DFS_MASTER == 1)
u8 g_fgManualCac = false;
u32 g_u4DriverCacTime;
u32 g_u4CacStartBootTime;
u8 g_ucRadarDetectMode = false;
struct P2P_RADAR_INFO g_rP2pRadarInfo;
u8 g_ucDfsState = DFS_STATE_INACTIVE;
static u8 *apucDfsState[DFS_STATE_NUM] = {
    (u8 *)DISP_STRING("DFS_STATE_INACTIVE"),
    (u8 *)DISP_STRING("DFS_STATE_CHECKING"),
    (u8 *)DISP_STRING("DFS_STATE_ACTIVE"),
    (u8 *)DISP_STRING("DFS_STATE_DETECTED")
};

u8 *apucW53RadarType[3] = { (u8 *)DISP_STRING("Unknown Type"),
                            (u8 *)DISP_STRING("Type 1 (short pulse)"),
                            (u8 *)DISP_STRING("Type 2 (short pulse)") };
u8 *apucW56RadarType[12] = {
    (u8 *)DISP_STRING("Unknown Type"),
    (u8 *)DISP_STRING("Type 1 (short pulse)"),
    (u8 *)DISP_STRING("Type 2 (short pulse)"),
    (u8 *)DISP_STRING("Type 3 (short pulse)"),
    (u8 *)DISP_STRING("Type 4 (short pulse)"),
    (u8 *)DISP_STRING("Type 5 (short pulse)"),
    (u8 *)DISP_STRING("Type 6 (short pulse)"),
    (u8 *)DISP_STRING("Type 7 (long pulse)"),
    (u8 *)DISP_STRING("Type 8 (short pulse)"),
    (u8 *)DISP_STRING("Type 4 or Type 5 or Type 6 (short pulse)"),
    (u8 *)DISP_STRING("Type 5 or Type 6 or Type 8 (short pulse)"),
    (u8 *)DISP_STRING("Type 5 or Type 6 (short pulse)")
};
#endif

static void
p2pFuncParseBeaconVenderId(IN P_ADAPTER_T prAdapter, IN u8 *pucIE,
                           IN P_P2P_SPECIFIC_BSS_INFO_T prP2pSpecificBssInfo,
                           IN u8 ucRoleIndex);

static void p2pFuncProcessP2pProbeRspAction(
    IN P_ADAPTER_T prAdapter, IN u8 *pucIEBuf, IN u8 ucElemIdType,
    OUT u8 *ucBssIdx, OUT P_BSS_INFO_T *prP2pBssInfo, OUT u8 *fgIsWSCIE,
    OUT u8 *fgIsP2PIE, OUT u8 *fgIsWFDIE);
static void p2pFuncGetSpecAttriAction(IN P_IE_P2P_T prP2pIE, IN u8 ucOuiType,
                                      IN u8 ucAttriID,
                                      OUT P_ATTRIBUTE_HDR_T *prTargetAttri);
/*----------------------------------------------------------------------------*/
/*!
 * @brief Function for requesting scan. There is an option to do ACTIVE or
 * PASSIVE scan.
 *
 * @param eScanType - Specify the scan type of the scan request. It can be an
 * ACTIVE/PASSIVE Scan. eChannelSet - Specify the preferred channel set. A FULL
 * scan would request a legacy full channel normal scan.(usually ACTIVE). A
 * P2P_SOCIAL scan would scan 1+6+11 channels.(usually ACTIVE) A SPECIFIC scan
 * would only 1/6/11 channels scan. (Passive Listen/Specific Search)
 *               ucChannelNum - A specific channel number. (Only when channel is
 * specified) eBand - A specific band. (Only when channel is specified)
 *
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void p2pFuncRequestScan(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                        IN P_P2P_SCAN_REQ_INFO_T prScanReqInfo)
{
    P_MSG_SCN_SCAN_REQ_V2 prScanReqV2 = (P_MSG_SCN_SCAN_REQ_V2)NULL;

#ifdef CFG_SUPPORT_BEAM_PLUS
    /*NFC Beam + Indication */
    P_P2P_FSM_INFO_T prP2pFsmInfo = (P_P2P_FSM_INFO_T)NULL;
#endif

    DEBUGFUNC("p2pFuncRequestScan()");

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prScanReqInfo != NULL));

        if (prScanReqInfo->eChannelSet == SCAN_CHANNEL_SPECIFIED) {
            ASSERT_BREAK(prScanReqInfo->ucNumChannelList > 0);
            DBGLOG(P2P, LOUD, "P2P Scan Request Channel:%d\n",
                   prScanReqInfo->arScanChannelList[0].ucChannelNum);
        }

        prScanReqV2 = (P_MSG_SCN_SCAN_REQ_V2)cnmMemAlloc(
            prAdapter, RAM_TYPE_MSG,
            (sizeof(MSG_SCN_SCAN_REQ_V2) +
             (sizeof(PARAM_SSID_T) * prScanReqInfo->ucSsidNum)));
        if (!prScanReqV2) {
            ASSERT(0);  /* Can't trigger SCAN FSM */
            DBGLOG(
                P2P, ERROR,
                "p2pFuncRequestScan: Memory allocation fail, can not send SCAN "
                "MSG to scan module\n");
            break;
        }

        prScanReqV2->rMsgHdr.eMsgId = MID_P2P_SCN_SCAN_REQ_V2;
        prScanReqV2->ucSeqNum = ++prScanReqInfo->ucSeqNumOfScnMsg;
        prScanReqV2->ucBssIndex = ucBssIndex;
        prScanReqV2->eScanType = prScanReqInfo->eScanType;
        prScanReqV2->eScanChannel = prScanReqInfo->eChannelSet;
        prScanReqV2->u2IELen = 0;
        prScanReqV2->prSsid = (P_PARAM_SSID_T)((unsigned long)prScanReqV2 +
                                               sizeof(MSG_SCN_SCAN_REQ_V2));

        /* Copy IE for Probe Request. */
        kalMemCopy(prScanReqV2->aucIE, prScanReqInfo->aucIEBuf,
                   prScanReqInfo->u4BufLength);
        prScanReqV2->u2IELen = (u16)prScanReqInfo->u4BufLength;

        prScanReqV2->u2ChannelDwellTime = prScanReqInfo->u2PassiveDewellTime;
        prScanReqV2->u2TimeoutValue = 0;
        prScanReqV2->u2ProbeDelay = 0;

        switch (prScanReqInfo->eChannelSet) {
        case SCAN_CHANNEL_SPECIFIED: {
            u32 u4Idx = 0;
            P_RF_CHANNEL_INFO_T prDomainInfo =
                (P_RF_CHANNEL_INFO_T)prScanReqInfo->arScanChannelList;

            if (prScanReqInfo->ucNumChannelList >
                MAXIMUM_OPERATION_CHANNEL_LIST) {
                prScanReqInfo->ucNumChannelList =
                    MAXIMUM_OPERATION_CHANNEL_LIST;
            }

            for (u4Idx = 0; u4Idx < prScanReqInfo->ucNumChannelList; u4Idx++) {
                prScanReqV2->arChnlInfoList[u4Idx].ucChannelNum =
                    prDomainInfo->ucChannelNum;
                prScanReqV2->arChnlInfoList[u4Idx].eBand = prDomainInfo->eBand;
                prDomainInfo++;
            }

            prScanReqV2->ucChannelListNum = prScanReqInfo->ucNumChannelList;
        }

        /* fallthrough */
        case SCAN_CHANNEL_FULL:
        /* fallthrough */
        case SCAN_CHANNEL_2G4:
        /* fallthrough */
        case SCAN_CHANNEL_P2P_SOCIAL: {
            /* u8 aucP2pSsid[] = P2P_WILDCARD_SSID; */
            P_PARAM_SSID_T prParamSsid = (P_PARAM_SSID_T)NULL;

            prParamSsid = prScanReqV2->prSsid;

            for (prScanReqV2->ucSSIDNum = 0;
                 prScanReqV2->ucSSIDNum < prScanReqInfo->ucSsidNum;
                 prScanReqV2->ucSSIDNum++) {
                COPY_SSID(
                    prParamSsid->aucSsid, prParamSsid->u4SsidLen,
                    prScanReqInfo->arSsidStruct[prScanReqV2->ucSSIDNum].aucSsid,
                    prScanReqInfo->arSsidStruct[prScanReqV2->ucSSIDNum]
                    .ucSsidLen);

                prParamSsid++;
            }

            /* For compatible. (in FW?) need to check. */
            if (prScanReqV2->ucSSIDNum == 0) {
                prScanReqV2->ucSSIDType = SCAN_REQ_SSID_P2P_WILDCARD;
            } else {
                prScanReqV2->ucSSIDType = SCAN_REQ_SSID_SPECIFIED;
            }
        } break;

        default:
            /* Currently there is no other scan channel set. */
            ASSERT(false);
            break;
        }

        prScanReqInfo->fgIsScanRequest = true;

        mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prScanReqV2,
                    MSG_SEND_METHOD_BUF);
    } while (false);
}

void p2pFuncCancelScan(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                       IN P_P2P_SCAN_REQ_INFO_T prScanInfo)
{
    P_MSG_SCN_SCAN_CANCEL prScanCancelMsg = (P_MSG_SCN_SCAN_CANCEL)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prScanInfo != NULL));

        if (!prScanInfo->fgIsScanRequest) {
            break;
        }

        if (prScanInfo->ucSeqNumOfScnMsg) {
            /* There is a channel privilege on hand. */
            DBGLOG(P2P, TRACE, "P2P Cancel Scan\n");

            prScanCancelMsg = (P_MSG_SCN_SCAN_CANCEL)cnmMemAlloc(
                prAdapter, RAM_TYPE_MSG, sizeof(MSG_SCN_SCAN_CANCEL));
            if (!prScanCancelMsg) {
                /* Buffer not enough, can not cancel scan
                 * request. */
                DBGLOG(P2P, TRACE, "Buffer not enough, can not cancel scan.\n");
                ASSERT(false);
                break;
            }
            kalMemZero(prScanCancelMsg, sizeof(MSG_SCN_SCAN_CANCEL));

            prScanCancelMsg->rMsgHdr.eMsgId = MID_P2P_SCN_SCAN_CANCEL;
            prScanCancelMsg->ucBssIndex = ucBssIndex;
            prScanCancelMsg->ucSeqNum = prScanInfo->ucSeqNumOfScnMsg++;
            prScanCancelMsg->fgIsChannelExt = false;
            prScanInfo->fgIsScanRequest = false;

            mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prScanCancelMsg,
                        MSG_SEND_METHOD_BUF);
        }
    } while (false);
}

void p2pFuncGCJoin(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prP2pBssInfo,
                   IN P_P2P_JOIN_INFO_T prP2pJoinInfo)
{
    P_MSG_JOIN_REQ_T prJoinReqMsg = (P_MSG_JOIN_REQ_T)NULL;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;
    P_BSS_DESC_T prBssDesc = (P_BSS_DESC_T)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL) &&
                     (prP2pJoinInfo != NULL));

        prBssDesc = prP2pJoinInfo->prTargetBssDesc;
        if ((prBssDesc) == NULL) {
            DBGLOG(P2P, ERROR, "p2pFuncGCJoin: NO Target BSS Descriptor\n");
            ASSERT(false);
            break;
        }

        if (prBssDesc->ucSSIDLen) {
            COPY_SSID(prP2pBssInfo->aucSSID, prP2pBssInfo->ucSSIDLen,
                      prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
        }

        /* 2 <1> We are goin to connect to this BSS */
        prBssDesc->fgIsConnecting = true;

        /* 2 <2> Setup corresponding STA_RECORD_T */
        prStaRec = bssCreateStaRecFromBssDesc(
            prAdapter,
            (prBssDesc->fgIsP2PPresent ? (STA_TYPE_P2P_GO) :
             (STA_TYPE_LEGACY_AP)),
            prP2pBssInfo->ucBssIndex, prBssDesc);

        if (prStaRec == NULL) {
            DBGLOG(P2P, TRACE, "Create station record fail\n");
            ASSERT(false);
            break;
        }

        prP2pJoinInfo->prTargetStaRec = prStaRec;
        prP2pJoinInfo->fgIsJoinComplete = false;
        prP2pJoinInfo->u4BufLength = 0;

        /* 2 <2.1> Sync. to FW domain */
        cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

        if (prP2pBssInfo->eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED) {
            prStaRec->fgIsReAssoc = false;
            prP2pJoinInfo->ucAvailableAuthTypes = (u8)AUTH_TYPE_OPEN_SYSTEM;
            prStaRec->ucTxAuthAssocRetryLimit = TX_AUTH_ASSOCI_RETRY_LIMIT;
        } else {
            DBGLOG(P2P, ERROR, "JOIN INIT: Join Request when connected.\n");
            ASSERT(false);
            /* TODO: Shall we considering ROAMIN case for P2P
             * Device?. */
            break;
        }

        /* 2 <4> Use an appropriate Authentication Algorithm Number
         * among the ucAvailableAuthTypes. */
        if (prP2pJoinInfo->ucAvailableAuthTypes & (u8)AUTH_TYPE_OPEN_SYSTEM) {
            DBGLOG(P2P, TRACE,
                   "JOIN INIT: Try to do Authentication with AuthType == "
                   "OPEN_SYSTEM.\n");

            prP2pJoinInfo->ucAvailableAuthTypes &= ~(u8)AUTH_TYPE_OPEN_SYSTEM;

            prStaRec->ucAuthAlgNum = (u8)AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
        } else {
            DBGLOG(P2P, ERROR, "JOIN INIT: ucAvailableAuthTypes Error.\n");
            ASSERT(false);
            break;
        }

        /* 4 <5> Overwrite Connection Setting for eConnectionPolicy ==
         * ANY (Used by Assoc Req) */

        /* 2 <5> Backup desired channel. */

        /* 2 <6> Send a Msg to trigger SAA to start JOIN process. */
        prJoinReqMsg = (P_MSG_JOIN_REQ_T)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                                     sizeof(MSG_JOIN_REQ_T));

        if (!prJoinReqMsg) {
            DBGLOG(P2P, TRACE, "Allocation Join Message Fail\n");
            ASSERT(false);
            return;
        }

        prJoinReqMsg->rMsgHdr.eMsgId = MID_P2P_SAA_FSM_START;
        prJoinReqMsg->ucSeqNum = ++prP2pJoinInfo->ucSeqNumOfReqMsg;
        prJoinReqMsg->prStaRec = prStaRec;

        /* TODO: Consider fragmentation info in station record. */

        mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prJoinReqMsg,
                    MSG_SEND_METHOD_BUF);
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will update the contain of BSS_INFO_T for AIS network
 * once the association was completed.
 *
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 * @param[in] prAssocRspSwRfb        Pointer to SW RFB of ASSOC RESP FRAME.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void p2pFuncUpdateBssInfoForJOIN(IN P_ADAPTER_T prAdapter,
                                 IN P_BSS_DESC_T prBssDesc,
                                 IN P_STA_RECORD_T prStaRec,
                                 IN P_BSS_INFO_T prP2pBssInfo,
                                 IN P_SW_RFB_T prAssocRspSwRfb)
{
    P_WLAN_ASSOC_RSP_FRAME_T prAssocRspFrame = (P_WLAN_ASSOC_RSP_FRAME_T)NULL;
    u16 u2IELength;
    u8 *pucIE;

    DEBUGFUNC("p2pUpdateBssInfoForJOIN()");

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prStaRec != NULL) &&
                     (prP2pBssInfo != NULL) && (prAssocRspSwRfb != NULL));

        prAssocRspFrame = (P_WLAN_ASSOC_RSP_FRAME_T)prAssocRspSwRfb->pvHeader;

        if (prBssDesc == NULL) {
            /* Target BSS NULL. */
            DBGLOG(P2P, TRACE, "Target BSS NULL\n");
            break;
        }

        DBGLOG(P2P, INFO, "Update P2P_BSS_INFO_T and apply settings to MAC\n");

        /* 3 <1> Update BSS_INFO_T from AIS_FSM_INFO_T or User Settings
         */
        /* 4 <1.1> Setup Operation Mode */
        ASSERT_BREAK(prP2pBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE);

        if (UNEQUAL_MAC_ADDR(prBssDesc->aucBSSID, prAssocRspFrame->aucBSSID)) {
            ASSERT(false);
        }
        /* 4 <1.2> Setup SSID */
        COPY_SSID(prP2pBssInfo->aucSSID, prP2pBssInfo->ucSSIDLen,
                  prBssDesc->aucSSID, prBssDesc->ucSSIDLen);

        /* 4 <1.3> Setup Channel, Band */
        prP2pBssInfo->ucPrimaryChannel = prBssDesc->ucChannelNum;
        prP2pBssInfo->eBand = prBssDesc->eBand;

        /* 3 <2> Update BSS_INFO_T from STA_RECORD_T */
        /* 4 <2.1> Save current AP's STA_RECORD_T and current AID */
        prP2pBssInfo->prStaRecOfAP = prStaRec;
        prP2pBssInfo->u2AssocId = prStaRec->u2AssocId;

        /* 4 <2.2> Setup Capability */
        prP2pBssInfo->u2CapInfo = prStaRec->u2CapInfo;  /* Use AP's Cap
                                                         * Info as BSS
                                                         * Cap Info */

        if (prP2pBssInfo->u2CapInfo & CAP_INFO_SHORT_PREAMBLE) {
            prP2pBssInfo->fgIsShortPreambleAllowed = true;
        } else {
            prP2pBssInfo->fgIsShortPreambleAllowed = false;
        }

        /* 4 <2.3> Setup PHY Attributes and Basic Rate Set/Operational
         * Rate Set */
        prP2pBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;

        prP2pBssInfo->ucNonHTBasicPhyType = prStaRec->ucNonHTBasicPhyType;

        prP2pBssInfo->u2OperationalRateSet = prStaRec->u2OperationalRateSet;
        prP2pBssInfo->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;

        nicTxUpdateBssDefaultRate(prP2pBssInfo);

        /* 3 <3> Update BSS_INFO_T from SW_RFB_T (Association Resp
         * Frame) */
        /* 4 <3.1> Setup BSSID */
        COPY_MAC_ADDR(prP2pBssInfo->aucBSSID, prAssocRspFrame->aucBSSID);

        u2IELength = (u16)((prAssocRspSwRfb->u2PacketLen -
                            prAssocRspSwRfb->u2HeaderLen) -
                           (OFFSET_OF(WLAN_ASSOC_RSP_FRAME_T, aucInfoElem[0]) -
                            WLAN_MAC_MGMT_HEADER_LEN));
        pucIE = prAssocRspFrame->aucInfoElem;

        /* 4 <3.2> Parse WMM and setup QBSS flag */
        /* Parse WMM related IEs and configure HW CRs accordingly */
        mqmProcessAssocRsp(prAdapter, prAssocRspSwRfb, pucIE, u2IELength);

        prP2pBssInfo->fgIsQBSS = prStaRec->fgIsQoS;

        /* 3 <4> Update BSS_INFO_T from BSS_DESC_T */

        prBssDesc->fgIsConnecting = false;
        prBssDesc->fgIsConnected = true;

        /* 4 <4.1> Setup MIB for current BSS */
        prP2pBssInfo->u2BeaconInterval = prBssDesc->u2BeaconInterval;
        /* NOTE: Defer ucDTIMPeriod updating to when beacon is received
         * after connection */
        prP2pBssInfo->ucDTIMPeriod = 0;
        prP2pBssInfo->u2ATIMWindow = 0;

        prP2pBssInfo->ucBeaconTimeoutCount = AIS_BEACON_TIMEOUT_COUNT_INFRA;

        /* 4 <4.2> Update HT information and set channel */
        /* Record HT related parameters in rStaRec and rBssInfo
         * Note: it shall be called before nicUpdateBss()
         */
        rlmProcessAssocRsp(prAdapter, prAssocRspSwRfb, pucIE, u2IELength);

        /* 4 <4.3> Sync with firmware for BSS-INFO */
        nicUpdateBss(prAdapter, prP2pBssInfo->ucBssIndex);

        /* 4 <4.4> *DEFER OPERATION* nicPmIndicateBssConnected() will be
         * invoked */
        /* inside scanProcessBeaconAndProbeResp() after 1st beacon is
         * received */
    } while (false);
}

WLAN_STATUS
p2pFunMgmtFrameTxDone(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo,
                      IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
    u8 fgIsSuccess = false;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prMsduInfo != NULL));

        if (rTxDoneStatus != TX_RESULT_SUCCESS) {
            DBGLOG(P2P, TRACE, "Mgmt Frame TX Fail, Status:%d.\n",
                   rTxDoneStatus);
        } else {
            fgIsSuccess = true;
            DBGLOG(P2P, TRACE, "Mgmt Frame TX Done.\n");
        }

        kalP2PIndicateMgmtTxStatus(prAdapter->prGlueInfo, prMsduInfo,
                                   fgIsSuccess);
    } while (false);

    return WLAN_STATUS_SUCCESS;
}

WLAN_STATUS
p2pFuncTxMgmtFrame(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                   IN P_MSDU_INFO_T prMgmtTxMsdu, IN u8 fgNonCckRate)
{
    WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;
    /* P_MSDU_INFO_T prTxMsduInfo = (P_MSDU_INFO_T)NULL; */
    P_WLAN_MAC_HEADER_T prWlanHdr = (P_WLAN_MAC_HEADER_T)NULL;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;
    u8 ucRetryLimit = 30;  /* TX_DESC_TX_COUNT_NO_LIMIT; */
    u8 fgDrop = false;
    P_BSS_INFO_T prBssInfo;

    do {
        ASSERT_BREAK(prAdapter != NULL);

        /* Drop this frame if BSS inactive */
        if (!IS_NET_ACTIVE(prAdapter, ucBssIndex)) {
            p2pDevFsmRunEventMgmtFrameTxDone(prAdapter, prMgmtTxMsdu,
                                             TX_RESULT_DROPPED_IN_DRIVER);
            cnmMgtPktFree(prAdapter, prMgmtTxMsdu);
            fgDrop = true;

            break;
        }

        prWlanHdr =
            (P_WLAN_MAC_HEADER_T)((unsigned long)prMgmtTxMsdu->prPacket +
                                  MAC_TX_RESERVED_FIELD);
        prStaRec =
            cnmGetStaRecByAddress(prAdapter, ucBssIndex, prWlanHdr->aucAddr1);
        /* prMgmtTxMsdu->ucBssIndex = ucBssIndex; */

        switch (prWlanHdr->u2FrameCtrl & MASK_FRAME_TYPE) {
        case MAC_FRAME_PROBE_RSP:
            DBGLOG(P2P, TRACE, "TX Probe Resposne Frame\n");
            prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
            if ((!nicTxIsMgmtResourceEnough(prAdapter)) ||
                (prBssInfo->fgIsNetAbsent)) {
                DBGLOG(P2P, TRACE,
                       "Drop Tx probe response due to resource issue\n");
                fgDrop = true;

                break;
            }
            prMgmtTxMsdu->ucStaRecIndex = (prStaRec != NULL) ?
                                          (prStaRec->ucIndex) :
                                          (STA_REC_INDEX_NOT_FOUND);
            /* Modifiy Lie time to 100 mS due to the STA only wait
             * 30-50mS */
            /* and AP do not need send it after STA left */
            nicTxSetPktLifeTime(prMgmtTxMsdu, 100);
            prMgmtTxMsdu =
                p2pFuncProcessP2pProbeRsp(prAdapter, ucBssIndex, prMgmtTxMsdu);
            ucRetryLimit = 6;
            break;

        default:
            prMgmtTxMsdu->ucBssIndex = ucBssIndex;
            break;
        }

        if (fgDrop) {
            /* Drop this frame */
            p2pDevFsmRunEventMgmtFrameTxDone(prAdapter, prMgmtTxMsdu,
                                             TX_RESULT_DROPPED_IN_DRIVER);
            cnmMgtPktFree(prAdapter, prMgmtTxMsdu);

            break;
        }

        TX_SET_MMPDU(prAdapter, prMgmtTxMsdu, prMgmtTxMsdu->ucBssIndex,
                     (prStaRec != NULL) ? (prStaRec->ucIndex) :
                     (STA_REC_INDEX_NOT_FOUND),
                     WLAN_MAC_MGMT_HEADER_LEN, prMgmtTxMsdu->u2FrameLength,
                     p2pDevFsmRunEventMgmtFrameTxDone, MSDU_RATE_MODE_AUTO);

        nicTxSetPktRetryLimit(prMgmtTxMsdu, ucRetryLimit);

        nicTxConfigPktControlFlag(prMgmtTxMsdu, MSDU_CONTROL_FLAG_FORCE_TX,
                                  true);

        nicTxEnqueueMsdu(prAdapter, prMgmtTxMsdu);
    } while (false);

    return rWlanStatus;
}

void p2pFuncStopComplete(IN P_ADAPTER_T prAdapter,
                         IN P_BSS_INFO_T prP2pBssInfo)
{
    do {
        ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL));

        DBGLOG(P2P, TRACE, "p2pFuncStopComplete\n");

        /* GO: It would stop Beacon TX. GC: Stop all BSS related PS
         * function. */
        nicPmIndicateBssAbort(prAdapter, prP2pBssInfo->ucBssIndex);
        /* Reset RLM related field of BSSINFO. */
        rlmBssAborted(prAdapter, prP2pBssInfo);

        UNSET_NET_ACTIVE(prAdapter, prP2pBssInfo->ucBssIndex);
        nicDeactivateNetwork(prAdapter, prP2pBssInfo->ucBssIndex);
        /* Release CNM channel */
        nicUpdateBss(prAdapter, prP2pBssInfo->ucBssIndex);

        /* Reset current OPMode */
        /* 20170628, remove reset opmode, otherwise we cannot free P2P
         * beacon buffer */
        prP2pBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;
        prP2pBssInfo->u4RsnSelectedAKMSuite = 0;

        /* Point StaRecOfAP to NULL when GC role stop Complete */
        prP2pBssInfo->prStaRecOfAP = NULL;
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will start a P2P Group Owner and send Beacon Frames.
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void p2pFuncStartGO(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo,
                    IN P_P2P_CONNECTION_REQ_INFO_T prP2pConnReqInfo,
                    IN P_P2P_CHNL_REQ_INFO_T prP2pChnlReqInfo)
{
#if (CFG_SUPPORT_DFS_MASTER == 1)
    P_CMD_RDD_ON_OFF_CTRL_T prCmdRddOnOffCtrl;
#endif

#ifdef CFG_SUPPORT_P2P_OPEN_SECURITY
    u8 fgIsOpenP2P = true;
#else
    u8 fgIsOpenP2P = false;
#endif

#ifdef CFG_SUPPORT_P2P_GO_11b_RATE
    u8 fgIs11bRate = true;
#else
    u8 fgIs11bRate = false;
#endif

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prBssInfo != NULL));

        if (prBssInfo->ucBssIndex >= MAX_BSS_INDEX) {
            DBGLOG(P2P, ERROR,
                   "P2P BSS exceed the number of P2P interface number.");
            ASSERT(false);
            break;
        }

        DBGLOG(P2P, TRACE, "p2pFuncStartGO:\n");

#if (CFG_SUPPORT_DFS_MASTER == 1)
        prCmdRddOnOffCtrl = (P_CMD_RDD_ON_OFF_CTRL_T)cnmMemAlloc(
            prAdapter, RAM_TYPE_MSG, sizeof(CMD_RDD_ON_OFF_CTRL_T));

        if (prCmdRddOnOffCtrl == NULL) {
            DBGLOG(P2P, ERROR, "Allocate memory for prCmdRddOnOffCtrl failed.");
            return;
        }
        kalMemZero(prCmdRddOnOffCtrl, sizeof(CMD_RDD_ON_OFF_CTRL_T));

        prCmdRddOnOffCtrl->ucDfsCtrl = RDD_START_TXQ;
        prCmdRddOnOffCtrl->ucRddIdx =
            prAdapter->aprBssInfo[prBssInfo->ucBssIndex]->eDBDCBand;

        DBGLOG(P2P, INFO, "p2pFuncStartGO: Start TXQ - DFS ctrl: %.d\n",
               prCmdRddOnOffCtrl->ucDfsCtrl);

        wlanSendSetQueryCmd(prAdapter, CMD_ID_RDD_ON_OFF_CTRL, true, false,
                            false, NULL, NULL, sizeof(*prCmdRddOnOffCtrl),
                            (u8 *)prCmdRddOnOffCtrl, NULL, 0);

        cnmMemFree(prAdapter, prCmdRddOnOffCtrl);
#endif

        /* Re-start AP mode.  */
        p2pFuncSwitchOPMode(prAdapter, prBssInfo, prBssInfo->eIntendOPMode,
                            false);

        prBssInfo->eIntendOPMode = OP_MODE_NUM;

        /* 4 <1.1> Assign SSID */
        COPY_SSID(prBssInfo->aucSSID, prBssInfo->ucSSIDLen,
                  prP2pConnReqInfo->rSsidStruct.aucSsid,
                  prP2pConnReqInfo->rSsidStruct.ucSsidLen);

        DBGLOG(P2P, TRACE, "GO SSID:%s\n", prBssInfo->aucSSID);

        /* 4 <1.2> Clear current AP's STA_RECORD_T and current AID */
        prBssInfo->prStaRecOfAP = (P_STA_RECORD_T)NULL;
        prBssInfo->u2AssocId = 0;

        /* 4 <1.3> Setup Channel, Band and Phy Attributes */
        prBssInfo->ucPrimaryChannel = prP2pChnlReqInfo->ucReqChnlNum;
        prBssInfo->eBand = prP2pChnlReqInfo->eBand;
        prBssInfo->eBssSCO = prP2pChnlReqInfo->eChnlSco;

        DBGLOG(P2P, TRACE, "GO Channel:%d SCO:%d\n",
               prBssInfo->ucPrimaryChannel, prBssInfo->eBssSCO);

        if (prBssInfo->eBand == BAND_5G) {
            /* Depend on eBand */
            prBssInfo->ucPhyTypeSet =
                (prAdapter->rWifiVar.ucAvailablePhyTypeSet &
                 PHY_TYPE_SET_802_11AN);
            prBssInfo->ucConfigAdHocAPMode = AP_MODE_11A;  /* Depend
                                                            * on
                                                            * eCurrentOPMode
                                                            * and
                                                            * ucPhyTypeSet
                                                            */
        } else if ((prP2pConnReqInfo->eConnRequest ==
                    P2P_CONNECTION_TYPE_PURE_AP) ||
                   fgIs11bRate) {
            /* Depend on eBand */
            prBssInfo->ucPhyTypeSet =
                (prAdapter->rWifiVar.ucAvailablePhyTypeSet &
                 PHY_TYPE_SET_802_11BGN);
            /* Depend on eCurrentOPMode and ucPhyTypeSet */
            prBssInfo->ucConfigAdHocAPMode = AP_MODE_MIXED_11BG;
        } else {
            ASSERT(prP2pConnReqInfo->eConnRequest == P2P_CONNECTION_TYPE_GO);
            /* Depend on eBand */
            prBssInfo->ucPhyTypeSet =
                (prAdapter->rWifiVar.ucAvailablePhyTypeSet &
                 PHY_TYPE_SET_802_11GN);
            /* Depend on eCurrentOPMode and ucPhyTypeSet */
            prBssInfo->ucConfigAdHocAPMode = AP_MODE_11G_P2P;
        }

        /* Overwrite BSS PHY type set by Feature Options */
        bssDetermineApBssInfoPhyTypeSet(prAdapter,
                                        (prP2pConnReqInfo->eConnRequest ==
                                         P2P_CONNECTION_TYPE_PURE_AP) ?
                                        true :
                                        false,
                                        prBssInfo);

        prBssInfo->ucNonHTBasicPhyType =
            (u8)rNonHTApModeAttributes[prBssInfo->ucConfigAdHocAPMode]
            .ePhyTypeIndex;
        prBssInfo->u2BSSBasicRateSet =
            rNonHTApModeAttributes[prBssInfo->ucConfigAdHocAPMode]
            .u2BSSBasicRateSet;
        prBssInfo->u2OperationalRateSet =
            rNonHTPhyAttributes[prBssInfo->ucNonHTBasicPhyType]
            .u2SupportedRateSet;

        if ((prBssInfo->ucAllSupportedRatesLen == 0) || fgIs11bRate) {
            rateGetDataRatesFromRateSet(prBssInfo->u2OperationalRateSet,
                                        prBssInfo->u2BSSBasicRateSet,
                                        prBssInfo->aucAllSupportedRates,
                                        &prBssInfo->ucAllSupportedRatesLen);
        }
        /* 4 <1.5> Setup MIB for current BSS */
        prBssInfo->u2ATIMWindow = 0;
        prBssInfo->ucBeaconTimeoutCount = 0;

        /* 3 <2> Update BSS_INFO_T common part */
#if CFG_SUPPORT_AAA
        prBssInfo->fgIsProtection = false;
        /* Always enable protection at P2P GO But OOBE AP*/
        if ((prP2pConnReqInfo->eConnRequest == P2P_CONNECTION_TYPE_GO) &&
            (!fgIsOpenP2P)) {
            prBssInfo->fgIsProtection = true;  /* Always enable
                                                * protection at P2P
                                                * GO */
        } else {
            if (!fgIsOpenP2P) {
                ASSERT(prP2pConnReqInfo->eConnRequest ==
                       P2P_CONNECTION_TYPE_PURE_AP);
            }
            if (kalP2PGetCipher(prAdapter->prGlueInfo,
                                (u8)prBssInfo->u4PrivateData)) {
                prBssInfo->fgIsProtection = true;
            }
        }

        bssInitForAP(prAdapter, prBssInfo, true);

        nicQmUpdateWmmParms(prAdapter, prBssInfo->ucBssIndex);
#endif

        /* 3 <3> Set MAC HW */
        /* 4 <3.1> Setup channel and bandwidth */
        rlmBssInitForAPandIbss(prAdapter, prBssInfo);

        /* 4 <3.2> Reset HW TSF Update Mode and Beacon Mode */
        nicUpdateBss(prAdapter, prBssInfo->ucBssIndex);

        /* 4 <3.3> Update Beacon again for network phy type confirmed.
         */
        bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);

        /* 4 <3.4> Setup BSSID */
        nicPmIndicateBssCreated(prAdapter, prBssInfo->ucBssIndex);
    } while (false);
}  /* p2pFuncStartGO() */

void p2pFuncStopGO(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prP2pBssInfo)
{
    u32 u4ClientCount = 0;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL));

        DBGLOG(P2P, TRACE, "p2pFuncStopGO\n");

        u4ClientCount = bssGetClientCount(prAdapter, prP2pBssInfo);

        if ((prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) &&
            (prP2pBssInfo->eIntendOPMode == OP_MODE_NUM)) {
            /* AP is created, Beacon Updated. */
            p2pFuncDissolve(prAdapter, prP2pBssInfo, true,
                            REASON_CODE_DEAUTH_LEAVING_BSS, true);
            prP2pBssInfo->eIntendOPMode = OP_MODE_P2P_DEVICE;
        }

        /* Do not Deactivate Network if any Client existed, we'll
         * deactive it after Deauth Tx done */
        if (u4ClientCount == 0) {
            DBGLOG(P2P, INFO, "No client! Deactive GO immediately.\n");
            p2pChangeMediaState(prAdapter, prP2pBssInfo,
                                PARAM_MEDIA_STATE_DISCONNECTED);
            p2pFuncStopComplete(prAdapter, prP2pBssInfo);
        }
    } while (false);
}

WLAN_STATUS p2pFuncRoleToBssIdx(IN P_ADAPTER_T prAdapter, IN u8 ucRoleIdx,
                                OUT u8 *pucBssIdx)
{
    WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (pucBssIdx != NULL));

        if (ucRoleIdx >= BSS_P2P_NUM) {
            rWlanStatus = WLAN_STATUS_FAILURE;
            break;
        }
        if (!prAdapter->rWifiVar.aprP2pRoleFsmInfo[ucRoleIdx]) {
            DBGLOG(P2P, WARN, "%s, invalid aprP2pRoleFsmInfo, ignore\n",
                   __func__);
            rWlanStatus = WLAN_STATUS_FAILURE;
        } else {
            *pucBssIdx =
                prAdapter->rWifiVar.aprP2pRoleFsmInfo[ucRoleIdx]->ucBssIndex;
        }
    } while (false);

    return rWlanStatus;
}

P_P2P_ROLE_FSM_INFO_T p2pFuncGetRoleByBssIdx(IN P_ADAPTER_T prAdapter,
                                             IN u8 ucBssIndex)
{
    s32 i = 0;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL));

        for (i = 0; i < BSS_P2P_NUM; i++) {
            if (!prAdapter->rWifiVar.aprP2pRoleFsmInfo[i]) {
                continue;
            }

            if (prAdapter->rWifiVar.aprP2pRoleFsmInfo[i]->ucBssIndex ==
                ucBssIndex) {
                break;
            }
        }
        if (i < BSS_P2P_NUM) {
            prP2pRoleFsmInfo = prAdapter->rWifiVar.aprP2pRoleFsmInfo[i];
        }
    } while (false);

    return prP2pRoleFsmInfo;
}

/* /////////////////////////////////   MT6630 CODE END
 * //////////////////////////////////////////////// */
void p2pFuncSwitchOPMode(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prP2pBssInfo,
                         IN ENUM_OP_MODE_T eOpMode, IN u8 fgSyncToFW)
{
    do {
        ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL) &&
                     (eOpMode < OP_MODE_NUM));

        if (prP2pBssInfo->eCurrentOPMode != eOpMode) {
            DBGLOG(P2P, TRACE,
                   "p2pFuncSwitchOPMode: Switch to from %d, to %d.\n",
                   prP2pBssInfo->eCurrentOPMode, eOpMode);

            switch (prP2pBssInfo->eCurrentOPMode) {
            case OP_MODE_ACCESS_POINT:
                /* p2pFuncDissolve will be done in
                 * p2pFuncStopGO(). */
                /* p2pFuncDissolve(prAdapter, prP2pBssInfo,
                * true, REASON_CODE_DEAUTH_LEAVING_BSS); */
                if (prP2pBssInfo->eIntendOPMode != OP_MODE_P2P_DEVICE) {
                    p2pFuncStopGO(prAdapter, prP2pBssInfo);

                    SET_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex);
                }
                break;

            default:
                break;
            }

            prP2pBssInfo->eIntendOPMode = eOpMode;

            /* The state is in disconnecting and can not change any
             * BSS status */
            if (IS_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex) &&
                IS_NET_ACTIVE(prAdapter, prP2pBssInfo->ucBssIndex)) {
                DBGLOG(P2P, TRACE, "under deauth procedure, Quit.\n");
                break;
            }

            prP2pBssInfo->eCurrentOPMode = eOpMode;
            switch (eOpMode) {
            case OP_MODE_INFRASTRUCTURE:
                DBGLOG(P2P, TRACE, "p2pFuncSwitchOPMode: Switch to Client.\n");

            /* fall through */
            case OP_MODE_ACCESS_POINT:
                /* Change interface address. */
                if (eOpMode == OP_MODE_ACCESS_POINT) {
                    DBGLOG(P2P, TRACE, "p2pFuncSwitchOPMode: Switch to AP.\n");
                    prP2pBssInfo->ucSSIDLen = 0;
                }

#if CFG_DUAL_P2PLIKE_INTERFACE
                /*avoid ap1 Bss  have diff A2 & A3, */
                /*ToDo :  fix for P2P case*/
#else
                COPY_MAC_ADDR(prP2pBssInfo->aucOwnMacAddr,
                              prAdapter->rWifiVar.aucInterfaceAddress);
                COPY_MAC_ADDR(prP2pBssInfo->aucBSSID,
                              prAdapter->rWifiVar.aucInterfaceAddress);
#endif
                break;

            case OP_MODE_P2P_DEVICE: {
                /* Change device address. */
                DBGLOG(P2P, TRACE,
                       "p2pFuncSwitchOPMode: Switch back to P2P Device.\n");

                p2pChangeMediaState(prAdapter, prP2pBssInfo,
                                    PARAM_MEDIA_STATE_DISCONNECTED);

                COPY_MAC_ADDR(prP2pBssInfo->aucOwnMacAddr,
                              prAdapter->rWifiVar.aucDeviceAddress);
                COPY_MAC_ADDR(prP2pBssInfo->aucBSSID,
                              prAdapter->rWifiVar.aucDeviceAddress);
            } break;

            default:
                ASSERT(false);
                break;
            }

            if (1) {
                P2P_DISCONNECT_INFO rP2PDisInfo;

                kalMemZero(&rP2PDisInfo, sizeof(P2P_DISCONNECT_INFO));
                rP2PDisInfo.ucRole = 2;
                wlanSendSetQueryCmd(prAdapter, CMD_ID_P2P_ABORT, true, false,
                                    false, NULL, NULL,
                                    sizeof(P2P_DISCONNECT_INFO),
                                    (u8 *)&rP2PDisInfo, NULL, 0);
            }

            DBGLOG(P2P, TRACE, "The device address is changed to " MACSTR "\n",
                   MAC2STR(prP2pBssInfo->aucOwnMacAddr));
            DBGLOG(P2P, TRACE, "The BSSID is changed to " MACSTR "\n",
                   MAC2STR(prP2pBssInfo->aucBSSID));

            /* Update BSS INFO to FW. */
            if ((fgSyncToFW) && (eOpMode != OP_MODE_ACCESS_POINT)) {
                nicUpdateBss(prAdapter, prP2pBssInfo->ucBssIndex);
            }
        }
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to inform CNM that channel privilege
 *           has been released
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void p2pFuncReleaseCh(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx,
                      IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo)
{
    P_MSG_CH_ABORT_T prMsgChRelease = (P_MSG_CH_ABORT_T)NULL;

    DEBUGFUNC("p2pFuncReleaseCh()");

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL));

        if (!prChnlReqInfo->fgIsChannelRequested) {
            break;
        }
        DBGLOG(P2P, TRACE, "P2P Release Channel\n");
        prChnlReqInfo->fgIsChannelRequested = false;

        /* 1. return channel privilege to CNM immediately */
        prMsgChRelease = (P_MSG_CH_ABORT_T)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                                       sizeof(MSG_CH_ABORT_T));
        if (!prMsgChRelease) {
            ASSERT(0);  /* Can't release Channel to CNM */
            break;
        }

        prMsgChRelease->rMsgHdr.eMsgId = MID_MNY_CNM_CH_ABORT;
        prMsgChRelease->ucBssIndex = ucBssIdx;
        prMsgChRelease->ucTokenID = prChnlReqInfo->ucSeqNumOfChReq++;
#if CFG_SUPPORT_DBDC
        prMsgChRelease->eDBDCBand = ENUM_BAND_AUTO;

        DBGLOG(P2P, INFO, "p2pFuncReleaseCh: P2P abort channel on band %u.\n",
               prMsgChRelease->eDBDCBand);
#endif
        mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prMsgChRelease,
                    MSG_SEND_METHOD_BUF);
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process of CHANNEL_REQ_JOIN Initial. Enter CHANNEL_REQ_JOIN State.
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void p2pFuncAcquireCh(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx,
                      IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo)
{
    P_MSG_CH_REQ_T prMsgChReq = (P_MSG_CH_REQ_T)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prChnlReqInfo != NULL));

        p2pFuncReleaseCh(prAdapter, ucBssIdx, prChnlReqInfo);

        /* send message to CNM for acquiring channel */
        prMsgChReq = (P_MSG_CH_REQ_T)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                                 sizeof(MSG_CH_REQ_T));

        if (!prMsgChReq) {
            ASSERT(0);  /* Can't indicate CNM for channel acquiring
                         */
            break;
        }

        prMsgChReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
        prMsgChReq->ucBssIndex = ucBssIdx;
        prMsgChReq->ucTokenID = ++prChnlReqInfo->ucSeqNumOfChReq;
        prMsgChReq->eReqType = prChnlReqInfo->eChnlReqType;
        prMsgChReq->u4MaxInterval = prChnlReqInfo->u4MaxInterval;
        prMsgChReq->ucPrimaryChannel = prChnlReqInfo->ucReqChnlNum;
        prMsgChReq->eRfSco = prChnlReqInfo->eChnlSco;
        prMsgChReq->eRfBand = prChnlReqInfo->eBand;
        prMsgChReq->eRfChannelWidth = prChnlReqInfo->eChannelWidth;
        prMsgChReq->ucRfCenterFreqSeg1 = prChnlReqInfo->ucCenterFreqS1;
        prMsgChReq->ucRfCenterFreqSeg2 = prChnlReqInfo->ucCenterFreqS2;
#if CFG_SUPPORT_DBDC
        prMsgChReq->eDBDCBand = ENUM_BAND_AUTO;

        DBGLOG(
            P2P, INFO,
            "p2pFuncAcquireCh: P2P Request channel on band %u. RfBand:%d. CH:%d\n",
            prMsgChReq->eDBDCBand, prChnlReqInfo->eBand,
            prChnlReqInfo->ucReqChnlNum);
#endif
        /* Channel request join BSSID. */

        mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prMsgChReq,
                    MSG_SEND_METHOD_BUF);

        prChnlReqInfo->fgIsChannelRequested = true;
    } while (false);
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
void p2pFuncStartRdd(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx)
{
    P_CMD_RDD_ON_OFF_CTRL_T prCmdRddOnOffCtrl;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    u8 ucReqChnlNum;

    DEBUGFUNC("p2pFuncStartRdd()");

    prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
        prAdapter, prAdapter->aprBssInfo[ucBssIdx]->u4PrivateData);

    ucReqChnlNum = prP2pRoleFsmInfo->rChnlReqInfo.ucReqChnlNum;

    prCmdRddOnOffCtrl = (P_CMD_RDD_ON_OFF_CTRL_T)cnmMemAlloc(
        prAdapter, RAM_TYPE_MSG, sizeof(CMD_RDD_ON_OFF_CTRL_T));

    if (!prCmdRddOnOffCtrl) {
        DBGLOG(P2P, ERROR, "cnmMemAlloc for prCmdRddOnOffCtrl failed!\n");
        return;
    }
    kalMemZero(prCmdRddOnOffCtrl, sizeof(CMD_RDD_ON_OFF_CTRL_T));

    prCmdRddOnOffCtrl->ucDfsCtrl = RDD_START;

    prCmdRddOnOffCtrl->ucRddIdx = prAdapter->aprBssInfo[ucBssIdx]->eDBDCBand;

    if (rlmDomainGetDfsRegion() == NL80211_DFS_JP) {
        if (ucReqChnlNum >= 52 && ucReqChnlNum <= 64) {
            prCmdRddOnOffCtrl->ucRegDomain = REG_JP_53;
        } else if (ucReqChnlNum >= 100 && ucReqChnlNum <= 140) {
            prCmdRddOnOffCtrl->ucRegDomain = REG_JP_56;
        }
    } else {
        prCmdRddOnOffCtrl->ucRegDomain = REG_DEFAULT;
    }

    if (prCmdRddOnOffCtrl->ucRddIdx) {
        prCmdRddOnOffCtrl->ucRddInSel = RDD_IN_SEL_1;
    } else {
        prCmdRddOnOffCtrl->ucRddInSel = RDD_IN_SEL_0;
    }

    DBGLOG(
        P2P, INFO,
        "p2pFuncStartRdd: Start Radar detection - DFS ctrl: %d, RDD index: %d\n",
        prCmdRddOnOffCtrl->ucDfsCtrl, prCmdRddOnOffCtrl->ucRddIdx);

    wlanSendSetQueryCmd(prAdapter, CMD_ID_RDD_ON_OFF_CTRL, true, false, false,
                        NULL, NULL, sizeof(*prCmdRddOnOffCtrl),
                        (u8 *)prCmdRddOnOffCtrl, NULL, 0);

    cnmMemFree(prAdapter, prCmdRddOnOffCtrl);
}

void p2pFuncStopRdd(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx)
{
    P_CMD_RDD_ON_OFF_CTRL_T prCmdRddOnOffCtrl;

    DEBUGFUNC("p2pFuncStopRdd()");

    prCmdRddOnOffCtrl = (P_CMD_RDD_ON_OFF_CTRL_T)cnmMemAlloc(
        prAdapter, RAM_TYPE_MSG, sizeof(CMD_RDD_ON_OFF_CTRL_T));

    if (!prCmdRddOnOffCtrl) {
        DBGLOG(P2P, ERROR, "cnmMemAlloc for prCmdRddOnOffCtrl failed!\n");
        return;
    }
    kalMemZero(prCmdRddOnOffCtrl, sizeof(CMD_RDD_ON_OFF_CTRL_T));

    prCmdRddOnOffCtrl->ucDfsCtrl = RDD_STOP;

    prCmdRddOnOffCtrl->ucRddIdx = prAdapter->aprBssInfo[ucBssIdx]->eDBDCBand;

    if (prCmdRddOnOffCtrl->ucRddIdx) {
        prCmdRddOnOffCtrl->ucRddInSel = RDD_IN_SEL_1;
    } else {
        prCmdRddOnOffCtrl->ucRddInSel = RDD_IN_SEL_0;
    }

    DBGLOG(
        P2P, INFO,
        "p2pFuncStopRdd: Stop Radar detection - DFS ctrl: %d, RDD index: %d\n",
        prCmdRddOnOffCtrl->ucDfsCtrl, prCmdRddOnOffCtrl->ucRddIdx);

    wlanSendSetQueryCmd(prAdapter, CMD_ID_RDD_ON_OFF_CTRL, true, false, false,
                        NULL, NULL, sizeof(*prCmdRddOnOffCtrl),
                        (u8 *)prCmdRddOnOffCtrl, NULL, 0);

    cnmMemFree(prAdapter, prCmdRddOnOffCtrl);
}

void p2pFuncDfsSwitchCh(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo,
                        IN P2P_CHNL_REQ_INFO_T rP2pChnlReqInfo)
{
    P_GLUE_INFO_T prGlueInfo;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_CMD_RDD_ON_OFF_CTRL_T prCmdRddOnOffCtrl;
    P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;

    DEBUGFUNC("p2pFuncDfsSwitchCh()");

    if (!prBssInfo) {
        DBGLOG(P2P, ERROR, "prBssInfo shouldn't be NULL!\n");
        return;
    }

    /*  Setup Channel, Band */
    prBssInfo->ucPrimaryChannel = rP2pChnlReqInfo.ucReqChnlNum;
    prBssInfo->eBand = rP2pChnlReqInfo.eBand;
    prBssInfo->eBssSCO = rP2pChnlReqInfo.eChnlSco;

    /* Setup channel and bandwidth */
    rlmBssInitForAPandIbss(prAdapter, prBssInfo);

    /* Update Beacon again for network phy type confirmed. */
    bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);

    /* Reset HW TSF Update Mode and Beacon Mode */
    nicUpdateBss(prAdapter, prBssInfo->ucBssIndex);

    prCmdRddOnOffCtrl = (P_CMD_RDD_ON_OFF_CTRL_T)cnmMemAlloc(
        prAdapter, RAM_TYPE_MSG, sizeof(CMD_RDD_ON_OFF_CTRL_T));

    if (!prCmdRddOnOffCtrl) {
        DBGLOG(P2P, ERROR, "cnmMemAlloc for prCmdRddOnOffCtrl failed!\n");
        return;
    }
    kalMemZero(prCmdRddOnOffCtrl, sizeof(CMD_RDD_ON_OFF_CTRL_T));

    prCmdRddOnOffCtrl->ucDfsCtrl = RDD_START_TXQ;
    prCmdRddOnOffCtrl->ucRddIdx =
        prAdapter->aprBssInfo[prBssInfo->ucBssIndex]->eDBDCBand;

    DBGLOG(P2P, STATE, "p2pFuncDfsSwitchCh: Start TXQ - DFS ctrl: %.d\n",
           prCmdRddOnOffCtrl->ucDfsCtrl);

    wlanSendSetQueryCmd(prAdapter, CMD_ID_RDD_ON_OFF_CTRL, true, false, false,
                        NULL, NULL, sizeof(*prCmdRddOnOffCtrl),
                        (u8 *)prCmdRddOnOffCtrl, NULL, 0);

    cnmMemFree(prAdapter, prCmdRddOnOffCtrl);

    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prBssInfo->u4PrivateData);

    prGlueInfo = prAdapter->prGlueInfo;

#if CFG_SUPPORT_DBDC_TC6
    prGlueP2pInfo = prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex];
    p2pFuncModifyChandef(prAdapter, prGlueP2pInfo, prBssInfo);
    WARN_ON(!cfg80211_chandef_valid(prGlueP2pInfo->chandef));

    DBGLOG(P2P, INFO, "role(%d) b=%d f=%d w=%d s1=%d s2=%d\n",
           prP2pRoleFsmInfo->ucRoleIndex, prGlueP2pInfo->chandef->chan->band,
           prGlueP2pInfo->chandef->chan->center_freq,
           prGlueP2pInfo->chandef->width, prGlueP2pInfo->chandef->center_freq1,
           prGlueP2pInfo->chandef->center_freq2);
#endif

    DBGLOG(P2P, STATE, "p2pFuncDfsSwitchCh: Update to OS\n");
    /* NL80211 event should send to p2p group netdevice.
     * Otherwise wpa_supplicant wouldn't perform beacon update.
     * Hostapd case: prDevHandler same with aprRoleHandler
     * P2P GO case: p2p0=>prDevHandler, p2p-xxx-x=> aprRoleHandler
     */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 2))
    cfg80211_ch_switch_notify(
        prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->aprRoleHandler,
        prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->chandef);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0))
    cfg80211_ch_switch_notify(
        prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->prDevHandler,
        prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->chandef, 0);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(6, 9, 0))
    cfg80211_ch_switch_notify(
        prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->aprRoleHandler,
        prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->chandef, 0, 0);
#else
    cfg80211_ch_switch_notify(
        prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->aprRoleHandler,
        prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->chandef, 0);
#endif
    DBGLOG(P2P, STATE, "p2pFuncDfsSwitchCh: Update to OS Done\n");

#if !CFG_SUPPORT_DBDC_TC6  /* Fix sap will stop to disconnect sta caused by \
                            * incorrect memory free */
    if (prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->chandef->chan) {
        cnmMemFree(prGlueInfo->prAdapter, prGlueP2pInfo->chandef->chan);
    }

    prGlueP2pInfo->chandef->chan = NULL;

    if (prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->chandef) {
        cnmMemFree(
            prGlueInfo->prAdapter,
            prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->chandef);
    }
#endif

#if CFG_SUPPORT_DBDC_TC6
    if (prAdapter->rWifiVar.fgDelayInidicateDISCON) {
        DBGLOG(P2P, STATE,
               "p2pFuncDfsSwitchCh Done, report AIS disconnect event\n");
        cfg80211_disconnected(prAdapter->prGlueInfo->prDevHandler, 0, NULL, 0,
                              true, GFP_KERNEL);
        prAdapter->rWifiVar.fgDelayInidicateDISCON = false;
    }
#endif
    prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex]->chandef = NULL;
}

u8 p2pFuncCheckWeatherRadarBand(IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo)
{
    u8 ucReqChnlNum;
    u8 ucCenterFreqS1;
    ENUM_CHANNEL_WIDTH_T eChannelWidth;
    ENUM_CHNL_EXT_T eChnlSco;

    ucReqChnlNum = prChnlReqInfo->ucReqChnlNum;
    ucCenterFreqS1 = prChnlReqInfo->ucCenterFreqS1;
    eChannelWidth = prChnlReqInfo->eChannelWidth;
    eChnlSco = prChnlReqInfo->eChnlSco;

    if (rlmDomainGetDfsRegion() == NL80211_DFS_ETSI) {
        if (eChannelWidth == VHT_OP_CHANNEL_WIDTH_80) {
            if (ucCenterFreqS1 >= 120 && ucCenterFreqS1 <= 128) {
                return true;
            }
        } else {
            if ((ucReqChnlNum >= 120 && ucReqChnlNum <= 128)) {
                return true;
            } else if (ucReqChnlNum == 116 &&
                       eChnlSco == CHNL_EXT_SCA) {  /* ch116, 120 BW40 */
                return true;
            }
        }
    }

    return false;
}

s32 p2pFuncSetDriverCacTime(IN u32 u4CacTime)
{
    WLAN_STATUS i4Status = WLAN_STATUS_SUCCESS;

    g_u4DriverCacTime = u4CacTime;

    DBGLOG(P2P, INFO, "p2pFuncSetDriverCacTime: g_u4ManualCacTime = %dsec\n",
           g_u4DriverCacTime);

    return i4Status;
}

void p2pFuncEnableManualCac(void)
{
    g_fgManualCac = true;
}

u32 p2pFuncGetDriverCacTime(void)
{
    return g_u4DriverCacTime;
}

u8 p2pFuncIsManualCac(void)
{
    return g_fgManualCac;
}

void p2pFuncRadarInfoInit(void)
{
    kalMemZero(&g_rP2pRadarInfo, sizeof(g_rP2pRadarInfo));
}

void p2pFuncShowRadarInfo(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx)
{
    u8 ucCnt = 0;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    u8 ucReqChnlNum;

    if (g_rP2pRadarInfo.ucRadarReportMode == 1) {
        prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
            prAdapter, prAdapter->aprBssInfo[ucBssIdx]->u4PrivateData);

        ucReqChnlNum = prP2pRoleFsmInfo->rChnlReqInfo.ucReqChnlNum;

        DBGLOG(P2P, INFO, "-----Radar Detected Event-----\n");
        DBGLOG(P2P, INFO, "Radar detected in DBDC band%d\n",
               g_rP2pRadarInfo.ucRddIdx);

        switch (rlmDomainGetDfsRegion()) {
        case NL80211_DFS_FCC:
            DBGLOG(P2P, INFO, "Regulation domain: FCC\n");
            break;

        case NL80211_DFS_ETSI:
            DBGLOG(P2P, INFO, "Regulation domain: ETSI\n");
            break;

        case NL80211_DFS_JP:
            DBGLOG(P2P, INFO, "Regulation domain: JP\n");

            if (ucReqChnlNum >= 52 && ucReqChnlNum <= 64) {
                DBGLOG(P2P, INFO, "Radar type: W53 - %s\n",
                       p2pFuncJpW53RadarType());
            } else if (ucReqChnlNum >= 100 && ucReqChnlNum <= 140) {
                DBGLOG(P2P, INFO, "Radar type: W56 - %s\n",
                       p2pFuncJpW56RadarType());
            }
            break;

        default:
            break;
        }

        DBGLOG(P2P, INFO, "Radar Content:\n");

        DBGLOG(P2P, INFO, "start time pulse width PRI\n");

        if (g_rP2pRadarInfo.ucPeriodicDetected) {
            DBGLOG(P2P, INFO, "%-10d %-11d -\n",
                   g_rP2pRadarInfo.arPpbContent[ucCnt].u4PeriodicStartTime,
                   g_rP2pRadarInfo.arPpbContent[ucCnt].u2PeriodicPulseWidth);

            for (ucCnt = 1; ucCnt < g_rP2pRadarInfo.ucPPBNum; ucCnt++) {
                DBGLOG(
                    P2P, INFO, "%-10d %-11d %d\n",
                    g_rP2pRadarInfo.arPpbContent[ucCnt].u4PeriodicStartTime,
                    g_rP2pRadarInfo.arPpbContent[ucCnt].u2PeriodicPulseWidth,
                    (g_rP2pRadarInfo.arPpbContent[ucCnt].u4PeriodicStartTime -
                     g_rP2pRadarInfo.arPpbContent[ucCnt - 1]
                     .u4PeriodicStartTime) *
                    2 / 5);
            }
        } else if (g_rP2pRadarInfo.ucLongDetected) {
            DBGLOG(P2P, INFO, "%-10d %-11d -\n",
                   g_rP2pRadarInfo.arLpbContent[ucCnt].u4LongStartTime,
                   g_rP2pRadarInfo.arLpbContent[ucCnt].u2LongPulseWidth);

            for (ucCnt = 1; ucCnt < g_rP2pRadarInfo.ucLPBNum; ucCnt++) {
                DBGLOG(
                    P2P, INFO, "%-10d %-11d %d\n",
                    g_rP2pRadarInfo.arLpbContent[ucCnt].u4LongStartTime,
                    g_rP2pRadarInfo.arLpbContent[ucCnt].u2LongPulseWidth,
                    (g_rP2pRadarInfo.arLpbContent[ucCnt].u4LongStartTime -
                     g_rP2pRadarInfo.arLpbContent[ucCnt - 1].u4LongStartTime) *
                    2 / 5);
            }
        }
    }
}

void p2pFuncGetRadarInfo(IN struct P2P_RADAR_INFO *prP2pRadarInfo)
{
    kalMemCopy(prP2pRadarInfo, &g_rP2pRadarInfo, sizeof(*prP2pRadarInfo));
}

u8 *p2pFuncJpW53RadarType(void)
{
    u32 u4Type1Diff;
    u32 u4Type2Diff;

    if (g_rP2pRadarInfo.u4PRI1stUs >= 1428) {
        u4Type1Diff = g_rP2pRadarInfo.u4PRI1stUs - 1428;
    } else {
        u4Type1Diff = 1428 - g_rP2pRadarInfo.u4PRI1stUs;
    }

    if (g_rP2pRadarInfo.u4PRI1stUs >= 3846) {
        u4Type2Diff = g_rP2pRadarInfo.u4PRI1stUs - 3846;
    } else {
        u4Type2Diff = 3846 - g_rP2pRadarInfo.u4PRI1stUs;
    }

    if (u4Type1Diff < u4Type2Diff) {
        return apucW53RadarType[1];
    } else {
        return apucW53RadarType[2];
    }
}

u8 *p2pFuncJpW56RadarType(void)
{
    u32 u4Type1Diff;
    u32 u4Type2Diff;

    if (g_rP2pRadarInfo.ucLongDetected) {
        return apucW56RadarType[7];
    }

    if (g_rP2pRadarInfo.u4PRI1stUs >= 3980 &&
        g_rP2pRadarInfo.u4PRI1stUs <= 4020) {
        return apucW56RadarType[3];
    }

    if (g_rP2pRadarInfo.u4PRI1stUs >= 1368 &&
        g_rP2pRadarInfo.u4PRI1stUs <= 1448) {
        if (g_rP2pRadarInfo.u4PRI1stUs >= 1388) {
            u4Type1Diff = g_rP2pRadarInfo.u4PRI1stUs - 1388;
        } else {
            u4Type1Diff = 1388 - g_rP2pRadarInfo.u4PRI1stUs;
        }

        if (g_rP2pRadarInfo.u4PRI1stUs >= 1428) {
            u4Type2Diff = g_rP2pRadarInfo.u4PRI1stUs - 1428;
        } else {
            u4Type2Diff = 1428 - g_rP2pRadarInfo.u4PRI1stUs;
        }

        if (u4Type1Diff < u4Type2Diff) {
            return apucW56RadarType[1];
        } else {
            return apucW56RadarType[2];
        }
    }

    if (g_rP2pRadarInfo.u4PRI1stUs >= 130 && g_rP2pRadarInfo.u4PRI1stUs < 200) {
        return apucW56RadarType[4];
    }

    if (g_rP2pRadarInfo.u4PRI1stUs >= 200 &&
        g_rP2pRadarInfo.u4PRI1stUs <= 520) {
        if (g_rP2pRadarInfo.u4PRI1stUs <= 230) {
            return apucW56RadarType[9];
        }

        if (g_rP2pRadarInfo.u4PRI1stUs >= 323 &&
            g_rP2pRadarInfo.u4PRI1stUs <= 343) {
            return apucW56RadarType[10];
        }

        return apucW56RadarType[11];
    }

    return apucW56RadarType[0];
}

void p2pFuncSetRadarDetectMode(IN u8 ucRadarDetectMode)
{
    g_ucRadarDetectMode = ucRadarDetectMode;

    DBGLOG(P2P, INFO, "p2pFuncSetRadarDetectMode: g_ucRadarDetectMode: %d\n",
           g_ucRadarDetectMode);
}

u8 p2pFuncGetRadarDetectMode(void)
{
    return g_ucRadarDetectMode;
}

void p2pFuncSetDfsState(IN u8 ucDfsState)
{
    DBGLOG(P2P, INFO, "[DFS_STATE] TRANSITION: [%s] -> [%s]\n",
           apucDfsState[g_ucDfsState], apucDfsState[ucDfsState]);

    g_ucDfsState = ucDfsState;
}

u8 p2pFuncGetDfsState(void)
{
    return g_ucDfsState;
}

u8 *p2pFuncShowDfsState(void)
{
    return apucDfsState[g_ucDfsState];
}

void p2pFuncRecordCacStartBootTime(void)
{
    g_u4CacStartBootTime = kalGetBootTime();
}

u32 p2pFuncGetCacRemainingTime(void)
{
    u32 u4CurrentBootTime;
    u32 u4CacRemainingTime;

    u4CurrentBootTime = kalGetBootTime();

    u4CacRemainingTime = g_u4DriverCacTime -
                         (u4CurrentBootTime - g_u4CacStartBootTime) / 1000000;

    return u4CacRemainingTime;
}
#endif

WLAN_STATUS
p2pFuncBeaconUpdate(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prP2pBssInfo,
                    IN P_P2P_BEACON_UPDATE_INFO_T prBcnUpdateInfo,
                    IN u8 *pucNewBcnHdr, IN u32 u4NewHdrLen,
                    IN u8 *pucNewBcnBody, IN u32 u4NewBodyLen)
{
    WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;
    P_WLAN_BEACON_FRAME_T prBcnFrame = (P_WLAN_BEACON_FRAME_T)NULL;
    P_MSDU_INFO_T prBcnMsduInfo = (P_MSDU_INFO_T)NULL;
    u8 *pucIEBuf = (u8 *)NULL;
    u8 aucIEBuf[MAX_IE_LENGTH];

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL) &&
                     (prBcnUpdateInfo != NULL));

        prBcnMsduInfo = prP2pBssInfo->prBeacon;

#if DBG
        if (prBcnUpdateInfo->pucBcnHdr != NULL) {
            ASSERT((u32)prBcnUpdateInfo->pucBcnHdr ==
                   ((u32)prBcnMsduInfo->prPacket + MAC_TX_RESERVED_FIELD));
        }

        if (prBcnUpdateInfo->pucBcnBody != NULL) {
            ASSERT((u32)prBcnUpdateInfo->pucBcnBody ==
                   ((u32)prBcnUpdateInfo->pucBcnHdr +
                    (u32)prBcnUpdateInfo->u4BcnHdrLen));
        }
#endif
        prBcnFrame =
            (P_WLAN_BEACON_FRAME_T)((unsigned long)prBcnMsduInfo->prPacket +
                                    MAC_TX_RESERVED_FIELD);

        if (!pucNewBcnBody) {
            /* Old body. */
            pucNewBcnBody = prBcnUpdateInfo->pucBcnBody;
            ASSERT(u4NewBodyLen == 0);
            u4NewBodyLen = prBcnUpdateInfo->u4BcnBodyLen;
        } else {
            prBcnUpdateInfo->u4BcnBodyLen = u4NewBodyLen;
        }

        /* Temp buffer body part. */
        kalMemCopy(aucIEBuf, pucNewBcnBody, u4NewBodyLen);

        if (pucNewBcnHdr) {
            kalMemCopy(prBcnFrame, pucNewBcnHdr, u4NewHdrLen);
            prBcnUpdateInfo->pucBcnHdr = (u8 *)prBcnFrame;
            prBcnUpdateInfo->u4BcnHdrLen = u4NewHdrLen;
        }

        pucIEBuf = (u8 *)((unsigned long)prBcnUpdateInfo->pucBcnHdr +
                          (unsigned long)prBcnUpdateInfo->u4BcnHdrLen);
        kalMemCopy(pucIEBuf, aucIEBuf, u4NewBodyLen);
        prBcnUpdateInfo->pucBcnBody = pucIEBuf;

        /* Frame Length */
        prBcnMsduInfo->u2FrameLength =
            (u16)(prBcnUpdateInfo->u4BcnHdrLen + prBcnUpdateInfo->u4BcnBodyLen);

        prBcnMsduInfo->ucPacketType = TX_PACKET_TYPE_MGMT;
        prBcnMsduInfo->fgIs802_11 = true;
        prBcnMsduInfo->ucBssIndex = prP2pBssInfo->ucBssIndex;

        /* Update BSS INFO related information. */
        COPY_MAC_ADDR(prP2pBssInfo->aucOwnMacAddr, prBcnFrame->aucSrcAddr);
        COPY_MAC_ADDR(prP2pBssInfo->aucBSSID, prBcnFrame->aucBSSID);
        prP2pBssInfo->u2CapInfo = prBcnFrame->u2CapInfo;

        p2pFuncParseBeaconContent(
            prAdapter, prP2pBssInfo, (u8 *)prBcnFrame->aucInfoElem,
            (prBcnMsduInfo->u2FrameLength -
             OFFSET_OF(WLAN_BEACON_FRAME_T, aucInfoElem)));
    } while (false);

    return rWlanStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to update extra IEs (ex: WPS) for assoc resp.
 *           Caller should sanity check the params.
 *
 * \param[in] prAdapter      Pointer of ADAPTER_T
 * \param[in] prP2pBssInfo   Pointer to BSS_INFO_T structure
 * \param[in] AssocRespIE    Pointer to extra IEs for assoc resp
 * \param[in] u4AssocRespLen Length of extra IEs for assoc resp
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/

WLAN_STATUS
p2pFuncAssocRespUpdate(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prP2pBssInfo,
                       IN u8 *AssocRespIE, IN u32 u4AssocRespLen)
{
    u8 ucOuiType = 0;
    u16 u2SubTypeVersion = 0;

    if (!rsnParseCheckForWFAInfoElem(prAdapter, AssocRespIE, &ucOuiType,
                                     &u2SubTypeVersion)) {
        return WLAN_STATUS_FAILURE;
    }

    if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
        kalP2PUpdateWSC_IE(prAdapter->prGlueInfo, 3, (u8 *)AssocRespIE,
                           IE_SIZE(AssocRespIE),
                           (u8)(prP2pBssInfo->u4PrivateData));
    }

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to dissolve from group or one group. (Would
 * not change P2P FSM.)
 *              1. GC: Disconnect from AP. (Send Deauth)
 *              2. GO: Disconnect all STA
 *
 * @param[in] prAdapter   Pointer to the adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void p2pFuncDissolve(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prP2pBssInfo,
                     IN u8 fgSendDeauth, IN u16 u2ReasonCode,
                     IN u8 fgIsLocallyGenerated)
{
    P_STA_RECORD_T prCurrStaRec, prStaRecNext;
    P_LINK_T prClientList;

    DEBUGFUNC("p2pFuncDissolve()");

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL));

        switch (prP2pBssInfo->eCurrentOPMode) {
        case OP_MODE_INFRASTRUCTURE:
            /* Reset station record status. */
            if (prP2pBssInfo->prStaRecOfAP) {
                kalP2PGCIndicateConnectionStatus(
                    prAdapter->prGlueInfo, (u8)prP2pBssInfo->u4PrivateData,
                    NULL, NULL, 0, REASON_CODE_DEAUTH_LEAVING_BSS,
                    fgIsLocallyGenerated ?
                    WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY :
                    WLAN_STATUS_MEDIA_DISCONNECT);

                /* 2012/02/14 frog: After formation before join
                 * group, prStaRecOfAP is NULL. */
                p2pFuncDisconnect(prAdapter, prP2pBssInfo,
                                  prP2pBssInfo->prStaRecOfAP, fgSendDeauth,
                                  u2ReasonCode, fgIsLocallyGenerated);
            }

            /* Fix possible KE when RX Beacon & call
             * nicPmIndicateBssConnected(). hit prStaRecOfAP ==
             * NULL.
             */
            p2pChangeMediaState(prAdapter, prP2pBssInfo,
                                PARAM_MEDIA_STATE_DISCONNECTED);

            prP2pBssInfo->prStaRecOfAP = NULL;

            break;

        case OP_MODE_ACCESS_POINT:
            /* Under AP mode, we would net send deauthentication
             * frame to each STA. We only stop the Beacon & let all
             * stations timeout.
             */
            /* Send deauth. */
            authSendDeauthFrame(prAdapter, prP2pBssInfo, NULL, (P_SW_RFB_T)NULL,
                                u2ReasonCode, (PFN_TX_DONE_HANDLER)NULL);

            prClientList = &prP2pBssInfo->rStaRecOfClientList;

            /* This case may let LINK_FOR_EACH_ENTRY_SAFE crash */
            if (prClientList == NULL) {
                break;
            }

            LINK_FOR_EACH_ENTRY_SAFE(prCurrStaRec, prStaRecNext, prClientList,
                                     rLinkEntry, STA_RECORD_T){
                ASSERT(prCurrStaRec);
                p2pFuncDisconnect(prAdapter, prP2pBssInfo, prCurrStaRec, true,
                                  u2ReasonCode, fgIsLocallyGenerated);
            }
            break;

        default:
            return;  /* 20110420 -- alreay in Device Mode. */
        }

        /* Make the deauth frame send to FW ASAP. */
        wlanProcessCommandQueue(prAdapter, &prAdapter->prGlueInfo->rCmdQueue);
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to dissolve from group or one group. (Would
 * not change P2P FSM.)
 *              1. GC: Disconnect from AP. (Send Deauth)
 *              2. GO: Disconnect all STA
 *
 * @param[in] prAdapter   Pointer to the adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void p2pFuncDisconnect(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prP2pBssInfo,
                       IN P_STA_RECORD_T prStaRec, IN u8 fgSendDeauth,
                       IN u16 u2ReasonCode, IN u8 fgIsLocallyGenerated)
{
    ENUM_PARAM_MEDIA_STATE_T eOriMediaStatus;

    DBGLOG(P2P, INFO, "p2pFuncDisconnect()");

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prStaRec != NULL) &&
                     (prP2pBssInfo != NULL));

        ASSERT_BREAK(prP2pBssInfo->eNetworkType == NETWORK_TYPE_P2P);

        ASSERT_BREAK(prP2pBssInfo->ucBssIndex < P2P_DEV_BSS_INDEX);

        eOriMediaStatus = prP2pBssInfo->eConnectionState;

        /* Indicate disconnect. */
        if (prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
            P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo =
                P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
                                               prP2pBssInfo->u4PrivateData);

            kalP2PGOStationUpdate(prAdapter->prGlueInfo,
                                  prP2pRoleFsmInfo->ucRoleIndex, prStaRec,
                                  false);
        } else {
            P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo =
                P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
                                               prP2pBssInfo->u4PrivateData);

            prP2pRoleFsmInfo->rJoinInfo.prTargetBssDesc = NULL;
            prP2pRoleFsmInfo->rJoinInfo.prTargetStaRec = NULL;

            scanRemoveConnFlagOfBssDescByBssid(prAdapter,
                                               prP2pBssInfo->aucBSSID);
        }

        if (fgSendDeauth) {
            prStaRec->u2ReasonCode = u2ReasonCode;
            prStaRec->fgIsLocallyGenerated = fgIsLocallyGenerated;
            /* Send deauth. */
            authSendDeauthFrame(
                prAdapter, prP2pBssInfo, prStaRec, (P_SW_RFB_T)NULL,
                u2ReasonCode,
                (PFN_TX_DONE_HANDLER)p2pRoleFsmRunEventDeauthTxDone);
            wlanProcessCommandQueue(prAdapter,
                                    &prAdapter->prGlueInfo->rCmdQueue);
        } else {
            /* Change station state. */
            cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

            /* Reset Station Record Status. */
            p2pFuncResetStaRecStatus(prAdapter, prStaRec);

            cnmStaRecFree(prAdapter, prStaRec);

            if ((prP2pBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) ||
                (bssGetClientCount(prAdapter, prP2pBssInfo) == 0)) {
                DBGLOG(P2P, TRACE,
                       "No More Client, Media Status DISCONNECTED\n");
                p2pChangeMediaState(prAdapter, prP2pBssInfo,
                                    PARAM_MEDIA_STATE_DISCONNECTED);
            }

            if (eOriMediaStatus != prP2pBssInfo->eConnectionState) {
                /* Update Disconnected state to FW. */
                nicUpdateBss(prAdapter, prP2pBssInfo->ucBssIndex);
            }
        }
    } while (false);

    return;
}

void p2pFuncSetChannel(IN P_ADAPTER_T prAdapter, IN u8 ucRoleIdx,
                       IN P_RF_CHANNEL_INFO_T prRfChannelInfo)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_P2P_CONNECTION_REQ_INFO_T prP2pConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prRfChannelInfo != NULL));

        prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx);
        if (!prP2pRoleFsmInfo) {
            break;
        }
        prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);

        prP2pConnReqInfo->rChannelInfo.ucChannelNum =
            prRfChannelInfo->ucChannelNum;
        prP2pConnReqInfo->rChannelInfo.eBand = prRfChannelInfo->eBand;
        prP2pConnReqInfo->eChnlBw = prRfChannelInfo->ucChnlBw;
        prP2pConnReqInfo->u2PriChnlFreq = prRfChannelInfo->u2PriChnlFreq;
        prP2pConnReqInfo->u4CenterFreq1 = prRfChannelInfo->u4CenterFreq1;
        prP2pConnReqInfo->u4CenterFreq2 = prRfChannelInfo->u4CenterFreq2;

#if (CFG_SUPPORT_DFS_MASTER == 1)
        prP2pConnReqInfo->u4ChnlDfsState = prRfChannelInfo->u4ChnlDfsState;
#endif
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Retry JOIN for AUTH_MODE_AUTO_SWITCH
 *
 * @param[in] prStaRec       Pointer to the STA_RECORD_T
 *
 * @retval true      We will retry JOIN
 * @retval false     We will not retry JOIN
 */
/*----------------------------------------------------------------------------*/
u8 p2pFuncRetryJOIN(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec,
                    IN P_P2P_JOIN_INFO_T prJoinInfo)
{
    P_MSG_JOIN_REQ_T prJoinReqMsg = (P_MSG_JOIN_REQ_T)NULL;
    u8 fgRetValue = false;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prStaRec != NULL) &&
                     (prJoinInfo != NULL));

        /* Retry other AuthType if possible */
        if (!prJoinInfo->ucAvailableAuthTypes) {
            break;
        }

        if (prJoinInfo->ucAvailableAuthTypes & (u8)AUTH_TYPE_SHARED_KEY) {
            DBGLOG(P2P, INFO,
                   "RETRY JOIN INIT: Retry Authentication with AuthType == "
                   "SHARED_KEY.\n");

            prJoinInfo->ucAvailableAuthTypes &= ~(u8)AUTH_TYPE_SHARED_KEY;

            prStaRec->ucAuthAlgNum = (u8)AUTH_ALGORITHM_NUM_SHARED_KEY;
        } else {
            DBGLOG(
                P2P, ERROR,
                "RETRY JOIN INIT: Retry Authentication with Unexpected AuthType.\n");
            ASSERT(0);
            break;
        }

        prJoinInfo->ucAvailableAuthTypes = 0;  /* No more available Auth
                                                * Types */

        /* Trigger SAA to start JOIN process. */
        prJoinReqMsg = (P_MSG_JOIN_REQ_T)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                                     sizeof(MSG_JOIN_REQ_T));
        if (!prJoinReqMsg) {
            ASSERT(0);  /* Can't trigger SAA FSM */
            break;
        }

        prJoinReqMsg->rMsgHdr.eMsgId = MID_P2P_SAA_FSM_START;
        prJoinReqMsg->ucSeqNum = ++prJoinInfo->ucSeqNumOfReqMsg;
        prJoinReqMsg->prStaRec = prStaRec;

        mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prJoinReqMsg,
                    MSG_SEND_METHOD_BUF);

        fgRetValue = true;
    } while (false);

    return fgRetValue;
}

P_BSS_INFO_T p2pFuncBSSIDFindBssInfo(IN P_ADAPTER_T prAdapter, IN u8 *pucBSSID)
{
    P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T)NULL;
    u8 ucBssIdx = 0;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (pucBSSID != NULL));

        for (ucBssIdx = 0; ucBssIdx < BSS_INFO_NUM; ucBssIdx++) {
            if (!IS_NET_ACTIVE(prAdapter, ucBssIdx)) {
                continue;
            }

            prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

            if (EQUAL_MAC_ADDR(prBssInfo->aucBSSID, pucBSSID) &&
                IS_BSS_P2P(prBssInfo)) {
                break;
            }

            prBssInfo = NULL;
        }
    } while (false);

    return prBssInfo;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Auth Frame and then return
 *        the status code to AAA to indicate if need to perform following
 * actions when the specified conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[in] pprStaRec          Pointer to pointer of STA_RECORD_T structure.
 * @param[out] pu2StatusCode     The Status Code of Validation Result
 *
 * @retval true      Reply the Auth
 * @retval false     Don't reply the Auth
 */
/*----------------------------------------------------------------------------*/
u8 p2pFuncValidateAuth(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prP2pBssInfo,
                       IN P_SW_RFB_T prSwRfb, IN PP_STA_RECORD_T pprStaRec,
                       OUT u16 *pu2StatusCode)
{
    u8 fgPmfConn = false;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;
    P_WLAN_AUTH_FRAME_T prAuthFrame = (P_WLAN_AUTH_FRAME_T)NULL;

    DBGLOG(P2P, TRACE, "p2pValidate Authentication Frame\n");

    /* P2P 3.2.8 */
    *pu2StatusCode = STATUS_CODE_REQ_DECLINED;
    prAuthFrame = (P_WLAN_AUTH_FRAME_T)prSwRfb->pvHeader;

    if ((prP2pBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) ||
        (prP2pBssInfo->eIntendOPMode != OP_MODE_NUM)) {
        /* We are not under AP Mode yet. */
        DBGLOG(P2P, WARN, "Current OP mode is not under AP mode. (%d)\n",
               prP2pBssInfo->eCurrentOPMode);
        return false;
    }

    prStaRec = cnmGetStaRecByAddress(prAdapter, prP2pBssInfo->ucBssIndex,
                                     prAuthFrame->aucSrcAddr);

    if (!prStaRec) {
        prStaRec = cnmStaRecAlloc(prAdapter, STA_TYPE_P2P_GC,
                                  prP2pBssInfo->ucBssIndex,
                                  prAuthFrame->aucSrcAddr);

        /* TODO(Kevin): Error handling of allocation of STA_RECORD_T for
         * exhausted case and do removal of unused STA_RECORD_T.
         */
        /* Sent a message event to clean un-used STA_RECORD_T. */
        ASSERT(prStaRec);

        prSwRfb->ucStaRecIdx = prStaRec->ucIndex;

        prStaRec->u2BSSBasicRateSet = prP2pBssInfo->u2BSSBasicRateSet;

        prStaRec->u2DesiredNonHTRateSet = RATE_SET_ERP_P2P;

        prStaRec->u2OperationalRateSet = RATE_SET_ERP_P2P;
        prStaRec->ucPhyTypeSet = PHY_TYPE_SET_802_11GN;

        /* Update default Tx rate */
        nicTxUpdateStaRecDefaultRate(prStaRec);

        /* NOTE(Kevin): Better to change state here, not at TX Done */
        cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);
    } else {
#if CFG_SUPPORT_802_11W
        /* AP PMF. if PMF connection, do not reset state & FSM */
        fgPmfConn = rsnCheckBipKeyInstalled(prAdapter, prStaRec);
        if (fgPmfConn) {
            DBGLOG(P2P, WARN, "PMF Connction, return false\n");
            return false;
        }
#endif

        prSwRfb->ucStaRecIdx = prStaRec->ucIndex;

        if ((prStaRec->ucStaState > STA_STATE_1) && (IS_STA_IN_P2P(prStaRec))) {
            cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

            p2pFuncResetStaRecStatus(prAdapter, prStaRec);

            bssRemoveClient(prAdapter, prP2pBssInfo, prStaRec);
#if CFG_SUPPORT_802_11W
            if (timerPendingTimer(&(prStaRec->rPmfCfg.rSAQueryTimer))) {
                cnmTimerStopTimer(prAdapter,
                                  &(prStaRec->rPmfCfg.rSAQueryTimer));
            }
#endif
            p2pFuncDisconnect(prAdapter, prP2pBssInfo, prStaRec, false,
                              REASON_CODE_DISASSOC_INACTIVITY, true);
        }
    }

    if (bssGetClientCount(prAdapter, prP2pBssInfo) >=
        P2P_MAXIMUM_CLIENT_COUNT ||
        !p2pRoleProcessACLInspection(prAdapter, prStaRec->aucMacAddr,
                                     prP2pBssInfo->ucBssIndex)
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
        || kalP2PMaxClients(prAdapter->prGlueInfo,
                            bssGetClientCount(prAdapter, prP2pBssInfo),
                            (u8)prP2pBssInfo->u4PrivateData)
#endif
        ) {
        /* GROUP limit full. */
        /* P2P 3.2.8 */
        DBGLOG(P2P, WARN, "Group Limit Full. (%d)\n",
               bssGetClientCount(prAdapter, prP2pBssInfo));
        cnmStaRecFree(prAdapter, prStaRec);
        return true;
    }
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
    else {
        /* Hotspot Blacklist */
        if (kalP2PCmpBlackList(prAdapter->prGlueInfo, prAuthFrame->aucSrcAddr,
                               (u8)prP2pBssInfo->u4PrivateData)) {
            return false;
        }
    }
#endif
    /* prStaRec->eStaType = STA_TYPE_INFRA_CLIENT; */
    prStaRec->eStaType = STA_TYPE_P2P_GC;

    /* Update Station Record - Status/Reason Code */
    prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;

    prStaRec->ucJoinFailureCount = 0;

    *pprStaRec = prStaRec;

    *pu2StatusCode = STATUS_CODE_SUCCESSFUL;

    return true;
}

void p2pFuncResetStaRecStatus(IN P_ADAPTER_T prAdapter,
                              IN P_STA_RECORD_T prStaRec)
{
    do {
        if ((prAdapter == NULL) || (prStaRec == NULL)) {
            ASSERT(false);
            break;
        }

        prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;
        prStaRec->u2ReasonCode = REASON_CODE_RESERVED;
        prStaRec->ucJoinFailureCount = 0;
        prStaRec->fgTransmitKeyExist = false;

        prStaRec->fgSetPwrMgtBit = false;
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief The function is used to initialize the value of the connection
 * settings for P2P network
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void p2pFuncInitConnectionSettings(
    IN P_ADAPTER_T prAdapter, IN P_P2P_CONNECTION_SETTINGS_T prP2PConnSettings,
    IN u8 fgIsApMode)
{
    P_WIFI_VAR_T prWifiVar = NULL;

    ASSERT(prP2PConnSettings);

    prWifiVar = &(prAdapter->rWifiVar);
    ASSERT(prWifiVar);

    prP2PConnSettings->fgIsApMode = fgIsApMode;

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
    prP2PConnSettings->fgIsWPSMode = prWifiVar->ucApWpsMode;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Assoc Req Frame and then return
 *        the status code to AAA to indicate if need to perform following
 * actions when the specified conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[out] pu2StatusCode     The Status Code of Validation Result
 *
 * @retval true      Reply the Assoc Resp
 * @retval false     Don't reply the Assoc Resp
 */
/*----------------------------------------------------------------------------*/
u8 p2pFuncValidateAssocReq(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb,
                           OUT u16 *pu2StatusCode)
{
    u8 fgReplyAssocResp = true;
    P_WLAN_ASSOC_REQ_FRAME_T prAssocReqFrame = (P_WLAN_ASSOC_REQ_FRAME_T)NULL;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;

    /* TODO(Kevin): Call P2P functions to check ..
     *  2. Check we can accept connection from thsi peer
     *  a. If we are in PROVISION state, only accept the peer we do the GO
     * formation previously. b. If we are in OPERATION state, only accept
     * the other peer when P2P_GROUP_LIMIT is 0.
     *  3. Check Black List here.
     */

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prSwRfb != NULL) &&
                     (pu2StatusCode != NULL));

        *pu2StatusCode = STATUS_CODE_REQ_DECLINED;
        prAssocReqFrame = (P_WLAN_ASSOC_REQ_FRAME_T)prSwRfb->pvHeader;

        prP2pBssInfo =
            p2pFuncBSSIDFindBssInfo(prAdapter, prAssocReqFrame->aucBSSID);

        if (prP2pBssInfo == NULL) {
            DBGLOG(P2P, ERROR,
                   "RX ASSOC frame without BSS active / BSSID match\n");
            ASSERT(false);
            break;
        }

        prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);

        if (prStaRec == NULL) {
            /* Station record should be ready while RX AUTH frame.
             */
            fgReplyAssocResp = false;
            ASSERT(false);
            break;
        }
        ASSERT(prSwRfb->prRxStatusGroup3);
        prStaRec->ucRCPI = nicRxGetRcpiValueFromRxv(RCPI_MODE_WF0, prSwRfb);

        prStaRec->u2DesiredNonHTRateSet &= prP2pBssInfo->u2OperationalRateSet;
        prStaRec->ucDesiredPhyTypeSet = prStaRec->ucPhyTypeSet &
                                        prP2pBssInfo->ucPhyTypeSet;

        if (prStaRec->ucDesiredPhyTypeSet == 0) {
            /* The station only support 11B rate. */
            *pu2StatusCode = STATUS_CODE_ASSOC_DENIED_RATE_NOT_SUPPORTED;
            break;
        }

        *pu2StatusCode = STATUS_CODE_SUCCESSFUL;
    } while (false);

    return fgReplyAssocResp;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to check the TKIP IE
 *
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
u8 p2pFuncParseCheckForTKIPInfoElem(IN u8 *pucBuf)
{
    u8 aucWfaOui[] = VENDOR_OUI_WFA;
    P_WPA_INFO_ELEM_T prWpaIE = (P_WPA_INFO_ELEM_T)NULL;
    u32 u4GroupKeyCipher = 0;

    if (pucBuf == NULL) {
        return false;
    }

    prWpaIE = (P_WPA_INFO_ELEM_T)pucBuf;

    if (prWpaIE->ucLength <= ELEM_MIN_LEN_WFA_OUI_TYPE_SUBTYPE) {
        return false;
    }

    if (kalMemCmp(prWpaIE->aucOui, aucWfaOui, sizeof(aucWfaOui))) {
        return false;
    }

    WLAN_GET_FIELD_32(&prWpaIE->u4GroupKeyCipherSuite, &u4GroupKeyCipher);

    if (prWpaIE->ucOuiType == VENDOR_OUI_TYPE_WPA &&
        u4GroupKeyCipher == WPA_CIPHER_SUITE_TKIP) {
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to check the P2P IE
 *
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
u8 p2pFuncParseCheckForP2PInfoElem(IN P_ADAPTER_T prAdapter, IN u8 *pucBuf,
                                   OUT u8 *pucOuiType)
{
    u8 aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;
    P_IE_WFA_T prWfaIE = (P_IE_WFA_T)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (pucBuf != NULL) &&
                     (pucOuiType != NULL));

        prWfaIE = (P_IE_WFA_T)pucBuf;

        if (IE_LEN(pucBuf) <= ELEM_MIN_LEN_WFA_OUI_TYPE_SUBTYPE) {
            break;
        } else if (prWfaIE->aucOui[0] != aucWfaOui[0] ||
                   prWfaIE->aucOui[1] != aucWfaOui[1] ||
                   prWfaIE->aucOui[2] != aucWfaOui[2]) {
            break;
        }

        *pucOuiType = prWfaIE->ucOuiType;

        return true;
    } while (false);

    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Probe Request Frame and then return
 *        result to BSS to indicate if need to send the corresponding Probe
 * Response Frame if the specified conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[out] pu4ControlFlags   Control flags for replying the Probe Response
 *
 * @retval true      Reply the Probe Response
 * @retval false     Don't reply the Probe Response
 */
/*----------------------------------------------------------------------------*/
u8 p2pFuncValidateProbeReq(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb,
                           OUT u32 *pu4ControlFlags, IN u8 fgIsDevInterface,
                           IN u8 ucRoleIdx)
{
    u8 fgIsReplyProbeRsp = false;
    u8 fgApplyp2PDevFilter = false;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    DEBUGFUNC("p2pFuncValidateProbeReq");

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prSwRfb != NULL));

        prP2pRoleFsmInfo = prAdapter->rWifiVar.aprP2pRoleFsmInfo[ucRoleIdx];

        /* Process both cases that with amd without add p2p interface */
        if (fgIsDevInterface) {
            fgApplyp2PDevFilter = true;
        } else {
            if (prAdapter->prGlueInfo->prP2PInfo[0]->prDevHandler ==
                prAdapter->prGlueInfo->prP2PInfo[ucRoleIdx]->aprRoleHandler) {
                fgApplyp2PDevFilter = true;
            } else {
                fgApplyp2PDevFilter = false;
            }
        }
        /* TODO: */
        if ((fgApplyp2PDevFilter &&
             (prAdapter->u4OsPacketFilter & PARAM_PACKET_FILTER_PROBE_REQ)) ||
            (!fgApplyp2PDevFilter && (prP2pRoleFsmInfo->u4P2pPacketFilter &
                                      PARAM_PACKET_FILTER_PROBE_REQ))) {
            /* Leave the probe response to p2p_supplicant. */
            kalP2PIndicateRxMgmtFrame(prAdapter->prGlueInfo, prSwRfb,
                                      fgIsDevInterface, ucRoleIdx);
        }
    } while (false);

    return fgIsReplyProbeRsp;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Probe Request Frame and then return
 *        result to BSS to indicate if need to send the corresponding Probe
 * Response Frame if the specified conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[out] pu4ControlFlags   Control flags for replying the Probe Response
 *
 * @retval true      Reply the Probe Response
 * @retval false     Don't reply the Probe Response
 */
/*----------------------------------------------------------------------------*/
void p2pFuncValidateRxActionFrame(IN P_ADAPTER_T prAdapter,
                                  IN P_SW_RFB_T prSwRfb, IN u8 fgIsDevInterface,
                                  IN u8 ucRoleIdx)
{
    u32 u4PacketFilter = 0;
    DEBUGFUNC("p2pFuncValidateRxActionFrame");

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prSwRfb != NULL));

        /* 20181109: frog. upper layer would crash if frame is not
         * registered. */
        if (fgIsDevInterface) {
            if (prAdapter->prGlueInfo->prP2PDevInfo) {
                u4PacketFilter =
                    prAdapter->prGlueInfo->prP2PDevInfo->u4OsMgmtFrameFilter;
            }
        } else {
            P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo =
                prAdapter->rWifiVar.aprP2pRoleFsmInfo[ucRoleIdx];
            if (prP2pRoleFsmInfo) {
                u4PacketFilter = prP2pRoleFsmInfo->u4P2pPacketFilter;
            }
        }

        if (u4PacketFilter & PARAM_PACKET_FILTER_ACTION_FRAME) {
            /* Leave the probe response to p2p_supplicant. */
            kalP2PIndicateRxMgmtFrame(prAdapter->prGlueInfo, prSwRfb,
                                      fgIsDevInterface, ucRoleIdx);
        } else {
            P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;
            struct net_device *prNetdevice = (struct net_device *)NULL;

            if (ucRoleIdx >= BSS_P2P_NUM) {
                break;
            }
            prGlueP2pInfo = prAdapter->prGlueInfo->prP2PInfo[ucRoleIdx];
            if (!prGlueP2pInfo) {
                break;
            }
            if (fgIsDevInterface) {
                prNetdevice = prGlueP2pInfo->prDevHandler;
            } else {
                prNetdevice = prGlueP2pInfo->aprRoleHandler;
            }
            if (prNetdevice  /* && prNetdevice->name >> this comparation are never be NULL (?) */)
            {
                DBGLOG(P2P, WARN,
                       "[%s] unregistered p2p action packet filter 0x%x\n",
                       prNetdevice->name, u4PacketFilter);
            }
        }
    } while (false);

    return;
}

u8 p2pFuncIsAPMode(IN P_P2P_CONNECTION_SETTINGS_T prP2pConnSettings)
{
    if (prP2pConnSettings) {
        if (prP2pConnSettings->fgIsWPSMode == 1) {
            return false;
        }

        return prP2pConnSettings->fgIsApMode;
    } else {
        return false;
    }
}

/* p2pFuncIsAPMode */

void p2pFuncParseBeaconContent(IN P_ADAPTER_T prAdapter,
                               IN P_BSS_INFO_T prP2pBssInfo, IN u8 *pucIEInfo,
                               IN u32 u4IELen)
{
    u8 *pucIE = (u8 *)NULL;
    u16 u2Offset = 0;
    P_P2P_SPECIFIC_BSS_INFO_T prP2pSpecificBssInfo =
        (P_P2P_SPECIFIC_BSS_INFO_T)NULL;
    u8 i = 0;
    RSN_INFO_T rRsnIe;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL));

        if (u4IELen == 0) {
            break;
        }

        prP2pSpecificBssInfo =
            prAdapter->rWifiVar
            .prP2pSpecificBssInfo[prP2pBssInfo->u4PrivateData];
        prP2pSpecificBssInfo->u2AttributeLen = 0;

        ASSERT_BREAK(pucIEInfo != NULL);

        pucIE = pucIEInfo;

        if (prP2pBssInfo->u2CapInfo & CAP_INFO_PRIVACY) {
            kalP2PSetCipher(prAdapter->prGlueInfo, IW_AUTH_CIPHER_WEP40,
                            (u8)prP2pBssInfo->u4PrivateData);
        } else {
            kalP2PSetCipher(prAdapter->prGlueInfo, IW_AUTH_CIPHER_NONE,
                            (u8)prP2pBssInfo->u4PrivateData);
        }

        IE_FOR_EACH(pucIE, u4IELen, u2Offset){
            switch (IE_ID(pucIE)) {
            case ELEM_ID_SSID:  /* 0 */ /* V */ /* Done */
            {
                /* DBGLOG(P2P, TRACE, ("SSID update\n")); */
                /* SSID is saved when start AP/GO */
                /* SSID IE set in beacon from supplicant will
                 * not always be the true since hidden SSID case
                 */
            } break;

            case ELEM_ID_SUP_RATES:  /* 1 */ /* V */ /* Done */
            {
#ifndef CFG_SUPPORT_P2P_GO_KEEP_RATE_SETTING
                DBGLOG(P2P, TRACE, "Support Rate IE\n");
                if ((SUP_RATES_IE(pucIE)->ucLength) > ELEM_MAX_LEN_SUP_RATES) {
                    SUP_RATES_IE(pucIE)->ucLength = ELEM_MAX_LEN_SUP_RATES;
                }
                kalMemCopy(prP2pBssInfo->aucAllSupportedRates,
                           SUP_RATES_IE(pucIE)->aucSupportedRates,
                           SUP_RATES_IE(pucIE)->ucLength);
                prP2pBssInfo->ucAllSupportedRatesLen =
                    SUP_RATES_IE(pucIE)->ucLength;
                DBGLOG_MEM8(P2P, TRACE, SUP_RATES_IE(pucIE)->aucSupportedRates,
                            SUP_RATES_IE(pucIE)->ucLength);
#endif
            } break;

            case ELEM_ID_DS_PARAM_SET:  /* 3 */ /* V */ /* Done */
            {
                DBGLOG(P2P, TRACE, "DS PARAM IE: %d.\n",
                       DS_PARAM_IE(pucIE)->ucCurrChnl);

                /* prP2pBssInfo->ucPrimaryChannel =
                 * DS_PARAM_IE(pucIE)->ucCurrChnl; */

                /* prP2pBssInfo->eBand = BAND_2G4; */
            } break;

            case ELEM_ID_TIM:  /* 5 */ /* V */
                TIM_IE(pucIE)->ucDTIMPeriod = prP2pBssInfo->ucDTIMPeriod;
                DBGLOG(P2P, TRACE, "TIM IE, Len:%d, DTIM:%d\n", IE_LEN(pucIE),
                       TIM_IE(pucIE)->ucDTIMPeriod);
                break;

            case ELEM_ID_ERP_INFO:  /* 42 */ /* V */
            {
                /* This IE would dynamic change due to FW
                 * detection change is required. */
                DBGLOG(P2P, TRACE, "ERP IE will be over write by driver\n");
                DBGLOG(P2P, TRACE, " ucERP: %x.\n", ERP_INFO_IE(pucIE)->ucERP);
            } break;

            case ELEM_ID_HT_CAP:  /* 45 */ /* V */
            {
                DBGLOG(P2P, TRACE,
                       "HT CAP IE would be overwritten by driver\n");

                DBGLOG(P2P, TRACE, "HT Cap Info:%x, AMPDU Param:%x\n",
                       HT_CAP_IE(pucIE)->u2HtCapInfo,
                       HT_CAP_IE(pucIE)->ucAmpduParam);

                DBGLOG(
                    P2P, TRACE,
                    "HT Extended Cap:%x, TX Beamforming Cap:%lx, Ant Selection "
                    "Cap:%x\n",
                    HT_CAP_IE(pucIE)->u2HtExtendedCap,
                    HT_CAP_IE(pucIE)->u4TxBeamformingCap,
                    HT_CAP_IE(pucIE)->ucAselCap);
            } break;

            case ELEM_ID_RSN:  /* 48 */ /* V */

                DBGLOG(P2P, TRACE, "RSN IE\n");
                kalP2PSetCipher(prAdapter->prGlueInfo, IW_AUTH_CIPHER_CCMP,
                                (u8)prP2pBssInfo->u4PrivateData);

                if (rsnParseRsnIE(prAdapter, RSN_IE(pucIE), &rRsnIe)) {
                    prP2pBssInfo->u4RsnSelectedGroupCipher =
                        RSN_CIPHER_SUITE_CCMP;
                    prP2pBssInfo->u4RsnSelectedPairwiseCipher =
                        RSN_CIPHER_SUITE_CCMP;
                    prP2pBssInfo->u4RsnSelectedAKMSuite = RSN_AKM_SUITE_PSK;
                    prP2pBssInfo->u2RsnSelectedCapInfo = rRsnIe.u2RsnCap;
                    prAdapter->prGlueInfo->rWpaInfo.ucRsneLen =
                        rRsnIe.ucRsneLen;
                    DBGLOG(RSN, TRACE, "RsnIe CAP:0x%x\n", rRsnIe.u2RsnCap);
                }

#if CFG_SUPPORT_802_11W
                /* AP PMF */
                prP2pBssInfo->rApPmfCfg.fgMfpc =
                    (rRsnIe.u2RsnCap & ELEM_WPA_CAP_MFPC) ? 1 : 0;
                prP2pBssInfo->rApPmfCfg.fgMfpr =
                    (rRsnIe.u2RsnCap & ELEM_WPA_CAP_MFPR) ? 1 : 0;

                for (i = 0; i < rRsnIe.u4AuthKeyMgtSuiteCount; i++) {
                    if ((rRsnIe.au4AuthKeyMgtSuite[i] ==
                         RSN_AKM_SUITE_PSK_SHA256) ||
                        (rRsnIe.au4AuthKeyMgtSuite[i] ==
                         RSN_AKM_SUITE_802_1X_SHA256)) {
                        DBGLOG(RSN, INFO, "SHA256 support\n");
                        /* over-write
                         * u4RsnSelectedAKMSuite by
                         * SHA256 AKM */
                        prP2pBssInfo->u4RsnSelectedAKMSuite =
                            rRsnIe.au4AuthKeyMgtSuite[i];
                        prP2pBssInfo->rApPmfCfg.fgSha256 = true;
                        break;
                    }
                }
                DBGLOG_RATELIMIT(RSN, INFO, "bcn mfpc:%d, mfpr:%d, sha256:%d\n",
                                 prP2pBssInfo->rApPmfCfg.fgMfpc,
                                 prP2pBssInfo->rApPmfCfg.fgMfpr,
                                 prP2pBssInfo->rApPmfCfg.fgSha256);
#endif

                break;

            case ELEM_ID_EXTENDED_SUP_RATES:  /* 50 */ /* V */
                /* ELEM_ID_SUP_RATES should be placed before
                 * ELEM_ID_EXTENDED_SUP_RATES. */
#ifndef CFG_SUPPORT_P2P_GO_KEEP_RATE_SETTING
                DBGLOG(P2P, TRACE, "Ex Support Rate IE\n");
                kalMemCopy(&(prP2pBssInfo->aucAllSupportedRates
                             [prP2pBssInfo->ucAllSupportedRatesLen]),
                           EXT_SUP_RATES_IE(pucIE)->aucExtSupportedRates,
                           EXT_SUP_RATES_IE(pucIE)->ucLength);

                DBGLOG_MEM8(P2P, TRACE,
                            EXT_SUP_RATES_IE(pucIE)->aucExtSupportedRates,
                            EXT_SUP_RATES_IE(pucIE)->ucLength);

                prP2pBssInfo->ucAllSupportedRatesLen +=
                    EXT_SUP_RATES_IE(pucIE)->ucLength;
#endif
                break;

            case ELEM_ID_HT_OP:
                /* 61 */ /* V */ /* TODO: */
            {
                DBGLOG(P2P, TRACE,
                       "HT OP IE would be overwritten by driver\n");

                DBGLOG(
                    P2P, TRACE,
                    " Primary Channel: %x, Info1: %x, Info2: %x, Info3: %x\n",
                    HT_OP_IE(pucIE)->ucPrimaryChannel,
                    HT_OP_IE(pucIE)->ucInfo1, HT_OP_IE(pucIE)->u2Info2,
                    HT_OP_IE(pucIE)->u2Info3);
            }
            break;

            case ELEM_ID_OBSS_SCAN_PARAMS:  /* 74 */ /* V */
            {
                DBGLOG(
                    P2P, TRACE,
                    "ELEM_ID_OBSS_SCAN_PARAMS IE would be replaced by driver\n");
            } break;

            case ELEM_ID_EXTENDED_CAP:  /* 127 */ /* V */
            {
                DBGLOG(P2P, TRACE,
                       "ELEM_ID_EXTENDED_CAP IE would be replaced by driver\n");
            } break;

            case ELEM_ID_VENDOR:  /* 221 */ /* V */
                DBGLOG(P2P, TRACE, "Vender Specific IE\n");
                {
                    p2pFuncParseBeaconVenderId(prAdapter, pucIE,
                                               prP2pSpecificBssInfo,
                                               (u8)prP2pBssInfo->u4PrivateData);
                    /* TODO: Store other Vender IE except
                     * for WMM Param. */
                }
                break;

            default:
                DBGLOG(P2P, TRACE, "Unprocessed element ID:%d\n", IE_ID(pucIE));
                break;
            }
        }
    } while (false);
}

/* Code refactoring for AOSP */
static void
p2pFuncParseBeaconVenderId(IN P_ADAPTER_T prAdapter, IN u8 *pucIE,
                           IN P_P2P_SPECIFIC_BSS_INFO_T prP2pSpecificBssInfo,
                           IN u8 ucRoleIndex)
{
    do {
        u8 ucOuiType;
        u16 u2SubTypeVersion;

        if (rsnParseCheckForWFAInfoElem(prAdapter, pucIE, &ucOuiType,
                                        &u2SubTypeVersion)) {
            if ((ucOuiType == VENDOR_OUI_TYPE_WPA) &&
                (u2SubTypeVersion == VERSION_WPA)) {
                if (IE_SIZE(pucIE) > (ELEM_HDR_LEN + ELEM_MAX_LEN_WPA)) {
                    DBGLOG(P2P, ERROR,
                           "wpa type only max 36 bytes but %d received\n",
                           IE_SIZE(pucIE));
                    ASSERT(false);
                    break;
                }
                if (!kalP2PGetCcmpCipher(prAdapter->prGlueInfo, ucRoleIndex)) {
                    kalP2PSetCipher(prAdapter->prGlueInfo, IW_AUTH_CIPHER_TKIP,
                                    ucRoleIndex);
                }
                kalMemCopy(prP2pSpecificBssInfo->aucWpaIeBuffer, pucIE,
                           IE_SIZE(pucIE));
                prP2pSpecificBssInfo->u2WpaIeLen = IE_SIZE(pucIE);
                DBGLOG(P2P, TRACE, "WPA IE in supplicant\n");
            } else if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
                kalP2PUpdateWSC_IE(prAdapter->prGlueInfo, 0, pucIE,
                                   IE_SIZE(pucIE), ucRoleIndex);
                DBGLOG(P2P, TRACE, "WPS IE in supplicant\n");
            } else if (ucOuiType == VENDOR_OUI_TYPE_WMM) {
                DBGLOG(P2P, TRACE, "WMM IE in supplicant\n");
            }
            /* WMM here. */
        } else if (p2pFuncParseCheckForP2PInfoElem(prAdapter, pucIE,
                                                   &ucOuiType)) {
            /* TODO Store the whole P2P IE & generate later. */
            /* Be aware that there may be one or more P2P IE. */
            if (prP2pSpecificBssInfo->u2AttributeLen + IE_SIZE(pucIE) >
                P2P_MAXIMUM_ATTRIBUTES_CACHE_SIZE) {
                DBGLOG(P2P, ERROR,
                       "aucAttributesCache only 768 bytes but received more\n");
                ASSERT(false);
                break;
            }
            if (ucOuiType == VENDOR_OUI_TYPE_P2P) {
                kalMemCopy(&prP2pSpecificBssInfo->aucAttributesCache
                           [prP2pSpecificBssInfo->u2AttributeLen],
                           pucIE, IE_SIZE(pucIE));
                prP2pSpecificBssInfo->u2AttributeLen += IE_SIZE(pucIE);
                DBGLOG(P2P, TRACE, "P2P IE in supplicant\n");
            } else if (ucOuiType == VENDOR_OUI_TYPE_WFD) {
                kalMemCopy(&prP2pSpecificBssInfo->aucAttributesCache
                           [prP2pSpecificBssInfo->u2AttributeLen],
                           pucIE, IE_SIZE(pucIE));

                prP2pSpecificBssInfo->u2AttributeLen += IE_SIZE(pucIE);
            } else {
                DBGLOG(P2P, TRACE, "Unknown 50-6F-9A-%d IE.\n", ucOuiType);
            }
        } else {
            if (prP2pSpecificBssInfo->u2AttributeLen + IE_SIZE(pucIE) >
                P2P_MAXIMUM_ATTRIBUTES_CACHE_SIZE) {
                DBGLOG(P2P, ERROR,
                       "aucAttributesCache only 768 bytes but received more\n");
                ASSERT(false);
                break;
            }
            kalMemCopy(
                &prP2pSpecificBssInfo
                ->aucAttributesCache[prP2pSpecificBssInfo->u2AttributeLen],
                pucIE, IE_SIZE(pucIE));

            prP2pSpecificBssInfo->u2AttributeLen += IE_SIZE(pucIE);
            DBGLOG(P2P, TRACE, "Driver unprocessed Vender Specific IE\n");
            ASSERT(false);
        }
    } while (0);
}

P_BSS_DESC_T
p2pFuncKeepOnConnection(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo,
                        IN P_P2P_CONNECTION_REQ_INFO_T prConnReqInfo,
                        IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo,
                        IN P_P2P_SCAN_REQ_INFO_T prScanReqInfo)
{
    P_BSS_DESC_T prTargetBss = (P_BSS_DESC_T)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prBssInfo != NULL) &&
                     (prConnReqInfo != NULL) && (prChnlReqInfo != NULL) &&
                     (prScanReqInfo != NULL));

        if (prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE) {
            break;
        }
        /* Update connection request information. */
        ASSERT(prConnReqInfo->eConnRequest == P2P_CONNECTION_TYPE_GC);

        /* Find BSS Descriptor first. */
        prTargetBss = scanP2pSearchDesc(prAdapter, prConnReqInfo);

        if (prTargetBss == NULL) {
            /* Update scan parameter... to scan target device. */
            /* TODO: Need refine. */
            prScanReqInfo->ucNumChannelList = 1;
            prScanReqInfo->eScanType = SCAN_TYPE_ACTIVE_SCAN;
            prScanReqInfo->eChannelSet = SCAN_CHANNEL_FULL;
            prScanReqInfo->u4BufLength = 0;  /* Prevent other P2P ID
                                              * in IE. */
            prScanReqInfo->fgIsAbort = true;
        } else {
            prChnlReqInfo->u8Cookie = 0;
            prChnlReqInfo->ucReqChnlNum = prTargetBss->ucChannelNum;
            prChnlReqInfo->eBand = prTargetBss->eBand;
            prChnlReqInfo->eChnlSco = prTargetBss->eSco;
            prChnlReqInfo->u4MaxInterval = AIS_JOIN_CH_REQUEST_INTERVAL;
            prChnlReqInfo->eChnlReqType = CH_REQ_TYPE_JOIN;

            prChnlReqInfo->eChannelWidth = prTargetBss->eChannelWidth;
            prChnlReqInfo->ucCenterFreqS1 = prTargetBss->ucCenterFreqS1;
            prChnlReqInfo->ucCenterFreqS2 = prTargetBss->ucCenterFreqS2;
        }
    } while (false);

    return prTargetBss;
}

/* Currently Only for ASSOC Response Frame. */
void p2pFuncStoreAssocRspIEBuffer(IN P_ADAPTER_T prAdapter,
                                  IN P_P2P_JOIN_INFO_T prP2pJoinInfo,
                                  IN P_SW_RFB_T prSwRfb)
{
    P_WLAN_ASSOC_RSP_FRAME_T prAssocRspFrame = (P_WLAN_ASSOC_RSP_FRAME_T)NULL;
    s16 i2IELen = 0;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prP2pJoinInfo != NULL) &&
                     (prSwRfb != NULL));

        prAssocRspFrame = (P_WLAN_ASSOC_RSP_FRAME_T)prSwRfb->pvHeader;

        if (prAssocRspFrame->u2FrameCtrl != MAC_FRAME_ASSOC_RSP) {
            break;
        }

        i2IELen =
            prSwRfb->u2PacketLen - (WLAN_MAC_HEADER_LEN + CAP_INFO_FIELD_LEN +
                                    STATUS_CODE_FIELD_LEN + AID_FIELD_LEN);

        if (i2IELen <= 0) {
            break;
        }

        prP2pJoinInfo->u4BufLength = (u32)i2IELen;

        kalMemCopy(prP2pJoinInfo->aucIEBuf, prAssocRspFrame->aucInfoElem,
                   prP2pJoinInfo->u4BufLength);
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set Packet Filter.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 * set. \param[in] u4SetBufferLen The length of the set buffer. \param[out]
 * pu4SetInfoLen If the call is successful, returns the number of bytes read
 * from the set buffer. If the call failed due to invalid length of the set
 * buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_NOT_SUPPORTED
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
void p2pFuncMgmtFrameRegister(IN P_ADAPTER_T prAdapter, IN u16 u2FrameType,
                              IN u8 fgIsRegistered,
                              OUT u32 *pu4P2pPacketFilter)
{
    u32 u4NewPacketFilter = 0;
    CMD_RX_PACKET_FILTER rSetRxPacketFilter;

    DEBUGFUNC("p2pFuncMgmtFrameRegister");

    do {
        ASSERT_BREAK(prAdapter != NULL);

        if (pu4P2pPacketFilter) {
            u4NewPacketFilter = *pu4P2pPacketFilter;
        }

        switch (u2FrameType) {
        case MAC_FRAME_PROBE_REQ:
            if (fgIsRegistered) {
                u4NewPacketFilter |= PARAM_PACKET_FILTER_PROBE_REQ;
                DBGLOG(P2P, TRACE, "Open packet filer probe request\n");
            } else {
                u4NewPacketFilter &= ~PARAM_PACKET_FILTER_PROBE_REQ;
                DBGLOG(P2P, TRACE, "Close packet filer probe request\n");
            }
            break;

        case MAC_FRAME_ACTION:
            if (fgIsRegistered) {
                u4NewPacketFilter |= PARAM_PACKET_FILTER_ACTION_FRAME;
                DBGLOG(P2P, TRACE, "Open packet filer action frame.\n");
            } else {
                u4NewPacketFilter &= ~PARAM_PACKET_FILTER_ACTION_FRAME;
                DBGLOG(P2P, TRACE, "Close packet filer action frame.\n");
            }
            break;

        default:
            DBGLOG(P2P, TRACE, "unsupported frame type:%x\n", u2FrameType);
            break;
        }

        if (pu4P2pPacketFilter) {
            *pu4P2pPacketFilter = u4NewPacketFilter;
        }

        /* u4NewPacketFilter |= prAdapter->u4OsPacketFilter; */

        prAdapter->u4OsPacketFilter &= ~PARAM_PACKET_FILTER_P2P_MASK;
        prAdapter->u4OsPacketFilter |= u4NewPacketFilter;

        DBGLOG(P2P, TRACE, "P2P Set PACKET filter:0x%lx\n",
               prAdapter->u4OsPacketFilter);

        kalMemZero(&rSetRxPacketFilter, sizeof(rSetRxPacketFilter));
        rSetRxPacketFilter.u4RxPacketFilter = prAdapter->u4OsPacketFilter;

        wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_RX_FILTER, true, false, false,
                            nicCmdEventSetCommon, nicOidCmdTimeoutCommon,
                            sizeof(CMD_RX_PACKET_FILTER),
                            (u8 *)&rSetRxPacketFilter, &u4NewPacketFilter,
                            sizeof(u4NewPacketFilter));

        prAdapter->u4OsPacketFilter = rSetRxPacketFilter.u4RxPacketFilter;
    } while (false);
}

void p2pFuncUpdateMgmtFrameRegister(IN P_ADAPTER_T prAdapter,
                                    IN u32 u4OsFilter)
{
    CMD_RX_PACKET_FILTER rSetRxPacketFilter;

    do {
        /* TODO: Filter need to be done. */
        /* prAdapter->rWifiVar.prP2pFsmInfo->u4P2pPacketFilter =
         * u4OsFilter; */

        if ((prAdapter->u4OsPacketFilter & PARAM_PACKET_FILTER_P2P_MASK) ^
            u4OsFilter) {
            prAdapter->u4OsPacketFilter &= ~PARAM_PACKET_FILTER_P2P_MASK;

            prAdapter->u4OsPacketFilter |=
                (u4OsFilter & PARAM_PACKET_FILTER_P2P_MASK);

            kalMemZero(&rSetRxPacketFilter, sizeof(rSetRxPacketFilter));
            rSetRxPacketFilter.u4RxPacketFilter = prAdapter->u4OsPacketFilter;

            wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_RX_FILTER, true, false,
                                false, nicCmdEventSetCommon,
                                nicOidCmdTimeoutCommon,
                                sizeof(CMD_RX_PACKET_FILTER),
                                (u8 *)&rSetRxPacketFilter, &u4OsFilter,
                                sizeof(u4OsFilter));

            prAdapter->u4OsPacketFilter = rSetRxPacketFilter.u4RxPacketFilter;
            DBGLOG(P2P, TRACE, "P2P Set PACKET filter:0x%lx\n",
                   prAdapter->u4OsPacketFilter);
        }
    } while (false);
}

void p2pFuncGetStationInfo(IN P_ADAPTER_T prAdapter, IN u8 *pucMacAddr,
                           OUT P_P2P_STATION_INFO_T prStaInfo)
{
    do {
        ASSERT_BREAK((prAdapter != NULL) && (pucMacAddr != NULL) &&
                     (prStaInfo != NULL));

        prStaInfo->u4InactiveTime = 0;
        prStaInfo->u4RxBytes = 0;
        prStaInfo->u4TxBytes = 0;
        prStaInfo->u4RxPackets = 0;
        prStaInfo->u4TxPackets = 0;
        /* TODO: */
    } while (false);
}

P_MSDU_INFO_T p2pFuncProcessP2pProbeRsp(IN P_ADAPTER_T prAdapter,
                                        IN u8 ucBssIdx,
                                        IN P_MSDU_INFO_T prMgmtTxMsdu)
{
    P_MSDU_INFO_T prRetMsduInfo = prMgmtTxMsdu;
    P_WLAN_PROBE_RSP_FRAME_T prProbeRspFrame = (P_WLAN_PROBE_RSP_FRAME_T)NULL;
    u8 *pucIEBuf = (u8 *)NULL;
    u16 u2Offset = 0, u2IELength = 0, u2ProbeRspHdrLen = 0;
    u8 fgIsP2PIE = false, fgIsWSCIE = false;
    u8 fgIsWFDIE = false;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    u16 u2EstimateSize = 0, u2EstimatedExtraIELen = 0;
    u32 u4IeArraySize = 0, u4Idx = 0;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prMgmtTxMsdu != NULL));

        prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

        /* 3 Make sure this is probe response frame. */
        prProbeRspFrame =
            (P_WLAN_PROBE_RSP_FRAME_T)((unsigned long)prMgmtTxMsdu->prPacket +
                                       MAC_TX_RESERVED_FIELD);
        ASSERT_BREAK((prProbeRspFrame->u2FrameCtrl & MASK_FRAME_TYPE) ==
                     MAC_FRAME_PROBE_RSP);

        /* 3 Get the importent P2P IE. */
        u2ProbeRspHdrLen = (WLAN_MAC_MGMT_HEADER_LEN + TIMESTAMP_FIELD_LEN +
                            BEACON_INTERVAL_FIELD_LEN + CAP_INFO_FIELD_LEN);
        pucIEBuf = prProbeRspFrame->aucInfoElem;
        u2IELength = prMgmtTxMsdu->u2FrameLength - u2ProbeRspHdrLen;

#if CFG_SUPPORT_WFD
        /* prAdapter->prGlueInfo->prP2PInfo[0]->u2VenderIELen = 0; */
        /* Reset in each time ?? */
        prAdapter->prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData]
        ->u2WFDIELen = 0;
#endif

        IE_FOR_EACH(pucIEBuf, u2IELength, u2Offset){
            switch (IE_ID(pucIEBuf)) {
            case ELEM_ID_SSID: {
                p2pFuncProcessP2pProbeRspAction(prAdapter, pucIEBuf,
                                                ELEM_ID_SSID, &ucBssIdx,
                                                &prP2pBssInfo, &fgIsWSCIE,
                                                &fgIsP2PIE, &fgIsWFDIE);
            } break;

            case ELEM_ID_VENDOR: {
                p2pFuncProcessP2pProbeRspAction(prAdapter, pucIEBuf,
                                                ELEM_ID_VENDOR, &ucBssIdx,
                                                &prP2pBssInfo, &fgIsWSCIE,
                                                &fgIsP2PIE, &fgIsWFDIE);
            } break;

            default:
                break;
            }
        }

        /* 3 Check the total size & current frame. */
        u2EstimateSize = WLAN_MAC_MGMT_HEADER_LEN + TIMESTAMP_FIELD_LEN +
                         BEACON_INTERVAL_FIELD_LEN + CAP_INFO_FIELD_LEN +
                         (ELEM_HDR_LEN + ELEM_MAX_LEN_SSID) +
                         (ELEM_HDR_LEN + ELEM_MAX_LEN_SUP_RATES) +
                         (ELEM_HDR_LEN + ELEM_MAX_LEN_DS_PARAMETER_SET);

        u2EstimatedExtraIELen = 0;

        u4IeArraySize =
            sizeof(txProbeRspIETable) / sizeof(APPEND_VAR_IE_ENTRY_T);
        for (u4Idx = 0; u4Idx < u4IeArraySize; u4Idx++) {
            if (txProbeRspIETable[u4Idx].u2EstimatedFixedIELen) {
                u2EstimatedExtraIELen +=
                    txProbeRspIETable[u4Idx].u2EstimatedFixedIELen;
            } else {
                ASSERT(txProbeRspIETable[u4Idx].pfnCalculateVariableIELen);

                u2EstimatedExtraIELen +=
                    (u16)(txProbeRspIETable[u4Idx].pfnCalculateVariableIELen(
                              prAdapter, ucBssIdx, NULL));
            }
        }

        if (fgIsWSCIE) {
            u2EstimatedExtraIELen += kalP2PCalWSC_IELen(
                prAdapter->prGlueInfo, 2, (u8)prP2pBssInfo->u4PrivateData);
        }

        if (fgIsP2PIE) {
            u2EstimatedExtraIELen += kalP2PCalWSC_IELen(
                prAdapter->prGlueInfo, 1, (u8)prP2pBssInfo->u4PrivateData);
            u2EstimatedExtraIELen +=
                p2pFuncCalculateP2P_IE_NoA(prAdapter, ucBssIdx, NULL);
        }
#if CFG_SUPPORT_WFD
        ASSERT(
            sizeof(prAdapter->prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData]
                   ->aucWFDIE) >=
            prAdapter->prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData]
            ->u2WFDIELen);
        if (fgIsWFDIE) {
            u2EstimatedExtraIELen +=
                prAdapter->prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData]
                ->u2WFDIELen;
        }
#endif

        u2EstimateSize += u2EstimatedExtraIELen;
        if ((u2EstimateSize) > (prRetMsduInfo->u2FrameLength)) {
            prRetMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimateSize);

            if (prRetMsduInfo == NULL) {
                DBGLOG(
                    P2P, WARN,
                    "No packet for sending new probe response, use original one\n");
                prRetMsduInfo = prMgmtTxMsdu;
                break;
            }
        }

        prRetMsduInfo->ucBssIndex = ucBssIdx;

        /* 3 Compose / Re-compose probe response frame. */
        bssComposeBeaconProbeRespFrameHeaderAndFF(
            (u8 *)((unsigned long)(prRetMsduInfo->prPacket) +
                   MAC_TX_RESERVED_FIELD),
            prProbeRspFrame->aucDestAddr, prProbeRspFrame->aucSrcAddr,
            prProbeRspFrame->aucBSSID, prProbeRspFrame->u2BeaconInterval,
            prProbeRspFrame->u2CapInfo);

        prRetMsduInfo->u2FrameLength =
            (WLAN_MAC_MGMT_HEADER_LEN + TIMESTAMP_FIELD_LEN +
             BEACON_INTERVAL_FIELD_LEN + CAP_INFO_FIELD_LEN);

        bssBuildBeaconProbeRespFrameCommonIEs(prRetMsduInfo, prP2pBssInfo,
                                              prProbeRspFrame->aucDestAddr);

        prRetMsduInfo->ucStaRecIndex = prMgmtTxMsdu->ucStaRecIndex;

        for (u4Idx = 0; u4Idx < u4IeArraySize; u4Idx++) {
            if (txProbeRspIETable[u4Idx].pfnAppendIE) {
                txProbeRspIETable[u4Idx].pfnAppendIE(prAdapter, prRetMsduInfo);
            }
        }

        if (fgIsWSCIE) {
            kalP2PGenWSC_IE(prAdapter->prGlueInfo, 2,
                            (u8 *)((unsigned long)prRetMsduInfo->prPacket +
                                   (unsigned long)prRetMsduInfo->u2FrameLength),
                            (u8)prP2pBssInfo->u4PrivateData);

            prRetMsduInfo->u2FrameLength += (u16)kalP2PCalWSC_IELen(
                prAdapter->prGlueInfo, 2, (u8)prP2pBssInfo->u4PrivateData);
        }

        if (fgIsP2PIE) {
            kalP2PGenWSC_IE(prAdapter->prGlueInfo, 1,
                            (u8 *)((unsigned long)prRetMsduInfo->prPacket +
                                   (unsigned long)prRetMsduInfo->u2FrameLength),
                            (u8)prP2pBssInfo->u4PrivateData);

            prRetMsduInfo->u2FrameLength += (u16)kalP2PCalWSC_IELen(
                prAdapter->prGlueInfo, 1, (u8)prP2pBssInfo->u4PrivateData);
            p2pFuncGenerateP2P_IE_NoA(prAdapter, prRetMsduInfo);
        }
#if CFG_SUPPORT_WFD
        if (fgIsWFDIE > 0) {
            ASSERT(prAdapter->prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData]
                   ->u2WFDIELen > 0);
            kalMemCopy(
                (u8 *)((unsigned long)prRetMsduInfo->prPacket +
                       (unsigned long)prRetMsduInfo->u2FrameLength),
                prAdapter->prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData]
                ->aucWFDIE,
                prAdapter->prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData]
                ->u2WFDIELen);
            prRetMsduInfo->u2FrameLength +=
                (u16)prAdapter->prGlueInfo
                ->prP2PInfo[prP2pBssInfo->u4PrivateData]
                ->u2WFDIELen;
        }
#endif
    } while (false);

    if (prRetMsduInfo != prMgmtTxMsdu) {
        cnmMgtPktFree(prAdapter, prMgmtTxMsdu);
    }

    return prRetMsduInfo;
}

/* Code refactoring for AOSP */
static void p2pFuncProcessP2pProbeRspAction(
    IN P_ADAPTER_T prAdapter, IN u8 *pucIEBuf, IN u8 ucElemIdType,
    OUT u8 *ucBssIdx, OUT P_BSS_INFO_T *prP2pBssInfo, OUT u8 *fgIsWSCIE,
    OUT u8 *fgIsP2PIE, OUT u8 *fgIsWFDIE)
{
    u8 ucOuiType = 0;
    u16 u2SubTypeVersion = 0;

    switch (ucElemIdType) {
    case ELEM_ID_SSID: {
        if (SSID_IE(pucIEBuf)->ucLength > 7) {
            for ((*ucBssIdx) = 0; (*ucBssIdx) < MAX_BSS_INDEX; (*ucBssIdx)++) {
                *prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, *ucBssIdx);
                if (!(*prP2pBssInfo)) {
                    continue;
                }
                if (EQUAL_SSID((*prP2pBssInfo)->aucSSID,
                               (*prP2pBssInfo)->ucSSIDLen,
                               SSID_IE(pucIEBuf)->aucSSID,
                               SSID_IE(pucIEBuf)->ucLength)) {
                    break;
                }
            }
            if ((*ucBssIdx) == P2P_DEV_BSS_INDEX) {
                *prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, *ucBssIdx);
            }
        } else {
            *prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, P2P_DEV_BSS_INDEX);
            COPY_SSID((*prP2pBssInfo)->aucSSID, (*prP2pBssInfo)->ucSSIDLen,
                      SSID_IE(pucIEBuf)->aucSSID, SSID_IE(pucIEBuf)->ucLength);
        }
    } break;

    case ELEM_ID_VENDOR:
        if (rsnParseCheckForWFAInfoElem(prAdapter, pucIEBuf, &ucOuiType,
                                        &u2SubTypeVersion)) {
            if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
                kalP2PUpdateWSC_IE(
                    prAdapter->prGlueInfo, 2, pucIEBuf, IE_SIZE(pucIEBuf),
                    (u8)((P_BSS_INFO_T)*prP2pBssInfo)->u4PrivateData);
                *fgIsWSCIE = true;
            }
        } else if (p2pFuncParseCheckForP2PInfoElem(prAdapter, pucIEBuf,
                                                   &ucOuiType)) {
            if (ucOuiType == VENDOR_OUI_TYPE_P2P) {
                /* 2 Note(frog): I use WSC IE buffer for Probe
                 * Request to store the P2P IE for Probe
                 * Response.
                 */
                kalP2PUpdateWSC_IE(
                    prAdapter->prGlueInfo, 1, pucIEBuf, IE_SIZE(pucIEBuf),
                    (u8)((P_BSS_INFO_T)*prP2pBssInfo)->u4PrivateData);
                *fgIsP2PIE = true;
            }
#if CFG_SUPPORT_WFD
            else if (ucOuiType == VENDOR_OUI_TYPE_WFD) {
                DBGLOG(P2P, INFO,
                       "WFD IE is found in probe resp (supp). Len %u\n",
                       IE_SIZE(pucIEBuf));
                if ((sizeof(prAdapter->prGlueInfo
                            ->prP2PInfo[((P_BSS_INFO_T)*prP2pBssInfo)
                                        ->u4PrivateData]
                            ->aucWFDIE) >=
                     (prAdapter->prGlueInfo
                      ->prP2PInfo[((P_BSS_INFO_T)*prP2pBssInfo)
                                  ->u4PrivateData]
                      ->u2WFDIELen +
                      IE_SIZE(pucIEBuf)))) {
                    *fgIsWFDIE = true;
                    kalMemCopy(prAdapter->prGlueInfo
                               ->prP2PInfo[((P_BSS_INFO_T)*prP2pBssInfo)
                                           ->u4PrivateData]
                               ->aucWFDIE,
                               pucIEBuf, IE_SIZE(pucIEBuf));
                    prAdapter->prGlueInfo
                    ->prP2PInfo[((P_BSS_INFO_T)*prP2pBssInfo)->u4PrivateData]
                    ->u2WFDIELen += IE_SIZE(pucIEBuf);
                }
            }  /*  VENDOR_OUI_TYPE_WFD */
#endif
        } else {
            DBGLOG(P2P, INFO,
                   "Other vender IE is found in probe resp (supp). Len %u\n",
                   IE_SIZE(pucIEBuf));
        }
        break;

    default:
        break;
    }
}

u32 p2pFuncCalculateP2p_IELenForBeacon(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx,
                                       IN P_STA_RECORD_T prStaRec)
{
    P_P2P_SPECIFIC_BSS_INFO_T prP2pSpeBssInfo = (P_P2P_SPECIFIC_BSS_INFO_T)NULL;
    u32 u4IELen = 0;
    P_BSS_INFO_T prBssInfo;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (ucBssIdx < BSS_INFO_NUM));

        prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

        if (!prAdapter->fgIsP2PRegistered) {
            break;
        }

        if (p2pFuncIsAPMode(prAdapter->rWifiVar
                            .prP2PConnSettings[prBssInfo->u4PrivateData])) {
            break;
        }

        prP2pSpeBssInfo =
            prAdapter->rWifiVar.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];

        u4IELen = prP2pSpeBssInfo->u2AttributeLen;
    } while (false);

    return u4IELen;
}

void p2pFuncGenerateP2p_IEForBeacon(IN P_ADAPTER_T prAdapter,
                                    IN P_MSDU_INFO_T prMsduInfo)
{
    P_P2P_SPECIFIC_BSS_INFO_T prP2pSpeBssInfo = (P_P2P_SPECIFIC_BSS_INFO_T)NULL;
    u8 *pucIEBuf = (u8 *)NULL;
    P_BSS_INFO_T prBssInfo;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prMsduInfo != NULL));

        if (!prAdapter->fgIsP2PRegistered) {
            break;
        }

        prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];

        prP2pSpeBssInfo =
            prAdapter->rWifiVar.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];

        if (p2pFuncIsAPMode(prAdapter->rWifiVar
                            .prP2PConnSettings[prBssInfo->u4PrivateData])) {
            break;
        }

        pucIEBuf = (u8 *)((unsigned long)prMsduInfo->prPacket +
                          (unsigned long)prMsduInfo->u2FrameLength);

        kalMemCopy(pucIEBuf, prP2pSpeBssInfo->aucAttributesCache,
                   prP2pSpeBssInfo->u2AttributeLen);

        prMsduInfo->u2FrameLength += prP2pSpeBssInfo->u2AttributeLen;
    } while (false);
}

u32 p2pFuncCalculateWSC_IELenForBeacon(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx,
                                       IN P_STA_RECORD_T prStaRec)
{
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;

    prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

    if (prP2pBssInfo->eNetworkType != NETWORK_TYPE_P2P) {
        return 0;
    }

    return kalP2PCalWSC_IELen(prAdapter->prGlueInfo, 0,
                              (u8)prP2pBssInfo->u4PrivateData);
}

void p2pFuncGenerateWSC_IEForBeacon(IN P_ADAPTER_T prAdapter,
                                    IN P_MSDU_INFO_T prMsduInfo)
{
    u8 *pucBuffer;
    u16 u2IELen = 0;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;

    ASSERT(prAdapter);
    ASSERT(prMsduInfo);

    prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);

    if (prP2pBssInfo->eNetworkType != NETWORK_TYPE_P2P) {
        return;
    }

    u2IELen = (u16)kalP2PCalWSC_IELen(prAdapter->prGlueInfo, 0,
                                      (u8)prP2pBssInfo->u4PrivateData);

    pucBuffer = (u8 *)((unsigned long)prMsduInfo->prPacket +
                       (unsigned long)prMsduInfo->u2FrameLength);

    ASSERT(pucBuffer);

    /* TODO: Check P2P FSM State. */
    kalP2PGenWSC_IE(prAdapter->prGlueInfo, 0, pucBuffer,
                    (u8)prP2pBssInfo->u4PrivateData);

    prMsduInfo->u2FrameLength += u2IELen;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to calculate P2P IE length for Beacon frame.
 *
 * @param[in] eNetTypeIndex      Specify which network
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 *
 * @return The length of P2P IE added
 */
/*----------------------------------------------------------------------------*/
u32 p2pFuncCalculateP2p_IELenForAssocRsp(IN P_ADAPTER_T prAdapter,
                                         IN u8 ucBssIndex,
                                         IN P_STA_RECORD_T prStaRec)
{
    P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T)NULL;

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    if (prBssInfo->eNetworkType != NETWORK_TYPE_P2P) {
        return 0;
    }

    return p2pFuncCalculateP2P_IELen(
        prAdapter, ucBssIndex, prStaRec, txAssocRspAttributesTable,
        sizeof(txAssocRspAttributesTable) / sizeof(APPEND_VAR_ATTRI_ENTRY_T));
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to generate P2P IE for Beacon frame.
 *
 * @param[in] prMsduInfo             Pointer to the composed MSDU_INFO_T.
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void p2pFuncGenerateP2p_IEForAssocRsp(IN P_ADAPTER_T prAdapter,
                                      IN P_MSDU_INFO_T prMsduInfo)
{
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;

    prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

    if (!prStaRec) {
        DBGLOG(P2P, ERROR, "prStaRec of ucStaRecIndex %d is NULL!\n",
               prMsduInfo->ucStaRecIndex);
        return;
    }

    if (IS_STA_IN_P2P(prStaRec)) {
        DBGLOG(P2P, TRACE, "Generate NULL P2P IE for Assoc Rsp.\n");

        p2pFuncGenerateP2P_IE(prAdapter, prMsduInfo->ucBssIndex, true,
                              &prMsduInfo->u2FrameLength, prMsduInfo->prPacket,
                              1500, txAssocRspAttributesTable,
                              sizeof(txAssocRspAttributesTable) /
                              sizeof(APPEND_VAR_ATTRI_ENTRY_T));
    } else {
        DBGLOG(P2P, TRACE, "Legacy device, no P2P IE.\n");
    }
}

u32 p2pFuncCalculateP2P_IELen(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                              IN P_STA_RECORD_T prStaRec,
                              IN APPEND_VAR_ATTRI_ENTRY_T arAppendAttriTable[],
                              IN u32 u4AttriTableSize)
{
    u32 u4OverallAttriLen, u4Dummy;
    u16 u2EstimatedFixedAttriLen;
    u32 i;

    /* Overall length of all Attributes */
    u4OverallAttriLen = 0;

    for (i = 0; i < u4AttriTableSize; i++) {
        u2EstimatedFixedAttriLen =
            arAppendAttriTable[i].u2EstimatedFixedAttriLen;

        if (u2EstimatedFixedAttriLen) {
            u4OverallAttriLen += u2EstimatedFixedAttriLen;
        } else {
            ASSERT(arAppendAttriTable[i].pfnCalculateVariableAttriLen);

            u4OverallAttriLen +=
                arAppendAttriTable[i].pfnCalculateVariableAttriLen(prAdapter,
                                                                   prStaRec);
        }
    }

    u4Dummy = u4OverallAttriLen;
    u4OverallAttriLen += P2P_IE_OUI_HDR;

    for (; (u4Dummy > P2P_MAXIMUM_ATTRIBUTE_LEN);) {
        u4OverallAttriLen += P2P_IE_OUI_HDR;
        u4Dummy -= P2P_MAXIMUM_ATTRIBUTE_LEN;
    }

    return u4OverallAttriLen;
}

void p2pFuncGenerateP2P_IE(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                           IN u8 fgIsAssocFrame, IN u16 *pu2Offset,
                           IN u8 *pucBuf, IN u16 u2BufSize,
                           IN APPEND_VAR_ATTRI_ENTRY_T arAppendAttriTable[],
                           IN u32 u4AttriTableSize)
{
    u8 *pucBuffer = (u8 *)NULL;
    P_IE_P2P_T prIeP2P = (P_IE_P2P_T)NULL;
    u32 u4OverallAttriLen;
    u32 u4AttriLen;
    u8 aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;
    u8 aucTempBuffer[P2P_MAXIMUM_ATTRIBUTE_LEN];
    u32 i;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (pucBuf != NULL));

        pucBuffer = (u8 *)((unsigned long)pucBuf + (*pu2Offset));

        ASSERT_BREAK(pucBuffer != NULL);

        /* Check buffer length is still enough. */
        ASSERT_BREAK((u2BufSize - (*pu2Offset)) >= P2P_IE_OUI_HDR);

        prIeP2P = (P_IE_P2P_T)pucBuffer;

        prIeP2P->ucId = ELEM_ID_P2P;

        prIeP2P->aucOui[0] = aucWfaOui[0];
        prIeP2P->aucOui[1] = aucWfaOui[1];
        prIeP2P->aucOui[2] = aucWfaOui[2];
        prIeP2P->ucOuiType = VENDOR_OUI_TYPE_P2P;

        (*pu2Offset) += P2P_IE_OUI_HDR;

        /* Overall length of all Attributes */
        u4OverallAttriLen = 0;

        for (i = 0; i < u4AttriTableSize; i++) {
            if (arAppendAttriTable[i].pfnAppendAttri) {
                u4AttriLen = arAppendAttriTable[i].pfnAppendAttri(
                    prAdapter, ucBssIndex, fgIsAssocFrame, pu2Offset, pucBuf,
                    u2BufSize);

                u4OverallAttriLen += u4AttriLen;

                if (u4OverallAttriLen > P2P_MAXIMUM_ATTRIBUTE_LEN) {
                    u4OverallAttriLen -= P2P_MAXIMUM_ATTRIBUTE_LEN;

                    prIeP2P->ucLength =
                        (VENDOR_OUI_TYPE_LEN + P2P_MAXIMUM_ATTRIBUTE_LEN);

                    pucBuffer = (u8 *)((unsigned long)prIeP2P +
                                       (VENDOR_OUI_TYPE_LEN +
                                        P2P_MAXIMUM_ATTRIBUTE_LEN));

                    prIeP2P = (P_IE_P2P_T)((unsigned long)prIeP2P +
                                           (ELEM_HDR_LEN +
                                            (VENDOR_OUI_TYPE_LEN +
                                             P2P_MAXIMUM_ATTRIBUTE_LEN)));

                    kalMemCopy(aucTempBuffer, pucBuffer, u4OverallAttriLen);

                    prIeP2P->ucId = ELEM_ID_P2P;

                    prIeP2P->aucOui[0] = aucWfaOui[0];
                    prIeP2P->aucOui[1] = aucWfaOui[1];
                    prIeP2P->aucOui[2] = aucWfaOui[2];
                    prIeP2P->ucOuiType = VENDOR_OUI_TYPE_P2P;

                    kalMemCopy(prIeP2P->aucP2PAttributes, aucTempBuffer,
                               u4OverallAttriLen);
                    (*pu2Offset) += P2P_IE_OUI_HDR;
                }
            }
        }

        prIeP2P->ucLength = (u8)(VENDOR_OUI_TYPE_LEN + u4OverallAttriLen);
    } while (false);
}

u32 p2pFuncAppendAttriStatusForAssocRsp(IN P_ADAPTER_T prAdapter,
                                        IN u8 ucBssIndex, IN u8 fgIsAssocFrame,
                                        IN u16 *pu2Offset, IN u8 *pucBuf,
                                        IN u16 u2BufSize)
{
    u8 *pucBuffer;
    P_P2P_ATTRI_STATUS_T prAttriStatus;
    u32 u4AttriLen = 0;

    ASSERT(prAdapter);
    ASSERT(pucBuf);

    if (fgIsAssocFrame) {
        return u4AttriLen;
    }

    /* TODO: For assoc request P2P IE check in driver & return status in P2P
     * IE. */

    pucBuffer = (u8 *)((unsigned long)pucBuf + (unsigned long)(*pu2Offset));

    ASSERT(pucBuffer);
    prAttriStatus = (P_P2P_ATTRI_STATUS_T)pucBuffer;

    ASSERT(u2BufSize >= ((*pu2Offset) + (u16)u4AttriLen));

    prAttriStatus->ucId = P2P_ATTRI_ID_STATUS;
    WLAN_SET_FIELD_16(&prAttriStatus->u2Length, P2P_ATTRI_MAX_LEN_STATUS);

    prAttriStatus->ucStatusCode = P2P_STATUS_SUCCESS;

    u4AttriLen = (P2P_ATTRI_HDR_LEN + P2P_ATTRI_MAX_LEN_STATUS);

    (*pu2Offset) += (u16)u4AttriLen;

    return u4AttriLen;
}

u32 p2pFuncAppendAttriExtListenTiming(IN P_ADAPTER_T prAdapter,
                                      IN u8 ucBssIndex, IN u8 fgIsAssocFrame,
                                      IN u16 *pu2Offset, IN u8 *pucBuf,
                                      IN u16 u2BufSize)
{
    u32 u4AttriLen = 0;
    P_P2P_ATTRI_EXT_LISTEN_TIMING_T prP2pExtListenTiming =
        (P_P2P_ATTRI_EXT_LISTEN_TIMING_T)NULL;
    P_P2P_SPECIFIC_BSS_INFO_T prP2pSpecificBssInfo =
        (P_P2P_SPECIFIC_BSS_INFO_T)NULL;
    u8 *pucBuffer = NULL;
    P_BSS_INFO_T prBssInfo = NULL;

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    ASSERT(prAdapter);
    ASSERT(pucBuf);
    ASSERT(prBssInfo);

    if (fgIsAssocFrame) {
        return u4AttriLen;
    }

    /* TODO: For extend listen timing. */

    prP2pSpecificBssInfo =
        prAdapter->rWifiVar.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];

    u4AttriLen = (P2P_ATTRI_HDR_LEN + P2P_ATTRI_MAX_LEN_EXT_LISTEN_TIMING);

    ASSERT(u2BufSize >= ((*pu2Offset) + (u16)u4AttriLen));

    pucBuffer = (u8 *)((unsigned long)pucBuf + (unsigned long)(*pu2Offset));

    ASSERT(pucBuffer);

    prP2pExtListenTiming = (P_P2P_ATTRI_EXT_LISTEN_TIMING_T)pucBuffer;

    prP2pExtListenTiming->ucId = P2P_ATTRI_ID_EXT_LISTEN_TIMING;
    WLAN_SET_FIELD_16(&prP2pExtListenTiming->u2Length,
                      P2P_ATTRI_MAX_LEN_EXT_LISTEN_TIMING);
    WLAN_SET_FIELD_16(&prP2pExtListenTiming->u2AvailInterval,
                      prP2pSpecificBssInfo->u2AvailabilityInterval);
    WLAN_SET_FIELD_16(&prP2pExtListenTiming->u2AvailPeriod,
                      prP2pSpecificBssInfo->u2AvailabilityPeriod);

    (*pu2Offset) += (u16)u4AttriLen;

    return u4AttriLen;
}

P_IE_HDR_T
p2pFuncGetSpecIE(IN P_ADAPTER_T prAdapter, IN u8 *pucIEBuf, IN u16 u2BufferLen,
                 IN u8 ucElemID, IN u8 *pfgIsMore)
{
    P_IE_HDR_T prTargetIE = (P_IE_HDR_T)NULL;
    u8 *pucIE = (u8 *)NULL;
    u16 u2Offset = 0;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (pucIEBuf != NULL));

        pucIE = pucIEBuf;

        if (pfgIsMore) {
            *pfgIsMore = false;
        }

        IE_FOR_EACH(pucIE, u2BufferLen, u2Offset){
            if (IE_ID(pucIE) == ucElemID) {
                if ((prTargetIE) && (pfgIsMore)) {
                    *pfgIsMore = true;
                    break;
                }
                prTargetIE = (P_IE_HDR_T)pucIE;

                if (pfgIsMore == NULL) {
                    break;
                }
            }
        }
    } while (false);

    return prTargetIE;
}

P_ATTRIBUTE_HDR_T
p2pFuncGetSpecAttri(IN P_ADAPTER_T prAdapter, IN u8 ucOuiType, IN u8 *pucIEBuf,
                    IN u16 u2BufferLen, IN u8 ucAttriID)
{
    P_IE_P2P_T prP2pIE = (P_IE_P2P_T)NULL;
    P_ATTRIBUTE_HDR_T prTargetAttri = (P_ATTRIBUTE_HDR_T)NULL;
    u8 fgIsMore = false;
    u8 *pucIE = (u8 *)NULL;
    u16 u2BufferLenLeft = 0;

    DBGLOG(P2P, INFO, "Check AssocReq Oui type %u attri %u for len %u\n",
           ucOuiType, ucAttriID, u2BufferLen);

    do {
        ASSERT_BREAK((prAdapter != NULL) && (pucIEBuf != NULL));

        u2BufferLenLeft = u2BufferLen;
        pucIE = pucIEBuf;

        do {
            fgIsMore = false;
            prP2pIE = (P_IE_P2P_T)p2pFuncGetSpecIE(
                prAdapter, pucIE, u2BufferLenLeft, ELEM_ID_VENDOR, &fgIsMore);
            if (prP2pIE) {
                ASSERT((unsigned long)prP2pIE >= (unsigned long)pucIE);
                u2BufferLenLeft =
                    u2BufferLen -
                    (u16)(((unsigned long)prP2pIE) - ((unsigned long)pucIEBuf));

                DBGLOG(P2P, INFO,
                       "Find vendor id %u len %u oui %u more %u LeftLen %u\n",
                       IE_ID(prP2pIE), IE_LEN(prP2pIE), prP2pIE->ucOuiType,
                       fgIsMore, u2BufferLenLeft);

                if (IE_LEN(prP2pIE) > P2P_OUI_TYPE_LEN) {
                    p2pFuncGetSpecAttriAction(prP2pIE, ucOuiType, ucAttriID,
                                              &prTargetAttri);
                }
                /* P2P_OUI_TYPE_LEN */
                pucIE = (u8 *)(((unsigned long)prP2pIE) + IE_SIZE(prP2pIE));
            }
            /* prP2pIE */
        } while (prP2pIE && fgIsMore && u2BufferLenLeft);
    } while (false);

    return prTargetAttri;
}

/* p2pFuncGetSpecAttri */

/* Code refactoring for AOSP */
static void p2pFuncGetSpecAttriAction(IN P_IE_P2P_T prP2pIE, IN u8 ucOuiType,
                                      IN u8 ucAttriID,
                                      OUT P_ATTRIBUTE_HDR_T *prTargetAttri)
{
    u8 *pucAttri = (u8 *)NULL;
    u16 u2OffsetAttri = 0;
    u8 aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;

    if (prP2pIE->ucOuiType == ucOuiType) {
        switch (ucOuiType) {
        case VENDOR_OUI_TYPE_WPS:
            aucWfaOui[0] = 0x00;
            aucWfaOui[1] = 0x50;
            aucWfaOui[2] = 0xF2;
            break;

        case VENDOR_OUI_TYPE_P2P:
            break;

        case VENDOR_OUI_TYPE_WPA:
        case VENDOR_OUI_TYPE_WMM:
        case VENDOR_OUI_TYPE_WFD:
        default:
            break;
        }

        if ((prP2pIE->aucOui[0] == aucWfaOui[0]) &&
            (prP2pIE->aucOui[1] == aucWfaOui[1]) &&
            (prP2pIE->aucOui[2] == aucWfaOui[2])) {
            u2OffsetAttri = 0;
            pucAttri = prP2pIE->aucP2PAttributes;

            if (ucOuiType == VENDOR_OUI_TYPE_WPS) {
                WSC_ATTRI_FOR_EACH(pucAttri, (IE_LEN(prP2pIE) - P2P_IE_OUI_HDR),
                                   u2OffsetAttri){
                    if (WSC_ATTRI_ID(pucAttri) == ucAttriID) {
                        *prTargetAttri = (P_ATTRIBUTE_HDR_T)pucAttri;
                        break;
                    }
                }
            } else if (ucOuiType == VENDOR_OUI_TYPE_P2P) {
                P2P_ATTRI_FOR_EACH(pucAttri, (IE_LEN(prP2pIE) - P2P_IE_OUI_HDR),
                                   u2OffsetAttri){
                    if (ATTRI_ID(pucAttri) == ucAttriID) {
                        *prTargetAttri = (P_ATTRIBUTE_HDR_T)pucAttri;
                        break;
                    }
                }
            }
#if CFG_SUPPORT_WFD
            else if (ucOuiType == VENDOR_OUI_TYPE_WFD) {
                WFD_ATTRI_FOR_EACH(pucAttri, (IE_LEN(prP2pIE) - P2P_IE_OUI_HDR),
                                   u2OffsetAttri){
                    if (ATTRI_ID(pucAttri) == (u8)ucAttriID) {
                        *prTargetAttri = (P_ATTRIBUTE_HDR_T)pucAttri;
                        break;
                    }
                }
            }
#endif
            else {
                /* Todo:: Nothing */
                /* Possible or else. */
            }
        }
    }
}

WLAN_STATUS
p2pFuncGenerateBeaconProbeRsp(IN P_ADAPTER_T prAdapter,
                              IN P_BSS_INFO_T prBssInfo,
                              IN P_MSDU_INFO_T prMsduInfo, IN u8 fgIsProbeRsp)
{
    WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;
    P_WLAN_BEACON_FRAME_T prBcnFrame = (P_WLAN_BEACON_FRAME_T)NULL;
    /* P_APPEND_VAR_IE_ENTRY_T prAppendIeTable =
     * (P_APPEND_VAR_IE_ENTRY_T)NULL; */

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prBssInfo != NULL) &&
                     (prMsduInfo != NULL));

        /* txBcnIETable */

        /* txProbeRspIETable */

        prBcnFrame = (P_WLAN_BEACON_FRAME_T)prMsduInfo->prPacket;

        return nicUpdateBeaconIETemplate(
            prAdapter, IE_UPD_METHOD_UPDATE_ALL, prBssInfo->ucBssIndex,
            prBssInfo->u2CapInfo, (u8 *)prBcnFrame->aucInfoElem,
            prMsduInfo->u2FrameLength -
            OFFSET_OF(WLAN_BEACON_FRAME_T, aucInfoElem));
    } while (false);

    return rWlanStatus;
}

WLAN_STATUS
p2pFuncComposeBeaconProbeRspTemplate(
    IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prP2pBssInfo, IN u8 *pucBcnBuffer,
    IN u32 u4BcnBufLen, IN u8 fgIsProbeRsp,
    IN P_P2P_PROBE_RSP_UPDATE_INFO_T prP2pProbeRspInfo, IN u8 fgSynToFW)
{
    WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;
    P_MSDU_INFO_T prMsduInfo = (P_MSDU_INFO_T)NULL;
    P_WLAN_MAC_HEADER_T prWlanBcnFrame = (P_WLAN_MAC_HEADER_T)NULL;

    u8 *pucBuffer = (u8 *)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (pucBcnBuffer != NULL) &&
                     (prP2pBssInfo != NULL));

        prWlanBcnFrame = (P_WLAN_MAC_HEADER_T)pucBcnBuffer;

        if ((prWlanBcnFrame->u2FrameCtrl != MAC_FRAME_BEACON) &&
            (!fgIsProbeRsp)) {
            rWlanStatus = WLAN_STATUS_INVALID_DATA;
            break;
        } else if (prWlanBcnFrame->u2FrameCtrl != MAC_FRAME_PROBE_RSP) {
            rWlanStatus = WLAN_STATUS_INVALID_DATA;
            break;
        }

        if (fgIsProbeRsp) {
            ASSERT_BREAK(prP2pProbeRspInfo != NULL);

            if (!prP2pProbeRspInfo->prProbeRspMsduTemplate) {
                cnmMgtPktFree(prAdapter,
                              prP2pProbeRspInfo->prProbeRspMsduTemplate);
            }

            prP2pProbeRspInfo->prProbeRspMsduTemplate =
                cnmMgtPktAlloc(prAdapter, u4BcnBufLen);

            prMsduInfo = prP2pProbeRspInfo->prProbeRspMsduTemplate;

            prMsduInfo->eSrc = TX_PACKET_MGMT;
            prMsduInfo->ucStaRecIndex = 0xFF;
            prMsduInfo->ucBssIndex = prP2pBssInfo->ucBssIndex;
        } else {
            prMsduInfo = prP2pBssInfo->prBeacon;

            if (prMsduInfo == NULL) {
                rWlanStatus = WLAN_STATUS_FAILURE;
                break;
            }

            if (u4BcnBufLen > (OFFSET_OF(WLAN_BEACON_FRAME_T, aucInfoElem[0]) +
                               MAX_IE_LENGTH)) {
                /* Unexpected error, buffer overflow. */
                ASSERT(false);
                break;
            }
        }

        pucBuffer = (u8 *)((unsigned long)(prMsduInfo->prPacket) +
                           MAC_TX_RESERVED_FIELD);

        kalMemCopy(pucBuffer, pucBcnBuffer, u4BcnBufLen);

        prMsduInfo->fgIs802_11 = true;
        prMsduInfo->u2FrameLength = (u16)u4BcnBufLen;

        if (fgSynToFW) {
            rWlanStatus = p2pFuncGenerateBeaconProbeRsp(
                prAdapter, prP2pBssInfo, prMsduInfo, fgIsProbeRsp);
        }
    } while (false);

    return rWlanStatus;
}

u32 wfdFuncCalculateWfdIELenForAssocRsp(IN P_ADAPTER_T prAdapter,
                                        IN u8 ucBssIndex,
                                        IN P_STA_RECORD_T prStaRec)
{
#if CFG_SUPPORT_WFD_COMPOSE_IE
    u16 u2EstimatedExtraIELen = 0;
    P_WFD_CFG_SETTINGS_T prWfdCfgSettings = (P_WFD_CFG_SETTINGS_T)NULL;
    P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T)NULL;

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    if (prBssInfo->eNetworkType != NETWORK_TYPE_P2P) {
        return 0;
    }

    prWfdCfgSettings = &(prAdapter->rWifiVar.rWfdConfigureSettings);

    if (IS_STA_P2P_TYPE(prStaRec) && (prWfdCfgSettings->ucWfdEnable > 0)) {
        u2EstimatedExtraIELen = prAdapter->prGlueInfo->prP2PInfo[0]->u2WFDIELen;
        ASSERT(u2EstimatedExtraIELen < 128);
    }
    return u2EstimatedExtraIELen;

#else
    return 0;

#endif
}

void wfdFuncGenerateWfdIEForAssocRsp(IN P_ADAPTER_T prAdapter,
                                     IN P_MSDU_INFO_T prMsduInfo)
{
#if CFG_SUPPORT_WFD_COMPOSE_IE
    P_WFD_CFG_SETTINGS_T prWfdCfgSettings = (P_WFD_CFG_SETTINGS_T)NULL;
    P_STA_RECORD_T prStaRec;
    u16 u2EstimatedExtraIELen;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;

    prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);

    prWfdCfgSettings = &(prAdapter->rWifiVar.rWfdConfigureSettings);

    do {
        ASSERT_BREAK((prMsduInfo != NULL) && (prAdapter != NULL));

        prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

        if (prStaRec) {
            if (IS_STA_P2P_TYPE(prStaRec)) {
                if (prWfdCfgSettings->ucWfdEnable > 0) {
                    u2EstimatedExtraIELen =
                        prAdapter->prGlueInfo
                        ->prP2PInfo[prP2pBssInfo->u4PrivateData]
                        ->u2WFDIELen;
                    if (u2EstimatedExtraIELen > 0) {
                        ASSERT(u2EstimatedExtraIELen < 128);
                        ASSERT(
                            sizeof(prAdapter->prGlueInfo
                                   ->prP2PInfo[prP2pBssInfo->u4PrivateData]
                                   ->aucWFDIE) >=
                            prAdapter->prGlueInfo
                            ->prP2PInfo[prP2pBssInfo->u4PrivateData]
                            ->u2WFDIELen);
                        kalMemCopy(
                            (prMsduInfo->prPacket + prMsduInfo->u2FrameLength),
                            prAdapter->prGlueInfo
                            ->prP2PInfo[prP2pBssInfo->u4PrivateData]
                            ->aucWFDIE,
                            u2EstimatedExtraIELen);
                        prMsduInfo->u2FrameLength += u2EstimatedExtraIELen;
                    }
                }
            }
        } else {
        }
    } while (false);

    return;

#else
    return;

#endif
}

void p2pFuncComposeNoaAttribute(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                                OUT u8 *aucNoaAttrArray, OUT u32 *pu4Len)
{
    P_BSS_INFO_T prBssInfo = NULL;
    P_P2P_ATTRI_NOA_T prNoaAttr = NULL;
    P_P2P_SPECIFIC_BSS_INFO_T prP2pSpecificBssInfo = NULL;
    P_NOA_DESCRIPTOR_T prNoaDesc = NULL;
    u32 u4NumOfNoaDesc = 0;
    u32 i = 0;

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
    prP2pSpecificBssInfo =
        prAdapter->rWifiVar.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];

    prNoaAttr = (P_P2P_ATTRI_NOA_T)aucNoaAttrArray;

    prNoaAttr->ucId = P2P_ATTRI_ID_NOTICE_OF_ABSENCE;
    prNoaAttr->ucIndex = prP2pSpecificBssInfo->ucNoAIndex;

    if (prP2pSpecificBssInfo->fgEnableOppPS) {
        prNoaAttr->ucCTWOppPSParam = P2P_CTW_OPPPS_PARAM_OPPPS_FIELD |
                                     (prP2pSpecificBssInfo->u2CTWindow &
                                      P2P_CTW_OPPPS_PARAM_CTWINDOW_MASK);
    } else {
        prNoaAttr->ucCTWOppPSParam = 0;
    }

    for (i = 0; i < prP2pSpecificBssInfo->ucNoATimingCount; i++) {
        if (prP2pSpecificBssInfo->arNoATiming[i].fgIsInUse) {
            prNoaDesc =
                (P_NOA_DESCRIPTOR_T)&prNoaAttr->aucNoADesc[u4NumOfNoaDesc];

            prNoaDesc->ucCountType =
                prP2pSpecificBssInfo->arNoATiming[i].ucCount;
            prNoaDesc->u4Duration =
                prP2pSpecificBssInfo->arNoATiming[i].u4Duration;
            prNoaDesc->u4Interval =
                prP2pSpecificBssInfo->arNoATiming[i].u4Interval;
            prNoaDesc->u4StartTime =
                prP2pSpecificBssInfo->arNoATiming[i].u4StartTime;

            u4NumOfNoaDesc++;
        }
    }

    /* include "index" + "OppPs Params" + "NOA descriptors" */
    prNoaAttr->u2Length = 2 + u4NumOfNoaDesc * sizeof(NOA_DESCRIPTOR_T);

    /* include "Attribute ID" + "Length" + "index" + "OppPs Params" + "NOA
     * descriptors" */
    *pu4Len = P2P_ATTRI_HDR_LEN + prNoaAttr->u2Length;
}

u32 p2pFuncCalculateP2P_IE_NoA(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx,
                               IN P_STA_RECORD_T prStaRec)
{
    P_P2P_SPECIFIC_BSS_INFO_T prP2pSpecificBssInfo = NULL;
    u8 ucIdx;
    u32 u4NumOfNoaDesc = 0;
    P_BSS_INFO_T prBssInfo;

    prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

    if (p2pFuncIsAPMode(
            prAdapter->rWifiVar.prP2PConnSettings[prBssInfo->u4PrivateData])) {
        return 0;
    }

    prP2pSpecificBssInfo =
        prAdapter->rWifiVar.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];

    for (ucIdx = 0; ucIdx < prP2pSpecificBssInfo->ucNoATimingCount; ucIdx++)
        if (prP2pSpecificBssInfo->arNoATiming[ucIdx].fgIsInUse) {
            u4NumOfNoaDesc++;
        }

    /* include "index" + "OppPs Params" + "NOA descriptors" */
    /* include "Attribute ID" + "Length" + "index" + "OppPs Params" + "NOA
     * descriptors" */
    return P2P_ATTRI_HDR_LEN + 2 + (u4NumOfNoaDesc * sizeof(NOA_DESCRIPTOR_T));
}

void p2pFuncGenerateP2P_IE_NoA(IN P_ADAPTER_T prAdapter,
                               IN P_MSDU_INFO_T prMsduInfo)
{
    P_IE_P2P_T prIeP2P;
    u8 aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;
    u32 u4AttributeLen;
    P_BSS_INFO_T prBssInfo;

    prBssInfo = prAdapter->aprBssInfo[prMsduInfo->ucBssIndex];

    if (p2pFuncIsAPMode(
            prAdapter->rWifiVar.prP2PConnSettings[prBssInfo->u4PrivateData])) {
        return;
    }

    prIeP2P = (P_IE_P2P_T)((unsigned long)prMsduInfo->prPacket +
                           (u32)prMsduInfo->u2FrameLength);

    prIeP2P->ucId = ELEM_ID_P2P;
    prIeP2P->aucOui[0] = aucWfaOui[0];
    prIeP2P->aucOui[1] = aucWfaOui[1];
    prIeP2P->aucOui[2] = aucWfaOui[2];
    prIeP2P->ucOuiType = VENDOR_OUI_TYPE_P2P;

    /* Compose NoA attribute */
    p2pFuncComposeNoaAttribute(prAdapter, prMsduInfo->ucBssIndex,
                               prIeP2P->aucP2PAttributes, &u4AttributeLen);

    prIeP2P->ucLength = VENDOR_OUI_TYPE_LEN + u4AttributeLen;

    prMsduInfo->u2FrameLength += (ELEM_HDR_LEN + prIeP2P->ucLength);
}

void p2pFuncClassifyAction(IN P_SW_RFB_T prSwRfb)
{
    P_P2P_PUBLIC_ACTION_FRAME_T pFrame =
        (P_P2P_PUBLIC_ACTION_FRAME_T)prSwRfb->pvHeader;

    if ((pFrame->ucCategory == CATEGORY_PUBLIC_ACTION) &&
        (pFrame->ucAction == ACTION_PUBLIC_WIFI_DIRECT)) {
        switch (pFrame->ucOuiSubtype) {
        case P2P_PUBLIC_ACTION_GO_NEGO_REQ:
            DBGLOG(P2P, WARN, "NEGO Req\n");
            break;

        case P2P_PUBLIC_ACTION_GO_NEGO_RSP:
            DBGLOG(P2P, WARN, "NEGO Resp\n");
            break;

        case P2P_PUBLIC_ACTION_GO_NEGO_CFM:
            DBGLOG(P2P, WARN, "NEGO Confirm\n");
            break;

        case P2P_PUBLIC_ACTION_INVITATION_REQ:
            DBGLOG(P2P, WARN, "Invitation Req\n");
            break;

        case P2P_PUBLIC_ACTION_INVITATION_RSP:
            DBGLOG(P2P, WARN, "Invitation Resp\n");
            break;

        case P2P_PUBLIC_ACTION_DEV_DISCOVER_REQ:
            DBGLOG(P2P, WARN, "Discovery Req\n");
            break;

        case P2P_PUBLIC_ACTION_DEV_DISCOVER_RSP:
            DBGLOG(P2P, WARN, "Discovery Resp\n");
            break;

        case P2P_PUBLIC_ACTION_PROV_DISCOVERY_REQ:
            DBGLOG(P2P, WARN, "Provision Req\n");
            break;

        case P2P_PUBLIC_ACTION_PROV_DISCOVERY_RSP:
            DBGLOG(P2P, WARN, "Provision Resp\n");
            break;

        default:
            DBGLOG(P2P, WARN, "unkown action type %d\n", pFrame->ucOuiSubtype);
            break;
        }
    }
}

#if CFG_SUPPORT_DBDC_TC6
void p2pFuncModifyChandef(IN P_ADAPTER_T prAdapter,
                          IN P_GL_P2P_INFO_T prGlueP2pInfo,
                          IN P_BSS_INFO_T prBssInfo)
{
    if (!prGlueP2pInfo) {
        DBGLOG(P2P, WARN, "p2p glue info is not active\n");
        return;
    }

    if (prGlueP2pInfo->chandef == NULL) {
        prGlueP2pInfo->chandef = (struct cfg80211_chan_def *)cnmMemAlloc(
            prAdapter, RAM_TYPE_BUF, sizeof(struct cfg80211_chan_def));
        prGlueP2pInfo->chandef->chan = (struct ieee80211_channel *)cnmMemAlloc(
            prAdapter, RAM_TYPE_BUF, sizeof(struct ieee80211_channel));
        if (!prGlueP2pInfo->chandef->chan) {
            DBGLOG(P2P, WARN, "ieee80211_channel alloc fail\n");
            return;
        }
        /* Fill chan def */
        prGlueP2pInfo->chandef->chan->band = (prBssInfo->eBand == BAND_5G) ?
                                             NL80211_BAND_5GHZ :
                                             NL80211_BAND_2GHZ;
        prGlueP2pInfo->chandef->chan->center_freq =
            nicChannelNum2Freq(prBssInfo->ucPrimaryChannel) / 1000;

        if (rlmDomainIsLegalDfsChannel(prAdapter, prBssInfo->eBand,
                                       prBssInfo->ucPrimaryChannel)) {
            prGlueP2pInfo->chandef->chan->dfs_state = NL80211_DFS_USABLE;
        } else {
            prGlueP2pInfo->chandef->chan->dfs_state = NL80211_DFS_AVAILABLE;
        }

        switch (prBssInfo->ucVhtChannelWidth) {
        case VHT_OP_CHANNEL_WIDTH_80P80:
            prGlueP2pInfo->chandef->width = NL80211_CHAN_WIDTH_80P80;
            prGlueP2pInfo->chandef->center_freq1 =
                nicChannelNum2Freq(prBssInfo->ucVhtChannelFrequencyS1) / 1000;
            prGlueP2pInfo->chandef->center_freq2 =
                nicChannelNum2Freq(prBssInfo->ucVhtChannelFrequencyS2) / 1000;
            break;

        case VHT_OP_CHANNEL_WIDTH_160:
            prGlueP2pInfo->chandef->width = NL80211_CHAN_WIDTH_160;
            prGlueP2pInfo->chandef->center_freq1 =
                nicChannelNum2Freq(prBssInfo->ucVhtChannelFrequencyS1) / 1000;
            prGlueP2pInfo->chandef->center_freq2 =
                nicChannelNum2Freq(prBssInfo->ucVhtChannelFrequencyS2) / 1000;
            break;

        case VHT_OP_CHANNEL_WIDTH_80:
            prGlueP2pInfo->chandef->width = NL80211_CHAN_WIDTH_80;
            prGlueP2pInfo->chandef->center_freq1 =
                nicChannelNum2Freq(prBssInfo->ucVhtChannelFrequencyS1) / 1000;
            prGlueP2pInfo->chandef->center_freq2 =
                nicChannelNum2Freq(prBssInfo->ucVhtChannelFrequencyS2) / 1000;
            break;

        case VHT_OP_CHANNEL_WIDTH_20_40:
            prGlueP2pInfo->chandef->center_freq1 =
                prGlueP2pInfo->chandef->chan->center_freq;
            if (prBssInfo->eBssSCO == CHNL_EXT_SCA) {
                prGlueP2pInfo->chandef->width = NL80211_CHAN_WIDTH_40;
                prGlueP2pInfo->chandef->center_freq1 += 10;
            } else if (prBssInfo->eBssSCO == CHNL_EXT_SCB) {
                prGlueP2pInfo->chandef->width = NL80211_CHAN_WIDTH_40;
                prGlueP2pInfo->chandef->center_freq1 -= 10;
            } else {
                prGlueP2pInfo->chandef->width = NL80211_CHAN_WIDTH_20;
            }
            prGlueP2pInfo->chandef->center_freq2 = 0;
            break;

        default:
            prGlueP2pInfo->chandef->width = NL80211_CHAN_WIDTH_20;
            prGlueP2pInfo->chandef->center_freq1 =
                prGlueP2pInfo->chandef->chan->center_freq;
            prGlueP2pInfo->chandef->center_freq2 = 0;
            break;
        }
    }
    return;
}
u8 p2pFuncSwitchSapChannel(IN P_ADAPTER_T prAdapter)
{
    u8 fgDbDcModeEn = false;
    u8 fgIsSapDfs = false;
    P_BSS_INFO_T prP2pBssInfo = NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = NULL;
    u8 ucStaChannelNum = 0;
    u8 ucSapChannelNum = 0;
    ENUM_BAND_T eStaBand = BAND_NULL;
    ENUM_BAND_T eSapBand = BAND_NULL;
    u8 fgAisDiscon = false;

    if (!prAdapter || !prAdapter->prAisBssInfo) {
        DBGLOG(P2P, WARN, "Not support concurrent STA + SAP\n");
        goto exit;
    }

    if (kalGetMediaStateIndicated(prAdapter->prGlueInfo) !=
        PARAM_MEDIA_STATE_CONNECTED) {
        ucStaChannelNum = 0;
#if CFG_SUPPORT_SAP_DFS_CHANNEL
        wlanUpdateDfsChannelTable(prAdapter->prGlueInfo, 0);
#endif
    } else {
        /* Get current channel info */
        ucStaChannelNum = prAdapter->prAisBssInfo->ucPrimaryChannel;
        eStaBand = (prAdapter->prAisBssInfo->ucPrimaryChannel <= 14) ?
                   BAND_2G4 :
                   BAND_5G;
        if (eStaBand != BAND_2G4 && eStaBand != BAND_5G) {
            DBGLOG(P2P, WARN, "STA has invalid band\n");
            goto exit;
        }
#if CFG_SUPPORT_SAP_DFS_CHANNEL
        wlanUpdateDfsChannelTable(prAdapter->prGlueInfo, ucStaChannelNum);
#endif
    }

    /* Assume only one sap bss info */
    prP2pBssInfo = cnmGetp2pSapBssInfo(prAdapter);
    if (!prP2pBssInfo) {
        DBGLOG(P2P, WARN, "SAP is not active\n");
        goto exit;
    }
    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prP2pBssInfo->u4PrivateData);
    if (!prP2pRoleFsmInfo) {
        DBGLOG(P2P, WARN, "SAP is not active\n");
        goto exit;
    }

    ucSapChannelNum = prP2pBssInfo->ucPrimaryChannel;
    eSapBand = prP2pBssInfo->eBand;
    if (eSapBand != BAND_2G4 && eSapBand != BAND_5G) {
        DBGLOG(P2P, WARN, "SAP has invalid band\n");
        goto exit;
    }

    if (eSapBand == BAND_5G) {
        fgIsSapDfs =
            rlmDomainIsLegalDfsChannel(prAdapter, eSapBand, ucSapChannelNum);
    }

    /* STA is not connected */
    if (ucStaChannelNum == 0) {
        if (fgIsSapDfs) {
            /* Choose one 5G channel */
            ucStaChannelNum = 36;
            eStaBand = BAND_5G;
            DBGLOG(P2P, INFO, "[SCC] Choose a channel\n");
        } else {
            DBGLOG(P2P, WARN, "STA is not connected\n");
            goto exit;
        }
    }

#if CFG_SUPPORT_DBDC
    fgDbDcModeEn = prAdapter->rWifiVar.fgDbDcModeEn;
#endif

    /* Check channel no */
    if (ucStaChannelNum == ucSapChannelNum) {
        /* Do nothing, i.e. SCC */
        DBGLOG(P2P, STATE, "[SCC] Keep StaCH(%d)\n", ucStaChannelNum);
        goto exit;
    } else if (fgDbDcModeEn == true && (eStaBand != eSapBand) && !fgIsSapDfs) {
        /* Do nothing, i.e. DBDC */
        DBGLOG(P2P, STATE, "[DBDC] Keep StaCH(%d), SapCH(%d)(dfs: %u)\n",
               ucStaChannelNum, ucSapChannelNum, fgIsSapDfs);
        goto exit;
    } else {
        /* Otherwise, switch to STA channel, i.e. SCC */

        RF_CHANNEL_INFO_T rRfChnlInfo;

        /* Use sta ch info to do sap ch switch */
        rRfChnlInfo.ucChannelNum = ucStaChannelNum;
        rRfChnlInfo.eBand = eStaBand;
        rRfChnlInfo.ucChnlBw = rlmGetBssOpBwByVhtAndHtOpInfo(prP2pBssInfo);

        /* Delay report disconnect event to NL80211 */
        if (eStaBand == BAND_5G) {
            prAdapter->rWifiVar.fgDelayInidicateDISCON = true;
        } else {
            prAdapter->rWifiVar.fgDelayInidicateDISCON = false;
        }

        /* Disconnect the AIS link */
        if (kalGetMediaStateIndicated(prAdapter->prGlueInfo) ==
            PARAM_MEDIA_STATE_CONNECTED) {
            DBGLOG(AIS, STATE, "Trigger aisBssLinkDown\n");
            aisBssLinkDown(prAdapter);
            fgAisDiscon = true;
        }

        DBGLOG(P2P, STATE,
               "[SCC] StaCH(%d), SapCH(%d), eBand(%d), ChnlBw(%d) (dfs: %u)\n",
               ucStaChannelNum, ucSapChannelNum, rRfChnlInfo.eBand,
               rRfChnlInfo.ucChnlBw, fgIsSapDfs);

        if (eStaBand == BAND_5G) {
            cnmSapChannelSwitchReq(prAdapter, &rRfChnlInfo,
                                   prP2pBssInfo->u4PrivateData);
        }
    }

exit:

    DBGLOG(P2P, TRACE, "Check done\n");
    return fgAisDiscon;
}
#endif
