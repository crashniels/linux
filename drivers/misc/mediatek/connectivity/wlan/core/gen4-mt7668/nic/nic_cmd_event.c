// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   nic_cmd_event.c
 *    \brief  Callback functions for Command packets.
 *
 * Various Event packet handlers which will be setup in the callback function of
 *    a command packet.
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
#include "gl_ate_agent.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

const NIC_CAPABILITY_V2_REF_TABLE_T gNicCapabilityV2InfoTable[] = {
    { TAG_CAP_TX_RESOURCE, nicCmdEventQueryNicTxResource },
    { TAG_CAP_TX_EFUSEADDRESS, nicCmdEventQueryNicEfuseAddr },
    { TAG_CAP_COEX_FEATURE, nicCmdEventQueryNicCoexFeature },
    { TAG_CAP_SINGLE_SKU, rlmDomainExtractSingleSkuInfoFromFirmware },
#if CFG_TCP_IP_CHKSUM_OFFLOAD
    { TAG_CAP_CSUM_OFFLOAD, nicCmdEventQueryNicCsumOffload },
#endif
    { TAG_CAP_EFUSE_OFFSET, nicCmdEventQueryEfuseOffset },
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                            F U N C T I O N   D A T A
 *******************************************************************************
 */
void nicCmdEventQueryMcrRead(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_PARAM_CUSTOM_MCR_RW_STRUCT_T prMcrRdInfo;
    P_GLUE_INFO_T prGlueInfo;
    P_CMD_ACCESS_REG prCmdAccessReg;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    /* 4 <2> Update information of OID */
    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        if (u4EventBufLen < sizeof(CMD_ACCESS_REG)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(CMD_ACCESS_REG));
            return;
        }
        prCmdAccessReg = (P_CMD_ACCESS_REG)(pucEventBuf);

        u4QueryInfoLen = sizeof(PARAM_CUSTOM_MCR_RW_STRUCT_T);

        prMcrRdInfo =
            (P_PARAM_CUSTOM_MCR_RW_STRUCT_T)prCmdInfo->pvInformationBuffer;
        prMcrRdInfo->u4McrOffset = prCmdAccessReg->u4Address;
        prMcrRdInfo->u4McrData = prCmdAccessReg->u4Data;

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }

    return;
}

void nicCmdEventQueryCoexIso(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_GLUE_INFO_T prGlueInfo;

    struct CMD_COEX_CTRL *prCmdCoexCtrl;
    struct CMD_COEX_ISO_DETECT *prCmdCoexIsoDetect;
    struct PARAM_COEX_CTRL *prCoexCtrl;
    struct PARAM_COEX_ISO_DETECT *prCoexIsoDetect;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    /* 4 <2> Update information of OID */
    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
        if (prAdapter->fgTestMode == false &&
            u4EventBufLen < sizeof(struct CMD_COEX_CTRL)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(struct CMD_COEX_CTRL));
            return;
        }
        prCmdCoexCtrl = (struct CMD_COEX_CTRL *)(pucEventBuf);
        u4QueryInfoLen = sizeof(struct PARAM_COEX_CTRL);
        prCmdCoexIsoDetect =
            (struct CMD_COEX_ISO_DETECT *)&prCmdCoexCtrl->aucBuffer[0];

        prCoexCtrl = (struct PARAM_COEX_CTRL *)prCmdInfo->pvInformationBuffer;
        prCoexIsoDetect =
            (struct PARAM_COEX_ISO_DETECT *)&prCoexCtrl->aucBuffer[0];
        prCoexIsoDetect->u4IsoPath = prCmdCoexIsoDetect->u4IsoPath;
        prCoexIsoDetect->u4Channel = prCmdCoexIsoDetect->u4Channel;
        /*prCoexIsoDetect->u4Band = prCmdCoexIsoDetect->u4Band;*/
        prCoexIsoDetect->u4Isolation = prCmdCoexIsoDetect->u4Isolation;

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

#if CFG_SUPPORT_QA_TOOL
void nicCmdEventQueryRxStatistics(IN P_ADAPTER_T prAdapter,
                                  IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                  IN u32 u4EventBufLen)
{
    P_PARAM_CUSTOM_ACCESS_RX_STAT prRxStatistics;
    P_EVENT_ACCESS_RX_STAT prEventAccessRxStat;
    u32 u4QueryInfoLen, i;
    P_GLUE_INFO_T prGlueInfo;
    u32 *prElement;
    u32 u4Temp;
    /* P_CMD_ACCESS_RX_STAT                  prCmdRxStat, prRxStat; */

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    /* 4 <2> Update information of OID */
    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        if (u4EventBufLen < sizeof(EVENT_ACCESS_RX_STAT)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_ACCESS_RX_STAT));
            return;
        }
        prEventAccessRxStat = (P_EVENT_ACCESS_RX_STAT)(pucEventBuf);

        prRxStatistics =
            (P_PARAM_CUSTOM_ACCESS_RX_STAT)prCmdInfo->pvInformationBuffer;
        prRxStatistics->u4SeqNum = prEventAccessRxStat->u4SeqNum;
        prRxStatistics->u4TotalNum = prEventAccessRxStat->u4TotalNum;

        u4QueryInfoLen = sizeof(CMD_ACCESS_RX_STAT);

        if (prRxStatistics->u4SeqNum == u4RxStatSeqNum) {
            prElement = &g_HqaRxStat.MAC_FCS_Err;
            for (i = 0; i < HQA_RX_STATISTIC_NUM; i++) {
                u4Temp = ntohl(prEventAccessRxStat->au4Buffer[i]);
                kalMemCopy(prElement, &u4Temp, 4);

                if (i < (HQA_RX_STATISTIC_NUM - 1)) {
                    prElement++;
                }
            }

            g_HqaRxStat.AllMacMdrdy0 = ntohl(prEventAccessRxStat->au4Buffer[i]);
            i++;
            g_HqaRxStat.AllMacMdrdy1 = ntohl(prEventAccessRxStat->au4Buffer[i]);
            /* i++; */
            /* g_HqaRxStat.AllFCSErr0 =
             * ntohl(prEventAccessRxStat->au4Buffer[i]); */
            /* i++; */
            /* g_HqaRxStat.AllFCSErr1 =
             * ntohl(prEventAccessRxStat->au4Buffer[i]); */
        }

        DBGLOG(INIT, ERROR,
               "MT6632 : RX Statistics Test SeqNum = %d, TotalNum = %d\n",
               (unsigned int)prEventAccessRxStat->u4SeqNum,
               (unsigned int)prEventAccessRxStat->u4TotalNum);

        DBGLOG(
            INIT, ERROR,
            "MAC_FCS_ERR = %d, MAC_MDRDY = %d, MU_RX_CNT = %d, RX_FIFO_FULL = %d\n",
            (unsigned int)prEventAccessRxStat->au4Buffer[0],
            (unsigned int)prEventAccessRxStat->au4Buffer[1],
            (unsigned int)prEventAccessRxStat->au4Buffer[65],
            (unsigned int)prEventAccessRxStat->au4Buffer[22]);

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

#if CFG_SUPPORT_TX_BF
void nicCmdEventPfmuDataRead(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_GLUE_INFO_T prGlueInfo;
    P_PFMU_DATA prEventPfmuDataRead = NULL;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    /* 4 <2> Update information of OID */
    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        if (u4EventBufLen < sizeof(PFMU_DATA)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d", __func__,
                   u4EventBufLen, sizeof(PFMU_DATA));
            return;
        }
        prEventPfmuDataRead = (P_PFMU_DATA)(pucEventBuf);

        u4QueryInfoLen = sizeof(PFMU_DATA);

        g_rPfmuData = *prEventPfmuDataRead;

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }

    DBGLOG(INIT, INFO, "=========== Before ===========\n");
    if (prEventPfmuDataRead != NULL) {
        DBGLOG(INIT, INFO, "u2Phi11 = 0x%x\n",
               prEventPfmuDataRead->rField.u2Phi11);
        DBGLOG(INIT, INFO, "ucPsi21 = 0x%x\n",
               prEventPfmuDataRead->rField.ucPsi21);
        DBGLOG(INIT, INFO, "u2Phi21 = 0x%x\n",
               prEventPfmuDataRead->rField.u2Phi21);
        DBGLOG(INIT, INFO, "ucPsi31 = 0x%x\n",
               prEventPfmuDataRead->rField.ucPsi31);
        DBGLOG(INIT, INFO, "u2Phi31 = 0x%x\n",
               prEventPfmuDataRead->rField.u2Phi31);
        DBGLOG(INIT, INFO, "ucPsi41 = 0x%x\n",
               prEventPfmuDataRead->rField.ucPsi41);
        DBGLOG(INIT, INFO, "u2Phi22 = 0x%x\n",
               prEventPfmuDataRead->rField.u2Phi22);
        DBGLOG(INIT, INFO, "ucPsi32 = 0x%x\n",
               prEventPfmuDataRead->rField.ucPsi32);
        DBGLOG(INIT, INFO, "u2Phi32 = 0x%x\n",
               prEventPfmuDataRead->rField.u2Phi32);
        DBGLOG(INIT, INFO, "ucPsi42 = 0x%x\n",
               prEventPfmuDataRead->rField.ucPsi42);
        DBGLOG(INIT, INFO, "u2Phi33 = 0x%x\n",
               prEventPfmuDataRead->rField.u2Phi33);
        DBGLOG(INIT, INFO, "ucPsi43 = 0x%x\n",
               prEventPfmuDataRead->rField.ucPsi43);
        DBGLOG(INIT, INFO, "u2dSNR00 = 0x%x\n",
               prEventPfmuDataRead->rField.u2dSNR00);
        DBGLOG(INIT, INFO, "u2dSNR01 = 0x%x\n",
               prEventPfmuDataRead->rField.u2dSNR01);
        DBGLOG(INIT, INFO, "u2dSNR02 = 0x%x\n",
               prEventPfmuDataRead->rField.u2dSNR02);
        DBGLOG(INIT, INFO, "u2dSNR03 = 0x%x\n",
               prEventPfmuDataRead->rField.u2dSNR03);
    }
}

