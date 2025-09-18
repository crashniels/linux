/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "hif.h"
 *    \brief  Functions for the driver to register bus and setup the IRQ
 *
 *    Functions for the driver to register bus and setup the IRQ
 */

#ifndef _HIF_H
#define _HIF_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

#define HIF_SDIO_SUPPORT_GPIO_SLEEP_MODE      0

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define HIF_NAME                  "SDIO"

/* Enable driver timing profiling */
#define CFG_SDIO_TIMING_PROFILING         0

#define SDIO_X86_WORKAROUND_WRITE_MCR         0x00C4
#define HIF_NUM_OF_QM_RX_PKT_NUM          512

#define HIF_TX_INIT_CMD_PORT              TX_RING_FWDL_IDX_3

#define HIF_IST_LOOP_COUNT            128
#define HIF_IST_TX_THRESHOLD \
    32  /* Min msdu count to trigger Tx during INT polling state */

#define HIF_TX_MAX_AGG_LENGTH             (511 * 512)  /* 511 blocks x 512 */

#define HIF_RX_MAX_AGG_NUM            16
/*!< Setting the maximum RX aggregation number 0: no limited (16) */

#define HIF_TX_BUFF_COUNT_TC0             8
#define HIF_TX_BUFF_COUNT_TC1             167
#define HIF_TX_BUFF_COUNT_TC2             8
#define HIF_TX_BUFF_COUNT_TC3             8
#define HIF_TX_BUFF_COUNT_TC4             7
#define HIF_TX_BUFF_COUNT_TC5             0

#define HIF_TX_RESOURCE_CTRL              1  /* enable/disable TX resource control */

#define HIF_TX_PAGE_SIZE_IN_POWER_OF_2        11
#define HIF_TX_PAGE_SIZE              2048  /* in unit of bytes */

#define HIF_EXTRA_IO_BUFFER_SIZE          \
    (sizeof(ENHANCE_MODE_DATA_STRUCT_T) + \
     HIF_RX_COALESCING_BUF_COUNT * HIF_RX_COALESCING_BUFFER_SIZE)

#define HIF_CR4_FWDL_SECTION_NUM          2
#define HIF_IMG_DL_STATUS_PORT_IDX        0

#define HIF_RX_ENHANCE_MODE_PAD_LEN       4

#define HIF_TX_TERMINATOR_LEN             4

#if CFG_SDIO_TX_AGG
#define HIF_TX_COALESCING_BUFFER_SIZE         (HIF_TX_MAX_AGG_LENGTH)
#else
#define HIF_TX_COALESCING_BUFFER_SIZE         (CFG_TX_MAX_PKT_SIZE)
#endif

#if CFG_SDIO_RX_AGG
#define HIF_RX_COALESCING_BUFFER_SIZE \
    ((HIF_RX_MAX_AGG_NUM + 1) * CFG_RX_MAX_PKT_SIZE)
#else
#define HIF_RX_COALESCING_BUFFER_SIZE         (CFG_RX_MAX_PKT_SIZE)
#endif

#if CFG_SDIO_RX_AGG_TASKLET
#define HIF_RX_COALESCING_BUF_COUNT       16
#else
#define HIF_RX_COALESCING_BUF_COUNT       1
#endif

/* WHISR device to host (D2H) */
/* N9 Interrupt Host to stop tx/rx operation (at the moment, HIF tx/rx are
 * stopted) */
#define SER_SDIO_N9_HOST_STOP_TX_RX_OP        BIT(8)
/* N9 Interrupt Host to stop tx operation (at the moment, HIF tx are stopted) */
#define SER_SDIO_N9_HOST_STOP_TX_OP       BIT(9)
/* N9 Interrupt Host all modules were reset done (to let host reinit HIF) */
#define SER_SDIO_N9_HOST_RESET_DONE       BIT(10)
/* N9 Interrupt Host System Error Recovery Done */
#define SER_SDIO_N9_HOST_RECOVERY_DONE        BIT(11)

/* WSICR host to device (H2D) */
/* Host ACK HIF tx/rx ring stop operatio */
#define SER_SDIO_HOST_N9_STOP_TX_RX_OP_ACK    BIT(19)
/* Host interrupt N9 HIF init done */
#define SER_SDIO_HOST_N9_RESET_DONE_ACK       BIT(20)
/* Host interrupt N9 System Error Recovery done */
#define SER_SDIO_HOST_N9_RECOVERY_DONE_ACK    BIT(21)

/* HIF SDIO WMM Tx resource flow control */
#if CFG_TX_WMM_ENHANCE
#define HIF_TX_RSRC_WMM_ENHANCE           1
#else
#define HIF_TX_RSRC_WMM_ENHANCE           0
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

typedef struct _ENHANCE_MODE_DATA_STRUCT_T SDIO_CTRL_T, *P_SDIO_CTRL_T;

