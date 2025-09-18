/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */
#ifndef _P2P_FUNC_H
#define _P2P_FUNC_H

#include "precomp.h"

#define P2P_OFF_CHNL_TX_DEFAULT_TIME_MS    1000

#if (CFG_SUPPORT_DFS_MASTER == 1)
extern struct P2P_RADAR_INFO g_rP2pRadarInfo;

enum _ENUM_DFS_STATE_T {
    DFS_STATE_INACTIVE = 0,
    DFS_STATE_CHECKING,
    DFS_STATE_ACTIVE,
    DFS_STATE_DETECTED,
    DFS_STATE_NUM
};
#endif

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#define ATTRI_ID(_fp)          (((P_P2P_ATTRIBUTE_T)_fp)->ucId)
#define ATTRI_LEN(_fp)                                       \
    (((u16)((u8 *)&((P_P2P_ATTRIBUTE_T)_fp)->u2Length)[0]) | \
     ((u16)((u8 *)&((P_P2P_ATTRIBUTE_T)_fp)->u2Length)[1] << 8))

#define ATTRI_SIZE(_fp)        (P2P_ATTRI_HDR_LEN + ATTRI_LEN(_fp))

#define P2P_ATTRI_FOR_EACH(_pucAttriBuf, _u2AttriBufLen, _u2Offset) \
    for ((_u2Offset) = 0; ((_u2Offset) < (_u2AttriBufLen));         \
         (_u2Offset) += ATTRI_SIZE(_pucAttriBuf),                   \
         ((_pucAttriBuf) += ATTRI_SIZE(_pucAttriBuf)))

#define P2P_IE(_fp)        ((P_IE_P2P_T)_fp)

#define WSC_ATTRI_ID(_fp)                                     \
    (((u16)((u8 *)&((P_WSC_ATTRIBUTE_T)_fp)->u2Id)[0] << 8) | \
     ((u16)((u8 *)&((P_WSC_ATTRIBUTE_T)_fp)->u2Id)[1]))

#define WSC_ATTRI_LEN(_fp)                                        \
    (((u16)((u8 *)&((P_WSC_ATTRIBUTE_T)_fp)->u2Length)[0] << 8) | \
     ((u16)((u8 *)&((P_WSC_ATTRIBUTE_T)_fp)->u2Length)[1]))

#define WSC_ATTRI_SIZE(_fp)    (WSC_ATTRI_HDR_LEN + WSC_ATTRI_LEN(_fp))

#define WSC_ATTRI_FOR_EACH(_pucAttriBuf, _u2AttriBufLen, _u2Offset) \
    for ((_u2Offset) = 0; ((_u2Offset) < (_u2AttriBufLen));         \
         (_u2Offset) += WSC_ATTRI_SIZE(_pucAttriBuf),               \
         ((_pucAttriBuf) += WSC_ATTRI_SIZE(_pucAttriBuf)))

#define WSC_IE(_fp)        ((P_IE_P2P_T)_fp)

#define WFD_ATTRI_SIZE(_fp)    (P2P_ATTRI_HDR_LEN + WSC_ATTRI_LEN(_fp))

#define WFD_ATTRI_FOR_EACH(_pucAttriBuf, _u2AttriBufLen, _u2Offset) \
    for ((_u2Offset) = 0; ((_u2Offset) < (_u2AttriBufLen));         \
         (_u2Offset) += WFD_ATTRI_SIZE(_pucAttriBuf),               \
         ((_pucAttriBuf) += WFD_ATTRI_SIZE(_pucAttriBuf)))

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void p2pFuncRequestScan(IN P_ADAPTER_T prAdapter,
                        IN u8 ucBssIndex,
                        IN P_P2P_SCAN_REQ_INFO_T prScanReqInfo);

void p2pFuncCancelScan(IN P_ADAPTER_T prAdapter,
                       IN u8 ucBssIndex,
                       IN P_P2P_SCAN_REQ_INFO_T prScanReqInfo);

void p2pFuncUpdateBssInfoForJOIN(IN P_ADAPTER_T prAdapter,
                                 IN P_BSS_DESC_T prBssDesc,
                                 IN P_STA_RECORD_T prStaRec,
                                 IN P_BSS_INFO_T prP2pBssInfo,
                                 IN P_SW_RFB_T prAssocRspSwRfb);

void p2pFuncAcquireCh(IN P_ADAPTER_T prAdapter,
                      IN u8 ucBssIdx,
                      IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo);

