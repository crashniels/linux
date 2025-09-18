// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "saa_fsm.c"
 *    \brief  This file defines the FSM for SAA MODULE.
 *
 *    This file defines the FSM for SAA MODULE.
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

#if !DBG_DISABLE_ALL_LOG
static u8 *apucDebugAAState[AA_STATE_NUM] = {
    (u8 *)DISP_STRING("AA_IDLE"), (u8 *)DISP_STRING("SAA_SEND_AUTH1"),
    (u8 *)DISP_STRING("SAA_WAIT_AUTH2"), (u8 *)DISP_STRING("SAA_SEND_AUTH3"),
    (u8 *)DISP_STRING("SAA_WAIT_AUTH4"), (u8 *)DISP_STRING("SAA_SEND_ASSOC1"),
    (u8 *)DISP_STRING("SAA_WAIT_ASSOC2"), (u8 *)DISP_STRING("AAA_SEND_AUTH2"),
    (u8 *)DISP_STRING("AAA_SEND_AUTH4"), (u8 *)DISP_STRING("AAA_SEND_ASSOC2"),
    (u8 *)DISP_STRING("AA_RESOURCE")
};
#endif
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
/*
 * @brief prepare to send authentication or association frame
 *
 * @param[in] prStaRec Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void saaSendAuthAssoc(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
    /* This function do the things like
     * "case SAA_STATE_SEND_AUTH1/ASSOC1" in SAA FSM steps
     */

    u32 rStatus = WLAN_STATUS_FAILURE;
    P_CONNECTION_SETTINGS_T prConnSettings = NULL;
    u16 u2AuthTransSN = AUTH_TRANSACTION_SEQ_1;  /* default for OPEN */
    P_BSS_DESC_T prBssDesc = NULL;
    P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo = NULL;
    PARAM_SSID_T rParamSsid;
#if CFG_SUPPORT_H2E
    u16 u2AuthStatusCode = STATUS_CODE_RESERVED;
#endif

    ASSERT(prAdapter);
    ASSERT(prStaRec);

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    DBGLOG(SAA, INFO, "[SAA]saaSendAuthAssoc, StaState:%d\n",
           prStaRec->ucStaState);

    if (prStaRec->ucTxAuthAssocRetryCount >=
        prStaRec->ucTxAuthAssocRetryLimit) {
        /* Record the Status Code of Authentication Request */
        prStaRec->u2StatusCode = (prConnSettings->fgIsSendAssoc) ?
                                 STATUS_CODE_ASSOC_TIMEOUT :
                                 STATUS_CODE_AUTH_TIMEOUT;

        if (saaFsmSendEventJoinComplete(prAdapter, WLAN_STATUS_FAILURE,
                                        prStaRec,
                                        NULL) == WLAN_STATUS_RESOURCES) {
            /* can set a timer and retry later */
            DBGLOG(SAA, WARN,
                   "[SAA]can't alloc msg for inform AIS join complete\n");
        }
    } else {
        prStaRec->ucTxAuthAssocRetryCount++;
        /* Prepare to send authentication frame */
        if (!prConnSettings->fgIsSendAssoc) {
            /* Fill authentication transaction sequence number
             * depends on auth type
             */
            if (((prAdapter->prGlueInfo->rWpaInfo.u4AuthAlg & AUTH_TYPE_SAE) ||
                 (prAdapter->prGlueInfo->rWpaInfo.u4AuthAlg &
                  AUTH_TYPE_SHARED_KEY)) &&
                prConnSettings->ucAuthDataLen) {
                kalMemCopy(&u2AuthTransSN, prConnSettings->aucAuthData,
                           AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN);
                DBGLOG(SAA, INFO, "[SAA]Get auth SN = %d from Conn Settings\n",
                       u2AuthTransSN);
            }

#if CFG_SUPPORT_H2E
            if (prAdapter->prGlueInfo->rWpaInfo.u4AuthAlg & AUTH_TYPE_SAE) {
                kalMemCopy(&u2AuthStatusCode, &prConnSettings->aucAuthData[2],
                           AUTH_STATUS_CODE_FIELD_LEN);
                DBGLOG(SAA, INFO,
                       "[SAA]Get auth StatusCode=%d from Conn Settings\n",
                       u2AuthStatusCode);
            }
#endif

            /* Update Station Record - Class 1 Flag */
            if (prStaRec->ucStaState != STA_STATE_1) {
                DBGLOG(SAA, WARN,
                       "[SAA]Rx send auth CMD at unexpect state:%d\n",
                       prStaRec->ucStaState);
                cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);
            }
#if !CFG_SUPPORT_AAA
            rStatus = authSendAuthFrame(prAdapter, prStaRec, u2AuthTransSN);
#else
            rStatus = authSendAuthFrame(prAdapter, prStaRec,
                                        prStaRec->ucBssIndex, NULL,
                                        u2AuthTransSN,
#if CFG_SUPPORT_H2E
                                        u2AuthStatusCode);
#else
                                        STATUS_CODE_RESERVED);
#endif
#endif
            prStaRec->eAuthAssocSent = u2AuthTransSN;
        } else {  /* Prepare to send association frame */
            /* Fill Cipher/AKM before sending association request,
             * copy fro m AIS search step
             */
            if (prConnSettings->ucSSIDLen) {
                rParamSsid.u4SsidLen = prConnSettings->ucSSIDLen;
                COPY_SSID(rParamSsid.aucSsid, rParamSsid.u4SsidLen,
                          prConnSettings->aucSSID, prConnSettings->ucSSIDLen);
                prBssDesc = scanSearchBssDescByBssidAndSsid(
                    prAdapter, prStaRec->aucMacAddr, true, &rParamSsid);
                DBGLOG(RSN, INFO,
                       "[RSN]saaSendAuthAssoc,"
                       "prBssDesc[" MACSTR " ,%s] Searched by"
                       " BSSID[" MACSTR "] & SSID %s.\n",
                       MAC2STR(prBssDesc->aucBSSID), prBssDesc->aucSSID,
                       MAC2STR(prStaRec->aucMacAddr), prConnSettings->aucSSID);
            } else {
                prBssDesc = scanSearchBssDescByBssidAndChanNum(
                    prAdapter, prStaRec->aucMacAddr, true,
                    prConnSettings->ucChannelNum);
                DBGLOG(RSN, INFO,
                       "[RSN]saaSendAuthAssoc,"
                       "prBssDesc[" MACSTR " ,%s] Searched by"
                       " BSSID[" MACSTR "] & ChanNum %d.\n",
                       MAC2STR(prBssDesc->aucBSSID), prBssDesc->aucSSID,
                       MAC2STR(prStaRec->aucMacAddr),
                       prConnSettings->ucChannelNum);
            }

            prAisSpecBssInfo = &(prAdapter->rWifiVar.rAisSpecificBssInfo);
            if (rsnPerformPolicySelection(prAdapter, prBssDesc)) {
                if (prAisSpecBssInfo->fgCounterMeasure) {
                    DBGLOG(RSN, WARN, "Skip whle at counter measure perid\n");
                } else {
                    DBGLOG(RSN, INFO, "Bss RSN matched!\n");
                    prAdapter->prAisBssInfo->u4RsnSelectedGroupCipher =
                        prBssDesc->u4RsnSelectedGroupCipher;
                    prAdapter->prAisBssInfo->u4RsnSelectedPairwiseCipher =
                        prBssDesc->u4RsnSelectedPairwiseCipher;
                    prAdapter->prAisBssInfo->u4RsnSelectedAKMSuite =
                        prBssDesc->u4RsnSelectedAKMSuite;
                }
            } else {
                DBGLOG(RSN, WARN, "Bss fail for RSN check\n");
            }

            if (prStaRec->ucStaState == STA_STATE_1) {
                /* don't change to state2 for reassociation */
                /* Update Station Record - Class 2 Flag */
                cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);
            }

            rStatus = assocSendReAssocReqFrame(prAdapter, prStaRec);
            prStaRec->eAuthAssocSent = AA_SENT_ASSOC1;
        }

        if (rStatus != WLAN_STATUS_SUCCESS) {
            /* maybe can't alloc msdu info, retry after timeout */
            cnmTimerInitTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                              (PFN_MGMT_TIMEOUT_FUNC)saaFsmRunEventTxReqTimeOut,
                              (unsigned long)prStaRec);

            cnmTimerStartTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                               TU_TO_MSEC(TX_AUTHENTICATION_RETRY_TIMEOUT_TU));
        }
    }
}

