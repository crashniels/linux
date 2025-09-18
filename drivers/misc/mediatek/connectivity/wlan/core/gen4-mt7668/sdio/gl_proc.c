// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "gl_proc.c"
 *    \brief  This file defines the interface which can interact with users in
 * /proc fs.
 *
 *    Detail description.
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
#include "gl_kal.h"
#include "debug.h"
#include "wlan_lib.h"
#include "debug.h"
#include "wlan_oid.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define PROC_MAX_BUF_SIZE            3000
#define PROC_MCR_ACCESS              "mcr"

#define PROC_ROOT_NAME               "wlan"

#if CFG_SUPPORT_DEBUG_FS
#define PROC_COUNTRY                 "country"
#endif
#define PROC_DRV_STATUS              "status"
#define PROC_RX_STATISTICS           "rx_statistics"
#define PROC_TX_STATISTICS           "tx_statistics"
#define PROC_DBG_LEVEL_NAME          "dbg_level"
#define PROC_DRIVER_CMD              "driver"
#define PROC_CFG                 "cfg"
#define PROC_EFUSE_DUMP              "efuse_dump"
#ifdef CFG_DUMP_TXPOWR_TABLE
#define PROC_GET_TXPWR_TBL           "get_txpwr_tbl"
#endif

#define PROC_MCR_ACCESS_MAX_USER_INPUT_LEN   20
#define PROC_RX_STATISTICS_MAX_USER_INPUT_LEN    10
#define PROC_TX_STATISTICS_MAX_USER_INPUT_LEN    10
#define PROC_DBG_LEVEL_MAX_USER_INPUT_LEN    20
#define PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN   30
#define PROC_UID_SHELL               2000
#define PROC_GID_WIFI                1010

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

static P_GLUE_INFO_T g_prGlueInfo_proc;
static u32 u4McrOffset;
static struct proc_dir_entry *gprProcRoot;
static u8 aucDbModuleName[][PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN] = {
    "INIT", "HAL", "INTR", "REQ", "TX", "RX", "RFTEST", "EMU", "SW1",
    "SW2", "SW3", "SW4", "HEM", "AIS", "RLM", "MEM", "CNM", "RSN",
    "BSS", "SCN", "SAA", "AAA", "P2P", "QM", "SEC", "BOW", "WAPI",
    "ROAMING", "TDLS", "PF", "OID", "NIC", "WNM"
};

/* This u32 is only for DriverCmdRead/Write, should not be used by other
 * function */
static s32 g_i4NextDriverReadLen;
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static ssize_t procDbgLevelRead(struct file *filp, char __user *buf,
                                size_t count, loff_t *f_pos)
{
    u8 *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
    u8 *temp = pucProcBuf;
    u32 u4CopySize = 0;
    u16 i;
    u16 u2ModuleNum = 0;
    s32 i4Pos = 0;
    s32 i4Ret = 0;

    /* if *f_ops>0, we should return 0 to make cat command exit */
    if (*f_pos > 0 || buf == NULL || pucProcBuf == NULL) {
        i4Ret = 0;
        goto freeBuf;
    }

    i4Pos = scnprintf(
        temp, (PROC_MAX_BUF_SIZE - i4Pos),
        "\nERROR|WARN|STATE|EVENT|TRACE|INFO|LOUD|TEMP\n"
        "bit0 |bit1|bit2 |bit3 |bit4 |bit5|bit6|bit7\n\n"
        "Debug Module\tIndex\tLevel\tDebug Module\tIndex\tLevel\n\n");

    u2ModuleNum =
        (sizeof(aucDbModuleName) / PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN) &
        0xfe;
    for (i = 0; i < u2ModuleNum; i += 2)
        i4Pos += scnprintf(
            (temp + i4Pos),
            (PROC_MAX_BUF_SIZE - i4Pos),
            "DBG_%s_IDX\t(0x%02x):\t0x%02x\tDBG_%s_IDX\t(0x%02x):\t0x%02x\n",
            &aucDbModuleName[i][0],
            i,
            aucDebugModule[i],
            &aucDbModuleName[i + 1][0],
            i + 1,
            aucDebugModule[i + 1]);

    if ((sizeof(aucDbModuleName) / PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN) &
        0x1) {
        i4Pos += scnprintf((temp + i4Pos), (PROC_MAX_BUF_SIZE - i4Pos),
                           "DBG_%s_IDX\t(0x%02x):\t0x%02x\n",
                           &aucDbModuleName[u2ModuleNum][0],
                           u2ModuleNum, aucDebugModule[u2ModuleNum]);
    }

    u4CopySize = i4Pos;
    if (u4CopySize > count) {
        u4CopySize = count;
    }
    if (copy_to_user(buf, pucProcBuf, u4CopySize)) {
        DBGLOG(INIT, ERROR, "copy to user failed\n");
        i4Ret = -EFAULT;
        goto freeBuf;
    }

    *f_pos += u4CopySize;
    i4Ret = u4CopySize;
freeBuf:
    if (pucProcBuf) {
        kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
    }
    return i4Ret;
}

#if WLAN_INCLUDE_PROC
#if CFG_SUPPORT_EASY_DEBUG

