/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "cmd_buf.h"
 *   \brief  In this file we define the structure for Command Packet.
 *
 * In this file we define the structure for Command Packet and the control unit
 *   of MGMT Memory Pool.
 */

#ifndef _CMD_BUF_H
#define _CMD_BUF_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

typedef enum _COMMAND_TYPE {
    COMMAND_TYPE_GENERAL_IOCTL,
    COMMAND_TYPE_NETWORK_IOCTL,
    COMMAND_TYPE_SECURITY_FRAME,
    COMMAND_TYPE_MANAGEMENT_FRAME,
    COMMAND_TYPE_NUM
} COMMAND_TYPE,
*P_COMMAND_TYPE;

typedef void (*PFN_CMD_DONE_HANDLER)(IN P_ADAPTER_T prAdapter,
                                     IN P_CMD_INFO_T prCmdInfo,
                                     IN u8 *pucEventBuf,
                                     IN u32 u4EventBufLen);

typedef void (*PFN_CMD_TIMEOUT_HANDLER)(IN P_ADAPTER_T prAdapter,
                                        IN P_CMD_INFO_T prCmdInfo);

typedef void (*PFN_HIF_TX_CMD_DONE_CB)(IN P_ADAPTER_T prAdapter,
                                       IN P_CMD_INFO_T prCmdInfo);

struct _CMD_INFO_T {
    QUE_ENTRY_T rQueEntry;

    COMMAND_TYPE eCmdType;

    u16 u2InfoBufLen;  /* This is actual CMD buffer length */
    u8 *pucInfoBuffer;  /* May pointer to structure in prAdapter */
    P_MSDU_INFO_T prMsduInfo;  /* only valid when it's a security/MGMT frame
                                */
    P_NATIVE_PACKET prPacket;  /* only valid when it's a security frame */

    PFN_CMD_DONE_HANDLER pfCmdDoneHandler;
    PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler;
    PFN_HIF_TX_CMD_DONE_CB pfHifTxCmdDoneCb;

    u8 fgIsOid;  /* Used to check if we need indicate */

    u8 ucCID;
    u8 fgSetQuery;
    u8 fgNeedResp;
    u8 ucCmdSeqNum;
    u32 u4SetInfoLen;  /* Indicate how many byte we read for Set OID */

    /* information indicating by OID/ioctl */
    void *pvInformationBuffer;
    u32 u4InformationBufferLength;

    /* private data */
    u32 u4PrivateData;

    /* TXD/TXP pointer/len for hif tx copy */
    u32 u4TxdLen;
    u32 u4TxpLen;
    u8 *pucTxd;
    u8 *pucTxp;
};

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

void cmdBufInitialize(IN P_ADAPTER_T prAdapter);

P_CMD_INFO_T cmdBufAllocateCmdInfo(IN P_ADAPTER_T prAdapter, IN u32 u4Length);

void cmdBufFreeCmdInfo(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo);

/*----------------------------------------------------------------------------*/
/* Routines for CMDs                                                          */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanSendSetQueryCmd(IN P_ADAPTER_T prAdapter,
                    u8 ucCID,
                    u8 fgSetQuery,
                    u8 fgNeedResp,
                    u8 fgIsOid,
                    PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
                    PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
                    u32 u4SetQueryInfoLen,
                    u8 *pucInfoBuffer,
                    OUT void *pvSetQueryBuffer,
                    IN u32 u4SetQueryBufferLen);

#if CFG_SUPPORT_TX_BF
WLAN_STATUS
wlanSendSetQueryExtCmd(IN P_ADAPTER_T prAdapter,
                       u8 ucCID,
                       u8 ucExtCID,
                       u8 fgSetQuery,
                       u8 fgNeedResp,
                       u8 fgIsOid,
                       PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
                       PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
                       u32 u4SetQueryInfoLen,
                       u8 *pucInfoBuffer,
                       OUT void *pvSetQueryBuffer,
                       IN u32 u4SetQueryBufferLen);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
