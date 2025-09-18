/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_wext_priv.h
 *    \brief  This file includes private ioctl support.
 */

#ifndef _GL_WEXT_PRIV_H
#define _GL_WEXT_PRIV_H
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/* If it is set to 1, iwpriv will support register read/write */
#define CFG_SUPPORT_PRIV_MCR_RW         1

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/* New wireless extensions API - SET/GET convention (even ioctl numbers are
 * root only)
 */
#define IOCTL_SET_INT               (SIOCIWFIRSTPRIV + 0)
#define IOCTL_GET_INT               (SIOCIWFIRSTPRIV + 1)

#define IOCTL_SET_ADDRESS           (SIOCIWFIRSTPRIV + 2)
#define IOCTL_GET_ADDRESS           (SIOCIWFIRSTPRIV + 3)
#define IOCTL_SET_STR               (SIOCIWFIRSTPRIV + 4)
#define IOCTL_GET_STR               (SIOCIWFIRSTPRIV + 5)
#define IOCTL_SET_KEY               (SIOCIWFIRSTPRIV + 6)
#define IOCTL_GET_KEY               (SIOCIWFIRSTPRIV + 7)
#define IOCTL_SET_STRUCT            (SIOCIWFIRSTPRIV + 8)
#define IOCTL_GET_STRUCT            (SIOCIWFIRSTPRIV + 9)
#define IOCTL_SET_STRUCT_FOR_EM         (SIOCIWFIRSTPRIV + 11)
#define IOCTL_SET_INTS              (SIOCIWFIRSTPRIV + 12)
#define IOCTL_GET_INTS              (SIOCIWFIRSTPRIV + 13)
#define IOCTL_SET_DRIVER            (SIOCIWFIRSTPRIV + 14)
#define IOCTL_GET_DRIVER            (SIOCIWFIRSTPRIV + 15)

#if CFG_SUPPORT_QA_TOOL
#define IOCTL_QA_TOOL_DAEMON            (SIOCIWFIRSTPRIV + 16)
#define IOCTL_IWPRIV_ATE            (SIOCIWFIRSTPRIV + 17)
#endif

#define PRIV_CMD_REG_DOMAIN         0
#define PRIV_CMD_BEACON_PERIOD          1
#define PRIV_CMD_ADHOC_MODE         2

#if CFG_TCP_IP_CHKSUM_OFFLOAD
#define PRIV_CMD_CSUM_OFFLOAD           3
#endif

#define PRIV_CMD_ROAMING            4
#define PRIV_CMD_VOIP_DELAY         5
#define PRIV_CMD_POWER_MODE         6

#define PRIV_CMD_WMM_PS             7
#define PRIV_CMD_BT_COEXIST         8
#define PRIV_GPIO2_MODE             9

#define PRIV_CUSTOM_SET_PTA         10
#define PRIV_CUSTOM_CONTINUOUS_POLL     11
#define PRIV_CUSTOM_SINGLE_ANTENNA      12
#define PRIV_CUSTOM_BWCS_CMD            13
#define PRIV_CUSTOM_DISABLE_BEACON_DETECTION    14  /* later */
#define PRIV_CMD_OID                15
#define PRIV_SEC_MSG_OID            16

#define PRIV_CMD_TEST_MODE          17
#define PRIV_CMD_TEST_CMD           18
#define PRIV_CMD_ACCESS_MCR         19
#define PRIV_CMD_SW_CTRL            20

#define PRIV_SEC_CHECK_OID          21

#define PRIV_CMD_WSC_PROBE_REQ          22

#define PRIV_CMD_P2P_VERSION            23

#define PRIV_CMD_GET_CH_LIST            24

#define PRIV_CMD_SET_TX_POWER           25

#define PRIV_CMD_BAND_CONFIG            26

#define PRIV_CMD_DUMP_MEM           27

#define PRIV_CMD_P2P_MODE           28

#if CFG_SUPPORT_QA_TOOL
#define PRIV_QACMD_SET              29
#endif

