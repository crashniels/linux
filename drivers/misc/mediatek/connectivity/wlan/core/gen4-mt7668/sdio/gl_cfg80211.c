// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_cfg80211.c
 *    \brief  Main routines for supporintg MT6620 cfg80211 control interface
 *
 *    This file contains the support routines of Linux driver for MediaTek Inc.
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

#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "gl_wext.h"
#include "precomp.h"
#include <linux/can/netlink.h>
#include <net/netlink.h>
#include <net/cfg80211.h>
#include "gl_cfg80211.h"

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

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for change STA type between
 *        1. Infrastructure Client (Non-AP STA)
 *        2. Ad-Hoc IBSS
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
int mtk_cfg80211_change_iface(struct wiphy *wiphy, struct net_device *ndev,
                              enum nl80211_iftype type,
                              struct vif_params *params)
#else
int mtk_cfg80211_change_iface(struct wiphy *wiphy, struct net_device *ndev,
                              enum nl80211_iftype type, u32 *flags,
                              struct vif_params *params)
#endif
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    ENUM_PARAM_OP_MODE_T eOpMode;
#if !DBG_DISABLE_ALL_LOG
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen;
#endif

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    if (type == NL80211_IFTYPE_STATION) {
        eOpMode = NET_TYPE_INFRA;
    } else if (type == NL80211_IFTYPE_ADHOC) {
        eOpMode = NET_TYPE_IBSS;
    } else {
        return -EINVAL;
    }

#if !DBG_DISABLE_ALL_LOG
    rStatus = kalIoctl(prGlueInfo, wlanoidSetInfrastructureMode, &eOpMode,
                       sizeof(eOpMode), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "set infrastructure mode error:%lx\n", rStatus);
    }
#endif

    /* reset wpa info */
    prGlueInfo->rWpaInfo.u4WpaVersion = IW_AUTH_WPA_VERSION_DISABLED;
    prGlueInfo->rWpaInfo.u4KeyMgmt = 0;
    prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo.u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;
#if CFG_SUPPORT_802_11W
    prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_DISABLED;
    prGlueInfo->rWpaInfo.ucRSNMfpCap = 0;
    prGlueInfo->rWpaInfo.u4CipherGroupMgmt = IW_AUTH_CIPHER_NONE;
#endif

    ndev->ieee80211_ptr->iftype = type;

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for adding key
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_add_key(struct wiphy *wiphy, struct net_device *ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                         int link_id,
#endif
                         u8 key_index, bool pairwise, const u8 *mac_addr,
                         struct key_params *params)
{
    PARAM_KEY_T rKey;
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    s32 i4Rslt = -EINVAL;
    u32 u4BufLen = 0;
    u8 tmp1[8], tmp2[8];
#if CFG_SUPPORT_REPLAY_DETECTION
    P_BSS_INFO_T prBssInfo = NULL;
    struct SEC_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;
    u8 ucCheckZeroKey = 0;
    u8 i = 0;
#endif

    const u8 aucBCAddr[] = BC_MAC_ADDR;
    /* const u8 aucZeroMacAddr[] = NULL_MAC_ADDR; */

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

#if DBG
    DBGLOG(RSN, INFO, "mtk_cfg80211_add_key\n");
    if (mac_addr) {
        DBGLOG(RSN, INFO, "keyIdx = %d pairwise = %d mac = " MACSTR "\n",
               key_index, pairwise, MAC2STR(mac_addr));
    } else {
        DBGLOG(RSN, INFO, "keyIdx = %d pairwise = %d null mac\n", key_index,
               pairwise);
    }
    DBGLOG(RSN, INFO, "Cipher = %x\n", params->cipher);
    DBGLOG_MEM8(RSN, INFO, params->key, params->key_len);
#endif

    kalMemZero(&rKey, sizeof(PARAM_KEY_T));

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

    if (pairwise) {
        ASSERT(mac_addr);
        rKey.u4KeyIndex |= BIT(31);
        rKey.u4KeyIndex |= BIT(30);
        COPY_MAC_ADDR(rKey.arBSSID, mac_addr);

        /* reset KCK, KEK, EAPOL Replay counter */
        kalMemZero(prGlueInfo->rWpaInfo.aucKek, NL80211_KEK_LEN);
        kalMemZero(prGlueInfo->rWpaInfo.aucKck, NL80211_KCK_LEN);
        kalMemZero(prGlueInfo->rWpaInfo.aucReplayCtr, NL80211_REPLAY_CTR_LEN);
    } else {  /* Group key */
        COPY_MAC_ADDR(rKey.arBSSID, aucBCAddr);
#if CFG_KEY_ERROR_STATISTIC_RECOVERY
        RX_RESET_CNT(&prGlueInfo->prAdapter->rRxCtrl, RX_BMC_NO_KEY_COUNT);
        RX_RESET_CNT(&prGlueInfo->prAdapter->rRxCtrl, RX_BMC_KEY_ERROR_COUNT);
        RX_RESET_CNT(&prGlueInfo->prAdapter->rRxCtrl, RX_BMC_PKT_COUNT);
#endif
    }

    if (params->key) {
#if CFG_SUPPORT_REPLAY_DETECTION
        for (i = 0; i < params->key_len; i++)
            if (params->key[i] == 0x00) {
                ucCheckZeroKey++;
            }

        if (ucCheckZeroKey == params->key_len) {
            return 0;
        }

#endif

        if (params->key_len > sizeof(rKey.aucKeyMaterial)) {
            return -EINVAL;
        }

        kalMemCopy(rKey.aucKeyMaterial, params->key, params->key_len);
        if (rKey.ucCipher == CIPHER_SUITE_TKIP) {
            kalMemCopy(tmp1, &params->key[16], 8);
            kalMemCopy(tmp2, &params->key[24], 8);
            kalMemCopy(&rKey.aucKeyMaterial[16], tmp2, 8);
            kalMemCopy(&rKey.aucKeyMaterial[24], tmp1, 8);
        }
    }

    rKey.ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex;

    rKey.u4KeyLength = params->key_len;
    rKey.u4Length = ((unsigned long)&(((P_PARAM_KEY_T)0)->aucKeyMaterial)) +
                    rKey.u4KeyLength;

#if CFG_SUPPORT_REPLAY_DETECTION
    prBssInfo = GET_BSS_INFO_BY_INDEX(
        prGlueInfo->prAdapter, prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex);

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

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for getting key for specified STA
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_get_key(struct wiphy *wiphy, struct net_device *ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                         int link_id,
#endif
                         u8 key_index, bool pairwise, const u8 *mac_addr,
                         void *cookie,
                         void (*callback)(void *cookie, struct key_params *))
{
    P_GLUE_INFO_T prGlueInfo = NULL;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    DBGLOG(INIT, INFO, "--> %s()\n", __func__);

    /* not implemented */

    return -EINVAL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for removing key for specified STA
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                         int link_id,
#endif
                         u8 key_index, bool pairwise, const u8 *mac_addr)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4Rslt = -EINVAL;
    u8 ucBssIndex = 0;
    u32 waitRet = 0;
    P_BSS_INFO_T prBssInfo;
    PARAM_REMOVE_KEY_T rRemoveKey;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

#if DBG
    DBGLOG(RSN, TRACE, "mtk_cfg80211_del_key\n");
    if (mac_addr) {
        DBGLOG(RSN, TRACE, "keyIdx = %d pairwise = %d mac = " MACSTR "\n",
               key_index, pairwise, MAC2STR(mac_addr));
    } else {
        DBGLOG(RSN, TRACE, "keyIdx = %d pairwise = %d null mac\n", key_index,
               pairwise);
    }
#endif

    kalMemZero(&rRemoveKey, sizeof(PARAM_REMOVE_KEY_T));
    rRemoveKey.u4KeyIndex = key_index;
    rRemoveKey.u4Length = sizeof(PARAM_REMOVE_KEY_T);
    if (mac_addr) {
        COPY_MAC_ADDR(rRemoveKey.arBSSID, mac_addr);
        rRemoveKey.u4KeyIndex |= BIT(30);
    }
    if ((prGlueInfo->prAdapter == NULL) ||
        (prGlueInfo->prAdapter->prAisBssInfo == NULL)) {
        return i4Rslt;
    }

    rRemoveKey.ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex;

    prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);

#if CFG_SUPPORT_802_11W
    /* if encrypted deauth frame is in process, pending remove key */
    if (IS_BSS_INDEX_AIS(prGlueInfo->prAdapter, ucBssIndex) &&
        prBssInfo->encryptedDeauthIsInProcess == true) {
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

    rStatus = kalIoctl(prGlueInfo, wlanoidSetRemoveKey, &rRemoveKey,
                       rRemoveKey.u4Length, false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(RSN, WARN, "remove key error:%lx\n", rStatus);
    } else {
        i4Rslt = 0;
    }

    return i4Rslt;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for setting default key on an interface
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_set_default_key(struct wiphy *wiphy, struct net_device *ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                                 int link_id,
#endif
                                 u8 key_index, bool unicast, bool multicast)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    PARAM_DEFAULT_KEY_T rDefaultKey;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    s32 i4Rst = -EINVAL;
    u32 u4BufLen = 0;
    u8 fgDef = false, fgMgtDef = false;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    /* For STA, should wep set the default key !! */
#if DBG
    DBGLOG(RSN, INFO, "mtk_cfg80211_set_default_key\n");
    DBGLOG(RSN, INFO, "keyIdx = %d unicast = %d multicast = %d\n", key_index,
           unicast, multicast);
#endif

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

    rDefaultKey.ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex;

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
 * @brief This routine is responsible for getting station information such as
 * RSSI
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_get_station(struct wiphy *wiphy, struct net_device *ndev,
                             const u8 *mac, struct station_info *sinfo)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    PARAM_MAC_ADDRESS arBssid;
    u32 u4BufLen, u4Rate;
    s32 i4Rssi;
    PARAM_GET_STA_STATISTICS rQueryStaStatistics;
    u32 u4TotalError;
    struct net_device_stats *prDevStats;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    kalMemZero(arBssid, MAC_ADDR_LEN);
    wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQueryBssid, &arBssid[0],
                         sizeof(arBssid), &u4BufLen);

    /* 1. check input MAC address */
    /* On Android O, this might be wlan0 address */
    if (UNEQUAL_MAC_ADDR(arBssid, mac) &&
        UNEQUAL_MAC_ADDR(prGlueInfo->prAdapter->rWifiVar.aucMacAddress, mac)) {
        /* wrong MAC address */
        DBGLOG(REQ, WARN,
               "incorrect BSSID: [" MACSTR "] currently connected BSSID[" MACSTR
               "]\n",
               MAC2STR(mac), MAC2STR(arBssid));

        return -ENOENT;
    }

    /* 2. fill TX rate */
    if (prGlueInfo->eParamMediaStateIndicated != PARAM_MEDIA_STATE_CONNECTED) {
        /* not connected */
        DBGLOG(REQ, WARN, "not yet connected\n");
    } else {
        rStatus = kalIoctl(prGlueInfo, wlanoidQueryLinkSpeed, &u4Rate,
                           sizeof(u4Rate), true, false, false, &u4BufLen);

        sinfo->filled |= BIT(NL80211_STA_INFO_TX_BITRATE);

        if ((rStatus != WLAN_STATUS_SUCCESS) || (u4Rate == 0)) {
            /*
             *  DBGLOG(REQ, WARN, "unable to retrieve link
             * speed\n"));
             */
            DBGLOG(REQ, WARN, "last link speed\n");
            sinfo->txrate.legacy = prGlueInfo->u4LinkSpeedCache;
        } else {
            /*
             *  sinfo->filled |= STATION_INFO_TX_BITRATE;
             */
            sinfo->txrate.legacy = u4Rate / 1000;  /* convert from
                                                    * 100bps to
                                                    * 100kbps */
            prGlueInfo->u4LinkSpeedCache = u4Rate / 1000;
        }
    }

    /* 3. fill RSSI */
    if (prGlueInfo->eParamMediaStateIndicated != PARAM_MEDIA_STATE_CONNECTED) {
        /* not connected */
        DBGLOG(REQ, WARN, "not yet connected\n");
    } else {
        rStatus = kalIoctl(prGlueInfo, wlanoidQueryRssi, &i4Rssi,
                           sizeof(i4Rssi), true, false, false, &u4BufLen);

        sinfo->filled |= BIT(NL80211_STA_INFO_SIGNAL);

        if ((rStatus != WLAN_STATUS_SUCCESS) ||
            (i4Rssi == PARAM_WHQL_RSSI_MIN_DBM) ||
            (i4Rssi == PARAM_WHQL_RSSI_MAX_DBM)) {
            DBGLOG(REQ, WARN, "last rssi\n");
            sinfo->signal = prGlueInfo->i4RssiCache;
        } else {
            sinfo->signal = i4Rssi;  /* dBm */
            prGlueInfo->i4RssiCache = i4Rssi;
        }
    }

    /* Get statistics from net_dev */
    prDevStats = (struct net_device_stats *)kalGetStats(ndev);

    if (prDevStats) {
        /* 4. fill RX_PACKETS */
        sinfo->filled |= BIT(NL80211_STA_INFO_RX_PACKETS);

        sinfo->rx_packets = prDevStats->rx_packets;

        /* 5. fill TX_PACKETS */
        sinfo->filled |= BIT(NL80211_STA_INFO_TX_PACKETS);

        sinfo->tx_packets = prDevStats->tx_packets;

        /* 6. fill TX_FAILED */
        kalMemZero(&rQueryStaStatistics, sizeof(rQueryStaStatistics));
        COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr, arBssid);
        rQueryStaStatistics.ucReadClear = true;

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
                           &rQueryStaStatistics, sizeof(rQueryStaStatistics),
                           true, false, true, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(REQ, WARN,
                   "unable to retrieve link speed,status code = %d\n", rStatus);
        } else {
            DBGLOG(REQ, INFO,
                   "BSSID: [" MACSTR "] TxFailCount %d LifeTimeOut %d\n",
                   MAC2STR(arBssid), rQueryStaStatistics.u4TxFailCount,
                   rQueryStaStatistics.u4TxLifeTimeoutCount);

            u4TotalError = rQueryStaStatistics.u4TxFailCount +
                           rQueryStaStatistics.u4TxLifeTimeoutCount;
            prDevStats->tx_errors += u4TotalError;
        }
        sinfo->filled |= BIT(NL80211_STA_INFO_TX_FAILED);
        sinfo->tx_failed = prDevStats->tx_errors;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for getting statistics for Link layer
 * statistics
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*------------------------------------------------------------------------*/
int mtk_cfg80211_get_link_statistics(struct wiphy *wiphy,
                                     struct net_device *ndev, u8 *mac,
                                     struct station_info *sinfo)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    PARAM_MAC_ADDRESS arBssid;
    u32 u4BufLen;
    s32 i4Rssi;
    PARAM_GET_STA_STATISTICS rQueryStaStatistics;
    PARAM_GET_BSS_STATISTICS rQueryBssStatistics;
    struct net_device_stats *prDevStats;
    P_NETDEV_PRIVATE_GLUE_INFO prNetDevPrivate =
        (P_NETDEV_PRIVATE_GLUE_INFO)NULL;
    u8 ucBssIndex;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    kalMemZero(arBssid, MAC_ADDR_LEN);
    wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQueryBssid, &arBssid[0],
                         sizeof(arBssid), &u4BufLen);

    /* 1. check BSSID */
    if (UNEQUAL_MAC_ADDR(arBssid, mac)) {
        /* wrong MAC address */
        DBGLOG(REQ, WARN,
               "incorrect BSSID: [" MACSTR "] currently connected BSSID[" MACSTR
               "]\n",
               MAC2STR(mac), MAC2STR(arBssid));
        return -ENOENT;
    }

    /* 2. fill RSSI */
    if (prGlueInfo->eParamMediaStateIndicated != PARAM_MEDIA_STATE_CONNECTED) {
        /* not connected */
        DBGLOG(REQ, WARN, "not yet connected\n");
    } else {
        rStatus = kalIoctl(prGlueInfo, wlanoidQueryRssi, &i4Rssi,
                           sizeof(i4Rssi), true, false, false, &u4BufLen);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(REQ, WARN, "unable to retrieve rssi\n");
        }
    }

    /* Get statistics from net_dev */
    prDevStats = (struct net_device_stats *)kalGetStats(ndev);

    /*3. get link layer statistics from Driver and FW */
    if (prDevStats) {
        /* 3.1 get per-STA link statistics */
        kalMemZero(&rQueryStaStatistics, sizeof(rQueryStaStatistics));
        COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr, arBssid);
        rQueryStaStatistics.ucLlsReadClear = false;  /* dont clear for
                                                      * get BSS statistic
                                                      */

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
                           &rQueryStaStatistics, sizeof(rQueryStaStatistics),
                           true, false, true, &u4BufLen);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(REQ, WARN, "unable to retrieve per-STA link statistics\n");
        }

        /*3.2 get per-BSS link statistics */
        if (rStatus == WLAN_STATUS_SUCCESS) {
            /* get Bss Index from ndev */
            prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(ndev);
            ASSERT(prNetDevPrivate->prGlueInfo == prGlueInfo);
            ucBssIndex = prNetDevPrivate->ucBssIdx;

            kalMemZero(&rQueryBssStatistics, sizeof(rQueryBssStatistics));
            rQueryBssStatistics.ucBssIndex = ucBssIndex;

            rStatus = kalIoctl(prGlueInfo, wlanoidQueryBssStatistics,
                               &rQueryBssStatistics,
                               sizeof(rQueryBssStatistics), true, false, true,
                               &u4BufLen);
        } else {
            DBGLOG(REQ, WARN, "unable to retrieve per-BSS link statistics\n");
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to do a scan
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_scan(struct wiphy *wiphy,
                      struct cfg80211_scan_request *request)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    u32 i, u4BufLen;
    PARAM_SCAN_REQUEST_ADV_T rScanRequest;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);
    kalMemZero(&rScanRequest, sizeof(rScanRequest));

    /* check if there is any pending scan/sched_scan not yet finished */
    if (prGlueInfo->prScanRequest != NULL) {
        return -EBUSY;
    }

    if (request->n_ssids == 0) {
        rScanRequest.u4SsidNum = 0;
    } else if (request->n_ssids <= SCN_SSID_MAX_NUM) {
        rScanRequest.u4SsidNum = request->n_ssids;

        for (i = 0; i < request->n_ssids; i++) {
            COPY_SSID(rScanRequest.rSsid[i].aucSsid,
                      rScanRequest.rSsid[i].u4SsidLen, request->ssids[i].ssid,
                      request->ssids[i].ssid_len);
        }
    } else {
        return -EINVAL;
    }

    rScanRequest.u4IELength = request->ie_len;
    if (request->ie_len > 0) {
        rScanRequest.pucIE = (u8 *)(request->ie);
    }

#if CFG_SCAN_CHANNEL_SPECIFIED
    DBGLOG(REQ, WARN, "scan channel num = %d\n", request->n_channels);

    if (request->n_channels > MAXIMUM_OPERATION_CHANNEL_LIST) {
        DBGLOG(REQ, WARN,
               "scan channel num (%d) exceeds %d, do a full scan instead\n",
               request->n_channels, MAXIMUM_OPERATION_CHANNEL_LIST);
        rScanRequest.ucChannelListNum = 0;
    } else {
        rScanRequest.ucChannelListNum = request->n_channels;
        for (i = 0; i < request->n_channels; i++) {
            rScanRequest.arChnlInfoList[i].eBand =
                kalCfg80211ToMtkBand(request->channels[i]->band);
            rScanRequest.arChnlInfoList[i].u4CenterFreq1 =
                request->channels[i]->center_freq;
            rScanRequest.arChnlInfoList[i].u4CenterFreq2 = 0;
            rScanRequest.arChnlInfoList[i].u2PriChnlFreq =
                request->channels[i]->center_freq;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0)
            rScanRequest.arChnlInfoList[i].ucChnlBw = request->scan_width;
#else
            rScanRequest.arChnlInfoList[i].ucChnlBw = 0;
#endif
            rScanRequest.arChnlInfoList[i].ucChannelNum =
                ieee80211_frequency_to_channel(
                    request->channels[i]->center_freq);
        }
    }
#endif

    /* 2018/04/18 frog: The point should be ready before doing IOCTL. */
    prGlueInfo->prScanRequest = request;

    rStatus = kalIoctl(prGlueInfo, wlanoidSetBssidListScanAdv, &rScanRequest,
                       sizeof(PARAM_SCAN_REQUEST_ADV_T), false, false, false,
                       &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "scan error:%lx\n", rStatus);
        /* 2018/04/18 frog: Remove pointer if IOCTL fail. */
        prGlueInfo->prScanRequest = NULL;

        return -EINVAL;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for abort an ongoing scan. The driver
 * shall indicate the status of the scan through cfg80211_scan_done()
 *
 * @param wiphy - pointer of wireless hardware description
 *        wdev - pointer of  wireless device state
 *
 */
