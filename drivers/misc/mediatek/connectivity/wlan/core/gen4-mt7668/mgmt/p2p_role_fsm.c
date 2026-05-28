// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */
#include "precomp.h"
#include "p2p_role_state.h"
#include "gl_p2p_os.h"

#if !DBG_DISABLE_ALL_LOG
/*lint -save -e64 Type mismatch */
static u8 *apucDebugP2pRoleState[P2P_ROLE_STATE_NUM] = {
    (u8 *)DISP_STRING("P2P_ROLE_STATE_IDLE"),
    (u8 *)DISP_STRING("P2P_ROLE_STATE_SCAN"),
    (u8 *)DISP_STRING("P2P_ROLE_STATE_REQING_CHANNEL"),
    (u8 *)DISP_STRING("P2P_ROLE_STATE_AP_CHNL_DETECTION"),
#if (CFG_SUPPORT_DFS_MASTER == 1)
    (u8 *)DISP_STRING("P2P_ROLE_STATE_GC_JOIN"),
    (u8 *)DISP_STRING("P2P_ROLE_STATE_DFS_CAC"),
    (u8 *)DISP_STRING("P2P_ROLE_STATE_SWITCH_CHANNEL")
#else
    (u8 *)DISP_STRING("P2P_ROLE_STATE_GC_JOIN")
#endif
};

/*lint -restore */
#endif

u8 p2pRoleFsmInit(IN P_ADAPTER_T prAdapter, IN u8 ucRoleIdx)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_P2P_CHNL_REQ_INFO_T prP2pChnlReqInfo = (P_P2P_CHNL_REQ_INFO_T)NULL;
    P_GL_P2P_INFO_T prP2PInfo = NULL;

    do {
        ASSERT_BREAK(prAdapter != NULL);

        ASSERT_BREAK(P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx) ==
                     NULL);

        prP2pRoleFsmInfo =
            kalMemAlloc(sizeof(P2P_ROLE_FSM_INFO_T), VIR_MEM_TYPE);

        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx) = prP2pRoleFsmInfo;

        ASSERT_BREAK(prP2pRoleFsmInfo != NULL);

        kalMemZero(prP2pRoleFsmInfo, sizeof(P2P_ROLE_FSM_INFO_T));

        prP2pRoleFsmInfo->ucRoleIndex = ucRoleIdx;

        prP2pRoleFsmInfo->eCurrentState = P2P_ROLE_STATE_IDLE;

        prP2pRoleFsmInfo->u4P2pPacketFilter = PARAM_PACKET_FILTER_SUPPORTED;

        prP2pChnlReqInfo = &prP2pRoleFsmInfo->rChnlReqInfo;
        LINK_INITIALIZE(&(prP2pChnlReqInfo->rP2pChnlReqLink));

        cnmTimerInitTimer(prAdapter,
                          &(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer),
                          (PFN_MGMT_TIMEOUT_FUNC)p2pRoleFsmRunEventTimeout,
                          (unsigned long)prP2pRoleFsmInfo);

#if (CFG_SUPPORT_DFS_MASTER == 1)
        cnmTimerInitTimer(
            prAdapter, &(prP2pRoleFsmInfo->rDfsShutDownTimer),
            (PFN_MGMT_TIMEOUT_FUNC)p2pRoleFsmRunEventDfsShutDownTimeout,
            (unsigned long)prP2pRoleFsmInfo);
#if CFG_SUPPORT_DBDC_TC6
        cnmTimerInitTimer(
            prAdapter, &(prP2pRoleFsmInfo->rDfsStartCacTimer),
            (PFN_MGMT_TIMEOUT_FUNC)p2pRoleFsmRunEventStartDfsCacTimeout,
            (unsigned long)prP2pRoleFsmInfo);
#endif
#endif

        prP2pBssInfo = cnmGetBssInfoAndInit(prAdapter, NETWORK_TYPE_P2P, false);

        if (!prP2pBssInfo) {
            DBGLOG(P2P, ERROR, "Error allocating BSS Info Structure\n");
            break;
        }

        BSS_INFO_INIT(prAdapter, prP2pBssInfo);
        prP2pRoleFsmInfo->ucBssIndex = prP2pBssInfo->ucBssIndex;

        /* For state identify, not really used. */
        prP2pBssInfo->eIntendOPMode = OP_MODE_P2P_DEVICE;

        /* glRegisterP2P has setup the mac address */
        /* For wlan0 as AP mode case, this function will be called when
         * changing interface type. And the MAC Addr overwrite by Role
         * isn't expected.
         * Maybe only using ucRoleIdx to calc MAC addr is better than
         * using Role type.
         */
        prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[ucRoleIdx];
        COPY_MAC_ADDR(prP2pBssInfo->aucOwnMacAddr,
                      prP2PInfo->prDevHandler->dev_addr);

        /* For BSS_INFO back trace to P2P Role & get Role FSM. */
        prP2pBssInfo->u4PrivateData = ucRoleIdx;

        if (p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings[ucRoleIdx])) {
            prP2pBssInfo->ucConfigAdHocAPMode = AP_MODE_11G;
            prP2pBssInfo->u2HwDefaultFixedRateCode = RATE_CCK_1M_LONG;
        } else {
            prP2pBssInfo->ucConfigAdHocAPMode = AP_MODE_11G_P2P;
            prP2pBssInfo->u2HwDefaultFixedRateCode = RATE_OFDM_6M;
        }

        prP2pBssInfo->ucNonHTBasicPhyType =
            (u8)rNonHTApModeAttributes[prP2pBssInfo->ucConfigAdHocAPMode]
            .ePhyTypeIndex;
        prP2pBssInfo->u2BSSBasicRateSet =
            rNonHTApModeAttributes[prP2pBssInfo->ucConfigAdHocAPMode]
            .u2BSSBasicRateSet;

        prP2pBssInfo->u2OperationalRateSet =
            rNonHTPhyAttributes[prP2pBssInfo->ucNonHTBasicPhyType]
            .u2SupportedRateSet;

        rateGetDataRatesFromRateSet(prP2pBssInfo->u2OperationalRateSet,
                                    prP2pBssInfo->u2BSSBasicRateSet,
                                    prP2pBssInfo->aucAllSupportedRates,
                                    &prP2pBssInfo->ucAllSupportedRatesLen);

        prP2pBssInfo->prBeacon = cnmMgtPktAlloc(
            prAdapter,
            OFFSET_OF(WLAN_BEACON_FRAME_T, aucInfoElem[0]) + MAX_IE_LENGTH);

        if (prP2pBssInfo->prBeacon) {
            prP2pBssInfo->prBeacon->eSrc = TX_PACKET_MGMT;
            prP2pBssInfo->prBeacon->ucStaRecIndex =
                STA_REC_INDEX_BMCAST;  /* NULL STA_REC */
            prP2pBssInfo->prBeacon->ucBssIndex = prP2pBssInfo->ucBssIndex;
        } else {
            /* Out of memory. */
            ASSERT(false);
        }

        prP2pBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC = PM_UAPSD_ALL;
        prP2pBssInfo->rPmProfSetupInfo.ucBmpTriggerAC = PM_UAPSD_ALL;
        prP2pBssInfo->rPmProfSetupInfo.ucUapsdSp = WMM_MAX_SP_LENGTH_2;
        prP2pBssInfo->ucPrimaryChannel = P2P_DEFAULT_LISTEN_CHANNEL;
        prP2pBssInfo->eBand = BAND_2G4;
        prP2pBssInfo->eBssSCO = CHNL_EXT_SCN;
        prP2pBssInfo->ucNss =
            wlanGetSupportNss(prAdapter, prP2pBssInfo->ucBssIndex);
        prP2pBssInfo->eDBDCBand = ENUM_BAND_0;
#if (CFG_HW_WMM_BY_BSS == 0)
        prP2pBssInfo->ucWmmQueSet =
            (prAdapter->rWifiVar.ucDbdcMode == DBDC_MODE_DISABLED) ?
            DBDC_5G_WMM_INDEX :
            DBDC_2G_WMM_INDEX;
#endif
        if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucQoS)) {
            prP2pBssInfo->fgIsQBSS = true;
        } else {
            prP2pBssInfo->fgIsQBSS = false;
        }

#if (CFG_SUPPORT_DFS_MASTER == 1)
        p2pFuncRadarInfoInit();
#endif
#if CFG_SUPPORT_802_11W
        init_completion(&prP2pBssInfo->rDeauthComp);
        prP2pBssInfo->encryptedDeauthIsInProcess = false;
#endif
        /* SET_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex);
         */

        p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                  P2P_ROLE_STATE_IDLE);
    } while (false);

    if (prP2pBssInfo) {
        return prP2pBssInfo->ucBssIndex;
    } else {
        return P2P_DEV_BSS_INDEX;
    }
}

void p2pRoleFsmUninit(IN P_ADAPTER_T prAdapter, IN u8 ucRoleIdx)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;

    do {
        ASSERT_BREAK(prAdapter != NULL);

        DEBUGFUNC("p2pRoleFsmUninit()");
        DBGLOG(P2P, INFO, "->p2pRoleFsmUninit()\n");

        prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx);

        ASSERT_BREAK(prP2pRoleFsmInfo != NULL);

        if (!prP2pRoleFsmInfo) {
            return;
        }

        prP2pBssInfo = prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex];

        p2pFuncDissolve(prAdapter, prP2pBssInfo, true,
                        REASON_CODE_DEAUTH_LEAVING_BSS, true);

        SET_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex);

        /* Function Dissolve should already enter IDLE state. */
        p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                  P2P_ROLE_STATE_IDLE);

        p2pRoleFsmRunEventAbort(prAdapter, prP2pRoleFsmInfo);

        /* Clear CmdQue */
        kalClearMgmtFramesByBssIdx(prAdapter->prGlueInfo,
                                   prP2pBssInfo->ucBssIndex);
        kalClearSecurityFramesByBssIdx(prAdapter->prGlueInfo,
                                       prP2pBssInfo->ucBssIndex);
        /* Clear PendingTxMsdu */
        nicFreePendingTxMsduInfoByBssIdx(prAdapter, prP2pBssInfo->ucBssIndex);

        /* Deactivate BSS. */
        UNSET_NET_ACTIVE(prAdapter, prP2pRoleFsmInfo->ucBssIndex);

        nicDeactivateNetwork(prAdapter, prP2pRoleFsmInfo->ucBssIndex);

        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx) = NULL;

        if (prP2pBssInfo->prBeacon) {
            cnmMgtPktFree(prAdapter, prP2pBssInfo->prBeacon);
            prP2pBssInfo->prBeacon = NULL;
        }

        cnmFreeBssInfo(prAdapter, prP2pBssInfo);

        /* ensure the timer be stopped */
        cnmTimerStopTimer(prAdapter,
                          &(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer));

#if (CFG_SUPPORT_DFS_MASTER == 1)
        cnmTimerStopTimer(prAdapter, &(prP2pRoleFsmInfo->rDfsShutDownTimer));
#if CFG_SUPPORT_DBDC
        cnmTimerStopTimer(prAdapter, &(prP2pRoleFsmInfo->rDfsStartCacTimer));
#endif
#endif

        if (prP2pRoleFsmInfo) {
            kalMemFree(prP2pRoleFsmInfo, VIR_MEM_TYPE,
                       sizeof(P2P_ROLE_FSM_INFO_T));
        }
    } while (false);

    return;
}

void p2pRoleFsmStateTransition(IN P_ADAPTER_T prAdapter,
                               IN P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo,
                               IN ENUM_P2P_ROLE_STATE_T eNextState)
{
    u8 fgIsTransitionOut = (u8)false;
    P_BSS_INFO_T prP2pRoleBssInfo = (P_BSS_INFO_T)NULL;

    prP2pRoleBssInfo =
        GET_BSS_INFO_BY_INDEX(prAdapter, prP2pRoleFsmInfo->ucBssIndex);

    do {
        if (!IS_BSS_ACTIVE(prP2pRoleBssInfo)) {
            if (!cnmP2PIsPermitted(prAdapter)) {
                return;
            }

            SET_NET_ACTIVE(prAdapter, prP2pRoleBssInfo->ucBssIndex);
            nicActivateNetwork(prAdapter, prP2pRoleBssInfo->ucBssIndex);
        }

        fgIsTransitionOut = fgIsTransitionOut ? false : true;

        if (!fgIsTransitionOut) {
            DBGLOG(P2P, STATE,
                   "[P2P_ROLE][%d]TRANSITION(Bss%d): [%s] -> [%s]\n",
                   prP2pRoleFsmInfo->ucRoleIndex, prP2pRoleFsmInfo->ucBssIndex,
                   apucDebugP2pRoleState[prP2pRoleFsmInfo->eCurrentState],
                   apucDebugP2pRoleState[eNextState]);

            /* Transition into current state. */
            prP2pRoleFsmInfo->eCurrentState = eNextState;
        }

        switch (prP2pRoleFsmInfo->eCurrentState) {
        case P2P_ROLE_STATE_IDLE:
            if (!fgIsTransitionOut) {
                p2pRoleStateInit_IDLE(prAdapter, prP2pRoleFsmInfo,
                                      prP2pRoleBssInfo);
            } else {
                p2pRoleStateAbort_IDLE(prAdapter, prP2pRoleFsmInfo,
                                       &(prP2pRoleFsmInfo->rChnlReqInfo));
            }
            break;

        case P2P_ROLE_STATE_SCAN:
            if (!fgIsTransitionOut) {
                p2pRoleStateInit_SCAN(prAdapter, prP2pRoleFsmInfo->ucBssIndex,
                                      &(prP2pRoleFsmInfo->rScanReqInfo));
            } else {
                p2pRoleStateAbort_SCAN(prAdapter, prP2pRoleFsmInfo);
            }
            break;

        case P2P_ROLE_STATE_REQING_CHANNEL:
            if (!fgIsTransitionOut) {
                p2pRoleStateInit_REQING_CHANNEL(
                    prAdapter, prP2pRoleFsmInfo->ucBssIndex,
                    &(prP2pRoleFsmInfo->rChnlReqInfo));
            } else {
                p2pRoleStateAbort_REQING_CHANNEL(prAdapter, prP2pRoleBssInfo,
                                                 prP2pRoleFsmInfo, eNextState);
            }
            break;

        case P2P_ROLE_STATE_AP_CHNL_DETECTION:
            if (!fgIsTransitionOut) {
                p2pRoleStateInit_AP_CHNL_DETECTION(
                    prAdapter, prP2pRoleFsmInfo->ucBssIndex,
                    &(prP2pRoleFsmInfo->rScanReqInfo),
                    &(prP2pRoleFsmInfo->rConnReqInfo));
            } else {
                p2pRoleStateAbort_AP_CHNL_DETECTION(
                    prAdapter, prP2pRoleFsmInfo->ucBssIndex,
                    &(prP2pRoleFsmInfo->rConnReqInfo),
                    &(prP2pRoleFsmInfo->rChnlReqInfo),
                    &(prP2pRoleFsmInfo->rScanReqInfo), eNextState);
            }
            break;

        case P2P_ROLE_STATE_GC_JOIN:
            if (!fgIsTransitionOut) {
                p2pRoleStateInit_GC_JOIN(prAdapter, prP2pRoleFsmInfo,
                                         &(prP2pRoleFsmInfo->rChnlReqInfo));
            } else {
                p2pRoleStateAbort_GC_JOIN(prAdapter, prP2pRoleFsmInfo,
                                          &(prP2pRoleFsmInfo->rJoinInfo),
                                          eNextState);
            }
            break;

#if (CFG_SUPPORT_DFS_MASTER == 1)
        case P2P_ROLE_STATE_DFS_CAC:
            if (!fgIsTransitionOut) {
                p2pRoleStateInit_DFS_CAC(prAdapter,
                                         prP2pRoleFsmInfo->ucBssIndex,
                                         &(prP2pRoleFsmInfo->rChnlReqInfo));
            } else {
                p2pRoleStateAbort_DFS_CAC(prAdapter, prP2pRoleBssInfo,
                                          prP2pRoleFsmInfo, eNextState);
            }
            break;

        case P2P_ROLE_STATE_SWITCH_CHANNEL:
            if (!fgIsTransitionOut) {
                p2pRoleStateInit_SWITCH_CHANNEL(
                    prAdapter, prP2pRoleFsmInfo->ucBssIndex,
                    &(prP2pRoleFsmInfo->rChnlReqInfo));
            } else {
                p2pRoleStateAbort_SWITCH_CHANNEL(prAdapter, prP2pRoleBssInfo,
                                                 prP2pRoleFsmInfo, eNextState);
            }
            break;

#endif
        default:
            ASSERT(false);
            break;
        }
    } while (fgIsTransitionOut);
}

