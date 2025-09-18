/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "wlan_lib.h"
 *    \brief  The declaration of the functions of the wlanAdpater objects
 *
 *    Detail description.
 */

#ifndef _WLAN_LIB_H
#define _WLAN_LIB_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "CFG_Wifi_File.h"
#include "rlm_domain.h"
#include "nic_init_cmd_event.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/* These values must sync from Wifi HAL
 * /hardware/libhardware_legacy/include/hardware_legacy/wifi_hal.h
 */
/* Basic infrastructure mode */
//#define WIFI_FEATURE_INFRA              (0x0001)
/* Support for 5 GHz Band */
#define WIFI_FEATURE_INFRA_5G            (0x0002)
/* Support for GAS/ANQP */
//#define WIFI_FEATURE_HOTSPOT            (0x0004)
/* Wifi-Direct */
#define WIFI_FEATURE_P2P             (0x0008)
/* Soft AP */
#define WIFI_FEATURE_SOFT_AP             (0x0010)
/* Tunnel directed link setup */
#define WIFI_FEATURE_TDLS            (0x1000)

/* note: WIFI_FEATURE_GSCAN be enabled  just for ACTS test item: scanner  */
#define WIFI_HAL_FEATURE_SET             ((WIFI_FEATURE_P2P) | \
                                          (WIFI_FEATURE_SOFT_AP))

#define MAX_NUM_GROUP_ADDR           32  /* max number of group addresses */
#define AUTO_RATE_NUM                8
#define AR_RATE_TABLE_ENTRY_MAX          25
#define AR_RATE_ENTRY_INDEX_NULL         0x80

#define TX_CS_TCP_UDP_GEN            BIT(1)
#define TX_CS_IP_GEN                 BIT(0)

#define CSUM_OFFLOAD_EN_TX_TCP           BIT(0)
#define CSUM_OFFLOAD_EN_TX_UDP           BIT(1)
#define CSUM_OFFLOAD_EN_TX_IP            BIT(2)
#define CSUM_OFFLOAD_EN_RX_TCP           BIT(3)
#define CSUM_OFFLOAD_EN_RX_UDP           BIT(4)
#define CSUM_OFFLOAD_EN_RX_IPv4          BIT(5)
#define CSUM_OFFLOAD_EN_RX_IPv6          BIT(6)
#define CSUM_OFFLOAD_EN_TX_MASK          BITS(0, 2)
#define CSUM_OFFLOAD_EN_ALL          BITS(0, 6)

/* TCP, UDP, IP Checksum */
#define RX_CS_TYPE_UDP               BIT(7)
#define RX_CS_TYPE_TCP               BIT(6)
#define RX_CS_TYPE_IPv6              BIT(5)
#define RX_CS_TYPE_IPv4              BIT(4)

#define RX_CS_STATUS_UDP             BIT(3)
#define RX_CS_STATUS_TCP             BIT(2)
#define RX_CS_STATUS_IP              BIT(0)

#define CSUM_NOT_SUPPORTED           0x0

#define TXPWR_USE_PDSLOPE            0

/* NVRAM error code definitions */
#define NVRAM_ERROR_VERSION_MISMATCH         BIT(1)
#define NVRAM_ERROR_INVALID_TXPWR        BIT(2)
#define NVRAM_ERROR_INVALID_DPD          BIT(3)
#define NVRAM_ERROR_INVALID_MAC_ADDR         BIT(4)
#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
#define NVRAM_POWER_LIMIT_TABLE_INVALID      BIT(5)
#endif

#define NUM_TC_RESOURCE_TO_STATISTICS        4

#define WLAN_CFG_ARGV_MAX            23
#define WLAN_CFG_ARGV_MAX_LONG           22  /* for WOW, 2+20 */
#define WLAN_CFG_ENTRY_NUM_MAX           200  /* 128 */
#define WLAN_CFG_KEY_LEN_MAX             32  /* include \x00  EOL */
#define WLAN_CFG_VALUE_LEN_MAX           32  /* include \x00 EOL */
#define WLAN_CFG_FLAG_SKIP_CB            BIT(0)
#define WLAN_CFG_FILE_BUF_SIZE           2048

#define WLAN_CFG_REC_ENTRY_NUM_MAX       200
#define WLAN_CFG_REC_FLAG_BIT            BIT(0)

#define WLAN_CFG_SET_CHIP_LEN_MAX        10
#define WLAN_CFG_SET_DEBUG_LEVEL_LEN_MAX     10
#define WLAN_CFG_SET_SW_CTRL_LEN_MAX         10

#define WLAN_OID_TIMEOUT_THRESHOLD       2000  /* OID timeout (in ms) */
#define WLAN_OID_TIMEOUT_THRESHOLD_MAX       10000  /* OID max timeout (in ms) */
#define WLAN_OID_TIMEOUT_THRESHOLD_IN_RESETTING \
    300  /* OID timeout during chip-resetting  (in ms) */

#define WLAN_OID_NO_ACK_THRESHOLD        3

#define WLAN_THREAD_TASK_PRIORITY \
    0  /* If not setting the priority, 0 is the default */
#define WLAN_THREAD_TASK_NICE \
    (-10)  /* If not setting the nice, -10 is the default */

#define WLAN_TX_STATS_LOG_TIMEOUT        30000
#define WLAN_TX_STATS_LOG_DURATION       1500

/* Define for wifi path usage */
#define WLAN_FLAG_2G4_WF0            BIT(0)  /*1: support, 0: NOT support */
#define WLAN_FLAG_5G_WF0             BIT(1)  /*1: support, 0: NOT support */
#define WLAN_FLAG_2G4_WF1            BIT(2)  /*1: support, 0: NOT support */
#define WLAN_FLAG_5G_WF1             BIT(3)  /*1: support, 0: NOT support */
#define WLAN_FLAG_2G4_COANT_SUPPORT      BIT(4)  /*1: support, 0: NOT support */
#define WLAN_FLAG_2G4_COANT_PATH         BIT(5)  /*1: WF1, 0:WF0 */
#define WLAN_FLAG_5G_COANT_SUPPORT       BIT(6)  /*1: support, 0: NOT support */
#define WLAN_FLAG_5G_COANT_PATH          BIT(7)  /*1: WF1, 0:WF0 */

#if CFG_SUPPORT_EASY_DEBUG

#define MAX_CMD_ITEM_MAX             4  /* Max item per cmd. */
#define MAX_CMD_NAME_MAX_LENGTH          32  /* Max name string length */
#define MAX_CMD_VALUE_MAX_LENGTH         32  /* Max value string length */
#define MAX_CMD_TYPE_LENGTH          1
#define MAX_CMD_STRING_LENGTH            1
#define MAX_CMD_VALUE_LENGTH             1
#define MAX_CMD_RESERVE_LENGTH           1

#define CMD_FORMAT_V1_LENGTH                                              \
    (MAX_CMD_NAME_MAX_LENGTH + MAX_CMD_VALUE_MAX_LENGTH +                 \
     MAX_CMD_TYPE_LENGTH + MAX_CMD_STRING_LENGTH + MAX_CMD_VALUE_LENGTH + \
     MAX_CMD_RESERVE_LENGTH)

#define MAX_CMD_BUFFER_LENGTH            (CMD_FORMAT_V1_LENGTH * \
                                          MAX_CMD_ITEM_MAX)

#define ED_STRING_SITE               0
#define ED_VALUE_SITE                1

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
#define ACS_AP_RSSI_LEVEL_HIGH           -50
#define ACS_AP_RSSI_LEVEL_LOW            -80
#define ACS_DIRTINESS_LEVEL_HIGH         52
#define ACS_DIRTINESS_LEVEL_MID          40
#define ACS_DIRTINESS_LEVEL_LOW          32
#endif

#if CFG_WOW_SUPPORT
#define INVALID_WOW_WAKE_UP_REASON       255
#endif

