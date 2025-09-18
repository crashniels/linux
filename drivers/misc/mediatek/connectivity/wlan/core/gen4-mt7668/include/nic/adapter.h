/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   adapter.h
 *    \brief  Definition of internal data structure for driver manipulation.
 *
 *    In this file we define the internal data structure - ADAPTER_T which
 * stands for MiniPort ADAPTER(From Windows point of view) or stands for Network
 * ADAPTER.
 */

#ifndef _ADAPTER_H
#define _ADAPTER_H

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

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

enum {
    ENUM_SW_TEST_MODE_NONE                    = 0,
    ENUM_SW_TEST_MODE_SIGMA_AC                = 0x1,
    ENUM_SW_TEST_MODE_SIGMA_WFD               = 0x2,
    ENUM_SW_TEST_MODE_CTIA                    = 0x3,
    ENUM_SW_TEST_MODE_SIGMA_TDLS              = 0x4,
    ENUM_SW_TEST_MODE_SIGMA_P2P               = 0x5,
    ENUM_SW_TEST_MODE_SIGMA_N                 = 0x6,
    ENUM_SW_TEST_MODE_SIGMA_HS20_R1           = 0x7,
    ENUM_SW_TEST_MODE_SIGMA_HS20_R2           = 0x8,
    ENUM_SW_TEST_MODE_SIGMA_PMF               = 0x9,
    ENUM_SW_TEST_MODE_SIGMA_WMMPS             = 0xA,
    ENUM_SW_TEST_MODE_SIGMA_AC_R2             = 0xB,
    ENUM_SW_TEST_MODE_SIGMA_NAN               = 0xC,
    ENUM_SW_TEST_MODE_SIGMA_AC_AP             = 0xD,
    ENUM_SW_TEST_MODE_SIGMA_N_AP              = 0xE,
    ENUM_SW_TEST_MODE_SIGMA_WFDS              = 0xF,
    ENUM_SW_TEST_MODE_SIGMA_WFD_R2            = 0x10,
    ENUM_SW_TEST_MODE_SIGMA_LOCATION          = 0x11,
    ENUM_SW_TEST_MODE_SIGMA_TIMING_MANAGEMENT = 0x12,
    ENUM_SW_TEST_MODE_SIGMA_WMMAC             = 0x13,
    ENUM_SW_TEST_MODE_SIGMA_VOICE_ENT         = 0x14
};

typedef struct _WLAN_INFO_T {
    PARAM_BSSID_EX_T rCurrBssId;

    /* Scan Result */
    PARAM_BSSID_EX_T arScanResult[CFG_MAX_NUM_BSS_LIST];
    u8 *apucScanResultIEs[CFG_MAX_NUM_BSS_LIST];
    u32 u4ScanResultNum;

    /* IE pool for Scanning Result */
    u8 aucScanIEBuf[CFG_MAX_COMMON_IE_BUF_LEN];
    u32 u4ScanIEBufferUsage;

    u32 u4SysTime;

    /* connection parameter (for Ad-Hoc) */
    u16 u2BeaconPeriod;
    u16 u2AtimWindow;

    PARAM_RATES eDesiredRates;
    CMD_LINK_ATTRIB eLinkAttr;
    /* CMD_PS_PROFILE_T         ePowerSaveMode; */
    CMD_PS_PROFILE_T arPowerSaveMode[BSS_INFO_NUM];

    /* trigger parameter */
    ENUM_RSSI_TRIGGER_TYPE eRssiTriggerType;
    PARAM_RSSI rRssiTriggerValue;

    /* Privacy Filter */
    ENUM_PARAM_PRIVACY_FILTER_T ePrivacyFilter;

    /* RTS Threshold */
    PARAM_RTS_THRESHOLD eRtsThreshold;

    /* Network Type */
    u8 ucNetworkType;

    /* Network Type In Use */
    u8 ucNetworkTypeInUse;
} WLAN_INFO_T, *P_WLAN_INFO_T;

/* Session for CONNECTION SETTINGS */
typedef struct _CONNECTION_SETTINGS_T {
    u8 aucMacAddress[MAC_ADDR_LEN];

    u8 ucDelayTimeOfDisconnectEvent;

    u8 fgIsConnByBssidIssued;
    u8 aucBSSID[MAC_ADDR_LEN];

    u8 fgIsConnReqIssued;
    u8 fgIsDisconnectedByNonRequest;

    u8 ucSSIDLen;
    u8 aucSSID[ELEM_MAX_LEN_SSID];

    ENUM_PARAM_OP_MODE_T eOPMode;

    ENUM_PARAM_CONNECTION_POLICY_T eConnectionPolicy;

    ENUM_PARAM_AD_HOC_MODE_T eAdHocMode;

    ENUM_PARAM_AUTH_MODE_T eAuthMode;

    ENUM_PARAM_ENCRYPTION_STATUS_T eEncStatus;

    u8 fgIsScanReqIssued;

    /* MIB attributes */
    u16 u2BeaconPeriod;

    u16 u2RTSThreshold;  /* User desired setting */

    u16 u2DesiredNonHTRateSet;  /* User desired setting */

    u8 ucAdHocChannelNum;  /* For AdHoc */

    ENUM_BAND_T eAdHocBand;  /* For AdHoc */

    u32 u4FreqInKHz;  /* Center frequency */

    /* ATIM windows using for IBSS power saving function */
    u16 u2AtimWindow;

    /* Features */
    u8 fgIsEnableRoaming;

    u8 fgIsAdHocQoSEnable;

    ENUM_PARAM_PHY_CONFIG_T eDesiredPhyConfig;

    /* Used for AP mode for desired channel and bandwidth */
    u16 u2CountryCode;
    u8 uc2G4BandwidthMode;  /* 20/40M or 20M only */ /* Not used */
    u8 uc5GBandwidthMode;  /* 20/40M or 20M only */ /* Not used */

#if CFG_SUPPORT_802_11D
    u8 fgMultiDomainCapabilityEnabled;
#endif

    u8 fgWapiMode;
    u32 u4WapiSelectedGroupCipher;
    u32 u4WapiSelectedPairwiseCipher;
    u32 u4WapiSelectedAKMSuite;

    /* CR1486, CR1640 */
    /* for WPS, disable the privacy check for AP selection policy */
    u8 fgPrivacyCheckDisable;

    /* b0~3: trigger-en AC0~3. b4~7: delivery-en AC0~3 */
    u8 bmfgApsdEnAc;

    /* for RSN info store, when upper layer set rsn info */
    RSN_INFO_T rRsnInfo;

    struct cfg80211_bss *bss;

    u8 fgIsConnInitialized;

    u8 fgIsSendAssoc;

    u8 ucAuthDataLen;
    /* Temp assign a fixed large number
     * Additional elements for Authentication frame,
     * starts with the Authentication transaction sequence number field
     */
    u8 aucAuthData[AUTH_DATA_MAX_LEN];
    u8 ucChannelNum;

#if CFG_SUPPORT_OWE
    /* for OWE info store, when upper layer set rsn info */
    struct OWE_INFO_T rOweInfo;
#endif

#if CFG_SUPPORT_H2E
    struct RSNXE rRsnXE;
#endif
} CONNECTION_SETTINGS_T, *P_CONNECTION_SETTINGS_T;

struct _BSS_INFO_T {
    ENUM_NETWORK_TYPE_T eNetworkType;

    u32 u4PrivateData;  /* Private data parameter for each NETWORK type
                         * usage. */
    /* P2P network type has 3 network interface to distinguish. */

