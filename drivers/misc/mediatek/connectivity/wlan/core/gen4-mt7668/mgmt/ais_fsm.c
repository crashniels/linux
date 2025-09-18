// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "aa_fsm.c"
 *    \brief  This file defines the FSM for SAA and AAA MODULE.
 *
 *    This file defines the FSM for SAA and AAA MODULE.
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

#define AIS_ROAMING_CONNECTION_TRIAL_LIMIT 2
#define AIS_JOIN_TIMEOUT 7

#define AIS_FSM_STATE_SEARCH_ACTION_PHASE_0 0
#define AIS_FSM_STATE_SEARCH_ACTION_PHASE_1 1
#define AIS_FSM_STATE_SEARCH_ACTION_PHASE_2 2

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
static u8 *apucDebugAisState[AIS_STATE_NUM] = {
    (u8 *)DISP_STRING("IDLE"),
    (u8 *)DISP_STRING("SEARCH"),
    (u8 *)DISP_STRING("SCAN"),
    (u8 *)DISP_STRING("ONLINE_SCAN"),
    (u8 *)DISP_STRING("LOOKING_FOR"),
    (u8 *)DISP_STRING("WAIT_FOR_NEXT_SCAN"),
    (u8 *)DISP_STRING("REQ_CHANNEL_JOIN"),
    (u8 *)DISP_STRING("JOIN"),
    (u8 *)DISP_STRING("JOIN_FAILURE"),
    (u8 *)DISP_STRING("IBSS_ALONE"),
    (u8 *)DISP_STRING("IBSS_MERGE"),
    (u8 *)DISP_STRING("NORMAL_TR"),
    (u8 *)DISP_STRING("DISCONNECTING"),
    (u8 *)DISP_STRING("REQ_REMAIN_ON_CHANNEL"),
    (u8 *)DISP_STRING("REMAIN_ON_CHANNEL")
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

static void aisFsmRunEventScanDoneTimeOut(IN P_ADAPTER_T prAdapter,
                                          unsigned long ulParam);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * @brief the function is used to initialize the value of the connection
 * settings for AIS network
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisInitializeConnectionSettings(IN P_ADAPTER_T prAdapter,
                                     IN P_REG_INFO_T prRegInfo)
{
    P_CONNECTION_SETTINGS_T prConnSettings;
    u8 aucAnyBSSID[] = BC_BSSID;
    u8 aucZeroMacAddr[] = NULL_MAC_ADDR;
    int i = 0;

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    /* Setup default values for operation */
    COPY_MAC_ADDR(prConnSettings->aucMacAddress, aucZeroMacAddr);

    prConnSettings->ucDelayTimeOfDisconnectEvent =
        prAdapter->rWifiVar.ucDelayTimeOfDisconnect;

    COPY_MAC_ADDR(prConnSettings->aucBSSID, aucAnyBSSID);
    prConnSettings->fgIsConnByBssidIssued = false;

    prConnSettings->fgIsConnReqIssued = false;
    prConnSettings->fgIsDisconnectedByNonRequest = false;

    prConnSettings->ucSSIDLen = 0;

    prConnSettings->eOPMode = NET_TYPE_INFRA;

    prConnSettings->eConnectionPolicy = CONNECT_BY_SSID_BEST_RSSI;

    if (prRegInfo) {
        prConnSettings->ucAdHocChannelNum =
            (u8)nicFreq2ChannelNum(prRegInfo->u4StartFreq);
        prConnSettings->eAdHocBand =
            prRegInfo->u4StartFreq < 5000000 ? BAND_2G4 : BAND_5G;
        prConnSettings->eAdHocMode =
            (ENUM_PARAM_AD_HOC_MODE_T)(prRegInfo->u4AdhocMode);
    }

    prConnSettings->eAuthMode = AUTH_MODE_OPEN;

    prConnSettings->eEncStatus = ENUM_ENCRYPTION_DISABLED;

    prConnSettings->fgIsScanReqIssued = false;

    /* MIB attributes */
    prConnSettings->u2BeaconPeriod = DOT11_BEACON_PERIOD_DEFAULT;

    prConnSettings->u2RTSThreshold = DOT11_RTS_THRESHOLD_DEFAULT;

    prConnSettings->u2DesiredNonHTRateSet = RATE_SET_ALL_ABG;

    /* prConnSettings->u4FreqInKHz; */ /* Center frequency */

    /* Set U-APSD AC */
    prConnSettings->bmfgApsdEnAc = PM_UAPSD_NONE;

    secInit(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

    /* Features */
    prConnSettings->fgIsEnableRoaming = false;
#if CFG_SUPPORT_ROAMING
    if (prAdapter->rWifiVar.fgDisRoaming) {
        prConnSettings->fgIsEnableRoaming = false;
    } else {
        prConnSettings->fgIsEnableRoaming = true;
    }
#endif

    prConnSettings->fgIsAdHocQoSEnable = false;

#if CFG_SUPPORT_802_11AC
    prConnSettings->eDesiredPhyConfig = PHY_CONFIG_802_11ABGNAC;
#else
    prConnSettings->eDesiredPhyConfig = PHY_CONFIG_802_11ABGN;
#endif

    /* Set default bandwidth modes */
    prConnSettings->uc2G4BandwidthMode = CONFIG_BW_20M;
    prConnSettings->uc5GBandwidthMode = CONFIG_BW_20_40M;

    prConnSettings->rRsnInfo.ucElemId = 0x30;
    prConnSettings->rRsnInfo.u2Version = 0x0001;
    prConnSettings->rRsnInfo.u4GroupKeyCipherSuite = 0;
    prConnSettings->rRsnInfo.u4GroupMgmtKeyCipherSuite = 0;
    prConnSettings->rRsnInfo.u4PairwiseKeyCipherSuiteCount = 0;
    for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
        prConnSettings->rRsnInfo.au4PairwiseKeyCipherSuite[i] = 0;
    prConnSettings->rRsnInfo.u4AuthKeyMgtSuiteCount = 0;
    for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++)
        prConnSettings->rRsnInfo.au4AuthKeyMgtSuite[i] = 0;
    prConnSettings->rRsnInfo.u2RsnCap = 0;
    prConnSettings->rRsnInfo.fgRsnCapPresent = false;
    prConnSettings->rRsnInfo.u2PmkidCnt = 0;
    kalMemZero(prConnSettings->rRsnInfo.aucPmkidList,
               (sizeof(u8) * MAX_NUM_SUPPORTED_PMKID * RSN_PMKID_LEN));
    prConnSettings->bss = NULL;

#if CFG_SUPPORT_OWE
    kalMemSet(&prConnSettings->rOweInfo, 0, sizeof(struct OWE_INFO_T));
#endif
#if CFG_SUPPORT_H2E
    kalMemSet(&prConnSettings->rRsnXE, 0, sizeof(struct RSNXE));
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief the function is used to initialize the value in AIS_FSM_INFO_T for
 *        AIS FSM operation
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmInit(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_INFO_T prAisBssInfo;
    P_AIS_SPECIFIC_BSS_INFO_T prAisSpecificBssInfo;
    u8 i;

    DEBUGFUNC("aisFsmInit()");
    DBGLOG(SW1, INFO, "->aisFsmInit()\n");

    /* avoid that the prAisBssInfo is realloc */
    if (prAdapter->prAisBssInfo != NULL) {
        return;
    }

    prAdapter->prAisBssInfo = prAisBssInfo =
        cnmGetBssInfoAndInit(prAdapter, NETWORK_TYPE_AIS, false);
    ASSERT(prAisBssInfo);

    /* update MAC address */
    COPY_MAC_ADDR(prAdapter->prAisBssInfo->aucOwnMacAddr,
                  prAdapter->rMyMacAddr);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisSpecificBssInfo = &(prAdapter->rWifiVar.rAisSpecificBssInfo);

    /* 4 <1> Initiate FSM */
    prAisFsmInfo->ePreviousState = AIS_STATE_IDLE;
    prAisFsmInfo->eCurrentState = AIS_STATE_IDLE;

    prAisFsmInfo->ucAvailableAuthTypes = 0;

    prAisFsmInfo->prTargetBssDesc = (P_BSS_DESC_T)NULL;

    prAisFsmInfo->ucSeqNumOfReqMsg = 0;
    prAisFsmInfo->ucSeqNumOfChReq = 0;
    prAisFsmInfo->ucSeqNumOfScanReq = 0;

    prAisFsmInfo->fgIsInfraChannelFinished = true;
#if CFG_SUPPORT_ROAMING
    prAisFsmInfo->fgIsRoamingScanPending = false;
#endif
    prAisFsmInfo->fgIsChannelRequested = false;
    prAisFsmInfo->fgIsChannelGranted = false;
    prAisFsmInfo->fgIsScanOidAborted = false;

    /* 4 <1.1> Initiate FSM - Timer INIT */
    cnmTimerInitTimer(prAdapter, &prAisFsmInfo->rBGScanTimer,
                      (PFN_MGMT_TIMEOUT_FUNC)aisFsmRunEventBGSleepTimeOut,
                      (unsigned long)NULL);

    cnmTimerInitTimer(prAdapter, &prAisFsmInfo->rBeaconLostTimer,
                      (PFN_MGMT_TIMEOUT_FUNC)aisFsmBeaconLostTimeOut,
                      (unsigned long)NULL);

    cnmTimerInitTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer,
                      (PFN_MGMT_TIMEOUT_FUNC)aisFsmRunEventIbssAloneTimeOut,
                      (unsigned long)NULL);

    cnmTimerInitTimer(prAdapter, &prAisFsmInfo->rIndicationOfDisconnectTimer,
                      (PFN_MGMT_TIMEOUT_FUNC)aisPostponedEventOfDisconnTimeout,
                      (unsigned long)NULL);

    cnmTimerInitTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer,
                      (PFN_MGMT_TIMEOUT_FUNC)aisFsmRunEventScanDoneTimeOut,
                      (unsigned long)NULL);

    cnmTimerInitTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer,
                      (PFN_MGMT_TIMEOUT_FUNC)aisFsmRunEventJoinTimeout,
                      (unsigned long)NULL);

    cnmTimerInitTimer(prAdapter, &prAisFsmInfo->rDeauthDoneTimer,
                      (PFN_MGMT_TIMEOUT_FUNC)aisFsmRunEventDeauthTimeout,
                      (unsigned long)NULL);

    /* 4 <1.2> Initiate PWR STATE */
    SET_NET_PWR_STATE_IDLE(prAdapter, prAisBssInfo->ucBssIndex);

    /* 4 <2> Initiate BSS_INFO_T - common part */
    BSS_INFO_INIT(prAdapter, prAisBssInfo);
    COPY_MAC_ADDR(prAisBssInfo->aucOwnMacAddr,
                  prAdapter->rWifiVar.aucMacAddress);

    /* 4 <3> Initiate BSS_INFO_T - private part */
    /* TODO */
    prAisBssInfo->eBand = BAND_2G4;
    prAisBssInfo->ucPrimaryChannel = 1;
    prAisBssInfo->prStaRecOfAP = (P_STA_RECORD_T)NULL;
    prAisBssInfo->ucNss =
        wlanGetSupportNss(prAdapter, prAisBssInfo->ucBssIndex);
    prAisBssInfo->eDBDCBand = ENUM_BAND_0;
#if (CFG_HW_WMM_BY_BSS == 0)
    prAisBssInfo->ucWmmQueSet =
        (prAdapter->rWifiVar.ucDbdcMode == DBDC_MODE_DISABLED) ?
        DBDC_5G_WMM_INDEX :
        DBDC_2G_WMM_INDEX;
#endif

    /* 4 <4> Allocate MSDU_INFO_T for Beacon */
    prAisBssInfo->prBeacon = cnmMgtPktAlloc(
        prAdapter,
        OFFSET_OF(WLAN_BEACON_FRAME_T, aucInfoElem[0]) + MAX_IE_LENGTH);

    if (prAisBssInfo->prBeacon) {
        prAisBssInfo->prBeacon->eSrc = TX_PACKET_MGMT;
        prAisBssInfo->prBeacon->ucStaRecIndex = 0xFF;  /* NULL STA_REC */
    } else {
        ASSERT(0);
    }

    prAisBssInfo->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;

    for (i = 0; i < MAX_KEY_NUM; i++) {
        prAisBssInfo->ucBMCWlanIndexS[i] = WTBL_RESERVED_ENTRY;
        prAisBssInfo->ucBMCWlanIndexSUsed[i] = false;
        prAisBssInfo->wepkeyUsed[i] = false;
    }

    if (prAdapter->u4UapsdAcBmp == 0) {
        prAdapter->u4UapsdAcBmp = CFG_INIT_UAPSD_AC_BMP;
        /* ASSERT(prAdapter->u4UapsdAcBmp); */
    }
    prAisBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC =
        (u8)prAdapter->u4UapsdAcBmp;
    prAisBssInfo->rPmProfSetupInfo.ucBmpTriggerAC = (u8)prAdapter->u4UapsdAcBmp;
    prAisBssInfo->rPmProfSetupInfo.ucUapsdSp = (u8)prAdapter->u4MaxSpLen;

    /* request list initialization */
    LINK_INITIALIZE(&prAisFsmInfo->rPendingReqList);

    LINK_MGMT_INIT(&prAisSpecificBssInfo->rNeighborApList);
#if CFG_SUPPORT_802_11V
    kalMemZero(&prAisSpecificBssInfo->rBTMParam,
               sizeof(prAisSpecificBssInfo->rBTMParam));
#endif
#if CFG_SUPPORT_802_11W
    init_completion(&prAisBssInfo->rDeauthComp);
    prAisBssInfo->encryptedDeauthIsInProcess = false;
#endif
    /* DBGPRINTF("[2] ucBmpDeliveryAC:0x%x, ucBmpTriggerAC:0x%x,
     * ucUapsdSp:0x%x", */
    /* prAisBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC, */
    /* prAisBssInfo->rPmProfSetupInfo.ucBmpTriggerAC, */
    /* prAisBssInfo->rPmProfSetupInfo.ucUapsdSp); */

    /* Bind NetDev & BssInfo */
    /* wlanBindBssIdxToNetInterface(prAdapter->prGlueInfo, NET_DEV_WLAN_IDX,
     * prAisBssInfo->ucBssIndex); */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief the function is used to uninitialize the value in AIS_FSM_INFO_T for
 *        AIS FSM operation
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmUninit(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_INFO_T prAisBssInfo;
    P_AIS_SPECIFIC_BSS_INFO_T prAisSpecificBssInfo;

    DEBUGFUNC("aisFsmUninit()");
    DBGLOG(SW1, INFO, "->aisFsmUninit()\n");

    /* avoid that the prAisBssInfo is double freed */
    if (prAdapter->prAisBssInfo == NULL) {
        return;
    }

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;
    prAisSpecificBssInfo = &(prAdapter->rWifiVar.rAisSpecificBssInfo);

    /* 4 <1> Stop all timers */
    cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rBGScanTimer);
    cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rBeaconLostTimer);
    cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer);
    cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rIndicationOfDisconnectTimer);
    cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);
    cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);

    /* 4 <2> flush pending request */
    aisFsmFlushRequest(prAdapter);

    /* 4 <3> Reset driver-domain BSS-INFO */
    if (prAisBssInfo) {
        if (prAisBssInfo->prBeacon) {
            cnmMgtPktFree(prAdapter, prAisBssInfo->prBeacon);
            prAisBssInfo->prBeacon = NULL;
        }

        cnmFreeBssInfo(prAdapter, prAisBssInfo);
        prAdapter->prAisBssInfo = NULL;
    }

#if CFG_SUPPORT_802_11W
    rsnStopSaQuery(prAdapter);
#endif
    /* end Support AP Selection */
    LINK_MGMT_UNINIT(&prAisSpecificBssInfo->rNeighborApList, NEIGHBOR_AP_T,
                     VIR_MEM_TYPE);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialization of JOIN STATE
 *
 * @param[in] prBssDesc  The pointer of BSS_DESC_T which is the BSS we will try
 * to join with.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateInit_JOIN(IN P_ADAPTER_T prAdapter, P_BSS_DESC_T prBssDesc)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_INFO_T prAisBssInfo;
    P_AIS_SPECIFIC_BSS_INFO_T prAisSpecificBssInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_STA_RECORD_T prStaRec;
    P_MSG_JOIN_REQ_T prJoinReqMsg;

    DEBUGFUNC("aisFsmStateInit_JOIN()");

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;
    prAisSpecificBssInfo = &(prAdapter->rWifiVar.rAisSpecificBssInfo);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    ASSERT(prBssDesc);

    /* 4 <1> We are going to connect to this BSS. */
    prBssDesc->fgIsConnecting = true;

    /* 4 <2> Setup corresponding STA_RECORD_T */
    prStaRec = bssCreateStaRecFromBssDesc(prAdapter, STA_TYPE_LEGACY_AP,
                                          prAdapter->prAisBssInfo->ucBssIndex,
                                          prBssDesc);

    prAisFsmInfo->prTargetStaRec = prStaRec;

    /* 4 <2.1> sync. to firmware domain */
    /* 20200622 frog: Always sync STATE 1 to FW.
     *    This is for same STA reassoc case.
     *    STA need stop TX/RX and back to STATE_1 when auth/reassoc.
     */
    cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

    /* 4 <3> Update ucAvailableAuthTypes which we can choice during SAA */
    if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED) {
        prStaRec->fgIsReAssoc = false;

        /*Fill Auth Type */
        prAisFsmInfo->ucAvailableAuthTypes =
            (u8)prAdapter->prGlueInfo->rWpaInfo.u4AuthAlg;
        DBGLOG(AIS, INFO, "JOIN INIT: Auth Algorithm :%d\n",
               prAisFsmInfo->ucAvailableAuthTypes);
        /* TODO(tyhsu): Assume that Roaming Auth Type is equal to
         * ConnSettings eAuthMode */
        prAisSpecificBssInfo->ucRoamingAuthTypes =
            prAisFsmInfo->ucAvailableAuthTypes;

        prStaRec->ucTxAuthAssocRetryLimit = TX_AUTH_ASSOCI_RETRY_LIMIT;

        /* Update Bss info before join */
        prAisBssInfo->eBand = prBssDesc->eBand;
        prAisBssInfo->ucPrimaryChannel = prBssDesc->ucChannelNum;
    } else {
        ASSERT(prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE);
        DBGLOG(AIS, LOUD, "JOIN INIT: AUTH TYPE = %d for Roaming\n",
               prAisSpecificBssInfo->ucRoamingAuthTypes);

        prStaRec->fgIsReAssoc = true;  /* We do roaming while the medium
                                          is connected */
        prAisFsmInfo->ucAvailableAuthTypes =
            (u8)prAdapter->prGlueInfo->rWpaInfo.u4AuthAlg;

        DBGLOG(AIS, INFO, "JOIN INIT: Auth Algorithm for Roaming:%d\n",
               prAisFsmInfo->ucAvailableAuthTypes);
        prStaRec->ucTxAuthAssocRetryLimit =
            TX_AUTH_ASSOCI_RETRY_LIMIT_FOR_ROAMING;
    }

    /* 4 <4> Use an appropriate Authentication Algorithm Number among the
     * ucAvailableAuthTypes */
    if (prAisFsmInfo->ucAvailableAuthTypes & (u8)AUTH_TYPE_OPEN_SYSTEM) {
        DBGLOG(
            AIS, LOUD,
            "JOIN INIT: Try to do Authentication with AuthType == OPEN_SYSTEM.\n");
        prAisFsmInfo->ucAvailableAuthTypes &= ~(u8)AUTH_TYPE_OPEN_SYSTEM;

        prStaRec->ucAuthAlgNum = (u8)AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
    } else if (prAisFsmInfo->ucAvailableAuthTypes & (u8)AUTH_TYPE_SHARED_KEY) {
        DBGLOG(
            AIS, LOUD,
            "JOIN INIT: Try to do Authentication with AuthType == SHARED_KEY.\n");

        prAisFsmInfo->ucAvailableAuthTypes &= ~(u8)AUTH_TYPE_SHARED_KEY;

        prStaRec->ucAuthAlgNum = (u8)AUTH_ALGORITHM_NUM_SHARED_KEY;
    } else if (prAisFsmInfo->ucAvailableAuthTypes &
               (u8)AUTH_TYPE_FAST_BSS_TRANSITION) {
        DBGLOG(AIS, LOUD,
               "JOIN INIT: Try to do Authentication with AuthType == "
               "FAST_BSS_TRANSITION.\n");

        prAisFsmInfo->ucAvailableAuthTypes &=
            ~(u8)AUTH_TYPE_FAST_BSS_TRANSITION;

        prStaRec->ucAuthAlgNum = (u8)AUTH_ALGORITHM_NUM_FAST_BSS_TRANSITION;
#if CFG_SUPPORT_SAE
    } else if (prAisFsmInfo->ucAvailableAuthTypes & (u8)AUTH_TYPE_SAE) {
        DBGLOG(AIS, LOUD,
               "JOIN INIT: Try to do Authentication with AuthType == SAE.\n");

        prAisFsmInfo->ucAvailableAuthTypes &= ~(u8)AUTH_TYPE_SAE;

        prStaRec->ucAuthAlgNum = (u8)AUTH_ALGORITHM_NUM_SAE;
#endif
    } else {
        ASSERT(0);
    }

    /* 4 <5> Overwrite Connection Setting for eConnectionPolicy == ANY (Used
     * by Assoc Req) */
    if (prBssDesc->ucSSIDLen) {
        COPY_SSID(prConnSettings->aucSSID, prConnSettings->ucSSIDLen,
                  prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
    }
    /* 4 <6> Send a Msg to trigger SAA to start JOIN process. */
    prJoinReqMsg = (P_MSG_JOIN_REQ_T)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                                 sizeof(MSG_JOIN_REQ_T));
    if (!prJoinReqMsg) {
        ASSERT(0);  /* Can't trigger SAA FSM */
        return;
    }

    prJoinReqMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_START;
    prJoinReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfReqMsg;
    prJoinReqMsg->prStaRec = prStaRec;

    nicRxClearFrag(prAdapter, prStaRec);
    prConnSettings->fgIsConnInitialized = true;

    mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prJoinReqMsg,
                MSG_SEND_METHOD_BUF);
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
u8 aisFsmStateInit_RetryJOIN(IN P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_MSG_JOIN_REQ_T prJoinReqMsg;

    DEBUGFUNC("aisFsmStateInit_RetryJOIN()");

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    /* Retry other AuthType if possible */
    if (!prAisFsmInfo->ucAvailableAuthTypes) {
        return false;
    }

    if (prAisFsmInfo->ucAvailableAuthTypes & (u8)AUTH_TYPE_SHARED_KEY) {
        DBGLOG(
            AIS, INFO,
            "RETRY JOIN INIT: Retry Authentication with AuthType == SHARED_KEY.\n");

        prAisFsmInfo->ucAvailableAuthTypes &= ~(u8)AUTH_TYPE_SHARED_KEY;

        prStaRec->ucAuthAlgNum = (u8)AUTH_ALGORITHM_NUM_SHARED_KEY;
    } else {
        DBGLOG(
            AIS, ERROR,
            "RETRY JOIN INIT: Retry Authentication with Unexpected AuthType.\n");
        ASSERT(0);
    }

    prAisFsmInfo->ucAvailableAuthTypes = 0;  /* No more available Auth Types
                                              */

    /* Trigger SAA to start JOIN process. */
    prJoinReqMsg = (P_MSG_JOIN_REQ_T)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                                 sizeof(MSG_JOIN_REQ_T));
    if (!prJoinReqMsg) {
        ASSERT(0);  /* Can't trigger SAA FSM */
        return false;
    }

    prJoinReqMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_START;
    prJoinReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfReqMsg;
    prJoinReqMsg->prStaRec = prStaRec;

    mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prJoinReqMsg,
                MSG_SEND_METHOD_BUF);

    return true;
}

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
 * @brief State Initialization of AIS_STATE_IBSS_ALONE
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateInit_IBSS_ALONE(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_BSS_INFO_T prAisBssInfo;

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prAisBssInfo = prAdapter->prAisBssInfo;

    /* 4 <1> Check if IBSS was created before ? */
    if (prAisBssInfo->fgIsBeaconActivated) {
        /* 4 <2> Start IBSS Alone Timer for periodic SCAN and then
         * SEARCH */
        cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer,
                           SEC_TO_MSEC(AIS_IBSS_ALONE_TIMEOUT_SEC));
    }

    aisFsmCreateIBSS(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief State Initialization of AIS_STATE_IBSS_MERGE
 *
 * @param[in] prBssDesc  The pointer of BSS_DESC_T which is the IBSS we will try
 * to merge with.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateInit_IBSS_MERGE(IN P_ADAPTER_T prAdapter,
                                P_BSS_DESC_T prBssDesc)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_BSS_INFO_T prAisBssInfo;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;

    ASSERT(prBssDesc);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prAisBssInfo = prAdapter->prAisBssInfo;

    /* 4 <1> We will merge with to this BSS immediately. */
    prBssDesc->fgIsConnecting = false;
    prBssDesc->fgIsConnected = true;

    /* 4 <2> Setup corresponding STA_RECORD_T */
    prStaRec = bssCreateStaRecFromBssDesc(prAdapter, STA_TYPE_ADHOC_PEER,
                                          prAdapter->prAisBssInfo->ucBssIndex,
                                          prBssDesc);

    prStaRec->fgIsMerging = true;

    prAisFsmInfo->prTargetStaRec = prStaRec;

    /* 4 <2.1> sync. to firmware domain */
    cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

    /* 4 <3> IBSS-Merge */
    aisFsmMergeIBSS(prAdapter, prStaRec);
}