void nicCmdEventPfmuTagRead(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_GLUE_INFO_T prGlueInfo;
    P_EVENT_PFMU_TAG_READ_T prEventPfmuTagRead = NULL;
    P_PARAM_CUSTOM_PFMU_TAG_READ_STRUCT_T prPfumTagRead = NULL;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    if (!pucEventBuf) {
        DBGLOG(INIT, ERROR, "pucEventBuf is NULL.\n");
        return;
    }
    if (u4EventBufLen < sizeof(EVENT_PFMU_TAG_READ_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d", __func__,
               u4EventBufLen, sizeof(EVENT_PFMU_TAG_READ_T));
        return;
    }
    if (!prCmdInfo->pvInformationBuffer) {
        DBGLOG(INIT, ERROR, "prCmdInfo->pvInformationBuffer is NULL.\n");
        return;
    }
    /* 4 <2> Update information of OID */
    if (!prCmdInfo->fgIsOid) {
        DBGLOG(INIT, ERROR, "cmd %u seq #%u not oid!", prCmdInfo->ucCID,
               prCmdInfo->ucCmdSeqNum);
        return;
    }
    prGlueInfo = prAdapter->prGlueInfo;
    prEventPfmuTagRead = (P_EVENT_PFMU_TAG_READ_T)(pucEventBuf);

    prPfumTagRead =
        (P_PARAM_CUSTOM_PFMU_TAG_READ_STRUCT_T)prCmdInfo->pvInformationBuffer;

    kalMemCopy(prPfumTagRead, prEventPfmuTagRead,
               sizeof(EVENT_PFMU_TAG_READ_T));

    u4QueryInfoLen = sizeof(CMD_TXBF_ACTION_T);

    g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1;
    g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2;

    if (prCmdInfo->fgIsOid) {
        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
        prCmdInfo->fgIsOid = false;
    }

    DBGLOG(
        INIT, INFO,
        "========================== (R)Tag1 info ==========================\n");

    DBGLOG(INIT, INFO,
           " Row data0 : %x, Row data1 : %x, Row data2 : %x, Row data3 : %x\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[0],
           prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[1],
           prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[2],
           prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[3]);
    DBGLOG(INIT, INFO, "ProfileID = %d Invalid status = %d\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucProfileID,
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucInvalidProf);
    DBGLOG(INIT, INFO, "0:iBF / 1:eBF = %d\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucTxBf);
    DBGLOG(INIT, INFO, "0:SU / 1:MU = %d\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSU_MU);
    DBGLOG(INIT, INFO, "DBW(0/1/2/3 BW20/40/80/160NC) = %d\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucDBW);
    DBGLOG(INIT, INFO, "RMSD = %d\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucRMSD);
    DBGLOG(INIT, INFO, "Nrow = %d, Ncol = %d, Ng = %d, LM = %d\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucNrow,
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucNcol,
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucNgroup,
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucLM);
    DBGLOG(
        INIT, INFO, "Mem1(%d, %d), Mem2(%d, %d), Mem3(%d, %d), Mem4(%d, %d)\n",
        prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr1ColIdx,
        prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr1RowIdx,
        prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr2ColIdx,
        (prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr2RowIdx |
         (prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr2RowIdxMsb << 5)),
        prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr3ColIdx,
        prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr3RowIdx,
        prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr4ColIdx,
        prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr4RowIdx);
    DBGLOG(INIT, INFO,
           "SNR STS0=0x%x, SNR STS1=0x%x, SNR STS2=0x%x, SNR STS3=0x%x\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS0,
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS1,
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS2,
           prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS3);
    DBGLOG(INIT, INFO,
           "===============================================================\n");

    DBGLOG(
        INIT, INFO,
        "========================== (R)Tag2 info ==========================\n");
    DBGLOG(INIT, INFO, " Row data0 : %x, Row data1 : %x, Row data2 : %x\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[0],
           prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[1],
           prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[2]);
    DBGLOG(INIT, INFO, "Smart Ant Cfg = %d\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.u2SmartAnt);
    DBGLOG(INIT, INFO, "SE index = %d\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucSEIdx);
    DBGLOG(INIT, INFO, "RMSD Threshold = %d\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucRMSDThd);
    DBGLOG(INIT, INFO,
           "MCS TH L1SS = %d, S1SS = %d, L2SS = %d, S2SS = %d\n"
           "L3SS = %d, S3SS = %d\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThL1SS,
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThS1SS,
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThL2SS,
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThS2SS,
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThL3SS,
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThS3SS);
    DBGLOG(INIT, INFO, "iBF lifetime limit(unit:4ms) = 0x%x\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfTimeOut);
    DBGLOG(INIT, INFO, "iBF desired DBW = %d\n  0/1/2/3 : BW20/40/80/160NC\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfDBW);
    DBGLOG(INIT, INFO, "iBF desired Ncol = %d\n  0/1/2 : Ncol = 1 ~ 3\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfNcol);
    DBGLOG(INIT, INFO, "iBF desired Nrow = %d\n  0/1/2/3 : Nrow = 1 ~ 4\n",
           prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfNrow);
    DBGLOG(INIT, INFO,
           "===============================================================\n");
}

#endif
#if CFG_SUPPORT_MU_MIMO
void nicCmdEventGetQd(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                      IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_GLUE_INFO_T prGlueInfo;
    P_EVENT_HQA_GET_QD prEventHqaGetQd;
    u32 i;

    P_PARAM_CUSTOM_GET_QD_STRUCT_T prGetQd = NULL;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);
    if (!pucEventBuf) {
        DBGLOG(INIT, ERROR, "pucEventBuf is NULL.\n");
        return;
    }
    if (u4EventBufLen < sizeof(EVENT_HQA_GET_QD)) {
        DBGLOG(NIC, ERROR, "%s:Invalid event length: %d < %d", __func__,
               u4EventBufLen, sizeof(EVENT_HQA_GET_QD));
        return;
    }
    if (!prCmdInfo->pvInformationBuffer) {
        DBGLOG(INIT, ERROR, "prCmdInfo->pvInformationBuffer is NULL.\n");
        return;
    }
    /* 4 <2> Update information of OID */
    if (!prCmdInfo->fgIsOid) {
        DBGLOG(INIT, ERROR, "cmd %u seq #%u not oid!\n", prCmdInfo->ucCID,
               prCmdInfo->ucCmdSeqNum);
        return;
    }
    prGlueInfo = prAdapter->prGlueInfo;
    prEventHqaGetQd = (P_EVENT_HQA_GET_QD)(pucEventBuf);

    prGetQd = (P_PARAM_CUSTOM_GET_QD_STRUCT_T)prCmdInfo->pvInformationBuffer;

    kalMemCopy(prGetQd, prEventHqaGetQd, sizeof(EVENT_HQA_GET_QD));

    u4QueryInfoLen = sizeof(CMD_MUMIMO_ACTION_T);

    /* g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1; */
    /* g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2; */

    kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                   WLAN_STATUS_SUCCESS);

    DBGLOG(INIT, INFO, " event id : %x\n", prGetQd->u4EventId);
    for (i = 0; i < 14; i++)
        DBGLOG(INIT, INFO, "au4RawData[%d]: %x\n", i, prGetQd->au4RawData[i]);
}

void nicCmdEventGetCalcLq(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                          IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_GLUE_INFO_T prGlueInfo;
    P_EVENT_HQA_GET_MU_CALC_LQ prEventHqaGetMuCalcLq;
    u32 i, j;

    P_PARAM_CUSTOM_GET_MU_CALC_LQ_STRUCT_T prGetMuCalcLq = NULL;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);
    if (!pucEventBuf) {
        DBGLOG(INIT, ERROR, "pucEventBuf is NULL.\n");
        return;
    }
    if (u4EventBufLen < sizeof(EVENT_HQA_GET_MU_CALC_LQ)) {
        DBGLOG(NIC, ERROR, "%s:Invalid event length: %d < %d", __func__,
               u4EventBufLen, sizeof(EVENT_HQA_GET_MU_CALC_LQ));
        return;
    }
    if (!prCmdInfo->pvInformationBuffer) {
        DBGLOG(INIT, ERROR, "prCmdInfo->pvInformationBuffer is NULL.\n");
        return;
    }
    /* 4 <2> Update information of OID */
    if (!prCmdInfo->fgIsOid) {
        DBGLOG(INIT, ERROR, "cmd %u seq #%u not oid!\n", prCmdInfo->ucCID,
               prCmdInfo->ucCmdSeqNum);
        return;
    }
    prGlueInfo = prAdapter->prGlueInfo;
    prEventHqaGetMuCalcLq = (P_EVENT_HQA_GET_MU_CALC_LQ)(pucEventBuf);

    prGetMuCalcLq =
        (P_PARAM_CUSTOM_GET_MU_CALC_LQ_STRUCT_T)prCmdInfo->pvInformationBuffer;

    kalMemCopy(prGetMuCalcLq, prEventHqaGetMuCalcLq,
               sizeof(EVENT_HQA_GET_MU_CALC_LQ));

    u4QueryInfoLen = sizeof(CMD_MUMIMO_ACTION_T);

    /* g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1; */
    /* g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2; */

    kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                   WLAN_STATUS_SUCCESS);

    DBGLOG(INIT, INFO, " event id : %x\n", prGetMuCalcLq->u4EventId);
    for (i = 0; i < NUM_OF_USER; i++)
        for (j = 0; j < NUM_OF_MODUL; j++)
            DBGLOG(INIT, INFO, " lq_report[%d][%d]: %x\n", i, j,
                   prGetMuCalcLq->rEntry.lq_report[i][j]);
}

void nicCmdEventGetCalcInitMcs(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                               IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_GLUE_INFO_T prGlueInfo;
    P_EVENT_SHOW_GROUP_TBL_ENTRY prEventShowGroupTblEntry = NULL;

    P_PARAM_CUSTOM_SHOW_GROUP_TBL_ENTRY_STRUCT_T prShowGroupTbl;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);
    if (!pucEventBuf) {
        DBGLOG(INIT, ERROR, "pucEventBuf is NULL.\n");
        return;
    }
    if (u4EventBufLen < sizeof(EVENT_SHOW_GROUP_TBL_ENTRY)) {
        DBGLOG(NIC, ERROR, "%s:Invalid event length: %d < %d", __func__,
               u4EventBufLen, sizeof(EVENT_SHOW_GROUP_TBL_ENTRY));
        return;
    }
    if (!prCmdInfo->pvInformationBuffer) {
        DBGLOG(INIT, ERROR, "prCmdInfo->pvInformationBuffer is NULL.\n");
        return;
    }
    /* 4 <2> Update information of OID */
    if (!prCmdInfo->fgIsOid) {
        DBGLOG(INIT, ERROR, "cmd %u seq #%u not oid!\n", prCmdInfo->ucCID,
               prCmdInfo->ucCmdSeqNum);
        return;
    }
    prGlueInfo = prAdapter->prGlueInfo;
    prEventShowGroupTblEntry = (P_EVENT_SHOW_GROUP_TBL_ENTRY)(pucEventBuf);

    prShowGroupTbl = (P_PARAM_CUSTOM_SHOW_GROUP_TBL_ENTRY_STRUCT_T)
                     prCmdInfo->pvInformationBuffer;

    kalMemCopy(prShowGroupTbl, prEventShowGroupTblEntry,
               sizeof(EVENT_SHOW_GROUP_TBL_ENTRY));

    u4QueryInfoLen = sizeof(CMD_MUMIMO_ACTION_T);

    /* g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1; */
    /* g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2; */

    kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                   WLAN_STATUS_SUCCESS);

    DBGLOG(INIT, INFO,
           "========================== (R)Group table info "
           "==========================\n");
    DBGLOG(INIT, INFO, " event id : %x\n", prEventShowGroupTblEntry->u4EventId);
    DBGLOG(INIT, INFO, "index = %x numUser = %x\n",
           prEventShowGroupTblEntry->index, prEventShowGroupTblEntry->numUser);
    DBGLOG(INIT, INFO, "BW = %x NS0/1/ = %x/%x\n", prEventShowGroupTblEntry->BW,
           prEventShowGroupTblEntry->NS0, prEventShowGroupTblEntry->NS1);
    DBGLOG(INIT, INFO, "PFIDUser0/1 = %x/%x\n",
           prEventShowGroupTblEntry->PFIDUser0,
           prEventShowGroupTblEntry->PFIDUser1);
    DBGLOG(INIT, INFO, "fgIsShortGI = %x, fgIsUsed = %x, fgIsDisable = %x\n",
           prEventShowGroupTblEntry->fgIsShortGI,
           prEventShowGroupTblEntry->fgIsUsed,
           prEventShowGroupTblEntry->fgIsDisable);
    DBGLOG(INIT, INFO, "initMcsUser0/1 = %x/%x\n",
           prEventShowGroupTblEntry->initMcsUser0,
           prEventShowGroupTblEntry->initMcsUser1);
    DBGLOG(INIT, INFO, "dMcsUser0: 0/1/ = %x/%x\n",
           prEventShowGroupTblEntry->dMcsUser0,
           prEventShowGroupTblEntry->dMcsUser1);
}
#endif
#endif

void nicCmdEventQuerySwCtrlRead(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_PARAM_CUSTOM_SW_CTRL_STRUCT_T prSwCtrlInfo;
    P_GLUE_INFO_T prGlueInfo;
    P_CMD_SW_DBG_CTRL_T prCmdSwCtrl;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    /* 4 <2> Update information of OID */
    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
        if (prAdapter->fgTestMode == false &&
            u4EventBufLen < sizeof(CMD_SW_DBG_CTRL_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(CMD_SW_DBG_CTRL_T));
            return;
        }
        prCmdSwCtrl = (P_CMD_SW_DBG_CTRL_T)(pucEventBuf);

        u4QueryInfoLen = sizeof(PARAM_CUSTOM_SW_CTRL_STRUCT_T);

        prSwCtrlInfo =
            (P_PARAM_CUSTOM_SW_CTRL_STRUCT_T)prCmdInfo->pvInformationBuffer;
        prSwCtrlInfo->u4Id = prCmdSwCtrl->u4Id;
        prSwCtrlInfo->u4Data = prCmdSwCtrl->u4Data;

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryChipConfig(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T prChipConfigInfo;
    P_GLUE_INFO_T prGlueInfo;
    P_CMD_CHIP_CONFIG_T prCmdChipConfig;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    /* 4 <2> Update information of OID */
    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        if (u4EventBufLen < sizeof(CMD_CHIP_CONFIG_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(CMD_CHIP_CONFIG_T));
            return;
        }
        prCmdChipConfig = (P_CMD_CHIP_CONFIG_T)(pucEventBuf);

        u4QueryInfoLen = sizeof(PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T);

        if (prCmdInfo->u4InformationBufferLength <
            sizeof(PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T)) {
            DBGLOG(
                REQ, INFO,
                "Chip config u4InformationBufferLength %u is not valid (event)\n",
                prCmdInfo->u4InformationBufferLength);
        }
        prChipConfigInfo =
            (P_PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T)prCmdInfo->pvInformationBuffer;
        prChipConfigInfo->ucRespType = prCmdChipConfig->ucRespType;
        prChipConfigInfo->u2MsgSize = prCmdChipConfig->u2MsgSize;
        DBGLOG(REQ, INFO, "%s: RespTyep  %u\n", __func__,
               prChipConfigInfo->ucRespType);
        DBGLOG(REQ, INFO, "%s: u2MsgSize %u\n", __func__,
               prChipConfigInfo->u2MsgSize);

        if (prChipConfigInfo->u2MsgSize > CHIP_CONFIG_RESP_SIZE) {
            DBGLOG(REQ, WARN, "Chip config Msg Size %u is not valid (event)\n",
                   prChipConfigInfo->u2MsgSize);
            prChipConfigInfo->u2MsgSize = CHIP_CONFIG_RESP_SIZE;
        }

        kalMemCopy(prChipConfigInfo->aucCmd, prCmdChipConfig->aucCmd,
                   prChipConfigInfo->u2MsgSize);
        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventSetCommon(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                          IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    ASSERT(prAdapter);
    ASSERT(prCmdInfo);

    if (prCmdInfo->fgIsOid) {
        /* Update Set Information Length */
        kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
                       prCmdInfo->u4InformationBufferLength,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventSetDisassociate(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen)
{
    ASSERT(prAdapter);
    ASSERT(prCmdInfo);

    if (prCmdInfo->fgIsOid) {
        /* Update Set Information Length */
        kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0,
                       WLAN_STATUS_SUCCESS);
    }

    kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
                                 WLAN_STATUS_MEDIA_DISCONNECT, NULL, 0);
}

void nicCmdEventSetIpAddress(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen)
{
    u32 u4Count;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);

    u4Count = (prCmdInfo->u4SetInfoLen -
               OFFSET_OF(CMD_SET_NETWORK_ADDRESS_LIST, arNetAddress)) /
              sizeof(IPV4_NETWORK_ADDRESS);

    if (prCmdInfo->fgIsOid) {
        /* Update Set Information Length */
        kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
                       OFFSET_OF(PARAM_NETWORK_ADDRESS_LIST, arAddress) +
                       u4Count *
                       (OFFSET_OF(PARAM_NETWORK_ADDRESS, aucAddress) +
                        sizeof(PARAM_NETWORK_ADDRESS_IP)),
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryRfTestATInfo(IN P_ADAPTER_T prAdapter,
                                  IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                  IN u32 u4EventBufLen)
{
    P_EVENT_TEST_STATUS prTestStatus, prQueryBuffer;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    if (u4EventBufLen < sizeof(EVENT_TEST_STATUS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_TEST_STATUS));
        return;
    }
    prTestStatus = (P_EVENT_TEST_STATUS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        prQueryBuffer = (P_EVENT_TEST_STATUS)prCmdInfo->pvInformationBuffer;

        kalMemCopy(prQueryBuffer, prTestStatus, sizeof(EVENT_TEST_STATUS));

        u4QueryInfoLen = sizeof(EVENT_TEST_STATUS);

        /* Update Query Information Length */
        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryLinkQuality(IN P_ADAPTER_T prAdapter,
                                 IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                 IN u32 u4EventBufLen)
{
    PARAM_RSSI rRssi, *prRssi;
    P_EVENT_LINK_QUALITY prLinkQuality;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_LINK_QUALITY)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_LINK_QUALITY));
        return;
    }
    prLinkQuality = (P_EVENT_LINK_QUALITY)pucEventBuf;

    rRssi = (PARAM_RSSI)prLinkQuality->cRssi;  /* ranged from (-128 ~ 30) in
                                                * unit of dBm */

    if (prAdapter->prAisBssInfo->eConnectionState ==
        PARAM_MEDIA_STATE_CONNECTED) {
        if (rRssi > PARAM_WHQL_RSSI_MAX_DBM) {
            rRssi = PARAM_WHQL_RSSI_MAX_DBM;
        } else if (rRssi < PARAM_WHQL_RSSI_MIN_DBM) {
            rRssi = PARAM_WHQL_RSSI_MIN_DBM;
        }
    } else {
        rRssi = PARAM_WHQL_RSSI_MIN_DBM;
    }

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        prRssi = (PARAM_RSSI *)prCmdInfo->pvInformationBuffer;

        kalMemCopy(prRssi, &rRssi, sizeof(PARAM_RSSI));
        u4QueryInfoLen = sizeof(PARAM_RSSI);

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is in response of OID_GEN_LINK_SPEED query request
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param prCmdInfo      Pointer to the pending command info
 * @param pucEventBuf
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventQueryLinkSpeed(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                               IN u32 u4EventBufLen)
{
    P_EVENT_LINK_QUALITY prLinkQuality;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;
    u32 *pu4LinkSpeed;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_LINK_QUALITY)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_LINK_QUALITY));
        return;
    }
    prLinkQuality = (P_EVENT_LINK_QUALITY)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        pu4LinkSpeed = (u32 *)(prCmdInfo->pvInformationBuffer);

        *pu4LinkSpeed = prLinkQuality->u2LinkSpeed * 5000;

        u4QueryInfoLen = sizeof(u32);

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryStatistics(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen)
{
    P_PARAM_802_11_STATISTICS_STRUCT_T prStatistics;
    P_EVENT_STATISTICS prEventStatistics;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    prEventStatistics = (P_EVENT_STATISTICS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        u4QueryInfoLen = sizeof(PARAM_802_11_STATISTICS_STRUCT_T);
        prStatistics =
            (P_PARAM_802_11_STATISTICS_STRUCT_T)prCmdInfo->pvInformationBuffer;

        prStatistics->u4Length = sizeof(PARAM_802_11_STATISTICS_STRUCT_T);
        prStatistics->rTransmittedFragmentCount =
            prEventStatistics->rTransmittedFragmentCount;
        prStatistics->rMulticastTransmittedFrameCount =
            prEventStatistics->rMulticastTransmittedFrameCount;
        prStatistics->rFailedCount = prEventStatistics->rFailedCount;
        prStatistics->rRetryCount = prEventStatistics->rRetryCount;
        prStatistics->rMultipleRetryCount =
            prEventStatistics->rMultipleRetryCount;
        prStatistics->rRTSSuccessCount = prEventStatistics->rRTSSuccessCount;
        prStatistics->rRTSFailureCount = prEventStatistics->rRTSFailureCount;
        prStatistics->rACKFailureCount = prEventStatistics->rACKFailureCount;
        prStatistics->rFrameDuplicateCount =
            prEventStatistics->rFrameDuplicateCount;
        prStatistics->rReceivedFragmentCount =
            prEventStatistics->rReceivedFragmentCount;
        prStatistics->rMulticastReceivedFrameCount =
            prEventStatistics->rMulticastReceivedFrameCount;
        prStatistics->rFCSErrorCount = prEventStatistics->rFCSErrorCount;
        prStatistics->rTKIPLocalMICFailures.QuadPart = 0;
        prStatistics->rTKIPICVErrors.QuadPart = 0;
        prStatistics->rTKIPCounterMeasuresInvoked.QuadPart = 0;
        prStatistics->rTKIPReplays.QuadPart = 0;
        prStatistics->rCCMPFormatErrors.QuadPart = 0;
        prStatistics->rCCMPReplays.QuadPart = 0;
        prStatistics->rCCMPDecryptErrors.QuadPart = 0;
        prStatistics->rFourWayHandshakeFailures.QuadPart = 0;
        prStatistics->rWEPUndecryptableCount.QuadPart = 0;
        prStatistics->rWEPICVErrorCount.QuadPart = 0;
        prStatistics->rDecryptSuccessCount.QuadPart = 0;
        prStatistics->rDecryptFailureCount.QuadPart = 0;

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventEnterRfTest(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    ASSERT(prAdapter);
    ASSERT(prCmdInfo);

    /* [driver-land] */
    /* prAdapter->fgTestMode = true; */
    if (prAdapter->fgTestMode) {
        prAdapter->fgTestMode = false;
    } else {
        prAdapter->fgTestMode = true;
    }

    /* 0. always indicate disconnection */
    if (kalGetMediaStateIndicated(prAdapter->prGlueInfo) !=
        PARAM_MEDIA_STATE_DISCONNECTED) {
        kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
                                     WLAN_STATUS_MEDIA_DISCONNECT, NULL, 0);
    }
    /* 1. Remove pending TX */
    nicTxRelease(prAdapter, true);

    /* 1.1 clear pending Security / Management Frames */
    kalClearSecurityFrames(prAdapter->prGlueInfo);
    kalClearMgmtFrames(prAdapter->prGlueInfo);

    /* 1.2 clear pending TX packet queued in glue layer */
    kalFlushPendingTxPackets(prAdapter->prGlueInfo);

    /* 2. Reset driver-domain FSMs */
    nicUninitMGMT(prAdapter);

    nicResetSystemService(prAdapter);
    nicInitMGMT(prAdapter, NULL);

    /* 8. completion indication */
    if (prCmdInfo->fgIsOid) {
        /* Update Set Information Length */
        kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
                       prCmdInfo->u4SetInfoLen, WLAN_STATUS_SUCCESS);
    }
#if CFG_SUPPORT_NVRAM
    /* 9. load manufacture data */
    if (kalIsConfigurationExist(prAdapter->prGlueInfo) == true) {
        wlanLoadManufactureData(prAdapter,
                                kalGetConfiguration(prAdapter->prGlueInfo));
    } else {
        DBGLOG(REQ, INFO, "%s: load manufacture data fail\n", __func__);
    }
#endif
}

void nicCmdEventLeaveRfTest(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    /* 6. set driver-land variable */
    prAdapter->fgTestMode = false;
    prAdapter->fgIcapMode = false;

    /* 7. completion indication */
    if (prCmdInfo->fgIsOid) {
        /* Update Set Information Length */
        kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
                       prCmdInfo->u4SetInfoLen, WLAN_STATUS_SUCCESS);
    }

    /* 8. Indicate as disconnected */
    if (kalGetMediaStateIndicated(prAdapter->prGlueInfo) !=
        PARAM_MEDIA_STATE_DISCONNECTED) {
        kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
                                     WLAN_STATUS_MEDIA_DISCONNECT, NULL, 0);

        prAdapter->rWlanInfo.u4SysTime = kalGetTimeTick();
    }
#if CFG_SUPPORT_NVRAM
    /* 9. load manufacture data */
    if (kalIsConfigurationExist(prAdapter->prGlueInfo) == true) {
        wlanLoadManufactureData(prAdapter,
                                kalGetConfiguration(prAdapter->prGlueInfo));
    } else {
        DBGLOG(REQ, INFO, "%s: load manufacture data fail\n", __func__);
    }
#endif

    /* 10. Override network address */
    wlanUpdateNetworkAddress(prAdapter);
}

void nicCmdEventQueryMcastAddr(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                               IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_GLUE_INFO_T prGlueInfo;
    P_EVENT_MAC_MCAST_ADDR prEventMacMcastAddr;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    /* 4 <2> Update information of OID */
    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
        if (prAdapter->fgTestMode == false &&
            u4EventBufLen < sizeof(EVENT_MAC_MCAST_ADDR)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_MAC_MCAST_ADDR));
            return;
        }
        prEventMacMcastAddr = (P_EVENT_MAC_MCAST_ADDR)(pucEventBuf);

        u4QueryInfoLen = prEventMacMcastAddr->u4NumOfGroupAddr * MAC_ADDR_LEN;

        /* buffer length check */
        if (prCmdInfo->u4InformationBufferLength < u4QueryInfoLen) {
            kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                           WLAN_STATUS_BUFFER_TOO_SHORT);
        } else {
            kalMemCopy(prCmdInfo->pvInformationBuffer,
                       prEventMacMcastAddr->arAddress,
                       prEventMacMcastAddr->u4NumOfGroupAddr * MAC_ADDR_LEN);

            kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                           WLAN_STATUS_SUCCESS);
        }
    }
}