static void *procEfuseDump_start(struct seq_file *s, loff_t *pos)
{
    static unsigned long counter;

    if (*pos == 0) {
        counter = *pos;  /* read file init */

    }
    if (counter >= EFUSE_ADDR_MAX) {
        return NULL;
    }

    return &counter;
}
static void *procEfuseDump_next(struct seq_file *s, void *v, loff_t *pos)
{
    unsigned long *tmp_v = (unsigned long *)v;

    (*tmp_v) += EFUSE_BLOCK_SIZE;

    if (*tmp_v >= EFUSE_ADDR_MAX) {
        return NULL;
    }

    return tmp_v;
}
static void procEfuseDump_stop(struct seq_file *s, void *v)
{
    /* nothing to do, we use a static value in start() */
}
static int procEfuseDump_show(struct seq_file *s, void *v)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 u4BufLen = 0;
    P_GLUE_INFO_T prGlueInfo;
    u32 idx_addr, idx_value;
    PARAM_CUSTOM_ACCESS_EFUSE_T rAccessEfuseInfo = {};

    prGlueInfo = g_prGlueInfo_proc;

#if (CFG_EEPROM_PAGE_ACCESS == 1)
    idx_addr = *(loff_t *)v;
    rAccessEfuseInfo.u4Address =
        (idx_addr / EFUSE_BLOCK_SIZE) * EFUSE_BLOCK_SIZE;

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryProcessAccessEfuseRead,
                       &rAccessEfuseInfo,
                       sizeof(PARAM_CUSTOM_ACCESS_EFUSE_T), true, true,
                       true, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        seq_printf(s, "efuse read fail (0x%03X)\n",
                   rAccessEfuseInfo.u4Address);
        return 0;
    }

    for (idx_value = 0; idx_value < EFUSE_BLOCK_SIZE; idx_value++)
        seq_printf(s, "0x%03X=0x%02X\n",
                   rAccessEfuseInfo.u4Address + idx_value,
                   prGlueInfo->prAdapter->aucEepromVaule[idx_value]);
    return 0;

#else
    seq_puts(s, "efuse ops is invalid\n");
    return -EPERM;  /* return negative value to stop read process */

#endif
}
static int procEfuseDumpOpen(struct inode *inode, struct file *file)
{
    static const struct seq_operations procEfuseDump_ops = {
        .start = procEfuseDump_start,
        .next = procEfuseDump_next,
        .stop = procEfuseDump_stop,
        .show = procEfuseDump_show
    };

    return seq_open(file, &procEfuseDump_ops);
}

static ssize_t procCfgRead(struct file *filp, char __user *buf, size_t count,
                           loff_t *f_pos)
{
    u8 *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
    u8 *temp = pucProcBuf;
    u32 u4CopySize = 0;
    u16 i;
    s32 i4Pos = 0;
    s32 i4Ret = 0;

#define BUFFER_RESERVE_BYTE    50

    P_GLUE_INFO_T prGlueInfo;

    P_WLAN_CFG_ENTRY_T prWlanCfgEntry;
    P_ADAPTER_T prAdapter;

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(gPrDev));

    if (!prGlueInfo) {
        DBGLOG(INIT, ERROR, "procCfgRead prGlueInfo is NULL\n");
        i4Ret = 0;
        goto freeBuf;
    }

    prAdapter = prGlueInfo->prAdapter;

    if (!prAdapter) {
        DBGLOG(INIT, ERROR, "procCfgRead prAdapter is NULL\n");
        i4Ret = 0;
        goto freeBuf;
    }

    /* if *f_ops>0, we should return 0 to make cat command exit */
    if (*f_pos > 0 || buf == NULL || pucProcBuf == NULL) {
        i4Ret = 0;
        goto freeBuf;
    }

    i4Pos = scnprintf(temp, (PROC_MAX_BUF_SIZE - i4Pos),
                      "\nDUMP CONFIGURATION :\n"
                      "<KEY|VALUE> OR <D:KEY|VALUE>\n"
                      "'D': driver part current setting\n"
                      "===================================\n");

    for (i = 0; i < WLAN_CFG_ENTRY_NUM_MAX; i++) {
        prWlanCfgEntry = wlanCfgGetEntryByIndex(prAdapter, i, 0);

        if ((!prWlanCfgEntry) || (prWlanCfgEntry->aucKey[0] == '\0')) {
            break;
        }

        i4Pos += scnprintf((temp + i4Pos), (PROC_MAX_BUF_SIZE - i4Pos),
                           "%s|%s\n", prWlanCfgEntry->aucKey,
                           prWlanCfgEntry->aucValue);

        if (i4Pos > (PROC_MAX_BUF_SIZE - BUFFER_RESERVE_BYTE)) {
            break;
        }
    }

    for (i = 0; i < WLAN_CFG_REC_ENTRY_NUM_MAX; i++) {
        prWlanCfgEntry = wlanCfgGetEntryByIndex(prAdapter, i, 1);

        if ((!prWlanCfgEntry) || (prWlanCfgEntry->aucKey[0] == '\0')) {
            break;
        }

        i4Pos += scnprintf((temp + i4Pos), (PROC_MAX_BUF_SIZE - i4Pos),
                           "D:%s|%s\n", prWlanCfgEntry->aucKey,
                           prWlanCfgEntry->aucValue);

        if (i4Pos > (PROC_MAX_BUF_SIZE - BUFFER_RESERVE_BYTE)) {
            break;
        }
    }

    u4CopySize = i4Pos;
    if (u4CopySize > count) {
        u4CopySize = count;
    }
    if (copy_to_user(buf, pucProcBuf, u4CopySize)) {
        DBGLOG(INIT, ERROR, "copy to user failed\n");
        i4Ret = -EFAULT;
        goto freeBuf;
    }

    *f_pos += u4CopySize;
    i4Ret = u4CopySize;
freeBuf:
    if (pucProcBuf) {
        kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
    }
    return i4Ret;
}

