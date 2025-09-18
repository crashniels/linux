/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "nic.h"
 *    \brief  The declaration of nic functions
 *
 *    Detail description.
 */

#ifndef _NIC_H
#define _NIC_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
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

struct _REG_ENTRY_T {
    u32 u4Offset;
    u32 u4Value;
};

struct _TABLE_ENTRY_T {
    P_REG_ENTRY_T pu4TablePtr;
    u16 u2Size;
};

/*! INT status to event map */
typedef struct _INT_EVENT_MAP_T {
    u32 u4Int;
    u32 u4Event;
} INT_EVENT_MAP_T, *P_INT_EVENT_MAP_T;

typedef struct _ECO_INFO_T {
    u8 ucHwVer;
    u8 ucRomVer;
    u8 ucFactoryVer;
    u8 ucEcoVer;
} ECO_INFO_T, *P_ECO_INFO_T;

enum ENUM_INT_EVENT_T {
    INT_EVENT_ABNORMAL,
    INT_EVENT_SW_INT,
    INT_EVENT_TX,
    INT_EVENT_RX,
    INT_EVENT_NUM
};

typedef enum _ENUM_IE_UPD_METHOD_T {
    IE_UPD_METHOD_UPDATE_RANDOM,
    IE_UPD_METHOD_UPDATE_ALL,
    IE_UPD_METHOD_DELETE_ALL,
} ENUM_IE_UPD_METHOD_T,
*P_ENUM_IE_UPD_METHOD_T;

typedef enum _ENUM_SER_STATE_T {
    SER_IDLE_DONE,  /* SER is idle or done */
    SER_STOP_HOST_TX,  /* Host HIF Tx is stopped */
    SER_STOP_HOST_TX_RX,  /* Host HIF Tx/Rx is stopped */
    SER_REINIT_HIF,  /* Host HIF is reinit */

    SER_STATE_NUM
} ENUM_SER_STATE_T,
*P_ENUM_SER_STATE_T;

/* Test mode bitmask of disable flag */
#define TEST_MODE_DISABLE_ONLINE_SCAN     BIT(0)
#define TEST_MODE_DISABLE_ROAMING     BIT(1)
#define TEST_MODE_FIXED_CAM_MODE      BIT(2)
#define TEST_MODE_DISABLE_BCN_LOST_DET    BIT(3)
#define TEST_MODE_NONE            0
#define TEST_MODE_THROUGHPUT                                     \
    (TEST_MODE_DISABLE_ONLINE_SCAN | TEST_MODE_DISABLE_ROAMING | \
     TEST_MODE_FIXED_CAM_MODE | TEST_MODE_DISABLE_BCN_LOST_DET)
#define TEST_MODE_SIGMA_AC_N_PMF \
    (TEST_MODE_DISABLE_ONLINE_SCAN | TEST_MODE_FIXED_CAM_MODE)
#define TEST_MODE_SIGMA_WMM_PS        (TEST_MODE_DISABLE_ONLINE_SCAN)
#define TEST_MODE_AUDIO_MRM                                      \
    (TEST_MODE_DISABLE_ONLINE_SCAN | TEST_MODE_DISABLE_ROAMING | \
     TEST_MODE_FIXED_CAM_MODE)

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
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

/*----------------------------------------------------------------------------*/
/* Routines in nic.c                                                          */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicAllocateAdapterMemory(IN P_ADAPTER_T prAdapter);

void nicReleaseAdapterMemory(IN P_ADAPTER_T prAdapter);

void nicDisableInterrupt(IN P_ADAPTER_T prAdapter);

void nicEnableInterrupt(IN P_ADAPTER_T prAdapter);

WLAN_STATUS nicProcessIST(IN P_ADAPTER_T prAdapter);

WLAN_STATUS nicProcessIST_impl(IN P_ADAPTER_T prAdapter, IN u32 u4IntStatus);

WLAN_STATUS nicInitializeAdapter(IN P_ADAPTER_T prAdapter);

void nicMCRInit(IN P_ADAPTER_T prAdapter);

u8 nicVerifyChipID(IN P_ADAPTER_T prAdapter);

void nicpmWakeUpWiFi(IN P_ADAPTER_T prAdapter);

u8 nicpmSetDriverOwn(IN P_ADAPTER_T prAdapter);

void nicpmSetFWOwn(IN P_ADAPTER_T prAdapter, IN u8 fgEnableGlobalInt);

u8 nicpmSetAcpiPowerD0(IN P_ADAPTER_T prAdapter);

u8 nicpmSetAcpiPowerD3(IN P_ADAPTER_T prAdapter);

void nicProcessSoftwareInterrupt(IN P_ADAPTER_T prAdapter);

void nicProcessAbnormalInterrupt(IN P_ADAPTER_T prAdapter);