    ENUM_PARAM_MEDIA_STATE_T
        eConnectionState;  /* Connected Flag used in AIS_NORMAL_TR */
    ENUM_PARAM_MEDIA_STATE_T
        eConnectionStateIndicated;  /* The Media State that report to HOST */

    ENUM_OP_MODE_T eCurrentOPMode;  /* Current Operation Mode - Infra/IBSS */
#if CFG_ENABLE_WIFI_DIRECT
    ENUM_OP_MODE_T eIntendOPMode;
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
    u8 fgIsDfsActive;
#endif

#if CFG_SUPPORT_DBDC_TC6
    u8 fgReConnBypassScan;
    /* Ignore stale state of target BSS
     * during DBDC sta reconnect searching to bypass scan
     */
#endif

    u8 fgIsInUse;  /* For CNM to assign BSS_INFO */
    u8 fgIsNetActive;  /* true if this network has been activated */

    u8 ucBssIndex;  /* BSS_INFO_T index */

    u8 ucReasonOfDisconnect;  /* Used by media state indication */

    u8 ucSSIDLen;  /* Length of SSID */

#if CFG_ENABLE_WIFI_DIRECT
    ENUM_HIDDEN_SSID_TYPE_T eHiddenSsidType;  /* For Hidden SSID usage. */
#endif

    u8 aucSSID[ELEM_MAX_LEN_SSID];  /* SSID used in this BSS */

    u8 aucBSSID[MAC_ADDR_LEN];  /* The BSSID of the associated BSS */

    u8 aucOwnMacAddr[MAC_ADDR_LEN];  /* Owned MAC Address used in this BSS */

    u8 ucOwnMacIndex;  /* Owned MAC index used in this BSS */

    P_STA_RECORD_T prStaRecOfAP;  /* For Infra Mode, and valid only if
                                   * eConnectionState ==
                                   * MEDIA_STATE_CONNECTED
                                   */
    LINK_T
        rStaRecOfClientList;  /* For IBSS/AP Mode, all known STAs in current BSS
                               */

    u8 ucBMCWlanIndex;  /* For open Mode, BC/MC Tx wlan index, For STA, BC/MC
                         * Rx wlan index */

    u8 ucBMCWlanIndexS[MAX_KEY_NUM];  /* For AP Mode, BC/MC Tx wlan index,
                                       * For STA, BC/MC Rx wlan index */
    u8 ucBMCWlanIndexSUsed[MAX_KEY_NUM];

    u8 fgBcDefaultKeyExist;  /* Bc Transmit key exist or not */
    u8 ucBcDefaultKeyIdx;  /* Bc default key idx, for STA, the Rx just set,
                            * for AP, the tx key id */

    u8 wepkeyUsed[MAX_KEY_NUM];
    u8 wepkeyWlanIdx;  /* wlan index of the wep key */

    u16 u2CapInfo;  /* Change Detection */

    u16 u2BeaconInterval;  /* The Beacon Interval of this BSS */

    u16 u2ATIMWindow;  /* For IBSS Mode */

    u16 u2AssocId;  /* For Infra Mode, it is the Assoc ID assigned by AP.
                     */

    u8 ucDTIMPeriod;  /* For Infra/AP Mode */

    u8 ucDTIMCount;  /* For AP Mode, it is the DTIM value we should carried
                      * in the Beacon of next TBTT.
                      */

    u8 ucPhyTypeSet;  /* Available PHY Type Set of this peer
                       * (This is deduced from received BSS_DESC_T)
                       */

    u8 ucNonHTBasicPhyType;  /* The Basic PHY Type Index, used to setup Phy
                              * Capability */

    u8 ucConfigAdHocAPMode;  /* The configuration of AdHoc/AP Mode. e.g. 11g
                              * or 11b */

    u8 ucBeaconTimeoutCount;  /* For Infra/AP Mode, it is a threshold of
                               * Beacon Lost Count to confirm connection was
                               * lost
                               */

    u8 fgHoldSameBssidForIBSS;  /* For IBSS Mode, to keep use same BSSID to
                                 * extend the life cycle of an IBSS */

    u8 fgIsBeaconActivated;  /* For AP/IBSS Mode, it is used to indicate that
                              * Beacon is sending */

    P_MSDU_INFO_T prBeacon;  /* For AP/IBSS Mode - Beacon Frame */

    u8 fgIsIBSSMaster;  /* For IBSS Mode - To indicate that we can reply
                         * ProbeResp Frame. In current TBTT interval
                         */

    u8 fgIsShortPreambleAllowed;  /* From Capability Info. of AssocResp Frame
                                   * AND of Beacon/ProbeResp Frame
                                   */
    u8 fgUseShortPreamble;  /* Short Preamble is enabled in current BSS. */
    u8 fgUseShortSlotTime;  /* Short Slot Time is enabled in current BSS. */

    u16 u2OperationalRateSet;  /* Operational Rate Set of current BSS */
    u16 u2BSSBasicRateSet;  /* Basic Rate Set of current BSS */

    u8 ucAllSupportedRatesLen;  /* Used for composing Beacon Frame in AdHoc
                                 * or AP Mode */
    u8 aucAllSupportedRates[RATE_NUM_SW];

    u8 ucAssocClientCnt;  /* TODO(Kevin): Number of associated clients */

    u8 fgIsProtection;
    u8 fgIsQBSS;
    /* fgIsWmmBSS; */ /* For Infra/AP/IBSS Mode, it is used to indicate if
     * we support WMM in current BSS.
     */
    u8 fgIsNetAbsent;  /* true: BSS is absent, false: BSS is present */

    u8 fgIsWepCipherGroup;
    u32 u4RsnSelectedGroupCipher;
    u32 u4RsnSelectedPairwiseCipher;
    u32 u4RsnSelectedAKMSuite;
    u16 u2RsnSelectedCapInfo;

    u8 ucOpChangeChannelWidth;  /* The OpMode channel width that we want to
                                 * change to*/
    /* 0:20MHz, 1:40MHz, 2:80MHz, 3:160MHz 4:80+80MHz */
    u8 fgIsOpChangeChannelWidth;

    /*------------------------------------------------------------------------*/
    /* Power Management related information */
    /*------------------------------------------------------------------------*/
    PM_PROFILE_SETUP_INFO_T rPmProfSetupInfo;

    /*------------------------------------------------------------------------*/
    /* WMM/QoS related information */
    /*------------------------------------------------------------------------*/
    u8 ucWmmParamSetCount;  /* Used to detect the change of EDCA parameters.
                             * For AP mode, the value is used in WMM IE
                             */

    AC_QUE_PARMS_T arACQueParms[WMM_AC_INDEX_NUM];

    u8 aucCWminLog2ForBcast[WMM_AC_INDEX_NUM];  /* For AP mode, broadcast the
                                                 * CWminLog2 */
    u8 aucCWmaxLog2ForBcast[WMM_AC_INDEX_NUM];  /* For AP mode, broadcast the
                                                 * CWmaxLog2 */
    AC_QUE_PARMS_T
        arACQueParmsForBcast[WMM_AC_INDEX_NUM];  /* For AP mode, broadcast the
                                                  * value */
    u8 ucWmmQueSet;
#if (CFG_HW_WMM_BY_BSS == 1)
    u8 fgIsWmmInited;
#endif