#define PRIV_CMD_MET_PROFILING          33

#if CFG_WOW_SUPPORT
#define PRIV_CMD_SET_WOW_ENABLE         34
#define PRIV_CMD_SET_WOW_PAR            35
#endif

/* 802.3 Objects (Ethernet) */
#define OID_802_3_CURRENT_ADDRESS       0x01010102

/* IEEE 802.11 OIDs */
#define OID_802_11_SUPPORTED_RATES      0x0D01020E
#define OID_802_11_CONFIGURATION        0x0D010211

/* PnP and PM OIDs, NDIS default OIDS */
#define OID_PNP_SET_POWER           0xFD010101

#define OID_CUSTOM_OID_INTERFACE_VERSION    0xFFA0C000

/* MT5921 specific OIDs */
#define OID_CUSTOM_BT_COEXIST_CTRL      0xFFA0C580
#define OID_CUSTOM_POWER_MANAGEMENT_PROFILE 0xFFA0C581
#define OID_CUSTOM_PATTERN_CONFIG       0xFFA0C582
#define OID_CUSTOM_BG_SSID_SEARCH_CONFIG    0xFFA0C583
#define OID_CUSTOM_VOIP_SETUP           0xFFA0C584
#define OID_CUSTOM_ADD_TS           0xFFA0C585
#define OID_CUSTOM_DEL_TS           0xFFA0C586
#define OID_CUSTOM_SLT              0xFFA0C587
#define OID_CUSTOM_ROAMING_EN           0xFFA0C588
#define OID_CUSTOM_WMM_PS_TEST          0xFFA0C589
#define OID_CUSTOM_COUNTRY_STRING       0xFFA0C58A
#define OID_CUSTOM_MULTI_DOMAIN_CAPABILITY  0xFFA0C58B
#define OID_CUSTOM_GPIO2_MODE           0xFFA0C58C
#define OID_CUSTOM_CONTINUOUS_POLL      0xFFA0C58D
#define OID_CUSTOM_DISABLE_BEACON_DETECTION 0xFFA0C58E

/* CR1460, WPS privacy bit check disable */
#define OID_CUSTOM_DISABLE_PRIVACY_CHECK    0xFFA0C600

/* Precedent OIDs */
#define OID_CUSTOM_MCR_RW           0xFFA0C801
#define OID_CUSTOM_EEPROM_RW            0xFFA0C803
#define OID_CUSTOM_SW_CTRL          0xFFA0C805
#define OID_CUSTOM_MEM_DUMP         0xFFA0C807

/* RF Test specific OIDs */
#define OID_CUSTOM_TEST_MODE            0xFFA0C901
#define OID_CUSTOM_TEST_RX_STATUS       0xFFA0C903
#define OID_CUSTOM_TEST_TX_STATUS       0xFFA0C905
#define OID_CUSTOM_ABORT_TEST_MODE      0xFFA0C906
#define OID_CUSTOM_MTK_WIFI_TEST        0xFFA0C911
#define OID_CUSTOM_TEST_ICAP_MODE       0xFFA0C913

/* BWCS */
#define OID_CUSTOM_BWCS_CMD         0xFFA0C931
#define OID_CUSTOM_SINGLE_ANTENNA       0xFFA0C932
#define OID_CUSTOM_SET_PTA          0xFFA0C933

/* NVRAM */
#define OID_CUSTOM_MTK_NVRAM_RW         0xFFA0C941
#define OID_CUSTOM_CFG_SRC_TYPE         0xFFA0C942
#define OID_CUSTOM_EEPROM_TYPE          0xFFA0C943

// #define OID_802_11_WAPI_MODE 0xFFA0CA00
// #define OID_802_11_WAPI_ASSOC_INFO 0xFFA0CA01
// #define OID_802_11_SET_WAPI_KEY 0xFFA0CA02

#if CFG_SUPPORT_WPS2
#define OID_802_11_WSC_ASSOC_INFO       0xFFA0CB00
#endif

