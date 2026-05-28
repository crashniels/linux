// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "swcr.c"
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
#include "swcr.h"

#if CFG_SUPPORT_SWCR

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

u32 g_au4SwCr[SWCR_CR_NUM];  /*: 0: command other: data */

/* JB mDNS Filter*/
u32 g_u4RXFilter;  /* [31] 0: stop 1: start, [3] IPv6 [2] IPv4 */

static TIMER_T g_rSwcrDebugTimer;
static u8 g_fgSwcrDebugTimer = false;
static u32 g_u4SwcrDebugCheckTimeout;
static ENUM_SWCR_DBG_TYPE_T g_ucSwcrDebugCheckType;
static u32 g_u4SwcrDebugFrameDumpType;

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

static const PFN_CMD_RW_T g_arSwCtrlCmd[] = {
    swCtrlCmdCategory0, swCtrlCmdCategory1
#if TEST_PS
    ,
    testPsCmdCategory0, testPsCmdCategory1
#endif
#if CFG_SUPPORT_802_11V
#if (CFG_SUPPORT_802_11V_TIMING_MEASUREMENT == 1) && (WNM_UNIT_TEST == 1)
    ,
    testWNMCmdCategory0
#endif
#endif
};

const PFN_SWCR_RW_T g_arSwCrModHandle[] = { swCtrlSwCr, NULL };

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

enum {
    SWCTRL_MAGIC,
    SWCTRL_DEBUG,
    SWCTRL_WIFI_VAR,
    SWCTRL_ENABLE_INT,
    SWCTRL_DISABLE_INT,
    SWCTRL_TXM_INFO,
    SWCTRL_RXM_INFO,
    SWCTRL_DUMP_BSS,
    SWCTRL_QM_INFO,
    SWCTRL_DUMP_ALL_QUEUE_LEN,
    SWCTRL_DUMP_MEM,
    SWCTRL_TX_CTRL_INFO,
    SWCTRL_DUMP_QUEUE,
    SWCTRL_DUMP_QM_DBG_CNT,
    SWCTRL_QM_DBG_CNT,
    SWCTRL_RX_PKTS_DUMP,
    SWCTRL_RX_FILTER,
#if CFG_INIT_ENABLE_PATTERN_FILTER_ARP
    SWCTRL_RX_ARP_OFFLOAD,
#endif
    SWCTRL_PS_DTIM_SKIP,
    SWCTRL_ROAMING,
    SWCTRL_CATA0_INDEX_NUM
};

enum {
    SWCTRL_STA_INFO,
    SWCTRL_DUMP_STA,
    SWCTRL_STA_QUE_INFO,
    SWCTRL_CATA1_INDEX_NUM
};

/* JB mDNS Filter*/
#define RX_FILTER_START    (1 << 31)
#define RX_FILTER_IPV4     (1 << 2)
#define RX_FILTER_IPV6     (1 << 3)
typedef enum _ENUM_SWCR_RX_FILTER_CMD_T {
    SWCR_RX_FILTER_CMD_STOP = 0,
    SWCR_RX_FILTER_CMD_START,
    SWCR_RX_FILTER_CMD_ADD,
    SWCR_RX_FILTER_CMD_REMOVE,
    SWCR_RX_FILTER_NUM
} ENUM_SWCR_RX_FILTER_CMD_T;

#if TEST_PS
enum {
    TEST_PS_MAGIC,
    TEST_PS_SETUP_BSS,
    TEST_PS_ENABLE_BEACON,
    TEST_PS_TRIGGER_BMC,
    TEST_PS_SEND_NULL,
    TEST_PS_BUFFER_BMC,
    TEST_PS_UPDATE_BEACON,
    TEST_PS_CATA0_INDEX_NUM
};

enum {
    TEST_PS_STA_PS,
    TEST_PS_STA_ENTER_PS,
    TEST_PS_STA_EXIT_PS,
    TEST_PS_STA_TRIGGER_PSPOLL,
    TEST_PS_STA_TRIGGER_FRAME,
    TEST_PS_CATA1_INDEX_NUM
};
#endif

#if CFG_SUPPORT_802_11V
#if WNM_UNIT_TEST
enum {
    TEST_WNM_TIMING_MEAS,
    TEST_WNM_CATA0_INDEX_NUM
};
#endif
#endif

#define _SWCTRL_MAGIC    0x66201642

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
void dumpQueue(P_ADAPTER_T prAdapter)
{
    P_TX_CTRL_T prTxCtrl;
    P_QUE_MGT_T prQM;
    P_GLUE_INFO_T prGlueInfo;
    u32 i;
    u32 j;

    DEBUGFUNC("dumpQueue");

    prTxCtrl = &prAdapter->rTxCtrl;
    prQM = &prAdapter->rQM;
    prGlueInfo = prAdapter->prGlueInfo;
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
    for (i = TC0_INDEX; i <= TC5_INDEX; i++) {
        DBGLOG(SW4, INFO, "TC %u\n", i);
        DBGLOG(SW4, INFO, "Max %u Free %u\n",
               prTxCtrl->rTc.au4MaxNumOfBuffer[i],
               prTxCtrl->rTc.au4FreeBufferCount[i]);

        DBGLOG(SW4,
               INFO,
               "Average %u minReserved %u CurrentTcResource %u GuaranteedTcResource %u\n",
               QM_GET_TX_QUEUE_LEN(prAdapter, i),
               prQM->au4MinReservedTcResource[i],
               prQM->au4CurrentTcResource[i],
               prQM->au4GuaranteedTcResource[i]);
    }
#endif

#if QM_FORWARDING_FAIRNESS
    for (i = 0; i < NUM_OF_PER_STA_TX_QUEUES; i++) {
        DBGLOG(SW4, INFO, "TC %u HeadStaIdx %u ForwardCount %u\n", i,
               prQM->au4HeadStaRecIndex[i],
               prQM->au4ResourceUsedCount[i]);
    }
#endif

    DBGLOG(SW4, INFO, "BMC or unknown TxQueue Len %u\n",
           prQM->arTxQueue[0].u4NumElem);
    DBGLOG(SW4, INFO, "Pending %d\n", prGlueInfo->i4TxPendingFrameNum);
    DBGLOG(SW4, INFO, "Pending Security %d\n",
           prGlueInfo->i4TxPendingSecurityFrameNum);

    for (i = 0; i < 4; i++) {
        for (j = 0; j < CFG_MAX_TXQ_NUM; j++)
            DBGLOG(SW4, INFO, "Pending Q[%u][%u] %d\n", i, j,
                   prGlueInfo->ai4TxPendingFrameNumPerQueue[i][j]);
    }

    DBGLOG(SW4, INFO, " rFreeSwRfbList %u\n",
           prAdapter->rRxCtrl.rFreeSwRfbList.u4NumElem);
    DBGLOG(SW4, INFO, " rReceivedRfbList %u\n",
           prAdapter->rRxCtrl.rReceivedRfbList.u4NumElem);
    DBGLOG(SW4, INFO, " rIndicatedRfbList %u\n",
           prAdapter->rRxCtrl.rIndicatedRfbList.u4NumElem);
    DBGLOG(SW4, INFO, " ucNumIndPacket %u\n",
           prAdapter->rRxCtrl.ucNumIndPacket);
    DBGLOG(SW4, INFO, " ucNumRetainedPacket %u\n",
           prAdapter->rRxCtrl.ucNumRetainedPacket);
}