/*----------------------------------------------------------------------------*/
void mtk_cfg80211_abort_scan(struct wiphy *wiphy, struct wireless_dev *wdev)
{
    u32 u4SetInfoLen = 0;
    WLAN_STATUS rStatus;
    P_GLUE_INFO_T prGlueInfo = NULL;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    rStatus = kalIoctl(prGlueInfo, wlanoidAbortScan, NULL, 1, false, false,
                       true, &u4SetInfoLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "wlanoidAbortScan fail 0x%x\n", rStatus);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting auth to
 *        the ESS with the specified parameters
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_auth(struct wiphy *wiphy, struct net_device *ndev,
                      struct cfg80211_auth_request *req)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u32 rStatus;
    u32 u4BufLen;
    PARAM_CONNECT_T rNewSsid;
    ENUM_PARAM_OP_MODE_T eOpMode;
    u8 wepBuf[48];
    P_CONNECTION_SETTINGS_T prConnSettings = NULL;
#if CFG_SUPPORT_REPLAY_DETECTION
    P_BSS_INFO_T prBssInfo = NULL;
    struct SEC_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;
#endif
    P_PARAM_WEP_T prWepKey;
    /*Is auth parameter needed to be updated to AIS.*/
    u8 fgNewAuthParam = false;
    P_STA_RECORD_T prStaRec = NULL;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

#if KERNEL_VERSION(4, 9, 113) >= CFG80211_VERSION_CODE
    if (req->sae_data_len != 0) {
        DBGLOG(REQ, INFO, "[wlan] mtk_cfg80211_auth %p %zu\n", req->sae_data,
               req->sae_data_len);
    }
#else
    if (req->auth_data_len != 0) {
        DBGLOG(REQ, INFO, "[wlan] mtk_cfg80211_auth %p %zu\n", req->auth_data,
               req->auth_data_len);
    }
#endif
    DBGLOG(REQ, STATE, "auth to  BSS [" MACSTR "]\n",
           MAC2STR((u8 *)req->bss->bssid));
    DBGLOG(REQ, STATE, "auth_type:%d\n", req->auth_type);

    prConnSettings = &prGlueInfo->prAdapter->rWifiVar.rConnSettings;

    /* <1>Set OP mode */
    if (prGlueInfo->prAdapter->rWifiVar.rConnSettings.eOPMode >
        NET_TYPE_AUTO_SWITCH) {
        eOpMode = NET_TYPE_AUTO_SWITCH;
    } else {
        eOpMode = prGlueInfo->prAdapter->rWifiVar.rConnSettings.eOPMode;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSetInfrastructureMode, &eOpMode,
                       sizeof(eOpMode), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(INIT, INFO, "wlanoidSetInfrastructureMode fail 0x%x\n", rStatus);
        return -EFAULT;
    }

    /*<2> Set  Auth data */
    prConnSettings->ucAuthDataLen = 0;
#if KERNEL_VERSION(4, 9, 113) >= CFG80211_VERSION_CODE
    if (req->sae_data_len != 0) {
        if (req->sae_data_len > AUTH_DATA_MAX_LEN) {
            DBGLOG(INIT, WARN, "request auth with unexpected length:%d\n",
                   req->sae_data_len);
            return -EFAULT;
        }

        kalMemCopy(prConnSettings->aucAuthData, req->sae_data,
                   req->sae_data_len);
        prConnSettings->ucAuthDataLen = req->sae_data_len;

        DBGLOG(INIT, INFO, "Dump auth data in connectSettings, auth len:%d\n",
               prConnSettings->ucAuthDataLen);
        DBGLOG_MEM8(REQ, INFO, prConnSettings->aucAuthData, req->sae_data_len);
    }
#else
    if (req->auth_data_len != 0) {
        if (req->auth_data_len > AUTH_DATA_MAX_LEN) {
            DBGLOG(INIT, WARN, "request auth with unexpected length:%d\n",
                   req->auth_data_len);
            return -EFAULT;
        }

        kalMemCopy(prConnSettings->aucAuthData, req->auth_data,
                   req->auth_data_len);
        prConnSettings->ucAuthDataLen = req->auth_data_len;

        DBGLOG(INIT, INFO, "Dump auth data in connectSettings, auth len:%d\n",
               prConnSettings->ucAuthDataLen);
        DBGLOG_MEM8(REQ, INFO, prConnSettings->aucAuthData, req->auth_data_len);
    }
#endif
    /*<2> Set ChannelNum */
    if (req->bss->channel->center_freq) {
        prConnSettings->ucChannelNum =
            nicFreq2ChannelNum(req->bss->channel->center_freq * 1000);
        DBGLOG(RSN, INFO, "set prConnSettings->ucChannelNum:%d\n",
               prConnSettings->ucChannelNum);
    } else {
        prConnSettings->ucChannelNum = 0;
        DBGLOG(RSN, INFO, "req->bss->channel->center_freq is NULL.\n");
    }

#if CFG_SUPPORT_REPLAY_DETECTION
    /* reset Detect replay information */
    prBssInfo = GET_BSS_INFO_BY_INDEX(
        prGlueInfo->prAdapter, prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex);

    prDetRplyInfo = &prBssInfo->rDetRplyInfo;

    kalMemZero(prDetRplyInfo, sizeof(struct SEC_DETECT_REPLAY_INFO));
#endif

    switch (req->auth_type) {
    case NL80211_AUTHTYPE_OPEN_SYSTEM:
        if (!(prGlueInfo->rWpaInfo.u4AuthAlg & AUTH_TYPE_OPEN_SYSTEM)) {
            fgNewAuthParam = true;
        }
        prGlueInfo->rWpaInfo.u4AuthAlg = AUTH_TYPE_OPEN_SYSTEM;
        break;

    case NL80211_AUTHTYPE_SHARED_KEY:
        if (!(prGlueInfo->rWpaInfo.u4AuthAlg & AUTH_TYPE_SHARED_KEY)) {
            fgNewAuthParam = true;
        }
        prGlueInfo->rWpaInfo.u4AuthAlg = AUTH_TYPE_SHARED_KEY;
        break;

    case NL80211_AUTHTYPE_SAE:
        if (!(prGlueInfo->rWpaInfo.u4AuthAlg & AUTH_TYPE_SAE)) {
            fgNewAuthParam = true;
        }
        prGlueInfo->rWpaInfo.u4AuthAlg = AUTH_TYPE_SAE;
        break;

    default:
        DBGLOG(REQ, WARN,
               "Auth type: %ld not support, use default OPEN system\n",
               req->auth_type);
        prGlueInfo->rWpaInfo.u4AuthAlg = AUTH_TYPE_OPEN_SYSTEM;
        break;
    }
    DBGLOG(REQ, INFO, "Auth Algorithm : %ld\n", prGlueInfo->rWpaInfo.u4AuthAlg);

    if (req->key_len != 0) {
        /* NL80211 only set the Tx wep key while connect,
         * the max 4 wep key set prior via add key cmd
         */

        if (!(prGlueInfo->rWpaInfo.u4AuthAlg & AUTH_TYPE_SHARED_KEY)) {
            DBGLOG(REQ, WARN, "Auth Algorithm : %ld with wep key\n",
                   prGlueInfo->rWpaInfo.u4AuthAlg);
        }

        prWepKey = (P_PARAM_WEP_T)wepBuf;

        kalMemZero(prWepKey, sizeof(PARAM_WEP_T));
        prWepKey->u4Length =
            OFFSET_OF(PARAM_WEP_T, aucKeyMaterial) + req->key_len;
        prWepKey->u4KeyLength = (u32)req->key_len;
        prWepKey->u4KeyIndex = (u32)req->key_idx;
        prWepKey->u4KeyIndex |= IS_TRANSMIT_KEY;
        if (prWepKey->u4KeyLength > MAX_KEY_LEN) {
            DBGLOG(REQ, WARN, "Too long key length (%u)\n",
                   prWepKey->u4KeyLength);
            return -EINVAL;
        }
        kalMemCopy(prWepKey->aucKeyMaterial, req->key, prWepKey->u4KeyLength);

        rStatus = kalIoctl(prGlueInfo, wlanoidSetAddWep, prWepKey,
                           prWepKey->u4Length, false, false, true, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(INIT, INFO, "wlanoidSetAddWep fail 0x%x\n", rStatus);
            return -EFAULT;
        }
    }
    kalMemZero(&rNewSsid, sizeof(PARAM_CONNECT_T));
    rNewSsid.pucBssid = (u8 *)req->bss->bssid;

    if (!EQUAL_MAC_ADDR(prConnSettings->aucBSSID, req->bss->bssid)) {
        fgNewAuthParam = true;
    }

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
    DBGLOG(REQ, WARN, "req IE len %d, ssid %.*s, ssid_len %d\n",
           req->bss->ies->len, SSID_IE(req->bss->ies->data)->ucLength,
           SSID_IE(req->bss->ies->data)->aucSSID,
           SSID_IE(req->bss->ies->data)->ucLength);
    if (req->bss->ies->len != 0 && IE_ID(req->bss->ies->data) == ELEM_ID_SSID) {
        rNewSsid.pucSsid = SSID_IE(req->bss->ies->data)->aucSSID;
        rNewSsid.u4SsidLen = SSID_IE(req->bss->ies->data)->ucLength;
    }
#endif
    /* rNewSsid.pucSsid = (u8 *)sme->ssid;*/
    /* rNewSsid.u4SsidLen = sme->ssid_len;*/

    DBGLOG(REQ, STATE, "auth to  BSS [" MACSTR "],UpperReq [" MACSTR "]\n",
           MAC2STR(rNewSsid.pucBssid), MAC2STR((u8 *)req->bss->bssid));

    prConnSettings->fgIsSendAssoc = false;
    if (!prConnSettings->fgIsConnInitialized || fgNewAuthParam) {
        /* [TODO] to consider if bssid/auth_alg changed
         * (need to update to AIS)
         */
        if (!prConnSettings->fgIsConnInitialized && fgNewAuthParam) {
            DBGLOG(REQ, INFO, "auth param update\n");
        }

        rStatus = kalIoctl(prGlueInfo, wlanoidSetConnect, (void *)&rNewSsid,
                           sizeof(PARAM_CONNECT_T), false, false, true,
                           &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(REQ, WARN, "set SSID:%x\n", rStatus);
            return -EINVAL;
        }
    } else {
        /* skip join initial flow
         * when it has been completed with the same auth parameters
         */
        prStaRec = cnmGetStaRecByAddress(
            prGlueInfo->prAdapter,
            prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex, rNewSsid.pucBssid);
        if (prStaRec) {
            saaSendAuthAssoc(prGlueInfo->prAdapter, prStaRec);
            DBGLOG(REQ, STATE, "Send auth \n");
        } else {
            DBGLOG(REQ, WARN, "can't send auth since can't find StaRec\n");
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to connect to
 *        the ESS with the specified parameters
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_connect(struct wiphy *wiphy, struct net_device *ndev,
                         struct cfg80211_connect_params *sme)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    u32 u4BufLen;
    ENUM_PARAM_ENCRYPTION_STATUS_T eEncStatus;
    ENUM_PARAM_AUTH_MODE_T eAuthMode;
    u32 cipher;
    PARAM_CONNECT_T rNewSsid;
    u8 fgCarryWPSIE = false;
    ENUM_PARAM_OP_MODE_T eOpMode;
    u8 wepBuf[48];
    u32 i, u4AkmSuite = 0;
    P_DOT11_RSNA_CONFIG_AUTHENTICATION_SUITES_ENTRY prEntry;
#if CFG_SUPPORT_REPLAY_DETECTION
    P_BSS_INFO_T prBssInfo = NULL;
    struct SEC_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;
#endif

    if (sme->channel) {
        /* Prevents NULL pointer dereference if sme->channel is NULL */
        DBGLOG(REQ, WARN, "BSSID[" MACSTR "] channel[%d]\n",
               MAC2STR(sme->bssid), sme->channel->center_freq);
    } else {
        DBGLOG(REQ, WARN, "BSSID[" MACSTR "] \n", MAC2STR(sme->bssid));
    }
    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    if (prGlueInfo->prAdapter->rWifiVar.rConnSettings.eOPMode >
        NET_TYPE_AUTO_SWITCH) {
        eOpMode = NET_TYPE_AUTO_SWITCH;
    } else {
        eOpMode = prGlueInfo->prAdapter->rWifiVar.rConnSettings.eOPMode;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSetInfrastructureMode, &eOpMode,
                       sizeof(eOpMode), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(INIT, INFO, "wlanoidSetInfrastructureMode fail 0x%lx\n",
               rStatus);
        return -EFAULT;
    }

    /* after set operation mode, key table are cleared */

    /* reset wpa info */
    prGlueInfo->rWpaInfo.u4WpaVersion = IW_AUTH_WPA_VERSION_DISABLED;
    prGlueInfo->rWpaInfo.u4KeyMgmt = 0;
    prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo.u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;
    prGlueInfo->rWpaInfo.fgPrivacyInvoke = false;

#if CFG_SUPPORT_REPLAY_DETECTION
    /* reset Detect replay information */
    prBssInfo = GET_BSS_INFO_BY_INDEX(
        prGlueInfo->prAdapter, prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex);

    prDetRplyInfo = &prBssInfo->rDetRplyInfo;
    kalMemZero(prDetRplyInfo, sizeof(struct SEC_DETECT_REPLAY_INFO));
#endif

#if CFG_SUPPORT_802_11W
    prGlueInfo->rWpaInfo.u4CipherGroupMgmt = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo.ucRSNMfpCap = RSN_AUTH_MFP_DISABLED;
    prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_DISABLED;
    switch (sme->mfp) {
    case NL80211_MFP_NO:
        prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_DISABLED;
        break;

    case NL80211_MFP_REQUIRED:
        prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_REQUIRED;
        break;

    default:
        prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_DISABLED;
        break;
    }
    /* DBGLOG(SCN, INFO, ("MFP=%d\n", prGlueInfo->rWpaInfo.u4Mfp)); */
#endif

    if (sme->crypto.wpa_versions & NL80211_WPA_VERSION_1) {
        prGlueInfo->rWpaInfo.u4WpaVersion = IW_AUTH_WPA_VERSION_WPA;
    } else if (sme->crypto.wpa_versions & NL80211_WPA_VERSION_2) {
        prGlueInfo->rWpaInfo.u4WpaVersion = IW_AUTH_WPA_VERSION_WPA2;
    } else {
        prGlueInfo->rWpaInfo.u4WpaVersion = IW_AUTH_WPA_VERSION_DISABLED;
    }

    switch (sme->auth_type) {
    case NL80211_AUTHTYPE_OPEN_SYSTEM:
        prGlueInfo->rWpaInfo.u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;
        break;

    case NL80211_AUTHTYPE_SHARED_KEY:
        prGlueInfo->rWpaInfo.u4AuthAlg = IW_AUTH_ALG_SHARED_KEY;
        break;

    default:
        prGlueInfo->rWpaInfo.u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM |
                                         IW_AUTH_ALG_SHARED_KEY;
        break;
    }

    if (sme->crypto.n_ciphers_pairwise) {
        DBGLOG(RSN, INFO, "[wlan] cipher pairwise (%x)\n",
               sme->crypto.ciphers_pairwise[0]);

        prGlueInfo->prAdapter->rWifiVar.rConnSettings.rRsnInfo
        .au4PairwiseKeyCipherSuite[0] = sme->crypto.ciphers_pairwise[0];
        switch (sme->crypto.ciphers_pairwise[0]) {
        case WLAN_CIPHER_SUITE_WEP40:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_WEP40;
            break;

        case WLAN_CIPHER_SUITE_WEP104:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_WEP104;
            break;

        case WLAN_CIPHER_SUITE_TKIP:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_TKIP;
            break;

        case WLAN_CIPHER_SUITE_CCMP:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_CCMP;
            break;

        case WLAN_CIPHER_SUITE_AES_CMAC:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_CCMP;
            break;

#if CFG_SUPPORT_SUITB
        case WLAN_CIPHER_SUITE_BIP_GMAC_256:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_GCMP256;
            break;

        case WLAN_CIPHER_SUITE_GCMP_256:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_GCMP256;
            break;

#endif
        default:
            DBGLOG(REQ, WARN, "invalid cipher pairwise (%d)\n",
                   sme->crypto.ciphers_pairwise[0]);
            return -EINVAL;
        }
    }

    if (sme->crypto.cipher_group) {
        prGlueInfo->prAdapter->rWifiVar.rConnSettings.rRsnInfo
        .u4GroupKeyCipherSuite = sme->crypto.cipher_group;
        switch (sme->crypto.cipher_group) {
        case WLAN_CIPHER_SUITE_WEP40:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_WEP40;
            break;

        case WLAN_CIPHER_SUITE_WEP104:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_WEP104;
            break;

        case WLAN_CIPHER_SUITE_TKIP:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_TKIP;
            break;

        case WLAN_CIPHER_SUITE_CCMP:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_CCMP;
            break;

        case WLAN_CIPHER_SUITE_AES_CMAC:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_CCMP;
            break;

#if CFG_SUPPORT_SUITB
        case WLAN_CIPHER_SUITE_BIP_GMAC_256:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_GCMP256;
            break;

        case WLAN_CIPHER_SUITE_GCMP_256:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_GCMP256;
            break;

        case WLAN_CIPHER_SUITE_NO_GROUP_ADDR:
            break;

#endif
        default:
            DBGLOG(REQ, WARN, "invalid cipher group (%d)\n",
                   sme->crypto.cipher_group);
            return -EINVAL;
        }
    }

    /* DBGLOG(SCN, INFO, ("akm_suites=%x\n", sme->crypto.akm_suites[0])); */
    if (sme->crypto.n_akm_suites) {
        prGlueInfo->prAdapter->rWifiVar.rConnSettings.rRsnInfo
        .au4AuthKeyMgtSuite[0] = sme->crypto.akm_suites[0];
        if (prGlueInfo->rWpaInfo.u4WpaVersion == IW_AUTH_WPA_VERSION_WPA) {
            switch (sme->crypto.akm_suites[0]) {
            case WLAN_AKM_SUITE_8021X:
                eAuthMode = AUTH_MODE_WPA;
                u4AkmSuite = WPA_AKM_SUITE_802_1X;
                break;

            case WLAN_AKM_SUITE_PSK:
                eAuthMode = AUTH_MODE_WPA_PSK;
                u4AkmSuite = WPA_AKM_SUITE_PSK;
                break;

            default:
                DBGLOG(REQ, WARN, "invalid Akm Suite (%d)\n",
                       sme->crypto.akm_suites[0]);
                return -EINVAL;
            }
        } else if (prGlueInfo->rWpaInfo.u4WpaVersion ==
                   IW_AUTH_WPA_VERSION_WPA2) {
            switch (sme->crypto.akm_suites[0]) {
            case WLAN_AKM_SUITE_8021X:
                eAuthMode = AUTH_MODE_WPA2;
                u4AkmSuite = RSN_AKM_SUITE_802_1X;
                break;

            case WLAN_AKM_SUITE_PSK:
                eAuthMode = AUTH_MODE_WPA2_PSK;
                u4AkmSuite = RSN_AKM_SUITE_PSK;
                break;

#if CFG_SUPPORT_802_11W
            /* Notice:: Need kernel patch!! */
            case WLAN_AKM_SUITE_8021X_SHA256:
                eAuthMode = AUTH_MODE_WPA2;
                u4AkmSuite = RSN_AKM_SUITE_802_1X_SHA256;
                break;

            case WLAN_AKM_SUITE_PSK_SHA256:
                eAuthMode = AUTH_MODE_WPA2_PSK;
                u4AkmSuite = RSN_AKM_SUITE_PSK_SHA256;
                break;

#endif
#if CFG_SUPPORT_SUITB
            case WLAN_AKM_SUITE_8021X_SUITE_B:
                eAuthMode = AUTH_MODE_WPA2_PSK;
                u4AkmSuite = RSN_AKM_SUITE_8021X_SUITE_B_192;
                break;

            case WLAN_AKM_SUITE_8021X_SUITE_B_192:
                eAuthMode = AUTH_MODE_WPA2_PSK;
                u4AkmSuite = RSN_AKM_SUITE_8021X_SUITE_B_192;
                break;

#endif
#if CFG_SUPPORT_OWE
            case WLAN_AKM_SUITE_OWE:
                eAuthMode = AUTH_MODE_WPA2_PSK;
                u4AkmSuite = RSN_AKM_SUITE_OWE;
                break;

#endif
            default:
                DBGLOG(REQ, WARN, "invalid Akm Suite (%d)\n",
                       sme->crypto.akm_suites[0]);
                return -EINVAL;
            }
        }
    }

    if (prGlueInfo->rWpaInfo.u4WpaVersion == IW_AUTH_WPA_VERSION_DISABLED) {
        eAuthMode =
            (prGlueInfo->rWpaInfo.u4AuthAlg == IW_AUTH_ALG_OPEN_SYSTEM) ?
            AUTH_MODE_OPEN :
            AUTH_MODE_AUTO_SWITCH;
    }

    prGlueInfo->rWpaInfo.fgPrivacyInvoke = sme->privacy;
    prGlueInfo->fgWpsActive = false;

    if (sme->ie && sme->ie_len > 0) {
        WLAN_STATUS rStatus;
        u32 u4BufLen;
        u8 *prDesiredIE = NULL;
        u8 *pucIEStart = (u8 *)sme->ie;

#if CFG_SUPPORT_WPS2
        if (wextSrchDesiredWPSIE(pucIEStart, sme->ie_len, 0xDD,
                                 (u8 **)&prDesiredIE)) {
            prGlueInfo->fgWpsActive = true;
            fgCarryWPSIE = true;

            rStatus = kalIoctl(prGlueInfo, wlanoidSetWSCAssocInfo, prDesiredIE,
                               IE_SIZE(prDesiredIE), false, false, false,
                               &u4BufLen);
            if (rStatus != WLAN_STATUS_SUCCESS) {
                DBGLOG(SEC, WARN, "[WSC] set WSC assoc info error:%lx\n",
                       rStatus);
            }
        }
#endif

        if (wextSrchDesiredWPAIE(pucIEStart, sme->ie_len, 0x30,
                                 (u8 **)&prDesiredIE)) {
            RSN_INFO_T rRsnInfo;

            if (rsnParseRsnIE(prGlueInfo->prAdapter,
                              (P_RSN_INFO_ELEM_T)prDesiredIE, &rRsnInfo)) {
#if CFG_SUPPORT_802_11W
                /* Fill RSNE MFP Cap */
                if (rRsnInfo.u2RsnCap & ELEM_WPA_CAP_MFPC) {
                    prGlueInfo->rWpaInfo.u4CipherGroupMgmt =
                        rRsnInfo.u4GroupMgmtKeyCipherSuite;
                    prGlueInfo->rWpaInfo.ucRSNMfpCap = RSN_AUTH_MFP_OPTIONAL;
                    if (rRsnInfo.u2RsnCap & ELEM_WPA_CAP_MFPR) {
                        prGlueInfo->rWpaInfo.ucRSNMfpCap =
                            RSN_AUTH_MFP_REQUIRED;
                    }
                } else {
                    prGlueInfo->rWpaInfo.ucRSNMfpCap = RSN_AUTH_MFP_DISABLED;
                }
#endif
            }
        }
    }

    /* clear WSC Assoc IE buffer in case WPS IE is not detected */
    if (fgCarryWPSIE == false) {
        kalMemZero(&prGlueInfo->aucWSCAssocInfoIE, 200);
        prGlueInfo->u2WSCAssocInfoIELen = 0;
    }

    /*Fill WPA info - mfp setting */
    /* Must put after paring RSNE from upper layer
     * for prGlueInfo->rWpaInfo.ucRSNMfpCap assignment
     */
#if CFG_SUPPORT_802_11W
    switch (sme->mfp) {
    case NL80211_MFP_NO:
        prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_DISABLED;
        /* Change Mfp parameter from DISABLED to OPTIONAL
         * if upper layer set MFPC = 1 in RSNE
         * since upper layer can't bring MFP OPTIONAL information
         * to driver by sme->mfp
         */
        if (prGlueInfo->rWpaInfo.ucRSNMfpCap == RSN_AUTH_MFP_OPTIONAL) {
            prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_OPTIONAL;
        } else if (prGlueInfo->rWpaInfo.ucRSNMfpCap == RSN_AUTH_MFP_REQUIRED) {
            DBGLOG(REQ, ERROR, "param(DISABLED) conflict with cap(REQUIRED)\n");
        }
        break;

    case NL80211_MFP_REQUIRED:
        prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_REQUIRED;
        break;

    default:
        prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_DISABLED;
        break;
    }
#endif

    rStatus = kalIoctl(prGlueInfo, wlanoidSetAuthMode, &eAuthMode,
                       sizeof(eAuthMode), false, false, false, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "set auth mode error:%lx\n", rStatus);
    }

    /* Enable the specific AKM suite only. */
    for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++) {
        prEntry = &prGlueInfo->prAdapter->rMib
                  .dot11RSNAConfigAuthenticationSuitesTable[i];

        if (prEntry->dot11RSNAConfigAuthenticationSuite == u4AkmSuite) {
            prEntry->dot11RSNAConfigAuthenticationSuiteEnabled = true;
        } else {
            prEntry->dot11RSNAConfigAuthenticationSuiteEnabled = false;
        }
    }

    cipher = prGlueInfo->rWpaInfo.u4CipherGroup |
             prGlueInfo->rWpaInfo.u4CipherPairwise;

    if (1  /* prGlueInfo->rWpaInfo.fgPrivacyInvoke */) {
#if CFG_SUPPORT_SUITB
        if (cipher & IW_AUTH_CIPHER_GCMP256) {
            eEncStatus = ENUM_ENCRYPTION4_ENABLED;
        } else
#endif
        if (cipher & IW_AUTH_CIPHER_CCMP) {
            eEncStatus = ENUM_ENCRYPTION3_ENABLED;
        } else if (cipher & IW_AUTH_CIPHER_TKIP) {
            eEncStatus = ENUM_ENCRYPTION2_ENABLED;
        } else if (cipher & (IW_AUTH_CIPHER_WEP104 | IW_AUTH_CIPHER_WEP40)) {
            eEncStatus = ENUM_ENCRYPTION1_ENABLED;
        } else if (cipher & IW_AUTH_CIPHER_NONE) {
            if (prGlueInfo->rWpaInfo.fgPrivacyInvoke) {
                eEncStatus = ENUM_ENCRYPTION1_ENABLED;
            } else {
                eEncStatus = ENUM_ENCRYPTION_DISABLED;
            }
        } else {
            eEncStatus = ENUM_ENCRYPTION_DISABLED;
        }
    } else {
        eEncStatus = ENUM_ENCRYPTION_DISABLED;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSetEncryptionStatus, &eEncStatus,
                       sizeof(eEncStatus), false, false, false, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "set encryption mode error:%lx\n", rStatus);
    }

    if (sme->key_len != 0 &&
        prGlueInfo->rWpaInfo.u4WpaVersion == IW_AUTH_WPA_VERSION_DISABLED) {
        /* NL80211 only set the Tx wep key while connect, the max 4 wep
         * key set prior via add key cmd */
        P_PARAM_WEP_T prWepKey = (P_PARAM_WEP_T)wepBuf;

        kalMemZero(prWepKey, sizeof(PARAM_WEP_T));
        prWepKey->u4Length =
            OFFSET_OF(PARAM_WEP_T, aucKeyMaterial) + sme->key_len;
        prWepKey->u4KeyLength = (u32)sme->key_len;
        prWepKey->u4KeyIndex = (u32)sme->key_idx;
        prWepKey->u4KeyIndex |= IS_TRANSMIT_KEY;
        if (prWepKey->u4KeyLength > MAX_KEY_LEN) {
            DBGLOG(REQ, WARN, "Too long key length (%lu)\n",
                   prWepKey->u4KeyLength);

            return -EINVAL;
        }
        kalMemCopy(prWepKey->aucKeyMaterial, sme->key, prWepKey->u4KeyLength);

        rStatus = kalIoctl(prGlueInfo, wlanoidSetAddWep, prWepKey,
                           prWepKey->u4Length, false, false, true, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(INIT, INFO, "wlanoidSetAddWep fail 0x%lx\n", rStatus);

            return -EFAULT;
        }
    }

    rNewSsid.u4CenterFreq = sme->channel ? sme->channel->center_freq : 0;
    rNewSsid.pucBssid = (u8 *)sme->bssid;
    rNewSsid.pucSsid = (u8 *)sme->ssid;
    rNewSsid.u4SsidLen = sme->ssid_len;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetConnect, (void *)&rNewSsid,
                       sizeof(PARAM_CONNECT_T), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "set SSID:%x\n", rStatus);

        return -EINVAL;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to disconnect from
 *        currently connected ESS
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *ndev,
                            u16 reason_code)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen;

    DBGLOG(REQ, WARN, "reason code[%d]\n", reason_code);
    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetDisassociate, NULL, 0, false,
                       false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "disassociate error:%lx\n", rStatus);
        return -EFAULT;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to deauth from
 *        currently connected ESS
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_deauth(struct wiphy *wiphy, struct net_device *ndev,
                        struct cfg80211_deauth_request *req)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u32 rStatus;
    u32 u4BufLen;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    DBGLOG(REQ, STATE, "mtk_cfg80211_deauth\n");

    kalIndicateStatusAndComplete(prGlueInfo, WLAN_STATUS_JOIN_ABORT, NULL, 0);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetDisassociate, NULL, 0, false,
                       false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "disassociate error:%x\n", rStatus);
        return -EFAULT;
    }

    return 0;
}