void nicSetSwIntr(IN P_ADAPTER_T prAdapter, IN u32 u4SwIntrBitmap);

P_CMD_INFO_T nicGetPendingCmdInfo(IN P_ADAPTER_T prAdapter, IN u8 ucSeqNum);

P_MSDU_INFO_T nicGetPendingTxMsduInfo(IN P_ADAPTER_T prAdapter,
                                      IN u8 ucWlanIndex,
                                      IN u8 ucSeqNum);

void nicFreePendingTxMsduInfoByBssIdx(IN P_ADAPTER_T prAdapter,
                                      IN u8 ucBssIndex);

u8 nicIncreaseCmdSeqNum(IN P_ADAPTER_T prAdapter);

u8 nicIncreaseTxSeqNum(IN P_ADAPTER_T prAdapter);

/* Media State Change */
WLAN_STATUS
nicMediaStateChange(IN P_ADAPTER_T prAdapter,
                    IN u8 ucBssIndex,
                    IN P_EVENT_CONNECTION_STATUS prConnectionStatus);

WLAN_STATUS nicMediaJoinFailure(IN P_ADAPTER_T prAdapter,
                                IN u8 ucBssIndex,
                                IN WLAN_STATUS rStatus);

/* Utility function for channel number conversion */
u32 nicChannelNum2Freq(IN u32 u4ChannelNum);

u32 nicFreq2ChannelNum(IN u32 u4FreqInKHz);

u8 nicGetVhtS1(IN u8 ucPrimaryChannel, IN u8 ucBandwidth);

/* firmware command wrapper */
/* NETWORK (WIFISYS) */
WLAN_STATUS nicActivateNetwork(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex);

WLAN_STATUS nicDeactivateNetwork(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex);

/* BSS-INFO */
WLAN_STATUS nicUpdateBss(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex);

/* BSS-INFO Indication (PM) */
WLAN_STATUS nicPmIndicateBssCreated(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex);

WLAN_STATUS nicPmIndicateBssConnected(IN P_ADAPTER_T prAdapter,
                                      IN u8 ucBssIndex);

WLAN_STATUS nicPmIndicateBssAbort(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex);

/* Beacon Template Update */
WLAN_STATUS
nicUpdateBeaconIETemplate(IN P_ADAPTER_T prAdapter,
                          IN ENUM_IE_UPD_METHOD_T eIeUpdMethod,
                          IN u8 ucBssIndex,
                          IN u16 u2Capability,
                          IN u8 *aucIe,
                          IN u16 u2IELen);

WLAN_STATUS nicQmUpdateWmmParms(IN P_ADAPTER_T prAdapter, IN u8 ucBssIndex);

WLAN_STATUS nicSetAutoTxPower(IN P_ADAPTER_T prAdapter,
                              IN P_CMD_AUTO_POWER_PARAM_T prAutoPwrParam);

/*----------------------------------------------------------------------------*/
/* Calibration Control                                                        */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicUpdateTxPower(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_TX_PWR_T prTxPwrParam);

WLAN_STATUS nicUpdate5GOffset(IN P_ADAPTER_T prAdapter,
                              IN P_CMD_5G_PWR_OFFSET_T pr5GPwrOffset);

WLAN_STATUS nicUpdateDPD(IN P_ADAPTER_T prAdapter,
                         IN P_CMD_PWR_PARAM_T prDpdCalResult);

