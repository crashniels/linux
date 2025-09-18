// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_p2p_kal.c
 *    \brief
 *
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "net/cfg80211.h"
#include "precomp.h"
#include "gl_wext.h"

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

struct ieee80211_channel *
kalP2pFuncGetChannelEntry(IN P_GL_P2P_INFO_T prP2pInfo,
                          IN P_RF_CHANNEL_INFO_T prChannelInfo);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief to update the assoc req to p2p
 *
 * \param[in]
 *           prGlueInfo
 *           pucFrameBody
 *           u4FrameBodyLen
 *           fgReassocRequest
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalP2PUpdateAssocInfo(IN P_GLUE_INFO_T prGlueInfo, IN u8 *pucFrameBody,
                           IN u32 u4FrameBodyLen, IN u8 fgReassocRequest,
                           IN u8 ucBssIndex)
{
    P_BSS_INFO_T prBssInfo;
    union iwreq_data wrqu;
    unsigned char *pucExtraInfo = NULL;
    unsigned char *pucDesiredIE = NULL;
    /* unsigned char aucExtraInfoBuf[200]; */
    u8 *cp;
    struct net_device *prNetdevice = (struct net_device *)NULL;

    memset(&wrqu, 0, sizeof(wrqu));

    if (fgReassocRequest) {
        if (u4FrameBodyLen < 15) {
            /*
             *  printk(KERN_WARNING "frameBodyLen too short:%ld\n",
             * frameBodyLen);
             */
            return;
        }
    } else {
        if (u4FrameBodyLen < 9) {
            /*
             *  printk(KERN_WARNING "frameBodyLen too short:%ld\n",
             * frameBodyLen);
             */
            return;
        }
    }

    cp = pucFrameBody;

    if (fgReassocRequest) {
        /* Capability information field 2 */
        /* Listen interval field 2 */
        /* Current AP address 6 */
        cp += 10;
        u4FrameBodyLen -= 10;
    } else {
        /* Capability information field 2 */
        /* Listen interval field 2 */
        cp += 4;
        u4FrameBodyLen -= 4;
    }

    /* do supplicant a favor, parse to the start of WPA/RSN IE */
    if (wextSrchDesiredWPSIE(cp, u4FrameBodyLen, 0xDD, &pucDesiredIE)) {
        /* WPS IE found */
    } else if (wextSrchDesiredWPAIE(cp, u4FrameBodyLen, 0x30, &pucDesiredIE)) {
        /* RSN IE found */
    } else if (wextSrchDesiredWPAIE(cp, u4FrameBodyLen, 0xDD, &pucDesiredIE)) {
        /* WPA IE found */
    } else {
        /* no WPA/RSN IE found, skip this event */
        return;
    }

    /* IWEVASSOCREQIE, indicate binary string */
    pucExtraInfo = pucDesiredIE;
    wrqu.data.length = pucDesiredIE[1] + 2;

    prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);

    if (ucBssIndex == P2P_DEV_BSS_INDEX) {
        prNetdevice =
            prGlueInfo->prP2PInfo[prBssInfo->u4PrivateData]->prDevHandler;
    } else {
        prNetdevice =
            prGlueInfo->prP2PInfo[prBssInfo->u4PrivateData]->aprRoleHandler;
    }

    /* Send event to user space */
    wireless_send_event(prNetdevice, IWEVASSOCREQIE, &wrqu, pucExtraInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to retrieve Bluetooth-over-Wi-Fi role
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           0: P2P Device
 *           1: Group Client
 *           2: Group Owner
 */
/*----------------------------------------------------------------------------*/
u8 kalP2PGetRole(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx)
{
    ASSERT(prGlueInfo);

    return prGlueInfo->prP2PInfo[ucRoleIdx]->ucRole;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to set Wi-Fi Direct role
 *
 * \param[in]
 *           prGlueInfo
 *           ucResult
 *                   0: successful
 *                   1: error
 *           ucRole
 *                   0: P2P Device
 *                   1: Group Client
 *                   2: Group Owner
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalP2PSetRole(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRole, IN u8 ucRoleIdx)
{
    ASSERT(prGlueInfo);
    ASSERT(ucRole <= 2);

    prGlueInfo->prP2PInfo[ucRoleIdx]->ucRole = ucRole;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to set the cipher for p2p
 *
 * \param[in]
 *           prGlueInfo
 *           u4Cipher
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalP2PSetCipher(IN P_GLUE_INFO_T prGlueInfo, IN u32 u4Cipher,
                     IN u8 ucRoleIdx)
{
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PInfo[ucRoleIdx]);

    /* It can be WEP40 (used to identify cipher is WEP), TKIP and CCMP */
    prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise = u4Cipher;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to get the cipher, return false for security is none
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           true: cipher is ccmp
 *           false: cipher is none
 */
/*----------------------------------------------------------------------------*/
u8 kalP2PGetCipher(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx)
{
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PInfo[ucRoleIdx]);

    if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise ==
        IW_AUTH_CIPHER_CCMP) {
        return true;
    }

    if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise ==
        IW_AUTH_CIPHER_TKIP) {
        return true;
    }

    if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise ==
        IW_AUTH_CIPHER_WEP40) {
        return true;
    }

    return false;
}