void nicCmdEventQueryEepromRead(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_PARAM_CUSTOM_EEPROM_RW_STRUCT_T prEepromRdInfo;
    P_GLUE_INFO_T prGlueInfo;
    P_EVENT_ACCESS_EEPROM prEventAccessEeprom;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    /* 4 <2> Update information of OID */
    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        if (u4EventBufLen < sizeof(EVENT_ACCESS_EEPROM)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_ACCESS_EEPROM));
            return;
        }
        prEventAccessEeprom = (P_EVENT_ACCESS_EEPROM)(pucEventBuf);

        u4QueryInfoLen = sizeof(PARAM_CUSTOM_EEPROM_RW_STRUCT_T);

        prEepromRdInfo =
            (P_PARAM_CUSTOM_EEPROM_RW_STRUCT_T)prCmdInfo->pvInformationBuffer;
        prEepromRdInfo->ucEepromIndex = (u8)(prEventAccessEeprom->u2Offset);
        prEepromRdInfo->u2EepromData = prEventAccessEeprom->u2Data;

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventSetMediaStreamMode(IN P_ADAPTER_T prAdapter,
                                   IN P_CMD_INFO_T prCmdInfo,
                                   IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    PARAM_MEDIA_STREAMING_INDICATION rParamMediaStreamIndication;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);

    if (prCmdInfo->fgIsOid) {
        /* Update Set Information Length */
        kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
                       prCmdInfo->u4SetInfoLen, WLAN_STATUS_SUCCESS);
    }

    rParamMediaStreamIndication.rStatus.eStatusType =
        ENUM_STATUS_TYPE_MEDIA_STREAM_MODE;
    rParamMediaStreamIndication.eMediaStreamMode =
        prAdapter->rWlanInfo.eLinkAttr.ucMediaStreamMode == 0 ?
        ENUM_MEDIA_STREAM_OFF :
        ENUM_MEDIA_STREAM_ON;

    kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
                                 WLAN_STATUS_MEDIA_SPECIFIC_INDICATION,
                                 (void *)&rParamMediaStreamIndication,
                                 sizeof(PARAM_MEDIA_STREAMING_INDICATION));
}

void nicCmdEventSetStopSchedScan(IN P_ADAPTER_T prAdapter,
                                 IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                 IN u32 u4EventBufLen)
{
    /*
     *  DBGLOG(SCN, INFO, "--->nicCmdEventSetStopSchedScan\n" ));
     */
    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    /*
     *  DBGLOG(SCN, INFO, "<--kalSchedScanStopped\n" );
     */
    if (prCmdInfo->fgIsOid) {
        /* Update Set Information Length */
        kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
                       prCmdInfo->u4InformationBufferLength,
                       WLAN_STATUS_SUCCESS);
    }

    DBGLOG(
        SCN, INFO,
        "nicCmdEventSetStopSchedScan OID done, release lock and send event to "
        "uplayer\n");
    /*Due to dead lock issue, need to release the IO control before calling
     * kernel APIs */
    kalSchedScanStopped(prAdapter->prGlueInfo);
}

