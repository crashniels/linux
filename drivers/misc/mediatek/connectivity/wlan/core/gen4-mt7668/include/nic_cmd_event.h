/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "nic_cmd_event.h"
 *    \brief This file contains the declairation file of the WLAN OID processing
 * routines of Windows driver for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */

#ifndef _NIC_CMD_EVENT_H
#define _NIC_CMD_EVENT_H

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

#define CMD_PQ_ID          (0x8000)
#define CMD_PACKET_TYPE_ID     (0xA0)

#define PKT_FT_CMD         0x2

#define CMD_STATUS_SUCCESS     0
#define CMD_STATUS_REJECTED    1
#define CMD_STATUS_UNKNOWN     2

#define EVENT_HDR_WITHOUT_RXD_SIZE           \
    (OFFSET_OF(WIFI_EVENT_T, aucBuffer[0]) - \
     OFFSET_OF(WIFI_EVENT_T, u2PacketLength))

#define MAX_IE_LENGTH          (600)
#define MAX_WSC_IE_LENGTH      (400)

/* Action field in structure CMD_CH_PRIVILEGE_T */
#define CMD_CH_ACTION_REQ      0
#define CMD_CH_ACTION_ABORT    1

/* Status field in structure EVENT_CH_PRIVILEGE_T */
#define EVENT_CH_STATUS_GRANT      0

/*CMD_POWER_OFFSET_T , follow 5G sub-band*/
/* #define MAX_SUBBAND_NUM             8 */
/*  */
/*  */
/*  */
/*  */
#define S2D_INDEX_CMD_H2N      0x0
#define S2D_INDEX_CMD_C2N      0x1
#define S2D_INDEX_CMD_H2C      0x2
#define S2D_INDEX_CMD_H2N_H2C      0x3

#define S2D_INDEX_EVENT_N2H    0x0
#define S2D_INDEX_EVENT_N2C    0x1
#define S2D_INDEX_EVENT_C2H    0x2
#define S2D_INDEX_EVENT_N2H_N2C    0x3

#define RDD_EVENT_HDR_SIZE     20
#define RDD_ONEPLUSE_SIZE      8  /* size of one pulse is 8 bytes */
#define RDD_PULSE_OFFSET0      0
#define RDD_PULSE_OFFSET1      1
#define RDD_PULSE_OFFSET2      2
#define RDD_PULSE_OFFSET3      3
#define RDD_PULSE_OFFSET4      4
#define RDD_PULSE_OFFSET5      5
#define RDD_PULSE_OFFSET6      6
#define RDD_PULSE_OFFSET7      7

#if (CFG_SUPPORT_DFS_MASTER == 1)
#define RDD_IN_SEL_0           0
#define RDD_IN_SEL_1           1
#define PPB_SIZE           32
#define LPB_SIZE           32
#endif

#if CFG_SUPPORT_QA_TOOL
#define IQ_FILE_LINE_OFFSET    18
#define IQ_FILE_IQ_STR_LEN     8
#define RTN_IQ_DATA_LEN        1024  /* return 1k per packet */

#define MCAST_WCID_TO_REMOVE       0

/* Network type */
#define NETWORK_INFRA          BIT(16)
#define NETWORK_P2P        BIT(17)
#define NETWORK_IBSS           BIT(18)
#define NETWORK_MESH           BIT(19)
#define NETWORK_BOW        BIT(20)
#define NETWORK_WDS        BIT(21)

/* Station role */
#define STA_TYPE_STA           BIT(0)
#define STA_TYPE_AP        BIT(1)
#define STA_TYPE_ADHOC         BIT(2)
#define STA_TYPE_TDLS          BIT(3)
#define STA_TYPE_WDS           BIT(4)

/* Connection type */
#define CONNECTION_INFRA_STA       (STA_TYPE_STA | NETWORK_INFRA)
#define CONNECTION_INFRA_AP    (STA_TYPE_AP | NETWORK_INFRA)
#define CONNECTION_P2P_GC      (STA_TYPE_STA | NETWORK_P2P)
#define CONNECTION_P2P_GO      (STA_TYPE_AP | NETWORK_P2P)
#define CONNECTION_MESH_STA    (STA_TYPE_STA | NETWORK_MESH)
#define CONNECTION_MESH_AP     (STA_TYPE_AP | NETWORK_MESH)
#define CONNECTION_IBSS_ADHOC      (STA_TYPE_ADHOC | NETWORK_IBSS)
#define CONNECTION_TDLS        (STA_TYPE_STA | NETWORK_INFRA | \
                                STA_TYPE_TDLS)
#define CONNECTION_WDS         (STA_TYPE_WDS | NETWORK_WDS)

#define ICAP_CONTENT_ADC       0x10000006
#define ICAP_CONTENT_TOAE      0x7
#define ICAP_CONTENT_SPECTRUM      0xB
#define ICAP_CONTENT_RBIST     0x10
#define ICAP_CONTENT_DCOC      0x20
#define ICAP_CONTENT_FIIQ      0x48
#define ICAP_CONTENT_FDIQ      0x49

#define MAX_MEMORY_DUMP_SIZE \
    4096  /* Same setting as MT7668 FW: hemFsmEventCmdDumpMem() */

#if CFG_SUPPORT_BUFFER_MODE

typedef struct _CMD_EFUSE_BUFFER_MODE_T {
    u8 ucSourceMode;
    u8 ucCount;
    u8 ucCmdType;  /* 0:6632, 1: 7668 */
    u8 ucReserved;
    u8 aBinContent[MAX_EEPROM_BUFFER_SIZE];
} CMD_EFUSE_BUFFER_MODE_T, *P_CMD_EFUSE_BUFFER_MODE_T;

/*#if (CFG_EEPROM_PAGE_ACCESS == 1)*/
typedef struct _CMD_ACCESS_EFUSE_T {
    u32 u4Address;
    u32 u4Valid;
    u8 aucData[16];
} CMD_ACCESS_EFUSE_T, *P_CMD_ACCESS_EFUSE_T;

typedef struct _CMD_EFUSE_FREE_BLOCK_T {
    u8 ucGetFreeBlock;
    u8 aucReserved[3];
} CMD_EFUSE_FREE_BLOCK_T, *P_CMD_EFUSE_FREE_BLOCK_T;

typedef struct _CMD_GET_TX_POWER_T {
    u8 ucTxPwrType;
    u8 ucCenterChannel;
    u8 ucDbdcIdx;  /* 0:Band 0, 1: Band1 */
    u8 ucBand;  /* 0:G-band 1: A-band*/
    u8 ucReserved[4];
} CMD_GET_TX_POWER_T, *P_CMD_GET_TX_POWER_T;

/*#endif*/

#endif

typedef struct _CMD_SET_TX_TARGET_POWER_T {
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
} CMD_SET_TX_TARGET_POWER_T, *P_CMD_SET_TX_TARGET_POWER_T;

/*
 * Definitions for extension CMD_ID
 */
typedef enum _ENUM_EXT_CMD_ID_T {
    EXT_CMD_ID_EFUSE_ACCESS             = 0x01,
    EXT_CMD_ID_RF_REG_ACCESS            = 0x02,
    EXT_CMD_ID_EEPROM_ACCESS            = 0x03,
    EXT_CMD_ID_RF_TEST                  = 0x04,
    EXT_CMD_ID_RADIO_ON_OFF_CTRL        = 0x05,
    EXT_CMD_ID_WIFI_RX_DISABLE          = 0x06,
    EXT_CMD_ID_PM_STATE_CTRL            = 0x07,
    EXT_CMD_ID_CHANNEL_SWITCH           = 0x08,
    EXT_CMD_ID_NIC_CAPABILITY           = 0x09,
    EXT_CMD_ID_AP_PWR_SAVING_CLEAR      = 0x0A,
    EXT_CMD_ID_SET_WTBL2_RATETABLE      = 0x0B,
    EXT_CMD_ID_GET_WTBL_INFORMATION     = 0x0C,
    EXT_CMD_ID_ASIC_INIT_UNINIT_CTRL    = 0x0D,
    EXT_CMD_ID_MULTIPLE_REG_ACCESS      = 0x0E,
    EXT_CMD_ID_AP_PWR_SAVING_CAPABILITY = 0x0F,
    EXT_CMD_ID_SECURITY_ADDREMOVE_KEY   = 0x10,
    EXT_CMD_ID_SET_TX_POWER_CONTROL     = 0x11,
    EXT_CMD_ID_SET_THERMO_CALIBRATION   = 0x12,
    EXT_CMD_ID_FW_LOG_2_HOST            = 0x13,
    EXT_CMD_ID_AP_PWR_SAVING_START      = 0x14,
    EXT_CMD_ID_MCC_OFFLOAD_START        = 0x15,
    EXT_CMD_ID_MCC_OFFLOAD_STOP         = 0x16,
    EXT_CMD_ID_LED                      = 0x17,
    EXT_CMD_ID_PACKET_FILTER            = 0x18,
    EXT_CMD_ID_COEXISTENCE              = 0x19,
    EXT_CMD_ID_PWR_MGT_BIT_WIFI         = 0x1B,
    EXT_CMD_ID_GET_TX_POWER             = 0x1C,
    EXT_CMD_ID_BF_ACTION                = 0x1E,

    EXT_CMD_ID_WMT_CMD_OVER_WIFI        = 0x20,
    EXT_CMD_ID_EFUSE_BUFFER_MODE        = 0x21,
    EXT_CMD_ID_OFFLOAD_CTRL             = 0x22,
    EXT_CMD_ID_THERMAL_PROTECT          = 0x23,
    EXT_CMD_ID_CLOCK_SWITCH_DISABLE     = 0x24,
    EXT_CMD_ID_STAREC_UPDATE            = 0x25,
    EXT_CMD_ID_BSSINFO_UPDATE           = 0x26,
    EXT_CMD_ID_EDCA_SET                 = 0x27,
    EXT_CMD_ID_SLOT_TIME_SET            = 0x28,
    EXT_CMD_ID_DEVINFO_UPDATE           = 0x2A,
    EXT_CMD_ID_NOA_OFFLOAD_CTRL         = 0x2B,
    EXT_CMD_ID_GET_SENSOR_RESULT        = 0x2C,
    EXT_CMD_ID_TMR_CAL                  = 0x2D,
    EXT_CMD_ID_WAKEUP_OPTION            = 0x2E,
    EXT_CMD_ID_OBTW                     = 0x2F,

    EXT_CMD_ID_GET_TX_STATISTICS        = 0x30,
    EXT_CMD_ID_AC_QUEUE_CONTROL         = 0x31,
    EXT_CMD_ID_WTBL_UPDATE              = 0x32,
    EXT_CMD_ID_BCN_UPDATE               = 0x33,

    EXT_CMD_ID_DRR_CTRL                 = 0x36,
    EXT_CMD_ID_BSSGROUP_CTRL            = 0x37,
    EXT_CMD_ID_VOW_FEATURE_CTRL         = 0x38,
    EXT_CMD_ID_PKT_PROCESSOR_CTRL       = 0x39,
    EXT_CMD_ID_PALLADIUM                = 0x3A,
#if CFG_SUPPORT_MU_MIMO
    EXT_CMD_ID_MU_CTRL                  = 0x40,
#endif

    EXT_CMD_ID_EFUSE_FREE_BLOCK         = 0x4F
} ENUM_EXT_CMD_ID_T,
*P_ENUM_EXT_CMD_ID_T;

typedef enum _NDIS_802_11_WEP_STATUS {
    Ndis802_11WEPEnabled,
    Ndis802_11Encryption1Enabled     = Ndis802_11WEPEnabled,
    Ndis802_11WEPDisabled,
    Ndis802_11EncryptionDisabled     = Ndis802_11WEPDisabled,
    Ndis802_11WEPKeyAbsent,
    Ndis802_11Encryption1KeyAbsent   = Ndis802_11WEPKeyAbsent,
    Ndis802_11WEPNotSupported,
    Ndis802_11EncryptionNotSupported = Ndis802_11WEPNotSupported,
    Ndis802_11TKIPEnable,
    Ndis802_11Encryption2Enabled     = Ndis802_11TKIPEnable,
    Ndis802_11Encryption2KeyAbsent,
    Ndis802_11AESEnable,
    Ndis802_11Encryption3Enabled     = Ndis802_11AESEnable,
    Ndis802_11CCMP256Enable,
    Ndis802_11GCMP128Enable,
    Ndis802_11GCMP256Enable,
    Ndis802_11Encryption3KeyAbsent,
    Ndis802_11TKIPAESMix,
    Ndis802_11Encryption4Enabled = Ndis802_11TKIPAESMix,  /* TKIP or AES mix
                                                           */
    Ndis802_11Encryption4KeyAbsent,
    Ndis802_11GroupWEP40Enabled,
    Ndis802_11GroupWEP104Enabled,
#ifdef WAPI_SUPPORT
    Ndis802_11EncryptionSMS4Enabled,  /* WPI SMS4 support */
#endif
} NDIS_802_11_WEP_STATUS,
*PNDIS_802_11_WEP_STATUS, NDIS_802_11_ENCRYPTION_STATUS,
*PNDIS_802_11_ENCRYPTION_STATUS;

#if CFG_SUPPORT_MU_MIMO
enum {
    /* debug commands */
    MU_SET_ENABLE = 0,
    MU_GET_ENABLE,
    MU_SET_MUPROFILE_ENTRY,
    MU_GET_MUPROFILE_ENTRY,
    MU_SET_GROUP_TBL_ENTRY,
    MU_GET_GROUP_TBL_ENTRY,
    MU_SET_CLUSTER_TBL_ENTRY,
    MU_GET_CLUSTER_TBL_ENTRY,
    MU_SET_GROUP_USER_THRESHOLD,
    MU_GET_GROUP_USER_THRESHOLD,
    MU_SET_GROUP_NSS_THRESHOLD,
    MU_GET_GROUP_NSS_THRESHOLD,
    MU_SET_TXREQ_MIN_TIME,
    MU_GET_TXREQ_MIN_TIME,
    MU_SET_SU_NSS_CHECK,
    MU_GET_SU_NSS_CHECK,
    MU_SET_CALC_INIT_MCS,
    MU_GET_CALC_INIT_MCS,
    MU_SET_TXOP_DEFAULT,
    MU_GET_TXOP_DEFAULT,
    MU_SET_SU_LOSS_THRESHOLD,
    MU_GET_SU_LOSS_THRESHOLD,
    MU_SET_MU_GAIN_THRESHOLD,
    MU_GET_MU_GAIN_THRESHOLD,
    MU_SET_SECONDARY_AC_POLICY,
    MU_GET_SECONDARY_AC_POLICY,
    MU_SET_GROUP_TBL_DMCS_MASK,
    MU_GET_GROUP_TBL_DMCS_MASK,
    MU_SET_MAX_GROUP_SEARCH_CNT,
    MU_GET_MAX_GROUP_SEARCH_CNT,
    MU_GET_MU_PROFILE_TX_STATUS_CNT,
    MU_SET_TRIGGER_MU_TX,
    /* F/W flow test commands */
    MU_SET_TRIGGER_GID_MGMT_FRAME,
    /* HQA commands */
    MU_HQA_SET_STA_PARAM,
    MU_HQA_SET_ENABLE,
    MU_HQA_SET_SNR_OFFSET,
    MU_HQA_SET_ZERO_NSS,
    MU_HQA_SET_SPEED_UP_LQ,
    MU_HQA_SET_GROUP,
    MU_HQA_SET_MU_TABLE,
    MU_HQA_SET_CALC_LQ,
    MU_HQA_GET_CALC_LQ,
    MU_HQA_SET_CALC_INIT_MCS,
    MU_HQA_GET_CALC_INIT_MCS,
    MU_HQA_GET_QD,
};
#endif
#endif

