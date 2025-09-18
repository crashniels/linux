// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */
/******************************************************************************
*[File]             hif_api.c
*[Version]          v1.0
*[Revision Date]    2015-09-08
*[Author]
*[Description]
*    The program provides SDIO HIF APIs
*[Copyright]
*    Copyright (C) 2015 MediaTek Incorporation. All Rights Reserved.
******************************************************************************/

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "precomp.h"

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>  /* sdio_readl(), etc */
#include <linux/mmc/sdio_ids.h>

#include <linux/mm.h>
#ifndef CONFIG_X86
#include <asm/memory.h>
#endif

#include "reg.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define RX_RESPONSE_TIMEOUT    (15000)

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
 * @brief Verify the CHIP ID
 *
 * @param prAdapter      a pointer to adapter private data structure.
 *
 *
 * @retval true          CHIP ID is the same as the setting compiled
 * @retval false         CHIP ID is different from the setting compiled
 */
/*----------------------------------------------------------------------------*/
u8 halVerifyChipID(IN P_ADAPTER_T prAdapter)
{
    u32 u4CIR = 0;
    struct chip_info *prChipInfo;

    ASSERT(prAdapter);

    if (prAdapter->fgIsReadRevID) {
        return true;
    }

    HAL_MCR_RD(prAdapter, MCR_WCIR, &u4CIR);

    DBGLOG(INIT, TRACE, "Chip ID: 0x%lx\n", u4CIR & WCIR_CHIP_ID);
    DBGLOG(INIT, TRACE, "Revision ID: 0x%lx\n",
           ((u4CIR & WCIR_REVISION_ID) >> 16));

    prChipInfo = prAdapter->chip_info;

    if ((u4CIR & WCIR_CHIP_ID) != prChipInfo->chip_id) {
        return false;
    }

    prAdapter->ucRevID = (u8)(((u4CIR & WCIR_REVISION_ID) >> 16) & 0xF);
    prAdapter->fgIsReadRevID = true;

    return true;
}