int mtk_cfg80211_disassoc(struct wiphy *wiphy, struct net_device *ndev,
                          struct cfg80211_disassoc_request *req)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;
    u32 rStatus;
    u32 u4BufLen;

    ASSERT(wiphy);

    prGlueInfo = *((P_GLUE_INFO_T *)wiphy_priv(wiphy));
    ASSERT(prGlueInfo);

    DBGLOG(REQ, STATE, "mtk_cfg80211_disassoc.\n");

    kalIndicateStatusAndComplete(prGlueInfo, WLAN_STATUS_JOIN_ABORT, NULL, 0);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetDisassociate, NULL, 0, false,
                       false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "disassociate error:%x\n", rStatus);
        return -EFAULT;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to join an IBSS group
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_join_ibss(struct wiphy *wiphy, struct net_device *ndev,
                           struct cfg80211_ibss_params *params)
{
    PARAM_SSID_T rNewSsid;
    P_GLUE_INFO_T prGlueInfo = NULL;
    u32 u4ChnlFreq;  /* Store channel or frequency information */
    u32 u4BufLen = 0, u4SsidLen = 0;
    WLAN_STATUS rStatus;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    /* set channel */
    if (params->channel_fixed) {
        u4ChnlFreq = params->chandef.center_freq1;

        rStatus = kalIoctl(prGlueInfo, wlanoidSetFrequency, &u4ChnlFreq,
                           sizeof(u4ChnlFreq), false, false, false, &u4BufLen);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -EFAULT;
        }
    }

    /* set SSID */
    u4SsidLen = (params->ssid_len > PARAM_MAX_LEN_SSID) ? PARAM_MAX_LEN_SSID :
                params->ssid_len;
    kalMemCopy(rNewSsid.aucSsid, params->ssid, u4SsidLen);
    rStatus = kalIoctl(prGlueInfo, wlanoidSetSsid, (void *)&rNewSsid,
                       sizeof(PARAM_SSID_T), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "set SSID:%lx\n", rStatus);
        return -EFAULT;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to leave from IBSS group
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_leave_ibss(struct wiphy *wiphy, struct net_device *ndev)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    u32 u4BufLen;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetDisassociate, NULL, 0, false,
                       false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "disassociate error:%lx\n", rStatus);
        return -EFAULT;
    }

    return 0;
}

#ifdef SUPPORT_ENFORCE_PWR_MODE
/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to configure
 *        WLAN power managemenet
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_set_power_mgmt(struct wiphy *wiphy, struct net_device *ndev,
                                bool enabled, int timeout)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    u32 u4BufLen;
    PARAM_POWER_MODE_T rPowerMode;
    PARAM_POWER_MODE eEnforcePowerMode = Param_PowerModeMax;
    P_BSS_INFO_T prBssInfo = NULL;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    if (!prGlueInfo) {
        return -EFAULT;
    }

    if (!prGlueInfo->prAdapter->prAisBssInfo) {
        return -EFAULT;
    }

    prBssInfo = prGlueInfo->prAdapter->prAisBssInfo;

    if (enabled) {
        if (timeout == -1) {
            rPowerMode.ePowerMode = Param_PowerModeFast_PSP;
        } else {
            rPowerMode.ePowerMode = Param_PowerModeMAX_PSP;
        }
    } else {
        rPowerMode.ePowerMode = Param_PowerModeCAM;
    }

    rPowerMode.ucBssIdx = prBssInfo->ucBssIndex;

    if (prBssInfo->ePowerModeFromUser != rPowerMode.ePowerMode) {
        /* 1. Store user's PS mode, it is used for wlanSuspendPmHandle()
         * when WoW is disabled
         * 2. Store user's PS mode for restoring when we do not enforce
         * power mode anymore
         */
        DBGLOG(INIT, STATE, "Store user's PS mode:%d\n", rPowerMode.ePowerMode);
        prBssInfo->ePowerModeFromUser = rPowerMode.ePowerMode;
    }

    /* Configured in wifi.cfg to keep CAM during activity  */
    if (prGlueInfo->prAdapter->rWifiVar.ucWlanSetCamDuringAct) {
        /* Keep CAM before wlan suspends */
        rPowerMode.ePowerMode = Param_PowerModeCAM;
    }

    if ((prBssInfo->eBand == BAND_2G4) &&
        (prGlueInfo->prAdapter->rWifiVar.ucEnforce2G < Param_PowerModeMax)) {
        eEnforcePowerMode =
            (PARAM_POWER_MODE)prGlueInfo->prAdapter->rWifiVar.ucEnforce2G;
    } else if ((prBssInfo->eBand == BAND_5G) &&
               (prGlueInfo->prAdapter->rWifiVar.ucEnforce5G <
                Param_PowerModeMax)) {
        eEnforcePowerMode =
            (PARAM_POWER_MODE)prGlueInfo->prAdapter->rWifiVar.ucEnforce5G;
    }

    if (eEnforcePowerMode < Param_PowerModeMax) {
        rPowerMode.ePowerMode = eEnforcePowerMode;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile,
                       &rPowerMode, sizeof(PARAM_POWER_MODE_T), false, false,
                       true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "set_power_mgmt error:%lx\n", rStatus);
        return -EFAULT;
    }

    return 0;
}
#else
/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to configure
 *        WLAN power managemenet
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_set_power_mgmt(struct wiphy *wiphy, struct net_device *ndev,
                                bool enabled, int timeout)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    u32 u4BufLen;
    PARAM_POWER_MODE_T rPowerMode;
    PARAM_POWER_MODE eEnforcePowerMode;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    if (!prGlueInfo) {
        return -EFAULT;
    }

    if (!prGlueInfo->prAdapter->prAisBssInfo) {
        return -EFAULT;
    }

    if (enabled && ((prGlueInfo->prAdapter->prAisBssInfo->eBand == BAND_5G) ||
                    (!prGlueInfo->prAdapter->rWifiVar.ucEnforceCAM2G))) {
        if (timeout == -1) {
            rPowerMode.ePowerMode = Param_PowerModeFast_PSP;
        } else {
            rPowerMode.ePowerMode = Param_PowerModeMAX_PSP;
        }
    } else {
        rPowerMode.ePowerMode = Param_PowerModeCAM;
    }

    rPowerMode.ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex;

    if ((prGlueInfo->prAdapter->prAisBssInfo->ePowerModeFromUser !=
         rPowerMode.ePowerMode) &&
        (prGlueInfo->prAdapter->rWifiVar.ucWlanSetCamDuringAct ||
         prGlueInfo->prAdapter->rWifiVar.ucEnforcePSMode <
         Param_PowerModeMax)) {
        /* 1. Store user's PS mode, it is used for wlanSuspendPmHandle()
         * when WoW is disabled
         * 2. Store user's PS mode for restoring when we do not enforce
         * power mode anymore
         */
        DBGLOG(INIT, STATE, "Set CAM and store user's PS mode:%d\n",
               rPowerMode.ePowerMode);
        prGlueInfo->prAdapter->prAisBssInfo->ePowerModeFromUser =
            rPowerMode.ePowerMode;
    }

    /* Configured in wifi.cfg to keep CAM during activity  */
    if (prGlueInfo->prAdapter->rWifiVar.ucWlanSetCamDuringAct) {
        /* Keep CAM before wlan suspends */
        rPowerMode.ePowerMode = Param_PowerModeCAM;
    }

    eEnforcePowerMode =
        (PARAM_POWER_MODE)prGlueInfo->prAdapter->rWifiVar.ucEnforcePSMode;

    if (eEnforcePowerMode < Param_PowerModeMax) {
        rPowerMode.ePowerMode = eEnforcePowerMode;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile,
                       &rPowerMode, sizeof(PARAM_POWER_MODE_T), false, false,
                       true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "set_power_mgmt error:%lx\n", rStatus);
        return -EFAULT;
    }

    return 0;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to cache
 *        a PMKID for a BSSID
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_set_pmksa(struct wiphy *wiphy, struct net_device *ndev,
                           struct cfg80211_pmksa *pmksa)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    u32 u4BufLen;
    P_PARAM_PMKID_T prPmkid;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    prPmkid = (P_PARAM_PMKID_T)kalMemAlloc(8 + sizeof(PARAM_BSSID_INFO_T),
                                           VIR_MEM_TYPE);
    if (!prPmkid) {
        DBGLOG(INIT, INFO, "Can not alloc memory for IW_PMKSA_ADD\n");
        return -ENOMEM;
    }

    prPmkid->u4Length = 8 + sizeof(PARAM_BSSID_INFO_T);
    prPmkid->u4BSSIDInfoCount = 1;
    kalMemCopy(prPmkid->arBSSIDInfo->arBSSID, pmksa->bssid, 6);
    kalMemCopy(prPmkid->arBSSIDInfo->arPMKID, pmksa->pmkid, IW_PMKID_LEN);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetPmkid, prPmkid,
                       sizeof(PARAM_PMKID_T), false, false, false, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(INIT, INFO, "add pmkid error:%lx\n", rStatus);
    }
    kalMemFree(prPmkid, VIR_MEM_TYPE, 8 + sizeof(PARAM_BSSID_INFO_T));

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to remove
 *        a cached PMKID for a BSSID
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_del_pmksa(struct wiphy *wiphy, struct net_device *ndev,
                           struct cfg80211_pmksa *pmksa)
{
    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to flush
 *        all cached PMKID
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_flush_pmksa(struct wiphy *wiphy, struct net_device *ndev)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    u32 u4BufLen;
    P_PARAM_PMKID_T prPmkid;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    prPmkid = (P_PARAM_PMKID_T)kalMemAlloc(8, VIR_MEM_TYPE);
    if (!prPmkid) {
        DBGLOG(INIT, INFO, "Can not alloc memory for IW_PMKSA_FLUSH\n");
        return -ENOMEM;
    }

    prPmkid->u4Length = 8;
    prPmkid->u4BSSIDInfoCount = 0;

    rStatus = kalIoctl(prGlueInfo, wlanoidSetPmkid, prPmkid,
                       sizeof(PARAM_PMKID_T), false, false, false, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(INIT, INFO, "flush pmkid error:%lx\n", rStatus);
    }
    kalMemFree(prPmkid, VIR_MEM_TYPE, 8);

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for setting the rekey data
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_set_rekey_data(struct wiphy *wiphy, struct net_device *dev,
                                struct cfg80211_gtk_rekey_data *data)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u32 u4BufLen;
    P_PARAM_GTK_REKEY_DATA prGtkData;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    s32 i4Rslt = -EINVAL;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    /* if disable offload, we store key data here, and enable rekey offload
     * when enter wow */
    if (!prGlueInfo->prAdapter->rWifiVar.ucEapolOffload) {
        kalMemZero(prGlueInfo->rWpaInfo.aucKek, NL80211_KEK_LEN);
        kalMemZero(prGlueInfo->rWpaInfo.aucKck, NL80211_KCK_LEN);
        kalMemZero(prGlueInfo->rWpaInfo.aucReplayCtr, NL80211_REPLAY_CTR_LEN);
        kalMemCopy(prGlueInfo->rWpaInfo.aucKek, data->kek, NL80211_KEK_LEN);
        kalMemCopy(prGlueInfo->rWpaInfo.aucKck, data->kck, NL80211_KCK_LEN);
        kalMemCopy(prGlueInfo->rWpaInfo.aucReplayCtr, data->replay_ctr,
                   NL80211_REPLAY_CTR_LEN);

        return 0;
    }

    prGtkData = (P_PARAM_GTK_REKEY_DATA)kalMemAlloc(
        sizeof(PARAM_GTK_REKEY_DATA), VIR_MEM_TYPE);

    if (!prGtkData) {
        return WLAN_STATUS_SUCCESS;
    }

