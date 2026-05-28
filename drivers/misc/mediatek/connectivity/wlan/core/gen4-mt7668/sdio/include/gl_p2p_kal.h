/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_p2p_kal.h
 *    \brief  Declaration of KAL functions for Wi-Fi Direct support
 *     - kal*() which is provided by GLUE Layer.
 *
 *    Any definitions in this file will be shared among GLUE Layer and internal
 * Driver Stack.
 */

#ifndef _GL_P2P_KAL_H
#define _GL_P2P_KAL_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "config.h"
#include "gl_typedef.h"
#include "gl_os.h"
#include "wlan_lib.h"
#include "wlan_oid.h"
#include "wlan_p2p.h"
#include "gl_kal.h"
#include "gl_wext_priv.h"
#include "gl_p2p_ioctl.h"
#include "nic/p2p.h"

#if DBG
extern int allocatedMemSize;
#endif

u8 kalP2pFuncGetChannelType(IN ENUM_CHNL_EXT_T rChnlSco,
                            OUT enum nl80211_channel_type *channel_type);

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
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
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/* Service Discovery */
void kalP2PIndicateSDRequest(IN P_GLUE_INFO_T prGlueInfo,
                             IN PARAM_MAC_ADDRESS rPeerAddr,
                             IN u8 ucSeqNum);

void kalP2PIndicateSDResponse(IN P_GLUE_INFO_T prGlueInfo,
                              IN PARAM_MAC_ADDRESS rPeerAddr,
                              IN u8 ucSeqNum);

void kalP2PIndicateTXDone(IN P_GLUE_INFO_T prGlueInfo,
                          IN u8 ucSeqNum,
                          IN u8 ucStatus);

/*----------------------------------------------------------------------------*/
/* Wi-Fi Direct handling                                                      */
/*----------------------------------------------------------------------------*/
/*ENUM_PARAM_MEDIA_STATE_T kalP2PGetState(IN P_GLUE_INFO_T prGlueInfo);*/

/*void
 * kalP2PSetState(IN P_GLUE_INFO_T prGlueInfo,
 *        IN ENUM_PARAM_MEDIA_STATE_T eState, IN PARAM_MAC_ADDRESS rPeerAddr, IN
 * u8 ucRole);
 */

void kalP2PUpdateAssocInfo(IN P_GLUE_INFO_T prGlueInfo,
                           IN u8 *pucFrameBody,
                           IN u32 u4FrameBodyLen,
                           IN u8 fgReassocRequest,
                           IN u8 ucBssIndex);

/*u32 kalP2PGetFreqInKHz(IN P_GLUE_INFO_T prGlueInfo);*/

s32 mtk_Netdev_To_RoleIdx(P_GLUE_INFO_T prGlueInfo,
                          struct net_device *ndev,
                          u8 *pucRoleIdx);

u8 kalP2PGetRole(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx);

void kalP2PSetRole(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRole, IN u8 ucRoleIdx);

void kalP2PSetCipher(IN P_GLUE_INFO_T prGlueInfo,
                     IN u32 u4Cipher,
                     IN u8 ucRoleIdx);

u8 kalP2PGetCipher(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx);

u8 kalP2PGetWepCipher(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx);

u8 kalP2PGetTkipCipher(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx);

u8 kalP2PGetCcmpCipher(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx);

#if CFG_SUPPORT_SUITB
u8 kalP2PGetGcmp256Cipher(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIdx);
#endif

void kalP2PSetWscMode(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucWscMode);

u8 kalP2PGetWscMode(IN P_GLUE_INFO_T prGlueInfo);

u16 kalP2PCalWSC_IELen(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucType,
                       IN u8 ucRoleIdx);

void kalP2PGenWSC_IE(IN P_GLUE_INFO_T prGlueInfo,
                     IN u8 ucType,
                     IN u8 *pucBuffer,
                     IN u8 ucRoleIdx);

void kalP2PUpdateWSC_IE(IN P_GLUE_INFO_T prGlueInfo,
                        IN u8 ucType,
                        IN u8 *pucBuffer,
                        IN u16 u2BufferLength,
                        IN u8 ucRoleIdx);

u8 kalP2PIndicateFound(IN P_GLUE_INFO_T prGlueInfo);

void kalP2PIndicateConnReq(IN P_GLUE_INFO_T prGlueInfo,
                           IN u8 *pucDevName,
                           IN s32 u4NameLength,
                           IN PARAM_MAC_ADDRESS rPeerAddr,
                           IN u8 ucDevType,
                           /* 0: P2P Device / 1: GC / 2: GO */
                           IN s32 i4ConfigMethod,
                           IN s32 i4ActiveConfigMethod);