WLAN_STATUS
halRxWaitResponse(IN P_ADAPTER_T prAdapter, IN u8 ucPortIdx,
                  OUT u8 *pucRspBuffer, IN u32 u4MaxRespBufferLen,
                  OUT u32 *pu4Length)
{
    u32 u4Value = 0, u4PktLen = 0, i = 0, u4CpyLen;
    WLAN_STATUS u4Status = WLAN_STATUS_SUCCESS;
    u32 u4Time, u4Current;
    P_RX_CTRL_T prRxCtrl;

    DEBUGFUNC("halRxWaitResponse");

    ASSERT(prAdapter);
    ASSERT(pucRspBuffer);

    prRxCtrl = &prAdapter->rRxCtrl;

    u4Time = (u32)kalGetTimeTick();

    do {
        HAL_MCR_RD(prAdapter, MCR_WHISR, &u4Value);
        if (!(u4Value & (WHISR_RX0_DONE_INT | WHISR_RX1_DONE_INT))) {
            /* timeout exceeding check */
            u4Current = (u32)kalGetTimeTick();

            if ((u4Current > u4Time) &&
                ((u4Current - u4Time) > RX_RESPONSE_TIMEOUT)) {
                DBGLOG(RX, ERROR, "Timeout! %d - %d = %d\n",
                       u4Current, u4Time, (u4Current - u4Time));
                return WLAN_STATUS_FAILURE;
            } else if (u4Current < u4Time &&
                       ((u4Current + (0xFFFFFFFF - u4Time)) >
                        RX_RESPONSE_TIMEOUT)) {
                DBGLOG(RX, ERROR, "Timeout! %d - %d = %d\n",
                       u4Current, u4Time,
                       (u4Current + (0xFFFFFFFF - u4Time)));
                return WLAN_STATUS_FAILURE;
            }
            /* Response packet is not ready */
            kalUdelay(50);

            continue;
        }

        /* Read the packet length */
        HAL_MCR_RD(prAdapter, MCR_WRPLR, &u4Value);

        if ((u4Value & 0xFFFF) != 0) {
            u4PktLen = u4Value & 0xFFFF;
            i = 0;
        } else {
            u4PktLen = (u4Value >> 16) & 0xFFFF;
            i = 1;
        }

        if (u4PktLen == 0) {
            DBGLOG(RX, ERROR, "Packet length is 0!!\n");
            return WLAN_STATUS_FAILURE;
        } else {
#if (CFG_ENABLE_READ_EXTRA_4_BYTES == 1)
#if CFG_SDIO_RX_AGG
            /* decide copy length */
            if (u4PktLen > u4MaxRespBufferLen) {
                u4CpyLen = u4MaxRespBufferLen;
                DBGLOG(RX, ERROR,
                       "Packet length is larger than buffer size!!\n");
            } else {
                u4CpyLen = u4PktLen;
            }

            /* read from SDIO to tmp. buffer */
            HAL_PORT_RD(prAdapter, i == 0 ? MCR_WRDR0 : MCR_WRDR1,
                        ALIGN_4(u4PktLen + 4),
                        prRxCtrl->pucRxCoalescingBufPtr,
                        HIF_RX_COALESCING_BUFFER_SIZE);

            /* copy to destination buffer */
            kalMemCopy(pucRspBuffer,
                       prRxCtrl->pucRxCoalescingBufPtr, u4CpyLen);

            /* update valid buffer count */
            u4PktLen = u4CpyLen;
#else
#error "Please turn on RX coalescing"
#endif
#else
            HAL_PORT_RD(prAdapter, i == 0 ? MCR_WRDR0 : MCR_WRDR1,
                        u4PktLen, pucRspBuffer, u4MaxRespBufferLen);
#endif
            *pu4Length = u4PktLen;
            break;
        }
    } while (true);

    return u4Status;
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
void halEnableInterrupt(IN P_ADAPTER_T prAdapter)
{
    u8 fgIsIntEnableCache, fgIsPendingInt;

    ASSERT(prAdapter);
    fgIsIntEnableCache = prAdapter->fgIsIntEnable;
    /* Not to enable interrupt if there is pending interrupt */
    fgIsPendingInt = prAdapter->prGlueInfo->rHifInfo.fgIsPendingInt;

    if (!fgIsPendingInt) {
        prAdapter->fgIsIntEnable = true;  /* NOTE(Kevin): It must be
                                           * placed before MCR GINT
                                           * write. */
    }
    /* If need enable INT and also set LPOwn at the same time. */
    if (prAdapter->fgIsIntEnableWithLPOwnSet) {
        prAdapter->fgIsIntEnableWithLPOwnSet = false;  /* NOTE(Kevin):
                                                        * It's better to
                                                        * place it before
                                                        * MCR GINT write.
                                                        */
        /* If INT was enabled, only set LPOwn */
        if (fgIsIntEnableCache) {
            HAL_MCR_WR(prAdapter, MCR_WHLPCR,
                       WHLPCR_FW_OWN_REQ_SET);
            prAdapter->fgIsFwOwn = true;
        }
        /* If INT was not enabled, enable it and also set LPOwn now */
        else if (!fgIsPendingInt) {
            HAL_MCR_WR(prAdapter, MCR_WHLPCR,
                       WHLPCR_FW_OWN_REQ_SET | WHLPCR_INT_EN_SET);
            prAdapter->fgIsFwOwn = true;
        }
    }
    /* If INT was not enabled, enable it now */
    else if (!fgIsIntEnableCache && !fgIsPendingInt) {
        HAL_BYTE_WR(prAdapter, MCR_WHLPCR, WHLPCR_INT_EN_SET);
    }

    if (fgIsPendingInt) {
        kalSetIntEvent(prAdapter->prGlueInfo);
    }
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
void halDisableInterrupt(IN P_ADAPTER_T prAdapter)
{
    ASSERT(prAdapter);

    HAL_BYTE_WR(prAdapter, MCR_WHLPCR, WHLPCR_INT_EN_CLR);

    prAdapter->fgIsIntEnable = false;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to process the POWER OFF procedure.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
u8 halSetDriverOwn(IN P_ADAPTER_T prAdapter)
{
    u8 fgStatus = true;
    u32 i, u4CurrTick = 0;
    u8 fgTimeout;
    u8 fgResult;
    u8 fgReady = false;

    ASSERT(prAdapter);

    GLUE_INC_REF_CNT(prAdapter->u4PwrCtrlBlockCnt);

    if (prAdapter->fgIsFwOwn == false) {
        return fgStatus;
    }

    DBGLOG(INIT, INFO, "DRIVER OWN\n");

    u4CurrTick = kalGetTimeTick();
    i = 0;

    glWakeupSdio(prAdapter->prGlueInfo);

    while (1) {
        HAL_LP_OWN_RD(prAdapter, &fgResult);

        if (TIME_BEFORE(kalGetTimeTick(), u4CurrTick)) {  /* To prevent
                                                           * timer
                                                           * wraparound
                                                           */
            fgTimeout = ((kalGetTimeTick() + (~u4CurrTick)) >
                         LP_OWN_BACK_TOTAL_DELAY_MS) ?
                        true :
                        false;
        } else {
            fgTimeout = ((kalGetTimeTick() - u4CurrTick) >
                         LP_OWN_BACK_TOTAL_DELAY_MS) ?
                        true :
                        false;
        }

        if (fgResult) {
            prAdapter->fgIsFwOwn = false;
            prAdapter->u4OwnFailedCount = 0;
            prAdapter->u4OwnFailedLogCount = 0;

            if (nicSerIsWaitingReset(prAdapter)) {
                /* SER is done, start Tx/Rx */
                nicSerStartTxRx(prAdapter);
            }
            break;
        } else if ((i > LP_OWN_BACK_FAILED_RETRY_CNT) &&
                   (kalIsCardRemoved(prAdapter->prGlueInfo) ||
                    fgIsBusAccessFailed || fgTimeout ||
                    wlanIsChipNoAck(prAdapter))) {
#if CFG_SUPPORT_LOW_POWER_DEBUG
            /* For driver own back fail debug,  get current PC value
             */
            halPrintMailbox(prAdapter);
            halPollDbgCr(prAdapter,
                         LP_OWN_BACK_FAILED_DBGCR_POLL_ROUND);
#endif
            if ((prAdapter->u4OwnFailedCount == 0) ||
                CHECK_FOR_TIMEOUT(
                    u4CurrTick,
                    prAdapter->rLastOwnFailedLogTime,
                    MSEC_TO_SYSTIME(
                        LP_OWN_BACK_FAILED_LOG_SKIP_MS))) {
                DBGLOG(INIT,
                       ERROR,
                       "LP cannot be own back, Timeout[%u](%ums), BusAccessError[%u]",
                       fgTimeout,
                       kalGetTimeTick() - u4CurrTick,
                       fgIsBusAccessFailed);

                DBGLOG(INIT,
                       INFO,
                       "Skip LP own back failed log for next %ums\n",
                       LP_OWN_BACK_FAILED_LOG_SKIP_MS);

                prAdapter->u4OwnFailedLogCount++;
                if (prAdapter->u4OwnFailedLogCount >
                    LP_OWN_BACK_FAILED_RESET_CNT) {
                    /* Trigger RESET */
                }
                GET_CURRENT_SYSTIME(
                    &prAdapter->rLastOwnFailedLogTime);
            }

            prAdapter->u4OwnFailedCount++;
            fgStatus = false;
            break;
        }

        if (i == 0) {
            /* Software get LP ownership - only one time.
             * Suppose one CLR_LP_OWN will trigger firmware to
             * return the hif_own. If not, there is something wrong
             * in chipset.
             */
            HAL_LP_OWN_CLR(prAdapter, &fgResult);
        }

        /* Delay for LP engine to complete its operation. */
        kalMsleep(LP_OWN_BACK_LOOP_DELAY_MS);
        i++;
    }

    /* For Low power Test */
    /* 1. Driver need to polling until CR4 ready, then could do normal Tx/Rx
     */
    /* 2. Send a dummy command to change data path to store-forward mode */

    if (prAdapter->fgIsFwDownloaded) {
        u4CurrTick = kalGetTimeTick();
        while (1) {
            HAL_WIFI_FUNC_READY_CHECK(
                prAdapter, WIFI_FUNC_READY_BITS, &fgReady);

            if (TIME_BEFORE(kalGetTimeTick(), u4CurrTick)) {  /* To
                                                               * prevent
                                                               * timer
                                                               * wraparound
                                                               */
                fgTimeout =
                    ((kalGetTimeTick() + (~u4CurrTick)) >
                     LP_OWN_BACK_TOTAL_DELAY_MS) ?
                    true :
                    false;
            } else {
                fgTimeout = ((kalGetTimeTick() - u4CurrTick) >
                             LP_OWN_BACK_TOTAL_DELAY_MS) ?
                            true :
                            false;
            }

            if (fgReady) {
                break;
            } else if (kalIsCardRemoved(prAdapter->prGlueInfo) ||
                       fgIsBusAccessFailed || fgTimeout ||
                       wlanIsChipNoAck(prAdapter)) {
#if CFG_SUPPORT_LOW_POWER_DEBUG
                /* For driver own back fail debug, get current
                 * PC value */
                halPrintMailbox(prAdapter);
                halPollDbgCr(
                    prAdapter,
                    LP_OWN_BACK_FAILED_DBGCR_POLL_ROUND);
#endif

                DBGLOG(INIT, INFO,
                       "Skip waiting CR4 ready for next %ums\n",
                       LP_OWN_BACK_FAILED_LOG_SKIP_MS);
                fgStatus = false;

                if (fgTimeout) {
                    /* Trigger RESET */
                }

                break;
            }
            /* Delay for CR4 to complete its operation. */
            kalMsleep(LP_OWN_BACK_LOOP_DELAY_MS);
        }

        HAL_MCR_RD(prAdapter, MCR_D2HRM1R, &i);
        if (i == 0x77889901) {
            /* fgIsWakeupFromDeepSleep */
            wlanSendDummyCmd(prAdapter, false);

            /* Workaround for dummy command which is not count in Tx
             * done count */
            prAdapter->prGlueInfo->rHifInfo
            .au4PendingTxDoneCount[TC4_INDEX]--;
        }
    }

    return fgStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to process the POWER ON procedure.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void halSetFWOwn(IN P_ADAPTER_T prAdapter, IN u8 fgEnableGlobalInt)
{
    u8 fgResult;

    ASSERT(prAdapter);

    ASSERT(prAdapter->u4PwrCtrlBlockCnt != 0);
    /* Decrease Block to Enter Low Power Semaphore count */
    GLUE_DEC_REF_CNT(prAdapter->u4PwrCtrlBlockCnt);

    if (prAdapter->u4PwrCtrlBlockCnt != 0) {
        return;
    }

    if (prAdapter->fgForceFwOwn == false) {
        if (prAdapter->fgWiFiInSleepyState == false) {
            return;
        }
    }

    if (prAdapter->fgIsFwOwn == true) {
        return;
    }

    if ((nicProcessIST(prAdapter) != WLAN_STATUS_NOT_INDICATING) &&
        !nicSerIsWaitingReset(prAdapter)) {
        DBGLOG(INIT, INFO, "FW OWN Skipped due to pending INT\n");
        /* pending interrupts */
        return;
    }

    if (fgEnableGlobalInt) {
        prAdapter->fgIsIntEnableWithLPOwnSet = true;
    } else {
        HAL_LP_OWN_SET(prAdapter, &fgResult);

        if (fgResult) {
            /* if set firmware own not successful (possibly pending
             * interrupts), */
            /* indicate an own clear event */
            HAL_LP_OWN_CLR(prAdapter, &fgResult);

            return;
        }

        prAdapter->fgIsFwOwn = true;

        DBGLOG(INIT, INFO, "FW OWN\n");
    }
}

void halWakeUpWiFi(IN P_ADAPTER_T prAdapter)
{
    u8 fgResult;

    ASSERT(prAdapter);

    HAL_LP_OWN_RD(prAdapter, &fgResult);

    if (fgResult) {
        prAdapter->fgIsFwOwn = false;
    }else{
        HAL_LP_OWN_CLR(prAdapter, &fgResult);
    }

#if CFG_SUPPORT_LOW_POWER_DEBUG
    /* Polling MCU programming counter */
    halPollDbgCr(prAdapter, LP_DBGCR_POLL_ROUND);
#endif
}

void halDevInit(IN P_ADAPTER_T prAdapter)
{
    u32 u4Value = 0;

    ASSERT(prAdapter);

#if CFG_SDIO_INTR_ENHANCE
    /* 4 <1> Check STATUS Buffer is DW alignment. */
    ASSERT(IS_ALIGN_4((unsigned long)&prAdapter->prGlueInfo->rHifInfo
                      .prSDIOCtrl->u4WHISR));

    /* 4 <2> Setup STATUS count. */
    {
        HAL_MCR_RD(prAdapter, MCR_WHCR, &u4Value);

        /* 4 <2.1> Setup the number of maximum RX length to be report */
        u4Value &= ~(WHCR_MAX_HIF_RX_LEN_NUM);
        u4Value |= ((SDIO_MAXIMUM_RX_LEN_NUM
                     << WHCR_OFFSET_MAX_HIF_RX_LEN_NUM));

        /* 4 <2.2> Setup RX enhancement mode */
#if CFG_SDIO_RX_ENHANCE
        u4Value |= WHCR_RX_ENHANCE_MODE_EN;
#else
        u4Value &= ~WHCR_RX_ENHANCE_MODE_EN;
#endif

        HAL_MCR_WR(prAdapter, MCR_WHCR, u4Value);
    }
#endif

    HAL_MCR_WR(prAdapter, MCR_WHIER, WHIER_DEFAULT);

    HAL_CFG_MAX_HIF_RX_LEN_NUM(prAdapter, HIF_RX_MAX_AGG_NUM);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Driver maintain a variable that is synchronous with the usage of
 * individual TC Buffer Count. This function will calculate TC page count
 * according to the given TX_STATUS COUNTER after TX Done.
 *
 * \param[in] prAdapter              Pointer to the Adapter structure.
 * \param[in] au2TxRlsCnt           array of TX STATUS
 * \param[in] au2FreeTcResource           array of free & available resource
 * count
 *
 * @return true      there are available resource to release
 * @return false     no available resource to release
 */
/*----------------------------------------------------------------------------*/
u8 halTxCalculateResource(IN P_ADAPTER_T prAdapter, IN u16 *au2TxRlsCnt,
                          OUT u16 *au2FreeTcResource)
{
    P_TX_TCQ_STATUS_T prTcqStatus;
    u8 bStatus = false;
    u8 ucTcIdx;
    u32 u4TotalTxDoneCnt = 0;
    u32 u4TotalExtraTxDone = 0;
    u32 au4UsedCnt[TC_NUM];
    u32 au4ExtraTxDone[TC_NUM];

    u32 *au4TxDoneCnt;
    u32 *au4PreUsedCnt;
    u32 u4AvaliableCnt;
    u8 fgEnExtraTxDone;

    KAL_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);
    prTcqStatus = &prAdapter->rTxCtrl.rTc;

    au4TxDoneCnt = prTcqStatus->au4TxDonePageCount;
    au4PreUsedCnt = prTcqStatus->au4PreUsedPageCount;
    u4AvaliableCnt = prTcqStatus->u4AvaliablePageCount;
    fgEnExtraTxDone = prAdapter->rWifiVar.ucExtraTxDone;

    /* Get used page count */
    if (fgEnExtraTxDone) {
        KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
        for (ucTcIdx = TC0_INDEX; ucTcIdx < TC_NUM; ucTcIdx++) {
            au4UsedCnt[ucTcIdx] =
                prTcqStatus->au4MaxNumOfPage[ucTcIdx] -
                prTcqStatus->au4FreePageCount[ucTcIdx];
        }
        KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
    }

    /* Get Tx done & available page count */
    u4AvaliableCnt += au2TxRlsCnt[HIF_TX_FFA_INDEX];
    for (ucTcIdx = TC0_INDEX; ucTcIdx < TC_NUM; ucTcIdx++) {
        /* Get Tx done count from Tx interrupt status */
        au4TxDoneCnt[ucTcIdx] +=
            au2TxRlsCnt[nicTxGetTxQByTc(prAdapter, ucTcIdx)];

        /* Get available EXTRA Tx done */
        if (fgEnExtraTxDone) {
            /* Release Tx done if there are pre-used resource */
            if (au4TxDoneCnt[ucTcIdx] >= au4PreUsedCnt[ucTcIdx]) {
                au4TxDoneCnt[ucTcIdx] -= au4PreUsedCnt[ucTcIdx];
                au4PreUsedCnt[ucTcIdx] = 0;
            } else {
                au4PreUsedCnt[ucTcIdx] -= au4TxDoneCnt[ucTcIdx];
                au4TxDoneCnt[ucTcIdx] = 0;
            }

            /* Calculate extra Tx done to share rest FFA resource */
            if (au4TxDoneCnt[ucTcIdx] >= au4UsedCnt[ucTcIdx]) {
                au4TxDoneCnt[ucTcIdx] = au4UsedCnt[ucTcIdx];
                au4ExtraTxDone[ucTcIdx] = 0;
            } else {
                au4ExtraTxDone[ucTcIdx] = au4UsedCnt[ucTcIdx] -
                                          au4TxDoneCnt[ucTcIdx];
            }
            u4TotalExtraTxDone += au4ExtraTxDone[ucTcIdx];
        }

        u4TotalTxDoneCnt += au4TxDoneCnt[ucTcIdx];
    }

    DBGLOG(TX, TRACE, "TxDone result, FFA[%u] AC[%u:%u:%u:%u] CPU[%u]\n",
           au2TxRlsCnt[HIF_TX_FFA_INDEX], au2TxRlsCnt[HIF_TX_AC0_INDEX],
           au2TxRlsCnt[HIF_TX_AC1_INDEX], au2TxRlsCnt[HIF_TX_AC2_INDEX],
           au2TxRlsCnt[HIF_TX_AC3_INDEX], au2TxRlsCnt[HIF_TX_CPU_INDEX]);

    DBGLOG(TX, TRACE, "TxDone Page count, TC[%u:%u:%u:%u:%u:%u]\n",
           au4TxDoneCnt[TC0_INDEX], au4TxDoneCnt[TC1_INDEX],
           au4TxDoneCnt[TC2_INDEX], au4TxDoneCnt[TC3_INDEX],
           au4TxDoneCnt[TC4_INDEX], au4TxDoneCnt[TC5_INDEX]);

    /* Calculate free Tc page count */
    if (u4AvaliableCnt && u4TotalTxDoneCnt) {
        /* Distribute resource by Tx done counter */
        if (u4AvaliableCnt >= u4TotalTxDoneCnt) {
            /* Fulfill all TC resource */
            kalMemCopy(au2FreeTcResource,
                       prTcqStatus->au4TxDonePageCount,
                       sizeof(prTcqStatus->au4TxDonePageCount));

            kalMemZero(prTcqStatus->au4TxDonePageCount,
                       sizeof(prTcqStatus->au4TxDonePageCount));

            u4AvaliableCnt -= u4TotalTxDoneCnt;
        } else {
            /* Round-robin distribute resource */
            ucTcIdx = prTcqStatus->ucNextTcIdx;
            while (u4AvaliableCnt) {
                /* Enough resource, fulfill this TC */
                if (u4AvaliableCnt >= au4TxDoneCnt[ucTcIdx]) {
                    au2FreeTcResource[ucTcIdx] =
                        au4TxDoneCnt[ucTcIdx];
                    u4AvaliableCnt -= au4TxDoneCnt[ucTcIdx];
                    au4TxDoneCnt[ucTcIdx] = 0;

                    /* Round-robin get next TC */
                    ucTcIdx++;
                    ucTcIdx %= TC_NUM;
                }
                /* no more resource, distribute rest of resource
                 * to this TC */
                else {
                    au2FreeTcResource[ucTcIdx] =
                        u4AvaliableCnt;
                    au4TxDoneCnt[ucTcIdx] -= u4AvaliableCnt;
                    u4AvaliableCnt = 0;
                }
            }
            prTcqStatus->ucNextTcIdx = ucTcIdx;
        }
        bStatus = true;
    }

    if (u4AvaliableCnt && u4TotalExtraTxDone && fgEnExtraTxDone) {
        /* Distribute resource by EXTRA Tx done counter */
        if (u4AvaliableCnt >= u4TotalExtraTxDone) {
            for (ucTcIdx = TC0_INDEX; ucTcIdx < TC_NUM; ucTcIdx++) {
                au2FreeTcResource[ucTcIdx] +=
                    au4ExtraTxDone[ucTcIdx];
                au4PreUsedCnt[ucTcIdx] +=
                    au4ExtraTxDone[ucTcIdx];
                au4ExtraTxDone[ucTcIdx] = 0;
            }

            u4AvaliableCnt -= u4TotalExtraTxDone;
        } else {
            /* Round-robin distribute resource */
            ucTcIdx = prTcqStatus->ucNextTcIdx;
            while (u4AvaliableCnt) {
                /* Enough resource, fulfill this TC */
                if (u4AvaliableCnt >= au4ExtraTxDone[ucTcIdx]) {
                    au2FreeTcResource[ucTcIdx] +=
                        au4ExtraTxDone[ucTcIdx];
                    au4PreUsedCnt[ucTcIdx] +=
                        au4ExtraTxDone[ucTcIdx];
                    u4AvaliableCnt -=
                        au4ExtraTxDone[ucTcIdx];
                    au4ExtraTxDone[ucTcIdx] = 0;

                    /* Round-robin get next TC */
                    ucTcIdx++;
                    ucTcIdx %= TC_NUM;
                }
                /* no more resource, distribute rest of resource
                 * to this TC */
                else {
                    au2FreeTcResource[ucTcIdx] +=
                        u4AvaliableCnt;
                    au4PreUsedCnt[ucTcIdx] +=
                        u4AvaliableCnt;
                    au4ExtraTxDone[ucTcIdx] -=
                        u4AvaliableCnt;
                    u4AvaliableCnt = 0;
                }
            }
            prTcqStatus->ucNextTcIdx = ucTcIdx;
        }
        bStatus = true;
    }

    prTcqStatus->u4AvaliablePageCount = u4AvaliableCnt;

    return bStatus;
}
u8 halTxReleaseResource(IN P_ADAPTER_T prAdapter, IN u16 *au2TxRlsCnt)
{
    P_TX_TCQ_STATUS_T prTcqStatus;
    u8 bStatus = false;
    u32 i;
    P_SDIO_STAT_COUNTER_T prStatCnt;
    u16 au2TxDoneCnt[HIF_TX_NUM] = { 0 };
    u16 u2ReturnCnt;

    KAL_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);
    prTcqStatus = &prAdapter->rTxCtrl.rTc;
    prStatCnt = &prAdapter->prGlueInfo->rHifInfo.rStatCounter;

    /* Update Free Tc resource counter */
    for (i = HIF_TX_AC0_INDEX; i <= HIF_TX_AC23_INDEX; i++) {
#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
        au2TxDoneCnt[i] = au2TxRlsCnt[i];
#else
        au2TxDoneCnt[i % WMM_AC_INDEX_NUM] += au2TxRlsCnt[i];
#endif
    }
    au2TxDoneCnt[HIF_TX_CPU_INDEX] = au2TxRlsCnt[HIF_TX_CPU_INDEX];

#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
    /*wmm_3: P2P_DEV*/
    au2TxDoneCnt[HIF_TX_AC3X_INDEX] = au2TxRlsCnt[HIF_TX_AC3X_INDEX];
#endif

    /* Return free Tc page count */
    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
    for (i = TC0_INDEX; i < TC_NUM; i++) {
        if ((i == TC5_INDEX) || (i == TC10_INDEX) ||
            (i == TC15_INDEX)) {
            /*bypass TC5/TC10/TC15, due to not use*/
            continue;
        }
#else
    for (i = TC0_INDEX; i < TC5_INDEX; i++) {
#endif
        u2ReturnCnt = au2TxDoneCnt[nicTxGetTxQByTc(prAdapter, i)];
        nicTxReleaseResource(prAdapter, i, u2ReturnCnt, false);
        prAdapter->prGlueInfo->rHifInfo.au4PendingTxDoneCount[i] -=
            u2ReturnCnt;
    }
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
    bStatus = true;

    /* Update Statistic counter */
    prStatCnt->u4TxDonePendingPktCnt += nicTxGetMsduPendingCnt(prAdapter);
    prStatCnt->u4TxDoneIntTotCnt++;

    for (i = HIF_TX_AC0_INDEX; i < HIF_TX_NUM; i++) {
        if (au2TxRlsCnt[i]) {
            prStatCnt->u4TxDoneCnt[i] += au2TxRlsCnt[i];
            prStatCnt->u4TxDoneIntCnt[i]++;
        }
    }

    if (!nicTxSanityCheckResource(prAdapter)) {
        DBGLOG(TX, ERROR,
               "Tx Done INT result, FFA[%u] AC[%u:%u:%u:%u] CPU[%u]\n",
               au2TxRlsCnt[HIF_TX_FFA_INDEX],
               au2TxRlsCnt[HIF_TX_AC0_INDEX],
               au2TxRlsCnt[HIF_TX_AC1_INDEX],
               au2TxRlsCnt[HIF_TX_AC2_INDEX],
               au2TxRlsCnt[HIF_TX_AC3_INDEX],
               au2TxRlsCnt[HIF_TX_CPU_INDEX]);
#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
        DBGLOG(TX, ERROR, "Tx Done INT result, AC[%u:%u:%u:%u]\n",
               au2TxRlsCnt[HIF_TX_AC10_INDEX],
               au2TxRlsCnt[HIF_TX_AC11_INDEX],
               au2TxRlsCnt[HIF_TX_AC12_INDEX],
               au2TxRlsCnt[HIF_TX_AC13_INDEX]);

        DBGLOG(TX, ERROR, "Tx Done INT result_2, AC[%u:%u:%u:%u:%u]\n",
               au2TxRlsCnt[HIF_TX_AC20_INDEX],
               au2TxRlsCnt[HIF_TX_AC21_INDEX],
               au2TxRlsCnt[HIF_TX_AC22_INDEX],
               au2TxRlsCnt[HIF_TX_AC23_INDEX],
               au2TxRlsCnt[HIF_TX_AC3X_INDEX]);
#endif
    }

    return bStatus;
}

WLAN_STATUS halTxPollingResource(IN P_ADAPTER_T prAdapter, IN u8 ucTC)
{
    P_TX_CTRL_T prTxCtrl;
    WLAN_STATUS u4Status = WLAN_STATUS_RESOURCES;
    u32 au4WTSR[8];
    P_GL_HIF_INFO_T prHifInfo;

    prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

    prTxCtrl = &prAdapter->rTxCtrl;

    if (prHifInfo->fgIsPendingInt &&
        (prHifInfo->prSDIOCtrl->u4WHISR & WHISR_TX_DONE_INT)) {
        /* Get Tx done resource from pending interrupt status */
        kalMemCopy(au4WTSR, &prHifInfo->prSDIOCtrl->rTxInfo,
                   sizeof(u32) * 8);

        /* Clear pending Tx done interrupt */
        prHifInfo->prSDIOCtrl->u4WHISR &= ~WHISR_TX_DONE_INT;
    } else {
        HAL_READ_TX_RELEASED_COUNT(prAdapter, au4WTSR);
    }

    if (kalIsCardRemoved(prAdapter->prGlueInfo) == true ||
        fgIsBusAccessFailed == true) {
        u4Status = WLAN_STATUS_FAILURE;
    } else if (halTxReleaseResource(prAdapter, (u16 *)au4WTSR)) {
        if (prTxCtrl->rTc.au4FreeBufferCount[ucTC] > 0) {
            u4Status = WLAN_STATUS_SUCCESS;
        }
    }

    return u4Status;
}

void halTxInterruptSanityCheck(IN P_ADAPTER_T prAdapter, IN u16 *au2TxRlsCnt)
{
    u8 ucIdx;
    u8 fgError = false;

    if (prAdapter->rWifiVar.ucTxDbg & BIT(1)) {
        for (ucIdx = HIF_TX_AC0_INDEX; ucIdx < HIF_TX_NUM; ucIdx++) {
            if (au2TxRlsCnt[ucIdx] >
                prAdapter->rTxCtrl.u4TotalPageNum) {
                fgError = true;
            }
        }

        if (fgError) {
            DBGLOG(TX,
                   ERROR,
                   "Tx Done INT result, FFA[%u] AC[%u:%u:%u:%u] CPU[%u]\n",
                   au2TxRlsCnt[HIF_TX_FFA_INDEX],
                   au2TxRlsCnt[HIF_TX_AC0_INDEX],
                   au2TxRlsCnt[HIF_TX_AC1_INDEX],
                   au2TxRlsCnt[HIF_TX_AC2_INDEX],
                   au2TxRlsCnt[HIF_TX_AC3_INDEX],
                   au2TxRlsCnt[HIF_TX_CPU_INDEX]);
        }
    }
}

#if CFG_SDIO_INTR_ENHANCE
void halProcessEnhanceInterruptStatus(IN P_ADAPTER_T prAdapter)
{
    P_SDIO_CTRL_T prSDIOCtrl = prAdapter->prGlueInfo->rHifInfo.prSDIOCtrl;

    /* Set Tx done interrupt if there are Tx done count */
    if ((prSDIOCtrl->u4WHISR & WHISR_TX_DONE_INT) == 0 &&
        (prSDIOCtrl->rTxInfo.au4WTSR[0] | prSDIOCtrl->rTxInfo.au4WTSR[1] |
         prSDIOCtrl->rTxInfo.au4WTSR[2] | prSDIOCtrl->rTxInfo.au4WTSR[3] |
         prSDIOCtrl->rTxInfo.au4WTSR[4] | prSDIOCtrl->rTxInfo.au4WTSR[5] |
         prSDIOCtrl->rTxInfo.au4WTSR[6] | prSDIOCtrl->rTxInfo.au4WTSR[7])) {
        prSDIOCtrl->u4WHISR |= WHISR_TX_DONE_INT;
    }

    /* Set SW ASSERT INFO interrupt if there are pending mail box */
    if (((prSDIOCtrl->u4WHISR & WHISR_D2H_SW_ASSERT_INFO_INT) == 0) &&
        HAL_GET_MAILBOX_READ_CLEAR(prAdapter) &&
        (prSDIOCtrl->u4RcvMailbox0 || prSDIOCtrl->u4RcvMailbox1)) {
        prSDIOCtrl->u4WHISR |= WHISR_D2H_SW_ASSERT_INFO_INT;
    }
}
#endif

void halProcessTxInterrupt(IN P_ADAPTER_T prAdapter)
{
    P_TX_CTRL_T prTxCtrl;
#if CFG_SDIO_INTR_ENHANCE
    P_SDIO_CTRL_T prSDIOCtrl;
#else
    u32 au4TxCount[2];
#endif
    SDIO_TIME_INTERVAL_DEC();

    ASSERT(prAdapter);

    prTxCtrl = &prAdapter->rTxCtrl;
    ASSERT(prTxCtrl);

    SDIO_REC_TIME_START();

    /* Get the TX STATUS */
#if CFG_SDIO_INTR_ENHANCE
    prSDIOCtrl = prAdapter->prGlueInfo->rHifInfo.prSDIOCtrl;
#if DBG
    /* DBGLOG_MEM8(RX, TRACE, (u8 *)prSDIOCtrl, sizeof(SDIO_CTRL_T)); */
#endif

    halTxInterruptSanityCheck(prAdapter, (u16 *)&prSDIOCtrl->rTxInfo);
    halTxReleaseResource(prAdapter, (u16 *)&prSDIOCtrl->rTxInfo);
    kalMemZero(&prSDIOCtrl->rTxInfo, sizeof(prSDIOCtrl->rTxInfo));
#else
    HAL_MCR_RD(prAdapter, MCR_WTSR0, &au4TxCount[0]);
    HAL_MCR_RD(prAdapter, MCR_WTSR1, &au4TxCount[1]);
    DBGLOG(EMU, TRACE, "MCR_WTSR0: 0x%x, MCR_WTSR1: 0x%x\n", au4TxCount[0],
           au4TxCount[1]);

    halTxReleaseResource(prAdapter, (u8 *)au4TxCount);
#endif

    nicTxAdjustTcq(prAdapter);

    SDIO_REC_TIME_END();
    SDIO_ADD_TIME_INTERVAL(
        prAdapter->prGlueInfo->rHifInfo.rStatCounter.u4TxDoneIntTime);
}

#if !CFG_SDIO_INTR_ENHANCE
/*----------------------------------------------------------------------------*/
/*!
 * @brief Read the rx data from data port and setup RFB
 *
 * @param prAdapter pointer to the Adapter handler
 * @param prSWRfb the RFB to receive rx data
 *
 * @retval WLAN_STATUS_SUCCESS: SUCCESS
 * @retval WLAN_STATUS_FAILURE: FAILURE
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS halRxReadBuffer(IN P_ADAPTER_T prAdapter,
                            IN OUT P_SW_RFB_T prSwRfb)
{
    P_RX_CTRL_T prRxCtrl;
    u8 *pucBuf;
    P_HW_MAC_RX_DESC_T prRxStatus;
    u32 u4PktLen = 0, u4ReadBytes;
    WLAN_STATUS u4Status = WLAN_STATUS_SUCCESS;
    u8 fgResult = true;
    u32 u4RegValue;
    u32 rxNum;

    DEBUGFUNC("halRxReadBuffer");

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    pucBuf = prSwRfb->pucRecvBuff;
    prRxStatus = prSwRfb->prRxStatus;

    ASSERT(prRxStatus);
    ASSERT(pucBuf);
    DBGLOG(RX, TRACE, "pucBuf= 0x%x, prRxStatus= 0x%x\n", pucBuf,
           prRxStatus);

    do {
        /* Read the RFB DW length and packet length */
        HAL_MCR_RD(prAdapter, MCR_WRPLR, &u4RegValue);
        if (!fgResult) {
            DBGLOG(RX, ERROR, "Read RX Packet Lentgh Error\n");
            return WLAN_STATUS_FAILURE;
        }
        /* 20091021 move the line to get the HIF RX header (for RX0/1)
         */
        if (u4RegValue == 0) {
            DBGLOG(RX, ERROR, "No RX packet\n");
            return WLAN_STATUS_FAILURE;
        }

        u4PktLen = u4RegValue & BITS(0, 15);
        if (u4PktLen != 0) {
            rxNum = 0;
        } else {
            rxNum = 1;
            u4PktLen = (u4RegValue & BITS(16, 31)) >> 16;
        }

        DBGLOG(RX, TRACE, "RX%d: u4PktLen = %d\n", rxNum, u4PktLen);

        /* 4 <4> Read Entire RFB and packet, include HW appended DW
         * (Checksum Status) */
        u4ReadBytes = ALIGN_4(u4PktLen) + 4;
        HAL_READ_RX_PORT(prAdapter, rxNum, u4ReadBytes, pucBuf,
                         CFG_RX_MAX_PKT_SIZE);

        /* 20091021 move the line to get the HIF RX header */
        /* u4PktLen = (u32)prHifRxHdr->u2PacketLen; */
        if (u4PktLen !=
            (u32)HAL_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus)) {
            DBGLOG(RX,
                   ERROR,
                   "Read u4PktLen = %d, prHifRxHdr->u2PacketLen: %d\n",
                   u4PktLen,
                   HAL_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus));
#if DBG
            DBGLOG_MEM8(RX, TRACE, (u8 *)prRxStatus,
                        (HAL_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus) >
                         4096) ?
                        4096 :
                        prRxStatus->u2RxByteCount);
#endif
            ASSERT(0);
        }
        /* u4PktLen is byte unit, not inlude HW appended DW */

        prSwRfb->ucPacketType =
            (u8)HAL_RX_STATUS_GET_PKT_TYPE(prRxStatus);
        DBGLOG(RX, TRACE, "ucPacketType = %d\n", prSwRfb->ucPacketType);

        prSwRfb->ucStaRecIdx = secGetStaIdxByWlanIdx(
            prAdapter, (u8)HAL_RX_STATUS_GET_WLAN_IDX(prRxStatus));

        /* fgResult will be updated in MACRO */
        if (!fgResult) {
            return WLAN_STATUS_FAILURE;
        }

        DBGLOG(RX, TRACE, "Dump RX buffer, length = 0x%x\n",
               u4ReadBytes);
        DBGLOG_MEM8(RX, TRACE, pucBuf, u4ReadBytes);
    } while (false);

    return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Read frames from the data port, fill RFB
 *        and put each frame into the rReceivedRFBList queue.
 *
 * @param prAdapter   Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void halRxSDIOReceiveRFBs(IN P_ADAPTER_T prAdapter)
{
    P_RX_CTRL_T prRxCtrl;
    P_SW_RFB_T prSwRfb = (P_SW_RFB_T)NULL;
    P_HW_MAC_RX_DESC_T prRxStatus;
    u32 u4HwAppendDW;
    u32 *pu4Temp;

    KAL_SPIN_LOCK_DECLARATION();

    DEBUGFUNC("halRxSDIOReceiveRFBs");

    ASSERT(prAdapter);

    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    do {
        KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
        QUEUE_REMOVE_HEAD(&prRxCtrl->rFreeSwRfbList, prSwRfb,
                          P_SW_RFB_T);
        KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

        if (!prSwRfb) {
            DBGLOG(RX, TRACE, "No More RFB\n");
            break;
        }
        /* need to consider */
        if (halRxReadBuffer(prAdapter, prSwRfb) ==
            WLAN_STATUS_FAILURE) {
            DBGLOG(RX, TRACE, "halRxFillRFB failed\n");
            nicRxReturnRFB(prAdapter, prSwRfb);
            break;
        }

        KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
        QUEUE_INSERT_TAIL(&prRxCtrl->rReceivedRfbList,
                          &prSwRfb->rQueEntry);
        RX_INC_CNT(prRxCtrl, RX_MPDU_TOTAL_COUNT);
        KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);

        prRxStatus = prSwRfb->prRxStatus;
        ASSERT(prRxStatus);

        pu4Temp = (u32 *)prRxStatus;
        u4HwAppendDW =
            *(pu4Temp + (ALIGN_4(prRxStatus->u2RxByteCount) >> 2));
        DBGLOG(RX, TRACE, "u4HwAppendDW = 0x%x\n", u4HwAppendDW);
        DBGLOG(RX, TRACE, "u2PacketLen = 0x%x\n",
               HAL_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus));
    } while (false);
}