void dumpSTA(P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec)
{
    u8 ucWTEntry;
    u32 i;
    P_BSS_INFO_T prBssInfo;

    DEBUGFUNC("dumpSTA");

    ASSERT(prStaRec);
    ucWTEntry = prStaRec->ucWlanIndex;

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
    ASSERT(prBssInfo);

    DBGLOG(SW4, INFO, "Mac address: " MACSTR " Rcpi %u\n",
           MAC2STR(prStaRec->aucMacAddr), prStaRec->ucRCPI);

    DBGLOG(SW4,
           INFO,
           "Idx %u Wtbl %u Used %u State %u Bss Phy 0x%x Sta DesiredPhy 0x%x\n",
           prStaRec->ucIndex,
           ucWTEntry,
           prStaRec->fgIsInUse,
           prStaRec->ucStaState,
           prBssInfo->ucPhyTypeSet,
           prStaRec->ucDesiredPhyTypeSet);

    DBGLOG(SW4,
           INFO,
           "Sta Operation 0x%x  DesiredNontHtRateSet  0x%x Mcs 0x%x u2HtCapInfo 0x%x\n",
           prStaRec->u2OperationalRateSet,
           prStaRec->u2DesiredNonHTRateSet,
           prStaRec->ucMcsSet,
           prStaRec->u2HtCapInfo);

    for (i = 0; i < NUM_OF_PER_STA_TX_QUEUES; i++)
        if (prStaRec->aprTargetQueue[i]) {
            DBGLOG(SW4, INFO, "TC %u Queue Len %u\n", i,
                   prStaRec->aprTargetQueue[i]->u4NumElem);
        }

    DBGLOG(SW4, INFO, "BmpDeliveryAC %x\n", prStaRec->ucBmpDeliveryAC);
    DBGLOG(SW4, INFO, "BmpTriggerAC  %x\n", prStaRec->ucBmpTriggerAC);
    DBGLOG(SW4, INFO, "UapsdSpSupproted  %u\n",
           prStaRec->fgIsUapsdSupported);
    DBGLOG(SW4, INFO, "IsQoS  %u\n", prStaRec->fgIsQoS);
    DBGLOG(SW4, INFO, "AssocId %u\n", prStaRec->u2AssocId);

    DBGLOG(SW4, INFO, "fgIsInPS %u\n", prStaRec->fgIsInPS);
    DBGLOG(SW4, INFO, "ucFreeQuota %u\n", prStaRec->ucFreeQuota);
    DBGLOG(SW4, INFO, "ucFreeQuotaForDelivery %u\n",
           prStaRec->ucFreeQuotaForDelivery);
    DBGLOG(SW4, INFO, "ucFreeQuotaForNonDelivery %u\n",
           prStaRec->ucFreeQuotaForNonDelivery);

    for (i = 0; i < CFG_RX_MAX_BA_TID_NUM; i++) {
        if (prStaRec->aprRxReorderParamRefTbl[i]) {
            DBGLOG(SW4, INFO, "RxReorder fgIsValid: %u\n",
                   prStaRec->aprRxReorderParamRefTbl[i]->fgIsValid);
            DBGLOG(SW4, INFO, "RxReorder Tid: %u\n",
                   prStaRec->aprRxReorderParamRefTbl[i]->ucTid);
            DBGLOG(SW4, INFO, "RxReorder rReOrderQue Len: %u\n",
                   prStaRec->aprRxReorderParamRefTbl[i]
                   ->rReOrderQue.u4NumElem);
            DBGLOG(SW4, INFO, "RxReorder WinStart: %u\n",
                   prStaRec->aprRxReorderParamRefTbl[i]->u2WinStart);
            DBGLOG(SW4, INFO, "RxReorder WinEnd: %u\n",
                   prStaRec->aprRxReorderParamRefTbl[i]->u2WinEnd);
            DBGLOG(SW4, INFO, "RxReorder WinSize: %u\n",
                   prStaRec->aprRxReorderParamRefTbl[i]->u2WinSize);
        }
    }
}

void dumpBss(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo)
{
    DBGLOG(SW4, INFO, "SSID %s\n", prBssInfo->aucSSID);
    DBGLOG(SW4, INFO, "OWN " MACSTR "\n",
           MAC2STR(prBssInfo->aucOwnMacAddr));
    DBGLOG(SW4, INFO, "BSSID " MACSTR "\n", MAC2STR(prBssInfo->aucBSSID));
    DBGLOG(SW4, INFO, "eNetworkType %u\n", prBssInfo->eNetworkType);
    DBGLOG(SW4, INFO, "ucBssIndex %u\n", prBssInfo->ucBssIndex);
    DBGLOG(SW4, INFO, "eConnectionState %u\n", prBssInfo->eConnectionState);
    DBGLOG(SW4, INFO, "eCurrentOPMode %u\n", prBssInfo->eCurrentOPMode);
    DBGLOG(SW4, INFO, "fgIsQBSS %u\n", prBssInfo->fgIsQBSS);
    DBGLOG(SW4, INFO, "fgIsShortPreambleAllowed %u\n",
           prBssInfo->fgIsShortPreambleAllowed);
    DBGLOG(SW4, INFO, "fgUseShortPreamble %u\n",
           prBssInfo->fgUseShortPreamble);
    DBGLOG(SW4, INFO, "fgUseShortSlotTime %u\n",
           prBssInfo->fgUseShortSlotTime);
    DBGLOG(SW4, INFO, "ucNonHTBasicPhyType %x\n",
           prBssInfo->ucNonHTBasicPhyType);
    DBGLOG(SW4, INFO, "u2OperationalRateSet %x\n",
           prBssInfo->u2OperationalRateSet);
    DBGLOG(SW4, INFO, "u2BSSBasicRateSet %x\n",
           prBssInfo->u2BSSBasicRateSet);
    DBGLOG(SW4, INFO, "ucPhyTypeSet %x\n", prBssInfo->ucPhyTypeSet);
    DBGLOG(SW4, INFO, "rStaRecOfClientList %d\n",
           prBssInfo->rStaRecOfClientList.u4NumElem);
    DBGLOG(SW4, INFO, "u2CapInfo %x\n", prBssInfo->u2CapInfo);
    DBGLOG(SW4, INFO, "u2ATIMWindow %x\n", prBssInfo->u2ATIMWindow);
    DBGLOG(SW4, INFO, "u2AssocId %x\n", prBssInfo->u2AssocId);
    DBGLOG(SW4, INFO, "ucDTIMPeriod %x\n", prBssInfo->ucDTIMPeriod);
    DBGLOG(SW4, INFO, "ucDTIMCount %x\n", prBssInfo->ucDTIMCount);
    DBGLOG(SW4, INFO, "fgIsNetAbsent %x\n", prBssInfo->fgIsNetAbsent);
    DBGLOG(SW4, INFO, "eBand %d\n", prBssInfo->eBand);
    DBGLOG(SW4, INFO, "ucPrimaryChannel %d\n", prBssInfo->ucPrimaryChannel);
    DBGLOG(SW4, INFO, "ucHtOpInfo1 %d\n", prBssInfo->ucHtOpInfo1);
    DBGLOG(SW4, INFO, "ucHtOpInfo2 %d\n", prBssInfo->u2HtOpInfo2);
    DBGLOG(SW4, INFO, "ucHtOpInfo3 %d\n", prBssInfo->u2HtOpInfo3);
    DBGLOG(SW4, INFO, "fgErpProtectMode %d\n", prBssInfo->fgErpProtectMode);
    DBGLOG(SW4, INFO, "eHtProtectMode %d\n", prBssInfo->eHtProtectMode);
    DBGLOG(SW4, INFO, "eGfOperationMode %d\n", prBssInfo->eGfOperationMode);
    DBGLOG(SW4, INFO, "eRifsOperationMode %d\n",
           prBssInfo->eRifsOperationMode);
    DBGLOG(SW4, INFO, "fgObssErpProtectMode %d\n",
           prBssInfo->fgObssErpProtectMode);
    DBGLOG(SW4, INFO, "eObssHtProtectMode %d\n",
           prBssInfo->eObssHtProtectMode);
    DBGLOG(SW4, INFO, "eObssGfProtectMode %d\n",
           prBssInfo->eObssGfOperationMode);
    DBGLOG(SW4, INFO, "fgObssRifsOperationMode %d\n",
           prBssInfo->fgObssRifsOperationMode);
    DBGLOG(SW4, INFO, "fgAssoc40mBwAllowed %d\n",
           prBssInfo->fgAssoc40mBwAllowed);
    DBGLOG(SW4, INFO, "fg40mBwAllowed %d\n", prBssInfo->fg40mBwAllowed);
    DBGLOG(SW4, INFO, "eBssSCO %d\n", prBssInfo->eBssSCO);
}