void p2pRoleFsmRunEventTimeout(IN P_ADAPTER_T prAdapter,
                               IN unsigned long ulParamPtr)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)ulParamPtr;
    P_P2P_CHNL_REQ_INFO_T prP2pChnlReqInfo = (P_P2P_CHNL_REQ_INFO_T)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prP2pRoleFsmInfo != NULL));

        switch (prP2pRoleFsmInfo->eCurrentState) {
        case P2P_ROLE_STATE_IDLE:
            prP2pChnlReqInfo = &(prP2pRoleFsmInfo->rChnlReqInfo);
            if (prP2pChnlReqInfo->fgIsChannelRequested) {
                p2pFuncReleaseCh(prAdapter, prP2pRoleFsmInfo->ucBssIndex,
                                 prP2pChnlReqInfo);
                if (IS_NET_PWR_STATE_IDLE(prAdapter,
                                          prP2pRoleFsmInfo->ucBssIndex)) {
                    ASSERT(false);
                }
            }

            if (IS_NET_PWR_STATE_IDLE(prAdapter,
                                      prP2pRoleFsmInfo->ucBssIndex)) {
                DBGLOG(P2P, TRACE, "Role BSS IDLE, deactive network.\n");
                UNSET_NET_ACTIVE(prAdapter, prP2pRoleFsmInfo->ucBssIndex);
                nicDeactivateNetwork(prAdapter, prP2pRoleFsmInfo->ucBssIndex);
                nicUpdateBss(prAdapter, prP2pRoleFsmInfo->ucBssIndex);
            }
            break;

        case P2P_ROLE_STATE_GC_JOIN:
            DBGLOG(
                P2P, ERROR,
                "Current P2P Role State P2P_ROLE_STATE_GC_JOIN is unexpected for "
                "FSM timeout event.\n");
            break;

#if (CFG_SUPPORT_DFS_MASTER == 1)
        case P2P_ROLE_STATE_DFS_CAC:
            p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                      P2P_ROLE_STATE_IDLE);
            kalP2PCacFinishedUpdate(prAdapter->prGlueInfo,
                                    prP2pRoleFsmInfo->ucRoleIndex);
#ifndef CFG_SUPPORT_DISABLE_SAP_DFS_RDD
            p2pFuncSetDfsState(DFS_STATE_ACTIVE);
            cnmTimerStartTimer(prAdapter,
                               &(prP2pRoleFsmInfo->rDfsShutDownTimer), 5000);
#else
            p2pFuncSetDfsState(DFS_STATE_INACTIVE);
#endif
            break;

#endif
        default:
            DBGLOG(
                P2P, ERROR,
                "Current P2P Role State %d is unexpected for FSM timeout event.\n",
                prP2pRoleFsmInfo->eCurrentState);
            ASSERT(false);
            break;
        }
    } while (false);
}

static void p2pRoleFsmDeauthComplete(IN P_ADAPTER_T prAdapter,
                                     IN P_STA_RECORD_T prStaRec)
{
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    ENUM_PARAM_MEDIA_STATE_T eOriMediaStatus;
    P_GL_P2P_INFO_T prP2PInfo;
    u16 u2ReasonCode;
    u8 fgIsLocallyGenerated;

    DBGLOG(P2P, INFO, "Deauth TX Complete!\n");

    if (!prAdapter) {
        DBGLOG(P2P, ERROR, "prAdapter shouldn't be NULL!\n");
        return;
    }

    if (!prStaRec) {
        DBGLOG(P2P, ERROR, "prStaRec shouldn't be NULL!\n");
        return;
    }

    prP2pBssInfo = prAdapter->aprBssInfo[prStaRec->ucBssIndex];
    if (!prP2pBssInfo) {
        DBGLOG(P2P, ERROR, "prP2pBssInfo shouldn't be NULL!\n");
        return;
    }

    eOriMediaStatus = prP2pBssInfo->eConnectionState;
    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prP2pBssInfo->u4PrivateData);

    if (!prP2pRoleFsmInfo) {
        DBGLOG(P2P, ERROR, "prP2pRoleFsmInfo shouldn't be NULL!\n");
        return;
    }

    prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[prP2pRoleFsmInfo->ucRoleIndex];

    if (!prP2PInfo) {
        DBGLOG(P2P, ERROR, "prP2PInfo shouldn't be NULL!\n");
        return;
    }

#if CFG_SUPPORT_802_11W
    /* Notify completion after encrypted deauth frame tx done */
    if (prP2pBssInfo->encryptedDeauthIsInProcess == true) {
        if (!completion_done(&prP2pBssInfo->rDeauthComp)) {
            DBGLOG(P2P, TRACE, "Complete rDeauthComp\n");
            complete(&prP2pBssInfo->rDeauthComp);
        }
    }
    prP2pBssInfo->encryptedDeauthIsInProcess = false;
#endif

    /*
     * After EAP exchange, GO/GC will disconnect
     * and re-connect in short time.
     * GC's new station record will be removed unexpectedly at GO's side
     * if new GC's connection happens
     * when previous GO's disconnection flow is
     * processing. 4-way handshake will NOT be triggered.
     */
    if ((prStaRec->eAuthAssocState == AAA_STATE_SEND_AUTH2 ||
         prStaRec->eAuthAssocState == AAA_STATE_SEND_ASSOC2) &&
        (prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) &&
        (p2pFuncIsAPMode(prAdapter->rWifiVar
                         .prP2PConnSettings[prP2pBssInfo->u4PrivateData]) ==
         false)) {
        DBGLOG(P2P, WARN,
               "Skip deauth tx done since AAA fsm is in progress.\n");
        return;
    } else if (prStaRec->eAuthAssocState == SAA_STATE_SEND_AUTH1 ||
               prStaRec->eAuthAssocState == SAA_STATE_SEND_ASSOC1) {
        DBGLOG(P2P, WARN,
               "Skip deauth tx done since SAA fsm is in progress.\n");
        return;
    }

    /* Change station state. */
    cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

    /* Save ReasonCode */
    u2ReasonCode = prStaRec->u2ReasonCode;
    fgIsLocallyGenerated = prStaRec->fgIsLocallyGenerated;

    /* Reset Station Record Status. */
    p2pFuncResetStaRecStatus(prAdapter, prStaRec);

    /* Try to remove StaRec in BSS client list before free it */
    bssRemoveClient(prAdapter, prP2pBssInfo, prStaRec);

    /* STA_RECORD free */
    cnmStaRecFree(prAdapter, prStaRec);

    if ((prP2pBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) ||
        (bssGetClientCount(prAdapter, prP2pBssInfo) == 0)) {
        if (prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
            DBGLOG(P2P, TRACE, "No More Client, Media Status DISCONNECTED\n");
        } else {
            DBGLOG(P2P, INFO,
                   "Deauth done, Media Status DISCONNECTED, reason=%d\n",
                   u2ReasonCode);
        }
        p2pChangeMediaState(prAdapter, prP2pBssInfo,
                            PARAM_MEDIA_STATE_DISCONNECTED);
        if (prP2pBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
            kalP2PGCIndicateConnectionStatus(
                prAdapter->prGlueInfo, prP2pRoleFsmInfo->ucRoleIndex, NULL,
                NULL, 0, u2ReasonCode,
                fgIsLocallyGenerated ? WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY :
                WLAN_STATUS_MEDIA_DISCONNECT);
        }
    }

    /* STOP BSS if power is IDLE */
    if (prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
        if (IS_NET_PWR_STATE_IDLE(prAdapter, prP2pRoleFsmInfo->ucBssIndex) &&
            (bssGetClientCount(prAdapter, prP2pBssInfo) == 0)) {
            /* All Peer disconnected !! Stop BSS now!! */
            p2pFuncStopComplete(prAdapter, prP2pBssInfo);
            p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                      P2P_ROLE_STATE_IDLE);
        } else if (eOriMediaStatus != prP2pBssInfo->eConnectionState) {
            /* Update the Media State if necessary */
            nicUpdateBss(prAdapter, prP2pBssInfo->ucBssIndex);
        }
    } else {  /* GC : Stop BSS when Deauth done */
        p2pFuncStopComplete(prAdapter, prP2pBssInfo);
        p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                  P2P_ROLE_STATE_IDLE);
    }
}

void p2pRoleFsmDeauthTimeout(IN P_ADAPTER_T prAdapter,
                             IN unsigned long ulParamPtr)
{
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)ulParamPtr;

    p2pRoleFsmDeauthComplete(prAdapter, prStaRec);
}

void p2pRoleFsmRunEventAbort(IN P_ADAPTER_T prAdapter,
                             IN P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo)
{
    do {
        ASSERT_BREAK((prAdapter != NULL) && (prP2pRoleFsmInfo != NULL));

        if (prP2pRoleFsmInfo->eCurrentState != P2P_ROLE_STATE_IDLE) {
            /* Get into IDLE state. */
            p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                      P2P_ROLE_STATE_IDLE);
        }

        /* Abort IDLE. */
        p2pRoleStateAbort_IDLE(prAdapter, prP2pRoleFsmInfo,
                               &(prP2pRoleFsmInfo->rChnlReqInfo));
    } while (false);
}

WLAN_STATUS
p2pRoleFsmRunEventDeauthTxDone(IN P_ADAPTER_T prAdapter,
                               IN P_MSDU_INFO_T prMsduInfo,
                               IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prMsduInfo != NULL));

        DBGLOG(P2P, INFO, "Deauth TX Done,rTxDoneStatus = %d\n", rTxDoneStatus);

        prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

        if (prStaRec == NULL) {
            DBGLOG(P2P, TRACE, "Station Record NULL, Index:%d\n",
                   prMsduInfo->ucStaRecIndex);
            break;
        }

        p2pRoleFsmDeauthComplete(prAdapter, prStaRec);
        /* Avoid re-entry */
        cnmTimerStopTimer(prAdapter, &(prStaRec->rDeauthTxDoneTimer));
    } while (false);

    return WLAN_STATUS_SUCCESS;
}

void p2pRoleFsmRunEventRxDeauthentication(IN P_ADAPTER_T prAdapter,
                                          IN P_STA_RECORD_T prStaRec,
                                          IN P_SW_RFB_T prSwRfb)
{
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    u16 u2ReasonCode = 0;
    u8 fgSendDeauth = false;  /* flag to send deauth when rx sta
                               * disassc/deauth */

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prSwRfb != NULL));

        if (prStaRec == NULL) {
            prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
        }

        if (!prStaRec) {
            break;
        }

        prP2pBssInfo = prAdapter->aprBssInfo[prStaRec->ucBssIndex];

        if (prStaRec->ucStaState == STA_STATE_1) {
            break;
        }

        DBGLOG(P2P, TRACE, "RX Deauth\n");

        switch (prP2pBssInfo->eCurrentOPMode) {
        case OP_MODE_INFRASTRUCTURE:
            if (authProcessRxDeauthFrame(prSwRfb, prStaRec->aucMacAddr,
                                         &u2ReasonCode) ==
                WLAN_STATUS_SUCCESS) {
                P_WLAN_DEAUTH_FRAME_T prDeauthFrame =
                    (P_WLAN_DEAUTH_FRAME_T)prSwRfb->pvHeader;
                u16 u2IELength = 0;

                if (prP2pBssInfo->prStaRecOfAP != prStaRec) {
                    break;
                }

                prStaRec->u2ReasonCode = u2ReasonCode;
                u2IELength = prSwRfb->u2PacketLen -
                             (WLAN_MAC_HEADER_LEN + REASON_CODE_FIELD_LEN);

                ASSERT(prP2pBssInfo->prStaRecOfAP == prStaRec);

                /* Indicate disconnect to Host. */
                kalP2PGCIndicateConnectionStatus(
                    prAdapter->prGlueInfo, (u8)prP2pBssInfo->u4PrivateData,
                    NULL, prDeauthFrame->aucInfoElem, u2IELength, u2ReasonCode,
                    WLAN_STATUS_MEDIA_DISCONNECT);

                prP2pBssInfo->prStaRecOfAP = NULL;

                p2pFuncDisconnect(prAdapter, prP2pBssInfo, prStaRec, false,
                                  u2ReasonCode, false);

                p2pFuncStopComplete(prAdapter, prP2pBssInfo);

                SET_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex);

                p2pRoleFsmStateTransition(
                    prAdapter,
                    P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
                                                   prP2pBssInfo->u4PrivateData),
                    P2P_ROLE_STATE_IDLE);
            }
            break;

        case OP_MODE_ACCESS_POINT:
            /* Delete client from client list. */
            if (authProcessRxDeauthFrame(prSwRfb, prP2pBssInfo->aucBSSID,
                                         &u2ReasonCode) ==
                WLAN_STATUS_SUCCESS) {
#if CFG_SUPPORT_802_11W
                /* AP PMF */
                if (rsnCheckBipKeyInstalled(prAdapter, prStaRec)) {
                    if (HAL_RX_STATUS_IS_CIPHER_MISMATCH(prSwRfb->prRxStatus) ||
                        HAL_RX_STATUS_IS_CLM_ERROR(prSwRfb->prRxStatus)) {
                        /* if cipher mismatch, or
                         * incorrect encrypt, just drop
                         */
                        DBGLOG(P2P, ERROR, "Rx deauth CM/CLM=1\n");
                        return;
                    }

                    /* 4.3.3.1 send unprotected deauth
                     * reason 6/7 */
                    DBGLOG(P2P, INFO, "deauth reason=6\n");
                    fgSendDeauth = true;
                    u2ReasonCode = REASON_CODE_CLASS_2_ERR;
                    prStaRec->rPmfCfg.fgRxDeauthResp = true;
                }
#endif

                if (bssRemoveClient(prAdapter, prP2pBssInfo, prStaRec)) {
                    /* Indicate disconnect to Host. */
                    p2pFuncDisconnect(prAdapter, prP2pBssInfo, prStaRec,
                                      fgSendDeauth, u2ReasonCode, false);
                    /* Deactive BSS if PWR is IDLE and no
                     * peer */
                    if (IS_NET_PWR_STATE_IDLE(prAdapter,
                                              prP2pBssInfo->ucBssIndex) &&
                        (bssGetClientCount(prAdapter, prP2pBssInfo) == 0)) {
                        /* All Peer disconnected !! Stop
                         * BSS now!! */
                        p2pFuncStopComplete(prAdapter, prP2pBssInfo);
                    }
                }
            }
            break;

        case OP_MODE_P2P_DEVICE:
        default:
            /* Findout why someone sent deauthentication frame to
             * us. */
            ASSERT(false);
            break;
        }

        DBGLOG(P2P, TRACE, "Deauth Reason:%d\n", u2ReasonCode);
    } while (false);
}

