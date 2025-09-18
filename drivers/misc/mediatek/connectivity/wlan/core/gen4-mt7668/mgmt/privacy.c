// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "privacy.c"
 *    \brief  This file including the protocol layer privacy function.
 *
 *    This file provided the macros and functions library support for the
 *    protocol layer security setting from rsn.c and nic_privacy.c
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

#include "precomp.h"

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
 * \brief This routine is called to initialize the privacy-related
 *        parameters.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucNetTypeIdx  Pointer to netowrk type index
 *
 * \retval NONE
 */
/*----------------------------------------------------------------------------*/
void secInit(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex)
{
    u8 i;
    P_CONNECTION_SETTINGS_T prConnSettings;
    P_BSS_INFO_T prBssInfo;
    P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo;

    DEBUGFUNC("secInit");

    ASSERT(prAdapter);

    prConnSettings = &prAdapter->rWifiVar.rConnSettings;
    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
    prAisSpecBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;

    prBssInfo->u4RsnSelectedGroupCipher = 0;
    prBssInfo->u4RsnSelectedPairwiseCipher = 0;
    prBssInfo->u4RsnSelectedAKMSuite = 0;

    prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[0]
    .dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_WEP40;
    prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[1]
    .dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_TKIP;
    prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[2]
    .dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_CCMP;
    prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[3]
    .dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_WEP104;

    prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[4]
    .dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_WEP40;
    prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[5]
    .dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_TKIP;
    prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[6]
    .dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_CCMP;
    prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[7]
    .dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_WEP104;
    prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[8]
    .dot11RSNAConfigPairwiseCipher =
        RSN_CIPHER_SUITE_GROUP_NOT_USED;
    prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[9]
    .dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_GCMP_256;

    for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
        prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[i]
        .dot11RSNAConfigPairwiseCipherEnabled = false;

    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[0]
    .dot11RSNAConfigAuthenticationSuite = WPA_AKM_SUITE_NONE;
    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[1]
    .dot11RSNAConfigAuthenticationSuite = WPA_AKM_SUITE_802_1X;
    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[2]
    .dot11RSNAConfigAuthenticationSuite = WPA_AKM_SUITE_PSK;
    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[3]
    .dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_NONE;
    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[4]
    .dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_802_1X;
    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[5]
    .dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_PSK;

    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[6]
    .dot11RSNAConfigAuthenticationSuite =
        RSN_AKM_SUITE_802_1X_SHA256;
    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[7]
    .dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_PSK_SHA256;
    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[8]
    .dot11RSNAConfigAuthenticationSuite =
        RSN_AKM_SUITE_8021X_SUITE_B;
    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[9]
    .dot11RSNAConfigAuthenticationSuite =
        RSN_AKM_SUITE_8021X_SUITE_B_192;
    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[10]
    .dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_SAE;
    prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[11]
    .dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_OWE;

    for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++)
        prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[i]
        .dot11RSNAConfigAuthenticationSuiteEnabled = false;

    secClearPmkid(prAdapter);

    cnmTimerInitTimer(prAdapter, &prAisSpecBssInfo->rPreauthenticationTimer,
                      (PFN_MGMT_TIMEOUT_FUNC)rsnIndicatePmkidCand,
                      (unsigned long)NULL);

    cnmTimerInitTimer(prAdapter, &prAisSpecBssInfo->rSaQueryTimer,
                      (PFN_MGMT_TIMEOUT_FUNC)rsnStartSaQueryTimer,
                      (unsigned long)NULL);

    prAisSpecBssInfo->fgCounterMeasure = false;
    prAdapter->prAisBssInfo->ucBcDefaultKeyIdx = 0xff;
    prAdapter->prAisBssInfo->fgBcDefaultKeyExist = false;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will indicate an Event of "Rx Class Error" to SEC_FSM
 * for JOIN Module.
 *
 * \param[in] prAdapter     Pointer to the Adapter structure
 * \param[in] prSwRfb       Pointer to the SW RFB.
 *
 * \return false                Class Error
 */