/* Statistics responder */
void nicCmdEventQueryXmitOk(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    P_EVENT_STATISTICS prEventStatistics;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;
    u32 *pu4Data;
    u64 *pu8Data;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    prEventStatistics = (P_EVENT_STATISTICS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        if (prCmdInfo->u4InformationBufferLength == sizeof(u32)) {
            u4QueryInfoLen = sizeof(u32);

            pu4Data = (u32 *)prCmdInfo->pvInformationBuffer;
            *pu4Data =
                (u32)prEventStatistics->rTransmittedFragmentCount.QuadPart;
        } else {
            u4QueryInfoLen = sizeof(u64);

            pu8Data = (u64 *)prCmdInfo->pvInformationBuffer;
            *pu8Data = prEventStatistics->rTransmittedFragmentCount.QuadPart;
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryRecvOk(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    P_EVENT_STATISTICS prEventStatistics;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;
    u32 *pu4Data;
    u64 *pu8Data;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    prEventStatistics = (P_EVENT_STATISTICS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        if (prCmdInfo->u4InformationBufferLength == sizeof(u32)) {
            u4QueryInfoLen = sizeof(u32);

            pu4Data = (u32 *)prCmdInfo->pvInformationBuffer;
            *pu4Data = (u32)prEventStatistics->rReceivedFragmentCount.QuadPart;
        } else {
            u4QueryInfoLen = sizeof(u64);

            pu8Data = (u64 *)prCmdInfo->pvInformationBuffer;
            *pu8Data = prEventStatistics->rReceivedFragmentCount.QuadPart;
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryXmitError(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                               IN u32 u4EventBufLen)
{
    P_EVENT_STATISTICS prEventStatistics;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;
    u32 *pu4Data;
    u64 *pu8Data;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    prEventStatistics = (P_EVENT_STATISTICS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        if (prCmdInfo->u4InformationBufferLength == sizeof(u32)) {
            u4QueryInfoLen = sizeof(u32);

            pu4Data = (u32 *)prCmdInfo->pvInformationBuffer;
            *pu4Data = (u32)prEventStatistics->rFailedCount.QuadPart;
        } else {
            u4QueryInfoLen = sizeof(u64);

            pu8Data = (u64 *)prCmdInfo->pvInformationBuffer;
            *pu8Data = (u64)prEventStatistics->rFailedCount.QuadPart;
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryRecvError(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                               IN u32 u4EventBufLen)
{
    P_EVENT_STATISTICS prEventStatistics;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;
    u32 *pu4Data;
    u64 *pu8Data;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    prEventStatistics = (P_EVENT_STATISTICS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        if (prCmdInfo->u4InformationBufferLength == sizeof(u32)) {
            u4QueryInfoLen = sizeof(u32);

            pu4Data = (u32 *)prCmdInfo->pvInformationBuffer;
            *pu4Data = (u32)prEventStatistics->rFCSErrorCount.QuadPart;
            /* @FIXME, RX_ERROR_DROP_COUNT/RX_FIFO_FULL_DROP_COUNT
             * is not calculated */
        } else {
            u4QueryInfoLen = sizeof(u64);

            pu8Data = (u64 *)prCmdInfo->pvInformationBuffer;
            *pu8Data = prEventStatistics->rFCSErrorCount.QuadPart;
            /* @FIXME, RX_ERROR_DROP_COUNT/RX_FIFO_FULL_DROP_COUNT
             * is not calculated */
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryRecvNoBuffer(IN P_ADAPTER_T prAdapter,
                                  IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                  IN u32 u4EventBufLen)
{
    P_EVENT_STATISTICS prEventStatistics;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;
    u32 *pu4Data;
    u64 *pu8Data;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    prEventStatistics = (P_EVENT_STATISTICS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        if (prCmdInfo->u4InformationBufferLength == sizeof(u32)) {
            u4QueryInfoLen = sizeof(u32);

            pu4Data = (u32 *)prCmdInfo->pvInformationBuffer;
            *pu4Data = 0;  /* @FIXME? */
        } else {
            u4QueryInfoLen = sizeof(u64);

            pu8Data = (u64 *)prCmdInfo->pvInformationBuffer;
            *pu8Data = 0;  /* @FIXME? */
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryRecvCrcError(IN P_ADAPTER_T prAdapter,
                                  IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                  IN u32 u4EventBufLen)
{
    P_EVENT_STATISTICS prEventStatistics;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;
    u32 *pu4Data;
    u64 *pu8Data;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    prEventStatistics = (P_EVENT_STATISTICS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        if (prCmdInfo->u4InformationBufferLength == sizeof(u32)) {
            u4QueryInfoLen = sizeof(u32);

            pu4Data = (u32 *)prCmdInfo->pvInformationBuffer;
            *pu4Data = (u32)prEventStatistics->rFCSErrorCount.QuadPart;
        } else {
            u4QueryInfoLen = sizeof(u64);

            pu8Data = (u64 *)prCmdInfo->pvInformationBuffer;
            *pu8Data = prEventStatistics->rFCSErrorCount.QuadPart;
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryRecvErrorAlignment(IN P_ADAPTER_T prAdapter,
                                        IN P_CMD_INFO_T prCmdInfo,
                                        IN u8 *pucEventBuf,
                                        IN u32 u4EventBufLen)
{
    P_EVENT_STATISTICS prEventStatistics;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;
    u32 *pu4Data;
    u64 *pu8Data;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    prEventStatistics = (P_EVENT_STATISTICS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        if (prCmdInfo->u4InformationBufferLength == sizeof(u32)) {
            u4QueryInfoLen = sizeof(u32);

            pu4Data = (u32 *)prCmdInfo->pvInformationBuffer;
            *pu4Data = (u32)0;  /* @FIXME */
        } else {
            u4QueryInfoLen = sizeof(u64);

            pu8Data = (u64 *)prCmdInfo->pvInformationBuffer;
            *pu8Data = 0;  /* @FIXME */
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryXmitOneCollision(IN P_ADAPTER_T prAdapter,
                                      IN P_CMD_INFO_T prCmdInfo,
                                      IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    P_EVENT_STATISTICS prEventStatistics;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;
    u32 *pu4Data;
    u64 *pu8Data;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    prEventStatistics = (P_EVENT_STATISTICS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        if (prCmdInfo->u4InformationBufferLength == sizeof(u32)) {
            u4QueryInfoLen = sizeof(u32);

            pu4Data = (u32 *)prCmdInfo->pvInformationBuffer;
            *pu4Data = (u32)(prEventStatistics->rMultipleRetryCount.QuadPart -
                             prEventStatistics->rRetryCount.QuadPart);
        } else {
            u4QueryInfoLen = sizeof(u64);

            pu8Data = (u64 *)prCmdInfo->pvInformationBuffer;
            *pu8Data = (u64)(prEventStatistics->rMultipleRetryCount.QuadPart -
                             prEventStatistics->rRetryCount.QuadPart);
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryXmitMoreCollisions(IN P_ADAPTER_T prAdapter,
                                        IN P_CMD_INFO_T prCmdInfo,
                                        IN u8 *pucEventBuf,
                                        IN u32 u4EventBufLen)
{
    P_EVENT_STATISTICS prEventStatistics;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;
    u32 *pu4Data;
    u64 *pu8Data;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    prEventStatistics = (P_EVENT_STATISTICS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        if (prCmdInfo->u4InformationBufferLength == sizeof(u32)) {
            u4QueryInfoLen = sizeof(u32);

            pu4Data = (u32 *)prCmdInfo->pvInformationBuffer;
            *pu4Data = (u32)prEventStatistics->rMultipleRetryCount.QuadPart;
        } else {
            u4QueryInfoLen = sizeof(u64);

            pu8Data = (u64 *)prCmdInfo->pvInformationBuffer;
            *pu8Data = (u64)prEventStatistics->rMultipleRetryCount.QuadPart;
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryXmitMaxCollisions(IN P_ADAPTER_T prAdapter,
                                       IN P_CMD_INFO_T prCmdInfo,
                                       IN u8 *pucEventBuf,
                                       IN u32 u4EventBufLen)
{
    P_EVENT_STATISTICS prEventStatistics;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;
    u32 *pu4Data;
    u64 *pu8Data;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    prEventStatistics = (P_EVENT_STATISTICS)pucEventBuf;

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        if (prCmdInfo->u4InformationBufferLength == sizeof(u32)) {
            u4QueryInfoLen = sizeof(u32);

            pu4Data = (u32 *)prCmdInfo->pvInformationBuffer;
            *pu4Data = (u32)prEventStatistics->rFailedCount.QuadPart;
        } else {
            u4QueryInfoLen = sizeof(u64);

            pu8Data = (u64 *)prCmdInfo->pvInformationBuffer;
            *pu8Data = (u64)prEventStatistics->rFailedCount.QuadPart;
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when command by OID/ioctl has been timeout
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 *
 * @return true
 *         false
 */
/*----------------------------------------------------------------------------*/
void nicOidCmdTimeoutCommon(IN P_ADAPTER_T prAdapter,
                            IN P_CMD_INFO_T prCmdInfo)
{
    ASSERT(prAdapter);

    if (prCmdInfo->fgIsOid) {
        kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0,
                       WLAN_STATUS_FAILURE);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is a generic command timeout handler
 *
 * @param pfnOidHandler      Pointer to the OID handler
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void nicCmdTimeoutCommon(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo)
{
    ASSERT(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when command for entering RF test has
 *        failed sending due to timeout (highly possibly by firmware crash)
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/
void nicOidCmdEnterRFTestTimeout(IN P_ADAPTER_T prAdapter,
                                 IN P_CMD_INFO_T prCmdInfo)
{
    ASSERT(prAdapter);

    /* 1. Remove pending TX frames */
    nicTxRelease(prAdapter, true);

    /* 1.1 clear pending Security / Management Frames */
    kalClearSecurityFrames(prAdapter->prGlueInfo);
    kalClearMgmtFrames(prAdapter->prGlueInfo);

    /* 1.2 clear pending TX packet queued in glue layer */
    kalFlushPendingTxPackets(prAdapter->prGlueInfo);

    /* 2. indicate for OID failure */
    kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0,
                   WLAN_STATUS_FAILURE);
}

#if CFG_SUPPORT_QA_TOOL
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when received dump memory event packet.
 *        transfer the memory data to the IQ format data and write into file
 *
 * @param prIQAry     Pointer to the array store I or Q data.
 *  prDataLen   The return data length - bytes
 *        u4IQ        0: get I data
 *  1 : get Q data
 *
 * @return -1: open file error
 *
 */
/*----------------------------------------------------------------------------*/
s32 GetIQData(s32 **prIQAry, u32 *prDataLen, u32 u4IQ, u32 u4GetWf1)
{
    u8 aucPath[50];  /* the path for iq data dump out */
    u8 aucData[50];  /* iq data in string format */
    u32 i = 0, j = 0, count = 0;
    s32 ret = -1;
    s32 rv;
    struct file *file = NULL;

    *prIQAry = g_au4IQData;

    /* sprintf(aucPath, "/pattern.txt");             // CSD's Pattern */
    snprintf(aucPath, sizeof(aucPath), "/tmp/dump_out_%05ld_WF%d.txt",
             (g_u2DumpIndex - 1), u4GetWf1);
    if (kalCheckPath(aucPath) == -1) {
        snprintf(aucPath, sizeof(aucPath), "/data/dump_out_%05ld_WF%d.txt",
                 (g_u2DumpIndex - 1), u4GetWf1);
    }

    DBGLOG(INIT, INFO, "iCap Read Dump File dump_out_%05ld_WF%d.txt\n",
           (g_u2DumpIndex - 1), u4GetWf1);

    file = kalFileOpen(aucPath, O_RDONLY, 0);

    if ((file != NULL) && !IS_ERR(file)) {
        /* read 1K data per time */
        for (i = 0; i < RTN_IQ_DATA_LEN / sizeof(u32);
             i++, g_au4Offset[u4GetWf1][u4IQ] += IQ_FILE_LINE_OFFSET) {
            if (kalFileRead(file, g_au4Offset[u4GetWf1][u4IQ], aucData,
                            IQ_FILE_IQ_STR_LEN) == 0) {
                break;
            }

            count = 0;

            for (j = 0; j < 8; j++)
                if (aucData[j] != ' ') {
                    aucData[count++] = aucData[j];
                }

            aucData[count] = '\0';

            rv = kstrtoint(aucData, 0, &g_au4IQData[i]);  /* transfer
                                                           * data
                                                           * format
                                                           * (string
                                                           * to int)
                                                           */
        }
        *prDataLen = i * sizeof(u32);
        kalFileClose(file);
        ret = 0;
    }

    DBGLOG(INIT, INFO, "MT6632 : QA_AGENT GetIQData prDataLen = %d\n",
           *prDataLen);
    DBGLOG(INIT, INFO, "MT6632 : QA_AGENT GetIQData i = %d\n", i);

    return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when received dump memory event packet.
 *        transfer the memory data to the IQ format data and write into file
 *
 * @param prEventDumpMem     Pointer to the event dump memory structure.
 *
 * @return 0: SUCCESS, -1: FAIL
 *
 */
/*----------------------------------------------------------------------------*/

u32 TsfRawData2IqFmt(P_EVENT_DUMP_MEM_T prEventDumpMem)
{
    static u8 aucPathWF0[40];  /* the path for iq data dump out */
    static u8 aucPathWF1[40];  /* the path for iq data dump out */
    static u8 aucPathRAWWF0[40];  /* the path for iq data dump out */
    static u8 aucPathRAWWF1[40];  /* the path for iq data dump out */
    u8 *pucDataWF0 = NULL;  /* the data write into file */
    u8 *pucDataWF1 = NULL;  /* the data write into file */
    u8 *pucDataRAWWF0 = NULL;  /* the data write into file */
    u8 *pucDataRAWWF1 = NULL;  /* the data write into file */
    u32 u4SrcOffset;  /* record the buffer offset */
    u32 u4FmtLen = 0;  /* bus format length */
    u32 u4CpyLen = 0;
    u32 u4RemainByte;
    u32 u4DataWBufSize = 150;
    u32 u4DataRAWWBufSize = 150;
    u32 u4DataWLenF0 = 0;
    u32 u4DataWLenF1 = 0;
    u32 u4DataRAWWLenF0 = 0;
    u32 u4DataRAWWLenF1 = 0;

    u8 fgAppend;
    s32 u4Iqc160WF0Q0, u4Iqc160WF1I1;

    static u8 ucDstOffset;  /* for alignment. bcs we send 2KB data per
                             * packet,*/
    /*the data will not align in 12 bytes case. */
    static u32 u4CurTimeTick;

    static ICAP_BUS_FMT icapBusData;
    u32 *ptr;

    pucDataWF0 = kmalloc(u4DataWBufSize, GFP_KERNEL);
    pucDataWF1 = kmalloc(u4DataWBufSize, GFP_KERNEL);
    pucDataRAWWF0 = kmalloc(u4DataRAWWBufSize, GFP_KERNEL);
    pucDataRAWWF1 = kmalloc(u4DataRAWWBufSize, GFP_KERNEL);

    if ((!pucDataWF0) || (!pucDataWF1) || (!pucDataRAWWF0) ||
        (!pucDataRAWWF1)) {
        DBGLOG(INIT, ERROR, "kmalloc failed.\n");
        kfree(pucDataWF0);
        kfree(pucDataWF1);
        kfree(pucDataRAWWF0);
        kfree(pucDataRAWWF1);
        ASSERT(-1);
        return -1;
    }

    fgAppend = true;
    if (prEventDumpMem->ucFragNum == 1) {
        u4CurTimeTick = kalGetTimeTick();
        /* Store memory dump into sdcard,
         * path /sdcard/dump_<current  system tick>_<memory
         * address>_<memory length>.hex
         */

        /*if blbist mkdir undre /data/blbist, the dump files wouls put
         * on it */
        scnprintf(aucPathWF0, sizeof(aucPathWF0), "/tmp/dump_out_%05ld_WF0.txt",
                  g_u2DumpIndex);
        scnprintf(aucPathWF1, sizeof(aucPathWF1), "/tmp/dump_out_%05ld_WF1.txt",
                  g_u2DumpIndex);
        if (kalCheckPath(aucPathWF0) == -1) {
            kalMemSet(aucPathWF0, 0x00, sizeof(aucPathWF0));
            scnprintf(aucPathWF0, sizeof(aucPathWF0),
                      "/data/dump_out_%05ld_WF0.txt", g_u2DumpIndex);
        } else {
            kalTrunkPath(aucPathWF0);
        }

        if (kalCheckPath(aucPathWF1) == -1) {
            kalMemSet(aucPathWF1, 0x00, sizeof(aucPathWF1));
            scnprintf(aucPathWF1, sizeof(aucPathWF1),
                      "/data/dump_out_%05ld_WF1.txt", g_u2DumpIndex);
        } else {
            kalTrunkPath(aucPathWF1);
        }

        scnprintf(aucPathRAWWF0, sizeof(aucPathRAWWF0),
                  "/dump_RAW_%05ld_WF0.txt", g_u2DumpIndex);
        scnprintf(aucPathRAWWF1, sizeof(aucPathRAWWF1),
                  "/dump_RAW_%05ld_WF1.txt", g_u2DumpIndex);
        if (kalCheckPath(aucPathRAWWF0) == -1) {
            kalMemSet(aucPathRAWWF0, 0x00, sizeof(aucPathRAWWF0));
            scnprintf(aucPathRAWWF0, sizeof(aucPathRAWWF0),
                      "/data/dump_RAW_%05ld_WF0.txt", g_u2DumpIndex);
        } else {
            kalTrunkPath(aucPathRAWWF0);
        }

        if (kalCheckPath(aucPathRAWWF1) == -1) {
            kalMemSet(aucPathRAWWF1, 0x00, sizeof(aucPathRAWWF1));
            scnprintf(aucPathRAWWF1, sizeof(aucPathRAWWF1),
                      "/data/dump_RAW_%05ld_WF1.txt", g_u2DumpIndex);
        } else {
            kalTrunkPath(aucPathRAWWF1);
        }

        /* fgAppend = false; */
    }

    ptr = (u32 *)(&prEventDumpMem->aucBuffer[0]);
    /*DBGLOG(INIT, INFO, ": ==> (prEventDumpMem = %08x %08x %08x)\n",
     **(ptr), *(ptr + 4), *(ptr + 8));*/
    /*DBGLOG(INIT, INFO, ": ==> (prEventDumpMem->eIcapContent = %x)\n",
     * prEventDumpMem->eIcapContent);*/

    for (u4SrcOffset = 0, u4RemainByte = prEventDumpMem->u4Length;
         u4RemainByte > 0;) {
        u4FmtLen = (prEventDumpMem->eIcapContent == ICAP_CONTENT_SPECTRUM) ?
                   sizeof(SPECTRUM_BUS_FMT_T) :
                   sizeof(ICAP_BUS_FMT);
        /* 4 bytes : 12 bytes */
        u4CpyLen = (u4RemainByte - u4FmtLen >= 0) ? u4FmtLen : u4RemainByte;

        if ((ucDstOffset + u4CpyLen) > sizeof(icapBusData)) {
            DBGLOG(
                INIT, ERROR,
                "ucDstOffset(%u) + u4CpyLen(%u) exceed bound of icapBusData\n",
                ucDstOffset, u4CpyLen);
            kfree(pucDataWF0);
            kfree(pucDataWF1);
            kfree(pucDataRAWWF0);
            kfree(pucDataRAWWF1);
            ASSERT(-1);
            return -1;
        }
        memcpy((u8 *)&icapBusData + ucDstOffset,
               &prEventDumpMem->aucBuffer[0] + u4SrcOffset, u4CpyLen);

        if (prEventDumpMem->eIcapContent == ICAP_CONTENT_FIIQ ||
            prEventDumpMem->eIcapContent == ICAP_CONTENT_FDIQ) {
            u4DataWLenF0 = scnprintf(pucDataWF0, u4DataWBufSize, "%8d,%8d\n",
                                     icapBusData.rIqcBusData.u4Iqc0I,
                                     icapBusData.rIqcBusData.u4Iqc0Q);
            u4DataWLenF1 = scnprintf(pucDataWF1, u4DataWBufSize, "%8d,%8d\n",
                                     icapBusData.rIqcBusData.u4Iqc1I,
                                     icapBusData.rIqcBusData.u4Iqc1Q);
        } else if (prEventDumpMem->eIcapContent - 1000 == ICAP_CONTENT_FIIQ ||
                   prEventDumpMem->eIcapContent - 1000 == ICAP_CONTENT_FDIQ) {
            u4Iqc160WF0Q0 = icapBusData.rIqc160BusData.u4Iqc0Q0P1 |
                            (icapBusData.rIqc160BusData.u4Iqc0Q0P2 << 8);
            u4Iqc160WF1I1 = icapBusData.rIqc160BusData.u4Iqc1I1P1 |
                            (icapBusData.rIqc160BusData.u4Iqc1I1P2 << 4);

            u4DataWLenF0 =
                scnprintf(pucDataWF0, u4DataWBufSize, "%8d,%8d\n%8d,%8d\n",
                          icapBusData.rIqc160BusData.u4Iqc0I0, u4Iqc160WF0Q0,
                          icapBusData.rIqc160BusData.u4Iqc0I1,
                          icapBusData.rIqc160BusData.u4Iqc0Q1);

            u4DataWLenF1 =
                scnprintf(pucDataWF1, u4DataWBufSize, "%8d,%8d\n%8d,%8d\n",
                          icapBusData.rIqc160BusData.u4Iqc1I0,
                          icapBusData.rIqc160BusData.u4Iqc1Q0, u4Iqc160WF1I1,
                          icapBusData.rIqc160BusData.u4Iqc1Q1);
        } else if (prEventDumpMem->eIcapContent == ICAP_CONTENT_SPECTRUM) {
            u4DataWLenF0 = scnprintf(pucDataWF0, u4DataWBufSize, "%8d,%8d\n",
                                     icapBusData.rSpectrumBusData.u4DcocI,
                                     icapBusData.rSpectrumBusData.u4DcocQ);
        } else if (prEventDumpMem->eIcapContent == ICAP_CONTENT_ADC) {
            u4DataWLenF0 = scnprintf(
                pucDataWF0, u4DataWBufSize,
                "%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n",
                icapBusData.rPackedAdcBusData.u4AdcI0T0,
                icapBusData.rPackedAdcBusData.u4AdcQ0T0,
                icapBusData.rPackedAdcBusData.u4AdcI0T1,
                icapBusData.rPackedAdcBusData.u4AdcQ0T1,
                icapBusData.rPackedAdcBusData.u4AdcI0T2,
                icapBusData.rPackedAdcBusData.u4AdcQ0T2,
                icapBusData.rPackedAdcBusData.u4AdcI0T3,
                icapBusData.rPackedAdcBusData.u4AdcQ0T3,
                icapBusData.rPackedAdcBusData.u4AdcI0T4,
                icapBusData.rPackedAdcBusData.u4AdcQ0T4,
                icapBusData.rPackedAdcBusData.u4AdcI0T5,
                icapBusData.rPackedAdcBusData.u4AdcQ0T5);

            u4DataWLenF1 = scnprintf(
                pucDataWF1, u4DataWBufSize,
                "%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n",
                icapBusData.rPackedAdcBusData.u4AdcI1T0,
                icapBusData.rPackedAdcBusData.u4AdcQ1T0,
                icapBusData.rPackedAdcBusData.u4AdcI1T1,
                icapBusData.rPackedAdcBusData.u4AdcQ1T1,
                icapBusData.rPackedAdcBusData.u4AdcI1T2,
                icapBusData.rPackedAdcBusData.u4AdcQ1T2,
                icapBusData.rPackedAdcBusData.u4AdcI1T3,
                icapBusData.rPackedAdcBusData.u4AdcQ1T3,
                icapBusData.rPackedAdcBusData.u4AdcI1T4,
                icapBusData.rPackedAdcBusData.u4AdcQ1T4,
                icapBusData.rPackedAdcBusData.u4AdcI1T5,
                icapBusData.rPackedAdcBusData.u4AdcQ1T5);
        } else if (prEventDumpMem->eIcapContent - 2000 == ICAP_CONTENT_ADC) {
            u4DataWLenF0 = scnprintf(pucDataWF0, u4DataWBufSize,
                                     "%8d,%8d\n%8d,%8d\n%8d,%8d\n",
                                     icapBusData.rPackedAdcBusData.u4AdcI0T0,
                                     icapBusData.rPackedAdcBusData.u4AdcQ0T0,
                                     icapBusData.rPackedAdcBusData.u4AdcI0T1,
                                     icapBusData.rPackedAdcBusData.u4AdcQ0T1,
                                     icapBusData.rPackedAdcBusData.u4AdcI0T2,
                                     icapBusData.rPackedAdcBusData.u4AdcQ0T2);

            u4DataWLenF1 = scnprintf(pucDataWF1, u4DataWBufSize,
                                     "%8d,%8d\n%8d,%8d\n%8d,%8d\n",
                                     icapBusData.rPackedAdcBusData.u4AdcI1T0,
                                     icapBusData.rPackedAdcBusData.u4AdcQ1T0,
                                     icapBusData.rPackedAdcBusData.u4AdcI1T1,
                                     icapBusData.rPackedAdcBusData.u4AdcQ1T1,
                                     icapBusData.rPackedAdcBusData.u4AdcI1T2,
                                     icapBusData.rPackedAdcBusData.u4AdcQ1T2);
        } else if (prEventDumpMem->eIcapContent == ICAP_CONTENT_TOAE) {
            /* actually, this is DCOC. we take TOAE as DCOC */
            u4DataWLenF0 = scnprintf(pucDataWF0, u4DataWBufSize, "%8d,%8d\n",
                                     icapBusData.rAdcBusData.u4Dcoc0I,
                                     icapBusData.rAdcBusData.u4Dcoc0Q);
            u4DataWLenF1 = scnprintf(pucDataWF1, u4DataWBufSize, "%8d,%8d\n",
                                     icapBusData.rAdcBusData.u4Dcoc1I,
                                     icapBusData.rAdcBusData.u4Dcoc1Q);
        }
        if (u4CpyLen == u4FmtLen) {  /* the data format is complete */
            kalWriteToFile(aucPathWF0, fgAppend, pucDataWF0, u4DataWLenF0);
            kalWriteToFile(aucPathWF1, fgAppend, pucDataWF1, u4DataWLenF1);
        }
        ptr = (u32 *)(&prEventDumpMem->aucBuffer[0] + u4SrcOffset);
        u4DataRAWWLenF0 = scnprintf(pucDataRAWWF0, u4DataWBufSize,
                                    "%08x%08x%08x\n", *(ptr + 2), *(ptr + 1),
                                    *ptr);
        kalWriteToFile(aucPathRAWWF0, fgAppend, pucDataRAWWF0, u4DataRAWWLenF0);
        kalWriteToFile(aucPathRAWWF1, fgAppend, pucDataRAWWF1, u4DataRAWWLenF1);

        u4RemainByte -= u4CpyLen;
        u4SrcOffset += u4CpyLen;  /* shift offset */
        ucDstOffset = 0;  /* only use ucDstOffset at first packet for
                           * align 2KB */
    }
    /* if this is a last packet, we can't transfer the remain data.
     *  bcs we can't guarantee the data is complete align data format
     */
    if (u4CpyLen != u4FmtLen) {  /* the data format is complete */
        ucDstOffset = u4CpyLen;  /* not align 2KB, keep the data and next
                                  * packet data will append it */
    }

    kfree(pucDataWF0);
    kfree(pucDataWF1);
    kfree(pucDataRAWWF0);
    kfree(pucDataRAWWF1);

    if (u4RemainByte < 0) {
        ASSERT(-1);
        return -1;
    }

    return 0;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to handle dump burst event
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo         Pointer to the command information
 * @param pucEventBuf       Pointer to event buffer
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/

void nicEventQueryMemDump(IN P_ADAPTER_T prAdapter, IN u8 *pucEventBuf,
                          IN u32 u4EventBufLen)
{
    P_EVENT_DUMP_MEM_T prEventDumpMem;
    static u8 aucPath[256] = { 0 };  // initialized all zeros
    static u8 aucPath_done[300];
    static u32 u4CurTimeTick;

    ASSERT(prAdapter);
    ASSERT(pucEventBuf);

    snprintf(aucPath, sizeof(aucPath), "/dump_%05ld.hex", g_u2DumpIndex);

    if (u4EventBufLen < sizeof(EVENT_DUMP_MEM_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_DUMP_MEM_T));
        return;
    }
    prEventDumpMem = (P_EVENT_DUMP_MEM_T)(pucEventBuf);
    if (u4EventBufLen < sizeof(EVENT_DUMP_MEM_T) + prEventDumpMem->u4Length) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen,
               sizeof(EVENT_DUMP_MEM_T) + prEventDumpMem->u4Length);
        return;
    }
    if (kalCheckPath(aucPath) == -1) {
        kalMemSet(aucPath, 0x00, 256);
        snprintf(aucPath, sizeof(aucPath), "/data/dump_%05ld.hex",
                 g_u2DumpIndex);
    }

    if (prEventDumpMem->ucFragNum == 1) {
        /* Store memory dump into sdcard,
         * path /sdcard/dump_<current  system tick>_<memory
         * address>_<memory length>.hex
         */
        u4CurTimeTick = kalGetTimeTick();

        /*if blbist mkdir undre /data/blbist, the dump files wouls put
         * on it */
        snprintf(aucPath, sizeof(aucPath), "/dump_%05ld.hex", g_u2DumpIndex);
        if (kalCheckPath(aucPath) == -1) {
            kalMemSet(aucPath, 0x00, 256);
            snprintf(aucPath, sizeof(aucPath), "/data/dump_%05ld.hex",
                     g_u2DumpIndex);
        }

        kalWriteToFile(aucPath, false, &prEventDumpMem->aucBuffer[0],
                       prEventDumpMem->u4Length);
    } else {
        /* Append current memory dump to the hex file */
        kalWriteToFile(aucPath, true, &prEventDumpMem->aucBuffer[0],
                       prEventDumpMem->u4Length);
    }
#if CFG_SUPPORT_QA_TOOL
    TsfRawData2IqFmt(prEventDumpMem);
#endif
    DBGLOG(INIT, INFO, "iCap : ==> (u4RemainLength = %x, u4Address=%x )\n",
           prEventDumpMem->u4RemainLength, prEventDumpMem->u4Address);

    if (prEventDumpMem->u4RemainLength == 0 ||
        prEventDumpMem->u4Address == 0xFFFFFFFF) {
        /* The request is finished or firmware response a error */
        /* Reply time tick to iwpriv */

        g_bIcapEnable = false;
        g_bCaptureDone = true;

        snprintf(aucPath_done, sizeof(aucPath_done), "/file_dump_done.txt");
        if (kalCheckPath(aucPath_done) == -1) {
            kalMemSet(aucPath_done, 0x00, 256);
            snprintf(aucPath_done, sizeof(aucPath_done),
                     "/data/file_dump_done.txt");
        }
        DBGLOG(INIT, INFO, ": ==> gen done_file\n");
        kalWriteToFile(aucPath_done, false, aucPath_done, sizeof(aucPath_done));
#if CFG_SUPPORT_QA_TOOL
        g_au4Offset[0][0] = 0;
        g_au4Offset[0][1] = 9;
        g_au4Offset[1][0] = 0;
        g_au4Offset[1][1] = 9;
#endif

        g_u2DumpIndex++;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when command for memory dump has
 *        replied a event.
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo         Pointer to the command information
 * @param pucEventBuf       Pointer to event buffer
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventQueryMemDump(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_GLUE_INFO_T prGlueInfo;
    P_EVENT_DUMP_MEM_T prEventDumpMem;
    static u8 aucPath[256] = { 0 };  // initialized all zeros
    /* static u8 aucPath_done[300]; */
    static u32 u4CurTimeTick;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    /* 4 <2> Update information of OID */
    if (1) {
        prGlueInfo = prAdapter->prGlueInfo;
        if (u4EventBufLen < sizeof(EVENT_DUMP_MEM_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_DUMP_MEM_T));
            return;
        }
        prEventDumpMem = (P_EVENT_DUMP_MEM_T)(pucEventBuf);
        if (u4EventBufLen <
            sizeof(EVENT_DUMP_MEM_T) + prEventDumpMem->u4Length) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen,
                   sizeof(EVENT_DUMP_MEM_T) + prEventDumpMem->u4Length);
            return;
        }
        u4QueryInfoLen = sizeof(P_PARAM_CUSTOM_MEM_DUMP_STRUCT_T);

        /* Currently, only allow to dump max to 4K bytes (per command).
         */
        if (prEventDumpMem->u4Length > MAX_MEMORY_DUMP_SIZE) {
            DBGLOG(INIT, WARN, "SKIP DUMP FILE: Invalid u4Length (%d)\n",
                   prEventDumpMem->u4Length);
            goto write_file_done;
        }

        if (prEventDumpMem->ucFragNum == 1) {
            /* Store memory dump into sdcard,
             * path /sdcard/dump_<current  system tick>_<memory
             * address>_<memory length>.hex
             */
            u4CurTimeTick = kalGetTimeTick();

            /* PeiHsuan add for avoiding out of memory 20160801 */
            if (g_u2DumpIndex >= 20) {
                g_u2DumpIndex = 0;
            }

            /*if blbist mkdir undre /data/blbist, the dump files
             * wouls put on it */
            snprintf(aucPath, sizeof(aucPath), "/dump_%05ld.hex",
                     g_u2DumpIndex);
            if (kalCheckPath(aucPath) == -1) {
                kalMemSet(aucPath, 0x00, 256);
                snprintf(aucPath, sizeof(aucPath), "/data/dump_%05ld.hex",
                         g_u2DumpIndex);
            } else {
                kalTrunkPath(aucPath);
            }

            DBGLOG(INIT, INFO, "iCap Create New Dump File dump_%05ld.hex\n",
                   g_u2DumpIndex);

            kalWriteToFile(aucPath, false, &prEventDumpMem->aucBuffer[0],
                           prEventDumpMem->u4Length);
        } else {
            if (kalCheckPath(aucPath) == -1) {
                DBGLOG(
                    INIT, WARN,
                    "Dump file:%s invalid while receiving frag[%d] pkt (not 1st "
                    "frag), skip dumpping.\n",
                    aucPath,
                    prEventDumpMem->ucFragNum);
            } else {
                /* Append current memory dump to the hex file */
                kalWriteToFile(aucPath, true, &prEventDumpMem->aucBuffer[0],
                               prEventDumpMem->u4Length);
            }
        }
#if CFG_SUPPORT_QA_TOOL
        TsfRawData2IqFmt(prEventDumpMem);
#endif
write_file_done:
        if (prEventDumpMem->u4RemainLength == 0 ||
            prEventDumpMem->u4Address == 0xFFFFFFFF) {
            /* The request is finished or firmware response a error
             */
            /* Reply time tick to iwpriv */
            if (prCmdInfo->fgIsOid) {
                /* the oid would be complete only in oid-trigger
                 * mode, that is no need to if the event-trigger
                 */
                if (g_bIcapEnable == false) {
                    *((u32 *)prCmdInfo->pvInformationBuffer) = u4CurTimeTick;
                    kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
                                   u4QueryInfoLen, WLAN_STATUS_SUCCESS);
                }
            }
            g_bIcapEnable = false;
            g_bCaptureDone = true;
            g_u2DumpIndex++;
        } else {
        }
    }

    return;
}

#if CFG_SUPPORT_BATCH_SCAN
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when event for SUPPORT_BATCH_SCAN
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 * @param pucEventBuf        Pointer to the event buffer
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventBatchScanResult(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_EVENT_BATCH_RESULT_T prEventBatchResult;
    P_GLUE_INFO_T prGlueInfo;

    DBGLOG(SCN, TRACE, "nicCmdEventBatchScanResult");

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    /* 4 <2> Update information of OID */
    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
        if (prAdapter->fgTestMode == false &&
            u4EventBufLen < sizeof(EVENT_BATCH_RESULT_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_BATCH_RESULT_T));
            return;
        }
        prEventBatchResult = (P_EVENT_BATCH_RESULT_T)pucEventBuf;

        u4QueryInfoLen = sizeof(EVENT_BATCH_RESULT_T);
        kalMemCopy(prCmdInfo->pvInformationBuffer, prEventBatchResult,
                   sizeof(EVENT_BATCH_RESULT_T));

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}
#endif

#if CFG_SUPPORT_BUILD_DATE_CODE
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when event for build date code information
 *        has been retrieved
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 * @param pucEventBuf        Pointer to the event buffer
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventBuildDateCode(IN P_ADAPTER_T prAdapter,
                              IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                              IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_EVENT_BUILD_DATE_CODE prEvent;
    P_GLUE_INFO_T prGlueInfo;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);

    /* 4 <2> Update information of OID */
    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        if (u4EventBufLen < sizeof(EVENT_BUILD_DATE_CODE)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_BUILD_DATE_CODE));
            return;
        }
        prEvent = (P_EVENT_BUILD_DATE_CODE)pucEventBuf;

        u4QueryInfoLen = sizeof(u8) * 16;
        kalMemCopy(prCmdInfo->pvInformationBuffer, prEvent->aucDateCode,
                   sizeof(u8) * 16);

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when event for query STA link status
 *        has been retrieved
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 * @param pucEventBuf        Pointer to the event buffer
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventQueryStaStatistics(IN P_ADAPTER_T prAdapter,
                                   IN P_CMD_INFO_T prCmdInfo,
                                   IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_EVENT_STA_STATISTICS_T prEvent;
    P_GLUE_INFO_T prGlueInfo;
    P_PARAM_GET_STA_STATISTICS prStaStatistics;
    ENUM_WMM_ACI_T eAci;
    P_STA_RECORD_T prStaRec;
    u8 ucDbdcIdx;
    extern u32 g_au4RxMpduCnt[ENUM_BAND_NUM];
    extern u32 g_au4FcsError[ENUM_BAND_NUM];
    extern u32 g_au4RxFifoCnt[ENUM_BAND_NUM];
    extern u32 g_au4AmpduTxSfCnt[ENUM_BAND_NUM];
    extern u32 g_au4AmpduTxAckSfCnt[ENUM_BAND_NUM];

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);
    ASSERT(prCmdInfo->pvInformationBuffer);

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
        if (prAdapter->fgTestMode == false &&
            u4EventBufLen < sizeof(EVENT_STA_STATISTICS_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_STA_STATISTICS_T));
            return;
        }
        prEvent = (P_EVENT_STA_STATISTICS_T)pucEventBuf;
        prStaStatistics =
            (P_PARAM_GET_STA_STATISTICS)prCmdInfo->pvInformationBuffer;

        u4QueryInfoLen = sizeof(PARAM_GET_STA_STATISTICS);

        /* Statistics from FW is valid */
        if (prEvent->u4Flags & BIT(0)) {
            prStaStatistics->ucPer = prEvent->ucPer;
            prStaStatistics->ucRcpi = prEvent->ucRcpi;
            prStaStatistics->u4PhyMode = prEvent->u4PhyMode;
            prStaStatistics->u2LinkSpeed = prEvent->u2LinkSpeed;

            prStaStatistics->u4TxFailCount = prEvent->u4TxFailCount;
            prStaStatistics->u4TxLifeTimeoutCount =
                prEvent->u4TxLifeTimeoutCount;
            prStaStatistics->u4TransmitCount = prEvent->u4TransmitCount;
            prStaStatistics->u4TransmitFailCount = prEvent->u4TransmitFailCount;
            prStaStatistics->u4Rate1TxCnt = prEvent->u4Rate1TxCnt;
            prStaStatistics->u4Rate1FailCnt = prEvent->u4Rate1FailCnt;

            prStaStatistics->ucTemperature = prEvent->ucTemperature;
            prStaStatistics->ucSkipAr = prEvent->ucSkipAr;
            prStaStatistics->ucArTableIdx = prEvent->ucArTableIdx;
            prStaStatistics->ucRateEntryIdx = prEvent->ucRateEntryIdx;
            prStaStatistics->ucRateEntryIdxPrev = prEvent->ucRateEntryIdxPrev;
            prStaStatistics->ucTxSgiDetectPassCnt =
                prEvent->ucTxSgiDetectPassCnt;
            prStaStatistics->ucAvePer = prEvent->ucAvePer;
            kalMemCopy(prStaStatistics->aucArRatePer, prEvent->aucArRatePer,
                       sizeof(prEvent->aucArRatePer));
            kalMemCopy(prStaStatistics->aucRateEntryIndex,
                       prEvent->aucRateEntryIndex,
                       sizeof(prEvent->aucRateEntryIndex));
            prStaStatistics->ucArStateCurr = prEvent->ucArStateCurr;
            prStaStatistics->ucArStatePrev = prEvent->ucArStatePrev;
            prStaStatistics->ucArActionType = prEvent->ucArActionType;
            prStaStatistics->ucHighestRateCnt = prEvent->ucHighestRateCnt;
            prStaStatistics->ucLowestRateCnt = prEvent->ucLowestRateCnt;
            prStaStatistics->u2TrainUp = prEvent->u2TrainUp;
            prStaStatistics->u2TrainDown = prEvent->u2TrainDown;
            kalMemCopy(&prStaStatistics->rTxVector, &prEvent->rTxVector,
                       sizeof(prEvent->rTxVector));
            kalMemCopy(&prStaStatistics->rMibInfo, &prEvent->rMibInfo,
                       sizeof(prEvent->rMibInfo));
            for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM; ucDbdcIdx++) {
                g_au4RxMpduCnt[ucDbdcIdx] +=
                    prStaStatistics->rMibInfo[ucDbdcIdx].u4RxMpduCnt;
                g_au4FcsError[ucDbdcIdx] +=
                    prStaStatistics->rMibInfo[ucDbdcIdx].u4FcsError;
                g_au4RxFifoCnt[ucDbdcIdx] +=
                    prStaStatistics->rMibInfo[ucDbdcIdx].u4RxFifoFull;
                g_au4AmpduTxSfCnt[ucDbdcIdx] +=
                    prStaStatistics->rMibInfo[ucDbdcIdx].u4AmpduTxSfCnt;
                g_au4AmpduTxAckSfCnt[ucDbdcIdx] +=
                    prStaStatistics->rMibInfo[ucDbdcIdx].u4AmpduTxAckSfCnt;
            }
            prStaStatistics->fgIsForceTxStream = prEvent->fgIsForceTxStream;
            prStaStatistics->fgIsForceSeOff = prEvent->fgIsForceSeOff;

            prStaRec = cnmGetStaRecByIndex(prAdapter, prEvent->ucStaRecIdx);

            if (prStaRec) {
                /*link layer statistics */
                for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
                    prStaStatistics->arLinkStatistics[eAci].u4TxFailMsdu =
                        prEvent->arLinkStatistics[eAci].u4TxFailMsdu;
                    prStaStatistics->arLinkStatistics[eAci].u4TxRetryMsdu =
                        prEvent->arLinkStatistics[eAci].u4TxRetryMsdu;

                    /*for dump bss statistics */
                    prStaRec->arLinkStatistics[eAci].u4TxFailMsdu =
                        prEvent->arLinkStatistics[eAci].u4TxFailMsdu;
                    prStaRec->arLinkStatistics[eAci].u4TxRetryMsdu =
                        prEvent->arLinkStatistics[eAci].u4TxRetryMsdu;
                }
            }
            if (prEvent->u4TxCount) {
                u32 u4TxDoneAirTimeMs =
                    USEC_TO_MSEC(prEvent->u4TxDoneAirTime * 32);

                prStaStatistics->u4TxAverageAirTime =
                    (u4TxDoneAirTimeMs / prEvent->u4TxCount);
            } else {
                prStaStatistics->u4TxAverageAirTime = 0;
            }
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when event for query LTE safe channels
 *        has been retrieved
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 * @param pucEventBuf        Pointer to the event buffer
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventQueryLteSafeChn(IN P_ADAPTER_T prAdapter,
                                IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                                IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_EVENT_LTE_SAFE_CHN_T prEvent;
    P_GLUE_INFO_T prGlueInfo;
    P_PARAM_GET_CHN_INFO prLteSafeChnInfo;
    u8 ucIdx = 0;

    if ((prAdapter == NULL) || (prCmdInfo == NULL) || (pucEventBuf == NULL) ||
        (prCmdInfo->pvInformationBuffer == NULL)) {
        ASSERT(false);
        return;
    }

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
        if (prAdapter->fgTestMode == false &&
            u4EventBufLen < sizeof(EVENT_LTE_SAFE_CHN_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_LTE_SAFE_CHN_T));
            return;
        }
        prEvent = (P_EVENT_LTE_SAFE_CHN_T)pucEventBuf;  /* FW responsed
                                                         * data */

        prLteSafeChnInfo = (P_PARAM_GET_CHN_INFO)prCmdInfo->pvInformationBuffer;

        u4QueryInfoLen = sizeof(PARAM_GET_CHN_INFO);

        /* Statistics from FW is valid */
        if (prEvent->u4Flags & BIT(0)) {
            for (ucIdx = 0; ucIdx < NL80211_TESTMODE_AVAILABLE_CHAN_ATTR_MAX;
                 ucIdx++) {
                prLteSafeChnInfo->rLteSafeChnList.au4SafeChannelBitmask[ucIdx] =
                    prEvent->rLteSafeChn.au4SafeChannelBitmask[ucIdx];

                DBGLOG(P2P, INFO, "[ACS]LTE safe channels[%d]=0x%08x\n", ucIdx,
                       prLteSafeChnInfo->rLteSafeChnList
                       .au4SafeChannelBitmask[ucIdx]);
            }
        }

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}
#endif

void nicEventRddPulseDump(IN P_ADAPTER_T prAdapter, IN u8 *pucEventBuf,
                          IN u32 u4EventBufLen)
{
    u16 u2Idx, u2PulseCnt;
    P_EVENT_WIFI_RDD_TEST_T prRddPulseEvent;

    ASSERT(prAdapter);
    ASSERT(pucEventBuf);
    if (u4EventBufLen < sizeof(EVENT_WIFI_RDD_TEST_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_WIFI_RDD_TEST_T));
        return;
    }
    prRddPulseEvent = (P_EVENT_WIFI_RDD_TEST_T)(pucEventBuf);

    u2PulseCnt = (prRddPulseEvent->u4FuncLength - RDD_EVENT_HDR_SIZE) /
                 RDD_ONEPLUSE_SIZE;
    if (u4EventBufLen <
        sizeof(EVENT_WIFI_RDD_TEST_T) + u2PulseCnt * RDD_ONEPLUSE_SIZE) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen,
               sizeof(EVENT_WIFI_RDD_TEST_T) + u2PulseCnt * RDD_ONEPLUSE_SIZE);
        return;
    }

    DBGLOG(INIT, INFO, "[RDD]0x%08x %08d[RDD%d]\n", prRddPulseEvent->u4Prefix,
           prRddPulseEvent->u4Count, prRddPulseEvent->ucRddIdx);

    for (u2Idx = 0; u2Idx < u2PulseCnt; u2Idx++) {
        DBGLOG(INIT, INFO, "[RDD]0x%02x%02x%02x%02x %02x%02x%02x%02x[RDD%d]\n",
               prRddPulseEvent
               ->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx + RDD_PULSE_OFFSET3],
               prRddPulseEvent
               ->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx + RDD_PULSE_OFFSET2],
               prRddPulseEvent
               ->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx + RDD_PULSE_OFFSET1],
               prRddPulseEvent
               ->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx + RDD_PULSE_OFFSET0],
               prRddPulseEvent
               ->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx + RDD_PULSE_OFFSET7],
               prRddPulseEvent
               ->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx + RDD_PULSE_OFFSET6],
               prRddPulseEvent
               ->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx + RDD_PULSE_OFFSET5],
               prRddPulseEvent
               ->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx + RDD_PULSE_OFFSET4],
               prRddPulseEvent->ucRddIdx);
    }

    DBGLOG(INIT, INFO, "[RDD]0x%08x %08x[RDD%d]\n",
           prRddPulseEvent->u4SubBandRssi0, prRddPulseEvent->u4SubBandRssi1,
           prRddPulseEvent->ucRddIdx);
}

#if CFG_SUPPORT_ADVANCE_CONTROL
void nicCmdEventQueryAdvCtrl(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen)
{
    u8 *query;
    P_GLUE_INFO_T prGlueInfo;
    u32 query_len;
    P_CMD_ADV_CONFIG_HEADER_T hdr;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);

    if (!pucEventBuf) {
        DBGLOG(REQ, ERROR, "pucEventBuf is null.\n");
        return;
    }
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(CMD_ADV_CONFIG_HEADER_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(CMD_ADV_CONFIG_HEADER_T));
        return;
    }
    hdr = (P_CMD_ADV_CONFIG_HEADER_T)pucEventBuf;
    DBGLOG(REQ, LOUD, "%s type %x len %d>\n", __func__, hdr->u2Type,
           hdr->u2Len);
    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        query_len = hdr->u2Len;
        query = prCmdInfo->pvInformationBuffer;
        if (query && (query_len == prCmdInfo->u4InformationBufferLength)) {
            kalMemCopy(query, hdr, query_len);
        } else {
            DBGLOG(REQ, LOUD, "%s type %x, len %d != buflen %d>\n", __func__,
                   hdr->u2Type, hdr->u2Len,
                   prCmdInfo->u4InformationBufferLength);
        }
        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, query_len,
                       WLAN_STATUS_SUCCESS);
    }
}
#endif

#if CFG_SUPPORT_MSP
void nicCmdEventQueryWlanInfo(IN P_ADAPTER_T prAdapter,
                              IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                              IN u32 u4EventBufLen)
{
    P_PARAM_HW_WLAN_INFO_T prWlanInfo;
    P_EVENT_WLAN_INFO prEventWlanInfo;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_WLAN_INFO)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_WLAN_INFO));
        return;
    }
    prEventWlanInfo = (P_EVENT_WLAN_INFO)pucEventBuf;

    DBGLOG(RSN, INFO, "MT6632 : nicCmdEventQueryWlanInfo\n");

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        u4QueryInfoLen = sizeof(PARAM_HW_WLAN_INFO_T);
        prWlanInfo = (P_PARAM_HW_WLAN_INFO_T)prCmdInfo->pvInformationBuffer;

        /* prWlanInfo->u4Length = sizeof(PARAM_HW_WLAN_INFO_T); */
        if (prEventWlanInfo && prWlanInfo) {
            kalMemCopy(&prWlanInfo->rWtblTxConfig,
                       &prEventWlanInfo->rWtblTxConfig,
                       sizeof(PARAM_TX_CONFIG_T));
            kalMemCopy(&prWlanInfo->rWtblSecConfig,
                       &prEventWlanInfo->rWtblSecConfig,
                       sizeof(PARAM_SEC_CONFIG_T));
            kalMemCopy(&prWlanInfo->rWtblKeyConfig,
                       &prEventWlanInfo->rWtblKeyConfig,
                       sizeof(PARAM_KEY_CONFIG_T));
            kalMemCopy(&prWlanInfo->rWtblRateInfo,
                       &prEventWlanInfo->rWtblRateInfo,
                       sizeof(PARAM_PEER_RATE_INFO_T));
            kalMemCopy(&prWlanInfo->rWtblBaConfig,
                       &prEventWlanInfo->rWtblBaConfig,
                       sizeof(PARAM_PEER_BA_CONFIG_T));
            kalMemCopy(&prWlanInfo->rWtblPeerCap,
                       &prEventWlanInfo->rWtblPeerCap,
                       sizeof(PARAM_PEER_CAP_T));
            kalMemCopy(&prWlanInfo->rWtblRxCounter,
                       &prEventWlanInfo->rWtblRxCounter,
                       sizeof(PARAM_PEER_RX_COUNTER_ALL_T));
            kalMemCopy(&prWlanInfo->rWtblTxCounter,
                       &prEventWlanInfo->rWtblTxCounter,
                       sizeof(PARAM_PEER_TX_COUNTER_ALL_T));
        }
        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}

void nicCmdEventQueryMibInfo(IN P_ADAPTER_T prAdapter,
                             IN P_CMD_INFO_T prCmdInfo, IN u8 *pucEventBuf,
                             IN u32 u4EventBufLen)
{
    P_PARAM_HW_MIB_INFO_T prMibInfo;
    P_EVENT_MIB_INFO prEventMibInfo;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4QueryInfoLen;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    if (u4EventBufLen < sizeof(EVENT_MIB_INFO)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_MIB_INFO));
        return;
    }
    prEventMibInfo = (P_EVENT_MIB_INFO)pucEventBuf;

    DBGLOG(RSN, INFO, "MT6632 : nicCmdEventQueryMibInfo\n");

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;

        u4QueryInfoLen = sizeof(PARAM_HW_MIB_INFO_T);
        prMibInfo = (P_PARAM_HW_MIB_INFO_T)prCmdInfo->pvInformationBuffer;
        if (prEventMibInfo && prMibInfo) {
            kalMemCopy(&prMibInfo->rHwMibCnt, &prEventMibInfo->rHwMibCnt,
                       sizeof(HW_MIB_COUNTER_T));
            kalMemCopy(&prMibInfo->rHwMib2Cnt, &prEventMibInfo->rHwMib2Cnt,
                       sizeof(HW_MIB2_COUNTER_T));
            kalMemCopy(&prMibInfo->rHwTxAmpduMts,
                       &prEventMibInfo->rHwTxAmpduMts,
                       sizeof(HW_TX_AMPDU_METRICS_T));
        }
        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}
#endif

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
void nicCmdEventTxMcsInfo(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                          IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_GLUE_INFO_T prGlueInfo;
    struct EVENT_TX_MCS_INFO *prTxMcsEvent;
    struct PARAM_TX_MCS_INFO *prTxMcsInfo;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);
    ASSERT(pucEventBuf);
    ASSERT(prCmdInfo->pvInformationBuffer);

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        if (u4EventBufLen < sizeof(struct EVENT_TX_MCS_INFO)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(struct EVENT_TX_MCS_INFO));
            return;
        }
        prTxMcsEvent = (struct EVENT_TX_MCS_INFO *)pucEventBuf;
        prTxMcsInfo =
            (struct PARAM_TX_MCS_INFO *)prCmdInfo->pvInformationBuffer;

        u4QueryInfoLen = sizeof(struct EVENT_TX_MCS_INFO);

        kalMemCopy(prTxMcsInfo->au2TxRateCode, prTxMcsEvent->au2TxRateCode,
                   sizeof(prTxMcsEvent->au2TxRateCode));
        kalMemCopy(prTxMcsInfo->aucTxRatePer, prTxMcsEvent->aucTxRatePer,
                   sizeof(prTxMcsEvent->aucTxRatePer));

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}
#endif