void p2pRoleFsmRunEventRxDisassociation(IN P_ADAPTER_T prAdapter,
                                        IN P_STA_RECORD_T prStaRec,
                                        IN P_SW_RFB_T prSwRfb)
{
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    u16 u2ReasonCode = 0;
    u8 fgSendDeauth = false;  /* flag to send deauth when rx sta
                               * disassc/deauth */

    if (prStaRec == NULL) {
        prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
    }

    if (!prStaRec) {
        DBGLOG(P2P, ERROR, "prStaRec of prSwRfb->ucStaRecIdx %d is NULL!\n",
               prSwRfb->ucStaRecIdx);
        return;
    }

    prP2pBssInfo = prAdapter->aprBssInfo[prStaRec->ucBssIndex];

    if (prStaRec->ucStaState == STA_STATE_1) {
        return;
    }

    DBGLOG(P2P, TRACE, "RX Disassoc\n");

    switch (prP2pBssInfo->eCurrentOPMode) {
    case OP_MODE_INFRASTRUCTURE:
        if (assocProcessRxDisassocFrame(
                prAdapter, prSwRfb, prStaRec->aucMacAddr,
                &prStaRec->u2ReasonCode) == WLAN_STATUS_SUCCESS) {
            P_WLAN_DISASSOC_FRAME_T prDisassocFrame =
                (P_WLAN_DISASSOC_FRAME_T)prSwRfb->pvHeader;
            u16 u2IELength = 0;

            ASSERT(prP2pBssInfo->prStaRecOfAP == prStaRec);

            if (prP2pBssInfo->prStaRecOfAP != prStaRec) {
                break;
            }

            u2IELength = prSwRfb->u2PacketLen -
                         (WLAN_MAC_HEADER_LEN + REASON_CODE_FIELD_LEN);

            /* Indicate disconnect to Host. */
            kalP2PGCIndicateConnectionStatus(prAdapter->prGlueInfo,
                                             (u8)prP2pBssInfo->u4PrivateData,
                                             NULL, prDisassocFrame->aucInfoElem,
                                             u2IELength, prStaRec->u2ReasonCode,
                                             WLAN_STATUS_MEDIA_DISCONNECT);

            prP2pBssInfo->prStaRecOfAP = NULL;

            p2pFuncDisconnect(prAdapter, prP2pBssInfo, prStaRec, false,
                              prStaRec->u2ReasonCode, false);

            p2pFuncStopComplete(prAdapter, prP2pBssInfo);

            SET_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex);

            p2pRoleFsmStateTransition(
                prAdapter,
                P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
                                               prP2pBssInfo->u4PrivateData),
                P2P_ROLE_STATE_IDLE);
        }
        break;

    case OP_MODE_ACCESS_POINT:
        /* Delete client from client list. */
        if (assocProcessRxDisassocFrame(prAdapter, prSwRfb,
                                        prP2pBssInfo->aucBSSID,
                                        &u2ReasonCode) == WLAN_STATUS_SUCCESS) {
#if CFG_SUPPORT_802_11W
            /* AP PMF */
            if (rsnCheckBipKeyInstalled(prAdapter, prStaRec)) {
                if (HAL_RX_STATUS_IS_CIPHER_MISMATCH(prSwRfb->prRxStatus) ||
                    HAL_RX_STATUS_IS_CLM_ERROR(prSwRfb->prRxStatus)) {
                    /* if cipher mismatch, or incorrect
                     * encrypt, just drop */
                    DBGLOG(P2P, ERROR, "Rx disassoc CM/CLM=1\n");
                    return;
                }

                /* 4.3.3.1 send unprotected deauth reason 6/7 */
                DBGLOG(P2P, INFO, "deauth reason=6\n");
                fgSendDeauth = true;
                u2ReasonCode = REASON_CODE_CLASS_2_ERR;
                prStaRec->rPmfCfg.fgRxDeauthResp = true;
            }
#endif

            if (bssRemoveClient(prAdapter, prP2pBssInfo, prStaRec)) {
                /* Indicate disconnect to Host. */
                p2pFuncDisconnect(prAdapter, prP2pBssInfo, prStaRec,
                                  fgSendDeauth, u2ReasonCode, false);
                /* Deactive BSS if PWR is IDLE and no peer */
                if (IS_NET_PWR_STATE_IDLE(prAdapter,
                                          prP2pBssInfo->ucBssIndex) &&
                    (bssGetClientCount(prAdapter, prP2pBssInfo) == 0)) {
                    /* All Peer disconnected !! Stop BSS
                     * now!! */
                    p2pFuncStopComplete(prAdapter, prP2pBssInfo);
                }
            }
        }
        break;

    case OP_MODE_P2P_DEVICE:
    default:
        ASSERT(false);
        break;
    }
}

void p2pRoleFsmRunEventBeaconTimeout(IN P_ADAPTER_T prAdapter,
                                     IN P_BSS_INFO_T prP2pBssInfo)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prP2pBssInfo != NULL));

        prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
            prAdapter, prP2pBssInfo->u4PrivateData);

        /* Only client mode would have beacon lost event. */
        if (prP2pBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE) {
            DBGLOG(P2P, ERROR,
                   "Error case, P2P BSS %d not INFRA mode but beacon timeout\n",
                   prP2pRoleFsmInfo->ucRoleIndex);
            break;
        }

        DBGLOG(P2P, TRACE,
               "p2pFsmRunEventBeaconTimeout: BSS %d Beacon Timeout\n",
               prP2pRoleFsmInfo->ucRoleIndex);

        if (prP2pBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
            /* Indicate disconnect to Host. */
            kalP2PGCIndicateConnectionStatus(
                prAdapter->prGlueInfo, prP2pRoleFsmInfo->ucRoleIndex, NULL,
                NULL, 0, REASON_CODE_DISASSOC_LEAVING_BSS,
                WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY);

            if (prP2pBssInfo->prStaRecOfAP != NULL) {
                P_STA_RECORD_T prStaRec = prP2pBssInfo->prStaRecOfAP;
                prP2pBssInfo->prStaRecOfAP = NULL;

                p2pFuncDisconnect(prAdapter, prP2pBssInfo, prStaRec, true,
                                  REASON_CODE_DISASSOC_LEAVING_BSS, true);
                p2pRoleFsmDeauthComplete(prAdapter, prStaRec);

                SET_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex);
                p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                          P2P_ROLE_STATE_IDLE);
            }
        }
    } while (false);
}

/*================== Message Event ==================*/
void p2pRoleFsmRunEventStartAP(IN P_ADAPTER_T prAdapter,
                               IN P_MSG_HDR_T prMsgHdr)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_MSG_P2P_START_AP_T prP2pStartAPMsg = (P_MSG_P2P_START_AP_T)NULL;
    P_P2P_CONNECTION_REQ_INFO_T prP2pConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_P2P_SPECIFIC_BSS_INFO_T prP2pSpecificBssInfo =
        (P_P2P_SPECIFIC_BSS_INFO_T)NULL;
#if CFG_SUPPORT_DBDC
    CNM_DBDC_CAP_T rDbdcCap;
#endif

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventStartAP\n");

    prP2pStartAPMsg = (P_MSG_P2P_START_AP_T)prMsgHdr;

    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prP2pStartAPMsg->ucRoleIdx);

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventStartAP with Role(%d)\n",
           prP2pStartAPMsg->ucRoleIdx);

    if (!prP2pRoleFsmInfo) {
        DBGLOG(
            P2P, ERROR,
            "p2pRoleFsmRunEventStartAP: Corresponding P2P Role FSM empty: %d.\n",
            prP2pStartAPMsg->ucRoleIdx);
        goto error;
    }

    prP2pBssInfo = prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex];
    prP2pSpecificBssInfo =
        prAdapter->rWifiVar.prP2pSpecificBssInfo[prP2pBssInfo->u4PrivateData];
    prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);

    if (prP2pStartAPMsg->u4BcnInterval) {
        DBGLOG(P2P, TRACE, "Beacon interval updated to :%ld\n",
               prP2pStartAPMsg->u4BcnInterval);
        prP2pBssInfo->u2BeaconInterval = (u16)prP2pStartAPMsg->u4BcnInterval;
    } else if (prP2pBssInfo->u2BeaconInterval == 0) {
        prP2pBssInfo->u2BeaconInterval = DOT11_BEACON_PERIOD_DEFAULT;
    }

    if (prP2pStartAPMsg->u4DtimPeriod) {
        DBGLOG(P2P, TRACE, "DTIM interval updated to :%ld\n",
               prP2pStartAPMsg->u4DtimPeriod);
        prP2pBssInfo->ucDTIMPeriod = (u8)prP2pStartAPMsg->u4DtimPeriod;
    } else if (prP2pBssInfo->ucDTIMPeriod == 0) {
        prP2pBssInfo->ucDTIMPeriod = DOT11_DTIM_PERIOD_DEFAULT;
    }

    if (prP2pStartAPMsg->u2SsidLen != 0) {
        kalMemCopy(prP2pConnReqInfo->rSsidStruct.aucSsid,
                   prP2pStartAPMsg->aucSsid, prP2pStartAPMsg->u2SsidLen);
        prP2pConnReqInfo->rSsidStruct.ucSsidLen =
            prP2pSpecificBssInfo->u2GroupSsidLen = prP2pStartAPMsg->u2SsidLen;
        kalMemCopy(prP2pSpecificBssInfo->aucGroupSsid, prP2pStartAPMsg->aucSsid,
                   prP2pStartAPMsg->u2SsidLen);
    }

    if (p2pFuncIsAPMode(
            prAdapter->rWifiVar.prP2PConnSettings[prP2pStartAPMsg->ucRoleIdx]))
    {
        prP2pConnReqInfo->eConnRequest = P2P_CONNECTION_TYPE_PURE_AP;

        /* Overwrite AP channel */
        if (prAdapter->rWifiVar.ucApChannel &&
            prAdapter->rWifiVar.ucApChnlDefFromCfg) {
            prP2pConnReqInfo->rChannelInfo.ucChannelNum =
                prAdapter->rWifiVar.ucApChannel;

            if (prAdapter->rWifiVar.ucApChannel <= 14) {
                prP2pConnReqInfo->rChannelInfo.eBand = BAND_2G4;
            } else {
                prP2pConnReqInfo->rChannelInfo.eBand = BAND_5G;
            }
        }
    } else {
        prP2pConnReqInfo->eConnRequest = P2P_CONNECTION_TYPE_GO;
    }

    /* Clear current AP's STA_RECORD_T and current AID to prevent
     * using previous p2p connection state. This is needed because
     * upper layer may add keys before we start SAP/GO.
     */
    prP2pBssInfo->prStaRecOfAP = (P_STA_RECORD_T)NULL;
    prP2pBssInfo->u2AssocId = 0;

    /* Clear list to ensure no client staRec */
    if (bssGetClientCount(prAdapter, prP2pBssInfo) != 0) {
        DBGLOG(P2P, WARN, "Clear list to ensure no empty/client staRec\n");
        bssInitializeClientList(prAdapter, prP2pBssInfo);
    }

    /* The supplicant may start AP before rP2pRoleFsmTimeoutTimer is time
     * out */
    /* We need to make sure the BSS was deactivated and all StaRec can be
     * free */
    if (timerPendingTimer(&(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer))) {
        /* call p2pRoleFsmRunEventTimeout() to deactive BSS and free
         * channel */
        p2pRoleFsmRunEventTimeout(prAdapter, (unsigned long)prP2pRoleFsmInfo);
        cnmTimerStopTimer(prAdapter,
                          &(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer));
    }

#if (CFG_SUPPORT_DFS_MASTER == 1)
    if (timerPendingTimer(&(prP2pRoleFsmInfo->rDfsShutDownTimer))) {
        DBGLOG(P2P, INFO,
               "p2pRoleFsmRunEventStartAP: Stop DFS shut down timer.\n");
        cnmTimerStopTimer(prAdapter, &(prP2pRoleFsmInfo->rDfsShutDownTimer));
    }

    /* Check if Channel is DFS Available State. If yes, Change DFS State and
     * Start RDD */
    if ((prP2pConnReqInfo->u4ChnlDfsState == NL80211_DFS_AVAILABLE) &&
        (p2pFuncGetDfsState() != DFS_STATE_ACTIVE)) {
#ifndef CFG_SUPPORT_DISABLE_SAP_DFS_RDD
        p2pFuncSetDfsState(DFS_STATE_ACTIVE);
        p2pFuncStartRdd(prAdapter, prP2pRoleFsmInfo->ucBssIndex);
#endif
    }
#endif

#if (CFG_HW_WMM_BY_BSS == 1)
    if (prP2pBssInfo->fgIsWmmInited == false) {
        prP2pBssInfo->ucWmmQueSet =
            cnmWmmIndexDecision(prAdapter, prP2pBssInfo);
    }
    prP2pBssInfo->eBand = prP2pConnReqInfo->rChannelInfo.eBand;
#endif
#if CFG_SUPPORT_DBDC
    cnmDbdcEnableDecision(prAdapter, prP2pBssInfo->ucBssIndex,
                          prP2pConnReqInfo->rChannelInfo.eBand);
    cnmGetDbdcCapability(prAdapter, prP2pBssInfo->ucBssIndex,
                         prP2pConnReqInfo->rChannelInfo.eBand,
                         prP2pConnReqInfo->rChannelInfo.ucChannelNum,
                         wlanGetSupportNss(prAdapter, prP2pBssInfo->ucBssIndex),
                         &rDbdcCap);

    DBGLOG(P2P, STATE, "p2pRoleFsmRunEventStartAP: start AP at CH %u NSS=%u.\n",
           prP2pConnReqInfo->rChannelInfo.ucChannelNum, rDbdcCap.ucNss);

    prP2pBssInfo->eDBDCBand = ENUM_BAND_AUTO;
    prP2pBssInfo->ucNss = rDbdcCap.ucNss;
#if (CFG_HW_WMM_BY_BSS == 0)
    prP2pBssInfo->ucWmmQueSet = rDbdcCap.ucWmmSetIndex;
#endif
#endif
    prP2pBssInfo->eHiddenSsidType = prP2pStartAPMsg->ucHiddenSsidType;

    /*
     * beacon content is related with Nss number ,
     * need to update because of modification
     */
    bssUpdateBeaconContent(prAdapter, prP2pBssInfo->ucBssIndex);

    if ((prP2pBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) ||
        (prP2pBssInfo->eIntendOPMode != OP_MODE_NUM)) {
        /* 1. No switch to AP mode.
         * 2. Not started yet.
         */

        if (prP2pRoleFsmInfo->eCurrentState !=
            P2P_ROLE_STATE_AP_CHNL_DETECTION &&
            prP2pRoleFsmInfo->eCurrentState != P2P_ROLE_STATE_IDLE) {
            /* Make sure the state is in IDLE state. */
            p2pRoleFsmRunEventAbort(prAdapter, prP2pRoleFsmInfo);
        } else if (prP2pRoleFsmInfo->eCurrentState ==
                   P2P_ROLE_STATE_AP_CHNL_DETECTION) {
            goto error;
        }

        /* Leave IDLE state. */
        SET_NET_PWR_STATE_ACTIVE(prAdapter, prP2pBssInfo->ucBssIndex);
        prP2pBssInfo->eIntendOPMode = OP_MODE_ACCESS_POINT;

        if (prP2pRoleFsmInfo->rConnReqInfo.rChannelInfo.ucChannelNum != 0) {
            DBGLOG(P2P, INFO, "Role(%d) StartAP at CH(%d)\n",
                   prP2pStartAPMsg->ucRoleIdx,
                   prP2pRoleFsmInfo->rConnReqInfo.rChannelInfo.ucChannelNum);

            p2pRoleStatePrepare_To_REQING_CHANNEL_STATE(
                prAdapter,
                GET_BSS_INFO_BY_INDEX(prAdapter, prP2pRoleFsmInfo->ucBssIndex),
                &(prP2pRoleFsmInfo->rConnReqInfo),
                &(prP2pRoleFsmInfo->rChnlReqInfo));
            p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                      P2P_ROLE_STATE_REQING_CHANNEL);
        } else {
            DBGLOG(P2P, INFO, "Role(%d) StartAP Scan for working channel\n",
                   prP2pStartAPMsg->ucRoleIdx);

            /* For AP/GO mode with specific channel or non-specific
             * channel. */
            p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                      P2P_ROLE_STATE_AP_CHNL_DETECTION);
        }
    }

