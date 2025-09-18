/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "rlm.h"
 *    \brief
 */

#ifndef _RLM_H
#define _RLM_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

extern u8 g_bIcapEnable;
extern u8 g_bCaptureDone;
extern u16 g_u2DumpIndex;
#if CFG_SUPPORT_QA_TOOL
extern u32 g_au4Offset[2][2];
extern u32 g_au4IQData[256];
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define ELEM_EXT_CAP_DEFAULT_VAL \
    (ELEM_EXT_CAP_20_40_COEXIST_SUPPORT  /*| ELEM_EXT_CAP_PSMP_CAP*/ )

#if CFG_SUPPORT_RX_STBC
#define FIELD_HT_CAP_INFO_RX_STBC    HT_CAP_INFO_RX_STBC_1_SS
#else
#define FIELD_HT_CAP_INFO_RX_STBC    HT_CAP_INFO_RX_STBC_NO_SUPPORTED
#endif

#if CFG_SUPPORT_RX_SGI
#define FIELD_HT_CAP_INFO_SGI_20M    HT_CAP_INFO_SHORT_GI_20M
#define FIELD_HT_CAP_INFO_SGI_40M    HT_CAP_INFO_SHORT_GI_40M
#else
#define FIELD_HT_CAP_INFO_SGI_20M    0
#define FIELD_HT_CAP_INFO_SGI_40M    0
#endif

#if CFG_SUPPORT_RX_HT_GF
#define FIELD_HT_CAP_INFO_HT_GF      HT_CAP_INFO_HT_GF
#else
#define FIELD_HT_CAP_INFO_HT_GF      0
#endif

#define HT_CAP_INFO_DEFAULT_VAL                                 \
    (HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_DSSS_CCK_IN_40M | \
     HT_CAP_INFO_SM_POWER_SAVE)

#define AMPDU_PARAM_DEFAULT_VAL \
    (AMPDU_PARAM_MAX_AMPDU_LEN_64K | AMPDU_PARAM_MSS_NO_RESTRICIT)

#define SUP_MCS_TX_DEFAULT_VAL \
    SUP_MCS_TX_SET_DEFINED  /* TX defined and TX/RX equal (TBD) */

#if CFG_SUPPORT_MFB
#define FIELD_HT_EXT_CAP_MFB              HT_EXT_CAP_MCS_FEEDBACK_BOTH
#else
#define FIELD_HT_EXT_CAP_MFB              HT_EXT_CAP_MCS_FEEDBACK_NO_FB
#endif

#if CFG_SUPPORT_RX_RDG
#define FIELD_HT_EXT_CAP_RDR              HT_EXT_CAP_RD_RESPONDER
#else
#define FIELD_HT_EXT_CAP_RDR              0
#endif

#if CFG_SUPPORT_MFB || CFG_SUPPORT_RX_RDG
#define FIELD_HT_EXT_CAP_HTC              HT_EXT_CAP_HTC_SUPPORT
#else
#define FIELD_HT_EXT_CAP_HTC              0
#endif

#define HT_EXT_CAP_DEFAULT_VAL                         \
    (HT_EXT_CAP_PCO | HT_EXT_CAP_PCO_TRANS_TIME_NONE | \
     FIELD_HT_EXT_CAP_MFB | FIELD_HT_EXT_CAP_HTC | FIELD_HT_EXT_CAP_RDR)

#define TX_BEAMFORMING_CAP_DEFAULT_VAL        0

#if CFG_SUPPORT_BFEE
#define TX_BEAMFORMING_CAP_BFEE                            \
    (TXBF_RX_NDP_CAPABLE |                                 \
     TXBF_EXPLICIT_COMPRESSED_FEEDBACK_IMMEDIATE_CAPABLE | \
     TXBF_MINIMAL_GROUPING_1_2_3_CAPABLE |                 \
     TXBF_COMPRESSED_TX_ANTENNANUM_4_SUPPORTED |           \
     TXBF_CHANNEL_ESTIMATION_4STS_CAPABILITY)
#else
#define TX_BEAMFORMING_CAP_BFEE           0
#endif