void p2pFuncDisconnect(IN P_ADAPTER_T prAdapter,
                       IN P_BSS_INFO_T prP2pBssInfo,
                       IN P_STA_RECORD_T prStaRec,
                       IN u8 fgSendDeauth,
                       IN u16 u2ReasonCode,
                       IN u8 fgIsLocallyGenerated);

P_BSS_INFO_T p2pFuncBSSIDFindBssInfo(IN P_ADAPTER_T prAdapter, IN u8 *pucBSSID);

/* //////////////////////////////// MT6630 CODE
 * //////////////////////////////////// */

void p2pFuncGCJoin(IN P_ADAPTER_T prAdapter,
                   IN P_BSS_INFO_T prP2pBssInfo,
                   IN P_P2P_JOIN_INFO_T prP2pJoinInfo);

void p2pFuncStopComplete(IN P_ADAPTER_T prAdapter,
                         IN P_BSS_INFO_T prP2pBssInfo);

void p2pFuncStartGO(IN P_ADAPTER_T prAdapter,
                    IN P_BSS_INFO_T prBssInfo,
                    IN P_P2P_CONNECTION_REQ_INFO_T prP2pConnReqInfo,
                    IN P_P2P_CHNL_REQ_INFO_T prP2pChnlReqInfo);

void p2pFuncStopGO(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prP2pBssInfo);

WLAN_STATUS p2pFuncRoleToBssIdx(IN P_ADAPTER_T prAdapter,
                                IN u8 ucRoleIdx,
                                OUT u8 *pucBssIdx);

P_P2P_ROLE_FSM_INFO_T p2pFuncGetRoleByBssIdx(IN P_ADAPTER_T prAdapter,
                                             IN u8 ucBssIndex);

void p2pFuncSwitchOPMode(IN P_ADAPTER_T prAdapter,
                         IN P_BSS_INFO_T prP2pBssInfo,
                         IN ENUM_OP_MODE_T eOpMode,
                         IN u8 fgSyncToFW);

void p2pFuncReleaseCh(IN P_ADAPTER_T prAdapter,
                      IN u8 ucBssIdx,
                      IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo);

#if (CFG_SUPPORT_DFS_MASTER == 1)
void p2pFuncStartRdd(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx);

void p2pFuncStopRdd(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx);

void p2pFuncDfsSwitchCh(IN P_ADAPTER_T prAdapter,
                        IN P_BSS_INFO_T prBssInfo,
                        IN P2P_CHNL_REQ_INFO_T rP2pChnlReqInfo);

u8 p2pFuncCheckWeatherRadarBand(IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo);

s32 p2pFuncSetDriverCacTime(IN u32 u4CacTime);

void p2pFuncEnableManualCac(void);

u32 p2pFuncGetDriverCacTime(void);

u8 p2pFuncIsManualCac(void);

void p2pFuncRadarInfoInit(void);

void p2pFuncShowRadarInfo(IN P_ADAPTER_T prAdapter, IN u8 ucBssIdx);

void p2pFuncGetRadarInfo(IN struct P2P_RADAR_INFO *prP2pRadarInfo);

u8 *p2pFuncJpW53RadarType(void);

u8 *p2pFuncJpW56RadarType(void);

void p2pFuncSetRadarDetectMode(IN u8 ucRadarDetectMode);

u8 p2pFuncGetRadarDetectMode(void);

void p2pFuncSetDfsState(IN u8 ucDfsState);

u8 p2pFuncGetDfsState(void);

u8 *p2pFuncShowDfsState(void);

void p2pFuncRecordCacStartBootTime(void);

u32 p2pFuncGetCacRemainingTime(void);
#endif

void p2pFuncSetChannel(IN P_ADAPTER_T prAdapter,
                       IN u8 ucRoleIdx,
                       IN P_RF_CHANNEL_INFO_T prRfChannelInfo);

u8 p2pFuncRetryJOIN(IN P_ADAPTER_T prAdapter,
                    IN P_STA_RECORD_T prStaRec,
                    IN P_P2P_JOIN_INFO_T prJoinInfo);

WLAN_STATUS
p2pFuncTxMgmtFrame(IN P_ADAPTER_T prAdapter,
                   IN u8 ucBssIndex,
                   IN P_MSDU_INFO_T prMgmtTxMsdu,
                   IN u8 fgNonCckRate);