error:
    cnmMemFree(prAdapter, prMsgHdr);
}

void p2pRoleFsmRunEventDelIface(IN P_ADAPTER_T prAdapter,
                                IN P_MSG_HDR_T prMsgHdr)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_MSG_P2P_DEL_IFACE_T prP2pDelIfaceMsg = (P_MSG_P2P_DEL_IFACE_T)NULL;
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    u8 ucRoleIdx;
    P_GL_P2P_INFO_T prP2pInfo = (P_GL_P2P_INFO_T)NULL;

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventDelIface\n");

    prGlueInfo = prAdapter->prGlueInfo;
    if (prGlueInfo == NULL) {
        DBGLOG(P2P, ERROR, "prGlueInfo shouldn't be NULL!\n");
        goto error;
    }

    prP2pDelIfaceMsg = (P_MSG_P2P_DEL_IFACE_T)prMsgHdr;
    ucRoleIdx = prP2pDelIfaceMsg->ucRoleIdx;
    prAdapter = prGlueInfo->prAdapter;
    prP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];
    prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx);
    if (!prP2pRoleFsmInfo) {
        DBGLOG(
            P2P, ERROR,
            "p2pRoleFsmRunEventDelIface: Corresponding P2P Role FSM empty: %d.\n",
            prP2pDelIfaceMsg->ucRoleIdx);
        goto error;
    }

    prP2pBssInfo = prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex];

    /* The state is in disconnecting and can not change any BSS status */
    if (IS_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex) &&
        IS_NET_ACTIVE(prAdapter, prP2pBssInfo->ucBssIndex)) {
        DBGLOG(P2P, TRACE, "under deauth procedure, Quit.\n");
    } else {
        /*p2pFuncDissolve(prAdapter, prP2pBssInfo, true,
         * REASON_CODE_DEAUTH_LEAVING_BSS);*/

        SET_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex);

        /* Function Dissolve should already enter IDLE state. */
        p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                  P2P_ROLE_STATE_IDLE);

        p2pRoleFsmRunEventAbort(prAdapter, prP2pRoleFsmInfo);

        /* Clear CmdQue */
        kalClearMgmtFramesByBssIdx(prAdapter->prGlueInfo,
                                   prP2pBssInfo->ucBssIndex);
        kalClearSecurityFramesByBssIdx(prAdapter->prGlueInfo,
                                       prP2pBssInfo->ucBssIndex);
        /* Clear PendingTxMsdu */
        nicFreePendingTxMsduInfoByBssIdx(prAdapter, prP2pBssInfo->ucBssIndex);

        /* Deactivate BSS. */
        UNSET_NET_ACTIVE(prAdapter, prP2pRoleFsmInfo->ucBssIndex);

        nicDeactivateNetwork(prAdapter, prP2pRoleFsmInfo->ucBssIndex);
        nicUpdateBss(prAdapter, prP2pRoleFsmInfo->ucBssIndex);
    }

error:
    cnmMemFree(prAdapter, prMsgHdr);
}

void p2pRoleFsmRunEventStopAP(IN P_ADAPTER_T prAdapter,
                              IN P_MSG_HDR_T prMsgHdr)
{
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_MSG_P2P_SWITCH_OP_MODE_T prP2pSwitchMode =
        (P_MSG_P2P_SWITCH_OP_MODE_T)NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_STA_RECORD_T prCurrStaRec;
    P_LINK_T prClientList;

    prP2pSwitchMode = (P_MSG_P2P_SWITCH_OP_MODE_T)prMsgHdr;

    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prP2pSwitchMode->ucRoleIdx);

    if (!prP2pRoleFsmInfo) {
        DBGLOG(
            P2P, ERROR,
            "p2pRoleFsmRunEventStopAP: Corresponding P2P Role FSM empty: %d.\n",
            prP2pSwitchMode->ucRoleIdx);
        goto error;
    }

    prP2pBssInfo = prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex];

    if (!prP2pBssInfo) {
        DBGLOG(P2P, ERROR,
               "prP2pBssInfo of prP2pRoleFsmInfo->ucBssIndex %d is NULL!\n",
               prP2pRoleFsmInfo->ucBssIndex);
        goto error;
    }

#if (CFG_SUPPORT_DFS_MASTER == 1)
    p2pFuncSetDfsState(DFS_STATE_INACTIVE);
    p2pFuncStopRdd(prAdapter, prP2pBssInfo->ucBssIndex);
#endif

    if (prP2pRoleFsmInfo->eCurrentState != P2P_ROLE_STATE_REQING_CHANNEL) {
        p2pFuncStopGO(prAdapter, prP2pBssInfo);

        /* Start all Deauth done timer for all client */
        prClientList = &prP2pBssInfo->rStaRecOfClientList;

        LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
                            STA_RECORD_T){
            ASSERT(prCurrStaRec);
            /* Do not restart timer if the timer is pending, */
            /* (start in p2pRoleFsmRunEventConnectionAbort()) */
            if (!timerPendingTimer(&(prCurrStaRec->rDeauthTxDoneTimer))) {
                cnmTimerInitTimer(
                    prAdapter, &(prCurrStaRec->rDeauthTxDoneTimer),
                    (PFN_MGMT_TIMEOUT_FUNC)p2pRoleFsmDeauthTimeout,
                    (unsigned long)prCurrStaRec);

                cnmTimerStartTimer(prAdapter,
                                   &(prCurrStaRec->rDeauthTxDoneTimer),
                                   P2P_DEAUTH_TIMEOUT_TIME_MS);
            }
        }
    }

    SET_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex);

    p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo, P2P_ROLE_STATE_IDLE);

error:
    cnmMemFree(prAdapter, prMsgHdr);
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
#if CFG_SUPPORT_DBDC_TC6
void p2pRoleFsmRunEventDfsCac(IN P_ADAPTER_T prAdapter,
                              IN P_MSG_HDR_T prMsgHdr)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_MSG_P2P_DFS_CAC_T prP2pDfsCacMsg = (P_MSG_P2P_DFS_CAC_T)NULL;
    P_P2P_CONNECTION_REQ_INFO_T prP2pConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
#if CFG_SUPPORT_DBDC
    CNM_DBDC_CAP_T rDbdcCap;
#endif

    DBGLOG(P2P, STATE, "p2pRoleFsmRunEventDfsCac\n");

    prP2pDfsCacMsg = (P_MSG_P2P_DFS_CAC_T)prMsgHdr;

    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prP2pDfsCacMsg->ucRoleIdx);

    prP2pRoleFsmInfo->rChannelWidth = prP2pDfsCacMsg->eChannelWidth;

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventDfsCac with Role(%d)\n",
           prP2pDfsCacMsg->ucRoleIdx);

    if (!prP2pRoleFsmInfo) {
        DBGLOG(
            P2P, ERROR,
            "p2pRoleFsmRunEventDfsCac: Corresponding P2P Role FSM empty: %d.\n",
            prP2pDfsCacMsg->ucRoleIdx);
        goto error;
    }

    if (timerPendingTimer(&(prP2pRoleFsmInfo->rDfsShutDownTimer))) {
        DBGLOG(P2P, INFO,
               "p2pRoleFsmRunEventDfsCac: Stop DFS shut down timer.\n");
        cnmTimerStopTimer(prAdapter, &(prP2pRoleFsmInfo->rDfsShutDownTimer));
    }

    prP2pBssInfo = prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex];

    prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);

    if (p2pFuncIsAPMode(
            prAdapter->rWifiVar.prP2PConnSettings[prP2pDfsCacMsg->ucRoleIdx])) {
        prP2pConnReqInfo->eConnRequest = P2P_CONNECTION_TYPE_PURE_AP;
    } else {
        prP2pConnReqInfo->eConnRequest = P2P_CONNECTION_TYPE_GO;
    }

#if (CFG_HW_WMM_BY_BSS == 1)
    if (prP2pBssInfo->fgIsWmmInited == false) {
        prP2pBssInfo->ucWmmQueSet =
            cnmWmmIndexDecision(prAdapter, prP2pBssInfo);
    }
    prP2pBssInfo->eBand = prP2pConnReqInfo->rChannelInfo.eBand;
#endif
#if CFG_SUPPORT_DBDC
    cnmDbdcEnableDecision(prAdapter, prP2pBssInfo->ucBssIndex,
                          prP2pConnReqInfo->rChannelInfo.eBand);
    cnmGetDbdcCapability(prAdapter, prP2pBssInfo->ucBssIndex,
                         prP2pConnReqInfo->rChannelInfo.eBand,
                         prP2pConnReqInfo->rChannelInfo.ucChannelNum,
                         prAdapter->rWifiVar.ucNSS, &rDbdcCap);

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventDfsCac: Set channel at CH %u.\n",
           prP2pConnReqInfo->rChannelInfo.ucChannelNum);

    prP2pBssInfo->eDBDCBand = ENUM_BAND_AUTO;
    prP2pBssInfo->ucNss = rDbdcCap.ucNss;
#if (CFG_HW_WMM_BY_BSS == 0)
    prP2pBssInfo->ucWmmQueSet = rDbdcCap.ucWmmSetIndex;
#endif

    if (prAdapter->rWifiVar.fgDbDcModeEn == true) {
        DBGLOG(P2P, INFO, "Delay DFS CAC for DBDC Switch: %d sec\n",
               P2P_DFS_CAC_DELAY_TIME_MS / 1000);
        cnmTimerStartTimer(prAdapter, &(prP2pRoleFsmInfo->rDfsStartCacTimer),
                           P2P_DFS_CAC_DELAY_TIME_MS);
    } else {
        DBGLOG(P2P, INFO, "Start DFS CAC immediately\n");
        cnmTimerStartTimer(prAdapter, &(prP2pRoleFsmInfo->rDfsStartCacTimer),
                           0);
    }
#endif

error:
    cnmMemFree(prAdapter, prMsgHdr);
}

void p2pRoleFsmRunEventStartDfsCacTimeout(IN P_ADAPTER_T prAdapter,
                                          IN unsigned long ulParamPtr)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)ulParamPtr;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventStartDfsCacTimeout\n");

    if (timerPendingTimer(&prAdapter->rWifiVar.rDBDCReconnectCountDown)) {
        DBGLOG(CNM, INFO, "DBDC reconnect timer protection still on\n");
        DBGLOG(P2P, INFO, "Extend DFS CAC Delay for another %d sec\n",
               P2P_DFS_CAC_DELAY_TIME_MS / 1000);
        cnmTimerStartTimer(prAdapter, &(prP2pRoleFsmInfo->rDfsStartCacTimer),
                           P2P_DFS_CAC_DELAY_TIME_MS);
        return;
    }

    if (prP2pRoleFsmInfo->eCurrentState != P2P_ROLE_STATE_IDLE) {
        /* Make sure the state is in IDLE state. */
        p2pRoleFsmRunEventAbort(prAdapter, prP2pRoleFsmInfo);
    }

    prP2pBssInfo = prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex];

    /* Leave IDLE state. */
    SET_NET_PWR_STATE_ACTIVE(prAdapter, prP2pBssInfo->ucBssIndex);

    prP2pBssInfo->eIntendOPMode = OP_MODE_ACCESS_POINT;
    prP2pBssInfo->fgIsDfsActive = true;

    if (prP2pRoleFsmInfo->rConnReqInfo.rChannelInfo.ucChannelNum != 0) {
        DBGLOG(P2P, INFO, "Role(%d) Set channel at CH(%d)\n",
               prP2pRoleFsmInfo->ucRoleIndex,
               prP2pRoleFsmInfo->rConnReqInfo.rChannelInfo.ucChannelNum);

        p2pRoleStatePrepare_To_DFS_CAC_STATE(
            prAdapter,
            GET_BSS_INFO_BY_INDEX(prAdapter, prP2pRoleFsmInfo->ucBssIndex),
            prP2pRoleFsmInfo->rChannelWidth, &(prP2pRoleFsmInfo->rConnReqInfo),
            &(prP2pRoleFsmInfo->rChnlReqInfo));
        p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                  P2P_ROLE_STATE_DFS_CAC);
    } else {
        DBGLOG(P2P, ERROR,
               "prP2pRoleFsmInfo->rConnReqInfo.rChannelInfo.ucChannelNum %d "
               "shouldn't be 0\n");
    }
}  /*p2pRoleFsmRunEventDfsCac*/

#else

void p2pRoleFsmRunEventDfsCac(IN P_ADAPTER_T prAdapter,
                              IN P_MSG_HDR_T prMsgHdr)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_MSG_P2P_DFS_CAC_T prP2pDfsCacMsg = (P_MSG_P2P_DFS_CAC_T)NULL;
    P_P2P_CONNECTION_REQ_INFO_T prP2pConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    ENUM_CHANNEL_WIDTH_T rChannelWidth;
#if CFG_SUPPORT_DBDC
    CNM_DBDC_CAP_T rDbdcCap;
#endif

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventDfsCac\n");

    prP2pDfsCacMsg = (P_MSG_P2P_DFS_CAC_T)prMsgHdr;

    rChannelWidth = prP2pDfsCacMsg->eChannelWidth;

    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prP2pDfsCacMsg->ucRoleIdx);

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventDfsCac with Role(%d)\n",
           prP2pDfsCacMsg->ucRoleIdx);

    if (!prP2pRoleFsmInfo) {
        DBGLOG(
            P2P, ERROR,
            "p2pRoleFsmRunEventDfsCac: Corresponding P2P Role FSM empty: %d.\n",
            prP2pDfsCacMsg->ucRoleIdx);
        goto error;
    }

    if (timerPendingTimer(&(prP2pRoleFsmInfo->rDfsShutDownTimer))) {
        DBGLOG(P2P, INFO,
               "p2pRoleFsmRunEventDfsCac: Stop DFS shut down timer.\n");
        cnmTimerStopTimer(prAdapter, &(prP2pRoleFsmInfo->rDfsShutDownTimer));
    }

    prP2pBssInfo = prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex];

    prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);

    if (p2pFuncIsAPMode(
            prAdapter->rWifiVar.prP2PConnSettings[prP2pDfsCacMsg->ucRoleIdx])) {
        prP2pConnReqInfo->eConnRequest = P2P_CONNECTION_TYPE_PURE_AP;
    } else {
        prP2pConnReqInfo->eConnRequest = P2P_CONNECTION_TYPE_GO;
    }

#if (CFG_HW_WMM_BY_BSS == 1)
    if (prP2pBssInfo->fgIsWmmInited == false) {
        prP2pBssInfo->ucWmmQueSet =
            cnmWmmIndexDecision(prAdapter, prP2pBssInfo);
    }
    prP2pBssInfo->eBand = prP2pConnReqInfo->rChannelInfo.eBand;