void swCtrlCmdCategory0(P_ADAPTER_T prAdapter, u8 ucCate, u8 ucAction,
                        u8 ucOpt0, u8 ucOpt1)
{
    u8 ucIndex, ucRead;
    u32 i;
    CMD_RX_PACKET_FILTER rSetRxPacketFilter;

    DEBUGFUNC("swCtrlCmdCategory0");

    SWCR_GET_RW_INDEX(ucAction, ucRead, ucIndex);

    i = 0;

    if (ucIndex >= SWCTRL_CATA0_INDEX_NUM) {
        return;
    }

    if (ucRead == SWCR_WRITE) {
        switch (ucIndex) {
        case SWCTRL_DEBUG:
            break;

        case SWCTRL_WIFI_VAR:
            break;

#if QM_DEBUG_COUNTER
        case SWCTRL_DUMP_QM_DBG_CNT:
            for (i = 0; i < QM_DBG_CNT_NUM; i++)
                prAdapter->rQM.au4QmDebugCounters[i] = 0;
            break;

        case SWCTRL_QM_DBG_CNT:
            if (ucOpt0 >= QM_DBG_CNT_NUM) {
                DBGLOG(INIT,
                       ERROR,
                       "%s-SWCTRL_QM_DBG_CNT: ucOpt0:%d out of bound\n",
                       __func__,
                       ucOpt0);
                return;
            }
            prAdapter->rQM.au4QmDebugCounters[ucOpt0] =
                g_au4SwCr[1];
            break;

#endif
#if CFG_RX_PKTS_DUMP
        case SWCTRL_RX_PKTS_DUMP:
            /* DBGLOG(SW4, INFO,("SWCTRL_RX_PKTS_DUMP: mask %x\n",
             * g_au4SwCr[1])); */
            prAdapter->rRxCtrl.u4RxPktsDumpTypeMask = g_au4SwCr[1];
            break;

#endif
        case SWCTRL_RX_FILTER: {
            u32 u4rxfilter;
            u8 fgUpdate = false;
            WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;

            if (ucOpt0 == SWCR_RX_FILTER_CMD_STOP) {
                g_u4RXFilter &= ~(RX_FILTER_START);
                prAdapter->u4OsPacketFilter &=
                    ~PARAM_PACKET_FILTER_MULTICAST;
                u4rxfilter = prAdapter->u4OsPacketFilter;
                fgUpdate = true;
            } else if (ucOpt0 == SWCR_RX_FILTER_CMD_START) {
                g_u4RXFilter |= (RX_FILTER_START);

                if ((g_u4RXFilter & RX_FILTER_IPV4) ||
                    (g_u4RXFilter & RX_FILTER_IPV6)) {
                    prAdapter->u4OsPacketFilter |=
                        PARAM_PACKET_FILTER_MULTICAST;
                }
                u4rxfilter = prAdapter->u4OsPacketFilter;
                fgUpdate = true;
            } else if (ucOpt0 == SWCR_RX_FILTER_CMD_ADD) {
                if (ucOpt1 < 31) {
                    g_u4RXFilter |= (1 << ucOpt1);
                }
            } else if (ucOpt0 == SWCR_RX_FILTER_CMD_REMOVE) {
                if (ucOpt1 < 31) {
                    g_u4RXFilter &= ~(1 << ucOpt1);
                }
            }

            if (fgUpdate == true) {
                kalMemZero(&rSetRxPacketFilter,
                           sizeof(rSetRxPacketFilter));
                rSetRxPacketFilter.u4RxPacketFilter =
                    u4rxfilter;
                rStatus = wlanSendSetQueryCmd(
                    prAdapter,  /* prAdapter */
                    CMD_ID_SET_RX_FILTER,  /* ucCID */
                    true,  /* fgSetQuery */
                    false,  /* fgNeedResp */
                    false,  /* fgIsOid */
                    NULL,  /* pfCmdDoneHandler */
                    NULL,  /* pfCmdTimeoutHandler */
                    sizeof(CMD_RX_PACKET_FILTER),  /*u4SetQueryInfoLen*/
                    (u8 *)&rSetRxPacketFilter,  /*pucInfoBuffer*/
                    NULL,  /* pvSetQueryBuffer */
                    0  /* un4SetQueryBufferLen */
                    );
                u4rxfilter =
                    rSetRxPacketFilter.u4RxPacketFilter;
            }
            /* DBGLOG(SW4, INFO,("SWCTRL_RX_FILTER:
             * g_u4RXFilter %x ucOpt0 %x ucOpt1 %x fgUpdate %x
             * u4rxfilter %x, rStatus %x\n",
             */
            /* g_u4RXFilter, ucOpt0, ucOpt1, fgUpdate, u4rxfilter,
             * rStatus)); */
        } break;

#if CFG_INIT_ENABLE_PATTERN_FILTER_ARP
        case SWCTRL_RX_ARP_OFFLOAD: {
            WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
            u32 u4SetInfoLen = 0;
            u32 u4Len = OFFSET_OF(PARAM_NETWORK_ADDRESS_LIST,
                                  arAddress);
            u32 u4NumIPv4 = 0, u4NumIPv6 = 0;
            u32 i = 0;
            u8 *pucBufIpAddr = NULL;
            P_PARAM_NETWORK_ADDRESS_LIST prParamNetAddrList = NULL;
            P_PARAM_NETWORK_ADDRESS_IP prParamIpAddr = NULL;
            u8 *pucIp = NULL;
            /* u8 *                         pucIpv6 = NULL; */
            u32 bufSize =
                u4Len +
                (OFFSET_OF(PARAM_NETWORK_ADDRESS, aucAddress) +
                 sizeof(PARAM_NETWORK_ADDRESS_IP)) *
                3;
            P_PARAM_NETWORK_ADDRESS prParamNetAddr = NULL;

            /* <1> allocate IP address buffer */
            pucBufIpAddr = kalMemAlloc(bufSize, VIR_MEM_TYPE);
            pucIp = kalMemAlloc(3 * 4, VIR_MEM_TYPE);  /* TODO:
                                                        * replace 3
                                                        * to macro */

            prParamNetAddrList =
                (P_PARAM_NETWORK_ADDRESS_LIST)pucBufIpAddr;
            prParamNetAddr = prParamNetAddrList->arAddress;
            /* <2> clear IP address buffer */
            kalMemZero(pucBufIpAddr, bufSize);
            kalMemZero(pucIp, 3 * 4);

            /* <3> setup the number of IP address */
            if (ucOpt1 == 1) {
                if (wlanGetIPV4Address(prAdapter->prGlueInfo,
                                       pucIp, &u4NumIPv4) &&
                    u4NumIPv4 > 3) {  /* TODO: repleace 3
                                       * to macro */
                    u4NumIPv4 = 3;
                }
            } else if (ucOpt1 == 0) {
                u4NumIPv4 = u4NumIPv6 = 0;
            }
            DBGLOG(INIT, INFO, "u4Len:%d bufSize:%d u4NumIPv4:%d\n",
                   u4Len, bufSize, u4NumIPv4);

            prParamNetAddrList->u4AddressCount =
                u4NumIPv6 + u4NumIPv4;
            prParamNetAddrList->u2AddressType =
                PARAM_PROTOCOL_ID_TCP_IP;

            for (i = 0; i < u4NumIPv4; i++) {
                prParamNetAddr->u2AddressLength =
                    sizeof(PARAM_NETWORK_ADDRESS_IP);
                prParamNetAddr->u2AddressType =
                    PARAM_PROTOCOL_ID_TCP_IP;
                prParamIpAddr =
                    (P_PARAM_NETWORK_ADDRESS_IP)
                    prParamNetAddr->aucAddress;
                kalMemCopy(&prParamIpAddr->in_addr,
                           pucIp + (i * 4), 4);
                prParamNetAddr =
                    (P_PARAM_NETWORK_ADDRESS)((u32)
                                              prParamNetAddr
                                              +
                                              OFFSET_OF(
                                                  PARAM_NETWORK_ADDRESS,
                                                  aucAddress)
                                              +
                                              sizeof(
                                                  PARAM_NETWORK_ADDRESS_IP));
                u4Len += OFFSET_OF(PARAM_NETWORK_ADDRESS,
                                   aucAddress) +
                         sizeof(PARAM_NETWORK_ADDRESS_IP);
            }

            ASSERT(u4Len <= bufSize);

            rStatus = wlanoidSetNetworkAddress(
                prAdapter, (void *)prParamNetAddrList, u4Len,
                &u4SetInfoLen);

            if (rStatus != WLAN_STATUS_SUCCESS) {
                DBGLOG(INIT, INFO,
                       "set HW packet filter fail 0x%1x\n",
                       rStatus);
            }

            if (pucIp) {
                kalMemFree(pucIp, VIR_MEM_TYPE, 3 * 4);  /* TODO:
                                                          * replace
                                                          * 3 to
                                                          * marco
                                                          */
            }
            if (pucBufIpAddr) {
                kalMemFree(pucBufIpAddr, VIR_MEM_TYPE, bufSize);
            }
        } break;

#endif
        case SWCTRL_PS_DTIM_SKIP:
            break;

        case SWCTRL_ROAMING:
            break;

        default:
            break;
        }
    } else {
        switch (ucIndex) {
        case SWCTRL_DEBUG:
            break;

        case SWCTRL_MAGIC:
            g_au4SwCr[1] = _SWCTRL_MAGIC;
            break;

        case SWCTRL_QM_INFO: {
            P_QUE_MGT_T prQM = &prAdapter->rQM;

            switch (ucOpt0) {
            case 0:
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
                if (ucOpt1 >= TC_NUM) {
                    DBGLOG(SW4,
                           WARN,
                           "%s-SWCTRL_QM_INFO(0): ucOpt1:%d out of bound\n",
                           __func__,
                           ucOpt1);
                    return;
                }
                g_au4SwCr[1] = (QM_GET_TX_QUEUE_LEN(prAdapter,
                                                    ucOpt1));
                g_au4SwCr[2] =
                    prQM->au4MinReservedTcResource[ucOpt1];
                g_au4SwCr[3] =
                    prQM->au4CurrentTcResource[ucOpt1];
                g_au4SwCr[4] =
                    prQM->au4GuaranteedTcResource[ucOpt1];
#endif
                break;

            case 1:
#if QM_FORWARDING_FAIRNESS
#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
                if (ucOpt1 >= TC_NUM)
#else
                if (ucOpt1 >= NUM_OF_PER_STA_TX_QUEUES)
#endif
                {
                    DBGLOG(SW4,
                           WARN,
                           "%s-SWCTRL_QM_INFO(1): ucOpt1:%d out of bound\n",
                           __func__,
                           ucOpt1);
                    return;
                }
                g_au4SwCr[1] =
                    prQM->au4ResourceUsedCount[ucOpt1];
                g_au4SwCr[2] = prQM->au4HeadStaRecIndex[ucOpt1];
#endif
                break;

            case 2:
                if (ucOpt1 >= NUM_OF_PER_TYPE_TX_QUEUES) {
                    DBGLOG(SW4,
                           WARN,
                           "%s-SWCTRL_QM_INFO(2): ucOpt1:%d out of bound\n",
                           __func__,
                           ucOpt1);
                    return;
                }
                g_au4SwCr[1] = prQM->arTxQueue[ucOpt1]
                               .u4NumElem;  /* only
                                             * one
                                             */

                break;
            }
        } break;

        case SWCTRL_TX_CTRL_INFO: {
            P_TX_CTRL_T prTxCtrl;

            prTxCtrl = &prAdapter->rTxCtrl;
            switch (ucOpt0) {
            case 0:
                if (ucOpt1 >= TC_NUM) {
                    DBGLOG(SW4,
                           WARN,
                           "%s-SWCTRL_TX_CTRL_INFO(0): ucOpt1:%d out of bound\n",
                           __func__,
                           ucOpt1);
                    return;
                }
                g_au4SwCr[1] =
                    prAdapter->rTxCtrl.rTc
                    .au4FreeBufferCount[ucOpt1];
                g_au4SwCr[2] =
                    prAdapter->rTxCtrl.rTc
                    .au4MaxNumOfBuffer[ucOpt1];
                break;
            }
        } break;

        case SWCTRL_DUMP_QUEUE:
            dumpQueue(prAdapter);

            break;

#if QM_DEBUG_COUNTER
        case SWCTRL_DUMP_QM_DBG_CNT:
            for (i = 0; i < QM_DBG_CNT_NUM; i++)
                DBGLOG(SW4, INFO, "QM:DBG %u %u\n", i,
                       prAdapter->rQM.au4QmDebugCounters[i]);
            break;

        case SWCTRL_QM_DBG_CNT:
            if (ucOpt0 >= QM_DBG_CNT_NUM) {
                DBGLOG(INIT,
                       ERROR,
                       "%s-SWCTRL_QM_DBG_CNT: ucOpt0:%d out of bound\n",
                       __func__,
                       ucOpt0);
                return;
            }
            g_au4SwCr[1] =
                prAdapter->rQM.au4QmDebugCounters[ucOpt0];
            break;

#endif
        case SWCTRL_DUMP_BSS: {
            if (ucOpt0 >= (HW_BSSID_NUM + 1)) {
                DBGLOG(INIT,
                       ERROR,
                       "%s-SWCTRL_DUMP_BSS: ucOpt0:%d out of bound\n",
                       __func__,
                       ucOpt0);
                return;
            }
            dumpBss(prAdapter,
                    GET_BSS_INFO_BY_INDEX(prAdapter, ucOpt0));
        } break;

        default:
            break;
        }
    }
}