void saaSendAuthSeq3(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
    u32 rStatus = WLAN_STATUS_FAILURE;

    ASSERT(prAdapter);
    ASSERT(prStaRec);

    DBGLOG(SAA, INFO, "[SAA]send auth 3\n");

    if (prStaRec->ucTxAuthAssocRetryCount >=
        prStaRec->ucTxAuthAssocRetryLimit) {
        /* Record the Status Code of Auth Request */
        prStaRec->u2StatusCode = STATUS_CODE_AUTH_TIMEOUT;
    } else {
        prStaRec->ucTxAuthAssocRetryCount++;

#if !CFG_SUPPORT_AAA
        rStatus =
            authSendAuthFrame(prAdapter, prStaRec, AUTH_TRANSACTION_SEQ_3);
#else
        rStatus = authSendAuthFrame(prAdapter, prStaRec, prStaRec->ucBssIndex,
                                    NULL, AUTH_TRANSACTION_SEQ_3,
                                    STATUS_CODE_RESERVED);
#endif

        prStaRec->eAuthAssocSent = AA_SENT_AUTH3;

        if (rStatus != WLAN_STATUS_SUCCESS) {
            cnmTimerInitTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                              (PFN_MGMT_TIMEOUT_FUNC)saaFsmRunEventTxReqTimeOut,
                              (unsigned long)prStaRec);

            cnmTimerStartTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                               TU_TO_MSEC(TX_AUTHENTICATION_RETRY_TIMEOUT_TU));
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief The Core FSM engine of SAA Module.
 *
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 * @param[in] eNextState         The value of Next State
 * @param[in] prRetainedSwRfb    Pointer to the retained SW_RFB_T for JOIN
 * Success
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void saaFsmSteps(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec,
                 IN ENUM_AA_STATE_T eNextState, IN P_SW_RFB_T prRetainedSwRfb)
{
    WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
    ENUM_AA_STATE_T ePreviousState;
    u8 fgIsTransition;

    ASSERT(prStaRec);
    if (!prStaRec || g_u4HaltFlag) {
        return;
    }

    do {
        DBGLOG(SAA, STATE, "[SAA]TRANSITION: [%s] -> [%s]\n",
               apucDebugAAState[prStaRec->eAuthAssocState],
               apucDebugAAState[eNextState]);

        ePreviousState = prStaRec->eAuthAssocState;

        /* NOTE(Kevin): This is the only place to change the
         * eAuthAssocState(except initial) */
        prStaRec->eAuthAssocState = eNextState;

        fgIsTransition = (u8)false;
        switch (prStaRec->eAuthAssocState) {
        case AA_STATE_IDLE:

            if (ePreviousState != prStaRec->eAuthAssocState) {  /* Only
                                                                 * trigger
                                                                 * this
                                                                 * event
                                                                 * once
                                                                 */
                if (prRetainedSwRfb) {
                    if (saaFsmSendEventJoinComplete(
                            prAdapter, WLAN_STATUS_SUCCESS, prStaRec,
                            prRetainedSwRfb) == WLAN_STATUS_SUCCESS) {
                        /* ToDo:: Nothing */
                    } else {
                        eNextState = AA_STATE_RESOURCE;
                        fgIsTransition = true;
                    }
                } else {
                    if (saaFsmSendEventJoinComplete(
                            prAdapter, WLAN_STATUS_FAILURE, prStaRec, NULL) ==
                        WLAN_STATUS_RESOURCES) {
                        eNextState = AA_STATE_RESOURCE;
                        fgIsTransition = true;
                    }
                }
            }

            /* Free allocated TCM memory */
            if (prStaRec->prChallengeText) {
                cnmMemFree(prAdapter, prStaRec->prChallengeText);
                prStaRec->prChallengeText = (P_IE_CHALLENGE_TEXT_T)NULL;
            }

            break;

        case SAA_STATE_SEND_AUTH1:

            /* Do tasks in INIT STATE */
            if (prStaRec->ucTxAuthAssocRetryCount >=
                prStaRec->ucTxAuthAssocRetryLimit) {
                /* Record the Status Code of Authentication
                 * Request */
                prStaRec->u2StatusCode = STATUS_CODE_AUTH_TIMEOUT;

                eNextState = AA_STATE_IDLE;
                fgIsTransition = true;
            } else {
                prStaRec->ucTxAuthAssocRetryCount++;

                /* Update Station Record - Class 1 Flag */
                cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

#if !CFG_SUPPORT_AAA
                rStatus = authSendAuthFrame(prAdapter, prStaRec,
                                            AUTH_TRANSACTION_SEQ_1);
#else
                rStatus = authSendAuthFrame(prAdapter, prStaRec,
                                            prStaRec->ucBssIndex, NULL,
                                            AUTH_TRANSACTION_SEQ_1,
                                            STATUS_CODE_RESERVED);
#endif
                if (rStatus != WLAN_STATUS_SUCCESS) {
                    cnmTimerInitTimer(
                        prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                        (PFN_MGMT_TIMEOUT_FUNC)saaFsmRunEventTxReqTimeOut,
                        (unsigned long)prStaRec);

                    cnmTimerStartTimer(
                        prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                        TU_TO_MSEC(TX_AUTHENTICATION_RETRY_TIMEOUT_TU));
                }
            }

            break;

        case SAA_STATE_WAIT_AUTH2:
            break;

        case SAA_STATE_SEND_AUTH3:

            /* Do tasks in INIT STATE */
            if (prStaRec->ucTxAuthAssocRetryCount >=
                prStaRec->ucTxAuthAssocRetryLimit) {
                /* Record the Status Code of Authentication
                 * Request */
                prStaRec->u2StatusCode = STATUS_CODE_AUTH_TIMEOUT;

                eNextState = AA_STATE_IDLE;
                fgIsTransition = true;
            } else {
                prStaRec->ucTxAuthAssocRetryCount++;

#if !CFG_SUPPORT_AAA
                rStatus = authSendAuthFrame(prAdapter, prStaRec,
                                            AUTH_TRANSACTION_SEQ_3);
#else
                rStatus = authSendAuthFrame(prAdapter, prStaRec,
                                            prStaRec->ucBssIndex, NULL,
                                            AUTH_TRANSACTION_SEQ_3,
                                            STATUS_CODE_RESERVED);
#endif
                if (rStatus != WLAN_STATUS_SUCCESS) {
                    cnmTimerInitTimer(
                        prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                        (PFN_MGMT_TIMEOUT_FUNC)saaFsmRunEventTxReqTimeOut,
                        (unsigned long)prStaRec);

                    cnmTimerStartTimer(
                        prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                        TU_TO_MSEC(TX_AUTHENTICATION_RETRY_TIMEOUT_TU));
                }
            }

            break;

        case SAA_STATE_WAIT_AUTH4:
            break;

        case SAA_STATE_SEND_ASSOC1:
            /* Do tasks in INIT STATE */
            if (prStaRec->ucTxAuthAssocRetryCount >=
                prStaRec->ucTxAuthAssocRetryLimit) {
                /* Record the Status Code of Authentication
                 * Request */
                prStaRec->u2StatusCode = STATUS_CODE_ASSOC_TIMEOUT;

                eNextState = AA_STATE_IDLE;
                fgIsTransition = true;
            } else {
                prStaRec->ucTxAuthAssocRetryCount++;

                rStatus = assocSendReAssocReqFrame(prAdapter, prStaRec);
                if (rStatus != WLAN_STATUS_SUCCESS) {
                    cnmTimerInitTimer(
                        prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                        (PFN_MGMT_TIMEOUT_FUNC)saaFsmRunEventTxReqTimeOut,
                        (unsigned long)prStaRec);

                    cnmTimerStartTimer(
                        prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                        TU_TO_MSEC(TX_ASSOCIATION_RETRY_TIMEOUT_TU));
                }
            }

            break;

        case SAA_STATE_WAIT_ASSOC2:
            break;

        case AA_STATE_RESOURCE:
            /* TODO(Kevin) Can setup a timer and send message later
             */
            break;

        default:
            DBGLOG(SAA, ERROR, "Unknown AA STATE\n");
            ASSERT(0);
            break;
        }
    } while (fgIsTransition);

    return;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send Event to AIS/BOW/P2P
 *
 * @param[in] rJoinStatus        To indicate JOIN success or failure.
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 * @param[in] prSwRfb            Pointer to the SW_RFB_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
saaFsmSendEventJoinComplete(IN P_ADAPTER_T prAdapter,
                            IN WLAN_STATUS rJoinStatus,
                            IN P_STA_RECORD_T prStaRec, IN P_SW_RFB_T prSwRfb)
{
    P_BSS_INFO_T prBssInfo;

    ASSERT(prStaRec);
    if (!prStaRec) {
        DBGLOG(SAA, ERROR, "[%s]prStaRec is NULL\n", __func__);
        return WLAN_STATUS_INVALID_PACKET;
    }
    if (!prAdapter) {
        DBGLOG(SAA, ERROR, "[%s]prAdapter is NULL\n", __func__);
        return WLAN_STATUS_INVALID_PACKET;
    }

    /* Store limitation about 40Mhz bandwidth capability during association
     */
    if (prStaRec->ucBssIndex < BSS_INFO_NUM) {
        prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

        if (prBssInfo != NULL) {
            if (rJoinStatus == WLAN_STATUS_SUCCESS) {
                prBssInfo->fg40mBwAllowed = prBssInfo->fgAssoc40mBwAllowed;
                /* Initialize OpMode Channel Width change indicator */
                prBssInfo->fgIsOpChangeChannelWidth = false;

                // /* reset add key action */
                // prBssInfo->eKeyAction = SEC_TX_KEY_COMMAND;
            }
            prBssInfo->fgAssoc40mBwAllowed = false;
        }
    }

    /* For wlan0 (AP) + p2p0, don't check the prAisBssInfo for the P2P. */
#if CFG_ENABLE_WIFI_DIRECT
    if ((prAdapter->fgIsP2PRegistered) && (IS_STA_IN_P2P(prStaRec))) {
        P_MSG_SAA_FSM_COMP_T prSaaFsmCompMsg;

        prSaaFsmCompMsg =
            cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_SAA_FSM_COMP_T));
        if (!prSaaFsmCompMsg) {
            return WLAN_STATUS_RESOURCES;
        }

        if (rJoinStatus == WLAN_STATUS_SUCCESS) {
            prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;
        }

        prSaaFsmCompMsg->rMsgHdr.eMsgId = MID_SAA_P2P_JOIN_COMPLETE;
        prSaaFsmCompMsg->ucSeqNum = prStaRec->ucAuthAssocReqSeqNum;
        prSaaFsmCompMsg->rJoinStatus = rJoinStatus;
        prSaaFsmCompMsg->prStaRec = prStaRec;
        prSaaFsmCompMsg->prSwRfb = prSwRfb;

        /* NOTE(Kevin): Set to UNBUF for immediately JOIN complete */
        mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prSaaFsmCompMsg,
                    MSG_SEND_METHOD_UNBUF);

        return WLAN_STATUS_SUCCESS;
    }
