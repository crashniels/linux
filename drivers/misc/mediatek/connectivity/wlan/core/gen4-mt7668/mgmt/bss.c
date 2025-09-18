// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "bss.c"
 *    \brief  This file contains the functions for creating BSS(AP)/IBSS(AdHoc).
 *
 *   This file contains the functions for BSS(AP)/IBSS(AdHoc). We may create a
 * BSS/IBSS network, or merge with exist IBSS network and sending Beacon Frame
 * or reply the Probe Response Frame for received Probe Request Frame.
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

const u8 *apucNetworkType[NETWORK_TYPE_NUM] = { (u8 *)"AIS", (u8 *)"P2P",
                                                (u8 *)"BOW", (u8 *)"MBSS" };

const u8 *apucNetworkOpMode[] = { (u8 *)"INFRASTRUCTURE", (u8 *)"IBSS",
                                  (u8 *)"ACCESS_POINT", (u8 *)"P2P_DEVICE",
                                  (u8 *)"BOW" };

#if (CFG_SUPPORT_ADHOC) || (CFG_SUPPORT_AAA)
APPEND_VAR_IE_ENTRY_T txBcnIETable[] = {
    { (ELEM_HDR_LEN + (RATE_NUM_SW - ELEM_MAX_LEN_SUP_RATES)), NULL,
      bssGenerateExtSuppRate_IE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_ERP), NULL, rlmRspGenerateErpIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP), NULL, rlmRspGenerateHtCapIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_OP), NULL, rlmRspGenerateHtOpIE }
#if CFG_ENABLE_WIFI_DIRECT
    ,
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_OBSS_SCAN), NULL,
      rlmRspGenerateObssScanIE }
#endif
    ,
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_EXT_CAP), NULL, rlmRspGenerateExtCapIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL, rsnGenerateWpaNoneIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_WMM_PARAM), NULL, mqmGenerateWmmParamIE }
#if CFG_ENABLE_WIFI_DIRECT
    ,
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_WPA), NULL, rsnGenerateWPAIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_RSN), NULL, rsnGenerateRSNIE },
    { 0, p2pFuncCalculateP2p_IELenForBeacon,
      p2pFuncGenerateP2p_IEForBeacon },
    { 0, p2pFuncCalculateWSC_IELenForBeacon,
      p2pFuncGenerateWSC_IEForBeacon },
    { 0, p2pFuncCalculateP2P_IE_NoA, p2pFuncGenerateP2P_IE_NoA }
#endif
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
#if (CFG_SUPPORT_DFS_MASTER == 1)
    ,
    { (ELEM_HDR_LEN + ELEM_MIN_LEN_CSA), NULL, rlmGenerateCsaIE }
#endif
};

APPEND_VAR_IE_ENTRY_T txProbRspIETable[] = {
    { (ELEM_HDR_LEN + (RATE_NUM_SW - ELEM_MAX_LEN_SUP_RATES)), NULL,
      bssGenerateExtSuppRate_IE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_ERP), NULL, rlmRspGenerateErpIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP), NULL, rlmRspGenerateHtCapIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_OP), NULL, rlmRspGenerateHtOpIE }
#if CFG_ENABLE_WIFI_DIRECT
    ,
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_RSN), NULL, rsnGenerateRSNIE },
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_OBSS_SCAN), NULL,
      rlmRspGenerateObssScanIE }
#endif
    ,
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

#endif

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
/* Routines for all Operation Modes                                           */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will decide PHY type set of STA_RECORD_T by given
 * BSS_DESC_T for Infrastructure or AdHoc Mode.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prBssDesc              Received Beacon/ProbeResp from this STA
 * @param[out] prStaRec              StaRec to be decided PHY type set
 *
 * @retval   void
 */
/*----------------------------------------------------------------------------*/
void bssDetermineStaRecPhyTypeSet(IN P_ADAPTER_T prAdapter,
                                  IN P_BSS_DESC_T prBssDesc,
                                  OUT P_STA_RECORD_T prStaRec)
{
    P_WIFI_VAR_T prWifiVar = &prAdapter->rWifiVar;
    u8 ucHtOption = FEATURE_ENABLED;
    u8 ucVhtOption = FEATURE_ENABLED;

    prStaRec->ucPhyTypeSet = prBssDesc->ucPhyTypeSet;
#if CFG_SUPPORT_BFEE
    prStaRec->ucVhtCapNumSoundingDimensions =
        prBssDesc->ucVhtCapNumSoundingDimensions;
#endif

    /* Decide AIS PHY type set */
    if (prStaRec->eStaType == STA_TYPE_LEGACY_AP) {
        if (!((prAdapter->rWifiVar.rConnSettings.eEncStatus ==
               ENUM_ENCRYPTION3_ENABLED) ||
              (prAdapter->rWifiVar.rConnSettings.eEncStatus ==
               ENUM_ENCRYPTION3_KEY_ABSENT) ||
              (prAdapter->rWifiVar.rConnSettings.eEncStatus ==
               ENUM_ENCRYPTION_DISABLED) ||
              (prAdapter->prGlueInfo->u2WSCAssocInfoIELen))) {
            DBGLOG(BSS,
                   INFO,
                   "Ignore the HT Bit for TKIP as pairwise cipher configed!\n");
            prStaRec->ucPhyTypeSet &=
                ~(PHY_TYPE_BIT_HT | PHY_TYPE_BIT_VHT);
        }

        ucHtOption = prWifiVar->ucStaHt;
        ucVhtOption = prWifiVar->ucStaVht;
    }
    /* Decide P2P GC PHY type set */
    else if (prStaRec->eStaType == STA_TYPE_P2P_GO) {
        ucHtOption = prWifiVar->ucP2pGcHt;
        ucVhtOption = prWifiVar->ucP2pGcVht;
    }

    /* Set HT/VHT capability from Feature Option */
    if (IS_FEATURE_DISABLED(ucHtOption)) {
        prStaRec->ucPhyTypeSet &= ~PHY_TYPE_BIT_HT;
    }else if (IS_FEATURE_FORCE_ENABLED(ucHtOption)) {
        prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_HT;
    }

    if (IS_FEATURE_DISABLED(ucVhtOption)) {
        prStaRec->ucPhyTypeSet &= ~PHY_TYPE_BIT_VHT;
    }else if (IS_FEATURE_FORCE_ENABLED(ucVhtOption)) {
        prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;
    }

    prStaRec->ucDesiredPhyTypeSet =
        prStaRec->ucPhyTypeSet &
        prAdapter->rWifiVar.ucAvailablePhyTypeSet;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will decide PHY type set of BSS_INFO for
 *        AP Mode.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] fgIsApMode             Legacy AP mode or P2P GO
 * @param[out] prBssInfo             BssInfo to be decided PHY type set
 *
 * @retval   void
 */
/*----------------------------------------------------------------------------*/
void bssDetermineApBssInfoPhyTypeSet(IN P_ADAPTER_T prAdapter, IN u8 fgIsPureAp,
                                     OUT P_BSS_INFO_T prBssInfo)
{
    P_WIFI_VAR_T prWifiVar = &prAdapter->rWifiVar;
    u8 ucHtOption = FEATURE_ENABLED;
    u8 ucVhtOption = FEATURE_ENABLED;

    /* Decide AP mode PHY type set */
    if (fgIsPureAp) {
        ucHtOption = prWifiVar->ucApHt;
        ucVhtOption = prWifiVar->ucApVht;
    }
    /* Decide P2P GO PHY type set */
    else {
        ucHtOption = prWifiVar->ucP2pGoHt;
        ucVhtOption = prWifiVar->ucP2pGoVht;
    }

    /* Set HT/VHT capability from Feature Option */
    if (IS_FEATURE_DISABLED(ucHtOption)) {
        prBssInfo->ucPhyTypeSet &= ~PHY_TYPE_BIT_HT;
    }else if (IS_FEATURE_ENABLED(ucHtOption)) {
        prBssInfo->ucPhyTypeSet |= PHY_TYPE_BIT_HT;
    }

    if (IS_FEATURE_DISABLED(ucVhtOption)) {
        prBssInfo->ucPhyTypeSet &= ~PHY_TYPE_BIT_VHT;
    } else if (IS_FEATURE_FORCE_ENABLED(ucVhtOption) ||
               (IS_FEATURE_ENABLED(ucVhtOption) &&
                (prBssInfo->eBand == BAND_5G))) {
        /* Enable HT capability if VHT is enabled */
        prBssInfo->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;
    }

    prBssInfo->ucPhyTypeSet &= prAdapter->rWifiVar.ucAvailablePhyTypeSet;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will create or reset a STA_RECORD_T by given BSS_DESC_T
 * for Infrastructure or AdHoc Mode.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] eStaType               Assign STA Type for this STA_RECORD_T
 * @param[in] eNetTypeIndex          Assign Net Type Index for this STA_RECORD_T
 * @param[in] prBssDesc              Received Beacon/ProbeResp from this STA
 *
 * @retval   Pointer to STA_RECORD_T
 */