void swCtrlCmdCategory1(P_ADAPTER_T prAdapter, u8 ucCate, u8 ucAction,
                        u8 ucOpt0, u8 ucOpt1)
{
    u8 ucIndex, ucRead;
    u8 ucWTEntry;
    P_STA_RECORD_T prStaRec;

    DEBUGFUNC("swCtrlCmdCategory1");

    SWCR_GET_RW_INDEX(ucAction, ucRead, ucIndex);

    if (ucOpt0 >= CFG_STA_REC_NUM) {
        return;
    }

    /* prStaRec = cnmGetStaRecByIndex (prAdapter, ucOpt0); */
    prStaRec = &prAdapter->arStaRec[ucOpt0];
    ucWTEntry = prStaRec->ucWlanIndex;
    if (ucRead == SWCR_WRITE) {
        /* ToDo:: Nothing */
    } else {
        /* Read */
        switch (ucIndex) {
        case SWCTRL_STA_QUE_INFO: {
            if (ucOpt1 >= NUM_OF_PER_STA_TX_QUEUES) {
                DBGLOG(SW4,
                       WARN,
                       "%s-SWCTRL_STA_QUE_INFO: ucOpt1:%d out of bound\n",
                       __func__,
                       ucOpt1);
                return;
            }
            g_au4SwCr[1] = prStaRec->arTxQueue[ucOpt1].u4NumElem;
        } break;

        case SWCTRL_STA_INFO:
            switch (ucOpt1) {
            case 0:
                g_au4SwCr[1] = prStaRec->fgIsInPS;
                break;
            }

            break;

        case SWCTRL_DUMP_STA: {
            dumpSTA(prAdapter, prStaRec);
        } break;

        default:

            break;
        }
    }
}

