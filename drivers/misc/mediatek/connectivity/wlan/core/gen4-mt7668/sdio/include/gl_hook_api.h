/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_hook_api.h
 *    \brief  This file includes private ioctl support.
 */

#ifndef _GL_HOOK_API_H
#define _GL_HOOK_API_H
#if CFG_SUPPORT_QA_TOOL
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
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

s32 MT_ATEStart(struct net_device *prNetDev, u8 *prInBuf);
s32 MT_ICAPStart(struct net_device *prNetDev, u8 *prInBuf);
s32 MT_ATEStop(struct net_device *prNetDev, u8 *prInBuf);
s32 MT_ATEStartTX(struct net_device *prNetDev, u8 *prInBuf);
s32 MT_ATEStopTX(struct net_device *prNetDev, u8 *prInBuf);
s32 MT_ATEStartRX(struct net_device *prNetDev, u8 *prInBuf);
s32 MT_ATEStopRX(struct net_device *prNetDev, u8 *prInBuf);
s32 MT_ATESetChannel(struct net_device *prNetDev, u32 u4SXIdx, u32 u4SetFreq);
s32 MT_ATESetPreamble(struct net_device *prNetDev, u32 u4Mode);
s32 MT_ATESetSystemBW(struct net_device *prNetDev, u32 u4BW);
s32 MT_ATESetTxLength(struct net_device *prNetDev, u32 u4TxLength);
s32 MT_ATESetTxCount(struct net_device *prNetDev, u32 u4TxCount);
s32 MT_ATESetTxIPG(struct net_device *prNetDev, u32 u4TxIPG);
s32 MT_ATESetTxPower0(struct net_device *prNetDev, u32 u4TxPower0);
s32 MT_ATESetPerPacketBW(struct net_device *prNetDev, u32 u4BW);
s32 MT_ATEPrimarySetting(struct net_device *prNetDev, u32 u4PrimaryCh);
s32 MT_ATESetTxGi(struct net_device *prNetDev, u32 u4SetTxGi);
s32 MT_ATESetTxPayLoad(struct net_device *prNetDev,
                       u32 u4Gen_payload_rule,
                       u8 ucPayload);
s32 MT_ATESetTxSTBC(struct net_device *prNetDev, u32 u4Stbc);
s32 MT_ATESetTxPath(struct net_device *prNetDev, u32 u4Tx_path);
s32 MT_ATESetTxVhtNss(struct net_device *prNetDev, u32 u4VhtNss);
s32 MT_ATESetRate(struct net_device *prNetDev, u32 u4Rate);
s32 MT_ATESetEncodeMode(struct net_device *prNetDev, u32 u4Ldpc);
s32 MT_ATESetiBFEnable(struct net_device *prNetDev, u32 u4iBF);
s32 MT_ATESeteBFEnable(struct net_device *prNetDev, u32 u4eBF);
s32 MT_ATESetMACAddress(struct net_device *prNetDev, u32 u4Type, u8 ucAddr[]);
s32 MT_ATELogOnOff(struct net_device *prNetDev,
                   u32 u4Type,
                   u32 u4On_off,
                   u32 u4Size);
s32 MT_ATEResetTXRXCounter(struct net_device *prNetDev);
s32 MT_ATESetDBDCBandIndex(struct net_device *prNetDev, u32 u4BandIdx);
s32 MT_ATESetBand(struct net_device *prNetDev, s32 i4Band);
s32 MT_ATESetTxToneType(struct net_device *prNetDev, s32 i4ToneType);
s32 MT_ATESetTxToneBW(struct net_device *prNetDev, s32 i4ToneFreq);
s32 MT_ATESetTxToneDCOffset(struct net_device *prNetDev,
                            s32 i4DcOffsetI,
                            s32 i4DcOffsetQ);
s32 MT_ATESetDBDCTxTonePower(struct net_device *prNetDev,
                             s32 i4AntIndex,
                             s32 i4RF_Power,
                             s32 i4Digi_Power);
s32 MT_ATEDBDCTxTone(struct net_device *prNetDev, s32 i4Control);
s32 MT_ATESetMacHeader(struct net_device *prNetDev,
                       u32 u2FrameCtrl,
                       u32 u2DurationID,
                       u32 u4SeqCtrl);
s32 MT_ATE_IRRSetADC(struct net_device *prNetDev,
                     u32 u4WFIdx,
                     u32 u4ChFreq,
                     u32 u4BW,
                     u32 u4Sx,
                     u32 u4Band,
                     u32 u4RunType,
                     u32 u4FType);
s32 MT_ATE_IRRSetRxGain(struct net_device *prNetDev,
                        u32 u4PgaLpfg,
                        u32 u4Lna,
                        u32 u4Band,
                        u32 u4WF_inx,
                        u32 u4Rfdgc);