    /*------------------------------------------------------------------------*/
    /* 802.11n HT operation IE when (prStaRec->ucPhyTypeSet &
     * PHY_TYPE_BIT_HT) */
    /* is true. They have the same definition with fields of */
    /* information element (CM) */
    /*------------------------------------------------------------------------*/
    ENUM_BAND_T eBand;
    u8 ucPrimaryChannel;
    u8 ucHtOpInfo1;
    u8 ucHtPeerOpInfo1;  /*Backup peer HT OP Info*/
    u16 u2HtOpInfo2;
    u16 u2HtOpInfo3;
    u8 ucNss;
    /*------------------------------------------------------------------------*/
    /* 802.11ac VHT operation IE when (prStaRec->ucPhyTypeSet &
     * PHY_TYPE_BIT_VHT) */
    /* is true. They have the same definition with fields of */
    /* information element (EASON) */
    /*------------------------------------------------------------------------*/

    u8 ucVhtChannelWidth;
    u8 ucVhtChannelFrequencyS1;
    u8 ucVhtChannelFrequencyS2;
    u16 u2VhtBasicMcsSet;

    /* Backup peer VHT OpInfo */
    u8 ucVhtPeerChannelWidth;
    u8 ucVhtPeerChannelFrequencyS1;
    u8 ucVhtPeerChannelFrequencyS2;

    /*------------------------------------------------------------------------*/
    /* Required protection modes (CM) */
    /*------------------------------------------------------------------------*/
    u8 fgErpProtectMode;
    ENUM_HT_PROTECT_MODE_T eHtProtectMode;
    ENUM_GF_MODE_T eGfOperationMode;
    ENUM_RIFS_MODE_T eRifsOperationMode;

    u8 fgObssErpProtectMode;  /* GO only */
    ENUM_HT_PROTECT_MODE_T eObssHtProtectMode;  /* GO only */
    ENUM_GF_MODE_T eObssGfOperationMode;  /* GO only */
    u8 fgObssRifsOperationMode;  /* GO only */

    /*------------------------------------------------------------------------*/
    /* OBSS to decide if 20/40M bandwidth is permitted. */
    /* The first member indicates the following channel list length. */
    /*------------------------------------------------------------------------*/
    u8 fgAssoc40mBwAllowed;
    u8 fg40mBwAllowed;
    ENUM_CHNL_EXT_T eBssSCO;  /* Real setting for HW
                               * 20/40M AP mode will always set 40M,
                               * but its OP IE can be changed.
                               */
    u8 auc2G_20mReqChnlList[CHNL_LIST_SZ_2G + 1];
    u8 auc2G_NonHtChnlList[CHNL_LIST_SZ_2G + 1];
    u8 auc2G_PriChnlList[CHNL_LIST_SZ_2G + 1];
    u8 auc2G_SecChnlList[CHNL_LIST_SZ_2G + 1];

    u8 auc5G_20mReqChnlList[CHNL_LIST_SZ_5G + 1];
    u8 auc5G_NonHtChnlList[CHNL_LIST_SZ_5G + 1];
    u8 auc5G_PriChnlList[CHNL_LIST_SZ_5G + 1];
    u8 auc5G_SecChnlList[CHNL_LIST_SZ_5G + 1];

    TIMER_T rObssScanTimer;
    u16 u2ObssScanInterval;  /* in unit of sec */

    u8 fgObssActionForcedTo20M;  /* GO only */
    u8 fgObssBeaconForcedTo20M;  /* GO only */

#if CFG_SUPPORT_QUIET
    TIMER_T rTxQuietTimer;
    u8 ucQuietPeriod;
    u16 u2QuietOffset;
    u16 u2QuietDuration;
    u8 fgRequestQuietInterval;
    u8 fgIsInQuietInterval;
#endif
    /*------------------------------------------------------------------------*/
    /* HW Related Fields (Kevin) */
    /*------------------------------------------------------------------------*/
    u16 u2HwDefaultFixedRateCode;  /* The default rate code copied to MAC TX
                                    * Desc */
    u16 u2HwLPWakeupGuardTimeUsec;

    u8 ucBssFreeQuota;  /* The value is updated from FW  */

    u16 u2DeauthReason;

#if CFG_SUPPORT_TDLS
    u8 fgTdlsIsProhibited;
    u8 fgTdlsIsChSwProhibited;
#endif
#if CFG_SUPPORT_PNO
    u8 fgIsPNOEnable;
    u8 fgIsNetRequestInActive;
#endif

    WIFI_WMM_AC_STAT_T
        arLinkStatistics[WMM_AC_INDEX_NUM];  /*link layer statistics */

    ENUM_DBDC_BN_T eDBDCBand;

    u32 u4CoexPhyRateLimit;

#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
    u8 ucRoamSkipTimes;
    u8 fgGoodRcpiArea;
    u8 fgPoorRcpiArea;
#endif

    u8 fgIsGranted;
    ENUM_BAND_T eBandGranted;
    u8 ucPrimaryChannelGranted;
    PARAM_CUSTOM_ACL rACL;

#if CFG_SUPPORT_802_11W
    /* AP PMF */
    struct AP_PMF_CFG rApPmfCfg;
    /* STA PMF: for encrypted deauth frame */
    struct completion rDeauthComp;
    u8 encryptedDeauthIsInProcess;
#endif

#if CFG_SUPPORT_REPLAY_DETECTION
    struct SEC_DETECT_REPLAY_INFO rDetRplyInfo;
#endif

    PARAM_POWER_MODE ePowerModeFromUser;

#if CFG_SUPPORT_DFS
    TIMER_T rCsaTimer;
    SWITCH_CH_AND_BAND_PARAMS_T CSAParams;
    u8 fgHasStopTx;
#endif

#if CFG_STR_DHCP_RENEW_OFFLOAD
    u8 fgIsDhcpAcked;
    u8 aucDhcpServerIpAddr[4];
    u32 u4DhcpRenewIntv;  /* DHCP renew offload interval configured by
                           * upper-layer */
#endif
};

struct _NEIGHBOR_AP_T {
    LINK_ENTRY_T rLinkEntry;
    u8 aucBssid[MAC_ADDR_LEN];
    u8 fgHT : 1;
    u8 fgSameMD : 1;
    u8 fgRmEnabled : 1;
    u8 fgFromBtm : 1;
    u8 fgQoS : 1;
    u8 ucReserved : 3;
    u8 fgPrefPresence;
    u8 ucPreference;
    u8 ucChannel;
    u64 u8TermTsf;
};

struct _AIS_SPECIFIC_BSS_INFO_T {
    u8 ucRoamingAuthTypes;  /* This value indicate the roaming type used in
                             * AIS_JOIN */

    u8 fgIsIBSSActive;

    /*! \brief Global flag to let arbiter stay at standby and not connect to
     * any network */
    u8 fgCounterMeasure;
    /* u8                  ucTxWlanIndex; */ /* Legacy wep, adhoc wep wpa
     *                                          Transmit
     * key wlan index */

    /* u8                 fgKeyMaterialExist[4]; */
    /* u8                  aucKeyMaterial[32][4]; */

    /* While Do CounterMeasure procedure, check the EAPoL Error report have
     * send out */
    u8 fgCheckEAPoLTxDone;

    u32 u4RsnaLastMICFailTime;

    /* Stored the current bss wpa rsn cap filed, used for roaming policy */
    /* u16                 u2RsnCap; */
    TIMER_T rPreauthenticationTimer;

    /* By the flow chart of 802.11i,
     *  wait 60 sec before associating to same AP
     *  or roaming to a new AP
     *  or sending data in IBSS,
     *  keep a timer for handle the 60 sec counterMeasure
     */
    TIMER_T rRsnaBlockTrafficTimer;
    TIMER_T rRsnaEAPoLReportTimeoutTimer;