    DBGLOG(RSN, INFO, "cfg80211_set_rekey_data size( %d)\n", sizeof(*data));

    DBGLOG(RSN, INFO, "kek\n");
    DBGLOG_MEM8(PF, ERROR, (u8 *)data->kek, NL80211_KEK_LEN);
    DBGLOG(RSN, INFO, "kck\n");
    DBGLOG_MEM8(PF, ERROR, (u8 *)data->kck, NL80211_KCK_LEN);
    DBGLOG(RSN, INFO, "replay count\n");
    DBGLOG_MEM8(PF, ERROR, (u8 *)data->replay_ctr, NL80211_REPLAY_CTR_LEN);

    kalMemCopy(prGtkData->aucKek, data->kek, NL80211_KEK_LEN);
    kalMemCopy(prGtkData->aucKck, data->kck, NL80211_KCK_LEN);
    kalMemCopy(prGtkData->aucReplayCtr, data->replay_ctr,
               NL80211_REPLAY_CTR_LEN);

    prGtkData->ucBssIndex = prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex;

    prGtkData->u4Proto = NL80211_WPA_VERSION_2;
    if (prGlueInfo->rWpaInfo.u4WpaVersion == IW_AUTH_WPA_VERSION_WPA) {
        prGtkData->u4Proto = NL80211_WPA_VERSION_1;
    }

    if (prGlueInfo->rWpaInfo.u4CipherPairwise == IW_AUTH_CIPHER_TKIP) {
        prGtkData->u4PairwiseCipher = BIT(3);
    } else if (prGlueInfo->rWpaInfo.u4CipherPairwise == IW_AUTH_CIPHER_CCMP) {
        prGtkData->u4PairwiseCipher = BIT(4);
    } else {
        kalMemFree(prGtkData, VIR_MEM_TYPE, sizeof(PARAM_GTK_REKEY_DATA));
        return WLAN_STATUS_SUCCESS;
    }

    if (prGlueInfo->rWpaInfo.u4CipherGroup == IW_AUTH_CIPHER_TKIP) {
        prGtkData->u4GroupCipher = BIT(3);
    } else if (prGlueInfo->rWpaInfo.u4CipherGroup == IW_AUTH_CIPHER_CCMP) {
        prGtkData->u4GroupCipher = BIT(4);
    } else {
        kalMemFree(prGtkData, VIR_MEM_TYPE, sizeof(PARAM_GTK_REKEY_DATA));
        return WLAN_STATUS_SUCCESS;
    }

    prGtkData->u4KeyMgmt = prGlueInfo->rWpaInfo.u4KeyMgmt;
    prGtkData->u4MgmtGroupCipher = 0;

    prGtkData->ucRekeyMode = GTK_REKEY_CMD_MODE_OFFLOAD_ON;

    rStatus = kalIoctl(prGlueInfo, wlanoidSetGtkRekeyData, prGtkData,
                       sizeof(PARAM_GTK_REKEY_DATA), false, false, true,
                       &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(INIT, INFO, "set GTK rekey data error:%lx\n", rStatus);
    } else {
        i4Rslt = 0;
    }

    kalMemFree(prGtkData, VIR_MEM_TYPE, sizeof(PARAM_GTK_REKEY_DATA));

    return i4Rslt;
}

void mtk_cfg80211_mgmt_frame_register(IN struct wiphy *wiphy,
                                      IN struct wireless_dev *wdev,
                                      IN u16 frame_type, IN bool reg)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)NULL;

    do {
        DBGLOG(INIT, TRACE, "mtk_cfg80211_mgmt_frame_register\n");

        prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

        switch (frame_type) {
        case MAC_FRAME_PROBE_REQ:
            if (reg) {
                prGlueInfo->u4OsMgmtFrameFilter |=
                    PARAM_PACKET_FILTER_PROBE_REQ;
                DBGLOG(INIT, TRACE, "Open packet filer probe request\n");
            } else {
                prGlueInfo->u4OsMgmtFrameFilter &=
                    ~PARAM_PACKET_FILTER_PROBE_REQ;
                DBGLOG(INIT, TRACE, "Close packet filer probe request\n");
            }
            break;

        case MAC_FRAME_ACTION:
            if (reg) {
                prGlueInfo->u4OsMgmtFrameFilter |=
                    PARAM_PACKET_FILTER_ACTION_FRAME;
                DBGLOG(INIT, TRACE, "Open packet filer action frame.\n");
            } else {
                prGlueInfo->u4OsMgmtFrameFilter &=
                    ~PARAM_PACKET_FILTER_ACTION_FRAME;
                DBGLOG(INIT, TRACE, "Close packet filer action frame.\n");
            }
            break;

        default:
            DBGLOG(INIT, TRACE, "unsupported frame type:%x\n", frame_type);
            break;
        }

        if (prGlueInfo->prAdapter != NULL) {
            set_bit(GLUE_FLAG_FRAME_FILTER_AIS_BIT, &prGlueInfo->ulFlag);

            /* wake up main thread */
            wake_up_interruptible(&prGlueInfo->waitq);

            if (in_interrupt()) {
                DBGLOG(INIT, TRACE, "It is in interrupt level\n");
            }
        }
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to stay on a
 *        specified channel
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_remain_on_channel(struct wiphy *wiphy,
                                   struct wireless_dev *wdev,
                                   struct ieee80211_channel *chan,
                                   unsigned int duration, u64 *cookie)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Rslt = -EINVAL;
    P_MSG_REMAIN_ON_CHANNEL_T prMsgChnlReq = (P_MSG_REMAIN_ON_CHANNEL_T)NULL;

    do {
        if ((wiphy == NULL) || (wdev == NULL) || (chan == NULL) ||
            (cookie == NULL)) {
            break;
        }

        prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
        ASSERT(prGlueInfo);

        DBGLOG(INIT, INFO, "--> %s()\n", __func__);

        *cookie = prGlueInfo->u8Cookie++;

        prMsgChnlReq = cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
                                   sizeof(MSG_REMAIN_ON_CHANNEL_T));

        if (prMsgChnlReq == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prMsgChnlReq->rMsgHdr.eMsgId = MID_MNY_AIS_REMAIN_ON_CHANNEL;
        prMsgChnlReq->u8Cookie = *cookie;
        prMsgChnlReq->u4DurationMs = duration;

        prMsgChnlReq->ucChannelNum =
            nicFreq2ChannelNum(chan->center_freq * 1000);

        switch (chan->band) {
        case NL80211_BAND_2GHZ:
            prMsgChnlReq->eBand = BAND_2G4;
            break;

        case NL80211_BAND_5GHZ:
            prMsgChnlReq->eBand = BAND_5G;
            break;

        default:
            prMsgChnlReq->eBand = BAND_2G4;
            break;
        }

        prMsgChnlReq->eSco = CHNL_EXT_SCN;

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0, (P_MSG_HDR_T)prMsgChnlReq,
                    MSG_SEND_METHOD_BUF);

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to cancel staying
 *        on a specified channel
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy,
                                          struct wireless_dev *wdev,
                                          u64 cookie)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Rslt = -EINVAL;
    P_MSG_CANCEL_REMAIN_ON_CHANNEL_T prMsgChnlAbort =
        (P_MSG_CANCEL_REMAIN_ON_CHANNEL_T)NULL;

    do {
        if ((wiphy == NULL) || (wdev == NULL)) {
            break;
        }

        prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
        ASSERT(prGlueInfo);

        prMsgChnlAbort = cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
                                     sizeof(MSG_CANCEL_REMAIN_ON_CHANNEL_T));

        if (prMsgChnlAbort == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prMsgChnlAbort->rMsgHdr.eMsgId = MID_MNY_AIS_CANCEL_REMAIN_ON_CHANNEL;
        prMsgChnlAbort->u8Cookie = cookie;

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (P_MSG_HDR_T)prMsgChnlAbort, MSG_SEND_METHOD_BUF);

        i4Rslt = 0;
    } while (false);

    return i4Rslt;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to send a management frame
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev,
                         struct cfg80211_mgmt_tx_params *params, u64 *cookie)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Rslt = -EINVAL;
    P_MSG_MGMT_TX_REQUEST_T prMsgTxReq = (P_MSG_MGMT_TX_REQUEST_T)NULL;
    P_MSDU_INFO_T prMgmtFrame = (P_MSDU_INFO_T)NULL;
    u8 *pucFrameBuf = (u8 *)NULL;

    do {
        if ((wiphy == NULL) || (wdev == NULL) || (params == 0) ||
            (cookie == NULL)) {
            break;
        }

        prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
        ASSERT(prGlueInfo);

        *cookie = prGlueInfo->u8Cookie++;

        /* Channel & Channel Type & Wait time are ignored. */
        prMsgTxReq = cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
                                 sizeof(MSG_MGMT_TX_REQUEST_T));

        if (prMsgTxReq == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prMsgTxReq->fgNoneCckRate = false;
        prMsgTxReq->fgIsWaitRsp = true;

        prMgmtFrame = cnmMgtPktAlloc(
            prGlueInfo->prAdapter, (u32)(params->len + MAC_TX_RESERVED_FIELD));
        prMsgTxReq->prMgmtMsduInfo = prMgmtFrame;
        if (prMsgTxReq->prMgmtMsduInfo == NULL) {
            ASSERT(false);
            i4Rslt = -ENOMEM;
            break;
        }

        prMsgTxReq->u8Cookie = *cookie;
        prMsgTxReq->rMsgHdr.eMsgId = MID_MNY_AIS_MGMT_TX;

        pucFrameBuf = (u8 *)((unsigned long)prMgmtFrame->prPacket +
                             MAC_TX_RESERVED_FIELD);

        kalMemCopy(pucFrameBuf, params->buf, params->len);

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

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to cancel the wait time
 *        from transmitting a management frame on another channel
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_mgmt_tx_cancel_wait(struct wiphy *wiphy,
                                     struct wireless_dev *wdev, u64 cookie)
{
    P_GLUE_INFO_T prGlueInfo = NULL;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    DBGLOG(INIT, INFO, "--> %s()\n", __func__);

    /* not implemented */

    return -EINVAL;
}

#ifdef CONFIG_NL80211_TESTMODE

int mtk_cfg80211_testmode_get_sta_statistics(IN struct wiphy *wiphy,
                                             IN void *data, IN int len,
                                             IN P_GLUE_INFO_T prGlueInfo)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen;
    u32 u4LinkScore;
    u32 u4TotalError;
    u32 u4TxExceedThresholdCount;
    u32 u4TxTotalCount;

    P_NL80211_DRIVER_GET_STA_STATISTICS_PARAMS prParams = NULL;
    PARAM_GET_STA_STATISTICS rQueryStaStatistics;
    struct sk_buff *skb;

    ASSERT(wiphy);
    ASSERT(prGlueInfo);

    if (len < sizeof(struct _NL80211_DRIVER_GET_STA_STATISTICS_PARAMS)) {
        DBGLOG(QM, ERROR, "len [%d] is invalid!\n", len);
        return -EINVAL;
    }

    if (data && len) {
        prParams = (P_NL80211_DRIVER_GET_STA_STATISTICS_PARAMS)data;
    }

    /*if (!prParams->aucMacAddr) {
        DBGLOG(QM, ERROR, "%s MAC Address is NULL\n", __func__);
        return -EINVAL;
    }*/

    skb = cfg80211_testmode_alloc_reply_skb(
        wiphy, sizeof(PARAM_GET_STA_STATISTICS) + 1);

    if (!skb) {
        DBGLOG(QM, ERROR, "%s allocate skb failed:%lx\n", __func__, rStatus);
        return -ENOMEM;
    }

    kalMemZero(&rQueryStaStatistics, sizeof(rQueryStaStatistics));
    COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr, prParams->aucMacAddr);
    rQueryStaStatistics.ucReadClear = true;

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
                       &rQueryStaStatistics, sizeof(rQueryStaStatistics), true,
                       false, true, &u4BufLen);

    /* Calcute Link Score */
    u4TxExceedThresholdCount = rQueryStaStatistics.u4TxExceedThresholdCount;
    u4TxTotalCount = rQueryStaStatistics.u4TxTotalCount;
    u4TotalError = rQueryStaStatistics.u4TxFailCount +
                   rQueryStaStatistics.u4TxLifeTimeoutCount;

    /* u4LinkScore 10~100 , ExceedThreshold ratio 0~90 only */
    /* u4LinkScore 0~9    , Drop packet ratio 0~9 and all packets exceed
     * threshold */
    if (u4TxTotalCount) {
        if (u4TxExceedThresholdCount <= u4TxTotalCount) {
            u4LinkScore =
                (90 - ((u4TxExceedThresholdCount * 90) / u4TxTotalCount));
        } else {
            u4LinkScore = 0;
        }
    } else {
        u4LinkScore = 90;
    }

    u4LinkScore += 10;

    if (u4LinkScore == 10) {
        if (u4TotalError <= u4TxTotalCount) {
            u4LinkScore = (10 - ((u4TotalError * 10) / u4TxTotalCount));
        } else {
            u4LinkScore = 0;
        }
    }

    if (u4LinkScore > 100) {
        u4LinkScore = 100;
    }

    {
        u8 __tmp = 0;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_INVALID,
                             sizeof(u8), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }
    {
        u8 __tmp = NL80211_DRIVER_TESTMODE_VERSION;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_VERSION,
                             sizeof(u8), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }
    if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_MAC, MAC_ADDR_LEN,
                         prParams->aucMacAddr) < 0)) {
        goto nla_put_failure;
    }
    {
        u8 __tmp = u4LinkScore;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_LINK_SCORE,
                             sizeof(u8), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }
    if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_MAC, MAC_ADDR_LEN,
                         prParams->aucMacAddr) < 0)) {
        goto nla_put_failure;
    }
    {
        u32 __tmp = rQueryStaStatistics.u4Flag;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_FLAG,
                             sizeof(u32), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }

    /* FW part STA link status */
    {
        u8 __tmp = rQueryStaStatistics.ucPer;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_PER,
                             sizeof(u8), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }
    {
        u8 __tmp = rQueryStaStatistics.ucRcpi;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_RSSI,
                             sizeof(u8), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }
    {
        u32 __tmp = rQueryStaStatistics.u4PhyMode;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_PHY_MODE,
                             sizeof(u32), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }
    {
        u16 __tmp = rQueryStaStatistics.u2LinkSpeed;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_TX_RATE,
                             sizeof(u16), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }
    {
        u32 __tmp = rQueryStaStatistics.u4TxFailCount;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_FAIL_CNT,
                             sizeof(u32), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }
    {
        u32 __tmp = rQueryStaStatistics.u4TxLifeTimeoutCount;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_TIMEOUT_CNT,
                             sizeof(u32), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }
    {
        u32 __tmp = rQueryStaStatistics.u4TxAverageAirTime;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_AVG_AIR_TIME,
                             sizeof(u32), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }

    /* Driver part link status */
    {
        u32 __tmp = rQueryStaStatistics.u4TxTotalCount;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_TOTAL_CNT,
                             sizeof(u32), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }
    {
        u32 __tmp = rQueryStaStatistics.u4TxExceedThresholdCount;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_THRESHOLD_CNT,
                             sizeof(u32), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }
    {
        u32 __tmp = rQueryStaStatistics.u4TxAverageProcessTime;

        if (unlikely(nla_put(skb,
                             NL80211_TESTMODE_STA_STATISTICS_AVG_PROCESS_TIME,
                             sizeof(u32), &__tmp) < 0)) {
            goto nla_put_failure;
        }
    }

    /* Network counter */
    if (unlikely(nla_put(skb,
                         NL80211_TESTMODE_STA_STATISTICS_TC_EMPTY_CNT_ARRAY,
                         sizeof(rQueryStaStatistics.au4TcResourceEmptyCount),
                         rQueryStaStatistics.au4TcResourceEmptyCount) < 0)) {
        goto nla_put_failure;
    }

    /* Sta queue length */
    if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_TC_QUE_LEN_ARRAY,
                         sizeof(rQueryStaStatistics.au4TcQueLen),
                         rQueryStaStatistics.au4TcQueLen) < 0)) {
        goto nla_put_failure;
    }

    /* Global QM counter */
    if (unlikely(nla_put(skb,
                         NL80211_TESTMODE_STA_STATISTICS_TC_AVG_QUE_LEN_ARRAY,
                         sizeof(rQueryStaStatistics.au4TcAverageQueLen),
                         rQueryStaStatistics.au4TcAverageQueLen) < 0)) {
        goto nla_put_failure;
    }

    if (unlikely(nla_put(skb,
                         NL80211_TESTMODE_STA_STATISTICS_TC_CUR_QUE_LEN_ARRAY,
                         sizeof(rQueryStaStatistics.au4TcCurrentQueLen),
                         rQueryStaStatistics.au4TcCurrentQueLen) < 0)) {
        goto nla_put_failure;
    }

    /* Reserved field */
    if (unlikely(nla_put(skb, NL80211_TESTMODE_STA_STATISTICS_RESERVED_ARRAY,
                         sizeof(rQueryStaStatistics.au4Reserved),
                         rQueryStaStatistics.au4Reserved) < 0)) {
        goto nla_put_failure;
    }

    return cfg80211_testmode_reply(skb);

nla_put_failure:
    /* nal_put_skb_fail */
    kfree_skb(skb);
    return -EFAULT;
}

int mtk_cfg80211_testmode_sw_cmd(IN struct wiphy *wiphy, IN void *data,
                                 IN int len)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_NL80211_DRIVER_SW_CMD_PARAMS prParams =
        (P_NL80211_DRIVER_SW_CMD_PARAMS)NULL;
    WLAN_STATUS rstatus = WLAN_STATUS_SUCCESS;
    int fgIsValid = 0;
    u32 u4SetInfoLen = 0;

    ASSERT(wiphy);

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    DBGLOG(INIT, INFO, "--> %s()\n", __func__);

    if (len < sizeof(struct _NL80211_DRIVER_SW_CMD_PARAMS)) {
        DBGLOG(REQ, ERROR, "len [%d] is invalid!\n", len);
        return -EINVAL;
    }
    if (data && len) {
        prParams = (P_NL80211_DRIVER_SW_CMD_PARAMS)data;
    }

    if (prParams) {
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

static int mtk_wlan_cfg_testmode_cmd(struct wiphy *wiphy, void *data, int len)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_NL80211_DRIVER_TEST_MODE_PARAMS prParams = NULL;
    s32 i4Status;

    ASSERT(wiphy);
    DBGLOG(INIT, INFO, "-->%s()\n", __func__);

    if (len < sizeof(struct _NL80211_DRIVER_TEST_MODE_PARAMS)) {
        DBGLOG(REQ, ERROR, "len [%d] is invalid!\n", len);
        return -EINVAL;
    }
    if (!data || !len) {
        DBGLOG(REQ, ERROR, "mtk_cfg80211_testmode_cmd null data\n");
        return -EINVAL;
    }

    if (!wiphy) {
        DBGLOG(REQ, ERROR, "mtk_cfg80211_testmode_cmd null wiphy\n");
        return -EINVAL;
    }

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    prParams = (P_NL80211_DRIVER_TEST_MODE_PARAMS)data;

    /* Clear the version byte */
    prParams->index = prParams->index & ~BITS(24, 31);

    switch (prParams->index) {
    case TESTMODE_CMD_ID_SW_CMD:  /* SW cmd */
        i4Status = mtk_cfg80211_testmode_sw_cmd(wiphy, data, len);
        break;

    case 0x10:
        i4Status = mtk_cfg80211_testmode_get_sta_statistics(wiphy, data, len,
                                                            prGlueInfo);
        break;

    case TESTMODE_CMD_ID_STR_CMD:
        i4Status = mtk_cfg80211_process_str_cmd(
            prGlueInfo, (u8 *)(prParams + 1), len - sizeof(*prParams));
        break;

    default:
        i4Status = -EINVAL;
        break;
    }

    if (i4Status != 0) {
        DBGLOG(REQ, TRACE, "prParams->index=%d, status=%d\n", prParams->index,
               i4Status);
    }

    return i4Status;
}

int mtk_cfg80211_testmode_cmd(struct wiphy *wiphy, struct wireless_dev *wdev,
                              void *data, int len)
{
    ASSERT(wdev);
    return mtk_wlan_cfg_testmode_cmd(wiphy, data, len);
}
#endif

