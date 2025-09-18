// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_p2p_cfg80211.c
 *    \brief  Main routines of Linux driver interface for Wi-Fi Direct
 *     using cfg80211 interface
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

#include "config.h"

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/wireless.h>
#include <linux/ieee80211.h>
#include <net/cfg80211.h>

#include "precomp.h"
#include "gl_cfg80211.h"
#include "gl_p2p_os.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
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

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#if CFG_ENABLE_UNIFY_WIPHY
#define P2P_WIPHY_PRIV(_wiphy, _priv) \
    (_priv = (P_GLUE_INFO_T)wiphy_priv(_wiphy))
#else
#define P2P_WIPHY_PRIV(_wiphy, _priv) \
    (_priv = *((P_GLUE_INFO_T *)wiphy_priv(_wiphy)))
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
u8 mtk_p2p_cfg80211func_channel_sco_switch(
    IN enum nl80211_channel_type channel_type, IN P_ENUM_CHNL_EXT_T prChnlSco)
{
    u8 fgIsValid = false;

    do {
        if (prChnlSco) {
            switch (channel_type) {
            case NL80211_CHAN_NO_HT:
                *prChnlSco = CHNL_EXT_SCN;
                break;

            case NL80211_CHAN_HT20:
                *prChnlSco = CHNL_EXT_SCN;
                break;

            case NL80211_CHAN_HT40MINUS:
                *prChnlSco = CHNL_EXT_SCA;
                break;

            case NL80211_CHAN_HT40PLUS:
                *prChnlSco = CHNL_EXT_SCB;
                break;

            default:
                ASSERT(false);
                *prChnlSco = CHNL_EXT_SCN;
                break;
            }
        }

        fgIsValid = true;
    } while (false);

    return fgIsValid;
}

u8 mtk_p2p_cfg80211func_channel_format_switch(
    IN struct cfg80211_chan_def *channel_def,
    IN struct ieee80211_channel *channel, IN P_RF_CHANNEL_INFO_T prRfChnlInfo)
{
    u8 fgIsValid = false;

    do {
        if (channel == NULL) {
            break;
        }

        if (prRfChnlInfo) {
            prRfChnlInfo->ucChannelNum =
                nicFreq2ChannelNum(channel->center_freq * 1000);

            switch (channel->band) {
            case NL80211_BAND_2GHZ:
                prRfChnlInfo->eBand = BAND_2G4;
                break;

            case NL80211_BAND_5GHZ:
                prRfChnlInfo->eBand = BAND_5G;
                break;

            default:
                prRfChnlInfo->eBand = BAND_2G4;
                break;
            }
        }

        if (channel_def && prRfChnlInfo) {
            switch (channel_def->width) {
            case NL80211_CHAN_WIDTH_20_NOHT:
            case NL80211_CHAN_WIDTH_20:
                prRfChnlInfo->ucChnlBw = MAX_BW_20MHZ;
                break;

            case NL80211_CHAN_WIDTH_40:
                prRfChnlInfo->ucChnlBw = MAX_BW_40MHZ;
                break;

            case NL80211_CHAN_WIDTH_80:
                prRfChnlInfo->ucChnlBw = MAX_BW_80MHZ;
                break;

            case NL80211_CHAN_WIDTH_80P80:
                prRfChnlInfo->ucChnlBw = MAX_BW_80_80_MHZ;
                break;

            case NL80211_CHAN_WIDTH_160:
                prRfChnlInfo->ucChnlBw = MAX_BW_160MHZ;
                break;

            default:
                prRfChnlInfo->ucChnlBw = MAX_BW_20MHZ;
                break;
            }
            prRfChnlInfo->u2PriChnlFreq = channel->center_freq;
            prRfChnlInfo->u4CenterFreq1 = channel_def->center_freq1;
            prRfChnlInfo->u4CenterFreq2 = channel_def->center_freq2;

#if (CFG_SUPPORT_DFS_MASTER == 1)
            prRfChnlInfo->u4ChnlDfsState = channel->dfs_state;
#endif
        }

        fgIsValid = true;
    } while (false);

    return fgIsValid;
}

/* mtk_p2p_cfg80211func_channel_format_switch */

s32 mtk_Netdev_To_RoleIdx(P_GLUE_INFO_T prGlueInfo, struct net_device *ndev,
                          u8 *pucRoleIdx)
{
    s32 i4Ret = -1;
    u32 u4Idx = 0;

    if ((pucRoleIdx == NULL) || (ndev == NULL)) {
        return i4Ret;
    }

    if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL ||
        prGlueInfo->prAdapter->prP2pInfo == NULL) {
        DBGLOG(INIT, ERROR, "prGlueInfo || prAdapter || prP2pInfo == NULL\n");
        return i4Ret;
    }

    /* The prP2PInfo[0] may be removed and prP2PInfo[1] is existing
     * under cfg80211 operation. So that check all KAL_P2P_NUM not only
     * prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum.
     */
    for (u4Idx = 0; u4Idx < KAL_P2P_NUM; u4Idx++) {
        if ((prGlueInfo->prP2PInfo[u4Idx] != NULL) &&
            (prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler != NULL) &&
            (prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler == ndev)) {
            *pucRoleIdx = (u8)u4Idx;
            i4Ret = 0;
        }
    }

    return i4Ret;
}

static void mtk_vif_destructor(struct net_device *dev)
{
    struct wireless_dev *prWdev = ERR_PTR(-ENOMEM);
    u32 u4Idx = 0;

    DBGLOG(P2P, INFO, "mtk_vif_destructor\n");
    if (dev) {
        prWdev = dev->ieee80211_ptr;
        free_netdev(dev);
        /* Expect that the gprP2pWdev isn't freed here */
        if ((prWdev) && (prWdev != gprP2pWdev)) {
            /* Role[i] and Dev share the same wdev by default */
            for (u4Idx = 0; u4Idx < KAL_P2P_NUM; u4Idx++) {
                if (prWdev != gprP2pRoleWdev[u4Idx]) {
                    continue;
                }
                /* In the initWlan gprP2pRoleWdev[0] is equal to
                 * gprP2pWdev. And other gprP2pRoleWdev[] should
                 * be NULL, if the 2nd P2P dev isn't created.
                 */
                if (u4Idx == 0) {
                    gprP2pRoleWdev[u4Idx] = gprP2pWdev;
                } else {
                    gprP2pRoleWdev[u4Idx] = NULL;
                }
                break;
            }
            kfree(prWdev);
        }
    }
}

#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
struct wireless_dev *mtk_p2p_cfg80211_add_iface(struct wiphy *wiphy,
                                                const char *name,
                                                unsigned char name_assign_type,
                                                enum nl80211_iftype type,
                                                struct vif_params *params)
#else
struct wireless_dev *mtk_p2p_cfg80211_add_iface(
    struct wiphy *wiphy, const char *name, unsigned char name_assign_type,
    enum nl80211_iftype type, u32 *flags, struct vif_params *params)
#endif
{
    /* 2 TODO: Fit kernel 3.10 modification */
    P_ADAPTER_T prAdapter;
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    struct net_device *prNewNetDevice = NULL;
    u32 u4Idx = 0;
    P_GL_P2P_INFO_T prP2pInfo = (P_GL_P2P_INFO_T)NULL;
    P_GL_HIF_INFO_T prHif = NULL;
    P_MSG_P2P_SWITCH_OP_MODE_T prSwitchModeMsg =
        (P_MSG_P2P_SWITCH_OP_MODE_T)NULL;
    struct wireless_dev *prWdev = ERR_PTR(-ENOMEM);
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_NETDEV_PRIVATE_GLUE_INFO prNetDevPriv = (P_NETDEV_PRIVATE_GLUE_INFO)NULL;
    PARAM_MAC_ADDRESS rMacAddr;
    P_MSG_P2P_ACTIVE_DEV_BSS_T prMsgActiveBss =
        (P_MSG_P2P_ACTIVE_DEV_BSS_T)NULL;

    DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_add_iface\n");

    do {
        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        if (prGlueInfo == NULL) {
            break;
        }

        prAdapter = prGlueInfo->prAdapter;

        for (u4Idx = 0; u4Idx < KAL_P2P_NUM; u4Idx++) {
            prP2pInfo = prGlueInfo->prP2PInfo[u4Idx];
            /* Expect that only create the new dev with the p2p0 */
            if (prP2pInfo == NULL) {
                continue;
            }
            if (prP2pInfo->aprRoleHandler == prP2pInfo->prDevHandler) {
                break;
            }
            if (prP2pInfo->aprRoleHandler == NULL) {
                p2pRoleFsmInit(prGlueInfo->prAdapter, u4Idx);
                break;
            }
        }

        if (u4Idx == KAL_P2P_NUM) {
            break;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_add_iface u4Idx=%d\n", u4Idx);

        prP2pInfo = prGlueInfo->prP2PInfo[u4Idx];

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_add_iface name = %s\n", name);

        prNewNetDevice = alloc_netdev_mq(sizeof(NETDEV_PRIVATE_GLUE_INFO), name,
                                         NET_NAME_PREDICTABLE, ether_setup,
                                         CFG_MAX_TXQ_NUM);

        if (prNewNetDevice == NULL) {
            DBGLOG(P2P, TRACE, "alloc_netdev_mq fail\n");
            break;
        }
        prP2pInfo->aprRoleHandler = prNewNetDevice;

        *((P_GLUE_INFO_T *)netdev_priv(prNewNetDevice)) = prGlueInfo;

        prNewNetDevice->needed_headroom += NIC_TX_HEAD_ROOM;
        prNewNetDevice->netdev_ops = &p2p_netdev_ops;

        prHif = &prGlueInfo->rHifInfo;
        ASSERT(prHif);

        SET_NETDEV_DEV(prNewNetDevice, &(prHif->func->dev));

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
        prWdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
        if (!prWdev) {
            DBGLOG(P2P, ERROR,
                   "allocate p2p wireless device fail, no memory\n");
#if CFG_ENABLE_UNIFY_WIPHY
            free_netdev(prP2pInfo->aprRoleHandler);
            prP2pInfo->aprRoleHandler = NULL;
#else
            prWdev = ERR_PTR(-ENOMEM);
            free_netdev(prP2pInfo->aprRoleHandler);
            prP2pInfo->aprRoleHandler = NULL;
#endif
            break;
        }
        kalMemCopy(prWdev, gprP2pWdev, sizeof(struct wireless_dev));
        prWdev->netdev = prNewNetDevice;
        prWdev->iftype = type;
        prNewNetDevice->ieee80211_ptr = prWdev;

        /* register destructor function for virtual interface */
#if KERNEL_VERSION(4, 11, 0) <= CFG80211_VERSION_CODE
        prNewNetDevice->priv_destructor = mtk_vif_destructor;
#else
        prNewNetDevice->destructor = mtk_vif_destructor;
#endif

        gprP2pRoleWdev[u4Idx] = prWdev;
        /*prP2pInfo->prRoleWdev[0] = prWdev;*/ /* TH3 multiple P2P */
#endif

#if CFG_TCP_IP_CHKSUM_OFFLOAD
        /* set HW checksum offload */
        if (prAdapter->fgIsSupportCsumOffload) {
            prNewNetDevice->features = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
                                       NETIF_F_RXCSUM;
        }
#endif

        kalResetStats(prNewNetDevice);

        /* register for net device */
#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
        if (cfg80211_register_netdevice(prP2pInfo->aprRoleHandler) < 0) {
#else
        if (register_netdevice(prP2pInfo->aprRoleHandler) < 0) {
#endif
            DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_add_iface 456\n");
            DBGLOG(INIT, WARN, "unable to register netdevice for p2p\n");
            kfree(prWdev);
            prWdev = ERR_PTR(-ENOMEM);
            free_netdev(prP2pInfo->aprRoleHandler);

#if CFG_ENABLE_UNIFY_WIPHY
            kfree(prWdev);
            prP2pInfo->aprRoleHandler = NULL;
#endif

            prP2pInfo->aprRoleHandler = NULL;
            break;
        } else {
            DBGLOG(P2P, TRACE, "register_netdev OK, iftype:%d\n",
                   prNewNetDevice->ieee80211_ptr->iftype);
            prGlueInfo->prAdapter->rP2PNetRegState =
                ENUM_NET_REG_STATE_REGISTERED;

            netif_carrier_off(prP2pInfo->aprRoleHandler);
            netif_tx_stop_all_queues(prP2pInfo->aprRoleHandler);
        }
        prP2pRoleFsmInfo = prAdapter->rWifiVar.aprP2pRoleFsmInfo[u4Idx];

        /* 13. bind netdev pointer to netdev index */
        wlanBindBssIdxToNetInterface(prGlueInfo, prP2pRoleFsmInfo->ucBssIndex,
                                     (void *)prP2pInfo->aprRoleHandler);
        prNetDevPriv =
            (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(prP2pInfo->aprRoleHandler);
        prNetDevPriv->prGlueInfo = prGlueInfo;
        prNetDevPriv->ucBssIdx = prP2pRoleFsmInfo->ucBssIndex;

#if CFG_ENABLE_UNIFY_WIPHY
        /* Expect that only P2P device uses the cfg80211_add_iface */
        prNetDevPriv->ucIsP2p = true;
#endif
        /* 4.2 fill hardware address */
        COPY_MAC_ADDR(rMacAddr, prAdapter->rMyMacAddr);
        if (prGlueInfo->prAdapter->rWifiVar.ucP2pShareMacAddr &&
            (type == NL80211_IFTYPE_P2P_CLIENT ||
             type == NL80211_IFTYPE_P2P_GO)) {
            rMacAddr[0] = gPrP2pDev[0]->dev_addr[0];
        } else {
            rMacAddr[0] |= 0x2;  /* change to local administrated
                                  * address */
            if (u4Idx > 0) {
                rMacAddr[0] ^= u4Idx << 2;
            } else {
                rMacAddr[0] ^= prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum
                               << 2;
            }
        }

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
        kalMemCopy(prNewNetDevice->dev_addr, rMacAddr, ETH_ALEN);
#else
        dev_addr_set(prNewNetDevice, rMacAddr);
#endif
        kalMemCopy(prNewNetDevice->perm_addr, rMacAddr, ETH_ALEN);
#if CFG_MESON_G12A_PATCH
        prNewNetDevice->mtu = 1408;
#endif

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_add_iface ucBssIdx=%d\n",
               prNetDevPriv->ucBssIdx);

        /* Switch OP MOde. */
        prSwitchModeMsg = (P_MSG_P2P_SWITCH_OP_MODE_T)cnmMemAlloc(
            prGlueInfo->prAdapter, RAM_TYPE_MSG,
            sizeof(MSG_P2P_SWITCH_OP_MODE_T));

        if (prSwitchModeMsg == NULL) {
            ASSERT(false);
            DBGLOG(INIT, WARN, "unable to alloc msg\n");
            kfree(prWdev);
            prWdev = ERR_PTR(-ENOMEM);
            free_netdev(prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler);
#if CFG_ENABLE_UNIFY_WIPHY
            //	kfree(prWdev);
            prP2pInfo->aprRoleHandler = NULL;
#endif
            prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler = NULL;
            break;
        } else {
            prSwitchModeMsg->rMsgHdr.eMsgId = MID_MNY_P2P_FUN_SWITCH;
            prSwitchModeMsg->ucRoleIdx = 0;
            switch (type) {
            case NL80211_IFTYPE_P2P_CLIENT:
                DBGLOG(P2P, TRACE, "NL80211_IFTYPE_P2P_CLIENT.\n");
                prSwitchModeMsg->eOpMode = OP_MODE_INFRASTRUCTURE;
                kalP2PSetRole(prGlueInfo, 1, u4Idx);
                break;

            case NL80211_IFTYPE_STATION:
                DBGLOG(P2P, TRACE, "NL80211_IFTYPE_STATION.\n");
                prSwitchModeMsg->eOpMode = OP_MODE_INFRASTRUCTURE;
                kalP2PSetRole(prGlueInfo, 1, u4Idx);
                break;

            case NL80211_IFTYPE_AP:
                DBGLOG(P2P, TRACE, "NL80211_IFTYPE_AP.\n");
                prSwitchModeMsg->eOpMode = OP_MODE_ACCESS_POINT;
                kalP2PSetRole(prGlueInfo, 2, u4Idx);
                break;

            case NL80211_IFTYPE_P2P_GO:
                DBGLOG(P2P, TRACE, "NL80211_IFTYPE_P2P_GO not AP.\n");
                prSwitchModeMsg->eOpMode = OP_MODE_ACCESS_POINT;
                kalP2PSetRole(prGlueInfo, 2, u4Idx);
                break;

            default:
                DBGLOG(P2P, TRACE, "Other type :%d .\n", type);
                prSwitchModeMsg->eOpMode = OP_MODE_P2P_DEVICE;
                kalP2PSetRole(prGlueInfo, 0, u4Idx);
                break;
            }
            mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                        (P_MSG_HDR_T)prSwitchModeMsg, MSG_SEND_METHOD_BUF);
        }

        /* Send Msg to DevFsm and active P2P dev BSS */
        prMsgActiveBss = (P_MSG_P2P_ACTIVE_DEV_BSS_T)cnmMemAlloc(
            prGlueInfo->prAdapter, RAM_TYPE_MSG,
            sizeof(MSG_P2P_ACTIVE_DEV_BSS_T));

#if CFG_ENABLE_UNIFY_WIPHY
        if (prMsgActiveBss == NULL) {
            DBGLOG(INIT, WARN, "unable to alloc prMsgActiveBss\n");
            kfree(prWdev);
            free_netdev(prP2pInfo->aprRoleHandler);
            prP2pInfo->aprRoleHandler = NULL;
            /* FIMXE: What is the error handler?
             * The gprP2pRoleWdev[0] is default as gprP2pWdev.
             */
            gprP2pRoleWdev[u4Idx] = gprP2pWdev;
            break;
        }
#else
        if (prMsgActiveBss == NULL) {
            ASSERT(false);
            DBGLOG(INIT, WARN, "unable to alloc msg\n");
            kfree(prWdev);
            prWdev = ERR_PTR(-ENOMEM);
            free_netdev(prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler);
            prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler = NULL;
            break;
        }
#endif

        prMsgActiveBss->rMsgHdr.eMsgId = MID_MNY_P2P_ACTIVE_BSS;
        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prMsgActiveBss, MSG_SEND_METHOD_BUF);
    } while (false);

    return prWdev;
}