#if TEST_PS

void testPsSendQoSNullFrame(IN P_ADAPTER_T prAdapter,
                            IN P_STA_RECORD_T prStaRec, IN u8 ucUP,
                            IN u8 ucBssIndex, IN u8 fgBMC, IN u8 fgIsBurstEnd,
                            IN u8 ucPacketType, IN u8 ucPsSessionID,
                            IN u8 fgSetEOSP)
{
    P_MSDU_INFO_T prMsduInfo;
    u16 u2EstimatedFrameLen;
    P_WLAN_MAC_HEADER_QOS_T prQoSNullFrame;

    DEBUGFUNC("testPsSendQoSNullFrame");
    DBGLOG(SW4, LOUD, "\n");

    /* 4 <1> Allocate a PKT_INFO_T for Null Frame */
    /* Init with MGMT Header Length */
    u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD + WLAN_MAC_HEADER_QOS_LEN;

    /* Allocate a MSDU_INFO_T */

    prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);

    if (prMsduInfo == NULL) {
        DBGLOG(SW4, WARN, "No PKT_INFO_T for sending Null Frame.\n");
        return;
    }
    /* 4 <2> Compose Null frame in MSDU_INfO_T. */
    bssComposeQoSNullFrame(prAdapter,
                           (u8 *)((unsigned long)(prMsduInfo->prPacket) +
                                  MAC_TX_RESERVED_FIELD),
                           prStaRec, ucUP, fgSetEOSP);

    TX_SET_MMPDU(prAdapter, prMsduInfo, ucBssIndex, prStaRec->ucIndex,
                 WLAN_MAC_HEADER_QOS_LEN, WLAN_MAC_HEADER_QOS_LEN, NULL,
                 MSDU_RATE_MODE_AUTO);

    prMsduInfo->ucUserPriority = ucUP;
    prMsduInfo->ucPacketType = ucPacketType;

    prQoSNullFrame = (P_WLAN_MAC_HEADER_QOS_T)((
                                                   u8 *)((unsigned long)
                                                         (prMsduInfo->
                                                          prPacket) +
                                                         MAC_TX_RESERVED_FIELD));

    if (fgBMC) {
        prQoSNullFrame->aucAddr1[0] = 0xfd;
    }else{
        prQoSNullFrame->aucAddr1[5] = 0xdd;
    }

    /* 4 <4> Inform TXM  to send this Null frame. */
    nicTxEnqueueMsdu(prAdapter, prMsduInfo);
}