static ssize_t procCfgWrite(struct file *file, const char __user *buffer,
                            size_t count, loff_t *data)
{
    u8 *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
    s32 u4CopySize = PROC_MAX_BUF_SIZE;
    P_GLUE_INFO_T prGlueInfo;
    u8 *pucTmp;
    s32 i4Pos = 0;
    s32 i4Ret = 0;

    if (count <= 0) {
        DBGLOG(INIT, ERROR, "Wrong buffer size\n");
        i4Ret = -EFAULT;
        goto freeBuf;
    }

    if (buffer == NULL || pucProcBuf == NULL) {
        i4Ret = 0;
        goto freeBuf;
    }

    pucTmp = pucProcBuf;
    i4Pos = scnprintf(pucTmp, PROC_MAX_BUF_SIZE, "%s ", "set_cfg");
    pucTmp += i4Pos;
    u4CopySize -= i4Pos;

    u4CopySize = (count < u4CopySize) ? count : (u4CopySize - 1);

    if ((u4CopySize < 0) || (copy_from_user(pucTmp, buffer, u4CopySize))) {
        DBGLOG(INIT, ERROR, "error of copy from user\n");
        i4Ret = -EFAULT;
        goto freeBuf;
    }
    pucProcBuf[u4CopySize + 8] = '\0';

    prGlueInfo = g_prGlueInfo_proc;
    /* if g_i4NextDriverReadLen >0,
     * the content for next DriverCmdRead will be in : pucProcBuf with
     * length : g_i4NextDriverReadLen
     */
    g_i4NextDriverReadLen = priv_driver_set_cfg(
        prGlueInfo->prDevHandler, pucProcBuf, kalStrLen(pucProcBuf));

    i4Ret = u4CopySize;
freeBuf:
    if (pucProcBuf) {
        kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
    }
    return i4Ret;
}

static ssize_t procDriverCmdRead(struct file *filp, char __user *buf,
                                 size_t count, loff_t *f_pos)
{
    return 0;
}

static ssize_t procDriverCmdWrite(struct file *file, const char __user *buffer,
                                  size_t count, loff_t *data)
{
    u8 *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
    u32 u4CopySize = PROC_MAX_BUF_SIZE;
    P_GLUE_INFO_T prGlueInfo = g_prGlueInfo_proc;
    s32 i4Ret = 0;

    if (buffer == NULL || pucProcBuf == NULL || prGlueInfo == NULL) {
        i4Ret = 0;
        goto freeBuf;
    }

    u4CopySize = (count < u4CopySize) ? count : (u4CopySize - 1);

    if (copy_from_user(pucProcBuf, buffer, u4CopySize)) {
        DBGLOG(INIT, ERROR, "error of copy from user\n");
        i4Ret = -EFAULT;
        goto freeBuf;
    }
    pucProcBuf[u4CopySize] = '\0';

    if (kalStrLen(pucProcBuf) > 0) {
        priv_driver_cmds(prGlueInfo->prDevHandler, pucProcBuf,
                         kalStrLen(pucProcBuf));
    }

    i4Ret = u4CopySize;
freeBuf:
    if (pucProcBuf) {
        kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
    }
    return i4Ret;
}

#endif
#endif

static ssize_t procDbgLevelWrite(struct file *file, const char __user *buffer,
                                 size_t count, loff_t *data)
{
    u32 u4NewDbgModule, u4NewDbgLevel;
    u8 *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
    u8 *temp = NULL;
    u32 u4CopySize = PROC_MAX_BUF_SIZE;
    s32 i4Ret = 0;

    if (buffer == NULL || pucProcBuf == NULL) {
        i4Ret = 0;
        goto freeBuf;
    }

    temp = pucProcBuf;
    u4CopySize = (count < u4CopySize) ? count : (u4CopySize - 1);

    if (copy_from_user(pucProcBuf, buffer, u4CopySize)) {
        DBGLOG(INIT, ERROR, "error of copy from user\n");
        i4Ret = -EFAULT;
        goto freeBuf;
    }
    pucProcBuf[u4CopySize] = '\0';

    while (temp) {
        if (sscanf(temp, "0x%x:0x%x", &u4NewDbgModule,
                   &u4NewDbgLevel) != 2) {
            DBGLOG(INIT,
                   ERROR,
                   "debug module and debug level should be one byte in length\n");
            break;
        }
        if (u4NewDbgModule == 0xFF) {
            u8 i = 0;

            for (; i < DBG_MODULE_NUM; i++)
                aucDebugModule[i] = u4NewDbgLevel &
                                    DBG_CLASS_MASK;

            break;
        }
        if (u4NewDbgModule >= DBG_MODULE_NUM) {
            DBGLOG(INIT, ERROR,
                   "debug module index should less than %d\n",
                   DBG_MODULE_NUM);
            break;
        }
        aucDebugModule[u4NewDbgModule] = u4NewDbgLevel & DBG_CLASS_MASK;
        temp = kalStrChr(temp, ',');
        if (!temp) {
            break;
        }
        temp++;  /* skip ',' */
    }

    i4Ret = u4CopySize;
freeBuf:
    if (pucProcBuf) {
        kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
    }
    return i4Ret;
}

#ifdef CFG_DUMP_TXPOWR_TABLE
#define TXPWR_TABLE_ENTRY(_siso_mcs, _cdd_mcs, _mimo_mcs, _idx)     \
    {                                                               \
        .mcs[STREAM_SISO] = _siso_mcs, .mcs[STREAM_CDD] = _cdd_mcs, \
        .mcs[STREAM_MIMO] = _mimo_mcs, .idx = (_idx),               \
    }

