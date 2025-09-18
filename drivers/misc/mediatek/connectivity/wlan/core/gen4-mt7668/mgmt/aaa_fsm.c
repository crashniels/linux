// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "aaa_fsm.c"
 *    \brief  This file defines the FSM for AAA MODULE.
 *
 *    This file defines the FSM for AAA MODULE.
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

#if CFG_SUPPORT_AAA

void aaaFsmRunEventTxReqTimeOut(IN P_ADAPTER_T prAdapter,
                                IN unsigned long plParamPtr)
{
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)plParamPtr;
    P_BSS_INFO_T prBssInfo;

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

    ASSERT(prStaRec);
    if (!prStaRec) {
        return;
    }

    DBGLOG(AAA, LOUD, "EVENT-TIMER: TX REQ TIMEOUT, Current Time = %d\n",
           kalGetTimeTick());

    /* Trigger statistics log if Auth/Assoc Tx timeout */
    wlanTriggerStatsLog(prAdapter, prAdapter->rWifiVar.u4StatsLogDuration);

    switch (prStaRec->eAuthAssocState) {
    case AAA_STATE_SEND_AUTH2:
        DBGLOG(AAA, ERROR,
               "LOST EVENT ,Auth Tx done disappear for (%d)Ms\n",
               TU_TO_MSEC(TX_AUTHENTICATION_RESPONSE_TIMEOUT_TU));

        prStaRec->eAuthAssocState = AA_STATE_IDLE;

        /* NOTE(Kevin): Change to STATE_1 */
        cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

#if CFG_ENABLE_WIFI_DIRECT
        if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
            p2pRoleFsmRunEventAAATxFail(prAdapter, prStaRec,
                                        prBssInfo);
        }
#endif
        break;

    default:
        return;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will process the Rx Auth Request Frame and then
 *        trigger AAA FSM.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aaaFsmRunEventRxAuth(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
    P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T)NULL;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;
    u16 u2StatusCode;
    u8 fgReplyAuth = false;
    P_WLAN_AUTH_FRAME_T prAuthFrame = (P_WLAN_AUTH_FRAME_T)NULL;

    ASSERT(prAdapter);

    do {
        prAuthFrame = (P_WLAN_AUTH_FRAME_T)prSwRfb->pvHeader;

#if CFG_ENABLE_WIFI_DIRECT
        prBssInfo = p2pFuncBSSIDFindBssInfo(prAdapter,
                                            prAuthFrame->aucBSSID);

        /* 4 <1> Check P2P network conditions */

        /* if (prBssInfo && prAdapter->fgIsP2PRegistered) */
        /* modify coding sytle to reduce indent */

        if (!prAdapter->fgIsP2PRegistered) {
            goto bow_proc;
        }

        if (prBssInfo && prBssInfo->fgIsNetActive) {
            /* 4 <1.1> Validate Auth Frame by Auth
             * Algorithm/Transation Seq */
            if (WLAN_STATUS_SUCCESS ==
                authProcessRxAuth1Frame(
                    prAdapter,
                    prSwRfb,
                    prBssInfo->aucBSSID,
                    AUTH_ALGORITHM_NUM_OPEN_SYSTEM,
                    AUTH_TRANSACTION_SEQ_1,
                    &u2StatusCode)) {
                if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
                    DBGLOG(AAA, TRACE,
                           "process RxAuth status success\n");
                    /* 4 <1.2> Validate Auth Frame for
                     * Network Specific Conditions */
                    fgReplyAuth = p2pFuncValidateAuth(
                        prAdapter, prBssInfo, prSwRfb,
                        &prStaRec, &u2StatusCode);

#if CFG_SUPPORT_802_11W
                    /* AP PMF, if PMF connection, ignore Rx
                     * auth */
                    /* Certification 4.3.3.4 */
                    if (rsnCheckBipKeyInstalled(prAdapter,
                                                prStaRec)) {
                        DBGLOG(AAA, INFO,
                               "Drop RxAuth\n");
                        return;
                    }
#endif
                } else {
                    fgReplyAuth = true;
                }
                break;
            }
        }
#endif