WLAN_STATUS
p2pFuncBeaconUpdate(IN P_ADAPTER_T prAdapter,
                    IN P_BSS_INFO_T prP2pBssInfo,
                    IN P_P2P_BEACON_UPDATE_INFO_T prBcnUpdateInfo,
                    IN u8 *pucNewBcnHdr,
                    IN u32 u4NewHdrLen,
                    IN u8 *pucNewBcnBody,
                    IN u32 u4NewBodyLen);

WLAN_STATUS
p2pFuncAssocRespUpdate(IN P_ADAPTER_T prAdapter,
                       IN P_BSS_INFO_T prP2pBssInfo,
                       IN u8 *AssocRespIE,
                       IN u32 u4AssocRespLen);

u8 p2pFuncValidateAuth(IN P_ADAPTER_T prAdapter,
                       IN P_BSS_INFO_T prBssInfo,
                       IN P_SW_RFB_T prSwRfb,
                       IN PP_STA_RECORD_T pprStaRec,
                       OUT u16 *pu2StatusCode);

u8 p2pFuncValidateAssocReq(IN P_ADAPTER_T prAdapter,
                           IN P_SW_RFB_T prSwRfb,
                           OUT u16 *pu2StatusCode);

void p2pFuncResetStaRecStatus(IN P_ADAPTER_T prAdapter,
                              IN P_STA_RECORD_T prStaRec);

void p2pFuncInitConnectionSettings(IN P_ADAPTER_T prAdapter,
                                   IN P_P2P_CONNECTION_SETTINGS_T
                                   prP2PConnSettings,
                                   IN u8 fgIsApMode);

u8 p2pFuncParseCheckForTKIPInfoElem(IN u8 *pucBuf);

u8 p2pFuncParseCheckForP2PInfoElem(IN P_ADAPTER_T prAdapter,
                                   IN u8 *pucBuf,
                                   OUT u8 *pucOuiType);

u8 p2pFuncValidateProbeReq(IN P_ADAPTER_T prAdapter,
                           IN P_SW_RFB_T prSwRfb,
                           OUT u32 *pu4ControlFlags,
                           IN u8 fgIsDevInterface,
                           IN u8 ucRoleIdx);

void p2pFuncValidateRxActionFrame(IN P_ADAPTER_T prAdapter,
                                  IN P_SW_RFB_T prSwRfb,
                                  IN u8 fgIsDevInterface,
                                  IN u8 ucRoleIdx);

u8 p2pFuncIsAPMode(IN P_P2P_CONNECTION_SETTINGS_T prP2pConnSettings);

void p2pFuncParseBeaconContent(IN P_ADAPTER_T prAdapter,
                               IN P_BSS_INFO_T prP2pBssInfo,
                               IN u8 *pucIEInfo,
                               IN u32 u4IELen);

P_BSS_DESC_T
p2pFuncKeepOnConnection(IN P_ADAPTER_T prAdapter,
                        IN P_BSS_INFO_T prBssInfo,
                        IN P_P2P_CONNECTION_REQ_INFO_T prConnReqInfo,
                        IN P_P2P_CHNL_REQ_INFO_T prChnlReqInfo,
                        IN P_P2P_SCAN_REQ_INFO_T prScanReqInfo);

void p2pFuncStoreAssocRspIEBuffer(IN P_ADAPTER_T prAdapter,
                                  IN P_P2P_JOIN_INFO_T prP2pJoinInfo,
                                  IN P_SW_RFB_T prSwRfb);

void p2pFuncMgmtFrameRegister(IN P_ADAPTER_T prAdapter,
                              IN u16 u2FrameType,
                              IN u8 fgIsRegistered,
                              OUT u32 *pu4P2pPacketFilter);

void p2pFuncUpdateMgmtFrameRegister(IN P_ADAPTER_T prAdapter,
                                    IN u32 u4OsFilter);

void p2pFuncGetStationInfo(IN P_ADAPTER_T prAdapter,
                           IN u8 *pucMacAddr,
                           OUT P_P2P_STATION_INFO_T prStaInfo);

P_MSDU_INFO_T p2pFuncProcessP2pProbeRsp(IN P_ADAPTER_T prAdapter,
                                        IN u8 ucBssIdx,
                                        IN P_MSDU_INFO_T prMgmtTxMsdu);

u32 p2pFuncCalculateP2p_IELenForBeacon(IN P_ADAPTER_T prAdapter,
                                       IN u8 ucBssIndex,
                                       IN P_STA_RECORD_T prStaRec);

