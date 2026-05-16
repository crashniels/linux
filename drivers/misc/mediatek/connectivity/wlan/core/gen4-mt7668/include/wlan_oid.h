/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "wlan_oid.h"
 *    \brief This file contains the declairation file of the WLAN OID processing
 * routines of Windows driver for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */

#ifndef _WLAN_OID_H
#define _WLAN_OID_H

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

#define PARAM_MAX_LEN_SSID               32

#define PARAM_MAC_ADDR_LEN               6

#define ETHERNET_HEADER_SZ               14
#define ETHERNET_MIN_PKT_SZ              60
#define ETHERNET_MAX_PKT_SZ              1514

#define PARAM_MAX_LEN_RATES              8
#define PARAM_MAX_LEN_RATES_EX               16

#define PARAM_AUTH_REQUEST_REAUTH            0x01
#define PARAM_AUTH_REQUEST_KEYUPDATE             0x02
#define PARAM_AUTH_REQUEST_PAIRWISE_ERROR        0x06
#define PARAM_AUTH_REQUEST_GROUP_ERROR           0x0E

#define PARAM_EEPROM_READ_METHOD_READ            1
#define PARAM_EEPROM_READ_METHOD_GETSIZE         0

#define PARAM_WHQL_RSSI_MAX_DBM              (-10)
#define PARAM_WHQL_RSSI_MIN_DBM              (-200)

#define PARAM_DEVICE_WAKE_UP_ENABLE          0x00000001
#define PARAM_DEVICE_WAKE_ON_PATTERN_MATCH_ENABLE    0x00000002
#define PARAM_DEVICE_WAKE_ON_MAGIC_PACKET_ENABLE     0x00000004

#define PARAM_WAKE_UP_MAGIC_PACKET           0x00000001
#define PARAM_WAKE_UP_PATTERN_MATCH          0x00000002
#define PARAM_WAKE_UP_LINK_CHANGE            0x00000004

/* Packet filter bit definitioin (u32 bit-wise definition) */
#define PARAM_PACKET_FILTER_DIRECTED             0x00000001
#define PARAM_PACKET_FILTER_MULTICAST            0x00000002
#define PARAM_PACKET_FILTER_ALL_MULTICAST        0x00000004
#define PARAM_PACKET_FILTER_BROADCAST            0x00000008
#define PARAM_PACKET_FILTER_PROMISCUOUS          0x00000020
#define PARAM_PACKET_FILTER_ALL_LOCAL            0x00000080
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
#define PARAM_PACKET_FILTER_P2P_MASK             0xC0000000
#define PARAM_PACKET_FILTER_PROBE_REQ            0x80000000
#define PARAM_PACKET_FILTER_ACTION_FRAME         0x40000000
#endif

#define PARAM_PACKET_FILTER_SUPPORTED                               \
    (PARAM_PACKET_FILTER_DIRECTED | PARAM_PACKET_FILTER_MULTICAST | \
     PARAM_PACKET_FILTER_BROADCAST | PARAM_PACKET_FILTER_ALL_MULTICAST)

#define PARAM_MEM_DUMP_MAX_SIZE              1536

#define BT_PROFILE_PARAM_LEN                 8

#define EFUSE_ADDR_MAX                   0x3BF  /* Based on EEPROM layout 20160120 */
#if CFG_SUPPORT_BUFFER_MODE

/* For MT7668 */
#define EFUSE_CONTENT_BUFFER_START           0x03A
#define EFUSE_CONTENT_BUFFER_END             0x1D9
#define EFUSE_CONTENT_BUFFER_SIZE \
    (EFUSE_CONTENT_BUFFER_END - EFUSE_CONTENT_BUFFER_START + 1)

#define DEFAULT_EFUSE_MACADDR_OFFSET             4

/* For MT6632 */
#define EFUSE_CONTENT_SIZE               16

#define EFUSE_BLOCK_SIZE                 16
#define EEPROM_SIZE                  1184
#define MAX_EEPROM_BUFFER_SIZE               1200
#endif

#if CFG_SUPPORT_TX_BF
#define TXBF_CMD_NEED_TO_RESPONSE(u4TxBfCmdId) \
    (u4TxBfCmdId == BF_PFMU_TAG_READ || u4TxBfCmdId == BF_PROFILE_READ)
#endif
#define MU_CMD_NEED_TO_RESPONSE(u4MuCmdId)                              \
    (u4MuCmdId == MU_GET_CALC_INIT_MCS || u4MuCmdId == MU_HQA_GET_QD || \
     u4MuCmdId == MU_HQA_GET_CALC_LQ)
#if CFG_SUPPORT_MU_MIMO
/* @NITESH: MACROS For Init MCS calculation (MU Metric Table) */
#define NUM_MUT_FEC   2
#define NUM_MUT_MCS   10
#define NUM_MUT_NR_NUM    3
#define NUM_MUT_INDEX     8

#define NUM_OF_USER   2
#define NUM_OF_MODUL      5
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/* Parameters of User Configuration which match to NDIS5.1                    */
/*----------------------------------------------------------------------------*/
/* NDIS_802_11_AUTHENTICATION_MODE */
typedef enum _ENUM_PARAM_AUTH_MODE_T {
    AUTH_MODE_OPEN,  /*!< Open system */
    AUTH_MODE_SHARED,  /*!< Shared key */
    AUTH_MODE_AUTO_SWITCH,  /*!< Either open system or shared key */
    AUTH_MODE_WPA,
    AUTH_MODE_WPA_PSK,
    AUTH_MODE_WPA_NONE,  /*!< For Ad hoc */
    AUTH_MODE_WPA2,
    AUTH_MODE_WPA2_PSK,
    AUTH_MODE_WPA2_SAE,
    AUTH_MODE_NUM  /*!< Upper bound, not real case */
} ENUM_PARAM_AUTH_MODE_T,
*P_ENUM_PARAM_AUTH_MODE_T;

/* NDIS_802_11_ENCRYPTION_STATUS */ /* Encryption types */
typedef enum _ENUM_WEP_STATUS_T {
    ENUM_WEP_ENABLED,
    ENUM_ENCRYPTION1_ENABLED      = ENUM_WEP_ENABLED,
    ENUM_WEP_DISABLED,
    ENUM_ENCRYPTION_DISABLED      = ENUM_WEP_DISABLED,
    ENUM_WEP_KEY_ABSENT,
    ENUM_ENCRYPTION1_KEY_ABSENT   = ENUM_WEP_KEY_ABSENT,
    ENUM_WEP_NOT_SUPPORTED,
    ENUM_ENCRYPTION_NOT_SUPPORTED = ENUM_WEP_NOT_SUPPORTED,
    ENUM_ENCRYPTION2_ENABLED,
    ENUM_ENCRYPTION2_KEY_ABSENT,
    ENUM_ENCRYPTION3_ENABLED,
    ENUM_ENCRYPTION3_KEY_ABSENT,
#if CFG_SUPPORT_SUITB
    ENUM_ENCRYPTION4_ENABLED,
    ENUM_ENCRYPTION4_KEY_ABSENT,
#endif
    ENUM_ENCRYPTION_NUM
} ENUM_PARAM_ENCRYPTION_STATUS_T,
*P_ENUM_PARAM_ENCRYPTION_STATUS_T;

typedef u8 PARAM_MAC_ADDRESS[PARAM_MAC_ADDR_LEN];

typedef u32 PARAM_KEY_INDEX;
typedef u64 PARAM_KEY_RSC;
typedef s32 PARAM_RSSI;

typedef u32 PARAM_FRAGMENTATION_THRESHOLD;
typedef u32 PARAM_RTS_THRESHOLD;

typedef u8 PARAM_RATES[PARAM_MAX_LEN_RATES];
typedef u8 PARAM_RATES_EX[PARAM_MAX_LEN_RATES_EX];

typedef enum _ENUM_PARAM_PHY_TYPE_T {
    PHY_TYPE_802_11ABG = 0,  /*!< Can associated with 802.11abg AP,
                              * Scan dual band.
                              */
    PHY_TYPE_802_11BG,  /*!< Can associated with 802_11bg AP,
                         *  Scan single band and not report 802_11a BSSs.
                         */
    PHY_TYPE_802_11G,  /*!< Can associated with 802_11g only AP,
                        * Scan single band and not report 802_11ab BSSs.
                        */
    PHY_TYPE_802_11A,  /*!< Can associated with 802_11a only AP,
                        * Scan single band and not report 802_11bg BSSs.
                        */
    PHY_TYPE_802_11B,  /*!< Can associated with 802_11b only AP
                        * Scan single band and not report 802_11ag BSSs.
                        */
    PHY_TYPE_NUM  /* 5 */
} ENUM_PARAM_PHY_TYPE_T,
*P_ENUM_PARAM_PHY_TYPE_T;

typedef enum _ENUM_PARAM_OP_MODE_T {
    NET_TYPE_IBSS = 0,  /*!< Try to merge/establish an AdHoc, do periodic
                         * SCAN for merging. */
    NET_TYPE_INFRA,  /*!< Try to join an Infrastructure, do periodic SCAN for
                      * joining. */
    NET_TYPE_AUTO_SWITCH,  /*!< Try to join an Infrastructure, if fail then
                            * try to merge or */
    /*  establish an AdHoc, do periodic SCAN for joining or merging. */
    NET_TYPE_DEDICATED_IBSS,  /*!< Try to merge an AdHoc first, */
    /* if fail then establish AdHoc permanently, no more SCAN. */
    NET_TYPE_NUM  /* 4 */
} ENUM_PARAM_OP_MODE_T,
*P_ENUM_PARAM_OP_MODE_T;

enum ENUM_CFG80211_WDEV_LOCK_FUNC {
    CFG80211_RX_ASSOC_RESP = 0,
    CFG80211_RX_MLME_MGMT,
    CFG80211_TX_MLME_MGMT,
    CFG80211_ASSOC_TIMEOUT
};

typedef struct _PARAM_SSID_T {
    u32 u4SsidLen;  /*!< SSID length in bytes. Zero length is broadcast(any)
                     * SSID */
    u8 aucSsid[PARAM_MAX_LEN_SSID];
} PARAM_SSID_T, *P_PARAM_SSID_T;

typedef struct _PARAM_CONNECT_T {
    u32 u4SsidLen;  /*!< SSID length in bytes. Zero length is broadcast(any)
                     * SSID */
    u8 *pucSsid;
    u8 *pucBssid;
    u32 u4CenterFreq;
} PARAM_CONNECT_T, *P_PARAM_CONNECT_T;

/* This is enum defined for user to select an AdHoc Mode */
typedef enum _ENUM_PARAM_AD_HOC_MODE_T {
    AD_HOC_MODE_11B = 0,  /*!< Create 11b IBSS if we support
                           * 802.11abg/802.11bg. */
    AD_HOC_MODE_MIXED_11BG,  /*!< Create 11bg mixed IBSS if we support
                              * 802.11abg/802.11bg/802.11g. */
    AD_HOC_MODE_11G,  /*!< Create 11g only IBSS if we support
                       * 802.11abg/802.11bg/802.11g. */
    AD_HOC_MODE_11A,  /*!< Create 11a only IBSS if we support 802.11abg. */
    AD_HOC_MODE_NUM  /* 4 */
} ENUM_PARAM_AD_HOC_MODE_T,
*P_ENUM_PARAM_AD_HOC_MODE_T;

typedef enum _ENUM_PARAM_MEDIA_STATE_T {
    PARAM_MEDIA_STATE_CONNECTED,
    PARAM_MEDIA_STATE_DISCONNECTED,
    PARAM_MEDIA_STATE_TO_BE_INDICATED  /* for following MSDN re-association
                                        * behavior */
} ENUM_PARAM_MEDIA_STATE_T,
*P_ENUM_PARAM_MEDIA_STATE_T;

typedef enum _ENUM_PARAM_NETWORK_TYPE_T {
    PARAM_NETWORK_TYPE_FH,
    PARAM_NETWORK_TYPE_DS,
    PARAM_NETWORK_TYPE_OFDM5,
    PARAM_NETWORK_TYPE_OFDM24,
    PARAM_NETWORK_TYPE_AUTOMODE,
    PARAM_NETWORK_TYPE_NUM  /*!< Upper bound, not real case */
} ENUM_PARAM_NETWORK_TYPE_T,
*P_ENUM_PARAM_NETWORK_TYPE_T;

typedef struct _PARAM_NETWORK_TYPE_LIST {
    u32 NumberOfItems;  /*!< At least 1 */
    ENUM_PARAM_NETWORK_TYPE_T eNetworkType[1];
} PARAM_NETWORK_TYPE_LIST, *PPARAM_NETWORK_TYPE_LIST;

typedef enum _ENUM_PARAM_PRIVACY_FILTER_T {
    PRIVACY_FILTER_ACCEPT_ALL,
    PRIVACY_FILTER_8021xWEP,
    PRIVACY_FILTER_NUM
} ENUM_PARAM_PRIVACY_FILTER_T,
*P_ENUM_PARAM_PRIVACY_FILTER_T;

typedef enum _ENUM_RELOAD_DEFAULTS {
    ENUM_RELOAD_WEP_KEYS
} PARAM_RELOAD_DEFAULTS,
*P_PARAM_RELOAD_DEFAULTS;

typedef struct _PARAM_PM_PACKET_PATTERN {
    u32 Priority;  /* Importance of the given pattern. */
    u32 Reserved;  /* Context information for transports. */
    u32 MaskSize;  /* Size in bytes of the pattern mask. */
    u32 PatternOffset;  /* Offset from beginning of this */
    /* structure to the pattern bytes. */
    u32 PatternSize;  /* Size in bytes of the pattern. */
    u32 PatternFlags;  /* Flags (TBD). */
} PARAM_PM_PACKET_PATTERN, *P_PARAM_PM_PACKET_PATTERN;

/* Combine ucTpTestMode and ucSigmaTestMode in one flag */
/* ucTpTestMode == 0, for normal driver */
/* ucTpTestMode == 1, for pure throughput test mode (ex: RvR) */
/* ucTpTestMode == 2, for sigma TGn/TGac/PMF */
/* ucTpTestMode == 3, for sigma WMM PS */
/* ucTpTestMode == 4, for Audio MRM mode */
typedef enum _ENUM_TP_TEST_MODE_T {
    ENUM_TP_TEST_MODE_NORMAL = 0,
    ENUM_TP_TEST_MODE_THROUGHPUT,
    ENUM_TP_TEST_MODE_SIGMA_AC_N_PMF,
    ENUM_TP_TEST_MODE_SIGMA_WMM_PS,
    ENUM_TP_TEST_MODE_AUDIO_MRM,
    ENUM_TP_TEST_MODE_NUM
} ENUM_TP_TEST_MODE_T,
*P_ENUM_TP_TEST_MODE_T;

/*--------------------------------------------------------------*/
/*! \brief Struct definition to indicate specific event.                */
/*--------------------------------------------------------------*/
typedef enum _ENUM_STATUS_TYPE_T {
    ENUM_STATUS_TYPE_AUTHENTICATION,
    ENUM_STATUS_TYPE_MEDIA_STREAM_MODE,
    ENUM_STATUS_TYPE_CANDIDATE_LIST,
    ENUM_STATUS_TYPE_NUM  /*!< Upper bound, not real case */
} ENUM_STATUS_TYPE_T,
*P_ENUM_STATUS_TYPE_T;

typedef struct _PARAM_802_11_CONFIG_FH_T {
    u32 u4Length;  /*!< Length of structure */
    u32 u4HopPattern;  /*!< Defined as 802.11 */
    u32 u4HopSet;  /*!< to one if non-802.11 */
    u32 u4DwellTime;  /*!< In unit of Kusec */
} PARAM_802_11_CONFIG_FH_T, *P_PARAM_802_11_CONFIG_FH_T;