#endif
#if CFG_SUPPORT_DBDC
    cnmDbdcEnableDecision(prAdapter, prP2pBssInfo->ucBssIndex,
                          prP2pConnReqInfo->rChannelInfo.eBand);
    cnmGetDbdcCapability(prAdapter, prP2pBssInfo->ucBssIndex,
                         prP2pConnReqInfo->rChannelInfo.eBand,
                         prP2pConnReqInfo->rChannelInfo.ucChannelNum,
                         prAdapter->rWifiVar.ucNSS, &rDbdcCap);

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventDfsCac: Set channel at CH %u.\n",
           prP2pConnReqInfo->rChannelInfo.ucChannelNum);

    prP2pBssInfo->eDBDCBand = ENUM_BAND_AUTO;
    prP2pBssInfo->ucNss = rDbdcCap.ucNss;
#if (CFG_HW_WMM_BY_BSS == 0)
    prP2pBssInfo->ucWmmQueSet = rDbdcCap.ucWmmSetIndex;
#endif
#endif

    if (prP2pRoleFsmInfo->eCurrentState != P2P_ROLE_STATE_IDLE) {
        /* Make sure the state is in IDLE state. */
        p2pRoleFsmRunEventAbort(prAdapter, prP2pRoleFsmInfo);
    }

    /* Leave IDLE state. */
    SET_NET_PWR_STATE_ACTIVE(prAdapter, prP2pBssInfo->ucBssIndex);

    prP2pBssInfo->eIntendOPMode = OP_MODE_ACCESS_POINT;
    prP2pBssInfo->fgIsDfsActive = true;

    if (prP2pRoleFsmInfo->rConnReqInfo.rChannelInfo.ucChannelNum != 0) {
        DBGLOG(P2P, INFO, "Role(%d) Set channel at CH(%d)\n",
               prP2pDfsCacMsg->ucRoleIdx,
               prP2pRoleFsmInfo->rConnReqInfo.rChannelInfo.ucChannelNum);

        p2pRoleStatePrepare_To_DFS_CAC_STATE(
            prAdapter,
            GET_BSS_INFO_BY_INDEX(prAdapter, prP2pRoleFsmInfo->ucBssIndex),
            rChannelWidth, &(prP2pRoleFsmInfo->rConnReqInfo),
            &(prP2pRoleFsmInfo->rChnlReqInfo));
        p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                  P2P_ROLE_STATE_DFS_CAC);
    } else {
        DBGLOG(P2P, ERROR,
               "prP2pRoleFsmInfo->rConnReqInfo.rChannelInfo.ucChannelNum %d "
               "shouldn't be 0\n");
    }

error:
    cnmMemFree(prAdapter, prMsgHdr);
}  /*p2pRoleFsmRunEventDfsCac*/

#endif

void p2pRoleFsmRunEventRadarDet(IN P_ADAPTER_T prAdapter,
                                IN P_MSG_HDR_T prMsgHdr)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_MSG_P2P_RADAR_DETECT_T prMsgP2pRddDetMsg;

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventRadarDet\n");

    prMsgP2pRddDetMsg = (P_MSG_P2P_RADAR_DETECT_T)prMsgHdr;

    prP2pBssInfo =
        GET_BSS_INFO_BY_INDEX(prAdapter, prMsgP2pRddDetMsg->ucBssIndex);

    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prP2pBssInfo->u4PrivateData);

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventRadarDet with Role(%d)\n",
           prP2pRoleFsmInfo->ucRoleIndex);

    if (prP2pRoleFsmInfo->eCurrentState != P2P_ROLE_STATE_DFS_CAC &&
        prP2pRoleFsmInfo->eCurrentState != P2P_ROLE_STATE_IDLE) {
        DBGLOG(P2P, ERROR, "Wrong prP2pRoleFsmInfo->eCurrentState \"%s\"!",
               (prP2pRoleFsmInfo->eCurrentState < P2P_ROLE_STATE_NUM ?
                (const char *)
                apucDebugP2pRoleState[prP2pRoleFsmInfo->eCurrentState] :
                ""));
        goto error;
    }

    if (p2pFuncGetRadarDetectMode()) {
        DBGLOG(P2P, INFO, "p2pRoleFsmRunEventRadarDet: Ignore radar event\n");
        if (prP2pRoleFsmInfo->eCurrentState == P2P_ROLE_STATE_DFS_CAC) {
            p2pFuncSetDfsState(DFS_STATE_CHECKING);
        } else {
            p2pFuncSetDfsState(DFS_STATE_ACTIVE);
        }
    } else {
        if (prP2pRoleFsmInfo->eCurrentState == P2P_ROLE_STATE_DFS_CAC) {
            p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                      P2P_ROLE_STATE_IDLE);
        }

        kalP2PRddDetectUpdate(prAdapter->prGlueInfo,
                              prP2pRoleFsmInfo->ucRoleIndex);
        cnmTimerStartTimer(prAdapter, &(prP2pRoleFsmInfo->rDfsShutDownTimer),
                           5000);
    }

    p2pFuncShowRadarInfo(prAdapter, prMsgP2pRddDetMsg->ucBssIndex);

error:
    cnmMemFree(prAdapter, prMsgHdr);
}  /*p2pRoleFsmRunEventRadarDet*/

void p2pRoleFsmRunEventSetNewChannel(IN P_ADAPTER_T prAdapter,
                                     IN P_MSG_HDR_T prMsgHdr)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_MSG_P2P_SET_NEW_CHANNEL_T prMsgP2pSetNewChannelMsg;

    DBGLOG(P2P, STATE, "p2pRoleFsmRunEventSetNewChannel\n");

    prMsgP2pSetNewChannelMsg = (P_MSG_P2P_SET_NEW_CHANNEL_T)prMsgHdr;
    DBGLOG(P2P, STATE, "ucBssIndex:%d\n", prMsgP2pSetNewChannelMsg->ucBssIndex);

    prP2pBssInfo =
        GET_BSS_INFO_BY_INDEX(prAdapter, prMsgP2pSetNewChannelMsg->ucBssIndex);

    prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
        prAdapter, prMsgP2pSetNewChannelMsg->ucRoleIdx);

    prP2pRoleFsmInfo->rChnlReqInfo.ucReqChnlNum =
        prP2pRoleFsmInfo->rConnReqInfo.rChannelInfo.ucChannelNum;
    prP2pRoleFsmInfo->rChnlReqInfo.eBand =
        prP2pRoleFsmInfo->rConnReqInfo.rChannelInfo.eBand;
    prP2pRoleFsmInfo->rChnlReqInfo.eChannelWidth =
        prMsgP2pSetNewChannelMsg->eChannelWidth;
    prP2pBssInfo->ucPrimaryChannel =
        prP2pRoleFsmInfo->rConnReqInfo.rChannelInfo.ucChannelNum;

    prP2pRoleFsmInfo->rChnlReqInfo.ucCenterFreqS1 =
        nicGetVhtS1(prP2pBssInfo->ucPrimaryChannel,
                    prP2pRoleFsmInfo->rChnlReqInfo.eChannelWidth);
    prP2pRoleFsmInfo->rChnlReqInfo.ucCenterFreqS2 = 0;

    cnmMemFree(prAdapter, prMsgHdr);
}  /*p2pRoleFsmRunEventCsaDone*/

void p2pRoleFsmRunEventCsaDone(IN P_ADAPTER_T prAdapter,
                               IN P_MSG_HDR_T prMsgHdr)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_MSG_P2P_CSA_DONE_T prMsgP2pCsaDoneMsg;

    DBGLOG(P2P, TRACE, "p2pRoleFsmRunEventCsaDone\n");

    prMsgP2pCsaDoneMsg = (P_MSG_P2P_CSA_DONE_T)prMsgHdr;

    prP2pBssInfo =
        GET_BSS_INFO_BY_INDEX(prAdapter, prMsgP2pCsaDoneMsg->ucBssIndex);

    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prP2pBssInfo->u4PrivateData);

    p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                              P2P_ROLE_STATE_SWITCH_CHANNEL);

    cnmMemFree(prAdapter, prMsgHdr);
}  /*p2pRoleFsmRunEventCsaDone*/

void p2pRoleFsmRunEventDfsShutDownTimeout(IN P_ADAPTER_T prAdapter,
                                          IN unsigned long ulParamPtr)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)ulParamPtr;

    DBGLOG(P2P, INFO, "p2pRoleFsmRunEventDfsShutDownTimeout: DFS shut down.\n");

    p2pFuncSetDfsState(DFS_STATE_INACTIVE);
    p2pFuncStopRdd(prAdapter, prP2pRoleFsmInfo->ucBssIndex);
}

#endif

void p2pRoleFsmScanTargetBss(IN P_ADAPTER_T prAdapter,
                             IN P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo,
                             IN u8 ucChannelNum, IN ENUM_BAND_T eBand,
                             IN P_P2P_SSID_STRUCT_T prSsid)
{
    /* Update scan parameter... to scan target device. */
    P_P2P_SCAN_REQ_INFO_T prScanReqInfo = &(prP2pRoleFsmInfo->rScanReqInfo);

    prScanReqInfo->ucNumChannelList = 1;
    prScanReqInfo->eScanType = SCAN_TYPE_ACTIVE_SCAN;
    prScanReqInfo->eChannelSet = SCAN_CHANNEL_SPECIFIED;
    prScanReqInfo->arScanChannelList[0].ucChannelNum = ucChannelNum;
    prScanReqInfo->arScanChannelList[0].eBand = eBand;
    prScanReqInfo->ucSsidNum = 1;
    kalMemCopy(&(prScanReqInfo->arSsidStruct[0]), prSsid,
               sizeof(P2P_SSID_STRUCT_T));
    prScanReqInfo->u4BufLength = 0;  /* Prevent other P2P ID in IE. */
    prScanReqInfo->fgIsAbort = true;

    p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo, P2P_ROLE_STATE_SCAN);
}

void p2pRoleFsmRunEventConnectionRequest(IN P_ADAPTER_T prAdapter,
                                         IN P_MSG_HDR_T prMsgHdr)
{
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_MSG_P2P_CONNECTION_REQUEST_T prP2pConnReqMsg =
        (P_MSG_P2P_CONNECTION_REQUEST_T)NULL;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;

    P_P2P_CONNECTION_REQ_INFO_T prConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;
    P_P2P_CHNL_REQ_INFO_T prChnlReqInfo = (P_P2P_CHNL_REQ_INFO_T)NULL;
    P_P2P_JOIN_INFO_T prJoinInfo = (P_P2P_JOIN_INFO_T)NULL;
#if CFG_SUPPORT_DBDC
    CNM_DBDC_CAP_T rDbdcCap;
#endif

    prP2pConnReqMsg = (P_MSG_P2P_CONNECTION_REQUEST_T)prMsgHdr;

    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prP2pConnReqMsg->ucRoleIdx);

    if (!prP2pRoleFsmInfo) {
        DBGLOG(P2P, ERROR, "Corresponding P2P Role FSM empty: %d.\n",
               prP2pConnReqMsg->ucRoleIdx);
        goto error;
    }

    prP2pBssInfo = prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex];

    if (!prP2pBssInfo) {
        DBGLOG(
            P2P, ERROR,
            "prP2pRoleFsmInfo->ucBssIndex %d of prAdapter->aprBssInfo is NULL!\n",
            prP2pRoleFsmInfo->ucBssIndex);
        goto error;
    }

    prConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
    prChnlReqInfo = &(prP2pRoleFsmInfo->rChnlReqInfo);
    prJoinInfo = &(prP2pRoleFsmInfo->rJoinInfo);

    DBGLOG(P2P, TRACE, "p2pFsmRunEventConnectionRequest\n");

    if (prP2pBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE) {
        goto error;
    }

    /* In case the network is already activated, we need to re-activate
     * the network. Otherwise, the connection may be failed in dbdc cases.
     */
    if (IS_NET_ACTIVE(prAdapter, prP2pBssInfo->ucBssIndex)) {
        UNSET_NET_ACTIVE(prAdapter, prP2pBssInfo->ucBssIndex);
        nicDeactivateNetwork(prAdapter, prP2pBssInfo->ucBssIndex);
    }

    SET_NET_PWR_STATE_ACTIVE(prAdapter, prP2pBssInfo->ucBssIndex);

    /* In P2P GC case, the interval of two ASSOC flow could be very short,
     */
    /* we must start to connect directly before Deauth done */
    prStaRec = prP2pBssInfo->prStaRecOfAP;
    if (prStaRec) {
        if (timerPendingTimer(&prStaRec->rDeauthTxDoneTimer)) {
            cnmTimerStopTimer(prAdapter, &(prStaRec->rDeauthTxDoneTimer));
            p2pRoleFsmDeauthComplete(prAdapter, prStaRec);  /* Force
                                                             * to
                                                             * stop
                                                             */
        }
    }
    /* Make sure the state is in IDLE state. */
    if (prP2pRoleFsmInfo->eCurrentState != P2P_ROLE_STATE_IDLE) {
        p2pRoleFsmRunEventAbort(prAdapter, prP2pRoleFsmInfo);
    }
    /* Update connection request information. */
    prConnReqInfo->eConnRequest = P2P_CONNECTION_TYPE_GC;
    COPY_MAC_ADDR(prConnReqInfo->aucBssid, prP2pConnReqMsg->aucBssid);
    COPY_MAC_ADDR(prP2pBssInfo->aucOwnMacAddr, prP2pConnReqMsg->aucSrcMacAddr);
    kalMemCopy(&(prConnReqInfo->rSsidStruct), &(prP2pConnReqMsg->rSsid),
               sizeof(P2P_SSID_STRUCT_T));
    kalMemCopy(prConnReqInfo->aucIEBuf, prP2pConnReqMsg->aucIEBuf,
               prP2pConnReqMsg->u4IELen);
    prConnReqInfo->u4BufLength = prP2pConnReqMsg->u4IELen;

    /* Find BSS Descriptor first. */
    prJoinInfo->prTargetBssDesc = scanP2pSearchDesc(prAdapter, prConnReqInfo);

    if (prJoinInfo->prTargetBssDesc == NULL) {
        p2pRoleFsmScanTargetBss(prAdapter, prP2pRoleFsmInfo,
                                prP2pConnReqMsg->rChannelInfo.ucChannelNum,
                                prP2pConnReqMsg->rChannelInfo.eBand,
                                &(prP2pConnReqMsg->rSsid));
    } else {
        prChnlReqInfo->u8Cookie = 0;
        prChnlReqInfo->ucReqChnlNum =
            prP2pConnReqMsg->rChannelInfo.ucChannelNum;
        prChnlReqInfo->eBand = prP2pConnReqMsg->rChannelInfo.eBand;
        prChnlReqInfo->eChnlSco = prP2pConnReqMsg->eChnlSco;
        prChnlReqInfo->u4MaxInterval = AIS_JOIN_CH_REQUEST_INTERVAL;
        prChnlReqInfo->eChnlReqType = CH_REQ_TYPE_JOIN;

        prChnlReqInfo->eChannelWidth =
            prJoinInfo->prTargetBssDesc->eChannelWidth;
        prChnlReqInfo->ucCenterFreqS1 =
            prJoinInfo->prTargetBssDesc->ucCenterFreqS1;
        prChnlReqInfo->ucCenterFreqS2 =
            prJoinInfo->prTargetBssDesc->ucCenterFreqS2;

        rlmReviseMaxBw(prAdapter, prP2pBssInfo->ucBssIndex,
                       &prChnlReqInfo->eChnlSco,
                       (u8 *)&prChnlReqInfo->eChannelWidth,
                       &prChnlReqInfo->ucCenterFreqS1,
                       &prChnlReqInfo->ucReqChnlNum);

#if (CFG_HW_WMM_BY_BSS == 1)
        if (prP2pBssInfo->fgIsWmmInited == false) {
            prP2pBssInfo->ucWmmQueSet =
                cnmWmmIndexDecision(prAdapter, prP2pBssInfo);
        }
        prP2pBssInfo->eBand = prChnlReqInfo->eBand;
#endif
#if CFG_SUPPORT_DBDC
        cnmDbdcEnableDecision(prAdapter, prP2pBssInfo->ucBssIndex,
                              prChnlReqInfo->eBand);
        cnmGetDbdcCapability(
            prAdapter, prP2pBssInfo->ucBssIndex, prChnlReqInfo->eBand,
            prChnlReqInfo->ucReqChnlNum,
            wlanGetSupportNss(prAdapter, prP2pBssInfo->ucBssIndex), &rDbdcCap);

        DBGLOG(
            P2P, INFO,
            "p2pRoleFsmRunEventConnectionRequest: start GC at CH %u, NSS=%u.\n",
            prChnlReqInfo->ucReqChnlNum, rDbdcCap.ucNss);

        prP2pBssInfo->eDBDCBand = ENUM_BAND_AUTO;
        prP2pBssInfo->ucNss = rDbdcCap.ucNss;
#if (CFG_HW_WMM_BY_BSS == 0)
        prP2pBssInfo->ucWmmQueSet = rDbdcCap.ucWmmSetIndex;
#endif
#endif
        p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                  P2P_ROLE_STATE_REQING_CHANNEL);
    }

