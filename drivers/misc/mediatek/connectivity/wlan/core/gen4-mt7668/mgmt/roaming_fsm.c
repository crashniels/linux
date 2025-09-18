// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "roaming_fsm.c"
 *    \brief  This file defines the FSM for Roaming MODULE.
 *
 *    This file defines the FSM for Roaming MODULE.
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

#if CFG_SUPPORT_ROAMING
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
static u8 *apucDebugRoamingState[ROAMING_STATE_NUM] = {
    (u8 *)DISP_STRING("IDLE"), (u8 *)DISP_STRING("DECISION"),
    (u8 *)DISP_STRING("DISCOVERY"), (u8 *)DISP_STRING("ROAM")
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
/*!
 * @brief Initialize the value in ROAMING_FSM_INFO_T for ROAMING FSM operation
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void roamingFsmInit(IN P_ADAPTER_T prAdapter)
{
    P_ROAMING_INFO_T prRoamingFsmInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;

    DBGLOG(ROAMING, LOUD, "->roamingFsmInit(): Current Time = %ld\n",
           kalGetTimeTick());

    prRoamingFsmInfo = (P_ROAMING_INFO_T)&
                       (prAdapter->rWifiVar.rRoamingInfo);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    /* 4 <1> Initiate FSM */
    prRoamingFsmInfo->fgIsEnableRoaming = prConnSettings->fgIsEnableRoaming;
    prRoamingFsmInfo->eCurrentState = ROAMING_STATE_IDLE;
    prRoamingFsmInfo->rRoamingDiscoveryUpdateTime = 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Uninitialize the value in AIS_FSM_INFO_T for AIS FSM operation
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void roamingFsmUninit(IN P_ADAPTER_T prAdapter)
{
    P_ROAMING_INFO_T prRoamingFsmInfo;

    DBGLOG(ROAMING, LOUD, "->roamingFsmUninit(): Current Time = %ld\n",
           kalGetTimeTick());

    prRoamingFsmInfo = (P_ROAMING_INFO_T)&
                       (prAdapter->rWifiVar.rRoamingInfo);

    prRoamingFsmInfo->eCurrentState = ROAMING_STATE_IDLE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Send commands to firmware
 *
 * @param [IN P_ADAPTER_T]       prAdapter
 *        [IN P_ROAMING_PARAM_T] prParam
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void roamingFsmSendCmd(IN P_ADAPTER_T prAdapter,
                       IN P_CMD_ROAMING_TRANSIT_T prTransit)
{
    P_ROAMING_INFO_T prRoamingFsmInfo;
    WLAN_STATUS rStatus;

    DBGLOG(ROAMING, LOUD, "->roamingFsmSendCmd(): Current Time = %ld\n",
           kalGetTimeTick());

    prRoamingFsmInfo = (P_ROAMING_INFO_T)&
                       (prAdapter->rWifiVar.rRoamingInfo);

    rStatus = wlanSendSetQueryCmd(
        prAdapter,  /* prAdapter */
        CMD_ID_ROAMING_TRANSIT,  /* ucCID */
        true,  /* fgSetQuery */
        false,  /* fgNeedResp */
        false,  /* fgIsOid */
        NULL,  /* pfCmdDoneHandler */
        NULL,  /* pfCmdTimeoutHandler */
        sizeof(CMD_ROAMING_TRANSIT_T),  /* u4SetQueryInfoLen
                                         */
        (u8 *)prTransit,  /* pucInfoBuffer */
        NULL,  /* pvSetQueryBuffer */
        0  /* u4SetQueryBufferLen */
        );

    /* ASSERT(rStatus == WLAN_STATUS_PENDING); */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Update the recent time when ScanDone occurred
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void roamingFsmScanResultsUpdate(IN P_ADAPTER_T prAdapter)
{
    P_ROAMING_INFO_T prRoamingFsmInfo;

    prRoamingFsmInfo = (P_ROAMING_INFO_T)&
                       (prAdapter->rWifiVar.rRoamingInfo);

    /* Check Roaming Conditions */
    if (!(prRoamingFsmInfo->fgIsEnableRoaming)) {
        return;
    }

    DBGLOG(ROAMING, LOUD,
           "->roamingFsmScanResultsUpdate(): Current Time = %ld\n",
           kalGetTimeTick());

    GET_CURRENT_SYSTIME(&prRoamingFsmInfo->rRoamingDiscoveryUpdateTime);
}

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
/*----------------------------------------------------------------------------*/
/*
 * @brief Check if need to do scan for roaming
 *
 * @param[out] fgIsNeedScan Set to true if need to scan since
 * there is roaming candidate in current scan result or skip roaming times >
 * limit times
 * @return
 */
/*----------------------------------------------------------------------------*/
static u8 roamingFsmIsNeedScan(IN P_ADAPTER_T prAdapter)
{
    P_SCAN_INFO_T prScanInfo;
    P_LINK_T prRoamBSSDescList;
    P_ROAM_BSS_DESC_T prRoamBssDesc;
    P_BSS_INFO_T prAisBssInfo;
    P_BSS_DESC_T prBssDesc;
    /*CMD_SW_DBG_CTRL_T rCmdSwCtrl;*/
    CMD_ROAMING_SKIP_ONE_AP_T rCmdRoamingSkipOneAP;
    u8 fgIsNeedScan, fgIsRoamingSSID;

    fgIsNeedScan = false;
    fgIsRoamingSSID = false;  /*Whether there's roaming candidate in
                               * RoamBssDescList*/

    kalMemZero(&rCmdRoamingSkipOneAP, sizeof(CMD_ROAMING_SKIP_ONE_AP_T));

    prAisBssInfo = prAdapter->prAisBssInfo;
    prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
    prRoamBSSDescList = &prScanInfo->rRoamBSSDescList;
    /* <1> Count same BSS Desc from current SCAN result list. */
    LINK_FOR_EACH_ENTRY(prRoamBssDesc, prRoamBSSDescList, rLinkEntry,
                        ROAM_BSS_DESC_T) {
        if (EQUAL_SSID(prRoamBssDesc->aucSSID, prRoamBssDesc->ucSSIDLen,
                       prAisBssInfo->aucSSID,
                       prAisBssInfo->ucSSIDLen)) {
            fgIsRoamingSSID = true;
            fgIsNeedScan = true;
            DBGLOG(ROAMING, INFO,
                   "roamingFsmSteps: IsRoamingSSID:%d\n",
                   fgIsRoamingSSID);
            break;
        }
    }

    /* <2> Start skip roaming scan mechanism if there is no candidate in
     * current SCAN result list */
    if (!fgIsRoamingSSID) {
        prBssDesc = scanSearchBssDescByBssid(
            prAdapter, prAisBssInfo->aucBSSID);  /* Get current
                                                  * BssDesc */
        if (prBssDesc) {
            /*rCmdSwCtrl.u4Id = 0xa0280000;*/
            /*rCmdSwCtrl.u4Data = 0x1;*/
            rCmdRoamingSkipOneAP.fgIsRoamingSkipOneAP = 1;

            DBGLOG(ROAMING, INFO,
                   "roamingFsmSteps: RCPI:%d RoamSkipTimes:%d\n",
                   prBssDesc->ucRCPI,
                   prAisBssInfo->ucRoamSkipTimes);
            if (prBssDesc->ucRCPI > 90) {  /* Set parameters related
                                            * to Good Area */
                prAisBssInfo->ucRoamSkipTimes = 3;
                prAisBssInfo->fgGoodRcpiArea = true;
                prAisBssInfo->fgPoorRcpiArea = false;
            } else {
                if (prAisBssInfo->fgGoodRcpiArea) {
                    prAisBssInfo->ucRoamSkipTimes--;
                } else if (prBssDesc->ucRCPI > 67) {
                    if (!prAisBssInfo
                        ->fgPoorRcpiArea) {  /* Set
                                              * parameters
                                              * related
                                              * to
                                              * Poor
                                              * Area
                                              */
                        prAisBssInfo->ucRoamSkipTimes =
                            2;
                        prAisBssInfo->fgPoorRcpiArea =
                            true;
                        prAisBssInfo->fgGoodRcpiArea =
                            false;
                    } else {
                        prAisBssInfo->ucRoamSkipTimes--;
                    }
                } else {
                    prAisBssInfo->fgPoorRcpiArea = false;
                    prAisBssInfo->fgGoodRcpiArea = false;
                    prAisBssInfo->ucRoamSkipTimes--;
                }
            }

            if (prAisBssInfo->ucRoamSkipTimes == 0) {
                prAisBssInfo->ucRoamSkipTimes = 3;
                prAisBssInfo->fgPoorRcpiArea = false;
                prAisBssInfo->fgGoodRcpiArea = false;
                DBGLOG(ROAMING, INFO,
                       "roamingFsmSteps: Need Scan\n");
                fgIsNeedScan = true;
            } else {
                wlanSendSetQueryCmd(
                    prAdapter, CMD_ID_SET_ROAMING_SKIP,
                    true, false, false, NULL, NULL,
                    sizeof(CMD_ROAMING_SKIP_ONE_AP_T),
                    (u8 *)&rCmdRoamingSkipOneAP, NULL, 0);
            }
        } else {
            DBGLOG(ROAMING,
                   WARN,
                   "Can't find the current associated AP in BssDescList\n");
        }
    }

    return fgIsNeedScan;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief The Core FSM engine of ROAMING for AIS Infra.
 *
 * @param [IN P_ADAPTER_T]          prAdapter
 *        [IN ENUM_ROAMING_STATE_T] eNextState Enum value of next AIS STATE
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void roamingFsmSteps(IN P_ADAPTER_T prAdapter,
                     IN ENUM_ROAMING_STATE_T eNextState)
{
    P_ROAMING_INFO_T prRoamingFsmInfo;
    ENUM_ROAMING_STATE_T ePreviousState;
    u8 fgIsTransition = (u8)false;
    u8 fgIsNeedScan = false;

    prRoamingFsmInfo = (P_ROAMING_INFO_T)&
                       (prAdapter->rWifiVar.rRoamingInfo);

    do {
        /* Do entering Next State */
        DBGLOG(ROAMING, STATE, "[ROAMING]TRANSITION: [%s] -> [%s]\n",
               apucDebugRoamingState[prRoamingFsmInfo->eCurrentState],
               apucDebugRoamingState[eNextState]);

        /* NOTE(Kevin): This is the only place to change the
         * eCurrentState(except initial) */
        ePreviousState = prRoamingFsmInfo->eCurrentState;
        prRoamingFsmInfo->eCurrentState = eNextState;

        fgIsTransition = (u8)false;

        /* Do tasks of the State that we just entered */
        switch (prRoamingFsmInfo->eCurrentState) {
        /* NOTE(Kevin): we don't have to rearrange the sequence of
         * following switch case. Instead I would like to use a common
         * lookup table of array of function pointer to speed up state
         * search.
         */
        case ROAMING_STATE_IDLE:
        case ROAMING_STATE_DECISION:
            break;

        case ROAMING_STATE_DISCOVERY: {
            u32 rCurrentTime;
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
            fgIsNeedScan = roamingFsmIsNeedScan(prAdapter);
#else
            fgIsNeedScan = true;
#endif

            GET_CURRENT_SYSTIME(&rCurrentTime);
            if (CHECK_FOR_TIMEOUT(
                    rCurrentTime,
                    prRoamingFsmInfo
                    ->rRoamingDiscoveryUpdateTime,
                    SEC_TO_SYSTIME(
                        ROAMING_DISCOVERY_TIMEOUT_SEC)) &&
                fgIsNeedScan) {
                DBGLOG(ROAMING,
                       LOUD,
                       "roamingFsmSteps: DiscoveryUpdateTime Timeout\n");
                aisFsmRunEventRoamingDiscovery(prAdapter, true);
            } else {
                DBGLOG(ROAMING,
                       LOUD,
                       "roamingFsmSteps: DiscoveryUpdateTime Updated\n");
                aisFsmRunEventRoamingDiscovery(prAdapter,
                                               false);
            }
        } break;

        case ROAMING_STATE_ROAM:
            break;

        default:
            ASSERT(0);  /* Make sure we have handle all STATEs */
        }
    } while (fgIsTransition);

    return;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Transit to Decision state after join completion
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void roamingFsmRunEventStart(IN P_ADAPTER_T prAdapter)
{
    P_ROAMING_INFO_T prRoamingFsmInfo;
    ENUM_ROAMING_STATE_T eNextState;
    P_BSS_INFO_T prAisBssInfo;
    CMD_ROAMING_TRANSIT_T rTransit;

    kalMemZero(&rTransit, sizeof(CMD_ROAMING_TRANSIT_T));

    prRoamingFsmInfo = (P_ROAMING_INFO_T)&
                       (prAdapter->rWifiVar.rRoamingInfo);

    /* Check Roaming Conditions */
    if (!(prRoamingFsmInfo->fgIsEnableRoaming)) {
        return;
    }

    prAisBssInfo = prAdapter->prAisBssInfo;
    if (prAisBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE) {
        return;
    }

    DBGLOG(ROAMING, EVENT, "EVENT-ROAMING START: Current Time = %ld\n",
           kalGetTimeTick());

    /* IDLE, ROAM -> DECISION */
    /* Errors as DECISION, DISCOVERY -> DECISION */
    if (!(prRoamingFsmInfo->eCurrentState == ROAMING_STATE_IDLE ||
          prRoamingFsmInfo->eCurrentState == ROAMING_STATE_ROAM)) {
        return;
    }

    eNextState = ROAMING_STATE_DECISION;
    if (eNextState != prRoamingFsmInfo->eCurrentState) {
        rTransit.u2Event = ROAMING_EVENT_START;
        rTransit.u2Data = prAisBssInfo->ucBssIndex;
        roamingFsmSendCmd(prAdapter,
                          (P_CMD_ROAMING_TRANSIT_T)&rTransit);

        /* Step to next state */
        roamingFsmSteps(prAdapter, eNextState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Transit to Discovery state when deciding to find a candidate
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void roamingFsmRunEventDiscovery(IN P_ADAPTER_T prAdapter,
                                 IN P_CMD_ROAMING_TRANSIT_T prTransit)
{
    P_ROAMING_INFO_T prRoamingFsmInfo;
    ENUM_ROAMING_STATE_T eNextState;

    prRoamingFsmInfo = (P_ROAMING_INFO_T)&
                       (prAdapter->rWifiVar.rRoamingInfo);

    /* Check Roaming Conditions */
    if (!(prRoamingFsmInfo->fgIsEnableRoaming)) {
        return;
    }

    DBGLOG(ROAMING, EVENT, "EVENT-ROAMING DISCOVERY: Current Time = %ld\n",
           kalGetTimeTick());

    /* DECISION -> DISCOVERY */
    /* Errors as IDLE, DISCOVERY, ROAM -> DISCOVERY */
    if (prRoamingFsmInfo->eCurrentState != ROAMING_STATE_DECISION) {
        return;
    }

    eNextState = ROAMING_STATE_DISCOVERY;
    /* DECISION -> DISCOVERY */
    if (eNextState != prRoamingFsmInfo->eCurrentState) {
        P_BSS_INFO_T prAisBssInfo;
        P_BSS_DESC_T prBssDesc;

        /* sync. rcpi with firmware */
        prAisBssInfo = prAdapter->prAisBssInfo;
        prBssDesc = scanSearchBssDescByBssid(prAdapter,
                                             prAisBssInfo->aucBSSID);
        if (prBssDesc) {
            prBssDesc->ucRCPI = (u8)(prTransit->u2Data & 0xff);
        }

        roamingFsmSteps(prAdapter, eNextState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Transit to Roam state after Scan Done
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void roamingFsmRunEventRoam(IN P_ADAPTER_T prAdapter)
{
    P_ROAMING_INFO_T prRoamingFsmInfo;
    ENUM_ROAMING_STATE_T eNextState;
    CMD_ROAMING_TRANSIT_T rTransit;

    kalMemZero(&rTransit, sizeof(CMD_ROAMING_TRANSIT_T));

    prRoamingFsmInfo = (P_ROAMING_INFO_T)&
                       (prAdapter->rWifiVar.rRoamingInfo);

    /* Check Roaming Conditions */
    if (!(prRoamingFsmInfo->fgIsEnableRoaming)) {
        return;
    }

    DBGLOG(ROAMING, EVENT, "EVENT-ROAMING ROAM: Current Time = %ld\n",
           kalGetTimeTick());

    /* IDLE, ROAM -> DECISION */
    /* Errors as IDLE, DECISION, ROAM -> ROAM */
    if (prRoamingFsmInfo->eCurrentState != ROAMING_STATE_DISCOVERY) {
        return;
    }

    eNextState = ROAMING_STATE_ROAM;
    /* DISCOVERY -> ROAM */
    if (eNextState != prRoamingFsmInfo->eCurrentState) {
        rTransit.u2Event = ROAMING_EVENT_ROAM;
        roamingFsmSendCmd(prAdapter,
                          (P_CMD_ROAMING_TRANSIT_T)&rTransit);

        /* Step to next state */
        roamingFsmSteps(prAdapter, eNextState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Transit to Decision state as being failed to find out any candidate
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void roamingFsmRunEventFail(IN P_ADAPTER_T prAdapter, IN u32 u4Param)
{
    P_ROAMING_INFO_T prRoamingFsmInfo;
    ENUM_ROAMING_STATE_T eNextState;
    CMD_ROAMING_TRANSIT_T rTransit;

    kalMemZero(&rTransit, sizeof(CMD_ROAMING_TRANSIT_T));
    prRoamingFsmInfo = (P_ROAMING_INFO_T)&
                       (prAdapter->rWifiVar.rRoamingInfo);

    /* Check Roaming Conditions */
    if (!(prRoamingFsmInfo->fgIsEnableRoaming)) {
        return;
    }

    DBGLOG(ROAMING, EVENT,
           "EVENT-ROAMING FAIL: reason %x Current Time = %ld\n", u4Param,
           kalGetTimeTick());

    /* IDLE, ROAM -> DECISION */
    /* Errors as IDLE, DECISION, DISCOVERY -> DECISION */
    if (prRoamingFsmInfo->eCurrentState != ROAMING_STATE_ROAM) {
        return;
    }

    eNextState = ROAMING_STATE_DECISION;
    /* ROAM -> DECISION */
    if (eNextState != prRoamingFsmInfo->eCurrentState) {
        rTransit.u2Event = ROAMING_EVENT_FAIL;
        rTransit.u2Data = (u16)(u4Param & 0xffff);
        roamingFsmSendCmd(prAdapter,
                          (P_CMD_ROAMING_TRANSIT_T)&rTransit);

        /* Step to next state */
        roamingFsmSteps(prAdapter, eNextState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Transit to Idle state as beging aborted by other moduels, AIS
 *
 * @param [IN P_ADAPTER_T] prAdapter
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void roamingFsmRunEventAbort(IN P_ADAPTER_T prAdapter)
{
    P_ROAMING_INFO_T prRoamingFsmInfo;
    ENUM_ROAMING_STATE_T eNextState;
    CMD_ROAMING_TRANSIT_T rTransit;

    kalMemZero(&rTransit, sizeof(CMD_ROAMING_TRANSIT_T));
    prRoamingFsmInfo = (P_ROAMING_INFO_T)&
                       (prAdapter->rWifiVar.rRoamingInfo);

    /* Check Roaming Conditions */
    if (!(prRoamingFsmInfo->fgIsEnableRoaming)) {
        return;
    }

    DBGLOG(ROAMING, EVENT, "EVENT-ROAMING ABORT: Current Time = %ld\n",
           kalGetTimeTick());

    eNextState = ROAMING_STATE_IDLE;
    /* IDLE, DECISION, DISCOVERY, ROAM -> IDLE */
    if (eNextState != prRoamingFsmInfo->eCurrentState) {
        rTransit.u2Event = ROAMING_EVENT_ABORT;
        roamingFsmSendCmd(prAdapter,
                          (P_CMD_ROAMING_TRANSIT_T)&rTransit);

        /* Step to next state */
        roamingFsmSteps(prAdapter, eNextState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process events from firmware
 *
 * @param [IN P_ADAPTER_T]       prAdapter
 *        [IN P_ROAMING_PARAM_T] prParam
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS roamingFsmProcessEvent(IN P_ADAPTER_T prAdapter,
                                   IN P_CMD_ROAMING_TRANSIT_T prTransit)
{
    DBGLOG(ROAMING, LOUD, "ROAMING Process Events: Current Time = %ld\n",
           kalGetTimeTick());

    if (prTransit->u2Event == ROAMING_EVENT_DISCOVERY) {
        roamingFsmRunEventDiscovery(prAdapter, prTransit);
    }

    return WLAN_STATUS_SUCCESS;
}

#endif