bow_proc:
        return;
    } while (false);

    if (prStaRec) {
        /* update RCPI */
        ASSERT(prSwRfb->prRxStatusGroup3);
        prStaRec->ucRCPI =
            nicRxGetRcpiValueFromRxv(RCPI_MODE_WF0, prSwRfb);
    }
    /* 4 <3> Update STA_RECORD_T and reply Auth_2(Response to Auth_1) Frame
     */
    if (fgReplyAuth) {
        if (prStaRec) {
            if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
                if (prStaRec->eAuthAssocState !=
                    AA_STATE_IDLE) {
                    DBGLOG(AAA,
                           WARN,
                           "Previous AuthAssocState (%d) != IDLE.\n",
                           prStaRec->eAuthAssocState);
                }

                prStaRec->eAuthAssocState =
                    AAA_STATE_SEND_AUTH2;
            } else {
                prStaRec->eAuthAssocState = AA_STATE_IDLE;

                /* NOTE(Kevin): Change to STATE_1 */
                cnmStaRecChangeState(prAdapter, prStaRec,
                                     STA_STATE_1);
            }

            /* Update the record join time. */
            GET_CURRENT_SYSTIME(&prStaRec->rUpdateTime);

            /* Update Station Record - Status/Reason Code */
            prStaRec->u2StatusCode = u2StatusCode;

            prStaRec->ucAuthAlgNum = AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
        } else {
            /* NOTE(Kevin): We should have STA_RECORD_T if the
             * status code was successful */
            ASSERT(!(u2StatusCode == STATUS_CODE_SUCCESSFUL)
#if CFG_SUPPORT_H2E
                   &&
                   (u2StatusCode != WLAN_STATUS_SAE_HASH_TO_ELEMENT)
#endif
                   );
        }

        /* NOTE: Ignore the return status for AAA */
        /* 4 <4> Reply  Auth */
        authSendAuthFrame(prAdapter, prStaRec, prBssInfo->ucBssIndex,
                          prSwRfb, AUTH_TRANSACTION_SEQ_2,
                          u2StatusCode);

        /*sta_rec might be removed when client list full, skip timer
         * setting*/
        if (prStaRec) {
            cnmTimerStopTimer(prAdapter,
                              &prStaRec->rTxReqDoneOrRxRespTimer);
            /*ToDo:Init Timer to check get Auth Txdone avoid sta_rec
             * not clear*/
            cnmTimerInitTimer(prAdapter,
                              &prStaRec->rTxReqDoneOrRxRespTimer,
                              (PFN_MGMT_TIMEOUT_FUNC)
                              aaaFsmRunEventTxReqTimeOut,
                              (unsigned long)prStaRec);

            cnmTimerStartTimer(
                prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer,
                TU_TO_MSEC(
                    TX_AUTHENTICATION_RESPONSE_TIMEOUT_TU));
        }
    } else if (prStaRec) {
        cnmStaRecFree(prAdapter, prStaRec);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will process the Rx (Re)Association Request Frame and
 * then trigger AAA FSM.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS           Always return success
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS aaaFsmRunEventRxAssoc(IN P_ADAPTER_T prAdapter,
                                  IN P_SW_RFB_T prSwRfb)
{
    P_BSS_INFO_T prBssInfo;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;
    u16 u2StatusCode = STATUS_CODE_RESERVED;
    u8 fgReplyAssocResp = false;
    u8 fgSendSAQ = false;

    ASSERT(prAdapter);
    DBGLOG(AAA, INFO, "aaaFsmRunEventRxAssoc\n");

    do {
        /* 4 <1> Check if we have the STA_RECORD_T for incoming Assoc
         * Req */
        prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);

        /* We should have the corresponding Sta Record. */
        if ((!prStaRec) || (!prStaRec->fgIsInUse)) {
            /* Not to reply association response with failure code
             * due to lack of STA_REC */
            break;
        }

        if (!IS_CLIENT_STA(prStaRec)) {
            break;
        }

        DBGLOG(AAA, TRACE,
               "RxAssoc enter ucStaState:%d, eAuthassocState:%d\n",
               prStaRec->ucStaState, prStaRec->eAuthAssocState);

        if (prStaRec->ucStaState == STA_STATE_3) {
            /* Do Reassocation */
        } else if ((prStaRec->ucStaState == STA_STATE_2) &&
                   (prStaRec->eAuthAssocState ==
                    AAA_STATE_SEND_AUTH2)) {
            /* Normal case */
        } else {
            DBGLOG(AAA, WARN,
                   "Previous AuthAssocState (%d) != SEND_AUTH2.\n",
                   prStaRec->eAuthAssocState);

            /* Maybe Auth Response TX fail, but actually it success.
             */
            cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);
        }

        /* update RCPI */
        ASSERT(prSwRfb->prRxStatusGroup3);
        prStaRec->ucRCPI =
            nicRxGetRcpiValueFromRxv(RCPI_MODE_WF0, prSwRfb);

        /* 4 <2> Check P2P network conditions */
#if CFG_ENABLE_WIFI_DIRECT
        if ((prAdapter->fgIsP2PRegistered) &&
            (IS_STA_IN_P2P(prStaRec))) {
            prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
                                              prStaRec->ucBssIndex);

            if (prBssInfo->fgIsNetActive) {
                /* 4 <2.1> Validate Assoc Req Frame and get
                 * Status Code */
                /* Check if for this BSSID */
                if (WLAN_STATUS_SUCCESS ==
                    assocProcessRxAssocReqFrame(
                        prAdapter, prSwRfb,
                        &u2StatusCode)) {
                    if (u2StatusCode ==
                        STATUS_CODE_SUCCESSFUL) {
                        /* 4 <2.2> Validate Assoc Req
                         * Frame for Network Specific
                         * Conditions */
                        fgReplyAssocResp =
                            p2pFuncValidateAssocReq(
                                prAdapter,
                                prSwRfb,
                                (u16 *)&
                                u2StatusCode);
                    } else {
                        fgReplyAssocResp = true;
                    }

                    break;
                }
            }
        }
