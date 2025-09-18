// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file gl_wext_priv.c
 *    \brief This file includes private ioctl support.
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
#include "gl_os.h"
#include "gl_wext_priv.h"

#if CFG_SUPPORT_QA_TOOL
#include "gl_ate_agent.h"
#include "gl_qa_agent.h"
#endif

#if CFG_ENABLE_WIFI_DIRECT
#include "gl_p2p_os.h"
#endif

/*
 * #if CFG_SUPPORT_QA_TOOL
 * extern u16 g_u2DumpIndex;
 * #endif
 */

#if (CFG_SUPPORT_DFS_MASTER == 1)
#include "gl_hook_api.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define NUM_SUPPORTED_OIDS    (sizeof(arWlanOidReqTable) / \
                               sizeof(WLAN_REQ_ENTRY))

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static int priv_get_ndis(IN struct net_device *prNetDev,
                         IN NDIS_TRANSPORT_STRUCT *prNdisReq,
                         OUT u32 *pu4OutputLen);

static int priv_set_ndis(IN struct net_device *prNetDev,
                         IN NDIS_TRANSPORT_STRUCT *prNdisReq,
                         OUT u32 *pu4OutputLen);

static u8 reqSearchSupportedOidEntry(IN u32 rOid,
                                     OUT P_WLAN_REQ_ENTRY *ppWlanReqEntry);

static WLAN_STATUS reqExtSetAcpiDevicePowerState(IN P_GLUE_INFO_T prGlueInfo,
                                                 IN void *pvSetBuffer,
                                                 IN u32 u4SetBufferLen,
                                                 OUT u32 *pu4SetInfoLen);

/*******************************************************************************
 *                       P R I V A T E   D A T A
 *******************************************************************************
 */

static u8 aucOidBuf[4096] = { 0 };

/* OID processing table */
/* Order is important here because the OIDs should be in order of
 *  increasing value for binary searching.
 */
static WLAN_REQ_ENTRY arWlanOidReqTable[] = {
    /* Ethernet Operational Characteristics */
    { OID_802_3_CURRENT_ADDRESS, DISP_STRING("OID_802_3_CURRENT_ADDRESS"),
      true, true, ENUM_OID_DRIVER_CORE, 6,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryCurrentAddr, NULL },

    /* OID_802_3_MULTICAST_LIST */
    /* OID_802_3_MAXIMUM_LIST_SIZE */
    /* Ethernet Statistics */

    /* NDIS 802.11 Wireless LAN OIDs */
    { OID_802_11_SUPPORTED_RATES, DISP_STRING("OID_802_11_SUPPORTED_RATES"),
      true, false, ENUM_OID_DRIVER_CORE, sizeof(PARAM_RATES_EX),
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidQuerySupportedRates, NULL },
    /*
     *  {OID_802_11_CONFIGURATION,
     *  DISP_STRING("OID_802_11_CONFIGURATION"),
     *  true, true, ENUM_OID_GLUE_EXTENSION, sizeof(PARAM_802_11_CONFIG_T),
     *  (PFN_OID_HANDLER_FUNC_REQ)reqExtQueryConfiguration,
     *  (PFN_OID_HANDLER_FUNC_REQ)reqExtSetConfiguration},
     */
    { OID_PNP_SET_POWER, DISP_STRING("OID_PNP_SET_POWER"), true, false,
      ENUM_OID_GLUE_EXTENSION, sizeof(PARAM_DEVICE_POWER_STATE), NULL,
      (PFN_OID_HANDLER_FUNC_REQ)reqExtSetAcpiDevicePowerState },

    /* Custom OIDs */
    { OID_CUSTOM_OID_INTERFACE_VERSION,
      DISP_STRING("OID_CUSTOM_OID_INTERFACE_VERSION"), true, false,
      ENUM_OID_DRIVER_CORE, 4,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryOidInterfaceVersion, NULL },

    { OID_CUSTOM_MCR_RW, DISP_STRING("OID_CUSTOM_MCR_RW"), true, true,
      ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_MCR_RW_STRUCT_T),
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryMcrRead,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetMcrWrite },

    { OID_CUSTOM_EEPROM_RW, DISP_STRING("OID_CUSTOM_EEPROM_RW"), true, true,
      ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_EEPROM_RW_STRUCT_T),
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryEepromRead,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetEepromWrite },

    { OID_CUSTOM_SW_CTRL, DISP_STRING("OID_CUSTOM_SW_CTRL"), true, true,
      ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_SW_CTRL_STRUCT_T),
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidQuerySwCtrlRead,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetSwCtrlWrite },

    { OID_CUSTOM_MEM_DUMP, DISP_STRING("OID_CUSTOM_MEM_DUMP"), true, true,
      ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_MEM_DUMP_STRUCT_T),
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryMemDump, NULL },

    { OID_CUSTOM_TEST_MODE, DISP_STRING("OID_CUSTOM_TEST_MODE"), false,
      false, ENUM_OID_DRIVER_CORE, 0, NULL,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidRftestSetTestMode },

    { OID_CUSTOM_ABORT_TEST_MODE, DISP_STRING("OID_CUSTOM_ABORT_TEST_MODE"),
      false, false, ENUM_OID_DRIVER_CORE, 0, NULL,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidRftestSetAbortTestMode },
    { OID_CUSTOM_MTK_WIFI_TEST, DISP_STRING("OID_CUSTOM_MTK_WIFI_TEST"),
      true, true, ENUM_OID_DRIVER_CORE,
      sizeof(PARAM_MTK_WIFI_TEST_STRUCT_T),
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidRftestQueryAutoTest,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidRftestSetAutoTest },
    { OID_CUSTOM_TEST_ICAP_MODE, DISP_STRING("OID_CUSTOM_TEST_ICAP_MODE"),
      false, false, ENUM_OID_DRIVER_CORE, 0, NULL,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidRftestSetTestIcapMode },

/* OID_CUSTOM_EMULATION_VERSION_CONTROL */

/* BWCS */
#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS
    { OID_CUSTOM_BWCS_CMD, DISP_STRING("OID_CUSTOM_BWCS_CMD"), false, false,
      ENUM_OID_DRIVER_CORE, sizeof(PTA_IPC_T),
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryBT,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetBT },
#endif

    { OID_CUSTOM_MTK_NVRAM_RW, DISP_STRING("OID_CUSTOM_MTK_NVRAM_RW"), true,
      true, ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_NVRAM_RW_STRUCT_T),
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryNvramRead,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetNvramWrite },

    { OID_CUSTOM_CFG_SRC_TYPE, DISP_STRING("OID_CUSTOM_CFG_SRC_TYPE"),
      false, false, ENUM_OID_DRIVER_CORE, sizeof(ENUM_CFG_SRC_TYPE_T),
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryCfgSrcType, NULL },

    { OID_CUSTOM_EEPROM_TYPE, DISP_STRING("OID_CUSTOM_EEPROM_TYPE"), false,
      false, ENUM_OID_DRIVER_CORE, sizeof(ENUM_EEPROM_TYPE_T),
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryEepromType, NULL },

#if CFG_SUPPORT_WPS2
    { OID_802_11_WSC_ASSOC_INFO, DISP_STRING("OID_802_11_WSC_ASSOC_INFO"),
      false, false, ENUM_OID_DRIVER_CORE, 0, NULL,
      (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetWSCAssocInfo },
#endif
};

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Dispatching function for private ioctl region (SIOCIWFIRSTPRIV ~
 *   SIOCIWLASTPRIV).
 *
 * \param[in] prNetDev Net device requested.
 * \param[in] prIfReq Pointer to ifreq structure.
 * \param[in] i4Cmd Command ID between SIOCIWFIRSTPRIV and SIOCIWLASTPRIV.
 *
 * \retval 0 for success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EFAULT For fail.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_support_ioctl(IN struct net_device *prNetDev,
                       IN OUT struct ifreq *prIfReq, IN int i4Cmd)
{
    /* prIfReq is verified in the caller function wlanDoIOCTL() */
    struct iwreq *prIwReq = (struct iwreq *)prIfReq;
    struct iw_request_info rIwReqInfo;

    /* prNetDev is verified in the caller function wlanDoIOCTL() */

    /* Prepare the call */
    rIwReqInfo.cmd = (__u16)i4Cmd;
    rIwReqInfo.flags = 0;

    switch (i4Cmd) {
    case IOCTL_SET_INT:
        /* NOTE(Kevin): 1/3 INT Type <= IFNAMSIZ, so we don't need
         * copy_from/to_user() */
        return priv_set_int(prNetDev, &rIwReqInfo, &(prIwReq->u),
                            (char *)&(prIwReq->u));

    case IOCTL_GET_INT:
        /* NOTE(Kevin): 1/3 INT Type <= IFNAMSIZ, so we don't need
         * copy_from/to_user() */
        return priv_get_int(prNetDev, &rIwReqInfo, &(prIwReq->u),
                            (char *)&(prIwReq->u));

    case IOCTL_SET_STRUCT:
        /* case IOCTL_SET_STRUCT_FOR_EM: Engineer Mode not need in FOS
         */
        return priv_set_struct(prNetDev, &rIwReqInfo, &prIwReq->u,
                               (char *)&(prIwReq->u));

    case IOCTL_GET_STRUCT:
        return priv_get_struct(prNetDev, &rIwReqInfo, &prIwReq->u,
                               (char *)&(prIwReq->u));

#if (CFG_SUPPORT_QA_TOOL)
    case IOCTL_QA_TOOL_DAEMON:
        return priv_qa_agent(prNetDev, &rIwReqInfo, &(prIwReq->u),
                             (char *)&(prIwReq->u));

#endif

    case IOCTL_GET_STR:

    default:
        return -EOPNOTSUPP;
    }  /* end of switch */
}

#if CFG_SUPPORT_BATCH_SCAN

EVENT_BATCH_RESULT_T g_rEventBatchResult[CFG_BATCH_MAX_MSCAN];

u32 batchChannelNum2Freq(u32 u4ChannelNum)
{
    u32 u4ChannelInMHz;

    if (u4ChannelNum >= 1 && u4ChannelNum <= 13) {
        u4ChannelInMHz = 2412 + (u4ChannelNum - 1) * 5;
    }else if (u4ChannelNum == 14) {
        u4ChannelInMHz = 2484;
    }else if (u4ChannelNum == 133) {
        u4ChannelInMHz = 3665;  /* 802.11y */
    }else if (u4ChannelNum == 137) {
        u4ChannelInMHz = 3685;  /* 802.11y */
    }else if (u4ChannelNum >= 34 && u4ChannelNum <= 165) {
        u4ChannelInMHz = 5000 + u4ChannelNum * 5;
    }else if (u4ChannelNum >= 183 && u4ChannelNum <= 196) {
        u4ChannelInMHz = 4000 + u4ChannelNum * 5;
    }else{
        u4ChannelInMHz = 0;
    }

    return u4ChannelInMHz;
}

#define TMP_TEXT_LEN_S    40
#define TMP_TEXT_LEN_L    60
static u8 text1[TMP_TEXT_LEN_S], text2[TMP_TEXT_LEN_L],
          text3[TMP_TEXT_LEN_L];  /* A safe len */

WLAN_STATUS
batchConvertResult(IN P_EVENT_BATCH_RESULT_T prEventBatchResult,
                   OUT void *pvBuffer, IN u32 u4MaxBufferLen,
                   OUT u32 *pu4RetLen)
{
    s8 *p = pvBuffer;
    s8 ssid[ELEM_MAX_LEN_SSID + 1];
    s32 nsize, nsize1, nsize2, nsize3, scancount;
    s32 i, j, nleft;
    u32 freq;

    P_EVENT_BATCH_RESULT_ENTRY_T prEntry;
    P_EVENT_BATCH_RESULT_T pBr;

    nsize = 0;
    nleft = u4MaxBufferLen;

    pBr = prEventBatchResult;
    scancount = 0;
    for (j = 0; j < CFG_BATCH_MAX_MSCAN; j++) {
        scancount += pBr->ucScanCount;
        pBr++;
    }

    nsize1 = scnprintf(text1, TMP_TEXT_LEN_S,
                       "scancount=%ld\nnextcount=%ld\n", scancount,
                       scancount);
    if (nsize1 < nleft) {
        nsize1 = scnprintf(p, nleft, "%s", text1);
        p += nsize1;
        nleft -= nsize1;
    } else {
        goto short_buf;
    }

    pBr = prEventBatchResult;
    for (j = 0; j < CFG_BATCH_MAX_MSCAN; j++) {
        DBGLOG(SCN, TRACE, "convert mscan = %d, apcount=%d, nleft=%d\n",
               j, pBr->ucScanCount, nleft);

        if (pBr->ucScanCount == 0) {
            pBr++;
            continue;
        }

        nleft -= 5;  /* -5 for "####\n" */

        /* We only support one round scan result now. */
        nsize1 = scnprintf(text1, TMP_TEXT_LEN_S, "apcount=%d\n",
                           pBr->ucScanCount);
        if (nsize1 < nleft) {
            nsize1 = scnprintf(p, nleft, "%s", text1);
            p += nsize1;
            nleft -= nsize1;
        } else {
            goto short_buf;
        }

        for (i = 0; i < pBr->ucScanCount; i++) {
            prEntry = &pBr->arBatchResult[i];

            nsize1 = scnprintf(text1, TMP_TEXT_LEN_S,
                               "bssid=" MACSTR "\n",
                               MAC2STR(prEntry->aucBssid));
            kalMemCopy(ssid, prEntry->aucSSID,
                       (prEntry->ucSSIDLen < ELEM_MAX_LEN_SSID ?
                        prEntry->ucSSIDLen :
                        ELEM_MAX_LEN_SSID));
            ssid[(prEntry->ucSSIDLen < (ELEM_MAX_LEN_SSID - 1) ?
                  prEntry->ucSSIDLen :
                  (ELEM_MAX_LEN_SSID - 1))] = '\0';
            nsize2 = scnprintf(text2, TMP_TEXT_LEN_L, "ssid=%s\n",
                               ssid);

            freq = batchChannelNum2Freq(prEntry->ucFreq);
            nsize3 = scnprintf(
                text3,
                TMP_TEXT_LEN_L,
                "freq=%lu\nlevel=%d\ndist=%lu\ndistSd=%lu\n====\n",
                freq,
                prEntry->cRssi,
                prEntry->u4Dist,
                prEntry->u4Distsd);

            nsize = nsize1 + nsize2 + nsize3;
            if (nsize < nleft) {
                kalStrnCpy(p, text1, TMP_TEXT_LEN_S);
                p += nsize1;

                kalStrnCpy(p, text2, TMP_TEXT_LEN_L);
                p += nsize2;

                kalStrnCpy(p, text3, TMP_TEXT_LEN_L);
                p += nsize3;

                nleft -= nsize;
            } else {
                DBGLOG(SCN, TRACE,
                       "Warning: Early break! (%d)\n", i);
                break;  /* discard following entries, TODO:
                         * apcount? */
            }
        }

        nsize1 = kalSnprintf(text1, TMP_TEXT_LEN_S, "%s", "####\n");
        if (nsize1 < nleft) {
            nsize1 = scnprintf(p, nleft, "%s", text1);
            p += nsize1;
            nleft -= nsize1;
        } else {
            goto short_buf;
        }

        pBr++;
    }

    nsize1 = scnprintf(text1, TMP_TEXT_LEN_S, "%s", "----\n");
    if (nsize1 < nleft) {
        scnprintf(p, nleft, "%s", text1);
    }else{
        goto short_buf;
    }

    *pu4RetLen = u4MaxBufferLen - nleft;
    DBGLOG(SCN, TRACE, "total len = %d (max len = %d)\n", *pu4RetLen,
           u4MaxBufferLen);

    return WLAN_STATUS_SUCCESS;

short_buf:
    DBGLOG(SCN, TRACE, "Short buffer issue! %d > %d, %s\n",
           u4MaxBufferLen + (nsize - nleft), u4MaxBufferLen, pvBuffer);
    return WLAN_STATUS_INVALID_LENGTH;
}
#endif

void parseNoiseHistogramReport(s32 *i4BytesWritten, s8 *pcCommand,
                               int *i4TotalLen,
                               IN struct CMD_NOISE_HISTOGRAM_REPORT *cmd)
{
    if (cmd->ucAction == CMD_NOISE_HISTOGRAM_GET) {
        *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                    *i4TotalLen - *i4BytesWritten,
                                    "\nWF0 Noise IPI");
#if CFG_IPI_2CHAIN_SUPPORT
    } else if (cmd->ucAction == CMD_NOISE_HISTOGRAM_GET2) {
        *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                    *i4TotalLen - *i4BytesWritten,
                                    "\nWF1 Noise IPI");
#endif
    }
    *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                *i4TotalLen - *i4BytesWritten,
                                "\n Power > -55: %10d", cmd->u4IPI10);
    *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                *i4TotalLen - *i4BytesWritten,
                                "\n-55 >= Power > -60: %10d", cmd->u4IPI9);
    *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                *i4TotalLen - *i4BytesWritten,
                                "\n-60 >= Power > -65: %10d", cmd->u4IPI8);
    *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                *i4TotalLen - *i4BytesWritten,
                                "\n-65 >= Power > -70: %10d", cmd->u4IPI7);
    *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                *i4TotalLen - *i4BytesWritten,
                                "\n-70 >= Power > -75: %10d", cmd->u4IPI6);
    *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                *i4TotalLen - *i4BytesWritten,
                                "\n-75 >= Power > -80: %10d", cmd->u4IPI5);
    *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                *i4TotalLen - *i4BytesWritten,
                                "\n-80 >= Power > -83: %10d", cmd->u4IPI4);
    *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                *i4TotalLen - *i4BytesWritten,
                                "\n-83 >= Power > -86: %10d", cmd->u4IPI3);
    *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                *i4TotalLen - *i4BytesWritten,
                                "\n-86 >= Power > -89: %10d", cmd->u4IPI2);
    *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                *i4TotalLen - *i4BytesWritten,
                                "\n-89 >= Power > -92: %10d", cmd->u4IPI1);
    *i4BytesWritten += snprintf(pcCommand + *i4BytesWritten,
                                *i4TotalLen - *i4BytesWritten,
                                "\n-92 >= Power : %10d", cmd->u4IPI0);
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl set int handler.
 *
 * \param[in] prNetDev Net device requested.
 * \param[in] prIwReqInfo Pointer to iwreq structure.
 * \param[in] prIwReqData The ioctl data structure, use the field of
 * sub-command. \param[in] pcExtra The buffer with input value
 *
 * \retval 0 For success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EINVAL If a value is out of range.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_set_int(IN struct net_device *prNetDev,
                 IN struct iw_request_info *prIwReqInfo,
                 IN union iwreq_data *prIwReqData, IN char *pcExtra)
{
    u32 u4SubCmd;
    u32 *pu4IntBuf;
    P_NDIS_TRANSPORT_STRUCT prNdisReq;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4BufLen = 0;
    int status = 0;
    P_PTA_IPC_T prPtaIpc;

    ASSERT(prNetDev);
    ASSERT(prIwReqInfo);
    ASSERT(prIwReqData);
    ASSERT(pcExtra);

    if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == false) {
        return -EINVAL;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    if (!test_bit(GLUE_FLAG_ADAPT_RDY_BIT, &prGlueInfo->ulFlag)) {
        DBGLOG(REQ, INFO, "adapter is not ready\n");
        return -EIO;
    }

    u4SubCmd = (u32)prIwReqData->mode;
    pu4IntBuf = (u32 *)pcExtra;

    switch (u4SubCmd) {
    case PRIV_CMD_TEST_MODE:

        prNdisReq = (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0];

        if (pu4IntBuf[1] == PRIV_CMD_TEST_MAGIC_KEY) {
            prNdisReq->ndisOidCmd = OID_CUSTOM_TEST_MODE;
        } else if (pu4IntBuf[1] == 0) {
            prNdisReq->ndisOidCmd = OID_CUSTOM_ABORT_TEST_MODE;
        } else if (pu4IntBuf[1] == PRIV_CMD_TEST_MAGIC_KEY_ICAP) {
            prNdisReq->ndisOidCmd = OID_CUSTOM_TEST_ICAP_MODE;
        } else {
            status = 0;
            break;
        }
        prNdisReq->inNdisOidlength = 0;
        prNdisReq->outNdisOidLength = 0;

        /* Execute this OID */
        status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
        break;

    case PRIV_CMD_TEST_CMD:

        prNdisReq = (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0];

        kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

        prNdisReq->ndisOidCmd = OID_CUSTOM_MTK_WIFI_TEST;
        prNdisReq->inNdisOidlength = 8;
        prNdisReq->outNdisOidLength = 8;

        /* Execute this OID */
        status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
        break;

#if CFG_SUPPORT_PRIV_MCR_RW
    case PRIV_CMD_ACCESS_MCR:

        prNdisReq = (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0];

        if (!prGlueInfo->fgMcrAccessAllowed) {
            if (pu4IntBuf[1] == PRIV_CMD_TEST_MAGIC_KEY &&
                pu4IntBuf[2] == PRIV_CMD_TEST_MAGIC_KEY) {
                prGlueInfo->fgMcrAccessAllowed = true;
            }
            status = 0;
            break;
        }

        if (pu4IntBuf[1] == PRIV_CMD_TEST_MAGIC_KEY &&
            pu4IntBuf[2] == PRIV_CMD_TEST_MAGIC_KEY) {
            status = 0;
            break;
        }

        kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

        prNdisReq->ndisOidCmd = OID_CUSTOM_MCR_RW;
        prNdisReq->inNdisOidlength = 8;
        prNdisReq->outNdisOidLength = 8;

        /* Execute this OID */
        status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
        break;
#endif

    case PRIV_CMD_SW_CTRL:

        prNdisReq = (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0];

        kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

        prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
        prNdisReq->inNdisOidlength = 8;
        prNdisReq->outNdisOidLength = 8;

        /* Execute this OID */
        status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
        break;

#if CFG_TCP_IP_CHKSUM_OFFLOAD
    case PRIV_CMD_CSUM_OFFLOAD: {
        u32 u4CSUMFlags;

        if (pu4IntBuf[1] == 1) {
            u4CSUMFlags = CSUM_OFFLOAD_EN_ALL;
        }else if (pu4IntBuf[1] == 0) {
            u4CSUMFlags = 0;
        }else{
            return -EINVAL;
        }

        if (kalIoctl(prGlueInfo, wlanoidSetCSUMOffload,
                     (void *)&u4CSUMFlags, sizeof(u32), false, false,
                     true, &u4BufLen) == WLAN_STATUS_SUCCESS) {
            if (pu4IntBuf[1] == 1) {
                prNetDev->features |= NETIF_F_IP_CSUM |
                                      NETIF_F_IPV6_CSUM |
                                      NETIF_F_RXCSUM;
            } else if (pu4IntBuf[1] == 0) {
                prNetDev->features &=
                    ~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
                      NETIF_F_RXCSUM);
            }
        }
    } break;
#endif

    case PRIV_CMD_POWER_MODE: {
        PARAM_POWER_MODE_T rPowerMode;

        P_BSS_INFO_T prBssInfo = prGlueInfo->prAdapter->prAisBssInfo;

        if (!prBssInfo) {
            break;
        }

        rPowerMode.ePowerMode = (PARAM_POWER_MODE)pu4IntBuf[1];
        rPowerMode.ucBssIdx = prBssInfo->ucBssIndex;

        /* pu4IntBuf[0] is used as input SubCmd */
        kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile,
                 &rPowerMode, sizeof(PARAM_POWER_MODE_T), false, false,
                 true, &u4BufLen);
    } break;

    case PRIV_CMD_WMM_PS: {
        PARAM_CUSTOM_WMM_PS_TEST_STRUCT_T rWmmPsTest;

        rWmmPsTest.bmfgApsdEnAc = (u8)pu4IntBuf[1];
        rWmmPsTest.ucIsEnterPsAtOnce = (u8)pu4IntBuf[2];
        rWmmPsTest.ucIsDisableUcTrigger = (u8)pu4IntBuf[3];
        rWmmPsTest.reserved = 0;

        kalIoctl(prGlueInfo, wlanoidSetWiFiWmmPsTest,
                 (void *)&rWmmPsTest,
                 sizeof(PARAM_CUSTOM_WMM_PS_TEST_STRUCT_T), false,
                 false, true, &u4BufLen);
    } break;

    case PRIV_CUSTOM_BWCS_CMD:

        DBGLOG(REQ, INFO,
               "pu4IntBuf[1] = %x, size of PTA_IPC_T = %d.\n",
               pu4IntBuf[1], sizeof(PARAM_PTA_IPC_T));

        prPtaIpc = (P_PTA_IPC_T)aucOidBuf;
        prPtaIpc->u.aucBTPParams[0] = (u8)(pu4IntBuf[1] >> 24);
        prPtaIpc->u.aucBTPParams[1] = (u8)(pu4IntBuf[1] >> 16);
        prPtaIpc->u.aucBTPParams[2] = (u8)(pu4IntBuf[1] >> 8);
        prPtaIpc->u.aucBTPParams[3] = (u8)(pu4IntBuf[1]);

        DBGLOG(REQ,
               INFO,
               "BCM BWCS CMD : PRIV_CUSTOM_BWCS_CMD : aucBTPParams[0] = %02x, aucBTPParams[1] = %02x.\n",
               prPtaIpc->u.aucBTPParams[0],
               prPtaIpc->u.aucBTPParams[1]);
        DBGLOG(REQ,
               INFO,
               "BCM BWCS CMD : PRIV_CUSTOM_BWCS_CMD : aucBTPParams[2] = %02x, aucBTPParams[3] = %02x.\n",
               prPtaIpc->u.aucBTPParams[2],
               prPtaIpc->u.aucBTPParams[3]);

        status = wlanoidSetBT(prGlueInfo->prAdapter,
                              (void *)&aucOidBuf[0],
                              sizeof(PARAM_PTA_IPC_T), &u4BufLen);

        if (status != WLAN_STATUS_SUCCESS) {
            status = -EFAULT;
        }

        break;

    case PRIV_CMD_BAND_CONFIG: {
        DBGLOG(INIT, INFO, "CMD set_band = %lu\n", (u32)pu4IntBuf[1]);
    } break;

#if CFG_ENABLE_WIFI_DIRECT
    case PRIV_CMD_P2P_MODE: {
        PARAM_CUSTOM_P2P_SET_STRUCT_T rSetP2P;
        WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;

        rSetP2P.u4Enable = pu4IntBuf[1];
        rSetP2P.u4Mode = pu4IntBuf[2];

        if (!rSetP2P.u4Enable) {
            p2pNetUnregister(prGlueInfo, true);
        }

        if (((rSetP2P.u4Mode == RUNNING_DUAL_AP_MODE) ||
             (rSetP2P.u4Mode == RUNNING_P2P_AP_MODE)) &&
            (gprP2pRoleWdev[1] == NULL) && (rSetP2P.u4Enable)) {
            glP2pCreateWirelessDevice(prGlueInfo);
            ASSERT(gprP2pRoleWdev[1] != NULL);
        }

        /* pu4IntBuf[0] is used as input SubCmd */
        rWlanStatus = kalIoctl(prGlueInfo, wlanoidSetP2pMode,
                               (void *)&rSetP2P,
                               sizeof(PARAM_CUSTOM_P2P_SET_STRUCT_T),
                               false, false, true, &u4BufLen);

        if ((rSetP2P.u4Enable) &&
            (rWlanStatus == WLAN_STATUS_SUCCESS)) {
            if (p2pNetRegister(prGlueInfo, true) == false) {
                DBGLOG(INIT, ERROR, "p2pNetRegister failed\n");
            }
        }
    } break;
#endif

#if (CFG_MET_PACKET_TRACE_SUPPORT == 1)
    case PRIV_CMD_MET_PROFILING: {
        /* PARAM_CUSTOM_WFD_DEBUG_STRUCT_T rWfdDebugModeInfo; */
        /* rWfdDebugModeInfo.ucWFDDebugMode=(u8)pu4IntBuf[1]; */
        /* rWfdDebugModeInfo.u2SNPeriod=(u16)pu4IntBuf[2]; */
        /* DBGLOG(REQ, INFO, ("WFD Debug Mode:%d Period:%d\n",
         *  rWfdDebugModeInfo.ucWFDDebugMode,
         * rWfdDebugModeInfo.u2SNPeriod));
         */
        prGlueInfo->fgMetProfilingEn = (u8)pu4IntBuf[1];
        prGlueInfo->u2MetUdpPort = (u16)pu4IntBuf[2];
        /* DBGLOG(INIT, INFO, ("MET_PROF: Enable=%d UDP_PORT=%d\n",
         *  prGlueInfo->fgMetProfilingEn, prGlueInfo->u2MetUdpPort);
         */
    } break;
#endif

    default:
        return -EOPNOTSUPP;
    }

    return status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl get int handler.
 *
 * \param[in] pDev Net device requested.
 * \param[out] pIwReq Pointer to iwreq structure.
 * \param[in] prIwReqData The ioctl req structure, use the field of sub-command.
 * \param[out] pcExtra The buffer with put the return value
 *
 * \retval 0 For success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EFAULT For fail.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_get_int(IN struct net_device *prNetDev,
                 IN struct iw_request_info *prIwReqInfo,
                 IN union iwreq_data *prIwReqData, IN OUT char *pcExtra)
{
    u32 u4SubCmd;
    u32 *pu4IntBuf;
    P_GLUE_INFO_T prGlueInfo;
    u32 u4BufLen = 0;
    int status = 0;
    P_NDIS_TRANSPORT_STRUCT prNdisReq;
    s32 ch[MAX_CHN_NUM];

    ASSERT(prNetDev);
    ASSERT(prIwReqInfo);
    ASSERT(prIwReqData);
    ASSERT(pcExtra);
    if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == false) {
        return -EINVAL;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    if (!test_bit(GLUE_FLAG_ADAPT_RDY_BIT, &prGlueInfo->ulFlag)) {
        DBGLOG(REQ, INFO, "adapter is not ready\n");
        return -EIO;
    }

    u4SubCmd = (u32)prIwReqData->mode;
    pu4IntBuf = (u32 *)pcExtra;

    switch (u4SubCmd) {
    case PRIV_CMD_TEST_CMD:

        prNdisReq = (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0];

        kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

        prNdisReq->ndisOidCmd = OID_CUSTOM_MTK_WIFI_TEST;
        prNdisReq->inNdisOidlength = 8;
        prNdisReq->outNdisOidLength = 8;

        status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
        if (status == 0) {
            prIwReqData->mode =
                *(u32 *)&prNdisReq->ndisOidContent[4];
            /*
             *  if (copy_to_user(prIwReqData->data.pointer,
             *  &prNdisReq->ndisOidContent[4], 4)) {
             *  printk(KERN_NOTICE "priv_get_int() copy_to_user
             * oidBuf fail(3)\n"); return -EFAULT;
             *  }
             */
        }
        return status;

#if CFG_SUPPORT_PRIV_MCR_RW
    case PRIV_CMD_ACCESS_MCR:

        prNdisReq = (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0];

        if (!prGlueInfo->fgMcrAccessAllowed) {
            status = 0;
            return status;
        }

        kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

        prNdisReq->ndisOidCmd = OID_CUSTOM_MCR_RW;
        prNdisReq->inNdisOidlength = 8;
        prNdisReq->outNdisOidLength = 8;

        status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
        if (status == 0) {
            prIwReqData->mode =
                *(u32 *)&prNdisReq->ndisOidContent[4];
        }
        return status;

#endif

    case PRIV_CMD_DUMP_MEM:
        prNdisReq = (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0];

        if (!prGlueInfo->fgMcrAccessAllowed ||
            !capable(CAP_NET_ADMIN)) {
            DBGLOG(REQ, WARN, "Access Denied\n");
            status = 0;
            return status;
        }

        kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

        prNdisReq->ndisOidCmd = OID_CUSTOM_MEM_DUMP;
        prNdisReq->inNdisOidlength =
            sizeof(PARAM_CUSTOM_MEM_DUMP_STRUCT_T);
        prNdisReq->outNdisOidLength =
            sizeof(PARAM_CUSTOM_MEM_DUMP_STRUCT_T);

        status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
        if (status == 0) {
            prIwReqData->mode =
                *(u32 *)&prNdisReq->ndisOidContent[0];
        }
        return status;

    case PRIV_CMD_SW_CTRL:

        prNdisReq = (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0];

        kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

        prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
        prNdisReq->inNdisOidlength = 8;
        prNdisReq->outNdisOidLength = 8;

        status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
        if (status == 0) {
            prIwReqData->mode =
                *(u32 *)&prNdisReq->ndisOidContent[4];
        }
        return status;

    case PRIV_CMD_BAND_CONFIG:
        DBGLOG(INIT, INFO, "CMD get_band=\n");
        prIwReqData->mode = 0;
        return status;

    default:
        break;
    }

    u4SubCmd = (u32)prIwReqData->data.flags;

    switch (u4SubCmd) {
    case PRIV_CMD_GET_CH_LIST: {
        u16 j = 0;
        u8 NumOfChannel = MAX_CHN_NUM;
        u8 ucMaxChannelNum = MAX_CHN_NUM;
        RF_CHANNEL_INFO_T aucChannelList[MAX_CHN_NUM];

        DBGLOG(RLM, INFO, "Domain: Query Channel List.\n");
        kalMemZero(aucChannelList,
                   sizeof(RF_CHANNEL_INFO_T) * MAX_CHN_NUM);
        kalGetChannelList(prGlueInfo, BAND_NULL, ucMaxChannelNum,
                          &NumOfChannel, aucChannelList);
        if (NumOfChannel > MAX_CHN_NUM) {
            NumOfChannel = MAX_CHN_NUM;
        }

        /* kalIsAPmode() always returns false => remove it to avoid
         * security issue: */
        /*                                       Leak kernel stack data
         * to userland */
        for (j = 0; j < NumOfChannel; j++)
            ch[j] = (s32)aucChannelList[j].ucChannelNum;

        prIwReqData->data.length = j;
        if (copy_to_user(prIwReqData->data.pointer, ch,
                         NumOfChannel * sizeof(s32))) {
            return -EFAULT;
        }else{
            return status;
        }
    }

    default:
        return -EOPNOTSUPP;
    }

    return status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl set int array handler.
 *
 * \param[in] prNetDev Net device requested.
 * \param[in] prIwReqInfo Pointer to iwreq structure.
 * \param[in] prIwReqData The ioctl data structure, use the field of
 * sub-command. \param[in] pcExtra The buffer with input value
 *
 * \retval 0 For success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EINVAL If a value is out of range.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_set_ints(IN struct net_device *prNetDev,
                  IN struct iw_request_info *prIwReqInfo,
                  IN union iwreq_data *prIwReqData, IN char *pcExtra)
{
    u32 u4SubCmd, u4BufLen, u4CmdLen;
    P_GLUE_INFO_T prGlueInfo;
    int status = 0;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    P_SET_TXPWR_CTRL_T prTxpwr;
    u16 i = 0;
    s32 setting[4] = { 0 };

    ASSERT(prNetDev);
    ASSERT(prIwReqInfo);
    ASSERT(prIwReqData);
    ASSERT(pcExtra);

    if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == false) {
        return -EINVAL;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    if (!test_bit(GLUE_FLAG_ADAPT_RDY_BIT, &prGlueInfo->ulFlag)) {
        DBGLOG(REQ, INFO, "adapter is not ready\n");
        return -EIO;
    }

    u4SubCmd = (u32)prIwReqData->data.flags;
    u4CmdLen = prIwReqData->data.length;

    switch (u4SubCmd) {
    case PRIV_CMD_SET_TX_POWER: {
        if (u4CmdLen > 4) {
            return -EINVAL;
        }

        if (copy_from_user(setting, prIwReqData->data.pointer,
                           u4CmdLen)) {
            return -EFAULT;
        }

        prTxpwr = &prGlueInfo->rTxPwr;
        if (setting[0] == 0 &&
            prIwReqData->data.length == 4  /* argc num */ ) {
            /* 0 (All networks), 1 (legacy STA), 2 (Hotspot AP), 3
             * (P2P), 4 (BT over Wi-Fi) */
            if (setting[1] == 1 || setting[1] == 0) {
                if (setting[2] == 0 || setting[2] == 1) {
                    prTxpwr->c2GLegacyStaPwrOffset =
                        setting[3];
                }
                if (setting[2] == 0 || setting[2] == 2) {
                    prTxpwr->c5GLegacyStaPwrOffset =
                        setting[3];
                }
            }
            if (setting[1] == 2 || setting[1] == 0) {
                if (setting[2] == 0 || setting[2] == 1) {
                    prTxpwr->c2GHotspotPwrOffset =
                        setting[3];
                }
                if (setting[2] == 0 || setting[2] == 2) {
                    prTxpwr->c5GHotspotPwrOffset =
                        setting[3];
                }
            }
            if (setting[1] == 3 || setting[1] == 0) {
                if (setting[2] == 0 || setting[2] == 1) {
                    prTxpwr->c2GP2pPwrOffset = setting[3];
                }
                if (setting[2] == 0 || setting[2] == 2) {
                    prTxpwr->c5GP2pPwrOffset = setting[3];
                }
            }
            if (setting[1] == 4 || setting[1] == 0) {
                if (setting[2] == 0 || setting[2] == 1) {
                    prTxpwr->c2GBowPwrOffset = setting[3];
                }
                if (setting[2] == 0 || setting[2] == 2) {
                    prTxpwr->c5GBowPwrOffset = setting[3];
                }
            }
        } else if (setting[0] == 1 && prIwReqData->data.length == 2) {
            prTxpwr->ucConcurrencePolicy = setting[1];
        } else if (setting[0] == 2 && prIwReqData->data.length == 3) {
            if (setting[1] == 0) {
                for (i = 0; i < 14; i++)
                    prTxpwr->acTxPwrLimit2G[i] = setting[2];
            } else if (setting[1] <= 14) {
                prTxpwr->acTxPwrLimit2G[setting[1] - 1] =
                    setting[2];
            }
        } else if (setting[0] == 3 && prIwReqData->data.length == 3) {
            if (setting[1] == 0) {
                for (i = 0; i < 4; i++)
                    prTxpwr->acTxPwrLimit5G[i] = setting[2];
            } else if (setting[1] <= 4) {
                prTxpwr->acTxPwrLimit5G[setting[1] - 1] =
                    setting[2];
            }
        } else if (setting[0] == 4 && prIwReqData->data.length == 2) {
            if (setting[1] == 0) {
                wlanDefTxPowerCfg(prGlueInfo->prAdapter);
            }
            rStatus = kalIoctl(prGlueInfo, wlanoidSetTxPower,
                               prTxpwr, sizeof(SET_TXPWR_CTRL_T),
                               false, false, true, &u4BufLen);
        } else {
            return -EFAULT;
        }
    }
        return status;

    default:
        break;
    }

    return status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl get int array handler.
 *
 * \param[in] pDev Net device requested.
 * \param[out] pIwReq Pointer to iwreq structure.
 * \param[in] prIwReqData The ioctl req structure, use the field of sub-command.
 * \param[out] pcExtra The buffer with put the return value
 *
 * \retval 0 For success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EFAULT For fail.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_get_ints(IN struct net_device *prNetDev,
                  IN struct iw_request_info *prIwReqInfo,
                  IN union iwreq_data *prIwReqData, IN OUT char *pcExtra)
{
    u32 u4SubCmd;
    P_GLUE_INFO_T prGlueInfo;
    int status = 0;
    s32 ch[MAX_CHN_NUM];

    ASSERT(prNetDev);
    ASSERT(prIwReqInfo);
    ASSERT(prIwReqData);
    ASSERT(pcExtra);
    if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == false) {
        return -EINVAL;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    if (!test_bit(GLUE_FLAG_ADAPT_RDY_BIT, &prGlueInfo->ulFlag)) {
        DBGLOG(REQ, INFO, "adapter is not ready\n");
        return -EIO;
    }

    u4SubCmd = (u32)prIwReqData->data.flags;

    switch (u4SubCmd) {
    case PRIV_CMD_GET_CH_LIST: {
        u16 i;
        u8 NumOfChannel = MAX_CHN_NUM;
        u8 ucMaxChannelNum = MAX_CHN_NUM;
        RF_CHANNEL_INFO_T aucChannelList[MAX_CHN_NUM];

        kalMemZero(aucChannelList,
                   sizeof(RF_CHANNEL_INFO_T) * MAX_CHN_NUM);
        kalGetChannelList(prGlueInfo, BAND_NULL, ucMaxChannelNum,
                          &NumOfChannel, aucChannelList);
        if (NumOfChannel > MAX_CHN_NUM) {
            NumOfChannel = MAX_CHN_NUM;
        }

        for (i = 0; i < NumOfChannel; i++)
            ch[i] = (s32)aucChannelList[i].ucChannelNum;

        prIwReqData->data.length = NumOfChannel;
        if (copy_to_user(prIwReqData->data.pointer, ch,
                         NumOfChannel * sizeof(s32))) {
            return -EFAULT;
        }else{
            return status;
        }
    }

    default:
        break;
    }

    return status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl set structure handler.
 *
 * \param[in] pDev Net device requested.
 * \param[in] prIwReqData Pointer to iwreq_data structure.
 *
 * \retval 0 For success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EINVAL If a value is out of range.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_set_struct(IN struct net_device *prNetDev,
                    IN struct iw_request_info *prIwReqInfo,
                    IN union iwreq_data *prIwReqData, IN char *pcExtra)
{
    u32 u4SubCmd = 0;
    int status = 0;
    /* WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS; */
    u32 u4CmdLen = 0;
    P_NDIS_TRANSPORT_STRUCT prNdisReq;

    P_GLUE_INFO_T prGlueInfo = NULL;
    u32 u4BufLen = 0;

    ASSERT(prNetDev);
    /* ASSERT(prIwReqInfo); */
    ASSERT(prIwReqData);
    /* ASSERT(pcExtra); */

    kalMemZero(&aucOidBuf[0], sizeof(aucOidBuf));

    if (GLUE_CHK_PR2(prNetDev, prIwReqData) == false) {
        return -EINVAL;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    if (!test_bit(GLUE_FLAG_ADAPT_RDY_BIT, &prGlueInfo->ulFlag)) {
        DBGLOG(REQ, INFO, "adapter is not ready\n");
        return -EIO;
    }

    u4SubCmd = (u32)prIwReqData->data.flags;
    switch (u4SubCmd) {
    case PRIV_CUSTOM_BWCS_CMD:
        u4CmdLen = prIwReqData->data.length * sizeof(u32);
        ASSERT(sizeof(PARAM_PTA_IPC_T) >= u4CmdLen);
        if (sizeof(PARAM_PTA_IPC_T) < u4CmdLen) {
            return -EFAULT;
        }

#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS && CFG_SUPPORT_BCM_BWCS_DEBUG
        DBGLOG(REQ,
               INFO,
               "ucCmdLen = %d, size of PTA_IPC_T = %d, prIwReqData->data = 0x%x.\n",
               u4CmdLen,
               sizeof(PARAM_PTA_IPC_T),
               prIwReqData->data);

        DBGLOG(REQ,
               INFO,
               "priv_set_struct(): prIwReqInfo->cmd(0x%X), u4SubCmd(%ld)\n",
               prIwReqInfo->cmd,
               u4SubCmd);
        DBGLOG(REQ, INFO, "*pcExtra = 0x%x\n", *pcExtra);
#endif

        if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer,
                           u4CmdLen)) {
            status = -EFAULT;  /* return -EFAULT; */
            break;
        }
#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS && CFG_SUPPORT_BCM_BWCS_DEBUG
        DBGLOG(REQ, INFO,
               "priv_set_struct(): BWCS CMD = %02x%02x%02x%02x\n",
               aucOidBuf[2], aucOidBuf[3], aucOidBuf[4], aucOidBuf[5]);
#endif
        status = wlanoidSetBT(prGlueInfo->prAdapter,
                              (void *)&aucOidBuf[0], u4CmdLen,
                              &u4BufLen);

        if (status != WLAN_STATUS_SUCCESS) {
            status = -EFAULT;
        }

        break;

#if CFG_SUPPORT_WPS2
    case PRIV_CMD_WSC_PROBE_REQ: {
        if (prIwReqData->data.length > 500) {
            status = -EINVAL;
            break;
        }

        /* retrieve IE for Probe Request */
        if (prIwReqData->data.length > 0) {
            if (copy_from_user(prGlueInfo->aucWSCIE,
                               prIwReqData->data.pointer,
                               prIwReqData->data.length)) {
                status = -EFAULT;
                break;
            }
            prGlueInfo->u2WSCIELen = prIwReqData->data.length;
        } else {
            prGlueInfo->u2WSCIELen = 0;
        }
    } break;

#endif
    case PRIV_CMD_OID:
        if (prIwReqData->data.length > 4096) {
            status = -EINVAL;
            break;
        }
        if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer,
                           prIwReqData->data.length)) {
            status = -EFAULT;
            break;
        }
        if (!kalMemCmp(&aucOidBuf[0], pcExtra,
                       prIwReqData->data.length)) {
            /* ToDo:: DBGLOG */
            DBGLOG(REQ, INFO, "pcExtra buffer is valid\n");
        } else {
            DBGLOG(REQ, INFO, "pcExtra 0x%p\n", pcExtra);
        }
        /* Execute this OID */
        status = priv_set_ndis(prNetDev,
                               (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0],
                               &u4BufLen);
        /* Copy result to user space */
        ((P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0])->outNdisOidLength =
            u4BufLen;

        if (copy_to_user(prIwReqData->data.pointer, &aucOidBuf[0],
                         OFFSET_OF(NDIS_TRANSPORT_STRUCT,
                                   ndisOidContent))) {
            DBGLOG(REQ, INFO, "copy_to_user oidBuf fail\n");
            status = -EFAULT;
        }

        break;

    case PRIV_CMD_SW_CTRL:
        if (prIwReqData->data.length >
            (sizeof(aucOidBuf) -
             OFFSET_OF(NDIS_TRANSPORT_STRUCT, ndisOidContent))) {
            DBGLOG(REQ, INFO,
                   "priv_get_struct() exceeds length limit\n");
            return -EFAULT;
        }

        prNdisReq = (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0];

        /* if (copy_from_user(&prNdisReq->ndisOidContent[0],
         * prIwReqData->data.pointer, prIwReqData->data.length)) {
         */
        /* Coverity uanble to detect real size of ndisOidContent,
         * it's 4096 bytes instead of 16 bytes
         */
        if (copy_from_user(&aucOidBuf[OFFSET_OF(NDIS_TRANSPORT_STRUCT,
                                                ndisOidContent)],
                           prIwReqData->data.pointer,
                           prIwReqData->data.length)) {
            status = -EFAULT;
            break;
        }
        prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
        prNdisReq->inNdisOidlength = 8;
        prNdisReq->outNdisOidLength = 8;

        /* Execute this OID */
        status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
        break;

    default:
        return -EOPNOTSUPP;
    }

    return status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl get struct handler.
 *
 * \param[in] pDev Net device requested.
 * \param[out] pIwReq Pointer to iwreq structure.
 * \param[in] cmd Private sub-command.
 *
 * \retval 0 For success.
 * \retval -EFAULT If copy from user space buffer fail.
 * \retval -EOPNOTSUPP Parameter "cmd" not recognized.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_get_struct(IN struct net_device *prNetDev,
                    IN struct iw_request_info *prIwReqInfo,
                    IN union iwreq_data *prIwReqData, IN OUT char *pcExtra)
{
    u32 u4SubCmd = 0;
    P_NDIS_TRANSPORT_STRUCT prNdisReq = NULL;

    P_GLUE_INFO_T prGlueInfo = NULL;
    u32 u4BufLen = 0;
    u32 *pu4IntBuf = NULL;
    u8 *prDest = NULL;
    int status = 0;

    kalMemZero(&aucOidBuf[0], sizeof(aucOidBuf));

    ASSERT(prNetDev);
    ASSERT(prIwReqData);
    if (!prNetDev || !prIwReqData) {
        DBGLOG(REQ, INFO,
               "priv_get_struct(): invalid param(0x%p, 0x%p)\n",
               prNetDev, prIwReqData);
        return -EINVAL;
    }

    u4SubCmd = (u32)prIwReqData->data.flags;
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    ASSERT(prGlueInfo);
    if (!prGlueInfo) {
        DBGLOG(REQ, INFO,
               "priv_get_struct(): invalid prGlueInfo(0x%p, 0x%p)\n",
               prNetDev, *((P_GLUE_INFO_T *)netdev_priv(prNetDev)));
        return -EINVAL;
    }

    if (!test_bit(GLUE_FLAG_ADAPT_RDY_BIT, &prGlueInfo->ulFlag)) {
        DBGLOG(REQ,
               INFO,
               "adapter is not ready\n");
        return -EIO;
    }

    memset(aucOidBuf, 0, sizeof(aucOidBuf));

    switch (u4SubCmd) {
    case PRIV_CMD_OID:
        if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer,
                           sizeof(NDIS_TRANSPORT_STRUCT))) {
            DBGLOG(REQ, INFO,
                   "priv_get_struct() copy_from_user oidBuf fail\n");
            return -EFAULT;
        }

        prNdisReq = (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0];
        if (priv_get_ndis(prNetDev, prNdisReq, &u4BufLen) == 0) {
            prNdisReq->outNdisOidLength = u4BufLen;
            kalMemCopy(pcExtra, prNdisReq,
                       u4BufLen + sizeof(NDIS_TRANSPORT_STRUCT) -
                       sizeof(prNdisReq->ndisOidContent));
            if (copy_to_user(
                    prIwReqData->data.pointer, &aucOidBuf[0],
                    u4BufLen + sizeof(NDIS_TRANSPORT_STRUCT) -
                    sizeof(prNdisReq->ndisOidContent))) {
                DBGLOG(REQ,
                       INFO,
                       "priv_get_struct() copy_to_user oidBuf fail(1)\n");
                return -EFAULT;
            }
            return 0;
        }
        prNdisReq->outNdisOidLength = u4BufLen;
        if (copy_to_user(prIwReqData->data.pointer, &aucOidBuf[0],
                         OFFSET_OF(NDIS_TRANSPORT_STRUCT,
                                   ndisOidContent))) {
            DBGLOG(REQ, INFO,
                   "priv_get_struct() copy_to_user oidBuf fail(2)\n");
        }
        return -EFAULT;

    case PRIV_CMD_SW_CTRL:
        pu4IntBuf = (u32 *)prIwReqData->data.pointer;
        prNdisReq = (P_NDIS_TRANSPORT_STRUCT)&aucOidBuf[0];
        prDest = (u8 *)&aucOidBuf[OFFSET_OF(NDIS_TRANSPORT_STRUCT,
                                            ndisOidContent)];

        if (prIwReqData->data.length >
            (sizeof(aucOidBuf) -
             OFFSET_OF(NDIS_TRANSPORT_STRUCT, ndisOidContent))) {
            DBGLOG(REQ, INFO,
                   "priv_get_struct() exceeds length limit\n");
            return -EFAULT;
        }

        if (copy_from_user(prDest, prIwReqData->data.pointer,
                           prIwReqData->data.length)) {
            DBGLOG(REQ, INFO,
                   "priv_get_struct() copy_from_user oidBuf fail\n");
            return -EFAULT;
        }

        prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
        prNdisReq->inNdisOidlength = 8;
        prNdisReq->outNdisOidLength = 8;

        status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
        if (status == 0) {
            prNdisReq->outNdisOidLength = u4BufLen;

            if (copy_to_user(prIwReqData->data.pointer,
                             &prNdisReq->ndisOidContent[4], 4)) {
                DBGLOG(REQ,
                       INFO,
                       "priv_get_struct() copy_to_user oidBuf fail(2)\n");
            }
        }
        return 0;

    default:
        DBGLOG(REQ, WARN, "get struct cmd:0x%lx\n", u4SubCmd);
        return -EOPNOTSUPP;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief The routine handles a set operation for a single OID.
 *
 * \param[in] pDev Net device requested.
 * \param[in] ndisReq Ndis request OID information copy from user.
 * \param[out] outputLen_p If the call is successful, returns the number of
 *                         bytes written into the query buffer. If the
 *                         call failed due to invalid length of the query
 *                         buffer, returns the amount of storage needed..
 *
 * \retval 0 On success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 *
 */
/*----------------------------------------------------------------------------*/
static int priv_set_ndis(IN struct net_device *prNetDev,
                         IN NDIS_TRANSPORT_STRUCT *prNdisReq,
                         OUT u32 *pu4OutputLen)
{
    P_WLAN_REQ_ENTRY prWlanReqEntry = NULL;
    WLAN_STATUS status = WLAN_STATUS_SUCCESS;
    P_GLUE_INFO_T prGlueInfo = NULL;
    u32 u4SetInfoLen = 0;

    ASSERT(prNetDev);
    ASSERT(prNdisReq);
    ASSERT(pu4OutputLen);

    if (!prNetDev || !prNdisReq || !pu4OutputLen) {
        DBGLOG(REQ, INFO,
               "priv_set_ndis(): invalid param(0x%p, 0x%p, 0x%p)\n",
               prNetDev, prNdisReq, pu4OutputLen);
        return -EINVAL;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    ASSERT(prGlueInfo);
    if (!prGlueInfo) {
        DBGLOG(REQ, INFO,
               "priv_set_ndis(): invalid prGlueInfo(0x%p, 0x%p)\n",
               prNetDev, *((P_GLUE_INFO_T *)netdev_priv(prNetDev)));
        return -EINVAL;
    }

    if (reqSearchSupportedOidEntry(prNdisReq->ndisOidCmd,
                                   &prWlanReqEntry) == false) {
        /* WARNLOG(("Set OID: 0x%08lx (unknown)\n",
         * prNdisReq->ndisOidCmd)); */
        return -EOPNOTSUPP;
    }

    if (prWlanReqEntry->pfOidSetHandler == NULL) {
        /* WARNLOG(("Set %s: Null set handler\n",
         * prWlanReqEntry->pucOidName)); */
        return -EOPNOTSUPP;
    }

    if (prWlanReqEntry->fgSetBufLenChecking) {
        if (prNdisReq->inNdisOidlength !=
            prWlanReqEntry->u4InfoBufLen) {
            DBGLOG(REQ,
                   WARN,
                   "Set %s: Invalid length (current=%ld, needed=%ld)\n",
                   prWlanReqEntry->pucOidName,
                   prNdisReq->inNdisOidlength,
                   prWlanReqEntry->u4InfoBufLen);

            *pu4OutputLen = prWlanReqEntry->u4InfoBufLen;
            return -EINVAL;
        }
    } else {
        if (prNdisReq->inNdisOidlength >
            (sizeof(aucOidBuf) -
             OFFSET_OF(NDIS_TRANSPORT_STRUCT, ndisOidContent))) {
            DBGLOG(REQ, INFO, "exceeds length limit\n");
            return -EINVAL;
        }
    }

    if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_ONLY) {
        /* GLUE sw info only */
        status = prWlanReqEntry->pfOidSetHandler(
            prGlueInfo, prNdisReq->ndisOidContent,
            prNdisReq->inNdisOidlength, &u4SetInfoLen);
    } else if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_EXTENSION) {
        /* multiple sw operations */
        status = prWlanReqEntry->pfOidSetHandler(
            prGlueInfo, prNdisReq->ndisOidContent,
            prNdisReq->inNdisOidlength, &u4SetInfoLen);
    } else if (prWlanReqEntry->eOidMethod == ENUM_OID_DRIVER_CORE) {
        /* driver core */

        status = kalIoctl(
            prGlueInfo,
            (PFN_OID_HANDLER_FUNC)prWlanReqEntry->pfOidSetHandler,
            prNdisReq->ndisOidContent, prNdisReq->inNdisOidlength,
            false, false, true, &u4SetInfoLen);
    } else {
        DBGLOG(REQ, INFO,
               "priv_set_ndis(): unsupported OID method:0x%x\n",
               prWlanReqEntry->eOidMethod);
        return -EOPNOTSUPP;
    }

    *pu4OutputLen = u4SetInfoLen;

    switch (status) {
    case WLAN_STATUS_SUCCESS:
        break;

    case WLAN_STATUS_INVALID_LENGTH:
        /* WARNLOG(("Set %s: Invalid length (current=%ld,
         * needed=%ld)\n", */
        /* prWlanReqEntry->pucOidName, */
        /* prNdisReq->inNdisOidlength, */
        /* u4SetInfoLen)); */
        break;
    }

    if (status != WLAN_STATUS_SUCCESS) {
        return -EFAULT;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief The routine handles a query operation for a single OID. Basically we
 *   return information about the current state of the OID in question.
 *
 * \param[in] pDev Net device requested.
 * \param[in] ndisReq Ndis request OID information copy from user.
 * \param[out] outputLen_p If the call is successful, returns the number of
 *                        bytes written into the query buffer. If the
 *                        call failed due to invalid length of the query
 *                        buffer, returns the amount of storage needed..
 *
 * \retval 0 On success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EINVAL invalid input parameters
 *
 */
/*----------------------------------------------------------------------------*/
static int priv_get_ndis(IN struct net_device *prNetDev,
                         IN NDIS_TRANSPORT_STRUCT *prNdisReq,
                         OUT u32 *pu4OutputLen)
{
    P_WLAN_REQ_ENTRY prWlanReqEntry = NULL;
    u32 u4BufLen = 0;
    WLAN_STATUS status = WLAN_STATUS_SUCCESS;
    P_GLUE_INFO_T prGlueInfo = NULL;

    ASSERT(prNetDev);
    ASSERT(prNdisReq);
    ASSERT(pu4OutputLen);

    if (!prNetDev || !prNdisReq || !pu4OutputLen) {
        DBGLOG(REQ, INFO,
               "priv_get_ndis(): invalid param(0x%p, 0x%p, 0x%p)\n",
               prNetDev, prNdisReq, pu4OutputLen);
        return -EINVAL;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    ASSERT(prGlueInfo);
    if (!prGlueInfo) {
        DBGLOG(REQ, INFO,
               "priv_get_ndis(): invalid prGlueInfo(0x%p, 0x%p)\n",
               prNetDev, *((P_GLUE_INFO_T *)netdev_priv(prNetDev)));
        return -EINVAL;
    }

    if (reqSearchSupportedOidEntry(prNdisReq->ndisOidCmd,
                                   &prWlanReqEntry) == false) {
        /* WARNLOG(("Query OID: 0x%08lx (unknown)\n",
         * prNdisReq->ndisOidCmd)); */
        return -EOPNOTSUPP;
    }

    if (prWlanReqEntry->pfOidQueryHandler == NULL) {
        /* WARNLOG(("Query %s: Null query handler\n",
         * prWlanReqEntry->pucOidName)); */
        return -EOPNOTSUPP;
    }

    if (prWlanReqEntry->fgQryBufLenChecking) {
        if (prNdisReq->inNdisOidlength < prWlanReqEntry->u4InfoBufLen) {
            /* Not enough room in InformationBuffer. Punt */
            /* WARNLOG(("Query %s: Buffer too short (current=%ld,
             * needed=%ld)\n", */
            /* prWlanReqEntry->pucOidName, */
            /* prNdisReq->inNdisOidlength, */
            /* prWlanReqEntry->u4InfoBufLen)); */

            *pu4OutputLen = prWlanReqEntry->u4InfoBufLen;

            status = WLAN_STATUS_INVALID_LENGTH;
            return -EINVAL;
        }
    } else {
        if (prNdisReq->inNdisOidlength >
            (sizeof(aucOidBuf) -
             OFFSET_OF(NDIS_TRANSPORT_STRUCT, ndisOidContent))) {
            DBGLOG(REQ, INFO, "exceeds length limit\n");
            return -EINVAL;
        }
    }

    if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_ONLY) {
        /* GLUE sw info only */
        status = prWlanReqEntry->pfOidQueryHandler(
            prGlueInfo, prNdisReq->ndisOidContent,
            prNdisReq->inNdisOidlength, &u4BufLen);
    } else if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_EXTENSION) {
        /* multiple sw operations */
        status = prWlanReqEntry->pfOidQueryHandler(
            prGlueInfo, prNdisReq->ndisOidContent,
            prNdisReq->inNdisOidlength, &u4BufLen);
    } else if (prWlanReqEntry->eOidMethod == ENUM_OID_DRIVER_CORE) {
        /* driver core */

        status = kalIoctl(
            prGlueInfo,
            (PFN_OID_HANDLER_FUNC)prWlanReqEntry->pfOidQueryHandler,
            prNdisReq->ndisOidContent, prNdisReq->inNdisOidlength,
            true, true, true, &u4BufLen);
    } else {
        DBGLOG(REQ, INFO,
               "priv_set_ndis(): unsupported OID method:0x%x\n",
               prWlanReqEntry->eOidMethod);
        return -EOPNOTSUPP;
    }

    *pu4OutputLen = u4BufLen;

    switch (status) {
    case WLAN_STATUS_SUCCESS:
        break;

    case WLAN_STATUS_INVALID_LENGTH:
        /* WARNLOG(("Set %s: Invalid length (current=%ld,
         * needed=%ld)\n", */
        /* prWlanReqEntry->pucOidName, */
        /* prNdisReq->inNdisOidlength, */
        /* u4BufLen)); */
        break;
    }

    if (status != WLAN_STATUS_SUCCESS) {
        return -EOPNOTSUPP;
    }

    return 0;
}

#if CFG_SUPPORT_QA_TOOL
/*----------------------------------------------------------------------------*/
/*!
 * \brief The routine handles ATE set operation.
 *
 * \param[in] pDev Net device requested.
 * \param[in] prIwReqInfo pointer to iwreq structure.
 * \param[in] prIwReqData The ioctl data structure, use the field of
 * sub-command. \param[in] pcExtra the buffer with input value.
 *
 * \retval 0 On success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EFAULT If copy from user space buffer fail.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_ate_set(IN struct net_device *prNetDev,
                 IN struct iw_request_info *prIwReqInfo,
                 IN union iwreq_data *prIwReqData, IN char *pcExtra)
{
    P_GLUE_INFO_T GlueInfo;
    s32 i4Status;
    u8 *InBuf;
    /* u8 *addr_str, *value_str; */
    u32 InBufLen;
    u32 u4SubCmd;
    /* u8 isWrite = 0;
     * u32 u4BufLen = 0;
     * P_NDIS_TRANSPORT_STRUCT prNdisReq;
     * u32 pu4IntBuf[2];
     */

    /* sanity check */
    ASSERT(prNetDev);
    ASSERT(prIwReqInfo);
    ASSERT(prIwReqData);
    ASSERT(pcExtra);

    /* init */
    DBGLOG(REQ, INFO, "priv_set_string (%s)(%d)\n",
           (u8 *)prIwReqData->data.pointer, (s32)prIwReqData->data.length);

    if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == false) {
        return -EINVAL;
    }

    GlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    if (!test_bit(GLUE_FLAG_ADAPT_RDY_BIT, &GlueInfo->ulFlag)) {
        DBGLOG(REQ, INFO, "adapter is not ready\n");
        return -EIO;
    }

    u4SubCmd = (u32)prIwReqData->data.flags;

    DBGLOG(REQ, INFO, "MT6632 : priv_ate_set u4SubCmd = %d\n", u4SubCmd);

    switch (u4SubCmd) {
    case PRIV_QACMD_SET:
        DBGLOG(REQ, INFO, "MT6632 : priv_ate_set PRIV_QACMD_SET\n");
        InBuf = aucOidBuf;
        InBufLen = prIwReqData->data.length;
        i4Status = 0;

        if (copy_from_user(InBuf, prIwReqData->data.pointer,
                           prIwReqData->data.length)) {
            return -EFAULT;
        }

        i4Status = AteCmdSetHandle(prNetDev, InBuf, InBufLen);
        break;

    default:
        return -EOPNOTSUPP;
    }
    return 0;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to search desired OID.
 *
 * \param rOid[in]               Desired NDIS_OID
 * \param ppWlanReqEntry[out]    Found registered OID entry
 *
 * \retval true: Matched OID is found
 * \retval false: No matched OID is found
 */
/*----------------------------------------------------------------------------*/
static u8 reqSearchSupportedOidEntry(IN u32 rOid,
                                     OUT P_WLAN_REQ_ENTRY *ppWlanReqEntry)
{
    s32 i, j, k;

    i = 0;
    j = NUM_SUPPORTED_OIDS - 1;

    while (i <= j) {
        k = (i + j) / 2;

        if (rOid == arWlanOidReqTable[k].rOid) {
            *ppWlanReqEntry = &arWlanOidReqTable[k];
            return true;
        } else if (rOid < arWlanOidReqTable[k].rOid) {
            j = k - 1;
        } else {
            i = k + 1;
        }
    }

    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl driver handler.
 *
 * \param[in] pDev Net device requested.
 * \param[out] pIwReq Pointer to iwreq structure.
 * \param[in] cmd Private sub-command.
 *
 * \retval 0 For success.
 * \retval -EFAULT If copy from user space buffer fail.
 * \retval -EOPNOTSUPP Parameter "cmd" not recognized.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_set_driver(IN struct net_device *prNetDev,
                    IN struct iw_request_info *prIwReqInfo,
                    IN union iwreq_data *prIwReqData, IN OUT char *pcExtra)
{
    u32 u4SubCmd = 0;
    u16 u2Cmd = 0;

    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4BytesWritten = 0;

    ASSERT(prNetDev);
    ASSERT(prIwReqData);
    if (!prNetDev || !prIwReqData) {
        DBGLOG(REQ, INFO,
               "priv_set_driver(): invalid param(0x%p, 0x%p)\n",
               prNetDev, prIwReqData);
        return -EINVAL;
    }

    u2Cmd = prIwReqInfo->cmd;
    DBGLOG(REQ, INFO, "prIwReqInfo->cmd %u\n", u2Cmd);

    u4SubCmd = (u32)prIwReqData->data.flags;
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    ASSERT(prGlueInfo);
    if (!prGlueInfo) {
        DBGLOG(REQ, INFO,
               "priv_set_driver(): invalid prGlueInfo(0x%p, 0x%p)\n",
               prNetDev, *((P_GLUE_INFO_T *)netdev_priv(prNetDev)));
        return -EINVAL;
    }

    if (!test_bit(GLUE_FLAG_ADAPT_RDY_BIT, &prGlueInfo->ulFlag)) {
        DBGLOG(REQ, INFO, "adapter is not ready\n");
        return -EIO;
    }

    /* trick,hack in ./net/wireless/wext-priv.c ioctl_private_iw_point */
    /* because the cmd number is odd (get), the input string will not be
     * copy_to_user */

    DBGLOG(REQ, INFO, "prIwReqData->data.length %u\n",
           prIwReqData->data.length);

    /* Use GET type becauase large data by iwpriv. */

    ASSERT(IW_IS_GET(u2Cmd));
    if (prIwReqData->data.length != 0) {
#if KERNEL_VERSION(5, 0, 0) <= LINUX_VERSION_CODE
        if (!access_ok(prIwReqData->data.pointer,
                       prIwReqData->data.length)) {
#else
        if (!access_ok(VERIFY_READ, prIwReqData->data.pointer,
                       prIwReqData->data.length)) {
#endif
            DBGLOG(REQ, INFO,
                   "%s access_ok Read fail written = %d\n",
                   __func__, i4BytesWritten);
            return -EFAULT;
        }

        if (prIwReqData->data.length >= IW_PRIV_BUF_SIZE) {
            return -EFAULT;
        }

        if (copy_from_user(pcExtra, prIwReqData->data.pointer,
                           prIwReqData->data.length)) {
            DBGLOG(REQ, INFO,
                   "%s copy_form_user fail written = %d\n",
                   __func__, prIwReqData->data.length);
            return -EFAULT;
        }
    }

    if (pcExtra) {
        DBGLOG(REQ, INFO, "pcExtra %s\n", pcExtra);
        /* Please check max length in rIwPrivTable */
        DBGLOG(REQ, INFO, "%s prIwReqData->data.length = %d\n",
               __func__, prIwReqData->data.length);
        i4BytesWritten = priv_driver_cmds(
            prNetDev, pcExtra, 2000  /*prIwReqData->data.length */ );
        DBGLOG(REQ, INFO, "%s i4BytesWritten = %d\n", __func__,
               i4BytesWritten);
    }

    DBGLOG(REQ, INFO, "pcExtra done\n");

    if (i4BytesWritten > 0) {
        if (i4BytesWritten > IW_PRIV_BUF_SIZE) {
            i4BytesWritten = IW_PRIV_BUF_SIZE;
        }
        prIwReqData->data.length = i4BytesWritten;  /* the iwpriv will
                                                     * use the length */
    } else if (i4BytesWritten == 0) {
        prIwReqData->data.length = i4BytesWritten;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set beacon detection function enable/disable
 * state This is mainly designed for usage under BT inquiry state (disable
 * function).
 *
 * \param[in] pvAdapter Pointer to the Adapter structure
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set
 * \param[in] u4SetBufferLen The length of the set buffer
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *   bytes read from the set buffer. If the call failed due to invalid length of
 *   the set buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA If new setting value is wrong.
 * \retval WLAN_STATUS_INVALID_LENGTH
 *
 */
/*----------------------------------------------------------------------------*/
static WLAN_STATUS reqExtSetAcpiDevicePowerState(IN P_GLUE_INFO_T prGlueInfo,
                                                 IN void *pvSetBuffer,
                                                 IN u32 u4SetBufferLen,
                                                 OUT u32 *pu4SetInfoLen)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;

    ASSERT(prGlueInfo);
    ASSERT(pvSetBuffer);
    ASSERT(pu4SetInfoLen);

    /* WIFI is enabled, when ACPI is D0 (ParamDeviceStateD0 = 1). And vice
     * versa */

    /* rStatus = wlanSetInformation(prGlueInfo->prAdapter, */
    /* wlanoidSetAcpiDevicePowerState, */
    /* pvSetBuffer, */
    /* u4SetBufferLen, */
    /* pu4SetInfoLen); */
    return rStatus;
}

#define CMD_START            "START"
#define CMD_STOP             "STOP"
#define CMD_SCAN_ACTIVE          "SCAN-ACTIVE"
#define CMD_SCAN_PASSIVE         "SCAN-PASSIVE"
#define CMD_RSSI             "RSSI"
#define CMD_LINKSPEED            "LINKSPEED"
#define CMD_RXFILTER_START       "RXFILTER-START"
#define CMD_RXFILTER_STOP        "RXFILTER-STOP"
#define CMD_RXFILTER_ADD         "RXFILTER-ADD"
#define CMD_RXFILTER_REMOVE      "RXFILTER-REMOVE"
#define CMD_BTCOEXSCAN_START         "BTCOEXSCAN-START"
#define CMD_BTCOEXSCAN_STOP      "BTCOEXSCAN-STOP"
#define CMD_BTCOEXMODE           "BTCOEXMODE"
#define CMD_SETSUSPENDOPT        "SETSUSPENDOPT"
#define CMD_SETSUSPENDMODE       "SETSUSPENDMODE"
#define CMD_P2P_DEV_ADDR         "P2P_DEV_ADDR"
#define CMD_SETFWPATH            "SETFWPATH"
#define CMD_SETBAND          "SETBAND"
#define CMD_GETBAND          "GETBAND"
#define CMD_AP_START             "AP_START"

#if CFG_SUPPORT_QA_TOOL
#define CMD_GET_RX_STATISTICS        "GET_RX_STATISTICS"
#endif
#define CMD_GET_STAT             "GET_STAT"
#define CMD_GET_BSS_STATISTICS       "GET_BSS_STATISTICS"
#define CMD_GET_STA_STATISTICS       "GET_STA_STATISTICS"
#define CMD_GET_WTBL_INFO        "GET_WTBL"
#define CMD_GET_MIB_INFO         "GET_MIB"
#if CFG_SUPPORT_LAST_SEC_MCS_INFO
#define CMD_GET_MCS_INFO         "GET_MCS_INFO"
#endif
#define CMD_GET_STA_INFO         "GET_STA"
#define CMD_GET_MAGIC_PKT_INFO       "GET_MAGIC_PKT_INFO"
#define CMD_SET_FW_LOG           "SET_FWLOG"
#define CMD_GET_QUE_INFO         "GET_QUE"
#define CMD_GET_MEM_INFO         "GET_MEM"
#define CMD_GET_HIF_INFO         "GET_HIF"
#define CMD_GET_STA_KEEP_CNT         "KEEPCOUNTER"
#define CMD_STAT_RESET_CNT       "RESETCOUNTER"
#define CMD_STAT_NOISE_SEL       "NOISESELECT"
#define CMD_STAT_GROUP_SEL       "GROUP"
#if CFG_SUPPORT_DBDC_TC6
#define CMD_CSA              "CSA"
#endif
#define CMD_SET_TXPOWER          "SET_TXPOWER"
#define CMD_COUNTRY          "COUNTRY"
#define CMD_GET_COUNTRY          "GET_COUNTRY"
#define CMD_GET_CHANNELS         "GET_CHANNELS"
#define CMD_GET_AP_CHANNELS      "GET_AP_CHANNELS"
#define CMD_P2P_SET_NOA          "P2P_SET_NOA"
#define CMD_P2P_GET_NOA          "P2P_GET_NOA"
#define CMD_P2P_SET_PS           "P2P_SET_PS"
#define CMD_SET_AP_WPS_P2P_IE        "SET_AP_WPS_P2P_IE"
#define CMD_SETROAMMODE          "SETROAMMODE"
#define CMD_MIRACAST             "MIRACAST"

#ifdef CFG_SUPPORT_ADJUST_MCC_STAY_TIME
#define CMD_MCCTIME          "MCCTIME"
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
#define CMD_SHOW_DFS_STATE       "SHOW_DFS_STATE"
#define CMD_SHOW_DFS_abd123_PARAM    "SHOW_DFS_abd123_PARAM"
#define CMD_SHOW_DFS_HELP        "SHOW_DFS_HELP"
#define CMD_SHOW_DFS_CAC_TIME        "SHOW_DFS_CAC_TIME"
#define CMD_SET_RDD_REPORT       "SET_RDD_REPORT"
//#define CMD_SET_BYPASS_CAC                 "SET_BYPASS_CAC"
#define CMD_SET_abd123_DETECT_MODE   "SET_abd123_DETECT_MODE"
#define CMD_CLEAN_DFS_abd123_PARAM   "CLEAN_DFS_abd123_PARAM"
#endif

#define CMD_PNOSSIDCLR_SET       "PNOSSIDCLR"
#define CMD_PNOSETUP_SET         "PNOSETUP "
#define CMD_PNOENABLE_SET        "PNOFORCE"
#define CMD_PNODEBUG_SET         "PNODEBUG"
#define CMD_WLS_BATCHING         "WLS_BATCHING"

#define CMD_OKC_SET_PMK          "SET_PMK"
#define CMD_OKC_ENABLE           "OKC_ENABLE"

#define CMD_SETMONITOR           "MONITOR"
#define CMD_SETBUFMODE           "BUFFER_MODE"
#define CMD_SETEEPROM_MODE       "EEPROM_MODE"

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
#define CMD_GET_CH_RANK_LIST         "GET_CH_RANK_LIST"
#define CMD_GET_CH_DIRTINESS         "GET_CH_DIRTINESS"
#endif

#define CMD_EFUSE            "EFUSE"

/* miracast related definition */
#define MIRACAST_MODE_OFF        0
#define MIRACAST_MODE_SOURCE         1
#define MIRACAST_MODE_SINK       2

#ifndef MIRACAST_AMPDU_SIZE
#define MIRACAST_AMPDU_SIZE      8
#endif

#ifndef MIRACAST_MCHAN_ALGO
#define MIRACAST_MCHAN_ALGO      1
#endif

#ifndef MIRACAST_MCHAN_BW
#define MIRACAST_MCHAN_BW        25
#endif

#define CMD_BAND_AUTO            0
#define CMD_BAND_5G          1
#define CMD_BAND_2G          2
#define CMD_BAND_ALL             3

/* Mediatek private command */
#define CMD_SET_MCR          "SET_MCR"
#define CMD_GET_MCR          "GET_MCR"
#define CMD_SET_DRV_MCR          "SET_DRV_MCR"
#define CMD_GET_DRV_MCR          "GET_DRV_MCR"
#define CMD_SET_SW_CTRL          "SET_SW_CTRL"
#define CMD_GET_SW_CTRL          "GET_SW_CTRL"
#define CMD_SET_CFG          "SET_CFG"
#define CMD_GET_CFG          "GET_CFG"
#define CMD_SET_CHIP             "SET_CHIP"
#define CMD_GET_CHIP             "GET_CHIP"
#define CMD_SET_DBG_LEVEL        "SET_DBG_LEVEL"
#define CMD_GET_DBG_LEVEL        "GET_DBG_LEVEL"
#define PRIV_CMD_SIZE            2000
#define CMD_SET_FIXED_RATE       "FixedRate"

#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT
#define CMD_SET_FIXED_MRATE      "FixedMRate"
#define CMD_SET_DUP_MPACKET      "SET_DupMPacket"
#define CMD_GET_DUP_MRACKET      "GET_DupMPacket"
#define CMD_SET_MCAST_BURST      "SET_McastBurst"
#define CMD_GET_MCAST_BURST      "GET_McastBurst"
#define CMD_SET_TXOP_TIME        "SET_TXOP"
#define CMD_GET_TXOP_TIME        "GET_TXOP"
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
#define CMD_SET_LOOK_BACK_MODE       "SET_LOOKBACK"
#define CMD_GET_LOOK_BACK_MODE       "GET_LOOKBACK"
#endif
#define CMD_SET_11MC_TYPE        "SET_11MC_TYPE"
#define CMD_GET_11MC_TYPE        "GET_11MC_TYPE"
#define CMD_SET_UNICAST_BURST        "SET_UNICAST_BURST"
#define CMD_GET_UNICAST_BURST        "GET_UNICAST_BURST"
#define CMD_SET_UNICAST_BURST_TIMEOUT    "SET_UNICAST_BURST_TIMEOUT"
#define CMD_GET_UNICAST_BURST_TIMEOUT    "GET_UNICAST_BURST_TIMEOUT"
#define CMD_SET_MRM_CLIENT       "SET_MRM_CLIENT"
#define CMD_GET_MRM_CLIENT       "GET_MRM_CLIENT"
#define CMD_SET_AUDIO_TOS        "SET_AUDIO_TOS"
#define CMD_GET_AUDIO_TOS        "GET_AUDIO_TOS"
#endif

#define CMD_GET_VERSION          "VER"
#define CMD_SET_TEST_MODE        "SET_TEST_MODE"
#define CMD_SET_TEST_CMD         "SET_TEST_CMD"
#define CMD_GET_TEST_RESULT      "GET_TEST_RESULT"
#define CMD_GET_STA_STAT         "STAT"
#define CMD_GET_STA_STAT2        "STAT2"
#define CMD_GET_STA_RX_STAT      "RX_STAT"
#define CMD_SET_ACL_POLICY       "SET_ACL_POLICY"
#define CMD_ADD_ACL_ENTRY        "ADD_ACL_ENTRY"
#define CMD_DEL_ACL_ENTRY        "DEL_ACL_ENTRY"
#define CMD_SHOW_ACL_ENTRY       "SHOW_ACL_ENTRY"
#define CMD_CLEAR_ACL_ENTRY      "CLEAR_ACL_ENTRY"
#define CMD_GET_CURR_AR_RATE         "GET_CURR_AR_RATE"
#define CMD_COEX_CONTROL         "COEX_CONTROL"
#define CMD_SET_CSI          "SET_CSI"
#define CMD_GET_CSI          "GET_CSI"

#if CFG_WOW_SUPPORT
#define CMD_WOW_START            "WOW_START"
#define CMD_SET_WOW_ENABLE       "SET_WOW_ENABLE"
#define CMD_SET_WOW_PAR          "SET_WOW_PAR"
#define CMD_SET_WOW_UDP          "SET_WOW_UDP"
#define CMD_SET_WOW_TCP          "SET_WOW_TCP"
#define CMD_GET_WOW_PORT         "GET_WOW_PORT"
#define CMD_GET_WOW_REASON       "GET_WOW_REASON"
#endif

#if CFG_STR_DHCP_RENEW_OFFLOAD
#define CMD_SET_DHCP_INFO        "SET_DHCP"
#endif

#define CMD_SET_ADV_PWS          "SET_ADV_PWS"
#define CMD_SET_MDTIM            "SET_MDTIM"
#define CMD_SET_LISTEN_DTIM_INTERVAL     "SET_LISTEN_DTIM_INTERVAL"

#define CMD_SET_DBDC             "SET_DBDC"

#define CMD_SET_P2P_PS           "SET_P2P_PS"
#define CMD_SET_P2P_NOA          "SET_P2P_NOA"

#define CMD_GET_CNM_INFO         "GET_CNM"
#define CMD_GET_DSLP_CNT         "GET_DSLEEP_CNT"

#ifdef CFG_SUPPORT_ANT_DIV
#define CMD_SET_ANT_DIV          "ANT_DIV_SET"
#define CMD_GET_ANT_DIV          "ANT_DIV_GET"
#define CMD_DETC_ANT_DIV         "ANT_DIV_DETC"
#define CMD_SWH_ANT_DIV          "ANT_DIV_SWH"

#define CMD_SET_ANT_DIV_ARG_NUM      2
#define CMD_GET_ANT_DIV_ARG_NUM      1
#define CMD_DETC_ANT_DIV_ARG_NUM     1
#define CMD_SWH_ANT_DIV_ARG_NUM      1

#endif

#define CMD_SEND_BEACONTIMEOUT       "SEND_BEACONTIMEOUT"

#define CMD_GET_DISCONNECT_REASON    "GET_DISCONNECT_REASON"
#define DISCONNECT_REASON_BASE       1000

static u8 g_ucMiracastMode = MIRACAST_MODE_OFF;

typedef struct cmd_tlv {
    char prefix;
    char version;
    char subver;
    char reserved;
} cmd_tlv_t;

typedef struct priv_driver_cmd_s {
    char *buf;
    int used_len;
    int total_len;
} priv_driver_cmd_t;

#ifdef CFG_ANDROID_AOSP_PRIV_CMD
struct android_wifi_priv_cmd {
    char *buf;
    int used_len;
    int total_len;
};
#endif

int priv_driver_get_dbg_level(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 u4DbgIdx = 0, u4DbgMask = 0;
    u8 fgIsCmdAccept = false;
    s32 u4Ret = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 2) {
        /* u4DbgIdx = kalStrtoul(apcArgv[1], NULL, 0); */
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4DbgIdx);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }
        if (wlanGetDebugLevel(u4DbgIdx, &u4DbgMask) ==
            WLAN_STATUS_SUCCESS) {
            fgIsCmdAccept = true;
            i4BytesWritten = snprintf(
                pcCommand, i4TotalLen,
                "Get DBG module[%lu] log level => [0x%02x]!",
                u4DbgIdx, (u8)u4DbgMask);
        }
    }

    if (!fgIsCmdAccept) {
        i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                                  "Get DBG module log level failed!");
    }

    return i4BytesWritten;
}

#if CFG_SUPPORT_QA_TOOL
#if CFG_SUPPORT_BUFFER_MODE
static int priv_driver_set_eeprom_mode(IN struct net_device *prNetDev,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    s32 i4Argc = 0;
    s32 i4BytesWritten = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 arg = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    if (i4Argc >= 2) {
        rStatus = kalkStrtou32(apcArgv[1], 0, &arg);
        if (rStatus) {
            DBGLOG(REQ, LOUD, "parse apcArgv error rStatus=%d\n",
                   rStatus);
            return -1;
        }

        rStatus = priv_set_eeprom_mode(arg);
        if (rStatus) {
            DBGLOG(REQ, LOUD, "priv_set_eeprom_mode rStatus=%d\n",
                   rStatus);
            return -1;
        }

        i4BytesWritten = snprintf(
            pcCommand, i4TotalLen, "Switch eeprom source as %s",
            (arg == EFUSE_MODE) ? "Efuse" : "Buffer Bin");
    }

    return i4BytesWritten;
}
static int priv_driver_set_efuse_buffer_mode(IN struct net_device *prNetDev,
                                             IN char *pcCommand,
                                             IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4Argc = 0;
    s32 i4BytesWritten = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    PARAM_CUSTOM_EFUSE_BUFFER_MODE_T *prSetEfuseBufModeInfo = NULL;
#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 0)
    BIN_CONTENT_T *pBinContent;
    int i = 0;
#endif
    u8 *pucConfigBuf = NULL;
    u32 u4ConfigReadLen;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    pucConfigBuf = (u8 *)kalMemAlloc(2048, VIR_MEM_TYPE);

    if (!pucConfigBuf) {
        DBGLOG(INIT, INFO, "allocate memory for pucConfigBuf failed\n");
        i4BytesWritten = -1;
        goto out;
    }
    kalMemZero(pucConfigBuf, 2048);
    u4ConfigReadLen = 0;

    if (kalReadToFile("/MT6632_eFuse_usage_table.xlsm.bin", pucConfigBuf,
                      2048, &u4ConfigReadLen) == 0) {
        /* ToDo:: Nothing */
    } else {
        DBGLOG(INIT, INFO, "can't find file\n");
        i4BytesWritten = -1;
        goto out;
    }

    /* pucConfigBuf */
    prSetEfuseBufModeInfo = (PARAM_CUSTOM_EFUSE_BUFFER_MODE_T *)kalMemAlloc(
        sizeof(PARAM_CUSTOM_EFUSE_BUFFER_MODE_T), VIR_MEM_TYPE);
    if (prSetEfuseBufModeInfo == NULL) {
        DBGLOG(INIT, INFO,
               "allocate memory for prSetEfuseBufModeInfo failed\n");
        i4BytesWritten = -1;
        goto out;
    }
    kalMemZero(prSetEfuseBufModeInfo,
               sizeof(PARAM_CUSTOM_EFUSE_BUFFER_MODE_T));

    prSetEfuseBufModeInfo->ucSourceMode = 1;
    prSetEfuseBufModeInfo->ucCount = (u8)EFUSE_CONTENT_SIZE;

#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 0)
    pBinContent = (BIN_CONTENT_T *)prSetEfuseBufModeInfo->aBinContent;
    for (i = 0; i < EFUSE_CONTENT_SIZE; i++) {
        pBinContent->u2Addr = i;
        pBinContent->ucValue = *(pucConfigBuf + i);

        pBinContent++;
    }

    for (i = 0; i < 20; i++)
        DBGLOG(INIT, INFO, "%x\n",
               prSetEfuseBufModeInfo->aBinContent[i].ucValue);
#endif

    rStatus = kalIoctl(prGlueInfo, wlanoidSetEfusBufferMode,
                       prSetEfuseBufModeInfo,
                       sizeof(PARAM_CUSTOM_EFUSE_BUFFER_MODE_T), false,
                       false, true, &u4BufLen);

    i4BytesWritten =
        snprintf(pcCommand, i4TotalLen, "set buffer mode %s",
                 (rStatus == WLAN_STATUS_SUCCESS) ? "success" : "fail");
out:
    if (pucConfigBuf) {
        kalMemFree(pucConfigBuf, VIR_MEM_TYPE, 2048);
    }

    if (prSetEfuseBufModeInfo) {
        kalMemFree(prSetEfuseBufModeInfo, VIR_MEM_TYPE,
                   sizeof(PARAM_CUSTOM_EFUSE_BUFFER_MODE_T));
    }

    return i4BytesWritten;
}
#endif

static int priv_driver_get_rx_statistics(IN struct net_device *prNetDev,
                                         IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 u4Ret = 0;
    PARAM_CUSTOM_ACCESS_RX_STAT rRxStatisticsTest;

    kalMemZero(&rRxStatisticsTest, sizeof(PARAM_CUSTOM_ACCESS_RX_STAT));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    DBGLOG(INIT, ERROR, "MT6632 : priv_driver_get_rx_statistics\n");

    if (i4Argc >= 2) {
        u4Ret = kalkStrtou32(apcArgv[1], 0,
                             &(rRxStatisticsTest.u4SeqNum));
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        rRxStatisticsTest.u4TotalNum = sizeof(PARAM_RX_STAT_T) / 4;

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryRxStatistics,
                           &rRxStatisticsTest,
                           sizeof(rRxStatisticsTest), true, true, true,
                           &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }
    }

    return i4BytesWritten;
}
#endif

#if CFG_SUPPORT_MSP

static int priv_driver_get_sta_statistics(IN struct net_device *prNetDev,
                                          IN char *pcCommand,
                                          IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 i4ArgNum = 3;
    PARAM_GET_STA_STATISTICS rQueryStaStatistics;
    PARAM_RSSI rRssi;
    u16 u2LinkSpeed;
    u32 u4Per;

    ASSERT(prNetDev);

    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    kalMemZero(&rQueryStaStatistics, sizeof(rQueryStaStatistics));
    rQueryStaStatistics.ucReadClear = true;

    if (i4Argc >= i4ArgNum) {
        if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT,
                     strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
            if (wlanHwAddrToBin(
                    apcArgv[2],
                    &rQueryStaStatistics.aucMacAddr[0]) < 0) {
                DBGLOG(REQ, WARN,
                       "%s: MAC addr Argv[2] convert failed\n",
                       __func__);
                return -1;
            }
            rQueryStaStatistics.ucReadClear = false;
        } else if (strnicmp(apcArgv[2], CMD_GET_STA_KEEP_CNT,
                            strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
            if (wlanHwAddrToBin(
                    apcArgv[1],
                    &rQueryStaStatistics.aucMacAddr[0]) < 0) {
                DBGLOG(REQ, WARN,
                       "%s: MAC addr Argv[1] convert failed\n",
                       __func__);
                return -1;
            }
            rQueryStaStatistics.ucReadClear = false;
        }
    } else {
        /* Get AIS AP address for no argument */
        if (prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP) {
            COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr,
                          prGlueInfo->prAdapter->prAisBssInfo
                          ->prStaRecOfAP->aucMacAddr);
            DBGLOG(RSN, INFO, "use ais ap " MACSTR "\n",
                   MAC2STR(prGlueInfo->prAdapter->prAisBssInfo
                           ->prStaRecOfAP->aucMacAddr));
        } else {
            DBGLOG(RSN, INFO, "not connect to ais ap %x\n",
                   prGlueInfo->prAdapter->prAisBssInfo
                   ->prStaRecOfAP);
            i4BytesWritten = kalScnprintf(pcCommand, i4TotalLen,
                                          "%s",
                                          "\n\nNo STA Stat:\n");
            return i4BytesWritten;
        }

        if (i4Argc == 2) {
            if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT,
                         strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
                rQueryStaStatistics.ucReadClear = false;
            }
        }
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
                       &rQueryStaStatistics, sizeof(rQueryStaStatistics),
                       true, false, true, &u4BufLen);

    if (rStatus == WLAN_STATUS_SUCCESS) {
        rRssi = RCPI_TO_dBm(rQueryStaStatistics.ucRcpi);
        u2LinkSpeed = rQueryStaStatistics.u2LinkSpeed == 0 ?
                      0 :
                      rQueryStaStatistics.u2LinkSpeed / 2;

        i4BytesWritten = kalScnprintf(pcCommand, i4TotalLen, "%s",
                                      "\n\nSTA Stat:\n");

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "Tx total cnt = %ld\n",
            rQueryStaStatistics.u4TransmitCount);

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "Tx success = %ld\n",
            rQueryStaStatistics.u4TransmitCount -
            rQueryStaStatistics.u4TransmitFailCount);

        u4Per = rQueryStaStatistics.u4TransmitCount == 0 ?
                0 :
                (1000 *
                 (rQueryStaStatistics.u4TransmitFailCount)) /
                rQueryStaStatistics.u4TransmitCount;
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "Tx fail count = %ld, PER=%ld.%1ld%%\n",
            rQueryStaStatistics.u4TransmitFailCount, u4Per / 10,
            u4Per % 10);

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "RSSI = %d\n", rRssi);
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "LinkSpeed = %d\n", u2LinkSpeed);
    } else {
        i4BytesWritten = kalScnprintf(pcCommand, i4TotalLen, "%s",
                                      "\n\nNo STA Stat:\n");
    }

    return i4BytesWritten;
}

static int priv_driver_get_bss_statistics(IN struct net_device *prNetDev,
                                          IN char *pcCommand,
                                          IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus;
    PARAM_MAC_ADDRESS arBssid;
    u32 u4BufLen;
    s32 i4Rssi;
    PARAM_GET_BSS_STATISTICS rQueryBssStatistics;
    P_NETDEV_PRIVATE_GLUE_INFO prNetDevPrivate =
        (P_NETDEV_PRIVATE_GLUE_INFO)NULL;
    u8 ucBssIndex;
    s32 i4BytesWritten = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    ASSERT(prGlueInfo);

    kalMemZero(arBssid, MAC_ADDR_LEN);
    wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQueryBssid,
                         &arBssid[0], sizeof(arBssid), &u4BufLen);

    /* 2. fill RSSI */
    if (prGlueInfo->eParamMediaStateIndicated !=
        PARAM_MEDIA_STATE_CONNECTED) {
        /* not connected */
        DBGLOG(REQ, WARN, "not yet connected\n");
        return WLAN_STATUS_SUCCESS;
    }
    rStatus = kalIoctl(prGlueInfo, wlanoidQueryRssi, &i4Rssi,
                       sizeof(i4Rssi), true, false, false, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "unable to retrieve rssi\n");
    }

    /* 3 get per-BSS link statistics */
    if (rStatus == WLAN_STATUS_SUCCESS) {
        /* get Bss Index from ndev */
        prNetDevPrivate =
            (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(prNetDev);
        ASSERT(prNetDevPrivate->prGlueInfo == prGlueInfo);
        ucBssIndex = prNetDevPrivate->ucBssIdx;

        kalMemZero(&rQueryBssStatistics, sizeof(rQueryBssStatistics));
        rQueryBssStatistics.ucBssIndex = ucBssIndex;

        rQueryBssStatistics.ucReadClear = true;

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryBssStatistics,
                           &rQueryBssStatistics,
                           sizeof(rQueryBssStatistics), true, false,
                           true, &u4BufLen);

        if (rStatus == WLAN_STATUS_SUCCESS) {
            i4BytesWritten = kalScnprintf(pcCommand, i4TotalLen,
                                          "%s", "\n\nStat:\n");
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten, "%s",
                             "CurrentTemperature = -\n");
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "Tx success = %ld\n",
                rQueryBssStatistics.u4TransmitCount -
                rQueryBssStatistics.u4TransmitFailCount);
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "Tx fail count = %ld\n",
                rQueryBssStatistics.u4TransmitFailCount);
        }
    } else {
        DBGLOG(REQ, WARN,
               "unable to retrieve per-BSS link statistics\n");
    }

    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

    return i4BytesWritten;
}

#define HW_TX_RATE_TO_MODE(_x)         (((_x) & (0x7 << 6)) >> 6)
#define HW_TX_RATE_TO_MCS(_x, _mode)       ((_x) & (0x3f))
#define HW_TX_RATE_TO_NSS(_x)          (((_x) & (0x3 << 9)) >> 9)
#define HW_TX_RATE_TO_STBC(_x)         (((_x) & (0x1 << 11)) >> 11)

#define TX_VECTOR_GET_TX_RATE(_txv)    (((_txv)->u4TxVector1) & BITS(0, 6))
#define TX_VECTOR_GET_TX_LDPC(_txv)    ((((_txv)->u4TxVector1) >> 7) & BIT(0))
#define TX_VECTOR_GET_TX_STBC(_txv)    ((((_txv)->u4TxVector1) >> 8) & BITS(0, \
                                                                            1))
#define TX_VECTOR_GET_TX_FRMODE(_txv) \
    ((((_txv)->u4TxVector1) >> 10) & BITS(0, 1))
#define TX_VECTOR_GET_TX_MODE(_txv)    ((((_txv)->u4TxVector1) >> \
                                         12) & BITS(0,            \
                                                    2))
#define TX_VECTOR_GET_TX_NSTS(_txv)    ((((_txv)->u4TxVector1) >> \
                                         21) & BITS(0,            \
                                                    1))
#define TX_VECTOR_GET_TX_PWR(_txv)     ((((_txv)->u4TxVector1) >> \
                                         24) & BITS(0,            \
                                                    6))
#define TX_VECTOR_GET_BF_EN(_txv)      ((((_txv)->u4TxVector2) >> \
                                         31) & BIT(0))
#define TX_VECTOR_GET_DYN_BW(_txv)     ((((_txv)->u4TxVector4) >> \
                                         31) & BIT(0))
#define TX_VECTOR_GET_NO_SOUNDING(_txv)    ((((_txv)->u4TxVector4) >> \
                                             28) & BIT(0))
#define TX_VECTOR_GET_TX_SGI(_txv)     ((((_txv)->u4TxVector4) >> \
                                         27) & BIT(0))

#define TX_RATE_MODE_CCK      0
#define TX_RATE_MODE_OFDM     1
#define TX_RATE_MODE_HTMIX    2
#define TX_RATE_MODE_HTGF     3
#define TX_RATE_MODE_VHT      4
#define MAX_TX_MODE       5

static char *HW_TX_MODE_STR[] = { "CCK", "OFDM", "MM", "GF", "VHT", "N/A" };
static char *HW_TX_RATE_CCK_STR[] = { "1M", "2M", "5.5M", "11M", "N/A" };
static char *HW_TX_RATE_OFDM_STR[] = { "6M", "9M", "12M", "18M", "24M",
                                       "36M", "48M", "54M", "N/A" };
static char *HW_TX_RATE_BW[] = { "BW20", "BW40", "BW80", "BW160/BW8080",
                                 "N/A" };
enum {
    RATE_TBL_B = 0,
    RATE_TBL_G,
    RATE_TBL_N,
    RATE_TBL_N_2SS,
    RATE_TBL_AC,
    RATE_TBL_AC_2SS,
    RATE_TBL_MAX
};

static char *RATE_TBLE[] = {
    [RATE_TBL_B] = "B", [RATE_TBL_G] = "G",
    [RATE_TBL_N] = "N", [RATE_TBL_N_2SS] = "N_2SS",
    [RATE_TBL_AC] = "AC", [RATE_TBL_AC_2SS] = "AC_2SS",
    [RATE_TBL_MAX] = "N/A"
};

#define BW_20      0
#define BW_40      1
#define BW_80      2
#define BW_160     3
#define BW_10      4
#define BW_5       6
#define BW_8080    7
#define BW_ALL     0xFF

static char *hw_rate_ofdm_str(u16 ofdm_idx)
{
    switch (ofdm_idx) {
    case 11:  /* 6M */
        return HW_TX_RATE_OFDM_STR[0];

    case 15:  /* 9M */
        return HW_TX_RATE_OFDM_STR[1];

    case 10:  /* 12M */
        return HW_TX_RATE_OFDM_STR[2];

    case 14:  /* 18M */
        return HW_TX_RATE_OFDM_STR[3];

    case 9:  /* 24M */
        return HW_TX_RATE_OFDM_STR[4];

    case 13:  /* 36M */
        return HW_TX_RATE_OFDM_STR[5];

    case 8:  /* 48M */
        return HW_TX_RATE_OFDM_STR[6];

    case 12:  /* 54M */
        return HW_TX_RATE_OFDM_STR[7];

    default:
        return HW_TX_RATE_OFDM_STR[8];
    }
}

static u8 priv_driver_get_sgi_info(IN P_PARAM_PEER_CAP_T prWtblPeerCap)
{
    if (!prWtblPeerCap) {
        return false;
    }

    switch (prWtblPeerCap->ucFrequencyCapability) {
    case BW_20:
        return prWtblPeerCap->fgG2;

    case BW_40:
        return prWtblPeerCap->fgG4;

    case BW_80:
        return prWtblPeerCap->fgG8;

    case BW_160:
        return prWtblPeerCap->fgG16;

    default:
        return false;
    }
}

static u8 priv_driver_get_ldpc_info(IN P_PARAM_TX_CONFIG_T prWtblTxConfig)
{
    if (!prWtblTxConfig) {
        return false;
    }

    if (prWtblTxConfig->fgIsVHT) {
        return prWtblTxConfig->fgVhtLDPC;
    }else{
        return prWtblTxConfig->fgLDPC;
    }
}

s32 priv_driver_rate_to_string(IN char *pcCommand, IN int i4TotalLen, u8 TxRx,
                               P_PARAM_HW_WLAN_INFO_T prHwWlanInfo)
{
    u8 i, txmode, rate, stbc;
    u8 nss;
    s32 i4BytesWritten = 0;

    for (i = 0; i < AUTO_RATE_NUM; i++) {
        txmode = HW_TX_RATE_TO_MODE(
            prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);
        if (txmode >= MAX_TX_MODE) {
            txmode = MAX_TX_MODE;
        }
        rate = HW_TX_RATE_TO_MCS(
            prHwWlanInfo->rWtblRateInfo.au2RateCode[i], txmode);
        nss = HW_TX_RATE_TO_NSS(
            prHwWlanInfo->rWtblRateInfo.au2RateCode[i]) +
              1;
        stbc = HW_TX_RATE_TO_STBC(
            prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "\tRate index[%d] ", i);

        if (prHwWlanInfo->rWtblRateInfo.ucRateIdx == i) {
            if (TxRx == 0) {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten, "%s",
                    "[Last RX Rate] ");
            } else {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten, "%s",
                    "[Last TX Rate] ");
            }
        } else {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%s", " ");
        }

        if (txmode == TX_RATE_MODE_CCK) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%s, ",
                rate < 4 ? HW_TX_RATE_CCK_STR[rate] :
                HW_TX_RATE_CCK_STR[4]);
        } else if (txmode == TX_RATE_MODE_OFDM) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "%s, ", hw_rate_ofdm_str(rate));
        } else {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "NSS%d_MCS%d, ", nss, rate);
        }

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s, ",
            HW_TX_RATE_BW[prHwWlanInfo->rWtblPeerCap
                          .ucFrequencyCapability]);

        if (txmode == TX_RATE_MODE_CCK) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "%s, ", rate < 4 ? "LP" : "SP");
        } else if (txmode == TX_RATE_MODE_OFDM) {
            ;
        } else {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%s, ",
                priv_driver_get_sgi_info(
                    &prHwWlanInfo->rWtblPeerCap) == 0 ?
                "LGI" :
                "SGI");
        }
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s%s %s\n", HW_TX_MODE_STR[txmode],
            stbc ? "STBC" : " ",
            priv_driver_get_ldpc_info(
                &prHwWlanInfo->rWtblTxConfig) == 0 ?
            "BCC" :
            "LDPC");
    }

    return i4BytesWritten;
}

static s32
priv_driver_dump_helper_wtbl_info(IN char *pcCommand, IN int i4TotalLen,
                                  P_PARAM_HW_WLAN_INFO_T prHwWlanInfo)
{
    u8 i;
    s32 i4BytesWritten = 0;

    ASSERT(pcCommand);

    i4BytesWritten =
        kalScnprintf(pcCommand, i4TotalLen, "%s", "\n\nwtbl:\n");
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "Dump WTBL info of WLAN_IDX = %d\n",
                                   prHwWlanInfo->u4Index);
    i4BytesWritten +=
        kalScnprintf(pcCommand + i4BytesWritten,
                     i4TotalLen - i4BytesWritten, "\tAddr=" MACSTR "\n",
                     MAC2STR(prHwWlanInfo->rWtblTxConfig.aucPA));
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\tMUAR_Idx = %d\n",
                                   prHwWlanInfo->rWtblSecConfig.ucMUARIdx);
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\trc_a1/rc_a2:%d/%d\n",
                                   prHwWlanInfo->rWtblSecConfig.fgRCA1,
                                   prHwWlanInfo->rWtblSecConfig.fgRCA2);
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\tKID:%d/RCID:%d/RKV:%d/RV:%d/IKV:%d/WPI_FLAG:%d\n",
        prHwWlanInfo->rWtblSecConfig.ucKeyID,
        prHwWlanInfo->rWtblSecConfig.fgRCID,
        prHwWlanInfo->rWtblSecConfig.fgRKV,
        prHwWlanInfo->rWtblSecConfig.fgRV,
        prHwWlanInfo->rWtblSecConfig.fgIKV,
        prHwWlanInfo->rWtblSecConfig.fgEvenPN);
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten, "%s",
                                   "\tGID_SU:NA");
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\tsw/DIS_RHTR:%d/%d\n", prHwWlanInfo->rWtblTxConfig.fgSW,
        prHwWlanInfo->rWtblTxConfig.fgDisRxHdrTran);
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\tHT/VHT/HT-LDPC/VHT-LDPC/DYN_BW/MMSS:%d/%d/%d/%d/%d/%d\n",
        prHwWlanInfo->rWtblTxConfig.fgIsHT,
        prHwWlanInfo->rWtblTxConfig.fgIsVHT,
        prHwWlanInfo->rWtblTxConfig.fgLDPC,
        prHwWlanInfo->rWtblTxConfig.fgVhtLDPC,
        prHwWlanInfo->rWtblTxConfig.fgDynBw,
        prHwWlanInfo->rWtblPeerCap.ucMMSS);
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\tFCAP/G2/G4/G8/G16/CBRN:%d/%d/%d/%d/%d/%d\n",
        prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability,
        prHwWlanInfo->rWtblPeerCap.fgG2,
        prHwWlanInfo->rWtblPeerCap.fgG4,
        prHwWlanInfo->rWtblPeerCap.fgG8,
        prHwWlanInfo->rWtblPeerCap.fgG16,
        prHwWlanInfo->rWtblPeerCap.ucChangeBWAfterRateN);
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten,
        i4TotalLen - i4BytesWritten,
        "\tHT-TxBF(tibf/tebf):%d/%d, VHT-TxBF(tibf/tebf):%d/%d, PFMU_IDX=%d\n",
        prHwWlanInfo->rWtblTxConfig.fgTIBF,
        prHwWlanInfo->rWtblTxConfig.fgTEBF,
        prHwWlanInfo->rWtblTxConfig.fgVhtTIBF,
        prHwWlanInfo->rWtblTxConfig.fgVhtTEBF,
        prHwWlanInfo->rWtblTxConfig.ucPFMUIdx);
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten, "%s",
                                   "\tSPE_IDX=NA\n");
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\tBA Enable:0x%x, BAFail Enable:%d\n",
                                   prHwWlanInfo->rWtblBaConfig.ucBaEn,
                                   prHwWlanInfo->rWtblTxConfig.fgBAFEn);
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\tQoS Enable:%d\n",
                                   prHwWlanInfo->rWtblTxConfig.fgIsQoS);
    if (prHwWlanInfo->rWtblTxConfig.fgIsQoS) {
        for (i = 0; i < 8; i += 2) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "\t\tBA WinSize: TID 0 - %d, TID 1 - %d\n",
                (prHwWlanInfo->rWtblBaConfig.u4BaWinSize >>
                 (i * 3)) &
                BITS(0, 2),
                (prHwWlanInfo->rWtblBaConfig.u4BaWinSize >>
                 ((i + 1) * 3)) &
                BITS(0, 2));
        }
    }

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\tpartial_aid:%d\n", prHwWlanInfo->rWtblTxConfig.u2PartialAID);
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\twpi_even:%d\n",
                                   prHwWlanInfo->rWtblSecConfig.fgEvenPN);
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\tAAD_OM/CipherSuit:%d/%d\n",
        prHwWlanInfo->rWtblTxConfig.fgAADOM,
        prHwWlanInfo->rWtblSecConfig.ucCipherSuit);
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\taf:%d\n", prHwWlanInfo->rWtblPeerCap.ucAmpduFactor);
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\trdg_ba:%d/rdg capability:%d\n",
                                   prHwWlanInfo->rWtblTxConfig.fgRdgBA,
                                   prHwWlanInfo->rWtblTxConfig.fgRDG);
    i4BytesWritten +=
        kalScnprintf(pcCommand + i4BytesWritten,
                     i4TotalLen - i4BytesWritten, "\tcipher_suit:%d\n",
                     prHwWlanInfo->rWtblSecConfig.ucCipherSuit);
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\tFromDS:%d\n",
                                   prHwWlanInfo->rWtblTxConfig.fgIsFromDS);
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\tToDS:%d\n",
                                   prHwWlanInfo->rWtblTxConfig.fgIsToDS);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\tRSSI = %d %d %d %d\n",
        RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi0),
        RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi1),
        RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi2),
        RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi3));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten, "%s",
                                   "\tRate Info\n");

    i4BytesWritten += priv_driver_rate_to_string(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten, 1,
        prHwWlanInfo);

    return i4BytesWritten;
}

static int priv_driver_get_wtbl_info(IN struct net_device *prNetDev,
                                     IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s32 u4Ret = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    P_PARAM_HW_WLAN_INFO_T prHwWlanInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    /* DBGLOG(RSN, INFO, "MT6632 : priv_driver_get_wtbl_info\n"); */

    prHwWlanInfo = (P_PARAM_HW_WLAN_INFO_T)kalMemAlloc(
        sizeof(PARAM_HW_WLAN_INFO_T), VIR_MEM_TYPE);
    if (!prHwWlanInfo) {
        return -1;
    }

    kalMemZero(prHwWlanInfo, sizeof(PARAM_HW_WLAN_INFO_T));

    if (i4Argc >= 2) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &prHwWlanInfo->u4Index);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        DBGLOG(REQ, INFO, "MT6632 : index = %d\n",
               prHwWlanInfo->u4Index);

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo,
                           prHwWlanInfo, sizeof(PARAM_HW_WLAN_INFO_T),
                           true, true, true, &u4BufLen);

        DBGLOG(REQ, INFO, "rStatus %u u4BufLen = %d\n", rStatus,
               u4BufLen);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
                       sizeof(PARAM_HW_WLAN_INFO_T));
            return -1;
        }
        i4BytesWritten = priv_driver_dump_helper_wtbl_info(
            pcCommand, i4TotalLen, prHwWlanInfo);
    }

    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

    kalMemFree(prHwWlanInfo, VIR_MEM_TYPE, sizeof(PARAM_HW_WLAN_INFO_T));

    return i4BytesWritten;
}

static int priv_driver_get_sta_info(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u8 aucMacAddr[MAC_ADDR_LEN];
    u8 ucWlanIndex;
    u8 *pucMacAddr = NULL;
    P_PARAM_HW_WLAN_INFO_T prHwWlanInfo = NULL;
    PARAM_GET_STA_STATISTICS rQueryStaStatistics;
    PARAM_RSSI rRssi;
    u16 u2LinkSpeed;
    u32 u4Per;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    kalMemZero(&rQueryStaStatistics, sizeof(rQueryStaStatistics));
    rQueryStaStatistics.ucReadClear = true;

    /* DBGLOG(RSN, INFO, "MT6632 : priv_driver_get_sta_info\n"); */
    if (i4Argc >= 3) {
        if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT,
                     strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
            if (wlanHwAddrToBin(apcArgv[2], &aucMacAddr[0]) < 0) {
                DBGLOG(REQ, WARN,
                       "%s: MAC addr Argv[2] convert failed\n",
                       __func__);
                return -1;
            }
            rQueryStaStatistics.ucReadClear = false;
        } else if (strnicmp(apcArgv[2], CMD_GET_STA_KEEP_CNT,
                            strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
            if (wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]) < 0) {
                DBGLOG(REQ, WARN,
                       "%s: MAC addr Argv[1] convert failed\n",
                       __func__);
                return -1;
            }
            rQueryStaStatistics.ucReadClear = false;
        }

        if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter,
                                     &aucMacAddr[0], &ucWlanIndex)) {
            return i4BytesWritten;
        }
    } else {
        /* Get AIS AP address for no argument */
        if (prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP) {
            ucWlanIndex = prGlueInfo->prAdapter->prAisBssInfo
                          ->prStaRecOfAP->ucWlanIndex;
        } else if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter, NULL,
                                            &ucWlanIndex)) {  /* try get
                                                               * a peer
                                                               */
            return i4BytesWritten;
        }

        if (i4Argc == 2) {
            if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT,
                         strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
                rQueryStaStatistics.ucReadClear = false;
            }
        }
    }

    prHwWlanInfo = (P_PARAM_HW_WLAN_INFO_T)kalMemAlloc(
        sizeof(PARAM_HW_WLAN_INFO_T), VIR_MEM_TYPE);

    if (!prHwWlanInfo) {
        DBGLOG(REQ, ERROR, "alloc memory for prHwWlanInfo failed!\n");
        i4BytesWritten = -1;
        goto out;
    }
    kalMemZero(prHwWlanInfo, sizeof(PARAM_HW_WLAN_INFO_T));
    prHwWlanInfo->u4Index = ucWlanIndex;

    DBGLOG(REQ, INFO, "MT6632 : index = %d i4TotalLen = %d\n",
           prHwWlanInfo->u4Index, i4TotalLen);

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo, prHwWlanInfo,
                       sizeof(PARAM_HW_WLAN_INFO_T), true, true, true,
                       &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "query prHwWlanInfo failed!\n");
        goto out;
    }

    i4BytesWritten = priv_driver_dump_helper_wtbl_info(
        pcCommand, i4TotalLen, prHwWlanInfo);

    pucMacAddr =
        wlanGetStaAddrByWlanIdx(prGlueInfo->prAdapter, ucWlanIndex);
    if (pucMacAddr) {
        COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr, pucMacAddr);
        /* i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
         * i4TotalLen - i4BytesWritten,
         * "\tAddr="MACSTR"\n",
         * MAC2STR(rQueryStaStatistics.aucMacAddr));
         */

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
                           &rQueryStaStatistics,
                           sizeof(rQueryStaStatistics), true, false,
                           true, &u4BufLen);

        if (rStatus == WLAN_STATUS_SUCCESS) {
            rRssi = RCPI_TO_dBm(rQueryStaStatistics.ucRcpi);
            u2LinkSpeed =
                rQueryStaStatistics.u2LinkSpeed == 0 ?
                0 :
                rQueryStaStatistics.u2LinkSpeed / 2;

            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten, "%s",
                             "\n\nSTA Stat:\n");

            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "Tx total cnt = %ld\n",
                rQueryStaStatistics.u4TransmitCount);

            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "Tx success = %ld\n",
                rQueryStaStatistics.u4TransmitCount -
                rQueryStaStatistics.u4TransmitFailCount);

            u4Per = rQueryStaStatistics.u4TransmitCount == 0 ?
                    0 :
                    (1000 *
                     (rQueryStaStatistics
                      .u4TransmitFailCount)) /
                    rQueryStaStatistics
                    .u4TransmitCount;
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "Tx fail count = %ld, PER=%ld.%1ld%%\n",
                rQueryStaStatistics.u4TransmitFailCount,
                u4Per / 10, u4Per % 10);

            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "RSSI = %d\n", rRssi);
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "LinkSpeed = %d\n", u2LinkSpeed);
        }
    }
    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

out:
    if (prHwWlanInfo) {
        kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
                   sizeof(PARAM_HW_WLAN_INFO_T));
    }

    return i4BytesWritten;
}

static int priv_driver_get_mib_info(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    u8 i;
    u32 u4Per;
    s32 u4Ret = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    P_PARAM_HW_MIB_INFO_T prHwMibInfo;
    P_RX_CTRL_T prRxCtrl;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    prRxCtrl = &prGlueInfo->prAdapter->rRxCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    DBGLOG(REQ, INFO, "MT6632 : priv_driver_get_mib_info\n");

    prHwMibInfo = (P_PARAM_HW_MIB_INFO_T)kalMemAlloc(
        sizeof(PARAM_HW_MIB_INFO_T), VIR_MEM_TYPE);
    if (!prHwMibInfo) {
        return -1;
    }

    kalMemZero(prHwMibInfo, sizeof(PARAM_HW_MIB_INFO_T));

    if (i4Argc == 1) {
        prHwMibInfo->u4Index = 0;
    }

    if (i4Argc >= 2) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &prHwMibInfo->u4Index);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }
    }

    DBGLOG(REQ, INFO, "MT6632 : index = %d\n", prHwMibInfo->u4Index);

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryMibInfo, prHwMibInfo,
                       sizeof(PARAM_HW_MIB_INFO_T), true, true, true,
                       &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        kalMemFree(prHwMibInfo, VIR_MEM_TYPE,
                   sizeof(PARAM_HW_MIB_INFO_T));
        return -1;
    }

    if (prHwMibInfo->u4Index < 2) {
        i4BytesWritten = kalScnprintf(pcCommand, i4TotalLen, "%s",
                                      "\n\nmib state:\n");
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "Dump MIB info of IDX = %d\n",
                                       prHwMibInfo->u4Index);
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%s",
                                       "===Rx Related Counters===\n");
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx with CRC=%ld\n",
            prHwMibInfo->rHwMibCnt.u4RxFcsErrCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx drop due to out of resource=%ld\n",
            prHwMibInfo->rHwMibCnt.u4RxFifoFullCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx Mpdu=%ld\n", prHwMibInfo->rHwMibCnt.u4RxMpduCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx AMpdu=%ld\n",
            prHwMibInfo->rHwMibCnt.u4RxAMPDUCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx PF Drop=%ld\n",
            prHwMibInfo->rHwMibCnt.u4PFDropCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx Len Mismatch=%ld\n",
            prHwMibInfo->rHwMibCnt.u4RxLenMismatchCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx data indicate total=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_DATA_INDICATION_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx data retain total=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_DATA_RETAINED_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx drop by SW total=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_DROP_TOTAL_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx reorder miss=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_MISS_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx reorder within=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_WITHIN_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx reorder ahead=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_AHEAD_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx reorder behind=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_BEHIND_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx reorder behind continuous=%llu\n",
            RX_GET_CNT(prRxCtrl,
                       RX_DATA_REORDER_BEHIND_CONTINUOUS_COUNT));

        do {
            u32 u4AmsduCntx100 = 0;

            if (RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT)) {
                u4AmsduCntx100 = (u32)div64_u64(
                    RX_GET_CNT(prRxCtrl,
                               RX_DATA_MSDU_IN_AMSDU_COUNT)
                    *
                    100,
                    RX_GET_CNT(prRxCtrl,
                               RX_DATA_AMSDU_COUNT));
            }

            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "\tRx avg MSDU in AMSDU=%1ld.%02ld\n",
                u4AmsduCntx100 / 100, u4AmsduCntx100 % 100);
        } while (false);

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx total MSDU in AMSDU=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_DATA_MSDU_IN_AMSDU_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx AMSDU=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx AMSDU miss=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_MISS_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx no StaRec drop=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_NO_STA_DROP_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx inactive BSS drop=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_INACTIVE_BSS_DROP_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx HS20 drop=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_HS20_DROP_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx low SwRfb drop=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_LESS_SW_RFB_DROP_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx dupicate drop=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_DUPICATE_DROP_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx MIC err drop=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_MIC_ERROR_DROP_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx BAR handle=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_BAR_DROP_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx non-interest drop=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_NO_INTEREST_DROP_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx type err drop=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_TYPE_ERR_DROP_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx class err drop=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_CLASS_ERR_DROP_COUNT));
#if CFG_KEY_ERROR_STATISTIC_RECOVERY
        i4BytesWritten += kalSnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx BMC cipher mismatch=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_BMC_NO_KEY_COUNT));
        i4BytesWritten += kalSnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx BMC ICV error=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_BMC_KEY_ERROR_COUNT));
        i4BytesWritten += kalSnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx BMC Pkt=%llu\n",
            RX_GET_CNT(prRxCtrl, RX_BMC_PKT_COUNT));
#endif
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s", "===Phy/Timing Related Counters===\n");
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tChannelIdleCnt=%ld\n",
            prHwMibInfo->rHwMibCnt.u4ChannelIdleCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tCCA_NAV_Tx_Time=%ld\n",
            prHwMibInfo->rHwMibCnt.u4CcaNavTx);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tRx_MDRDY_CNT=%ld\n",
            prHwMibInfo->rHwMibCnt.u4MdrdyCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten,
            i4TotalLen - i4BytesWritten,
            "\tCCK_MDRDY=%ld, OFDM_MDRDY=0x%lx, OFDM_GREEN_MDRDY=0x%lx\n",
            prHwMibInfo->rHwMibCnt.u4CCKMdrdyCnt,
            prHwMibInfo->rHwMibCnt.u4OFDMLGMixMdrdy,
            prHwMibInfo->rHwMibCnt.u4OFDMGreenMdrdy);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tPrim CCA Time=%ld\n",
            prHwMibInfo->rHwMibCnt.u4PCcaTime);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tSec CCA Time=%ld\n",
            prHwMibInfo->rHwMibCnt.u4SCcaTime);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tPrim ED Time=%ld\n",
            prHwMibInfo->rHwMibCnt.u4PEDTime);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s", "===Tx Related Counters(Generic)===\n");
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tBeaconTxCnt=%ld\n",
            prHwMibInfo->rHwMibCnt.u4BeaconTxCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tTx 40MHz Cnt=%ld\n",
            prHwMibInfo->rHwMib2Cnt.u4Tx40MHzCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tTx 80MHz Cnt=%ld\n",
            prHwMibInfo->rHwMib2Cnt.u4Tx80MHzCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tTx 160MHz Cnt=%ld\n",
            prHwMibInfo->rHwMib2Cnt.u4Tx160MHzCnt);
        for (i = 0; i < BSSID_NUM; i++) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "\t===BSSID[%d] Related Counters===\n", i);
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "\tBA Miss Cnt=%ld\n",
                prHwMibInfo->rHwMibCnt.au4BaMissedCnt[i]);
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "\tRTS Tx Cnt=%ld\n",
                prHwMibInfo->rHwMibCnt.au4RtsTxCnt[i]);
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "\tFrame Retry Cnt=%ld\n",
                prHwMibInfo->rHwMibCnt.au4FrameRetryCnt[i]);
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "\tFrame Retry 2 Cnt=%ld\n",
                prHwMibInfo->rHwMibCnt.au4FrameRetry2Cnt[i]);
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "\tRTS Retry Cnt=%ld\n",
                prHwMibInfo->rHwMibCnt.au4RtsRetryCnt[i]);
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "\tAck Failed Cnt=%ld\n",
                prHwMibInfo->rHwMibCnt.au4AckFailedCnt[i]);
        }

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s", "===AMPDU Related Counters===\n");
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tTx AMPDU_Pkt_Cnt=%ld\n",
            prHwMibInfo->rHwTxAmpduMts.u2TxAmpduCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tTx AMPDU_MPDU_Pkt_Cnt=%ld\n",
            prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tAMPDU SuccessCnt=%ld\n",
            prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt);
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tAMPDU Tx success = %ld\n",
            prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt);

        u4Per = prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt == 0 ?
                0 :
                (1000 *
                 (prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt -
                  prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt)) /
                prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt;
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tAMPDU Tx fail count = %ld, PER=%ld.%1ld%%\n",
            prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt -
            prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt,
            u4Per / 10, u4Per % 10);
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%s", "\tTx Agg\n");
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s",
            "\tRange:  1 2~5 6~15 16~22 23~33 34~49 50~57 58~64\n");
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\t\t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d\n",
            prHwMibInfo->rHwTxAmpduMts.u2TxRange1AmpduCnt,
            prHwMibInfo->rHwTxAmpduMts.u2TxRange2AmpduCnt,
            prHwMibInfo->rHwTxAmpduMts.u2TxRange3AmpduCnt,
            prHwMibInfo->rHwTxAmpduMts.u2TxRange4AmpduCnt,
            prHwMibInfo->rHwTxAmpduMts.u2TxRange5AmpduCnt,
            prHwMibInfo->rHwTxAmpduMts.u2TxRange6AmpduCnt,
            prHwMibInfo->rHwTxAmpduMts.u2TxRange7AmpduCnt,
            prHwMibInfo->rHwTxAmpduMts.u2TxRange8AmpduCnt);
    } else {
        i4BytesWritten = kalScnprintf(pcCommand, i4TotalLen, "%s",
                                      "\nClear All Statistics\n");
    }

    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

    kalMemFree(prHwMibInfo, VIR_MEM_TYPE, sizeof(PARAM_HW_MIB_INFO_T));

    nicRxClearStatistics(prGlueInfo->prAdapter);

    return i4BytesWritten;
}

/* Private Coex Ctrl Subcmd for Isolation Detection */
static int priv_driver_iso_detect(IN P_GLUE_INFO_T prGlueInfo,
                                  IN struct CMD_COEX_CTRL *prCmdCoexCtrl,
                                  IN signed char *argv[])
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    u32 u4Ret = 0;

    struct CMD_COEX_ISO_DETECT rCmdCoexIsoDetect;

    rCmdCoexIsoDetect.u4Isolation = 0;
    kalMemZero(&rCmdCoexIsoDetect, sizeof(struct CMD_COEX_ISO_DETECT));

    u4Ret = kalkStrtou32(argv[2], 0, &(rCmdCoexIsoDetect.u4IsoPath));
    if (u4Ret) {
        DBGLOG(REQ,
               LOUD,
               " -priv_driver_coex_iso_detect - Parse Iso Path failed u4Ret=%d\n",
               u4Ret);
        return -1;
    }

    u4Ret = kalkStrtou32(argv[3], 0, &(rCmdCoexIsoDetect.u4Channel));
    if (u4Ret) {
        DBGLOG(REQ,
               LOUD,
               " -priv_driver_coex_iso_detect - Parse channel failed u4Ret = %d\n",
               u4Ret);
        return -1;
    }

    /* Copy Memory */
    kalMemCopy(prCmdCoexCtrl->aucBuffer, &rCmdCoexIsoDetect,
               sizeof(struct CMD_COEX_ISO_DETECT));

    /* Ioctl Isolation Detect */
    rStatus = kalIoctl(prGlueInfo, wlanoidQueryCoexIso, prCmdCoexCtrl,
                       sizeof(struct CMD_COEX_CTRL), true, true, true,
                       &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    /* If all pass, return u4Ret to 0 */
    return u4Ret;
}

/* Private Command for Coex Ctrl */
static int priv_driver_coex_ctrl(IN struct net_device *prNetDev,
                                 IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s32 i4ArgNum = 2;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 u4Ret = 0;
    enum ENUM_COEX_CTRL_CMD CoexCtrlCmd;
    struct CMD_COEX_CTRL rCmdCoexCtrl;

    kalMemZero(&rCmdCoexCtrl, sizeof(struct CMD_COEX_CTRL));

    ASSERT(prNetDev);

    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    /* Prevent Kernel Panic, set default i4ArgNum to 2 */
    if (i4Argc >= i4ArgNum) {
        /* Parse Coex SubCmd */
        u4Ret = kalkStrtou32(apcArgv[1], 0, &rCmdCoexCtrl.u4SubCmd);
        if (u4Ret) {
            return -1;
        }

        CoexCtrlCmd = (enum ENUM_COEX_CTRL_CMD)rCmdCoexCtrl.u4SubCmd;

        switch (CoexCtrlCmd) {
        /* Isolation Detection */
        case ENUM_COEX_CTRL_ISO_DETECT: {
            s32 i4SubArgNum = 4;
            /* Safely dereference "argv[3]".*/
            if (i4Argc >= i4SubArgNum) {
                struct CMD_COEX_ISO_DETECT *prCmdCoexIsoDetect;

                /* Isolation Detection Method */
                u4Ret = priv_driver_iso_detect(
                    prGlueInfo, &rCmdCoexCtrl, apcArgv);
                if (u4Ret) {
                    return -1;
                }

                /* Get Isolation value */
                prCmdCoexIsoDetect =
                    (struct CMD_COEX_ISO_DETECT *)
                    rCmdCoexCtrl.aucBuffer;

                /* Set Return i4BytesWritten Value */
                i4BytesWritten = snprintf(
                    pcCommand, i4TotalLen, "%d",
                    prCmdCoexIsoDetect->u4Isolation);
                DBGLOG(REQ, INFO, "Isolation: %d\n",
                       prCmdCoexIsoDetect->u4Isolation);
            }
            break;
        }

        /* Default Coex Cmd */
        default:
            break;
        }
    }
    return i4BytesWritten;
}

static int priv_driver_set_fw_log(IN struct net_device *prNetDev,
                                  IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 u4McuDest = 0;
    u32 u4LogType = 0;
#if CFG_SUPPORT_FW_DBG_LEVEL_CTRL
    u32 ucFwLogLevel = FW_DBG_LEVEL_DONT_SET;
#endif
    P_CMD_FW_LOG_2_HOST_CTRL_T prFwLog2HostCtrl = NULL;
    u32 u4Ret = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(RSN, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    DBGLOG(RSN, INFO, "MT6632 : priv_driver_set_fw_log\n");

    prFwLog2HostCtrl = (P_CMD_FW_LOG_2_HOST_CTRL_T)kalMemAlloc(
        sizeof(CMD_FW_LOG_2_HOST_CTRL_T), VIR_MEM_TYPE);
    if (!prFwLog2HostCtrl) {
        DBGLOG(REQ, ERROR,
               "allocate memory for prFwLog2HostCtrl failed\n");
        i4BytesWritten = -1;
        goto out;
    }
    kalMemZero(prFwLog2HostCtrl, sizeof(CMD_FW_LOG_2_HOST_CTRL_T));

#if CFG_SUPPORT_FW_DBG_LEVEL_CTRL
    if ((i4Argc != 3) && (i4Argc != 4)) {
        DBGLOG(REQ, ERROR, "argc %i  must be 3 or 4\n", i4Argc);
        i4BytesWritten = -1;
        goto out;
    }
#else
    if (i4Argc != 3) {
        DBGLOG(REQ, ERROR, "argc %i is not equal to 3\n", i4Argc);
        i4BytesWritten = -1;
        goto out;
    }
#endif

    u4Ret = kalkStrtou32(apcArgv[1], 0, &u4McuDest);
    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse u4McuDest error u4Ret=%d\n", u4Ret);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[2], 0, &u4LogType);
    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse u4LogType error u4Ret=%d\n", u4Ret);
        return -1;
    }

#if CFG_SUPPORT_FW_DBG_LEVEL_CTRL
    if (i4Argc == 4) {
        u4Ret = kalkStrtou32(apcArgv[3], 0, &ucFwLogLevel);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucFwLogLevel error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }
    }
    prFwLog2HostCtrl->ucFwLogLevel = (u8)ucFwLogLevel;
#endif

    prFwLog2HostCtrl->ucMcuDest = (u8)u4McuDest;
    prFwLog2HostCtrl->ucFwLog2HostCtrl = (u8)u4LogType;

    if (prFwLog2HostCtrl->ucMcuDest == 0) {
        prGlueInfo->prAdapter->rWifiVar.ucN9Log2HostCtrl =
            prFwLog2HostCtrl->ucFwLog2HostCtrl;
    } else if (prFwLog2HostCtrl->ucMcuDest == 1) {
        prGlueInfo->prAdapter->rWifiVar.ucCR4Log2HostCtrl =
            prFwLog2HostCtrl->ucFwLog2HostCtrl;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSetFwLog2Host, prFwLog2HostCtrl,
                       sizeof(CMD_FW_LOG_2_HOST_CTRL_T), true, true, true,
                       &u4BufLen);

    DBGLOG(REQ, INFO, "%s: command result is %s (%d %d)\n", __func__,
           pcCommand, u4McuDest, u4LogType);
    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "send fw log to host cmd failed\n");
        i4BytesWritten = -1;
        goto out;
    }

out:
    if (prFwLog2HostCtrl) {
        kalMemFree(prFwLog2HostCtrl, VIR_MEM_TYPE,
                   sizeof(CMD_FW_LOG_2_HOST_CTRL_T));
    }

    return i4BytesWritten;
}
#endif

static int priv_driver_get_magic_pkt_info(IN struct net_device *prNetDev,
                                          IN char *pcCommand,
                                          IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    CMD_GET_MAGIC_PKT_INFO_T *cmd = NULL;
    u8 fgWaitResp = true;
    u8 fgRead = true;

    ASSERT(prNetDev);

    DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    if (!prGlueInfo) {
        goto get_info_invalid;
    }

    cmd = (CMD_GET_MAGIC_PKT_INFO_T *)kalMemAlloc(sizeof(*cmd),
                                                  VIR_MEM_TYPE);

    if (!cmd) {
        goto get_info_invalid;
    }

    if (i4Argc > 1) {
        goto get_info_invalid;
    }

    memset(cmd, 0, sizeof(*cmd));
    cmd->u2Type = CMD_GET_MAGIC_PKT_INFO_TYPE;
    cmd->u2Len = sizeof(*cmd);

    rStatus = kalIoctl(prGlueInfo, wlanoidAdvCtrl, cmd, sizeof(*cmd),
                       fgWaitResp, fgRead, true, &u4BufLen);

    if ((rStatus != WLAN_STATUS_SUCCESS) &&
        (rStatus != WLAN_STATUS_PENDING)) {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\ncommand failed %x", rStatus);
    } else {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nMagicPacket_Rx count = %d",
                                   cmd->u4MagicPktCntTotal);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nPullLowGpio_Wakeup count = %d",
                                   cmd->u4GpioPullLowCntTotal);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nPullHighGpio_Wakeup count = %d",
                                   cmd->u4GpioPullHighCntTotal);
    }

get_info_invalid:
    if (cmd) {
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    }
    return i4BytesWritten;
}

static int priv_driver_get_mcr(IN struct net_device *prNetDev,
                               IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 u4Ret;
    s32 i4ArgNum = 2;
    CMD_ACCESS_REG rCmdAccessReg;

    kalMemZero(&rCmdAccessReg, sizeof(CMD_ACCESS_REG));
    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "parse get_mcr error (Address) u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        /* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
        rCmdAccessReg.u4Data = 0;

        DBGLOG(REQ, LOUD, "address is %x\n", rCmdAccessReg.u4Address);

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryMcrRead,
                           &rCmdAccessReg, sizeof(rCmdAccessReg), true,
                           true, true, &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }

        i4BytesWritten = snprintf(pcCommand, i4TotalLen, "0x%08x",
                                  (unsigned int)rCmdAccessReg.u4Data);
        DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
               pcCommand);
    }

    return i4BytesWritten;
}

int priv_driver_set_mcr(IN struct net_device *prNetDev, IN char *pcCommand,
                        IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Ret;
    s32 i4ArgNum = 3;
    CMD_ACCESS_REG rCmdAccessReg;

    kalMemZero(&rCmdAccessReg, sizeof(CMD_ACCESS_REG));
    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "parse get_mcr error (Address) u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        u4Ret = kalkStrtou32(apcArgv[2], 0, &(rCmdAccessReg.u4Data));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "parse get_mcr error (Data) u4Ret=%d\n", u4Ret);
            return -1;
        }

        /* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
        /* rCmdAccessReg.u4Data = kalStrtoul(apcArgv[2], NULL, 0); */

        rStatus = kalIoctl(prGlueInfo, wlanoidSetMcrWrite,
                           &rCmdAccessReg, sizeof(rCmdAccessReg), false,
                           false, true, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }
    }

    return i4BytesWritten;
}

static int priv_driver_set_test_mode(IN struct net_device *prNetDev,
                                     IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 u4Ret;
    s32 i4ArgNum = 2, u4MagicKey = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(u4MagicKey));
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse Magic Key error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        DBGLOG(REQ, LOUD, "The Set Test Mode Magic Key is %d\n",
               u4MagicKey);

        if (u4MagicKey == PRIV_CMD_TEST_MAGIC_KEY) {
            rStatus = kalIoctl(prGlueInfo, wlanoidRftestSetTestMode,
                               NULL, 0, false, false, true,
                               &u4BufLen);
        } else if (u4MagicKey == 0) {
            rStatus = kalIoctl(prGlueInfo,
                               wlanoidRftestSetAbortTestMode, NULL,
                               0, false, false, true, &u4BufLen);
        }

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }
    }

    return i4BytesWritten;
}

static int priv_driver_set_test_cmd(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 u4Ret;
    s32 i4ArgNum = 3;
    PARAM_MTK_WIFI_TEST_STRUCT_T rRfATInfo;

    kalMemZero(&rRfATInfo, sizeof(PARAM_MTK_WIFI_TEST_STRUCT_T));
    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rRfATInfo.u4FuncIndex));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "Parse Test CMD Index error u4Ret=%d\n", u4Ret);
            return -1;
        }

        u4Ret = kalkStrtou32(apcArgv[2], 0, &(rRfATInfo.u4FuncData));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "Parse Test CMD Data error u4Ret=%d\n", u4Ret);
            return -1;
        }

        DBGLOG(REQ, LOUD,
               "Set Test CMD FuncIndex = %d, FuncData = %d\n",
               rRfATInfo.u4FuncIndex, rRfATInfo.u4FuncData);

        rStatus = kalIoctl(prGlueInfo, wlanoidRftestSetAutoTest,
                           &rRfATInfo, sizeof(rRfATInfo), false, false,
                           true, &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }
    }

    return i4BytesWritten;
}

static int priv_driver_get_test_result(IN struct net_device *prNetDev,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 u4Ret;
    u32 u4Data = 0;
    s32 i4ArgNum = 3;
    PARAM_MTK_WIFI_TEST_STRUCT_T rRfATInfo;

    kalMemZero(&rRfATInfo, sizeof(PARAM_MTK_WIFI_TEST_STRUCT_T));
    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rRfATInfo.u4FuncIndex));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "Parse Test CMD Index error u4Ret=%d\n", u4Ret);
            return -1;
        }

        u4Ret = kalkStrtou32(apcArgv[2], 0, &(rRfATInfo.u4FuncData));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "Parse Test CMD Data error u4Ret=%d\n", u4Ret);
            return -1;
        }

        DBGLOG(REQ, LOUD,
               "Get Test CMD FuncIndex = %d, FuncData = %d\n",
               rRfATInfo.u4FuncIndex, rRfATInfo.u4FuncData);

        rStatus = kalIoctl(prGlueInfo, wlanoidRftestQueryAutoTest,
                           &rRfATInfo, sizeof(rRfATInfo), true, true,
                           true, &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }

        u4Data = (unsigned int)rRfATInfo.u4FuncData;
        i4BytesWritten = snprintf(pcCommand, i4TotalLen, "%d[0x%08x]",
                                  u4Data, u4Data);
        DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
               pcCommand);
    }

    return i4BytesWritten;
}

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
s32 priv_driver_last_sec_mcs_info(IN P_ADAPTER_T prAdapter, IN char *pcCommand,
                                  IN int i4TotalLen,
                                  P_PARAM_HW_WLAN_INFO_T prHwWlanInfo,
                                  struct PARAM_TX_MCS_INFO *prTxMcsInfo)
{
    u8 i, j, txmode, rate, stbc;
    u8 nsts;
    s32 i4BytesWritten = 0;
    u32 au4RxVect0Que[MCS_INFO_SAMPLE_CNT],
        au4RxVect1Que[MCS_INFO_SAMPLE_CNT];
    u8 ucStaIdx = prTxMcsInfo->ucStaIndex;

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nTx MCS:\n");

    for (i = 0; i < MCS_INFO_SAMPLE_CNT; i++) {
        u8 tmpPerSum = 0, cnt = 0;
        u16 tmpRateCode = 0xFFFF;

        if (prTxMcsInfo->au2TxRateCode[i] == 0xFFFF) {
            continue;
        }

        if (tmpRateCode == 0xFFFF) {
            tmpRateCode = prTxMcsInfo->au2TxRateCode[i];
        }

        txmode = HW_TX_RATE_TO_MODE(prTxMcsInfo->au2TxRateCode[i]);
        if (txmode >= MAX_TX_MODE) {
            txmode = MAX_TX_MODE;
        }
        rate = HW_TX_RATE_TO_MCS(prTxMcsInfo->au2TxRateCode[i], txmode);
        nsts = HW_TX_RATE_TO_NSS(prTxMcsInfo->au2TxRateCode[i]) + 1;
        stbc = HW_TX_RATE_TO_STBC(prTxMcsInfo->au2TxRateCode[i]);

        for (j = 0; j < MCS_INFO_SAMPLE_CNT; j++) {
            if (tmpRateCode == prTxMcsInfo->au2TxRateCode[j]) {
                tmpPerSum += prTxMcsInfo->aucTxRatePer[j];
                cnt++;
                prTxMcsInfo->au2TxRateCode[j] = 0xFFFF;
            }
        }

        if (txmode == TX_RATE_MODE_CCK) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, " %s, ",
                HW_TX_RATE_CCK_STR[rate & 0x3]);
        } else if (txmode == TX_RATE_MODE_OFDM) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             " %s, ", hw_rate_ofdm_str(rate));
        } else if ((txmode == TX_RATE_MODE_HTMIX) ||
                   (txmode == TX_RATE_MODE_HTGF)) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, " MCS%d, ", rate);
        } else {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             " NSS%d_MCS%d, ", nsts, rate);
        }

        if ((txmode == TX_RATE_MODE_CCK) ||
            (txmode == TX_RATE_MODE_OFDM)) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "%s, ", HW_TX_RATE_BW[0]);
        } else if (i >
                   prHwWlanInfo->rWtblPeerCap.ucChangeBWAfterRateN) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "%s, ",
                prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability
                <
                4 ?
                (prHwWlanInfo->rWtblPeerCap
                 .ucFrequencyCapability >
                 BW_20 ?
                 HW_TX_RATE_BW
                 [prHwWlanInfo
                  ->rWtblPeerCap
                  .ucFrequencyCapability -
                  1] :
                 HW_TX_RATE_BW
                 [prHwWlanInfo
                  ->rWtblPeerCap
                  .ucFrequencyCapability]) :
                HW_TX_RATE_BW[4]);
        } else {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "%s, ",
                prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability
                <
                4 ?
                HW_TX_RATE_BW
                [prHwWlanInfo->rWtblPeerCap
                 .ucFrequencyCapability] :
                HW_TX_RATE_BW[4]);
        }

        if (txmode == TX_RATE_MODE_CCK) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "%s, ", rate < 4 ? "LP" : "SP");
        } else if (txmode == TX_RATE_MODE_OFDM) {
            ;
        } else {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%s, ",
                priv_driver_get_sgi_info(
                    &prHwWlanInfo->rWtblPeerCap) == 0 ?
                "LGI" :
                "SGI");
        }

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s%s%s [PER: %02d%%]\t",
            txmode < 5 ? HW_TX_MODE_STR[txmode] : HW_TX_MODE_STR[5],
            stbc ? ", STBC, " : ", ",
            ((priv_driver_get_ldpc_info(
                  &prHwWlanInfo->rWtblTxConfig) == 0) ||
             (txmode == TX_RATE_MODE_CCK) ||
             (txmode == TX_RATE_MODE_OFDM)) ?
            "BCC" :
            "LDPC",
            tmpPerSum / cnt);

        for (j = 0; j < cnt; j++)
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten, "*");

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "\n");
    }

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nRx MCS:\n");

    kalMemCopy(au4RxVect0Que, prAdapter->arStaRec[ucStaIdx].au4RxVect0Que,
               sizeof(au4RxVect0Que));
    kalMemCopy(au4RxVect1Que, prAdapter->arStaRec[ucStaIdx].au4RxVect1Que,
               sizeof(au4RxVect1Que));

    for (i = 0; i < MCS_INFO_SAMPLE_CNT; i++) {
        u8 cnt = 0;
        u32 u4RxVector0 = 0xFFFFFFFF;
        u32 txmode, rate, frmode, sgi, nsts, ldpc, stbc, groupid, mu;
#define RX_MCS_INFO_MASK    BITS(0, 17)

        if (au4RxVect0Que[i] == 0xFFFFFFFF) {
            continue;
        }

        if (u4RxVector0 == 0xFFFFFFFF) {
            u4RxVector0 = au4RxVect0Que[i];
        }

        txmode = (au4RxVect0Que[i] & RX_VT_RX_MODE_MASK) >>
                 RX_VT_RX_MODE_OFFSET;
        rate = (au4RxVect0Que[i] & RX_VT_RX_RATE_MASK) >>
               RX_VT_RX_RATE_OFFSET;
        frmode = (au4RxVect0Que[i] & RX_VT_FR_MODE_MASK) >>
                 RX_VT_FR_MODE_OFFSET;
        nsts = ((au4RxVect1Que[i] & RX_VT_NSTS_MASK) >>
                RX_VT_NSTS_OFFSET);
        stbc = (au4RxVect0Que[i] & RX_VT_STBC_MASK) >>
               RX_VT_STBC_OFFSET;
        sgi = au4RxVect0Que[i] & RX_VT_SHORT_GI;
        ldpc = au4RxVect0Que[i] & RX_VT_LDPC;
        groupid = (au4RxVect1Que[i] & RX_VT_GROUP_ID_MASK) >>
                  RX_VT_GROUP_ID_OFFSET;

        for (j = 0; j < MCS_INFO_SAMPLE_CNT; j++) {
            if ((u4RxVector0 & RX_MCS_INFO_MASK) ==
                (au4RxVect0Que[j] & RX_MCS_INFO_MASK)) {
                au4RxVect0Que[j] = 0xFFFFFFFF;
                cnt++;
            }
        }

        if (groupid && groupid != 63) {
            mu = 1;
        } else {
            mu = 0;
            nsts += 1;
        }

        if (txmode == TX_RATE_MODE_CCK) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, " %s, ",
                rate < 4 ? HW_TX_RATE_CCK_STR[rate] :
                HW_TX_RATE_CCK_STR[4]);
        } else if (txmode == TX_RATE_MODE_OFDM) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             " %s, ", hw_rate_ofdm_str(rate));
        } else if ((txmode == TX_RATE_MODE_HTMIX) ||
                   (txmode == TX_RATE_MODE_HTGF)) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, " MCS%d, ", rate);
        } else {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             " NSS%d_MCS%d, ", nsts, rate);
        }

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s, ",
            frmode < 4 ? HW_TX_RATE_BW[frmode] : HW_TX_RATE_BW[4]);

        if (txmode == TX_RATE_MODE_CCK) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "%s, ", rate < 4 ? "LP" : "SP");
        } else if (txmode == TX_RATE_MODE_OFDM) {
            ;
        } else {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten, "%s,",
                             sgi == 0 ? "LGI" : "SGI");
        }

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%s",
                                       stbc == 0 ? " " : " STBC, ");

        if (mu) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "%s, %s, %s (%d)\t",
                txmode < 5 ? HW_TX_MODE_STR[txmode] :
                HW_TX_MODE_STR[5],
                ldpc == 0 ? "BCC" : "LDPC", "MU", groupid);
        } else {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%s, %s\t",
                txmode < 5 ? HW_TX_MODE_STR[txmode] :
                HW_TX_MODE_STR[5],
                ldpc == 0 ? "BCC" : "LDPC");
        }

        for (j = 0; j < cnt; j++)
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten, "*");

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "\n");
    }

    return i4BytesWritten;
}
#endif

s32 priv_driver_tx_rate_info(IN char *pcCommand, IN int i4TotalLen,
                             u8 fgDumpAll, P_PARAM_HW_WLAN_INFO_T prHwWlanInfo,
                             P_PARAM_GET_STA_STATISTICS prQueryStaStatistics)
{
    u8 i, txmode, rate, stbc;
    u8 nsts;
    s32 i4BytesWritten = 0;

    for (i = 0; i < AUTO_RATE_NUM; i++) {
        txmode = HW_TX_RATE_TO_MODE(
            prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);
        if (txmode >= MAX_TX_MODE) {
            txmode = MAX_TX_MODE;
        }
        rate = HW_TX_RATE_TO_MCS(
            prHwWlanInfo->rWtblRateInfo.au2RateCode[i], txmode);
        nsts = HW_TX_RATE_TO_NSS(
            prHwWlanInfo->rWtblRateInfo.au2RateCode[i]) +
               1;
        stbc = HW_TX_RATE_TO_STBC(
            prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);

        if (fgDumpAll) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "Rate index - %d ", i);

            if (prHwWlanInfo->rWtblRateInfo.ucRateIdx == i) {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten, "%s",
                    "--> ");
            } else {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten, "%s", " ");
            }
        }

        if (!fgDumpAll) {
            if (prHwWlanInfo->rWtblRateInfo.ucRateIdx != i) {
                continue;
            } else {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten, "%-20s%s",
                    "AR TX Rate", " = ");
            }
        }

        if (txmode == TX_RATE_MODE_CCK) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%s, ",
                HW_TX_RATE_CCK_STR[rate & 0x3]);
        } else if (txmode == TX_RATE_MODE_OFDM) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "%s, ", hw_rate_ofdm_str(rate));
        } else if ((txmode == TX_RATE_MODE_HTMIX) ||
                   (txmode == TX_RATE_MODE_HTGF)) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "MCS%d, ", rate);
        } else {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "NSS%d_MCS%d, ", nsts, rate);
        }

        if (prQueryStaStatistics->ucSkipAr) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "%s, ",
                prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability
                <
                4 ?
                HW_TX_RATE_BW
                [prHwWlanInfo->rWtblPeerCap
                 .ucFrequencyCapability] :
                HW_TX_RATE_BW[4]);
        } else {
            if ((txmode == TX_RATE_MODE_CCK) ||
                (txmode == TX_RATE_MODE_OFDM)) {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten, "%s, ",
                    HW_TX_RATE_BW[0]);
            } else if (i > prHwWlanInfo->rWtblPeerCap
                       .ucChangeBWAfterRateN) {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten, "%s, ",
                    prHwWlanInfo->rWtblPeerCap
                    .ucFrequencyCapability <
                    4 ?
                    (prHwWlanInfo->rWtblPeerCap
                     .ucFrequencyCapability >
                     BW_20 ?
                     HW_TX_RATE_BW
                     [prHwWlanInfo
                      ->rWtblPeerCap
                      .ucFrequencyCapability -
                      1] :
                     HW_TX_RATE_BW
                     [prHwWlanInfo
                      ->rWtblPeerCap
                      .ucFrequencyCapability]) :
                    HW_TX_RATE_BW[4]);
            } else {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten, "%s, ",
                    prHwWlanInfo->rWtblPeerCap
                    .ucFrequencyCapability <
                    4 ?
                    HW_TX_RATE_BW
                    [prHwWlanInfo
                     ->rWtblPeerCap
                     .ucFrequencyCapability] :
                    HW_TX_RATE_BW[4]);
            }
        }

        if (txmode == TX_RATE_MODE_CCK) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "%s, ", rate < 4 ? "LP" : "SP");
        } else if (txmode == TX_RATE_MODE_OFDM) {
            ;
        } else {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%s, ",
                priv_driver_get_sgi_info(
                    &prHwWlanInfo->rWtblPeerCap) == 0 ?
                "LGI" :
                "SGI");
        }

        if (prQueryStaStatistics->ucSkipAr) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%s%s%s\n",
                txmode < 5 ? HW_TX_MODE_STR[txmode] :
                HW_TX_MODE_STR[5],
                stbc ? ", STBC, " : ", ",
                priv_driver_get_ldpc_info(
                    &prHwWlanInfo->rWtblTxConfig) == 0 ?
                "BCC" :
                "LDPC");
        } else if (prQueryStaStatistics->aucArRatePer
                   [prQueryStaStatistics->aucRateEntryIndex[i]] ==
                   0xFF) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%s%s%s (--)\n",
                txmode < 5 ? HW_TX_MODE_STR[txmode] :
                HW_TX_MODE_STR[5],
                stbc ? ", STBC, " : ", ",
                ((priv_driver_get_ldpc_info(
                      &prHwWlanInfo->rWtblTxConfig) == 0) ||
                 (txmode == TX_RATE_MODE_CCK) ||
                 (txmode == TX_RATE_MODE_OFDM)) ?
                "BCC" :
                "LDPC");
        } else {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%s%s%s (%d)\n",
                txmode < 5 ? HW_TX_MODE_STR[txmode] :
                HW_TX_MODE_STR[5],
                stbc ? ", STBC, " : ", ",
                ((priv_driver_get_ldpc_info(
                      &prHwWlanInfo->rWtblTxConfig) == 0) ||
                 (txmode == TX_RATE_MODE_CCK) ||
                 (txmode == TX_RATE_MODE_OFDM)) ?
                "BCC" :
                "LDPC",
                prQueryStaStatistics->aucArRatePer
                [prQueryStaStatistics
                 ->aucRateEntryIndex[i]]);
        }

        if (!fgDumpAll) {
            break;
        }
    }

    return i4BytesWritten;
}

s32 priv_driver_last_rx_rssi(P_ADAPTER_T prAdapter, IN char *pcCommand,
                             IN int i4TotalLen, IN u8 ucWlanIdx)
{
    s32 i4RSSI0 = 0, i4RSSI1 = 0, i4RSSI2 = 0, i4RSSI3;
    s32 i4BytesWritten = 0;
    u32 u4RxVector3 = 0;
    u8 ucStaIdx;
#if CFG_SUPPORT_RSSI_COMP
    P_BSS_INFO_T prBssInfo;
#endif

    if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIdx, &ucStaIdx) ==
        WLAN_STATUS_SUCCESS) {
        u4RxVector3 = prAdapter->arStaRec[ucStaIdx].u4RxVector3;
        DBGLOG(REQ, LOUD, "****** RX Vector3 = 0x%08x ******\n",
               u4RxVector3);
    } else {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s", "Last RX RSSI",
                                       " = NOT SUPPORT");
        return i4BytesWritten;
    }

    i4RSSI0 = RCPI_TO_dBm((u4RxVector3 & RX_VT_RCPI0_MASK) >>
                          RX_VT_RCPI0_OFFSET);
    i4RSSI1 = RCPI_TO_dBm((u4RxVector3 & RX_VT_RCPI1_MASK) >>
                          RX_VT_RCPI1_OFFSET);

#if CFG_SUPPORT_RSSI_COMP
    prBssInfo = GET_BSS_INFO_BY_INDEX(
        prAdapter, prAdapter->arStaRec[ucStaIdx].ucBssIndex);
    if (prBssInfo->eBand == BAND_2G4) {
        i4RSSI0 += prAdapter->rWifiVar.rRssiPathCompasation
                   .c2GRssiCompensation;
        i4RSSI1 += prAdapter->rWifiVar.rRssiPathCompasation
                   .c2GRssiCompensation;
    } else {
        i4RSSI0 += prAdapter->rWifiVar.rRssiPathCompasation
                   .c5GRssiCompensation;
        i4RSSI1 += prAdapter->rWifiVar.rRssiPathCompasation
                   .c5GRssiCompensation;
    }
#endif
    if (prAdapter->rWifiVar.ucNSS > 2) {
        i4RSSI2 = RCPI_TO_dBm((u4RxVector3 & RX_VT_RCPI2_MASK) >>
                              RX_VT_RCPI2_OFFSET);
        i4RSSI3 = RCPI_TO_dBm((u4RxVector3 & RX_VT_RCPI3_MASK) >>
                              RX_VT_RCPI3_OFFSET);
#if CFG_SUPPORT_RSSI_COMP
        if (prBssInfo->eBand == BAND_2G4) {
            i4RSSI2 += prAdapter->rWifiVar.rRssiPathCompasation
                       .c2GRssiCompensation;
            i4RSSI3 += prAdapter->rWifiVar.rRssiPathCompasation
                       .c2GRssiCompensation;
        } else {
            i4RSSI2 += prAdapter->rWifiVar.rRssiPathCompasation
                       .c5GRssiCompensation;
            i4RSSI3 += prAdapter->rWifiVar.rRssiPathCompasation
                       .c5GRssiCompensation;
        }
#endif
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d %d %d %d\n", "Last RX Data RSSI", " = ",
            i4RSSI0, i4RSSI1, i4RSSI2, i4RSSI3);
    } else {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%d %d\n",
                                       "Last RX Data RSSI", " = ",
                                       i4RSSI0, i4RSSI1);
    }

    return i4BytesWritten;
}

s32 priv_driver_rx_rate_info(P_ADAPTER_T prAdapter, IN char *pcCommand,
                             IN int i4TotalLen, IN u8 ucWlanIdx)
{
    u32 txmode, rate, frmode, sgi, nsts, ldpc, stbc, groupid, mu;
    s32 i4BytesWritten = 0;
    u32 u4RxVector0 = 0, u4RxVector1 = 0;
    u8 ucStaIdx;

    if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIdx, &ucStaIdx) ==
        WLAN_STATUS_SUCCESS) {
        u4RxVector0 = prAdapter->arStaRec[ucStaIdx].u4RxVector0;
        u4RxVector1 = prAdapter->arStaRec[ucStaIdx].u4RxVector1;
        DBGLOG(REQ, LOUD, "****** RX Vector0 = 0x%08x ******\n",
               u4RxVector0);
        DBGLOG(REQ, LOUD, "****** RX Vector1 = 0x%08x ******\n",
               u4RxVector1);
    } else {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s", "Last RX Rate",
                                       " = NOT SUPPORT");
        return i4BytesWritten;
    }

    txmode = (u4RxVector0 & RX_VT_RX_MODE_MASK) >> RX_VT_RX_MODE_OFFSET;
    rate = (u4RxVector0 & RX_VT_RX_RATE_MASK) >> RX_VT_RX_RATE_OFFSET;
    frmode = (u4RxVector0 & RX_VT_FR_MODE_MASK) >> RX_VT_FR_MODE_OFFSET;
    nsts = ((u4RxVector1 & RX_VT_NSTS_MASK) >> RX_VT_NSTS_OFFSET);
    stbc = (u4RxVector0 & RX_VT_STBC_MASK) >> RX_VT_STBC_OFFSET;
    sgi = u4RxVector0 & RX_VT_SHORT_GI;
    ldpc = u4RxVector0 & RX_VT_LDPC;
    groupid = (u4RxVector1 & RX_VT_GROUP_ID_MASK) >> RX_VT_GROUP_ID_OFFSET;

    if (groupid && groupid != 63) {
        mu = 1;
    } else {
        mu = 0;
        nsts += 1;
    }

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten, "%-20s%s",
                                   "Last RX Rate", " = ");

    if (txmode == TX_RATE_MODE_CCK) {
        i4BytesWritten +=
            kalScnprintf(pcCommand + i4BytesWritten,
                         i4TotalLen - i4BytesWritten, "%s, ",
                         rate < 4 ? HW_TX_RATE_CCK_STR[rate] :
                         HW_TX_RATE_CCK_STR[4]);
    } else if (txmode == TX_RATE_MODE_OFDM) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%s, ", hw_rate_ofdm_str(rate));
    } else if ((txmode == TX_RATE_MODE_HTMIX) ||
               (txmode == TX_RATE_MODE_HTGF)) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "MCS%d, ", rate);
    } else {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "NSS%d_MCS%d, ", nsts, rate);
    }

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten, "%s, ",
        frmode < 4 ? HW_TX_RATE_BW[frmode] : HW_TX_RATE_BW[4]);

    if (txmode == TX_RATE_MODE_CCK) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%s, ", rate < 4 ? "LP" : "SP");
    } else if (txmode == TX_RATE_MODE_OFDM) {
        ;
    } else {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%s,", sgi == 0 ? "LGI" : "SGI");
    }

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten, "%s",
                                   stbc == 0 ? " " : " STBC, ");

    if (mu) {
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s, %s, %s (%d)\n",
            txmode < 5 ? HW_TX_MODE_STR[txmode] : HW_TX_MODE_STR[5],
            ldpc == 0 ? "BCC" : "LDPC", "MU", groupid);
    } else {
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s, %s\n",
            txmode < 5 ? HW_TX_MODE_STR[txmode] : HW_TX_MODE_STR[5],
            ldpc == 0 ? "BCC" : "LDPC");
    }

    return i4BytesWritten;
}

s32 priv_driver_tx_vector_info(IN char *pcCommand, IN int i4TotalLen,
                               IN P_TX_VECTOR_BBP_LATCH_T prTxV)
{
    u8 rate, txmode, frmode, sgi, ldpc, nsts, stbc, txpwr;
    s32 i4BytesWritten = 0;

    rate = TX_VECTOR_GET_TX_RATE(prTxV);
    txmode = TX_VECTOR_GET_TX_MODE(prTxV);
    frmode = TX_VECTOR_GET_TX_FRMODE(prTxV);
    nsts = TX_VECTOR_GET_TX_NSTS(prTxV) + 1;
    sgi = TX_VECTOR_GET_TX_SGI(prTxV);
    ldpc = TX_VECTOR_GET_TX_LDPC(prTxV);
    stbc = TX_VECTOR_GET_TX_STBC(prTxV);
    txpwr = TX_VECTOR_GET_TX_PWR(prTxV);

    if (prTxV->u4TxVector1 == 0xFFFFFFFF) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%s\n", "Last TX Rate",
                                       " = ", "N/A");
    } else {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s", "Last TX Rate",
                                       " = ");

        if (txmode == TX_RATE_MODE_CCK) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%s, ",
                rate < 4 ? HW_TX_RATE_CCK_STR[rate] :
                HW_TX_RATE_CCK_STR[4]);
        } else if (txmode == TX_RATE_MODE_OFDM) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "%s, ", hw_rate_ofdm_str(rate));
        } else if ((txmode == TX_RATE_MODE_HTMIX) ||
                   (txmode == TX_RATE_MODE_HTGF)) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "MCS%d, ", rate);
        } else {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "NSS%d_MCS%d, ", nsts, rate);
        }

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s, ",
            frmode < 4 ? HW_TX_RATE_BW[frmode] : HW_TX_RATE_BW[4]);

        if (txmode == TX_RATE_MODE_CCK) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "%s, ", rate < 4 ? "LP" : "SP");
        } else if (txmode == TX_RATE_MODE_OFDM) {
            ;
        } else {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "%s, ", sgi == 0 ? "LGI" : "SGI");
        }

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s%s%s\n",
            txmode < 5 ? HW_TX_MODE_STR[txmode] : HW_TX_MODE_STR[5],
            stbc ? ", STBC, " : ", ", ldpc == 0 ? "BCC" : "LDPC");
    }

    return i4BytesWritten;
}

static s32
priv_driver_dump_stat_info(P_ADAPTER_T prAdapter, IN char *pcCommand,
                           IN int i4TotalLen,
                           P_PARAM_HW_WLAN_INFO_T prHwWlanInfo,
                           P_PARAM_GET_STA_STATISTICS prQueryStaStatistics,
                           u8 fgResetCnt, u32 u4StatGroup)
{
    s32 i4BytesWritten = 0;
    PARAM_RSSI rRssi = 0;
    u16 u2LinkSpeed;
    u32 u4Per, u4RxPer[ENUM_BAND_NUM], u4AmpduPer[ENUM_BAND_NUM],
        u4InstantPer;
    u8 ucDbdcIdx, ucStaIdx, ucNss;
    u8 ucSkipAr;
    static u32 u4TotalTxCnt, u4TotalFailCnt;
    static u32 u4Rate1TxCnt, u4Rate1FailCnt;
    static u32 au4RxMpduCnt[ENUM_BAND_NUM] = { 0 };
    static u32 au4FcsError[ENUM_BAND_NUM] = { 0 };
    static u32 au4RxFifoCnt[ENUM_BAND_NUM] = { 0 };
    static u32 au4AmpduTxSfCnt[ENUM_BAND_NUM] = { 0 };
    static u32 au4AmpduTxAckSfCnt[ENUM_BAND_NUM] = { 0 };
    extern u32 g_au4RxMpduCnt[ENUM_BAND_NUM];
    extern u32 g_au4FcsError[ENUM_BAND_NUM];
    extern u32 g_au4RxFifoCnt[ENUM_BAND_NUM];
    extern u32 g_au4AmpduTxSfCnt[ENUM_BAND_NUM];
    extern u32 g_au4AmpduTxAckSfCnt[ENUM_BAND_NUM];
    P_RX_CTRL_T prRxCtrl;
    u32 u4InstantRxPer[ENUM_BAND_NUM];
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    s16 i2Wf0AvgPwr;
    s16 i2Wf1AvgPwr;
    u32 u4BufLen = 0;

    ucSkipAr = prQueryStaStatistics->ucSkipAr;
    prRxCtrl = &prAdapter->rRxCtrl;
    ucNss = prAdapter->rWifiVar.ucNSS;

    if (ucSkipAr) {
        u4TotalTxCnt += prHwWlanInfo->rWtblTxCounter.u2CurBwTxCnt +
                        prHwWlanInfo->rWtblTxCounter.u2OtherBwTxCnt;
        u4TotalFailCnt += prHwWlanInfo->rWtblTxCounter.u2CurBwFailCnt +
                          prHwWlanInfo->rWtblTxCounter.u2OtherBwFailCnt;
        u4Rate1TxCnt += prHwWlanInfo->rWtblTxCounter.u2Rate1TxCnt;
        u4Rate1FailCnt += prHwWlanInfo->rWtblTxCounter.u2Rate1FailCnt;
    }

    if (ucSkipAr) {
        u4Per = (prHwWlanInfo->rWtblTxCounter.u2Rate1TxCnt == 0) ?
                (0) :
                (1000 * u4Rate1FailCnt / u4Rate1TxCnt);

        u4InstantPer =
            (prHwWlanInfo->rWtblTxCounter.u2Rate1TxCnt == 0) ?
            (0) :
            (1000 *
             (prHwWlanInfo->rWtblTxCounter.u2Rate1FailCnt) /
             (prHwWlanInfo->rWtblTxCounter.u2Rate1TxCnt));
    } else {
        u4Per = (prQueryStaStatistics->u4Rate1TxCnt == 0) ?
                (0) :
                (1000 * (prQueryStaStatistics->u4Rate1FailCnt) /
                 (prQueryStaStatistics->u4Rate1TxCnt));

        u4InstantPer = (prQueryStaStatistics->ucPer == 0) ?
                       (0) :
                       (prQueryStaStatistics->ucPer);
    }

    for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM; ucDbdcIdx++) {
        au4RxMpduCnt[ucDbdcIdx] += g_au4RxMpduCnt[ucDbdcIdx];
        au4FcsError[ucDbdcIdx] += g_au4FcsError[ucDbdcIdx];
        au4RxFifoCnt[ucDbdcIdx] += g_au4RxFifoCnt[ucDbdcIdx];
        au4AmpduTxSfCnt[ucDbdcIdx] += g_au4AmpduTxSfCnt[ucDbdcIdx];
        au4AmpduTxAckSfCnt[ucDbdcIdx] +=
            g_au4AmpduTxAckSfCnt[ucDbdcIdx];
        g_au4RxMpduCnt[ucDbdcIdx] = 0;
        g_au4FcsError[ucDbdcIdx] = 0;
        g_au4RxFifoCnt[ucDbdcIdx] = 0;
        g_au4AmpduTxSfCnt[ucDbdcIdx] = 0;
        g_au4AmpduTxAckSfCnt[ucDbdcIdx] = 0;

        u4RxPer[ucDbdcIdx] = ((au4RxMpduCnt[ucDbdcIdx] +
                               au4FcsError[ucDbdcIdx]) == 0) ?
                             (0) :
                             (1000 * au4FcsError[ucDbdcIdx] /
                              (au4RxMpduCnt[ucDbdcIdx] +
                               au4FcsError[ucDbdcIdx]));

        u4AmpduPer[ucDbdcIdx] =
            (au4AmpduTxSfCnt[ucDbdcIdx] == 0) ?
            (0) :
            (1000 *
             (au4AmpduTxSfCnt[ucDbdcIdx] -
              au4AmpduTxAckSfCnt[ucDbdcIdx]) /
             au4AmpduTxSfCnt[ucDbdcIdx]);

        u4InstantRxPer[ucDbdcIdx] =
            ((prQueryStaStatistics->rMibInfo[ucDbdcIdx].u4RxMpduCnt
              +
              prQueryStaStatistics->rMibInfo[ucDbdcIdx]
              .u4FcsError) == 0) ?
            (0) :
            (1000 *
             prQueryStaStatistics->rMibInfo[ucDbdcIdx]
             .u4FcsError /
             (prQueryStaStatistics->rMibInfo[ucDbdcIdx]
              .u4RxMpduCnt +
              prQueryStaStatistics->rMibInfo[ucDbdcIdx]
              .u4FcsError));
    }

    /* get Beacon RSSI */
    rStatus = kalIoctl(prAdapter->prGlueInfo, wlanoidQueryRssi, &rRssi,
                       sizeof(rRssi), true, false, false, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "unable to retrieve rssi\n");
    }

    u2LinkSpeed = (prQueryStaStatistics->u2LinkSpeed == 0) ?
                  0 :
                  prQueryStaStatistics->u2LinkSpeed / 2;

    /* =========== Group 0x0001 =========== */
    if (u4StatGroup & 0x0001) {
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s", "\n----- STA Stat (Group 0x01) -----\n");

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "CurrTemperature", " = ",
            prQueryStaStatistics->ucTemperature);

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "Tx Total cnt", " = ",
            ucSkipAr ? (u4TotalTxCnt) :
            (prQueryStaStatistics->u4TransmitCount));

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "Tx Fail Cnt", " = ",
            ucSkipAr ? (u4TotalFailCnt) :
            (prQueryStaStatistics->u4TransmitFailCount));

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "Rate1 Tx Cnt", " = ",
            ucSkipAr ? (u4Rate1TxCnt) :
            (prQueryStaStatistics->u4Rate1TxCnt));

        if (ucSkipAr) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "%-20s%s%d, PER = %d.%1d%%, instant PER = %d.%1d%%\n",
                "Rate1 Fail Cnt",
                " = ",
                u4Rate1FailCnt,
                u4Per / 10,
                u4Per % 10,
                u4InstantPer / 10,
                u4InstantPer % 10);
        } else {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "%-20s%s%d, PER = %d.%1d%%, instant PER = %d%%\n",
                "Rate1 Fail Cnt",
                " = ",
                prQueryStaStatistics->u4Rate1FailCnt,
                u4Per / 10,
                u4Per % 10,
                u4InstantPer);
        }

        if ((ucSkipAr) && (fgResetCnt)) {
            u4TotalTxCnt = 0;
            u4TotalFailCnt = 0;
            u4Rate1TxCnt = 0;
            u4Rate1FailCnt = 0;
        }
    }

    /* =========== Group 0x0002 =========== */
    if (u4StatGroup & 0x0002) {
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s", "----- MIB Info (Group 0x02) -----\n");

        if (!prAdapter->rWifiVar.fgDbDcModeEn) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%-20s%s%d\n",
                "RX Success", " = ", au4RxMpduCnt[ENUM_BAND_0]);

            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "%-20s%s%d, PER = %d.%1d%%, instant PER = %d.%1d%%\n",
                "RX with CRC",
                " = ",
                au4FcsError[ENUM_BAND_0],
                u4RxPer[ENUM_BAND_0] / 10,
                u4RxPer[ENUM_BAND_0] % 10,
                u4InstantRxPer[ENUM_BAND_0] / 10,
                u4InstantRxPer[ENUM_BAND_0] % 10);

            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "%-20s%s%d\n", "RX drop FIFO full",
                             " = ", au4RxFifoCnt[ENUM_BAND_0]);
        } else {
            for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM;
                 ucDbdcIdx++) {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "[DBDC_%d] :\n", ucDbdcIdx);

                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%d\n", "RX Success", " = ",
                    au4RxMpduCnt[ucDbdcIdx]);

                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%d, PER = %d.%1d%%, instant PER = %d.%1d%%\n",
                    "RX with CRC",
                    " = ",
                    au4FcsError[ucDbdcIdx],
                    u4RxPer[ucDbdcIdx] / 10,
                    u4RxPer[ucDbdcIdx] % 10,
                    u4InstantRxPer[ucDbdcIdx] / 10,
                    u4InstantRxPer[ucDbdcIdx] % 10);

                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%d\n", "RX drop FIFO full",
                    " = ", au4RxFifoCnt[ucDbdcIdx]);
            }
        }

        if (fgResetCnt) {
            kalMemZero(au4RxMpduCnt, sizeof(au4RxMpduCnt));
            kalMemZero(au4FcsError, sizeof(au4RxMpduCnt));
            kalMemZero(au4RxFifoCnt, sizeof(au4RxMpduCnt));
            kalMemZero(au4AmpduTxSfCnt, sizeof(au4RxMpduCnt));
            kalMemZero(au4AmpduTxAckSfCnt, sizeof(au4RxMpduCnt));
        }
    }

    /* =========== Group 0x0004 =========== */
    if (u4StatGroup & 0x0004) {
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s", "----- Last Rx Info (Group 0x04) -----\n");

        rSwCtrlInfo.u4Data = 0;
        rSwCtrlInfo.u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + 1;

        rStatus = kalIoctl(prAdapter->prGlueInfo,
                           wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                           sizeof(rSwCtrlInfo), true, true, true,
                           &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u, rSwCtrlInfo.u4Data 0x%x\n",
               rStatus, rSwCtrlInfo.u4Data);
        if (rStatus == WLAN_STATUS_SUCCESS) {
            i2Wf0AvgPwr = rSwCtrlInfo.u4Data & 0xFFFF;
            i2Wf1AvgPwr = (rSwCtrlInfo.u4Data >> 16) & 0xFFFF;

            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%-20s%s%d %d\n",
                "NOISE", " = ", i2Wf0AvgPwr, i2Wf1AvgPwr);
        }

        /* Last RX Rate */
        i4BytesWritten += priv_driver_rx_rate_info(
            prAdapter, pcCommand + i4BytesWritten,
            i4TotalLen - i4BytesWritten,
            (u8)(prHwWlanInfo->u4Index));

        /* Last RX RSSI */
        i4BytesWritten += priv_driver_last_rx_rssi(
            prAdapter, pcCommand + i4BytesWritten,
            i4TotalLen - i4BytesWritten,
            (u8)(prHwWlanInfo->u4Index));

        /* Last RX Resp RSSI */
        if (ucNss > 2) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "%-20s%s%d %d %d %d\n", "Tx Response RSSI",
                " = ",
                RCPI_TO_dBm(
                    prHwWlanInfo->rWtblRxCounter.ucRxRcpi0),
                RCPI_TO_dBm(
                    prHwWlanInfo->rWtblRxCounter.ucRxRcpi1),
                RCPI_TO_dBm(
                    prHwWlanInfo->rWtblRxCounter.ucRxRcpi2),
                RCPI_TO_dBm(
                    prHwWlanInfo->rWtblRxCounter.ucRxRcpi3));
        } else {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%-20s%s%d %d\n",
                "Tx Response RSSI", " = ",
                RCPI_TO_dBm(
                    prHwWlanInfo->rWtblRxCounter.ucRxRcpi0),
                RCPI_TO_dBm(
                    prHwWlanInfo->rWtblRxCounter.ucRxRcpi1));
        }

        /* Last Beacon RSSI */
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%d\n", "Beacon RSSI",
                                       " = ", rRssi);
    }

    /* =========== Group 0x0008 =========== */
    if (u4StatGroup & 0x0008) {
        /* TxV */
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s", "----- Last TX Info (Group 0x08) -----\n");

        if (!prAdapter->rWifiVar.fgDbDcModeEn) {
            i4BytesWritten += priv_driver_tx_vector_info(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                &prQueryStaStatistics->rTxVector[ENUM_BAND_0]);

            if (prQueryStaStatistics->rTxVector[ENUM_BAND_0]
                .u4TxVector1 == 0xFFFFFFFF) {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%s\n", "Chip Out TX Power",
                    " = ", "N/A");
            } else {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%ld.%1ld dBm\n",
                    "Chip Out TX Power", " = ",
                    TX_VECTOR_GET_TX_PWR(
                        &prQueryStaStatistics->rTxVector
                        [ENUM_BAND_0]) >>
                    1,
                    5 * (TX_VECTOR_GET_TX_PWR(
                             &prQueryStaStatistics->
                             rTxVector
                             [ENUM_BAND_0]) %
                         2));
            }
        } else {
            for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM;
                 ucDbdcIdx++) {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "[DBDC_%d] :\n", ucDbdcIdx);

                i4BytesWritten += priv_driver_tx_vector_info(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    &prQueryStaStatistics
                    ->rTxVector[ucDbdcIdx]);

                if (prQueryStaStatistics->rTxVector[ucDbdcIdx]
                    .u4TxVector1 == 0xFFFFFFFF) {
                    i4BytesWritten += kalScnprintf(
                        pcCommand + i4BytesWritten,
                        i4TotalLen - i4BytesWritten,
                        "%-20s%s%s\n",
                        "Chip Out TX Power", " = ",
                        "N/A");
                } else {
                    i4BytesWritten += kalScnprintf(
                        pcCommand + i4BytesWritten,
                        i4TotalLen - i4BytesWritten,
                        "%-20s%s%ld.%1ld dBm\n",
                        "ChipOut TX Power", " = ",
                        TX_VECTOR_GET_TX_PWR(
                            &prQueryStaStatistics->
                            rTxVector
                            [ucDbdcIdx]) >>
                        1,
                        5 * (TX_VECTOR_GET_TX_PWR(
                                 &
                                 prQueryStaStatistics
                                 ->rTxVector
                                 [ucDbdcIdx]) %
                             2));
                }
            }
        }
    }

    /* =========== Group 0x0010 =========== */
    if (u4StatGroup & 0x0010) {
        /* RX Reorder */
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s", "------ RX Reorder (Group 0x10) -----\n");
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%llu\n", "Rx reorder miss", " = ",
            RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_MISS_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%llu\n", "Rx reorder within", " = ",
            RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_WITHIN_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%llu\n", "Rx reorder ahead", " = ",
            RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_AHEAD_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%llu\n", "Rx reorder behind", " = ",
            RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_BEHIND_COUNT));
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%llu\n", "Rx reorder behind continuous", " = ",
            RX_GET_CNT(prRxCtrl,
                       RX_DATA_REORDER_BEHIND_CONTINUOUS_COUNT));
    }

    /* =========== Group 0x0020 =========== */
    if (u4StatGroup & 0x0020) {
        /* AR info */
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s", "------ AR Info (Group 0x20) -----\n");

        /* Last TX Rate */
        i4BytesWritten += priv_driver_tx_rate_info(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            false, prHwWlanInfo, prQueryStaStatistics);

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%d\n", "LinkSpeed",
                                       " = ", u2LinkSpeed);

        if (!prQueryStaStatistics->ucSkipAr) {
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%-20s%s%s\n",
                "RateTable", " = ",
                prQueryStaStatistics->ucArTableIdx <
                RATE_TBL_MAX ?
                RATE_TBLE[prQueryStaStatistics
                          ->ucArTableIdx] :
                RATE_TBLE[RATE_TBL_MAX]);

            if (wlanGetStaIdxByWlanIdx(
                    prAdapter, (u8)(prHwWlanInfo->u4Index),
                    &ucStaIdx) == WLAN_STATUS_SUCCESS) {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%d\n", "2G Support 256QAM TX",
                    " = ",
                    (prAdapter->arStaRec[ucStaIdx].u4Flags &
                     MTK_SYNERGY_CAP_SUPPORT_24G_MCS89) ?
                    1 :
                    0);
            }

            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%-20s%s%d%%\n",
                "Rate1 instantPer", " = ", u4InstantPer);

            if (prQueryStaStatistics->ucAvePer == 0xFF) {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%s\n", "Train Down", " = ",
                    "N/A");

                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%s\n", "Train Up", " = ",
                    "N/A");
            } else {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%d -> %d\n", "Train Down",
                    " = ",
                    (prQueryStaStatistics->u2TrainDown) &
                    BITS(0, 7),
                    ((prQueryStaStatistics->u2TrainDown) >>
                     8) & BITS(0, 7));

                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%d -> %d\n", "Train Up", " = ",
                    (prQueryStaStatistics->u2TrainUp) &
                    BITS(0, 7),
                    ((prQueryStaStatistics->u2TrainUp) >>
                     8) & BITS(0, 7));
            }

            if (prQueryStaStatistics->fgIsForceTxStream == 0) {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%s\n", "Force Tx Stream", " = ",
                    "N/A");
            } else {
                i4BytesWritten += kalScnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "%-20s%s%d\n", "Force Tx Stream", " = ",
                    prQueryStaStatistics->fgIsForceTxStream);
            }

            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, "%-20s%s%d\n",
                "Force SE off", " = ",
                prQueryStaStatistics->fgIsForceSeOff);
        }

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "CBRN", " = ",
            prHwWlanInfo->rWtblPeerCap.ucChangeBWAfterRateN);

        /* Rate1~Rate8 */
        i4BytesWritten += priv_driver_tx_rate_info(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            true, prHwWlanInfo, prQueryStaStatistics);
    }

    /* =========== Group 0x0040 =========== */
    if (u4StatGroup & 0x0040) {
        /* Tx Agg */
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s", "------ TX AGG (Group 0x40) -----\n");

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-12s%s",
            "Range:", "1 2~5 6~15 16~22 23~33 34~49 50~57 58~64\n");

        for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM; ucDbdcIdx++) {
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "DBDC%d:", ucDbdcIdx);
            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "%7d%8d%9d%9d%9d%9d%9d%9d\n",
                prQueryStaStatistics->rMibInfo[ucDbdcIdx]
                .u2TxRange1AmpduCnt,
                prQueryStaStatistics->rMibInfo[ucDbdcIdx]
                .u2TxRange2AmpduCnt,
                prQueryStaStatistics->rMibInfo[ucDbdcIdx]
                .u2TxRange3AmpduCnt,
                prQueryStaStatistics->rMibInfo[ucDbdcIdx]
                .u2TxRange4AmpduCnt,
                prQueryStaStatistics->rMibInfo[ucDbdcIdx]
                .u2TxRange5AmpduCnt,
                prQueryStaStatistics->rMibInfo[ucDbdcIdx]
                .u2TxRange6AmpduCnt,
                prQueryStaStatistics->rMibInfo[ucDbdcIdx]
                .u2TxRange7AmpduCnt,
                prQueryStaStatistics->rMibInfo[ucDbdcIdx]
                .u2TxRange8AmpduCnt);
        }
    }

    return i4BytesWritten;
}

static int priv_driver_get_sta_stat(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0, u4Ret, u4StatGroup = 0xFFFFFFFF;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u8 aucMacAddr[MAC_ADDR_LEN];
    u8 ucWlanIndex;
    u8 *pucMacAddr = NULL;
    P_PARAM_HW_WLAN_INFO_T prHwWlanInfo = NULL;
    P_PARAM_GET_STA_STATISTICS prQueryStaStatistics = NULL;
    u8 fgResetCnt = false;
    u8 fgRxCCSel = false;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 3) {
        if (strnicmp(apcArgv[1], CMD_STAT_GROUP_SEL,
                     strlen(CMD_STAT_GROUP_SEL)) == 0) {
            u4Ret = kalkStrtou32(apcArgv[2], 0, &(u4StatGroup));
            if (u4Ret) {
                DBGLOG(REQ,
                       LOUD,
                       "parse get_mcr error (Address) u4Ret=%d\n",
                       u4Ret);
                return -1;
            }

            if (u4StatGroup == 0) {
                u4StatGroup = 0xFFFFFFFF;
            }

            if (prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP) {
                ucWlanIndex =
                    prGlueInfo->prAdapter->prAisBssInfo
                    ->prStaRecOfAP->ucWlanIndex;
            } else if (!wlanGetWlanIdxByAddress(
                           prGlueInfo->prAdapter, NULL,
                           &ucWlanIndex)) {
                DBGLOG(REQ,
                       INFO,
                       "Can't find the wlan index of MAC addr %pM!\n",
                       aucMacAddr);
                goto out;
            }
        } else {
            if (strnicmp(apcArgv[1], CMD_STAT_RESET_CNT,
                         strlen(CMD_STAT_RESET_CNT)) == 0) {
                if (wlanHwAddrToBin(apcArgv[2],
                                    &aucMacAddr[0]) < 0) {
                    DBGLOG(REQ,
                           WARN,
                           "%s: MAC addr Argv[2] convert failed\n",
                           __func__);
                    return -1;
                }
                fgResetCnt = true;
            } else if (strnicmp(apcArgv[2], CMD_STAT_RESET_CNT,
                                strlen(CMD_STAT_RESET_CNT)) == 0) {
                if (wlanHwAddrToBin(apcArgv[1],
                                    &aucMacAddr[0]) < 0) {
                    DBGLOG(REQ,
                           WARN,
                           "%s: MAC addr Argv[1] convert failed\n",
                           __func__);
                    return -1;
                }
                fgResetCnt = true;
            } else {
                if (wlanHwAddrToBin(apcArgv[1],
                                    &aucMacAddr[0]) < 0) {
                    DBGLOG(REQ,
                           WARN,
                           "%s: MAC addr Argv[1] convert failed\n",
                           __func__);
                    return -1;
                }
                fgResetCnt = false;
            }

            if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter,
                                         &aucMacAddr[0],
                                         &ucWlanIndex)) {
                DBGLOG(REQ,
                       INFO,
                       "Can't find the wlan index of MAC addr %pM!\n",
                       aucMacAddr);
                goto out;
            }
        }
    } else {
        /* Get AIS AP address for no argument */
        if (prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP) {
            ucWlanIndex = prGlueInfo->prAdapter->prAisBssInfo
                          ->prStaRecOfAP->ucWlanIndex;
        } else if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter, NULL,
                                            &ucWlanIndex)) {
            DBGLOG(REQ, INFO, "No connected peer found!\n");
            goto out;
        }

        if (i4Argc == 2) {
            if (strnicmp(apcArgv[1], CMD_STAT_RESET_CNT,
                         strlen(CMD_STAT_RESET_CNT)) == 0) {
                fgResetCnt = true;
            }else if (strnicmp(apcArgv[1], CMD_STAT_NOISE_SEL,
                               strlen(CMD_STAT_NOISE_SEL)) == 0) {
                fgRxCCSel = true;
            }
        }
    }

    prHwWlanInfo = (P_PARAM_HW_WLAN_INFO_T)kalMemAlloc(
        sizeof(PARAM_HW_WLAN_INFO_T), VIR_MEM_TYPE);
    if (!prHwWlanInfo) {
        DBGLOG(REQ, ERROR,
               "Allocate memory for prHwWlanInfo failed!\n");
        i4BytesWritten = -1;
        goto out;
    }
    kalMemZero(prHwWlanInfo, sizeof(PARAM_HW_WLAN_INFO_T));

    prHwWlanInfo->u4Index = ucWlanIndex;
    if (fgRxCCSel == true) {
        prHwWlanInfo->rWtblRxCounter.fgRxCCSel = true;
    }else{
        prHwWlanInfo->rWtblRxCounter.fgRxCCSel = false;
    }

    DBGLOG(REQ, INFO, "MT6632 : index = %d i4TotalLen = %d\n",
           prHwWlanInfo->u4Index, i4TotalLen);

    /* Get WTBL info */
    rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo, prHwWlanInfo,
                       sizeof(PARAM_HW_WLAN_INFO_T), true, true, true,
                       &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "Query prHwWlanInfo failed!\n");
        i4BytesWritten = -1;
        goto out;
    }

    /* Get Statistics info */
    prQueryStaStatistics = (P_PARAM_GET_STA_STATISTICS)kalMemAlloc(
        sizeof(PARAM_GET_STA_STATISTICS), VIR_MEM_TYPE);
    if (!prQueryStaStatistics) {
        DBGLOG(REQ, ERROR,
               "Allocate memory for prQueryStaStatistics failed!\n");
        i4BytesWritten = -1;
        goto out;
    }

    prQueryStaStatistics->ucResetCounter = fgResetCnt;

    pucMacAddr =
        wlanGetStaAddrByWlanIdx(prGlueInfo->prAdapter, ucWlanIndex);

    if (!pucMacAddr) {
        DBGLOG(REQ, ERROR,
               "Couldn't find the MAC addr of WlanIndex %d!\n",
               ucWlanIndex);
        i4BytesWritten = -1;
        goto out;
    }

    COPY_MAC_ADDR(prQueryStaStatistics->aucMacAddr, pucMacAddr);

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
                       prQueryStaStatistics,
                       sizeof(PARAM_GET_STA_STATISTICS), true, true, true,
                       &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "Query prQueryStaStatistics failed!\n");
        i4BytesWritten = -1;
        goto out;
    }

    if (pucMacAddr) {
        i4BytesWritten = priv_driver_dump_stat_info(
            prAdapter, pcCommand, i4TotalLen, prHwWlanInfo,
            prQueryStaStatistics, fgResetCnt, u4StatGroup);
    }
    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

out:
    if (prHwWlanInfo) {
        kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
                   sizeof(PARAM_HW_WLAN_INFO_T));
    }

    if (prQueryStaStatistics) {
        kalMemFree(prQueryStaStatistics, VIR_MEM_TYPE,
                   sizeof(PARAM_GET_STA_STATISTICS));
    }

    if (fgResetCnt) {
        nicRxClearStatistics(prGlueInfo->prAdapter);
    }

    return i4BytesWritten;
}

static s32
priv_driver_dump_stat2_info(P_ADAPTER_T prAdapter, IN char *pcCommand,
                            IN int i4TotalLen,
                            P_UMAC_STAT2_GET_T prUmacStat2GetInfo,
                            P_PARAM_GET_DRV_STATISTICS prQueryDrvStatistics)
{
    s32 i4BytesWritten = 0;
    u16 u2PleTotalRevPage = 0;
    u16 u2PleTotalSrcPage = 0;
    u16 u2PseTotalRevPage = 0;
    u16 u2PseTotalSrcPage = 0;

    u2PleTotalRevPage = prUmacStat2GetInfo->u2PleRevPgHif0Group0 +
                        prUmacStat2GetInfo->u2PleRevPgCpuGroup2;

    u2PleTotalSrcPage = prUmacStat2GetInfo->u2PleSrvPgHif0Group0 +
                        prUmacStat2GetInfo->u2PleSrvPgCpuGroup2;

    u2PseTotalRevPage = prUmacStat2GetInfo->u2PseRevPgHif0Group0 +
                        prUmacStat2GetInfo->u2PseRevPgHif1Group1 +
                        prUmacStat2GetInfo->u2PseRevPgCpuGroup2 +
                        prUmacStat2GetInfo->u2PseRevPgLmac0Group3 +
                        prUmacStat2GetInfo->u2PseRevPgLmac1Group4 +
                        prUmacStat2GetInfo->u2PseRevPgLmac2Group5 +
                        prUmacStat2GetInfo->u2PseRevPgPleGroup6;

    u2PseTotalSrcPage = prUmacStat2GetInfo->u2PseSrvPgHif0Group0 +
                        prUmacStat2GetInfo->u2PseSrvPgHif1Group1 +
                        prUmacStat2GetInfo->u2PseSrvPgCpuGroup2 +
                        prUmacStat2GetInfo->u2PseSrvPgLmac0Group3 +
                        prUmacStat2GetInfo->u2PseSrvPgLmac1Group4 +
                        prUmacStat2GetInfo->u2PseSrvPgLmac2Group5 +
                        prUmacStat2GetInfo->u2PseSrvPgPleGroup6;

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten, "%s",
                                   "\n----- Stat2 Info -----\n");

    /* Rev Page number Info. */
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten, "%s",
        "\n----- PLE Reservation Page Info. -----\n");

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Ple Hif0 Group0 RevPage", " = ",
        prUmacStat2GetInfo->u2PleRevPgHif0Group0);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "Ple Cpu Group2 RevPage",
                                   " = ",
                                   prUmacStat2GetInfo->u2PleRevPgCpuGroup2);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "Ple Total RevPage",
                                   " = ", u2PleTotalRevPage);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten, "%s",
        "\n----- PLE Source Page Info. ----------\n");

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Ple Hif0 Group0 SrcPage", " = ",
        prUmacStat2GetInfo->u2PleSrvPgHif0Group0);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "Ple Cpu Group2 SrcPage",
                                   " = ",
                                   prUmacStat2GetInfo->u2PleSrvPgCpuGroup2);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "Ple Total SrcPage",
                                   " = ", u2PleTotalSrcPage);

    /* umac MISC Info. */
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten, "%s",
        "\n----- PLE Misc Info. -----------------\n");

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "ple Total Page Number",
                                   " = ",
                                   prUmacStat2GetInfo->u2PleTotalPageNum);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "ple Free Page Number",
                                   " = ",
                                   prUmacStat2GetInfo->u2PleFreePageNum);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "ple FFA Page Number",
                                   " = ", prUmacStat2GetInfo->u2PleFfaNum);

    /* PSE Info. */
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten, "%s",
        "\n----- PSE Reservation Page Info. -----\n");

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pse Hif0 Group0 RevPage", " = ",
        prUmacStat2GetInfo->u2PseRevPgHif0Group0);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pse Hif1 Group1 RevPage", " = ",
        prUmacStat2GetInfo->u2PseRevPgHif1Group1);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "Pse Cpu Group2 RevPage",
                                   " = ",
                                   prUmacStat2GetInfo->u2PseRevPgCpuGroup2);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pse Lmac0 Group3 RevPage", " = ",
        prUmacStat2GetInfo->u2PseRevPgLmac0Group3);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pse Lmac1 Group4 RevPage", " = ",
        prUmacStat2GetInfo->u2PseRevPgLmac1Group4);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pse Lmac2 Group5 RevPage", " = ",
        prUmacStat2GetInfo->u2PseRevPgLmac2Group5);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "Pse Ple Group6 RevPage",
                                   " = ",
                                   prUmacStat2GetInfo->u2PseRevPgPleGroup6);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "Pse Total RevPage",
                                   " = ", u2PseTotalRevPage);

    /* PSE Info. */
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten, "%s",
        "\n----- PSE Source Page Info. ----------\n");

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pse Hif0 Group0 SrcPage", " = ",
        prUmacStat2GetInfo->u2PseSrvPgHif0Group0);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pse Hif1 Group1 SrcPage", " = ",
        prUmacStat2GetInfo->u2PseSrvPgHif1Group1);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "Pse Cpu Group2 SrcPage",
                                   " = ",
                                   prUmacStat2GetInfo->u2PseSrvPgCpuGroup2);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pse Lmac0 Group3 SrcPage", " = ",
        prUmacStat2GetInfo->u2PseSrvPgLmac0Group3);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pse Lmac1 Group4 SrcPage", " = ",
        prUmacStat2GetInfo->u2PseSrvPgLmac1Group4);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pse Lmac2 Group5 SrcPage", " = ",
        prUmacStat2GetInfo->u2PseSrvPgLmac2Group5);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "Pse Ple Group6 SrcPage",
                                   " = ",
                                   prUmacStat2GetInfo->u2PseSrvPgPleGroup6);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "Pse Total SrcPage",
                                   " = ", u2PseTotalSrcPage);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten, "%s",
        "\n----- PSE Misc Info. -----------------\n");

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "pse Total Page Number",
                                   " = ",
                                   prUmacStat2GetInfo->u2PseTotalPageNum);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "pse Free Page Number",
                                   " = ",
                                   prUmacStat2GetInfo->u2PseFreePageNum);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n", "pse FFA Page Number",
                                   " = ", prUmacStat2GetInfo->u2PseFfaNum);

    /* driver info */
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten, "%s",
        "\n\n----- DRV Stat -----------------------\n\n");

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pending Data", " = ",
        prQueryDrvStatistics->i4TxPendingFrameNum);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Pending Sec", " = ",
        prQueryDrvStatistics->i4TxPendingSecurityFrameNum);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Tx Pending For-pkt Number", " = ",
        prQueryDrvStatistics->i4PendingFwdFrameCount);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-26s%s%d\n",
                                   "MsduInfo Available Number", " = ",
                                   prQueryDrvStatistics->u4MsduNumElem);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "MgmtTxRing Pending Number", " = ",
        prQueryDrvStatistics->u4TxMgmtTxringQueueNumElem);

    /* Driver Rx Info. */
    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Rx Free Sw Rfb Number", " = ",
        prQueryDrvStatistics->u4RxFreeSwRfbMsduNumElem);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Rx Received Sw Rfb Number", " = ",
        prQueryDrvStatistics->u4RxReceivedRfbNumElem);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "%-26s%s%d\n", "Rx Indicated Sw Rfb Number", " = ",
        prQueryDrvStatistics->u4RxIndicatedNumElem);

    return i4BytesWritten;
}

static int priv_driver_get_sta_stat2(IN struct net_device *prNetDev,
                                     IN char *pcCommand, IN int i4TotalLen)
{
    s32 i4BytesWritten = 0;
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4ArgNum = 1;
    P_UMAC_STAT2_GET_T prUmacStat2GetInfo = NULL;
    P_PARAM_GET_DRV_STATISTICS prQueryDrvStatistics = NULL;
    P_QUE_T prQueList, prTxMgmtTxRingQueList;
    P_RX_CTRL_T prRxCtrl;

    KAL_SPIN_LOCK_DECLARATION();

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc < i4ArgNum) {
        return -1;
    }

    prQueList = &prAdapter->rTxCtrl.rFreeMsduInfoList;

    prTxMgmtTxRingQueList = &prAdapter->rTxCtrl.rTxMgmtTxingQueue;

    prRxCtrl = &prAdapter->rRxCtrl;
    ASSERT(prRxCtrl);

    /* to do for UMAC Dump */
    prUmacStat2GetInfo = (P_UMAC_STAT2_GET_T)kalMemAlloc(
        sizeof(UMAC_STAT2_GET_T), VIR_MEM_TYPE);
    if (!prUmacStat2GetInfo) {
        DBGLOG(REQ, ERROR,
               "allocate memory for prUmacStat2GetInfo failed\n");
        i4BytesWritten = -1;
        goto out;
    }

    halUmacInfoGetMiscStatus(prAdapter, prUmacStat2GetInfo);

    /* Get Driver stat info */
    prQueryDrvStatistics = (P_PARAM_GET_DRV_STATISTICS)kalMemAlloc(
        sizeof(PARAM_GET_DRV_STATISTICS), VIR_MEM_TYPE);
    if (!prQueryDrvStatistics) {
        DBGLOG(REQ, ERROR,
               "allocate memory for prQueryDrvStatistics failed\n");
        i4BytesWritten = -1;
        goto out;
    }

    prQueryDrvStatistics->i4TxPendingFrameNum =
        (u32)GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum);
    prQueryDrvStatistics->i4TxPendingSecurityFrameNum =
        (u32)GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingSecurityFrameNum);

    prQueryDrvStatistics->i4PendingFwdFrameCount =
        prAdapter->rTxCtrl.i4PendingFwdFrameCount;

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);
    prQueryDrvStatistics->u4MsduNumElem = prQueList->u4NumElem;
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);
    prQueryDrvStatistics->u4TxMgmtTxringQueueNumElem =
        prTxMgmtTxRingQueList->u4NumElem;
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
    prQueryDrvStatistics->u4RxFreeSwRfbMsduNumElem =
        prRxCtrl->rFreeSwRfbList.u4NumElem;
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
    prQueryDrvStatistics->u4RxReceivedRfbNumElem =
        prRxCtrl->rReceivedRfbList.u4NumElem;
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
    prQueryDrvStatistics->u4RxIndicatedNumElem =
        prRxCtrl->rIndicatedRfbList.u4NumElem;
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

    i4BytesWritten = priv_driver_dump_stat2_info(prAdapter, pcCommand,
                                                 i4TotalLen,
                                                 prUmacStat2GetInfo,
                                                 prQueryDrvStatistics);

out:
    if (prUmacStat2GetInfo) {
        kalMemFree(prUmacStat2GetInfo, VIR_MEM_TYPE,
                   sizeof(UMAC_STAT2_GET_T));
    }
    if (prQueryDrvStatistics) {
        kalMemFree(prQueryDrvStatistics, VIR_MEM_TYPE,
                   sizeof(PARAM_GET_DRV_STATISTICS));
    }

    return i4BytesWritten;
}

static s32 priv_driver_dump_rx_stat_info(P_ADAPTER_T prAdapter,
                                         IN char *pcCommand, IN int i4TotalLen,
                                         IN u8 fgResetCnt)
{
    s32 i4BytesWritten = 0;
    u32 u4RxVector0 = 0, u4RxVector2 = 0, u4RxVector3 = 0, u4RxVector4 = 0;
    u8 ucStaIdx, ucWlanIndex, cbw;
    u8 fgWlanIdxFound = true, fgSkipRxV = false;
    u32 u4FAGCRssiWBR0, u4FAGCRssiIBR0;
    u32 u4Value, u4Foe, foe_const;
    static u32 au4MacMdrdy[ENUM_BAND_NUM] = { 0 };
    static u32 au4FcsError[ENUM_BAND_NUM] = { 0 };
    static u32 au4OutOfResource[ENUM_BAND_NUM] = { 0 };
    static u32 au4LengthMismatch[ENUM_BAND_NUM] = { 0 };

    au4MacMdrdy[ENUM_BAND_0] += htonl(g_HqaRxStat.MAC_Mdrdy);
    au4MacMdrdy[ENUM_BAND_1] += htonl(g_HqaRxStat.MAC_Mdrdy1);
    au4FcsError[ENUM_BAND_0] += htonl(g_HqaRxStat.MAC_FCS_Err);
    au4FcsError[ENUM_BAND_1] += htonl(g_HqaRxStat.MAC_FCS_Err1);
    au4OutOfResource[ENUM_BAND_0] += htonl(g_HqaRxStat.OutOfResource);
    au4OutOfResource[ENUM_BAND_1] += htonl(g_HqaRxStat.OutOfResource1);
    au4LengthMismatch[ENUM_BAND_0] +=
        htonl(g_HqaRxStat.LengthMismatchCount_B0);
    au4LengthMismatch[ENUM_BAND_1] +=
        htonl(g_HqaRxStat.LengthMismatchCount_B1);

    if (fgResetCnt) {
        kalMemZero(au4MacMdrdy, sizeof(au4MacMdrdy));
        kalMemZero(au4FcsError, sizeof(au4FcsError));
        kalMemZero(au4OutOfResource, sizeof(au4OutOfResource));
        kalMemZero(au4LengthMismatch, sizeof(au4LengthMismatch));
    }

    if (prAdapter->prAisBssInfo->prStaRecOfAP) {
        ucWlanIndex =
            prAdapter->prAisBssInfo->prStaRecOfAP->ucWlanIndex;
    } else if (!wlanGetWlanIdxByAddress(prAdapter, NULL, &ucWlanIndex)) {
        fgWlanIdxFound = false;
    }

    if (fgWlanIdxFound) {
        if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIndex, &ucStaIdx) ==
            WLAN_STATUS_SUCCESS) {
            u4RxVector0 = prAdapter->arStaRec[ucStaIdx].u4RxVector0;
            u4RxVector2 = prAdapter->arStaRec[ucStaIdx].u4RxVector2;
            u4RxVector3 = prAdapter->arStaRec[ucStaIdx].u4RxVector3;
            u4RxVector4 = prAdapter->arStaRec[ucStaIdx].u4RxVector4;
        } else {
            fgSkipRxV = true;
        }
    } else {
        fgSkipRxV = true;
    }

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten, "%s",
                                   "\n\nRX Stat:\n");

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "MAC Mdrdy0", " = ",
                                   au4MacMdrdy[ENUM_BAND_0]);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "MAC Mdrdy1", " = ",
                                   au4MacMdrdy[ENUM_BAND_1]);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "FCS Err0", " = ",
                                   au4FcsError[ENUM_BAND_0]);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "FCS Err1", " = ",
                                   au4FcsError[ENUM_BAND_1]);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "CCK PD Cnt B0", " = ",
                                   htonl(g_HqaRxStat.CCK_PD));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "CCK PD Cnt B1", " = ",
                                   htonl(g_HqaRxStat.CCK_PD_Band1));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "CCK SIG Err B0", " = ",
                                   htonl(g_HqaRxStat.CCK_SIG_Err));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "CCK SIG Err B1", " = ",
                                   htonl(g_HqaRxStat.CCK_SIG_Err_Band1));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "OFDM PD Cnt B0", " = ",
                                   htonl(g_HqaRxStat.OFDM_PD));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "OFDM PD Cnt B1", " = ",
                                   htonl(g_HqaRxStat.OFDM_PD_Band1));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "OFDM TAG Error", " = ",
                                   htonl(g_HqaRxStat.OFDM_TAG_Err));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "CCK SFD Err B0", " = ",
                                   htonl(g_HqaRxStat.CCK_SFD_Err));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "CCK SFD Err B1", " = ",
                                   htonl(g_HqaRxStat.CCK_SFD_Err_Band1));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "OFDM SIG Err B0", " = ",
                                   htonl(g_HqaRxStat.OFDM_SIG_Err));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "OFDM SIG Err B1", " = ",
                                   htonl(g_HqaRxStat.OFDM_SIG_Err_Band1));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "CCK FCS Err B0", " = ",
                                   htonl(g_HqaRxStat.FCSErr_CCK));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "CCK FCS Err B1", " = ",
                                   htonl(g_HqaRxStat.CCK_FCS_Err_Band1));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "OFDM FCS Err B0", " = ",
                                   htonl(g_HqaRxStat.FCSErr_OFDM));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "OFDM FCS Err B1", " = ",
                                   htonl(g_HqaRxStat.OFDM_FCS_Err_Band1));

    if (!fgSkipRxV) {
        u4FAGCRssiIBR0 = (u4RxVector2 & BITS(16, 23)) >> 16;
        u4FAGCRssiWBR0 = (u4RxVector2 & BITS(24, 31)) >> 24;

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "FAGC RSSI W", " = ",
            (u4FAGCRssiWBR0 >= 128) ? (u4FAGCRssiWBR0 - 256) :
            (u4FAGCRssiWBR0));

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "FAGC RSSI I", " = ",
            (u4FAGCRssiIBR0 >= 128) ? (u4FAGCRssiIBR0 - 256) :
            (u4FAGCRssiIBR0));
    } else {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%s\n", "FAGC RSSI W",
                                       " = ", "N/A");

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%s\n", "FAGC RSSI I",
                                       " = ", "N/A");
    }

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "CCK MDRDY B0", " = ",
                                   htonl(g_HqaRxStat.PhyMdrdyCCK));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "CCK MDRDY B1", " = ",
                                   htonl(g_HqaRxStat.PHY_CCK_MDRDY_Band1));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "OFDM MDRDY B0", " = ",
                                   htonl(g_HqaRxStat.PhyMdrdyOFDM));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "OFDM MDRDY B1", " = ",
                                   htonl(g_HqaRxStat.PHY_OFDM_MDRDY_Band1));

    if (!fgSkipRxV) {
        u4Value = (u4RxVector0 & BITS(12, 14)) >> 12;
        if (u4Value == 0) {
            u4Foe = (((u4RxVector4 & BITS(7, 31)) >> 7) & 0x7ff);
            u4Foe = (u4Foe * 1000) >> 11;
        } else {
            cbw = ((u4RxVector0 & BITS(15, 16)) >> 15);
            foe_const = ((1 << (cbw + 1)) & 0xf) * 10000;
            u4Foe = (((u4RxVector4 & BITS(7, 31)) >> 7) & 0xfff);
            u4Foe = (u4Foe * foe_const) >> 15;
        }

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "Freq Offset From RX", " = ", u4Foe);

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "RX SNR (dB)", " = ",
            ((u4RxVector4 & BITS(26, 31)) >> 26) - 16);

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%d\n", "RCPI RX0", " = ",
                                       u4RxVector3 & BITS(0, 7));

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "RCPI RX1", " = ",
            (u4RxVector3 & BITS(8, 15)) >> 8);

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "RCPI RX2", " = ",
            ((u4RxVector3 & BITS(16, 23)) >> 16) == 0xFF ?
            (0) :
            ((u4RxVector3 & BITS(16, 23)) >> 16));

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%d\n", "RCPI RX3", " = ",
            ((u4RxVector3 & BITS(24, 31)) >> 24) == 0xFF ?
            (0) :
            ((u4RxVector3 & BITS(24, 31)) >> 24));
    } else {
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%-20s%s%s\n", "Freq Offset From RX", " = ", "N/A");

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%s\n", "RX SNR (dB)",
                                       " = ", "N/A");

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%s\n", "RCPI RX0", " = ",
                                       "N/A");

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%s\n", "RCPI RX1", " = ",
                                       "N/A");

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%s\n", "RCPI RX2", " = ",
                                       "N/A");

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%-20s%s%s\n", "RCPI RX3", " = ",
                                       "N/A");
    }

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "Inst RSSI IB R0", " = ",
                                   htonl(g_HqaRxStat.InstRssiIBR0));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "Inst RSSI WB R0", " = ",
                                   htonl(g_HqaRxStat.InstRssiWBR0));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "Inst RSSI IB R1", " = ",
                                   htonl(g_HqaRxStat.InstRssiIBR1));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "Inst RSSI WB R1", " = ",
                                   htonl(g_HqaRxStat.InstRssiWBR1));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "Inst RSSI IB R2", " = ",
                                   htonl(g_HqaRxStat.InstRssiIBR2));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "Inst RSSI WB R2", " = ",
                                   htonl(g_HqaRxStat.InstRssiWBR2));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "Inst RSSI IB R3", " = ",
                                   htonl(g_HqaRxStat.InstRssiIBR3));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "Inst RSSI WB R3", " = ",
                                   htonl(g_HqaRxStat.InstRssiWBR3));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "ACI Hit Lower", " = ",
                                   htonl(g_HqaRxStat.ACIHitLower));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "ACI Hit Higher", " = ",
                                   htonl(g_HqaRxStat.ACIHitUpper));

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "OutOf Resource Pkt0",
                                   " = ", au4OutOfResource[ENUM_BAND_0]);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "OutOf Resource Pkt1",
                                   " = ", au4OutOfResource[ENUM_BAND_1]);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "Len Mismatch Cnt B0",
                                   " = ", au4LengthMismatch[ENUM_BAND_0]);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%-20s%s%d\n", "Len Mismatch Cnt B1",
                                   " = ", au4LengthMismatch[ENUM_BAND_1]);

    return i4BytesWritten;
}

static int priv_driver_show_rx_stat(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    P_PARAM_CUSTOM_ACCESS_RX_STAT prRxStatisticsTest;
    u8 fgResetCnt = false;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    u32 u4Id = 0x99980000;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    DBGLOG(INIT, ERROR, "MT6632 : priv_driver_show_rx_stat\n");

    if (i4Argc >= 2) {
        if (strnicmp(apcArgv[1], CMD_STAT_RESET_CNT,
                     strlen(CMD_STAT_RESET_CNT)) == 0) {
            fgResetCnt = true;
        }
    }

    if (i4Argc >= 1) {
        if (fgResetCnt) {
            rSwCtrlInfo.u4Id = u4Id;
            rSwCtrlInfo.u4Data = 0;

            rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
                               &rSwCtrlInfo, sizeof(rSwCtrlInfo),
                               false, false, true, &u4BufLen);

            if (rStatus != WLAN_STATUS_SUCCESS) {
                return -1;
            }
        }

        prRxStatisticsTest = (P_PARAM_CUSTOM_ACCESS_RX_STAT)kalMemAlloc(
            sizeof(PARAM_CUSTOM_ACCESS_RX_STAT), VIR_MEM_TYPE);
        if (!prRxStatisticsTest) {
            return -1;
        }

        prRxStatisticsTest->u4SeqNum = u4RxStatSeqNum;
        prRxStatisticsTest->u4TotalNum = sizeof(PARAM_RX_STAT_T) / 4;

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryRxStatistics,
                           prRxStatisticsTest,
                           sizeof(PARAM_CUSTOM_ACCESS_RX_STAT), true,
                           true, true, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            kalMemFree(prRxStatisticsTest, VIR_MEM_TYPE,
                       sizeof(PARAM_CUSTOM_ACCESS_RX_STAT));
            return -1;
        }

        i4BytesWritten = priv_driver_dump_rx_stat_info(
            prAdapter, pcCommand, i4TotalLen, fgResetCnt);

        kalMemFree(prRxStatisticsTest, VIR_MEM_TYPE,
                   sizeof(PARAM_CUSTOM_ACCESS_RX_STAT));
    }

    return i4BytesWritten;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle command to get current Tx rate from rate table and respond
 *        string buffer. Example: VHT-2SS-BW80-SGI-MCS7
 *
 * \param[in] net_device Pointer to the Adapter structure.
 * \param[out] pcCommand Pointer to the command buffer to respond.
 * \param[in] i4TotalLen The length of  buffer.
 *
 * \retval Length of response buffer
 */
/*----------------------------------------------------------------------------*/
static int priv_driver_get_sta_curr_ar_rate(IN struct net_device *prNetDev,
                                            IN char *pcCommand,
                                            IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    u8 ucWlanIndex = 0;
    u8 *pucMacAddr = NULL;
    u8 idx, txmode, rate;
    P_PARAM_GET_STA_STATISTICS prQueryStaStatistics = NULL;
    P_PARAM_HW_WLAN_INFO_T prHwWlanInfo = NULL;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -EINVAL;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    /* Get AIS AP address for no argument */
    if (prAdapter->prAisBssInfo->prStaRecOfAP) {
        ucWlanIndex =
            prAdapter->prAisBssInfo->prStaRecOfAP->ucWlanIndex;
    } else if (!wlanGetWlanIdxByAddress(prAdapter, NULL, &ucWlanIndex)) {
        return i4BytesWritten;
    }

    pucMacAddr = wlanGetStaAddrByWlanIdx(prAdapter, ucWlanIndex);
    if (!pucMacAddr) {
        DBGLOG(REQ, WARN, "%s: MAC address is invalid!\n", __func__);
        return -EFAULT;
    }
    /* Get WTBL info */
    prHwWlanInfo = (P_PARAM_HW_WLAN_INFO_T)kalMemAlloc(
        sizeof(PARAM_HW_WLAN_INFO_T), VIR_MEM_TYPE);
    if (!prHwWlanInfo) {
        return -ENOMEM;
    }

    kalMemZero(prHwWlanInfo, sizeof(PARAM_HW_WLAN_INFO_T));
    prHwWlanInfo->u4Index = ucWlanIndex;
    rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo, prHwWlanInfo,
                       sizeof(PARAM_HW_WLAN_INFO_T), true, true, true,
                       &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        i4BytesWritten = -EFAULT;
        goto out_get_curr_ar_rate;
    }

    prQueryStaStatistics = (P_PARAM_GET_STA_STATISTICS)kalMemAlloc(
        sizeof(PARAM_GET_STA_STATISTICS), VIR_MEM_TYPE);
    if (!prQueryStaStatistics) {
        i4BytesWritten = -ENOMEM;
        goto out_get_curr_ar_rate;
    }

    /* Get Statistics info */
    COPY_MAC_ADDR(prQueryStaStatistics->aucMacAddr, pucMacAddr);
    rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
                       prQueryStaStatistics,
                       sizeof(PARAM_GET_STA_STATISTICS), true, true, true,
                       &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        i4BytesWritten = -EFAULT;
        goto out_get_curr_ar_rate;
    }

    idx = prHwWlanInfo->rWtblRateInfo.ucRateIdx;
    if (idx >= AUTO_RATE_NUM) {
        DBGLOG(REQ, WARN, "%s: Rate index is incorrect (%d)\n",
               __func__, idx);
        i4BytesWritten = -EFAULT;
        goto out_get_curr_ar_rate;
    }
    txmode = HW_TX_RATE_TO_MODE(
        prHwWlanInfo->rWtblRateInfo.au2RateCode[idx]);
    if (txmode >= MAX_TX_MODE) {
        txmode = MAX_TX_MODE;
    }
    rate = HW_TX_RATE_TO_MCS(prHwWlanInfo->rWtblRateInfo.au2RateCode[idx],
                             txmode);

    /* Mode: [CCK | OFDM | HT | VHT] */
    if (txmode == TX_RATE_MODE_CCK) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "CCK-");
    } else if (txmode == TX_RATE_MODE_OFDM) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "OFDM-");
    } else if (txmode == TX_RATE_MODE_HTGF ||
               txmode == TX_RATE_MODE_HTMIX) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "HT-");
    } else {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "VHT-");
    }
    /* Spatial Streams: [1SS | 2SS | N/A] */
    if (prQueryStaStatistics->ucArTableIdx == RATE_TBL_N_2SS ||
        prQueryStaStatistics->ucArTableIdx == RATE_TBL_AC_2SS) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "2SS-");
    } else if (prQueryStaStatistics->ucArTableIdx == RATE_TBL_N ||
               prQueryStaStatistics->ucArTableIdx == RATE_TBL_AC) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "1SS-");
    } else {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "N/A-");
    }

    /* BW mode: [BW20 | BW40 | BW80 | BW160/BW8080] */
    if ((txmode == TX_RATE_MODE_CCK) || (txmode == TX_RATE_MODE_OFDM)) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%s-", HW_TX_RATE_BW[0]);
    } else if (idx > prHwWlanInfo->rWtblPeerCap.ucChangeBWAfterRateN) {
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s-",
            prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability < 4 ?
            (prHwWlanInfo->rWtblPeerCap
             .ucFrequencyCapability >
             BW_20 ?
             HW_TX_RATE_BW
             [prHwWlanInfo->rWtblPeerCap
              .ucFrequencyCapability -
              1] :
             HW_TX_RATE_BW
             [prHwWlanInfo->rWtblPeerCap
              .ucFrequencyCapability]) :
            HW_TX_RATE_BW[4]);
    } else {
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "%s-",
            prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability < 4 ?
            HW_TX_RATE_BW[prHwWlanInfo->rWtblPeerCap
                          .ucFrequencyCapability] :
            HW_TX_RATE_BW[4]);
    }
    /* GI mode: [LGI | SGI | X] */
    if (txmode == TX_RATE_MODE_CCK || txmode == TX_RATE_MODE_OFDM) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "N/A-");
    } else {
        i4BytesWritten +=
            kalScnprintf(pcCommand + i4BytesWritten,
                         i4TotalLen - i4BytesWritten, "%s-",
                         priv_driver_get_sgi_info(
                             &prHwWlanInfo->rWtblPeerCap) == 0 ?
                         "LGI" :
                         "SGI");
    }
    /* Rate index: [1M | 2M | 5.5M | 11M |
     * 6M | 9M | 12M | 18M | 24M | 36M | 48M | 54M |
     * MCS# ]
     */
    if (txmode == TX_RATE_MODE_CCK) {
        i4BytesWritten +=
            kalScnprintf(pcCommand + i4BytesWritten,
                         i4TotalLen - i4BytesWritten, "%s",
                         rate < 4 ? HW_TX_RATE_CCK_STR[rate] :
                         HW_TX_RATE_CCK_STR[4]);
    } else if (txmode == TX_RATE_MODE_OFDM) {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "%s", hw_rate_ofdm_str(rate));
    } else {
        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "MCS%d", rate);
    }

out_get_curr_ar_rate:
    if (prHwWlanInfo) {
        kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
                   sizeof(PARAM_HW_WLAN_INFO_T));
    }
    if (prQueryStaStatistics) {
        kalMemFree(prQueryStaStatistics, VIR_MEM_TYPE,
                   sizeof(PARAM_GET_STA_STATISTICS));
    }

    return i4BytesWritten;
}

/*----------------------------------------------------------------------------*/
/*
 * @ The function will set policy of ACL.
 *  0: disable ACL
 *  1: enable accept list
 *  2: enable deny list
 * example: iwpriv p2p0 driver "set_acl_policy 1"
 */
/*----------------------------------------------------------------------------*/
static int priv_driver_set_acl_policy(IN struct net_device *prNetDev,
                                      IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    P_BSS_INFO_T prBssInfo = NULL;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4Argc = 0, i4BytesWritten = 0, i4Ret = 0, i4Policy = 0;
    u8 ucRoleIdx = 0, ucBssIdx = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    /* get Bss Index from ndev */
    if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0) {
        return -1;
    }

    if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
        WLAN_STATUS_SUCCESS) {
        return -1;
    }

    prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

    DBGLOG(REQ, LOUD, "ucRoleIdx %hhu ucBssIdx %hhu\n", ucRoleIdx,
           ucBssIdx);
    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc < 2) {
        return -1;
    }

    i4Ret = kalkStrtou32(apcArgv[1], 0, &i4Policy);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "integer format error i4Ret=%d\n", i4Ret);
        return -1;
    }

    switch (i4Policy) {
    case PARAM_CUSTOM_ACL_POLICY_DISABLE:
    case PARAM_CUSTOM_ACL_POLICY_ACCEPT:
    case PARAM_CUSTOM_ACL_POLICY_DENY:
        prBssInfo->rACL.ePolicy = i4Policy;
        break;

    default:  /*Invalid argument */
        DBGLOG(REQ, ERROR, "Invalid ACL Policy=%d\n", i4Policy);
        return -1;
    }

    DBGLOG(REQ, TRACE, "ucBssIdx[%hhu] ACL Policy=%d\n", ucBssIdx,
           prBssInfo->rACL.ePolicy);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "ucBssIdx[%hhu] ACL Policy=%d\n",
                                   ucBssIdx, prBssInfo->rACL.ePolicy);

    /* check if the change in ACL affects any existent association */
    if (prBssInfo->rACL.ePolicy != PARAM_CUSTOM_ACL_POLICY_DISABLE) {
        p2pRoleUpdateACLEntry(prAdapter, ucBssIdx);
    }

    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

    return i4BytesWritten;
}

static s32 priv_driver_inspect_mac_addr(IN char *pcMacAddr)
{
    s32 i = 0;

    if (pcMacAddr == NULL) {
        return -1;
    }

    for (i = 0; i < 17; i++) {
        if ((i % 3 != 2) && (!kalIsXdigit(pcMacAddr[i]))) {
            DBGLOG(REQ, ERROR, "[%c] is not hex digit\n",
                   pcMacAddr[i]);
            return -1;
        }
        if ((i % 3 == 2) && (pcMacAddr[i] != ':')) {
            DBGLOG(REQ, ERROR, "[%c]separate symbol is error\n",
                   pcMacAddr[i]);
            return -1;
        }
    }

    if (pcMacAddr[17] != '\0') {
        DBGLOG(REQ, ERROR, "no null-terminated character\n");
        return -1;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*
 * @ The function will add entry to ACL for accept or deny list.
 *  example: iwpriv p2p0 driver "add_acl_entry 01:02:03:04:05:06"
 */
/*----------------------------------------------------------------------------*/
static int priv_driver_add_acl_entry(IN struct net_device *prNetDev,
                                     IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    P_BSS_INFO_T prBssInfo = NULL;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u8 aucMacAddr[MAC_ADDR_LEN] = { 0 };
    s32 i = 0, i4Argc = 0, i4BytesWritten = 0, i4Ret = 0;
    u8 ucRoleIdx = 0, ucBssIdx = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    /* get Bss Index from ndev */
    if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0) {
        return -1;
    }

    if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
        WLAN_STATUS_SUCCESS) {
        return -1;
    }

    prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

    DBGLOG(REQ, LOUD, "ucRoleIdx %hhu ucBssIdx %hhu\n", ucRoleIdx,
           ucBssIdx);
    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc < 2) {
        return -1;
    }

    i4Ret = priv_driver_inspect_mac_addr(apcArgv[1]);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "inspect mac format error u4Ret=%d\n",
               i4Ret);
        return -1;
    }

    i4Ret = sscanf(apcArgv[1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                   &aucMacAddr[0], &aucMacAddr[1], &aucMacAddr[2],
                   &aucMacAddr[3], &aucMacAddr[4], &aucMacAddr[5]);

    if (i4Ret != MAC_ADDR_LEN) {
        DBGLOG(REQ, ERROR, "sscanf mac format fail u4Ret=%d\n", i4Ret);
        return -1;
    }

    for (i = 0; i <= prBssInfo->rACL.u4Num; i++) {
        if (memcmp(prBssInfo->rACL.rEntry[i].aucAddr, &aucMacAddr,
                   MAC_ADDR_LEN) == 0) {
            DBGLOG(REQ, ERROR,
                   "add this mac [" MACSTR "] is duplicate.\n",
                   MAC2STR(aucMacAddr));
            return -1;
        }
    }

    if ((i < 1) || (i > MAX_NUMBER_OF_ACL)) {
        DBGLOG(REQ, ERROR, "idx[%d] error or ACL is full.\n", i);
        return -1;
    }

    memcpy(prBssInfo->rACL.rEntry[i - 1].aucAddr, &aucMacAddr,
           MAC_ADDR_LEN);
    prBssInfo->rACL.u4Num = i;
    DBGLOG(REQ, TRACE, "add mac addr [" MACSTR "] to ACL(%d).\n",
           MAC2STR(prBssInfo->rACL.rEntry[i - 1].aucAddr), i);

    i4BytesWritten += kalScnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "add mac addr [" MACSTR "] to ACL(%d)\n",
        MAC2STR(prBssInfo->rACL.rEntry[i - 1].aucAddr), i);

    /* Check if the change in ACL affects any existent association. */
    if (prBssInfo->rACL.ePolicy == PARAM_CUSTOM_ACL_POLICY_DENY) {
        p2pRoleUpdateACLEntry(prAdapter, ucBssIdx);
    }

    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

    return i4BytesWritten;
}

/*----------------------------------------------------------------------------*/
/*
 * @ The function will delete entry to ACL for accept or deny list.
 *  example: iwpriv p2p0 driver "add_del_entry 01:02:03:04:05:06"
 */
/*----------------------------------------------------------------------------*/
static int priv_driver_del_acl_entry(IN struct net_device *prNetDev,
                                     IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    P_BSS_INFO_T prBssInfo = NULL;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u8 aucMacAddr[MAC_ADDR_LEN] = { 0 };
    s32 i = 0, j = 0, i4Argc = 0, i4BytesWritten = 0, i4Ret = 0;
    u8 ucRoleIdx = 0, ucBssIdx = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    /* get Bss Index from ndev */
    if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0) {
        return -1;
    }

    if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
        WLAN_STATUS_SUCCESS) {
        return -1;
    }

    prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

    DBGLOG(REQ, LOUD, "ucRoleIdx %hhu ucBssIdx %hhu\n", ucRoleIdx,
           ucBssIdx);
    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc < 2) {
        return -1;
    }

    i4Ret = priv_driver_inspect_mac_addr(apcArgv[1]);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "inspect mac format error u4Ret=%d\n",
               i4Ret);
        return -1;
    }

    i4Ret = sscanf(apcArgv[1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                   &aucMacAddr[0], &aucMacAddr[1], &aucMacAddr[2],
                   &aucMacAddr[3], &aucMacAddr[4], &aucMacAddr[5]);

    if (i4Ret != MAC_ADDR_LEN) {
        DBGLOG(REQ, ERROR, "sscanf mac format fail u4Ret=%d\n", i4Ret);
        return -1;
    }

    for (i = 0; i < prBssInfo->rACL.u4Num; i++) {
        if (memcmp(prBssInfo->rACL.rEntry[i].aucAddr, &aucMacAddr,
                   MAC_ADDR_LEN) == 0) {
            memset(&prBssInfo->rACL.rEntry[i], 0x00,
                   sizeof(PARAM_CUSTOM_ACL_ENTRY));
            DBGLOG(REQ, TRACE, "delete this mac [" MACSTR "]\n",
                   MAC2STR(aucMacAddr));

            i4BytesWritten += kalScnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "delete this mac [" MACSTR "] from ACL(%d)\n",
                MAC2STR(aucMacAddr), i + 1);
            break;
        }
    }

    if ((prBssInfo->rACL.u4Num == 0) || (i == MAX_NUMBER_OF_ACL)) {
        DBGLOG(REQ, ERROR, "delete entry fail, num of entries=%d\n", i);
        return -1;
    }

    for (j = i + 1; j < prBssInfo->rACL.u4Num; j++)
        memcpy(prBssInfo->rACL.rEntry[j - 1].aucAddr,
               prBssInfo->rACL.rEntry[j].aucAddr, MAC_ADDR_LEN);

    prBssInfo->rACL.u4Num = j - 1;
    memset(prBssInfo->rACL.rEntry[j - 1].aucAddr, 0x00, MAC_ADDR_LEN);

    /* check if the change in ACL affects any existent association */
    if (prBssInfo->rACL.ePolicy == PARAM_CUSTOM_ACL_POLICY_ACCEPT) {
        p2pRoleUpdateACLEntry(prAdapter, ucBssIdx);
    }

    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

    return i4BytesWritten;
}

/*----------------------------------------------------------------------------*/
/*
 * @ The function will show all entries to ACL for accept or deny list.
 *  example: iwpriv p2p0 driver "show_acl_entry"
 */
/*----------------------------------------------------------------------------*/
static int priv_driver_show_acl_entry(IN struct net_device *prNetDev,
                                      IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    P_BSS_INFO_T prBssInfo = NULL;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i = 0, i4Argc = 0, i4BytesWritten = 0;
    u8 ucRoleIdx = 0, ucBssIdx = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    /* get Bss Index from ndev */
    if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0) {
        return -1;
    }

    if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
        WLAN_STATUS_SUCCESS) {
        return -1;
    }

    prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);
    DBGLOG(REQ, TRACE, "ACL Policy = %d\n", prBssInfo->rACL.ePolicy);
    DBGLOG(REQ, TRACE, "Total ACLs = %d\n", prBssInfo->rACL.u4Num);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "ACL Policy = %d, Total ACLs = %d\n",
                                   prBssInfo->rACL.ePolicy,
                                   prBssInfo->rACL.u4Num);

    for (i = 0; i < prBssInfo->rACL.u4Num; i++) {
        DBGLOG(REQ, TRACE, "ACL(%d): [" MACSTR "]\n", i + 1,
               MAC2STR(prBssInfo->rACL.rEntry[i].aucAddr));

        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "ACL(%d): [" MACSTR "]\n", i + 1,
            MAC2STR(prBssInfo->rACL.rEntry[i].aucAddr));
    }

    return i4BytesWritten;
}

/*----------------------------------------------------------------------------*/
/*
 * @ The function will clear all entries to ACL for accept or deny list.
 *  example: iwpriv p2p0 driver "clear_acl_entry"
 */
/*----------------------------------------------------------------------------*/
static int priv_driver_clear_acl_entry(IN struct net_device *prNetDev,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    P_BSS_INFO_T prBssInfo = NULL;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4Argc = 0, i4BytesWritten = 0;
    u8 ucRoleIdx = 0, ucBssIdx = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    /* get Bss Index from ndev */
    if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) != 0) {
        return -1;
    }

    if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx, &ucBssIdx) !=
        WLAN_STATUS_SUCCESS) {
        return -1;
    }

    prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (prBssInfo->rACL.u4Num) {
        memset(&prBssInfo->rACL.rEntry[0], 0x00,
               sizeof(PARAM_CUSTOM_ACL_ENTRY) * MAC_ADDR_LEN);
        prBssInfo->rACL.u4Num = 0;
    }

    DBGLOG(REQ, TRACE, "ACL Policy = %d\n", prBssInfo->rACL.ePolicy);
    DBGLOG(REQ, TRACE, "Total ACLs = %d\n", prBssInfo->rACL.u4Num);

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "ACL Policy = %d, Total ACLs = %d\n",
                                   prBssInfo->rACL.ePolicy,
                                   prBssInfo->rACL.u4Num);

    /* check if the change in ACL affects any existent association */
    if (prBssInfo->rACL.ePolicy == PARAM_CUSTOM_ACL_POLICY_ACCEPT) {
        p2pRoleUpdateACLEntry(prAdapter, ucBssIdx);
    }

    return i4BytesWritten;
}

static int priv_driver_get_drv_mcr(IN struct net_device *prNetDev,
                                   IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 u4Ret;
    /* s32 i4ArgNum_with_ant_sel = 3; */ /* Add Antenna Selection Input */
    s32 i4ArgNum = 2;

    CMD_ACCESS_REG rCmdAccessReg;

    kalMemZero(&rCmdAccessReg, sizeof(CMD_ACCESS_REG));
    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "parse get_drv_mcr error (Address) u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        /* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
        rCmdAccessReg.u4Data = 0;

        DBGLOG(REQ, LOUD, "address is %x\n", rCmdAccessReg.u4Address);

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryDrvMcrRead,
                           &rCmdAccessReg, sizeof(rCmdAccessReg), true,
                           true, true, &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }

        i4BytesWritten = snprintf(pcCommand, i4TotalLen, "0x%08x",
                                  (unsigned int)rCmdAccessReg.u4Data);
        DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
               pcCommand);
    }

    return i4BytesWritten;
}

int priv_driver_set_drv_mcr(IN struct net_device *prNetDev, IN char *pcCommand,
                            IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Ret;
    /* s32 i4ArgNum_with_ant_sel = 4; */ /* Add Antenna Selection Input */
    s32 i4ArgNum = 3;

    CMD_ACCESS_REG rCmdAccessReg;

    kalMemZero(&rCmdAccessReg, sizeof(CMD_ACCESS_REG));
    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "parse get_drv_mcr error (Address) u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        u4Ret = kalkStrtou32(apcArgv[2], 0, &(rCmdAccessReg.u4Data));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "parse get_drv_mcr error (Data) u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvMcrWrite,
                           &rCmdAccessReg, sizeof(rCmdAccessReg), false,
                           false, true, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }
    }

    return i4BytesWritten;
}

static int priv_driver_get_sw_ctrl(IN struct net_device *prNetDev,
                                   IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 u4Ret = 0;

    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    kalMemZero(&rSwCtrlInfo, sizeof(PARAM_CUSTOM_SW_CTRL_STRUCT_T));
    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 2) {
        /* rSwCtrlInfo.u4Id = kalStrtoul(apcArgv[1], NULL, 0); */
        rSwCtrlInfo.u4Data = 0;
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rSwCtrlInfo.u4Id));
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse rSwCtrlInfo error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        DBGLOG(REQ, LOUD, "id is %x\n", rSwCtrlInfo.u4Id);

        rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead,
                           &rSwCtrlInfo, sizeof(rSwCtrlInfo), true,
                           true, true, &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }

        i4BytesWritten = snprintf(pcCommand, i4TotalLen, "0x%08x",
                                  (unsigned int)rSwCtrlInfo.u4Data);
        DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
               pcCommand);
    }

    return i4BytesWritten;
}

int priv_driver_set_sw_ctrl(IN struct net_device *prNetDev, IN char *pcCommand,
                            IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0;

    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    kalMemZero(&rSwCtrlInfo, sizeof(PARAM_CUSTOM_SW_CTRL_STRUCT_T));
    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 3) {
        /* rSwCtrlInfo.u4Id = kalStrtoul(apcArgv[1], NULL, 0);
         *  rSwCtrlInfo.u4Data = kalStrtoul(apcArgv[2], NULL, 0);
         */
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rSwCtrlInfo.u4Id));
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse rSwCtrlInfo error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }
        u4Ret = kalkStrtou32(apcArgv[2], 0, &(rSwCtrlInfo.u4Data));
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse rSwCtrlInfo error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
                           &rSwCtrlInfo, sizeof(rSwCtrlInfo), false,
                           false, true, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }
    }

    return i4BytesWritten;
}

int priv_driver_set_fixed_rate(IN struct net_device *prNetDev,
                               IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    /* s32 u4Ret = 0; */
    u32 u4WCID = 0;
    u32 u4Mode = 0, u4Bw = 0, u4Mcs = 0, u4VhtNss = 0;
    u32 u4SGI = 0, u4Preamble = 0, u4STBC = 0, u4LDPC = 0, u4SpeEn = 0;
    s32 i4Recv = 0;
    s8 *this_char = NULL;
    u32 u4Id = 0xa0610000;
    u32 u4Data = 0x80000000;
    u32 u4Id2 = 0xa0600000;
    u8 u4Nsts = 1;
    u8 fgStatus = true;

    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    if (i4Argc == 0) {
        DBGLOG(REQ, ERROR, "%s: invalid argc=0\n", __func__);
        return -1;
    }
    DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

    this_char = kalStrStr(*apcArgv, "=");

    if (!this_char) {
        return -1;
    }

    this_char++;

    DBGLOG(REQ, LOUD, "string = %s\n", this_char);

    if (strnicmp(this_char, "auto", strlen("auto")) == 0) {
        i4Recv = 1;
    } else {
        i4Recv = sscanf(this_char, "%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
                        &(u4WCID), &(u4Mode), &(u4Bw), &(u4Mcs),
                        &(u4VhtNss), &(u4SGI), &(u4Preamble), &(u4STBC),
                        &(u4LDPC), &(u4SpeEn));

        DBGLOG(REQ, LOUD, "u4WCID=%d\nu4Mode=%d\nu4Bw=%d\n", u4WCID,
               u4Mode, u4Bw);
        DBGLOG(REQ, LOUD, "u4Mcs=%d\nu4VhtNss=%d\nu4SGI=%d\n", u4Mcs,
               u4VhtNss, u4SGI);
        DBGLOG(REQ, LOUD, "u4Preamble=%d\nu4STBC=%d\n", u4Preamble,
               u4STBC);
        DBGLOG(REQ, LOUD, "u4LDPC=%d\nu4SpeEn=%d\n", u4LDPC, u4SpeEn);
    }

    if (i4Recv == 1) {
        rSwCtrlInfo.u4Id = u4Id2;
        rSwCtrlInfo.u4Data = 0;

        rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
                           &rSwCtrlInfo, sizeof(rSwCtrlInfo), false,
                           false, true, &u4BufLen);
    } else if (i4Recv == 10) {
        rSwCtrlInfo.u4Id = u4Id;
        rSwCtrlInfo.u4Data = u4Data;

        if (u4SGI) {
            rSwCtrlInfo.u4Data |= BIT(30);
        }
        if (u4LDPC) {
            rSwCtrlInfo.u4Data |= BIT(29);
        }
        if (u4SpeEn) {
            rSwCtrlInfo.u4Data |= BIT(28);
        }
        if (u4STBC) {
            rSwCtrlInfo.u4Data |= BIT(11);
        }

        if (u4Bw <= 3) {
            rSwCtrlInfo.u4Data |= ((u4Bw << 26) & BITS(26, 27));
        } else {
            fgStatus = false;
            DBGLOG(INIT, ERROR,
                   "Wrong BW! BW20=0, BW40=1, BW80=2,BW160=3\n");
        }
        if (u4Mode <= 4) {
            rSwCtrlInfo.u4Data |= ((u4Mode << 6) & BITS(6, 8));

            switch (u4Mode) {
            case 0:
                if (u4Mcs <= 3) {
                    rSwCtrlInfo.u4Data |= u4Mcs;
                } else {
                    fgStatus = false;
                    DBGLOG(INIT, ERROR,
                           "CCK mode but wrong MCS!\n");
                }

                if (u4Preamble) {
                    rSwCtrlInfo.u4Data |= BIT(2);
                }else{
                    rSwCtrlInfo.u4Data &= ~BIT(2);
                }

                break;

            case 1:
                switch (u4Mcs) {
                case 0:
                    /* 6'b001011 */
                    rSwCtrlInfo.u4Data |= 11;
                    break;

                case 1:
                    /* 6'b001111 */
                    rSwCtrlInfo.u4Data |= 15;
                    break;

                case 2:
                    /* 6'b001010 */
                    rSwCtrlInfo.u4Data |= 10;
                    break;

                case 3:
                    /* 6'b001110 */
                    rSwCtrlInfo.u4Data |= 14;
                    break;

                case 4:
                    /* 6'b001001 */
                    rSwCtrlInfo.u4Data |= 9;
                    break;

                case 5:
                    /* 6'b001101 */
                    rSwCtrlInfo.u4Data |= 13;
                    break;

                case 6:
                    /* 6'b001000 */
                    rSwCtrlInfo.u4Data |= 8;
                    break;

                case 7:
                    /* 6'b001100 */
                    rSwCtrlInfo.u4Data |= 12;
                    break;

                default:
                    fgStatus = false;
                    DBGLOG(INIT, ERROR,
                           "OFDM mode but wrong MCS!\n");
                    break;
                }
                break;

            case 2:
            case 3:
                if (u4Mcs <= 32) {
                    rSwCtrlInfo.u4Data |= u4Mcs;
                } else {
                    fgStatus = false;
                    DBGLOG(INIT, ERROR,
                           "HT mode but wrong MCS!\n");
                }

                if (u4Mcs != 32) {
                    u4Nsts += (u4Mcs >> 3);
                    if (u4STBC && (u4Nsts == 1)) {
                        u4Nsts++;
                    }
                }
                break;

            case 4:
                if (u4Mcs <= 9) {
                    rSwCtrlInfo.u4Data |= u4Mcs;
                } else {
                    fgStatus = false;
                    DBGLOG(INIT, ERROR,
                           "VHT mode but wrong MCS!\n");
                }
                if (u4STBC && (u4VhtNss == 1)) {
                    u4Nsts++;
                }else{
                    u4Nsts = u4VhtNss;
                }
                break;

            default:
                break;
            }
        } else {
            fgStatus = false;
            DBGLOG(INIT,
                   ERROR,
                   "Wrong TxMode! CCK=0, OFDM=1, HT=2, GF=3, VHT=4\n");
        }

        rSwCtrlInfo.u4Data |= (((u4Nsts - 1) << 9) & BITS(9, 10));

        if (fgStatus) {
            rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
                               &rSwCtrlInfo, sizeof(rSwCtrlInfo),
                               false, false, true, &u4BufLen);
        }

        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }
    } else {
        DBGLOG(INIT, ERROR, "iwpriv wlanXX driver FixedRate=Option\n");
        DBGLOG(INIT,
               ERROR,
               "Option:[WCID]-[Mode]-[BW]-[MCS]-[VhtNss]-[SGI]-[Preamble]-[STBC]-[LDPC]-[SPE_EN]\n");
        DBGLOG(INIT, ERROR, "[WCID]Wireless Client ID\n");
        DBGLOG(INIT, ERROR, "[Mode]CCK=0, OFDM=1, HT=2, GF=3, VHT=4\n");
        DBGLOG(INIT, ERROR, "[BW]BW20=0, BW40=1, BW80=2,BW160=3\n");
        DBGLOG(INIT, ERROR,
               "[MCS]CCK=0~3, OFDM=0~7, HT=0~32, VHT=0~9\n");
        DBGLOG(INIT, ERROR, "[VhtNss]VHT=1~4, Other=ignore\n");
        DBGLOG(INIT, ERROR, "[Preamble]Long=0, Other=Short\n");
    }

    return i4BytesWritten;
}

#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT
int priv_driver_set_fixed_mrate(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    /* s32 u4Ret = 0; */
    u32 u4WCID = 0;
    u32 u4Mode = 0, u4Bw = 0, u4Mcs = 0, u4VhtNss = 0;
    u32 u4SGI = 0, u4Preamble = 0, u4STBC = 0, u4LDPC = 0, u4SpeEn = 0;
    s32 i4Recv = 0;
    s8 *this_char = NULL;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

    this_char = kalStrStr(*apcArgv, "=");

    if (!this_char) {
        return -1;
    }

    this_char++;
    DBGLOG(REQ, LOUD, "string = %s\n", this_char);

    if (strnicmp(this_char, "auto", strlen("auto")) == 0) {
        i4Recv = 1;
    } else {
        i4Recv = sscanf(this_char, "%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
                        &(u4WCID), &(u4Mode), &(u4Bw), &(u4Mcs),
                        &(u4VhtNss), &(u4SGI), &(u4Preamble), &(u4STBC),
                        &(u4LDPC), &(u4SpeEn));

        DBGLOG(REQ, LOUD, "u4WCID=%d\nu4Mode=%d\nu4Bw=%d\n", u4WCID,
               u4Mode, u4Bw);
        DBGLOG(REQ, LOUD, "u4Mcs=%d\nu4VhtNss=%d\nu4SGI=%d\n", u4Mcs,
               u4VhtNss, u4SGI);
        DBGLOG(REQ, LOUD, "u4Preamble=%d\nu4STBC=%d\n", u4Preamble,
               u4STBC);
        DBGLOG(REQ, LOUD, "u4LDPC=%d\nu4SpeEn=%d\n", u4LDPC, u4SpeEn);
    }

    if (i4Recv == 1) {
        /* Recovery Mrate */
        prAdapter->fgIsFixedMRate = false;
    } else if (i4Recv == 10) {
        u16 u2FixedRateCode;
        u8 ucRatePreamble, ucRateIndex;
        MSDU_INFO_T rMsduInfo;

        kalMemZero(&rMsduInfo, sizeof(MSDU_INFO_T));

        switch (u4Mode) {
        case 0:
            if (u4Preamble) {
                ucRatePreamble = PREAMBLE_OPTION_SHORT;
            }else{
                ucRatePreamble = PREAMBLE_DEFAULT_LONG_NONE;
            }

            if (u4Mcs > RATE_11M_INDEX) {
                ucRateIndex = RATE_11M_INDEX;
            }else{
                ucRateIndex = (u8)u4Mcs;
            }
            break;

        case 1:
            ucRatePreamble = PREAMBLE_OFDM_MODE;
            if (u4Mcs > RATE_54M_INDEX) {
                ucRateIndex = RATE_54M_INDEX;
            }else{
                ucRateIndex = (u8)u4Mcs;
            }
            break;

        case 2:
            ucRatePreamble = PREAMBLE_HT_MIXED_MODE;

            if ((u4Mcs >= 0) && (u4Mcs <= 7)) {
                ucRateIndex = (u8)u4Mcs + HT_RATE_MCS0_INDEX;
            } else if (u4Mcs == 32) {
                ucRateIndex = HT_RATE_MCS32_INDEX;
            } else {
                ucRateIndex = HT_RATE_MCS7_INDEX;
            }
            break;

        case 3:
            ucRatePreamble = PREAMBLE_HT_GREEN_FIELD;

            if ((u4Mcs >= 0) && (u4Mcs <= 7)) {
                ucRateIndex = (u8)u4Mcs + HT_RATE_MCS0_INDEX;
            } else if (u4Mcs == 32) {
                ucRateIndex = HT_RATE_MCS32_INDEX;
            } else {
                ucRateIndex = HT_RATE_MCS7_INDEX;
            }
            break;

        case 4:
            ucRatePreamble = PREAMBLE_VHT_FIELD;
            if (u4Mcs > VHT_RATE_MCS9_INDEX) {
                ucRateIndex = VHT_RATE_MCS9_INDEX;
            }else{
                ucRateIndex = (u8)u4Mcs;
            }
            break;

        default:
            DBGLOG(INIT, ERROR, "Unknown data rate mode\n");
            DBGLOG(INIT, ERROR,
                   " [Mode]CCK=0, OFDM=1, HT=2, GF=3, VHT=4\n");
            return -1;

            break;
        }

        nicRateIndex2RateCode(ucRatePreamble, ucRateIndex,
                              &u2FixedRateCode);

        nicTxSetPktFixedRateOption(&rMsduInfo, u2FixedRateCode,
                                   FIX_BW_NO_FIXED,
                                   (u4SGI) ? true : false, false);

        prAdapter->rWifiVar.u4DataTxRateCode =
            rMsduInfo.u4FixedRateOption;
        /* Recovery Enable fix Mrate */
        prAdapter->fgIsFixedMRate = true;
    } else {
        DBGLOG(INIT, ERROR, "iwpriv wlanXX driver FixedMRate=Option\n");
        DBGLOG(INIT,
               ERROR,
               "Option:[WCID]-[Mode]-[BW]-[MCS]-[VhtNss]-[SGI]-[Preamble]-[STBC]-[LDPC]-[SPE_EN]\n");
        // DBGLOG(INIT, ERROR, "[WCID]Wireless Client ID\n");
        DBGLOG(INIT, ERROR, "[WCID] ** any number ** don't care\n");
        DBGLOG(INIT, ERROR, "[Mode]CCK=0, OFDM=1, HT=2, GF=3, VHT=4\n");
        DBGLOG(INIT, ERROR, "[BW] ** any number ** don't care\n");
        // DBGLOG(INIT, ERROR, "[BW]BW20=0, BW40=1, BW80=2,BW160=3\n");
        DBGLOG(INIT, ERROR,
               "[MCS]CCK=0~3, OFDM=0~7, HT=0~32, VHT=0~9\n");
        // DBGLOG(INIT, ERROR, "[VhtNss]VHT=1~4, Other=ignore\n");
        DBGLOG(INIT, ERROR, "[VhtNss] ** any number ** don't care\n");
        DBGLOG(INIT, ERROR, "[Preamble]Long=0, Other=Short\n");
        DBGLOG(INIT, ERROR, "[STBC] ** any number ** don't care\n");
        DBGLOG(INIT, ERROR, "[LDPC] ** any number ** don't care\n");
        DBGLOG(INIT, ERROR, "[SPE_EN] ** any number ** don't care\n");
    }
    return i4BytesWritten;
}

int priv_driver_set_dup_mpacket(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;
    u32 u4Ret = 0;
    u8 ucDupMPktNum = 0;
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    u4Ret = kalkStrtou8(apcArgv[1], 0, &ucDupMPktNum);

    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse DupMPkt number error u4Ret=%d\n",
               u4Ret);
        return i4BytesWritten;
    }

#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
    if (prAdapter->fgIsLookBackMode == true) {
        i4BytesWritten = snprintf(
            pcCommand,
            i4TotalLen,
            "Dulpicated packets return, Look Back Mode is enabled (0x%04x) \n",
            (unsigned int)prAdapter->fgIsLookBackMode);
        return i4BytesWritten;
    }
#endif

    DBGLOG(PF, INFO, "Duplicate Multicast number %d\n", ucDupMPktNum);

    prAdapter->ucDupMcastPacketNum = ucDupMPktNum;
    return i4BytesWritten;
}
int priv_driver_get_dup_mpacket(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                              "duplicated packet number : 0x%04x",
                              (unsigned int)prAdapter->ucDupMcastPacketNum);
    return i4BytesWritten;
}

int priv_driver_set_mcast_burst(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;
    u32 u4Ret = 0;
    u8 ucMCastBrust = 0;
    u8 ucAc = TC2_INDEX;
    PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T rChipConfigInfo;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;
    kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));

    DBGLOG(REQ, INFO, "command is %s\n", pcCommand);

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    if (i4Argc < 2) {
        DBGLOG(REQ, ERROR,
               "Set_Mcastburst [0|1] [AC_IDX:0~3 default:2]\n");
        return -EFAULT;
    }

    u4Ret = kalkStrtou8(apcArgv[1], 0, &ucMCastBrust);
    if (u4Ret) {
        DBGLOG(REQ, ERROR, "parse mcast McastMode error u4Ret=%d\n",
               u4Ret);
        return i4BytesWritten;
    }

    if (i4Argc == 3) {
        u4Ret = kalkStrtou8(apcArgv[2], 0, &ucAc);
    }

    DBGLOG(REQ, STATE, "Multicast Burst Mode %d\n", ucMCastBrust);

    if (!prAdapter->fgIsMcastBurstMode && !ucMCastBrust) {
        DBGLOG(REQ, WARN,
               "Multicast Burst Mode(%d) has already been disabled\n",
               prAdapter->fgIsMcastBurstMode);
        return 0;
    }

    if (ucMCastBrust > 0) {
        prAdapter->fgIsMcastBurstMode = true;
        qmFuncChangeBmcTcIdx((ucAc < TC4_INDEX) ? (ucAc) : (TC2_INDEX));
        DBGLOG(REQ, STATE, "Multicast Burst AC Index %d\n",
               ((ucAc < TC4_INDEX) ? (ucAc) : (TC2_INDEX)));
    } else {
        prAdapter->fgIsMcastBurstMode = false;
        qmFuncChangeBmcTcIdx(BMC_TC_INDEX);
        DBGLOG(REQ, STATE,
               "Multicast Burst back to default AC Index:%d\n",
               BMC_TC_INDEX);
    }

    /* Adding McastBurstEn  */
    rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
    i4BytesWritten = kalScnprintf(rChipConfigInfo.aucCmd,
                                  CHIP_CONFIG_RESP_SIZE, "McastBurst %d\n",
                                  prAdapter->fgIsMcastBurstMode);

    if (i4BytesWritten > 0) {
        rChipConfigInfo.u2MsgSize = (u16)i4BytesWritten;
        kalIoctl(prGlueInfo, wlanoidSetChipConfig, &rChipConfigInfo,
                 sizeof(rChipConfigInfo), false, false, true, &u4Ret);
    }

    return 0;
}
int priv_driver_get_mcast_burst(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;
#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
    u8 i;
#endif

    DBGLOG(REQ, LOUD, "get mcast burst\n");

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    i4BytesWritten = snprintf(pcCommand, i4TotalLen, "MBurstMode: 0x%01x ",
                              (unsigned int)prAdapter->fgIsMcastBurstMode);
#if (HIF_TX_RSRC_WMM_ENHANCE == 1)
    for (i = 0; i < MAX_BSSID_NUM; i++) {
        i4BytesWritten += kalSnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "WmmSetIdx:%d TC_IDX:%d\n", i,
            arTcRemapTable[i][qmFuncGetBmcTcIdx(i)]);
    }
#endif
    return i4BytesWritten;
}
int priv_driver_set_txop(IN struct net_device *prNetDev, IN char *pcCommand,
                         IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;
    u32 u4Ret = 0;
    u16 u2TXOPPeriod = 0;
    CMD_ACCESS_REG rCmdAccessReg;
    u32 u4BufLen = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    u4Ret = kalkStrtou16(apcArgv[1], 0, &u2TXOPPeriod);

    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse txop period  error u4Ret=%d\n", u4Ret);
        return i4BytesWritten;
    }

    DBGLOG(REQ, LOUD, "The setting TXOP period (u2TXOPPeriod) is 0x%04x\n",
           u2TXOPPeriod);

    // The maxmium of TXOP
    if (u2TXOPPeriod >= 0x68) {
        prAdapter->u4TXOPPeriod = 0x0068;
    } else {
        prAdapter->u4TXOPPeriod = u2TXOPPeriod;
    }
    prAdapter->u4TXOPPeriod |= (prAdapter->u4TXOPPeriod << 16);

    DBGLOG(REQ, ERROR, "The setting TXOP period (u4TXOPPeriod) is 0x%08x\n",
           prAdapter->u4TXOPPeriod);

    // Write down to register
    rCmdAccessReg.u4Data = prAdapter->u4TXOPPeriod;

    /* Set TXOP for extenad setting 3.3ms */
    rCmdAccessReg.u4Address = 0x820f4010;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvMcrWrite, &rCmdAccessReg,
                       sizeof(rCmdAccessReg), false, false, true,
                       &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "TXOP Add: 0x%08x value: 0x%08x\n",
           rCmdAccessReg.u4Address, rCmdAccessReg.u4Data);

    rCmdAccessReg.u4Address = 0x820f4014;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvMcrWrite, &rCmdAccessReg,
                       sizeof(rCmdAccessReg), false, false, true,
                       &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "TXOP Add: 0x%08x value: 0x%08x\n",
           rCmdAccessReg.u4Address, rCmdAccessReg.u4Data);

    rCmdAccessReg.u4Address = 0x820f4018;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvMcrWrite, &rCmdAccessReg,
                       sizeof(rCmdAccessReg), false, false, true,
                       &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "TXOP Add: 0x%08x value: 0x%08x\n",
           rCmdAccessReg.u4Address, rCmdAccessReg.u4Data);

    rCmdAccessReg.u4Address = 0x820f401c;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvMcrWrite, &rCmdAccessReg,
                       sizeof(rCmdAccessReg), false, false, true,
                       &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "TXOP Add: 0x%08x value: 0x%08x\n",
           rCmdAccessReg.u4Address, rCmdAccessReg.u4Data);

    /* Set TXOP for extenad setting 3.3ms */
    rCmdAccessReg.u4Address = 0x820f4060;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvMcrWrite, &rCmdAccessReg,
                       sizeof(rCmdAccessReg), false, false, true,
                       &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "TXOP Add: 0x%08x value: 0x%08x\n",
           rCmdAccessReg.u4Address, rCmdAccessReg.u4Data);

    rCmdAccessReg.u4Address = 0x820f4064;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvMcrWrite, &rCmdAccessReg,
                       sizeof(rCmdAccessReg), false, false, true,
                       &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "TXOP Add: 0x%08x value: 0x%08x\n",
           rCmdAccessReg.u4Address, rCmdAccessReg.u4Data);

    rCmdAccessReg.u4Address = 0x820f4068;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvMcrWrite, &rCmdAccessReg,
                       sizeof(rCmdAccessReg), false, false, true,
                       &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "TXOP Add: 0x%08x value: 0x%08x\n",
           rCmdAccessReg.u4Address, rCmdAccessReg.u4Data);

    rCmdAccessReg.u4Address = 0x820f406c;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetDrvMcrWrite, &rCmdAccessReg,
                       sizeof(rCmdAccessReg), false, false, true,
                       &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "TXOP Add: 0x%08x value: 0x%08x\n",
           rCmdAccessReg.u4Address, rCmdAccessReg.u4Data);

    return i4BytesWritten;
}
int priv_driver_get_txop(IN struct net_device *prNetDev, IN char *pcCommand,
                         IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    i4BytesWritten =
        snprintf(pcCommand, i4TotalLen,
                 "TXOP Period: 0x%04x, it shuold be between 0~0x0068\n",
                 (unsigned int)(prAdapter->u4TXOPPeriod >> 16));
    return i4BytesWritten;
}

#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
int priv_driver_set_look_back(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;
    u32 u4Ret = 0;
    u8 ucLookBackMode = 0;
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    u4Ret = kalkStrtou8(apcArgv[1], 0, &ucLookBackMode);

    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse Look Back error u4Ret=%d\n", u4Ret);
        return i4BytesWritten;
    }

    DBGLOG(PF, INFO, "Look Back Mode %d\n", ucLookBackMode);

    if (prAdapter->ucDupMcastPacketNum > 0) {
        i4BytesWritten = snprintf(
            pcCommand,
            i4TotalLen,
            "Look Back Mode return, Duplicate Num is not 0 (0x%04x) \n",
            (unsigned int)prAdapter->ucDupMcastPacketNum);
        return i4BytesWritten;
    }

    if (ucLookBackMode > 0) {
        prAdapter->fgIsLookBackMode = true;
    } else {
        prAdapter->fgIsLookBackMode = false;
    }

    return i4BytesWritten;
}
int priv_driver_get_look_back(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                              "Look Back Mode: 0x%04x",
                              (unsigned int)prAdapter->fgIsLookBackMode);
    return i4BytesWritten;
}
#endif
int priv_driver_set_11mc_type(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;
    u32 u4Ret = 0;
    u8 uc11mcType = 0;
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    u4Ret = kalkStrtou8(apcArgv[1], 0, &uc11mcType);

    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse 11mc type error u4Ret=%d\n", u4Ret);
        return i4BytesWritten;
    }

    DBGLOG(PF, ERROR, "Look Back Mode %d\n", uc11mcType);

    /* We define currnet support packet type
     *  0 -> disable packet parsing
     *  1 -> icmp
     *  2 -> multicast
     *  3 -> TOS : ip header 0x20
     */
    prAdapter->uc11mcSupportType = uc11mcType;
    i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                              "11MC Support Type (0x%02x) \n",
                              (unsigned int)prAdapter->uc11mcSupportType);

    return i4BytesWritten;
}
int priv_driver_get_11mc_type(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                              "11mc support type: 0x%02x",
                              (unsigned int)prAdapter->uc11mcSupportType);
    return i4BytesWritten;
}

int priv_driver_set_unicast_burst(IN struct net_device *prNetDev,
                                  IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;
    u32 u4Ret = 0;
    u8 ucUcastBurst = 0;
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    u4Ret = kalkStrtou8(apcArgv[1], 0, &ucUcastBurst);

    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse Unicast burst mode error u4Ret=%d\n",
               u4Ret);
        return i4BytesWritten;
    }

    DBGLOG(PF, INFO, "Unicast Burst Mode %d\n", ucUcastBurst);

    if (ucUcastBurst > 0) {
        prAdapter->fgIsUcastBurstMode = true;
        prAdapter->ucUnicastBurstTimeout = 10;
    } else {
        prAdapter->fgIsUcastBurstMode = false;
        prAdapter->ucUnicastBurstTimeout = 0;
    }

    return i4BytesWritten;
}
int priv_driver_get_unicast_burst(IN struct net_device *prNetDev,
                                  IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;
    printk("get unicast burst mode\n");

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                              "Unicast Burst Mode: 0x%04x",
                              (unsigned int)prAdapter->fgIsUcastBurstMode);
    return i4BytesWritten;
}

int priv_driver_set_unicast_burst_timeout(IN struct net_device *prNetDev,
                                          IN char *pcCommand,
                                          IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;
    u32 u4Ret = 0;
    u8 ucUnicastBrustTimeout = 0;
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    u4Ret = kalkStrtou8(apcArgv[1], 0, &ucUnicastBrustTimeout);

    if (u4Ret) {
        DBGLOG(REQ, LOUD, "Unicast BurstMode Timeout = %d\n", u4Ret);
        return i4BytesWritten;
    }

    DBGLOG(PF, ERROR, "Unicast BurstMode Timeout %d\n",
           ucUnicastBrustTimeout);

    prAdapter->ucUnicastBurstTimeout = ucUnicastBrustTimeout;
    i4BytesWritten = snprintf(
        pcCommand, i4TotalLen, "Set Unicast Burst Timeout (%d) ms \n",
        (unsigned int)prAdapter->ucUnicastBurstTimeout);

    return i4BytesWritten;
}
int priv_driver_get_unicast_burst_timeout(IN struct net_device *prNetDev,
                                          IN char *pcCommand,
                                          IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    i4BytesWritten =
        snprintf(pcCommand, i4TotalLen, "Unicast Burst Timeout: %d ms",
                 (unsigned int)prAdapter->ucUnicastBurstTimeout);
    return i4BytesWritten;
}

int priv_driver_set_mrm_clinet(IN struct net_device *prNetDev,
                               IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;
    u32 u4Ret = 0;
    u8 ucMRMClientMode = 0;
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    u4Ret = kalkStrtou8(apcArgv[1], 0, &ucMRMClientMode);

    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse MRM client mode error u4Ret=%d\n",
               u4Ret);
        return i4BytesWritten;
    }

    DBGLOG(PF, INFO, "Unicast Burst Mode %d\n", ucMRMClientMode);

    if (ucMRMClientMode > 0) {
        prAdapter->rWifiVar.ucTpTestMode = ENUM_TP_TEST_MODE_AUDIO_MRM;
        /* update MRM client mode */
        DBGLOG(REQ, ERROR, "Enable MRM Client Mode %d\n",
               prAdapter->rWifiVar.ucTpTestMode);
        /* clean all the setting  before start to set new mode*/
        nicEnterTPTestMode(prAdapter, TEST_MODE_NONE);
        /* Update new setting before to new mode*/
        nicEnterTPTestMode(prAdapter, TEST_MODE_AUDIO_MRM);
    } else {
        prAdapter->rWifiVar.ucTpTestMode = ENUM_TP_TEST_MODE_NORMAL;
        nicEnterTPTestMode(prAdapter, TEST_MODE_NONE);
    }

    i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                              "Set MRM Clinet mode (%d) to TP mode (%d)\n",
                              ucMRMClientMode,
                              prAdapter->rWifiVar.ucTpTestMode);

    return i4BytesWritten;
}

int priv_driver_get_mrm_client(IN struct net_device *prNetDev,
                               IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;
    u8 ucMRMClient = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    if (prAdapter->rWifiVar.ucTpTestMode == ENUM_TP_TEST_MODE_AUDIO_MRM) {
        ucMRMClient = true;
    }

    i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                              "Get MRM Clinet mode (%d) & TP mode (%d)\n",
                              ucMRMClient,
                              prAdapter->rWifiVar.ucTpTestMode);
    return i4BytesWritten;
}

int priv_driver_set_audio_tos(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;
    u32 u4Ret = 0;
    u8 ucAudioTOS = 0;
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    u4Ret = kalkStrtou8(apcArgv[1], 0, &ucAudioTOS);

    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse Audio TOS type error u4Ret=%d\n",
               u4Ret);
        return i4BytesWritten;
    }

    DBGLOG(REQ, ERROR, "Specific Audio TOS %02x\n", ucAudioTOS);

    prAdapter->ucAudioTOS = ucAudioTOS;
    i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                              "Specific Audio TOS (0x%02x) \n",
                              (unsigned int)prAdapter->ucAudioTOS);

    return i4BytesWritten;
}
int priv_driver_get_audio_tos(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                              "Specific Audio TOS: 0x%02x",
                              (unsigned int)prAdapter->ucAudioTOS);
    return i4BytesWritten;
}
#endif

int priv_driver_set_cfg(IN struct net_device *prNetDev, IN char *pcCommand,
                        IN int i4TotalLen)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };

    PARAM_CUSTOM_KEY_CFG_STRUCT_T rKeyCfgInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);
    prAdapter = prGlueInfo->prAdapter;

    kalMemZero(&rKeyCfgInfo, sizeof(rKeyCfgInfo));

    if (i4Argc >= 3) {
        s8 ucTmp[WLAN_CFG_VALUE_LEN_MAX];
        u8 *pucCurrBuf = ucTmp;
        u8 i = 0;
        s32 i4TmpBufLen = 0;

        kalMemZero(ucTmp, WLAN_CFG_VALUE_LEN_MAX);

        if (i4Argc == 3) {
            /*no space for it, driver can't accept space in the end
             * of the line*/
            /*ToDo: skip the space when parsing*/
            scnprintf(pucCurrBuf, sizeof(ucTmp), "%s", apcArgv[2]);
        } else {
            for (i = 2; i < i4Argc; i++)
                i4TmpBufLen +=
                    scnprintf((pucCurrBuf + i4TmpBufLen),
                              (sizeof(ucTmp) - i4TmpBufLen),
                              "%s", apcArgv[i]);
        }

        DBGLOG(INIT, WARN, "Update to driver temp buffer as [%s]\n",
               ucTmp);

        /* wlanCfgSet(prAdapter, apcArgv[1], apcArgv[2], 0); */
        /* Call by  wlanoid because the set_cfg will trigger callback */
        kalStrnCpy(rKeyCfgInfo.aucKey, apcArgv[1],
                   WLAN_CFG_KEY_LEN_MAX - 1);
        rKeyCfgInfo.aucKey[WLAN_CFG_KEY_LEN_MAX - 1] = '\0';
        kalStrnCpy(rKeyCfgInfo.aucValue, ucTmp,
                   WLAN_CFG_KEY_LEN_MAX - 1);
        rKeyCfgInfo.aucValue[WLAN_CFG_KEY_LEN_MAX - 1] = '\0';
        rStatus = kalIoctl(prGlueInfo, wlanoidSetKeyCfg, &rKeyCfgInfo,
                           sizeof(rKeyCfgInfo), false, false, true,
                           &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }
    }

    return i4BytesWritten;
}  /* priv_driver_set_cfg  */

int priv_driver_get_cfg(IN struct net_device *prNetDev, IN char *pcCommand,
                        IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s8 aucValue[WLAN_CFG_VALUE_LEN_MAX];

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);
    prAdapter = prGlueInfo->prAdapter;

    if (i4Argc >= 2) {
        /* by wlanoid ? */
        if (wlanCfgGet(prAdapter, apcArgv[1], aucValue, "", 0) ==
            WLAN_STATUS_SUCCESS) {
            kalStrnCpy(pcCommand, aucValue,
                       WLAN_CFG_VALUE_LEN_MAX - 1);
            pcCommand[WLAN_CFG_KEY_LEN_MAX - 1] = '\0';
            i4BytesWritten = kalStrnLen(pcCommand,
                                        WLAN_CFG_VALUE_LEN_MAX - 1);
        }
    }

    return i4BytesWritten;
}  /* priv_driver_get_cfg  */

int priv_driver_set_chip_config(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    u32 u4CmdLen = 0;
    u32 u4PrefixLen = 0;
    /* s32 i4Argc = 0; */
    /* s8 *  apcArgv[WLAN_CFG_ARGV_MAX] = {0}; */
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    char *pcTmpCommand;
    u32 u4StrLen;

    PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T rChipConfigInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    /* wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv); */
    /* DBGLOG(REQ, LOUD,("argc is %i\n",i4Argc)); */
    /*  */

    u4StrLen = kalStrLen(pcCommand);
    pcTmpCommand = (char *)kalMemAlloc(u4StrLen + 1, VIR_MEM_TYPE);

    if (!pcTmpCommand) {
        DBGLOG(REQ, ERROR, "TmpCmd : Memory alloc failed\n");
        return -1;
    }

    kalStrnCpy(pcTmpCommand, pcCommand, u4StrLen);
    pcTmpCommand[u4StrLen] = '\0';

    wlanCfgParseArgument(pcTmpCommand, &i4Argc, apcArgv);

    /* KeepFullPower Enable cmd is blocked when entering suspend mode */
    if ((i4Argc == 3) && (apcArgv[0] != NULL) && (apcArgv[1] != NULL) &&
        (apcArgv[2] != NULL)) {
        if ((kalStrnCmp("KeepFullPwr", apcArgv[1], 11) == 0) &&
            (kalStrnCmp("1", apcArgv[2], 1) == 0) &&
            (prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap &
             BLOCK_KEEP_FULL_PWR)) {
            DBGLOG(REQ, STATE,
                   "KeepFullPower Enable Command is blocked\n");
            kalMemFree(pcTmpCommand, VIR_MEM_TYPE, u4StrLen + 1);
            return 0;
        }
    }

    kalMemFree(pcTmpCommand, VIR_MEM_TYPE, u4StrLen + 1);

    u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);
    u4PrefixLen = kalStrLen(CMD_SET_CHIP) + 1  /*space */;

    kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));

    /* if(i4Argc >= 2) { */
    if (u4CmdLen > u4PrefixLen) {
        rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
        /* rChipConfigInfo.u2MsgSize =
         * kalStrnLen(apcArgv[1],CHIP_CONFIG_RESP_SIZE); */
        rChipConfigInfo.u2MsgSize = u4CmdLen - u4PrefixLen;
        /* kalStrnCpy(rChipConfigInfo.aucCmd,apcArgv[1],CHIP_CONFIG_RESP_SIZE);
         */
        kalStrnCpy(rChipConfigInfo.aucCmd, pcCommand + u4PrefixLen,
                   CHIP_CONFIG_RESP_SIZE - 1);

        rStatus = kalIoctl(prGlueInfo, wlanoidSetChipConfig,
                           &rChipConfigInfo, sizeof(rChipConfigInfo),
                           false, false, true, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(REQ, INFO, "%s: kalIoctl ret=%d\n", __func__,
                   rStatus);
            i4BytesWritten = -1;
        }
    }

    return i4BytesWritten;
}  /* priv_driver_set_chip_config  */

void priv_driver_get_chip_config_16(u8 *pucStartAddr, u32 u4Length, u32 u4Line,
                                    int i4TotalLen, s32 i4BytesWritten,
                                    char *pcCommand)
{
    while (u4Length >= 16) {
        if (i4TotalLen > i4BytesWritten) {
            i4BytesWritten += snprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "%04lx %02x %02x %02x %02x  %02x %02x %02x %02x - %02x %02x %02x %02x  %02x %02x %02x %02x\n",
                u4Line,
                pucStartAddr[0],
                pucStartAddr[1],
                pucStartAddr[2],
                pucStartAddr[3],
                pucStartAddr[4],
                pucStartAddr[5],
                pucStartAddr[6],
                pucStartAddr[7],
                pucStartAddr[8],
                pucStartAddr[9],
                pucStartAddr[10],
                pucStartAddr[11],
                pucStartAddr[12],
                pucStartAddr[13],
                pucStartAddr[14],
                pucStartAddr[15]);
        }

        pucStartAddr += 16;
        u4Length -= 16;
        u4Line += 16;
    }
}

void priv_driver_get_chip_config_4(u32 *pu4StartAddr, u32 u4Length, u32 u4Line,
                                   int i4TotalLen, s32 i4BytesWritten,
                                   char *pcCommand)
{
    while (u4Length >= 16) {
        if (i4TotalLen > i4BytesWritten) {
            i4BytesWritten += snprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "%04lx %08lx %08lx %08lx %08lx\n", u4Line,
                pu4StartAddr[0], pu4StartAddr[1],
                pu4StartAddr[2], pu4StartAddr[3]);
        }

        pu4StartAddr += 4;
        u4Length -= 16;
        u4Line += 4;
    }
}

int priv_driver_get_chip_config(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0;
    u32 u4BufLen = 0;
    u32 u2MsgSize = 0;
    u32 u4CmdLen = 0;
    u32 u4PrefixLen = 0;
    /* s32 i4Argc = 0; */
    /* s8 *  apcArgv[WLAN_CFG_ARGV_MAX]; */

    PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T rChipConfigInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    /* wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv); */
    /* DBGLOG(REQ, LOUD,("argc is %i\n",i4Argc)); */

    u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);
    u4PrefixLen = kalStrLen(CMD_GET_CHIP) + 1  /*space */;

    /* if(i4Argc >= 2) { */
    if (u4CmdLen > u4PrefixLen) {
        rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
        /* rChipConfigInfo.u2MsgSize =
         * kalStrnLen(apcArgv[1],CHIP_CONFIG_RESP_SIZE); */
        rChipConfigInfo.u2MsgSize = u4CmdLen - u4PrefixLen;
        /* kalStrnCpy(rChipConfigInfo.aucCmd,apcArgv[1],CHIP_CONFIG_RESP_SIZE);
         */
        kalStrnCpy(rChipConfigInfo.aucCmd, pcCommand + u4PrefixLen,
                   CHIP_CONFIG_RESP_SIZE - 1);
        rStatus = kalIoctl(prGlueInfo, wlanoidQueryChipConfig,
                           &rChipConfigInfo, sizeof(rChipConfigInfo),
                           true, true, true, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(REQ, INFO, "%s: kalIoctl ret=%d\n", __func__,
                   rStatus);
            return -1;
        }

        /* Check respType */
        u2MsgSize = rChipConfigInfo.u2MsgSize;
        DBGLOG(REQ, INFO, "%s: RespTyep  %u\n", __func__,
               rChipConfigInfo.ucRespType);
        DBGLOG(REQ, INFO, "%s: u2MsgSize %u\n", __func__,
               rChipConfigInfo.u2MsgSize);

        if (u2MsgSize > sizeof(rChipConfigInfo.aucCmd)) {
            DBGLOG(REQ, INFO, "%s: u2MsgSize error ret=%u\n",
                   __func__, rChipConfigInfo.u2MsgSize);
            return -1;
        }

        if (u2MsgSize > 0) {
            if (rChipConfigInfo.ucRespType ==
                CHIP_CONFIG_TYPE_ASCII) {
                i4BytesWritten = snprintf(
                    pcCommand + i4BytesWritten, i4TotalLen,
                    "%s", rChipConfigInfo.aucCmd);
            } else {
                u32 u4Length;
                u32 u4Line;

                if (rChipConfigInfo.ucRespType ==
                    CHIP_CONFIG_TYPE_MEM8) {
                    u8 *pucStartAddr = NULL;

                    pucStartAddr =
                        (u8 *)rChipConfigInfo.aucCmd;
                    /* align 16 bytes because one print line
                     * is 16 bytes */
                    u4Length = (((u2MsgSize + 15) >> 4))
                               << 4;
                    u4Line = 0;
                    priv_driver_get_chip_config_16(
                        pucStartAddr, u4Length, u4Line,
                        i4TotalLen, i4BytesWritten,
                        pcCommand);
                } else {
                    u32 *pu4StartAddr = NULL;

                    pu4StartAddr =
                        (u32 *)rChipConfigInfo.aucCmd;
                    /* align 16 bytes because one print line
                     * is 16 bytes */
                    u4Length = (((u2MsgSize + 15) >> 4))
                               << 4;
                    u4Line = 0;

                    if (IS_ALIGN_4((unsigned long)
                                   pu4StartAddr)) {
                        priv_driver_get_chip_config_4(
                            pu4StartAddr, u4Length,
                            u4Line, i4TotalLen,
                            i4BytesWritten,
                            pcCommand);
                    } else {
                        DBGLOG(REQ,
                               INFO,
                               "%s: rChipConfigInfo.aucCmd is not 4 bytes alignment %p\n",
                               __func__,
                               rChipConfigInfo.aucCmd);
                    }
                }  /* ChipConfigInfo.ucRespType */
            }
        }
        /* u2MsgSize > 0 */
        DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
               pcCommand);
    }
    /* i4Argc */
    return i4BytesWritten;
}  /* priv_driver_get_chip_config  */

int priv_driver_set_ap_start(IN struct net_device *prNetDev, IN char *pcCommand,
                             IN int i4TotalLen)
{
    PARAM_CUSTOM_P2P_SET_STRUCT_T rSetP2P;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Ret;
    s32 i4ArgNum = 2;

    kalMemZero(&rSetP2P, sizeof(PARAM_CUSTOM_P2P_SET_STRUCT_T));
    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rSetP2P.u4Mode));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "parse ap-start error (u4Enable) u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        if (rSetP2P.u4Mode >= RUNNING_P2P_MODE_NUM) {
            rSetP2P.u4Mode = 0;
            rSetP2P.u4Enable = 0;
        } else {
            rSetP2P.u4Enable = 1;
        }

        if (set_p2p_mode_handler(prNetDev, rSetP2P) != 0) {
            DBGLOG(REQ, ERROR, "set_p2p_mode_handler falied\n");
        }
    }

    return 0;
}

int priv_driver_get_linkspeed(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    u32 u4Rate = 0;
    u32 u4LinkSpeed = 0;
    s32 i4BytesWritten = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    if (!netif_carrier_ok(prNetDev)) {
        return -1;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryLinkSpeed, &u4Rate,
                       sizeof(u4Rate), true, true, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    u4LinkSpeed = u4Rate * 100;
    i4BytesWritten = snprintf(pcCommand, i4TotalLen, "LinkSpeed %u",
                              (unsigned int)u4LinkSpeed);
    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);
    return i4BytesWritten;
}

int priv_driver_set_band(IN struct net_device *prNetDev, IN char *pcCommand,
                         IN int i4TotalLen)
{
    P_ADAPTER_T prAdapter = NULL;
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Argc = 0;
    u32 ucBand = 0;
    u8 ucBssIndex;
    ENUM_BAND_T eBand = BAND_NULL;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 u4Ret = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    prAdapter = prGlueInfo->prAdapter;
    if (i4Argc >= 2) {
        /* ucBand = kalStrtoul(apcArgv[1], NULL, 0); */
        u4Ret = kalkStrtou32(apcArgv[1], 0, &ucBand);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucBand error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        ucBssIndex = wlanGetAisBssIndex(prGlueInfo->prAdapter);
        eBand = BAND_NULL;
        if (ucBand == CMD_BAND_5G) {
            eBand = BAND_5G;
        }else if (ucBand == CMD_BAND_2G) {
            eBand = BAND_2G4;
        }
        prAdapter->aePreferBand[ucBssIndex] = eBand;
        /* XXX call wlanSetPreferBandByNetwork directly in different
         * thread */
        /* wlanSetPreferBandByNetwork (prAdapter, eBand, ucBssIndex); */
    }

    return 0;
}

int priv_driver_set_txpower(IN struct net_device *prNetDev, IN char *pcCommand,
                            IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    P_SET_TXPWR_CTRL_T prTxpwr;
    u16 i;
    s32 u4Ret = 0;
    s32 ai4Setting[4];

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    prTxpwr = &prGlueInfo->rTxPwr;

    if (i4Argc >= 3 && i4Argc <= 5) {
        for (i = 0; i < (i4Argc - 1); i++) {
            /* ai4Setting[i] = kalStrtol(apcArgv[i + 1], NULL, 0);
             */
            u4Ret = kalkStrtos32(apcArgv[i + 1], 0,
                                 &(ai4Setting[i]));
            if (u4Ret) {
                DBGLOG(REQ, LOUD,
                       "parse apcArgv error u4Ret=%d\n", u4Ret);
            }
        }
    } else {
        DBGLOG(REQ, INFO, "set_txpower wrong argc : %d\n", i4Argc);
        return -1;
    }

    /*
     *  ai4Setting[0]
     *  0 : Set TX power offset for specific network
     *  1 : Set TX power offset policy when multiple networks are in the
     * same channel 2 : Set TX power limit for specific channel in 2.4GHz
     * band 3 : Set TX power limit of specific sub-band in 5GHz band 4 :
     * Enable or reset setting
     */
    if (ai4Setting[0] == 0 && (i4Argc - 1) == 4  /* argc num */ ) {
        /* ai4Setting[1] : 0 (All networks), 1 (legacy STA), 2 (Hotspot
         * AP), 3 (P2P), 4 (BT over Wi-Fi) */
        /* ai4Setting[2] : 0 (All bands),1 (2.4G), 2 (5G) */
        /* ai4Setting[3] : -30 ~ 20 in unit of 0.5dBm (default: 0) */
        if (ai4Setting[1] == 1 || ai4Setting[1] == 0) {
            if (ai4Setting[2] == 0 || ai4Setting[2] == 1) {
                prTxpwr->c2GLegacyStaPwrOffset = ai4Setting[3];
            }
            if (ai4Setting[2] == 0 || ai4Setting[2] == 2) {
                prTxpwr->c5GLegacyStaPwrOffset = ai4Setting[3];
            }
        }
        if (ai4Setting[1] == 2 || ai4Setting[1] == 0) {
            if (ai4Setting[2] == 0 || ai4Setting[2] == 1) {
                prTxpwr->c2GHotspotPwrOffset = ai4Setting[3];
            }
            if (ai4Setting[2] == 0 || ai4Setting[2] == 2) {
                prTxpwr->c5GHotspotPwrOffset = ai4Setting[3];
            }
        }
        if (ai4Setting[1] == 3 || ai4Setting[1] == 0) {
            if (ai4Setting[2] == 0 || ai4Setting[2] == 1) {
                prTxpwr->c2GP2pPwrOffset = ai4Setting[3];
            }
            if (ai4Setting[2] == 0 || ai4Setting[2] == 2) {
                prTxpwr->c5GP2pPwrOffset = ai4Setting[3];
            }
        }
        if (ai4Setting[1] == 4 || ai4Setting[1] == 0) {
            if (ai4Setting[2] == 0 || ai4Setting[2] == 1) {
                prTxpwr->c2GBowPwrOffset = ai4Setting[3];
            }
            if (ai4Setting[2] == 0 || ai4Setting[2] == 2) {
                prTxpwr->c5GBowPwrOffset = ai4Setting[3];
            }
        }
    } else if (ai4Setting[0] == 1 && (i4Argc - 1) == 2) {
        /* ai4Setting[1] : 0 (highest power is used) (default), 1
         * (lowest power is used) */
        prTxpwr->ucConcurrencePolicy = ai4Setting[1];
    } else if (ai4Setting[0] == 2 && (i4Argc - 1) == 3) {
        /* ai4Setting[1] : 0 (all channels in 2.4G), 1~14 */
        /* ai4Setting[2] : 10 ~ 46 in unit of 0.5dBm (default: 46) */
        if (ai4Setting[1] == 0) {
            for (i = 0; i < 14; i++)
                prTxpwr->acTxPwrLimit2G[i] = ai4Setting[2];
        } else if (ai4Setting[1] <= 14) {
            prTxpwr->acTxPwrLimit2G[ai4Setting[1] - 1] =
                ai4Setting[2];
        }
    } else if (ai4Setting[0] == 3 && (i4Argc - 1) == 3) {
        /* ai4Setting[1] : 0 (all sub-bands in 5G),
         *  1 (5000 ~ 5250MHz),
         *  2 (5255 ~ 5350MHz),
         *  3 (5355 ~ 5725MHz),
         *  4 (5730 ~ 5825MHz)
         */
        /* ai4Setting[2] : 10 ~ 46 in unit of 0.5dBm (default: 46) */
        if (ai4Setting[1] == 0) {
            for (i = 0; i < 4; i++)
                prTxpwr->acTxPwrLimit5G[i] = ai4Setting[2];
        } else if (ai4Setting[1] <= 4) {
            prTxpwr->acTxPwrLimit5G[ai4Setting[1] - 1] =
                ai4Setting[2];
        }
    } else if (ai4Setting[0] == 4 && (i4Argc - 1) == 2) {
        /* ai4Setting[1] : 1 (enable), 0 (reset and disable) */
        if (ai4Setting[1] == 0) {
            wlanDefTxPowerCfg(prGlueInfo->prAdapter);
        }

        rStatus = kalIoctl(prGlueInfo, wlanoidSetTxPower, prTxpwr,
                           sizeof(SET_TXPWR_CTRL_T), false, false, true,
                           &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }
    } else {
        return -EFAULT;
    }

    return 0;
}
#if CFG_SUPPORT_DBDC_TC6
int priv_driver_set_csa(IN struct net_device *prNetDev, IN char *pcCommand,
                        IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 ch_num = 0;
    s32 u4Ret = 0;
    P_P2P_ROLE_FSM_INFO_T prP2pRoleFsmInfo = (P_P2P_ROLE_FSM_INFO_T)NULL;
    P_BSS_INFO_T prBssInfo;
    u32 i4BytesWritten = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

    prBssInfo = cnmGetp2pSapBssInfo(prGlueInfo->prAdapter);
    if (prBssInfo == NULL) {
        return -1;
    }

    prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
        prGlueInfo->prAdapter, prBssInfo->u4PrivateData);

    if (i4Argc >= 2) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &ch_num);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ch_num error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        u4Ret = cnmIdcCsaReq(prGlueInfo->prAdapter, ch_num,
                             prP2pRoleFsmInfo->ucRoleIndex);
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
               "P2P role:%d send CSA\n", prP2pRoleFsmInfo->ucRoleIndex);
        DBGLOG(REQ, INFO, "u4Ret is %d\n", u4Ret);
    } else {
        DBGLOG(REQ, INFO, "Input insufficent\n");
    }

    return i4BytesWritten;
}
#endif
int priv_driver_set_country(IN struct net_device *prNetDev, IN char *pcCommand,
                            IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u8 aucCountry[2];

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    if (i4Argc < 2) {
        DBGLOG(REQ, WARN, "%s: argc is %d, need >=2\n", __func__,
               i4Argc);
        return -1;
    }
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (regd_is_single_sku_en()) {
        u8 aucCountry_code[4] = { 0, 0, 0, 0 };
        u8 i, count;

        /* command like "COUNTRY US", "COUNTRY US1" and "COUNTRY US01"
         */
        count = kalStrnLen(apcArgv[1], sizeof(aucCountry_code));
        for (i = 0; i < count; i++)
            aucCountry_code[i] = apcArgv[1][i];

        rStatus = kalIoctl(prGlueInfo, wlanoidSetCountryCode,
                           &aucCountry_code[0], count, false, false,
                           true, &u4BufLen);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }

        return 0;
    }

    /* command like "COUNTRY US", "COUNTRY EU" and "COUNTRY JP" */
    aucCountry[0] = apcArgv[1][0];
    aucCountry[1] = apcArgv[1][1];

    rStatus = kalIoctl(prGlueInfo, wlanoidSetCountryCode, &aucCountry[0], 2,
                       false, false, true, &u4BufLen);

    return 0;
}

int priv_driver_get_country(IN struct net_device *prNetDev, IN char *pcCommand,
                            IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 i4BytesWritten = 0;
    u32 country = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (!regd_is_single_sku_en()) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "Not Supported.");
        return i4BytesWritten;
    }

    country = rlmDomainGetCountryCode();

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nCountry Code: \"%s\" (0x%x)", &country, country);

    return i4BytesWritten;
}

int priv_driver_get_channels(IN struct net_device *prNetDev, IN char *pcCommand,
                             IN int i4TotalLen)
{
    u32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
#if (CFG_SUPPORT_SINGLE_SKU == 1)
    u32 ch_idx, start_idx, end_idx;
    struct channel *pCh;
    u32 ch_num = 0;
    u8 maxbw = 160;
    u32 u4Ret = 0;
#endif

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (!regd_is_single_sku_en()) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "Not Supported.");
        return i4BytesWritten;
    }

#if (CFG_SUPPORT_SINGLE_SKU == 1)
    /**
     * Usage: iwpriv wlan0 driver "get_channels [2g |5g |ch_num]"
     **/
    if (i4Argc >= 2 && (apcArgv[1][0] == '2') && (apcArgv[1][1] == 'g')) {
        start_idx = 0;
        end_idx = rlmDomainGetActiveChannelCount(NL80211_BAND_2GHZ);
    } else if (i4Argc >= 2 && (apcArgv[1][0] == '5') &&
               (apcArgv[1][1] == 'g')) {
        start_idx = rlmDomainGetActiveChannelCount(NL80211_BAND_2GHZ);
        end_idx = rlmDomainGetActiveChannelCount(NL80211_BAND_2GHZ) +
                  rlmDomainGetActiveChannelCount(NL80211_BAND_5GHZ);
    } else {
        start_idx = 0;
        end_idx = rlmDomainGetActiveChannelCount(NL80211_BAND_2GHZ) +
                  rlmDomainGetActiveChannelCount(NL80211_BAND_5GHZ);
        if (i4Argc >= 2) {
            /* Dump only specified channel */
            u4Ret = kalkStrtou32(apcArgv[1], 0, &ch_num);
            if (u4Ret) {
                DBGLOG(REQ, LOUD,
                       "parse ch_num error u4Ret=%d\n", u4Ret);
                return -1;
            }
        }
    }

    if (regd_is_single_sku_en()) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\n");

        for (ch_idx = start_idx; ch_idx < end_idx; ch_idx++) {
            pCh = (rlmDomainGetActiveChannels() + ch_idx);

            if (ch_num && (ch_num != pCh->chNum)) {
                continue;  /*show specific channel information*/

            }
            /* Channel number */
            LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                   "CH-%d:", pCh->chNum);
            /* Active/Passive */
            if (pCh->flags & IEEE80211_CHAN_NO_IR) {
                LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                       "NO_IR ");
            } else {
                LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                       " ACTIVE");
            }
            /* Max BW */
            if ((pCh->flags & IEEE80211_CHAN_NO_160MHZ) ==
                IEEE80211_CHAN_NO_160MHZ) {
                maxbw = 80;
            }
            if ((pCh->flags & IEEE80211_CHAN_NO_80MHZ) ==
                IEEE80211_CHAN_NO_80MHZ) {
                maxbw = 40;
            }
            if ((pCh->flags & IEEE80211_CHAN_NO_HT40) ==
                IEEE80211_CHAN_NO_HT40) {
                maxbw = 20;
            }
            LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                   " BW_%dMHz", maxbw);
            /* Channel flags */
            LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                   "  (flags=0x%x)\n", pCh->flags);
        }
    }
#endif

    return i4BytesWritten;
}

int priv_driver_get_ap_channels(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    u32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
#if (CFG_SUPPORT_SINGLE_SKU == 1)
    u32 ch_idx, start_idx, end_idx;
    struct channel *pCh;
    u32 ch_num = 0;
    u8 maxbw = 160;
    u32 u4Ret = 0;
#endif
    P_ADAPTER_T prAdapter = NULL;
    P_WIFI_VAR_T prWifiVar = NULL;
    P_GLUE_INFO_T prGlueInfo = NULL;
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    prAdapter = prGlueInfo->prAdapter;
    prWifiVar = &prAdapter->rWifiVar;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (!regd_is_single_sku_en()) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "Not Supported.");
        return i4BytesWritten;
    }

#if (CFG_SUPPORT_SINGLE_SKU == 1)
    /**
     * Usage: iwpriv wlan0 driver "get_channels [2g |5g |ch_num]"
     **/
    if (i4Argc >= 2 && (apcArgv[1][0] == '2') && (apcArgv[1][1] == 'g')) {
        start_idx = 0;
        end_idx = rlmDomainGetActiveChannelCount(NL80211_BAND_2GHZ);
    } else if (i4Argc >= 2 && (apcArgv[1][0] == '5') &&
               (apcArgv[1][1] == 'g')) {
        start_idx = rlmDomainGetActiveChannelCount(NL80211_BAND_2GHZ);
        end_idx = rlmDomainGetActiveChannelCount(NL80211_BAND_2GHZ) +
                  rlmDomainGetActiveChannelCount(NL80211_BAND_5GHZ);
    } else {
        start_idx = 0;
        end_idx = rlmDomainGetActiveChannelCount(NL80211_BAND_2GHZ) +
                  rlmDomainGetActiveChannelCount(NL80211_BAND_5GHZ);
        if (i4Argc >= 2) {
            /* Dump only specified channel */
            u4Ret = kalkStrtou32(apcArgv[1], 0, &ch_num);
            if (u4Ret) {
                DBGLOG(REQ, LOUD,
                       "parse ucBand error u4Ret=%d\n", u4Ret);
                return -1;
            }
        }
    }

    if (regd_is_single_sku_en()) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\n");

        for (ch_idx = start_idx; ch_idx < end_idx; ch_idx++) {
            u8 ApMaxbw = 0;

            pCh = (rlmDomainGetActiveChannels() + ch_idx);

            if (ch_num && (ch_num != pCh->chNum)) {
                continue;  /*show specific channel information*/

            }
            /* Channel number */
            LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                   "CH-%d:", pCh->chNum);
            /* Active/Passive */
            if (pCh->flags & IEEE80211_CHAN_NO_IR) {
                LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                       " NO_IR");
            } else {
                LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                       " ACTIVE");
            }
            /* Max BW */
            if ((pCh->flags & IEEE80211_CHAN_NO_160MHZ) ==
                IEEE80211_CHAN_NO_160MHZ) {
                maxbw = 80;
            }
            if ((pCh->flags & IEEE80211_CHAN_NO_80MHZ) ==
                IEEE80211_CHAN_NO_80MHZ) {
                maxbw = 40;
            }
            if ((pCh->flags & IEEE80211_CHAN_NO_HT40) ==
                IEEE80211_CHAN_NO_HT40) {
                maxbw = 20;
            }

            // Checking 2G BW setting
            if (pCh->chNum < 13) {
                switch (prWifiVar->ucAp2gBandwidth) {
                case MAX_BW_40MHZ:
                    ApMaxbw = 40;
                    break;

                case MAX_BW_20MHZ:
                    ApMaxbw = 20;
                    break;

                default:
                    ApMaxbw = 40;
                    break;
                }
            } else {
                switch (prWifiVar->ucAp5gBandwidth) {
                case MAX_BW_80MHZ:
                    ApMaxbw = 80;
                    break;

                case MAX_BW_40MHZ:
                    ApMaxbw = 40;
                    break;

                case MAX_BW_20MHZ:
                    ApMaxbw = 20;
                    break;

                default:
                    ApMaxbw = 80;
                    break;
                }
            }
            LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                   " BW_%dMHz",
                   ((maxbw <= ApMaxbw) ? (maxbw) : (ApMaxbw)));
            /* Channel flags */
            LOGBUF(pcCommand,
                   i4TotalLen,
                   i4BytesWritten,
                   "  (flags=0x%x)\n",
                   pCh->flags);
        }
    }
#endif

    return i4BytesWritten;
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
int priv_driver_show_dfs_state(IN struct net_device *prNetDev,
                               IN char *pcCommand, IN int i4TotalLen)
{
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\nDFS State: \"%s\"",
           p2pFuncShowDfsState());

    return i4BytesWritten;
}

int priv_driver_show_dfs_abd123_param(IN struct net_device *prNetDev,
                                      IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;
    u8 ucCnt = 0;
    struct P2P_RADAR_INFO *prP2pabd123Info = NULL;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    prP2pabd123Info = (struct P2P_RADAR_INFO *)cnmMemAlloc(
        prGlueInfo->prAdapter, RAM_TYPE_MSG, sizeof(*prP2pabd123Info));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    p2pFuncGetRadarInfo(prP2pabd123Info);

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\nRDD idx: %d\n",
           prP2pabd123Info->ucRddIdx);
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nLong Pulse detected: %d\n", prP2pabd123Info->ucLongDetected);
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nPeriodic Pulse detected: %d\n",
           prP2pabd123Info->ucPeriodicDetected);
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\nLPB Num: %d\n",
           prP2pabd123Info->ucLPBNum);
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\nPPB Num: %d\n",
           prP2pabd123Info->ucPPBNum);

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n===========================");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nLong Pulse Buffer Contents:\n");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\npulse_time pulse_width PRI\n");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\n%-10d %-11d -\n",
           prP2pabd123Info->arLpbContent[ucCnt].u4LongStartTime,
           prP2pabd123Info->arLpbContent[ucCnt].u2LongPulseWidth);
    for (ucCnt = 1; ucCnt < prP2pabd123Info->ucLPBNum; ucCnt++) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
               "\n%-10d %-11d %d\n",
               prP2pabd123Info->arLpbContent[ucCnt].u4LongStartTime,
               prP2pabd123Info->arLpbContent[ucCnt].u2LongPulseWidth,
               (prP2pabd123Info->arLpbContent[ucCnt].u4LongStartTime -
                prP2pabd123Info->arLpbContent[ucCnt - 1]
                .u4LongStartTime) *
               2 / 5);
    }
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\nLPB Period Valid: %d",
           prP2pabd123Info->ucLPBPeriodValid);
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nLPB Period Valid: %d\n", prP2pabd123Info->ucLPBWidthValid);

    ucCnt = 0;
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n===========================");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nPeriod Pulse Buffer Contents:\n");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\npulse_time pulse_width PRI\n");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\n%-10d %-11d -\n",
           prP2pabd123Info->arPpbContent[ucCnt].u4PeriodicStartTime,
           prP2pabd123Info->arPpbContent[ucCnt].u2PeriodicPulseWidth);
    for (ucCnt = 1; ucCnt < prP2pabd123Info->ucPPBNum; ucCnt++) {
        LOGBUF(pcCommand,
               i4TotalLen,
               i4BytesWritten,
               "\n%-10d %-11d %d\n",
               prP2pabd123Info->arPpbContent[ucCnt].u4PeriodicStartTime,
               prP2pabd123Info->arPpbContent[ucCnt].u2PeriodicPulseWidth,
               (prP2pabd123Info->arPpbContent[ucCnt].u4PeriodicStartTime
                -
                prP2pabd123Info->arPpbContent[ucCnt - 1]
                .u4PeriodicStartTime) *
               2 / 5);
    }
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nPRI Count M1 TH: %d; PRI Count M1: %d",
           prP2pabd123Info->ucPRICountM1TH, prP2pabd123Info->ucPRICountM1);
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nPRI Count M2 TH: %d; PRI Count M2: %d",
           prP2pabd123Info->ucPRICountM2TH, prP2pabd123Info->ucPRICountM2);

    cnmMemFree(prGlueInfo->prAdapter, prP2pabd123Info);

    return i4BytesWritten;
}

int priv_driver_show_dfs_help(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n--iwpriv wlanX driver \"show_dfs_state\"\n");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nINACTIVE: RDD disable or temporary RDD disable");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nCHECKING: During CAC time");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nACTIVE  : In-serive monitoring");
    LOGBUF(pcCommand,
           i4TotalLen,
           i4BytesWritten,
           "\nDETECTED: Has detected abd123 but hasn't moved to new channel\n");

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n--iwpriv wlanX driver \"show_dfs_abd123_param\"\n");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nShow the latest pulse information\n");

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n--iwpriv wlanX driver \"show_dfs_cac_time\"\n");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nShow the remaining time of CAC\n");

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n--iwpriv wlanX set ByPassCac=yy\n");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nValue yy: set the time of CAC\n");

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n--iwpriv wlanX set RDDReport=yy\n");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nValue yy is \"0\" or \"1\"");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n\"0\": Emulate RDD0 manual abd123 event");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n\"1\": Emulate RDD1 manual abd123 event\n");

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n--iwpriv wlanX set abd123DetectMode=yy\n");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nValue yy is \"0\" or \"1\"");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n\"0\": Switch channel when abd123 detected (default)");
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\n\"1\": Do not switch channel when abd123 detected");

    return i4BytesWritten;
}

int priv_driver_show_dfs_cac_time(IN struct net_device *prNetDev,
                                  IN char *pcCommand, IN int i4TotalLen)
{
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (p2pFuncGetDfsState() != DFS_STATE_CHECKING) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
               "\nNot in CAC period");
        return i4BytesWritten;
    }

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nRemaining time of CAC: %dsec", p2pFuncGetCacRemainingTime());

    return i4BytesWritten;
}

int priv_driver_set_rdd_report(IN struct net_device *prNetDev,
                               IN char *pcCommand, IN int i4TotalLen)
{
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    // u32 u4Ret;
    s32 i4ArgNum = 2;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        SetRddReport(prNetDev, apcArgv[1]);
    }

    return 0;
}

int priv_driver_clean_dfs_abd123_param(IN struct net_device *prNetDev,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4BytesWritten = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    p2pFuncRadarInfoInit();

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nClean up abd123 param\n");
    return i4BytesWritten;
}

int priv_driver_set_abd123_detect_mode(IN struct net_device *prNetDev,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Ret;
    s32 i4ArgNum = 2;
    u8 ucabd123DetectMode;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        u4Ret = kalkStrtou8(apcArgv[1], 0, &ucabd123DetectMode);
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "parse abd123 detect mode error - u4Ret=%d\n",
                   u4Ret);
            return -1;
        }
        if (p2pFuncGetDfsState() == DFS_STATE_INACTIVE ||
            p2pFuncGetDfsState() == DFS_STATE_DETECTED) {
            DBGLOG(REQ,
                   ERROR,
                   "RDD Report is not supported in this DFS state (inactive or deteted)\n");
            return -1;
        }

        if (ucabd123DetectMode != 0 && ucabd123DetectMode != 1) {
            DBGLOG(REQ,
                   ERROR,
                   "abd123 Detect Mode is not \"0\" or \"1\", Invalid data\n");
            return -1;
        }

        p2pFuncSetRadarDetectMode(ucabd123DetectMode);

        if (MT_ATESetRadarDetectMode(prNetDev, ucabd123DetectMode) != 0) {
            return -1;
        }
    }

    return 0;
}

#endif
#ifdef CFG_SUPPORT_ADJUST_MCC_STAY_TIME
int priv_driver_set_mcc_time(IN struct net_device *prNetDev, IN char *pcCommand,
                             IN int i4TotalLen)
{
    P_ADAPTER_T prAdapter = NULL;
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Argc = 0;
    u32 i4BytesWritten = 0;
    u32 ucLinkType = 0;  /* 0 for AIS and 1 for P2P */
    u32 ucStayTime = 0;  /* In unit of us */
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 i4Ret = 0;

    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    prAdapter = prGlueInfo->prAdapter;
    if (i4Argc != 3) {
        goto out;
    }

    i4Ret = kalkStrtou32(apcArgv[1], 0, &ucLinkType);
    if (i4Ret) {
        DBGLOG(REQ, LOUD, "parse ucLinkType error i4Ret=%d\n", i4Ret);
        goto out;
    }
    i4Ret = kalkStrtou32(apcArgv[2], 0, &ucStayTime);
    if (i4Ret) {
        DBGLOG(REQ, LOUD, "parse ucStayTime error i4Ret=%d\n", i4Ret);
        goto out;
    }
    if (ucLinkType == NETWORK_TYPE_AIS) {
        snprintf(pcCommand, i4TotalLen, CMD_SET_CHIP " mccTime 0 %d",
                 ucStayTime);
    } else if (ucLinkType == NETWORK_TYPE_P2P) {
        snprintf(pcCommand, i4TotalLen, CMD_SET_CHIP " mccTime 1 %d",
                 ucStayTime);
    } else {
        DBGLOG(REQ, LOUD, "Wrong network type %i\n", ucLinkType);
        i4BytesWritten = -1;
        goto out;
    }
    priv_driver_set_chip_config(prNetDev, pcCommand, i4TotalLen);
out:
    return i4BytesWritten;
}
#endif
int priv_driver_set_miracast(IN struct net_device *prNetDev, IN char *pcCommand,
                             IN int i4TotalLen)
{
    P_ADAPTER_T prAdapter = NULL;
    P_GLUE_INFO_T prGlueInfo = NULL;
    u32 i4BytesWritten = 0;
    /* WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS; */
    /* u32 u4BufLen = 0; */
    s32 i4Argc = 0;
    u32 ucMode = 0;
    P_WFD_CFG_SETTINGS_T prWfdCfgSettings = (P_WFD_CFG_SETTINGS_T)NULL;
    P_MSG_WFD_CONFIG_SETTINGS_CHANGED_T prMsgWfdCfgUpdate =
        (P_MSG_WFD_CONFIG_SETTINGS_CHANGED_T)NULL;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    s32 u4Ret = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    prAdapter = prGlueInfo->prAdapter;
    if (i4Argc >= 2) {
        /* ucMode = kalStrtoul(apcArgv[1], NULL, 0); */
        u4Ret = kalkStrtou32(apcArgv[1], 0, &ucMode);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucMode error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        if (g_ucMiracastMode == (u8)ucMode) {
            /* XXX: continue or skip */
            /* XXX: continue or skip */
        }

        g_ucMiracastMode = (u8)ucMode;
        prMsgWfdCfgUpdate =
            cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                        sizeof(MSG_WFD_CONFIG_SETTINGS_CHANGED_T));

        if (prMsgWfdCfgUpdate != NULL) {
            prWfdCfgSettings =
                &(prAdapter->rWifiVar.rWfdConfigureSettings);
            prMsgWfdCfgUpdate->rMsgHdr.eMsgId =
                MID_MNY_P2P_WFD_CFG_UPDATE;
            prMsgWfdCfgUpdate->prWfdCfgSettings = prWfdCfgSettings;

            if (ucMode == MIRACAST_MODE_OFF) {
                prWfdCfgSettings->ucWfdEnable = 0;
                snprintf(pcCommand, i4TotalLen,
                         CMD_SET_CHIP " mira 0");
            } else if (ucMode == MIRACAST_MODE_SOURCE) {
                prWfdCfgSettings->ucWfdEnable = 1;
                snprintf(pcCommand, i4TotalLen,
                         CMD_SET_CHIP " mira 1");
            } else if (ucMode == MIRACAST_MODE_SINK) {
                prWfdCfgSettings->ucWfdEnable = 2;
                snprintf(pcCommand, i4TotalLen,
                         CMD_SET_CHIP " mira 2");
            } else {
                prWfdCfgSettings->ucWfdEnable = 0;
                snprintf(pcCommand, i4TotalLen,
                         CMD_SET_CHIP " mira 0");
            }

            mboxSendMsg(prAdapter, MBOX_ID_0,
                        (P_MSG_HDR_T)prMsgWfdCfgUpdate,
                        MSG_SEND_METHOD_BUF);

            priv_driver_set_chip_config(prNetDev, pcCommand,
                                        i4TotalLen);
        } else {
            ASSERT(false);
            i4BytesWritten = -1;
        }
    }

    /* i4Argc */
    return i4BytesWritten;
}

#if CFG_WOW_SUPPORT
static int priv_driver_set_wow(IN struct net_device *prNetDev,
                               IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_WOW_CTRL_T pWOW_CTRL = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Ret = 0;
    u32 Enable = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    if (i4Argc < 2) {
        DBGLOG(REQ, WARN, "%s: argc is %d, need >=2\n", __func__,
               i4Argc);
        return -1;
    }
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    u4Ret = kalkStrtou32(apcArgv[1], 0, &Enable);

    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse bEnable error u4Ret=%d\n", u4Ret);
        return -1;
    }

    DBGLOG(INIT, INFO, "CMD set_wow_enable = %d\n", Enable);
    DBGLOG(INIT, INFO, "Scenario ID %d\n", pWOW_CTRL->ucScenarioId);
    DBGLOG(INIT, INFO, "ucBlockCount %d\n", pWOW_CTRL->ucBlockCount);
    DBGLOG(INIT, INFO, "interface %d\n",
           pWOW_CTRL->astWakeHif[0].ucWakeupHif);
    DBGLOG(INIT, INFO, "gpio_pin %d\n", pWOW_CTRL->astWakeHif[0].ucGpioPin);
    DBGLOG(INIT, INFO, "gpio_level 0x%x\n",
           pWOW_CTRL->astWakeHif[0].ucTriggerLvl);
    DBGLOG(INIT, INFO, "gpio_timer %d\n",
           pWOW_CTRL->astWakeHif[0].u4GpioInterval);
    kalWowProcess(prGlueInfo, Enable);

    return 0;
}

static int priv_driver_set_wow_enable(IN struct net_device *prNetDev,
                                      IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_WOW_CTRL_T pWOW_CTRL = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Ret = 0;
    u8 ucEnable = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    if (i4Argc < 2) {
        DBGLOG(REQ, WARN, "%s: argc is %d, need >=2\n", __func__,
               i4Argc);
        return -1;
    }
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 2) {
        u4Ret = kalkStrtou8(apcArgv[1], 0, &ucEnable);

        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse bEnable error u4Ret=%d\n",
                   u4Ret);
        }

        pWOW_CTRL->fgWowEnable = ucEnable;

        DBGLOG(PF, INFO, "WOW enable %d\n", pWOW_CTRL->fgWowEnable);

        return 0;
    } else {
        return -1;
    }
}

static int priv_driver_set_wow_par(IN struct net_device *prNetDev,
                                   IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_WOW_CTRL_T pWOW_CTRL = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0;
    u8 ucWakeupHif = 0, GpioPin = 0, ucGpioLevel = 0, ucBlockCount,
       ucScenario = 0;
    u32 u4GpioTimer = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 7) {
        u4Ret = kalkStrtou8(apcArgv[1], 0, &ucWakeupHif);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
                   u4Ret);
        }
        pWOW_CTRL->astWakeHif[0].ucWakeupHif = ucWakeupHif;

        u4Ret = kalkStrtou8(apcArgv[2], 0, &GpioPin);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse GpioPin error u4Ret=%d\n",
                   u4Ret);
        }
        pWOW_CTRL->astWakeHif[0].ucGpioPin = GpioPin;

        u4Ret = kalkStrtou8(apcArgv[3], 0, &ucGpioLevel);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse Gpio level error u4Ret=%d\n",
                   u4Ret);
        }
        pWOW_CTRL->astWakeHif[0].ucTriggerLvl = ucGpioLevel;

        u4Ret = kalkStrtou32(apcArgv[4], 0, &u4GpioTimer);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse u4GpioTimer error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        pWOW_CTRL->astWakeHif[0].u4GpioInterval = u4GpioTimer;

        u4Ret = kalkStrtou8(apcArgv[5], 0, &ucScenario);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucScenario error u4Ret=%d\n",
                   u4Ret);
        }
        pWOW_CTRL->ucScenarioId = ucScenario;

        u4Ret = kalkStrtou8(apcArgv[6], 0, &ucBlockCount);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucBlockCnt error u4Ret=%d\n",
                   u4Ret);
        }
        pWOW_CTRL->ucBlockCount = ucBlockCount;

        DBGLOG(INIT, INFO, "gpio_scenario%d\n",
               pWOW_CTRL->ucScenarioId);
        DBGLOG(INIT, INFO, "interface %d\n",
               pWOW_CTRL->astWakeHif[0].ucWakeupHif);
        DBGLOG(INIT, INFO, "gpio_pin %d\n",
               pWOW_CTRL->astWakeHif[0].ucGpioPin);
        DBGLOG(INIT, INFO, "gpio_level %d\n",
               pWOW_CTRL->astWakeHif[0].ucTriggerLvl);
        DBGLOG(INIT, INFO, "gpio_timer %d\n",
               pWOW_CTRL->astWakeHif[0].u4GpioInterval);

        return 0;
    } else {
        return -1;
    }
}

static int priv_driver_set_wow_udpport(IN struct net_device *prNetDev,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_WOW_CTRL_T pWOW_CTRL = NULL;
    s32 i4Argc = 0;
    s8 *apcPortArgv[WLAN_CFG_ARGV_MAX_LONG] = { 0 };  /* to input 20 port */
    s32 u4Ret = 0, ii;
    u8 ucVer, ucCount;
    u16 u2Port = 0;
    u16 *pausPortArry;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgumentLong(pcCommand, &i4Argc, apcPortArgv);
    DBGLOG(REQ, WARN, "argc is %i\n", i4Argc);

    /* example: set_wow_udp 0 5353,8080 (set) */
    /* example: set_wow_udp 1 (clear) */

    if (i4Argc >= 3) {
        /* Pick Max */
        ucCount = ((i4Argc - 2) > MAX_TCP_UDP_PORT) ? MAX_TCP_UDP_PORT :
                  (i4Argc - 2);
        DBGLOG(PF, INFO, "UDP ucCount=%d\n", ucCount);

        u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        /* IPv4/IPv6 */
        DBGLOG(PF, INFO, "ucVer=%d\n", ucVer);
        if (ucVer == 0) {
            pWOW_CTRL->stWowPort.ucIPv4UdpPortCnt = ucCount;
            pausPortArry = pWOW_CTRL->stWowPort.ausIPv4UdpPort;
        } else {
            pWOW_CTRL->stWowPort.ucIPv6UdpPortCnt = ucCount;
            pausPortArry = pWOW_CTRL->stWowPort.ausIPv6UdpPort;
        }

        /* Port */
        for (ii = 0; ii < ucCount; ii++) {
            u4Ret = kalkStrtou16(apcPortArgv[ii + 2], 0, &u2Port);
            if (u4Ret) {
                DBGLOG(PF, ERROR,
                       "parse u2Port error u4Ret=%d\n", u4Ret);
                return -1;
            }

            pausPortArry[ii] = u2Port;
            DBGLOG(PF, INFO, "ucPort=%d, idx=%d\n", u2Port, ii);
        }

        return 0;
    } else if (i4Argc == 2) {
        u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        if (ucVer == 0) {
            kalMemZero(prGlueInfo->prAdapter->rWowCtrl.stWowPort
                       .ausIPv4UdpPort,
                       sizeof(u16) * MAX_TCP_UDP_PORT);
            prGlueInfo->prAdapter->rWowCtrl.stWowPort
            .ucIPv4UdpPortCnt = 0;
        } else {
            kalMemZero(prGlueInfo->prAdapter->rWowCtrl.stWowPort
                       .ausIPv6UdpPort,
                       sizeof(u16) * MAX_TCP_UDP_PORT);
            prGlueInfo->prAdapter->rWowCtrl.stWowPort
            .ucIPv6UdpPortCnt = 0;
        }

        return 0;
    } else {
        return -1;
    }
}

static int priv_driver_set_wow_tcpport(IN struct net_device *prNetDev,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_WOW_CTRL_T pWOW_CTRL = NULL;
    s32 i4Argc = 0;
    s8 *apcPortArgv[WLAN_CFG_ARGV_MAX_LONG] = { 0 };  /* to input 20 port */
    s32 u4Ret = 0, ii;
    u8 ucVer, ucCount;
    u16 u2Port = 0;
    u16 *pausPortArry;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgumentLong(pcCommand, &i4Argc, apcPortArgv);
    DBGLOG(REQ, WARN, "argc is %i\n", i4Argc);

    /* example: set_wow_tcp 0 5353,8080 (Set) */
    /* example: set_wow_tcp 1 (clear) */

    if (i4Argc >= 3) {
        /* Pick Max */
        ucCount = ((i4Argc - 2) > MAX_TCP_UDP_PORT) ? MAX_TCP_UDP_PORT :
                  (i4Argc - 2);
        DBGLOG(PF, INFO, "TCP ucCount=%d\n", ucCount);

        u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        /* IPv4/IPv6 */
        DBGLOG(PF, INFO, "Ver=%d\n", ucVer);
        if (ucVer == 0) {
            pWOW_CTRL->stWowPort.ucIPv4TcpPortCnt = ucCount;
            pausPortArry = pWOW_CTRL->stWowPort.ausIPv4TcpPort;
        } else {
            pWOW_CTRL->stWowPort.ucIPv6TcpPortCnt = ucCount;
            pausPortArry = pWOW_CTRL->stWowPort.ausIPv6TcpPort;
        }

        /* Port */
        for (ii = 0; ii < ucCount; ii++) {
            u4Ret = kalkStrtou16(apcPortArgv[ii + 2], 0, &u2Port);
            if (u4Ret) {
                DBGLOG(PF, ERROR,
                       "parse u2Port error u4Ret=%d\n", u4Ret);
                return -1;
            }

            pausPortArry[ii] = u2Port;
            DBGLOG(PF, INFO, "ucPort=%d, idx=%d\n", u2Port, ii);
        }

        return 0;
    } else if (i4Argc == 2) {
        u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        if (ucVer == 0) {
            kalMemZero(prGlueInfo->prAdapter->rWowCtrl.stWowPort
                       .ausIPv4UdpPort,
                       sizeof(u16) * MAX_TCP_UDP_PORT);
            prGlueInfo->prAdapter->rWowCtrl.stWowPort
            .ucIPv4UdpPortCnt = 0;
        } else {
            kalMemZero(prGlueInfo->prAdapter->rWowCtrl.stWowPort
                       .ausIPv6UdpPort,
                       sizeof(u16) * MAX_TCP_UDP_PORT);
            prGlueInfo->prAdapter->rWowCtrl.stWowPort
            .ucIPv6UdpPortCnt = 0;
        }

        return 0;
    } else {
        return -1;
    }
}

static int priv_driver_get_wow_port(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_WOW_CTRL_T pWOW_CTRL = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0, ii;
    u8 ucVer, ucProto;
    u16 ucCount;
    u16 *pausPortArry;
#if !DBG_DISABLE_ALL_LOG
    s8 *aucIp[2] = { "IPv4", "IPv6" };
    s8 *aucProto[2] = { "UDP", "TCP" };
#endif

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    /* example: get_wow_port 0 0 (ipv4-udp) */
    /* example: get_wow_port 0 1 (ipv4-tcp) */
    /* example: get_wow_port 1 0 (ipv6-udp) */
    /* example: get_wow_port 1 1 (ipv6-tcp) */

    if (i4Argc >= 3) {
        /* 0=IPv4, 1=IPv6 */
        u4Ret = kalkStrtou8(apcArgv[1], 0, &ucVer);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse argc[1] error u4Ret=%d\n",
                   u4Ret);
        }

        /* 0=UDP, 1=TCP */
        u4Ret = kalkStrtou8(apcArgv[2], 0, &ucProto);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse argc[2] error u4Ret=%d\n",
                   u4Ret);
        }

        if (ucVer > 1) {
            ucVer = 0;
        }

        if (ucProto > 1) {
            ucProto = 0;
        }

        if (ucVer == 0) {
            if (ucProto == 0) {
                /* IPv4/UDP */
                ucCount = pWOW_CTRL->stWowPort.ucIPv4UdpPortCnt;
                pausPortArry =
                    pWOW_CTRL->stWowPort.ausIPv4UdpPort;
            } else {
                /* IPv4/TCP */
                ucCount = pWOW_CTRL->stWowPort.ucIPv4TcpPortCnt;
                pausPortArry =
                    pWOW_CTRL->stWowPort.ausIPv4TcpPort;
            }
        } else {
            if (ucProto == 0) {
                /* IPv6/UDP */
                ucCount = pWOW_CTRL->stWowPort.ucIPv6UdpPortCnt;
                pausPortArry =
                    pWOW_CTRL->stWowPort.ausIPv6UdpPort;
            } else {
                /* IPv6/TCP */
                ucCount = pWOW_CTRL->stWowPort.ucIPv6TcpPortCnt;
                pausPortArry =
                    pWOW_CTRL->stWowPort.ausIPv6TcpPort;
            }
        }

        /* Dunp Port */
        for (ii = 0; ii < ucCount; ii++)
            DBGLOG(PF, INFO, "ucPort=%d, idx=%d\n",
                   pausPortArry[ii], ii);

        DBGLOG(PF, INFO, "[%s/%s] count:%d\n", aucIp[ucVer],
               aucProto[ucProto], ucCount);

        return 0;
    } else {
        return -1;
    }
}

static int priv_driver_get_wow_reason(IN struct net_device *prNetDev,
                                      IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Argc = 0;
    s32 i4BytesWritten = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    P_WOW_CTRL_T pWOW_CTRL = NULL;

    ASSERT(prNetDev);
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (pWOW_CTRL->ucReason != INVALID_WOW_WAKE_UP_REASON) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
               "\nwakeup_reason:%d", pWOW_CTRL->ucReason);
    }

    return i4BytesWritten;
}
#endif

#if CFG_STR_DHCP_RENEW_OFFLOAD
static int priv_driver_set_dhcp_info(IN struct net_device *prNetDev,
                                     IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_BSS_INFO_T prBssInfo;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u8 aucIpAddr[4];
    u8 i = 0;
    u8 fgIsIpInvalid = false;
    u32 u4Ret = 0;
    u32 u4RenewIntv = 0, u4Value = 0;
    u8 ucLength = 0, ucNum = 0;
    s8 *pcTmp, *pcStart;
    s8 CurrChar;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    DBGLOG(REQ, LOUD, "MT7668 : priv_driver_set_dhcp_info\n");

    if (i4Argc != 3) {
        DBGLOG(REQ, ERROR, "argc %i is not equal to 3\n", i4Argc);
        goto out;
    }

    pcStart = apcArgv[1];
    pcTmp = pcStart;
    ucLength = strlen(apcArgv[1]);

    while (i < 4) {
        u4Value = 0;

        while (1) {
            CurrChar = *pcTmp;

            pcTmp++;

            if (pcTmp > (pcStart + ucLength)) {
                if (ucNum != 3 && !isdigit(CurrChar)) {
                    DBGLOG(REQ, WARN, "Invalid: Num=%d\n",
                           ucNum + 1);
                    fgIsIpInvalid = true;
                }
                break;
            } else if (CurrChar >= '0' && CurrChar <= '9') {
                u4Value *= 10;
                u4Value += CurrChar - '0';
            } else if (i < 3 && CurrChar == '.') {
                ucNum++;
                break;
            } else {
                if (i != 3 || !isdigit(CurrChar)) {
                    DBGLOG(REQ, WARN, "Invalid\n");
                    fgIsIpInvalid = true;
                }
                break;
            }
        }

        if (u4Value >= 256) {
            DBGLOG(REQ, WARN, "Number is larger than 255\n");
            fgIsIpInvalid = true;
            break;
        }

        aucIpAddr[i] = (u8)u4Value;
        i++;
    }

    if (fgIsIpInvalid) {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "Dhcp Server IP is invalid");
        return i4BytesWritten;
    }

    u4Ret = kalkStrtou32(apcArgv[2], 0, &u4RenewIntv);
    if (u4Ret) {
        DBGLOG(REQ, WARN, "parse u4LogType error u4Ret=%d\n", u4Ret);
        goto out;
    }

    /* TODO: Only support AIS for now */
    prBssInfo = prGlueInfo->prAdapter->prAisBssInfo;

    if ((prBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) &&
        (prBssInfo->fgIsNetActive)) {
        if (u4RenewIntv != 0) {
            prBssInfo->fgIsDhcpAcked = true;
            prBssInfo->u4DhcpRenewIntv = u4RenewIntv;

            for (i = 0; i < 4; i++)
                prBssInfo->aucDhcpServerIpAddr[i] =
                    aucIpAddr[i];

            DBGLOG(REQ,
                   EVENT,
                   "Store DHCP Renew info: ServerIP= %d.%d.%d.%d ,RenewIntv= %d seconds\n",
                   prBssInfo->aucDhcpServerIpAddr[0],
                   prBssInfo->aucDhcpServerIpAddr[1],
                   prBssInfo->aucDhcpServerIpAddr[2],
                   prBssInfo->aucDhcpServerIpAddr[3],
                   prBssInfo->u4DhcpRenewIntv);
        } else {
            /* disable dhcp offload if lease time is set to 0 */
            prBssInfo->fgIsDhcpAcked = false;
            prBssInfo->u4DhcpRenewIntv = 0;
            kalMemZero(prBssInfo->aucDhcpServerIpAddr,
                       sizeof(prBssInfo->aucDhcpServerIpAddr));
            DBGLOG(REQ, EVENT, "Disable Dhcp Offload during STR\n");
        }
    } else {
        DBGLOG(REQ, ERROR, "Cannot set_dhcp when disconnected\n");
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "set_dhcp failed due to wifi is disconnected");
        return i4BytesWritten;
    }

    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten,
                               "set_dhcp success");
    return i4BytesWritten;

out:
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "format:set_dhcp [Dhcp Server IP] [Renew interval]");
    return i4BytesWritten;
}
#endif

static int priv_driver_set_adv_pws(IN struct net_device *prNetDev,
                                   IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Ret = 0;
    u8 ucAdvPws = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);
    if (i4Argc < 2) {
        DBGLOG(REQ, WARN, "%s: argc is %d, need >=2\n", __func__,
               i4Argc);
        return -1;
    }
    if (i4Argc >= 2) {
        u4Ret = kalkStrtou8(apcArgv[1], 0, &ucAdvPws);

        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse bEnable error u4Ret=%d\n",
                   u4Ret);
        }

        prGlueInfo->prAdapter->rWifiVar.ucAdvPws = ucAdvPws;

        DBGLOG(INIT, INFO, "AdvPws:%d\n",
               &prGlueInfo->prAdapter->rWifiVar.ucAdvPws);

        return 0;
    } else {
        return -1;
    }
}

static int priv_driver_set_mdtim(IN struct net_device *prNetDev,
                                 IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Ret = 0;
    u8 ucMultiDtim = 0, ucVer;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    /* iwpriv wlan0 driver "set_mdtim 1 3 */
    if (i4Argc >= 3) {
        u4Ret = kalkStrtou8(apcArgv[1], 0, &ucVer);
        if (u4Ret) {
            DBGLOG(REQ, ERROR, "parse apcArgv1 error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        u4Ret = kalkStrtou8(apcArgv[2], 0, &ucMultiDtim);
        if (u4Ret) {
            DBGLOG(REQ, ERROR, "parse apcArgv2 error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        if (ucVer == 0) {
            prGlueInfo->prAdapter->rWifiVar.ucWowOnMdtim =
                ucMultiDtim;
            DBGLOG(REQ, INFO, "WOW On MDTIM:%d\n",
                   &prGlueInfo->prAdapter->rWifiVar.ucWowOnMdtim);
        } else {
            prGlueInfo->prAdapter->rWifiVar.ucWowOffMdtim =
                ucMultiDtim;
            DBGLOG(REQ, INFO, "WOW Off MDTIM:%d\n",
                   &prGlueInfo->prAdapter->rWifiVar.ucWowOffMdtim);
        }
    }

    return 0;
}

static int priv_driver_set_listen_dtim_interval(IN struct net_device *prNetDev,
                                                IN char *pcCommand,
                                                IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Ret = 0;
    u8 ucInterval = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    /* iwpriv wlan0 driver "set_listen_dtim_interval x */
    if (i4Argc >= 2) {
        u4Ret = kalkStrtou8(apcArgv[1], 0, &ucInterval);
        if (u4Ret) {
            DBGLOG(REQ, ERROR, "parse apcArgv1 error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        prGlueInfo->prAdapter->rWifiVar.ucListenDtimInterval =
            ucInterval;
        DBGLOG(REQ, INFO, "Listen Interval(DTIM) :%d\n",
               &prGlueInfo->prAdapter->rWifiVar.ucListenDtimInterval);
    }

    return 0;
}

int priv_driver_set_suspend_mode(IN struct net_device *prNetDev,
                                 IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u8 fgEnable;
    u32 u4Enable = false;
    s32 u4Ret = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 2) {
        /* fgEnable = (kalStrtoul(apcArgv[1], NULL, 0) == 1) ? true :
         * false; */
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Enable);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse u4Enable error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        if (u4Enable == 1) {
            fgEnable = true;
        }else{
            fgEnable = false;
        }

        DBGLOG(REQ, INFO, "%s: Set suspend mode [%u]\n", __func__,
               fgEnable);

        if (prGlueInfo->fgIsInSuspendMode == fgEnable) {
            DBGLOG(REQ, INFO,
                   "%s: Already in suspend mode, SKIP!\n",
                   __func__);
            return 0;
        }

        prGlueInfo->fgIsInSuspendMode = fgEnable;

        wlanSetSuspendMode(prGlueInfo, fgEnable);
        p2pSetSuspendMode(prGlueInfo, fgEnable);
    }

    return 0;
}

#if CFG_SUPPORT_SNIFFER
int priv_driver_set_monitor(IN struct net_device *prNetDev, IN char *pcCommand,
                            IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4Argc = 0;
    s32 i4BytesWritten = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    PARAM_CUSTOM_MONITOR_SET_STRUCT_T rMonitorSetInfo;
    u8 ucEnable = 0;
    u8 ucPriChannel = 0;
    u8 ucChannelWidth = 0;
    u8 ucExt = 0;
    u8 ucSco = 0;
    u8 ucChannelS1 = 0;
    u8 ucChannelS2 = 0;
    u8 fgIsLegalChannel = false;
    u8 fgError = false;
    u8 fgEnable = false;
    ENUM_BAND_T eBand = BAND_NULL;
    u32 u4Parse = 0;
    s32 u4Ret = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    if (i4Argc >= 5) {
        /* ucEnable = (u8) (kalStrtoul(apcArgv[1], NULL, 0));
         *  ucPriChannel = (u8) (kalStrtoul(apcArgv[2], NULL, 0));
         *  ucChannelWidth = (u8) (kalStrtoul(apcArgv[3], NULL, 0));
         *  ucExt = (u8) (kalStrtoul(apcArgv[4], NULL, 0));
         */
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        ucEnable = (u8)u4Parse;
        u4Ret = kalkStrtou32(apcArgv[2], 0, &u4Parse);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        ucPriChannel = (u8)u4Parse;
        u4Ret = kalkStrtou32(apcArgv[3], 0, &u4Parse);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        ucChannelWidth = (u8)u4Parse;
        u4Ret = kalkStrtou32(apcArgv[4], 0, &u4Parse);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        ucExt = (u8)u4Parse;

        eBand = (ucPriChannel <= 14) ? BAND_2G4 : BAND_5G;
        fgIsLegalChannel =
            rlmDomainIsLegalChannel(prAdapter, eBand, ucPriChannel);

        if (fgIsLegalChannel == false) {
            i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                                      "Illegal primary channel %d",
                                      ucPriChannel);
            return i4BytesWritten;
        }

        switch (ucChannelWidth) {
        case 160:
            ucChannelWidth = (u8)CW_160MHZ;
            ucSco = (u8)CHNL_EXT_SCN;

            if (ucPriChannel >= 36 && ucPriChannel <= 64) {
                ucChannelS2 = 50;
            }else if (ucPriChannel >= 100 && ucPriChannel <= 128) {
                ucChannelS2 = 114;
            }else{
                fgError = true;
            }
            break;

        case 80:
            ucChannelWidth = (u8)CW_80MHZ;
            ucSco = (u8)CHNL_EXT_SCN;

            if (ucPriChannel >= 36 && ucPriChannel <= 48) {
                ucChannelS1 = 42;
            }else if (ucPriChannel >= 52 && ucPriChannel <= 64) {
                ucChannelS1 = 58;
            }else if (ucPriChannel >= 100 && ucPriChannel <= 112) {
                ucChannelS1 = 106;
            }else if (ucPriChannel >= 116 && ucPriChannel <= 128) {
                ucChannelS1 = 122;
            }else if (ucPriChannel >= 132 && ucPriChannel <= 144) {
                ucChannelS1 = 138;
            }else if (ucPriChannel >= 149 && ucPriChannel <= 161) {
                ucChannelS1 = 155;
            }else{
                fgError = true;
            }
            break;

        case 40:
            ucChannelWidth = (u8)CW_20_40MHZ;
            ucSco = (ucExt) ? (u8)CHNL_EXT_SCA : (u8)CHNL_EXT_SCB;
            break;

        case 20:
            ucChannelWidth = (u8)CW_20_40MHZ;
            ucSco = (u8)CHNL_EXT_SCN;
            break;

        default:
            fgError = true;
            break;
        }

        if (fgError) {
            i4BytesWritten = snprintf(
                pcCommand, i4TotalLen,
                "Invalid primary channel %d with bandwidth %d",
                ucPriChannel, ucChannelWidth);
            return i4BytesWritten;
        }

        fgEnable = (ucEnable) ? true : false;

        if (prGlueInfo->fgIsEnableMon != fgEnable) {
            prGlueInfo->fgIsEnableMon = fgEnable;
            schedule_work(&prGlueInfo->monWork);
        }

        kalMemZero(&rMonitorSetInfo, sizeof(rMonitorSetInfo));

        rMonitorSetInfo.ucEnable = ucEnable;
        rMonitorSetInfo.ucPriChannel = ucPriChannel;
        rMonitorSetInfo.ucSco = ucSco;
        rMonitorSetInfo.ucChannelWidth = ucChannelWidth;
        rMonitorSetInfo.ucChannelS1 = ucChannelS1;
        rMonitorSetInfo.ucChannelS2 = ucChannelS2;

        rStatus = kalIoctl(prGlueInfo, wlanoidSetMonitor,
                           &rMonitorSetInfo, sizeof(rMonitorSetInfo),
                           false, false, true, &u4BufLen);

        i4BytesWritten = snprintf(
            pcCommand, i4TotalLen, "set monitor config %s",
            (rStatus == WLAN_STATUS_SUCCESS) ? "success" : "fail");

        return i4BytesWritten;
    }

    i4BytesWritten =
        snprintf(pcCommand, i4TotalLen,
                 "monitor [Enable][PriChannel][ChannelWidth][Sco]");

    return i4BytesWritten;
}
#endif

static int priv_driver_get_version(IN struct net_device *prNetDev,
                                   IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter;
    s32 i4BytesWritten = 0;
    u32 u4Offset = 0;
    P_WIFI_VER_INFO_T prVerInfo;
    tailer_format_t *prTailer;
    u8 aucBuf[32], aucDate[32];

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;
    prVerInfo = &prAdapter->rVerInfo;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "\nChip ROM ver [%u]\n",
                         prAdapter->chip_info->eco_ver);

    wlanPrintVersion(prAdapter);

    kalStrnCpy(aucBuf, prVerInfo->aucFwBranchInfo, 4);
    aucBuf[4] = '\0';
    kalStrnCpy(aucDate, prVerInfo->aucFwDateCode, 16);
    aucDate[16] = '\0';
    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "\nN9 FW version %s-%u.%u.%u[DEC] (%s)\n", aucBuf,
                         (prVerInfo->u2FwOwnVersion >> 8),
                         (prVerInfo->u2FwOwnVersion & BITS(0, 7)),
                         prVerInfo->ucFwBuildNumber, aucDate);
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
    if (prVerInfo->fgIsN9CompressedFW) {
        tailer_format_t_2 *prTailer;

        prTailer = &prVerInfo->rN9Compressedtailer;
        kalMemCopy(aucBuf, prTailer->ram_version, 10);
        aucBuf[10] = '\0';
        u4Offset +=
            snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                     "N9  tailer version %s (%s) info %u:E%u\n",
                     aucBuf, prTailer->ram_built_date,
                     prTailer->chip_info, prTailer->eco_code + 1);
    } else {
        prTailer = &prVerInfo->rN9tailer;
        kalMemCopy(aucBuf, prTailer->ram_version, 10);
        aucBuf[10] = '\0';
        u4Offset +=
            snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                     "N9  tailer version %s (%s) info %u:E%u\n",
                     aucBuf, prTailer->ram_built_date,
                     prTailer->chip_info, prTailer->eco_code + 1);
    }
    if (prVerInfo->fgIsCR4CompressedFW) {
        tailer_format_t_2 *prTailer;

        prTailer = &prVerInfo->rCR4Compressedtailer;
        kalMemCopy(aucBuf, prTailer->ram_version, 10);
        aucBuf[10] = '\0';
        u4Offset +=
            snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                     "CR4 tailer version %s (%s) info %u:E%u\n",
                     aucBuf, prTailer->ram_built_date,
                     prTailer->chip_info, prTailer->eco_code + 1);
    } else {
        prTailer = &prVerInfo->rCR4tailer;
        kalMemCopy(aucBuf, prTailer->ram_version, 10);
        aucBuf[10] = '\0';
        u4Offset +=
            snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                     "CR4 tailer version %s (%s) info %u:E%u\n",
                     aucBuf, prTailer->ram_built_date,
                     prTailer->chip_info, prTailer->eco_code + 1);
    }
#else
    prTailer = &prVerInfo->rN9tailer;
    kalMemCopy(aucBuf, prTailer->ram_version, 10);
    aucBuf[10] = '\0';
    kalMemCopy(aucDate, prTailer->ram_built_date,
               sizeof(prTailer->ram_built_date));
    aucDate[sizeof(prTailer->ram_built_date)] = '\0';
    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "N9  tailer version %s (%s) info %u:E%u\n", aucBuf,
                         aucDate, prTailer->chip_info,
                         prTailer->eco_code + 1);

    prTailer = &prVerInfo->rCR4tailer;
    kalMemCopy(aucBuf, prTailer->ram_version, 10);
    aucBuf[10] = '\0';
    kalMemCopy(aucDate, prTailer->ram_built_date,
               sizeof(prTailer->ram_built_date));
    aucDate[sizeof(prTailer->ram_built_date)] = '\0';
    u4Offset += snprintf(pcCommand + u4Offset,
                         i4TotalLen - u4Offset,
                         "CR4 tailer version %s (%s) info %u:E%u\n",
                         aucBuf,
                         aucDate,
                         prTailer->chip_info,
                         prTailer->eco_code + 1);
#endif
    if (!prVerInfo->fgPatchIsDlByDrv) {
        u4Offset += snprintf(
            pcCommand + u4Offset, i4TotalLen - u4Offset,
            "Patch is not downloaded by driver, read patch binary\n");
        wlanGetPatchInfo(prAdapter);
    }

    kalStrnCpy(aucBuf, prVerInfo->rPatchHeader.aucPlatform, 4);
    aucBuf[4] = '\0';
    kalStrnCpy(aucDate, prVerInfo->rPatchHeader.aucBuildDate, 16);
    aucDate[16] = '\0';
    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "Patch platform %s version 0x%04X %s\n", aucBuf,
                         prVerInfo->rPatchHeader.u4PatchVersion, aucDate);

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "WiFi Driver Version " NIC_DRIVER_VERSION_STRING
                         "\n");

    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}

#if CFG_SUPPORT_DBDC
int priv_driver_set_dbdc(IN struct net_device *prNetDev, IN char *pcCommand,
                         IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };

    u32 u4Ret, u4Parse = 0;

    u8 ucDBDCEnable;
    /*u8 ucBssIndex;*/
    /*P_BSS_INFO_T prBssInfo;*/

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    if (prGlueInfo->prAdapter->rWifiVar.ucDbdcMode != DBDC_MODE_DYNAMIC) {
        DBGLOG(REQ, LOUD,
               "Current DBDC mode %u cannot enable/disable DBDC!!\n",
               prGlueInfo->prAdapter->rWifiVar.ucDbdcMode);
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    if (i4Argc == 2) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        ucDBDCEnable = (u8)u4Parse;
        if ((!prGlueInfo->prAdapter->rWifiVar.fgDbDcModeEn &&
             !ucDBDCEnable) ||
            (prGlueInfo->prAdapter->rWifiVar.fgDbDcModeEn &&
             ucDBDCEnable)) {
            return i4BytesWritten;
        }

        rStatus = kalIoctl(prGlueInfo, wlanoidSetDbdcEnable,
                           &ucDBDCEnable, 1, false, false, true,
                           &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }
    } else {
        DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_DBDC <enable>\n");
        DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
    }

    return i4BytesWritten;
}
#endif

#if CFG_SUPPORT_BATCH_SCAN
#define CMD_BATCH_SET     "WLS_BATCHING SET"
#define CMD_BATCH_GET     "WLS_BATCHING GET"
#define CMD_BATCH_STOP    "WLS_BATCHING STOP"
#endif

static int priv_driver_get_que_info(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;

    ASSERT(prNetDev);
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    return qmDumpQueueStatus(prGlueInfo->prAdapter, pcCommand, i4TotalLen);
}

static int priv_driver_get_mem_info(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;

    ASSERT(prNetDev);
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    return cnmDumpMemoryStatus(prGlueInfo->prAdapter, pcCommand,
                               i4TotalLen);
}

static int priv_driver_get_hif_info(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;

    ASSERT(prNetDev);
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    return halDumpHifStatus(prGlueInfo->prAdapter, pcCommand, i4TotalLen);
}

int priv_driver_set_p2p_ps(IN struct net_device *prNetDev, IN char *pcCommand,
                           IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Ret;
    s32 i4Parameter;
    PARAM_CUSTOM_OPPPS_PARAM_STRUCT_T rOpppsParamInfo;
    u8 ucRoleIdx = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    if (i4Argc >= 3) {
        /* get Bss Index from ndev */
        if (mtk_Netdev_To_RoleIdx(prGlueInfo, prNetDev, &ucRoleIdx) !=
            0) {
            return -1;
        }

        if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter, ucRoleIdx,
                                &rOpppsParamInfo.ucBssIdx) !=
            WLAN_STATUS_SUCCESS) {
            return -1;
        }

        DBGLOG(REQ, LOUD, "priv_driver_set_p2p_ps bss Idx %u\n",
               rOpppsParamInfo.ucBssIdx);

        u4Ret = kalkStrtos32(apcArgv[1], 0, &i4Parameter);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv[1] error u4Ret=%d\n",
                   u4Ret);
        }

        if (i4Parameter >= 1) {
            rOpppsParamInfo.ucLegcyPS = 1;
        }else{
            rOpppsParamInfo.ucLegcyPS = 0;
        }

        u4Ret = kalkStrtos32(apcArgv[2], 0, &i4Parameter);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv[2] error u4Ret=%d\n",
                   u4Ret);
        }

        if (i4Parameter >= 1) {
            rOpppsParamInfo.ucOppPs = 1;
        }else{
            rOpppsParamInfo.ucOppPs = 0;
        }

        u4Ret = kalkStrtos32(apcArgv[3], 0, &i4Parameter);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse apcArgv[2] error u4Ret=%d\n",
                   u4Ret);
        }

        if (rOpppsParamInfo.ucOppPs) {
            rOpppsParamInfo.u4CTwindowMs = (u32)i4Parameter;
        }

        rStatus = kalIoctl(prGlueInfo, wlanoidSetOppPsParam,
                           &rOpppsParamInfo, sizeof(rOpppsParamInfo),
                           false, false, true, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }
    } else {
        DBGLOG(INIT,
               ERROR,
               "ERR, iwpriv wlanXX driver SET_P2P_PS <legacy ps> <Oppps> <CTW>\n");
    }

    return i4BytesWritten;
}

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
static int priv_driver_get_mcs_info(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0, i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    P_PARAM_HW_WLAN_INFO_T prHwWlanInfo = NULL;
    struct PARAM_TX_MCS_INFO *prTxMcsInfo = NULL;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (prGlueInfo->prAdapter->rRxMcsInfoTimer.pfMgmtTimeOutFunc == NULL) {
        cnmTimerInitTimer(
            prGlueInfo->prAdapter,
            &prGlueInfo->prAdapter->rRxMcsInfoTimer,
            (PFN_MGMT_TIMEOUT_FUNC)aisRxMcsCollectionTimeout,
            (unsigned long)NULL);
    }

    if (i4Argc >= 2) {
        if (strnicmp(apcArgv[1], "START", strlen("START")) == 0) {
            cnmTimerStartTimer(
                prGlueInfo->prAdapter,
                &prGlueInfo->prAdapter->rRxMcsInfoTimer, 100);
            prGlueInfo->prAdapter->fgIsMcsInfoValid = true;

            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "\nStart the MCS Info Function\n");
            return i4BytesWritten;
        } else if (strnicmp(apcArgv[1], "STOP", strlen("STOP")) == 0) {
            cnmTimerStopTimer(
                prGlueInfo->prAdapter,
                &prGlueInfo->prAdapter->rRxMcsInfoTimer);
            prGlueInfo->prAdapter->fgIsMcsInfoValid = false;

            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "\nStop the MCS Info Function\n");
            return i4BytesWritten;
        }
    }

    if (prGlueInfo->prAdapter->fgIsMcsInfoValid != true) {
        i4BytesWritten += kalScnprintf(
            pcCommand + i4BytesWritten,
            i4TotalLen - i4BytesWritten,
            "\nUse GET_MCS_INFO [START/STOP] to control the MCS Info Function\n");
        return i4BytesWritten;
    }

    prHwWlanInfo = (P_PARAM_HW_WLAN_INFO_T)kalMemAlloc(
        sizeof(PARAM_HW_WLAN_INFO_T), VIR_MEM_TYPE);
    if (!prHwWlanInfo) {
        return -1;
    }

    kalMemZero(prHwWlanInfo, sizeof(PARAM_HW_WLAN_INFO_T));

    /* TODO: frog 20180518: Glue layer no need to know HW wlan idx
     * information. Glue layer only know AIS network. But
     * wlanoidQueryWlanInfo is used by other APIs and can get WTBL info by
     * providing wlan index. Method I: Use an IOCTL to get AIS wlan index
     * first. then do WTBL IOCTL query afterward. Method II: If wlan index
     * is not specified(RESERVED ENTRY). Driver get WLAN index of the AIS
     * network by default. So that it won't impact other existing feature.
     */
    prHwWlanInfo->u4Index = WTBL_RESERVED_ENTRY;

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo, prHwWlanInfo,
                       sizeof(PARAM_HW_WLAN_INFO_T), true, true, true,
                       &u4BufLen);

    DBGLOG(REQ, INFO, "rStatus %u u4BufLen = %d\n", rStatus, u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        goto out;
    }

    prTxMcsInfo = (struct PARAM_TX_MCS_INFO *)kalMemAlloc(
        sizeof(struct PARAM_TX_MCS_INFO), VIR_MEM_TYPE);
    if (!prTxMcsInfo) {
        goto out;
    }

    /* TODO: frog 20180518: Let the function wlanoidTxMcsInfo decide the
     * station record. Currently it is hard coded for ony AIS network.
     * Target STA is a connected AP. If we want make this function more
     * common and general, we need define new parameters.
     */
    rStatus = kalIoctl(prGlueInfo, wlanoidTxMcsInfo, prTxMcsInfo,
                       sizeof(struct PARAM_TX_MCS_INFO), true, true, true,
                       &u4BufLen);

    DBGLOG(REQ, INFO, "rStatus %u u4BufLen = %d\n", rStatus, u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        goto out;
    }

    i4BytesWritten = priv_driver_last_sec_mcs_info(prGlueInfo->prAdapter,
                                                   pcCommand, i4TotalLen,
                                                   prHwWlanInfo,
                                                   prTxMcsInfo);

    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

out:
    if (prHwWlanInfo) {
        kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
                   sizeof(PARAM_HW_WLAN_INFO_T));
    }
    if (prTxMcsInfo) {
        kalMemFree(prTxMcsInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_TX_MCS_INFO));
    }

    return i4BytesWritten;
}
#endif

static int priv_driver_get_deep_sleep_cnt(IN struct net_device *prNetDev,
                                          IN char *pcCommand,
                                          IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    CNM_STATUS_T rCnmStatus;
    u32 *pu4Ptr;
    u32 u4Offset = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = 0xb0100000;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    pu4Ptr = (u32 *)&rCnmStatus;
    *pu4Ptr = rSwCtrlInfo.u4Data;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "Deep Sleep Cnt %d\n", rSwCtrlInfo.u4Data);

    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}

static int priv_driver_get_cnm_info(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    CNM_STATUS_T rCnmStatus;
    u32 *pu4Ptr;
    P_CNM_CH_LIST_T prChList;
    u32 u4Offset = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = 0xb0000000;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    pu4Ptr = (u32 *)&rCnmStatus;
    *pu4Ptr = rSwCtrlInfo.u4Data;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "DBDC is %s, %u CHs in BAND0, %u CHs in BAND1\n",
                         rCnmStatus.fgDbDcModeEn ? "ON" : "OFF",
                         rCnmStatus.ucChNumB0, rCnmStatus.ucChNumB1);

    if (rCnmStatus.ucChNumB0 > 0) {
        rSwCtrlInfo.u4Id = 0xb0010000;

        rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead,
                           &rSwCtrlInfo, sizeof(rSwCtrlInfo), true,
                           true, true, &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }

        prChList = (P_CNM_CH_LIST_T)&rSwCtrlInfo.u4Data;

        u4Offset += snprintf(pcCommand + u4Offset,
                             i4TotalLen - u4Offset,
                             "BAND0 channels : %u %u %u\n",
                             prChList->ucChNum[0], prChList->ucChNum[1],
                             prChList->ucChNum[2]);
    }
    if (rCnmStatus.ucChNumB1 > 0) {
        rSwCtrlInfo.u4Id = 0xb0010001;

        rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead,
                           &rSwCtrlInfo, sizeof(rSwCtrlInfo), true,
                           true, true, &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS) {
            return -1;
        }

        prChList = (P_CNM_CH_LIST_T)&rSwCtrlInfo.u4Data;

        u4Offset += snprintf(pcCommand + u4Offset,
                             i4TotalLen - u4Offset,
                             "BAND1 channels : %u %u %u\n",
                             prChList->ucChNum[0], prChList->ucChNum[1],
                             prChList->ucChNum[2]);
    }

    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
static int priv_driver_get_ch_rank_list(IN struct net_device *prNetDev,
                                        IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    u32 i4BytesWritten = 0;
    s8 ucIdx = 0, ucIdx2 = 0, ucChannelNum = 0, ucNumOf2gChannel = 0,
       ucNumOf5gChannel = 0;
    P_PARAM_GET_CHN_INFO prChnLoadInfo = NULL;
    RF_CHANNEL_INFO_T *prChannelList = NULL,
                      auc2gChannelList[MAX_2G_BAND_CHN_NUM],
                      auc5gChannelList[MAX_5G_BAND_CHN_NUM];

    ASSERT(prNetDev);
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prChnLoadInfo = &(prGlueInfo->prAdapter->rWifiVar.rChnLoadInfo);
    kalMemZero(pcCommand, i4TotalLen);

    rlmDomainGetChnlList(prGlueInfo->prAdapter, BAND_2G4, true,
                         MAX_2G_BAND_CHN_NUM, &ucNumOf2gChannel,
                         auc2gChannelList);
    rlmDomainGetChnlList(prGlueInfo->prAdapter, BAND_5G, true,
                         MAX_5G_BAND_CHN_NUM, &ucNumOf5gChannel,
                         auc5gChannelList);

    for (ucIdx = 0; ucIdx < MAX_CHN_NUM; ucIdx++) {
        if (prChnLoadInfo->rChnRankList[ucIdx].ucChannel > 14) {
            prChannelList = auc5gChannelList;
            ucChannelNum = ucNumOf5gChannel;
        } else {
            prChannelList = auc2gChannelList;
            ucChannelNum = ucNumOf2gChannel;
        }

        for (ucIdx2 = 0; ucIdx2 < ucChannelNum; ucIdx2++) {
            if (prChnLoadInfo->rChnRankList[ucIdx].ucChannel ==
                prChannelList[ucIdx2].ucChannelNum) {
                pcCommand[i4BytesWritten++] =
                    prChnLoadInfo->rChnRankList[ucIdx]
                    .ucChannel;
                DBGLOG(SCN, TRACE, "ch %u, dirtiness %d\n",
                       prChnLoadInfo->rChnRankList[ucIdx]
                       .ucChannel,
                       prChnLoadInfo->rChnRankList[ucIdx]
                       .u4Dirtiness);
                break;
            }
        }
    }

    return i4BytesWritten;
}

static int priv_driver_get_ch_dirtiness(IN struct net_device *prNetDev,
                                        IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s8 cIdx = 0;
    u8 ucNumOf2gChannel = 0;
    u8 ucNumOf5gChannel = 0;
    u32 i4BytesWritten = 0;
    P_PARAM_GET_CHN_INFO prChnLoadInfo = NULL;
    RF_CHANNEL_INFO_T ar2gChannelList[MAX_2G_BAND_CHN_NUM];
    RF_CHANNEL_INFO_T ar5gChannelList[MAX_5G_BAND_CHN_NUM];

    ASSERT(prNetDev);
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prChnLoadInfo = &(prGlueInfo->prAdapter->rWifiVar.rChnLoadInfo);
    kalMemZero(pcCommand, i4TotalLen);

    rlmDomainGetChnlList(prGlueInfo->prAdapter, BAND_2G4, true,
                         MAX_2G_BAND_CHN_NUM, &ucNumOf2gChannel,
                         ar2gChannelList);
    rlmDomainGetChnlList(prGlueInfo->prAdapter, BAND_5G, true,
                         MAX_5G_BAND_CHN_NUM, &ucNumOf5gChannel,
                         ar5gChannelList);

    for (cIdx = 0; cIdx < MAX_CHN_NUM; cIdx++) {
        s8 cIdx2 = 0;
        u8 ucChannelNum = 0;
        u32 u4Offset = 0;
        RF_CHANNEL_INFO_T *prChannelList = NULL;

        if (prChnLoadInfo->rChnRankList[cIdx].ucChannel > 14) {
            prChannelList = ar5gChannelList;
            ucChannelNum = ucNumOf5gChannel;
        } else {
            prChannelList = ar2gChannelList;
            ucChannelNum = ucNumOf2gChannel;
        }

        for (cIdx2 = 0; cIdx2 < ucChannelNum; cIdx2++) {
            if (prChnLoadInfo->rChnRankList[cIdx].ucChannel ==
                prChannelList[cIdx2].ucChannelNum) {
                u4Offset = kalSnprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "\nch %03u -> dirtiness %lu",
                    prChnLoadInfo->rChnRankList[cIdx]
                    .ucChannel,
                    prChnLoadInfo->rChnRankList[cIdx]
                    .u4Dirtiness);
                i4BytesWritten += u4Offset;
                break;
            }

            if (i4BytesWritten >= i4TotalLen) {
                return i4BytesWritten;
            }
        }
    }

    return i4BytesWritten;
}
#endif

static int priv_driver_efuse_ops(IN struct net_device *prNetDev,
                                 IN char *pcCommand, IN int i4TotalLen)
{
    enum EFUSE_OP_MODE {
        EFUSE_READ,
        EFUSE_WRITE,
        EFUSE_FREE,
        EFUSE_INVALID,
    };
    u8 ucOpMode = EFUSE_INVALID;
    u8 ucOpChar;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 u4Ret;
    s32 i4Parameter;
    u32 u4Efuse_addr = 0;
    u8 ucEfuse_value = 0;

#if (CFG_EEPROM_PAGE_ACCESS == 1)
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4Offset = 0;
    u32 u4BufLen = 0;
    u8 u4Index = 0;
    P_GLUE_INFO_T prGlueInfo = NULL;
    PARAM_CUSTOM_ACCESS_EFUSE_T rAccessEfuseInfo;
#endif
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    /* Sanity check */
    if (i4Argc < 2) {
        goto efuse_op_invalid;
    }

    ucOpChar = (u8)apcArgv[1][0];
    if ((i4Argc == 3) && (ucOpChar == 'r' || ucOpChar == 'R')) {
        ucOpMode = EFUSE_READ;
    }else if ((i4Argc == 4) && (ucOpChar == 'w' || ucOpChar == 'W')) {
        ucOpMode = EFUSE_WRITE;
    }else if ((ucOpChar == 'f' || ucOpChar == 'F')) {
        ucOpMode = EFUSE_FREE;
    }

    /* Print out help if input format is wrong */
    if (ucOpMode == EFUSE_INVALID) {
        goto efuse_op_invalid;
    }

    /* convert address */
    if (ucOpMode == EFUSE_READ || ucOpMode == EFUSE_WRITE) {
        u4Ret = kalkStrtos32(apcArgv[2], 16, &i4Parameter);
        u4Efuse_addr = (u32)i4Parameter;
    }

    /* convert value */
    if (ucOpMode == EFUSE_WRITE) {
        u4Ret = kalkStrtos32(apcArgv[3], 16, &i4Parameter);
        ucEfuse_value = (u8)i4Parameter;
    }

    /* Start operation */
#if (CFG_EEPROM_PAGE_ACCESS == 1)
    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    kalMemSet(&rAccessEfuseInfo, 0, sizeof(PARAM_CUSTOM_ACCESS_EFUSE_T));
    rAccessEfuseInfo.u4Address =
        (u4Efuse_addr / EFUSE_BLOCK_SIZE) * EFUSE_BLOCK_SIZE;
    u4Index = u4Efuse_addr % EFUSE_BLOCK_SIZE;

    if (ucOpMode == EFUSE_READ) {
        rStatus = kalIoctl(prGlueInfo,
                           wlanoidQueryProcessAccessEfuseRead,
                           &rAccessEfuseInfo,
                           sizeof(PARAM_CUSTOM_ACCESS_EFUSE_T), true,
                           true, true, &u4BufLen);

        if (rStatus == WLAN_STATUS_SUCCESS) {
            u4Offset += snprintf(
                pcCommand + u4Offset, i4TotalLen - u4Offset,
                "Read success 0x%X = 0x%X\n", u4Efuse_addr,
                prGlueInfo->prAdapter->aucEepromVaule[u4Index]);
        }
    } else if (ucOpMode == EFUSE_WRITE) {
        prGlueInfo->prAdapter->aucEepromVaule[u4Index] = ucEfuse_value;

        kalMemCopy(rAccessEfuseInfo.aucData,
                   prGlueInfo->prAdapter->aucEepromVaule, 16);

        rStatus = kalIoctl(prGlueInfo,
                           wlanoidQueryProcessAccessEfuseWrite,
                           &rAccessEfuseInfo,
                           sizeof(PARAM_CUSTOM_ACCESS_EFUSE_T), false,
                           false, true, &u4BufLen);
        if (rStatus == WLAN_STATUS_SUCCESS) {
            u4Offset += snprintf(pcCommand + u4Offset,
                                 i4TotalLen - u4Offset,
                                 "Write success 0x%X = 0x%X\n",
                                 u4Efuse_addr, ucEfuse_value);
        }
    } else if (ucOpMode == EFUSE_FREE) {
        PARAM_CUSTOM_EFUSE_FREE_BLOCK_T rEfuseFreeBlock = {};

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryEfuseFreeBlock,
                           &rEfuseFreeBlock,
                           sizeof(PARAM_CUSTOM_EFUSE_FREE_BLOCK_T),
                           true, true, true, &u4BufLen);
        if (rStatus == WLAN_STATUS_SUCCESS) {
            u4Offset += snprintf(
                pcCommand + u4Offset, i4TotalLen - u4Offset,
                "Free block size 0x%X\n",
                prGlueInfo->prAdapter->u4FreeBlockNum);
        }
    }
#else
    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "efuse ops is invalid\n");
#endif

    return (s32)u4Offset;

efuse_op_invalid:

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "\nHelp menu\n");
    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "\tRead:\t\"efuse read addr_hex\"\n");
    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "\tWrite:\t\"efuse write addr_hex val_hex\"\n");
    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "\tFree Blocks:\t\"efuse free\"\n");
    return (s32)u4Offset;
}

#if CFG_SUPPORT_ADVANCE_CONTROL
static int priv_driver_set_noise(IN struct net_device *prNetDev,
                                 IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0;
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_NOISE_ID;
    u32 u4Sel = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    rSwCtrlInfo.u4Id = u4Id;

    if (i4Argc <= 1) {
        DBGLOG(REQ, ERROR, "Argc(%d) ERR: SET_NOISE <Sel>\n", i4Argc);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);
    if (u4Ret) {
        DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);
        return -1;
    }

    rSwCtrlInfo.u4Data = u4Sel << 30;
    DBGLOG(REQ, LOUD, "u4Sel=%d u4Data=0x%x,\n", u4Sel, rSwCtrlInfo.u4Data);
    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

static int priv_driver_get_noise(IN struct net_device *prNetDev,
                                 IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_NOISE_ID;
    u32 u4Offset = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    s16 u2Wf0AvgPwr, u2Wf1AvgPwr;

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    u2Wf0AvgPwr = rSwCtrlInfo.u4Data & 0xFFFF;
    u2Wf1AvgPwr = (rSwCtrlInfo.u4Data >> 16) & 0xFFFF;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "Noise Idle Avg. Power: WF0:%ddB WF1:%ddB\n",
                         u2Wf0AvgPwr, u2Wf1AvgPwr);

    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}

#ifdef CFG_SUPPORT_ADMINCTRL
static int priv_driver_admin_ctrl_config(IN struct net_device *prNetDev,
                                         IN char *pcCommand, IN int i4TotalLen)
{
    struct CMD_ADMIN_CTRL_CONFIG *cmd = NULL;
    struct ADMIN_CTRL_PARAM *content = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    P_GLUE_INFO_T prGlueInfo;
    s32 i4BytesWritten = 0;
    u32 u4BufLen = 0;
    s32 i4Argc = 0;
    s32 u4Ret = 0;
    s32 i = 0;
    u16 u2Val = 0;
    u8 arAdminTbl[ADMIN_CTRL_TBL_ENTRY_NUM] = { 0 };
    u8 ucVal = 0;
    u8 fgWaitResp = false;
    u8 txmode, rate;
    u8 nss;

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        goto admin_ctrl_invalid;
    }

    cmd = (struct CMD_ADMIN_CTRL_CONFIG *)kalMemAlloc(sizeof(*cmd),
                                                      VIR_MEM_TYPE);
    if (!cmd) {
        goto admin_ctrl_invalid;
    }

    if ((i4Argc > 14) || (i4Argc < 2)) {
        goto admin_ctrl_invalid;
    }

    memset(cmd, 0, sizeof(*cmd));
    content = &cmd->content;
    cmd->u2Type = CMD_ADMINCTRL_CONFIG_TYPE;
    cmd->u2Len = sizeof(*cmd);
    /* get command */
    if (strnicmp(apcArgv[1], "AUTO", strlen("AUTO")) == 0) {
        if (i4Argc != 2) {
            goto admin_ctrl_invalid;
        }
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->u2Action = ADMIN_CTRL_SET_MODE;
        content->u2eMode = ADMIN_CTRL_MODE_AUTO;
    } else if (strnicmp(apcArgv[1], "DISABLE", strlen("DISABLE")) == 0) {
        if (i4Argc != 2) {
            goto admin_ctrl_invalid;
        }
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->u2Action = ADMIN_CTRL_SET_MODE;
        content->u2eMode = ADMIN_CTRL_MODE_DIS;
    } else if (strnicmp(apcArgv[1], "MAN", strlen("MAN")) == 0) {
        if (i4Argc != 3) {
            goto admin_ctrl_invalid;
        }

        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->u2Action = ADMIN_CTRL_SET_MODE;
        u4Ret = kalkStrtou16(apcArgv[2], 0, &u2Val);

        if ((u2Val > ADMIN_CTRL_MAX_PERCENTAGE) || u4Ret) {
            goto admin_ctrl_invalid;
        }
        content->u2eMode = ADMIN_CTRL_MODE_MAN;
        content->u2ForceAdminTime = u2Val;
    } else if (strnicmp(apcArgv[1], "RESET", strlen("RESET")) == 0) {
        if (i4Argc != 2) {
            goto admin_ctrl_invalid;
        }

        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->u2Action = ADMIN_CTRL_SET_MODE;
        content->u2eMode = ADMIN_CTRL_MODE_RESET;
    } else if (strnicmp(apcArgv[1], "ADMIN_BASE", strlen("ADMIN_BASE")) ==
               0) {
        if (i4Argc != 3) {
            goto admin_ctrl_invalid;
        }
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->u2Action = ADMIN_CTRL_SET_BASE;
        u4Ret = kalkStrtou16(apcArgv[2], 0, &u2Val);

        if ((!u2Val) || u4Ret) {
            goto admin_ctrl_invalid;
        }
        content->u2AdminCtrlBase = u2Val;
    } else if (strnicmp(apcArgv[1], "TBL1", strlen("TBL1")) == 0) {
        if (i4Argc != 8) {
            goto admin_ctrl_invalid;
        }
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->u2Action = ADMIN_CTRL_SET_TBL1;
        for (i = 0; i < ADMIN_CTRL_TBL_ENTRY_NUM; i++) {
            u4Ret = kalkStrtou8(apcArgv[i + 2], 0, &ucVal);
            arAdminTbl[i] = ucVal;
            if (u4Ret) {
                goto admin_ctrl_invalid;
            }
        }
        memcpy(content->aucAdminTbl1, arAdminTbl,
               sizeof(*content->aucAdminTbl1) *
               ADMIN_CTRL_TBL_ENTRY_NUM);
    } else if (strnicmp(apcArgv[1], "TBL2", strlen("TBL2")) == 0) {
        if (i4Argc != 8) {
            goto admin_ctrl_invalid;
        }
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->u2Action = ADMIN_CTRL_SET_TBL2;
        for (i = 0; i < ADMIN_CTRL_TBL_ENTRY_NUM; i++) {
            u4Ret = kalkStrtou8(apcArgv[i + 2], 0, &ucVal);
            arAdminTbl[i] = ucVal;
            if (u4Ret) {
                goto admin_ctrl_invalid;
            }
        }
        memcpy(content->aucAdminTbl2, arAdminTbl,
               sizeof(*content->aucAdminTbl2) *
               ADMIN_CTRL_TBL_ENTRY_NUM);
    } else if (strnicmp(apcArgv[1], "TBL3", strlen("TBL3")) == 0) {
        if (i4Argc != 8) {
            goto admin_ctrl_invalid;
        }
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->u2Action = ADMIN_CTRL_SET_TBL3;
        for (i = 0; i < ADMIN_CTRL_TBL_ENTRY_NUM; i++) {
            u4Ret = kalkStrtou8(apcArgv[i + 2], 0, &ucVal);
            arAdminTbl[i] = ucVal;
            if (u4Ret) {
                goto admin_ctrl_invalid;
            }
        }
        memcpy(content->aucAdminTbl3, arAdminTbl,
               sizeof(*content->aucAdminTbl3) *
               ADMIN_CTRL_TBL_ENTRY_NUM);
    } else if (strnicmp(apcArgv[1], "METHOD1", strlen("METHOD1")) == 0) {
        if (i4Argc != 3) {
            goto admin_ctrl_invalid;
        }
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->u2Action = ADMIN_CTRL_SET_METHOD;
        u4Ret = kalkStrtou8(apcArgv[i + 2], 0, &ucVal);
        if (u4Ret) {
            goto admin_ctrl_invalid;
        }
        if (ucVal) {
            cmd->content.ucAdminStatus |= ADMIN_METHOD1_ENABLED;
        }
    } else if (strnicmp(apcArgv[1], "METHOD2", strlen("METHOD2")) == 0) {
        if (i4Argc != 3) {
            goto admin_ctrl_invalid;
        }
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->u2Action = ADMIN_CTRL_SET_METHOD;
        u4Ret = kalkStrtou8(apcArgv[i + 2], 0, &ucVal);
        if (u4Ret) {
            goto admin_ctrl_invalid;
        }
        if (ucVal) {
            cmd->content.ucAdminStatus |= ADMIN_METHOD2_ENABLED;
        }
    } else if (strnicmp(apcArgv[1], "GET", strlen("GET")) == 0) {
        fgWaitResp = true;
    } else {
        goto admin_ctrl_invalid;
    }

    DBGLOG(REQ, INFO, "%s(%s) action %x wait_resp %x\n", __func__,
           pcCommand, cmd->u2Action, fgWaitResp);

    rStatus = kalIoctl(prGlueInfo, wlanoidAdvCtrl, cmd, sizeof(*cmd), true,
                       true, true, &u4BufLen);

    if ((rStatus != WLAN_STATUS_SUCCESS) &&
        (rStatus != WLAN_STATUS_PENDING)) {
        i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                    i4TotalLen - i4BytesWritten,
                                    "\ncommand failed %x", rStatus);
    } else if (!(cmd->u2Type & CMD_ADV_CONTROL_SET)) {
        /* print info if its get command */
        i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                    i4TotalLen - i4BytesWritten,
                                    "\nAdmission Ctrl Info:");
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nStatus[0x%x]: %s", content->ucAdminStatus,
            (content->ucAdminStatus & ADMIN_LINK_2G) ?
            "2.4G connected" :
            (content->ucAdminStatus & ADMIN_LINK_OTHER) ?
            "5G connected" :
            "Disconnected");
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tAdminCtrl: %s",
            (content->ucAdminStatus & ADMIN_ENABLED) ? "Enabled" :
            "Disabled");
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\tPer-Packet RW enabled: %s",
            (content->ucAdminStatus & ADMIN_PER_PKT_ENABLED) ?
            "Enabled" :
            "Disabled");
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nperiod %uus, %% %u%%, mode:%u(0:dis,1:auto,2:man)",
            content->u2AdminCtrlBase, content->u2CurAdminTime,
            content->u2eMode);
        if (content->u2eMode == ADMIN_CTRL_MODE_MAN) {
            i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                        i4TotalLen - i4BytesWritten,
                                        " force_admin_%%: %u%%",
                                        content->u2ForceAdminTime);
        }
        if (content->ucAdminThermalLimit < 100) {
            i4BytesWritten +=
                scnprintf(pcCommand + i4BytesWritten,
                          i4TotalLen - i4BytesWritten,
                          "\nLimited %% from thermal: %u%%",
                          content->ucAdminThermalLimit);
        }
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nBT Info[%x]:", content->u4CoexMode);
        i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                    i4TotalLen - i4BytesWritten, " %s",
                                    (content->u4CoexMode &
                                     BT_PROF_LINK_CONNECTED) ?
                                    "Connected" :
                                    "None");
        if (content->u4CoexMode & BT_PROF_LINK_CONNECTED) {
            i4BytesWritten += scnprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten, ", %s",
                (content->u4CoexMode & BT_PROF_A2DP_SRC) ?
                "A2DP SRC" :
                (content->u4CoexMode & BT_PROF_A2DP_SINK) ?
                "A2DP SINK" :
                "short-term Profile");
        }

        if (content->ucAdminStatus & ADMIN_LINK_2G) {
            i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                        i4TotalLen - i4BytesWritten,
                                        "\nInstant Rate Tbl:");
            for (i = 0; i < AUTO_RATE_NUM; i++) {
                i4BytesWritten +=
                    scnprintf(pcCommand + i4BytesWritten,
                              i4TotalLen - i4BytesWritten,
                              "\nRate[%d]: ", i + 1);
                txmode = HW_TX_RATE_TO_MODE(
                    content->au2RateCode[i]);
                if (txmode >= MAX_TX_MODE) {
                    txmode = MAX_TX_MODE;
                }
                rate = HW_TX_RATE_TO_MCS(
                    content->au2RateCode[i], txmode);
                nss = HW_TX_RATE_TO_NSS(
                    content->au2RateCode[i]) +
                      1;
                if (txmode == TX_RATE_MODE_CCK) {
                    i4BytesWritten += kalScnprintf(
                        pcCommand + i4BytesWritten,
                        i4TotalLen - i4BytesWritten,
                        "%s",
                        HW_TX_RATE_CCK_STR[rate & 0x3]);
                } else if (txmode == TX_RATE_MODE_OFDM) {
                    i4BytesWritten += kalScnprintf(
                        pcCommand + i4BytesWritten,
                        i4TotalLen - i4BytesWritten,
                        "%s", hw_rate_ofdm_str(rate));
                } else if ((txmode == TX_RATE_MODE_HTMIX) ||
                           (txmode == TX_RATE_MODE_HTGF)) {
                    i4BytesWritten += kalScnprintf(
                        pcCommand + i4BytesWritten,
                        i4TotalLen - i4BytesWritten,
                        "MCS%d", rate);
                } else {
                    i4BytesWritten += kalScnprintf(
                        pcCommand + i4BytesWritten,
                        i4TotalLen - i4BytesWritten,
                        "NSS%d_MCS%d", nss, rate);
                }
            }
        }
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nTBL_#:\tCCK%%\tOFDM%%\tBPSK%%\tQPSK%%");
        i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                    i4TotalLen - i4BytesWritten,
                                    "\tQAM16%%\tQAM64%%");
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nTbl1:\t%d%%\t%d%%\t%d%%\t%d%%\t%d%%\t%d%%",
            content->aucAdminTbl1[0], content->aucAdminTbl1[1],
            content->aucAdminTbl1[2], content->aucAdminTbl1[3],
            content->aucAdminTbl1[4], content->aucAdminTbl1[5]);
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nTbl2:\t%d%%\t%d%%\t%d%%\t%d%%\t%d%%\t%d%%",
            content->aucAdminTbl2[0], content->aucAdminTbl2[1],
            content->aucAdminTbl2[2], content->aucAdminTbl2[3],
            content->aucAdminTbl2[4], content->aucAdminTbl2[5]);
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nTbl3:\t%d%%\t%d%%\t%d%%\t%d%%\t%d%%\t%d%%",
            content->aucAdminTbl3[0], content->aucAdminTbl3[1],
            content->aucAdminTbl3[2], content->aucAdminTbl3[3],
            content->aucAdminTbl3[4], content->aucAdminTbl3[5]);
        i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                    i4TotalLen - i4BytesWritten,
                                    "\nLast Pick Tbl: Tbl%d",
                                    content->ucLastChosenTbl);
    } else {
        i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                    i4TotalLen - i4BytesWritten,
                                    "\ncommand sent %x", rStatus);
    }

    if (cmd) {
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    }

    return i4BytesWritten;

admin_ctrl_invalid:
    if (cmd) {
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    }
    i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                i4TotalLen - i4BytesWritten,
                                "\nformat:adminctrl ");
    i4BytesWritten += scnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "[auto|disable|man $admin_time|admin_base $admin_base|get]");
    i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                i4TotalLen - i4BytesWritten,
                                "\nformat:adminctrl ");
    i4BytesWritten += scnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "[tbl1 $CCK%% $OFDM%% $BPSK%% $QPSK%% $QAM_16%% $QAM_64%%]");
    i4BytesWritten += scnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n$admin_time: need < 100, which means 100%%");
    i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                i4TotalLen - i4BytesWritten,
                                "\n$admin_base: admin base in us");
    return i4BytesWritten;
}
#endif

static int priv_driver_get_traffic_report(IN struct net_device *prNetDev,
                                          IN char *pcCommand,
                                          IN int i4TotalLen)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    P_GLUE_INFO_T prGlueInfo;
    s32 i4BytesWritten = 0;
    u32 u4BufLen = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    struct CMD_GET_TRAFFIC_REPORT *cmd = NULL;
    u8 ucBand = ENUM_BAND_0;
    u16 u2Val = 0;
    u8 ucVal = 0;
    s32 u4Ret = 0;
    u8 fgWaitResp = false;
    u8 fgRead = false;
    u8 fgGetDbg = false;

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        goto get_report_invalid;
    }

    cmd = (struct CMD_GET_TRAFFIC_REPORT *)kalMemAlloc(sizeof(*cmd),
                                                       VIR_MEM_TYPE);
    if (!cmd) {
        goto get_report_invalid;
    }

    if ((i4Argc > 4) || (i4Argc < 2)) {
        goto get_report_invalid;
    }

    memset(cmd, 0, sizeof(*cmd));

    cmd->u2Type = CMD_GET_REPORT_TYPE;
    cmd->u2Len = sizeof(*cmd);
    cmd->ucBand = ucBand;

    if (strnicmp(apcArgv[1], "ENABLE", strlen("ENABLE")) == 0) {
        /* TrafficReport Enable cmd is blocked when entering suspend
         * mode */
        if (prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap &
            BLOCK_KEEP_FULL_PWR) {
            DBGLOG(REQ, STATE,
                   "TrafficReport Enable Command is blocked\n");
            goto get_report_invalid;
        }
        prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap |=
            KEEP_FULL_PWR_TRAFFIC_REPORT_BIT;
        cmd->ucAction = CMD_GET_REPORT_ENABLE;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
    } else if (strnicmp(apcArgv[1], "DISABLE", strlen("DISABLE")) == 0) {
        prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap &=
            ~KEEP_FULL_PWR_TRAFFIC_REPORT_BIT;
        cmd->ucAction = CMD_GET_REPORT_DISABLE;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
    } else if (strnicmp(apcArgv[1], "RESET", strlen("RESET")) == 0) {
        cmd->ucAction = CMD_GET_REPORT_RESET;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
    } else if ((strnicmp(apcArgv[1], "GET", strlen("GET")) == 0) ||
               (strnicmp(apcArgv[1], "GETDBG", strlen("GETDBG")) == 0)) {
        cmd->ucAction = CMD_GET_REPORT_GET;
        fgWaitResp = true;
        fgRead = true;
        if ((i4Argc == 4) &&
            (strnicmp(apcArgv[2], "BAND", strlen("BAND")) == 0)) {
            u4Ret = kalkStrtou8(apcArgv[3], 0, &ucVal);
            cmd->ucBand = ucVal;
        }
        if (strnicmp(apcArgv[1], "GETDBG", strlen("GETDBG")) == 0) {
            fgGetDbg = true;
        }
    } else if ((strnicmp(apcArgv[1], "SAMPLEPOINTS",
                         strlen("SAMPLEPOINTS")) == 0) &&
               (i4Argc == 3)) {
        u4Ret = kalkStrtou16(apcArgv[2], 0, &u2Val);
        cmd->u2SamplePoints = u2Val;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->ucAction = CMD_SET_REPORT_SAMPLE_POINT;
    } else if ((strnicmp(apcArgv[1], "TXTHRES", strlen("TXTHRES")) == 0) &&
               (i4Argc == 3)) {
        u4Ret = kalkStrtou8(apcArgv[2], 0, &ucVal);
        /* valid val range is from 0 - 100% */
        if (ucVal > 100) {
            ucVal = 100;
        }
        cmd->ucTxThres = ucVal;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->ucAction = CMD_SET_REPORT_TXTHRES;
    } else if ((strnicmp(apcArgv[1], "RXTHRES", strlen("RXTHRES")) == 0) &&
               (i4Argc == 3)) {
        u4Ret = kalkStrtou8(apcArgv[2], 0, &ucVal);
        /* valid val range is from 0 - 100% */
        if (ucVal > 100) {
            ucVal = 100;
        }
        cmd->ucRxThres = ucVal;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->ucAction = CMD_SET_REPORT_RXTHRES;
    } else {
        goto get_report_invalid;
    }

    DBGLOG(REQ, LOUD, "%s(%s) action %x band %x wait_resp %x\n", __func__,
           pcCommand, cmd->ucAction, ucBand, fgWaitResp);

    rStatus = kalIoctl(prGlueInfo, wlanoidAdvCtrl, cmd, sizeof(*cmd), true,
                       true, true, &u4BufLen);

    if ((rStatus != WLAN_STATUS_SUCCESS) &&
        (rStatus != WLAN_STATUS_PENDING)) {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\ncommand failed %x", rStatus);
    } else if (cmd->ucAction == CMD_GET_REPORT_GET) {
        int persentage = 0;
        int sample_dur = cmd->u4FetchEd - cmd->u4FetchSt;

        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nCCK false detect cnt: %d",
            (cmd->u4FalseCCA >> EVENT_REPORT_CCK_FCCA) &
            EVENT_REPORT_CCK_FCCA_FEILD);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nOFDM false detect cnt: %d",
            (cmd->u4FalseCCA >> EVENT_REPORT_OFDM_FCCA) &
            EVENT_REPORT_OFDM_FCCA_FEILD);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nCCK Sig CRC cnt: %d",
            (cmd->u4HdrCRC >> EVENT_REPORT_CCK_SIGERR) &
            EVENT_REPORT_CCK_SIGERR_FEILD);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nOFDM Sig CRC cnt: %d",
            (cmd->u4HdrCRC >> EVENT_REPORT_OFDM_SIGERR) &
            EVENT_REPORT_OFDM_SIGERR_FEILD);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nBand%d Info:", cmd->ucBand);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n\tSample every %u ms with %u points",
            cmd->u4TimerDur, cmd->u2SamplePoints);
        if (fgGetDbg) {
            i4BytesWritten += snprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                " from systime %u - %u total_dur %u us f_cost %u us t_drift %d ms",
                cmd->u4FetchSt,
                cmd->u4FetchEd,
                sample_dur,
                cmd->u4FetchCost,
                cmd->TimerDrift);
            i4BytesWritten += snprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "\n\tbusy-RMAC %u us, idle-TMAC %u us, t_total %u",
                cmd->u4ChBusy,
                cmd->u4ChIdle,
                cmd->u4ChBusy + cmd->u4ChIdle);
            i4BytesWritten += snprintf(
                pcCommand + i4BytesWritten,
                i4TotalLen - i4BytesWritten,
                "\n\theavy tx threshold %u%% rx threshold %u%%",
                cmd->ucTxThres, cmd->ucRxThres);
        }
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n\tch_busy %u us, ch_idle %u us, total_period %u us",
            sample_dur - cmd->u4ChIdle, cmd->u4ChIdle, sample_dur);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n\tmy_tx_time: %u us",
                                   cmd->u4TxAirTime);
        if (cmd->u4FetchEd - cmd->u4FetchSt) {
            persentage = cmd->u4TxAirTime / (sample_dur / 1000);
            i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       ", tx utility: %d.%1d%%",
                                       persentage / 10,
                                       persentage % 10);
        }
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n\tmy_data_rx_time (no BMC data): %u us",
            cmd->u4RxAirTime);
        if (cmd->u4FetchEd - cmd->u4FetchSt) {
            persentage = cmd->u4RxAirTime / (sample_dur / 1000);
            i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       ", rx utility: %d.%1d%%",
                                       persentage / 10,
                                       persentage % 10);
        }
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n\tTotal packet transmitted: %u",
                                   cmd->u4PktSent);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n\tTotal tx ok packet: %u",
                                   cmd->u4PktSent - cmd->u4PktTxfailed);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n\tTotal tx failed packet: %u",
                                   cmd->u4PktTxfailed);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n\tTotal tx retried packet: %u",
                                   cmd->u4PktRetried);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n\tTotal rx mpdu: %u",
                                   cmd->u4RxMPDU);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n\tTotal rx fcs: %u",
                                   cmd->u4RxFcs);
    } else {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\ncommand sent %x", rStatus);
    }

    if (cmd) {
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    }

    return i4BytesWritten;

get_report_invalid:
    if (cmd) {
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    }
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\nformat:get_report [enable|disable|get|reset]");
    return i4BytesWritten;
}

static int priv_driver_pta_config(IN struct net_device *prNetDev,
                                  IN char *pcCommand,
                                  IN int i4TotalLen)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    P_GLUE_INFO_T prGlueInfo;
    s32 i4BytesWritten = 0;
    u32 u4BufLen = 0;
    s32 i4Argc = 0;
    u32 u4Val = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    P_CMD_PTA_CONFIG_T cmd = NULL;
    s32 u4Ret = 0;
    s32 i = 0;

    DBGLOG(REQ, LOUD, "%s(%s)>\n", __func__, pcCommand);

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    if (i4Argc < 2) {
        goto set_pta_invalid;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        goto set_pta_invalid;
    }

    cmd = (P_CMD_PTA_CONFIG_T)kalMemAlloc(sizeof(*cmd), VIR_MEM_TYPE);
    if (!cmd) {
        goto set_pta_invalid;
    }

    memset(cmd, 0, sizeof(*cmd));
    cmd->u2Type = CMD_PTA_CONFIG_TYPE;
    cmd->u2Len = sizeof(*cmd);
    /* set command + parameter must be even number */
    if ((strnicmp(apcArgv[1], "SET", strlen("SET")) == 0) &&
        (i4Argc >= 4) && !(i4Argc & 1)) {
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        for (i = 2; i < i4Argc; i += 2) {
            u4Ret = kalkStrtou32(apcArgv[i + 1], 0, &u4Val);
            if (u4Ret) {
                i4BytesWritten += snprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "\nparsing err(%d) %s %s", u4Ret,
                    apcArgv[i], apcArgv[i + 1]);
                goto set_pta_invalid;
            }

            DBGLOG(REQ, LOUD, "arg[%d] %s %s (0x%x)\n", i,
                   apcArgv[i], apcArgv[i + 1], u4Val);
            if (strnicmp(apcArgv[i], "ENABLE", strlen("ENABLE")) ==
                0) {
                cmd->u4ConfigMask |= CMD_PTA_CONFIG_PTA;
                if (u4Val) {
                    cmd->u4PtaConfig |=
                        CMD_PTA_CONFIG_PTA_EN;
                }
            } else if (strnicmp(apcArgv[i], "TXDATA",
                                strlen("TXDATA")) == 0) {
                cmd->u4ConfigMask |= CMD_PTA_CONFIG_TXDATA_TAG;
                cmd->u4TxDataTag = u4Val;
            } else if (strnicmp(apcArgv[i], "RXDATAACK",
                                strlen("RXDATAACK")) == 0) {
                cmd->u4ConfigMask |=
                    CMD_PTA_CONFIG_RXDATAACK_TAG;
                cmd->u4RxDataAckTag = u4Val;
            } else if (strnicmp(apcArgv[i], "RXDATA",
                                strlen("RXDATA")) == 0) {
                cmd->u4ConfigMask |= CMD_PTA_CONFIG_RX_NSW_TAG;
                cmd->u4RxNswTag = u4Val;
            } else if (strnicmp(apcArgv[i], "TXACK",
                                strlen("TXACK")) == 0) {
                cmd->u4ConfigMask |= CMD_PTA_CONFIG_TXACK_TAG;
                cmd->u4TxAckTag = u4Val;
            } else if (strnicmp(apcArgv[i], "PROTTAG",
                                strlen("PROTTAG")) == 0) {
                cmd->u4ConfigMask |=
                    CMD_PTA_CONFIG_TXPROTFRAME_TAG;
                cmd->u4ConfigMask |=
                    CMD_PTA_CONFIG_RXPROTFRAMEACK_TAG;
                cmd->u4TxProtFrameTag = u4Val;
                cmd->u4RxProtFrameAckTag = u4Val;
            } else if (strnicmp(apcArgv[i], "TXBMC",
                                strlen("TXBMC")) == 0) {
                cmd->u4ConfigMask |= CMD_PTA_CONFIG_TX_BMC_TAG;
                cmd->u4TxBMCTag = u4Val;
            } else if (strnicmp(apcArgv[i], "TXBCN",
                                strlen("TXBCN")) == 0) {
                cmd->u4ConfigMask |= CMD_PTA_CONFIG_TX_BCN_TAG;
                cmd->u4TxBCNTag = u4Val;
            } else if (strnicmp(apcArgv[i], "RXBCN",
                                strlen("RXBCN")) == 0) {
                cmd->u4ConfigMask |= CMD_PTA_CONFIG_RX_SP_TAG;
                cmd->u4RxSPTag = u4Val;
            } else if (strnicmp(apcArgv[i], "TXMGMT",
                                strlen("TXMGMT")) == 0) {
                cmd->u4ConfigMask |= CMD_PTA_CONFIG_TX_MGMT_TAG;
                cmd->u4TxMgmtTag = u4Val;
            } else if (strnicmp(apcArgv[i], "RXMGMTACK",
                                strlen("RXMGMTACK")) == 0) {
                cmd->u4ConfigMask |=
                    CMD_PTA_CONFIG_RXMGMTACK_TAG;
                cmd->u4RxMgmtAckTag = u4Val;
            } else if (strnicmp(apcArgv[i], "STATENABLE",
                                strlen("STATENABLE")) == 0) {
                cmd->u4ConfigMask |= CMD_PTA_CONFIG_PTA_STAT;
                if (!u4Val) {
                    cmd->u4PtaConfig &=
                        ~CMD_PTA_CONFIG_PTA_STAT_EN;
                } else {
                    cmd->u4PtaConfig |=
                        CMD_PTA_CONFIG_PTA_STAT_EN;
                }
            } else if (strnicmp(apcArgv[i], "STATRESET",
                                strlen("STATRESET")) == 0) {
                cmd->u4ConfigMask |=
                    CMD_PTA_CONFIG_PTA_STAT_RESET;
            } else if (strnicmp(apcArgv[i],
                                "COMM_ACT_BT_WF0_INBAND",
                                strlen("COMM_ACT_BT_WF0_INBAND")) ==
                       0) {
                cmd->u4ConfigMask |=
                    CMD_PTA_CONFIG_COMM_ACT_BT_WF0_INBAND;
                cmd->u4CommActBtWf0Inband = u4Val;
            } else if (strnicmp(apcArgv[i],
                                "COMM_ACT_BT_WF0_OUTBAND",
                                strlen(
                                    "COMM_ACT_BT_WF0_OUTBAND"))
                       ==
                       0) {
                cmd->u4ConfigMask |=
                    CMD_PTA_CONFIG_COMM_ACT_BT_WF0_OUTBAND;
                cmd->u4CommActBtWf0Outband = u4Val;
            } else if (strnicmp(apcArgv[i],
                                "COMM_ACT_BT_WF1_INBAND",
                                strlen("COMM_ACT_BT_WF1_INBAND")) ==
                       0) {
                cmd->u4ConfigMask |=
                    CMD_PTA_CONFIG_COMM_ACT_BT_WF1_INBAND;
                cmd->u4CommActBtWf1Inband = u4Val;
            } else if (strnicmp(apcArgv[i],
                                "COMM_ACT_BT_WF1_OUTBAND",
                                strlen(
                                    "COMM_ACT_BT_WF1_OUTBAND"))
                       ==
                       0) {
                cmd->u4ConfigMask |=
                    CMD_PTA_CONFIG_COMM_ACT_BT_WF1_OUTBAND;
                cmd->u4CommActBtWf1Outband = u4Val;
            } else {
                i4BytesWritten +=
                    snprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "\nunknown parameter %s %s",
                             apcArgv[i], apcArgv[i + 1]);
                goto set_pta_invalid;
            }
        }

        rStatus = wlanSendSetQueryCmd(prGlueInfo->prAdapter,
                                      CMD_ID_ADV_CONTROL, true, false,
                                      false, NULL, NULL, sizeof(*cmd),
                                      (u8 *)cmd, NULL, 0);
    } else if (strnicmp(apcArgv[1], "GET", strlen("GET")) == 0) {
        rStatus = kalIoctl(prGlueInfo, wlanoidAdvCtrl, cmd,
                           sizeof(*cmd), true, true, true, &u4BufLen);
    } else {
        goto set_pta_invalid;
    }

    if ((rStatus != WLAN_STATUS_SUCCESS) &&
        (rStatus != WLAN_STATUS_PENDING)) {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\ncommand failed %x", rStatus);
    } else if (!(cmd->u2Type & CMD_ADV_CONTROL_SET)) {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nCoex mode: %s",
                                   (cmd->u4CoexMode) ? "FDD" : "TDD");
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nPTA status:");
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n module enable %x wifi %x bt %x wifi arb %x",
            (cmd->u4PtaConfig >> EVENT_CONFIG_PTA_OFFSET) &
            EVENT_CONFIG_PTA_FEILD,
            (cmd->u4PtaConfig >> EVENT_CONFIG_PTA_WIFI_OFFSET) &
            EVENT_CONFIG_PTA_WIFI_FEILD,
            (cmd->u4PtaConfig >> EVENT_CONFIG_PTA_BT_OFFSET) &
            EVENT_CONFIG_PTA_BT_FEILD,
            (cmd->u4PtaConfig >> EVENT_CONFIG_PTA_ARB_OFFSET) &
            EVENT_CONFIG_PTA_ARB_FEILD);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nPriority stat:");
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n txData %d", cmd->u4TxDataTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n rxDataAck %d",
                                   cmd->u4RxDataAckTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n rxData %d", cmd->u4RxNswTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n txAck %d", cmd->u4TxAckTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n txBmc %d", cmd->u4TxBMCTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n txBcn %d", cmd->u4TxBCNTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n txMgmt %d", cmd->u4TxMgmtTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n rxMgmtAck %d",
                                   cmd->u4RxMgmtAckTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n rxBcn %d", cmd->u4RxSPTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n prottag %d",
                                   cmd->u4TxProtFrameTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nLast fetched grant stat:");
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n wifi grant %x txreq %x rxreq %x pritag %x",
            (cmd->u4GrantStat >> EVENT_CONFIG_WIFI_GRANT_OFFSET) &
            EVENT_CONFIG_WIFI_GRANT_FEILD,
            (cmd->u4GrantStat >> EVENT_CONFIG_WIFI_TXREQ_OFFSET) &
            EVENT_CONFIG_WIFI_TXREQ_FEILD,
            (cmd->u4GrantStat >> EVENT_CONFIG_WIFI_RXREQ_OFFSET) &
            EVENT_CONFIG_WIFI_RXREQ_FEILD,
            (cmd->u4GrantStat >> EVENT_CONFIG_WIFI_PRI_OFFSET) &
            EVENT_CONFIG_WIFI_PRI_FEILD);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n bt grant %x txreq %x rxreq %x pritag %x",
            (cmd->u4GrantStat >> EVENT_CONFIG_BT_GRANT_OFFSET) &
            EVENT_CONFIG_BT_GRANT_FEILD,
            (cmd->u4GrantStat >> EVENT_CONFIG_BT_TXREQ_OFFSET) &
            EVENT_CONFIG_BT_TXREQ_FEILD,
            (cmd->u4GrantStat >> EVENT_CONFIG_BT_RXREQ_OFFSET) &
            EVENT_CONFIG_BT_RXREQ_FEILD,
            (cmd->u4GrantStat >> EVENT_CONFIG_BT_PRI_OFFSET) &
            EVENT_CONFIG_BT_PRI_FEILD);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nPTA stat:");
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n wf0 txreq_cnt %d txgrant_cnt %d txabort_cnt %d",
            (cmd->u4PtaWF0TxCnt >> EVENT_PTA_WFTRX_CNT_OFFSET) &
            EVENT_PTA_WFTRX_CNT_FEILD,
            (cmd->u4PtaWF0TxCnt >>
             EVENT_PTA_WFTRX_GRANT_CNT_OFFSET) &
            EVENT_PTA_WFTRX_GRANT_CNT_FEILD,
            (cmd->u4PtaWF0AbtCnt >> EVENT_PTA_TX_ABT_CNT_OFFSET) &
            EVENT_PTA_TX_ABT_CNT_FEILD);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n wf0 rxreq_cnt %d rxgrant_cnt %d rxabort_cnt %d",
            (cmd->u4PtaWF0RxCnt >> EVENT_PTA_WFTRX_CNT_OFFSET) &
            EVENT_PTA_WFTRX_CNT_FEILD,
            (cmd->u4PtaWF0RxCnt >>
             EVENT_PTA_WFTRX_GRANT_CNT_OFFSET) &
            EVENT_PTA_WFTRX_GRANT_CNT_FEILD,
            (cmd->u4PtaWF0AbtCnt >> EVENT_PTA_RX_ABT_CNT_OFFSET) &
            EVENT_PTA_RX_ABT_CNT_FEILD);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n wf1 txreq_cnt %d txgrant_cnt %d txabort_cnt %d",
            (cmd->u4PtaWF1TxCnt >> EVENT_PTA_WFTRX_CNT_OFFSET) &
            EVENT_PTA_WFTRX_CNT_FEILD,
            (cmd->u4PtaWF1TxCnt >>
             EVENT_PTA_WFTRX_GRANT_CNT_OFFSET) &
            EVENT_PTA_WFTRX_GRANT_CNT_FEILD,
            (cmd->u4PtaWF1AbtCnt >> EVENT_PTA_TX_ABT_CNT_OFFSET) &
            EVENT_PTA_TX_ABT_CNT_FEILD);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n wf1 rxreq_cnt %d rxgrant_cnt %d rxabort_cnt %d",
            (cmd->u4PtaWF1RxCnt >> EVENT_PTA_WFTRX_CNT_OFFSET) &
            EVENT_PTA_WFTRX_CNT_FEILD,
            (cmd->u4PtaWF1RxCnt >>
             EVENT_PTA_WFTRX_GRANT_CNT_OFFSET) &
            EVENT_PTA_WFTRX_GRANT_CNT_FEILD,
            (cmd->u4PtaWF1AbtCnt >> EVENT_PTA_RX_ABT_CNT_OFFSET) &
            EVENT_PTA_RX_ABT_CNT_FEILD);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n bt txreq_cnt %d txgrant_cnt %d txabort_cnt %d",
            (cmd->u4PtaBTTxCnt >> EVENT_PTA_BTTRX_CNT_OFFSET) &
            EVENT_PTA_BTTRX_CNT_FEILD,
            (cmd->u4PtaBTTxCnt >>
             EVENT_PTA_BTTRX_GRANT_CNT_OFFSET) &
            EVENT_PTA_BTTRX_GRANT_CNT_FEILD,
            (cmd->u4PtaBTAbtCnt >> EVENT_PTA_TX_ABT_CNT_OFFSET) &
            EVENT_PTA_TX_ABT_CNT_FEILD);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n bt rxreq_cnt %d rxgrant_cnt %d rxabort_cnt %d",
            (cmd->u4PtaBTRxCnt >> EVENT_PTA_BTTRX_CNT_OFFSET) &
            EVENT_PTA_BTTRX_CNT_FEILD,
            (cmd->u4PtaBTRxCnt >>
             EVENT_PTA_BTTRX_GRANT_CNT_OFFSET) &
            EVENT_PTA_BTTRX_GRANT_CNT_FEILD,
            (cmd->u4PtaBTAbtCnt >> EVENT_PTA_RX_ABT_CNT_OFFSET) &
            EVENT_PTA_RX_ABT_CNT_FEILD);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nCommon action stat (1:allow, 0:not allow):");
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n inband arb mode");
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_TX vs WF0_RX[b0]: %d",
            (cmd->u4CommActBtWf0Inband & BIT(0)) >> 0);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_RX vs WF0_TX[b1]: %d",
            (cmd->u4CommActBtWf0Inband & BIT(1)) >> 1);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_RX vs WF0_RX[b2]: %d",
            (cmd->u4CommActBtWf0Inband & BIT(2)) >> 2);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_TX vs WF0_TX[b3]: %d\n",
            (cmd->u4CommActBtWf0Inband & BIT(3)) >> 3);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_TX vs WF1_RX[b0]: %d",
            (cmd->u4CommActBtWf1Inband & BIT(0)) >> 0);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_RX vs WF1_TX[b1]: %d",
            (cmd->u4CommActBtWf1Inband & BIT(1)) >> 1);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_RX vs WF1_RX[b2]: %d",
            (cmd->u4CommActBtWf1Inband & BIT(2)) >> 2);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_TX vs WF1_TX[b3]: %d\n",
            (cmd->u4CommActBtWf1Inband & BIT(3)) >> 3);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n outband arb mode");
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_TX vs WF0_RX[b0]: %d",
            (cmd->u4CommActBtWf0Outband & BIT(0)) >> 0);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_RX vs WF0_TX[b1]: %d",
            (cmd->u4CommActBtWf0Outband & BIT(1)) >> 1);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_RX vs WF0_RX[b2]: %d",
            (cmd->u4CommActBtWf0Outband & BIT(2)) >> 2);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_TX vs WF0_TX[b3]: %d\n",
            (cmd->u4CommActBtWf0Outband & BIT(3)) >> 3);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_TX vs WF1_RX[b0]: %d",
            (cmd->u4CommActBtWf1Outband & BIT(0)) >> 0);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_RX vs WF1_TX[b1]: %d",
            (cmd->u4CommActBtWf1Outband & BIT(1)) >> 1);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_RX vs WF1_RX[b2]: %d",
            (cmd->u4CommActBtWf1Outband & BIT(2)) >> 2);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n BT_TX vs WF1_TX[b3]: %d\n",
            (cmd->u4CommActBtWf1Outband & BIT(3)) >> 3);
    } else {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\ncommand sent %x", rStatus);
    }

    if (cmd) {
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    }

    return i4BytesWritten;

set_pta_invalid:
    if (cmd) {
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    }
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten,
        i4TotalLen - i4BytesWritten,
        "\nformat:pta_config set [enable 1|0][txdata val][rxdataack val]");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n\t[rxdata val][txack val][txbmc val][txbcn val][rxbcn val]");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n\t[txmgmt val][rxmgmtack val][prottag val]");
    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten,
                               "\n\t[statenable 1|0][statreset 1]");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n\t[comm_act_bt_wf0_inband val][comm_act_bt_wf1_inband val]");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n\t[comm_act_bt_wf0_outband val][comm_act_bt_wf1_outband val]");
    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten,
                               "\n [enable val]: enable PTA(1) or not(0)");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [txtag val<0~15>]: priority tag for tx ac0-ac3");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [rxdataack val<0~15>]: priority tag for rx ac0-ac3 ack");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [rxdata val<0~15>]: priority tag for rx data");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [txack val<0~15>]: priority tag for tx ack");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [txbmc val<0~15>]: priority tag for tx bmc packet");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [txbcn val<0~15>]: priority tag for tx beacon");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [rxbcn val<0~15>]: priority tag for rx beacon");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [prottag val<0~15>]: priority tag for Protection frame");
    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten,
                               "\nformat(%d):pta_config get", i4Argc);
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten,
        i4TotalLen - i4BytesWritten,
        "\n [comm_act_bt_wf0_inband val<0~15>]: bt&wf0 inband common action");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten,
        i4TotalLen - i4BytesWritten,
        "\n [comm_act_bt_wf1_inband val<0~15>]: bt&wf1 inband common action");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten,
        i4TotalLen - i4BytesWritten,
        "\n [comm_act_bt_wf0_outband val<0~15>]: bt&wf0 outband common action");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten,
        i4TotalLen - i4BytesWritten,
        "\n [comm_act_bt_wf1_outband val<0~15>]: bt&wf1 outband common action");
    return i4BytesWritten;
}

#ifdef CFG_SUPPORT_EXT_PTA_DEBUG_COMMAND
static int priv_driver_ext_pta_config(IN struct net_device *prNetDev,
                                      IN char *pcCommand, IN int i4TotalLen)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    P_GLUE_INFO_T prGlueInfo;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    P_CMD_EXT_PTA_CONFIG_T cmd = NULL;
    s32 i4BytesWritten = 0;
    u32 u4Val = 0;
    s32 u4Ret = 0;
    s32 i = 0;
    u32 u4BufLen = 0;
    u32 u4TotalReqCnt = 0;

    DBGLOG(REQ, LOUD, "%s(%s)>\n", __func__, pcCommand);

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    if (i4Argc < 2) {
        goto set_ext_pta_invalid;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        goto set_ext_pta_invalid;
    }

    cmd = (P_CMD_EXT_PTA_CONFIG_T)kalMemAlloc(sizeof(*cmd), VIR_MEM_TYPE);
    if (!cmd) {
        goto set_ext_pta_invalid;
    }

    memset(cmd, 0, sizeof(*cmd));
    cmd->u2Type = CMD_EXT_PTA_CONFIG_TYPE;
    cmd->u2Len = sizeof(*cmd);

    if ((strnicmp(apcArgv[1], "SET", strlen("SET")) == 0) &&
        (i4Argc >= 4) && !(i4Argc & 1)) {
        cmd->u2Type |= CMD_ADV_CONTROL_SET;

        /* parsing parameters (data value) */
        for (i = 2; i < i4Argc; i += 2) {
            /* value field */
            u4Ret = kalkStrtou32(apcArgv[i + 1], 0, &u4Val);
            if (u4Ret) {
                i4BytesWritten += snprintf(
                    pcCommand + i4BytesWritten,
                    i4TotalLen - i4BytesWritten,
                    "\nparsing err(%d) %s %s", u4Ret,
                    apcArgv[i], apcArgv[i + 1]);
                goto set_ext_pta_invalid;
            }

            DBGLOG(REQ, LOUD, "arg[%d] %s %s (0x%x)\n", i,
                   apcArgv[i], apcArgv[i + 1], u4Val);

            /* data field*/
            if (strnicmp(apcArgv[i], "ENABLE", strlen("ENABLE")) ==
                0) {
                cmd->u4ConfigMask |= CMD_EXT_PTA_CONFIG_EXT_PTA;
                cmd->u4ExtPtaConfig = u4Val;
            } else if (strnicmp(apcArgv[i], "LOTX",
                                strlen("LOTX")) == 0) {
                cmd->u4ConfigMask |=
                    CMD_EXT_PTA_CONFIG_LO_TX_TAG;
                cmd->u4ZbLoTxTag = u4Val;
            } else if (strnicmp(apcArgv[i], "HITX",
                                strlen("HITX")) == 0) {
                cmd->u4ConfigMask |=
                    CMD_EXT_PTA_CONFIG_HI_TX_TAG;
                cmd->u4ZbHiTxTag = u4Val;
            } else if (strnicmp(apcArgv[i], "LORX",
                                strlen("LORX")) == 0) {
                cmd->u4ConfigMask |=
                    CMD_EXT_PTA_CONFIG_LO_RX_TAG;
                cmd->u4ZbLoRxTag = u4Val;
            } else if (strnicmp(apcArgv[i], "HIRX",
                                strlen("HIRX")) == 0) {
                cmd->u4ConfigMask |=
                    CMD_EXT_PTA_CONFIG_HI_RX_TAG;
                cmd->u4ZbHiRxTag = u4Val;
            } else if (strnicmp(apcArgv[i], "COMM_ACT_ZB_BT_UNSAFE",
                                strlen("COMM_ACT_ZB_BT_UNSAFE")) ==
                       0) {
                cmd->u4ConfigMask |=
                    CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_BT_UNSAFE;
                cmd->u4CommActZbBtUnsafe = u4Val;
            } else if (strnicmp(apcArgv[i], "COMM_ACT_ZB_BT_HSF",
                                strlen("COMM_ACT_ZB_BT_HSF")) ==
                       0) {
                cmd->u4ConfigMask |=
                    CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_BT_HSF;
                cmd->u4CommActZbBtHsf = u4Val;
            } else if (strnicmp(apcArgv[i],
                                "COMM_ACT_ZB_WF0_UNSAFE",
                                strlen("COMM_ACT_ZB_WF0_UNSAFE")) ==
                       0) {
                cmd->u4ConfigMask |=
                    CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_WF0_UNSAFE;
                cmd->u4CommActZbWf0Unsafe = u4Val;
            } else if (strnicmp(apcArgv[i], "COMM_ACT_ZB_WF0_HSF",
                                strlen("COMM_ACT_ZB_WF0_HSF")) ==
                       0) {
                cmd->u4ConfigMask |=
                    CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_WF0_HSF;
                cmd->u4CommActZbWf0Hsf = u4Val;
            } else if (strnicmp(apcArgv[i],
                                "COMM_ACT_ZB_WF1_UNSAFE",
                                strlen("COMM_ACT_ZB_WF1_UNSAFE")) ==
                       0) {
                cmd->u4ConfigMask |=
                    CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_WF1_UNSAFE;
                cmd->u4CommActZbWf1Unsafe = u4Val;
            } else if (strnicmp(apcArgv[i], "COMM_ACT_ZB_WF1_HSF",
                                strlen("COMM_ACT_ZB_WF1_HSF")) ==
                       0) {
                cmd->u4ConfigMask |=
                    CMD_EXT_PTA_CONFIG_COMM_ACT_ZB_WF1_HSF;
                cmd->u4CommActZbWf1Hsf = u4Val;
            } else {
                i4BytesWritten +=
                    snprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten,
                             "\nunknown parameter %s %s",
                             apcArgv[i], apcArgv[i + 1]);
                goto set_ext_pta_invalid;
            }
        }

        /* send to FW */
        rStatus = wlanSendSetQueryCmd(prGlueInfo->prAdapter,
                                      CMD_ID_ADV_CONTROL, true, false,
                                      false, NULL, NULL, sizeof(*cmd),
                                      (u8 *)cmd, NULL, 0);
    } else if (strnicmp(apcArgv[1], "GET", strlen("GET")) == 0) {
        rStatus = kalIoctl(prGlueInfo, wlanoidAdvCtrl, cmd,
                           sizeof(*cmd), true, true, true, &u4BufLen);
    } else {
        goto set_ext_pta_invalid;
    }

    if ((rStatus != WLAN_STATUS_SUCCESS) &&
        (rStatus != WLAN_STATUS_PENDING)) {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\ncommand failed %x", rStatus);
    } else if (!(cmd->u2Type & CMD_ADV_CONTROL_SET)) {
        u4TotalReqCnt = cmd->u4ZbLoTxReqCnt + cmd->u4ZbHiTxReqCnt +
                        cmd->u4ZbLoRxReqCnt + cmd->u4ZbHiRxReqCnt;

        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nPriority stat:");
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n ZB Lo TX : %d",
                                   cmd->u4ZbLoTxTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n ZB Hi TX : %d",
                                   cmd->u4ZbHiTxTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n ZB Lo RX : %d",
                                   cmd->u4ZbLoRxTag);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n ZB Hi RX : %d",
                                   cmd->u4ZbHiRxTag);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nCommon action stat (1:allow, 0:not allow):");
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n Unsafe arb mode");
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n ZB_TX vs BT_RX[b0]: %d",
            (cmd->u4CommActZbBtUnsafe & BIT(0)) >> 0);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n ZB_RX vs BT_TX[b1]: %d",
            (cmd->u4CommActZbBtUnsafe & BIT(1)) >> 1);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n ZB_RX vs BT_RX[b2]: %d",
            (cmd->u4CommActZbBtUnsafe & BIT(2)) >> 2);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n ZB_TX vs BT_TX[b3]: %d\n",
            (cmd->u4CommActZbBtUnsafe & BIT(3)) >> 3);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF0_TX vs ZB_RX[b0]: %d",
            (cmd->u4CommActZbWf0Unsafe & BIT(0)) >> 0);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF0_RX vs ZB_TX[b1]: %d",
            (cmd->u4CommActZbWf0Unsafe & BIT(1)) >> 1);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF0_RX vs ZB_RX[b2]: %d",
            (cmd->u4CommActZbWf0Unsafe & BIT(2)) >> 2);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF0_TX vs ZB_TX[b3]: %d\n",
            (cmd->u4CommActZbWf0Unsafe & BIT(3)) >> 3);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF1_TX vs ZB_RX[b0]: %d",
            (cmd->u4CommActZbWf1Unsafe & BIT(0)) >> 0);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF1_RX vs ZB_TX[b1]: %d",
            (cmd->u4CommActZbWf1Unsafe & BIT(1)) >> 1);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF1_RX vs ZB_RX[b2]: %d",
            (cmd->u4CommActZbWf1Unsafe & BIT(2)) >> 2);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF1_TX vs ZB_TX[b3]: %d\n",
            (cmd->u4CommActZbWf1Unsafe & BIT(3)) >> 3);
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\n half-safe arb mode");
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n ZB_TX vs BT_RX[b0]: %d",
            (cmd->u4CommActZbBtHsf & BIT(0)) >> 0);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n ZB_RX vs BT_TX[b1]: %d",
            (cmd->u4CommActZbBtHsf & BIT(1)) >> 1);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n ZB_RX vs BT_RX[b2]: %d",
            (cmd->u4CommActZbBtHsf & BIT(2)) >> 2);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n ZB_TX vs BT_TX[b3]: %d\n",
            (cmd->u4CommActZbBtHsf & BIT(3)) >> 3);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF0_TX vs ZB_RX[b0]: %d",
            (cmd->u4CommActZbWf0Hsf & BIT(0)) >> 0);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF0_RX vs ZB_TX[b1]: %d",
            (cmd->u4CommActZbWf0Hsf & BIT(1)) >> 1);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF0_RX vs ZB_RX[b2]: %d",
            (cmd->u4CommActZbWf0Hsf & BIT(2)) >> 2);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF0_TX vs ZB_TX[b3]: %d\n",
            (cmd->u4CommActZbWf0Hsf & BIT(3)) >> 3);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF1_TX vs ZB_RX[b0]: %d",
            (cmd->u4CommActZbWf1Hsf & BIT(0)) >> 0);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF1_RX vs ZB_TX[b1]: %d",
            (cmd->u4CommActZbWf1Hsf & BIT(1)) >> 1);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF1_RX vs ZB_RX[b2]: %d",
            (cmd->u4CommActZbWf1Hsf & BIT(2)) >> 2);
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n WF1_TX vs ZB_TX[b3]: %d",
            (cmd->u4CommActZbWf1Hsf & BIT(3)) >> 3);

        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nCounter stat:");
        if (cmd->u4ConfigMask & CMD_EXT_PTA_CONFIG_EXT_PTA) {
            i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "\n Grant cnt: %d",
                                       cmd->u4ZbGntCnt);
            i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "\n Abort cnt: %d",
                                       cmd->u4ZbAbtCnt);
            i4BytesWritten +=
                snprintf(pcCommand + i4BytesWritten,
                         i4TotalLen - i4BytesWritten,
                         "\n Denied cnt: %d",
                         u4TotalReqCnt - cmd->u4ZbGntCnt);
            i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "\n Lo tx req cnt: %d",
                                       cmd->u4ZbLoTxReqCnt);
            i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "\n Hi tx req cnt: %d",
                                       cmd->u4ZbHiTxReqCnt);
            i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "\n Lo rx req cnt: %d",
                                       cmd->u4ZbLoRxReqCnt);
            i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "\n Hi rx req cnt: %d",
                                       cmd->u4ZbHiRxReqCnt);
        } else {
            i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten,
                                       "\n Don't support");
        }
    } else {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\ncommand sent %x", rStatus);
    }

    if (cmd) {
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    }

    return i4BytesWritten;

set_ext_pta_invalid:
    if (cmd) {
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    }
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\nformat:ext_pta_config set [enable 1|0][lotx val][hitx val]");
    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten,
                               "\n\t[lorx val][hirx val]");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n\t[comm_act_zb_bt_unsafe val][comm_act_zb_bt_hsf val]");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n\t[comm_act_zb_wf0_unsafe val][comm_act_zb_wf0_hsf val]");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n\t[comm_act_zb_wf1_unsafe val][comm_act_zb_wf1_hsf val]");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [enable val]: enable EXT PTA(1) or not(0)");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [lotx val<0~15>]: priority tag for lo tx");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [hitx val<0~15>]: priority tag for hi tx");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [lorx val<0~15>]: priority tag for lo rx");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [hirx val<0~15>]: priority tag for lo tx");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten,
        i4TotalLen - i4BytesWritten,
        "\n [comm_act_zb_bt_unsafe val<0~15>]: zb&bt unsafe common action");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [comm_act_zb_bt_hsf val<0~15>]: zb&bt hsf common action");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten,
        i4TotalLen - i4BytesWritten,
        "\n [comm_act_zb_wf0_unsafe val<0~15>]: zb&wf0 unsafe common action");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [comm_act_zb_wf0_hsf val<0~15>]: zb&wf0 hsf common action");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten,
        i4TotalLen - i4BytesWritten,
        "\n [comm_act_zb_wf1_unsafe val<0~15>]: zb&wf1 unsafe common action");
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\n [comm_act_zb_wf1_hsf val<0~15>]: zb&wf1 hsf common action");
    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten,
                               "\nformat(%d):pta_config get", i4Argc);
    return i4BytesWritten;
}
#endif
static int priv_driver_set_pop(IN struct net_device *prNetDev,
                               IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0;
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_POP_ID;
    u32 u4Sel = 0, u4CckTh = 0, u4OfdmTh = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    rSwCtrlInfo.u4Id = u4Id;

    if (i4Argc <= 3) {
        DBGLOG(REQ, ERROR,
               "Argc(%d) ERR: SET_POP <Sel> <CCK TH> <OFDM TH>\n",
               i4Argc);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);
    if (u4Ret) {
        DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[2], 0, &u4CckTh);
    if (u4Ret) {
        DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[3], 0, &u4OfdmTh);
    if (u4Ret) {
        DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);
        return -1;
    }

    rSwCtrlInfo.u4Data = (u4CckTh | (u4OfdmTh << 8) | (u4Sel << 30));
    DBGLOG(REQ, LOUD, "u4Sel=%d u4CckTh=%d u4OfdmTh=%d, u4Data=0x%x,\n",
           u4Sel, u4CckTh, u4OfdmTh, rSwCtrlInfo.u4Data);
    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

static int priv_driver_get_pop(IN struct net_device *prNetDev,
                               IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_POP_ID;
    u32 u4Offset = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    u32 u4CckTh = 0, u4OfdmTh = 0;

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    u4CckTh = rSwCtrlInfo.u4Data & 0xFF;
    u4OfdmTh = (rSwCtrlInfo.u4Data >> 8) & 0xFF;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "PoP: CckTh:%ddB OfdmTh:%ddB\n", u4CckTh,
                         u4OfdmTh);

    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}

static int priv_driver_set_ed(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 i4Ret = 0;
    s32 i4EdVal[2] = { 0 };
    u32 u4Sel = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    if (prAdapter == NULL) {
        DBGLOG(REQ, LOUD, "Adapter is NULL!\n");
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc <= 3) {
        DBGLOG(REQ,
               ERROR,
               "Argc(%d) ERR: SET_ED <Sel> <2.4G EDCCA(-49~-81dBm)> <5G EDCCA(-49~-81dBm)>\n",
               i4Argc);
        return -1;
    }

    i4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse  error i4Ret=%d\n", i4Ret);
        return -1;
    }

    i4Ret = kalkStrtos32(apcArgv[2], 0, &i4EdVal[0]);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse i4EdVal(2.4G) error i4Ret=%d\n",
               i4Ret);
    }
    i4Ret = kalkStrtos32(apcArgv[3], 0, &i4EdVal[1]);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse i4EdVal(5G) error u4Ret=%d\n", i4Ret);
    }

    rStatus = wlanSetEd(prAdapter, i4EdVal[0], i4EdVal[1], u4Sel);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

static int priv_driver_get_ed(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_ED_ID;
    u32 u4Offset = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    s8 iEdVal[2] = { 0 };

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    iEdVal[0] = rSwCtrlInfo.u4Data & 0xFF;
    iEdVal[1] = (rSwCtrlInfo.u4Data >> 16) & 0xFF;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "ED: 2.4G(%ddB), 5G(%ddB)\n", iEdVal[0],
                         iEdVal[1]);

    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}

static int priv_driver_set_pd(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0;
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_PD_ID;
    u32 u4Sel = 0;
    s32 u4CckTh = 0, u4OfdmTh = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    rSwCtrlInfo.u4Id = u4Id;

    if (i4Argc <= 1) {
        DBGLOG(REQ, ERROR,
               "Argc(%d) ERR: SET_PD <Sel> [CCK TH] [OFDM TH]\n",
               i4Argc);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);
    if (u4Ret) {
        DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);
        return -1;
    }

    if (u4Sel == 1) {
        if (i4Argc <= 3) {
            DBGLOG(REQ, ERROR,
                   "Argc(%d) ERR: SET_PD 1 <CCK TH> <OFDM CH>\n",
                   i4Argc);
            return -1;
        }
        u4Ret = kalkStrtos32(apcArgv[2], 0, &u4CckTh);
        if (u4Ret) {
            DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n",
                   u4Ret);
        }
        u4Ret = kalkStrtos32(apcArgv[3], 0, &u4OfdmTh);
        if (u4Ret) {
            DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n",
                   u4Ret);
        }
    }

    rSwCtrlInfo.u4Data = ((u4OfdmTh & 0xFFFF) | ((u4CckTh & 0xFF) << 16) |
                          (u4Sel << 30));
    DBGLOG(REQ, LOUD, "u4Sel=%d u4OfdmTh=%d, u4CckTh=%d, u4Data=0x%x,\n",
           u4Sel, u4OfdmTh, u4CckTh, rSwCtrlInfo.u4Data);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

static int priv_driver_get_pd(IN struct net_device *prNetDev,
                              IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_PD_ID;
    u32 u4Offset = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    s8 u4CckTh = 0, u4OfdmTh = 0;

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    u4CckTh = rSwCtrlInfo.u4Data & 0xFF;
    u4OfdmTh = (rSwCtrlInfo.u4Data >> 8) & 0xFF;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "PD: CckTh:%ddB OfdmTh:%ddB\n", u4CckTh, u4OfdmTh);

    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}

static int priv_cmd_not_support(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    DBGLOG(REQ, WARN, "not support priv command: %s\n", pcCommand);

    return -EOPNOTSUPP;
}

static int priv_driver_set_maxrfgain(IN struct net_device *prNetDev,
                                     IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0;
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_MAX_RFGAIN_ID;
    u32 u4Sel = 0;
    s32 u4Wf0Gain = 0, u4Wf1Gain = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    rSwCtrlInfo.u4Id = u4Id;

    if (i4Argc <= 1) {
        DBGLOG(REQ, ERROR,
               "Argc(%d) ERR: SET_RFGAIN <Sel> <WF0 Gain> <WF1 Gain>\n",
               i4Argc);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);
    if (u4Ret) {
        DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);
        return -1;
    }

    if (u4Sel == 1) {
        if (i4Argc <= 3) {
            DBGLOG(REQ,
                   ERROR,
                   "Argc(%d) ERR: SET_RFGAIN 1 <WF0 Gain> <WF1 Gain>\n",
                   i4Argc);
            return -1;
        }
        u4Ret = kalkStrtos32(apcArgv[2], 0, &u4Wf0Gain);
        if (u4Ret) {
            DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n",
                   u4Ret);
        }
        u4Ret = kalkStrtos32(apcArgv[3], 0, &u4Wf1Gain);
        if (u4Ret) {
            DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n",
                   u4Ret);
        }
    }

    rSwCtrlInfo.u4Data = ((u4Wf0Gain & 0xFF) | ((u4Wf1Gain & 0xFF) << 8) |
                          (u4Sel << 31));
    DBGLOG(REQ, LOUD, "u4Sel=%d u4Wf0Gain=%d, u4Wf1Gain=%d, u4Data=0x%x,\n",
           u4Sel, u4Wf0Gain, u4Wf1Gain, rSwCtrlInfo.u4Data);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

static int priv_driver_get_maxrfgain(IN struct net_device *prNetDev,
                                     IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_MAX_RFGAIN_ID;
    u32 u4Offset = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    u8 u4Wf0Gain = 0, u4Wf1Gain = 0;

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    u4Wf0Gain = rSwCtrlInfo.u4Data & 0xFF;
    u4Wf1Gain = (rSwCtrlInfo.u4Data >> 8) & 0xFF;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "Max RFGain: WF0:%ddB WF1:%ddB\n", u4Wf0Gain,
                         u4Wf1Gain);

    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}

static int priv_driver_noise_histogram(IN struct net_device *prNetDev,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    P_GLUE_INFO_T prGlueInfo;
    s32 i4BytesWritten = 0;
    u32 u4BufLen = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    struct CMD_NOISE_HISTOGRAM_REPORT *cmd = NULL;

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        goto noise_histogram_invalid;
    }

    cmd = (struct CMD_NOISE_HISTOGRAM_REPORT *)kalMemAlloc(sizeof(*cmd),
                                                           VIR_MEM_TYPE);
    if (!cmd) {
        goto noise_histogram_invalid;
    }

    if ((i4Argc > 4) || (i4Argc < 2)) {
        goto noise_histogram_invalid;
    }

    memset(cmd, 0, sizeof(*cmd));

    cmd->u2Type = CMD_NOISE_HISTOGRAM_TYPE;
    cmd->u2Len = sizeof(*cmd);

    if (strnicmp(apcArgv[1], "ENABLE", strlen("ENABLE")) == 0) {
        /* NoiseHistogram Enable cmd is blocked when entering suspend
         * mode */
        if (prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap &
            BLOCK_KEEP_FULL_PWR) {
            DBGLOG(REQ, STATE,
                   "NoiseHistogram Enable Command is blocked\n");
            goto noise_histogram_invalid;
        }
        prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap |=
            KEEP_FULL_PWR_NOISE_HISTOGRAM_BIT;
        cmd->ucAction = CMD_NOISE_HISTOGRAM_ENABLE;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
    } else if (strnicmp(apcArgv[1], "DISABLE", strlen("DISABLE")) == 0) {
        prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap &=
            ~KEEP_FULL_PWR_NOISE_HISTOGRAM_BIT;
        cmd->ucAction = CMD_NOISE_HISTOGRAM_DISABLE;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
    } else if (strnicmp(apcArgv[1], "RESET", strlen("RESET")) == 0) {
        cmd->ucAction = CMD_NOISE_HISTOGRAM_RESET;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
#if CFG_IPI_2CHAIN_SUPPORT
    } else if (strnicmp(apcArgv[1], "GET2", strlen("GET2")) == 0) {
        /*
         * For getting backward compatibility
         * to original format for default chain
         */
        cmd->u2Type = CMD_NOISE_HISTOGRAM_TYPE2;
        cmd->ucAction = CMD_NOISE_HISTOGRAM_GET;
        rStatus = kalIoctl(prGlueInfo, wlanoidAdvCtrl, cmd,
                           sizeof(*cmd), true, true, true, &u4BufLen);
        if (rStatus == WLAN_STATUS_SUCCESS) {
            parseNoiseHistogramReport(&i4BytesWritten, pcCommand,
                                      &i4TotalLen, cmd);
        }

        cmd->ucAction = CMD_NOISE_HISTOGRAM_GET2;
#endif
    } else if (strnicmp(apcArgv[1], "GET", strlen("GET")) == 0) {
        cmd->ucAction = CMD_NOISE_HISTOGRAM_GET;
    } else {
        goto noise_histogram_invalid;
    }

    DBGLOG(REQ, LOUD, "%s(%s) action %x\n", __func__, pcCommand,
           cmd->ucAction);

    rStatus = kalIoctl(prGlueInfo, wlanoidAdvCtrl, cmd, sizeof(*cmd), true,
                       true, true, &u4BufLen);

    if (rStatus == WLAN_STATUS_NOT_SUPPORTED) {
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nThis fw version does not support priv command: %s\n",
            pcCommand);
    } else if ((rStatus != WLAN_STATUS_SUCCESS) &&
               (rStatus != WLAN_STATUS_PENDING)) {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\ncommand failed %x", rStatus);
    } else if (cmd->ucAction == CMD_NOISE_HISTOGRAM_GET
#if CFG_IPI_2CHAIN_SUPPORT
               || cmd->ucAction == CMD_NOISE_HISTOGRAM_GET2
#endif
               ) {
        parseNoiseHistogramReport(&i4BytesWritten, pcCommand,
                                  &i4TotalLen, cmd);
    } else {
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\ncommand sent %x", rStatus);
    }

    if (cmd) {
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    }

    return i4BytesWritten;

noise_histogram_invalid:
    if (cmd) {
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    }
    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\nformat:get_report [enable|disable|get|reset]");
    return i4BytesWritten;
}

static int priv_driver_set_adm_ctrl(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0;
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_ADM_CTRL_ID;
    u32 u4Enable = 0, u4TimeRatio = 100, u4AdmBase = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    rSwCtrlInfo.u4Id = u4Id;

    if ((i4Argc > 4) || (i4Argc < 2)) {
        DBGLOG(REQ, ERROR, "Argc(%d) ERR: set_admission_ctrl\n",
               i4Argc);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Enable);
    if (u4Ret) {
        DBGLOG(REQ, ERROR, "parse u4Enable error u4Ret=%d\n", u4Ret);
        return -1;
    }

    if (i4Argc > 2) {
        /* u4AdmTime default is 100% */
        u4Ret = kalkStrtos32(apcArgv[2], 0, &u4TimeRatio);
        if (u4Ret) {
            DBGLOG(REQ, ERROR, "parse u4AdmTime error u4Ret=%d\n",
                   u4Ret);
        }
    }

    if (i4Argc > 3) {
        u4Ret = kalkStrtos32(apcArgv[3], 0, &u4AdmBase);
        if (u4Ret) {
            DBGLOG(REQ, ERROR, "parse u4AdmBase error u4Ret=%d\n",
                   u4Ret);
        }
    }

    rSwCtrlInfo.u4Data =
        ((u4AdmBase & 0xFFFF) | ((u4TimeRatio & 0xFF) << 16) |
         ((u4Enable & 0xFF) << 24));
    DBGLOG(REQ, LOUD,
           "u4Enable=%d u4AdmTime=%d, u4AdmBase=%d, u4Data=0x%x,\n",
           u4Enable, u4TimeRatio, u4AdmBase, rSwCtrlInfo.u4Data);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

#if CFG_ENABLE_PS_INTV_CTRL
static int priv_driver_set_act_intv(IN struct net_device *prNetDev,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 i4Ret = 0;
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_ACT_INTV_ID;
    u16 ucSwitchInterval, ucMeasureInterval;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    rSwCtrlInfo.u4Id = u4Id;

    if (i4Argc != 3) {
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nformat:set_act_intv <SwitchIntv> <MeasureIntv>");
        return i4BytesWritten;
    }

    i4Ret = kalkStrtou16(apcArgv[1], 0, &ucSwitchInterval);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse BSSIndex error i4Ret=%d\n", i4Ret);
    }

    i4Ret = kalkStrtou16(apcArgv[2], 0, &ucMeasureInterval);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse SwitchInterval error i4Ret=%d\n",
               i4Ret);
    }

    if (ucSwitchInterval < 10 || ucSwitchInterval > 65535) {
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n<SwitchInterval> range is 10 to 65535");
        return i4BytesWritten;
    }
    if (ucMeasureInterval < 10 || ucMeasureInterval > 65535) {
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\n<MeasureInterval> range is 10 to 65535");
        return i4BytesWritten;
    }

    rSwCtrlInfo.u4Data = (ucSwitchInterval | (ucMeasureInterval << 16));
    DBGLOG(REQ, LOUD, "SwitchIntv=%d, MeasureIntv=%d u4Data=0x%x,\n",
           ucSwitchInterval, ucMeasureInterval, rSwCtrlInfo.u4Data);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}
#endif

#if CFG_ENABLE_1RPD_MMPS_CTRL
static int priv_driver_set_1rpd(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0;
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_1RPD;
    u32 u4Enable = false;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Id = u4Id;

    if (i4Argc <= 1) {
        DBGLOG(REQ, ERROR, "Argc(%d) ERR: SET_1RPD <Sel>\n", i4Argc);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Enable);

    if (u4Ret) {
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nCommand failed, invalid parameter");
        return i4BytesWritten;
    }

    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten, "\nSet 1RPD %d",
                               u4Enable);

    rSwCtrlInfo.u4Data = u4Enable;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

static int priv_driver_get_1rpd(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_1RPD;
    u32 u4Sel = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    u4Sel = rSwCtrlInfo.u4Data & 0xFF;

    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten,
                               "\n1RPD status: %d", u4Sel);

    return i4BytesWritten;
}
static int priv_driver_set_mmps(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0;
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_MMPS;
    u32 u4Enable = false;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Id = u4Id;

    if (i4Argc <= 1) {
        DBGLOG(REQ, ERROR, "Argc(%d) ERR: SET_NOISE <Sel>\n", i4Argc);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Enable);

    if (u4Ret) {
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nCommand failed, invalid parameter");
        return i4BytesWritten;
    }

    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten, "\nSet MMPS %d",
                               u4Enable);

    rSwCtrlInfo.u4Data = u4Enable;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

static int priv_driver_get_mmps(IN struct net_device *prNetDev,
                                IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_MMPS;
    u32 u4Sel = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    u4Sel = rSwCtrlInfo.u4Data & 0xFF;

    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten,
                               "\nMMPS status: %d", u4Sel);

    return i4BytesWritten;
}
#endif
#if CFG_ENABLE_DEWEIGHTING_CTRL
static int priv_driver_set_deweighting_th(IN struct net_device *prNetDev,
                                          IN char *pcCommand,
                                          IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0;
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_DEWEIGHTING_TH_ID;
    u32 u4Sel = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Id = u4Id;

    if (i4Argc <= 1) {
        DBGLOG(REQ, ERROR, "Argc(%d) ERR: SET_NOISE <Sel>\n", i4Argc);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);

    if (u4Ret) {
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nCommand failed, threshold range is 0 to 15");
        return i4BytesWritten;
    }

    if ((u4Sel < 0) || (u4Sel > 15)) {
        i4BytesWritten += snprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nCommand failed, threshold range is 0 to 15");
        return i4BytesWritten;
    }

    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten,
                               "\nSetting noise difference threshold %d",
                               u4Sel);

    rSwCtrlInfo.u4Data = u4Sel << 16;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

static int priv_driver_get_deweighting_th(IN struct net_device *prNetDev,
                                          IN char *pcCommand,
                                          IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_DEWEIGHTING_TH_ID;
    u32 u4Sel = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    u4Sel = (rSwCtrlInfo.u4Data >> 16) & 0xFF;

    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten,
                               "\nNoise difference threshold is %d", u4Sel);

    return i4BytesWritten;
}

static int priv_driver_get_deweighting_noise(IN struct net_device *prNetDev,
                                             IN char *pcCommand,
                                             IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id =
        CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_DEWEIGHTING_NOISE_ID;
    s8 u2Wf0AvgNoise = 0, u2Wf1AvgNoise = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    u2Wf1AvgNoise = rSwCtrlInfo.u4Data & 0xFF;
    u2Wf0AvgNoise = (rSwCtrlInfo.u4Data >> 16) & 0xFF;

    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\nDeweighting Average noise, WF0 = %d, WF1 = %d",
        u2Wf0AvgNoise, u2Wf1AvgNoise);

    return i4BytesWritten;
}

static int priv_driver_get_deweighting_weight(IN struct net_device *prNetDev,
                                              IN char *pcCommand,
                                              IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id =
        CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_DEWEIGHTING_WEIGHT_ID;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    u8 ucWF0OfdmSetpt, ucWF0CckSetpt, ucWF1OfdmSetpt, ucWF1CckSetpt;

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    ucWF1CckSetpt = rSwCtrlInfo.u4Data & 0xFF;
    ucWF1OfdmSetpt = (rSwCtrlInfo.u4Data >> 8) & 0xFF;
    ucWF0CckSetpt = (rSwCtrlInfo.u4Data >> 16) & 0xFF;
    ucWF0OfdmSetpt = (rSwCtrlInfo.u4Data >> 24) & 0xFF;

    i4BytesWritten += snprintf(
        pcCommand + i4BytesWritten,
        i4TotalLen - i4BytesWritten,
        "\nDeweighting weight, WF0 ofdm=%02X cck=%02X, WF1 ofdm=%02X cck=%02X",
        ucWF0OfdmSetpt,
        ucWF0CckSetpt,
        ucWF1OfdmSetpt,
        ucWF1CckSetpt);

    return i4BytesWritten;
}
#endif
#if CFG_RX_SINGLE_CHAIN_SUPPORT
static int priv_driver_set_rxchain(IN struct net_device *prNetDev,
                                   IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 u4Ret = 0;
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_RXC_ID;
    u32 u4Sel = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    P_ADAPTER_T prAdapter = NULL;
    u32 u4Offset = 0;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    rSwCtrlInfo.u4Id = u4Id;

    if (i4Argc <= 1) {
        u4Offset += snprintf(
            pcCommand + u4Offset, i4TotalLen - u4Offset,
            "SET RX CHAIN: Fail!(%d) SET_RXC <Sel: 0/1/2>\n",
            i4Argc);
        goto out;
    }

    /* not connected */
    /* if (prGlueInfo->eParamMediaStateIndicated !=
     * PARAM_MEDIA_STATE_CONNECTED) {
     * u4Offset += snprintf(pcCommand + u4Offset,
     * i4TotalLen - u4Offset,
     * "SET RX CHAIN: Fail! not connected\n");
     * goto out;
     * }
     */

    /* if (prAdapter->rWifiVar.ucNSS > 1 ||
     * prAdapter->rWifiVar.ucSpeIdxCtrl == 2) {
     * u4Offset += snprintf(pcCommand + u4Offset,
     * i4TotalLen - u4Offset,
     * "SET RX CHAIN: Fail!NSS =%d &
     * SpeIdxCtrl =%d wrong setting\n",
     * prAdapter->rWifiVar.ucNSS,
     * prAdapter->rWifiVar.ucSpeIdxCtrl);
     * goto out;
     * }
     */

    u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);
    if (u4Ret) {
        DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);
        return -1;
    }
    if (u4Sel < 0 || u4Sel > 2) {
        u4Offset += snprintf(pcCommand + u4Offset,
                             i4TotalLen - u4Offset,
                             "SET RX CHAIN:Fail!(%d) not equal 0/1/2\n",
                             u4Sel);
        goto out;
    }
    /* else if (u4Sel != prAdapter->rWifiVar.ucSpeIdxCtrl) {
     * u4Offset += snprintf(pcCommand + u4Offset,
     * i4TotalLen - u4Offset,
     * "SET RX CHAIN:Fail!(%d)
     * not equal to SpeIdxCtrl(%d)\n",
     * u4Sel, prAdapter->rWifiVar.ucSpeIdxCtrl);
     * goto out;
     * }
     */

    rSwCtrlInfo.u4Data = u4Sel;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        u4Offset +=
            snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                     "SET RX CHAIN: kalIoctl fail (%d)\n", rStatus);

        goto out;
    } else {
        u4Offset += snprintf(
            pcCommand + u4Offset, i4TotalLen - u4Offset,
            "SET RX CHAIN: %d (0: WF0 1:WF1 2:WF0+WF1)Success\n",
            u4Sel);
    }
out:
    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}

static int priv_driver_get_rxchain(IN struct net_device *prNetDev,
                                   IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_RXC_ID;
    u32 u4Offset = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    u8 ucRxChain;
    P_ADAPTER_T prAdapter = NULL;

    ASSERT(prNetDev);
    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    /*not connected*/
    /*if (prGlueInfo->eParamMediaStateIndicated !=
     * PARAM_MEDIA_STATE_CONNECTED) {
     * u4Offset += snprintf(pcCommand + u4Offset,
     * i4TotalLen - u4Offset,
     * "GET RX CHAIN: Fail! not yet connected\n");
     * goto out;
     * }
     */

    /*if (prAdapter->rWifiVar.ucNSS > 1 ||
     * prAdapter->rWifiVar.ucSpeIdxCtrl == 2) {
     * u4Offset += snprintf(pcCommand + u4Offset,
     * i4TotalLen - u4Offset,
     * "GET RX CHAIN: Fail! NSS=%d (Need <2)
     * SpeIdxCtrl=%d (Need <2) \n",
     * prAdapter->rWifiVar.ucNSS,
     * prAdapter->rWifiVar.ucSpeIdxCtrl);
     * goto out;
     * }
     */
    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    ucRxChain = (rSwCtrlInfo.u4Data) & 0xFF;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "GET RX CHAIN: %d (0: WF0 1:WF1 2:WF0+WF1)\n",
                         ucRxChain);

    /*out:*/
    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}
#endif

static int priv_driver_set_bcn_th(IN struct net_device *prNetDev,
                                  IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 i4Ret = 0;
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_BCN_TH_ID;
    u8 ucAdhoc, ucInfra, ucWithbt;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    rSwCtrlInfo.u4Id = u4Id;

    if (i4Argc != 4) {
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
            "\nformat:set_bcn_th <adhoc> <infra> <withbt>");
        return i4BytesWritten;
    }

    i4Ret = kalkStrtou8(apcArgv[1], 0, &ucAdhoc);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error i4Ret=%d\n", i4Ret);
    }
    i4Ret = kalkStrtou8(apcArgv[2], 0, &ucInfra);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error i4Ret=%d\n", i4Ret);
    }
    i4Ret = kalkStrtou8(apcArgv[3], 0, &ucWithbt);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error i4Ret=%d\n", i4Ret);
    }

    rSwCtrlInfo.u4Data = (ucAdhoc | (ucInfra << 8) | (ucWithbt << 16));
    DBGLOG(REQ, LOUD, "u4Withbt=%d u4Infra=%d, u4Withbt=%d u4Data=0x%x,\n",
           ucAdhoc, ucInfra, ucWithbt, rSwCtrlInfo.u4Data);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

static int priv_driver_get_bcn_th(IN struct net_device *prNetDev,
                                  IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_BCN_TH_ID;
    u8 ucAdhoc, ucInfra, ucWithbt;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc != 1) {
        i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                    i4TotalLen - i4BytesWritten,
                                    "\nformat:get_bcn_th");
        return i4BytesWritten;
    }

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    ucAdhoc = rSwCtrlInfo.u4Data & 0xFF;
    ucInfra = (rSwCtrlInfo.u4Data >> 8) & 0xFF;
    ucWithbt = (rSwCtrlInfo.u4Data >> 16) & 0xFF;

    i4BytesWritten += scnprintf(
        pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
        "\nBeacon loss threshold: ADHOC:%d, INFRA:%d, With BT:%d\n",
        ucAdhoc, ucInfra, ucWithbt);

    return i4BytesWritten;
}

static int priv_driver_get_bcntimeout_num(IN struct net_device *prNetDev,
                                          IN char *pcCommand,
                                          IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_BCNTIMOUT_NUM_ID;
    u32 u4Offset = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    u32 u4BcnTimeoutNum = 0;

    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    if (!prGlueInfo->prAdapter->prAisBssInfo) {
        return -EFAULT;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex;
    rSwCtrlInfo.u4Id = u4Id;
    DBGLOG(REQ, LOUD, "BssIndex=%d\n", rSwCtrlInfo.u4Data);

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    u4BcnTimeoutNum = rSwCtrlInfo.u4Data;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "Beacon Timeout Number: %d\n", u4BcnTimeoutNum);

    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}

static int priv_driver_set_every_tbtt(IN struct net_device *prNetDev,
                                      IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_EVERY_TBTT_ID;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    s32 i4Ret = 0;
    u32 u4EveryTbtt[2] = { 0 };

    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    if (prAdapter == NULL) {
        DBGLOG(REQ, LOUD, "Adapter is NULL!\n");
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc <= 2) {
        DBGLOG(REQ,
               ERROR,
               "Argc(%d) ERR: SET_EVERY_TBTT <2G sel:0/1> <5G sel: 0/1>\n",
               i4Argc);
        return -1;
    }

    i4Ret = kalkStrtou32(apcArgv[1], 0, &u4EveryTbtt[0]);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse  error i4Ret=%d\n", i4Ret);
        return -1;
    }

    i4Ret = kalkStrtou32(apcArgv[2], 0, &u4EveryTbtt[1]);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse  error i4Ret=%d\n", i4Ret);
        return -1;
    }

    if ((u4EveryTbtt[0] != 0 && u4EveryTbtt[0] != 1) ||
        (u4EveryTbtt[1] != 0 && u4EveryTbtt[1] != 1)) {
        DBGLOG(REQ,
               ERROR,
               "ERR val=%d,%d: SET_EVERY_TBTT <2G sel:0/1> <5G sel: 0/1>\n",
               u4EveryTbtt[0],
               u4EveryTbtt[1]);
        return -1;
    }

    rSwCtrlInfo.u4Id = u4Id;
    rSwCtrlInfo.u4Data = ((u4EveryTbtt[0] & 0xFF) << 16) |
                         (u4EveryTbtt[1] & 0xFF);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), false, false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

static int priv_driver_get_every_tbtt(IN struct net_device *prNetDev,
                                      IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u32 u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + CMD_ADVCTL_EVERY_TBTT_ID;
    u32 u4Offset = 0;
    PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;
    u8 bEveryTbtt[2] = { 0 };

    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), true, true, true, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    bEveryTbtt[0] = (rSwCtrlInfo.u4Data >> 16) & 0xFF;
    bEveryTbtt[1] = rSwCtrlInfo.u4Data & 0xFF;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "Get every TBTT settings: 2G[%d], 5G[%d]\n",
                         bEveryTbtt[0], bEveryTbtt[1]);

    i4BytesWritten = (s32)u4Offset;

    return i4BytesWritten;
}
#endif

static int priv_driver_set_csi(IN struct net_device *prNetDev,
                               IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    struct CMD_CSI_CONTROL_T *prCSICtrl = NULL;
    u32 u4Ret = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    DBGLOG(RSN, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    DBGLOG(RSN, INFO, "priv_driver_csi_control\n");

    prCSICtrl = (struct CMD_CSI_CONTROL_T *)kalMemAlloc(
        sizeof(struct CMD_CSI_CONTROL_T), VIR_MEM_TYPE);
    if (!prCSICtrl) {
        DBGLOG(REQ, ERROR, "allocate memory for prCSICtrl failed\n");
        i4BytesWritten = -1;
        goto out;
    }
    kalMemZero(prCSICtrl, sizeof(struct CMD_CSI_CONTROL_T));

    if (i4Argc != 2 && i4Argc != 5) {
        DBGLOG(REQ, ERROR, "argc %i is invalid\n", i4Argc);
        i4BytesWritten = -1;
        goto out;
    }

    u4Ret = kalkStrtou8(apcArgv[1], 0, &(prCSICtrl->ucMode));
    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse ucMode error u4Ret=%d\n", u4Ret);
        goto out;
    }

    if (prCSICtrl->ucMode > 1) {
        DBGLOG(REQ, LOUD, "Invalid ucMode %d, should be 0 or 1\n",
               prCSICtrl->ucMode);
    }

    if (prCSICtrl->ucMode == CSI_CONTROL_MODE_STOP) {
        goto send_cmd;
    }

    prCSICtrl->ucBand = ENUM_BAND_0;
    prCSICtrl->ucRole = RTT_ROLE_SENDING;

    u4Ret = kalkStrtou8(apcArgv[2], 0, &(prCSICtrl->ucWf));
    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse ucWf error u4Ret=%d\n", u4Ret);
        goto out;
    }

    if (prCSICtrl->ucWf > 1) {
        DBGLOG(REQ, LOUD, "Invalid ucWf %u, should be 0 or 1\n",
               prCSICtrl->ucWf);
        goto out;
    }

    u4Ret = kalkStrtou8(apcArgv[3], 0, &(prCSICtrl->ucFrameTypeIndex));
    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse ucFrameTypeIndex error u4Ret=%d\n",
               u4Ret);
        goto out;
    }

    if (prCSICtrl->ucFrameTypeIndex > 3) {
        DBGLOG(REQ, LOUD,
               "Invalid ucFrameTypeIndex %u, should be 0 ~ 3\n",
               prCSICtrl->ucFrameTypeIndex);
        goto out;
    }

    u4Ret = kalkStrtou8(apcArgv[4], 0, &(prCSICtrl->ucFrameType));
    if (u4Ret) {
        DBGLOG(REQ, LOUD, "parse ucFrameType error u4Ret=%d\n", u4Ret);
        goto out;
    }

    if (prCSICtrl->ucFrameType != 0) {
        DBGLOG(REQ, LOUD,
               "Invalid ucFrameType %u, only support 0(beacon)\n",
               prCSICtrl->ucFrameType);
        goto out;
    }

send_cmd:
    rStatus = kalIoctl(prGlueInfo, wlanoidSetCSIControl, prCSICtrl,
                       sizeof(struct CMD_CSI_CONTROL_T), true, true, true,
                       &u4BufLen);

    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);
    DBGLOG(REQ,
           INFO,
           "mode %d, band %d, role %d, frame type idx %d, frame type idx %d",
           prCSICtrl->ucMode,
           prCSICtrl->ucBand,
           prCSICtrl->ucRole,
           prCSICtrl->ucFrameType,
           prCSICtrl->ucFrameTypeIndex);
    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "send CSI control cmd failed\n");
        i4BytesWritten = -1;
    }

out:
    if (prCSICtrl) {
        kalMemFree(prCSICtrl, VIR_MEM_TYPE,
                   sizeof(struct CMD_CSI_CONTROL_T));
    }

    return i4BytesWritten;
}

static int priv_driver_get_csi(IN struct net_device *prNetDev,
                               IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4BytesWritten = 0;
    P_ADAPTER_T prAdapter = NULL;
    u8 ucBw = 20;
    u16 i = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u8 ucDataType = 0;
    s16 *pri2Data = NULL;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    prAdapter = prGlueInfo->prAdapter;

    if (prAdapter == NULL) {
        DBGLOG(REQ, LOUD, "Adapter is NULL!\n");
        return -1;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc != 2 || kalkStrtou8(apcArgv[1], 0, &ucDataType) ||
        ucDataType > 1) {
        LOGBUF(pcCommand,
               i4TotalLen,
               i4BytesWritten,
               "0 or 1 should be given as the parameter to output I or Q data!\n");
        goto out;
    }

    if (prAdapter->rCsiData.u2DataCount == 0) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "No CSI Data\n");
        goto out;
    }

    if (prAdapter->rCsiData.ucBw == 0) {
        ucBw = 20;
    }else if (prAdapter->rCsiData.ucBw == 1) {
        ucBw = 40;
    }else if (prAdapter->rCsiData.ucBw == 2) {
        ucBw = 80;
    }

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\nCSI Data:\n");

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "BW: %d\n", ucBw);
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "isCck: %d\n",
           prAdapter->rCsiData.bIsCck);
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "data count: %d\n",
           prAdapter->rCsiData.u2DataCount);

    if (ucDataType == 0) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "I data:\n");
        pri2Data = prAdapter->rCsiData.ac2IData;
        prAdapter->rCsiData.ucDataOutputted |= BIT(0);
    } else {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "Q data:\n");
        pri2Data = prAdapter->rCsiData.ac2QData;
        prAdapter->rCsiData.ucDataOutputted |= BIT(1);
    }

    for (i = 0; i < prAdapter->rCsiData.u2DataCount; i++) {
        if ((i % 16) == 0) {
            LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\n");
        }

        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "%d ",
               pri2Data[i]);
    }

    if (prAdapter->rCsiData.ucDataOutputted == 0x3) {
        prAdapter->rCsiData.u2DataCount = 0;
        /* clean this indate I/Q data indicates that I & Q data are all
         * outputted, we can start to receive the new IQ data
         */
        prAdapter->rCsiData.ucDataOutputted = 0;
    }

out:
    return i4BytesWritten;
}

#ifdef CFG_SUPPORT_ANT_DIV
static int priv_driver_ant_diversity_config(IN struct net_device *prNetDev,
                                            IN char *pcCommand,
                                            IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX];
    u32 u4Ret;
    u8 fgRead = false;
    u8 fgWaitResp = false;
    struct CMD_ANT_DIV_CTRL rAntDivInfo;

    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -EFAULT;
    }

    DBGLOG(REQ, INFO, "priv_driver_ant_diversity_config()\n");
    kalMemZero(&rAntDivInfo, sizeof(struct CMD_ANT_DIV_CTRL));

    rAntDivInfo.ucAction = 0;
    rAntDivInfo.ucAntId = 0;
    rAntDivInfo.ucRcpi = 0;
    rAntDivInfo.ucState = 0;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc == 0) {
        DBGLOG(REQ, INFO, "ANT_DIV Argc = %d\n", i4Argc);
        return -EFAULT;
    }
    if (strnicmp(apcArgv[0], CMD_GET_ANT_DIV, strlen(CMD_GET_ANT_DIV)) ==
        0) {
        if (i4Argc < CMD_GET_ANT_DIV_ARG_NUM) {
            DBGLOG(REQ, INFO,
                   "GET_ANT_DIV arg num=%d, must be %d\n", i4Argc,
                   CMD_GET_ANT_DIV_ARG_NUM);
            return -EFAULT;
        }
        rAntDivInfo.ucAction = ANT_DIV_CMD_GET_ANT;
        fgRead = true;
        fgWaitResp = true;
    } else if (strnicmp(apcArgv[0], CMD_DETC_ANT_DIV,
                        strlen(CMD_DETC_ANT_DIV)) == 0) {
        if (i4Argc < CMD_DETC_ANT_DIV_ARG_NUM) {
            DBGLOG(REQ, INFO,
                   "DECT_ANT_DIV arg num=%d, must be %d\n", i4Argc,
                   CMD_DETC_ANT_DIV_ARG_NUM);
            return -EFAULT;
        }

        rAntDivInfo.ucAction = ANT_DIV_CMD_DETC;
        fgRead = true;
        fgWaitResp = true;
    } else if (strnicmp(apcArgv[0], CMD_SWH_ANT_DIV,
                        strlen(CMD_SWH_ANT_DIV)) == 0) {
        if (i4Argc < CMD_SWH_ANT_DIV_ARG_NUM) {
            DBGLOG(REQ, INFO,
                   "SWH_ANT_DIV arg num=%d, must be %d\n", i4Argc,
                   CMD_SWH_ANT_DIV_ARG_NUM);
            return -EFAULT;
        }

        rAntDivInfo.ucAction = ANT_DIV_CMD_SWH;
        fgRead = true;
        fgWaitResp = true;
    } else if (strnicmp(apcArgv[0], CMD_SET_ANT_DIV,
                        strlen(CMD_SET_ANT_DIV)) == 0) {
        if (i4Argc < CMD_SET_ANT_DIV_ARG_NUM) {
            DBGLOG(REQ, INFO,
                   "SET_ANT_DIV arg num=%d, must be %d\n", i4Argc,
                   CMD_SET_ANT_DIV_ARG_NUM);
            return -EFAULT;
        }

        rAntDivInfo.ucAction = ANT_DIV_CMD_SET_ANT;
        fgRead = false;
        fgWaitResp = false;
        u4Ret = kalkStrtou8(apcArgv[1], 0, &(rAntDivInfo.ucAntId));
        if (u4Ret || ((rAntDivInfo.ucAntId != 1) &&
                      (rAntDivInfo.ucAntId != 2))) {
            DBGLOG(REQ, INFO,
                   "Parse ANT Index error Ret=%d, AntId =%d\n",
                   u4Ret, rAntDivInfo.ucAntId);
            return -EFAULT;
        }
    } else {
        DBGLOG(REQ, INFO, "ANT DIV subcmd(%s) error !\n", apcArgv[0]);
        return -EFAULT;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidAntDivCfg, &rAntDivInfo,
                       sizeof(rAntDivInfo), fgRead, fgWaitResp, true,
                       &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        return -1;
    }

    if (fgWaitResp) {
        DBGLOG(REQ, LOUD,
               "[ANT-DIV] priv_driver_ant_diversity_config\n");
        DBGLOG(REQ, LOUD, "[ANT-DIV] action=%d\n",
               rAntDivInfo.ucAction);
        DBGLOG(REQ, LOUD, "[ANT-DIV] ucState=%d\n",
               rAntDivInfo.ucState);

        switch (rAntDivInfo.ucAction) {
        case ANT_DIV_CMD_GET_ANT:
            i4BytesWritten = scnprintf(pcCommand, i4TotalLen, "%d",
                                       rAntDivInfo.ucState);
            break;

        case ANT_DIV_CMD_DETC:
            if (rAntDivInfo.ucState == ANT_DIV_SUCCESS) {
                /* rcpi to rssi */
                DBGLOG(REQ, LOUD,
                       "[ANT-DIV] ucRcpi=%d rssi=%d\n",
                       rAntDivInfo.ucRcpi,
                       RCPI_TO_dBm(rAntDivInfo.ucRcpi));
                i4BytesWritten = scnprintf(
                    pcCommand, i4TotalLen, "%d",
                    RCPI_TO_dBm(rAntDivInfo.ucRcpi));
            } else {
                i4BytesWritten = scnprintf(pcCommand,
                                           i4TotalLen, "%d",
                                           rAntDivInfo.ucState);
            }
            break;

        case ANT_DIV_CMD_SWH:
            i4BytesWritten = scnprintf(pcCommand, i4TotalLen, "%d",
                                       rAntDivInfo.ucState);
            break;

        default:
            DBGLOG(REQ, ERROR, "[ANT-DIV][WARN] ucAction=%d\n",
                   rAntDivInfo.ucAction);
            i4BytesWritten =
                scnprintf(pcCommand, i4TotalLen, "fail");
            break;
        }

        DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
               pcCommand);
    }

    return i4BytesWritten;
}
#endif

static int priv_driver_enforce_power_mode(IN struct net_device *prNetDev,
                                          IN char *pcCommand,
                                          IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 i4Ret = 0;
    u16 u2EnforcePowerMode;
#ifdef SUPPORT_ENFORCE_PWR_MODE
    P_BSS_INFO_T prAisBssInfo;
#endif
    PARAM_POWER_MODE_T rPowerMode;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    if ((i4Argc != 2) && (i4Argc != 3)) {
        i4BytesWritten += scnprintf(
            pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
#ifdef SUPPORT_PERIODIC_PS
            "\nformat:enforce_power_mode [0|1|2|3|4] [band]"
#else
            "\nformat:enforce_power_mode [0|1|2|3]"
#endif
            );
        return i4BytesWritten;
    }

    i4Ret = kalkStrtou16(apcArgv[1], 0, &u2EnforcePowerMode);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse enforce_power_mode error i4Ret=%d\n",
               i4Ret);
    }

#ifdef SUPPORT_ENFORCE_PWR_MODE
    if (u2EnforcePowerMode > Param_PowerModeMax) {
        return -EINVAL;
    }

#endif

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        return -EFAULT;
    }

    if (!prGlueInfo->prAdapter->prAisBssInfo) {
        return -EFAULT;
    }

    rPowerMode.ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex;
    rPowerMode.ePowerMode = (PARAM_POWER_MODE)u2EnforcePowerMode;
#ifdef SUPPORT_ENFORCE_PWR_MODE
    prAisBssInfo = prGlueInfo->prAdapter->prAisBssInfo;

    if (i4Argc == 3) {
        u16 band;
        i4Ret = kalkStrtou16(apcArgv[2], 0, &band);
        if (i4Ret) {
            DBGLOG(REQ, ERROR, "parse band error i4Ret=%d\n",
                   i4Ret);
            return -EFAULT;
        }

        if (band == BAND_2G4) {
            DBGLOG(REQ, EVENT, "enforce power mode 2G %d\n",
                   u2EnforcePowerMode);
            prGlueInfo->prAdapter->rWifiVar.ucEnforce2G =
                u2EnforcePowerMode;
        } else if (band == BAND_5G) {
            DBGLOG(REQ, EVENT, "enforce power mode 5G %d\n",
                   u2EnforcePowerMode);
            prGlueInfo->prAdapter->rWifiVar.ucEnforce5G =
                u2EnforcePowerMode;
        } else {
            DBGLOG(REQ, ERROR, "wrong band\n");
            return -EINVAL;
        }

        if (prAisBssInfo->eBand != band) {
            DBGLOG(REQ, ERROR,
                   "mismatched band, saved but without apply\n");
            return 0;
        }
    } else {  // for both 2.4G and 5G band
        prGlueInfo->prAdapter->rWifiVar.ucEnforce2G =
            u2EnforcePowerMode;
        prGlueInfo->prAdapter->rWifiVar.ucEnforce5G =
            u2EnforcePowerMode;
    }
#else
    prGlueInfo->prAdapter->rWifiVar.ucEnforcePSMode = rPowerMode.ePowerMode;
#endif

    // Restore Android's power mode setting if we do not enforce power mode.
    if (rPowerMode.ePowerMode >= Param_PowerModeMax &&
        prGlueInfo->prAdapter->prAisBssInfo->ePowerModeFromUser <
        Param_PowerModeMax) {
        rPowerMode.ePowerMode =
            prGlueInfo->prAdapter->prAisBssInfo->ePowerModeFromUser;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile,
                       &rPowerMode, sizeof(PARAM_POWER_MODE_T), false,
                       false, true, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}

static int priv_driver_get_power_mode(IN struct net_device *prNetDev,
                                      IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
#ifdef SUPPORT_ENFORCE_PWR_MODE
    P_BSS_INFO_T prBssInfo = NULL;
#endif

    s32 i4BytesWritten = 0;
    u16 u2EnforcePowerMode;
    u16 ePowerModeFromUser;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        return -EFAULT;
    }

    prAdapter = prGlueInfo->prAdapter;
    if (!prAdapter || !prAdapter->prAisBssInfo) {
        return -EFAULT;
    }

#ifdef SUPPORT_ENFORCE_PWR_MODE
    prBssInfo = prAdapter->prAisBssInfo;
    ePowerModeFromUser = prBssInfo->ePowerModeFromUser;
    if (prBssInfo->eBand == BAND_2G4) {
        u2EnforcePowerMode = prAdapter->rWifiVar.ucEnforce2G;
    } else if (prBssInfo->eBand == BAND_5G) {
        u2EnforcePowerMode = prAdapter->rWifiVar.ucEnforce5G;
    } else {
        DBGLOG(REQ, ERROR, "prAisBssInfo->eBand unknonwn:%d\n",
               prBssInfo->eBand);
        return -EFAULT;
    }
#else
    u2EnforcePowerMode = prAdapter->rWifiVar.ucEnforcePSMode;
    ePowerModeFromUser = prAdapter->prAisBssInfo->ePowerModeFromUser;
#endif
    if (u2EnforcePowerMode < Param_PowerModeMax) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
               "\n enforce power mode %d\n", u2EnforcePowerMode);
    } else {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
               "\n enforce power mode: disabled\n");
    }
    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           " system power mode: %d\n", ePowerModeFromUser);

    return i4BytesWritten;
}

static int priv_driver_send_beacon_timeout(IN struct net_device *prNetDev,
                                           IN char *pcCommand,
                                           IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));

    prAdapter = prGlueInfo->prAdapter;
    if (!prAdapter || !prAdapter->prAisBssInfo) {
        return -EFAULT;
    }

    /* A testing reason code for disconnect reason get. */
    prAdapter->prAisBssInfo->u2DeauthReason = 100;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (prGlueInfo->eParamMediaStateIndicated ==
        PARAM_MEDIA_STATE_CONNECTED) {
        kalIndicateStatusAndComplete(
            prGlueInfo, WLAN_STATUS_BEACON_TIMEOUT, NULL, 0);
    }

    return 0;
}

static int priv_driver_get_disconnect_reason(IN struct net_device *prNetDev,
                                             IN char *pcCommand,
                                             IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    s32 i4BytesWritten = 0, temp;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };

    ASSERT(prNetDev);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        return -EFAULT;
    }

    prAdapter = prGlueInfo->prAdapter;
    if (!prAdapter || !prAdapter->prAisBssInfo) {
        return -EFAULT;
    }

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    temp = prAdapter->prAisBssInfo->u2DeauthReason + DISCONNECT_REASON_BASE;
    memset(pcCommand, 0x00, i4TotalLen);
    memcpy(pcCommand, &temp, sizeof(s32));
    i4BytesWritten += sizeof(s32);
    DBGLOG(REQ, INFO, "i4BytesWritten=%d ucBcnTimeoutReason=0x%x\n",
           i4BytesWritten, *(s32 *)pcCommand);
    return i4BytesWritten;
}

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
static int priv_driver_bss_transition_query(IN struct net_device *prNetDev,
                                            IN char *pcCommand,
                                            IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    u8 *pucQueryReason = NULL;

    if (strnicmp(pcCommand, "BSS-TRANSITION-QUERY", 20) == 0) {
        if (strnicmp(pcCommand + 20, " reason=", 8) == 0) {
            pucQueryReason = pcCommand + 28;
            DBGLOG(REQ, INFO,
                   "BSS-TRANSITION-QUERY, pucQueryReason=%s\n",
                   pucQueryReason);
        } else {
            DBGLOG(REQ,
                   ERROR,
                   "Incorrect format, please specify reason code after reason=\n");
            return -EFAULT;
        }
    } else {
        DBGLOG(REQ, ERROR, "BSS-TRANSITION-QUERY command format error");
        return -EFAULT;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        return -EFAULT;
    }

    if (!prGlueInfo->prAdapter->prAisBssInfo) {
        return -EFAULT;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSendBTMQuery, pucQueryReason,
                       sizeof(pucQueryReason), false, false, true,
                       &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;
}
#endif

static int priv_driver_get_1xtx_status(IN struct net_device *prNetDev,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;

    s32 i4BytesWritten = 0;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        return -EFAULT;
    }

    prAdapter = prGlueInfo->prAdapter;
    if (!prAdapter || !prAdapter->prAisBssInfo) {
        return -EFAULT;
    }

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "%d\n",
           prAdapter->r1xTxDoneStatus);

    return i4BytesWritten;
}

static int priv_driver_test_1xtx_status(IN struct net_device *prNetDev,
                                        IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4BytesWritten = 0;
    s32 i4Argc = 0;
    s8 *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    s32 i4Ret = 0;
    u8 ucTest1xTxStatus;
    P_ADAPTER_T prAdapter = NULL;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    DBGLOG(REQ, ERROR, "i4Argc is %d\n", i4Argc);
    if (i4Argc != 2) {
        i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                    i4TotalLen - i4BytesWritten,
                                    "\nformat:test_1xtx_status [0|1]");
        return i4BytesWritten;
    }

    i4Ret = kalkStrtou8(apcArgv[1], 0, &ucTest1xTxStatus);
    if (i4Ret) {
        DBGLOG(REQ, ERROR, "parse test_1xtx_status error i4Ret=%d\n",
               i4Ret);
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        return -EFAULT;
    }

    prAdapter = prGlueInfo->prAdapter;
    if (!prAdapter || !prAdapter->prAisBssInfo) {
        return -EFAULT;
    }

    prAdapter->fgIsTest1xTx = ucTest1xTxStatus;
    DBGLOG(REQ, STATE, "set fgIsTest1xTx %d\n", prAdapter->fgIsTest1xTx);

    return i4BytesWritten;
}

#if CFG_SUPPORT_802_11K

static int priv_driver_neighbor_request(IN struct net_device *prNetDev,
                                        IN char *pcCommand, IN int i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    s32 i4BytesWritten = 0;
    u8 *pucSSID = NULL;

    if (strnicmp(pcCommand, "NEIGHBOR-REQUEST", 16) == 0) {
        if (strnicmp(pcCommand + 16, " SSID=", 6) == 0) {
            pucSSID = pcCommand + 22;
            DBGLOG(REQ, INFO, "NEIGHBOR-REQUEST, ssid=%s\n",
                   pucSSID);
        } else {
            DBGLOG(REQ,
                   ERROR,
                   "Incorrect format, please specify ssid after ssid=\n");
            return -EFAULT;
        }
    } else {
        DBGLOG(REQ, ERROR, "NEIGHBOR-REQUEST command format error\n");
        return -EFAULT;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        return -EFAULT;
    }

    if (!prGlueInfo->prAdapter->prAisBssInfo) {
        return -EFAULT;
    }

    if (pucSSID == NULL) {
        rStatus = kalIoctl(prGlueInfo, wlanoidSendNeighborRequest, NULL,
                           0, false, false, true, &u4BufLen);
    } else {
        rStatus = kalIoctl(prGlueInfo, wlanoidSendNeighborRequest,
                           pucSSID, kalStrLen(pucSSID), false, false,
                           true, &u4BufLen);
    }

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }
    return i4BytesWritten;
}
#endif

s32 priv_driver_cmds(IN struct net_device *prNetDev, IN s8 *pcCommand,
                     IN s32 i4TotalLen)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    s32 i4BytesWritten = 0;

    if (GLUE_CHK_PR2(prNetDev, pcCommand) == false) {
        return -1;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (strnicmp(pcCommand, CMD_RSSI, strlen(CMD_RSSI)) == 0) {
        /* i4BytesWritten =
         *  wl_android_get_rssi(net, command, i4TotalLen);
         */
    } else if (strnicmp(pcCommand, CMD_AP_START, strlen(CMD_AP_START)) ==
               0) {
        i4BytesWritten = priv_driver_set_ap_start(prNetDev, pcCommand,
                                                  i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_LINKSPEED, strlen(CMD_LINKSPEED)) ==
               0) {
        i4BytesWritten = priv_driver_get_linkspeed(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_PNOSSIDCLR_SET,
                        strlen(CMD_PNOSSIDCLR_SET)) == 0) {
        /* ToDo:: Nothing */
    } else if (strnicmp(pcCommand, CMD_PNOSETUP_SET,
                        strlen(CMD_PNOSETUP_SET)) == 0) {
        /* ToDo:: Nothing */
    } else if (strnicmp(pcCommand, CMD_RXFILTER_START,
                        strlen(CMD_RXFILTER_START)) == 0) {
        /* ToDo:: Nothing */
    } else if (strnicmp(pcCommand, CMD_RXFILTER_STOP,
                        strlen(CMD_RXFILTER_STOP)) == 0) {
    } else if (strnicmp(pcCommand, CMD_SET_AP_WPS_P2P_IE,
                        strlen(CMD_SET_AP_WPS_P2P_IE)) == 0) {
        /* ToDo:: Nothing */
    } else if (strnicmp(pcCommand, CMD_RXFILTER_ADD,
                        strlen(CMD_RXFILTER_ADD)) == 0) {
        /* ToDo:: Nothing */
    } else if (strnicmp(pcCommand, CMD_PNOENABLE_SET,
                        strlen(CMD_PNOENABLE_SET)) == 0) {
        /* ToDo:: Nothing */
    } else if (strnicmp(pcCommand, CMD_BTCOEXSCAN_STOP,
                        strlen(CMD_BTCOEXSCAN_STOP)) == 0) {
        /* ToDo:: Nothing */
    } else if (strnicmp(pcCommand, CMD_SETSUSPENDOPT,
                        strlen(CMD_SETSUSPENDOPT)) == 0) {
        /* i4BytesWritten = wl_android_set_suspendopt(net, pcCommand,
         * i4TotalLen); */
    } else if (strnicmp(pcCommand, CMD_SETSUSPENDMODE,
                        strlen(CMD_SETSUSPENDMODE)) == 0) {
        i4BytesWritten = priv_driver_set_suspend_mode(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SETBAND, strlen(CMD_SETBAND)) == 0) {
        i4BytesWritten =
            priv_driver_set_band(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GETBAND, strlen(CMD_GETBAND)) == 0) {
        /* i4BytesWritten = wl_android_get_band(net, pcCommand,
         * i4TotalLen); */
    } else if (strnicmp(pcCommand, CMD_SET_TXPOWER,
                        strlen(CMD_SET_TXPOWER)) == 0) {
        i4BytesWritten = priv_driver_set_txpower(prNetDev, pcCommand,
                                                 i4TotalLen);
#if CFG_SUPPORT_DBDC_TC6
    } else if (strnicmp(pcCommand, CMD_CSA, strlen(CMD_CSA)) == 0) {
        i4BytesWritten =
            priv_driver_set_csa(prNetDev, pcCommand, i4TotalLen);
#endif
    } else if (strnicmp(pcCommand, CMD_COUNTRY, strlen(CMD_COUNTRY)) == 0) {
        i4BytesWritten = priv_driver_set_country(prNetDev, pcCommand,
                                                 i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_COUNTRY,
                        strlen(CMD_GET_COUNTRY)) == 0) {
        i4BytesWritten = priv_driver_get_country(prNetDev, pcCommand,
                                                 i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_CHANNELS,
                        strlen(CMD_GET_CHANNELS)) == 0) {
        i4BytesWritten = priv_driver_get_channels(prNetDev, pcCommand,
                                                  i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_AP_CHANNELS,
                        strlen(CMD_GET_AP_CHANNELS)) == 0) {
        i4BytesWritten = priv_driver_get_ap_channels(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_MIRACAST, strlen(CMD_MIRACAST)) ==
               0) {
        i4BytesWritten = priv_driver_set_miracast(prNetDev, pcCommand,
                                                  i4TotalLen);
    }
    /* Mediatek private command  */
    else if (strnicmp(pcCommand, CMD_SET_SW_CTRL,
                      strlen(CMD_SET_SW_CTRL)) == 0) {
        i4BytesWritten = priv_driver_set_sw_ctrl(prNetDev, pcCommand,
                                                 i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_FIXED_RATE,
                        strlen(CMD_SET_FIXED_RATE)) == 0) {
        i4BytesWritten = priv_driver_set_fixed_rate(prNetDev, pcCommand,
                                                    i4TotalLen);
    }
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT
    else if (strnicmp(pcCommand, CMD_SET_FIXED_MRATE,
                      strlen(CMD_SET_FIXED_MRATE)) == 0) {
        i4BytesWritten = priv_driver_set_fixed_mrate(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_DUP_MPACKET,
                        strlen(CMD_SET_DUP_MPACKET)) == 0) {
        i4BytesWritten = priv_driver_set_dup_mpacket(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_DUP_MRACKET,
                        strlen(CMD_SET_DUP_MPACKET)) == 0) {
        i4BytesWritten = priv_driver_get_dup_mpacket(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_MCAST_BURST,
                        strlen(CMD_SET_MCAST_BURST)) == 0) {
        i4BytesWritten = priv_driver_set_mcast_burst(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_MCAST_BURST,
                        strlen(CMD_GET_MCAST_BURST)) == 0) {
        i4BytesWritten = priv_driver_get_mcast_burst(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_TXOP_TIME,
                        strlen(CMD_SET_TXOP_TIME)) == 0) {
        i4BytesWritten =
            priv_driver_set_txop(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_TXOP_TIME,
                        strlen(CMD_GET_TXOP_TIME)) == 0) {
        i4BytesWritten =
            priv_driver_get_txop(prNetDev, pcCommand, i4TotalLen);
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
    } else if (strnicmp(pcCommand, CMD_SET_LOOK_BACK_MODE,
                        strlen(CMD_SET_LOOK_BACK_MODE)) == 0) {
        i4BytesWritten = priv_driver_set_look_back(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_LOOK_BACK_MODE,
                        strlen(CMD_GET_LOOK_BACK_MODE)) == 0) {
        i4BytesWritten = priv_driver_get_look_back(prNetDev, pcCommand,
                                                   i4TotalLen);
#endif
    } else if (strnicmp(pcCommand, CMD_SET_11MC_TYPE,
                        strlen(CMD_SET_11MC_TYPE)) == 0) {
        i4BytesWritten = priv_driver_set_11mc_type(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_11MC_TYPE,
                        strlen(CMD_SET_11MC_TYPE)) == 0) {
        i4BytesWritten = priv_driver_get_11mc_type(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_UNICAST_BURST_TIMEOUT,
                        strlen(CMD_SET_UNICAST_BURST_TIMEOUT)) == 0) {
        i4BytesWritten = priv_driver_set_unicast_burst_timeout(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_UNICAST_BURST_TIMEOUT,
                        strlen(CMD_GET_UNICAST_BURST_TIMEOUT)) == 0) {
        i4BytesWritten = priv_driver_get_unicast_burst_timeout(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_UNICAST_BURST,
                        strlen(CMD_SET_UNICAST_BURST)) == 0) {
        i4BytesWritten = priv_driver_set_unicast_burst(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_UNICAST_BURST,
                        strlen(CMD_GET_UNICAST_BURST)) == 0) {
        i4BytesWritten = priv_driver_get_unicast_burst(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_MRM_CLIENT,
                        strlen(CMD_SET_MRM_CLIENT)) == 0) {
        i4BytesWritten = priv_driver_set_mrm_clinet(prNetDev, pcCommand,
                                                    i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_MRM_CLIENT,
                        strlen(CMD_GET_MRM_CLIENT)) == 0) {
        i4BytesWritten = priv_driver_get_mrm_client(prNetDev, pcCommand,
                                                    i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_AUDIO_TOS,
                        strlen(CMD_SET_AUDIO_TOS)) == 0) {
        i4BytesWritten = priv_driver_set_audio_tos(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_AUDIO_TOS,
                        strlen(CMD_GET_AUDIO_TOS)) == 0) {
        i4BytesWritten = priv_driver_get_audio_tos(prNetDev, pcCommand,
                                                   i4TotalLen);
    }
#endif
    else if (strnicmp(pcCommand, CMD_GET_SW_CTRL,
                      strlen(CMD_GET_SW_CTRL)) == 0) {
        i4BytesWritten = priv_driver_get_sw_ctrl(prNetDev, pcCommand,
                                                 i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_MCR, strlen(CMD_SET_MCR)) == 0) {
        i4BytesWritten =
            priv_driver_set_mcr(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_MCR, strlen(CMD_GET_MCR)) == 0) {
        i4BytesWritten =
            priv_driver_get_mcr(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_DRV_MCR,
                        strlen(CMD_SET_DRV_MCR)) == 0) {
        i4BytesWritten = priv_driver_set_drv_mcr(prNetDev, pcCommand,
                                                 i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_DRV_MCR,
                        strlen(CMD_GET_DRV_MCR)) == 0) {
        i4BytesWritten = priv_driver_get_drv_mcr(prNetDev, pcCommand,
                                                 i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_TEST_MODE,
                        strlen(CMD_SET_TEST_MODE)) == 0) {
        i4BytesWritten = priv_driver_set_test_mode(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_TEST_CMD,
                        strlen(CMD_SET_TEST_CMD)) == 0) {
        i4BytesWritten = priv_driver_set_test_cmd(prNetDev, pcCommand,
                                                  i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_TEST_RESULT,
                        strlen(CMD_GET_TEST_RESULT)) == 0) {
        i4BytesWritten = priv_driver_get_test_result(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_STA_STAT2,
                        strlen(CMD_GET_STA_STAT2)) == 0) {
        i4BytesWritten = priv_driver_get_sta_stat2(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_STA_STAT,
                        strlen(CMD_GET_STA_STAT)) == 0) {
        i4BytesWritten = priv_driver_get_sta_stat(prNetDev, pcCommand,
                                                  i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_CURR_AR_RATE,
                        strlen(CMD_GET_CURR_AR_RATE)) == 0) {
        i4BytesWritten = priv_driver_get_sta_curr_ar_rate(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_STA_RX_STAT,
                        strlen(CMD_GET_STA_RX_STAT)) == 0) {
        i4BytesWritten = priv_driver_show_rx_stat(prNetDev, pcCommand,
                                                  i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_ACL_POLICY,
                        strlen(CMD_SET_ACL_POLICY)) == 0) {
        i4BytesWritten = priv_driver_set_acl_policy(prNetDev, pcCommand,
                                                    i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_ADD_ACL_ENTRY,
                        strlen(CMD_ADD_ACL_ENTRY)) == 0) {
        i4BytesWritten = priv_driver_add_acl_entry(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_DEL_ACL_ENTRY,
                        strlen(CMD_DEL_ACL_ENTRY)) == 0) {
        i4BytesWritten = priv_driver_del_acl_entry(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SHOW_ACL_ENTRY,
                        strlen(CMD_SHOW_ACL_ENTRY)) == 0) {
        i4BytesWritten = priv_driver_show_acl_entry(prNetDev, pcCommand,
                                                    i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_CLEAR_ACL_ENTRY,
                        strlen(CMD_CLEAR_ACL_ENTRY)) == 0) {
        i4BytesWritten = priv_driver_clear_acl_entry(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_COEX_CONTROL,
                        strlen(CMD_COEX_CONTROL)) == 0) {
        i4BytesWritten =
            priv_driver_coex_ctrl(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_CSI, strlen(CMD_SET_CSI)) == 0) {
        i4BytesWritten =
            priv_driver_set_csi(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_CSI, strlen(CMD_GET_CSI)) == 0) {
        i4BytesWritten =
            priv_driver_get_csi(prNetDev, pcCommand, i4TotalLen);
    }
#if (CFG_SUPPORT_DFS_MASTER == 1)
    else if (strnicmp(pcCommand, CMD_SHOW_DFS_STATE,
                      strlen(CMD_SHOW_DFS_STATE)) == 0) {
        i4BytesWritten = priv_driver_show_dfs_state(prNetDev, pcCommand,
                                                    i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SHOW_DFS_abd123_PARAM,
                        strlen(CMD_SHOW_DFS_abd123_PARAM)) == 0) {
        i4BytesWritten = priv_driver_show_dfs_abd123_param(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SHOW_DFS_HELP,
                        strlen(CMD_SHOW_DFS_HELP)) == 0) {
        i4BytesWritten = priv_driver_show_dfs_help(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SHOW_DFS_CAC_TIME,
                        strlen(CMD_SHOW_DFS_CAC_TIME)) == 0) {
        i4BytesWritten = priv_driver_show_dfs_cac_time(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_RDD_REPORT,
                        strlen(CMD_SET_RDD_REPORT)) == 0) {
        i4BytesWritten = priv_driver_set_rdd_report(prNetDev, pcCommand,
                                                    i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_CLEAN_DFS_abd123_PARAM,
                        strlen(CMD_CLEAN_DFS_abd123_PARAM)) == 0) {
        i4BytesWritten = priv_driver_clean_dfs_abd123_param(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_abd123_DETECT_MODE,
                        strlen(CMD_SET_abd123_DETECT_MODE)) == 0) {
        i4BytesWritten = priv_driver_set_abd123_detect_mode(
            prNetDev, pcCommand, i4TotalLen);
    }
#endif

#if CFG_WOW_SUPPORT
    else if (strnicmp(pcCommand, CMD_WOW_START, strlen(CMD_WOW_START)) ==
             0) {
        i4BytesWritten =
            priv_driver_set_wow(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_WOW_ENABLE,
                        strlen(CMD_SET_WOW_ENABLE)) == 0) {
        i4BytesWritten = priv_driver_set_wow_enable(prNetDev, pcCommand,
                                                    i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_WOW_PAR,
                        strlen(CMD_SET_WOW_PAR)) == 0) {
        i4BytesWritten = priv_driver_set_wow_par(prNetDev, pcCommand,
                                                 i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_WOW_UDP,
                        strlen(CMD_SET_WOW_UDP)) == 0) {
        i4BytesWritten = priv_driver_set_wow_udpport(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_WOW_TCP,
                        strlen(CMD_SET_WOW_TCP)) == 0) {
        i4BytesWritten = priv_driver_set_wow_tcpport(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_WOW_PORT,
                        strlen(CMD_GET_WOW_PORT)) == 0) {
        i4BytesWritten = priv_driver_get_wow_port(prNetDev, pcCommand,
                                                  i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_WOW_REASON,
                        strlen(CMD_GET_WOW_REASON)) == 0) {
        i4BytesWritten = priv_driver_get_wow_reason(prNetDev, pcCommand,
                                                    i4TotalLen);
    }
#endif
#if CFG_STR_DHCP_RENEW_OFFLOAD
    else if (strnicmp(pcCommand, CMD_SET_DHCP_INFO,
                      strlen(CMD_SET_DHCP_INFO)) == 0) {
        i4BytesWritten = priv_driver_set_dhcp_info(prNetDev, pcCommand,
                                                   i4TotalLen);
    }
#endif
    else if (strnicmp(pcCommand, CMD_SET_ADV_PWS,
                      strlen(CMD_SET_ADV_PWS)) == 0) {
        i4BytesWritten = priv_driver_set_adv_pws(prNetDev, pcCommand,
                                                 i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_MDTIM, strlen(CMD_SET_MDTIM)) ==
               0) {
        i4BytesWritten =
            priv_driver_set_mdtim(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_LISTEN_DTIM_INTERVAL,
                        strlen(CMD_SET_LISTEN_DTIM_INTERVAL)) == 0) {
        i4BytesWritten = priv_driver_set_listen_dtim_interval(
            prNetDev, pcCommand, i4TotalLen);
    }
#if CFG_SUPPORT_QA_TOOL
    else if (strnicmp(pcCommand, CMD_GET_RX_STATISTICS,
                      strlen(CMD_GET_RX_STATISTICS)) == 0) {
        i4BytesWritten = priv_driver_get_rx_statistics(
            prNetDev, pcCommand, i4TotalLen);
    }
#if CFG_SUPPORT_BUFFER_MODE
    else if (strnicmp(pcCommand, CMD_SETBUFMODE, strlen(CMD_SETBUFMODE)) ==
             0) {
        i4BytesWritten = priv_driver_set_efuse_buffer_mode(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SETEEPROM_MODE,
                        strlen(CMD_SETEEPROM_MODE)) == 0) {
        i4BytesWritten = priv_driver_set_eeprom_mode(
            prNetDev, pcCommand, i4TotalLen);
    }
#endif
#endif
#if CFG_SUPPORT_MSP
    else if (strnicmp(pcCommand, CMD_GET_STA_STATISTICS,
                      strlen(CMD_GET_STA_STATISTICS)) == 0) {
        i4BytesWritten = priv_driver_get_sta_statistics(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_BSS_STATISTICS,
                        strlen(CMD_GET_BSS_STATISTICS)) == 0) {
        i4BytesWritten = priv_driver_get_bss_statistics(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_STA_INFO,
                        strlen(CMD_GET_STA_INFO)) == 0) {
        i4BytesWritten = priv_driver_get_sta_info(prNetDev, pcCommand,
                                                  i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_WTBL_INFO,
                        strlen(CMD_GET_WTBL_INFO)) == 0) {
        i4BytesWritten = priv_driver_get_wtbl_info(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_MIB_INFO,
                        strlen(CMD_GET_MIB_INFO)) == 0) {
        i4BytesWritten = priv_driver_get_mib_info(prNetDev, pcCommand,
                                                  i4TotalLen);
    }
#if CFG_SUPPORT_LAST_SEC_MCS_INFO
    else if (strnicmp(pcCommand, CMD_GET_MCS_INFO,
                      strlen(CMD_GET_MCS_INFO)) == 0) {
        i4BytesWritten = priv_driver_get_mcs_info(prNetDev, pcCommand,
                                                  i4TotalLen);
    }
#endif
    else if (strnicmp(pcCommand, CMD_SET_FW_LOG, strlen(CMD_SET_FW_LOG)) ==
             0) {
        i4BytesWritten =
            priv_driver_set_fw_log(prNetDev, pcCommand, i4TotalLen);
    }
#endif
    else if (strnicmp(pcCommand, CMD_GET_MAGIC_PKT_INFO,
                      strlen(CMD_GET_MAGIC_PKT_INFO)) == 0) {
        i4BytesWritten = priv_driver_get_magic_pkt_info(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_CFG, strlen(CMD_SET_CFG)) == 0) {
        i4BytesWritten =
            priv_driver_set_cfg(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_CFG, strlen(CMD_GET_CFG)) == 0) {
        i4BytesWritten =
            priv_driver_get_cfg(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_CHIP, strlen(CMD_SET_CHIP)) ==
               0) {
        i4BytesWritten = priv_driver_set_chip_config(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_CHIP, strlen(CMD_GET_CHIP)) ==
               0) {
        i4BytesWritten = priv_driver_get_chip_config(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_VERSION,
                        strlen(CMD_GET_VERSION)) == 0) {
        i4BytesWritten = priv_driver_get_version(prNetDev, pcCommand,
                                                 i4TotalLen);
#if CFG_SUPPORT_DBDC
    } else if (strnicmp(pcCommand, CMD_SET_DBDC, strlen(CMD_SET_DBDC)) ==
               0) {
        i4BytesWritten =
            priv_driver_set_dbdc(prNetDev, pcCommand, i4TotalLen);
#endif
#if CFG_SUPPORT_BATCH_SCAN
    } else if (strnicmp(pcCommand, CMD_BATCH_SET, strlen(CMD_BATCH_SET)) ==
               0) {
        kalIoctl(prGlueInfo, wlanoidSetBatchScanReq, (void *)pcCommand,
                 i4TotalLen, false, false, true, &i4BytesWritten);
    } else if (strnicmp(pcCommand, CMD_BATCH_GET, strlen(CMD_BATCH_GET)) ==
               0) {
        /* strcpy(pcCommand, "BATCH SCAN DATA FROM FIRMWARE"); */
        /* i4BytesWritten = strlen("BATCH SCAN DATA FROM FIRMWARE") + 1;
         */
        /* i4BytesWritten = priv_driver_get_linkspeed (prNetDev,
         * pcCommand, i4TotalLen); */

        u32 u4BufLen;
        int i;
        /* int rlen=0; */

        for (i = 0; i < CFG_BATCH_MAX_MSCAN; i++) {
            g_rEventBatchResult[i].ucScanCount = i + 1;  /* for get
                                                         * which
                                                         * mscan */
            kalIoctl(prGlueInfo, wlanoidQueryBatchScanResult,
                     (void *)&g_rEventBatchResult[i],
                     sizeof(EVENT_BATCH_RESULT_T), true, true, true,
                     &u4BufLen);
        }

        batchConvertResult(&g_rEventBatchResult[0], pcCommand,
                           i4TotalLen, &i4BytesWritten);

        /* Dump for debug */
        /* print_hex_dump(KERN_INFO,
         *  "BATCH", DUMP_PREFIX_ADDRESS, 16, 1, pcCommand,
         * i4BytesWritten, true);
         */
    } else if (strnicmp(pcCommand, CMD_BATCH_STOP,
                        strlen(CMD_BATCH_STOP)) == 0) {
        kalIoctl(prGlueInfo, wlanoidSetBatchScanReq, (void *)pcCommand,
                 i4TotalLen, false, false, true, &i4BytesWritten);
#endif
    }
#if CFG_SUPPORT_SNIFFER
    else if (strnicmp(pcCommand, CMD_SETMONITOR, strlen(CMD_SETMONITOR)) ==
             0) {
        i4BytesWritten = priv_driver_set_monitor(prNetDev, pcCommand,
                                                 i4TotalLen);
    }
#endif
    else if (strnicmp(pcCommand, CMD_GET_QUE_INFO,
                      strlen(CMD_GET_QUE_INFO)) == 0) {
        i4BytesWritten = priv_driver_get_que_info(prNetDev, pcCommand,
                                                  i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_MEM_INFO,
                        strlen(CMD_GET_MEM_INFO)) == 0) {
        i4BytesWritten = priv_driver_get_mem_info(prNetDev, pcCommand,
                                                  i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_HIF_INFO,
                        strlen(CMD_GET_HIF_INFO)) == 0) {
        i4BytesWritten = priv_driver_get_hif_info(prNetDev, pcCommand,
                                                  i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_P2P_PS,
                        strlen(CMD_SET_P2P_PS)) == 0) {
        i4BytesWritten =
            priv_driver_set_p2p_ps(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_CNM_INFO,
                        strlen(CMD_GET_CNM_INFO)) == 0) {
        i4BytesWritten = priv_driver_get_cnm_info(prNetDev, pcCommand,
                                                  i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_DSLP_CNT,
                        strlen(CMD_GET_DSLP_CNT)) == 0) {
        i4BytesWritten = priv_driver_get_deep_sleep_cnt(
            prNetDev, pcCommand, i4TotalLen);
    }
#if CFG_AUTO_CHANNEL_SEL_SUPPORT
    else if (strnicmp(pcCommand, CMD_GET_CH_RANK_LIST,
                      strlen(CMD_GET_CH_RANK_LIST)) == 0) {
        i4BytesWritten = priv_driver_get_ch_rank_list(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_CH_DIRTINESS,
                        strlen(CMD_GET_CH_DIRTINESS)) == 0) {
        i4BytesWritten = priv_driver_get_ch_dirtiness(
            prNetDev, pcCommand, i4TotalLen);
    }
#endif
    else if (strnicmp(pcCommand, CMD_EFUSE, sizeof(CMD_EFUSE) - 1) == 0) {
        i4BytesWritten =
            priv_driver_efuse_ops(prNetDev, pcCommand, i4TotalLen);
    }
#if CFG_SUPPORT_ADVANCE_CONTROL
    else if (strnicmp(pcCommand, CMD_SET_NOISE, strlen(CMD_SET_NOISE)) ==
             0) {
        i4BytesWritten =
            priv_driver_set_noise(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_NOISE, strlen(CMD_GET_NOISE)) ==
               0) {
        i4BytesWritten =
            priv_driver_get_noise(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_PTA_CONFIG,
                        strlen(CMD_PTA_CONFIG)) == 0) {
        i4BytesWritten =
            priv_driver_pta_config(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_TRAFFIC_REPORT,
                        strlen(CMD_TRAFFIC_REPORT)) == 0) {
        i4BytesWritten = priv_driver_get_traffic_report(
            prNetDev, pcCommand, i4TotalLen);
    }
#ifdef CFG_SUPPORT_EXT_PTA_DEBUG_COMMAND
    else if (strnicmp(pcCommand, CMD_EXT_PTA_CONFIG,
                      strlen(CMD_EXT_PTA_CONFIG)) == 0) {
        i4BytesWritten = priv_driver_ext_pta_config(prNetDev, pcCommand,
                                                    i4TotalLen);
    }
#endif
#ifdef CFG_SUPPORT_ADMINCTRL
    else if (strnicmp(pcCommand, CMD_ADMINCTRL_CONFIG,
                      strlen(CMD_ADMINCTRL_CONFIG)) == 0) {
        i4BytesWritten = priv_driver_admin_ctrl_config(
            prNetDev, pcCommand, i4TotalLen);
    }
#endif
    else if (strnicmp(pcCommand, CMD_SET_POP, strlen(CMD_SET_POP)) == 0) {
        i4BytesWritten =
            priv_driver_set_pop(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_POP, strlen(CMD_GET_POP)) == 0) {
        i4BytesWritten =
            priv_driver_get_pop(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_ED, strlen(CMD_SET_ED)) == 0) {
        i4BytesWritten =
            priv_driver_set_ed(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_ED, strlen(CMD_GET_ED)) == 0) {
        i4BytesWritten =
            priv_driver_get_ed(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_PD, strlen(CMD_SET_PD)) == 0) {
        i4BytesWritten =
            priv_driver_set_pd(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_PD, strlen(CMD_GET_PD)) == 0) {
        i4BytesWritten =
            priv_driver_get_pd(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_MAX_RFGAIN,
                        strlen(CMD_SET_MAX_RFGAIN)) == 0) {
        i4BytesWritten = priv_driver_set_maxrfgain(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_MAX_RFGAIN,
                        strlen(CMD_GET_MAX_RFGAIN)) == 0) {
        i4BytesWritten = priv_driver_get_maxrfgain(prNetDev, pcCommand,
                                                   i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_NOISE_HISTOGRAM,
                        strlen(CMD_NOISE_HISTOGRAM)) == 0) {
        i4BytesWritten = priv_driver_noise_histogram(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_ADM_CTRL,
                        strlen(CMD_SET_ADM_CTRL)) == 0) {
        i4BytesWritten = priv_driver_set_adm_ctrl(prNetDev, pcCommand,
                                                  i4TotalLen);
    }
#if CFG_RX_SINGLE_CHAIN_SUPPORT
    else if (strnicmp(pcCommand, CMD_SET_RXC, strlen(CMD_SET_RXC)) == 0) {
        i4BytesWritten = priv_driver_set_rxchain(prNetDev, pcCommand,
                                                 i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_RXC, strlen(CMD_GET_RXC)) == 0) {
        i4BytesWritten = priv_driver_get_rxchain(prNetDev, pcCommand,
                                                 i4TotalLen);
    }
#endif
    else if (strnicmp(pcCommand, CMD_SET_BCN_TH, strlen(CMD_SET_BCN_TH)) ==
             0) {
        i4BytesWritten =
            priv_driver_set_bcn_th(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_BCN_TH,
                        strlen(CMD_GET_BCN_TH)) == 0) {
        i4BytesWritten =
            priv_driver_get_bcn_th(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_BCNTIMEOUT_NUM,
                        strlen(CMD_GET_BCNTIMEOUT_NUM)) == 0) {
        i4BytesWritten = priv_driver_get_bcntimeout_num(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_EVERY_TBTT,
                        strlen(CMD_SET_EVERY_TBTT)) == 0) {
        i4BytesWritten = priv_driver_set_every_tbtt(prNetDev, pcCommand,
                                                    i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_EVERY_TBTT,
                        strlen(CMD_GET_EVERY_TBTT)) == 0) {
        i4BytesWritten = priv_driver_get_every_tbtt(prNetDev, pcCommand,
                                                    i4TotalLen);
    }
#endif
#if CFG_ENABLE_PS_INTV_CTRL
    else if (strnicmp(pcCommand, CMD_SET_ACT_INTV,
                      strlen(CMD_SET_ACT_INTV)) == 0) {
        i4BytesWritten = priv_driver_set_act_intv(prNetDev, pcCommand,
                                                  i4TotalLen);
    }
#endif
#if CFG_ENABLE_1RPD_MMPS_CTRL
    else if (strnicmp(pcCommand, CMD_SET_1RPD, strlen(CMD_SET_1RPD)) == 0) {
        i4BytesWritten =
            priv_driver_set_1rpd(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_1RPD, strlen(CMD_GET_1RPD)) ==
               0) {
        i4BytesWritten =
            priv_driver_get_1rpd(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_MMPS, strlen(CMD_SET_MMPS)) ==
               0) {
        i4BytesWritten =
            priv_driver_set_mmps(prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_MMPS, strlen(CMD_GET_MMPS)) ==
               0) {
        i4BytesWritten =
            priv_driver_get_mmps(prNetDev, pcCommand, i4TotalLen);
    }
#endif
#if CFG_ENABLE_DEWEIGHTING_CTRL
    else if (strnicmp(pcCommand, CMD_SET_DEWEIGHTING_TH,
                      strlen(CMD_SET_DEWEIGHTING_TH)) == 0) {
        i4BytesWritten = priv_driver_set_deweighting_th(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_DEWEIGHTING_TH,
                        strlen(CMD_GET_DEWEIGHTING_TH)) == 0) {
        i4BytesWritten = priv_driver_get_deweighting_th(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_DEWEIGHTING_NOISE,
                        strlen(CMD_GET_DEWEIGHTING_NOISE)) == 0) {
        i4BytesWritten = priv_driver_get_deweighting_noise(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_DEWEIGHTING_WEIGHT,
                        strlen(CMD_GET_DEWEIGHTING_WEIGHT)) == 0) {
        i4BytesWritten = priv_driver_get_deweighting_weight(
            prNetDev, pcCommand, i4TotalLen);
    }
#endif

#ifdef CFG_SUPPORT_ADJUST_MCC_STAY_TIME
    else if (strnicmp(pcCommand, CMD_MCCTIME, strlen(CMD_MCCTIME)) == 0) {
        i4BytesWritten = priv_driver_set_mcc_time(prNetDev, pcCommand,
                                                  i4TotalLen);
    }
#endif
#ifdef CFG_SUPPORT_ANT_DIV
    else if (strnicmp(pcCommand, CMD_GET_ANT_DIV,
                      strlen(CMD_GET_ANT_DIV)) == 0) {
        i4BytesWritten = priv_driver_ant_diversity_config(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_DETC_ANT_DIV,
                        strlen(CMD_DETC_ANT_DIV)) == 0) {
        i4BytesWritten = priv_driver_ant_diversity_config(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SWH_ANT_DIV,
                        strlen(CMD_SWH_ANT_DIV)) == 0) {
        i4BytesWritten = priv_driver_ant_diversity_config(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SET_ANT_DIV,
                        strlen(CMD_SET_ANT_DIV)) == 0) {
        i4BytesWritten = priv_driver_ant_diversity_config(
            prNetDev, pcCommand, i4TotalLen);
    }
#endif
    else if (strnicmp(pcCommand, CMD_ENFORCE_POWER_MODE,
                      strlen(CMD_ENFORCE_POWER_MODE)) == 0) {
        i4BytesWritten = priv_driver_enforce_power_mode(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_POWER_MODE,
                        strlen(CMD_GET_POWER_MODE)) == 0) {
        i4BytesWritten = priv_driver_get_power_mode(prNetDev, pcCommand,
                                                    i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_SEND_BEACONTIMEOUT,
                        strlen(CMD_SEND_BEACONTIMEOUT)) == 0) {
        i4BytesWritten = priv_driver_send_beacon_timeout(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_DISCONNECT_REASON,
                        strlen(CMD_GET_DISCONNECT_REASON)) == 0) {
        i4BytesWritten = priv_driver_get_disconnect_reason(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_GET_1XTX_STATUS,
                        strlen(CMD_GET_1XTX_STATUS)) == 0) {
        i4BytesWritten = priv_driver_get_1xtx_status(
            prNetDev, pcCommand, i4TotalLen);
    } else if (strnicmp(pcCommand, CMD_TEST_1XTX_STATUS,
                        strlen(CMD_TEST_1XTX_STATUS)) == 0) {
        i4BytesWritten = priv_driver_test_1xtx_status(
            prNetDev, pcCommand, i4TotalLen);
    }
#if CFG_SUPPORT_802_11K
    else if (strnicmp(pcCommand, CMD_NEIGHBOR_REQUEST,
                      strlen(CMD_NEIGHBOR_REQUEST)) == 0) {
        i4BytesWritten = priv_driver_neighbor_request(
            prNetDev, pcCommand, i4TotalLen);
    }
#endif
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
    else if (strnicmp(pcCommand, CMD_BSS_TRANSITION_QUERY,
                      strlen(CMD_BSS_TRANSITION_QUERY)) == 0) {
        i4BytesWritten = priv_driver_bss_transition_query(
            prNetDev, pcCommand, i4TotalLen);
    } else if (kalStrStr(pcCommand, "-IT ")) {
        i4BytesWritten = kalIoctl(prGlueInfo, wlanoidPktProcessIT,
                                  (void *)pcCommand, i4TotalLen, false,
                                  false, false, &i4BytesWritten);
    }
#endif
    else {
        i4BytesWritten =
            priv_cmd_not_support(prNetDev, pcCommand, i4TotalLen);
    }

    if (i4BytesWritten >= 0) {
        if ((i4BytesWritten == 0) && (i4TotalLen > 0)) {
            /* reset the command buffer */
            pcCommand[0] = '\0';
        }

        if (i4BytesWritten >= i4TotalLen) {
            DBGLOG(REQ, INFO,
                   "%s: i4BytesWritten %d > i4TotalLen < %d\n",
                   __func__, i4BytesWritten, i4TotalLen);
            i4BytesWritten = i4TotalLen;
        } else {
            pcCommand[i4BytesWritten] = '\0';
            i4BytesWritten++;
        }
    }

    return i4BytesWritten;
}

int priv_support_driver_cmd(IN struct net_device *prNetDev,
                            IN OUT struct ifreq *prReq, IN int i4Cmd)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    int ret = 0;
    char *pcCommand = NULL;
    priv_driver_cmd_t *priv_cmd = NULL;
    int i4BytesWritten = 0;
    int i4TotalLen = 0;
    struct iwreq *wrqin = (struct iwreq *)prReq;

    if (!prReq->ifr_data) {
        ret = -EINVAL;
        goto exit;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prNetDev));
    if (!prGlueInfo) {
        DBGLOG(REQ, WARN, "No glue info\n");
        ret = -EFAULT;
        goto exit;
    }
    if (prGlueInfo->u4ReadyFlag == 0) {
        ret = -EINVAL;
        goto exit;
    }

    priv_cmd = kzalloc(sizeof(priv_driver_cmd_t), GFP_KERNEL);
    if (!priv_cmd) {
        DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
        return -ENOMEM;
    }

    if (copy_from_user(priv_cmd->buf, prReq->ifr_data, PRIV_CMD_SIZE)) {
        DBGLOG(REQ, INFO, "%s: copy_from_user fail\n", __func__);
        ret = -EFAULT;
        goto exit;
    }

    priv_cmd->total_len = wrqin->u.data.length;

    i4TotalLen = priv_cmd->total_len;

    if (i4TotalLen <= 0) {
        ret = -EINVAL;
        DBGLOG(REQ, INFO, "%s: i4TotalLen invalid %x %x %s\n", __func__,
               i4TotalLen, priv_cmd->used_len, priv_cmd->buf);
        goto exit;
    }
    priv_cmd->buf[PRIV_CMD_SIZE - 1] = '\0';
    pcCommand = priv_cmd->buf;

    DBGLOG(REQ, INFO, "%s: driver cmd \"%s\" on %s\n", __func__, pcCommand,
           prReq->ifr_name);

    i4BytesWritten = priv_driver_cmds(prNetDev, pcCommand, PRIV_CMD_SIZE);

    if (i4BytesWritten < 0) {
        DBGLOG(REQ, INFO, "%s: command %s Written is %d\n", __func__,
               pcCommand, i4BytesWritten);
        if (i4TotalLen >= 3) {
            snprintf(pcCommand, 3, "OK");
            i4BytesWritten = strlen("OK");
        }
    }

    if (copy_to_user(prReq->ifr_data, priv_cmd->buf, PRIV_CMD_SIZE)) {
        DBGLOG(REQ, INFO, "%s: copy_to_user fail\n", __func__);
        ret = -EFAULT;
        goto exit;
    }

    if (i4BytesWritten > 0) {
        if (i4BytesWritten > PRIV_CMD_SIZE) {
            i4BytesWritten = PRIV_CMD_SIZE;
        }
        wrqin->u.data.length = i4BytesWritten;  /* the iwpriv will use
                                                 * the length */
    } else if (i4BytesWritten == 0) {
        wrqin->u.data.length = i4BytesWritten;
    }

exit:
    kfree(priv_cmd);

    return ret;
}

#ifdef CFG_ANDROID_AOSP_PRIV_CMD
int android_private_support_driver_cmd(IN struct net_device *prNetDev,
                                       IN OUT struct ifreq *prReq,
                                       IN int i4Cmd)
{
    struct android_wifi_priv_cmd priv_cmd;
    char *command = NULL;
    int ret = 0, bytes_written = 0;

    if (!prReq->ifr_data) {
        return -EINVAL;
    }

    if (copy_from_user(&priv_cmd, prReq->ifr_data, sizeof(priv_cmd))) {
        return -EFAULT;
    }

    if (priv_cmd.total_len <= 0 || priv_cmd.total_len > PRIV_CMD_SIZE) {
        return -EINVAL;
    }

    command = kzalloc(priv_cmd.total_len, GFP_KERNEL);
    if (!command) {
        DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
        return -ENOMEM;
    }

    if (copy_from_user(command, priv_cmd.buf, priv_cmd.total_len)) {
        ret = -EFAULT;
        goto FREE;
    }

    bytes_written = priv_driver_cmds(prNetDev, command, priv_cmd.total_len);

    if (bytes_written >= 0) {
        /* priv_cmd in but no response */
        if ((bytes_written == 0) && (priv_cmd.total_len > 0)) {
            command[0] = '\0';
        }

        if (bytes_written >= priv_cmd.total_len) {
            bytes_written = priv_cmd.total_len;
        }else{
            bytes_written++;
        }

        priv_cmd.used_len = bytes_written;

        if (copy_to_user(priv_cmd.buf, command, bytes_written)) {
            ret = -EFAULT;
        }
    } else {
        ret = bytes_written;
    }

FREE:
    kfree(command);

    return ret;
}
#endif