int mtk_p2p_cfg80211_del_iface(struct wiphy *wiphy, struct wireless_dev *wdev)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    P_MSG_P2P_DEL_IFACE_T prP2pDelIfaceMsg = (P_MSG_P2P_DEL_IFACE_T)NULL;
    P_ADAPTER_T prAdapter;
    P_GL_P2P_INFO_T prP2pInfo = (P_GL_P2P_INFO_T)NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_GL_P2P_DEV_INFO_T prP2pGlueDevInfo = (P_GL_P2P_DEV_INFO_T)NULL;
    struct net_device *UnregRoleHander = (struct net_device *)NULL;
    unsigned char ucBssIdx = 0;
    P_BSS_INFO_T prP2pBssInfo = NULL;
#if CFG_ENABLE_UNIFY_WIPHY
    struct cfg80211_scan_request *prScanRequest = NULL;
#endif
    GLUE_SPIN_LOCK_DECLARATION();

    DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_del_iface\n");

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);
    if (prGlueInfo == NULL) {
        return -EINVAL;
    }

    prAdapter = prGlueInfo->prAdapter;
    prP2pInfo = prGlueInfo->prP2PInfo[0];
    prP2pGlueDevInfo = prGlueInfo->prP2PDevInfo;

    if ((prP2pInfo == NULL) || (prP2pInfo->aprRoleHandler == NULL) ||
        (prP2pInfo->aprRoleHandler == prP2pInfo->prDevHandler)) {
        /* This iface isn't added. */
        return -EINVAL;
    }

    KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_DEL_INF);

    prP2pRoleFsmInfo = prAdapter->rWifiVar.aprP2pRoleFsmInfo[0];
    if (prP2pRoleFsmInfo == NULL) {
        KAL_RELEASE_MUTEX(prAdapter, MUTEX_DEL_INF);
        return -EINVAL;
    }
    ucBssIdx = prP2pRoleFsmInfo->ucBssIndex;
    wlanBindBssIdxToNetInterface(prGlueInfo, ucBssIdx,
                                 (void *)prGlueInfo->prP2PInfo[0]->prDevHandler);

    UnregRoleHander = prP2pInfo->aprRoleHandler;

    /* fix that the kernel warning occures when the GC is connected */
    prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
    if ((prP2pBssInfo != NULL) &&
        (prP2pBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) &&
        (wdev->iftype == NL80211_IFTYPE_P2P_CLIENT)) {
        cfg80211_disconnected(UnregRoleHander, 0, NULL, 0, true, GFP_KERNEL);
    }
    /* Wait for kalSendCompleteAndAwakeQueue() complete */
    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
    prP2pInfo->aprRoleHandler = prP2pInfo->prDevHandler;

#if CFG_ENABLE_UNIFY_WIPHY
    prScanRequest = prP2pGlueDevInfo->prScanRequest;
    if ((prScanRequest != NULL) &&
        (prScanRequest->wdev == UnregRoleHander->ieee80211_ptr)) {
        prP2pGlueDevInfo->prScanRequest = NULL;
    }
#endif

    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
    /* Check the Scan request is pending and we abort it before delete
     * interface */

#if CFG_ENABLE_UNIFY_WIPHY
    if (prScanRequest) {
        kalCfg80211ScanDone(prScanRequest, true);
    }
#else

    if (prP2pGlueDevInfo->prScanRequest != NULL) {
        /* Check the wdev with backup scan req due to */
        /* the kernel will free this request by error handling */
        if (prP2pGlueDevInfo->rBackupScanRequest.wdev ==
            UnregRoleHander->ieee80211_ptr) {
            kalCfg80211ScanDone(&(prP2pGlueDevInfo->rBackupScanRequest), true);
            /* clear the request to avoid the Role FSM calls the
             * scan_done again */
            prP2pGlueDevInfo->prScanRequest = NULL;
        }
    }
#endif

    /* prepare for removal */
    if (netif_carrier_ok(UnregRoleHander)) {
        netif_carrier_off(UnregRoleHander);
    }

    netif_tx_stop_all_queues(UnregRoleHander);

    /* Here are functions which need rtnl_lock */
#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
    cfg80211_unregister_netdevice(UnregRoleHander);
#else
    unregister_netdevice(UnregRoleHander);
#endif

    /* free is called at destructor */
    /* free_netdev(UnregRoleHander); */

#if CFG_ENABLE_UNIFY_WIPHY
    /* Role[0] and Dev share the same wireless dev */
    gprP2pRoleWdev[0] = gprP2pWdev;
#endif

    KAL_RELEASE_MUTEX(prAdapter, MUTEX_DEL_INF);

    prP2pDelIfaceMsg = (P_MSG_P2P_DEL_IFACE_T)cnmMemAlloc(
        prGlueInfo->prAdapter, RAM_TYPE_MSG, sizeof(MSG_P2P_DEL_IFACE_T));

    if (prP2pDelIfaceMsg == NULL) {
        ASSERT(false);
        DBGLOG(INIT, WARN, "unable to alloc msg\n");
    } else {
        prP2pDelIfaceMsg->rMsgHdr.eMsgId = MID_MNY_P2P_DEL_IFACE;
        prP2pDelIfaceMsg->ucRoleIdx = 0;

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prP2pDelIfaceMsg, MSG_SEND_METHOD_BUF);
    }

    return 0;
}

int mtk_p2p_cfg80211_add_key(struct wiphy *wiphy, struct net_device *ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
                             int link_id,
#endif
                             u8 key_index, bool pairwise, const u8 *mac_addr,
                             struct key_params *params)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Rslt = -EINVAL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    P2P_PARAM_KEY_T rKey;
    u8 ucRoleIdx = 0;
    const u8 aucBCAddr[] = BC_MAC_ADDR;
    /* const u8 aucZeroMacAddr[] = NULL_MAC_ADDR; */
#if CFG_SUPPORT_REPLAY_DETECTION
    P_BSS_INFO_T prBssInfo = NULL;
    struct SEC_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;
    u8 ucCheckZeroKey = 0;
    u8 i = 0;
#endif

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev, &ucRoleIdx) != 0) {
        return -EINVAL;
    }

#if DBG
    DBGLOG(RSN, TRACE, "mtk_p2p_cfg80211_add_key\n");
    if (mac_addr) {
        DBGLOG(RSN, INFO, "keyIdx = %d pairwise = %d mac = " MACSTR "\n",
               key_index, pairwise, MAC2STR(mac_addr));
    } else {
        DBGLOG(RSN, INFO, "keyIdx = %d pairwise = %d null mac\n", key_index,
               pairwise);
    }
    DBGLOG(RSN, TRACE, "Cipher = %x\n", params->cipher);
    DBGLOG_MEM8(RSN, TRACE, params->key, params->key_len);
#endif

    /* Todo:: By Cipher to set the key */

    kalMemZero(&rKey, sizeof(P2P_PARAM_KEY_T));

    if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &rKey.ucBssIdx) !=
        WLAN_STATUS_SUCCESS) {
        return -EINVAL;
    }

    rKey.u4KeyIndex = key_index;

    if (params->cipher) {
        switch (params->cipher) {
        case WLAN_CIPHER_SUITE_WEP40:
            rKey.ucCipher = CIPHER_SUITE_WEP40;
            break;

        case WLAN_CIPHER_SUITE_WEP104:
            rKey.ucCipher = CIPHER_SUITE_WEP104;
            break;

        case WLAN_CIPHER_SUITE_TKIP:
            rKey.ucCipher = CIPHER_SUITE_TKIP;
            break;

        case WLAN_CIPHER_SUITE_CCMP:
            rKey.ucCipher = CIPHER_SUITE_CCMP;
            break;

        case WLAN_CIPHER_SUITE_SMS4:
            rKey.ucCipher = CIPHER_SUITE_WPI;
            break;

        case WLAN_CIPHER_SUITE_AES_CMAC:
            rKey.ucCipher = CIPHER_SUITE_BIP;
            break;

        default:
            ASSERT(false);
        }
    }

    /* For BC addr case: ex: AP mode, driver_nl80211 will not have mac_addr
     */
    if (pairwise) {
        rKey.u4KeyIndex |= BIT(31);  /* Tx */
        rKey.u4KeyIndex |= BIT(30);  /* Pairwise */
        COPY_MAC_ADDR(rKey.arBSSID, mac_addr);
    } else {
        COPY_MAC_ADDR(rKey.arBSSID, aucBCAddr);
    }

    /* Check if add key under AP mode */
    if (kalP2PGetRole(prGlueInfo, ucRoleIdx) == 2) {
        rKey.u4KeyIndex |= BIT(28);  /* authenticator */
    }
#if CFG_SUPPORT_REPLAY_DETECTION
    if (params->key) {
        for (i = 0; i < params->key_len; i++)
            if (params->key[i] == 0x00) {
                ucCheckZeroKey++;
            }

        if (ucCheckZeroKey == params->key_len) {
            return 0;
        }
    }
#endif

    if (params->key) {
        kalMemCopy(rKey.aucKeyMaterial, params->key, params->key_len);
    }
    rKey.u4KeyLength = params->key_len;
    rKey.u4Length = ((unsigned long)&(((P_P2P_PARAM_KEY_T)0)->aucKeyMaterial)) +
                    rKey.u4KeyLength;

#if CFG_SUPPORT_REPLAY_DETECTION
    prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, rKey.ucBssIdx);

    prDetRplyInfo = &prBssInfo->rDetRplyInfo;

    if ((!pairwise) && ((params->cipher == WLAN_CIPHER_SUITE_TKIP) ||
                        (params->cipher == WLAN_CIPHER_SUITE_CCMP))) {
        if ((prDetRplyInfo->ucCurKeyId == key_index) &&
            (!kalMemCmp(prDetRplyInfo->aucKeyMaterial, params->key,
                        params->key_len))) {
            DBGLOG(RSN, TRACE, "M3/G1, KeyID and KeyValue equal.\n");
            DBGLOG(RSN, TRACE,
                   "hit group key reinstall case, so no update BC/MC PN.\n");
        } else {
            kalMemCopy(prDetRplyInfo->arReplayPNInfo[key_index].auPN,
                       params->seq, params->seq_len);
            prDetRplyInfo->ucCurKeyId = key_index;
            prDetRplyInfo->u4KeyLength = params->key_len;
            kalMemCopy(prDetRplyInfo->aucKeyMaterial, params->key,
                       params->key_len);
        }

        prDetRplyInfo->fgKeyRscFresh = true;
    }
#endif

    rStatus = kalIoctl(prGlueInfo, wlanoidSetAddKey, &rKey, rKey.u4Length,
                       false, false, true, &u4BufLen);
    if (rStatus == WLAN_STATUS_SUCCESS) {
        i4Rslt = 0;
    }

    return i4Rslt;
}

int mtk_p2p_cfg80211_get_key(struct wiphy *wiphy, struct net_device *ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
                             int link_id,
#endif
                             u8 key_index, bool pairwise, const u8 *mac_addr,
                             void *cookie,
                             void (*callback)(void *cookie,
                                              struct key_params *))
{
    P_GLUE_INFO_T prGlueInfo = NULL;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    /* not implemented yet */

    return -EINVAL;
}

int mtk_p2p_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
                             int link_id,
#endif
                             u8 key_index, bool pairwise, const u8 *mac_addr)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    PARAM_REMOVE_KEY_T rRemoveKey;
    s32 i4Rslt = -EINVAL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    u8 ucRoleIdx = 0;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev, &ucRoleIdx) < 0) {
        return -EINVAL;
    }

#if DBG
    DBGLOG(RSN, TRACE, "mtk_p2p_cfg80211_del_key\n");
    if (mac_addr) {
        DBGLOG(RSN, TRACE, "keyIdx = %d pairwise = %d mac = " MACSTR "\n",
               key_index, pairwise, MAC2STR(mac_addr));
    } else {
        DBGLOG(RSN, TRACE, "keyIdx = %d pairwise = %d null mac\n", key_index,
               pairwise);
    }
#endif

    kalMemZero(&rRemoveKey, sizeof(PARAM_REMOVE_KEY_T));

    if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx,
                            &rRemoveKey.ucBssIdx) != WLAN_STATUS_SUCCESS) {
        return -EINVAL;
    }

    if (mac_addr) {
        COPY_MAC_ADDR(rRemoveKey.arBSSID, mac_addr);
    }
    rRemoveKey.u4KeyIndex = key_index;
    rRemoveKey.u4Length = sizeof(PARAM_REMOVE_KEY_T);
    if (mac_addr) {
        COPY_MAC_ADDR(rRemoveKey.arBSSID, mac_addr);
        rRemoveKey.u4KeyIndex |= BIT(30);
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSetRemoveKey, &rRemoveKey,
                       rRemoveKey.u4Length, false, false, true, &u4BufLen);

    if (rStatus == WLAN_STATUS_SUCCESS) {
        i4Rslt = 0;
    }

    return i4Rslt;
}

int mtk_p2p_cfg80211_set_default_key(struct wiphy *wiphy,
                                     struct net_device *netdev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
                                     int link_id,
#endif
                                     u8 key_index, bool unicast,
                                     bool multicast)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    PARAM_DEFAULT_KEY_T rDefaultKey;
    u8 ucRoleIdx = 0;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    s32 i4Rst = -EINVAL;
    u32 u4BufLen = 0;
    u8 fgDef = false, fgMgtDef = false;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    if (mtk_Netdev_To_RoleIdx(prGlueInfo, netdev, &ucRoleIdx) != 0) {
        return -EINVAL;
    }

#if DBG
    DBGLOG(RSN, TRACE, "mtk_p2p_cfg80211_set_default_key\n");
    DBGLOG(RSN, TRACE, "keyIdx = %d unicast = %d multicast = %d\n", key_index,
           unicast, multicast);
