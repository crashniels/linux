// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   nic_rx.c
 *    \brief  Functions that provide many rx-related functions
 *
 *    This file includes the functions used to process RFB and dispatch RFBs to
 *    the appropriate related rx functions for protocols.
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
#include "que_mgt.h"

#include <linux/limits.h>

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

#if CFG_MGMT_FRAME_HANDLING
static PROCESS_RX_MGT_FUNCTION apfnProcessRxMgtFrame[MAX_NUM_OF_FC_SUBTYPES] = {
#if CFG_SUPPORT_AAA
    aaaFsmRunEventRxAssoc,  /* subtype 0000: Association request */
#else
    NULL,  /* subtype 0000: Association request */
#endif
    saaFsmRunEventRxAssoc,  /* subtype 0001: Association response */
#if CFG_SUPPORT_AAA
    aaaFsmRunEventRxAssoc,  /* subtype 0010: Reassociation request */
#else
    NULL,  /* subtype 0010: Reassociation request */
#endif
    saaFsmRunEventRxAssoc,  /* subtype 0011: Reassociation response */
#if CFG_SUPPORT_ADHOC || CFG_ENABLE_WIFI_DIRECT
    bssProcessProbeRequest,  /* subtype 0100: Probe request */
#else
    NULL,  /* subtype 0100: Probe request */
#endif
    scanProcessBeaconAndProbeResp,  /* subtype 0101: Probe response */
    NULL,  /* subtype 0110: reserved */
    NULL,  /* subtype 0111: reserved */
    scanProcessBeaconAndProbeResp,  /* subtype 1000: Beacon */
    NULL,  /* subtype 1001: ATIM */
    saaFsmRunEventRxDisassoc,  /* subtype 1010: Disassociation */
    authCheckRxAuthFrameTransSeq,  /* subtype 1011: Authentication */
    saaFsmRunEventRxDeauth,  /* subtype 1100: Deauthentication */
    nicRxProcessActionFrame,  /* subtype 1101: Action */
    NULL,  /* subtype 1110: reserved */
    NULL  /* subtype 1111: reserved */
};
#endif

static RX_EVENT_HANDLER_T arEventTable[] = {
    { EVENT_ID_RX_ADDBA, qmHandleEventRxAddBa },
    { EVENT_ID_RX_DELBA, qmHandleEventRxDelBa },
    { EVENT_ID_CHECK_REORDER_BUBBLE, qmHandleEventCheckReorderBubble },
    { EVENT_ID_LINK_QUALITY, nicEventLinkQuality },
    { EVENT_ID_LAYER_0_EXT_MAGIC_NUM, nicEventLayer0ExtMagic },
    { EVENT_ID_MIC_ERR_INFO, nicEventMicErrorInfo },
    { EVENT_ID_SCAN_DONE, nicEventScanDone },
    { EVENT_ID_NLO_DONE, nicEventNloDone },
    { EVENT_ID_TX_DONE, nicTxProcessTxDoneEvent },
    { EVENT_ID_SLEEPY_INFO, nicEventSleepyNotify },
    { EVENT_ID_STATISTICS, nicEventStatistics },
    { EVENT_ID_WLAN_INFO, nicEventWlanInfo },
    { EVENT_ID_MIB_INFO, nicEventMibInfo },
#if CFG_SUPPORT_LAST_SEC_MCS_INFO
    { EVENT_ID_TX_MCS_INFO, nicEventTxMcsInfo },
#endif
    { EVENT_ID_CH_PRIVILEGE, cnmChMngrHandleChEvent },
    { EVENT_ID_BSS_ABSENCE_PRESENCE, qmHandleEventBssAbsencePresence },
    { EVENT_ID_STA_CHANGE_PS_MODE, qmHandleEventStaChangePsMode },
    { EVENT_ID_STA_UPDATE_FREE_QUOTA, qmHandleEventStaUpdateFreeQuota },
    { EVENT_ID_BSS_BEACON_TIMEOUT, nicEventBeaconTimeout },
    { EVENT_ID_UPDATE_NOA_PARAMS, nicEventUpdateNoaParams },
    { EVENT_ID_STA_AGING_TIMEOUT, nicEventStaAgingTimeout },
    { EVENT_ID_AP_OBSS_STATUS, nicEventApObssStatus },
    { EVENT_ID_ROAMING_STATUS, nicEventRoamingStatus },
    { EVENT_ID_SEND_DEAUTH, nicEventSendDeauth },
    { EVENT_ID_UPDATE_RDD_STATUS, nicEventUpdateRddStatus },
    { EVENT_ID_UPDATE_BWCS_STATUS, nicEventUpdateBwcsStatus },
    { EVENT_ID_UPDATE_BCM_DEBUG, nicEventUpdateBcmDebug },
    { EVENT_ID_ADD_PKEY_DONE, nicEventAddPkeyDone },
    { EVENT_ID_ICAP_DONE, nicEventIcapDone },
    { EVENT_ID_DEBUG_MSG, nicEventDebugMsg },
    { EVENT_ID_TDLS, nicEventTdls },
    { EVENT_ID_DUMP_MEM, nicEventDumpMem },
#if CFG_ASSERT_DUMP
    { EVENT_ID_ASSERT_DUMP, nicEventAssertDump },
#endif
    { EVENT_ID_RDD_SEND_PULSE, nicEventRddSendPulse },
#if (CFG_SUPPORT_DFS_MASTER == 1)
    { EVENT_ID_UPDATE_COEX_PHYRATE, nicEventUpdateCoexPhyrate },
    { EVENT_ID_RDD_REPORT, cnmRadarDetectEvent },
    { EVENT_ID_CSA_DONE, cnmCsaDoneEvent },
#else
    { EVENT_ID_UPDATE_COEX_PHYRATE, nicEventUpdateCoexPhyrate },
#endif
#if (CFG_WOW_SUPPORT == 1)
    { EVENT_ID_WOW_WAKEUP_REASON, nicEventWakeUpReason },
#endif
    { EVENT_ID_CSI_DATA, nicEventCSIData },

#if CFG_SUPPORT_REPLAY_DETECTION
    { EVENT_ID_GET_GTK_REKEY_DATA, nicEventGetGtkDataSync },
#endif
};

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
 * @brief Initialize the RFBs
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxInitialize(IN P_ADAPTER_T prAdapter)
{
    P_RX_CTRL_T prRxCtrl;
    u8 *pucMemHandle;
    P_SW_RFB_T prSwRfb = (P_SW_RFB_T)NULL;
    u32 i;

    DEBUGFUNC("nicRxInitialize");

    ASSERT(prAdapter);
    prRxCtrl = &prAdapter->rRxCtrl;

    /* 4 <0> Clear allocated memory. */
    kalMemZero((void *)prRxCtrl->pucRxCached, prRxCtrl->u4RxCachedSize);

    /* 4 <1> Initialize the RFB lists */
    QUEUE_INITIALIZE(&prRxCtrl->rFreeSwRfbList);
    QUEUE_INITIALIZE(&prRxCtrl->rReceivedRfbList);
    QUEUE_INITIALIZE(&prRxCtrl->rIndicatedRfbList);

    pucMemHandle = prRxCtrl->pucRxCached;
    for (i = CFG_RX_MAX_PKT_NUM; i != 0; i--) {
        prSwRfb = (P_SW_RFB_T)pucMemHandle;

        if (nicRxSetupRFB(prAdapter, prSwRfb)) {
            DBGLOG(RX,
                   ERROR,
                   "nicRxInitialize failed: Cannot allocate packet buffer for SwRfb!\n");
            return;
        }
        nicRxReturnRFB(prAdapter, prSwRfb);

        pucMemHandle += ALIGN_4(sizeof(SW_RFB_T));
    }

    ASSERT(prRxCtrl->rFreeSwRfbList.u4NumElem == CFG_RX_MAX_PKT_NUM);
    /* Check if the memory allocation consist with this initialization
     * function */
    ASSERT((u32)(pucMemHandle - prRxCtrl->pucRxCached) ==
           prRxCtrl->u4RxCachedSize);

    /* 4 <2> Clear all RX counters */
    RX_RESET_ALL_CNTS(prRxCtrl);

    prRxCtrl->pucRxCoalescingBufPtr = prAdapter->pucCoalescingBufCached;

#if CFG_HIF_STATISTICS
    prRxCtrl->u4TotalRxAccessNum = 0;
    prRxCtrl->u4TotalRxPacketNum = 0;
#endif

#if CFG_HIF_RX_STARVATION_WARNING
    prRxCtrl->u4QueuedCnt = 0;
    prRxCtrl->u4DequeuedCnt = 0;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Uninitialize the RFBs
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxUninitialize(IN P_ADAPTER_T prAdapter)
{
    P_RX_CTRL_T prRxCtrl;
    P_SW_RFB_T prSwRfb = (P_SW_RFB_T)NULL;

    KAL_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);
    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    nicRxFlush(prAdapter);

    do {
        KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
        QUEUE_REMOVE_HEAD(&prRxCtrl->rReceivedRfbList, prSwRfb,
                          P_SW_RFB_T);
        KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
        if (prSwRfb) {
            if (prSwRfb->pvPacket) {
                kalPacketFree(prAdapter->prGlueInfo,
                              prSwRfb->pvPacket);
            }
            prSwRfb->pvPacket = NULL;
        } else {
            break;
        }
    } while (true);

    do {
        KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
        QUEUE_REMOVE_HEAD(&prRxCtrl->rFreeSwRfbList, prSwRfb,
                          P_SW_RFB_T);
        KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
        if (prSwRfb) {
            if (prSwRfb->pvPacket) {
                kalPacketFree(prAdapter->prGlueInfo,
                              prSwRfb->pvPacket);
            }
            prSwRfb->pvPacket = NULL;
        } else {
            break;
        }
    } while (true);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Fill RFB
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb   specify the RFB to receive rx data
 *
 * @return true: no logic check error, false: logic check error
 *
 */
/*----------------------------------------------------------------------------*/
u8 nicRxFillRFB(IN P_ADAPTER_T prAdapter, IN OUT P_SW_RFB_T prSwRfb)
{
    P_HW_MAC_RX_DESC_T prRxStatus;

    u32 u4PktLen = 0;
    /* u32 u4MacHeaderLen; */
    u32 u4HeaderOffset;
    u16 u2RxStatusOffset;

    DEBUGFUNC("nicRxFillRFB");

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prRxStatus = prSwRfb->prRxStatus;
    ASSERT(prRxStatus);

    u4PktLen = (u32)HAL_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus);
    if (u4PktLen < sizeof(HW_MAC_RX_DESC_T)) {
        DBGLOG(RX, ERROR, "Logic Check Error - u4PktLen(%lu) < RXD.\n",
               u4PktLen);
        return false;
    }

    u4HeaderOffset = (u32)(HAL_RX_STATUS_GET_HEADER_OFFSET(prRxStatus));
    /* u4MacHeaderLen = (u32)(HAL_RX_STATUS_GET_HEADER_LEN(prRxStatus)); */

    /* DBGLOG(RX, TRACE, ("u4HeaderOffset = %d, u4MacHeaderLen = %d\n", */
    /* u4HeaderOffset, u4MacHeaderLen)); */
    u2RxStatusOffset = sizeof(HW_MAC_RX_DESC_T);
    prSwRfb->ucGroupVLD = (u8)HAL_RX_STATUS_GET_GROUP_VLD(prRxStatus);
    if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_4)) {
        prSwRfb->prRxStatusGroup4 =
            (P_HW_MAC_RX_STS_GROUP_4_T)((u8 *)prRxStatus +
                                        u2RxStatusOffset);
        u2RxStatusOffset += sizeof(HW_MAC_RX_STS_GROUP_4_T);

        /* Fill out the TID and SSN */
        prSwRfb->ucTid = (u8)(HAL_RX_STATUS_GET_TID(prRxStatus));
        prSwRfb->u2SSN = HAL_RX_STATUS_GET_SEQFrag_NUM(
            prSwRfb->prRxStatusGroup4) >>
                         RX_STATUS_SEQ_NUM_OFFSET;
    }
    if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_1)) {
        prSwRfb->prRxStatusGroup1 =
            (P_HW_MAC_RX_STS_GROUP_1_T)((u8 *)prRxStatus +
                                        u2RxStatusOffset);
        u2RxStatusOffset += sizeof(HW_MAC_RX_STS_GROUP_1_T);
    }
    if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_2)) {
        prSwRfb->prRxStatusGroup2 =
            (P_HW_MAC_RX_STS_GROUP_2_T)((u8 *)prRxStatus +
                                        u2RxStatusOffset);
        u2RxStatusOffset += sizeof(HW_MAC_RX_STS_GROUP_2_T);
    }
    if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3)) {
        prSwRfb->prRxStatusGroup3 =
            (P_HW_MAC_RX_STS_GROUP_3_T)((u8 *)prRxStatus +
                                        u2RxStatusOffset);
        u2RxStatusOffset += sizeof(HW_MAC_RX_STS_GROUP_3_T);
    }

    if (u4PktLen < u2RxStatusOffset + u4HeaderOffset) {
        DBGLOG(RX, ERROR,
               "Logic Check Error - u4PktLen(%lu) < (%d + %d)\n",
               u4PktLen, u2RxStatusOffset, u4HeaderOffset);
        return false;
    }
    prSwRfb->u2RxStatusOffst = u2RxStatusOffset;
    prSwRfb->pvHeader =
        (u8 *)prRxStatus + u2RxStatusOffset + u4HeaderOffset;
    prSwRfb->u2PacketLen =
        (u16)(u4PktLen - (u2RxStatusOffset + u4HeaderOffset));
    prSwRfb->u2HeaderLen = (u16)HAL_RX_STATUS_GET_HEADER_LEN(prRxStatus);
    if (u4PktLen <
        u2RxStatusOffset + u4HeaderOffset + prSwRfb->u2HeaderLen) {
        DBGLOG(RX, ERROR,
               "Logic Check Error - u4PktLen(%lu) < (%d + %d + %d)\n",
               u4PktLen, u2RxStatusOffset, u4HeaderOffset,
               prSwRfb->u2HeaderLen);
        return false;
    }
    prSwRfb->ucWlanIdx = (u8)HAL_RX_STATUS_GET_WLAN_IDX(prRxStatus);
    prSwRfb->ucStaRecIdx = secGetStaIdxByWlanIdx(
        prAdapter, (u8)HAL_RX_STATUS_GET_WLAN_IDX(prRxStatus));
    prSwRfb->prStaRec =
        cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
    prSwRfb->ucPayloadFormat = HAL_RX_STATUS_GET_PAYLOAD_FORMAT(prRxStatus);
    prSwRfb->fgHdrTran = HAL_RX_STATUS_IS_HEADER_TRAN(prRxStatus);
    prSwRfb->ucSecMode = HAL_RX_STATUS_GET_SEC_MODE(prRxStatus);
    prSwRfb->ucTid = (u8)HAL_RX_STATUS_GET_TID(prRxStatus);
    prSwRfb->ucHeaderOffset = HAL_RX_STATUS_GET_HEADER_OFFSET(prRxStatus);
    prSwRfb->fgIsBC = HAL_RX_STATUS_IS_BC(prRxStatus);
    prSwRfb->fgIsMC = HAL_RX_STATUS_IS_MC(prRxStatus);

    /* DBGLOG(RX, TRACE, ("Dump Rx packet, u2PacketLen = %d\n",
     * prSwRfb->u2PacketLen)); */
    /* DBGLOG_MEM8(RX, TRACE, prSwRfb->pvHeader, prSwRfb->u2PacketLen); */

    return true;
}