static struct txpwr_table_entry dsss[] = {
    TXPWR_TABLE_ENTRY("DSSS1", "", "", PWR_DSSS_CCK),
    TXPWR_TABLE_ENTRY("DSSS2", "", "", PWR_DSSS_CCK),
    TXPWR_TABLE_ENTRY("CCK5", "", "", PWR_DSSS_BPKS),
    TXPWR_TABLE_ENTRY("CCK11", "", "", PWR_DSSS_BPKS),
};

static struct txpwr_table_entry ofdm[] = {
    TXPWR_TABLE_ENTRY("OFDM6", "OFDM6", "", PWR_OFDM_BPSK),
    TXPWR_TABLE_ENTRY("OFDM9", "OFDM9", "", PWR_OFDM_BPSK),
    TXPWR_TABLE_ENTRY("OFDM12", "OFDM12", "", PWR_OFDM_QPSK),
    TXPWR_TABLE_ENTRY("OFDM18", "OFDM18", "", PWR_OFDM_QPSK),
    TXPWR_TABLE_ENTRY("OFDM24", "OFDM24", "", PWR_OFDM_16QAM),
    TXPWR_TABLE_ENTRY("OFDM36", "OFDM36", "", PWR_OFDM_16QAM),
    TXPWR_TABLE_ENTRY("OFDM48", "OFDM48", "", PWR_OFDM_48Mbps),
    TXPWR_TABLE_ENTRY("OFDM54", "OFDM54", "", PWR_OFDM_54Mbps),
};

static struct txpwr_table_entry ht[] = {
    TXPWR_TABLE_ENTRY("MCS0", "MCS0", "MCS8", PWR_HT_BPSK),
    TXPWR_TABLE_ENTRY("MCS1", "MCS1", "MCS9", PWR_HT_QPSK),
    TXPWR_TABLE_ENTRY("MCS2", "MCS2", "MCS10", PWR_HT_QPSK),
    TXPWR_TABLE_ENTRY("MCS3", "MCS3", "MCS11", PWR_HT_16QAM),
    TXPWR_TABLE_ENTRY("MCS4", "MCS4", "MCS12", PWR_HT_16QAM),
    TXPWR_TABLE_ENTRY("MCS5", "MCS5", "MCS13", PWR_HT_MCS5),
    TXPWR_TABLE_ENTRY("MCS6", "MCS6", "MCS14", PWR_HT_MCS6),
    TXPWR_TABLE_ENTRY("MCS7", "MCS7", "MCS15", PWR_HT_MCS7),
};

static struct txpwr_table_entry vht[] = {
    TXPWR_TABLE_ENTRY("MCS0", "MCS0", "MCS0", PWR_VHT20_BPSK),
    TXPWR_TABLE_ENTRY("MCS1", "MCS1", "MCS1", PWR_VHT20_QPSK),
    TXPWR_TABLE_ENTRY("MCS2", "MCS2", "MCS2", PWR_VHT20_QPSK),
    TXPWR_TABLE_ENTRY("MCS3", "MCS3", "MCS3", PWR_VHT20_16QAM),
    TXPWR_TABLE_ENTRY("MCS4", "MCS4", "MCS4", PWR_VHT20_16QAM),
    TXPWR_TABLE_ENTRY("MCS5", "MCS5", "MCS5", PWR_VHT20_64QAM),
    TXPWR_TABLE_ENTRY("MCS6", "MCS6", "MCS6", PWR_VHT20_64QAM),
    TXPWR_TABLE_ENTRY("MCS7", "MCS7", "MCS7", PWR_VHT20_MCS7),
    TXPWR_TABLE_ENTRY("MCS8", "MCS8", "MCS8", PWR_VHT20_MCS8),
    TXPWR_TABLE_ENTRY("MCS9", "MCS9", "MCS9", PWR_VHT20_MCS9),
};

static struct txpwr_table txpwr_tables[] = {
    { "Legacy", dsss, ARRAY_SIZE(dsss) }, { "11g", ofdm, ARRAY_SIZE(ofdm) },
    { "11a", ofdm, ARRAY_SIZE(ofdm) }, { "HT20", ht, ARRAY_SIZE(ht) },
    { "HT40", ht, ARRAY_SIZE(ht) }, { "VHT20", vht, ARRAY_SIZE(vht) },
    { "VHT40", vht, ARRAY_SIZE(vht) }, { "VHT80", vht, ARRAY_SIZE(vht) },
};