u8 kalP2PGetWepCipher(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx)
{
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PInfo[ucRoleIdx]);

    if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise ==
        IW_AUTH_CIPHER_WEP40) {
        return true;
    }

    if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise ==
        IW_AUTH_CIPHER_WEP104) {
        return true;
    }

    return false;
}

#if CFG_SUPPORT_SUITB
u8 kalP2PGetGcmp256Cipher(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx)
{
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PInfo[ucRoleIdx]);

    if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise ==
        IW_AUTH_CIPHER_GCMP256) {
        return true;
    }

    return false;
}
#endif

u8 kalP2PGetCcmpCipher(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx)
{
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PInfo[ucRoleIdx]);

    if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise ==
        IW_AUTH_CIPHER_CCMP) {
        return true;
    }

    if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise ==
        IW_AUTH_CIPHER_TKIP) {
        return false;
    }

    return false;
}

u8 kalP2PGetTkipCipher(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx)
{
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PInfo[ucRoleIdx]);

    if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise ==
        IW_AUTH_CIPHER_CCMP) {
        return false;
    }

    if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise ==
        IW_AUTH_CIPHER_TKIP) {
        return true;
    }

    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to set the status of WSC
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*----------------------------------------------------------------------------*/
void kalP2PSetWscMode(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucWscMode)
{
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PDevInfo);

    prGlueInfo->prP2PDevInfo->ucWSCRunning = ucWscMode;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to get the status of WSC
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*----------------------------------------------------------------------------*/
u8 kalP2PGetWscMode(IN P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PDevInfo);

    return prGlueInfo->prP2PDevInfo->ucWSCRunning;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to get the wsc ie length
 *
 * \param[in]
 *           prGlueInfo
 *           ucType : 0 for beacon, 1 for probe req, 2 for probe resp
 *
 * \return
 *           The WSC IE length
 */