#if CFG_TCP_IP_CHKSUM_OFFLOAD || CFG_TCP_IP_CHKSUM_OFFLOAD_NDIS_60
/*----------------------------------------------------------------------------*/
/*!
 * @brief Fill checksum status in RFB
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 * @param u4TcpUdpIpCksStatus specify the Checksum status
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxFillChksumStatus(IN P_ADAPTER_T prAdapter, IN OUT P_SW_RFB_T prSwRfb,
                           IN u32 u4TcpUdpIpCksStatus)
{
    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    if (prAdapter->u4CSUMFlags != CSUM_NOT_SUPPORTED) {
        if (u4TcpUdpIpCksStatus & RX_CS_TYPE_IPv4) {  /* IPv4 packet */
            prSwRfb->aeCSUM[CSUM_TYPE_IPV6] = CSUM_RES_NONE;
            if (u4TcpUdpIpCksStatus & RX_CS_STATUS_IP) {  /* IP
                                                           * packet
                                                           * csum
                                                           * failed
                                                           */
                prSwRfb->aeCSUM[CSUM_TYPE_IPV4] =
                    CSUM_RES_FAILED;
            } else {
                prSwRfb->aeCSUM[CSUM_TYPE_IPV4] =
                    CSUM_RES_SUCCESS;
            }

            if (u4TcpUdpIpCksStatus & RX_CS_TYPE_TCP) {  /* TCP
                                                          * packet */
                prSwRfb->aeCSUM[CSUM_TYPE_UDP] = CSUM_RES_NONE;
                if (u4TcpUdpIpCksStatus &
                    RX_CS_STATUS_TCP) {  /* TCP packet csum
                                          * failed */
                    prSwRfb->aeCSUM[CSUM_TYPE_TCP] =
                        CSUM_RES_FAILED;
                } else {
                    prSwRfb->aeCSUM[CSUM_TYPE_TCP] =
                        CSUM_RES_SUCCESS;
                }
            } else if (u4TcpUdpIpCksStatus &
                       RX_CS_TYPE_UDP) {  /* UDP
                                           * packet
                                           */
                prSwRfb->aeCSUM[CSUM_TYPE_TCP] = CSUM_RES_NONE;
                if (u4TcpUdpIpCksStatus &
                    RX_CS_STATUS_UDP) {  /* UDP packet csum
                                          * failed */
                    prSwRfb->aeCSUM[CSUM_TYPE_UDP] =
                        CSUM_RES_FAILED;
                } else {
                    prSwRfb->aeCSUM[CSUM_TYPE_UDP] =
                        CSUM_RES_SUCCESS;
                }
            } else {
                prSwRfb->aeCSUM[CSUM_TYPE_UDP] = CSUM_RES_NONE;
                prSwRfb->aeCSUM[CSUM_TYPE_TCP] = CSUM_RES_NONE;
            }
        } else if (u4TcpUdpIpCksStatus & RX_CS_TYPE_IPv6) {  /* IPv6
                                                              * packet */
            prSwRfb->aeCSUM[CSUM_TYPE_IPV4] = CSUM_RES_NONE;
            prSwRfb->aeCSUM[CSUM_TYPE_IPV6] = CSUM_RES_SUCCESS;

            if (u4TcpUdpIpCksStatus & RX_CS_TYPE_TCP) {  /* TCP
                                                          * packet */
                prSwRfb->aeCSUM[CSUM_TYPE_UDP] = CSUM_RES_NONE;
                if (u4TcpUdpIpCksStatus &
                    RX_CS_STATUS_TCP) {  /* TCP packet csum
                                          * failed */
                    prSwRfb->aeCSUM[CSUM_TYPE_TCP] =
                        CSUM_RES_FAILED;
                } else {
                    prSwRfb->aeCSUM[CSUM_TYPE_TCP] =
                        CSUM_RES_SUCCESS;
                }
            } else if (u4TcpUdpIpCksStatus &
                       RX_CS_TYPE_UDP) {  /* UDP
                                           * packet
                                           */
                prSwRfb->aeCSUM[CSUM_TYPE_TCP] = CSUM_RES_NONE;
                if (u4TcpUdpIpCksStatus &
                    RX_CS_STATUS_UDP) {  /* UDP packet csum
                                          * failed */
                    prSwRfb->aeCSUM[CSUM_TYPE_UDP] =
                        CSUM_RES_FAILED;
                } else {
                    prSwRfb->aeCSUM[CSUM_TYPE_UDP] =
                        CSUM_RES_SUCCESS;
                }
            } else {
                prSwRfb->aeCSUM[CSUM_TYPE_UDP] = CSUM_RES_NONE;
                prSwRfb->aeCSUM[CSUM_TYPE_TCP] = CSUM_RES_NONE;
            }
        } else {
            prSwRfb->aeCSUM[CSUM_TYPE_IPV4] = CSUM_RES_NONE;
            prSwRfb->aeCSUM[CSUM_TYPE_IPV6] = CSUM_RES_NONE;
        }
    }
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief nicRxClearFrag() is used to clean all fragments in the fragment cache.
 *
 * \param[in] prAdapter       pointer to the Adapter handler
 * \param[in] prStaRec        The fragment cache is stored under station record.
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxClearFrag(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
    int j;
    FRAG_INFO_T *prFragInfo;

    for (j = 0; j < MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS; j++) {
        prFragInfo = &prStaRec->rFragInfo[j];

        if (prFragInfo->pr1stFrag) {
            nicRxReturnRFB(prAdapter, prFragInfo->pr1stFrag);
            prFragInfo->pr1stFrag = (SW_RFB_T *)NULL;
        }
    }

    DBGLOG(RX, INFO, "%s\n", __func__);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief rxDefragMPDU() is used to defragment the incoming packets.
 *
 * \param[in] prSWRfb        The RFB which is being processed.
 * \param[in] u16     u2FrameCtrl
 *
 * \retval NOT NULL  Receive the last fragment data
 * \retval NULL      Receive the fragment packet which is not the last
 */
/*----------------------------------------------------------------------------*/
P_SW_RFB_T incRxDefragMPDU(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSWRfb,
                           OUT P_QUE_T prReturnedQue)
{
    P_SW_RFB_T prOutputSwRfb = (P_SW_RFB_T)NULL;
#if CFG_SUPPORT_FRAG_SUPPORT
    P_RX_CTRL_T prRxCtrl;
    P_FRAG_INFO_T prFragInfo;
    u32 i = 0, j;
    u16 u2SeqCtrl, u2FrameCtrl;
    u16 u2SeqNo;
    u8 ucFragNo;
    u8 fgFirst = false;
    u8 fgLast = false;
    u32 rCurrentTime;
    P_WLAN_MAC_HEADER_T prWlanHeader = NULL;
    P_HW_MAC_RX_DESC_T prRxStatus = NULL;
    P_HW_MAC_RX_STS_GROUP_4_T prRxStatusGroup4 = NULL;
#if CFG_SUPPORT_FRAG_ATTACK_DETECTION
    u8 ucSecMode = CIPHER_SUITE_NONE;
    u64 u8PN;
#endif
    u16 u2Frag1FrameCtrl;

    DEBUGFUNC("nicRx: rxmDefragMPDU\n");

    ASSERT(prSWRfb);

    prRxCtrl = &prAdapter->rRxCtrl;

    prRxStatus = prSWRfb->prRxStatus;
    ASSERT(prRxStatus);

    if (HAL_RX_STATUS_IS_HEADER_TRAN(prRxStatus) == false) {
        prWlanHeader = (P_WLAN_MAC_HEADER_T)prSWRfb->pvHeader;
        prSWRfb->u2SequenceControl = prWlanHeader->u2SeqCtrl;
        u2FrameCtrl = prWlanHeader->u2FrameCtrl;
    } else {
        prRxStatusGroup4 = prSWRfb->prRxStatusGroup4;
        prSWRfb->u2SequenceControl =
            HAL_RX_STATUS_GET_SEQFrag_NUM(prRxStatusGroup4);
        u2FrameCtrl =
            HAL_RX_STATUS_GET_FRAME_CTL_FIELD(prRxStatusGroup4);
    }
    u2SeqCtrl = prSWRfb->u2SequenceControl;
    u2SeqNo = u2SeqCtrl >> MASK_SC_SEQ_NUM_OFFSET;
    ucFragNo = (u8)(u2SeqCtrl & MASK_SC_FRAG_NUM);
    prSWRfb->u2FrameCtrl = u2FrameCtrl;

    if (!(u2FrameCtrl & MASK_FC_MORE_FRAG)) {
        /* The last fragment frame */
        if (ucFragNo) {
            DBGLOG(RX, LOUD, "FC %04x M %04x SQ %04x\n",
                   u2FrameCtrl, (u2FrameCtrl & MASK_FC_MORE_FRAG),
                   u2SeqCtrl);
            fgLast = true;
        }
        /* Non-fragment frame */
        else {
            return prSWRfb;
        }
    }
    /* The fragment frame except the last one */
    else {
        if (ucFragNo == 0) {
            DBGLOG(RX, LOUD, "FC %04x M %04x SQ %04x\n",
                   u2FrameCtrl, (u2FrameCtrl & MASK_FC_MORE_FRAG),
                   u2SeqCtrl);
            fgFirst = true;
        } else {
            DBGLOG(RX, LOUD, "FC %04x M %04x SQ %04x\n",
                   u2FrameCtrl, (u2FrameCtrl & MASK_FC_MORE_FRAG),
                   u2SeqCtrl);
        }
    }

    GET_CURRENT_SYSTIME(&rCurrentTime);

#if CFG_SUPPORT_FRAG_ATTACK_DETECTION
    /* check cipher suite to set if we need to get PN */
    if (prSWRfb->ucSecMode == CIPHER_SUITE_TKIP ||
        prSWRfb->ucSecMode == CIPHER_SUITE_TKIP_WO_MIC ||
        prSWRfb->ucSecMode == CIPHER_SUITE_CCMP ||
        prSWRfb->ucSecMode == CIPHER_SUITE_CCMP_W_CCX ||
        prSWRfb->ucSecMode == CIPHER_SUITE_CCMP_256 ||
        prSWRfb->ucSecMode == CIPHER_SUITE_GCMP_128 ||
        prSWRfb->ucSecMode == CIPHER_SUITE_GCMP_256) {
        ucSecMode = prSWRfb->ucSecMode;
        if (!qmRxPNtoU64(prSWRfb->prRxStatusGroup1->aucPN, CCMPTSCPNNUM,
                         &u8PN)) {
            DBGLOG(QM, ERROR, "PN2U64 failed\n");
            /* should not enter here, just fallback */
            ucSecMode = CIPHER_SUITE_NONE;
        }
    }
#endif

    for (j = 0; j < MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS; j++) {
        prFragInfo = &prSWRfb->prStaRec->rFragInfo[j];
        if (prFragInfo->pr1stFrag) {
            /* I. If the receive timer for the MSDU or MMPDU that is
             * stored in the fragments queue exceeds
             * dot11MaxReceiveLifetime, we discard the uncompleted
             * fragments. II. If we didn't receive the last MPDU for
             * a period, we use this function for remove frames.
             */
            if (CHECK_FOR_EXPIRATION(
                    rCurrentTime,
                    prFragInfo->rReceiveLifetimeLimit)) {
                /* cnmPktFree((P_PKT_INFO_T)prFragInfo->pr1stFrag,
                 * true); */
                prFragInfo->pr1stFrag->eDst =
                    RX_PKT_DESTINATION_NULL;
                QUEUE_INSERT_TAIL(
                    prReturnedQue,
                    (P_QUE_ENTRY_T)prFragInfo->pr1stFrag);

                prFragInfo->pr1stFrag = (P_SW_RFB_T)NULL;
            }
        }
    }

    for (i = 0; i < MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS; i++) {
        prFragInfo = &prSWRfb->prStaRec->rFragInfo[i];

        if (fgFirst) {  /* looking for timed-out frag buffer */
            if (prFragInfo->pr1stFrag == (P_SW_RFB_T)NULL) {  /* find a
                                                               * free
                                                               * frag
                                                               * buffer
                                                               */
                break;
            }
        } else {  /* looking for a buffer with desired next seqctrl */
            if (prFragInfo->pr1stFrag == (P_SW_RFB_T)NULL) {
                continue;
            }

            u2Frag1FrameCtrl = prFragInfo->pr1stFrag->u2FrameCtrl;

            if (RXM_IS_QOS_DATA_FRAME(u2FrameCtrl)) {
                if (RXM_IS_QOS_DATA_FRAME(u2Frag1FrameCtrl)) {
                    if (u2SeqNo == prFragInfo->u2SeqNo
#if CFG_SUPPORT_FRAG_ATTACK_DETECTION
                        &&
                        ucSecMode == prFragInfo->ucSecMode
#endif
                        ) {
                        break;
                    }
                }
            } else {
                if (!RXM_IS_QOS_DATA_FRAME(u2Frag1FrameCtrl)) {
                    if (u2SeqNo == prFragInfo->u2SeqNo
#if CFG_SUPPORT_FRAG_ATTACK_DETECTION
                        &&
                        ucSecMode == prFragInfo->ucSecMode
#endif
                        ) {
                        break;
                    }
                }
            }
        }
    }

    if (i >= MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS) {
        /* Can't find a proper FRAG_INFO_T.
         * I. 1st Fragment MPDU, all of the FragInfo are exhausted
         * II. 2nd ~ (n-1)th Fragment MPDU, can't find the right
         * FragInfo for defragment. Because we won't process fragment
         * frame outside this function, so we should free it right away.
         */
        nicRxReturnRFB(prAdapter, prSWRfb);

        return (P_SW_RFB_T)NULL;
    }

    if (prFragInfo->pr1stFrag != (SW_RFB_T *)NULL) {
        /* check if the FragNo is cont. */
        if (ucFragNo != prFragInfo->ucNextFragNo
#if CFG_SUPPORT_FRAG_ATTACK_DETECTION
            || ((ucSecMode != CIPHER_SUITE_NONE) &&
                (u8PN != prFragInfo->u8NextPN))
#endif
            ) {
            DBGLOG(RX, INFO, "non-cont FragNo or PN, drop it.");

            DBGLOG(RX,
                   INFO,
                   "u2SeqNo = %04x, NextFragNo = %02x, FragNo = %02x\n",
                   prFragInfo->u2SeqNo,
                   prFragInfo->ucNextFragNo,
                   ucFragNo);

#if CFG_SUPPORT_FRAG_ATTACK_DETECTION
            if (ucSecMode != CIPHER_SUITE_NONE) {
                DBGLOG(RX,
                       INFO,
                       "u2SeqNo = %04x, NextPN = %016x, PN = %016x\n",
                       prFragInfo->u2SeqNo,
                       prFragInfo->u8NextPN,
                       u8PN);
            }
#endif

            /* discard fragments if FragNo is non-cont. */
            nicRxReturnRFB(prAdapter, prFragInfo->pr1stFrag);
            prFragInfo->pr1stFrag = (SW_RFB_T *)NULL;

            nicRxReturnRFB(prAdapter, prSWRfb);
            return (SW_RFB_T *)NULL;
        }
    }

    ASSERT(prFragInfo);

    /* retrieve Rx payload */
    prSWRfb->u2HeaderLen = HAL_RX_STATUS_GET_HEADER_LEN(prRxStatus);
    prSWRfb->pucPayload = (u8 *)(((unsigned long)prSWRfb->pvHeader) +
                                 prSWRfb->u2HeaderLen);
    prSWRfb->u2PayloadLength =
        (u16)(HAL_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus) -
              ((unsigned long)prSWRfb->pucPayload -
               (unsigned long)prRxStatus));

    if (fgFirst) {
        DBGLOG(RX, LOUD, "rxDefragMPDU first\n");

        SET_EXPIRATION_TIME(
            prFragInfo->rReceiveLifetimeLimit,
            TU_TO_SYSTIME(DOT11_RECEIVE_LIFETIME_TU_DEFAULT));

        prFragInfo->pr1stFrag = prSWRfb;

        prFragInfo->pucNextFragStart =
            (u8 *)prSWRfb->pucRecvBuff +
            HAL_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus);

        prFragInfo->u2SeqNo = u2SeqNo;
        prFragInfo->ucNextFragNo = ucFragNo + 1;  /* should be 1 */

#if CFG_SUPPORT_FRAG_ATTACK_DETECTION
        prFragInfo->ucSecMode = ucSecMode;
        if (prFragInfo->ucSecMode != CIPHER_SUITE_NONE) {
            prFragInfo->u8NextPN = u8PN + 1;
        }else{
            prFragInfo->u8NextPN = 0;
        }
#endif

        DBGLOG(RX,
               LOUD,
               "First: SeqCtrl = %04x, SeqNo = %04x, NextFragNo = %02x\n",
               u2SeqCtrl,
               prFragInfo->u2SeqNo,
               prFragInfo->ucNextFragNo);

        /* prSWRfb->fgFragmented = true; */
        /* whsu: todo for checksum */
    } else {
        prFragInfo->pr1stFrag->prRxStatus->u2RxByteCount +=
            prSWRfb->u2PayloadLength;

        if (prFragInfo->pr1stFrag->prRxStatus->u2RxByteCount >
            CFG_RX_MAX_PKT_SIZE) {
            prFragInfo->pr1stFrag->eDst = RX_PKT_DESTINATION_NULL;
            QUEUE_INSERT_TAIL(prReturnedQue,
                              (P_QUE_ENTRY_T)prFragInfo->pr1stFrag);

            prFragInfo->pr1stFrag = (P_SW_RFB_T)NULL;

            nicRxReturnRFB(prAdapter, prSWRfb);
        } else {
            kalMemCopy(prFragInfo->pucNextFragStart,
                       prSWRfb->pucPayload,
                       prSWRfb->u2PayloadLength);
            /* [6630] update rx byte count and packet length */
            prFragInfo->pr1stFrag->u2PacketLen +=
                prSWRfb->u2PayloadLength;
            prFragInfo->pr1stFrag->u2PayloadLength +=
                prSWRfb->u2PayloadLength;

            if (fgLast) {  /* The last one, free the buffer */
                DBGLOG(RX, LOUD, "Defrag: finished\n");

                prOutputSwRfb = prFragInfo->pr1stFrag;

                prFragInfo->pr1stFrag = (P_SW_RFB_T)NULL;
            } else {
                DBGLOG(RX, LOUD, "Defrag: mid fraged\n");

                prFragInfo->pucNextFragStart +=
                    prSWRfb->u2PayloadLength;

                prFragInfo->ucNextFragNo++;

#if CFG_SUPPORT_FRAG_ATTACK_DETECTION
                if (prFragInfo->ucSecMode !=
                    CIPHER_SUITE_NONE) {
                    /* PN in security protocol header */
                    prFragInfo->u8NextPN++;
                }
#endif
            }

            nicRxReturnRFB(prAdapter, prSWRfb);
        }
    }

    /* DBGLOG_MEM8(RXM, INFO, */
    /* prFragInfo->pr1stFrag->pucPayload, */
    /* prFragInfo->pr1stFrag->u2PayloadLength); */