#endif  /* CFG_ENABLE_WIFI_DIRECT */
    if (!prAdapter->prAisBssInfo) {
        DBGLOG(SAA, ERROR, "prAdapter->prAisBssInfo is NULL\n");
        return WLAN_STATUS_INVALID_PACKET;
    }
    if (prStaRec->ucBssIndex == prAdapter->prAisBssInfo->ucBssIndex) {
        P_MSG_SAA_FSM_COMP_T prSaaFsmCompMsg;

        prSaaFsmCompMsg =
            cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_SAA_FSM_COMP_T));
        if (!prSaaFsmCompMsg) {
            return WLAN_STATUS_RESOURCES;
        }

        if (rJoinStatus == WLAN_STATUS_SUCCESS) {
            prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;
        }

        prSaaFsmCompMsg->rMsgHdr.eMsgId = MID_SAA_AIS_JOIN_COMPLETE;
        prSaaFsmCompMsg->ucSeqNum = prStaRec->ucAuthAssocReqSeqNum;
        prSaaFsmCompMsg->rJoinStatus = rJoinStatus;
        prSaaFsmCompMsg->prStaRec = prStaRec;
        prSaaFsmCompMsg->prSwRfb = prSwRfb;

        /* NOTE(Kevin): Set to UNBUF for immediately JOIN complete */
        mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prSaaFsmCompMsg,
                    MSG_SEND_METHOD_UNBUF);

        return WLAN_STATUS_SUCCESS;
    } else {
        DBGLOG(SAA, ERROR, "Invalid case in %s.\n", __func__);
        return WLAN_STATUS_FAILURE;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Start Event to SAA FSM.
 *
 * @param[in] prMsgHdr   Message of Join Request for a particular STA.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void saaFsmRunEventStart(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
    P_MSG_SAA_FSM_START_T prSaaFsmStartMsg;
    P_STA_RECORD_T prStaRec;
    P_BSS_INFO_T prBssInfo;

    ASSERT(prAdapter);
    ASSERT(prMsgHdr);

    prSaaFsmStartMsg = (P_MSG_SAA_FSM_START_T)prMsgHdr;
    prStaRec = prSaaFsmStartMsg->prStaRec;

    if ((!prStaRec) || (prStaRec->fgIsInUse == false)) {
        cnmMemFree(prAdapter, prMsgHdr);
        return;
    }

    ASSERT(prStaRec);

    DBGLOG(SAA, LOUD, "EVENT-START: Trigger SAA FSM.\n");

    /* record sequence number of request message */
    prStaRec->ucAuthAssocReqSeqNum = prSaaFsmStartMsg->ucSeqNum;

    cnmMemFree(prAdapter, prMsgHdr);

    /* 4 <1> Validation of SAA Start Event */
    if (!IS_AP_STA(prStaRec)) {
        DBGLOG(SAA, ERROR, "EVENT-START: STA Type - %d was not supported.\n",
               prStaRec->eStaType);

        /* Ignore the return value because don't care the prSwRfb */
        saaFsmSendEventJoinComplete(prAdapter, WLAN_STATUS_FAILURE, prStaRec,
                                    NULL);

        return;
    }
    /* 4 <2> The previous JOIN process is not completed ? */
    if (prStaRec->eAuthAssocState != AA_STATE_IDLE) {
        DBGLOG(SAA, ERROR, "EVENT-START: Reentry of SAA Module.\n");
        prStaRec->eAuthAssocState = AA_STATE_IDLE;
    }
    /* 4 <3> Reset Status Code and Time */
    /* Update Station Record - Status/Reason Code */
    prStaRec->u2StatusCode = STATUS_CODE_UNSPECIFIED_FAILURE;

    /* Update the record join time. */
    GET_CURRENT_SYSTIME(&prStaRec->rLastJoinTime);

    prStaRec->ucTxAuthAssocRetryCount = 0;

    if (prStaRec->prChallengeText) {
        cnmMemFree(prAdapter, prStaRec->prChallengeText);
        prStaRec->prChallengeText = (P_IE_CHALLENGE_TEXT_T)NULL;
    }

    cnmTimerStopTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer);

    /* 4 <4> Init the sec fsm */
    /* secFsmInit(prAdapter, prStaRec); */

    /* 4 <5> Reset the STA STATE */
    /* Update Station Record - Class 1 Flag */
    /* NOTE(Kevin): Moved to AIS FSM for Reconnect issue -
     * We won't deactivate the same STA_RECORD_T and then activate it again
     * for the case of reconnection.
     */
    /* cnmStaRecChangeState(prStaRec, STA_STATE_1); */

    /* 4 <6> Decide if this BSS 20/40M bandwidth is allowed */
    if (prStaRec->ucBssIndex < BSS_INFO_NUM) {
        prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

        if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet &
             PHY_TYPE_SET_802_11N) &&
            (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)) {
            prBssInfo->fgAssoc40mBwAllowed =
                cnmBss40mBwPermitted(prAdapter, prBssInfo->ucBssIndex);
        } else {
            prBssInfo->fgAssoc40mBwAllowed = false;
        }
        DBGLOG(RLM, TRACE, "STA 40mAllowed=%d\n",
               prBssInfo->fgAssoc40mBwAllowed);
    }

    if (!IS_STA_IN_P2P(prStaRec)) {
        /* skip SAA FSM */
        prStaRec->eAuthAssocSent = AA_SENT_NONE;
        saaSendAuthAssoc(prAdapter, prStaRec);
    } else {
        /* 4 <7> Trigger SAA FSM */
        if (prStaRec->ucStaState == STA_STATE_1) {
            saaFsmSteps(prAdapter, prStaRec, SAA_STATE_SEND_AUTH1,
                        (P_SW_RFB_T)NULL);
        } else if (prStaRec->ucStaState == STA_STATE_2 ||
                   prStaRec->ucStaState == STA_STATE_3) {
            saaFsmSteps(prAdapter, prStaRec, SAA_STATE_SEND_ASSOC1,
                        (P_SW_RFB_T)NULL);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle TxDone(Auth1/Auth3/AssocReq) Event of SAA
 * FSM.
 *
 * @param[in] prMsduInfo     Pointer to the MSDU_INFO_T.
 * @param[in] rTxDoneStatus  Return TX status of the Auth1/Auth3/AssocReq frame.
 *
 * @retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
saaFsmRunEventTxDone(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo,
                     IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
    P_STA_RECORD_T prStaRec;
    ENUM_AA_STATE_T eNextState;

    ASSERT(prMsduInfo);

    prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

    if (!prStaRec) {
        return WLAN_STATUS_INVALID_PACKET;
    }

    ASSERT(prStaRec);

    DBGLOG(SAA, LOUD, "EVENT-TX DONE: Current Time = %d\n", kalGetTimeTick());

    /* Trigger statistics log if Auth/Assoc Tx failed */
    if (rTxDoneStatus != TX_RESULT_SUCCESS) {
        wlanTriggerStatsLog(prAdapter, prAdapter->rWifiVar.u4StatsLogDuration);
    }

    if (!IS_STA_IN_P2P(prStaRec)) {
        /* check the outgoing frame is matched with
         * the last sent frame, ignore the unmatched txdone
         */
        if ((prStaRec->eAuthAssocSent >= AA_SENT_AUTH1) &&
            (prStaRec->eAuthAssocSent <= AA_SENT_AUTH4)) {
            if (authCheckTxAuthFrame(prAdapter, prMsduInfo,
                                     prStaRec->eAuthAssocSent) !=
                WLAN_STATUS_SUCCESS) {
                return WLAN_STATUS_SUCCESS;
            }
        } else if (prStaRec->eAuthAssocSent == AA_SENT_ASSOC1) {
            if (assocCheckTxReAssocReqFrame(prAdapter, prMsduInfo) !=
                WLAN_STATUS_SUCCESS) {
                return WLAN_STATUS_SUCCESS;
            }
        } else {
            DBGLOG(SAA, WARN, "unexpected sent frame = %d\n",
                   prStaRec->eAuthAssocSent);
        }

        cnmTimerStopTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer);

        if (rTxDoneStatus == TX_RESULT_SUCCESS) {
            cnmTimerInitTimer(
                prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                (PFN_MGMT_TIMEOUT_FUNC)saaFsmRunEventRxRespTimeOut,
                (unsigned long)prStaRec);

            if (prAdapter->prGlueInfo->rWpaInfo.u4AuthAlg & AUTH_TYPE_SAE) {
                cnmTimerStartTimer(
                    prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                    TU_TO_MSEC(DOT11_RSNA_SAE_RETRANS_PERIOD_TU));
            } else {
                cnmTimerStartTimer(
                    prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                    TU_TO_MSEC(DOT11_AUTHENTICATION_RESPONSE_TIMEOUT_TU));
            }
        } else {  /* Tx failed, do retry if possible */
            /* Add for support wep when enable wpa3 */
            if (prStaRec->eAuthAssocSent == AA_SENT_AUTH3) {
                saaSendAuthSeq3(prAdapter, prStaRec);
            } else {
                saaSendAuthAssoc(prAdapter, prStaRec);
            }
        }
    } else {
        eNextState = prStaRec->eAuthAssocState;

        switch (prStaRec->eAuthAssocState) {
        case SAA_STATE_SEND_AUTH1: {
            /* Strictly check the outgoing frame is matched with
             * current AA STATE */
            if (authCheckTxAuthFrame(prAdapter, prMsduInfo,
                                     AUTH_TRANSACTION_SEQ_1) !=
                WLAN_STATUS_SUCCESS) {
                break;
            }

            if (rTxDoneStatus == TX_RESULT_SUCCESS) {
                eNextState = SAA_STATE_WAIT_AUTH2;

                cnmTimerStopTimer(prAdapter,
                                  &prStaRec->rTxReqDoneOrRxRespTimer);

                cnmTimerInitTimer(
                    prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                    (PFN_MGMT_TIMEOUT_FUNC)saaFsmRunEventRxRespTimeOut,
                    (unsigned long)prStaRec);

                if (prAdapter->prGlueInfo->rWpaInfo.u4AuthAlg & AUTH_TYPE_SAE) {
                    cnmTimerStartTimer(
                        prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                        TU_TO_MSEC(DOT11_RSNA_SAE_RETRANS_PERIOD_TU));
                } else {
                    cnmTimerStartTimer(
                        prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                        TU_TO_MSEC(DOT11_AUTHENTICATION_RESPONSE_TIMEOUT_TU));
                }
            }

            /* if TX was successful, change to next state.
             * if TX was failed, do retry if possible.
             */
            saaFsmSteps(prAdapter, prStaRec, eNextState, (P_SW_RFB_T)NULL);
        } break;

        case SAA_STATE_SEND_AUTH3: {
            /* Strictly check the outgoing frame is matched with
             * current JOIN STATE */
            if (authCheckTxAuthFrame(prAdapter, prMsduInfo,
                                     AUTH_TRANSACTION_SEQ_3) !=
                WLAN_STATUS_SUCCESS) {
                break;
            }

            if (rTxDoneStatus == TX_RESULT_SUCCESS) {
                eNextState = SAA_STATE_WAIT_AUTH4;

                cnmTimerStopTimer(prAdapter,
                                  &prStaRec->rTxReqDoneOrRxRespTimer);

                cnmTimerInitTimer(
                    prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                    (PFN_MGMT_TIMEOUT_FUNC)saaFsmRunEventRxRespTimeOut,
                    (unsigned long)prStaRec);

                cnmTimerStartTimer(
                    prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                    TU_TO_MSEC(DOT11_AUTHENTICATION_RESPONSE_TIMEOUT_TU));
            }

            /* if TX was successful, change to next state.
             * if TX was failed, do retry if possible.
             */
            saaFsmSteps(prAdapter, prStaRec, eNextState, (P_SW_RFB_T)NULL);
        } break;

        case SAA_STATE_SEND_ASSOC1: {
            /* Strictly check the outgoing frame is matched with
             * current SAA STATE */
            if (assocCheckTxReAssocReqFrame(prAdapter, prMsduInfo) !=
                WLAN_STATUS_SUCCESS) {
                break;
            }

            if (rTxDoneStatus == TX_RESULT_SUCCESS) {
                eNextState = SAA_STATE_WAIT_ASSOC2;

                cnmTimerStopTimer(prAdapter,
                                  &prStaRec->rTxReqDoneOrRxRespTimer);

                cnmTimerInitTimer(
                    prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                    (PFN_MGMT_TIMEOUT_FUNC)saaFsmRunEventRxRespTimeOut,
                    (unsigned long)prStaRec);

                cnmTimerStartTimer(
                    prAdapter, &(prStaRec->rTxReqDoneOrRxRespTimer),
                    TU_TO_MSEC(DOT11_ASSOCIATION_RESPONSE_TIMEOUT_TU));
            }

            /* if TX was successful, change to next state.
             * if TX was failed, do retry if possible.
             */
            saaFsmSteps(prAdapter, prStaRec, eNextState, (P_SW_RFB_T)NULL);
        } break;

        default:
            break;  /* Ignore other cases */
        }
    }
    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send Tx Request Timeout Event to SAA FSM.
 *
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void saaFsmRunEventTxReqTimeOut(IN P_ADAPTER_T prAdapter,
                                IN unsigned long plParamPtr)
{
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)plParamPtr;

    ASSERT(prStaRec);
    if (!prStaRec) {
        return;
    }

    DBGLOG(SAA, LOUD, "EVENT-TIMER: TX REQ TIMEOUT, Current Time = %d\n",
           kalGetTimeTick());

    /* Trigger statistics log if Auth/Assoc Tx timeout */
    wlanTriggerStatsLog(prAdapter, prAdapter->rWifiVar.u4StatsLogDuration);
    if (!IS_STA_IN_P2P(prStaRec)) {
        saaSendAuthAssoc(prAdapter, prStaRec);
    } else {
        switch (prStaRec->eAuthAssocState) {
        case SAA_STATE_SEND_AUTH1:
        case SAA_STATE_SEND_AUTH3:
        case SAA_STATE_SEND_ASSOC1:
            saaFsmSteps(prAdapter, prStaRec, prStaRec->eAuthAssocState,
                        (P_SW_RFB_T)NULL);
            break;

        default:
            return;
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send Rx Response Timeout Event to SAA FSM.
 *
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void saaFsmRunEventRxRespTimeOut(IN P_ADAPTER_T prAdapter,
                                 IN unsigned long ulParamPtr)
{
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)ulParamPtr;
    ENUM_AA_STATE_T eNextState;
    DBGLOG(SAA, LOUD, "EVENT-TIMER: RX RESP TIMEOUT, Current Time = %d\n",
           kalGetTimeTick());

    ASSERT(prStaRec);
    if (!prStaRec) {
        return;
    }

    if (!IS_STA_IN_P2P(prStaRec)) {
        /* Retry the last sent frame if possible */
        if (prStaRec->ucStaState != STA_STATE_3) {
            saaSendAuthAssoc(prAdapter, prStaRec);
        }
    } else {
        eNextState = prStaRec->eAuthAssocState;

        switch (prStaRec->eAuthAssocState) {
        case SAA_STATE_WAIT_AUTH2:
            /* Record the Status Code of Authentication Request */
            prStaRec->u2StatusCode = STATUS_CODE_AUTH_TIMEOUT;

            /* Pull back to earlier state to do retry */
            eNextState = SAA_STATE_SEND_AUTH1;
            break;

        case SAA_STATE_WAIT_AUTH4:
            /* Record the Status Code of Authentication Request */
            prStaRec->u2StatusCode = STATUS_CODE_AUTH_TIMEOUT;

            /* Pull back to earlier state to do retry */
            eNextState = SAA_STATE_SEND_AUTH3;
            break;

        case SAA_STATE_WAIT_ASSOC2:
            /* Record the Status Code of Authentication Request */
            prStaRec->u2StatusCode = STATUS_CODE_ASSOC_TIMEOUT;

            /* Pull back to earlier state to do retry */
            eNextState = SAA_STATE_SEND_ASSOC1;
            break;

        default:
            break;  /* Ignore other cases */
        }

        if (eNextState != prStaRec->eAuthAssocState) {
            saaFsmSteps(prAdapter, prStaRec, eNextState, (P_SW_RFB_T)NULL);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will process the Rx Auth Response Frame and then
 *        trigger SAA FSM.
 *
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void saaFsmRunEventRxAuth(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
    P_STA_RECORD_T prStaRec;
    u16 u2StatusCode;
    ENUM_AA_STATE_T eNextState;
    u8 ucWlanIdx;
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_WLAN_AUTH_FRAME_T prAuthFrame = (P_WLAN_AUTH_FRAME_T)NULL;
    u8 *pFrameBuf = NULL;
    u8 fgIsInterruptContext = false;

    ASSERT(prSwRfb);

    prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
    ucWlanIdx = (u8)HAL_RX_STATUS_GET_WLAN_IDX(prSwRfb->prRxStatus);
    prGlueInfo = prAdapter->prGlueInfo;

    ASSERT(prGlueInfo);

    /* We should have the corresponding Sta Record. */
    if (!prStaRec) {
        DBGLOG(SAA, WARN,
               "Received a AuthResp: wlanIdx[%d] w/o corresponding staRec\n",
               ucWlanIdx);
        return;
    }

    if (!IS_AP_STA(prStaRec)) {
        return;
    }

    /* check received auth frame */
    if ((authCheckRxAuthFrameStatus(prAdapter, prSwRfb,
                                    prStaRec->eAuthAssocSent,
                                    &u2StatusCode) == WLAN_STATUS_SUCCESS) &&
        (!(IS_STA_IN_P2P(prStaRec)))) {
        cnmTimerStopTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer);

        /* Record the Status Code of Authentication Request */
        prStaRec->u2StatusCode = u2StatusCode;

        /*Report Rx auth frame to upper layer*/
        prAuthFrame = (P_WLAN_AUTH_FRAME_T)prSwRfb->pvHeader;

        DBGLOG(INIT, INFO, "Dump rx auth data\n");
        DBGLOG_MEM8(REQ, INFO, prAuthFrame, prSwRfb->u2PacketLen);

        /*add for WEP */
        if ((prAuthFrame->u2AuthAlgNum == AUTH_ALGORITHM_NUM_SHARED_KEY) &&
            (prAuthFrame->aucAuthData[0] == AUTH_TRANSACTION_SEQ_2)) {
            /* Reset Send Auth/(Re)Assoc Frame Count */
            prStaRec->ucTxAuthAssocRetryCount = 0;
            if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
                authProcessRxAuth2_Auth4Frame(prAdapter, prSwRfb);
                saaSendAuthSeq3(prAdapter, prStaRec);
                return;
            }
        } else {
            DBGLOG(
                SAA, INFO,
                "Report RX auth to upper layer with alg:%d, SN:%d, status:%d\n",
                prAuthFrame->u2AuthAlgNum, prAuthFrame->aucAuthData[0],
                prAuthFrame->aucAuthData[2]);

            if (in_interrupt()) {
                pFrameBuf = kalMemAlloc(prSwRfb->u2PacketLen, PHY_MEM_TYPE);
                fgIsInterruptContext = true;
            } else {
                pFrameBuf = kalMemAlloc(prSwRfb->u2PacketLen, VIR_MEM_TYPE);
                fgIsInterruptContext = false;
            }

            if (!pFrameBuf) {
                DBGLOG(SAA, ERROR, "Alloc buffer for frame failed\n");
                return;
            }

            kalMemCopy((void *)pFrameBuf, (void *)prAuthFrame,
                       prSwRfb->u2PacketLen);
            kalWDevLockThread(prGlueInfo, prGlueInfo->prDevHandler,
                              CFG80211_RX_MLME_MGMT, pFrameBuf,
                              prSwRfb->u2PacketLen, NULL, 0, NULL, 0,
                              fgIsInterruptContext);

            DBGLOG(SAA, INFO, "notification of RX Authentication Done\n");
        }

        /* Reset Send Auth/(Re)Assoc Frame Count */
        prStaRec->ucTxAuthAssocRetryCount = 0;
        if (u2StatusCode == STATUS_CODE_SUCCESSFUL ||
            (u2StatusCode == WLAN_STATUS_SAE_HASH_TO_ELEMENT)) {
            authProcessRxAuth2_Auth4Frame(prAdapter, prSwRfb);
        } else {
            DBGLOG(SAA, INFO,
                   "Auth Req was rejected by [" MACSTR "], Status Code = %d\n",
                   MAC2STR(prStaRec->aucMacAddr), u2StatusCode);

            /* AIS retry JOIN or indicate JOIN FAILURE to upper
             * layer*/
            if (saaFsmSendEventJoinComplete(prAdapter, WLAN_STATUS_FAILURE,
                                            prStaRec,
                                            NULL) == WLAN_STATUS_RESOURCES) {
                /* can set a timer and retry later */
                DBGLOG(SAA, WARN,
                       "[SAA]can't alloc msg for inform AIS join complete\n");
            }
        }
    } else {
        switch (prStaRec->eAuthAssocState) {
        case SAA_STATE_SEND_AUTH1:
        case SAA_STATE_WAIT_AUTH2:
            /* Check if the incoming frame is what we are waiting
             * for */
            if (authCheckRxAuthFrameStatus(
                    prAdapter, prSwRfb, AUTH_TRANSACTION_SEQ_2,
                    &u2StatusCode) == WLAN_STATUS_SUCCESS) {
                cnmTimerStopTimer(prAdapter,
                                  &prStaRec->rTxReqDoneOrRxRespTimer);

                /* Record the Status Code of Authentication
                 * Request */
                prStaRec->u2StatusCode = u2StatusCode;

                if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
                    authProcessRxAuth2_Auth4Frame(prAdapter, prSwRfb);

                    if (prStaRec->ucAuthAlgNum ==
                        (u8)AUTH_ALGORITHM_NUM_SHARED_KEY) {
                        eNextState = SAA_STATE_SEND_AUTH3;
                    } else {
                        /* Update Station Record - Class
                         * 2 Flag */
                        cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);

                        eNextState = SAA_STATE_SEND_ASSOC1;
                    }
                } else {
                    DBGLOG(SAA, INFO,
                           "Auth Req was rejected by [" MACSTR
                           "], Status Code = %d\n",
                           MAC2STR(prStaRec->aucMacAddr), u2StatusCode);

                    eNextState = AA_STATE_IDLE;
                }

                /* Reset Send Auth/(Re)Assoc Frame Count */
                prStaRec->ucTxAuthAssocRetryCount = 0;

                saaFsmSteps(prAdapter, prStaRec, eNextState, (P_SW_RFB_T)NULL);
            }
            break;

        case SAA_STATE_SEND_AUTH3:
        case SAA_STATE_WAIT_AUTH4:
            /* Check if the incoming frame is what we are waiting
             * for */
            if (authCheckRxAuthFrameStatus(
                    prAdapter, prSwRfb, AUTH_TRANSACTION_SEQ_4,
                    &u2StatusCode) == WLAN_STATUS_SUCCESS) {
                cnmTimerStopTimer(prAdapter,
                                  &prStaRec->rTxReqDoneOrRxRespTimer);

                /* Record the Status Code of Authentication
                 * Request */
                prStaRec->u2StatusCode = u2StatusCode;

                if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
                    authProcessRxAuth2_Auth4Frame(prAdapter,
                                                  prSwRfb);  /* Add for
                                                              * 802.11r
                                                              * handling
                                                              */

                    /* Update Station Record - Class 2 Flag
                     */
                    cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);

                    eNextState = SAA_STATE_SEND_ASSOC1;
                } else {
                    DBGLOG(SAA, INFO,
                           "Auth Req was rejected by [" MACSTR
                           "], Status Code = %d\n",
                           MAC2STR(prStaRec->aucMacAddr), u2StatusCode);

                    eNextState = AA_STATE_IDLE;
                }

                /* Reset Send Auth/(Re)Assoc Frame Count */
                prStaRec->ucTxAuthAssocRetryCount = 0;

                saaFsmSteps(prAdapter, prStaRec, eNextState, (P_SW_RFB_T)NULL);
            }
            break;

        default:
            break;  /* Ignore other cases */
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will process the Rx (Re)Association Response Frame and
 * then trigger SAA FSM.
 *
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS           if the status code was not success
 * @retval WLAN_STATUS_BUFFER_RETAINED   if the status code was success
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS saaFsmRunEventRxAssoc(IN P_ADAPTER_T prAdapter,
                                  IN P_SW_RFB_T prSwRfb)
{
    P_STA_RECORD_T prStaRec;
    u16 u2StatusCode;
    ENUM_AA_STATE_T eNextState;
    P_SW_RFB_T prRetainedSwRfb = (P_SW_RFB_T)NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u8 ucWlanIdx;
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_WLAN_ASSOC_RSP_FRAME_T prAssocRspFrame = NULL;
    P_CONNECTION_SETTINGS_T prConnSettings = NULL;

    ASSERT(prSwRfb);
    prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
    ucWlanIdx = (u8)HAL_RX_STATUS_GET_WLAN_IDX(prSwRfb->prRxStatus);
    prGlueInfo = prAdapter->prGlueInfo;

    ASSERT(prGlueInfo);

    prConnSettings = &prGlueInfo->prAdapter->rWifiVar.rConnSettings;

    DBGLOG(SAA, INFO, "RX Assoc Resp\n");

    /* We should have the corresponding Sta Record. */
    if (!prStaRec) {
        /* ASSERT(0); */
        DBGLOG(SAA, WARN,
               "Received a AssocResp: wlanIdx[%d] w/o corresponding staRec\n",
               ucWlanIdx);
        return rStatus;
    }

    if (!IS_AP_STA(prStaRec)) {
        return rStatus;
    }

    /* true if the incoming frame is what we are waiting for */
    if ((assocCheckRxReAssocRspFrameStatus(prAdapter, prSwRfb, &u2StatusCode) ==
         WLAN_STATUS_SUCCESS) &&
        (!(IS_STA_IN_P2P(prStaRec)))) {
        cnmTimerStopTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer);

        /* Record the Status Code of Authentication Request */
        prStaRec->u2StatusCode = u2StatusCode;

        /*Report Rx assoc frame to upper layer*/
        prAssocRspFrame = (P_WLAN_ASSOC_RSP_FRAME_T)prSwRfb->pvHeader;

        /* The BSS from cfg80211_ops.assoc must give back to
         * cfg80211_send_rx_assoc() or to cfg80211_assoc_timeout().
         * To ensure proper refcounting, new association requests
         * while already associating must be rejected.
         */
        DBGLOG(SAA, INFO, "Report RX Assoc to upper layer, %s\n",
               prConnSettings->bss ? "DO IT" : "Oops");

        kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
                                     WLAN_STATUS_ASSOC_RESP, prAssocRspFrame,
                                     prSwRfb->u2PacketLen);

        /* Reset Send Auth/(Re)Assoc Frame Count */
        prStaRec->ucTxAuthAssocRetryCount = 0;

        /* update RCPI */
        ASSERT(prSwRfb->prRxStatusGroup3);
        prStaRec->ucRCPI = nicRxGetRcpiValueFromRxv(RCPI_MODE_WF0, prSwRfb);

        if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
            /* Update Station Record - Class 3 Flag */
            /* NOTE(Kevin): Moved to AIS FSM for roaming issue
             * We should deactivate the struct STA_RECORD of
             * previous AP before activate new one in Driver.
             */
            /* cnmStaRecChangeState(prStaRec, STA_STATE_3); */
            /* Clear history. */
            prStaRec->ucJoinFailureCount = 0;

            if (saaFsmSendEventJoinComplete(prAdapter, WLAN_STATUS_SUCCESS,
                                            prStaRec,
                                            prSwRfb) == WLAN_STATUS_RESOURCES) {
                /* can set a timer and retry later */
                DBGLOG(SAA, WARN,
                       "[SAA]can't alloc msg for inform AIS join complete\n");
            }

            rStatus = WLAN_STATUS_PENDING;
        } else {
            DBGLOG(SAA, INFO,
                   "Assoc Req was rejected by [" MACSTR "], Status Code = %d\n",
                   MAC2STR(prStaRec->aucMacAddr), u2StatusCode);

            if (saaFsmSendEventJoinComplete(prAdapter, WLAN_STATUS_FAILURE,
                                            prStaRec,
                                            NULL) == WLAN_STATUS_RESOURCES) {
                /* can set a timer and retry later */
                DBGLOG(SAA, WARN,
                       "[SAA]can't alloc msg for inform AIS join complete\n");
            }
        }
    } else {
        switch (prStaRec->eAuthAssocState) {
        case SAA_STATE_SEND_ASSOC1:
        case SAA_STATE_WAIT_ASSOC2:
            /* true if the incoming frame is what we are waiting for
             */
            if (assocCheckRxReAssocRspFrameStatus(
                    prAdapter, prSwRfb, &u2StatusCode) == WLAN_STATUS_SUCCESS) {
                cnmTimerStopTimer(prAdapter,
                                  &prStaRec->rTxReqDoneOrRxRespTimer);

                /* Record the Status Code of Authentication
                 * Request */
                prStaRec->u2StatusCode = u2StatusCode;

                if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
                    /* Update Station Record - Class 3 Flag
                     */
                    /* NOTE(Kevin): Moved to AIS FSM for
                     * roaming issue - We should deactivate
                     * the STA_RECORD_T of previous AP
                     * before activate new one in Driver.
                     */
                    /* cnmStaRecChangeState(prStaRec,
                     * STA_STATE_3); */

                    prStaRec->ucJoinFailureCount = 0;  /* Clear
                                                        * history.
                                                        */

                    prRetainedSwRfb = prSwRfb;
                    rStatus = WLAN_STATUS_PENDING;
                } else {
                    DBGLOG(SAA, INFO,
                           "Assoc Req was rejected by [" MACSTR
                           "], Status Code = %d\n",
                           MAC2STR(prStaRec->aucMacAddr), u2StatusCode);
                }

                /* Reset Send Auth/(Re)Assoc Frame Count */
                prStaRec->ucTxAuthAssocRetryCount = 0;

                /* update RCPI */
                ASSERT(prSwRfb->prRxStatusGroup3);
                prStaRec->ucRCPI =
                    nicRxGetRcpiValueFromRxv(RCPI_MODE_WF0, prSwRfb);

                eNextState = AA_STATE_IDLE;

                saaFsmSteps(prAdapter, prStaRec, eNextState, prRetainedSwRfb);
            }
            break;

        default:
            break;  /* Ignore other cases */
        }
    }
    return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check the incoming Deauth Frame.
 *
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Always not retain deauthentication frames
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS saaFsmRunEventRxDeauth(IN P_ADAPTER_T prAdapter,
                                   IN P_SW_RFB_T prSwRfb)
{
    P_STA_RECORD_T prStaRec;
    P_WLAN_DEAUTH_FRAME_T prDeauthFrame;
    u8 ucWlanIdx;
    u8 ucRoleIdx = 0;
    u8 *pFrameBuf = NULL;
    P_BSS_INFO_T prBssInfo = NULL;
    u8 fgIsInterruptContext = false;

    ASSERT(prSwRfb);
    prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
    prDeauthFrame = (P_WLAN_DEAUTH_FRAME_T)prSwRfb->pvHeader;
    ucWlanIdx = (u8)HAL_RX_STATUS_GET_WLAN_IDX(prSwRfb->prRxStatus);

    DBGLOG(SAA, EVENT,
           "Rx Deauth frame ,DA[" MACSTR "] SA[" MACSTR "] BSSID[" MACSTR
           "] ReasonCode[0x%x]\n",
           MAC2STR(prDeauthFrame->aucDestAddr),
           MAC2STR(prDeauthFrame->aucSrcAddr), MAC2STR(prDeauthFrame->aucBSSID),
           prDeauthFrame->u2ReasonCode);

    do {
        /* We should have the corresponding Sta Record. */
        if (!prStaRec) {
            DBGLOG(SAA, WARN,
                   "Received a Deauth: wlanIdx[%d] w/o corresponding staRec\n",
                   ucWlanIdx);
            break;
        }

        if (IS_STA_IN_AIS(prStaRec)) {
            P_BSS_INFO_T prAisBssInfo;

            if (!IS_AP_STA(prStaRec)) {
                break;
            }

            prAisBssInfo = prAdapter->prAisBssInfo;

            if (prStaRec->ucStaState >= STA_STATE_1) {
                cnmTimerStopTimer(prAdapter,
                                  &prStaRec->rTxReqDoneOrRxRespTimer);

                /* Check if this is the AP we are associated or
                 * associating with */
                if (authProcessRxDeauthFrame(prSwRfb, prStaRec->aucMacAddr,
                                             &prStaRec->u2ReasonCode) ==
                    WLAN_STATUS_SUCCESS) {
                    P_AIS_SPECIFIC_BSS_INFO_T
                        prAisSpecBssInfo;

                    prAisSpecBssInfo =
                        &(prAdapter->rWifiVar.rAisSpecificBssInfo);

                    DBGLOG(
                        RSN, INFO,
                        "QM RX MGT: Deauth frame, P=%d Sec=%d CM=%d BC=%d fc=%02x\n",
                        prAisSpecBssInfo->fgMgmtProtection,
                        HAL_RX_STATUS_GET_SEC_MODE(prSwRfb->prRxStatus),
                        HAL_RX_STATUS_IS_CIPHER_MISMATCH(prSwRfb->prRxStatus),
                        IS_BMCAST_MAC_ADDR(prDeauthFrame->aucDestAddr),
                        prDeauthFrame->u2FrameCtrl);
                    if (prAisSpecBssInfo->fgMgmtProtection &&
                        prStaRec->fgIsTxAllowed &&
                        HAL_RX_STATUS_IS_CIPHER_MISMATCH(prSwRfb->prRxStatus)
                        /* HAL_RX_STATUS_GET_SEC_MODE(prSwRfb->prRxStatus)
                         * != CIPHER_SUITE_BIP */
                        ) {
                        saaChkDeauthfrmParamHandler(prAdapter, prSwRfb,
                                                    prStaRec);
                        if (prStaRec->fgIsTxAllowed) {
                            DBGLOG(RSN, INFO, "ignore no sec deauth\n");
                        }
                        return WLAN_STATUS_SUCCESS;
                    }

                    DBGLOG(SAA, INFO,
                           "notification of RX deauthentication %d\n",
                           prSwRfb->u2PacketLen);

                    if (in_interrupt()) {
                        pFrameBuf =
                            kalMemAlloc(prSwRfb->u2PacketLen, PHY_MEM_TYPE);
                        fgIsInterruptContext = true;
                    } else {
                        pFrameBuf =
                            kalMemAlloc(prSwRfb->u2PacketLen, VIR_MEM_TYPE);
                        fgIsInterruptContext = false;
                    }

                    if (!pFrameBuf) {
                        DBGLOG(SAA, ERROR, "Alloc buffer for frame failed\n");
                        return WLAN_STATUS_RESOURCES;
                    }

                    kalMemCopy((void *)pFrameBuf, (void *)prDeauthFrame,
                               prSwRfb->u2PacketLen);
                    kalWDevLockThread(prAdapter->prGlueInfo,
                                      prAdapter->prGlueInfo->prDevHandler,
                                      CFG80211_RX_MLME_MGMT, pFrameBuf,
                                      prSwRfb->u2PacketLen, NULL, 0, NULL, 0,
                                      fgIsInterruptContext);

                    DBGLOG(SAA, INFO,
                           "notification of RX deauthentication Done\n");
                    saaSendDisconnectMsgHandler(prAdapter, prStaRec,
                                                prAisBssInfo, FRM_DEAUTH);
                }
            }
        } else if (prAdapter->fgIsP2PRegistered && IS_STA_IN_P2P(prStaRec)) {
            DBGLOG(SAA, INFO, "notification of RX deauthentication %d\n",
                   prSwRfb->u2PacketLen);
            prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
            ucRoleIdx = (u8)prBssInfo->u4PrivateData;

            if (in_interrupt()) {
                pFrameBuf = kalMemAlloc(prSwRfb->u2PacketLen, PHY_MEM_TYPE);
                fgIsInterruptContext = true;
            } else {
                pFrameBuf = kalMemAlloc(prSwRfb->u2PacketLen, VIR_MEM_TYPE);
                fgIsInterruptContext = false;
            }

            if (!pFrameBuf) {
                DBGLOG(SAA, ERROR, "Alloc buffer for frame failed\n");
                return WLAN_STATUS_RESOURCES;
            }

            kalMemCopy((void *)pFrameBuf, (void *)prDeauthFrame,
                       prSwRfb->u2PacketLen);
            kalWDevLockThread(
                prAdapter->prGlueInfo,
                prAdapter->prGlueInfo->prP2PInfo[ucRoleIdx]->aprRoleHandler,
                CFG80211_RX_MLME_MGMT, pFrameBuf, prSwRfb->u2PacketLen, NULL, 0,
                NULL, 0, fgIsInterruptContext);

            DBGLOG(SAA, INFO, "notification of RX deauthentication Done\n");

            p2pRoleFsmRunEventRxDeauthentication(prAdapter, prStaRec, prSwRfb);
        } else {
            ASSERT(0);
        }
    } while (false);

    return WLAN_STATUS_SUCCESS;
}