s32 MT_ATE_IRRSetTTG(struct net_device *prNetDev,
                     u32 u4TTGPwrIdx,
                     u32 u4ChFreq,
                     u32 u4FIToneFreq,
                     u32 u4Band);
s32 MT_ATE_IRRSetTrunOnTTG(struct net_device *prNetDev,
                           u32 u4TTGOnOff,
                           u32 u4Band,
                           u32 u4WF_inx);
s32 MT_ATE_TMRSetting(struct net_device *prNetDev,
                      u32 u4Setting,
                      u32 u4Version,
                      u32 u4MPThres,
                      u32 u4MPIter);
s32 MT_ATERDDStart(struct net_device *prNetDev, u8 *prInBuf);
s32 MT_ATERDDStop(struct net_device *prNetDev, u8 *prInBuf);
s32 MT_ATEMPSSetSeqData(struct net_device *prNetDev,
                        u32 u4TestNum,
                        u32 *pu4Phy,
                        u32 u4Band);
s32 MT_ATEMPSSetPayloadLength(struct net_device *prNetDev,
                              u32 u4TestNum,
                              u32 *pu4Length,
                              u32 u4Band);
s32 MT_ATEMPSSetPacketCount(struct net_device *prNetDev,
                            u32 u4TestNum,
                            u32 *pu4PktCnt,
                            u32 u4Band);
s32 MT_ATEMPSSetPowerGain(struct net_device *prNetDev,
                          u32 u4TestNum,
                          u32 *pu4PwrGain,
                          u32 u4Band);
s32 MT_ATEMPSSetNss(struct net_device *prNetDev,
                    u32 u4TestNum,
                    u32 *pu4Nss,
                    u32 u4Band);
s32 MT_ATEMPSSetPerpacketBW(struct net_device *prNetDev,
                            u32 u4TestNum,
                            u32 *pu4PerPktBW,
                            u32 u4Band);

s32 MT_ATEWriteEfuse(struct net_device *prNetDev, u16 u2Offset, u16 u2Content);
s32 MT_ATESetTxTargetPower(struct net_device *prNetDev, u8 ucTxTargetPower);
#if (CFG_SUPPORT_DFS_MASTER == 1)
s32 MT_ATESetRddReport(struct net_device *prNetDev, u8 ucDbdcIdx);
s32 MT_ATESetRadarDetectMode(struct net_device *prNetDev, u8 ucRadarDetectMode);
#endif

#if CFG_SUPPORT_TX_BF
s32 TxBfProfileTag_InValid(struct net_device *prNetDev,
                           P_PFMU_PROFILE_TAG1 prPfmuTag1,
                           u8 ucInValid);
s32 TxBfProfileTag_PfmuIdx(struct net_device *prNetDev,
                           P_PFMU_PROFILE_TAG1 prPfmuTag1,
                           u8 ucProfileIdx);
s32 TxBfProfileTag_TxBfType(struct net_device *prNetDev,
                            P_PFMU_PROFILE_TAG1 prPfmuTag1,
                            u8 ucBFType);
s32 TxBfProfileTag_DBW(struct net_device *prNetDev,
                       P_PFMU_PROFILE_TAG1 prPfmuTag1,
                       u8 ucBW);
s32 TxBfProfileTag_SuMu(struct net_device *prNetDev,
                        P_PFMU_PROFILE_TAG1 prPfmuTag1,
                        u8 ucSuMu);
s32 TxBfProfileTag_Mem(struct net_device *prNetDev,
                       P_PFMU_PROFILE_TAG1 prPfmuTag1,
                       u8 *aucMemAddrColIdx,
                       u8 *aucMemAddrRowIdx);
s32 TxBfProfileTag_Matrix(struct net_device *prNetDev,
                          P_PFMU_PROFILE_TAG1 prPfmuTag1,
                          u8 ucNrow,
                          u8 ucNcol,
                          u8 ucNgroup,
                          u8 ucLM,
                          u8 ucCodeBook,
                          u8 ucHtcExist);
s32 TxBfProfileTag_SNR(struct net_device *prNetDev,
                       P_PFMU_PROFILE_TAG1 prPfmuTag1,
                       u8 ucSNR_STS0,
                       u8 ucSNR_STS1,
                       u8 ucSNR_STS2,
                       u8 ucSNR_STS3);
s32 TxBfProfileTag_SmtAnt(struct net_device *prNetDev,
                          P_PFMU_PROFILE_TAG2 prPfmuTag2,
                          u8 ucSmartAnt);
s32 TxBfProfileTag_SeIdx(struct net_device *prNetDev,
                         P_PFMU_PROFILE_TAG2 prPfmuTag2,
                         u8 ucSeIdx);