#if CFG_SUPPORT_ADVANCE_CONTROL
#define KEEP_FULL_PWR_TRAFFIC_REPORT_BIT     BIT(0)
#define KEEP_FULL_PWR_NOISE_HISTOGRAM_BIT    BIT(1)
#define BLOCK_KEEP_FULL_PWR          BIT(31)
#endif

typedef enum _CMD_VER_T {
    CMD_VER_1,  /* Type[2]+String[32]+Value[32] */
    CMD_VER_2  /* for furtur define. */
} CMD_VER_T,
*P_CMD_VER_T;

typedef enum _CMD_TYPE_T {
    CMD_TYPE_QUERY,
    CMD_TYPE_SET
} CMD_TYPE_T,
*P_CMD_TYPE_T;

#define ITEM_TYPE_DEC    1
#define ITEM_TYPE_HEX    2
#define ITEM_TYPE_STR    3

typedef enum _CMD_DEFAULT_SETTING_VALUE {
    CMD_PNO_ENABLE,
    CMD_PNO_SCAN_PERIOD,
    CMD_SCN_CHANNEL_PLAN,
    CMD_SCN_DWELL_TIME,
    CMD_SCN_STOP_SCAN,
    CMD_MAX,
} CMD_DEFAULT_SETTING_VALUE;

typedef enum _CMD_DEFAULT_STR_SETTING_VALUE {
    CMD_STR_TEST_STR,
    CMD_STR_MAX,
} CMD_DEFAULT_STR_SETTING_VALUE;

typedef struct _CMD_FORMAT_V1_T {
    u8 itemType;
    u8 itemStringLength;
    u8 itemValueLength;
    u8 Reserved;
    u8 itemString[MAX_CMD_NAME_MAX_LENGTH];
    u8 itemValue[MAX_CMD_VALUE_MAX_LENGTH];
} CMD_FORMAT_V1_T, *P_CMD_FORMAT_V1_T;

typedef struct _CMD_HEADER_T {
    CMD_VER_T cmdVersion;
    CMD_TYPE_T cmdType;
    u8 itemNum;
    u16 cmdBufferLen;
    u8 buffer[MAX_CMD_BUFFER_LENGTH];
} CMD_HEADER_T, *P_CMD_HEADER_T;

typedef struct _CFG_DEFAULT_SETTING_TABLE_T {
    u32 itemNum;
    const char *String;
    u8 itemType;
    u32 defaultValue;
    u32 minValue;
    u32 maxValue;
} CFG_DEFAULT_SETTING_TABLE_T, *P_CFG_DEFAULT_SETTING_TABLE_T;

typedef struct _CFG_DEFAULT_SETTING_STR_TABLE_T {
    u32 itemNum;
    const char *String;
    u8 itemType;
    const char *DefString;
    u16 minLen;
    u16 maxLen;
} CFG_DEFAULT_SETTING_STR_TABLE_T, *P_CFG_DEFAULT_SETTING_STR_TABLE_T;

typedef struct _CFG_QUERY_FORMAT_T {
    u32 Length;
    u32 Value;
    u32 Type;
    u32 *ptr;
} CFG_QUERY_FORMAT_T, *P_CFG_QUERY_FORMAT_T;

/*Globol Configure define */
typedef struct _CFG_SETTING_T {
    u8 PnoEnable;
    u32 PnoScanPeriod;
    u8 ScnChannelPlan;
    u16 ScnDwellTime;
    u8 ScnStopScan;
    u8 TestStr[80];
} CFG_SETTING_T, *P_CFG_SETTING_T;

#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

typedef WLAN_STATUS (*PFN_OID_HANDLER_FUNC)(IN P_ADAPTER_T prAdapter,
                                            IN void *pvBuf,
                                            IN u32 u4BufLen,
                                            OUT u32 *pu4OutInfoLen);

typedef enum _ENUM_CSUM_TYPE_T {
    CSUM_TYPE_IPV4,
    CSUM_TYPE_IPV6,
    CSUM_TYPE_TCP,
    CSUM_TYPE_UDP,
    CSUM_TYPE_NUM
} ENUM_CSUM_TYPE_T,
*P_ENUM_CSUM_TYPE_T;

typedef enum _ENUM_CSUM_RESULT_T {
    CSUM_RES_NONE,
    CSUM_RES_SUCCESS,
    CSUM_RES_FAILED,
    CSUM_RES_NUM
} ENUM_CSUM_RESULT_T,
*P_ENUM_CSUM_RESULT_T;

typedef enum _ENUM_PHY_MODE_T {
    ENUM_PHY_2G4_CCK,
    ENUM_PHY_2G4_OFDM_BPSK,
    ENUM_PHY_2G4_OFDM_QPSK,
    ENUM_PHY_2G4_OFDM_16QAM,
    ENUM_PHY_2G4_OFDM_48M,
    ENUM_PHY_2G4_OFDM_54M,
    ENUM_PHY_2G4_HT20_BPSK,
    ENUM_PHY_2G4_HT20_QPSK,
    ENUM_PHY_2G4_HT20_16QAM,
    ENUM_PHY_2G4_HT20_MCS5,
    ENUM_PHY_2G4_HT20_MCS6,
    ENUM_PHY_2G4_HT20_MCS7,
    ENUM_PHY_2G4_HT40_BPSK,
    ENUM_PHY_2G4_HT40_QPSK,
    ENUM_PHY_2G4_HT40_16QAM,
    ENUM_PHY_2G4_HT40_MCS5,
    ENUM_PHY_2G4_HT40_MCS6,
    ENUM_PHY_2G4_HT40_MCS7,
    ENUM_PHY_5G_OFDM_BPSK,
    ENUM_PHY_5G_OFDM_QPSK,
    ENUM_PHY_5G_OFDM_16QAM,
    ENUM_PHY_5G_OFDM_48M,
    ENUM_PHY_5G_OFDM_54M,
    ENUM_PHY_5G_HT20_BPSK,
    ENUM_PHY_5G_HT20_QPSK,
    ENUM_PHY_5G_HT20_16QAM,
    ENUM_PHY_5G_HT20_MCS5,
    ENUM_PHY_5G_HT20_MCS6,
    ENUM_PHY_5G_HT20_MCS7,
    ENUM_PHY_5G_HT40_BPSK,
    ENUM_PHY_5G_HT40_QPSK,
    ENUM_PHY_5G_HT40_16QAM,
    ENUM_PHY_5G_HT40_MCS5,
    ENUM_PHY_5G_HT40_MCS6,
    ENUM_PHY_5G_HT40_MCS7,
    ENUM_PHY_MODE_NUM
} ENUM_PHY_MODE_T,
*P_ENUM_PHY_MODE_T;

typedef enum _ENUM_POWER_SAVE_POLL_MODE_T {
    ENUM_POWER_SAVE_POLL_DISABLE,
    ENUM_POWER_SAVE_POLL_LEGACY_NULL,
    ENUM_POWER_SAVE_POLL_QOS_NULL,
    ENUM_POWER_SAVE_POLL_NUM
} ENUM_POWER_SAVE_POLL_MODE_T,
*P_ENUM_POWER_SAVE_POLL_MODE_T;

typedef enum _ENUM_AC_TYPE_T {
    ENUM_AC_TYPE_AC0,
    ENUM_AC_TYPE_AC1,
    ENUM_AC_TYPE_AC2,
    ENUM_AC_TYPE_AC3,
    ENUM_AC_TYPE_AC4,
    ENUM_AC_TYPE_AC5,
    ENUM_AC_TYPE_AC6,
    ENUM_AC_TYPE_BMC,
    ENUM_AC_TYPE_NUM
} ENUM_AC_TYPE_T,
*P_ENUM_AC_TYPE_T;

typedef enum _ENUM_ADV_AC_TYPE_T {
    ENUM_ADV_AC_TYPE_RX_NSW,
    ENUM_ADV_AC_TYPE_RX_PTA,
    ENUM_ADV_AC_TYPE_RX_SP,
    ENUM_ADV_AC_TYPE_TX_PTA,
    ENUM_ADV_AC_TYPE_TX_RSP,
    ENUM_ADV_AC_TYPE_NUM
} ENUM_ADV_AC_TYPE_T,
*P_ENUM_ADV_AC_TYPE_T;