#endif

        return WLAN_STATUS_SUCCESS;  /* To release the SW_RFB_T */
    } while (false);

    /* 4 <4> Update STA_RECORD_T and reply Assoc Resp Frame */
    if (fgReplyAssocResp) {
        u16 u2IELength;
        u8 *pucIE;

        cnmTimerStopTimer(prAdapter,
                          &prStaRec->rTxReqDoneOrRxRespTimer);

        if ((((P_WLAN_ASSOC_REQ_FRAME_T)(prSwRfb->pvHeader))
             ->u2FrameCtrl &
             MASK_FRAME_TYPE) == MAC_FRAME_REASSOC_REQ) {
            u2IELength = prSwRfb->u2PacketLen -
                         (u16)OFFSET_OF(WLAN_REASSOC_REQ_FRAME_T,
                                        aucInfoElem[0]);

            pucIE =
                ((P_WLAN_REASSOC_REQ_FRAME_T)(prSwRfb->pvHeader))
                ->aucInfoElem;
        } else {
            u2IELength = prSwRfb->u2PacketLen -
                         (u16)OFFSET_OF(WLAN_ASSOC_REQ_FRAME_T,
                                        aucInfoElem[0]);

            pucIE = ((P_WLAN_ASSOC_REQ_FRAME_T)(prSwRfb->pvHeader))
                    ->aucInfoElem;
        }

        rlmProcessAssocReq(prAdapter, prSwRfb, pucIE, u2IELength);

        /* 4 <4.1> Assign Association ID */
        if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
#if CFG_ENABLE_WIFI_DIRECT
            if ((prAdapter->fgIsP2PRegistered) &&
                (IS_STA_IN_P2P(prStaRec))) {
                prBssInfo = GET_BSS_INFO_BY_INDEX(
                    prAdapter, prStaRec->ucBssIndex);
                if (p2pRoleFsmRunEventAAAComplete(
                        prAdapter, prStaRec, prBssInfo) ==
                    WLAN_STATUS_SUCCESS) {
                    prStaRec->u2AssocId =
                        bssAssignAssocID(prStaRec);
                    /* prStaRec->eAuthAssocState =
                     * AA_STATE_IDLE; */
                    /* NOTE(Kevin): for TX done */
                    prStaRec->eAuthAssocState =
                        AAA_STATE_SEND_ASSOC2;
                    /* NOTE(Kevin): Method A: Change to
                     * STATE_3 before handle TX Done */
                    /* cnmStaRecChangeState(prAdapter,
                     * prStaRec, STA_STATE_3); */
                } else {
                    /* Client List FULL. */
                    u2StatusCode = STATUS_CODE_REQ_DECLINED;

                    prStaRec->u2AssocId = 0;  /* Invalid
                                               * Association
                                               * ID */

                    /* If(Re)association fail,remove sta
                     * record and use class error to handle
                     * sta */
                    prStaRec->eAuthAssocState =
                        AA_STATE_IDLE;

                    /* NOTE(Kevin): Better to change state
                     * here, not at TX Done */
                    cnmStaRecChangeState(prAdapter,
                                         prStaRec,
                                         STA_STATE_2);
                }
            }
#endif
        } else {
#if CFG_SUPPORT_802_11W
            /* AP PMF */
            /* don't change state, just send assoc resp (NO need TX
             * done, TIE + code30) and then SAQ */
            if (u2StatusCode ==
                STATUS_CODE_ASSOC_REJECTED_TEMPORARILY) {
                DBGLOG(AAA, INFO, "AP send SAQ\n");
                fgSendSAQ = true;
            } else
#endif
            {
                prStaRec->u2AssocId = 0;  /* Invalid Association
                                           * ID */

                /* If (Re)association fail, remove sta record
                 * and use class error to handle sta */
                prStaRec->eAuthAssocState = AA_STATE_IDLE;
                /* Remove from client list if it was previously
                 * associated */
                if ((prStaRec->ucStaState > STA_STATE_1) &&
                    prAdapter->fgIsP2PRegistered &&
                    (IS_STA_IN_P2P(prStaRec))) {
                    P_BSS_INFO_T prBssInfo = NULL;

                    prBssInfo = GET_BSS_INFO_BY_INDEX(
                        prAdapter,
                        prStaRec->ucBssIndex);
                    if (prBssInfo) {
                        DBGLOG(AAA, INFO,
                               "Remove client!\n");
                        bssRemoveClient(prAdapter,
                                        prBssInfo,
                                        prStaRec);
                    }
                }
                /* NOTE(Kevin): Better to change state here, not
                 * at TX Done */
                cnmStaRecChangeState(prAdapter, prStaRec,
                                     STA_STATE_2);
            }
        }

        /* Update the record join time. */
        GET_CURRENT_SYSTIME(&prStaRec->rUpdateTime);

        /* Update Station Record - Status/Reason Code */
        prStaRec->u2StatusCode = u2StatusCode;

        /* NOTE: Ignore the return status for AAA */
        /* 4 <4.2> Reply  Assoc Resp */
        assocSendReAssocRespFrame(prAdapter, prStaRec);