/* Define magic key of test mode (Don't change it for future compatibity) */
#define PRIV_CMD_TEST_MAGIC_KEY         2011
#define PRIV_CMD_TEST_MAGIC_KEY_ICAP        2013

/* CFG_SUPPORT_ADVANCE_CONTROL */
#define TX_RATE_MODE_CCK            0
#define TX_RATE_MODE_OFDM           1
#define TX_RATE_MODE_HTMIX          2
#define TX_RATE_MODE_HTGF           3
#define TX_RATE_MODE_VHT            4
#define MAX_TX_MODE             5

#if CFG_SUPPORT_ADVANCE_CONTROL
#define CMD_SW_DBGCTL_ADVCTL_SET_ID     0xa1260000
#define CMD_SW_DBGCTL_ADVCTL_GET_ID     0xb1260000
#define CMD_SET_NOISE               "SET_NOISE"
#define CMD_GET_NOISE               "GET_NOISE"
#define CMD_AFH_RANGE_CONFIG            "AFH_RANGE_CONFIG"
#define CMD_PTA_CONFIG              "PTA_CONFIG"
#define CMD_PTA_TAG_CONFIG          "PTA_TAG_CONFIG"
#define CMD_BA_SIZE_CONFIG          "BA_SIZE_CONFIG"
#define CMD_TRAFFIC_REPORT          "TRAFFIC_REPORT"
#define CMD_ADMINCTRL_CONFIG            "ADMINCTRL"
#define CMD_SET_POP             "SET_POP"
#define CMD_GET_POP             "GET_POP"
#define CMD_SET_ED              "SET_ED"
#define CMD_GET_ED              "GET_ED"
#define CMD_SET_PD              "SET_PD"
#define CMD_GET_PD              "GET_PD"
#define CMD_SET_MAX_RFGAIN          "SET_MAX_RFGAIN"
#define CMD_GET_MAX_RFGAIN          "GET_MAX_RFGAIN"
#define CMD_NOISE_HISTOGRAM         "NOISE_HISTOGRAM"
#define CMD_SET_ADM_CTRL            "SET_ADM"
#define CMD_SET_BCN_TH              "SET_BCN_TH"
#define CMD_GET_BCN_TH              "GET_BCN_TH"
#define CMD_SET_DEWEIGHTING_TH          "SET_DEWEIGHTING_TH"
#define CMD_GET_DEWEIGHTING_TH          "GET_DEWEIGHTING_TH"
#define CMD_GET_DEWEIGHTING_NOISE       "GET_DEWEIGHTING_NOISE"
#define CMD_GET_DEWEIGHTING_WEIGHT      "GET_DEWEIGHTING_WEIGHT"
#define CMD_SET_ACT_INTV            "SET_ACT_INTV"
#define CMD_SET_1RPD                "SET_1RPD"
#define CMD_GET_1RPD                "GET_1RPD"
#define CMD_SET_MMPS                "SET_MMPS"
#define CMD_GET_MMPS                "GET_MMPS"
#define CMD_ENFORCE_POWER_MODE          "ENFORCE_POWER_MODE"
#define CMD_GET_POWER_MODE          "GET_POWER_MODE"
#if CFG_SUPPORT_802_11K
#define CMD_NEIGHBOR_REQUEST            "NEIGHBOR-REQUEST"
#endif
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
#define CMD_BSS_TRANSITION_QUERY        "BSS-TRANSITION-QUERY"
#endif

#define CMD_GET_1XTX_STATUS         "GET_1XTX_STATUS"
#define CMD_TEST_1XTX_STATUS            "TEST_1XTX_STATUS"

#ifdef CFG_SUPPORT_EXT_PTA_DEBUG_COMMAND
#define CMD_EXT_PTA_CONFIG          "EXT_PTA_CONFIG"
#endif
#if CFG_RX_SINGLE_CHAIN_SUPPORT
#define CMD_SET_RXC             "SET_RXC"
#define CMD_GET_RXC             "GET_RXC"
#endif