#else
    /* no CFG_SUPPORT_FRAG_SUPPORT, so just free it */
    nicRxReturnRFB(prAdapter, prSWRfb);
#endif

    return prOutputSwRfb;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Do duplicate detection
 *
 * @param prSwRfb Pointer to the RX packet
 *
 * @return true: a duplicate, false: not a duplicate
 */
/*----------------------------------------------------------------------------*/
u8 nicRxIsDuplicateFrame(IN OUT P_SW_RFB_T prSwRfb)
{
    /* Non-QoS Unicast Data or Unicast MMPDU: SC Cache #4;
     *   QoS Unicast Data: SC Cache #0~3;
     *   Broadcast/Multicast: RetryBit == 0
     */
    u32 u4SeqCtrlCacheIdx;
    u16 u2SequenceControl, u2FrameCtrl;
    u8 fgIsDuplicate = false, fgIsAmsduSubframe = false;
    P_WLAN_MAC_HEADER_T prWlanHeader = NULL;
    P_HW_MAC_RX_DESC_T prRxStatus = NULL;
    P_HW_MAC_RX_STS_GROUP_4_T prRxStatusGroup4 = NULL;

    DEBUGFUNC("nicRx: Enter rxmIsDuplicateFrame()\n");

    ASSERT(prSwRfb);

    /* Situations in which the STC_REC is missing include:
     *   (1) Probe Request (2) (Re)Association Request (3) IBSS data frames
     * (4) Probe Response
     */
    if (!prSwRfb->prStaRec) {
        return false;
    }

    prRxStatus = prSwRfb->prRxStatus;
    ASSERT(prRxStatus);

    fgIsAmsduSubframe = HAL_RX_STATUS_GET_PAYLOAD_FORMAT(prRxStatus);
    if (HAL_RX_STATUS_IS_HEADER_TRAN(prRxStatus) == false) {
        prWlanHeader = (P_WLAN_MAC_HEADER_T)prSwRfb->pvHeader;
        u2SequenceControl = prWlanHeader->u2SeqCtrl;
        u2FrameCtrl = prWlanHeader->u2FrameCtrl;
    } else {
        prRxStatusGroup4 = prSwRfb->prRxStatusGroup4;
        u2SequenceControl =
            HAL_RX_STATUS_GET_SEQFrag_NUM(prRxStatusGroup4);
        u2FrameCtrl =
            HAL_RX_STATUS_GET_FRAME_CTL_FIELD(prRxStatusGroup4);
    }
    prSwRfb->u2SequenceControl = u2SequenceControl;

    /* Case 1: Unicast QoS data */
    if (RXM_IS_QOS_DATA_FRAME(u2FrameCtrl)) {  /* WLAN header shall exist
                                                * when doing duplicate
                                                * detection */
        if (prSwRfb->ucTid < CFG_RX_MAX_BA_TID_NUM &&
            prSwRfb->prStaRec->aprRxReorderParamRefTbl[prSwRfb->ucTid]) {
            /* QoS data with an RX BA agreement
             *  Case 1: The packet is not an AMPDU subframe, so the
             * RetryBit may be set to 1 (TBC). Case 2: The RX BA
             * agreement was just established. Some enqueued packets
             * may not be sent with aggregation.
             */

            DBGLOG(RX, LOUD, "RX: SC=0x%X (BA Entry present)\n",
                   u2SequenceControl);

            /* Update the SN cache in order to ensure the
             * correctness of duplicate removal in case the BA
             * agreement is deleted
             */
            prSwRfb->prStaRec->au2CachedSeqCtrl[prSwRfb->ucTid] =
                u2SequenceControl;

            /* HW scoreboard shall take care Case 1. Let the layer
             * layer handle Case 2. */
            return false;  /* Not a duplicate */
        }

        if (prSwRfb->prStaRec->ucDesiredPhyTypeSet &
            (PHY_TYPE_BIT_HT | PHY_TYPE_BIT_VHT)) {
            u4SeqCtrlCacheIdx = prSwRfb->ucTid;
        } else {
            if (prSwRfb->ucTid < 8) {  /* UP = 0~7 */
                u4SeqCtrlCacheIdx = aucTid2ACI[prSwRfb->ucTid];
            } else {
                DBGLOG(RX,
                       WARN,
                       "RXM: (Warning) Unknown QoS Data with TID=%d\n",
                       prSwRfb->ucTid);

                return true;  /* Will be dropped */
            }
        }
    }
    /* Case 2: Unicast non-QoS data or MMPDUs */
    else {
        u4SeqCtrlCacheIdx = TID_NUM;
    }

    /* If this is a retransmission */
    if (u2FrameCtrl & MASK_FC_RETRY) {
        if (u2SequenceControl !=
            prSwRfb->prStaRec->au2CachedSeqCtrl[u4SeqCtrlCacheIdx]) {
            prSwRfb->prStaRec->au2CachedSeqCtrl[u4SeqCtrlCacheIdx] =
                u2SequenceControl;
            if (fgIsAmsduSubframe ==
                RX_PAYLOAD_FORMAT_FIRST_SUB_AMSDU) {
                prSwRfb->prStaRec->afgIsIgnoreAmsduDuplicate
                [u4SeqCtrlCacheIdx] = true;
            }
            DBGLOG(RX, LOUD, "RXM: SC= 0x%X (Cache[%lu] updated)\n",
                   u2SequenceControl, u4SeqCtrlCacheIdx);
        } else {
            /* A duplicate. */
            if (prSwRfb->prStaRec->afgIsIgnoreAmsduDuplicate
                [u4SeqCtrlCacheIdx]) {
                if (fgIsAmsduSubframe ==
                    RX_PAYLOAD_FORMAT_LAST_SUB_AMSDU) {
                    prSwRfb->prStaRec
                    ->afgIsIgnoreAmsduDuplicate
                    [u4SeqCtrlCacheIdx] =
                        false;
                }
            } else {
                fgIsDuplicate = true;
                DBGLOG(RX, LOUD,
                       "RXM: SC= 0x%X (Cache[%lu] duplicate)\n",
                       u2SequenceControl, u4SeqCtrlCacheIdx);
            }
        }
    }
    /* Not a retransmission */
    else {
        prSwRfb->prStaRec->au2CachedSeqCtrl[u4SeqCtrlCacheIdx] =
            u2SequenceControl;
        prSwRfb->prStaRec->afgIsIgnoreAmsduDuplicate[u4SeqCtrlCacheIdx]
            =
                false;

        DBGLOG(RX, LOUD, "RXM: SC= 0x%X (Cache[%lu] updated)\n",
               u2SequenceControl, u4SeqCtrlCacheIdx);
    }

    return fgIsDuplicate;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process packet doesn't need to do buffer reordering
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessPktWithoutReorder(IN P_ADAPTER_T prAdapter,
                                   IN P_SW_RFB_T prSwRfb)
{
    P_RX_CTRL_T prRxCtrl;
    P_TX_CTRL_T prTxCtrl;
    u32 u4CurrentRxBufferCount;
    /* P_STA_RECORD_T prStaRec = (P_STA_RECORD_T)NULL; */

    DEBUGFUNC("nicRxProcessPktWithoutReorder");
    /* DBGLOG(RX, TRACE, ("\n")); */

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    prTxCtrl = &prAdapter->rTxCtrl;
    ASSERT(prTxCtrl);

    u4CurrentRxBufferCount = prRxCtrl->rFreeSwRfbList.u4NumElem;

#if CFG_ENABLE_PER_STA_STATISTICS
    if (prSwRfb->prStaRec &&
        (prAdapter->rWifiVar.rWfdConfigureSettings.ucWfdEnable > 0)) {
        prSwRfb->prStaRec->u4TotalRxPktsNumber++;
    }
#endif
    if (kalProcessRxPacket(prAdapter->prGlueInfo, prSwRfb->pvPacket,
                           prSwRfb->pvHeader, (u32)prSwRfb->u2PacketLen,
                           prSwRfb->aeCSUM) != WLAN_STATUS_SUCCESS) {
        DBGLOG(RX,
               ERROR,
               "kalProcessRxPacket return value != WLAN_STATUS_SUCCESS\n");
        ASSERT(0);

        nicRxReturnRFB(prAdapter, prSwRfb);
        return;
    }

    if (HAL_IS_RX_DIRECT(prAdapter)) {
        kalRxIndicateOnePkt(prAdapter->prGlueInfo, prSwRfb->pvPacket);
        RX_ADD_CNT(prRxCtrl, RX_DATA_INDICATION_COUNT, 1);
    } else {
        KAL_SPIN_LOCK_DECLARATION();

        KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_TO_OS_QUE);
        QUEUE_INSERT_TAIL(&(prAdapter->rRxQueue),
                          (P_QUE_ENTRY_T)GLUE_GET_PKT_QUEUE_ENTRY(
                              prSwRfb->pvPacket));
        KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_TO_OS_QUE);

        prRxCtrl->ucNumIndPacket++;
        kalSetTxEvent2Rx(prAdapter->prGlueInfo);
    }

    prSwRfb->pvPacket = NULL;

    /* Return RFB */
    if (!timerPendingTimer(&prAdapter->rPacketDelaySetupTimer)) {
        if (nicRxSetupRFB(prAdapter, prSwRfb)) {
            DBGLOG(RX,
                   WARN,
                   "Allocate SwRfb packet buf failed, Start ReturnIndicatedRfb Timer (%u)\n",
                   RX_RETURN_INDICATED_RFB_TIMEOUT_SEC);
            cnmTimerStartTimer(
                prAdapter, &prAdapter->rPacketDelaySetupTimer,
                SEC_TO_MSEC(
                    RX_RETURN_INDICATED_RFB_TIMEOUT_SEC));
        }
    }
    nicRxReturnRFB(prAdapter, prSwRfb);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process forwarding data packet
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessForwardPkt(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
    P_MSDU_INFO_T prMsduInfo, prRetMsduInfoList;
    P_TX_CTRL_T prTxCtrl;
    P_RX_CTRL_T prRxCtrl;

    KAL_SPIN_LOCK_DECLARATION();

    DEBUGFUNC("nicRxProcessForwardPkt");

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prTxCtrl = &prAdapter->rTxCtrl;
    prRxCtrl = &prAdapter->rRxCtrl;

    prMsduInfo = cnmPktAlloc(prAdapter, 0);

    if (prMsduInfo &&
        kalProcessRxPacket(prAdapter->prGlueInfo, prSwRfb->pvPacket,
                           prSwRfb->pvHeader, (u32)prSwRfb->u2PacketLen,
                           prSwRfb->aeCSUM) == WLAN_STATUS_SUCCESS) {
        /* parsing forward frame */
        wlanProcessTxFrame(prAdapter,
                           (P_NATIVE_PACKET)(prSwRfb->pvPacket));
        /* pack into MSDU_INFO_T */
        nicTxFillMsduInfo(prAdapter, prMsduInfo,
                          (P_NATIVE_PACKET)(prSwRfb->pvPacket));

        prMsduInfo->eSrc = TX_PACKET_FORWARDING;
        prMsduInfo->ucBssIndex =
            secGetBssIdxByWlanIdx(prAdapter, prSwRfb->ucWlanIdx);

        /* release RX buffer (to rIndicatedRfbList) */
        prSwRfb->pvPacket = NULL;
        nicRxReturnRFB(prAdapter, prSwRfb);

        /* Handle if prMsduInfo out of bss index range*/
        if (prMsduInfo->ucBssIndex > BSSID_NUM) {
            DBGLOG(QM, INFO, "Invalid bssidx:%u\n",
                   prMsduInfo->ucBssIndex);
            if (prMsduInfo->pfTxDoneHandler != NULL) {
                prMsduInfo->pfTxDoneHandler(
                    prAdapter, prMsduInfo,
                    TX_RESULT_DROPPED_IN_DRIVER);
            }
            nicTxReturnMsduInfo(prAdapter, prMsduInfo);
            return;
        }

        /* increase forward frame counter */
        GLUE_INC_REF_CNT(prTxCtrl->i4PendingFwdFrameCount);

        /* send into TX queue */
        KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);
        prRetMsduInfoList = qmEnqueueTxPackets(prAdapter, prMsduInfo);
        KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);

        if (prRetMsduInfoList != NULL) {  /* TX queue refuses queuing the
                                           * packet */
            nicTxFreeMsduInfoPacket(prAdapter, prRetMsduInfoList);
            nicTxReturnMsduInfo(prAdapter, prRetMsduInfoList);
        }
        /* indicate service thread for sending */
        if (prTxCtrl->i4PendingFwdFrameCount > 0) {
            kalSetEvent(prAdapter->prGlueInfo);
        }
    } else {  /* no TX resource */
        DBGLOG(QM, INFO, "No Tx MSDU_INFO for forwarding frames\n");
        nicRxReturnRFB(prAdapter, prSwRfb);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process broadcast data packet for both host and forwarding
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessGOBroadcastPkt(IN P_ADAPTER_T prAdapter,
                                IN P_SW_RFB_T prSwRfb)
{
    P_SW_RFB_T prSwRfbDuplicated;
    P_TX_CTRL_T prTxCtrl;
    P_RX_CTRL_T prRxCtrl;
    P_HW_MAC_RX_DESC_T prRxStatus;

    KAL_SPIN_LOCK_DECLARATION();

    DEBUGFUNC("nicRxProcessGOBroadcastPkt");

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prTxCtrl = &prAdapter->rTxCtrl;
    prRxCtrl = &prAdapter->rRxCtrl;

    prRxStatus = prSwRfb->prRxStatus;
    ASSERT(prRxStatus);

    ASSERT(CFG_NUM_OF_QM_RX_PKT_NUM >= 16);

    if (prRxCtrl->rFreeSwRfbList.u4NumElem >=
        (CFG_RX_MAX_PKT_NUM -
         (CFG_NUM_OF_QM_RX_PKT_NUM - 16  /* Reserved for others */ ))) {
        /* 1. Duplicate SW_RFB_T */
        KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
        QUEUE_REMOVE_HEAD(&prRxCtrl->rFreeSwRfbList, prSwRfbDuplicated,
                          P_SW_RFB_T);
        KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

        if (prSwRfbDuplicated) {
            kalMemCopy(prSwRfbDuplicated->pucRecvBuff,
                       prSwRfb->pucRecvBuff,
                       ALIGN_4(prRxStatus->u2RxByteCount +
                               HIF_RX_HW_APPENDED_LEN));

            prSwRfbDuplicated->ucPacketType = RX_PKT_TYPE_RX_DATA;
            prSwRfbDuplicated->ucStaRecIdx = prSwRfb->ucStaRecIdx;
            if (nicRxFillRFB(prAdapter, prSwRfbDuplicated) ==
                false) {
                nicRxReturnRFB(prAdapter, prSwRfb);
                return;
            }

            /* 2. Modify eDst */
            prSwRfbDuplicated->eDst = RX_PKT_DESTINATION_FORWARD;

            /* 4. Forward */
            nicRxProcessForwardPkt(prAdapter, prSwRfbDuplicated);
        }
    } else {
        DBGLOG(RX,
               WARN,
               "Stop to forward BMC packet due to less free Sw Rfb %lu\n",
               prRxCtrl->rFreeSwRfbList.u4NumElem);
    }

    /* 3. Indicate to host */
    prSwRfb->eDst = RX_PKT_DESTINATION_HOST;
    nicRxProcessPktWithoutReorder(prAdapter, prSwRfb);
}

#if CFG_SUPPORT_SNIFFER
void nicRxFillRadiotapMCS(IN OUT P_MONITOR_RADIOTAP_T prMonitorRadiotap,
                          IN P_HW_MAC_RX_STS_GROUP_3_T prRxStatusGroup3)
{
    u8 ucFrMode;
    u8 ucShortGI;
    u8 ucRxMode;
    u8 ucLDPC;
    u8 ucSTBC;
    u8 ucNess;

    ucFrMode = (((prRxStatusGroup3)->u4RxVector[0] & RX_VT_FR_MODE_MASK) >>
                RX_VT_FR_MODE_OFFSET);
    /* VHTA1 B0-B1 */
    ucShortGI = ((prRxStatusGroup3)->u4RxVector[0] & RX_VT_SHORT_GI) ?
                1 :
                0;  /* HT_shortgi */
    ucRxMode = (((prRxStatusGroup3)->u4RxVector[0] & RX_VT_RX_MODE_MASK) >>
                RX_VT_RX_MODE_OFFSET);
    ucLDPC = ((prRxStatusGroup3)->u4RxVector[0] & RX_VT_LDPC) ?
             1 :
             0;  /* HT_adcode
                  */
    ucSTBC = (((prRxStatusGroup3)->u4RxVector[0] & RX_VT_STBC_MASK) >>
              RX_VT_STBC_OFFSET);  /* HT_stbc */
    ucNess = (((prRxStatusGroup3)->u4RxVector[0] & RX_VT_NESS_MASK) >>
              RX_VT_NESS_OFFSET);  /* HT_extltf */

    prMonitorRadiotap->ucMcsKnown =
        (BITS(0, 6) | (((ucNess & BIT(1)) >> 1) << 7));

    prMonitorRadiotap->ucMcsFlags =
        ((ucFrMode) | (ucShortGI << 2) | ((ucRxMode & BIT(0)) << 3) |
         (ucLDPC << 4) | (ucSTBC << 5) | ((ucNess & BIT(0)) << 7));
    /* Bit[6:0] for 802.11n, mcs0 ~ mcs7 */
    prMonitorRadiotap->ucMcsMcs =
        ((prRxStatusGroup3)->u4RxVector[0] & RX_VT_RX_RATE_MASK);
}

void nicRxFillRadiotapVHT(IN OUT P_MONITOR_RADIOTAP_T prMonitorRadiotap,
                          IN P_HW_MAC_RX_STS_GROUP_3_T prRxStatusGroup3)
{
    u8 ucSTBC;
    u8 ucTxopPsNotAllow;
    u8 ucShortGI;
    u8 ucNsym;
    u8 ucLdpcExtraOfdmSym;
    u8 ucBeamFormed;
    u8 ucFrMode;
    u8 ucNsts;
    u8 ucMcs;

    prMonitorRadiotap->u2VhtKnown = RADIOTAP_VHT_ALL_KNOWN;
    prMonitorRadiotap->u2VhtKnown &= ~RADIOTAP_VHT_SHORT_GI_NSYM_KNOWN;

    ucSTBC = (((prRxStatusGroup3)->u4RxVector[0] & RX_VT_STBC_MASK) >>
              RX_VT_STBC_OFFSET);  /* BIT[7]: VHTA1 B3 */
    ucTxopPsNotAllow = ((prRxStatusGroup3)->u4RxVector[0] &
                        RX_VT_TXOP_PS_NOT_ALLOWED) ?
                       1 :
                       0;  /* VHTA1 B22 */
    /*
     * ucNsym = ((prRxStatusGroup3)->u4RxVector[0] & RX_VT_SHORT_GI_NSYM) ?
     * 1 : 0; //VHTA2 B1
     */
    ucNsym = 0;  /* Invalid in MT6632*/
    ucShortGI = ((prRxStatusGroup3)->u4RxVector[0] & RX_VT_SHORT_GI) ?
                1 :
                0;  /* VHTA2 B0 */
    ucLdpcExtraOfdmSym = ((prRxStatusGroup3)->u4RxVector[0] &
                          RX_VT_LDPC_EXTRA_OFDM_SYM) ?
                         1 :
                         0;  /* VHTA2 B3 */
    ucBeamFormed = ((prRxStatusGroup3)->u4RxVector[0] & RX_VT_BEAMFORMED) ?
                   1 :
                   0;  /* VHTA2 B8 */
    prMonitorRadiotap->ucVhtFlags =
        ((ucSTBC) | (ucTxopPsNotAllow << 1) | (ucShortGI << 2) |
         (ucNsym << 3) | (ucLdpcExtraOfdmSym << 4) |
         (ucBeamFormed << 5));

    ucFrMode = (((prRxStatusGroup3)->u4RxVector[0] & RX_VT_FR_MODE_MASK) >>
                RX_VT_FR_MODE_OFFSET);
    /* VHTA1 B0-B1 */
    switch (ucFrMode) {
    case RX_VT_FR_MODE_20:
        prMonitorRadiotap->ucVhtBandwidth = 0;
        break;

    case RX_VT_FR_MODE_40:
        prMonitorRadiotap->ucVhtBandwidth = 1;
        break;

    case RX_VT_FR_MODE_80:
        prMonitorRadiotap->ucVhtBandwidth = 4;
        break;

    case RX_VT_FR_MODE_160:
        prMonitorRadiotap->ucVhtBandwidth = 11;
        break;

    default:
        prMonitorRadiotap->ucVhtBandwidth = 0;
    }

    /* Set to 0~7 for 1~8 space time streams */
    ucNsts = (((prRxStatusGroup3)->u4RxVector[1] & RX_VT_NSTS_MASK) >>
              RX_VT_NSTS_OFFSET) +
             1;
    /* VHTA1 B10-B12 */

    /* Bit[3:0] for 802.11ac, mcs0 ~ mcs9 */
    ucMcs = ((prRxStatusGroup3)->u4RxVector[0] & RX_VT_RX_RATE_AC_MASK);

    prMonitorRadiotap->aucVhtMcsNss[0] =
        ((ucMcs << 4) | (ucNsts - ucSTBC));  /* STBC = Nsts - Nss */

    /*
     * prMonitorRadiotap->ucVhtCoding =
     * (((prRxStatusGroup3)->u4RxVector[0] & RX_VT_CODING_MASK) >>
     * RX_VT_CODING_OFFSET);
     */
    prMonitorRadiotap->ucVhtCoding = 0;  /* Invalid in MT6632*/

    /* VHTA2 B2-B3 */

    prMonitorRadiotap->ucVhtGroupId =
        (((((prRxStatusGroup3)->u4RxVector[1] & RX_VT_GROUPID_1_MASK) >>
           RX_VT_GROUPID_1_OFFSET)
            << 2) |
         (((prRxStatusGroup3)->u4RxVector[0] & RX_VT_GROUPID_0_MASK) >>
          RX_VT_GROUPID_0_OFFSET));
    /* VHTA1 B4-B9 */
    /* VHTA1 B13-B21 */
    prMonitorRadiotap->u2VhtPartialAid =
        ((((prRxStatusGroup3)->u4RxVector[2] & RX_VT_AID_1_MASK) << 4) |
         (((prRxStatusGroup3)->u4RxVector[1] & RX_VT_AID_0_MASK) >>
          RX_VT_AID_0_OFFSET));
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process HIF monitor packet
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessMonitorPacket(IN P_ADAPTER_T prAdapter,
                               IN OUT P_SW_RFB_T prSwRfb)
{
    struct sk_buff *prSkb = NULL;
    P_RX_CTRL_T prRxCtrl;
    P_HW_MAC_RX_DESC_T prRxStatus;
    P_HW_MAC_RX_STS_GROUP_2_T prRxStatusGroup2;
    P_HW_MAC_RX_STS_GROUP_3_T prRxStatusGroup3;
    MONITOR_RADIOTAP_T rMonitorRadiotap;
    RADIOTAP_FIELD_VENDOR_T rRadiotapFieldVendor;
    u8 *prVendorNsOffset;
    u32 u4VendorNsLen;
    u32 u4RadiotapLen;
    u32 u4ItPresent;
    u8 aucMtkOui[] = VENDOR_OUI_MTK;
    u8 ucRxRate;
    u8 ucRxMode;
    u8 ucChanNum;
    u8 ucMcs;
    u8 ucFrMode;
    u8 ucShortGI;
    u32 u4PhyRate;

    KAL_SPIN_LOCK_DECLARATION();

    DEBUGFUNC("nicRxProcessMonitorPacket");

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prRxCtrl = &prAdapter->rRxCtrl;

    if (nicRxFillRFB(prAdapter, prSwRfb) == false) {
        nicRxReturnRFB(prAdapter, prSwRfb);
        return;
    }

    /* can't parse radiotap info if no rx vector */
    if (((prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_2)) == 0) ||
        ((prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3)) == 0)) {
        nicRxReturnRFB(prAdapter, prSwRfb);
        return;
    }

    prRxStatus = prSwRfb->prRxStatus;
    prRxStatusGroup2 = prSwRfb->prRxStatusGroup2;
    prRxStatusGroup3 = prSwRfb->prRxStatusGroup3;

    /* Bit Number 30 Vendor Namespace */
    u4VendorNsLen = sizeof(RADIOTAP_FIELD_VENDOR_T);
    rRadiotapFieldVendor.aucOUI[0] = aucMtkOui[0];
    rRadiotapFieldVendor.aucOUI[1] = aucMtkOui[1];
    rRadiotapFieldVendor.aucOUI[2] = aucMtkOui[2];
    rRadiotapFieldVendor.ucSubNamespace = 0;
    rRadiotapFieldVendor.u2DataLen = u4VendorNsLen - 6;
    /* VHTA1 B0-B1 */
    rRadiotapFieldVendor.ucData =
        (((prRxStatusGroup3)->u4RxVector[0] & RX_VT_FR_MODE_MASK) >>
         RX_VT_FR_MODE_OFFSET);

    ucRxMode = (((prRxStatusGroup3)->u4RxVector[0] & RX_VT_RX_MODE_MASK) >>
                RX_VT_RX_MODE_OFFSET);

    if (ucRxMode == RX_VT_VHT_MODE) {
        u4RadiotapLen = RADIOTAP_LEN_VHT;
        u4ItPresent = RADIOTAP_FIELDS_VHT;
    } else if ((ucRxMode == RX_VT_MIXED_MODE) ||
               (ucRxMode == RX_VT_GREEN_MODE)) {
        u4RadiotapLen = RADIOTAP_LEN_HT;
        u4ItPresent = RADIOTAP_FIELDS_HT;
    } else {
        u4RadiotapLen = RADIOTAP_LEN_LEGACY;
        u4ItPresent = RADIOTAP_FIELDS_LEGACY;
    }

    /* Radiotap Header & Bit Number 30 Vendor Namespace */
    prVendorNsOffset = (u8 *)&rMonitorRadiotap + u4RadiotapLen;
    u4RadiotapLen += u4VendorNsLen;
    kalMemSet(&rMonitorRadiotap, 0, sizeof(MONITOR_RADIOTAP_T));
    kalMemCopy(prVendorNsOffset, (u8 *)&rRadiotapFieldVendor,
               u4VendorNsLen);
    rMonitorRadiotap.u2ItLen = cpu_to_le16(u4RadiotapLen);
    rMonitorRadiotap.u4ItPresent = u4ItPresent;

    /* Bit Number 0 TSFT */
    rMonitorRadiotap.u8MacTime = (prRxStatusGroup2->u4Timestamp);

    /* Bit Number 1 FLAGS */
    if (HAL_RX_STATUS_IS_FRAG(prRxStatus) == true) {
        rMonitorRadiotap.ucFlags |= BIT(3);
    }

    if (HAL_RX_STATUS_IS_FCS_ERROR(prRxStatus) == true) {
        rMonitorRadiotap.ucFlags |= BIT(6);
    }

    /* Bit Number 2 RATE */
    if ((ucRxMode == RX_VT_LEGACY_CCK) || (ucRxMode == RX_VT_LEGACY_OFDM)) {
        /* Bit[2:0] for Legacy CCK, Bit[3:0] for Legacy OFDM */
        ucRxRate = ((prRxStatusGroup3)->u4RxVector[0] & BITS(0, 3));
        rMonitorRadiotap.ucRate = nicGetHwRateByPhyRate(ucRxRate);
    } else {
        ucMcs = ((prRxStatusGroup3)->u4RxVector[0] &
                 RX_VT_RX_RATE_AC_MASK);
        /* VHTA1 B0-B1 */
        ucFrMode = (((prRxStatusGroup3)->u4RxVector[0] &
                     RX_VT_FR_MODE_MASK) >>
                    RX_VT_FR_MODE_OFFSET);
        ucShortGI =
            ((prRxStatusGroup3)->u4RxVector[0] & RX_VT_SHORT_GI) ?
            1 :
            0;  /* VHTA2 B0 */

        /* ucRate(500kbs) = u4PhyRate(100kbps) / 5, max ucRate = 0xFF */
        u4PhyRate = nicGetPhyRateByMcsRate(ucMcs, ucFrMode, ucShortGI);
        if (u4PhyRate > 1275) {
            rMonitorRadiotap.ucRate = 0xFF;
        }else{
            rMonitorRadiotap.ucRate = u4PhyRate / 5;
        }
    }

    /* Bit Number 3 CHANNEL */
    if (ucRxMode == RX_VT_LEGACY_CCK) {
        rMonitorRadiotap.u2ChFlags |= BIT(5);
    }else{  /* OFDM */
        rMonitorRadiotap.u2ChFlags |= BIT(6);
    }

    ucChanNum = HAL_RX_STATUS_GET_CHNL_NUM(prRxStatus);
    if (HAL_RX_STATUS_GET_RF_BAND(prRxStatus) == BAND_2G4) {
        rMonitorRadiotap.u2ChFlags |= BIT(7);
        rMonitorRadiotap.u2ChFrequency = (ucChanNum * 5 + 2407);
    } else {  /* BAND_5G */
        rMonitorRadiotap.u2ChFlags |= BIT(8);
        rMonitorRadiotap.u2ChFrequency = (ucChanNum * 5 + 5000);
    }

    /* Bit Number 5 ANT SIGNAL */
    rMonitorRadiotap.ucAntennaSignal =
        RCPI_TO_dBm(HAL_RX_STATUS_GET_RCPI0(prSwRfb->prRxStatusGroup3));

    /* Bit Number 6 ANT NOISE */
    rMonitorRadiotap.ucAntennaNoise =
        ((((prRxStatusGroup3)->u4RxVector[5] & RX_VT_NF0_MASK) >> 1) +
         128);

    /* Bit Number 11 ANT, Invalid for MT6632 and MT7615 */
    rMonitorRadiotap.ucAntenna =
        ((prRxStatusGroup3)->u4RxVector[2] & RX_VT_SEL_ANT) ? 1 : 0;

    /* Bit Number 19 MCS */
    if ((u4ItPresent & RADIOTAP_FIELD_MCS)) {
        nicRxFillRadiotapMCS(&rMonitorRadiotap, prRxStatusGroup3);
    }

    /* Bit Number 20 AMPDU */
    if (HAL_RX_STATUS_IS_AMPDU_SUB_FRAME(prRxStatus)) {
        if (HAL_RX_STATUS_GET_RXV_SEQ_NO(prRxStatus)) {
            ++prRxCtrl->u4AmpduRefNum;
        }
        rMonitorRadiotap.u4AmpduRefNum = prRxCtrl->u4AmpduRefNum;
    }

    /* Bit Number 21 VHT */
    if ((u4ItPresent & RADIOTAP_FIELD_VHT)) {
        nicRxFillRadiotapVHT(&rMonitorRadiotap, prRxStatusGroup3);
    }

    prSwRfb->pvHeader -= u4RadiotapLen;
    kalMemCopy(prSwRfb->pvHeader, &rMonitorRadiotap, u4RadiotapLen);

    prSkb = (struct sk_buff *)(prSwRfb->pvPacket);
    prSkb->data = (unsigned char *)(prSwRfb->pvHeader);

    skb_reset_tail_pointer(prSkb);
    skb_trim(prSkb, 0);
    skb_put(prSkb, (u4RadiotapLen + prSwRfb->u2PacketLen));

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_TO_OS_QUE);
    QUEUE_INSERT_TAIL(
        &(prAdapter->rRxQueue),
        (P_QUE_ENTRY_T)GLUE_GET_PKT_QUEUE_ENTRY(prSwRfb->pvPacket));
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_TO_OS_QUE);

    prRxCtrl->ucNumIndPacket++;
    kalSetTxEvent2Rx(prAdapter->prGlueInfo);

    prSwRfb->pvPacket = NULL;
    /* Return RFB */
    if (nicRxSetupRFB(prAdapter, prSwRfb)) {
        DBGLOG(RX, WARN, "Cannot allocate packet buffer for SwRfb!\n");
        if (!timerPendingTimer(&prAdapter->rPacketDelaySetupTimer)) {
            DBGLOG(RX, WARN,
                   "Start ReturnIndicatedRfb Timer (%u)\n",
                   RX_RETURN_INDICATED_RFB_TIMEOUT_SEC);
            cnmTimerStartTimer(
                prAdapter, &prAdapter->rPacketDelaySetupTimer,
                SEC_TO_MSEC(
                    RX_RETURN_INDICATED_RFB_TIMEOUT_SEC));
        }
    }
    nicRxReturnRFB(prAdapter, prSwRfb);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process HIF data packet
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessDataPacket(IN P_ADAPTER_T prAdapter,
                            IN OUT P_SW_RFB_T prSwRfb)
{
    P_RX_CTRL_T prRxCtrl;
    P_SW_RFB_T prRetSwRfb, prNextSwRfb;
    P_HW_MAC_RX_DESC_T prRxStatus;
    u8 fgDrop;
    struct chip_info *prChipInfo;

    DEBUGFUNC("nicRxProcessDataPacket");
    /* DBGLOG(INIT, TRACE, ("\n")); */

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    fgDrop = false;

    prRxStatus = prSwRfb->prRxStatus;
    prRxCtrl = &prAdapter->rRxCtrl;
    prChipInfo = prAdapter->chip_info;

    /* Check AMPDU_nERR_Bitmap */
    prSwRfb->fgDataFrame = true;
    prSwRfb->fgFragFrame = false;
    prSwRfb->fgReorderBuffer = false;

    DBGLOG(RSN, INFO, "StatusFlag:0x%x\n", prRxStatus->u2StatusFlag);

    /* Get RXD in the beginning */
    if (nicRxFillRFB(prAdapter, prSwRfb) == false) {
        nicRxReturnRFB(prAdapter, prSwRfb);
        RX_INC_CNT(prRxCtrl, RX_CLASS_ERR_DROP_COUNT);
        RX_INC_CNT(prRxCtrl, RX_DROP_TOTAL_COUNT);
        return;
    }

    /* BA session */
    if ((prRxStatus->u2StatusFlag & RXS_DW2_AMPDU_nERR_BITMAP) ==
        RXS_DW2_AMPDU_nERR_VALUE) {
        prSwRfb->fgReorderBuffer = true;
    }
    /* non BA session */
    else if ((prRxStatus->u2StatusFlag & RXS_DW2_RX_nERR_BITMAP) ==
             RXS_DW2_RX_nERR_VALUE) {
        if ((prRxStatus->u2StatusFlag & RXS_DW2_RX_nDATA_BITMAP) ==
            RXS_DW2_RX_nDATA_VALUE) {
            prSwRfb->fgDataFrame = false;
        }

        if ((prRxStatus->u2StatusFlag & RXS_DW2_RX_FRAG_BITMAP) ==
            RXS_DW2_RX_FRAG_VALUE) {
            prSwRfb->fgFragFrame = true;
        }
    } else if (HAL_RX_STATUS_IS_CIPHER_MISMATCH(prRxStatus)) {
        fgDrop = true;
        DBGLOG_RATELIMIT(RSN, EVENT,
                         "HAL_RX_STATUS_IS_CIPHER_MISMATCH\n");
    } else {
        fgDrop = true;

        if (HAL_RX_STATUS_IS_DE_AMSDU_FAIL(prRxStatus)) {
            DBGLOG_RATELIMIT(RSN, EVENT, "de-amsdu fail\n");
        }

        if (HAL_RX_STATUS_IS_ICV_ERROR(prRxStatus)) {
            DBGLOG_RATELIMIT(RSN, EVENT, "icv error\n");
        }

        if (!HAL_RX_STATUS_IS_ICV_ERROR(prRxStatus) &&
            HAL_RX_STATUS_IS_TKIP_MIC_ERROR(prRxStatus)) {
            P_STA_RECORD_T prStaRec;

            prStaRec = cnmGetStaRecByAddress(
                prAdapter, prAdapter->prAisBssInfo->ucBssIndex,
                prAdapter->rWlanInfo.rCurrBssId.arMacAddress);
            if (prStaRec) {
                DBGLOG_RATELIMIT(RSN, EVENT, "MIC_ERR_PKT\n");
                rsnTkipHandleMICFailure(prAdapter, prStaRec, 0);
            }
        }
#if UNIFIED_MAC_RX_FORMAT
        else if (HAL_RX_STATUS_IS_LLC_MIS(prRxStatus) &&
                 !HAL_RX_STATUS_IS_ERROR(prRxStatus) &&
                 !FEAT_SUP_LLC_VLAN_RX(prChipInfo)) {
            u16 *pu2EtherType;

            pu2EtherType = (u16 *)((u8 *)prSwRfb->pvHeader +
                                   2 * MAC_ADDR_LEN);

#if CFG_SUPPORT_AMSDU_ATTACK_DETECTION
            if (HAL_RX_STATUS_GET_PAYLOAD_FORMAT(prRxStatus) ==
                RX_PAYLOAD_FORMAT_FIRST_SUB_AMSDU) {
                DBGLOG(RX, INFO, "LLC_MIS:%d, EthType:0x%x\n",
                       HAL_RX_STATUS_IS_LLC_MIS(prRxStatus),
                       pu2EtherType);
                fgDrop = false;
            }
#endif

            /* If ethernet type is VLAN, do not drop it. Pass up to
             * driver process */
            if (prSwRfb->u2HeaderLen >= ETH_HLEN &&
                *pu2EtherType == NTOHS(ETH_P_VLAN)) {
                fgDrop = false;
            }
        }
#else
        else if (HAL_RX_STATUS_IS_LLC_MIS(prRxStatus)) {
            DBGLOG_RATELIMIT(RSN, EVENT, ("LLC_MIS_ERR\n"));
            fgDrop = false;  /* Drop after send de-auth  */
        }
#endif
    }

    /* Drop plain text during security connection */
    if (HAL_RX_STATUS_IS_CIPHER_MISMATCH(prRxStatus) &&
        (prSwRfb->fgDataFrame == true)) {
        u16 *pu2EtherType;

        pu2EtherType =
            (u16 *)((u8 *)prSwRfb->pvHeader + 2 * MAC_ADDR_LEN);

        DBGLOG_RATELIMIT(
            RSN, EVENT,
            "HAL_RX_STATUS_IS_CIPHER_MISMATCH, htr:%d, HdrLen:%d\n",
            HAL_RX_STATUS_IS_HEADER_TRAN(prRxStatus),
            HAL_RX_STATUS_GET_HEADER_LEN(prRxStatus));

        if (prSwRfb->u2HeaderLen >= ETH_HLEN &&
            (*pu2EtherType == NTOHS(ETH_P_1X))) {
            fgDrop = false;
            DBGLOG(RSN, INFO, "Don't drop eapol or wpi packet\n");
        } else {
            fgDrop = true;
            DBGLOG_RATELIMIT(
                RSN, EVENT,
                "Drop plain text during security connection\n");
        }
    }

#if CFG_SUPPORT_FRAG_ATTACK_DETECTION
    /* Drop fragmented broadcast and multicast frame */
    if ((prSwRfb->fgIsBC | prSwRfb->fgIsMC) &&
        (prSwRfb->fgFragFrame == true)) {
        fgDrop = true;
        DBGLOG(RSN, INFO, "Drop fragmented broadcast and multicast\n");
    }
#endif

#if CFG_KEY_ERROR_STATISTIC_RECOVERY
    if ((prSwRfb->fgIsBC || prSwRfb->fgIsMC) &&
        (prSwRfb->fgDataFrame == true)) {
        u8 ucBssIndex = secGetBssIdxByWlanIdx(
            prAdapter, HAL_RX_STATUS_GET_WLAN_IDX(prRxStatus));
        P_BSS_INFO_T prBssInfo =
            GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

        if (prBssInfo && IS_BSS_AIS(prBssInfo)) {
            u8 fgTriggerBCNTimeout = false;

            RX_INC_CNT(&prAdapter->rRxCtrl, RX_BMC_PKT_COUNT);

            if (HAL_RX_STATUS_IS_ICV_ERROR(prRxStatus)) {
                RX_INC_CNT(&prAdapter->rRxCtrl,
                           RX_BMC_KEY_ERROR_COUNT);

                DBGLOG_RATELIMIT(
                    RSN,
                    EVENT,
                    "BMC Data Packet(%llu) from AIS Wi-Fi interface with ICV error\n",
                    RX_GET_CNT(&prAdapter->rRxCtrl,
                               RX_BMC_KEY_ERROR_COUNT));

                if (RX_GET_CNT(&prAdapter->rRxCtrl,
                               RX_BMC_KEY_ERROR_COUNT) ==
                    prAdapter->rWifiVar.u4BmcKeyErrorTh) {
                    fgTriggerBCNTimeout = true;
                }
            } else if (HAL_RX_STATUS_IS_CIPHER_MISMATCH(
                           prRxStatus)) {
                RX_INC_CNT(&prAdapter->rRxCtrl,
                           RX_BMC_NO_KEY_COUNT);

                DBGLOG_RATELIMIT(
                    RSN,
                    EVENT,
                    "BMC Data Packet(%llu) from AIS Wi-Fi interface with Cipher Mismatch\n",
                    RX_GET_CNT(&prAdapter->rRxCtrl,
                               RX_BMC_NO_KEY_COUNT));

                if (RX_GET_CNT(&prAdapter->rRxCtrl,
                               RX_BMC_NO_KEY_COUNT) ==
                    prAdapter->rWifiVar.u4BmcKeyErrorTh) {
                    fgTriggerBCNTimeout = true;
                }
            }

            if ((fgTriggerBCNTimeout) &&
                (prAdapter->rWifiVar.u4BmcKeyErrorTh)) {
                DBGLOG(QM,
                       EVENT,
                       "Trigger BCN timeout due to RX more than\n"
                       "  %llu cipher mismatch BMC packets\n"
                       "  %llu ICV error BMC packets\n",
                       RX_GET_CNT(&prAdapter->rRxCtrl,
                                  RX_BMC_NO_KEY_COUNT),
                       RX_GET_CNT(&prAdapter->rRxCtrl,
                                  RX_BMC_KEY_ERROR_COUNT));

                prBssInfo->u2DeauthReason =
                    BEACON_TIMEOUT_REASON_DUE_2_BMC_ERR;
                kalIndicateStatusAndComplete(
                    prAdapter->prGlueInfo,
                    WLAN_STATUS_BEACON_TIMEOUT, NULL, 0);
            }
        }
    }
#endif

#if CFG_TCP_IP_CHKSUM_OFFLOAD || CFG_TCP_IP_CHKSUM_OFFLOAD_NDIS_60
    if (prAdapter->fgIsSupportCsumOffload && fgDrop == false) {
        u32 u4TcpUdpIpCksStatus;
        u32 *pu4Temp;

        pu4Temp = (u32 *)prRxStatus;
        u4TcpUdpIpCksStatus =
            *(pu4Temp + (ALIGN_4(prRxStatus->u2RxByteCount) >> 2));
        nicRxFillChksumStatus(prAdapter, prSwRfb, u4TcpUdpIpCksStatus);
    }
#endif

    /* if(secCheckClassError(prAdapter, prSwRfb, prStaRec) == true && */
    if (prAdapter->fgTestMode == false && fgDrop == false) {
#if CFG_HIF_RX_STARVATION_WARNING
        prRxCtrl->u4QueuedCnt++;
#endif
        GLUE_SET_PKT_BSS_IDX(prSwRfb->pvPacket,
                             secGetBssIdxByWlanIdx(prAdapter,
                                                   prSwRfb->ucWlanIdx));

        prRetSwRfb = qmHandleRxPackets(prAdapter, prSwRfb);
        if (prRetSwRfb != NULL) {
            do {
#if CFG_SUPPORT_MSP
                if (prRetSwRfb->ucGroupVLD &
                    BIT(RX_GROUP_VLD_3)) {
                    if (prRetSwRfb->ucStaRecIdx <
                        CFG_STA_REC_NUM) {
                        prAdapter
                        ->arStaRec[prRetSwRfb
                                   ->ucStaRecIdx]
                        .u4RxVector0 =
                            HAL_RX_VECTOR_GET_RX_VECTOR
                            (
                                prRetSwRfb
                                ->
                                prRxStatusGroup3,
                                0);

                        prAdapter
                        ->arStaRec[prRetSwRfb
                                   ->ucStaRecIdx]
                        .u4RxVector1 =
                            HAL_RX_VECTOR_GET_RX_VECTOR
                            (
                                prRetSwRfb
                                ->
                                prRxStatusGroup3,
                                1);

                        prAdapter
                        ->arStaRec[prRetSwRfb
                                   ->ucStaRecIdx]
                        .u4RxVector2 =
                            HAL_RX_VECTOR_GET_RX_VECTOR
                            (
                                prRetSwRfb
                                ->
                                prRxStatusGroup3,
                                2);

                        prAdapter
                        ->arStaRec[prRetSwRfb
                                   ->ucStaRecIdx]
                        .u4RxVector3 =
                            HAL_RX_VECTOR_GET_RX_VECTOR
                            (
                                prRetSwRfb
                                ->
                                prRxStatusGroup3,
                                3);

                        prAdapter
                        ->arStaRec[prRetSwRfb
                                   ->ucStaRecIdx]
                        .u4RxVector4 =
                            HAL_RX_VECTOR_GET_RX_VECTOR
                            (
                                prRetSwRfb
                                ->
                                prRxStatusGroup3,
                                4);
                    } else {
                        DBGLOG(RX,
                               ERROR,
                               "invalid ucStaRecIdx %d\n",
                               prRetSwRfb->ucStaRecIdx);
                    }
                }
#endif
                /* save next first */
                prNextSwRfb = (P_SW_RFB_T)QUEUE_GET_NEXT_ENTRY(
                    (P_QUE_ENTRY_T)prRetSwRfb);

                switch (prRetSwRfb->eDst) {
                case RX_PKT_DESTINATION_HOST:
                    nicRxProcessPktWithoutReorder(
                        prAdapter, prRetSwRfb);
                    break;

                case RX_PKT_DESTINATION_FORWARD:
                    nicRxProcessForwardPkt(prAdapter,
                                           prRetSwRfb);
                    break;

                case RX_PKT_DESTINATION_HOST_WITH_FORWARD:
                    nicRxProcessGOBroadcastPkt(prAdapter,
                                               prRetSwRfb);
                    break;

                case RX_PKT_DESTINATION_NULL:
                    nicRxReturnRFB(prAdapter, prRetSwRfb);
                    RX_INC_CNT(prRxCtrl,
                               RX_DST_NULL_DROP_COUNT);
                    RX_INC_CNT(prRxCtrl,
                               RX_DROP_TOTAL_COUNT);
                    break;

                default:
                    break;
                }
#if CFG_HIF_RX_STARVATION_WARNING
                prRxCtrl->u4DequeuedCnt++;
#endif
                prRetSwRfb = prNextSwRfb;
            } while (prRetSwRfb);
        }
    } else {
        nicRxReturnRFB(prAdapter, prSwRfb);
        RX_INC_CNT(prRxCtrl, RX_CLASS_ERR_DROP_COUNT);
        RX_INC_CNT(prRxCtrl, RX_DROP_TOTAL_COUNT);
    }
}