typedef enum _ENUM_CMD_ID_T {
    CMD_ID_DUMMY_RSV = 0x00,  /* 0x00 (Set) */
    CMD_ID_TEST_CTRL = 0x01,  /* 0x01 (Set) */
    CMD_ID_BASIC_CONFIG,  /* 0x02 (Set) */
    CMD_ID_SCAN_REQ_V2,  /* 0x03 (Set) */
    CMD_ID_NIC_POWER_CTRL,  /* 0x04 (Set) */
    CMD_ID_POWER_SAVE_MODE,  /* 0x05 (Set) */
    CMD_ID_LINK_ATTRIB,  /* 0x06 (Set) */
    CMD_ID_ADD_REMOVE_KEY,  /* 0x07 (Set) */
    CMD_ID_DEFAULT_KEY_ID,  /* 0x08 (Set) */
    CMD_ID_INFRASTRUCTURE,  /* 0x09 (Set) */
    CMD_ID_SET_RX_FILTER,  /* 0x0a (Set) */
    CMD_ID_DOWNLOAD_BUF,  /* 0x0b (Set) */
    CMD_ID_WIFI_START,  /* 0x0c (Set) */
    CMD_ID_CMD_BT_OVER_WIFI,  /* 0x0d (Set) */
    CMD_ID_SET_MEDIA_CHANGE_DELAY_TIME,  /* 0x0e (Set) */
    CMD_ID_SET_DOMAIN_INFO,  /* 0x0f (Set) */
    CMD_ID_SET_IP_ADDRESS,  /* 0x10 (Set) */
    CMD_ID_BSS_ACTIVATE_CTRL,  /* 0x11 (Set) */
    CMD_ID_SET_BSS_INFO,  /* 0x12 (Set) */
    CMD_ID_UPDATE_STA_RECORD,  /* 0x13 (Set) */
    CMD_ID_REMOVE_STA_RECORD,  /* 0x14 (Set) */
    CMD_ID_INDICATE_PM_BSS_CREATED,  /* 0x15 (Set) */
    CMD_ID_INDICATE_PM_BSS_CONNECTED,  /* 0x16 (Set) */
    CMD_ID_INDICATE_PM_BSS_ABORT,  /* 0x17 (Set) */
    CMD_ID_UPDATE_BEACON_CONTENT,  /* 0x18 (Set) */
    CMD_ID_SET_BSS_RLM_PARAM,  /* 0x19 (Set) */
    CMD_ID_SCAN_REQ,  /* 0x1a (Set) */
    CMD_ID_SCAN_CANCEL,  /* 0x1b (Set) */
    CMD_ID_CH_PRIVILEGE,  /* 0x1c (Set) */
    CMD_ID_UPDATE_WMM_PARMS,  /* 0x1d (Set) */
    CMD_ID_SET_WMM_PS_TEST_PARMS,  /* 0x1e (Set) */
    CMD_ID_TX_AMPDU,  /* 0x1f (Set) */
    CMD_ID_ADDBA_REJECT,  /* 0x20 (Set) */
    CMD_ID_SET_PS_PROFILE_ADV,  /* 0x21 (Set) */
    CMD_ID_SET_RAW_PATTERN,  /* 0x22 (Set) */
    CMD_ID_CONFIG_PATTERN_FUNC,  /* 0x23 (Set) */
    CMD_ID_SET_TX_PWR,  /* 0x24 (Set) */
    CMD_ID_SET_PWR_PARAM,  /* 0x25 (Set) */
    CMD_ID_P2P_ABORT,  /* 0x26 (Set) */
    CMD_ID_SET_DBDC_PARMS     = 0x28, /* 0x28 (Set) */

    CMD_ID_KEEP_FULL_PWR      = 0x2A, /* 0x2A (Set) */

    /* SLT commands */
    CMD_ID_RANDOM_RX_RESET_EN = 0x2C,  /* 0x2C (Set ) */
    CMD_ID_RANDOM_RX_RESET_DE = 0x2D,  /* 0x2D (Set ) */
    CMD_ID_SAPP_EN            = 0x2E, /* 0x2E (Set ) */
    CMD_ID_SAPP_DE            = 0x2F, /* 0x2F (Set ) */

    CMD_ID_ROAMING_TRANSIT    = 0x30, /* 0x30 (Set) */
    CMD_ID_SET_PHY_PARAM,  /* 0x31 (Set) */
    CMD_ID_SET_NOA_PARAM,  /* 0x32 (Set) */
    CMD_ID_SET_OPPPS_PARAM,  /* 0x33 (Set) */
    CMD_ID_SET_UAPSD_PARAM,  /* 0x34 (Set) */
    CMD_ID_SET_SIGMA_STA_SLEEP,  /* 0x35 (Set) */
    CMD_ID_SET_EDGE_TXPWR_LIMIT,  /* 0x36 (Set) */
    CMD_ID_SET_DEVICE_MODE,  /* 0x37 (Set) */
    CMD_ID_SET_TXPWR_CTRL,  /* 0x38 (Set) */
    CMD_ID_SET_AUTOPWR_CTRL,  /* 0x39 (Set) */
    CMD_ID_SET_WFD_CTRL,  /* 0x3a (Set) */
    CMD_ID_SET_NLO_REQ,  /* 0x3b (Set) */
    CMD_ID_SET_NLO_CANCEL,  /* 0x3c (Set) */
    CMD_ID_SET_GTK_REKEY_DATA,  /* 0x3d (Set) */
    CMD_ID_ROAMING_CONTROL,  /* 0x3e (Set) */
    /* CFG_M0VE_BA_TO_DRIVER */
    CMD_ID_RESET_BA_SCOREBOARD     = 0x3f, /* 0x3f (Set) */
    CMD_ID_SET_EDGE_TXPWR_LIMIT_5G = 0x40,  /* 0x40 (Set) */
    CMD_ID_SET_CHANNEL_PWR_OFFSET,  /* 0x41 (Set) */
    CMD_ID_SET_80211AC_TX_PWR,  /* 0x42 (Set) */
    CMD_ID_SET_PATH_COMPASATION,  /* 0x43 (Set) */

    CMD_ID_SET_BATCH_REQ = 0x47,  /* 0x47 (Set) */
    CMD_ID_SET_NVRAM_SETTINGS,  /* 0x48 (Set) */
    CMD_ID_SET_COUNTRY_POWER_LIMIT,  /* 0x49 (Set) */

#if CFG_WOW_SUPPORT
    CMD_ID_SET_WOWLAN,  /* 0x4a (Set) */
#endif

    CMD_ID_CSI_CONTROL            = 0x4c, /* 0x4c (Set /Query) */

#if CFG_SUPPORT_WIFI_HOST_OFFLOAD
    CMD_ID_SET_AM_FILTER          = 0x55, /* 0x55 (Set) */
    CMD_ID_SET_AM_HEARTBEAT,  /* 0x56 (Set) */
    CMD_ID_SET_AM_TCP,  /* 0x57 (Set) */
#endif
    CMD_ID_SET_SUSPEND_MODE       = 0x58, /* 0x58 (Set) */

#if CFG_WOW_SUPPORT
    CMD_ID_SET_PF_CAPABILITY      = 0x59, /* 0x59 (Set) */
#endif

#if CFG_STR_DHCP_RENEW_OFFLOAD
    CMD_ID_SET_DHCP_RENEW_OFFLOAD = 0x5A,  /* 0x5A (Set) */
#endif

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
    CMD_ID_SET_ROAMING_SKIP       = 0x6D, /* 0x6D (Set) used to setting roaming
                                           * skip*/
#endif
    CMD_ID_GET_SET_CUSTOMER_CFG   = 0x70, /* 0x70(Set) */
    CMD_ID_COEX_CTRL              = 0x7C, /* 0x7C (Set/Query) */
    CMD_ID_GET_NIC_CAPABILITY     = 0x80, /* 0x80 (Query) */
    CMD_ID_GET_LINK_QUALITY,  /* 0x81 (Query) */
    CMD_ID_GET_STATISTICS,  /* 0x82 (Query) */
    CMD_ID_GET_CONNECTION_STATUS,  /* 0x83 (Query) */
    CMD_ID_GET_STA_STATISTICS     = 0x85, /* 0x85 (Query) */

    CMD_ID_GET_LTE_CHN            = 0x87, /* 0x87 (Query) */
    CMD_ID_GET_CHN_LOADING        = 0x88, /* 0x88 (Query) */
    CMD_ID_GET_BUG_REPORT         = 0x89, /* 0x89 (Query) */
    CMD_ID_GET_NIC_CAPABILITY_V2  = 0x8A, /* 0x8A (Query) */
    CMD_ID_GET_TEMPERATURE        = 0x8C, /* 0x8B (Query) */

#if (CFG_SUPPORT_DFS_MASTER == 1)
    CMD_ID_RDD_ON_OFF_CTRL        = 0x8F, /* 0x8F(Set) */
#endif

#ifdef CFG_SUPPORT_ANT_DIV
    CMD_ID_ANT_DIV_CTRL           = 0x91,
#endif

    CMD_ID_ACCESS_REG             = 0xc0, /* 0xc0 (Set / Query) */
    CMD_ID_MAC_MCAST_ADDR,  /* 0xc1 (Set / Query) */
    CMD_ID_802_11_PMKID,  /* 0xc2 (Set / Query) */
    CMD_ID_ACCESS_EEPROM,  /* 0xc3 (Set / Query) */
    CMD_ID_SW_DBG_CTRL,  /* 0xc4 (Set / Query) */
    CMD_ID_FW_LOG_2_HOST,  /* 0xc5 (Set) */
    CMD_ID_DUMP_MEM,  /* 0xc6 (Query) */
    CMD_ID_RESOURCE_CONFIG,  /* 0xc7 (Set / Query) */
#if CFG_SUPPORT_QA_TOOL
    CMD_ID_ACCESS_RX_STAT,  /* 0xc8 (Query) */
#endif
    CMD_ID_CHIP_CONFIG              = 0xCA, /* 0xca (Set / Query) */
    CMD_ID_STATS_LOG                = 0xCB, /* 0xcb (Set) */

    CMD_ID_WLAN_INFO                = 0xCD, /* 0xcd (Query) */
    CMD_ID_MIB_INFO                 = 0xCE, /* 0xce (Query) */

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
    CMD_ID_TX_MCS_INFO              = 0xCF, /* 0xcf (Query) */
#endif
#ifdef CFG_DUMP_TXPOWR_TABLE
    CMD_ID_GET_TXPWR_TBL            = 0xD0, /* 0xd0 (Query) */
#endif

    CMD_ID_SET_RDD_CH               = 0xE1,

#if CFG_SUPPORT_QA_TOOL
    CMD_ID_LAYER_0_EXT_MAGIC_NUM    = 0xED, /* magic number for Extending
                                             *  MT6630 original CMD header */
#endif

    CMD_ID_SET_BWCS                 = 0xF1,
    CMD_ID_SET_OSC                  = 0xF2,

    CMD_ID_HIF_CTRL                 = 0xF6, /* 0xF6 (Set) */

    CMD_ID_GET_BUILD_DATE_CODE      = 0xF8, /* 0xf8 (Query) */
    CMD_ID_GET_BSS_INFO             = 0xF9, /* 0xF9 (Query) */
    CMD_ID_SET_HOTSPOT_OPTIMIZATION = 0xFA,  /* 0xFA (Set) */
    CMD_ID_SET_TDLS_CH_SW           = 0xFB,
    CMD_ID_SET_MONITOR              = 0xFC, /* 0xFC (Set) */
#if CFG_SUPPORT_ADVANCE_CONTROL
    CMD_ID_ADV_CONTROL              = 0xFE, /* 0xFE (Set / Query) */
#endif
    CMD_ID_END
} ENUM_CMD_ID_T,
*P_ENUM_CMD_ID_T;

typedef enum _ENUM_EVENT_ID_T {
    EVENT_ID_NIC_CAPABILITY = 0x01,  /* 0x01 (Query) */
    EVENT_ID_LINK_QUALITY,  /* 0x02 (Query / Unsolicited) */
    EVENT_ID_STATISTICS,  /* 0x03 (Query) */
    EVENT_ID_MIC_ERR_INFO,  /* 0x04 (Unsolicited) */
    EVENT_ID_ACCESS_REG,  /* 0x05 (Query - CMD_ID_ACCESS_REG) */
    EVENT_ID_ACCESS_EEPROM,  /* 0x06 (Query - CMD_ID_ACCESS_EEPROM) */
    EVENT_ID_SLEEPY_INFO,  /* 0x07 (Unsolicited) */
    EVENT_ID_BT_OVER_WIFI,  /* 0x08 (Unsolicited) */
    EVENT_ID_TEST_STATUS,  /* 0x09 (Query - CMD_ID_TEST_CTRL) */
    EVENT_ID_RX_ADDBA,  /* 0x0a (Unsolicited) */
    EVENT_ID_RX_DELBA,  /* 0x0b (Unsolicited) */
    EVENT_ID_ACTIVATE_STA_REC,  /* 0x0c (Response) */
    EVENT_ID_SCAN_DONE,  /* 0x0d (Unsoiicited) */
    EVENT_ID_RX_FLUSH,  /* 0x0e (Unsolicited) */
    EVENT_ID_TX_DONE,  /* 0x0f (Unsolicited) */
    EVENT_ID_CH_PRIVILEGE,  /* 0x10 (Unsolicited) */
    EVENT_ID_BSS_ABSENCE_PRESENCE,  /* 0x11 (Unsolicited) */
    EVENT_ID_STA_CHANGE_PS_MODE,  /* 0x12 (Unsolicited) */
    EVENT_ID_BSS_BEACON_TIMEOUT,  /* 0x13 (Unsolicited) */
    EVENT_ID_UPDATE_NOA_PARAMS,  /* 0x14 (Unsolicited) */
    EVENT_ID_AP_OBSS_STATUS,  /* 0x15 (Unsolicited) */
    EVENT_ID_STA_UPDATE_FREE_QUOTA,  /* 0x16 (Unsolicited) */
    EVENT_ID_SW_DBG_CTRL,  /* 0x17 (Query - CMD_ID_SW_DBG_CTRL) */
    EVENT_ID_ROAMING_STATUS,  /* 0x18 (Unsolicited) */
    EVENT_ID_STA_AGING_TIMEOUT,  /* 0x19 (Unsolicited) */
    EVENT_ID_SEC_CHECK_RSP,  /* 0x1a (Query - CMD_ID_SEC_CHECK) */
    EVENT_ID_SEND_DEAUTH,  /* 0x1b (Unsolicited) */
    EVENT_ID_UPDATE_RDD_STATUS,  /* 0x1c (Unsolicited) */
    EVENT_ID_UPDATE_BWCS_STATUS,  /* 0x1d (Unsolicited) */
    EVENT_ID_UPDATE_BCM_DEBUG,  /* 0x1e (Unsolicited) */
    EVENT_ID_RX_ERR,  /* 0x1f (Unsolicited) */
    EVENT_ID_DUMP_MEM = 0x20,  /* 0x20 (Query - CMD_ID_DUMP_MEM) */
    EVENT_ID_STA_STATISTICS,  /* 0x21 (Query ) */
    EVENT_ID_STA_STATISTICS_UPDATE,  /* 0x22 (Unsolicited) */
    EVENT_ID_NLO_DONE,  /* 0x23 (Unsoiicited) */
    EVENT_ID_ADD_PKEY_DONE,  /* 0x24 (Unsoiicited) */
    EVENT_ID_ICAP_DONE,  /* 0x25 (Unsoiicited) */
    EVENT_ID_RESOURCE_CONFIG      = 0x26, /* 0x26 (Query -
                                           * CMD_ID_RESOURCE_CONFIG) */
    EVENT_ID_DEBUG_MSG            = 0x27, /* 0x27 (Unsoiicited) */
    EVENT_ID_RTT_CALIBR_DONE      = 0x28, /* 0x28 (Unsoiicited) */
    EVENT_ID_RTT_UPDATE_RANGE     = 0x29, /* 0x29 (Unsoiicited) */
    EVENT_ID_CHECK_REORDER_BUBBLE = 0x2a,  /* 0x2a (Unsoiicited) */
    EVENT_ID_BATCH_RESULT         = 0x2b, /* 0x2b (Query) */

    EVENT_ID_CSI_DATA             = 0x3c, /* 0x3c (Query) */
    EVENT_ID_GET_GTK_REKEY_DATA   = 0x3d, /* 0x3d (Query) */

    EVENT_ID_UART_ACK             = 0x40, /* 0x40 (Unsolicited) */
    EVENT_ID_UART_NAK,  /* 0x41 (Unsolicited) */
    EVENT_ID_GET_CHIPID,  /* 0x42 (Query - CMD_ID_GET_CHIPID) */
    EVENT_ID_SLT_STATUS,  /* 0x43 (Query - CMD_ID_SET_SLTINFO) */
    EVENT_ID_CHIP_CONFIG,  /* 0x44 (Query - CMD_ID_CHIP_CONFIG) */
#if CFG_SUPPORT_QA_TOOL
    EVENT_ID_ACCESS_RX_STAT,  /* 0x45 (Query - CMD_ID_ACCESS_RX_STAT) */
#endif

    EVENT_ID_RDD_SEND_PULSE        = 0x50,

#if CFG_SUPPORT_TX_BF
    EVENT_ID_PFMU_TAG_READ         = 0x51,
    EVENT_ID_PFMU_DATA_READ        = 0x52,
#endif

#if CFG_SUPPORT_MU_MIMO
    EVENT_ID_MU_GET_QD             = 0x53,
    EVENT_ID_MU_GET_LQ             = 0x54,
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
    EVENT_ID_RDD_REPORT            = 0x60,
    EVENT_ID_CSA_DONE              = 0x61,
#endif

#if (CFG_WOW_SUPPORT == 1)
    EVENT_ID_WOW_WAKEUP_REASON     = 0x62,
#endif

    EVENT_ID_TDLS                  = 0x80, /* TDLS event_id */
    EVENT_ID_GET_TEMPERATURE       = 0x8C,

    EVENT_ID_UPDATE_COEX_PHYRATE   = 0x90, /* 0x90 (Unsolicited) */

    EVENT_ID_WLAN_INFO             = 0xCD,
    EVENT_ID_MIB_INFO              = 0xCE,

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
    EVENT_ID_TX_MCS_INFO           = 0xCF,
#endif
#ifdef CFG_DUMP_TXPOWR_TABLE
    EVENT_ID_GET_TXPWR_TBL         = 0xD0,
#endif

    EVENT_ID_NIC_CAPABILITY_V2     = 0xEC, /* 0xEC (Query -
                                            * CMD_ID_GET_NIC_CAPABILITY_V2) */
    /*#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)*/
    EVENT_ID_LAYER_0_EXT_MAGIC_NUM = 0xED,  /* magic number for Extending
                                             * MT6630 original EVENT header
                                             */
/*#endif*/

#if CFG_ASSERT_DUMP
    EVENT_ID_ASSERT_DUMP           = 0xF0,
#endif
    EVENT_ID_HIF_CTRL              = 0xF6,
    EVENT_ID_BUILD_DATE_CODE       = 0xF8,
    EVENT_ID_GET_AIS_BSS_INFO      = 0xF9,
    EVENT_ID_DEBUG_CODE            = 0xFB,
    EVENT_ID_RFTEST_READY          = 0xFC, /* 0xFC */
#if CFG_SUPPORT_ADVANCE_CONTROL
    EVENT_ID_ADV_CONTROL           = 0xFE,
#endif
    EVENT_ID_END
} ENUM_EVENT_ID_T,
*P_ENUM_EVENT_ID_T;

#if CFG_WOW_SUPPORT

/* Filter Flag */
#define WOWLAN_FF_DROP_ALL           BIT(0)
#define WOWLAN_FF_SEND_MAGIC_TO_HOST         BIT(1)
#define WOWLAN_FF_ALLOW_ARP          BIT(2)
#define WOWLAN_FF_ALLOW_BMC          BIT(3)
#define WOWLAN_FF_ALLOW_UC           BIT(4)
#define WOWLAN_FF_ALLOW_1X           BIT(5)
#define WOWLAN_FF_ALLOW_ARP_REQ2ME       BIT(6)

/* wow detect type */
#define WOWLAN_DETECT_TYPE_MAGIC         BIT(0)
#define WOWLAN_DETECT_TYPE_ALLOW_NORMAL      BIT(1)
#define WOWLAN_DETECT_TYPE_ONLY_PHONE_SUSPEND    BIT(2)
#define WOWLAN_DETECT_TYPE_DISASSOCIATION    BIT(3)
#define WOWLAN_DETECT_TYPE_BCN_LOST      BIT(4)

/* Wakeup command bit define */
#define PF_WAKEUP_CMD_BIT0_OUTPUT_MODE_EN    BIT(0)
#define PF_WAKEUP_CMD_BIT1_OUTPUT_DATA       BIT(1)
#define PF_WAKEUP_CMD_BIT2_WAKEUP_LEVEL      BIT(2)

#define PM_WOWLAN_REQ_START          0x1
#define PM_WOWLAN_REQ_STOP           0x2

typedef struct _CMD_WAKE_HIF_T {
    u8 ucWakeupHif;  /* use in-band signal to wakeup system, ENUM_HIF_TYPE */
    u8 ucGpioPin;  /* GPIO Pin */
    u8 ucTriggerLvl;  /* GPIO Pin */
    u8 aucResv1[1];
    u32 u4GpioInterval;  /* 0: low to high, 1: high to low */
    u8 aucResv2[4];
} CMD_WAKE_HIF_T, *P_CMD_WAKE_HIF_T;

typedef struct _CMD_WOWLAN_PARAM_T {
    u8 ucCmd;
    u8 ucDetectType;
    u16 u2FilterFlag;  /* ARP/MC/DropExceptMagic/SendMagicToHost */
    u8 ucScenarioID;  /* WOW/WOBLE/Proximity */
    u8 ucBlockCount;
    u8 ucDbdcBand;
    u8 aucReserved1[1];
    CMD_WAKE_HIF_T astWakeHif[2];
    WOW_PORT_T stWowPort;
    u8 aucReserved2[32];
} CMD_WOWLAN_PARAM_T, *P_CMD_WOWLAN_PARAM_T;

typedef struct _EVENT_WOWLAN_NOTIFY_T {
    u8 ucNetTypeIndex;
    u8 aucReserved[3];
} EVENT_WOWLAN_NOTIFY_T, *P_EVENT_WOWLAN_NOTIFY_T;

/* PACKETFILTER CAPABILITY TYPE */

#define PACKETF_CAP_TYPE_ARP          BIT(1)
#define PACKETF_CAP_TYPE_MAGIC        BIT(2)
#define PACKETF_CAP_TYPE_BITMAP       BIT(3)
#define PACKETF_CAP_TYPE_EAPOL        BIT(4)
#define PACKETF_CAP_TYPE_TDLS         BIT(5)
#define PACKETF_CAP_TYPE_CF       BIT(6)
#define PACKETF_CAP_TYPE_HEARTBEAT    BIT(7)
#define PACKETF_CAP_TYPE_TCP_SYN      BIT(8)
#define PACKETF_CAP_TYPE_UDP_SYN      BIT(9)
#define PACKETF_CAP_TYPE_BCAST_SYN    BIT(10)
#define PACKETF_CAP_TYPE_MCAST_SYN    BIT(11)
#define PACKETF_CAP_TYPE_V6       BIT(12)
#define PACKETF_CAP_TYPE_TDIM         BIT(13)

typedef enum _ENUM_FUNCTION_SELECT {
    FUNCTION_PF        = 1,
    FUNCTION_BITMAP    = 2,
    FUNCTION_EAPOL     = 3,
    FUNCTION_TDLS      = 4,
    FUNCTION_ARPNS     = 5,
    FUNCTION_CF        = 6,
    FUNCTION_MODE      = 7,
    FUNCTION_BSSID     = 8,
    FUNCTION_MGMT      = 9,
    FUNCTION_BMC_DROP  = 10,
    FUNCTION_UC_DROP   = 11,
    FUNCTION_ALL_TOMCU = 12,
} _ENUM_FUNCTION_SELECT,
*P_ENUM_FUNCTION_SELECT;

typedef enum _ENUM_PF_OPCODE_T {
    PF_OPCODE_ADD = 0,
    PF_OPCODE_DEL,
    PF_OPCODE_ENABLE,
    PF_OPCODE_DISABLE,
    PF_OPCODE_NUM
} ENUM_PF_OPCODE_T;

typedef struct _CMD_PACKET_FILTER_CAP_T {
    u8 ucCmd;
    u16 packet_cap_type;
    u8 aucReserved1[1];
    /* GLOBAL */
    u32 PFType;
    u32 FunctionSelect;
    u32 Enable;
    /* MAGIC */
    u8 ucBssid;
    u16 usEnableBits;
    u8 aucReserved5[1];
    /* DTIM */
    u8 DtimEnable;
    u8 DtimValue;
    u8 aucReserved2[2];
    /* BITMAP_PATTERN_T */
    u32 Index;
    u32 Offset;
    u32 FeatureBits;
    u32 Resv;
    u32 PatternLength;
    u32 Mask[4];
    u32 Pattern[32];
    /* COALESCE */
    u32 FilterID;
    u32 PacketType;
    u32 CoalesceOP;
    u8 FieldLength;
    u8 CompareOP;
    u8 FieldID;
    u8 aucReserved3[1];
    u32 Pattern3[4];
    /* TCPSYN */
    u32 AddressType;
    u32 TCPSrcPort;
    u32 TCPDstPort;
    u32 SourceIP[4];
    u32 DstIP[4];
    u8 aucReserved4[64];
} CMD_PACKET_FILTER_CAP_T, *P_CMD_PACKET_FILTER_CAP_T;
#endif

