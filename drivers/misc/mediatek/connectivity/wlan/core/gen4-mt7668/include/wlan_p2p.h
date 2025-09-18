/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "wlan_p2p.h"
 *    \brief This file contains the declairations of Wi-Fi Direct command
 *    processing routines for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */

#ifndef _WLAN_P2P_H
#define _WLAN_P2P_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#if CFG_ENABLE_WIFI_DIRECT
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/* Service Discovery */
typedef struct _PARAM_P2P_SEND_SD_RESPONSE {
    PARAM_MAC_ADDRESS rReceiverAddr;
    u8 fgNeedTxDoneIndication;
    u8 ucChannelNum;
    u16 u2PacketLength;
    u8 aucPacketContent[0];  /*native 802.11 */
} PARAM_P2P_SEND_SD_RESPONSE, *P_PARAM_P2P_SEND_SD_RESPONSE;

typedef struct _PARAM_P2P_GET_SD_REQUEST {
    PARAM_MAC_ADDRESS rTransmitterAddr;
    u16 u2PacketLength;
    u8 aucPacketContent[0];  /*native 802.11 */
} PARAM_P2P_GET_SD_REQUEST, *P_PARAM_P2P_GET_SD_REQUEST;

typedef struct _PARAM_P2P_GET_SD_REQUEST_EX {
    PARAM_MAC_ADDRESS rTransmitterAddr;
    u16 u2PacketLength;
    u8 ucChannelNum;  /* Channel Number Where SD Request is received. */
    u8 ucSeqNum;  /* Get SD Request by sequence number. */
    u8 aucPacketContent[0];  /*native 802.11 */
} PARAM_P2P_GET_SD_REQUEST_EX, *P_PARAM_P2P_GET_SD_REQUEST_EX;

typedef struct _PARAM_P2P_SEND_SD_REQUEST {
    PARAM_MAC_ADDRESS rReceiverAddr;
    u8 fgNeedTxDoneIndication;
    u8 ucVersionNum;  /* Indicate the Service Discovery Supplicant Version.
                       */
    u16 u2PacketLength;
    u8 aucPacketContent[0];  /*native 802.11 */
} PARAM_P2P_SEND_SD_REQUEST, *P_PARAM_P2P_SEND_SD_REQUEST;

/* Service Discovery 1.0. */
typedef struct _PARAM_P2P_GET_SD_RESPONSE {
    PARAM_MAC_ADDRESS rTransmitterAddr;
    u16 u2PacketLength;
    u8 aucPacketContent[0];  /*native 802.11 */
} PARAM_P2P_GET_SD_RESPONSE, *P_PARAM_P2P_GET_SD_RESPONSE;

/* Service Discovery 2.0. */
typedef struct _PARAM_P2P_GET_SD_RESPONSE_EX {
    PARAM_MAC_ADDRESS rTransmitterAddr;
    u16 u2PacketLength;
    u8 ucSeqNum;  /* Get SD Response by sequence number. */
    u8 aucPacketContent[0];  /*native 802.11 */
} PARAM_P2P_GET_SD_RESPONSE_EX, *P_PARAM_P2P_GET_SD_RESPONSE_EX;

typedef struct _PARAM_P2P_TERMINATE_SD_PHASE {
    PARAM_MAC_ADDRESS rPeerAddr;
} PARAM_P2P_TERMINATE_SD_PHASE, *P_PARAM_P2P_TERMINATE_SD_PHASE;

/*! \brief Key mapping of BSSID */
typedef struct _P2P_PARAM_KEY_T {
    u32 u4Length;  /*!< Length of structure */
    u32 u4KeyIndex;  /*!< KeyID */
    u32 u4KeyLength;  /*!< Key length in bytes */
    PARAM_MAC_ADDRESS arBSSID;  /*!< MAC address */
    PARAM_KEY_RSC rKeyRSC;
    /* Following add to change the original windows structure */
    u8 ucBssIdx;  /* for specific P2P BSS interface. */
    u8 ucCipher;
    u8 aucKeyMaterial[32];  /*!< Key content by above setting */
} P2P_PARAM_KEY_T, *P_P2P_PARAM_KEY_T;

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

/*--------------------------------------------------------------*/
/* Routines to handle command                                   */
/*--------------------------------------------------------------*/
/* WLAN_STATUS */
/* wlanoidSetAddP2PKey(IN P_ADAPTER_T prAdapter, */
/* IN void * pvSetBuffer, IN u32 u4SetBufferLen, OUT u32 * pu4SetInfoLen); */

