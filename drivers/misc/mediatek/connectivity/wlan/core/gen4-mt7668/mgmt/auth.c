// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "auth.c"
 *    \brief  This file includes the authentication-related functions.
 *
 *   This file includes the authentication-related functions.
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

APPEND_IE_ENTRY_T txAuthIETable[] = {
    { (ELEM_HDR_LEN + ELEM_MAX_LEN_CHALLENGE_TEXT), authAddIEChallengeText }
};

HANDLE_IE_ENTRY_T rxAuthIETable[] = { { ELEM_ID_CHALLENGE_TEXT,
                                        authHandleIEChallengeText } };

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
 * @brief This function will compose the Authentication frame header and fixed
 * fields.
 *
 * @param[in] pucBuffer              Pointer to the frame buffer.
 * @param[in] aucPeerMACAddress      Given Peer MAC Address.
 * @param[in] aucMACAddress          Given Our MAC Address.
 * @param[in] u2AuthAlgNum           Authentication Algorithm Number
 * @param[in] u2TransactionSeqNum    Transaction Sequence Number
 * @param[in] u2StatusCode           Status Code
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void authComposeAuthFrameHeaderAndFF(
    IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec, IN u8 *pucBuffer,
    IN u8 aucPeerMACAddress[], IN u8 aucMACAddress[], IN u16 u2AuthAlgNum,
    IN u16 u2TransactionSeqNum, IN u16 u2StatusCode)
{
    P_WLAN_AUTH_FRAME_T prAuthFrame;
    u16 u2FrameCtrl;
    P_CONNECTION_SETTINGS_T prConnSettings;
    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    ASSERT(pucBuffer);
    ASSERT(aucPeerMACAddress);
    ASSERT(aucMACAddress);

    prAuthFrame = (P_WLAN_AUTH_FRAME_T)pucBuffer;

    /* 4 <1> Compose the frame header of the Authentication frame. */
    /* Fill the Frame Control field. */
    u2FrameCtrl = MAC_FRAME_AUTH;

    /* If this frame is the third frame in the shared key authentication
     * sequence, it shall be encrypted.
     */
    if ((u2AuthAlgNum == AUTH_ALGORITHM_NUM_SHARED_KEY) &&
        (u2TransactionSeqNum == AUTH_TRANSACTION_SEQ_3)) {
        u2FrameCtrl |= MASK_FC_PROTECTED_FRAME;  /* HW will also detect
                                                    this bit for applying
                                                    encryption */
    }
    /* WLAN_SET_FIELD_16(&prAuthFrame->u2FrameCtrl, u2FrameCtrl); */
    prAuthFrame->u2FrameCtrl = u2FrameCtrl;  /* NOTE(Kevin): Optimized for
                                                ARM */

    /* Fill the DA field with Target BSSID. */
    COPY_MAC_ADDR(prAuthFrame->aucDestAddr, aucPeerMACAddress);

    /* Fill the SA field with our MAC Address. */
    COPY_MAC_ADDR(prAuthFrame->aucSrcAddr, aucMACAddress);

    if (prStaRec != NULL && IS_AP_STA(prStaRec)) {
        /* Fill the BSSID field with Target BSSID. */
        COPY_MAC_ADDR(prAuthFrame->aucBSSID, aucPeerMACAddress);
    } else if (prStaRec != NULL && IS_CLIENT_STA(prStaRec)) {
        /* Fill the BSSID field with Current BSSID. */
        COPY_MAC_ADDR(prAuthFrame->aucBSSID, aucMACAddress);
    } else {
        COPY_MAC_ADDR(prAuthFrame->aucBSSID, aucMACAddress);
        DBGLOG(SAA, INFO, "Error status code flow!\n");
    }

    /* Clear the SEQ/FRAG_NO field. */
    prAuthFrame->u2SeqCtrl = 0;

    /* 4 <2> Compose the frame body's fixed field part of the Authentication
     * frame. */
    /* Fill the Authentication Algorithm Number field. */
    /* WLAN_SET_FIELD_16(&prAuthFrame->u2AuthAlgNum, u2AuthAlgNum); */
    prAuthFrame->u2AuthAlgNum = u2AuthAlgNum;  /* NOTE(Kevin): Optimized for
                                                  ARM */

    if ((prConnSettings->ucAuthDataLen != 0) && !IS_STA_IN_P2P(prStaRec)) {
        kalMemCopy(prAuthFrame->aucAuthData,
                   prConnSettings->aucAuthData,
                   prConnSettings->ucAuthDataLen);
    } else {
        /* Fill the Authentication Transaction Sequence Number field. */
        /* NOTE(Kevin): Optimized for ARM */
        prAuthFrame->aucAuthData[0] = (u8)(u2TransactionSeqNum & 0xff);
        prAuthFrame->aucAuthData[1] =
            (u8)((u2TransactionSeqNum >> 8) & 0xff);

        /* Fill the Status Code field. */
        /* NOTE(Kevin): Optimized for ARM */
        prAuthFrame->aucAuthData[2] = (u8)(u2StatusCode & 0xff);
        prAuthFrame->aucAuthData[3] = (u8)((u2StatusCode >> 8) & 0xff);
    }
    DBGLOG(SAA, INFO, "Compose auth with TransSN = %d,Status = %d\n",
           prAuthFrame->aucAuthData[0], prAuthFrame->aucAuthData[2]);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will append Challenge Text IE to the Authentication
 * frame
 *
 * @param[in] prMsduInfo     Pointer to the composed MSDU_INFO_T.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void authAddIEChallengeText(IN P_ADAPTER_T prAdapter,
                            IN OUT P_MSDU_INFO_T prMsduInfo)
{
    P_WLAN_AUTH_FRAME_T prAuthFrame;
    P_STA_RECORD_T prStaRec;
    u16 u2TransactionSeqNum;

    ASSERT(prMsduInfo);

    prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

    if (!prStaRec) {
        return;
    }

    ASSERT(prStaRec);

    /* For Management, frame header and payload are in a continuous buffer
     */
    prAuthFrame = (P_WLAN_AUTH_FRAME_T)prMsduInfo->prPacket;

    WLAN_GET_FIELD_16(&prAuthFrame->aucAuthData[0], &u2TransactionSeqNum)

    /* Only consider SEQ_3 for Challenge Text */
    if ((u2TransactionSeqNum == AUTH_TRANSACTION_SEQ_3) &&
        (prStaRec->ucAuthAlgNum == AUTH_ALGORITHM_NUM_SHARED_KEY) &&
        (prStaRec->prChallengeText != NULL)) {
        COPY_IE(((unsigned long)(prMsduInfo->prPacket) +
                 prMsduInfo->u2FrameLength),
                (prStaRec->prChallengeText));

        prMsduInfo->u2FrameLength += IE_SIZE(prStaRec->prChallengeText);
    }

    return;
}

#if !CFG_SUPPORT_AAA
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send the Authenticiation frame
 *
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 * @param[in] u2TransactionSeqNum    Transaction Sequence Number
 *
 * @retval WLAN_STATUS_RESOURCES No available resource for frame composing.
 * @retval WLAN_STATUS_SUCCESS   Successfully send frame to TX Module
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS authSendAuthFrame(IN P_ADAPTER_T prAdapter,
                              IN P_STA_RECORD_T prStaRec,
                              IN u16 u2TransactionSeqNum)
{
    P_MSDU_INFO_T prMsduInfo;
    P_BSS_INFO_T prBssInfo;
    u16 u2EstimatedFrameLen;
    u16 u2EstimatedExtraIELen;
    u16 u2PayloadLen;
    u32 i;

    DBGLOG(SAA, LOUD, "Send Auth Frame\n");

    ASSERT(prStaRec);

    /* 4 <1> Allocate a PKT_INFO_T for Authentication Frame */
    /* Init with MGMT Header Length + Length of Fixed Fields */
    u2EstimatedFrameLen =
        (MAC_TX_RESERVED_FIELD + WLAN_MAC_MGMT_HEADER_LEN +
         AUTH_ALGORITHM_NUM_FIELD_LEN +
         AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN +
         STATUS_CODE_FIELD_LEN);

    /* + Extra IE Length */
    u2EstimatedExtraIELen = 0;

    for (i = 0; i < sizeof(txAuthIETable) / sizeof(APPEND_IE_ENTRY_T); i++)
        u2EstimatedExtraIELen += txAuthIETable[i].u2EstimatedIELen;

    u2EstimatedFrameLen += u2EstimatedExtraIELen;

    /* Allocate a MSDU_INFO_T */
    prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
    if (prMsduInfo == NULL) {
        DBGLOG(SAA, WARN, "No PKT_INFO_T for sending Auth Frame.\n");
        return WLAN_STATUS_RESOURCES;
    }
    /* 4 <2> Compose Authentication Request frame header and fixed fields in
     * MSDU_INfO_T. */
    ASSERT(prStaRec->ucBssIndex <= MAX_BSS_INDEX);
    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex)

                /* Compose Header and some Fixed Fields */
                authComposeAuthFrameHeaderAndFF(
        prAdapter, prStaRec,
        (u8 *)((u32)(prMsduInfo->prPacket) +
               MAC_TX_RESERVED_FIELD),
        prStaRec->aucMacAddr, prBssInfo->aucOwnMacAddr,
        prStaRec->ucAuthAlgNum, u2TransactionSeqNum,
        STATUS_CODE_RESERVED);

    u2PayloadLen = (AUTH_ALGORITHM_NUM_FIELD_LEN +
                    AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN +
                    STATUS_CODE_FIELD_LEN);

    /* 4 <3> Update information of MSDU_INFO_T */
    TX_SET_MMPDU(prAdapter, prMsduInfo, prStaRec->ucBssIndex,
                 prStaRec->ucIndex, WLAN_MAC_MGMT_HEADER_LEN,
                 WLAN_MAC_MGMT_HEADER_LEN + u2PayloadLen,
                 saaFsmRunEventTxDone, MSDU_RATE_MODE_AUTO);

    /* 4 <4> Compose IEs in MSDU_INFO_T */
    for (i = 0; i < sizeof(txAuthIETable) / sizeof(APPEND_IE_ENTRY_T);
         i++)
        if (txAuthIETable[i].pfnAppendIE) {
            txAuthIETable[i].pfnAppendIE(prAdapter, prMsduInfo);
        }

    /* TODO(Kevin): Also release the unused tail room of the composed MMPDU
     */

    nicTxConfigPktControlFlag(prMsduInfo, MSDU_CONTROL_FLAG_FORCE_TX, true);

    /* 4 <6> Inform TXM  to send this Authentication frame. */
    nicTxEnqueueMsdu(prAdapter, prMsduInfo);

    return WLAN_STATUS_SUCCESS;
}

