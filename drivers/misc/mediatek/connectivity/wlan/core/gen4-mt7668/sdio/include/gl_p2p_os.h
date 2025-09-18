/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_p2p_os.h
 *    \brief  List the external reference to OS for p2p GLUE Layer.
 *
 *    In this file we define the data structure - GLUE_INFO_T to store those
 * objects we acquired from OS - e.g. TIMER, SPINLOCK, NET DEVICE ... . And all
 * the external reference (header file, extern func() ..) to OS for GLUE Layer
 * should also list down here.
 */

#ifndef _GL_P2P_OS_H
#define _GL_P2P_OS_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   V A R I A B L E
 *******************************************************************************
 */

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
extern const struct net_device_ops p2p_netdev_ops;
#endif

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

extern struct net_device *g_P2pPrDev;
extern struct wireless_dev *gprP2pWdev;
extern struct wireless_dev *gprP2pRoleWdev[KAL_P2P_NUM];

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

struct _GL_P2P_INFO_T {
    /* P2P Device interface handle */
    /*only first p2p have this devhandler*/
    struct net_device *prDevHandler;
    /*struct net_device *prRoleDevHandler;*/ /* TH3 multiple P2P */

    struct net_device *aprRoleHandler;

    /* Todo : should move to the glueinfo or not*/
    /*u8 ucRoleInterfaceNum;*/ /* TH3 multiple P2P */

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
    /* cfg80211 */
    struct wireless_dev *prWdev;
    /*struct wireless_dev *prRoleWdev[KAL_P2P_NUM];*/ /* TH3 multiple P2P */

    /*struct cfg80211_scan_request *prScanRequest;*/ /* TH3 multiple P2P */

    /*u64 u8Cookie;*/ /* TH3 multiple P2P */

    /* Generation for station list update. */
    s32 i4Generation;

    /*u32 u4OsMgmtFrameFilter;*/ /* TH3 multiple P2P */
#endif

    /* Device statistics */
    /*struct net_device_stats rNetDevStats;*/ /* TH3 multiple P2P */

    /* glue layer variables */
    /*move to glueinfo->adapter */
    /* u8                     fgIsRegistered; */
    /*u32 u4FreqInKHz;*/ /* TH3 multiple P2P */ /* frequency */
    u8 ucRole;  /* 0: P2P Device, 1: Group Client, 2: Group Owner */
    /*u8 ucIntent;*/ /* TH3 multiple P2P */ /* range: 0-15 */
    /*u8 ucScanMode;*/
    /* TH3 multiple P2P */ /* 0: Search & Listen, 1: Scan without probe
     *                        response */

    /*ENUM_PARAM_MEDIA_STATE_T eState;*/ /* TH3 multiple P2P */
    /*u32 u4PacketFilter;*/ /* TH3 multiple P2P */
    /*PARAM_MAC_ADDRESS aucMCAddrList[MAX_NUM_GROUP_ADDR];*/ /* TH3 multiple
     *                                                          P2P
     */

    /* connection-requested peer information */ /* TH3 multiple P2P */
    /*u8 aucConnReqDevName[32];*/ /* TH3 multiple P2P */
    /*s32 u4ConnReqNameLength;*/ /* TH3 multiple P2P */
    /*PARAM_MAC_ADDRESS rConnReqPeerAddr;*/ /* TH3 multiple P2P */
    /*PARAM_MAC_ADDRESS rConnReqGroupAddr;*/
    /* TH3 multiple P2P */ /* For invitation group. */
    /*u8 ucConnReqDevType;*/ /* TH3 multiple P2P */
    /*s32 i4ConnReqConfigMethod;*/ /* TH3 multiple P2P */
    /*s32 i4ConnReqActiveConfigMethod;*/ /* TH3 multiple P2P */

    u32 u4CipherPairwise;
    /*u8 ucWSCRunning;*/ /* TH3 multiple P2P */

    u8 aucWSCIE[4][400];  /* 0 for beacon, 1 for probe req, 2 for probe
                           * response, 3 for assoc response */
    u16 u2WSCIELen[4];

#if CFG_SUPPORT_WFD
    u8 aucWFDIE[400];  /* 0 for beacon, 1 for probe req, 2 for probe response
                        */
    u16 u2WFDIELen;
    /* u8                      aucVenderIE[1024]; */ /* Save the other IE
     *                                                  for
     * prove resp */
/* u16                     u2VenderIELen; */
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
    struct cfg80211_chan_def *chandef;
    u32 cac_time_ms;
#endif

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
    /* Hotspot Client Management */
    /* dependent with  #define P2P_MAXIMUM_CLIENT_COUNT 10,
     * fix me to PARAM_MAC_ADDRESS
     * aucblackMACList[P2P_MAXIMUM_CLIENT_COUNT];
     */
    PARAM_MAC_ADDRESS aucblackMACList[10];
    u8 ucMaxClients;
#endif

#if CFG_SUPPORT_HOTSPOT_OPTIMIZATION
    /*u8 fgEnableHotspotOptimization;*/ /* TH3 multiple P2P */
    /*u32 u4PsLevel;*/ /* TH3 multiple P2P */
#endif

#if 0
    u8 fgIsNetDevRegistered;
#endif
};