error:
    cnmMemFree(prAdapter, prMsgHdr);
}

void p2pRoleFsmRunEventConnectionAbort(IN P_ADAPTER_T prAdapter,
                                       IN P_MSG_HDR_T prMsgHdr)
{
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_MSG_P2P_CONNECTION_ABORT_T prDisconnMsg =
        (P_MSG_P2P_CONNECTION_ABORT_T)NULL;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;

    prDisconnMsg = (P_MSG_P2P_CONNECTION_ABORT_T)prMsgHdr;

    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prDisconnMsg->ucRoleIdx);

    DBGLOG(P2P, TRACE, "p2pFsmRunEventConnectionAbort: Connection Abort.\n");

    if (!prP2pRoleFsmInfo) {
        DBGLOG(P2P, ERROR,
               "p2pRoleFsmRunEventConnectionAbort: Corresponding P2P Role FSM "
               "empty: %d.\n",
               prDisconnMsg->ucRoleIdx);
        goto error;
    }

    prP2pBssInfo = prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex];

    if (!prP2pBssInfo) {
        DBGLOG(
            P2P, ERROR,
            "prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex(%d)] is NULL!",
            prP2pRoleFsmInfo->ucBssIndex);
        goto error;
    }

    switch (prP2pBssInfo->eCurrentOPMode) {
    case OP_MODE_INFRASTRUCTURE: {
        u8 aucBCBSSID[] = BC_BSSID;

        if (!prP2pBssInfo->prStaRecOfAP) {
            DBGLOG(P2P, TRACE, "GO's StaRec is NULL\n");
            break;
        }
        if (UNEQUAL_MAC_ADDR(prP2pBssInfo->prStaRecOfAP->aucMacAddr,
                             prDisconnMsg->aucTargetID) &&
            UNEQUAL_MAC_ADDR(prDisconnMsg->aucTargetID, aucBCBSSID)) {
            DBGLOG(P2P, TRACE, "Unequal MAC ADDR [" MACSTR ":" MACSTR "]\n",
                   MAC2STR(prP2pBssInfo->prStaRecOfAP->aucMacAddr),
                   MAC2STR(prDisconnMsg->aucTargetID));
            break;
        }

        kalP2PGCIndicateConnectionStatus(prAdapter->prGlueInfo,
                                         prP2pRoleFsmInfo->ucRoleIndex, NULL,
                                         NULL, 0, 0,
                                         WLAN_STATUS_MEDIA_DISCONNECT);

        prStaRec = prP2pBssInfo->prStaRecOfAP;

        /* Stop rejoin timer if it is started. */
        /* TODO: If it has. */

        p2pFuncDisconnect(prAdapter, prP2pBssInfo, prStaRec,
                          prDisconnMsg->fgSendDeauth,
                          prDisconnMsg->u2ReasonCode, true);

        cnmTimerStopTimer(prAdapter, &(prStaRec->rDeauthTxDoneTimer));

        cnmTimerInitTimer(prAdapter, &(prStaRec->rDeauthTxDoneTimer),
                          (PFN_MGMT_TIMEOUT_FUNC)p2pRoleFsmDeauthTimeout,
                          (unsigned long)prStaRec);

        cnmTimerStartTimer(prAdapter, &(prStaRec->rDeauthTxDoneTimer),
                           P2P_DEAUTH_TIMEOUT_TIME_MS);

        SET_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex);

        p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                  P2P_ROLE_STATE_IDLE);
    } break;

    case OP_MODE_ACCESS_POINT: {
        /* Search specific client device, and disconnect. */
        /* 1. Send deauthentication frame. */
        /* 2. Indication: Device disconnect. */
        P_STA_RECORD_T prCurrStaRec = (P_STA_RECORD_T)NULL;

        DBGLOG(P2P, TRACE, "Disconnecting with Target ID: " MACSTR "\n",
               MAC2STR(prDisconnMsg->aucTargetID));

        prCurrStaRec = bssGetClientByMac(prAdapter, prP2pBssInfo,
                                         prDisconnMsg->aucTargetID);

        if (prCurrStaRec) {
            DBGLOG(P2P, TRACE, "Disconnecting: " MACSTR "\n",
                   MAC2STR(prCurrStaRec->aucMacAddr));

            if (!prDisconnMsg->fgSendDeauth) {
                p2pRoleFsmDeauthComplete(prAdapter, prCurrStaRec);
                break;
            }

            /* Glue layer indication. */
            /* kalP2PGOStationUpdate(prAdapter->prGlueInfo,
             * prCurrStaRec, false); */

            /* Send deauth & do indication. */
            p2pFuncDisconnect(prAdapter, prP2pBssInfo, prCurrStaRec,
                              prDisconnMsg->fgSendDeauth,
                              prDisconnMsg->u2ReasonCode, true);

            cnmTimerStopTimer(prAdapter, &(prCurrStaRec->rDeauthTxDoneTimer));

            cnmTimerInitTimer(prAdapter, &(prCurrStaRec->rDeauthTxDoneTimer),
                              (PFN_MGMT_TIMEOUT_FUNC)p2pRoleFsmDeauthTimeout,
                              (unsigned long)prCurrStaRec);

            cnmTimerStartTimer(prAdapter, &(prCurrStaRec->rDeauthTxDoneTimer),
                               P2P_DEAUTH_TIMEOUT_TIME_MS);
#if CFG_SUPPORT_802_11W
        } else if (prP2pBssInfo->u4RsnSelectedAKMSuite == RSN_AKM_SUITE_SAE) {
            if (!completion_done(&prP2pBssInfo->rDeauthComp)) {
                DBGLOG(P2P, INFO, "Complete rDeauthComp\n");
                complete(&prP2pBssInfo->rDeauthComp);
            }
            prP2pBssInfo->encryptedDeauthIsInProcess = false;
#endif
        }
    } break;

    case OP_MODE_P2P_DEVICE:
    default:
        ASSERT(false);
        break;
    }

error:
    cnmMemFree(prAdapter, prMsgHdr);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is called when JOIN complete message event is
 * received from SAA.
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void p2pRoleFsmRunEventJoinComplete(IN P_ADAPTER_T prAdapter,
                                    IN P_MSG_HDR_T prMsgHdr)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_P2P_JOIN_INFO_T prJoinInfo = (P_P2P_JOIN_INFO_T)NULL;
    P_MSG_JOIN_COMP_T prJoinCompMsg = (P_MSG_JOIN_COMP_T)NULL;
    P_SW_RFB_T prAssocRspSwRfb = (P_SW_RFB_T)NULL;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;

    prJoinCompMsg = (P_MSG_JOIN_COMP_T)prMsgHdr;
    prStaRec = prJoinCompMsg->prStaRec;
    prAssocRspSwRfb = prJoinCompMsg->prSwRfb;

    DBGLOG(P2P, TRACE, "P2P BSS %d, Join Complete\n", prStaRec->ucBssIndex);

    ASSERT(prStaRec);
    if (!prStaRec) {
        DBGLOG(P2P, ERROR, "prJoinCompMsg->prStaRec is NULL!\n");
        goto error;
    }

    ASSERT(prStaRec->ucBssIndex < P2P_DEV_BSS_INDEX);
    if (!(prStaRec->ucBssIndex < P2P_DEV_BSS_INDEX)) {
        DBGLOG(P2P, ERROR,
               "prStaRec->ucBssIndex % should < P2P_DEV_BSS_INDEX(%d)!\n",
               prStaRec->ucBssIndex, P2P_DEV_BSS_INDEX);
        goto error;
    }

    prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

    ASSERT(prP2pBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE);
    if (prP2pBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE) {
        DBGLOG(
            P2P, ERROR,
            "prP2pBssInfo->eCurrentOPMode %d != OP_MODE_INFRASTRUCTURE(%d)!\n",
            prP2pBssInfo->eCurrentOPMode, OP_MODE_INFRASTRUCTURE);
        goto error;
    }

    ASSERT(prP2pBssInfo->u4PrivateData < BSS_P2P_NUM);
    if (!(prP2pBssInfo->u4PrivateData < BSS_P2P_NUM)) {
        DBGLOG(P2P, ERROR,
               "prP2pBssInfo->u4PrivateData %s should < BSS_P2P_NUM(%d)!\n",
               prP2pBssInfo->u4PrivateData, BSS_P2P_NUM);
        goto error;
    }

    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prP2pBssInfo->u4PrivateData);

    prJoinInfo = &(prP2pRoleFsmInfo->rJoinInfo);

    if (!prJoinInfo->prTargetStaRec) {
        DBGLOG(P2P, ERROR, "prJoinInfo->prTargetStaRec is NULL!\n");
        goto error;
    }

    /* Check SEQ NUM */
    if (prJoinCompMsg->ucSeqNum == prJoinInfo->ucSeqNumOfReqMsg) {
        ASSERT(prStaRec == prJoinInfo->prTargetStaRec);
        prJoinInfo->fgIsJoinComplete = true;

        if (prJoinCompMsg->rJoinStatus == WLAN_STATUS_SUCCESS) {
            /* 4 <1.1> Change FW's Media State immediately. */
            p2pChangeMediaState(prAdapter, prP2pBssInfo,
                                PARAM_MEDIA_STATE_CONNECTED);

            /* 4 <1.2> Deactivate previous AP's STA_RECORD_T in
             * Driver if have. */
            if ((prP2pBssInfo->prStaRecOfAP) &&
                (prP2pBssInfo->prStaRecOfAP != prStaRec)) {
                cnmStaRecChangeState(prAdapter, prP2pBssInfo->prStaRecOfAP,
                                     STA_STATE_1);

                cnmStaRecFree(prAdapter, prP2pBssInfo->prStaRecOfAP);

                prP2pBssInfo->prStaRecOfAP = NULL;
            }
            /* 4 <1.3> Update BSS_INFO_T */
            if (prAssocRspSwRfb) {
                p2pFuncUpdateBssInfoForJOIN(prAdapter,
                                            prJoinInfo->prTargetBssDesc,
                                            prStaRec, prP2pBssInfo,
                                            prAssocRspSwRfb);
            } else {
                DBGLOG(
                    P2P, INFO,
                    "prAssocRspSwRfb is NULL! Skip p2pFuncUpdateBssInfoForJOIN\n");
            }

            /* 4 <1.4> Activate current AP's STA_RECORD_T in Driver.
             */
            cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);

#if CFG_SUPPORT_P2P_RSSI_QUERY
            /* <1.5> Update RSSI if necessary */
            nicUpdateRSSI(prAdapter, prP2pBssInfo->ucBssIndex,
                          (s8)(RCPI_TO_dBm(prStaRec->ucRCPI)), 0);
#endif

            /* 4 <1.6> Indicate Connected Event to Host immediately.
             */
            /* Require BSSID, Association ID, Beacon Interval.. from
             * AIS_BSS_INFO_T */
            /* p2pIndicationOfMediaStateToHost(prAdapter,
             * PARAM_MEDIA_STATE_CONNECTED, prStaRec->aucMacAddr);
             */
            if (prJoinInfo->prTargetBssDesc) {
                scanReportBss2Cfg80211(prAdapter, BSS_TYPE_P2P_DEVICE,
                                       prJoinInfo->prTargetBssDesc);
            }

            kalP2PGCIndicateConnectionStatus(
                prAdapter->prGlueInfo, prP2pRoleFsmInfo->ucRoleIndex,
                &prP2pRoleFsmInfo->rConnReqInfo, prJoinInfo->aucIEBuf,
                prJoinInfo->u4BufLength, prStaRec->u2StatusCode,
                WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY);
        } else {
            /* Join Fail */
            /* 4 <2.1> Redo JOIN process with other Auth Type if
             * possible */
            if (p2pFuncRetryJOIN(prAdapter, prStaRec, prJoinInfo) == false) {
                P_BSS_DESC_T prBssDesc;

                /* Increase Failure Count */
                prStaRec->ucJoinFailureCount++;

                prBssDesc = prJoinInfo->prTargetBssDesc;

                ASSERT(prBssDesc);
                ASSERT(prBssDesc->fgIsConnecting);

                prBssDesc->fgIsConnecting = false;

                if (prStaRec->ucJoinFailureCount >= P2P_SAA_RETRY_COUNT) {
                    kalP2PGCIndicateConnectionStatus(
                        prAdapter->prGlueInfo, prP2pRoleFsmInfo->ucRoleIndex,
                        &prP2pRoleFsmInfo->rConnReqInfo, prJoinInfo->aucIEBuf,
                        prJoinInfo->u4BufLength, prStaRec->u2StatusCode,
                        WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY);

                    /* Reset p2p state */
                    prJoinInfo->prTargetBssDesc = NULL;
                    prJoinInfo->prTargetStaRec = NULL;

                    SET_NET_PWR_STATE_IDLE(prAdapter, prP2pBssInfo->ucBssIndex);

                    p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                              P2P_ROLE_STATE_IDLE);

                    p2pFuncStopComplete(prAdapter, prP2pBssInfo);
                }
            }
        }
    }

    if (prP2pRoleFsmInfo->eCurrentState == P2P_ROLE_STATE_GC_JOIN) {
        if (prP2pBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
            /* do nothing & wait for timeout or EAPOL 4/4 TX done */
        } else {
            P_BSS_DESC_T prBssDesc;
            P2P_SSID_STRUCT_T rSsid;

            prBssDesc = prJoinInfo->prTargetBssDesc;

            if (prBssDesc) {
                COPY_SSID(rSsid.aucSsid, rSsid.ucSsidLen, prBssDesc->aucSSID,
                          prBssDesc->ucSSIDLen);
                p2pRoleFsmScanTargetBss(prAdapter, prP2pRoleFsmInfo,
                                        prBssDesc->ucChannelNum,
                                        prBssDesc->eBand, &rSsid);
            }
        }
    }

error:
    if (prAssocRspSwRfb) {
        nicRxReturnRFB(prAdapter, prAssocRspSwRfb);
    }

    cnmMemFree(prAdapter, prMsgHdr);
}