typedef struct _PARAM_802_11_CONFIG_T {
    u32 u4Length;  /*!< Length of structure */
    u32 u4BeaconPeriod;  /*!< In unit of Kusec */
    u32 u4ATIMWindow;  /*!< In unit of Kusec */
    u32 u4DSConfig;  /*!< Channel frequency in unit of kHz */
    PARAM_802_11_CONFIG_FH_T rFHConfig;
} PARAM_802_11_CONFIG_T, *P_PARAM_802_11_CONFIG_T;

typedef struct _PARAM_STATUS_INDICATION_T {
    ENUM_STATUS_TYPE_T eStatusType;
} PARAM_STATUS_INDICATION_T, *P_PARAM_STATUS_INDICATION_T;

typedef struct _PARAM_AUTH_REQUEST_T {
    u32 u4Length;  /*!< Length of this struct */
    PARAM_MAC_ADDRESS arBssid;
    u32 u4Flags;  /*!< Definitions are as follows */
} PARAM_AUTH_REQUEST_T, *P_PARAM_AUTH_REQUEST_T;

typedef struct _PARAM_AUTH_EVENT_T {
    PARAM_STATUS_INDICATION_T rStatus;
    PARAM_AUTH_REQUEST_T arRequest[1];
} PARAM_AUTH_EVENT_T, *P_PARAM_AUTH_EVENT_T;

/*! \brief Capabilities, privacy, rssi and IEs of each BSSID */
typedef struct _PARAM_BSSID_EX_T {
    u32 u4Length;  /*!< Length of structure */
    PARAM_MAC_ADDRESS arMacAddress;  /*!< BSSID */
    u8 Reserved[2];
    PARAM_SSID_T rSsid;  /*!< SSID */
    u32 u4Privacy;  /*!< Need WEP encryption */
    PARAM_RSSI rRssi;  /*!< in dBm */
    ENUM_PARAM_NETWORK_TYPE_T eNetworkTypeInUse;
    PARAM_802_11_CONFIG_T rConfiguration;
    ENUM_PARAM_OP_MODE_T eOpMode;
    PARAM_RATES_EX rSupportedRates;
    u32 u4IELength;
    u8 aucIEs[1];
} PARAM_BSSID_EX_T, *P_PARAM_BSSID_EX_T;

typedef struct _PARAM_BSSID_LIST_EX {
    u32 u4NumberOfItems;  /*!< at least 1 */
    PARAM_BSSID_EX_T arBssid[1];
} PARAM_BSSID_LIST_EX_T, *P_PARAM_BSSID_LIST_EX_T;

typedef struct _PARAM_WEP_T {
    u32 u4Length;  /*!< Length of structure */
    u32 u4KeyIndex;  /*!< 0: pairwise key, others group keys */
    u32 u4KeyLength;  /*!< Key length in bytes */
    u8 aucKeyMaterial[32];  /*!< Key content by above setting */
} PARAM_WEP_T, *P_PARAM_WEP_T;

/*! \brief Key mapping of BSSID */
typedef struct _PARAM_KEY_T {
    u32 u4Length;  /*!< Length of structure */
    u32 u4KeyIndex;  /*!< KeyID */
    u32 u4KeyLength;  /*!< Key length in bytes */
    PARAM_MAC_ADDRESS arBSSID;  /*!< MAC address */
    PARAM_KEY_RSC rKeyRSC;
    u8 ucBssIdx;
    u8 ucCipher;
    u8 aucKeyMaterial[32];  /*!< Key content by above setting */
    /* Following add to change the original windows structure */
} PARAM_KEY_T, *P_PARAM_KEY_T;

/* for more remove key control (ucCtrlFlag) */
#define FLAG_RM_KEY_CTRL_WO_OID    BIT(0)  /* not OID operation */

typedef struct _PARAM_REMOVE_KEY_T {
    u32 u4Length;  /*!< Length of structure */
    u32 u4KeyIndex;  /*!< KeyID */
    PARAM_MAC_ADDRESS arBSSID;  /*!< MAC address */
    u8 ucBssIdx;
    u8 ucCtrlFlag;  /* Ctrl Flag for RM key CMD */
} PARAM_REMOVE_KEY_T, *P_PARAM_REMOVE_KEY_T;

/*! \brief Default key */
typedef struct _PARAM_DEFAULT_KEY_T {
    u8 ucKeyID;
    u8 ucUnicast;
    u8 ucMulticast;
    u8 ucBssIdx;
} PARAM_DEFAULT_KEY_T, *P_PARAM_DEFAULT_KEY_T;

typedef enum _PARAM_POWER_MODE {
    Param_PowerModeCAM,
    Param_PowerModeMAX_PSP,
    Param_PowerModeFast_PSP,
#ifdef SUPPORT_PERIODIC_PS
    Param_PowerModePeriodic_PSP,
#endif
    Param_PowerModeMax  /* Upper bound, not real case */
} PARAM_POWER_MODE,
*PPARAM_POWER_MODE;

typedef enum _PARAM_DEVICE_POWER_STATE {
    ParamDeviceStateUnspecified = 0,
    ParamDeviceStateD0,
    ParamDeviceStateD1,
    ParamDeviceStateD2,
    ParamDeviceStateD3,
    ParamDeviceStateMaximum
} PARAM_DEVICE_POWER_STATE,
*PPARAM_DEVICE_POWER_STATE;

#if CFG_SUPPORT_FW_DBG_LEVEL_CTRL
/* FW debug control level related definition and enumerations */
#define FW_DBG_LEVEL_DONT_SET    0
#define FW_DBG_LEVEL_ERROR   (1 << 0)
#define FW_DBG_LEVEL_WARN    (1 << 1)
#define FW_DBG_LEVEL_STATE   (1 << 2)
#define FW_DBG_LEVEL_INFO    (1 << 3)
#define FW_DBG_LEVEL_LOUD    (1 << 4)
#endif

typedef struct _PARAM_POWER_MODE_T {
    u8 ucBssIdx;
    PARAM_POWER_MODE ePowerMode;
} PARAM_POWER_MODE_T, *P_PARAM_POWER_MODE_T;

#if CFG_SUPPORT_802_11D

/*! \brief The enumeration definitions for OID_IPN_MULTI_DOMAIN_CAPABILITY */
typedef enum _PARAM_MULTI_DOMAIN_CAPABILITY {
    ParamMultiDomainCapDisabled,
    ParamMultiDomainCapEnabled
} PARAM_MULTI_DOMAIN_CAPABILITY,
*P_PARAM_MULTI_DOMAIN_CAPABILITY;
#endif

typedef struct _COUNTRY_STRING_ENTRY {
    u8 aucCountryCode[2];
    u8 aucEnvironmentCode[2];
} COUNTRY_STRING_ENTRY, *P_COUNTRY_STRING_ENTRY;

/* Power management related definition and enumerations */
#define UAPSD_NONE    0
#define UAPSD_AC0     (BIT(0) | BIT(4))
#define UAPSD_AC1     (BIT(1) | BIT(5))
#define UAPSD_AC2     (BIT(2) | BIT(6))
#define UAPSD_AC3     (BIT(3) | BIT(7))
#define UAPSD_ALL     (UAPSD_AC0 | UAPSD_AC1 | UAPSD_AC2 | UAPSD_AC3)

typedef enum _ENUM_POWER_SAVE_PROFILE_T {
    ENUM_PSP_CONTINUOUS_ACTIVE = 0,
    ENUM_PSP_CONTINUOUS_POWER_SAVE,
    ENUM_PSP_FAST_SWITCH,
    ENUM_PSP_NUM
} ENUM_POWER_SAVE_PROFILE_T,
*PENUM_POWER_SAVE_PROFILE_T;

/*--------------------------------------------------------------*/
/*! \brief Set/Query testing type.                              */
/*--------------------------------------------------------------*/
typedef struct _PARAM_802_11_TEST_T {
    u32 u4Length;
    u32 u4Type;
    union {
        PARAM_AUTH_EVENT_T AuthenticationEvent;
        PARAM_RSSI RssiTrigger;
    } u;
} PARAM_802_11_TEST_T, *P_PARAM_802_11_TEST_T;

/*--------------------------------------------------------------*/
/*! \brief Set/Query authentication and encryption capability.  */
/*--------------------------------------------------------------*/
typedef struct _PARAM_AUTH_ENCRYPTION_T {
    ENUM_PARAM_AUTH_MODE_T eAuthModeSupported;
    ENUM_PARAM_ENCRYPTION_STATUS_T eEncryptStatusSupported;
} PARAM_AUTH_ENCRYPTION_T, *P_PARAM_AUTH_ENCRYPTION_T;

typedef struct _PARAM_CAPABILITY_T {
    u32 u4Length;
    u32 u4Version;
    u32 u4NoOfPMKIDs;
    u32 u4NoOfAuthEncryptPairsSupported;
    PARAM_AUTH_ENCRYPTION_T arAuthenticationEncryptionSupported[1];
} PARAM_CAPABILITY_T, *P_PARAM_CAPABILITY_T;

typedef u8 PARAM_PMKID_VALUE[16];

typedef struct _PARAM_BSSID_INFO_T {
    PARAM_MAC_ADDRESS arBSSID;
    PARAM_PMKID_VALUE arPMKID;
} PARAM_BSSID_INFO_T, *P_PARAM_BSSID_INFO_T;

typedef struct _PARAM_PMKID_T {
    u32 u4Length;
    u32 u4BSSIDInfoCount;
    PARAM_BSSID_INFO_T arBSSIDInfo[1];
} PARAM_PMKID_T, *P_PARAM_PMKID_T;

/*! \brief PMKID candidate lists. */
typedef struct _PARAM_PMKID_CANDIDATE_T {
    PARAM_MAC_ADDRESS arBSSID;
    u32 u4Flags;
} PARAM_PMKID_CANDIDATE_T, *P_PARAM_PMKID_CANDIDATE_T;

typedef struct _PARAM_PMKID_CANDIDATE_LIST_T {
    u32 u4Version;  /*!< Version */
    u32 u4NumCandidates;  /*!< How many candidates follow */
    PARAM_PMKID_CANDIDATE_T arCandidateList[1];
} PARAM_PMKID_CANDIDATE_LIST_T, *P_PARAM_PMKID_CANDIDATE_LIST_T;

#define NL80211_KCK_LEN       16
#define NL80211_KEK_LEN       16
#define NL80211_REPLAY_CTR_LEN    8
#define NL80211_KEYRSC_LEN    8

typedef struct _PARAM_GTK_REKEY_DATA {
    u8 aucKek[NL80211_KEK_LEN];
    u8 aucKck[NL80211_KCK_LEN];
    u8 aucReplayCtr[NL80211_REPLAY_CTR_LEN];
    u8 ucBssIndex;
    u8 ucRekeyMode;
    u8 ucCurKeyId;
    u8 ucRsv;
    u32 u4Proto;
    u32 u4PairwiseCipher;
    u32 u4GroupCipher;
    u32 u4KeyMgmt;
    u32 u4MgmtGroupCipher;
} PARAM_GTK_REKEY_DATA, *P_PARAM_GTK_REKEY_DATA;

typedef struct _PARAM_CUSTOM_MCR_RW_STRUCT_T {
    u32 u4McrOffset;
    u32 u4McrData;
} PARAM_CUSTOM_MCR_RW_STRUCT_T, *P_PARAM_CUSTOM_MCR_RW_STRUCT_T;

/* CMD_COEX_CTRL & EVENT_COEX_CTRL */
/************************************************/
/*  u32 u4SubCmd : Coex Ctrl Sub Command    */
/*  u8 aucBuffer : Reserve for Sub Command  */
/*                        Data Structure        */
/************************************************/
struct PARAM_COEX_CTRL {
    u32 u4SubCmd;
    u8 aucBuffer[64];
};

/* Isolation Structure */
/************************************************/
/*  u32 u4IsoPath : WF Path (WF0/WF1)       */
/*  u32 u4Channel : WF Channel              */
/*  u32 u4Band    : WF Band (Band0/Band1)(Not used now)   */
/*  u32 u4Isolation  : Isolation value      */
/************************************************/
struct PARAM_COEX_ISO_DETECT {
    u32 u4IsoPath;
    u32 u4Channel;
    /*u32 u4Band;*/
    u32 u4Isolation;
};

#if CFG_SUPPORT_QA_TOOL
#if CFG_SUPPORT_BUFFER_MODE
typedef struct _BIN_CONTENT_T {
    u16 u2Addr;
    u8 ucValue;
    u8 ucReserved;
} BIN_CONTENT_T, *P_BIN_CONTENT_T;

typedef struct _PARAM_CUSTOM_EFUSE_BUFFER_MODE_T {
    u8 ucSourceMode;
    u8 ucCount;
    u8 ucCmdType;
    u8 ucReserved;
    u8 aBinContent[MAX_EEPROM_BUFFER_SIZE];
} PARAM_CUSTOM_EFUSE_BUFFER_MODE_T, *P_PARAM_CUSTOM_EFUSE_BUFFER_MODE_T;

/*#if (CFG_EEPROM_PAGE_ACCESS == 1)*/
typedef struct _PARAM_CUSTOM_ACCESS_EFUSE_T {
    u32 u4Address;
    u32 u4Valid;
    u8 aucData[16];
} PARAM_CUSTOM_ACCESS_EFUSE_T, *P_PARAM_CUSTOM_ACCESS_EFUSE_T;

typedef struct _PARAM_CUSTOM_EFUSE_FREE_BLOCK_T {
    u8 ucGetFreeBlock;
    u8 aucReserved[3];
} PARAM_CUSTOM_EFUSE_FREE_BLOCK_T, *P_PARAM_CUSTOM_EFUSE_FREE_BLOCK_T;

typedef struct _PARAM_CUSTOM_GET_TX_POWER_T {
    u8 ucTxPwrType;
    u8 ucCenterChannel;
    u8 ucDbdcIdx;  /* 0:Band 0, 1: Band1 */
    u8 ucBand;  /* 0:G-band 1: A-band*/
    u8 ucReserved[4];
} PARAM_CUSTOM_GET_TX_POWER_T, *P_PARAM_CUSTOM_GET_TX_POWER_T;

/*#endif*/

#endif

typedef struct _PARAM_CUSTOM_SET_TX_TARGET_POWER_T {
    s8 cTxPwr2G4Cck;  /* signed, in unit of 0.5dBm */
    s8 cTxPwr2G4Dsss;  /* signed, in unit of 0.5dBm */
    u8 ucTxTargetPwr;  /* Tx target power base for all*/
    u8 ucReserved;

    s8 cTxPwr2G4OFDM_BPSK;
    s8 cTxPwr2G4OFDM_QPSK;
    s8 cTxPwr2G4OFDM_16QAM;
    s8 cTxPwr2G4OFDM_Reserved;
    s8 cTxPwr2G4OFDM_48Mbps;
    s8 cTxPwr2G4OFDM_54Mbps;

    s8 cTxPwr2G4HT20_BPSK;
    s8 cTxPwr2G4HT20_QPSK;
    s8 cTxPwr2G4HT20_16QAM;
    s8 cTxPwr2G4HT20_MCS5;
    s8 cTxPwr2G4HT20_MCS6;
    s8 cTxPwr2G4HT20_MCS7;

    s8 cTxPwr2G4HT40_BPSK;
    s8 cTxPwr2G4HT40_QPSK;
    s8 cTxPwr2G4HT40_16QAM;
    s8 cTxPwr2G4HT40_MCS5;
    s8 cTxPwr2G4HT40_MCS6;
    s8 cTxPwr2G4HT40_MCS7;

    s8 cTxPwr5GOFDM_BPSK;
    s8 cTxPwr5GOFDM_QPSK;
    s8 cTxPwr5GOFDM_16QAM;
    s8 cTxPwr5GOFDM_Reserved;
    s8 cTxPwr5GOFDM_48Mbps;
    s8 cTxPwr5GOFDM_54Mbps;

    s8 cTxPwr5GHT20_BPSK;
    s8 cTxPwr5GHT20_QPSK;
    s8 cTxPwr5GHT20_16QAM;
    s8 cTxPwr5GHT20_MCS5;
    s8 cTxPwr5GHT20_MCS6;
    s8 cTxPwr5GHT20_MCS7;

    s8 cTxPwr5GHT40_BPSK;
    s8 cTxPwr5GHT40_QPSK;
    s8 cTxPwr5GHT40_16QAM;
    s8 cTxPwr5GHT40_MCS5;
    s8 cTxPwr5GHT40_MCS6;
    s8 cTxPwr5GHT40_MCS7;
} PARAM_CUSTOM_SET_TX_TARGET_POWER_T, *P_PARAM_CUSTOM_SET_TX_TARGET_POWER_T;