#else
/*----------------------------------------------------------------------------*/
/*!
 * @brief Read frames from the data port, fill RFB
 *        and put each frame into the rReceivedRFBList queue.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param u4DataPort     Specify which port to read
 * @param u2RxLength     Specify to the the rx packet length in Byte.
 * @param prSwRfb        the RFB to receive rx data.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/

WLAN_STATUS
halRxEnhanceReadBuffer(IN P_ADAPTER_T prAdapter, IN u32 u4DataPort,
                       IN u16 u2RxLength, IN OUT P_SW_RFB_T prSwRfb)
{
    P_RX_CTRL_T prRxCtrl;
    u8 *pucBuf;
    P_HW_MAC_RX_DESC_T prRxStatus;
    u32 u4PktLen = 0;
    WLAN_STATUS u4Status = WLAN_STATUS_FAILURE;
    u8 fgResult = true;

    DEBUGFUNC("halRxEnhanceReadBuffer");

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    pucBuf = prSwRfb->pucRecvBuff;
    ASSERT(pucBuf);

    prRxStatus = prSwRfb->prRxStatus;
    ASSERT(prRxStatus);

    /* DBGLOG(RX, TRACE, ("u2RxLength = %d\n", u2RxLength)); */

    do {
        /* 4 <1> Read RFB frame from MCR_WRDR0, include HW appended DW
         */
        HAL_READ_RX_PORT(prAdapter, u4DataPort,
                         ALIGN_4(u2RxLength + HIF_RX_HW_APPENDED_LEN),
                         pucBuf, CFG_RX_MAX_PKT_SIZE);

        if (!fgResult) {
            DBGLOG(RX, ERROR, "Read RX Packet Lentgh Error\n");
            break;
        }

        u4PktLen = (u32)(HAL_RX_STATUS_GET_RX_BYTE_CNT(prRxStatus));
        /* DBGLOG(RX, TRACE, ("u4PktLen = %d\n", u4PktLen)); */

        prSwRfb->ucPacketType =
            (u8)HAL_RX_STATUS_GET_PKT_TYPE(prRxStatus);
        /* DBGLOG(RX, TRACE, ("ucPacketType = %d\n",
         * prSwRfb->ucPacketType)); */

        prSwRfb->ucStaRecIdx = secGetStaIdxByWlanIdx(
            prAdapter, (u8)HAL_RX_STATUS_GET_WLAN_IDX(prRxStatus));

        /* 4 <2> if the RFB dw size or packet size is zero */
        if (u4PktLen == 0) {
            DBGLOG(RX, ERROR, "Packet Length = %lu\n", u4PktLen);
            ASSERT(0);
            break;
        }
        /* 4 <3> if the packet is too large or too small */
        /* ToDo[6630]: adjust CFG_RX_MAX_PKT_SIZE */
        if ((u4PktLen > CFG_RX_MAX_PKT_SIZE) ||
            (u4PktLen < sizeof(HW_MAC_RX_DESC_T))) {
            DBGLOG(RX, TRACE, "Read RX Packet Lentgh Error (%lu)\n",
                   u4PktLen);
            ASSERT(0);
            break;
        }

        u4Status = WLAN_STATUS_SUCCESS;
    } while (false);

    DBGLOG_MEM8(RX, TRACE, pucBuf,
                ALIGN_4(u2RxLength + HIF_RX_HW_APPENDED_LEN));
    return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Read frames from the data port for SDIO
 *        I/F, fill RFB and put each frame into the rReceivedRFBList queue.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void halRxSDIOEnhanceReceiveRFBs(IN P_ADAPTER_T prAdapter)
{
    P_SDIO_CTRL_T prSDIOCtrl;
    P_RX_CTRL_T prRxCtrl;
    P_SW_RFB_T prSwRfb = (P_SW_RFB_T)NULL;
    u32 i, rxNum;
    u16 u2RxPktNum, u2RxLength = 0, u2Tmp = 0;

    KAL_SPIN_LOCK_DECLARATION();

    DEBUGFUNC("halRxSDIOEnhanceReceiveRFBs");

    ASSERT(prAdapter);

    prSDIOCtrl = prAdapter->prGlueInfo->rHifInfo.prSDIOCtrl;
    ASSERT(prSDIOCtrl);

    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    for (rxNum = 0; rxNum < 2; rxNum++) {
        u2RxPktNum = (rxNum == 0 ?
                      prSDIOCtrl->rRxInfo.u.u2NumValidRx0Len :
                      prSDIOCtrl->rRxInfo.u.u2NumValidRx1Len);

        if (u2RxPktNum == 0) {
            continue;
        }

        for (i = 0; i < u2RxPktNum; i++) {
            if (rxNum == 0) {
                /* HAL_READ_RX_LENGTH */
                HAL_READ_RX_LENGTH(prAdapter, &u2RxLength,
                                   &u2Tmp);
            } else if (rxNum == 1) {
                /* HAL_READ_RX_LENGTH */
                HAL_READ_RX_LENGTH(prAdapter, &u2Tmp,
                                   &u2RxLength);
            }

            if (!u2RxLength) {
                break;
            }

            KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
            QUEUE_REMOVE_HEAD(&prRxCtrl->rFreeSwRfbList, prSwRfb,
                              P_SW_RFB_T);
            KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

            if (!prSwRfb) {
                DBGLOG(RX, TRACE, "No More RFB\n");
                break;
            }
            ASSERT(prSwRfb);

            if (halRxEnhanceReadBuffer(prAdapter, rxNum, u2RxLength,
                                       prSwRfb) ==
                WLAN_STATUS_FAILURE) {
                DBGLOG(RX, TRACE,
                       "nicRxEnhanceRxReadBuffer failed\n");
                nicRxReturnRFB(prAdapter, prSwRfb);
                break;
            }
            /* prSDIOCtrl->au4RxLength[i] = 0; */

            KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
            QUEUE_INSERT_TAIL(&prRxCtrl->rReceivedRfbList,
                              &prSwRfb->rQueEntry);
            RX_INC_CNT(prRxCtrl, RX_MPDU_TOTAL_COUNT);
            KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
        }
    }

    prSDIOCtrl->rRxInfo.u.u2NumValidRx0Len = 0;
    prSDIOCtrl->rRxInfo.u.u2NumValidRx1Len = 0;
}