#if CFG_SUPPORT_WIFI_HOST_OFFLOAD
typedef struct _CMD_TCP_GENERATOR {
    ENUM_PF_OPCODE_T eOpcode;
    u32 u4ReplyId;
    u32 u4Period;
    u32 u4Timeout;
    u32 u4IpId;
    u32 u4DestPort;
    u32 u4SrcPort;
    u32 u4Seq;
    u8 aucDestIp[4];
    u8 aucSrcIp[4];
    u8 aucDestMac[6];
    u8 ucBssId;
    u8 aucReserved1[1];
    u8 aucReserved2[64];
} CMD_TCP_GENERATOR, *P_CMD_TCP_GENERATOR;

typedef struct _CMD_PATTERN_GENERATOR {
    ENUM_PF_OPCODE_T eOpcode;
    u32 u4ReplyId;
    u32 u4EthernetLength;
    u32 u4Period;
    u8 aucEthernetFrame[128];
    u8 ucBssId;
    u8 aucReserved1[3];
    u8 aucReserved2[64];
} CMD_PATTERN_GENERATOR, *P_CMD_PATTERN_GENERATOR;

typedef struct _CMD_BITMAP_FILTER {
    ENUM_PF_OPCODE_T eOpcode;
    u32 u4ReplyId;
    u32 u4Offset;
    u32 u4Length;
    u8 aucPattern[64];
    u8 aucBitMask[64];
    u8 fgIsEqual;
    u8 fgIsAccept;
    u8 ucBssId;
    u8 aucReserved1[1];
    u8 aucReserved2[64];
} CMD_BITMAP_FILTER, *P_CMD_BITMAP_FILTER;

#endif

#if CFG_SUPPORT_PER_BSS_FILTER
typedef struct _CMD_RX_PACKET_FILTER {
    u32 u4RxPacketFilter;
    u8 ucIsPerBssFilter;
    u8 ucBssIndex;
    u8 aucReserved[2];
    u32 u4BssMgmtFilter;
    u8 aucReserved2[56];
} CMD_RX_PACKET_FILTER, *P_CMD_RX_PACKET_FILTER;
#else
typedef struct _CMD_RX_PACKET_FILTER {
    u32 u4RxPacketFilter;
    u8 aucReserved[64];
} CMD_RX_PACKET_FILTER, *P_CMD_RX_PACKET_FILTER;
#endif

#if defined(MT6632)
#define S2D_INDEX_CMD_H2N        0x0
#define S2D_INDEX_CMD_C2N        0x1
#define S2D_INDEX_CMD_H2C        0x2
#define S2D_INDEX_CMD_H2N_H2C        0x3

#define S2D_INDEX_EVENT_N2H      0x0
#define S2D_INDEX_EVENT_N2C      0x1
#define S2D_INDEX_EVENT_C2H      0x2
#define S2D_INDEX_EVENT_N2H_N2C      0x3
#endif

#define EXT_EVENT_ID_CMD_RESULT      0x00

/*#if (CFG_EEPROM_PAGE_ACCESS == 1)*/
#define EXT_EVENT_ID_CMD_EFUSE_ACCESS    0x1
#define EXT_EVENT_ID_EFUSE_FREE_BLOCK    0x4D
#define EXT_EVENT_ID_GET_TX_POWER    0x1C
#define EXT_EVENT_TARGET_TX_POWER    0x1

/*#endif*/
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* for Event Packet (via HIF-RX) */
typedef struct _PSE_CMD_HDR_T {
    /* DW0 */
    u16 u2TxByteCount;
    u16 u2Reserved1 : 10;
    u16 u2Qidx : 5;
    u16 u2Pidx : 1;

    /* DW1 */
    u16 u2Reserved2 : 13;
    u16 u2Hf : 2;
    u16 u2Ft : 1;
    u16 u2Reserved3 : 8;
    u16 u2PktFt : 2;
    u16 u2Reserved4 : 6;

    /* DW2~7 */
    u32 au4Reserved[6];
} PSE_CMD_HDR_T, *P_PSE_CMD_HDR_T;

typedef struct _WIFI_CMD_T {
    u16 u2TxByteCount;  /* Max value is over 2048 */
    u16 u2PQ_ID;  /* Must be 0x8000 (Port1, Queue 0) */

    u8 ucWlanIdx;
    u8 ucHeaderFormat;
    u8 ucHeaderPadding;
    u8 ucPktFt : 2;
    u8 ucOwnMAC : 6;
    u32 au4Reserved1[6];

    u16 u2Length;
    u16 u2PqId;

    u8 ucCID;
    u8 ucPktTypeID;  /* Must be 0x20 (CMD Packet) */
    u8 ucSetQuery;
    u8 ucSeqNum;

    u8 ucD2B0Rev;  /* padding fields, hw may auto modify this field */
    u8 ucExtenCID;  /* Extend CID */
    u8 ucS2DIndex;  /* Index for Src to Dst in CMD usage */
    u8 ucExtCmdOption;  /* Extend CID option */

    u8 ucCmdVersion;
    u8 ucReserved2[3];
    u32 au4Reserved3[4];  /* padding fields */

    u8 aucBuffer[0];
} WIFI_CMD_T, *P_WIFI_CMD_T;

/* for Command Packet (via HIF-TX) */
/* following CM's documentation v0.7 */
typedef struct _WIFI_EVENT_T {
    u32 au4HwMacRxDesc[4];

    u16 u2PacketLength;
    u16 u2PacketType;  /* Must be filled with 0xE000 (EVENT Packet) */
    u8 ucEID;
    u8 ucSeqNum;
    u8 ucEventVersion;
    u8 aucReserved[1];

    u8 ucExtenEID;
    u8 aucReserved2[2];
    u8 ucS2DIndex;

    u8 aucBuffer[];
} WIFI_EVENT_T, *P_WIFI_EVENT_T;

/* CMD_ID_TEST_CTRL */
typedef struct _CMD_TEST_CTRL_T {
    u8 ucAction;
    u8 aucReserved[3];
    union {
        u32 u4OpMode;
        u32 u4ChannelFreq;
        PARAM_MTK_WIFI_TEST_STRUCT_T rRfATInfo;
    } u;
} CMD_TEST_CTRL_T, *P_CMD_TEST_CTRL_T;

/* EVENT_TEST_STATUS */
typedef struct _PARAM_CUSTOM_RFTEST_TX_STATUS_STRUCT_T {
    u32 u4PktSentStatus;
    u32 u4PktSentCount;
    u16 u2AvgAlc;
    u8 ucCckGainControl;
    u8 ucOfdmGainControl;
} PARAM_CUSTOM_RFTEST_TX_STATUS_STRUCT_T,
*P_PARAM_CUSTOM_RFTEST_TX_STATUS_STRUCT_T;

typedef struct _PARAM_CUSTOM_RFTEST_RX_STATUS_STRUCT_T {
    u32 u4IntRxOk;  /*!< number of packets that Rx ok from interrupt */
    u32 u4IntCrcErr;  /*!< number of packets that CRC error from interrupt */
    u32 u4IntShort;  /*!< number of packets that is short preamble from
                      * interrupt */
    u32 u4IntLong;  /*!< number of packets that is long preamble from
                     * interrupt */
    u32 u4PauRxPktCount;  /*!< number of packets that Rx ok from PAU */
    u32 u4PauCrcErrCount;  /*!< number of packets that CRC error from PAU */
    u32 u4PauRxFifoFullCount;  /*!< number of packets that is short preamble
                                * from PAU */
    u32 u4PauCCACount;  /*!< CCA rising edge count */
} PARAM_CUSTOM_RFTEST_RX_STATUS_STRUCT_T,
*P_PARAM_CUSTOM_RFTEST_RX_STATUS_STRUCT_T;

typedef union _EVENT_TEST_STATUS {
    PARAM_MTK_WIFI_TEST_STRUCT_T rATInfo;
    /* PARAM_CUSTOM_RFTEST_TX_STATUS_STRUCT_T   rTxStatus; */
    /* PARAM_CUSTOM_RFTEST_RX_STATUS_STRUCT_T   rRxStatus; */
} EVENT_TEST_STATUS, *P_EVENT_TEST_STATUS;

/* CMD_BUILD_CONNECTION */
typedef struct _CMD_BUILD_CONNECTION {
    u8 ucInfraMode;
    u8 ucAuthMode;
    u8 ucEncryptStatus;
    u8 ucSsidLen;
    u8 aucSsid[PARAM_MAX_LEN_SSID];
    u8 aucBssid[PARAM_MAC_ADDR_LEN];

    /* Ad-hoc mode */
    u16 u2BeaconPeriod;
    u16 u2ATIMWindow;
    u8 ucJoinOnly;
    u8 ucReserved;
    u32 u4FreqInKHz;

    /* for faster connection */
    u8 aucScanResult[0];
} CMD_BUILD_CONNECTION, *P_CMD_BUILD_CONNECTION;

/* CMD_ADD_REMOVE_KEY */
typedef struct _CMD_802_11_KEY {
    u8 ucAddRemove;
    u8 ucTxKey;
    u8 ucKeyType;
    u8 ucIsAuthenticator;
    u8 aucPeerAddr[6];
    u8 ucBssIdx;
    u8 ucAlgorithmId;
    u8 ucKeyId;
    u8 ucKeyLen;
    u8 ucWlanIndex;
    u8 ucMgmtProtection;
    u8 aucKeyMaterial[32];
    u8 aucKeyRsc[16];
} CMD_802_11_KEY, *P_CMD_802_11_KEY;

/* CMD_ID_DEFAULT_KEY_ID */
typedef struct _CMD_DEFAULT_KEY {
    u8 ucBssIdx;
    u8 ucKeyId;
    u8 ucWlanIndex;
    u8 ucMulticast;
} CMD_DEFAULT_KEY, *P_CMD_DEFAULT_KEY;

/* WPA2 PMKID cache structure */
typedef struct _PMKID_ENTRY_T {
    PARAM_BSSID_INFO_T rBssidInfo;
    u8 fgPmkidExist;
} PMKID_ENTRY_T, *P_PMKID_ENTRY_T;

typedef struct _CMD_802_11_PMKID {
    u32 u4BSSIDInfoCount;
    P_PMKID_ENTRY_T arPMKIDInfo[1];
} CMD_802_11_PMKID, *P_CMD_802_11_PMKID;

typedef struct _CMD_GTK_REKEY_DATA_T {
    u8 aucKek[16];
    u8 aucKck[16];
    u8 aucReplayCtr[8];
} CMD_GTK_REKEY_DATA_T, *P_CMD_GTK_REKEY_DATA_T;

typedef struct _CMD_CSUM_OFFLOAD_T {
    u16 u2RxChecksum;  /* bit0: IP, bit1: UDP, bit2: TCP */
    u16 u2TxChecksum;  /* bit0: IP, bit1: UDP, bit2: TCP */
} CMD_CSUM_OFFLOAD_T, *P_CMD_CSUM_OFFLOAD_T;

/* CMD_BASIC_CONFIG */
typedef struct _CMD_BASIC_CONFIG_T {
    u8 ucNative80211;
    u8 ucCtrlFlagAssertPath;
    u8 ucCtrlFlagDebugLevel;
    u8 aucReserved[1];
    CMD_CSUM_OFFLOAD_T rCsumOffload;
    u8 ucCrlFlagSegememt;
    u8 aucReserved2[3];
} CMD_BASIC_CONFIG_T, *P_CMD_BASIC_CONFIG_T;

/* CMD_MAC_MCAST_ADDR */
typedef struct _CMD_MAC_MCAST_ADDR {
    u32 u4NumOfGroupAddr;
    u8 ucBssIndex;
    u8 aucReserved[3];
    PARAM_MAC_ADDRESS arAddress[MAX_NUM_GROUP_ADDR];
} CMD_MAC_MCAST_ADDR, *P_CMD_MAC_MCAST_ADDR, EVENT_MAC_MCAST_ADDR,
*P_EVENT_MAC_MCAST_ADDR;

/* CMD_ACCESS_EEPROM */
typedef struct _CMD_ACCESS_EEPROM {
    u16 u2Offset;
    u16 u2Data;
} CMD_ACCESS_EEPROM, *P_CMD_ACCESS_EEPROM, EVENT_ACCESS_EEPROM,
*P_EVENT_ACCESS_EEPROM;

typedef struct _CMD_CUSTOM_NOA_PARAM_STRUCT_T {
    u32 u4NoaDurationMs;
    u32 u4NoaIntervalMs;
    u32 u4NoaCount;
    u8 ucBssIdx;
    u8 aucReserved[3];
} CMD_CUSTOM_NOA_PARAM_STRUCT_T, *P_CMD_CUSTOM_NOA_PARAM_STRUCT_T;

typedef struct _CMD_CUSTOM_OPPPS_PARAM_STRUCT_T {
    u32 u4CTwindowMs;
    u8 ucBssIdx;
    u8 aucReserved[3];
} CMD_CUSTOM_OPPPS_PARAM_STRUCT_T, *P_CMD_CUSTOM_OPPPS_PARAM_STRUCT_T;

typedef struct _CMD_CUSTOM_UAPSD_PARAM_STRUCT_T {
    u8 fgEnAPSD;
    u8 fgEnAPSD_AcBe;
    u8 fgEnAPSD_AcBk;
    u8 fgEnAPSD_AcVo;
    u8 fgEnAPSD_AcVi;
    u8 ucMaxSpLen;
    u8 aucResv[2];
} CMD_CUSTOM_UAPSD_PARAM_STRUCT_T, *P_CMD_CUSTOM_UAPSD_PARAM_STRUCT_T;

/* EVENT_CONNECTION_STATUS */
typedef struct _EVENT_CONNECTION_STATUS {
    u8 ucMediaStatus;
    u8 ucReasonOfDisconnect;

    u8 ucInfraMode;
    u8 ucSsidLen;
    u8 aucSsid[PARAM_MAX_LEN_SSID];
    u8 aucBssid[PARAM_MAC_ADDR_LEN];
    u8 ucAuthenMode;
    u8 ucEncryptStatus;
    u16 u2BeaconPeriod;
    u16 u2AID;
    u16 u2ATIMWindow;
    u8 ucNetworkType;
    u8 aucReserved[1];
    u32 u4FreqInKHz;

#if CFG_ENABLE_WIFI_DIRECT
    u8 aucInterfaceAddr[PARAM_MAC_ADDR_LEN];
#endif
} EVENT_CONNECTION_STATUS, *P_EVENT_CONNECTION_STATUS;

/* EVENT_NIC_CAPABILITY */
typedef struct _EVENT_NIC_CAPABILITY_T {
    u16 u2ProductID;
    u16 u2FwVersion;
    u16 u2DriverVersion;
    u8 ucHw5GBandDisabled;
    u8 ucEepromUsed;
    u8 aucMacAddr[6];
    u8 ucEndianOfMacAddrNumber;
    u8 ucHwNotSupportAC;

    u8 ucRfVersion;
    u8 ucPhyVersion;
    u8 ucRfCalFail;
    u8 ucBbCalFail;
    u8 aucDateCode[16];
    u32 u4FeatureFlag0;
    u32 u4FeatureFlag1;
    u32 u4CompileFlag0;
    u32 u4CompileFlag1;
    u8 aucBranchInfo[4];
    u8 ucFwBuildNumber;
    u8 ucHwSetNss1x1;
    u8 ucHwNotSupportDBDC;
    u8 ucHwWiFiZeroOnly;
    u8 aucReserved1[56];
} EVENT_NIC_CAPABILITY_T, *P_EVENT_NIC_CAPABILITY_T;

typedef struct _EVENT_NIC_CAPABILITY_V2_T {
    u16 u2TotalElementNum;
    u8 aucReserved[2];
    u8 aucBuffer[];
} EVENT_NIC_CAPABILITY_V2_T, *P_EVENT_NIC_CAPABILITY_V2_T;

typedef struct _NIC_CAPABILITY_V2_ELEMENT {
    u32 tag_type;  /* NIC_CAPABILITY_V2_TAG_T */
    u32 body_len;
    u8 aucbody[];
} NIC_CAPABILITY_V2_ELEMENT, *P_NIC_CAPABILITY_V2_ELEMENT;

typedef WLAN_STATUS (*NIC_CAP_V2_ELEMENT_HDLR)(P_ADAPTER_T prAdapter,
                                               u8 *buff,
                                               u32 buff_len);
typedef struct _NIC_CAPABILITY_V2_REF_TABLE_T {
    u32 tag_type;  /* NIC_CAPABILITY_V2_TAG_T */
    NIC_CAP_V2_ELEMENT_HDLR hdlr;
} NIC_CAPABILITY_V2_REF_TABLE_T, *P_NIC_CAPABILITY_V2_REF_TABLE_T;

typedef enum _NIC_CAPABILITY_V2_TAG_T {
    TAG_CAP_TX_RESOURCE     = 0x0,
    TAG_CAP_TX_EFUSEADDRESS = 0x1,
    TAG_CAP_COEX_FEATURE    = 0x2,
    TAG_CAP_SINGLE_SKU      = 0x3,
#if CFG_TCP_IP_CHKSUM_OFFLOAD
    TAG_CAP_CSUM_OFFLOAD    = 0x4,
#endif
    TAG_CAP_EFUSE_OFFSET    = 0x5,
    TAG_CAP_TOTAL
} NIC_CAPABILITY_V2_TAG_T;

#if CFG_TCP_IP_CHKSUM_OFFLOAD
typedef struct _NIC_CSUM_OFFLOAD_T {
    u8 ucIsSupportCsumOffload;  /* 1: Support, 0: Not Support */
    u8 acReseved[3];
} NIC_CSUM_OFFLOAD_T, *P_NIC_CSUM_OFFLOAD_T;
#endif

typedef struct _NIC_COEX_FEATURE_T {
    u32 u4FddMode;  /* true for COEX FDD mode */
} NIC_COEX_FEATURE_T, *P_NIC_COEX_FEATURE_T;

typedef struct _NIC_EFUSE_ADDRESS_T {
    u32 u4EfuseStartAddress;  /* Efuse Start Address */
    u32 u4EfuseEndAddress;  /* Efuse End Address */
} NIC_EFUSE_ADDRESS_T, *P_NIC_EFUSE_ADDRESS_T;

struct _NIC_EFUSE_OFFSET_T {
    u32 u4TotalItem;  /* Efuse offset items */
    u32 u4WlanMacAddr;  /* Efuse Offset 1 */
};

typedef struct _NIC_TX_RESOURCE_T {
    u32 u4McuTotalResource;  /* the total usable resource for MCU port */
    u32 u4McuResourceUnit;  /* the unit of a MCU resource */
    u32 u4LmacTotalResource;  /* the total usable resource for LMAC port */
    u32 u4LmacResourceUnit;  /* the unit of a LMAC resource */
} NIC_TX_RESOURCE_T, *P_NIC_TX_RESOURCE_T;

/* modified version of WLAN_BEACON_FRAME_BODY_T for simplier buffering */
typedef struct _WLAN_BEACON_FRAME_BODY_T_LOCAL {
    /* Beacon frame body */
    u32 au4Timestamp[2];  /* Timestamp */
    u16 u2BeaconInterval;  /* Beacon Interval */
    u16 u2CapInfo;  /* Capability */
    u8 aucInfoElem[MAX_IE_LENGTH];  /* Various IEs, start from SSID */
    u16 u2IELength;  /* This field is *NOT* carried by F/W but caculated by
                      * nic_rx */
} WLAN_BEACON_FRAME_BODY_T_LOCAL, *P_WLAN_BEACON_FRAME_BODY_T_LOCAL;

/* EVENT_SCAN_RESULT */
typedef struct _EVENT_SCAN_RESULT_T {
    s32 i4RSSI;
    u32 u4LinkQuality;
    u32 u4DSConfig;  /* Center frequency */
    u32 u4DomainInfo;  /* Require CM opinion */
    u32 u4Reserved;
    u8 ucNetworkType;
    u8 ucOpMode;
    u8 aucBssid[MAC_ADDR_LEN];
    u8 aucRatesEx[PARAM_MAX_LEN_RATES_EX];
    WLAN_BEACON_FRAME_BODY_T_LOCAL rBeaconFrameBody;
} EVENT_SCAN_RESULT_T, *P_EVENT_SCAN_RESULT_T;