#define TMP_SZ        (512)
#define CDD_PWR_OFFSET    (6)
#define TXPWR_DUMP_SZ     (8192)
void print_txpwr_tbl(struct txpwr_table *txpwr_tbl, unsigned char ch,
                     unsigned char fe_loss, unsigned char *tx_pwr[],
                     char pwr_offset[], char *stream_buf[],
                     unsigned int stream_pos[])
{
    struct txpwr_table_entry *tmp_tbl = txpwr_tbl->tables;
    unsigned int idx, pwr_idx, stream_idx;
    signed char pwr[TXPWR_TBL_NUM] = { 0 }, tmp_pwr = 0;
    char prefix[5], tmp[4];
    char *buf = NULL;
    unsigned int *pos = NULL;
    int i;

    for (i = 0; i < txpwr_tbl->n_tables; i++) {
        idx = tmp_tbl[i].idx;

        for (pwr_idx = 0; pwr_idx < TXPWR_TBL_NUM; pwr_idx++) {
            if (!tx_pwr[pwr_idx]) {
                DBGLOG(REQ, WARN, "Power table[%d] is NULL\n",
                       pwr_idx);
                return;
            }
            pwr[pwr_idx] =
                tx_pwr[pwr_idx][idx] + pwr_offset[pwr_idx];
            pwr[pwr_idx] = (pwr[pwr_idx] > MAX_TX_POWER) ?
                           MAX_TX_POWER :
                           pwr[pwr_idx];
        }

        for (stream_idx = 0; stream_idx < STREAM_NUM; stream_idx++) {
            buf = stream_buf[stream_idx];
            pos = &stream_pos[stream_idx];

            if (tmp_tbl[i].mcs[stream_idx][0] == '\0') {
                continue;
            }

            switch (stream_idx) {
            case STREAM_SISO:
                kalStrnCpy(prefix, "siso", sizeof(prefix));
                break;

            case STREAM_CDD:
                kalStrnCpy(prefix, "cdd", sizeof(prefix));
                break;

            case STREAM_MIMO:
                kalStrnCpy(prefix, "mimo", sizeof(prefix));
                break;
            }

            *pos += kalScnprintf(buf + *pos, TMP_SZ - *pos,
                                 "%s, %d, %s, %s, ", prefix, ch,
                                 txpwr_tbl->phy_mode,
                                 tmp_tbl[i].mcs[stream_idx]);

            for (pwr_idx = 0; pwr_idx < TXPWR_TBL_NUM; pwr_idx++) {
                tmp_pwr = pwr[pwr_idx];

                tmp_pwr = (tmp_pwr > 0) ? tmp_pwr : 0;

                if (pwr_idx + 1 == TXPWR_TBL_NUM) {
                    kalStrnCpy(tmp, "\n", sizeof(tmp));
                }else{
                    kalStrnCpy(tmp, ", ", sizeof(tmp));
                }
                *pos += kalScnprintf(buf + *pos, TMP_SZ - *pos,
                                     "%d.%d%s", tmp_pwr / 2,
                                     tmp_pwr % 2 * 5, tmp);

                /* print power limit with front-end loss */
                if (pwr_idx == LIMIT_TBL) {
                    tmp_pwr += fe_loss;
                    tmp_pwr = (tmp_pwr > MAX_TX_POWER) ?
                              MAX_TX_POWER :
                              tmp_pwr;
                    *pos += kalScnprintf(
                        buf + *pos, TMP_SZ - *pos,
                        "%d.%d%s", tmp_pwr / 2,
                        tmp_pwr % 2 * 5, tmp);
                }
            }
        }
    }
}

char *g_txpwr_tbl_read_buffer = NULL;
unsigned int g_txpwr_tbl_read_residual = 0;

static ssize_t procGetTxpwrTblRead(struct file *filp, char __user *buf,
                                   size_t count, loff_t *f_pos)
{
    P_GLUE_INFO_T prGlueInfo = NULL;
    P_ADAPTER_T prAdapter = NULL;
    P_BSS_INFO_T prBssInfo = NULL;
    unsigned char ucBssIndex;
    P_NETDEV_PRIVATE_GLUE_INFO prNetDevPrivate = NULL;
    WLAN_STATUS status;
    struct PARAM_CMD_GET_TXPWR_TBL pwr_tbl;
    struct POWER_LIMIT *tx_pwr_tbl = pwr_tbl.tx_pwr_tbl;
    char *buffer;
    unsigned int pos = 0, buf_len = TXPWR_DUMP_SZ, oid_len;
    unsigned char i, j;
    char *stream_buf[STREAM_NUM] = { NULL };
    unsigned int stream_pos[STREAM_NUM] = { 0 };
    unsigned char *tx_pwr[TXPWR_TBL_NUM] = { NULL };
    char pwr_offset[TXPWR_TBL_NUM] = { 0 };
    unsigned char offset = 0;
    int ret;

    if (*f_pos > 0) {  /* re-entry */
        pos = g_txpwr_tbl_read_residual;
        buffer = g_txpwr_tbl_read_buffer;
        goto next_entry;
    }

    prGlueInfo = g_prGlueInfo_proc;
    if (!prGlueInfo) {
        return -EFAULT;
    }

    prAdapter = prGlueInfo->prAdapter;
    prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(gPrDev);
    if (prNetDevPrivate->prGlueInfo != prGlueInfo) {
        return -EFAULT;
    }

    ucBssIndex = prNetDevPrivate->ucBssIdx;
    prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
    if (!prBssInfo) {
        return -EFAULT;
    }

    kalMemZero(&pwr_tbl, sizeof(pwr_tbl));

    if (prAdapter->rWifiVar.fgDbDcModeEn) {
        pwr_tbl.ucDbdcIdx = prBssInfo->eDBDCBand;
    }else{
        pwr_tbl.ucDbdcIdx = ENUM_BAND_0;
    }

    status = kalIoctl(prGlueInfo, wlanoidGetTxPwrTbl, &pwr_tbl,
                      sizeof(pwr_tbl), true, false, true, &oid_len);

    if (status != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "Query Tx Power Table fail\n");
        return -EINVAL;
    }

    buffer = (char *)kalMemAlloc(buf_len, VIR_MEM_TYPE);
    if (!buffer) {
        return -ENOMEM;
    }

    g_txpwr_tbl_read_buffer = buffer;

    for (i = 0; i < STREAM_NUM; i++) {
        stream_buf[i] = (char *)kalMemAlloc(TMP_SZ, VIR_MEM_TYPE);
        if (!stream_buf[i]) {
            ret = -ENOMEM;
            goto out;
        }
    }

    pos = kalScnprintf(buffer, buf_len, "\n%s",
                       "spatial_stream, channel, bw, modulation, ");
    pos += kalScnprintf(
        buffer + pos, buf_len - pos, "%s",
        "regulatory_limit, regulatory_limit_with_fe_loss, ");
    pos += kalScnprintf(buffer + pos, buf_len - pos, "%s\n",
                        "board_limit, target_power");

    for (i = 0; i < ARRAY_SIZE(txpwr_tables); i++) {
        for (j = 0; j < STREAM_NUM; j++) {
            kalMemZero(stream_buf[j], TMP_SZ);
            stream_pos[j] = 0;
        }

        for (j = 0; j < TXPWR_TBL_NUM; j++) {
            tx_pwr[j] = NULL;
            pwr_offset[j] = 0;
        }

        switch (i) {
        case DSSS:
            if (pwr_tbl.ucCenterCh > 14) {
                continue;
            }
            for (j = 0; j < TXPWR_TBL_NUM; j++)
                tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_dsss;
            break;

        case OFDM_24G:
            if (pwr_tbl.ucCenterCh > 14) {
                continue;
            }
            for (j = 0; j < TXPWR_TBL_NUM; j++)
                tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_ofdm;
            break;

        case OFDM_5G:
            if (pwr_tbl.ucCenterCh <= 14) {
                continue;
            }
            for (j = 0; j < TXPWR_TBL_NUM; j++)
                tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_ofdm;
            break;

        case HT20:
            for (j = 0; j < TXPWR_TBL_NUM; j++)
                tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_ht20;
            break;

        case HT40:
            for (j = 0; j < TXPWR_TBL_NUM; j++)
                tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_ht40;
            break;

        case VHT20:
            if (pwr_tbl.ucCenterCh <= 14) {
                continue;
            }
            for (j = 0; j < TXPWR_TBL_NUM; j++)
                tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_vht20;
            break;

        case VHT40:
        case VHT80:
            if (pwr_tbl.ucCenterCh <= 14) {
                continue;
            }
            offset = (i == VHT40) ? PWR_Vht40_OFFSET :
                     PWR_Vht80_OFFSET;
            for (j = 0; j < TXPWR_TBL_NUM; j++) {
                tx_pwr[j] = tx_pwr_tbl[j].tx_pwr_vht20;
                pwr_offset[j] =
                    tx_pwr_tbl[j].tx_pwr_vht_OFST[offset];
                /* Covert 7bit 2'complement value to 8bit */
                pwr_offset[j] |=
                    (pwr_offset[j] & BIT(6)) ? BIT(7) : 0;
            }
            break;

        default:
            break;
        }

        print_txpwr_tbl(&txpwr_tables[i], pwr_tbl.ucCenterCh,
                        pwr_tbl.ucFeLoss, tx_pwr, pwr_offset,
                        stream_buf, stream_pos);

        for (j = 0; j < STREAM_NUM; j++)
            pos += kalScnprintf(buffer + pos, buf_len - pos, "%s",
                                stream_buf[j]);
    }
    g_txpwr_tbl_read_residual = pos;