#if CFG_SUPPORT_BFER
#define TX_BEAMFORMING_CAP_BFER \
    (TXBF_TX_NDP_CAPABLE | TXBF_EXPLICIT_COMPRESSED_TX_CAPAB)
#else
#define TX_BEAMFORMING_CAP_BFER           0
#endif

#define ASEL_CAP_DEFAULT_VAL              0

/* Define bandwidth from user setting */
#define CONFIG_BW_20_40M              0
#define CONFIG_BW_20M                 1  /* 20MHz only */

#if CFG_SUPPORT_BFER
#define MODE_LEGACY               0
#define MODE_HT                   1
#define MODE_VHT                  2
#endif

#if CFG_SUPPORT_802_11AC
#if CFG_SUPPORT_BFEE
#define FIELD_VHT_CAP_INFO_BFEE           ( \
        VHT_CAP_INFO_SU_BEAMFORMEE_CAPABLE)
#define VHT_CAP_INFO_BEAMFORMEE_STS_CAP_MAX   3
#else
#define FIELD_VHT_CAP_INFO_BFEE           0
#endif

#if CFG_SUPPORT_BFER
#define FIELD_VHT_CAP_INFO_BFER           \
    (VHT_CAP_INFO_SU_BEAMFORMER_CAPABLE | \
     VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS_2_SUPPORTED)
#else
#define FIELD_VHT_CAP_INFO_BFER           0
#endif

#define VHT_CAP_INFO_DEFAULT_VAL      \
    (VHT_CAP_INFO_MAX_MPDU_LEN_3K |   \
     (AMPDU_PARAM_MAX_AMPDU_LEN_1024K \
        << VHT_CAP_INFO_MAX_AMPDU_LENGTH_OFFSET))

#define VHT_CAP_INFO_DEFAULT_HIGHEST_DATA_RATE    0
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

#if CFG_SUPPORT_DFS
typedef struct _SWITCH_CH_AND_BAND_PARAMS_T {
    u8 fgBeaconNewChannelIsDFS;
    u8 fgActionNewChannelIsDFS;
    u8 fgNewChannelIsDisabled;
    u8 ucCsaNewCh;
    u8 ucCsaCount;
    u8 ucVhtS1;
    u8 ucVhtS2;
    u8 ucVhtBw;
    ENUM_CHNL_EXT_T eSco;
    u8 ucBssIndex;
} SWITCH_CH_AND_BAND_PARAMS_T, *P_SWITCH_CH_AND_BAND_PARAMS_T;
#endif

struct SUB_ELEMENT_LIST {
    struct SUB_ELEMENT_LIST *prNext;
    struct SUB_ELEMENT rSubIE;
};

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

/* It is used for RLM module to judge if specific network is valid
 * Note: Ad-hoc mode of AIS is not included now. (TBD)
 */
#define RLM_NET_PARAM_VALID(_prBssInfo)                                \
    (IS_BSS_ACTIVE(_prBssInfo) &&                                      \
     ((_prBssInfo)->eConnectionState == PARAM_MEDIA_STATE_CONNECTED || \
      (_prBssInfo)->eCurrentOPMode == OP_MODE_ACCESS_POINT ||          \
      (_prBssInfo)->eCurrentOPMode == OP_MODE_IBSS ||                  \
      IS_BSS_BOW(_prBssInfo)))