#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process of JOIN Abort
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_JOIN(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_MSG_JOIN_ABORT_T prJoinAbortMsg;

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    /* 1. Abort JOIN process */
    prJoinAbortMsg = (P_MSG_JOIN_ABORT_T)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                                     sizeof(MSG_JOIN_ABORT_T));
    if (!prJoinAbortMsg) {
        ASSERT(0);  /* Can't abort SAA FSM */
        return;
    }

    prJoinAbortMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_ABORT;
    prJoinAbortMsg->ucSeqNum = prAisFsmInfo->ucSeqNumOfReqMsg;
    prJoinAbortMsg->prStaRec = prAisFsmInfo->prTargetStaRec;

    scanRemoveConnFlagOfBssDescByBssid(
        prAdapter, prAisFsmInfo->prTargetStaRec->aucMacAddr);

    mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prJoinAbortMsg,
                MSG_SEND_METHOD_BUF);

    /* 2. Return channel privilege */
    aisFsmReleaseCh(prAdapter);

    /* 3.1 stop join timeout timer */
    cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);

    /* 3.2 reset local variable */
    prAisFsmInfo->fgIsInfraChannelFinished = true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process of SCAN Abort
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_SCAN(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_MSG_SCN_SCAN_CANCEL prScanCancelMsg;

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    DBGLOG(AIS, STATE, "aisFsmStateAbort_SCAN\n");

    /* Abort JOIN process. */
    prScanCancelMsg = (P_MSG_SCN_SCAN_CANCEL)cnmMemAlloc(
        prAdapter, RAM_TYPE_MSG, sizeof(MSG_SCN_SCAN_CANCEL));
    if (!prScanCancelMsg) {
        ASSERT(0);  /* Can't abort SCN FSM */
        return;
    }

    prScanCancelMsg->rMsgHdr.eMsgId = MID_AIS_SCN_SCAN_CANCEL;
    prScanCancelMsg->ucSeqNum = prAisFsmInfo->ucSeqNumOfScanReq;
    prScanCancelMsg->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
    prScanCancelMsg->fgIsChannelExt = false;
    if (prAisFsmInfo->fgIsScanOidAborted) {
        prScanCancelMsg->fgIsOidRequest = true;
        prAisFsmInfo->fgIsScanOidAborted = false;
    }
    /* unbuffered message to guarantee scan is cancelled in sequence */
    mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prScanCancelMsg,
                MSG_SEND_METHOD_UNBUF);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process of NORMAL_TR Abort
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_NORMAL_TR(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;

    ASSERT(prAdapter);
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    /* TODO(Kevin): Do abort other MGMT func */

    /* 1. Release channel to CNM */
    aisFsmReleaseCh(prAdapter);

    /* 2.1 stop join timeout timer */
    cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);

    /* 2.2 reset local variable */
    prAisFsmInfo->fgIsInfraChannelFinished = true;
}

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
 * @brief Process of NORMAL_TR Abort
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_IBSS(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_DESC_T prBssDesc;

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    /* reset BSS-DESC */
    if (prAisFsmInfo->prTargetStaRec) {
        prBssDesc = scanSearchBssDescByTA(
            prAdapter, prAisFsmInfo->prTargetStaRec->aucMacAddr);

        if (prBssDesc) {
            prBssDesc->fgIsConnected = false;
            prBssDesc->fgIsConnecting = false;
        }
    }
    /* release channel privilege */
    aisFsmReleaseCh(prAdapter);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief The Core FSM engine of AIS(Ad-hoc, Infra STA)
 *
 * @param[in] eNextState Enum value of next AIS STATE
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmSteps(IN P_ADAPTER_T prAdapter, ENUM_AIS_STATE_T eNextState)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_INFO_T prAisBssInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_BSS_DESC_T prBssDesc;
    P_MSG_CH_REQ_T prMsgChReq;
    P_MSG_SCN_SCAN_REQ_V2 prScanReqMsg;
    P_AIS_REQ_HDR_T prAisReq;
    ENUM_BAND_T eBand;
    u8 ucChannel;
    u16 u2ScanIELen;
    u8 fgIsTransition = (u8)false;
#if CFG_SUPPORT_DBDC
    CNM_DBDC_CAP_T rDbdcCap;
#if CFG_SUPPORT_DBDC_TC6
    u8 ucBssIndex;
    P_BSS_INFO_T prBssInfo;
    u8 fgDiffBandExist = false;
#endif
#endif

    DEBUGFUNC("aisFsmSteps()");

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    do {
        /* Do entering Next State */
        prAisFsmInfo->ePreviousState = prAisFsmInfo->eCurrentState;

        // DBGLOG(AIS, STATE, "[AIS]TRANSITION: [%s] -> [%s]\n",
        //        apucDebugAisState[prAisFsmInfo->eCurrentState],
        //        apucDebugAisState[eNextState]);

        /* NOTE(Kevin): This is the only place to change the
         * eCurrentState(except initial) */
        prAisFsmInfo->eCurrentState = eNextState;

        fgIsTransition = (u8)false;

        /* Do tasks of the State that we just entered */
        switch (prAisFsmInfo->eCurrentState) {
        /* NOTE(Kevin): we don't have to rearrange the sequence of
         * following switch case. Instead I would like to use a common
         * lookup table of array of function pointer to speed up state
         * search.
         */
        case AIS_STATE_IDLE:
            if (prAisFsmInfo->ePreviousState != prAisFsmInfo->eCurrentState) {
                prConnSettings->fgIsConnInitialized = false;
            }

            prAisReq = aisFsmGetNextRequest(prAdapter);
            cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);
            cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rBeaconLostTimer);

            if (prAisReq == NULL ||
                prAisReq->eReqType == AIS_REQUEST_RECONNECT) {
                if (prConnSettings->fgIsConnReqIssued == true &&
                    prConnSettings->fgIsDisconnectedByNonRequest == false) {
                    prAisFsmInfo->fgTryScan = true;

                    if (!IS_NET_ACTIVE(prAdapter,
                                       prAdapter->prAisBssInfo->ucBssIndex)) {
                        SET_NET_ACTIVE(prAdapter,
                                       prAdapter->prAisBssInfo->ucBssIndex);
                        /* sync with firmware */
                        nicActivateNetwork(prAdapter,
                                           prAdapter->prAisBssInfo->ucBssIndex);
                    }

                    SET_NET_PWR_STATE_ACTIVE(
                        prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

                    prAisBssInfo->fgIsNetRequestInActive = false;

                    /* reset trial count */
                    prAisFsmInfo->ucConnTrialCount = 0;

                    eNextState = AIS_STATE_SEARCH;
                    fgIsTransition = true;
                } else {
                    SET_NET_PWR_STATE_IDLE(prAdapter,
                                           prAdapter->prAisBssInfo->ucBssIndex);

                    /* sync with firmware */
                    prAisBssInfo->fgIsNetRequestInActive = true;
                    if (prAisBssInfo->fgIsPNOEnable) {
                        DBGLOG(
                            BSS, INFO,
                            "[BSSidx][Network]=%d "
                            "PNOEnable&&OP_MODE_INFRASTRUCTURE,KEEP ACTIVE\n",
                            prAisBssInfo->ucBssIndex);
                    } else {
                        UNSET_NET_ACTIVE(prAdapter,
                                         prAdapter->prAisBssInfo->ucBssIndex);
                        nicDeactivateNetwork(
                            prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
                    }

                    /* check for other pending request */
                    if (prAisReq &&
                        (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN,
                                                true) == true)) {
                        wlanClearScanningResult(prAdapter);
                        eNextState = AIS_STATE_SCAN;

                        fgIsTransition = true;
                    }
                }

                if (prAisReq) {
                    /* free the message */
                    cnmMemFree(prAdapter, prAisReq);
                }
            } else if (prAisReq->eReqType == AIS_REQUEST_SCAN) {
#if CFG_SUPPORT_ROAMING
                prAisFsmInfo->fgIsRoamingScanPending = false;
#endif
                wlanClearScanningResult(prAdapter);

                eNextState = AIS_STATE_SCAN;
                fgIsTransition = true;

                /* free the message */
                cnmMemFree(prAdapter, prAisReq);
            } else if (prAisReq->eReqType == AIS_REQUEST_ROAMING_CONNECT ||
                       prAisReq->eReqType == AIS_REQUEST_ROAMING_SEARCH) {
                /* ignore */
                /* free the message */
                cnmMemFree(prAdapter, prAisReq);
            } else if (prAisReq->eReqType == AIS_REQUEST_REMAIN_ON_CHANNEL) {
                eNextState = AIS_STATE_REQ_REMAIN_ON_CHANNEL;
                fgIsTransition = true;

                /* free the message */
                cnmMemFree(prAdapter, prAisReq);
            }

            prAisFsmInfo->u4SleepInterval = AIS_BG_SCAN_INTERVAL_MIN_SEC;

            break;

        case AIS_STATE_SEARCH:
            /* 4 <1> Search for a matched candidate and save it to
             * prTargetBssDesc. */
            prBssDesc = scanSearchBssDescByPolicy(
                prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

            /* we are under Roaming Condition. */
            if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
                if (prAisFsmInfo->ucConnTrialCount >
                    AIS_ROAMING_CONNECTION_TRIAL_LIMIT) {
#if CFG_SUPPORT_ROAMING
                    roamingFsmRunEventFail(prAdapter,
                                           ROAMING_FAIL_REASON_CONNLIMIT);
#endif
                    /* reset retry count */
                    prAisFsmInfo->ucConnTrialCount = 0;

                    /* abort connection trial */
                    prConnSettings->fgIsConnReqIssued = false;
                    eNextState = AIS_STATE_NORMAL_TR;
                    fgIsTransition = true;

                    break;
                }
            }
            /* 4 <2> We are not under Roaming Condition. */
            if (prAisBssInfo->eConnectionState ==
                PARAM_MEDIA_STATE_DISCONNECTED) {
                /* 4 <2.a> If we have the matched one */
                if (prBssDesc) {
                    /* 4 <A> Stored the Selected BSS
                     * security cipher. */
                    /* or later asoc req compose IE */
                    prAisBssInfo->u4RsnSelectedGroupCipher =
                        prBssDesc->u4RsnSelectedGroupCipher;
                    prAisBssInfo->u4RsnSelectedPairwiseCipher =
                        prBssDesc->u4RsnSelectedPairwiseCipher;
                    prAisBssInfo->u4RsnSelectedAKMSuite =
                        prBssDesc->u4RsnSelectedAKMSuite;
#if (CFG_HW_WMM_BY_BSS == 1)
                    prAisBssInfo->eBand = prBssDesc->eBand;
                    if (prAisBssInfo->fgIsWmmInited == false) {
                        prAisBssInfo->ucWmmQueSet =
                            cnmWmmIndexDecision(prAdapter, prAisBssInfo);
                    }
#endif
#if CFG_SUPPORT_DBDC
#if CFG_SUPPORT_DBDC_TC6
                    /* DBDC case: SAP exists, but STA is
                     * going to connect to AP check current
                     * existing BSS to decide enable DBDC or
                     * not
                     */
                    fgDiffBandExist = false;

                    for (ucBssIndex = 0; ucBssIndex < HW_BSSID_NUM;
                         ucBssIndex++) {
                        prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

                        if (!prBssInfo->fgIsInUse ||
                            !prBssInfo->fgIsNetActive ||
                            (prBssInfo->eConnectionState !=
                             PARAM_MEDIA_STATE_CONNECTED &&
                             prBssInfo->eCurrentOPMode !=
                             OP_MODE_ACCESS_POINT)) {
                            continue;
                        }

                        if (prBssInfo->eBand != BAND_2G4 &&
                            prBssInfo->eBand != BAND_5G) {
                            continue;
                        }

                        if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT &&
                            prBssInfo->eBand != prBssDesc->eBand) {
                            fgDiffBandExist = true;
                            DBGLOG(
                                CNM, INFO,
                                "[DBDC] GO/SAP[%d] exists on different band[%d] \n",
                                prBssInfo->ucBssIndex, prBssInfo->eBand);

                            /* Planing to Enable
                             * DBDC */
                            if (timerPendingTimer(
                                    &prAdapter->rWifiVar
                                    .rDBDCDisableCountdownTimer)) {
                                cnmTimerStopTimer(
                                    prAdapter,
                                    &prAdapter->rWifiVar
                                    .rDBDCDisableCountdownTimer);
                            }

                            /* only stop pening
                             * Switch Guard Timer
                             * when DBDC is being
                             * disabled */
                            if (timerPendingTimer(
                                    &prAdapter->rWifiVar.rDBDCSwitchGuardTimer)
                                &&
                                !prAdapter->rWifiVar.fgDbDcModeEn) {
                                cnmTimerStopTimer(
                                    prAdapter,
                                    &prAdapter->rWifiVar.rDBDCSwitchGuardTimer);
                            }

                            if (!prAdapter->rWifiVar.fgDbDcModeEn) {
                                /* Enable DBDC
                                 * only when
                                 * DBDC is
                                 * disable Do
                                 * nothing if
                                 * DBDC is
                                 * already
                                 * enable
                                 */
                                cnmUpdateDbdcSetting(prAdapter, true);

                                /* Start Switch
                                * Guard Time */
                                DBGLOG(
                                    CNM, INFO,
                                    "Start DBDC Switch Guard timer for DBDC Enable\n");
                                cnmTimerStartTimer(
                                    prAdapter,
                                    &prAdapter->rWifiVar.rDBDCSwitchGuardTimer,
                                    DBDC_SWITCH_GUARD_TIME);
                            }
                            break;
                        }
                    }

                    if (timerPendingTimer(
                            &prAdapter->rWifiVar.rDBDCDisableCountdownTimer) &&
                        !fgDiffBandExist) {
                        /* DBDC Disable CountDown timer
                         * is on-going however, DBDC
                         * disable is confirmed
                         */
                        DBGLOG(CNM, INFO, "Confirm to Disable DBDC\n");
                        cnmTimerStopTimer(
                            prAdapter,
                            &prAdapter->rWifiVar.rDBDCDisableCountdownTimer);
                        cnmDbdcDecision(prAdapter,
                                        DBDC_DECISION_TIMER_DISABLE_COUNT_DOWN);
                    }

                    cnmTimerStartTimer(
                        prAdapter,
                        &prAdapter->rWifiVar.rDBDCAisConnectCountDown,
                        DBDC_AIS_CONNECT_COUNTDOWN_TIME);

                    if (prAdapter->rCnmInfo.fgSkipDbdcDisable) {
                        prAdapter->rCnmInfo.fgSkipDbdcDisable = false;
                    }
#else
                    cnmDbdcEnableDecision(prAdapter, prAisBssInfo->ucBssIndex,
                                          prBssDesc->eBand);
#endif
                    cnmGetDbdcCapability(
                        prAdapter, prAisBssInfo->ucBssIndex, prBssDesc->eBand,
                        prBssDesc->ucChannelNum,
                        wlanGetSupportNss(prAdapter, prAisBssInfo->ucBssIndex),
                        &rDbdcCap);

                    prAisBssInfo->eDBDCBand = ENUM_BAND_AUTO;
                    prAisBssInfo->ucNss = rDbdcCap.ucNss;
#if (CFG_HW_WMM_BY_BSS == 0)
                    prAisBssInfo->ucWmmQueSet = rDbdcCap.ucWmmSetIndex;
#endif
#endif
                    /* 4 <B> Do STATE transition and update
                     * current Operation Mode. */
                    if (prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE) {
                        prAisBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;

                        /* Record the target BSS_DESC_T
                         * for next STATE. */
                        prAisFsmInfo->prTargetBssDesc = prBssDesc;

                        /* Transit to channel acquire */
                        eNextState = AIS_STATE_REQ_CHANNEL_JOIN;
                        fgIsTransition = true;

                        /* increase connection trial
                         * count */
                        prAisFsmInfo->ucConnTrialCount++;
                    }
#if CFG_SUPPORT_ADHOC
                    else if (prBssDesc->eBSSType == BSS_TYPE_IBSS) {
                        prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;

                        /* Record the target BSS_DESC_T
                         * for next STATE. */
                        prAisFsmInfo->prTargetBssDesc = prBssDesc;

                        eNextState = AIS_STATE_IBSS_MERGE;
                        fgIsTransition = true;
                    }
#endif
                    else {
                        ASSERT(0);
                        eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
                        fgIsTransition = true;
                    }
                } else {
                    /* 4 <2.b> If we don't have the matched one */
#if CFG_SUPPORT_DBDC_TC6
                    if (prAdapter->rCnmInfo.fgSkipDbdcDisable) {
                        /* Beacon timeout */
                        prAdapter->rCnmInfo.fgSkipDbdcDisable = false;
                        DBGLOG(
                            CNM, INFO,
                            "[DBDC] Beacon timeout: Keep current DBDC Status [%s] \n",
                            prAdapter->rWifiVar.fgDbDcModeEn ? "Enable" :
                            "Disable");
                    }
#endif
                    /* increase connection trial count for
                     * infrastructure connection */
                    if (prConnSettings->eOPMode == NET_TYPE_INFRA) {
                        prAisFsmInfo->ucConnTrialCount++;
                    }

                    /* 4 <A> Try to SCAN */
                    if (prAisFsmInfo->fgTryScan) {
                        eNextState = AIS_STATE_LOOKING_FOR;

                        fgIsTransition = true;
                    }
                    /* 4 <B> We've do SCAN already, now wait
                       in some STATE. */
                    else {
                        eNextState = aisFsmStateSearchAction(
                            prAdapter, AIS_FSM_STATE_SEARCH_ACTION_PHASE_0);
                        fgIsTransition = true;
                    }
                }
            } else {
                /* 4 <3> We are under Roaming Condition. */
                /* prAdapter->eConnectionState ==
                       MEDIA_STATE_CONNECTED. */
                /* 4 <3.a> This BSS_DESC_T is our AP. */
                /* NOTE(Kevin 2008/05/16): Following cases will
                 * go back to NORMAL_TR. CASE I: During Roaming,
                 * APP(WZC/NDISTEST) change the connection
                 *         settings. That make we can NOT match
                 * the original AP, so the prBssDesc is NULL.
                 * CASE II: The same reason as CASE I. Because
                 * APP change the eOPMode to other network type
                 * in connection setting (e.g. NET_TYPE_IBSS),
                 * so the BssDesc become the IBSS node. (For
                 * CASE I/II, before WZC/NDISTEST set the
                 * OID_SSID, it will change other parameters in
                 * connection setting first. So if we do roaming
                 * at the same time, it will hit these cases.)
                 *
                 * CASE III: Normal case, we can't find other
                 * candidate to roam out, so only the current AP
                 * will be matched.
                 *
                 * CASE VI: Timestamp of the current AP might be
                 * reset
                 */
                if (prAisBssInfo->ucReasonOfDisconnect !=
                    DISCONNECT_REASON_CODE_REASSOCIATION &&
                    ((!prBssDesc) ||  /* CASE I */
                     (prBssDesc->eBSSType !=
                      BSS_TYPE_INFRASTRUCTURE) ||  /* CASE II */
                     (prBssDesc->fgIsConnected) ||  /* CASE III
                                                     */
                     (EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
                                     prAisBssInfo->aucBSSID)))  /* CASE VI */) {
#if DBG
                    if ((prBssDesc) && (prBssDesc->fgIsConnected)) {
                        ASSERT(EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
                                              prAisBssInfo->aucBSSID));
                    }
#endif
                    /* We already associated with
                    * it, go back to NORMAL_TR */
                    /* TODO(Kevin): Roaming Fail */
#if CFG_SUPPORT_ROAMING
                    roamingFsmRunEventFail(prAdapter,
                                           ROAMING_FAIL_REASON_NOCANDIDATE);
#endif

                    /* Retreat to NORMAL_TR state */
                    eNextState = AIS_STATE_NORMAL_TR;
                    fgIsTransition = true;
                } else {
                    /* 4 <3.b> Try to roam out for JOIN this
                       BSS_DESC_T. */
                    if (prBssDesc == NULL) {
                        fgIsTransition = true;
                        eNextState = aisFsmStateSearchAction(
                            prAdapter, AIS_FSM_STATE_SEARCH_ACTION_PHASE_1);
                    } else {
                        aisFsmStateSearchAction(
                            prAdapter, AIS_FSM_STATE_SEARCH_ACTION_PHASE_2);
                        /* 4 <A> Record the target
                         * BSS_DESC_T for next STATE. */
                        prAisFsmInfo->prTargetBssDesc = prBssDesc;

                        /* tyhsu: increase connection
                         * trial count */
                        prAisFsmInfo->ucConnTrialCount++;

                        /* set NSS for re-assoc */
                        prAisBssInfo->ucNss = prAdapter->rWifiVar.ucNSS;

                        /* Transit to channel acquire */
                        eNextState = AIS_STATE_REQ_CHANNEL_JOIN;
                        fgIsTransition = true;
                    }
                }
            }

            break;

        case AIS_STATE_WAIT_FOR_NEXT_SCAN:

            DBGLOG(AIS, LOUD, "SCAN: Idle Begin - Current Time = %u\n",
                   kalGetTimeTick());

            cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rBGScanTimer,
                               SEC_TO_MSEC(prAisFsmInfo->u4SleepInterval));

            SET_NET_PWR_STATE_IDLE(prAdapter,
                                   prAdapter->prAisBssInfo->ucBssIndex);

            if (prAisFsmInfo->u4SleepInterval < AIS_BG_SCAN_INTERVAL_MAX_SEC) {
                prAisFsmInfo->u4SleepInterval <<= 1;
            }

            break;

        case AIS_STATE_SCAN:
        case AIS_STATE_ONLINE_SCAN:
        case AIS_STATE_LOOKING_FOR:

            if (!IS_NET_ACTIVE(prAdapter,
                               prAdapter->prAisBssInfo->ucBssIndex)) {
                SET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

                /* sync with firmware */
                nicActivateNetwork(prAdapter,
                                   prAdapter->prAisBssInfo->ucBssIndex);
                prAisBssInfo->fgIsNetRequestInActive = false;
            }

            /* IE length decision */
            if (prAisFsmInfo->u4ScanIELength > 0) {
                u2ScanIELen = (u16)prAisFsmInfo->u4ScanIELength;
            } else {
#if CFG_SUPPORT_WPS2
                u2ScanIELen = prAdapter->prGlueInfo->u2WSCIELen;
#else
                u2ScanIELen = 0;
#endif
            }

            prScanReqMsg = (P_MSG_SCN_SCAN_REQ_V2)cnmMemAlloc(
                prAdapter, RAM_TYPE_MSG,
                OFFSET_OF(MSG_SCN_SCAN_REQ_V2, aucIE) + u2ScanIELen);
            if (!prScanReqMsg) {
                ASSERT(0);  /* Can't trigger SCAN FSM */
                return;
            }

            prScanReqMsg->rMsgHdr.eMsgId = MID_AIS_SCN_SCAN_REQ_V2;
            prScanReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfScanReq;
            prScanReqMsg->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;