next_entry:
    if (pos > count) {
        pos = count;
    }
    if (copy_to_user(buf, buffer, pos)) {
        DBGLOG(INIT, ERROR, "copy to user failed\n");
        ret = -EFAULT;
        goto out;
    }
    g_txpwr_tbl_read_buffer += pos;
    g_txpwr_tbl_read_residual -= pos;
    *f_pos += pos;
    ret = pos;
out:
    if (ret == 0 || ret == -ENOMEM) {
        for (i = 0; i < STREAM_NUM; i++)
            if (stream_buf[i]) {
                kalMemFree(stream_buf[i], VIR_MEM_TYPE, TMP_SZ);
            }
        if (buffer) {
            kalMemFree(buffer, VIR_MEM_TYPE, buf_len);
        }
        g_txpwr_tbl_read_buffer = NULL;
        g_txpwr_tbl_read_residual = 0;
    }
    return ret;
}
#endif

#if KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE
static const struct proc_ops dbglevel_ops = {
    .proc_read = procDbgLevelRead,
    .proc_write = procDbgLevelWrite,
};

#if WLAN_INCLUDE_PROC
#if CFG_SUPPORT_EASY_DEBUG
static const struct proc_ops efusedump_ops = {
    .proc_open = procEfuseDumpOpen,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release,
};

static const struct proc_ops drivercmd_ops = {
    .proc_read = procDriverCmdRead,
    .proc_write = procDriverCmdWrite,
};

static const struct proc_ops cfg_ops = {
    .proc_read = procCfgRead,
    .proc_write = procCfgWrite,
};
#endif
#endif

#ifdef CFG_DUMP_TXPOWR_TABLE
static const struct proc_ops get_txpwr_tbl_ops = {
    .proc_read = procGetTxpwrTblRead,
};
#endif
#else

static const struct file_operations dbglevel_ops = {
    .owner = THIS_MODULE,
    .read = procDbgLevelRead,
    .write = procDbgLevelWrite,
};

#if WLAN_INCLUDE_PROC
#if CFG_SUPPORT_EASY_DEBUG

static const struct file_operations efusedump_ops = {
    .owner = THIS_MODULE,
    .open = procEfuseDumpOpen,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
};

static const struct file_operations drivercmd_ops = {
    .owner = THIS_MODULE,
    .read = procDriverCmdRead,
    .write = procDriverCmdWrite,
};

static const struct file_operations cfg_ops = {
    .owner = THIS_MODULE,
    .read = procCfgRead,
    .write = procCfgWrite,
};
#endif
#endif