/*----------------------------------------------------------------------------*/
u16 kalP2PCalWSC_IELen(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucType,
                       IN u8 ucRoleIdx)
{
    ASSERT(prGlueInfo);

    ASSERT(ucType < 4);

    return prGlueInfo->prP2PInfo[ucRoleIdx]->u2WSCIELen[ucType];
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to copy the wsc ie setting from p2p supplicant
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           The WPS IE length
 */
/*----------------------------------------------------------------------------*/
void kalP2PGenWSC_IE(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucType,
                     IN u8 *pucBuffer, IN u8 ucRoleIdx)
{
    P_GL_P2P_INFO_T prGlP2pInfo = (P_GL_P2P_INFO_T)NULL;

    do {
        if ((prGlueInfo == NULL) || (ucType >= 4) || (pucBuffer == NULL)) {
            break;
        }

        prGlP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

        kalMemCopy(pucBuffer, prGlP2pInfo->aucWSCIE[ucType],
                   prGlP2pInfo->u2WSCIELen[ucType]);
    } while (false);
}

void kalP2PUpdateWSC_IE(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucType,
                        IN u8 *pucBuffer, IN u16 u2BufferLength,
                        IN u8 ucRoleIdx)
{
    P_GL_P2P_INFO_T prGlP2pInfo = (P_GL_P2P_INFO_T)NULL;

    do {
        if ((prGlueInfo == NULL) || (ucType >= 4) ||
            ((u2BufferLength > 0) && (pucBuffer == NULL))) {
            break;
        }

        if (u2BufferLength > 400) {
            DBGLOG(
                P2P, ERROR,
                "Buffer length is not enough, GLUE only 400 bytes but %d received\n",
                u2BufferLength);
            ASSERT(false);
            break;
        }

        prGlP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

        kalMemCopy(prGlP2pInfo->aucWSCIE[ucType], pucBuffer, u2BufferLength);

        prGlP2pInfo->u2WSCIELen[ucType] = u2BufferLength;
    } while (false);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Indicate an event to supplicant for Service Discovery request from
 * other device.
 *
 * \param[in] prGlueInfo Pointer of GLUE_INFO_T
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalP2PIndicateSDRequest(IN P_GLUE_INFO_T prGlueInfo,
                             IN PARAM_MAC_ADDRESS rPeerAddr, IN u8 ucSeqNum)
{
    union iwreq_data evt;
    u8 aucBuffer[IW_CUSTOM_MAX];

    ASSERT(prGlueInfo);

    memset(&evt, 0, sizeof(evt));

    snprintf(aucBuffer, IW_CUSTOM_MAX - 1, "P2P_SD_REQ %d", ucSeqNum);
    evt.data.length = strlen(aucBuffer);

    /* indicate IWEVP2PSDREQ event */
    wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler, IWEVCUSTOM,
                        &evt, aucBuffer);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Indicate an event to supplicant for Service Discovery response
 *         from other device.
 *
 * \param[in] prGlueInfo Pointer of GLUE_INFO_T
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalP2PIndicateSDResponse(IN P_GLUE_INFO_T prGlueInfo,
                              IN PARAM_MAC_ADDRESS rPeerAddr, IN u8 ucSeqNum)
{
    union iwreq_data evt;
    u8 aucBuffer[IW_CUSTOM_MAX];

    ASSERT(prGlueInfo);

    memset(&evt, 0, sizeof(evt));

    snprintf(aucBuffer, IW_CUSTOM_MAX - 1, "P2P_SD_RESP %d", ucSeqNum);
    evt.data.length = strlen(aucBuffer);

    /* indicate IWEVP2PSDREQ event */
    wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler, IWEVCUSTOM,
                        &evt, aucBuffer);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Indicate an event to supplicant for Service Discovery TX Done
 *         from other device.
 *
 * \param[in] prGlueInfo Pointer of GLUE_INFO_T
 * \param[in] ucSeqNum   Sequence number of the frame
 * \param[in] ucStatus   Status code for TX
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalP2PIndicateTXDone(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucSeqNum,
                          IN u8 ucStatus)
{
    union iwreq_data evt;
    u8 aucBuffer[IW_CUSTOM_MAX];

    ASSERT(prGlueInfo);

    memset(&evt, 0, sizeof(evt));

    snprintf(aucBuffer, IW_CUSTOM_MAX - 1, "P2P_SD_XMITTED: %d %d", ucSeqNum,
             ucStatus);
    evt.data.length = strlen(aucBuffer);

    /* indicate IWEVP2PSDREQ event */
    wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler, IWEVCUSTOM,
                        &evt, aucBuffer);
}

struct net_device *kalP2PGetDevHdlr(P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PInfo[0]);
    return prGlueInfo->prP2PInfo[0]->prDevHandler;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void kalGetChnlList(IN P_GLUE_INFO_T prGlueInfo, IN ENUM_BAND_T eSpecificBand,
                    IN u8 ucMaxChannelNum, IN u8 *pucNumOfChannel,
                    IN P_RF_CHANNEL_INFO_T paucChannelList)
{
    rlmDomainGetChnlList(prGlueInfo->prAdapter, eSpecificBand, false,
                         ucMaxChannelNum, pucNumOfChannel, paucChannelList);
}

/* ////////////////////////////////////ICS
 * SUPPORT////////////////////////////////////// */
void kalP2PIndicateChannelReady(IN P_GLUE_INFO_T prGlueInfo, IN u64 u8SeqNum,
                                IN u32 u4ChannelNum, IN ENUM_BAND_T eBand,
                                IN ENUM_CHNL_EXT_T eSco, IN u32 u4Duration)
{
    struct ieee80211_channel *prIEEE80211ChnlStruct =
        (struct ieee80211_channel *)NULL;
    RF_CHANNEL_INFO_T rChannelInfo;
    enum nl80211_channel_type eChnlType = NL80211_CHAN_NO_HT;

    do {
        if (prGlueInfo == NULL) {
            break;
        }

        kalMemZero(&rChannelInfo, sizeof(RF_CHANNEL_INFO_T));

        rChannelInfo.ucChannelNum = u4ChannelNum;
        rChannelInfo.eBand = eBand;

        prIEEE80211ChnlStruct =
            kalP2pFuncGetChannelEntry(prGlueInfo->prP2PInfo[0], &rChannelInfo);

        kalP2pFuncGetChannelType(eSco, &eChnlType);

        cfg80211_ready_on_channel(
            prGlueInfo->prP2PInfo[0]->prWdev,  /* struct
                                                * wireless_dev, */
            u8SeqNum,  /* u64 cookie, */
            prIEEE80211ChnlStruct,  /* struct ieee80211_channel *
                                     * chan, */
            u4Duration,  /* unsigned int duration, */
            GFP_KERNEL);
        /* gfp_t gfp */ /* allocation flags */
    } while (false);
}

void kalP2PIndicateChannelExpired(IN P_GLUE_INFO_T prGlueInfo, IN u64 u8SeqNum,
                                  IN u32 u4ChannelNum, IN ENUM_BAND_T eBand,
                                  IN ENUM_CHNL_EXT_T eSco)
{
    P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;
    struct ieee80211_channel *prIEEE80211ChnlStruct =
        (struct ieee80211_channel *)NULL;
    enum nl80211_channel_type eChnlType = NL80211_CHAN_NO_HT;
    RF_CHANNEL_INFO_T rRfChannelInfo;

    do {
        if (prGlueInfo == NULL) {
            ASSERT(false);
            break;
        }

        prGlueP2pInfo = prGlueInfo->prP2PInfo[0];

        if (prGlueP2pInfo == NULL) {
            ASSERT(false);
            break;
        }

        DBGLOG(P2P, TRACE, "kalP2PIndicateChannelExpired\n");

        rRfChannelInfo.eBand = eBand;
        rRfChannelInfo.ucChannelNum = u4ChannelNum;

        prIEEE80211ChnlStruct =
            kalP2pFuncGetChannelEntry(prGlueP2pInfo, &rRfChannelInfo);

        kalP2pFuncGetChannelType(eSco, &eChnlType);

        cfg80211_remain_on_channel_expired(
            prGlueP2pInfo->prWdev,  /* struct wireless_dev, */
            u8SeqNum, prIEEE80211ChnlStruct, GFP_KERNEL);
    } while (false);
}

void kalP2PIndicateScanDone(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIndex,
                            IN u8 fgIsAbort)
{
    P_GL_P2P_DEV_INFO_T prP2pGlueDevInfo = (P_GL_P2P_DEV_INFO_T)NULL;
    P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;
    struct cfg80211_scan_request *prScanRequest = NULL;

    GLUE_SPIN_LOCK_DECLARATION();

    do {
        if (prGlueInfo == NULL) {
            ASSERT(false);
            break;
        }

        prGlueP2pInfo = prGlueInfo->prP2PInfo[0];
        prP2pGlueDevInfo = prGlueInfo->prP2PDevInfo;

        if ((prGlueP2pInfo == NULL) || (prP2pGlueDevInfo == NULL)) {
            ASSERT(false);
            break;
        }

        DBGLOG(INIT, INFO, "[p2p] scan complete %p\n",
               prP2pGlueDevInfo->prScanRequest);

        KAL_ACQUIRE_MUTEX(prGlueInfo->prAdapter, MUTEX_DEL_INF);
        GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

        if (prP2pGlueDevInfo->prScanRequest != NULL) {
            prScanRequest = prP2pGlueDevInfo->prScanRequest;
            prP2pGlueDevInfo->prScanRequest = NULL;
        }
        GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

        if ((prScanRequest != NULL) &&
            (prGlueInfo->prAdapter->fgIsP2PRegistered == true)) {
            /* report all queued beacon/probe response frames  to
             * upper layer */
            scanReportBss2Cfg80211(prGlueInfo->prAdapter, BSS_TYPE_P2P_DEVICE,
                                   NULL);

            DBGLOG(INIT, INFO, "DBG:p2p_cfg_scan_done\n");
            kalCfg80211ScanDone(prScanRequest, fgIsAbort);
        }
        KAL_RELEASE_MUTEX(prGlueInfo->prAdapter, MUTEX_DEL_INF);
    } while (false);
}

void kalP2PIndicateBssInfo(IN P_GLUE_INFO_T prGlueInfo, IN u8 *pucFrameBuf,
                           IN u32 u4BufLen,
                           IN P_RF_CHANNEL_INFO_T prChannelInfo,
                           IN s32 i4SignalStrength)
{
    P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;
    struct ieee80211_channel *prChannelEntry = (struct ieee80211_channel *)NULL;
    struct ieee80211_mgmt *prBcnProbeRspFrame =
        (struct ieee80211_mgmt *)pucFrameBuf;
    struct cfg80211_bss *prCfg80211Bss = (struct cfg80211_bss *)NULL;

    do {
        if ((prGlueInfo == NULL) || (pucFrameBuf == NULL) ||
            (prChannelInfo == NULL)) {
            ASSERT(false);
            break;
        }

        prGlueP2pInfo = prGlueInfo->prP2PInfo[0];

        if (prGlueP2pInfo == NULL) {
            ASSERT(false);
            break;
        }

        prChannelEntry =
            kalP2pFuncGetChannelEntry(prGlueP2pInfo, prChannelInfo);

        if (prChannelEntry == NULL) {
            DBGLOG(P2P, TRACE, "Unknown channel info\n");
            break;
        }

        /* rChannelInfo.center_freq =
         * nicChannelNum2Freq((u32)prChannelInfo->ucChannelNum) / 1000;
         */

        prCfg80211Bss = cfg80211_inform_bss_frame(
            prGlueP2pInfo->prWdev->wiphy,  /* struct wiphy * wiphy,
                                            */
            prChannelEntry, prBcnProbeRspFrame, u4BufLen, i4SignalStrength,
            GFP_KERNEL);

        /* Return this structure. */
        cfg80211_put_bss(prGlueP2pInfo->prWdev->wiphy, prCfg80211Bss);
    } while (false);

    return;
}

void kalP2PIndicateMgmtTxStatus(IN P_GLUE_INFO_T prGlueInfo,
                                IN P_MSDU_INFO_T prMsduInfo, IN u8 fgIsAck)
{
    P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;
    u64 *pu8GlCookie = (u64 *)NULL;
    struct net_device *prNetdevice = (struct net_device *)NULL;

    do {
        if ((prGlueInfo == NULL) || (prMsduInfo == NULL)) {
            DBGLOG(P2P, WARN, "Unexpected pointer PARAM. 0x%lx, 0x%lx.\n",
                   prGlueInfo, prMsduInfo);
            ASSERT(false);
            break;
        }

        pu8GlCookie = (u64 *)((unsigned long)prMsduInfo->prPacket +
                              (unsigned long)prMsduInfo->u2FrameLength +
                              MAC_TX_RESERVED_FIELD);

        if (prMsduInfo->ucBssIndex == P2P_DEV_BSS_INDEX) {
            prGlueP2pInfo = prGlueInfo->prP2PInfo[0];

            if (prGlueP2pInfo == NULL) {
                return;
            }

            prNetdevice = prGlueP2pInfo->prDevHandler;
        } else {
            P_BSS_INFO_T prP2pBssInfo = GET_BSS_INFO_BY_INDEX(
                prGlueInfo->prAdapter, prMsduInfo->ucBssIndex);
            prGlueP2pInfo = prGlueInfo->prP2PInfo[prP2pBssInfo->u4PrivateData];

            if (prGlueP2pInfo == NULL) {
                return;
            }

            prNetdevice = prGlueP2pInfo->aprRoleHandler;
        }

        cfg80211_mgmt_tx_status(
            prNetdevice->ieee80211_ptr,  /* struct net_device * dev,
                                          */
            *pu8GlCookie,
            (u8 *)((unsigned long)prMsduInfo->prPacket + MAC_TX_RESERVED_FIELD),
            prMsduInfo->u2FrameLength, fgIsAck, GFP_KERNEL);
    } while (false);
}

void kalP2PIndicateRxMgmtFrame(IN P_GLUE_INFO_T prGlueInfo,
                               IN P_SW_RFB_T prSwRfb, IN u8 fgIsDevInterface,
                               IN u8 ucRoleIdx)
{
#define DBG_P2P_MGMT_FRAME_INDICATION 1
    P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;
    s32 i4Freq = 0;
    u8 ucChnlNum = 0;
#if DBG_P2P_MGMT_FRAME_INDICATION
    P_WLAN_MAC_HEADER_T prWlanHeader = (P_WLAN_MAC_HEADER_T)NULL;
#endif
    struct net_device *prNetdevice = (struct net_device *)NULL;

    do {
        if ((prGlueInfo == NULL) || (prSwRfb == NULL)) {
            ASSERT(false);
            break;
        }

        prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

        /* ToDo[6630]: Get the following by channel freq */
        /* HAL_RX_STATUS_GET_CHAN_FREQ( prSwRfb->prRxStatus) */
        /* ucChnlNum = prSwRfb->prHifRxHdr->ucHwChannelNum; */

        ucChnlNum = HAL_RX_STATUS_GET_CHNL_NUM(prSwRfb->prRxStatus);

#if DBG_P2P_MGMT_FRAME_INDICATION
        prWlanHeader = (P_WLAN_MAC_HEADER_T)prSwRfb->pvHeader;

        switch (prWlanHeader->u2FrameCtrl) {
        case MAC_FRAME_PROBE_REQ:
            DBGLOG(P2P, TRACE, "RX Probe Req at channel %d ", ucChnlNum);
            break;

        case MAC_FRAME_PROBE_RSP:
            DBGLOG(P2P, TRACE, "RX Probe Rsp at channel %d ", ucChnlNum);
            break;

        case MAC_FRAME_ACTION:
            DBGLOG(P2P, TRACE, "RX Action frame at channel %d ", ucChnlNum);
            p2pFuncClassifyAction(prSwRfb);
            break;

        default:
            DBGLOG(P2P, TRACE, "RX Packet:%d at channel %d ",
                   prWlanHeader->u2FrameCtrl, ucChnlNum);
            break;
        }

        DBGLOG(P2P, TRACE, "from: " MACSTR "\n",
               MAC2STR(prWlanHeader->aucAddr2));
#endif
        i4Freq = nicChannelNum2Freq(ucChnlNum) / 1000;

        if (fgIsDevInterface) {
            prNetdevice = prGlueP2pInfo->prDevHandler;
        } else {
            prNetdevice = prGlueP2pInfo->aprRoleHandler;
        }

        cfg80211_rx_mgmt(
            prNetdevice->ieee80211_ptr,  /* struct
                                         * net_device *
                                         * dev, */
            i4Freq,
            RCPI_TO_dBm(nicRxGetRcpiValueFromRxv(RCPI_MODE_WF0, prSwRfb)),
            prSwRfb->pvHeader, prSwRfb->u2PacketLen,
            NL80211_RXMGMT_FLAG_ANSWERED);
    } while (false);
}

void kalP2PGCIndicateConnectionStatus(
    IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIndex,
    IN P_P2P_CONNECTION_REQ_INFO_T prP2pConnInfo, IN u8 *pucRxIEBuf,
    IN u16 u2RxIELen, IN u16 u2StatusReason, IN WLAN_STATUS eStatus)
{
    P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;

    do {
        if (prGlueInfo == NULL) {
            ASSERT(false);
            break;
        }

        prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIndex];

        /* This exception occurs at wlanRemove. */
        if ((prGlueP2pInfo == NULL) ||
            (prGlueP2pInfo->aprRoleHandler == NULL) ||
            (prGlueInfo->prAdapter->rP2PNetRegState !=
             ENUM_NET_REG_STATE_REGISTERED) ||
            ((prGlueInfo->ulFlag & GLUE_FLAG_HALT) == 1)) {
            break;
        }

        if (prP2pConnInfo) {
            /* switch netif on */
            netif_carrier_on(prGlueP2pInfo->aprRoleHandler);

            cfg80211_connect_result(prGlueP2pInfo->aprRoleHandler,
                                    /* struct net_device * dev, */
                                    prP2pConnInfo->aucBssid,
                                    prP2pConnInfo->aucIEBuf,
                                    prP2pConnInfo->u4BufLength, pucRxIEBuf,
                                    u2RxIELen, u2StatusReason, GFP_KERNEL);
            /* gfp_t gfp */ /* allocation flags */

            prP2pConnInfo->eConnRequest = P2P_CONNECTION_TYPE_IDLE;
        } else {
            DBGLOG(INIT, INFO,
                   "indicate disconnection event to kernel, reason=%d, "
                   "locally_generated=%d\n",
                   u2StatusReason,
                   eStatus == WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY);
            /* Disconnect, what if u2StatusReason == 0? */
            cfg80211_disconnected(
                prGlueP2pInfo->aprRoleHandler,
                /* struct net_device * dev, */
                u2StatusReason, pucRxIEBuf, u2RxIELen,
                eStatus == WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY, GFP_KERNEL);
        }
    } while (false);
}