    /* For Keep the Tx/Rx Mic key for TKIP SW Calculate Mic */
    /* This is only one for AIS/AP */
    u8 aucTxMicKey[8];
    u8 aucRxMicKey[8];

    /* Buffer for WPA2 PMKID */
    /* The PMKID cache lifetime is expire by media_disconnect_indication */
    u32 u4PmkidCandicateCount;
    PMKID_CANDICATE_T arPmkidCandicate[CFG_MAX_PMKID_CACHE];
    u32 u4PmkidCacheCount;
    PMKID_ENTRY_T arPmkidCache[CFG_MAX_PMKID_CACHE];
    u8 fgIndicatePMKID;
#if CFG_SUPPORT_802_11W
    u8 fgMgmtProtection;
    u8 fgAPApplyPmfReq;
    u32 u4SaQueryStart;
    u32 u4SaQueryCount;
    u8 ucSaQueryTimedOut;
    u8 *pucSaQueryTransId;
    TIMER_T rSaQueryTimer;
    u8 fgBipKeyInstalled;
#endif
#if CFG_SUPPORT_802_11V
    BSS_TRANSITION_MGT_PARAM_T rBTMParam;
#endif
    LINK_MGMT_T rNeighborApList;
    u32 rNeiApRcvTime;
    u32 u4NeiApValidInterval;
};

struct _BOW_SPECIFIC_BSS_INFO_T {
    u16 u2Reserved;  /* Reserved for Data Type Check */
};

typedef struct _WLAN_TABLE_T {
    u8 ucUsed;
    u8 ucBssIndex;
    u8 ucKeyId;
    u8 ucPairwise;
    u8 aucMacAddr[MAC_ADDR_LEN];
    u8 ucStaIndex;
} WLAN_TABLE_T, *P_WLAN_TABLE_T;

/* Major member variables for WiFi FW operation.
 *  Variables within this region will be ready for access after WIFI function is
 * enabled.
 */
