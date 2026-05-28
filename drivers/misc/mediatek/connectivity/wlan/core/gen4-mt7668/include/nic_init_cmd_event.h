/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "nic_init_cmd_event.h"
 *    \brief This file contains the declairation file of the WLAN initialization
 * routines for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */

#ifndef _NIC_INIT_CMD_EVENT_H
#define _NIC_INIT_CMD_EVENT_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "gl_typedef.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define INIT_CMD_STATUS_SUCCESS            0
#define INIT_CMD_STATUS_REJECTED_INVALID_PARAMS    1
#define INIT_CMD_STATUS_REJECTED_CRC_ERROR     2
#define INIT_CMD_STATUS_REJECTED_DECRYPT_FAIL      3
#define INIT_CMD_STATUS_UNKNOWN            4

#define EVENT_HDR_WITHOUT_RXD_SIZE           \
    (OFFSET_OF(WIFI_EVENT_T, aucBuffer[0]) - \
     OFFSET_OF(WIFI_EVENT_T, u2PacketLength))

#define INIT_PKT_FT_CMD                0x2
#define INIT_PKT_FT_PDA_FWDL               0x3

#define INIT_CMD_PQ_ID                 (0x8000)
#define INIT_CMD_PACKET_TYPE_ID            (0xA0)

#define INIT_CMD_PDA_PQ_ID             (0xF800)
#define INIT_CMD_PDA_PACKET_TYPE_ID        (0xA0)

#if (CFG_UMAC_GENERATION >= 0x20)
#define TXD_Q_IDX_MCU_RQ0              0
#define TXD_Q_IDX_MCU_RQ1              1
#define TXD_Q_IDX_MCU_RQ2              2
#define TXD_Q_IDX_MCU_RQ3              3

#define TXD_Q_IDX_PDA_FW_DL            0x1E

/* DW0 Bit31 */
#define TXD_P_IDX_LMAC                 0
#define TXD_P_IDX_MCU                  1

/* DW1 Bit 14:13 */
#define TXD_HF_NON_80211_FRAME             0x0
#define TXD_HF_CMD                 0x1
#define TXD_HF_80211_NORMAL            0x2
#define TXD_HF_80211_ENHANCEMENT           0x3

/* DW1 Bit 15 */
#define TXD_FT_OFFSET                  15
#define TXD_FT_SHORT_FORMAT            0x0
#define TXD_FT_LONG_FORMAT             0x1

/* DW1 Bit 16 */
#define TXD_TXDLEN_OFFSET              16
#define TXD_TXDLEN_1PAGE               0x0
#define TXD_TXDLEN_2PAGE               0x1

/* DW1 Bit 25:24 */
#define TXD_PKT_FT_CUT_THROUGH             0x0
#define TXD_PKT_FT_STORE_FORWARD           0X1
#define TXD_PKT_FT_CMD                 0X2
#define TXD_PKT_FT_PDA_FW              0X3
#endif

typedef enum _ENUM_INIT_CMD_ID {
    INIT_CMD_ID_DOWNLOAD_CONFIG = 1,
    INIT_CMD_ID_WIFI_START,
    INIT_CMD_ID_ACCESS_REG,
    INIT_CMD_ID_QUERY_PENDING_ERROR,
    INIT_CMD_ID_PATCH_START,
    INIT_CMD_ID_PATCH_WRITE,
    INIT_CMD_ID_PATCH_FINISH,
    INIT_CMD_ID_PATCH_SEMAPHORE_CONTROL = 0x10,
    INIT_CMD_ID_HIF_LOOPBACK            = 0x20,
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
    INIT_CMD_ID_DECOMPRESSED_WIFI_START = 0xFF,
#endif
    INIT_CMD_ID_NUM
} ENUM_INIT_CMD_ID,
*P_ENUM_INIT_CMD_ID;

typedef enum _ENUM_INIT_EVENT_ID {
    INIT_EVENT_ID_CMD_RESULT = 1,
    INIT_EVENT_ID_ACCESS_REG,
    INIT_EVENT_ID_PENDING_ERROR,
    INIT_EVENT_ID_PATCH_SEMA_CTRL
} ENUM_INIT_EVENT_ID,
*P_ENUM_INIT_EVENT_ID;

typedef enum _ENUM_INIT_PATCH_STATUS {
    PATCH_STATUS_NO_SEMA_NEED_PATCH = 0,  /* no SEMA, need patch */
    PATCH_STATUS_NO_NEED_TO_PATCH,  /* patch is DL & ready */
    PATCH_STATUS_GET_SEMA_NEED_PATCH,  /* get SEMA, need patch */
    PATCH_STATUS_RELEASE_SEMA  /* release SEMA */
} ENUM_INIT_PATCH_STATUS,
*P_ENUM_INIT_PATCH_STATUS;

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

typedef u8 CMD_STATUS;