typedef enum _ENUM_REG_CH_MAP_T {
    REG_CH_MAP_COUNTRY_CODE,
    REG_CH_MAP_TBL_IDX,
    REG_CH_MAP_CUSTOMIZED,
    REG_CH_MAP_NUM
} ENUM_REG_CH_MAP_T,
*P_ENUM_REG_CH_MAP_T;

typedef enum _ENUM_FEATURE_OPTION_T {
    FEATURE_DISABLED,
    FEATURE_ENABLED,
    FEATURE_FORCE_ENABLED
} ENUM_FEATURE_OPTION_T,
*P_ENUM_FEATURE_OPTION_T;

/* This enum is for later added feature options which use command reserved field
 * as option switch */
typedef enum _ENUM_FEATURE_OPTION_IN_CMD_T {
    FEATURE_OPT_CMD_AUTO,
    FEATURE_OPT_CMD_DISABLED,
    FEATURE_OPT_CMD_ENABLED,
    FEATURE_OPT_CMD_FORCE_ENABLED
} ENUM_FEATURE_OPTION_IN_CMD_T,
*P_ENUM_FEATURE_OPTION_IN_CMD_T;

#define DEBUG_MSG_SIZE_MAX    1200
enum {
    DEBUG_MSG_ID_UNKNOWN = 0x00,
    DEBUG_MSG_ID_PRINT   = 0x01,
    DEBUG_MSG_ID_FWLOG   = 0x02,
    DEBUG_MSG_ID_END
};

enum {
    DEBUG_MSG_TYPE_UNKNOWN = 0x00,
    DEBUG_MSG_TYPE_MEM8    = 0x01,
    DEBUG_MSG_TYPE_MEM32   = 0x02,
    DEBUG_MSG_TYPE_ASCII   = 0x03,
    DEBUG_MSG_TYPE_BINARY  = 0x04,
    DEBUG_MSG_TYPE_END
};

#define CHIP_CONFIG_RESP_SIZE    320
enum {
    CHIP_CONFIG_TYPE_WO_RESPONSE     = 0x00,
    CHIP_CONFIG_TYPE_MEM8            = 0x01,
    CHIP_CONFIG_TYPE_MEM32           = 0x02,
    CHIP_CONFIG_TYPE_ASCII           = 0x03,
    CHIP_CONFIG_TYPE_BINARY          = 0x04,
    CHIP_CONFIG_TYPE_DRV_PASSTHROUGH = 0x05,
    CHIP_CONFIG_TYPE_END
};

typedef struct _SET_TXPWR_CTRL_T {
    s8 c2GLegacyStaPwrOffset;  /* Unit: 0.5dBm, default: 0 */
    s8 c2GHotspotPwrOffset;
    s8 c2GP2pPwrOffset;
    s8 c2GBowPwrOffset;
    s8 c5GLegacyStaPwrOffset;  /* Unit: 0.5dBm, default: 0 */
    s8 c5GHotspotPwrOffset;
    s8 c5GP2pPwrOffset;
    s8 c5GBowPwrOffset;
    u8 ucConcurrencePolicy;  /* TX power policy when concurrence
                              *  in the same channel
                              *  0: Highest power has priority
                              *  1: Lowest power has priority
                              */
    s8 acReserved1[3];  /* Must be zero */

    /* Power limit by channel for all data rates */
    s8 acTxPwrLimit2G[14];  /* Channel 1~14, Unit: 0.5dBm */
    s8 acTxPwrLimit5G[4];  /* UNII 1~4 */
    s8 acReserved2[2];  /* Must be zero */
} SET_TXPWR_CTRL_T, *P_SET_TXPWR_CTRL_T;

#if CFG_WOW_SUPPORT

typedef struct _WOW_WAKE_HIF_T {
    u8 ucWakeupHif;  /* use in-band signal to wakeup system, ENUM_HIF_TYPE */
    u8 ucGpioPin;  /* GPIO Pin */
    u8 ucTriggerLvl;  /* refer to PF_WAKEUP_CMD_BIT0_OUTPUT_MODE_EN */
    u32 u4GpioInterval;  /* non-zero means output reverse wakeup signal after
                          * delay time */
    u8 aucResv[5];
} WOW_WAKE_HIF_T, *P_WOW_WAKE_HIF_T;

typedef struct _WOW_PORT_T {
    u8 ucIPv4UdpPortCnt;
    u8 ucIPv4TcpPortCnt;
    u8 ucIPv6UdpPortCnt;
    u8 ucIPv6TcpPortCnt;
    u16 ausIPv4UdpPort[MAX_TCP_UDP_PORT];
    u16 ausIPv4TcpPort[MAX_TCP_UDP_PORT];
    u16 ausIPv6UdpPort[MAX_TCP_UDP_PORT];
    u16 ausIPv6TcpPort[MAX_TCP_UDP_PORT];
} WOW_PORT_T, *P_WOW_PORT_T;

typedef struct _WOW_CTRL_T {
    u8 fgWowEnable;  /* 0: disable, 1: wow enable */
    u8 ucScenarioId;  /* just a profile ID */
    u8 ucBlockCount;
    u8 aucReserved1[1];
    WOW_WAKE_HIF_T astWakeHif[2];
    WOW_PORT_T stWowPort;
    u8 ucReason;
} WOW_CTRL_T, *P_WOW_CTRL_T;

#endif

typedef enum _ENUM_NVRAM_MTK_FEATURE_T {
    MTK_FEATURE_2G_256QAM_DISABLED = 0,
    MTK_FEATURE_NUM
} ENUM_NVRAM_MTK_FEATURES_T,
*P_ENUM_NVRAM_MTK_FEATURES_T;

/* For storing driver initialization value from glue layer */
typedef struct _REG_INFO_T {
    u32 u4SdBlockSize;  /* SDIO block size */
    u32 u4SdBusWidth;  /* SDIO bus width. 1 or 4 */
    u32 u4SdClockRate;  /* SDIO clock rate. (in unit of HZ) */
    u32 u4StartFreq;  /* Start Frequency for Ad-Hoc network : in unit of KHz
                       */
    u32 u4AdhocMode;  /* Default mode for Ad-Hoc network :
                       * ENUM_PARAM_AD_HOC_MODE_T */
    u32 u4RddStartFreq;
    u32 u4RddStopFreq;
    u32 u4RddTestMode;
    u32 u4RddShutFreq;
    u32 u4RddDfs;
    s32 i4HighRssiThreshold;
    s32 i4MediumRssiThreshold;
    s32 i4LowRssiThreshold;
    s32 au4TxPriorityTag[ENUM_AC_TYPE_NUM];
    s32 au4RxPriorityTag[ENUM_AC_TYPE_NUM];
    s32 au4AdvPriorityTag[ENUM_ADV_AC_TYPE_NUM];
    u32 u4FastPSPoll;
    u32 u4PTA;  /* 0: disable, 1: enable */
    u32 u4TXLimit;  /* 0: disable, 1: enable */
    u32 u4SilenceWindow;  /* range: 100 - 625, unit: us */
    u32 u4TXLimitThreshold;  /* range: 250 - 1250, unit: us */
    u32 u4PowerMode;
    u32 fgEnArpFilter;
    u32 u4PsCurrentMeasureEn;
    u32 u4UapsdAcBmp;
    u32 u4MaxSpLen;
    u32 fgDisOnlineScan;  /* 0: enable online scan, non-zero: disable online
                           * scan */
    u32 fgDisBcnLostDetection;  /* 0: enable online scan, non-zero: disable
                                 * online scan */
    u32 u4FixedRate;  /* 0: automatic, non-zero: fixed rate */
    u32 u4ArSysParam0;
    u32 u4ArSysParam1;
    u32 u4ArSysParam2;
    u32 u4ArSysParam3;
    u32 fgDisRoaming;  /* 0:enable roaming 1:disable */

    /* NVRAM - MP Data -START- */
    u16 u2Part1OwnVersion;
    u16 u2Part1PeerVersion;

    u8 aucMacAddr[6];
    u16 au2CountryCode[4];  /* Country code (in ISO 3166-1 expression, ex:
                             * "US", "TW")  */
    TX_PWR_PARAM_T rTxPwr;
    u8 aucEFUSE[144];
    u8 ucTxPwrValid;
    u8 ucSupport5GBand;
    u8 fg2G4BandEdgePwrUsed;
    s8 cBandEdgeMaxPwrCCK;
    s8 cBandEdgeMaxPwrOFDM20;
    s8 cBandEdgeMaxPwrOFDM40;
    ENUM_REG_CH_MAP_T eRegChannelListMap;
    u8 ucRegChannelListIndex;
    DOMAIN_INFO_ENTRY rDomainInfo;
    RSSI_PATH_COMPASATION_T rRssiPathCompasation;
    u8 ucRssiPathCompasationUsed;
    /* NVRAM - MP Data -END- */

    /* NVRAM - Functional Data -START- */
    u8 uc2G4BwFixed20M;
    u8 uc5GBwFixed20M;
    u8 ucEnable5GBand;
    u8 ucGpsDesense;
    u8 ucRxDiversity;
    /* NVRAM - Functional Data -END- */

    P_NEW_EFUSE_MAPPING2NVRAM_T prOldEfuseMapping;

    u8 aucNvram[512];
    P_WIFI_CFG_PARAM_STRUCT prNvramSettings;
} REG_INFO_T, *P_REG_INFO_T;