/*----------------------------------------------------------------------------*/
u8 secCheckClassError(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb,
                      IN P_STA_RECORD_T prStaRec)
{
    P_HW_MAC_RX_DESC_T prRxStatus;

    ASSERT(prAdapter);
    ASSERT(prSwRfb);

    prRxStatus = prSwRfb->prRxStatus;

    if (!prStaRec ||
        (prRxStatus->u2StatusFlag & RXS_DW2_RX_CLASSERR_BITMAP) ==
        RXS_DW2_RX_CLASSERR_VALUE) {
        DBGLOG(RSN, ERROR,
               "prStaRec=%x RX Status = %x RX_CLASSERR check!\n",
               prStaRec, prRxStatus->u2StatusFlag);

        /* if (IS_NET_ACTIVE(prAdapter, ucBssIndex)) { */
        authSendDeauthFrame(prAdapter, NULL, NULL, prSwRfb,
                            REASON_CODE_CLASS_3_ERR,
                            (PFN_TX_DONE_HANDLER)NULL);
        return false;
        /* } */
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to handle The Rx Security process MSDU.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] prSWRfb SW rfb pinter
 *
 * \retval true Accept the packet
 * \retval false Refuse the MSDU packet due port control
 */
/*----------------------------------------------------------------------------*/
u8 secRxPortControlCheck(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSWRfb)
{
    ASSERT(prSWRfb);
    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine will enable/disable the cipher suite
 *
 * \param[in] prAdapter Pointer to the adapter object data area.
 * \param[in] u4CipherSuitesFlags flag for cipher suite
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void secSetCipherSuite(IN P_ADAPTER_T prAdapter, IN u32 u4CipherSuitesFlags)
{
    u32 i;
    P_DOT11_RSNA_CONFIG_PAIRWISE_CIPHERS_ENTRY prEntry;
    P_IEEE_802_11_MIB_T prMib;

    ASSERT(prAdapter);

    prMib = &prAdapter->rMib;

    ASSERT(prMib);

    if (u4CipherSuitesFlags == CIPHER_FLAG_NONE) {
        /* Disable all the pairwise cipher suites. */
        for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
            prMib->dot11RSNAConfigPairwiseCiphersTable[i]
            .dot11RSNAConfigPairwiseCipherEnabled = false;

        /* Update the group cipher suite. */
        prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_NONE;

        return;
    }

    for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++) {
        prEntry = &prMib->dot11RSNAConfigPairwiseCiphersTable[i];

        switch (prEntry->dot11RSNAConfigPairwiseCipher) {
#if CFG_SUPPORT_SUITB
        case RSN_CIPHER_SUITE_GCMP_256:
            if (u4CipherSuitesFlags & CIPHER_FLAG_GCMP256) {
                prEntry->dot11RSNAConfigPairwiseCipherEnabled =
                    true;
            } else {
                prEntry->dot11RSNAConfigPairwiseCipherEnabled =
                    false;
            }
            break;

#endif
        case WPA_CIPHER_SUITE_WEP40:
        case RSN_CIPHER_SUITE_WEP40:
            if (u4CipherSuitesFlags & CIPHER_FLAG_WEP40) {
                prEntry->dot11RSNAConfigPairwiseCipherEnabled =
                    true;
            } else {
                prEntry->dot11RSNAConfigPairwiseCipherEnabled =
                    false;
            }
            break;

        case WPA_CIPHER_SUITE_TKIP:
        case RSN_CIPHER_SUITE_TKIP:
            if (u4CipherSuitesFlags & CIPHER_FLAG_TKIP) {
                prEntry->dot11RSNAConfigPairwiseCipherEnabled =
                    true;
            } else {
                prEntry->dot11RSNAConfigPairwiseCipherEnabled =
                    false;
            }
            break;

        case WPA_CIPHER_SUITE_CCMP:
        case RSN_CIPHER_SUITE_CCMP:
            if (u4CipherSuitesFlags & CIPHER_FLAG_CCMP) {
                prEntry->dot11RSNAConfigPairwiseCipherEnabled =
                    true;
            } else {
                prEntry->dot11RSNAConfigPairwiseCipherEnabled =
                    false;
            }
            break;

        case WPA_CIPHER_SUITE_WEP104:
        case RSN_CIPHER_SUITE_WEP104:
            if (u4CipherSuitesFlags & CIPHER_FLAG_WEP104) {
                prEntry->dot11RSNAConfigPairwiseCipherEnabled =
                    true;
            } else {
                prEntry->dot11RSNAConfigPairwiseCipherEnabled =
                    false;
            }
            break;

        default:
            break;
        }
    }

    /* Update the group cipher suite. */
    if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_CCMP, &i)) {
        prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_CCMP;
    } else if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_TKIP,
                                        &i)) {
        prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_TKIP;
    } else if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_WEP104,
                                        &i)) {
        prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_WEP104;
    } else if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_WEP40,
                                        &i)) {
        prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_WEP40;
    }