/* for AOSP */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check param of deauth frame and reson code for
 * deauth
 *
 * @param[in]
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/

void saaChkDeauthfrmParamHandler(IN P_ADAPTER_T prAdapter,
                                 IN P_SW_RFB_T prSwRfb,
                                 IN P_STA_RECORD_T prStaRec)
{
    P_WLAN_DEAUTH_FRAME_T prDeauthFrame;

    do {
        prDeauthFrame = (P_WLAN_DEAUTH_FRAME_T)prSwRfb->pvHeader;
        if (!IS_BMCAST_MAC_ADDR(prDeauthFrame->aucDestAddr) &&
            (prStaRec->u2ReasonCode == REASON_CODE_CLASS_2_ERR ||
             prStaRec->u2ReasonCode == REASON_CODE_CLASS_3_ERR)) {
            DBGLOG(RSN, INFO, "QM RX MGT: rsnStartSaQuery\n");
            /* MFP test plan 5.3.3.5 */
            rsnStartSaQuery(prAdapter);
        } else {
            DBGLOG(RSN, INFO, "RXM: Drop unprotected Mgmt frame\n");
            DBGLOG(
                RSN, INFO,
                "RXM: (MAC RX Done) RX (u2StatusFlag=0x%x) (ucKIdxSecMode=0x%x) "
                "(ucWlanIdx=0x%x)\n",
                prSwRfb->prRxStatus->u2StatusFlag,
                prSwRfb->prRxStatus->ucTidSecMode,
                prSwRfb->prRxStatus->ucWlanIdx);
        }
    } while (0);
}