void p2pRoleFsmRunEventScanRequest(IN P_ADAPTER_T prAdapter,
                                   IN P_MSG_HDR_T prMsgHdr)
{
    P_MSG_P2P_SCAN_REQUEST_T prP2pScanReqMsg = (P_MSG_P2P_SCAN_REQUEST_T)NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_P2P_SCAN_REQ_INFO_T prScanReqInfo = (P_P2P_SCAN_REQ_INFO_T)NULL;
    u32 u4ChnlListSize = 0;
    P_P2P_SSID_STRUCT_T prP2pSsidStruct = (P_P2P_SSID_STRUCT_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = NULL;

    prP2pScanReqMsg = (P_MSG_P2P_SCAN_REQUEST_T)prMsgHdr;

    prP2pBssInfo = prAdapter->aprBssInfo[prP2pScanReqMsg->ucBssIdx];

    prP2pRoleFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prP2pBssInfo->u4PrivateData);

    if (!prP2pRoleFsmInfo) {
        DBGLOG(P2P, ERROR, "prP2pRoleFsmInfo is NULL!");
        goto error;
    }

    prP2pScanReqMsg = (P_MSG_P2P_SCAN_REQUEST_T)prMsgHdr;
    prScanReqInfo = &(prP2pRoleFsmInfo->rScanReqInfo);

    DBGLOG(P2P, TRACE, "p2pDevFsmRunEventScanRequest\n");

    /* Do we need to be in IDLE state? */
    /* p2pDevFsmRunEventAbort(prAdapter, prP2pDevFsmInfo); */

    ASSERT(prScanReqInfo->fgIsScanRequest == false);

    prScanReqInfo->fgIsAbort = true;
    prScanReqInfo->eScanType = prP2pScanReqMsg->eScanType;

    if (prP2pScanReqMsg->u4NumChannel) {
        prScanReqInfo->eChannelSet = SCAN_CHANNEL_SPECIFIED;

        /* Channel List */
        prScanReqInfo->ucNumChannelList = prP2pScanReqMsg->u4NumChannel;
        DBGLOG(P2P, TRACE, "Scan Request Channel List Number: %d\n",
               prScanReqInfo->ucNumChannelList);
        if (prScanReqInfo->ucNumChannelList > MAXIMUM_OPERATION_CHANNEL_LIST) {
            DBGLOG(P2P, TRACE,
                   "Channel List Number Overloaded: %d, change to: %d\n",
                   prScanReqInfo->ucNumChannelList,
                   MAXIMUM_OPERATION_CHANNEL_LIST);
            prScanReqInfo->ucNumChannelList = MAXIMUM_OPERATION_CHANNEL_LIST;
        }

        u4ChnlListSize =
            sizeof(RF_CHANNEL_INFO_T) * prScanReqInfo->ucNumChannelList;
        kalMemCopy(prScanReqInfo->arScanChannelList,
                   prP2pScanReqMsg->arChannelListInfo, u4ChnlListSize);
    } else {
        /* If channel number is ZERO.
         * It means do a FULL channel scan.
         */
        prScanReqInfo->eChannelSet = SCAN_CHANNEL_FULL;
    }

    /* SSID */
    prP2pSsidStruct = prP2pScanReqMsg->prSSID;
    for (prScanReqInfo->ucSsidNum = 0;
         prScanReqInfo->ucSsidNum < prP2pScanReqMsg->i4SsidNum;
         prScanReqInfo->ucSsidNum++) {
        kalMemCopy(
            prScanReqInfo->arSsidStruct[prScanReqInfo->ucSsidNum].aucSsid,
            prP2pSsidStruct->aucSsid, prP2pSsidStruct->ucSsidLen);

        prScanReqInfo->arSsidStruct[prScanReqInfo->ucSsidNum].ucSsidLen =
            prP2pSsidStruct->ucSsidLen;

        prP2pSsidStruct++;
    }

    /* IE Buffer */
    kalMemCopy(prScanReqInfo->aucIEBuf, prP2pScanReqMsg->pucIEBuf,
               prP2pScanReqMsg->u4IELen);

    prScanReqInfo->u4BufLength = prP2pScanReqMsg->u4IELen;

    p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo, P2P_ROLE_STATE_SCAN);

error:
    cnmMemFree(prAdapter, prMsgHdr);
}

void p2pRoleFsmRunEventScanDone(IN P_ADAPTER_T prAdapter,
                                IN P_MSG_HDR_T prMsgHdr,
                                IN P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo)
{
    P_MSG_SCN_SCAN_DONE prScanDoneMsg = (P_MSG_SCN_SCAN_DONE)prMsgHdr;
    P_P2P_SCAN_REQ_INFO_T prScanReqInfo = (P_P2P_SCAN_REQ_INFO_T)NULL;
    ENUM_P2P_ROLE_STATE_T eNextState = P2P_ROLE_STATE_NUM;
    P_P2P_CONNECTION_REQ_INFO_T prConnReqInfo =
        &(prP2pRoleFsmInfo->rConnReqInfo);
    P_P2P_JOIN_INFO_T prP2pJoinInfo = &(prP2pRoleFsmInfo->rJoinInfo);
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_P2P_CHNL_REQ_INFO_T prChnlReqInfo = (P_P2P_CHNL_REQ_INFO_T)NULL;
#if CFG_SUPPORT_DBDC
    CNM_DBDC_CAP_T rDbdcCap;
#endif

    if (prP2pRoleFsmInfo == NULL) {
        DBGLOG(P2P, TRACE, "prP2pRoleFsmInfo is NULL\n");
        goto error;
    }

    DBGLOG(P2P, TRACE, "P2P Role Scan Done Event\n");

    prScanReqInfo = &(prP2pRoleFsmInfo->rScanReqInfo);
    prScanDoneMsg = (P_MSG_SCN_SCAN_DONE)prMsgHdr;

    if (prScanDoneMsg->ucSeqNum != prScanReqInfo->ucSeqNumOfScnMsg) {
        /* Scan Done message sequence number mismatch.
         * Ignore this event. (P2P FSM issue two scan events.)
         */
        /* The scan request has been cancelled.
         * Ignore this message. It is possible.
         */
        DBGLOG(P2P, TRACE,
               "P2P Role Scan Don SeqNum Received:%d <-> P2P Role Fsm SCAN Seq "
               "Issued:%d\n",
               prScanDoneMsg->ucSeqNum, prScanReqInfo->ucSeqNumOfScnMsg);

        goto error;
    }

    switch (prP2pRoleFsmInfo->eCurrentState) {
    case P2P_ROLE_STATE_SCAN:
        prScanReqInfo->fgIsAbort = false;

        if (prConnReqInfo->eConnRequest == P2P_CONNECTION_TYPE_GC) {
            prP2pJoinInfo->prTargetBssDesc = p2pFuncKeepOnConnection(
                prAdapter, prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex],
                prConnReqInfo, &prP2pRoleFsmInfo->rChnlReqInfo,
                &prP2pRoleFsmInfo->rScanReqInfo);
            if ((prP2pJoinInfo->prTargetBssDesc) == NULL) {
                eNextState = P2P_ROLE_STATE_SCAN;
            } else {
                prP2pBssInfo =
                    prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex];
                if (!prP2pBssInfo) {
                    break;
                }
                prChnlReqInfo = &(prP2pRoleFsmInfo->rChnlReqInfo);
                if (!prChnlReqInfo) {
                    break;
                }
#if (CFG_HW_WMM_BY_BSS == 1)
                if (prP2pBssInfo->fgIsWmmInited == false) {
                    prP2pBssInfo->ucWmmQueSet =
                        cnmWmmIndexDecision(prAdapter, prP2pBssInfo);
                }
                prP2pBssInfo->eBand = prChnlReqInfo->eBand;
#endif
#if CFG_SUPPORT_DBDC
                cnmDbdcEnableDecision(prAdapter, prP2pRoleFsmInfo->ucBssIndex,
                                      prChnlReqInfo->eBand);
                cnmGetDbdcCapability(
                    prAdapter, prP2pRoleFsmInfo->ucBssIndex,
                    prChnlReqInfo->eBand, prChnlReqInfo->ucReqChnlNum,
                    wlanGetSupportNss(prAdapter, prP2pRoleFsmInfo->ucBssIndex),
                    &rDbdcCap);

                DBGLOG(
                    P2P, INFO,
                    "p2pRoleFsmRunEventScanDone: start GC at CH %u, NSS=%u.\n",
                    prChnlReqInfo->ucReqChnlNum, rDbdcCap.ucNss);

                prP2pBssInfo->eDBDCBand = ENUM_BAND_AUTO;
                prP2pBssInfo->ucNss = rDbdcCap.ucNss;
#if (CFG_HW_WMM_BY_BSS == 0)
                prP2pBssInfo->ucWmmQueSet = rDbdcCap.ucWmmSetIndex;
#endif
#endif
                /* For GC join. */
                eNextState = P2P_ROLE_STATE_REQING_CHANNEL;
            }
        } else {
            eNextState = P2P_ROLE_STATE_IDLE;
        }
        break;

    case P2P_ROLE_STATE_AP_CHNL_DETECTION:
        eNextState = P2P_ROLE_STATE_REQING_CHANNEL;
        break;

    default:
        /* Unexpected channel scan done event without being chanceled.
         */
        ASSERT(false);
        break;
    }

    prScanReqInfo->fgIsScanRequest = false;

    p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo, eNextState);

error:
    cnmMemFree(prAdapter, prMsgHdr);
}

void p2pRoleFsmRunEventChnlGrant(IN P_ADAPTER_T prAdapter,
                                 IN P_MSG_HDR_T prMsgHdr,
                                 IN P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo)
{
    P_P2P_CHNL_REQ_INFO_T prChnlReqInfo = (P_P2P_CHNL_REQ_INFO_T)NULL;
    P_MSG_CH_GRANT_T prMsgChGrant = (P_MSG_CH_GRANT_T)NULL;
#if (CFG_SUPPORT_DFS_MASTER == 1)
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    u32 u4CacTimeMs;
#endif
    u8 ucTokenID = 0;

    if (!prP2pRoleFsmInfo) {
        DBGLOG(P2P, ERROR, "prP2pRoleFsmInfo is NULL!\n");
        goto error;
    }

    DBGLOG(P2P, TRACE, "P2P Run Event Role Channel Grant\n");

    prMsgChGrant = (P_MSG_CH_GRANT_T)prMsgHdr;
    ucTokenID = prMsgChGrant->ucTokenID;
    prChnlReqInfo = &(prP2pRoleFsmInfo->rChnlReqInfo);

#if (CFG_SUPPORT_DFS_MASTER == 1)
    prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsgChGrant->ucBssIndex);
#endif
    if (prChnlReqInfo->u4MaxInterval != prMsgChGrant->u4GrantInterval) {
        DBGLOG(P2P, WARN,
               "P2P Role:%d Request Channel Interval:%d, Grant Interval:%d\n",
               prP2pRoleFsmInfo->ucRoleIndex, prChnlReqInfo->u4MaxInterval,
               prMsgChGrant->u4GrantInterval);
        prChnlReqInfo->u4MaxInterval = prMsgChGrant->u4GrantInterval;
    }

    if (ucTokenID == prChnlReqInfo->ucSeqNumOfChReq) {
        ENUM_P2P_ROLE_STATE_T eNextState = P2P_ROLE_STATE_NUM;

        switch (prP2pRoleFsmInfo->eCurrentState) {
        case P2P_ROLE_STATE_REQING_CHANNEL:
            switch (prChnlReqInfo->eChnlReqType) {
            case CH_REQ_TYPE_JOIN:
                eNextState = P2P_ROLE_STATE_GC_JOIN;
                break;

            case CH_REQ_TYPE_GO_START_BSS:
                eNextState = P2P_ROLE_STATE_IDLE;
                break;

            default:
                DBGLOG(
                    P2P, WARN,
                    "p2pRoleFsmRunEventChnlGrant: Invalid Channel Request Type:%d\n",
                    prChnlReqInfo->eChnlReqType);
                ASSERT(false);
                break;
            }

            p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo, eNextState);
            break;

#if (CFG_SUPPORT_DFS_MASTER == 1)
        case P2P_ROLE_STATE_DFS_CAC:
#ifndef CFG_SUPPORT_DISABLE_SAP_DFS_RDD
            p2pFuncStartRdd(prAdapter, prMsgChGrant->ucBssIndex);

            if (p2pFuncCheckWeatherRadarBand(prChnlReqInfo)) {
                u4CacTimeMs = P2P_AP_CAC_WEATHER_CHNL_HOLD_TIME_MS;
            } else {
                u4CacTimeMs = prP2pRoleFsmInfo->rChnlReqInfo.u4MaxInterval;
            }
#else
            u4CacTimeMs = 0;
#endif

            if (p2pFuncIsManualCac()) {
                u4CacTimeMs = p2pFuncGetDriverCacTime() * 1000;
            } else {
                p2pFuncSetDriverCacTime(u4CacTimeMs / 1000);
            }

            cnmTimerStartTimer(prAdapter,
                               &(prP2pRoleFsmInfo->rP2pRoleFsmTimeoutTimer),
                               u4CacTimeMs);

            p2pFuncRecordCacStartBootTime();

            p2pFuncSetDfsState(DFS_STATE_CHECKING);

            DBGLOG(P2P, INFO, "p2pRoleFsmRunEventChnlGrant: CAC time = %ds\n",
                   u4CacTimeMs / 1000);
            break;

        case P2P_ROLE_STATE_SWITCH_CHANNEL:
            p2pFuncDfsSwitchCh(prAdapter, prP2pBssInfo,
                               prP2pRoleFsmInfo->rChnlReqInfo);
            p2pRoleFsmStateTransition(prAdapter, prP2pRoleFsmInfo,
                                      P2P_ROLE_STATE_IDLE);
            break;

#endif
        default:
            /* Channel is granted under unexpected state.
             * Driver should cancel channel privileagea before
             * leaving the states.
             */
            if (IS_BSS_ACTIVE(
                    prAdapter->aprBssInfo[prP2pRoleFsmInfo->ucBssIndex])) {
                DBGLOG(P2P, WARN,
                       "p2pRoleFsmRunEventChnlGrant: Invalid CurrentState:%d\n",
                       prP2pRoleFsmInfo->eCurrentState);
                ASSERT(false);
            }
            break;
        }
    } else {
        /* Channel requsted, but released. */
        ASSERT(!prChnlReqInfo->fgIsChannelRequested);
        if (prChnlReqInfo->fgIsChannelRequested) {
            DBGLOG(P2P, ERROR,
                   "fgIsChannelRequested is true!Channel was requested, but "
                   "released!\n");
        }
    }

error:
    cnmMemFree(prAdapter, prMsgHdr);
}

