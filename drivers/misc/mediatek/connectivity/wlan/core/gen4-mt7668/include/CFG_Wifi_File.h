/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   CFG_Wifi_File.h
 *    \brief  Collection of NVRAM structure used for YuSu project
 *
 *    In this file we collect all compiler flags and detail the driver behavior
 * if enable/disable such switch or adjust numeric parameters.
 */

#ifndef _CFG_WIFI_FILE_H
#define _CFG_WIFI_FILE_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "gl_typedef.h"

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

/* duplicated from nic_cmd_event.h to avoid header dependency */
typedef struct _TX_PWR_PARAM_T {
    s8 cTxPwr2G4Cck;  /* signed, in unit of 0.5dBm */
    s8 cTxPwr2G4Dsss;  /* signed, in unit of 0.5dBm */
    s8 acReserved[2];

    s8 cTxPwr2G4OFDM_BPSK;
    s8 cTxPwr2G4OFDM_QPSK;
    s8 cTxPwr2G4OFDM_16QAM;
    s8 cTxPwr2G4OFDM_Reserved;
    s8 cTxPwr2G4OFDM_48Mbps;
    s8 cTxPwr2G4OFDM_54Mbps;

    s8 cTxPwr2G4HT20_BPSK;
    s8 cTxPwr2G4HT20_QPSK;
    s8 cTxPwr2G4HT20_16QAM;
    s8 cTxPwr2G4HT20_MCS5;
    s8 cTxPwr2G4HT20_MCS6;
    s8 cTxPwr2G4HT20_MCS7;

    s8 cTxPwr2G4HT40_BPSK;
    s8 cTxPwr2G4HT40_QPSK;
    s8 cTxPwr2G4HT40_16QAM;
    s8 cTxPwr2G4HT40_MCS5;
    s8 cTxPwr2G4HT40_MCS6;
    s8 cTxPwr2G4HT40_MCS7;

    s8 cTxPwr5GOFDM_BPSK;
    s8 cTxPwr5GOFDM_QPSK;
    s8 cTxPwr5GOFDM_16QAM;
    s8 cTxPwr5GOFDM_Reserved;
    s8 cTxPwr5GOFDM_48Mbps;
    s8 cTxPwr5GOFDM_54Mbps;

    s8 cTxPwr5GHT20_BPSK;
    s8 cTxPwr5GHT20_QPSK;
    s8 cTxPwr5GHT20_16QAM;
    s8 cTxPwr5GHT20_MCS5;
    s8 cTxPwr5GHT20_MCS6;
    s8 cTxPwr5GHT20_MCS7;

    s8 cTxPwr5GHT40_BPSK;
    s8 cTxPwr5GHT40_QPSK;
    s8 cTxPwr5GHT40_16QAM;
    s8 cTxPwr5GHT40_MCS5;
    s8 cTxPwr5GHT40_MCS6;
    s8 cTxPwr5GHT40_MCS7;
} TX_PWR_PARAM_T, *P_TX_PWR_PARAM_T;

typedef struct _TX_AC_PWR_T {
    s8 c11AcTxPwr_BPSK;
    s8 c11AcTxPwr_QPSK;
    s8 c11AcTxPwr_16QAM;
    s8 c11AcTxPwr_MCS5_MCS6;
    s8 c11AcTxPwr_MCS7;
    s8 c11AcTxPwr_MCS8;
    s8 c11AcTxPwr_MCS9;
    s8 c11AcTxPwrVht40_OFFSET;
    s8 c11AcTxPwrVht80_OFFSET;
    s8 c11AcTxPwrVht160_OFFSET;
    s8 acReverse[2];
} TX_AC_PWR_T, *P_TX_AC_PWR_T;

typedef struct _RSSI_PATH_COMPASATION_T {
    s8 c2GRssiCompensation;
    s8 c5GRssiCompensation;
} RSSI_PATH_COMPASATION_T, *P_RSSI_PATH_COMPASATION_T;

typedef struct _PWR_5G_OFFSET_T {
    s8 cOffsetBand0;  /* 4.915-4.980G */
    s8 cOffsetBand1;  /* 5.000-5.080G */
    s8 cOffsetBand2;  /* 5.160-5.180G */
    s8 cOffsetBand3;  /* 5.200-5.280G */
    s8 cOffsetBand4;  /* 5.300-5.340G */
    s8 cOffsetBand5;  /* 5.500-5.580G */
    s8 cOffsetBand6;  /* 5.600-5.680G */
    s8 cOffsetBand7;  /* 5.700-5.825G */
} PWR_5G_OFFSET_T, *P_PWR_5G_OFFSET_T;

typedef struct _PWR_PARAM_T {
    u32 au4Data[28];
    u32 u4RefValue1;
    u32 u4RefValue2;
} PWR_PARAM_T, *P_PWR_PARAM_T;

typedef struct _AC_PWR_SETTING_STRUCT {
    u8 c11AcTxPwr_BPSK;
    u8 c11AcTxPwr_QPSK;
    u8 c11AcTxPwr_16QAM;
    u8 c11AcTxPwr_MCS5_MCS6;
    u8 c11AcTxPwr_MCS7;
    u8 c11AcTxPwr_MCS8;
    u8 c11AcTxPwr_MCS9;
    u8 c11AcTxPwr_Reserved;
    u8 c11AcTxPwrVht40_OFFSET;
    u8 c11AcTxPwrVht80_OFFSET;
    u8 c11AcTxPwrVht160_OFFSET;
} AC_PWR_SETTING_STRUCT, *P_AC_PWR_SETTING_STRUCT;