#endif

    /* For wep case, this set the key for tx */
    if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx,
                            &rDefaultKey.ucBssIdx) != WLAN_STATUS_SUCCESS) {
        return -EINVAL;
    }

    rDefaultKey.ucKeyID = key_index;
    rDefaultKey.ucUnicast = unicast;
    rDefaultKey.ucMulticast = multicast;
    if (rDefaultKey.ucUnicast && !rDefaultKey.ucMulticast) {
        return WLAN_STATUS_SUCCESS;
    }

    if (rDefaultKey.ucUnicast && rDefaultKey.ucMulticast) {
        fgDef = true;
    }

    if (!rDefaultKey.ucUnicast && rDefaultKey.ucMulticast) {
        fgMgtDef = true;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSetDefaultKey, &rDefaultKey,
                       sizeof(PARAM_DEFAULT_KEY_T), false, false, true,
                       &u4BufLen);

    if (rStatus == WLAN_STATUS_SUCCESS) {
        i4Rst = 0;
    }

    return i4Rst;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for setting the default mgmt key index
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_p2p_cfg80211_set_mgmt_key(struct wiphy *wiphy, struct net_device *dev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
                                  int link_id,
#endif
                                  u8 key_index)
{
    DBGLOG(RSN, INFO, "mtk_p2p_cfg80211_set_mgmt_key, kid:%d\n", key_index);

    return 0;
}

int mtk_p2p_cfg80211_get_station(struct wiphy *wiphy, struct net_device *ndev,
                                 const u8 *mac, struct station_info *sinfo)
{
    s32 i4RetRslt = -EINVAL;
    u8 ucRoleIdx = 0;
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    P_GL_P2P_INFO_T prP2pGlueInfo = (P_GL_P2P_INFO_T)NULL;
    P2P_STATION_INFO_T rP2pStaInfo;
    s32 i4Rssi = 0;
    u8 ucBssIdx = 0;
    u32 u4Rate = 0;
    P_BSS_INFO_T prBssInfo;
    P_PARAM_GET_STA_STATISTICS prQuery;

    ASSERT(wiphy);

    do {
        if ((wiphy == NULL) || (ndev == NULL) || (sinfo == NULL) ||
            (mac == NULL)) {
            break;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_get_station\n");

        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev, &ucRoleIdx) != 0) {
            return -EINVAL;
        }

        prP2pGlueInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

        /* Get station information. */
        /* 1. Inactive time? */
        p2pFuncGetStationInfo(prGlueInfo->prAdapter, (u8 *)mac, &rP2pStaInfo);

        /* Inactive time. */
        sinfo->filled |= BIT(NL80211_STA_INFO_INACTIVE_TIME);
        sinfo->inactive_time = rP2pStaInfo.u4InactiveTime;
        sinfo->generation = prP2pGlueInfo->i4Generation;

        /* 2. fill TX rate */
        if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
            WLAN_STATUS_SUCCESS) {
            return -EINVAL;
        }
        prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIdx);
        if (!prBssInfo) {
            DBGLOG(P2P, WARN, "bss is not active\n");
            return -EINVAL;
        }
        if (prBssInfo->eConnectionState != PARAM_MEDIA_STATE_CONNECTED) {
            /* not connected */
            DBGLOG(P2P, WARN, "not yet connected\n");
            return 0;
        }

        prQuery =
            prGlueInfo->prAdapter->rWifiVar.prP2pQueryStaStatistics[ucRoleIdx];
        if (prQuery) {
            u4Rate = prQuery->u2LinkSpeed * 5000;
            i4Rssi = RCPI_TO_dBm(prQuery->ucRcpi);
        }

        sinfo->txrate.legacy = u4Rate / 1000;
        sinfo->signal = i4Rssi;

        DBGLOG(P2P, INFO, "ucRoleIdx = %d, rate = %u, signal = %d\n", ucRoleIdx,
               sinfo->txrate.legacy, sinfo->signal);

        sinfo->filled |= BIT(NL80211_STA_INFO_TX_BITRATE);
        sinfo->filled |= BIT(NL80211_STA_INFO_SIGNAL);

        i4RetRslt = 0;
    } while (false);

    return i4RetRslt;
}

int mtk_p2p_cfg80211_scan(struct wiphy *wiphy,
                          struct cfg80211_scan_request *request)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    P_GL_P2P_INFO_T prP2pGlueInfo = (P_GL_P2P_INFO_T)NULL;
    P_GL_P2P_DEV_INFO_T prP2pGlueDevInfo = (P_GL_P2P_DEV_INFO_T)NULL;
    P_MSG_P2P_SCAN_REQUEST_T prMsgScanRequest = (P_MSG_P2P_SCAN_REQUEST_T)NULL;
    u32 u4MsgSize = 0, u4Idx = 0;
    u32 rStatus, u4SetInfoLen = 0;
    s32 i4RetRslt = -EINVAL;
    P_RF_CHANNEL_INFO_T prRfChannelInfo = (P_RF_CHANNEL_INFO_T)NULL;
    P_P2P_SSID_STRUCT_T prSsidStruct = (P_P2P_SSID_STRUCT_T)NULL;
    struct ieee80211_channel *prChannel = NULL;
    struct cfg80211_ssid *prSsid = NULL;
    u8 ucBssIdx = 0;
    u8 fgIsFullChanScan = false;

    /* [---------Channel---------]
     * [---------SSID---------][---------IE---------] */

    do {
        if ((wiphy == NULL) || (request == NULL)) {
            break;
        }

        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        if (wlanIsChipAssert(prGlueInfo->prAdapter)) {
            break;
        }

        prP2pGlueInfo = prGlueInfo->prP2PInfo[0];
        prP2pGlueDevInfo = prGlueInfo->prP2PDevInfo;

        if ((prP2pGlueInfo == NULL) || (prP2pGlueDevInfo == NULL)) {
            ASSERT(false);
            break;
        }

        DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_scan.\n");

        if (prP2pGlueDevInfo->prScanRequest != NULL) {
            /* There have been a scan request on-going processing.
             */
            DBGLOG(P2P, TRACE,
                   "There have been a scan request on-going processing.\n");
            break;
        }

        prP2pGlueDevInfo->prScanRequest = request;

        /* Should find out why the n_channels so many? */
        if (request->n_channels > MAXIMUM_OPERATION_CHANNEL_LIST) {
            request->n_channels = MAXIMUM_OPERATION_CHANNEL_LIST;
            fgIsFullChanScan = true;
            DBGLOG(P2P, TRACE, "Channel list exceed the maximun support.\n");
        }
        /* TODO: */
        /* Find a way to distinct DEV port scan & ROLE port scan.
         */
        ucBssIdx = P2P_DEV_BSS_INDEX;
        DBGLOG(P2P, TRACE, "Device Port Scan.\n");

        u4MsgSize = sizeof(MSG_P2P_SCAN_REQUEST_T) +
                    (request->n_channels * sizeof(RF_CHANNEL_INFO_T)) +
                    (request->n_ssids * sizeof(PARAM_SSID_T)) + request->ie_len;

        prMsgScanRequest =
            cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG, u4MsgSize);

        if (prMsgScanRequest == NULL) {
            ASSERT(false);
            i4RetRslt = -ENOMEM;
            break;
        }

        DBGLOG(P2P, TRACE, "Generating scan request message.\n");

        prMsgScanRequest->rMsgHdr.eMsgId = MID_MNY_P2P_DEVICE_DISCOVERY;
        prMsgScanRequest->eScanType = SCAN_TYPE_ACTIVE_SCAN;
        prMsgScanRequest->ucBssIdx = ucBssIdx;

        DBGLOG(P2P, INFO, "Requesting channel number:%d.\n",
               request->n_channels);

        for (u4Idx = 0; u4Idx < request->n_channels; u4Idx++) {
            /* Translate Freq from MHz to channel number. */
            prRfChannelInfo = &(prMsgScanRequest->arChannelListInfo[u4Idx]);
            prChannel = request->channels[u4Idx];

            prRfChannelInfo->ucChannelNum =
                nicFreq2ChannelNum(prChannel->center_freq * 1000);
            DBGLOG(P2P, TRACE, "Scanning Channel:%d,  freq: %d\n",
                   prRfChannelInfo->ucChannelNum, prChannel->center_freq);
            switch (prChannel->band) {
            case NL80211_BAND_2GHZ:
                prRfChannelInfo->eBand = BAND_2G4;
                break;

            case NL80211_BAND_5GHZ:
                prRfChannelInfo->eBand = BAND_5G;
                break;

            default:
                DBGLOG(P2P, TRACE, "UNKNOWN Band info from supplicant\n");
                prRfChannelInfo->eBand = BAND_NULL;
                break;
            }

            /* Iteration. */
            prRfChannelInfo++;
        }
        prMsgScanRequest->u4NumChannel = request->n_channels;
        if (fgIsFullChanScan) {
            prMsgScanRequest->u4NumChannel = SCN_P2P_FULL_SCAN_PARAM;
            DBGLOG(P2P, INFO,
                   "request->n_channels = SCN_P2P_FULL_SCAN_PARAM\n");
        }
        DBGLOG(P2P, TRACE, "Finish channel list.\n");

        /* SSID */
        prSsid = request->ssids;
        prSsidStruct = (P_P2P_SSID_STRUCT_T)prRfChannelInfo;
        if (request->n_ssids) {
            ASSERT(
                (unsigned long)prSsidStruct ==
                (unsigned long)&(prMsgScanRequest->arChannelListInfo[u4Idx]));
            prMsgScanRequest->prSSID = prSsidStruct;
        }

        for (u4Idx = 0; u4Idx < request->n_ssids; u4Idx++) {
            COPY_SSID(prSsidStruct->aucSsid, prSsidStruct->ucSsidLen,
                      request->ssids->ssid, request->ssids->ssid_len);

            prSsidStruct++;
            prSsid++;
        }

        prMsgScanRequest->i4SsidNum = request->n_ssids;

        DBGLOG(P2P, TRACE, "Finish SSID list:%d.\n", request->n_ssids);

        /* IE BUFFERS */
        prMsgScanRequest->pucIEBuf = (u8 *)prSsidStruct;
        if (request->ie_len) {
            kalMemCopy(prMsgScanRequest->pucIEBuf, request->ie,
                       request->ie_len);
            prMsgScanRequest->u4IELen = request->ie_len;
        } else {
            prMsgScanRequest->u4IELen = 0;
        }

        DBGLOG(P2P, TRACE, "Finish IE Buffer.\n");

        /* Abort previous scan */
        rStatus = kalIoctl(prGlueInfo, wlanoidAbortP2pScan, &ucBssIdx,
                           sizeof(ucBssIdx), false, false, true, &u4SetInfoLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(REQ, ERROR, "mtk_p2p_cfg80211_scan abort scan fail 0x%x\n",
                   rStatus);
        }

        prP2pGlueDevInfo->prScanRequest = request;

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prMsgScanRequest, MSG_SEND_METHOD_BUF);

#if 0
        /* Backup scan request structure */
        /* The purpose of this backup is due to the kernel free the scan
         * req */
        /* when the wpa supplicant down the iface before down, and it
         * will */
        /* free the original scan request structure */
        /* In this case, the scan resoure could be locked by kernel, and
         */
        /* driver needs this work around to clear the state */
        kalMemCopy(&(prP2pGlueDevInfo->rBackupScanRequest), request,
                   sizeof(struct cfg80211_scan_request));
#endif

        i4RetRslt = 0;
    } while (false);

    return i4RetRslt;
}

void mtk_p2p_cfg80211_abort_scan(struct wiphy *wiphy,
                                 struct wireless_dev *wdev)
{
    u32 u4SetInfoLen = 0;
    u32 u4Value = 0;
    u32 rStatus;
    P_GLUE_INFO_T prGlueInfo = NULL;

    DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_abort_scan\n");

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    rStatus = kalIoctl(prGlueInfo, wlanoidAbortP2pScan, &u4Value,
                       sizeof(u4Value), false, false, true, &u4SetInfoLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "mtk_p2p_cfg80211_abort_scan fail 0x%x\n", rStatus);
    }
}

int mtk_p2p_cfg80211_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
    s32 i4Rslt = -EINVAL;
    P_GLUE_INFO_T prGlueInfo = NULL;

    do {
        if (wiphy == NULL) {
            break;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_set_wiphy_params\n");
        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        if (changed & WIPHY_PARAM_RETRY_SHORT) {
            /* TODO: */
            DBGLOG(P2P, TRACE, "The RETRY short param is changed.\n");
        }

        if (changed & WIPHY_PARAM_RETRY_LONG) {
            /* TODO: */
            DBGLOG(P2P, TRACE, "The RETRY long param is changed.\n");
        }

        if (changed & WIPHY_PARAM_FRAG_THRESHOLD) {
            /* TODO: */
            DBGLOG(P2P, TRACE,
                   "The RETRY fragmentation threshold is changed.\n");
        }

        if (changed & WIPHY_PARAM_RTS_THRESHOLD) {
            /* TODO: */
            DBGLOG(P2P, TRACE, "The RETRY RTS threshold is changed.\n");
        }

        if (changed & WIPHY_PARAM_COVERAGE_CLASS) {
            /* TODO: */
            DBGLOG(P2P, TRACE, "The coverage class is changed???\n");
        }

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

int mtk_p2p_cfg80211_join_ibss(struct wiphy *wiphy, struct net_device *dev,
                               struct cfg80211_ibss_params *params)
{
    P_GLUE_INFO_T prGlueInfo = NULL;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    /* not implemented yet */

    return -EINVAL;
}

int mtk_p2p_cfg80211_leave_ibss(struct wiphy *wiphy, struct net_device *dev)
{
    P_GLUE_INFO_T prGlueInfo = NULL;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    /* not implemented yet */

    return -EINVAL;
}

int mtk_p2p_cfg80211_set_txpower(struct wiphy *wiphy, struct wireless_dev *wdev,
                                 enum nl80211_tx_power_setting type, int mbm)
{
    P_GLUE_INFO_T prGlueInfo = NULL;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    /* not implemented yet */

    return -EINVAL;
}

int mtk_p2p_cfg80211_get_txpower(struct wiphy *wiphy, struct wireless_dev *wdev,
                                 int *dbm)
{
    P_GLUE_INFO_T prGlueInfo = NULL;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    /* not implemented yet */

    return -EINVAL;
}

int mtk_p2p_cfg80211_set_power_mgmt(struct wiphy *wiphy,
                                    struct net_device *ndev, bool enabled,
                                    int timeout)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    PARAM_POWER_MODE ePowerMode;
    PARAM_POWER_MODE_T rPowerMode;
    u32 u4Leng;
    u8 ucRoleIdx;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    if (enabled) {
        ePowerMode = Param_PowerModeFast_PSP;
    } else {
        ePowerMode = Param_PowerModeCAM;
    }

    DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_set_power_mgmt ps=%d.\n", enabled);

    if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev, &ucRoleIdx) != 0) {
        return -EINVAL;
    }

    if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx,
                            &rPowerMode.ucBssIdx) != WLAN_STATUS_SUCCESS) {
        return -EINVAL;
    }

    rPowerMode.ePowerMode = ePowerMode;

    /* p2p_set_power_save */
    kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile, &rPowerMode,
             sizeof(rPowerMode), false, false, true, &u4Leng);

    return 0;
}

/* &&&&&&&&&&&&&&&&&&&&&&&&&& Add for ICS Wi-Fi Direct Support.
 * &&&&&&&&&&&&&&&&&&&&&&& */
