// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "cnm.c"
 *    \brief  Module of Concurrent Network Management
 *
 *    Module of Concurrent Network Management
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
 * @brief This function is used to initialize variables in CNM_INFO_T.
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmInit(P_ADAPTER_T prAdapter)
{
    P_CNM_INFO_T prCnmInfo;

    ASSERT(prAdapter);

    prCnmInfo = &prAdapter->rCnmInfo;
    prCnmInfo->fgChGranted = false;
#if CFG_SUPPORT_DBDC
    cnmTimerInitTimer(prAdapter, &prAdapter->rWifiVar.rDBDCSwitchGuardTimer,
                      (PFN_MGMT_TIMEOUT_FUNC)cnmDbdcDecision,
                      (unsigned long)DBDC_DECISION_TIMER_SWITCH_GUARD_TIME);

    cnmTimerInitTimer(
        prAdapter, &prAdapter->rWifiVar.rDBDCDisableCountdownTimer,
        (PFN_MGMT_TIMEOUT_FUNC)cnmDbdcDecision,
        (unsigned long)DBDC_DECISION_TIMER_DISABLE_COUNT_DOWN);
#endif
#if CFG_SUPPORT_DBDC_TC6
    cnmTimerInitTimer(
        prAdapter, &prAdapter->rWifiVar.rDBDCReconnectCountDown,
        (PFN_MGMT_TIMEOUT_FUNC)cnmDbdcDecision,
        (unsigned long)DBDC_DECISION_TIMER_RECONNECT_COUNT_DOWN);

    cnmTimerInitTimer(
        prAdapter, &prAdapter->rWifiVar.rDBDCAisConnectCountDown,
        (PFN_MGMT_TIMEOUT_FUNC)cnmDbdcDecision,
        (unsigned long)DBDC_DECISION_TIMER_AIS_CONNECT_COUNT_DOWN);
#endif
}  /* end of cnmInit()*/

/*----------------------------------------------------------------------------*/
/*!
 * @brief Before handle the message from other module, it need to obtain
 *        the Channel privilege from Channel Manager
 *
 * @param[in] prMsgHdr   The message need to be handled.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmChMngrRequestPrivilege(P_ADAPTER_T prAdapter, P_MSG_HDR_T prMsgHdr)
{
    P_MSG_CH_REQ_T prMsgChReq;
    P_CMD_CH_PRIVILEGE_T prCmdBody;
    WLAN_STATUS rStatus;

    ASSERT(prAdapter);
    ASSERT(prMsgHdr);

    prMsgChReq = (P_MSG_CH_REQ_T)prMsgHdr;

#if CFG_SUPPORT_DBDC_TC6
    if (timerPendingTimer(&prAdapter->rWifiVar.rDBDCSwitchGuardTimer) ||
        timerPendingTimer(
            &prAdapter->rWifiVar.rDBDCDisableCountdownTimer)) {
        LINK_INSERT_TAIL(
            &prAdapter->rCnmInfo.rDbdcSwitchGuradPendingReqList,
            &prMsgHdr->rLinkEntry);
        DBGLOG(CNM, STATE, "[DBDC] ChReq: queued BSS %u Token %u REQ\n",
               prMsgChReq->ucBssIndex, prMsgChReq->ucTokenID);
        return;
    }
#endif

    prCmdBody = (P_CMD_CH_PRIVILEGE_T)cnmMemAlloc(
        prAdapter, RAM_TYPE_BUF, sizeof(CMD_CH_PRIVILEGE_T));
    ASSERT(prCmdBody);

    /* To do: exception handle */
    if (!prCmdBody) {
        DBGLOG(CNM, ERROR,
               "ChReq: fail to get buf (net=%d, token=%d)\n",
               prMsgChReq->ucBssIndex, prMsgChReq->ucTokenID);

        cnmMemFree(prAdapter, prMsgHdr);
        return;
    }

    DBGLOG(CNM, INFO, "ChReq net=%d token=%d b=%d c=%d s=%d w=%d\n",
           prMsgChReq->ucBssIndex, prMsgChReq->ucTokenID,
           prMsgChReq->eRfBand, prMsgChReq->ucPrimaryChannel,
           prMsgChReq->eRfSco, prMsgChReq->eRfChannelWidth);

    prCmdBody->ucBssIndex = prMsgChReq->ucBssIndex;
    prCmdBody->ucTokenID = prMsgChReq->ucTokenID;
    prCmdBody->ucAction = CMD_CH_ACTION_REQ;  /* Request */
    prCmdBody->ucPrimaryChannel = prMsgChReq->ucPrimaryChannel;
    prCmdBody->ucRfSco = (u8)prMsgChReq->eRfSco;
    prCmdBody->ucRfBand = (u8)prMsgChReq->eRfBand;
    prCmdBody->ucRfChannelWidth = (u8)prMsgChReq->eRfChannelWidth;
    prCmdBody->ucRfCenterFreqSeg1 = (u8)prMsgChReq->ucRfCenterFreqSeg1;
    prCmdBody->ucRfCenterFreqSeg2 = (u8)prMsgChReq->ucRfCenterFreqSeg2;
    prCmdBody->ucReqType = (u8)prMsgChReq->eReqType;
    prCmdBody->ucDBDCBand = (u8)prMsgChReq->eDBDCBand;
    prCmdBody->aucReserved = 0;
    prCmdBody->u4MaxInterval = prMsgChReq->u4MaxInterval;
    prCmdBody->aucReserved2[0] = 0;
    prCmdBody->aucReserved2[1] = 0;
    prCmdBody->aucReserved2[2] = 0;
    prCmdBody->aucReserved2[3] = 0;
    prCmdBody->aucReserved2[4] = 0;
    prCmdBody->aucReserved2[5] = 0;
    prCmdBody->aucReserved2[6] = 0;
    prCmdBody->aucReserved2[7] = 0;

    ASSERT(prCmdBody->ucBssIndex <= MAX_BSS_INDEX);

    /* For monkey testing 20110901 */
    if (prCmdBody->ucBssIndex > MAX_BSS_INDEX) {
        DBGLOG(CNM, ERROR, "CNM: ChReq with wrong netIdx=%d\n\n",
               prCmdBody->ucBssIndex);
    }

    rStatus = wlanSendSetQueryCmd(
        prAdapter,  /* prAdapter */
        CMD_ID_CH_PRIVILEGE,  /* ucCID */
        true,  /* fgSetQuery */
        false,  /* fgNeedResp */
        false,  /* fgIsOid */
        NULL,  /* pfCmdDoneHandler */
        NULL,  /* pfCmdTimeoutHandler */
        sizeof(CMD_CH_PRIVILEGE_T),  /* u4SetQueryInfoLen
                                      */
        (u8 *)prCmdBody,  /* pucInfoBuffer */
        NULL,  /* pvSetQueryBuffer */
        0  /* u4SetQueryBufferLen */
        );

    /* ASSERT(rStatus == WLAN_STATUS_PENDING); */

    cnmMemFree(prAdapter, prCmdBody);
    cnmMemFree(prAdapter, prMsgHdr);
}  /* end of cnmChMngrRequestPrivilege()*/

/*----------------------------------------------------------------------------*/
/*!
 * @brief Before deliver the message to other module, it need to release
 *        the Channel privilege to Channel Manager.
 *
 * @param[in] prMsgHdr   The message need to be delivered
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmChMngrAbortPrivilege(P_ADAPTER_T prAdapter, P_MSG_HDR_T prMsgHdr)
{
    P_MSG_CH_ABORT_T prMsgChAbort;
    P_CMD_CH_PRIVILEGE_T prCmdBody;
    P_CNM_INFO_T prCnmInfo;
    WLAN_STATUS rStatus;
#if CFG_SISO_SW_DEVELOP
    P_BSS_INFO_T prBssInfo;
#endif
#if CFG_SUPPORT_DBDC_TC6
    P_LINK_ENTRY_T prLinkEntry_pendingMsg;
    P_MSG_CH_REQ_T prPendingMsg;
#endif

    ASSERT(prAdapter);
    ASSERT(prMsgHdr);

    prMsgChAbort = (P_MSG_CH_ABORT_T)prMsgHdr;

#if CFG_SUPPORT_DBDC_TC6
    if (timerPendingTimer(&prAdapter->rWifiVar.rDBDCSwitchGuardTimer) ||
        timerPendingTimer(
            &prAdapter->rWifiVar.rDBDCDisableCountdownTimer)) {
        LINK_FOR_EACH(
            prLinkEntry_pendingMsg,
            &prAdapter->rCnmInfo.rDbdcSwitchGuradPendingReqList) {
            prPendingMsg = (P_MSG_CH_REQ_T)LINK_ENTRY(
                prLinkEntry_pendingMsg, MSG_HDR_T, rLinkEntry);

            /* Find matched request and check
             * if it is being served.
             */
            if (prPendingMsg->ucBssIndex ==
                prMsgChAbort->ucBssIndex &&
                prPendingMsg->ucTokenID ==
                prMsgChAbort->ucTokenID) {
                LINK_REMOVE_KNOWN_ENTRY(
                    &prAdapter->rCnmInfo
                    .rDbdcSwitchGuradPendingReqList,
                    &prPendingMsg->rMsgHdr.rLinkEntry);

                DBGLOG(CNM,
                       STATE,
                       "[DBDC] ChAbort: remove BSS %u Token %u REQ)\n",
                       prPendingMsg->ucBssIndex,
                       prPendingMsg->ucTokenID);

                cnmMemFree(prAdapter, prPendingMsg);
                cnmMemFree(prAdapter, prMsgHdr);

                return;
            }
        }
    }