void p2pFuncGenerateP2p_IEForBeacon(IN P_ADAPTER_T prAdapter,
                                    IN P_MSDU_INFO_T prMsduInfo);

u32 p2pFuncCalculateWSC_IELenForBeacon(IN P_ADAPTER_T prAdapter,
                                       IN u8 ucBssIndex,
                                       IN P_STA_RECORD_T prStaRec);

void p2pFuncGenerateWSC_IEForBeacon(IN P_ADAPTER_T prAdapter,
                                    IN P_MSDU_INFO_T prMsduInfo);

u32 p2pFuncCalculateP2p_IELenForAssocRsp(IN P_ADAPTER_T prAdapter,
                                         IN u8 ucBssIndex,
                                         IN P_STA_RECORD_T prStaRec);

void p2pFuncGenerateP2p_IEForAssocRsp(IN P_ADAPTER_T prAdapter,
                                      IN P_MSDU_INFO_T prMsduInfo);

u32 p2pFuncCalculateP2P_IELen(IN P_ADAPTER_T prAdapter,
                              IN u8 ucBssIndex,
                              IN P_STA_RECORD_T prStaRec,
                              IN APPEND_VAR_ATTRI_ENTRY_T arAppendAttriTable[],
                              IN u32 u4AttriTableSize);

void p2pFuncGenerateP2P_IE(IN P_ADAPTER_T prAdapter,
                           IN u8 ucBssIndex,
                           IN u8 fgIsAssocFrame,
                           IN u16 *pu2Offset,
                           IN u8 *pucBuf,
                           IN u16 u2BufSize,
                           IN APPEND_VAR_ATTRI_ENTRY_T arAppendAttriTable[],
                           IN u32 u4AttriTableSize);

u32 p2pFuncAppendAttriStatusForAssocRsp(IN P_ADAPTER_T prAdapter,
                                        IN u8 ucBssIndex,
                                        IN u8 fgIsAssocFrame,
                                        IN u16 *pu2Offset,
                                        IN u8 *pucBuf,
                                        IN u16 u2BufSize);

u32 p2pFuncAppendAttriExtListenTiming(IN P_ADAPTER_T prAdapter,
                                      IN u8 ucBssIndex,
                                      IN u8 fgIsAssocFrame,
                                      IN u16 *pu2Offset,
                                      IN u8 *pucBuf,
                                      IN u16 u2BufSize);

void p2pFuncDissolve(IN P_ADAPTER_T prAdapter,
                     IN P_BSS_INFO_T prP2pBssInfo,
                     IN u8 fgSendDeauth,
                     IN u16 u2ReasonCode,
                     IN u8 fgIsLocallyGenerated);

P_IE_HDR_T
p2pFuncGetSpecIE(IN P_ADAPTER_T prAdapter,
                 IN u8 *pucIEBuf,
                 IN u16 u2BufferLen,
                 IN u8 ucElemID,
                 IN u8 *pfgIsMore);

P_ATTRIBUTE_HDR_T
p2pFuncGetSpecAttri(IN P_ADAPTER_T prAdapter,
                    IN u8 ucOuiType,
                    IN u8 *pucIEBuf,
                    IN u16 u2BufferLen,
                    IN u8 ucAttriID);

u32 wfdFuncCalculateWfdIELenForAssocRsp(IN P_ADAPTER_T prAdapter,
                                        IN u8 ucBssIndex,
                                        IN P_STA_RECORD_T prStaRec);

void wfdFuncGenerateWfdIEForAssocRsp(IN P_ADAPTER_T prAdapter,
                                     IN P_MSDU_INFO_T prMsduInfo);

u32 p2pFuncCalculateP2P_IE_NoA(IN P_ADAPTER_T prAdapter,
                               IN u8 ucBssIdx,
                               IN P_STA_RECORD_T prStaRec);

void p2pFuncGenerateP2P_IE_NoA(IN P_ADAPTER_T prAdapter,
                               IN P_MSDU_INFO_T prMsduInfo);

void p2pFuncClassifyAction(IN P_SW_RFB_T prSwRfb);

#if CFG_SUPPORT_DBDC_TC6
void p2pFuncModifyChandef(IN P_ADAPTER_T prAdapter,
                          IN P_GL_P2P_INFO_T prGlueP2pInfo,
                          IN P_BSS_INFO_T prBssInfo);
u8 p2pFuncSwitchSapChannel(IN P_ADAPTER_T prAdapter);
#endif

#endif