void nicRxProcessEventPacket(IN P_ADAPTER_T prAdapter,
                             IN OUT P_SW_RFB_T prSwRfb)
{
    P_CMD_INFO_T prCmdInfo;
    P_WIFI_EVENT_T prEvent;
    u32 u4Idx, u4Size;
    P_HW_MAC_RX_DESC_T prRxStatus;

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prRxStatus = prSwRfb->prRxStatus;
    ASSERT(prRxStatus);

    prSwRfb->u2PacketLen = HAL_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus);
    if (prSwRfb->u2PacketLen < sizeof(HW_MAC_RX_DESC_T)) {
        DBGLOG(RX, ERROR,
               "%s: Logic Check Error - u4PktLen(%lu) < RXD.\n",
               __func__, prSwRfb->u2PacketLen);
        return;
    }

    prEvent = (P_WIFI_EVENT_T)prSwRfb->pucRecvBuff;

    if (prEvent->ucEID != EVENT_ID_DEBUG_MSG
#if CFG_ASSERT_DUMP
        && prEvent->ucEID != EVENT_ID_ASSERT_DUMP
#endif
        ) {
        DBGLOG(NIC, INFO, "RX EVENT: ID[0x%02X] SEQ[%u] LEN[%u]\n",
               prEvent->ucEID, prEvent->ucSeqNum,
               prEvent->u2PacketLength);
    }
    if ((prEvent->u2PacketLength - EVENT_HDR_WITHOUT_RXD_SIZE) >
        prSwRfb->u2PacketLen - sizeof(WIFI_EVENT_T)) {
        DBGLOG(NIC,
               ERROR,
               "RX EVENT: ID[0x%02X] SEQ[%u], invalid payload LEN[%u] > %u\n",
               prEvent->ucEID,
               prEvent->ucSeqNum,
               prEvent->u2PacketLength - EVENT_HDR_WITHOUT_RXD_SIZE,
               prSwRfb->u2PacketLen - sizeof(WIFI_EVENT_T));
        goto done;
    }

    /* Event handler table */
    u4Size = sizeof(arEventTable) / sizeof(RX_EVENT_HANDLER_T);

    for (u4Idx = 0; u4Idx < u4Size; u4Idx++) {
        if (prEvent->ucEID == arEventTable[u4Idx].eEID) {
            arEventTable[u4Idx].pfnHandler(
                prAdapter, prEvent,
                (prEvent->u2PacketLength -
                 EVENT_HDR_WITHOUT_RXD_SIZE));
            break;
        }
    }

    /* Event cannot be found in event handler table, use default action */
    if (u4Idx >= u4Size) {
        prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

        if (prCmdInfo != NULL) {
            if (prCmdInfo->pfCmdDoneHandler) {
                prCmdInfo->pfCmdDoneHandler(
                    prAdapter, prCmdInfo,
                    prEvent->aucBuffer,
                    (prEvent->u2PacketLength -
                     EVENT_HDR_WITHOUT_RXD_SIZE));
            } else if (prCmdInfo->fgIsOid) {
                kalOidComplete(prAdapter->prGlueInfo,
                               prCmdInfo->fgSetQuery, 0,
                               WLAN_STATUS_SUCCESS);
            }

            /* return prCmdInfo */
            cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
        } else {
            DBGLOG(RX,
                   WARN,
                   "UNHANDLED RX EVENT: ID[0x%02X] SEQ[%u] LEN[%u]\n",
                   prEvent->ucEID,
                   prEvent->ucSeqNum,
                   prEvent->u2PacketLength);
        }
    }