#if CFG_TCP_IP_CHKSUM_OFFLOAD
WLAN_STATUS nicCmdEventQueryNicCsumOffload(IN P_ADAPTER_T prAdapter,
                                           IN u8 *pucEventBuf,
                                           IN u32 u4EventBufLen)
{
    P_NIC_CSUM_OFFLOAD_T prChecksumOffload = (P_NIC_CSUM_OFFLOAD_T)pucEventBuf;
    if (u4EventBufLen < sizeof(NIC_CSUM_OFFLOAD_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(NIC_CSUM_OFFLOAD_T));
        return WLAN_STATUS_FAILURE;
    }
    prAdapter->fgIsSupportCsumOffload =
        prChecksumOffload->ucIsSupportCsumOffload;

    DBGLOG(INIT, INFO,
           "nicCmdEventQueryNicCsumOffload: ucIsSupportCsumOffload = %x\n",
           prAdapter->fgIsSupportCsumOffload);

    return WLAN_STATUS_SUCCESS;
}
#endif

WLAN_STATUS nicCmdEventQueryNicCoexFeature(IN P_ADAPTER_T prAdapter,
                                           IN u8 *pucEventBuf,
                                           IN u32 u4EventBufLen)
{
    P_NIC_COEX_FEATURE_T prCoexFeature = (P_NIC_COEX_FEATURE_T)pucEventBuf;
    if (u4EventBufLen < sizeof(NIC_COEX_FEATURE_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(NIC_COEX_FEATURE_T));
        return WLAN_STATUS_FAILURE;
    }
    prAdapter->u4FddMode = prCoexFeature->u4FddMode;

    DBGLOG(INIT, INFO, "nicCmdEventQueryNicCoexFeature: u4FddMode = %x\n",
           prAdapter->u4FddMode);

    return WLAN_STATUS_SUCCESS;
}

