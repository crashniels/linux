/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

#ifndef _P2P_H
#define _P2P_H

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

/* refer to 'Config Methods' in WPS */
#define WPS_CONFIG_USBA             0x0001
#define WPS_CONFIG_ETHERNET         0x0002
#define WPS_CONFIG_LABEL            0x0004
#define WPS_CONFIG_DISPLAY          0x0008
#define WPS_CONFIG_EXT_NFC          0x0010
#define WPS_CONFIG_INT_NFC          0x0020
#define WPS_CONFIG_NFC              0x0040
#define WPS_CONFIG_PBC              0x0080
#define WPS_CONFIG_KEYPAD           0x0100

/* refer to 'Device Password ID' in WPS */
#define WPS_DEV_PASSWORD_ID_PIN         0x0000
#define WPS_DEV_PASSWORD_ID_USER        0x0001
#define WPS_DEV_PASSWORD_ID_MACHINE     0x0002
#define WPS_DEV_PASSWORD_ID_REKEY       0x0003
#define WPS_DEV_PASSWORD_ID_PUSHBUTTON      0x0004
#define WPS_DEV_PASSWORD_ID_REGISTRAR       0x0005

#define P2P_DEVICE_TYPE_NUM         2
#define P2P_DEVICE_NAME_LENGTH          32
#define P2P_NETWORK_NUM             8
#define P2P_MEMBER_NUM              8

/* Device Capability Definition. */
#define P2P_MAXIMUM_CLIENT_COUNT        10
#define P2P_MAXIMUM_NOA_COUNT           8

#define P2P_MAX_SUPPORTED_CHANNEL_LIST_SIZE 51  /* Contains 6 sub-band. */

/* Memory Size Definition. */
#define P2P_MAXIMUM_ATTRIBUTES_CACHE_SIZE   768
#define WPS_MAXIMUM_ATTRIBUTES_CACHE_SIZE   300

#define P2P_WILDCARD_SSID           "DIRECT-"

/* Device Charactoristic. */
#define P2P_AP_CHNL_HOLD_TIME_MS \
    5000  /* 1000 is too short , the deauth would block in the queue */
#define P2P_DEFAULT_LISTEN_CHANNEL      1

#if (CFG_SUPPORT_DFS_MASTER == 1)
#define P2P_AP_CAC_WEATHER_CHNL_HOLD_TIME_MS    (600 * 1000)
#endif

#define P2P_DEAUTH_TIMEOUT_TIME_MS      1000

#define P2P_SAA_RETRY_COUNT         3

/* Define the Delay time for DBDC DFS Master mode  */
#if CFG_SUPPORT_DBDC_TC6
#define P2P_DFS_CAC_DELAY_TIME_MS       5000
#endif

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#if DBG
#define ASSERT_BREAK(_exp) \
    {                      \
        if (!(_exp)) {     \
            ASSERT(false); \
        }                  \
    }

#else
#define ASSERT_BREAK(_exp)
#endif

#define p2pChangeMediaState(_prAdapter, _prP2pBssInfo, _eNewMediaState) \
    (_prP2pBssInfo->eConnectionState = (_eNewMediaState))

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct _P2P_INFO_T {
    u32 u4DeviceNum;
    EVENT_P2P_DEV_DISCOVER_RESULT_T
        arP2pDiscoverResult[CFG_MAX_NUM_BSS_LIST];
    u8 *pucCurrIePtr;
    /* A common pool for IE of all scan results. */
    u8 aucCommIePool[CFG_MAX_COMMON_IE_BUF_LEN];
};

typedef enum {
    ENUM_P2P_PEER_GROUP,
    ENUM_P2P_PEER_DEVICE,
    ENUM_P2P_PEER_NUM
} ENUM_P2P_PEER_TYPE,
*P_ENUM_P2P_PEER_TYPE;

typedef struct _P2P_DEVICE_INFO {
    u8 aucDevAddr[PARAM_MAC_ADDR_LEN];
    u8 aucIfAddr[PARAM_MAC_ADDR_LEN];
    u8 ucDevCapabilityBitmap;
    s32 i4ConfigMethod;
    u8 aucPrimaryDeviceType[8];
    u8 aucSecondaryDeviceType[8];
    u8 aucDeviceName[P2P_DEVICE_NAME_LENGTH];
} P2P_DEVICE_INFO, *P_P2P_DEVICE_INFO;

typedef struct _P2P_GROUP_INFO {
    PARAM_SSID_T rGroupID;
    P2P_DEVICE_INFO rGroupOwnerInfo;
    u8 ucMemberNum;
    P2P_DEVICE_INFO arMemberInfo[P2P_MEMBER_NUM];
} P2P_GROUP_INFO, *P_P2P_GROUP_INFO;

typedef struct _P2P_NETWORK_INFO {
    ENUM_P2P_PEER_TYPE eNodeType;

    union {
        P2P_GROUP_INFO rGroupInfo;
        P2P_DEVICE_INFO rDeviceInfo;
    } node;
} P2P_NETWORK_INFO, *P_P2P_NETWORK_INFO;

typedef struct _P2P_NETWORK_LIST {
    u8 ucNetworkNum;
    P2P_NETWORK_INFO rP2PNetworkInfo[P2P_NETWORK_NUM];
} P2P_NETWORK_LIST, *P_P2P_NETWORK_LIST;

typedef struct _P2P_DISCONNECT_INFO {
    u8 ucRole;
    u8 ucRsv[3];
} P2P_DISCONNECT_INFO, *P_P2P_DISCONNECT_INFO;