#if (CFG_SUPPORT_DFS_MASTER == 1)
typedef struct _PARAM_CUSTOM_SET_RDD_REPORT_T {
    u8 ucDbdcIdx;  /* 0:Band 0, 1: Band1 */
} PARAM_CUSTOM_SET_RDD_REPORT_T, *P_PARAM_CUSTOM_SET_RDD_REPORT_T;

struct PARAM_CUSTOM_SET_RADAR_DETECT_MODE {
    u8 ucRadarDetectMode;  /* 0:Switch channel, 1: Don't switch channel */
};
#endif

typedef struct _PARAM_CUSTOM_ACCESS_RX_STAT {
    u32 u4SeqNum;
    u32 u4TotalNum;
} PARAM_CUSTOM_ACCESS_RX_STAT, *P_PARAM_CUSTOM_ACCESS_RX_STAT;

/* Ext DevInfo Tag */
typedef enum _EXT_ENUM_DEVINFO_TAG_HANDLE_T {
    DEV_INFO_ACTIVE = 0,
    DEV_INFO_BSSID,
    DEV_INFO_MAX_NUM
} EXT_ENUM_TAG_DEVINFO_HANDLE_T;

/*  STA record TLV tag */
typedef enum _EXT_ENUM_STAREC_TAG_HANDLE_T {
    STA_REC_BASIC = 0,
    STA_REC_RA,
    STA_REC_RA_COMMON_INFO,
    STA_REC_RA_UPDATE,
    STA_REC_BF,
    STA_REC_MAUNAL_ASSOC,
    STA_REC_BA = 6,
    STA_REC_MAX_NUM
} EXT_ENUM_TAG_STAREC_HANDLE_T;

#if CFG_SUPPORT_TX_BF
typedef enum _BF_ACTION_CATEGORY {
    BF_SOUNDING_OFF = 0,
    BF_SOUNDING_ON,
    BF_HW_CTRL,
    BF_DATA_PACKET_APPLY,
    BF_PFMU_MEM_ALLOCATE,
    BF_PFMU_MEM_RELEASE,
    BF_PFMU_TAG_READ,
    BF_PFMU_TAG_WRITE,
    BF_PROFILE_READ,
    BF_PROFILE_WRITE,
    BF_PN_READ,
    BF_PN_WRITE,
    BF_PFMU_MEM_ALLOC_MAP_READ
} BF_ACTION_CATEGORY;

enum {
    DEVINFO_ACTIVE  = 0,
    DEVINFO_MAX_NUM = 1,
};

enum {
    DEVINFO_ACTIVE_FEATURE  = (1 << DEVINFO_ACTIVE),
    DEVINFO_MAX_NUM_FEATURE = (1 << DEVINFO_MAX_NUM)
};

enum {
    BSS_INFO_OWN_MAC        = 0,
    BSS_INFO_BASIC          = 1,
    BSS_INFO_RF_CH          = 2,
    BSS_INFO_PM             = 3,
    BSS_INFO_UAPSD          = 4,
    BSS_INFO_ROAM_DETECTION = 5,
    BSS_INFO_LQ_RM          = 6,
    BSS_INFO_EXT_BSS        = 7,
    BSS_INFO_BROADCAST_INFO = 8,
    BSS_INFO_SYNC_MODE      = 9,
    BSS_INFO_MAX_NUM
};

typedef union _PFMU_PROFILE_TAG1 {
    struct {
        u32 ucProfileID : 7;  /* [6:0] : 0 ~ 63 */
        u32 ucTxBf : 1;  /* [7] : 0: iBF, 1: eBF */
        u32 ucDBW : 2;  /* [9:8] : 0/1/2/3: DW20/40/80/160NC */
        u32 ucSU_MU : 1;  /* [10] : 0:SU, 1: MU */
        u32 ucInvalidProf : 1;  /* [11] : 0:default, 1: This profile
                                 * number is invalid by SW */
        u32 ucRMSD : 3;  /* [14:12] : RMSD value from CE */
        u32 ucMemAddr1ColIdx : 3;  /* [17 : 15] : column index : 0 ~ 5 */
        u32 ucMemAddr1RowIdx : 6;  /* [23 : 18] : row index : 0 ~ 63 */
        u32 ucMemAddr2ColIdx : 3;  /* [26 : 24] : column index : 0 ~ 5 */
        u32 ucMemAddr2RowIdx : 5;  /* [31 : 27] : row index : 0 ~ 63 */
        u32 ucMemAddr2RowIdxMsb : 1;  /* [32] : MSB of row index */
        u32 ucMemAddr3ColIdx : 3;  /* [35 : 33] : column index : 0 ~ 5 */
        u32 ucMemAddr3RowIdx : 6;  /* [41 : 36] : row index : 0 ~ 63 */
        u32 ucMemAddr4ColIdx : 3;  /* [44 : 42] : column index : 0 ~ 5 */
        u32 ucMemAddr4RowIdx : 6;  /* [50 : 45] : row index : 0 ~ 63 */
        u32 ucReserved : 1;  /* [51] : Reserved */
        u32 ucNrow : 2;  /* [53 : 52] : Nrow */
        u32 ucNcol : 2;  /* [55 : 54] : Ncol */
        u32 ucNgroup : 2;  /* [57 : 56] : Ngroup */
        u32 ucLM : 2;  /* [59 : 58] : 0/1/2 */
        u32 ucCodeBook : 2;  /* [61:60] : Code book */
        u32 ucHtcExist : 1;  /* [62] : HtcExist */
        u32 ucReserved1 : 1;  /* [63] : Reserved */
        u32 ucSNR_STS0 : 8;  /* [71:64] : SNR_STS0 */
        u32 ucSNR_STS1 : 8;  /* [79:72] : SNR_STS1 */
        u32 ucSNR_STS2 : 8;  /* [87:80] : SNR_STS2 */
        u32 ucSNR_STS3 : 8;  /* [95:88] : SNR_STS3 */
        u32 ucIBfLnaIdx : 8;  /* [103:96] : iBF LNA index */
    } rField;
    u32 au4RawData[4];
} PFMU_PROFILE_TAG1, *P_PFMU_PROFILE_TAG1;

typedef union _PFMU_PROFILE_TAG2 {
    struct {
        u32 u2SmartAnt : 12;  /* [11:0] : Smart Ant config */
        u32 ucReserved0 : 3;  /* [14:12] : Reserved */
        u32 ucSEIdx : 5;  /* [19:15] : SE index */
        u32 ucRMSDThd : 3;  /* [22:20] : RMSD Threshold */
        u32 ucReserved1 : 1;  /* [23] : Reserved */
        u32 ucMCSThL1SS : 4;  /* [27:24] : MCS TH long 1SS */
        u32 ucMCSThS1SS : 4;  /* [31:28] : MCS TH short 1SS */
        u32 ucMCSThL2SS : 4;  /* [35:32] : MCS TH long 2SS */
        u32 ucMCSThS2SS : 4;  /* [39:36] : MCS TH short 2SS */
        u32 ucMCSThL3SS : 4;  /* [43:40] : MCS TH long 3SS */
        u32 ucMCSThS3SS : 4;  /* [47:44] : MCS TH short 3SS */
        u32 uciBfTimeOut : 8;  /* [55:48] : iBF timeout limit */
        u32 ucReserved2 : 8;  /* [63:56] : Reserved */
        u32 ucReserved3 : 8;  /* [71:64] : Reserved */
        u32 ucReserved4 : 8;  /* [79:72] : Reserved */
        u32 uciBfDBW : 2;  /* [81:80] : iBF desired DBW 0/1/2/3 :
                            * BW20/40/80/160NC */
        u32 uciBfNcol : 2;  /* [83:82] : iBF desired Ncol = 1 ~ 3 */
        u32 uciBfNrow : 2;  /* [85:84] : iBF desired Nrow = 1 ~ 4 */
        u32 u2Reserved5 : 10;  /* [95:86] : Reserved */
    } rField;
    u32 au4RawData[3];
} PFMU_PROFILE_TAG2, *P_PFMU_PROFILE_TAG2;

typedef union _PFMU_DATA {
    struct {
        u32 u2Phi11 : 9;
        u32 ucPsi21 : 7;
        u32 u2Phi21 : 9;
        u32 ucPsi31 : 7;
        u32 u2Phi31 : 9;
        u32 ucPsi41 : 7;
        u32 u2Phi22 : 9;
        u32 ucPsi32 : 7;
        u32 u2Phi32 : 9;
        u32 ucPsi42 : 7;
        u32 u2Phi33 : 9;
        u32 ucPsi43 : 7;
        u32 u2dSNR00 : 4;
        u32 u2dSNR01 : 4;
        u32 u2dSNR02 : 4;
        u32 u2dSNR03 : 4;
        u32 u2Reserved : 16;
    } rField;
    u32 au4RawData[5];
} PFMU_DATA, *P_PFMU_DATA;

typedef struct _PROFILE_TAG_READ_T {
    u8 ucTxBfCategory;
    u8 ucProfileIdx;
    u8 fgBfer;
    u8 ucRsv;
} PROFILE_TAG_READ_T, *P_PROFILE_TAG_READ_T;

typedef struct _PROFILE_TAG_WRITE_T {
    u8 ucTxBfCategory;
    u8 ucPfmuId;
    u8 ucBuffer[28];
} PROFILE_TAG_WRITE_T, *P_PROFILE_TAG_WRITE_T;

typedef struct _PROFILE_DATA_READ_T {
    u8 ucTxBfCategory;
    u8 ucPfmuIdx;
    u8 fgBFer;
    u8 ucReserved[3];
    u8 ucSubCarrIdxLsb;
    u8 ucSubCarrIdxMsb;
} PROFILE_DATA_READ_T, *P_PROFILE_DATA_READ_T;

typedef struct _PROFILE_DATA_WRITE_T {
    u8 ucTxBfCategory;
    u8 ucPfmuIdx;
    u8 u2SubCarrIdxLsb;
    u8 u2SubCarrIdxMsb;
    PFMU_DATA rTxBfPfmuData;
} PROFILE_DATA_WRITE_T, *P_PROFILE_DATA_WRITE_T;

typedef struct _PROFILE_PN_READ_T {
    u8 ucTxBfCategory;
    u8 ucPfmuIdx;
    u8 ucReserved[2];
} PROFILE_PN_READ_T, *P_PROFILE_PN_READ_T;

typedef struct _PROFILE_PN_WRITE_T {
    u8 ucTxBfCategory;
    u8 ucPfmuIdx;
    u16 u2bw;
    u8 ucBuf[32];
} PROFILE_PN_WRITE_T, *P_PROFILE_PN_WRITE_T;

typedef enum _BF_SOUNDING_MODE {
    SU_SOUNDING = 0,
    MU_SOUNDING,
    SU_PERIODIC_SOUNDING,
    MU_PERIODIC_SOUNDING,
    AUTO_SU_PERIODIC_SOUNDING
} BF_SOUNDING_MODE;

typedef struct _EXT_CMD_ETXBf_SND_PERIODIC_TRIGGER_CTRL_T {
    u8 ucCmdCategoryID;
    u8 ucSuMuSndMode;
    u8 ucWlanIdx;
    u32 u4SoundingInterval;  /* By ms */
} EXT_CMD_ETXBf_SND_PERIODIC_TRIGGER_CTRL_T,
*P_EXT_CMD_ETXBf_SND_PERIODIC_TRIGGER_CTRL_T;

typedef struct _EXT_CMD_ETXBf_MU_SND_PERIODIC_TRIGGER_CTRL_T {
    u8 ucCmdCategoryID;
    u8 ucSuMuSndMode;
    u8 ucWlanId[4];
    u8 ucStaNum;
    u32 u4SoundingInterval;  /* By ms */
} EXT_CMD_ETXBf_MU_SND_PERIODIC_TRIGGER_CTRL_T,
*P_EXT_CMD_ETXBf_MU_SND_PERIODIC_TRIGGER_CTRL_T;

/* Device information (Tag0) */
typedef struct _CMD_DEVINFO_ACTIVE_T {
    u16 u2Tag;  /* Tag = 0x00 */
    u16 u2Length;
    u8 ucActive;
    u8 ucBandNum;
    u8 aucOwnMacAddr[6];
    u8 aucReserve[4];
} CMD_DEVINFO_ACTIVE_T, *P_CMD_DEVINFO_ACTIVE_T;

typedef struct _BSSINFO_BASIC_T {
    /* Basic BSS information (Tag1) */
    u16 u2Tag;  /* Tag = 0x01 */
    u16 u2Length;
    u32 u4NetworkType;
    u8 ucActive;
    u8 ucReserve0;
    u16 u2BcnInterval;
    u8 aucBSSID[6];
    u8 ucWmmIdx;
    u8 ucDtimPeriod;
    u8 ucBcMcWlanidx;  /* indicate which wlan-idx used for MC/BC
                        * transmission. */
    u8 ucCipherSuit;
    u8 acuReserve[6];
} CMD_BSSINFO_BASIC_T, *P_CMD_BSSINFO_BASIC_T;

typedef struct _TXBF_PFMU_STA_INFO {
    u16 u2PfmuId;  /* 0xFFFF means no access right for PFMU */
    u8 fgSU_MU;  /* 0 : SU, 1 : MU */
    u8 fgETxBfCap;  /* 0 : ITxBf, 1 : ETxBf */
    u8 ucSoundingPhy;  /* 0: legacy, 1: OFDM, 2: HT, 4: VHT */
    u8 ucNdpaRate;
    u8 ucNdpRate;
    u8 ucReptPollRate;
    u8 ucTxMode;  /* 0: legacy, 1: OFDM, 2: HT, 4: VHT */
    u8 ucNc;
    u8 ucNr;
    u8 ucCBW;  /* 0 : 20M, 1 : 40M, 2 : 80M, 3 : 80 + 80M */
    u8 ucTotMemRequire;
    u8 ucMemRequire20M;
    u8 ucMemRow0;
    u8 ucMemCol0;
    u8 ucMemRow1;
    u8 ucMemCol1;
    u8 ucMemRow2;
    u8 ucMemCol2;
    u8 ucMemRow3;
    u8 ucMemCol3;
    u16 u2SmartAnt;
    u8 ucSEIdx;
    u8 uciBfTimeOut;
    u8 uciBfDBW;
    u8 uciBfNcol;
    u8 uciBfNrow;
    u8 aucReserved[3];
} TXBF_PFMU_STA_INFO, *P_TXBF_PFMU_STA_INFO;

typedef struct _STA_REC_UPD_ENTRY_T {
    TXBF_PFMU_STA_INFO rTxBfPfmuStaInfo;
    u8 aucAddr[PARAM_MAC_ADDR_LEN];
    u8 ucAid;
    u8 ucRsv;
} STA_REC_UPD_ENTRY_T, *P_STA_REC_UPD_ENTRY_T;

