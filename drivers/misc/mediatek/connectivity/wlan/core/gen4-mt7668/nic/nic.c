// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   nic.c
 *    \brief  Functions that provide operation in NIC's (Network Interface Card)
 * point of view.
 *
 *    This file includes functions which unite multiple hal(Hardware) operations
 *    and also take the responsibility of Software Resource Management in order
 *    to keep the synchronization with Hardware Manipulation.
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

const u8 aucPhyCfg2PhyTypeSet[PHY_CONFIG_NUM] = {
    PHY_TYPE_SET_802_11ABG,  /* PHY_CONFIG_802_11ABG */
    PHY_TYPE_SET_802_11BG,  /* PHY_CONFIG_802_11BG */
    PHY_TYPE_SET_802_11G,  /* PHY_CONFIG_802_11G */
    PHY_TYPE_SET_802_11A,  /* PHY_CONFIG_802_11A */
    PHY_TYPE_SET_802_11B,  /* PHY_CONFIG_802_11B */
    PHY_TYPE_SET_802_11ABGN,  /* PHY_CONFIG_802_11ABGN */
    PHY_TYPE_SET_802_11BGN,  /* PHY_CONFIG_802_11BGN */
    PHY_TYPE_SET_802_11AN,  /* PHY_CONFIG_802_11AN */
    PHY_TYPE_SET_802_11GN,  /* PHY_CONFIG_802_11GN */
    PHY_TYPE_SET_802_11AC, PHY_TYPE_SET_802_11ANAC, PHY_TYPE_SET_802_11ABGNAC
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

static INT_EVENT_MAP_T arIntEventMapTable[] = {
    { WHISR_ABNORMAL_INT, INT_EVENT_ABNORMAL },
    { WHISR_D2H_SW_INT, INT_EVENT_SW_INT },
    { WHISR_TX_DONE_INT, INT_EVENT_TX },
    { (WHISR_RX0_DONE_INT | WHISR_RX1_DONE_INT), INT_EVENT_RX }
};

static const u8 ucIntEventMapSize =
    (sizeof(arIntEventMapTable) / sizeof(INT_EVENT_MAP_T));

static IST_EVENT_FUNCTION apfnEventFuncTable[] = {
    nicProcessAbnormalInterrupt,  /*!< INT_EVENT_ABNORMAL */
    nicProcessSoftwareInterrupt,  /*!< INT_EVENT_SW_INT */
    nicProcessTxInterrupt,  /*!< INT_EVENT_TX */
    nicProcessRxInterrupt,  /*!< INT_EVENT_RX */
};

ECO_INFO_T g_eco_info = { 0xFF };
/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*! This macro is used to reduce coding errors inside nicAllocateAdapterMemory()
 * and also enhance the readability.
 */
#define LOCAL_NIC_ALLOCATE_MEMORY(pucMem, u4Size, eMemType, pucComment)   \
    {                                                                     \
        DBGLOG(INIT, INFO, "Allocating %ld bytes for %s.\n", u4Size,      \
               pucComment);                                               \
        pucMem = (u8 *)kalMemAlloc(u4Size, eMemType);                     \
        if (pucMem == (u8 *)NULL) {                                       \
            DBGLOG(INIT, ERROR, "Could not allocate %ld bytes for %s.\n", \
                   u4Size, pucComment);                                   \
            break;                                                        \
        }                                                                 \
        ASSERT(((unsigned long)pucMem % 4) == 0);                         \
        DBGLOG(INIT, INFO, "Virtual Address = 0x%p for %s.\n",            \
               (unsigned long)pucMem, pucComment);                        \
    }

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
 * @brief This routine is responsible for the allocation of the data structures
 *        inside the Adapter structure, include:
 *        1. SW_RFB_Ts
 *        2. Common coalescing buffer for TX PATH.
 *
 * @param prAdapter Pointer of Adapter Data Structure
 *
 * @retval WLAN_STATUS_SUCCESS - Has enough memory.
 * @retval WLAN_STATUS_RESOURCES - Memory is not enough.
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicAllocateAdapterMemory(IN P_ADAPTER_T prAdapter)
{
    WLAN_STATUS status = WLAN_STATUS_RESOURCES;
    P_RX_CTRL_T prRxCtrl;
    P_TX_CTRL_T prTxCtrl;

    DEBUGFUNC("nicAllocateAdapterMemory");

    ASSERT(prAdapter);
    prRxCtrl = &prAdapter->rRxCtrl;
    prTxCtrl = &prAdapter->rTxCtrl;

    do {
        /* 4 <0> Reset all Memory Handler */
#if CFG_DBG_MGT_BUF
        prAdapter->u4MemFreeDynamicCount = 0;
        prAdapter->u4MemAllocDynamicCount = 0;
#endif
        prAdapter->pucMgtBufCached = (u8 *)NULL;
        prRxCtrl->pucRxCached = (u8 *)NULL;

        /* 4 <1> Memory for Management Memory Pool and CMD_INFO_T */
        /* Allocate memory for the CMD_INFO_T and its MGMT memory pool.
         */
        prAdapter->u4MgtBufCachedSize = MGT_BUFFER_SIZE;

#ifdef CFG_PREALLOC_MEMORY
        prAdapter->pucMgtBufCached = preallocGetMem(MEM_ID_NIC_ADAPTER);
#else
        LOCAL_NIC_ALLOCATE_MEMORY(prAdapter->pucMgtBufCached,
                                  prAdapter->u4MgtBufCachedSize, PHY_MEM_TYPE,
                                  "COMMON MGMT MEMORY POOL");
#endif

        /* 4 <2> Memory for RX Descriptor */
        /* Initialize the number of rx buffers we will have in our
         * queue. */
        /* <TODO> We may setup ucRxPacketDescriptors by GLUE Layer, and
         * using this variable directly.
         */
        /* Allocate memory for the SW receive structures. */
        prRxCtrl->u4RxCachedSize =
            CFG_RX_MAX_PKT_NUM * ALIGN_4(sizeof(SW_RFB_T));

        LOCAL_NIC_ALLOCATE_MEMORY(prRxCtrl->pucRxCached,
                                  prRxCtrl->u4RxCachedSize, VIR_MEM_TYPE,
                                  "SW_RFB_T");

        /* 4 <3> Memory for TX DEscriptor */
        prTxCtrl->u4TxCachedSize =
            CFG_TX_MAX_PKT_NUM * ALIGN_4(sizeof(MSDU_INFO_T));

        LOCAL_NIC_ALLOCATE_MEMORY(prTxCtrl->pucTxCached,
                                  prTxCtrl->u4TxCachedSize, VIR_MEM_TYPE,
                                  "MSDU_INFO_T");

        /* 4 <4> Memory for Common Coalescing Buffer */

        /* Get valid buffer size based on config & host capability */
        prAdapter->u4CoalescingBufCachedSize =
            halGetValidCoalescingBufSize(prAdapter);

        /* Allocate memory for the common coalescing buffer. */
#ifdef CFG_PREALLOC_MEMORY
        prAdapter->pucCoalescingBufCached = preallocGetMem(MEM_ID_IO_BUFFER);
#else
        prAdapter->pucCoalescingBufCached =
            kalAllocateIOBuffer(prAdapter->u4CoalescingBufCachedSize);
#endif

        if (prAdapter->pucCoalescingBufCached == NULL) {
            DBGLOG(INIT, ERROR,
                   "Could not allocate %ld bytes for coalescing buffer.\n",
                   prAdapter->u4CoalescingBufCachedSize);
            break;
        }

        /* <5> Memory for HIF */
        if (halAllocateIOBuffer(prAdapter) != WLAN_STATUS_SUCCESS) {
            break;
        }

        status = WLAN_STATUS_SUCCESS;
    } while (false);

    if (status != WLAN_STATUS_SUCCESS) {
        nicReleaseAdapterMemory(prAdapter);
    }

    return status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for releasing the allocated memory by
 *        nicAllocatedAdapterMemory().
 *
 * @param prAdapter Pointer of Adapter Data Structure
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicReleaseAdapterMemory(IN P_ADAPTER_T prAdapter)
{
    P_TX_CTRL_T prTxCtrl;
    P_RX_CTRL_T prRxCtrl;
    u32 u4Idx;

    ASSERT(prAdapter);
    prTxCtrl = &prAdapter->rTxCtrl;
    prRxCtrl = &prAdapter->rRxCtrl;

    /* 4 <5> Memory for HIF */
    halReleaseIOBuffer(prAdapter);

    /* 4 <4> Memory for Common Coalescing Buffer */
    if (prAdapter->pucCoalescingBufCached) {
#ifndef CFG_PREALLOC_MEMORY
        kalReleaseIOBuffer((void *)prAdapter->pucCoalescingBufCached,
                           prAdapter->u4CoalescingBufCachedSize);
#endif
        prAdapter->pucCoalescingBufCached = (u8 *)NULL;
    }

    /* 4 <3> Memory for TX Descriptor */
    if (prTxCtrl->pucTxCached) {
        kalMemFree((void *)prTxCtrl->pucTxCached, VIR_MEM_TYPE,
                   prTxCtrl->u4TxCachedSize);
        prTxCtrl->pucTxCached = (u8 *)NULL;
    }
    /* 4 <2> Memory for RX Descriptor */
    if (prRxCtrl->pucRxCached) {
        kalMemFree((void *)prRxCtrl->pucRxCached, VIR_MEM_TYPE,
                   prRxCtrl->u4RxCachedSize);
        prRxCtrl->pucRxCached = (u8 *)NULL;
    }
    /* 4 <1> Memory for Management Memory Pool */
    if (prAdapter->pucMgtBufCached) {
#ifndef CFG_PREALLOC_MEMORY
        kalMemFree((void *)prAdapter->pucMgtBufCached, PHY_MEM_TYPE,
                   prAdapter->u4MgtBufCachedSize);
#endif
        prAdapter->pucMgtBufCached = (u8 *)NULL;
    }

    /* Memory for TX Desc Template */
    for (u4Idx = 0; u4Idx < CFG_STA_REC_NUM; u4Idx++)
        nicTxFreeDescTemplate(prAdapter, &prAdapter->arStaRec[u4Idx]);

#if CFG_DBG_MGT_BUF
    do {
        u8 fgUnfreedMem = false;
        P_BUF_INFO_T prBufInfo;

        /* Dynamic allocated memory from OS */
        if (prAdapter->u4MemFreeDynamicCount !=
            prAdapter->u4MemAllocDynamicCount) {
            fgUnfreedMem = true;
        }

        /* MSG buffer */
        prBufInfo = &prAdapter->rMsgBufInfo;
        if (prBufInfo->u4AllocCount !=
            (prBufInfo->u4FreeCount + prBufInfo->u4AllocNullCount)) {
            fgUnfreedMem = true;
        }

        /* MGT buffer */
        prBufInfo = &prAdapter->rMgtBufInfo;
        if (prBufInfo->u4AllocCount !=
            (prBufInfo->u4FreeCount + prBufInfo->u4AllocNullCount)) {
            fgUnfreedMem = true;
        }

        /* Check if all allocated memories are free */
        if (fgUnfreedMem) {
            DBGLOG(MEM, ERROR, "Unequal memory alloc/free count!\n");

            qmDumpQueueStatus(prAdapter, NULL, 0);
            cnmDumpMemoryStatus(prAdapter, NULL, 0);
        }

        if (!wlanIsChipNoAck(prAdapter)) {
            /* Skip this ASSERT if chip is no ACK */
            ASSERT(prAdapter->u4MemFreeDynamicCount ==
                   prAdapter->u4MemAllocDynamicCount);
        }
    } while (false);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief disable global interrupt
 *
 * @param prAdapter pointer to the Adapter handler
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicDisableInterrupt(IN P_ADAPTER_T prAdapter)
{
    halDisableInterrupt(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief enable global interrupt
 *
 * @param prAdapter pointer to the Adapter handler
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicEnableInterrupt(IN P_ADAPTER_T prAdapter)
{
    halEnableInterrupt(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief The function used to read interrupt status and then invoking
 *        dispatching procedure for the appropriate functions
 *        corresponding to specific interrupt bits
 *
 * @param prAdapter pointer to the Adapter handler
 *
 * @retval WLAN_STATUS_SUCCESS
 * @retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicProcessIST(IN P_ADAPTER_T prAdapter)
{
    WLAN_STATUS u4Status = WLAN_STATUS_SUCCESS;
    u32 u4IntStatus = 0;
    u32 i;

    ASSERT(prAdapter);

    if (prAdapter->rAcpiState == ACPI_STATE_D3) {
        DBGLOG(
            REQ, WARN,
            "Fail in set nicProcessIST! (Adapter not ready). ACPI=D%d, Radio=%d\n",
            prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
        return WLAN_STATUS_ADAPTER_NOT_READY;
    }

    for (i = 0; i < prAdapter->rWifiVar.u4HifIstLoopCount; i++) {
        HAL_READ_INT_STATUS(prAdapter, &u4IntStatus);
        /* DBGLOG(INIT, TRACE, ("u4IntStatus: 0x%x\n", u4IntStatus)); */

        if (u4IntStatus == 0) {
            if (i == 0) {
                u4Status = WLAN_STATUS_NOT_INDICATING;
            }
            break;
        }

        nicProcessIST_impl(prAdapter, u4IntStatus);

        /* Have to TX now. Skip RX polling ASAP */
        if (test_bit(GLUE_FLAG_HIF_TX_CMD_BIT,
                     &prAdapter->prGlueInfo->ulFlag) ||
            test_bit(GLUE_FLAG_HIF_TX_BIT, &prAdapter->prGlueInfo->ulFlag)) {
            i *= 2;
        }
    }

    return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief The function used to dispatch the appropriate functions for specific
 *        interrupt bits
 *
 * @param prAdapter   pointer to the Adapter handler
 *        u4IntStatus interrupt status bits
 *
 * @retval WLAN_STATUS_SUCCESS
 * @retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicProcessIST_impl(IN P_ADAPTER_T prAdapter, IN u32 u4IntStatus)
{
    u32 u4IntCount = 0;
    P_INT_EVENT_MAP_T prIntEventMap = NULL;

    ASSERT(prAdapter);

    prAdapter->u4IntStatus = u4IntStatus;

    /* Process each of the interrupt status consequently */
    prIntEventMap = &arIntEventMapTable[0];
    for (u4IntCount = 0; u4IntCount < ucIntEventMapSize;
         prIntEventMap++, u4IntCount++) {
        if (prIntEventMap->u4Int & prAdapter->u4IntStatus) {
            if (0  /*prIntEventMap->u4Event == INT_EVENT_RX && prAdapter->fgIsEnterD3ReqIssued == true*/)
            {
                /* ignore */
            } else if (apfnEventFuncTable[prIntEventMap->u4Event] != NULL) {
                apfnEventFuncTable[prIntEventMap->u4Event](prAdapter);
            } else {
                DBGLOG(
                    INTR, WARN,
                    "Empty INTR handler! ISAR bit#: %ld, event:%lu, func: 0x%x\n",
                    prIntEventMap->u4Int, prIntEventMap->u4Event,
                    apfnEventFuncTable[prIntEventMap->u4Event]);

                ASSERT(0);  /* to trap any NULL interrupt handler
                             */
            }
            prAdapter->u4IntStatus &= ~prIntEventMap->u4Int;
        }
    }

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Verify the CHIP ID
 *
 * @param prAdapter      a pointer to adapter private data structure.
 *
 *
 * @retval true          CHIP ID is the same as the setting compiled
 * @retval false         CHIP ID is different from the setting compiled
 */
/*----------------------------------------------------------------------------*/
u8 nicVerifyChipID(IN P_ADAPTER_T prAdapter)
{
    return halVerifyChipID(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialize the MCR to the appropriate init value, and verify the init
 *        value
 *
 * @param prAdapter      a pointer to adapter private data structure.
 *
 * @return -
 */
/*----------------------------------------------------------------------------*/
void nicMCRInit(IN P_ADAPTER_T prAdapter)
{
    ASSERT(prAdapter);

    /* 4 <0> Initial value */
}

void nicHifInit(IN P_ADAPTER_T prAdapter)
{
    ASSERT(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialize the Adapter soft variable
 *
 * @param prAdapter pointer to the Adapter handler
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicInitializeAdapter(IN P_ADAPTER_T prAdapter)
{
    WLAN_STATUS u4Status = WLAN_STATUS_SUCCESS;

    ASSERT(prAdapter);

    prAdapter->fgIsIntEnableWithLPOwnSet = false;
    prAdapter->fgIsReadRevID = false;

#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
    prAdapter->fgIsBufferBinExtract = false;

    prAdapter->u4EfuseMacAddrOffset = DEFAULT_EFUSE_MACADDR_OFFSET;
#endif

    do {
        if (!nicVerifyChipID(prAdapter)) {
            u4Status = WLAN_STATUS_FAILURE;
            break;
        }
        /* 4 <1> MCR init */
        nicMCRInit(prAdapter);

        HAL_HIF_INIT(prAdapter);

        /* 4 <2> init FW HIF */
        nicHifInit(prAdapter);
    } while (false);

    return u4Status;
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
void nicProcessAbnormalInterrupt(IN P_ADAPTER_T prAdapter)
{
    u32 u4Value;

    HAL_MCR_RD(prAdapter, MCR_WASR, &u4Value);
    DBGLOG(REQ, WARN, "MCR_WASR: 0x%lx\n", u4Value);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief .
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicProcessSoftwareInterrupt(IN P_ADAPTER_T prAdapter)
{
    halProcessSoftwareInterrupt(prAdapter);
}

void nicSetSwIntr(IN P_ADAPTER_T prAdapter, IN u32 u4SwIntrBitmap)
{
    /* NOTE:
     *  SW interrupt in HW bit 16 is mapping to SW bit 0 (shift 16bit in HW
     * transparancy) SW interrupt valid from b0~b15
     */
    ASSERT((u4SwIntrBitmap & BITS(0, 15)) == 0);
    /* DBGLOG(INIT, TRACE, ("u4SwIntrBitmap: 0x%08x\n", u4SwIntrBitmap)); */

    HAL_MCR_WR(prAdapter, MCR_WSICR, u4SwIntrBitmap);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This procedure is used to dequeue from prAdapter->rPendingCmdQueue
 *        with specified sequential number
 *
 * @param    prAdapter   Pointer of ADAPTER_T
 *           ucSeqNum    Sequential Number
 *
 * @retval - P_CMD_INFO_T
 */
/*----------------------------------------------------------------------------*/
P_CMD_INFO_T nicGetPendingCmdInfo(IN P_ADAPTER_T prAdapter, IN u8 ucSeqNum)
{
    P_QUE_T prCmdQue;
    QUE_T rTempCmdQue;
    P_QUE_T prTempCmdQue = &rTempCmdQue;
    P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T)NULL;
    P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T)NULL;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);

    prCmdQue = &prAdapter->rPendingCmdQueue;
    QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);

    QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    while (prQueueEntry) {
        prCmdInfo = (P_CMD_INFO_T)prQueueEntry;

        if (prCmdInfo->ucCmdSeqNum == ucSeqNum) {
            break;
        }

        QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);

        prCmdInfo = NULL;

        QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    }
    QUEUE_CONCATENATE_QUEUES(prCmdQue, prTempCmdQue);

    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);

    return prCmdInfo;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This procedure is used to dequeue from
 * prAdapter->rTxCtrl.rTxMgmtTxingQueue with specified sequential number
 *
 * @param    prAdapter   Pointer of ADAPTER_T
 *           ucSeqNum    Sequential Number
 *
 * @retval - P_MSDU_INFO_T
 */
/*----------------------------------------------------------------------------*/
P_MSDU_INFO_T nicGetPendingTxMsduInfo(IN P_ADAPTER_T prAdapter,
                                      IN u8 ucWlanIndex, IN u8 ucPID)
{
    P_QUE_T prTxingQue;
    QUE_T rTempQue;
    P_QUE_T prTempQue = &rTempQue;
    P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T)NULL;
    P_MSDU_INFO_T prMsduInfo = (P_MSDU_INFO_T)NULL;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);

    prTxingQue = &(prAdapter->rTxCtrl.rTxMgmtTxingQueue);
    QUEUE_MOVE_ALL(prTempQue, prTxingQue);

    QUEUE_REMOVE_HEAD(prTempQue, prQueueEntry, P_QUE_ENTRY_T);
    while (prQueueEntry) {
        prMsduInfo = (P_MSDU_INFO_T)prQueueEntry;

        if ((prMsduInfo->ucPID == ucPID) &&
            (prMsduInfo->ucWlanIndex == ucWlanIndex)) {
            break;
        }

        QUEUE_INSERT_TAIL(prTxingQue, prQueueEntry);

        prMsduInfo = NULL;

        QUEUE_REMOVE_HEAD(prTempQue, prQueueEntry, P_QUE_ENTRY_T);
    }
    QUEUE_CONCATENATE_QUEUES(prTxingQue, prTempQue);

    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);

    if (prMsduInfo) {
        DBGLOG(TX, TRACE, "Get Msdu WIDX:PID[%u:%u] SEQ[%u] from Pending Q\n",
               prMsduInfo->ucWlanIndex, prMsduInfo->ucPID,
               prMsduInfo->ucTxSeqNum);
    } else {
        DBGLOG(TX, WARN,
               "Cannot get Target Msdu WIDX:PID[%u:%u] from Pending Q\n",
               ucWlanIndex, ucPID);
    }

    return prMsduInfo;
}

void nicFreePendingTxMsduInfoByBssIdx(IN P_ADAPTER_T prAdapter,
                                      IN u8 ucBssIndex)
{
    P_QUE_T prTxingQue;
    QUE_T rTempQue;
    P_QUE_T prTempQue = &rTempQue;
    P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T)NULL;
    P_MSDU_INFO_T prMsduInfoListHead = (P_MSDU_INFO_T)NULL;
    P_MSDU_INFO_T prMsduInfoListTail = (P_MSDU_INFO_T)NULL;
    P_MSDU_INFO_T prMsduInfo = (P_MSDU_INFO_T)NULL;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);

    prTxingQue = &(prAdapter->rTxCtrl.rTxMgmtTxingQueue);
    QUEUE_MOVE_ALL(prTempQue, prTxingQue);

    QUEUE_REMOVE_HEAD(prTempQue, prQueueEntry, P_QUE_ENTRY_T);
    while (prQueueEntry) {
        prMsduInfo = (P_MSDU_INFO_T)prQueueEntry;

        if (prMsduInfo->ucBssIndex == ucBssIndex) {
            DBGLOG(TX, TRACE,
                   "%s: Get Msdu WIDX:PID[%u:%u] SEQ[%u] from Pending Q\n",
                   __func__, prMsduInfo->ucWlanIndex, prMsduInfo->ucPID,
                   prMsduInfo->ucTxSeqNum);

            if (prMsduInfoListHead == NULL) {
                prMsduInfoListHead = prMsduInfoListTail = prMsduInfo;
            } else {
                QM_TX_SET_NEXT_MSDU_INFO(prMsduInfoListTail, prMsduInfo);
                prMsduInfoListTail = prMsduInfo;
            }
        } else {
            QUEUE_INSERT_TAIL(prTxingQue, prQueueEntry);

            prMsduInfo = NULL;
        }

        QUEUE_REMOVE_HEAD(prTempQue, prQueueEntry, P_QUE_ENTRY_T);
    }
    QUEUE_CONCATENATE_QUEUES(prTxingQue, prTempQue);

    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);

    /* free */
    if (prMsduInfoListHead) {
        nicTxFreeMsduInfoPacket(prAdapter, prMsduInfoListHead);
        nicTxReturnMsduInfo(prAdapter, prMsduInfoListHead);
    }

    return;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This procedure is used to retrieve a CMD sequence number atomically
 *
 * @param    prAdapter   Pointer of ADAPTER_T
 *
 * @retval - u8
 */
/*----------------------------------------------------------------------------*/
u8 nicIncreaseCmdSeqNum(IN P_ADAPTER_T prAdapter)
{
    u8 ucRetval;

    KAL_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_SEQ_NUM);

    prAdapter->ucCmdSeqNum++;
    ucRetval = prAdapter->ucCmdSeqNum;

    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_SEQ_NUM);

    return ucRetval;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This procedure is used to retrieve a TX sequence number atomically
 *
 * @param    prAdapter   Pointer of ADAPTER_T
 *
 * @retval - u8
 */
/*----------------------------------------------------------------------------*/
u8 nicIncreaseTxSeqNum(IN P_ADAPTER_T prAdapter)
{
    u8 ucRetval;

    KAL_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_SEQ_NUM);

    ucRetval = prAdapter->ucTxSeqNum;

    prAdapter->ucTxSeqNum++;

    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_SEQ_NUM);

    return ucRetval;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to handle
 *        media state change event
 *
 * @param
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
nicMediaStateChange(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                    IN P_EVENT_CONNECTION_STATUS prConnectionStatus)
{
    P_GLUE_INFO_T prGlueInfo;

    ASSERT(prAdapter);
    prGlueInfo = prAdapter->prGlueInfo;

    switch (GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType) {
    case NETWORK_TYPE_AIS:
        if (prConnectionStatus->ucMediaStatus ==
            PARAM_MEDIA_STATE_DISCONNECTED) {  /* disconnected */
            if (kalGetMediaStateIndicated(prGlueInfo) !=
                PARAM_MEDIA_STATE_DISCONNECTED) {
                kalIndicateStatusAndComplete(
                    prGlueInfo, WLAN_STATUS_MEDIA_DISCONNECT, NULL, 0);

                prAdapter->rWlanInfo.u4SysTime = kalGetTimeTick();
            }

            /* reset buffered link quality information */
            prAdapter->fgIsLinkQualityValid = false;
            prAdapter->fgIsLinkRateValid = false;
        } else if (prConnectionStatus->ucMediaStatus ==
                   PARAM_MEDIA_STATE_CONNECTED) {  /* connected */
            prAdapter->rWlanInfo.u4SysTime = kalGetTimeTick();

            /* fill information for association result */
            prAdapter->rWlanInfo.rCurrBssId.rSsid.u4SsidLen =
                prConnectionStatus->ucSsidLen;
            kalMemCopy(prAdapter->rWlanInfo.rCurrBssId.rSsid.aucSsid,
                       prConnectionStatus->aucSsid,
                       prConnectionStatus->ucSsidLen);
            kalMemCopy(prAdapter->rWlanInfo.rCurrBssId.arMacAddress,
                       prConnectionStatus->aucBssid, MAC_ADDR_LEN);
            prAdapter->rWlanInfo.rCurrBssId.u4Privacy =
                prConnectionStatus->ucEncryptStatus;  /* @FIXME
                                                       */
            prAdapter->rWlanInfo.rCurrBssId.rRssi = 0;  /* @FIXME */
            prAdapter->rWlanInfo.rCurrBssId.eNetworkTypeInUse =
                PARAM_NETWORK_TYPE_AUTOMODE;  /* @FIXME */
            prAdapter->rWlanInfo.rCurrBssId.rConfiguration.u4BeaconPeriod =
                prConnectionStatus->u2BeaconPeriod;
            prAdapter->rWlanInfo.rCurrBssId.rConfiguration.u4ATIMWindow =
                prConnectionStatus->u2ATIMWindow;
            prAdapter->rWlanInfo.rCurrBssId.rConfiguration.u4DSConfig =
                prConnectionStatus->u4FreqInKHz;
            prAdapter->rWlanInfo.ucNetworkType =
                prConnectionStatus->ucNetworkType;
            prAdapter->rWlanInfo.rCurrBssId.eOpMode =
                (ENUM_PARAM_OP_MODE_T)prConnectionStatus->ucInfraMode;

            /* always indicate to OS according to MSDN
             * (re-association/roaming) */
            if (kalGetMediaStateIndicated(prGlueInfo) !=
                PARAM_MEDIA_STATE_CONNECTED) {
                kalIndicateStatusAndComplete(
                    prGlueInfo, WLAN_STATUS_MEDIA_CONNECT, NULL, 0);
            } else {
                /* connected -> connected : roaming ? */
                kalIndicateStatusAndComplete(
                    prGlueInfo, WLAN_STATUS_ROAM_OUT_FIND_BEST, NULL, 0);
            }
        }
        break;

#if CFG_ENABLE_WIFI_DIRECT
    case NETWORK_TYPE_P2P:
        break;

#endif
    default:
        ASSERT(0);
    }

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to generate a join failure event to OS
 *
 * @param
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicMediaJoinFailure(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                                IN WLAN_STATUS rStatus)
{
    P_GLUE_INFO_T prGlueInfo;

    ASSERT(prAdapter);
    prGlueInfo = prAdapter->prGlueInfo;

    switch (GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType) {
    case NETWORK_TYPE_AIS:
        kalIndicateStatusAndComplete(prGlueInfo, rStatus, NULL, 0);

        break;

    case NETWORK_TYPE_BOW:
    case NETWORK_TYPE_P2P:
    default:
        break;
    }

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to convert between
 *        frequency and channel number
 *
 * @param u4ChannelNum
 *
 * @retval - Frequency in unit of KHz, 0 for invalid channel number
 */
/*----------------------------------------------------------------------------*/
u32 nicChannelNum2Freq(u32 u4ChannelNum)
{
    u32 u4ChannelInMHz;

    if (u4ChannelNum >= 1 && u4ChannelNum <= 13) {
        u4ChannelInMHz = 2412 + (u4ChannelNum - 1) * 5;
    } else if (u4ChannelNum == 14) {
        u4ChannelInMHz = 2484;
    } else if (u4ChannelNum == 133) {
        u4ChannelInMHz = 3665;  /* 802.11y */
    } else if (u4ChannelNum == 137) {
        u4ChannelInMHz = 3685;  /* 802.11y */
    } else if ((u4ChannelNum >= 34 && u4ChannelNum <= 181) ||
               (u4ChannelNum == 16)) {
        u4ChannelInMHz = 5000 + u4ChannelNum * 5;
    } else if (u4ChannelNum >= 182 && u4ChannelNum <= 196) {
        u4ChannelInMHz = 4000 + u4ChannelNum * 5;
    } else if (u4ChannelNum == 201) {
        u4ChannelInMHz = 2730;
    } else if (u4ChannelNum == 202) {
        u4ChannelInMHz = 2498;
    } else {
        u4ChannelInMHz = 0;
    }

    return 1000 * u4ChannelInMHz;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to convert between
 *        frequency and channel number
 *
 * @param u4FreqInKHz
 *
 * @retval - Frequency Number, 0 for invalid freqency
 */
/*----------------------------------------------------------------------------*/
u32 nicFreq2ChannelNum(u32 u4FreqInKHz)
{
    switch (u4FreqInKHz) {
    case 2412000:
        return 1;

    case 2417000:
        return 2;

    case 2422000:
        return 3;

    case 2427000:
        return 4;

    case 2432000:
        return 5;

    case 2437000:
        return 6;

    case 2442000:
        return 7;

    case 2447000:
        return 8;

    case 2452000:
        return 9;

    case 2457000:
        return 10;

    case 2462000:
        return 11;

    case 2467000:
        return 12;

    case 2472000:
        return 13;

    case 2484000:
        return 14;

    case 3665000:
        return 133;  /* 802.11y */

    case 3685000:
        return 137;  /* 802.11y */

    case 4915000:
        return 183;

    case 4920000:
        return 184;

    case 4925000:
        return 185;

    case 4930000:
        return 186;

    case 4935000:
        return 187;

    case 4940000:
        return 188;

    case 4945000:
        return 189;

    case 4960000:
        return 192;

    case 4980000:
        return 196;

    case 5170000:
        return 34;

    case 5180000:
        return 36;

    case 5190000:
        return 38;

    case 5200000:
        return 40;

    case 5210000:
        return 42;

    case 5220000:
        return 44;

    case 5230000:
        return 46;

    case 5240000:
        return 48;

    case 5250000:
        return 50;

    case 5260000:
        return 52;

    case 5270000:
        return 54;

    case 5280000:
        return 56;

    case 5290000:
        return 58;

    case 5300000:
        return 60;

    case 5310000:
        return 62;

    case 5320000:
        return 64;

    case 5500000:
        return 100;

    case 5510000:
        return 102;

    case 5520000:
        return 104;

    case 5530000:
        return 106;

    case 5540000:
        return 108;

    case 5550000:
        return 110;

    case 5560000:
        return 112;

    case 5570000:
        return 114;

    case 5580000:
        return 116;

    case 5590000:
        return 118;

    case 5600000:
        return 120;

    case 5610000:
        return 122;

    case 5620000:
        return 124;

    case 5630000:
        return 126;

    case 5640000:
        return 128;

    case 5660000:
        return 132;

    case 5670000:
        return 134;

    case 5680000:
        return 136;

    case 5690000:
        return 138;

    case 5700000:
        return 140;

    case 5710000:
        return 142;

    case 5720000:
        return 144;

    case 5745000:
        return 149;

    case 5755000:
        return 151;

    case 5765000:
        return 153;

    case 5775000:
        return 155;

    case 5785000:
        return 157;

    case 5795000:
        return 159;

    case 5805000:
        return 161;

    case 5825000:
        return 165;

    case 5845000:
        return 169;

    case 5865000:
        return 173;

    default:
        DBGLOG(BSS, INFO, "Return Invalid Channelnum = 0.\n");
        return 0;
    }
}

u8 nicGetVhtS1(u8 ucPrimaryChannel, u8 ucBandwidth)
{
    /* find S1 (central channel 42, 58, 106, 122, and 155) */

    if ((ucBandwidth == VHT_OP_CHANNEL_WIDTH_80) ||
        (ucBandwidth == VHT_OP_CHANNEL_WIDTH_80P80)) {
        if (ucPrimaryChannel >= 36 && ucPrimaryChannel <= 48) {
            return 42;
        } else if (ucPrimaryChannel >= 52 && ucPrimaryChannel <= 64) {
            return 58;
        } else if (ucPrimaryChannel >= 100 && ucPrimaryChannel <= 112) {
            return 106;
        } else if (ucPrimaryChannel >= 116 && ucPrimaryChannel <= 128) {
            return 122;
        } else if (ucPrimaryChannel >= 132 && ucPrimaryChannel <= 144) {
            return 138;
        } else if (ucPrimaryChannel >= 149 && ucPrimaryChannel <= 161) {
            return 155;
        }
    } else if (ucBandwidth == VHT_OP_CHANNEL_WIDTH_160) {
        if (ucPrimaryChannel >= 36 && ucPrimaryChannel <= 64) {
            return 50;
        } else if (ucPrimaryChannel >= 100 && ucPrimaryChannel <= 128) {
            return 114;
        }
    } else {
        return 0;
    }

    return 0;
}

/* firmware command wrapper */
/* NETWORK (WIFISYS) */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to activate WIFISYS for specified
 * network
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        eNetworkTypeIdx    Index of network type
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicActivateNetwork(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex)
{
    CMD_BSS_ACTIVATE_CTRL rCmdActivateCtrl;
    P_BSS_INFO_T prBssInfo;
    /* const u8 aucZeroMacAddr[] = NULL_MAC_ADDR; */

    ASSERT(prAdapter);
    ASSERT(IS_BSS_INDEX_VALID(ucBssIndex));

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    prBssInfo->fg40mBwAllowed = false;
    prBssInfo->fgAssoc40mBwAllowed = false;

    rCmdActivateCtrl.ucBssIndex = ucBssIndex;
    rCmdActivateCtrl.ucActive = 1;
    rCmdActivateCtrl.ucNetworkType = (u8)prBssInfo->eNetworkType;
    rCmdActivateCtrl.ucOwnMacAddrIndex = prBssInfo->ucOwnMacIndex;
    COPY_MAC_ADDR(rCmdActivateCtrl.aucBssMacAddr, prBssInfo->aucOwnMacAddr);

    prBssInfo->ucBMCWlanIndex = secPrivacySeekForBcEntry(
        prAdapter, prBssInfo->ucBssIndex, prBssInfo->aucOwnMacAddr,
        STA_REC_INDEX_NOT_FOUND, CIPHER_SUITE_NONE, 0xFF);

    prBssInfo->ucBMCWlanIndexSUsed[0] = true;

    rCmdActivateCtrl.ucBMCWlanIndex = prBssInfo->ucBMCWlanIndex;

    kalMemZero(&rCmdActivateCtrl.ucReserved,
               sizeof(rCmdActivateCtrl.ucReserved));

    DBGLOG(RSN, INFO, "[wlan index][Network]=%d activate=%d\n", ucBssIndex, 1);
    DBGLOG(RSN, INFO,
           "[wlan index][Network] OwnMac=" MACSTR " BSSID=" MACSTR
           " BMCIndex = %d NetType=%d\n",
           MAC2STR(prBssInfo->aucOwnMacAddr), MAC2STR(prBssInfo->aucBSSID),
           prBssInfo->ucBMCWlanIndex, prBssInfo->eNetworkType);

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_BSS_ACTIVATE_CTRL, true, false,
                               false, NULL, NULL, sizeof(CMD_BSS_ACTIVATE_CTRL),
                               (u8 *)&rCmdActivateCtrl, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to deactivate WIFISYS for specified
 * network
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        eNetworkTypeIdx    Index of network type
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicDeactivateNetwork(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex)
{
    WLAN_STATUS u4Status;
    CMD_BSS_ACTIVATE_CTRL rCmdActivateCtrl;
    P_BSS_INFO_T prBssInfo;

    ASSERT(prAdapter);
    ASSERT(IS_BSS_INDEX_VALID(ucBssIndex));

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    kalMemZero(&rCmdActivateCtrl, sizeof(CMD_BSS_ACTIVATE_CTRL));

    rCmdActivateCtrl.ucBssIndex = ucBssIndex;
    rCmdActivateCtrl.ucActive = 0;

    DBGLOG(RSN, INFO, "[wlan index][Network]=%d activate=%d\n", ucBssIndex, 0);
    DBGLOG(RSN, INFO,
           "[wlan index][Network] OwnMac=" MACSTR " BSSID=" MACSTR
           " BMCIndex = %d\n",
           MAC2STR(prBssInfo->aucOwnMacAddr), MAC2STR(prBssInfo->aucBSSID),
           prBssInfo->ucBMCWlanIndex);

    rCmdActivateCtrl.ucOwnMacAddrIndex = prBssInfo->ucOwnMacIndex;
    /* 20170628, if deactive bssid, do not reset NetworkType, otherwise we
     * cannot free bcn */
    rCmdActivateCtrl.ucNetworkType = (u8)prBssInfo->eNetworkType;

    u4Status = wlanSendSetQueryCmd(prAdapter, CMD_ID_BSS_ACTIVATE_CTRL, true,
                                   false, false, NULL, NULL,
                                   sizeof(CMD_BSS_ACTIVATE_CTRL),
                                   (u8 *)&rCmdActivateCtrl, NULL, 0);

    secRemoveBssBcEntry(prAdapter, prBssInfo, false);

    /* 20190301 To free all the correlated StaRec,
     * clients (StaRec) in BSS client list also need to be removed from list
     */
    bssInitializeClientList(prAdapter, prBssInfo);

    /* free all correlated station records */
    cnmStaFreeAllStaByNetwork(prAdapter, ucBssIndex, STA_REC_EXCLUDE_NONE);
    if (HAL_IS_TX_DIRECT(prAdapter)) {
        nicTxDirectClearBssAbsentQ(prAdapter, ucBssIndex);
    } else {
        qmFreeAllByBssIdx(prAdapter, ucBssIndex);
    }
    nicFreePendingTxMsduInfoByBssIdx(prAdapter, ucBssIndex);
    kalClearSecurityFramesByBssIdx(prAdapter->prGlueInfo, ucBssIndex);
#if (CFG_HW_WMM_BY_BSS == 1)
    cnmFreeWmmIndex(prAdapter, prBssInfo);
#endif
    return u4Status;
}

/* BSS-INFO */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to sync bss info with firmware
 *        when a new BSS has been connected or disconnected
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        ucBssIndex         Index of BSS-INFO
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicUpdateBss(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex)
{
    WLAN_STATUS u4Status = WLAN_STATUS_NOT_ACCEPTED;
    P_BSS_INFO_T prBssInfo;
    CMD_SET_BSS_INFO rCmdSetBssInfo;
    P_WIFI_VAR_T prWifiVar = &prAdapter->rWifiVar;

    ASSERT(prAdapter);
    ASSERT(ucBssIndex <= MAX_BSS_INDEX);

    prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

    if ((prBssInfo->eDBDCBand != ENUM_BAND_0) &&
        (prBssInfo->eDBDCBand != ENUM_BAND_1)) {
        DBGLOG(BSS, ERROR, "Wrong eDBDCBand - [%u]\n", prBssInfo->eDBDCBand);
        prBssInfo->eDBDCBand = ENUM_BAND_0;  /* Work around : temp
                                              * solution */
        /*ASSERT(0);*/                      /* FATAL ERROR */
    }

    kalMemZero(&rCmdSetBssInfo, sizeof(CMD_SET_BSS_INFO));

    rCmdSetBssInfo.ucBssIndex = ucBssIndex;
    rCmdSetBssInfo.ucConnectionState = (u8)prBssInfo->eConnectionState;
    rCmdSetBssInfo.ucCurrentOPMode = (u8)prBssInfo->eCurrentOPMode;
    rCmdSetBssInfo.ucSSIDLen = (u8)prBssInfo->ucSSIDLen;
    kalMemCopy(rCmdSetBssInfo.aucSSID, prBssInfo->aucSSID,
               prBssInfo->ucSSIDLen);
    COPY_MAC_ADDR(rCmdSetBssInfo.aucBSSID, prBssInfo->aucBSSID);
    rCmdSetBssInfo.ucIsQBSS = (u8)prBssInfo->fgIsQBSS;
    rCmdSetBssInfo.ucNonHTBasicPhyType = prBssInfo->ucNonHTBasicPhyType;
    rCmdSetBssInfo.u2OperationalRateSet = prBssInfo->u2OperationalRateSet;
    rCmdSetBssInfo.u2BSSBasicRateSet = prBssInfo->u2BSSBasicRateSet;
    rCmdSetBssInfo.u2HwDefaultFixedRateCode =
        prBssInfo->u2HwDefaultFixedRateCode;
    rCmdSetBssInfo.ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
    rCmdSetBssInfo.u4PrivateData = prBssInfo->u4PrivateData;
#if CFG_SUPPORT_DBDC
    rCmdSetBssInfo.ucDBDCBand = prBssInfo->eDBDCBand;
#endif
    rCmdSetBssInfo.ucWmmSet = prBssInfo->ucWmmQueSet;
    rCmdSetBssInfo.ucNss = prBssInfo->ucNss;

    if (prBssInfo->fgBcDefaultKeyExist) {
        if (prBssInfo->wepkeyUsed[prBssInfo->ucBcDefaultKeyIdx] &&
            prBssInfo->wepkeyWlanIdx < NIC_TX_DEFAULT_WLAN_INDEX) {
            rCmdSetBssInfo.ucBMCWlanIndex = prBssInfo->wepkeyWlanIdx;
        } else if (prBssInfo->ucBMCWlanIndexSUsed[prBssInfo->ucBcDefaultKeyIdx])
        {
            rCmdSetBssInfo.ucBMCWlanIndex =
                prBssInfo->ucBMCWlanIndexS[prBssInfo->ucBcDefaultKeyIdx];
        }
    } else {
        rCmdSetBssInfo.ucBMCWlanIndex = prBssInfo->ucBMCWlanIndex;
    }
    DBGLOG(RSN, TRACE, "Update BSS BMC WlanIdx %u\n",
           rCmdSetBssInfo.ucBMCWlanIndex);

#ifdef CFG_ENABLE_WIFI_DIRECT
    rCmdSetBssInfo.ucHiddenSsidMode = prBssInfo->eHiddenSsidType;
#endif
    rlmFillSyncCmdParam(&rCmdSetBssInfo.rBssRlmParam, prBssInfo);

    rCmdSetBssInfo.ucWapiMode = (u8)false;

    if ((prAdapter->prAisBssInfo != NULL) &&
        (rCmdSetBssInfo.ucBssIndex == prAdapter->prAisBssInfo->ucBssIndex)) {
        P_CONNECTION_SETTINGS_T prConnSettings =
            &(prAdapter->rWifiVar.rConnSettings);

        rCmdSetBssInfo.ucAuthMode = (u8)prConnSettings->eAuthMode;
        rCmdSetBssInfo.ucEncStatus = (u8)prConnSettings->eEncStatus;
        rCmdSetBssInfo.ucWapiMode = (u8)prConnSettings->fgWapiMode;
        rCmdSetBssInfo.ucDisconnectDetectTh =
            prWifiVar->ucStaDisconnectDetectTh;
    } else {
#if CFG_ENABLE_WIFI_DIRECT
        if (prAdapter->fgIsP2PRegistered) {
#if CFG_SUPPORT_SUITB
            if (kalP2PGetGcmp256Cipher(prAdapter->prGlueInfo,
                                       (u8)prBssInfo->u4PrivateData)) {
                rCmdSetBssInfo.ucAuthMode = (u8)AUTH_MODE_WPA2_PSK;
                rCmdSetBssInfo.ucEncStatus = (u8)ENUM_ENCRYPTION4_ENABLED;
            } else
#endif
            if (kalP2PGetCcmpCipher(prAdapter->prGlueInfo,
                                    (u8)prBssInfo->u4PrivateData)) {
                rCmdSetBssInfo.ucAuthMode = (u8)AUTH_MODE_WPA2_PSK;
                rCmdSetBssInfo.ucEncStatus = (u8)ENUM_ENCRYPTION3_ENABLED;
            } else if (kalP2PGetTkipCipher(prAdapter->prGlueInfo,
                                           (u8)prBssInfo->u4PrivateData)) {
                rCmdSetBssInfo.ucAuthMode = (u8)AUTH_MODE_WPA_PSK;
                rCmdSetBssInfo.ucEncStatus = (u8)ENUM_ENCRYPTION2_ENABLED;
            } else if (kalP2PGetWepCipher(prAdapter->prGlueInfo,
                                          (u8)prBssInfo->u4PrivateData)) {
                rCmdSetBssInfo.ucAuthMode = (u8)AUTH_MODE_OPEN;
                rCmdSetBssInfo.ucEncStatus = (u8)ENUM_ENCRYPTION1_ENABLED;
            } else {
                rCmdSetBssInfo.ucAuthMode = (u8)AUTH_MODE_OPEN;
                rCmdSetBssInfo.ucEncStatus = (u8)ENUM_ENCRYPTION_DISABLED;
            }
            /* Need the probe response to detect the PBC overlap */
            rCmdSetBssInfo.ucIsApMode = p2pFuncIsAPMode(
                prAdapter->rWifiVar.prP2PConnSettings[prBssInfo->u4PrivateData]);

            if (rCmdSetBssInfo.ucIsApMode) {
                rCmdSetBssInfo.ucDisconnectDetectTh =
                    prWifiVar->ucApDisconnectDetectTh;
            } else {
                rCmdSetBssInfo.ucDisconnectDetectTh =
                    prWifiVar->ucP2pDisconnectDetectTh;
            }
        }
#else
        rCmdSetBssInfo.ucAuthMode = (u8)AUTH_MODE_WPA2_PSK;
        rCmdSetBssInfo.ucEncStatus = (u8)ENUM_ENCRYPTION3_KEY_ABSENT;
#endif
    }

    if ((prAdapter->prAisBssInfo != NULL) &&
        (ucBssIndex == prAdapter->prAisBssInfo->ucBssIndex) &&
        (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) &&
        (prBssInfo->prStaRecOfAP != NULL)) {
        rCmdSetBssInfo.ucStaRecIdxOfAP = prBssInfo->prStaRecOfAP->ucIndex;
    }
#if CFG_ENABLE_WIFI_DIRECT
    else if ((prAdapter->fgIsP2PRegistered) &&
             (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) &&
             (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) &&
             (prBssInfo->prStaRecOfAP != NULL)) {
        rCmdSetBssInfo.ucStaRecIdxOfAP = prBssInfo->prStaRecOfAP->ucIndex;
    }
#endif

    else {
        rCmdSetBssInfo.ucStaRecIdxOfAP = STA_REC_INDEX_NOT_FOUND;
    }

    DBGLOG(BSS, INFO,
           "Update Bss[%u] ConnState[%u] OPmode[%u] BSSID[" MACSTR
           "] AuthMode[%u] EncStatus[%u]\n",
           ucBssIndex, prBssInfo->eConnectionState, prBssInfo->eCurrentOPMode,
           MAC2STR(prBssInfo->aucBSSID), rCmdSetBssInfo.ucAuthMode,
           rCmdSetBssInfo.ucEncStatus);

    u4Status = wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_BSS_INFO, true, false,
                                   false, NULL, NULL, sizeof(CMD_SET_BSS_INFO),
                                   (u8 *)&rCmdSetBssInfo, NULL, 0);

    /* if BSS-INFO is going to be disconnected state, free all correlated
     * station records */
    if (prBssInfo->eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED) {
        /* clear client list */
        bssInitializeClientList(prAdapter, prBssInfo);

#if DBG
        DBGLOG(BSS, TRACE, "nicUpdateBss for disconnect state\n");
#endif
        /* free all correlated station records */
        cnmStaFreeAllStaByNetwork(prAdapter, ucBssIndex, STA_REC_EXCLUDE_NONE);
        if (HAL_IS_TX_DIRECT(prAdapter)) {
            nicTxDirectClearBssAbsentQ(prAdapter, ucBssIndex);
        } else {
            qmFreeAllByBssIdx(prAdapter, ucBssIndex);
        }
        kalClearSecurityFramesByBssIdx(prAdapter->prGlueInfo, ucBssIndex);

#if CFG_SUPPORT_DBDC
        cnmDbdcDisableDecision(prAdapter, ucBssIndex);
#endif
    }

    return u4Status;
}

/* BSS-INFO Indication (PM) */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to indicate PM that
 *        a BSS has been created. (for AdHoc / P2P-GO)
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        ucBssIndex         Index of BSS-INFO
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicPmIndicateBssCreated(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex)
{
    P_BSS_INFO_T prBssInfo;
    CMD_INDICATE_PM_BSS_CREATED rCmdIndicatePmBssCreated;

    kalMemZero(&rCmdIndicatePmBssCreated, sizeof(CMD_INDICATE_PM_BSS_CREATED));

    ASSERT(prAdapter);
    ASSERT(ucBssIndex <= MAX_BSS_INDEX);

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    rCmdIndicatePmBssCreated.ucBssIndex = ucBssIndex;
    rCmdIndicatePmBssCreated.ucDtimPeriod = prBssInfo->ucDTIMPeriod;
    rCmdIndicatePmBssCreated.u2BeaconInterval = prBssInfo->u2BeaconInterval;
    rCmdIndicatePmBssCreated.u2AtimWindow = prBssInfo->u2ATIMWindow;

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_INDICATE_PM_BSS_CREATED, true,
                               false, false, NULL, NULL,
                               sizeof(CMD_INDICATE_PM_BSS_CREATED),
                               (u8 *)&rCmdIndicatePmBssCreated, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to indicate PM that
 *        a BSS has been connected
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        eNetworkTypeIdx    Index of BSS-INFO
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicPmIndicateBssConnected(IN P_ADAPTER_T prAdapter,
                                      IN u8 ucBssIndex)
{
    P_BSS_INFO_T prBssInfo;
    CMD_INDICATE_PM_BSS_CONNECTED rCmdIndicatePmBssConnected;

    kalMemZero(&rCmdIndicatePmBssConnected,
               sizeof(CMD_INDICATE_PM_BSS_CONNECTED));

    ASSERT(prAdapter);
    ASSERT(ucBssIndex <= MAX_BSS_INDEX);

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    rCmdIndicatePmBssConnected.ucBssIndex = ucBssIndex;
    rCmdIndicatePmBssConnected.ucDtimPeriod = prBssInfo->ucDTIMPeriod;
    rCmdIndicatePmBssConnected.u2AssocId = prBssInfo->u2AssocId;
    rCmdIndicatePmBssConnected.u2BeaconInterval = prBssInfo->u2BeaconInterval;
    rCmdIndicatePmBssConnected.u2AtimWindow = prBssInfo->u2ATIMWindow;

    rCmdIndicatePmBssConnected.ucBmpDeliveryAC =
        prBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC;
    rCmdIndicatePmBssConnected.ucBmpTriggerAC =
        prBssInfo->rPmProfSetupInfo.ucBmpTriggerAC;

    /* DBGPRINTF("nicPmIndicateBssConnected: ucBmpDeliveryAC:0x%x,
     * ucBmpTriggerAC:0x%x", */
    /* rCmdIndicatePmBssConnected.ucBmpDeliveryAC, */
    /* rCmdIndicatePmBssConnected.ucBmpTriggerAC); */

    if ((GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType ==
         NETWORK_TYPE_AIS)
#if CFG_ENABLE_WIFI_DIRECT
        || ((GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType ==
             NETWORK_TYPE_P2P) &&
            (prAdapter->fgIsP2PRegistered))
#endif
        ) {
        if (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
            rCmdIndicatePmBssConnected.fgIsUapsdConnection =
                (u8)prBssInfo->prStaRecOfAP->fgIsUapsdSupported;
        } else {
            rCmdIndicatePmBssConnected.fgIsUapsdConnection = 0;  /* @FIXME */
        }
    } else {
        rCmdIndicatePmBssConnected.fgIsUapsdConnection = 0;
    }

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_INDICATE_PM_BSS_CONNECTED,
                               true, false, false, NULL, NULL,
                               sizeof(CMD_INDICATE_PM_BSS_CONNECTED),
                               (u8 *)&rCmdIndicatePmBssConnected, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to indicate PM that
 *        a BSS has been disconnected
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        ucBssIndex         Index of BSS-INFO
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicPmIndicateBssAbort(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex)
{
    CMD_INDICATE_PM_BSS_ABORT rCmdIndicatePmBssAbort;

    kalMemZero(&rCmdIndicatePmBssAbort, sizeof(CMD_INDICATE_PM_BSS_ABORT));

    ASSERT(prAdapter);
    ASSERT(ucBssIndex <= MAX_BSS_INDEX);

    rCmdIndicatePmBssAbort.ucBssIndex = ucBssIndex;

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_INDICATE_PM_BSS_ABORT, true,
                               false, false, NULL, NULL,
                               sizeof(CMD_INDICATE_PM_BSS_ABORT),
                               (u8 *)&rCmdIndicatePmBssAbort, NULL, 0);
}

WLAN_STATUS
nicConfigPowerSaveProfile(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                          IN PARAM_POWER_MODE ePwrMode, IN u8 fgEnCmdEvent)
{
    DEBUGFUNC("nicConfigPowerSaveProfile");
    DBGLOG(INIT, TRACE, "ucBssIndex:%d, ePwrMode:%d, fgEnCmdEvent:%d\n",
           ucBssIndex, ePwrMode, fgEnCmdEvent);

    ASSERT(prAdapter);

    if (ucBssIndex >= BSS_INFO_NUM) {
        ASSERT(0);
        return WLAN_STATUS_NOT_SUPPORTED;
    }

    prAdapter->rWlanInfo.arPowerSaveMode[ucBssIndex].ucBssIndex =
        ucBssIndex;
    prAdapter->rWlanInfo.arPowerSaveMode[ucBssIndex].ucPsProfile =
        (u8)ePwrMode;
#ifdef SUPPORT_PERIODIC_PS
    /* Always given these two field should be fine for other PSP. */
    prAdapter->rWlanInfo.arPowerSaveMode[ucBssIndex].ucPspCAMInt =
        prAdapter->rWifiVar.ucAwakePspCAMInt;
    prAdapter->rWlanInfo.arPowerSaveMode[ucBssIndex].ucPspPSInt =
        prAdapter->rWifiVar.ucAwakePspPSInt;
#endif

    return wlanSendSetQueryCmd(
        prAdapter, CMD_ID_POWER_SAVE_MODE, true, false, true,
        (fgEnCmdEvent ? nicCmdEventSetCommon : NULL),
        (fgEnCmdEvent ? nicOidCmdTimeoutCommon : NULL),
        sizeof(CMD_PS_PROFILE_T),
        (u8 *)&(prAdapter->rWlanInfo.arPowerSaveMode[ucBssIndex]), NULL,
        sizeof(PARAM_POWER_MODE));
}

WLAN_STATUS
nicConfigPowerSaveWowProfile(IN P_ADAPTER_T prAdapter, u8 ucBssIndex,
                             PARAM_POWER_MODE ePwrMode, u8 fgEnCmdEvent,
                             u8 fgSuspend)
{
    CMD_PS_PROFILE_T rPowerSaveMode;

    kalMemZero(&rPowerSaveMode, sizeof(CMD_PS_PROFILE_T));

    if (fgSuspend) {
        rPowerSaveMode.ucBssIndex = ucBssIndex;
        rPowerSaveMode.ucPsProfile = ePwrMode;
#ifdef SUPPORT_PERIODIC_PS
        rPowerSaveMode.ucPspCAMInt = prAdapter->rWifiVar.ucPspCAMInt;
        rPowerSaveMode.ucPspPSInt = prAdapter->rWifiVar.ucPspPSInt;
#endif
    } else {
        /* if resume, restore power save profile */
        DBGLOG(HAL, STATE, "Resume wow power save idx:%d, mode:%d\n",
               ucBssIndex,
               prAdapter->rWlanInfo.arPowerSaveMode[ucBssIndex].ucPsProfile);
        kalMemCopy(&rPowerSaveMode,
                   &(prAdapter->rWlanInfo.arPowerSaveMode[ucBssIndex]),
                   sizeof(rPowerSaveMode));
    }

    /* if suspend, config power save mode w/o update
     * arPowerSaveMode[ucBssIndex] */
    return wlanSendSetQueryCmd(prAdapter, CMD_ID_POWER_SAVE_MODE, true, false,
                               true,
                               (fgEnCmdEvent ? nicCmdEventSetCommon : NULL),
                               (fgEnCmdEvent ? nicOidCmdTimeoutCommon : NULL),
                               sizeof(CMD_PS_PROFILE_T), (u8 *)&rPowerSaveMode,
                               NULL, sizeof(PARAM_POWER_MODE));
}

WLAN_STATUS nicEnterCtiaMode(IN P_ADAPTER_T prAdapter, u8 fgEnterCtia,
                             u8 fgEnCmdEvent)
{
    CMD_SW_DBG_CTRL_T rCmdSwCtrl;
    /* CMD_ACCESS_REG rCmdAccessReg; */
    WLAN_STATUS rWlanStatus;

    kalMemZero(&rCmdSwCtrl, sizeof(CMD_SW_DBG_CTRL_T));

    DEBUGFUNC("nicEnterCtiaMode");
    DBGLOG(INIT, TRACE, "nicEnterCtiaMode: %d\n", fgEnterCtia);

    ASSERT(prAdapter);

    rWlanStatus = WLAN_STATUS_SUCCESS;

    if (fgEnterCtia) {
        /* 1. Disable On-Lin Scan */
        prAdapter->fgEnOnlineScan = false;

        /* 2. Disable FIFO FULL no ack */
        /* 3. Disable Roaming */
        /* 4. Disalbe auto tx power */
        rCmdSwCtrl.u4Id = 0xa0100003;
        rCmdSwCtrl.u4Data = 0x0;
        wlanSendSetQueryCmd(prAdapter, CMD_ID_SW_DBG_CTRL, true, false, false,
                            NULL, NULL, sizeof(CMD_SW_DBG_CTRL_T),
                            (u8 *)&rCmdSwCtrl, NULL, 0);

        /* 2. Keep at CAM mode */
        {
            PARAM_POWER_MODE ePowerMode;

            prAdapter->u4CtiaPowerMode = 0;
            prAdapter->fgEnCtiaPowerMode = true;

            ePowerMode = Param_PowerModeCAM;
            rWlanStatus = nicConfigPowerSaveProfile(
                prAdapter, prAdapter->prAisBssInfo->ucBssIndex, ePowerMode,
                fgEnCmdEvent);
        }

        /* 5. Disable Beacon Timeout Detection */
        prAdapter->fgDisBcnLostDetection = true;
    } else {
        /* 1. Enaable On-Lin Scan */
        prAdapter->fgEnOnlineScan = true;

        /* 2. Enable FIFO FULL no ack */
        /* 3. Enable Roaming */
        /* 4. Enable auto tx power */
        /*  */

        rCmdSwCtrl.u4Id = 0xa0100003;
        rCmdSwCtrl.u4Data = 0x1;
        wlanSendSetQueryCmd(prAdapter, CMD_ID_SW_DBG_CTRL, true, false, false,
                            NULL, NULL, sizeof(CMD_SW_DBG_CTRL_T),
                            (u8 *)&rCmdSwCtrl, NULL, 0);

        /* 2. Keep at Fast PS */
        {
            PARAM_POWER_MODE ePowerMode;

            prAdapter->u4CtiaPowerMode = 2;
            prAdapter->fgEnCtiaPowerMode = true;

            ePowerMode = Param_PowerModeFast_PSP;
            rWlanStatus = nicConfigPowerSaveProfile(
                prAdapter, prAdapter->prAisBssInfo->ucBssIndex, ePowerMode,
                fgEnCmdEvent);
        }

        /* 5. Enable Beacon Timeout Detection */
        prAdapter->fgDisBcnLostDetection = false;
    }

    return rWlanStatus;
}

WLAN_STATUS nicEnterTPTestMode(IN P_ADAPTER_T prAdapter, IN u8 ucFuncMask)
{
    CMD_SW_DBG_CTRL_T rCmdSwCtrl;
    WLAN_STATUS rWlanStatus;
    u8 ucBssIdx;
    P_BSS_INFO_T prBssInfo;

    kalMemZero(&rCmdSwCtrl, sizeof(CMD_SW_DBG_CTRL_T));

    ASSERT(prAdapter);

    rWlanStatus = WLAN_STATUS_SUCCESS;

    if (ucFuncMask) {
        /* 1. Disable On-Lin Scan */
        if (ucFuncMask & TEST_MODE_DISABLE_ONLINE_SCAN) {
            prAdapter->fgEnOnlineScan = false;
        }

        /* 2. Disable Roaming */
        if (ucFuncMask & TEST_MODE_DISABLE_ROAMING) {
            rCmdSwCtrl.u4Id = 0xa0210000;
            rCmdSwCtrl.u4Data = 0x0;
            wlanSendSetQueryCmd(prAdapter, CMD_ID_SW_DBG_CTRL, true, false,
                                false, NULL, NULL, sizeof(CMD_SW_DBG_CTRL_T),
                                (u8 *)&rCmdSwCtrl, NULL, 0);
        }
        /* 3. Keep at CAM mode */
        if (ucFuncMask & TEST_MODE_FIXED_CAM_MODE) {
            for (ucBssIdx = 0; ucBssIdx < BSS_INFO_NUM; ucBssIdx++) {
                prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
                if (prBssInfo->fgIsInUse &&
                    (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE)) {
                    nicConfigPowerSaveProfile(prAdapter, ucBssIdx,
                                              Param_PowerModeCAM, false);
                }
            }
        }

        /* 4. Disable Beacon Timeout Detection */
        if (ucFuncMask & TEST_MODE_DISABLE_BCN_LOST_DET) {
            prAdapter->fgDisBcnLostDetection = true;
        }
    } else {
        /* 1. Enaable On-Lin Scan */
        prAdapter->fgEnOnlineScan = true;

        /* 2. Enable Roaming */
        rCmdSwCtrl.u4Id = 0xa0210000;
        rCmdSwCtrl.u4Data = 0x1;
        wlanSendSetQueryCmd(prAdapter, CMD_ID_SW_DBG_CTRL, true, false, false,
                            NULL, NULL, sizeof(CMD_SW_DBG_CTRL_T),
                            (u8 *)&rCmdSwCtrl, NULL, 0);

        /* 3. Keep at Fast PS */
        for (ucBssIdx = 0; ucBssIdx < BSS_INFO_NUM; ucBssIdx++) {
            prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
            if (prBssInfo->fgIsInUse &&
                (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE)) {
                nicConfigPowerSaveProfile(prAdapter, ucBssIdx,
                                          Param_PowerModeFast_PSP, false);
            }
        }

        /* 4. Enable Beacon Timeout Detection */
        prAdapter->fgDisBcnLostDetection = false;
    }

    return rWlanStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to indicate firmware domain
 *        for beacon generation parameters
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        eIeUpdMethod,      Update Method
 *        ucBssIndex         Index of BSS-INFO
 *        u2Capability       Capability
 *        aucIe              Pointer to buffer of IEs
 *        u2IELen            Length of IEs
 *
 * @retval - WLAN_STATUS_SUCCESS
 *           WLAN_STATUS_FAILURE
 *           WLAN_STATUS_PENDING
 *           WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
nicUpdateBeaconIETemplate(IN P_ADAPTER_T prAdapter,
                          IN ENUM_IE_UPD_METHOD_T eIeUpdMethod,
                          IN u8 ucBssIndex, IN u16 u2Capability, IN u8 *aucIe,
                          IN u16 u2IELen)
{
    P_CMD_BEACON_TEMPLATE_UPDATE prCmdBcnUpdate;
    u16 u2CmdBufLen = 0;
    P_GLUE_INFO_T prGlueInfo;
    P_CMD_INFO_T prCmdInfo;
    P_WIFI_CMD_T prWifiCmd;
    u8 ucCmdSeqNum;

    DEBUGFUNC("wlanUpdateBeaconIETemplate");
    DBGLOG(INIT, LOUD, "\n");

    ASSERT(prAdapter);
    prGlueInfo = prAdapter->prGlueInfo;

    if (u2IELen > MAX_IE_LENGTH) {
        return WLAN_STATUS_INVALID_DATA;
    }

    if (eIeUpdMethod == IE_UPD_METHOD_UPDATE_RANDOM ||
        eIeUpdMethod == IE_UPD_METHOD_UPDATE_ALL) {
        u2CmdBufLen = OFFSET_OF(CMD_BEACON_TEMPLATE_UPDATE, aucIE) + u2IELen;
    } else if (eIeUpdMethod == IE_UPD_METHOD_DELETE_ALL) {
        u2CmdBufLen = OFFSET_OF(CMD_BEACON_TEMPLATE_UPDATE, u2IELen);
    } else {
        DBGLOG(INIT, ERROR, "Unknown IeUpdMethod.\n");
        return WLAN_STATUS_FAILURE;
    }

    /* prepare command info */
    prCmdInfo = cmdBufAllocateCmdInfo(prAdapter, (CMD_HDR_SIZE + u2CmdBufLen));
    if (!prCmdInfo) {
        DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
        return WLAN_STATUS_FAILURE;
    }
    /* increase command sequence number */
    ucCmdSeqNum = nicIncreaseCmdSeqNum(prAdapter);
    DBGLOG(REQ, TRACE, "ucCmdSeqNum =%d\n", ucCmdSeqNum);

    /* Setup common CMD Info Packet */
    prCmdInfo->eCmdType = COMMAND_TYPE_NETWORK_IOCTL;
    prCmdInfo->u2InfoBufLen = (u16)(CMD_HDR_SIZE + u2CmdBufLen);
    prCmdInfo->pfCmdDoneHandler = NULL;  /* @FIXME */
    prCmdInfo->pfCmdTimeoutHandler = NULL;  /* @FIXME */
    prCmdInfo->fgIsOid = false;
    prCmdInfo->ucCID = CMD_ID_UPDATE_BEACON_CONTENT;
    prCmdInfo->fgSetQuery = true;
    prCmdInfo->fgNeedResp = false;
    prCmdInfo->ucCmdSeqNum = ucCmdSeqNum;
    prCmdInfo->u4SetInfoLen = u2CmdBufLen;
    prCmdInfo->pvInformationBuffer = NULL;
    prCmdInfo->u4InformationBufferLength = 0;

    /* Setup WIFI_CMD_T (no payload) */
    prWifiCmd = (P_WIFI_CMD_T)(prCmdInfo->pucInfoBuffer);
    prWifiCmd->u2TxByteCount = prCmdInfo->u2InfoBufLen;
    prWifiCmd->u2PQ_ID = CMD_PQ_ID;
    prWifiCmd->ucPktTypeID = CMD_PACKET_TYPE_ID;
    prWifiCmd->ucCID = prCmdInfo->ucCID;
    prWifiCmd->ucSetQuery = prCmdInfo->fgSetQuery;
    prWifiCmd->ucSeqNum = prCmdInfo->ucCmdSeqNum;

    prCmdBcnUpdate = (P_CMD_BEACON_TEMPLATE_UPDATE)(prWifiCmd->aucBuffer);

    /* fill beacon updating command */
    prCmdBcnUpdate->ucUpdateMethod = (u8)eIeUpdMethod;
    prCmdBcnUpdate->ucBssIndex = ucBssIndex;
    prCmdBcnUpdate->u2Capability = u2Capability;
    prCmdBcnUpdate->u2IELen = u2IELen;
    if (u2IELen > 0) {
        kalMemCopy(prCmdBcnUpdate->aucIE, aucIe, u2IELen);
    }
    /* insert into prCmdQueue */
    kalEnqueueCommand(prGlueInfo, (P_QUE_ENTRY_T)prCmdInfo);

    /* wakeup txServiceThread later */
    GLUE_SET_EVENT(prGlueInfo);
    return WLAN_STATUS_PENDING;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to initialization PHY related
 *        varaibles
 *
 * @param prAdapter  Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicSetAvailablePhyTypeSet(IN P_ADAPTER_T prAdapter)
{
    P_CONNECTION_SETTINGS_T prConnSettings;

    ASSERT(prAdapter);

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

    if (prConnSettings->eDesiredPhyConfig >= PHY_CONFIG_NUM) {
        ASSERT(0);
        return;
    }

    prAdapter->rWifiVar.ucAvailablePhyTypeSet =
        aucPhyCfg2PhyTypeSet[prConnSettings->eDesiredPhyConfig];

    if (prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_BIT_ERP) {
        prAdapter->rWifiVar.eNonHTBasicPhyType2G4 = PHY_TYPE_ERP_INDEX;
    }
    /* NOTE(Kevin): Because we don't have N only mode, TBD */
    else {  /* if (ucNonHTPhyTypeSet & PHY_TYPE_HR_DSSS_INDEX) */
        prAdapter->rWifiVar.eNonHTBasicPhyType2G4 = PHY_TYPE_HR_DSSS_INDEX;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update WMM Parms
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        ucBssIndex         Index of BSS-INFO
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicQmUpdateWmmParms(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex)
{
    P_BSS_INFO_T prBssInfo;
    CMD_UPDATE_WMM_PARMS_T rCmdUpdateWmmParms;

    kalMemZero(&rCmdUpdateWmmParms, sizeof(CMD_UPDATE_WMM_PARMS_T));

    ASSERT(prAdapter);

    DBGLOG(QM, INFO, "Update WMM parameters for BSS[%u]\n", ucBssIndex);

    DBGLOG(QM, EVENT, "sizeof(AC_QUE_PARMS_T): %d\n", sizeof(AC_QUE_PARMS_T));
    DBGLOG(QM, EVENT, "sizeof(CMD_UPDATE_WMM_PARMS): %d\n",
           sizeof(CMD_UPDATE_WMM_PARMS_T));
    DBGLOG(QM, EVENT, "sizeof(WIFI_CMD_T): %d\n", sizeof(WIFI_CMD_T));

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
    rCmdUpdateWmmParms.ucBssIndex = (u8)ucBssIndex;
    kalMemCopy(&rCmdUpdateWmmParms.arACQueParms[0], &prBssInfo->arACQueParms[0],
               (sizeof(AC_QUE_PARMS_T) * AC_NUM));

    rCmdUpdateWmmParms.fgIsQBSS = prBssInfo->fgIsQBSS;
    rCmdUpdateWmmParms.ucWmmSet = (u8)prBssInfo->ucWmmQueSet;

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_UPDATE_WMM_PARMS, true, false,
                               false, NULL, NULL,
                               sizeof(CMD_UPDATE_WMM_PARMS_T),
                               (u8 *)&rCmdUpdateWmmParms, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update TX power gain corresponding to
 *        each band/modulation combination
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        prTxPwrParam       Pointer of TX power parameters
 *
 * @retval WLAN_STATUS_PENDING
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicUpdateTxPower(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_TX_PWR_T prTxPwrParam)
{
    DEBUGFUNC("nicUpdateTxPower");

    ASSERT(prAdapter);

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_TX_PWR, true, false, false,
                               NULL, NULL, sizeof(CMD_TX_PWR_T),
                               (u8 *)prTxPwrParam, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to set auto tx power parameter
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        prTxPwrParam       Pointer of Auto TX power parameters
 *
 * @retval WLAN_STATUS_PENDING
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicSetAutoTxPower(IN P_ADAPTER_T prAdapter,
                              IN P_CMD_AUTO_POWER_PARAM_T prAutoPwrParam)
{
    DEBUGFUNC("nicSetAutoTxPower");

    ASSERT(prAdapter);

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_AUTOPWR_CTRL, true, false,
                               false, NULL, NULL,
                               sizeof(CMD_AUTO_POWER_PARAM_T),
                               (u8 *)prAutoPwrParam, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update TX power gain corresponding to
 *        each band/modulation combination
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        prTxPwrParam       Pointer of TX power parameters
 *
 * @retval WLAN_STATUS_PENDING
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicSetAutoTxPowerControl(IN P_ADAPTER_T prAdapter,
                                     IN P_CMD_TX_PWR_T prTxPwrParam)
{
    DEBUGFUNC("nicUpdateTxPower");

    ASSERT(prAdapter);

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_TX_PWR, true, false, false,
                               NULL, NULL, sizeof(CMD_TX_PWR_T),
                               (u8 *)prTxPwrParam, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update power offset around 5GHz band
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        pr5GPwrOffset      Pointer of 5GHz power offset parameter
 *
 * @retval WLAN_STATUS_PENDING
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicUpdate5GOffset(IN P_ADAPTER_T prAdapter,
                              IN P_CMD_5G_PWR_OFFSET_T pr5GPwrOffset)
{
    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update DPD calibration result
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        pr5GPwrOffset      Pointer of parameter for DPD calibration result
 *
 * @retval WLAN_STATUS_PENDING
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicUpdateDPD(IN P_ADAPTER_T prAdapter,
                         IN P_CMD_PWR_PARAM_T prDpdCalResult)
{
    DEBUGFUNC("nicUpdateDPD");

    ASSERT(prAdapter);

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_PWR_PARAM, true, false,
                               false, NULL, NULL, sizeof(CMD_PWR_PARAM_T),
                               (u8 *)prDpdCalResult, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function starts system service such as timer and
 *        memory pools
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicInitSystemService(IN P_ADAPTER_T prAdapter)
{
    ASSERT(prAdapter);

    /* <1> Initialize MGMT Memory pool and STA_REC */
    cnmMemInit(prAdapter);
    cnmStaRecInit(prAdapter);
    cmdBufInitialize(prAdapter);

    /* <2> Mailbox Initialization */
    mboxInitialize(prAdapter);

    /* <3> Timer Initialization */
    cnmTimerInitialize(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function reset some specific system service,
 *        such as STA-REC
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicResetSystemService(IN P_ADAPTER_T prAdapter)
{
    ASSERT(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update WMM Parms
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicUninitSystemService(IN P_ADAPTER_T prAdapter)
{
    ASSERT(prAdapter);

    /* Timer Destruction */
    cnmTimerDestroy(prAdapter);

    /* Mailbox Destruction */
    mboxDestroy(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update WMM Parms
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicInitMGMT(IN P_ADAPTER_T prAdapter, IN P_REG_INFO_T prRegInfo)
{
    ASSERT(prAdapter);

    /* CNM Module - initialization */
    cnmInit(prAdapter);

    /* RLM Module - initialization */
    rlmFsmEventInit(prAdapter);

    /* SCN Module - initialization */
    scnInit(prAdapter);

    /* AIS Module - intiailization */
    aisFsmInit(prAdapter);
    aisInitializeConnectionSettings(prAdapter, prRegInfo);

#if CFG_SUPPORT_ROAMING
    /* Roaming Module - intiailization */
    roamingFsmInit(prAdapter);
#endif

#if CFG_SUPPORT_SWCR
    swCrDebugInit(prAdapter);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update WMM Parms
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicUninitMGMT(IN P_ADAPTER_T prAdapter)
{
    ASSERT(prAdapter);

#if CFG_SUPPORT_SWCR
    swCrDebugUninit(prAdapter);
#endif

#if CFG_SUPPORT_ROAMING
    /* Roaming Module - unintiailization */
    roamingFsmUninit(prAdapter);
#endif

    /* AIS Module - unintiailization */
    aisFsmUninit(prAdapter);

    /* SCN Module - unintiailization */
    scnUninit(prAdapter);

    /* RLM Module - uninitialization */
    rlmFsmEventUninit(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is invoked to buffer scan result
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param rMacAddr           BSSID
 * @param prSsid             Pointer to SSID
 * @param u4Privacy          Privacy settings (0: Open / 1: WEP/WPA/WPA2
 * enabled)
 * @param rRssi              Received Strength (-10 ~ -200 dBm)
 * @param eNetworkType       Network Type (a/b/g)
 * @param prConfiguration    Network Parameter
 * @param eOpMode            Infra/Ad-Hoc
 * @param rSupportedRates    Supported basic rates
 * @param u2IELength         IE Length
 * @param pucIEBuf           Pointer to Information Elements(IEs)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicAddScanResult(IN P_ADAPTER_T prAdapter, IN PARAM_MAC_ADDRESS rMacAddr,
                      IN P_PARAM_SSID_T prSsid, IN u32 u4Privacy,
                      IN PARAM_RSSI rRssi,
                      IN ENUM_PARAM_NETWORK_TYPE_T eNetworkType,
                      IN P_PARAM_802_11_CONFIG_T prConfiguration,
                      IN ENUM_PARAM_OP_MODE_T eOpMode,
                      IN PARAM_RATES_EX rSupportedRates, IN u16 u2IELength,
                      IN u8 *pucIEBuf)
{
    u8 bReplace;
    u32 i;
    u32 u4IdxWeakest = 0;
    PARAM_RSSI rWeakestRssi;
    u32 u4BufferSize;

    ASSERT(prAdapter);

    rWeakestRssi = (PARAM_RSSI)INT_MAX;
    u4BufferSize = ARRAY_SIZE(prAdapter->rWlanInfo.aucScanIEBuf);

    bReplace = false;

    /* decide to replace or add */
    for (i = 0; i < prAdapter->rWlanInfo.u4ScanResultNum; i++) {
        /* find weakest entry && not connected one */
        if (UNEQUAL_MAC_ADDR(prAdapter->rWlanInfo.arScanResult[i].arMacAddress,
                             prAdapter->rWlanInfo.rCurrBssId.arMacAddress) &&
            prAdapter->rWlanInfo.arScanResult[i].rRssi < rWeakestRssi) {
            u4IdxWeakest = i;
            rWeakestRssi = prAdapter->rWlanInfo.arScanResult[i].rRssi;
        }

        if (prAdapter->rWlanInfo.arScanResult[i].eOpMode == eOpMode &&
            EQUAL_MAC_ADDR(&(prAdapter->rWlanInfo.arScanResult[i].arMacAddress),
                           rMacAddr) &&
            (EQUAL_SSID(prAdapter->rWlanInfo.arScanResult[i].rSsid.aucSsid,
                        prAdapter->rWlanInfo.arScanResult[i].rSsid.u4SsidLen,
                        prSsid->aucSsid, prSsid->u4SsidLen) ||
             prAdapter->rWlanInfo.arScanResult[i].rSsid.u4SsidLen == 0)) {
            /* replace entry */
            bReplace = true;

            /* free IE buffer then zero */
            nicFreeScanResultIE(prAdapter, i);
            kalMemZero(&(prAdapter->rWlanInfo.arScanResult[i]),
                       OFFSET_OF(PARAM_BSSID_EX_T, aucIEs));

            /* then fill buffer */
            prAdapter->rWlanInfo.arScanResult[i].u4Length =
                OFFSET_OF(PARAM_BSSID_EX_T, aucIEs) + u2IELength;
            COPY_MAC_ADDR(prAdapter->rWlanInfo.arScanResult[i].arMacAddress,
                          rMacAddr);
            COPY_SSID(prAdapter->rWlanInfo.arScanResult[i].rSsid.aucSsid,
                      prAdapter->rWlanInfo.arScanResult[i].rSsid.u4SsidLen,
                      prSsid->aucSsid, prSsid->u4SsidLen);
            prAdapter->rWlanInfo.arScanResult[i].u4Privacy = u4Privacy;
            prAdapter->rWlanInfo.arScanResult[i].rRssi = rRssi;
            prAdapter->rWlanInfo.arScanResult[i].eNetworkTypeInUse =
                eNetworkType;
            kalMemCopy(&(prAdapter->rWlanInfo.arScanResult[i].rConfiguration),
                       prConfiguration, sizeof(PARAM_802_11_CONFIG_T));
            prAdapter->rWlanInfo.arScanResult[i].eOpMode = eOpMode;
            kalMemCopy((prAdapter->rWlanInfo.arScanResult[i].rSupportedRates),
                       rSupportedRates, sizeof(PARAM_RATES_EX));
            prAdapter->rWlanInfo.arScanResult[i].u4IELength = (u32)u2IELength;

            /* IE - allocate buffer and update pointer */
            if (u2IELength > 0) {
                if (ALIGN_4(u2IELength) +
                    prAdapter->rWlanInfo.u4ScanIEBufferUsage <=
                    u4BufferSize) {
                    kalMemCopy(&(prAdapter->rWlanInfo
                                 .aucScanIEBuf[prAdapter->rWlanInfo
                                               .u4ScanIEBufferUsage]),
                               pucIEBuf, u2IELength);

                    prAdapter->rWlanInfo.apucScanResultIEs[i] =
                        &(prAdapter->rWlanInfo.aucScanIEBuf
                          [prAdapter->rWlanInfo.u4ScanIEBufferUsage]);

                    prAdapter->rWlanInfo.u4ScanIEBufferUsage +=
                        ALIGN_4(u2IELength);
                } else {
                    /* buffer is not enough */
                    prAdapter->rWlanInfo.arScanResult[i].u4Length -= u2IELength;
                    prAdapter->rWlanInfo.arScanResult[i].u4IELength = 0;
                    prAdapter->rWlanInfo.apucScanResultIEs[i] = NULL;
                }
            } else {
                prAdapter->rWlanInfo.apucScanResultIEs[i] = NULL;
            }

            break;
        }
    }

    if (bReplace == false) {
        if (prAdapter->rWlanInfo.u4ScanResultNum < (CFG_MAX_NUM_BSS_LIST - 1)) {
            i = prAdapter->rWlanInfo.u4ScanResultNum;

            /* zero */
            kalMemZero(&(prAdapter->rWlanInfo.arScanResult[i]),
                       OFFSET_OF(PARAM_BSSID_EX_T, aucIEs));

            /* then fill buffer */
            prAdapter->rWlanInfo.arScanResult[i].u4Length =
                OFFSET_OF(PARAM_BSSID_EX_T, aucIEs) + u2IELength;
            COPY_MAC_ADDR(prAdapter->rWlanInfo.arScanResult[i].arMacAddress,
                          rMacAddr);
            COPY_SSID(prAdapter->rWlanInfo.arScanResult[i].rSsid.aucSsid,
                      prAdapter->rWlanInfo.arScanResult[i].rSsid.u4SsidLen,
                      prSsid->aucSsid, prSsid->u4SsidLen);
            prAdapter->rWlanInfo.arScanResult[i].u4Privacy = u4Privacy;
            prAdapter->rWlanInfo.arScanResult[i].rRssi = rRssi;
            prAdapter->rWlanInfo.arScanResult[i].eNetworkTypeInUse =
                eNetworkType;
            kalMemCopy(&(prAdapter->rWlanInfo.arScanResult[i].rConfiguration),
                       prConfiguration, sizeof(PARAM_802_11_CONFIG_T));
            prAdapter->rWlanInfo.arScanResult[i].eOpMode = eOpMode;
            kalMemCopy((prAdapter->rWlanInfo.arScanResult[i].rSupportedRates),
                       rSupportedRates, sizeof(PARAM_RATES_EX));
            prAdapter->rWlanInfo.arScanResult[i].u4IELength = (u32)u2IELength;

            /* IE - allocate buffer and update pointer */
            if (u2IELength > 0) {
                if (ALIGN_4(u2IELength) +
                    prAdapter->rWlanInfo.u4ScanIEBufferUsage <=
                    u4BufferSize) {
                    kalMemCopy(&(prAdapter->rWlanInfo
                                 .aucScanIEBuf[prAdapter->rWlanInfo
                                               .u4ScanIEBufferUsage]),
                               pucIEBuf, u2IELength);

                    prAdapter->rWlanInfo.apucScanResultIEs[i] =
                        &(prAdapter->rWlanInfo.aucScanIEBuf
                          [prAdapter->rWlanInfo.u4ScanIEBufferUsage]);

                    prAdapter->rWlanInfo.u4ScanIEBufferUsage +=
                        ALIGN_4(u2IELength);
                } else {
                    /* buffer is not enough */
                    prAdapter->rWlanInfo.arScanResult[i].u4Length -= u2IELength;
                    prAdapter->rWlanInfo.arScanResult[i].u4IELength = 0;
                    prAdapter->rWlanInfo.apucScanResultIEs[i] = NULL;
                }
            } else {
                prAdapter->rWlanInfo.apucScanResultIEs[i] = NULL;
            }

            prAdapter->rWlanInfo.u4ScanResultNum++;
        } else if (rWeakestRssi != (PARAM_RSSI)INT_MAX) {
            /* replace weakest one */
            i = u4IdxWeakest;

            /* free IE buffer then zero */
            nicFreeScanResultIE(prAdapter, i);
            kalMemZero(&(prAdapter->rWlanInfo.arScanResult[i]),
                       OFFSET_OF(PARAM_BSSID_EX_T, aucIEs));

            /* then fill buffer */
            prAdapter->rWlanInfo.arScanResult[i].u4Length =
                OFFSET_OF(PARAM_BSSID_EX_T, aucIEs) + u2IELength;
            COPY_MAC_ADDR(prAdapter->rWlanInfo.arScanResult[i].arMacAddress,
                          rMacAddr);
            COPY_SSID(prAdapter->rWlanInfo.arScanResult[i].rSsid.aucSsid,
                      prAdapter->rWlanInfo.arScanResult[i].rSsid.u4SsidLen,
                      prSsid->aucSsid, prSsid->u4SsidLen);
            prAdapter->rWlanInfo.arScanResult[i].u4Privacy = u4Privacy;
            prAdapter->rWlanInfo.arScanResult[i].rRssi = rRssi;
            prAdapter->rWlanInfo.arScanResult[i].eNetworkTypeInUse =
                eNetworkType;
            kalMemCopy(&(prAdapter->rWlanInfo.arScanResult[i].rConfiguration),
                       prConfiguration, sizeof(PARAM_802_11_CONFIG_T));
            prAdapter->rWlanInfo.arScanResult[i].eOpMode = eOpMode;
            kalMemCopy((prAdapter->rWlanInfo.arScanResult[i].rSupportedRates),
                       rSupportedRates, sizeof(PARAM_RATES_EX));
            prAdapter->rWlanInfo.arScanResult[i].u4IELength = (u32)u2IELength;

            if (u2IELength > 0) {
                /* IE - allocate buffer and update pointer */
                if (ALIGN_4(u2IELength) +
                    prAdapter->rWlanInfo.u4ScanIEBufferUsage <=
                    u4BufferSize) {
                    kalMemCopy(&(prAdapter->rWlanInfo
                                 .aucScanIEBuf[prAdapter->rWlanInfo
                                               .u4ScanIEBufferUsage]),
                               pucIEBuf, u2IELength);

                    prAdapter->rWlanInfo.apucScanResultIEs[i] =
                        &(prAdapter->rWlanInfo.aucScanIEBuf
                          [prAdapter->rWlanInfo.u4ScanIEBufferUsage]);

                    prAdapter->rWlanInfo.u4ScanIEBufferUsage +=
                        ALIGN_4(u2IELength);
                } else {
                    /* buffer is not enough */
                    prAdapter->rWlanInfo.arScanResult[i].u4Length -= u2IELength;
                    prAdapter->rWlanInfo.arScanResult[i].u4IELength = 0;
                    prAdapter->rWlanInfo.apucScanResultIEs[i] = NULL;
                }
            } else {
                prAdapter->rWlanInfo.apucScanResultIEs[i] = NULL;
            }
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is invoked to free IE buffer for dedicated scan result
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param u4Idx              Index of Scan Result
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicFreeScanResultIE(IN P_ADAPTER_T prAdapter, IN u32 u4Idx)
{
    u32 i;
    u8 *pucPivot, *pucMovePivot;
    u32 u4MoveSize, u4FreeSize, u4ReserveSize;

    ASSERT(prAdapter);
    ASSERT(u4Idx < CFG_MAX_NUM_BSS_LIST);

    if (prAdapter->rWlanInfo.arScanResult[u4Idx].u4IELength == 0 ||
        prAdapter->rWlanInfo.apucScanResultIEs[u4Idx] == NULL) {
        return;
    }

    u4FreeSize = ALIGN_4(prAdapter->rWlanInfo.arScanResult[u4Idx].u4IELength);

    pucPivot = prAdapter->rWlanInfo.apucScanResultIEs[u4Idx];
    pucMovePivot =
        (u8 *)((unsigned long)(prAdapter->rWlanInfo.apucScanResultIEs[u4Idx]) +
               u4FreeSize);

    u4ReserveSize = ((unsigned long)pucPivot) -
                    (unsigned long)(&(prAdapter->rWlanInfo.aucScanIEBuf[0]));
    u4MoveSize =
        prAdapter->rWlanInfo.u4ScanIEBufferUsage - u4ReserveSize - u4FreeSize;

    /* 1. rest of buffer to move forward */
    kalMemCopy(pucPivot, pucMovePivot, u4MoveSize);

    /* 1.1 modify pointers */
    for (i = 0; i < prAdapter->rWlanInfo.u4ScanResultNum; i++) {
        if (i != u4Idx) {
            if (prAdapter->rWlanInfo.apucScanResultIEs[i] >= pucMovePivot) {
                prAdapter->rWlanInfo.apucScanResultIEs[i] =
                    (u8 *)((unsigned long)(prAdapter->rWlanInfo
                                           .apucScanResultIEs[i]) -
                           u4FreeSize);
            }
        }
    }

    /* 1.2 reset the freed one */
    prAdapter->rWlanInfo.arScanResult[u4Idx].u4IELength = 0;
    prAdapter->rWlanInfo.apucScanResultIEs[i] = NULL;

    /* 2. reduce IE buffer usage */
    prAdapter->rWlanInfo.u4ScanIEBufferUsage -= u4FreeSize;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to hack parameters for WLAN TABLE for
 *        fixed rate settings
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param eRateSetting
 * @param pu2DesiredNonHTRateSet,
 * @param pu2BSSBasicRateSet,
 * @param pucMcsSet
 * @param pucSupMcs32
 * @param pu2HtCapInfo
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
nicUpdateRateParams(IN P_ADAPTER_T prAdapter,
                    IN ENUM_REGISTRY_FIXED_RATE_T eRateSetting,
                    IN u8 *pucDesiredPhyTypeSet, IN u16 *pu2DesiredNonHTRateSet,
                    IN u16 *pu2BSSBasicRateSet, IN u8 *pucMcsSet,
                    IN u8 *pucSupMcs32, IN u16 *pu2HtCapInfo)
{
    ASSERT(prAdapter);
    ASSERT(eRateSetting > FIXED_RATE_NONE && eRateSetting < FIXED_RATE_NUM);

    switch (prAdapter->rWifiVar.eRateSetting) {
    case FIXED_RATE_1M:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HR_DSSS;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_1M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_1M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_2M:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HR_DSSS;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_2M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_2M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_5_5M:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HR_DSSS;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_5_5M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_5_5M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_11M:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HR_DSSS;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_11M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_11M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_6M:
        if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
        } else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;
        }

        *pu2DesiredNonHTRateSet = RATE_SET_BIT_6M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_6M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_9M:
        if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
        } else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;
        }

        *pu2DesiredNonHTRateSet = RATE_SET_BIT_9M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_9M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_12M:
        if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
        } else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;
        }

        *pu2DesiredNonHTRateSet = RATE_SET_BIT_12M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_12M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_18M:
        if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
        } else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;
        }

        *pu2DesiredNonHTRateSet = RATE_SET_BIT_18M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_18M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_24M:
        if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
        } else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;
        }

        *pu2DesiredNonHTRateSet = RATE_SET_BIT_24M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_24M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_36M:
        if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
        } else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;
        }

        *pu2DesiredNonHTRateSet = RATE_SET_BIT_36M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_36M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_48M:
        if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
        } else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;
        }

        *pu2DesiredNonHTRateSet = RATE_SET_BIT_48M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_48M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_54M:
        if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
        } else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM) {
            *pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;
        }

        *pu2DesiredNonHTRateSet = RATE_SET_BIT_54M;
        *pu2BSSBasicRateSet = RATE_SET_BIT_54M;
        *pucMcsSet = 0;
        *pucSupMcs32 = 0;
        *pu2HtCapInfo = 0;
        break;

    case FIXED_RATE_MCS0_20M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS0_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &=
            ~(HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_20M |
              HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        break;

    case FIXED_RATE_MCS1_20M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS1_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &=
            ~(HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_20M |
              HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        break;

    case FIXED_RATE_MCS2_20M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS2_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &=
            ~(HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_20M |
              HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        break;

    case FIXED_RATE_MCS3_20M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS3_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &=
            ~(HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_20M |
              HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        break;

    case FIXED_RATE_MCS4_20M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS4_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &=
            ~(HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_20M |
              HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        break;

    case FIXED_RATE_MCS5_20M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS5_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &=
            ~(HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_20M |
              HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        break;

    case FIXED_RATE_MCS6_20M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS6_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &=
            ~(HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_20M |
              HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        break;

    case FIXED_RATE_MCS7_20M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS7_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &=
            ~(HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_20M |
              HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        break;

    case FIXED_RATE_MCS0_20M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS0_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
        break;

    case FIXED_RATE_MCS1_20M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS1_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
        break;

    case FIXED_RATE_MCS2_20M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS2_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
        break;

    case FIXED_RATE_MCS3_20M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS3_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
        break;

    case FIXED_RATE_MCS4_20M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS4_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
        break;

    case FIXED_RATE_MCS5_20M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS5_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
        break;

    case FIXED_RATE_MCS6_20M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS6_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
        break;

    case FIXED_RATE_MCS7_20M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS7_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
        break;

    case FIXED_RATE_MCS0_40M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS0_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
        break;

    case FIXED_RATE_MCS1_40M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS1_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
        break;

    case FIXED_RATE_MCS2_40M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS2_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
        break;

    case FIXED_RATE_MCS3_40M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS3_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
        break;

    case FIXED_RATE_MCS4_40M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS4_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
        break;

    case FIXED_RATE_MCS5_40M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS5_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
        break;

    case FIXED_RATE_MCS6_40M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS6_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
        break;

    case FIXED_RATE_MCS7_40M_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS7_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
        break;

    case FIXED_RATE_MCS32_800NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = 0;
        *pucSupMcs32 = 1;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
                             HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
        break;

    case FIXED_RATE_MCS0_40M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS0_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |=
            (HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_40M);
        break;

    case FIXED_RATE_MCS1_40M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS1_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |=
            (HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_40M);
        break;

    case FIXED_RATE_MCS2_40M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS2_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |=
            (HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_40M);
        break;

    case FIXED_RATE_MCS3_40M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS3_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |=
            (HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_40M);
        break;

    case FIXED_RATE_MCS4_40M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS4_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |=
            (HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_40M);
        break;

    case FIXED_RATE_MCS5_40M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS5_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |=
            (HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_40M);
        break;

    case FIXED_RATE_MCS6_40M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS6_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |=
            (HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_40M);
        break;

    case FIXED_RATE_MCS7_40M_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = BIT(HT_RATE_MCS7_INDEX - 1);
        *pucSupMcs32 = 0;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |=
            (HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_40M);
        break;

    case FIXED_RATE_MCS32_400NS:
        *pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
        *pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
        *pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
        *pucMcsSet = 0;
        *pucSupMcs32 = 1;
        (*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_HT_GF);
        (*pu2HtCapInfo) |=
            (HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_SHORT_GI_40M);
        break;

    default:
        ASSERT(0);
    }

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to write the register
 *
 * @param u4Address         Register address
 *        u4Value           the value to be written
 *
 * @retval WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/

WLAN_STATUS nicWriteMcr(IN P_ADAPTER_T prAdapter, IN u32 u4Address,
                        IN u32 u4Value)
{
    CMD_ACCESS_REG rCmdAccessReg;

    rCmdAccessReg.u4Address = u4Address;
    rCmdAccessReg.u4Data = u4Value;

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_ACCESS_REG, true, false, false,
                               NULL, NULL, sizeof(CMD_ACCESS_REG),
                               (u8 *)&rCmdAccessReg, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to modify the auto rate parameters
 *
 * @param u4ArSysParam0  see description below
 *        u4ArSysParam1
 *        u4ArSysParam2
 *        u4ArSysParam3
 *
 *
 * @retval WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 *
 * @note
 *   ArSysParam0[0:3] -> auto rate version (0:disable 1:version1 2:version2)
 *   ArSysParam0[4:5]-> auto bw version (0:disable 1:version1 2:version2)
 *   ArSysParam0[6:7]-> auto gi version (0:disable 1:version1 2:version2)
 *   ArSysParam0[8:15]-> HT rate clear mask
 *   ArSysParam0[16:31]-> Legacy rate clear mask
 *   ArSysParam1[0:7]-> Auto Rate check weighting window
 *   ArSysParam1[8:15]-> Auto Rate v1 Force Rate down
 *   ArSysParam1[16:23]-> Auto Rate v1 PerH
 *   ArSysParam1[24:31]-> Auto Rate v1 PerL
 *
 *   Examples
 *   ArSysParam0 = 1,
 *   Enable auto rate version 1
 *
 *   ArSysParam0 = 983041,
 *   Enable auto rate version 1
 *   Remove CCK 1M, 2M, 5.5M, 11M
 *
 *   ArSysParam0 = 786433
 *   Enable auto rate version 1
 *   Remove CCK 5.5M 11M
 */
/*----------------------------------------------------------------------------*/

WLAN_STATUS
nicRlmArUpdateParms(IN P_ADAPTER_T prAdapter, IN u32 u4ArSysParam0,
                    IN u32 u4ArSysParam1, IN u32 u4ArSysParam2,
                    IN u32 u4ArSysParam3)
{
    u8 ucArVer, ucAbwVer, ucAgiVer;
    u16 u2HtClrMask;
    u16 u2LegacyClrMask;
    u8 ucArCheckWindow;
    u8 ucArPerL;
    u8 ucArPerH;
    u8 ucArPerForceRateDownPer;

    ucArVer = (u8)(u4ArSysParam0 & BITS(0, 3));
    ucAbwVer = (u8)((u4ArSysParam0 & BITS(4, 5)) >> 4);
    ucAgiVer = (u8)((u4ArSysParam0 & BITS(6, 7)) >> 6);
    u2HtClrMask = (u16)((u4ArSysParam0 & BITS(8, 15)) >> 8);
    u2LegacyClrMask = (u16)((u4ArSysParam0 & BITS(16, 31)) >> 16);

    ucArCheckWindow = (u8)(u4ArSysParam1 & BITS(0, 7));
    ucArPerForceRateDownPer = (u8)(((u4ArSysParam1 >> 8) & BITS(0, 7)));
    ucArPerH = (u8)(((u4ArSysParam1 >> 16) & BITS(0, 7)));
    ucArPerL = (u8)(((u4ArSysParam1 >> 24) & BITS(0, 7)));

    DBGLOG(INIT, INFO, "ArParam %ld %ld %ld %ld\n", u4ArSysParam0,
           u4ArSysParam1, u4ArSysParam2, u4ArSysParam3);
    DBGLOG(INIT, INFO, "ArVer %u AbwVer %u AgiVer %u\n", ucArVer, ucAbwVer,
           ucAgiVer);
    DBGLOG(INIT, INFO, "HtMask %x LegacyMask %x\n", u2HtClrMask,
           u2LegacyClrMask);
    DBGLOG(INIT, INFO, "CheckWin %u RateDownPer %u PerH %u PerL %u\n",
           ucArCheckWindow, ucArPerForceRateDownPer, ucArPerH, ucArPerL);

#define SWCR_DATA_ADDR(MOD, ADDR) (0x90000000 + (MOD << 8) + (ADDR))
#define SWCR_DATA_CMD(CATE, WRITE, INDEX, OPT0, OPT1) \
    ((CATE << 24) | (WRITE << 23) | (INDEX << 16) | (OPT0 << 8) | OPT1)
#define SWCR_DATA0 0x0
#define SWCR_DATA1 0x4
#define SWCR_DATA2 0x8
#define SWCR_DATA3 0xC
#define SWCR_DATA4 0x10
#define SWCR_WRITE 1
#define SWCR_READ 0

    if (ucArVer > 0) {
        /* dummy = WiFi.WriteMCR(&h90000104, &h00000001) */
        /* dummy = WiFi.WriteMCR(&h90000100, &h00850000) */

        nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1  /*MOD*/, SWCR_DATA1), 1);
        nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1  /*MOD*/, SWCR_DATA0),
                    SWCR_DATA_CMD(0, SWCR_WRITE, 5, 0, 0));
    } else {
        nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1  /*MOD*/, SWCR_DATA1), 0);
        nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1  /*MOD*/, SWCR_DATA0),
                    SWCR_DATA_CMD(0, SWCR_WRITE, 5, 0, 0));
    }

    /* ucArVer 0: none 1:PER 2:Rcpi */
    nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1  /*MOD*/, SWCR_DATA1), ucArVer);
    nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1  /*MOD*/, SWCR_DATA0),
                SWCR_DATA_CMD(0, SWCR_WRITE, 7, 0, 0));

    /* Candidate rate Ht mask */
    nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1  /*MOD*/, SWCR_DATA1), u2HtClrMask);
    nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1  /*MOD*/, SWCR_DATA0),
                SWCR_DATA_CMD(0, SWCR_WRITE, 0x1c, 0, 0));

    /* Candidate rate legacy mask */
    nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1  /*MOD*/, SWCR_DATA1),
                u2LegacyClrMask);
    nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1  /*MOD*/, SWCR_DATA0),
                SWCR_DATA_CMD(0, SWCR_WRITE, 0x1d, 0, 0));

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to enable roaming
 *
 * @param u4EnableRoaming
 *
 *
 * @retval WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 *
 * @note
 *   u4EnableRoaming -> Enable Romaing
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicRoamingUpdateParams(IN P_ADAPTER_T prAdapter,
                                   IN u32 u4EnableRoaming)
{
    P_CONNECTION_SETTINGS_T prConnSettings;

    prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
    prConnSettings->fgIsEnableRoaming =
        ((u4EnableRoaming > 0) ? (true) : (false));

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to update Link Quality information
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *        ucBssIndex
 *        prEventLinkQuality
 *        cRssi
 *        cLinkQuality
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void nicUpdateLinkQuality(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                          IN P_EVENT_LINK_QUALITY_V2 prEventLinkQuality)
{
    s8 cRssi;
    u16 u2AdjustRssi = 10;

    ASSERT(prAdapter);
    ASSERT(ucBssIndex <= MAX_BSS_INDEX);
    ASSERT(prEventLinkQuality);

    switch (GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType) {
    case NETWORK_TYPE_AIS:
        if (GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eConnectionState ==
            PARAM_MEDIA_STATE_CONNECTED) {
            /* check is to prevent RSSI to be updated by incorrect
             * initial RSSI from hardware */
            /* buffer statistics for further query */
            if (prAdapter->fgIsLinkQualityValid == false ||
                (kalGetTimeTick() - prAdapter->rLinkQualityUpdateTime) >
                CFG_LINK_QUALITY_VALID_PERIOD) {
                /* ranged from (-128 ~ 30) in unit of dBm */
                cRssi = prEventLinkQuality->rLq[ucBssIndex].cRssi;
                cRssi = (s8)(((s16)(cRssi) * u2AdjustRssi) / 10);
                DBGLOG(RLM, INFO, "Rssi=%d, NewRssi=%d\n",
                       prEventLinkQuality->rLq[ucBssIndex].cRssi, cRssi);
                nicUpdateRSSI(prAdapter, ucBssIndex, cRssi,
                              prEventLinkQuality->rLq[ucBssIndex].cLinkQuality);
            }

            if (prAdapter->fgIsLinkRateValid == false ||
                (kalGetTimeTick() - prAdapter->rLinkRateUpdateTime) >
                CFG_LINK_QUALITY_VALID_PERIOD) {
                nicUpdateLinkSpeed(
                    prAdapter, ucBssIndex,
                    prEventLinkQuality->rLq[ucBssIndex].u2LinkSpeed);
            }
        }
        break;

    default:
        break;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to update RSSI and Link Quality information
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *        ucBssIndex
 *        cRssi
 *        cLinkQuality
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void nicUpdateRSSI(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex, IN s8 cRssi,
                   IN s8 cLinkQuality)
{
    ASSERT(prAdapter);
    ASSERT(ucBssIndex <= MAX_BSS_INDEX);

    switch (GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType) {
    case NETWORK_TYPE_AIS:
        if (GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eConnectionState ==
            PARAM_MEDIA_STATE_CONNECTED) {
            prAdapter->fgIsLinkQualityValid = true;
            prAdapter->rLinkQualityUpdateTime = kalGetTimeTick();

            prAdapter->rLinkQuality.cRssi = cRssi;
            prAdapter->rLinkQuality.cLinkQuality = cLinkQuality;
            /* indicate to glue layer */
            kalUpdateRSSI(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_AIS_INDEX,
                          prAdapter->rLinkQuality.cRssi,
                          prAdapter->rLinkQuality.cLinkQuality);
        }

        break;

#if CFG_ENABLE_WIFI_DIRECT && CFG_SUPPORT_P2P_RSSI_QUERY
    case NETWORK_TYPE_P2P:
        prAdapter->fgIsP2pLinkQualityValid = true;
        prAdapter->rP2pLinkQualityUpdateTime = kalGetTimeTick();

        prAdapter->rP2pLinkQuality.cRssi = cRssi;
        prAdapter->rP2pLinkQuality.cLinkQuality = cLinkQuality;

        kalUpdateRSSI(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_P2P_INDEX, cRssi,
                      cLinkQuality);
        break;

#endif
    default:
        break;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to update Link Quality information
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *        ucBssIndex
 *        prEventLinkQuality
 *        cRssi
 *        cLinkQuality
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void nicUpdateLinkSpeed(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                        IN u16 u2LinkSpeed)
{
    ASSERT(prAdapter);
    ASSERT(ucBssIndex <= MAX_BSS_INDEX);

    switch (GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType) {
    case NETWORK_TYPE_AIS:
        if (GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eConnectionState ==
            PARAM_MEDIA_STATE_CONNECTED) {
            /* buffer statistics for further query */
            prAdapter->fgIsLinkRateValid = true;
            prAdapter->rLinkRateUpdateTime = kalGetTimeTick();

            prAdapter->rLinkQuality.u2LinkSpeed = u2LinkSpeed;
        }
        break;

    default:
        break;
    }
}

#if CFG_SUPPORT_RDD_TEST_MODE
WLAN_STATUS nicUpdateRddTestMode(IN P_ADAPTER_T prAdapter,
                                 IN P_CMD_RDD_CH_T prRddChParam)
{
    DEBUGFUNC("nicUpdateRddTestMode.\n");

    ASSERT(prAdapter);

    /* aisFsmScanRequest(prAdapter, NULL); */

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_RDD_CH, true, false, false,
                               NULL, NULL, sizeof(CMD_RDD_CH_T),
                               (u8 *)prRddChParam, NULL, 0);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to apply network address setting to
 *        both OS side and firmware domain
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/

WLAN_STATUS nicApplyNetworkAddress(IN P_ADAPTER_T prAdapter)
{
    u32 i;

    ASSERT(prAdapter);

    /* copy to adapter */
    COPY_MAC_ADDR(prAdapter->rMyMacAddr, prAdapter->rWifiVar.aucMacAddress);

    /* 4 <3> Update new MAC address to all 3 networks */
    COPY_MAC_ADDR(prAdapter->rWifiVar.aucDeviceAddress, prAdapter->rMyMacAddr);
    prAdapter->rWifiVar.aucDeviceAddress[0] ^= MAC_ADDR_LOCAL_ADMIN;

    COPY_MAC_ADDR(prAdapter->rWifiVar.aucInterfaceAddress,
                  prAdapter->rMyMacAddr);
    prAdapter->rWifiVar.aucInterfaceAddress[0] ^= MAC_ADDR_LOCAL_ADMIN;

#if CFG_ENABLE_WIFI_DIRECT
    if (prAdapter->fgIsP2PRegistered) {
        for (i = 0; i < BSS_INFO_NUM; i++) {
            if (prAdapter->rWifiVar.arBssInfoPool[i].eNetworkType ==
                NETWORK_TYPE_P2P) {
                COPY_MAC_ADDR(
                    prAdapter->rWifiVar.arBssInfoPool[i].aucOwnMacAddr,
                    prAdapter->rWifiVar.aucDeviceAddress);
            }
        }
    }
#endif

#if CFG_TEST_WIFI_DIRECT_GO
    if (prAdapter->rWifiVar.prP2pFsmInfo->eCurrentState == P2P_STATE_IDLE) {
        wlanEnableP2pFunction(prAdapter);

        wlanEnableATGO(prAdapter);
    }
#endif

    kalUpdateMACAddress(prAdapter->prGlueInfo,
                        prAdapter->rWifiVar.aucMacAddress);

    return WLAN_STATUS_SUCCESS;
}

u8 nicGetChipHwVer(void)
{
    return g_eco_info.ucHwVer;
}

u8 nicGetChipSwVer(void)
{
    return g_eco_info.ucRomVer;
}

u8 nicGetChipFactoryVer(void)
{
    return g_eco_info.ucFactoryVer;
}

u8 nicSetChipHwVer(u8 value)
{
    g_eco_info.ucHwVer = value;
    return 0;
}

u8 nicSetChipSwVer(u8 value)
{
    g_eco_info.ucRomVer = value;
    return 0;
}

u8 nicSetChipFactoryVer(u8 value)
{
    g_eco_info.ucFactoryVer = value;
    return 0;
}

u8 nicGetChipEcoVer(IN P_ADAPTER_T prAdapter)
{
    P_ECO_INFO_T prEcoInfo;
    u8 ucEcoVer;
    u8 ucCurSwVer, ucCurHwVer, ucCurFactoryVer;

    ucCurSwVer = nicGetChipSwVer();
    ucCurHwVer = nicGetChipHwVer();
    ucCurFactoryVer = nicGetChipFactoryVer();

    ucEcoVer = 0;

    while (true) {
        /* Get ECO info from table */
        prEcoInfo = (P_ECO_INFO_T)&(prAdapter->chip_info->eco_info[ucEcoVer]);

        if ((prEcoInfo->ucRomVer == 0) && (prEcoInfo->ucHwVer == 0) &&
            (prEcoInfo->ucFactoryVer == 0)) {
            /* End of table */
            break;
        }

        if ((prEcoInfo->ucRomVer == ucCurSwVer) &&
            (prEcoInfo->ucHwVer == ucCurHwVer) &&
            (prEcoInfo->ucFactoryVer == ucCurFactoryVer)) {
            break;
        }

        ucEcoVer++;
    }
    return prAdapter->chip_info->eco_info[ucEcoVer].ucEcoVer;
}

u8 nicIsEcoVerEqualTo(IN P_ADAPTER_T prAdapter, u8 ucEcoVer)
{
    if (ucEcoVer == prAdapter->chip_info->eco_ver) {
        return true;
    } else {
        return false;
    }
}

u8 nicIsEcoVerEqualOrLaterTo(IN P_ADAPTER_T prAdapter, u8 ucEcoVer)
{
    if (ucEcoVer <= prAdapter->chip_info->eco_ver) {
        return true;
    } else {
        return false;
    }
}

void nicSerStopTxRx(IN P_ADAPTER_T prAdapter)
{
    DBGLOG(NIC, WARN, "SER: Stop HIF Tx/Rx!\n");

    prAdapter->ucSerState = SER_STOP_HOST_TX_RX;

    /* Force own to FW as ACK and stop HIF */
    prAdapter->fgWiFiInSleepyState = true;
}

void nicSerStopTx(IN P_ADAPTER_T prAdapter)
{
    DBGLOG(NIC, WARN, "SER: Stop HIF Tx!\n");

    prAdapter->ucSerState = SER_STOP_HOST_TX;
}

void nicSerStartTxRx(IN P_ADAPTER_T prAdapter)
{
    DBGLOG(NIC, WARN, "SER: Start HIF T/R!\n");

    halSerHifReset(prAdapter);
    prAdapter->ucSerState = SER_IDLE_DONE;
}

u8 nicSerIsWaitingReset(IN P_ADAPTER_T prAdapter)
{
    if (prAdapter->ucSerState == SER_STOP_HOST_TX_RX) {
        return true;
    } else {
        return false;
    }
}

u8 nicSerIsTxStop(IN P_ADAPTER_T prAdapter)
{
    switch (prAdapter->ucSerState) {
    case SER_STOP_HOST_TX:
    case SER_STOP_HOST_TX_RX:
    case SER_REINIT_HIF:
        return true;

    case SER_IDLE_DONE:
    default:
        return false;
    }
}

u8 nicSerIsRxStop(IN P_ADAPTER_T prAdapter)
{
    switch (prAdapter->ucSerState) {
    case SER_STOP_HOST_TX_RX:
    case SER_REINIT_HIF:
        return true;

    case SER_STOP_HOST_TX:
    case SER_IDLE_DONE:
    default:
        return false;
    }
}

void nicDumpMsduInfo(IN P_MSDU_INFO_T prMsduInfo)
{
    struct sk_buff *prSkb;

    if (!prMsduInfo) {
        DBGLOG(NIC, ERROR, "Invalid MsduInfo, skip dump.");
        return;
    }

    /* [1]prPacket(txd)            [2]eSrc
     * [3]ucUserPriority           [4]ucTC
     * [5]ucPacketType             [6]ucStaRecIndex
     * [7]ucBssIndex               [8]ucWlanIndex
     * [9]ucPacketFormat           [10]fgIs802_1x
     * [11]fgIs802_1x_NonProtected [12]fgIs802_11
     * [13]fgIs802_3               [14]fgIsVlanExists
     * [15]u4Option                [16]cPowerOffset
     * [17]u2SwSN                  [18]ucRetryLimit
     * [19]u4RemainingLifetime     [20]ucControlFlag
     * [21]ucRateMode              [22]u4FixedRateOption
     * [23]fgIsTXDTemplateValid    [24]ucMacHeaderLength
     * [25]ucLlcLength             [26]u2FrameLength
     * [27]aucEthDestAddr          [28]u4PageCount
     * [29]ucTxSeqNum              [30]ucPID
     * [31]ucWmmQueSet             [32]pfTxDoneHandler
     * [33]u4TxDoneTag             [34]ucPktType
     */

#define TEMP_LINE1                                  \
    "[1][%p], [2][%u], [3][%u], [4][%u], [5][%u], " \
    "[6][%u], [7][%u], [8][%u], [9][%u], [10][%u]\n"

#define TEMP_LINE2                                       \
    "[11][%u], [12][%u], [13][%u], [14][%u], [15][%u], " \
    "[16][%d], [17][%u], [18][%u], [19][%u], [20][%u]\n"

#define TEMP_LINE3                                       \
    "[21][%u], [22][%u], [23][%u], [24][%u], [25][%u], " \
    "[26][%u], [27][" MACSTR "], [28][%u], [29][%u], [30][%u]\n"

#define TEMP_LINE4 "[31][%u], [32][%p], [33][%u], [34][%u]\n"

    DBGLOG(NIC, INFO, TEMP_LINE1, prMsduInfo->prPacket, prMsduInfo->eSrc,
           prMsduInfo->ucUserPriority, prMsduInfo->ucTC,
           prMsduInfo->ucPacketType, prMsduInfo->ucStaRecIndex,
           prMsduInfo->ucBssIndex, prMsduInfo->ucWlanIndex,
           prMsduInfo->ucPacketFormat, prMsduInfo->fgIs802_1x);

    DBGLOG(NIC, INFO, TEMP_LINE2, prMsduInfo->fgIs802_1x_NonProtected,
           prMsduInfo->fgIs802_11, prMsduInfo->fgIs802_3,
           prMsduInfo->fgIsVlanExists, prMsduInfo->u4Option,
           prMsduInfo->cPowerOffset, prMsduInfo->u2SwSN,
           prMsduInfo->ucRetryLimit, prMsduInfo->u4RemainingLifetime,
           prMsduInfo->ucControlFlag);

    DBGLOG(NIC, INFO, TEMP_LINE3, prMsduInfo->ucRateMode,
           prMsduInfo->u4FixedRateOption, prMsduInfo->fgIsTXDTemplateValid,
           prMsduInfo->ucMacHeaderLength, prMsduInfo->ucLlcLength,
           prMsduInfo->u2FrameLength, prMsduInfo->aucEthDestAddr,
           prMsduInfo->u4PageCount, prMsduInfo->ucTxSeqNum, prMsduInfo->ucPID);

    DBGLOG(NIC, INFO, TEMP_LINE4, prMsduInfo->ucWmmQueSet,
           prMsduInfo->pfTxDoneHandler, prMsduInfo->u4TxDoneTag,
           prMsduInfo->ucPacketType);
#undef TEMP_LINE1
#undef TEMP_LINE2
#undef TEMP_LINE3
#undef TEMP_LINE4

    /* dump txd */
    if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_DATA &&
        prMsduInfo->prPacket) {
        prSkb = prMsduInfo->prPacket;
        DBGLOG_MEM8(NIC, INFO, prSkb->data, 64);
    }
}