void testPsSetupBss(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex)
{
    P_BSS_INFO_T prBssInfo;
    u8 _aucZeroMacAddr[] = NULL_MAC_ADDR;

    DEBUGFUNC("testPsSetupBss()");
    DBGLOG(SW4, INFO, "index %d\n", ucBssIndex);

    if (!IS_BSS_INDEX_VALID(ucBssIndex)) {
        DBGLOG(RLM, ERROR, "Invalid bssidx:%d\n", ucBssIndex);
        return;
    }

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    /* 4 <1.2> Initiate PWR STATE */
    /* SET_NET_PWR_STATE_IDLE(prAdapter, ucNetworkTypeIndex); */

    /* 4 <2> Initiate BSS_INFO_T - common part */
    BSS_INFO_INIT(prAdapter, prBssInfo);

    prBssInfo->eConnectionState = PARAM_MEDIA_STATE_DISCONNECTED;
    prBssInfo->eConnectionStateIndicated = PARAM_MEDIA_STATE_DISCONNECTED;
    prBssInfo->eCurrentOPMode = OP_MODE_ACCESS_POINT;
    prBssInfo->fgIsNetActive = true;
    prBssInfo->ucBssIndex = ucBssIndex;
    prBssInfo->ucReasonOfDisconnect = DISCONNECT_REASON_CODE_RESERVED;

    prBssInfo->ucPhyTypeSet = PHY_TYPE_SET_802_11BG;  /* Depend on eBand */
    prBssInfo->ucConfigAdHocAPMode = AP_MODE_MIXED_11BG;  /* Depend on
                                                           * eCurrentOPMode
                                                           * and ucPhyTypeSet
                                                           */
    prBssInfo->u2BSSBasicRateSet = RATE_SET_ERP;
    prBssInfo->u2OperationalRateSet = RATE_SET_OFDM;
    prBssInfo->fgErpProtectMode = false;
    prBssInfo->fgIsQBSS = true;

    /* 4 <1.5> Setup MIB for current BSS */
    prBssInfo->u2BeaconInterval = 100;
    prBssInfo->ucDTIMPeriod = DOT11_DTIM_PERIOD_DEFAULT;
    prBssInfo->u2ATIMWindow = 0;

    prBssInfo->ucBeaconTimeoutCount = 0;

    bssInitForAP(prAdapter, prBssInfo, true);

    COPY_MAC_ADDR(prBssInfo->aucBSSID, _aucZeroMacAddr);
    LINK_INITIALIZE(&prBssInfo->rStaRecOfClientList);
    prBssInfo->fgIsBeaconActivated = true;
    prBssInfo->u2HwDefaultFixedRateCode = RATE_CCK_1M_LONG;

    COPY_MAC_ADDR(prBssInfo->aucOwnMacAddr,
                  prAdapter->rWifiVar.aucMacAddress);

    /* 4 <3> Initiate BSS_INFO_T - private part */
    /* TODO */
    prBssInfo->eBand = BAND_2G4;
    prBssInfo->ucPrimaryChannel = 1;
    prBssInfo->prStaRecOfAP = (P_STA_RECORD_T)NULL;

    /* prBssInfo->fgErpProtectMode =  eErpProectMode; */
    /* prBssInfo->eHtProtectMode = eHtProtectMode; */
    /* prBssInfo->eGfOperationMode = eGfOperationMode; */

    /* 4 <4> Allocate MSDU_INFO_T for Beacon */
    prBssInfo->prBeacon = cnmMgtPktAlloc(
        prAdapter,
        OFFSET_OF(WLAN_BEACON_FRAME_T, aucInfoElem[0]) + MAX_IE_LENGTH);

    if (prBssInfo->prBeacon) {
        prBssInfo->prBeacon->eSrc = TX_PACKET_MGMT;
        prBssInfo->prBeacon->ucBssIndex = ucBssIndex;
    } else {
        DBGLOG(SW4, INFO, "prBeacon allocation fail\n");
    }

    prBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC =
        (u8)prAdapter->u4UapsdAcBmp;
    prBssInfo->rPmProfSetupInfo.ucBmpTriggerAC =
        (u8)prAdapter->u4UapsdAcBmp;
    prBssInfo->rPmProfSetupInfo.ucUapsdSp = (u8)prAdapter->u4MaxSpLen;

    DBGLOG(SW4, INFO,
           "[2] ucBmpDeliveryAC:0x%x, ucBmpTriggerAC:0x%x, ucUapsdSp:0x%x",
           prBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC,
           prBssInfo->rPmProfSetupInfo.ucBmpTriggerAC,
           prBssInfo->rPmProfSetupInfo.ucUapsdSp);
}

void testPsCmdCategory0(P_ADAPTER_T prAdapter, u8 ucCate, u8 ucAction,
                        u8 ucOpt0, u8 ucOpt1)
{
    u8 ucIndex, ucRead;
    P_STA_RECORD_T prStaRec;

    DEBUGFUNC("testPsCmdCategory0");
    SWCR_GET_RW_INDEX(ucAction, ucRead, ucIndex);

    DBGLOG(SW4, LOUD, "Read %u Index %u\n", ucRead, ucIndex);

    prStaRec = cnmGetStaRecByIndex(prAdapter, 0);

    if (ucIndex >= TEST_PS_CATA0_INDEX_NUM) {
        return;
    }

    if (ucRead == SWCR_WRITE) {
        switch (ucIndex) {
        case TEST_PS_SETUP_BSS:
            testPsSetupBss(prAdapter, ucOpt0);
            break;

        case TEST_PS_ENABLE_BEACON:
            break;

        case TEST_PS_TRIGGER_BMC:
            /* txmForwardQueuedBmcPkts (ucOpt0); */
            break;

        case TEST_PS_SEND_NULL: {
            testPsSendQoSNullFrame(
                prAdapter, prStaRec,
                (u8)(g_au4SwCr[1] & 0xFF),  /* UP */
                ucOpt0, (u8)((g_au4SwCr[1] >> 8) & 0xFF),  /* BMC
                                                            */
                (u8)((g_au4SwCr[1] >> 16) & 0xFF),  /* BurstEnd
                                                     */
                (u8)((g_au4SwCr[1] >> 24) & 0xFF),  /* Packet
                                                    * type */
                (u8)((g_au4SwCr[2]) & 0xFF),  /* PS sesson ID 7:
                                               * NOACK */
                false  /* EOSP */
                );
        } break;

        case TEST_PS_BUFFER_BMC:
            /* g_aprBssInfo[ucOpt0]->fgApToBufferBMC = (g_au4SwCr[1]
             * & 0xFF); */
            break;

        case TEST_PS_UPDATE_BEACON:
            bssUpdateBeaconContent(prAdapter,
                                   ucOpt0  /*networktype */ );
            break;

        default:
            break;
        }
    } else {
        switch (ucIndex) {
        case TEST_PS_MAGIC:
            g_au4SwCr[1] = 0x88660011;
            break;
        }
    }
}

#endif

#if TEST_PS