#ifdef CFG_DUMP_TXPOWR_TABLE
static const struct file_operations get_txpwr_tbl_ops = {
    .owner = THIS_MODULE,
    .read = procGetTxpwrTblRead,
};
#endif
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief The PROC function for reading MCR register to User Space, the offset
 * of the MCR is specified in u4McrOffset.
 *
 * \param[in] page       Buffer provided by kernel.
 * \param[in out] start  Start Address to read(3 methods).
 * \param[in] off        Offset.
 * \param[in] count      Allowable number to read.
 * \param[out] eof       End of File indication.
 * \param[in] data       Pointer to the private data structure.
 *
 * \return number of characters print to the buffer from User Space.
 */
/*----------------------------------------------------------------------------*/
static ssize_t procMCRRead(struct file *filp, char __user *buf, size_t count,
                           loff_t *f_pos)
{
    u8 *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
    P_GLUE_INFO_T prGlueInfo;
    PARAM_CUSTOM_MCR_RW_STRUCT_T rMcrInfo;
    u32 u4BufLen;
    u32 u4CopySize = 0;
    u8 *temp = NULL;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    s32 i4Ret = 0;

    /*if *f_ops>0, we should return 0 to make cat command exit*/
    if (*f_pos > 0 || buf == NULL || pucProcBuf == NULL) {
        i4Ret = 0;
        goto freeBuf;
    }

    temp = pucProcBuf;
    prGlueInfo = g_prGlueInfo_proc;

    rMcrInfo.u4McrOffset = u4McrOffset;

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryMcrRead, (void *)&rMcrInfo,
                       sizeof(rMcrInfo), true, true, true, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(INIT, ERROR, "[%s] kalIoctl failed\n", __func__);
        i4Ret = -EFAULT;
        goto freeBuf;
    }

    u4CopySize = scnprintf(temp, PROC_MAX_BUF_SIZE - kalStrLen(pucProcBuf),
                           "MCR (0x%08xh): 0x%08x\n", rMcrInfo.u4McrOffset,
                           rMcrInfo.u4McrData);

    if (u4CopySize > count) {
        u4CopySize = count;
    }

    if (copy_to_user(buf, pucProcBuf, u4CopySize)) {
        DBGLOG(INIT, ERROR, "copy to user failed\n");
        i4Ret = -EFAULT;
        goto freeBuf;
    }

    *f_pos += u4CopySize;
    i4Ret = u4CopySize;
freeBuf:
    if (pucProcBuf) {
        kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
    }
    return i4Ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief The PROC function for writing MCR register to HW or update u4McrOffset
 *        for reading MCR later.
 *
 * \param[in] file   pointer to file.
 * \param[in] buffer Buffer from user space.
 * \param[in] count  Number of characters to write
 * \param[in] data   Pointer to the private data structure.
 *
 * \return number of characters write from User Space.
 */
/*----------------------------------------------------------------------------*/
static ssize_t procMCRWrite(struct file *file, const char __user *buffer,
                            size_t count, loff_t *data)
{
    P_GLUE_INFO_T prGlueInfo;
    char acBuf[PROC_MCR_ACCESS_MAX_USER_INPUT_LEN + 1];  /* + 1 for "\0" */
    u32 u4CopySize = 0;
    PARAM_CUSTOM_MCR_RW_STRUCT_T rMcrInfo;
    u32 u4BufLen;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    int num = 0;

    ASSERT(data);

    u4CopySize = (count < sizeof(acBuf)) ? count : (sizeof(acBuf) - 1);
    if (copy_from_user(acBuf, buffer, u4CopySize)) {
        DBGLOG(INIT, ERROR, "error of copy from user\n");
        return -EFAULT;
    }
    acBuf[u4CopySize] = '\0';

    num = sscanf(acBuf, "0x%x 0x%x", &rMcrInfo.u4McrOffset,
                 &rMcrInfo.u4McrData);
    switch (num) {
    case 2:
        /* NOTE: Sometimes we want to test if bus will still be ok,
         * after accessing the MCR which is not align to DW boundary.
         */
        /* if (IS_ALIGN_4(rMcrInfo.u4McrOffset)) */
    {
        prGlueInfo = g_prGlueInfo_proc;
        u4McrOffset = rMcrInfo.u4McrOffset;

        /* rMcrInfo.u4McrOffset, rMcrInfo.u4McrData); */

        rStatus = kalIoctl(prGlueInfo, wlanoidSetMcrWrite,
                           (void *)&rMcrInfo, sizeof(rMcrInfo),
                           false, false, true, &u4BufLen);
    }
    break;

    case 1:
        /* if (IS_ALIGN_4(rMcrInfo.u4McrOffset)) */
    {
        u4McrOffset = rMcrInfo.u4McrOffset;
    }
    break;

    default:
        break;
    }

    return u4CopySize;
}

#if KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE
static const struct proc_ops mcr_ops = {
    .proc_read = procMCRRead,
    .proc_write = procMCRWrite,
};
#else
static const struct file_operations mcr_ops = {
    .owner = THIS_MODULE,
    .read = procMCRRead,
    .write = procMCRWrite,
};
#endif