typedef struct _STAREC_COMMON_T {
    /* Basic STA record (Group0) */
    u16 u2Tag;  /* Tag = 0x00 */
    u16 u2Length;
    u32 u4ConnectionType;
    u8 ucConnectionState;
    u8 ucIsQBSS;
    u16 u2AID;
    u8 aucPeerMacAddr[6];
    u16 u2Reserve1;
} CMD_STAREC_COMMON_T, *P_CMD_STAREC_COMMON_T;

typedef struct _CMD_STAREC_BF {
    u16 u2Tag;  /* Tag = 0x02 */
    u16 u2Length;
    TXBF_PFMU_STA_INFO rTxBfPfmuInfo;
    u8 ucReserved[3];
} CMD_STAREC_BF, *P_CMD_STAREC_BF;

/* QA tool: maunal assoc */
typedef struct _CMD_MANUAL_ASSOC_STRUCT_T {
    /*
     * u8              ucBssIndex;
     * u8              ucWlanIdx;
     * u16             u2TotalElementNum;
     * u32             u4Reserve;
     */
    /* extension */
    u16 u2Tag;  /* Tag = 0x05 */
    u16 u2Length;
    u8 aucMac[MAC_ADDR_LEN];
    u8 ucType;
    u8 ucWtbl;
    u8 ucOwnmac;
    u8 ucMode;
    u8 ucBw;
    u8 ucNss;
    u8 ucPfmuId;
    u8 ucMarate;
    u8 ucSpeIdx;
    u8 ucaid;
} CMD_MANUAL_ASSOC_STRUCT_T, *P_CMD_MANUAL_ASSOC_STRUCT_T;

typedef struct _TX_BF_SOUNDING_START_T {
    union {
        EXT_CMD_ETXBf_SND_PERIODIC_TRIGGER_CTRL_T
            rExtCmdExtBfSndPeriodicTriggerCtrl;
        EXT_CMD_ETXBf_MU_SND_PERIODIC_TRIGGER_CTRL_T
            rExtCmdExtBfMuSndPeriodicTriggerCtrl;
    } rTxBfSounding;
} TX_BF_SOUNDING_START_T, *P_TX_BF_SOUNDING_START_T;

typedef struct _TX_BF_SOUNDING_STOP_T {
    u8 ucTxBfCategory;
    u8 ucSndgStop;
    u8 ucReserved[2];
} TX_BF_SOUNDING_STOP_T, *P_TX_BF_SOUNDING_STOP_T;

typedef struct _TX_BF_TX_APPLY_T {
    u8 ucTxBfCategory;
    u8 ucWlanId;
    u8 fgETxBf;
    u8 fgITxBf;
    u8 fgMuTxBf;
    u8 ucReserved[3];
} TX_BF_TX_APPLY_T, *P_TX_BF_TX_APPLY_T;

typedef struct _TX_BF_PFMU_MEM_ALLOC_T {
    u8 ucTxBfCategory;
    u8 ucSuMuMode;
    u8 ucWlanIdx;
    u8 ucReserved;
} TX_BF_PFMU_MEM_ALLOC_T, *P_TX_BF_PFMU_MEM_ALLOC_T;

typedef struct _TX_BF_PFMU_MEM_RLS_T {
    u8 ucTxBfCategory;
    u8 ucWlanId;
    u8 ucReserved[2];
} TX_BF_PFMU_MEM_RLS_T, *P_TX_BF_PFMU_MEM_RLS_T;

typedef union _PARAM_CUSTOM_TXBF_ACTION_STRUCT_T {
    PROFILE_TAG_READ_T rProfileTagRead;
    PROFILE_TAG_WRITE_T rProfileTagWrite;
    PROFILE_DATA_READ_T rProfileDataRead;
    PROFILE_DATA_WRITE_T rProfileDataWrite;
    PROFILE_PN_READ_T rProfilePnRead;
    PROFILE_PN_WRITE_T rProfilePnWrite;
    TX_BF_SOUNDING_START_T rTxBfSoundingStart;
    TX_BF_SOUNDING_STOP_T rTxBfSoundingStop;
    TX_BF_TX_APPLY_T rTxBfTxApply;
    TX_BF_PFMU_MEM_ALLOC_T rTxBfPfmuMemAlloc;
    TX_BF_PFMU_MEM_RLS_T rTxBfPfmuMemRls;
} PARAM_CUSTOM_TXBF_ACTION_STRUCT_T, *P_PARAM_CUSTOM_TXBF_ACTION_STRUCT_T;

typedef struct _PARAM_CUSTOM_STA_REC_UPD_STRUCT_T {
    u8 ucBssIndex;
    u8 ucWlanIdx;
    u16 u2TotalElementNum;
    u8 ucAppendCmdTLV;
    u8 ucMuarIdx;
    u8 aucReserve[2];
    u32 *prStaRec;
    CMD_STAREC_BF rCmdStaRecBf;
} PARAM_CUSTOM_STA_REC_UPD_STRUCT_T, *P_PARAM_CUSTOM_STA_REC_UPD_STRUCT_T;

typedef struct _BSSINFO_ARGUMENT_T {
    u8 OwnMacIdx;
    u8 ucBssIndex;
    u8 Bssid[PARAM_MAC_ADDR_LEN];
    u8 ucBcMcWlanIdx;
    u8 ucPeerWlanIdx;
    u32 NetworkType;
    u32 u4ConnectionType;
    u8 CipherSuit;
    u8 Active;
    u8 WmmIdx;
    u32 u4BssInfoFeature;
    u8 aucBuffer[0];
} BSSINFO_ARGUMENT_T, *P_BSSINFO_ARGUMENT_T;

typedef struct _PARAM_CUSTOM_PFMU_TAG_READ_STRUCT_T {
    PFMU_PROFILE_TAG1 ru4TxBfPFMUTag1;
    PFMU_PROFILE_TAG2 ru4TxBfPFMUTag2;
} PARAM_CUSTOM_PFMU_TAG_READ_STRUCT_T, *P_PARAM_CUSTOM_PFMU_TAG_READ_STRUCT_T;

#if CFG_SUPPORT_MU_MIMO
typedef struct _PARAM_CUSTOM_SHOW_GROUP_TBL_ENTRY_STRUCT_T {
    u32 u4EventId;
    u8 index;
    u8 numUser : 2;
    u8 BW : 2;
    u8 NS0 : 2;
    u8 NS1 : 2;
    /* u8       NS2:1; */
    /* u8       NS3:1; */
    u8 PFIDUser0;
    u8 PFIDUser1;
    /* u8       PFIDUser2; */
    /* u8       PFIDUser3; */
    u8 fgIsShortGI;
    u8 fgIsUsed;
    u8 fgIsDisable;
    u8 initMcsUser0 : 4;
    u8 initMcsUser1 : 4;
    /* u8       initMcsUser2:4; */
    /* u8       initMcsUser3:4; */
    u8 dMcsUser0 : 4;
    u8 dMcsUser1 : 4;
    /* u8       dMcsUser2:4; */
    /* u8       dMcsUser3:4; */
} PARAM_CUSTOM_SHOW_GROUP_TBL_ENTRY_STRUCT_T,
*P_PARAM_CUSTOM_SHOW_GROUP_TBL_ENTRY_STRUCT_T;

typedef struct _PARAM_CUSTOM_GET_QD_STRUCT_T {
    u32 u4EventId;
    u32 au4RawData[14];
} PARAM_CUSTOM_GET_QD_STRUCT_T, *P_PARAM_CUSTOM_GET_QD_STRUCT_T;

typedef struct _MU_STRUCT_LQ_REPORT {
    int lq_report[NUM_OF_USER][NUM_OF_MODUL];
} MU_STRUCT_LQ_REPORT, *P_MU_STRUCT_LQ_REPORT;

typedef struct _PARAM_CUSTOM_GET_MU_CALC_LQ_STRUCT_T {
    u32 u4EventId;
    MU_STRUCT_LQ_REPORT rEntry;
} PARAM_CUSTOM_GET_MU_CALC_LQ_STRUCT_T, *P_PARAM_CUSTOM_GET_MU_CALC_LQ_STRUCT_T;

typedef struct _MU_GET_CALC_INIT_MCS_T {
    u8 ucgroupIdx;
    u8 ucRsv[3];
} MU_GET_CALC_INIT_MCS_T, *P_MU_GET_CALC_INIT_MCS_T;

typedef struct _MU_SET_INIT_MCS_T {
    u8 ucNumOfUser;  /* zero-base : 0~3: means 1~2 users */
    u8 ucBandwidth;  /* zero-base : 0:20 hz 1:40 hz 2: 80 hz 3: 160 */
    u8 ucNssOfUser0;  /* zero-base : 0~1 means uesr0 use 1~2 ss , if no use
                       * keep 0 */
    u8 ucNssOfUser1;  /* zero-base : 0~1 means uesr0 use 1~2 ss , if no use
                       * keep 0 */
    u8 ucPfMuIdOfUser0;  /* zero-base : for now, uesr0 use pf mu id 0 */
    u8 ucPfMuIdOfUser1;  /* zero-base : for now, uesr1 use pf mu id 1 */
    u8 ucNumOfTxer;  /* 0~3: mean use 1~4 anntain, for now, should fix 3 */
    u8 ucSpeIndex;  /*add new field to fill special extension index which
                     * replace reserve */
    u32 u4GroupIndex;  /* 0~ :the index of group table entry for calculation
                        */
} MU_SET_INIT_MCS_T, *P_MU_SET_INIT_MCS_T;

typedef struct _MU_SET_CALC_LQ_T {
    u8 ucNumOfUser;  /* zero-base : 0~3: means 1~2 users */
    u8 ucBandwidth;  /* zero-base : 0:20 hz 1:40 hz 2: 80 hz 3: 160 */
    u8 ucNssOfUser0;  /* zero-base : 0~1 means uesr0 use 1~2 ss , if no use
                       * keep 0 */
    u8 ucNssOfUser1;  /* zero-base : 0~1 means uesr0 use 1~2 ss , if no use
                       * keep 0 */
    u8 ucPfMuIdOfUser0;  /* zero-base : for now, uesr0 use pf mu id 0 */
    u8 ucPfMuIdOfUser1;  /* zero-base : for now, uesr1 use pf mu id 1 */
    u8 ucNumOfTxer;  /* 0~3: mean use 1~4 anntain, for now, should fix 3 */
    u8 ucSpeIndex;  /*add new field to fill special extension index which
                     * replace reserve */
    u32 u4GroupIndex;  /* 0~ :the index of group table entry for calculation
                        */
} MU_SET_CALC_LQ_T, *P_MU_SET_CALC_LQ_T;

typedef struct _MU_GET_LQ_T {
    u8 ucType;
    u8 ucRsv[3];
} MU_GET_LQ_T, *P_MU_GET_LQ_T;

typedef struct _MU_SET_SNR_OFFSET_T {
    u8 ucVal;
    u8 ucRsv[3];
} MU_SET_SNR_OFFSET_T, *P_MU_SET_SNR_OFFSET_T;

typedef struct _MU_SET_ZERO_NSS_T {
    u8 ucVal;
    u8 ucRsv[3];
} MU_SET_ZERO_NSS_T, *P_MU_SET_ZERO_NSS_T;

typedef struct _MU_SPEED_UP_LQ_T {
    u32 u4Val;
} MU_SPEED_UP_LQ_T, *P_MU_SPEED_UP_LQ_T;

typedef struct _MU_SET_MU_TABLE_T {
    /* u16  u2Type; */
    /* u32  u4Length; */
    u8 aucMetricTable[NUM_MUT_NR_NUM * NUM_MUT_FEC * NUM_MUT_MCS *
                      NUM_MUT_INDEX];
} MU_SET_MU_TABLE_T, *P_MU_SET_MU_TABLE_T;

typedef struct _MU_SET_GROUP_T {
    u32 u4GroupIndex;  /* Group Table Idx */
    u32 u4NumOfUser;
    u32 u4User0Ldpc;
    u32 u4User1Ldpc;
    u32 u4ShortGI;
    u32 u4Bw;
    u32 u4User0Nss;
    u32 u4User1Nss;
    u32 u4GroupId;
    u32 u4User0UP;
    u32 u4User1UP;
    u32 u4User0MuPfId;
    u32 u4User1MuPfId;
    u32 u4User0InitMCS;
    u32 u4User1InitMCS;
    u8 aucUser0MacAddr[PARAM_MAC_ADDR_LEN];
    u8 aucUser1MacAddr[PARAM_MAC_ADDR_LEN];
} MU_SET_GROUP_T, *P_MU_SET_GROUP_T;

typedef struct _MU_GET_QD_T {
    u8 ucSubcarrierIndex;
    /* u32 u4Length; */
    /* u8 *prQd; */
} MU_GET_QD_T, *P_MU_GET_QD_T;

typedef struct _MU_SET_ENABLE_T {
    u8 ucVal;
    u8 ucRsv[3];
} MU_SET_ENABLE_T, *P_MU_SET_ENABLE_T;

typedef struct _MU_SET_GID_UP_T {
    u32 au4Gid[2];
    u32 au4Up[4];
} MU_SET_GID_UP_T, *P_MU_SET_GID_UP_T;

typedef struct _MU_TRIGGER_MU_TX_T {
    u8 fgIsRandomPattern;  /* is random pattern or not */
    u32 u4MsduPayloadLength0;  /* payload length of the MSDU for user 0 */
    u32 u4MsduPayloadLength1;  /* payload length of the MSDU for user 1 */
    u32 u4MuPacketCount;  /* MU TX count */
    u32 u4NumOfSTAs;  /* number of user in the MU TX */
    u8 aucMacAddrs[2][6];  /* MAC address of users*/
} MU_TRIGGER_MU_TX_T, *P_MU_TRIGGER_MU_TX_T;

typedef struct _PARAM_CUSTOM_MUMIMO_ACTION_STRUCT_T {
    u8 ucMuMimoCategory;
    u8 aucRsv[3];
    union {
        MU_GET_CALC_INIT_MCS_T rMuGetCalcInitMcs;
        MU_SET_INIT_MCS_T rMuSetInitMcs;
        MU_SET_CALC_LQ_T rMuSetCalcLq;
        MU_GET_LQ_T rMuGetLq;
        MU_SET_SNR_OFFSET_T rMuSetSnrOffset;
        MU_SET_ZERO_NSS_T rMuSetZeroNss;
        MU_SPEED_UP_LQ_T rMuSpeedUpLq;
        MU_SET_MU_TABLE_T rMuSetMuTable;
        MU_SET_GROUP_T rMuSetGroup;
        MU_GET_QD_T rMuGetQd;
        MU_SET_ENABLE_T rMuSetEnable;
        MU_SET_GID_UP_T rMuSetGidUp;
        MU_TRIGGER_MU_TX_T rMuTriggerMuTx;
    } unMuMimoParam;
} PARAM_CUSTOM_MUMIMO_ACTION_STRUCT_T, *P_PARAM_CUSTOM_MUMIMO_ACTION_STRUCT_T;
#endif
#endif
#endif

typedef struct _PARAM_CUSTOM_MEM_DUMP_STRUCT_T {
    u32 u4Address;
    u32 u4Length;
    u32 u4RemainLength;
#if CFG_SUPPORT_QA_TOOL
    u32 u4IcapContent;
#endif
    u8 ucFragNum;
} PARAM_CUSTOM_MEM_DUMP_STRUCT_T, *P_PARAM_CUSTOM_MEM_DUMP_STRUCT_T;

typedef struct _PARAM_CUSTOM_SW_CTRL_STRUCT_T {
    u32 u4Id;
    s32 u4Data;
} PARAM_CUSTOM_SW_CTRL_STRUCT_T, *P_PARAM_CUSTOM_SW_CTRL_STRUCT_T;

typedef struct _PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T {
    u16 u2Id;
    u8 ucType;
    u8 ucRespType;
    u16 u2MsgSize;
    u8 aucReserved0[2];
    u8 aucCmd[CHIP_CONFIG_RESP_SIZE];
} PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T, *P_PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T;