#endif

    /* Check if being granted channel privilege is aborted */
    prCnmInfo = &prAdapter->rCnmInfo;
    if (prCnmInfo->fgChGranted &&
        prCnmInfo->ucBssIndex == prMsgChAbort->ucBssIndex &&
        prCnmInfo->ucTokenID == prMsgChAbort->ucTokenID) {
        prCnmInfo->fgChGranted = false;
    }

    prCmdBody = (P_CMD_CH_PRIVILEGE_T)cnmMemAlloc(
        prAdapter, RAM_TYPE_BUF, sizeof(CMD_CH_PRIVILEGE_T));
    ASSERT(prCmdBody);

    /* To do: exception handle */
    if (!prCmdBody) {
        DBGLOG(CNM, ERROR,
               "ChAbort: fail to get buf (net=%d, token=%d)\n",
               prMsgChAbort->ucBssIndex, prMsgChAbort->ucTokenID);

        cnmMemFree(prAdapter, prMsgHdr);
        return;
    }
    kalMemZero(prCmdBody, sizeof(CMD_CH_PRIVILEGE_T));

    prCmdBody->ucBssIndex = prMsgChAbort->ucBssIndex;
    prCmdBody->ucTokenID = prMsgChAbort->ucTokenID;
    prCmdBody->ucAction = CMD_CH_ACTION_ABORT;  /* Abort */
    prCmdBody->ucDBDCBand = (u8)prMsgChAbort->eDBDCBand;

    DBGLOG(CNM, INFO, "ChAbort net=%d token=%d dbdc=%u\n",
           prCmdBody->ucBssIndex, prCmdBody->ucTokenID,
           prCmdBody->ucDBDCBand);

    ASSERT(prCmdBody->ucBssIndex <= MAX_BSS_INDEX);

    /* For monkey testing 20110901 */
    if (prCmdBody->ucBssIndex > MAX_BSS_INDEX) {
        DBGLOG(CNM, ERROR, "CNM: ChAbort with wrong netIdx=%d\n\n",
               prCmdBody->ucBssIndex);
    }

    rStatus = wlanSendSetQueryCmd(
        prAdapter,  /* prAdapter */
        CMD_ID_CH_PRIVILEGE,  /* ucCID */
        true,  /* fgSetQuery */
        false,  /* fgNeedResp */
        false,  /* fgIsOid */
        NULL,  /* pfCmdDoneHandler */
        NULL,  /* pfCmdTimeoutHandler */
        sizeof(CMD_CH_PRIVILEGE_T),  /* u4SetQueryInfoLen
                                      */
        (u8 *)prCmdBody,  /* pucInfoBuffer */
        NULL,  /* pvSetQueryBuffer */
        0  /* u4SetQueryBufferLen */
        );

    /* ASSERT(rStatus == WLAN_STATUS_PENDING); */

#if CFG_SISO_SW_DEVELOP
    prBssInfo = prAdapter->aprBssInfo[prMsgChAbort->ucBssIndex];
    /* Driver clear granted CH in BSS info */
    prBssInfo->fgIsGranted = false;
    prBssInfo->eBandGranted = BAND_NULL;
    prBssInfo->ucPrimaryChannelGranted = 0;
#endif

    cnmMemFree(prAdapter, prCmdBody);
    cnmMemFree(prAdapter, prMsgHdr);
}  /* end of cnmChMngrAbortPrivilege()*/

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmChMngrHandleChEvent(P_ADAPTER_T prAdapter, P_WIFI_EVENT_T prEvent,
                            IN u32 u4EventBufLen)
{
    P_EVENT_CH_PRIVILEGE_T prEventBody;
    P_MSG_CH_GRANT_T prChResp;
    P_BSS_INFO_T prBssInfo;
    P_CNM_INFO_T prCnmInfo;

    ASSERT(prAdapter);
    ASSERT(prEvent);
    if (u4EventBufLen < sizeof(EVENT_CH_PRIVILEGE_T)) {
        DBGLOG(CNM, ERROR, "%s: Invalid event length: %d < %d\n",
               __func__, u4EventBufLen, sizeof(EVENT_CH_PRIVILEGE_T));
        return;
    }
    prEventBody = (P_EVENT_CH_PRIVILEGE_T)(prEvent->aucBuffer);
    prChResp = (P_MSG_CH_GRANT_T)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                             sizeof(MSG_CH_GRANT_T));
    ASSERT(prChResp);

    /* To do: exception handle */
    if (!prChResp) {
        DBGLOG(CNM, ERROR,
               "ChGrant: fail to get buf (net=%d, token=%d)\n",
               prEventBody->ucBssIndex, prEventBody->ucTokenID);

        return;
    }

    DBGLOG(CNM, INFO, "ChGrant net=%d token=%d ch=%d sco=%d\n",
           prEventBody->ucBssIndex, prEventBody->ucTokenID,
           prEventBody->ucPrimaryChannel, prEventBody->ucRfSco);

    ASSERT(prEventBody->ucBssIndex <= MAX_BSS_INDEX);
    ASSERT(prEventBody->ucStatus == EVENT_CH_STATUS_GRANT);
    if (prEventBody->ucBssIndex > MAX_BSS_INDEX) {
        DBGLOG(CNM,
               ERROR,
               "cnmChMngrHandleChEvent: (ucBssIndex = %d) out-of-bound\n",
               prEventBody->ucBssIndex);
        return;
    }

    prBssInfo = prAdapter->aprBssInfo[prEventBody->ucBssIndex];

    /* Decide message ID based on network and response status */
    if (IS_BSS_AIS(prBssInfo)) {
        prChResp->rMsgHdr.eMsgId = MID_CNM_AIS_CH_GRANT;
    }
#if CFG_ENABLE_WIFI_DIRECT
    else if (prAdapter->fgIsP2PRegistered && IS_BSS_P2P(prBssInfo)) {
        prChResp->rMsgHdr.eMsgId = MID_CNM_P2P_CH_GRANT;
    }
#endif

    else {
        cnmMemFree(prAdapter, prChResp);
        return;
    }

    prChResp->ucBssIndex = prEventBody->ucBssIndex;
    prChResp->ucTokenID = prEventBody->ucTokenID;
    prChResp->ucPrimaryChannel = prEventBody->ucPrimaryChannel;
    prChResp->eRfSco = (ENUM_CHNL_EXT_T)prEventBody->ucRfSco;
    prChResp->eRfBand = (ENUM_BAND_T)prEventBody->ucRfBand;
    prChResp->eRfChannelWidth =
        (ENUM_CHANNEL_WIDTH_T)prEventBody->ucRfChannelWidth;
    prChResp->ucRfCenterFreqSeg1 = prEventBody->ucRfCenterFreqSeg1;
    prChResp->ucRfCenterFreqSeg2 = prEventBody->ucRfCenterFreqSeg2;
    prChResp->eReqType = (ENUM_CH_REQ_TYPE_T)prEventBody->ucReqType;
    prChResp->eDBDCBand = (ENUM_DBDC_BN_T)prEventBody->ucDBDCBand;
    prChResp->u4GrantInterval = prEventBody->u4GrantInterval;

    mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prChResp,
                MSG_SEND_METHOD_BUF);

    /* Record current granted BSS for TXM's reference */
    prCnmInfo = &prAdapter->rCnmInfo;
    prCnmInfo->ucBssIndex = prEventBody->ucBssIndex;
    prCnmInfo->ucTokenID = prEventBody->ucTokenID;
    prCnmInfo->fgChGranted = true;
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
void cnmRadarDetectEvent(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                         IN u32 u4EventBufLen)
{
    P_EVENT_RDD_REPORT_T prEventBody;
    P_BSS_INFO_T prBssInfo;
    P_MSG_P2P_RADAR_DETECT_T prP2pRddDetMsg;
    u8 ucBssIndex;

    ASSERT(prAdapter);
    ASSERT(prEvent);

    DBGLOG(CNM, INFO, "cnmRadarDetectEvent.\n");
    if (u4EventBufLen < sizeof(EVENT_RDD_REPORT_T)) {
        DBGLOG(CNM, ERROR, "%s: Invalid event length: %d < %d\n",
               __func__, u4EventBufLen, sizeof(EVENT_RDD_REPORT_T));
        return;
    }
    prEventBody = (P_EVENT_RDD_REPORT_T)(prEvent->aucBuffer);

    prP2pRddDetMsg = (P_MSG_P2P_RADAR_DETECT_T)cnmMemAlloc(
        prAdapter, RAM_TYPE_MSG, sizeof(*prP2pRddDetMsg));

    if (!prP2pRddDetMsg) {
        DBGLOG(CNM, ERROR, "cnmMemAlloc for prP2pRddDetMsg failed!\n");
        return;
    }

    prP2pRddDetMsg->rMsgHdr.eMsgId = MID_CNM_P2P_RADAR_DETECT;

    for (ucBssIndex = 0; ucBssIndex < BSS_INFO_NUM; ucBssIndex++) {
        prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

        if (prBssInfo && prBssInfo->fgIsDfsActive) {
            prP2pRddDetMsg->ucBssIndex = ucBssIndex;
            break;
        }
    }

    p2pFuncSetDfsState(DFS_STATE_DETECTED);

    p2pFuncRadarInfoInit();

    g_rP2pRadarInfo.ucRadarReportMode = prEventBody->ucRadarReportMode;
    g_rP2pRadarInfo.ucRddIdx = prEventBody->ucRddIdx;
    g_rP2pRadarInfo.ucLongDetected = prEventBody->ucLongDetected;
    g_rP2pRadarInfo.ucPeriodicDetected = prEventBody->ucPeriodicDetected;
    g_rP2pRadarInfo.ucLPBNum = prEventBody->ucLPBNum;
    g_rP2pRadarInfo.ucPPBNum = prEventBody->ucPPBNum;
    g_rP2pRadarInfo.ucLPBPeriodValid = prEventBody->ucLPBPeriodValid;
    g_rP2pRadarInfo.ucLPBWidthValid = prEventBody->ucLPBWidthValid;
    g_rP2pRadarInfo.ucPRICountM1 = prEventBody->ucPRICountM1;
    g_rP2pRadarInfo.ucPRICountM1TH = prEventBody->ucPRICountM1TH;
    g_rP2pRadarInfo.ucPRICountM2 = prEventBody->ucPRICountM2;
    g_rP2pRadarInfo.ucPRICountM2TH = prEventBody->ucPRICountM2TH;
    g_rP2pRadarInfo.u4PRI1stUs = prEventBody->u4PRI1stUs;
    if (prEventBody->ucLPBNum <= LPB_SIZE) {
        kalMemCopy(&g_rP2pRadarInfo.arLpbContent[0],
                   &prEventBody->arLpbContent[0],
                   prEventBody->ucLPBNum * sizeof(LONG_PULSE_BUFFER_T));
    }
    if (prEventBody->ucPPBNum <= PPB_SIZE) {
        kalMemCopy(&g_rP2pRadarInfo.arPpbContent[0],
                   &prEventBody->arPpbContent[0],
                   prEventBody->ucPPBNum *
                   sizeof(PERIODIC_PULSE_BUFFER_T));
    }

    mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prP2pRddDetMsg,
                MSG_SEND_METHOD_BUF);
}