void kalP2PGOStationUpdate(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIndex,
                           IN P_STA_RECORD_T prCliStaRec, IN u8 fgIsNew)
{
    P_GL_P2P_INFO_T prP2pGlueInfo = (P_GL_P2P_INFO_T)NULL;

    do {
        if ((prGlueInfo == NULL) || (prCliStaRec == NULL) ||
            (ucRoleIndex >= 2)) {
            break;
        }

        prP2pGlueInfo = prGlueInfo->prP2PInfo[ucRoleIndex];

        if ((prP2pGlueInfo == NULL) ||
            (prP2pGlueInfo->aprRoleHandler == NULL)) {
            /* This case may occur when the usb is unplugged */
            break;
        }

        if (fgIsNew) {
            struct station_info rStationInfo;

            kalMemZero(&rStationInfo, sizeof(rStationInfo));

            rStationInfo.generation = ++prP2pGlueInfo->i4Generation;
            rStationInfo.assoc_req_ies = prCliStaRec->pucAssocReqIe;
            rStationInfo.assoc_req_ies_len = prCliStaRec->u2AssocReqIeLen;

            cfg80211_new_sta(prP2pGlueInfo->aprRoleHandler,
                             /* struct net_device * dev, */
                             prCliStaRec->aucMacAddr, &rStationInfo,
                             GFP_KERNEL);
        } else {
            ++prP2pGlueInfo->i4Generation;

            /* The exception occurs at wlanRemove */
            if ((prP2pGlueInfo != NULL) &&
                (prP2pGlueInfo->aprRoleHandler != NULL) &&
                (prGlueInfo->prAdapter->rP2PNetRegState ==
                 ENUM_NET_REG_STATE_REGISTERED) &&
                ((prGlueInfo->ulFlag & GLUE_FLAG_HALT) == 0)) {
                cfg80211_del_sta(prP2pGlueInfo->aprRoleHandler,
                                 prCliStaRec->aucMacAddr, GFP_KERNEL);
            }
        }
    } while (false);

    return;
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
void kalP2PRddDetectUpdate(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIndex)
{
    DBGLOG(INIT, INFO, "Radar Detection event\n");

    do {
        if (prGlueInfo == NULL) {
            ASSERT(false);
            break;
        }

        if (prGlueInfo->prP2PInfo[ucRoleIndex]->chandef == NULL) {
            ASSERT(false);
            break;
        }

        /* cac start disable for next cac slot if enable in dfs channel
         */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0)
        prGlueInfo->prP2PInfo[ucRoleIndex]->prWdev->links[0].cac_started =
            false;
#else
        prGlueInfo->prP2PInfo[ucRoleIndex]->prWdev->cac_started = false;
#endif
        DBGLOG(INIT, INFO, "kalP2PRddDetectUpdate: Update to OS\n");
        cfg80211_radar_event(prGlueInfo->prP2PInfo[ucRoleIndex]->prWdev->wiphy,
                             prGlueInfo->prP2PInfo[ucRoleIndex]->chandef,
                             GFP_KERNEL);
        DBGLOG(INIT, INFO, "kalP2PRddDetectUpdate: Update to OS Done\n");

        /* NL80211 event should send to p2p group netdevice.
         * Otherwise wpa_supplicant wouldn't perform beacon update.
         * Hostapd case: prDevHandler same with aprRoleHandler
         * P2P GO case: p2p0=>prDevHandler, p2p-xxx-x=> aprRoleHandler
         */
        netif_carrier_off(prGlueInfo->prP2PInfo[ucRoleIndex]->aprRoleHandler);
        netif_tx_stop_all_queues(
            prGlueInfo->prP2PInfo[ucRoleIndex]->aprRoleHandler);

        if (prGlueInfo->prP2PInfo[ucRoleIndex]->chandef->chan) {
            cnmMemFree(prGlueInfo->prAdapter,
                       prGlueInfo->prP2PInfo[ucRoleIndex]->chandef->chan);
        }

        prGlueInfo->prP2PInfo[ucRoleIndex]->chandef->chan = NULL;

        if (prGlueInfo->prP2PInfo[ucRoleIndex]->chandef) {
            cnmMemFree(prGlueInfo->prAdapter,
                       prGlueInfo->prP2PInfo[ucRoleIndex]->chandef);
        }

        prGlueInfo->prP2PInfo[ucRoleIndex]->chandef = NULL;
    } while (false);
}