typedef struct _P2P_SSID_STRUCT_T {
    u8 aucSsid[32];
    u8 ucSsidLen;
} P2P_SSID_STRUCT_T, *P_P2P_SSID_STRUCT_T;

typedef struct _P2P_SCAN_REQ_INFO_T {
    ENUM_SCAN_TYPE_T eScanType;
    ENUM_SCAN_CHANNEL eChannelSet;
    u16 u2PassiveDewellTime;
    u8 ucSeqNumOfScnMsg;
    u8 fgIsAbort;
    u8 fgIsScanRequest;
    u8 ucNumChannelList;
    RF_CHANNEL_INFO_T arScanChannelList[MAXIMUM_OPERATION_CHANNEL_LIST];
    u32 u4BufLength;
    u8 aucIEBuf[MAX_IE_LENGTH];
    u8 ucSsidNum;
    P2P_SSID_STRUCT_T arSsidStruct[SCN_SSID_MAX_NUM];  /* Currently we can
                                                        * only take one SSID
                                                        * scan request */
} P2P_SCAN_REQ_INFO_T, *P_P2P_SCAN_REQ_INFO_T;

typedef struct _P2P_CHNL_REQ_INFO_T {
    LINK_T rP2pChnlReqLink;
    u8 fgIsChannelRequested;
    u8 ucSeqNumOfChReq;
    u64 u8Cookie;
    u8 ucReqChnlNum;
    ENUM_BAND_T eBand;
    ENUM_CHNL_EXT_T eChnlSco;
    u8 ucOriChnlNum;
    ENUM_CHANNEL_WIDTH_T eChannelWidth;  /*VHT operation ie */
    u8 ucCenterFreqS1;
    u8 ucCenterFreqS2;
    ENUM_BAND_T eOriBand;
    ENUM_CHNL_EXT_T eOriChnlSco;
    u32 u4MaxInterval;
    ENUM_CH_REQ_TYPE_T eChnlReqType;
#if CFG_SUPPORT_NFC_BEAM_PLUS
    u32 NFC_BEAM;  /*NFC Beam + Indication */
#endif
} P2P_CHNL_REQ_INFO_T, *P_P2P_CHNL_REQ_INFO_T;

/* Glubal Connection Settings. */
struct _P2P_CONNECTION_SETTINGS_T {
    /*u8 ucRfChannelListSize;*/
#if P2P_MAX_SUPPORTED_CHANNEL_LIST_SIZE
    /*u8 aucChannelEntriesField[P2P_MAX_SUPPORTED_CHANNEL_LIST_SIZE];*/
#endif

    u8 fgIsApMode;
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
    u8 fgIsWPSMode;
#endif
};

typedef struct _NOA_TIMING_T {
    u8 fgIsInUse;  /* Indicate if this entry is in use or not */
    u8 ucCount;  /* Count */

    u8 aucReserved[2];

    u32 u4Duration;  /* Duration */
    u32 u4Interval;  /* Interval */
    u32 u4StartTime;  /* Start Time */
} NOA_TIMING_T, *P_NOA_TIMING_T;

struct _P2P_SPECIFIC_BSS_INFO_T {
    /* For GO(AP) Mode - Compose TIM IE */
    /*u16 u2SmallestAID;*/ /* TH3 multiple P2P */
    /*u16 u2LargestAID;*/ /* TH3 multiple P2P */
    /*u8 ucBitmapCtrl;*/ /* TH3 multiple P2P */
    /* u8                  aucPartialVirtualBitmap[MAX_LEN_TIM_PARTIAL_BMP];
     */

    /* For GC/GO OppPS */
    u8 fgEnableOppPS;
    u16 u2CTWindow;

    /* For GC/GO NOA */
    u8 ucNoAIndex;
    u8 ucNoATimingCount;  /* Number of NoA Timing */
    NOA_TIMING_T arNoATiming[P2P_MAXIMUM_NOA_COUNT];

    u8 fgIsNoaAttrExisted;

    /* For P2P Device */
    /*u8 ucRegClass;*/ /* TH3 multiple P2P */ /* Regulatory Class for
     *                                           channel.
     */
    /* Linten Channel only on channels 1, 6 and 11 in the 2.4 GHz. */
    /*u8 ucListenChannel;*/ /* TH3 multiple P2P */

    /* Operating Channel, should be one of channel */
    /* list in p2p connection settings. */
    u8 ucPreferredChannel;
    ENUM_CHNL_EXT_T eRfSco;
    ENUM_BAND_T eRfBand;

    /* Extended Listen Timing. */
    u16 u2AvailabilityPeriod;
    u16 u2AvailabilityInterval;

    u16 u2AttributeLen;
    u8 aucAttributesCache[P2P_MAXIMUM_ATTRIBUTES_CACHE_SIZE];

    /*u16 u2WscAttributeLen;*/ /* TH3 multiple P2P */
    /*u8 aucWscAttributesCache[WPS_MAXIMUM_ATTRIBUTES_CACHE_SIZE];*/ /* TH3
     *                                                                  multiple
     *                                                                  P2P
     */

    /*u8 aucGroupID[MAC_ADDR_LEN];*/ /* TH3 multiple P2P */
    u16 u2GroupSsidLen;
    u8 aucGroupSsid[ELEM_MAX_LEN_SSID];

    PARAM_CUSTOM_NOA_PARAM_STRUCT_T rNoaParam;
    PARAM_CUSTOM_OPPPS_PARAM_STRUCT_T rOppPsParam;

    u16 u2WpaIeLen;
    u8 aucWpaIeBuffer[ELEM_HDR_LEN + ELEM_MAX_LEN_WPA];
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
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

#endif