int mtk_p2p_cfg80211_start_ap(struct wiphy *wiphy, struct net_device *dev,
                              struct cfg80211_ap_settings *settings)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    s32 i4Rslt = -EINVAL;
    P_MSG_P2P_BEACON_UPDATE_T prP2pBcnUpdateMsg =
        (P_MSG_P2P_BEACON_UPDATE_T)NULL;
    P_MSG_P2P_START_AP_T prP2pStartAPMsg = (P_MSG_P2P_START_AP_T)NULL;
    u8 *pucBuffer = (u8 *)NULL;
    u8 ucRoleIdx = 0;
    struct cfg80211_chan_def *chandef;
    RF_CHANNEL_INFO_T rRfChnlInfo;

    /* RF_CHANNEL_INFO_T rRfChnlInfo; */
    /* P_IE_SSID_T prSsidIE = (P_IE_SSID_T)NULL; */

    do {
        if ((wiphy == NULL) || (settings == NULL)) {
            break;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_start_ap.\n");
        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

#if (CFG_SUPPORT_DFS_MASTER == 1)
        /*DFS todo 20161220_DFS*/
        netif_carrier_on(dev);
        netif_tx_start_all_queues(dev);
#endif

        chandef = &settings->chandef;

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
            break;
        }

        if (chandef) {
            mtk_p2p_cfg80211func_channel_format_switch(chandef, chandef->chan,
                                                       &rRfChnlInfo);

            DBGLOG(
                P2P, INFO,
                "=============== role(%d) b=%d f=%d w=%d s1=%d s2=%d ============ \n",
                ucRoleIdx, chandef->chan->band, chandef->chan->center_freq,
                chandef->width, chandef->center_freq1, chandef->center_freq2);

            /* Follow the channel info from wifi.cfg prior to
             * hostapd.conf */
            {
                P_ADAPTER_T prAdapter = (P_ADAPTER_T)NULL;
                P_WIFI_VAR_T prWifiVar = (P_WIFI_VAR_T)NULL;

                prAdapter = prGlueInfo->prAdapter;
                prWifiVar = &prAdapter->rWifiVar;

                if ((prWifiVar->ucApChannel != 0) &&
                    (prWifiVar->ucApChnlDefFromCfg != 0) &&
                    (prWifiVar->ucApChannel != rRfChnlInfo.ucChannelNum)) {
                    rRfChnlInfo.ucChannelNum = prWifiVar->ucApChannel;
                    rRfChnlInfo.eBand =
                        (rRfChnlInfo.ucChannelNum <= 14) ? BAND_2G4 : BAND_5G;
                    /* [TODO][20160829]If we will set SCO by
                     * nl80211_channel_type afterward, to
                     * check if we need to modify SCO by
                     * wifi.cfg here
                     */
                }
            }

            p2pFuncSetChannel(prGlueInfo->prAdapter, ucRoleIdx, &rRfChnlInfo);
        } else {
            DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_start_ap. !!! no CH def!!!\n");
        }

        prP2pBcnUpdateMsg = (P_MSG_P2P_BEACON_UPDATE_T)cnmMemAlloc(
            prGlueInfo->prAdapter, RAM_TYPE_MSG,
            (sizeof(MSG_P2P_BEACON_UPDATE_T) + settings->beacon.head_len +
             settings->beacon.tail_len + settings->beacon.assocresp_ies_len));

        if (prP2pBcnUpdateMsg == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prP2pBcnUpdateMsg->ucRoleIndex = ucRoleIdx;
        prP2pBcnUpdateMsg->rMsgHdr.eMsgId = MID_MNY_P2P_BEACON_UPDATE;
        pucBuffer = prP2pBcnUpdateMsg->aucBuffer;
        DBGLOG(P2P, STATE, "mtk_p2p_cfg80211_start_ap.(role %d)\n", ucRoleIdx);

#if (CFG_SUPPORT_DFS_MASTER == 1)
        if (p2pFuncGetDfsState() == DFS_STATE_DETECTED) {
            p2pFuncSetDfsState(DFS_STATE_INACTIVE);
        }
#endif
        if (settings->beacon.head_len != 0) {
            kalMemCopy(pucBuffer, settings->beacon.head,
                       settings->beacon.head_len);

            prP2pBcnUpdateMsg->u4BcnHdrLen = settings->beacon.head_len;

            prP2pBcnUpdateMsg->pucBcnHdr = pucBuffer;

            pucBuffer += settings->beacon.head_len;
        } else {
            prP2pBcnUpdateMsg->u4BcnHdrLen = 0;

            prP2pBcnUpdateMsg->pucBcnHdr = NULL;
        }

        if (settings->beacon.tail_len != 0) {
            u8 ucLen = settings->beacon.tail_len;

            prP2pBcnUpdateMsg->pucBcnBody = pucBuffer;
            kalMemCopy(pucBuffer, settings->beacon.tail,
                       settings->beacon.tail_len);

            prP2pBcnUpdateMsg->u4BcnBodyLen = ucLen;

            pucBuffer += settings->beacon.tail_len;
        } else {
            prP2pBcnUpdateMsg->u4BcnBodyLen = 0;

            prP2pBcnUpdateMsg->pucBcnBody = NULL;
        }

        if ((settings->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP40) ||
            (settings->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP104)) {
            prP2pBcnUpdateMsg->fgIsWepCipher = true;
        } else {
            prP2pBcnUpdateMsg->fgIsWepCipher = false;
        }

        if (settings->beacon.assocresp_ies_len != 0 &&
            settings->beacon.assocresp_ies != NULL) {
            prP2pBcnUpdateMsg->pucAssocRespIE = pucBuffer;
            kalMemCopy(pucBuffer, settings->beacon.assocresp_ies,
                       settings->beacon.assocresp_ies_len);
            prP2pBcnUpdateMsg->u4AssocRespLen =
                settings->beacon.assocresp_ies_len;
        } else {
            prP2pBcnUpdateMsg->u4AssocRespLen = 0;
            prP2pBcnUpdateMsg->pucAssocRespIE = NULL;
        }

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prP2pBcnUpdateMsg, MSG_SEND_METHOD_BUF);

        prP2pStartAPMsg = (P_MSG_P2P_START_AP_T)cnmMemAlloc(
            prGlueInfo->prAdapter, RAM_TYPE_MSG, sizeof(MSG_P2P_START_AP_T));

        if (prP2pStartAPMsg == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prP2pStartAPMsg->rMsgHdr.eMsgId = MID_MNY_P2P_START_AP;

        prP2pStartAPMsg->fgIsPrivacy = settings->privacy;

        prP2pStartAPMsg->u4BcnInterval = settings->beacon_interval;

        prP2pStartAPMsg->u4DtimPeriod = settings->dtim_period;

        /* Copy NO SSID. */
        prP2pStartAPMsg->ucHiddenSsidType = settings->hidden_ssid;

        prP2pStartAPMsg->ucRoleIdx = ucRoleIdx;

        kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);

        COPY_SSID(prP2pStartAPMsg->aucSsid, prP2pStartAPMsg->u2SsidLen,
                  settings->ssid, settings->ssid_len);

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prP2pStartAPMsg, MSG_SEND_METHOD_BUF);

        i4Rslt = 0;
    } while (false);

    return i4Rslt;

    /* /////////////////////// */
    /**
     * struct cfg80211_ap_settings - AP configuration
     *
     * Used to configure an AP interface.
     *
     * @beacon: beacon data
     * @beacon_interval: beacon interval
     * @dtim_period: DTIM period
     * @ssid: SSID to be used in the BSS (note: may be %NULL if not provided
     * from user space)
     * @ssid_len: length of @ssid
     * @hidden_ssid: whether to hide the SSID in Beacon/Probe Response
     * frames
     * @crypto: crypto settings
     * @privacy: the BSS uses privacy
     * @auth_type: Authentication type (algorithm)
     * @inactivity_timeout: time in seconds to determine station's
     * inactivity.
     */
    /* struct cfg80211_ap_settings { */
    /* struct cfg80211_beacon_data beacon; */
    /*  */
    /* int beacon_interval, dtim_period; */
    /* const u8 *ssid; */
    /* size_t ssid_len; */
    /* enum nl80211_hidden_ssid hidden_ssid; */
    /* struct cfg80211_crypto_settings crypto; */
    /* bool privacy; */
    /* enum nl80211_auth_type auth_type; */
    /* int inactivity_timeout; */
    /* }; */
    /* ////////////////// */
}

#if (CFG_SUPPORT_DFS_MASTER == 1)

static int mtk_p2p_cfg80211_start_radar_detection_impl(
    struct wiphy *wiphy, struct net_device *dev,
    struct cfg80211_chan_def *chandef, unsigned int cac_time_ms)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    s32 i4Rslt = -EINVAL;
    P_MSG_P2P_DFS_CAC_T prP2pDfsCacMsg = (P_MSG_P2P_DFS_CAC_T)NULL;
    u8 ucRoleIdx = 0;
    RF_CHANNEL_INFO_T rRfChnlInfo;

    do {
        if ((wiphy == NULL) || (chandef == NULL)) {
            break;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_start_radar_detection.\n");
        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
            break;
        }

        if (prGlueInfo->prP2PInfo[ucRoleIdx]->chandef == NULL) {
            prGlueInfo->prP2PInfo[ucRoleIdx]->chandef =
                (struct cfg80211_chan_def *)cnmMemAlloc(
                    prGlueInfo->prAdapter, RAM_TYPE_BUF,
                    sizeof(struct cfg80211_chan_def));

            prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan =
                (struct ieee80211_channel *)cnmMemAlloc(
                    prGlueInfo->prAdapter, RAM_TYPE_BUF,
                    sizeof(struct ieee80211_channel));
        }

        /* Copy chan def to local buffer*/
        prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->center_freq1 =
            chandef->center_freq1;
        prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->center_freq2 =
            chandef->center_freq2;
        prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->width = chandef->width;
        memcpy(prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan, chandef->chan,
               sizeof(struct ieee80211_channel));
        prGlueInfo->prP2PInfo[ucRoleIdx]->cac_time_ms = cac_time_ms;

        mtk_p2p_cfg80211func_channel_format_switch(chandef, chandef->chan,
                                                   &rRfChnlInfo);

        p2pFuncSetChannel(prGlueInfo->prAdapter, ucRoleIdx, &rRfChnlInfo);

        DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_start_radar_detection.(role %d)\n",
               ucRoleIdx);

        p2pFuncSetDfsState(DFS_STATE_INACTIVE);

        prP2pDfsCacMsg = (P_MSG_P2P_DFS_CAC_T)cnmMemAlloc(
            prGlueInfo->prAdapter, RAM_TYPE_MSG, sizeof(*prP2pDfsCacMsg));

        if (prP2pDfsCacMsg == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prP2pDfsCacMsg->rMsgHdr.eMsgId = MID_MNY_P2P_DFS_CAC;

        switch (chandef->width) {
        case NL80211_CHAN_WIDTH_20_NOHT:
        case NL80211_CHAN_WIDTH_20:
        case NL80211_CHAN_WIDTH_40:
            prP2pDfsCacMsg->eChannelWidth = CW_20_40MHZ;
            break;

        case NL80211_CHAN_WIDTH_80:
            prP2pDfsCacMsg->eChannelWidth = CW_80MHZ;
            break;

        case NL80211_CHAN_WIDTH_80P80:
            prP2pDfsCacMsg->eChannelWidth = CW_80P80MHZ;
            break;

        default:
            DBGLOG(
                P2P, ERROR,
                "mtk_p2p_cfg80211_start_radar_detection. !!!Bandwidth do not "
                "support!!!\n");
            ASSERT(false);
            break;
        }

        prP2pDfsCacMsg->ucRoleIdx = ucRoleIdx;

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prP2pDfsCacMsg, MSG_SEND_METHOD_BUF);

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

int mtk_p2p_cfg80211_start_radar_detection(struct wiphy *wiphy,
                                           struct net_device *dev,
                                           struct cfg80211_chan_def *chandef,
                                           unsigned int cac_time_ms
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
                                           ,
                                           int link_id
#endif
                                           )
{
    return mtk_p2p_cfg80211_start_radar_detection_impl(wiphy, dev, chandef,
                                                       cac_time_ms);
}

int mtk_p2p_cfg80211_channel_switch(struct wiphy *wiphy, struct net_device *dev,
                                    struct cfg80211_csa_settings *params)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    s32 i4Rslt = -EINVAL;
    P_MSG_P2P_BEACON_UPDATE_T prP2pBcnUpdateMsg =
        (P_MSG_P2P_BEACON_UPDATE_T)NULL;
    P_MSG_P2P_SET_NEW_CHANNEL_T prP2pSetNewChannelMsg =
        (P_MSG_P2P_SET_NEW_CHANNEL_T)NULL;
    u8 *pucBuffer = (u8 *)NULL;
    u8 ucRoleIdx = 0;
    RF_CHANNEL_INFO_T rRfChnlInfo;
    P_BSS_INFO_T prBssInfo;
    u8 ucBssIndex;
    u32 u4Len;

    do {
        if ((wiphy == NULL) || (params == NULL)) {
            break;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_channel_switch.\n");
        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
            break;
        }

        /*DFS todo 20161220_DFS*/
        netif_carrier_on(dev);
        netif_tx_start_all_queues(dev);

        if (prGlueInfo->prP2PInfo[ucRoleIdx]->chandef == NULL) {
            prGlueInfo->prP2PInfo[ucRoleIdx]->chandef =
                (struct cfg80211_chan_def *)cnmMemAlloc(
                    prGlueInfo->prAdapter, RAM_TYPE_BUF,
                    sizeof(struct cfg80211_chan_def));

            prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan =
                (struct ieee80211_channel *)cnmMemAlloc(
                    prGlueInfo->prAdapter, RAM_TYPE_BUF,
                    sizeof(struct ieee80211_channel));
        }
        /* Copy chan def to local buffer*/
        prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->center_freq1 =
            params->chandef.center_freq1;
        prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->center_freq2 =
            params->chandef.center_freq2;
        prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->width =
            params->chandef.width;
        memcpy(prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan,
               params->chandef.chan, sizeof(struct ieee80211_channel));

        mtk_p2p_cfg80211func_channel_format_switch(
            &params->chandef, params->chandef.chan, &rRfChnlInfo);

        p2pFuncSetChannel(prGlueInfo->prAdapter, ucRoleIdx, &rRfChnlInfo);

        DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_channel_switch.(role %d)\n",
               ucRoleIdx);

        p2pFuncSetDfsState(DFS_STATE_INACTIVE);

        /* Set CSA IE parameters */
        prGlueInfo->prAdapter->rWifiVar.fgCsaInProgress = true;
        prGlueInfo->prAdapter->rWifiVar.ucChannelSwitchMode = 1;
        prGlueInfo->prAdapter->rWifiVar.ucNewChannelNumber =
            nicFreq2ChannelNum(params->chandef.chan->center_freq * 1000);
        prGlueInfo->prAdapter->rWifiVar.ucChannelSwitchCount = params->count;

        /* Set new channel parameters */
        prP2pSetNewChannelMsg = (P_MSG_P2P_SET_NEW_CHANNEL_T)cnmMemAlloc(
            prGlueInfo->prAdapter, RAM_TYPE_MSG,
            sizeof(*prP2pSetNewChannelMsg));

        if (prP2pSetNewChannelMsg == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prP2pSetNewChannelMsg->rMsgHdr.eMsgId = MID_MNY_P2P_SET_NEW_CHANNEL;

        switch (params->chandef.width) {
        case NL80211_CHAN_WIDTH_20_NOHT:
        case NL80211_CHAN_WIDTH_20:
        case NL80211_CHAN_WIDTH_40:
            prP2pSetNewChannelMsg->eChannelWidth = CW_20_40MHZ;
            break;

        case NL80211_CHAN_WIDTH_80:
            prP2pSetNewChannelMsg->eChannelWidth = CW_80MHZ;
            break;

        case NL80211_CHAN_WIDTH_80P80:
            prP2pSetNewChannelMsg->eChannelWidth = CW_80P80MHZ;
            break;

        default:
            DBGLOG(
                P2P, ERROR,
                "mtk_p2p_cfg80211_channel_switch. !!!Bandwidth do not support!!!\n");
            ASSERT(false);
            break;
        }

        prP2pSetNewChannelMsg->ucRoleIdx = ucRoleIdx;

        if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx,
                                &ucBssIndex) != WLAN_STATUS_SUCCESS) {
            DBGLOG(P2P, ERROR, "Get bss index fail by role(%d)\n", ucRoleIdx);
            break;
        } else {
            DBGLOG(P2P, INFO,
                   "mtk_p2p_cfg80211_channel_switch: Get BssIdx:%d\n",
                   ucBssIndex);

            prBssInfo =
                GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);