void kalP2PCacFinishedUpdate(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIndex)
{
    DBGLOG(INIT, INFO, "CAC Finished event\n");

    if (prGlueInfo == NULL) {
        return;
    }

    if (prGlueInfo->prP2PInfo[ucRoleIndex]->chandef == NULL) {
        return;
    }

    /* NL80211 event should send to p2p group netdevice.
     * Otherwise wpa_supplicant wouldn't perform beacon update.
     * Hostapd case: prDevHandler same with aprRoleHandler
     * P2P GO case: p2p0=>prDevHandler, p2p-xxx-x=> aprRoleHandler
     */
    DBGLOG(INIT, INFO, "kalP2PCacFinishedUpdate: Update to OS\n");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0)
    cfg80211_cac_event(prGlueInfo->prP2PInfo[ucRoleIndex]->prDevHandler,
                       prGlueInfo->prP2PInfo[ucRoleIndex]->chandef,
                       NL80211_RADAR_CAC_FINISHED, GFP_KERNEL, 0);
#else
    cfg80211_cac_event(prGlueInfo->prP2PInfo[ucRoleIndex]->aprRoleHandler,
                       prGlueInfo->prP2PInfo[ucRoleIndex]->chandef,
                       NL80211_RADAR_CAC_FINISHED, GFP_KERNEL);