WLAN_STATUS nicCmdEventQueryNicEfuseAddr(IN P_ADAPTER_T prAdapter,
                                         IN u8 *pucEventBuf,
                                         IN u32 u4EventBufLen)
{
    P_NIC_EFUSE_ADDRESS_T prTxResource = (P_NIC_EFUSE_ADDRESS_T)pucEventBuf;
    if (u4EventBufLen < sizeof(NIC_EFUSE_ADDRESS_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(NIC_EFUSE_ADDRESS_T));
        return WLAN_STATUS_FAILURE;
    }
    prAdapter->u4EfuseStartAddress = prTxResource->u4EfuseStartAddress;
    prAdapter->u4EfuseEndAddress = prTxResource->u4EfuseEndAddress;

    DBGLOG(INIT, STATE,
           "nicCmdEventQueryNicEfuseAddr: u4EfuseStartAddress = %x\n",
           prAdapter->u4EfuseStartAddress);
    DBGLOG(INIT, STATE,
           "nicCmdEventQueryNicEfuseAddr: u4EfuseEndAddress = %x\n",
           prAdapter->u4EfuseEndAddress);

    return WLAN_STATUS_SUCCESS;
}

WLAN_STATUS nicCmdEventQueryEfuseOffset(IN P_ADAPTER_T prAdapter,
                                        IN u8 *pucEventBuf,
                                        IN u32 u4EventBufLen)
{
    struct _NIC_EFUSE_OFFSET_T *prEfuseOffset =
        (struct _NIC_EFUSE_OFFSET_T *)pucEventBuf;
    if (u4EventBufLen < sizeof(struct _NIC_EFUSE_OFFSET_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(struct _NIC_EFUSE_OFFSET_T));
        return WLAN_STATUS_FAILURE;
    }
    if (prEfuseOffset->u4TotalItem > 0) {
        prAdapter->u4EfuseMacAddrOffset = prEfuseOffset->u4WlanMacAddr;
    }

    return WLAN_STATUS_SUCCESS;
}

WLAN_STATUS nicCmdEventQueryNicTxResource(IN P_ADAPTER_T prAdapter,
                                          IN u8 *pucEventBuf,
                                          IN u32 u4EventBufLen)
{
    P_NIC_TX_RESOURCE_T prTxResource = (P_NIC_TX_RESOURCE_T)pucEventBuf;
    if (u4EventBufLen < sizeof(NIC_TX_RESOURCE_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(NIC_TX_RESOURCE_T));
        return WLAN_STATUS_FAILURE;
    }
    prAdapter->fgIsNicTxReousrceValid = true;
    prAdapter->nicTxReousrce.u4McuTotalResource =
        prTxResource->u4McuTotalResource;
    prAdapter->nicTxReousrce.u4McuResourceUnit =
        prTxResource->u4McuResourceUnit;
    prAdapter->nicTxReousrce.u4LmacTotalResource =
        prTxResource->u4LmacTotalResource;
    prAdapter->nicTxReousrce.u4LmacResourceUnit =
        prTxResource->u4LmacResourceUnit;

    DBGLOG(INIT, INFO,
           "nicCmdEventQueryNicTxResource: u4McuTotalResource = %x\n",
           prAdapter->nicTxReousrce.u4McuTotalResource);
    DBGLOG(INIT, INFO,
           "nicCmdEventQueryNicTxResource: u4McuResourceUnit = %x\n",
           prAdapter->nicTxReousrce.u4McuResourceUnit);
    DBGLOG(INIT, INFO,
           "nicCmdEventQueryNicTxResource: u4LmacTotalResource = %x\n",
           prAdapter->nicTxReousrce.u4LmacTotalResource);
    DBGLOG(INIT, INFO,
           "nicCmdEventQueryNicTxResource: u4LmacResourceUnit = %x\n",
           prAdapter->nicTxReousrce.u4LmacResourceUnit);

    return WLAN_STATUS_SUCCESS;
}

void nicCmdEventQueryNicCapabilityV2(IN P_ADAPTER_T prAdapter,
                                     IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    P_EVENT_NIC_CAPABILITY_V2_T prEventNicV2 =
        (P_EVENT_NIC_CAPABILITY_V2_T)pucEventBuf;
    P_NIC_CAPABILITY_V2_ELEMENT prElement;
    u32 tag_idx, table_idx, offset;

    // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
    if (prAdapter->fgTestMode == false &&
        u4EventBufLen < sizeof(EVENT_NIC_CAPABILITY_V2_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_NIC_CAPABILITY_V2_T));
        return;
    }

    offset = 0;

    /* process each element */
    for (tag_idx = 0; tag_idx < prEventNicV2->u2TotalElementNum; tag_idx++) {
        prElement =
            (P_NIC_CAPABILITY_V2_ELEMENT)(prEventNicV2->aucBuffer + offset);
        if (u4EventBufLen < offset + sizeof(NIC_CAPABILITY_V2_ELEMENT)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d (offset)\n",
                   __func__, u4EventBufLen, offset +
                   sizeof(NIC_CAPABILITY_V2_ELEMENT));
            return;
        }

        for (table_idx = 0; table_idx < (sizeof(gNicCapabilityV2InfoTable) /
                                         sizeof(NIC_CAPABILITY_V2_REF_TABLE_T));
             table_idx++) {
            /* find the corresponding tag's handler */
            if (gNicCapabilityV2InfoTable[table_idx].tag_type ==
                prElement->tag_type) {
                gNicCapabilityV2InfoTable[table_idx].hdlr(
                    prAdapter, prElement->aucbody, u4EventBufLen - offset);
                break;
            }
        }

        /* move to the next tag */
        offset += prElement->body_len + sizeof(NIC_CAPABILITY_V2_ELEMENT);
    }
}

void nicEventLinkQuality(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                         IN u32 u4EventBufLen)
{
    P_CMD_INFO_T prCmdInfo;

#if CFG_ENABLE_WIFI_DIRECT && CFG_SUPPORT_P2P_RSSI_QUERY
    if (prEvent->u2PacketLen ==
        EVENT_HDR_WITHOUT_RXD_SIZE + sizeof(EVENT_LINK_QUALITY_EX)) {
        P_EVENT_LINK_QUALITY_EX prLqEx =
            (P_EVENT_LINK_QUALITY_EX)(prEvent->aucBuffer);
        if (u4EventBufLen < sizeof(EVENT_LINK_QUALITY_EX)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_LINK_QUALITY_EX));
            return;
        }
        if (prLqEx->ucIsLQ0Rdy) {
            nicUpdateLinkQuality(prAdapter, 0, (P_EVENT_LINK_QUALITY)prLqEx);
        }
        if (prLqEx->ucIsLQ1Rdy) {
            nicUpdateLinkQuality(prAdapter, 1, (P_EVENT_LINK_QUALITY)prLqEx);
        }
    } else {
        if (u4EventBufLen < sizeof(EVENT_LINK_QUALITY)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_LINK_QUALITY));
            return;
        }
        /* For old FW, P2P may invoke link quality query, and make
         * driver flag becone true. */
        DBGLOG(P2P, WARN, "Old FW version, not support P2P RSSI query.\n");

        /* Must not use NETWORK_TYPE_P2P_INDEX, cause the structure is
         * mismatch. */
        nicUpdateLinkQuality(prAdapter, 0,
                             (P_EVENT_LINK_QUALITY)(prEvent->aucBuffer));
    }
#else
    /*only support ais query */
    {
        u8 ucBssIndex;
        P_BSS_INFO_T prBssInfo;
        if (u4EventBufLen < sizeof(EVENT_LINK_QUALITY_V2)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_LINK_QUALITY_V2));
            return;
        }
        for (ucBssIndex = 0; ucBssIndex < BSS_INFO_NUM; ucBssIndex++) {
            prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

            if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS &&
                prBssInfo->fgIsInUse) {
                break;
            }
        }

        if (ucBssIndex >= BSS_INFO_NUM) {
            ucBssIndex = 1;  /* No hit(bss1 for default ais network)
                              */
        }
        nicUpdateLinkQuality(prAdapter, ucBssIndex,
                             (P_EVENT_LINK_QUALITY_V2)(prEvent->aucBuffer));
    }
#endif

    /* command response handling */
    prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

    if (prCmdInfo != NULL) {
        if (prCmdInfo->pfCmdDoneHandler) {
            prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
                                        prEvent->aucBuffer, u4EventBufLen);
        } else if (prCmdInfo->fgIsOid) {
            kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0,
                           WLAN_STATUS_SUCCESS);
        }
        /* return prCmdInfo */
        cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
    }
}

void nicEventLayer0ExtMagic(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                            IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_CMD_INFO_T prCmdInfo;
    P_EVENT_ACCESS_EFUSE prEventEfuseAccess;
    P_EVENT_EFUSE_FREE_BLOCK_T prEventGetFreeBlock;
    P_EVENT_GET_TX_POWER_T prEventGetTXPower;

    if ((prEvent->ucExtenEID) == EXT_EVENT_ID_CMD_RESULT) {
        u4QueryInfoLen = sizeof(PARAM_CUSTOM_EFUSE_BUFFER_MODE_T);

        prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

        if (prCmdInfo != NULL) {
            if ((prCmdInfo->fgIsOid) != 0) {
                kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
                               u4QueryInfoLen, WLAN_STATUS_SUCCESS);
                /* return prCmdInfo */
                cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
            }
        }
    } else if ((prEvent->ucExtenEID) == EXT_EVENT_ID_CMD_EFUSE_ACCESS) {
        if (u4EventBufLen < sizeof(EVENT_ACCESS_EFUSE)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_ACCESS_EFUSE));
            return;
        }
        u4QueryInfoLen = sizeof(PARAM_CUSTOM_ACCESS_EFUSE_T);
        prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);
        prEventEfuseAccess = (P_EVENT_ACCESS_EFUSE)(prEvent->aucBuffer);

        /* Efuse block size 16 */
        kalMemCopy(prAdapter->aucEepromVaule, prEventEfuseAccess->aucData, 16);

        if (prCmdInfo != NULL) {
            if ((prCmdInfo->fgIsOid) != 0) {
                kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
                               u4QueryInfoLen, WLAN_STATUS_SUCCESS);
                /* return prCmdInfo */
                cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
            }
        }
    } else if ((prEvent->ucExtenEID) == EXT_EVENT_ID_EFUSE_FREE_BLOCK) {
        if (u4EventBufLen < sizeof(EVENT_EFUSE_FREE_BLOCK_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_EFUSE_FREE_BLOCK_T));
            return;
        }
        u4QueryInfoLen = sizeof(PARAM_CUSTOM_EFUSE_FREE_BLOCK_T);
        prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);
        prEventGetFreeBlock = (P_EVENT_EFUSE_FREE_BLOCK_T)(prEvent->aucBuffer);
        prAdapter->u4FreeBlockNum = prEventGetFreeBlock->u2FreeBlockNum;

        if (prCmdInfo != NULL) {
            if ((prCmdInfo->fgIsOid) != 0) {
                kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
                               u4QueryInfoLen, WLAN_STATUS_SUCCESS);
                /* return prCmdInfo */
                cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
            }
        }
    } else if ((prEvent->ucExtenEID) == EXT_EVENT_ID_GET_TX_POWER) {
        if (u4EventBufLen < sizeof(EVENT_GET_TX_POWER_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_GET_TX_POWER_T));
            return;
        }
        u4QueryInfoLen = sizeof(PARAM_CUSTOM_GET_TX_POWER_T);
        prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);
        prEventGetTXPower = (P_EVENT_GET_TX_POWER_T)(prEvent->aucBuffer);

        prAdapter->u4GetTxPower = prEventGetTXPower->ucTx0TargetPower;

        if (prCmdInfo != NULL) {
            if ((prCmdInfo->fgIsOid) != 0) {
                kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
                               u4QueryInfoLen, WLAN_STATUS_SUCCESS);
                /* return prCmdInfo */
                cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
            }
        }
    }
}

void nicEventMicErrorInfo(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen)
{
    P_EVENT_MIC_ERR_INFO prMicError;
    /* P_PARAM_AUTH_EVENT_T prAuthEvent; */
    P_STA_RECORD_T prStaRec;

    DBGLOG(RSN, EVENT, "EVENT_ID_MIC_ERR_INFO\n");
    if (u4EventBufLen < sizeof(EVENT_MIC_ERR_INFO)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_MIC_ERR_INFO));
        return;
    }
    prMicError = (P_EVENT_MIC_ERR_INFO)(prEvent->aucBuffer);
    prStaRec =
        cnmGetStaRecByAddress(prAdapter, prAdapter->prAisBssInfo->ucBssIndex,
                              prAdapter->rWlanInfo.rCurrBssId.arMacAddress);
    ASSERT(prStaRec);

    if (prStaRec) {
        rsnTkipHandleMICFailure(prAdapter, prStaRec, (u8)prMicError->u4Flags);
    } else {
        DBGLOG(RSN, INFO, "No STA rec!!\n");
    }
}

void nicEventScanDone(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                      IN u32 u4EventBufLen)
{
    if (u4EventBufLen < sizeof(EVENT_SCAN_DONE)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_SCAN_DONE));
        return;
    }
    scnEventScanDone(prAdapter, (P_EVENT_SCAN_DONE)(prEvent->aucBuffer), true);
}

void nicEventNloDone(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                     IN u32 u4EventBufLen)
{
    DBGLOG(INIT, INFO, "EVENT_ID_NLO_DONE\n");
    if (u4EventBufLen < sizeof(EVENT_NLO_DONE_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_NLO_DONE_T));
        return;
    }
    scnEventNloDone(prAdapter, (P_EVENT_NLO_DONE_T)(prEvent->aucBuffer));
#if CFG_SUPPORT_PNO
    prAdapter->prAisBssInfo->fgIsPNOEnable = false;
    if (prAdapter->prAisBssInfo->fgIsNetRequestInActive &&
        prAdapter->prAisBssInfo->fgIsPNOEnable) {
        UNSET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
        DBGLOG(INIT, INFO, "INACTIVE  AIS from  ACTIVEto disable PNO\n");
        /* sync with firmware */
        nicDeactivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
    }
#endif
}

void nicEventSleepyNotify(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen)
{
    P_EVENT_SLEEPY_INFO_T prEventSleepyNotify;
    if (u4EventBufLen < sizeof(EVENT_SLEEPY_INFO_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_SLEEPY_INFO_T));
        return;
    }
    prEventSleepyNotify = (P_EVENT_SLEEPY_INFO_T)(prEvent->aucBuffer);
    prAdapter->fgWiFiInSleepyState = (u8)(prEventSleepyNotify->ucSleepyState);

    if (prEventSleepyNotify->ucSleepyState) {
        kalSetFwOwnEvent2Hif(prAdapter->prGlueInfo);
    }
}