typedef struct _WIFI_VAR_T {
    u8 fgIsRadioOff;

    u8 fgIsEnterD3ReqIssued;

    u8 fgDebugCmdResp;

    CONNECTION_SETTINGS_T rConnSettings;

    SCAN_INFO_T rScanInfo;

#if CFG_SUPPORT_ROAMING
    ROAMING_INFO_T rRoamingInfo;
#endif

    AIS_FSM_INFO_T rAisFsmInfo;

    ENUM_PWR_STATE_T aePwrState[BSS_INFO_NUM + 1];

    BSS_INFO_T arBssInfoPool[BSS_INFO_NUM];

    P2P_DEV_INFO_T rP2pDevInfo;

    AIS_SPECIFIC_BSS_INFO_T rAisSpecificBssInfo;

#if CFG_ENABLE_WIFI_DIRECT
    P_P2P_CONNECTION_SETTINGS_T prP2PConnSettings[BSS_P2P_NUM];

    P_P2P_SPECIFIC_BSS_INFO_T prP2pSpecificBssInfo[BSS_P2P_NUM];

    /* P_P2P_FSM_INFO_T prP2pFsmInfo; */

    P_P2P_DEV_FSM_INFO_T prP2pDevFsmInfo;

    /* Currently we only support 2 p2p interface. */
    P_P2P_ROLE_FSM_INFO_T aprP2pRoleFsmInfo[BSS_P2P_NUM];

    P_PARAM_GET_STA_STATISTICS prP2pQueryStaStatistics[BSS_P2P_NUM];
#endif

    WLAN_TABLE_T arWtbl[WTBL_SIZE];

    DEAUTH_INFO_T arDeauthInfo[MAX_DEAUTH_INFO_COUNT];

    /* Current Wi-Fi Settings and Flags */
    u8 aucPermanentAddress[MAC_ADDR_LEN];
    u8 aucMacAddress[MAC_ADDR_LEN];
    u8 aucDeviceAddress[MAC_ADDR_LEN];
    u8 aucInterfaceAddress[MAC_ADDR_LEN];

    u8 ucAvailablePhyTypeSet;

    ENUM_PHY_TYPE_INDEX_T
        eNonHTBasicPhyType2G4;  /* Basic Phy Type used by SCN according
                                 * to the set of Available PHY Types
                                 */

    ENUM_PARAM_PREAMBLE_TYPE_T ePreambleType;
    ENUM_REGISTRY_FIXED_RATE_T eRateSetting;

    u8 fgIsShortSlotTimeOptionEnable;
    /* User desired setting, but will honor the capability of AP */

    u8 fgEnableJoinToHiddenSSID;
    u8 fgSupportWZCDisassociation;

#if CFG_SUPPORT_WFD
    WFD_CFG_SETTINGS_T rWfdConfigureSettings;
#endif

    u8 aucMediatekOuiIE[64];
    u16 u2MediatekOuiIELen;

    /* Feature Options */
    u8 ucQoS;

    u8 ucStaHt;
    u8 ucStaVht;
    u8 ucApHt;
    u8 ucApVht;
    u8 ucP2pGoHt;
    u8 ucP2pGoVht;
    u8 ucP2pGcHt;
    u8 ucP2pGcVht;

    u8 ucAmpduTx;
    u8 ucAmpduRx;
    u8 ucAmsduInAmpduTx;
    u8 ucAmsduInAmpduRx;
    u8 ucHtAmsduInAmpduTx;
    u8 ucHtAmsduInAmpduRx;
    u8 ucVhtAmsduInAmpduTx;
    u8 ucVhtAmsduInAmpduRx;
    u8 ucTspec;
    u8 ucUapsd;
    u8 ucStaUapsd;
    u8 ucApUapsd;
    u8 ucP2pUapsd;

    u8 ucTxShortGI;
    u8 ucRxShortGI;
    u8 ucTxLdpc;
    u8 ucRxLdpc;
    u8 ucTxStbc;
    u8 ucRxStbc;
    u8 ucRxStbcNss;
    u8 ucTxGf;
    u8 ucRxGf;

    u8 ucMCS32;

    u8 ucTxopPsTx;
    u8 ucSigTaRts;
    u8 ucDynBwRts;

    u8 ucStaHtBfee;
    u8 ucStaVhtBfee;
    u8 ucStaHtBfer;
    u8 ucStaVhtBfer;
    u8 ucStaVhtMuBfee;

    u8 ucDataTxDone;
    u8 ucDataTxRateMode;
    u32 u4DataTxRateCode;

    u8 ucApWpsMode;
    u8 ucApChannel;

    u8 ucApSco;
    u8 ucP2pGoSco;

    u8 ucStaBandwidth;
    u8 ucSta5gBandwidth;
    u8 ucSta2gBandwidth;
    u8 ucApBandwidth;
    u8 ucAp2gBandwidth;
    u8 ucAp5gBandwidth;
    u8 ucP2p5gBandwidth;
    u8 ucP2p2gBandwidth;

    /* If enable, AP channel bandwidth Channel Center Frequency Segment 0/1
     */
    /* and secondary channel offset will align wifi.cfg */
    /* Otherwise align cfg80211 */
    u8 ucApChnlDefFromCfg;

    u8 ucNSS;

    u8 ucAp5gNSS;  /* Less or euqal than ucNss */
    u8 ucAp2gNSS;  /* Less or euqal than ucNss */
    u8 ucGo5gNSS;  /* Less or euqal than ucNss */
    u8 ucGo2gNSS;  /* Less or euqal than ucNss */

    u8 ucRxMaxMpduLen;
    u32 u4TxMaxAmsduInAmpduLen;

    u8 ucTxBaSize;
    u8 ucRxHtBaSize;
    u8 ucRxVhtBaSize;

    u8 ucStaDisconnectDetectTh;
    u8 ucApDisconnectDetectTh;
    u8 ucP2pDisconnectDetectTh;

    u8 ucThreadScheduling;
    u8 ucThreadPriority;
    s8 cThreadNice;

    u8 ucTcRestrict;
    u32 u4MaxTxDeQLimit;
    u8 ucAlwaysResetUsedRes;

    u32 u4NetifStopTh;
    u32 u4NetifStartTh;
#if CFG_AUTO_CHANNEL_SEL_SUPPORT
    PARAM_GET_CHN_INFO rChnLoadInfo;
#endif
#if CFG_SUPPORT_MTK_SYNERGY
    u8 ucMtkOui;
    u32 u4MtkOuiCap;
    u8 aucMtkFeature[4];
#endif

    u8 fgCsaInProgress;
    u8 ucChannelSwitchMode;
    u8 ucNewChannelNumber;
    u8 ucChannelSwitchCount;
#if CFG_SUPPORT_DBDC_TC6
    u8 fgDelayInidicateDISCON;
#endif

    u32 u4HifIstLoopCount;
    u32 u4Rx2OsLoopCount;
    u32 u4HifTxloopCount;
    u32 u4TxRxLoopCount;
    u32 u4TxFromOsLoopCount;
    u32 u4TxIntThCount;

    u32 au4TcPageCount[TC_NUM];
    u8 ucExtraTxDone;
    u8 ucTxDbg;

    u8 ucCmdRsvResource;
    u32 u4MgmtQueueDelayTimeout;

    u32 u4StatsLogTimeout;
    u32 u4StatsLogDuration;
    u8 ucDhcpTxDone;
    u8 ucArpTxDone;

    u8 ucMacAddrOverride;
    u8 aucMacAddrStr[32];

    u8 ucCtiaMode;
    u8 ucTpTestMode;
    u8 ucSigmaTestMode;
#if CFG_SUPPORT_DBDC
    u8 ucDbdcMode;
    u8 fgDbDcModeEn;
    TIMER_T
        rDBDCDisableCountdownTimer;  /* Prevent continuously trigger by
                                      * reconnection  */
    TIMER_T rDBDCSwitchGuardTimer;  /* Prevent switch too quick*/
#endif
#if CFG_SUPPORT_DBDC_TC6
    TIMER_T
        rDBDCReconnectCountDown;  /* Prevent driver-level reconnection process
                                   * from DBDC switching */
    TIMER_T
        rDBDCAisConnectCountDown;  /* Prevent AIS connect process from DBDC
                                    * switching */
#endif
    u8 u4ScanCtrl;
    u8 ucScanChannelListenTime;

#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
    u8 ucEfuseBufferModeCal;
#endif
    u8 ucCalTimingCtrl;
    u8 ucWow;
    u8 ucOffload;
    u8 ucAdvPws;  /* enable LP multiple DTIM function, default enable */
    u8 ucWowOnMdtim;  /* multiple DTIM if WOW enable, default 1 */
    u8 ucWowOffMdtim;  /* multiple DTIM if WOW disable, default 3 */
    u8 ucWowPwsMode;  /* when enter wow, automatically enter wow power-saving
                       * profile */
    u8 ucWlanSetCamDuringAct;  /* Set wlan PS=CAM during activity */
    u8 ucListenDtimInterval;  /* adjust the listen interval by dtim interval
                               */
    u8 ucEapolOffload;  /* eapol offload when active mode / wow mode */
#ifdef SUPPORT_ENFORCE_PWR_MODE
    u8 ucEnforce2G;  /* Enforce to a specific power mode regardless of
                      * Android's setting */
    u8 ucEnforce5G;  /* Enforce to a specific power mode regardless of
                      * Android's setting */
#else
    u8 ucEnforcePSMode;  /* Enforce to a specific power mode regardless of
                          * Android's setting */
    u8 ucEnforceCAM2G;
#endif

#ifdef SUPPORT_PERIODIC_PS
    u8 ucPspCAMInt;
    u8 ucAwakePspCAMInt;
#define PSP_CAM_INT_DEFAULT      20
#define AWAKE_PSP_CAM_INT_DEFAULT    20
    u8 ucPspPSInt;
    u8 ucAwakePspPSInt;
#define PSP_PS_INT_DEFAULT       10
#define AWAKE_PSP_PS_INT_DEFAULT     10
#endif

    u8 ucWaitConnect;
#define WAIT_CONNECT_DEFAULT         20

#if CFG_SUPPORT_REPLAY_DETECTION
    u8 ucRpyDetectOffload;  /* eapol offload when active mode / wow mode */
#endif

    u8 u4SwTestMode;
    u8 ucCtrlFlagAssertPath;
    u8 ucCtrlFlagDebugLevel;
    u32 u4WakeLockRxTimeout;
    u32 u4WakeLockThreadWakeup;
    u32 u4RegP2pIfAtProbe;  /* register p2p interface during probe */
    u8 ucP2pShareMacAddr;  /* p2p group interface use the same mac addr as
                            * p2p device interface */
    u8 ucSmartRTS;

    u32 u4UapsdAcBmp;
    u32 u4MaxSpLen;
    u32 fgDisOnlineScan;  /* 0: enable online scan, non-zero: disable online
                           * scan */
    u32 fgDisBcnLostDetection;
    u32 fgDisRoaming;  /* 0:enable roaming 1:disable */
    u32 fgEnArpFilter;

    u8 uDeQuePercentEnable;
    u32 u4DeQuePercentVHT80Nss1;
    u32 u4DeQuePercentVHT40Nss1;
    u32 u4DeQuePercentVHT20Nss1;
    u32 u4DeQuePercentHT40Nss1;
    u32 u4DeQuePercentHT20Nss1;

    u8 fgTdlsBufferSTASleep;  /* Support TDLS 5.5.4.2 optional case */
    u8 fgChipResetRecover;

    u8 ucN9Log2HostCtrl;
    u8 ucCR4Log2HostCtrl;

#if CFG_SUPPORT_ANT_SELECT
    u8 ucSpeIdxCtrl;  /* 0:WF0, 1:WF1, 2: both WF0/1 */
    u8 ucSpeIdxCtrl2G;  /* 0:WF0, 1:WF1, 2: both WF0/1 only in 2G*/
#endif

#ifdef CFG_SUPPORT_ADJUST_JOIN_CH_REQ_INTERVAL
    u32 u4AisJoinChReqIntervel;
#endif

#if CFG_SUPPORT_RSSI_COMP
    RSSI_PATH_COMPASATION_T rRssiPathCompasation;
#endif
    s32 ucEd2GNonEU;
    s32 ucEd5GNonEU;
    s32 ucEd2GEU;
    s32 ucEd5GEU;
    u8 ucDelayTimeOfDisconnect;
#if CFG_KEY_ERROR_STATISTIC_RECOVERY
    s32 u4BmcKeyErrorTh;
#endif
} WIFI_VAR_T, *P_WIFI_VAR_T;  /* end of _WIFI_VAR_T */

/* cnm_timer module */
typedef struct {
    LINK_T rLinkHead;
    u32 rNextExpiredSysTime;
    u8 fgWakeLocked;
} ROOT_TIMER, *P_ROOT_TIMER;

