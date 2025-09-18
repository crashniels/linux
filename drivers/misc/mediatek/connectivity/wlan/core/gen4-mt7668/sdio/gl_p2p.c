// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_p2p.c
 *    \brief  Main routines of Linux driver interface for Wi-Fi Direct
 *
 *    This file contains the main routines of Linux driver for MediaTek Inc.
 * 802.11 Wireless LAN Adapters.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include <linux/poll.h>

#include <linux/kmod.h>

#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "gl_wext.h"

/* #include <net/cfg80211.h> */
#include "gl_p2p_ioctl.h"

#include "precomp.h"
#include "gl_cfg80211.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define ARGV_MAX_NUM (4)

/*For CFG80211 - wiphy parameters*/
#define MAX_SCAN_LIST_NUM (1)
#define MAX_SCAN_IE_LEN (512)

#if (CFG_SUPPORT_DFS_MASTER == 1)
#define MAX_CSA_COUNTER 10
#endif

#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
#define MAX_LOOK_BACK_NUN (3)
#endif
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

struct net_device *g_P2pPrDev;
struct wireless_dev *gprP2pWdev;
struct wireless_dev *gprP2pRoleWdev[KAL_P2P_NUM];
struct net_device *gPrP2pDev[KAL_P2P_NUM];

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211

#if (CFG_ENABLE_UNIFY_WIPHY == 0)
static struct cfg80211_ops mtk_p2p_ops = {
    .add_virtual_intf = mtk_p2p_cfg80211_add_iface,
    .change_virtual_intf = mtk_p2p_cfg80211_change_iface,  /* 1 st */
    .del_virtual_intf = mtk_p2p_cfg80211_del_iface,
    .change_bss = mtk_p2p_cfg80211_change_bss,
    .scan = mtk_p2p_cfg80211_scan,
    .abort_scan = mtk_p2p_cfg80211_abort_scan,
    .remain_on_channel = mtk_p2p_cfg80211_remain_on_channel,
    .cancel_remain_on_channel = mtk_p2p_cfg80211_cancel_remain_on_channel,
    .mgmt_tx = mtk_p2p_cfg80211_mgmt_tx,
    .mgmt_tx_cancel_wait = mtk_p2p_cfg80211_mgmt_tx_cancel_wait,
    .connect = mtk_p2p_cfg80211_connect,
    .disconnect = mtk_p2p_cfg80211_disconnect,
    .deauth = mtk_p2p_cfg80211_deauth,
    .disassoc = mtk_p2p_cfg80211_disassoc,
    .start_ap = mtk_p2p_cfg80211_start_ap,
    .change_beacon = mtk_p2p_cfg80211_change_beacon,
    .stop_ap = mtk_p2p_cfg80211_stop_ap,
    .set_wiphy_params = mtk_p2p_cfg80211_set_wiphy_params,
    .del_station = mtk_p2p_cfg80211_del_station,
    .set_bitrate_mask = mtk_p2p_cfg80211_set_bitrate_mask,
#if KERNEL_VERSION(5, 8, 0) >= CFG80211_VERSION_CODE
    .mgmt_frame_register = mtk_p2p_cfg80211_mgmt_frame_register,
#endif
    .get_station = mtk_p2p_cfg80211_get_station,
    .add_key = mtk_p2p_cfg80211_add_key,
    .get_key = mtk_p2p_cfg80211_get_key,
    .del_key = mtk_p2p_cfg80211_del_key,
    .set_default_key = mtk_p2p_cfg80211_set_default_key,
    .set_default_mgmt_key = mtk_p2p_cfg80211_set_mgmt_key,
    .join_ibss = mtk_p2p_cfg80211_join_ibss,
    .leave_ibss = mtk_p2p_cfg80211_leave_ibss,
    .set_tx_power = mtk_p2p_cfg80211_set_txpower,
    .get_tx_power = mtk_p2p_cfg80211_get_txpower,
    .set_power_mgmt = mtk_p2p_cfg80211_set_power_mgmt,
    .start_radar_detection = mtk_p2p_cfg80211_start_radar_detection,
    .channel_switch = mtk_p2p_cfg80211_channel_switch,
#ifdef CONFIG_NL80211_TESTMODE
    .testmode_cmd = mtk_p2p_cfg80211_testmode_cmd,
#endif
};

/* There isn't a lot of sense in it, but you can transmit anything you like */
static const struct ieee80211_txrx_stypes
    mtk_cfg80211_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
    [NL80211_IFTYPE_ADHOC] =
    {  /**/
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ACTION >> 4)
    },
    [NL80211_IFTYPE_STATION] =
    {  /**/
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
              BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
    },
    [NL80211_IFTYPE_AP] =
    {  /**/
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
              BIT(IEEE80211_STYPE_ACTION >> 4)
    },
    [NL80211_IFTYPE_AP_VLAN] =
    {  /* copy AP */
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
              BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
              BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
              BIT(IEEE80211_STYPE_DISASSOC >> 4) |
              BIT(IEEE80211_STYPE_AUTH >> 4) |
              BIT(IEEE80211_STYPE_DEAUTH >> 4) |
              BIT(IEEE80211_STYPE_ACTION >> 4)
    },
    [NL80211_IFTYPE_P2P_CLIENT] =
    {  /**/
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
              BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
    },
    [NL80211_IFTYPE_P2P_GO] = {  /**/
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
              BIT(IEEE80211_STYPE_ACTION >> 4)
    }
};

#endif
#endif

static const struct iw_priv_args rP2PIwPrivTable[] = {
    { IOCTL_GET_DRIVER, IW_PRIV_TYPE_CHAR | 2000, IW_PRIV_TYPE_CHAR | 2000,
      "driver" },
};

#ifdef CONFIG_PM
static const struct wiphy_wowlan_support mtk_p2p_wowlan_support = {
    .flags = WIPHY_WOWLAN_DISCONNECT | WIPHY_WOWLAN_ANY,
};
#endif

static const struct ieee80211_iface_limit mtk_p2p_sta_go_limits[] = {
    {
        .max = 3,
        .types = BIT(NL80211_IFTYPE_STATION),
    },

    {
        .max = 1,
        .types = BIT(NL80211_IFTYPE_P2P_GO) | BIT(NL80211_IFTYPE_P2P_CLIENT),
    },
};

static const struct ieee80211_iface_combination mtk_iface_combinations_sta[] = {
    {
#ifdef CFG_NUM_DIFFERENT_CHANNELS_STA
        .num_different_channels = CFG_NUM_DIFFERENT_CHANNELS_STA,
#else
        .num_different_channels = 2,
#endif
        .max_interfaces = 3,
        .limits = mtk_p2p_sta_go_limits,
        .n_limits = 1,  /* include p2p */
    },
};

static const struct ieee80211_iface_combination mtk_iface_combinations_p2p[] = {
    {
#ifdef CFG_NUM_DIFFERENT_CHANNELS_P2P
        .num_different_channels = CFG_NUM_DIFFERENT_CHANNELS_P2P,
#else
        .num_different_channels = 2,
#endif
        .max_interfaces = 3,
        .limits = mtk_p2p_sta_go_limits,
        .n_limits = ARRAY_SIZE(mtk_p2p_sta_go_limits),  /* include p2p */
    },
};

const struct ieee80211_iface_combination *p_mtk_iface_combinations_sta =
    mtk_iface_combinations_sta;
const s32 mtk_iface_combinations_sta_num =
    ARRAY_SIZE(mtk_iface_combinations_sta);

const struct ieee80211_iface_combination *p_mtk_iface_combinations_p2p =
    mtk_iface_combinations_p2p;
const s32 mtk_iface_combinations_p2p_num =
    ARRAY_SIZE(mtk_iface_combinations_p2p);

#ifdef STA_P2P_MCC
static const struct ieee80211_iface_limit p2p_iface_limits_mcc[] = {
    { .max = 2,
      .types = (BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_P2P_CLIENT) |
                BIT(NL80211_IFTYPE_AP) | BIT(NL80211_IFTYPE_P2P_GO)) }
};

static const struct ieee80211_iface_combination
    p2p_iface_comb_mcc[] = {  /**/
    {
        .limits = p2p_iface_limits_mcc,
        .n_limits = ARRAY_SIZE(p2p_iface_limits_mcc),
        .max_interfaces = 2,
        .num_different_channels = 2,
        .beacon_int_infra_match = false,
    }
};
#endif

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/* Net Device Hooks */
static int p2pOpen(IN struct net_device *prDev);

static int p2pStop(IN struct net_device *prDev);

static struct net_device_stats *p2pGetStats(IN struct net_device *prDev);

static void p2pSetMulticastList(IN struct net_device *prDev);

static int p2pHardStartXmit(IN struct sk_buff *prSkb,
                            IN struct net_device *prDev);

static int p2pSetMACAddress(IN struct net_device *prDev, void *addr);

static int p2pDoIOCTL(struct net_device *prDev, struct ifreq *prIFReq,
                      int i4Cmd);

/*----------------------------------------------------------------------------*/
/*!
 * \brief A function for prDev->init
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0         The execution of wlanInit succeeds.
 * \retval -ENXIO    No such device.
 */
/*----------------------------------------------------------------------------*/
static int p2pInit(struct net_device *prDev)
{
    if (!prDev) {
        return -ENXIO;
    }

    return 0;
}

const struct net_device_ops p2p_netdev_ops = {
    .ndo_open = p2pOpen,
    .ndo_stop = p2pStop,
    .ndo_set_mac_address = p2pSetMACAddress,
    .ndo_set_rx_mode = p2pSetMulticastList,
    .ndo_get_stats = p2pGetStats,
    .ndo_do_ioctl = p2pDoIOCTL,
    .ndo_start_xmit = p2pHardStartXmit,
    .ndo_select_queue = wlanSelectQueue,
    .ndo_init = p2pInit,
};

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Allocate memory for P2P_INFO, GL_P2P_INFO, P2P_CONNECTION_SETTINGS
 *                                          P2P_SPECIFIC_BSS_INFO, P2P_FSM_INFO
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   true
 *           false
 */
