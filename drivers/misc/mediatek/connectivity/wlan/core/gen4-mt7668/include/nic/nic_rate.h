/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file  nic_rate.h
 *    \brief This file contains the rate utility function of
 *    IEEE 802.11 family for MediaTek 802.11 Wireless LAN Adapters.
 */

#ifndef _NIC_RATE_H
#define _NIC_RATE_H

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
 *                         D A T A   T Y P E S
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

u32 nicGetPhyRateByMcsRate(IN u8 ucIdx, IN u8 ucBw, IN u8 ucGI);

u32 nicGetHwRateByPhyRate(IN u8 ucIdx);

WLAN_STATUS
nicSwIndex2RateIndex(IN u8 ucRateSwIndex,
                     OUT u8 *pucRateIndex,
                     OUT u8 *pucPreambleOption);

WLAN_STATUS
nicRateIndex2RateCode(IN u8 ucPreambleOption,
                      IN u8 ucRateIndex,
                      OUT u16 *pu2RateCode);

u32 nicRateCode2PhyRate(IN u16 u2RateCode,
                        IN u8 ucBandwidth,
                        IN u8 ucGI,
                        IN u8 ucRateNss);

u32 nicRateCode2DataRate(IN u16 u2RateCode, IN u8 ucBandwidth, IN u8 ucGI);

u8 nicGetRateIndexFromRateSetWithLimit(IN u16 u2RateSet,
                                       IN u32 u4PhyRateLimit,
                                       IN u8 fgGetLowest,
                                       OUT u8 *pucRateSwIndex);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