#define CMD_GET_BCNTIMEOUT_NUM          "GET_BCNTIMEOUT_NUM"
#define CMD_SET_EVERY_TBTT          "SET_EVERY_TBTT"
#define CMD_GET_EVERY_TBTT          "GET_EVERY_TBTT"

enum {
    CMD_ADVCTL_NOISE_ID = 1,
    CMD_ADVCTL_POP_ID,
    CMD_ADVCTL_ED_ID,
    CMD_ADVCTL_PD_ID,
    CMD_ADVCTL_MAX_RFGAIN_ID,
    CMD_ADVCTL_ADM_CTRL_ID,
    CMD_ADVCTL_BCN_TH_ID = 9,
    CMD_ADVCTL_DEWEIGHTING_TH_ID,
    CMD_ADVCTL_DEWEIGHTING_NOISE_ID,
    CMD_ADVCTL_DEWEIGHTING_WEIGHT_ID,
    CMD_ADVCTL_ACT_INTV_ID,
    CMD_ADVCTL_1RPD,
    CMD_ADVCTL_MMPS,
#if CFG_RX_SINGLE_CHAIN_SUPPORT
    CMD_ADVCTL_RXC_ID           = 17,
#endif
    CMD_ADVCTL_SNR_ID           = 18,
    CMD_ADVCTL_BCNTIMOUT_NUM_ID = 19,
    CMD_ADVCTL_EVERY_TBTT_ID    = 20,
    CMD_ADVCTL_MAX
};
#endif

#define COEX_REF_TABLE_ID_ISO_DETECTION_VALUE       1
#define COEX_REF_TABLE_ID_COEX_FDD_PARAM        2
#define COEX_REF_TABLE_ID_COEX_WMT_CONFIG       3
#define COEX_REF_TABLE_ID_ACTIVE_BSSID          4
#define COEX_REF_TABLE_ID_COEX_MODE         5
#define COEX_REF_TABLE_ID_HYBRID_MODE           6
#define COEX_REF_TABLE_ID_COEX_ISO          7
#define COEX_REF_TABLE_ID_COEX_CHANNEL_INFO     8
#define COEX_REF_TABLE_ID_COEX_BT_PROFILE       9
#define COEX_REF_TABLE_ID_BT_LONGRX_DISABLE_WFTX    10
#define COEX_REF_TABLE_ID_BT_PORT           11
#define COEX_REF_TABLE_ID_BTTX_POWER            12
#define COEX_REF_TABLE_ID_TDD_BAND          13
#define COEX_REF_TABLE_ID_PER_PKT_STAT          30
#define COEX_REF_TABLE_ID_PER_PKT_BT_HIT_CNT        31
#define COEX_REF_TABLE_ID_PER_PKT_RX_PROTECT_CTL    32
#define COEX_REF_TABLE_ID_PER_PKT_WF_PROTTIME       33
#define COEX_REF_TABLE_ID_PER_PKT_BT_DURATION       34
#define COEX_REF_TABLE_ID_BTRX_GAIN_INFO        35
#define COEX_REF_TABLE_ID_BTTX_PWR_DIST         36
#define COEX_REF_TABLE_ID_WFRX_GAIN_DIST        37

#define IW_PRIV_BUF_SIZE                2000

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* NIC BBCR configuration entry structure */
typedef struct _PRIV_CONFIG_ENTRY {
    u8 ucOffset;
    u8 ucValue;
} PRIV_CONFIG_ENTRY, *PPRIV_CONFIG_ENTRY;

typedef WLAN_STATUS (*PFN_OID_HANDLER_FUNC_REQ)(IN void *prAdapter,
                                                IN OUT void *pvBuf,
                                                IN u32 u4BufLen,
                                                OUT u32 *pu4OutInfoLen);