#else

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send the Authenticiation frame
 *
 * @param[in] prStaRec               Pointer to the STA_RECORD_T
 * @param[in] u2TransactionSeqNum    Transaction Sequence Number
 *
 * @retval WLAN_STATUS_RESOURCES No available resource for frame composing.
 * @retval WLAN_STATUS_SUCCESS   Successfully send frame to TX Module
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
authSendAuthFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec,
                  IN u8 ucBssIndex, IN P_SW_RFB_T prFalseAuthSwRfb,
                  IN u16 u2TransactionSeqNum, IN u16 u2StatusCode)
{
    u8 *pucReceiveAddr;
    u8 *pucTransmitAddr;
    P_MSDU_INFO_T prMsduInfo;
    P_BSS_INFO_T prBssInfo;
    /*get from input parameter */
    /* ENUM_NETWORK_TYPE_INDEX_T eNetTypeIndex = NETWORK_TYPE_AIS_INDEX; */
    PFN_TX_DONE_HANDLER pfTxDoneHandler = (PFN_TX_DONE_HANDLER)NULL;
    u16 u2EstimatedFrameLen;
    u16 u2EstimatedExtraIELen;
    u16 u2PayloadLen;
    u16 ucAuthAlgNum;
    u32 i;

    P_CONNECTION_SETTINGS_T prConnSettings;

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    DBGLOG(SAA, LOUD, "Send Auth Frame %d, Status Code = %d\n",
           u2TransactionSeqNum, u2StatusCode);

    if (prStaRec && !IS_STA_IN_P2P(prStaRec) &&
        (prConnSettings->ucAuthDataLen != 0)) {
        DBGLOG(SAA, INFO, "prConnSettings->ucAuthDataLen = %d\n",
               prConnSettings->ucAuthDataLen);
        u2EstimatedFrameLen =
            (MAC_TX_RESERVED_FIELD + WLAN_MAC_MGMT_HEADER_LEN +
             AUTH_ALGORITHM_NUM_FIELD_LEN +
             prConnSettings->ucAuthDataLen);
    } else {
        u2EstimatedFrameLen =
            (MAC_TX_RESERVED_FIELD + WLAN_MAC_MGMT_HEADER_LEN +
             AUTH_ALGORITHM_NUM_FIELD_LEN +
             AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN +
             STATUS_CODE_FIELD_LEN);
    }

    /* + Extra IE Length */
    u2EstimatedExtraIELen = 0;

    for (i = 0; i < sizeof(txAuthIETable) / sizeof(APPEND_IE_ENTRY_T); i++)
        u2EstimatedExtraIELen += txAuthIETable[i].u2EstimatedIELen;

    u2EstimatedFrameLen += u2EstimatedExtraIELen;

    /* Allocate a MSDU_INFO_T */
    prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
    if (prMsduInfo == NULL) {
        DBGLOG(SAA, WARN, "No PKT_INFO_T for sending Auth Frame.\n");
        return WLAN_STATUS_RESOURCES;
    }
    /* 4 <2> Compose Authentication Request frame header and fixed fields in
     * MSDU_INfO_T. */
    if (prStaRec) {
        ASSERT(prStaRec->ucBssIndex <= MAX_BSS_INDEX);
        prBssInfo =
            GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

        pucTransmitAddr = prBssInfo->aucOwnMacAddr;

        pucReceiveAddr = prStaRec->aucMacAddr;

        ucAuthAlgNum = prStaRec->ucAuthAlgNum;

        if (!IS_STA_IN_P2P(prStaRec)) {
            if (IS_AP_STA(prStaRec)) {  /* STA mode */
                pfTxDoneHandler = saaFsmRunEventTxDone;
            } else if (IS_CLIENT_STA(prStaRec)) {
                pfTxDoneHandler = aaaFsmRunEventTxDone;
            } else {
                DBGLOG(SAA, WARN,
                       "Can't send auth with unsupport peer's StaType:%d\n",
                       prStaRec->eStaType);
                return WLAN_STATUS_FAILURE;
            }
        } else {
            switch (u2TransactionSeqNum) {
            case AUTH_TRANSACTION_SEQ_1:
            case AUTH_TRANSACTION_SEQ_3:
                pfTxDoneHandler = saaFsmRunEventTxDone;
                break;

            case AUTH_TRANSACTION_SEQ_2:
            case AUTH_TRANSACTION_SEQ_4:
                pfTxDoneHandler = aaaFsmRunEventTxDone;
                break;
            }
        }
    } else {  /* For Error Status Code */
        P_WLAN_AUTH_FRAME_T prFalseAuthFrame;

        ASSERT(prFalseAuthSwRfb);
        prFalseAuthFrame =
            (P_WLAN_AUTH_FRAME_T)prFalseAuthSwRfb->pvHeader;

        ASSERT(u2StatusCode != STATUS_CODE_SUCCESSFUL &&
               (u2StatusCode != WLAN_STATUS_SAE_HASH_TO_ELEMENT));

        pucTransmitAddr = prFalseAuthFrame->aucDestAddr;
        pucReceiveAddr = prFalseAuthFrame->aucSrcAddr;
        ucAuthAlgNum = prFalseAuthFrame->u2AuthAlgNum;

        u2TransactionSeqNum = (prFalseAuthFrame->aucAuthData[1] << 8) +
                              (prFalseAuthFrame->aucAuthData[0] + 1);
    }

    /* Compose Header and some Fixed Fields */
    authComposeAuthFrameHeaderAndFF(
        prAdapter, prStaRec,
        (u8 *)((unsigned long)(prMsduInfo->prPacket) +
               MAC_TX_RESERVED_FIELD),
        pucReceiveAddr, pucTransmitAddr, ucAuthAlgNum,
        u2TransactionSeqNum, u2StatusCode);

    /* fill the length of auth frame body */
    if ((prConnSettings->ucAuthDataLen != 0) && !IS_STA_IN_P2P(prStaRec)) {
        u2PayloadLen = (AUTH_ALGORITHM_NUM_FIELD_LEN +
                        prConnSettings->ucAuthDataLen);
    }else{
        u2PayloadLen = (AUTH_ALGORITHM_NUM_FIELD_LEN +
                        AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN +
                        STATUS_CODE_FIELD_LEN);
    }

    /* 4 <3> Update information of MSDU_INFO_T */
    TX_SET_MMPDU(prAdapter, prMsduInfo, ucBssIndex,
                 (prStaRec != NULL) ? (prStaRec->ucIndex) :
                 (STA_REC_INDEX_NOT_FOUND),
                 WLAN_MAC_MGMT_HEADER_LEN,
                 WLAN_MAC_MGMT_HEADER_LEN + u2PayloadLen, pfTxDoneHandler,
                 MSDU_RATE_MODE_AUTO);

    if ((ucAuthAlgNum == AUTH_ALGORITHM_NUM_SHARED_KEY) &&
        (u2TransactionSeqNum == AUTH_TRANSACTION_SEQ_3)) {
        nicTxConfigPktOption(prMsduInfo, MSDU_OPT_PROTECTED_FRAME,
                             true);
    }
    /* 4 <4> Compose IEs in MSDU_INFO_T */
    for (i = 0; i < sizeof(txAuthIETable) / sizeof(APPEND_IE_ENTRY_T);
         i++)
        if (txAuthIETable[i].pfnAppendIE) {
            txAuthIETable[i].pfnAppendIE(prAdapter, prMsduInfo);
        }

    /* TODO(Kevin): Also release the unused tail room of the composed MMPDU
     */

    nicTxConfigPktControlFlag(prMsduInfo, MSDU_CONTROL_FLAG_FORCE_TX, true);

    /* 4 <6> Inform TXM  to send this Authentication frame. */
    nicTxEnqueueMsdu(prAdapter, prMsduInfo);

    return WLAN_STATUS_SUCCESS;
}