#if CFG_SUPPORT_DBDC_TC6
            if (prBssInfo && IS_BSS_P2P(prBssInfo) &&
                p2pFuncIsAPMode(
                    prGlueInfo->prAdapter->rWifiVar
                    .prP2PConnSettings[prBssInfo->u4PrivateData]) &&
                IS_NET_PWR_STATE_ACTIVE(prGlueInfo->prAdapter,
                                        prBssInfo->ucBssIndex)) {
                prP2pSetNewChannelMsg->ucBssIndex = ucBssIndex;
            } else {
                DBGLOG(P2P, ERROR, "Bss is not in AP mode or not active\n");
                break;
            }
#else
            if (prBssInfo && prBssInfo->fgIsDfsActive) {
                /* Only support switching to DFS Active BSS
                 * which has been CAC done */
                DBGLOG(
                    P2P, INFO,
                    "mtk_p2p_cfg80211_channel_switch: DFS Active BssIdx:%d\n",
                    ucBssIndex);
                prP2pSetNewChannelMsg->ucBssIndex = ucBssIndex;
            } else {
                DBGLOG(P2P, ERROR, "Bss is not DFS Active\n");
                break;
            }
#endif
        }

        DBGLOG(P2P, STATE,
               "mtk_p2p_cfg80211_channel_switch: SetNewChnl BssIdx:%d\n",
               ucBssIndex);

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prP2pSetNewChannelMsg, MSG_SEND_METHOD_BUF);

        /* Update beacon */
        if ((params->beacon_csa.head_len != 0) ||
            (params->beacon_csa.tail_len != 0)) {
            u4Len = sizeof(MSG_P2P_BEACON_UPDATE_T) +
                    params->beacon_csa.head_len + params->beacon_csa.tail_len;
            prP2pBcnUpdateMsg = (P_MSG_P2P_BEACON_UPDATE_T)cnmMemAlloc(
                prGlueInfo->prAdapter, RAM_TYPE_MSG, u4Len);

            if (prP2pBcnUpdateMsg == NULL) {
                ASSERT(false);
                i4Rslt = -ENOMEM;
                break;
            }
            kalMemZero(prP2pBcnUpdateMsg, u4Len);

            prP2pBcnUpdateMsg->ucRoleIndex = ucRoleIdx;
            prP2pBcnUpdateMsg->rMsgHdr.eMsgId = MID_MNY_P2P_BEACON_UPDATE;
            pucBuffer = prP2pBcnUpdateMsg->aucBuffer;

            if (params->beacon_csa.head_len != 0) {
                kalMemCopy(pucBuffer, params->beacon_csa.head,
                           params->beacon_csa.head_len);

                prP2pBcnUpdateMsg->u4BcnHdrLen = params->beacon_csa.head_len;

                prP2pBcnUpdateMsg->pucBcnHdr = pucBuffer;

                pucBuffer = (u8 *)((unsigned long)pucBuffer +
                                   (unsigned long)params->beacon_csa.head_len);
            } else {
                prP2pBcnUpdateMsg->u4BcnHdrLen = 0;

                prP2pBcnUpdateMsg->pucBcnHdr = NULL;
            }

            if (params->beacon_csa.tail_len != 0) {
                u8 ucLen = params->beacon_csa.tail_len;

                prP2pBcnUpdateMsg->pucBcnBody = pucBuffer;
                kalMemCopy(pucBuffer, params->beacon_csa.tail,
                           params->beacon_csa.tail_len);

                prP2pBcnUpdateMsg->u4BcnBodyLen = ucLen;
            } else {
                prP2pBcnUpdateMsg->u4BcnBodyLen = 0;
                prP2pBcnUpdateMsg->pucBcnBody = NULL;
            }

            kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);

            mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                        (P_MSG_HDR_T)prP2pBcnUpdateMsg, MSG_SEND_METHOD_BUF);

            i4Rslt = 0;  /* Return Success */
        }
    } while (false);

    return i4Rslt;
}

#endif

int mtk_p2p_cfg80211_change_beacon(struct wiphy *wiphy, struct net_device *dev,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0))
                                   struct cfg80211_beacon_data *info
#else
                                   struct cfg80211_ap_update *info
#endif
                                   )
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0))
    struct cfg80211_beacon_data *beacon = info;
#else
    struct cfg80211_beacon_data *beacon = &info->beacon;
#endif
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    s32 i4Rslt = -EINVAL;
    P_MSG_P2P_BEACON_UPDATE_T prP2pBcnUpdateMsg =
        (P_MSG_P2P_BEACON_UPDATE_T)NULL;
    u8 *pucBuffer = (u8 *)NULL;
    u8 ucRoleIdx = 0;
    u32 u4Len = 0;

    do {
        if ((wiphy == NULL) || (beacon == NULL)) {
            break;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_change_beacon.\n");
        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
            break;
        }

        if ((beacon->head_len != 0) || (beacon->tail_len != 0)) {
            u4Len = sizeof(MSG_P2P_BEACON_UPDATE_T) + beacon->head_len +
                    beacon->tail_len + beacon->assocresp_ies_len;

            prP2pBcnUpdateMsg = (P_MSG_P2P_BEACON_UPDATE_T)cnmMemAlloc(
                prGlueInfo->prAdapter, RAM_TYPE_MSG, u4Len);

            if (prP2pBcnUpdateMsg == NULL) {
                ASSERT(false);
                i4Rslt = -ENOMEM;
                break;
            }
            kalMemZero(prP2pBcnUpdateMsg, u4Len);

            prP2pBcnUpdateMsg->ucRoleIndex = ucRoleIdx;
            prP2pBcnUpdateMsg->rMsgHdr.eMsgId = MID_MNY_P2P_BEACON_UPDATE;
            pucBuffer = prP2pBcnUpdateMsg->aucBuffer;

            if (beacon->head_len != 0) {
                kalMemCopy(pucBuffer, beacon->head, beacon->head_len);

                prP2pBcnUpdateMsg->u4BcnHdrLen = beacon->head_len;

                prP2pBcnUpdateMsg->pucBcnHdr = pucBuffer;

                pucBuffer += beacon->head_len;
            } else {
                prP2pBcnUpdateMsg->u4BcnHdrLen = 0;

                prP2pBcnUpdateMsg->pucBcnHdr = NULL;
            }

            if (beacon->tail_len != 0) {
                u8 ucLen = beacon->tail_len;

                prP2pBcnUpdateMsg->pucBcnBody = pucBuffer;
                kalMemCopy(pucBuffer, beacon->tail, beacon->tail_len);

                prP2pBcnUpdateMsg->u4BcnBodyLen = ucLen;

                pucBuffer += beacon->tail_len;
            } else {
                prP2pBcnUpdateMsg->u4BcnBodyLen = 0;
                prP2pBcnUpdateMsg->pucBcnBody = NULL;
            }

            if (beacon->assocresp_ies_len != 0 &&
                beacon->assocresp_ies != NULL) {
                prP2pBcnUpdateMsg->pucAssocRespIE = pucBuffer;
                kalMemCopy(pucBuffer, beacon->assocresp_ies,
                           beacon->assocresp_ies_len);
                prP2pBcnUpdateMsg->u4AssocRespLen = beacon->assocresp_ies_len;
            } else {
                prP2pBcnUpdateMsg->u4AssocRespLen = 0;
                prP2pBcnUpdateMsg->pucAssocRespIE = NULL;
            }

            kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);

            mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                        (P_MSG_HDR_T)prP2pBcnUpdateMsg, MSG_SEND_METHOD_BUF);

            i4Rslt = 0;  /* Return Success */
        }

        /* TODO: Probe Rsp, Assoc Rsp, Beacon IE update. */

        /* ////////////////////////// */
        /**
         * struct cfg80211_beacon_data - beacon data
         * @head: head portion of beacon (before TIM IE)
         *     or %NULL if not changed
         * @tail: tail portion of beacon (after TIM IE)
         *     or %NULL if not changed
         * @head_len: length of @head
         * @tail_len: length of @tail
         * @beacon_ies: extra information element(s) to add into Beacon
         * frames or %NULL
         * @beacon_ies_len: length of beacon_ies in octets
         * @proberesp_ies: extra information element(s) to add into
         * Probe Response frames or %NULL
         * @proberesp_ies_len: length of proberesp_ies in octets
         * @assocresp_ies: extra information element(s) to add into
         * (Re)Association Response frames or %NULL
         * @assocresp_ies_len: length of assocresp_ies in octets
         * @probe_resp_len: length of probe response template
         * (@probe_resp)
         * @probe_resp: probe response template (AP mode only)
         */
        /* struct cfg80211_beacon_data { */
        /* const u8 *head, *tail; */
        /* const u8 *beacon_ies; */
        /* const u8 *proberesp_ies; */
        /* const u8 *assocresp_ies; */
        /* const u8 *probe_resp; */

        /* size_t head_len, tail_len; */
        /* size_t beacon_ies_len; */
        /* size_t proberesp_ies_len; */
        /* size_t assocresp_ies_len; */
        /* size_t probe_resp_len; */
        /* }; */

        /* ////////////////////////// */
    } while (false);

    return i4Rslt;
}

int mtk_p2p_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *dev
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
                             ,
                             unsigned int link_id
#endif
                             )
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    s32 i4Rslt = -EINVAL;
    P_MSG_P2P_SWITCH_OP_MODE_T prP2pSwitchMode =
        (P_MSG_P2P_SWITCH_OP_MODE_T)NULL;
    u8 ucRoleIdx = 0;

    do {
        if (wiphy == NULL) {
            break;
        }

        DBGLOG(P2P, STATE, "mtk_p2p_cfg80211_stop_ap.\n");
        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

#if (CFG_SUPPORT_DFS_MASTER == 1)
        if (dev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP) {
            netif_carrier_off(dev);
            netif_tx_stop_all_queues(dev);
        }
#endif

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
            break;
        }
        /* Switch OP MOde. */
        prP2pSwitchMode = cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
                                      sizeof(MSG_P2P_SWITCH_OP_MODE_T));

        if (prP2pSwitchMode == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prP2pSwitchMode->rMsgHdr.eMsgId = MID_MNY_P2P_STOP_AP;
        prP2pSwitchMode->ucRoleIdx = ucRoleIdx;

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prP2pSwitchMode, MSG_SEND_METHOD_BUF);

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

/* TODO: */
int mtk_p2p_cfg80211_deauth(struct wiphy *wiphy, struct net_device *dev,
                            struct cfg80211_deauth_request *req)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    /* not implemented yet */
    DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_deauth.\n");

    return -EINVAL;
}

/* TODO: */
int mtk_p2p_cfg80211_disassoc(struct wiphy *wiphy, struct net_device *dev,
                              struct cfg80211_disassoc_request *req)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_disassoc.\n");

    /* not implemented yet */

    return -EINVAL;
}

int mtk_p2p_cfg80211_remain_on_channel(struct wiphy *wiphy,
                                       struct wireless_dev *wdev,
                                       struct ieee80211_channel *chan,
                                       unsigned int duration, u64 *cookie)
{
    s32 i4Rslt = -EINVAL;
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    P_GL_P2P_DEV_INFO_T prGlueP2pDevInfo = (P_GL_P2P_DEV_INFO_T)NULL;
    P_MSG_P2P_CHNL_REQUEST_T prMsgChnlReq = (P_MSG_P2P_CHNL_REQUEST_T)NULL;

    DBGLOG(P2P, STATE, "mtk_p2p_cfg80211_remain_on_channel\n");

    do {
        if ((wiphy == NULL) ||
            /* (dev == NULL) || */
            (chan == NULL) || (cookie == NULL)) {
            break;
        }

        P2P_WIPHY_PRIV(wiphy, prGlueInfo);
        prGlueP2pDevInfo = prGlueInfo->prP2PDevInfo;

        *cookie = prGlueP2pDevInfo->u8Cookie++;

        prMsgChnlReq = cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
                                   sizeof(MSG_P2P_CHNL_REQUEST_T));

        if (prMsgChnlReq == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_remain_on_channel:%d\n",
               (s32) * cookie);

        prMsgChnlReq->rMsgHdr.eMsgId = MID_MNY_P2P_CHNL_REQ;
        prMsgChnlReq->u8Cookie = *cookie;
        prMsgChnlReq->u4Duration = duration;
        prMsgChnlReq->eChnlReqType = CH_REQ_TYPE_P2P_LISTEN;

        mtk_p2p_cfg80211func_channel_format_switch(NULL, chan,
                                                   &prMsgChnlReq->rChannelInfo);
        mtk_p2p_cfg80211func_channel_sco_switch(NL80211_CHAN_NO_HT,
                                                &prMsgChnlReq->eChnlSco);

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prMsgChnlReq,
                    MSG_SEND_METHOD_BUF);

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

/* mtk_p2p_cfg80211_remain_on_channel */

int mtk_p2p_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy,
                                              struct wireless_dev *wdev,
                                              u64 cookie)
{
    s32 i4Rslt = -EINVAL;
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;
    P_MSG_P2P_CHNL_ABORT_T prMsgChnlAbort = (P_MSG_P2P_CHNL_ABORT_T)NULL;

    do {
        if ((wiphy == NULL)  /* || (dev == NULL) */) {
            break;
        }

        P2P_WIPHY_PRIV(wiphy, prGlueInfo);
        prGlueP2pInfo = prGlueInfo->prP2PInfo[0];

        prMsgChnlAbort = cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
                                     sizeof(MSG_P2P_CHNL_ABORT_T));

        if (prMsgChnlAbort == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_cancel_remain_on_channel%d\n",
               (s32)cookie);

        prMsgChnlAbort->rMsgHdr.eMsgId = MID_MNY_P2P_CHNL_ABORT;
        prMsgChnlAbort->u8Cookie = cookie;

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prMsgChnlAbort, MSG_SEND_METHOD_BUF);

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

int mtk_p2p_cfg80211_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev,
                             struct cfg80211_mgmt_tx_params *params,
                             u64 *cookie)
{
    struct net_device *dev = NULL;
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;
    s32 i4Rslt = -EINVAL;
    P_MSG_P2P_MGMT_TX_REQUEST_T prMsgTxReq = (P_MSG_P2P_MGMT_TX_REQUEST_T)NULL;
    P_MSDU_INFO_T prMgmtFrame = (P_MSDU_INFO_T)NULL;
    u8 *pucFrameBuf = (u8 *)NULL;
    u64 *pu8GlCookie = (u64 *)NULL;
    u8 ucRoleIdx = 0, ucBssIdx = 0;

    do {
        if ((wiphy == NULL) || (wdev == NULL) || (params == 0) ||
            (cookie == NULL)) {
            break;
        }

        P2P_WIPHY_PRIV(wiphy, prGlueInfo);
        /* The owner of this function please check following line*/
        prGlueP2pInfo = (P_GL_P2P_INFO_T)prGlueInfo->prP2PInfo;

        dev = wdev->netdev;

        /* The owner of this function please check following line*/
        if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
            /* Device Interface. */
            ucBssIdx = P2P_DEV_BSS_INDEX;
        } else {
            ASSERT(ucRoleIdx < KAL_P2P_NUM);
            /* Role Interface. */
            if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx,
                                    &ucBssIdx) != WLAN_STATUS_SUCCESS) {
                /* Can't find BSS index. */
                break;
            }
        }
        /* The owner of this function please check following line*/
        *cookie = prGlueInfo->u8Cookie++;

        /* Channel & Channel Type & Wait time are ignored. */
        prMsgTxReq = cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
                                 sizeof(MSG_P2P_MGMT_TX_REQUEST_T));

        if (prMsgTxReq == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        if (params->offchan) {
            DBGLOG(P2P, TRACE, " Off channel true\n");
            prMsgTxReq->fgIsOffChannel = true;

            mtk_p2p_cfg80211func_channel_format_switch(
                NULL, params->chan, &prMsgTxReq->rChannelInfo);
            mtk_p2p_cfg80211func_channel_sco_switch(NL80211_CHAN_NO_HT,
                                                    &prMsgTxReq->eChnlExt);
        } else {
            prMsgTxReq->fgIsOffChannel = false;
        }

        if (params->no_cck) {
            prMsgTxReq->fgNoneCckRate = true;
        } else {
            prMsgTxReq->fgNoneCckRate = false;
        }

        if (params->dont_wait_for_ack) {
            prMsgTxReq->fgIsWaitRsp = false;
        } else {
            prMsgTxReq->fgIsWaitRsp = true;
        }
        prMgmtFrame = cnmMgtPktAlloc(
            prGlueInfo->prAdapter,
            (s32)(params->len + sizeof(u64) + MAC_TX_RESERVED_FIELD));
        prMsgTxReq->prMgmtMsduInfo = prMgmtFrame;
        if (prMsgTxReq->prMgmtMsduInfo == NULL) {
            /* ASSERT(false); */
            i4Rslt = -ENOMEM;
            break;
        }

        prMsgTxReq->u8Cookie = *cookie;
        prMsgTxReq->rMsgHdr.eMsgId = MID_MNY_P2P_MGMT_TX;
        prMsgTxReq->ucBssIdx = ucBssIdx;

        pucFrameBuf = (u8 *)((unsigned long)prMgmtFrame->prPacket +
                             MAC_TX_RESERVED_FIELD);

        pu8GlCookie =
            (u64 *)((unsigned long)prMgmtFrame->prPacket +
                    (unsigned long)params->len + MAC_TX_RESERVED_FIELD);

        kalMemCopy(pucFrameBuf, params->buf, params->len);

        *pu8GlCookie = *cookie;

        prMgmtFrame->u2FrameLength = params->len;

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prMsgTxReq,
                    MSG_SEND_METHOD_BUF);

        i4Rslt = 0;
    } while (false);

    if ((i4Rslt != 0) && (prMsgTxReq != NULL)) {
        if (prMsgTxReq->prMgmtMsduInfo != NULL) {
            cnmMgtPktFree(prGlueInfo->prAdapter, prMsgTxReq->prMgmtMsduInfo);
        }

        cnmMemFree(prGlueInfo->prAdapter, prMsgTxReq);
    }

    return i4Rslt;
}