#endif

    DBGLOG(INIT, INFO, "kalP2PCacFinishedUpdate: Update to OS Done\n");
}
#endif

u8 kalP2pFuncGetChannelType(IN ENUM_CHNL_EXT_T rChnlSco,
                            OUT enum nl80211_channel_type *channel_type)
{
    u8 fgIsValid = false;

    do {
        if (channel_type) {
            switch (rChnlSco) {
            case CHNL_EXT_SCN:
                *channel_type = NL80211_CHAN_NO_HT;
                break;

            case CHNL_EXT_SCA:
                *channel_type = NL80211_CHAN_HT40MINUS;
                break;

            case CHNL_EXT_SCB:
                *channel_type = NL80211_CHAN_HT40PLUS;
                break;

            default:
                ASSERT(false);
                *channel_type = NL80211_CHAN_NO_HT;
                break;
            }
        }

        fgIsValid = true;
    } while (false);

    return fgIsValid;
}

struct ieee80211_channel *
kalP2pFuncGetChannelEntry(IN P_GL_P2P_INFO_T prP2pInfo,
                          IN P_RF_CHANNEL_INFO_T prChannelInfo)
{
    struct ieee80211_channel *prTargetChannelEntry =
        (struct ieee80211_channel *)NULL;
    struct wiphy *wiphy = (struct wiphy *)NULL;
    u32 u4TblSize = 0, u4Idx = 0;

    if ((prP2pInfo == NULL) || (prChannelInfo == NULL)) {
        return NULL;
    }

    wiphy = prP2pInfo->prWdev->wiphy;

    do {
        switch (prChannelInfo->eBand) {
        case BAND_2G4:
            prTargetChannelEntry = wiphy->bands[NL80211_BAND_2GHZ]->channels;
            u4TblSize = wiphy->bands[NL80211_BAND_2GHZ]->n_channels;
            break;

        case BAND_5G:
            prTargetChannelEntry = wiphy->bands[NL80211_BAND_5GHZ]->channels;
            u4TblSize = wiphy->bands[NL80211_BAND_5GHZ]->n_channels;
            break;

        default:
            break;
        }

        if (prTargetChannelEntry == NULL) {
            break;
        }

        for (u4Idx = 0; u4Idx < u4TblSize; u4Idx++, prTargetChannelEntry++) {
            if (prTargetChannelEntry->hw_value == prChannelInfo->ucChannelNum) {
                break;
            }
        }

        if (u4Idx == u4TblSize) {
            prTargetChannelEntry = NULL;
            break;
        }
    } while (false);

    return prTargetChannelEntry;
}

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER

/*----------------------------------------------------------------------------*/
/*!
 * \brief to set the block list of Hotspot
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*----------------------------------------------------------------------------*/
u8 kalP2PSetBlackList(IN P_GLUE_INFO_T prGlueInfo, IN PARAM_MAC_ADDRESS rbssid,
                      IN u8 fgIsblock, IN u8 ucRoleIndex)
{
    u8 aucNullAddr[] = NULL_MAC_ADDR;
    u8 fgIsValid = false;
    u32 i;

    ASSERT(prGlueInfo);
    /*ASSERT(prGlueInfo->prP2PInfo[ucRoleIndex]);*/

    /*if only one ap mode register, prGlueInfo->prP2PInfo[1] would be null*/
    if (!prGlueInfo->prP2PInfo[ucRoleIndex]) {
        return fgIsValid;
    }

    if (fgIsblock) {
        for (i = 0; i < P2P_MAXIMUM_CLIENT_COUNT; i++) {
            if (UNEQUAL_MAC_ADDR(rbssid, aucNullAddr)) {
                if (UNEQUAL_MAC_ADDR(&(prGlueInfo->prP2PInfo[ucRoleIndex]
                                       ->aucblackMACList[i]),
                                     rbssid)) {
                    if (EQUAL_MAC_ADDR(&(prGlueInfo->prP2PInfo[ucRoleIndex]
                                         ->aucblackMACList[i]),
                                       aucNullAddr)) {
                        COPY_MAC_ADDR(&(prGlueInfo->prP2PInfo[ucRoleIndex]
                                        ->aucblackMACList[i]),
                                      rbssid);
                        fgIsValid = false;
                        return fgIsValid;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < P2P_MAXIMUM_CLIENT_COUNT; i++) {
            if (EQUAL_MAC_ADDR(
                    &(prGlueInfo->prP2PInfo[ucRoleIndex]->aucblackMACList[i]),
                    rbssid)) {
                COPY_MAC_ADDR(
                    &(prGlueInfo->prP2PInfo[ucRoleIndex]->aucblackMACList[i]),
                    aucNullAddr);
                fgIsValid = false;
                return fgIsValid;
            }
        }
    }

    return fgIsValid;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to compare the black list of Hotspot
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*----------------------------------------------------------------------------*/
u8 kalP2PCmpBlackList(IN P_GLUE_INFO_T prGlueInfo, IN PARAM_MAC_ADDRESS rbssid,
                      IN u8 ucRoleIndex)
{
    u8 aucNullAddr[] = NULL_MAC_ADDR;
    u8 fgIsExsit = false;
    u32 i;

    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PInfo[ucRoleIndex]);

    for (i = 0; i < P2P_MAXIMUM_CLIENT_COUNT; i++) {
        if (UNEQUAL_MAC_ADDR(rbssid, aucNullAddr)) {
            if (EQUAL_MAC_ADDR(
                    &(prGlueInfo->prP2PInfo[ucRoleIndex]->aucblackMACList[i]),
                    rbssid)) {
                fgIsExsit = true;
                return fgIsExsit;
            }
        }
    }

    return fgIsExsit;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to return the max clients of Hotspot
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*----------------------------------------------------------------------------*/
void kalP2PSetMaxClients(IN P_GLUE_INFO_T prGlueInfo, IN u32 u4MaxClient,
                         IN u8 ucRoleIndex)
{
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PInfo[ucRoleIndex]);

    if (u4MaxClient == 0 || prGlueInfo->prP2PInfo[ucRoleIndex]->ucMaxClients >=
        P2P_MAXIMUM_CLIENT_COUNT) {
        prGlueInfo->prP2PInfo[ucRoleIndex]->ucMaxClients =
            P2P_MAXIMUM_CLIENT_COUNT;
    } else {
        prGlueInfo->prP2PInfo[ucRoleIndex]->ucMaxClients = u4MaxClient;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to return the max clients of Hotspot
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*----------------------------------------------------------------------------*/
u8 kalP2PMaxClients(IN P_GLUE_INFO_T prGlueInfo, IN u32 u4NumClient,
                    IN u8 ucRoleIndex)
{
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prP2PInfo[ucRoleIndex]);

    if (prGlueInfo->prP2PInfo[ucRoleIndex]->ucMaxClients) {
        if ((u8)u4NumClient >
            prGlueInfo->prP2PInfo[ucRoleIndex]->ucMaxClients) {
            return true;
        } else {
            return false;
        }
    }

    return false;
}

#endif