#endif

#if CFG_SDIO_RX_AGG
/*----------------------------------------------------------------------------*/
/*!
 * @brief Read frames from the data port for SDIO with Rx aggregation enabled
 *        I/F, fill RFB and put each frame into the rReceivedRFBList queue.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void halRxSDIOAggReceiveRFBs(IN P_ADAPTER_T prAdapter)
{
    P_ENHANCE_MODE_DATA_STRUCT_T prEnhDataStr;
    P_RX_CTRL_T prRxCtrl;
    u32 u4RxLength;
    u32 i, rxNum;
    u32 u4RxAggCount = 0, u4RxAggLength = 0;
    u32 u4RxAvailAggLen;
#if CFG_SDIO_RX_ENHANCE
    u8 *pucSrcAddr;
#endif
    u16 u2RxPktNum;
    P_GL_HIF_INFO_T prHifInfo;
    P_SDIO_RX_COALESCING_BUF_T prRxBuf;
    u8 fgNoFreeBuf = false;

    SDIO_TIME_INTERVAL_DEC();

    DEBUGFUNC("halRxSDIOAggReceiveRFBs");

    ASSERT(prAdapter);

    prRxCtrl = &prAdapter->rRxCtrl;
    prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
    prEnhDataStr = prHifInfo->prSDIOCtrl;

    if (prEnhDataStr->rRxInfo.u.u2NumValidRx0Len == 0 &&
        prEnhDataStr->rRxInfo.u.u2NumValidRx1Len == 0) {
        return;
    }

    for (rxNum = 0; rxNum < 2; rxNum++) {
        u2RxPktNum = (rxNum == 0 ?
                      prEnhDataStr->rRxInfo.u.u2NumValidRx0Len :
                      prEnhDataStr->rRxInfo.u.u2NumValidRx1Len);

        /* if this assertion happened, it is most likely a F/W bug */
        ASSERT(u2RxPktNum <= HIF_RX_MAX_AGG_NUM);

        if (u2RxPktNum > HIF_RX_MAX_AGG_NUM) {
            continue;
        }

        if (u2RxPktNum == 0) {
            continue;
        }