int mtk_p2p_cfg80211_mgmt_tx_cancel_wait(struct wiphy *wiphy,
                                         struct wireless_dev *wdev, u64 cookie)
{
    s32 i4Rslt = -EINVAL;

    return i4Rslt;
}

int mtk_p2p_cfg80211_change_bss(struct wiphy *wiphy, struct net_device *dev,
                                struct bss_parameters *params)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    s32 i4Rslt = -EINVAL;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    switch (params->use_cts_prot) {
    case -1:
        DBGLOG(P2P, TRACE, "CTS protection no change\n");
        break;

    case 0:
        DBGLOG(P2P, TRACE, "CTS protection disable.\n");
        break;

    case 1:
        DBGLOG(P2P, TRACE, "CTS protection enable\n");
        break;

    default:
        DBGLOG(P2P, TRACE, "CTS protection unknown\n");
        break;
    }

    switch (params->use_short_preamble) {
    case -1:
        DBGLOG(P2P, TRACE, "Short prreamble no change\n");
        break;

    case 0:
        DBGLOG(P2P, TRACE, "Short prreamble disable.\n");
        break;

    case 1:
        DBGLOG(P2P, TRACE, "Short prreamble enable\n");
        break;

    default:
        DBGLOG(P2P, TRACE, "Short prreamble unknown\n");
        break;
    }

    i4Rslt = 0;

    return i4Rslt;
}

static const u8 bcast_addr[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
int mtk_p2p_cfg80211_del_station(struct wiphy *wiphy, struct net_device *dev,
                                 struct station_del_parameters *params)
{
    const u8 *mac = params->mac ? params->mac : bcast_addr;
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    s32 i4Rslt = -EINVAL;
    P_MSG_P2P_CONNECTION_ABORT_T prDisconnectMsg =
        (P_MSG_P2P_CONNECTION_ABORT_T)NULL;
    u8 aucBcMac[] = BC_MAC_ADDR;
    u8 ucRoleIdx = 0;
    u8 ucBssIdx = 0;
    u32 waitRet = 0;
    P_BSS_INFO_T prBssInfo = NULL;
    P_STA_RECORD_T prCurrStaRec = NULL;

    do {
        if ((wiphy == NULL) || (dev == NULL)) {
            break;
        }

        if (mac == NULL) {
            mac = aucBcMac;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_del_station.\n");

        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
            break;
        }
        /* prDisconnectMsg =
         * (P_MSG_P2P_CONNECTION_ABORT_T)kalMemAlloc(sizeof(MSG_P2P_CONNECTION_ABORT_T),
         * VIR_MEM_TYPE);
         */

        if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
            WLAN_STATUS_SUCCESS) {
            break;
        }

        prDisconnectMsg = (P_MSG_P2P_CONNECTION_ABORT_T)cnmMemAlloc(
            prGlueInfo->prAdapter, RAM_TYPE_MSG,
            sizeof(MSG_P2P_CONNECTION_ABORT_T));

        if (prDisconnectMsg == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prDisconnectMsg->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_ABORT;
        prDisconnectMsg->ucRoleIdx = ucRoleIdx;
        COPY_MAC_ADDR(prDisconnectMsg->aucTargetID, mac);
        prDisconnectMsg->u2ReasonCode = REASON_CODE_UNSPECIFIED;
        prDisconnectMsg->fgSendDeauth = true;

        prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIdx);
        prCurrStaRec = bssGetClientByMac(prGlueInfo->prAdapter, prBssInfo,
                                         prDisconnectMsg->aucTargetID);
        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prDisconnectMsg, MSG_SEND_METHOD_BUF);

#if CFG_SUPPORT_802_11W
        /* if encrypted deauth frame
         * is in process, pending remove key
         */
        if (prBssInfo && prCurrStaRec && IS_BSS_APGO(prBssInfo) &&
            (prBssInfo->u4RsnSelectedAKMSuite == RSN_AKM_SUITE_SAE)) {
            reinit_completion(&prBssInfo->rDeauthComp);
            DBGLOG(P2P, INFO, "Start deauth wait\n");
            waitRet = wait_for_completion_timeout(&prBssInfo->rDeauthComp,
                                                  MSEC_TO_JIFFIES(1000));
            if (!waitRet) {
                DBGLOG(RSN, INFO, "timeout\n");
                prBssInfo->encryptedDeauthIsInProcess = false;
            } else {
                DBGLOG(RSN, INFO, "complete\n");
            }
        }
#endif
        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

int mtk_p2p_cfg80211_auth(struct wiphy *wiphy,
                          struct net_device *ndev,
                          struct cfg80211_auth_request *req)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_P2P_CONNECTION_SETTINGS_T prP2pConnSettings = NULL;
    struct cfg80211_connect_params connect;
    struct cfg80211_connect_params *sme = &connect;
    const struct cfg80211_bss_ies *ies;
    const u8 *ssidie = NULL;
    u8 ssid_len = 0;
    u8 ucRoleIdx = 0;

    DBGLOG(REQ, INFO,
           "auth to  BSS [" MACSTR "]\n",
           MAC2STR((u8 *)req->bss->bssid));
    DBGLOG(REQ, INFO, "auth_type:%d\n", req->auth_type);

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    memset(&connect, 0, sizeof(connect));
    sme->bssid = req->bss->bssid;
    if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev,
                              &ucRoleIdx) < 0) {
        return -EINVAL;
    }

    DBGLOG(REQ, INFO, "ucRoleIndex = %d\n", ucRoleIdx);

    prP2pConnSettings = prGlueInfo->prAdapter->
                        rWifiVar.prP2PConnSettings[ucRoleIdx];

    ies = rcu_access_pointer(req->bss->ies);
    if (!ies) {
        return false;
    }

    ssidie = cfg80211_find_ie(WLAN_EID_SSID, ies->data, ies->len);
    if (!ssidie) {
        return false;
    }

    ssid_len = ssidie[1];
    sme->ssid = ssidie + 2;
    sme->ssid_len = ssid_len;
    // yhpgi
    // COPY_SSID(prP2pConnSettings->aucSSID, prP2pConnSettings->ucSSIDLen,
    //  sme->ssid, sme->ssid_len);
    // prP2pConnSettings->fgIsSendAssoc = false;

    DBGLOG(REQ, INFO, "ssid_len %d ssid %s\n", sme->ssid_len, sme->ssid);

    return mtk_p2p_cfg80211_connect(wiphy, ndev, sme);
}


int mtk_p2p_cfg80211_assoc(struct wiphy *wiphy,
                           struct net_device *ndev,
                           struct cfg80211_assoc_request *req)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 ucRoleIdx = 0;
    P_P2P_CONNECTION_SETTINGS_T prP2pConnSettings = NULL;
    P_STA_RECORD_T prStaRec = NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo =
        (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_P2P_CONNECTION_REQ_INFO_T prConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev,
                              &ucRoleIdx) < 0) {
        return -EINVAL;
    }

    prP2pConnSettings = prGlueInfo->prAdapter->
                        rWifiVar.prP2PConnSettings[ucRoleIdx];

    /* [todo]temp use for indicate rx assoc resp, may need to be modified */
    /* The BSS from cfg80211_ops.assoc must give back to
     * cfg80211_send_rx_assoc() or to cfg80211_assoc_timeout().
     * To ensure proper refcounting,
     * new association requests while already associating
     * must be rejected.
     *  yhpgi
     */
    // if (prP2pConnSettings->bss)
    //  return -ENOENT;
    // prP2pConnSettings->bss = req->bss;

    DBGLOG(REQ, INFO, "ucRoleIndex = %d\n", ucRoleIdx);

    /*[TODO]may to check if assoc parameters change as cfg80211_auth*/
    // prP2pConnSettings->fgIsSendAssoc = true;
    /* skip join initial flow when it has been completed*/
    prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
        prGlueInfo->prAdapter,
        ucRoleIdx);
    prStaRec = prP2pRoleFsmInfo->rJoinInfo.prTargetStaRec;
    prConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
    kalMemCopy(prConnReqInfo->aucIEBuf,
               req->ie, req->ie_len);
    prConnReqInfo->u4BufLength = req->ie_len;

    /* set crypto */
    kalP2PSetCipher(prGlueInfo, IW_AUTH_CIPHER_NONE,
                    ucRoleIdx);
    DBGLOG(REQ, INFO,
           "n_ciphers_pairwise %d, ciphers_pairwise[0] %#x\n",
           req->crypto.n_ciphers_pairwise,
           req->crypto.ciphers_pairwise[0]);

    if (req->crypto.n_ciphers_pairwise) {
        switch (req->crypto.ciphers_pairwise[0]) {
        case WLAN_CIPHER_SUITE_WEP40:
        case WLAN_CIPHER_SUITE_WEP104:
            kalP2PSetCipher(prGlueInfo,
                            IW_AUTH_CIPHER_WEP40,
                            ucRoleIdx);
            break;
        case WLAN_CIPHER_SUITE_TKIP:
            kalP2PSetCipher(prGlueInfo,
                            IW_AUTH_CIPHER_TKIP,
                            ucRoleIdx);
            break;
        case WLAN_CIPHER_SUITE_CCMP:
        case WLAN_CIPHER_SUITE_AES_CMAC:
            kalP2PSetCipher(prGlueInfo,
                            IW_AUTH_CIPHER_CCMP,
                            ucRoleIdx);
            break;
        default:
            DBGLOG(REQ, WARN,
                   "invalid cipher pairwise (%d)\n",
                   req->crypto.ciphers_pairwise[0]);
            /* do cfg80211_put_bss before return */
            return -EINVAL;
        }
    }
    /* end	*/

    if (prStaRec) {
        saaSendAuthAssoc(prGlueInfo->prAdapter, prStaRec);
    }else{
        DBGLOG(REQ, WARN,
               "can't send auth since can't find StaRec\n");
    }

    return 0;
}

int mtk_p2p_cfg80211_connect(struct wiphy *wiphy, struct net_device *dev,
                             struct cfg80211_connect_params *sme)
{
    s32 i4Rslt = -EINVAL;
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_MSG_P2P_CONNECTION_REQUEST_T prConnReqMsg =
        (P_MSG_P2P_CONNECTION_REQUEST_T)NULL;
    u8 ucRoleIdx = 0;
#if CFG_SUPPORT_REPLAY_DETECTION
    u8 ucBssIndex = 0;
    P_BSS_INFO_T prBssInfo = NULL;
    struct SEC_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;
#endif

    do {
        if ((wiphy == NULL) || (dev == NULL) || (sme == NULL)) {
            break;
        }

        DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_connect.\n");

        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
            break;
        }

        prConnReqMsg = (P_MSG_P2P_CONNECTION_REQUEST_T)cnmMemAlloc(
            prGlueInfo->prAdapter, RAM_TYPE_MSG,
            (sizeof(MSG_P2P_CONNECTION_REQUEST_T) + sme->ie_len));

        if (prConnReqMsg == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prConnReqMsg->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_REQ;
        prConnReqMsg->ucRoleIdx = ucRoleIdx;

        COPY_SSID(prConnReqMsg->rSsid.aucSsid, prConnReqMsg->rSsid.ucSsidLen,
                  sme->ssid, sme->ssid_len);

        COPY_MAC_ADDR(prConnReqMsg->aucBssid, sme->bssid);
        COPY_MAC_ADDR(prConnReqMsg->aucSrcMacAddr, dev->dev_addr);

        DBGLOG(P2P, TRACE, "Assoc Req IE Buffer Length:%d\n", sme->ie_len);
        kalMemCopy(prConnReqMsg->aucIEBuf, sme->ie, sme->ie_len);
        prConnReqMsg->u4IELen = sme->ie_len;

        kalP2PSetCipher(prGlueInfo, IW_AUTH_CIPHER_NONE, ucRoleIdx);

        if (sme->crypto.n_ciphers_pairwise) {
            switch (sme->crypto.ciphers_pairwise[0]) {
            case WLAN_CIPHER_SUITE_WEP40:
            case WLAN_CIPHER_SUITE_WEP104:
                kalP2PSetCipher(prGlueInfo, IW_AUTH_CIPHER_WEP40, ucRoleIdx);
                break;

            case WLAN_CIPHER_SUITE_TKIP:
                kalP2PSetCipher(prGlueInfo, IW_AUTH_CIPHER_TKIP, ucRoleIdx);
                break;

            case WLAN_CIPHER_SUITE_CCMP:
            case WLAN_CIPHER_SUITE_AES_CMAC:
                kalP2PSetCipher(prGlueInfo, IW_AUTH_CIPHER_CCMP, ucRoleIdx);
                break;

            default:
                DBGLOG(REQ, WARN, "invalid cipher pairwise (%d)\n",
                       sme->crypto.ciphers_pairwise[0]);
                return -EINVAL;
            }
        }

        mtk_p2p_cfg80211func_channel_format_switch(NULL, sme->channel,
                                                   &prConnReqMsg->rChannelInfo);
        mtk_p2p_cfg80211func_channel_sco_switch(NL80211_CHAN_NO_HT,
                                                &prConnReqMsg->eChnlSco);

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prConnReqMsg,
                    MSG_SEND_METHOD_BUF);

#if CFG_SUPPORT_REPLAY_DETECTION
        if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx,
                                &ucBssIndex) != WLAN_STATUS_SUCCESS) {
            return -EINVAL;
        }

        prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
        ASSERT(prBssInfo);
        prDetRplyInfo = &prBssInfo->rDetRplyInfo;
        kalMemZero(prDetRplyInfo, sizeof(struct SEC_DETECT_REPLAY_INFO));
#endif

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