#define RLM_NET_IS_11N(_prBssInfo) \
    ((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11N)
#define RLM_NET_IS_11GN(_prBssInfo) \
    ((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11GN)

#if CFG_SUPPORT_802_11AC
#define RLM_NET_IS_11AC(_prBssInfo) \
    ((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11AC)
#endif

/* The bandwidth modes are not used anymore. They represent if AP
 * can use 20/40 bandwidth, not all modes. (20110411)
 */
#define RLM_AP_IS_BW_40_ALLOWED(_prAdapter, _prBssInfo)          \
    (((_prBssInfo)->eBand == BAND_2G4 &&                         \
      (_prAdapter)->rWifiVar.rConnSettings.uc2G4BandwidthMode == \
      CONFIG_BW_20_40M) ||                                       \
     ((_prBssInfo)->eBand == BAND_5G &&                          \
      (_prAdapter)->rWifiVar.rConnSettings.uc5GBandwidthMode ==  \
      CONFIG_BW_20_40M))

#if CFG_SUPPORT_DFS
#define MAX_CSA_COUNT    255
#define HAS_CH_SWITCH_PARAMS(prCSAParams, prBssDesc) \
    (prCSAParams->ucCsaNewCh > 0 &&                  \
     prCSAParams->ucCsaNewCh != prBssDesc->ucChannelNum)
#define HAS_SCO_PARAMS(prCSAParams)    (prCSAParams->eSco > 0)
#define HAS_WIDE_BAND_PARAMS(prCSAParams)                    \
    (prCSAParams->ucVhtBw > 0 || prCSAParams->ucVhtS1 > 0 || \
     prCSAParams->ucVhtS2 > 0)
#define SHOULD_CH_SWITCH(current, prCSAParams, prBssDesc) \
    (HAS_CH_SWITCH_PARAMS(prCSAParams, prBssDesc) &&      \
     (current < prCSAParams->ucCsaCount))
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void rlmFsmEventInit(P_ADAPTER_T prAdapter);

void rlmFsmEventUninit(P_ADAPTER_T prAdapter);

void rlmReqGeneratePowerCapIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rlmReqGenerateSupportedChIE(P_ADAPTER_T prAdapter,
                                 P_MSDU_INFO_T prMsduInfo);

void rlmReqGenerateHtCapIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rlmReqGenerateExtCapIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rlmRspGenerateHtCapIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rlmRspGenerateExtCapIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rlmRspGenerateHtOpIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rlmRspGenerateErpIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rlmGenerateMTKOuiIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

u8 rlmParseCheckMTKOuiIE(IN P_ADAPTER_T prAdapter, IN u8 *pucBuf,
                         IN u32 *pu4Cap);

void rlmGenerateCsaIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rlmProcessBcn(P_ADAPTER_T prAdapter,
                   P_SW_RFB_T prSwRfb,
                   u8 *pucIE,
                   u16 u2IELength);

void rlmProcessAssocRsp(P_ADAPTER_T prAdapter,
                        P_SW_RFB_T prSwRfb,
                        u8 *pucIE,
                        u16 u2IELength);

void rlmProcessHtAction(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb);

#if CFG_SUPPORT_802_11AC
void rlmProcessVhtAction(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb);
#endif

void rlmFillSyncCmdParam(P_CMD_SET_BSS_RLM_PARAM_T prCmdBody,
                         P_BSS_INFO_T prBssInfo);

void rlmSyncOperationParams(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);

void rlmBssInitForAPandIbss(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);

void rlmProcessAssocReq(P_ADAPTER_T prAdapter,
                        P_SW_RFB_T prSwRfb,
                        u8 *pucIE,
                        u16 u2IELength);

void rlmBssAborted(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);

#if CFG_SUPPORT_TDLS
u32 rlmFillHtCapIEByParams(u8 fg40mAllowed,
                           u8 fgShortGIDisabled,
                           u8 u8SupportRxSgi20,
                           u8 u8SupportRxSgi40,
                           u8 u8SupportRxGf,
                           ENUM_OP_MODE_T eCurrentOPMode,
                           u8 *pOutBuf);

u32 rlmFillHtCapIEByAdapter(P_ADAPTER_T prAdapter,
                            P_BSS_INFO_T prBssInfo,
                            u8 *pOutBuf);

u32 rlmFillVhtCapIEByAdapter(P_ADAPTER_T prAdapter,
                             P_BSS_INFO_T prBssInfo,
                             u8 *pOutBuf);

#endif

#if CFG_SUPPORT_802_11AC
void rlmReqGenerateVhtCapIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rlmRspGenerateVhtCapIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rlmRspGenerateVhtOpIE(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

void rlmFillVhtOpIE(P_ADAPTER_T prAdapter,
                    P_BSS_INFO_T prBssInfo,
                    P_MSDU_INFO_T prMsduInfo);

void rlmRspGenerateVhtOpNotificationIE(P_ADAPTER_T prAdapter,
                                       P_MSDU_INFO_T prMsduInfo);
void rlmReqGenerateVhtOpNotificationIE(P_ADAPTER_T prAdapter,
                                       P_MSDU_INFO_T prMsduInfo);

#endif

#if CFG_SUPPORT_DFS
void rlmProcessSpecMgtAction(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb);

void rlmResetCSAParams(P_BSS_INFO_T prBssInfo);

void rlmCsaTimeout(IN P_ADAPTER_T prAdapter, unsigned long ulParamPtr);
#endif

void rlmSendOpModeNotificationFrame(P_ADAPTER_T prAdapter,
                                    P_STA_RECORD_T prStaRec,
                                    u8 ucChannelWidth,
                                    u8 ucNss);

void rlmSendSmPowerSaveFrame(P_ADAPTER_T prAdapter,
                             P_STA_RECORD_T prStaRec,
                             u8 ucNss);

void rlmChangeVhtOpBwPara(P_ADAPTER_T prAdapter, u8 ucBssIndex,
                          u8 ucChannelWidth);

u8 rlmChangeOperationMode(P_ADAPTER_T prAdapter,
                          u8 ucBssIndex,
                          u8 ucChannelWidth,
                          u8 ucNss);

#if CFG_SUPPORT_BFER
void rlmBfStaRecPfmuUpdate(P_ADAPTER_T prAdapter, P_STA_RECORD_T prStaRec);

void rlmETxBfTriggerPeriodicSounding(P_ADAPTER_T prAdapter);

u8 rlmClientSupportsVhtETxBF(P_STA_RECORD_T prStaRec);

u8 rlmClientSupportsVhtBfeeStsCap(P_STA_RECORD_T prStaRec);

u8 rlmClientSupportsHtETxBF(P_STA_RECORD_T prStaRec);
#endif

#if CFG_SUPPORT_DBDC_TC6
void rlmSendChannelSwitchFrame(P_ADAPTER_T prAdapter, u8 ucBssIndex);

u8 rlmGetBssOpBwByVhtAndHtOpInfo(P_BSS_INFO_T prBssInfo);

u8 rlmGetVhtOpBwByBssOpBw(u8 ucBssOpBw);
#endif

void rlmModifyVhtBwPara(u8 *pucVhtChannelFrequencyS1,
                        u8 *pucVhtChannelFrequencyS2,
                        u8 *pucVhtChannelWidth);

void rlmReviseMaxBw(P_ADAPTER_T prAdapter,
                    u8 ucBssIndex,
                    P_ENUM_CHNL_EXT_T peExtend,
                    u8 *peChannelWidth,
                    u8 *pucS1,
                    u8 *pucPrimaryCh);

void rlmRevisePreferBandwidthNss(P_ADAPTER_T prAdapter,
                                 u8 ucBssIndex,
                                 P_STA_RECORD_T prStaRec);

#if CFG_SUPPORT_802_11K
void rlmReqGenerateRRMEnabledCapIE(P_ADAPTER_T prAdapter,
                                   P_MSDU_INFO_T prMsduInfo);

void rlmFillRrmCapa(u8 *pucCapa);

void rlmTxNeighborReportRequest(P_ADAPTER_T prAdapter,
                                P_STA_RECORD_T prStaRec,
                                struct SUB_ELEMENT_LIST *prSubIEs);

void rlmProcessNeighborReportResponse(P_ADAPTER_T prAdapter,
                                      P_WLAN_ACTION_FRAME prAction,
                                      u16 u2PacketLen);
#endif

#if CFG_SUPPORT_QUIET
void rrmQuietIeNotExist(P_ADAPTER_T prAdapter, P_BSS_INFO_T prBssInfo);

void rrmQuietHandleQuietIE(P_ADAPTER_T prAdapter,
                           P_BSS_INFO_T prBssInfo,
                           P_IE_QUIET_T pucQUIE);

void rrmTxQuietTimeout(P_ADAPTER_T prAdapter, unsigned long ulParamPtr);

#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