/* WLAN_STATUS */
/* wlanoidSetRemoveP2PKey(IN P_ADAPTER_T prAdapter, */
/* IN void * pvSetBuffer, IN u32 u4SetBufferLen, OUT u32 * pu4SetInfoLen); */

WLAN_STATUS
wlanoidSetNetworkAddress(IN P_ADAPTER_T prAdapter,
                         IN void *pvSetBuffer,
                         IN u32 u4SetBufferLen,
                         OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetP2PMulticastList(IN P_ADAPTER_T prAdapter,
                           IN void *pvSetBuffer,
                           IN u32 u4SetBufferLen,
                           OUT u32 *pu4SetInfoLen);

/*--------------------------------------------------------------*/
/* Service Discovery Subroutines                                */
/*--------------------------------------------------------------*/
WLAN_STATUS
wlanoidSendP2PSDRequest(IN P_ADAPTER_T prAdapter,
                        IN void *pvSetBuffer,
                        IN u32 u4SetBufferLen,
                        OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSendP2PSDResponse(IN P_ADAPTER_T prAdapter,
                         IN void *pvSetBuffer,
                         IN u32 u4SetBufferLen,
                         OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidGetP2PSDRequest(IN P_ADAPTER_T prAdapter,
                       IN void *pvSetBuffer,
                       IN u32 u4SetBufferLen,
                       OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidGetP2PSDResponse(IN P_ADAPTER_T prAdapter,
                        IN void *pvQueryBuffer,
                        IN u32 u4QueryBufferLen,
                        OUT u32 *puQueryInfoLen);

WLAN_STATUS
wlanoidSetP2PTerminateSDPhase(IN P_ADAPTER_T prAdapter,
                              IN void *pvQueryBuffer,
                              IN u32 u4QueryBufferLen,
                              OUT u32 *pu4QueryInfoLen);

#if CFG_SUPPORT_ANTI_PIRACY
WLAN_STATUS
wlanoidSetSecCheckRequest(IN P_ADAPTER_T prAdapter,
                          IN void *pvSetBuffer,
                          IN u32 u4SetBufferLen,
                          OUT u32 *pu4SetInfoLen);

/*WLAN_STATUS
 * wlanoidGetSecCheckResponse(IN P_ADAPTER_T prAdapter,
 *    IN void * pvQueryBuffer, IN u32 u4QueryBufferLen, OUT u32 *
 * pu4QueryInfoLen);
 */
#endif

WLAN_STATUS
wlanoidSetNoaParam(IN P_ADAPTER_T prAdapter,
                   IN void *pvSetBuffer,
                   IN u32 u4SetBufferLen,
                   OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetOppPsParam(IN P_ADAPTER_T prAdapter,
                     IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen,
                     OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetUApsdParam(IN P_ADAPTER_T prAdapter,
                     IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen,
                     OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryP2pPowerSaveProfile(IN P_ADAPTER_T prAdapter,
                                IN void *pvQueryBuffer,
                                IN u32 u4QueryBufferLen,
                                OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetP2pPowerSaveProfile(IN P_ADAPTER_T prAdapter,
                              IN void *pvSetBuffer,
                              IN u32 u4SetBufferLen,
                              OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetP2pSetNetworkAddress(IN P_ADAPTER_T prAdapter,
                               IN void *pvSetBuffer,
                               IN u32 u4SetBufferLen,
                               OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryP2pVersion(IN P_ADAPTER_T prAdapter,
                       IN void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen,
                       OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetP2pSupplicantVersion(IN P_ADAPTER_T prAdapter,
                               IN void *pvSetBuffer,
                               IN u32 u4SetBufferLen,
                               OUT u32 *pu4SetInfoLen);

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
WLAN_STATUS
wlanoidSetP2pWPSmode(IN P_ADAPTER_T prAdapter,
                     IN void *pvQueryBuffer,
                     IN u32 u4QueryBufferLen,
                     OUT u32 *pu4QueryInfoLen);
#endif

#if CFG_SUPPORT_P2P_RSSI_QUERY
WLAN_STATUS
wlanoidQueryP2pRssi(IN P_ADAPTER_T prAdapter,
                    IN void *pvQueryBuffer,
                    IN u32 u4QueryBufferLen,
                    OUT u32 *pu4QueryInfoLen);
#endif

u32 wlanoidAbortP2pScan(IN P_ADAPTER_T prAdapter,
                        OUT void *pvQueryBuffer,
                        IN u32 u4QueryBufferLen,
                        OUT u32 *pu4QueryInfoLen);

/*--------------------------------------------------------------*/
/* Callbacks for event indication                               */
/*--------------------------------------------------------------*/

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
#endif
