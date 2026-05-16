/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   precomp.h
 *    \brief  Collection of most compiler flags are described here.
 *
 *    In this file we collect all compiler flags and detail the driver behavior
 * if enable/disable such switch or adjust numeric parameters.
 */

#ifndef _PRECOMP_H
#define _PRECOMP_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

//#define DBG    1

#ifdef __GNUC__
#ifdef DBG
#if (DBG == 0)
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma GCC diagnostic ignored "-Wstringop-overread"
#pragma GCC diagnostic ignored "-Wenum-conversion"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wempty-body"
#endif
#endif
#endif

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "gl_os.h"  /* Include "config.h" */

#if CFG_ENABLE_WIFI_DIRECT
#include "gl_p2p_os.h"
#endif

#include "debug.h"

#include "link.h"
#include "queue.h"

/*------------------------------------------------------------------------------
 * .\include\mgmt
 *------------------------------------------------------------------------------
 */
#include "wlan_typedef.h"

#include "mac.h"

/* Dependency:  mac.h (MAC_ADDR_LEN) */
#include "wlan_def.h"

#if CFG_SUPPORT_SWCR
#include "swcr.h"
#endif

/*------------------------------------------------------------------------------
 * .\include\nic
 *------------------------------------------------------------------------------
 */
/* Dependency:  wlan_def.h (ENUM_NETWORK_TYPE_T) */
#include "cmd_buf.h"

/* Dependency:  mac.h (MAC_ADDR_LEN) */
#include "nic_cmd_event.h"

/* Dependency:  nic_cmd_event.h (P_EVENT_CONNECTION_STATUS) */
#include "nic.h"

#include "nic_init_cmd_event.h"

#include "hif_rx.h"
#include "hif_tx.h"

#include "nic_tx.h"

/* Dependency:  hif_rx.h (P_HIF_RX_HEADER_T) */
#include "nic_rx.h"

#include "nic_umac.h"

#include "bss.h"

#include "nic_rate.h"

#if CFG_ENABLE_WIFI_DIRECT
#include "p2p_typedef.h"
#include "p2p_cmd_buf.h"
#include "p2p_nic_cmd_event.h"
#include "p2p_mac.h"
#include "p2p_nic.h"
#endif

/*------------------------------------------------------------------------------
 * .\include\mgmt
 *------------------------------------------------------------------------------
 */

#include "hem_mbox.h"

#include "scan.h"

#include "wlan_lib.h"
#include "wlan_oid.h"

#if CFG_ENABLE_WIFI_DIRECT
#include "wlan_p2p.h"
#endif

#include "hal.h"

#include "reg.h"

#include "rlm.h"
#include "rlm_domain.h"
#include "rlm_protection.h"
#include "rlm_obss.h"
#include "rate.h"
#if CFG_SUPPORT_802_11V
#include "wnm.h"
#endif

#include "aa_fsm.h"

#include "cnm_timer.h"

#include "que_mgt.h"

#include "pwr_mgt.h"

#include "cnm.h"
/* Dependency:  aa_fsm.h (ENUM_AA_STATE_T), p2p_fsm.h
 * (WPS_ATTRI_MAX_LEN_DEVICE_NAME) */
#include "cnm_mem.h"
#include "cnm_scan.h"

#if CFG_ENABLE_WIFI_DIRECT
#include "p2p_rlm_obss.h"
#include "p2p.h"

#include "p2p_rlm.h"
#include "p2p_assoc.h"
#include "p2p_ie.h"
#include "p2p_role.h"

#include "p2p_func.h"
#include "p2p_scan.h"
#include "p2p_dev.h"
#include "p2p_fsm.h"
#endif

#include "privacy.h"

#include "mib.h"

#include "auth.h"
#include "assoc.h"

#if CFG_SUPPORT_ROAMING
#include "roaming_fsm.h"
#endif

#include "ais_fsm.h"

#include "adapter.h"

#include "que_mgt.h"
#include "rftest.h"

#include "rsn.h"

/*------------------------------------------------------------------------------
 * NVRAM structure
 *------------------------------------------------------------------------------
 */
#include "CFG_Wifi_File.h"

#if CFG_ENABLE_WIFI_DIRECT
#include "gl_p2p_kal.h"
#endif

#if CFG_SUPPORT_TDLS
#include "tdls.h"
#endif

#if CFG_SUPPORT_QA_TOOL
#include "gl_qa_agent.h"
#endif

/*------------------------------------------------------------------------------
 * Memory Prealloc
 *------------------------------------------------------------------------------
 */