/* event of tkip mic error */
typedef struct _EVENT_MIC_ERR_INFO {
    u32 u4Flags;
} EVENT_MIC_ERR_INFO, *P_EVENT_MIC_ERR_INFO;

/* event of add key done for port control */
typedef struct _EVENT_ADD_KEY_DONE_INFO {
    u8 ucBSSIndex;
    u8 ucReserved;
    u8 aucStaAddr[6];
} EVENT_ADD_KEY_DONE_INFO, *P_EVENT_ADD_KEY_DONE_INFO;

typedef struct _EVENT_PMKID_CANDIDATE_LIST_T {
    u32 u4Version;  /*!< Version */
    u32 u4NumCandidates;  /*!< How many candidates follow */
    PARAM_PMKID_CANDIDATE_T arCandidateList[1];
} EVENT_PMKID_CANDIDATE_LIST_T, *P_EVENT_PMKID_CANDIDATE_LIST_T;

typedef struct _EVENT_CMD_RESULT {
    u8 ucCmdID;
    u8 ucStatus;
    u8 aucReserved[2];
} EVENT_CMD_RESULT, *P_EVENT_CMD_RESULT;

/* CMD_ID_ACCESS_REG & EVENT_ID_ACCESS_REG */
typedef struct _CMD_ACCESS_REG {
    u32 u4Address;
    u32 u4Data;
} CMD_ACCESS_REG, *P_CMD_ACCESS_REG;

/* CMD_COEX_CTRL & EVENT_COEX_CTRL */
/************************************************/
/*  u32 u4SubCmd : Coex Ctrl Sub Command    */
/*  u8 aucBuffer : Reserve for Sub Command  */
/*                    Data Structure            */
/************************************************/
struct CMD_COEX_CTRL {
    u32 u4SubCmd;
    u8 aucBuffer[64];
};

/* Sub Command Data Structure */
/************************************************/
/*  u32 u4IsoPath : WF Path (WF0/WF1)       */
/*  u32 u4Channel : WF Channel              */
/*  u32 u4Band    : WF Band (Band0/Band1)(Not used now)   */
/*  u32 u4Isolation  : Isolation value      */
/************************************************/
struct CMD_COEX_ISO_DETECT {
    u32 u4IsoPath;
    u32 u4Channel;
    /*u32 u4Band;*/
    u32 u4Isolation;
};

/* Use for Coex Ctrl Cmd */
enum ENUM_COEX_CTRL_CMD {
    ENUM_COEX_CTRL_ISO_DETECT = 1,
    ENUM_COEX_CTRL_NUM
};

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
typedef struct _CMD_ACCESS_CHN_LOAD {
    u32 u4Address;
    u32 u4Data;
    u16 u2Channel;
    u8 aucReserved[2];
} CMD_ACCESS_CHN_LOAD, *P_ACCESS_CHN_LOAD;

typedef struct _CMD_GET_LTE_SAFE_CHN_T {
    u8 ucIndex;
    u8 ucFlags;
    u8 aucReserved0[2];
    u8 aucReserved2[16];
} CMD_GET_LTE_SAFE_CHN_T, *P_CMD_GET_LTE_SAFE_CHN_T;
#endif

/* CMD_DUMP_MEMORY */
typedef struct _CMD_DUMP_MEM {
    u32 u4Address;
    u32 u4Length;
    u32 u4RemainLength;
#if CFG_SUPPORT_QA_TOOL
    u32 u4IcapContent;
#endif
    u8 ucFragNum;
} CMD_DUMP_MEM, *P_CMD_DUMP_MEM;

typedef struct _EVENT_DUMP_MEM_T {
    u32 u4Address;
    u32 u4Length;
    u32 u4RemainLength;
#if CFG_SUPPORT_QA_TOOL
    u32 eIcapContent;
#endif
    u8 ucFragNum;
    u8 aucBuffer[1];
} EVENT_DUMP_MEM_T, *P_EVENT_DUMP_MEM_T;

#if CFG_SUPPORT_QA_TOOL
typedef struct _CMD_ACCESS_RX_STAT {
    u32 u4SeqNum;
    u32 u4TotalNum;
} CMD_ACCESS_RX_STAT, *P_CMD_ACCESS_RX_STAT;

typedef struct _EVENT_ACCESS_RX_STAT {
    u32 u4SeqNum;
    u32 u4TotalNum;
    u32 au4Buffer[1];
} EVENT_ACCESS_RX_STAT, *P_EVENT_ACCESS_RX_STAT;

#if CFG_SUPPORT_TX_BF
typedef union _CMD_TXBF_ACTION_T {
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
} CMD_TXBF_ACTION_T, *P_CMD_TXBF_ACTION_T;

#define CMD_DEVINFO_UPDATE_HDR_SIZE    8
typedef struct _CMD_DEV_INFO_UPDATE_T {
    u8 ucOwnMacIdx;
    u8 ucReserve;
    u16 u2TotalElementNum;
    u8 ucAppendCmdTLV;
    u8 aucReserve[3];
    u8 aucBuffer[0];
    /* CMD_DEVINFO_ACTIVE_T rCmdDevInfoActive; */
} CMD_DEV_INFO_UPDATE_T, *P_CMD_DEV_INFO_UPDATE_T;

#define CMD_BSSINFO_UPDATE_HDR_SIZE    8
typedef struct _CMD_BSS_INFO_UPDATE_T {
    u8 ucBssIndex;
    u8 ucReserve;
    u16 u2TotalElementNum;
    u32 u4Reserve;
    /* CMD_BSSINFO_BASIC_T rCmdBssInfoBasic; */
    u8 aucBuffer[0];
} CMD_BSS_INFO_UPDATE_T, *P_CMD_BSS_INFO_UPDATE_T;

/*  STA record command */
#define CMD_STAREC_UPDATE_HDR_SIZE    8
typedef struct _CMD_STAREC_UPDATE_T {
    u8 ucBssIndex;
    u8 ucWlanIdx;
    u16 u2TotalElementNum;
    u32 u4Reserve;
    u8 aucBuffer[0];
} CMD_STAREC_UPDATE_T, *P_CMD_STAREC_UPDATE_T;

typedef struct _EVENT_PFMU_TAG_READ_T {
    PFMU_PROFILE_TAG1 ru4TxBfPFMUTag1;
    PFMU_PROFILE_TAG2 ru4TxBfPFMUTag2;
} EVENT_PFMU_TAG_READ_T, *P_EVENT_PFMU_TAG_READ_T;

#if CFG_SUPPORT_MU_MIMO
typedef struct _EVENT_HQA_GET_QD {
    u32 u4EventId;
    u32 au4RawData[14];
} EVENT_HQA_GET_QD, *P_EVENT_HQA_GET_QD;

typedef struct _EVENT_HQA_GET_MU_CALC_LQ {
    u32 u4EventId;
    MU_STRUCT_LQ_REPORT rEntry;
} EVENT_HQA_GET_MU_CALC_LQ, *P_EVENT_HQA_GET_MU_CALC_LQ;

typedef struct _EVENT_SHOW_GROUP_TBL_ENTRY {
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
} EVENT_SHOW_GROUP_TBL_ENTRY, *P_EVENT_SHOW_GROUP_TBL_ENTRY;

typedef union _CMD_MUMIMO_ACTION_T {
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
} CMD_MUMIMO_ACTION_T, *P_CMD_MUMIMO_ACTION_T;
#endif
#endif
#endif

typedef struct _CMD_SW_DBG_CTRL_T {
    u32 u4Id;
    s32 u4Data;
    /* Debug Support */
    u32 u4DebugCnt[64];
} CMD_SW_DBG_CTRL_T, *P_CMD_SW_DBG_CTRL_T;

#ifdef CFG_SUPPORT_ANT_DIV
enum {
    ANT_DIV_DISABLE = 0,
    ANT_DIV_ANT_1,
    ANT_DIV_ANT_2,
    ANT_DIV_SUCCESS,
    ANT_DIV_WF_SCNING,
    ANT_DIV_BT_SCNING,
    ANT_DIV_DISCONNECT,
    ANT_DIV_INVALID_RSSI,
    ANT_DIV_MSG_TIMEOUT,
    ANT_DIV_SWH_FAIL,
    ANT_DIV_PARA_ERR,
    ANT_DIV_OTHER_FAIL,
    ANT_DIV_STATE_NUM
};

enum {
    ANT_DIV_CMD_SET_ANT = 0,
    ANT_DIV_CMD_GET_ANT,
    ANT_DIV_CMD_DETC,
    ANT_DIV_CMD_SWH,
};

struct CMD_ANT_DIV_CTRL {
    u8 ucAction;
    u8 ucAntId;
    u8 ucState;
    u8 ucRcpi;
    u8 ucReserve[8];
};
#endif

typedef struct _CMD_FW_LOG_2_HOST_CTRL_T {
    u8 ucFwLog2HostCtrl;
    u8 ucMcuDest;
#if CFG_SUPPORT_FW_DBG_LEVEL_CTRL
    u8 ucFwLogLevel;
    u8 ucReserve;
#else
    u8 ucReserve[2];
#endif
    u32 u4HostTimeSec;
    u32 u4HostTimeMSec;
} CMD_FW_LOG_2_HOST_CTRL_T, *P_CMD_FW_LOG_2_HOST_CTRL_T;

typedef struct _CMD_GET_MAGIC_PKT_INFO {
    u16 u2Type;
    u16 u2Len;
    u32 u4ConfigMask;
    u32 u4MagicPktCntTotal;
    u32 u4GpioPullLowCntTotal;
    u32 u4GpioPullHighCntTotal;
} CMD_GET_MAGIC_PKT_INFO_T, *P_CMD_GET_MAGIC_PKT_INFO_T;

typedef struct _CMD_CHIP_CONFIG_T {
    u16 u2Id;
    u8 ucType;
    u8 ucRespType;
    u16 u2MsgSize;
    u8 aucReserved0[2];
    u8 aucCmd[CHIP_CONFIG_RESP_SIZE];
} CMD_CHIP_CONFIG_T, *P_CMD_CHIP_CONFIG_T;

/* CMD_ID_LINK_ATTRIB */
typedef struct _CMD_LINK_ATTRIB {
    s8 cRssiTrigger;
    u8 ucDesiredRateLen;
    u16 u2DesiredRate[32];
    u8 ucMediaStreamMode;
    u8 aucReserved[1];
} CMD_LINK_ATTRIB, *P_CMD_LINK_ATTRIB;

/* CMD_ID_NIC_POWER_CTRL */
typedef struct _CMD_NIC_POWER_CTRL {
    u8 ucPowerMode;
    u8 aucReserved[3];
} CMD_NIC_POWER_CTRL, *P_CMD_NIC_POWER_CTRL;

/* CMD_ID_KEEP_FULL_PWR */
struct CMD_KEEP_FULL_PWR_T {
    u8 ucEnable;
    u8 aucReserved[3];
};

/* CMD_ID_POWER_SAVE_MODE */
typedef struct _CMD_PS_PROFILE_T {
    u8 ucBssIndex;
    u8 ucPsProfile;
#ifdef SUPPORT_PERIODIC_PS
    u8 ucPspCAMInt;
    u8 ucPspPSInt;
#else
    u8 aucReserved[2];
#endif
} CMD_PS_PROFILE_T, *P_CMD_PS_PROFILE_T;

/* EVENT_LINK_QUALITY */

typedef struct _LINK_QUALITY_ {
    s8 cRssi;  /* AIS Network. */
    s8 cLinkQuality;
    u16 u2LinkSpeed;  /* TX rate1 */
    u8 ucMediumBusyPercentage;  /* Read clear */
    u8 ucIsLQ0Rdy;  /* Link Quality BSS0 Ready. */
} LINK_QUALITY, *P_LINK_QUALITY;

typedef struct _EVENT_LINK_QUALITY_V2 {
    LINK_QUALITY rLq[BSSID_NUM];
} EVENT_LINK_QUALITY_V2, *P_EVENT_LINK_QUALITY_V2;

typedef struct _EVENT_LINK_QUALITY {
    s8 cRssi;
    s8 cLinkQuality;
    u16 u2LinkSpeed;
    u8 ucMediumBusyPercentage;
} EVENT_LINK_QUALITY, *P_EVENT_LINK_QUALITY;

#if CFG_SUPPORT_P2P_RSSI_QUERY
/* EVENT_LINK_QUALITY */
typedef struct _EVENT_LINK_QUALITY_EX {
    s8 cRssi;
    s8 cLinkQuality;
    u16 u2LinkSpeed;
    u8 ucMediumBusyPercentage;
    u8 ucIsLQ0Rdy;
    s8 cRssiP2P;  /* For P2P Network. */
    s8 cLinkQualityP2P;
    u16 u2LinkSpeedP2P;
    u8 ucMediumBusyPercentageP2P;
    u8 ucIsLQ1Rdy;
} EVENT_LINK_QUALITY_EX, *P_EVENT_LINK_QUALITY_EX;
#endif

/* EVENT_ID_STATISTICS */
typedef struct _EVENT_STATISTICS {
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
} EVENT_STATISTICS, *P_EVENT_STATISTICS;

/* EVENT_ID_FW_SLEEPY_NOTIFY */
typedef struct _EVENT_SLEEPY_INFO_T {
    u8 ucSleepyState;
    u8 aucReserved[3];
} EVENT_SLEEPY_INFO_T, *P_EVENT_SLEEPY_INFO_T;

typedef struct _EVENT_ACTIVATE_STA_REC_T {
    u8 aucMacAddr[6];
    u8 ucStaRecIdx;
    u8 ucBssIndex;
} EVENT_ACTIVATE_STA_REC_T, *P_EVENT_ACTIVATE_STA_REC_T;

typedef struct _EVENT_DEACTIVATE_STA_REC_T {
    u8 ucStaRecIdx;
    u8 aucReserved[3];
} EVENT_DEACTIVATE_STA_REC_T, *P_EVENT_DEACTIVATE_STA_REC_T;

/* CMD_BT_OVER_WIFI */
typedef struct _CMD_BT_OVER_WIFI {
    u8 ucAction;  /* 0: query, 1: setup, 2: destroy */
    u8 ucChannelNum;
    PARAM_MAC_ADDRESS rPeerAddr;
    u16 u2BeaconInterval;
    u8 ucTimeoutDiscovery;
    u8 ucTimeoutInactivity;
    u8 ucRole;
    u8 PAL_Capabilities;
    u8 cMaxTxPower;
    u8 ucChannelBand;
    u8 ucReserved[1];
} CMD_BT_OVER_WIFI, *P_CMD_BT_OVER_WIFI;

#if (CFG_SUPPORT_DFS_MASTER == 1)
typedef enum _ENUM_REG_DOMAIN_T {
    REG_DEFAULT = 0,
    REG_JP_53,
    REG_JP_56
} ENUM_REG_DOMAIN_T,
*P_ENUM_REG_DOMAIN_T;

typedef struct _CMD_RDD_ON_OFF_CTRL_T {
    u8 ucDfsCtrl;
    u8 ucRddIdx;
    u8 ucRddInSel;
    u8 ucRegDomain;
    u8 ucRadarDetectMode;
} CMD_RDD_ON_OFF_CTRL_T, *P_CMD_RDD_ON_OFF_CTRL_T;
#endif

/* EVENT_BT_OVER_WIFI */
typedef struct _EVENT_BT_OVER_WIFI {
    u8 ucLinkStatus;
    u8 ucSelectedChannel;
    s8 cRSSI;
    u8 ucReserved[1];
} EVENT_BT_OVER_WIFI, *P_EVENT_BT_OVER_WIFI;

/* Same with DOMAIN_SUBBAND_INFO */
typedef struct _CMD_SUBBAND_INFO {
    u8 ucRegClass;
    u8 ucBand;
    u8 ucChannelSpan;
    u8 ucFirstChannelNum;
    u8 ucNumChannels;
    u8 aucReserved[3];
} CMD_SUBBAND_INFO, *P_CMD_SUBBAND_INFO;

/* CMD_SET_DOMAIN_INFO */
#if (CFG_SUPPORT_SINGLE_SKU == 1)
typedef struct _CMD_SET_DOMAIN_INFO_V2_T {
    u32 u4CountryCode;

    u8 uc2G4Bandwidth;  /* CONFIG_BW_20_40M or CONFIG_BW_20M */
    u8 uc5GBandwidth;  /* CONFIG_BW_20_40M or CONFIG_BW_20M */
    u8 aucReserved[2];
    struct acctive_channel_list active_chs;
} CMD_SET_DOMAIN_INFO_V2_T, *P_CMD_SET_DOMAIN_INFO_V2_T;
#endif

typedef struct _CMD_SET_DOMAIN_INFO_T {
    u16 u2CountryCode;
    u16 u2IsSetPassiveScan;
    CMD_SUBBAND_INFO rSubBand[6];

    u8 uc2G4Bandwidth;  /* CONFIG_BW_20_40M or CONFIG_BW_20M */
    u8 uc5GBandwidth;  /* CONFIG_BW_20_40M or CONFIG_BW_20M */
    u8 aucReserved[2];
} CMD_SET_DOMAIN_INFO_T, *P_CMD_SET_DOMAIN_INFO_T;

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY

/* CMD_SET_PWR_LIMIT_TABLE */
typedef struct _CMD_CHANNEL_POWER_LIMIT {
    u8 ucCentralCh;
    s8 cPwrLimitCCK;
    s8 cPwrLimit20;
    s8 cPwrLimit40;
    s8 cPwrLimit80;
    s8 cPwrLimit160;
    u8 ucFlag;
    u8 aucReserved[1];
} CMD_CHANNEL_POWER_LIMIT, *P_CMD_CHANNEL_POWER_LIMIT;

typedef struct _CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_T {
    u16 u2CountryCode;
    u8 ucCountryFlag;
    u8 ucNum;
    u8 aucReserved[4];
    CMD_CHANNEL_POWER_LIMIT rChannelPowerLimit[1];
} CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_T,
*P_CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_T;

#if (CFG_SUPPORT_SINGLE_SKU == 1)
typedef struct _CMD_CHANNEL_POWER_LIMIT_V2 {
    u8 ucCentralCh;
    u8 ucReserved[3];

    u8 tx_pwr_dsss_cck;
    u8 tx_pwr_dsss_bpsk;

    u8 tx_pwr_ofdm_bpsk;  /* 6M, 9M */
    u8 tx_pwr_ofdm_qpsk;  /* 12M, 18M */
    u8 tx_pwr_ofdm_16qam;  /* 24M, 36M */
    u8 tx_pwr_ofdm_48m;
    u8 tx_pwr_ofdm_54m;

    u8 tx_pwr_ht20_bpsk;  /* MCS0*/
    u8 tx_pwr_ht20_qpsk;  /* MCS1, MCS2*/
    u8 tx_pwr_ht20_16qam;  /* MCS3, MCS4*/
    u8 tx_pwr_ht20_mcs5;  /* MCS5*/
    u8 tx_pwr_ht20_mcs6;  /* MCS6*/
    u8 tx_pwr_ht20_mcs7;  /* MCS7*/

    u8 tx_pwr_ht40_bpsk;  /* MCS0*/
    u8 tx_pwr_ht40_qpsk;  /* MCS1, MCS2*/
    u8 tx_pwr_ht40_16qam;  /* MCS3, MCS4*/
    u8 tx_pwr_ht40_mcs5;  /* MCS5*/
    u8 tx_pwr_ht40_mcs6;  /* MCS6*/
    u8 tx_pwr_ht40_mcs7;  /* MCS7*/
    u8 tx_pwr_ht40_mcs32;  /* MCS32*/

    u8 tx_pwr_vht20_bpsk;  /* MCS0*/
    u8 tx_pwr_vht20_qpsk;  /* MCS1, MCS2*/
    u8 tx_pwr_vht20_16qam;  /* MCS3, MCS4*/
    u8 tx_pwr_vht20_64qam;  /* MCS5, MCS6*/
    u8 tx_pwr_vht20_mcs7;
    u8 tx_pwr_vht20_mcs8;
    u8 tx_pwr_vht20_mcs9;

    u8 tx_pwr_vht_40;
    u8 tx_pwr_vht_80;
    u8 tx_pwr_vht_160c;
    u8 tx_pwr_vht_160nc;
    u8 tx_pwr_lg_40;
    u8 tx_pwr_lg_80;

    u8 tx_pwr_1ss_delta;
    u8 ucReserved_1[2];
} CMD_CHANNEL_POWER_LIMIT_V2, *P_CMD_CHANNEL_POWER_LIMIT_V2;