/*----------------------------------------------------------------------------*/
u8 p2PAllocInfo(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucIdex)
{
    P_ADAPTER_T prAdapter = NULL;
    P_WIFI_VAR_T prWifiVar = NULL;
    /* u32 u4Idx = 0; */

    ASSERT(prGlueInfo);
    if (!prGlueInfo) {
        DBGLOG(P2P, ERROR, "prGlueInfo error\n");
        return false;
    }

    prAdapter = prGlueInfo->prAdapter;
    prWifiVar = &(prAdapter->rWifiVar);

    ASSERT(prAdapter);
    ASSERT(prWifiVar);

    do {
        if (prGlueInfo->prP2PInfo[ucIdex] == NULL) {
            /*alloc memory for p2p info */
#if CFG_ENABLE_UNIFY_WIPHY
            prGlueInfo->prP2PInfo[ucIdex] =
                kalMemAlloc(sizeof(GL_P2P_INFO_T), VIR_MEM_TYPE);
            if (prGlueInfo->prP2PDevInfo == NULL) {
                prGlueInfo->prP2PDevInfo =
                    kalMemAlloc(sizeof(GL_P2P_DEV_INFO_T), VIR_MEM_TYPE);
                if (prGlueInfo->prP2PDevInfo) {
                    kalMemZero(prGlueInfo->prP2PDevInfo,
                               sizeof(GL_P2P_DEV_INFO_T));
                }
            }
            if (prAdapter->prP2pInfo == NULL) {
                prAdapter->prP2pInfo =
                    kalMemAlloc(sizeof(P2P_INFO_T), VIR_MEM_TYPE);
                if (prAdapter->prP2pInfo) {
                    kalMemZero(prAdapter->prP2pInfo, sizeof(P2P_INFO_T));
                }
            }
            if (prWifiVar->prP2pDevFsmInfo == NULL) {
                /* Don't only create P2P device for ucIdex 0.
                 * Avoid the exception that mtk_init_ap_role
                 * called without p2p0.
                 */
                prWifiVar->prP2pDevFsmInfo =
                    kalMemAlloc(sizeof(P2P_DEV_FSM_INFO_T), VIR_MEM_TYPE);
                if (prWifiVar->prP2pDevFsmInfo) {
                    kalMemZero(prWifiVar->prP2pDevFsmInfo,
                               sizeof(P2P_DEV_FSM_INFO_T));
                }
            }
#else  /* (CFG_ENABLE_UNIFY_WIPHY == 0) */
            prGlueInfo->prP2PInfo[ucIdex] =
                kalMemAlloc(sizeof(GL_P2P_INFO_T), VIR_MEM_TYPE);

            if (ucIdex == 0) {
                /*printk("[CHECK!]p2PAllocInfo : Alloc Common
                 * part only first interface\n");*/
                prGlueInfo->prP2PDevInfo =
                    kalMemAlloc(sizeof(GL_P2P_DEV_INFO_T), VIR_MEM_TYPE);
                prAdapter->prP2pInfo =
                    kalMemAlloc(sizeof(P2P_INFO_T), VIR_MEM_TYPE);
                prWifiVar->prP2pDevFsmInfo =
                    kalMemAlloc(sizeof(P2P_DEV_FSM_INFO_T), VIR_MEM_TYPE);
            }
#endif  /* CFG_ENABLE_UNIFY_WIPHY */

            prWifiVar->prP2PConnSettings[ucIdex] =
                kalMemAlloc(sizeof(P2P_CONNECTION_SETTINGS_T), VIR_MEM_TYPE);
            prWifiVar->prP2pSpecificBssInfo[ucIdex] =
                kalMemAlloc(sizeof(P2P_SPECIFIC_BSS_INFO_T), VIR_MEM_TYPE);

            prWifiVar->prP2pQueryStaStatistics[ucIdex] =
                kalMemAlloc(sizeof(PARAM_GET_STA_STATISTICS), VIR_MEM_TYPE);
        } else {
            ASSERT(prAdapter->prP2pInfo != NULL);
            ASSERT(prWifiVar->prP2PConnSettings[ucIdex] != NULL);
            /* ASSERT(prWifiVar->prP2pFsmInfo != NULL); */
            ASSERT(prWifiVar->prP2pSpecificBssInfo[ucIdex] != NULL);
        }

        /*MUST set memory to 0 */
        kalMemZero(prGlueInfo->prP2PInfo[ucIdex], sizeof(GL_P2P_INFO_T));
#if (CFG_ENABLE_UNIFY_WIPHY == 0)
        if (ucIdex == 0) {
            kalMemZero(prGlueInfo->prP2PDevInfo, sizeof(GL_P2P_DEV_INFO_T));
            kalMemZero(prAdapter->prP2pInfo, sizeof(P2P_INFO_T));
            kalMemZero(prWifiVar->prP2pDevFsmInfo, sizeof(P2P_DEV_FSM_INFO_T));
        }
#endif
        kalMemZero(prWifiVar->prP2PConnSettings[ucIdex],
                   sizeof(P2P_CONNECTION_SETTINGS_T));
        /* kalMemZero(prWifiVar->prP2pFsmInfo, sizeof(P2P_FSM_INFO_T));
         */
        kalMemZero(prWifiVar->prP2pSpecificBssInfo[ucIdex],
                   sizeof(P2P_SPECIFIC_BSS_INFO_T));

        if (prWifiVar->prP2pQueryStaStatistics[ucIdex]) {
            kalMemZero(prWifiVar->prP2pQueryStaStatistics[ucIdex],
                       sizeof(PARAM_GET_STA_STATISTICS));
        }
    } while (false);

    if (!prGlueInfo->prP2PDevInfo) {
        DBGLOG(P2P, ERROR, "prP2PDevInfo error\n");
    } else {
        DBGLOG(P2P, INFO, "prP2PDevInfo ok\n");
    }

    if (!prGlueInfo->prP2PInfo[ucIdex]) {
        DBGLOG(P2P, ERROR, "prP2PInfo error\n");
    } else {
        DBGLOG(P2P, INFO, "prP2PInfo ok\n");
    }

    /* chk if alloc successful or not */
    if (prGlueInfo->prP2PInfo[ucIdex] && prGlueInfo->prP2PDevInfo &&
        prAdapter->prP2pInfo && prWifiVar->prP2PConnSettings[ucIdex] &&
        /* prWifiVar->prP2pFsmInfo && */
        prWifiVar->prP2pSpecificBssInfo[ucIdex]) {
        return true;
    }

    DBGLOG(P2P, ERROR, "[fail!]p2PAllocInfo :fail\n");

    if (prWifiVar->prP2pSpecificBssInfo[ucIdex]) {
        kalMemFree(prWifiVar->prP2pSpecificBssInfo[ucIdex], VIR_MEM_TYPE,
                   sizeof(P2P_SPECIFIC_BSS_INFO_T));

        prWifiVar->prP2pSpecificBssInfo[ucIdex] = NULL;
    }

    if (prWifiVar->prP2pQueryStaStatistics[ucIdex]) {
        kalMemFree(prWifiVar->prP2pQueryStaStatistics[ucIdex], VIR_MEM_TYPE,
                   sizeof(P_PARAM_GET_STA_STATISTICS));

        prWifiVar->prP2pQueryStaStatistics[ucIdex] = NULL;
    }

    /* if (prWifiVar->prP2pFsmInfo) { */
    /* kalMemFree(prWifiVar->prP2pFsmInfo, VIR_MEM_TYPE,
     * sizeof(P2P_FSM_INFO_T)); */

    /* prWifiVar->prP2pFsmInfo = NULL; */
    /* } */

    if (prWifiVar->prP2PConnSettings[ucIdex]) {
        kalMemFree(prWifiVar->prP2PConnSettings[ucIdex], VIR_MEM_TYPE,
                   sizeof(P2P_CONNECTION_SETTINGS_T));

        prWifiVar->prP2PConnSettings[ucIdex] = NULL;
    }
    if (prGlueInfo->prP2PDevInfo) {
        kalMemFree(prGlueInfo->prP2PDevInfo, VIR_MEM_TYPE,
                   sizeof(GL_P2P_DEV_INFO_T));

        prGlueInfo->prP2PDevInfo = NULL;
    }
    if (prGlueInfo->prP2PInfo[ucIdex]) {
        kalMemFree(prGlueInfo->prP2PInfo[ucIdex], VIR_MEM_TYPE,
                   sizeof(GL_P2P_INFO_T));

        prGlueInfo->prP2PInfo[ucIdex] = NULL;
    }
    if (prAdapter->prP2pInfo) {
        kalMemFree(prAdapter->prP2pInfo, VIR_MEM_TYPE, sizeof(P2P_INFO_T));

        prAdapter->prP2pInfo = NULL;
    }
    return false;
}