void nicEventStatistics(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                        IN u32 u4EventBufLen)
{
    P_CMD_INFO_T prCmdInfo;

    /* buffer statistics for further query */
    prAdapter->fgIsStatValid = true;
    prAdapter->rStatUpdateTime = kalGetTimeTick();
    if (u4EventBufLen < sizeof(EVENT_STATISTICS)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_STATISTICS));
        return;
    }
    kalMemCopy(&prAdapter->rStatStruct, prEvent->aucBuffer,
               sizeof(EVENT_STATISTICS));

    /* command response handling */
    prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

    if (prCmdInfo != NULL) {
        if (prCmdInfo->pfCmdDoneHandler) {
            prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
                                        prEvent->aucBuffer, u4EventBufLen);
        } else if (prCmdInfo->fgIsOid) {
            kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0,
                           WLAN_STATUS_SUCCESS);
        }
        /* return prCmdInfo */
        cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
    }
}
void nicEventWlanInfo(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                      IN u32 u4EventBufLen)
{
    P_CMD_INFO_T prCmdInfo;

    /* buffer statistics for further query */
    prAdapter->fgIsStatValid = true;
    prAdapter->rStatUpdateTime = kalGetTimeTick();
    if (u4EventBufLen < sizeof(EVENT_WLAN_INFO)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_WLAN_INFO));
        return;
    }
    kalMemCopy(&prAdapter->rEventWlanInfo, prEvent->aucBuffer,
               sizeof(EVENT_WLAN_INFO));

    DBGLOG(RSN, INFO, "EVENT_ID_WLAN_INFO");
    /* command response handling */
    prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

    if (prCmdInfo != NULL) {
        if (prCmdInfo->pfCmdDoneHandler) {
            prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
                                        prEvent->aucBuffer, u4EventBufLen);
        } else if (prCmdInfo->fgIsOid) {
            kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0,
                           WLAN_STATUS_SUCCESS);
        }
        /* return prCmdInfo */
        cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
    }
}

void nicEventMibInfo(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                     IN u32 u4EventBufLen)
{
    P_CMD_INFO_T prCmdInfo;

    /* buffer statistics for further query */
    prAdapter->fgIsStatValid = true;
    prAdapter->rStatUpdateTime = kalGetTimeTick();

    DBGLOG(RSN, INFO, "EVENT_ID_MIB_INFO");
    /* command response handling */
    prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

    if (prCmdInfo != NULL) {
        if (prCmdInfo->pfCmdDoneHandler) {
            prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
                                        prEvent->aucBuffer, u4EventBufLen);
        } else if (prCmdInfo->fgIsOid) {
            kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0,
                           WLAN_STATUS_SUCCESS);
        }
        /* return prCmdInfo */
        cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
    }
}

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
void nicEventTxMcsInfo(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                       IN u32 u4EventBufLen)
{
    P_CMD_INFO_T prCmdInfo;

    DBGLOG(RSN, INFO, "EVENT_ID_TX_MCS_INFO");
    /* command response handling */
    prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

    if (prCmdInfo != NULL) {
        if (prCmdInfo->pfCmdDoneHandler) {
            prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
                                        prEvent->aucBuffer, u4EventBufLen);
        } else if (prCmdInfo->fgIsOid) {
            kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0,
                           WLAN_STATUS_SUCCESS);
        }
        /* return prCmdInfo */
        cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
    }
}
#endif

void nicEventBeaconTimeout(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                           IN u32 u4EventBufLen)
{
    DBGLOG(NIC, INFO, "EVENT_ID_BSS_BEACON_TIMEOUT\n");

    if (prAdapter->fgDisBcnLostDetection == false) {
        P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T)NULL;
        P_EVENT_BSS_BEACON_TIMEOUT_T prEventBssBeaconTimeout;
        if (u4EventBufLen < sizeof(EVENT_BSS_BEACON_TIMEOUT_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_BSS_BEACON_TIMEOUT_T));
            return;
        }
        prEventBssBeaconTimeout =
            (P_EVENT_BSS_BEACON_TIMEOUT_T)(prEvent->aucBuffer);

        if (prEventBssBeaconTimeout->ucBssIndex >= MAX_BSS_INDEX) {
            return;
        }

        DBGLOG(NIC, INFO, "Reason code: %d\n",
               prEventBssBeaconTimeout->ucReasonCode);

        if (prEventBssBeaconTimeout->ucBssIndex > MAX_BSS_INDEX) {
            DBGLOG(SW4, ERROR, "Invalid BssInfo index[%u], skip dump!\n",
                   prEventBssBeaconTimeout->ucBssIndex);
            return;
        }

        prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
                                          prEventBssBeaconTimeout->ucBssIndex);

        prBssInfo->u2DeauthReason = prEventBssBeaconTimeout->ucReasonCode;

        if ((prAdapter->prAisBssInfo != NULL) &&
            (prEventBssBeaconTimeout->ucBssIndex ==
             prAdapter->prAisBssInfo->ucBssIndex)) {
            if (!timerPendingTimer(
                    &prAdapter->rWifiVar.rAisFsmInfo.rBeaconLostTimer)) {
                cnmTimerStartTimer(
                    prAdapter,
                    &prAdapter->rWifiVar.rAisFsmInfo.rBeaconLostTimer,
                    prAdapter->rWifiVar.ucWaitConnect * MSEC_PER_SEC);
            }

            kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
                                         WLAN_STATUS_BEACON_TIMEOUT, NULL, 0);
        }
#if CFG_ENABLE_WIFI_DIRECT
        else if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
            p2pRoleFsmRunEventBeaconTimeout(prAdapter, prBssInfo);
        }
#endif

        else {
            DBGLOG(RX, ERROR,
                   "EVENT_ID_BSS_BEACON_TIMEOUT: (ucBssIndex = %d)\n",
                   prEventBssBeaconTimeout->ucBssIndex);
        }
    }
}

void nicEventUpdateNoaParams(IN P_ADAPTER_T prAdapter,
                             IN P_WIFI_EVENT_T prEvent, IN u32 u4EventBufLen)
{
#if CFG_ENABLE_WIFI_DIRECT
    if (prAdapter->fgIsP2PRegistered) {
        P_EVENT_UPDATE_NOA_PARAMS_T prEventUpdateNoaParam;
        if (u4EventBufLen < sizeof(EVENT_UPDATE_NOA_PARAMS_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_UPDATE_NOA_PARAMS_T));
            return;
        }
        prEventUpdateNoaParam =
            (P_EVENT_UPDATE_NOA_PARAMS_T)(prEvent->aucBuffer);
        if (prEventUpdateNoaParam->ucBssIndex > MAX_BSS_INDEX) {
            DBGLOG(NIC, ERROR,
                   "nicEventUpdateNoaParams: (ucBssIndex = %d) out-of-bound\n",
                   prEventUpdateNoaParam->ucBssIndex);
            return;
        }
        if (GET_BSS_INFO_BY_INDEX(prAdapter, prEventUpdateNoaParam->ucBssIndex)
            ->eNetworkType == NETWORK_TYPE_P2P) {
            p2pProcessEvent_UpdateNOAParam(prAdapter,
                                           prEventUpdateNoaParam->ucBssIndex,
                                           prEventUpdateNoaParam);
        } else {
            ASSERT(0);
        }
    }
#else
    ASSERT(0);
#endif
}

void nicEventStaAgingTimeout(IN P_ADAPTER_T prAdapter,
                             IN P_WIFI_EVENT_T prEvent, IN u32 u4EventBufLen)
{
    if (prAdapter->fgDisStaAgingTimeoutDetection == false) {
        P_EVENT_STA_AGING_TIMEOUT_T prEventStaAgingTimeout;
        P_STA_RECORD_T prStaRec;
        P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T)NULL;
        if (u4EventBufLen < sizeof(EVENT_STA_AGING_TIMEOUT_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_STA_AGING_TIMEOUT_T));
            return;
        }
        prEventStaAgingTimeout =
            (P_EVENT_STA_AGING_TIMEOUT_T)(prEvent->aucBuffer);
        prStaRec =
            cnmGetStaRecByIndex(prAdapter, prEventStaAgingTimeout->ucStaRecIdx);
        if (prStaRec == NULL) {
            return;
        }

        DBGLOG(NIC, INFO, "EVENT_ID_STA_AGING_TIMEOUT %u " MACSTR "\n",
               prEventStaAgingTimeout->ucStaRecIdx,
               MAC2STR(prStaRec->aucMacAddr));

        if (prStaRec->ucBssIndex > MAX_BSS_INDEX) {
            DBGLOG(SW4, ERROR, "Invalid BssInfo index[%u], skip dump!\n",
                   prStaRec->ucBssIndex);
            return;
        }

        prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

        bssRemoveClient(prAdapter, prBssInfo, prStaRec);

        /* Call False Auth */
        if (prAdapter->fgIsP2PRegistered) {
            p2pFuncDisconnect(prAdapter, prBssInfo, prStaRec, true,
                              REASON_CODE_DISASSOC_INACTIVITY, true);
        }
    }
    /* gDisStaAgingTimeoutDetection */
}

void nicEventApObssStatus(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen)
{
#if CFG_ENABLE_WIFI_DIRECT
    if (prAdapter->fgIsP2PRegistered) {
        if (u4EventBufLen < sizeof(EVENT_AP_OBSS_STATUS_T)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(EVENT_AP_OBSS_STATUS_T));
            return;
        }
        rlmHandleObssStatusEventPkt(
            prAdapter, (P_EVENT_AP_OBSS_STATUS_T)prEvent->aucBuffer);
    }
#endif
}

void nicEventRoamingStatus(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                           IN u32 u4EventBufLen)
{
#if CFG_SUPPORT_ROAMING
    P_CMD_ROAMING_TRANSIT_T prTransit;
    if (u4EventBufLen < sizeof(CMD_ROAMING_TRANSIT_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(CMD_ROAMING_TRANSIT_T));
        return;
    }
    prTransit = (P_CMD_ROAMING_TRANSIT_T)(prEvent->aucBuffer);

    roamingFsmProcessEvent(prAdapter, prTransit);
#endif
}

void nicEventSendDeauth(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                        IN u32 u4EventBufLen)
{
    SW_RFB_T rSwRfb;

#if DBG
    P_WLAN_MAC_HEADER_T prWlanMacHeader;
#endif
    if (u4EventBufLen < sizeof(WLAN_MAC_HEADER_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(WLAN_MAC_HEADER_T));
        return;
    }
#if DBG
    prWlanMacHeader = (P_WLAN_MAC_HEADER_T)&prEvent->aucBuffer[0];
    DBGLOG(RX, TRACE, "nicRx: aucAddr1: " MACSTR "\n",
           MAC2STR(prWlanMacHeader->aucAddr1));
    DBGLOG(RX, TRACE, "nicRx: aucAddr2: " MACSTR "\n",
           MAC2STR(prWlanMacHeader->aucAddr2));
#endif

    /* receive packets without StaRec */
    rSwRfb.pvHeader = (P_WLAN_MAC_HEADER_T)&prEvent->aucBuffer[0];
    if (authSendDeauthFrame(prAdapter, NULL, NULL, &rSwRfb,
                            REASON_CODE_CLASS_3_ERR,
                            (PFN_TX_DONE_HANDLER)NULL) == WLAN_STATUS_SUCCESS) {
        DBGLOG(RX, ERROR, "Send Deauth Class Error by FW Event\n");
    }
}

void nicEventUpdateRddStatus(IN P_ADAPTER_T prAdapter,
                             IN P_WIFI_EVENT_T prEvent, IN u32 u4EventBufLen)
{
#if CFG_SUPPORT_RDD_TEST_MODE
    P_EVENT_RDD_STATUS_T prEventRddStatus;
    if (u4EventBufLen < sizeof(EVENT_RDD_STATUS_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_RDD_STATUS_T));
        return;
    }
    prEventRddStatus = (P_EVENT_RDD_STATUS_T)(prEvent->aucBuffer);

    prAdapter->ucRddStatus = prEventRddStatus->ucRddStatus;
#endif
}

void nicEventUpdateBwcsStatus(IN P_ADAPTER_T prAdapter,
                              IN P_WIFI_EVENT_T prEvent, IN u32 u4EventBufLen)
{
    P_PTA_IPC_T prEventBwcsStatus;
    if (u4EventBufLen < sizeof(PTA_IPC_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(PTA_IPC_T));
        return;
    }
    prEventBwcsStatus = (P_PTA_IPC_T)(prEvent->aucBuffer);

#if CFG_SUPPORT_BCM_BWCS_DEBUG
    DBGLOG(RSN, EVENT, "BCM BWCS Event: %02x%02x%02x%02x\n",
           prEventBwcsStatus->u.aucBTPParams[0],
           prEventBwcsStatus->u.aucBTPParams[1],
           prEventBwcsStatus->u.aucBTPParams[2],
           prEventBwcsStatus->u.aucBTPParams[3]);
#endif

    kalIndicateStatusAndComplete(prAdapter->prGlueInfo, WLAN_STATUS_BWCS_UPDATE,
                                 (void *)prEventBwcsStatus, sizeof(PTA_IPC_T));
}

void nicEventUpdateBcmDebug(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                            IN u32 u4EventBufLen)
{
    P_PTA_IPC_T prEventBwcsStatus;
    if (u4EventBufLen < sizeof(PTA_IPC_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(PTA_IPC_T));
        return;
    }
    prEventBwcsStatus = (P_PTA_IPC_T)(prEvent->aucBuffer);

#if CFG_SUPPORT_BCM_BWCS_DEBUG
    DBGLOG(RSN, EVENT, "BCM FW status: %02x%02x%02x%02x\n",
           prEventBwcsStatus->u.aucBTPParams[0],
           prEventBwcsStatus->u.aucBTPParams[1],
           prEventBwcsStatus->u.aucBTPParams[2],
           prEventBwcsStatus->u.aucBTPParams[3]);
#endif
}

void nicEventAddPkeyDone(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                         IN u32 u4EventBufLen)
{
    P_EVENT_ADD_KEY_DONE_INFO prAddKeyDone;
    P_STA_RECORD_T prStaRec;
    if (u4EventBufLen < sizeof(EVENT_ADD_KEY_DONE_INFO)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_ADD_KEY_DONE_INFO));
        return;
    }
    prAddKeyDone = (P_EVENT_ADD_KEY_DONE_INFO)(prEvent->aucBuffer);

    DBGLOG(RSN, INFO, "EVENT_ID_ADD_PKEY_DONE BSSIDX=%d " MACSTR "\n",
           prAddKeyDone->ucBSSIndex, MAC2STR(prAddKeyDone->aucStaAddr));

    prStaRec = cnmGetStaRecByAddress(prAdapter, prAddKeyDone->ucBSSIndex,
                                     prAddKeyDone->aucStaAddr);

    if (prStaRec) {
        DBGLOG(RSN, INFO, "STA " MACSTR " Add Key Done!!\n",
               MAC2STR(prStaRec->aucMacAddr));
        prStaRec->fgIsTxKeyReady = true;
        qmUpdateStaRec(prAdapter, prStaRec);
    }
}

void nicEventIcapDone(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                      IN u32 u4EventBufLen)
{
    P_EVENT_ICAP_STATUS_T prEventIcapStatus;
    PARAM_CUSTOM_MEM_DUMP_STRUCT_T rMemDumpInfo;
    u32 u4QueryInfo;
    if (u4EventBufLen < sizeof(EVENT_ICAP_STATUS_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_ICAP_STATUS_T));
        return;
    }
    prEventIcapStatus = (P_EVENT_ICAP_STATUS_T)(prEvent->aucBuffer);

    rMemDumpInfo.u4Address = prEventIcapStatus->u4StartAddress;
    rMemDumpInfo.u4Length = prEventIcapStatus->u4IcapSieze;
#if CFG_SUPPORT_QA_TOOL
    rMemDumpInfo.u4IcapContent = prEventIcapStatus->u4IcapContent;
#endif

    wlanoidQueryMemDump(prAdapter, &rMemDumpInfo, sizeof(rMemDumpInfo),
                        &u4QueryInfo);
}

void nicEventDebugMsg(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                      IN u32 u4EventBufLen)
{
    P_EVENT_DEBUG_MSG_T prEventDebugMsg;
    u16 u2DebugMsgId;
    u8 ucMsgType;
    u8 ucFlags;
    u32 u4Value;
    u16 u2MsgSize;
    u8 *pucMsg;

    if (u4EventBufLen < sizeof(EVENT_DEBUG_MSG_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_DEBUG_MSG_T));
        return;
    }
    prEventDebugMsg = (P_EVENT_DEBUG_MSG_T)(prEvent->aucBuffer);

    if (u4EventBufLen <
        /*sizeof(EVENT_DEBUG_MSG_T) +*/ prEventDebugMsg->u2MsgSize /*- 1*/) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen,
               sizeof(EVENT_DEBUG_MSG_T) + prEventDebugMsg->u2MsgSize);
        return;
    }
    u2DebugMsgId = prEventDebugMsg->u2DebugMsgId;
    ucMsgType = prEventDebugMsg->ucMsgType;
    ucFlags = prEventDebugMsg->ucFlags;
    u4Value = prEventDebugMsg->u4Value;
    u2MsgSize = prEventDebugMsg->u2MsgSize;
    pucMsg = prEventDebugMsg->aucMsg;

    DBGLOG(SW4, TRACE, "DEBUG_MSG Id %u Type %u Fg 0x%x Val 0x%x Size %u\n",
           u2DebugMsgId, ucMsgType, ucFlags, u4Value, u2MsgSize);

    if (u2MsgSize <= DEBUG_MSG_SIZE_MAX) {
        if (ucMsgType >= DEBUG_MSG_TYPE_END) {
            ucMsgType = DEBUG_MSG_TYPE_MEM32;
        }

        if (ucMsgType == DEBUG_MSG_TYPE_ASCII) {
            u8 *pucChr;

            pucMsg[u2MsgSize] = '\0';

            /* skip newline */
            pucChr = kalStrChr(pucMsg, '\0');
            if (*(pucChr - 1) == '\n') {
                *(pucChr - 1) = '\0';
            }

            DBGLOG(SW4, EVENT, "<FW>%s\n", pucMsg);
        } else if (ucMsgType == DEBUG_MSG_TYPE_MEM8) {
            DBGLOG(SW4, INFO, "<FW>Dump MEM8\n");
            DBGLOG_MEM8(SW4, INFO, pucMsg, u2MsgSize);
        } else {
            DBGLOG(SW4, INFO, "<FW>Dump MEM32\n");
            DBGLOG_MEM32(SW4, INFO, pucMsg, u2MsgSize);
        }
    } else {
        DBGLOG(SW4, INFO, "Debug msg size %u is too large.\n", u2MsgSize);
    }
}

void nicEventTdls(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                  IN u32 u4EventBufLen)
{
#if CFG_SUPPORT_TDLS
    /* [TDLS  Event Format]
     * FW:   FW_EVT_HDR   + 8 + evt_payload
     * Host: HOST_EVT_HDR + 8 + evt_payload
     *       EVENT_HDR_WITHOUT_RXD_SIZE = size(FW_EVT_HDR)
     *       prEvent->u2PacketLength    = size(FW_EVT_HDR) + 8 +
     * size(evt_payload) HOST_EVT_HDR               = HDR_RXD
     * (au4HwMacRxDesc) + size(FW_EVT_HDR) u4EventBufLen              =
     * should be  8+size(evt_payload) 8             = EVT_ID(4) + SUB_ID(4)
     */
    if (u4EventBufLen < prEvent->u2PacketLength - EVENT_HDR_WITHOUT_RXD_SIZE) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, prEvent->u2PacketLength);
        return;
    }
    if (prEvent->u2PacketLength - EVENT_HDR_WITHOUT_RXD_SIZE < 8) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: (%d < 8)\n", __func__,
               prEvent->u2PacketLength - EVENT_HDR_WITHOUT_RXD_SIZE);
        return;
    }
    TdlsexEventHandle(prAdapter->prGlueInfo, (u8 *)prEvent->aucBuffer,
                      (u32)(prEvent->u2PacketLength - 8));