void testPsCmdCategory1(P_ADAPTER_T prAdapter, u8 ucCate, u8 ucAction,
                        u8 ucOpt0, u8 ucOpt1)
{
    u8 ucIndex, ucRead;
    u8 ucWTEntry;
    P_STA_RECORD_T prStaRec;

    DEBUGFUNC("testPsCmdCategory1");

    SWCR_GET_RW_INDEX(ucAction, ucRead, ucIndex);

    if (ucOpt0 >= CFG_STA_REC_NUM) {
        return;
    }

    prStaRec = cnmGetStaRecByIndex(prAdapter, ucOpt0);

    if (!prStaRec) {
        DBGLOG(SW4, INFO, "prStaRec is NULL, ucOpt0:%d\n", ucOpt0);
        return;
    }

    ucWTEntry = prStaRec->ucWlanIndex;
    if (ucRead == SWCR_WRITE) {
        switch (ucIndex) {
        case TEST_PS_STA_PS:
            prStaRec->fgIsInPS = (u8)(g_au4SwCr[1] & 0x1);
            prStaRec->fgIsQoS = (u8)(g_au4SwCr[1] >> 8 & 0xFF);
            prStaRec->fgIsUapsdSupported =
                (u8)(g_au4SwCr[1] >> 16 & 0xFF);
            prStaRec->ucBmpDeliveryAC =
                (u8)(g_au4SwCr[1] >> 24 & 0xFF);
            break;
        }
    } else {
        /* Read */
        switch (ucIndex) {
        default:
            break;
        }
    }
}

#endif

#if CFG_SUPPORT_802_11V
#if (CFG_SUPPORT_802_11V_TIMING_MEASUREMENT == 1) && (WNM_UNIT_TEST == 1)
void testWNMCmdCategory0(P_ADAPTER_T prAdapter, u8 ucCate, u8 ucAction,
                         u8 ucOpt0, u8 ucOpt1)
{
    u8 ucIndex, ucRead;
    P_STA_RECORD_T prStaRec;

    DEBUGFUNC("testWNMCmdCategory0");
    SWCR_GET_RW_INDEX(ucAction, ucRead, ucIndex);

    DBGLOG(SW4, INFO, "Read %u Index %u\n", ucRead, ucIndex);

    if (ucIndex >= TEST_WNM_CATA0_INDEX_NUM) {
        return;
    }

    if (ucRead == SWCR_WRITE) {
        switch (ucIndex) {
        case TEST_WNM_TIMING_MEAS:
            wnmTimingMeasUnitTest1(prAdapter, ucOpt0);
            break;

        default:
            break;
        }
    }
}
#endif
#endif

void swCtrlSwCr(P_ADAPTER_T prAdapter, u8 ucRead, u16 u2Addr, u32 *pu4Data)
{
    /* According other register STAIDX */
    u8 ucOffset;

    ucOffset = (u2Addr >> 2) & 0x3F;

    if (ucOffset >= SWCR_CR_NUM) {
        return;
    }

    if (ucRead == SWCR_WRITE) {
        g_au4SwCr[ucOffset] = *pu4Data;
        if (ucOffset == 0x0) {
            /* Commmand   [31:24]: Category */
            /* Commmand   [23:23]: 1(W) 0(R) */
            /* Commmand   [22:16]: Index */
            /* Commmand   [15:08]: Option0  */
            /* Commmand   [07:00]: Option1   */
            u8 ucCate;
            u32 u4Cmd;

            u4Cmd = g_au4SwCr[0];
            ucCate = (u8)(u4Cmd >> 24);
            if (ucCate < ARRAY_SIZE(g_arSwCtrlCmd)) {
                if (g_arSwCtrlCmd[ucCate] != NULL) {
                    g_arSwCtrlCmd[ucCate](
                        prAdapter, ucCate,
                        (u8)(u4Cmd >> 16 & 0xFF),
                        (u8)((u4Cmd >> 8) & 0xFF),
                        (u8)(u4Cmd & 0xFF));
                }
            }
        }
    } else {
        *pu4Data = g_au4SwCr[ucOffset];
    }
}

void swCrReadWriteCmd(P_ADAPTER_T prAdapter, u8 ucRead, u16 u2Addr,
                      u32 *pu4Data)
{
    u8 ucMod;

    ucMod = u2Addr >> 8;
    /* Address [15:8] MOD ID */
    /* Address [7:0] OFFSET */

    DEBUGFUNC("swCrReadWriteCmd");
    DBGLOG(SW4, INFO, "%u addr 0x%x data 0x%x\n", ucRead, u2Addr, *pu4Data);

    if (ucMod < (ARRAY_SIZE(g_arSwCrModHandle))) {
        if (g_arSwCrModHandle[ucMod] != NULL) {
            g_arSwCrModHandle[ucMod](prAdapter, ucRead, u2Addr,
                                     pu4Data);
        }
    }
}

/* Debug Support */
void swCrFrameCheckEnable(P_ADAPTER_T prAdapter, u32 u4DumpType)
{
    g_u4SwcrDebugFrameDumpType = u4DumpType;
#if CFG_RX_PKTS_DUMP
    prAdapter->rRxCtrl.u4RxPktsDumpTypeMask = u4DumpType;
#endif
}

void swCrDebugInit(P_ADAPTER_T prAdapter)
{
    /* frame dump */
    if (g_u4SwcrDebugFrameDumpType) {
        swCrFrameCheckEnable(prAdapter, g_u4SwcrDebugFrameDumpType);
    }
    /* debug counter */
    g_fgSwcrDebugTimer = false;

    cnmTimerInitTimer(prAdapter, &g_rSwcrDebugTimer,
                      (PFN_MGMT_TIMEOUT_FUNC)swCrDebugCheckTimeout,
                      (unsigned long)NULL);

    if (g_u4SwcrDebugCheckTimeout) {
        swCrDebugCheckEnable(prAdapter, true, g_ucSwcrDebugCheckType,
                             g_u4SwcrDebugCheckTimeout);
    }
}

void swCrDebugUninit(P_ADAPTER_T prAdapter)
{
    cnmTimerStopTimer(prAdapter, &g_rSwcrDebugTimer);

    g_fgSwcrDebugTimer = false;
}

void swCrDebugCheckEnable(P_ADAPTER_T prAdapter, u8 fgIsEnable, u8 ucType,
                          u32 u4Timeout)
{
    if (fgIsEnable) {
        g_ucSwcrDebugCheckType = ucType;
        g_u4SwcrDebugCheckTimeout = u4Timeout;
        if (g_fgSwcrDebugTimer == false) {
            swCrDebugCheckTimeout(prAdapter, 0);
        }
    } else {
        cnmTimerStopTimer(prAdapter, &g_rSwcrDebugTimer);
        g_u4SwcrDebugCheckTimeout = 0;
    }

    g_fgSwcrDebugTimer = fgIsEnable;
}