#if CFG_SUPPORT_802_11W
        /* AP PMF */
        if (fgSendSAQ) {
            /* if PMF connection, and return code 30, send SAQ */
            rsnApStartSaQuery(prAdapter, prStaRec);
        }
#endif
    }

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle TxDone(Auth2/AssocReq) Event of AAA FSM.
 *
 * @param[in] prAdapter      Pointer to the Adapter structure.
 * @param[in] prMsduInfo     Pointer to the MSDU_INFO_T.
 * @param[in] rTxDoneStatus  Return TX status of the Auth1/Auth3/AssocReq frame.
 *
 * @retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
aaaFsmRunEventTxDone(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo,
                     IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
    P_STA_RECORD_T prStaRec;
    P_BSS_INFO_T prBssInfo;

    ASSERT(prAdapter);
    ASSERT(prMsduInfo);

    DBGLOG(AAA, LOUD, "EVENT-TX DONE: Current Time = %ld\n",
           kalGetTimeTick());

    prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

    if ((!prStaRec) || (!prStaRec->fgIsInUse)) {
        return WLAN_STATUS_SUCCESS;  /* For the case of replying ERROR
                                      * STATUS CODE */

    }
    ASSERT(prStaRec->ucBssIndex <= MAX_BSS_INDEX);

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

    DBGLOG(AAA, LOUD, "TxDone ucStaState:%d, eAuthAssocState:%d\n",
           prStaRec->ucStaState, prStaRec->eAuthAssocState);

    /* Trigger statistics log if Auth/Assoc Tx failed */
    if (rTxDoneStatus != TX_RESULT_SUCCESS) {
        wlanTriggerStatsLog(prAdapter,
                            prAdapter->rWifiVar.u4StatsLogDuration);
    }

    switch (prStaRec->eAuthAssocState) {
    case AAA_STATE_SEND_AUTH2: {
        /* Strictly check the outgoing frame is matched with current AA
         * STATE */
        if (authCheckTxAuthFrame(prAdapter, prMsduInfo,
                                 AUTH_TRANSACTION_SEQ_2) !=
            WLAN_STATUS_SUCCESS) {
            break;
        }

        cnmTimerStopTimer(prAdapter,
                          &prStaRec->rTxReqDoneOrRxRespTimer);

        if (prStaRec->u2StatusCode == STATUS_CODE_SUCCESSFUL) {
            if (rTxDoneStatus == TX_RESULT_SUCCESS) {
                /* NOTE(Kevin): Change to STATE_2 at TX Done */
                cnmStaRecChangeState(prAdapter, prStaRec,
                                     STA_STATE_2);
                /* Error handle if can not complete the ASSOC
                 * flow */
                cnmTimerStartTimer(
                    prAdapter,
                    &prStaRec->rTxReqDoneOrRxRespTimer,
                    TU_TO_MSEC(TX_ASSOCIATE_TIMEOUT_TU));
            } else {
                prStaRec->eAuthAssocState = AA_STATE_IDLE;

                /* NOTE(Kevin): Change to STATE_1 */
                cnmStaRecChangeState(prAdapter, prStaRec,
                                     STA_STATE_1);

#if CFG_ENABLE_WIFI_DIRECT
                if (prBssInfo->eNetworkType ==
                    NETWORK_TYPE_P2P) {
                    p2pRoleFsmRunEventAAATxFail(
                        prAdapter, prStaRec, prBssInfo);
                }
#endif
            }
        }
        /* NOTE(Kevin): Ignore the TX Done Event of Auth Frame with
         * Error Status Code */
    } break;

    case AAA_STATE_SEND_ASSOC2: {
        /* Strictly check the outgoing frame is matched with current SAA
         * STATE */
        if (assocCheckTxReAssocRespFrame(prAdapter, prMsduInfo) !=
            WLAN_STATUS_SUCCESS) {
            break;
        }

        if (prStaRec->u2StatusCode == STATUS_CODE_SUCCESSFUL) {
            if (rTxDoneStatus == TX_RESULT_SUCCESS) {
                prStaRec->eAuthAssocState = AA_STATE_IDLE;

                /* NOTE(Kevin): Change to STATE_3 at TX Done */
#if CFG_ENABLE_WIFI_DIRECT
                if (prBssInfo->eNetworkType ==
                    NETWORK_TYPE_P2P) {
                    p2pRoleFsmRunEventAAASuccess(
                        prAdapter, prStaRec, prBssInfo);
                }
#endif
            } else {
                prStaRec->eAuthAssocState =
                    AAA_STATE_SEND_AUTH2;

                /* NOTE(Kevin): Change to STATE_2 */
                cnmStaRecChangeState(prAdapter, prStaRec,
                                     STA_STATE_2);

#if CFG_ENABLE_WIFI_DIRECT
                if (prBssInfo->eNetworkType ==
                    NETWORK_TYPE_P2P) {
                    p2pRoleFsmRunEventAAATxFail(
                        prAdapter, prStaRec, prBssInfo);
                }
#endif
            }
        }
        /* NOTE(Kevin): Ignore the TX Done Event of Auth Frame with
         * Error Status Code */
    } break;

    case AA_STATE_IDLE:
        /* 2013-08-27 frog:  Do nothing.
         * Somtimes we may send Assoc Resp twice. (Rx Assoc Req before
         * the first Assoc TX Done) The AssocState is changed to IDLE
         * after first TX done. Free station record when IDLE is
         * seriously wrong.
         */
        /* 2017-01-12 Do nothing only when STA is in state 3 */
        /* Free the StaRec if found any unexpected status */
        if (prStaRec->ucStaState != STA_STATE_3) {
            cnmStaRecFree(prAdapter, prStaRec);
        }
        break;

    default:
        break;  /* Ignore other cases */
    }

    DBGLOG(AAA, LOUD, "TxDone end ucStaState:%d, eAuthAssocState:%d\n",
           prStaRec->ucStaState, prStaRec->eAuthAssocState);

    return WLAN_STATUS_SUCCESS;
}
#endif