/*----------------------------------------------------------------------------*/
/* PHY configuration                                                          */
/*----------------------------------------------------------------------------*/
void nicSetAvailablePhyTypeSet(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* MGMT and System Service Control                                            */
/*----------------------------------------------------------------------------*/
void nicInitSystemService(IN P_ADAPTER_T prAdapter);

void nicResetSystemService(IN P_ADAPTER_T prAdapter);

void nicUninitSystemService(IN P_ADAPTER_T prAdapter);

void nicInitMGMT(IN P_ADAPTER_T prAdapter, IN P_REG_INFO_T prRegInfo);

void nicUninitMGMT(IN P_ADAPTER_T prAdapter);

WLAN_STATUS
nicConfigPowerSaveProfile(IN P_ADAPTER_T prAdapter,
                          u8 ucBssIndex,
                          PARAM_POWER_MODE ePwrMode,
                          u8 fgEnCmdEvent);

WLAN_STATUS
nicConfigPowerSaveWowProfile(IN P_ADAPTER_T prAdapter,
                             u8 ucBssIndex,
                             PARAM_POWER_MODE ePwrMode,
                             u8 fgEnCmdEvent,
                             u8 fgSuspend);

WLAN_STATUS nicEnterCtiaMode(IN P_ADAPTER_T prAdapter,
                             u8 fgEnterCtia,
                             u8 fgEnCmdEvent);
WLAN_STATUS nicEnterTPTestMode(IN P_ADAPTER_T prAdapter, IN u8 ucFuncMask);

/*----------------------------------------------------------------------------*/
/* Scan Result Processing                                                     */
/*----------------------------------------------------------------------------*/
void nicAddScanResult(IN P_ADAPTER_T prAdapter,
                      IN PARAM_MAC_ADDRESS rMacAddr,
                      IN P_PARAM_SSID_T prSsid,
                      IN u32 u4Privacy,
                      IN PARAM_RSSI rRssi,
                      IN ENUM_PARAM_NETWORK_TYPE_T eNetworkType,
                      IN P_PARAM_802_11_CONFIG_T prConfiguration,
                      IN ENUM_PARAM_OP_MODE_T eOpMode,
                      IN PARAM_RATES_EX rSupportedRates,
                      IN u16 u2IELength,
                      IN u8 *pucIEBuf);

void nicFreeScanResultIE(IN P_ADAPTER_T prAdapter, IN u32 u4Idx);

/*----------------------------------------------------------------------------*/
/* Fixed Rate Hacking                                                         */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
nicUpdateRateParams(IN P_ADAPTER_T prAdapter,
                    IN ENUM_REGISTRY_FIXED_RATE_T eRateSetting,
                    IN u8 *pucDesiredPhyTypeSet,
                    IN u16 *pu2DesiredNonHTRateSet,
                    IN u16 *pu2BSSBasicRateSet,
                    IN u8 *pucMcsSet,
                    IN u8 *pucSupMcs32,
                    IN u16 *u2HtCapInfo);

/*----------------------------------------------------------------------------*/
/* Write registers                                                            */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicWriteMcr(IN P_ADAPTER_T prAdapter,
                        IN u32 u4Address,
                        IN u32 u4Value);

/*----------------------------------------------------------------------------*/
/* Update auto rate                                                           */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
nicRlmArUpdateParms(IN P_ADAPTER_T prAdapter,
                    IN u32 u4ArSysParam0,
                    IN u32 u4ArSysParam1,
                    IN u32 u4ArSysParam2,
                    IN u32 u4ArSysParam3);

/*----------------------------------------------------------------------------*/
/* Enable/Disable Roaming                                                     */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicRoamingUpdateParams(IN P_ADAPTER_T prAdapter,
                                   IN u32 u4EnableRoaming);

/*----------------------------------------------------------------------------*/
/* Link Quality Updating                                                      */
/*----------------------------------------------------------------------------*/
void nicUpdateLinkQuality(IN P_ADAPTER_T prAdapter,
                          IN u8 ucBssIndex,
                          IN P_EVENT_LINK_QUALITY_V2 prEventLinkQuality);

void nicUpdateRSSI(IN P_ADAPTER_T prAdapter,
                   IN u8 ucBssIndex,
                   IN s8 cRssi,
                   IN s8 cLinkQuality);

void nicUpdateLinkSpeed(IN P_ADAPTER_T prAdapter,
                        IN u8 ucBssIndex,
                        IN u16 u2LinkSpeed);

#if CFG_SUPPORT_RDD_TEST_MODE
WLAN_STATUS nicUpdateRddTestMode(IN P_ADAPTER_T prAdapter,
                                 IN P_CMD_RDD_CH_T prRddChParam);
#endif

/*----------------------------------------------------------------------------*/
/* Address Setting Apply                                                      */
/*----------------------------------------------------------------------------*/
WLAN_STATUS nicApplyNetworkAddress(IN P_ADAPTER_T prAdapter);

/*----------------------------------------------------------------------------*/
/* ECO Version                                                                */
/*----------------------------------------------------------------------------*/
u8 nicGetChipSwVer(void);
u8 nicGetChipEcoVer(IN P_ADAPTER_T prAdapter);
u8 nicIsEcoVerEqualTo(IN P_ADAPTER_T prAdapter, u8 ucEcoVer);
u8 nicIsEcoVerEqualOrLaterTo(IN P_ADAPTER_T prAdapter, u8 ucEcoVer);
u8 nicSetChipHwVer(u8 value);
u8 nicSetChipSwVer(u8 value);
u8 nicSetChipFactoryVer(u8 value);

void nicSerStopTxRx(IN P_ADAPTER_T prAdapter);
void nicSerStopTx(IN P_ADAPTER_T prAdapter);
void nicSerStartTxRx(IN P_ADAPTER_T prAdapter);
u8 nicSerIsWaitingReset(IN P_ADAPTER_T prAdapter);
u8 nicSerIsTxStop(IN P_ADAPTER_T prAdapter);
u8 nicSerIsRxStop(IN P_ADAPTER_T prAdapter);

void nicDumpMsduInfo(IN P_MSDU_INFO_T prMsduInfo);

#endif