typedef enum _ENUM_OID_METHOD_T {
    ENUM_OID_GLUE_ONLY,
    ENUM_OID_GLUE_EXTENSION,
    ENUM_OID_DRIVER_CORE
} ENUM_OID_METHOD_T,
*P_ENUM_OID_METHOD_T;

/* OID set/query processing entry */
typedef struct _WLAN_REQ_ENTRY {
    u32 rOid;  /* OID */
    u8 *pucOidName;  /* OID name text */
    u8 fgQryBufLenChecking;
    u8 fgSetBufLenChecking;
    ENUM_OID_METHOD_T eOidMethod;
    u32 u4InfoBufLen;
    PFN_OID_HANDLER_FUNC_REQ pfOidQueryHandler;  /*  PFN_OID_HANDLER_FUNC */
    PFN_OID_HANDLER_FUNC_REQ pfOidSetHandler;  /* PFN_OID_HANDLER_FUNC */
} WLAN_REQ_ENTRY, *P_WLAN_REQ_ENTRY;

typedef struct _NDIS_TRANSPORT_STRUCT {
    u32 ndisOidCmd;
    u32 inNdisOidlength;
    u32 outNdisOidLength;
    u8 ndisOidContent[16];
} NDIS_TRANSPORT_STRUCT, *P_NDIS_TRANSPORT_STRUCT;

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

int priv_set_int(IN struct net_device *prNetDev,
                 IN struct iw_request_info *prIwReqInfo,
                 IN union iwreq_data *prIwReqData,
                 IN char *pcExtra);

int priv_get_int(IN struct net_device *prNetDev,
                 IN struct iw_request_info *prIwReqInfo,
                 IN union iwreq_data *prIwReqData,
                 IN OUT char *pcExtra);

int priv_set_ints(IN struct net_device *prNetDev,
                  IN struct iw_request_info *prIwReqInfo,
                  IN union iwreq_data *prIwReqData,
                  IN char *pcExtra);

int priv_get_ints(IN struct net_device *prNetDev,
                  IN struct iw_request_info *prIwReqInfo,
                  IN union iwreq_data *prIwReqData,
                  IN OUT char *pcExtra);

int priv_set_struct(IN struct net_device *prNetDev,
                    IN struct iw_request_info *prIwReqInfo,
                    IN union iwreq_data *prIwReqData,
                    IN char *pcExtra);

int priv_get_struct(IN struct net_device *prNetDev,
                    IN struct iw_request_info *prIwReqInfo,
                    IN union iwreq_data *prIwReqData,
                    IN OUT char *pcExtra);

int priv_set_driver(IN struct net_device *prNetDev,
                    IN struct iw_request_info *prIwReqInfo,
                    IN union iwreq_data *prIwReqData,
                    IN OUT char *pcExtra);

int priv_support_ioctl(IN struct net_device *prDev,
                       IN OUT struct ifreq *prReq,
                       IN int i4Cmd);

int priv_support_driver_cmd(IN struct net_device *prDev,
                            IN OUT struct ifreq *prReq,
                            IN int i4Cmd);

#ifdef CFG_ANDROID_AOSP_PRIV_CMD
int android_private_support_driver_cmd(IN struct net_device *prDev,
                                       IN OUT struct ifreq *prReq,
                                       IN int i4Cmd);
#endif

s32 priv_driver_cmds(IN struct net_device *prNetDev,
                     IN s8 *pcCommand,
                     IN s32 i4TotalLen);

int priv_driver_set_cfg(IN struct net_device *prNetDev,
                        IN char *pcCommand,
                        IN int i4TotalLen);

#if CFG_SUPPORT_QA_TOOL
int priv_qa_agent(IN struct net_device *prNetDev,
                  IN struct iw_request_info *prIwReqInfo,
                  IN union iwreq_data *prIwReqData,
                  IN char *pcExtra);
int priv_ate_set(IN struct net_device *prNetDev,
                 IN struct iw_request_info *prIwReqInfo,
                 IN union iwreq_data *prIwReqData,
                 IN char *pcExtra);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