#ifdef CFG_PREALLOC_MEMORY
#include "prealloc.h"
#endif

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
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void rlmDomainBuildCmdByDefaultTable(
    P_CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_T prCmd, u16 u2DefaultTableIndex);
void rlmDomainBuildCmdByConfigTable(
    P_ADAPTER_T prAdapter, P_CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_T prCmd);
u8 rlmDomainGetTxPwrLimit(u32 country_code, P_GLUE_INFO_T prGlueInfo,
                          P_CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_V2_T pSetCmd_2g,
                          P_CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_V2_T pSetCmd_5g);
s8 rlmDomainTxPwrLimitGetChIdx(struct TX_PWR_LIMIT_DATA *pTxPwrLimit,
                               u8 ucChannel);
u8 rlmDomainTxPwrLimitLoadChannelSetting(u8 *pucBuf, u32 *pu4Pos, u32 u4BufEnd,
                                         struct TX_PWR_LIMIT_DATA *pTxPwrLimit,
                                         u8 ucSectionIdx);
u8 rlmDomainTxPwrLimitLoad(P_ADAPTER_T prAdapter, u8 *pucBuf, u32 u4BufLen,
                           u32 u4CountryCode,
                           struct TX_PWR_LIMIT_DATA *pTxPwrLimit);
void rlmDomainTxPwrLimitSetChValues(P_CMD_CHANNEL_POWER_LIMIT_V2 pCmd,
                                    struct CHANNEL_TX_PWR_LIMIT *pChTxPwrLimit);
void rlmDomainTxPwrLimitSetValues(
    P_CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_V2_T pSetCmd,
    struct TX_PWR_LIMIT_DATA *pTxPwrLimit);
u8 rlmDomainTxPwrLimitLoadFromFile(P_ADAPTER_T prAdapter, u32 u4CountryCode,
                                   struct TX_PWR_LIMIT_DATA *pTxPwrLimit);
void saaSendAuthSeq3(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec);
void tkipMicB(IN OUT u32 *pu4L, IN OUT u32 *pu4R);
void tkipMicGen(IN u8 *pucMickey, IN u8 *pucData, IN u32 u4DataLen,
                IN u8 *pucSa, IN u8 *pucDa, IN u8 ucPriority, OUT u8 *pucMic);
void tkipMicEncapsulate(IN u8 *pucDa, IN u8 *pucSa, IN u8 ucPriority,
                        IN u16 u2PayloadLen, IN u8 *pucPayload, IN u8 *pucMic,
                        IN u8 *pucMicKey);

#if CFG_SUPPORT_LAST_SEC_MCS_INFO
s32 priv_driver_last_sec_mcs_info(IN P_ADAPTER_T prAdapter, IN char *pcCommand,
                                  IN int i4TotalLen,
                                  P_PARAM_HW_WLAN_INFO_T prHwWlanInfo,
                                  struct PARAM_TX_MCS_INFO *prTxMcsInfo);
#endif

s32 priv_driver_tx_rate_info(IN char *pcCommand, IN int i4TotalLen,
                             u8 fgDumpAll, P_PARAM_HW_WLAN_INFO_T prHwWlanInfo,
                             P_PARAM_GET_STA_STATISTICS prQueryStaStatistics);

s32 priv_driver_last_rx_rssi(P_ADAPTER_T prAdapter, IN char *pcCommand,
                             IN int i4TotalLen, IN u8 ucWlanIdx);

s32 priv_driver_rx_rate_info(P_ADAPTER_T prAdapter, IN char *pcCommand,
                             IN int i4TotalLen, IN u8 ucWlanIdx);

s32 priv_driver_tx_vector_info(IN char *pcCommand, IN int i4TotalLen,
                               IN P_TX_VECTOR_BBP_LATCH_T prTxV);

s32 priv_driver_rate_to_string(IN char *pcCommand, IN int i4TotalLen, u8 TxRx,
                               P_PARAM_HW_WLAN_INFO_T prHwWlanInfo);

void parseNoiseHistogramReport(s32 *i4BytesWritten, s8 *pcCommand,
                               int *i4TotalLen,
                               IN struct CMD_NOISE_HISTOGRAM_REPORT *cmd);

WLAN_STATUS
batchConvertResult(IN P_EVENT_BATCH_RESULT_T prEventBatchResult,
                   OUT void *pvBuffer, IN u32 u4MaxBufferLen,
                   OUT u32 *pu4RetLen);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

extern int mtk_sdio_probe(struct sdio_func *func,
                          const struct sdio_device_id *id);
extern void mtk_sdio_remove(struct sdio_func *func);
int mtk_sdio_async_irq_enable(struct sdio_func *func);

#endif