/* commands */
typedef struct _INIT_WIFI_CMD_T {
    u8 ucCID;
    u8 ucPktTypeID;  /* Must be 0xA0 (CMD Packet) */
    u8 ucReserved;
    u8 ucSeqNum;

    u8 ucD2B0Rev;  /* padding fields, hw may auto modify this field */
    u8 ucExtenCID;  /* Extend CID */
    u8 ucS2DIndex;  /* Index for Src to Dst in CMD usage */
    u8 ucExtCmdOption;  /* Extend CID option */

    u32 au4D3toD7Rev[5];  /* padding fields */

    u8 aucBuffer[0];
} INIT_WIFI_CMD_T, *P_INIT_WIFI_CMD_T;

typedef struct _INIT_HIF_TX_HEADER_T {
    u16 u2TxByteCount;  /* Max value is over 2048 */
    u16 u2PQ_ID;  /* Must be 0x8000 (Port1, Queue 0) */

    u8 ucWlanIdx;
    u8 ucHeaderFormat;
    u8 ucHeaderPadding;
    u8 ucPktFt : 2;
    u8 ucOwnMAC : 6;
    u32 au4D2toD7Rev[6];

    u16 u2Length;
    u16 u2PqId;

    INIT_WIFI_CMD_T rInitWifiCmd;
} INIT_HIF_TX_HEADER_T, *P_INIT_HIF_TX_HEADER_T;

#define DOWNLOAD_CONFIG_ENCRYPTION_MODE       BIT(0)
#define DOWNLOAD_CONFIG_KEY_INDEX_MASK        BITS(1, 2)
#define DOWNLOAD_CONFIG_RESET_OPTION          BIT(3)
#define DOWNLOAD_CONFIG_WORKING_PDA_OPTION    BIT(4)
#define DOWNLOAD_CONFIG_ACK_OPTION        BIT(31)
typedef struct _INIT_CMD_DOWNLOAD_CONFIG {
    u32 u4Address;
    u32 u4Length;
    u32 u4DataMode;
} INIT_CMD_DOWNLOAD_CONFIG, *P_INIT_CMD_DOWNLOAD_CONFIG;

#define START_OVERRIDE_START_ADDRESS          BIT(0)
#define START_DELAY_CALIBRATION           BIT(1)
#define START_WORKING_PDA_OPTION          BIT(2)
#define START_CRC_CHECK               BIT(3)
#define CHANGE_DECOMPRESSION_TMP_ADDRESS      BIT(4)

#if CFG_SUPPORT_COMPRESSION_FW_OPTION
#define WIFI_FW_DECOMPRESSION_FAILED          0xFF
typedef struct _INIT_CMD_WIFI_DECOMPRESSION_START {
    u32 u4Override;
    u32 u4Address;
    u32 u4Region1length;
    u32 u4Region2length;
    u32 u4Region1Address;
    u32 u4Region2Address;
    u32 u4BlockSize;
    u32 u4Region1CRC;
    u32 u4Region2CRC;
    u32 u4DecompressTmpAddress;
} INIT_CMD_WIFI_DECOMPRESSION_START, *P_INIT_CMD_WIFI_DECOMPRESSION_START;
#endif

typedef struct _INIT_CMD_WIFI_START {
    u32 u4Override;
    u32 u4Address;
} INIT_CMD_WIFI_START, *P_INIT_CMD_WIFI_START;

#define PATCH_GET_SEMA_CONTROL        1
#define PATCH_RELEASE_SEMA_CONTROL    0
typedef struct _INIT_CMD_PATCH_SEMA_CONTROL {
    u8 ucGetSemaphore;
    u8 aucReserved[3];
} INIT_CMD_PATCH_SEMA_CONTROL, *P_INIT_CMD_PATCH_SEMA_CONTROL;

typedef struct _INIT_CMD_ACCESS_REG {
    u8 ucSetQuery;
    u8 aucReserved[3];
    u32 u4Address;
    u32 u4Data;
} INIT_CMD_ACCESS_REG, *P_INIT_CMD_ACCESS_REG;

/* Events */
typedef struct _INIT_WIFI_EVENT_T {
    u32 au4HwMacRxDesc[4];

    u16 u2RxByteCount;
    u16 u2PacketType;  /* Must be filled with 0xE000 (EVENT Packet) */
    u8 ucEID;
    u8 ucSeqNum;
    u8 aucReserved[2];

    u8 aucBuffer[0];
} INIT_WIFI_EVENT_T, *P_INIT_WIFI_EVENT_T;

typedef struct _INIT_HIF_RX_HEADER_T {
    INIT_WIFI_EVENT_T rInitWifiEvent;
} INIT_HIF_RX_HEADER_T, *P_INIT_HIF_RX_HEADER_T;

typedef struct _INIT_EVENT_CMD_RESULT {
    u8 ucStatus;  /* 0: success */
    /* 1: rejected by invalid param */
    /* 2: rejected by incorrect CRC */
    /* 3: rejected by decryption failure */
    /* 4: unknown CMD */
    /* 5: timeout */
    u8 aucReserved[3];
} INIT_EVENT_CMD_RESULT, *P_INIT_EVENT_CMD_RESULT, INIT_EVENT_PENDING_ERROR,
*P_INIT_EVENT_PENDING_ERROR;

typedef struct _INIT_EVENT_ACCESS_REG {
    u32 u4Address;
    u32 u4Data;
} INIT_EVENT_ACCESS_REG, *P_INIT_EVENT_ACCESS_REG;

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

#endif