static void p2pFreeMemSafe(P_GLUE_INFO_T prGlueInfo, void **pprMemInfo,
                           u32 size)
{
    void *prTmpMemInfo = NULL;

    GLUE_SPIN_LOCK_DECLARATION();

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
    prTmpMemInfo = *pprMemInfo;
    *pprMemInfo = NULL;
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

    kalMemFree(prTmpMemInfo, VIR_MEM_TYPE, size);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Free memory for P2P_INFO, GL_P2P_INFO, P2P_CONNECTION_SETTINGS
 *        P2P_SPECIFIC_BSS_INFO, P2P_FSM_INFO
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *	     [in] ucIdx	     The BSS with the idx will be freed.
 *			             "ucIdx == 0xff" will free all BSSs.
 *			             Only has meaning for "CFG_ENABLE_UNIFY_WIPHY == 1"
 * \return   true
 *           false
 */
/*----------------------------------------------------------------------------*/
u8 p2PFreeInfo(P_GLUE_INFO_T prGlueInfo, u8 ucIdx)
#if CFG_ENABLE_UNIFY_WIPHY
{
    P_ADAPTER_T prAdapter = prGlueInfo->prAdapter;
    ASSERT(prGlueInfo);
    ASSERT(prAdapter);
    if (ucIdx >= KAL_P2P_NUM) {
        DBGLOG(P2P, ERROR, "ucIdx=%d is invalid\n", ucIdx);
        return false;
    }
    /* TODO: how can I sure that the specific P2P device can be freed?
     * The original check is that prGlueInfo->prAdapter->fgIsP2PRegistered.
     */
    if (prGlueInfo->prP2PInfo[ucIdx] != NULL) {
        kalMemFree(prAdapter->rWifiVar.prP2PConnSettings[ucIdx], VIR_MEM_TYPE,
                   sizeof(P2P_CONNECTION_SETTINGS_T));
        prAdapter->rWifiVar.prP2PConnSettings[ucIdx] = NULL;
        kalMemFree(prAdapter->rWifiVar.prP2pSpecificBssInfo[ucIdx],
                   VIR_MEM_TYPE, sizeof(P2P_SPECIFIC_BSS_INFO_T));
        prAdapter->rWifiVar.prP2pSpecificBssInfo[ucIdx] = NULL;
        kalMemFree(prGlueInfo->prP2PInfo[ucIdx], VIR_MEM_TYPE,
                   sizeof(GL_P2P_INFO_T));
        prGlueInfo->prP2PInfo[ucIdx] = NULL;
        prAdapter->prP2pInfo->u4DeviceNum--;
    }
    if (prAdapter->prP2pInfo->u4DeviceNum == 0) {
        /* all prP2PInfo are freed, and free the general part now */
        if (prAdapter->prP2pInfo) {
            kalMemFree(prAdapter->prP2pInfo, VIR_MEM_TYPE, sizeof(P2P_INFO_T));
            prAdapter->prP2pInfo = NULL;
        }
        if (prGlueInfo->prP2PDevInfo) {
            kalMemFree(prGlueInfo->prP2PDevInfo, VIR_MEM_TYPE,
                       sizeof(GL_P2P_DEV_INFO_T));
            prGlueInfo->prP2PDevInfo = NULL;
        }
        if (prAdapter->rWifiVar.prP2pDevFsmInfo) {
            kalMemFree(prAdapter->rWifiVar.prP2pDevFsmInfo, VIR_MEM_TYPE,
                       sizeof(P2P_DEV_FSM_INFO_T));
            prAdapter->rWifiVar.prP2pDevFsmInfo = NULL;
        }
        /*Reomve p2p bss scan list */
        scanRemoveAllP2pBssDesc(prAdapter);
    }
    return true;
}
#else  /* (CFG_ENABLE_UNIFY_WIPHY == 0) */
{
    u8 i;
    P_ADAPTER_T prAdapter = prGlueInfo->prAdapter;
    P_WIFI_VAR_T prWifiVar;

    ASSERT(prGlueInfo);
    ASSERT(prAdapter);

    /* Expect that prAdapter->prP2pInfo must be existing. */
    if (prAdapter->prP2pInfo == NULL) {
        DBGLOG(P2P, ERROR, "prAdapter->prP2pInfo is NULL\n");
        return false;
    }

    prWifiVar = &prAdapter->rWifiVar;

    /* TODO: how can I sure that the specific P2P device can be freed?
     * The original check is that prGlueInfo->prAdapter->fgIsP2PRegistered.
     * For one wiphy feature, this func may be called without
     * (fgIsP2PRegistered == false) condition.
     */
    for (i = 0; i < KAL_P2P_NUM; i++) {  /* clear all for now */
        if (prGlueInfo->prP2PInfo[i] != NULL) {
            p2pFreeMemSafe(prGlueInfo,
                           (void **)&prWifiVar->prP2PConnSettings[i],
                           sizeof(P2P_CONNECTION_SETTINGS_T));

            p2pFreeMemSafe(prGlueInfo,
                           (void **)&prWifiVar->prP2pSpecificBssInfo[i],
                           sizeof(P2P_SPECIFIC_BSS_INFO_T));

            p2pFreeMemSafe(prGlueInfo, (void **)&prGlueInfo->prP2PInfo[i],
                           sizeof(GL_P2P_INFO_T));
            prAdapter->prP2pInfo->u4DeviceNum--;
        }

        if (prAdapter->prP2pInfo->u4DeviceNum == i) {
            p2pFreeMemSafe(prGlueInfo, (void **)&prAdapter->prP2pInfo,
                           sizeof(P2P_INFO_T));

            if (prGlueInfo->prP2PDevInfo) {
                p2pFreeMemSafe(prGlueInfo, (void **)&prGlueInfo->prP2PDevInfo,
                               sizeof(GL_P2P_DEV_INFO_T));
            }
            if (prAdapter->rWifiVar.prP2pDevFsmInfo) {
                p2pFreeMemSafe(prGlueInfo, (void **)&prWifiVar->prP2pDevFsmInfo,
                               sizeof(P2P_DEV_FSM_INFO_T));
            }

            p2pFreeMemSafe(prGlueInfo,
                           (void **)&prWifiVar->prP2pQueryStaStatistics,
                           sizeof(P2P_CONNECTION_SETTINGS_T));

            scanRemoveAllP2pBssDesc(prAdapter);
        }
    }

    return true;
}
#endif

u8 p2pNetRegister(P_GLUE_INFO_T prGlueInfo, u8 fgIsRtnlLockAcquired)
{
    u8 fgDoRegister = false;
    u8 fgRollbackRtnlLock = false;
    u8 ret = false;
    P_ADAPTER_T prAdapter = NULL;

    GLUE_SPIN_LOCK_DECLARATION();

    prAdapter = prGlueInfo->prAdapter;

    ASSERT(prGlueInfo);
    ASSERT(prAdapter);

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
    if (prAdapter->rP2PNetRegState == ENUM_NET_REG_STATE_UNREGISTERED &&
        prAdapter->rP2PRegState == ENUM_P2P_REG_STATE_REGISTERED) {
        prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_REGISTERING;
        fgDoRegister = true;
    }
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

    if (!fgDoRegister) {
        return true;
    }

    if (fgIsRtnlLockAcquired && rtnl_is_locked()) {
        fgRollbackRtnlLock = true;
        rtnl_unlock();
    }

    /* net device initialize */
    netif_carrier_off(prGlueInfo->prP2PInfo[0]->prDevHandler);
    netif_tx_stop_all_queues(prGlueInfo->prP2PInfo[0]->prDevHandler);

    /* register for net device */
    if (register_netdev(prGlueInfo->prP2PInfo[0]->prDevHandler) < 0) {
        DBGLOG(INIT, WARN, "unable to register netdevice for p2p[0]\n");

        free_netdev(prGlueInfo->prP2PInfo[0]->prDevHandler);

        ret = false;
    } else {
        prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_REGISTERED;
        gPrP2pDev[0] = prGlueInfo->prP2PInfo[0]->prDevHandler;
#if 0
        prGlueInfo->prP2PInfo[0]->fgIsNetDevRegistered = true;
#endif
        ret = true;
    }

    if (prAdapter->prP2pInfo->u4DeviceNum == KAL_P2P_NUM) {
        /* net device initialize */
        netif_carrier_off(prGlueInfo->prP2PInfo[1]->prDevHandler);
        netif_tx_stop_all_queues(prGlueInfo->prP2PInfo[1]->prDevHandler);

        /* register for net device */
        if (register_netdev(prGlueInfo->prP2PInfo[1]->prDevHandler) < 0) {
            DBGLOG(INIT, WARN, "unable to register netdevice for p2p[1]\n");

            free_netdev(prGlueInfo->prP2PInfo[1]->prDevHandler);

            ret = false;
        } else {
            prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_REGISTERED;
            gPrP2pDev[1] = prGlueInfo->prP2PInfo[1]->prDevHandler;
#if 0
            DBGLOG(P2P, STATE, "P2P 2nd NetDev registered\n");
            prGlueInfo->prP2PInfo[1]->fgIsNetDevRegistered = true;
#else
            ret = true;
#endif
        }

        DBGLOG(P2P, INFO, "P2P 2nd interface work\n");
    }
    if (fgRollbackRtnlLock) {
        rtnl_lock();
    }

    return ret;
}

u8 p2pNetUnregister(P_GLUE_INFO_T prGlueInfo, u8 fgIsRtnlLockAcquired)
{
    u8 fgDoUnregister = false;
    u8 fgRollbackRtnlLock = false;
    u8 ucRoleIdx;
    struct net_device *prRoleDev = NULL;
    int iftype = 0;
    P_BSS_INFO_T prP2pBssInfo = NULL;
    P_NETDEV_PRIVATE_GLUE_INFO prNetDevPriv = NULL;
    P_ADAPTER_T prAdapter = NULL;
#if CFG_ENABLE_UNIFY_WIPHY
    P_GL_P2P_INFO_T prP2PInfo = NULL;
#endif

    GLUE_SPIN_LOCK_DECLARATION();

    prAdapter = prGlueInfo->prAdapter;

    ASSERT(prGlueInfo);
    ASSERT(prAdapter);

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
    if (prAdapter->rP2PNetRegState == ENUM_NET_REG_STATE_REGISTERED &&
        prAdapter->rP2PRegState == ENUM_P2P_REG_STATE_REGISTERED) {
        prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_UNREGISTERING;
        fgDoUnregister = true;
    }
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

    if (!fgDoUnregister) {
        return true;
    }

#if CFG_ENABLE_UNIFY_WIPHY
    if (fgIsRtnlLockAcquired && rtnl_is_locked()) {
        fgRollbackRtnlLock = true;
    }
    for (ucRoleIdx = 0; ucRoleIdx < KAL_P2P_NUM; ucRoleIdx++) {
        prP2PInfo = prGlueInfo->prP2PInfo[ucRoleIdx];
        if (prP2PInfo == NULL) {
            continue;
        }
        /* don't unregister the dev that share with the AIS */
        if (prP2PInfo->prDevHandler == gprWdev->netdev) {
            continue;
        }
        prRoleDev = prP2PInfo->aprRoleHandler;
        if (prRoleDev != NULL) {
            /* info cfg80211 disconnect */
            prNetDevPriv = (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(prRoleDev);
            iftype = prRoleDev->ieee80211_ptr->iftype;
            prP2pBssInfo =
                GET_BSS_INFO_BY_INDEX(prAdapter, prNetDevPriv->ucBssIdx);
            /* FIXME: The p2pRoleFsmUninit may call the
             * cfg80211_disconnected.
             * p2pRemove()->glUnregisterP2P->p2pRoleFsmUninit(),
             * it may be too late.
             */
            if ((prP2pBssInfo != NULL) &&
                (prP2pBssInfo->eConnectionState ==
                 PARAM_MEDIA_STATE_CONNECTED) &&
                ((iftype == NL80211_IFTYPE_P2P_CLIENT) ||
                 (iftype == NL80211_IFTYPE_STATION))) {
                cfg80211_disconnected(prRoleDev, 0, NULL, 0, true, GFP_KERNEL);
            }
            if (prRoleDev != prP2PInfo->prDevHandler) {
                if (netif_carrier_ok(prRoleDev)) {
                    netif_carrier_off(prRoleDev);
                }
                netif_tx_stop_all_queues(prRoleDev);
            }
        }
        if (netif_carrier_ok(prP2PInfo->prDevHandler)) {
            netif_carrier_off(prP2PInfo->prDevHandler);
        }
        netif_tx_stop_all_queues(prP2PInfo->prDevHandler);
        if (fgRollbackRtnlLock) {
            rtnl_unlock();
        }
        /* Here are functions which need rtnl_lock */
        if ((prRoleDev) && (prP2PInfo->prDevHandler != prRoleDev)) {
            DBGLOG(INIT, INFO, "unregister p2p[%d]\n", ucRoleIdx);
            unregister_netdev(prRoleDev);
            /* This ndev is created in mtk_p2p_cfg80211_add_iface(),
             * and unregister_netdev will also free the ndev.
             */
        }
        DBGLOG(INIT, INFO, "unregister p2pdev[%d]\n", ucRoleIdx);
        unregister_netdev(prP2PInfo->prDevHandler);
        if (fgRollbackRtnlLock) {
            rtnl_lock();
        }
    }
#else  /* CFG_ENABLE_UNIFY_WIPHY */

    /* prepare for removal */
    if (prGlueInfo->prP2PInfo[0]->prDevHandler !=
        prGlueInfo->prP2PInfo[0]->aprRoleHandler) {
        prRoleDev = prGlueInfo->prP2PInfo[0]->aprRoleHandler;

        if (prRoleDev != NULL) {
            /* info cfg80211 disconnect */
            prNetDevPriv = (NETDEV_PRIVATE_GLUE_INFO *)netdev_priv(prRoleDev);
            iftype = prRoleDev->ieee80211_ptr->iftype;
            prP2pBssInfo =
                GET_BSS_INFO_BY_INDEX(prAdapter, prNetDevPriv->ucBssIdx);

            /* p2pRoleFsmUninit may call cfg80211_disconnected.
             * p2pRemove()->glUnregisterP2P->p2pRoleFsmUninit(),
             * and it may be too late to call cfg80211_disconnected
             * there
             */

            if ((prP2pBssInfo != NULL) &&
                (prP2pBssInfo->eConnectionState ==
                 PARAM_MEDIA_STATE_CONNECTED) &&
                ((iftype == NL80211_IFTYPE_P2P_CLIENT) ||
                 (iftype == NL80211_IFTYPE_STATION))) {
                cfg80211_disconnected(prRoleDev, 0, NULL, 0, true, GFP_KERNEL);
            }
        }

        if (netif_carrier_ok(prGlueInfo->prP2PInfo[0]->aprRoleHandler)) {
            netif_carrier_off(prGlueInfo->prP2PInfo[0]->aprRoleHandler);
        }

        netif_tx_stop_all_queues(prGlueInfo->prP2PInfo[0]->aprRoleHandler);
    }

    if (netif_carrier_ok(prGlueInfo->prP2PInfo[0]->prDevHandler)) {
        netif_carrier_off(prGlueInfo->prP2PInfo[0]->prDevHandler);
    }

    netif_tx_stop_all_queues(prGlueInfo->prP2PInfo[0]->prDevHandler);

    if (fgIsRtnlLockAcquired && rtnl_is_locked()) {
        fgRollbackRtnlLock = true;
        rtnl_unlock();
    }
    /* Here are functions which need rtnl_lock */
    if (prGlueInfo->prP2PInfo[0]->prDevHandler !=
        prGlueInfo->prP2PInfo[0]->aprRoleHandler) {
        DBGLOG(INIT, INFO, "unregister p2p[0]\n");
        unregister_netdev(prGlueInfo->prP2PInfo[0]->aprRoleHandler);
    }
#if 0
    if (prGlueInfo->prP2PInfo[0]->fgIsNetDevRegistered == true) {
        prGlueInfo->prP2PInfo[0]->fgIsNetDevRegistered = false;
#endif
    DBGLOG(INIT, INFO, "unregister p2pdev[0]\n");
    unregister_netdev(prGlueInfo->prP2PInfo[0]->prDevHandler);
#if 0
}
#endif

    /* unregister the netdev and index > 0 */
    if (prAdapter->prP2pInfo->u4DeviceNum >= 2) {
        for (ucRoleIdx = 1; ucRoleIdx < BSS_P2P_NUM; ucRoleIdx++) {
            /* prepare for removal */
            if (netif_carrier_ok(
                    prGlueInfo->prP2PInfo[ucRoleIdx]->prDevHandler)) {
                netif_carrier_off(
                    prGlueInfo->prP2PInfo[ucRoleIdx]->prDevHandler);
            }

            netif_tx_stop_all_queues(
                prGlueInfo->prP2PInfo[ucRoleIdx]->prDevHandler);

            if (fgIsRtnlLockAcquired && rtnl_is_locked()) {
                fgRollbackRtnlLock = true;
                rtnl_unlock();
            }
            /* Here are functions which need rtnl_lock */
#if 0
            if (prGlueInfo->prP2PInfo[ucRoleIdx]
                ->fgIsNetDevRegistered == true) {
                prGlueInfo->prP2PInfo[ucRoleIdx]
                ->fgIsNetDevRegistered = false;
                DBGLOG(INIT, INFO, "unregister p2pdev[%d]\n",
                       ucRoleIdx);
#endif
            unregister_netdev(prGlueInfo->prP2PInfo[ucRoleIdx]->prDevHandler);
#if 0
        }
#endif
        }
    }

    if (fgRollbackRtnlLock) {
        rtnl_lock();
    }

#endif  /* CFG_ENABLE_UNIFY_WIPHY */

    prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_UNREGISTERED;

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Setup the P2P device information
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *       [in] prP2pWdev       Pointer to the wireless device
 *       [in] prP2pDev        Pointer to the net device
 *       [in] u4Idx           The P2P Role index (max : (KAL_P2P_NUM-1))
 *       [in] fgIsApMode      Indicate that this device is AP Role or not
 *
 * \return    0	Success
 *           -1	Failure
 */
/*----------------------------------------------------------------------------*/
#if CFG_ENABLE_UNIFY_WIPHY
int glSetupP2P(P_GLUE_INFO_T prGlueInfo, struct wireless_dev *prP2pWdev,
               struct net_device *prP2pDev, int u4Idx, u8 fgIsApMode)
{
    P_ADAPTER_T prAdapter = NULL;
    P_GL_P2P_INFO_T prP2PInfo = NULL;
    P_GL_HIF_INFO_T prHif = NULL;
    P_NETDEV_PRIVATE_GLUE_INFO prNetDevPriv = NULL;
    struct chip_info *prChipInfo = NULL;
    DBGLOG(INIT, INFO, "setup the p2p dev\n");
    prHif = &prGlueInfo->rHifInfo;
    prAdapter = prGlueInfo->prAdapter;
    if ((prGlueInfo == NULL) || (prAdapter == NULL) || (prHif == NULL) ||
        (prP2pWdev == NULL) || (prP2pWdev->wiphy == NULL) ||
        (prP2pDev == NULL)) {
        DBGLOG(INIT, ERROR, "parameter is NULL!!\n");
        return -1;
    }
    /* FIXME: check KAL_P2P_NUM in trunk? */
    if (u4Idx >= KAL_P2P_NUM) {
        DBGLOG(INIT, ERROR, "u4Idx(%d) is out of range!!\n", u4Idx);
        return -1;
    }
    prChipInfo = prAdapter->chip_info;
    /*0. allocate p2pinfo */
    if (!p2PAllocInfo(prGlueInfo, u4Idx)) {
        DBGLOG(INIT, WARN, "Allocate memory for p2p FAILED\n");
        ASSERT(0);
        return -1;
    }
    prP2PInfo = prGlueInfo->prP2PInfo[u4Idx];
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
    /* fill wiphy parameters */
    prP2PInfo->prWdev = prP2pWdev;
    if (!prAdapter->fgEnable5GBand) {
        prP2pWdev->wiphy->bands[BAND_5G] = NULL;
    }
#endif  /* CFG_ENABLE_WIFI_DIRECT_CFG_80211 */
        /* setup netdev */
        /* Point to shared glue structure */
    prNetDevPriv = (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(prP2pDev);
    prNetDevPriv->prGlueInfo = prGlueInfo;
    /* set ucIsP2p for P2P function device */
    if (fgIsApMode == true) {
        prP2pWdev->iftype = NL80211_IFTYPE_AP;
        prNetDevPriv->ucIsP2p = false;
    } else {
        prP2pWdev->iftype = NL80211_IFTYPE_P2P_CLIENT;
        prNetDevPriv->ucIsP2p = true;
    }
    /* register callback functions */

    prP2pDev->needed_headroom +=
        NIC_TX_DESC_AND_PADDING_LENGTH +
        44  /* yhpgi: prChipInfo->txd_append_size */;

    prP2pDev->netdev_ops = &p2p_netdev_ops;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
    prP2PInfo->prDevHandler->wireless_handlers = &mtk_p2p_wext_handler_def;
#endif
    // #if defined(_HIF_SDIO)
    // #if (MTK_WCN_HIF_SDIO == 0)
    SET_NETDEV_DEV(prP2pDev, &(prHif->func->dev));
// #endif
// #endif
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
    prP2pDev->ieee80211_ptr = prP2pWdev;
    prP2pWdev->netdev = prP2pDev;
#endif
#if CFG_TCP_IP_CHKSUM_OFFLOAD
    /* set HW checksum offload */
    if (prAdapter->fgIsSupportCsumOffload) {
        prP2pDev->features = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
                             NETIF_F_RXCSUM;
    }
#endif  /* CFG_TCP_IP_CHKSUM_OFFLOAD */
    kalResetStats(prP2pDev);
    /* finish */
    /* bind netdev pointer to netdev index */
    prP2PInfo->prDevHandler = prP2pDev;
    /* XXX: All the P2P/AP devices do p2pDevFsmInit in the original code */
    wlanBindBssIdxToNetInterface(prGlueInfo, p2pDevFsmInit(prAdapter),
                                 (void *)prP2PInfo->prDevHandler);
    prP2PInfo->aprRoleHandler = prP2PInfo->prDevHandler;
    DBGLOG(P2P, INFO, "check prDevHandler = %p\n", prP2PInfo->prDevHandler);
    DBGLOG(P2P, INFO, "aprRoleHandler = %p\n", prP2PInfo->aprRoleHandler);
    prNetDevPriv->ucBssIdx = p2pRoleFsmInit(prAdapter, u4Idx);
    /* Currently wpasupplicant can't support create interface. */
    /* so initial the corresponding data structure here. */
    wlanBindBssIdxToNetInterface(prGlueInfo, prNetDevPriv->ucBssIdx,
                                 (void *)prP2PInfo->aprRoleHandler);
    /* bind netdev pointer to netdev index */
#if 0
    wlanBindNetInterface(prGlueInfo, NET_DEV_P2P_IDX,
                         (void *)prGlueInfo->prP2PInfo->prDevHandler);
#endif
    /* setup running mode */
    p2pFuncInitConnectionSettings(
        prAdapter, prAdapter->rWifiVar.prP2PConnSettings[u4Idx], fgIsApMode);
    return 0;
}
#endif  /* CFG_ENABLE_UNIFY_WIPHY */

/*---------------------------------------------------------------------------*/
/*!
 * \brief Register for cfg80211 for Wi-Fi Direct
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   true
 *           false
 */
/*---------------------------------------------------------------------------*/
u8 glRegisterP2P(P_GLUE_INFO_T prGlueInfo, const char *prDevName,
                 const char *prDevName2, u8 ucApMode)
{
    P_ADAPTER_T prAdapter = NULL;
    PARAM_MAC_ADDRESS rMacAddr;
    u8 fgIsApMode = false;
    u8 ucRegisterNum = 1, i = 0;
    struct wireless_dev *prP2pWdev = NULL;
#if CFG_ENABLE_UNIFY_WIPHY
    struct net_device *prP2pDev = NULL;
    struct wiphy *prWiphy = NULL;
#else  /* (CFG_ENABLE_UNIFY_WIPHY == 0) */
    P_GL_HIF_INFO_T prHif = NULL;
    P_NETDEV_PRIVATE_GLUE_INFO prNetDevPriv = NULL;
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
    struct device *prDev;
#endif
#endif
    const char *prSetDevName;

    ASSERT(prGlueInfo);

    prAdapter = prGlueInfo->prAdapter;
    ASSERT(prAdapter);

#if CFG_ENABLE_UNIFY_WIPHY
    if ((ucApMode == RUNNING_DUAL_AP_MODE) ||
        (ucApMode == RUNNING_P2P_AP_MODE)) {
        ucRegisterNum = 2;
        glP2pCreateWirelessDevice(prGlueInfo);
    }
    do {
        if (ucApMode == RUNNING_P2P_AP_MODE) {
            if (i == 0) {
                prSetDevName = prDevName;
                fgIsApMode = false;
            } else {
                prSetDevName = prDevName2;
                fgIsApMode = true;
            }
        } else {
            /* RUNNING_AP_MODE
             * RUNNING_DUAL_AP_MODE
             * RUNNING_P2P_MODE
             */
            prSetDevName = prDevName;
            if (ucApMode == RUNNING_P2P_MODE) {
                fgIsApMode = false;
            }else{
                fgIsApMode = true;
            }
        }
        if (!gprP2pRoleWdev[i]) {
            DBGLOG(P2P, ERROR, "gprP2pRoleWdev[%d] is NULL\n", i);
            return false;
        }
        prP2pWdev = gprP2pRoleWdev[i];
        DBGLOG(INIT, INFO, "glRegisterP2P(%d)\n", i);
        /* Reset prP2pWdev for the issue that the prP2pWdev doesn't
         * reset when the usb unplug/plug.
         */
        prWiphy = prP2pWdev->wiphy;
        memset(prP2pWdev, 0, sizeof(struct wireless_dev));
        prP2pWdev->wiphy = prWiphy;
        /* allocate netdev */

        prP2pDev = alloc_netdev_mq(sizeof(NETDEV_PRIVATE_GLUE_INFO),
                                   prSetDevName, NET_NAME_PREDICTABLE,
                                   ether_setup, CFG_MAX_TXQ_NUM);

        if (!prP2pDev) {
            DBGLOG(INIT, WARN, "unable to allocate ndev for p2p\n");
            goto err_alloc_netdev;
        }
        /* fill hardware address */
        COPY_MAC_ADDR(rMacAddr, prAdapter->rMyMacAddr);
        rMacAddr[0] |= 0x2;
        /* change to local administrated address */
        rMacAddr[0] ^= i << 2;

#if CFG_MESON_G12A_PATCH
        prP2pDev->mtu = 1408;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
        dev_addr_set(prP2pDev, rMacAddr);
#else
        kalMemCopy(prP2pDev->dev_addr, rMacAddr, ETH_ALEN);
#endif
        kalMemCopy(prP2pDev->perm_addr, prP2pDev->dev_addr, ETH_ALEN);
        if (glSetupP2P(prGlueInfo, prP2pWdev, prP2pDev, i, fgIsApMode) != 0) {
            DBGLOG(INIT, WARN, "glSetupP2P FAILED\n");
            free_netdev(prP2pDev);
            return false;
        }
        i++;
        /* prP2pInfo is alloc at glSetupP2P()->p2PAllocInfo() */
        prAdapter->prP2pInfo->u4DeviceNum++;
        /* set p2p net device register state */
        /* p2pNetRegister() will check prAdapter->rP2PNetRegState. */
        prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_UNREGISTERED;
    } while (i < ucRegisterNum);
#else  /* (CFG_ENABLE_UNIFY_WIPHY == 0) */

    prHif = &prGlueInfo->rHifInfo;
    ASSERT(prHif);

    if ((ucApMode == RUNNING_AP_MODE) || (ucApMode == RUNNING_DUAL_AP_MODE ||
                                          (ucApMode == RUNNING_P2P_AP_MODE))) {
        fgIsApMode = true;
        if ((ucApMode == RUNNING_DUAL_AP_MODE) ||
            (ucApMode == RUNNING_P2P_AP_MODE)) {
            ucRegisterNum = 2;
            if (gprP2pRoleWdev[1] == NULL) {
                /* Create device only when not created before. */
                DBGLOG(
                    P2P, ERROR,
                    "Might encounter deadlock creating wireless device here\n");
                glP2pCreateWirelessDevice(prGlueInfo);
                /* There would be risk encouter deadlock here. */
            }
        }
    }
    prSetDevName = prDevName;

    do {
        if ((ucApMode == RUNNING_P2P_AP_MODE) && (i == 0)) {
            prSetDevName = prDevName;
            fgIsApMode = false;
        } else if ((ucApMode == RUNNING_P2P_AP_MODE) && (i == 1)) {
            prSetDevName = prDevName2;
            fgIsApMode = true;
        }
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
        if (!gprP2pRoleWdev[i]) {
            DBGLOG(P2P, ERROR, "gl_p2p, wireless device is not exist\n");
            return false;
        }
#endif
        prP2pWdev = gprP2pRoleWdev[i];
        DBGLOG(INIT, INFO, "glRegisterP2P(%d), fgIsApMode(%d)\n", i,
               fgIsApMode);
        /*0. allocate p2pinfo */
        if (!p2PAllocInfo(prGlueInfo, i)) {
            DBGLOG(INIT, WARN, "Allocate memory for p2p FAILED\n");
            ASSERT(0);
            return false;
        }
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
        /* 1.1 fill wiphy parameters */
        glGetHifDev(prHif, &prDev);
        if (!prDev) {
            DBGLOG(INIT, INFO, "unable to get struct dev for p2p\n");
        }

        prGlueInfo->prP2PInfo[i]->prWdev = prP2pWdev;
        /*prGlueInfo->prP2PInfo[i]->prRoleWdev[0] = prP2pWdev;*/
        /* TH3
         * multiple
         * P2P
         */

        ASSERT(prP2pWdev);
        ASSERT(prP2pWdev->wiphy);
        ASSERT(prDev);
        ASSERT(prGlueInfo->prAdapter);

        set_wiphy_dev(prP2pWdev->wiphy, prDev);

        if (!prGlueInfo->prAdapter->fgEnable5GBand) {
            prP2pWdev->wiphy->bands[BAND_5G] = NULL;
        }

        /* 2.2 wdev initialization */
        if (fgIsApMode) {
            prP2pWdev->iftype = NL80211_IFTYPE_AP;
        } else {
            prP2pWdev->iftype = NL80211_IFTYPE_P2P_CLIENT;
        }
#endif

        /* 3. allocate netdev */
        prGlueInfo->prP2PInfo[i]->prDevHandler =
            alloc_netdev_mq(sizeof(NETDEV_PRIVATE_GLUE_INFO), prSetDevName,
                            NET_NAME_PREDICTABLE, ether_setup, CFG_MAX_TXQ_NUM);

        if (!prGlueInfo->prP2PInfo[i]->prDevHandler) {
            DBGLOG(INIT, WARN, "unable to allocate netdevice for p2p\n");

            goto err_alloc_netdev;
        }

        /* 4. setup netdev */
        /* 4.1 Point to shared glue structure */
        /**((P_GLUE_INFO_T *)
         * netdev_priv(prGlueInfo->prP2PInfo->prDevHandler)) =
         * prGlueInfo; */
        prNetDevPriv = (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(
            prGlueInfo->prP2PInfo[i]->prDevHandler);
        prNetDevPriv->prGlueInfo = prGlueInfo;

        /* 4.2 fill hardware address */
        COPY_MAC_ADDR(rMacAddr, prAdapter->rMyMacAddr);
        rMacAddr[0] |= 0x2;
        rMacAddr[0] ^= i << 2;  /* change to local administrated address
                                 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
        kalMemCopy(prGlueInfo->prP2PInfo[i]->prDevHandler->dev_addr, rMacAddr,
                   ETH_ALEN);
#else
        dev_addr_set(prGlueInfo->prP2PInfo[i]->prDevHandler, rMacAddr);
#endif

        kalMemCopy(prGlueInfo->prP2PInfo[i]->prDevHandler->perm_addr,
                   prGlueInfo->prP2PInfo[i]->prDevHandler->dev_addr, ETH_ALEN);
#if CFG_MESON_G12A_PATCH
        prGlueInfo->prP2PInfo[i]->prDevHandler->mtu = 1408;
#endif

        /* 4.3 register callback functions */
        prGlueInfo->prP2PInfo[i]->prDevHandler->needed_headroom +=
            NIC_TX_HEAD_ROOM;
        prGlueInfo->prP2PInfo[i]->prDevHandler->netdev_ops = &p2p_netdev_ops;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
        prGlueInfo->prP2PInfo->prDevHandler->wireless_handlers =
            &mtk_p2p_wext_handler_def;
#endif

        SET_NETDEV_DEV(prGlueInfo->prP2PInfo[i]->prDevHandler,
                       &(prHif->func->dev));

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
        prGlueInfo->prP2PInfo[i]->prDevHandler->ieee80211_ptr = prP2pWdev;
        prP2pWdev->netdev = prGlueInfo->prP2PInfo[i]->prDevHandler;
#endif

#if CFG_TCP_IP_CHKSUM_OFFLOAD
        /* set HW checksum offload */
        if (prAdapter->fgIsSupportCsumOffload) {
            prGlueInfo->prP2PInfo[i]->prDevHandler->features =
                NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM;
        }
#endif

        kalResetStats(prGlueInfo->prP2PInfo[i]->prDevHandler);

        /* 8. set p2p net device register state */
        prGlueInfo->prAdapter->rP2PNetRegState =
            ENUM_NET_REG_STATE_UNREGISTERED;

        /* 9. setup running mode */
        /* prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo->fgIsApMode =
         * fgIsApMode; */

        /* 10. finish */
        /* 13. bind netdev pointer to netdev index */
        wlanBindBssIdxToNetInterface(
            prGlueInfo, p2pDevFsmInit(prAdapter),
            (void *)prGlueInfo->prP2PInfo[i]->prDevHandler);

        prGlueInfo->prP2PInfo[i]->aprRoleHandler =
            prGlueInfo->prP2PInfo[i]->prDevHandler;

        DBGLOG(P2P, INFO, "check prDevHandler = %p\n",
               prGlueInfo->prP2PInfo[i]->prDevHandler);
        DBGLOG(P2P, INFO, "aprRoleHandler = %p\n",
               prGlueInfo->prP2PInfo[i]->aprRoleHandler);

        prNetDevPriv->ucBssIdx = p2pRoleFsmInit(prAdapter, i);
        /* 11. Currently wpasupplicant can't support create interface.
         */
        /* so initial the corresponding data structure here. */
        wlanBindBssIdxToNetInterface(
            prGlueInfo, prNetDevPriv->ucBssIdx,
            (void *)prGlueInfo->prP2PInfo[i]->aprRoleHandler);

        /* 13. bind netdev pointer to netdev index */
        /* wlanBindNetInterface(prGlueInfo, NET_DEV_P2P_IDX, (void
         **)prGlueInfo->prP2PInfo->prDevHandler); */

        /* 12. setup running mode */
        p2pFuncInitConnectionSettings(
            prAdapter, prAdapter->rWifiVar.prP2PConnSettings[i], fgIsApMode);

        /* Active network too early would cause HW not able to sleep.
         * Defer the network active time.
         */
        /* nicActivateNetwork(prAdapter, NETWORK_TYPE_P2P_INDEX); */

        i++;
    } while (i < ucRegisterNum);

    if ((ucApMode == RUNNING_DUAL_AP_MODE) ||
        (ucApMode == RUNNING_P2P_AP_MODE)) {
        prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum = 2;
    } else {
        prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum = 1;
    }

#endif  /* CFG_ENABLE_UNIFY_WIPHY */
    return true;

err_alloc_netdev:
    return false;
}

#if CFG_ENABLE_UNIFY_WIPHY
u8 glP2pCreateWirelessDevice(P_GLUE_INFO_T prGlueInfo)
{
    struct wiphy *prWiphy = gprWdev->wiphy;
    struct wireless_dev *prWdev = NULL;
    u8 i = 0;

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
    if (!prWiphy) {
        DBGLOG(P2P, ERROR, "unable to allocate wiphy for p2p\n");
        return false;
    }
    for (i = 0; i < KAL_P2P_NUM; i++)
        if (!gprP2pRoleWdev[i]) {
            break;
        }
    if (i >= KAL_P2P_NUM) {
        DBGLOG(INIT, WARN, "fail to register wiphy to driver\n");
        return false;
    }
    prWdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
    if (!prWdev) {
        DBGLOG(P2P, ERROR, "allocate p2p wdev fail, no memory\n");
        return false;
    }
    /* set priv as pointer to glue structure */
    prWdev->wiphy = prWiphy;
    gprP2pRoleWdev[i] = prWdev;
    DBGLOG(INIT, INFO, "glP2pCreateWirelessDevice (%x)\n",
           gprP2pRoleWdev[i]->wiphy);
    return true;
#else
    return false;
#endif
}
#else  /* (CFG_ENABLE_UNIFY_WIPHY == 0) */
u8 glP2pCreateWirelessDevice(P_GLUE_INFO_T prGlueInfo)
{
    struct wiphy *prWiphy = NULL;
    struct wireless_dev *prWdev = NULL;
    u8 i = 0;
    u32 band_idx, ch_idx;
    struct ieee80211_supported_band *sband = NULL;
    struct ieee80211_channel *chan = NULL;

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
    prWdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
    if (!prWdev) {
        DBGLOG(P2P, ERROR, "allocate p2p wireless device fail, no memory\n");
        return false;
    }
    /* 1. allocate WIPHY */
    prWiphy = wiphy_new(&mtk_p2p_ops, sizeof(P_GLUE_INFO_T));
    if (!prWiphy) {
        DBGLOG(P2P, ERROR, "unable to allocate wiphy for p2p\n");
        goto free_wdev;
    }

    prWiphy->interface_modes =
        BIT(NL80211_IFTYPE_AP) | BIT(NL80211_IFTYPE_P2P_CLIENT) |
        BIT(NL80211_IFTYPE_P2P_GO) | BIT(NL80211_IFTYPE_STATION);

    prWiphy->software_iftypes |= BIT(NL80211_IFTYPE_P2P_DEVICE);

    prWiphy->iface_combinations = p_mtk_iface_combinations_p2p;
    prWiphy->n_iface_combinations = mtk_iface_combinations_p2p_num;

    prWiphy->bands[NL80211_BAND_2GHZ] = &mtk_band_2ghz;
    prWiphy->bands[NL80211_BAND_5GHZ] = &mtk_band_5ghz;

    /*
     * Clear flags in ieee80211_channel before p2p registers to resolve
     * overriding flags issue. For example, when country is changed to US,
     * both WW and US flags are applied. The issue flow is:
     *
     * 1. Register wlan wiphy (wiphy_register()@core.c)
     *    chan->orig_flags = chan->flags = 0
     * 2. Apply WW regdomain (handle_channel()@reg.c):
     *    chan->flags = chan->orig_flags|reg_channel_flags =
     * 0|WW_channel_flags
     * 3. Register p2p wiphy:
     *    chan->orig_flags = chan->flags = WW channel flags
     * 4. Apply US regdomain:
     *    chan->flags = chan->orig_flags|reg_channel_flags
     *                = WW_channel_flags|US_channel_flags
     *                  (Unexpected! It includes WW flags)
     */

    for (band_idx = 0; band_idx < NUM_NL80211_BANDS; band_idx++) {
        sband = prWiphy->bands[band_idx];
        if (!sband) {
            continue;
        }
        for (ch_idx = 0; ch_idx < sband->n_channels; ch_idx++) {
            chan = &sband->channels[ch_idx];
            chan->flags = 0;
        }
    }

    prWiphy->mgmt_stypes = mtk_cfg80211_default_mgmt_stypes;
    prWiphy->max_remain_on_channel_duration = 5000;
    prWiphy->n_cipher_suites = 5;
    prWiphy->cipher_suites = mtk_cipher_suites;
    prWiphy->flags = WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL | WIPHY_FLAG_HAVE_AP_SME |
                     WIPHY_FLAG_HAS_CHANNEL_SWITCH;
    prWiphy->max_num_csa_counters = MAX_CSA_COUNTER;
    prWiphy->ap_sme_capa = 1;
    prWiphy->max_scan_ssids = MAX_SCAN_LIST_NUM;
    prWiphy->max_scan_ie_len = MAX_SCAN_IE_LEN;
    prWiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
    prWiphy->iface_combinations = p2p_iface_comb_mcc;
    prWiphy->n_iface_combinations = ARRAY_SIZE(p2p_iface_comb_mcc);
    prWiphy->max_num_csa_counters = 2;

    prWiphy->regulatory_flags = REGULATORY_CUSTOM_REG;
    cfg80211_regd_set_wiphy(prWiphy);

    /* 2.1 set priv as pointer to glue structure */
    *((P_GLUE_INFO_T *)wiphy_priv(prWiphy)) = prGlueInfo;
    /* Here are functions which need rtnl_lock */
    if (wiphy_register(prWiphy) < 0) {
        DBGLOG(INIT, WARN, "fail to register wiphy for p2p\n");
        goto free_wiphy;
    }
    prWdev->wiphy = prWiphy;

    for (i = 0; i < KAL_P2P_NUM; i++) {
        if (!gprP2pRoleWdev[i]) {
            gprP2pRoleWdev[i] = prWdev;
            DBGLOG(INIT, INFO, "glP2pCreateWirelessDevice (%x)\n",
                   gprP2pRoleWdev[i]->wiphy);
            break;
        }
    }

    if (i == KAL_P2P_NUM) {
        DBGLOG(INIT, WARN, "fail to register wiphy to driver\n");
        goto free_wiphy;
    }

    return true;

free_wiphy:
    wiphy_free(prWiphy);
free_wdev:
    kfree(prWdev);
#endif
    return false;
}
#endif  /* CFG_ENABLE_UNIFY_WIPHY */

#if (CFG_ENABLE_UNIFY_WIPHY == 0)
void glP2pDestroyWirelessDevice(void)
{
    int i = 0;

    set_wiphy_dev(gprP2pWdev->wiphy, NULL);

    wiphy_unregister(gprP2pWdev->wiphy);
    wiphy_free(gprP2pWdev->wiphy);
    kfree(gprP2pWdev);

    for (i = 0; i < KAL_P2P_NUM; i++) {
        if (gprP2pRoleWdev[i] == NULL) {
            continue;
        }

        if (i != 0) {  /* The P2P is always in index 0 and shares Wiphy
                        * with P2PWdev */
            DBGLOG(INIT, INFO, "glP2pDestroyWirelessDevice (%p)\n",
                   gprP2pRoleWdev[i]->wiphy);
            set_wiphy_dev(gprP2pRoleWdev[i]->wiphy, NULL);
            wiphy_unregister(gprP2pRoleWdev[i]->wiphy);
            wiphy_free(gprP2pRoleWdev[i]->wiphy);
        }
        if (gprP2pRoleWdev[i] && (gprP2pWdev != gprP2pRoleWdev[i])) {
            kfree(gprP2pRoleWdev[i]);
        }
        gprP2pRoleWdev[i] = NULL;
    }

    gprP2pWdev = NULL;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Unregister Net Device for Wi-Fi Direct
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *	[in] ucIdx	     The BSS with the idx will be freed.
 *			     "ucIdx == 0xff" will free all BSSs.
 *			     Only has meaning for "CFG_ENABLE_UNIFY_WIPHY == 1"
 *
 * \return   true
 *           false
 */
/*----------------------------------------------------------------------------*/
u8 glUnregisterP2P(P_GLUE_INFO_T prGlueInfo, u8 ucIdx)
#if CFG_ENABLE_UNIFY_WIPHY
{
    u8 ucRoleIdx;
    P_ADAPTER_T prAdapter;
    P_GL_P2P_INFO_T prP2PInfo = NULL;
    int i4Start = 0, i4End = 0;
    ASSERT(prGlueInfo);
    if (ucIdx == 0xff) {
        i4Start = 0;
        i4End = BSS_P2P_NUM;
    } else if (ucIdx < BSS_P2P_NUM) {
        i4Start = ucIdx;
        i4End = ucIdx + 1;
    } else {
        DBGLOG(INIT, WARN, "The ucIdx (%d) is a wrong value\n", ucIdx);
        return false;
    }
    prAdapter = prGlueInfo->prAdapter;
    /* 4 <1> Uninit P2P dev FSM */
    /* Uninit P2P device FSM */
    /* only do p2pDevFsmUninit, when unregister all P2P device */
    if (ucIdx == 0xff) {
        p2pDevFsmUninit(prAdapter);
    }
    /* 4 <2> Uninit P2P role FSM */
    for (ucRoleIdx = i4Start; ucRoleIdx < i4End; ucRoleIdx++) {
        if (P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx)) {
            /* FIXME: The cfg80211_XXX() is following the
             * p2pRoleFsmUninit() sub-progress.
             * ex: The cfg80211_del_sta() is called in the
             *     kalP2PGOStationUpdate().
             * But the netdev had be unregistered at
             * p2pNetUnregister(). EXCEPTION!!
             */
            p2pRoleFsmUninit(prGlueInfo->prAdapter, ucRoleIdx);
        }
    }
    /* 4 <3> Free Wiphy & netdev */
    for (ucRoleIdx = i4Start; ucRoleIdx < i4End; ucRoleIdx++) {
        prP2PInfo = prGlueInfo->prP2PInfo[ucRoleIdx];
        if (prP2PInfo == NULL) {
            continue;
        }
        /* For P2P interfaces, prDevHandler points to the net_device of
         * p2p0 interface. And aprRoleHandler points to the net_device
         * of p2p virtual interface (i.e., p2p1) when it was created.
         * And when p2p virtual interface is deleted, aprRoleHandler
         * will change to point to prDevHandler. Hence, when
         * aprRoleHandler & prDevHandler are pointing to different
         * addresses, it means vif p2p1 exists. Otherwise it means p2p1
         * was already deleted.
         */
        if ((prP2PInfo->aprRoleHandler != NULL) &&
            (prP2PInfo->aprRoleHandler != prP2PInfo->prDevHandler)) {
            /* This device is added by the P2P, and use
             * ndev->destructor to free. The p2pDevFsmUninit() use
             * prP2PInfo->aprRoleHandler to do some check.
             */
            prP2PInfo->aprRoleHandler = NULL;
            DBGLOG(P2P, INFO, "aprRoleHandler idx %d set NULL\n", ucRoleIdx);
            /* XXX: reference from mtk_p2p_cfg80211_del_iface */
            gprP2pRoleWdev[ucRoleIdx] = gprP2pWdev;
        }
        if (prP2PInfo->prDevHandler) {
            /* don't free the dev that share with the AIS */
            if (prP2PInfo->prDevHandler == gprWdev->netdev) {
                gprP2pRoleWdev[ucRoleIdx] = NULL;
            }else{
                free_netdev(prP2PInfo->prDevHandler);
            }
            prP2PInfo->prDevHandler = NULL;
        }
        /* 4 <4> Free P2P internal memory */
        if (!p2PFreeInfo(prGlueInfo, ucRoleIdx)) {
            /* false: (fgIsP2PRegistered!=false)||(ucRoleIdx err) */
            DBGLOG(INIT, ERROR, "p2PFreeInfo FAILED\n");
            ASSERT(0);
            return false;
        }
    }
    return true;
}  /* end of glUnregisterP2P() */
#else  /* (CFG_ENABLE_UNIFY_WIPHY == 0) */
{
    u8 ucRoleIdx;
    P_ADAPTER_T prAdapter;
    P_GL_P2P_INFO_T prP2PInfo;

    ASSERT(prGlueInfo);

    prAdapter = prGlueInfo->prAdapter;

    /* 4 <1> Uninit P2P dev FSM */
    /* Uninit P2P device FSM */
    p2pDevFsmUninit(prAdapter);

    /* 4 <2> Uninit P2P role FSM */
    for (ucRoleIdx = 0; ucRoleIdx < BSS_P2P_NUM; ucRoleIdx++)
        if (P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx)) {
            p2pRoleFsmUninit(prGlueInfo->prAdapter, ucRoleIdx);
        }

    /* 4 <3> Free Wiphy & netdev */
    for (ucRoleIdx = 0; ucRoleIdx < BSS_P2P_NUM; ucRoleIdx++) {
        prP2PInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

        if (prP2PInfo == NULL) {
            continue;
        }
        /* For P2P interfaces, prDevHandler points to the net_device of
         * p2p0 interface.            */
        /* And aprRoleHandler points to the net_device of p2p virtual
         * interface (i.e., p2p1)       */
        /* when it was created. And when p2p virtual interface is
         * deleted, aprRoleHandler will     */
        /* change to point to prDevHandler. Hence, when aprRoleHandler &
         * prDevHandler are pointing */
        /* to different addresses, it means vif p2p1 exists. Otherwise
         * it means p2p1 was           */
        /* already deleted. */
        if ((prP2PInfo->aprRoleHandler != NULL) &&
            (prP2PInfo->aprRoleHandler != prP2PInfo->prDevHandler)) {
            /* This device is added by the P2P, and use
             * ndev->destructor to free. */
            prP2PInfo->aprRoleHandler = NULL;
            DBGLOG(P2P, INFO, "aprRoleHandler idx %d set NULL\n", ucRoleIdx);
        }

        if (prP2PInfo->prDevHandler) {
            free_netdev(prP2PInfo->prDevHandler);
            prP2PInfo->prDevHandler = NULL;
        }
    }

    /* 4 <4> Free P2P internal memory */
    if (!p2PFreeInfo(prGlueInfo, 0xff)) {
        DBGLOG(INIT, WARN, "Free memory for p2p FAILED\n");
        ASSERT(0);
        return false;
    }

    return true;
}
#endif  /* CFG_ENABLE_UNIFY_WIPHY */

/* Net Device Hooks */
/*----------------------------------------------------------------------------*/
/*!
 * \brief A function for net_device open (ifup)
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0     The execution succeeds.
 * \retval < 0   The execution failed.
 */
/*----------------------------------------------------------------------------*/
static int p2pOpen(IN struct net_device *prDev)
{
    ASSERT(prDev);

    /* 2. carrier on & start TX queue */
    /*DFS todo 20161220_DFS*/
#if (CFG_SUPPORT_DFS_MASTER == 1)
    if (prDev->ieee80211_ptr->iftype != NL80211_IFTYPE_AP) {
        netif_carrier_on(prDev);
        netif_tx_start_all_queues(prDev);
    }
#else
    netif_carrier_on(prDev);
    netif_tx_start_all_queues(prDev);
#endif

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief A function for net_device stop (ifdown)
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0     The execution succeeds.
 * \retval < 0   The execution failed.
 */
/*----------------------------------------------------------------------------*/
static int p2pStop(IN struct net_device *prDev)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    P_GL_P2P_DEV_INFO_T prP2pGlueDevInfo = (P_GL_P2P_DEV_INFO_T)NULL;
#if (CFG_ENABLE_UNIFY_WIPHY == 0)
    u8 ucRoleIdx = 0;
    struct net_device *prTargetDev = NULL;
#endif
    struct cfg80211_scan_request *prScanRequest = NULL;
    /* P_MSG_P2P_FUNCTION_SWITCH_T prFuncSwitch; */

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prDev));
    ASSERT(prGlueInfo);

    prAdapter = prGlueInfo->prAdapter;
    ASSERT(prAdapter);

    /* XXX: The p2pStop may be triggered after the wlanRemove.	*/
    /*      And prGlueInfo->prP2PDevInfo is freed in p2PFreeInfo.	*/
    if (!prAdapter->fgIsP2PRegistered) {
        return -EFAULT;
    }

    prP2pGlueDevInfo = prGlueInfo->prP2PDevInfo;
    ASSERT(prP2pGlueDevInfo);

    /* 0. Do the scan done and set parameter to abort if the scan pending */
#if (CFG_ENABLE_UNIFY_WIPHY == 0)
    /* Default : P2P dev */
    prTargetDev = prGlueInfo->prP2PInfo[0]->prDevHandler;
    if (mtk_Netdev_To_RoleIdx(prGlueInfo, prDev, &ucRoleIdx) != 0) {
        prTargetDev = prGlueInfo->prP2PInfo[ucRoleIdx]->aprRoleHandler;
    }
#endif
    /*DBGLOG(INIT, INFO, "p2pStop and ucRoleIdx = %u\n", ucRoleIdx);*/

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
    if ((prP2pGlueDevInfo->prScanRequest != NULL) &&
#if CFG_ENABLE_UNIFY_WIPHY
        (prP2pGlueDevInfo->prScanRequest->wdev == prDev->ieee80211_ptr)
#else
        (prP2pGlueDevInfo->prScanRequest->wdev == prTargetDev->ieee80211_ptr)
#endif
        ) {
        prScanRequest = prP2pGlueDevInfo->prScanRequest;
        prP2pGlueDevInfo->prScanRequest = NULL;
    }
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

    if (prScanRequest) {
        DBGLOG(INIT, INFO, "p2pStop and abort scan!!\n");
        kalCfg80211ScanDone(prScanRequest, true);
    }

    /* 1. stop TX queue */
    netif_tx_stop_all_queues(prDev);

    /* 3. stop queue and turn off carrier */
    /*prGlueInfo->prP2PInfo[0]->eState = PARAM_MEDIA_STATE_DISCONNECTED;*/ /* TH3
                                                                            multiple
                                                                            P2P
     */

    netif_tx_stop_all_queues(prDev);
    if (netif_carrier_ok(prDev)) {
        netif_carrier_off(prDev);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, to get the network interface
 * statistical information.
 *
 * Whenever an application needs to get statistics for the interface, this
 * method is called. This happens, for example, when ifconfig or netstat -i is
 * run.
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \return net_device_stats buffer pointer.
 */
/*----------------------------------------------------------------------------*/
struct net_device_stats *p2pGetStats(IN struct net_device *prDev)
{
    return (struct net_device_stats *)kalGetStats(prDev);
}

static void p2pSetMulticastList(IN struct net_device *prDev)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;

    prGlueInfo = (prDev != NULL) ? *((P_GLUE_INFO_T *)netdev_priv(prDev)) :
                 NULL;

    ASSERT(prDev);
    ASSERT(prGlueInfo);
    if (!prDev || !prGlueInfo) {
        DBGLOG(INIT, WARN,
               " abnormal dev or skb: prDev(0x%p), prGlueInfo(0x%p)\n", prDev,
               prGlueInfo);
        return;
    }

    g_P2pPrDev = prDev;

    /* 4  Mark HALT, notify main thread to finish current job */
    set_bit(GLUE_FLAG_SUB_MOD_MULTICAST_BIT, &prGlueInfo->ulFlag);
    /* wake up main thread */
    wake_up_interruptible(&prGlueInfo->waitq);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is to set multicast list and set rx mode.
 *
 * \param[in] prDev  Pointer to struct net_device
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void mtk_p2p_wext_set_Multicastlist(P_GLUE_INFO_T prGlueInfo)
{
    u32 u4SetInfoLen = 0;
    u32 u4McCount;
    struct net_device *prDev;

    ASSERT(prGlueInfo);
    if (!prGlueInfo) {
        DBGLOG(INIT, WARN, " abnormal skb: prGlueInfo(0x%p)\n", prGlueInfo);
        return;
    }

    KAL_ACQUIRE_MUTEX(prGlueInfo->prAdapter, MUTEX_DEL_P2P_VIF);
    prDev = g_P2pPrDev;

    if (!prDev) {
        DBGLOG(INIT, WARN,
               " abnormal dev: prDev(0x%p) or netdev has been freed\n", prDev);
        KAL_RELEASE_MUTEX(prGlueInfo->prAdapter, MUTEX_DEL_P2P_VIF);
        return;
    }

    if (prDev->flags & IFF_PROMISC) {
        prGlueInfo->prP2PDevInfo->u4PacketFilter |=
            PARAM_PACKET_FILTER_PROMISCUOUS;
    }

    if (prDev->flags & IFF_BROADCAST) {
        prGlueInfo->prP2PDevInfo->u4PacketFilter |=
            PARAM_PACKET_FILTER_BROADCAST;
    }
    u4McCount = netdev_mc_count(prDev);

    if (prDev->flags & IFF_MULTICAST) {
        if ((prDev->flags & IFF_ALLMULTI) || (u4McCount > MAX_NUM_GROUP_ADDR)) {
            prGlueInfo->prP2PDevInfo->u4PacketFilter |=
                PARAM_PACKET_FILTER_ALL_MULTICAST;
        } else {
            prGlueInfo->prP2PDevInfo->u4PacketFilter |=
                PARAM_PACKET_FILTER_MULTICAST;
        }
    }

    if (prGlueInfo->prP2PDevInfo->u4PacketFilter &
        PARAM_PACKET_FILTER_MULTICAST) {
        /* Prepare multicast address list */
        struct netdev_hw_addr *ha;
        u32 i = 0;

        netdev_for_each_mc_addr(ha, prDev) {
            if (i < MAX_NUM_GROUP_ADDR) {
                COPY_MAC_ADDR(&(prGlueInfo->prP2PDevInfo->aucMCAddrList[i]),
                              GET_ADDR(ha));
                i++;
            }
        }

        DBGLOG(P2P, TRACE, "SEt Multicast Address List\n");

        if (i >= MAX_NUM_GROUP_ADDR) {
            KAL_RELEASE_MUTEX(prGlueInfo->prAdapter, MUTEX_DEL_P2P_VIF);
            return;
        }

        wlanoidSetP2PMulticastList(
            prGlueInfo->prAdapter,
            &(prGlueInfo->prP2PDevInfo->aucMCAddrList[0]), (i * ETH_ALEN),
            &u4SetInfoLen);
    }

    KAL_RELEASE_MUTEX(prGlueInfo->prAdapter, MUTEX_DEL_P2P_VIF);
}

/*----------------------------------------------------------------------------*/
/*!
** \brief This function is TX entry point of NET DEVICE.
**
** \param[in] prSkb  Pointer of the sk_buff to be sent
** \param[in] prDev  Pointer to struct net_device
**
** \retval NETDEV_TX_OK - on success.
** \retval NETDEV_TX_BUSY - on failure, packet will be discarded by upper layer.
*/
/*----------------------------------------------------------------------------*/
int p2pHardStartXmit(IN struct sk_buff *prSkb, IN struct net_device *prDev)
{
    P_NETDEV_PRIVATE_GLUE_INFO prNetDevPrivate =
        (P_NETDEV_PRIVATE_GLUE_INFO)NULL;
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 ucBssIndex;
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT
    P_ADAPTER_T prAdapter = NULL;
    u8 aucEthDestAddr[PARAM_MAC_ADDR_LEN];
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
    u8 i = 0;
    struct sk_buff *prSkbLb = NULL;
    struct sk_buff *prSkbkick = NULL;
    P_QUE_T prTxLookBackQueue = NULL;
#endif
#endif

    ASSERT(prSkb);
    ASSERT(prDev);

    prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(prDev);
    prGlueInfo = prNetDevPrivate->prGlueInfo;
    ucBssIndex = prNetDevPrivate->ucBssIdx;

#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT
    prAdapter = prGlueInfo->prAdapter;
    // Get dest Addr
    kalGetEthDestAddr(prGlueInfo, (P_NATIVE_PACKET)prSkb, aucEthDestAddr);
    if (prAdapter->ucDupMcastPacketNum > 0) {
        struct sk_buff *prSkbDup = NULL;
        u8 i = 0;
        // TODO: We take care both BCast and MCast at this stage
        DBGLOG(P2P, TRACE, "Duplicated %d MCast packets\n",
               prAdapter->ucDupMcastPacketNum);
        if (IS_BMCAST_MAC_ADDR(aucEthDestAddr)) {
            for (i = 0; i < prAdapter->ucDupMcastPacketNum; i++) {
                prSkbDup = skb_copy(prSkb, GFP_ATOMIC);
                kalResetPacket(prGlueInfo, (P_NATIVE_PACKET)prSkbDup);
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
                if (i == 0) {
                    // Set first packet of duplicated packet
                    // flag
                    GLUE_SET_PKT_FLAG((P_NATIVE_PACKET)prSkbDup,
                                      ENUM_PKT_FIRST_DUP);
                }
#endif
                kalHardStartXmit(prSkbDup, prDev, prGlueInfo, ucBssIndex);
            }
        }
    }
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
    if (prAdapter->fgIsLookBackMode && IS_BMCAST_MAC_ADDR(aucEthDestAddr)) {
        P_QUE_ENTRY_T prQueueEntry = NULL;
        struct iphdr *ip_header;
        GLUE_SPIN_LOCK_DECLARATION();

        ip_header = (struct iphdr *)skb_network_header(prSkb);
        // DBGLOG(INIT, ERROR, "The TOS is : %02x\n", ip_header->tos);
        if ((ip_header->tos != prAdapter->ucAudioTOS) ||
            prAdapter->ucAudioTOS == 0) {
            /* Not expect packet, just send and return */
            kalResetPacket(prGlueInfo, (P_NATIVE_PACKET)prSkb);
            kalHardStartXmit(prSkb, prDev, prGlueInfo, ucBssIndex);
            return NETDEV_TX_OK;
        }

        prTxLookBackQueue = &prGlueInfo->rTxLookBackQueue;
        prSkbLb = skb_copy(prSkb, GFP_ATOMIC);
        kalResetPacket(prGlueInfo, (P_NATIVE_PACKET)prSkbLb);

        /* set tag on first packet */
        /* add latest one in tail */
        prQueueEntry = (P_QUE_ENTRY_T)GLUE_GET_PKT_QUEUE_ENTRY(prSkbLb);
        GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
        QUEUE_INSERT_TAIL(prTxLookBackQueue, prQueueEntry);
        GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);

        DBGLOG(INIT, LOUD, "LB ADD: %p LB list size %d\n", prSkbLb,
               QUEUE_GET_SIZE(prTxLookBackQueue));

        if (QUEUE_GET_SIZE(prTxLookBackQueue) == 1) {
            /* Not expect packet, just send and return */
            DBGLOG(INIT, ERROR, "first packet, just send out %d\n", prSkbLb);
            kalResetPacket(prGlueInfo, (P_NATIVE_PACKET)prSkb);
            kalHardStartXmit(prSkb, prDev, prGlueInfo, ucBssIndex);
            return NETDEV_TX_OK;
        } else if (QUEUE_GET_SIZE(prTxLookBackQueue) == 2) {
            prSkbkick = NULL;
            GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
            /* get packet from look-back queue then kick */
            QUEUE_REMOVE_HEAD(prTxLookBackQueue, prQueueEntry, P_QUE_ENTRY_T);
            GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
            prSkbLb = (struct sk_buff *)GLUE_GET_PKT_DESCRIPTOR(prQueueEntry);
            /* fill lookback sequence here */
            ip_header = (struct iphdr *)skb_network_header(prSkbLb);
            ip_header->tos |= BIT(1);
            prSkbkick = skb_copy(prSkbLb, GFP_ATOMIC);
            /* re-queue the packet for next look-back */
            GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
            QUEUE_INSERT_HEAD(prTxLookBackQueue, prQueueEntry);
            GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
            /* proccess packet if Skbkick != NULL */
            if (prSkbkick) {
                /* send the look-back packet */
                kalResetPacket(prGlueInfo, (P_NATIVE_PACKET)prSkbkick);
                kalHardStartXmit(prSkbkick, prDev, prGlueInfo, ucBssIndex);
                DBGLOG(INIT, LOUD, "[%d] LB kick/requeue: %p\n", i, prSkbLb);
            }
            kalResetPacket(prGlueInfo, (P_NATIVE_PACKET)prSkb);
            kalHardStartXmit(prSkb, prDev, prGlueInfo, ucBssIndex);
            return NETDEV_TX_OK;
        } else if (QUEUE_GET_SIZE(prTxLookBackQueue) == 3) {
            for (i = 0; i < QUEUE_GET_SIZE(prTxLookBackQueue); i++) {
                prSkbkick = NULL;
                GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
                /* get packet from look-back queue then kick */
                QUEUE_REMOVE_HEAD(prTxLookBackQueue, prQueueEntry,
                                  P_QUE_ENTRY_T);
                GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
                prSkbLb =
                    (struct sk_buff *)GLUE_GET_PKT_DESCRIPTOR(prQueueEntry);
                /* fill lookback sequence here */
                ip_header = (struct iphdr *)skb_network_header(prSkbLb);
                /* switch tag to be 0xaa,0xbb,0xcc,0xdd */
                /* Tagging packet number for our propritry usage
                 *  Rx side will recover the tag
                 */
                switch (i) {
                case 0:
                    ip_header->tos |= BIT(2);
                    prSkbkick = skb_copy(prSkbLb, GFP_ATOMIC);
                    break;

                case 1:
                    ip_header->tos |= BIT(1);
                    prSkbkick = skb_copy(prSkbLb, GFP_ATOMIC);
                    break;
                }
                ;
                /* re-queue the packet for next look-back */
                GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
                QUEUE_INSERT_TAIL(prTxLookBackQueue, prQueueEntry);
                GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
                /* proccess packet if Skbkick != NULL */
                if (prSkbkick) {
                    /* send the look-back packet */
                    kalResetPacket(prGlueInfo, (P_NATIVE_PACKET)prSkbkick);
                    kalHardStartXmit(prSkbkick, prDev, prGlueInfo, ucBssIndex);
                    DBGLOG(INIT, LOUD, "[%d] LB kick/requeue: %p\n", i,
                           prSkbLb);
                }
            }
            kalResetPacket(prGlueInfo, (P_NATIVE_PACKET)prSkb);
            kalHardStartXmit(prSkb, prDev, prGlueInfo, ucBssIndex);
            return NETDEV_TX_OK;
        }

        /* kick look-back */
        for (i = 0; i <= MAX_LOOK_BACK_NUN; i++) {
            prSkbkick = NULL;
            GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
            /* get packet from look-back queue then kick */
            QUEUE_REMOVE_HEAD(prTxLookBackQueue, prQueueEntry, P_QUE_ENTRY_T);
            GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
            prSkbLb = (struct sk_buff *)GLUE_GET_PKT_DESCRIPTOR(prQueueEntry);
            /* fill lookback sequence here */
            ip_header = (struct iphdr *)skb_network_header(prSkbLb);
            /* switch tag to be 0xaa,0xbb,0xcc,0xdd */
            /* Tagging packet number for our propritry usage
             *  Rx side will recover the tag
             */
            switch (i) {
            case 0:
                ip_header->tos |= BITS(0, 1);
                break;

            case 1:
                ip_header->tos |= BIT(2);
                break;

            case 2:
                ip_header->tos |= BIT(1);
                break;
            }
            ;

            /* packet from driver all be kicked out after
             * MAX_LOOK_BACK_NUM - 1 transmit MAX_LOOK_BACK_NUM of
             * packet in total
             */
            if (i < MAX_LOOK_BACK_NUN) {
                prSkbkick = skb_copy(prSkbLb, GFP_ATOMIC);
            }
            /* re-queue the packet for next look-back */
            GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
            QUEUE_INSERT_TAIL(prTxLookBackQueue, prQueueEntry);
            GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);

            /* proccess packet if Skbkick != NULL */
            if (prSkbkick) {
                /* send the look-back packet */
                kalResetPacket(prGlueInfo, (P_NATIVE_PACKET)prSkbkick);
                ip_header = (struct iphdr *)skb_network_header(prSkbkick);
                if (i == 0) {
                    // Set first packet of duplicated packet
                    // flag for HIF Agg
                    GLUE_SET_PKT_FLAG((P_NATIVE_PACKET)prSkbkick,
                                      ENUM_PKT_FIRST_DUP);
                    DBGLOG(INIT, LOUD,
                           "Flag before Set ENUM_PKT_FIRST_DUP %04x \n",
                           GLUE_IS_PKT_FLAG_SET((P_NATIVE_PACKET)prSkbkick));
                }
                kalHardStartXmit(prSkbkick, prDev, prGlueInfo, ucBssIndex);
                DBGLOG(INIT, LOUD, "[%d] LB kick/requeue: %p\n", i, prSkbLb);
            }
            DBGLOG(INIT, LOUD, "The TOS in queue is : %02x\n", ip_header->tos);
        }
        /* check if too much packet in lookback queue */
        prQueueEntry = NULL;
        GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
        if (QUEUE_GET_SIZE(prTxLookBackQueue) > MAX_LOOK_BACK_NUN) {
            QUEUE_REMOVE_HEAD(prTxLookBackQueue, prQueueEntry, P_QUE_ENTRY_T);
            DBGLOG(INIT, LOUD, "LB Remove: %p\n",
                   (struct sk_buff *)GLUE_GET_PKT_DESCRIPTOR(prQueueEntry));
        }
        GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
        if (prQueueEntry) {
            dev_kfree_skb(
                (struct sk_buff *)GLUE_GET_PKT_DESCRIPTOR(prQueueEntry));
        }
        ip_header = (struct iphdr *)skb_network_header(prSkb);
        DBGLOG(INIT, LOUD, "The TOS in new packet is : %02x\n", ip_header->tos);
    }
#endif
#endif
    kalResetPacket(prGlueInfo, (P_NATIVE_PACKET)prSkb);
    kalHardStartXmit(prSkb, prDev, prGlueInfo, ucBssIndex);
    return NETDEV_TX_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, a primary SOCKET interface to configure
 *        the interface lively. Handle an ioctl call on one of our devices.
 *        Everything Linux ioctl specific is done here. Then we pass the
 * contents of the ifr->data to the request message handler.
 *
 * \param[in] prDev      Linux kernel netdevice
 *
 * \param[in] prIFReq    Our private ioctl request structure, typed for the
 * generic struct ifreq so we can use ptr to function
 *
 * \param[in] cmd        Command ID
 *
 * \retval WLAN_STATUS_SUCCESS The IOCTL command is executed successfully.
 * \retval OTHER The execution of IOCTL command is failed.
 */
/*----------------------------------------------------------------------------*/

int p2pDoIOCTL(struct net_device *prDev, struct ifreq *prIfReq, int i4Cmd)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    int ret = 0;
    /* char *prExtraBuf = NULL; */
    /* u32 u4ExtraSize = 0; */
    struct iwreq *prIwReq = (struct iwreq *)prIfReq;
    struct iw_request_info rIwReqInfo;
    /* fill rIwReqInfo */
    rIwReqInfo.cmd = (__u16)i4Cmd;
    rIwReqInfo.flags = 0;

    ASSERT(prDev && prIfReq);

    DBGLOG(P2P, INFO, "p2pDoIOCTL In %x %x\n", i4Cmd, SIOCDEVPRIVATE);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prDev));
    if (!prGlueInfo) {
        DBGLOG(P2P, ERROR, "prGlueInfo is NULL\n");
        return -EFAULT;
    }

    if (prGlueInfo->u4ReadyFlag == 0) {
        DBGLOG(P2P, ERROR, "Adapter is not ready\n");
        return -EINVAL;
    }

    if (i4Cmd == IOCTL_GET_DRIVER) {
        ret = priv_support_driver_cmd(prDev, prIfReq, i4Cmd);
    } else if (i4Cmd == SIOCGIWPRIV) {
        ret = mtk_p2p_wext_get_priv(prDev, &rIwReqInfo, &(prIwReq->u), NULL);
    }
#ifdef CFG_ANDROID_AOSP_PRIV_CMD
    else if (i4Cmd == SIOCDEVPRIVATE + 1) {
        ret = android_private_support_driver_cmd(prDev, prIfReq, i4Cmd);
    }
#endif
    else {
        DBGLOG(INIT, INFO, "Unexpected ioctl command: 0x%04x\n", i4Cmd);
        ret = -1;
    }

    return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief To override p2p interface address
 *
 * \param[in] prDev Net device requested.
 * \param[in] addr  Pointer to address
 *
 * \retval 0 For success.
 * \retval -E2BIG For user's buffer size is too small.
 * \retval -EFAULT For fail.
 *
 */
/*----------------------------------------------------------------------------*/
int p2pSetMACAddress(IN struct net_device *prDev, void *addr)
{
    P_ADAPTER_T prAdapter = NULL;
    P_GLUE_INFO_T prGlueInfo = NULL;

    ASSERT(prDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prDev));
    ASSERT(prGlueInfo);

    prAdapter = prGlueInfo->prAdapter;
    ASSERT(prAdapter);

    /* @FIXME */
    return eth_mac_addr(prDev, addr);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief To report the private supported IOCTLs table to user space.
 *
 * \param[in] prDev Net device requested.
 * \param[out] prIfReq Pointer to ifreq structure, content is copied back to
 *                  user space buffer in gl_iwpriv_table.
 *
 * \retval 0 For success.
 * \retval -E2BIG For user's buffer size is too small.
 * \retval -EFAULT For fail.
 *
 */
/*----------------------------------------------------------------------------*/
int mtk_p2p_wext_get_priv(IN struct net_device *prDev,
                          IN struct iw_request_info *info,
                          IN OUT union iwreq_data *wrqu, IN OUT char *extra)
{
    struct iw_point *prData = (struct iw_point *)&wrqu->data;
    u16 u2BufferSize = 0;

    ASSERT(prDev);

    u2BufferSize = prData->length;

    /* update our private table size */
    prData->length =
        (__u16)sizeof(rP2PIwPrivTable) / sizeof(struct iw_priv_args);

    if (u2BufferSize < prData->length) {
        return -E2BIG;
    }

    if (prData->length) {
        if (copy_to_user(prData->pointer, rP2PIwPrivTable,
                         sizeof(rP2PIwPrivTable))) {
            return -EFAULT;
        }
    }

    return 0;
}