/*----------------------------------------------------------------------------*/
P_STA_RECORD_T
bssCreateStaRecFromBssDesc(IN P_ADAPTER_T prAdapter,
                           IN ENUM_STA_TYPE_T eStaType, IN u8 ucBssIndex,
                           IN P_BSS_DESC_T prBssDesc)
{
    P_STA_RECORD_T prStaRec;
    u8 ucNonHTPhyTypeSet;
    P_CONNECTION_SETTINGS_T prConnSettings;

    ASSERT(prBssDesc);

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    /* 4 <1> Get a valid STA_RECORD_T */
    prStaRec = cnmGetStaRecByAddress(prAdapter, ucBssIndex,
                                     prBssDesc->aucSrcAddr);
    if (!prStaRec) {
        prStaRec = cnmStaRecAlloc(prAdapter, eStaType, ucBssIndex,
                                  prBssDesc->aucSrcAddr);

        if (!prStaRec) {
            DBGLOG(BSS,
                   WARN,
                   "STA_REC entry is full, cannot acquire new entry for ["
                   MACSTR
                   "]!!\n",
                   MAC2STR(prBssDesc->aucSrcAddr));
            ASSERT(false);
            return NULL;
        }

        prStaRec->ucStaState = STA_STATE_1;
        prStaRec->ucJoinFailureCount = 0;
        /* TODO(Kevin): If this is an old entry, we may also reset the
         * ucJoinFailureCount to 0. */
    }
    /* 4 <2> Update information from BSS_DESC_T to current P_STA_RECORD_T */
    prStaRec->u2CapInfo = prBssDesc->u2CapInfo;

    prStaRec->u2OperationalRateSet = prBssDesc->u2OperationalRateSet;
    prStaRec->u2BSSBasicRateSet = prBssDesc->u2BSSBasicRateSet;

    bssDetermineStaRecPhyTypeSet(prAdapter, prBssDesc, prStaRec);

    ucNonHTPhyTypeSet = prStaRec->ucDesiredPhyTypeSet &
                        PHY_TYPE_SET_802_11ABG;

    /* Check for Target BSS's non HT Phy Types */
    if (ucNonHTPhyTypeSet) {
        if (ucNonHTPhyTypeSet & PHY_TYPE_BIT_ERP) {
            prStaRec->ucNonHTBasicPhyType = PHY_TYPE_ERP_INDEX;
        } else if (ucNonHTPhyTypeSet & PHY_TYPE_BIT_OFDM) {
            prStaRec->ucNonHTBasicPhyType = PHY_TYPE_OFDM_INDEX;
        } else {  /* if (ucNonHTPhyTypeSet & PHY_TYPE_HR_DSSS_INDEX) */
            prStaRec->ucNonHTBasicPhyType = PHY_TYPE_HR_DSSS_INDEX;
        }

        prStaRec->fgHasBasicPhyType = true;
    } else {
        /* Use mandatory for 11N only BSS */
        ASSERT(prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N);

        {
            /* TODO(Kevin): which value should we set for 11n ? ERP
             * ? */
            prStaRec->ucNonHTBasicPhyType = PHY_TYPE_HR_DSSS_INDEX;
        }

        prStaRec->fgHasBasicPhyType = false;
    }

    /* Update non HT Desired Rate Set */
    prStaRec->u2DesiredNonHTRateSet =
        (prStaRec->u2OperationalRateSet &
         prConnSettings->u2DesiredNonHTRateSet);

    /* 4 <3> Update information from BSS_DESC_T to current P_STA_RECORD_T */
    if (IS_AP_STA(prStaRec)) {
        /* do not need to parse IE for DTIM,
         * which have been parsed before inserting into BSS_DESC_T
         */
        if (prBssDesc->ucDTIMPeriod) {
            prStaRec->ucDTIMPeriod = prBssDesc->ucDTIMPeriod;
        } else {
            prStaRec->ucDTIMPeriod = 0;  /* Means that TIM was not
                                          * parsed. */
        }
    }
    /* 4 <4> Update default value */
    prStaRec->fgDiagnoseConnection = false;

    /* 4 <5> Update default value for other Modules */
    /* Determine WMM related parameters for STA_REC */
    mqmProcessScanResult(prAdapter, prBssDesc, prStaRec);

    /* 4 <6> Update Tx Rate */
    /* Update default Tx rate */
    nicTxUpdateStaRecDefaultRate(prStaRec);

    return prStaRec;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the Null Data frame.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] pucBuffer              Pointer to the frame buffer.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void bssComposeNullFrame(IN P_ADAPTER_T prAdapter, IN u8 *pucBuffer,
                         IN P_STA_RECORD_T prStaRec)
{
    P_WLAN_MAC_HEADER_T prNullFrame;
    P_BSS_INFO_T prBssInfo;
    u16 u2FrameCtrl;
    u8 ucBssIndex;

    ASSERT(prStaRec);
    ucBssIndex = prStaRec->ucBssIndex;

    ASSERT(ucBssIndex <= MAX_BSS_INDEX);

    ASSERT(pucBuffer);

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
    ASSERT(prBssInfo);

    prNullFrame = (P_WLAN_MAC_HEADER_T)pucBuffer;

    /* 4 <1> Decide the Frame Control Field */
    u2FrameCtrl = MAC_FRAME_NULL;

    if (IS_AP_STA(prStaRec)) {
        u2FrameCtrl |= MASK_FC_TO_DS;

        if (prStaRec->fgSetPwrMgtBit) {
            u2FrameCtrl |= MASK_FC_PWR_MGT;
        }
    } else if (IS_CLIENT_STA(prStaRec)) {
        u2FrameCtrl |= MASK_FC_FROM_DS;
    } else if (IS_DLS_STA(prStaRec)) {
        /* TODO(Kevin) */
    } else {
        /* NOTE(Kevin): We won't send Null frame for IBSS */
        ASSERT(0);
        return;
    }

    /* 4 <2> Compose the Null frame */
    /* Fill the Frame Control field. */
    /* WLAN_SET_FIELD_16(&prNullFrame->u2FrameCtrl, u2FrameCtrl); */
    prNullFrame->u2FrameCtrl = u2FrameCtrl;  /* NOTE(Kevin): Optimized for
                                              * ARM */

    /* Fill the Address 1 field with Target Peer Address. */
    COPY_MAC_ADDR(prNullFrame->aucAddr1, prStaRec->aucMacAddr);

    /* Fill the Address 2 field with our MAC Address. */
    COPY_MAC_ADDR(prNullFrame->aucAddr2, prBssInfo->aucOwnMacAddr);

    /* Fill the Address 3 field with Target BSSID. */
    COPY_MAC_ADDR(prNullFrame->aucAddr3, prBssInfo->aucBSSID);

    /* Clear the SEQ/FRAG_NO field(HW won't overide the FRAG_NO, so we need
     * to clear it). */
    prNullFrame->u2SeqCtrl = 0;

    return;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the QoS Null Data frame.
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] pucBuffer              Pointer to the frame buffer.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T.
 * @param[in] ucUP                   User Priority.
 * @param[in] fgSetEOSP              Set the EOSP bit.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void bssComposeQoSNullFrame(IN P_ADAPTER_T prAdapter, IN u8 *pucBuffer,
                            IN P_STA_RECORD_T prStaRec, IN u8 ucUP,
                            IN u8 fgSetEOSP)
{
    P_WLAN_MAC_HEADER_QOS_T prQoSNullFrame;
    P_BSS_INFO_T prBssInfo;
    u16 u2FrameCtrl;
    u16 u2QosControl;
    u8 ucBssIndex;

    ASSERT(prStaRec);
    ucBssIndex = prStaRec->ucBssIndex;

    ASSERT(ucBssIndex <= MAX_BSS_INDEX);

    ASSERT(pucBuffer);

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
    ASSERT(prBssInfo);

    prQoSNullFrame = (P_WLAN_MAC_HEADER_QOS_T)pucBuffer;

    /* 4 <1> Decide the Frame Control Field */
    u2FrameCtrl = MAC_FRAME_QOS_NULL;

    if (IS_AP_STA(prStaRec)) {
        u2FrameCtrl |= MASK_FC_TO_DS;

        if (prStaRec->fgSetPwrMgtBit) {
            u2FrameCtrl |= MASK_FC_PWR_MGT;
        }
    } else if (IS_CLIENT_STA(prStaRec)) {
        u2FrameCtrl |= MASK_FC_FROM_DS;
    } else if (IS_DLS_STA(prStaRec)) {
        /* TODO(Kevin) */
    } else {
        /* NOTE(Kevin): We won't send QoS Null frame for IBSS */
        ASSERT(0);
        return;
    }

    /* 4 <2> Compose the QoS Null frame */
    /* Fill the Frame Control field. */
    /* WLAN_SET_FIELD_16(&prQoSNullFrame->u2FrameCtrl, u2FrameCtrl); */
    prQoSNullFrame->u2FrameCtrl = u2FrameCtrl;  /* NOTE(Kevin): Optimized for
                                                 * ARM */

    /* Fill the Address 1 field with Target Peer Address. */
    COPY_MAC_ADDR(prQoSNullFrame->aucAddr1, prStaRec->aucMacAddr);

    /* Fill the Address 2 field with our MAC Address. */
    COPY_MAC_ADDR(prQoSNullFrame->aucAddr2, prBssInfo->aucOwnMacAddr);

    /* Fill the Address 3 field with Target BSSID. */
    COPY_MAC_ADDR(prQoSNullFrame->aucAddr3, prBssInfo->aucBSSID);

    /* Clear the SEQ/FRAG_NO field(HW won't overide the FRAG_NO, so we need
     * to clear it). */
    prQoSNullFrame->u2SeqCtrl = 0;

    u2QosControl = (u16)(ucUP & WMM_QC_UP_MASK);

    if (fgSetEOSP) {
        u2QosControl |= WMM_QC_EOSP;
    }

    /* WLAN_SET_FIELD_16(&prQoSNullFrame->u2QosCtrl, u2QosControl); */
    prQoSNullFrame->u2QosCtrl = u2QosControl;  /* NOTE(Kevin): Optimized for
                                                * ARM */

    return;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Send the Null Frame
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 * @param[in] pfTxDoneHandler    TX Done call back function
 *
 * @retval WLAN_STATUS_RESOURCE  No available resources to send frame.
 * @retval WLAN_STATUS_SUCCESS   Succe]ss.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
bssSendNullFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec,
                 IN PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
    P_MSDU_INFO_T prMsduInfo;
    u16 u2EstimatedFrameLen;

    /* 4 <1> Allocate a PKT_INFO_T for Null Frame */
    /* Init with MGMT Header Length */
    u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD + WLAN_MAC_HEADER_LEN;

    /* Allocate a MSDU_INFO_T */
    prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
    if (prMsduInfo == NULL) {
        DBGLOG(BSS, WARN, "No PKT_INFO_T for sending Null Frame.\n");
        return WLAN_STATUS_RESOURCES;
    }
    /* 4 <2> Compose Null frame in MSDU_INfO_T. */
    bssComposeNullFrame(prAdapter,
                        (u8 *)((unsigned long)prMsduInfo->prPacket +
                               MAC_TX_RESERVED_FIELD),
                        prStaRec);

    TX_SET_MMPDU(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
                 prStaRec->ucIndex, WLAN_MAC_HEADER_LEN,
                 WLAN_MAC_HEADER_LEN, pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

    /* 4 <4> Inform TXM  to send this Null frame. */
    nicTxEnqueueMsdu(prAdapter, prMsduInfo);

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Send the QoS Null Frame
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 * @param[in] pfTxDoneHandler    TX Done call back function
 *
 * @retval WLAN_STATUS_RESOURCE  No available resources to send frame.
 * @retval WLAN_STATUS_SUCCESS   Success.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
bssSendQoSNullFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec,
                    IN u8 ucUP, IN PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
    P_MSDU_INFO_T prMsduInfo;
    u16 u2EstimatedFrameLen;

    /* 4 <1> Allocate a PKT_INFO_T for Null Frame */
    /* Init with MGMT Header Length */
    u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD + WLAN_MAC_HEADER_QOS_LEN;

    /* Allocate a MSDU_INFO_T */
    prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
    if (prMsduInfo == NULL) {
        DBGLOG(BSS, WARN, "No PKT_INFO_T for sending Null Frame.\n");
        return WLAN_STATUS_RESOURCES;
    }
    /* 4 <2> Compose Null frame in MSDU_INfO_T. */
    bssComposeQoSNullFrame(prAdapter,
                           (u8 *)((unsigned long)(prMsduInfo->prPacket) +
                                  MAC_TX_RESERVED_FIELD),
                           prStaRec, ucUP, false);

    TX_SET_MMPDU(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
                 prStaRec->ucIndex, WLAN_MAC_HEADER_QOS_LEN,
                 WLAN_MAC_HEADER_QOS_LEN, pfTxDoneHandler,
                 MSDU_RATE_MODE_AUTO);

    /* 4 <4> Inform TXM  to send this Null frame. */
    nicTxEnqueueMsdu(prAdapter, prMsduInfo);

    return WLAN_STATUS_SUCCESS;
}

