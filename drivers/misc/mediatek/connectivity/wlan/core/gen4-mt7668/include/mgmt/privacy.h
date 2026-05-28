/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   privacy.h
 *  \brief This file contains the function declaration for privacy.c.
 */

#ifndef _PRIVACY_H
#define _PRIVACY_H

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

#define MAX_KEY_NUM         4
#define WEP_40_LEN          5
#define WEP_104_LEN         13
#define WEP_128_LEN         16
#define LEGACY_KEY_MAX_LEN      16
#define CCMP_KEY_LEN            16
#define TKIP_KEY_LEN            32
#define MAX_KEY_LEN         32
#define MIC_RX_KEY_OFFSET       16
#define MIC_TX_KEY_OFFSET       24
#define MIC_KEY_LEN         8

#define WEP_KEY_ID_FIELD        BITS(0, 29)
#define KEY_ID_FIELD            BITS(0, 7)

#define IS_TRANSMIT_KEY         BIT(31)
#define IS_UNICAST_KEY          BIT(30)
#define IS_AUTHENTICATOR        BIT(28)

#define CIPHER_SUITE_NONE       0
#define CIPHER_SUITE_WEP40      1
#define CIPHER_SUITE_TKIP       2
#define CIPHER_SUITE_TKIP_WO_MIC    3
#define CIPHER_SUITE_CCMP       4
#define CIPHER_SUITE_WEP104     5
#define CIPHER_SUITE_BIP        6
#define CIPHER_SUITE_WEP128     7
#define CIPHER_SUITE_WPI        8
#define CIPHER_SUITE_CCMP_W_CCX     9
#define CIPHER_SUITE_CCMP_256       10
#define CIPHER_SUITE_GCMP_128       11
#define CIPHER_SUITE_GCMP_256       12
#define CIPHER_SUITE_GCM_WPI_128    13

/* Todo:: Move to register */
#if defined(MT6630)
#define WTBL_RESERVED_ENTRY     255
#else
#define WTBL_RESERVED_ENTRY     255
#endif
/* Todo:: By chip capability */
/* Max wlan table size, the max+1 used for probe request,... mgmt frame */
/*sending use basic rate and no security */
#define WTBL_SIZE           32

#define WTBL_ALLOC_FAIL         WTBL_RESERVED_ENTRY
#define WTBL_DEFAULT_ENTRY      0

/*******************************************************************************
 *                         D A T A   T Y P E S
 *******************************************************************************
 */

typedef struct _IEEE_802_1X_HDR {
    u8 ucVersion;
    u8 ucType;
    u16 u2Length;
    /* followed by length octets of data */
} IEEE_802_1X_HDR, *P_IEEE_802_1X_HDR;

typedef struct _EAPOL_KEY {
    u8 ucType;
    /* Note: key_info, key_length, and key_data_length are unaligned */
    u8 aucKeyInfo[2];  /* big endian */
    u8 aucKeyLength[2];  /* big endian */
    u8 aucReplayCounter[8];
    u8 aucKeyNonce[16];
    u8 aucKeyIv[16];
    u8 aucKeyRsc[8];
    u8 aucKeyId[8];  /* Reserved in IEEE 802.11i/RSN */
    u8 aucKeyMic[16];
    u8 aucKeyDataLength[2];  /* big endian */
    /* followed by key_data_length bytes of key_data */
} EAPOL_KEY, *P_EAPOL_KEY;

/* WPA2 PMKID candicate structure */
typedef struct _PMKID_CANDICATE_T {
    u8 aucBssid[MAC_ADDR_LEN];
    u32 u4PreAuthFlags;
} PMKID_CANDICATE_T, *P_PMKID_CANDICATE_T;

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
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void secInit(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex);

void secSetPortBlocked(IN P_ADAPTER_T prAdapter,
                       IN P_STA_RECORD_T prSta,
                       IN u8 fgPort);

u8 secCheckClassError(IN P_ADAPTER_T prAdapter,
                      IN P_SW_RFB_T prSwRfb,
                      IN P_STA_RECORD_T prStaRec);

u8 secTxPortControlCheck(IN P_ADAPTER_T prAdapter,
                         IN P_MSDU_INFO_T prMsduInfo,
                         IN P_STA_RECORD_T prStaRec);

u8 secRxPortControlCheck(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSWRfb);

void secSetCipherSuite(IN P_ADAPTER_T prAdapter, IN u32 u4CipherSuitesFlags);

u8 secIsProtectedFrame(IN P_ADAPTER_T prAdapter,
                       IN P_MSDU_INFO_T prMsdu,
                       IN P_STA_RECORD_T prStaRec);

void secClearPmkid(IN P_ADAPTER_T prAdapter);

u8 secRsnKeyHandshakeEnabled(IN P_ADAPTER_T prAdapter);

u8 secGetBmcWlanIndex(IN P_ADAPTER_T prAdapter,
                      IN ENUM_NETWORK_TYPE_T eNetType,
                      IN P_STA_RECORD_T prStaRec);

u8 secTransmitKeyExist(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta);

u8 secEnabledInAis(IN P_ADAPTER_T prAdapter);

u8 secPrivacySeekForEntry(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta);

void secPrivacyFreeForEntry(IN P_ADAPTER_T prAdapter, IN u8 ucEntry);

void secPrivacyFreeSta(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

void secRemoveBssBcEntry(IN P_ADAPTER_T prAdapter,
                         IN P_BSS_INFO_T prBssInfo,
                         IN u8 fgRoam);

u8 secPrivacySeekForBcEntry(IN P_ADAPTER_T prAdapter,
                            IN u8 ucBssIndex,
                            IN u8 *pucAddr,
                            IN u8 ucStaIdx,
                            IN u8 ucAlg,
                            IN u8 ucKeyId);

u8 secGetStaIdxByWlanIdx(IN P_ADAPTER_T prAdapter, IN u8 ucWlanIdx);

u8 secGetBssIdxByWlanIdx(IN P_ADAPTER_T prAdapter, IN u8 ucWlanIdx);

u8 secLookupStaRecIndexFromTA(P_ADAPTER_T prAdapter, u8 *pucMacAddress);

void secPrivacyDumpWTBL(IN P_ADAPTER_T prAdapter);

u8 secCheckWTBLAssign(IN P_ADAPTER_T prAdapter);

u8 secIsProtected1xFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);

u8 secIsProtectedBss(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo);

u8 tkipMicDecapsulate(IN P_SW_RFB_T prSwRfb, IN u8 *pucMicKey);

u8 tkipMicDecapsulateInRxHdrTransMode(IN P_SW_RFB_T prSwRfb, IN u8 *pucMicKey);

void secPostUpdateAddr(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