void swCrDebugCheck(P_ADAPTER_T prAdapter, P_CMD_SW_DBG_CTRL_T prCmdSwCtrl)
{
    P_RX_CTRL_T prRxCtrl;
    P_TX_CTRL_T prTxCtrl;

    ASSERT(prAdapter);

    prTxCtrl = &prAdapter->rTxCtrl;
    prRxCtrl = &prAdapter->rRxCtrl;

    /* dump counters */
    if (prCmdSwCtrl) {
        if (prCmdSwCtrl->u4Data == SWCR_DBG_TYPE_ALL) {
            /* TX Counter from fw */
            DBGLOG(SW4, INFO,
                   "TX0\n"
                   "%08x %08x %08x %08x\n"
                   "%08x %08x %08x %08x\n",
                   prCmdSwCtrl->u4DebugCnt[SWCR_DBG_ALL_TX_CNT],
                   prCmdSwCtrl->u4DebugCnt[SWCR_DBG_ALL_TX_BCN_CNT],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_TX_FAILED_CNT],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_TX_RETRY_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_TX_AGING_TIMEOUT_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_TX_PS_OVERFLOW_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_TX_MGNT_DROP_CNT],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_TX_ERROR_CNT]);

            /* TX Counter from drv */
            DBGLOG(SW4, INFO,
                   "TX1\n"
                   "%08x %08x %08x %08x\n",
                   (u32)TX_GET_CNT(prTxCtrl, TX_INACTIVE_BSS_DROP),
                   (u32)TX_GET_CNT(prTxCtrl, TX_INACTIVE_STA_DROP),
                   (u32)TX_GET_CNT(prTxCtrl,
                                   TX_FORWARD_OVERFLOW_DROP),
                   (u32)TX_GET_CNT(prTxCtrl, TX_AP_BORADCAST_DROP));

            /* RX Counter */
            DBGLOG(SW4,
                   INFO,
                   "RX0\n"
                   "%08x %08x %08x %08x\n"
                   "%08x %08x %08x %08x\n"
                   "%08x %08x %08x %08x\n"
                   "%08x %08x %08x %08x\n",
                   prCmdSwCtrl->u4DebugCnt[SWCR_DBG_ALL_RX_CNT],
                   prCmdSwCtrl->u4DebugCnt[SWCR_DBG_ALL_RX_DROP_CNT],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_RX_DUP_DROP_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_RX_TYPE_ERROR_DROP_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_RX_CLASS_ERROR_DROP_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_RX_AMPDU_ERROR_DROP_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_RX_STATUS_ERROR_DROP_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_RX_FORMAT_ERROR_DROP_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_RX_ICV_ERROR_DROP_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_RX_KEY_ERROR_DROP_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_RX_TKIP_ERROR_DROP_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_RX_MIC_ERROR_DROP_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_RX_BIP_ERROR_DROP_CNT],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_RX_FCSERR_CNT],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_RX_FIFOFULL_CNT],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_RX_PFDROP_CNT]);

            DBGLOG(SW4, INFO,
                   "RX1\n"
                   "%08x %08x %08x %08x\n"
                   "%08x %08x %08x %08x\n",
                   (u32)RX_GET_CNT(prRxCtrl, RX_MPDU_TOTAL_COUNT),
                   (u32)RX_GET_CNT(prRxCtrl,
                                   RX_DATA_INDICATION_COUNT),
                   (u32)RX_GET_CNT(prRxCtrl,
                                   RX_DATA_RETURNED_COUNT),
                   (u32)RX_GET_CNT(prRxCtrl,
                                   RX_DATA_RETAINED_COUNT),
                   (u32)RX_GET_CNT(prRxCtrl, RX_DROP_TOTAL_COUNT),
                   (u32)RX_GET_CNT(prRxCtrl,
                                   RX_TYPE_ERR_DROP_COUNT),
                   (u32)RX_GET_CNT(prRxCtrl,
                                   RX_CLASS_ERR_DROP_COUNT),
                   (u32)RX_GET_CNT(prRxCtrl,
                                   RX_DST_NULL_DROP_COUNT));

            DBGLOG(SW4, INFO,
                   "PWR\n"
                   "%08x %08x %08x %08x\n"
                   "%08x %08x %08x %08x\n",
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_PWR_PS_POLL_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_PWR_TRIGGER_NULL_CNT],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_PWR_BCN_IND_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_PWR_BCN_TIMEOUT_CNT],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_PWR_PM_STATE0],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_PWR_PM_STATE1],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_PWR_CUR_PS_PROF0],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_PWR_CUR_PS_PROF1]);

            DBGLOG(SW4, INFO,
                   "ARM\n"
                   "%08x %08x %08x %08x\n"
                   "%08x %08x\n",
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_AR_STA0_RATE],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_AR_STA0_BWGI],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_AR_STA0_RX_RATE_RCPI],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_ROAMING_ENABLE],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_ROAMING_ROAM_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_ROAMING_INT_CNT]);

            DBGLOG(SW4, INFO,
                   "BB\n"
                   "%08x %08x %08x %08x\n"
                   "%08x %08x %08x %08x\n",
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_BB_RX_MDRDY_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_BB_RX_FCSERR_CNT],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_BB_CCK_PD_CNT],
                   prCmdSwCtrl
                   ->u4DebugCnt[SWCR_DBG_ALL_BB_OFDM_PD_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_BB_CCK_SFDERR_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_BB_CCK_SIGERR_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_BB_OFDM_TAGERR_CNT],
                   prCmdSwCtrl->u4DebugCnt
                   [SWCR_DBG_ALL_BB_OFDM_SIGERR_CNT]);
        }
    }
    /* start the next check */
    if (g_u4SwcrDebugCheckTimeout) {
        cnmTimerStartTimer(prAdapter, &g_rSwcrDebugTimer,
                           g_u4SwcrDebugCheckTimeout * MSEC_PER_SEC);
    }
}

void swCrDebugCheckTimeout(IN P_ADAPTER_T prAdapter, unsigned long ulParamPtr)
{
    CMD_SW_DBG_CTRL_T rCmdSwCtrl;
    WLAN_STATUS rStatus;

    kalMemZero(&rCmdSwCtrl, sizeof(CMD_SW_DBG_CTRL_T));
    rCmdSwCtrl.u4Id = (0xb000 << 16) + g_ucSwcrDebugCheckType;
    rCmdSwCtrl.u4Data = 0;
    rStatus = wlanSendSetQueryCmd(
        prAdapter,  /* prAdapter */
        CMD_ID_SW_DBG_CTRL,  /* ucCID */
        false,  /* fgSetQuery */
        true,  /* fgNeedResp */
        false,  /* fgIsOid */
        swCrDebugQuery,  /* pfCmdDoneHandler */
        swCrDebugQueryTimeout,  /* pfCmdTimeoutHandler
                                 */
        sizeof(CMD_SW_DBG_CTRL_T),  /* u4SetQueryInfoLen
                                     */
        (u8 *)&rCmdSwCtrl,  /* pucInfoBuffer */
        NULL,  /* pvSetQueryBuffer */
        0  /* u4SetQueryBufferLen */
        );

    ASSERT(rStatus == WLAN_STATUS_PENDING);
}

void swCrDebugQuery(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                    IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    ASSERT(prAdapter);
    if (u4EventBufLen < sizeof(CMD_SW_DBG_CTRL_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n",
               __func__, u4EventBufLen, sizeof(CMD_SW_DBG_CTRL_T));
        return;
    }
    swCrDebugCheck(prAdapter, (P_CMD_SW_DBG_CTRL_T)(pucEventBuf));
}

void swCrDebugQueryTimeout(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo)
{
    ASSERT(prAdapter);

    swCrDebugCheck(prAdapter, NULL);
}

#endif