/* for AOSP */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check and send disconnect message to AIS module
 *
 * @param[in]
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
void saaSendDisconnectMsgHandler(IN P_ADAPTER_T prAdapter,
                                 IN P_STA_RECORD_T prStaRec,
                                 IN P_BSS_INFO_T prAisBssInfo,
                                 IN ENUM_AA_FRM_TYPE_T eFrmType)
{
    do {
        if (eFrmType == FRM_DEAUTH) {
            if (prStaRec->ucStaState == STA_STATE_3) {
                P_MSG_AIS_ABORT_T prAisAbortMsg;

                /* NOTE(Kevin): Change state immediately to
                 * avoid starvation of MSG buffer because of too
                 * many deauth frames before changing the STA
                 * state.
                 */
                cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

                prAisAbortMsg = (P_MSG_AIS_ABORT_T)cnmMemAlloc(
                    prAdapter, RAM_TYPE_MSG, sizeof(MSG_AIS_ABORT_T));
                if (!prAisAbortMsg) {
                    break;
                }

                prAisAbortMsg->rMsgHdr.eMsgId = MID_SAA_AIS_FSM_ABORT;
                prAisAbortMsg->ucReasonOfDisconnect =
                    DISCONNECT_REASON_CODE_DEAUTHENTICATED;
                prAisAbortMsg->fgDelayIndication = false;
                mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prAisAbortMsg,
                            MSG_SEND_METHOD_BUF);
            } else {
                prStaRec->ucTxAuthAssocRetryCount = 0;
                /* 20210421 frog: STA STATE other than STATE_3,
                 * so should be JOIN_ABORT. */
                if (saaFsmSendEventJoinComplete(
                        prAdapter, WLAN_STATUS_JOIN_ABORT, prStaRec, NULL) ==
                    WLAN_STATUS_RESOURCES) {
                    /* can set a timer and retry later */
                    DBGLOG(
                        SAA, WARN,
                        "[SAA]can't alloc msg for inform AIS join complete\n");
                }
            }
        } else {  /* FRM_DISASSOC */
            if (prStaRec->ucStaState == STA_STATE_3) {
                P_MSG_AIS_ABORT_T prAisAbortMsg;

                prAisAbortMsg = (P_MSG_AIS_ABORT_T)cnmMemAlloc(
                    prAdapter, RAM_TYPE_MSG, sizeof(MSG_AIS_ABORT_T));
                if (!prAisAbortMsg) {
                    break;
                }

                prAisAbortMsg->rMsgHdr.eMsgId = MID_SAA_AIS_FSM_ABORT;
                prAisAbortMsg->ucReasonOfDisconnect =
                    DISCONNECT_REASON_CODE_DISASSOCIATED;
                prAisAbortMsg->fgDelayIndication = false;
                mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prAisAbortMsg,
                            MSG_SEND_METHOD_BUF);
            } else {
                prStaRec->ucTxAuthAssocRetryCount = 0;

                /* 20210421 frog: STA STATE other than STATE_3,
                 * so should be JOIN_ABORT. */
                if (saaFsmSendEventJoinComplete(
                        prAdapter, WLAN_STATUS_JOIN_ABORT, prStaRec, NULL) ==
                    WLAN_STATUS_RESOURCES) {
                    /* can set a timer and retry later */
                    DBGLOG(
                        SAA, WARN,
                        "[SAA]can't alloc msg for inform AIS join complete\n");
                }
            }
        }
        if (prAisBssInfo) {
            prAisBssInfo->u2DeauthReason = prStaRec->u2ReasonCode;
        }
    } while (0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check the incoming Disassociation Frame.
 *
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Always not retain disassociation frames
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS saaFsmRunEventRxDisassoc(IN P_ADAPTER_T prAdapter,
                                     IN P_SW_RFB_T prSwRfb)
{
    P_STA_RECORD_T prStaRec;
    P_WLAN_DISASSOC_FRAME_T prDisassocFrame;
    u8 ucWlanIdx;
    struct wireless_dev *wdev = NULL;
    P_BSS_INFO_T prBssInfo = NULL;
    u8 ucRoleIdx = 0;
    u8 *pFrameBuf = NULL;
    u8 fgIsInterruptContext = false;

    ASSERT(prSwRfb);
    prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
    prDisassocFrame = (P_WLAN_DISASSOC_FRAME_T)prSwRfb->pvHeader;
    ucWlanIdx = (u8)HAL_RX_STATUS_GET_WLAN_IDX(prSwRfb->prRxStatus);
    wdev = prAdapter->prGlueInfo->prDevHandler->ieee80211_ptr;

    DBGLOG(SAA, EVENT,
           "Rx Disassoc frame from BSSID[" MACSTR "] DA[" MACSTR
           "] ReasonCode[0x%x]\n",
           MAC2STR(prDisassocFrame->aucBSSID),
           MAC2STR(prDisassocFrame->aucDestAddr),
           prDisassocFrame->u2ReasonCode);

    do {
        /* We should have the corresponding Sta Record. */
        if (!prStaRec) {
            DBGLOG(
                SAA, WARN,
                "Received a DisAssoc: wlanIdx[%d] w/o corresponding staRec\n",
                ucWlanIdx);
            break;
        }

        if (IS_STA_IN_AIS(prStaRec)) {
            P_BSS_INFO_T prAisBssInfo;

            if (!IS_AP_STA(prStaRec)) {
                break;
            }

            prAisBssInfo = prAdapter->prAisBssInfo;

            if (prStaRec->ucStaState > STA_STATE_1) {
                /* Check if this is the AP we are associated or
                 * associating with */
                if (assocProcessRxDisassocFrame(
                        prAdapter, prSwRfb, prStaRec->aucMacAddr,
                        &prStaRec->u2ReasonCode) == WLAN_STATUS_SUCCESS) {
                    P_AIS_SPECIFIC_BSS_INFO_T
                        prAisSpecBssInfo;

                    prAisSpecBssInfo =
                        &(prAdapter->rWifiVar.rAisSpecificBssInfo);

                    DBGLOG(
                        RSN, INFO,
                        "QM RX MGT: Disassoc frame, P=%d Sec=%d CM=%d BC=%d fc=%02x\n",
                        prAisSpecBssInfo->fgMgmtProtection,
                        HAL_RX_STATUS_GET_SEC_MODE(prSwRfb->prRxStatus),
                        HAL_RX_STATUS_IS_CIPHER_MISMATCH(prSwRfb->prRxStatus),
                        IS_BMCAST_MAC_ADDR(prDisassocFrame->aucDestAddr),
                        prDisassocFrame->u2FrameCtrl);
                    if (IS_STA_IN_AIS(prStaRec) &&
                        prAisSpecBssInfo->fgMgmtProtection &&
                        HAL_RX_STATUS_IS_CIPHER_MISMATCH(prSwRfb->prRxStatus)
                        /* HAL_RX_STATUS_GET_SEC_MODE(prSwRfb->prRxStatus)
                         * != CIPHER_SUITE_CCMP */
                        ) {
                        /* prDisassocFrame =
                         * (P_WLAN_DISASSOC_FRAME_T)
                         * prSwRfb->pvHeader; */
                        saaChkDisassocfrmParamHandler(
                            prAdapter, prDisassocFrame, prStaRec, prSwRfb);
                        return WLAN_STATUS_SUCCESS;
                    }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0))
#define CONNECTED_BSS(wdev) wdev->current_bss
#else
#define CONNECTED_BSS(wdev) wdev->connected
#endif
                    DBGLOG(SAA, INFO, "notification of RX disassociation %d\n",
                           prSwRfb->u2PacketLen);
                    if (CONNECTED_BSS(wdev)) {
                        if (in_interrupt()) {
                            pFrameBuf =
                                kalMemAlloc(prSwRfb->u2PacketLen, PHY_MEM_TYPE);
                            fgIsInterruptContext = true;
                        } else {
                            pFrameBuf =
                                kalMemAlloc(prSwRfb->u2PacketLen, VIR_MEM_TYPE);
                            fgIsInterruptContext = false;
                        }

                        if (!pFrameBuf) {
                            DBGLOG(SAA, ERROR,
                                   "Alloc buffer for frame failed\n");
                            return WLAN_STATUS_RESOURCES;
                        }

                        kalMemCopy((void *)pFrameBuf, (void *)prDisassocFrame,
                                   prSwRfb->u2PacketLen);
                        kalWDevLockThread(prAdapter->prGlueInfo,
                                          prAdapter->prGlueInfo->prDevHandler,
                                          CFG80211_RX_MLME_MGMT, pFrameBuf,
                                          prSwRfb->u2PacketLen, NULL, 0, NULL,
                                          0, fgIsInterruptContext);
                    }
                    DBGLOG(SAA, INFO,
                           "notification of RX disassociation Done\n");
                    saaSendDisconnectMsgHandler(prAdapter, prStaRec,
                                                prAisBssInfo, FRM_DISASSOC);
                }
            }
        } else if (prAdapter->fgIsP2PRegistered && (IS_STA_IN_P2P(prStaRec))) {
            DBGLOG(SAA, INFO, "notification of RX disassociation %d\n",
                   prSwRfb->u2PacketLen);
            prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
            ucRoleIdx = (u8)prBssInfo->u4PrivateData;
            wdev = prAdapter->prGlueInfo->prP2PInfo[ucRoleIdx]
                   ->prDevHandler->ieee80211_ptr;
            if (CONNECTED_BSS(wdev)) {
                if (in_interrupt()) {
                    pFrameBuf = kalMemAlloc(prSwRfb->u2PacketLen, PHY_MEM_TYPE);
                    fgIsInterruptContext = true;
                } else {
                    pFrameBuf = kalMemAlloc(prSwRfb->u2PacketLen, VIR_MEM_TYPE);
                    fgIsInterruptContext = false;
                }

                if (!pFrameBuf) {
                    DBGLOG(SAA, ERROR, "Alloc buffer for frame failed\n");
                    return WLAN_STATUS_RESOURCES;
                }

                kalMemCopy((void *)pFrameBuf, (void *)prDisassocFrame,
                           prSwRfb->u2PacketLen);
                kalWDevLockThread(
                    prAdapter->prGlueInfo,
                    prAdapter->prGlueInfo->prP2PInfo[ucRoleIdx]->aprRoleHandler,
                    CFG80211_RX_MLME_MGMT, pFrameBuf, prSwRfb->u2PacketLen,
                    NULL, 0, NULL, 0, fgIsInterruptContext);
            }
            DBGLOG(SAA, INFO, "notification of RX disassociation Done\n");

            p2pRoleFsmRunEventRxDisassociation(prAdapter, prStaRec, prSwRfb);
        } else {
            ASSERT(0);
        }
    } while (false);

    return WLAN_STATUS_SUCCESS;
}