typedef struct _PARAM_CUSTOM_KEY_CFG_STRUCT_T {
    u8 aucKey[WLAN_CFG_KEY_LEN_MAX];
    u8 aucValue[WLAN_CFG_VALUE_LEN_MAX];
} PARAM_CUSTOM_KEY_CFG_STRUCT_T, *P_PARAM_CUSTOM_KEY_CFG_STRUCT_T;

typedef struct _PARAM_CUSTOM_EEPROM_RW_STRUCT_T {
    u8 ucEepromMethod;  /* For read only read: 1, query size: 0 */
    u8 ucEepromIndex;
    u8 reserved;
    u16 u2EepromData;
} PARAM_CUSTOM_EEPROM_RW_STRUCT_T, *P_PARAM_CUSTOM_EEPROM_RW_STRUCT_T,
PARAM_CUSTOM_NVRAM_RW_STRUCT_T, *P_PARAM_CUSTOM_NVRAM_RW_STRUCT_T;

typedef struct _PARAM_CUSTOM_WMM_PS_TEST_STRUCT_T {
    u8 bmfgApsdEnAc;  /* b0~3: trigger-en AC0~3. b4~7: delivery-en AC0~3 */
    u8 ucIsEnterPsAtOnce;  /* enter PS immediately without 5 second guard
                            * after connected */
    u8 ucIsDisableUcTrigger;  /* not to trigger UC on beacon TIM is matched
                               * (under U-APSD) */
    u8 reserved;
} PARAM_CUSTOM_WMM_PS_TEST_STRUCT_T, *P_PARAM_CUSTOM_WMM_PS_TEST_STRUCT_T;

typedef struct _PARAM_CUSTOM_NOA_PARAM_STRUCT_T {
    u32 u4NoaDurationMs;
    u32 u4NoaIntervalMs;
    u32 u4NoaCount;
    u8 ucBssIdx;
} PARAM_CUSTOM_NOA_PARAM_STRUCT_T, *P_PARAM_CUSTOM_NOA_PARAM_STRUCT_T;

typedef struct _PARAM_CUSTOM_OPPPS_PARAM_STRUCT_T {
    u8 ucBssIdx;
    u8 ucLegcyPS;
    u8 ucOppPs;
    u8 aucResv[1];
    u32 u4CTwindowMs;
} PARAM_CUSTOM_OPPPS_PARAM_STRUCT_T, *P_PARAM_CUSTOM_OPPPS_PARAM_STRUCT_T;

typedef struct _PARAM_CUSTOM_UAPSD_PARAM_STRUCT_T {
    u8 ucBssIdx;
    u8 fgEnAPSD;
    u8 fgEnAPSD_AcBe;
    u8 fgEnAPSD_AcBk;
    u8 fgEnAPSD_AcVo;
    u8 fgEnAPSD_AcVi;
    u8 ucMaxSpLen;
    u8 aucResv[2];
} PARAM_CUSTOM_UAPSD_PARAM_STRUCT_T, *P_PARAM_CUSTOM_UAPSD_PARAM_STRUCT_T;

typedef struct _PARAM_CUSTOM_P2P_SET_STRUCT_T {
    u32 u4Enable;
    u32 u4Mode;
} PARAM_CUSTOM_P2P_SET_STRUCT_T, *P_PARAM_CUSTOM_P2P_SET_STRUCT_T;

#define MAX_NUMBER_OF_ACL    20

typedef enum _ENUM_PARAM_CUSTOM_ACL_POLICY_T {
    PARAM_CUSTOM_ACL_POLICY_DISABLE,
    PARAM_CUSTOM_ACL_POLICY_ACCEPT,
    PARAM_CUSTOM_ACL_POLICY_DENY,
    PARAM_CUSTOM_ACL_POLICY_NUM
} ENUM_PARAM_CUSTOM_ACL_POLICY_T,
*P_ENUM_PARAM_CUSTOM_ACL_POLICY_T;

typedef struct _PARAM_CUSTOM_ACL_ENTRY {
    u8 aucAddr[MAC_ADDR_LEN];
    u16 u2Rsv;
} PARAM_CUSTOM_ACL_ENTRY, *PPARAM_CUSTOM_ACL_ENTRY;

typedef struct _PARAM_CUSTOM_ACL {
    ENUM_PARAM_CUSTOM_ACL_POLICY_T ePolicy;
    u32 u4Num;
    PARAM_CUSTOM_ACL_ENTRY rEntry[MAX_NUMBER_OF_ACL];
} PARAM_CUSTOM_ACL, *PPARAM_CUSTOM_ACL;

typedef enum _ENUM_CFG_SRC_TYPE_T {
    CFG_SRC_TYPE_EEPROM,
    CFG_SRC_TYPE_NVRAM,
    CFG_SRC_TYPE_UNKNOWN,
    CFG_SRC_TYPE_NUM
} ENUM_CFG_SRC_TYPE_T,
*P_ENUM_CFG_SRC_TYPE_T;

typedef enum _ENUM_EEPROM_TYPE_T {
    EEPROM_TYPE_NO,
    EEPROM_TYPE_PRESENT,
    EEPROM_TYPE_NUM
} ENUM_EEPROM_TYPE_T,
*P_ENUM_EEPROM_TYPE_T;

typedef struct _PARAM_QOS_TSINFO {
    u8 ucTrafficType;  /* Traffic Type: 1 for isochronous 0 for asynchronous
                        */
    u8 ucTid;  /* TSID: must be between 8 ~ 15 */
    u8 ucDirection;  /* direction */
    u8 ucAccessPolicy;  /* access policy */
    u8 ucAggregation;  /* aggregation */
    u8 ucApsd;  /* APSD */
    u8 ucuserPriority;  /* user priority */
    u8 ucTsInfoAckPolicy;  /* TSINFO ACK policy */
    u8 ucSchedule;  /* Schedule */
} PARAM_QOS_TSINFO, *P_PARAM_QOS_TSINFO;

typedef struct _PARAM_QOS_TSPEC {
    PARAM_QOS_TSINFO rTsInfo;  /* TS info field */
    u16 u2NominalMSDUSize;  /* nominal MSDU size */
    u16 u2MaxMSDUsize;  /* maximum MSDU size */
    u32 u4MinSvcIntv;  /* minimum service interval */
    u32 u4MaxSvcIntv;  /* maximum service interval */
    u32 u4InactIntv;  /* inactivity interval */
    u32 u4SpsIntv;  /* suspension interval */
    u32 u4SvcStartTime;  /* service start time */
    u32 u4MinDataRate;  /* minimum Data rate */
    u32 u4MeanDataRate;  /* mean data rate */
    u32 u4PeakDataRate;  /* peak data rate */
    u32 u4MaxBurstSize;  /* maximum burst size */
    u32 u4DelayBound;  /* delay bound */
    u32 u4MinPHYRate;  /* minimum PHY rate */
    u16 u2Sba;  /* surplus bandwidth allowance */
    u16 u2MediumTime;  /* medium time */
} PARAM_QOS_TSPEC, *P_PARAM_QOS_TSPEC;

typedef struct _PARAM_QOS_ADDTS_REQ_INFO {
    PARAM_QOS_TSPEC rTspec;
} PARAM_QOS_ADDTS_REQ_INFO, *P_PARAM_QOS_ADDTS_REQ_INFO;

typedef struct _PARAM_VOIP_CONFIG {
    u32 u4VoipTrafficInterval;  /* 0: disable VOIP configuration */
} PARAM_VOIP_CONFIG, *P_PARAM_VOIP_CONFIG;

/*802.11 Statistics Struct*/
typedef struct _PARAM_802_11_STATISTICS_STRUCT_T {
    u32 u4Length;  /* Length of structure */
    LARGE_INTEGER rTransmittedFragmentCount;
    LARGE_INTEGER rMulticastTransmittedFrameCount;
    LARGE_INTEGER rFailedCount;
    LARGE_INTEGER rRetryCount;
    LARGE_INTEGER rMultipleRetryCount;
    LARGE_INTEGER rRTSSuccessCount;
    LARGE_INTEGER rRTSFailureCount;
    LARGE_INTEGER rACKFailureCount;
    LARGE_INTEGER rFrameDuplicateCount;
    LARGE_INTEGER rReceivedFragmentCount;
    LARGE_INTEGER rMulticastReceivedFrameCount;
    LARGE_INTEGER rFCSErrorCount;
    LARGE_INTEGER rTKIPLocalMICFailures;
    LARGE_INTEGER rTKIPICVErrors;
    LARGE_INTEGER rTKIPCounterMeasuresInvoked;
    LARGE_INTEGER rTKIPReplays;
    LARGE_INTEGER rCCMPFormatErrors;
    LARGE_INTEGER rCCMPReplays;
    LARGE_INTEGER rCCMPDecryptErrors;
    LARGE_INTEGER rFourWayHandshakeFailures;
    LARGE_INTEGER rWEPUndecryptableCount;
    LARGE_INTEGER rWEPICVErrorCount;
    LARGE_INTEGER rDecryptSuccessCount;
    LARGE_INTEGER rDecryptFailureCount;
} PARAM_802_11_STATISTICS_STRUCT_T, *P_PARAM_802_11_STATISTICS_STRUCT_T;

/* Linux Network Device Statistics Struct */
typedef struct _PARAM_LINUX_NETDEV_STATISTICS_T {
    u32 u4RxPackets;
    u32 u4TxPackets;
    u32 u4RxBytes;
    u32 u4TxBytes;
    u32 u4RxErrors;
    u32 u4TxErrors;
    u32 u4Multicast;
} PARAM_LINUX_NETDEV_STATISTICS_T, *P_PARAM_LINUX_NETDEV_STATISTICS_T;

typedef struct _PARAM_MTK_WIFI_TEST_STRUCT_T {
    u32 u4FuncIndex;
    u32 u4FuncData;
} PARAM_MTK_WIFI_TEST_STRUCT_T, *P_PARAM_MTK_WIFI_TEST_STRUCT_T;

/* 802.11 Media stream constraints */
typedef enum _ENUM_MEDIA_STREAM_MODE {
    ENUM_MEDIA_STREAM_OFF,
    ENUM_MEDIA_STREAM_ON
} ENUM_MEDIA_STREAM_MODE,
*P_ENUM_MEDIA_STREAM_MODE;

/* for NDIS 5.1 Media Streaming Change */
typedef struct _PARAM_MEDIA_STREAMING_INDICATION {
    PARAM_STATUS_INDICATION_T rStatus;
    ENUM_MEDIA_STREAM_MODE eMediaStreamMode;
} PARAM_MEDIA_STREAMING_INDICATION, *P_PARAM_MEDIA_STREAMING_INDICATION;

#define PARAM_PROTOCOL_ID_DEFAULT    0x00
#define PARAM_PROTOCOL_ID_TCP_IP     0x02
#define PARAM_PROTOCOL_ID_IPX        0x06
#define PARAM_PROTOCOL_ID_NBF        0x07
#define PARAM_PROTOCOL_ID_MAX        0x0F
#define PARAM_PROTOCOL_ID_MASK       0x0F

/* for NDIS OID_GEN_NETWORK_LAYER_ADDRESSES */
typedef struct _PARAM_NETWORK_ADDRESS_IP {
    u16 sin_port;
    u32 in_addr;
    u8 sin_zero[8];
} PARAM_NETWORK_ADDRESS_IP, *P_PARAM_NETWORK_ADDRESS_IP;

typedef struct _PARAM_NETWORK_ADDRESS {
    u16 u2AddressLength;  /* length in bytes of Address[] in this */
    u16 u2AddressType;  /* type of this address (PARAM_PROTOCOL_ID_XXX above)
                         */
    u8 aucAddress[1];  /* actually AddressLength bytes long */
} PARAM_NETWORK_ADDRESS, *P_PARAM_NETWORK_ADDRESS;

/* The following is used with OID_GEN_NETWORK_LAYER_ADDRESSES to set network
 * layer addresses on an interface */

typedef struct _PARAM_NETWORK_ADDRESS_LIST {
    u8 ucBssIdx;
    u32 u4AddressCount;  /* number of addresses following */
    u16 u2AddressType;  /* type of this address (NDIS_PROTOCOL_ID_XXX above)
                         */
    PARAM_NETWORK_ADDRESS
        arAddress[1];  /* actually AddressCount elements long */
} PARAM_NETWORK_ADDRESS_LIST, *P_PARAM_NETWORK_ADDRESS_LIST;

#if CFG_SUPPORT_MSP
/* Should by chip */
typedef struct _PARAM_SEC_CONFIG_T {
    u8 fgWPIFlag;
    u8 fgRV;
    u8 fgIKV;
    u8 fgRKV;

    u8 fgRCID;
    u8 fgRCA1;
    u8 fgRCA2;
    u8 fgEvenPN;

    u8 ucKeyID;
    u8 ucMUARIdx;
    u8 ucCipherSuit;
    u8 aucReserved[1];
} PARAM_SEC_CONFIG_T, *P_PARAM_SEC_CONFIG_T;

typedef struct _PARAM_TX_CONFIG_T {
    u8 aucPA[6];
    u8 fgSW;
    u8 fgDisRxHdrTran;

    u8 fgAADOM;
    u8 ucPFMUIdx;
    u16 u2PartialAID;

    u8 fgTIBF;
    u8 fgTEBF;
    u8 fgIsHT;
    u8 fgIsVHT;

    u8 fgMesh;
    u8 fgBAFEn;
    u8 fgCFAck;
    u8 fgRdgBA;

    u8 fgRDG;
    u8 fgIsPwrMgt;
    u8 fgRTS;
    u8 fgSMPS;

    u8 fgTxopPS;
    u8 fgDonotUpdateIPSM;
    u8 fgSkipTx;
    u8 fgLDPC;

    u8 fgIsQoS;
    u8 fgIsFromDS;
    u8 fgIsToDS;
    u8 fgDynBw;

    u8 fgIsAMSDUCrossLG;
    u8 fgCheckPER;
    u8 fgIsGID63;
    u8 aucReserved[1];

#if (1)
    u8 fgVhtTIBF;
    u8 fgVhtTEBF;
    u8 fgVhtLDPC;
    u8 aucReserved2[1];
#endif
} PARAM_TX_CONFIG_T, *P_PARAM_TX_CONFIG_T;

typedef struct _PARAM_KEY_CONFIG_T {
    u8 aucKey[32];
} PARAM_KEY_CONFIG_T, *P_PARAM_KEY_CONFIG_T;

typedef struct _PARAM_PEER_RATE_INFO_T {
    u8 ucCounterMPDUFail;
    u8 ucCounterMPDUTx;
    u8 ucRateIdx;
    u8 ucReserved[1];

    u16 au2RateCode[AUTO_RATE_NUM];
} PARAM_PEER_RATE_INFO_T, *P_PARAM_PEER_RATE_INFO_T;

typedef struct _PARAM_PEER_BA_CONFIG_T {
    u8 ucBaEn;
    u8 ucRsv[3];
    u32 u4BaWinSize;
} PARAM_PEER_BA_CONFIG_T, *P_PARAM_PEER_BA_CONFIG_T;

typedef struct _PARAM_ANT_ID_CONFIG_T {
    u8 ucANTIDSts0;
    u8 ucANTIDSts1;
    u8 ucANTIDSts2;
    u8 ucANTIDSts3;
} PARAM_ANT_ID_CONFIG_T, *P_PARAM_ANT_ID_CONFIG_T;

typedef struct _PARAM_PEER_CAP_T {
    PARAM_ANT_ID_CONFIG_T rAntIDConfig;

    u8 ucTxPowerOffset;
    u8 ucCounterBWSelector;
    u8 ucChangeBWAfterRateN;
    u8 ucFrequencyCapability;
    u8 ucSpatialExtensionIndex;

    u8 fgG2;
    u8 fgG4;
    u8 fgG8;
    u8 fgG16;

    u8 ucMMSS;
    u8 ucAmpduFactor;
    u8 ucReserved[1];
} PARAM_PEER_CAP_T, *P_PARAM_PEER_CAP_T;