#if CFG_SUPPORT_SUITB
    else if (rsnSearchSupportedCipher(
                 prAdapter, RSN_CIPHER_SUITE_GROUP_NOT_USED, &i)) {
        prMib->dot11RSNAConfigGroupCipher =
            RSN_CIPHER_SUITE_GROUP_NOT_USED;
    } else if (rsnSearchSupportedCipher(prAdapter,
                                        RSN_CIPHER_SUITE_GCMP_256, &i)) {
        prMib->dot11RSNAConfigGroupCipher = RSN_CIPHER_SUITE_GCMP_256;
    }
#endif
    else {
        prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_NONE;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to initialize the pmkid parameters.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 *
 * \retval NONE
 */
/*----------------------------------------------------------------------------*/
void secClearPmkid(IN P_ADAPTER_T prAdapter)
{
    P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo;

    DEBUGFUNC("secClearPmkid");

    prAisSpecBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;
    DBGLOG(RSN, TRACE, "secClearPmkid\n");
    prAisSpecBssInfo->u4PmkidCandicateCount = 0;
    prAisSpecBssInfo->u4PmkidCacheCount = 0;
    kalMemZero((void *)prAisSpecBssInfo->arPmkidCandicate,
               sizeof(PMKID_CANDICATE_T) * CFG_MAX_PMKID_CACHE);
    kalMemZero((void *)prAisSpecBssInfo->arPmkidCache,
               sizeof(PMKID_ENTRY_T) * CFG_MAX_PMKID_CACHE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Whether 802.11 privacy is enabled.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 *
 * \retval u8
 */
/*----------------------------------------------------------------------------*/
u8 secEnabledInAis(IN P_ADAPTER_T prAdapter)
{
    DEBUGFUNC("secEnabledInAis");

    ASSERT(prAdapter->rWifiVar.rConnSettings.eEncStatus <
           ENUM_ENCRYPTION_NUM);

    if ((prAdapter->rWifiVar.rConnSettings.eEncStatus ==
         ENUM_ENCRYPTION1_ENABLED) ||
        (prAdapter->rWifiVar.rConnSettings.eEncStatus ==
         ENUM_ENCRYPTION2_ENABLED) ||
        (prAdapter->rWifiVar.rConnSettings.eEncStatus ==
         ENUM_ENCRYPTION3_ENABLED)
#if CFG_SUPPORT_SUITB
        || (prAdapter->rWifiVar.rConnSettings.eEncStatus ==
            ENUM_ENCRYPTION4_ENABLED)
#endif
        ) {
        return true;
    } else if ((prAdapter->rWifiVar.rConnSettings.eEncStatus ==
                ENUM_ENCRYPTION_DISABLED)) {
        DBGLOG(RSN, TRACE, "Unknown encryption setting %d\n",
               prAdapter->rWifiVar.rConnSettings.eEncStatus);
    }
    return false;
}

u8 secIsProtected1xFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
    P_BSS_INFO_T prBssInfo;

    ASSERT(prAdapter);

    if (prStaRec) {
        prBssInfo =
            GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
        if (prBssInfo && prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
        }

        return prStaRec->fgTransmitKeyExist;
    }
    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the privacy bit at mac header for TxM
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] prMsdu the msdu for known the sta record
 *
 * \return true the privacy need to set
 *            false the privacy no need to set
 */
/*----------------------------------------------------------------------------*/
u8 secIsProtectedFrame(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsdu,
                       IN P_STA_RECORD_T prStaRec)
{
    ASSERT(prAdapter);
    ASSERT(prMsdu);

    if (prMsdu->ucPacketType == TX_PACKET_TYPE_MGMT) {
        return false;
    }

    return secIsProtectedBss(prAdapter,
                             GET_BSS_INFO_BY_INDEX(prAdapter,
                                                   prMsdu->ucBssIndex));
}

u8 secIsProtectedBss(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
    ASSERT(prBssInfo);

    if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
        return secEnabledInAis(prAdapter);
    }
#if CFG_ENABLE_WIFI_DIRECT
    else if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
        return kalP2PGetCipher(prAdapter->prGlueInfo,
                               (u8)prBssInfo->u4PrivateData);
    }
#endif
    else if (prBssInfo->eNetworkType == NETWORK_TYPE_BOW) {
        return true;
    }

    ASSERT(false);
    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used before add/update a WLAN entry.
 *        Info the WLAN Table has available entry for this request
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in]  prSta the P_STA_RECORD_T for store
 *
 * \return true Free Wlan table is reserved for this request
 *            false No free entry for this request
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
u8 secPrivacySeekForEntry(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
    P_BSS_INFO_T prP2pBssInfo;
    u8 ucEntry = WTBL_RESERVED_ENTRY;
    u8 i;
    u8 ucStartIDX = 0, ucMaxIDX = 0;
    P_WLAN_TABLE_T prWtbl;
    u8 ucRoleIdx = 0;

    ASSERT(prSta);

    if (!prSta->fgIsInUse) {
        ASSERT(false);
    }

    prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prSta->ucBssIndex);
    ucRoleIdx = prP2pBssInfo->u4PrivateData;

    prWtbl = prAdapter->rWifiVar.arWtbl;

    ucStartIDX = 1;
    ucMaxIDX = NIC_TX_DEFAULT_WLAN_INDEX - 1;

    DBGLOG(RSN, INFO, "secPrivacySeekForEntry\n");

    for (i = ucStartIDX; i <= ucMaxIDX; i++) {
        if (prWtbl[i].ucUsed &&
            EQUAL_MAC_ADDR(prSta->aucMacAddr, prWtbl[i].aucMacAddr) &&
            prWtbl[i].ucPairwise  /* This function for ucPairwise only */ ) {
            ucEntry = i;
            DBGLOG(RSN, INFO, "[Wlan index]: Reuse entry #%d\n", i);
            break;
        }
    }

    if (i == (ucMaxIDX + 1)) {
        for (i = ucStartIDX; i <= ucMaxIDX; i++) {
            if (prWtbl[i].ucUsed == false) {
                ucEntry = i;
                DBGLOG(RSN, INFO,
                       "[Wlan index]: Assign entry #%d\n", i);
                break;
            }
        }
    }

    /* Save to the driver maintain table */
    if (ucEntry < NIC_TX_DEFAULT_WLAN_INDEX) {
        prWtbl[ucEntry].ucUsed = true;
        prWtbl[ucEntry].ucBssIndex = prSta->ucBssIndex;
        prWtbl[ucEntry].ucKeyId = 0xFF;
        prWtbl[ucEntry].ucPairwise = 1;
        COPY_MAC_ADDR(prWtbl[ucEntry].aucMacAddr, prSta->aucMacAddr);
        prWtbl[ucEntry].ucStaIndex = prSta->ucIndex;

        prSta->ucWlanIndex = ucEntry;

        {
            P_BSS_INFO_T prBssInfo = GET_BSS_INFO_BY_INDEX(
                prAdapter, prSta->ucBssIndex);
            /* for AP mode , if wep key exist, peer sta should also
             * fgTransmitKeyExist */
            if (IS_BSS_P2P(prBssInfo) &&
                kalP2PGetRole(prAdapter->prGlueInfo, ucRoleIdx) ==
                2) {
                if (prBssInfo->fgBcDefaultKeyExist &&
                    !(kalP2PGetCcmpCipher(prAdapter->prGlueInfo,
                                          ucRoleIdx) ||
                      kalP2PGetTkipCipher(prAdapter->prGlueInfo,
                                          ucRoleIdx))) {
                    prSta->fgTransmitKeyExist = true;
                    prWtbl[ucEntry].ucKeyId =
                        prBssInfo->ucBcDefaultKeyIdx;
                    DBGLOG(RSN,
                           INFO,
                           "peer sta set fgTransmitKeyExist\n");
                }
            }
        }

        DBGLOG(RSN,
               INFO,
               "[Wlan index] BSS#%d keyid#%d P=%d use WlanIndex#%d STAIdx=%d "
               MACSTR
               " staType=%x\n",
               prSta->ucBssIndex,
               0,
               prWtbl[ucEntry].ucPairwise,
               ucEntry,
               prSta->ucIndex,
               MAC2STR(prSta->aucMacAddr),
               prSta->eStaType);

        secCheckWTBLAssign(prAdapter);

        return true;
    }