typedef struct _CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_V2_T {
    u8 ucNum;
    u8 eband;  /*ENUM_BAND_T*/
    u8 usReserved[2];
    u32 countryCode;
    CMD_CHANNEL_POWER_LIMIT_V2 rChannelPowerLimit[0];
} CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_V2_T,
*P_CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_V2_T;

#define TX_PWR_LIMIT_SECTION_NUM        5
#define TX_PWR_LIMIT_ELEMENT_NUM        7
#define TX_PWR_LIMIT_COUNTRY_STR_MAX_LEN    4
#define TX_PWR_LIMIT_MAX_VAL            63

struct CHANNEL_TX_PWR_LIMIT {
    u8 ucChannel;
    s8 rTxPwrLimitValue[TX_PWR_LIMIT_SECTION_NUM][TX_PWR_LIMIT_ELEMENT_NUM];
};

struct TX_PWR_LIMIT_DATA {
    u32 countryCode;
    u32 ucChNum;
    struct CHANNEL_TX_PWR_LIMIT *rChannelTxPwrLimit;
};

#endif

#endif

/* CMD_SET_IP_ADDRESS */
typedef struct _IPV4_NETWORK_ADDRESS {
    u8 aucIpAddr[4];
} IPV4_NETWORK_ADDRESS, *P_IPV4_NETWORK_ADDRESS;

typedef struct _CMD_SET_NETWORK_ADDRESS_LIST {
    u8 ucBssIndex;
    u8 ucAddressCount;
    u8 ucReserved[2];
    IPV4_NETWORK_ADDRESS arNetAddress[1];
} CMD_SET_NETWORK_ADDRESS_LIST, *P_CMD_SET_NETWORK_ADDRESS_LIST;

typedef struct _PATTERN_DESCRIPTION {
    u8 fgCheckBcA1;
    u8 fgCheckMcA1;
    u8 ePatternHeader;
    u8 fgAndOp;
    u8 fgNotOp;
    u8 ucPatternMask;
    u16 u2PatternOffset;
    u8 aucPattern[8];
} PATTERN_DESCRIPTION, *P_PATTERN_DESCRIPTION;

typedef struct _CMD_RAW_PATTERN_CONFIGURATION_T {
    PATTERN_DESCRIPTION arPatternDesc[4];
} CMD_RAW_PATTERN_CONFIGURATION_T, *P_CMD_RAW_PATTERN_CONFIGURATION_T;

typedef struct _CMD_PATTERN_FUNC_CONFIG {
    u8 fgBcA1En;
    u8 fgMcA1En;
    u8 fgBcA1MatchDrop;
    u8 fgMcA1MatchDrop;
} CMD_PATTERN_FUNC_CONFIG, *P_CMD_PATTERN_FUNC_CONFIG;

typedef struct _EVENT_TX_DONE_T {
    u8 ucPacketSeq;
    u8 ucStatus;
    u16 u2SequenceNumber;

    u8 ucWlanIndex;
    u8 ucTxCount;
    u16 u2TxRate;

    u8 ucFlag;
    u8 ucTid;
    u8 ucRspRate;
    u8 ucRateTableIdx;

    u8 ucBandwidth;
    u8 ucTxPower;
    u8 aucReserved0[2];

    u32 u4TxDelay;
    u32 u4Timestamp;
    u32 u4AppliedFlag;

    u8 aucRawTxS[28];

    u8 aucReserved1[32];
} EVENT_TX_DONE_T, *P_EVENT_TX_DONE_T;

typedef enum _ENUM_TXS_APPLIED_FLAG_T {
    TX_FRAME_IN_AMPDU_FORMAT = 0,
    TX_FRAME_EXP_BF,
    TX_FRAME_IMP_BF,
    TX_FRAME_PS_BIT
} ENUM_TXS_APPLIED_FLAG_T,
*P_ENUM_TXS_APPLIED_FLAG_T;

typedef enum _ENUM_TXS_CONTROL_FLAG_T {
    TXS_WITH_ADVANCED_INFO = 0,
    TXS_IS_EXIST
} ENUM_TXS_CONTROL_FLAG_T,
*P_ENUM_TXS_CONTROL_FLAG_T;

#if (CFG_SUPPORT_DFS_MASTER == 1)
typedef enum _ENUM_DFS_CTRL_T {
    RDD_STOP = 0,
    RDD_START,
    RDD_DET_MODE,
    RDD_RADAR_EMULATE,
    RDD_START_TXQ
} ENUM_DFS_CTRL_T,
*P_ENUM_DFS_CTRL_T;
#endif

typedef struct _CMD_BSS_ACTIVATE_CTRL {
    u8 ucBssIndex;
    u8 ucActive;
    u8 ucNetworkType;
    u8 ucOwnMacAddrIndex;
    u8 aucBssMacAddr[6];
    u8 ucBMCWlanIndex;
    u8 ucReserved;
} CMD_BSS_ACTIVATE_CTRL, *P_CMD_BSS_ACTIVATE_CTRL;

typedef struct _CMD_SET_BSS_RLM_PARAM_T {
    u8 ucBssIndex;
    u8 ucRfBand;
    u8 ucPrimaryChannel;
    u8 ucRfSco;
    u8 ucErpProtectMode;
    u8 ucHtProtectMode;
    u8 ucGfOperationMode;
    u8 ucTxRifsMode;
    u16 u2HtOpInfo3;
    u16 u2HtOpInfo2;
    u8 ucHtOpInfo1;
    u8 ucUseShortPreamble;
    u8 ucUseShortSlotTime;
    u8 ucVhtChannelWidth;
    u8 ucVhtChannelFrequencyS1;
    u8 ucVhtChannelFrequencyS2;
    u16 u2VhtBasicMcsSet;
    u8 ucNss;
} CMD_SET_BSS_RLM_PARAM_T, *P_CMD_SET_BSS_RLM_PARAM_T;

typedef struct _CMD_SET_BSS_INFO {
    u8 ucBssIndex;
    u8 ucConnectionState;
    u8 ucCurrentOPMode;
    u8 ucSSIDLen;
    u8 aucSSID[32];
    u8 aucBSSID[6];
    u8 ucIsQBSS;
    u8 ucReserved1;
    u16 u2OperationalRateSet;
    u16 u2BSSBasicRateSet;
    u8 ucStaRecIdxOfAP;
    u16 u2HwDefaultFixedRateCode;
    u8 ucNonHTBasicPhyType;  /* For Slot Time and CWmin */
    u8 ucAuthMode;
    u8 ucEncStatus;
    u8 ucPhyTypeSet;
    u8 ucWapiMode;
    u8 ucIsApMode;
    u8 ucBMCWlanIndex;
    u8 ucHiddenSsidMode;
    u8 ucDisconnectDetectTh;
    u32 u4PrivateData;
    CMD_SET_BSS_RLM_PARAM_T rBssRlmParam;
    u8 ucDBDCBand;
    u8 ucWmmSet;
    u8 ucDBDCAction;
    u8 ucNss;
    u8 aucReserved[20];
} CMD_SET_BSS_INFO, *P_CMD_SET_BSS_INFO;

typedef enum _ENUM_RTS_POLICY_T {
    RTS_POLICY_AUTO,
    RTS_POLICY_STATIC_BW,
    RTS_POLICY_DYNAMIC_BW,
    RTS_POLICY_LEGACY,
    RTS_POLICY_NO_RTS
} ENUM_RTS_POLICY;

typedef struct _CMD_UPDATE_STA_RECORD_T {
    u8 ucStaIndex;
    u8 ucStaType;
    /* This field should assign at create and keep consistency for update
     * usage */
    u8 aucMacAddr[MAC_ADDR_LEN];

    u16 u2AssocId;
    u16 u2ListenInterval;
    u8 ucBssIndex;  /* This field should assign at create and keep
                     * consistency for update usage */
    u8 ucDesiredPhyTypeSet;
    u16 u2DesiredNonHTRateSet;

    u16 u2BSSBasicRateSet;
    u8 ucIsQoS;
    u8 ucIsUapsdSupported;
    u8 ucStaState;
    u8 ucMcsSet;
    u8 ucSupMcs32;
    u8 aucReserved1[1];

    u8 aucRxMcsBitmask[10];
    u16 u2RxHighestSupportedRate;
    u32 u4TxRateInfo;

    u16 u2HtCapInfo;
    u16 u2HtExtendedCap;
    u32 u4TxBeamformingCap;

    u8 ucAmpduParam;
    u8 ucAselCap;
    u8 ucRCPI;
    u8 ucNeedResp;
    u8 ucUapsdAc;  /* b0~3: Trigger enabled, b4~7: Delivery enabled */
    u8 ucUapsdSp;  /* 0: all, 1: max 2, 2: max 4, 3: max 6 */
    u8 ucWlanIndex;  /* This field should assign at create and keep
                      * consistency for update usage */
    u8 ucBMCWlanIndex;  /* This field should assign at create and keep
                         * consistency for update usage */

    u32 u4VhtCapInfo;
    u16 u2VhtRxMcsMap;
    u16 u2VhtRxHighestSupportedDataRate;
    u16 u2VhtTxMcsMap;
    u16 u2VhtTxHighestSupportedDataRate;
    u8 ucRtsPolicy;  /* 0: auto 1: Static BW 2: Dynamic BW 3: Legacy 7: WoRts
                      */
    u8 ucVhtOpMode;  /* VHT operating mode, bit 7: Rx NSS Type, bit 4-6, Rx
                      * NSS, bit 0-1: Channel Width */

    u8 ucTrafficDataType;  /* 0: auto 1: data 2: video 3: voice */
    u8 ucTxGfMode;
    u8 ucTxSgiMode;
    u8 ucTxStbcMode;
    u16 u2HwDefaultFixedRateCode;
    u8 ucTxAmpdu;
    u8 ucRxAmpdu;
    u32 u4FixedPhyRate;  /**/
    u16 u2MaxLinkSpeed;  /* unit is 0.5 Mbps */
    u16 u2MinLinkSpeed;

    u32 u4Flags;

    u8 ucTxBaSize;
    u8 ucRxBaSize;
    u8 aucReserved3[2];

    TXBF_PFMU_STA_INFO rTxBfPfmuInfo;

    u8 ucTxAmsduInAmpdu;
    u8 ucRxAmsduInAmpdu;
    u8 aucReserved5[2];

    u32 u4TxMaxAmsduInAmpduLen;
    /* u8 aucReserved4[30]; */
} CMD_UPDATE_STA_RECORD_T, *P_CMD_UPDATE_STA_RECORD_T;

typedef struct _CMD_REMOVE_STA_RECORD_T {
    u8 ucActionType;
    u8 ucStaIndex;
    u8 ucBssIndex;
    u8 ucReserved;
} CMD_REMOVE_STA_RECORD_T, *P_CMD_REMOVE_STA_RECORD_T;

typedef struct _CMD_INDICATE_PM_BSS_CREATED_T {
    u8 ucBssIndex;
    u8 ucDtimPeriod;
    u16 u2BeaconInterval;
    u16 u2AtimWindow;
    u8 aucReserved[2];
} CMD_INDICATE_PM_BSS_CREATED, *P_CMD_INDICATE_PM_BSS_CREATED;

typedef struct _CMD_INDICATE_PM_BSS_CONNECTED_T {
    u8 ucBssIndex;
    u8 ucDtimPeriod;
    u16 u2AssocId;
    u16 u2BeaconInterval;
    u16 u2AtimWindow;
    u8 fgIsUapsdConnection;
    u8 ucBmpDeliveryAC;
    u8 ucBmpTriggerAC;
    u8 aucReserved[1];
} CMD_INDICATE_PM_BSS_CONNECTED, *P_CMD_INDICATE_PM_BSS_CONNECTED;

typedef struct _CMD_INDICATE_PM_BSS_ABORT {
    u8 ucBssIndex;
    u8 aucReserved[3];
} CMD_INDICATE_PM_BSS_ABORT, *P_CMD_INDICATE_PM_BSS_ABORT;

typedef struct _CMD_BEACON_TEMPLATE_UPDATE {
    /* 0: update randomly, 1: update all, 2: delete all (1 and 2 will update
     * directly without search) */
    u8 ucUpdateMethod;
    u8 ucBssIndex;
    u8 aucReserved[2];
    u16 u2Capability;
    u16 u2IELen;
    u8 aucIE[MAX_IE_LENGTH];
} CMD_BEACON_TEMPLATE_UPDATE, *P_CMD_BEACON_TEMPLATE_UPDATE;

typedef struct _CMD_SET_WMM_PS_TEST_STRUCT_T {
    u8 ucBssIndex;
    u8 bmfgApsdEnAc;  /* b0~3: trigger-en AC0~3. b4~7: delivery-en AC0~3 */
    u8 ucIsEnterPsAtOnce;  /* enter PS immediately without 5 second guard
                            * after connected */
    u8 ucIsDisableUcTrigger;  /* not to trigger UC on beacon TIM is matched
                               * (under U-APSD) */
} CMD_SET_WMM_PS_TEST_STRUCT_T, *P_CMD_SET_WMM_PS_TEST_STRUCT_T;

/* Definition for CHANNEL_INFO.ucBand:
 * 0:       Reserved
 * 1:       BAND_2G4
 * 2:       BAND_5G
 * Others:  Reserved
 */
typedef struct _CHANNEL_INFO_T {
    u8 ucBand;
    u8 ucChannelNum;
} CHANNEL_INFO_T, *P_CHANNEL_INFO_T;

typedef struct _CMD_SCAN_REQ_T {
    u8 ucSeqNum;
    u8 ucBssIndex;
    u8 ucScanType;
    u8 ucSSIDType;  /* BIT(0) wildcard / BIT(1) P2P-wildcard / BIT(2)
                     * specific */
    u8 ucSSIDLength;
    u8 ucNumProbeReq;
    u16 u2ChannelMinDwellTime;
    u16 u2ChannelDwellTime;
    u16 u2TimeoutValue;
    u8 aucSSID[32];
    u8 ucChannelType;
    u8 ucChannelListNum;
    u8 aucReserved[2];
    CHANNEL_INFO_T arChannelList[32];
    u16 u2IELen;
    u8 aucIE[MAX_IE_LENGTH];
} CMD_SCAN_REQ, *P_CMD_SCAN_REQ;

typedef struct _CMD_SCAN_REQ_V2_T {
    u8 ucSeqNum;
    u8 ucBssIndex;
    u8 ucScanType;
    u8 ucSSIDType;
    u8 ucSSIDNum;
    u8 ucNumProbeReq;
    u8 aucReserved[2];  /*total 8*/
    PARAM_SSID_T arSSID[4];  /*(4+32)*4 = 144, total 152*/
    u16 u2ProbeDelayTime;
    u16 u2ChannelDwellTime;
    u16 u2TimeoutValue;
    u8 ucChannelType;
    u8 ucChannelListNum;  /*total 160*/
    CHANNEL_INFO_T arChannelList[32];  /*total 160+64=224*/
    u16 u2IELen;  /*total 226*/
    u8 aucIE[MAX_IE_LENGTH];  /*total 826*/
    u8 ucScnCtrlFlag;
    u8 aucReserved2;  /*total 828*/
    /*Extend for Scan cmds*/
    CHANNEL_INFO_T arChannelListExtend[32];  /*total 892*/
    u8 arPerChannelControl[32];
    u8 arPerExtendChannelControl[32];  /*total 956*/
    u8 ucScanChannelListenTime;  /*total 957*/
    u8 aucReserved3[3];  /*total 960, max 1024*/
} CMD_SCAN_REQ_V2, *P_CMD_SCAN_REQ_V2;

typedef struct _CMD_SCAN_CANCEL_T {
    u8 ucSeqNum;
    u8 ucIsExtChannel;  /* For P2P channel extension. */
    u8 aucReserved[2];
} CMD_SCAN_CANCEL, *P_CMD_SCAN_CANCEL;

/* 20150107  Daniel Added complete channels number in the scan done event */
/* before*/
/*
 * typedef struct _EVENT_SCAN_DONE_T {
 * u8          ucSeqNum;
 * u8          ucSparseChannelValid;
 * CHANNEL_INFO_T  rSparseChannel;
 *} EVENT_SCAN_DONE, *P_EVENT_SCAN_DONE;
 */
/* after */

#define EVENT_SCAN_DONE_CHANNEL_NUM_MAX    64
typedef struct _EVENT_SCAN_DONE_T {
    u8 ucSeqNum;
    u8 ucSparseChannelValid;
    CHANNEL_INFO_T rSparseChannel;
    /*scan done version #2 */
    u8 ucCompleteChanCount;
    u8 ucCurrentState;
    u8 ucScanDoneVersion;
    /*scan done version #3 */
    u8 ucReserved;
    u32 u4ScanDurBcnCnt;
    u8 fgIsPNOenabled;
    u8 aucReserving[3];
    /*channel idle count # Mike */
    u8 ucSparseChannelArrayValidNum;
    u8 aucReserved[3];
    u8 aucChannelNum[EVENT_SCAN_DONE_CHANNEL_NUM_MAX];
    /* Idle format for au2ChannelIdleTime */
    /* 0: first bytes: idle time(ms) 2nd byte: dwell time(ms) */
    /* 1: first bytes: idle time(8ms) 2nd byte: dwell time(8ms) */
    /* 2: dwell time (16us) */
    u16 au2ChannelIdleTime[EVENT_SCAN_DONE_CHANNEL_NUM_MAX];
    /* B0: Active/Passive B3-B1: Idle format  */
    u8 aucChannelFlag[EVENT_SCAN_DONE_CHANNEL_NUM_MAX];
    u8 aucChannelMDRDYCnt[EVENT_SCAN_DONE_CHANNEL_NUM_MAX];
} EVENT_SCAN_DONE, *P_EVENT_SCAN_DONE;

#if CFG_SUPPORT_BATCH_SCAN
typedef struct _CMD_BATCH_REQ_T {
    u8 ucSeqNum;
    u8 ucNetTypeIndex;
    u8 ucCmd;  /* Start/ Stop */
    u8 ucMScan;  /* an integer number of scans per batch */
    u8 ucBestn;  /* an integer number of the max AP to remember per scan */
    u8 ucRtt;  /* an integer number of highest-strength AP for which we'd */
    /* like approximate distance reported */
    u8 ucChannel;  /* channels */
    u8 ucChannelType;
    u8 ucChannelListNum;
    u8 aucReserved[3];
    u32 u4Scanfreq;  /* an integer number of seconds between scans */
    CHANNEL_INFO_T arChannelList[32];  /* channels */
} CMD_BATCH_REQ_T, *P_CMD_BATCH_REQ_T;

typedef struct _EVENT_BATCH_RESULT_ENTRY_T {
    u8 aucBssid[MAC_ADDR_LEN];
    u8 aucSSID[ELEM_MAX_LEN_SSID];
    u8 ucSSIDLen;
    s8 cRssi;
    u32 ucFreq;
    u32 u4Age;
    u32 u4Dist;
    u32 u4Distsd;
} EVENT_BATCH_RESULT_ENTRY_T, *P_EVENT_BATCH_RESULT_ENTRY_T;

typedef struct _EVENT_BATCH_RESULT_T {
    u8 ucScanCount;
    u8 aucReserved[3];
    EVENT_BATCH_RESULT_ENTRY_T
        arBatchResult[12];  /* Must be the same with SCN_BATCH_STORE_MAX_NUM */
} EVENT_BATCH_RESULT_T, *P_EVENT_BATCH_RESULT_T;
#endif

typedef struct _CMD_CH_PRIVILEGE_T {
    u8 ucBssIndex;
    u8 ucTokenID;
    u8 ucAction;
    u8 ucPrimaryChannel;
    u8 ucRfSco;
    u8 ucRfBand;
    u8 ucRfChannelWidth;  /* To support 80/160MHz bandwidth */
    u8 ucRfCenterFreqSeg1;  /* To support 80/160MHz bandwidth */
    u8 ucRfCenterFreqSeg2;  /* To support 80/160MHz bandwidth */
    u8 ucReqType;
    u8 ucDBDCBand;
    u8 aucReserved;
    u32 u4MaxInterval;  /* In unit of ms */
    u8 aucReserved2[8];
} CMD_CH_PRIVILEGE_T, *P_CMD_CH_PRIVILEGE_T;

typedef struct _CMD_TX_PWR_T {
    s8 cTxPwr2G4Cck;  /* signed, in unit of 0.5dBm */
    s8 cTxPwr2G4Dsss;  /* signed, in unit of 0.5dBm */
    s8 acReserved[2];

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
} CMD_TX_PWR_T, *P_CMD_TX_PWR_T;