/* FW/DRV/NVRAM version information */
typedef struct {
    /* NVRAM or Registry */
    u16 u2Part1CfgOwnVersion;
    u16 u2Part1CfgPeerVersion;
    u16 u2Part2CfgOwnVersion;
    u16 u2Part2CfgPeerVersion;

    /* Firmware */
    /* N9 SW */
    u16 u2FwProductID;
    u16 u2FwOwnVersion;
    u16 u2FwPeerVersion;
    u8 ucFwBuildNumber;
    u8 aucFwBranchInfo[4];
    u8 aucFwDateCode[16];

    /* N9 tailer */
    tailer_format_t rN9tailer;

    /* CR4 tailer */
    tailer_format_t rCR4tailer;
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
    /* N9 Compressed tailer */
    tailer_format_t_2 rN9Compressedtailer;
    /* CR4 tailer */
    tailer_format_t_2 rCR4Compressedtailer;
    u8 fgIsN9CompressedFW;
    u8 fgIsCR4CompressedFW;
#endif
    /* Patch header */
    PATCH_FORMAT_T rPatchHeader;
    u8 fgPatchIsDlByDrv;
} WIFI_VER_INFO_T, *P_WIFI_VER_INFO_T;

#if CFG_ENABLE_WIFI_DIRECT
/*
 * p2p function pointer structure
 */

typedef struct _P2P_FUNCTION_LINKER {
    P2P_REMOVE prP2pRemove;
    /* NIC_P2P_MEDIA_STATE_CHANGE                  prNicP2pMediaStateChange;
     */
    /* SCAN_UPDATE_P2P_DEVICE_DESC prScanUpdateP2pDeviceDesc; */
    /* P2P_FSM_RUN_EVENT_RX_PROBE_RESPONSE_FRAME
     * prP2pFsmRunEventRxProbeResponseFrame; */
    P2P_GENERATE_P2P_IE prP2pGenerateWSC_IEForBeacon;
    /* P2P_CALCULATE_WSC_IE_LEN_FOR_PROBE_RSP
     * prP2pCalculateWSC_IELenForProbeRsp; */
    /* P2P_GENERATE_WSC_IE_FOR_PROBE_RSP prP2pGenerateWSC_IEForProbeRsp; */
    /* SCAN_REMOVE_P2P_BSS_DESC                    prScanRemoveP2pBssDesc;
     */
    /* P2P_HANDLE_SEC_CHECK_RSP                    prP2pHandleSecCheckRsp;
     */
    P2P_NET_REGISTER prP2pNetRegister;
    P2P_NET_UNREGISTER prP2pNetUnregister;
    P2P_CALCULATE_P2P_IE_LEN
        prP2pCalculateP2p_IELenForAssocReq;  /* All IEs generated from
                                              * supplicant. */
    P2P_GENERATE_P2P_IE
        prP2pGenerateP2p_IEForAssocReq;  /* All IEs generated from supplicant. */
} P2P_FUNCTION_LINKER, *P_P2P_FUNCTION_LINKER;

#endif

typedef struct _WIFI_FEM_CFG_T {
    /* WiFi FEM path */
    u16 u2WifiPath;
    u16 u2Reserved;
    /* Reserved  */
    u32 au4Reserved[4];
} WIFI_FEM_CFG_T, *P_WIFI_FEM_CFG_T;

struct CSI_DATA_T {
    u8 ucBw;
    u8 bIsCck;
    u16 u2DataCount;
    s16 ac2IData[256];
    s16 ac2QData[256];
    u8 ucDbdcIdx;
    u8 ucDataOutputted;  /* bit 0: I data, bit 1: Q data. Set to 1 if it's
                          * ouputted */
};

/*
 * Major ADAPTER structure
 * Major data structure for driver operation
 */
struct _ADAPTER_T {
    struct chip_info *chip_info;
    u8 ucRevID;
    u8 fgIsReadRevID;

    u16 u2NicOpChnlNum;

    u8 fgIsEnableWMM;
    u8 fgIsWmmAssoc;  /* This flag is used to indicate that WMM is enable in
                       * current BSS */

    u32 u4OsPacketFilter;  /* packet filter used by OS */
    u8 fgAllMulicastFilter;  /* mDNS filter used by OS */

    P_BSS_INFO_T aprBssInfo[HW_BSSID_NUM + 1];
    P_BSS_INFO_T prAisBssInfo;

    u8 ucHwBssIdNum;
    u8 ucWmmSetNum;
    u8 ucWtblEntryNum;
    u8 ucTxDefaultWlanIndex;
    u8 ucP2PDevBssIdx;

#if CFG_TCP_IP_CHKSUM_OFFLOAD
    u8 fgIsSupportCsumOffload;  /* Does FW support Checksum Offload feature
                                 */
    u32 u4CSUMFlags;
#endif

    ENUM_BAND_T aePreferBand[BSS_INFO_NUM];

    /* ADAPTER flags */
    u32 u4Flags;
    u32 u4HwFlags;

    u8 fgIsRadioOff;

    u8 fgIsEnterD3ReqIssued;

    u8 aucMacAddress[MAC_ADDR_LEN];

    ENUM_PHY_TYPE_INDEX_T
        eCurrentPhyType;  /* Current selection basing on the set of Available PHY
                           * Types */

    u32 u4CoalescingBufCachedSize;
    u8 *pucCoalescingBufCached;

    /* Buffer for CMD_INFO_T, Mgt packet and mailbox message */
    BUF_INFO_T rMgtBufInfo;
    BUF_INFO_T rMsgBufInfo;
    u8 *pucMgtBufCached;
    u32 u4MgtBufCachedSize;
    u8 aucMsgBuf[MSG_BUFFER_SIZE];
#if CFG_DBG_MGT_BUF
    u32 u4MemAllocDynamicCount;  /* Debug only */
    u32 u4MemFreeDynamicCount;  /* Debug only */
#endif

    STA_RECORD_T arStaRec[CFG_STA_REC_NUM];

    /* Element for TX PATH */
    TX_CTRL_T rTxCtrl;
    QUE_T rFreeCmdList;
    CMD_INFO_T arHifCmdDesc[CFG_TX_MAX_CMD_PKT_NUM];

    /* Element for RX PATH */
    RX_CTRL_T rRxCtrl;

    /* Timer for restarting RFB setup procedure */
    TIMER_T rPacketDelaySetupTimer;

    /* Buffer for Authentication Event */
    /* <Todo> Move to glue layer and refine the kal function */
    /* Reference to rsnGeneratePmkidIndication function at rsn.c */
    u8 aucIndicationEventBuffer[(CFG_MAX_PMKID_CACHE * 20) + 8];

    u32 u4IntStatus;

    ENUM_ACPI_STATE_T rAcpiState;

    u8 fgIsIntEnable;
    u8 fgIsIntEnableWithLPOwnSet;

    u8 fgIsFwOwn;
    u8 fgWiFiInSleepyState;

    /* Set by callback to make sure WOW process done before system suspend
     */
    u8 fgSetPfCapabilityDone;
    u8 fgSetWowDone;

    u8 fgForceFwOwn;

    u32 rLastOwnFailedLogTime;
    u32 u4OwnFailedCount;
    u32 u4OwnFailedLogCount;

    u32 u4PwrCtrlBlockCnt;

    /* TX Direct related : BEGIN */
    u8 fgTxDirectInited;

#define TX_DIRECT_CHECK_INTERVAL    (1000 * HZ / USEC_PER_SEC)
    struct timer_list rTxDirectSkbTimer;  /* check if an empty MsduInfo is
                                           * available */
    struct timer_list rTxDirectHifTimer;  /* check if HIF port is ready to
                                           * accept a new Msdu */