/* for divided firmware loading */
typedef struct _FWDL_SECTION_INFO_T {
    u32 u4DestAddr;
    u8 ucChipInfo;
    u8 ucFeatureSet;
    u8 ucEcoCode;
    u8 aucReserved[9];
    u8 aucBuildDate[16];
    u32 u4Length;
} FWDL_SECTION_INFO_T, *P_FWDL_SECTION_INFO_T;

typedef struct _FIRMWARE_DIVIDED_DOWNLOAD_T {
    FWDL_SECTION_INFO_T arSection[2];
} FIRMWARE_DIVIDED_DOWNLOAD_T, *P_FIRMWARE_DIVIDED_DOWNLOAD_T;

#if (CFG_UMAC_GENERATION >= 0x20)
#define LEN_4_BYTE_CRC    (4)

typedef struct _tailer_format_tag {
    u32 addr;
    u8 chip_info;
    u8 feature_set;
    u8 eco_code;
    u8 ram_version[10];
    u8 ram_built_date[15];
    u32 len;
} tailer_format_t;

typedef struct _fw_image_tailer_tag {
    tailer_format_t ilm_info;
    tailer_format_t dlm_info;
} fw_image_tailer_t;
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
typedef struct _tailer_format_tag_2 {
    u32 crc;
    u32 addr;
    u32 block_size;
    u32 real_size;
    u8 chip_info;
    u8 feature_set;
    u8 eco_code;
    u8 ram_version[10];
    u8 ram_built_date[15];
    u32 len;
} tailer_format_t_2;
typedef struct _fw_image_tailer_tag_2 {
    tailer_format_t_2 ilm_info;
    tailer_format_t_2 dlm_info;
} fw_image_tailer_t_2;
typedef struct _fw_image_tailer_check {
    u8 chip_info;
    u8 feature_set;
    u8 eco_code;
    u8 ram_version[10];
    u8 ram_built_date[15];
    u32 len;
} fw_image_tailer_check;
#endif
typedef struct _PATCH_FORMAT_T {
    u8 aucBuildDate[16];
    u8 aucPlatform[4];
    u32 u4SwHwVersion;
    u32 u4PatchVersion;
    u16 u2CRC;  /* CRC calculated for image only */
    u8 ucPatchImage[0];
} PATCH_FORMAT_T, *P_PATCH_FORMAT_T;

/* PDA - Patch Decryption Accelerator */
#define PDA_N9            0
#define PDA_CR4           1

#define CR4_FWDL_SECTION_NUM      HIF_CR4_FWDL_SECTION_NUM
#define IMG_DL_STATUS_PORT_IDX    HIF_IMG_DL_STATUS_PORT_IDX

typedef enum _ENUM_IMG_DL_IDX_T {
    IMG_DL_IDX_N9_FW,
    IMG_DL_IDX_CR4_FW,
    IMG_DL_IDX_PATCH
} ENUM_IMG_DL_IDX_T,
*P_ENUM_IMG_DL_IDX_T;

#endif

typedef struct _PARAM_MCR_RW_STRUCT_T {
    u32 u4McrOffset;
    u32 u4McrData;
} PARAM_MCR_RW_STRUCT_T, *P_PARAM_MCR_RW_STRUCT_T;

/* per access category statistics */
typedef struct _WIFI_WMM_AC_STAT_T {
    u32 u4TxMsdu;
    u32 u4RxMsdu;
    u32 u4TxDropMsdu;
    u32 u4TxFailMsdu;
    u32 u4TxRetryMsdu;
} WIFI_WMM_AC_STAT_T, *P_WIFI_WMM_AC_STAT_T;

typedef struct _TX_VECTOR_BBP_LATCH_T {
    u32 u4TxVector1;
    u32 u4TxVector2;
    u32 u4TxVector4;
} TX_VECTOR_BBP_LATCH_T, *P_TX_VECTOR_BBP_LATCH_T;

typedef struct _MIB_INFO_STAT_T {
    u32 u4RxMpduCnt;
    u32 u4FcsError;
    u32 u4RxFifoFull;
    u32 u4AmpduTxSfCnt;
    u32 u4AmpduTxAckSfCnt;
    u16 u2TxRange1AmpduCnt;
    u16 u2TxRange2AmpduCnt;
    u16 u2TxRange3AmpduCnt;
    u16 u2TxRange4AmpduCnt;
    u16 u2TxRange5AmpduCnt;
    u16 u2TxRange6AmpduCnt;
    u16 u2TxRange7AmpduCnt;
    u16 u2TxRange8AmpduCnt;
} MIB_INFO_STAT_T, *P_MIB_INFO_STAT_T;

typedef struct _PARAM_GET_STA_STATISTICS {
    /* Per-STA statistic */
    u8 aucMacAddr[MAC_ADDR_LEN];

    u32 u4Flag;

    u8 ucReadClear;
    u8 ucLlsReadClear;

    /* From driver */
    u32 u4TxTotalCount;
    u32 u4TxExceedThresholdCount;

    u32 u4TxMaxTime;
    u32 u4TxAverageProcessTime;

    u32 u4RxTotalCount;

    u32 au4TcResourceEmptyCount[NUM_TC_RESOURCE_TO_STATISTICS];
    u32 au4TcQueLen[NUM_TC_RESOURCE_TO_STATISTICS];

    /* From FW */
    u8 ucPer;  /* base: 128 */
    u8 ucRcpi;
    u32 u4PhyMode;
    u16 u2LinkSpeed;  /* unit is 0.5 Mbits */

    u32 u4TxFailCount;
    u32 u4TxLifeTimeoutCount;

    u32 u4TxAverageAirTime;
    u32 u4TransmitCount;  /* Transmit in the air (wtbl) */
    u32 u4TransmitFailCount;  /* Transmit without ack/ba in the air (wtbl) */

    WIFI_WMM_AC_STAT_T arLinkStatistics[AC_NUM];  /*link layer statistics */

    /* Global queue management statistic */
    u32 au4TcAverageQueLen[NUM_TC_RESOURCE_TO_STATISTICS];
    u32 au4TcCurrentQueLen[NUM_TC_RESOURCE_TO_STATISTICS];

    u8 ucTemperature;
    u8 ucSkipAr;
    u8 ucArTableIdx;
    u8 ucRateEntryIdx;
    u8 ucRateEntryIdxPrev;
    u8 ucTxSgiDetectPassCnt;
    u8 ucAvePer;
    u8 aucArRatePer[AR_RATE_TABLE_ENTRY_MAX];
    u8 aucRateEntryIndex[AUTO_RATE_NUM];
    u8 ucArStateCurr;
    u8 ucArStatePrev;
    u8 ucArActionType;
    u8 ucHighestRateCnt;
    u8 ucLowestRateCnt;
    u16 u2TrainUp;
    u16 u2TrainDown;
    u32 u4Rate1TxCnt;
    u32 u4Rate1FailCnt;
    TX_VECTOR_BBP_LATCH_T rTxVector[ENUM_BAND_NUM];
    MIB_INFO_STAT_T rMibInfo[ENUM_BAND_NUM];
    u8 ucResetCounter;
    u8 fgIsForceTxStream;
    u8 fgIsForceSeOff;

    /* Reserved fields */
    u8 au4Reserved[20];
} PARAM_GET_STA_STATISTICS, *P_PARAM_GET_STA_STATISTICS;