/* ////////////////////////////////////// */
void p2pRoleFsmRunEventDissolve(IN P_ADAPTER_T prAdapter,
                                IN P_MSG_HDR_T prMsgHdr)
{
    /* TODO: */

    if (prMsgHdr) {
        cnmMemFree(prAdapter, prMsgHdr);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @ This routine update the current MAC table based on the current ACL.
 * If ACL change causing an associated STA become un-authorized. This STA
 * will be kicked out immediately.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] ucBssIdx            Bss index.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void p2pRoleUpdateACLEntry(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx)
{
    u8 bMatchACL = false;
    s32 i = 0, i4Ret = 0;
    P_LINK_T prClientList;
    P_STA_RECORD_T prCurrStaRec, prNextStaRec;
    P_BSS_INFO_T prP2pBssInfo;

    if ((!prAdapter) || (ucBssIdx > HW_BSSID_NUM)) {
        return;
    }

    DBGLOG(P2P, TRACE, "Update ACL Entry ucBssIdx = %d\n", ucBssIdx);
    prP2pBssInfo = prAdapter->aprBssInfo[ucBssIdx];

    /* ACL is disabled. Do nothing about the MAC table. */
    if (prP2pBssInfo->rACL.ePolicy == PARAM_CUSTOM_ACL_POLICY_DISABLE) {
        return;
    }

    prClientList = &prP2pBssInfo->rStaRecOfClientList;

    LINK_FOR_EACH_ENTRY_SAFE(prCurrStaRec, prNextStaRec, prClientList,
                             rLinkEntry, STA_RECORD_T){
        bMatchACL = false;
        for (i = 0; i < prP2pBssInfo->rACL.u4Num; i++) {
            if (EQUAL_MAC_ADDR(prCurrStaRec->aucMacAddr,
                               prP2pBssInfo->rACL.rEntry[i].aucAddr)) {
                bMatchACL = true;
                break;
            }
        }

        if (((!bMatchACL) &&
             (prP2pBssInfo->rACL.ePolicy == PARAM_CUSTOM_ACL_POLICY_ACCEPT)) ||
            ((bMatchACL) &&
             (prP2pBssInfo->rACL.ePolicy == PARAM_CUSTOM_ACL_POLICY_DENY))) {
            DBGLOG(P2P, TRACE, "ucBssIdx=%d, ACL Policy=%d\n", ucBssIdx,
                   prP2pBssInfo->rACL.ePolicy);

            i4Ret = assocSendDisAssocFrame(prAdapter, prCurrStaRec,
                                           STATUS_CODE_REQ_DECLINED);
            if (!i4Ret) {
                DBGLOG(P2P, TRACE,
                       "Send DISASSOC to [" MACSTR "], Reason = %d\n",
                       MAC2STR(prCurrStaRec->aucMacAddr),
                       STATUS_CODE_REQ_DECLINED);
            }
            LINK_REMOVE_KNOWN_ENTRY(prClientList, &prCurrStaRec->rLinkEntry);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @ Check if the specified STA pass the Access Control List inspection.
 * If fails to pass the checking, then no authentication or association is
 * allowed.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] pMacAddr           Pointer to the mac address.
 * @param[in] ucBssIdx            Bss index.
 *
 * @return true - pass ACL inspection, false - ACL inspection fail
 */
/*----------------------------------------------------------------------------*/
u8 p2pRoleProcessACLInspection(IN P_ADAPTER_T prAdapter, IN u8 *pMacAddr,
                               IN u8 ucBssIdx)
{
    u8 bPassACL = true;
    s32 i = 0;
    P_BSS_INFO_T prP2pBssInfo;

    if ((!prAdapter) || (!pMacAddr) || (ucBssIdx > HW_BSSID_NUM)) {
        return false;
    }

    prP2pBssInfo = prAdapter->aprBssInfo[ucBssIdx];

    if (prP2pBssInfo->rACL.ePolicy == PARAM_CUSTOM_ACL_POLICY_DISABLE) {
        return true;
    }

    if (prP2pBssInfo->rACL.ePolicy == PARAM_CUSTOM_ACL_POLICY_ACCEPT) {
        bPassACL = false;
    } else {
        bPassACL = true;
    }

    for (i = 0; i < prP2pBssInfo->rACL.u4Num; i++) {
        if (EQUAL_MAC_ADDR(pMacAddr, prP2pBssInfo->rACL.rEntry[i].aucAddr)) {
            bPassACL = !bPassACL;
            break;
        }
    }

    if (bPassACL == false) {
        DBGLOG(P2P, WARN,
               "this mac [" MACSTR "] is fail to pass ACL inspection.\n",
               MAC2STR(pMacAddr));
    }

    return bPassACL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate the Event of Successful Completion of AAA
 * Module.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
p2pRoleFsmRunEventAAAComplete(IN P_ADAPTER_T prAdapter,
                              IN P_STA_RECORD_T prStaRec,
                              IN P_BSS_INFO_T prP2pBssInfo)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    ENUM_PARAM_MEDIA_STATE_T eOriMediaState;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prStaRec != NULL) &&
                     (prP2pBssInfo != NULL));

        eOriMediaState = prP2pBssInfo->eConnectionState;

        bssRemoveClient(prAdapter, prP2pBssInfo, prStaRec);

        if (prP2pBssInfo->rStaRecOfClientList.u4NumElem >=
            P2P_MAXIMUM_CLIENT_COUNT ||
            !p2pRoleProcessACLInspection(prAdapter, prStaRec->aucMacAddr,
                                         prP2pBssInfo->ucBssIndex)
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
            || kalP2PMaxClients(prAdapter->prGlueInfo,
                                prP2pBssInfo->rStaRecOfClientList.u4NumElem,
                                (u8)prP2pBssInfo->u4PrivateData)
#endif
            ) {
            rStatus = WLAN_STATUS_RESOURCES;
            break;
        }

        bssAddClient(prAdapter, prP2pBssInfo, prStaRec);

        prStaRec->u2AssocId = bssAssignAssocID(prStaRec);

        cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);

#if CFG_SUPPORT_BFER
        if ((IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaVhtBfer) ||
             IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaHtBfer)) &&
            prAdapter->fgIsHwSupportBfer) {
            rlmBfStaRecPfmuUpdate(prAdapter, prStaRec);
            rlmETxBfTriggerPeriodicSounding(prAdapter);
        }
#endif

        p2pChangeMediaState(prAdapter, prP2pBssInfo,
                            PARAM_MEDIA_STATE_CONNECTED);

        /* Update Connected state to FW. */
        if (eOriMediaState != prP2pBssInfo->eConnectionState) {
            nicUpdateBss(prAdapter, prP2pBssInfo->ucBssIndex);
        }
    } while (false);

    return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate the Event of Successful Completion of AAA
 * Module.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
p2pRoleFsmRunEventAAASuccess(IN P_ADAPTER_T prAdapter,
                             IN P_STA_RECORD_T prStaRec,
                             IN P_BSS_INFO_T prP2pBssInfo)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prStaRec != NULL) &&
                     (prP2pBssInfo != NULL));

        if ((prP2pBssInfo->eNetworkType != NETWORK_TYPE_P2P) ||
            (prP2pBssInfo->u4PrivateData >= BSS_P2P_NUM)) {
            ASSERT(false);
            rStatus = WLAN_STATUS_INVALID_DATA;
            break;
        }

        ASSERT(prP2pBssInfo->ucBssIndex < P2P_DEV_BSS_INDEX);

        prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
            prAdapter, prP2pBssInfo->u4PrivateData);

        /* Glue layer indication. */
        kalP2PGOStationUpdate(prAdapter->prGlueInfo,
                              prP2pRoleFsmInfo->ucRoleIndex, prStaRec, true);
    } while (false);

    return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate the Event of Tx Fail of AAA Module.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void p2pRoleFsmRunEventAAATxFail(IN P_ADAPTER_T prAdapter,
                                 IN P_STA_RECORD_T prStaRec,
                                 IN P_BSS_INFO_T prP2pBssInfo)
{
    ASSERT(prAdapter);
    ASSERT(prStaRec);

    bssRemoveClient(prAdapter, prP2pBssInfo, prStaRec);

    p2pFuncDisconnect(prAdapter, prP2pBssInfo, prStaRec, false,
                      prStaRec->eAuthAssocState == AAA_STATE_SEND_AUTH2 ?
                      STATUS_CODE_AUTH_TIMEOUT :
                      STATUS_CODE_ASSOC_TIMEOUT,
                      true);

    /* 20120830 moved into p2puUncDisconnect. */
    /* cnmStaRecFree(prAdapter, prStaRec); */
}

void p2pRoleFsmRunEventSwitchOPMode(IN P_ADAPTER_T prAdapter,
                                    IN P_MSG_HDR_T prMsgHdr)
{
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_MSG_P2P_SWITCH_OP_MODE_T prSwitchOpMode =
        (P_MSG_P2P_SWITCH_OP_MODE_T)prMsgHdr;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;

    ASSERT(prSwitchOpMode->ucRoleIdx < BSS_P2P_NUM);
    if (!(prSwitchOpMode->ucRoleIdx < BSS_P2P_NUM)) {
        DBGLOG(P2P, ERROR,
               "prSwitchOpMode->ucRoleIdx %d should < BSS_P2P_NUM(%d)\n",
               prSwitchOpMode->ucRoleIdx, BSS_P2P_NUM);
        goto error;
    }

    DBGLOG(P2P, TRACE, "p2pRoleFsmRunEventSwitchOPMode\n");

    prP2pRoleFsmInfo =
        prAdapter->rWifiVar.aprP2pRoleFsmInfo[prSwitchOpMode->ucRoleIdx];

    ASSERT(prP2pRoleFsmInfo->ucBssIndex < P2P_DEV_BSS_INDEX);

    prP2pBssInfo =
        GET_BSS_INFO_BY_INDEX(prAdapter, prP2pRoleFsmInfo->ucBssIndex);

    if (!(prSwitchOpMode->eOpMode < OP_MODE_NUM)) {
        DBGLOG(P2P, ERROR,
               "prSwitchOpMode->eOpMode %d should < OP_MODE_NUM(%d)\n",
               prSwitchOpMode->eOpMode, OP_MODE_NUM);
        goto error;
    }

    /* P2P Device / GC. */
    p2pFuncSwitchOPMode(prAdapter, prP2pBssInfo, prSwitchOpMode->eOpMode, true);

error:
    cnmMemFree(prAdapter, prMsgHdr);
}

/* /////////////////////////////// TO BE REFINE ////////////////////////////////
 */
void p2pRoleFsmRunEventBeaconUpdate(IN P_ADAPTER_T prAdapter,
                                    IN P_MSG_HDR_T prMsgHdr)
{
    P_P2P_ROLE_FSM_INFO_T prRoleP2pFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_MSG_P2P_BEACON_UPDATE_T prBcnUpdateMsg = (P_MSG_P2P_BEACON_UPDATE_T)NULL;
    P_P2P_BEACON_UPDATE_INFO_T prBcnUpdateInfo =
        (P_P2P_BEACON_UPDATE_INFO_T)NULL;

    DBGLOG(P2P, TRACE, "p2pRoleFsmRunEventBeaconUpdate\n");
    prBcnUpdateMsg = (P_MSG_P2P_BEACON_UPDATE_T)prMsgHdr;
    if (prBcnUpdateMsg->ucRoleIndex >= BSS_P2P_NUM) {
        DBGLOG(P2P, ERROR,
               "prBcnUpdateMsg->ucRoleIndex %d should < BSS_P2P_NUM(%d)\n",
               prBcnUpdateMsg->ucRoleIndex, BSS_P2P_NUM);
        goto error;
    }

    prRoleP2pFsmInfo =
        P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, prBcnUpdateMsg->ucRoleIndex);
    if (!prRoleP2pFsmInfo) {
        DBGLOG(P2P, ERROR,
               "prRoleP2pFsmInfo of prBcnUpdateMsg->ucRoleIndex %d is NULL\n",
               prBcnUpdateMsg->ucRoleIndex);
        goto error;
    }

    prP2pBssInfo =
        GET_BSS_INFO_BY_INDEX(prAdapter, prRoleP2pFsmInfo->ucBssIndex);

    prP2pBssInfo->fgIsWepCipherGroup = prBcnUpdateMsg->fgIsWepCipher;

    prBcnUpdateInfo = &(prRoleP2pFsmInfo->rBeaconUpdateInfo);

    p2pFuncBeaconUpdate(prAdapter, prP2pBssInfo, prBcnUpdateInfo,
                        prBcnUpdateMsg->pucBcnHdr, prBcnUpdateMsg->u4BcnHdrLen,
                        prBcnUpdateMsg->pucBcnBody,
                        prBcnUpdateMsg->u4BcnBodyLen);

    if (prBcnUpdateMsg->pucAssocRespIE != NULL &&
        prBcnUpdateMsg->u4AssocRespLen > 0) {
        DBGLOG(P2P, TRACE, "Copy extra IEs for assoc resp (Length= %d)\n",
               prBcnUpdateMsg->u4AssocRespLen);
        DBGLOG_MEM8(P2P, INFO, prBcnUpdateMsg->pucAssocRespIE,
                    prBcnUpdateMsg->u4AssocRespLen);

        if (p2pFuncAssocRespUpdate(
                prAdapter, prP2pBssInfo, prBcnUpdateMsg->pucAssocRespIE,
                prBcnUpdateMsg->u4AssocRespLen) == WLAN_STATUS_FAILURE) {
            DBGLOG(P2P, INFO, "Update extra IEs for asso resp fail!\n");
        }
    }

    if ((prP2pBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) &&
        (prP2pBssInfo->eIntendOPMode == OP_MODE_NUM)) {
        /* AP is created, Beacon Update. */
        /* nicPmIndicateBssAbort(prAdapter, NETWORK_TYPE_P2P_INDEX); */

        DBGLOG(P2P, TRACE, "p2pRoleFsmRunEventBeaconUpdate with Bssidex(%d)\n",
               prRoleP2pFsmInfo->ucBssIndex);

        bssUpdateBeaconContent(prAdapter, prRoleP2pFsmInfo->ucBssIndex);

        /* nicPmIndicateBssCreated(prAdapter, NETWORK_TYPE_P2P_INDEX);
         */
    }

error:
    cnmMemFree(prAdapter, prMsgHdr);
}

void p2pProcessEvent_UpdateNOAParam(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx,
                                    IN P_EVENT_UPDATE_NOA_PARAMS_T
                                    prEventUpdateNoaParam)
{
    P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T)NULL;
    P_P2P_SPECIFIC_BSS_INFO_T prP2pSpecificBssInfo;
    u32 i;
    u8 fgNoaAttrExisted = false;

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
    prP2pSpecificBssInfo =
        prAdapter->rWifiVar.prP2pSpecificBssInfo[prBssInfo->u4PrivateData];

    prP2pSpecificBssInfo->fgEnableOppPS = prEventUpdateNoaParam->ucEnableOppPS;
    prP2pSpecificBssInfo->u2CTWindow = prEventUpdateNoaParam->u2CTWindow;
    prP2pSpecificBssInfo->ucNoAIndex = prEventUpdateNoaParam->ucNoAIndex;
    prP2pSpecificBssInfo->ucNoATimingCount =
        prEventUpdateNoaParam->ucNoATimingCount;

    fgNoaAttrExisted |= prP2pSpecificBssInfo->fgEnableOppPS;

    ASSERT(prP2pSpecificBssInfo->ucNoATimingCount <= P2P_MAXIMUM_NOA_COUNT);

    for (i = 0; i < prP2pSpecificBssInfo->ucNoATimingCount; i++) {
        /* in used */
        prP2pSpecificBssInfo->arNoATiming[i].fgIsInUse =
            prEventUpdateNoaParam->arEventNoaTiming[i].ucIsInUse;
        /* count */
        prP2pSpecificBssInfo->arNoATiming[i].ucCount =
            prEventUpdateNoaParam->arEventNoaTiming[i].ucCount;
        /* duration */
        prP2pSpecificBssInfo->arNoATiming[i].u4Duration =
            prEventUpdateNoaParam->arEventNoaTiming[i].u4Duration;
        /* interval */
        prP2pSpecificBssInfo->arNoATiming[i].u4Interval =
            prEventUpdateNoaParam->arEventNoaTiming[i].u4Interval;
        /* start time */
        prP2pSpecificBssInfo->arNoATiming[i].u4StartTime =
            prEventUpdateNoaParam->arEventNoaTiming[i].u4StartTime;

        fgNoaAttrExisted |= prP2pSpecificBssInfo->arNoATiming[i].fgIsInUse;
    }

    prP2pSpecificBssInfo->fgIsNoaAttrExisted = fgNoaAttrExisted;

    DBGLOG(P2P, TRACE, "p2pProcessEvent_UpdateNOAParam\n");
    /* update beacon content by the change */
    bssUpdateBeaconContent(prAdapter, ucBssIdx);
}