#if DBG
    secCheckWTBLAssign(prAdapter);
#endif
    DBGLOG(RSN, WARN,
           "[Wlan index] No more wlan table entry available!!!!\n");
    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used free a WLAN entry.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in]  ucEntry the wlan table index to free
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void secPrivacyFreeForEntry(IN P_ADAPTER_T prAdapter, IN u8 ucEntry)
{
    P_WLAN_TABLE_T prWtbl;

    ASSERT(prAdapter);

    if (ucEntry >= WTBL_SIZE) {
        return;
    }

    DBGLOG(RSN, INFO, "secPrivacyFreeForEntry %d", ucEntry);

    prWtbl = prAdapter->rWifiVar.arWtbl;

    if (prWtbl[ucEntry].ucUsed) {
        prWtbl[ucEntry].ucUsed = false;
        prWtbl[ucEntry].ucKeyId = 0xff;
        prWtbl[ucEntry].ucBssIndex = MAX_BSS_INDEX + 1;
        prWtbl[ucEntry].ucPairwise = 0;
        kalMemZero(prWtbl[ucEntry].aucMacAddr, MAC_ADDR_LEN);
        prWtbl[ucEntry].ucStaIndex = STA_REC_INDEX_NOT_FOUND;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used free a STA WLAN entry.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in]  prStaRec the sta which want to free
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void secPrivacyFreeSta(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
    u32 entry;
    P_WLAN_TABLE_T prWtbl;

    if (!prStaRec) {
        return;
    }

    prWtbl = prAdapter->rWifiVar.arWtbl;

    for (entry = 0; entry < WTBL_SIZE; entry++) {
        /* Consider GTK case !! */
        if (prWtbl[entry].ucUsed &&
            EQUAL_MAC_ADDR(prStaRec->aucMacAddr,
                           prWtbl[entry].aucMacAddr) &&
            prWtbl[entry].ucPairwise) {
            DBGLOG(RSN, INFO, "Free STA entry (%lu)!\n", entry);

            secPrivacyFreeForEntry(prAdapter, entry);
            prStaRec->ucWlanIndex = WTBL_RESERVED_ENTRY;
            /* prStaRec->ucBMCWlanIndex = WTBL_RESERVED_ENTRY; */
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear the group WEP key
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucBssIndex The BSS index
 * \param[in] u4KeyId The key index
 *
 * \note
 */
/*----------------------------------------------------------------------------*/

static inline u32 secRemoveBmcWepKey(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                                     IN u32 u4KeyId)
{
    PARAM_REMOVE_KEY_T rRemoveKey;
    u32 u4SetLen = 0;
    u32 u4Ret;

    DBGLOG(RSN, INFO, "BssIdx=%d, KeyId=%d\n", ucBssIndex, u4KeyId);

    if (prAdapter->rAcpiState == ACPI_STATE_D3) {
        DBGLOG(REQ,
               WARN,
               "Fail in set remove WEP! (Adapter not ready). ACPI=D%d, Radio=%d\n",
               prAdapter->rAcpiState,
               prAdapter->fgIsRadioOff);
        return WLAN_STATUS_ADAPTER_NOT_READY;
    }

    if (u4KeyId > MAX_KEY_NUM - 1) {
        DBGLOG(REQ, ERROR, "invalid WEP key ID %u\n", u4KeyId);
        return WLAN_STATUS_INVALID_DATA;
    }

    kalMemZero(&rRemoveKey, sizeof(PARAM_REMOVE_KEY_T));
    rRemoveKey.u4Length = sizeof(PARAM_REMOVE_KEY_T);
    rRemoveKey.u4KeyIndex = u4KeyId;
    rRemoveKey.ucBssIdx = ucBssIndex;
    /* Should set FLAG_RM_KEY_CTRL_WO_OID for not OID operation */
    rRemoveKey.ucCtrlFlag = FLAG_RM_KEY_CTRL_WO_OID;

    u4Ret = wlanoidSetRemoveKey(prAdapter, (void *)&rRemoveKey,
                                sizeof(PARAM_REMOVE_KEY_T), &u4SetLen);

    if (u4Ret != WLAN_STATUS_PENDING) {
        DBGLOG(RSN, WARN, "Can't send remove bmc wep key cmd\n");
    }

    return u4Ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used for remove the BC entry of the BSS
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucBssIndex The BSS index
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
void secRemoveBssBcEntry(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo,
                         IN u8 fgRoam)
{
    int i;
    P_CONNECTION_SETTINGS_T prConnSettings =
        &(prAdapter->rWifiVar.rConnSettings);

    if (!prBssInfo) {
        return;
    }

    DBGLOG(RSN, INFO, "remove all the key related with BSS!");

    if (fgRoam) {
        if (IS_BSS_AIS(prBssInfo) && prBssInfo->prStaRecOfAP &&
            (prConnSettings->eAuthMode >= AUTH_MODE_WPA &&
             prConnSettings->eAuthMode != AUTH_MODE_WPA_NONE)) {
            for (i = 0; i < MAX_KEY_NUM; i++) {
                if (prBssInfo->ucBMCWlanIndexSUsed[i]) {
                    secPrivacyFreeForEntry(
                        prAdapter,
                        prBssInfo->ucBMCWlanIndexS[i]);
                }
            }
            prBssInfo->fgBcDefaultKeyExist = false;
            prBssInfo->ucBcDefaultKeyIdx = 0xff;
        }
    } else {
        prBssInfo->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;
        secPrivacyFreeForEntry(prAdapter, prBssInfo->ucBMCWlanIndex);

        for (i = 0; i < MAX_KEY_NUM; i++) {
            if (prBssInfo->wepkeyUsed[i] == false) {
                continue;
            }
            /* remove key to avoid that cfg80211_del_key is called
             * after nicDeactivateNetwork.
             */
            secRemoveBmcWepKey(prAdapter, prBssInfo->ucBssIndex, i);
            prBssInfo->wepkeyUsed[i] = false;
        }
        /* wlanoidSetRemoveKey would clear prBssInfo->wepkeyUsed[],
         * but won't call secPrivacyFreeForEntry.
         * For the case that the cfg80211_del_key is called before
         * nicDeactivateNetwork, check wepkeyWlanIdx to do
         * secPrivacyFreeForEntry.
         */
        if (prBssInfo->wepkeyWlanIdx != WTBL_RESERVED_ENTRY) {
            secPrivacyFreeForEntry(prAdapter,
                                   prBssInfo->wepkeyWlanIdx);
        }

        prBssInfo->wepkeyWlanIdx = WTBL_RESERVED_ENTRY;
        prBssInfo->fgBcDefaultKeyExist = false;
        prBssInfo->ucBcDefaultKeyIdx = 0xff;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used for adding the broadcast key used, to assign a
 * wlan table entry for reserved the specific entry for these key for
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucBssIndex The BSS index
 * \param[in] ucNetTypeIdx The Network index
 * \param[in] ucAlg the entry assign related with algorithm
 * \param[in] ucKeyId The key id
 * \param[in] ucTxRx The Type of the key
 *
 * \return ucEntryIndex The entry to be used, WTBL_ALLOC_FAIL for allocation
 * fail
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
u8 secPrivacySeekForBcEntry(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex,
                            IN u8 *pucAddr, IN u8 ucStaIdx, IN u8 ucAlg,
                            IN u8 ucKeyId)
{
    u8 ucEntry = WTBL_ALLOC_FAIL;
    u8 ucStartIDX = 0, ucMaxIDX = 0;
    u8 i = 0;
    u8 fgCheckKeyId = true;
    P_WLAN_TABLE_T prWtbl;
    P_BSS_INFO_T prBSSInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

    prWtbl = prAdapter->rWifiVar.arWtbl;
    ASSERT(prAdapter);
    ASSERT(pucAddr);

    if (ucAlg == CIPHER_SUITE_WPI ||  /* CIPHER_SUITE_GCM_WPI || */
        ucAlg == CIPHER_SUITE_WEP40 || ucAlg == CIPHER_SUITE_WEP104 ||
        ucAlg == CIPHER_SUITE_WEP128 || ucAlg == CIPHER_SUITE_NONE) {
        fgCheckKeyId = false;
    }

    if (ucKeyId == 0xFF || ucAlg == CIPHER_SUITE_BIP) {
        fgCheckKeyId = false;
    }

    if (prBSSInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
        fgCheckKeyId = false;
    }

    ucStartIDX = 1;
    ucMaxIDX = NIC_TX_DEFAULT_WLAN_INDEX - 1;

    /*always use index 0 for BIP*/
    if (ucAlg == CIPHER_SUITE_BIP) {
        ucEntry = 0;
    } else {
        for (i = ucStartIDX; i <= ucMaxIDX; i++) {
            if (prWtbl[i].ucUsed && !prWtbl[i].ucPairwise &&
                prWtbl[i].ucBssIndex == ucBssIndex) {
                if (!fgCheckKeyId) {
                    ucEntry = i;
                    DBGLOG(RSN,
                           INFO,
                           "[Wlan index]: Reuse entry #%d for open/wep/wpi\n",
                           i);
                    break;
                }

                if (fgCheckKeyId &&
                    (prWtbl[i].ucKeyId == ucKeyId ||
                     prWtbl[i].ucKeyId == 0xFF)) {
                    ucEntry = i;
                    DBGLOG(RSN,
                           INFO,
                           "[Wlan index]: Reuse entry #%d\n",
                           i);
                    break;
                }
            }
        }
    }

    if (i == (ucMaxIDX + 1)) {
        for (i = ucStartIDX; i <= ucMaxIDX; i++) {
            if (prWtbl[i].ucUsed == false) {
                ucEntry = i;
                DBGLOG(RSN, INFO,
                       "[Wlan index]: Assign entry #%d\n", i);
                break;
            }
        }
    }

    if (ucEntry < NIC_TX_DEFAULT_WLAN_INDEX) {
        prWtbl[ucEntry].ucUsed = true;
        prWtbl[ucEntry].ucKeyId = ucKeyId;
        prWtbl[ucEntry].ucBssIndex = ucBssIndex;
        prWtbl[ucEntry].ucPairwise = 0;
        kalMemCopy(prWtbl[ucEntry].aucMacAddr, pucAddr, MAC_ADDR_LEN);
        prWtbl[ucEntry].ucStaIndex = ucStaIdx;

        DBGLOG(RSN,
               INFO,
               "[Wlan index] BSS#%d keyid#%d P=%d use WlanIndex#%d STAIdx=%d "
               MACSTR
               "\n",
               ucBssIndex,
               ucKeyId,
               prWtbl[ucEntry].ucPairwise,
               ucEntry,
               ucStaIdx,
               MAC2STR(pucAddr));

        /* DBG */
        secCheckWTBLAssign(prAdapter);
    } else {
        secCheckWTBLAssign(prAdapter);
        DBGLOG(RSN, ERROR,
               "[Wlan index] No more wlan entry available!!!!\n");
    }

    return ucEntry;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 *
 * \return ucEntryIndex The entry to be used, WTBL_ALLOC_FAIL for allocation
 * fail
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
u8 secCheckWTBLAssign(IN P_ADAPTER_T prAdapter)
{
    u8 fgCheckFail = false;

    secPrivacyDumpWTBL(prAdapter);

    /* AIS STA should just has max 2 entry */
    /* Max STA check */
    if (fgCheckFail) {
        ASSERT(false);
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Got the STA record index by wlan index
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucWlanIdx The Rx wlan index
 *
 * \return The STA record index, 0xff for invalid sta index
 */
/*----------------------------------------------------------------------------*/
u8 secGetStaIdxByWlanIdx(P_ADAPTER_T prAdapter, u8 ucWlanIdx)
{
    P_WLAN_TABLE_T prWtbl;

    ASSERT(prAdapter);

    if (ucWlanIdx >= WTBL_SIZE) {
        return STA_REC_INDEX_NOT_FOUND;
    }

    prWtbl = prAdapter->rWifiVar.arWtbl;

    /* DBGLOG(RSN, TRACE, ("secGetStaIdxByWlanIdx=%d "MACSTR" used=%d\n",
     * ucWlanIdx, MAC2STR(prWtbl[ucWlanIdx].aucMacAddr),
     * prWtbl[ucWlanIdx].ucUsed));
     */

    if (prWtbl[ucWlanIdx].ucUsed) {
        return prWtbl[ucWlanIdx].ucStaIndex;
    }else{
        return STA_REC_INDEX_NOT_FOUND;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  At Sw wlan table, got the BSS index by wlan index
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucWlanIdx The Rx wlan index
 *
 * \return The BSS index, 0xff for invalid bss index
 */
/*----------------------------------------------------------------------------*/
u8 secGetBssIdxByWlanIdx(P_ADAPTER_T prAdapter, u8 ucWlanIdx)
{
    P_WLAN_TABLE_T prWtbl;

    ASSERT(prAdapter);

    if (ucWlanIdx >= WTBL_SIZE) {
        return WTBL_RESERVED_ENTRY;
    }

    prWtbl = prAdapter->rWifiVar.arWtbl;

    if (prWtbl[ucWlanIdx].ucUsed) {
        return prWtbl[ucWlanIdx].ucBssIndex;
    }else{
        return WTBL_RESERVED_ENTRY;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Got the STA record index by mac addr
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] pucMacAddress MAC Addr
 *
 * \return The STA record index, 0xff for invalid sta index
 */
/*----------------------------------------------------------------------------*/
u8 secLookupStaRecIndexFromTA(P_ADAPTER_T prAdapter, u8 *pucMacAddress)
{
    int i;
    P_WLAN_TABLE_T prWtbl;

    ASSERT(prAdapter);
    prWtbl = prAdapter->rWifiVar.arWtbl;

    for (i = 0; i < WTBL_SIZE; i++) {
        if (prWtbl[i].ucUsed) {
            if (EQUAL_MAC_ADDR(pucMacAddress,
                               prWtbl[i].aucMacAddr) &&
                prWtbl[i].ucPairwise) {
                return prWtbl[i].ucStaIndex;
            }
        }
    }

    return STA_REC_INDEX_NOT_FOUND;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
void secPrivacyDumpWTBL(IN P_ADAPTER_T prAdapter)
{
    P_WLAN_TABLE_T prWtbl;
    u8 i;

    prWtbl = prAdapter->rWifiVar.arWtbl;

    DBGLOG(RSN, INFO, "The Wlan index\n");

    for (i = 0; i < WTBL_SIZE; i++) {
        if (prWtbl[i].ucUsed) {
            DBGLOG(RSN,
                   INFO,
                   "#%d Used=%d  BSSIdx=%d keyid=%d P=%d STA=%d Addr="
                   MACSTR
                   "\n",
                   i,
                   prWtbl[i].ucUsed,
                   prWtbl[i].ucBssIndex,
                   prWtbl[i].ucKeyId,
                   prWtbl[i].ucPairwise,
                   prWtbl[i].ucStaIndex,
                   MAC2STR(prWtbl[i].aucMacAddr));
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Assin the wlan table with the join AP info
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
void secPostUpdateAddr(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
    P_CONNECTION_SETTINGS_T prConnSettings =
        &(prAdapter->rWifiVar.rConnSettings);
    P_WLAN_TABLE_T prWtbl;

    if (IS_BSS_AIS(prBssInfo) && prBssInfo->prStaRecOfAP) {
        if (prConnSettings->eEncStatus == ENUM_ENCRYPTION1_ENABLED) {
            if (prBssInfo->fgBcDefaultKeyExist) {
                prWtbl = &prAdapter->rWifiVar.arWtbl
                         [prBssInfo->wepkeyWlanIdx];

                kalMemCopy(prWtbl->aucMacAddr,
                           prBssInfo->prStaRecOfAP->aucMacAddr,
                           MAC_ADDR_LEN);
                prWtbl->ucStaIndex =
                    prBssInfo->prStaRecOfAP->ucIndex;
                DBGLOG(RSN, INFO,
                       "secPostUpdateAddr at [%d] " MACSTR
                       "= STA Index=%d\n",
                       prBssInfo->wepkeyWlanIdx,
                       MAC2STR(prWtbl->aucMacAddr),
                       prBssInfo->prStaRecOfAP->ucIndex);

                /* Update the wlan table of the prStaRecOfAP */
                prWtbl =
                    &prAdapter->rWifiVar
                    .arWtbl[prBssInfo->prStaRecOfAP
                            ->ucWlanIndex];
                prWtbl->ucKeyId = prBssInfo->ucBcDefaultKeyIdx;
                prBssInfo->prStaRecOfAP->fgTransmitKeyExist =
                    true;
            }
        }
        if (prConnSettings->eEncStatus == ENUM_ENCRYPTION_DISABLED) {
            prWtbl = &prAdapter->rWifiVar
                     .arWtbl[prBssInfo->ucBMCWlanIndex];

            kalMemCopy(prWtbl->aucMacAddr,
                       prBssInfo->prStaRecOfAP->aucMacAddr,
                       MAC_ADDR_LEN);
            prWtbl->ucStaIndex = prBssInfo->prStaRecOfAP->ucIndex;
            DBGLOG(RSN, INFO,
                   "secPostUpdateAddr at [%d] " MACSTR
                   "= STA Index=%d\n",
                   prBssInfo->ucBMCWlanIndex,
                   MAC2STR(prWtbl->aucMacAddr),
                   prBssInfo->prStaRecOfAP->ucIndex);
        }
    }
}