typedef struct _PARAM_GET_BSS_STATISTICS {
    /* Per-STA statistic */
    u8 aucMacAddr[MAC_ADDR_LEN];

    u32 u4Flag;

    u8 ucReadClear;

    u8 ucLlsReadClear;

    u8 ucBssIndex;

    /* From driver */
    u32 u4TxTotalCount;
    u32 u4TxExceedThresholdCount;

    u32 u4TxMaxTime;
    u32 u4TxAverageProcessTime;

    u32 u4RxTotalCount;

    u32 au4TcResourceEmptyCount[NUM_TC_RESOURCE_TO_STATISTICS];
    u32 au4TcQueLen[NUM_TC_RESOURCE_TO_STATISTICS];

    /* From FW */
    u8 ucPer;  /* base: 128 */
    u8 ucRcpi;
    u32 u4PhyMode;
    u16 u2LinkSpeed;  /* unit is 0.5 Mbits */

    u32 u4TxFailCount;
    u32 u4TxLifeTimeoutCount;

    u32 u4TxAverageAirTime;
    u32 u4TransmitCount;  /* Transmit in the air (wtbl) */
    u32 u4TransmitFailCount;  /* Transmit without ack/ba in the air (wtbl) */

    WIFI_WMM_AC_STAT_T arLinkStatistics[AC_NUM];  /*link layer statistics */

    /* Global queue management statistic */
    u32 au4TcAverageQueLen[NUM_TC_RESOURCE_TO_STATISTICS];
    u32 au4TcCurrentQueLen[NUM_TC_RESOURCE_TO_STATISTICS];

    /* Reserved fields */
    u8 au4Reserved[32];  /* insufficient for LLS?? */
} PARAM_GET_BSS_STATISTICS, *P_PARAM_GET_BSS_STATISTICS;

typedef struct _PARAM_GET_DRV_STATISTICS {
    s32 i4TxPendingFrameNum;
    s32 i4TxPendingSecurityFrameNum;
    s32 i4TxPendingCmdNum;
    s32 i4PendingFwdFrameCount;  /* sync i4PendingFwdFrameCount in _TX_CTRL_T
                                  */
    u32 u4MsduNumElem;  /* sync pad->rTxCtrl.rFreeMsduInfoList.u4NumElem */
    u32 u4TxMgmtTxringQueueNumElem;  /* sync
                                      * pad->rTxCtrl.rTxMgmtTxingQueue.u4NumElem
                                      */

    u32 u4RxFreeSwRfbMsduNumElem;  /* sync
                                    * pad->prRxCtrl.rFreeSwRfbList.u4NumElem
                                    */
    u32 u4RxReceivedRfbNumElem;  /* sync
                                  * pad->prRxCtrl.rReceivedRfbList.u4NumElem
                                  */
    u32 u4RxIndicatedNumElem;  /* sync
                                * pad->prRxCtrl.rIndicatedRfbList.u4NumElem
                                */
} PARAM_GET_DRV_STATISTICS, *P_PARAM_GET_DRV_STATISTICS;

typedef struct _NET_INTERFACE_INFO_T {
    u8 ucBssIndex;
    void *pvNetInterface;
} NET_INTERFACE_INFO_T, *P_NET_INTERFACE_INFO_T;

typedef enum _ENUM_TX_RESULT_CODE_T {
    TX_RESULT_SUCCESS = 0,
    TX_RESULT_LIFE_TIMEOUT,
    TX_RESULT_RTS_ERROR,
    TX_RESULT_MPDU_ERROR,
    TX_RESULT_AGING_TIMEOUT,
    TX_RESULT_FLUSHED,
    TX_RESULT_BIP_ERROR,
    TX_RESULT_UNSPECIFIED_ERROR,
    TX_RESULT_DROPPED_IN_DRIVER = 32,
    TX_RESULT_DROPPED_IN_FW,
    TX_RESULT_QUEUE_CLEARANCE,
    TX_RESULT_UNINITIALIZED     = 48, // driver only
    TX_RESULT_1XTX_CLEAR,  // driver only
    TX_RESULT_NUM
} ENUM_TX_RESULT_CODE_T,
*P_ENUM_TX_RESULT_CODE_T;

struct _WLAN_CFG_ENTRY_T {
    u8 aucKey[WLAN_CFG_KEY_LEN_MAX];
    u8 aucValue[WLAN_CFG_VALUE_LEN_MAX];
    WLAN_CFG_SET_CB pfSetCb;
    void *pPrivate;
    u32 u4Flags;
};

struct _WLAN_CFG_T {
    u32 u4WlanCfgEntryNumMax;
    u32 u4WlanCfgKeyLenMax;
    u32 u4WlanCfgValueLenMax;
    WLAN_CFG_ENTRY_T arWlanCfgBuf[WLAN_CFG_ENTRY_NUM_MAX];
};

struct _WLAN_CFG_REC_T {
    u32 u4WlanCfgEntryNumMax;
    u32 u4WlanCfgKeyLenMax;
    u32 u4WlanCfgValueLenMax;
    WLAN_CFG_ENTRY_T arWlanCfgBuf[WLAN_CFG_REC_ENTRY_NUM_MAX];
};

typedef enum _ENUM_MAX_BANDWIDTH_SETTING_T {
    MAX_BW_20MHZ = 0,
    MAX_BW_40MHZ,
    MAX_BW_80MHZ,
    MAX_BW_160MHZ,
    MAX_BW_80_80_MHZ
} ENUM_MAX_BANDWIDTH_SETTING,
*P_ENUM_MAX_BANDWIDTH_SETTING_T;

typedef struct _TX_PACKET_INFO {
    u8 ucPriorityParam;
    u32 u4PacketLen;
    u8 aucEthDestAddr[MAC_ADDR_LEN];
    u16 u2Flag;
} TX_PACKET_INFO, *P_TX_PACKET_INFO;

typedef enum _ENUM_TX_PROFILING_TAG_T {
    TX_PROF_TAG_OS_TO_DRV = 0,
    TX_PROF_TAG_DRV_ENQUE,
    TX_PROF_TAG_DRV_DEQUE,
    TX_PROF_TAG_DRV_TX_DONE,
    TX_PROF_TAG_MAC_TX_DONE
} ENUM_TX_PROFILING_TAG_T,
*P_ENUM_TX_PROFILING_TAG_T;