int mtk_cfg80211_sched_scan_start(IN struct wiphy *wiphy,
                                  IN struct net_device *ndev,
                                  IN struct cfg80211_sched_scan_request *request)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    u32 i, u4BufLen;
    P_PARAM_SCHED_SCAN_REQUEST prSchedScanRequest;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    /* check if there is any pending scan/sched_scan not yet finished */
    if (prGlueInfo->prScanRequest != NULL ||
        prGlueInfo->prSchedScanRequest != NULL) {
        DBGLOG(SCN, INFO,
               "(prGlueInfo->prScanRequest != NULL || "
               "prGlueInfo->prSchedScanRequest != NULL)\n");
        return -EBUSY;
    } else if (request == NULL ||
               request->n_match_sets > CFG_SCAN_SSID_MATCH_MAX_NUM) {
        DBGLOG(SCN, INFO,
               "(request == NULL || request->n_match_sets > "
               "CFG_SCAN_SSID_MATCH_MAX_NUM)\n");
        /* invalid scheduled scan request */
        return -EINVAL;
    } else if (!request->n_ssids || !request->n_match_sets) {
        /* invalid scheduled scan request */
        return -EINVAL;
    }

    prSchedScanRequest = (P_PARAM_SCHED_SCAN_REQUEST)kalMemAlloc(
        sizeof(PARAM_SCHED_SCAN_REQUEST), VIR_MEM_TYPE);
    if (prSchedScanRequest == NULL) {
        DBGLOG(SCN, INFO, "(prSchedScanRequest == NULL) kalMemAlloc fail\n");
        return -ENOMEM;
    }

    prSchedScanRequest->u4SsidNum = request->n_match_sets;
    for (i = 0; i < request->n_match_sets; i++) {
        if (request->match_sets ==
            NULL  /* || &(request->match_sets[i]) == NULL >> this comparation are always false(?) */)
        {
            prSchedScanRequest->arSsid[i].u4SsidLen = 0;
        } else {
            COPY_SSID(prSchedScanRequest->arSsid[i].aucSsid,
                      prSchedScanRequest->arSsid[i].u4SsidLen,
                      request->match_sets[i].ssid.ssid,
                      request->match_sets[i].ssid.ssid_len);
        }
    }

    prSchedScanRequest->u4IELength = request->ie_len;
    if (request->ie_len > 0) {
        prSchedScanRequest->pucIE = (u8 *)(request->ie);
    }

    prSchedScanRequest->u2ScanInterval = (u16)(request->scan_plans->interval);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetStartSchedScan, prSchedScanRequest,
                       sizeof(PARAM_SCHED_SCAN_REQUEST), false, false, true,
                       &u4BufLen);

    kalMemFree(prSchedScanRequest, VIR_MEM_TYPE,
               sizeof(PARAM_SCHED_SCAN_REQUEST));

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "scheduled scan error:%lx\n", rStatus);
        return -EINVAL;
    }

    prGlueInfo->prSchedScanRequest = request;

    return 0;
}

int mtk_cfg80211_sched_scan_stop(IN struct wiphy *wiphy,
                                 IN struct net_device *ndev, IN u64 reqid)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    u32 u4BufLen;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    /* check if there is any pending scan/sched_scan not yet finished */
    if (prGlueInfo->prSchedScanRequest == NULL) {
        return -EBUSY;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSetStopSchedScan, NULL, 0, false,
                       false, true, &u4BufLen);

    if (rStatus == WLAN_STATUS_FAILURE) {
        DBGLOG(REQ, WARN, "scheduled scan error:%lx\n", rStatus);
        return -EINVAL;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for handling association request
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_assoc(struct wiphy *wiphy, struct net_device *ndev,
                       struct cfg80211_assoc_request *req)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 arBssid[PARAM_MAC_ADDR_LEN];
    u32 rStatus;
    u32 u4BufLen;
    ENUM_PARAM_ENCRYPTION_STATUS_T eEncStatus;
    ENUM_PARAM_AUTH_MODE_T eAuthMode;
    u32 cipher;
    u32 i, u4AkmSuite;
    P_DOT11_RSNA_CONFIG_AUTHENTICATION_SUITES_ENTRY prEntry;
    P_CONNECTION_SETTINGS_T prConnSettings = NULL;
    u8 *prDesiredIE = NULL;
    u8 *pucIEStart = NULL;
    RSN_INFO_T rRsnInfo;
    u8 fgCarryRsnxe = false;
    P_STA_RECORD_T prStaRec = NULL;
    u8 fgCarryWPSIE = false;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    kalMemZero(arBssid, MAC_ADDR_LEN);
    wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQueryBssid, &arBssid[0],
                         sizeof(arBssid), &u4BufLen);

    prConnSettings = &prGlueInfo->prAdapter->rWifiVar.rConnSettings;

    /* [todo]temp use for indicate rx assoc resp, may need to be modified */

    /* The BSS from cfg80211_ops.assoc must give back to
     * cfg80211_send_rx_assoc() or to cfg80211_assoc_timeout().
     * To ensure proper refcounting, new association requests
     * while already associating must be rejected.
     */
    if (prConnSettings->bss) {
        DBGLOG(REQ, WARN, "Still referencing to another cfg80211_bss.\n");
        DBGLOG(REQ, WARN,
               "Previous auth/assoc handshake not terminate correctly\n");
        return -ENOENT;
    }

    cfg80211_ref_bss(wiphy, req->bss);
    prConnSettings->bss = req->bss;

    DBGLOG(REQ, INFO, "mtk_cfg80211_assoc, media state:%d\n",
           prGlueInfo->eParamMediaStateIndicated);

    kalMemZero(arBssid, MAC_ADDR_LEN);
    if (prGlueInfo->eParamMediaStateIndicated == PARAM_MEDIA_STATE_CONNECTED) {
        wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQueryBssid,
                             &arBssid[0], sizeof(arBssid), &u4BufLen);

#if !CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
        /* 1. check BSSID */
        if (UNEQUAL_MAC_ADDR(arBssid, req->bss->bssid)) {
            /* wrong MAC address */
            DBGLOG(REQ, WARN,
                   "incorrect BSSID: [" MACSTR
                   "] currently connected BSSID[" MACSTR "]\n",
                   MAC2STR(req->bss->bssid), MAC2STR(arBssid));
            return -ENOENT;
        }
#endif
    }

    /* <1> Reset WPA info */
    prGlueInfo->rWpaInfo.u4WpaVersion = IW_AUTH_WPA_VERSION_DISABLED;
    prGlueInfo->rWpaInfo.u4KeyMgmt = 0;
    prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo.u4CipherGroupMgmt = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_DISABLED;
    prGlueInfo->rWpaInfo.ucRSNMfpCap = RSN_AUTH_MFP_DISABLED;
    prGlueInfo->rWpaInfo.ucRSNMfpCap = RSN_AUTH_MFP_DISABLED;

    /* 2.Fill WPA version */
    if (req->crypto.wpa_versions & NL80211_WPA_VERSION_1) {
        prGlueInfo->rWpaInfo.u4WpaVersion = IW_AUTH_WPA_VERSION_WPA;
    } else if (req->crypto.wpa_versions & NL80211_WPA_VERSION_2) {
        prGlueInfo->rWpaInfo.u4WpaVersion = IW_AUTH_WPA_VERSION_WPA2;
    } else {
        prGlueInfo->rWpaInfo.u4WpaVersion = IW_AUTH_WPA_VERSION_DISABLED;
    }
    DBGLOG(REQ, INFO, "wpa ver=%d\n", prGlueInfo->rWpaInfo.u4WpaVersion);

    /* 3.Fill pairwise cipher suite */
    if (req->crypto.n_ciphers_pairwise) {
        DBGLOG(RSN, INFO, "[wlan] cipher pairwise (%x)\n",
               req->crypto.ciphers_pairwise[0]);

        prGlueInfo->prAdapter->rWifiVar.rConnSettings.rRsnInfo
        .au4PairwiseKeyCipherSuite[0] = req->crypto.ciphers_pairwise[0];
        switch (req->crypto.ciphers_pairwise[0]) {
        case WLAN_CIPHER_SUITE_WEP40:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_WEP40;
            break;

        case WLAN_CIPHER_SUITE_WEP104:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_WEP104;
            break;

        case WLAN_CIPHER_SUITE_TKIP:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_TKIP;
            break;

        case WLAN_CIPHER_SUITE_CCMP:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_CCMP;
            break;

        case WLAN_CIPHER_SUITE_AES_CMAC:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_CCMP;
            break;

        case WLAN_CIPHER_SUITE_BIP_GMAC_256:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_GCMP256;
            break;

        case WLAN_CIPHER_SUITE_GCMP_256:
            prGlueInfo->rWpaInfo.u4CipherPairwise = IW_AUTH_CIPHER_GCMP256;
            break;

        default:
            DBGLOG(REQ, WARN, "invalid cipher pairwise (%d)\n",
                   req->crypto.ciphers_pairwise[0]);
            return -EINVAL;
        }
    }

    /* 4. Fill group cipher suite */
    if (req->crypto.cipher_group) {
        DBGLOG(RSN, INFO, "[wlan] cipher group (%x)\n",
               req->crypto.cipher_group);
        prGlueInfo->prAdapter->rWifiVar.rConnSettings.rRsnInfo
        .u4GroupKeyCipherSuite = req->crypto.cipher_group;
        switch (req->crypto.cipher_group) {
        case WLAN_CIPHER_SUITE_WEP40:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_WEP40;
            break;

        case WLAN_CIPHER_SUITE_WEP104:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_WEP104;
            break;

        case WLAN_CIPHER_SUITE_TKIP:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_TKIP;
            break;

        case WLAN_CIPHER_SUITE_CCMP:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_CCMP;
            break;

        case WLAN_CIPHER_SUITE_AES_CMAC:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_CCMP;
            break;

        case WLAN_CIPHER_SUITE_BIP_GMAC_256:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_GCMP256;
            break;

        case WLAN_CIPHER_SUITE_GCMP_256:
            prGlueInfo->rWpaInfo.u4CipherGroup = IW_AUTH_CIPHER_GCMP256;
            break;

        case WLAN_CIPHER_SUITE_NO_GROUP_ADDR:
            break;

        default:
            DBGLOG(REQ, WARN, "invalid cipher group (%d)\n",
                   req->crypto.cipher_group);
            return -EINVAL;
        }
    }

    /* 5. Fill encryption status */
    cipher = prGlueInfo->rWpaInfo.u4CipherGroup |
             prGlueInfo->rWpaInfo.u4CipherPairwise;
    if (1  /* prGlueInfo->rWpaInfo.fgPrivacyInvoke */) {
        if (cipher & IW_AUTH_CIPHER_CCMP) {
            eEncStatus = ENUM_ENCRYPTION3_ENABLED;
#if CFG_SUPPORT_SUITB
        } else if (cipher & IW_AUTH_CIPHER_GCMP256) {
            eEncStatus = ENUM_ENCRYPTION4_ENABLED;
#endif
        } else if (cipher & IW_AUTH_CIPHER_TKIP) {
            eEncStatus = ENUM_ENCRYPTION2_ENABLED;
        } else if (cipher & (IW_AUTH_CIPHER_WEP104 | IW_AUTH_CIPHER_WEP40)) {
            eEncStatus = ENUM_ENCRYPTION1_ENABLED;
        } else if (cipher & IW_AUTH_CIPHER_NONE) {
            if (prGlueInfo->rWpaInfo.fgPrivacyInvoke) {
                eEncStatus = ENUM_ENCRYPTION1_ENABLED;
            } else {
                eEncStatus = ENUM_ENCRYPTION_DISABLED;
            }
        } else {
            eEncStatus = ENUM_ENCRYPTION_DISABLED;
        }
    } else {
        eEncStatus = ENUM_ENCRYPTION_DISABLED;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSetEncryptionStatus, &eEncStatus,
                       sizeof(eEncStatus), false, false, false, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "set encryption mode error:%x\n", rStatus);
    }

    /* 6. Fill AKM suites */
    u4AkmSuite = 0;
    eAuthMode = 0;
    DBGLOG(REQ, INFO, "request numbers of Akm Suite:%d\n",
           req->crypto.n_akm_suites);
    for (i = 0; i < req->crypto.n_akm_suites; i++)
        DBGLOG(REQ, INFO, "request Akm Suite[%d]:%d\n", i,
               req->crypto.akm_suites[i]);

    if (req->crypto.n_akm_suites) {
        prGlueInfo->prAdapter->rWifiVar.rConnSettings.rRsnInfo
        .au4AuthKeyMgtSuite[0] = req->crypto.akm_suites[0];
        DBGLOG(REQ, INFO, "Akm Suite:%d\n", req->crypto.akm_suites[0]);

        if (prGlueInfo->rWpaInfo.u4WpaVersion == IW_AUTH_WPA_VERSION_WPA) {
            switch (req->crypto.akm_suites[0]) {
            case WLAN_AKM_SUITE_8021X:
                eAuthMode = AUTH_MODE_WPA;
                u4AkmSuite = WPA_AKM_SUITE_802_1X;
                break;

            case WLAN_AKM_SUITE_PSK:
                eAuthMode = AUTH_MODE_WPA_PSK;
                u4AkmSuite = WPA_AKM_SUITE_PSK;
                break;

            default:
                DBGLOG(REQ, WARN, "invalid Akm Suite (%08x)\n",
                       req->crypto.akm_suites[0]);
                return -EINVAL;
            }
        } else if (prGlueInfo->rWpaInfo.u4WpaVersion ==
                   IW_AUTH_WPA_VERSION_WPA2) {
            switch (req->crypto.akm_suites[0]) {
            case WLAN_AKM_SUITE_8021X:
                eAuthMode = AUTH_MODE_WPA2;
                u4AkmSuite = RSN_AKM_SUITE_802_1X;
                break;

            case WLAN_AKM_SUITE_PSK:
                eAuthMode = AUTH_MODE_WPA2_PSK;
                u4AkmSuite = RSN_AKM_SUITE_PSK;
                break;

#if CFG_SUPPORT_802_11W
            /* Notice:: Need kernel patch!! */
            case WLAN_AKM_SUITE_8021X_SHA256:
                eAuthMode = AUTH_MODE_WPA2;
                u4AkmSuite = RSN_AKM_SUITE_802_1X_SHA256;
                break;

            case WLAN_AKM_SUITE_PSK_SHA256:
                eAuthMode = AUTH_MODE_WPA2_PSK;
                u4AkmSuite = RSN_AKM_SUITE_PSK_SHA256;
                break;

#endif
            case WLAN_AKM_SUITE_8021X_SUITE_B:
                eAuthMode = AUTH_MODE_WPA2_PSK;
                u4AkmSuite = RSN_AKM_SUITE_8021X_SUITE_B_192;
                break;

            case WLAN_AKM_SUITE_8021X_SUITE_B_192:
                eAuthMode = AUTH_MODE_WPA2_PSK;
                u4AkmSuite = RSN_AKM_SUITE_8021X_SUITE_B_192;
                break;

#if CFG_SUPPORT_SAE
            /* Need to add in WPA also? */
            case WLAN_AKM_SUITE_SAE:
                eAuthMode = AUTH_MODE_WPA2_SAE;
                u4AkmSuite = RSN_AKM_SUITE_SAE;
                break;

#endif
#if CFG_SUPPORT_OWE
            case WLAN_AKM_SUITE_OWE:
                eAuthMode = AUTH_MODE_WPA2_PSK;
                u4AkmSuite = RSN_AKM_SUITE_OWE;
                break;

#endif
            default:
                DBGLOG(REQ, WARN, "invalid Akm Suite (%08x)\n",
                       req->crypto.akm_suites[0]);
                return -EINVAL;
            }
        }
    }
    if (prGlueInfo->rWpaInfo.u4WpaVersion == IW_AUTH_WPA_VERSION_DISABLED) {
        eAuthMode =
            (prGlueInfo->rWpaInfo.u4AuthAlg == IW_AUTH_ALG_OPEN_SYSTEM) ?
            AUTH_MODE_OPEN :
            AUTH_MODE_AUTO_SWITCH;
    }

    DBGLOG(REQ, STATE, "set auth mode:%d, akm suite:0x%x\n", eAuthMode,
           u4AkmSuite);

    /* 6.1 Set auth mode*/
    rStatus = kalIoctl(prGlueInfo, wlanoidSetAuthMode, &eAuthMode,
                       sizeof(eAuthMode), false, false, false, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "set auth mode error:%x\n", rStatus);
    }

    /* 6.2 Enable the specific AKM suite only. */
    for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++) {
        prEntry = &prGlueInfo->prAdapter->rMib
                  .dot11RSNAConfigAuthenticationSuitesTable[i];

        if (prEntry->dot11RSNAConfigAuthenticationSuite == u4AkmSuite) {
            prEntry->dot11RSNAConfigAuthenticationSuiteEnabled = true;
            DBGLOG(REQ, INFO, "match AuthenticationSuite = 0x%x", u4AkmSuite);
        } else {
            prEntry->dot11RSNAConfigAuthenticationSuiteEnabled = false;
        }
    }

    /* 7. Parsing desired ie from upper layer */
    prGlueInfo->fgWpsActive = false;

    if (req->ie && req->ie_len > 0) {
        pucIEStart = (u8 *)req->ie;
        if (wextSrchDesiredWPSIE(pucIEStart, req->ie_len, 0xDD,
                                 (u8 **)&prDesiredIE)) {
            prGlueInfo->fgWpsActive = true;
            fgCarryWPSIE = true;
            rStatus = kalIoctl(prGlueInfo, wlanoidSetWSCAssocInfo, prDesiredIE,
                               IE_SIZE(prDesiredIE), false, false, false,
                               &u4BufLen);
            if (rStatus != WLAN_STATUS_SUCCESS) {
                DBGLOG(SEC, WARN, "[WSC] set WSC assoc info error:%x\n",
                       rStatus);
            }
        }

        if (wextSrchDesiredWPAIE(pucIEStart, req->ie_len, 0x30,
                                 (u8 **)&prDesiredIE)) {
            if (rsnParseRsnIE(prGlueInfo->prAdapter,
                              (P_RSN_INFO_ELEM_T)prDesiredIE, &rRsnInfo)) {
                /* Fill RSNE MFP Cap */
                if (rRsnInfo.u2RsnCap & ELEM_WPA_CAP_MFPC) {
                    prGlueInfo->rWpaInfo.u4CipherGroupMgmt =
                        rRsnInfo.u4GroupMgmtKeyCipherSuite;
                    prGlueInfo->rWpaInfo.ucRSNMfpCap = RSN_AUTH_MFP_OPTIONAL;
                    if (rRsnInfo.u2RsnCap & ELEM_WPA_CAP_MFPR) {
                        prGlueInfo->rWpaInfo.ucRSNMfpCap =
                            RSN_AUTH_MFP_REQUIRED;
                    }
                } else {
                    prGlueInfo->rWpaInfo.ucRSNMfpCap = RSN_AUTH_MFP_DISABLED;
                }

                prGlueInfo->rWpaInfo.ucRsneLen = rRsnInfo.ucRsneLen;

                /* Fill RSNE PMKID Count and List */
                prConnSettings->rRsnInfo.u2PmkidCnt = rRsnInfo.u2PmkidCnt;
                if (rRsnInfo.u2PmkidCnt > 0) {
                    kalMemCopy(prConnSettings->rRsnInfo.aucPmkidList,
                               rRsnInfo.aucPmkidList,
                               (rRsnInfo.u2PmkidCnt * RSN_PMKID_LEN));
                }
            }
        }

        /* Gen OWE IE */
        if (wextSrchDesiredWPAIE(pucIEStart, req->ie_len, 0xff,
                                 (u8 **)&prDesiredIE)) {
            u8 ucLength = (*(prDesiredIE + 1) + 2);

            kalMemCopy(&prGlueInfo->prAdapter->rWifiVar.rConnSettings.rOweInfo,
                       prDesiredIE, ucLength);

            DBGLOG(REQ, INFO, "DUMP OWE INFO, EID %x length %x\n", *prDesiredIE,
                   ucLength);
            DBGLOG_MEM8(REQ, INFO,
                        &prGlueInfo->prAdapter->rWifiVar.rConnSettings.rOweInfo,
                        ucLength);
        } else {
            kalMemSet(&prGlueInfo->prAdapter->rWifiVar.rConnSettings.rOweInfo,
                      0, sizeof(struct OWE_INFO_T));
        }

        /* Gen RSNXE */
        if (wextSrchDesiredWPAIE(pucIEStart, req->ie_len, 0xf4,
                                 (u8 **)&prDesiredIE)) {
            u16 u2Length = (*(prDesiredIE + 1) + 2);

            if (u2Length <= sizeof(prConnSettings->rRsnXE)) {
                kalMemCopy(&prConnSettings->rRsnXE, prDesiredIE, u2Length);
                fgCarryRsnxe = true;

                DBGLOG(REQ, INFO, "DUMP RSNXE, EID %x length %x\n",
                       *prDesiredIE, u2Length);
                DBGLOG_MEM8(REQ, INFO, &prConnSettings->rRsnXE, u2Length);
            } else {
                DBGLOG(RSN, ERROR, "RSNXE length exceeds 2\n");
            }
        }

        if (fgCarryRsnxe == false) {
            kalMemSet(&prConnSettings->rRsnXE, 0, sizeof(struct RSNXE));
        }

        /* clear WSC Assoc IE buffer in case WPS IE is not detected */
        if (fgCarryWPSIE == false) {
            kalMemZero(&prGlueInfo->aucWSCAssocInfoIE, 200);
            prGlueInfo->u2WSCAssocInfoIELen = 0;
        }
    }
    /* Fill WPA info - mfp setting */
    /* Must put after paring RSNE from upper layer
     * for prGlueInfo->rWpaInfo.ucRSNMfpCap assignment
     */

    prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_DISABLED;
    if (req->use_mfp) {
        prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_REQUIRED;
    } else {
        /* Change Mfp parameter from DISABLED to OPTIONAL
         * if upper layer set MFPC = 1 in RSNE
         * since upper layer can't bring MFP OPTIONAL information
         * to driver by sme->mfp
         */
        if (prGlueInfo->rWpaInfo.ucRSNMfpCap == RSN_AUTH_MFP_OPTIONAL) {
            prGlueInfo->rWpaInfo.u4Mfp = IW_AUTH_MFP_OPTIONAL;
        } else if (prGlueInfo->rWpaInfo.ucRSNMfpCap == RSN_AUTH_MFP_REQUIRED) {
            DBGLOG(REQ, WARN,
                   "mfp parameter(DISABLED) conflict with mfp cap(REQUIRED)\n");
        }
    }

    prConnSettings->fgIsSendAssoc = true;
    if (!prConnSettings->fgIsConnInitialized) {
        DBGLOG(REQ, WARN, "Send assoc without connection initialized first\n");
        rStatus = kalIoctl(prGlueInfo, wlanoidSetBssid, (void *)req->bss->bssid,
                           MAC_ADDR_LEN, false, false, true, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(REQ, WARN, "set BSSID:%x\n", rStatus);
            return -EINVAL;
        }
    } else {  /* skip join initial flow when it has been completed*/
        prStaRec = cnmGetStaRecByAddress(
            prGlueInfo->prAdapter,
            prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex, req->bss->bssid);

        if (prStaRec) {
            saaSendAuthAssoc(prGlueInfo->prAdapter, prStaRec);
            DBGLOG(REQ, STATE, "Send assoc.\n");
        } else {
            DBGLOG(REQ, WARN, "can't send assoc since can't find StaRec\n");
        }
    }

    return 0;
}