done:
    /* Reset Chip NoAck flag */
    if (prAdapter->fgIsChipNoAck) {
        DBGLOG(RX, WARN, "Got response from chip, clear NoAck flag!\n");
        WARN_ON(true);
    }
    prAdapter->ucOidTimeoutCount = 0;
    prAdapter->fgIsChipNoAck = false;

    nicRxReturnRFB(prAdapter, prSwRfb);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief nicRxProcessMgmtPacket is used to dispatch management frames
 *        to corresponding modules
 *
 * @param prAdapter Pointer to the Adapter structure.
 * @param prSWRfb the RFB to receive rx data
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessMgmtPacket(IN P_ADAPTER_T prAdapter,
                            IN OUT P_SW_RFB_T prSwRfb)
{
    u8 ucSubtype;
#if CFG_SUPPORT_802_11W
    /* u8   fgMfgDrop = false; */
#endif
    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    if (nicRxFillRFB(prAdapter, prSwRfb) == false) {
        nicRxReturnRFB(prAdapter, prSwRfb);
        RX_INC_CNT(&prAdapter->rRxCtrl, RX_DROP_TOTAL_COUNT);
        return;
    }

    ucSubtype = (*(u8 *)(prSwRfb->pvHeader) & MASK_FC_SUBTYPE) >>
                OFFSET_OF_FC_SUBTYPE;

#if CFG_RX_PKTS_DUMP
    {
        P_WLAN_MAC_MGMT_HEADER_T prWlanMgmtHeader;
        u16 u2TxFrameCtrl;

        u2TxFrameCtrl = (*(u8 *)(prSwRfb->pvHeader) & MASK_FRAME_TYPE);
        if (prAdapter->rRxCtrl.u4RxPktsDumpTypeMask &
            BIT(HIF_RX_PKT_TYPE_MANAGEMENT)) {
            if (u2TxFrameCtrl == MAC_FRAME_BEACON ||
                u2TxFrameCtrl == MAC_FRAME_PROBE_RSP) {
                prWlanMgmtHeader =
                    (P_WLAN_MAC_MGMT_HEADER_T)(prSwRfb->
                                               pvHeader);

                DBGLOG(SW4,
                       INFO,
                       "QM RX MGT: net %u sta idx %u wlan idx %u ssn %u ptype %u subtype %u 11 %u\n",
                       prSwRfb->prStaRec->ucBssIndex,
                       prSwRfb->ucStaRecIdx,
                       prSwRfb->ucWlanIdx,
                       prWlanMgmtHeader->u2SeqCtrl,
                       /* The new SN of the frame */
                       prSwRfb->ucPacketType,
                       ucSubtype);
                /* HIF_RX_HDR_GET_80211_FLAG(prHifRxHdr))); */

                DBGLOG_MEM8(SW4, TRACE, (u8 *)prSwRfb->pvHeader,
                            prSwRfb->u2PacketLen);
            }
        }
    }
#endif
#if CFG_SUPPORT_802_11W
    if (HAL_RX_STATUS_IS_ICV_ERROR(prSwRfb->prRxStatus)) {
        if (HAL_RX_STATUS_GET_SEC_MODE(prSwRfb->prRxStatus) ==
            CIPHER_SUITE_BIP) {
            DBGLOG(RSN, INFO, "[MFP] RX with BIP ICV ERROR\n");
        }else{
            DBGLOG(RSN, INFO, "[MFP] RX with ICV ERROR\n");
        }

        nicRxReturnRFB(prAdapter, prSwRfb);
        RX_INC_CNT(&prAdapter->rRxCtrl, RX_DROP_TOTAL_COUNT);
        return;
    }
#endif

    if (prAdapter->fgTestMode == false) {
#if CFG_MGMT_FRAME_HANDLING
        if (apfnProcessRxMgtFrame[ucSubtype]) {
            switch (apfnProcessRxMgtFrame[ucSubtype](prAdapter,
                                                     prSwRfb)) {
            case WLAN_STATUS_PENDING:
                return;

            case WLAN_STATUS_SUCCESS:
            case WLAN_STATUS_FAILURE:
                break;

            default:
                DBGLOG(RX,
                       WARN,
                       "Unexpected MMPDU(0x%02X) returned with abnormal status\n",
                       ucSubtype);
                break;
            }
        }
#endif
    }

    nicRxReturnRFB(prAdapter, prSwRfb);
}

