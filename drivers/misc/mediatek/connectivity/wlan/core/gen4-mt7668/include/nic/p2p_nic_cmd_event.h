/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   p2p_nic_cmd_event.h
 *  \brief
 */

#ifndef _P2P_NIC_CMD_EVENT_H
#define _P2P_NIC_CMD_EVENT_H

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
 *                                 M A C R O S
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
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

typedef struct _EVENT_P2P_DEV_DISCOVER_RESULT_T {
    /* u8                          aucCommunicateAddr[MAC_ADDR_LEN];  //
     * Deprecated. */
    u8 aucDeviceAddr[MAC_ADDR_LEN];  /* Device Address. */
    u8 aucInterfaceAddr[MAC_ADDR_LEN];  /* Device Address. */
    u8 ucDeviceCapabilityBitmap;
    u8 ucGroupCapabilityBitmap;
    u16 u2ConfigMethod;  /* Configure Method. */
    P2P_DEVICE_TYPE_T rPriDevType;
    u8 ucSecDevTypeNum;
    P2P_DEVICE_TYPE_T arSecDevType[2];
    u16 u2NameLength;
    u8 aucName[32];
    u8 *pucIeBuf;
    u16 u2IELength;
    u8 aucBSSID[MAC_ADDR_LEN];
    /* TODO: Service Information or PasswordID valid? */
} EVENT_P2P_DEV_DISCOVER_RESULT_T, *P_EVENT_P2P_DEV_DISCOVER_RESULT_T;

#endif