#if CFG_HIF_STATISTICS
        prRxCtrl->u4TotalRxAccessNum++;
        prRxCtrl->u4TotalRxPacketNum += u2RxPktNum;
#endif

        mutex_lock(&prHifInfo->rRxFreeBufQueMutex);
        fgNoFreeBuf = QUEUE_IS_EMPTY(&prHifInfo->rRxFreeBufQueue);
        mutex_unlock(&prHifInfo->rRxFreeBufQueMutex);

        if (fgNoFreeBuf) {
            DBGLOG(RX, TRACE, "[%s] No free Rx buffer\n", __func__);
            prHifInfo->rStatCounter.u4RxBufUnderFlowCnt++;

            if (prAdapter->prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
                QUE_T rTempQue;
                P_QUE_T prTempQue = &rTempQue;

                /* During halt state, move all pending Rx buffer
                 * to free queue */
                mutex_lock(&prHifInfo->rRxDeAggQueMutex);
                QUEUE_MOVE_ALL(prTempQue,
                               &prHifInfo->rRxDeAggQueue);
                mutex_unlock(&prHifInfo->rRxDeAggQueMutex);

                mutex_lock(&prHifInfo->rRxFreeBufQueMutex);
                QUEUE_CONCATENATE_QUEUES(
                    &prHifInfo->rRxFreeBufQueue, prTempQue);
                mutex_unlock(&prHifInfo->rRxFreeBufQueMutex);
            }

            continue;
        }

        u4RxAvailAggLen = HIF_RX_COALESCING_BUFFER_SIZE;
#if CFG_SDIO_RX_ENHANCE
        u4RxAvailAggLen -= (sizeof(ENHANCE_MODE_DATA_STRUCT_T) +
                            HIF_RX_ENHANCE_MODE_PAD_LEN);
#endif
        u4RxAggCount = 0;

        for (i = 0; i < u2RxPktNum; i++) {
            u4RxLength = (rxNum == 0 ? (u32)prEnhDataStr->rRxInfo.u
                          .au2Rx0Len[i] :
                          (u32)prEnhDataStr->rRxInfo.u
                          .au2Rx1Len[i]);

            if (!u4RxLength) {
                ASSERT(0);
                DBGLOG(RX, ERROR, "[%s] RxLength == 0\n",
                       __func__);
                break;
            }

            if (ALIGN_4(u4RxLength + HIF_RX_HW_APPENDED_LEN) <
                u4RxAvailAggLen) {
                u4RxAvailAggLen -= ALIGN_4(
                    u4RxLength + HIF_RX_HW_APPENDED_LEN);
                u4RxAggCount++;
            } else {
                /* CFG_RX_COALESCING_BUFFER_SIZE is not large
                 * enough */
                DBGLOG(RX,
                       ERROR,
                       "[%s] Request_len(%d) >= Available_len(%d)\n",
                       __func__,
                       (ALIGN_4(u4RxLength +
                                HIF_RX_HW_APPENDED_LEN)),
                       u4RxAvailAggLen);
                ASSERT(0);
                break;
            }
        }

        mutex_lock(&prHifInfo->rRxFreeBufQueMutex);
        QUEUE_REMOVE_HEAD(&prHifInfo->rRxFreeBufQueue, prRxBuf,
                          P_SDIO_RX_COALESCING_BUF_T);
        mutex_unlock(&prHifInfo->rRxFreeBufQueMutex);

        prRxBuf->u4PktCount = u4RxAggCount;

        u4RxAggLength =
            (HIF_RX_COALESCING_BUFFER_SIZE - u4RxAvailAggLen);

        SDIO_REC_TIME_START();
        HAL_READ_RX_PORT(prAdapter, rxNum, u4RxAggLength,
                         prRxBuf->pvRxCoalescingBuf,
                         HIF_RX_COALESCING_BUFFER_SIZE);
        SDIO_REC_TIME_END();
        SDIO_ADD_TIME_INTERVAL(prHifInfo->rStatCounter.u4PortReadTime);

#if CFG_SDIO_RX_ENHANCE
        pucSrcAddr = prRxBuf->pvRxCoalescingBuf + u4RxAggLength -
                     sizeof(ENHANCE_MODE_DATA_STRUCT_T);
        kalMemCopy(prHifInfo->prSDIOCtrl, pucSrcAddr,
                   sizeof(ENHANCE_MODE_DATA_STRUCT_T));

        halProcessEnhanceInterruptStatus(prAdapter);

        if (prHifInfo->prSDIOCtrl->u4WHISR) {
            /* Interrupt status without Rx done */
            /* Mask Rx done interrupt to avoid recurrsion */
            u32 u4IntStatus =
                prHifInfo->prSDIOCtrl->u4WHISR &
                (~(WHISR_RX0_DONE_INT | WHISR_RX1_DONE_INT));

            if ((rxNum == 0) &&
                prEnhDataStr->rRxInfo.u.u2NumValidRx1Len &&
                u4IntStatus) {
                /* Handle interrupt here if there are pending Rx
                 * port1 */

                nicProcessIST_impl(prAdapter, u4IntStatus);
            } else {
                prAdapter->prGlueInfo->rHifInfo.fgIsPendingInt =
                    true;
            }
        }
#endif
        halDeAggRxPkt(prAdapter, prRxBuf);

        /* Update statistic counter */
        prHifInfo->rStatCounter.u4PktReadCnt[rxNum] += u4RxAggCount;
        prHifInfo->rStatCounter.u4PortReadCnt[rxNum]++;
    }
}
#endif

void halProcessRxInterrupt(IN P_ADAPTER_T prAdapter)
{
#if CFG_MESON_G12A_PATCH
    halRxSDIOEnhanceReceiveRFBs(prAdapter);
#else
#if CFG_SDIO_INTR_ENHANCE
#if CFG_SDIO_RX_AGG
    halRxSDIOAggReceiveRFBs(prAdapter);
#else
    halRxSDIOEnhanceReceiveRFBs(prAdapter);
#endif
#else
    halRxSDIOReceiveRFBs(prAdapter);
#endif
#endif
}