#if CFG_SUPPORT_NFC_BEAM_PLUS

int mtk_cfg80211_testmode_get_scan_done(IN struct wiphy *wiphy, IN void *data,
                                        IN int len,
                                        IN P_GLUE_INFO_T prGlueInfo)
{
#define NL80211_TESTMODE_P2P_SCANDONE_INVALID 0
#define NL80211_TESTMODE_P2P_SCANDONE_STATUS 1

#ifdef CONFIG_NL80211_TESTMODE
#if !DBG_DISABLE_ALL_LOG
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
#endif
    s32 i4Status = -EINVAL, READY_TO_BEAM = 0;

    struct sk_buff *skb = NULL;

    ASSERT(wiphy);
    ASSERT(prGlueInfo);

    skb = cfg80211_testmode_alloc_reply_skb(wiphy, sizeof(u32));

    /* READY_TO_BEAM = */
    /* (u32)(prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo->rScanReqInfo.fgIsGOInitialDone)
     */
    /* &(!prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo->rScanReqInfo.fgIsScanRequest);
     */
    READY_TO_BEAM = 1;
    /* DBGLOG(QM, TRACE, */
    /* ("NFC:GOInitialDone[%d] and P2PScanning[%d]\n", */
    /* prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo->rScanReqInfo.fgIsGOInitialDone,
     */
    /* prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo->rScanReqInfo.fgIsScanRequest));
     */

    if (!skb) {
        DBGLOG(QM, TRACE, "%s allocate skb failed:%lx\n", __func__, rStatus);
        return -ENOMEM;
    }
    {
        u8 __tmp = 0;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_P2P_SCANDONE_INVALID,
                             sizeof(u8), &__tmp) < 0)) {
            kfree_skb(skb);
            goto nla_put_failure;
        }
    }
    {
        u32 __tmp = READY_TO_BEAM;

        if (unlikely(nla_put(skb, NL80211_TESTMODE_P2P_SCANDONE_STATUS,
                             sizeof(u32), &__tmp) < 0)) {
            kfree_skb(skb);
            goto nla_put_failure;
        }
    }

    i4Status = cfg80211_testmode_reply(skb);

nla_put_failure:
    return i4Status;

#else
    DBGLOG(QM, WARN, "CONFIG_NL80211_TESTMODE not enabled\n");
    return -EINVAL;

#endif
}

#endif

#if CFG_SUPPORT_TDLS

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for changing a station information
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_change_station(struct wiphy *wiphy, struct net_device *ndev,
                                const u8 *mac,
                                struct station_parameters *params)
{
    /* return 0; */

    /* from supplicant -- wpa_supplicant_tdls_peer_addset() */
    P_GLUE_INFO_T prGlueInfo = NULL;
    CMD_PEER_UPDATE_T rCmdUpdate;
    WLAN_STATUS rStatus;
    u32 u4BufLen, u4Temp;
    u8 *u4Rates;
    ADAPTER_T *prAdapter;
    P_BSS_INFO_T prAisBssInfo;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    /* make up command */

    prAdapter = prGlueInfo->prAdapter;
    prAisBssInfo = prAdapter->prAisBssInfo;

    if (params == NULL) {
        return 0;
    }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0))
    u4Rates = params->supported_rates;
    u4Temp = params->supported_rates_len;
#else
    u4Rates = (u8 *) params->link_sta_params.supported_rates;
    u4Temp = params->link_sta_params.supported_rates_len;
#endif

    if (u4Rates == NULL) {
        return 0;
    }

    /* init */
    kalMemZero(&rCmdUpdate, sizeof(rCmdUpdate));
    kalMemCopy(rCmdUpdate.aucPeerMac, mac, 6);

    if (u4Temp > CMD_PEER_UPDATE_SUP_RATE_MAX) {
        u4Temp = CMD_PEER_UPDATE_SUP_RATE_MAX;
    }

    kalMemCopy(rCmdUpdate.aucSupRate, u4Rates, u4Temp);
    rCmdUpdate.u2SupRateLen = u4Temp;

    /*
     * In supplicant, only recognize WLAN_EID_QOS 46, not 0xDD WMM
     * So force to support UAPSD here.
     */
    rCmdUpdate.UapsdBitmap = 0x0F;  /*params->uapsd_queues; */
    rCmdUpdate.UapsdMaxSp = 0;  /*params->max_sp; */

    rCmdUpdate.u2Capability = params->capability;

    if (params->ext_capab != NULL) {
        u4Temp = params->ext_capab_len;
        if (u4Temp > CMD_PEER_UPDATE_EXT_CAP_MAXLEN) {
            u4Temp = CMD_PEER_UPDATE_EXT_CAP_MAXLEN;
        }
        kalMemCopy(rCmdUpdate.aucExtCap, params->ext_capab, u4Temp);
        rCmdUpdate.u2ExtCapLen = u4Temp;
    }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0))
#define HT_CAPA params->ht_capa
#define VHT_CAPA params->vht_capa
#else
#define HT_CAPA params->link_sta_params.ht_capa
#define VHT_CAPA params->link_sta_params.vht_capa
#endif

    if (HT_CAPA != NULL) {
        rCmdUpdate.rHtCap.u2CapInfo = HT_CAPA->cap_info;
        rCmdUpdate.rHtCap.ucAmpduParamsInfo = HT_CAPA->ampdu_params_info;
        rCmdUpdate.rHtCap.u2ExtHtCapInfo = HT_CAPA->extended_ht_cap_info;
        rCmdUpdate.rHtCap.u4TxBfCapInfo = HT_CAPA->tx_BF_cap_info;
        rCmdUpdate.rHtCap.ucAntennaSelInfo = HT_CAPA->antenna_selection_info;
        kalMemCopy(rCmdUpdate.rHtCap.rMCS.arRxMask, HT_CAPA->mcs.rx_mask,
                   sizeof(rCmdUpdate.rHtCap.rMCS.arRxMask));

        rCmdUpdate.rHtCap.rMCS.u2RxHighest = HT_CAPA->mcs.rx_highest;
        rCmdUpdate.rHtCap.rMCS.ucTxParams = HT_CAPA->mcs.tx_params;
        rCmdUpdate.fgIsSupHt = true;
    }
    /* vht */

    if (VHT_CAPA != NULL) {
        rCmdUpdate.rVHtCap.u4CapInfo = VHT_CAPA->vht_cap_info;
        rCmdUpdate.rVHtCap.rVMCS.u2RxMcsMap = VHT_CAPA->supp_mcs.rx_mcs_map;
        rCmdUpdate.rVHtCap.rVMCS.u2RxHighest = VHT_CAPA->supp_mcs.rx_highest;
        rCmdUpdate.rVHtCap.rVMCS.u2TxMcsMap = VHT_CAPA->supp_mcs.tx_mcs_map;
        rCmdUpdate.rVHtCap.rVMCS.u2TxHighest = VHT_CAPA->supp_mcs.tx_highest;
        rCmdUpdate.fgIsSupVht = true;
    }

    /* update a TDLS peer record */
    /* sanity check */
    if ((params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))) {
        rCmdUpdate.eStaType = STA_TYPE_DLS_PEER;
    }
    rStatus = kalIoctl(prGlueInfo, cnmPeerUpdate, &rCmdUpdate,
                       sizeof(CMD_PEER_UPDATE_T), false, false, false,
                       /* false,    //6628 -> 6630  fgIsP2pOid-> x */
                       &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -EINVAL;
    }

    /* for Ch Sw AP prohibit case */
    if (prAisBssInfo->fgTdlsIsChSwProhibited) {
        /* disable TDLS ch sw function */

        rStatus = kalIoctl(prGlueInfo, TdlsSendChSwControlCmd,
                           &TdlsSendChSwControlCmd, sizeof(CMD_TDLS_CH_SW_T),
                           false, false, false,
                           /* false,    //6628 -> 6630  fgIsP2pOid-> x */
                           &u4BufLen);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for adding a station information
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_add_station(struct wiphy *wiphy, struct net_device *ndev,
                             const u8 *mac, struct station_parameters *params)
{
    /* return 0; */

    /* from supplicant -- wpa_supplicant_tdls_peer_addset() */
    P_GLUE_INFO_T prGlueInfo = NULL;
    CMD_PEER_ADD_T rCmdCreate;
    ADAPTER_T *prAdapter;
    WLAN_STATUS rStatus;
    u32 u4BufLen;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    /* make up command */

    prAdapter = prGlueInfo->prAdapter;

    /* init */
    kalMemZero(&rCmdCreate, sizeof(rCmdCreate));
    kalMemCopy(rCmdCreate.aucPeerMac, mac, 6);

    /* create a TDLS peer record */
    if ((params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))) {
        rCmdCreate.eStaType = STA_TYPE_DLS_PEER;
        rStatus = kalIoctl(prGlueInfo, cnmPeerAdd, &rCmdCreate,
                           sizeof(CMD_PEER_ADD_T), false, false, false,
                           /* false,    //6628 -> 6630  fgIsP2pOid-> x
                            */
                           &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -EINVAL;
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for deleting a station information
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 *
 * @other
 * must implement if you have add_station().
 */
/*----------------------------------------------------------------------------*/
static const u8 bcast_addr[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
int mtk_cfg80211_del_station(struct wiphy *wiphy, struct net_device *ndev,
                             struct station_del_parameters *params)
{
    /* fgIsTDLSlinkEnable = 0; */

    /* return 0; */
    /* from supplicant -- wpa_supplicant_tdls_peer_addset() */

    const u8 *mac = params->mac ? params->mac : bcast_addr;
    P_GLUE_INFO_T prGlueInfo = NULL;
    ADAPTER_T *prAdapter;
    STA_RECORD_T *prStaRec;
    u8 deleteMac[MAC_ADDR_LEN];

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    prAdapter = prGlueInfo->prAdapter;

    /* For kernel 3.18 modification, we trasfer to local buff to query sta
     */
    memset(deleteMac, 0, MAC_ADDR_LEN);
    memcpy(deleteMac, mac, MAC_ADDR_LEN);

    prStaRec = cnmGetStaRecByAddress(
        prAdapter, (u8)prAdapter->prAisBssInfo->ucBssIndex, deleteMac);

    if (prStaRec != NULL) {
        cnmStaRecFree(prAdapter, prStaRec);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to transmit a TDLS data frame from nl80211.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in]
 * \param[in]
 * \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_tdls_mgmt(struct wiphy *wiphy, struct net_device *dev,
                           const u8 *peer,
#if (CFG80211_VERSION_CODE >= KERNEL_VERSION(6, 5, 0))
                           int link_id,
#endif
                           u8 action_code, u8 dialog_token, u16 status_code,
                           u32 peer_capability, bool initiator, const u8 *buf,
                           size_t len)
{
    GLUE_INFO_T *prGlueInfo;
    TDLS_CMD_LINK_MGT_T rCmdMgt;
    u32 u4BufLen;

    /* sanity check */
    if ((wiphy == NULL) || (peer == NULL) || (buf == NULL)) {
        return -EINVAL;
    }

    /* init */
    prGlueInfo = (GLUE_INFO_T *)wiphy_priv(wiphy);
    if (prGlueInfo == NULL) {
        return -EINVAL;
    }

    kalMemZero(&rCmdMgt, sizeof(rCmdMgt));

    rCmdMgt.u2StatusCode = status_code;
    rCmdMgt.u4SecBufLen = len;
    rCmdMgt.ucDialogToken = dialog_token;
    rCmdMgt.ucActionCode = action_code;
    kalMemCopy(&(rCmdMgt.aucPeer), peer, 6);

    if (len > TDLS_SEC_BUF_LENGTH) {
        DBGLOG(REQ, WARN, "%s:len > TDLS_SEC_BUF_LENGTH\n", __func__);
        return -EINVAL;
    }

    kalMemCopy(&(rCmdMgt.aucSecBuf), buf, len);

    kalIoctl(prGlueInfo, TdlsexLinkMgt, &rCmdMgt, sizeof(TDLS_CMD_LINK_MGT_T),
             false, false, false,
             /* false,    //6628 -> 6630  fgIsP2pOid-> x */
             &u4BufLen);
    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to hadel TDLS link from nl80211.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in]
 * \param[in]
 * \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_tdls_oper(struct wiphy *wiphy, struct net_device *dev,
                           const u8 *peer, enum nl80211_tdls_operation oper)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u32 u4BufLen;
    ADAPTER_T *prAdapter;
    TDLS_CMD_LINK_OPER_T rCmdOper;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);
    prAdapter = prGlueInfo->prAdapter;

    kalMemZero(&rCmdOper, sizeof(rCmdOper));
    kalMemCopy(rCmdOper.aucPeerMac, peer, 6);

    rCmdOper.oper = (ENUM_TDLS_LINK_OPER) oper;

    kalIoctl(prGlueInfo, TdlsexLinkOper, &rCmdOper,
             sizeof(TDLS_CMD_LINK_OPER_T), false, false, false,
             /* false,    //6628 -> 6630  fgIsP2pOid-> x */
             &u4BufLen);
    return 0;
}

#endif

s32 mtk_cfg80211_process_str_cmd(P_GLUE_INFO_T prGlueInfo, u8 *cmd, s32 len)
{
    u32 rStatus = WLAN_STATUS_SUCCESS;

#if CFG_SUPPORT_802_11K || CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
    u32 u4SetInfoLen = 0;
#endif

#if CFG_SUPPORT_802_11K
    if (strnicmp(cmd, "NEIGHBOR-REQUEST", 16) == 0) {
        u8 *pucSSID = NULL;
        u32 u4SSIDLen = 0;

        if (len > 16 && (strnicmp(cmd + 16, " SSID=", 6) == 0)) {
            pucSSID = cmd + 22;
            u4SSIDLen = len - 22;
            DBGLOG(REQ, ERROR, "cmd=%s, ssid len %u, ssid=%s\n", cmd, u4SSIDLen,
                   pucSSID);
        }
        rStatus = kalIoctl(prGlueInfo, wlanoidSendNeighborRequest,
                           (void *)pucSSID, u4SSIDLen, false, false, true,
                           &u4SetInfoLen);
    } else {
        return -EOPNOTSUPP;
    }
#endif

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
    if (strnicmp(cmd, "BSS-TRANSITION-QUERY", 20) == 0) {
        u8 *pucReason = NULL;

        if (len > 20 && (strnicmp(cmd + 20, " reason=", 8) == 0)) {
            pucReason = cmd + 28;
        }
        rStatus = kalIoctl(prGlueInfo, wlanoidSendBTMQuery, (void *)pucReason,
                           1, false, false, true, &u4SetInfoLen);
    } else {
        return -EOPNOTSUPP;
    }
#endif

    if (rStatus == WLAN_STATUS_SUCCESS) {
        return 0;
    }

    return -EINVAL;
}

#if !(CFG_BUILT_IN_DRIVER)
bool is_world_regdom(char *alpha2)
{
    if (!alpha2) {
        return false;
    }

    return (alpha2[0] == '0') && (alpha2[1] == '0');
}
#endif

enum regd_state regd_state_machine(IN struct regulatory_request *pRequest)
{
    switch (pRequest->initiator) {
    case NL80211_REGDOM_SET_BY_USER:
        DBGLOG(RLM, INFO, "regd_state_machine: SET_BY_USER\n");

        return rlmDomainStateTransition(REGD_STATE_SET_COUNTRY_USER, pRequest);

    case NL80211_REGDOM_SET_BY_DRIVER:
        DBGLOG(RLM, INFO, "regd_state_machine: SET_BY_DRIVER\n");

        return rlmDomainStateTransition(REGD_STATE_SET_COUNTRY_DRIVER,
                                        pRequest);

    case NL80211_REGDOM_SET_BY_CORE:
        DBGLOG(RLM, INFO, "regd_state_machine: SET_BY_CORE\n");

        return rlmDomainStateTransition(REGD_STATE_SET_WW_CORE, pRequest);

    case NL80211_REGDOM_SET_BY_COUNTRY_IE:
        DBGLOG(RLM, WARN, "regd_state_machine: SET_BY_COUNTRY_IE\n");

	/* Received Country Information Element in a beacon frame. */
        return rlmDomainStateTransition(REGD_STATE_SET_COUNTRY_IE, pRequest);

    default:
        return rlmDomainStateTransition(REGD_STATE_INVALID, pRequest);
    }
}

void mtk_apply_custom_regulatory(IN struct wiphy *pWiphy,
                                 IN const struct ieee80211_regdomain *pRegdom)
{
    u32 band_idx, ch_idx;
    struct ieee80211_supported_band *sband;
    struct ieee80211_channel *chan;

    DBGLOG(RLM, INFO, "%s()\n", __func__);

    /* to reset cha->flags*/
    for (band_idx = 0; band_idx < NUM_NL80211_BANDS; band_idx++) {
        sband = pWiphy->bands[band_idx];
        if (!sband) {
            continue;
        }

        for (ch_idx = 0; ch_idx < sband->n_channels; ch_idx++) {
            chan = &sband->channels[ch_idx];

            /*reset chan->flags*/
            chan->flags = 0;
        }
    }

    /* update to kernel */
    wiphy_apply_custom_regulatory(pWiphy, pRegdom);
}

void mtk_reg_notify(IN struct wiphy *pWiphy,
                    IN struct regulatory_request *pRequest)
{
    P_GLUE_INFO_T prGlueInfo;
    P_ADAPTER_T prAdapter;
    enum regd_state old_state;

    if (!pWiphy) {
        DBGLOG(RLM, ERROR, "%s(): pWiphy = NULL.\n", __func__);
        return;
    }

    if (g_u4HaltFlag) {
        DBGLOG(RLM, WARN, "wlan is halt, skip reg callback\n");
        return;
    }

    /*
     * Magic flow for driver to send inband command after kernel's calling
     * reg_notifier callback
     */
    if (!pRequest) {
        /*triggered by our driver in wlan initial process.*/

        if (rlmDomainIsCtrlStateEqualTo(REGD_STATE_INIT)) {
            if (rlmDomainIsUsingLocalRegDomainDataBase()) {
                DBGLOG(RLM, WARN, "County Code is not assigned. Use default WW.\n");
                goto DOMAIN_SEND_CMD;
            } else {
                DBGLOG(RLM, ERROR, "Invalid REG state happened. state = 0x%x\n",
                       rlmDomainGetCtrlState());
                return;
            }
        } else if ((rlmDomainIsCtrlStateEqualTo(REGD_STATE_SET_WW_CORE)) ||
                   (rlmDomainIsCtrlStateEqualTo(REGD_STATE_SET_COUNTRY_USER)) ||
                   (rlmDomainIsCtrlStateEqualTo(
                        REGD_STATE_SET_COUNTRY_DRIVER))) {
            goto DOMAIN_SEND_CMD;
        } else {
            DBGLOG(RLM, ERROR, "Invalid REG state happened. state = 0x%x\n",
                   rlmDomainGetCtrlState());
            return;
        }
    }

    /*
     * Ignore the CORE's WW setting when using local data base of regulatory
     * rules
     */
    if ((pRequest->initiator == NL80211_REGDOM_SET_BY_CORE) &&
        (pWiphy->regulatory_flags & REGULATORY_CUSTOM_REG)) {
        return;  /*Ignore the CORE's WW setting*/
    }
    /*
     * State machine transition
     */
    DBGLOG(RLM, INFO, "request->alpha2=%s, initiator=%x, intersect=%d\n",
           pRequest->alpha2, pRequest->initiator, pRequest->intersect);

    old_state = rlmDomainGetCtrlState();
    regd_state_machine(pRequest);

    if (rlmDomainGetCtrlState() == old_state) {
        if ((old_state == REGD_STATE_SET_COUNTRY_USER) &&
            (!(rlmDomainIsSameCountryCode(pRequest->alpha2,
                                          sizeof(pRequest->alpha2))))) {
            DBGLOG(RLM, INFO, "Set by user to NEW country code\n");
        } else if (old_state != REGD_STATE_SET_COUNTRY_DRIVER) {
            /* Change to same state or same country, ignore */
            return;
        }
    } else if (rlmDomainIsCtrlStateEqualTo(REGD_STATE_INVALID)) {
        DBGLOG(RLM, ERROR, "\n%s():\n---> WARNING. Transit to invalid state.\n",
               __func__);
        DBGLOG(RLM, ERROR, "---> WARNING.\n ");
        rlmDomainAssert(0);
    }

    /*
     * Set country code
     */
    if (pRequest->initiator != NL80211_REGDOM_SET_BY_DRIVER) {
        rlmDomainSetCountryCode(pRequest->alpha2, sizeof(pRequest->alpha2));
    } else {
        /*SET_BY_DRIVER*/

        if (rlmDomainIsEfuseUsed()) {
            if (!rlmDomainIsUsingLocalRegDomainDataBase()) {
                DBGLOG(
                    RLM, WARN,
                    "[WARNING!!!] Local DB must be used if country code from efuse.\n");
            }
        } else {
            /* iwpriv case */
            if (rlmDomainIsUsingLocalRegDomainDataBase()) {
                /*iwpriv set country but local data base*/
                u32 country_code = rlmDomainGetTempCountryCode();

                rlmDomainSetCountryCode((char *)&country_code,
                                        sizeof(country_code));
            } else {
                /*iwpriv set country but query CRDA*/
                rlmDomainSetCountryCode(pRequest->alpha2,
                                        sizeof(pRequest->alpha2));
            }
        }
    }

    rlmDomainSetDfsRegion(pRequest->dfs_region);

DOMAIN_SEND_CMD:
    DBGLOG(RLM, INFO, "g_mtk_regd_control.alpha2 = 0x%x\n",
           rlmDomainGetCountryCode());

    /*
     * Check if using customized regulatory rule
     */
    if (rlmDomainIsUsingLocalRegDomainDataBase() &&
		   rlmDomainIsEfuseUsed()) {
        const struct ieee80211_regdomain *pRegdom;
        u32 country_code = rlmDomainGetCountryCode();
        char alpha2[4];

        /*fetch regulatory rules from local data base*/
        alpha2[0] = country_code & 0xFF;
        alpha2[1] = (country_code >> 8) & 0xFF;
        alpha2[2] = (country_code >> 16) & 0xFF;
        alpha2[3] = (country_code >> 24) & 0xFF;

        pRegdom = rlmDomainSearchRegdomainFromLocalDataBase(alpha2);
        if (!pRegdom) {
            DBGLOG(
                RLM, INFO,
                "%s(): Error, Cannot find the correct RegDomain. country = %s\n",
                __func__, rlmDomainGetCountryCode());

            rlmDomainAssert(0);
            return;
        }

        mtk_apply_custom_regulatory(pWiphy, pRegdom);
    }

    /*
     * Always use the wlan GlueInfo as parameter.
     */
    prGlueInfo = rlmDomainGetGlueInfo();
    if (!prGlueInfo) {
        DBGLOG(RLM, INFO, "prGlueInfo is NULL!\n");
        return;  /*interface is not up yet.*/
    }
    prAdapter = prGlueInfo->prAdapter;
    if (!prAdapter) {
        DBGLOG(RLM, INFO, "prAdapter is NULL!\n");
        return;  /*interface is not up yet.*/
    }
    /*
     * Awlays use wlan0's base wiphy pointer to update reg notifier.
     * Because only one reg state machine is handled.
     */
    if (pWiphy != priv_to_wiphy(prAdapter->prGlueInfo)) {
        pWiphy = priv_to_wiphy(prAdapter->prGlueInfo);
        DBGLOG(RLM, INFO, "Use base wiphy to update (p=0x%x)\n",
               priv_to_wiphy(prAdapter->prGlueInfo));
    }

    /*
     * Parsing channels
     */
    rlmDomainParsingChannel(pWiphy);  /*real regd update*/

    /*
     * Check if firmawre support single sku
     */
    if (!regd_is_single_sku_en()) {
        return;  /*no need to send information to firmware due to
                  * firmware is not supported*/
    }
    /*
     * Send commands to firmware
     */
    prAdapter->rWifiVar.rConnSettings.u2CountryCode =
        (u16)rlmDomainGetCountryCode();
    rlmDomainSendCmd(prAdapter, false);
}

void cfg80211_regd_set_wiphy(IN struct wiphy *prWiphy)
{
    /*
     * register callback
     */
    prWiphy->reg_notifier = mtk_reg_notify;

    /*
     * clear REGULATORY_CUSTOM_REG flag
     */
    prWiphy->regulatory_flags &= ~(REGULATORY_CUSTOM_REG);

    /*ignore the hint from IE*/
    prWiphy->regulatory_flags |= REGULATORY_COUNTRY_IE_IGNORE;

#ifdef CFG_SUPPORT_DISABLE_BCN_HINTS
    prWiphy->regulatory_flags |= REGULATORY_DISABLE_BEACON_HINTS;
#endif

    /*
     * set REGULATORY_CUSTOM_REG flag
     */
#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
    prWiphy->regulatory_flags |= (REGULATORY_CUSTOM_REG);

    /* assigned a defautl one */
    if (rlmDomainGetLocalDefaultRegd()) {
        wiphy_apply_custom_regulatory(prWiphy, rlmDomainGetLocalDefaultRegd());
    }
#endif

    /*
     * Initialize regd control information
     */
    rlmDomainResetCtrlInfo(false);
}

/*
 * Use these commands to control the enable and disable of WoWLAN.
 * iw phy0 wowlan disable
 * iw phy0 wowlan enable any
 */
void mtk_cfg80211_set_wakeup(struct wiphy *wiphy, bool enabled)
{
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    DBGLOG(REQ, WARN, "%s\n", enabled ? "enabled" : "disalbed");

    if (prGlueInfo && prGlueInfo->prAdapter) {
        prGlueInfo->prAdapter->rWowCtrl.fgWowEnable = enabled;
    } else {
        DBGLOG(REQ, WARN, "not applied (%p, %p)\n", prGlueInfo,
               prGlueInfo ? prGlueInfo->prAdapter : NULL);
    }
}

int mtk_cfg80211_suspend(struct wiphy *wiphy, struct cfg80211_wowlan *wow)
{
    P_GLUE_INFO_T prGlueInfo;
    ADAPTER_T *prAdapter;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;

    DBGLOG(REQ, INFO, "CFG80211 suspend CB\n");
    if (!wlanGetGlueInfo()) {
        DBGLOG(REQ, ERROR, "NIC does not exist!\n");
        return 0;
    }

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);
    ASSERT(prGlueInfo);

    prAdapter = prGlueInfo->prAdapter;
    prAdapter->fgIsCfg80211SuspendCalled = true;

    DBGLOG(REQ, WARN, "Wow:%d, WowEnable:%d, state:%d\n",
           prGlueInfo->prAdapter->rWifiVar.ucWow,
           prGlueInfo->prAdapter->rWowCtrl.fgWowEnable,
           kalGetMediaStateIndicated(prGlueInfo));

    rStatus = wlanSuspendLinkDown(prGlueInfo);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "cfg 80211 suspend fail!\n");
        return -EINVAL;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief cfg80211 resume callback, will be invoked in wiphy_resume.
 *
 * @param wiphy: pointer to wiphy
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_resume(struct wiphy *wiphy)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    u32 rStatus, u4InfoLen;

    DBGLOG(REQ, TRACE, "mtk_cfg80211_resume\n");

    WIPHY_PRIV(wiphy, prGlueInfo);
    if (prGlueInfo) {
        prAdapter = prGlueInfo->prAdapter;
    }
    if (prAdapter == NULL) {
        goto end;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidIndicateBssInfo, (void *)NULL, 0,
                       false, false, false, &u4InfoLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "ScanResultLog error:%x\n", rStatus);
    }
end:
    return 0;
}