s32 TxBfProfileTag_RmsdThd(struct net_device *prNetDev,
                           P_PFMU_PROFILE_TAG2 prPfmuTag2,
                           u8 ucRmsdThrd);
s32 TxBfProfileTag_McsThd(struct net_device *prNetDev,
                          P_PFMU_PROFILE_TAG2 prPfmuTag2,
                          u8 *pMCSThLSS,
                          u8 *pMCSThSSS);
s32 TxBfProfileTag_TimeOut(struct net_device *prNetDev,
                           P_PFMU_PROFILE_TAG2 prPfmuTag2,
                           u8 ucTimeOut);
s32 TxBfProfileTag_DesiredBW(struct net_device *prNetDev,
                             P_PFMU_PROFILE_TAG2 prPfmuTag2,
                             u8 ucDesiredBW);
s32 TxBfProfileTag_DesiredNc(struct net_device *prNetDev,
                             P_PFMU_PROFILE_TAG2 prPfmuTag2,
                             u8 ucDesiredNc);
s32 TxBfProfileTag_DesiredNr(struct net_device *prNetDev,
                             P_PFMU_PROFILE_TAG2 prPfmuTag2,
                             u8 ucDesiredNr);
s32 TxBfProfileTagWrite(struct net_device *prNetDev,
                        P_PFMU_PROFILE_TAG1 prPfmuTag1,
                        P_PFMU_PROFILE_TAG2 prPfmuTag2,
                        u8 profileIdx);
s32 TxBfProfileTagRead(struct net_device *prNetDev, u8 PfmuIdx, u8 fgBFer);
s32 TxBfProfileDataRead(struct net_device *prNetDev,
                        u8 profileIdx,
                        u8 fgBFer,
                        u8 subcarrierIdxMsb,
                        u8 subcarrierIdxLsb);
s32 TxBfProfileDataWrite(struct net_device *prNetDev,
                         u8 profileIdx,
                         u16 subcarrierIdx,
                         u16 au2Phi[6],
                         u8 aucPsi[6],
                         u8 aucDSnr[4]);
s32 TxBfProfilePnRead(struct net_device *prNetDev, u8 profileIdx);
s32 TxBfProfilePnWrite(struct net_device *prNetDev,
                       u8 ucProfileIdx,
                       u16 u2bw,
                       u16 au2XSTS[12]);

s32 TxBfSounding(struct net_device *prNetDev, u8 ucSuMu,  /* 0/1/2/3 */
                 u8 ucNumSta,  /* 00~04 */
                 u8 ucSndInterval,  /* 00~FF */
                 u8 ucWLan0,  /* 00~7F */
                 u8 ucWLan1,  /* 00~7F */
                 u8 ucWLan2,  /* 00~7F */
                 u8 ucWLan3  /* 00~7F */
                 );
s32 TxBfSoundingStop(struct net_device *prNetDev);
s32 TxBfTxApply(struct net_device *prNetDev,
                u8 ucWlanId,
                u8 fgETxBf,
                u8 fgITxBf,
                u8 fgMuTxBf);

s32 TxBfManualAssoc(struct net_device *prNetDev,
                    u8 aucMac[MAC_ADDR_LEN],
                    u8 ucType,
                    u8 ucWtbl,
                    u8 ucOwnmac,
                    u8 ucPhyMode,
                    u8 ucBw,
                    u8 ucNss,
                    u8 ucPfmuId,
                    u8 ucMarate,
                    u8 ucSpeIdx,
                    u8 ucRca2,
                    u8 ucRv);

s32 TxBfPfmuMemAlloc(struct net_device *prNetDev, u8 ucSuMuMode, u8 ucWlanIdx);

s32 TxBfPfmuMemRelease(struct net_device *prNetDev, u8 ucWlanId);

s32 DevInfoUpdate(struct net_device *prNetDev,
                  u8 ucOwnMacIdx,
                  u8 fgBand,
                  u8 aucMacAddr[MAC_ADDR_LEN]);

s32 BssInfoUpdate(struct net_device *prNetDev,
                  u8 u4OwnMacIdx,
                  u8 u4BssIdx,
                  u8 u4BssId[MAC_ADDR_LEN]);

s32 StaRecCmmUpdate(struct net_device *prNetDev,
                    u8 ucWlanId,
                    u8 ucBssId,
                    u8 u4Aid,
                    u8 aucMacAddr[MAC_ADDR_LEN]);

s32 StaRecBfUpdate(struct net_device *prNetDev,
                   STA_REC_BF_UPD_ARGUMENT rStaRecBfUpdArg,
                   u8 aucMemRow[4],
                   u8 aucMemCol[4]);

#endif
#endif
#endif