u32 halDumpHifStatus(IN P_ADAPTER_T prAdapter, IN u8 *pucBuf, IN u32 u4Max)
{
    P_GLUE_INFO_T prGlueInfo = prAdapter->prGlueInfo;
    P_GL_HIF_INFO_T prHifInfo = &prGlueInfo->rHifInfo;
    P_SDIO_STAT_COUNTER_T prStatCnt = &prHifInfo->rStatCounter;
    u32 u4Len = 0;

    /* Print out counter */
    LOGBUF(pucBuf, u4Max, u4Len, "\n");
    LOGBUF(pucBuf, u4Max, u4Len, "------<Dump SDIO Status>------\n");

    LOGBUF(pucBuf,
           u4Max,
           u4Len,
           "Coalescing buffer size[%u] Rx Cnt[%u/%u] DeAgg[%u] UF Cnt[%u]\n",
           prAdapter->u4CoalescingBufCachedSize,
           prHifInfo->rRxFreeBufQueue.u4NumElem,
           HIF_RX_COALESCING_BUF_COUNT,
           prHifInfo->rRxDeAggQueue.u4NumElem,
           prStatCnt->u4RxBufUnderFlowCnt);

    LOGBUF(pucBuf, u4Max, u4Len,
           "Pkt cnt Tx[%u] RxP0[%u] RxP1[%u] Tx/Rx ratio[%u.%u]\n",
           prStatCnt->u4DataPktWriteCnt, prStatCnt->u4PktReadCnt[0],
           prStatCnt->u4PktReadCnt[1],
           DIV2INT(prStatCnt->u4DataPktWriteCnt,
                   prStatCnt->u4PktReadCnt[0]),
           DIV2DEC(prStatCnt->u4DataPktWriteCnt,
                   prStatCnt->u4PktReadCnt[0]));

    LOGBUF(pucBuf, u4Max, u4Len,
           "Tx pkt/wt[%u.%u] pkt/kick[%u.%u] cmd/wt[%u.%u]\n",
           DIV2INT(prStatCnt->u4DataPktWriteCnt,
                   prStatCnt->u4DataPortWriteCnt),
           DIV2DEC(prStatCnt->u4DataPktWriteCnt,
                   prStatCnt->u4DataPortWriteCnt),
           DIV2INT(prStatCnt->u4DataPktWriteCnt,
                   prStatCnt->u4DataPortKickCnt),
           DIV2DEC(prStatCnt->u4DataPktWriteCnt,
                   prStatCnt->u4DataPortKickCnt),
           DIV2INT(prStatCnt->u4CmdPktWriteCnt,
                   prStatCnt->u4CmdPortWriteCnt),
           DIV2DEC(prStatCnt->u4CmdPktWriteCnt,
                   prStatCnt->u4CmdPortWriteCnt));

    LOGBUF(pucBuf, u4Max, u4Len, "Rx P0 pkt/rd[%u.%u] P1 pkt/rd[%u.%u]\n",
           DIV2INT(prStatCnt->u4PktReadCnt[0], prStatCnt->u4PortReadCnt[0]),
           DIV2DEC(prStatCnt->u4PktReadCnt[0], prStatCnt->u4PortReadCnt[0]),
           DIV2INT(prStatCnt->u4PktReadCnt[1], prStatCnt->u4PortReadCnt[1]),
           DIV2DEC(prStatCnt->u4PktReadCnt[1],
                   prStatCnt->u4PortReadCnt[1]));

    LOGBUF(pucBuf, u4Max, u4Len,
           "Tx done pending cnt TC00~05[%u, %u, %u, %u, %u, %u]\n",
           prHifInfo->au4PendingTxDoneCount[TC0_INDEX],
           prHifInfo->au4PendingTxDoneCount[TC1_INDEX],
           prHifInfo->au4PendingTxDoneCount[TC2_INDEX],
           prHifInfo->au4PendingTxDoneCount[TC3_INDEX],
           prHifInfo->au4PendingTxDoneCount[TC4_INDEX],
           prHifInfo->au4PendingTxDoneCount[TC5_INDEX]);

    LOGBUF(pucBuf, u4Max, u4Len, "Tx done counter/int:\n");
    LOGBUF(pucBuf, u4Max, u4Len, "AC00~03[%u.%u, %u.%u, %u.%u, %u.%u]\n",
           DIV2INT(prStatCnt->u4TxDoneCnt[0], prStatCnt->u4TxDoneIntCnt[0]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[0], prStatCnt->u4TxDoneIntCnt[0]),
           DIV2INT(prStatCnt->u4TxDoneCnt[1], prStatCnt->u4TxDoneIntCnt[1]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[1], prStatCnt->u4TxDoneIntCnt[1]),
           DIV2INT(prStatCnt->u4TxDoneCnt[2], prStatCnt->u4TxDoneIntCnt[2]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[2], prStatCnt->u4TxDoneIntCnt[2]),
           DIV2INT(prStatCnt->u4TxDoneCnt[3], prStatCnt->u4TxDoneIntCnt[3]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[3],
                   prStatCnt->u4TxDoneIntCnt[3]));

    LOGBUF(pucBuf, u4Max, u4Len, "AC10~13[%u.%u, %u.%u, %u.%u, %u.%u]\n",
           DIV2INT(prStatCnt->u4TxDoneCnt[4], prStatCnt->u4TxDoneIntCnt[4]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[4], prStatCnt->u4TxDoneIntCnt[4]),
           DIV2INT(prStatCnt->u4TxDoneCnt[5], prStatCnt->u4TxDoneIntCnt[5]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[5], prStatCnt->u4TxDoneIntCnt[5]),
           DIV2INT(prStatCnt->u4TxDoneCnt[6], prStatCnt->u4TxDoneIntCnt[6]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[5], prStatCnt->u4TxDoneIntCnt[5]),
           DIV2INT(prStatCnt->u4TxDoneCnt[7], prStatCnt->u4TxDoneIntCnt[7]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[7],
                   prStatCnt->u4TxDoneIntCnt[7]));

    LOGBUF(pucBuf, u4Max, u4Len,
           "AC20~23[%u.%u, %u.%u, %u.%u, %u.%u] FFA,CPU[%u.%u, %u.%u]\n",
           DIV2INT(prStatCnt->u4TxDoneCnt[8], prStatCnt->u4TxDoneIntCnt[8]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[8], prStatCnt->u4TxDoneIntCnt[8]),
           DIV2INT(prStatCnt->u4TxDoneCnt[9], prStatCnt->u4TxDoneIntCnt[9]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[9], prStatCnt->u4TxDoneIntCnt[9]),
           DIV2INT(prStatCnt->u4TxDoneCnt[10],
                   prStatCnt->u4TxDoneIntCnt[10]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[10],
                   prStatCnt->u4TxDoneIntCnt[10]),
           DIV2INT(prStatCnt->u4TxDoneCnt[11],
                   prStatCnt->u4TxDoneIntCnt[11]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[11],
                   prStatCnt->u4TxDoneIntCnt[11]),
           DIV2INT(prStatCnt->u4TxDoneCnt[14],
                   prStatCnt->u4TxDoneIntCnt[14]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[14],
                   prStatCnt->u4TxDoneIntCnt[14]),
           DIV2INT(prStatCnt->u4TxDoneCnt[15],
                   prStatCnt->u4TxDoneIntCnt[15]),
           DIV2DEC(prStatCnt->u4TxDoneCnt[15],
                   prStatCnt->u4TxDoneIntCnt[15]));

    LOGBUF(pucBuf, u4Max, u4Len,
           "Pending pkt/int[%u.%u] kick/int[%u.%u] rx_enh/sts[%u.%u]\n",
           DIV2INT(prStatCnt->u4TxDonePendingPktCnt,
                   prStatCnt->u4TxDoneIntTotCnt),
           DIV2DEC(prStatCnt->u4TxDonePendingPktCnt,
                   prStatCnt->u4TxDoneIntTotCnt),
           DIV2INT(prStatCnt->u4DataPortKickCnt,
                   prStatCnt->u4TxDoneIntTotCnt),
           DIV2DEC(prStatCnt->u4DataPortKickCnt,
                   prStatCnt->u4TxDoneIntTotCnt),
           DIV2INT((prStatCnt->u4IntCnt - prStatCnt->u4IntReadCnt),
                   prStatCnt->u4IntCnt),
           DIV2DEC((prStatCnt->u4IntCnt - prStatCnt->u4IntReadCnt),
                   prStatCnt->u4IntCnt));

#if CFG_SDIO_TIMING_PROFILING
    LOGBUF(pucBuf, u4Max, u4Len, "Tx cp_t/pkt[%u.%uus] free/pkt[%u.%uus]\n",
           DIV2INT(prStatCnt->u4TxDataCpTime, prStatCnt->u4DataPktWriteCnt),
           DIV2DEC(prStatCnt->u4TxDataCpTime, prStatCnt->u4DataPktWriteCnt),
           DIV2INT(prStatCnt->u4TxDataFreeTime,
                   prStatCnt->u4DataPktWriteCnt),
           DIV2DEC(prStatCnt->u4TxDataFreeTime,
                   prStatCnt->u4DataPktWriteCnt));

    LOGBUF(pucBuf, u4Max, u4Len,
           "Rx P0 cp_t/pkt[%u.%uus] avg read[%u.%uus]\n",
           DIV2INT(prStatCnt->u4RxDataCpTime, prStatCnt->u4PktReadCnt[0]),
           DIV2DEC(prStatCnt->u4RxDataCpTime, prStatCnt->u4PktReadCnt[0]),
           DIV2INT(prStatCnt->u4PortReadTime, prStatCnt->u4PortReadCnt[0]),
           DIV2DEC(prStatCnt->u4PortReadTime, prStatCnt->u4PortReadCnt[0]));

    LOGBUF(pucBuf, u4Max, u4Len,
           "INT rd_sts/sts[%u.%uus] tx_sts/sts[%u.%uus]\n",
           DIV2INT(prStatCnt->u4IntReadTime, prStatCnt->u4IntReadCnt),
           DIV2DEC(prStatCnt->u4IntReadTime, prStatCnt->u4IntReadCnt),
           DIV2INT(prStatCnt->u4TxDoneIntTime,
                   prStatCnt->u4TxDoneIntTotCnt),
           DIV2DEC(prStatCnt->u4TxDoneIntTime,
                   prStatCnt->u4TxDoneIntTotCnt));
#endif

    LOGBUF(pucBuf, u4Max, u4Len, "---------------------------------\n");

    /* Reset statistic counter */
    kalMemZero(prStatCnt, sizeof(SDIO_STAT_COUNTER_T));

    return u4Len;
}

#if (CFG_SDIO_ACCESS_N9_REGISTER_BY_MAILBOX == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *       This routine is used to get the value of N9 register
 *       by SDIO SW interrupt and mailbox.
 *
 * \param[in]
 *       pvAdapter: Pointer to the Adapter structure.
 *       addr: the interested address to be read
 *       prresult: to stored the value of the addr
 *
 * \return
 *       the error of the reading operation
 */
/*----------------------------------------------------------------------------*/