void cnmCsaDoneEvent(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                     IN u32 u4EventBufLen)
{
    P_BSS_INFO_T prBssInfo;
    P_MSG_P2P_CSA_DONE_T prP2pCsaDoneMsg;
    u8 ucBssIndex;
#if CFG_SUPPORT_DBDC_TC6
    u8 fgBssDfsActive = false;
#endif

    DBGLOG(CNM, INFO, "cnmCsaDoneEvent.\n");

    prP2pCsaDoneMsg = (P_MSG_P2P_CSA_DONE_T)cnmMemAlloc(
        prAdapter, RAM_TYPE_MSG, sizeof(*prP2pCsaDoneMsg));

    if (!prP2pCsaDoneMsg) {
        DBGLOG(CNM, ERROR, "cnmMemAlloc for prP2pCsaDoneMsg failed!\n");
        return;
    }

    prAdapter->rWifiVar.fgCsaInProgress = false;

    prP2pCsaDoneMsg->rMsgHdr.eMsgId = MID_CNM_P2P_CSA_DONE;

    for (ucBssIndex = 0; ucBssIndex < BSS_INFO_NUM; ucBssIndex++) {
        prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

        if (prBssInfo && prBssInfo->fgIsDfsActive) {
            prP2pCsaDoneMsg->ucBssIndex = ucBssIndex;
#if CFG_SUPPORT_DBDC_TC6
            fgBssDfsActive = true;
#endif
            break;
        }
    }

#if CFG_SUPPORT_DBDC_TC6
    prBssInfo = cnmGetp2pSapBssInfo(prAdapter);
    if (!fgBssDfsActive && prBssInfo) {
        prP2pCsaDoneMsg->ucBssIndex = prBssInfo->ucBssIndex;
        DBGLOG(CNM, INFO, "cnmCsaDoneEvent.ucBssIndex=%d\n",
               prP2pCsaDoneMsg->ucBssIndex);
    }
#endif

    mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prP2pCsaDoneMsg,
                MSG_SEND_METHOD_BUF);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is invoked for P2P or BOW networks
 *
 * @param (none)
 *
 * @return true: suggest to adopt the returned preferred channel
 *         false: No suggestion. Caller should adopt its preference
 */
/*----------------------------------------------------------------------------*/
u8 cnmPreferredChannel(P_ADAPTER_T prAdapter, P_ENUM_BAND_T prBand,
                       u8 *pucPrimaryChannel, P_ENUM_CHNL_EXT_T prBssSCO)
{
    P_BSS_INFO_T prBssInfo;
    u8 i;

    ASSERT(prAdapter);
    ASSERT(prBand);
    ASSERT(pucPrimaryChannel);
    ASSERT(prBssSCO);

    for (i = 0; i < BSS_INFO_NUM; i++) {
        prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);

        if (prBssInfo) {
            if (IS_BSS_AIS(prBssInfo) &&
                RLM_NET_PARAM_VALID(prBssInfo)) {
                *prBand = prBssInfo->eBand;
                *pucPrimaryChannel =
                    prBssInfo->ucPrimaryChannel;
                *prBssSCO = prBssInfo->eBssSCO;

                return true;
            }
        }
    }

    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return true: available channel is limited to return value
 *         false: no limited
 */
/*----------------------------------------------------------------------------*/
u8 cnmAisInfraChannelFixed(P_ADAPTER_T prAdapter, P_ENUM_BAND_T prBand,
                           u8 *pucPrimaryChannel)
{
    P_BSS_INFO_T prBssInfo;
    u8 i;
    P_WIFI_VAR_T prWifiVar = &prAdapter->rWifiVar;

    ASSERT(prAdapter);

    if (prWifiVar->u4ScanCtrl & SCN_CTRL_DEFAULT_SCAN_CTRL) {
        /*DBGLOG(CNM, INFO, "cnmAisInfraChannelFixed: ByPass AIS channel
         * Fix check\n");*/
        return false;
    }

    for (i = 0; i < BSS_INFO_NUM; i++) {
        prBssInfo = prAdapter->aprBssInfo[i];

        if (!IS_NET_ACTIVE(prAdapter, i)) {
            continue;
        }

#if CFG_ENABLE_WIFI_DIRECT
        if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
            u8 fgFixedChannel = p2pFuncIsAPMode(
                prAdapter->rWifiVar.prP2PConnSettings
                [prBssInfo->u4PrivateData]);

            if (fgFixedChannel) {
                *prBand = prBssInfo->eBand;
                *pucPrimaryChannel =
                    prBssInfo->ucPrimaryChannel;

                return true;
            }
        }
#endif
    }

    return false;
}

