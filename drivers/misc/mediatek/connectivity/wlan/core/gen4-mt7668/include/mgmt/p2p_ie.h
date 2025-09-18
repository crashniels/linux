/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */
#ifndef _P2P_IE_H
#define _P2P_IE_H

#if CFG_SUPPORT_WFD

#define ELEM_MAX_LEN_WFD               62  /* TODO: Move to appropriate place */

/*---------------- WFD Data Element Definitions ----------------*/
/* WFD 4.1.1 - WFD IE format */
#define WFD_OUI_TYPE_LEN               4

/* == OFFSET_OF(IE_P2P_T,*/
/*aucP2PAttributes[0]) */
#define WFD_IE_OUI_HDR                 (ELEM_HDR_LEN + WFD_OUI_TYPE_LEN)

/* WFD 4.1.1 - General WFD Attribute */
#define WFD_ATTRI_HDR_LEN              3  /* ID(1 octet) + Length(2 octets) */

/* WFD Attribute Code */
#define WFD_ATTRI_ID_DEV_INFO              0
#define WFD_ATTRI_ID_ASSOC_BSSID           1
#define WFD_ATTRI_ID_COUPLED_SINK_INFO         6
#define WFD_ATTRI_ID_EXT_CAPABILITY        7
#define WFD_ATTRI_ID_SESSION_INFO          9
#define WFD_ATTRI_ID_ALTER_MAC_ADDRESS         10

/* Maximum Length of WFD Attributes */
#define WFD_ATTRI_MAX_LEN_DEV_INFO         6  /* 0 */
#define WFD_ATTRI_MAX_LEN_ASSOC_BSSID          6  /* 1 */
#define WFD_ATTRI_MAX_LEN_COUPLED_SINK_INFO    7  /* 6 */
#define WFD_ATTRI_MAX_LEN_EXT_CAPABILITY       2  /* 7 */
#define WFD_ATTRI_MAX_LEN_SESSION_INFO         0  /* 9 */ /* 24 * #Clients */
#define WFD_ATTRI_MAX_LEN_ALTER_MAC_ADDRESS    6  /* 10 */

typedef struct _WFD_DEVICE_INFORMATION_IE_T {
    u8 ucElemID;
    u16 u2Length;
    u16 u2WfdDevInfo;
    u16 u2SessionMgmtCtrlPort;
    u16 u2WfdDevMaxSpeed;
} __KAL_ATTRIB_PACKED__ WFD_DEVICE_INFORMATION_IE_T,
*P_WFD_DEVICE_INFORMATION_IE_T;

#endif

u32 p2pCalculate_IEForAssocReq(IN P_ADAPTER_T prAdapter,
                               IN u8 ucBssIndex,
                               IN P_STA_RECORD_T prStaRec);

void p2pGenerate_IEForAssocReq(IN P_ADAPTER_T prAdapter,
                               IN P_MSDU_INFO_T prMsduInfo);

#endif