enum ENUM_WF_PATH_FAVOR_T {
    ENUM_WF_NON_FAVOR                 = 0xff,
    ENUM_WF_0_ONE_STREAM_PATH_FAVOR   = 0,
    ENUM_WF_1_ONE_STREAM_PATH_FAVOR   = 1,
    ENUM_WF_0_1_TWO_STREAM_PATH_FAVOR = 2,
    ENUM_WF_0_1_DUP_STREAM_PATH_FAVOR = 3,
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

#define BUILD_SIGN(ch0, ch1, ch2, ch3)                                 \
    ((u32)(u8)(ch0) | ((u32)(u8)(ch1) << 8) | ((u32)(u8)(ch2) << 16) | \
     ((u32)(u8)(ch3) << 24))

#define MTK_WIFI_SIGNATURE    BUILD_SIGN('M', 'T', 'K', 'W')

#define IS_FEATURE_ENABLED(_ucFeature)    \
    (((_ucFeature) == FEATURE_ENABLED) || \
     ((_ucFeature) == FEATURE_FORCE_ENABLED))
#define IS_FEATURE_FORCE_ENABLED(_ucFeature) \
    ((_ucFeature) == FEATURE_FORCE_ENABLED)
#define IS_FEATURE_DISABLED(_ucFeature)       ((_ucFeature) == FEATURE_DISABLED)

/* This macro is for later added feature options which use command reserved
 * field as option switch */
/* 0: AUTO
 * 1: Disabled
 * 2: Enabled
 * 3: Force disabled
 */
#define FEATURE_OPT_IN_COMMAND(_ucFeature)    ((_ucFeature) + 1)

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

P_ADAPTER_T wlanAdapterCreate(IN P_GLUE_INFO_T prGlueInfo);

void wlanAdapterDestroy(IN P_ADAPTER_T prAdapter);

void wlanCardEjected(IN P_ADAPTER_T prAdapter);

void wlanIST(IN P_ADAPTER_T prAdapter);

u8 wlanISR(IN P_ADAPTER_T prAdapter, IN u8 fgGlobalIntrCtrl);

WLAN_STATUS wlanProcessCommandQueue(IN P_ADAPTER_T prAdapter,
                                    IN P_QUE_T prCmdQue);

WLAN_STATUS wlanSendCommand(IN P_ADAPTER_T prAdapter,
                            IN P_CMD_INFO_T prCmdInfo);

WLAN_STATUS wlanSendCommandMthread(IN P_ADAPTER_T prAdapter,
                                   IN P_CMD_INFO_T prCmdInfo);

WLAN_STATUS wlanTxCmdMthread(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanTxCmdDoneMthread(IN P_ADAPTER_T prAdapter);

void wlanClearTxCommandQueue(IN P_ADAPTER_T prAdapter);

void wlanClearTxCommandDoneQueue(IN P_ADAPTER_T prAdapter);

void wlanClearDataQueue(IN P_ADAPTER_T prAdapter);

void wlanClearRxToOsQueue(IN P_ADAPTER_T prAdapter);

void wlanClearPendingCommandQueue(IN P_ADAPTER_T prAdapter);

void wlanReleaseCommand(IN P_ADAPTER_T prAdapter,
                        IN P_CMD_INFO_T prCmdInfo,
                        IN ENUM_TX_RESULT_CODE_T rTxDoneStatus);

void wlanReleaseCommandEx(IN P_ADAPTER_T prAdapter,
                          IN P_CMD_INFO_T prCmdInfo,
                          IN ENUM_TX_RESULT_CODE_T rTxDoneStatus,
                          IN u8 fgIsNeedHandler);

void wlanReleasePendingOid(IN P_ADAPTER_T prAdapter,
                           IN unsigned long ulParamPtr);

void wlanReturnPacketDelaySetupTimeout(IN P_ADAPTER_T prAdapter,
                                       IN unsigned long ulParamPtr);

void wlanReturnPacket(IN P_ADAPTER_T prAdapter, IN void *pvPacket);

WLAN_STATUS
wlanQueryInformation(IN P_ADAPTER_T prAdapter,
                     IN PFN_OID_HANDLER_FUNC pfOidQryHandler,
                     IN void *pvInfoBuf,
                     IN u32 u4InfoBufLen,
                     OUT u32 *pu4QryInfoLen);

WLAN_STATUS
wlanSetInformation(IN P_ADAPTER_T prAdapter,
                   IN PFN_OID_HANDLER_FUNC pfOidSetHandler,
                   IN void *pvInfoBuf,
                   IN u32 u4InfoBufLen,
                   OUT u32 *pu4SetInfoLen);

WLAN_STATUS wlanAdapterStart(IN P_ADAPTER_T prAdapter,
                             IN P_REG_INFO_T prRegInfo);

WLAN_STATUS wlanAdapterStop(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanCheckWifiFunc(IN P_ADAPTER_T prAdapter, IN u8 fgRdyChk);

void wlanReturnRxPacket(IN void *pvAdapter, IN void *pvPacket);

void wlanRxSetBroadcast(IN P_ADAPTER_T prAdapter, IN u8 fgEnableBroadcast);

u8 wlanIsHandlerNeedHwAccess(IN PFN_OID_HANDLER_FUNC pfnOidHandler,
                             IN u8 fgSetInfo);

void wlanSetPromiscuousMode(IN P_ADAPTER_T prAdapter,
                            IN u8 fgEnablePromiscuousMode);
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
WLAN_STATUS
wlanImageSectionDownloadStage(IN P_ADAPTER_T prAdapter,
                              IN void *pvFwImageMapFile,
                              IN u32 u4FwImageFileLength,
                              u8 ucSectionNumber,
                              IN ENUM_IMG_DL_IDX_T eDlIdx,
                              OUT u8 *ucIsCompressed,
                              OUT P_INIT_CMD_WIFI_DECOMPRESSION_START
                              prFwImageInFo);
#else
WLAN_STATUS wlanImageSectionDownloadStage(IN P_ADAPTER_T prAdapter,
                                          IN void *pvFwImageMapFile,
                                          IN u32 u4FwImageFileLength,
                                          u8 ucSectionNumber,
                                          IN ENUM_IMG_DL_IDX_T eDlIdx);

#endif

#if CFG_ENABLE_FW_DOWNLOAD
WLAN_STATUS wlanImageSectionConfig(IN P_ADAPTER_T prAdapter,
                                   IN u32 u4DestAddr,
                                   IN u32 u4ImgSecSize,
                                   IN u32 u4DataMode,
                                   IN ENUM_IMG_DL_IDX_T eDlIdx);

WLAN_STATUS wlanImageSectionDownload(IN P_ADAPTER_T prAdapter,
                                     IN u32 u4ImgSecSize,
                                     IN u8 *pucImgSecBuf);

WLAN_STATUS wlanImageQueryStatus(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanImageSectionDownloadStatus(IN P_ADAPTER_T prAdapter,
                                           IN u8 ucCmdSeqNum);
#define wlanConfigWifiFuncStatus    wlanImageSectionDownloadStatus

WLAN_STATUS wlanConfigWifiFunc(IN P_ADAPTER_T prAdapter,
                               IN u8 fgEnable,
                               IN u32 u4StartAddress,
                               IN u8 ucPDA);

u32 wlanCRC32(u8 *buf, u32 len);
#endif

WLAN_STATUS wlanSendDummyCmd(IN P_ADAPTER_T prAdapter, IN u8 fgIsReqTxRsrc);

WLAN_STATUS wlanSendNicPowerCtrlCmd(IN P_ADAPTER_T prAdapter,
                                    IN u8 ucPowerMode);

WLAN_STATUS wlanKeepFullPwr(IN P_ADAPTER_T prAdapter, IN u8 fgEnable);

u8 wlanIsHandlerAllowedInRFTest(IN PFN_OID_HANDLER_FUNC pfnOidHandler,
                                IN u8 fgSetInfo);

WLAN_STATUS wlanProcessQueuedSwRfb(IN P_ADAPTER_T prAdapter,
                                   IN P_SW_RFB_T prSwRfbListHead);

WLAN_STATUS wlanProcessQueuedMsduInfo(IN P_ADAPTER_T prAdapter,
                                      IN P_MSDU_INFO_T prMsduInfoListHead);

u8 wlanoidTimeoutCheck(IN P_ADAPTER_T prAdapter,
                       IN PFN_OID_HANDLER_FUNC pfnOidHandler,
                       IN u32 u4Timeout);

void wlanoidClearTimeoutCheck(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanUpdateNetworkAddress(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanUpdateBasicConfig(IN P_ADAPTER_T prAdapter);

u8 wlanQueryTestMode(IN P_ADAPTER_T prAdapter);

u8 wlanProcessTxFrame(IN P_ADAPTER_T prAdapter, IN P_NATIVE_PACKET prPacket);

/* Security Frame Handling */
u8 wlanProcessSecurityFrame(IN P_ADAPTER_T prAdapter,
                            IN P_NATIVE_PACKET prPacket);

void wlanSecurityFrameTxDone(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo,
                             IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen);

void wlanSecurityFrameTxTimeout(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo);

/*----------------------------------------------------------------------------*/
/* OID/IOCTL Handling                                                         */
/*----------------------------------------------------------------------------*/
void wlanClearScanningResult(IN P_ADAPTER_T prAdapter);

void wlanClearBssInScanningResult(IN P_ADAPTER_T prAdapter, IN u8 *arBSSID);

#if CFG_TEST_WIFI_DIRECT_GO
void wlanEnableP2pFunction(IN P_ADAPTER_T prAdapter);

void wlanEnableATGO(IN P_ADAPTER_T prAdapter);
#endif

/*----------------------------------------------------------------------------*/
/* NIC Capability Retrieve by Polling                                         */
/*----------------------------------------------------------------------------*/
WLAN_STATUS wlanQueryNicCapability(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* PD MCR Retrieve by Polling                                                 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS wlanQueryPdMcr(IN P_ADAPTER_T prAdapter,
                           IN P_PARAM_MCR_RW_STRUCT_T prMcrRdInfo);

/*----------------------------------------------------------------------------*/
/* Loading Manufacture Data                                                   */
/*----------------------------------------------------------------------------*/
WLAN_STATUS wlanLoadManufactureData(IN P_ADAPTER_T prAdapter,
                                    IN P_REG_INFO_T prRegInfo);

/*----------------------------------------------------------------------------*/
/* Media Stream Mode                                                          */
/*----------------------------------------------------------------------------*/
u8 wlanResetMediaStreamMode(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* Timer Timeout Check (for Glue Layer)                                       */
/*----------------------------------------------------------------------------*/
WLAN_STATUS wlanTimerTimeoutCheck(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* Mailbox Message Check (for Glue Layer)                                     */
/*----------------------------------------------------------------------------*/
WLAN_STATUS wlanProcessMboxMessage(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* TX Pending Packets Handling (for Glue Layer)                               */
/*----------------------------------------------------------------------------*/
WLAN_STATUS wlanEnqueueTxPacket(IN P_ADAPTER_T prAdapter,
                                IN P_NATIVE_PACKET prNativePacket);

WLAN_STATUS wlanFlushTxPendingPackets(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanTxPendingPackets(IN P_ADAPTER_T prAdapter,
                                 IN OUT u8 *pfgHwAccess);

/*----------------------------------------------------------------------------*/
/* Low Power Acquire/Release (for Glue Layer)                                 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS wlanAcquirePowerControl(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanReleasePowerControl(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* Pending Packets Number Reporting (for Glue Layer)                          */
/*----------------------------------------------------------------------------*/
u32 wlanGetTxPendingFrameCount(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* ACPI state inquiry (for Glue Layer)                                        */
/*----------------------------------------------------------------------------*/
ENUM_ACPI_STATE_T wlanGetAcpiState(IN P_ADAPTER_T prAdapter);

void wlanSetAcpiState(IN P_ADAPTER_T prAdapter,
                      IN ENUM_ACPI_STATE_T ePowerState);

void wlanDefTxPowerCfg(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* get ECO version from Revision ID register (for Win32)                      */
/*----------------------------------------------------------------------------*/
u8 wlanGetEcoVersion(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* get Rom version                     */
/*----------------------------------------------------------------------------*/
u8 wlanGetRomVersion(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* set preferred band configuration corresponding to network type             */
/*----------------------------------------------------------------------------*/
void wlanSetPreferBandByNetwork(IN P_ADAPTER_T prAdapter,
                                IN ENUM_BAND_T eBand,
                                IN u8 ucBssIndex);

/*----------------------------------------------------------------------------*/
/* get currently operating channel information                                */
/*----------------------------------------------------------------------------*/
u8 wlanGetChannelNumberByNetwork(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex);

/*----------------------------------------------------------------------------*/
/* check for system configuration to generate message on scan list            */
/*----------------------------------------------------------------------------*/
WLAN_STATUS wlanCheckSystemConfiguration(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* query bss statistics information from driver and firmware                  */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidQueryBssStatistics(IN P_ADAPTER_T prAdapter,
                          IN void *pvQueryBuffer,
                          IN u32 u4QueryBufferLen,
                          OUT u32 *pu4QueryInfoLen);

/*----------------------------------------------------------------------------*/
/* dump per-BSS statistics            */
/*----------------------------------------------------------------------------*/
void wlanDumpBssStatistics(IN P_ADAPTER_T prAdapter, u8 ucBssIndex);

/*----------------------------------------------------------------------------*/
/* query sta statistics information from driver and firmware                  */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidQueryStaStatistics(IN P_ADAPTER_T prAdapter,
                          IN void *pvQueryBuffer,
                          IN u32 u4QueryBufferLen,
                          OUT u32 *pu4QueryInfoLen);

/*----------------------------------------------------------------------------*/
/* query NIC resource information from chip and reset Tx resource for normal
 * operation        */
/*----------------------------------------------------------------------------*/
void wlanQueryNicResourceInformation(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanQueryNicCapabilityV2(IN P_ADAPTER_T prAdapter);

void wlanUpdateNicResourceInformation(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* GET/SET BSS index mapping for network interfaces */
/*----------------------------------------------------------------------------*/
void wlanBindNetInterface(IN P_GLUE_INFO_T prGlueInfo,
                          IN u8 ucNetInterfaceIndex,
                          IN void *pvNetInterface);

void wlanBindBssIdxToNetInterface(IN P_GLUE_INFO_T prGlueInfo,
                                  IN u8 ucBssIndex,
                                  IN void *pvNetInterface);

u8 wlanGetBssIdxByNetInterface(IN P_GLUE_INFO_T prGlueInfo,
                               IN void *pvNetInterface);

void *wlanGetNetInterfaceByBssIdx(IN P_GLUE_INFO_T prGlueInfo,
                                  IN u8 ucBssIndex);

/* for windows as windows glue cannot see through P_ADAPTER_T */
u8 wlanGetAisBssIndex(IN P_ADAPTER_T prAdapter);

void wlanInitFeatureOption(IN P_ADAPTER_T prAdapter);

void wlanCfgSetSwCtrl(IN P_ADAPTER_T prAdapter);

void wlanCfgSetChip(IN P_ADAPTER_T prAdapter);

void wlanCfgSetDebugLevel(IN P_ADAPTER_T prAdapter);

void wlanCfgSetCountryCode(IN P_ADAPTER_T prAdapter);

P_WLAN_CFG_ENTRY_T wlanCfgGetEntry(IN P_ADAPTER_T prAdapter,
                                   const s8 *pucKey,
                                   u8 fgGetCfgRec);

WLAN_STATUS
wlanCfgGet(IN P_ADAPTER_T prAdapter,
           const s8 *pucKey,
           s8 *pucValue,
           s8 *pucValueDef,
           u32 u4Flags);

u32 wlanCfgGetUint32(IN P_ADAPTER_T prAdapter, const s8 *pucKey,
                     u32 u4ValueDef);

s32 wlanCfgGetInt32(IN P_ADAPTER_T prAdapter, const s8 *pucKey, s32 i4ValueDef);

WLAN_STATUS wlanCfgSetUint32(IN P_ADAPTER_T prAdapter,
                             const s8 *pucKey,
                             u32 u4Value);

WLAN_STATUS wlanCfgSet(IN P_ADAPTER_T prAdapter,
                       const s8 *pucKey,
                       s8 *pucValue,
                       u32 u4Flags);

WLAN_STATUS
wlanCfgSetCb(IN P_ADAPTER_T prAdapter,
             const s8 *pucKey,
             WLAN_CFG_SET_CB pfSetCb,
             void *pPrivate,
             u32 u4Flags);

#if CFG_SUPPORT_EASY_DEBUG

WLAN_STATUS wlanCfgParse(IN P_ADAPTER_T prAdapter,
                         u8 *pucConfigBuf,
                         u32 u4ConfigBufLen,
                         u8 isFwConfig);
void wlanFeatureToFw(IN P_ADAPTER_T prAdapter);
#if CFG_SUPPORT_SEND_ONLY_ONE_CFG
WLAN_STATUS wlanFeatureToFwOnlyOneCfg(IN P_ADAPTER_T prAdapter,
                                      const s8 *pucKey,
                                      s8 *pucValue);
#endif
#endif

void wlanLoadDefaultCustomerSetting(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanCfgInit(IN P_ADAPTER_T prAdapter,
                        u8 *pucConfigBuf,
                        u32 u4ConfigBufLen,
                        u32 u4Flags);

WLAN_STATUS wlanCfgParseArgument(s8 *cmdLine, s32 *argc, s8 *argv[]);

#if CFG_WOW_SUPPORT
WLAN_STATUS wlanCfgParseArgumentLong(s8 *cmdLine, s32 *argc, s8 *argv[]);
#endif

s32 wlanHexToNum(s8 c);
s32 wlanHexToByte(s8 *hex);

s32 wlanHwAddrToBin(s8 *txt, u8 *addr);

u8 wlanIsChipNoAck(IN P_ADAPTER_T prAdapter);

u8 wlanIsChipRstRecEnabled(IN P_ADAPTER_T prAdapter);

u8 wlanIsChipAssert(IN P_ADAPTER_T prAdapter);

void wlanChipRstPreAct(IN P_ADAPTER_T prAdapter);

void wlanTxProfilingTagPacket(IN P_ADAPTER_T prAdapter,
                              IN P_NATIVE_PACKET prPacket,
                              IN ENUM_TX_PROFILING_TAG_T eTag);

void wlanTxProfilingTagMsdu(IN P_ADAPTER_T prAdapter,
                            IN P_MSDU_INFO_T prMsduInfo,
                            IN ENUM_TX_PROFILING_TAG_T eTag);
#if CFG_ASSERT_DUMP
void wlanCorDumpTimerReset(IN P_ADAPTER_T prAdapter, u8 fgIsResetN9);

void wlanN9CorDumpTimeOut(IN P_ADAPTER_T prAdapter,
                          IN unsigned long ulParamPtr);

void wlanCr4CorDumpTimeOut(IN P_ADAPTER_T prAdapter,
                           IN unsigned long ulParamPtr);
#endif
#endif

u8 wlanGetWlanIdxByAddress(IN P_ADAPTER_T prAdapter,
                           IN u8 *pucAddr,
                           OUT u8 *pucIndex);

u8 *wlanGetStaAddrByWlanIdx(IN P_ADAPTER_T prAdapter, IN u8 ucIndex);

P_WLAN_CFG_ENTRY_T wlanCfgGetEntryByIndex(IN P_ADAPTER_T prAdapter,
                                          const u8 ucIdx,
                                          u32 flag);

WLAN_STATUS wlanGetStaIdxByWlanIdx(IN P_ADAPTER_T prAdapter,
                                   IN u8 ucIndex,
                                   OUT u8 *pucStaIdx);

/*----------------------------------------------------------------------------*/
/* update per-AC statistics for LLS                */
/*----------------------------------------------------------------------------*/
void wlanUpdateTxStatistics(IN P_ADAPTER_T prAdapter,
                            IN P_MSDU_INFO_T prMsduInfo,
                            u8 fgTxDrop);

void wlanUpdateRxStatistics(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

WLAN_STATUS wlanTriggerStatsLog(IN P_ADAPTER_T prAdapter,
                                IN u32 u4DurationInMs);

WLAN_STATUS wlanDhcpTxDone(IN P_ADAPTER_T prAdapter,
                           IN P_MSDU_INFO_T prMsduInfo,
                           IN ENUM_TX_RESULT_CODE_T rTxDoneStatus);

WLAN_STATUS wlanArpTxDone(IN P_ADAPTER_T prAdapter,
                          IN P_MSDU_INFO_T prMsduInfo,
                          IN ENUM_TX_RESULT_CODE_T rTxDoneStatus);

WLAN_STATUS wlan1xTxDone(IN P_ADAPTER_T prAdapter,
                         IN P_MSDU_INFO_T prMsduInfo,
                         IN ENUM_TX_RESULT_CODE_T rTxDoneStatus);

WLAN_STATUS wlanDownloadFW(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanDownloadPatch(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanGetPatchInfo(IN P_ADAPTER_T prAdapter);

WLAN_STATUS wlanPowerOffWifi(IN P_ADAPTER_T prAdapter);

void wlanPrintVersion(P_ADAPTER_T prAdapter);
WLAN_STATUS wlanAccessRegister(IN P_ADAPTER_T prAdapter,
                               IN u32 u4Addr,
                               IN u32 *pru4Result,
                               IN u32 u4Data,
                               IN u8 ucSetQuery);

WLAN_STATUS wlanAccessRegisterStatus(IN P_ADAPTER_T prAdapter,
                                     IN u8 ucCmdSeqNum,
                                     IN u8 ucSetQuery,
                                     IN void *prEvent,
                                     IN u32 u4EventLen);

WLAN_STATUS wlanSetChipEcoInfo(IN P_ADAPTER_T prAdapter);

#if CFG_STR_DHCP_RENEW_OFFLOAD
void wlanSetDhcpOffloadInfo(P_GLUE_INFO_T prGlueInfo,
                            struct net_device *prDev,
                            u8 fgSuspend);
#endif

void wlanNotifyFwSuspend(P_GLUE_INFO_T prGlueInfo,
                         struct net_device *prDev,
                         u8 fgSuspend);

void wlanClearPendingInterrupt(IN P_ADAPTER_T prAdapter);

u8 wlanGetAntPathType(IN P_ADAPTER_T prAdapter,
                      IN enum ENUM_WF_PATH_FAVOR_T eWfPathFavor,
                      IN u8 ucBssIndex);

u8 wlanGetSpeIdx(IN P_ADAPTER_T prAdapter,
                 IN u8 ucBssIndex,
                 IN enum ENUM_WF_PATH_FAVOR_T eWfPathFavor);

u8 wlanGetSupportNss(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex);

s32 wlanGetFileContent(P_ADAPTER_T prAdapter,
                       const u8 *pcFileName,
                       u8 *pucBuf,
                       u32 u4MaxFileLen,
                       u32 *pu4ReadFileLen,
                       u8 bReqFw);

#if CFG_SUPPORT_ANT_SELECT
WLAN_STATUS wlanUpdateExtInfo(IN P_ADAPTER_T prAdapter);
#endif

#if CFG_SUPPORT_RSSI_COMP
WLAN_STATUS wlanUpdateRssiComp(IN P_ADAPTER_T prAdapter);
#endif

WLAN_STATUS wlanSetEd(IN P_ADAPTER_T prAdapter,
                      s32 u4EdVal2G,
                      s32 u4EdVal5G,
                      u32 u4Sel);
int wlanSuspendRekeyOffload(P_GLUE_INFO_T prGlueInfo, IN u8 ucRekeyMode);
void wlanDisTrafficReport(P_GLUE_INFO_T prGlueInfo);
void wlanSuspendPmHandle(P_GLUE_INFO_T prGlueInfo);
void wlanResumePmHandle(P_GLUE_INFO_T prGlueInfo);

void disconnect_sta(P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec);