#if CFG_SUPPORT_RDD_TEST_MODE
            prScanReqMsg->eScanType = SCAN_TYPE_PASSIVE_SCAN;
#else
            if (prAisFsmInfo->eCurrentState == AIS_STATE_SCAN ||
                prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN) {
                if (prAisFsmInfo->ucScanSSIDNum == 0) {
#if CFG_SUPPORT_AIS_PASSIVE_SCAN
                    prScanReqMsg->eScanType = SCAN_TYPE_PASSIVE_SCAN;

                    prScanReqMsg->ucSSIDType = 0;
                    prScanReqMsg->ucSSIDNum = 0;
#else
                    prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;

                    prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_WILDCARD;
                    prScanReqMsg->ucSSIDNum = 0;
#endif
                } else if (prAisFsmInfo->ucScanSSIDNum == 1 &&
                           prAisFsmInfo->arScanSSID[0].u4SsidLen == 0) {
                    prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;

                    prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_WILDCARD;
                    prScanReqMsg->ucSSIDNum = 0;
                } else {
                    prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;

                    prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_SPECIFIED;
                    prScanReqMsg->ucSSIDNum = prAisFsmInfo->ucScanSSIDNum;
                    prScanReqMsg->prSsid = prAisFsmInfo->arScanSSID;
                }
            } else {
                prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;

                COPY_SSID(prAisFsmInfo->rRoamingSSID.aucSsid,
                          prAisFsmInfo->rRoamingSSID.u4SsidLen,
                          prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

                /* Scan for determined SSID */
                prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_SPECIFIED;
                prScanReqMsg->ucSSIDNum = 1;
                prScanReqMsg->prSsid = &(prAisFsmInfo->rRoamingSSID);
            }
#endif

            /* using default channel dwell time/timeout value */
            prScanReqMsg->u2ProbeDelay = 0;
            prScanReqMsg->u2ChannelDwellTime = 0;
            prScanReqMsg->u2TimeoutValue = 0;

            /* check if tethering is running and need to fix on
             * specific channel */
            if (cnmAisInfraChannelFixed(prAdapter, &eBand, &ucChannel) ==
                true) {
                prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
                prScanReqMsg->ucChannelListNum = 1;
                prScanReqMsg->arChnlInfoList[0].eBand = eBand;
                prScanReqMsg->arChnlInfoList[0].ucChannelNum = ucChannel;
#if CFG_SCAN_CHANNEL_SPECIFIED
            } else if (is_valid_scan_chnl_cnt(
                           prAisFsmInfo->ucScanChannelListNum)) {
                prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
                prScanReqMsg->ucChannelListNum =
                    prAisFsmInfo->ucScanChannelListNum;
                kalMemCopy(prScanReqMsg->arChnlInfoList,
                           prAisFsmInfo->arScanChnlInfoList,
                           sizeof(RF_CHANNEL_INFO_T) *
                           prScanReqMsg->ucChannelListNum);
#endif
            } else if (prAdapter
                       ->aePreferBand[prAdapter->prAisBssInfo->ucBssIndex] ==
                       BAND_NULL) {
                if (prAdapter->fgEnable5GBand == true) {
                    prScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
                } else {
                    prScanReqMsg->eScanChannel = SCAN_CHANNEL_2G4;
                }
            } else if (prAdapter
                       ->aePreferBand[prAdapter->prAisBssInfo->ucBssIndex] ==
                       BAND_2G4) {
                prScanReqMsg->eScanChannel = SCAN_CHANNEL_2G4;
            } else if (prAdapter
                       ->aePreferBand[prAdapter->prAisBssInfo->ucBssIndex] ==
                       BAND_5G) {
                prScanReqMsg->eScanChannel = SCAN_CHANNEL_5G;
            } else {
                prScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
                ASSERT(0);
            }

            if (prAisFsmInfo->u4ScanIELength > 0) {
                kalMemCopy(prScanReqMsg->aucIE, prAisFsmInfo->aucScanIEBuf,
                           prAisFsmInfo->u4ScanIELength);
            } else {
#if CFG_SUPPORT_WPS2
                if (prAdapter->prGlueInfo->u2WSCIELen > 0) {
                    kalMemCopy(prScanReqMsg->aucIE,
                               &prAdapter->prGlueInfo->aucWSCIE,
                               prAdapter->prGlueInfo->u2WSCIELen);
                }
#endif
            }

            prScanReqMsg->u2IELen = u2ScanIELen;

            mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prScanReqMsg,
                        MSG_SEND_METHOD_BUF);

            prAisFsmInfo->fgTryScan = false;  /* Will enable
                                                 background sleep for
                                                 infrastructure */

            break;

        case AIS_STATE_REQ_CHANNEL_JOIN:
            /* send message to CNM for acquiring channel */
            prMsgChReq = (P_MSG_CH_REQ_T)cnmMemAlloc(prAdapter,
                                                     RAM_TYPE_MSG,
                                                     sizeof(MSG_CH_REQ_T));
            if (!prMsgChReq) {
                ASSERT(0);  /* Can't indicate CNM for channel
                               acquiring */
                return;
            }

            prMsgChReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
            prMsgChReq->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
            prMsgChReq->ucTokenID = ++prAisFsmInfo->ucSeqNumOfChReq;
            prMsgChReq->eReqType = CH_REQ_TYPE_JOIN;
#ifdef CFG_SUPPORT_ADJUST_JOIN_CH_REQ_INTERVAL
            prMsgChReq->u4MaxInterval =
                prAdapter->rWifiVar.u4AisJoinChReqIntervel;
#else
            prMsgChReq->u4MaxInterval = AIS_JOIN_CH_REQUEST_INTERVAL;
#endif
            DBGLOG(AIS, INFO, "Request join interval: %u\n",
                   prMsgChReq->u4MaxInterval);
            prMsgChReq->ucPrimaryChannel =
                prAisFsmInfo->prTargetBssDesc->ucChannelNum;
            prMsgChReq->eRfSco = prAisFsmInfo->prTargetBssDesc->eSco;
            prMsgChReq->eRfBand = prAisFsmInfo->prTargetBssDesc->eBand;
#if CFG_SUPPORT_DBDC
            prMsgChReq->eDBDCBand = ENUM_BAND_AUTO;
#endif
            /* To do: check if 80/160MHz bandwidth is needed here */
            prMsgChReq->eRfChannelWidth =
                prAisFsmInfo->prTargetBssDesc->eChannelWidth;
            prMsgChReq->ucRfCenterFreqSeg1 =
                prAisFsmInfo->prTargetBssDesc->ucCenterFreqS1;
            prMsgChReq->ucRfCenterFreqSeg2 =
                prAisFsmInfo->prTargetBssDesc->ucCenterFreqS2;

            rlmReviseMaxBw(prAdapter, prAisBssInfo->ucBssIndex,
                           &prMsgChReq->eRfSco,
                           (u8 *)&prMsgChReq->eRfChannelWidth,
                           &prMsgChReq->ucRfCenterFreqSeg1,
                           &prMsgChReq->ucPrimaryChannel);

            mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prMsgChReq,
                        MSG_SEND_METHOD_BUF);

            prAisFsmInfo->fgIsChannelRequested = true;
            break;

        case AIS_STATE_JOIN:
            aisFsmStateInit_JOIN(prAdapter, prAisFsmInfo->prTargetBssDesc);
            break;

        case AIS_STATE_JOIN_FAILURE:
            if (prAisFsmInfo->prTargetBssDesc) {
                if (prAisFsmInfo->prTargetBssDesc->fgIsConnecting !=
                    false) {
                    DBGLOG(
                        AIS, ERROR,
                        "Connecting Flag(%d) is unusual in JOIN_FAILURE state\n",
                        prAisFsmInfo->prTargetBssDesc->fgIsConnecting);
                }
            }

            nicMediaJoinFailure(prAdapter,
                                prAdapter->prAisBssInfo->ucBssIndex,
                                (prConnSettings->
                                 fgIsDisconnectedByNonRequest) ?
                                (WLAN_STATUS_JOIN_ABORT) :
                                (WLAN_STATUS_JOIN_TIMEOUT));

            prConnSettings->fgIsDisconnectedByNonRequest = true;

            eNextState = AIS_STATE_IDLE;
            fgIsTransition = true;

            break;

#if CFG_SUPPORT_ADHOC
        case AIS_STATE_IBSS_ALONE:
            aisFsmStateInit_IBSS_ALONE(prAdapter);
            break;

        case AIS_STATE_IBSS_MERGE:
            aisFsmStateInit_IBSS_MERGE(prAdapter,
                                       prAisFsmInfo->prTargetBssDesc);
            break;
#endif

        case AIS_STATE_NORMAL_TR:
            if (prAisFsmInfo->fgIsInfraChannelFinished == false) {
                /* Don't do anything when rJoinTimeoutTimer is
                 * still ticking */
            } else {
                /* 1. Process for pending scan */
                if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN,
                                           true) ==
                    true) {
                    wlanClearScanningResult(prAdapter);
                    eNextState = AIS_STATE_ONLINE_SCAN;
                    fgIsTransition = true;
                }
                /* 2. Process for pending roaming scan */
                else if (aisFsmIsRequestPending(prAdapter,
                                                AIS_REQUEST_ROAMING_SEARCH,
                                                true) == true) {
                    eNextState = AIS_STATE_LOOKING_FOR;
                    fgIsTransition = true;
                }
                /* 3. Process for pending roaming scan */
                else if (aisFsmIsRequestPending(prAdapter,
                                                AIS_REQUEST_ROAMING_CONNECT,
                                                true) == true) {
                    eNextState = AIS_STATE_SEARCH;
                    fgIsTransition = true;
                } else if (aisFsmIsRequestPending(prAdapter,
                                                  AIS_REQUEST_REMAIN_ON_CHANNEL,
                                                  true) == true) {
                    eNextState = AIS_STATE_REQ_REMAIN_ON_CHANNEL;
                    fgIsTransition = true;
                }
            }

            break;

        case AIS_STATE_DISCONNECTING:
            cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP,
                                 STA_STATE_1);
            /* send for deauth frame for disconnection */
            authSendDeauthFrame(prAdapter, prAisBssInfo,
                                prAisBssInfo->prStaRecOfAP,
                                (P_SW_RFB_T)NULL,
                                REASON_CODE_DEAUTH_LEAVING_BSS,
                                aisDeauthXmitComplete);
            cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rDeauthDoneTimer,
                               200);
            break;

        case AIS_STATE_REQ_REMAIN_ON_CHANNEL:
            /* send message to CNM for acquiring channel */
            prMsgChReq = (P_MSG_CH_REQ_T)cnmMemAlloc(prAdapter,
                                                     RAM_TYPE_MSG,
                                                     sizeof(MSG_CH_REQ_T));
            if (!prMsgChReq) {
                ASSERT(0);  /* Can't indicate CNM for channel
                               acquiring */
                return;
            }

            /* zero-ize */
            kalMemZero(prMsgChReq, sizeof(MSG_CH_REQ_T));

            /* filling */
            prMsgChReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
            prMsgChReq->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
            prMsgChReq->ucTokenID = ++prAisFsmInfo->ucSeqNumOfChReq;
            prMsgChReq->eReqType = CH_REQ_TYPE_JOIN;
            prMsgChReq->u4MaxInterval =
                prAisFsmInfo->rChReqInfo.u4DurationMs;
            prMsgChReq->ucPrimaryChannel =
                prAisFsmInfo->rChReqInfo.ucChannelNum;
            prMsgChReq->eRfSco = prAisFsmInfo->rChReqInfo.eSco;
            prMsgChReq->eRfBand = prAisFsmInfo->rChReqInfo.eBand;
#if CFG_SUPPORT_DBDC
            prMsgChReq->eDBDCBand = ENUM_BAND_AUTO;
#endif
            mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prMsgChReq,
                        MSG_SEND_METHOD_BUF);

            prAisFsmInfo->fgIsChannelRequested = true;

            break;

        case AIS_STATE_REMAIN_ON_CHANNEL:
            if (!IS_NET_ACTIVE(prAdapter,
                               prAdapter->prAisBssInfo->ucBssIndex)) {
                SET_NET_ACTIVE(prAdapter,
                               prAdapter->prAisBssInfo->ucBssIndex);
                /* sync with firmware */
                /* FW will re-activate BSS, and set BSS: */
                /*   1. pwr state = IDLE         */
                /*   2. connected = Disconnected */
                nicActivateNetwork(prAdapter,
                                   prAdapter->prAisBssInfo->ucBssIndex);
            }
            prAisBssInfo->fgIsNetRequestInActive = false;
            break;

        default:
            ASSERT(0);  /* Make sure we have handle all STATEs */
            break;
        }
    }while (fgIsTransition);

    return;
}

enum _ENUM_AIS_STATE_T aisFsmStateSearchAction(
    IN struct _ADAPTER_T *prAdapter,
    u8 ucPhase)
{
    struct _CONNECTION_SETTINGS_T *prConnSettings;
    struct _BSS_INFO_T *prAisBssInfo;
    struct _AIS_FSM_INFO_T *prAisFsmInfo;
    struct _BSS_DESC_T *prBssDesc;
    enum _ENUM_AIS_STATE_T eState = AIS_STATE_IDLE;

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prBssDesc = scanSearchBssDescByPolicy(prAdapter,
                                          prAdapter->prAisBssInfo->ucBssIndex);

    if (ucPhase == AIS_FSM_STATE_SEARCH_ACTION_PHASE_0) {
        if (prConnSettings->eOPMode == NET_TYPE_INFRA) {
            /* issue reconnect request, */
            /*and retreat to idle state for scheduling */
            aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);
            eState = AIS_STATE_IDLE;
        }
#if CFG_SUPPORT_ADHOC
        else if ((prConnSettings->eOPMode == NET_TYPE_IBSS) ||
                 (prConnSettings->eOPMode == NET_TYPE_AUTO_SWITCH) ||
                 (prConnSettings->eOPMode == NET_TYPE_DEDICATED_IBSS)) {
            prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;
            prAisFsmInfo->prTargetBssDesc = NULL;
            eState = AIS_STATE_IBSS_ALONE;
        }
#endif
        else {
            ASSERT(0);
            eState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
        }
    } else if (ucPhase == AIS_FSM_STATE_SEARCH_ACTION_PHASE_1) {
        /* increase connection trial count for infrastructure connection
         */
        if (prConnSettings->eOPMode == NET_TYPE_INFRA) {
            prAisFsmInfo->ucConnTrialCount++;
        }
        /* 4 <A> Try to SCAN */
        if (prAisFsmInfo->fgTryScan) {
            eState = AIS_STATE_LOOKING_FOR;
        }
        /* 4 <B> We've do SCAN already, now wait in some STATE. */
        else {
            if (prConnSettings->eOPMode == NET_TYPE_INFRA) {
                /* issue reconnect request, and */
                /* retreat to idle state for scheduling */
                aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);

                eState = AIS_STATE_IDLE;
            }
#if CFG_SUPPORT_ADHOC
            else if ((prConnSettings->eOPMode == NET_TYPE_IBSS) ||
                     (prConnSettings->eOPMode == NET_TYPE_AUTO_SWITCH) ||
                     (prConnSettings->eOPMode == NET_TYPE_DEDICATED_IBSS)) {
                prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;
                prAisFsmInfo->prTargetBssDesc = NULL;

                eState = AIS_STATE_IBSS_ALONE;
            }
#endif
            else {
                ASSERT(0);
                eState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
            }
        }
    } else {
#if DBG
        if (prAisBssInfo->ucReasonOfDisconnect !=
            DISCONNECT_REASON_CODE_REASSOCIATION) {
            ASSERT(
                UNEQUAL_MAC_ADDR(prBssDesc->aucBSSID,
                                 prAisBssInfo->aucBSSID));
        }
#endif
    }
    return eState;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventScanDone(IN P_ADAPTER_T prAdapter,
                            IN P_MSG_HDR_T prMsgHdr)
{
    P_MSG_SCN_SCAN_DONE prScanDoneMsg;
    P_AIS_FSM_INFO_T prAisFsmInfo;
    ENUM_AIS_STATE_T eNextState;
    u8 ucSeqNumOfCompMsg;
    P_CONNECTION_SETTINGS_T prConnSettings;

    DEBUGFUNC("aisFsmRunEventScanDone()");

    ASSERT(prAdapter);
    ASSERT(prMsgHdr);
    DBGLOG(AIS, INFO, "ScanDone\n");

    DBGLOG(AIS, LOUD, "EVENT-SCAN DONE: Current Time = %u\n",
           kalGetTimeTick());

    if (prAdapter->prAisBssInfo == NULL) {
        /* This case occurs when the AIS isn't done, but the wlan0 */
        /* has changed to AP mode. And the prAisBssInfo is freed.  */
        DBGLOG(AIS, WARN, "prAisBssInfo is NULL, and then return\n");
        return;
    }

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    prScanDoneMsg = (P_MSG_SCN_SCAN_DONE)prMsgHdr;
    ASSERT(prScanDoneMsg->ucBssIndex ==
           prAdapter->prAisBssInfo->ucBssIndex);

    ucSeqNumOfCompMsg = prScanDoneMsg->ucSeqNum;
    cnmMemFree(prAdapter, prMsgHdr);

    eNextState = prAisFsmInfo->eCurrentState;

    if (ucSeqNumOfCompMsg != prAisFsmInfo->ucSeqNumOfScanReq) {
        DBGLOG(AIS, WARN, "SEQ NO of AIS SCN DONE MSG is not matched.\n");
    } else {
        cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);
        switch (prAisFsmInfo->eCurrentState) {
        case AIS_STATE_IDLE:
        case AIS_STATE_SCAN:
            prConnSettings->fgIsScanReqIssued = false;

            /* reset scan IE buffer */
            prAisFsmInfo->u4ScanIELength = 0;

            kalScanDone(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_AIS_INDEX,
                        WLAN_STATUS_SUCCESS);
            eNextState = AIS_STATE_IDLE;
            scanReportScanResultToAgps(prAdapter);

            break;

        case AIS_STATE_ONLINE_SCAN:
            prConnSettings->fgIsScanReqIssued = false;

            /* reset scan IE buffer */
            prAisFsmInfo->u4ScanIELength = 0;

            kalScanDone(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_AIS_INDEX,
                        WLAN_STATUS_SUCCESS);
#if CFG_SUPPORT_ROAMING
            eNextState = aisFsmRoamingScanResultsUpdate(prAdapter);
#else
            eNextState = AIS_STATE_NORMAL_TR;
#endif
            scanReportScanResultToAgps(prAdapter);
            break;

        case AIS_STATE_LOOKING_FOR:
            if (prConnSettings->fgIsDisconnectedByNonRequest) {
                eNextState = AIS_STATE_IDLE;
            } else {
#if CFG_SUPPORT_ROAMING
                eNextState = aisFsmRoamingScanResultsUpdate(prAdapter);
#else
                eNextState = AIS_STATE_SEARCH;
#endif
            }
            break;

        default:
            prConnSettings->fgIsScanReqIssued = false;

            /* reset scan IE buffer */
            prAisFsmInfo->u4ScanIELength = 0;

            kalScanDone(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_AIS_INDEX,
                        WLAN_STATUS_SUCCESS);
            break;
        }
    }

    if (eNextState != prAisFsmInfo->eCurrentState) {
        aisFsmSteps(prAdapter, eNextState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventAbort(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
    P_MSG_AIS_ABORT_T prAisAbortMsg;
    P_AIS_FSM_INFO_T prAisFsmInfo;
    u8 ucReasonOfDisconnect;
    u8 fgDelayIndication;
    P_CONNECTION_SETTINGS_T prConnSettings;
    struct _BSS_INFO_T *prAisBssInfo;

    DEBUGFUNC("aisFsmRunEventAbort()");

    ASSERT(prAdapter);
    ASSERT(prMsgHdr);
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prAisBssInfo = prAdapter->prAisBssInfo;

    // @ShiuanWen - Bug fix when run WPA3-SAE 5.2.1 then 5.2.6
    // TC5.2.1, prAisBssInfo->aucSSID keep the ssid Wi-Fi-5.2.1 due to
    // connect successful. TC5.2.6, aisFsmJoinCompleteAction use the wrong
    // prAisBssInfo->aucSSID and run to scanSearchBssDescByBssidAndSsid. It
    // caused the prBssDesc is NULL and return to
    // aisFsmRunEventJoinComplete. AIS state machine is blocked on JOIN
    // state.
    kalMemZero(prAisBssInfo->aucSSID, sizeof(prAisBssInfo->aucSSID));
    prAisBssInfo->ucSSIDLen = 0;

    /* 4 <1> Extract information of Abort Message and then free memory. */
    prAisAbortMsg = (P_MSG_AIS_ABORT_T)prMsgHdr;
    ucReasonOfDisconnect = prAisAbortMsg->ucReasonOfDisconnect;
    fgDelayIndication = prAisAbortMsg->fgDelayIndication;

    cnmMemFree(prAdapter, prMsgHdr);

    DBGLOG(AIS, STATE,
           "EVENT-ABORT: Current State %s, ucReasonOfDisconnect:%d\n",
           apucDebugAisState[prAisFsmInfo->eCurrentState],
           ucReasonOfDisconnect);

    /* record join request time */
    GET_CURRENT_SYSTIME(&(prAisFsmInfo->rJoinReqTime));

    /* 4 <2> clear previous pending connection request and insert new one */
    if (ucReasonOfDisconnect == DISCONNECT_REASON_CODE_DEAUTHENTICATED
#if CFG_SUPPORT_DBDC_TC6
        || ucReasonOfDisconnect == DISCONNECT_REASON_CODE_DBDC_REASSOCIATION
#endif
        || ucReasonOfDisconnect == DISCONNECT_REASON_CODE_DISASSOCIATED) {
        prConnSettings->fgIsDisconnectedByNonRequest = true;
    } else {
        prConnSettings->fgIsDisconnectedByNonRequest = false;
    }

    /* to support user space triggered roaming */
    if (ucReasonOfDisconnect == DISCONNECT_REASON_CODE_REASSOCIATION &&
        prAisFsmInfo->eCurrentState != AIS_STATE_DISCONNECTING) {
        if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR &&
            prAisFsmInfo->fgIsInfraChannelFinished == true) {
#if CFG_SUPPORT_SAME_BSS_REASSOC
            P_BSS_INFO_T prAisBssInfo = prAdapter->prAisBssInfo;
            prAisBssInfo->ucReasonOfDisconnect = ucReasonOfDisconnect;
#endif
            aisFsmSteps(prAdapter, AIS_STATE_SEARCH);
        } else {
            aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH,
                                   true);
            aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT,
                                   true);
            aisFsmInsertRequest(prAdapter, AIS_REQUEST_ROAMING_CONNECT);
        }
        return;
    }

    aisFsmIsRequestPending(prAdapter, AIS_REQUEST_RECONNECT, true);
    aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);

    if (prAisFsmInfo->eCurrentState != AIS_STATE_DISCONNECTING) {
        /* 4 <3> invoke abort handler */
        DBGLOG(AIS, STATE, "ucReasonOfDisconnect:%d\n",
               ucReasonOfDisconnect);
        aisFsmStateAbort(prAdapter, ucReasonOfDisconnect,
                         fgDelayIndication);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief        This function handles AIS-FSM abort event/command
 *
 * \param[in] prAdapter              Pointer of ADAPTER_T
 *            ucReasonOfDisconnect   Reason for disonnection
 *            fgDelayIndication      Option to delay disconnection indication
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort(IN P_ADAPTER_T prAdapter, u8 ucReasonOfDisconnect,
                      u8 fgDelayIndication)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_INFO_T prAisBssInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;
    u8 fgIsCheckConnected;

    ASSERT(prAdapter);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;

    /* XXX: The wlan0 may has been changed to AP mode. */
    if (prAisBssInfo == NULL) {
        return;
    }

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    fgIsCheckConnected = false;

    DBGLOG(AIS, STATE, "aisFsmStateAbort DiscReason[%d], CurState[%d]\n",
           ucReasonOfDisconnect, prAisFsmInfo->eCurrentState);

    /* 4 <1> Save information of Abort Message and then free memory. */
    prAisBssInfo->ucReasonOfDisconnect = ucReasonOfDisconnect;

    /* 4 <2> Abort current job. */
    switch (prAisFsmInfo->eCurrentState) {
    case AIS_STATE_IDLE:
    case AIS_STATE_SEARCH:
    case AIS_STATE_JOIN_FAILURE:
        break;

    case AIS_STATE_WAIT_FOR_NEXT_SCAN:
        /* Do cancel timer */
        cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rBGScanTimer);

        /* in case roaming is triggered */
        fgIsCheckConnected = true;
        break;

    case AIS_STATE_SCAN:
        /* Do abort SCAN */
        aisFsmStateAbort_SCAN(prAdapter);

        /* queue for later handling */
        if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, false) ==
            false) {
            aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
        }

        break;

    case AIS_STATE_LOOKING_FOR:
        /* Do abort SCAN */
        aisFsmStateAbort_SCAN(prAdapter);

        /* in case roaming is triggered */
        fgIsCheckConnected = true;
        break;

    case AIS_STATE_REQ_CHANNEL_JOIN:
        /* Release channel to CNM */
        aisFsmReleaseCh(prAdapter);

        /* in case roaming is triggered */
        fgIsCheckConnected = true;
        break;

    case AIS_STATE_JOIN:
        /* Do abort JOIN */
        aisFsmStateAbort_JOIN(prAdapter);

        /* in case roaming is triggered */
        fgIsCheckConnected = true;
        break;