typedef struct _SDIO_STAT_COUNTER_T {
    /* Tx data */
    u32 u4DataPortWriteCnt;
    u32 u4DataPktWriteCnt;
    u32 u4DataPortKickCnt;

    /* Tx command */
    u32 u4CmdPortWriteCnt;
    u32 u4CmdPktWriteCnt;

    /* Tx done interrupt */
    u32 u4TxDoneCnt[16];
    u32 u4TxDoneIntCnt[16];
    u32 u4TxDoneIntTotCnt;
    u32 u4TxDonePendingPktCnt;

    u32 u4IntReadCnt;
    u32 u4IntCnt;

    /* Rx data/cmd*/
    u32 u4PortReadCnt[2];
    u32 u4PktReadCnt[2];

    u32 u4RxBufUnderFlowCnt;

#if CFG_SDIO_TIMING_PROFILING
    u32 u4TxDataCpTime;
    u32 u4TxDataFreeTime;

    u32 u4RxDataCpTime;
    u32 u4PortReadTime;

    u32 u4TxDoneIntTime;
    u32 u4IntReadTime;
#endif
} SDIO_STAT_COUNTER_T, *P_SDIO_STAT_COUNTER_T;

typedef struct _SDIO_RX_COALESCING_BUF_T {
    QUE_ENTRY_T rQueEntry;
    void *pvRxCoalescingBuf;
    u32 u4BufSize;
    u32 u4PktCount;
} SDIO_RX_COALESCING_BUF_T, *P_SDIO_RX_COALESCING_BUF_T;

/* host interface's private data structure, which is attached to os glue
** layer info structure.
*/
typedef struct _GL_HIF_INFO_T {
    struct sdio_func *func;

    P_SDIO_CTRL_T prSDIOCtrl;

    u8 fgIntReadClear;
    u8 fgMbxReadClear;
    QUE_T rFreeQueue;
    u8 fgIsPendingInt;

#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
    /*17 = TC_NUM*/
    u32 au4PendingTxDoneCount[17];
#else
    u32 au4PendingTxDoneCount[6];
#endif
    /* Statistic counter */
    SDIO_STAT_COUNTER_T rStatCounter;

    SDIO_RX_COALESCING_BUF_T rRxCoalesingBuf[HIF_RX_COALESCING_BUF_COUNT];

    QUE_T rRxDeAggQueue;
    QUE_T rRxFreeBufQueue;

    struct mutex rRxFreeBufQueMutex;
    struct mutex rRxDeAggQueMutex;
} GL_HIF_INFO_T, *P_GL_HIF_INFO_T;

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

#if CFG_SDIO_TIMING_PROFILING
#define SDIO_TIME_INTERVAL_DEC()         KAL_TIME_INTERVAL_DECLARATION()
#define SDIO_REC_TIME_START()            KAL_REC_TIME_START()
#define SDIO_REC_TIME_END()          KAL_REC_TIME_END()
#define SDIO_GET_TIME_INTERVAL()         KAL_GET_TIME_INTERVAL()
#define SDIO_ADD_TIME_INTERVAL(_Interval)    KAL_ADD_TIME_INTERVAL(_Interval)
#else
#define SDIO_TIME_INTERVAL_DEC()
#define SDIO_REC_TIME_START()
#define SDIO_REC_TIME_END()
#define SDIO_GET_TIME_INTERVAL()
#define SDIO_ADD_TIME_INTERVAL(_Interval)
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

WLAN_STATUS glRegisterBus(probe_card pfProbe, remove_card pfRemove);

void glUnregisterBus(remove_card pfRemove);

void glSetHifInfo(P_GLUE_INFO_T prGlueInfo, unsigned long ulCookie);

void glClearHifInfo(P_GLUE_INFO_T prGlueInfo);

u8 glBusInit(void *pvData);

s32 glBusSetIrq(void *pvData, void *pfnIsr, void *pvCookie);

void glBusFreeIrq(void *pvData, void *pvCookie);

void glGetDev(void *ctx, struct device **dev);

void glGetHifDev(P_GL_HIF_INFO_T prHif, struct device **dev);

u8 glWakeupSdio(P_GLUE_INFO_T prGlueInfo);

#if !CFG_SDIO_INTR_ENHANCE
void halRxSDIOReceiveRFBs(IN P_ADAPTER_T prAdapter);

WLAN_STATUS halRxReadBuffer(IN P_ADAPTER_T prAdapter,
                            IN OUT P_SW_RFB_T prSwRfb);

#else
void halRxSDIOEnhanceReceiveRFBs(IN P_ADAPTER_T prAdapter);

WLAN_STATUS halRxEnhanceReadBuffer(IN P_ADAPTER_T prAdapter,
                                   IN u32 u4DataPort,
                                   IN u16 u2RxLength,
                                   IN OUT P_SW_RFB_T prSwRfb);

void halProcessEnhanceInterruptStatus(IN P_ADAPTER_T prAdapter);

#endif

#if CFG_SDIO_RX_AGG
void halRxSDIOAggReceiveRFBs(IN P_ADAPTER_T prAdapter);
#endif

void halPutMailbox(IN P_ADAPTER_T prAdapter, IN u32 u4MailboxNum,
                   IN u32 u4Data);
void halGetMailbox(IN P_ADAPTER_T prAdapter,
                   IN u32 u4MailboxNum,
                   OUT u32 *pu4Data);
void halDeAggRxPkt(P_ADAPTER_T prAdapter, P_SDIO_RX_COALESCING_BUF_T prRxBuf);
void halPrintMailbox(IN P_ADAPTER_T prAdapter);
void halPollDbgCr(IN P_ADAPTER_T prAdapter, IN u32 u4LoopCount);
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