int mtk_p2p_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *dev,
                                u16 reason_code)
{
    s32 i4Rslt = -EINVAL;
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_MSG_P2P_CONNECTION_ABORT_T prDisconnMsg =
        (P_MSG_P2P_CONNECTION_ABORT_T)NULL;
    u8 aucBCAddr[] = BC_MAC_ADDR;
    u8 ucRoleIdx = 0;

    do {
        if ((wiphy == NULL) || (dev == NULL)) {
            break;
        }

        DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_disconnect.\n");

        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
            break;
        }
        /* prDisconnMsg =
         * (P_MSG_P2P_CONNECTION_ABORT_T)kalMemAlloc(sizeof(P_MSG_P2P_CONNECTION_ABORT_T),
         * VIR_MEM_TYPE);
         */
        prDisconnMsg = (P_MSG_P2P_CONNECTION_ABORT_T)cnmMemAlloc(
            prGlueInfo->prAdapter, RAM_TYPE_MSG,
            sizeof(MSG_P2P_CONNECTION_ABORT_T));

        if (prDisconnMsg == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prDisconnMsg->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_ABORT;
        prDisconnMsg->ucRoleIdx = ucRoleIdx;
        prDisconnMsg->u2ReasonCode = reason_code;
        prDisconnMsg->fgSendDeauth = true;
        COPY_MAC_ADDR(prDisconnMsg->aucTargetID, aucBCAddr);

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prDisconnMsg,
                    MSG_SEND_METHOD_BUF);

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
int mtk_p2p_cfg80211_change_iface(IN struct wiphy *wiphy,
                                  IN struct net_device *ndev,
                                  IN enum nl80211_iftype type,
                                  IN struct vif_params *params)
#else
int mtk_p2p_cfg80211_change_iface(IN struct wiphy *wiphy,
                                  IN struct net_device *ndev,
                                  IN enum nl80211_iftype type, IN u32 *flags,
                                  IN struct vif_params *params)
#endif
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    s32 i4Rslt = -EINVAL;
    P_MSG_P2P_SWITCH_OP_MODE_T prSwitchModeMsg =
        (P_MSG_P2P_SWITCH_OP_MODE_T)NULL;
    u8 ucRoleIdx = 0;

    do {
        if ((wiphy == NULL) || (ndev == NULL)) {
            DBGLOG(P2P, ERROR, "wiphy=%x, ndev=%x.\n", wiphy, ndev);
            break;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_change_iface.\n");

        if (ndev->ieee80211_ptr) {
            ndev->ieee80211_ptr->iftype = type;
        }

        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        if (prGlueInfo == NULL) {
            DBGLOG(INIT, WARN, "prGlueInfo is NULL\n");
            return -EFAULT;
        }

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev, &ucRoleIdx) != 0) {
            DBGLOG(P2P, TRACE,
                   "Device Interface no need to change interface type.\n");
            return 0;
        }
        /* Switch OP MOde. */
        prSwitchModeMsg = (P_MSG_P2P_SWITCH_OP_MODE_T)cnmMemAlloc(
            prGlueInfo->prAdapter, RAM_TYPE_MSG,
            sizeof(MSG_P2P_SWITCH_OP_MODE_T));

        if (prSwitchModeMsg == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prSwitchModeMsg->rMsgHdr.eMsgId = MID_MNY_P2P_FUN_SWITCH;
        prSwitchModeMsg->ucRoleIdx = ucRoleIdx;

        switch (type) {
        case NL80211_IFTYPE_P2P_CLIENT:
            DBGLOG(P2P, TRACE, "NL80211_IFTYPE_P2P_CLIENT.\n");
            prSwitchModeMsg->eOpMode = OP_MODE_INFRASTRUCTURE;
            kalP2PSetRole(prGlueInfo, 1, ucRoleIdx);
            break;

        case NL80211_IFTYPE_STATION:
            DBGLOG(P2P, TRACE, "NL80211_IFTYPE_STATION.\n");
            prSwitchModeMsg->eOpMode = OP_MODE_INFRASTRUCTURE;
            kalP2PSetRole(prGlueInfo, 1, ucRoleIdx);
            break;

        case NL80211_IFTYPE_AP:
            DBGLOG(P2P, TRACE, "NL80211_IFTYPE_AP.\n");
            prSwitchModeMsg->eOpMode = OP_MODE_ACCESS_POINT;
            kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);
            break;

        case NL80211_IFTYPE_P2P_GO:
            DBGLOG(P2P, TRACE, "NL80211_IFTYPE_P2P_GO not AP.\n");
            prSwitchModeMsg->eOpMode = OP_MODE_ACCESS_POINT;
            kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);
            break;

        default:
            DBGLOG(P2P, TRACE, "Other type :%d .\n", type);
            prSwitchModeMsg->eOpMode = OP_MODE_P2P_DEVICE;
            kalP2PSetRole(prGlueInfo, 0, ucRoleIdx);
            break;
        }

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prSwitchModeMsg, MSG_SEND_METHOD_BUF);

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

int mtk_p2p_cfg80211_set_channel(IN struct wiphy *wiphy,
                                 struct cfg80211_chan_def *chandef)
{
    s32 i4Rslt = -EINVAL;
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    struct net_device *dev = (struct net_device *)NULL;
    RF_CHANNEL_INFO_T rRfChnlInfo;
    u8 ucRoleIdx = 0;

    if ((wiphy == NULL) || (chandef == NULL)) {
        return i4Rslt;
    }

    dev = (struct net_device *)wiphy_dev(wiphy);

    do {
        DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_set_channel.\n");

        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        mtk_p2p_cfg80211func_channel_format_switch(chandef, chandef->chan,
                                                   &rRfChnlInfo);

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
            break;
        }

        p2pFuncSetChannel(prGlueInfo->prAdapter, ucRoleIdx, &rRfChnlInfo);

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

/* mtk_p2p_cfg80211_set_channel */

int mtk_p2p_cfg80211_set_bitrate_mask(
    IN struct wiphy *wiphy, IN struct net_device *dev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
    unsigned int link_id,
#endif
    IN const u8 *peer, IN const struct cfg80211_bitrate_mask *mask)
{
    s32 i4Rslt = -EINVAL;
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;

    do {
        if ((wiphy == NULL) || (dev == NULL) || (mask == NULL)) {
            break;
        }

        DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_set_bitrate_mask\n");

        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        /* TODO: Set bitrate mask of the peer? */

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

void mtk_p2p_cfg80211_mgmt_frame_register(IN struct wiphy *wiphy,
                                          struct wireless_dev *wdev,
                                          IN u16 frame_type, IN bool reg)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    u8 ucRoleIdx = 0;
    u32 *pu4P2pPacketFilter = NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;

    if (g_u4HaltFlag) {
        DBGLOG(RLM, WARN, "wlan is halt, skip reg callback\n");
        return;
    }

    do {
        if ((wiphy == NULL) || (wdev == NULL)) {
            break;
        }

        DBGLOG(P2P, TRACE, "[%s] mtk_p2p_cfg80211_mgmt_frame_register\n",
               wdev->netdev->name);
        P2P_WIPHY_PRIV(wiphy, prGlueInfo);

        /* since p2p device share the aprRoleHandler so needs to check
         * DevHandler 1st */
        if (prGlueInfo->prP2PInfo[0]->prDevHandler == wdev->netdev) {
            /* P2P device*/
            pu4P2pPacketFilter = &prGlueInfo->prP2PDevInfo->u4OsMgmtFrameFilter;
        } else {
            if (mtk_Netdev_To_RoleIdx(prGlueInfo, wdev->netdev, &ucRoleIdx) <
                0) {
                /* P2P device*/
                DBGLOG(
                    P2P, WARN,
                    "mtk_p2p_cfg80211_mgmt_frame_register wireless dev match fail!\n");
                break;
            } else {
                /* Non P2P device*/
                ASSERT(ucRoleIdx < KAL_P2P_NUM);
                DBGLOG(P2P, TRACE, "Open packet filer RoleIdx %u\n", ucRoleIdx);
                prP2pRoleFsmInfo =
                    prGlueInfo->prAdapter->rWifiVar.aprP2pRoleFsmInfo[ucRoleIdx];
                pu4P2pPacketFilter = &prP2pRoleFsmInfo->u4P2pPacketFilter;
            }
        }
        switch (frame_type) {
        case MAC_FRAME_PROBE_REQ:
            if (reg) {
                *pu4P2pPacketFilter |= PARAM_PACKET_FILTER_PROBE_REQ;
                DBGLOG(P2P, TRACE, "Open packet filer probe request\n");
            } else {
                *pu4P2pPacketFilter &= ~PARAM_PACKET_FILTER_PROBE_REQ;
                DBGLOG(P2P, TRACE, "Close packet filer probe request\n");
            }
            break;

        case MAC_FRAME_ACTION:
            if (reg) {
                *pu4P2pPacketFilter |= PARAM_PACKET_FILTER_ACTION_FRAME;
                DBGLOG(P2P, TRACE, "Open packet filer action frame.\n");
            } else {
                *pu4P2pPacketFilter &= ~PARAM_PACKET_FILTER_ACTION_FRAME;
                DBGLOG(P2P, TRACE, "Close packet filer action frame.\n");
            }
            break;

        default:
            DBGLOG(P2P, ERROR, "unsupported frame type:%x\n", frame_type);
            break;
        }

        set_bit(GLUE_FLAG_FRAME_FILTER_BIT, &prGlueInfo->ulFlag);

        /* wake up main thread */
        wake_up_interruptible(&prGlueInfo->waitq);

        if (in_interrupt()) {
            DBGLOG(P2P, TRACE, "It is in interrupt level\n");
        }
    } while (false);
}

#ifdef CONFIG_NL80211_TESTMODE

int mtk_p2p_cfg80211_testmode_cmd(struct wiphy *wiphy,
                                  struct wireless_dev *wdev, void *data,
                                  int len)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_NL80211_DRIVER_TEST_PARAMS prParams = (P_NL80211_DRIVER_TEST_PARAMS)NULL;
    s32 i4Status = -EINVAL;

    ASSERT(wiphy);
    ASSERT(wdev);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_testmode_cmd\n");

    if (len < sizeof(NL80211_DRIVER_TEST_PARAMS)) {
        DBGLOG(P2P, WARN, "len [%d] is invalid!\n", len);
        return -EINVAL;
    }

    if (data && len) {
        prParams = (P_NL80211_DRIVER_TEST_PARAMS)data;
    } else {
        DBGLOG(P2P, ERROR, "mtk_p2p_cfg80211_testmode_cmd, data is NULL\n");
        return i4Status;
    }
    if (prParams->index >> 24 == 0x01) {
        /* New version */
        prParams->index = prParams->index & ~BITS(24, 31);
    } else {
        /* Old version */
        mtk_p2p_cfg80211_testmode_p2p_sigma_pre_cmd(wiphy, data, len);
        i4Status = 0;
        return i4Status;
    }

    /* Clear the version byte */
    prParams->index = prParams->index & ~BITS(24, 31);

    if (prParams) {
        switch (prParams->index) {
        case 1:  /* P2P Simga */
#if CFG_SUPPORT_HOTSPOT_OPTIMIZATION
        {
            P_NL80211_DRIVER_SW_CMD_PARAMS prParamsCmd;

            prParamsCmd = (P_NL80211_DRIVER_SW_CMD_PARAMS)data;

            if ((prParamsCmd->adr & 0xffff0000) == 0xffff0000) {
                i4Status = mtk_p2p_cfg80211_testmode_sw_cmd(wiphy, data, len);
                break;
            }
        }
#endif
            i4Status =
                mtk_p2p_cfg80211_testmode_p2p_sigma_cmd(wiphy, data, len);
            break;

        case 2:  /* WFD */
#if CFG_SUPPORT_WFD
            /* use normal driver command wifi_display */
            /* i4Status =
             * mtk_p2p_cfg80211_testmode_wfd_update_cmd(wiphy, data,
             * len); */
#endif
            break;

        case 3:  /* Hotspot Client Management */
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
            i4Status = mtk_p2p_cfg80211_testmode_hotspot_block_list_cmd(
                wiphy, data, len);
#endif
            break;

        case 0x10:
            i4Status = mtk_cfg80211_testmode_get_sta_statistics(
                wiphy, data, len, prGlueInfo);
            break;

#if CFG_SUPPORT_NFC_BEAM_PLUS
        case 0x11:  /*NFC Beam + Indication */
            if (data && len) {
                P_NL80211_DRIVER_SET_NFC_PARAMS prParams =
                    (P_NL80211_DRIVER_SET_NFC_PARAMS)data;

                DBGLOG(P2P, INFO, "NFC: BEAM[%d]\n", prParams->NFC_Enable);
            }
            break;

        case 0x12:  /*NFC Beam + Indication */
            DBGLOG(P2P, INFO, "NFC: Polling\n");
            i4Status = mtk_cfg80211_testmode_get_scan_done(wiphy, data, len,
                                                           prGlueInfo);
            break;

#endif
#if CFG_AUTO_CHANNEL_SEL_SUPPORT
        case 0x30:
            i4Status =
                mtk_p2p_cfg80211_testmode_get_best_channel(wiphy, data, len);
            break;
#endif

        default:
            i4Status = -EINVAL;
            break;
        }
    }

    return i4Status;
}

int mtk_p2p_cfg80211_testmode_p2p_sigma_pre_cmd(IN struct wiphy *wiphy,
                                                IN void *data, IN int len)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    NL80211_DRIVER_TEST_PRE_PARAMS rParams;
    P_P2P_SPECIFIC_BSS_INFO_T prP2pSpecificBssInfo =
        (P_P2P_SPECIFIC_BSS_INFO_T)NULL;
    /* P_P2P_CONNECTION_SETTINGS_T prP2pConnSettings =
     * (P_P2P_CONNECTION_SETTINGS_T)NULL; */
    u32 index_mode;
    u32 index;
    s32 value;
    int status = 0;
    u32 u4Leng;
    u8 ucBssIdx;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    kalMemZero(&rParams, sizeof(NL80211_DRIVER_TEST_PRE_PARAMS));

    prP2pSpecificBssInfo =
        prGlueInfo->prAdapter->rWifiVar.prP2pSpecificBssInfo[0];
    /* prP2pConnSettings =
     * prGlueInfo->prAdapter->rWifiVar.prP2PConnSettings; */

    DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_testmode_cmd\n");

    if (data && len) {
        memcpy(&rParams, data, len);
    }

    DBGLOG(P2P, TRACE, "NL80211_ATTR_TESTDATA,idx_mode=%d idx=%d value=%lu\n",
           (s16)rParams.idx_mode, (s16)rParams.idx, rParams.value);

    index_mode = rParams.idx_mode;
    index = rParams.idx;
    value = rParams.value;

    /* 3 FIX ME: Add p2p role index selection */
    if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, 0, &ucBssIdx) !=
        WLAN_STATUS_SUCCESS) {
        return -EINVAL;
    }

    switch (index) {
    case 0:  /* Listen CH */
        break;

    case 1:  /* P2p mode */
        break;

    case 4:  /* Noa duration */
        prP2pSpecificBssInfo->rNoaParam.u4NoaDurationMs = value;
        /* only to apply setting when setting NOA count */
        /* status = mtk_p2p_wext_set_noa_param(prDev, info, wrqu, (char
         **)&prP2pSpecificBssInfo->rNoaParam); */
        break;

    case 5:  /* Noa interval */
        prP2pSpecificBssInfo->rNoaParam.u4NoaIntervalMs = value;
        /* only to apply setting when setting NOA count */
        /* status = mtk_p2p_wext_set_noa_param(prDev, info, wrqu, (char
         **)&prP2pSpecificBssInfo->rNoaParam); */
        break;

    case 6:  /* Noa count */
        prP2pSpecificBssInfo->rNoaParam.u4NoaCount = value;
        /* status = mtk_p2p_wext_set_noa_param(prDev, info, wrqu, (char
         **)&prP2pSpecificBssInfo->rNoaParam); */
        break;

    case 100:  /* Oper CH */
        /* 20110920 - frog: User configurations are placed in
         * ConnSettings. */
        /* prP2pConnSettings->ucOperatingChnl = value; */
        break;

    case 101:  /* Local config Method, for P2P SDK */
        /* prP2pConnSettings->u2LocalConfigMethod = value; */
        break;

    case 102:  /* Sigma P2p reset */
        /* kalMemZero(prP2pConnSettings->aucTargetDevAddr,
         * MAC_ADDR_LEN); */
        /* prP2pConnSettings->eConnectionPolicy =
         * ENUM_P2P_CONNECTION_POLICY_AUTO; */
        /* p2pFsmUninit(prGlueInfo->prAdapter); */
        /* p2pFsmInit(prGlueInfo->prAdapter); */
        break;

    case 103:  /* WPS MODE */
        kalP2PSetWscMode(prGlueInfo, value);
        break;

    case 104:  /* P2p send persence, duration */
        break;

    case 105:  /* P2p send persence, interval */
        break;

    case 106:  /* P2P set sleep  */
    {
        PARAM_POWER_MODE_T rPowerMode;

        rPowerMode.ePowerMode = Param_PowerModeMAX_PSP;
        rPowerMode.ucBssIdx = ucBssIdx;

        kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile, &rPowerMode,
                 sizeof(rPowerMode), false, false, true, &u4Leng);
    } break;

    case 107:  /* P2P set opps, CTWindowl */
        prP2pSpecificBssInfo->rOppPsParam.u4CTwindowMs = value;
        /* status = mtk_p2p_wext_set_oppps_param(prDev, info, wrqu,
         * (char *)&prP2pSpecificBssInfo->rOppPsParam);
         */
        break;

    case 108:  /* p2p_set_power_save */
    {
        PARAM_POWER_MODE_T rPowerMode;

        rPowerMode.ePowerMode = value;
        rPowerMode.ucBssIdx = ucBssIdx;

        kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile, &rPowerMode,
                 sizeof(rPowerMode), false, false, true, &u4Leng);
    } break;

    default:
        break;
    }

    return status;
}