struct _GL_P2P_DEV_INFO_T {
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
    struct cfg80211_scan_request *prScanRequest;
#if (CFG_ENABLE_UNIFY_WIPHY == 0)
    struct cfg80211_scan_request rBackupScanRequest;
#endif
    u64 u8Cookie;
    u32 u4OsMgmtFrameFilter;
#endif
    u32 u4PacketFilter;
    PARAM_MAC_ADDRESS aucMCAddrList[MAX_NUM_GROUP_ADDR];
    u8 ucWSCRunning;
};

#ifdef CONFIG_NL80211_TESTMODE
typedef struct _NL80211_DRIVER_TEST_PRE_PARAMS {
    u16 idx_mode;
    u16 idx;
    u32 value;
} NL80211_DRIVER_TEST_PRE_PARAMS, *P_NL80211_DRIVER_TEST_PRE_PARAMS;

typedef struct _NL80211_DRIVER_TEST_PARAMS {
    u32 index;
    u32 buflen;
} NL80211_DRIVER_TEST_PARAMS, *P_NL80211_DRIVER_TEST_PARAMS;

/* P2P Sigma*/
typedef struct _NL80211_DRIVER_P2P_SIGMA_PARAMS {
    NL80211_DRIVER_TEST_PARAMS hdr;
    u32 idx;
    u32 value;
} NL80211_DRIVER_P2P_SIGMA_PARAMS, *P_NL80211_DRIVER_P2P_SIGMA_PARAMS;

/* Hotspot Client Management */
typedef struct _NL80211_DRIVER_hotspot_block_PARAMS {
    NL80211_DRIVER_TEST_PARAMS hdr;
    u8 ucblocked;
    u8 aucBssid[MAC_ADDR_LEN];
} NL80211_DRIVER_hotspot_block_PARAMS, *P_NL80211_DRIVER_hotspot_block_PARAMS;

#if CFG_SUPPORT_WFD
typedef struct _NL80211_DRIVER_WFD_PARAMS {
    NL80211_DRIVER_TEST_PARAMS hdr;
    u32 WfdCmdType;
    u8 WfdEnable;
    u8 WfdCoupleSinkStatus;
    u8 WfdSessionAvailable;
    u8 WfdSigmaMode;
    u16 WfdDevInfo;
    u16 WfdControlPort;
    u16 WfdMaximumTp;
    u16 WfdExtendCap;
    u8 WfdCoupleSinkAddress[MAC_ADDR_LEN];
    u8 WfdAssociatedBssid[MAC_ADDR_LEN];
    u8 WfdVideoIp[4];
    u8 WfdAudioIp[4];
    u16 WfdVideoPort;
    u16 WfdAudioPort;
    u32 WfdFlag;
    u32 WfdPolicy;
    u32 WfdState;
    u8 WfdSessionInformationIE[24 * 8];  /* Include Subelement ID, length */
    u16 WfdSessionInformationIELen;
    u8 aucReserved1[2];
    u8 aucWfdPrimarySinkMac[MAC_ADDR_LEN];
    u8 aucWfdSecondarySinkMac[MAC_ADDR_LEN];
    u32 WfdAdvanceFlag;
    /* Group 1 64 bytes */
    u8 aucWfdLocalIp[4];
    u16 WfdLifetimeAc2;  /* Unit is 2 TU */
    u16 WfdLifetimeAc3;  /* Unit is 2 TU */
    u16 WfdCounterThreshold;  /* Unit is ms */
    u8 aucReserved2[54];
    /* Group 3 64 bytes */
    u8 aucReserved3[64];
    /* Group 3 64 bytes */
    u8 aucReserved4[64];
} NL80211_DRIVER_WFD_PARAMS, *P_NL80211_DRIVER_WFD_PARAMS;
#endif
#endif

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

u8 p2pRegisterToWlan(P_GLUE_INFO_T prGlueInfo);

u8 p2pUnregisterToWlan(P_GLUE_INFO_T prGlueInfo);

u8 p2pLaunch(P_GLUE_INFO_T prGlueInfo);

u8 p2pRemove(P_GLUE_INFO_T prGlueInfo);

void p2pSetMode(IN u8 ucAPMode);

u8 glRegisterP2P(P_GLUE_INFO_T prGlueInfo,
                 const char *prDevName,
                 const char *prDevName2,
                 u8 ucApMode);


#if CFG_ENABLE_UNIFY_WIPHY
int glSetupP2P(P_GLUE_INFO_T prGlueInfo, struct wireless_dev *prP2pWdev,
               struct net_device *prP2pDev, int u4Idx, u8 fgIsApMode);
#endif
u8 glUnregisterP2P(P_GLUE_INFO_T prGlueInfo, u8 ucIdx);

u8 p2pNetRegister(P_GLUE_INFO_T prGlueInfo, u8 fgIsRtnlLockAcquired);

u8 p2pNetUnregister(P_GLUE_INFO_T prGlueInfo, u8 fgIsRtnlLockAcquired);

#if CFG_ENABLE_UNIFY_WIPHY
u8 p2PAllocInfo(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucIdex);
#endif
u8 p2PFreeInfo(P_GLUE_INFO_T prGlueInfo, u8 ucIdx);

void p2pSetSuspendMode(P_GLUE_INFO_T prGlueInfo, u8 fgEnable);
u8 glP2pCreateWirelessDevice(P_GLUE_INFO_T prGlueInfo);
void glP2pDestroyWirelessDevice(void);
void p2pUpdateChannelTableByDomain(P_GLUE_INFO_T prGlueInfo);

#endif