u8 halReadN9RegisterByMailBox(IN P_ADAPTER_T prAdapter, IN u32 addr,
                              IN u32 *prresult)
{
    u32 ori_whlpcr, temp, counter = 0;
    u8 err = true, stop = false;

    /* use polling mode */
    HAL_MCR_RD(prAdapter, MCR_WHLPCR, &ori_whlpcr);  /* backup the original
                                                      * setting of W_INT_EN
                                                      */
    ori_whlpcr &= WHLPCR_INT_EN_SET;
    HAL_MCR_WR(prAdapter, MCR_WHLPCR, WHLPCR_INT_EN_CLR);  /* disabel
                                                            * interrupt */

    /* progrqm h2d mailbox0 as interested register address */
    HAL_MCR_WR(prAdapter, MCR_H2DSM0R, addr);

    /* set h2d interrupt to notify firmware (bit16) */
    HAL_MCR_WR(prAdapter, MCR_WSICR, SDIO_MAILBOX_FUNC_READ_REG_IDX);

    /* polling interrupt status for the returned result */
    while (!stop) {
        HAL_MCR_RD(prAdapter, MCR_WHISR, &temp);  /* read clear mode */
        if (temp & SDIO_MAILBOX_FUNC_READ_REG_IDX) {
            /* get the result */

            /* read d2h mailbox0 for interested register address */
            HAL_MCR_RD(prAdapter, MCR_D2HRM0R, &temp);
            if (temp == addr) {
                /* read d2h mailbox1 for the value of the
                 * register */
                HAL_MCR_RD(prAdapter, MCR_D2HRM1R, prresult);
                err = false;
            } else {
                DBGLOG(HAL,
                       ERROR,
                       "halReadN9RegisterByMailBox >> interested address is not correct.\n");
            }
            stop = true;
        } else {
            counter++;

            if (counter > 300000) {
                DBGLOG(HAL,
                       ERROR,
                       "halReadN9RegisterByMailBox >> get response failure.\n");
                ASSERT(0);
                break;
            }
        }
    }

    HAL_MCR_WR(prAdapter, MCR_WHLPCR, ori_whlpcr);  /* restore the W_INT_EN
                                                     */

    return err;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *       This routine is used to write the value of N9 register by SDIO SW
 * interrupt and mailbox.
 *
 * \param[in]
 *       pvAdapter: Pointer to the Adapter structure.
 *       addr: the interested address to be write
 *       value: the value to write into the addr
 *
 * \return
 *       the error of the write operation
 */
/*----------------------------------------------------------------------------*/

u8 halWriteN9RegisterByMailBox(IN P_ADAPTER_T prAdapter, IN u32 addr,
                               IN u32 value)
{
    u32 ori_whlpcr, temp, counter = 0;
    u8 err = true, stop = false;

    /* use polling mode */
    HAL_MCR_RD(prAdapter, MCR_WHLPCR, &ori_whlpcr);  /* backup the original
                                                      * setting of W_INT_EN
                                                      */
    ori_whlpcr &= WHLPCR_INT_EN_SET;
    HAL_MCR_WR(prAdapter, MCR_WHLPCR, WHLPCR_INT_EN_CLR);  /* disabel
                                                            * interrupt */

    /* progrqm h2d mailbox0 as interested register address */
    HAL_MCR_WR(prAdapter, MCR_H2DSM0R, addr);

    /* progrqm h2d mailbox1 as the value to write */
    HAL_MCR_WR(prAdapter, MCR_H2DSM1R, value);

    /* set h2d interrupt to notify firmware (bit17) */
    HAL_MCR_WR(prAdapter, MCR_WSICR, SDIO_MAILBOX_FUNC_WRITE_REG_IDX);

    /* polling interrupt status for the returned result */
    while (!stop) {
        HAL_MCR_RD(prAdapter, MCR_WHISR, &temp);  /* read clear mode */

        if (temp & SDIO_MAILBOX_FUNC_WRITE_REG_IDX) {
            /* get the result */

            /* read d2h mailbox0 for interested register address */
            HAL_MCR_RD(prAdapter, MCR_D2HRM0R, &temp);
            if (temp == addr) {
                err = false;
            } else {
                DBGLOG(HAL, ERROR,
                       "halWriteN9RegisterByMailBox >> ");
                DBGLOG(HAL, ERROR,
                       "interested address is not correct.\n");
            }
            stop = true;
        } else {
            counter++;

            if (counter > 300000) {
                DBGLOG(HAL,
                       ERROR,
                       "halWriteN9RegisterByMailBox >> get response failure.\n");
                ASSERT(0);
                break;
            }
        }
    }

    HAL_MCR_WR(prAdapter, MCR_WHLPCR, ori_whlpcr);  /* restore the W_INT_EN
                                                     */

    return err;
}
#endif

u8 halIsPendingRx(IN P_ADAPTER_T prAdapter)
{
    return false;
}

u32 halGetValidCoalescingBufSize(IN P_ADAPTER_T prAdapter)
{
    P_GL_HIF_INFO_T prHifInfo;
    u32 u4BufSize;

    struct sdio_func *prSdioFunc;
    u32 u4RuntimeMaxBuf;

    prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

    if (HIF_TX_COALESCING_BUFFER_SIZE > HIF_RX_COALESCING_BUFFER_SIZE) {
        u4BufSize = HIF_TX_COALESCING_BUFFER_SIZE;
    }else{
        u4BufSize = HIF_RX_COALESCING_BUFFER_SIZE;
    }

    prSdioFunc = prHifInfo->func;

    /* Check host capability */
    /* 1. Should less than host-max_req_size */
    if (u4BufSize > prSdioFunc->card->host->max_req_size) {
        u4BufSize = prSdioFunc->card->host->max_req_size;
    }

    /* 2. Should less than runtime-blksize * host-blk_count  */
    u4RuntimeMaxBuf =
        prSdioFunc->cur_blksize * prSdioFunc->card->host->max_blk_count;
    if (u4BufSize > u4RuntimeMaxBuf) {
        u4BufSize = u4RuntimeMaxBuf;
    }

    DBGLOG(INIT, TRACE,
           "\n"
           "Final buf : 0x%X\n"
           "Default TX buf : 0x%X\n"
           "Default RX buf : 0x%X\n"
           "Host caps -\n"
           "max_req_size : 0x%X\n"
           "max_seg_size : 0x%X\n"
           "max_segs : 0x%X\n"
           "max_blk_size : 0x%X\n"
           "max_blk_count : 0x%X\n"
           "Runtime -\n"
           "cur_blksize : 0x%X\n",
           u4BufSize, HIF_TX_COALESCING_BUFFER_SIZE,
           HIF_RX_COALESCING_BUFFER_SIZE,
           prSdioFunc->card->host->max_req_size,
           prSdioFunc->card->host->max_seg_size,
           prSdioFunc->card->host->max_segs,
           prSdioFunc->card->host->max_blk_size,
           prSdioFunc->card->host->max_blk_count, prSdioFunc->cur_blksize);

    return u4BufSize;
}

WLAN_STATUS halAllocateIOBuffer(IN P_ADAPTER_T prAdapter)
{
    P_GL_HIF_INFO_T prHifInfo;
    u8 ucIdx;
    P_SDIO_RX_COALESCING_BUF_T prRxBuf;

    prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

    /* 4 <5> Memory for enhanced interrupt response */
#ifdef CFG_PREALLOC_MEMORY
    prHifInfo->prSDIOCtrl = (P_SDIO_CTRL_T)preallocGetMem(MEM_ID_IO_CTRL);
#else
    prHifInfo->prSDIOCtrl = (P_SDIO_CTRL_T)kalAllocateIOBuffer(
        sizeof(ENHANCE_MODE_DATA_STRUCT_T));
#endif
    if (prHifInfo->prSDIOCtrl == NULL) {
        DBGLOG(HAL, ERROR,
               "Could not allocate %d bytes for interrupt response.\n",
               sizeof(ENHANCE_MODE_DATA_STRUCT_T));

        return WLAN_STATUS_RESOURCES;
    }

    /* Alloc coalescing buffer */
    for (ucIdx = 0; ucIdx < HIF_RX_COALESCING_BUF_COUNT; ucIdx++) {
        prRxBuf = &prHifInfo->rRxCoalesingBuf[ucIdx];

        prRxBuf->u4PktCount = 0;

        prRxBuf->u4BufSize = HIF_RX_COALESCING_BUFFER_SIZE;
#ifdef CFG_PREALLOC_MEMORY
        prRxBuf->pvRxCoalescingBuf = preallocGetMem(MEM_ID_RX_DATA);
#else
        prRxBuf->pvRxCoalescingBuf =
            kalAllocateIOBuffer(prRxBuf->u4BufSize);
#endif
        if (!prRxBuf->pvRxCoalescingBuf) {
            DBGLOG(HAL, ERROR, "Rx coalescing alloc failed!\n");
            continue;
        }
        QUEUE_INSERT_TAIL(&prHifInfo->rRxFreeBufQueue,
                          &prRxBuf->rQueEntry);
    }

    return WLAN_STATUS_SUCCESS;
}

WLAN_STATUS halReleaseIOBuffer(IN P_ADAPTER_T prAdapter)
{
    P_GL_HIF_INFO_T prHifInfo;
    u8 ucIdx;
    P_SDIO_RX_COALESCING_BUF_T prRxBuf;

    prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

    /* Release coalescing buffer */
    for (ucIdx = 0; ucIdx < HIF_RX_COALESCING_BUF_COUNT; ucIdx++) {
        prRxBuf = &prHifInfo->rRxCoalesingBuf[ucIdx];
#ifndef CFG_PREALLOC_MEMORY
        kalReleaseIOBuffer(prRxBuf->pvRxCoalescingBuf,
                           prRxBuf->u4BufSize);
#endif
        prRxBuf->pvRxCoalescingBuf = NULL;
    }

    /* 4 <5> Memory for enhanced interrupt response */
    if (prHifInfo->prSDIOCtrl) {
#ifndef CFG_PREALLOC_MEMORY
        kalReleaseIOBuffer((void *)prHifInfo->prSDIOCtrl,
                           sizeof(ENHANCE_MODE_DATA_STRUCT_T));
#endif
        prHifInfo->prSDIOCtrl = (P_SDIO_CTRL_T)NULL;
    }

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief dump firmware Assert message
 *
 * \param[in]
 *           prAdapter
 *
 * \return
 *           true
 *           false
 */
/*----------------------------------------------------------------------------*/
void halPrintFirmwareAssertInfo(IN P_ADAPTER_T prAdapter)
{
    u32 u4MailBox0, u4MailBox1;
    u32 line = 0;
    u8 aucAssertFile[7];
    /* u32 u4ChipId; */

#if CFG_SDIO_INTR_ENHANCE
    u4MailBox0 = prAdapter->prGlueInfo->rHifInfo.prSDIOCtrl->u4RcvMailbox0;
    u4MailBox1 = prAdapter->prGlueInfo->rHifInfo.prSDIOCtrl->u4RcvMailbox1;
#else
    halGetMailbox(prAdapter, 0, &u4MailBox0);
    halGetMailbox(prAdapter, 1, &u4MailBox1);
#endif

    line = u4MailBox0 & 0x0000FFFF;

    u4MailBox0 = ((u4MailBox0 >> 16) & 0x0000FFFF);

    kalMemCopy(&aucAssertFile[0], &u4MailBox0, 2);
    kalMemCopy(&aucAssertFile[2], &u4MailBox1, 4);

    aucAssertFile[6] = '\0';

    LOG_FUNC("[%s][wifi][Firmware] Assert at \"%s\" #%ld\n\n", NIC_NAME,
             aucAssertFile, line);
}

void halPrintMailbox(IN P_ADAPTER_T prAdapter)
{
    u32 u4MailBoxStatus0, u4MailBoxStatus1;

    halGetMailbox(prAdapter, 0, &u4MailBoxStatus0);
    halGetMailbox(prAdapter, 1, &u4MailBoxStatus1);
    DBGLOG(INIT, INFO, "MailBox Status = 0x%08X, 0x%08X\n",
           u4MailBoxStatus0, u4MailBoxStatus1);
}

void halProcessSoftwareInterrupt(IN P_ADAPTER_T prAdapter)
{
    u32 u4IntrBits;

    ASSERT(prAdapter);

    u4IntrBits = prAdapter->u4IntStatus & BITS(8, 31);

    if ((u4IntrBits & WHISR_D2H_SW_ASSERT_INFO_INT) != 0) {
        halPrintFirmwareAssertInfo(prAdapter);
    }

    if (u4IntrBits & WHISR_D2H_WKUP_BY_RX_PACKET) {
        DBGLOG(RX, INFO, "Wake up by Rx\n");
    }

    if (u4IntrBits & WHISR_D2H_SW_RD_MAILBOX_INT) {
        halPrintMailbox(prAdapter);
    }

    if (u4IntrBits & SER_SDIO_N9_HOST_STOP_TX_OP) {
        halPrintMailbox(prAdapter);
        /* Stop HIF Tx operation */
        nicSerStopTx(prAdapter);
    }

    if (u4IntrBits & SER_SDIO_N9_HOST_STOP_TX_RX_OP) {
        halPrintMailbox(prAdapter);
        /* Stop HIF Tx/Rx operation */
        nicSerStopTxRx(prAdapter);
    }

    if ((u4IntrBits & ~WHISR_D2H_WKUP_BY_RX_PACKET) != 0) {
        DBGLOG(SW4, WARN, "u4IntrBits: 0x%lx\n", u4IntrBits);
    }
}

void halPutMailbox(IN P_ADAPTER_T prAdapter, IN u32 u4MailboxNum,
                   IN u32 u4Data)
{
    switch (u4MailboxNum) {
    case 0:
        HAL_MCR_WR(prAdapter, MCR_H2DSM0R, u4Data);
        break;

    case 1:
        HAL_MCR_WR(prAdapter, MCR_H2DSM1R, u4Data);
        break;

    default:
        ASSERT(0);
    }
}

void halGetMailbox(IN P_ADAPTER_T prAdapter, IN u32 u4MailboxNum,
                   OUT u32 *pu4Data)
{
    switch (u4MailboxNum) {
    case 0:
        HAL_MCR_RD(prAdapter, MCR_D2HRM0R, pu4Data);
        break;

    case 1:
        HAL_MCR_RD(prAdapter, MCR_D2HRM1R, pu4Data);
        break;

    default:
        ASSERT(0);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief process one prRxBuf. If there is not enough free SW_RFB, queue prRxBuf
 * back to rRxDeAggQue and schedule work again.
 *
 * @param prAdapter pointer to the Adapter handler, prRxBuf received buffer
 *
 * @return True if reschedule otherwise False
 */
/*----------------------------------------------------------------------------*/
u8 halDeAggRxPktProc(P_ADAPTER_T prAdapter, P_SDIO_RX_COALESCING_BUF_T prRxBuf)
{
    P_GL_HIF_INFO_T prHifInfo;
    P_RX_CTRL_T prRxCtrl;
    P_SW_RFB_T prSwRfb = (P_SW_RFB_T)NULL;
    u8 *pucSrcAddr;
    u16 u2PktLength;
    u32 i, u4Offset;
    u8 fgReschedule = false;

    QUE_T rTempFreeRfbList, rTempRxRfbList;
    P_QUE_T prTempFreeRfbList = &rTempFreeRfbList;
    P_QUE_T prTempRxRfbList = &rTempRxRfbList;

    KAL_SPIN_LOCK_DECLARATION();
    SDIO_TIME_INTERVAL_DEC();

    prRxCtrl = &prAdapter->rRxCtrl;
    prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

    QUEUE_INITIALIZE(prTempFreeRfbList);
    QUEUE_INITIALIZE(prTempRxRfbList);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
    if (prRxCtrl->rFreeSwRfbList.u4NumElem < prRxBuf->u4PktCount) {
        fgReschedule = true;
    } else {
        /* Get enough free SW_RFB to be Rx */
        for (i = 0; i < prRxBuf->u4PktCount; i++) {
            QUEUE_REMOVE_HEAD(&prRxCtrl->rFreeSwRfbList, prSwRfb,
                              P_SW_RFB_T);
            QUEUE_INSERT_TAIL(prTempFreeRfbList,
                              &prSwRfb->rQueEntry);
        }
        fgReschedule = false;
    }
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

    if (fgReschedule) {
        mutex_lock(&prHifInfo->rRxDeAggQueMutex);
        QUEUE_INSERT_HEAD(&prHifInfo->rRxDeAggQueue,
                          (P_QUE_ENTRY_T)prRxBuf);
        mutex_unlock(&prHifInfo->rRxDeAggQueMutex);

        /* Reschedule this work */
        if ((prAdapter->prGlueInfo->ulFlag & GLUE_FLAG_HALT) == 0) {
            schedule_delayed_work(
                &prAdapter->prGlueInfo->rRxPktDeAggWork, 0);
        }

        return fgReschedule;
    }

    pucSrcAddr = prRxBuf->pvRxCoalescingBuf;
    u4Offset = 0;
    SDIO_REC_TIME_START();
    for (i = 0; i < prRxBuf->u4PktCount; i++) {
        u2PktLength = HAL_RX_STATUS_GET_RX_BYTE_CNT(
            (P_HW_MAC_RX_DESC_T)pucSrcAddr);

        QUEUE_REMOVE_HEAD(prTempFreeRfbList, prSwRfb, P_SW_RFB_T);
        if (ALIGN_4(u2PktLength + HIF_RX_HW_APPENDED_LEN) >
            CFG_RX_MAX_PKT_SIZE) {
            DBGLOG(RX,
                   ERROR,
                   "%s: coalescing packet length(%d) too large, will overflow swRfb.\n",
                   __func__,
                   u2PktLength);
            break;
        }
        u4Offset += ALIGN_4(u2PktLength + HIF_RX_HW_APPENDED_LEN);
        if (u4Offset > HIF_RX_COALESCING_BUFFER_SIZE) {
            DBGLOG(RX,
                   ERROR,
                   "%s: coalescing buffer read: offset(%u), out of bound\n",
                   __func__,
                   u4Offset);
            break;
        }
        kalMemCopy(prSwRfb->pucRecvBuff, pucSrcAddr,
                   ALIGN_4(u2PktLength + HIF_RX_HW_APPENDED_LEN));

        prSwRfb->ucPacketType =
            (u8)HAL_RX_STATUS_GET_PKT_TYPE(prSwRfb->prRxStatus);

        QUEUE_INSERT_TAIL(prTempRxRfbList, &prSwRfb->rQueEntry);

        pucSrcAddr += ALIGN_4(u2PktLength + HIF_RX_HW_APPENDED_LEN);
    }
    SDIO_REC_TIME_END();
    SDIO_ADD_TIME_INTERVAL(prHifInfo->rStatCounter.u4RxDataCpTime);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
    RX_ADD_CNT(prRxCtrl, RX_MPDU_TOTAL_COUNT, prTempRxRfbList->u4NumElem);
    QUEUE_CONCATENATE_QUEUES(&prRxCtrl->rReceivedRfbList, prTempRxRfbList);
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);

    /* Wake up Rx handling thread */
    set_bit(GLUE_FLAG_RX_BIT, &(prAdapter->prGlueInfo->ulFlag));
    wake_up_interruptible(&(prAdapter->prGlueInfo->waitq));

    if (prTempFreeRfbList->u4NumElem) {
        KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
        QUEUE_CONCATENATE_QUEUES(&prRxCtrl->rFreeSwRfbList,
                                 prTempFreeRfbList);
        KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
    }

    prRxBuf->u4PktCount = 0;
    mutex_lock(&prHifInfo->rRxFreeBufQueMutex);
    QUEUE_INSERT_TAIL(&prHifInfo->rRxFreeBufQueue, (P_QUE_ENTRY_T)prRxBuf);
    mutex_unlock(&prHifInfo->rRxFreeBufQueMutex);

    return fgReschedule;
}

void halDeAggRxPktWorker(struct work_struct *work)
{
    P_GLUE_INFO_T prGlueInfo;
    P_GL_HIF_INFO_T prHifInfo;
    P_ADAPTER_T prAdapter;
    P_SDIO_RX_COALESCING_BUF_T prRxBuf;
    P_RX_CTRL_T prRxCtrl;
    u8 bRescheduled = false;

    if (g_u4HaltFlag) {
        return;
    }

    prGlueInfo = ENTRY_OF(work, GLUE_INFO_T, rRxPktDeAggWork);
    prHifInfo = &prGlueInfo->rHifInfo;
    prAdapter = prGlueInfo->prAdapter;

    if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
        return;
    }

    prRxCtrl = &prAdapter->rRxCtrl;
    prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

    mutex_lock(&prHifInfo->rRxDeAggQueMutex);
    QUEUE_REMOVE_HEAD(&prHifInfo->rRxDeAggQueue, prRxBuf,
                      P_SDIO_RX_COALESCING_BUF_T);
    mutex_unlock(&prHifInfo->rRxDeAggQueMutex);

    while (prRxBuf) {
        bRescheduled = halDeAggRxPktProc(prAdapter, prRxBuf);

        if (bRescheduled) {
            DBGLOG(RX, WARN,
                   "halDeAggRxPktProc return rescheduled\n");
            return;
        }

        if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
            return;
        }

        mutex_lock(&prHifInfo->rRxDeAggQueMutex);
        QUEUE_REMOVE_HEAD(&prHifInfo->rRxDeAggQueue, prRxBuf,
                          P_SDIO_RX_COALESCING_BUF_T);
        mutex_unlock(&prHifInfo->rRxDeAggQueMutex);
    }
}

void halDeAggRxPkt(P_ADAPTER_T prAdapter, P_SDIO_RX_COALESCING_BUF_T prRxBuf)
{
    P_GL_HIF_INFO_T prHifInfo;
    prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

    /* Avoid to schedule DeAggWorker during uninit flow */
    if (prAdapter->prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
        mutex_lock(&prHifInfo->rRxFreeBufQueMutex);
        QUEUE_INSERT_TAIL(&prHifInfo->rRxFreeBufQueue,
                          (P_QUE_ENTRY_T)prRxBuf);
        mutex_unlock(&prHifInfo->rRxFreeBufQueMutex);

        return;
    }

#if CFG_SDIO_RX_AGG_TASKLET
    mutex_lock(&prHifInfo->rRxDeAggQueMutex);
    QUEUE_INSERT_TAIL(&prHifInfo->rRxDeAggQueue, (P_QUE_ENTRY_T)prRxBuf);
    mutex_unlock(&prHifInfo->rRxDeAggQueMutex);

    schedule_delayed_work(&prAdapter->prGlueInfo->rRxPktDeAggWork, 0);
#else
    if (QUEUE_IS_NOT_EMPTY(&prHifInfo->rRxDeAggQueue)) {
        mutex_lock(&prHifInfo->rRxDeAggQueMutex);
        QUEUE_INSERT_TAIL(&prHifInfo->rRxDeAggQueue,
                          (P_QUE_ENTRY_T)prRxBuf);
        QUEUE_REMOVE_HEAD(&prHifInfo->rRxDeAggQueue, prRxBuf,
                          P_SDIO_RX_COALESCING_BUF_T);
        mutex_unlock(&prHifInfo->rRxDeAggQueMutex);
    }

    halDeAggRxPktProc(prAdapter, prRxBuf);
#endif
}

/* Hif power off wifi */
WLAN_STATUS halHifPowerOffWifi(IN P_ADAPTER_T prAdapter)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;

    if (prAdapter->rAcpiState == ACPI_STATE_D0 &&
        !wlanIsChipNoAck(prAdapter) &&
        !kalIsCardRemoved(prAdapter->prGlueInfo)) {
        /* 0. Disable interrupt, this can be done without Driver own */
        nicDisableInterrupt(prAdapter);

        ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);

        /* 1. Set CMD to FW to tell WIFI to stop (enter power off state)
         */
        if (prAdapter->fgIsFwOwn == false &&
            wlanSendNicPowerCtrlCmd(prAdapter, 1) ==
            WLAN_STATUS_SUCCESS) {
            u32 i;
            /* 2. Clear pending interrupt */
            i = 0;
            while (i < CFG_IST_LOOP_COUNT &&
                   nicProcessIST(prAdapter) !=
                   WLAN_STATUS_NOT_INDICATING)
                i++;
            ;

            /* 3. Wait til RDY bit has been cleaerd */
            rStatus = wlanCheckWifiFunc(prAdapter, false);
        }
#if !CFG_ENABLE_FULL_PM
        /* 4. Set Onwership to F/W */
        nicpmSetFWOwn(prAdapter, false);
#endif

#if CFG_FORCE_RESET_UNDER_BUS_ERROR
        if (HAL_TEST_FLAG(prAdapter, ADAPTER_FLAG_HW_ERR) == true) {
            /* force acquire firmware own */
            kalDevRegWrite(prAdapter->prGlueInfo, MCR_WHLPCR,
                           WHLPCR_FW_OWN_REQ_CLR);

            /* delay for 10ms */
            kalMdelay(10);

            /* force firmware reset via software interrupt */
            kalDevRegWrite(prAdapter->prGlueInfo, MCR_WSICR,
                           WSICR_H2D_SW_INT_SET);

            /* force release firmware own */
            kalDevRegWrite(prAdapter->prGlueInfo, MCR_WHLPCR,
                           WHLPCR_FW_OWN_REQ_SET);
        }
#endif

        RECLAIM_POWER_CONTROL_TO_PM(prAdapter, false);
    }
    return rStatus;
}