#if CFG_SUPPORT_DEBUG_FS
static ssize_t procCountryRead(struct file *filp, char __user *buf,
                               size_t count, loff_t *f_pos)
{
    u8 *pucProcBuf = kalMemZAlloc(PROC_MAX_BUF_SIZE, VIR_MEM_TYPE);
    u32 u4CopySize;
    u32 country = 0;
    s32 i4Ret = 0;

    /* if *f_pos > 0, it means has read successed last time, don't try again
     */
    if (*f_pos > 0 || buf == NULL || pucProcBuf == NULL) {
        i4Ret = 0;
        goto freeBuf;
    }
    if (!regd_is_single_sku_en()) {
        kalStrCpy(pucProcBuf,
                  "Country Code is controlled by Local DB\n");
    }

    country = rlmDomainGetCountryCode();

    if (country) {
        kalSprintf(pucProcBuf, "Current Country Code: %s\n", &country);
    }else{
        kalStrCpy(pucProcBuf, "Current Country Code: NULL\n");
    }

    u4CopySize = kalStrLen(pucProcBuf);

    if (u4CopySize > count) {
        u4CopySize = count;
    }

    if (copy_to_user(buf, pucProcBuf, u4CopySize)) {
        DBGLOG(INIT, ERROR, "copy to user failed\n");
        i4Ret = -EFAULT;
        goto freeBuf;
    }
    *f_pos += u4CopySize;
    i4Ret = u4CopySize;
freeBuf:
    if (pucProcBuf) {
        kalMemFree(pucProcBuf, VIR_MEM_TYPE, PROC_MAX_BUF_SIZE);
    }
    return i4Ret;
}

#if KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE
static const struct proc_ops country_ops = {
    .proc_read = procCountryRead,
};
#else
static const struct file_operations country_ops = {
    .owner = THIS_MODULE,
    .read = procCountryRead,
};
#endif
#endif

s32 procInitFs(void)
{
    g_i4NextDriverReadLen = 0;

    if (init_net.proc_net == (struct proc_dir_entry *)NULL) {
        DBGLOG(INIT, ERROR, "init proc fs fail: proc_net == NULL\n");
        return -ENOENT;
    }

    /*
     * Directory: Root (/proc/net/wlan0)
     */

    gprProcRoot = proc_mkdir(PROC_ROOT_NAME, init_net.proc_net);
    if (!gprProcRoot) {
        DBGLOG(INIT, ERROR, "gprProcRoot == NULL\n");
        return -ENOENT;
    }
    proc_set_user(gprProcRoot, KUIDT_INIT(PROC_UID_SHELL),
                  KGIDT_INIT(PROC_GID_WIFI));

    return 0;
}

s32 procUninitProcFs(void)
{
    remove_proc_subtree(PROC_ROOT_NAME, init_net.proc_net);
    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function clean up a PROC fs created by procInitProcfs().
 *
 * \param[in] prDev      Pointer to the struct net_device.
 * \param[in] pucDevName Pointer to the name of net_device.
 *
 * \return N/A
 */
/*----------------------------------------------------------------------------*/
s32 procRemoveProcfs(void)
{
    remove_proc_entry(PROC_MCR_ACCESS, gprProcRoot);
    remove_proc_entry(PROC_DRIVER_CMD, gprProcRoot);
    remove_proc_entry(PROC_DBG_LEVEL_NAME, gprProcRoot);
    remove_proc_entry(PROC_CFG, gprProcRoot);
    remove_proc_entry(PROC_EFUSE_DUMP, gprProcRoot);
#ifdef CFG_DUMP_TXPOWR_TABLE
    remove_proc_entry(PROC_GET_TXPWR_TBL, gprProcRoot);
#endif
#if CFG_SUPPORT_DEBUG_FS
    remove_proc_entry(PROC_COUNTRY, gprProcRoot);
#endif

    return 0;
}

s32 procCreateFsEntry(P_GLUE_INFO_T prGlueInfo)
{
    struct proc_dir_entry *prEntry;

    DBGLOG(INIT, INFO, "[%s]\n", __func__);
    g_prGlueInfo_proc = prGlueInfo;

    prEntry = proc_create(PROC_MCR_ACCESS, 0664, gprProcRoot, &mcr_ops);
    if (prEntry == NULL) {
        DBGLOG(INIT, ERROR, "Unable to create /proc entry\n\r");
        return -1;
    }
#if CFG_SUPPORT_DEBUG_FS
    prEntry = proc_create(PROC_COUNTRY, 0664, gprProcRoot, &country_ops);
    if (prEntry == NULL) {
        DBGLOG(INIT, ERROR, "Unable to create /proc entry\n\r");
        return -1;
    }
#endif
#if WLAN_INCLUDE_PROC
#if CFG_SUPPORT_EASY_DEBUG
    prEntry =
        proc_create(PROC_DRIVER_CMD, 0664, gprProcRoot, &drivercmd_ops);
    if (prEntry == NULL) {
        DBGLOG(INIT, ERROR,
               "Unable to create /proc entry for driver command\n\r");
        return -1;
    }

    prEntry = proc_create(PROC_CFG, 0664, gprProcRoot, &cfg_ops);
    if (prEntry == NULL) {
        DBGLOG(INIT, ERROR,
               "Unable to create /proc entry for driver command\n\r");
        return -1;
    }

    prEntry =
        proc_create(PROC_EFUSE_DUMP, 0664, gprProcRoot, &efusedump_ops);
    if (prEntry == NULL) {
        DBGLOG(INIT, ERROR, "Unable to create /proc entry efuse\n\r");
        return -1;
    }
#endif
#endif

#ifdef CFG_DUMP_TXPOWR_TABLE
    prEntry = proc_create(PROC_GET_TXPWR_TBL, 0664, gprProcRoot,
                          &get_txpwr_tbl_ops);
    if (prEntry == NULL) {
        DBGLOG(INIT, ERROR, "Unable to create /proc entry efuse\n\r");
        return -1;
    }
#endif

    prEntry = proc_create(PROC_DBG_LEVEL_NAME, 0664, gprProcRoot,
                          &dbglevel_ops);
    if (prEntry == NULL) {
        DBGLOG(INIT, ERROR,
               "Unable to create /proc entry dbgLevel\n\r");
        return -1;
    }

    return 0;
}