#if (CFG_SUPPORT_ADHOC) || (CFG_SUPPORT_AAA)
/*----------------------------------------------------------------------------*/
/* Routines for both IBSS(AdHoc) and BSS(AP)                                  */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to generate Information Elements of Extended
 *        Support Rate
 *
 * @param[in] prAdapter      Pointer to the Adapter structure.
 * @param[in] prMsduInfo     Pointer to the composed MSDU_INFO_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void bssGenerateExtSuppRate_IE(IN P_ADAPTER_T prAdapter,
                               IN P_MSDU_INFO_T prMsduInfo)
{
    P_BSS_INFO_T prBssInfo;
    u8 *pucBuffer;
    u8 ucExtSupRatesLen;

    ASSERT(prMsduInfo);

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
    ASSERT(prBssInfo);

    pucBuffer = (u8 *)((unsigned long)prMsduInfo->prPacket +
                       (unsigned long)prMsduInfo->u2FrameLength);
    ASSERT(pucBuffer);

    if (prBssInfo->ucAllSupportedRatesLen > ELEM_MAX_LEN_SUP_RATES) {
        ucExtSupRatesLen = prBssInfo->ucAllSupportedRatesLen -
                           ELEM_MAX_LEN_SUP_RATES;
    } else {
        ucExtSupRatesLen = 0;
    }

    /* Fill the Extended Supported Rates element. */
    if (ucExtSupRatesLen) {
        EXT_SUP_RATES_IE(pucBuffer)->ucId = ELEM_ID_EXTENDED_SUP_RATES;
        EXT_SUP_RATES_IE(pucBuffer)->ucLength = ucExtSupRatesLen;

        kalMemCopy(
            EXT_SUP_RATES_IE(pucBuffer)->aucExtSupportedRates,
            &prBssInfo->aucAllSupportedRates[ELEM_MAX_LEN_SUP_RATES],
            ucExtSupRatesLen);

        prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to compose Common Information Elements for
 * Beacon or Probe Response Frame.
 *
 * @param[in] prMsduInfo     Pointer to the composed MSDU_INFO_T.
 * @param[in] prBssInfo      Pointer to the BSS_INFO_T.
 * @param[in] pucDestAddr    Pointer to the Destination Address, if NULL, means
 * Beacon.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void bssBuildBeaconProbeRespFrameCommonIEs(IN P_MSDU_INFO_T prMsduInfo,
                                           IN P_BSS_INFO_T prBssInfo,
                                           IN u8 *pucDestAddr)
{
    u8 *pucBuffer;
    u8 ucSupRatesLen;

    ASSERT(prMsduInfo);
    ASSERT(prBssInfo);

    pucBuffer = (u8 *)((unsigned long)prMsduInfo->prPacket +
                       (unsigned long)prMsduInfo->u2FrameLength);
    ASSERT(pucBuffer);

    /* Compose the frame body of the Probe Response frame. */
    /* 4 <1> Fill the SSID element. */
    SSID_IE(pucBuffer)->ucId = ELEM_ID_SSID;

    if (prBssInfo->eHiddenSsidType == ENUM_HIDDEN_SSID_LEN) {
        if ((!pucDestAddr) &&  /* For Beacon only. */
            (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)) {
            SSID_IE(pucBuffer)->ucLength = 0;
        } else {  /* probe response */
            SSID_IE(pucBuffer)->ucLength = prBssInfo->ucSSIDLen;
            if (prBssInfo->ucSSIDLen) {
                kalMemCopy(SSID_IE(pucBuffer)->aucSSID,
                           prBssInfo->aucSSID,
                           prBssInfo->ucSSIDLen);
            }
        }
    } else {
        SSID_IE(pucBuffer)->ucLength = prBssInfo->ucSSIDLen;
        if (prBssInfo->ucSSIDLen) {
            kalMemCopy(SSID_IE(pucBuffer)->aucSSID,
                       prBssInfo->aucSSID, prBssInfo->ucSSIDLen);
        }
    }

    prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
    pucBuffer += IE_SIZE(pucBuffer);

    /* 4 <2> Fill the Supported Rates element. */
    if (prBssInfo->ucAllSupportedRatesLen > ELEM_MAX_LEN_SUP_RATES) {
        ucSupRatesLen = ELEM_MAX_LEN_SUP_RATES;
    } else {
        ucSupRatesLen = prBssInfo->ucAllSupportedRatesLen;
    }

    if (ucSupRatesLen) {
        SUP_RATES_IE(pucBuffer)->ucId = ELEM_ID_SUP_RATES;
        SUP_RATES_IE(pucBuffer)->ucLength = ucSupRatesLen;
        kalMemCopy(SUP_RATES_IE(pucBuffer)->aucSupportedRates,
                   prBssInfo->aucAllSupportedRates, ucSupRatesLen);

        prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
        pucBuffer += IE_SIZE(pucBuffer);
    }

    /* 4 <3> Fill the DS Parameter Set element. */
    if (prBssInfo->eBand == BAND_2G4) {
        DS_PARAM_IE(pucBuffer)->ucId = ELEM_ID_DS_PARAM_SET;
        DS_PARAM_IE(pucBuffer)->ucLength =
            ELEM_MAX_LEN_DS_PARAMETER_SET;
        DS_PARAM_IE(pucBuffer)->ucCurrChnl =
            prBssInfo->ucPrimaryChannel;

        prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
        pucBuffer += IE_SIZE(pucBuffer);
    }

    /* 4 <4> IBSS Parameter Set element, ID: 6 */
    if (prBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
        IBSS_PARAM_IE(pucBuffer)->ucId = ELEM_ID_IBSS_PARAM_SET;
        IBSS_PARAM_IE(pucBuffer)->ucLength =
            ELEM_MAX_LEN_IBSS_PARAMETER_SET;
        WLAN_SET_FIELD_16(&(IBSS_PARAM_IE(pucBuffer)->u2ATIMWindow),
                          prBssInfo->u2ATIMWindow);

        prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
        pucBuffer += IE_SIZE(pucBuffer);
    }

    /* 4 <5> TIM element, ID: 5 */
    if ((!pucDestAddr) &&  /* For Beacon only. */
        (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)) {
#if CFG_ENABLE_WIFI_DIRECT
        /*no fgIsP2PRegistered protect */
        if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
            /* IEEE 802.11 2007 - 7.3.2.6 */
            TIM_IE(pucBuffer)->ucId = ELEM_ID_TIM;
            /* NOTE: fixed PVB length (AID is allocated from 8 ~ 15
             * only) */
            TIM_IE(pucBuffer)->ucLength =
                (3 + MAX_LEN_TIM_PARTIAL_BMP)  /*((u4N2 - u4N1) +
                                                * 4) */
            ;
            TIM_IE(pucBuffer)->ucDTIMCount =
                0  /*prBssInfo->ucDTIMCount */;
            /* will be overwritten by FW */
            TIM_IE(pucBuffer)->ucDTIMPeriod =
                prBssInfo->ucDTIMPeriod;
            /* will be overwritten by FW */
            TIM_IE(pucBuffer)->ucBitmapControl =
                0  /*ucBitmapControl | (u8)u4N1 */;

            prMsduInfo->u2FrameLength += IE_SIZE(pucBuffer);
        } else
#endif
        {
            /* NOTE(Kevin): 1. AIS - Didn't Support AP Mode.
             *              2. BOW - Didn't Support BCAST and PS.
             */
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the Beacon/Probe Response frame header and
 *        its fixed fields.
 *
 * @param[in] pucBuffer              Pointer to the frame buffer.
 * @param[in] pucDestAddr            Pointer to the Destination Address, if
 * NULL, means Beacon.
 * @param[in] pucOwnMACAddress       Given Our MAC Address.
 * @param[in] pucBSSID               Given BSSID of the BSS.
 * @param[in] u2BeaconInterval       Given Beacon Interval.
 * @param[in] u2CapInfo              Given Capability Info.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void bssComposeBeaconProbeRespFrameHeaderAndFF(
    IN u8 *pucBuffer, IN u8 *pucDestAddr, IN u8 *pucOwnMACAddress,
    IN u8 *pucBSSID, IN u16 u2BeaconInterval, IN u16 u2CapInfo)
{
    P_WLAN_BEACON_FRAME_T prBcnProbRspFrame;
    u8 aucBCAddr[] = BC_MAC_ADDR;
    u16 u2FrameCtrl;

    DEBUGFUNC("bssComposeBeaconProbeRespFrameHeaderAndFF");
    /* DBGLOG(INIT, LOUD, ("\n")); */

    ASSERT(pucBuffer);
    ASSERT(pucOwnMACAddress);
    ASSERT(pucBSSID);

    prBcnProbRspFrame = (P_WLAN_BEACON_FRAME_T)pucBuffer;

    /* 4 <1> Compose the frame header of the Beacon /ProbeResp frame. */
    /* Fill the Frame Control field. */
    if (pucDestAddr) {
        u2FrameCtrl = MAC_FRAME_PROBE_RSP;
    } else {
        u2FrameCtrl = MAC_FRAME_BEACON;
        pucDestAddr = aucBCAddr;
    }
    /* WLAN_SET_FIELD_16(&prBcnProbRspFrame->u2FrameCtrl, u2FrameCtrl); */
    prBcnProbRspFrame->u2FrameCtrl = u2FrameCtrl;  /* NOTE(Kevin): Optimized
                                                    * for ARM */

    /* Fill the DA field with BCAST MAC ADDR or TA of ProbeReq. */
    COPY_MAC_ADDR(prBcnProbRspFrame->aucDestAddr, pucDestAddr);

    /* Fill the SA field with our MAC Address. */
    COPY_MAC_ADDR(prBcnProbRspFrame->aucSrcAddr, pucOwnMACAddress);

    /* Fill the BSSID field with current BSSID. */
    COPY_MAC_ADDR(prBcnProbRspFrame->aucBSSID, pucBSSID);

    /* Clear the SEQ/FRAG_NO field(HW won't overide the FRAG_NO, so we need
     * to clear it). */
    prBcnProbRspFrame->u2SeqCtrl = 0;

    /* 4 <2> Compose the frame body's common fixed field part of the Beacon
     * /ProbeResp frame. */
    /* MAC will update TimeStamp field */

    /* Fill the Beacon Interval field. */
    /* WLAN_SET_FIELD_16(&prBcnProbRspFrame->u2BeaconInterval,
     * u2BeaconInterval); */
    prBcnProbRspFrame->u2BeaconInterval = u2BeaconInterval;  /* NOTE(Kevin):
                                                              * Optimized for
                                                              * ARM */

    /* Fill the Capability Information field. */
    /* WLAN_SET_FIELD_16(&prBcnProbRspFrame->u2CapInfo, u2CapInfo); */
    prBcnProbRspFrame->u2CapInfo = u2CapInfo;  /* NOTE(Kevin): Optimized for
                                                * ARM */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Update the Beacon Frame Template to FW for AIS AdHoc and P2P GO.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] ucBssIndex         Specify which network reply the Probe Response.
 *
 * @retval WLAN_STATUS_SUCCESS   Success.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS bssUpdateBeaconContent(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex)
{
    P_BSS_INFO_T prBssInfo;
    P_MSDU_INFO_T prMsduInfo;
    P_WLAN_BEACON_FRAME_T prBcnFrame;
    u32 i;

    DEBUGFUNC("bssUpdateBeaconContent");
    DBGLOG(INIT, LOUD, "\n");

    ASSERT(ucBssIndex <= MAX_BSS_INDEX);

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    /* 4 <1> Allocate a PKT_INFO_T for Beacon Frame */
    /* Allocate a MSDU_INFO_T */
    /* For Beacon */
    prMsduInfo = prBssInfo->prBeacon;

    /* beacon prMsduInfo will be NULLify once BSS deactivated, so skip if it
     * is */
    if (prMsduInfo == NULL) {
        return WLAN_STATUS_SUCCESS;
    }

    /* 4 <2> Compose header */
    bssComposeBeaconProbeRespFrameHeaderAndFF(
        (u8 *)((unsigned long)(prMsduInfo->prPacket) +
               MAC_TX_RESERVED_FIELD),
        NULL, prBssInfo->aucOwnMacAddr, prBssInfo->aucBSSID,
        prBssInfo->u2BeaconInterval, prBssInfo->u2CapInfo);

    prMsduInfo->u2FrameLength =
        (WLAN_MAC_MGMT_HEADER_LEN +
         (TIMESTAMP_FIELD_LEN + BEACON_INTERVAL_FIELD_LEN +
          CAP_INFO_FIELD_LEN));

    prMsduInfo->ucBssIndex = ucBssIndex;

    /* 4 <3> Compose the frame body's Common IEs of the Beacon frame. */
    bssBuildBeaconProbeRespFrameCommonIEs(prMsduInfo, prBssInfo, NULL);

    /* 4 <4> Compose IEs in MSDU_INFO_T */

    /* Append IE for Beacon */
    for (i = 0; i < sizeof(txBcnIETable) / sizeof(APPEND_VAR_IE_ENTRY_T);
         i++)
        if (txBcnIETable[i].pfnAppendIE) {
            txBcnIETable[i].pfnAppendIE(prAdapter, prMsduInfo);
        }

    prBcnFrame = (P_WLAN_BEACON_FRAME_T)prMsduInfo->prPacket;

    return nicUpdateBeaconIETemplate(
        prAdapter, IE_UPD_METHOD_UPDATE_ALL, ucBssIndex,
        prBssInfo->u2CapInfo, (u8 *)prBcnFrame->aucInfoElem,
        prMsduInfo->u2FrameLength -
        OFFSET_OF(WLAN_BEACON_FRAME_T, aucInfoElem));
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Send the Beacon Frame(for BOW) or Probe Response Frame according to
 * the given Destination Address.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] ucBssIndex         Specify which network reply the Probe Response.
 * @param[in] pucDestAddr        Pointer to the Destination Address to reply
 * @param[in] u4ControlFlags     Control flags for information on Probe
 * Response.
 *
 * @retval WLAN_STATUS_RESOURCE  No available resources to send frame.
 * @retval WLAN_STATUS_SUCCESS   Success.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
bssSendBeaconProbeResponse(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                           IN u8 *pucDestAddr, IN u32 u4ControlFlags)
{
    P_BSS_INFO_T prBssInfo;
    P_MSDU_INFO_T prMsduInfo;
    u16 u2EstimatedFrameLen;
    u16 u2EstimatedFixedIELen;
    u16 u2EstimatedExtraIELen;
    P_APPEND_VAR_IE_ENTRY_T prIeArray = NULL;
    u32 u4IeArraySize = 0;
    u32 i;

    ASSERT(ucBssIndex <= MAX_BSS_INDEX);

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    if (!pucDestAddr) {  /* For Beacon */
        prIeArray = &txBcnIETable[0];
        u4IeArraySize =
            sizeof(txBcnIETable) / sizeof(APPEND_VAR_IE_ENTRY_T);
    } else {
        prIeArray = &txProbRspIETable[0];
        u4IeArraySize = sizeof(txProbRspIETable) /
                        sizeof(APPEND_VAR_IE_ENTRY_T);
    }

    /* 4 <1> Allocate a PKT_INFO_T for Beacon /Probe Response Frame */
    /* Allocate a MSDU_INFO_T */

    /* Init with MGMT Header Length + Length of Fixed Fields + Common IE
     * Fields */
    u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD + WLAN_MAC_MGMT_HEADER_LEN +
                          TIMESTAMP_FIELD_LEN + BEACON_INTERVAL_FIELD_LEN +
                          CAP_INFO_FIELD_LEN +
                          (ELEM_HDR_LEN + ELEM_MAX_LEN_SSID) +
                          (ELEM_HDR_LEN + ELEM_MAX_LEN_SUP_RATES) +
                          (ELEM_HDR_LEN + ELEM_MAX_LEN_DS_PARAMETER_SET) +
                          (ELEM_HDR_LEN + ELEM_MAX_LEN_IBSS_PARAMETER_SET) +
                          (ELEM_HDR_LEN + (3 + MAX_LEN_TIM_PARTIAL_BMP));

    /* + Extra IE Length */
    u2EstimatedExtraIELen = 0;

    for (i = 0; i < u4IeArraySize; i++) {
        u2EstimatedFixedIELen = prIeArray[i].u2EstimatedFixedIELen;

        if (u2EstimatedFixedIELen) {
            u2EstimatedExtraIELen += u2EstimatedFixedIELen;
        } else {
            ASSERT(prIeArray[i].pfnCalculateVariableIELen);

            u2EstimatedExtraIELen +=
                (u16)prIeArray[i].pfnCalculateVariableIELen(
                    prAdapter, ucBssIndex, NULL);
        }
    }

    u2EstimatedFrameLen += u2EstimatedExtraIELen;
    prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
    if (prMsduInfo == NULL) {
        DBGLOG(BSS, WARN, "No PKT_INFO_T for sending %s.\n",
               ((!pucDestAddr) ? "Beacon" : "Probe Response"));
        return WLAN_STATUS_RESOURCES;
    }

    /* 4 <2> Compose Beacon/Probe Response frame header and fixed fields in
     * MSDU_INfO_T. */
    /* Compose Header and Fixed Field */
#if CFG_ENABLE_WIFI_DIRECT
    if (u4ControlFlags & BSS_PROBE_RESP_USE_P2P_DEV_ADDR) {
        if (prAdapter->fgIsP2PRegistered) {
            bssComposeBeaconProbeRespFrameHeaderAndFF(
                (u8 *)((unsigned long)(prMsduInfo->prPacket) +
                       MAC_TX_RESERVED_FIELD),
                pucDestAddr,
                prAdapter->rWifiVar.aucDeviceAddress,
                prAdapter->rWifiVar.aucDeviceAddress,
                DOT11_BEACON_PERIOD_DEFAULT,
                (prBssInfo->u2CapInfo &
                 ~(CAP_INFO_ESS | CAP_INFO_IBSS)));
        }
    } else
#endif
    {
        bssComposeBeaconProbeRespFrameHeaderAndFF(
            (u8 *)((unsigned long)(prMsduInfo->prPacket) +
                   MAC_TX_RESERVED_FIELD),
            pucDestAddr, prBssInfo->aucOwnMacAddr,
            prBssInfo->aucBSSID, prBssInfo->u2BeaconInterval,
            prBssInfo->u2CapInfo);
    }

    /* 4 <3> Update information of MSDU_INFO_T */

    TX_SET_MMPDU(prAdapter, prMsduInfo, ucBssIndex, STA_REC_INDEX_NOT_FOUND,
                 WLAN_MAC_MGMT_HEADER_LEN,
                 (WLAN_MAC_MGMT_HEADER_LEN + TIMESTAMP_FIELD_LEN +
                  BEACON_INTERVAL_FIELD_LEN + CAP_INFO_FIELD_LEN),
                 NULL, MSDU_RATE_MODE_AUTO);

    /* 4 <4> Compose the frame body's Common IEs of the Beacon/ProbeResp
     * frame. */
    bssBuildBeaconProbeRespFrameCommonIEs(prMsduInfo, prBssInfo,
                                          pucDestAddr);

    /* 4 <5> Compose IEs in MSDU_INFO_T */

    /* Append IE */
    for (i = 0; i < u4IeArraySize; i++)
        if (prIeArray[i].pfnAppendIE) {
            prIeArray[i].pfnAppendIE(prAdapter, prMsduInfo);
        }

    /* Set limited retry count and lifetime for Probe Resp is reasonable */
    nicTxSetPktLifeTime(prMsduInfo, 100);
    nicTxSetPktRetryLimit(prMsduInfo, 2);

    /* TODO(Kevin): Also release the unused tail room of the composed MMPDU
     */

    /* 4 <6> Inform TXM  to send this Beacon /Probe Response frame. */
    nicTxEnqueueMsdu(prAdapter, prMsduInfo);

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will process the Rx Probe Request Frame and then send
 *        back the corresponding Probe Response Frame if the specified
 * conditions were matched.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Always return success
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS bssProcessProbeRequest(IN P_ADAPTER_T prAdapter,
                                   IN P_SW_RFB_T prSwRfb)
{
    P_WLAN_MAC_MGMT_HEADER_T prMgtHdr;
    P_BSS_INFO_T prBssInfo;
    u8 ucBssIndex;
    u8 aucBCBSSID[] = BC_BSSID;
    u8 fgIsBcBssid;
    u8 fgReplyProbeResp;
    u32 u4CtrlFlagsForProbeResp = 0;
    ENUM_BAND_T eBand;
    u8 ucHwChannelNum;

    ASSERT(prSwRfb);

    /* 4 <1> Parse Probe Req and Get BSSID */
    prMgtHdr = (P_WLAN_MAC_MGMT_HEADER_T)prSwRfb->pvHeader;

    if (EQUAL_MAC_ADDR(aucBCBSSID, prMgtHdr->aucBSSID)) {
        fgIsBcBssid = true;
    }else{
        fgIsBcBssid = false;
    }

    /* 4 <2> Check network conditions before reply Probe Response Frame
     * (Consider Concurrent) */
    for (ucBssIndex = 0; ucBssIndex <= P2P_DEV_BSS_INDEX; ucBssIndex++) {
        if (!IS_NET_ACTIVE(prAdapter, ucBssIndex)) {
            continue;
        }

        prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

        if ((!fgIsBcBssid) &&
            UNEQUAL_MAC_ADDR(prBssInfo->aucBSSID, prMgtHdr->aucBSSID)) {
            continue;
        }

        eBand = HAL_RX_STATUS_GET_RF_BAND(prSwRfb->prRxStatus);
        ucHwChannelNum =
            HAL_RX_STATUS_GET_CHNL_NUM(prSwRfb->prRxStatus);

        if (prBssInfo->eBand != eBand) {
            continue;
        }

        if (prBssInfo->ucPrimaryChannel != ucHwChannelNum) {
            continue;
        }

        fgReplyProbeResp = false;

        if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
#if CFG_SUPPORT_ADHOC
            fgReplyProbeResp = aisValidateProbeReq(
                prAdapter, prSwRfb, &u4CtrlFlagsForProbeResp);
#endif
        }
#if CFG_ENABLE_WIFI_DIRECT
        else if ((prAdapter->fgIsP2PRegistered) &&
                 (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)) {
            fgReplyProbeResp = p2pFuncValidateProbeReq(
                prAdapter, prSwRfb, &u4CtrlFlagsForProbeResp,
                (prBssInfo->ucBssIndex == P2P_DEV_BSS_INDEX),
                (u8)prBssInfo->u4PrivateData);
        }
#endif

        if (fgReplyProbeResp) {
            if (nicTxGetFreeCmdCount(prAdapter) >
                (CFG_TX_MAX_CMD_PKT_NUM / 2)) {
                /* Resource margin is enough */
                bssSendBeaconProbeResponse(
                    prAdapter, ucBssIndex,
                    prMgtHdr->aucSrcAddr,
                    u4CtrlFlagsForProbeResp);
            }
        }
    }

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to initialize the client list for AdHoc or AP
 * Mode
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prBssInfo              Given related BSS_INFO_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void bssInitializeClientList(IN P_ADAPTER_T prAdapter,
                             IN P_BSS_INFO_T prBssInfo)
{
    P_LINK_T prStaRecOfClientList;

    ASSERT(prBssInfo);

    prStaRecOfClientList = &prBssInfo->rStaRecOfClientList;

    if (!LINK_IS_EMPTY(prStaRecOfClientList)) {
        LINK_INITIALIZE(prStaRecOfClientList);
    }

    DBGLOG(BSS, INFO, "Init BSS[%u] Client List\n", prBssInfo->ucBssIndex);

    bssCheckClientList(prAdapter, prBssInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to Add a STA_RECORD_T to the client list for
 * AdHoc or AP Mode
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prBssInfo              Given related BSS_INFO_T.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void bssAddClient(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo,
                  IN P_STA_RECORD_T prStaRec)
{
    P_LINK_T prClientList;
    P_STA_RECORD_T prCurrStaRec;

    ASSERT(prBssInfo);

    prClientList = &prBssInfo->rStaRecOfClientList;

    LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
                        STA_RECORD_T) {
        if (prCurrStaRec == prStaRec) {
            DBGLOG(BSS,
                   WARN,
                   "Current Client List already contains that STA_RECORD_T["
                   MACSTR
                   "]\n",
                   MAC2STR(prStaRec->aucMacAddr));
            return;
        }
    }

    LINK_INSERT_TAIL(prClientList, &prStaRec->rLinkEntry);

    bssCheckClientList(prAdapter, prBssInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to Remove a STA_RECORD_T from the client list
 * for AdHoc or AP Mode
 *
 * @param[in] prAdapter              Pointer to the Adapter structure.
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
u8 bssRemoveClient(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo,
                   IN P_STA_RECORD_T prStaRec)
{
    P_LINK_T prClientList;
    P_STA_RECORD_T prCurrStaRec;

    ASSERT(prBssInfo);

    prClientList = &prBssInfo->rStaRecOfClientList;

    LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
                        STA_RECORD_T) {
        if (prCurrStaRec == prStaRec) {
            LINK_REMOVE_KNOWN_ENTRY(prClientList,
                                    &prStaRec->rLinkEntry);

            return true;
        }
    }

    DBGLOG(BSS, INFO,
           "Current Client List didn't contain that STA_RECORD_T[" MACSTR
           "] before removing.\n",
           MAC2STR(prStaRec->aucMacAddr));

    bssCheckClientList(prAdapter, prBssInfo);

    return false;
}

P_STA_RECORD_T bssRemoveClientByMac(IN P_ADAPTER_T prAdapter,
                                    IN P_BSS_INFO_T prBssInfo, IN u8 *pucMac)
{
    P_LINK_T prClientList;
    P_STA_RECORD_T prCurrStaRec;

    ASSERT(prBssInfo);

    prClientList = &prBssInfo->rStaRecOfClientList;

    LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
                        STA_RECORD_T) {
        if (EQUAL_MAC_ADDR(prCurrStaRec->aucMacAddr, pucMac)) {
            LINK_REMOVE_KNOWN_ENTRY(prClientList,
                                    &prCurrStaRec->rLinkEntry);

            return prCurrStaRec;
        }
    }

    DBGLOG(BSS, INFO,
           "Current Client List didn't contain that STA_RECORD_T[" MACSTR
           "] before removing.\n",
           MAC2STR(pucMac));

    bssCheckClientList(prAdapter, prBssInfo);

    return NULL;
}

P_STA_RECORD_T bssGetClientByMac(IN P_ADAPTER_T prAdapter,
                                 IN P_BSS_INFO_T prBssInfo, IN u8 *pucMac)
{
    P_LINK_T prClientList;
    P_STA_RECORD_T prCurrStaRec;

    ASSERT(prBssInfo);

    prClientList = &prBssInfo->rStaRecOfClientList;

    LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
                        STA_RECORD_T) {
        if (EQUAL_MAC_ADDR(prCurrStaRec->aucMacAddr, pucMac)) {
            return prCurrStaRec;
        }
    }

    DBGLOG(BSS, INFO,
           "Current Client List didn't contain that STA_RECORD_T[" MACSTR
           "] before removing.\n",
           MAC2STR(pucMac));

    bssCheckClientList(prAdapter, prBssInfo);

    return NULL;
}

P_STA_RECORD_T bssRemoveHeadClient(IN P_ADAPTER_T prAdapter,
                                   IN P_BSS_INFO_T prBssInfo)
{
    P_LINK_T prStaRecOfClientList;
    P_STA_RECORD_T prStaRec = NULL;

    ASSERT(prBssInfo);

    prStaRecOfClientList = &prBssInfo->rStaRecOfClientList;

    if (!LINK_IS_EMPTY(prStaRecOfClientList)) {
        LINK_REMOVE_HEAD(prStaRecOfClientList, prStaRec,
                         P_STA_RECORD_T);
    }

    bssCheckClientList(prAdapter, prBssInfo);

    return prStaRec;
}

u32 bssGetClientCount(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
    return prBssInfo->rStaRecOfClientList.u4NumElem;
}

void bssDumpClientList(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
    P_LINK_T prClientList;
    P_STA_RECORD_T prCurrStaRec;
    u8 ucCount = 0;

    ASSERT(prBssInfo);

    prClientList = &prBssInfo->rStaRecOfClientList;

    DBGLOG(SW4, INFO, "Dump BSS[%u] Client List NUM[%u]\n",
           prBssInfo->ucBssIndex, prClientList->u4NumElem);

    LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
                        STA_RECORD_T) {
        if (!prCurrStaRec) {
            DBGLOG(SW4, INFO, "[%2u] is NULL STA_REC\n", ucCount);
            break;
        }
        DBGLOG(SW4, INFO, "[%2u] STA[%u] [" MACSTR "]\n", ucCount,
               prCurrStaRec->ucIndex,
               MAC2STR(prCurrStaRec->aucMacAddr));

        ucCount++;
    }
}

void bssCheckClientList(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
    P_LINK_T prClientList;
    P_STA_RECORD_T prCurrStaRec;
    u8 ucCount = 0;
    u8 fgError = false;

    ASSERT(prBssInfo);

    prClientList = &prBssInfo->rStaRecOfClientList;

    /* Check MAX number */
    if (prClientList->u4NumElem > P2P_MAXIMUM_CLIENT_COUNT) {
        DBGLOG(SW4, INFO, "BSS[%u] Client List NUM[%u] ERR\n",
               prBssInfo->ucBssIndex, prClientList->u4NumElem);

        fgError = true;
    }

    /* Check default list status */
    if (prClientList->u4NumElem == 0) {
        if ((void *)prClientList->prNext != (void *)prClientList) {
            fgError = true;
        }
        if ((void *)prClientList->prPrev != (void *)prClientList) {
            fgError = true;
        }

        if (fgError) {
            DBGLOG(SW4, INFO,
                   "BSS[%u] Client List PTR next/prev[%p/%p] ERR\n",
                   prBssInfo->ucBssIndex, prClientList->prNext,
                   prClientList->prPrev);
        }
    }

    /* Traverse list */
    LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList, rLinkEntry,
                        STA_RECORD_T) {
        if (!prCurrStaRec) {
            fgError = true;
            DBGLOG(SW4, INFO, "BSS[%u] Client List NULL PTR ERR\n",
                   prBssInfo->ucBssIndex);

            break;
        }

        ucCount++;
    }

    /* Check real count and list number */
    if (ucCount != prClientList->u4NumElem) {
        DBGLOG(SW4, INFO,
               "BSS[%u] Client List NUM[%u] REAL CNT[%u] ERR\n",
               prBssInfo->ucBssIndex, prClientList->u4NumElem, ucCount);

        fgError = true;
    }

    if (fgError) {
        bssDumpClientList(prAdapter, prBssInfo);
    }
}

#endif

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/* Routines for IBSS(AdHoc) only                                              */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to process Beacons from current Ad-Hoc network
 * peers. We also process Beacons from other Ad-Hoc network during SCAN. If it
 * has the same SSID and we'll decide to merge into it if it has a larger TSF.
 *
 * @param[in] prAdapter  Pointer to the Adapter structure.
 * @param[in] prBssInfo  Pointer to the BSS_INFO_T.
 * @param[in] prBSSDesc  Pointer to the BSS Descriptor.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void ibssProcessMatchedBeacon(IN P_ADAPTER_T prAdapter,
                              IN P_BSS_INFO_T prBssInfo,
                              IN P_BSS_DESC_T prBssDesc, IN u8 ucRCPI)
{
    P_STA_RECORD_T prStaRec = NULL;

    u8 fgIsCheckCapability = false;
    u8 fgIsCheckTSF = false;
    u8 fgIsGoingMerging = false;
    u8 fgIsSameBSSID;

    ASSERT(prBssInfo);
    ASSERT(prBssDesc);

    /* 4 <1> Process IBSS Beacon only after we create or merge with other
     * IBSS. */
    if (!prBssInfo->fgIsBeaconActivated) {
        return;
    }

    /* 4 <2> Get the STA_RECORD_T of TA. */
    prStaRec = cnmGetStaRecByAddress(prAdapter,
                                     prAdapter->prAisBssInfo->ucBssIndex,
                                     prBssDesc->aucSrcAddr);

    fgIsSameBSSID =
        UNEQUAL_MAC_ADDR(prBssInfo->aucBSSID, prBssDesc->aucBSSID) ?
        false :
        true;

    /* 4 <3> IBSS Merge Decision Flow for Processing Beacon. */
    if (fgIsSameBSSID) {
        /* Same BSSID:
         * Case I.  This is a new TA and it has decide to merged with
         * us. a)  If fgIsMerging == false - we will send msg to notify
         * AIS. b)  If fgIsMerging == true - already notify AIS. Case
         * II. This is an old TA and we've already merged together.
         */
        if (!prStaRec) {
            /* For Case I - Check this IBSS's capability first
             * before adding this Sta Record. */
            fgIsCheckCapability = true;

            /* If check is passed, then we perform merging with this
             * new IBSS */
            fgIsGoingMerging = true;
        } else {
            ASSERT((prStaRec->ucBssIndex ==
                    prAdapter->prAisBssInfo->ucBssIndex) &&
                   IS_ADHOC_STA(prStaRec));

            if (prStaRec->ucStaState != STA_STATE_3) {
                if (!prStaRec->fgIsMerging) {
                    /* For Case I - */
                    /* Check this IBSS's capability first
                     * before */
                    /* adding this Sta Record. */
                    fgIsCheckCapability = true;

                    /* If check is passed, then we perform
                     * merging with this new IBSS */
                    fgIsGoingMerging = true;
                } else {
                    /* For Case II - Update rExpirationTime
                     * of Sta Record */
                    GET_CURRENT_SYSTIME(
                        &prStaRec->rUpdateTime);
                }
            } else {
                /* For Case II - Update rExpirationTime of Sta
                 * Record */
                GET_CURRENT_SYSTIME(&prStaRec->rUpdateTime);
            }
        }
    } else {
        /* Unequal BSSID:
         * Case III. This is a new TA and we need to compare the TSF and
         * get the winner. Case IV.  This is an old TA and it merge into
         * a new IBSS before we do the same thing. We need to compare
         * the TSF to get the winner. Case V.   This is an old TA and it
         * restart a new IBSS. We also need to compare the TSF to get
         * the winner.
         */

        /* For Case III, IV & V - We'll always check this new IBSS's
         * capability first before merging into new IBSS.
         */
        fgIsCheckCapability = true;

        /* If check is passed, we need to perform TSF check to decide
         * the major BSSID */
        fgIsCheckTSF = true;

        /* For Case IV & V - We won't update rExpirationTime of Sta
         * Record */
    }

    /* 4 <7> Check this BSS_DESC_T's capability. */
    if (fgIsCheckCapability) {
        u8 fgIsCapabilityMatched = false;

        do {
            if (!(prBssDesc->ucPhyTypeSet &
                  (prAdapter->rWifiVar.ucAvailablePhyTypeSet))) {
                DBGLOG(BSS, LOUD,
                       "IBSS MERGE: Ignore Peer MAC: " MACSTR
                       " - Unsupported Phy.\n",
                       MAC2STR(prBssDesc->aucSrcAddr));

                break;
            }

            if (prBssDesc->fgIsUnknownBssBasicRate) {
                DBGLOG(BSS, LOUD,
                       "IBSS MERGE: Ignore Peer MAC: " MACSTR
                       " - Unknown Basic Rate.\n",
                       MAC2STR(prBssDesc->aucSrcAddr));

                break;
            }

            if (ibssCheckCapabilityForAdHocMode(prAdapter,
                                                prBssDesc) ==
                WLAN_STATUS_FAILURE) {
                DBGLOG(BSS, LOUD,
                       "IBSS MERGE: Ignore Peer MAC: " MACSTR
                       " - Capability is not matched.\n",
                       MAC2STR(prBssDesc->aucSrcAddr));

                break;
            }

            fgIsCapabilityMatched = true;
        } while (false);

        if (!fgIsCapabilityMatched) {
            if (prStaRec) {
                /* For Case II - We merge this STA_RECORD in RX
                 * Path. Case IV & V - They change their BSSID
                 * after we merge with them.
                 */

                DBGLOG(BSS, LOUD,
                       "IBSS MERGE: Ignore Peer MAC: " MACSTR
                       " - Capability is not matched.\n",
                       MAC2STR(prBssDesc->aucSrcAddr));
            }

            return;
        }

        DBGLOG(BSS, LOUD,
               "IBSS MERGE: Peer MAC: " MACSTR
               " - Check capability was passed.\n",
               MAC2STR(prBssDesc->aucSrcAddr));
    }

    if (fgIsCheckTSF) {
        if (prBssDesc->fgIsLargerTSF) {
            fgIsGoingMerging = true;
        }else{
            return;
        }
    }

    if (fgIsGoingMerging) {
        P_MSG_AIS_IBSS_PEER_FOUND_T prAisIbssPeerFoundMsg;

        /* 4 <1> We will merge with to this BSS immediately. */
        prBssDesc->fgIsConnecting = true;
        prBssDesc->fgIsConnected = false;

        /* 4 <2> Setup corresponding STA_RECORD_T */
        prStaRec = bssCreateStaRecFromBssDesc(
            prAdapter, STA_TYPE_ADHOC_PEER,
            prAdapter->prAisBssInfo->ucBssIndex, prBssDesc);

        if (!prStaRec) {
            /* no memory ? */
            return;
        }

        prStaRec->fgIsMerging = true;

        /* update RCPI */
        prStaRec->ucRCPI = ucRCPI;

        /* 4 <3> Send Merge Msg to CNM to obtain the channel privilege.
         */
        prAisIbssPeerFoundMsg =
            (P_MSG_AIS_IBSS_PEER_FOUND_T)cnmMemAlloc(
                prAdapter, RAM_TYPE_MSG,
                sizeof(MSG_AIS_IBSS_PEER_FOUND_T));

        if (!prAisIbssPeerFoundMsg) {
            ASSERT(0);  /* Can't send Merge Msg */
            return;
        }

        prAisIbssPeerFoundMsg->rMsgHdr.eMsgId = MID_SCN_AIS_FOUND_IBSS;
        prAisIbssPeerFoundMsg->ucBssIndex =
            prAdapter->prAisBssInfo->ucBssIndex;
        prAisIbssPeerFoundMsg->prStaRec = prStaRec;

        /* Inform AIS to do STATE TRANSITION
         * For Case I - If AIS in IBSS_ALONE, let it jump to NORMAL_TR
         * after we know the new member. For Case III, IV - Now this new
         * BSSID wins the TSF, follow it.
         */
        if (fgIsSameBSSID) {
            prAisIbssPeerFoundMsg->fgIsMergeIn = true;
        } else {
            prAisIbssPeerFoundMsg->fgIsMergeIn =
                (prBssDesc->fgIsLargerTSF) ? false : true;
        }

        mboxSendMsg(prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prAisIbssPeerFoundMsg,
                    MSG_SEND_METHOD_BUF);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check the Capability for Ad-Hoc to decide if we are
 *        able to merge with(same capability).
 *
 * @param[in] prBSSDesc  Pointer to the BSS Descriptor.
 *
 * @retval WLAN_STATUS_FAILURE   Can't pass the check of Capability.
 * @retval WLAN_STATUS_SUCCESS   Pass the check of Capability.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS ibssCheckCapabilityForAdHocMode(IN P_ADAPTER_T prAdapter,
                                            IN P_BSS_DESC_T prBssDesc)
{
    P_CONNECTION_SETTINGS_T prConnSettings;
    WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;

    ASSERT(prBssDesc);
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    do {
        /* 4 <1> Check the BSS Basic Rate Set for current AdHoc Mode */
        if ((prConnSettings->eAdHocMode == AD_HOC_MODE_11B) &&
            (prBssDesc->u2BSSBasicRateSet & ~RATE_SET_HR_DSSS)) {
            break;
        } else if ((prConnSettings->eAdHocMode == AD_HOC_MODE_11A) &&
                   (prBssDesc->u2BSSBasicRateSet & ~RATE_SET_OFDM)) {
            break;
        }

        /* 4 <3> Check the ATIM window setting. */
        if (prBssDesc->u2ATIMWindow) {
            DBGLOG(BSS, INFO,
                   "AdHoc PS was not supported(ATIM Window: %d)\n",
                   prBssDesc->u2ATIMWindow);
            break;
        }
        /* 4 <4> Check the Security setting. */
        if (!rsnPerformPolicySelection(prAdapter, prBssDesc)) {
            break;
        }

        rStatus = WLAN_STATUS_SUCCESS;
    } while (false);

    return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will initial the BSS_INFO_T for IBSS Mode.
 *
 * @param[in] prBssInfo      Pointer to the BSS_INFO_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void ibssInitForAdHoc(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
    u8 aucBSSID[MAC_ADDR_LEN];
    u16 *pu2BSSID = (u16 *)&aucBSSID[0];
    u32 i;

    ASSERT(prBssInfo);
    ASSERT(prBssInfo->eCurrentOPMode == OP_MODE_IBSS);

    /* 4 <1> Setup PHY Attributes and Basic Rate Set/Operational Rate Set */
    prBssInfo->ucNonHTBasicPhyType =
        (u8)rNonHTAdHocModeAttributes[prBssInfo->ucConfigAdHocAPMode]
        .ePhyTypeIndex;
    prBssInfo->u2BSSBasicRateSet =
        rNonHTAdHocModeAttributes[prBssInfo->ucConfigAdHocAPMode]
        .u2BSSBasicRateSet;

    prBssInfo->u2OperationalRateSet =
        rNonHTPhyAttributes[prBssInfo->ucNonHTBasicPhyType]
        .u2SupportedRateSet;

    rateGetDataRatesFromRateSet(prBssInfo->u2OperationalRateSet,
                                prBssInfo->u2BSSBasicRateSet,
                                prBssInfo->aucAllSupportedRates,
                                &prBssInfo->ucAllSupportedRatesLen);

    /* 4 <2> Setup BSSID */
    if (!prBssInfo->fgHoldSameBssidForIBSS) {
        for (i = 0; i < sizeof(aucBSSID) / sizeof(u16); i++)
            pu2BSSID[i] = (u16)(kalRandomNumber() & 0xFFFF);

        aucBSSID[0] &= ~0x01;  /* 7.1.3.3.3 - The individual/group bit of
                                * the address is set to 0. */
        aucBSSID[0] |= 0x02;  /* 7.1.3.3.3 - The universal/local bit of
                               * the address is set to 1. */

        COPY_MAC_ADDR(prBssInfo->aucBSSID, aucBSSID);
    }

    /* 4 <3> Setup Capability - Short Preamble */
    if (rNonHTPhyAttributes[prBssInfo->ucNonHTBasicPhyType]
        .fgIsShortPreambleOptionImplemented &&
        /* Short Preamble Option Enable is true */
        ((prAdapter->rWifiVar.ePreambleType == PREAMBLE_TYPE_SHORT) ||
         (prAdapter->rWifiVar.ePreambleType == PREAMBLE_TYPE_AUTO))) {
        prBssInfo->fgIsShortPreambleAllowed = true;
        prBssInfo->fgUseShortPreamble = true;
    } else {
        prBssInfo->fgIsShortPreambleAllowed = false;
        prBssInfo->fgUseShortPreamble = false;
    }

    /* 4 <4> Setup Capability - Short Slot Time */
    /* 7.3.1.4 For IBSS, the Short Slot Time subfield shall be set to 0. */
    prBssInfo->fgUseShortSlotTime = false;  /* Set to false for AdHoc */

    /* 4 <5> Compoase Capability */
    prBssInfo->u2CapInfo = CAP_INFO_IBSS;

    if (prBssInfo->fgIsProtection) {
        prBssInfo->u2CapInfo |= CAP_INFO_PRIVACY;
    }

    if (prBssInfo->fgIsShortPreambleAllowed) {
        prBssInfo->u2CapInfo |= CAP_INFO_SHORT_PREAMBLE;
    }

    if (prBssInfo->fgUseShortSlotTime) {
        prBssInfo->u2CapInfo |= CAP_INFO_SHORT_SLOT_TIME;
    }

    /* 4 <6> Find Lowest Basic Rate Index for default TX Rate of MMPDU */
    nicTxUpdateBssDefaultRate(prBssInfo);
}

#endif

#if CFG_SUPPORT_AAA

/*----------------------------------------------------------------------------*/
/* Routines for BSS(AP) only                                                  */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will initial the BSS_INFO_T for AP Mode.
 *
 * @param[in] prBssInfo              Given related BSS_INFO_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void bssInitForAP(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo,
                  IN u8 fgIsRateUpdate)
{
    P_AC_QUE_PARMS_T prACQueParms;

    ENUM_WMM_ACI_T eAci;

    u8 auCWminLog2ForBcast[WMM_AC_INDEX_NUM] = { 4  /*BE*/, 4  /*BK*/,
                                                 3  /*VO*/, 2  /*VI*/ };
    u8 auCWmaxLog2ForBcast[WMM_AC_INDEX_NUM] = { 10, 10, 4, 3 };
    u8 auAifsForBcast[WMM_AC_INDEX_NUM] = { 3, 7, 2, 2 };
    u8 auTxopForBcast[WMM_AC_INDEX_NUM] = { 0, 0, 94, 47 };  /* If the AP is
                                                              * OFDM */

    u8 auCWminLog2[WMM_AC_INDEX_NUM] = { 4  /*BE*/, 4  /*BK*/, 3  /*VO*/,
                                         2  /*VI*/ };
    u8 auCWmaxLog2[WMM_AC_INDEX_NUM] = { 6, 10, 4, 3 };
    u8 auAifs[WMM_AC_INDEX_NUM] = { 3, 7, 1, 1 };
    u8 auTxop[WMM_AC_INDEX_NUM] = { 0, 0, 94, 47 };  /* If the AP is OFDM */

    DEBUGFUNC("bssInitForAP");
    DBGLOG(BSS, LOUD, "\n");

    ASSERT(prBssInfo);
    ASSERT((prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) ||
           (prBssInfo->eCurrentOPMode == OP_MODE_BOW));

    /* 4 <1> Setup PHY Attributes and Basic Rate Set/Operational Rate Set */
    prBssInfo->ucNonHTBasicPhyType =
        (u8)rNonHTApModeAttributes[prBssInfo->ucConfigAdHocAPMode]
        .ePhyTypeIndex;
    prBssInfo->u2BSSBasicRateSet =
        rNonHTApModeAttributes[prBssInfo->ucConfigAdHocAPMode]
        .u2BSSBasicRateSet;

    /* 4 <1.1> Mask CCK 1M For Sco scenario except FDD mode */
    if (prAdapter->u4FddMode == false) {
        prBssInfo->u2BSSBasicRateSet &= ~RATE_SET_BIT_1M;
    }
    /* prBssInfo->u2OperationalRateSet &= ~RATE_SET_BIT_1M; */

    prBssInfo->u2OperationalRateSet =
        rNonHTPhyAttributes[prBssInfo->ucNonHTBasicPhyType]
        .u2SupportedRateSet;

    if (fgIsRateUpdate) {
        rateGetDataRatesFromRateSet(prBssInfo->u2OperationalRateSet,
                                    prBssInfo->u2BSSBasicRateSet,
                                    prBssInfo->aucAllSupportedRates,
                                    &prBssInfo->ucAllSupportedRatesLen);
    }
    /* 4 <2> Setup BSSID */
    COPY_MAC_ADDR(prBssInfo->aucBSSID, prBssInfo->aucOwnMacAddr);

    /* 4 <3> Setup Capability - Short Preamble */
    if (rNonHTPhyAttributes[prBssInfo->ucNonHTBasicPhyType]
        .fgIsShortPreambleOptionImplemented &&
        /* Short Preamble Option Enable is true */
        ((prAdapter->rWifiVar.ePreambleType == PREAMBLE_TYPE_SHORT) ||
         (prAdapter->rWifiVar.ePreambleType == PREAMBLE_TYPE_AUTO))) {
        prBssInfo->fgIsShortPreambleAllowed = true;
        prBssInfo->fgUseShortPreamble = true;
    } else {
        prBssInfo->fgIsShortPreambleAllowed = false;
        prBssInfo->fgUseShortPreamble = false;
    }

    /* 4 <4> Setup Capability - Short Slot Time */
    prBssInfo->fgUseShortSlotTime = true;

    /* 4 <5> Compoase Capability */
    prBssInfo->u2CapInfo = CAP_INFO_ESS;

    if (prBssInfo->fgIsProtection) {
        prBssInfo->u2CapInfo |= CAP_INFO_PRIVACY;
    }

    if (prBssInfo->fgIsShortPreambleAllowed) {
        prBssInfo->u2CapInfo |= CAP_INFO_SHORT_PREAMBLE;
    }

    if (prBssInfo->fgUseShortSlotTime) {
        prBssInfo->u2CapInfo |= CAP_INFO_SHORT_SLOT_TIME;
    }

    /* 4 <6> Find Lowest Basic Rate Index for default TX Rate of MMPDU */
    nicTxUpdateBssDefaultRate(prBssInfo);

    /* 4 <7> Fill the EDCA */

    prACQueParms = prBssInfo->arACQueParmsForBcast;

    for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
        prACQueParms[eAci].ucIsACMSet = false;
        prACQueParms[eAci].u2Aifsn = auAifsForBcast[eAci];
        prACQueParms[eAci].u2CWmin = BIT(auCWminLog2ForBcast[eAci]) - 1;
        prACQueParms[eAci].u2CWmax = BIT(auCWmaxLog2ForBcast[eAci]) - 1;
        prACQueParms[eAci].u2TxopLimit = auTxopForBcast[eAci];

        prBssInfo->aucCWminLog2ForBcast[eAci] =
            auCWminLog2ForBcast[eAci];  /* used to send WMM IE */
        prBssInfo->aucCWmaxLog2ForBcast[eAci] =
            auCWmaxLog2ForBcast[eAci];

        DBGLOG(BSS,
               INFO,
               "Bcast: eAci = %d, ACM = %d, Aifsn = %d, CWmin = %d, CWmax = %d, TxopLimit = %d\n",
               eAci,
               prACQueParms[eAci].ucIsACMSet,
               prACQueParms[eAci].u2Aifsn,
               prACQueParms[eAci].u2CWmin,
               prACQueParms[eAci].u2CWmax,
               prACQueParms[eAci].u2TxopLimit);
    }

    prACQueParms = prBssInfo->arACQueParms;

    for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
        prACQueParms[eAci].ucIsACMSet = false;
        prACQueParms[eAci].u2Aifsn = auAifs[eAci];
        prACQueParms[eAci].u2CWmin = BIT(auCWminLog2[eAci]) - 1;
        prACQueParms[eAci].u2CWmax = BIT(auCWmaxLog2[eAci]) - 1;
        prACQueParms[eAci].u2TxopLimit = auTxop[eAci];

        DBGLOG(BSS,
               INFO,
               "eAci = %d, ACM = %d, Aifsn = %d, CWmin = %d, CWmax = %d, TxopLimit = %d\n",
               eAci,
               prACQueParms[eAci].ucIsACMSet,
               prACQueParms[eAci].u2Aifsn,
               prACQueParms[eAci].u2CWmin,
               prACQueParms[eAci].u2CWmax,
               prACQueParms[eAci].u2TxopLimit);
    }

    /* Note: Caller should update the EDCA setting to HW by
     * nicQmUpdateWmmParms() it there is no AIS network */
    /* Note: In E2, only 4 HW queues. The the Edca parameters should be
     * folow by AIS network */
    /* Note: In E3, 8 HW queues.  the Wmm parameters should be updated to
     * right queues  according to BSS */
}

#endif

void bssDumpBssInfo(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex)
{
    P_BSS_INFO_T prBssInfo;
    /* P_LINK_T prStaRecOfClientList = (P_LINK_T) NULL; */
    /* P_STA_RECORD_T prCurrStaRec = (P_STA_RECORD_T) NULL; */

    if (ucBssIndex > MAX_BSS_INDEX) {
        DBGLOG(SW4, INFO, "Invalid BssInfo index[%u], skip dump!\n",
               ucBssIndex);
        return;
    }

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    if (!prBssInfo) {
        DBGLOG(SW4, INFO, "Invalid BssInfo index[%u], skip dump!\n",
               ucBssIndex);
        return;
    }

    DBGLOG(SW4, INFO, "OWNMAC[" MACSTR "] BSSID[" MACSTR "] SSID[%s]\n",
           MAC2STR(prBssInfo->aucOwnMacAddr), MAC2STR(prBssInfo->aucBSSID),
           prBssInfo->aucSSID);

    DBGLOG(SW4, INFO,
           "BSS IDX[%u] Type[%s] OPMode[%s] ConnState[%u] Absent[%u]\n",
           prBssInfo->ucBssIndex, apucNetworkType[prBssInfo->eNetworkType],
           apucNetworkOpMode[prBssInfo->eCurrentOPMode],
           prBssInfo->eConnectionState, prBssInfo->fgIsNetAbsent);

    DBGLOG(SW4,
           INFO,
           "Channel[%u] Band[%u] SCO[%u] Assoc40mBwAllowed[%u] 40mBwAllowed[%u] MaxBw[%u] Nss[%u] eDBDCBand[%u]\n",
           prBssInfo->ucPrimaryChannel,
           prBssInfo->eBand,
           prBssInfo->eBssSCO,
           prBssInfo->fgAssoc40mBwAllowed,
           prBssInfo->fg40mBwAllowed,
           cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex),
           prBssInfo->ucNss,
           prBssInfo->eDBDCBand);

    DBGLOG(SW4, INFO, "QBSS[%u] CapInfo[0x%04x] AID[%u]\n",
           prBssInfo->fgIsQBSS, prBssInfo->u2CapInfo, prBssInfo->u2AssocId);

    DBGLOG(SW4, INFO,
           "ShortPreamble Allowed[%u] EN[%u], ShortSlotTime[%u]\n",
           prBssInfo->fgIsShortPreambleAllowed,
           prBssInfo->fgUseShortPreamble, prBssInfo->fgUseShortSlotTime);

    DBGLOG(SW4, INFO, "PhyTypeSet: Basic[0x%02x] NonHtBasic[0x%02x]\n",
           prBssInfo->ucPhyTypeSet, prBssInfo->ucNonHTBasicPhyType);

    DBGLOG(SW4, INFO, "RateSet: BssBasic[0x%04x] Operational[0x%04x]\n",
           prBssInfo->u2BSSBasicRateSet, prBssInfo->u2OperationalRateSet);

    DBGLOG(SW4, INFO, "ATIMWindow[%u] DTIM Period[%u] Count[%u]\n",
           prBssInfo->u2ATIMWindow, prBssInfo->ucDTIMPeriod,
           prBssInfo->ucDTIMCount);

    DBGLOG(SW4, INFO,
           "HT Operation Info1[0x%02x] Info2[0x%04x] Info3[0x%04x]\n",
           prBssInfo->ucHtOpInfo1, prBssInfo->u2HtOpInfo2,
           prBssInfo->u2HtOpInfo3);

    DBGLOG(SW4, INFO,
           "ProtectMode HT[%u] ERP[%u], OperationMode GF[%u] RIFS[%u]\n",
           prBssInfo->eHtProtectMode, prBssInfo->fgErpProtectMode,
           prBssInfo->eGfOperationMode, prBssInfo->eRifsOperationMode);

    DBGLOG(SW4,
           INFO,
           "(OBSS) ProtectMode HT[%u] ERP[%u], OperationMode GF[%u] RIFS[%u]\n",
           prBssInfo->eObssHtProtectMode,
           prBssInfo->fgObssErpProtectMode,
           prBssInfo->eObssGfOperationMode,
           prBssInfo->fgObssRifsOperationMode);

    DBGLOG(SW4,
           INFO,
           "VhtChannelWidth[%u] OpChangeChannelWidth[%u], IsOpChangeChannelWidth[%u]\n",
           prBssInfo->ucVhtChannelWidth,
           prBssInfo->ucOpChangeChannelWidth,
           prBssInfo->fgIsOpChangeChannelWidth);

    DBGLOG(SW4, INFO, "======== Dump Connected Client ========\n");

    bssDumpClientList(prAdapter, prBssInfo);

    DBGLOG(SW4, INFO, "============== Dump Done ==============\n");
}