typedef struct _PARAM_PEER_RX_COUNTER_ALL_T {
    u8 ucRxRcpi0;
    u8 ucRxRcpi1;
    u8 ucRxRcpi2;
    u8 ucRxRcpi3;

    u8 ucRxCC0;
    u8 ucRxCC1;
    u8 ucRxCC2;
    u8 ucRxCC3;

    u8 fgRxCCSel;
    u8 ucCeRmsd;
    u8 aucReserved[2];
} PARAM_PEER_RX_COUNTER_ALL_T, *P_PARAM_PEER_RX_COUNTER_ALL_T;

typedef struct _PARAM_PEER_TX_COUNTER_ALL_T {
    u16 u2Rate1TxCnt;
    u16 u2Rate1FailCnt;
    u16 u2Rate2OkCnt;
    u16 u2Rate3OkCnt;
    u16 u2CurBwTxCnt;
    u16 u2CurBwFailCnt;
    u16 u2OtherBwTxCnt;
    u16 u2OtherBwFailCnt;
} PARAM_PEER_TX_COUNTER_ALL_T, *P_PARAM_PEER_TX_COUNTER_ALL_T;

typedef struct _PARAM_HW_WLAN_INFO_T {
    u32 u4Index;
    PARAM_TX_CONFIG_T rWtblTxConfig;
    PARAM_SEC_CONFIG_T rWtblSecConfig;
    PARAM_KEY_CONFIG_T rWtblKeyConfig;
    PARAM_PEER_RATE_INFO_T rWtblRateInfo;
    PARAM_PEER_BA_CONFIG_T rWtblBaConfig;
    PARAM_PEER_CAP_T rWtblPeerCap;
    PARAM_PEER_RX_COUNTER_ALL_T rWtblRxCounter;
    PARAM_PEER_TX_COUNTER_ALL_T rWtblTxCounter;
} PARAM_HW_WLAN_INFO_T, *P_PARAM_HW_WLAN_INFO_T;

typedef struct _HW_TX_AMPDU_METRICS_T {
    u32 u4TxSfCnt;
    u32 u4TxAckSfCnt;
    u32 u2TxAmpduCnt;
    u32 u2TxRspBaCnt;
    u16 u2TxEarlyStopCnt;
    u16 u2TxRange1AmpduCnt;
    u16 u2TxRange2AmpduCnt;
    u16 u2TxRange3AmpduCnt;
    u16 u2TxRange4AmpduCnt;
    u16 u2TxRange5AmpduCnt;
    u16 u2TxRange6AmpduCnt;
    u16 u2TxRange7AmpduCnt;
    u16 u2TxRange8AmpduCnt;
    u16 u2TxRange9AmpduCnt;
} HW_TX_AMPDU_METRICS_T, *P_HW_TX_AMPDU_METRICS_T;

typedef struct _HW_MIB_COUNTER_T {
    u32 u4RxFcsErrCnt;
    u32 u4RxFifoFullCnt;
    u32 u4RxMpduCnt;
    u32 u4RxAMPDUCnt;
    u32 u4RxTotalByte;
    u32 u4RxValidAMPDUSF;
    u32 u4RxValidByte;
    u32 u4ChannelIdleCnt;
    u32 u4RxVectorDropCnt;
    u32 u4DelimiterFailedCnt;
    u32 u4RxVectorMismatchCnt;
    u32 u4MdrdyCnt;
    u32 u4CCKMdrdyCnt;
    u32 u4OFDMLGMixMdrdy;
    u32 u4OFDMGreenMdrdy;
    u32 u4PFDropCnt;
    u32 u4RxLenMismatchCnt;
    u32 u4PCcaTime;
    u32 u4SCcaTime;
    u32 u4CcaNavTx;
    u32 u4PEDTime;
    u32 u4BeaconTxCnt;
    u32 au4BaMissedCnt[BSSID_NUM];
    u32 au4RtsTxCnt[BSSID_NUM];
    u32 au4FrameRetryCnt[BSSID_NUM];
    u32 au4FrameRetry2Cnt[BSSID_NUM];
    u32 au4RtsRetryCnt[BSSID_NUM];
    u32 au4AckFailedCnt[BSSID_NUM];
} HW_MIB_COUNTER_T, *P_HW_MIB_COUNTER_T;

typedef struct _HW_MIB2_COUNTER_T {
    u32 u4Tx40MHzCnt;
    u32 u4Tx80MHzCnt;
    u32 u4Tx160MHzCnt;
} HW_MIB2_COUNTER_T, *P_HW_MIB2_COUNTER_T;

typedef struct _PARAM_HW_MIB_INFO_T {
    u32 u4Index;
    HW_MIB_COUNTER_T rHwMibCnt;
    HW_MIB2_COUNTER_T rHwMib2Cnt;
    HW_TX_AMPDU_METRICS_T rHwTxAmpduMts;
} PARAM_HW_MIB_INFO_T, *P_PARAM_HW_MIB_INFO_T;
#endif

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
struct PARAM_TX_MCS_INFO {
    u8 ucStaIndex;
    u16 au2TxRateCode[MCS_INFO_SAMPLE_CNT];
    u8 aucTxRatePer[MCS_INFO_SAMPLE_CNT];
};
#endif

#ifdef CFG_DUMP_TXPOWR_TABLE
struct PARAM_CMD_GET_TXPWR_TBL {
    u8 ucDbdcIdx;
    u8 ucCenterCh;
    u8 ucFeLoss;
    struct POWER_LIMIT tx_pwr_tbl[TXPWR_TBL_NUM];
};

enum ENUM_TXPWR_TYPE {
    DSSS = 0,
    OFDM_24G,
    OFDM_5G,
    HT20,
    HT40,
    VHT20,
    VHT40,
    VHT80,
    TXPWR_TYPE_NUM,
};

enum ENUM_STREAM_MODE {
    STREAM_SISO,
    STREAM_CDD,
    STREAM_MIMO,
    STREAM_NUM
};

struct txpwr_table_entry {
    char mcs[STREAM_NUM][8];
    unsigned int idx;
};

struct txpwr_table {
    char phy_mode[8];
    struct txpwr_table_entry *tables;
    int n_tables;
};
#endif

/*--------------------------------------------------------------*/
/*! \brief For Fixed Rate Configuration (Registry)              */
/*--------------------------------------------------------------*/
typedef enum _ENUM_REGISTRY_FIXED_RATE_T {
    FIXED_RATE_NONE,
    FIXED_RATE_1M,
    FIXED_RATE_2M,
    FIXED_RATE_5_5M,
    FIXED_RATE_11M,
    FIXED_RATE_6M,
    FIXED_RATE_9M,
    FIXED_RATE_12M,
    FIXED_RATE_18M,
    FIXED_RATE_24M,
    FIXED_RATE_36M,
    FIXED_RATE_48M,
    FIXED_RATE_54M,
    FIXED_RATE_MCS0_20M_800NS,
    FIXED_RATE_MCS1_20M_800NS,
    FIXED_RATE_MCS2_20M_800NS,
    FIXED_RATE_MCS3_20M_800NS,
    FIXED_RATE_MCS4_20M_800NS,
    FIXED_RATE_MCS5_20M_800NS,
    FIXED_RATE_MCS6_20M_800NS,
    FIXED_RATE_MCS7_20M_800NS,
    FIXED_RATE_MCS0_20M_400NS,
    FIXED_RATE_MCS1_20M_400NS,
    FIXED_RATE_MCS2_20M_400NS,
    FIXED_RATE_MCS3_20M_400NS,
    FIXED_RATE_MCS4_20M_400NS,
    FIXED_RATE_MCS5_20M_400NS,
    FIXED_RATE_MCS6_20M_400NS,
    FIXED_RATE_MCS7_20M_400NS,
    FIXED_RATE_MCS0_40M_800NS,
    FIXED_RATE_MCS1_40M_800NS,
    FIXED_RATE_MCS2_40M_800NS,
    FIXED_RATE_MCS3_40M_800NS,
    FIXED_RATE_MCS4_40M_800NS,
    FIXED_RATE_MCS5_40M_800NS,
    FIXED_RATE_MCS6_40M_800NS,
    FIXED_RATE_MCS7_40M_800NS,
    FIXED_RATE_MCS32_800NS,
    FIXED_RATE_MCS0_40M_400NS,
    FIXED_RATE_MCS1_40M_400NS,
    FIXED_RATE_MCS2_40M_400NS,
    FIXED_RATE_MCS3_40M_400NS,
    FIXED_RATE_MCS4_40M_400NS,
    FIXED_RATE_MCS5_40M_400NS,
    FIXED_RATE_MCS6_40M_400NS,
    FIXED_RATE_MCS7_40M_400NS,
    FIXED_RATE_MCS32_400NS,
    FIXED_RATE_NUM
} ENUM_REGISTRY_FIXED_RATE_T,
*P_ENUM_REGISTRY_FIXED_RATE_T;

typedef enum _ENUM_BT_CMD_T {
    BT_CMD_PROFILE = 0,
    BT_CMD_UPDATE,
    BT_CMD_NUM
} ENUM_BT_CMD_T;

typedef enum _ENUM_BT_PROFILE_T {
    BT_PROFILE_CUSTOM = 0,
    BT_PROFILE_SCO,
    BT_PROFILE_ACL,
    BT_PROFILE_MIXED,
    BT_PROFILE_NO_CONNECTION,
    BT_PROFILE_NUM
} ENUM_BT_PROFILE_T;

typedef struct _PTA_PROFILE_T {
    ENUM_BT_PROFILE_T eBtProfile;
    union {
        u8 aucBTPParams[BT_PROFILE_PARAM_LEN];
        /*  0: sco reserved slot time,
         *  1: sco idle slot time,
         *  2: acl throughput,
         *   3: bt tx power,
         *   4: bt rssi
         *   5: VoIP interval
         *   6: BIT(0) Use this field, BIT(1) 0 apply single/ 1 dual PTA
         * setting.
         */
        u32 au4Btcr[4];
    } u;
} PTA_PROFILE_T, *P_PTA_PROFILE_T;

typedef struct _PTA_IPC_T {
    u8 ucCmd;
    u8 ucLen;
    union {
        PTA_PROFILE_T rProfile;
        u8 aucBTPParams[BT_PROFILE_PARAM_LEN];
    } u;
} PARAM_PTA_IPC_T, *P_PARAM_PTA_IPC_T, PTA_IPC_T, *P_PTA_IPC_T;

/*--------------------------------------------------------------*/
/*! \brief CFG80211 Scan Request Container                      */
/*--------------------------------------------------------------*/
typedef struct _PARAM_SCAN_REQUEST_EXT_T {
    PARAM_SSID_T rSsid;
    u32 u4IELength;
    u8 *pucIE;
} PARAM_SCAN_REQUEST_EXT_T, *P_PARAM_SCAN_REQUEST_EXT_T;

typedef struct _PARAM_SCAN_REQUEST_ADV_T {
    u32 u4SsidNum;
    PARAM_SSID_T rSsid[CFG_SCAN_SSID_MAX_NUM];
    u32 u4IELength;
    u8 *pucIE;
#if CFG_SCAN_CHANNEL_SPECIFIED
    u8 ucChannelListNum;
    RF_CHANNEL_INFO_T arChnlInfoList[MAXIMUM_OPERATION_CHANNEL_LIST];
#endif
} PARAM_SCAN_REQUEST_ADV_T, *P_PARAM_SCAN_REQUEST_ADV_T;

/*--------------------------------------------------------------*/
/*! \brief CFG80211 Scheduled Scan Request Container            */
/*--------------------------------------------------------------*/
typedef struct _PARAM_SCHED_SCAN_REQUEST_T {
    u32 u4SsidNum;
    PARAM_SSID_T arSsid[CFG_SCAN_SSID_MATCH_MAX_NUM];
    u32 u4IELength;
    u8 *pucIE;
    u16 u2ScanInterval;  /* in milliseconds */
} PARAM_SCHED_SCAN_REQUEST, *P_PARAM_SCHED_SCAN_REQUEST;

#if CFG_SUPPORT_SNIFFER
typedef struct _PARAM_CUSTOM_MONITOR_SET_STRUCT_T {
    u8 ucEnable;
    u8 ucBand;
    u8 ucPriChannel;
    u8 ucSco;
    u8 ucChannelWidth;
    u8 ucChannelS1;
    u8 ucChannelS2;
    u8 aucResv[9];
} PARAM_CUSTOM_MONITOR_SET_STRUCT_T, *P_PARAM_CUSTOM_MONITOR_SET_STRUCT_T;
#endif

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
/*--------------------------------------------------------------*/
/*! \brief MTK Auto Channel Selection related Container         */
/*--------------------------------------------------------------*/
typedef struct _LTE_SAFE_CHN_INFO_T {
    u32 au4SafeChannelBitmask[5];  /* NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_MAX
                                    */
} LTE_SAFE_CHN_INFO_T, *P_CMD_LTE_SAFE_CHN_INFO_T;

typedef struct _PARAM_CHN_LOAD_INFO {
    /* Per-CHN Load */
    u8 ucChannel;
    u16 u2APNum;
    u32 u4Dirtiness;
    u8 ucReserved;
} PARAM_CHN_LOAD_INFO, *P_PARAM_CHN_LOAD_INFO;

typedef struct _PARAM_CHN_RANK_INFO {
    u8 ucChannel;
    u32 u4Dirtiness;
    u8 ucReserved;
} PARAM_CHN_RANK_INFO, *P_PARAM_CHN_RANK_INFO;

typedef struct _PARAM_GET_CHN_INFO {
    LTE_SAFE_CHN_INFO_T rLteSafeChnList;
    PARAM_CHN_LOAD_INFO rEachChnLoad[MAX_CHN_NUM];
    u8 fgDataReadyBit;
    PARAM_CHN_RANK_INFO rChnRankList[MAX_CHN_NUM];
    u8 aucReserved[3];
} PARAM_GET_CHN_INFO, *P_PARAM_GET_CHN_INFO;

typedef struct _PARAM_PREFER_CHN_INFO {
    u8 ucChannel;
    u16 u2APNumScore;
    u8 ucReserved;
} PARAM_PREFER_CHN_INFO, *P_PARAM_PREFER_CHN_INFO;
#endif

typedef struct _UMAC_STAT2_GET_T {
    u16 u2PleRevPgHif0Group0;
    u16 u2PleRevPgCpuGroup2;

    u16 u2PseRevPgHif0Group0;
    u16 u2PseRevPgHif1Group1;
    u16 u2PseRevPgCpuGroup2;
    u16 u2PseRevPgLmac0Group3;
    u16 u2PseRevPgLmac1Group4;
    u16 u2PseRevPgLmac2Group5;
    u16 u2PseRevPgPleGroup6;

    u16 u2PleSrvPgHif0Group0;
    u16 u2PleSrvPgCpuGroup2;

    u16 u2PseSrvPgHif0Group0;
    u16 u2PseSrvPgHif1Group1;
    u16 u2PseSrvPgCpuGroup2;
    u16 u2PseSrvPgLmac0Group3;
    u16 u2PseSrvPgLmac1Group4;
    u16 u2PseSrvPgLmac2Group5;
    u16 u2PseSrvPgPleGroup6;

    u16 u2PleTotalPageNum;
    u16 u2PleFreePageNum;
    u16 u2PleFfaNum;

    u16 u2PseTotalPageNum;
    u16 u2PseFreePageNum;
    u16 u2PseFfaNum;
} UMAC_STAT2_GET_T, *P_UMAC_STAT2_GET_T;