#if CFG_ENABLE_UNIFY_WIPHY
/*----------------------------------------------------------------------------*/
/*!
 * @brief Check the net device is P2P net device (P2P GO/GC, AP), or not.
 *
 * @param prGlueInfo : the driver private data
 *        ndev       : the net device
 *
 * @retval 0:  AIS device (STA/IBSS)
 *         1:  P2P GO/GC, AP
 */
/*----------------------------------------------------------------------------*/
int mtk_IsP2PNetDevice(P_GLUE_INFO_T prGlueInfo, struct net_device *ndev)
{
    P_NETDEV_PRIVATE_GLUE_INFO prNetDevPrivate = NULL;
    int iftype = 0;
    int ret = 1;

    if (ndev == NULL) {
        DBGLOG(REQ, WARN, "ndev is NULL\n");
        return -1;
    }

    prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(ndev);
    iftype = ndev->ieee80211_ptr->iftype;

    /* P2P device/GO/GC always return 1 */
    if (prNetDevPrivate->ucIsP2p == true) {
        ret = 1;
    }else if (iftype == NL80211_IFTYPE_STATION) {
        ret = 0;
    }else if (iftype == NL80211_IFTYPE_ADHOC) {
        ret = 0;
    }

    DBGLOG(REQ, INFO, "cfg path select = %d\n", ret);
    return ret;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialize the AIS related FSM and data.
 *
 * @param prGlueInfo : the driver private data
 *        ndev       : the net device
 *        ucBssIdx   : the AIS BSS index adssigned by the driver (wlanProbe)
 *
 * @retval 0
 *
 */
/*----------------------------------------------------------------------------*/
int mtk_init_sta_role(P_ADAPTER_T prAdapter, struct net_device *ndev)
{
    P_NETDEV_PRIVATE_GLUE_INFO prNdevPriv = NULL;

    if ((prAdapter == NULL) || (ndev == NULL)) {
        return -1;
    }

    /* uninit AIS FSM */
    aisFsmInit(prAdapter);

#if CFG_SUPPORT_ROAMING
    /* Roaming Module - unintiailization */
    roamingFsmInit(prAdapter);
#endif  /* CFG_SUPPORT_ROAMING */

    ndev->netdev_ops = wlanGetNdevOps();
    ndev->ieee80211_ptr->iftype = NL80211_IFTYPE_STATION;
    kal_eth_hw_addr_set(ndev, prAdapter->rMyMacAddr);

    /* set the ndev's ucBssIdx to the AIS BSS index */
    prNdevPriv = (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(ndev);
    prNdevPriv->ucBssIdx = prAdapter->prAisBssInfo->ucBssIndex;
    return 0;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief Uninitialize the AIS related FSM and data.
 *
 * @param prAdapter : the driver private data
 *
 * @retval 0
 *
 */
/*----------------------------------------------------------------------------*/
int mtk_uninit_sta_role(P_ADAPTER_T prAdapter, struct net_device *ndev)
{
    P_NETDEV_PRIVATE_GLUE_INFO prNdevPriv = NULL;

    if ((prAdapter == NULL) || (ndev == NULL)) {
        return -1;
    }

#if CFG_SUPPORT_ROAMING
    /* Roaming Module - unintiailization */
    roamingFsmUninit(prAdapter);
#endif  /* CFG_SUPPORT_ROAMING */

    /* uninit AIS FSM */
    aisFsmUninit(prAdapter);

    /* set the ucBssIdx to the illegal value */
    prNdevPriv = (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(ndev);
    prNdevPriv->ucBssIdx = 0xff;
    return 0;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialize the AP (P2P) related FSM and data.
 *
 * @param prGlueInfo : the driver private data
 *        ndev       : net device
 *
 * @retval 0      : success
 *         others : can't alloc and setup the AP FSM & data
 *
 */
/*----------------------------------------------------------------------------*/
int mtk_init_ap_role(P_GLUE_INFO_T prGlueInfo, struct net_device *ndev)
{
    int u4Idx = 0;
    P_ADAPTER_T prAdapter = prGlueInfo->prAdapter;

    for (u4Idx = 0; u4Idx < KAL_P2P_NUM; u4Idx++)
        if (gprP2pRoleWdev[u4Idx] == NULL) {
            break;
        }

    if (u4Idx >= KAL_P2P_NUM) {
        DBGLOG(INIT, ERROR, "There is no free gprP2pRoleWdev.\n");
        return -ENOMEM;
    }

    if ((u4Idx == 0) || (prAdapter == NULL) ||
        (prAdapter->rP2PNetRegState != ENUM_NET_REG_STATE_REGISTERED)) {
        DBGLOG(INIT, ERROR, "The wlan0 can't set to AP without p2p0\n");
        /* System will crash, if p2p0 isn't existing. */
        return -EFAULT;
    }

    /* reference from the glRegisterP2P() */
    gprP2pRoleWdev[u4Idx] = ndev->ieee80211_ptr;

    if (glSetupP2P(prGlueInfo, gprP2pRoleWdev[u4Idx], ndev, u4Idx, true)) {
        gprP2pRoleWdev[u4Idx] = NULL;
        return -EFAULT;
    }

    prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum++;

    /* reference from p2pNetRegister() */
    /* The ndev doesn't need register_netdev, only reassign the gPrP2pDev.*/
    gPrP2pDev[u4Idx] = ndev;

    return 0;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief Unnitialize the AP (P2P) related FSM and data.
 *
 * @param prGlueInfo : the driver private data
 *        ndev       : net device
 *
 * @retval 0      : success
 *         others : can't find the AP information by the ndev
 *
 */
/*----------------------------------------------------------------------------*/
int mtk_uninit_ap_role(P_GLUE_INFO_T prGlueInfo, struct net_device *ndev)
{
    unsigned char u4Idx;

    if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev, &u4Idx) != 0) {
        DBGLOG(INIT, WARN, "can't find the matched dev to uninit AP\n");
        return -EFAULT;
    }

    glUnregisterP2P(prGlueInfo, u4Idx);

    gPrP2pDev[u4Idx] = NULL;
    gprP2pRoleWdev[u4Idx] = NULL;

    return 0;
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
int mtk_cfg_start_radar_detection(struct wiphy *wiphy,
                                  struct net_device *dev,
                                  struct cfg80211_chan_def *chandef,
                                  unsigned int cac_time_ms
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
                                  , int link_id
#endif
                                  )
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    uint8_t state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
        // DBGLOG(REQ, WARN, "STA doesn't support this function\n");
        return -EFAULT;
    }

    return mtk_p2p_cfg80211_start_radar_detection(wiphy,
                                                  dev, chandef, cac_time_ms
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
                                                  , link_id
#endif
                                                  );
}

int mtk_cfg_channel_switch(struct wiphy *wiphy,
                           struct net_device *dev,
                           struct cfg80211_csa_settings *params)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    uint8_t state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
        // DBGLOG(REQ, WARN, "STA doesn't support this function\n");
        return -EFAULT;
    }

    return mtk_p2p_cfg80211_channel_switch(wiphy, dev, params);
}
#endif

struct wireless_dev *mtk_cfg_add_iface(struct wiphy *wiphy, const char *name,
                                       unsigned char name_assign_type,
                                       enum nl80211_iftype type,
                                       struct vif_params *params)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return ERR_PTR(-EFAULT);
    }


    /* TODO: error handele for the non-P2P interface */
#if (CFG_ENABLE_WIFI_DIRECT_CFG_80211 == 0)
    DBGLOG(REQ, WARN, "P2P is not supported\n");
    return ERR_PTR(-EINVAL);
#else  /* CFG_ENABLE_WIFI_DIRECT_CFG_80211 */

    return mtk_p2p_cfg80211_add_iface(wiphy, name, name_assign_type, type,
                                      params);

#endif  /* CFG_ENABLE_WIFI_DIRECT_CFG_80211 */
}
int mtk_cfg_del_iface(struct wiphy *wiphy, struct wireless_dev *wdev)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    /* TODO: error handele for the non-P2P interface */
#if (CFG_ENABLE_WIFI_DIRECT_CFG_80211 == 0)
    DBGLOG(REQ, WARN, "P2P is not supported\n");
    return -EINVAL;
#else  /* CFG_ENABLE_WIFI_DIRECT_CFG_80211 */
    return mtk_p2p_cfg80211_del_iface(wiphy, wdev);
#endif  /* CFG_ENABLE_WIFI_DIRECT_CFG_80211 */
}


int mtk_cfg_change_iface(struct wiphy *wiphy, struct net_device *ndev,
                         enum nl80211_iftype type,
                         struct vif_params *params)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    P_NETDEV_PRIVATE_GLUE_INFO prNetdevPriv = NULL;
    P_P2P_INFO_T prP2pInfo = NULL;
    struct cfg80211_scan_request *prScanRequest = NULL;
    u8 state = 0;

    GLUE_SPIN_LOCK_DECLARATION();
    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    ASSERT(prGlueInfo);

    DBGLOG(P2P, INFO, "ndev=%p, new type=%d\n", ndev, type);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (!ndev) {
        DBGLOG(REQ, WARN, "ndev is NULL\n");
        return -EINVAL;
    }

    prNetdevPriv = (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(ndev);

#if (CFG_ENABLE_WIFI_DIRECT_CFG_80211)
    /* P2P Interfcace */
    if (prNetdevPriv->ucIsP2p == true) {
        return mtk_p2p_cfg80211_change_iface(wiphy, ndev, type, params);
    }
#endif  /* CFG_ENABLE_WIFI_DIRECT_CFG_80211 */

    prAdapter = prGlueInfo->prAdapter;

    if (ndev->ieee80211_ptr->iftype == type) {
        DBGLOG(REQ, INFO, "ndev type is not changed (%d)\n", type);
        return 0;
    }

    netif_carrier_off(ndev);
    /* stop ap will stop all queue, and kalIndicateStatusAndComplete only do
     * netif_carrier_on. So that, the following STA can't send 4-way M2 to
     * AP.
     */
    netif_tx_stop_all_queues(ndev);

    /* flush scan */
    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
    if ((prGlueInfo->prScanRequest != NULL) &&
        (prGlueInfo->prScanRequest->wdev == ndev->ieee80211_ptr)) {
        prScanRequest = prGlueInfo->prScanRequest;
        if (prScanRequest) {
            kalCfg80211ScanDone(prScanRequest, true);
        }
        prGlueInfo->prScanRequest = NULL;
    }
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

    /* expect that only AP & STA will be handled here (excluding IBSS) */
    if (type == NL80211_IFTYPE_AP) {
        /* STA mode change to AP mode */
        prP2pInfo = prAdapter->prP2pInfo;

        if (prP2pInfo == NULL) {
            DBGLOG(INIT, ERROR, "prP2pInfo is NULL\n");
            return -EFAULT;
        }

        if (prP2pInfo->u4DeviceNum >= KAL_P2P_NUM) {
            DBGLOG(INIT, ERROR, "resource invalid, u4DeviceNum=%d\n",
                   prP2pInfo->u4DeviceNum);
            return -EFAULT;
        }

        mtk_uninit_sta_role(prAdapter, ndev);

        if (mtk_init_ap_role(prGlueInfo, ndev) != 0) {
            DBGLOG(INIT, ERROR, "mtk_init_ap_role FAILED\n");
            /* Only AP/P2P resource has the failure case.	*/
            /* So, just re-init AIS.			*/
            mtk_init_sta_role(prAdapter, ndev);
            return -EFAULT;
        }
    } else {
        /* AP mode change to STA mode */
        if (mtk_uninit_ap_role(prGlueInfo, ndev) != 0) {
            DBGLOG(INIT, ERROR, "mtk_uninit_ap_role FAILED\n");
            return -EFAULT;
        }

        mtk_init_sta_role(prAdapter, ndev);
        /* continue the mtk_cfg80211_change_iface() process */
        mtk_cfg80211_change_iface(wiphy, ndev, type, params);
    }

    return 0;
}

