// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "p2p_rlm.c"
 *    \brief
 *
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
#include "rlm.h"

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
 * \brief  Init AP Bss
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmBssInitForAP(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo)
{
    u8 i;

    ASSERT(prAdapter);
    ASSERT(prBssInfo);

    if (prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) {
        return;
    }

    /* Operation band, channel shall be ready before invoking this function.
     * Bandwidth may be ready if other network is connected
     */
    prBssInfo->fg40mBwAllowed = false;
    prBssInfo->fgAssoc40mBwAllowed = false;
    prBssInfo->eBssSCO = CHNL_EXT_SCN;

    /* Check if AP can set its bw to 40MHz
     * But if any of BSS is setup in 40MHz, the second BSS would prefer to
     * use 20MHz in order to remain in SCC case
     */
    if (cnmBss40mBwPermitted(prAdapter, prBssInfo->ucBssIndex)) {
        prBssInfo->eBssSCO = rlmGetScoForAP(prAdapter, prBssInfo);

        if (prBssInfo->eBssSCO != CHNL_EXT_SCN) {
            prBssInfo->fg40mBwAllowed = true;
            prBssInfo->fgAssoc40mBwAllowed = true;

            prBssInfo->ucHtOpInfo1 =
                (u8)(((u32)prBssInfo->eBssSCO) |
                     HT_OP_INFO1_STA_CHNL_WIDTH);

            rlmUpdateBwByChListForAP(prAdapter, prBssInfo);
        }
    }

    /* Filled the VHT BW/S1/S2 and MCS rate set */
    if (prBssInfo->ucPhyTypeSet & PHY_TYPE_BIT_VHT) {
        for (i = 0; i < 8; i++)
            prBssInfo->u2VhtBasicMcsSet |= BITS(2 * i, (2 * i + 1));
        prBssInfo->u2VhtBasicMcsSet &= (VHT_CAP_INFO_MCS_MAP_MCS9
                                        << VHT_CAP_INFO_MCS_1SS_OFFSET);

        prBssInfo->ucVhtChannelWidth = cnmGetBssMaxBwToChnlBW(
            prAdapter, prBssInfo->ucBssIndex);
        if (prBssInfo->ucVhtChannelWidth ==
            VHT_OP_CHANNEL_WIDTH_80P80) {
            /* TODO: BW80+80 support */
            DBGLOG(RLM,
                   WARN,
                   "BW80+80 not support. Fallback  to VHT_OP_CHANNEL_WIDTH_20_40\n");
            prBssInfo->ucVhtChannelWidth =
                VHT_OP_CHANNEL_WIDTH_20_40;
            prBssInfo->ucVhtChannelFrequencyS1 = 0;
            prBssInfo->ucVhtChannelFrequencyS2 = 0;
        } else {
            prBssInfo->ucVhtChannelFrequencyS1 =
                rlmGetVhtS1ForAP(prAdapter, prBssInfo);
            prBssInfo->ucVhtChannelFrequencyS2 = 0;
        }

        /* If the S1 is invalid, force to change bandwidth */
        if (prBssInfo->ucVhtChannelFrequencyS1 == 0) {
            prBssInfo->ucVhtChannelWidth =
                VHT_OP_CHANNEL_WIDTH_20_40;
        }
    } else {
        prBssInfo->ucVhtChannelWidth = VHT_OP_CHANNEL_WIDTH_20_40;
        prBssInfo->ucVhtChannelFrequencyS1 = 0;
        prBssInfo->ucVhtChannelFrequencyS2 = 0;
    }

    /*ERROR HANDLE*/
    if ((prBssInfo->ucVhtChannelWidth == VHT_OP_CHANNEL_WIDTH_80) ||
        (prBssInfo->ucVhtChannelWidth == VHT_OP_CHANNEL_WIDTH_160) ||
        (prBssInfo->ucVhtChannelWidth == VHT_OP_CHANNEL_WIDTH_80P80)) {
        if (prBssInfo->ucVhtChannelFrequencyS1 == 0) {
            DBGLOG(RLM,
                   INFO,
                   "Wrong AP S1 parameter setting, back to BW20!!!\n");

            prBssInfo->ucVhtChannelWidth =
                VHT_OP_CHANNEL_WIDTH_20_40;
            prBssInfo->ucVhtChannelFrequencyS1 = 0;
            prBssInfo->ucVhtChannelFrequencyS2 = 0;
        }
    }

    DBGLOG(RLM, INFO, "WLAN AP SCO=%d BW=%d S1=%d S2=%d CH=%d Band=%d\n",
           prBssInfo->eBssSCO, prBssInfo->ucVhtChannelWidth,
           prBssInfo->ucVhtChannelFrequencyS1,
           prBssInfo->ucVhtChannelFrequencyS2, prBssInfo->ucPrimaryChannel,
           prBssInfo->eBand);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief For probe response (GO, IBSS) and association response
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmRspGenerateObssScanIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo)
{
    P_BSS_INFO_T prBssInfo;
    P_IE_OBSS_SCAN_PARAM_T prObssScanIe;
    P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL;

    ASSERT(prAdapter);
    ASSERT(prMsduInfo);

    prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
    if (!prBssInfo) {
        return;
    }

    if (!IS_BSS_ACTIVE(prBssInfo)) {
        return;
    }

    if (RLM_NET_IS_11N(prBssInfo) &&  /* !RLM_NET_IS_BOW(prBssInfo) && FIXME.
                                       */
        prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT &&
        (!prStaRec || (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)) &&
        prBssInfo->eBand == BAND_2G4 &&
        prBssInfo->eBssSCO != CHNL_EXT_SCN) {
        prObssScanIe =
            (P_IE_OBSS_SCAN_PARAM_T)(((u8 *)prMsduInfo->prPacket) +
                                     prMsduInfo->u2FrameLength);

        /* Add 20/40 BSS coexistence IE */
        prObssScanIe->ucId = ELEM_ID_OBSS_SCAN_PARAMS;
        prObssScanIe->ucLength =
            sizeof(IE_OBSS_SCAN_PARAM_T) - ELEM_HDR_LEN;

        prObssScanIe->u2ScanPassiveDwell = dot11OBSSScanPassiveDwell;
        prObssScanIe->u2ScanActiveDwell = dot11OBSSScanActiveDwell;
        prObssScanIe->u2TriggerScanInterval =
            dot11BSSWidthTriggerScanInterval;
        prObssScanIe->u2ScanPassiveTotalPerChnl =
            dot11OBSSScanPassiveTotalPerChannel;
        prObssScanIe->u2ScanActiveTotalPerChnl =
            dot11OBSSScanActiveTotalPerChannel;
        prObssScanIe->u2WidthTransDelayFactor =
            dot11BSSWidthChannelTransitionDelayFactor;
        prObssScanIe->u2ScanActivityThres =
            dot11OBSSScanActivityThreshold;

        ASSERT(IE_SIZE(prObssScanIe) <=
               (ELEM_HDR_LEN + ELEM_MAX_LEN_OBSS_SCAN));

        prMsduInfo->u2FrameLength += IE_SIZE(prObssScanIe);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief P2P GO.
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
u8 rlmUpdateBwByChListForAP(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo)
{
    u8 ucLevel;
    u8 fgBwChange;

    ASSERT(prAdapter);
    ASSERT(prBssInfo);

    fgBwChange = false;

    if (prBssInfo->eBssSCO == CHNL_EXT_SCN) {
        return fgBwChange;
    }

    ucLevel = rlmObssChnlLevel(prBssInfo, prBssInfo->eBand,
                               prBssInfo->ucPrimaryChannel,
                               prBssInfo->eBssSCO);

    if (ucLevel == CHNL_LEVEL0) {
        /* Forced to 20MHz, so extended channel is SCN and STA width is
         * zero */
        prBssInfo->fgObssActionForcedTo20M = true;

        if (prBssInfo->ucHtOpInfo1 != (u8)CHNL_EXT_SCN) {
            prBssInfo->ucHtOpInfo1 = (u8)CHNL_EXT_SCN;
            fgBwChange = true;
        }

        cnmTimerStartTimer(prAdapter, &prBssInfo->rObssScanTimer,
                           OBSS_20_40M_TIMEOUT * MSEC_PER_SEC);
    }

    /* Clear up all channel lists */
    prBssInfo->auc2G_20mReqChnlList[0] = 0;
    prBssInfo->auc2G_NonHtChnlList[0] = 0;
    prBssInfo->auc2G_PriChnlList[0] = 0;
    prBssInfo->auc2G_SecChnlList[0] = 0;
    prBssInfo->auc5G_20mReqChnlList[0] = 0;
    prBssInfo->auc5G_NonHtChnlList[0] = 0;
    prBssInfo->auc5G_PriChnlList[0] = 0;
    prBssInfo->auc5G_SecChnlList[0] = 0;

    return fgBwChange;
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
void rlmProcessPublicAction(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb)
{
    P_ACTION_20_40_COEXIST_FRAME prRxFrame;
    P_IE_20_40_COEXIST_T prCoexist;
    P_IE_INTOLERANT_CHNL_REPORT_T prChnlReport;
    P_BSS_INFO_T prBssInfo;
    P_STA_RECORD_T prStaRec;
    u8 *pucIE;
    u16 u2IELength, u2Offset;
    u8 i, j;

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prRxFrame = (P_ACTION_20_40_COEXIST_FRAME)prSwRfb->pvHeader;
    prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);

    if (!(prSwRfb->prStaRec)) {
        DBGLOG(P2P, INFO, "prSwRfb->prStaRec is null.\n");
        return;
    }

    if (prRxFrame->ucAction != ACTION_PUBLIC_20_40_COEXIST || !prStaRec ||
        prStaRec->ucStaState != STA_STATE_3 ||
        prSwRfb->u2PacketLen < (WLAN_MAC_MGMT_HEADER_LEN + 5) ||
        prSwRfb->prStaRec->ucBssIndex !=
        /* HIF_RX_HDR_GET_NETWORK_IDX(prSwRfb->prHifRxHdr) != */
        prStaRec->ucBssIndex) {
        return;
    }

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
    ASSERT(prBssInfo);

    if (!IS_BSS_ACTIVE(prBssInfo) ||
        prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT ||
        prBssInfo->eBssSCO == CHNL_EXT_SCN) {
        return;
    }

    prCoexist = &prRxFrame->rBssCoexist;
    if (prCoexist->ucData &
        (BSS_COEXIST_40M_INTOLERANT | BSS_COEXIST_20M_REQ)) {
        ASSERT(prBssInfo->auc2G_20mReqChnlList[0] <= CHNL_LIST_SZ_2G);
        for (i = 1; i <= prBssInfo->auc2G_20mReqChnlList[0] &&
             i <= CHNL_LIST_SZ_2G;
             i++) {
            if (prBssInfo->auc2G_20mReqChnlList[i] ==
                prBssInfo->ucPrimaryChannel) {
                break;
            }
        }
        if ((i > prBssInfo->auc2G_20mReqChnlList[0]) &&
            (i <= CHNL_LIST_SZ_2G)) {
            prBssInfo->auc2G_20mReqChnlList[i] =
                prBssInfo->ucPrimaryChannel;
            prBssInfo->auc2G_20mReqChnlList[0]++;
        }
    }

    /* Process intolerant channel report IE */
    pucIE = (u8 *)&prRxFrame->rChnlReport;
    u2IELength = prSwRfb->u2PacketLen - (WLAN_MAC_MGMT_HEADER_LEN + 5);

    IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
        switch (IE_ID(pucIE)) {
        case ELEM_ID_20_40_INTOLERANT_CHNL_REPORT:
            prChnlReport = (P_IE_INTOLERANT_CHNL_REPORT_T)pucIE;

            if (prChnlReport->ucLength <= 1) {
                break;
            }

            /* To do: process regulatory class. Now we assume 2.4G
             * band */

            for (j = 0; j < prChnlReport->ucLength - 1; j++) {
                /* Update non-HT channel list */
                ASSERT(prBssInfo->auc2G_NonHtChnlList[0] <=
                       CHNL_LIST_SZ_2G);
                for (i = 1;
                     i <= prBssInfo->auc2G_NonHtChnlList[0] &&
                     i <= CHNL_LIST_SZ_2G;
                     i++) {
                    if (prBssInfo->auc2G_NonHtChnlList[i] ==
                        prChnlReport->aucChannelList[j]) {
                        break;
                    }
                }
                if ((i > prBssInfo->auc2G_NonHtChnlList[0]) &&
                    (i <= CHNL_LIST_SZ_2G)) {
                    prBssInfo->auc2G_NonHtChnlList[i] =
                        prChnlReport->aucChannelList[j];
                    prBssInfo->auc2G_NonHtChnlList[0]++;
                }
            }
            break;

        default:
            break;
        }
    }  /* end of IE_FOR_EACH */

    if (rlmUpdateBwByChListForAP(prAdapter, prBssInfo)) {
        bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);
        rlmSyncOperationParams(prAdapter, prBssInfo);
    }

    /* Check if OBSS scan exemption response should be sent */
    if (prCoexist->ucData & BSS_COEXIST_OBSS_SCAN_EXEMPTION_REQ) {
        rlmObssScanExemptionRsp(prAdapter, prBssInfo, prSwRfb);
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
void rlmHandleObssStatusEventPkt(P_ADAPTER_T prAdapter,
                                 P_EVENT_AP_OBSS_STATUS_T prObssStatus)
{
    P_BSS_INFO_T prBssInfo;

    ASSERT(prAdapter);
    ASSERT(prObssStatus);
    // ASSERT(prObssStatus->ucBssIndex < MAX_BSS_INDEX);
    if (prObssStatus->ucBssIndex >= MAX_BSS_INDEX) {
        DBGLOG(RLM,
               ERROR,
               "rlmHandleObssStatusEventPkt: (ucBssIndex = %d) out-of-bound\n",
               prObssStatus->ucBssIndex);
        return;
    }

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prObssStatus->ucBssIndex);

    if (prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) {
        return;
    }

    prBssInfo->fgObssErpProtectMode =
        (u8)prObssStatus->ucObssErpProtectMode;
    prBssInfo->eObssHtProtectMode =
        (ENUM_HT_PROTECT_MODE_T)prObssStatus->ucObssHtProtectMode;
    prBssInfo->eObssGfOperationMode =
        (ENUM_GF_MODE_T)prObssStatus->ucObssGfOperationMode;
    prBssInfo->fgObssRifsOperationMode =
        (u8)prObssStatus->ucObssRifsOperationMode;
    prBssInfo->fgObssBeaconForcedTo20M =
        (u8)prObssStatus->ucObssBeaconForcedTo20M;

    /* Check if Beacon content need to be updated */
    rlmUpdateParamsForAP(prAdapter, prBssInfo, true);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief It is only for AP mode in NETWORK_TYPE_P2P_INDEX.
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmUpdateParamsForAP(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo,
                          u8 fgUpdateBeacon)
{
    P_LINK_T prStaList;
    P_STA_RECORD_T prStaRec;
    u8 fgErpProtectMode, fgSta40mIntolerant;
    u8 fgUseShortPreamble, fgUseShortSlotTime;
    ENUM_HT_PROTECT_MODE_T eHtProtectMode;
    ENUM_GF_MODE_T eGfOperationMode;
    u8 ucHtOpInfo1;

    ASSERT(prAdapter);
    ASSERT(prBssInfo);

    if (!IS_BSS_ACTIVE(prBssInfo) ||
        prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) {
        return;
    }

    fgErpProtectMode = false;
    eHtProtectMode = HT_PROTECT_MODE_NONE;
    eGfOperationMode = GF_MODE_NORMAL;
    fgSta40mIntolerant = false;
    fgUseShortPreamble = prBssInfo->fgIsShortPreambleAllowed;
    fgUseShortSlotTime = true;
    ucHtOpInfo1 = (u8)CHNL_EXT_SCN;

    prStaList = &prBssInfo->rStaRecOfClientList;

    LINK_FOR_EACH_ENTRY(prStaRec, prStaList, rLinkEntry, STA_RECORD_T) {
        if (!prStaRec) {
            DBGLOG(P2P, WARN,
                   "NULL STA_REC ptr in BSS client list\n");
            bssDumpClientList(prAdapter, prBssInfo);
            break;
        }

        if (prStaRec->fgIsInUse &&
            prStaRec->ucStaState == STA_STATE_3 &&
            prStaRec->ucBssIndex == prBssInfo->ucBssIndex) {
            if (!(prStaRec->ucPhyTypeSet &
                  (PHY_TYPE_SET_802_11GN | PHY_TYPE_SET_802_11A))) {
                /* B-only mode, so mode 1 (ERP protection) */
                fgErpProtectMode = true;
            }

            if (!(prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)) {
                /* BG-only or A-only */
                eHtProtectMode = HT_PROTECT_MODE_NON_HT;
            } else if (prBssInfo->fg40mBwAllowed &&
                       !(prStaRec->u2HtCapInfo &
                         HT_CAP_INFO_SUP_CHNL_WIDTH)) {
                /* 20MHz-only */
                if (eHtProtectMode == HT_PROTECT_MODE_NONE) {
                    eHtProtectMode = HT_PROTECT_MODE_20M;
                }
            }

            if (!(prStaRec->u2HtCapInfo & HT_CAP_INFO_HT_GF)) {
                eGfOperationMode = GF_MODE_PROTECT;
            }

            if (!(prStaRec->u2CapInfo & CAP_INFO_SHORT_PREAMBLE)) {
                fgUseShortPreamble = false;
            }

            /*ap mode throughput enhancement
             * only 2.4G with B mode client connecion use long slot
             * time
             */
            if ((!(prStaRec->u2CapInfo &
                   CAP_INFO_SHORT_SLOT_TIME)) &&
                fgErpProtectMode && prBssInfo->eBand == BAND_2G4) {
                fgUseShortSlotTime = false;
            }

            if (prStaRec->u2HtCapInfo & HT_CAP_INFO_40M_INTOLERANT) {
                fgSta40mIntolerant = true;
            }
        }
    }  /* end of LINK_FOR_EACH_ENTRY */

    /* Check if HT operation IE about 20/40M bandwidth shall be updated */
    if (prBssInfo->eBssSCO != CHNL_EXT_SCN) {
        if (  /*!LINK_IS_EMPTY(prStaList) && */ !fgSta40mIntolerant &&
              !prBssInfo->fgObssActionForcedTo20M &&
              !prBssInfo->fgObssBeaconForcedTo20M) {
            ucHtOpInfo1 = (u8)(((u32)prBssInfo->eBssSCO) |
                               HT_OP_INFO1_STA_CHNL_WIDTH);
        }
    }

    /* Check if any new parameter may be updated */
    if (prBssInfo->fgErpProtectMode != fgErpProtectMode ||
        prBssInfo->eHtProtectMode != eHtProtectMode ||
        prBssInfo->eGfOperationMode != eGfOperationMode ||
        prBssInfo->ucHtOpInfo1 != ucHtOpInfo1 ||
        prBssInfo->fgUseShortPreamble != fgUseShortPreamble ||
        prBssInfo->fgUseShortSlotTime != fgUseShortSlotTime) {
        prBssInfo->fgErpProtectMode = fgErpProtectMode;
        prBssInfo->eHtProtectMode = eHtProtectMode;
        prBssInfo->eGfOperationMode = eGfOperationMode;
        prBssInfo->ucHtOpInfo1 = ucHtOpInfo1;
        prBssInfo->fgUseShortPreamble = fgUseShortPreamble;
        prBssInfo->fgUseShortSlotTime = fgUseShortSlotTime;

        if (fgUseShortSlotTime) {
            prBssInfo->u2CapInfo |= CAP_INFO_SHORT_SLOT_TIME;
        }else{
            prBssInfo->u2CapInfo &= ~CAP_INFO_SHORT_SLOT_TIME;
        }

        rlmSyncOperationParams(prAdapter, prBssInfo);
        fgUpdateBeacon = true;
    }

    /* Update Beacon content if related IE content is changed */
    if (fgUpdateBeacon) {
        bssUpdateBeaconContent(prAdapter, prBssInfo->ucBssIndex);
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
ENUM_CHNL_EXT_T rlmDecideScoForAP(P_ADAPTER_T prAdapter,
                                  P_BSS_INFO_T prBssInfo)
{
    P_DOMAIN_SUBBAND_INFO prSubband;
    P_DOMAIN_INFO_ENTRY prDomainInfo;
    u8 ucSecondChannel, i, j;
    ENUM_CHNL_EXT_T eSCO;
    ENUM_CHNL_EXT_T eTempSCO;
    u8 ucMaxBandwidth = MAX_BW_80_80_MHZ;  /*chip capability*/

    eSCO = CHNL_EXT_SCN;
    eTempSCO = CHNL_EXT_SCN;

    if (prBssInfo->eBand == BAND_2G4) {
        if (prBssInfo->ucPrimaryChannel != 14) {
            eSCO = (prBssInfo->ucPrimaryChannel > 7) ?
                   CHNL_EXT_SCB :
                   CHNL_EXT_SCA;
        }
    } else {
        if (regd_is_single_sku_en()) {
            if (rlmDomainIsLegalChannel(
                    prAdapter, prBssInfo->eBand,
                    prBssInfo->ucPrimaryChannel)) {
                eSCO = rlmSelectSecondaryChannelType(
                    prAdapter, prBssInfo->eBand,
                    prBssInfo->ucPrimaryChannel);
            }
        } else {
            prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
            ASSERT(prDomainInfo);

            for (i = 0; i < MAX_SUBBAND_NUM; i++) {
                prSubband = &prDomainInfo->rSubBand[i];
                if (prSubband->ucBand == prBssInfo->eBand) {
                    for (j = 0;
                         j < prSubband->ucNumChannels;
                         j++) {
                        if ((prSubband->
                             ucFirstChannelNum +
                             j *
                             prSubband->ucChannelSpan)
                            ==
                            prBssInfo->ucPrimaryChannel) {
                            eSCO = (j & 1) ?
                                   CHNL_EXT_SCB :
                                   CHNL_EXT_SCA;
                            break;
                        }
                    }

                    if (j < prSubband->ucNumChannels) {
                        break;  /* Found */
                    }
                }
            }
        }
    }

    /* Check if it is boundary channel and 40MHz BW is permitted */
    if (eSCO != CHNL_EXT_SCN) {
        ucSecondChannel =
            (eSCO == CHNL_EXT_SCA) ?
            (prBssInfo->ucPrimaryChannel + CHNL_SPAN_20) :
            (prBssInfo->ucPrimaryChannel - CHNL_SPAN_20);

        if (!rlmDomainIsLegalChannel(prAdapter, prBssInfo->eBand,
                                     ucSecondChannel)) {
            eSCO = CHNL_EXT_SCN;
        }
    }

    /* Overwrite SCO settings by wifi cfg */
    if (IS_BSS_P2P(prBssInfo)) {
        /* AP mode */
        if (p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings
                            [prBssInfo->u4PrivateData])) {
            if (prAdapter->rWifiVar.ucApSco == CHNL_EXT_SCA ||
                prAdapter->rWifiVar.ucApSco == CHNL_EXT_SCB) {
                eTempSCO = (ENUM_CHNL_EXT_T)
                           prAdapter->rWifiVar.ucApSco;
            }
        }
        /* P2P mode */
        else {
            if (prAdapter->rWifiVar.ucP2pGoSco == CHNL_EXT_SCA ||
                prAdapter->rWifiVar.ucP2pGoSco == CHNL_EXT_SCB) {
                eTempSCO =
                    (ENUM_CHNL_EXT_T)
                    prAdapter->rWifiVar.ucP2pGoSco;
            }
        }

        /* Check again if it is boundary channel and 40MHz BW is
         * permitted */
        if (eTempSCO != CHNL_EXT_SCN) {
            ucSecondChannel =
                (eTempSCO == CHNL_EXT_SCA) ?
                (prBssInfo->ucPrimaryChannel + 4) :
                (prBssInfo->ucPrimaryChannel - 4);
            if (rlmDomainIsLegalChannel(prAdapter, prBssInfo->eBand,
                                        ucSecondChannel)) {
                eSCO = eTempSCO;
            }
        }
    }

    /* Overwrite SCO settings by wifi cfg bandwidth setting */
    if (IS_BSS_P2P(prBssInfo)) {
        /* AP mode */
        if (p2pFuncIsAPMode(prAdapter->rWifiVar.prP2PConnSettings
                            [prBssInfo->u4PrivateData])) {
            if (prBssInfo->eBand == BAND_2G4) {
                ucMaxBandwidth =
                    prAdapter->rWifiVar.ucAp2gBandwidth;
            } else {
                ucMaxBandwidth =
                    prAdapter->rWifiVar.ucAp5gBandwidth;
            }
        }
        /* P2P mode */
        else {
            if (prBssInfo->eBand == BAND_2G4) {
                ucMaxBandwidth =
                    prAdapter->rWifiVar.ucP2p2gBandwidth;
            } else {
                ucMaxBandwidth =
                    prAdapter->rWifiVar.ucP2p5gBandwidth;
            }
        }

        if (ucMaxBandwidth < MAX_BW_40MHZ) {
            eSCO = CHNL_EXT_SCN;
        }
    }

    return eSCO;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief: Get AP secondary channel offset from cfg80211 or wifi.cfg
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T, prBssInfo Pointer of BSS_INFO_T,
 *
 * \return ENUM_CHNL_EXT_T AP secondary channel offset
 */
/*----------------------------------------------------------------------------*/
ENUM_CHNL_EXT_T rlmGetScoForAP(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo)
{
    ENUM_BAND_T eBand;
    u8 ucChannel;
    ENUM_CHNL_EXT_T eSCO;
    s32 i4DeltaBw;
    u32 u4AndOneSCO;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_P2P_CONNECTION_REQ_INFO_T prP2pConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;

    prP2pRoleFsmInfo =
        p2pFuncGetRoleByBssIdx(prAdapter, prBssInfo->ucBssIndex);

    if (!prAdapter->rWifiVar.ucApChnlDefFromCfg && prP2pRoleFsmInfo) {
        prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
        eSCO = CHNL_EXT_SCN;
        if (cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex) ==
            MAX_BW_40MHZ) {
            /* If BW 40, compare S0 and primary channel freq */
            if (prP2pConnReqInfo->u4CenterFreq1 >
                prP2pConnReqInfo->u2PriChnlFreq) {
                eSCO = CHNL_EXT_SCA;
            }else{
                eSCO = CHNL_EXT_SCB;
            }
        } else if (cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex) >
                   MAX_BW_40MHZ) {
            /* P: PriChnlFreq, A:CHNL_EXT_SCA, B: CHNL_EXT_SCB, -:BW
             * SPAN 5M */
            /* --|----|--CenterFreq1--|----|-- */
            /* --|----|--CenterFreq1--B----P-- */
            /* --|----|--CenterFreq1--P----A-- */
            i4DeltaBw = prP2pConnReqInfo->u2PriChnlFreq -
                        prP2pConnReqInfo->u4CenterFreq1;
            u4AndOneSCO = CHNL_EXT_SCB;
            eSCO = CHNL_EXT_SCA;
            if (i4DeltaBw < 0) {
                /* --|----|--CenterFreq1--|----|-- */
                /* --P----A--CenterFreq1--|----|-- */
                /* --B----P--CenterFreq1--|----|-- */
                u4AndOneSCO = CHNL_EXT_SCA;
                eSCO = CHNL_EXT_SCB;
                i4DeltaBw = -i4DeltaBw;
            }
            i4DeltaBw = i4DeltaBw - (CHANNEL_SPAN_20 >> 1);
            if ((i4DeltaBw / CHANNEL_SPAN_20) & 1) {
                eSCO = u4AndOneSCO;
            }
        }
    } else {
        /* In this case, the first BSS's SCO is 40MHz and known, so AP
         * can apply 40MHz bandwidth, but the first BSS's SCO may be
         * changed later if its Beacon lost timeout occurs
         */
        if (!(cnmPreferredChannel(prAdapter, &eBand, &ucChannel,
                                  &eSCO) &&
              eSCO != CHNL_EXT_SCN &&
              ucChannel == prBssInfo->ucPrimaryChannel &&
              eBand == prBssInfo->eBand)) {
            eSCO = rlmDecideScoForAP(prAdapter, prBssInfo);
        }
    }
    return eSCO;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief: Get AP channel number of Channel Center Frequency Segment 0 from
 * cfg80211 or wifi.cfg
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T, prBssInfo Pointer of BSS_INFO_T,
 *
 * \return u8 AP channel number of Channel Center Frequency Segment 0
 */
/*----------------------------------------------------------------------------*/
u8 rlmGetVhtS1ForAP(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo)
{
    u32 ucFreq1Channel;
    u8 ucPrimaryChannel = prBssInfo->ucPrimaryChannel;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_P2P_CONNECTION_REQ_INFO_T prP2pConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;

    prP2pRoleFsmInfo =
        p2pFuncGetRoleByBssIdx(prAdapter, prBssInfo->ucBssIndex);

    if (prBssInfo->ucVhtChannelWidth == VHT_OP_CHANNEL_WIDTH_20_40) {
        return 0;
    }

    if (!prAdapter->rWifiVar.ucApChnlDefFromCfg && prP2pRoleFsmInfo) {
        prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
        ucFreq1Channel = nicFreq2ChannelNum(
            prP2pConnReqInfo->u4CenterFreq1 * 1000);
    } else {
        ucFreq1Channel = nicGetVhtS1(ucPrimaryChannel,
                                     prBssInfo->ucVhtChannelWidth);
    }

    return ucFreq1Channel;
}