void nicRxProcessMsduReport(IN P_ADAPTER_T prAdapter,
                            IN OUT P_SW_RFB_T prSwRfb)
{
    nicRxReturnRFB(prAdapter, prSwRfb);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief nicProcessRFBs is used to process RFBs in the rReceivedRFBList queue.
 *
 * @param prAdapter Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxProcessRFBs(IN P_ADAPTER_T prAdapter)
{
    P_RX_CTRL_T prRxCtrl;
    P_SW_RFB_T prSwRfb = (P_SW_RFB_T)NULL;
    QUE_T rTempRfbList;
    P_QUE_T prTempRfbList = &rTempRfbList;
    u32 u4RxLoopCount;

    KAL_SPIN_LOCK_DECLARATION();

    DEBUGFUNC("nicRxProcessRFBs");

    ASSERT(prAdapter);

    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    prRxCtrl->ucNumIndPacket = 0;
    prRxCtrl->ucNumRetainedPacket = 0;
    u4RxLoopCount = prAdapter->rWifiVar.u4TxRxLoopCount;

    QUEUE_INITIALIZE(prTempRfbList);

    while (u4RxLoopCount--) {
        while (QUEUE_IS_NOT_EMPTY(&prRxCtrl->rReceivedRfbList)) {
            KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
            QUEUE_MOVE_ALL(prTempRfbList,
                           &prRxCtrl->rReceivedRfbList);
            KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);

            while (QUEUE_IS_NOT_EMPTY(prTempRfbList)) {
                QUEUE_REMOVE_HEAD(prTempRfbList, prSwRfb,
                                  P_SW_RFB_T);

                switch (prSwRfb->ucPacketType) {
                case RX_PKT_TYPE_RX_DATA:
#if CFG_SUPPORT_SNIFFER
                    if (prAdapter->prGlueInfo
                        ->fgIsEnableMon &&
                        HAL_IS_RX_DIRECT(prAdapter)) {
                        nicRxProcessMonitorPacket(
                            prAdapter, prSwRfb);
                        break;
                    } else if (prAdapter->prGlueInfo
                               ->fgIsEnableMon) {
                        nicRxProcessMonitorPacket(
                            prAdapter, prSwRfb);
                        break;
                    }
#endif
                    if (HAL_IS_RX_DIRECT(prAdapter)) {
                        nicRxProcessDataPacket(
                            prAdapter, prSwRfb);
                    } else {
                        nicRxProcessDataPacket(
                            prAdapter, prSwRfb);
                    }
                    break;

                case RX_PKT_TYPE_SW_DEFINED:
                    /* HIF_RX_PKT_TYPE_EVENT */
                    if ((prSwRfb->prRxStatus->u2PktTYpe &
                         RXM_RXD_PKT_TYPE_SW_BITMAP) ==
                        RXM_RXD_PKT_TYPE_SW_EVENT) {
                        nicRxProcessEventPacket(
                            prAdapter, prSwRfb);
                    }
                    /* case HIF_RX_PKT_TYPE_MANAGEMENT: */
                    else if ((prSwRfb->prRxStatus->u2PktTYpe
                              &
                              RXM_RXD_PKT_TYPE_SW_BITMAP) ==
                             RXM_RXD_PKT_TYPE_SW_FRAME) {
                        nicRxProcessMgmtPacket(
                            prAdapter, prSwRfb);
                    } else {
                        DBGLOG(RX,
                               ERROR,
                               "u2PktTYpe(0x%04X) is OUT OF DEF.!!!\n",
                               prSwRfb->prRxStatus
                               ->u2PktTYpe);
                        DBGLOG_MEM8(
                            RX, ERROR,
                            (u8 *)prSwRfb->pvHeader,
                            prSwRfb->u2PacketLen);

                        /*ASSERT(0);*/
                        nicRxReturnRFB(prAdapter,
                                       prSwRfb);
                        RX_INC_CNT(
                            prRxCtrl,
                            RX_TYPE_ERR_DROP_COUNT);
                        RX_INC_CNT(prRxCtrl,
                                   RX_DROP_TOTAL_COUNT);
                    }
                    break;

                case RX_PKT_TYPE_MSDU_REPORT:
                    nicRxProcessMsduReport(prAdapter,
                                           prSwRfb);
                    break;

                /* case HIF_RX_PKT_TYPE_TX_LOOPBACK: */
                /* case HIF_RX_PKT_TYPE_MANAGEMENT: */
                case RX_PKT_TYPE_TX_STATUS:
                case RX_PKT_TYPE_RX_VECTOR:
                case RX_PKT_TYPE_TM_REPORT:
                default:
                    nicRxReturnRFB(prAdapter, prSwRfb);
                    RX_INC_CNT(prRxCtrl,
                               RX_TYPE_ERR_DROP_COUNT);
                    RX_INC_CNT(prRxCtrl,
                               RX_DROP_TOTAL_COUNT);
                    DBGLOG(RX, ERROR, "ucPacketType = %d\n",
                           prSwRfb->ucPacketType);
                    break;
                }
            }

            if (prRxCtrl->ucNumIndPacket > 0) {
                RX_ADD_CNT(prRxCtrl, RX_DATA_INDICATION_COUNT,
                           prRxCtrl->ucNumIndPacket);
                RX_ADD_CNT(prRxCtrl, RX_DATA_RETAINED_COUNT,
                           prRxCtrl->ucNumRetainedPacket);
            }
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Setup a RFB and allocate the os packet to the RFB
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param prSwRfb        Pointer to the RFB
 *
 * @retval WLAN_STATUS_SUCCESS
 * @retval WLAN_STATUS_RESOURCES
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicRxSetupRFB(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
    void *pvPacket;
    u8 *pucRecvBuff;

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    if (!prSwRfb->pvPacket) {
        kalMemZero(prSwRfb, sizeof(SW_RFB_T));
        pvPacket = kalPacketAlloc(prAdapter->prGlueInfo,
                                  CFG_RX_MAX_PKT_SIZE, &pucRecvBuff);
        if (pvPacket == NULL) {
            return WLAN_STATUS_RESOURCES;
        }

        prSwRfb->pvPacket = pvPacket;
        prSwRfb->pucRecvBuff = (void *)pucRecvBuff;
    } else {
        kalMemZero(
            ((u8 *)prSwRfb + OFFSET_OF(SW_RFB_T, prRxStatus)),
            (sizeof(SW_RFB_T) - OFFSET_OF(SW_RFB_T, prRxStatus)));
    }

    /* ToDo: remove prHifRxHdr */
    /* prSwRfb->prHifRxHdr = (P_HIF_RX_HEADER_T)(prSwRfb->pucRecvBuff); */
    prSwRfb->prRxStatus = (P_HW_MAC_RX_DESC_T)(prSwRfb->pucRecvBuff);

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called to put a RFB back onto the "RFB with Buffer"
 * list or "RFB without buffer" list according to pvPacket.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param prSwRfb          Pointer to the RFB
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxReturnRFB(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
    P_RX_CTRL_T prRxCtrl;
    P_QUE_ENTRY_T prQueEntry;

    KAL_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);
    ASSERT(prSwRfb);
    prRxCtrl = &prAdapter->rRxCtrl;
    prQueEntry = &prSwRfb->rQueEntry;

    ASSERT(prQueEntry);

    /* The processing on this RFB is done, so put it back on the tail of our
     * list */
    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

    if (prSwRfb->pvPacket) {
        /* Zero out the packet buffer between uses */
        kalMemZero(
            ((u8 *)prSwRfb + OFFSET_OF(SW_RFB_T, prRxStatus)),
            (sizeof(SW_RFB_T) - OFFSET_OF(SW_RFB_T, prRxStatus)));
        prSwRfb->prRxStatus =
            (P_HW_MAC_RX_DESC_T)(prSwRfb->pucRecvBuff);

        /* QUEUE_INSERT_TAIL */
        QUEUE_INSERT_TAIL(&prRxCtrl->rFreeSwRfbList, prQueEntry);
    } else {
        /* QUEUE_INSERT_TAIL */
        QUEUE_INSERT_TAIL(&prRxCtrl->rIndicatedRfbList, prQueEntry);
    }
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

    /* Trigger Rx if there are free SwRfb */
    if (halIsPendingRx(prAdapter) &&
        (prRxCtrl->rFreeSwRfbList.u4NumElem > 0)) {
        kalSetIntEvent(prAdapter->prGlueInfo);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process rx interrupt. When the rx
 *        Interrupt is asserted, it means there are frames in queue.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicProcessRxInterrupt(IN P_ADAPTER_T prAdapter)
{
    ASSERT(prAdapter);

    if (nicSerIsRxStop(prAdapter)) {
        return;
    }

    halProcessRxInterrupt(prAdapter);

    set_bit(GLUE_FLAG_RX_BIT, &(prAdapter->prGlueInfo->ulFlag));
    wake_up_interruptible(&(prAdapter->prGlueInfo->waitq));

    return;
}

#if CFG_TCP_IP_CHKSUM_OFFLOAD
/*----------------------------------------------------------------------------*/
/*!
 * @brief Used to update IP/TCP/UDP checksum statistics of RX Module.
 *
 * @param prAdapter  Pointer to the Adapter structure.
 * @param aeCSUM     The array of checksum result.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxUpdateCSUMStatistics(IN P_ADAPTER_T prAdapter,
                               IN const ENUM_CSUM_RESULT_T aeCSUM[])
{
    P_RX_CTRL_T prRxCtrl;

    ASSERT(prAdapter);
    ASSERT(aeCSUM);

    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    if ((aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_SUCCESS) ||
        (aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_SUCCESS)) {
        /* count success num */
        RX_INC_CNT(prRxCtrl, RX_CSUM_IP_SUCCESS_COUNT);
    } else if ((aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_FAILED) ||
               (aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_FAILED)) {
        RX_INC_CNT(prRxCtrl, RX_CSUM_IP_FAILED_COUNT);
    } else if ((aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_NONE) &&
               (aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_NONE)) {
        RX_INC_CNT(prRxCtrl, RX_CSUM_UNKNOWN_L3_PKT_COUNT);
    } else {
        ASSERT(0);
    }

    if (aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_SUCCESS) {
        /* count success num */
        RX_INC_CNT(prRxCtrl, RX_CSUM_TCP_SUCCESS_COUNT);
    } else if (aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_FAILED) {
        RX_INC_CNT(prRxCtrl, RX_CSUM_TCP_FAILED_COUNT);
    } else if (aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_SUCCESS) {
        RX_INC_CNT(prRxCtrl, RX_CSUM_UDP_SUCCESS_COUNT);
    } else if (aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_FAILED) {
        RX_INC_CNT(prRxCtrl, RX_CSUM_UDP_FAILED_COUNT);
    } else if ((aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_NONE) &&
               (aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_NONE)) {
        RX_INC_CNT(prRxCtrl, RX_CSUM_UNKNOWN_L4_PKT_COUNT);
    } else {
        ASSERT(0);
    }
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to query current status of RX Module.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param pucBuffer      Pointer to the message buffer.
 * @param pu4Count      Pointer to the buffer of message length count.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxQueryStatus(IN P_ADAPTER_T prAdapter, IN u8 *pucBuffer,
                      OUT u32 *pu4Count)
{
    P_RX_CTRL_T prRxCtrl;
    u8 *pucCurrBuf = pucBuffer;
    u32 u4CurrCount;

    ASSERT(prAdapter);
    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    /* if (pucBuffer) {} */ /* For Windows, we'll print directly instead of
     *                         sprintf() */
    ASSERT(pu4Count);

#define SPRINTF_RX_QSTATUS(arg)                               \
    {                                                         \
        u4CurrCount =                                         \
            scnprintf(pucCurrBuf, *pu4Count, PRINTF_ARG arg); \
        pucCurrBuf += (u8)u4CurrCount;                        \
        *pu4Count -= u4CurrCount;                             \
    }

    SPRINTF_RX_QSTATUS(("\n\nRX CTRL STATUS:"));
    SPRINTF_RX_QSTATUS(("\n==============="));
    SPRINTF_RX_QSTATUS(("\nFREE RFB w/i BUF LIST :%9ld",
                        prRxCtrl->rFreeSwRfbList.u4NumElem));
    SPRINTF_RX_QSTATUS(("\nFREE RFB w/o BUF LIST :%9ld",
                        prRxCtrl->rIndicatedRfbList.u4NumElem));
    SPRINTF_RX_QSTATUS(("\nRECEIVED RFB LIST :%9ld",
                        prRxCtrl->rReceivedRfbList.u4NumElem));

    SPRINTF_RX_QSTATUS(("\n\n"));

    /**pu4Count = (u32)((u32)pucCurrBuf - (u32)pucBuffer); */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Clear RX related counters
 *
 * @param prAdapter Pointer of Adapter Data Structure
 *
 * @return - (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxClearStatistics(IN P_ADAPTER_T prAdapter)
{
    P_RX_CTRL_T prRxCtrl;

    ASSERT(prAdapter);
    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    RX_RESET_ALL_CNTS(prRxCtrl);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to query current statistics of RX Module.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param pucBuffer      Pointer to the message buffer.
 * @param pu4Count      Pointer to the buffer of message length count.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxQueryStatistics(IN P_ADAPTER_T prAdapter, IN u8 *pucBuffer,
                          OUT u32 *pu4Count)
{
    P_RX_CTRL_T prRxCtrl;
    u8 *pucCurrBuf = pucBuffer;
    u32 u4CurrCount;

    ASSERT(prAdapter);
    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    /* if (pucBuffer) {} */ /* For Windows, we'll print directly instead of
     *                         sprintf() */
    ASSERT(pu4Count);

#define SPRINTF_RX_COUNTER(eCounter)                           \
    {                                                          \
        u4CurrCount = scnprintf(                               \
            pucCurrBuf, *pu4Count, "%-30s : %ld\n", #eCounter, \
            (u32)prRxCtrl->au8Statistics[eCounter]);           \
        pucCurrBuf += (u8)u4CurrCount;                         \
        *pu4Count -= u4CurrCount;                              \
    }

    SPRINTF_RX_COUNTER(RX_MPDU_TOTAL_COUNT);
    SPRINTF_RX_COUNTER(RX_SIZE_ERR_DROP_COUNT);
    SPRINTF_RX_COUNTER(RX_DATA_INDICATION_COUNT);
    SPRINTF_RX_COUNTER(RX_DATA_RETURNED_COUNT);
    SPRINTF_RX_COUNTER(RX_DATA_RETAINED_COUNT);

#if CFG_TCP_IP_CHKSUM_OFFLOAD || CFG_TCP_IP_CHKSUM_OFFLOAD_NDIS_60
    SPRINTF_RX_COUNTER(RX_CSUM_TCP_FAILED_COUNT);
    SPRINTF_RX_COUNTER(RX_CSUM_UDP_FAILED_COUNT);
    SPRINTF_RX_COUNTER(RX_CSUM_IP_FAILED_COUNT);
    SPRINTF_RX_COUNTER(RX_CSUM_TCP_SUCCESS_COUNT);
    SPRINTF_RX_COUNTER(RX_CSUM_UDP_SUCCESS_COUNT);
    SPRINTF_RX_COUNTER(RX_CSUM_IP_SUCCESS_COUNT);
    SPRINTF_RX_COUNTER(RX_CSUM_UNKNOWN_L4_PKT_COUNT);
    SPRINTF_RX_COUNTER(RX_CSUM_UNKNOWN_L3_PKT_COUNT);
    SPRINTF_RX_COUNTER(RX_IP_V6_PKT_CCOUNT);
#endif

    /**pu4Count = (u32)(pucCurrBuf - pucBuffer); */

    nicRxClearStatistics(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Read the Response data from data port
 *
 * @param prAdapter pointer to the Adapter handler
 * @param pucRspBuffer pointer to the Response buffer
 *
 * @retval WLAN_STATUS_SUCCESS: Response packet has been read
 * @retval WLAN_STATUS_FAILURE: Read Response packet timeout or error occurred
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
nicRxWaitResponse(IN P_ADAPTER_T prAdapter, IN u8 ucPortIdx,
                  OUT u8 *pucRspBuffer, IN u32 u4MaxRespBufferLen,
                  OUT u32 *pu4Length)
{
    P_WIFI_EVENT_T prEvent;
    WLAN_STATUS u4Status = WLAN_STATUS_SUCCESS;

    u4Status = halRxWaitResponse(prAdapter, ucPortIdx, pucRspBuffer,
                                 u4MaxRespBufferLen, pu4Length);
    if (u4Status == WLAN_STATUS_SUCCESS) {
        DBGLOG(RX, TRACE, "Dump Response buffer, length = 0x%lx\n",
               *pu4Length);
        DBGLOG_MEM8(RX, TRACE, pucRspBuffer, *pu4Length);

        prEvent = (P_WIFI_EVENT_T)pucRspBuffer;
        DBGLOG(INIT, TRACE, "RX EVENT: ID[0x%02X] SEQ[%u] LEN[%u]\n",
               prEvent->ucEID, prEvent->ucSeqNum,
               prEvent->u2PacketLength);
    } else {
        DBGLOG(RX, ERROR, "halRxWaitResponse fail!\n");
    }

    return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Set filter to enable Promiscuous Mode
 *
 * @param prAdapter          Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxEnablePromiscuousMode(IN P_ADAPTER_T prAdapter)
{
    ASSERT(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Set filter to disable Promiscuous Mode
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRxDisablePromiscuousMode(IN P_ADAPTER_T prAdapter)
{
    ASSERT(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief this function flushes all packets queued in reordering module
 *
 * @param prAdapter              Pointer to the Adapter structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Flushed successfully
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicRxFlush(IN P_ADAPTER_T prAdapter)
{
    P_SW_RFB_T prSwRfb;

    ASSERT(prAdapter);
    prSwRfb = qmFlushRxQueues(prAdapter);
    if (prSwRfb != NULL) {
        do {
            P_SW_RFB_T prNextSwRfb;

            /* save next first */
            prNextSwRfb = (P_SW_RFB_T)QUEUE_GET_NEXT_ENTRY(
                (P_QUE_ENTRY_T)prSwRfb);

            /* free */
            nicRxReturnRFB(prAdapter, prSwRfb);

            prSwRfb = prNextSwRfb;
        } while (prSwRfb);
    }

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicRxProcessActionFrame(IN P_ADAPTER_T prAdapter,
                                    IN P_SW_RFB_T prSwRfb)
{
    P_WLAN_ACTION_FRAME prActFrame;
    P_BSS_INFO_T prBssInfo = NULL;
#if CFG_SUPPORT_802_11W
    u8 fgRobustAction = false;
    P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo;
#endif

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    DBGLOG(RSN, TRACE, "[Rx] nicRxProcessActionFrame\n");

    if (prSwRfb->u2PacketLen < sizeof(WLAN_ACTION_FRAME) - 1) {
        return WLAN_STATUS_INVALID_PACKET;
    }

    prActFrame = (P_WLAN_ACTION_FRAME)prSwRfb->pvHeader;

    DBGLOG(RSN, INFO, "Action frame category=%d\n", prActFrame->ucCategory);

#if CFG_SUPPORT_802_11W
    if ((prActFrame->ucCategory <=
         CATEGORY_PROTECTED_DUAL_OF_PUBLIC_ACTION &&
         prActFrame->ucCategory != CATEGORY_PUBLIC_ACTION &&
         prActFrame->ucCategory != CATEGORY_HT_ACTION)  /* At 11W spec Code 7
                                                         * is reserved */
        || (prActFrame->ucCategory ==
            CATEGORY_VENDOR_SPECIFIC_ACTION_PROTECTED)) {
        fgRobustAction = true;
    }
    /* DBGLOG(RSN, TRACE, ("[Rx] fgRobustAction=%d\n", fgRobustAction)); */

    if (fgRobustAction && prSwRfb->prStaRec &&
        GET_BSS_INFO_BY_INDEX(prAdapter, prSwRfb->prStaRec->ucBssIndex)
        ->eNetworkType == NETWORK_TYPE_AIS) {
        prAisSpecBssInfo = &(prAdapter->rWifiVar.rAisSpecificBssInfo);

        DBGLOG(RSN, INFO, "[Rx]RobustAction %x %x %x\n",
               prSwRfb->prRxStatus->u2StatusFlag,
               prSwRfb->prRxStatus->ucWlanIdx,
               prSwRfb->prRxStatus->ucTidSecMode);

        if (prAisSpecBssInfo->fgMgmtProtection &&
            (!(prActFrame->u2FrameCtrl & MASK_FC_PROTECTED_FRAME) &&
             (HAL_RX_STATUS_GET_SEC_MODE(prSwRfb->prRxStatus) ==
              CIPHER_SUITE_CCMP))) {
            DBGLOG(RSN,
                   INFO,
                   "[MFP] Not handle and drop un-protected robust action frame!!\n");
            return WLAN_STATUS_INVALID_PACKET;
        }
    }
    /* DBGLOG(RSN, INFO, "[Rx] pre check done, handle cateory %d\n",
     * prActFrame->ucCategory); */
#endif

    if (prSwRfb->prStaRec) {
        prBssInfo = GET_BSS_INFO_BY_INDEX(
            prAdapter, prSwRfb->prStaRec->ucBssIndex);
    }

    switch (prActFrame->ucCategory) {
    case CATEGORY_PUBLIC_ACTION:
        if (prAdapter->prAisBssInfo && prSwRfb->prStaRec &&
            prSwRfb->prStaRec->ucBssIndex ==
            prAdapter->prAisBssInfo->ucBssIndex) {
            aisFuncValidateRxActionFrame(prAdapter, prSwRfb);
        }

        if (prAdapter->prAisBssInfo &&
            prAdapter->prAisBssInfo->ucBssIndex ==
            KAL_NETWORK_TYPE_AIS_INDEX) {
            aisFuncValidateRxActionFrame(prAdapter, prSwRfb);
        }
#if CFG_ENABLE_WIFI_DIRECT
        if (prAdapter->fgIsP2PRegistered) {
            rlmProcessPublicAction(prAdapter, prSwRfb);
            if (prBssInfo) {
                p2pFuncValidateRxActionFrame(
                    prAdapter, prSwRfb,
                    (prBssInfo->ucBssIndex ==
                     P2P_DEV_BSS_INDEX),
                    (u8)prBssInfo->u4PrivateData);
            } else {
                p2pFuncValidateRxActionFrame(prAdapter, prSwRfb,
                                             true, 0);
            }
        }
#endif
        break;

    case CATEGORY_HT_ACTION:
        rlmProcessHtAction(prAdapter, prSwRfb);
        break;

    case CATEGORY_VENDOR_SPECIFIC_ACTION:
#if CFG_ENABLE_WIFI_DIRECT
        if (prAdapter->fgIsP2PRegistered) {
            if (prBssInfo) {
                p2pFuncValidateRxActionFrame(
                    prAdapter, prSwRfb,
                    (prBssInfo->ucBssIndex ==
                     P2P_DEV_BSS_INDEX),
                    (u8)prBssInfo->u4PrivateData);
            } else {
                p2pFuncValidateRxActionFrame(prAdapter, prSwRfb,
                                             true, 0);
            }
        }
#endif
        break;

#if CFG_SUPPORT_802_11W
    case CATEGORY_SA_QUERY_ACTION: {
        P_BSS_INFO_T prBssInfo;

        if (prSwRfb->prStaRec) {
            prBssInfo = GET_BSS_INFO_BY_INDEX(
                prAdapter, prSwRfb->prStaRec->ucBssIndex);
            ASSERT(prBssInfo);
            if ((prBssInfo->eNetworkType == NETWORK_TYPE_AIS) &&
                prAdapter->rWifiVar.rAisSpecificBssInfo
                .fgMgmtProtection  /* Use MFP */ ) {
                /* MFP test plan 5.3.3.4 */
                rsnSaQueryAction(prAdapter, prSwRfb);
            } else if ((prBssInfo->eNetworkType ==
                        NETWORK_TYPE_P2P) &&
                       (prBssInfo->eCurrentOPMode ==
                        OP_MODE_ACCESS_POINT)) {
                /* AP PMF */
                DBGLOG(RSN, INFO,
                       "[Rx] nicRx AP PMF SAQ action\n");
                if (rsnCheckBipKeyInstalled(
                        prAdapter, prSwRfb->prStaRec)) {
                    /* MFP test plan 4.3.3.4 */
                    rsnApSaQueryAction(prAdapter, prSwRfb);
                }
            }
        }
    } break;

#endif
#if CFG_SUPPORT_802_11V
    case CATEGORY_WNM_ACTION: {
        if (prSwRfb->prStaRec &&
            GET_BSS_INFO_BY_INDEX(prAdapter,
                                  prSwRfb->prStaRec->ucBssIndex)
            ->eNetworkType == NETWORK_TYPE_AIS) {
            DBGLOG(RX, INFO, "WNM action frame: %d\n", __LINE__);
            wnmWNMAction(prAdapter, prSwRfb);
        } else {
            DBGLOG(RX, INFO, "WNM action frame: %d\n", __LINE__);
        }
    } break;
#endif

#if CFG_SUPPORT_DFS
    case CATEGORY_SPEC_MGT: {
        if (prAdapter->fgEnable5GBand) {
            DBGLOG(RLM, INFO,
                   "[Channel Switch]nicRxProcessActionFrame\n");
            rlmProcessSpecMgtAction(prAdapter, prSwRfb);
        }
    } break;
#endif

#if CFG_SUPPORT_802_11AC
    case CATEGORY_VHT_ACTION:
        rlmProcessVhtAction(prAdapter, prSwRfb);
        break;
#endif

#if CFG_SUPPORT_802_11K
    case CATEGORY_RM_ACTION:
        switch (prActFrame->ucAction) {
        case RM_ACTION_RM_REQUEST:
            // rlmProcessRadioMeasurementRequest(prAdapter,
            // prSwRfb);
            break;

        case RM_ACTION_REIGHBOR_RESPONSE:
            rlmProcessNeighborReportResponse(prAdapter, prActFrame,
                                             prSwRfb->u2PacketLen);
            break;
        }
        break;

#endif
    default:
        break;
    }  /* end of switch case */

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
u8 nicRxGetRcpiValueFromRxv(IN u8 ucRcpiMode, IN P_SW_RFB_T prSwRfb)
{
    u8 ucRcpi0, ucRcpi1;
    u8 ucRcpiValue = 0;
    u8 ucRxNum;

    ASSERT(prSwRfb);

    if (ucRcpiMode >= RCPI_MODE_NUM) {
        DBGLOG(RX,
               WARN,
               "Rcpi Mode = %d is invalid for getting RCPI value from RXV\n",
               ucRcpiMode);
        return 0;
    }

    ucRcpi0 = HAL_RX_STATUS_GET_RCPI0(prSwRfb->prRxStatusGroup3);
    ucRcpi1 = HAL_RX_STATUS_GET_RCPI1(prSwRfb->prRxStatusGroup3);
    ucRxNum = HAL_RX_STATUS_GET_RX_NUM(prSwRfb->prRxStatusGroup3);

    if (ucRxNum == 0) {
        ucRcpiValue = ucRcpi0;  /*0:1R, BBP always report RCPI0 at 1R
                                 * mode*/
    } else if (ucRxNum == 1) {
        switch (ucRcpiMode) {
        case RCPI_MODE_WF0:
            ucRcpiValue = ucRcpi0;
            break;

        case RCPI_MODE_WF1:
            ucRcpiValue = ucRcpi1;
            break;

        case RCPI_MODE_WF2:
        case RCPI_MODE_WF3:
            DBGLOG(RX,
                   WARN,
                   "Rcpi Mode = %d is invalid for device with only 2 antenna\n",
                   ucRcpiMode);
            break;

        case RCPI_MODE_AVG:  /*Not recommended for CBW80+80*/
            ucRcpiValue = (ucRcpi0 + ucRcpi1) / 2;
            break;

        case RCPI_MODE_MAX:
            ucRcpiValue = (ucRcpi0 > ucRcpi1) ? (ucRcpi0) :
                          (ucRcpi1);
            break;

        case RCPI_MODE_MIN:
            ucRcpiValue = (ucRcpi0 < ucRcpi1) ? (ucRcpi0) :
                          (ucRcpi1);
            break;

        default:
            break;
        }
    } else {
        DBGLOG(RX,
               WARN,
               "RX_NUM = %d is invalid for getting RCPI value from RXV\n",
               ucRxNum);
        return 0;
    }

    return ucRcpiValue;
}