void halPollDbgCr(IN P_ADAPTER_T prAdapter, IN u32 u4LoopCount)
{
    u32 u4Data = 0;
    u32 u4Loop = 0;

    for (u4Loop = 0; u4Loop < u4LoopCount; u4Loop++) {
        HAL_MCR_RD(prAdapter, MCR_SWPCDBGR, &u4Data);
        DBGLOG(INIT, INFO, "SWPCDBGR 0x%08X\n", u4Data);
    }
}

void halSerHifReset(IN P_ADAPTER_T prAdapter)
{
    P_GL_HIF_INFO_T prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
    u32 i;

    KAL_SPIN_LOCK_DECLARATION();

    /* Restore Tx resource */
    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
    for (i = TC0_INDEX; i < TC_NUM; i++) {
#else
    for (i = TC0_INDEX; i <= TC5_INDEX; i++) {
#endif
        nicTxReleaseResource(prAdapter, i,
                             prHifInfo->au4PendingTxDoneCount[i],
                             false);
        prHifInfo->au4PendingTxDoneCount[i] = 0;
    }
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

    /* Clear interrupt status from Rx interrupt enhance mode */
    prHifInfo->fgIsPendingInt = false;
    kalMemZero(prHifInfo->prSDIOCtrl, sizeof(ENHANCE_MODE_DATA_STRUCT_T));
}

void halPrintHifDbgInfo(IN P_ADAPTER_T prAdapter)
{
    halPrintMailbox(prAdapter);
    halPollDbgCr(prAdapter, LP_OWN_BACK_FAILED_DBGCR_POLL_ROUND);
}

u8 halIsTxResourceControlEn(IN P_ADAPTER_T prAdapter)
{
    return true;
}

void halTxResourceResetHwTQCounter(IN P_ADAPTER_T prAdapter)
{
    u32 *pu4WHISR = NULL;
    u16 au2TxCount[16];

    pu4WHISR = (u32 *)kalMemAlloc(sizeof(u32), PHY_MEM_TYPE);
    if (!pu4WHISR) {
        DBGLOG(INIT, ERROR, "Allocate pu4WHISR fail\n");
        return;
    }

    HAL_READ_INTR_STATUS(prAdapter, sizeof(u32), (u8 *)pu4WHISR);
    if (HAL_IS_TX_DONE_INTR(*pu4WHISR)) {
        HAL_READ_TX_RELEASED_COUNT(prAdapter, au2TxCount);
    }

    if (pu4WHISR) {
        kalMemFree(pu4WHISR, PHY_MEM_TYPE, sizeof(u32));
    }
}