#if CFG_SUPPORT_ADHOC
    case AIS_STATE_IBSS_ALONE:
    case AIS_STATE_IBSS_MERGE:
        aisFsmStateAbort_IBSS(prAdapter);
        break;
#endif

    case AIS_STATE_ONLINE_SCAN:
        /* Do abort SCAN */
        aisFsmStateAbort_SCAN(prAdapter);

        /* queue for later handling */
        if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, false) ==
            false) {
            aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
        }

        fgIsCheckConnected = true;
        break;

    case AIS_STATE_NORMAL_TR:
        fgIsCheckConnected = true;
        break;

    case AIS_STATE_DISCONNECTING:
        /* Do abort NORMAL_TR */
        aisFsmStateAbort_NORMAL_TR(prAdapter);

        break;

    case AIS_STATE_REQ_REMAIN_ON_CHANNEL:
        fgIsCheckConnected = true;

        /* release channel */
        aisFsmReleaseCh(prAdapter);
        break;

    case AIS_STATE_REMAIN_ON_CHANNEL:
        fgIsCheckConnected = true;

        /* 1. release channel */
        aisFsmReleaseCh(prAdapter);

        /* 2. stop channel timeout timer */
        cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer);

        break;

    default:
        break;
    }

    if (fgIsCheckConnected &&
        (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED)) {
        /* switch into DISCONNECTING state for sending DEAUTH if
         * necessary */
        if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
            prAisBssInfo->ucReasonOfDisconnect !=
            DISCONNECT_REASON_CODE_DEAUTHENTICATED &&
            prAisBssInfo->prStaRecOfAP &&
            prAisBssInfo->prStaRecOfAP->fgIsInUse) {
            aisFsmSteps(prAdapter, AIS_STATE_DISCONNECTING);

            return;
        }
        /* Do abort NORMAL_TR */
        aisFsmStateAbort_NORMAL_TR(prAdapter);
    }

    aisFsmDisconnect(prAdapter, fgDelayIndication);

    return;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Join Complete Event from SAA FSM for AIS
 * FSM
 *
 * @param[in] prMsgHdr   Message of Join Complete of SAA FSM.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventJoinComplete(IN struct _ADAPTER_T *prAdapter,
                                IN struct _MSG_HDR_T *prMsgHdr)
{
    struct _MSG_SAA_FSM_COMP_T *prJoinCompMsg;
    struct _AIS_FSM_INFO_T *prAisFsmInfo;
    enum _ENUM_AIS_STATE_T eNextState;
    struct _SW_RFB_T *prAssocRspSwRfb;

    DEBUGFUNC("aisFsmRunEventJoinComplete()");
    ASSERT(prMsgHdr);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prJoinCompMsg = (struct _MSG_SAA_FSM_COMP_T *)prMsgHdr;
    prAssocRspSwRfb = prJoinCompMsg->prSwRfb;

    eNextState = prAisFsmInfo->eCurrentState;

    /* Check State and SEQ NUM */
    if (prAisFsmInfo->eCurrentState == AIS_STATE_JOIN) {
        /* Check SEQ NUM */
        if (prJoinCompMsg->ucSeqNum == prAisFsmInfo->ucSeqNumOfReqMsg) {
            eNextState = aisFsmJoinCompleteAction(prAdapter, prMsgHdr);
        }
#if DBG
        else {
            DBGLOG(AIS, WARN,
                   "SEQ NO of AIS JOIN COMP MSG is not matched.\n");
        }
#endif
    }
    if (eNextState != prAisFsmInfo->eCurrentState) {
        aisFsmSteps(prAdapter, eNextState);
    }

    if (eNextState == AIS_STATE_NORMAL_TR) {
        DBGLOG(AIS, STATE, "cancel beacon lost timer.\n");
        cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rBeaconLostTimer);
    }

    if (prAssocRspSwRfb) {
        nicRxReturnRFB(prAdapter, prAssocRspSwRfb);
    }

    cnmMemFree(prAdapter, prMsgHdr);
}

enum _ENUM_AIS_STATE_T aisFsmJoinCompleteAction(
    IN struct _ADAPTER_T *prAdapter,
    IN struct _MSG_HDR_T *prMsgHdr)
{
    struct _MSG_SAA_FSM_COMP_T *prJoinCompMsg;
    struct _AIS_FSM_INFO_T *prAisFsmInfo;
    enum _ENUM_AIS_STATE_T eNextState;
    struct _STA_RECORD_T *prStaRec;
    struct _SW_RFB_T *prAssocRspSwRfb;
    struct _BSS_INFO_T *prAisBssInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;
    u32 rCurrentTime;
#if CFG_SUPPORT_BFER
    u8 ucStaVhtBfer = prAdapter->rWifiVar.ucStaVhtBfer;
    u8 ucStaHtBfer = prAdapter->rWifiVar.ucStaHtBfer;
#endif

    DEBUGFUNC("aisFsmJoinCompleteAction()");

    ASSERT(prMsgHdr);

    GET_CURRENT_SYSTIME(&rCurrentTime);

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prJoinCompMsg = (struct _MSG_SAA_FSM_COMP_T *)prMsgHdr;
    prStaRec = prJoinCompMsg->prStaRec;
    prAssocRspSwRfb = prJoinCompMsg->prSwRfb;
    prAisBssInfo = prAdapter->prAisBssInfo;
    eNextState = prAisFsmInfo->eCurrentState;

    /* 4 <1> JOIN was successful */
    if (prJoinCompMsg->rJoinStatus == WLAN_STATUS_SUCCESS) {
        /* 1. Reset retry count */
        prAisFsmInfo->ucConnTrialCount = 0;

        /* Completion of roaming */
        if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
#if CFG_SUPPORT_ROAMING
            /* 2. Deactivate previous BSS */
            aisFsmRoamingDisconnectPrevAP(prAdapter, prStaRec);
#endif

#if CFG_SUPPORT_ROAMING || CFG_SUPPORT_SAME_BSS_REASSOC
            /* 3. Update bss based on roaming staRec */
            aisUpdateBssInfoForRoamingAP(prAdapter, prStaRec,
                                         prAssocRspSwRfb);
#endif
#if CFG_SUPPORT_SAME_BSS_REASSOC
            aisFsmReleaseCh(prAdapter);
            cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);
            prAisFsmInfo->fgIsInfraChannelFinished = true;
#endif
        } else {
            /* 4 <1.1> Change FW's Media State immediately. */
            aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

            /* 4 <1.2> Deactivate previous AP's STA_RECORD_T in
             * Driver if have. */
            if ((prAisBssInfo->prStaRecOfAP) &&
                (prAisBssInfo->prStaRecOfAP != prStaRec) &&
                (prAisBssInfo->prStaRecOfAP->fgIsInUse)) {
                cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP,
                                     STA_STATE_1);
                cnmStaRecFree(prAdapter, prAisBssInfo->prStaRecOfAP);
            }

            /* For temp solution, need to refine */
            /* 4 <1.4> Update BSS_INFO_T */
            aisUpdateBssInfoForJOIN(prAdapter, prStaRec, prAssocRspSwRfb);

            /* 4 <1.3> Activate current AP's STA_RECORD_T in Driver.
             */
            cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);

#if CFG_SUPPORT_BFER
            if ((IS_FEATURE_ENABLED(ucStaVhtBfer) ||
                 IS_FEATURE_ENABLED(ucStaHtBfer)) &&
                prAdapter->fgIsHwSupportBfer) {
                rlmBfStaRecPfmuUpdate(prAdapter, prStaRec);
                rlmETxBfTriggerPeriodicSounding(prAdapter);
            }
#endif

            /* 4 <1.5> Update RSSI if necessary */
            nicUpdateRSSI(prAdapter, prAdapter->prAisBssInfo->ucBssIndex,
                          (s8)(RCPI_TO_dBm(prStaRec->ucRCPI)), 0);

            /* 4 <1.6> Indicate Connected Event to Host immediately.
             */
            /* Require BSSID, Association ID, Beacon Interval */
            /* .. from AIS_BSS_INFO_T */
            aisIndicationOfMediaStateToHost(prAdapter,
                                            PARAM_MEDIA_STATE_CONNECTED,
                                            false);

            if (prAdapter->rWifiVar.ucTpTestMode ==
                ENUM_TP_TEST_MODE_THROUGHPUT) {
                nicEnterTPTestMode(prAdapter, TEST_MODE_THROUGHPUT);
            } else if (prAdapter->rWifiVar.ucTpTestMode ==
                       ENUM_TP_TEST_MODE_SIGMA_AC_N_PMF) {
                nicEnterTPTestMode(prAdapter, TEST_MODE_SIGMA_AC_N_PMF);
            } else if (prAdapter->rWifiVar.ucTpTestMode ==
                       ENUM_TP_TEST_MODE_SIGMA_WMM_PS) {
                nicEnterTPTestMode(prAdapter, TEST_MODE_SIGMA_WMM_PS);
            }
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT
            else if (prAdapter->rWifiVar.ucTpTestMode ==
                     ENUM_TP_TEST_MODE_AUDIO_MRM) {
                nicEnterTPTestMode(prAdapter, TEST_MODE_AUDIO_MRM);
            }
#endif
        }

#if CFG_SUPPORT_ROAMING
        /* if user space roaming is enabled, we should disable driver/fw
         * roaming */
#ifdef CONFIG_CFG80211_ALLOW_RECONNECT
        if (prAdapter->rWifiVar.rConnSettings.eConnectionPolicy !=
            CONNECT_BY_BSSID)
#endif
        roamingFsmRunEventStart(prAdapter);
#endif

#if CFG_SUPPORT_802_11K || CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
        aisResetNeighborApList(prAdapter);
#endif
#if CFG_SUPPORT_802_11K
        if (prAisFsmInfo->prTargetBssDesc->aucRrmCap[0] &
            BIT(RRM_CAP_INFO_NEIGHBOR_REPORT_BIT)) {
            aisSendNeighborRequest(prAdapter);
        }
#endif
#if CFG_SUPPORT_DBDC_TC6
        if (timerPendingTimer(&prAdapter->rWifiVar.rDBDCReconnectCountDown)) {
            DBGLOG(
                CNM, INFO,
                "DBDC reconnect timer protection end due to join complete\n");
            cnmTimerStopTimer(prAdapter,
                              &prAdapter->rWifiVar.rDBDCReconnectCountDown);

            if (timerPendingTimer(
                    &prAdapter->rWifiVar.rDBDCSwitchGuardTimer)) {
                DBGLOG(CNM, INFO,
                       "DBDC switch guard timer end due to join complete\n");
                cnmTimerStopTimer(prAdapter,
                                  &prAdapter->rWifiVar.rDBDCSwitchGuardTimer);
            }
        }

        if (timerPendingTimer(&prAdapter->rWifiVar.rDBDCAisConnectCountDown)) {
            DBGLOG(
                CNM, INFO,
                "DBDC AIS connect timer protection end due to join complete\n");
            cnmTimerStopTimer(prAdapter,
                              &prAdapter->rWifiVar.rDBDCAisConnectCountDown);
        }
