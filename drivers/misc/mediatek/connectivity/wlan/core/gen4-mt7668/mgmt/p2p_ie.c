// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */
#include "precomp.h"

u32 p2pCalculate_IEForAssocReq(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                               IN P_STA_RECORD_T prStaRec)
{
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_BSS_INFO_T prP2pBssInfo = (P_BSS_INFO_T)NULL;
    P_P2P_CONNECTION_REQ_INFO_T prConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;
    u32 u4RetValue = 0;

    do {
        ASSERT_BREAK((prStaRec != NULL) && (prAdapter != NULL));

        prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

        prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
            prAdapter, (u8)prP2pBssInfo->u4PrivateData);

        prConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);

        u4RetValue = prConnReqInfo->u4BufLength;

        /* ADD WMM Information Element */
        u4RetValue += (ELEM_HDR_LEN + ELEM_MAX_LEN_WMM_INFO);

        /* ADD HT Capability */
        if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet &
             PHY_TYPE_SET_802_11N) &&
            (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N)) {
            u4RetValue += (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP);
        }
#if CFG_SUPPORT_802_11AC
        /* ADD VHT Capability */
        if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet &
             PHY_TYPE_SET_802_11AC) &&
            (prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11AC)) {
            u4RetValue += (ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_CAP);
        }
#endif

#if CFG_SUPPORT_MTK_SYNERGY
        if (prAdapter->rWifiVar.ucMtkOui == FEATURE_ENABLED) {
            u4RetValue += (ELEM_HDR_LEN + ELEM_MIN_LEN_MTK_OUI);
        }
#endif
    } while (false);

    return u4RetValue;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to generate P2P IE for Beacon frame.
 *
 * @param[in] prMsduInfo             Pointer to the composed MSDU_INFO_T.
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void p2pGenerate_IEForAssocReq(IN P_ADAPTER_T prAdapter,
                               IN P_MSDU_INFO_T prMsduInfo)
{
    P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T)NULL;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_P2P_CONNECTION_REQ_INFO_T prConnReqInfo =
        (P_P2P_CONNECTION_REQ_INFO_T)NULL;
    u8 *pucIEBuf = (u8 *)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (prMsduInfo != NULL));

        prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
                                          prMsduInfo->ucBssIndex);

        prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
            prAdapter, (u8)prBssInfo->u4PrivateData);

        prConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);

        pucIEBuf = (u8 *)((unsigned long)prMsduInfo->prPacket +
                          (unsigned long)prMsduInfo->u2FrameLength);

        kalMemCopy(pucIEBuf, prConnReqInfo->aucIEBuf,
                   prConnReqInfo->u4BufLength);

        prMsduInfo->u2FrameLength += prConnReqInfo->u4BufLength;

        /* Add WMM IE */
        mqmGenerateWmmInfoIE(prAdapter, prMsduInfo);

        /* Add HT IE */
        rlmReqGenerateHtCapIE(prAdapter, prMsduInfo);

#if CFG_SUPPORT_802_11AC
        /* Add VHT IE */
        rlmReqGenerateVhtCapIE(prAdapter, prMsduInfo);
        rlmReqGenerateVhtOpNotificationIE(prAdapter, prMsduInfo);
#endif

#if CFG_SUPPORT_MTK_SYNERGY
        rlmGenerateMTKOuiIE(prAdapter, prMsduInfo);
#endif
    } while (false);

    return;
}

u32 wfdFuncAppendAttriDevInfo(IN P_ADAPTER_T prAdapter, IN u8 fgIsAssocFrame,
                              IN u16 *pu2Offset, IN u8 *pucBuf,
                              IN u16 u2BufSize)
{
    u32 u4AttriLen = 0;
    u8 *pucBuffer = NULL;
    P_WFD_DEVICE_INFORMATION_IE_T prWfdDevInfo =
        (P_WFD_DEVICE_INFORMATION_IE_T)NULL;
    P_WFD_CFG_SETTINGS_T prWfdCfgSettings = (P_WFD_CFG_SETTINGS_T)NULL;

    do {
        ASSERT_BREAK((prAdapter != NULL) && (pucBuf != NULL) &&
                     (pu2Offset != NULL));

        prWfdCfgSettings = &(prAdapter->rWifiVar.rWfdConfigureSettings);

        ASSERT_BREAK((prWfdCfgSettings != NULL));

        if ((prWfdCfgSettings->ucWfdEnable == 0) ||
            ((prWfdCfgSettings->u4WfdFlag & WFD_FLAGS_DEV_INFO_VALID) ==
             0)) {
            break;
        }

        pucBuffer = (u8 *)((unsigned long)pucBuf +
                           (unsigned long)(*pu2Offset));

        ASSERT_BREAK(pucBuffer != NULL);

        prWfdDevInfo = (P_WFD_DEVICE_INFORMATION_IE_T)pucBuffer;

        prWfdDevInfo->ucElemID = WFD_ATTRI_ID_DEV_INFO;

        WLAN_SET_FIELD_BE16(&prWfdDevInfo->u2WfdDevInfo,
                            prWfdCfgSettings->u2WfdDevInfo);

        WLAN_SET_FIELD_BE16(&prWfdDevInfo->u2SessionMgmtCtrlPort,
                            prWfdCfgSettings->u2WfdControlPort);

        WLAN_SET_FIELD_BE16(&prWfdDevInfo->u2WfdDevMaxSpeed,
                            prWfdCfgSettings->u2WfdMaximumTp);

        WLAN_SET_FIELD_BE16(&prWfdDevInfo->u2Length,
                            WFD_ATTRI_MAX_LEN_DEV_INFO);

        u4AttriLen = WFD_ATTRI_MAX_LEN_DEV_INFO + WFD_ATTRI_HDR_LEN;
    } while (false);

    (*pu2Offset) += (u16)u4AttriLen;

    return u4AttriLen;
}

/* wfdFuncAppendAttriDevInfo */