typedef struct _CNM_STATUS_T {
    u8 fgDbDcModeEn;
    u8 ucChNumB0;
    u8 ucChNumB1;
    u8 usReserved;
} CNM_STATUS_T, *P_CNM_STATUS_T;

typedef struct _CNM_CH_LIST_T {
    u8 ucChNum[4];
} CNM_CH_LIST_T, *P_CNM_CH_LIST_T;

typedef struct _PARAM_WDEV_LOCK_THREAD_T {
    QUE_ENTRY_T rQueEntry;
    struct net_device *pDev;
    enum ENUM_CFG80211_WDEV_LOCK_FUNC fn;
    u8 *pFrameBuf;
    size_t frameLen;
    struct cfg80211_bss *pBss;
    u32 uapsd_queues;
    u8 fgIsInterruptContext;
    const u8 *req_ies;
    size_t req_ies_len;
} PARAM_WDEV_LOCK_THREAD_T, *P_PARAM_WDEV_LOCK_THREAD_T;

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

/*--------------------------------------------------------------*/
/* Routines to set parameters or query information.             */
/*--------------------------------------------------------------*/
/***** Routines in wlan_oid.c *****/
WLAN_STATUS
wlanoidQueryNetworkTypesSupported(IN P_ADAPTER_T prAdapter,
                                  OUT void *pvQueryBuffer,
                                  IN u32 u4QueryBufferLen,
                                  OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryNetworkTypeInUse(IN P_ADAPTER_T prAdapter,
                             OUT void *pvQueryBuffer,
                             IN u32 u4QueryBufferLen,
                             OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetNetworkTypeInUse(IN P_ADAPTER_T prAdapter,
                           IN void *pvSetBuffer,
                           IN u32 u4SetBufferLen,
                           OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryBssid(IN P_ADAPTER_T prAdapter,
                  OUT void *pvQueryBuffer,
                  IN u32 u4QueryBufferLen,
                  OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetBssidListScan(IN P_ADAPTER_T prAdapter,
                        IN void *pvSetBuffer,
                        IN u32 u4SetBufferLen,
                        OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetBssidListScanExt(IN P_ADAPTER_T prAdapter,
                           IN void *pvSetBuffer,
                           IN u32 u4SetBufferLen,
                           OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetBssidListScanAdv(IN P_ADAPTER_T prAdapter,
                           IN void *pvSetBuffer,
                           IN u32 u4SetBufferLen,
                           OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryBssidList(IN P_ADAPTER_T prAdapter,
                      OUT void *pvQueryBuffer,
                      IN u32 u4QueryBufferLen,
                      OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetBssid(IN P_ADAPTER_T prAdapter,
                IN void *pvSetBuffer,
                IN u32 u4SetBufferLen,
                OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetConnect(IN P_ADAPTER_T prAdapter,
                  IN void *pvSetBuffer,
                  IN u32 u4SetBufferLen,
                  OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetSsid(IN P_ADAPTER_T prAdapter,
               IN void *pvSetBuffer,
               IN u32 u4SetBufferLen,
               OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQuerySsid(IN P_ADAPTER_T prAdapter,
                 OUT void *pvQueryBuffer,
                 IN u32 u4QueryBufferLen,
                 OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryInfrastructureMode(IN P_ADAPTER_T prAdapter,
                               OUT void *pvQueryBuffer,
                               IN u32 u4QueryBufferLen,
                               OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetInfrastructureMode(IN P_ADAPTER_T prAdapter,
                             IN void *pvSetBuffer,
                             IN u32 u4SetBufferLen,
                             OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryAuthMode(IN P_ADAPTER_T prAdapter,
                     OUT void *pvQueryBuffer,
                     IN u32 u4QueryBufferLen,
                     OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetAuthMode(IN P_ADAPTER_T prAdapter,
                   IN void *pvSetBuffer,
                   IN u32 u4SetBufferLen,
                   OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetEncryptionStatus(IN P_ADAPTER_T prAdapter,
                           IN void *pvSetBuffer,
                           IN u32 u4SetBufferLen,
                           OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryEncryptionStatus(IN P_ADAPTER_T prAdapter,
                             IN void *pvQueryBuffer,
                             IN u32 u4QueryBufferLen,
                             OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetAddWep(IN P_ADAPTER_T prAdapter,
                 IN void *pvSetBuffer,
                 IN u32 u4SetBufferLen,
                 OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetRemoveWep(IN P_ADAPTER_T prAdapter,
                    IN void *pvSetBuffer,
                    IN u32 u4SetBufferLen,
                    OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetAddKey(IN P_ADAPTER_T prAdapter,
                 IN void *pvSetBuffer,
                 IN u32 u4SetBufferLen,
                 OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetRemoveKey(IN P_ADAPTER_T prAdapter,
                    IN void *pvSetBuffer,
                    IN u32 u4SetBufferLen,
                    OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetReloadDefaults(IN P_ADAPTER_T prAdapter,
                         IN void *pvSetBuffer,
                         IN u32 u4SetBufferLen,
                         OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetTest(IN P_ADAPTER_T prAdapter,
               IN void *pvSetBuffer,
               IN u32 u4SetBufferLen,
               OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryCapability(IN P_ADAPTER_T prAdapter,
                       OUT void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen,
                       OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryFrequency(IN P_ADAPTER_T prAdapter,
                      OUT void *pvQueryBuffer,
                      IN u32 u4QueryBufferLen,
                      OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetFrequency(IN P_ADAPTER_T prAdapter,
                    IN void *pvSetBuffer,
                    IN u32 u4SetBufferLen,
                    OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryAtimWindow(IN P_ADAPTER_T prAdapter,
                       OUT void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen,
                       OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetAtimWindow(IN P_ADAPTER_T prAdapter,
                     IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen,
                     OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetChannel(IN P_ADAPTER_T prAdapter,
                  IN void *pvSetBuffer,
                  IN u32 u4SetBufferLen,
                  OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryRssi(IN P_ADAPTER_T prAdapter,
                 OUT void *pvQueryBuffer,
                 IN u32 u4QueryBufferLen,
                 OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryRssiTrigger(IN P_ADAPTER_T prAdapter,
                        OUT void *pvQueryBuffer,
                        IN u32 u4QueryBufferLen,
                        OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetRssiTrigger(IN P_ADAPTER_T prAdapter,
                      IN void *pvSetBuffer,
                      IN u32 u4SetBufferLen,
                      OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryRtsThreshold(IN P_ADAPTER_T prAdapter,
                         OUT void *pvQueryBuffer,
                         IN u32 u4QueryBufferLen,
                         OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetRtsThreshold(IN P_ADAPTER_T prAdapter,
                       IN void *pvSetBuffer,
                       IN u32 u4SetBufferLen,
                       OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQuery802dot11PowerSaveProfile(IN P_ADAPTER_T prAdapter,
                                     IN void *pvQueryBuffer,
                                     IN u32 u4QueryBufferLen,
                                     OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSet802dot11PowerSaveProfile(IN P_ADAPTER_T prAdapter,
                                   IN void *prSetBuffer,
                                   IN u32 u4SetBufferLen,
                                   OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryPmkid(IN P_ADAPTER_T prAdapter,
                  OUT void *pvQueryBuffer,
                  IN u32 u4QueryBufferLen,
                  OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetPmkid(IN P_ADAPTER_T prAdapter,
                IN void *pvSetBuffer,
                IN u32 u4SetBufferLen,
                OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQuerySupportedRates(IN P_ADAPTER_T prAdapter,
                           OUT void *pvQueryBuffer,
                           IN u32 u4QueryBufferLen,
                           OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryDesiredRates(IN P_ADAPTER_T prAdapter,
                         OUT void *pvQueryBuffer,
                         IN u32 u4QueryBufferLen,
                         OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetDesiredRates(IN P_ADAPTER_T prAdapter,
                       IN void *pvSetBuffer,
                       IN u32 u4SetBufferLen,
                       OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryPermanentAddr(IN P_ADAPTER_T prAdapter,
                          IN void *pvQueryBuf,
                          IN u32 u4QueryBufLen,
                          OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryCurrentAddr(IN P_ADAPTER_T prAdapter,
                        IN void *pvQueryBuf,
                        IN u32 u4QueryBufLen,
                        OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryPermanentAddr(IN P_ADAPTER_T prAdapter,
                          IN void *pvQueryBuf,
                          IN u32 u4QueryBufLen,
                          OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryLinkSpeed(IN P_ADAPTER_T prAdapter,
                      IN void *pvQueryBuffer,
                      IN u32 u4QueryBufferLen,
                      OUT u32 *pu4QueryInfoLen);

#if CFG_SUPPORT_QA_TOOL
#if CFG_SUPPORT_BUFFER_MODE
WLAN_STATUS wlanoidSetEfusBufferMode(IN P_ADAPTER_T prAdapter,
                                     IN void *pvSetBuffer,
                                     IN u32 u4SetBufferLen,
                                     OUT u32 *pu4SetInfoLen);

/*#if (CFG_EEPROM_PAGE_ACCESS == 1)*/
WLAN_STATUS
wlanoidQueryProcessAccessEfuseRead(IN P_ADAPTER_T prAdapter,
                                   IN void *pvSetBuffer,
                                   IN u32 u4SetBufferLen,
                                   OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryProcessAccessEfuseWrite(IN P_ADAPTER_T prAdapter,
                                    IN void *pvSetBuffer,
                                    IN u32 u4SetBufferLen,
                                    OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryEfuseFreeBlock(IN P_ADAPTER_T prAdapter,
                           IN void *pvSetBuffer,
                           IN u32 u4SetBufferLen,
                           OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryGetTxPower(IN P_ADAPTER_T prAdapter,
                       IN void *pvSetBuffer,
                       IN u32 u4SetBufferLen,
                       OUT u32 *pu4SetInfoLen);
/*#endif*/

#endif
WLAN_STATUS
wlanoidQueryRxStatistics(IN P_ADAPTER_T prAdapter,
                         IN void *pvQueryBuffer,
                         IN u32 u4QueryBufferLen,
                         OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidBssInfoBasic(IN P_ADAPTER_T prAdapter,
                    IN void *pvSetBuffer,
                    IN u32 u4SetBufferLen,
                    OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidDevInfoActive(IN P_ADAPTER_T prAdapter,
                     IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen,
                     OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidManualAssoc(IN P_ADAPTER_T prAdapter,
                   IN void *pvSetBuffer,
                   IN u32 u4SetBufferLen,
                   OUT u32 *pu4SetInfoLen);

#if CFG_SUPPORT_TX_BF
WLAN_STATUS
wlanoidTxBfAction(IN P_ADAPTER_T prAdapter,
                  IN void *pvSetBuffer,
                  IN u32 u4SetBufferLen,
                  OUT u32 *pu4SetInfoLen);
WLAN_STATUS wlanoidMuMimoAction(IN P_ADAPTER_T prAdapter,
                                IN void *pvSetBuffer,
                                IN u32 u4SetBufferLen,
                                OUT u32 *pu4SetInfoLen);
WLAN_STATUS wlanoidStaRecUpdate(IN P_ADAPTER_T prAdapter,
                                IN void *pvSetBuffer,
                                IN u32 u4SetBufferLen,
                                OUT u32 *pu4SetInfoLen);
WLAN_STATUS wlanoidStaRecBFUpdate(IN P_ADAPTER_T prAdapter,
                                  IN void *pvSetBuffer,
                                  IN u32 u4SetBufferLen,
                                  OUT u32 *pu4SetInfoLen);
#endif
#endif

WLAN_STATUS
wlanoidQueryMcrRead(IN P_ADAPTER_T prAdapter,
                    IN void *pvQueryBuffer,
                    IN u32 u4QueryBufferLen,
                    OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryMemDump(IN P_ADAPTER_T prAdapter,
                    IN void *pvQueryBuffer,
                    IN u32 u4QueryBufferLen,
                    OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetMcrWrite(IN P_ADAPTER_T prAdapter,
                   IN void *pvSetBuffer,
                   IN u32 u4SetBufferLen,
                   OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryDrvMcrRead(IN P_ADAPTER_T prAdapter,
                       IN void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen,
                       OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetDrvMcrWrite(IN P_ADAPTER_T prAdapter,
                      IN void *pvSetBuffer,
                      IN u32 u4SetBufferLen,
                      OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQuerySwCtrlRead(IN P_ADAPTER_T prAdapter,
                       IN void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen,
                       OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetSwCtrlWrite(IN P_ADAPTER_T prAdapter,
                      IN void *pvSetBuffer,
                      IN u32 u4SetBufferLen,
                      OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetChipConfig(IN P_ADAPTER_T prAdapter,
                     IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen,
                     OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryChipConfig(IN P_ADAPTER_T prAdapter,
                       IN void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen,
                       OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetKeyCfg(IN P_ADAPTER_T prAdapter,
                 IN void *pvSetBuffer,
                 IN u32 u4SetBufferLen,
                 OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryEepromRead(IN P_ADAPTER_T prAdapter,
                       IN void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen,
                       OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetEepromWrite(IN P_ADAPTER_T prAdapter,
                      IN void *pvSetBuffer,
                      IN u32 u4SetBufferLen,
                      OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryRfTestRxStatus(IN P_ADAPTER_T prAdapter,
                           IN void *pvQueryBuffer,
                           IN u32 u4QueryBufferLen,
                           OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryRfTestTxStatus(IN P_ADAPTER_T prAdapter,
                           IN void *pvQueryBuffer,
                           IN u32 u4QueryBufferLen,
                           OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryOidInterfaceVersion(IN P_ADAPTER_T prAdapter,
                                IN void *pvQueryBuffer,
                                IN u32 u4QueryBufferLen,
                                OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryVendorId(IN P_ADAPTER_T prAdapter,
                     OUT void *pvQueryBuffer,
                     IN u32 u4QueryBufferLen,
                     OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryMulticastList(IN P_ADAPTER_T prAdapter,
                          OUT void *pvQueryBuffer,
                          IN u32 u4QueryBufferLen,
                          OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetMulticastList(IN P_ADAPTER_T prAdapter,
                        IN void *pvSetBuffer,
                        IN u32 u4SetBufferLen,
                        OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryRcvError(IN P_ADAPTER_T prAdapter,
                     IN void *pvQueryBuffer,
                     IN u32 u4QueryBufferLen,
                     OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryRcvNoBuffer(IN P_ADAPTER_T prAdapter,
                        IN void *pvQueryBuffer,
                        IN u32 u4QueryBufferLen,
                        OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryRcvCrcError(IN P_ADAPTER_T prAdapter,
                        IN void *pvQueryBuffer,
                        IN u32 u4QueryBufferLen,
                        OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryStatistics(IN P_ADAPTER_T prAdapter,
                       IN void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen,
                       OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryCoexIso(IN P_ADAPTER_T prAdapter,
                    IN void *pvQueryBuffer,
                    IN u32 u4QueryBufferLen,
                    OUT u32 *pu4QueryInfoLen);

#ifdef LINUX

WLAN_STATUS
wlanoidQueryStatisticsForLinux(IN P_ADAPTER_T prAdapter,
                               IN void *pvQueryBuffer,
                               IN u32 u4QueryBufferLen,
                               OUT u32 *pu4QueryInfoLen);

#endif

WLAN_STATUS
wlanoidQueryMediaStreamMode(IN P_ADAPTER_T prAdapter,
                            IN void *pvQueryBuffer,
                            IN u32 u4QueryBufferLen,
                            OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetMediaStreamMode(IN P_ADAPTER_T prAdapter,
                          IN void *pvSetBuffer,
                          IN u32 u4SetBufferLen,
                          OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryRcvOk(IN P_ADAPTER_T prAdapter,
                  IN void *pvQueryBuffer,
                  IN u32 u4QueryBufferLen,
                  OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryXmitOk(IN P_ADAPTER_T prAdapter,
                   IN void *pvQueryBuffer,
                   IN u32 u4QueryBufferLen,
                   OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryXmitError(IN P_ADAPTER_T prAdapter,
                      IN void *pvQueryBuffer,
                      IN u32 u4QueryBufferLen,
                      OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryXmitOneCollision(IN P_ADAPTER_T prAdapter,
                             IN void *pvQueryBuffer,
                             IN u32 u4QueryBufferLen,
                             OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryXmitMoreCollisions(IN P_ADAPTER_T prAdapter,
                               IN void *pvQueryBuffer,
                               IN u32 u4QueryBufferLen,
                               OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryXmitMaxCollisions(IN P_ADAPTER_T prAdapter,
                              IN void *pvQueryBuffer,
                              IN u32 u4QueryBufferLen,
                              OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetCurrentPacketFilter(IN P_ADAPTER_T prAdapter,
                              IN void *pvSetBuffer,
                              IN u32 u4SetBufferLen,
                              OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryCurrentPacketFilter(IN P_ADAPTER_T prAdapter,
                                IN void *pvQueryBuffer,
                                IN u32 u4QueryBufferLen,
                                OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetAcpiDevicePowerState(IN P_ADAPTER_T prAdapter,
                               IN void *pvSetBuffer,
                               IN u32 u4SetBufferLen,
                               OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryAcpiDevicePowerState(IN P_ADAPTER_T prAdapter,
                                 IN void *pvQueryBuffer,
                                 IN u32 u4QueryBufferLen,
                                 OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetDisassociate(IN P_ADAPTER_T prAdapter,
                       IN void *pvSetBuffer,
                       IN u32 u4SetBufferLen,
                       OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryFragThreshold(IN P_ADAPTER_T prAdapter,
                          OUT void *pvQueryBuffer,
                          IN u32 u4QueryBufferLen,
                          OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetFragThreshold(IN P_ADAPTER_T prAdapter,
                        IN void *pvSetBuffer,
                        IN u32 u4SetBufferLen,
                        OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryAdHocMode(IN P_ADAPTER_T prAdapter,
                      OUT void *pvQueryBuffer,
                      IN u32 u4QueryBufferLen,
                      OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetAdHocMode(IN P_ADAPTER_T prAdapter,
                    IN void *pvSetBuffer,
                    IN u32 u4SetBufferLen,
                    OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryBeaconInterval(IN P_ADAPTER_T prAdapter,
                           OUT void *pvQueryBuffer,
                           IN u32 u4QueryBufferLen,
                           OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetBeaconInterval(IN P_ADAPTER_T prAdapter,
                         IN void *pvSetBuffer,
                         IN u32 u4SetBufferLen,
                         OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetCurrentAddr(IN P_ADAPTER_T prAdapter,
                      IN void *pvSetBuffer,
                      IN u32 u4SetBufferLen,
                      OUT u32 *pu4SetInfoLen);

#if CFG_TCP_IP_CHKSUM_OFFLOAD
WLAN_STATUS
wlanoidSetCSUMOffload(IN P_ADAPTER_T prAdapter,
                      IN void *pvSetBuffer,
                      IN u32 u4SetBufferLen,
                      OUT u32 *pu4SetInfoLen);
#endif

WLAN_STATUS
wlanoidSetNetworkAddress(IN P_ADAPTER_T prAdapter,
                         IN void *pvSetBuffer,
                         IN u32 u4SetBufferLen,
                         OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryMaxFrameSize(IN P_ADAPTER_T prAdapter,
                         OUT void *pvQueryBuffer,
                         IN u32 u4QueryBufferLen,
                         OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryMaxTotalSize(IN P_ADAPTER_T prAdapter,
                         OUT void *pvQueryBuffer,
                         IN u32 u4QueryBufferLen,
                         OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetCurrentLookahead(IN P_ADAPTER_T prAdapter,
                           IN void *pvSetBuffer,
                           IN u32 u4SetBufferLen,
                           OUT u32 *pu4SetInfoLen);

/* RF Test related APIs */
WLAN_STATUS
wlanoidRftestSetTestMode(IN P_ADAPTER_T prAdapter,
                         IN void *pvSetBuffer,
                         IN u32 u4SetBufferLen,
                         OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidRftestSetTestIcapMode(IN P_ADAPTER_T prAdapter,
                             IN void *pvSetBuffer,
                             IN u32 u4SetBufferLen,
                             OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidRftestSetAbortTestMode(IN P_ADAPTER_T prAdapter,
                              IN void *pvSetBuffer,
                              IN u32 u4SetBufferLen,
                              OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidRftestQueryAutoTest(IN P_ADAPTER_T prAdapter,
                           OUT void *pvQueryBuffer,
                           IN u32 u4QueryBufferLen,
                           OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidRftestSetAutoTest(IN P_ADAPTER_T prAdapter,
                         OUT void *pvSetBuffer,
                         IN u32 u4SetBufferLen,
                         OUT u32 *pu4SetInfoLen);

#if CFG_SUPPORT_WPS2
WLAN_STATUS
wlanoidSetWSCAssocInfo(IN P_ADAPTER_T prAdapter,
                       IN void *pvSetBuffer,
                       IN u32 u4SetBufferLen,
                       OUT u32 *pu4SetInfoLen);
#endif

#if CFG_ENABLE_WAKEUP_ON_LAN
WLAN_STATUS
wlanoidSetAddWakeupPattern(IN P_ADAPTER_T prAdapter,
                           IN void *pvSetBuffer,
                           IN u32 u4SetBufferLen,
                           OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetRemoveWakeupPattern(IN P_ADAPTER_T prAdapter,
                              IN void *pvSetBuffer,
                              IN u32 u4SetBufferLen,
                              OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryEnableWakeup(IN P_ADAPTER_T prAdapter,
                         OUT void *pvQueryBuffer,
                         IN u32 u4QueryBufferLen,
                         OUT u32 *u4QueryInfoLen);

WLAN_STATUS
wlanoidSetEnableWakeup(IN P_ADAPTER_T prAdapter,
                       IN void *pvSetBuffer,
                       IN u32 u4SetBufferLen,
                       OUT u32 *pu4SetInfoLen);
#endif

WLAN_STATUS
wlanoidSetWiFiWmmPsTest(IN P_ADAPTER_T prAdapter,
                        IN void *pvSetBuffer,
                        IN u32 u4SetBufferLen,
                        OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetTxAmpdu(IN P_ADAPTER_T prAdapter,
                  IN void *pvSetBuffer,
                  IN u32 u4SetBufferLen,
                  OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetAddbaReject(IN P_ADAPTER_T prAdapter,
                      IN void *pvSetBuffer,
                      IN u32 u4SetBufferLen,
                      OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryNvramRead(IN P_ADAPTER_T prAdapter,
                      OUT void *pvQueryBuffer,
                      IN u32 u4QueryBufferLen,
                      OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetNvramWrite(IN P_ADAPTER_T prAdapter,
                     IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen,
                     OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryCfgSrcType(IN P_ADAPTER_T prAdapter,
                       OUT void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen,
                       OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryEepromType(IN P_ADAPTER_T prAdapter,
                       OUT void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen,
                       OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetCountryCode(IN P_ADAPTER_T prAdapter,
                      IN void *pvSetBuffer,
                      IN u32 u4SetBufferLen,
                      OUT u32 *pu4SetInfoLen);

WLAN_STATUS wlanSendMemDumpCmd(IN P_ADAPTER_T prAdapter,
                               IN void *pvQueryBuffer,
                               IN u32 u4QueryBufferLen);

#if CFG_SUPPORT_ADVANCE_CONTROL
WLAN_STATUS
wlanoidAdvCtrl(IN P_ADAPTER_T prAdapter,
               OUT void *pvQueryBuffer,
               IN u32 u4QueryBufferLen,
               OUT u32 *pu4QueryInfoLen);
#endif

WLAN_STATUS
wlanoidQueryWlanInfo(IN P_ADAPTER_T prAdapter,
                     OUT void *pvQueryBuffer,
                     IN u32 u4QueryBufferLen,
                     OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryMibInfo(IN P_ADAPTER_T prAdapter,
                    OUT void *pvQueryBuffer,
                    IN u32 u4QueryBufferLen,
                    OUT u32 *pu4QueryInfoLen);
#if CFG_SUPPORT_LAST_SEC_MCS_INFO
WLAN_STATUS
wlanoidTxMcsInfo(IN P_ADAPTER_T prAdapter,
                 IN void *pvQueryBuffer,
                 IN u32 u4QueryBufferLen,
                 OUT u32 *pu4QueryInfoLen);
#endif

WLAN_STATUS
wlanoidSetFwLog2Host(IN P_ADAPTER_T prAdapter,
                     IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen,
                     OUT u32 *pu4SetInfoLen);

/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidSetBT(IN P_ADAPTER_T prAdapter,
             IN void *pvSetBuffer,
             IN u32 u4SetBufferLen,
             OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryBT(IN P_ADAPTER_T prAdapter,
               OUT void *pvQueryBuffer,
               IN u32 u4QueryBufferLen,
               OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetTxPower(IN P_ADAPTER_T prAdapter,
                  IN void *pvSetBuffer,
                  IN u32 u4SetBufferLen,
                  OUT u32 *pu4SetInfoLen);

#if CFG_ENABLE_WIFI_DIRECT
WLAN_STATUS
wlanoidSetP2pMode(IN P_ADAPTER_T prAdapter,
                  IN void *pvSetBuffer,
                  IN u32 u4SetBufferLen,
                  OUT u32 *pu4SetInfoLen);
#endif

WLAN_STATUS
wlanoidSetDefaultKey(IN P_ADAPTER_T prAdapter,
                     IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen,
                     OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetGtkRekeyData(IN P_ADAPTER_T prAdapter,
                       IN void *pvSetBuffer,
                       IN u32 u4SetBufferLen,
                       OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetStartSchedScan(IN P_ADAPTER_T prAdapter,
                         IN void *pvSetBuffer,
                         IN u32 u4SetBufferLen,
                         OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidSetStopSchedScan(IN P_ADAPTER_T prAdapter,
                        IN void *pvSetBuffer,
                        IN u32 u4SetBufferLen,
                        OUT u32 *pu4SetInfoLen);

#if CFG_SUPPORT_BATCH_SCAN
WLAN_STATUS
wlanoidSetBatchScanReq(IN P_ADAPTER_T prAdapter,
                       IN void *pvSetBuffer,
                       IN u32 u4SetBufferLen,
                       OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryBatchScanResult(IN P_ADAPTER_T prAdapter,
                            OUT void *pvQueryBuffer,
                            IN u32 u4QueryBufferLen,
                            OUT u32 *pu4QueryInfoLen);
#endif

#if CFG_SUPPORT_SNIFFER
WLAN_STATUS wlanoidSetMonitor(IN P_ADAPTER_T prAdapter,
                              IN void *pvSetBuffer,
                              IN u32 u4SetBufferLen,
                              OUT u32 *pu4SetInfoLen);
#endif

#if CFG_STR_DHCP_RENEW_OFFLOAD
WLAN_STATUS
wlanoidSetDhcpOffladInfo(IN P_ADAPTER_T prAdapter,
                         IN void *pvSetBuffer,
                         IN u32 u4SetBufferLen,
                         OUT u32 *pu4SetInfoLen);
#endif

WLAN_STATUS
wlanoidNotifyFwSuspend(IN P_ADAPTER_T prAdapter,
                       IN void *pvSetBuffer,
                       IN u32 u4SetBufferLen,
                       OUT u32 *pu4SetInfoLen);
#if CFG_SUPPORT_DBDC
WLAN_STATUS
wlanoidSetDbdcEnable(IN P_ADAPTER_T prAdapter,
                     IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen,
                     OUT u32 *pu4SetInfoLen);
#endif

WLAN_STATUS
wlanoidQuerySetTxTargetPower(IN P_ADAPTER_T prAdapter,
                             IN void *pvSetBuffer,
                             IN u32 u4SetBufferLen,
                             OUT u32 *pu4SetInfoLen);

#if (CFG_SUPPORT_DFS_MASTER == 1)
WLAN_STATUS
wlanoidQuerySetRddReport(IN P_ADAPTER_T prAdapter,
                         IN void *pvSetBuffer,
                         IN u32 u4SetBufferLen,
                         OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidQuerySetRadarDetectMode(IN P_ADAPTER_T prAdapter,
                               IN void *pvSetBuffer,
                               IN u32 u4SetBufferLen,
                               OUT u32 *pu4SetInfoLen);
#endif

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
WLAN_STATUS
wlanoidQueryLteSafeChannel(IN P_ADAPTER_T prAdapter,
                           IN void *pvQueryBuffer,
                           IN u32 u4QueryBufferLen,
                           OUT u32 *pu4QueryInfoLen);
WLAN_STATUS
wlanCalculateAllChannelDirtiness(IN P_ADAPTER_T prAdapter);
void wlanInitChnLoadInfoChannelList(IN P_ADAPTER_T prAdapter);
u8 wlanGetChannelIndex(IN u8 channel);
u8 wlanGetChannelNumFromIndex(IN u8 ucIdx);
void wlanSortChannel(IN P_ADAPTER_T prAdapter);
#endif

#ifdef CFG_SUPPORT_ANT_DIV
WLAN_STATUS
wlanoidAntDivCfg(IN P_ADAPTER_T prAdapter,
                 IN void *pvSetBuffer,
                 IN u32 u4SetBufferLen,
                 OUT u32 *pu4SetInfoLen);
#endif

WLAN_STATUS
wlanoidLinkDown(IN P_ADAPTER_T prAdapter,
                IN void *pvSetBuffer,
                IN u32 u4SetBufferLen,
                OUT u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidAbortScan(IN P_ADAPTER_T prAdapter,
                 OUT void *pvQueryBuffer,
                 IN u32 u4QueryBufferLen,
                 OUT u32 *pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetCSIControl(IN P_ADAPTER_T prAdapter,
                     IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen,
                     OUT u32 *pu4SetInfoLen);

#ifdef CFG_DUMP_TXPOWR_TABLE
WLAN_STATUS
wlanoidGetTxPwrTbl(IN P_ADAPTER_T prAdapter,
                   IN void *pvQueryBuffer,
                   IN u32 u4QueryBufferLen,
                   OUT u32 *pu4QueryInfoLen);
#endif

u32 wlanGetSupportedFeatureSet(IN P_GLUE_INFO_T prGlueInfo);

WLAN_STATUS wlanSuspendLinkDown(IN P_GLUE_INFO_T prGlueInfo);

#if CFG_SUPPORT_802_11K
WLAN_STATUS
wlanoidSendNeighborRequest(IN P_ADAPTER_T prAdapter,
                           IN void *pvSetBuffer,
                           u32 u4SetBufferLen,
                           u32 *pu4SetInfoLen);
#endif

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
WLAN_STATUS
wlanoidSendBTMQuery(IN P_ADAPTER_T prAdapter,
                    IN void *pvSetBuffer,
                    u32 u4SetBufferLen,
                    u32 *pu4SetInfoLen);

WLAN_STATUS
wlanoidPktProcessIT(IN P_ADAPTER_T prAdapter,
                    IN void *pvBuffer,
                    u32 u4BufferLen,
                    u32 *pu4InfoLen);
#endif

u32 wlanoidIndicateBssInfo(IN P_ADAPTER_T prAdapter,
                           IN void *pvSetBuffer,
                           IN u32 u4SetBufferLen,
                           OUT u32 *pu4SetInfoLen);

WLAN_STATUS batchSetCmd(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer, IN u32 u4SetBufferLen, OUT u32 *pu4WritenLen);
WLAN_STATUS batchGetCmd(IN P_ADAPTER_T prAdapter, OUT void *pvQueryBuffer, IN u32 u4QueryBufferLen, OUT u32 *pu4QueryInfoLen);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