#endif
        /* 4 <1.7> Set the Next State of AIS FSM */
        eNextState = AIS_STATE_NORMAL_TR;
    }
    /* 4 <2> JOIN was not successful */
    else {
        /* 4 <2.1> Redo JOIN process with other Auth Type if possible */
        if (aisFsmStateInit_RetryJOIN(prAdapter, prStaRec) == false) {
            struct _BSS_DESC_T *prBssDesc;
            PARAM_SSID_T rParamSsid;

            prBssDesc = prAisFsmInfo->prTargetBssDesc;

            /* 1. Increase Failure Count */
            prStaRec->ucJoinFailureCount++;

            /* 2. release channel */
            aisFsmReleaseCh(prAdapter);

            /* 3.1 stop join timeout timer */
            cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);

            /* 3.2 reset local variable */
            prAisFsmInfo->fgIsInfraChannelFinished = true;

            kalMemZero(&rParamSsid, sizeof(PARAM_SSID_T));

            if (prBssDesc) {
                COPY_SSID(rParamSsid.aucSsid, rParamSsid.u4SsidLen,
                          prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
            } else {
                COPY_SSID(rParamSsid.aucSsid, rParamSsid.u4SsidLen,
                          prConnSettings->aucSSID,
                          prConnSettings->ucSSIDLen);
            }

            prBssDesc = scanSearchBssDescByBssidAndSsid(
                prAdapter, prStaRec->aucMacAddr, true, &rParamSsid);

            if (prBssDesc == NULL) {
                prBssDesc = scanSearchBssDescByBssidAndChanNum(
                    prAdapter, prConnSettings->aucBSSID, true,
                    prConnSettings->ucChannelNum);
            }

            if (prBssDesc == NULL) {
                return eNextState;
            }

            /* ASSERT(prBssDesc); */
            /* ASSERT(prBssDesc->fgIsConnecting); */
            prBssDesc->ucJoinFailureCount++;
            if (prBssDesc->ucJoinFailureCount >
                SCN_BSS_JOIN_FAIL_THRESOLD) {
                GET_CURRENT_SYSTIME(&prBssDesc->rJoinFailTime);
                DBGLOG(AIS, INFO, "Bss " MACSTR " join fail %d times,",
                       MAC2STR(prBssDesc->aucBSSID),
                       SCN_BSS_JOIN_FAIL_THRESOLD);
                DBGLOG(AIS, INFO, " temp disable it at time: %u\n",
                       prBssDesc->rJoinFailTime);
            }

            if (prBssDesc) {
                prBssDesc->fgIsConnecting = false;
            }

            /* 3.3 Free STA-REC */
            if (prStaRec != prAisBssInfo->prStaRecOfAP) {
                cnmStaRecFree(prAdapter, prStaRec);
            }

            if (prAisBssInfo->eConnectionState ==
                PARAM_MEDIA_STATE_CONNECTED) {
#if CFG_SUPPORT_ROAMING
                eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
#endif
            } else if (CHECK_FOR_TIMEOUT(rCurrentTime,
                                         prAisFsmInfo->rJoinReqTime,
                                         SEC_TO_SYSTIME(AIS_JOIN_TIMEOUT))) {
                /* 4.a temrminate join operation */
                eNextState = AIS_STATE_JOIN_FAILURE;
            } else {
                /* 20210419 frog: Won't retry join if supplicant
                 * SME. Require upper layer trigger connection
                 * again.
                 */
                eNextState = AIS_STATE_JOIN_FAILURE;

                if (prJoinCompMsg->rJoinStatus == WLAN_STATUS_JOIN_ABORT) {
                    prConnSettings->fgIsDisconnectedByNonRequest = true;
                    DBGLOG(AIS, WARN, "Join abort, disconnect\n");
                }
                DBGLOG(AIS, WARN, "Join fail, disconnect\n");
            }
        }
    }
    DBGLOG(AIS, STATE, "Joined BSS eBand %d channel %d ucChannelBw %d\n",
           prAisBssInfo->eBand, prAisBssInfo->ucPrimaryChannel,
           rlmDomainGetChannelBw(prAisBssInfo->ucPrimaryChannel));
    return eNextState;
}

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Grant Msg of IBSS Create which was sent
 * by CNM to indicate that channel was changed for creating IBSS.
 *
 * @param[in] prAdapter  Pointer of ADAPTER_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmCreateIBSS(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;

    ASSERT(prAdapter);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    do {
        /* Check State */
        if (prAisFsmInfo->eCurrentState == AIS_STATE_IBSS_ALONE) {
            aisUpdateBssInfoForCreateIBSS(prAdapter);
        }
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Grant Msg of IBSS Merge which was sent
 * by CNM to indicate that channel was changed for merging IBSS.
 *
 * @param[in] prAdapter  Pointer of ADAPTER_T
 * @param[in] prStaRec   Pointer of STA_RECORD_T for merge
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmMergeIBSS(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    ENUM_AIS_STATE_T eNextState;
    P_BSS_INFO_T prAisBssInfo;

    ASSERT(prAdapter);
    ASSERT(prStaRec);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;

    do {
        eNextState = prAisFsmInfo->eCurrentState;

        switch (prAisFsmInfo->eCurrentState) {
        case AIS_STATE_IBSS_MERGE: {
            P_BSS_DESC_T prBssDesc;

            /* 4 <1.1> Change FW's Media State immediately. */
            aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

            /* 4 <1.2> Deactivate previous Peers' STA_RECORD_T in
             * Driver if have. */
            bssInitializeClientList(prAdapter, prAisBssInfo);

            /* 4 <1.3> Unmark connection flag of previous
             * BSS_DESC_T. */
            prBssDesc =
                scanSearchBssDescByBssid(prAdapter, prAisBssInfo->aucBSSID);
            if (prBssDesc != NULL) {
                prBssDesc->fgIsConnecting = false;
                prBssDesc->fgIsConnected = false;
            }
            /* 4 <1.4> Add Peers' STA_RECORD_T to Client List */
            bssAddClient(prAdapter, prAisBssInfo, prStaRec);

            /* 4 <1.5> Activate current Peer's STA_RECORD_T in
             * Driver. */
            cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
            prStaRec->fgIsMerging = false;

            /* 4 <1.6> Update BSS_INFO_T */
            aisUpdateBssInfoForMergeIBSS(prAdapter, prStaRec);

            /* 4 <1.7> Enable other features */

            /* 4 <1.8> Indicate Connected Event to Host immediately.
             */
            aisIndicationOfMediaStateToHost(prAdapter,
                                            PARAM_MEDIA_STATE_CONNECTED,
                                            false);

            /* 4 <1.9> Set the Next State of AIS FSM */
            eNextState = AIS_STATE_NORMAL_TR;

            /* 4 <1.10> Release channel privilege */
            aisFsmReleaseCh(prAdapter);
        } break;

        default:
            break;
        }

        if (eNextState != prAisFsmInfo->eCurrentState) {
            aisFsmSteps(prAdapter, eNextState);
        }
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Notification of existing IBSS was found
 *        from SCN.
 *
 * @param[in] prMsgHdr   Message of Notification of an IBSS was present.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventFoundIBSSPeer(IN P_ADAPTER_T prAdapter,
                                 IN P_MSG_HDR_T prMsgHdr)
{
    P_MSG_AIS_IBSS_PEER_FOUND_T prAisIbssPeerFoundMsg;
    P_AIS_FSM_INFO_T prAisFsmInfo;
    ENUM_AIS_STATE_T eNextState;
    P_STA_RECORD_T prStaRec;
    P_BSS_INFO_T prAisBssInfo;
    P_BSS_DESC_T prBssDesc;
    u8 fgIsMergeIn;

    ASSERT(prMsgHdr);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;

    prAisIbssPeerFoundMsg = (P_MSG_AIS_IBSS_PEER_FOUND_T)prMsgHdr;

    ASSERT(prAisIbssPeerFoundMsg->ucBssIndex ==
           prAdapter->prAisBssInfo->ucBssIndex);

    prStaRec = prAisIbssPeerFoundMsg->prStaRec;
    ASSERT(prStaRec);

    fgIsMergeIn = prAisIbssPeerFoundMsg->fgIsMergeIn;

    cnmMemFree(prAdapter, prMsgHdr);

    eNextState = prAisFsmInfo->eCurrentState;
    switch (prAisFsmInfo->eCurrentState) {
    case AIS_STATE_IBSS_ALONE: {
        /* 4 <1> An IBSS Peer 'merged in'. */
        if (fgIsMergeIn) {
            /* 4 <1.1> Change FW's Media State immediately. */
            aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

            /* 4 <1.2> Add Peers' STA_RECORD_T to Client List */
            bssAddClient(prAdapter, prAisBssInfo, prStaRec);

            /* 4 <1.3> Mark connection flag of BSS_DESC_T. */
            prBssDesc =
                scanSearchBssDescByBssid(prAdapter, prAisBssInfo->aucBSSID);

            if (prBssDesc != NULL) {
                prBssDesc->fgIsConnecting = false;
                prBssDesc->fgIsConnected = true;
            } else {
                ASSERT(0);  /* Should be able to find a
                               BSS_DESC_T here. */
            }

            /* 4 <1.4> Activate current Peer's STA_RECORD_T in
             * Driver. */
            prStaRec->fgIsQoS = false;  /* TODO(Kevin): TBD */

            cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
            prStaRec->fgIsMerging = false;

            /* 4 <1.6> sync. to firmware */
            nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

            /* 4 <1.7> Indicate Connected Event to Host immediately.
             */
            aisIndicationOfMediaStateToHost(prAdapter,
                                            PARAM_MEDIA_STATE_CONNECTED,
                                            false);

            /* 4 <1.8> indicate PM for connected */
            nicPmIndicateBssConnected(prAdapter,
                                      prAdapter->prAisBssInfo->ucBssIndex);

            /* 4 <1.9> Set the Next State of AIS FSM */
            eNextState = AIS_STATE_NORMAL_TR;

            /* 4 <1.10> Release channel privilege */
            aisFsmReleaseCh(prAdapter);
        }
        /* 4 <2> We need 'merge out' to this IBSS */
        else {
            /* 4 <2.1> Get corresponding BSS_DESC_T */
            prBssDesc = scanSearchBssDescByTA(prAdapter,
                                              prStaRec->aucMacAddr);

            prAisFsmInfo->prTargetBssDesc = prBssDesc;

            /* 4 <2.2> Set the Next State of AIS FSM */
            eNextState = AIS_STATE_IBSS_MERGE;
        }
    } break;

    case AIS_STATE_NORMAL_TR: {
        /* 4 <3> An IBSS Peer 'merged in'. */
        if (fgIsMergeIn) {
            /* 4 <3.1> Add Peers' STA_RECORD_T to Client List */
            bssAddClient(prAdapter, prAisBssInfo, prStaRec);

            /* 4 <3.2> Activate current Peer's STA_RECORD_T in
             * Driver. */
            prStaRec->fgIsQoS = false;  /* TODO(Kevin): TBD */

            cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
            prStaRec->fgIsMerging = false;
        }
        /* 4 <4> We need 'merge out' to this IBSS */
        else {
            /* 4 <4.1> Get corresponding BSS_DESC_T */
            prBssDesc = scanSearchBssDescByTA(prAdapter,
                                              prStaRec->aucMacAddr);

            prAisFsmInfo->prTargetBssDesc = prBssDesc;

            /* 4 <4.2> Set the Next State of AIS FSM */
            eNextState = AIS_STATE_IBSS_MERGE;
        }
    } break;

    default:
        break;
    }

    if (eNextState != prAisFsmInfo->eCurrentState) {
        aisFsmSteps(prAdapter, eNextState);
    }
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate the Media State to HOST
 *
 * @param[in] eConnectionState   Current Media State
 * @param[in] fgDelayIndication  Set true for postponing the Disconnect
 * Indication.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisIndicationOfMediaStateToHost(IN P_ADAPTER_T prAdapter,
                                     ENUM_PARAM_MEDIA_STATE_T eConnectionState,
                                     u8 fgDelayIndication)
{
    EVENT_CONNECTION_STATUS rEventConnStatus;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_BSS_INFO_T prAisBssInfo;
    P_AIS_FSM_INFO_T prAisFsmInfo;

    DEBUGFUNC("aisIndicationOfMediaStateToHost()");

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prAisBssInfo = prAdapter->prAisBssInfo;
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    DBGLOG(AIS, LOUD,
           "AIS indicate Media State to Host Current State [%d]\n",
           prAisBssInfo->eConnectionState);

    /* NOTE(Kevin): Move following line to aisChangeMediaState() macro per
     * CM's request. */
    /* prAisBssInfo->eConnectionState = eConnectionState; */

    /* For indicating the Disconnect Event only if current media state is
     * disconnected and we didn't do indication yet.
     */
    if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED) {
        if (prAisBssInfo->eConnectionStateIndicated == eConnectionState) {
            return;
        }
    }

    if (!fgDelayIndication) {
        /* 4 <0> Cancel Delay Timer */
        cnmTimerStopTimer(prAdapter,
                          &prAisFsmInfo->rIndicationOfDisconnectTimer);

        /* 4 <1> Fill EVENT_CONNECTION_STATUS */
        rEventConnStatus.ucMediaStatus = (u8)eConnectionState;

        if (eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
            rEventConnStatus.ucReasonOfDisconnect =
                DISCONNECT_REASON_CODE_RESERVED;

            if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
                rEventConnStatus.ucInfraMode = (u8)NET_TYPE_INFRA;
                rEventConnStatus.u2AID = prAisBssInfo->u2AssocId;
                rEventConnStatus.u2ATIMWindow = 0;
            } else if (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
                rEventConnStatus.ucInfraMode = (u8)NET_TYPE_IBSS;
                rEventConnStatus.u2AID = 0;
                rEventConnStatus.u2ATIMWindow = prAisBssInfo->u2ATIMWindow;
            } else {
                ASSERT(0);
            }

            COPY_SSID(rEventConnStatus.aucSsid, rEventConnStatus.ucSsidLen,
                      prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

            COPY_MAC_ADDR(rEventConnStatus.aucBssid,
                          prAisBssInfo->aucBSSID);

            rEventConnStatus.u2BeaconPeriod =
                prAisBssInfo->u2BeaconInterval;
            rEventConnStatus.u4FreqInKHz =
                nicChannelNum2Freq(prAisBssInfo->ucPrimaryChannel);

            switch (prAisBssInfo->ucNonHTBasicPhyType) {
            case PHY_TYPE_HR_DSSS_INDEX:
                rEventConnStatus.ucNetworkType = (u8)PARAM_NETWORK_TYPE_DS;
                break;

            case PHY_TYPE_ERP_INDEX:
                rEventConnStatus.ucNetworkType =
                    (u8)PARAM_NETWORK_TYPE_OFDM24;
                break;

            case PHY_TYPE_OFDM_INDEX:
                rEventConnStatus.ucNetworkType =
                    (u8)PARAM_NETWORK_TYPE_OFDM5;
                break;

            default:
                ASSERT(0);
                rEventConnStatus.ucNetworkType = (u8)PARAM_NETWORK_TYPE_DS;
                break;
            }
        } else {
            /* Clear the pmkid cache while media disconnect */
            secClearPmkid(prAdapter);
            rEventConnStatus.ucReasonOfDisconnect =
                prAisBssInfo->ucReasonOfDisconnect;
        }

        /* 4 <2> Indication */
        nicMediaStateChange(prAdapter, prAdapter->prAisBssInfo->ucBssIndex,
                            &rEventConnStatus);
        prAisBssInfo->eConnectionStateIndicated = eConnectionState;
    } else {
        /* NOTE: Only delay the Indication of Disconnect Event */
        ASSERT(eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED);

#if CFG_SUPPORT_DBDC_TC6
        if (prAisBssInfo->ucReasonOfDisconnect ==
            DISCONNECT_REASON_CODE_DBDC_REASSOCIATION ||
            cnmGetp2pSapBssInfo(prAdapter) ||
            prAdapter->rWifiVar.fgDbDcModeEn) {
            DBGLOG(
                AIS, INFO,
                "[DBDC] Postpone the indication of Disconnect for %d seconds\n",
                DBDC_AIS_REASSOC_DELAY_DISCONNECT_EVENT_TIME / 1000);

            cnmTimerStartTimer(prAdapter,
                               &prAisFsmInfo->rIndicationOfDisconnectTimer,
                               DBDC_AIS_REASSOC_DELAY_DISCONNECT_EVENT_TIME);
        } else
#endif
        {
            DBGLOG(AIS, INFO,
                   "Postpone the indication of Disconnect for %d seconds\n",
                   prConnSettings->ucDelayTimeOfDisconnectEvent);

            cnmTimerStartTimer(
                prAdapter, &prAisFsmInfo->rIndicationOfDisconnectTimer,
                SEC_TO_MSEC(prConnSettings->ucDelayTimeOfDisconnectEvent));
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate an Event of "Media Disconnect" to HOST
 *
 * @param[in] u4Param  Unused timer parameter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisPostponedEventOfDisconnTimeout(IN P_ADAPTER_T prAdapter,
                                       unsigned long ulParamPtr)
{
    P_BSS_INFO_T prAisBssInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;

    prAisBssInfo = prAdapter->prAisBssInfo;
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    DBGLOG(AIS, EVENT, "aisPostponedEventOfDisconnTimeout\n");

    /* 4 <1> Deactivate previous AP's STA_RECORD_T in Driver if have. */
    if (prAisBssInfo->prStaRecOfAP) {
        /* cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP,
         * STA_STATE_1); */

        prAisBssInfo->prStaRecOfAP = (P_STA_RECORD_T)NULL;
    }
    /* 4 <2> Remove pending connection request */
    aisFsmIsRequestPending(prAdapter, AIS_REQUEST_RECONNECT, true);
    prConnSettings->fgIsDisconnectedByNonRequest = true;
    prAisBssInfo->u2DeauthReason += REASON_CODE_BEACON_TIMEOUT;
    /* 4 <3> Indicate Disconnected Event to Host immediately. */
    aisFsmStateAbort(prAdapter, DISCONNECT_REASON_CODE_RADIO_LOST, false);
    // aisIndicationOfMediaStateToHost(prAdapter,
    // PARAM_MEDIA_STATE_DISCONNECTED, false);
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
void aisUpdateBssInfoForJOIN(IN P_ADAPTER_T prAdapter,
                             P_STA_RECORD_T prStaRec,
                             P_SW_RFB_T prAssocRspSwRfb)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_INFO_T prAisBssInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_WLAN_ASSOC_RSP_FRAME_T prAssocRspFrame;
    P_BSS_DESC_T prBssDesc;
    u16 u2IELength;
    u8 *pucIE;
    PARAM_SSID_T rParamSsid;

    DEBUGFUNC("aisUpdateBssInfoForJOIN()");

    ASSERT(prStaRec);
    ASSERT(prAssocRspSwRfb);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prAssocRspFrame = (P_WLAN_ASSOC_RSP_FRAME_T)prAssocRspSwRfb->pvHeader;

    DBGLOG(AIS, INFO, "Update AIS_BSS_INFO_T and apply settings to MAC\n");

    /* 3 <1> Update BSS_INFO_T from AIS_FSM_INFO_T or User Settings */
    /* 4 <1.1> Setup Operation Mode */
    prAisBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;

    /* 4 <1.2> Setup SSID */
    COPY_SSID(prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen,
              prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

    /* 4 <1.3> Setup Channel, Band */
    prAisBssInfo->ucPrimaryChannel =
        prAisFsmInfo->prTargetBssDesc->ucChannelNum;
    prAisBssInfo->eBand = prAisFsmInfo->prTargetBssDesc->eBand;

    /* 3 <2> Update BSS_INFO_T from STA_RECORD_T */
    /* 4 <2.1> Save current AP's STA_RECORD_T and current AID */
    prAisBssInfo->prStaRecOfAP = prStaRec;
    DBGLOG(AIS, EVENT, "Update target Sta Rec to :%d\n", prStaRec->ucIndex);
    prAisBssInfo->u2AssocId = prStaRec->u2AssocId;

    /* 4 <2.2> Setup Capability */
    prAisBssInfo->u2CapInfo = prStaRec->u2CapInfo;  /* Use AP's Cap Info as
                                                       BSS Cap Info */

    if (prAisBssInfo->u2CapInfo & CAP_INFO_SHORT_PREAMBLE) {
        prAisBssInfo->fgIsShortPreambleAllowed = true;
    } else {
        prAisBssInfo->fgIsShortPreambleAllowed = false;
    }

#if CFG_SUPPORT_TDLS
    prAisBssInfo->fgTdlsIsProhibited = prStaRec->fgTdlsIsProhibited;
    prAisBssInfo->fgTdlsIsChSwProhibited = prStaRec->fgTdlsIsChSwProhibited;
#endif

    /* 4 <2.3> Setup PHY Attributes and Basic Rate Set/Operational Rate Set
     */
    prAisBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;

    prAisBssInfo->ucNonHTBasicPhyType = prStaRec->ucNonHTBasicPhyType;

    prAisBssInfo->u2OperationalRateSet = prStaRec->u2OperationalRateSet;
    prAisBssInfo->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;

    nicTxUpdateBssDefaultRate(prAisBssInfo);

    /* 3 <3> Update BSS_INFO_T from SW_RFB_T (Association Resp Frame) */
    /* 4 <3.1> Setup BSSID */
    COPY_MAC_ADDR(prAisBssInfo->aucBSSID, prAssocRspFrame->aucBSSID);

    u2IELength =
        (u16)((prAssocRspSwRfb->u2PacketLen -
               prAssocRspSwRfb->u2HeaderLen) -
              (OFFSET_OF(WLAN_ASSOC_RSP_FRAME_T, aucInfoElem[0]) -
               WLAN_MAC_MGMT_HEADER_LEN));
    pucIE = prAssocRspFrame->aucInfoElem;

    /* 4 <3.2> Parse WMM and setup QBSS flag */
    /* Parse WMM related IEs and configure HW CRs accordingly */
    mqmProcessAssocRsp(prAdapter, prAssocRspSwRfb, pucIE, u2IELength);

    prAisBssInfo->fgIsQBSS = prStaRec->fgIsQoS;

    /* 3 <4> Update BSS_INFO_T from BSS_DESC_T */
    if (prAisBssInfo->ucSSIDLen) {
        rParamSsid.u4SsidLen = prAisBssInfo->ucSSIDLen;
        COPY_SSID(rParamSsid.aucSsid, rParamSsid.u4SsidLen,
                  prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen);
        prBssDesc = scanSearchBssDescByBssidAndSsid(
            prAdapter, prAisBssInfo->aucBSSID, true, &rParamSsid);
    } else {
        prBssDesc = scanSearchBssDescByBssidAndChanNum(
            prAdapter, prConnSettings->aucBSSID, true,
            prConnSettings->ucChannelNum);
    }

    if (prBssDesc) {
        prBssDesc->fgIsConnecting = false;
        prBssDesc->fgIsConnected = true;
        prBssDesc->ucJoinFailureCount = 0;
        /* 4 <4.1> Setup MIB for current BSS */
        prAisBssInfo->u2BeaconInterval = prBssDesc->u2BeaconInterval;
    } else {
        /* should never happen */
        ASSERT(0);
    }

    /* NOTE: Defer ucDTIMPeriod updating to when beacon is received after
     * connection */
    prAisBssInfo->ucDTIMPeriod = 0;
    prAisBssInfo->u2ATIMWindow = 0;

    prAisBssInfo->ucBeaconTimeoutCount = AIS_BEACON_TIMEOUT_COUNT_INFRA;
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
    prAisBssInfo->ucRoamSkipTimes = ROAMING_ONE_AP_SKIP_TIMES;
    prAisBssInfo->fgGoodRcpiArea = false;
    prAisBssInfo->fgPoorRcpiArea = false;
#endif

#if CFG_SUPPORT_QUIET
    prAisBssInfo->ucQuietPeriod = 0;
    prAisBssInfo->u2QuietOffset = 0;
    prAisBssInfo->u2QuietDuration = 0;
    prAisBssInfo->fgRequestQuietInterval = false;
    prAisBssInfo->fgIsInQuietInterval = false;
#endif

    /* 4 <4.2> Update HT information and set channel */
    /* Record HT related parameters in rStaRec and rBssInfo
     * Note: it shall be called before nicUpdateBss()
     */
    rlmProcessAssocRsp(prAdapter, prAssocRspSwRfb, pucIE, u2IELength);

    secPostUpdateAddr(prAdapter, prAdapter->prAisBssInfo);

    /* 4 <4.3> Sync with firmware for BSS-INFO */
    nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

    /* 4 <4.4> *DEFER OPERATION* nicPmIndicateBssConnected() will be invoked
     */
    /* inside scanProcessBeaconAndProbeResp() after 1st beacon is received
     */
}

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will create an Ad-Hoc network and start sending Beacon
 * Frames.
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisUpdateBssInfoForCreateIBSS(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_INFO_T prAisBssInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    if (prAisBssInfo->fgIsBeaconActivated) {
        return;
    }

    /* 3 <1> Update BSS_INFO_T per Network Basis */
    /* 4 <1.1> Setup Operation Mode */
    prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;

    /* 4 <1.2> Setup SSID */
    COPY_SSID(prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen,
              prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

    /* 4 <1.3> Clear current AP's STA_RECORD_T and current AID */
    prAisBssInfo->prStaRecOfAP = (P_STA_RECORD_T)NULL;
    prAisBssInfo->u2AssocId = 0;

    /* 4 <1.4> Setup Channel, Band and Phy Attributes */
    prAisBssInfo->ucPrimaryChannel = prConnSettings->ucAdHocChannelNum;
    prAisBssInfo->eBand = prConnSettings->eAdHocBand;

    if (prAisBssInfo->eBand == BAND_2G4) {
        /* Depend on eBand */
        prAisBssInfo->ucPhyTypeSet =
            prAdapter->rWifiVar.ucAvailablePhyTypeSet &
            PHY_TYPE_SET_802_11BGN;
        /* Depend on eCurrentOPMode and ucPhyTypeSet */
        prAisBssInfo->ucConfigAdHocAPMode = AD_HOC_MODE_MIXED_11BG;
    } else {
        /* Depend on eBand */
        prAisBssInfo->ucPhyTypeSet =
            prAdapter->rWifiVar.ucAvailablePhyTypeSet &
            PHY_TYPE_SET_802_11ANAC;
        /* Depend on eCurrentOPMode and ucPhyTypeSet */
        prAisBssInfo->ucConfigAdHocAPMode = AD_HOC_MODE_11A;
    }

    /* 4 <1.5> Setup MIB for current BSS */
    prAisBssInfo->u2BeaconInterval = prConnSettings->u2BeaconPeriod;
    prAisBssInfo->ucDTIMPeriod = 0;
    prAisBssInfo->u2ATIMWindow = prConnSettings->u2AtimWindow;

    prAisBssInfo->ucBeaconTimeoutCount = AIS_BEACON_TIMEOUT_COUNT_ADHOC;

    if (prConnSettings->eEncStatus == ENUM_ENCRYPTION1_ENABLED ||
        prConnSettings->eEncStatus == ENUM_ENCRYPTION2_ENABLED ||
        prConnSettings->eEncStatus == ENUM_ENCRYPTION3_ENABLED) {
        prAisBssInfo->fgIsProtection = true;
    } else {
        prAisBssInfo->fgIsProtection = false;
    }

    /* 3 <2> Update BSS_INFO_T common part */
    ibssInitForAdHoc(prAdapter, prAisBssInfo);
    /* 4 <2.1> Initialize client list */
    bssInitializeClientList(prAdapter, prAisBssInfo);

    /* 3 <3> Set MAC HW */
    /* 4 <3.1> Setup channel and bandwidth */
    rlmBssInitForAPandIbss(prAdapter, prAisBssInfo);

    /* 4 <3.2> use command packets to inform firmware */
    nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

    /* 4 <3.3> enable beaconing */
    bssUpdateBeaconContent(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

    /* 4 <3.4> Update AdHoc PM parameter */
    nicPmIndicateBssCreated(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

    /* 3 <4> Set ACTIVE flag. */
    prAisBssInfo->fgIsBeaconActivated = true;
    prAisBssInfo->fgHoldSameBssidForIBSS = true;

    /* 3 <5> Start IBSS Alone Timer */
    cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer,
                       SEC_TO_MSEC(AIS_IBSS_ALONE_TIMEOUT_SEC));
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will update the contain of BSS_INFO_T for AIS network
 * once the existing IBSS was found.
 *
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisUpdateBssInfoForMergeIBSS(IN P_ADAPTER_T prAdapter,
                                  IN P_STA_RECORD_T prStaRec)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_INFO_T prAisBssInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_BSS_DESC_T prBssDesc;
    /* u16 u2IELength; */
    /* u8 * pucIE; */

    ASSERT(prStaRec);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer);

    if (!prAisBssInfo->fgIsBeaconActivated) {
        /* 3 <1> Update BSS_INFO_T per Network Basis */
        /* 4 <1.1> Setup Operation Mode */
        prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;

        /* 4 <1.2> Setup SSID */
        COPY_SSID(prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen,
                  prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

        /* 4 <1.3> Clear current AP's STA_RECORD_T and current AID */
        prAisBssInfo->prStaRecOfAP = (P_STA_RECORD_T)NULL;
        prAisBssInfo->u2AssocId = 0;
    }
    /* 3 <2> Update BSS_INFO_T from STA_RECORD_T */
    /* 4 <2.1> Setup Capability */
    prAisBssInfo->u2CapInfo = prStaRec->u2CapInfo;  /* Use Peer's Cap Info as
                                                       IBSS Cap Info */

    if (prAisBssInfo->u2CapInfo & CAP_INFO_SHORT_PREAMBLE) {
        prAisBssInfo->fgIsShortPreambleAllowed = true;
        prAisBssInfo->fgUseShortPreamble = true;
    } else {
        prAisBssInfo->fgIsShortPreambleAllowed = false;
        prAisBssInfo->fgUseShortPreamble = false;
    }

    /* 7.3.1.4 For IBSS, the Short Slot Time subfield shall be set to 0. */
    prAisBssInfo->fgUseShortSlotTime = false;  /* Set to false for AdHoc */
    prAisBssInfo->u2CapInfo &= ~CAP_INFO_SHORT_SLOT_TIME;

    if (prAisBssInfo->u2CapInfo & CAP_INFO_PRIVACY) {
        prAisBssInfo->fgIsProtection = true;
    } else {
        prAisBssInfo->fgIsProtection = false;
    }

    /* 4 <2.2> Setup PHY Attributes and Basic Rate Set/Operational Rate Set
     */
    prAisBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;

    prAisBssInfo->ucNonHTBasicPhyType = prStaRec->ucNonHTBasicPhyType;

    prAisBssInfo->u2OperationalRateSet = prStaRec->u2OperationalRateSet;
    prAisBssInfo->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;

    rateGetDataRatesFromRateSet(prAisBssInfo->u2OperationalRateSet,
                                prAisBssInfo->u2BSSBasicRateSet,
                                prAisBssInfo->aucAllSupportedRates,
                                &prAisBssInfo->ucAllSupportedRatesLen);

    /* 3 <3> X Update BSS_INFO_T from SW_RFB_T (Association Resp Frame) */

    /* 3 <4> Update BSS_INFO_T from BSS_DESC_T */
    prBssDesc = scanSearchBssDescByTA(prAdapter, prStaRec->aucMacAddr);
    if (prBssDesc) {
        prBssDesc->fgIsConnecting = false;
        prBssDesc->fgIsConnected = true;

        /* 4 <4.1> Setup BSSID */
        COPY_MAC_ADDR(prAisBssInfo->aucBSSID, prBssDesc->aucBSSID);

        /* 4 <4.2> Setup Channel, Band */
        prAisBssInfo->ucPrimaryChannel = prBssDesc->ucChannelNum;
        prAisBssInfo->eBand = prBssDesc->eBand;

        /* 4 <4.3> Setup MIB for current BSS */
        prAisBssInfo->u2BeaconInterval = prBssDesc->u2BeaconInterval;
        prAisBssInfo->ucDTIMPeriod = 0;
        prAisBssInfo->u2ATIMWindow = 0;  /* TBD(Kevin) */

        prAisBssInfo->ucBeaconTimeoutCount = AIS_BEACON_TIMEOUT_COUNT_ADHOC;
    } else {
        /* should never happen */
        ASSERT(0);
    }

    /* 3 <5> Set MAC HW */
    /* 4 <5.1> Find Lowest Basic Rate Index for default TX Rate of MMPDU */
    nicTxUpdateBssDefaultRate(prAisBssInfo);

    /* 4 <5.2> Setup channel and bandwidth */
    rlmBssInitForAPandIbss(prAdapter, prAisBssInfo);

    /* 4 <5.3> use command packets to inform firmware */
    nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

    /* 4 <5.4> enable beaconing */
    bssUpdateBeaconContent(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

    /* 4 <5.5> Update AdHoc PM parameter */
    nicPmIndicateBssConnected(prAdapter,
                              prAdapter->prAisBssInfo->ucBssIndex);

    /* 3 <6> Set ACTIVE flag. */
    prAisBssInfo->fgIsBeaconActivated = true;
    prAisBssInfo->fgHoldSameBssidForIBSS = true;
}
#endif

#if (CFG_SUPPORT_ADHOC || CFG_SUPPORT_PROBE_REQ_REPORT)
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
u8 aisValidateProbeReq(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb,
                       OUT u32 *pu4ControlFlags)
{
    P_WLAN_MAC_MGMT_HEADER_T prMgtHdr;
    P_BSS_INFO_T prBssInfo;
    P_IE_SSID_T prIeSsid = (P_IE_SSID_T)NULL;
    u8 *pucIE;
    u16 u2IELength;
    u16 u2Offset = 0;
    u8 fgReplyProbeResp = false;
    P_AIS_FSM_INFO_T prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    ASSERT(prSwRfb);
    ASSERT(pu4ControlFlags);

    prBssInfo = prAdapter->prAisBssInfo;

    /* 4 <1> Parse Probe Req IE and Get IE ptr (SSID, Supported Rate IE,
     * ...) */
    prMgtHdr = (P_WLAN_MAC_MGMT_HEADER_T)prSwRfb->pvHeader;

    u2IELength = prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen;
    pucIE = (u8 *)((unsigned long)prSwRfb->pvHeader + prSwRfb->u2HeaderLen);

    IE_FOR_EACH(pucIE, u2IELength, u2Offset){
        if (IE_ID(pucIE) == ELEM_ID_SSID) {
            if ((!prIeSsid) && (IE_LEN(pucIE) <= ELEM_MAX_LEN_SSID)) {
                prIeSsid = (P_IE_SSID_T)pucIE;
            }

            break;
        }
    }  /* end of IE_FOR_EACH */

    /* 4 <2> Check network conditions */

    if (prBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
        if ((prIeSsid) &&
            ((prIeSsid->ucLength == BC_SSID_LEN) ||  /* WILDCARD SSID */
             EQUAL_SSID(prBssInfo->aucSSID, prBssInfo->ucSSIDLen,  /* CURRENT
                                                                      SSID
                                                                    */
                        prIeSsid->aucSSID, prIeSsid->ucLength))) {
            fgReplyProbeResp = true;
        }
    }

    if (prAisFsmInfo->u4AisPacketFilter & PARAM_PACKET_FILTER_PROBE_REQ) {
        DBGLOG(AIS, INFO, "[AIS] RX Probe Req Frame\n");
        kalIndicateRxMgmtFrame(prAdapter->prGlueInfo, prSwRfb);
    }

    return fgReplyProbeResp;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will modify and update necessary information to firmware
 *        for disconnection handling
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 *
 * @retval None
 */
/*----------------------------------------------------------------------------*/
void aisFsmDisconnect(IN P_ADAPTER_T prAdapter, IN u8 fgDelayIndication)
{
    P_BSS_INFO_T prAisBssInfo;

    ASSERT(prAdapter);

    prAisBssInfo = prAdapter->prAisBssInfo;

    nicPmIndicateBssAbort(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

#if CFG_SUPPORT_ADHOC
    if (prAisBssInfo->fgIsBeaconActivated) {
        nicUpdateBeaconIETemplate(prAdapter, IE_UPD_METHOD_DELETE_ALL,
                                  prAdapter->prAisBssInfo->ucBssIndex, 0,
                                  NULL,
                                  0);

        prAisBssInfo->fgIsBeaconActivated = false;
    }
#endif

    rlmBssAborted(prAdapter, prAisBssInfo);

    /* 4 <3> Unset the fgIsConnected flag of BSS_DESC_T and send Deauth if
     * needed. */
    if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
        {
            if (prAdapter->rWifiVar.ucTpTestMode !=
                ENUM_TP_TEST_MODE_NORMAL) {
                nicEnterTPTestMode(prAdapter, TEST_MODE_NONE);
            }
        }

        if (prAisBssInfo->ucReasonOfDisconnect ==
            DISCONNECT_REASON_CODE_RADIO_LOST) {
            scanRemoveBssDescByBssid(prAdapter, prAisBssInfo->aucBSSID);

            /* remove from scanning results as well */
            wlanClearBssInScanningResult(prAdapter, prAisBssInfo->aucBSSID);

            /* trials for re-association */
            if (fgDelayIndication) {
                aisFsmIsRequestPending(prAdapter, AIS_REQUEST_RECONNECT,
                                       true);
                aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);
            }
        } else {
            scanRemoveConnFlagOfBssDescByBssid(prAdapter,
                                               prAisBssInfo->aucBSSID);
        }

        if (fgDelayIndication) {
            if (prAisBssInfo->eCurrentOPMode != OP_MODE_IBSS) {
                prAisBssInfo->fgHoldSameBssidForIBSS = false;
            }
        } else {
            prAisBssInfo->fgHoldSameBssidForIBSS = false;
        }
    } else {
        prAisBssInfo->fgHoldSameBssidForIBSS = false;
    }

    /* 4 <4> Change Media State immediately. */
    if (prAisBssInfo->ucReasonOfDisconnect !=
        DISCONNECT_REASON_CODE_REASSOCIATION) {
        aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_DISCONNECTED);

#if CFG_STR_DHCP_RENEW_OFFLOAD
        if (prAisBssInfo->fgIsDhcpAcked) {
            prAisBssInfo->fgIsDhcpAcked = false;
            prAisBssInfo->u4DhcpRenewIntv = 0;
            kalMemZero(prAisBssInfo->aucDhcpServerIpAddr,
                       sizeof(prAisBssInfo->aucDhcpServerIpAddr));
        }
#endif

        /* 4 <4.1> sync. with firmware */
        nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
    }

    if (!fgDelayIndication) {
        /* 4 <5> Deactivate previous AP's STA_RECORD_T or all Clients in
         * Driver if have. */
        if (prAisBssInfo->prStaRecOfAP) {
            /* cnmStaRecChangeState(prAdapter,
             * prAisBssInfo->prStaRecOfAP, STA_STATE_1); */
            prAisBssInfo->prStaRecOfAP = (P_STA_RECORD_T)NULL;
        }
    }
#if CFG_SUPPORT_ROAMING
    roamingFsmRunEventAbort(prAdapter);

    /* clear pending roaming connection request */
    aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, true);
    aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, true);
#endif

    /* 4 <6> Indicate Disconnected Event to Host */
    aisIndicationOfMediaStateToHost(prAdapter,
                                    PARAM_MEDIA_STATE_DISCONNECTED,
                                    fgDelayIndication);

    /* 4 <7> Trigger AIS FSM */
    aisFsmSteps(prAdapter, AIS_STATE_IDLE);
}

static void aisFsmRunEventScanDoneTimeOut(IN P_ADAPTER_T prAdapter,
                                          unsigned long ulParam)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    ENUM_AIS_STATE_T eNextState;
    P_CONNECTION_SETTINGS_T prConnSettings;

    DEBUGFUNC("aisFsmRunEventScanDoneTimeOut()");

    ASSERT(prAdapter);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    DBGLOG(AIS, STATE, "aisFsmRunEventScanDoneTimeOut Current[%d]\n",
           prAisFsmInfo->eCurrentState);

    /* report all scanned frames to upper layer to avoid scanned frame is
     * timeout */
    /* must be put before kalScanDone */
    /* scanReportBss2Cfg80211(prAdapter,BSS_TYPE_INFRASTRUCTURE,NULL); */

    prConnSettings->fgIsScanReqIssued = false;
    kalScanDone(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_AIS_INDEX,
                WLAN_STATUS_SUCCESS);
    eNextState = prAisFsmInfo->eCurrentState;

    switch (prAisFsmInfo->eCurrentState) {
    case AIS_STATE_SCAN:
        prAisFsmInfo->u4ScanIELength = 0;
        eNextState = AIS_STATE_IDLE;
        break;
    case AIS_STATE_ONLINE_SCAN:
        /* reset scan IE buffer */
        prAisFsmInfo->u4ScanIELength = 0;
#if CFG_SUPPORT_ROAMING
        eNextState = aisFsmRoamingScanResultsUpdate(prAdapter);
#else
        eNextState = AIS_STATE_NORMAL_TR;
#endif
        break;
    default:
        break;
    }

    /* try to stop scan in CONNSYS */
    aisFsmStateAbort_SCAN(prAdapter);

    /* wlanQueryDebugCode(prAdapter); */ /* display current SCAN FSM in FW,
                      debug use */

    if (eNextState != prAisFsmInfo->eCurrentState) {
        aisFsmSteps(prAdapter, eNextState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will run aisBssBeaconTimeout
 *
 * @param[in] u4Param  Unused timer parameter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmBeaconLostTimeOut(IN P_ADAPTER_T prAdapter,
                             unsigned long ulParamPtr)
{
    DBGLOG(AIS, STATE, "Beacon Lost timer expires\n");
    ASSERT(prAdapter);
    aisBssBeaconTimeout(prAdapter, DISCONNECT_REASON_CODE_RADIO_LOST);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate an Event of "Background Scan Time-Out" to
 * AIS FSM.
 *
 * @param[in] u4Param  Unused timer parameter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventBGSleepTimeOut(IN P_ADAPTER_T prAdapter,
                                  unsigned long ulParamPtr)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    ENUM_AIS_STATE_T eNextState;

    DEBUGFUNC("aisFsmRunEventBGSleepTimeOut()");

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    eNextState = prAisFsmInfo->eCurrentState;

    switch (prAisFsmInfo->eCurrentState) {
    case AIS_STATE_WAIT_FOR_NEXT_SCAN:
        DBGLOG(AIS, LOUD,
               "EVENT - SCAN TIMER: Idle End - Current Time = %u\n",
               kalGetTimeTick());

        eNextState = AIS_STATE_LOOKING_FOR;

        SET_NET_PWR_STATE_ACTIVE(prAdapter,
                                 prAdapter->prAisBssInfo->ucBssIndex);

        break;

    default:
        break;
    }

    /* Call aisFsmSteps() when we are going to change AIS STATE */
    if (eNextState != prAisFsmInfo->eCurrentState) {
        aisFsmSteps(prAdapter, eNextState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate an Event of "IBSS ALONE Time-Out" to AIS
 * FSM.
 *
 * @param[in] u4Param  Unused timer parameter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventIbssAloneTimeOut(IN P_ADAPTER_T prAdapter,
                                    unsigned long ulParamPtr)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    ENUM_AIS_STATE_T eNextState;

    DEBUGFUNC("aisFsmRunEventIbssAloneTimeOut()");

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    eNextState = prAisFsmInfo->eCurrentState;

    switch (prAisFsmInfo->eCurrentState) {
    case AIS_STATE_IBSS_ALONE:

        /* There is no one participate in our AdHoc during this TIMEOUT
         * Interval so go back to search for a valid IBSS again.
         */

        DBGLOG(AIS, LOUD, "EVENT-IBSS ALONE TIMER: Start pairing\n");

        prAisFsmInfo->fgTryScan = true;

        /* abort timer */
        aisFsmReleaseCh(prAdapter);

        /* Pull back to SEARCH to find candidate again */
        eNextState = AIS_STATE_SEARCH;

        break;

    default:
        break;
    }

    /* Call aisFsmSteps() when we are going to change AIS STATE */
    if (eNextState != prAisFsmInfo->eCurrentState) {
        aisFsmSteps(prAdapter, eNextState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate an Event of "Join Time-Out" to AIS FSM.
 *
 * @param[in] u4Param  Unused timer parameter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventJoinTimeout(IN P_ADAPTER_T prAdapter,
                               unsigned long ulParamPtr)
{
    P_BSS_INFO_T prAisBssInfo;
    P_AIS_FSM_INFO_T prAisFsmInfo;
    ENUM_AIS_STATE_T eNextState;
    u32 rCurrentTime;

    DEBUGFUNC("aisFsmRunEventJoinTimeout()");

    prAisBssInfo = prAdapter->prAisBssInfo;
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    eNextState = prAisFsmInfo->eCurrentState;

    GET_CURRENT_SYSTIME(&rCurrentTime);

    switch (prAisFsmInfo->eCurrentState) {
    case AIS_STATE_JOIN:
        DBGLOG(AIS, LOUD, "EVENT- JOIN TIMEOUT\n");

        /* 1. Do abort JOIN */
        aisFsmStateAbort_JOIN(prAdapter);

        /* 2. Increase Join Failure Count */
        prAisFsmInfo->prTargetStaRec->ucJoinFailureCount++;

        if (prAisFsmInfo->prTargetStaRec->ucJoinFailureCount <
            JOIN_MAX_RETRY_FAILURE_COUNT) {
            /* 3.1 Retreat to AIS_STATE_SEARCH state for next try */
            eNextState = AIS_STATE_SEARCH;
        } else if (prAisBssInfo->eConnectionState ==
                   PARAM_MEDIA_STATE_CONNECTED) {
            /* roaming cases */
            /* 3.2 Retreat to AIS_STATE_WAIT_FOR_NEXT_SCAN state for
             * next try */
            eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
        } else {
            /* 3.4 Retreat to AIS_STATE_JOIN_FAILURE to terminate
             * join operation */
            eNextState = AIS_STATE_JOIN_FAILURE;
        }

        break;

    case AIS_STATE_NORMAL_TR:
        /* 1. release channel */
        aisFsmReleaseCh(prAdapter);
        prAisFsmInfo->fgIsInfraChannelFinished = true;

        /* 2. process if there is pending scan */
        if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN,
                                   true) == true) {
            wlanClearScanningResult(prAdapter);
            eNextState = AIS_STATE_ONLINE_SCAN;
        }
        /* 3. Process for pending roaming scan */
        else if (aisFsmIsRequestPending(prAdapter,
                                        AIS_REQUEST_ROAMING_SEARCH,
                                        true) == true) {
            eNextState = AIS_STATE_LOOKING_FOR;
        }
        /* 4. Process for pending roaming scan */
        else if (aisFsmIsRequestPending(prAdapter,
                                        AIS_REQUEST_ROAMING_CONNECT,
                                        true) == true) {
            eNextState = AIS_STATE_SEARCH;
        } else if (aisFsmIsRequestPending(prAdapter,
                                          AIS_REQUEST_REMAIN_ON_CHANNEL,
                                          true) == true) {
            eNextState = AIS_STATE_REQ_REMAIN_ON_CHANNEL;
        }

        break;

    default:
        /* release channel */
        aisFsmReleaseCh(prAdapter);
        prAisFsmInfo->fgIsInfraChannelFinished = true;
        DBGLOG(AIS, WARN, "Join Timeout in state(%d)\n",
               prAisFsmInfo->eCurrentState);
        break;
    }

    /* Call aisFsmSteps() when we are going to change AIS STATE */
    if (eNextState != prAisFsmInfo->eCurrentState) {
        aisFsmSteps(prAdapter, eNextState);
    }
}

void aisFsmRunEventDeauthTimeout(IN P_ADAPTER_T prAdapter,
                                 unsigned long ulParamPtr)
{
    DBGLOG(AIS, EVENT, "aisDeauthTimeout\n");
    aisDeauthXmitComplete(prAdapter, NULL, TX_RESULT_LIFE_TIMEOUT);
}

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
void aisRxMcsCollectionTimeout(IN P_ADAPTER_T prAdapter,
                               unsigned long ulParamPtr)
{
    static u8 ucSmapleCnt;
    u8 ucStaIdx = 0;

    if (prAdapter->prAisBssInfo->prStaRecOfAP == NULL) {
        goto out;
    }

    ucStaIdx = prAdapter->prAisBssInfo->prStaRecOfAP->ucIndex;

    if (prAdapter->arStaRec[ucStaIdx].fgIsValid &&
        prAdapter->arStaRec[ucStaIdx].fgIsInUse) {
        prAdapter->arStaRec[ucStaIdx].au4RxVect0Que[ucSmapleCnt] =
            prAdapter->arStaRec[ucStaIdx].u4RxVector0;
        prAdapter->arStaRec[ucStaIdx].au4RxVect1Que[ucSmapleCnt] =
            prAdapter->arStaRec[ucStaIdx].u4RxVector1;
        ucSmapleCnt = (ucSmapleCnt + 1) % MCS_INFO_SAMPLE_CNT;
    }

out:
    cnmTimerStartTimer(prAdapter, &prAdapter->rRxMcsInfoTimer, 100);
}
#endif

#if defined(CFG_TEST_MGMT_FSM) && (CFG_TEST_MGMT_FSM != 0)
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisTest(void)
{
    P_MSG_AIS_ABORT_T prAisAbortMsg;
    P_CONNECTION_SETTINGS_T prConnSettings;
    u8 aucSSID[] = "pci-11n";
    u8 ucSSIDLen = 7;

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    /* Set Connection Request Issued Flag */
    prConnSettings->fgIsConnReqIssued = true;
    prConnSettings->ucSSIDLen = ucSSIDLen;
    kalMemCopy(prConnSettings->aucSSID, aucSSID, ucSSIDLen);

    prAisAbortMsg = (P_MSG_AIS_ABORT_T)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                                   sizeof(MSG_AIS_ABORT_T));
    if (!prAisAbortMsg) {
        ASSERT(0);  /* Can't trigger SCAN FSM */
        return;
    }

    prAisAbortMsg->rMsgHdr.eMsgId = MID_HEM_AIS_FSM_ABORT;

    mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prAisAbortMsg,
                MSG_SEND_METHOD_BUF);

    wifi_send_msg(INDX_WIFI, MSG_ID_WIFI_IST, 0);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is used to handle OID_802_11_BSSID_LIST_SCAN
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 * \param[in] prSsid     Pointer of SSID_T if specified
 * \param[in] pucIe      Pointer to buffer of extra information elements to be
 * attached \param[in] u4IeLength Length of information elements
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisFsmScanRequest(IN P_ADAPTER_T prAdapter, IN P_PARAM_SSID_T prSsid,
                       IN u8 *pucIe, IN u32 u4IeLength)
{
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_BSS_INFO_T prAisBssInfo;
    P_AIS_FSM_INFO_T prAisFsmInfo;

    DEBUGFUNC("aisFsmScanRequest()");

    ASSERT(prAdapter);
    ASSERT(u4IeLength <= MAX_IE_LENGTH);

    prAisBssInfo = prAdapter->prAisBssInfo;
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    if (!prConnSettings->fgIsScanReqIssued) {
        prConnSettings->fgIsScanReqIssued = true;

        if (prSsid == NULL) {
            prAisFsmInfo->ucScanSSIDNum = 0;
        } else {
            prAisFsmInfo->ucScanSSIDNum = 1;

            COPY_SSID(prAisFsmInfo->arScanSSID[0].aucSsid,
                      prAisFsmInfo->arScanSSID[0].u4SsidLen,
                      prSsid->aucSsid,
                      prSsid->u4SsidLen);
        }

        if (u4IeLength > 0) {
            prAisFsmInfo->u4ScanIELength = u4IeLength;
            kalMemCopy(prAisFsmInfo->aucScanIEBuf, pucIe, u4IeLength);
        } else {
            prAisFsmInfo->u4ScanIELength = 0;
        }

        if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR) {
            if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
                prAisFsmInfo->fgIsInfraChannelFinished == false) {
                /* 802.1x might not finished yet, pend it for
                 * later handling .. */
                aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
            } else {
                if (prAisFsmInfo->fgIsChannelGranted == true) {
                    DBGLOG(
                        AIS, WARN,
                        "Scan Request with channel granted for join operation: %d, %d",
                        prAisFsmInfo->fgIsChannelGranted,
                        prAisFsmInfo->fgIsChannelRequested);
                }

                /* start online scan */
                wlanClearScanningResult(prAdapter);
                aisFsmSteps(prAdapter, AIS_STATE_ONLINE_SCAN);
            }
        } else if (prAisFsmInfo->eCurrentState == AIS_STATE_IDLE) {
            wlanClearScanningResult(prAdapter);
            aisFsmSteps(prAdapter, AIS_STATE_SCAN);
        } else {
            aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
        }
    } else {
        DBGLOG(AIS, WARN, "Scan Request dropped. (state: %d)\n",
               prAisFsmInfo->eCurrentState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is used to handle OID_802_11_BSSID_LIST_SCAN
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 * \param[in] ucSsidNum  Number of SSID
 * \param[in] prSsid     Pointer to the array of SSID_T if specified
 * \param[in] pucIe      Pointer to buffer of extra information elements to be
 * attached \param[in] u4IeLength Length of information elements
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisFsmScanRequestAdv(IN P_ADAPTER_T prAdapter, IN u8 ucSsidNum,
                          IN P_PARAM_SSID_T prSsid, IN u8 ucChannelListNum,
                          IN P_RF_CHANNEL_INFO_T prChnlInfoList,
                          IN u8 *pucIe,
                          IN u32 u4IeLength)
{
    u32 i;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_BSS_INFO_T prAisBssInfo;
    P_AIS_FSM_INFO_T prAisFsmInfo;

    DEBUGFUNC("aisFsmScanRequestAdv()");

    ASSERT(prAdapter);
    ASSERT(ucSsidNum <= SCN_SSID_MAX_NUM);
    ASSERT(u4IeLength <= MAX_IE_LENGTH);

    prAisBssInfo = prAdapter->prAisBssInfo;
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    if (!prConnSettings->fgIsScanReqIssued) {
        prConnSettings->fgIsScanReqIssued = true;

        if (ucSsidNum == 0) {
            prAisFsmInfo->ucScanSSIDNum = 0;
        } else {
            prAisFsmInfo->ucScanSSIDNum = ucSsidNum;

            for (i = 0; i < ucSsidNum; i++) {
                COPY_SSID(prAisFsmInfo->arScanSSID[i].aucSsid,
                          prAisFsmInfo->arScanSSID[i].u4SsidLen,
                          prSsid[i].aucSsid, prSsid[i].u4SsidLen);
            }
        }

        if (u4IeLength > 0) {
            prAisFsmInfo->u4ScanIELength = u4IeLength;
            kalMemCopy(prAisFsmInfo->aucScanIEBuf, pucIe, u4IeLength);
        } else {
            prAisFsmInfo->u4ScanIELength = 0;
        }

#if CFG_SCAN_CHANNEL_SPECIFIED
        if (ucChannelListNum) {
            prAisFsmInfo->ucScanChannelListNum = ucChannelListNum;
            kalMemCopy(prAisFsmInfo->arScanChnlInfoList, prChnlInfoList,
                       sizeof(RF_CHANNEL_INFO_T) *
                       prAisFsmInfo->ucScanChannelListNum);
        } else {
            prAisFsmInfo->ucScanChannelListNum = 0;
        }
#endif

        if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR) {
            if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
                prAisFsmInfo->fgIsInfraChannelFinished == false) {
                /* 802.1x might not finished yet, pend it for
                 * later handling .. */
                aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
            } else {
                if (prAisFsmInfo->fgIsChannelGranted == true) {
                    DBGLOG(
                        AIS, WARN,
                        "Scan Request with channel granted for join operation: %d, %d",
                        prAisFsmInfo->fgIsChannelGranted,
                        prAisFsmInfo->fgIsChannelRequested);
                }

                /* start online scan */
                wlanClearScanningResult(prAdapter);
                aisFsmSteps(prAdapter, AIS_STATE_ONLINE_SCAN);
            }
        } else if (prAisFsmInfo->eCurrentState == AIS_STATE_IDLE) {
            wlanClearScanningResult(prAdapter);
            aisFsmSteps(prAdapter, AIS_STATE_SCAN);
        } else {
            aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
        }
    } else {
        DBGLOG(AIS, WARN, "Scan Request dropped. (state: %d)\n",
               prAisFsmInfo->eCurrentState);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is invoked when CNM granted channel privilege
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventChGrant(IN P_ADAPTER_T prAdapter,
                           IN P_MSG_HDR_T prMsgHdr)
{
    P_BSS_INFO_T prAisBssInfo;
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_MSG_CH_GRANT_T prMsgChGrant;
    u8 ucTokenID;
    u32 u4GrantInterval;

    ASSERT(prAdapter);
    ASSERT(prMsgHdr);

    prAisBssInfo = prAdapter->prAisBssInfo;
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prMsgChGrant = (P_MSG_CH_GRANT_T)prMsgHdr;

    ucTokenID = prMsgChGrant->ucTokenID;
    u4GrantInterval = prMsgChGrant->u4GrantInterval;
#if CFG_SUPPORT_DBDC
    if (prAisBssInfo->eDBDCBand == ENUM_BAND_AUTO) {
        prAisBssInfo->eDBDCBand = prMsgChGrant->eDBDCBand;
    }
#endif

#if CFG_SISO_SW_DEVELOP
    /* Driver record granted CH in BSS info */
    prAisBssInfo->fgIsGranted = true;
    prAisBssInfo->eBandGranted = prMsgChGrant->eRfBand;
    prAisBssInfo->ucPrimaryChannelGranted = prMsgChGrant->ucPrimaryChannel;
#endif

    /* 1. free message */
    cnmMemFree(prAdapter, prMsgHdr);

    if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN &&
        prAisFsmInfo->ucSeqNumOfChReq == ucTokenID) {
        /* 2. channel privilege has been approved */
        prAisFsmInfo->u4ChGrantedInterval = u4GrantInterval;

        /* 3. state transition to join/ibss-alone/ibss-merge */
        /* 3.1 set timeout timer in cases join could not be completed */
        cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer,
                           prAisFsmInfo->u4ChGrantedInterval -
                           AIS_JOIN_CH_GRANT_THRESHOLD);
        /* 3.2 set local variable to indicate join timer is ticking */
        prAisFsmInfo->fgIsInfraChannelFinished = false;

        /* 3.3 switch to join state */
        aisFsmSteps(prAdapter, AIS_STATE_JOIN);

        prAisFsmInfo->fgIsChannelGranted = true;
    } else if (prAisFsmInfo->eCurrentState ==
               AIS_STATE_REQ_REMAIN_ON_CHANNEL &&
               prAisFsmInfo->ucSeqNumOfChReq == ucTokenID) {
        /* 2. channel privilege has been approved */
        prAisFsmInfo->u4ChGrantedInterval = u4GrantInterval;

        /* 3.1 set timeout timer in cases upper layer
         * cancel_remain_on_channel never comes */
        cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer,
                           prAisFsmInfo->u4ChGrantedInterval);

        /* 3.2 switch to remain_on_channel state */
        aisFsmSteps(prAdapter, AIS_STATE_REMAIN_ON_CHANNEL);

        /* 3.3. indicate upper layer for channel ready */
        kalReadyOnChannel(prAdapter->prGlueInfo,
                          prAisFsmInfo->rChReqInfo.u8Cookie,
                          prAisFsmInfo->rChReqInfo.eBand,
                          prAisFsmInfo->rChReqInfo.eSco,
                          prAisFsmInfo->rChReqInfo.ucChannelNum,
                          prAisFsmInfo->rChReqInfo.u4DurationMs);

        prAisFsmInfo->fgIsChannelGranted = true;
    } else {  /* mismatched grant */
        /* 2. return channel privilege to CNM immediately */
        aisFsmReleaseCh(prAdapter);
    }
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
void aisFsmReleaseCh(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_MSG_CH_ABORT_T prMsgChAbort;

    ASSERT(prAdapter);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    if (prAisFsmInfo->fgIsChannelGranted == true ||
        prAisFsmInfo->fgIsChannelRequested == true) {
        prAisFsmInfo->fgIsChannelRequested = false;
        prAisFsmInfo->fgIsChannelGranted = false;

        /* 1. return channel privilege to CNM immediately */
        prMsgChAbort = (P_MSG_CH_ABORT_T)cnmMemAlloc(prAdapter,
                                                     RAM_TYPE_MSG,
                                                     sizeof(MSG_CH_ABORT_T));
        if (!prMsgChAbort) {
            ASSERT(0);  /* Can't release Channel to CNM */
            return;
        }

        prMsgChAbort->rMsgHdr.eMsgId = MID_MNY_CNM_CH_ABORT;
        prMsgChAbort->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
        prMsgChAbort->ucTokenID = prAisFsmInfo->ucSeqNumOfChReq;
#if CFG_SUPPORT_DBDC
        prMsgChAbort->eDBDCBand = ENUM_BAND_AUTO;
#endif
        mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prMsgChAbort,
                    MSG_SEND_METHOD_BUF);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to inform AIS that corresponding beacon has not
 *           been received for a while and probing is not successful
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisBssBeaconTimeout(IN P_ADAPTER_T prAdapter, IN u8 ucReasonCode)
{
    P_BSS_INFO_T prAisBssInfo;
    u8 fgDoAbortIndication = false;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_AIS_FSM_INFO_T prAisFsmInfo;
#if CFG_SUPPORT_DBDC_TC6
    P_CNM_INFO_T prCnmInfo;
#endif

    ASSERT(prAdapter);

    prAisBssInfo = prAdapter->prAisBssInfo;
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
#if CFG_SUPPORT_DBDC_TC6
    prCnmInfo = &prAdapter->rCnmInfo;
#endif
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    GET_CURRENT_SYSTIME(&(prAisFsmInfo->rJoinReqTime));

    /* 4 <1> Diagnose Connection for Beacon Timeout Event */
    if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
        if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
            P_STA_RECORD_T prStaRec = prAisBssInfo->prStaRecOfAP;

            if (prStaRec) {
                fgDoAbortIndication = true;
            }
        } else if (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
            fgDoAbortIndication = true;
        }
    }
    /* 4 <2> invoke abort handler */
    if (fgDoAbortIndication) {
        prConnSettings->fgIsDisconnectedByNonRequest = false;
        DBGLOG(AIS, EVENT, "aisBssBeaconTimeout\n");

#ifdef CFG_STR_DEAUTH_DELAY
        if (ucReasonCode == BEACON_TIMEOUT_EVENT_DUE_2_RX_DEAUTH_IN_STR) {
            int iCount = 0;
            DBGLOG(AIS, STATE, "[STR]: Deauth From STR (%d) \r\n",
                   kalPmResumeState());
            netif_carrier_off(prAdapter->prGlueInfo->prDevHandler);

            while ((kalPmResumeState() != 1) && (iCount < 400)) {
                kalMsleep(20);
                iCount++;
            }

            DBGLOG(AIS, STATE, "[STR]: PM Resume State (%d, %d)\r\n",
                   kalPmResumeState(), iCount);

            aisFsmStateAbort(prAdapter,
                             DISCONNECT_REASON_CODE_DEAUTHENTICATED,
                             true);
        } else
#endif
        {
            /* 20210326 frog: Once BCN timeout, disconnect
             * imediately. */
            prConnSettings->fgIsConnReqIssued = false;
            prCnmInfo->fgSkipDbdcDisable = true;

            if (ucReasonCode ==
                BEACON_TIMEOUT_REASON_DUE_2_DBDC_RECONNECT) {
                aisFsmStateAbort(
                    prAdapter, DISCONNECT_REASON_CODE_DBDC_REASSOCIATION,
                    true);
            } else {
                aisFsmStateAbort(prAdapter,
                                 DISCONNECT_REASON_CODE_RADIO_LOST,
                                 true);
            }
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to inform AIS that corresponding beacon has not
 *           been received for a while and probing is not successful
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void aisBssLinkDown(IN P_ADAPTER_T prAdapter)
{
    P_BSS_INFO_T prAisBssInfo;
    u8 fgDoAbortIndication = false;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_AIS_FSM_INFO_T prAisFsmInfo;

    ASSERT(prAdapter);

    prAisBssInfo = prAdapter->prAisBssInfo;
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    if (!prAisFsmInfo) {
        return;
    }

    /* 4 <1> Diagnose Connection for Beacon Timeout Event */
    if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
        if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
            P_STA_RECORD_T prStaRec = prAisBssInfo->prStaRecOfAP;

            if (prStaRec) {
                fgDoAbortIndication = true;
            }
        } else if (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
            fgDoAbortIndication = true;
        }
    }
    /* 4 <2> invoke abort handler */
    if (fgDoAbortIndication) {
        prConnSettings->fgIsDisconnectedByNonRequest = true;
        DBGLOG(AIS, EVENT, "aisBssLinkDown\n");
        aisFsmStateAbort(prAdapter, DISCONNECT_REASON_CODE_DISASSOCIATED,
                         false);
        cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rDeauthDoneTimer);
        aisDeauthXmitComplete(prAdapter, NULL, TX_RESULT_LIFE_TIMEOUT);
    } else {
        DBGLOG(AIS, EVENT, "Skip aisBssLinkDown (state=%d)\n",
               prAisBssInfo->eConnectionState);
    }

    /* kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
     * WLAN_STATUS_SCAN_COMPLETE, NULL, 0); */
}
#if CFG_SUPPORT_DBDC_TC6
u8 aisBssChangeNSS(IN P_ADAPTER_T prAdapter, IN u8 fgDbdcEn)
{
    P_BSS_INFO_T prAisBssInfo;

    ASSERT(prAdapter);

    prAisBssInfo = prAdapter->prAisBssInfo;

    /* Update AIS NSS */
    if (fgDbdcEn && prAisBssInfo->ucNss == prAdapter->rWifiVar.ucNSS) {
        prAisBssInfo->ucNss = 1;
        return true;
    } else if (!fgDbdcEn &&
               prAisBssInfo->ucNss != prAdapter->rWifiVar.ucNSS) {
        prAisBssInfo->ucNss = prAdapter->rWifiVar.ucNSS;
        return true;
    }

    return false;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to inform AIS that DEAUTH frame has been
 *           sent and thus state machine could go ahead
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 * \param[in] prMsduInfo Pointer of MSDU_INFO_T for DEAUTH frame
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
aisDeauthXmitComplete(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo,
                      IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_INFO_T prAisBssInfo;
    u8 ucBssIndex = 0;

    ASSERT(prAdapter);

    ucBssIndex = prMsduInfo->ucBssIndex;
    if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
        DBGLOG(AIS, INFO, "Use default, invalid index = %d\n", ucBssIndex);
        ucBssIndex = AIS_DEFAULT_INDEX;
    }

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = (prAdapter->aprBssInfo[ucBssIndex]);
#if CFG_SUPPORT_802_11W
    /* Notify completion after encrypted deauth frame tx done */
    if (prAisBssInfo->encryptedDeauthIsInProcess == true) {
        if (!completion_done(&prAisBssInfo->rDeauthComp)) {
            DBGLOG(AIS, EVENT, "Complete rDeauthComp\n");
            complete(&prAisBssInfo->rDeauthComp);
        }
    }
    prAisBssInfo->encryptedDeauthIsInProcess = false;
#endif
    if (rTxDoneStatus == TX_RESULT_SUCCESS ||
        rTxDoneStatus == TX_RESULT_DROPPED_IN_DRIVER) {
        cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rDeauthDoneTimer);
    }

    if (prAisFsmInfo->eCurrentState == AIS_STATE_DISCONNECTING) {
        DBGLOG(AIS, EVENT, "aisDeauthXmitComplete, status code: %d\n",
               rTxDoneStatus);
        if (rTxDoneStatus != TX_RESULT_DROPPED_IN_DRIVER &&
            rTxDoneStatus != TX_RESULT_QUEUE_CLEARANCE) {
            aisFsmStateAbort(prAdapter,
                             DISCONNECT_REASON_CODE_NEW_CONNECTION,
                             false);
        }
    } else {
        DBGLOG(AIS, WARN,
               "DEAUTH frame transmitted without further handling,"
               "status code: %d\n",
               rTxDoneStatus);
    }

    return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_ROAMING
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will indicate an Event of "Looking for a candidate due
 * to weak signal" to AIS FSM.
 *
 * @param[in] u4ReqScan  Requesting Scan or not
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmRunEventRoamingDiscovery(IN P_ADAPTER_T prAdapter,
                                    u32 u4ReqScan)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;
    ENUM_AIS_REQUEST_TYPE_T eAisRequest;

    DBGLOG(AIS, LOUD, "aisFsmRunEventRoamingDiscovery()\n");

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    /* search candidates by best rssi */
    prConnSettings->eConnectionPolicy = CONNECT_BY_SSID_BEST_RSSI;
    prConnSettings->fgIsConnByBssidIssued = false;

    /* TODO: Stop roaming event in FW */
#if CFG_SUPPORT_WFD
#if CFG_ENABLE_WIFI_DIRECT
    {
        /* Check WFD is running */
        P_WFD_CFG_SETTINGS_T prWfdCfgSettings = (P_WFD_CFG_SETTINGS_T)NULL;

        prWfdCfgSettings = &(prAdapter->rWifiVar.rWfdConfigureSettings);
        if ((prWfdCfgSettings->ucWfdEnable != 0)) {
            DBGLOG(ROAMING, INFO, "WFD is running. Stop roaming.\n");
            roamingFsmRunEventRoam(prAdapter);
            roamingFsmRunEventFail(prAdapter,
                                   ROAMING_FAIL_REASON_NOCANDIDATE);
            return;
        }
    }
#endif
#endif

    /* results are still new */
    if (!u4ReqScan) {
        roamingFsmRunEventRoam(prAdapter);
        eAisRequest = AIS_REQUEST_ROAMING_CONNECT;
    } else {
        if (prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN ||
            prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR) {
            eAisRequest = AIS_REQUEST_ROAMING_CONNECT;
        } else {
            eAisRequest = AIS_REQUEST_ROAMING_SEARCH;
        }
    }

    if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR &&
        prAisFsmInfo->fgIsInfraChannelFinished == true) {
        if (eAisRequest == AIS_REQUEST_ROAMING_SEARCH) {
            aisFsmSteps(prAdapter, AIS_STATE_LOOKING_FOR);
        } else {
            aisFsmSteps(prAdapter, AIS_STATE_SEARCH);
        }
    } else {
        aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, true);
        aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT,
                               true);

        aisFsmInsertRequest(prAdapter, eAisRequest);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Update the time of ScanDone for roaming and transit to Roam state.
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
ENUM_AIS_STATE_T aisFsmRoamingScanResultsUpdate(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_ROAMING_INFO_T prRoamingFsmInfo;
    ENUM_AIS_STATE_T eNextState;

    DBGLOG(AIS, LOUD, "->aisFsmRoamingScanResultsUpdate()\n");

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prRoamingFsmInfo =
        (P_ROAMING_INFO_T)&(prAdapter->rWifiVar.rRoamingInfo);

    roamingFsmScanResultsUpdate(prAdapter);

    eNextState = prAisFsmInfo->eCurrentState;
    if (prRoamingFsmInfo->eCurrentState == ROAMING_STATE_DISCOVERY) {
        roamingFsmRunEventRoam(prAdapter);
        eNextState = AIS_STATE_SEARCH;
    } else if (prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR) {
        eNextState = AIS_STATE_SEARCH;
    } else if (prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN) {
        eNextState = AIS_STATE_NORMAL_TR;
    }

    return eNextState;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will modify and update necessary information to firmware
 *        for disconnection of last AP before switching to roaming bss.
 *
 * @param IN prAdapter          Pointer to the Adapter structure.
 *           prTargetStaRec     Target of StaRec of roaming
 *
 * @retval None
 */
/*----------------------------------------------------------------------------*/
void aisFsmRoamingDisconnectPrevAP(IN P_ADAPTER_T prAdapter,
                                   IN P_STA_RECORD_T prTargetStaRec)
{
    P_BSS_INFO_T prAisBssInfo;

    DBGLOG(AIS, EVENT, "aisFsmRoamingDisconnectPrevAP()");

    ASSERT(prAdapter);

    prAisBssInfo = prAdapter->prAisBssInfo;

    nicPmIndicateBssAbort(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

    /* Not invoke rlmBssAborted() here to avoid prAisBssInfo->fg40mBwAllowed
     * to be reset. RLM related parameters will be reset again when handling
     * association response in rlmProcessAssocRsp(). 20110413
     */
    /* rlmBssAborted(prAdapter, prAisBssInfo); */

    /* 4 <3> Unset the fgIsConnected flag of BSS_DESC_T and send Deauth if
     * needed. */
    if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
        authSendDeauthFrame(prAdapter, prAisBssInfo,
                            prAisBssInfo->prStaRecOfAP,
                            (P_SW_RFB_T)NULL,
                            REASON_CODE_DEAUTH_LEAVING_BSS,
                            (PFN_TX_DONE_HANDLER)NULL);

        scanRemoveConnFlagOfBssDescByBssid(prAdapter,
                                           prAisBssInfo->aucBSSID);
    }

    /* 4 <4> Change Media State immediately. */
    aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_DISCONNECTED);

    /* 4 <4.1> sync. with firmware */
    prTargetStaRec->ucBssIndex = (MAX_BSS_INDEX + 1);  /* Virtial BSSID */
    nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

    secRemoveBssBcEntry(prAdapter, prAisBssInfo, true);
    prTargetStaRec->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
}
#endif

#if CFG_SUPPORT_ROAMING || CFG_SUPPORT_SAME_BSS_REASSOC
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will update the contain of BSS_INFO_T for AIS network
 * once the roaming was completed.
 *
 * @param IN prAdapter          Pointer to the Adapter structure.
 *           prStaRec           StaRec of roaming AP
 *           prAssocRspSwRfb
 *
 * @retval None
 */
/*----------------------------------------------------------------------------*/
void aisUpdateBssInfoForRoamingAP(IN P_ADAPTER_T prAdapter,
                                  IN P_STA_RECORD_T prStaRec,
                                  IN P_SW_RFB_T prAssocRspSwRfb)
{
    P_BSS_INFO_T prAisBssInfo;

    DBGLOG(AIS, LOUD, "aisUpdateBssInfoForRoamingAP()");

    ASSERT(prAdapter);

    prAisBssInfo = prAdapter->prAisBssInfo;

    /* 4 <1.1> Change FW's Media State immediately. */
    aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

    /* 4 <1.2> Deactivate previous AP's STA_RECORD_T in Driver if have. */
    if ((prAisBssInfo->prStaRecOfAP) &&
        (prAisBssInfo->prStaRecOfAP != prStaRec) &&
        (prAisBssInfo->prStaRecOfAP->fgIsInUse)) {
        /* cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP,
         * STA_STATE_1); */
        cnmStaRecFree(prAdapter, prAisBssInfo->prStaRecOfAP);
    }

    /* 4 <1.4> Update BSS_INFO_T */
    aisUpdateBssInfoForJOIN(prAdapter, prStaRec, prAssocRspSwRfb);

    /* 4 <1.3> Activate current AP's STA_RECORD_T in Driver. */
    cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);

    /* 4 <1.6> Indicate Connected Event to Host immediately. */
    /* Require BSSID, Association ID, Beacon Interval.. from AIS_BSS_INFO_T
     */
    aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_CONNECTED,
                                    false);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Check if there is any pending request and remove it (optional)
 *
 * @param prAdapter
 *        eReqType
 *        bRemove
 *
 * @return true
 *         false
 */
/*----------------------------------------------------------------------------*/
u8 aisFsmIsRequestPending(IN P_ADAPTER_T prAdapter,
                          IN ENUM_AIS_REQUEST_TYPE_T eReqType,
                          IN u8 bRemove)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_AIS_REQ_HDR_T prPendingReqHdr, prPendingReqHdrNext;

    ASSERT(prAdapter);
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    /* traverse through pending request list */
    LINK_FOR_EACH_ENTRY_SAFE(prPendingReqHdr, prPendingReqHdrNext,
                             &(prAisFsmInfo->rPendingReqList), rLinkEntry,
                             AIS_REQ_HDR_T){
        /* check for specified type */
        if (prPendingReqHdr->eReqType == eReqType) {
            /* check if need to remove */
            if (bRemove == true) {
                LINK_REMOVE_KNOWN_ENTRY(&(prAisFsmInfo->rPendingReqList),
                                        &(prPendingReqHdr->rLinkEntry));

                cnmMemFree(prAdapter, prPendingReqHdr);
            }

            return true;
        }
    }

    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Get next pending request
 *
 * @param prAdapter
 *
 * @return P_AIS_REQ_HDR_T
 */
/*----------------------------------------------------------------------------*/
P_AIS_REQ_HDR_T aisFsmGetNextRequest(IN P_ADAPTER_T prAdapter)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_AIS_REQ_HDR_T prPendingReqHdr;

    ASSERT(prAdapter);
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    LINK_REMOVE_HEAD(&(prAisFsmInfo->rPendingReqList), prPendingReqHdr,
                     P_AIS_REQ_HDR_T);

    return prPendingReqHdr;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Insert a new request
 *
 * @param prAdapter
 *        eReqType
 *
 * @return true
 *         false
 */
/*----------------------------------------------------------------------------*/
u8 aisFsmInsertRequest(IN P_ADAPTER_T prAdapter,
                       IN ENUM_AIS_REQUEST_TYPE_T eReqType)
{
    P_AIS_REQ_HDR_T prAisReq;
    P_AIS_FSM_INFO_T prAisFsmInfo;

    ASSERT(prAdapter);
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

    prAisReq = (P_AIS_REQ_HDR_T)cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                            sizeof(AIS_REQ_HDR_T));

    if (!prAisReq) {
        ASSERT(0);  /* Can't generate new message */
        return false;
    }

    prAisReq->eReqType = eReqType;

    /* attach request into pending request list */
    LINK_INSERT_TAIL(&prAisFsmInfo->rPendingReqList, &prAisReq->rLinkEntry);

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Flush all pending requests
 *
 * @param prAdapter
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void aisFsmFlushRequest(IN P_ADAPTER_T prAdapter)
{
    P_AIS_REQ_HDR_T prAisReq;

    ASSERT(prAdapter);

    while ((prAisReq = aisFsmGetNextRequest(prAdapter)) != NULL)
        cnmMemFree(prAdapter, prAisReq);
}

void aisFsmRunEventRemainOnChannel(IN P_ADAPTER_T prAdapter,
                                   IN P_MSG_HDR_T prMsgHdr)
{
    P_MSG_REMAIN_ON_CHANNEL_T prRemainOnChannel;
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_CONNECTION_SETTINGS_T prConnSettings;

    DEBUGFUNC("aisFsmRunEventRemainOnChannel()");

    ASSERT(prAdapter);
    ASSERT(prMsgHdr);

    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    prRemainOnChannel = (P_MSG_REMAIN_ON_CHANNEL_T)prMsgHdr;

    /* record parameters */
    prAisFsmInfo->rChReqInfo.eBand = prRemainOnChannel->eBand;
    prAisFsmInfo->rChReqInfo.eSco = prRemainOnChannel->eSco;
    prAisFsmInfo->rChReqInfo.ucChannelNum = prRemainOnChannel->ucChannelNum;
    prAisFsmInfo->rChReqInfo.u4DurationMs = prRemainOnChannel->u4DurationMs;
    prAisFsmInfo->rChReqInfo.u8Cookie = prRemainOnChannel->u8Cookie;

    if (prAisFsmInfo->eCurrentState == AIS_STATE_IDLE ||
        (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR &&
         prAisFsmInfo->fgIsInfraChannelFinished == true)) {
        /* transit to next state */
        aisFsmSteps(prAdapter, AIS_STATE_REQ_REMAIN_ON_CHANNEL);
    } else {
        aisFsmInsertRequest(prAdapter, AIS_REQUEST_REMAIN_ON_CHANNEL);
    }

    /* free messages */
    cnmMemFree(prAdapter, prMsgHdr);
}

void aisFsmRunEventCancelRemainOnChannel(IN P_ADAPTER_T prAdapter,
                                         IN P_MSG_HDR_T prMsgHdr)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_INFO_T prAisBssInfo;
    P_MSG_CANCEL_REMAIN_ON_CHANNEL_T prCancelRemainOnChannel;

    ASSERT(prAdapter);
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;

    prCancelRemainOnChannel = (P_MSG_CANCEL_REMAIN_ON_CHANNEL_T)prMsgHdr;

    /* 1. Check the cookie first */
    if (prCancelRemainOnChannel->u8Cookie ==
        prAisFsmInfo->rChReqInfo.u8Cookie) {
        /* 2. release channel privilege/request */
        if (prAisFsmInfo->eCurrentState ==
            AIS_STATE_REQ_REMAIN_ON_CHANNEL) {
            /* 2.1 elease channel */
            aisFsmReleaseCh(prAdapter);
        } else if (prAisFsmInfo->eCurrentState ==
                   AIS_STATE_REMAIN_ON_CHANNEL) {
            /* 2.1 release channel */
            aisFsmReleaseCh(prAdapter);

            /* 2.2 stop channel timeout timer */
            cnmTimerStopTimer(prAdapter,
                              &prAisFsmInfo->rChannelTimeoutTimer);
        }

        /* 3. clear pending request of remain_on_channel */
        aisFsmIsRequestPending(prAdapter, AIS_REQUEST_REMAIN_ON_CHANNEL,
                               true);

        /* 4. decide which state to retreat */
        if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
            aisFsmSteps(prAdapter, AIS_STATE_NORMAL_TR);
        } else {
            aisFsmSteps(prAdapter, AIS_STATE_IDLE);
        }
    }

    /* 5. free message */
    cnmMemFree(prAdapter, prMsgHdr);
}

void aisFsmRunEventMgmtFrameTx(IN P_ADAPTER_T prAdapter,
                               IN P_MSG_HDR_T prMsgHdr)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_MSG_MGMT_TX_REQUEST_T prMgmtTxMsg = (P_MSG_MGMT_TX_REQUEST_T)NULL;

    do {
        ASSERT((prAdapter != NULL) && (prMsgHdr != NULL));

        prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

        if (prAisFsmInfo == NULL) {
            break;
        }

        prMgmtTxMsg = (P_MSG_MGMT_TX_REQUEST_T)prMsgHdr;

        aisFuncTxMgmtFrame(prAdapter, &prAisFsmInfo->rMgmtTxInfo,
                           prMgmtTxMsg->prMgmtMsduInfo,
                           prMgmtTxMsg->u8Cookie);
    } while (false);

    if (prMsgHdr) {
        cnmMemFree(prAdapter, prMsgHdr);
    }
}

void aisFsmRunEventChannelTimeout(IN P_ADAPTER_T prAdapter,
                                  unsigned long ulParamPtr)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_BSS_INFO_T prAisBssInfo;

    DEBUGFUNC("aisFsmRunEventRemainOnChannel()");

    ASSERT(prAdapter);
    prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
    prAisBssInfo = prAdapter->prAisBssInfo;

    if (prAisFsmInfo->eCurrentState == AIS_STATE_REMAIN_ON_CHANNEL) {
        /* 1. release channel */
        aisFsmReleaseCh(prAdapter);

        /* 2. stop channel timeout timer */
        cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer);

        /* 3. expiration indication to upper layer */
        kalRemainOnChannelExpired(prAdapter->prGlueInfo,
                                  prAisFsmInfo->rChReqInfo.u8Cookie,
                                  prAisFsmInfo->rChReqInfo.eBand,
                                  prAisFsmInfo->rChReqInfo.eSco,
                                  prAisFsmInfo->rChReqInfo.ucChannelNum);

        /* 4. decide which state to retreat */
        if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
            aisFsmSteps(prAdapter, AIS_STATE_NORMAL_TR);
        } else {
            aisFsmSteps(prAdapter, AIS_STATE_IDLE);
        }
    } else {
        DBGLOG(AIS, WARN, "Unexpected remain_on_channel timeout event\n");
        DBGLOG(AIS, STATE, "CURRENT State: [%s]\n",
               apucDebugAisState[prAisFsmInfo->eCurrentState]);
    }
}

WLAN_STATUS
aisFsmRunEventMgmtFrameTxDone(IN P_ADAPTER_T prAdapter,
                              IN P_MSDU_INFO_T prMsduInfo,
                              IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
    P_AIS_FSM_INFO_T prAisFsmInfo;
    P_AIS_MGMT_TX_REQ_INFO_T prMgmtTxReqInfo =
        (P_AIS_MGMT_TX_REQ_INFO_T)NULL;
    u8 fgIsSuccess = false;

    do {
        ASSERT((prAdapter != NULL) && (prMsduInfo != NULL));

        prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
        prMgmtTxReqInfo = &(prAisFsmInfo->rMgmtTxInfo);

        if (rTxDoneStatus == TX_RESULT_SUCCESS) {
            fgIsSuccess = true;
        }

        if (prMgmtTxReqInfo->prMgmtTxMsdu == prMsduInfo) {
            kalIndicateMgmtTxStatus(prAdapter->prGlueInfo,
                                    prMgmtTxReqInfo->u8Cookie, fgIsSuccess,
                                    prMsduInfo->prPacket,
                                    (u32)prMsduInfo->u2FrameLength);

            prMgmtTxReqInfo->prMgmtTxMsdu = NULL;
        }
    } while (false);

    return WLAN_STATUS_SUCCESS;
}

WLAN_STATUS
aisFuncTxMgmtFrame(IN P_ADAPTER_T prAdapter,
                   IN P_AIS_MGMT_TX_REQ_INFO_T prMgmtTxReqInfo,
                   IN P_MSDU_INFO_T prMgmtTxMsdu, IN u64 u8Cookie)
{
    WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;
    P_MSDU_INFO_T prTxMsduInfo = (P_MSDU_INFO_T)NULL;
    P_WLAN_MAC_HEADER_T prWlanHdr = (P_WLAN_MAC_HEADER_T)NULL;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;

    do {
        ASSERT((prAdapter != NULL) && (prMgmtTxReqInfo != NULL));

        if (prMgmtTxReqInfo->fgIsMgmtTxRequested) {
            /* 1. prMgmtTxReqInfo->prMgmtTxMsdu != NULL */
            /* Packet on driver, not done yet, drop it. */
            prTxMsduInfo = prMgmtTxReqInfo->prMgmtTxMsdu;
            if (prTxMsduInfo != NULL) {
                kalIndicateMgmtTxStatus(prAdapter->prGlueInfo,
                                        prMgmtTxReqInfo->u8Cookie, false,
                                        prTxMsduInfo->prPacket,
                                        (u32)prTxMsduInfo->u2FrameLength);

                /* Leave it to TX Done handler. */
                /* cnmMgtPktFree(prAdapter, prTxMsduInfo); */
                prMgmtTxReqInfo->prMgmtTxMsdu = NULL;
            }
            /* 2. prMgmtTxReqInfo->prMgmtTxMsdu == NULL */
            /* Packet transmitted, wait tx done. (cookie issue) */
        }

        ASSERT(prMgmtTxReqInfo->prMgmtTxMsdu == NULL);

        prWlanHdr =
            (P_WLAN_MAC_HEADER_T)((unsigned long)prMgmtTxMsdu->prPacket +
                                  MAC_TX_RESERVED_FIELD);
        prStaRec = cnmGetStaRecByAddress(prAdapter,
                                         prAdapter->prAisBssInfo->ucBssIndex,
                                         prWlanHdr->aucAddr1);

        TX_SET_MMPDU(prAdapter, prMgmtTxMsdu,
                     (prStaRec != NULL) ? (prStaRec->ucBssIndex) :
                     (prAdapter->prAisBssInfo->ucBssIndex),
                     (prStaRec != NULL) ? (prStaRec->ucIndex) :
                     (STA_REC_INDEX_NOT_FOUND),
                     WLAN_MAC_MGMT_HEADER_LEN, prMgmtTxMsdu->u2FrameLength,
                     aisFsmRunEventMgmtFrameTxDone, MSDU_RATE_MODE_AUTO);

        if ((prWlanHdr->u2FrameCtrl & MASK_FRAME_TYPE) ==
            MAC_FRAME_PROBE_RSP) {
            nicTxSetPktLifeTime(prMgmtTxMsdu, 100);
            nicTxSetPktRetryLimit(prMgmtTxMsdu, 3);
        }

        prMgmtTxReqInfo->u8Cookie = u8Cookie;
        prMgmtTxReqInfo->prMgmtTxMsdu = prMgmtTxMsdu;
        prMgmtTxReqInfo->fgIsMgmtTxRequested = true;

        nicTxConfigPktControlFlag(prMgmtTxMsdu, MSDU_CONTROL_FLAG_FORCE_TX,
                                  true);

        /* send to TX queue */
        nicTxEnqueueMsdu(prAdapter, prMgmtTxMsdu);
    } while (false);

    return rWlanStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the Rx Action Frame and indicate to uppoer
 * layer if the specified conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[out] pu4ControlFlags   Control flags for replying the Probe Response
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void aisFuncValidateRxActionFrame(IN P_ADAPTER_T prAdapter,
                                  IN P_SW_RFB_T prSwRfb)
{
    P_AIS_FSM_INFO_T prAisFsmInfo = (P_AIS_FSM_INFO_T)NULL;

    DEBUGFUNC("aisFuncValidateRxActionFrame");

    do {
        ASSERT((prAdapter != NULL) && (prSwRfb != NULL));

        prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
        if (prAisFsmInfo->u4AisPacketFilter &
            PARAM_PACKET_FILTER_ACTION_FRAME) {
            /* Leave the action frame to wpa_supplicant. */
            kalIndicateRxMgmtFrame(prAdapter->prGlueInfo, prSwRfb);
        }
    } while (false);

    return;
}

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
void aisFsmRunEventBssTransition(IN P_ADAPTER_T prAdapter,
                                 IN P_MSG_HDR_T prMsgHdr)
{
    P_MSG_AIS_BSS_TRANSITION_T prMsg = (P_MSG_AIS_BSS_TRANSITION_T)prMsgHdr;
    P_AIS_SPECIFIC_BSS_INFO_T prAisSpecificBssInfo =
        &prAdapter->rWifiVar.rAisSpecificBssInfo;
    P_BSS_TRANSITION_MGT_PARAM_T prBtmParam =
        &prAisSpecificBssInfo->rBTMParam;
    enum WNM_AIS_BSS_TRANSITION eTransType = BSS_TRANSITION_MAX_NUM;
    P_BSS_DESC_T prBssDesc =
        prAdapter->rWifiVar.rAisFsmInfo.prTargetBssDesc;
    u8 fgNeedBtmResponse = false;
    u8 ucStatus = BSS_TRANSITION_MGT_STATUS_UNSPECIFIED;
    u8 ucRcvToken = 0;
    static u8 aucChnlList[MAXIMUM_OPERATION_CHANNEL_LIST];

    if (!prMsg) {
        DBGLOG(AIS, WARN, "Msg Header is NULL\n");
        return;
    }
    eTransType = prMsg->eTransitionType;
    fgNeedBtmResponse = prMsg->fgNeedResponse;
    ucRcvToken = prMsg->ucToken;

    DBGLOG(AIS, INFO, "Transition Type: %d\n", eTransType);
    aisCollectNeighborAP(prAdapter, prMsg->pucCandList,
                         prMsg->u2CandListLen,
                         prMsg->ucValidityInterval);
    cnmMemFree(prAdapter, prMsgHdr);
    /* Solicited BTM request: the case we're waiting btm request
    ** after send btm query before roaming scan
    */
    if (prBtmParam->ucDialogToken == ucRcvToken) {
        prBtmParam->fgPendingResponse = fgNeedBtmResponse;
        prBtmParam->fgUnsolicitedReq = false;

        switch (prAdapter->rWifiVar.rRoamingInfo.eCurrentState) {
        case ROAMING_STATE_REQ_CAND_LIST:
            roamingFsmSteps(prAdapter, ROAMING_STATE_DISCOVERY);
            return;
        case ROAMING_STATE_DISCOVERY:
        /* this case need to fall through */
        case ROAMING_STATE_ROAM:
            ucStatus = BSS_TRANSITION_MGT_STATUS_UNSPECIFIED;
            goto send_response;
        default:
            /* not solicited btm request, but dialog token matches
            ** occasionally.
            */
            break;
        }
    }
    prBtmParam->fgUnsolicitedReq = true;
    /* Unsolicited BTM request */
    switch (eTransType) {
    case BSS_TRANSITION_DISASSOC:
        ucStatus = BSS_TRANSITION_MGT_STATUS_ACCEPT;
        break;
    case BSS_TRANSITION_REQ_ROAMING: {
        P_NEIGHBOR_AP_T prNeiAP = NULL;
        P_LINK_T prUsingLink =
            &prAisSpecificBssInfo->rNeighborApList.rUsingLink;
        u8 i = 0;
        u8 ucChannel = 0;
        u8 ucChnlCnt = 0;
        u16 u2LeftTime = 0;

        if (!prBssDesc) {
            DBGLOG(AIS, ERROR, "Target Bss Desc is NULL\n");
            break;
        }
        prBtmParam->fgPendingResponse = fgNeedBtmResponse;
        kalMemZero(aucChnlList, sizeof(aucChnlList));
        LINK_FOR_EACH_ENTRY(prNeiAP, prUsingLink, rLinkEntry,
                            NEIGHBOR_AP_T){
            ucChannel = prNeiAP->ucChannel;
            for (i = 0; i < ucChnlCnt && ucChannel != aucChnlList[i]; i++)
                ;
            if (i == ucChnlCnt) {
                ucChnlCnt++;
            }
        }
        /* reserve 1 second for association */
        u2LeftTime =
            prBtmParam->u2DisassocTimer * prBssDesc->u2BeaconInterval -
            1000;
        /* check if left time is enough to do partial scan, if not
        ** enought, reject directly
        */
        if (u2LeftTime < ucChnlCnt * prBssDesc->u2BeaconInterval) {
            ucStatus = BSS_TRANSITION_MGT_STATUS_UNSPECIFIED;
            goto send_response;
        }
        roamingFsmSteps(prAdapter, ROAMING_STATE_DISCOVERY);
        return;
    }
    default:
        ucStatus = BSS_TRANSITION_MGT_STATUS_ACCEPT;
        break;
    }
send_response:
    if (fgNeedBtmResponse && prAdapter->prAisBssInfo &&
        prAdapter->prAisBssInfo->prStaRecOfAP) {
        prBtmParam->ucStatusCode = ucStatus;
        prBtmParam->ucTermDelay = 0;
        kalMemZero(prBtmParam->aucTargetBssid, MAC_ADDR_LEN);
        prBtmParam->u2OurNeighborBssLen = 0;
        prBtmParam->fgPendingResponse = false;
        wnmSendBTMResponseFrame(prAdapter,
                                prAdapter->prAisBssInfo->prStaRecOfAP);
    }
}
#endif

#if CFG_SUPPORT_802_11K
void aisSendNeighborRequest(IN P_ADAPTER_T prAdapter)
{
    struct SUB_ELEMENT_LIST *prSSIDIE;
    u8 aucBuffer[sizeof(*prSSIDIE) + 31];
    P_BSS_INFO_T prBssInfo = prAdapter->prAisBssInfo;

    kalMemZero(aucBuffer, sizeof(aucBuffer));
    prSSIDIE = (struct SUB_ELEMENT_LIST *)&aucBuffer[0];
    prSSIDIE->rSubIE.ucSubID = ELEM_ID_SSID;
    COPY_SSID(&prSSIDIE->rSubIE.aucOptInfo[0], prSSIDIE->rSubIE.ucLength,
              prBssInfo->aucSSID, prBssInfo->ucSSIDLen);
    rlmTxNeighborReportRequest(prAdapter, prBssInfo->prStaRecOfAP,
                               prSSIDIE);
}
#endif

#if CFG_SUPPORT_802_11K || CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
static u8 aisCandPrefIEIsExist(u8 *pucSubIe, u8 ucLength)
{
    u16 u2Offset = 0;

    IE_FOR_EACH(pucSubIe, ucLength, u2Offset){
        if (IE_ID(pucSubIe) == ELEM_ID_NR_BSS_TRANSITION_CAND_PREF) {
            return true;
        }
    }
    return false;
}

static u8 aisGetNeighborApPreference(u8 *pucSubIe, u8 ucLength)
{
    u16 u2Offset = 0;

    IE_FOR_EACH(pucSubIe, ucLength, u2Offset){
        if (IE_ID(pucSubIe) == ELEM_ID_NR_BSS_TRANSITION_CAND_PREF) {
            return pucSubIe[2];
        }
    }
    /* If no preference element is presence, give default value(lowest) 0,
     */
    /* but it will not be used as a reference. */
    return 0;
}

static u64 aisGetBssTermTsf(u8 *pucSubIe, u8 ucLength)
{
    u16 u2Offset = 0;

    IE_FOR_EACH(pucSubIe, ucLength, u2Offset){
        if (IE_ID(pucSubIe) == ELEM_ID_NR_BSS_TERMINATION_DURATION) {
            return *(u64 *)&pucSubIe[2];
        }
    }
    /* If no preference element is presence, give default value(lowest) 0 */
    return 0;
}

void aisCollectNeighborAP(IN P_ADAPTER_T prAdapter, u8 *pucApBuf,
                          u16 u2ApBufLen, u8 ucValidInterval)
{
    P_NEIGHBOR_AP_T prNeighborAP = NULL;
    P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo =
        &prAdapter->rWifiVar.rAisSpecificBssInfo;
    P_LINK_MGMT_T prAPlist = &prAisSpecBssInfo->rNeighborApList;
    P_IE_NEIGHBOR_REPORT_T prIe = (P_IE_NEIGHBOR_REPORT_T)pucApBuf;
    u16 u2BufLen;
    u16 u2PrefIsZeroCount = 0;

    if (!prIe || !u2ApBufLen || u2ApBufLen < prIe->ucLength) {
        return;
    }

    LINK_MERGE_TO_TAIL(&prAPlist->rFreeLink, &prAPlist->rUsingLink);
    for (u2BufLen = u2ApBufLen; u2BufLen > 0; u2BufLen -= IE_SIZE(prIe),
         prIe = (P_IE_NEIGHBOR_REPORT_T)((u8 *)prIe + IE_SIZE(prIe))) {
        /* BIT0-1: AP reachable, BIT2: same security with current
        ** setting,
        ** BIT3: same authenticator with current AP
        */
        if (prIe->ucId != ELEM_ID_NEIGHBOR_REPORT ||
            (prIe->u4BSSIDInfo & 0x7) != 0x7) {
            continue;
        }

        LINK_MGMT_GET_ENTRY(prAPlist, prNeighborAP, NEIGHBOR_AP_T,
                            VIR_MEM_TYPE);
        if (!prNeighborAP) {
            break;
        }
        prNeighborAP->fgHT = !!(prIe->u4BSSIDInfo & BIT(11));
        prNeighborAP->fgFromBtm = !!ucValidInterval;
        prNeighborAP->fgRmEnabled = !!(prIe->u4BSSIDInfo & BIT(7));
        prNeighborAP->fgQoS = !!(prIe->u4BSSIDInfo & BIT(5));
        prNeighborAP->fgSameMD = !!(prIe->u4BSSIDInfo & BIT(10));
        prNeighborAP->ucChannel = prIe->ucChnlNumber;
        prNeighborAP->fgPrefPresence = aisCandPrefIEIsExist(
            prIe->aucSubElem,
            IE_SIZE(prIe) - OFFSET_OF(IE_NEIGHBOR_REPORT_T, aucSubElem));
        prNeighborAP->ucPreference = aisGetNeighborApPreference(
            prIe->aucSubElem,
            IE_SIZE(prIe) - OFFSET_OF(IE_NEIGHBOR_REPORT_T, aucSubElem));
        prNeighborAP->u8TermTsf = aisGetBssTermTsf(
            prIe->aucSubElem,
            IE_SIZE(prIe) - OFFSET_OF(IE_NEIGHBOR_REPORT_T, aucSubElem));
        COPY_MAC_ADDR(prNeighborAP->aucBssid, prIe->aucBSSID);
        DBGLOG(AIS, INFO,
               "Bssid" MACSTR
               ", PrefPresence %d, Pref %d, Chnl %d, BssidInfo 0x%08x\n",
               MAC2STR(
                   prNeighborAP->aucBssid), prNeighborAP->fgPrefPresence,
               prNeighborAP->ucPreference, prIe->ucChnlNumber,
               prIe->u4BSSIDInfo);
        /* No need to save neighbor ap list with decendant preference
        ** for (prTemp = LINK_ENTRY(prAPlist->rUsingLink.prNext, struct
        ** NEIGHBOR_AP_T, rLinkEntry);
        ** prTemp != prNeighborAP;
        ** prTemp = LINK_ENTRY(prTemp->rLinkEntry.prNext, struct
        ** NEIGHBOR_AP_T, rLinkEntry)) {
        ** if (prTemp->ucPreference < prNeighborAP->ucPreference) {
        ** __linkDel(prNeighborAP->rLinkEntry.prPrev,
        ** prNeighborAP->rLinkEntry.prNext);
        ** __linkAdd(&prNeighborAP->rLinkEntry,
        ** prTemp->rLinkEntry.prPrev, &prTemp->rLinkEntry);
        ** break;
        ** }
        ** }
        */
        if (prNeighborAP->fgPrefPresence &&
            prNeighborAP->ucPreference == 0) {
            u2PrefIsZeroCount++;
        }
    }
    prAisSpecBssInfo->rNeiApRcvTime = kalGetTimeTick();
    prAisSpecBssInfo->u4NeiApValidInterval =
        !ucValidInterval ?
        0xffffffff :
        TU_TO_MSEC(ucValidInterval *
                   prAdapter->prAisBssInfo->u2BeaconInterval);

    if (prAPlist->rUsingLink.u4NumElem > 0 &&
        prAPlist->rUsingLink.u4NumElem == u2PrefIsZeroCount) {
        DBGLOG(
            AIS, INFO,
            "The number of valid neighbors is equal to the number of perf value "
            "is 0.\n");
    }
}
#endif

#if CFG_SUPPORT_802_11K || CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
void aisResetNeighborApList(IN P_ADAPTER_T prAdapter)
{
    P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo =
        &prAdapter->rWifiVar.rAisSpecificBssInfo;
    P_LINK_MGMT_T prAPlist = &prAisSpecBssInfo->rNeighborApList;

    LINK_MERGE_TO_TAIL(&prAPlist->rFreeLink, &prAPlist->rUsingLink);
}
#endif