#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will strictly check the TX Authentication frame for
 * SAA/AAA event handling.
 *
 * @param[in] prMsduInfo             Pointer of MSDU_INFO_T
 * @param[in] u2TransactionSeqNum    Transaction Sequence Number
 *
 * @retval WLAN_STATUS_FAILURE   This is not the frame we should handle at
 * current state.
 * @retval WLAN_STATUS_SUCCESS   This is the frame we should handle.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS authCheckTxAuthFrame(IN P_ADAPTER_T prAdapter,
                                 IN P_MSDU_INFO_T prMsduInfo,
                                 IN u16 u2TransactionSeqNum)
{
    P_WLAN_AUTH_FRAME_T prAuthFrame;
    P_STA_RECORD_T prStaRec;
    u16 u2TxFrameCtrl;
    u16 u2TxAuthAlgNum;
    u16 u2TxTransactionSeqNum;

    ASSERT(prMsduInfo);

    prAuthFrame = (P_WLAN_AUTH_FRAME_T)(prMsduInfo->prPacket);
    ASSERT(prAuthFrame);

    prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
    ASSERT(prStaRec);

    if (!prStaRec) {
        return WLAN_STATUS_INVALID_PACKET;
    }

    /* WLAN_GET_FIELD_16(&prAuthFrame->u2FrameCtrl, &u2TxFrameCtrl) */
    u2TxFrameCtrl = prAuthFrame->u2FrameCtrl;  /* NOTE(Kevin): Optimized for
                                                  ARM */
    u2TxFrameCtrl &= MASK_FRAME_TYPE;
    if (u2TxFrameCtrl != MAC_FRAME_AUTH) {
        return WLAN_STATUS_FAILURE;
    }

    /* WLAN_GET_FIELD_16(&prAuthFrame->u2AuthAlgNum, &u2TxAuthAlgNum) */
    u2TxAuthAlgNum = prAuthFrame->u2AuthAlgNum;  /* NOTE(Kevin): Optimized
                                                    for ARM */
    if (u2TxAuthAlgNum != (u16)(prStaRec->ucAuthAlgNum)) {
        return WLAN_STATUS_FAILURE;
    }

    u2TxTransactionSeqNum = (prAuthFrame->aucAuthData[1] << 8) +
                            prAuthFrame->aucAuthData[0];
    if (u2TxTransactionSeqNum != u2TransactionSeqNum) {
        return WLAN_STATUS_FAILURE;
    }

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will check the incoming Auth Frame's Transaction
 * Sequence Number before delivering it to the corresponding SAA or AAA Module.
 *
 * @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Always not retain authentication frames
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS authCheckRxAuthFrameTransSeq(IN P_ADAPTER_T prAdapter,
                                         IN P_SW_RFB_T prSwRfb)
{
    P_WLAN_AUTH_FRAME_T prAuthFrame;
    u16 u2RxTransactionSeqNum;
    u16 u2MinPayloadLen;
#if CFG_SUPPORT_SAE
    P_STA_RECORD_T prStaRec;
    P_BSS_INFO_T prBssInfo = NULL;
#endif

    ASSERT(prSwRfb);

    /* 4 <1> locate the Authentication Frame. */
    prAuthFrame = (P_WLAN_AUTH_FRAME_T)prSwRfb->pvHeader;

    /* 4 <2> Parse the Header of Authentication Frame. */
    u2MinPayloadLen = (AUTH_ALGORITHM_NUM_FIELD_LEN +
                       AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN +
                       STATUS_CODE_FIELD_LEN);
    if ((prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) < u2MinPayloadLen) {
        DBGLOG(SAA, WARN,
               "Rx Auth payload: len[%u] < min expected len[%u]\n",
               (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen),
               u2MinPayloadLen);
        DBGLOG(SAA, WARN, "=== Dump Rx Auth ===\n");
        DBGLOG_MEM8(SAA, WARN, prAuthFrame, prSwRfb->u2PacketLen);
        return WLAN_STATUS_SUCCESS;
    }

    u2RxTransactionSeqNum = (prAuthFrame->aucAuthData[1] << 8) +
                            prAuthFrame->aucAuthData[0];

    if ((u2RxTransactionSeqNum < 0) || (u2RxTransactionSeqNum > 4)) {
        DBGLOG(SAA, WARN,
               "RX auth with unexpected TransactionSeqNum:%d\n",
               u2RxTransactionSeqNum);
        return WLAN_STATUS_SUCCESS;
    }

    if (prAuthFrame->u2AuthAlgNum == AUTH_ALGORITHM_NUM_SAE) {
        if ((u2RxTransactionSeqNum == AUTH_TRANSACTION_SEQ_1) ||
            (u2RxTransactionSeqNum == AUTH_TRANSACTION_SEQ_2)) {
            prStaRec = prSwRfb->prStaRec;
            if (prStaRec) {
                prBssInfo = GET_BSS_INFO_BY_INDEX(
                    prAdapter, prStaRec->ucBssIndex);
            } else {
                prBssInfo = p2pFuncBSSIDFindBssInfo(
                    prAdapter, prAuthFrame->aucBSSID);
            }

            if (prBssInfo == NULL) {
                return WLAN_STATUS_SUCCESS;
            }

            if (prBssInfo->eCurrentOPMode ==
                OP_MODE_INFRASTRUCTURE) {
                saaFsmRunEventRxAuth(prAdapter, prSwRfb);
            } else if (prBssInfo->eCurrentOPMode ==
                       OP_MODE_ACCESS_POINT) {
                aaaFsmRunEventRxAuth(prAdapter, prSwRfb);
            } else {
                DBGLOG(SAA, WARN,
                       "Don't support SAE for non-AIS/P2P network\n");
            }
        } else {
            DBGLOG(SAA, WARN,
                   "RX SAE auth with unexpected TransSeqNum:%d\n",
                   u2RxTransactionSeqNum);
        }
    } else {
        switch (u2RxTransactionSeqNum) {
        case AUTH_TRANSACTION_SEQ_2:
        case AUTH_TRANSACTION_SEQ_4:
            saaFsmRunEventRxAuth(prAdapter, prSwRfb);
            break;
        case AUTH_TRANSACTION_SEQ_1:
        case AUTH_TRANSACTION_SEQ_3:
            aaaFsmRunEventRxAuth(prAdapter, prSwRfb);
            break;
        default:
            DBGLOG(SAA, WARN,
                   "Strange Authentication Packet: Auth Trans Seq No = %d\n",
                   u2RxTransactionSeqNum);
        }
    }
    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will validate the incoming Authentication Frame and take
 *        the status code out.
 *
 * @param[in] prSwRfb                Pointer to SW RFB data structure.
 * @param[in] u2TransactionSeqNum    Transaction Sequence Number
 * @param[out] pu2StatusCode         Pointer to store the Status Code from
 * Authentication.
 *
 * @retval WLAN_STATUS_FAILURE   This is not the frame we should handle at
 * current state.
 * @retval WLAN_STATUS_SUCCESS   This is the frame we should handle.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
authCheckRxAuthFrameStatus(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb,
                           IN u16 u2TransactionSeqNum,
                           OUT u16 *pu2StatusCode)
{
    P_STA_RECORD_T prStaRec;
    P_WLAN_AUTH_FRAME_T prAuthFrame;
    u16 u2RxAuthAlgNum;
    u16 u2RxTransactionSeqNum;
    /* u16 u2RxStatusCode; // NOTE(Kevin): Optimized for ARM */

    ASSERT(prSwRfb);
    ASSERT(pu2StatusCode);

    prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
    ASSERT(prStaRec);

    if (!prStaRec) {
        return WLAN_STATUS_INVALID_PACKET;
    }

    /* 4 <1> locate the Authentication Frame. */
    prAuthFrame = (P_WLAN_AUTH_FRAME_T)prSwRfb->pvHeader;

    /* 4 <2> Parse the Fixed Fields of Authentication Frame Body. */
    /* WLAN_GET_FIELD_16(&prAuthFrame->u2AuthAlgNum, &u2RxAuthAlgNum); */
    u2RxAuthAlgNum = prAuthFrame->u2AuthAlgNum;  /* NOTE(Kevin): Optimized
                                                    for ARM */
    if (u2RxAuthAlgNum != (u16)prStaRec->ucAuthAlgNum) {
        DBGLOG(SAA, LOUD,
               "Discard Auth frame with auth type = %d, current = %d\n",
               u2RxAuthAlgNum, prStaRec->ucAuthAlgNum);
        return WLAN_STATUS_FAILURE;
    }

    u2RxTransactionSeqNum = (prAuthFrame->aucAuthData[1] << 8) +
                            prAuthFrame->aucAuthData[0];
    /* Still report to upper layer to let it do the error handling */
    if (u2RxTransactionSeqNum < u2TransactionSeqNum) {
        DBGLOG(SAA, WARN,
               "Rx Auth frame with unexpected Transaction Seq No = %d\n",
               u2RxTransactionSeqNum);
    }

    *pu2StatusCode = (prAuthFrame->aucAuthData[3] << 8) +
                     prAuthFrame->aucAuthData[2];

    DBGLOG(SAA, INFO,
           "Rx Auth frame with auth type = %d, SN = %d, Status Code = %d\n",
           u2RxAuthAlgNum, u2RxTransactionSeqNum, *pu2StatusCode);

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will handle the Challenge Text IE from the
 * Authentication frame
 *
 * @param[in] prSwRfb                Pointer to SW RFB data structure.
 * @param[in] prIEHdr                Pointer to start address of IE
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void authHandleIEChallengeText(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb,
                               P_IE_HDR_T prIEHdr)
{
    P_WLAN_AUTH_FRAME_T prAuthFrame;
    P_STA_RECORD_T prStaRec;
    u16 u2TransactionSeqNum;

    ASSERT(prSwRfb);
    ASSERT(prIEHdr);

    prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
    ASSERT(prStaRec);

    if (!prStaRec) {
        return;
    }

    /* For Management, frame header and payload are in a continuous buffer
     */
    prAuthFrame = (P_WLAN_AUTH_FRAME_T)prSwRfb->pvHeader;

    u2TransactionSeqNum = (prAuthFrame->aucAuthData[1] << 8) +
                          prAuthFrame->aucAuthData[0];

    /* Only consider SEQ_2 for Challenge Text */
    if ((u2TransactionSeqNum == AUTH_TRANSACTION_SEQ_2) &&
        (prStaRec->ucAuthAlgNum == AUTH_ALGORITHM_NUM_SHARED_KEY)) {
        /* Free previous allocated TCM memory */
        if (prStaRec->prChallengeText) {
            /* ASSERT(0); */
            cnmMemFree(prAdapter, prStaRec->prChallengeText);
            prStaRec->prChallengeText = (P_IE_CHALLENGE_TEXT_T)NULL;
        }
        prStaRec->prChallengeText =
            cnmMemAlloc(prAdapter, RAM_TYPE_MSG, IE_SIZE(prIEHdr));
        if (prStaRec->prChallengeText == NULL) {
            return;
        }

        /* Save the Challenge Text from Auth Seq 2 Frame, before sending
         * Auth Seq 3 Frame */
        COPY_IE(prStaRec->prChallengeText, prIEHdr);
    }

    return;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will parse and process the incoming Authentication
 * frame.
 *
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 *
 * @retval WLAN_STATUS_SUCCESS   This is the frame we should handle.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS authProcessRxAuth2_Auth4Frame(IN P_ADAPTER_T prAdapter,
                                          IN P_SW_RFB_T prSwRfb)
{
    P_WLAN_AUTH_FRAME_T prAuthFrame;
    u8 *pucIEsBuffer;
    u16 u2IEsLen;
    u16 u2Offset;
    u8 ucIEID;
    u32 i;

    ASSERT(prSwRfb);

    prAuthFrame = (P_WLAN_AUTH_FRAME_T)prSwRfb->pvHeader;
    pucIEsBuffer = (u8 *)&prAuthFrame->aucAuthData[0] + 4;

    u2IEsLen = (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
               (AUTH_ALGORITHM_NUM_FIELD_LEN +
                AUTH_TRANSACTION_SEQENCE_NUM_FIELD_LEN +
                STATUS_CODE_FIELD_LEN);

    IE_FOR_EACH(pucIEsBuffer, u2IEsLen, u2Offset){
        ucIEID = IE_ID(pucIEsBuffer);

        for (i = 0;
             i < (sizeof(rxAuthIETable) / sizeof(HANDLE_IE_ENTRY_T));
             i++) {
            if ((ucIEID == rxAuthIETable[i].ucElemID) &&
                (rxAuthIETable[i].pfnHandleIE != NULL)) {
                rxAuthIETable[i].pfnHandleIE(
                    prAdapter, prSwRfb,
                    (P_IE_HDR_T)pucIEsBuffer);
            }
        }
    }
    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will compose the Deauthentication frame
 *
 * @param[in] pucBuffer              Pointer to the frame buffer.
 * @param[in] aucPeerMACAddress      Given Peer MAC Address.
 * @param[in] aucMACAddress          Given Our MAC Address.
 * @param[in] u2StatusCode           Status Code
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void
authComposeDeauthFrameHeaderAndFF(IN u8 *pucBuffer,
                                  IN u8 aucPeerMACAddress[],
                                  IN u8 aucMACAddress[], IN u8 aucBssid[],
                                  IN u16 u2ReasonCode)
{
    P_WLAN_DEAUTH_FRAME_T prDeauthFrame;
    u16 u2FrameCtrl;

    ASSERT(pucBuffer);
    ASSERT(aucPeerMACAddress);
    ASSERT(aucMACAddress);
    ASSERT(aucBssid);

    prDeauthFrame = (P_WLAN_DEAUTH_FRAME_T)pucBuffer;

    /* 4 <1> Compose the frame header of the Deauthentication frame. */
    /* Fill the Frame Control field. */
    u2FrameCtrl = MAC_FRAME_DEAUTH;

    /* WLAN_SET_FIELD_16(&prDeauthFrame->u2FrameCtrl, u2FrameCtrl); */
    prDeauthFrame->u2FrameCtrl = u2FrameCtrl;  /* NOTE(Kevin): Optimized for
                                                  ARM */

    /* Fill the DA field with Target BSSID. */
    COPY_MAC_ADDR(prDeauthFrame->aucDestAddr, aucPeerMACAddress);

    /* Fill the SA field with our MAC Address. */
    COPY_MAC_ADDR(prDeauthFrame->aucSrcAddr, aucMACAddress);

    /* Fill the BSSID field with Target BSSID. */
    COPY_MAC_ADDR(prDeauthFrame->aucBSSID, aucBssid);

    /* Clear the SEQ/FRAG_NO field(HW won't overide the FRAG_NO, so we need
     * to clear it). */
    prDeauthFrame->u2SeqCtrl = 0;

    /* 4 <2> Compose the frame body's fixed field part of the Authentication
     * frame. */
    /* Fill the Status Code field. */
    /* WLAN_SET_FIELD_16(&prDeauthFrame->u2ReasonCode, u2ReasonCode); */
    prDeauthFrame->u2ReasonCode = u2ReasonCode;  /* NOTE(Kevin): Optimized
                                                    for ARM */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will send the Deauthenticiation frame
 *
 * @param[in] prStaRec           Pointer to the STA_RECORD_T
 * @param[in] prClassErrSwRfb    Pointer to the SW_RFB_T which is Class Error.
 * @param[in] u2ReasonCode       A reason code to indicate why to leave BSS.
 * @param[in] pfTxDoneHandler    TX Done call back function
 *
 * @retval WLAN_STATUS_RESOURCES No available resource for frame composing.
 * @retval WLAN_STATUS_SUCCESS   Successfully send frame to TX Module
 * @retval WLAN_STATUS_FAILURE   Didn't send Deauth frame for various reasons.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
authSendDeauthFrame(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo,
                    IN P_STA_RECORD_T prStaRec,
                    IN P_SW_RFB_T prClassErrSwRfb,
                    IN u16 u2ReasonCode,
                    IN PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
    u8 *pucReceiveAddr;
    u8 *pucTransmitAddr;
    u8 *pucBssid = NULL;
    P_MSDU_INFO_T prMsduInfo;
    u16 u2EstimatedFrameLen;
    u8 ucRoleIdx = 0;
    u8 fgIsInterruptContext = false;
    u8 *pFrameBuf;
    P_DEAUTH_INFO_T prDeauthInfo;
    u32 rCurrentTime;
    s32 i4NewEntryIndex, i;
    u8 ucStaRecIdx = STA_REC_INDEX_NOT_FOUND;
    u8 ucBssIndex = BSS_INFO_NUM;
    u8 aucBMC[] = BC_MAC_ADDR;

    DBGLOG(RSN, INFO, "authSendDeauthFrame\n");

    /* NOTE(Kevin): The best way to reply the Deauth is according to the
     * incoming data frame
     */
    /* 4 <1.1> Find the Receiver Address */
    if (prClassErrSwRfb) {
        u8 fgIsAbleToSendDeauth = false;
        u16 u2RxFrameCtrl;
        P_WLAN_MAC_HEADER_A4_T prWlanMacHeader = NULL;

        prWlanMacHeader =
            (P_WLAN_MAC_HEADER_A4_T)prClassErrSwRfb->pvHeader;

        /* WLAN_GET_FIELD_16(&prWlanMacHeader->u2FrameCtrl,
         * &u2RxFrameCtrl); */
        u2RxFrameCtrl = prWlanMacHeader->u2FrameCtrl;  /* NOTE(Kevin):
                                                          Optimized for
                                                          ARM */

        /* TODO(Kevin): Currently we won't send Deauth for IBSS node.
         * How about DLS ? */
        if ((prWlanMacHeader->u2FrameCtrl & MASK_TO_DS_FROM_DS) == 0) {
            return WLAN_STATUS_FAILURE;
        }

        /* Check if corresponding BSS is able to send Deauth */
        for (i = 0; i < BSS_INFO_NUM; i++) {
            prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);

            if (IS_NET_ACTIVE(prAdapter, i) &&
                (EQUAL_MAC_ADDR(prWlanMacHeader->aucAddr1,
                                prBssInfo->aucOwnMacAddr))) {
                fgIsAbleToSendDeauth = true;
                ucBssIndex = (u8)i;
                break;
            }
        }

        if (!fgIsAbleToSendDeauth) {
            return WLAN_STATUS_FAILURE;
        }

        pucReceiveAddr = prWlanMacHeader->aucAddr2;
    } else if (prStaRec) {
        prBssInfo =
            GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
        ucStaRecIdx = prStaRec->ucIndex;
        ucBssIndex = prBssInfo->ucBssIndex;

        pucReceiveAddr = prStaRec->aucMacAddr;
    } else if (prBssInfo) {
        ucBssIndex = prBssInfo->ucBssIndex;
        ucStaRecIdx = STA_REC_INDEX_BMCAST;

        pucReceiveAddr = aucBMC;
    } else {
        DBGLOG(SAA, WARN, "Not to send Deauth, invalid data!\n");
        return WLAN_STATUS_INVALID_DATA;
    }

    /* 4 <1.2> Find Transmitter Address and BSSID. */
    pucTransmitAddr = prBssInfo->aucOwnMacAddr;
    pucBssid = prBssInfo->aucBSSID;

    if (ucStaRecIdx != STA_REC_INDEX_BMCAST) {
        /* 4 <2> Check if already send a Deauth frame in
         * MIN_DEAUTH_INTERVAL_MSEC */
        GET_CURRENT_SYSTIME(&rCurrentTime);

        i4NewEntryIndex = -1;
        for (i = 0; i < MAX_DEAUTH_INFO_COUNT; i++) {
            prDeauthInfo = &(prAdapter->rWifiVar.arDeauthInfo[i]);

            /* For continuously sending Deauth frame, the minimum
             * interval is MIN_DEAUTH_INTERVAL_MSEC.
             */
            if (CHECK_FOR_TIMEOUT(
                    rCurrentTime, prDeauthInfo->rLastSendTime,
                    MSEC_TO_SYSTIME(MIN_DEAUTH_INTERVAL_MSEC))) {
                i4NewEntryIndex = i;
            } else if (EQUAL_MAC_ADDR(pucReceiveAddr,
                                      prDeauthInfo->aucRxAddr) &&
                       (!pfTxDoneHandler)) {
                return WLAN_STATUS_FAILURE;
            }
        }

        /* 4 <3> Update information. */
        if (i4NewEntryIndex > 0) {
            prDeauthInfo =
                &(prAdapter->rWifiVar
                  .arDeauthInfo[i4NewEntryIndex]);

            COPY_MAC_ADDR(prDeauthInfo->aucRxAddr, pucReceiveAddr);
            prDeauthInfo->rLastSendTime = rCurrentTime;
        } else {
            /* NOTE(Kevin): for the case of AP mode, we may
             * encounter this case if deauth all the associated
             * clients.
             */
            DBGLOG(SAA, WARN, "No unused DEAUTH_INFO_T !\n");
        }
    }
    /* 4 <5> Allocate a PKT_INFO_T for Deauthentication Frame */
    /* Init with MGMT Header Length + Length of Fixed Fields + IE Length */
    u2EstimatedFrameLen =
        (MAC_TX_RESERVED_FIELD + WLAN_MAC_MGMT_HEADER_LEN +
         REASON_CODE_FIELD_LEN);

    /* Allocate a MSDU_INFO_T */
    prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
    if (prMsduInfo == NULL) {
        DBGLOG(SAA, WARN,
               "No PKT_INFO_T for sending Deauth Request.\n");
        return WLAN_STATUS_RESOURCES;
    }
    /* 4 <6> compose Deauthentication frame header and some fixed fields */
    authComposeDeauthFrameHeaderAndFF(
        (u8 *)((unsigned long)(prMsduInfo->prPacket) +
               MAC_TX_RESERVED_FIELD),
        pucReceiveAddr, pucTransmitAddr, pucBssid, u2ReasonCode);

#if CFG_SUPPORT_802_11W
    /* AP PMF */
    if (rsnCheckBipKeyInstalled(prAdapter, prStaRec)) {
        /* PMF certification 4.3.3.1, 4.3.3.2 send unprotected deauth
         * reason 6/7 */
        /* if (AP mode & not for PMF reply case) OR (STA PMF) */
        if (((GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex)
              ->eCurrentOPMode == OP_MODE_ACCESS_POINT) &&
             (prStaRec->rPmfCfg.fgRxDeauthResp != true)) ||
            (GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex)
             ->eNetworkType == (u8)NETWORK_TYPE_AIS)) {
            P_WLAN_DEAUTH_FRAME_T prDeauthFrame;

            prDeauthFrame =
                (P_WLAN_DEAUTH_FRAME_T)(u8 *)((unsigned long)(prMsduInfo
                                                              ->prPacket) +
                                              MAC_TX_RESERVED_FIELD);

            prDeauthFrame->u2FrameCtrl |= MASK_FC_PROTECTED_FRAME;
        }
    }
#endif
    nicTxSetPktLifeTime(prMsduInfo, 100);

    if (pfTxDoneHandler) {
        nicTxSetPktRetryLimit(prMsduInfo, TX_DESC_TX_COUNT_NO_LIMIT);
    }else{
        nicTxSetPktRetryLimit(prMsduInfo, 5);
    }

    /* 4 <7> Update information of MSDU_INFO_T */
    TX_SET_MMPDU(prAdapter, prMsduInfo, ucBssIndex, ucStaRecIdx,
                 WLAN_MAC_MGMT_HEADER_LEN,
                 WLAN_MAC_MGMT_HEADER_LEN + REASON_CODE_FIELD_LEN,
                 pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

    /* AP PMF */
    /* caution: access prStaRec only if true */
    if (rsnCheckBipKeyInstalled(prAdapter, prStaRec)) {
        /* 4.3.3.1 send unprotected deauth reason 6/7 */
        if (prStaRec->rPmfCfg.fgRxDeauthResp != true) {
            DBGLOG(RSN, INFO,
                   "Deauth Set MSDU_OPT_PROTECTED_FRAME\n");
            nicTxConfigPktOption(prMsduInfo,
                                 MSDU_OPT_PROTECTED_FRAME, true);
        }

        prStaRec->rPmfCfg.fgRxDeauthResp = false;
    }

    {
        P_WLAN_DEAUTH_FRAME_T prDeauthFrame;
        prDeauthFrame =
            (P_WLAN_DEAUTH_FRAME_T)(u8 *)((unsigned long)(prMsduInfo
                                                          ->prPacket) +
                                          MAC_TX_RESERVED_FIELD);
        DBGLOG(SAA, INFO, "notification of TX deauthentication, %d\n",
               prMsduInfo->u2FrameLength);

        /*
         * check prStaRec is not NULL first
         * prStaRec can be NULL if it is P2P GO
         */
        if ((prStaRec) && (IS_STA_IN_AIS(prStaRec))) {
            if (in_interrupt()) {
                pFrameBuf =
                    kalMemAlloc(prMsduInfo->u2FrameLength,
                                PHY_MEM_TYPE);
                fgIsInterruptContext = true;
            } else {
                pFrameBuf =
                    kalMemAlloc(prMsduInfo->u2FrameLength,
                                VIR_MEM_TYPE);
                fgIsInterruptContext = false;
            }

            if (!pFrameBuf) {
                DBGLOG(SAA, ERROR,
                       "Alloc buffer for frame failed\n");
                return WLAN_STATUS_RESOURCES;
            }

            kalMemCopy((void *)pFrameBuf, (void *)prDeauthFrame,
                       prMsduInfo->u2FrameLength);

            kalWDevLockThread(prAdapter->prGlueInfo,
                              prAdapter->prGlueInfo->prDevHandler,
                              CFG80211_TX_MLME_MGMT, pFrameBuf,
                              prMsduInfo->u2FrameLength, NULL, 0,
                              NULL, 0, fgIsInterruptContext);
        } else if (prAdapter->fgIsP2PRegistered) {
            ucRoleIdx = (u8)prBssInfo->u4PrivateData;
            if (in_interrupt()) {
                pFrameBuf =
                    kalMemAlloc(prMsduInfo->u2FrameLength,
                                PHY_MEM_TYPE);
                fgIsInterruptContext = true;
            } else {
                pFrameBuf =
                    kalMemAlloc(prMsduInfo->u2FrameLength,
                                VIR_MEM_TYPE);
                fgIsInterruptContext = false;
            }

            if (!pFrameBuf) {
                DBGLOG(SAA, ERROR,
                       "Alloc buffer for frame failed\n");
                return WLAN_STATUS_RESOURCES;
            }

            kalMemCopy((void *)pFrameBuf, (void *)prDeauthFrame,
                       prMsduInfo->u2FrameLength);
            kalWDevLockThread(prAdapter->prGlueInfo,
                              prAdapter->prGlueInfo
                              ->prP2PInfo[ucRoleIdx]
                              ->aprRoleHandler,
                              CFG80211_TX_MLME_MGMT, pFrameBuf,
                              prMsduInfo->u2FrameLength, NULL, 0,
                              NULL, 0, fgIsInterruptContext);
        } else {
            DBGLOG(SAA, INFO,
                   "notification of TX deauthentication, FAILED\n");
        }
        DBGLOG(SAA, INFO,
               "notification of TX deauthentication, Done\n");
    }

    nicTxConfigPktControlFlag(prMsduInfo, MSDU_CONTROL_FLAG_FORCE_TX, true);

    /* 4 <8> Inform TXM to send this Deauthentication frame. */
    nicTxEnqueueMsdu(prAdapter, prMsduInfo);

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will parse and process the incoming Deauthentication
 * frame if the given BSSID is matched.
 *
 * @param[in] prSwRfb            Pointer to SW RFB data structure.
 * @param[in] aucBSSID           Given BSSID
 * @param[out] pu2ReasonCode     Pointer to store the Reason Code from
 * Deauthentication.
 *
 * @retval WLAN_STATUS_FAILURE   This is not the frame we should handle at
 * current state.
 * @retval WLAN_STATUS_SUCCESS   This is the frame we should handle.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS authProcessRxDeauthFrame(IN P_SW_RFB_T prSwRfb,
                                     IN u8 aucBSSID[],
                                     OUT u16 *pu2ReasonCode)
{
    P_WLAN_DEAUTH_FRAME_T prDeauthFrame;
    u16 u2RxReasonCode;

    if (!prSwRfb || !aucBSSID || !pu2ReasonCode) {
        DBGLOG(SAA, WARN, "Invalid parameters, ignore this pkt!\n");
        return WLAN_STATUS_FAILURE;
    }

    /* 4 <1> locate the Deauthentication Frame. */
    prDeauthFrame = (P_WLAN_DEAUTH_FRAME_T)prSwRfb->pvHeader;

    if ((prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) <
        REASON_CODE_FIELD_LEN) {
        DBGLOG(SAA, WARN,
               "Invalid Deauth packet length. u2PacketLen(%u), u2HeaderLen(%u)\n",
               prSwRfb->u2PacketLen, prSwRfb->u2HeaderLen);
        return WLAN_STATUS_FAILURE;
    }

    /* Check if this Deauth Frame is coming from Target BSSID */
    if (UNEQUAL_MAC_ADDR(prDeauthFrame->aucBSSID, aucBSSID)) {
        DBGLOG(SAA, LOUD,
               "Ignore Deauth Frame from other BSS [" MACSTR "]\n",
               MAC2STR(prDeauthFrame->aucSrcAddr));
        return WLAN_STATUS_FAILURE;
    }
    /* 4 <3> Parse the Fixed Fields of Deauthentication Frame Body. */
    WLAN_GET_FIELD_16(&prDeauthFrame->u2ReasonCode, &u2RxReasonCode);
    *pu2ReasonCode = u2RxReasonCode;

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will parse and process the incoming Authentication
 * frame.
 *
 * @param[in] prSwRfb                Pointer to SW RFB data structure.
 * @param[in] aucExpectedBSSID       Given Expected BSSID.
 * @param[in] u2ExpectedAuthAlgNum   Given Expected Authentication Algorithm
 * Number
 * @param[in] u2ExpectedTransSeqNum  Given Expected Transaction Sequence Number.
 * @param[out] pu2ReturnStatusCode   Return Status Code.
 *
 * @retval WLAN_STATUS_SUCCESS   This is the frame we should handle.
 * @retval WLAN_STATUS_FAILURE   The frame we will ignore.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
authProcessRxAuth1Frame(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb,
                        IN u8 aucExpectedBSSID[],
                        IN u16 u2ExpectedAuthAlgNum,
                        IN u16 u2ExpectedTransSeqNum,
                        OUT u16 *pu2ReturnStatusCode)
{
    P_WLAN_AUTH_FRAME_T prAuthFrame;
    u16 u2RxStatusCode;
    u16 u2ReturnStatusCode = STATUS_CODE_SUCCESSFUL;

    ASSERT(prSwRfb);
    ASSERT(aucExpectedBSSID);
    ASSERT(pu2ReturnStatusCode);

    /* 4 <1> locate the Authentication Frame. */
    prAuthFrame = (P_WLAN_AUTH_FRAME_T)prSwRfb->pvHeader;

    /* 4 <2> Check the BSSID */
    if (UNEQUAL_MAC_ADDR(prAuthFrame->aucBSSID, aucExpectedBSSID)) {
        return WLAN_STATUS_FAILURE;  /* Just Ignore this MMPDU */

    }
    /* 4 <3> Check the SA, which should not be MC/BC */
    if (prAuthFrame->aucSrcAddr[0] & BIT(0)) {
        DBGLOG(P2P, WARN,
               "Invalid STA MAC with MC/BC bit set: " MACSTR "\n",
               MAC2STR(prAuthFrame->aucSrcAddr));
        return WLAN_STATUS_FAILURE;
    }

    /* 4 <4> Parse the Fixed Fields of Authentication Frame Body. */
    u2RxStatusCode = (prAuthFrame->aucAuthData[3] << 8) +
                     prAuthFrame->aucAuthData[2];

    if (u2RxStatusCode != STATUS_CODE_RESERVED) {
        DBGLOG(AAA, LOUD, "Invalid Status code %d\n", u2RxStatusCode);
        return WLAN_STATUS_FAILURE;
    }

    if (prAuthFrame->u2AuthAlgNum != u2ExpectedAuthAlgNum) {
        u2ReturnStatusCode = STATUS_CODE_AUTH_ALGORITHM_NOT_SUPPORTED;
    }

    if (prAuthFrame->aucAuthData[0] != u2ExpectedTransSeqNum) {
        u2ReturnStatusCode = STATUS_CODE_AUTH_OUT_OF_SEQ;
    }

    *pu2ReturnStatusCode = u2ReturnStatusCode;
    return WLAN_STATUS_SUCCESS;
}