typedef struct _CMD_TX_AC_PWR_T {
    s8 ucBand;
    AC_PWR_SETTING_STRUCT rAcPwr;
} CMD_TX_AC_PWR_T, *P_CMD_TX_AC_PWR_T;

typedef struct _CMD_RSSI_PATH_COMPASATION_T {
    s8 c2GRssiCompensation;
    s8 c5GRssiCompensation;
} CMD_RSSI_PATH_COMPASATION_T, *P_CMD_RSSI_PATH_COMPASATION_T;
typedef struct _CMD_5G_PWR_OFFSET_T {
    s8 cOffsetBand0;  /* 4.915-4.980G */
    s8 cOffsetBand1;  /* 5.000-5.080G */
    s8 cOffsetBand2;  /* 5.160-5.180G */
    s8 cOffsetBand3;  /* 5.200-5.280G */
    s8 cOffsetBand4;  /* 5.300-5.340G */
    s8 cOffsetBand5;  /* 5.500-5.580G */
    s8 cOffsetBand6;  /* 5.600-5.680G */
    s8 cOffsetBand7;  /* 5.700-5.825G */
} CMD_5G_PWR_OFFSET_T, *P_CMD_5G_PWR_OFFSET_T;

typedef struct _CMD_PWR_PARAM_T {
    u32 au4Data[28];
    u32 u4RefValue1;
    u32 u4RefValue2;
} CMD_PWR_PARAM_T, *P_CMD_PWR_PARAM_T;

typedef struct _CMD_PHY_PARAM_T {
    u8 aucData[144];  /* eFuse content */
} CMD_PHY_PARAM_T, *P_CMD_PHY_PARAM_T;

typedef struct _CMD_AUTO_POWER_PARAM_T {
    u8 ucType;  /* 0: Disable 1: Enalbe 0x10: Change parameters */
    u8 ucBssIndex;
    u8 aucReserved[2];
    u8 aucLevelRcpiTh[3];
    u8 aucReserved2[1];
    s8 aicLevelPowerOffset[3];  /* signed, in unit of 0.5dBm */
    u8 aucReserved3[1];
    u8 aucReserved4[8];
} CMD_AUTO_POWER_PARAM_T, *P_CMD_AUTO_POWER_PARAM_T;

typedef struct _CMD_DBDC_SETTING_T {
    u8 ucDbdcEn;
    u8 ucWmmBandBitmap;
    u8 ucUpdateSettingNextChReq;
    u8 aucReserved1;
    u8 aucReserved2[32];
} CMD_DBDC_SETTING_T, *P_CMD_DBDC_SETTING_T;

typedef struct _EVENT_CH_PRIVILEGE_T {
    u8 ucBssIndex;
    u8 ucTokenID;
    u8 ucStatus;
    u8 ucPrimaryChannel;
    u8 ucRfSco;
    u8 ucRfBand;
    u8 ucRfChannelWidth;  /* To support 80/160MHz bandwidth */
    u8 ucRfCenterFreqSeg1;  /* To support 80/160MHz bandwidth */
    u8 ucRfCenterFreqSeg2;  /* To support 80/160MHz bandwidth */
    u8 ucReqType;
    u8 ucDBDCBand;
    u8 aucReserved;
    u32 u4GrantInterval;  /* In unit of ms */
    u8 aucReserved2[8];
} EVENT_CH_PRIVILEGE_T, *P_EVENT_CH_PRIVILEGE_T;

#if (CFG_SUPPORT_DFS_MASTER == 1)
typedef struct _LONG_PULSE_BUFFER_T {
    u32 u4LongStartTime;
    u16 u2LongPulseWidth;
} LONG_PULSE_BUFFER_T, *PLONG_PULSE_BUFFER_T;

typedef struct _PERIODIC_PULSE_BUFFER_T {
    u32 u4PeriodicStartTime;
    u16 u2PeriodicPulseWidth;
    s16 i2PeriodicPulsePower;
} PERIODIC_PULSE_BUFFER_T, *PPERIODIC_PULSE_BUFFER_T;

typedef struct _EVENT_RDD_REPORT_T {
    u8 ucRadarReportMode;  /*0: Only report radar detected; 1:  Add parameter
                            * reports*/
    u8 ucRddIdx;
    u8 ucLongDetected;
    u8 ucPeriodicDetected;
    u8 ucLPBNum;
    u8 ucPPBNum;
    u8 ucLPBPeriodValid;
    u8 ucLPBWidthValid;
    u8 ucPRICountM1;
    u8 ucPRICountM1TH;
    u8 ucPRICountM2;
    u8 ucPRICountM2TH;
    u32 u4PRI1stUs;
    LONG_PULSE_BUFFER_T arLpbContent[LPB_SIZE];
    PERIODIC_PULSE_BUFFER_T arPpbContent[PPB_SIZE];
} EVENT_RDD_REPORT_T, *P_EVENT_RDD_REPORT_T;
#endif

#if (CFG_WOW_SUPPORT == 1)
/* event of wake up reason */
struct _EVENT_WAKEUP_REASON_INFO {
    u8 reason;
    u8 aucReserved[3];
};
#endif

typedef struct _EVENT_BSS_BEACON_TIMEOUT_T {
    u8 ucBssIndex;
    u8 ucReasonCode;
    u8 aucReserved[2];
} EVENT_BSS_BEACON_TIMEOUT_T, *P_EVENT_BSS_BEACON_TIMEOUT_T;

typedef struct _EVENT_STA_AGING_TIMEOUT_T {
    u8 ucStaRecIdx;
    u8 aucReserved[3];
} EVENT_STA_AGING_TIMEOUT_T, *P_EVENT_STA_AGING_TIMEOUT_T;

typedef struct _EVENT_NOA_TIMING_T {
    u8 ucIsInUse;  /* Indicate if this entry is in use or not */
    u8 ucCount;  /* Count */
    u8 aucReserved[2];

    u32 u4Duration;  /* Duration */
    u32 u4Interval;  /* Interval */
    u32 u4StartTime;  /* Start Time */
} EVENT_NOA_TIMING_T, *P_EVENT_NOA_TIMING_T;

typedef struct _EVENT_UPDATE_NOA_PARAMS_T {
    u8 ucBssIndex;
    u8 aucReserved[2];
    u8 ucEnableOppPS;
    u16 u2CTWindow;

    u8 ucNoAIndex;
    u8 ucNoATimingCount;  /* Number of NoA Timing */
    EVENT_NOA_TIMING_T arEventNoaTiming[8  /*P2P_MAXIMUM_NOA_COUNT */ ];
} EVENT_UPDATE_NOA_PARAMS_T, *P_EVENT_UPDATE_NOA_PARAMS_T;

typedef struct _EVENT_AP_OBSS_STATUS_T {
    u8 ucBssIndex;
    u8 ucObssErpProtectMode;
    u8 ucObssHtProtectMode;
    u8 ucObssGfOperationMode;
    u8 ucObssRifsOperationMode;
    u8 ucObssBeaconForcedTo20M;
    u8 aucReserved[2];
} EVENT_AP_OBSS_STATUS_T, *P_EVENT_AP_OBSS_STATUS_T;

typedef struct _EVENT_DEBUG_MSG_T {
    u16 u2DebugMsgId;
    u8 ucMsgType;
    u8 ucFlags;  /* unused */
    u32 u4Value;  /* memory addre or ... */
    u16 u2MsgSize;
    u8 aucReserved0[2];
    u8 aucMsg[1];
} EVENT_DEBUG_MSG_T, *P_EVENT_DEBUG_MSG_T;

typedef struct _CMD_EDGE_TXPWR_LIMIT_T {
    s8 cBandEdgeMaxPwrCCK;
    s8 cBandEdgeMaxPwrOFDM20;
    s8 cBandEdgeMaxPwrOFDM40;
    s8 cBandEdgeMaxPwrOFDM80;
} CMD_EDGE_TXPWR_LIMIT_T, *P_CMD_EDGE_TXPWR_LIMIT_T;

typedef struct _CMD_POWER_OFFSET_T {
    u8 ucBand;  /*1:2.4G ;  2:5G */
    u8 ucSubBandOffset[MAX_SUBBAND_NUM_5G];  /*the max num subband is 5G,
                                              * devide with 8 subband */
    u8 aucReverse[3];
} CMD_POWER_OFFSET_T, *P_CMD_POWER_OFFSET_T;

typedef struct _CMD_NVRAM_SETTING_T {
    WIFI_CFG_PARAM_STRUCT rNvramSettings;
} CMD_NVRAM_SETTING_T, *P_CMD_NVRAM_SETTING_T;

#if CFG_SUPPORT_TDLS
typedef struct _CMD_TDLS_CH_SW_T {
    u8 fgIsTDLSChSwProhibit;
} CMD_TDLS_CH_SW_T, *P_CMD_TDLS_CH_SW_T;
#endif

struct EVENT_CSI_DATA_T {
    u8 ucBw;
    u8 bIsCck;
    u16 u2DataCount;
    s16 ac2IData[256];
    s16 ac2QData[256];
    u8 ucDbdcIdx;
    u8 aucReserved[3];
};

#if CFG_SUPPORT_ADVANCE_CONTROL
/* command type */
#define CMD_ADV_CONTROL_SET              (1 << 15)
#define CMD_PTA_CONFIG_TYPE              (0x1)
#define CMD_AFH_CONFIG_TYPE              (0x2)
#define CMD_BA_CONFIG_TYPE               (0x3)
#define CMD_GET_REPORT_TYPE              (0x4)
#define CMD_NOISE_HISTOGRAM_TYPE             (0x5)
#if CFG_IPI_2CHAIN_SUPPORT
#define CMD_NOISE_HISTOGRAM_TYPE2            (0x51)
#endif
#define CMD_ADMINCTRL_CONFIG_TYPE            (0x6)
#ifdef CFG_SUPPORT_EXT_PTA_DEBUG_COMMAND
#define CMD_EXT_PTA_CONFIG_TYPE              (0x7)
#endif
/* 0x8 is reserved for GARP count */
#define CMD_GET_MAGIC_PKT_INFO_TYPE          (0x9)

/* for PtaConfig field */
#define CMD_PTA_CONFIG_PTA_EN                (1 << 0)
#define CMD_PTA_CONFIG_RW_EN                 (1 << 1)
#define CMD_PTA_CONFIG_PTA_STAT_EN           (1 << 2)

/* pta config related mask */
#define CMD_PTA_CONFIG_PTA               (1 << 0)
#define CMD_PTA_CONFIG_RW                (1 << 1)
#define CMD_PTA_CONFIG_TXDATA_TAG            (1 << 2)
#define CMD_PTA_CONFIG_RXDATAACK_TAG             (1 << 3)
#define CMD_PTA_CONFIG_RX_NSW_TAG            (1 << 4)
#define CMD_PTA_CONFIG_TXACK_TAG             (1 << 5)
#define CMD_PTA_CONFIG_TXPROTFRAME_TAG           (1 << 6)
#define CMD_PTA_CONFIG_RXPROTFRAMEACK_TAG        (1 << 7)
#define CMD_PTA_CONFIG_TX_BMC_TAG            (1 << 8)
#define CMD_PTA_CONFIG_TX_BCN_TAG            (1 << 9)
#define CMD_PTA_CONFIG_RX_SP_TAG             (1 << 10)
#define CMD_PTA_CONFIG_TX_MGMT_TAG           (1 << 11)
#define CMD_PTA_CONFIG_RXMGMTACK_TAG             (1 << 12)
#define CMD_PTA_CONFIG_PTA_STAT              (1 << 13)
#define CMD_PTA_CONFIG_PTA_STAT_RESET            (1 << 14)
#define CMD_PTA_CONFIG_COMM_ACT_BT_WF0_INBAND        (1 << 15)
#define CMD_PTA_CONFIG_COMM_ACT_BT_WF0_OUTBAND       (1 << 16)
#define CMD_PTA_CONFIG_COMM_ACT_BT_WF1_INBAND        (1 << 17)
#define CMD_PTA_CONFIG_COMM_ACT_BT_WF1_OUTBAND       (1 << 18)

#ifdef CFG_SUPPORT_EXT_PTA_DEBUG_COMMAND
/* ext pta config related mask */
#define CMD_EXT_PTA_CONFIG_EXT_PTA           (1 << 0)
#define CMD_EXT_PTA_CONFIG_HI_RX_TAG             (1 << 1)
#define CMD_EXT_PTA_CONFIG_LO_RX_TAG             (1 << 2)
#define CMD_EXT_PTA_CONFIG_HI_TX_TAG             (1 << 3)
#define CMD_EXT_PTA_CONFIG_LO_TX_TAG             (1 << 4)
#define CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_BT_UNSAFE     (1 << 5)
#define CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_WF0_UNSAFE    (1 << 6)
#define CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_WF1_UNSAFE    (1 << 7)
#define CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_BT_HSF        (1 << 8)
#define CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_WF0_HSF       (1 << 9)
#define CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_WF1_HSF       (1 << 10)
#endif

/* for config PTA Tag */
#define EVENT_CONFIG_PTA_OFFSET              (0)
#define EVENT_CONFIG_PTA_FEILD               (0x1)
#define EVENT_CONFIG_PTA_WIFI_OFFSET             (12)
#define EVENT_CONFIG_PTA_WIFI_FEILD          (0x1)
#define EVENT_CONFIG_PTA_BT_OFFSET           (15)
#define EVENT_CONFIG_PTA_BT_FEILD            (0x1)
#define EVENT_CONFIG_PTA_ARB_OFFSET          (16)
#define EVENT_CONFIG_PTA_ARB_FEILD           (0x1)

#define EVENT_CONFIG_WIFI_GRANT_OFFSET           (30)
#define EVENT_CONFIG_WIFI_GRANT_FEILD            (0x1)
#define EVENT_CONFIG_WIFI_PRI_OFFSET             (12)
#define EVENT_CONFIG_WIFI_PRI_FEILD          (0xf)
#define EVENT_CONFIG_WIFI_TXREQ_OFFSET           (8)
#define EVENT_CONFIG_WIFI_TXREQ_FEILD            (0x1)
#define EVENT_CONFIG_WIFI_RXREQ_OFFSET           (9)
#define EVENT_CONFIG_WIFI_RXREQ_FEILD            (0x1)

#define EVENT_CONFIG_BT_GRANT_OFFSET             (29)
#define EVENT_CONFIG_BT_GRANT_FEILD          (0x1)
#define EVENT_CONFIG_BT_PRI_OFFSET           (4)
#define EVENT_CONFIG_BT_PRI_FEILD            (0xf)
#define EVENT_CONFIG_BT_TXREQ_OFFSET             (0)
#define EVENT_CONFIG_BT_TXREQ_FEILD          (0x1)
#define EVENT_CONFIG_BT_RXREQ_OFFSET             (1)
#define EVENT_CONFIG_BT_RXREQ_FEILD          (0x1)

#define EVENT_PTA_BTTRX_CNT_OFFSET           (16)
#define EVENT_PTA_BTTRX_CNT_FEILD            (0xFFFF)
#define EVENT_PTA_BTTRX_GRANT_CNT_OFFSET         (0)
#define EVENT_PTA_BTTRX_GRANT_CNT_FEILD          (0xFFFF)

#define EVENT_PTA_WFTRX_CNT_OFFSET           (16)
#define EVENT_PTA_WFTRX_CNT_FEILD            (0xFFFF)
#define EVENT_PTA_WFTRX_GRANT_CNT_OFFSET         (0)
#define EVENT_PTA_WFTRX_GRANT_CNT_FEILD          (0xFFFF)

#define EVENT_PTA_TX_ABT_CNT_OFFSET          (16)
#define EVENT_PTA_TX_ABT_CNT_FEILD           (0xFFFF)
#define EVENT_PTA_RX_ABT_CNT_OFFSET          (0)
#define EVENT_PTA_RX_ABT_CNT_FEILD           (0xFFFF)
typedef struct _CMD_PTA_CONFIG {
    u16 u2Type;
    u16 u2Len;
    u32 u4ConfigMask;
    /* common usage in set/get */
    u32 u4PtaConfig;
    u32 u4TxDataTag;
    u32 u4RxDataAckTag;
    u32 u4RxNswTag;
    u32 u4TxAckTag;
    u32 u4TxProtFrameTag;
    u32 u4RxProtFrameAckTag;
    u32 u4TxBMCTag;
    u32 u4TxBCNTag;
    u32 u4RxSPTag;
    u32 u4TxMgmtTag;
    u32 u4RxMgmtAckTag;
    u32 u4CommActBtWf0Inband;
    u32 u4CommActBtWf0Outband;
    u32 u4CommActBtWf1Inband;
    u32 u4CommActBtWf1Outband;
    /* Only used in get */
    u32 u4PtaWF0TxCnt;
    u32 u4PtaWF0RxCnt;
    u32 u4PtaWF0AbtCnt;
    u32 u4PtaWF1TxCnt;
    u32 u4PtaWF1RxCnt;
    u32 u4PtaWF1AbtCnt;
    u32 u4PtaBTTxCnt;
    u32 u4PtaBTRxCnt;
    u32 u4PtaBTAbtCnt;
    u32 u4GrantStat;
    u32 u4CoexMode;
} CMD_PTA_CONFIG_T, *P_CMD_PTA_CONFIG_T;

#ifdef CFG_SUPPORT_EXT_PTA_DEBUG_COMMAND
typedef struct _CMD_EXT_PTA_CONFIG {
    u16 u2Type;
    u16 u2Len;
    u32 u4ConfigMask;
    /* common usage in set/get */
    u32 u4ExtPtaConfig;
    u32 u4ZbHiRxTag;
    u32 u4ZbLoRxTag;
    u32 u4ZbHiTxTag;
    u32 u4ZbLoTxTag;
    u32 u4CommActZbBtUnsafe;
    u32 u4CommActZbWf0Unsafe;
    u32 u4CommActZbWf1Unsafe;
    u32 u4CommActZbBtHsf;
    u32 u4CommActZbWf0Hsf;
    u32 u4CommActZbWf1Hsf;
    /* used in get */
    u32 u4ZbGntCnt;
    u32 u4ZbAbtCnt;
    u32 u4ZbLoTxReqCnt;
    u32 u4ZbHiTxReqCnt;
    u32 u4ZbLoRxReqCnt;
    u32 u4ZbHiRxReqCnt;
} CMD_EXT_PTA_CONFIG_T, *P_CMD_EXT_PTA_CONFIG_T;
#endif

/* get report related */
enum _ENUM_GET_REPORT_ACTION_T {
    CMD_GET_REPORT_ENABLE = 1,
    CMD_GET_REPORT_DISABLE,
    CMD_GET_REPORT_RESET,
    CMD_GET_REPORT_GET,
    CMD_SET_REPORT_SAMPLE_DUR,
    CMD_SET_REPORT_SAMPLE_POINT,
    CMD_SET_REPORT_TXTHRES,
    CMD_SET_REPORT_RXTHRES,
    CMD_GET_REPORT_ACTIONS
};
#define EVENT_REPORT_OFDM_FCCA        (16)
#define EVENT_REPORT_OFDM_FCCA_FEILD      (0xffff)
#define EVENT_REPORT_CCK_FCCA         (0)
#define EVENT_REPORT_CCK_FCCA_FEILD   (0xffff)
#define EVENT_REPORT_OFDM_SIGERR      (16)
#define EVENT_REPORT_OFDM_SIGERR_FEILD    (0xffff)
#define EVENT_REPORT_CCK_SIGERR       (0)
#define EVENT_REPORT_CCK_SIGERR_FEILD     (0xffff)
struct CMD_GET_TRAFFIC_REPORT {
    u16 u2Type;
    u16 u2Len;
    /* parameter */
    u8 ucBand;
    u8 ucAction;
    u8 reserved[2];
    /* report 1 */
    u32 u4FalseCCA;
    u32 u4HdrCRC;
    u32 u4PktSent;
    u32 u4PktRetried;
    u32 u4PktTxfailed;
    u32 u4RxMPDU;
    u32 u4RxFcs;
    /* air time report */
    u32 u4FetchSt;  /* ms */
    u32 u4FetchEd;  /* ms */
    u32 u4ChBusy;  /* us */
    u32 u4ChIdle;  /* us */
    u32 u4TxAirTime;  /* us */
    u32 u4RxAirTime;  /* us */
    u32 u4TimerDur;  /* ms */
    u32 u4FetchCost;  /* us */
    s32 TimerDrift;  /* ms */
    s16 u2SamplePoints;  /* ms */
    s8 ucTxThres;  /* ms */
    s8 ucRxThres;  /* ms */
};