int mtk_cfg_add_key(struct wiphy *wiphy, struct net_device *ndev,
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                    int link_id,
#endif
                    u8 key_index,
                    bool pairwise, const u8 *mac_addr,
                    struct key_params *params)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_add_key(wiphy, ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                                        0,
#endif
                                        key_index, pairwise,
                                        mac_addr, params);
    }
    /* STA Mode */
    return mtk_cfg80211_add_key(wiphy, ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                                0,
#endif
                                key_index, pairwise, mac_addr,
                                params);
}


int mtk_cfg_get_key(struct wiphy *wiphy, struct net_device *ndev,
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                    int link_id,
#endif
                    u8 key_index,
                    bool pairwise, const u8 *mac_addr, void *cookie,
                    void (*callback)(void *cookie, struct key_params *))
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_get_key(wiphy, ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                                        link_id,
#endif
                                        key_index, pairwise,
                                        mac_addr, cookie, callback);
    }
    /* STA Mode */
    return mtk_cfg80211_get_key(wiphy, ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                                link_id,
#endif
                                key_index, pairwise, mac_addr,
                                cookie, callback);
}


int mtk_cfg_del_key(struct wiphy *wiphy, struct net_device *ndev,
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                    int link_id,
#endif
                    u8 key_index,
                    bool pairwise, const u8 *mac_addr)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_del_key(wiphy, ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                                        link_id,
#endif
                                        key_index, pairwise,
                                        mac_addr);
    }
    /* STA Mode */
    return mtk_cfg80211_del_key(wiphy, ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                                link_id,
#endif
                                key_index, pairwise, mac_addr);
}

int mtk_cfg_set_default_key(struct wiphy *wiphy, struct net_device *ndev,
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                            int link_id,
#endif
                            u8 key_index, bool unicast, bool multicast)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_set_default_key(wiphy, ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                                                link_id,
#endif
                                                key_index, unicast,
                                                multicast);
    }
    /* STA Mode */
    return mtk_cfg80211_set_default_key(wiphy, ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
                                        link_id,
#endif
                                        key_index, unicast,
                                        multicast);
}

#if KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_set_default_mgmt_key(struct wiphy *wiphy,
                                 struct net_device *ndev, int link_id,
                                 u8 key_index)
#else
int mtk_cfg_set_default_mgmt_key(struct wiphy *wiphy,
                                 struct net_device *ndev, u8 key_index)
#endif
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
        return mtk_p2p_cfg80211_set_mgmt_key(wiphy, ndev, link_id, key_index);
#else
        return mtk_p2p_cfg80211_set_mgmt_key(wiphy, ndev, key_index);
#endif
    }
    /* STA Mode */
    DBGLOG(REQ, WARN, "STA don't support this function\n");
    return -EFAULT;
}


int mtk_cfg_get_station(struct wiphy *wiphy, struct net_device *ndev,
                        const u8 *mac, struct station_info *sinfo)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_get_station(wiphy, ndev, mac, sinfo);
    }

    /* STA Mode */
    return mtk_cfg80211_get_station(wiphy, ndev, mac, sinfo);
}

#if CFG_SUPPORT_TDLS
int mtk_cfg_change_station(struct wiphy *wiphy,
                           struct net_device *ndev,
                           const u8 *mac, struct station_parameters *params)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
        return -EFAULT;
    }
    /* STA Mode */
    return mtk_cfg80211_change_station(wiphy, ndev, mac,
                                       params);
}

int mtk_cfg_add_station(struct wiphy *wiphy, struct net_device *ndev,
                        const u8 *mac, struct station_parameters *params)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
        return -EFAULT;
    }
    /* STA Mode */
    return mtk_cfg80211_add_station(wiphy, ndev, mac, params);
}


int mtk_cfg_tdls_oper(struct wiphy *wiphy, struct net_device *ndev,
                      const u8 *peer, enum nl80211_tdls_operation oper)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
        return -EFAULT;
    }
    /* STA Mode */
    return mtk_cfg80211_tdls_oper(wiphy, ndev, peer, oper);
}

int mtk_cfg_tdls_mgmt(struct wiphy *wiphy, struct net_device *dev,
                      const u8 *peer,
#if (CFG80211_VERSION_CODE >= KERNEL_VERSION(6, 5, 0))
                      int link_id,
#endif
                      u8 action_code, u8 dialog_token,
                      u16 status_code, u32 peer_capability, bool initiator,
                      const u8 *buf, size_t len
                      )
{
    GLUE_INFO_T *prGlueInfo;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, dev) > 0) {
        DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
        return -EFAULT;
    }

    return mtk_cfg80211_tdls_mgmt(wiphy, dev, peer,
#if (CFG80211_VERSION_CODE >= KERNEL_VERSION(6, 5, 0))
                                  link_id,
#endif
                                  action_code, dialog_token,
                                  status_code, peer_capability, initiator, buf,
                                  len);

}
#endif  /* CFG_SUPPORT_TDLS */


int mtk_cfg_del_station(struct wiphy *wiphy, struct net_device *ndev,
                        struct station_del_parameters *params)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_del_station(wiphy, ndev, params);
    }
    /* STA Mode */
#if CFG_SUPPORT_TDLS
    return mtk_cfg80211_del_station(wiphy, ndev, params);
#else  /* CFG_SUPPORT_TDLS == 0 */
    /* AIS only support this function when CFG_SUPPORT_TDLS */
    return -EFAULT;
#endif  /* CFG_SUPPORT_TDLS */
}

int mtk_cfg_scan(struct wiphy *wiphy, struct cfg80211_scan_request *request)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, request->wdev->netdev) > 0) {
        return mtk_p2p_cfg80211_scan(wiphy, request);
    }

    /* STA Mode */
    return mtk_cfg80211_scan(wiphy, request);
}

void mtk_cfg_abort_scan(struct wiphy *wiphy, struct wireless_dev *wdev)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0) {
        mtk_p2p_cfg80211_abort_scan(wiphy, wdev);
    }
    /* STA Mode */
    mtk_cfg80211_abort_scan(wiphy, wdev);
}

int mtk_cfg_sched_scan_start(IN struct wiphy *wiphy,
                             IN struct net_device *ndev,
                             IN struct cfg80211_sched_scan_request *request)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
        return -EFAULT;
    }

    return mtk_cfg80211_sched_scan_start(wiphy, ndev, request);

}


int mtk_cfg_sched_scan_stop(IN struct wiphy *wiphy,
                            IN struct net_device *ndev,
                            IN u64 reqid)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
        return -EFAULT;
    }

    return mtk_cfg80211_sched_scan_stop(wiphy, ndev, reqid);
}

int mtk_cfg_connect(struct wiphy *wiphy, struct net_device *ndev,
                    struct cfg80211_connect_params *sme)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_connect(wiphy, ndev, sme);
    }
    /* STA Mode */
    return mtk_cfg80211_connect(wiphy, ndev, sme);
}

int mtk_cfg_disconnect(struct wiphy *wiphy, struct net_device *ndev,
                       u16 reason_code)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_disconnect(wiphy, ndev, reason_code);
    }
    /* STA Mode */
    return mtk_cfg80211_disconnect(wiphy, ndev, reason_code);
}

int mtk_cfg_join_ibss(struct wiphy *wiphy, struct net_device *ndev,
                      struct cfg80211_ibss_params *params)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T)wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_join_ibss(wiphy, ndev, params);
    }
    /* STA Mode */
    return mtk_cfg80211_join_ibss(wiphy, ndev, params);
}

int mtk_cfg_leave_ibss(struct wiphy *wiphy,
                       struct net_device *ndev)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_leave_ibss(wiphy, ndev);
    }
    /* STA Mode */
    return mtk_cfg80211_leave_ibss(wiphy, ndev);
}

int mtk_cfg_set_power_mgmt(struct wiphy *wiphy,
                           struct net_device *ndev,
                           bool enabled, int timeout)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_set_power_mgmt(wiphy, ndev,
                                               enabled, timeout);
    }
    /* STA Mode */
    return mtk_cfg80211_set_power_mgmt(wiphy, ndev, enabled,
                                       timeout);
}

int mtk_cfg_set_pmksa(struct wiphy *wiphy,
                      struct net_device *ndev,
                      struct cfg80211_pmksa *pmksa)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
        return -EFAULT;
    }

    return mtk_cfg80211_set_pmksa(wiphy, ndev, pmksa);
}

int mtk_cfg_del_pmksa(struct wiphy *wiphy,
                      struct net_device *ndev,
                      struct cfg80211_pmksa *pmksa)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
        return -EFAULT;
    }

    return mtk_cfg80211_del_pmksa(wiphy, ndev, pmksa);
}

int mtk_cfg_flush_pmksa(struct wiphy *wiphy,
                        struct net_device *ndev)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
        return -EFAULT;
    }

    return mtk_cfg80211_flush_pmksa(wiphy, ndev);
}

#if CONFIG_SUPPORT_GTK_REKEY
int mtk_cfg_set_rekey_data(struct wiphy *wiphy,
                           struct net_device *dev,
                           struct cfg80211_gtk_rekey_data *data)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, dev) > 0) {
        DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
        return -EFAULT;
    }

    return mtk_cfg80211_set_rekey_data(wiphy, dev, data);
}
#endif  /* CONFIG_SUPPORT_GTK_REKEY */

int mtk_cfg_suspend(struct wiphy *wiphy,
                    struct cfg80211_wowlan *wow)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return 0;
    }

    /* TODO: AP/P2P do not support this function, should take that case. */
    return mtk_cfg80211_suspend(wiphy, wow);
}

int mtk_cfg_resume(struct wiphy *wiphy)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return 0;
    }

    /* TODO: AP/P2P do not support this function, should take that case. */
    return mtk_cfg80211_resume(wiphy);
}

int mtk_cfg_auth(struct wiphy *wiphy, struct net_device *ndev,
                 struct cfg80211_auth_request *req)
{
    P_GLUE_INFO_T prGlueInfo = NULL;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if ((!prGlueInfo) || (prGlueInfo->u4ReadyFlag == 0)) {
        DBGLOG(REQ, WARN, "driver is not ready\n");
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_auth(wiphy, ndev, req);
    }

    /* STA Mode */
    return mtk_cfg80211_auth(wiphy, ndev, req);
}

int mtk_cfg_assoc(struct wiphy *wiphy,
                  struct net_device *ndev,
                  struct cfg80211_assoc_request *req)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
        return mtk_p2p_cfg80211_assoc(wiphy, ndev, req);
    }

    /* STA Mode */
    return mtk_cfg80211_assoc(wiphy, ndev, req);
}

int mtk_cfg_remain_on_channel(struct wiphy *wiphy,
                              struct wireless_dev *wdev,
                              struct ieee80211_channel *chan,
                              unsigned int duration, u64 *cookie)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0) {
        return mtk_p2p_cfg80211_remain_on_channel(wiphy, wdev, chan,
                                                  duration, cookie);
    }
    /* STA Mode */
    return mtk_cfg80211_remain_on_channel(wiphy, wdev, chan,
                                          duration, cookie);
}

int mtk_cfg_cancel_remain_on_channel(struct wiphy *wiphy,
                                     struct wireless_dev *wdev, u64 cookie)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0) {
        return mtk_p2p_cfg80211_cancel_remain_on_channel(wiphy,
                                                         wdev,
                                                         cookie);
    }
    /* STA Mode */
    return mtk_cfg80211_cancel_remain_on_channel(wiphy, wdev,
                                                 cookie);
}

int mtk_cfg_mgmt_tx(struct wiphy *wiphy,
                    struct wireless_dev *wdev,
                    struct cfg80211_mgmt_tx_params *params, u64 *cookie)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0) {
        return mtk_p2p_cfg80211_mgmt_tx(wiphy, wdev, params,
                                        cookie);
    }
    /* STA Mode */
    return mtk_cfg80211_mgmt_tx(wiphy, wdev, params, cookie);

}

#if KERNEL_VERSION(5, 8, 0) > CFG80211_VERSION_CODE
void mtk_cfg_mgmt_frame_register(struct wiphy *wiphy,
                                 struct wireless_dev *wdev,
                                 u16 frame_type, bool reg)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0) {
        mtk_p2p_cfg80211_mgmt_frame_register(wiphy, wdev,
                                             frame_type,
                                             reg);
    } else {
        mtk_cfg80211_mgmt_frame_register(wiphy, wdev, frame_type,
                                         reg);
    }
}

#else
void mtk_cfg_mgmt_frame_update(struct wiphy *wiphy,
                               struct wireless_dev *wdev,
                               struct mgmt_frame_regs *upd)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u_int8_t fgIsP2pNetDevice = false;
    u32 *pu4PacketFilter = NULL;

    if ((wiphy == NULL) || (wdev == NULL) || (upd == NULL)) {
        DBGLOG(INIT, TRACE, "Invalidate params\n");
        return;
    }
    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);
    if ((!prGlueInfo) || (prGlueInfo->u4ReadyFlag == 0)) {
        DBGLOG(REQ, WARN, "driver is not ready\n");
        return;
    }
    fgIsP2pNetDevice = mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev);
    DBGLOG(INIT, TRACE,
           "netdev(0x%p) update management frame filter: 0x%08x\n",
           wdev->netdev, upd->interface_stypes);

    if (fgIsP2pNetDevice) {
        u8 ucRoleIdx = 0;
        P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo =
            (P_P2P_ROLE_FSM_INFO_T)NULL;
        if (prGlueInfo->prP2PInfo[0]->prDevHandler ==
            wdev->netdev) {
            pu4PacketFilter =
                &prGlueInfo->prP2PDevInfo
                ->u4OsMgmtFrameFilter;
            /* Reset filters*/
            *pu4PacketFilter = 0;
        } else {
            if (mtk_Netdev_To_RoleIdx(prGlueInfo,
                                      wdev->netdev, &ucRoleIdx) < 0) {
                DBGLOG(P2P, WARN,
                       "wireless dev match fail!\n");
                return;
            }
            /* Non P2P device*/
            if (ucRoleIdx >= KAL_P2P_NUM) {
                DBGLOG(P2P, WARN,
                       "Invalid RoleIdx %u\n",
                       ucRoleIdx);
                return;
            }
            DBGLOG(P2P, TRACE,
                   "Open packet filer RoleIdx %u\n",
                   ucRoleIdx);
            prP2pRoleFsmInfo =
                prGlueInfo->prAdapter->rWifiVar
                .aprP2pRoleFsmInfo[ucRoleIdx];
            pu4PacketFilter = &prP2pRoleFsmInfo
                              ->u4P2pPacketFilter;
            *pu4PacketFilter =
                PARAM_PACKET_FILTER_SUPPORTED;
        }
    } else {
        pu4PacketFilter = &prGlueInfo->u4OsMgmtFrameFilter;
        *pu4PacketFilter = 0;
    }
    // if (upd->interface_stypes & MASK_MAC_FRAME_PROBE_REQ) {
    //     *pu4PacketFilter |= PARAM_PACKET_FILTER_PROBE_REQ;
    // }
    // if (upd->interface_stypes & MASK_MAC_FRAME_ACTION) {
    //     *pu4PacketFilter |= PARAM_PACKET_FILTER_ACTION_FRAME;
    // }
    set_bit(fgIsP2pNetDevice ?
            GLUE_FLAG_FRAME_FILTER_BIT :
            GLUE_FLAG_FRAME_FILTER_AIS_BIT,
            &prGlueInfo->ulFlag);
    /* wake up main thread */
    wake_up_interruptible(&prGlueInfo->waitq);

}
#endif

#if (CFG_ENABLE_WIFI_DIRECT_CFG_80211 != 0)
int mtk_cfg_change_bss(struct wiphy *wiphy,
                       struct net_device *dev,
                       struct bss_parameters *params)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
        // DBGLOG(REQ, WARN, "STA doesn't support this function\n");
        return -EFAULT;
    }

    return mtk_p2p_cfg80211_change_bss(wiphy, dev, params);
}

int mtk_cfg_mgmt_tx_cancel_wait(struct wiphy *wiphy,
                                struct wireless_dev *wdev,
                                u64 cookie)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) <= 0) {
        // DBGLOG(REQ, WARN, "STA doesn't support this function\n");
        return -EFAULT;
    }

    return mtk_p2p_cfg80211_mgmt_tx_cancel_wait(wiphy, wdev,
                                                cookie);
}

int mtk_cfg_deauth(struct wiphy *wiphy,
                   struct net_device *dev,
                   struct cfg80211_deauth_request *req)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;
    int ret = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, dev) > 0) {
        ret = mtk_p2p_cfg80211_deauth(wiphy, dev, req);
    }else{
        ret = mtk_cfg80211_deauth(wiphy, dev, req);
    }

    return ret;
}

int mtk_cfg_disassoc(struct wiphy *wiphy,
                     struct net_device *dev,
                     struct cfg80211_disassoc_request *req)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
        // DBGLOG(REQ, WARN, "STA doesn't support this function\n");
        return -EFAULT;
    }

    return mtk_p2p_cfg80211_disassoc(wiphy, dev, req);
}

int mtk_cfg_start_ap(struct wiphy *wiphy,
                     struct net_device *dev,
                     struct cfg80211_ap_settings *settings)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
        // DBGLOG(REQ, WARN, "STA doesn't support this function\n");
        return -EFAULT;
    }

    return mtk_p2p_cfg80211_start_ap(wiphy, dev, settings);
}

int mtk_cfg_change_beacon(struct wiphy *wiphy,
                          struct net_device *dev,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0))
                          struct cfg80211_beacon_data *info
#else
                          struct cfg80211_ap_update *info
#endif
                          )
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
        // DBGLOG(REQ, WARN, "STA doesn't support this function\n");
        return -EFAULT;
    }

    return mtk_p2p_cfg80211_change_beacon(wiphy, dev, info);
}

#if KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_stop_ap(struct wiphy *wiphy,
                    struct net_device *dev,
                    unsigned int link_id)
#else
int mtk_cfg_stop_ap(struct wiphy *wiphy,
                    struct net_device *dev)
#endif
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
        // DBGLOG(REQ, WARN, "STA doesn't support this function\n");
        return -EFAULT;
    }
#if KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE
    return mtk_p2p_cfg80211_stop_ap(wiphy, dev, link_id);
#else
    return mtk_p2p_cfg80211_stop_ap(wiphy, dev);
#endif
}

int mtk_cfg_set_wiphy_params(struct wiphy *wiphy,
                             u32 changed)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    /* TODO: AIS not support this function */
    return mtk_p2p_cfg80211_set_wiphy_params(wiphy, changed);
}

int mtk_cfg_set_bitrate_mask(struct wiphy *wiphy,
                             struct net_device *dev,
#if KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE
                             unsigned int link_id,
#endif
                             const u8 *peer,
                             const struct cfg80211_bitrate_mask *mask)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
        // DBGLOG(REQ, WARN, "STA doesn't support this function\n");
        return -EFAULT;
    }

#if KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE
    return mtk_p2p_cfg80211_set_bitrate_mask(wiphy, dev, link_id,
                                             peer, mask);
#else
    return mtk_p2p_cfg80211_set_bitrate_mask(wiphy, dev, peer,
                                             mask);
#endif
}

int mtk_cfg_set_txpower(struct wiphy *wiphy,
                        struct wireless_dev *wdev,
                        enum nl80211_tx_power_setting type, int mbm)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) <= 0) {
        // DBGLOG(REQ, WARN, "STA doesn't support this function\n");
        return -EFAULT;
    }

    return mtk_p2p_cfg80211_set_txpower(wiphy, wdev, type, mbm);
}

int mtk_cfg_get_txpower(struct wiphy *wiphy,
                        struct wireless_dev *wdev,
                        int *dbm)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u8 state = 0;

    prGlueInfo = (P_GLUE_INFO_T )wiphy_priv(wiphy);

    if (!halIsHifStateReady(prGlueInfo->prAdapter, &state)) {
        DBGLOG(REQ, WARN, "driver is not ready, state:%d\n", state);
        return -EFAULT;
    }

    if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) <= 0) {
        DBGLOG_RATELIMIT(REQ, WARN,
                         "STA doesn't support this function\n");
        return -EFAULT;
    }

    return mtk_p2p_cfg80211_get_txpower(wiphy, wdev, dbm);
}
#endif  /* (CFG_ENABLE_WIFI_DIRECT_CFG_80211 != 0) */
#endif  /* CFG_ENABLE_UNIFY_WIPHY */