#endif
}

void nicEventDumpMem(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                     IN u32 u4EventBufLen)
{
    P_CMD_INFO_T prCmdInfo;

    DBGLOG(SW4, INFO, "%s: EVENT_ID_DUMP_MEM\n", __func__);

    prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

    if (prCmdInfo != NULL) {
        DBGLOG(NIC, INFO, ": ==> 1\n");
        if (prCmdInfo->pfCmdDoneHandler) {
            prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
                                        prEvent->aucBuffer, u4EventBufLen);
        } else if (prCmdInfo->fgIsOid) {
            kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0,
                           WLAN_STATUS_SUCCESS);
        }
        /* return prCmdInfo */
        cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
    } else {
        /* Burst mode */
        DBGLOG(NIC, INFO, ": ==> 2\n");
        nicEventQueryMemDump(prAdapter, prEvent->aucBuffer, u4EventBufLen);
    }
}

void nicEventAssertDump(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                        IN u32 u4EventBufLen)
{
#if CFG_ASSERT_DUMP
    if (wlanIsChipRstRecEnabled(prAdapter)) {
        wlanChipRstPreAct(prAdapter);
    }

    if (prEvent->ucS2DIndex == S2D_INDEX_EVENT_N2H) {
        if (!prAdapter->fgN9AssertDumpOngoing) {
            DBGLOG(NIC, ERROR, "%s: EVENT_ID_ASSERT_DUMP\n", __func__);
            DBGLOG(NIC, ERROR, "\n[DUMP_N9]====N9 ASSERT_DUMPSTART====\n");
            prAdapter->fgKeepPrintCoreDump = true;
            if (kalOpenCorDumpFile(true) != WLAN_STATUS_SUCCESS) {
                DBGLOG(NIC, ERROR, "kalOpenCorDumpFile fail\n");
            } else {
                prAdapter->fgN9CorDumpFileOpend = true;
            }

            prAdapter->fgN9AssertDumpOngoing = true;
        }
        if (prAdapter->fgN9AssertDumpOngoing) {
            if (u4EventBufLen < 5 ||
                u4EventBufLen <
                prEvent->u2PacketLength - EVENT_HDR_WITHOUT_RXD_SIZE) {
                DBGLOG(NIC, ERROR,
                       "%s: Invalid event length: %d < %d (or < 5?)\n",
                       __func__, u4EventBufLen,
                       prEvent->u2PacketLength - EVENT_HDR_WITHOUT_RXD_SIZE);
                return;
            }
            if (prAdapter->fgKeepPrintCoreDump) {
                DBGLOG(NIC, ERROR, "[DUMP_N9]%s:\n", prEvent->aucBuffer);
            }
            if (!kalStrnCmp(prEvent->aucBuffer, ";more log added here", 5) ||
                !kalStrnCmp(prEvent->aucBuffer, ";[core dump start]", 5)) {
                prAdapter->fgKeepPrintCoreDump = false;
            }

            if (prAdapter->fgN9CorDumpFileOpend) {
                if (kalWriteCorDumpFile(prEvent->aucBuffer,
                                        prEvent->u2PacketLength -
                                        EVENT_HDR_WITHOUT_RXD_SIZE,
                                        true) != WLAN_STATUS_SUCCESS) {
                    DBGLOG(NIC, INFO, "kalWriteN9CorDumpFile fail\n");
                }
            }
            wlanCorDumpTimerReset(prAdapter, true);
        }
    } else {
        /* prEvent->ucS2DIndex == S2D_INDEX_EVENT_C2H */
        if (!prAdapter->fgCr4AssertDumpOngoing) {
            DBGLOG(NIC, ERROR, "%s: EVENT_ID_ASSERT_DUMP\n", __func__);
            DBGLOG(NIC, ERROR, "\n[DUMP_Cr4]====CR4 ASSERT_DUMPSTART====\n");
            prAdapter->fgKeepPrintCoreDump = true;
            if (kalOpenCorDumpFile(false) != WLAN_STATUS_SUCCESS) {
                DBGLOG(NIC, ERROR, "kalOpenCorDumpFile fail\n");
            } else {
                prAdapter->fgCr4CorDumpFileOpend = true;
            }

            prAdapter->fgCr4AssertDumpOngoing = true;
        }
        if (prAdapter->fgCr4AssertDumpOngoing) {
            if (prAdapter->fgKeepPrintCoreDump) {
                DBGLOG(NIC, ERROR, "[DUMP_CR4]%s:\n", prEvent->aucBuffer);
            }
            if (!kalStrnCmp(prEvent->aucBuffer, ";more log added here", 5)) {
                prAdapter->fgKeepPrintCoreDump = false;
            }

            if (prAdapter->fgCr4CorDumpFileOpend) {
                if (kalWriteCorDumpFile(prEvent->aucBuffer,
                                        prEvent->u2PacketLength -
                                        EVENT_HDR_WITHOUT_RXD_SIZE,
                                        false) != WLAN_STATUS_SUCCESS) {
                    DBGLOG(NIC, ERROR, "kalWriteN9CorDumpFile fail\n");
                }
            }
            wlanCorDumpTimerReset(prAdapter, false);
        }
    }
#endif
}

void nicEventRddSendPulse(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen)
{
    DBGLOG(RLM, INFO, "%s: EVENT_ID_RDD_SEND_PULSE\n", __func__);

    nicEventRddPulseDump(prAdapter, prEvent->aucBuffer, u4EventBufLen);
}

void nicEventUpdateCoexPhyrate(IN P_ADAPTER_T prAdapter,
                               IN P_WIFI_EVENT_T prEvent, IN u32 u4EventBufLen)
{
    u8 i;
    P_EVENT_UPDATE_COEX_PHYRATE_T prEventUpdateCoexPhyrate;

    DBGLOG(NIC, LOUD, "%s\n", __func__);
    if (u4EventBufLen < sizeof(EVENT_UPDATE_COEX_PHYRATE_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(EVENT_UPDATE_COEX_PHYRATE_T));
        return;
    }
    prEventUpdateCoexPhyrate =
        (P_EVENT_UPDATE_COEX_PHYRATE_T)(prEvent->aucBuffer);

    for (i = 0; i < (HW_BSSID_NUM + 1); i++) {
        prAdapter->aprBssInfo[i]->u4CoexPhyRateLimit =
            prEventUpdateCoexPhyrate->au4PhyRateLimit[i];
        DBGLOG(NIC, INFO, "Coex:BSS[%d]R:%d\n", i,
               prAdapter->aprBssInfo[i]->u4CoexPhyRateLimit);
    }
}

#if (CFG_WOW_SUPPORT == 1)
void nicEventWakeUpReason(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                          IN u32 u4EventBufLen)
{
    struct _EVENT_WAKEUP_REASON_INFO *prWakeUpReason;
    P_GLUE_INFO_T prGlueInfo;

    DBGLOG(NIC, INFO, "nicEventWakeUpReason\n");
    prGlueInfo = prAdapter->prGlueInfo;
    if (u4EventBufLen < sizeof(struct _EVENT_WAKEUP_REASON_INFO)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(struct _EVENT_WAKEUP_REASON_INFO));
        return;
    }
    /* Driver receives EVENT_ID_WOW_WAKEUP_REASON after firmware wake up
     * host The possible Wakeup Reason define in FW as following 0:  MAGIC
     * PACKET 1:  BITMAP 2:  ARPNS 3:  GTK_REKEY 4:  COALESCING_FILTER 5:
     * HW_GLOBAL_ENABLE 6:  TCP_SYN PACKET 7:  TDLS 8:  DISCONNECT 9:
     * IPV4_UDP PACKET 10: IPV4_TCP PACKET 11: IPV6_UDP PACKET 12: IPV6_TCP
     * PACKET
     */
    prWakeUpReason = (struct _EVENT_WAKEUP_REASON_INFO *)(prEvent->aucBuffer);
    prGlueInfo->prAdapter->rWowCtrl.ucReason = prWakeUpReason->reason;
    DBGLOG(NIC, INFO, "nicEventWakeUpReason:%d\n",
           prGlueInfo->prAdapter->rWowCtrl.ucReason);
}
#endif

void nicEventCSIData(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                     IN u32 u4EventBufLen)
{
    struct EVENT_CSI_DATA_T *prCsiData;

    DBGLOG(NIC, INFO, "nicEventCSIData\n");

    if (prAdapter->rCsiData.ucDataOutputted != 0) {
        DBGLOG(NIC, INFO,
               "Previous %s data is not outputted. Ignore this new data!\n",
               (prAdapter->rCsiData.ucDataOutputted & BIT(0)) ? "Q" : "I");
        return;
    }
    if (u4EventBufLen < sizeof(struct EVENT_CSI_DATA_T)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(struct EVENT_CSI_DATA_T));
        return;
    }
    prCsiData = (struct EVENT_CSI_DATA_T *)(prEvent->aucBuffer);
    prAdapter->rCsiData.ucDbdcIdx = prCsiData->ucDbdcIdx;
    prAdapter->rCsiData.ucBw = prCsiData->ucBw;
    prAdapter->rCsiData.bIsCck = prCsiData->bIsCck;
    if (prCsiData->u2DataCount > 256) {
        DBGLOG(NIC, WARN, "%s: u2DataCount (%d) is invalid! Need <= 256.\n",
               __func__, prCsiData->u2DataCount);
        return;
    }
    prAdapter->rCsiData.u2DataCount = prCsiData->u2DataCount;
    kalMemZero(prAdapter->rCsiData.ac2IData,
               sizeof(prAdapter->rCsiData.ac2IData));
    kalMemZero(prAdapter->rCsiData.ac2QData,
               sizeof(prAdapter->rCsiData.ac2QData));
    kalMemCopy(prAdapter->rCsiData.ac2IData, prCsiData->ac2IData,
               sizeof(prCsiData->ac2IData));

    kalMemCopy(prAdapter->rCsiData.ac2QData, prCsiData->ac2QData,
               sizeof(prCsiData->ac2QData));
}

#if CFG_SUPPORT_REPLAY_DETECTION
void nicCmdEventSetAddKey(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                          IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    P_WIFI_CMD_T prWifiCmd = NULL;
    P_CMD_802_11_KEY prCmdKey = NULL;
    struct SEC_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;
    u8 ucBssIndex = 0;
    P_BSS_INFO_T prBssInfo = NULL;

    ASSERT(prAdapter);
    ASSERT(prCmdInfo);

    if (prCmdInfo->fgIsOid) {
        /* Update Set Information Length */
        kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
                       prCmdInfo->u4InformationBufferLength,
                       WLAN_STATUS_SUCCESS);
    }

    prWifiCmd = (P_WIFI_CMD_T)(prCmdInfo->pucInfoBuffer);
    prCmdKey = (P_CMD_802_11_KEY)(prWifiCmd->aucBuffer);
    ucBssIndex = prCmdKey->ucBssIdx;

    if (ucBssIndex > MAX_BSS_INDEX) {
        DBGLOG(SW4, ERROR, "Invalid BssInfo index[%u], skip dump!\n",
               ucBssIndex);
        return;
    }

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
    ASSERT(prBssInfo);

    prDetRplyInfo = &prBssInfo->rDetRplyInfo;

    if (pucEventBuf) {
        // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
        if (prAdapter->fgTestMode == false &&
            u4EventBufLen < sizeof(WIFI_CMD_T) + sizeof(CMD_802_11_KEY)) {
            DBGLOG(NIC, ERROR, "Invalid event length:%d < %d\n", u4EventBufLen,
                   sizeof(WIFI_CMD_T) + sizeof(CMD_802_11_KEY));
            return;
        }
        prWifiCmd = (P_WIFI_CMD_T)(pucEventBuf);
        prCmdKey = (P_CMD_802_11_KEY)(prWifiCmd->aucBuffer);
        if (!prCmdKey->ucKeyType) {
            prDetRplyInfo->ucCurKeyId = prCmdKey->ucKeyId;
            prDetRplyInfo->ucKeyType = prCmdKey->ucKeyType;
            prDetRplyInfo->arReplayPNInfo[prCmdKey->ucKeyId].fgRekey = true;
            prDetRplyInfo->arReplayPNInfo[prCmdKey->ucKeyId].fgFirstPkt = true;
            DBGLOG(NIC, TRACE, "Keyid is %d, ucKeyType is %d\n",
                   prCmdKey->ucKeyId, prCmdKey->ucKeyType);
        }
    }
}

void nicOidCmdTimeoutSetAddKey(IN P_ADAPTER_T prAdapter,
                               IN P_CMD_INFO_T prCmdInfo)
{
    ASSERT(prAdapter);

    DBGLOG(NIC, WARN, "Wlan setaddkey timeout.\n");
    if (prCmdInfo->fgIsOid) {
        kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0,
                       WLAN_STATUS_FAILURE);
    }
}

void nicEventGetGtkDataSync(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent,
                            IN u32 u4EventBufLen)
{
    P_PARAM_GTK_REKEY_DATA prGtkData = NULL;
    struct SEC_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;
    P_BSS_INFO_T prBssInfo = NULL;
    u8 ucCurKeyId;
    if (u4EventBufLen < sizeof(PARAM_GTK_REKEY_DATA)) {
        DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
               u4EventBufLen, sizeof(PARAM_GTK_REKEY_DATA));
        return;
    }
    prGtkData = (P_PARAM_GTK_REKEY_DATA)(prEvent->aucBuffer);

    if (prAdapter->prAisBssInfo->ucBssIndex > MAX_BSS_INDEX) {
        DBGLOG(SW4, ERROR, "Invalid BssInfo index[%u], skip dump!\n",
               prAdapter->prAisBssInfo->ucBssIndex);
        return;
    }

    prBssInfo =
        GET_BSS_INFO_BY_INDEX(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

    prDetRplyInfo = &prBssInfo->rDetRplyInfo;
    prDetRplyInfo->ucCurKeyId = prGtkData->ucCurKeyId;
    ucCurKeyId = prDetRplyInfo->ucCurKeyId;

    if (ucCurKeyId >= 4) {
        DBGLOG(RSN, WARN, "Invalid KeyId of PN: %d, out of bound.\n",
               ucCurKeyId);
        return;
    }
    kalMemZero(prDetRplyInfo->arReplayPNInfo[ucCurKeyId].auPN,
               NL80211_REPLAY_CTR_LEN);
    kalMemCopy(prDetRplyInfo->arReplayPNInfo[ucCurKeyId].auPN,
               prGtkData->aucReplayCtr, 6);

    DBGLOG(RSN, INFO, "Get BC/MC PN update from fw.\n");
    DBGLOG_MEM8(RSN, INFO, (u8 *)prDetRplyInfo->arReplayPNInfo[ucCurKeyId].auPN,
                NL80211_REPLAY_CTR_LEN);
}

#endif

#ifdef CFG_SUPPORT_ANT_DIV
void nicCmdEventAntDiv(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                       IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    P_GLUE_INFO_T prGlueInfo;
    struct CMD_ANT_DIV_CTRL *prAntDivInfo;
    u32 u4QueryInfoLen;

    if (prAdapter == NULL) {
        DBGLOG(RSN, INFO, "prAdapter is null\n");
        return;
    }
    if (prCmdInfo == NULL) {
        DBGLOG(RSN, INFO, "prCmdInfo is null\n");
        return;
    }

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        u4QueryInfoLen = sizeof(struct CMD_ANT_DIV_CTRL);
        prAntDivInfo =
            (struct CMD_ANT_DIV_CTRL *)prCmdInfo->pvInformationBuffer;

        if (pucEventBuf && prAntDivInfo) {
            if (u4EventBufLen < u4QueryInfoLen) {
                DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n",
                       __func__, u4EventBufLen, u4QueryInfoLen);
                return;
            }
            kalMemCopy(prAntDivInfo, pucEventBuf, u4QueryInfoLen);
        }
        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}
#endif

#ifdef CFG_DUMP_TXPOWR_TABLE
void nicCmdEventGetTxPwrTbl(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo,
                            IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    u32 u4QueryInfoLen;
    P_GLUE_INFO_T prGlueInfo;
    struct EVENT_GET_TXPWR_TBL *prTxPwrTblEvent = NULL;
    struct PARAM_CMD_GET_TXPWR_TBL *prTxPwrTbl = NULL;
    void *info_buf = NULL;

    if (!prAdapter) {
        DBGLOG(NIC, ERROR, "NULL prAdapter!\n");
        return;
    }

    if (!prCmdInfo) {
        DBGLOG(NIC, ERROR, "NULL prCmdInfo!\n");
        return;
    }

    if (!pucEventBuf || !prCmdInfo->pvInformationBuffer) {
        if (prCmdInfo->fgIsOid) {
            kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0,
                           WLAN_STATUS_FAILURE);
        }

        if (!pucEventBuf) {
            DBGLOG(NIC, WARN, "NULL pucEventBuf!\n");
        }

        if (!prCmdInfo->pvInformationBuffer) {
            DBGLOG(NIC, WARN, "NULL pvInformationBuffer!\n");
        }
        return;
    }

    if (prCmdInfo->fgIsOid) {
        prGlueInfo = prAdapter->prGlueInfo;
        info_buf = prCmdInfo->pvInformationBuffer;
        // !IS_ALLOWED_CMD_IN_TEST_MODE(), skip check in test mode
        if (prAdapter->fgTestMode == false &&
            u4EventBufLen < sizeof(struct EVENT_GET_TXPWR_TBL)) {
            DBGLOG(NIC, ERROR, "%s: Invalid event length: %d < %d\n", __func__,
                   u4EventBufLen, sizeof(struct EVENT_GET_TXPWR_TBL));
            return;
        }
        prTxPwrTblEvent = (struct EVENT_GET_TXPWR_TBL *)pucEventBuf;
        prTxPwrTbl = (struct PARAM_CMD_GET_TXPWR_TBL *)info_buf;

        u4QueryInfoLen = sizeof(struct PARAM_CMD_GET_TXPWR_TBL);

        prTxPwrTbl->ucCenterCh = prTxPwrTblEvent->ucCenterCh;
        prTxPwrTbl->ucFeLoss = prTxPwrTblEvent->ucFeLoss;

        kalMemCopy(prTxPwrTbl->tx_pwr_tbl, prTxPwrTblEvent->tx_pwr_tbl,
                   sizeof(prTxPwrTblEvent->tx_pwr_tbl));

        kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen,
                       WLAN_STATUS_SUCCESS);
    }
}
#endif