/*void kalP2PInvitationStatus(IN P_GLUE_INFO_T prGlueInfo, IN u32
 * u4InvStatus);*/

void kalP2PInvitationIndication(IN P_GLUE_INFO_T prGlueInfo,
                                IN P_P2P_DEVICE_DESC_T prP2pDevDesc,
                                IN u8 *pucSsid,
                                IN u8 ucSsidLen,
                                IN u8 ucOperatingChnl,
                                IN u8 ucInvitationType,
                                IN u8 *pucGroupBssid);

struct net_device *kalP2PGetDevHdlr(P_GLUE_INFO_T prGlueInfo);

void kalGetChnlList(IN P_GLUE_INFO_T prGlueInfo,
                    IN ENUM_BAND_T eSpecificBand,
                    IN u8 ucMaxChannelNum,
                    IN u8 *pucNumOfChannel,
                    IN P_RF_CHANNEL_INFO_T paucChannelList);

#if CFG_SUPPORT_ANTI_PIRACY
void kalP2PIndicateSecCheckRsp(IN P_GLUE_INFO_T prGlueInfo,
                               IN u8 *pucRsp,
                               IN u16 u2RspLen);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

void kalP2PIndicateChannelReady(IN P_GLUE_INFO_T prGlueInfo,
                                IN u64 u8SeqNum,
                                IN u32 u4ChannelNum,
                                IN ENUM_BAND_T eBand,
                                IN ENUM_CHNL_EXT_T eSco,
                                IN u32 u4Duration);

void kalP2PIndicateScanDone(IN P_GLUE_INFO_T prGlueInfo,
                            IN u8 ucRoleIndex,
                            IN u8 fgIsAbort);

void kalP2PIndicateBssInfo(IN P_GLUE_INFO_T prGlueInfo,
                           IN u8 *pucFrameBuf,
                           IN u32 u4BufLen,
                           IN P_RF_CHANNEL_INFO_T prChannelInfo,
                           IN s32 i4SignalStrength);

void kalP2PIndicateRxMgmtFrame(IN P_GLUE_INFO_T prGlueInfo,
                               IN P_SW_RFB_T prSwRfb,
                               IN u8 fgIsDevInterface,
                               IN u8 ucRoleIdx);

void kalP2PIndicateMgmtTxStatus(IN P_GLUE_INFO_T prGlueInfo,
                                IN P_MSDU_INFO_T prMsduInfo,
                                IN u8 fgIsAck);

void kalP2PIndicateChannelExpired(IN P_GLUE_INFO_T prGlueInfo,
                                  IN u64 u8SeqNum,
                                  IN u32 u4ChannelNum,
                                  IN ENUM_BAND_T eBand,
                                  IN ENUM_CHNL_EXT_T eSco);

void kalP2PGCIndicateConnectionStatus(IN P_GLUE_INFO_T prGlueInfo,
                                      IN u8 ucRoleIndex,
                                      IN P_P2P_CONNECTION_REQ_INFO_T
                                      prP2pConnInfo,
                                      IN u8 *pucRxIEBuf,
                                      IN u16 u2RxIELen,
                                      IN u16 u2StatusReason,
                                      IN WLAN_STATUS eStatus);

void kalP2PGOStationUpdate(IN P_GLUE_INFO_T prGlueInfo,
                           IN u8 ucRoleIndex,
                           IN P_STA_RECORD_T prCliStaRec,
                           IN u8 fgIsNew);

#if (CFG_SUPPORT_DFS_MASTER == 1)
void kalP2PRddDetectUpdate(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIndex);

void kalP2PCacFinishedUpdate(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucRoleIndex);
#endif

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER

u8 kalP2PSetBlackList(IN P_GLUE_INFO_T prGlueInfo,
                      IN PARAM_MAC_ADDRESS rbssid,
                      IN u8 fgIsblock,
                      IN u8 ucRoleIndex);

u8 kalP2PCmpBlackList(IN P_GLUE_INFO_T prGlueInfo,
                      IN PARAM_MAC_ADDRESS rbssid,
                      IN u8 ucRoleIndex);

void kalP2PSetMaxClients(IN P_GLUE_INFO_T prGlueInfo,
                         IN u32 u4MaxClient,
                         IN u8 ucRoleIndex);

u8 kalP2PMaxClients(IN P_GLUE_INFO_T prGlueInfo,
                    IN u32 u4NumClient,
                    IN u8 ucRoleIndex);

#endif

#endif