    struct sk_buff_head rTxDirectSkbQueue;
    QUE_T rTxDirectHifQueue[TX_PORT_NUM];

    QUE_T rStaPsQueue[CFG_STA_REC_NUM];
    u32 u4StaPsBitmap;
    QUE_T rBssAbsentQueue[HW_BSSID_NUM + 1];
    u32 u4BssAbsentBitmap;
    /* TX Direct related : END */

    QUE_T rPendingCmdQueue;
    QUE_T rTxCmdQueue;
    QUE_T rTxCmdDoneQueue;
    QUE_T rWDevLockQueue;

#if CFG_FIX_2_TX_PORT
    QUE_T rTxP0Queue;
    QUE_T rTxP1Queue;
#else
    QUE_T rTxPQueue[TX_PORT_NUM];
#endif
    QUE_T rRxQueue;
    QUE_T rTxDataDoneQueue;

    P_GLUE_INFO_T prGlueInfo;

    u8 ucCmdSeqNum;
    u8 ucTxSeqNum;
    u8 aucPidPool[WTBL_SIZE];

    u8 fgUseWapi;

    /* RF Test flags */
    u8 fgTestMode;
    u8 fgIcapMode;

    /* WLAN Info for DRIVER_CORE OID query */
    WLAN_INFO_T rWlanInfo;

#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_DBDC_TC6
    u8 u4P2pMode;
#endif
    u8 fgIsP2PRegistered;
    u8 p2p_scan_report_all_bss;  /* flag to report all networks in p2p scan
                                  */
    ENUM_NET_REG_STATE_T rP2PNetRegState;
    ENUM_P2P_REG_STATE_T rP2PRegState;
    /* u8             fgIsWlanLaunched; */
    P_P2P_INFO_T prP2pInfo;
#if CFG_SUPPORT_P2P_RSSI_QUERY
    u32 rP2pLinkQualityUpdateTime;
    u8 fgIsP2pLinkQualityValid;
    EVENT_LINK_QUALITY rP2pLinkQuality;
#endif
#endif

    /* Online Scan Option */
    u8 fgEnOnlineScan;

    /* Online Scan Option */
    u8 fgDisBcnLostDetection;

    /* MAC address */
    PARAM_MAC_ADDRESS rMyMacAddr;

    /* Wake-up Event for WOL */
    u32 u4WakeupEventEnable;

    /* Event Buffering */
    EVENT_STATISTICS rStatStruct;
    u32 rStatUpdateTime;
    u8 fgIsStatValid;

#if CFG_SUPPORT_MSP
    EVENT_WLAN_INFO rEventWlanInfo;
#endif

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
    TIMER_T rRxMcsInfoTimer;
    u8 fgIsMcsInfoValid;
#endif

    EVENT_LINK_QUALITY rLinkQuality;
    u32 rLinkQualityUpdateTime;
    u8 fgIsLinkQualityValid;
    u32 rLinkRateUpdateTime;
    u8 fgIsLinkRateValid;

    /* WIFI_VAR_T */
    WIFI_VAR_T rWifiVar;

    /* MTK WLAN NIC driver IEEE 802.11 MIB */
    IEEE_802_11_MIB_T rMib;

    /* Mailboxs for inter-module communication */
    MBOX_T arMbox[MBOX_ID_TOTAL_NUM];

    /* Timers for OID Pending Handling */
    TIMER_T rOidTimeoutTimer;
    u8 ucOidTimeoutCount;

    /* Root Timer for cnm_timer module */
    ROOT_TIMER rRootTimer;

    u8 fgIsChipNoAck;
    u8 fgIsChipAssert;

    /* RLM maintenance */
    ENUM_CHNL_EXT_T eRfSco;
    ENUM_SYS_PROTECT_MODE_T eSysProtectMode;
    ENUM_GF_MODE_T eSysHtGfMode;
    ENUM_RIFS_MODE_T eSysTxRifsMode;
    ENUM_SYS_PCO_PHASE_T eSysPcoPhase;

    P_DOMAIN_INFO_ENTRY prDomainInfo;

    /* QM */
    QUE_MGT_T rQM;

    CNM_INFO_T rCnmInfo;

    u32 u4PowerMode;

    u32 u4CtiaPowerMode;
    u8 fgEnCtiaPowerMode;

    /* Bitmap is defined as #define KEEP_FULL_PWR_{FEATURE}_BIT in
     * wlan_lib.h Each feature controls KeepFullPwr(CMD_ID_KEEP_FULL_PWR)
     * should register bitmap to ensure low power during suspend.
     */
    u32 u4IsKeepFullPwrBitmap;

    u32 fgEnArpFilter;

    u32 u4UapsdAcBmp;

    u32 u4MaxSpLen;

    u32 u4PsCurrentMeasureEn;

    /* Version Information */
    WIFI_VER_INFO_T rVerInfo;

    /* 5GHz support (from F/W) */
    u8 fgIsHw5GBandDisabled;
    u8 fgEnable5GBand;
    u8 fgIsEepromUsed;
    u8 fgIsEfuseValid;
    u8 fgIsEmbbededMacAddrValid;

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
    u8 fgIsPowerLimitTableValid;
#endif

    /* Packet Forwarding Tracking */
    s32 i4PendingFwdFrameCount;

#if CFG_SUPPORT_RDD_TEST_MODE
    u8 ucRddStatus;
#endif

    u8 fgDisStaAgingTimeoutDetection;

    u32 u4FwCompileFlag0;
    u32 u4FwCompileFlag1;
    u32 u4FwFeatureFlag0;
    u32 u4FwFeatureFlag1;

#if CFG_SUPPORT_CFG_FILE
    P_WLAN_CFG_T prWlanCfg;
    WLAN_CFG_T rWlanCfg;

    P_WLAN_CFG_REC_T prWlanCfgRec;
    WLAN_CFG_REC_T rWlanCfgRec;
#endif

#if CFG_ASSERT_DUMP
    TIMER_T rN9CorDumpTimer;
    TIMER_T rCr4CorDumpTimer;
    u8 fgN9CorDumpFileOpend;
    u8 fgCr4CorDumpFileOpend;
    u8 fgN9AssertDumpOngoing;
    u8 fgCr4AssertDumpOngoing;
    u8 fgKeepPrintCoreDump;
#endif
    /* Tx resource information */
    u8 fgIsNicTxReousrceValid;
    NIC_TX_RESOURCE_T nicTxReousrce;

    /* Efuse Start and End address */
    u32 u4EfuseStartAddress;
    u32 u4EfuseEndAddress;

    /* COEX feature */
    u32 u4FddMode;

#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
    /* MAC address Efuse Offset */
    u32 u4EfuseMacAddrOffset;
#endif

#if CFG_WOW_SUPPORT
    WOW_CTRL_T rWowCtrl;
#endif
    u8 fgIsCfg80211SuspendCalled;