int mtk_p2p_cfg80211_testmode_p2p_sigma_cmd(IN struct wiphy *wiphy,
                                            IN void *data, IN int len)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_NL80211_DRIVER_P2P_SIGMA_PARAMS prParams =
        (P_NL80211_DRIVER_P2P_SIGMA_PARAMS)NULL;
    P_P2P_SPECIFIC_BSS_INFO_T prP2pSpecificBssInfo =
        (P_P2P_SPECIFIC_BSS_INFO_T)NULL;
    /* P_P2P_CONNECTION_SETTINGS_T prP2pConnSettings =
     * (P_P2P_CONNECTION_SETTINGS_T)NULL; */
    u32 index;
    s32 value;
    int status = 0;
    u32 u4Leng;
    u8 ucBssIdx;
    u32 i;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    prP2pSpecificBssInfo =
        prGlueInfo->prAdapter->rWifiVar.prP2pSpecificBssInfo[0];
    /* prP2pConnSettings =
     * prGlueInfo->prAdapter->rWifiVar.prP2PConnSettings; */

    DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_testmode_p2p_sigma_cmd\n");

    if (len < sizeof(NL80211_DRIVER_P2P_SIGMA_PARAMS)) {
        DBGLOG(P2P, WARN, "len [%d] is invalid!\n", len);
        return -EINVAL;
    }

    if (data && len) {
        prParams = (P_NL80211_DRIVER_P2P_SIGMA_PARAMS)data;
    }

    index = (s32)prParams->idx;
    value = (s32)prParams->value;

    DBGLOG(P2P, TRACE, "NL80211_ATTR_TESTDATA, idx=%lu value=%lu\n",
           (s32)prParams->idx, (s32)prParams->value);

    /* 3 FIX ME: Add p2p role index selection */
    if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, 0, &ucBssIdx) !=
        WLAN_STATUS_SUCCESS) {
        return -EINVAL;
    }

    switch (index) {
    case 0:  /* Listen CH */
        break;

    case 1:  /* P2p mode */
        break;

    case 4:  /* Noa duration */
        prP2pSpecificBssInfo->rNoaParam.u4NoaDurationMs = value;
        /* only to apply setting when setting NOA count */
        /* status = mtk_p2p_wext_set_noa_param(prDev, info, wrqu, (char
         **)&prP2pSpecificBssInfo->rNoaParam); */
        break;

    case 5:  /* Noa interval */
        prP2pSpecificBssInfo->rNoaParam.u4NoaIntervalMs = value;
        /* only to apply setting when setting NOA count */
        /* status = mtk_p2p_wext_set_noa_param(prDev, info, wrqu, (char
         **)&prP2pSpecificBssInfo->rNoaParam); */
        break;

    case 6:  /* Noa count */
        prP2pSpecificBssInfo->rNoaParam.u4NoaCount = value;
        /* status = mtk_p2p_wext_set_noa_param(prDev, info, wrqu, (char
         **)&prP2pSpecificBssInfo->rNoaParam); */
        break;

    case 100:  /* Oper CH */
        /* 20110920 - frog: User configurations are placed in
         * ConnSettings. */
        /* prP2pConnSettings->ucOperatingChnl = value; */
        break;

    case 101:  /* Local config Method, for P2P SDK */
        /* prP2pConnSettings->u2LocalConfigMethod = value; */
        break;

    case 102:  /* Sigma P2p reset */
        /* kalMemZero(prP2pConnSettings->aucTargetDevAddr,
         * MAC_ADDR_LEN); */
        /* prP2pConnSettings->eConnectionPolicy =
         * ENUM_P2P_CONNECTION_POLICY_AUTO; */
        break;

    case 103:  /* WPS MODE */
        kalP2PSetWscMode(prGlueInfo, value);
        break;

    case 104:  /* P2p send persence, duration */
        break;

    case 105:  /* P2p send persence, interval */
        break;

    case 106:  /* P2P set sleep  */
    {
        PARAM_POWER_MODE_T rPowerMode;

        rPowerMode.ePowerMode = Param_PowerModeMAX_PSP;
        rPowerMode.ucBssIdx = ucBssIdx;

        kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile, &rPowerMode,
                 sizeof(rPowerMode), false, false, true, &u4Leng);
    } break;

    case 107:  /* P2P set opps, CTWindowl */
        prP2pSpecificBssInfo->rOppPsParam.u4CTwindowMs = value;
        /* status = mtk_p2p_wext_set_oppps_param(prDev, info, wrqu,
         * (char *)&prP2pSpecificBssInfo->rOppPsParam);
         */
        break;

    case 108:  /* p2p_set_power_save */
    {
        PARAM_POWER_MODE_T rPowerMode;

        rPowerMode.ePowerMode = value;
        rPowerMode.ucBssIdx = ucBssIdx;

        kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile, &rPowerMode,
                 sizeof(rPowerMode), false, false, true, &u4Leng);
    } break;

    case 109:  /* Max Clients */
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
        for (i = 0; i < KAL_P2P_NUM; i++)
            kalP2PSetMaxClients(prGlueInfo, value, i);
#endif
        break;

    case 110:  /* Hotspot WPS mode */
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
        kalIoctl(prGlueInfo, wlanoidSetP2pWPSmode, &value, sizeof(value), false,
                 false, true, &u4Leng);
#endif
        break;

    default:
        break;
    }

    return status;
}

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER

int mtk_p2p_cfg80211_testmode_hotspot_block_list_cmd(IN struct wiphy *wiphy,
                                                     IN void *data, IN int len)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_NL80211_DRIVER_hotspot_block_PARAMS prParams =
        (P_NL80211_DRIVER_hotspot_block_PARAMS)NULL;
    int fgIsValid = 0;
    u32 i;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    if (len < sizeof(NL80211_DRIVER_hotspot_block_PARAMS)) {
        DBGLOG(P2P, WARN, "len [%d] is invalid!\n", len);
        return -EINVAL;
    }

    if (data && len) {
        prParams = (P_NL80211_DRIVER_hotspot_block_PARAMS)data;
    }

    DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_testmode_hotspot_block_list_cmd\n");

    for (i = 0; i < KAL_P2P_NUM; i++)
        fgIsValid |= kalP2PSetBlackList(prGlueInfo, prParams->aucBssid,
                                        prParams->ucblocked, i);

    return fgIsValid;
}

#endif

int mtk_p2p_cfg80211_testmode_sw_cmd(IN struct wiphy *wiphy, IN void *data,
                                     IN int len)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_NL80211_DRIVER_SW_CMD_PARAMS prParams =
        (P_NL80211_DRIVER_SW_CMD_PARAMS)NULL;
    WLAN_STATUS rstatus = WLAN_STATUS_SUCCESS;
    int fgIsValid = 0;
    u32 u4SetInfoLen = 0;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);

    DBGLOG(P2P, TRACE, "--> %s()\n", __func__);

    if (len < sizeof(struct _NL80211_DRIVER_SW_CMD_PARAMS)) {
        rstatus = WLAN_STATUS_INVALID_LENGTH;
    } else if (!data) {
        rstatus = WLAN_STATUS_INVALID_DATA;
    } else {
        prParams = (P_NL80211_DRIVER_SW_CMD_PARAMS)data;
        if (prParams->set == 1) {
            rstatus = kalIoctl(prGlueInfo,
                               (PFN_OID_HANDLER_FUNC)wlanoidSetSwCtrlWrite,
                               &prParams->adr, (u32)8, false, false, true,
                               &u4SetInfoLen);
        }
    }

    if (rstatus != WLAN_STATUS_SUCCESS) {
        fgIsValid = -EFAULT;
    }

    return fgIsValid;
}

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
int mtk_p2p_cfg80211_testmode_get_best_channel(IN struct wiphy *wiphy,
                                               IN void *data, IN int len)
{
    struct sk_buff *skb;

    u8 fgIsReady = false;

    P_GLUE_INFO_T prGlueInfo = NULL;
    RF_CHANNEL_INFO_T aucChannelList[MAX_2G_BAND_CHN_NUM];
    u8 ucNumOfChannel, i, ucIdx;
    u16 u2APNumScore = 0, u2UpThreshold = 0, u2LowThreshold = 0, ucInnerIdx = 0;
    u32 u4BufLen, u4LteSafeChnBitMask_2G = 0;
    u32 u4AcsChnReport[5];

    P_PARAM_GET_CHN_INFO prGetChnLoad, prQueryLteChn;
    PARAM_PREFER_CHN_INFO rPreferChannel = { 0, 0xFFFF, 0 };
    PARAM_PREFER_CHN_INFO arChannelDirtyScore_2G[MAX_2G_BAND_CHN_NUM];

    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;

    ASSERT(wiphy);

    P2P_WIPHY_PRIV(wiphy, prGlueInfo);
    if (!prGlueInfo) {
        DBGLOG(P2P, ERROR, "No glue info\n");
        return -EFAULT;
    }

    /* Prepare reply skb buffer */
    skb = cfg80211_testmode_alloc_reply_skb(wiphy, sizeof(u4AcsChnReport));
    if (!skb) {
        DBGLOG(P2P, ERROR, "Allocate skb failed\n");
        return -ENOMEM;
    }

    kalMemZero(u4AcsChnReport, sizeof(u4AcsChnReport));

    fgIsReady = prGlueInfo->prAdapter->rWifiVar.rChnLoadInfo.fgDataReadyBit;
    if (fgIsReady == false) {
        goto acs_report;
    }

    /*
     * 1. Get 2.4G Band channel list in current regulatory domain
     */
    rlmDomainGetChnlList(prGlueInfo->prAdapter, BAND_2G4, true,
                         MAX_2G_BAND_CHN_NUM, &ucNumOfChannel, aucChannelList);

    /*
     * 2. Calculate each channel's dirty score
     */
    prGetChnLoad = &(prGlueInfo->prAdapter->rWifiVar.rChnLoadInfo);

    for (i = 0; i < ucNumOfChannel; i++) {
        ucIdx = aucChannelList[i].ucChannelNum - 1;

        /* Current channel's dirty score */
        u2APNumScore = prGetChnLoad->rEachChnLoad[ucIdx].u2APNum *
                       CHN_DIRTY_WEIGHT_UPPERBOUND;
        u2LowThreshold = u2UpThreshold = 3;

        if (ucIdx < 3) {
            u2LowThreshold = ucIdx;
            u2UpThreshold = 3;
        } else if (ucIdx >= (ucNumOfChannel - 3)) {
            u2LowThreshold = 3;
            u2UpThreshold = ucNumOfChannel - (ucIdx + 1);
        }

        /* Lower channel's dirty score */
        for (ucInnerIdx = 0; ucInnerIdx < u2LowThreshold; ucInnerIdx++) {
            u2APNumScore +=
                (prGetChnLoad->rEachChnLoad[ucIdx - ucInnerIdx - 1].u2APNum *
                 (CHN_DIRTY_WEIGHT_UPPERBOUND - 1 - ucInnerIdx));
        }

        /* Upper channel's dirty score */
        for (ucInnerIdx = 0; ucInnerIdx < u2UpThreshold; ucInnerIdx++) {
            u2APNumScore +=
                (prGetChnLoad->rEachChnLoad[ucIdx + ucInnerIdx + 1].u2APNum *
                 (CHN_DIRTY_WEIGHT_UPPERBOUND - 1 - ucInnerIdx));
        }

        arChannelDirtyScore_2G[i].ucChannel = aucChannelList[i].ucChannelNum;
        arChannelDirtyScore_2G[i].u2APNumScore = u2APNumScore;

        DBGLOG(P2P, INFO, "[ACS]channel=%d, AP num=%d, score=%d\n",
               aucChannelList[i].ucChannelNum,
               prGetChnLoad->rEachChnLoad[ucIdx].u2APNum, u2APNumScore);
    }

    /*
     * 3. Query LTE safe channels
     */
    prQueryLteChn = kalMemAlloc(sizeof(PARAM_GET_CHN_INFO), VIR_MEM_TYPE);
    if (prQueryLteChn == NULL) {
        DBGLOG(P2P, ERROR, "Alloc prQueryLteChn failed\n");
        /* Continue anyway */
    } else {
        kalMemZero(prQueryLteChn, sizeof(PARAM_GET_CHN_INFO));

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryLteSafeChannel,
                           prQueryLteChn, sizeof(PARAM_GET_CHN_INFO), true,
                           false, true, &u4BufLen);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(P2P, ERROR, "Query LTE safe channels failed\n");
            /* Continue anyway */
        }

        u4LteSafeChnBitMask_2G =
            prQueryLteChn->rLteSafeChnList.au4SafeChannelBitmask
            [NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_2G_BASE_1 - 1];
        if (!u4LteSafeChnBitMask_2G) {
            DBGLOG(P2P, WARN, "  Can't get any 2G4 safe channel from fw!?\n");
            u4LteSafeChnBitMask_2G = BITS(1, 14);
        }

        kalMemFree(prQueryLteChn, VIR_MEM_TYPE, sizeof(PARAM_GET_CHN_INFO));
    }

    /* 4. Find out the best channel, skip LTE unsafe channels */
    for (i = 0; i < ucNumOfChannel; i++) {
        if (!(u4LteSafeChnBitMask_2G &
              BIT(arChannelDirtyScore_2G[i].ucChannel))) {
            continue;
        }

        if (rPreferChannel.u2APNumScore >=
            arChannelDirtyScore_2G[i].u2APNumScore) {
            rPreferChannel.ucChannel = arChannelDirtyScore_2G[i].ucChannel;
            rPreferChannel.u2APNumScore =
                arChannelDirtyScore_2G[i].u2APNumScore;
        }
    }

    u4AcsChnReport[NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_2G_BASE_1 - 1] =
        fgIsReady ? BIT(31) : 0;
    if (rPreferChannel.ucChannel > 0) {
        u4AcsChnReport[NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_2G_BASE_1 - 1] |=
            BIT(rPreferChannel.ucChannel - 1);
    }

    /* ToDo: Support 5G Channel Selection */

acs_report:
    if (unlikely(
            nla_put_u32(
                skb, NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_2G_BASE_1,
                u4AcsChnReport[NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_2G_BASE_1 -
                               1]) < 0)) {
        goto nla_put_failure;
    }

    if (unlikely(
            nla_put_u32(
                skb, NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_5G_BASE_36,
                u4AcsChnReport[NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_5G_BASE_36 -
                               1]) < 0)) {
        goto nla_put_failure;
    }

    if (unlikely(
            nla_put_u32(
                skb, NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_5G_BASE_52,
                u4AcsChnReport[NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_5G_BASE_52 -
                               1]) < 0)) {
        goto nla_put_failure;
    }

    if (unlikely(
            nla_put_u32(
                skb, NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_5G_BASE_100,
                u4AcsChnReport[NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_5G_BASE_100
                               -
                               1]) < 0)) {
        goto nla_put_failure;
    }

    if (unlikely(
            nla_put_u32(
                skb, NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_5G_BASE_149,
                u4AcsChnReport[NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_5G_BASE_149
                               -
                               1]) < 0)) {
        goto nla_put_failure;
    }

    DBGLOG(P2P, INFO, "[ACS]Relpy u4AcsChnReport[2G_BASE_1]=0x%08x\n",
           u4AcsChnReport[0]);

    return cfg80211_testmode_reply(skb);

nla_put_failure:
    kfree_skb(skb);
    return -EMSGSIZE;
}
#endif
#endif

#endif