#if CFG_SUPPORT_CHNL_CONFLICT_REVISE
u8 cnmAisDetectP2PChannel(P_ADAPTER_T prAdapter, P_ENUM_BAND_T prBand,
                          u8 *pucPrimaryChannel)
{
    u8 i = 0;
    P_BSS_INFO_T prBssInfo;
#if CFG_ENABLE_WIFI_DIRECT
    for (; i < BSS_INFO_NUM; i++) {
        prBssInfo = prAdapter->aprBssInfo[i];
        if (prBssInfo->eNetworkType != NETWORK_TYPE_P2P) {
            continue;
        }
        if (prBssInfo->eConnectionState ==
            PARAM_MEDIA_STATE_CONNECTED ||
            (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT &&
             prBssInfo->eIntendOPMode == OP_MODE_NUM)) {
            *prBand = prBssInfo->eBand;
            *pucPrimaryChannel = prBssInfo->ucPrimaryChannel;
            return true;
        }
    }
#endif
    return false;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return true: permitted
 *         false: Not permitted
 */
/*----------------------------------------------------------------------------*/
u8 cnmAisIbssIsPermitted(P_ADAPTER_T prAdapter)
{
    P_BSS_INFO_T prBssInfo;
    u8 i;

    ASSERT(prAdapter);

    /* P2P device network shall be included */
    for (i = 0; i <= BSS_INFO_NUM; i++) {
        prBssInfo = prAdapter->aprBssInfo[i];

        if (prBssInfo && IS_BSS_ACTIVE(prBssInfo) &&
            !IS_BSS_AIS(prBssInfo)) {
            return false;
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return true: permitted
 *         false: Not permitted
 */
/*----------------------------------------------------------------------------*/
u8 cnmP2PIsPermitted(P_ADAPTER_T prAdapter)
{
    P_BSS_INFO_T prBssInfo;
    u8 i;
    u8 fgBowIsActive;

    ASSERT(prAdapter);

    fgBowIsActive = false;

    for (i = 0; i < BSS_INFO_NUM; i++) {
        prBssInfo = prAdapter->aprBssInfo[i];

        if (prBssInfo && IS_BSS_ACTIVE(prBssInfo)) {
            if (prBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
                return false;
            }else if (IS_BSS_BOW(prBssInfo)) {
                fgBowIsActive = true;
            }
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return true: permitted
 *         false: Not permitted
 */
/*----------------------------------------------------------------------------*/
u8 cnmBowIsPermitted(P_ADAPTER_T prAdapter)
{
    P_BSS_INFO_T prBssInfo;
    u8 i;

    ASSERT(prAdapter);

    /* P2P device network shall be included */
    for (i = 0; i <= BSS_INFO_NUM; i++) {
        prBssInfo = prAdapter->aprBssInfo[i];

        if (prBssInfo && IS_BSS_ACTIVE(prBssInfo) &&
            (IS_BSS_P2P(prBssInfo) ||
             prBssInfo->eCurrentOPMode == OP_MODE_IBSS)) {
            return false;
        }
    }

    return true;
}

static u8 cnmGetAPBwPermitted(P_ADAPTER_T prAdapter, u8 ucBssIndex)
{
    P_BSS_INFO_T prBssInfo;
    u8 ucAPBandwidth = MAX_BW_160MHZ;
    P_BSS_DESC_T prBssDesc = NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    u8 i = 0;
    u8 ucOffset = (MAX_BW_80MHZ - CW_80MHZ);

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    if (IS_BSS_AIS(prBssInfo)) {
        /*AIS station mode*/
        prBssDesc = prAdapter->rWifiVar.rAisFsmInfo.prTargetBssDesc;
    } else if (IS_BSS_P2P(prBssInfo)) {
        /* P2P mode */

        for (i = 0; i < BSS_P2P_NUM; i++) {
            if (!prAdapter->rWifiVar.aprP2pRoleFsmInfo[i]) {
                continue;
            }

            if (prAdapter->rWifiVar.aprP2pRoleFsmInfo[i]
                ->ucBssIndex == ucBssIndex) {
                break;
            }
        }

        if (i >= BSS_P2P_NUM) {
            prP2pRoleFsmInfo = NULL;
        } else {
            prP2pRoleFsmInfo =
                prAdapter->rWifiVar.aprP2pRoleFsmInfo[i];

            /*only GC need to consider GO's BW*/
            if (!p2pFuncIsAPMode(
                    prAdapter->rWifiVar.prP2PConnSettings
                    [prBssInfo->u4PrivateData])) {
                prBssDesc = prP2pRoleFsmInfo->rJoinInfo
                            .prTargetBssDesc;
            }
        }
    }

    if (prBssDesc) {
        if (prBssDesc->eChannelWidth == CW_20_40MHZ) {
            if ((prBssDesc->eSco == CHNL_EXT_SCA) ||
                (prBssDesc->eSco == CHNL_EXT_SCB)) {
                ucAPBandwidth = MAX_BW_40MHZ;
            }else{
                ucAPBandwidth = MAX_BW_20MHZ;
            }
        } else {
            ucAPBandwidth = prBssDesc->eChannelWidth + ucOffset;
        }
    }

    return ucAPBandwidth;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return true: permitted
 *         false: Not permitted
 */
/*----------------------------------------------------------------------------*/
u8 cnmBss40mBwPermitted(P_ADAPTER_T prAdapter, u8 ucBssIndex)
{
    ASSERT(prAdapter);

    /* Note: To support real-time decision instead of current
     * activated-time, the STA roaming case shall be considered about
     * synchronization problem. Another variable fgAssoc40mBwAllowed is
     * added to represent HT capability when association
     */

    /* Decide max bandwidth by feature option */
    if (cnmGetBssMaxBw(prAdapter, ucBssIndex) < MAX_BW_40MHZ) {
        return false;
    }

    /*check AP or GO capbility for Station or GC */
    if (cnmGetAPBwPermitted(prAdapter, ucBssIndex) < MAX_BW_40MHZ) {
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param (none)
 *
 * @return true: permitted
 *         false: Not permitted
 */
/*----------------------------------------------------------------------------*/
u8 cnmBss80mBwPermitted(P_ADAPTER_T prAdapter, u8 ucBssIndex)
{
    ASSERT(prAdapter);

    /* Note: To support real-time decision instead of current
     * activated-time, the STA roaming case shall be considered about
     * synchronization problem. Another variable fgAssoc40mBwAllowed is
     * added to represent HT capability when association
     */

    /* Check 40Mhz first */
    if (!cnmBss40mBwPermitted(prAdapter, ucBssIndex)) {
        return false;
    }

    /* Decide max bandwidth by feature option */
    if (cnmGetBssMaxBw(prAdapter, ucBssIndex) < MAX_BW_80MHZ) {
        return false;
    }

    /*check AP or GO capbility for Station or GC */
    if (cnmGetAPBwPermitted(prAdapter, ucBssIndex) < MAX_BW_80MHZ) {
        return false;
    }

    return true;
}

u8 cnmGetBssMaxBw(P_ADAPTER_T prAdapter, u8 ucBssIndex)
{
    P_BSS_INFO_T prBssInfo;
    u8 ucMaxBandwidth = MAX_BW_80_80_MHZ;  /*chip capability*/
    P_BSS_DESC_T prBssDesc = NULL;
    ENUM_BAND_T eBand = BAND_NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_P2P_CONNECTION_REQ_INFO_T prP2pConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;
#if (CFG_SUPPORT_SINGLE_SKU == 1)
    u8 ucChannelBw = MAX_BW_80_80_MHZ;
#endif

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    if (IS_BSS_AIS(prBssInfo)) {
        /* STA mode */

        /*should check Bss_info could be used or not
         * the info might not be trustable before state3
         */
        prBssDesc = prAdapter->rWifiVar.rAisFsmInfo.prTargetBssDesc;

        if (prBssInfo->eConnectionState ==
            PARAM_MEDIA_STATE_CONNECTED) {
            eBand = prBssInfo->eBand;
            DBGLOG(CNM,
                   INFO,
                   "use connected bcn eBand %d target prBssDesc %p\n",
                   eBand,
                   prBssDesc);
        } else {
            if (prBssDesc) {
                eBand = prBssDesc->eBand;
                DBGLOG(CNM, INFO, "use target bcn eBand %d\n",
                       eBand);
            } else {
                eBand = prBssInfo->eBand;
                DBGLOG(CNM, INFO, "use bcn eBand %d\n", eBand);
            }
        }

        ASSERT(eBand != BAND_NULL);

        if (eBand == BAND_2G4) {
            ucMaxBandwidth = prAdapter->rWifiVar.ucSta2gBandwidth;
        }else{
            ucMaxBandwidth = prAdapter->rWifiVar.ucSta5gBandwidth;
        }

        if (ucMaxBandwidth > prAdapter->rWifiVar.ucStaBandwidth) {
            ucMaxBandwidth = prAdapter->rWifiVar.ucStaBandwidth;
        }
    } else if (IS_BSS_P2P(prBssInfo)) {
        prP2pRoleFsmInfo =
            p2pFuncGetRoleByBssIdx(prAdapter, ucBssIndex);
        if (!prAdapter->rWifiVar.ucApChnlDefFromCfg &&
            prP2pRoleFsmInfo &&
            prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
            prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
            ucMaxBandwidth = prP2pConnReqInfo->eChnlBw;
        } else {
            /* AP mode */
            if (p2pFuncIsAPMode(
                    prAdapter->rWifiVar.prP2PConnSettings
                    [prBssInfo->u4PrivateData])) {
                if (prBssInfo->eBand == BAND_2G4) {
                    ucMaxBandwidth =
                        prAdapter->rWifiVar
                        .ucAp2gBandwidth;
                } else {
                    ucMaxBandwidth =
                        prAdapter->rWifiVar
                        .ucAp5gBandwidth;
                }

                if (ucMaxBandwidth >
                    prAdapter->rWifiVar.ucApBandwidth) {
                    ucMaxBandwidth = prAdapter->rWifiVar
                                     .ucApBandwidth;
                }
            }
            /* P2P mode */
            else {
                if (prBssInfo->eBand == BAND_2G4) {
                    ucMaxBandwidth =
                        prAdapter->rWifiVar
                        .ucP2p2gBandwidth;
                } else {
                    ucMaxBandwidth =
                        prAdapter->rWifiVar
                        .ucP2p5gBandwidth;
                }
            }
        }
    }

#if (CFG_SUPPORT_SINGLE_SKU == 1)
    if (IS_BSS_AIS(prBssInfo) &&
        (prBssInfo->eConnectionState != PARAM_MEDIA_STATE_CONNECTED) &&
        prBssDesc) {
        ucChannelBw = rlmDomainGetChannelBw(prBssDesc->ucChannelNum);
        DBGLOG(CNM, INFO, "target channel %d ucChannelBw %d\n",
               prBssDesc->ucChannelNum, ucChannelBw);
    } else {  // P2P and IS_BSS_AIS(prBssInfo) in connected state
        ucChannelBw =
            rlmDomainGetChannelBw(prBssInfo->ucPrimaryChannel);
        DBGLOG(CNM, INFO, "channel %d ucChannelBw %d\n",
               prBssInfo->ucPrimaryChannel, ucChannelBw);
    }
    if (ucMaxBandwidth > ucChannelBw) {
        ucMaxBandwidth = ucChannelBw;
    }
#endif

    DBGLOG(CNM, INFO, "ucMaxBandwidth %d\n", ucMaxBandwidth);

    return ucMaxBandwidth;
}

u8 cnmGetBssMaxBwToChnlBW(P_ADAPTER_T prAdapter, u8 ucBssIndex)
{
    u8 ucMaxBandwidth = cnmGetBssMaxBw(prAdapter, ucBssIndex);
    return ucMaxBandwidth == MAX_BW_20MHZ ? ucMaxBandwidth :
           (ucMaxBandwidth - 1);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief    Search available HW ID and BSS_INFO structure and initialize
 *           these parameters, i.e., fgIsNetActive, ucBssIndex, eNetworkType
 *           and ucOwnMacIndex
 *
 * @param (none)
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
P_BSS_INFO_T cnmGetBssInfoAndInit(P_ADAPTER_T prAdapter,
                                  ENUM_NETWORK_TYPE_T eNetworkType,
                                  u8 fgIsP2pDevice)
{
    P_BSS_INFO_T prBssInfo;
    u8 ucBssIndex, ucOwnMacIdx;

    ASSERT(prAdapter);

    /*specific case for p2p device scan*/
    if (eNetworkType == NETWORK_TYPE_P2P && fgIsP2pDevice) {
        prBssInfo = prAdapter->aprBssInfo[P2P_DEV_BSS_INDEX];

        prBssInfo->fgIsInUse = true;
        prBssInfo->ucBssIndex = P2P_DEV_BSS_INDEX;
        prBssInfo->eNetworkType = eNetworkType;
        prBssInfo->ucOwnMacIndex = HW_BSSID_NUM;
#if CFG_SUPPORT_PNO
        prBssInfo->fgIsPNOEnable = false;
        prBssInfo->fgIsNetRequestInActive = false;
#endif
        return prBssInfo;
    }

    /*reserve ownMAC0 for MBSS*/
    ucOwnMacIdx = (eNetworkType == NETWORK_TYPE_MBSS) ? 0 : 1;

    /* Find available HW set  with the order 1,2,..*/
    do {
        for (ucBssIndex = 0; ucBssIndex < BSS_INFO_NUM; ucBssIndex++) {
            prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

            if (prBssInfo && prBssInfo->fgIsInUse &&
                ucOwnMacIdx == prBssInfo->ucOwnMacIndex) {
                break;
            }
        }

        if (ucBssIndex >= BSS_INFO_NUM) {
            /* No hit  the ucOwnMacIndex could be assigned to this
             * new bss */
            break;
        }
    } while (++ucOwnMacIdx < HW_BSSID_NUM);

    /*should not dispatch P2P_DEV_BSS_INDEX (HW_BSSID_NUM)to general bss,
     * It means total BSS_INFO_NUM BSS are created,
     * no more reseve for MBSS
     */
    if (ucOwnMacIdx == HW_BSSID_NUM) {
        for (ucBssIndex = 0; ucBssIndex < BSS_INFO_NUM; ucBssIndex++) {
            prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

            /*If the Bss was alredy assigned, and in use*/
            if (prBssInfo && prBssInfo->fgIsInUse &&
                prBssInfo->ucOwnMacIndex == 0) {
                break;
            }
        }

        if (ucBssIndex >= BSS_INFO_NUM) {
            /*there is no NETWORK_TYPE_MBSS used before */
            DBGLOG(INIT,
                   WARN,
                   "[Warning] too much Bss in use, take reserve OwnMac(%d)for usage!\n",
                   ucOwnMacIdx);
            ucOwnMacIdx = 0;
        }
    }

    /* Find available BSS_INFO */
    for (ucBssIndex = 0; ucBssIndex < BSS_INFO_NUM; ucBssIndex++) {
        prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

        if (prBssInfo && !prBssInfo->fgIsInUse) {
            prBssInfo->fgIsInUse = true;
            prBssInfo->ucBssIndex = ucBssIndex;
            prBssInfo->eNetworkType = eNetworkType;
            prBssInfo->ucOwnMacIndex = ucOwnMacIdx;
#if (CFG_HW_WMM_BY_BSS == 1)
            prBssInfo->ucWmmQueSet = DEFAULT_HW_WMM_INDEX;
            prBssInfo->fgIsWmmInited = false;
#endif
            break;
        }
    }

    if (ucOwnMacIdx >= HW_BSSID_NUM || ucBssIndex >= BSS_INFO_NUM) {
        prBssInfo = NULL;
    }
#if CFG_SUPPORT_PNO
    if (prBssInfo) {
        prBssInfo->fgIsPNOEnable = false;
        prBssInfo->fgIsNetRequestInActive = false;
    }
#endif

#if CFG_SUPPORT_DFS
    if (prBssInfo) {
        cnmTimerInitTimer(prAdapter, &prBssInfo->rCsaTimer,
                          (PFN_MGMT_TIMEOUT_FUNC)rlmCsaTimeout,
                          (unsigned long)ucBssIndex);

        rlmResetCSAParams(prBssInfo);
    }
#endif

    return prBssInfo;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief    Search available HW ID and BSS_INFO structure and initialize
 *           these parameters, i.e., ucBssIndex, eNetworkType and ucOwnMacIndex
 *
 * @param (none)
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
void cnmFreeBssInfo(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo)
{
    ASSERT(prAdapter);
    ASSERT(prBssInfo);

#if CFG_SUPPORT_DFS
    cnmTimerStopTimer(prAdapter, &prBssInfo->rCsaTimer);
#endif

    prBssInfo->fgIsInUse = false;
}

#if CFG_SUPPORT_DBDC
void cnmInitDbdcSetting(IN P_ADAPTER_T prAdapter)
{
#if CFG_SUPPORT_DBDC_TC6
    P_CNM_INFO_T prCnmInfo;

    prCnmInfo = &prAdapter->rCnmInfo;
    prCnmInfo->fgSkipDbdcDisable = false;
#endif

    /* Parameter decision */
    switch (prAdapter->rWifiVar.ucDbdcMode) {
    case DBDC_MODE_DISABLED:
    case DBDC_MODE_DYNAMIC:
        cnmUpdateDbdcSetting(prAdapter, false);
#if CFG_SUPPORT_DBDC_TC6
        LINK_INITIALIZE(&prCnmInfo->rDbdcSwitchGuradPendingReqList);
#endif
        break;

    case DBDC_MODE_STATIC:
        cnmUpdateDbdcSetting(prAdapter, true);
        break;

    default:
        break;
    }
}

void cnmUpdateDbdcSetting(IN P_ADAPTER_T prAdapter, IN u8 fgDbdcEn)
{
    u8 ucWmmSetBitmap = 0;
    CMD_DBDC_SETTING_T rDbdcSetting;
    P_CMD_DBDC_SETTING_T prCmdBody;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u8 ucBssIndex;
    P_BSS_INFO_T prBssInfo;
#if !CFG_SUPPORT_DBDC_TC6
    u8 ucMaxBw;
#else
    P_AIS_FSM_INFO_T prAisFsmInfo;
#endif

    DBGLOG(CNM, STATE, "DBDC %s\n", fgDbdcEn ? "Enable" : "Disable");

#if CFG_SUPPORT_DBDC_TC6
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
#endif

    /* Parameter decision */
#if (CFG_HW_WMM_BY_BSS == 1)
    if (fgDbdcEn) {
        u8 ucWmmSetBitmapPerBSS;
        /*
         * As DBDC enabled, for BSS use 2.4g Band, assign related
         * WmmGroupSet bitmask to 1. This is used to indicate the
         * WmmGroupSet is associated to Band#1 (otherwise, use for
         * band#0)
         */
        for (ucBssIndex = 0; ucBssIndex < BSS_INFO_NUM; ucBssIndex++) {
            prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

            if (!prBssInfo || prBssInfo->fgIsInUse == false) {
                continue;
            }

            if (prBssInfo->eBand == BAND_2G4) {
                ucWmmSetBitmapPerBSS = prBssInfo->ucWmmQueSet;
                ucWmmSetBitmap |= BIT(ucWmmSetBitmapPerBSS);
            }
        }
        ucWmmSetBitmap |= BIT(MAX_HW_WMM_INDEX);  /* For P2P Device*/
    }
#else
    if (fgDbdcEn) {
        ucWmmSetBitmap |= BIT(DBDC_2G_WMM_INDEX);
    }
#endif

    /* Send event to FW */
    prCmdBody = (P_CMD_DBDC_SETTING_T)&rDbdcSetting;

    kalMemZero(prCmdBody, sizeof(CMD_DBDC_SETTING_T));

    prCmdBody->ucDbdcEn = fgDbdcEn;
    prCmdBody->ucWmmBandBitmap = ucWmmSetBitmap;

    if (!fgDbdcEn) {
        rStatus = wlanSendSetQueryCmd(
            prAdapter,  /* prAdapter */
            CMD_ID_SET_DBDC_PARMS,  /* ucCID */
            true,  /* fgSetQuery */
            false,  /* fgNeedResp */
            false,  /* fgIsOid */
            NULL,  /* pfCmdDoneHandler */
            NULL,  /* pfCmdTimeoutHandler */
            sizeof(CMD_DBDC_SETTING_T),  /* u4SetQueryInfoLen
                                          */
            (u8 *)prCmdBody,  /* pucInfoBuffer
                               */
            NULL,  /* pvSetQueryBuffer */
            0  /* u4SetQueryBufferLen */ );
    }

    for (ucBssIndex = 0; ucBssIndex <= HW_BSSID_NUM; ucBssIndex++) {
        prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

        /* TC6 DBDC Case: Switch DBDC mode, change Nss and reconnect to
         * AP */
        if (IS_BSS_AIS(prBssInfo) &&
            (ucBssIndex == prAdapter->prAisBssInfo->ucBssIndex)) {
            /* Update AIS NSS */
            prAdapter->rWifiVar.fgDbDcModeEn = fgDbdcEn;
            if (aisBssChangeNSS(prAdapter, fgDbdcEn) &&
                kalGetMediaStateIndicated(prAdapter->prGlueInfo) ==
                PARAM_MEDIA_STATE_CONNECTED &&
                !timerPendingTimer(&(
                                       prAisFsmInfo
                                       ->
                                       rIndicationOfDisconnectTimer))) {
                DBGLOG(CNM, STATE,
                       "[DBDC] station mode trigger reconnect\n");
                /* start DBDC reconnect countdown timer */
                cnmTimerStartTimer(
                    prAdapter,
                    &prAdapter->rWifiVar
                    .rDBDCReconnectCountDown,
                    DBDC_AIS_REASSOC_COUNTDOWN_TIME);
                prBssInfo->fgReConnBypassScan = 1;
                prBssInfo->u2DeauthReason =
                    BEACON_TIMEOUT_REASON_DUE_2_DBDC_RECONNECT;

                if (!timerPendingTimer(
                        &prAdapter->rWifiVar.rAisFsmInfo
                        .rBeaconLostTimer)) {
                    cnmTimerStartTimer(
                        prAdapter,
                        &prAdapter->rWifiVar.rAisFsmInfo
                        .rBeaconLostTimer,
                        prAdapter->rWifiVar
                        .ucWaitConnect *
                        MSEC_PER_SEC);
                }

                kalIndicateStatusAndComplete(
                    prAdapter->prGlueInfo,
                    WLAN_STATUS_BEACON_TIMEOUT, NULL, 0);
            }
            DBGLOG(CNM,
                   STATE,
                   "[DBDC %s] station mode is on and NSS:%d, AIS BssIndex:%d\n",
                   fgDbdcEn ? "Enable" : "Disable",
                   prBssInfo->ucNss,
                   prAdapter->prAisBssInfo->ucBssIndex);
        }
    }

    if (fgDbdcEn) {
        rStatus = wlanSendSetQueryCmd(
            prAdapter,  /* prAdapter */
            CMD_ID_SET_DBDC_PARMS,  /* ucCID */
            true,  /* fgSetQuery */
            false,  /* fgNeedResp */
            false,  /* fgIsOid */
            NULL,  /* pfCmdDoneHandler */
            NULL,  /* pfCmdTimeoutHandler */
            sizeof(CMD_DBDC_SETTING_T),  /* u4SetQueryInfoLen
                                          */
            (u8 *)prCmdBody,  /* pucInfoBuffer
                               */
            NULL,  /* pvSetQueryBuffer */
            0  /* u4SetQueryBufferLen */ );
    }

    /*DBGLOG(CNM, INFO, "DBDC CMD status %u\n", rStatus);*/

    if (rStatus == WLAN_STATUS_SUCCESS || rStatus == WLAN_STATUS_PENDING) {
        for (ucBssIndex = 0; ucBssIndex <= HW_BSSID_NUM; ucBssIndex++) {
            prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

            if (prBssInfo->eBand == BAND_2G4) {
                if (fgDbdcEn) {
                    prBssInfo->eDBDCBand = ENUM_BAND_1;
                }else{
                    prBssInfo->eDBDCBand = ENUM_BAND_0;
                }
            }
        }
    }
}

void cnmGetDbdcCapability(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                          IN ENUM_BAND_T eRfBand, IN u8 ucPrimaryChannel,
                          IN u8 ucNss, OUT P_CNM_DBDC_CAP_T prDbdcCap)
{
    if (!prDbdcCap) {
        return;
    }

    /* BSS index */
    prDbdcCap->ucBssIndex = ucBssIndex;

#if (CFG_HW_WMM_BY_BSS == 0)
    /* WMM set */
    if (eRfBand == BAND_5G) {
        prDbdcCap->ucWmmSetIndex = DBDC_5G_WMM_INDEX;
    } else {
        prDbdcCap->ucWmmSetIndex =
            (prAdapter->rWifiVar.ucDbdcMode == DBDC_MODE_DISABLED) ?
            DBDC_5G_WMM_INDEX :
            DBDC_2G_WMM_INDEX;
    }
#endif

    /* Nss & band 0/1 */
    switch (prAdapter->rWifiVar.ucDbdcMode) {
    case DBDC_MODE_DISABLED:
        /* DBDC is disabled, all BSS run on band 0 */
        if (wlanGetSupportNss(prAdapter, ucBssIndex) < ucNss) {
            prDbdcCap->ucNss =
                wlanGetSupportNss(prAdapter, ucBssIndex);
        } else {
            prDbdcCap->ucNss = ucNss;
        }
        break;

    case DBDC_MODE_STATIC:
        /* Static DBDC mode, 1SS only */
        prDbdcCap->ucNss = 1;
        break;

    case DBDC_MODE_DYNAMIC:
        if (prAdapter->rWifiVar.fgDbDcModeEn) {
            prDbdcCap->ucNss = 1;
        } else {
            prDbdcCap->ucNss =
                wlanGetSupportNss(prAdapter, ucBssIndex);
        }
        break;

    default:
        break;
    }
    DBGLOG(CNM, INFO, "[DBDC] mode:%d enable:%s NSS:%d \n",
           prAdapter->rWifiVar.ucDbdcMode,
           prAdapter->rWifiVar.fgDbDcModeEn ? "Enable" : "Disable",
           prDbdcCap->ucNss);
}

void cnmDbdcEnableDecision(IN P_ADAPTER_T prAdapter, IN u8 ucChangedBssIndex,
                           IN ENUM_BAND_T eRfBand)
{
    P_BSS_INFO_T prBssInfo;
    u8 ucBssIndex;

    DBGLOG(CNM, STATE, "[DBDC][Enable Decison] mode:%d enable:%s \n",
           prAdapter->rWifiVar.ucDbdcMode,
           prAdapter->rWifiVar.fgDbDcModeEn ? "Enable" : "Disable");

    if ((prAdapter->rWifiVar.ucDbdcMode != DBDC_MODE_DYNAMIC) &&
        (prAdapter->rWifiVar.ucDbdcMode != DBDC_MODE_STATIC)) {
        return;
    }

    if (prAdapter->rWifiVar.fgDbDcModeEn) {
        if (timerPendingTimer(
                &prAdapter->rWifiVar.rDBDCSwitchGuardTimer)) {
            /* update timer for connection retry */
            DBGLOG(CNM, INFO, "DBDC guard time extend\n");
            cnmTimerStopTimer(
                prAdapter,
                &prAdapter->rWifiVar.rDBDCSwitchGuardTimer);
            cnmTimerStartTimer(
                prAdapter,
                &prAdapter->rWifiVar.rDBDCSwitchGuardTimer,
                DBDC_SWITCH_GUARD_TIME);
        }

#if CFG_SUPPORT_DBDC_TC6
        if (timerPendingTimer(
                &prAdapter->rWifiVar.rDBDCReconnectCountDown)) {
            DBGLOG(CNM, INFO,
                   "DBDC reconnect timer protection stop\n");
            cnmTimerStopTimer(
                prAdapter,
                &prAdapter->rWifiVar.rDBDCReconnectCountDown);
        }

        if (timerPendingTimer(
                &prAdapter->rWifiVar.rDBDCDisableCountdownTimer)) {
            DBGLOG(CNM,
                   INFO,
                   "DBDC disable count down stop for SoftAP re-start\n");
            cnmTimerStopTimer(
                prAdapter,
                &prAdapter->rWifiVar.rDBDCDisableCountdownTimer);
        }
#endif

        /* DBDC is already ON, so renew WMM band information only */
        cnmUpdateDbdcSetting(prAdapter, true);
        return;
    }

    if (timerPendingTimer(&prAdapter->rWifiVar.rDBDCSwitchGuardTimer)) {
        cnmTimerStopTimer(prAdapter,
                          &prAdapter->rWifiVar.rDBDCSwitchGuardTimer);

        cnmUpdateDbdcSetting(prAdapter, true);

        /* Extend guard timer for connection retry */
        DBGLOG(CNM, STATE,
               "DBDC switch guard time extend for SoftAP Re-Start\n");
        cnmTimerStartTimer(prAdapter,
                           &prAdapter->rWifiVar.rDBDCSwitchGuardTimer,
                           DBDC_SWITCH_GUARD_TIME);
        return;
    }

    if (eRfBand != BAND_2G4 && eRfBand != BAND_5G) {
        return;
    }

    for (ucBssIndex = 0; ucBssIndex < HW_BSSID_NUM; ucBssIndex++) {
        if (ucBssIndex == ucChangedBssIndex) {
            continue;
        }

        prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

        if (!prBssInfo->fgIsInUse || !prBssInfo->fgIsNetActive ||
            (prBssInfo->eConnectionState !=
             PARAM_MEDIA_STATE_CONNECTED &&
             prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT)) {
            continue;
        }

        if (prBssInfo->eBand != BAND_2G4 && prBssInfo->eBand != BAND_5G) {
            continue;
        }

        if (prBssInfo->eBand != eRfBand) {
            /* Enable DBDC */

            /* if disable timer exist, close it*/
            if (timerPendingTimer(
                    &prAdapter->rWifiVar
                    .rDBDCDisableCountdownTimer)) {
                cnmTimerStopTimer(
                    prAdapter,
                    &prAdapter->rWifiVar
                    .rDBDCDisableCountdownTimer);
            }

            cnmUpdateDbdcSetting(prAdapter, true);

            /* Start Switch Guard Time */
            DBGLOG(CNM, INFO,
                   "Start DBDC Switch Guard timer for DBDC Enable\n");
            cnmTimerStartTimer(
                prAdapter,
                &prAdapter->rWifiVar.rDBDCSwitchGuardTimer,
                DBDC_SWITCH_GUARD_TIME);
            return;
        }
    }
}

void cnmDbdcDisableDecision(IN P_ADAPTER_T prAdapter, IN u8 ucChangedBssIndex)
{
    P_BSS_INFO_T prBssInfo;
    u8 ucBssIndex;
    ENUM_BAND_T eBandCompare;
#if CFG_SUPPORT_DBDC_TC6
    P_CNM_INFO_T prCnmInfo;
#endif

    DBGLOG(CNM, STATE, "[DBDC][Disable Decision] mode:%d enable:%s \n",
           prAdapter->rWifiVar.ucDbdcMode,
           prAdapter->rWifiVar.fgDbDcModeEn ? "Enable" : "Disable");

#if CFG_SUPPORT_DBDC_TC6
    prCnmInfo = &prAdapter->rCnmInfo;
#endif

    if (prAdapter->rWifiVar.ucDbdcMode != DBDC_MODE_DYNAMIC) {
        return;
    }

    if (!prAdapter->rWifiVar.fgDbDcModeEn) {
        DBGLOG(CNM, STATE, "[DBDC] Keep DBDC status [%s]\n",
               prAdapter->rWifiVar.fgDbDcModeEn ? "Enable" : "Disable");
        return;
    }

    if (timerPendingTimer(&prAdapter->rWifiVar.rDBDCDisableCountdownTimer)) {
        return;
    }

    if (timerPendingTimer(&prAdapter->rWifiVar.rDBDCSwitchGuardTimer)) {
        return;
    }

#if CFG_SUPPORT_DBDC_TC6
    if (timerPendingTimer(&prAdapter->rWifiVar.rDBDCReconnectCountDown)) {
        return;
    }

#endif

    eBandCompare = BAND_NULL;
    for (ucBssIndex = 0; ucBssIndex < HW_BSSID_NUM; ucBssIndex++) {
        if (ucBssIndex == ucChangedBssIndex) {
            continue;
        }

        prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

        if (!prBssInfo->fgIsInUse || !prBssInfo->fgIsNetActive ||
            (prBssInfo->eConnectionState !=
             PARAM_MEDIA_STATE_CONNECTED &&
             prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT)) {
            continue;
        }

        if (prBssInfo->eBand != BAND_2G4 && prBssInfo->eBand != BAND_5G) {
            continue;
        }

        if (eBandCompare == BAND_NULL) {
            eBandCompare = prBssInfo->eBand;
            continue;
        }

        if (prBssInfo->eBand != eBandCompare) {
            /*no need to disable DBDC*/
            return;
        }
    }

#if CFG_SUPPORT_DBDC_TC6
    if (prCnmInfo->fgSkipDbdcDisable) {
        DBGLOG(CNM, INFO, "Skip DBDC disable for Beacon Timeout\n");
        cnmTimerStartTimer(
            prAdapter, &prAdapter->rWifiVar.rDBDCReconnectCountDown,
            DBDC_AIS_BCN_TIMEOUT_RECONNECT_COUNTDOWN_TIME);
    } else
#endif
    {
        /* start DBDC disable countdown timer */
        DBGLOG(CNM, INFO, "Start DBDC disable countdown timer\n");
        cnmTimerStartTimer(
            prAdapter,
            &prAdapter->rWifiVar.rDBDCDisableCountdownTimer,
            DBDC_DISABLE_COUNTDOWN_TIME);
    }
}

void cnmDbdcDecision(IN P_ADAPTER_T prAdapter, IN unsigned long plParamPtr)
{
    P_BSS_INFO_T prBssInfo;
    u8 ucBssIndex;
    ENUM_BAND_T eBandCompare;
#if CFG_SUPPORT_DBDC_TC6
    P_MSG_CH_REQ_T prPendingMsg;
    P_MSG_HDR_T prMsgHdr;
#endif

    DBGLOG(CNM, STATE, "[DBDC] mode:%d enable:%s \n",
           prAdapter->rWifiVar.ucDbdcMode,
           prAdapter->rWifiVar.fgDbDcModeEn ? "Enable" : "Disable");

    if (prAdapter->rWifiVar.ucDbdcMode != DBDC_MODE_DYNAMIC) {
        return;
    }

    if (plParamPtr == DBDC_DECISION_TIMER_SWITCH_GUARD_TIME) {
        DBGLOG(CNM, INFO,
               "DBDC timer timeout : switch guard time end\n");

#if CFG_SUPPORT_DBDC_TC6
        while (!LINK_IS_EMPTY(
                   &prAdapter->rCnmInfo.
                   rDbdcSwitchGuradPendingReqList)) {
            LINK_REMOVE_HEAD(
                &prAdapter->rCnmInfo
                .rDbdcSwitchGuradPendingReqList,
                prMsgHdr, P_MSG_HDR_T);

            if (prMsgHdr) {
                prPendingMsg = (P_MSG_CH_REQ_T)prMsgHdr;

                DBGLOG(CNM,
                       STATE,
                       "[DBDC] ChReq: send queued REQ of BSS %u Token %u\n",
                       prPendingMsg->ucBssIndex,
                       prPendingMsg->ucTokenID);

                cnmChMngrRequestPrivilege(
                    prAdapter, &prPendingMsg->rMsgHdr);
            } else {
                ASSERT(0);
            }
        }
#endif
    } else if (plParamPtr == DBDC_DECISION_TIMER_DISABLE_COUNT_DOWN) {
        DBGLOG(CNM, INFO,
               "DBDC timer timeout : disable countdown finish\n");
    }
#if CFG_SUPPORT_DBDC_TC6
    else if (plParamPtr == DBDC_DECISION_TIMER_RECONNECT_COUNT_DOWN) {
        DBGLOG(CNM, INFO,
               "DBDC timer timeout : reconnect countdown finish\n");
        return;
    } else if (plParamPtr == DBDC_DECISION_TIMER_AIS_CONNECT_COUNT_DOWN) {
        DBGLOG(CNM, INFO,
               "DBDC timer timeout : AIS connect countdown finish\n");
        return;
    }
#endif

    eBandCompare = BAND_NULL;
    for (ucBssIndex = 0; ucBssIndex < HW_BSSID_NUM; ucBssIndex++) {
        prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

        if (!prBssInfo->fgIsInUse || !prBssInfo->fgIsNetActive ||
            (prBssInfo->eConnectionState !=
             PARAM_MEDIA_STATE_CONNECTED &&
             prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT)) {
            continue;
        }

        if (prBssInfo->eBand != BAND_2G4 && prBssInfo->eBand != BAND_5G) {
            continue;
        }

        if (eBandCompare == BAND_NULL) {
            eBandCompare = prBssInfo->eBand;
            continue;
        }

        if (prBssInfo->eBand != eBandCompare) {
            DBGLOG(CNM, INFO, "exist two different bands\n");
            if (!prAdapter->rWifiVar.fgDbDcModeEn) {
#if CFG_SUPPORT_DBDC_TC6
                if (timerPendingTimer(
                        &prAdapter->rWifiVar
                        .rDBDCReconnectCountDown)) {
                    DBGLOG(CNM,
                           INFO,
                           "DBDC-ReconnectCountDown still on, skip DBDC Enable this time\n");
                    return;
                }

                if (timerPendingTimer(
                        &prAdapter->rWifiVar
                        .rDBDCAisConnectCountDown)) {
                    DBGLOG(CNM,
                           INFO,
                           "DBDC-AISconnectCountDown still on, skip DBDC Enable this time\n");
                    return;
                }
#endif
                /* DBDC Enable */
                cnmUpdateDbdcSetting(prAdapter, true);
                DBGLOG(CNM,
                       STATE,
                       "Start DBDC Switch Guard timer for DBDC Enable\n");
                cnmTimerStartTimer(
                    prAdapter,
                    &prAdapter->rWifiVar
                    .rDBDCSwitchGuardTimer,
                    DBDC_SWITCH_GUARD_TIME);
            }
            return;
        }
    }

    if (prAdapter->rWifiVar.fgDbDcModeEn) {
#if CFG_SUPPORT_DBDC_TC6
        if (timerPendingTimer(
                &prAdapter->rWifiVar.rDBDCReconnectCountDown)) {
            DBGLOG(CNM,
                   INFO,
                   "DBDC-ReconnectCountDown still on, skip DBDC Disable this time\n");
            return;
        }

        if (timerPendingTimer(
                &prAdapter->rWifiVar.rDBDCAisConnectCountDown)) {
            DBGLOG(CNM,
                   INFO,
                   "DBDC-AISconnectCountDown still on, skip DBDC Disable this time\n");
            return;
        }
#endif
        /* DBDC Disable */
        cnmUpdateDbdcSetting(prAdapter, false);
        DBGLOG(CNM, STATE,
               "Start DBDC Switch Guard timer for DBDC Disable\n");
        cnmTimerStartTimer(prAdapter,
                           &prAdapter->rWifiVar.rDBDCSwitchGuardTimer,
                           DBDC_SWITCH_GUARD_TIME);
    }
    return;
}

#endif

#if (CFG_HW_WMM_BY_BSS == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief    Search available HW WMM index.
 *
 * @param (none)
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
u8 cnmWmmIndexDecision(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
    u8 ucWmmIndex;

    for (ucWmmIndex = 0; ucWmmIndex < HW_WMM_NUM; ucWmmIndex++) {
        if (prBssInfo && prBssInfo->fgIsInUse &&
            prBssInfo->fgIsWmmInited == false) {
            if (!(prAdapter->ucHwWmmEnBit & BIT(ucWmmIndex))) {
                prAdapter->ucHwWmmEnBit |= BIT(ucWmmIndex);
                prBssInfo->fgIsWmmInited = true;
                break;
            }
        }
    }
    return (ucWmmIndex < HW_WMM_NUM) ? ucWmmIndex : MAX_HW_WMM_INDEX;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief    Free BSS HW WMM index.
 *
 * @param (none)
 *
 * @return None
 */
/*----------------------------------------------------------------------------*/
void cnmFreeWmmIndex(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
    prAdapter->ucHwWmmEnBit &= (~BIT(prBssInfo->ucWmmQueSet));
    prBssInfo->ucWmmQueSet = DEFAULT_HW_WMM_INDEX;
    prBssInfo->fgIsWmmInited = false;
}
#endif

#if CFG_SUPPORT_DBDC_TC6
u8 cnmSapIsConcurrent(IN P_ADAPTER_T prAdapter)
{
    if (prAdapter) {
        return prAdapter->u4P2pMode == RUNNING_P2P_AP_MODE;
    }else{
        return false;
    }
}

u8 cnmSapIsActive(IN P_ADAPTER_T prAdapter)
{
    return cnmGetp2pSapBssInfo(prAdapter) != NULL;
}

P_BSS_INFO_T cnmGetp2pSapBssInfo(IN P_ADAPTER_T prAdapter)
{
    P_BSS_INFO_T prBssInfo;
    u8 i;

    if (!prAdapter) {
        return NULL;
    }

    for (i = 0; i < BSS_INFO_NUM; i++) {
        prBssInfo = prAdapter->aprBssInfo[i];

        if (prBssInfo && IS_BSS_P2P(prBssInfo) &&
            p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings
                            [prBssInfo->u4PrivateData]) &&
            IS_NET_PWR_STATE_ACTIVE(prAdapter, prBssInfo->ucBssIndex)) {
            return prBssInfo;
        }
    }

    return NULL;
}

void cnmSapChannelSwitchReq(IN P_ADAPTER_T prAdapter,
                            IN P_RF_CHANNEL_INFO_T prRfChannelInfo,
                            IN u8 ucRoleIdx)
{
    P_GLUE_INFO_T prGlueInfo = prAdapter->prGlueInfo;
    P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;
    P_MSG_P2P_SET_NEW_CHANNEL_T prP2pSetNewChannelMsg =
        (P_MSG_P2P_SET_NEW_CHANNEL_T)NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_P2P_CONNECTION_REQ_INFO_T prP2pConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;
    u8 ucBssIdx = 0;

    DBGLOG(P2P, INFO, "role(%d) c=%d b=%d opw=%d\n", ucRoleIdx,
           prRfChannelInfo->ucChannelNum, prRfChannelInfo->eBand,
           prRfChannelInfo->ucChnlBw);

    /* Free chandef buffer */
    if (!prGlueInfo) {
        DBGLOG(P2P, WARN, "glue info is not active\n");
        return;
    }
    prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];
    if (!prGlueP2pInfo) {
        DBGLOG(P2P, WARN, "p2p glue info is not active\n");
        return;
    }
    if (prGlueP2pInfo->chandef != NULL) {
        if (prGlueP2pInfo->chandef->chan) {
            cnmMemFree(prGlueInfo->prAdapter,
                       prGlueP2pInfo->chandef->chan);
            prGlueP2pInfo->chandef->chan = NULL;
        }
        cnmMemFree(prGlueInfo->prAdapter, prGlueP2pInfo->chandef);
        prGlueP2pInfo->chandef = NULL;
    }

    /* Fill conn info */
    prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx);
    if (!prP2pRoleFsmInfo) {
        return;
    }

    prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
    if (!prP2pConnReqInfo) {
        return;
    }

    prP2pConnReqInfo->rChannelInfo.ucChannelNum =
        prRfChannelInfo->ucChannelNum;
    prP2pConnReqInfo->rChannelInfo.eBand = prRfChannelInfo->eBand;
    prP2pConnReqInfo->eChnlBw = prRfChannelInfo->ucChnlBw;

    p2pFuncSetDfsState(DFS_STATE_INACTIVE);

    if (p2pFuncRoleToBssIdx(prAdapter, ucRoleIdx, &ucBssIdx) !=
        WLAN_STATUS_SUCCESS) {
        DBGLOG(P2P, WARN, "Incorrect role index");
        return;
    }

    /* Set CSA IE */
    prAdapter->rWifiVar.fgCsaInProgress = true;
    prAdapter->rWifiVar.ucChannelSwitchMode = 1;
    prAdapter->rWifiVar.ucNewChannelNumber = prRfChannelInfo->ucChannelNum;
    prAdapter->rWifiVar.ucChannelSwitchCount = 5;

    /* Set new channel parameters */
    prP2pSetNewChannelMsg = (P_MSG_P2P_SET_NEW_CHANNEL_T)cnmMemAlloc(
        prGlueInfo->prAdapter, RAM_TYPE_MSG,
        sizeof(*prP2pSetNewChannelMsg));

    if (prP2pSetNewChannelMsg == NULL) {
        DBGLOG(P2P, WARN, "prP2pSetNewChannelMsg alloc fail\n");
        return;
    }

    prP2pSetNewChannelMsg->rMsgHdr.eMsgId = MID_MNY_P2P_SET_NEW_CHANNEL;
    prP2pSetNewChannelMsg->eChannelWidth =
        (ENUM_CHANNEL_WIDTH_T)rlmGetVhtOpBwByBssOpBw(
            prRfChannelInfo->ucChnlBw);
    prP2pSetNewChannelMsg->ucRoleIdx = ucRoleIdx;
    prP2pSetNewChannelMsg->ucBssIndex = ucBssIdx;
    mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                (P_MSG_HDR_T)prP2pSetNewChannelMsg, MSG_SEND_METHOD_BUF);

    kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);

    /* Send Action Frame */
    rlmSendChannelSwitchFrame(prAdapter, ucBssIdx);

    /* Update Beacon */
    bssUpdateBeaconContent(prAdapter, ucBssIdx);
}

u8 cnmIdcCsaReq(IN P_ADAPTER_T prAdapter, IN u8 ch_num, IN u8 ucRoleIdx)
{
    P_BSS_INFO_T prBssInfo = NULL;
    u8 ucBssIdx = 0;
    RF_CHANNEL_INFO_T rRfChnlInfo;

    ASSERT(ch_num);

    if (p2pFuncRoleToBssIdx(prAdapter, ucRoleIdx, &ucBssIdx) !=
        WLAN_STATUS_SUCCESS) {
        return -1;
    }

    DBGLOG(REQ, INFO, "[CSA]RoleIdx = %d ,CH = %d BssIdx = %d\n", ucRoleIdx,
           ch_num, ucBssIdx);

    prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

    if (prBssInfo->ucPrimaryChannel != ch_num) {
        rRfChnlInfo.ucChannelNum = ch_num;
        rRfChnlInfo.eBand =
            (rRfChnlInfo.ucChannelNum <= 14) ? BAND_2G4 : BAND_5G;
        rRfChnlInfo.ucChnlBw = MAX_BW_20MHZ;
        rRfChnlInfo.u2PriChnlFreq = nicChannelNum2Freq(ch_num) / 1000;
        rRfChnlInfo.u4CenterFreq1 = rRfChnlInfo.u2PriChnlFreq;
        rRfChnlInfo.u4CenterFreq2 = 0;

        DBGLOG(REQ, ERROR,
               "[CSA]CH=%d,Band=%d,BW=%d,PriFreq=%d,S1=%d\n",
               rRfChnlInfo.ucChannelNum, rRfChnlInfo.eBand,
               rRfChnlInfo.ucChnlBw, rRfChnlInfo.u2PriChnlFreq,
               rRfChnlInfo.u4CenterFreq1);

        prAdapter->rWifiVar.ucNewChannelNumber = ch_num;
        cnmSapChannelSwitchReq(prAdapter, &rRfChnlInfo, ucRoleIdx);

        return 0;  /* Return Success */
    } else {
        DBGLOG(CNM, INFO, "[CSA]Req CH = cur CH:%d, Stop Req\n",
               prBssInfo->ucPrimaryChannel);
        return -1;
    }
}

#endif