/* admission control related define */
/* support set operations */
#define ADMIN_CTRL_SET_MODE      (0)
#define ADMIN_CTRL_SET_BASE      (1)
#define ADMIN_CTRL_SET_TBL1      (2)
#define ADMIN_CTRL_SET_TBL2      (3)
#define ADMIN_CTRL_SET_TBL3      (4)
#define ADMIN_CTRL_SET_METHOD        (5)

/* admission ctrl mode */
#define ADMIN_CTRL_MODE_DIS      (0)
#define ADMIN_CTRL_MODE_AUTO         (1)
#define ADMIN_CTRL_MODE_MAN      (2)
#define ADMIN_CTRL_MODE_RESET        (3)
/* default value */
#define ADMIN_CTRL_RATE_CODE_NUM     (8)  /* AUTO_RATE_NUM */
#define ADMIN_CTRL_TBL_ENTRY_NUM     (6)
#define ADMIN_CTRL_MAX_PERCENTAGE    (100)
/* status define */
#define BT_PROF_A2DP_SRC         0x02
#define BT_PROF_LINK_CONNECTED       0x04
#define BT_PROF_A2DP_SINK        0x400
#define ADMIN_LINK_2G            BIT(0)
#define ADMIN_LINK_OTHER         BIT(1)
#define ADMIN_ENABLED            BIT(2)
#define ADMIN_PER_PKT_ENABLED        BIT(3)
#define ADMIN_METHOD1_ENABLED        BIT(4)
#define ADMIN_METHOD2_ENABLED        BIT(5)

struct ADMIN_CTRL_PARAM {
    /* bt info updated to admin ctrl */
    u32 u4CoexMode;
    /* Ctrl mode */
    u16 u2eMode;
    /* Admin ctrl related */
    u16 u2AdminCtrlBase;
    u16 u2CurAdminTime;
    u16 u2ForceAdminTime;
    /* Rate related */
    u16 au2RateCode[ADMIN_CTRL_RATE_CODE_NUM];
    /* admin ctrl % tbl */
    u8 aucAdminTbl1[ADMIN_CTRL_TBL_ENTRY_NUM];
    u8 aucAdminTbl2[ADMIN_CTRL_TBL_ENTRY_NUM];
    u8 aucAdminTbl3[ADMIN_CTRL_TBL_ENTRY_NUM];
    u8 ucAdminThermalLimit;
    u8 ucLastChosenTbl;
    u8 ucAdminStatus;
    u8 reserved[3];
};

struct CMD_ADMIN_CTRL_CONFIG {
    u16 u2Type;
    u16 u2Len;
    /* parameter */
    u16 u2Action;
    u8 reserved[2];
    /* content */
    struct ADMIN_CTRL_PARAM content;
};

typedef struct _CMD_ADV_CONFIG_HEADER {
    u16 u2Type;
    u16 u2Len;
} CMD_ADV_CONFIG_HEADER_T, *P_CMD_ADV_CONFIG_HEADER_T;

/* noise histogram related */
enum _ENUM_NOISE_HISTOGRAM_ACTION_T {
    CMD_NOISE_HISTOGRAM_ENABLE = 1,
    CMD_NOISE_HISTOGRAM_DISABLE,
    CMD_NOISE_HISTOGRAM_RESET,
    CMD_NOISE_HISTOGRAM_GET,
#if CFG_IPI_2CHAIN_SUPPORT
    CMD_NOISE_HISTOGRAM_GET2
#endif
};
struct CMD_NOISE_HISTOGRAM_REPORT {
    u16 u2Type;
    u16 u2Len;
    /* parameter */
    u8 ucAction;
    u8 reserved[3];
    /* IPI_report */
    u32 u4IPI0;  /* Power <= -92 */
    u32 u4IPI1;  /* -92 < Power <= -89 */
    u32 u4IPI2;  /* -89 < Power <= -86 */
    u32 u4IPI3;  /* -86 < Power <= -83 */
    u32 u4IPI4;  /* -83 < Power <= -80 */
    u32 u4IPI5;  /* -80 < Power <= -75 */
    u32 u4IPI6;  /* -75 < Power <= -70 */
    u32 u4IPI7;  /* -70 < Power <= -65 */
    u32 u4IPI8;  /* -65 < Power <= -60 */
    u32 u4IPI9;  /* -60 < Power <= -55 */
    u32 u4IPI10;  /* -55 < Power  */
};
#endif
typedef struct _CMD_SET_DEVICE_MODE_T {
    u16 u2ChipID;
    u16 u2Mode;
} CMD_SET_DEVICE_MODE_T, *P_CMD_SET_DEVICE_MODE_T;

#if CFG_SUPPORT_RDD_TEST_MODE
typedef struct _CMD_RDD_CH_T {
    u8 ucRddTestMode;
    u8 ucRddShutCh;
    u8 ucRddStartCh;
    u8 ucRddStopCh;
    u8 ucRddDfs;
    u8 ucReserved;
    u8 ucReserved1;
    u8 ucReserved2;
} CMD_RDD_CH_T, *P_CMD_RDD_CH_T;

typedef struct _EVENT_RDD_STATUS_T {
    u8 ucRddStatus;
    u8 aucReserved[3];
} EVENT_RDD_STATUS_T, *P_EVENT_RDD_STATUS_T;
#endif

typedef struct _EVENT_ICAP_STATUS_T {
    u8 ucRddStatus;
    u8 aucReserved[3];
    u32 u4StartAddress;
    u32 u4IcapSieze;
#if CFG_SUPPORT_QA_TOOL
    u32 u4IcapContent;
#endif
} EVENT_ICAP_STATUS_T, *P_EVENT_ICAP_STATUS_T;

#if CFG_SUPPORT_QA_TOOL
typedef struct _ADC_BUS_FMT_T {
    u32 u4Dcoc0Q : 14;  /* [13:0] */
    u32 u4Dcoc0I : 14;  /* [27:14] */
    u32 u4DbgData1 : 4;  /* [31:28] */

    u32 u4Dcoc1Q : 14;  /* [45:32] */
    u32 u4Dcoc1I : 14;  /* [46:59] */
    u32 u4DbgData2 : 4;  /* [63:60] */

    u32 u4DbgData3;  /* [95:64] */
} ADC_BUS_FMT_T, *P_ADC_BUS_FMT_T;

typedef struct _IQC_BUS_FMT_T {
    s32 u4Iqc0Q : 14;  /* [13:0] */
    s32 u4Iqc0I : 14;  /* [27:14] */
    s32 u4Na1 : 4;  /* [31:28] */

    s32 u4Iqc1Q : 14;  /* [45:32] */
    s32 u4Iqc1I : 14;  /* [59:46] */
    s32 u4Na2 : 4;  /* [63:60] */

    s32 u4Na3;  /* [95:64] */
} IQC_BUS_FMT_T, *P_IQC_BUS_FMT_T;

typedef struct _IQC_160_BUS_FMT_T {
    s32 u4Iqc0Q1 : 12;  /* [11:0] */
    s32 u4Iqc0I1 : 12;  /* [23:12] */
    u32 u4Iqc0Q0P1 : 8;  /* [31:24] */

    s32 u4Iqc0Q0P2 : 4;  /* [35:32] */
    s32 u4Iqc0I0 : 12;  /* [47:36] */
    s32 u4Iqc1Q1 : 12;  /* [59:48] */
    u32 u4Iqc1I1P1 : 4;  /* [63:60] */

    s32 u4Iqc1I1P2 : 8;  /* [71:64] */
    s32 u4Iqc1Q0 : 12;  /* [83:72] */
    s32 u4Iqc1I0 : 12;  /* [95:84] */
} IQC_160_BUS_FMT_T, *P_IQC_160_BUS_FMT_T;

typedef struct _SPECTRUM_BUS_FMT_T {
    s32 u4DcocQ : 12;  /* [11:0] */
    s32 u4DcocI : 12;  /* [23:12] */
    s32 u4LpfGainIdx : 4;  /* [27:24] */
    s32 u4LnaGainIdx : 2;  /* [29:28] */
    s32 u4AssertData : 2;  /* [31:30] */
} SPECTRUM_BUS_FMT_T, *P_SPECTRUM_BUS_FMT_T;

typedef struct _PACKED_ADC_BUS_FMT_T {
    u32 u4AdcQ0T2 : 4;  /* [19:16] */
    u32 u4AdcQ0T1 : 4;  /* [11:8] */
    u32 u4AdcQ0T0 : 4;  /* [3:0] */

    u32 u4AdcI0T2 : 4;  /* [23:20] */
    u32 u4AdcI0T1 : 4;  /* [15:12] */
    u32 u4AdcI0T0 : 4;  /* [7:4] */

    u32 u4AdcQ0T5 : 4;  /* [43:40] */
    u32 u4AdcQ0T4 : 4;  /* [35:32] */
    u32 u4AdcQ0T3 : 4;  /* [27:24] */

    u32 u4AdcI0T5 : 4;  /* [47:44] */
    u32 u4AdcI0T4 : 4;  /* [39:36] */
    u32 u4AdcI0T3 : 4;  /* [31:28] */

    u32 u4AdcQ1T2 : 4;  /* [19:16] */
    u32 u4AdcQ1T1 : 4;  /* [11:8] */
    u32 u4AdcQ1T0 : 4;  /* [3:0] */

    u32 u4AdcI1T2 : 4;  /* [23:20] */
    u32 u4AdcI1T1 : 4;  /* [15:12] */
    u32 u4AdcI1T0 : 4;  /* [7:4] */

    u32 u4AdcQ1T5 : 4;  /* [43:40] */
    u32 u4AdcQ1T4 : 4;  /* [35:32] */
    u32 u4AdcQ1T3 : 4;  /* [27:24] */

    u32 u4AdcI1T5 : 4;  /* [47:44] */
    u32 u4AdcI1T4 : 4;  /* [39:36] */
    u32 u4AdcI1T3 : 4;  /* [31:28] */
} PACKED_ADC_BUS_FMT_T, *P_PACKED_ADC_BUS_FMT_T;

typedef union _ICAP_BUS_FMT {
    ADC_BUS_FMT_T rAdcBusData;  /* 12 bytes */
    IQC_BUS_FMT_T rIqcBusData;  /* 12 bytes */
    IQC_160_BUS_FMT_T rIqc160BusData;  /* 12 bytes */
    SPECTRUM_BUS_FMT_T rSpectrumBusData;  /* 4  bytes */
    PACKED_ADC_BUS_FMT_T rPackedAdcBusData;  /* 12 bytes */
} ICAP_BUS_FMT, *P_ICAP_BUS_FMT;
#endif

typedef struct _CMD_SET_TXPWR_CTRL_T {
    s8 c2GLegacyStaPwrOffset;  /* Unit: 0.5dBm, default: 0 */
    s8 c2GHotspotPwrOffset;
    s8 c2GP2pPwrOffset;
    s8 c2GBowPwrOffset;
    s8 c5GLegacyStaPwrOffset;  /* Unit: 0.5dBm, default: 0 */
    s8 c5GHotspotPwrOffset;
    s8 c5GP2pPwrOffset;
    s8 c5GBowPwrOffset;
    /* TX power policy when concurrence
     *  in the same channel
     *  0: Highest power has priority
     *  1: Lowest power has priority
     */
    u8 ucConcurrencePolicy;
    s8 acReserved1[3];  /* Must be zero */

    /* Power limit by channel for all data rates */
    s8 acTxPwrLimit2G[14];  /* Channel 1~14, Unit: 0.5dBm */
    s8 acTxPwrLimit5G[4];  /* UNII 1~4 */
    s8 acReserved2[2];  /* Must be zero */
} CMD_SET_TXPWR_CTRL_T, *P_CMD_SET_TXPWR_CTRL_T;

typedef enum _ENUM_NLO_CIPHER_ALGORITHM {
    NLO_CIPHER_ALGO_NONE          = 0x00,
    NLO_CIPHER_ALGO_WEP40         = 0x01,
    NLO_CIPHER_ALGO_TKIP          = 0x02,
    NLO_CIPHER_ALGO_CCMP          = 0x04,
    NLO_CIPHER_ALGO_WEP104        = 0x05,
    NLO_CIPHER_ALGO_WPA_USE_GROUP = 0x100,
    NLO_CIPHER_ALGO_RSN_USE_GROUP = 0x100,
    NLO_CIPHER_ALGO_WEP           = 0x101,
} ENUM_NLO_CIPHER_ALGORITHM,
*P_ENUM_NLO_CIPHER_ALGORITHM;

typedef enum _ENUM_NLO_AUTH_ALGORITHM {
    NLO_AUTH_ALGO_80211_OPEN       = 1,
    NLO_AUTH_ALGO_80211_SHARED_KEY = 2,
    NLO_AUTH_ALGO_WPA              = 3,
    NLO_AUTH_ALGO_WPA_PSK          = 4,
    NLO_AUTH_ALGO_WPA_NONE         = 5,
    NLO_AUTH_ALGO_RSNA             = 6,
    NLO_AUTH_ALGO_RSNA_PSK         = 7,
} ENUM_NLO_AUTH_ALGORITHM,
*P_ENUM_NLO_AUTH_ALGORITHM;

typedef struct _NLO_NETWORK {
    u8 ucNumChannelHint[4];
    u8 ucSSIDLength;
    u8 ucCipherAlgo;
    u16 u2AuthAlgo;
    u8 aucSSID[32];
} NLO_NETWORK, *P_NLO_NETWORK;

typedef struct _CMD_NLO_REQ {
    u8 ucSeqNum;
    u8 ucBssIndex;
    u8 fgStopAfterIndication;
    u8 ucFastScanIteration;
    u16 u2FastScanPeriod;
    u16 u2SlowScanPeriod;
    u8 ucEntryNum;
    u8 ucFlag;  /* BIT(0) Check cipher */
    u16 u2IELen;
    NLO_NETWORK arNetworkList[16];
    u8 aucIE[0];
} CMD_NLO_REQ, *P_CMD_NLO_REQ;

typedef struct _CMD_NLO_CANCEL_T {
    u8 ucSeqNum;
    u8 ucBssIndex;
    u8 aucReserved[2];
} CMD_NLO_CANCEL, *P_CMD_NLO_CANCEL;

typedef struct _EVENT_NLO_DONE_T {
    u8 ucSeqNum;
    u8 ucStatus;
    u8 aucReserved[2];
} EVENT_NLO_DONE_T, *P_EVENT_NLO_DONE_T;

typedef struct _CMD_HIF_CTRL_T {
    u8 ucHifType;
    u8 ucHifDirection;
    u8 ucHifStop;
    u8 aucReserved1;
    u8 aucReserved2[32];
} CMD_HIF_CTRL_T, *P_CMD_HIF_CTRL_T;

typedef enum _ENUM_HIF_TYPE {
    ENUM_HIF_TYPE_SDIO = 0x00,
    ENUM_HIF_TYPE_USB  = 0x01,
    ENUM_HIF_TYPE_PCIE = 0x02,
    ENUM_HIF_TYPE_GPIO = 0x03,
} ENUM_HIF_TYPE,
*P_ENUM_HIF_TYPE;

typedef enum _ENUM_HIF_DIRECTION {
    ENUM_HIF_TX  = 0x01,
    ENUM_HIF_RX  = 0x02,
    ENUM_HIF_TRX = 0x03,
} ENUM_HIF_DIRECTION,
*P_ENUM_HIF_DIRECTION;

typedef enum _ENUM_HIF_TRAFFIC_STATUS {
    ENUM_HIF_TRAFFIC_BUSY    = 0x01,
    ENUM_HIF_TRAFFIC_IDLE    = 0x02,
    ENUM_HIF_TRAFFIC_INVALID = 0x3,
} ENUM_HIF_TRAFFIC_STATUS,
*P_ENUM_HIF_TRAFFIC_STATUS;

typedef struct _EVENT_HIF_CTRL_T {
    u8 ucHifType;
    u8 ucHifTxTrafficStatus;
    u8 ucHifRxTrafficStatus;
    u8 ucReserved1;
    u8 aucReserved2[32];
} EVENT_HIF_CTRL_T, *P_EVENT_HIF_CTRL_T;

#if CFG_SUPPORT_BUILD_DATE_CODE
typedef struct _CMD_GET_BUILD_DATE_CODE {
    u8 aucReserved[4];
} CMD_GET_BUILD_DATE_CODE, *P_CMD_GET_BUILD_DATE_CODE;

typedef struct _EVENT_BUILD_DATE_CODE {
    u8 aucDateCode[16];
} EVENT_BUILD_DATE_CODE, *P_EVENT_BUILD_DATE_CODE;
#endif

typedef struct _CMD_GET_STA_STATISTICS_T {
    u8 ucIndex;
    u8 ucFlags;
    u8 ucReadClear;
    u8 ucLlsReadClear;
    u8 aucMacAddr[MAC_ADDR_LEN];
    u8 ucResetCounter;
    u8 aucReserved1[1];
    u8 aucReserved2[16];
} CMD_GET_STA_STATISTICS_T, *P_CMD_GET_STA_STATISTICS_T;

/* per access category statistics */
typedef struct _WIFI_WMM_AC_STAT_GET_FROM_FW_T {
    u32 u4TxFailMsdu;
    u32 u4TxRetryMsdu;
} WIFI_WMM_AC_STAT_GET_FROM_FW_T, *P_WIFI_WMM_AC_STAT_GET_FROM_FW_T;

/* CFG_SUPPORT_WFD */
typedef struct _EVENT_STA_STATISTICS_T {
    /* Event header */
    /* u16     u2Length; */
    /* u16     u2Reserved1; */ /* Must be filled with 0x0001 (EVENT Packet)
     */
    /* u8            ucEID; */
    /* u8      ucSeqNum; */
    /* u8            aucReserved2[2]; */

    /* Event Body */
    u8 ucVersion;
    u8 aucReserved1[3];
    u32 u4Flags;  /* Bit0: valid */

    u8 ucStaRecIdx;
    u8 ucNetworkTypeIndex;
    u8 ucWTEntry;
    u8 aucReserved4[1];

    u8 ucMacAddr[MAC_ADDR_LEN];
    u8 ucPer;  /* base: 128 */
    u8 ucRcpi;

    u32 u4PhyMode;  /* SGI BW */
    u16 u2LinkSpeed;  /* unit is 0.5 Mbits */
    u8 ucLinkQuality;
    u8 ucLinkReserved;

    u32 u4TxCount;
    u32 u4TxFailCount;
    u32 u4TxLifeTimeoutCount;
    u32 u4TxDoneAirTime;
    u32 u4TransmitCount;  /* Transmit in the air (wtbl) */
    u32 u4TransmitFailCount;  /* Transmit without ack/ba in the air (wtbl) */

    WIFI_WMM_AC_STAT_GET_FROM_FW_T
        arLinkStatistics[AC_NUM];  /*link layer statistics */

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
    u8 fgIsForceTxStream;
    u8 fgIsForceSeOff;
    u8 aucReserved[21];
} EVENT_STA_STATISTICS_T, *P_EVENT_STA_STATISTICS_T;

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
typedef struct _EVENT_LTE_SAFE_CHN_T {
    u8 ucVersion;
    u8 aucReserved[3];
    u32 u4Flags;  /* Bit0: valid */
    LTE_SAFE_CHN_INFO_T rLteSafeChn;
} EVENT_LTE_SAFE_CHN_T, *P_EVENT_LTE_SAFE_CHN_T;
#endif

#if CFG_SUPPORT_SNIFFER
typedef struct _CMD_MONITOR_SET_INFO_T {
    u8 ucEnable;
    u8 ucBand;
    u8 ucPriChannel;
    u8 ucSco;
    u8 ucChannelWidth;
    u8 ucChannelS1;
    u8 ucChannelS2;
    u8 aucResv[9];
} CMD_MONITOR_SET_INFO_T, *P_CMD_MONITOR_SET_INFO_T;
#endif

typedef struct _CMD_STATS_LOG_T {
    u32 u4DurationInMs;
    u8 aucReserved[32];
} CMD_STATS_LOG_T, *P_CMD_STATS_LOG_T;

typedef struct _EVENT_WIFI_RDD_TEST_T {
    u32 u4FuncIndex;
    u32 u4FuncLength;
    u32 u4Prefix;
    u32 u4Count;
    u32 u4SubBandRssi0;
    u32 u4SubBandRssi1;
    u8 ucRddIdx;
    u8 aucReserve[3];
    u8 aucBuffer[0];
} EVENT_WIFI_RDD_TEST_T, *P_EVENT_WIFI_RDD_TEST_T;