    /*#if (CFG_EEPROM_PAGE_ACCESS == 1)*/
    u8 aucEepromVaule[16];  /* HQA CMD for Efuse Block size contents */
    u32 u4FreeBlockNum;
    u32 u4GetTxPower;
    /*#endif*/
    u8 fgIsCr4FwDownloaded;
    u8 fgIsFwDownloaded;
    /* u8 fgIsSupportBufferBinSize16Byte; */
    /* u8 fgIsSupportDelayCal; */
    /* u8 fgIsSupportGetFreeEfuseBlockCount; */
    /* u8 fgIsSupportQAAccessEfuse; */
    /* u8 fgIsSupportPowerOnSendBufferModeCMD; */
    u8 fgIsBufferBinExtract;
    /* u8 fgIsSupportGetTxPower; */
    u8 fgIsEnableLpdvt;

#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT
    u8 fgIsFixedMRate;
    u8 ucDupMcastPacketNum;
    u32 u4TXOPPeriod;
    u8 fgIsMcastBurstMode;
    u8 uc11mcSupportType;
    u8 fgIsUcastBurstMode;
    u8 ucUnicastBurstTimeout;  // the default time is 10 ms
    /* Specific AudioTos */
    u8 ucAudioTOS;
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
    u8 fgIsLookBackMode;
    u16 ucRxLBList[5];
    u8 ucRxLBSize;
    u8 ucRxLBIndex;
#endif
#endif

    /* SER related info */
    u8 ucSerState;

#if CFG_SUPPORT_BFER
    u8 fgIsHwSupportBfer;
#endif

#if (CFG_HW_WMM_BY_BSS == 1)
    u8 ucHwWmmEnBit;
#endif
    WIFI_FEM_CFG_T rWifiFemCfg;
    struct CSI_DATA_T rCsiData;

    ENUM_TX_RESULT_CODE_T r1xTxDoneStatus;

    u8 fgIsTest1xTx;
};  /* end of _ADAPTER_T */

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

/* TODO */
#define SUSPEND_FLAG_FOR_WAKEUP_REASON    (0)
#define SUSPEND_FLAG_CLEAR_WHEN_RESUME    (1)

/* Macros for argument _BssIndex */
#define IS_NET_ACTIVE(_prAdapter, _BssIndex) \
    ((_prAdapter)->aprBssInfo[(_BssIndex)]->fgIsNetActive)

/* Macros for argument _prBssInfo */
#define IS_BSS_ACTIVE(_prBssInfo)    ((_prBssInfo)->fgIsNetActive)

#define IS_BSS_AIS(_prBssInfo)       ((_prBssInfo)->eNetworkType == \
                                      NETWORK_TYPE_AIS)

#define IS_BSS_P2P(_prBssInfo)       ((_prBssInfo)->eNetworkType == \
                                      NETWORK_TYPE_P2P)

#define IS_BSS_BOW(_prBssInfo)       ((_prBssInfo)->eNetworkType == \
                                      NETWORK_TYPE_BOW)

#define IS_BSS_APGO(_prBssInfo) \
    (IS_BSS_P2P(_prBssInfo) &&  \
     (_prBssInfo)->eCurrentOPMode == OP_MODE_ACCESS_POINT)

#define SET_NET_ACTIVE(_prAdapter, _BssIndex)                        \
    {                                                                \
        (_prAdapter)->aprBssInfo[(_BssIndex)]->fgIsNetActive = true; \
    }

#define UNSET_NET_ACTIVE(_prAdapter, _BssIndex)                       \
    {                                                                 \
        (_prAdapter)->aprBssInfo[(_BssIndex)]->fgIsNetActive = false; \
    }

#define BSS_INFO_INIT(_prAdapter, _prBssInfo)                       \
    {                                                               \
        u8 _aucZeroMacAddr[] = NULL_MAC_ADDR;                       \
                                                                    \
        (_prBssInfo)->eConnectionState =                            \
            PARAM_MEDIA_STATE_DISCONNECTED;                         \
        (_prBssInfo)->eConnectionStateIndicated =                   \
            PARAM_MEDIA_STATE_DISCONNECTED;                         \
        (_prBssInfo)->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;      \
        (_prBssInfo)->ucReasonOfDisconnect =                        \
            DISCONNECT_REASON_CODE_RESERVED;                        \
        COPY_MAC_ADDR((_prBssInfo)->aucBSSID, _aucZeroMacAddr);     \
        LINK_INITIALIZE(&((_prBssInfo)->rStaRecOfClientList));      \
        (_prBssInfo)->fgIsBeaconActivated = false;                  \
        (_prBssInfo)->u2HwDefaultFixedRateCode = RATE_CCK_1M_LONG;  \
        (_prBssInfo)->ePowerModeFromUser = Param_PowerModeFast_PSP; \
    }

/*----------------------------------------------------------------------------*/
/* Macros for Power State                                                     */
/*----------------------------------------------------------------------------*/
#define SET_NET_PWR_STATE_IDLE(_prAdapter, _BssIndex)                  \
    {                                                                  \
        _prAdapter->rWifiVar.aePwrState[(_BssIndex)] = PWR_STATE_IDLE; \
    }

#define SET_NET_PWR_STATE_ACTIVE(_prAdapter, _BssIndex) \
    {                                                   \
        _prAdapter->rWifiVar.aePwrState[(_BssIndex)] =  \
            PWR_STATE_ACTIVE;                           \
    }

#define SET_NET_PWR_STATE_PS(_prAdapter, _BssIndex)                  \
    {                                                                \
        _prAdapter->rWifiVar.aePwrState[(_BssIndex)] = PWR_STATE_PS; \
    }

#define IS_NET_PWR_STATE_ACTIVE(_prAdapter, _BssIndex) \
    (_prAdapter->rWifiVar.aePwrState[(_BssIndex)] == PWR_STATE_ACTIVE)

#define IS_NET_PWR_STATE_IDLE(_prAdapter, _BssIndex) \
    (_prAdapter->rWifiVar.aePwrState[(_BssIndex)] == PWR_STATE_IDLE)

#define IS_SCN_PWR_STATE_ACTIVE(_prAdapter) \
    (_prAdapter->rWifiVar.rScanInfo.eScanPwrState == SCAN_PWR_STATE_ACTIVE)

#define IS_SCN_PWR_STATE_IDLE(_prAdapter) \
    (_prAdapter->rWifiVar.rScanInfo.eScanPwrState == SCAN_PWR_STATE_IDLE)

#define IS_WIFI_2G4_WF0_SUPPORT(_prAdapter) \
    ((_prAdapter)->rWifiFemCfg.u2WifiPath & WLAN_FLAG_2G4_WF0)

#define IS_WIFI_5G_WF0_SUPPORT(_prAdapter) \
    ((_prAdapter)->rWifiFemCfg.u2WifiPath & WLAN_FLAG_5G_WF0)

#define IS_WIFI_2G4_WF1_SUPPORT(_prAdapter) \
    ((_prAdapter)->rWifiFemCfg.u2WifiPath & WLAN_FLAG_2G4_WF1)

#define IS_WIFI_5G_WF1_SUPPORT(_prAdapter) \
    ((_prAdapter)->rWifiFemCfg.u2WifiPath & WLAN_FLAG_5G_WF1)

#define IS_WIFI_2G4_SISO(_prAdapter)             \
    ((IS_WIFI_2G4_WF0_SUPPORT(_prAdapter) &&     \
      !(IS_WIFI_2G4_WF1_SUPPORT(_prAdapter))) || \
     (IS_WIFI_2G4_WF1_SUPPORT(_prAdapter) &&     \
      !(IS_WIFI_2G4_WF0_SUPPORT(_prAdapter))))

#define IS_WIFI_5G_SISO(_prAdapter)             \
    ((IS_WIFI_5G_WF0_SUPPORT(_prAdapter) &&     \
      !(IS_WIFI_5G_WF1_SUPPORT(_prAdapter))) || \
     (IS_WIFI_5G_WF1_SUPPORT(_prAdapter) &&     \
      !(IS_WIFI_5G_WF0_SUPPORT(_prAdapter))))

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