typedef struct _BANDEDGE_5G_T {
    u8 uc5GBandEdgePwrUsed;
    u8 c5GBandEdgeMaxPwrOFDM20;
    u8 c5GBandEdgeMaxPwrOFDM40;
    u8 c5GBandEdgeMaxPwrOFDM80;
} BANDEDGE_5G_T, *P_BANDEDGE_5G_T;

typedef struct _NEW_EFUSE_MAPPING2NVRAM_T {
    u8 ucReverse1[8];
    u16 u2Signature;
    BANDEDGE_5G_T r5GBandEdgePwr;
    u8 ucReverse2[14];

    /* 0x50 */
    u8 aucChOffset[3];
    u8 ucChannelOffsetVaild;
    u8 acAllChannelOffset;
    u8 aucChOffset3[11];

    /* 0x60 */
    u8 auc5GChOffset[8];
    u8 uc5GChannelOffsetVaild;
    u8 aucChOffset4[7];

    /* 0x70 */
    AC_PWR_SETTING_STRUCT r11AcTxPwr;
    u8 uc11AcTxPwrValid;

    u8 ucReverse4[20];

    /* 0x90 */
    AC_PWR_SETTING_STRUCT r11AcTxPwr2G;
    u8 uc11AcTxPwrValid2G;

    u8 ucReverse5[40];
} NEW_EFUSE_MAPPING2NVRAM_T, *P_NEW_EFUSE_MAPPING2NVRAM_T;

typedef struct _MT6620_CFG_PARAM_STRUCT {
    /* 256 bytes of MP data */
    u16 u2Part1OwnVersion;
    u16 u2Part1PeerVersion;
    u8 aucMacAddress[6];
    u8 aucCountryCode[2];
    TX_PWR_PARAM_T rTxPwr;
#if CFG_SUPPORT_NVRAM_5G
    union {
        NEW_EFUSE_MAPPING2NVRAM_T u;
        u8 aucEFUSE[144];
    } EfuseMapping;
#else
    u8 aucEFUSE[144];
#endif
    u8 ucTxPwrValid;
    u8 ucSupport5GBand;
    u8 fg2G4BandEdgePwrUsed;
    s8 cBandEdgeMaxPwrCCK;
    s8 cBandEdgeMaxPwrOFDM20;
    s8 cBandEdgeMaxPwrOFDM40;

    u8 ucRegChannelListMap;
    u8 ucRegChannelListIndex;
    u8 aucRegSubbandInfo[36];

    u8 aucReserved2[256 - 240];

    /* 256 bytes of function data */
    u16 u2Part2OwnVersion;
    u16 u2Part2PeerVersion;
    u8 uc2G4BwFixed20M;
    u8 uc5GBwFixed20M;
    u8 ucEnable5GBand;
    u8 ucRxDiversity;
    RSSI_PATH_COMPASATION_T rRssiPathCompensation;
    u8 fgRssiCompensationVaildbit;
    u8 ucGpsDesense;
    u16 u2FeatureReserved;
    u8 aucPreTailReserved;
    u8 aucTailReserved[256 - 15];
} MT6620_CFG_PARAM_STRUCT, *P_MT6620_CFG_PARAM_STRUCT, WIFI_CFG_PARAM_STRUCT,
*P_WIFI_CFG_PARAM_STRUCT;

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#ifndef DATA_STRUCT_INSPECTING_ASSERT
#define DATA_STRUCT_INSPECTING_ASSERT(expr) \
    {                                       \
        switch (0) {                        \
        case 0:                             \
        case (expr):                        \
        default:;                           \
        }                                   \
    }
#endif

#define CFG_FILE_WIFI_REC_SIZE    sizeof(WIFI_CFG_PARAM_STRUCT)

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#ifndef _lint
/* We don't have to call following function to inspect the data structure.
 * It will check automatically while at compile time.
 * We'll need this to guarantee the same member order in different structures
 * to simply handling effort in some functions.
 */
static __KAL_INLINE__ void nvramOffsetCheck(void)
{
    DATA_STRUCT_INSPECTING_ASSERT(
        OFFSET_OF(WIFI_CFG_PARAM_STRUCT, u2Part2OwnVersion) == 256);

    DATA_STRUCT_INSPECTING_ASSERT(sizeof(WIFI_CFG_PARAM_STRUCT) == 512);
#if CFG_SUPPORT_NVRAM_5G
    DATA_STRUCT_INSPECTING_ASSERT(
        (OFFSET_OF(WIFI_CFG_PARAM_STRUCT, EfuseMapping) & 0x0001) == 0);
#else
    DATA_STRUCT_INSPECTING_ASSERT(
        (OFFSET_OF(WIFI_CFG_PARAM_STRUCT, aucEFUSE) & 0x0001) == 0);
#endif
    DATA_STRUCT_INSPECTING_ASSERT(
        (OFFSET_OF(WIFI_CFG_PARAM_STRUCT, aucRegSubbandInfo) &
         0x0001) == 0);
}
#endif

#endif