#if CFG_SUPPORT_MSP
/* EVENT_ID_WLAN_INFO */
typedef struct _EVENT_WLAN_INFO {
    PARAM_TX_CONFIG_T rWtblTxConfig;
    PARAM_SEC_CONFIG_T rWtblSecConfig;
    PARAM_KEY_CONFIG_T rWtblKeyConfig;
    PARAM_PEER_RATE_INFO_T rWtblRateInfo;
    PARAM_PEER_BA_CONFIG_T rWtblBaConfig;
    PARAM_PEER_CAP_T rWtblPeerCap;
    PARAM_PEER_RX_COUNTER_ALL_T rWtblRxCounter;
    PARAM_PEER_TX_COUNTER_ALL_T rWtblTxCounter;
} EVENT_WLAN_INFO, *P_EVENT_WLAN_INFO;

/* EVENT_ID_MIB_INFO */
typedef struct _EVENT_MIB_INFO {
    HW_MIB_COUNTER_T rHwMibCnt;
    HW_MIB2_COUNTER_T rHwMib2Cnt;
    HW_TX_AMPDU_METRICS_T rHwTxAmpduMts;
} EVENT_MIB_INFO, *P_EVENT_MIB_INFO;
#endif

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
struct EVENT_TX_MCS_INFO {
    u16 au2TxRateCode[MCS_INFO_SAMPLE_CNT];
    u8 aucTxRatePer[MCS_INFO_SAMPLE_CNT];
    u8 aucReserved[2];
};
#endif

/*#if (CFG_EEPROM_PAGE_ACCESS == 1)*/
typedef struct _EVENT_ACCESS_EFUSE {
    u32 u4Address;
    u32 u4Valid;
    u8 aucData[16];
} EVENT_ACCESS_EFUSE, *P_EVENT_ACCESS_EFUSE;

typedef struct _EXT_EVENT_EFUSE_FREE_BLOCK_T {
    u16 u2FreeBlockNum;
    u8 aucReserved[2];
} EVENT_EFUSE_FREE_BLOCK_T, *P_EVENT_EFUSE_FREE_BLOCK_T;

typedef struct _EXT_EVENT_GET_TX_POWER_T {
    u8 ucTxPwrType;
    u8 ucEfuseAddr;
    u8 ucTx0TargetPower;
    u8 ucDbdcIdx;
} EVENT_GET_TX_POWER_T, *P_EVENT_GET_TX_POWER_T;

#if CFG_STR_DHCP_RENEW_OFFLOAD
typedef struct _CMD_DHCP_OFFLOAD_SETTING_T {
    u32 u4RenewIntv;  /* DHCP renew offload interval configured by
                       * upper-layer */
    u8 aucDhcpServerIpAddr[4];
    u8 ucBssIndex;
    u8 ucEnableOffload;
    u8 ucSuspend;
    u8 ucReserved[1];
} CMD_DHCP_OFFLOAD_SETTING_T, *P_CMD_DHCP_OFFLOAD_SETTING_T;
#endif

typedef struct _CMD_SUSPEND_MODE_SETTING_T {
    u8 ucBssIndex;
    u8 ucEnableSuspendMode;
    u8 ucMdtim;  /* LP parameter */
    u8 ucReserved1[1];
    u8 ucReserved2[64];
} CMD_SUSPEND_MODE_SETTING_T, *P_CMD_SUSPEND_MODE_SETTING_T;

typedef struct _EVENT_UPDATE_COEX_PHYRATE_T {
    u8 ucVersion;
    u8 aucReserved1[3];  /* 4 byte alignment */
    u32 u4Flags;
    u32 au4PhyRateLimit[HW_BSSID_NUM + 1];
} EVENT_UPDATE_COEX_PHYRATE_T, *P_EVENT_UPDATE_COEX_PHYRATE_T;

enum CSI_CONTROL_MODE_T {
    CSI_CONTROL_MODE_STOP,
    CSI_CONTROL_MODE_START,
    CSI_CONTROL_MODE_NUM
};

enum ENUM_RTT_ROLE_T {
    RTT_ROLE_RECEIVING = 0,
    RTT_ROLE_SENDING,
    RTT_ROLE_NUM
};

enum ENUM_RTT_FRAME_TYPE_T {
    RTT_FRAME_TYPE_BEACON,
    RTT_FRAME_TYPE_QOS_DATA,
    RTT_FRAME_TYPE_NUM
};

struct CMD_CSI_CONTROL_T {
    u8 ucMode;
    u8 ucBand;
    u8 ucWf;
    u8 ucRole;
    u8 ucFrameType;
    u8 ucFrameTypeIndex;
    u8 ucReserved[2];
};

#ifdef CFG_DUMP_TXPOWR_TABLE
struct CMD_GET_TXPWR_TBL {
    u8 ucDbdcIdx;
    u8 aucReserved[3];
};

struct EVENT_GET_TXPWR_TBL {
    u8 ucCenterCh;
    u8 ucFeLoss;
    u8 aucReserved[2];
    struct POWER_LIMIT tx_pwr_tbl[TXPWR_TBL_NUM];
};
#endif

struct EVENT_GET_TEMPERATURE {
    u8 ucTemperaute;
    u8 aucReserved[3];
};
/*#endif*/

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

void nicCmdEventQueryMcrRead(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo,
                             IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen);
/* Nic cmd/event for Coex related */
void nicCmdEventQueryCoexIso(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo,
                             IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen);

#if CFG_SUPPORT_QA_TOOL
void nicCmdEventQueryRxStatistics(IN P_ADAPTER_T prAdapter,
                                  IN P_CMD_INFO_T prCmdInfo,
                                  IN u8 *pucEventBuf,
                                  IN u32 u4EventBufLen);

u32 TsfRawData2IqFmt(P_EVENT_DUMP_MEM_T prEventDumpMem);

s32 GetIQData(s32 **prIQAry, u32 *prDataLen, u32 u4IQ, u32 u4GetWf1);

#if CFG_SUPPORT_TX_BF
void nicCmdEventPfmuDataRead(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo,
                             IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen);

void nicCmdEventPfmuTagRead(IN P_ADAPTER_T prAdapter,
                            IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf,
                            IN u32 u4EventBufLen);
#endif
#if CFG_SUPPORT_MU_MIMO
void nicCmdEventGetQd(IN P_ADAPTER_T prAdapter,
                      IN P_CMD_INFO_T prCmdInfo,
                      IN u8 *pucEventBuf,
                      IN u32 u4EventBufLen);
void nicCmdEventGetCalcLq(IN P_ADAPTER_T prAdapter,
                          IN P_CMD_INFO_T prCmdInfo,
                          IN u8 *pucEventBuf,
                          IN u32 u4EventBufLen);
void nicCmdEventGetCalcInitMcs(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo,
                               IN u8 *pucEventBuf,
                               IN u32 u4EventBufLen);
#endif
#endif

void nicEventQueryMemDump(IN P_ADAPTER_T prAdapter,
                          IN u8 *pucEventBuf,
                          IN u32 u4EventBufLen);

void nicCmdEventQueryMemDump(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo,
                             IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen);

void nicCmdEventQuerySwCtrlRead(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo,
                                IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen);

void nicCmdEventQueryChipConfig(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo,
                                IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen);

void nicCmdEventQueryRfTestATInfo(IN P_ADAPTER_T prAdapter,
                                  IN P_CMD_INFO_T prCmdInfo,
                                  IN u8 *pucEventBuf,
                                  IN u32 u4EventBufLen);

void nicCmdEventSetCommon(IN P_ADAPTER_T prAdapter,
                          IN P_CMD_INFO_T prCmdInfo,
                          IN u8 *pucEventBuf,
                          IN u32 u4EventBufLen);

void nicCmdEventSetDisassociate(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo,
                                IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen);

void nicCmdEventSetIpAddress(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo,
                             IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen);

void nicCmdEventQueryLinkQuality(IN P_ADAPTER_T prAdapter,
                                 IN P_CMD_INFO_T prCmdInfo,
                                 IN u8 *pucEventBuf,
                                 IN u32 u4EventBufLen);

void nicCmdEventQueryLinkSpeed(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo,
                               IN u8 *pucEventBuf,
                               IN u32 u4EventBufLen);

void nicCmdEventQueryStatistics(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo,
                                IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen);

void nicCmdEventEnterRfTest(IN P_ADAPTER_T prAdapter,
                            IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf,
                            IN u32 u4EventBufLen);

void nicCmdEventLeaveRfTest(IN P_ADAPTER_T prAdapter,
                            IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf,
                            IN u32 u4EventBufLen);

void nicCmdEventQueryMcastAddr(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo,
                               IN u8 *pucEventBuf,
                               IN u32 u4EventBufLen);

void nicCmdEventQueryEepromRead(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo,
                                IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen);

void nicCmdEventSetMediaStreamMode(IN P_ADAPTER_T prAdapter,
                                   IN P_CMD_INFO_T prCmdInfo,
                                   IN u8 *pucEventBuf,
                                   IN u32 u4EventBufLen);

void nicCmdEventSetStopSchedScan(IN P_ADAPTER_T prAdapter,
                                 IN P_CMD_INFO_T prCmdInfo,
                                 IN u8 *pucEventBuf,
                                 IN u32 u4EventBufLen);

/* Statistics responder */
void nicCmdEventQueryXmitOk(IN P_ADAPTER_T prAdapter,
                            IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf,
                            IN u32 u4EventBufLen);

void nicCmdEventQueryRecvOk(IN P_ADAPTER_T prAdapter,
                            IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf,
                            IN u32 u4EventBufLen);

void nicCmdEventQueryXmitError(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo,
                               IN u8 *pucEventBuf,
                               IN u32 u4EventBufLen);

void nicCmdEventQueryRecvError(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo,
                               IN u8 *pucEventBuf,
                               IN u32 u4EventBufLen);

void nicCmdEventQueryRecvNoBuffer(IN P_ADAPTER_T prAdapter,
                                  IN P_CMD_INFO_T prCmdInfo,
                                  IN u8 *pucEventBuf,
                                  IN u32 u4EventBufLen);

void nicCmdEventQueryRecvCrcError(IN P_ADAPTER_T prAdapter,
                                  IN P_CMD_INFO_T prCmdInfo,
                                  IN u8 *pucEventBuf,
                                  IN u32 u4EventBufLen);

void nicCmdEventQueryRecvErrorAlignment(IN P_ADAPTER_T prAdapter,
                                        IN P_CMD_INFO_T prCmdInfo,
                                        IN u8 *pucEventBuf,
                                        IN u32 u4EventBufLen);

void nicCmdEventQueryXmitOneCollision(IN P_ADAPTER_T prAdapter,
                                      IN P_CMD_INFO_T prCmdInfo,
                                      IN u8 *pucEventBuf,
                                      IN u32 u4EventBufLen);

void nicCmdEventQueryXmitMoreCollisions(IN P_ADAPTER_T prAdapter,
                                        IN P_CMD_INFO_T prCmdInfo,
                                        IN u8 *pucEventBuf,
                                        IN u32 u4EventBufLen);

void nicCmdEventQueryXmitMaxCollisions(IN P_ADAPTER_T prAdapter,
                                       IN P_CMD_INFO_T prCmdInfo,
                                       IN u8 *pucEventBuf,
                                       IN u32 u4EventBufLen);

/* for timeout check */
void nicOidCmdTimeoutCommon(IN P_ADAPTER_T prAdapter,
                            IN P_CMD_INFO_T prCmdInfo);

void nicCmdTimeoutCommon(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo);

void nicOidCmdEnterRFTestTimeout(IN P_ADAPTER_T prAdapter,
                                 IN P_CMD_INFO_T prCmdInfo);

#if CFG_SUPPORT_BUILD_DATE_CODE
void nicCmdEventBuildDateCode(IN P_ADAPTER_T prAdapter,
                              IN P_CMD_INFO_T prCmdInfo,
                              IN u8 *pucEventBuf,
                              IN u32 u4EventBufLen);
#endif

void nicCmdEventQueryStaStatistics(IN P_ADAPTER_T prAdapter,
                                   IN P_CMD_INFO_T prCmdInfo,
                                   IN u8 *pucEventBuf,
                                   IN u32 u4EventBufLen);

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
/* 4 Auto Channel Selection */
void nicCmdEventQueryLteSafeChn(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo,
                                IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen);
#endif

#if CFG_SUPPORT_BATCH_SCAN
void nicCmdEventBatchScanResult(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo,
                                IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen);
#endif

#if CFG_SUPPORT_ADVANCE_CONTROL
void nicCmdEventQueryAdvCtrl(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo,
                             IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen);
#endif

void nicEventRddPulseDump(IN P_ADAPTER_T prAdapter,
                          IN u8 *pucEventBuf,
                          IN u32 u4EventBufLen);

void nicCmdEventQueryWlanInfo(IN P_ADAPTER_T prAdapter,
                              IN P_CMD_INFO_T prCmdInfo,
                              IN u8 *pucEventBuf,
                              IN u32 u4EventBufLen);

void nicCmdEventQueryMibInfo(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo,
                             IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen);

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
void nicCmdEventTxMcsInfo(IN P_ADAPTER_T prAdapter,
                          IN P_CMD_INFO_T prCmdInfo,
                          IN u8 *pucEventBuf,
                          IN u32 u4EventBufLen);
#endif

void nicCmdEventQueryNicCapabilityV2(IN P_ADAPTER_T prAdapter,
                                     IN u8 *pucEventBuf,
                                     IN u32 u4EventBufLen);

WLAN_STATUS nicCmdEventQueryNicTxResource(IN P_ADAPTER_T prAdapter,
                                          IN u8 *pucEventBuf,
                                          IN u32 u4EventBufLen);

WLAN_STATUS nicCmdEventQueryNicEfuseAddr(IN P_ADAPTER_T prAdapter,
                                         IN u8 *pucEventBuf,
                                         IN u32 u4EventBufLen);

WLAN_STATUS nicCmdEventQueryEfuseOffset(IN P_ADAPTER_T prAdapter,
                                        IN u8 *pucEventBuf,
                                        IN u32 u4EventBufLen);

WLAN_STATUS nicCmdEventQueryNicCoexFeature(IN P_ADAPTER_T prAdapter,
                                           IN u8 *pucEventBuf,
                                           IN u32 u4EventBufLen);

#if CFG_TCP_IP_CHKSUM_OFFLOAD
WLAN_STATUS nicCmdEventQueryNicCsumOffload(IN P_ADAPTER_T prAdapter,
                                           IN u8 *pucEventBuf,
                                           IN u32 u4EventBufLen);
#endif

void nicEventLinkQuality(IN P_ADAPTER_T prAdapter,
                         IN P_WIFI_EVENT_T prEvent,
                         IN u32 u4EventBufLen);
void nicEventLayer0ExtMagic(IN P_ADAPTER_T prAdapter,
                            IN P_WIFI_EVENT_T prEvent,
                            IN u32 u4EventBufLen);
void nicEventMicErrorInfo(IN P_ADAPTER_T prAdapter,
                          IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen);
void nicEventScanDone(IN P_ADAPTER_T prAdapter,
                      IN P_WIFI_EVENT_T prEvent,
                      IN u32 u4EventBufLen);
void nicEventNloDone(IN P_ADAPTER_T prAdapter,
                     IN P_WIFI_EVENT_T prEvent,
                     IN u32 u4EventBufLen);
void nicEventSleepyNotify(IN P_ADAPTER_T prAdapter,
                          IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen);
void nicEventBtOverWifi(IN P_ADAPTER_T prAdapter,
                        IN P_WIFI_EVENT_T prEvent,
                        IN u32 u4EventBufLen);
void nicEventStatistics(IN P_ADAPTER_T prAdapter,
                        IN P_WIFI_EVENT_T prEvent,
                        IN u32 u4EventBufLen);
void nicEventWlanInfo(IN P_ADAPTER_T prAdapter,
                      IN P_WIFI_EVENT_T prEvent,
                      IN u32 u4EventBufLen);
void nicEventMibInfo(IN P_ADAPTER_T prAdapter,
                     IN P_WIFI_EVENT_T prEvent,
                     IN u32 u4EventBufLen);
#if CFG_SUPPORT_LAST_SEC_MCS_INFO
void nicEventTxMcsInfo(IN P_ADAPTER_T prAdapter,
                       IN P_WIFI_EVENT_T prEvent,
                       IN u32 u4EventBufLen);
#endif
void nicEventBeaconTimeout(IN P_ADAPTER_T prAdapter,
                           IN P_WIFI_EVENT_T prEvent,
                           IN u32 u4EventBufLen);
void nicEventUpdateNoaParams(IN P_ADAPTER_T prAdapter,
                             IN P_WIFI_EVENT_T prEvent,
                             IN u32 u4EventBufLen);
void nicEventStaAgingTimeout(IN P_ADAPTER_T prAdapter,
                             IN P_WIFI_EVENT_T prEvent,
                             IN u32 u4EventBufLen);
void nicEventApObssStatus(IN P_ADAPTER_T prAdapter,
                          IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen);
void nicEventRoamingStatus(IN P_ADAPTER_T prAdapter,
                           IN P_WIFI_EVENT_T prEvent,
                           IN u32 u4EventBufLen);
void nicEventSendDeauth(IN P_ADAPTER_T prAdapter,
                        IN P_WIFI_EVENT_T prEvent,
                        IN u32 u4EventBufLen);
void nicEventUpdateRddStatus(IN P_ADAPTER_T prAdapter,
                             IN P_WIFI_EVENT_T prEvent,
                             IN u32 u4EventBufLen);
void nicEventUpdateBwcsStatus(IN P_ADAPTER_T prAdapter,
                              IN P_WIFI_EVENT_T prEvent,
                              IN u32 u4EventBufLen);
void nicEventUpdateBcmDebug(IN P_ADAPTER_T prAdapter,
                            IN P_WIFI_EVENT_T prEvent,
                            IN u32 u4EventBufLen);
void nicEventAddPkeyDone(IN P_ADAPTER_T prAdapter,
                         IN P_WIFI_EVENT_T prEvent,
                         IN u32 u4EventBufLen);
void nicEventIcapDone(IN P_ADAPTER_T prAdapter,
                      IN P_WIFI_EVENT_T prEvent,
                      IN u32 u4EventBufLen);
void nicEventDebugMsg(IN P_ADAPTER_T prAdapter,
                      IN P_WIFI_EVENT_T prEvent,
                      IN u32 u4EventBufLen);
void nicEventTdls(IN P_ADAPTER_T prAdapter,
                  IN P_WIFI_EVENT_T prEvent,
                  IN u32 u4EventBufLen);
void nicEventDumpMem(IN P_ADAPTER_T prAdapter,
                     IN P_WIFI_EVENT_T prEvent,
                     IN u32 u4EventBufLen);
void nicEventAssertDump(IN P_ADAPTER_T prAdapter,
                        IN P_WIFI_EVENT_T prEvent,
                        IN u32 u4EventBufLen);
void nicEventRddSendPulse(IN P_ADAPTER_T prAdapter,
                          IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen);
void nicEventUpdateCoexPhyrate(IN P_ADAPTER_T prAdapter,
                               IN P_WIFI_EVENT_T prEvent,
                               IN u32 u4EventBufLen);
#ifdef CFG_SUPPORT_ANT_DIV
void nicCmdEventAntDiv(IN P_ADAPTER_T prAdapter,
                       IN P_CMD_INFO_T prCmdInfo,
                       IN u8 *pucEventBuf,
                       IN u32 u4EventBufLen);
#endif

#if (CFG_WOW_SUPPORT == 1)
void nicEventWakeUpReason(IN P_ADAPTER_T prAdapter,
                          IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen);
#endif
void nicEventCSIData(IN P_ADAPTER_T prAdapter,
                     IN P_WIFI_EVENT_T prEvent,
                     IN u32 u4EventBufLen);

#if CFG_SUPPORT_REPLAY_DETECTION
void nicCmdEventSetAddKey(IN P_ADAPTER_T prAdapter,
                          IN P_CMD_INFO_T prCmdInfo,
                          IN u8 *pucEventBuf,
                          IN u32 u4EventBufLen);
void nicOidCmdTimeoutSetAddKey(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo);

void nicEventGetGtkDataSync(IN P_ADAPTER_T prAdapter,
                            IN P_WIFI_EVENT_T prEvent,
                            IN u32 u4EventBufLen);
#endif

#ifdef CFG_DUMP_TXPOWR_TABLE
void nicCmdEventGetTxPwrTbl(IN P_ADAPTER_T prAdapter,
                            IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf,
                            IN u32 u4EventBufLen);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