/* for AOSP */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check param of Disassoc frame and reson code for
 * Disassoc
 *
 * @param[in]
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/

void saaChkDisassocfrmParamHandler(IN P_ADAPTER_T prAdapter,
                                   IN P_WLAN_DISASSOC_FRAME_T prDisassocFrame,
                                   IN P_STA_RECORD_T prStaRec,
                                   IN P_SW_RFB_T prSwRfb)
{
    if (!IS_BMCAST_MAC_ADDR(prDisassocFrame->aucDestAddr) &&
        (prStaRec->u2ReasonCode == REASON_CODE_CLASS_2_ERR ||
         prStaRec->u2ReasonCode == REASON_CODE_CLASS_3_ERR)) {
        /* MFP test plan 5.3.3.5 */
        DBGLOG(RSN, INFO, "QM RX MGT: rsnStartSaQuery\n");
        rsnStartSaQuery(prAdapter);
    } else {
        DBGLOG(RSN, INFO, "RXM: Drop unprotected Mgmt frame\n");
        DBGLOG(RSN, INFO,
               "RXM: (MAC RX Done) RX (u2StatusFlag=0x%x) (ucKIdxSecMode=0x%x) "
               "(ucWlanIdx=0x%x)\n",
               prSwRfb->prRxStatus->u2StatusFlag,
               prSwRfb->prRxStatus->ucTidSecMode,
               prSwRfb->prRxStatus->ucWlanIdx);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Abort Event to SAA FSM.
 *
 * @param[in] prMsgHdr   Message of Abort Request for a particular STA.
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void saaFsmRunEventAbort(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
    P_MSG_SAA_FSM_ABORT_T prSaaFsmAbortMsg;
    P_STA_RECORD_T prStaRec;

    ASSERT(prMsgHdr);

    prSaaFsmAbortMsg = (P_MSG_SAA_FSM_ABORT_T)prMsgHdr;
    prStaRec = prSaaFsmAbortMsg->prStaRec;

    ASSERT(prStaRec);
    if (!prStaRec) {
        cnmMemFree(prAdapter, prMsgHdr);
        return;
    }

    DBGLOG(SAA, LOUD, "EVENT-ABORT: Stop SAA FSM.\n");

    cnmMemFree(prAdapter, prMsgHdr);

    /* Reset Send Auth/(Re)Assoc Frame Count */
    prStaRec->ucTxAuthAssocRetryCount = 0;

    /* Cancel JOIN relative Timer */
    cnmTimerStopTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer);

    if (prStaRec->eAuthAssocState != AA_STATE_IDLE) {
        DBGLOG(SAA, LOUD, "EVENT-ABORT: Previous Auth/Assoc State == %s.\n",
               apucDebugAAState[prStaRec->eAuthAssocState]);
    }

    /* Free this StaRec */
    cnmStaRecFree(prAdapter, prStaRec);
}
