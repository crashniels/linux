/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_kal.h
 *    \brief  Declaration of KAL functions - kal*() which is provided by GLUE
 * Layer.
 *
 *    Any definitions in this file will be shared among GLUE Layer and internal
 * Driver Stack.
 */

#ifndef _GL_KAL_H
#define _GL_KAL_H

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
#include "gl_wext_priv.h"
#include "link.h"
#include "nic/mac.h"
#include "nic/wlan_def.h"
#include "wlan_lib.h"
#include "wlan_oid.h"

#include "linux/kallsyms.h"
/*#include <linux/ftrace_event.h>*/

#if DBG
extern int allocatedMemSize;
#endif

extern struct semaphore g_halt_sem;
extern int g_u4HaltFlag;

extern struct delayed_work sched_workq;
extern struct delayed_work wdev_lock_workq;

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/* Define how many concurrent operation networks. */
#define KAL_BSS_NUM          4

#define KAL_AIS_NUM          1

#if CFG_DUAL_P2PLIKE_INTERFACE
#define KAL_P2P_NUM          2
#else
#define KAL_P2P_NUM          1
#endif

#define GLUE_FLAG_MAIN_PROCESS                                     \
    (GLUE_FLAG_HALT | GLUE_FLAG_SUB_MOD_MULTICAST |                \
     GLUE_FLAG_TX_CMD_DONE | GLUE_FLAG_TXREQ | GLUE_FLAG_TIMEOUT | \
     GLUE_FLAG_FRAME_FILTER | GLUE_FLAG_OID | GLUE_FLAG_RX)

#define GLUE_FLAG_HIF_PROCESS                            \
    (GLUE_FLAG_HALT | GLUE_FLAG_INT | GLUE_FLAG_HIF_TX | \
     GLUE_FLAG_HIF_TX_CMD | GLUE_FLAG_HIF_FW_OWN)

#define GLUE_FLAG_RX_PROCESS         (GLUE_FLAG_HALT | GLUE_FLAG_RX_TO_OS)

#if CFG_SUPPORT_SNIFFER
#define RADIOTAP_FIELD_TSFT      BIT(0)
#define RADIOTAP_FIELD_FLAGS         BIT(1)
#define RADIOTAP_FIELD_RATE      BIT(2)
#define RADIOTAP_FIELD_CHANNEL       BIT(3)
#define RADIOTAP_FIELD_ANT_SIGNAL    BIT(5)
#define RADIOTAP_FIELD_ANT_NOISE     BIT(6)
#define RADIOTAP_FIELD_ANT       BIT(11)
#define RADIOTAP_FIELD_MCS       BIT(19)
#define RADIOTAP_FIELD_AMPDU         BIT(20)
#define RADIOTAP_FIELD_VHT       BIT(21)
#define RADIOTAP_FIELD_VENDOR        BIT(30)

#define RADIOTAP_LEN_VHT         48
#define RADIOTAP_FIELDS_VHT                                             \
    (RADIOTAP_FIELD_TSFT | RADIOTAP_FIELD_FLAGS | RADIOTAP_FIELD_RATE | \
     RADIOTAP_FIELD_CHANNEL | RADIOTAP_FIELD_ANT_SIGNAL |               \
     RADIOTAP_FIELD_ANT_NOISE | RADIOTAP_FIELD_ANT |                    \
     RADIOTAP_FIELD_AMPDU | RADIOTAP_FIELD_VHT | RADIOTAP_FIELD_VENDOR)

#define RADIOTAP_LEN_HT          36
#define RADIOTAP_FIELDS_HT                                                \
    (RADIOTAP_FIELD_TSFT | RADIOTAP_FIELD_FLAGS | RADIOTAP_FIELD_RATE |   \
     RADIOTAP_FIELD_CHANNEL | RADIOTAP_FIELD_ANT_SIGNAL |                 \
     RADIOTAP_FIELD_ANT_NOISE | RADIOTAP_FIELD_ANT | RADIOTAP_FIELD_MCS | \
     RADIOTAP_FIELD_AMPDU | RADIOTAP_FIELD_VENDOR)

#define RADIOTAP_LEN_LEGACY      26
#define RADIOTAP_FIELDS_LEGACY                                          \
    (RADIOTAP_FIELD_TSFT | RADIOTAP_FIELD_FLAGS | RADIOTAP_FIELD_RATE | \
     RADIOTAP_FIELD_CHANNEL | RADIOTAP_FIELD_ANT_SIGNAL |               \
     RADIOTAP_FIELD_ANT_NOISE | RADIOTAP_FIELD_ANT |                    \
     RADIOTAP_FIELD_VENDOR)
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

typedef enum _ENUM_SPIN_LOCK_CATEGORY_E {
    SPIN_LOCK_FSM = 0,
    SPIN_LOCK_TX_PORT_QUE,
    SPIN_LOCK_TX_CMD_QUE,
    SPIN_LOCK_TX_CMD_DONE_QUE,
    SPIN_LOCK_TC_RESOURCE,
    SPIN_LOCK_RX_TO_OS_QUE,
    SPIN_LOCK_WDEV_LOCK,

    /* FIX ME */
    SPIN_LOCK_RX_QUE,
    SPIN_LOCK_RX_FREE_QUE,
    SPIN_LOCK_TX_QUE,
    SPIN_LOCK_CMD_QUE,
    SPIN_LOCK_TX_RESOURCE,
    SPIN_LOCK_CMD_RESOURCE,
    SPIN_LOCK_QM_TX_QUEUE,
    SPIN_LOCK_CMD_PENDING,
    SPIN_LOCK_CMD_SEQ_NUM,
    SPIN_LOCK_TX_MSDU_INFO_LIST,
    SPIN_LOCK_TXING_MGMT_LIST,
    SPIN_LOCK_TX_SEQ_NUM,
    SPIN_LOCK_TX_COUNT,
    SPIN_LOCK_TXS_COUNT,
    /* end    */
    SPIN_LOCK_TX,
    /* TX/RX Direct : BEGIN */
    SPIN_LOCK_TX_DIRECT,
    SPIN_LOCK_TX_DESC,
    SPIN_LOCK_RX_DIRECT_REORDER,
    /* TX/RX Direct : END */
    SPIN_LOCK_IO_REQ,
    SPIN_LOCK_INT,

    SPIN_LOCK_MGT_BUF,
    SPIN_LOCK_MSG_BUF,
    SPIN_LOCK_STA_REC,

    SPIN_LOCK_MAILBOX,
    SPIN_LOCK_TIMER,

    SPIN_LOCK_BOW_TABLE,

    SPIN_LOCK_EHPI_BUS,  /* only for EHPI */
    SPIN_LOCK_NET_DEV,
    SPIN_LOCK_CHIP_RST,
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
    SPIN_LOCK_TX_LB_QUE,
    SPIN_LOCK_RX_LB_QUE,
#endif
    SPIN_LOCK_NUM
} ENUM_SPIN_LOCK_CATEGORY_E;

typedef enum _ENUM_MUTEX_CATEGORY_E {
    MUTEX_TX_CMD_CLEAR,
    MUTEX_TX_DATA_DONE_QUE,
    MUTEX_DEL_INF,
    MUTEX_DEL_P2P_VIF,
    MUTEX_NUM
} ENUM_MUTEX_CATEGORY_E;

/* event for assoc information update */
typedef struct _EVENT_ASSOC_INFO {
    u8 ucAssocReq;  /* 1 for assoc req, 0 for assoc rsp */
    u8 ucReassoc;  /* 0 for assoc, 1 for reassoc */
    u16 u2Length;
    u8 *pucIe;
} EVENT_ASSOC_INFO, *P_EVENT_ASSOC_INFO;

typedef enum _ENUM_KAL_NETWORK_TYPE_INDEX_T {
    KAL_NETWORK_TYPE_AIS_INDEX = 0,
#if CFG_ENABLE_WIFI_DIRECT
    KAL_NETWORK_TYPE_P2P_INDEX,
#endif

    KAL_NETWORK_TYPE_INDEX_NUM
} ENUM_KAL_NETWORK_TYPE_INDEX_T;

typedef enum _ENUM_KAL_MEM_ALLOCATION_TYPE_E {
    PHY_MEM_TYPE,  /* physically continuous */
    VIR_MEM_TYPE,  /* virtually continuous */
    MEM_TYPE_NUM
} ENUM_KAL_MEM_ALLOCATION_TYPE;

typedef enum _ENUM_MTK_AGPS_ATTR {
    MTK_ATTR_AGPS_INVALID,
    MTK_ATTR_AGPS_CMD,
    MTK_ATTR_AGPS_DATA,
    MTK_ATTR_AGPS_IFINDEX,
    MTK_ATTR_AGPS_IFNAME,
    MTK_ATTR_AGPS_MAX
} ENUM_MTK_CCX_ATTR;

typedef enum _ENUM_AGPS_EVENT {
    AGPS_EVENT_WLAN_ON,
    AGPS_EVENT_WLAN_OFF,
    AGPS_EVENT_WLAN_AP_LIST,
} ENUM_CCX_EVENT;
u8 kalIndicateAgpsNotify(P_ADAPTER_T prAdapter, u8 cmd, u8 *data, u16 dataLen);

#if CFG_SUPPORT_SNIFFER
/* Vendor Namespace
 * Bit Number 30
 * Required Alignment 2 bytes
 */
typedef struct _RADIOTAP_FIELD_VENDOR_T {
    u8 aucOUI[3];
    u8 ucSubNamespace;
    u16 u2DataLen;
    u8 ucData;
} __packed RADIOTAP_FIELD_VENDOR_T, *P_RADIOTAP_FIELD_VENDOR_T;

typedef struct _MONITOR_RADIOTAP_T {
    /* radiotap header */
    u8 ucItVersion;  /* set to 0 */
    u8 ucItPad;
    u16 u2ItLen;  /* entire length */
    u32 u4ItPresent;  /* fields present */

    /* TSFT
     * Bit Number 0
     * Required Alignment 8 bytes
     * Unit microseconds
     */
    u64 u8MacTime;

    /* Flags
     * Bit Number 1
     */
    u8 ucFlags;

    /* Rate
     * Bit Number 2
     * Unit 500 Kbps
     */
    u8 ucRate;

    /* Channel
     * Bit Number 3
     * Required Alignment 2 bytes
     */
    u16 u2ChFrequency;
    u16 u2ChFlags;

    /* Antenna signal
     * Bit Number 5
     * Unit dBm
     */
    u8 ucAntennaSignal;

    /* Antenna noise
     * Bit Number 6
     * Unit dBm
     */
    u8 ucAntennaNoise;

    /* Antenna
     * Bit Number 11
     * Unit antenna index
     */
    u8 ucAntenna;

    /* MCS
     * Bit Number 19
     * Required Alignment 1 byte
     */
    u8 ucMcsKnown;
    u8 ucMcsFlags;
    u8 ucMcsMcs;

    /* A-MPDU status
     * Bit Number 20
     * Required Alignment 4 bytes
     */
    u32 u4AmpduRefNum;
    u16 u2AmpduFlags;
    u8 ucAmpduDelimiterCRC;
    u8 ucAmpduReserved;

    /* VHT
     * Bit Number 21
     * Required Alignment 2 bytes
     */
    u16 u2VhtKnown;
    u8 ucVhtFlags;
    u8 ucVhtBandwidth;
    u8 aucVhtMcsNss[4];
    u8 ucVhtCoding;
    u8 ucVhtGroupId;
    u16 u2VhtPartialAid;

    /* extension space */
    u8 aucReserve[12];
} __packed MONITOR_RADIOTAP_T, *P_MONITOR_RADIOTAP_T;
#endif

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

/*----------------------------------------------------------------------------*/
/* Macros of getting current thread id                                        */
/*----------------------------------------------------------------------------*/
#define KAL_GET_CURRENT_THREAD_ID()  (current->pid)
#define KAL_GET_CURRENT_THREAD_NAME()    (current->comm)

/*----------------------------------------------------------------------------*/
/* Macros of SPIN LOCK operations for using in Driver Layer                   */
/*----------------------------------------------------------------------------*/
#define KAL_SPIN_LOCK_DECLARATION()  unsigned long __ulFlags

#define KAL_ACQUIRE_SPIN_LOCK(_prAdapter, _rLockCategory)     \
    kalAcquireSpinLock(((P_ADAPTER_T)_prAdapter)->prGlueInfo, \
                       _rLockCategory, &__ulFlags)

#define KAL_RELEASE_SPIN_LOCK(_prAdapter, _rLockCategory)     \
    kalReleaseSpinLock(((P_ADAPTER_T)_prAdapter)->prGlueInfo, \
                       _rLockCategory, __ulFlags)

/*----------------------------------------------------------------------------*/
/* Macros of MUTEX operations for using in Driver Layer                   */
/*----------------------------------------------------------------------------*/
#define KAL_ACQUIRE_MUTEX(_prAdapter, _rLockCategory) \
    kalAcquireMutex(((P_ADAPTER_T)_prAdapter)->prGlueInfo, _rLockCategory)

#define KAL_RELEASE_MUTEX(_prAdapter, _rLockCategory) \
    kalReleaseMutex(((P_ADAPTER_T)_prAdapter)->prGlueInfo, _rLockCategory)

/*----------------------------------------------------------------------------*/
/* Macros for accessing Reserved Fields of native packet                      */
/*----------------------------------------------------------------------------*/
#define KAL_GET_PKT_QUEUE_ENTRY(_p) GLUE_GET_PKT_QUEUE_ENTRY(_p)
#define KAL_GET_PKT_DESCRIPTOR(_prQueueEntry) \
    GLUE_GET_PKT_DESCRIPTOR(_prQueueEntry)
#define KAL_GET_PKT_TID(_p)     GLUE_GET_PKT_TID(_p)
#define KAL_GET_PKT_IS1X(_p)        GLUE_GET_PKT_IS1X(_p)
#define KAL_GET_PKT_HEADER_LEN(_p)  GLUE_GET_PKT_HEADER_LEN(_p)
#define KAL_GET_PKT_PAYLOAD_LEN(_p) GLUE_GET_PKT_PAYLOAD_LEN(_p)
#define KAL_GET_PKT_ARRIVAL_TIME(_p)    GLUE_GET_PKT_ARRIVAL_TIME(_p)

/*----------------------------------------------------------------------------*/
/* Macros for kernel related defines                      */
/*----------------------------------------------------------------------------*/

/**
 * kalCfg80211ToMtkBand - Band translation helper
 *
 * @band: cfg80211_band
 *
 * Translates cfg80211 band into internal band definition
 */
#if CFG_SCAN_CHANNEL_SPECIFIED
#define kalCfg80211ToMtkBand(cfg80211_band)          \
    (cfg80211_band == NL80211_BAND_2GHZ ? BAND_2G4 : \
     cfg80211_band == NL80211_BAND_5GHZ ? BAND_5G :  \
     BAND_NULL)
#endif

/**
 * kalCfg80211ScanDone - abstraction of cfg80211_scan_done
 *
 * @request: the corresponding scan request (sanity checked by callers!)
 * @aborted: set to true if the scan was aborted for any reason,
 * userspace will be notified of that
 *
 * Since linux-4.8.y the 2nd parameter is changed from bool to
 * struct cfg80211_scan_info, but we don't use all fields yet.
 */
static inline void kalCfg80211ScanDone(struct cfg80211_scan_request *request,
                                       bool aborted)
{
    struct cfg80211_scan_info info = { .aborted = aborted };
    cfg80211_scan_done(request, &info);
}

/* Consider on some Android platform, using request_firmware_direct()
 * may cause system failed to load firmware. So we still use
 * request_firmware().
 */
#define REQUEST_FIRMWARE(_fw, _name, _dev)    request_firmware(_fw, _name, _dev)

#define RELEASE_FIRMWARE(_fw)  \
    do {                       \
        release_firmware(_fw); \
        _fw = NULL;            \
    } while (0)

/*----------------------------------------------------------------------------*/
/*!
 * \brief Cache memory allocation
 *
 * \param[in] u4Size Required memory size.
 * \param[in] eMemType  Memory allocation type
 *
 * \return Pointer to allocated memory
 *         or NULL
 */
/*----------------------------------------------------------------------------*/
#if DBG
#define kalMemAlloc(u4Size, eMemType)                           \
    ({                                                          \
        void *pvAddr;                                           \
        if (eMemType == PHY_MEM_TYPE) {                         \
            if (in_interrupt())                                 \
            pvAddr = kmalloc(u4Size, GFP_ATOMIC);               \
            else                                                \
            pvAddr = kmalloc(u4Size, GFP_KERNEL);               \
        } else {                                                \
            pvAddr = vmalloc(u4Size);                           \
        }                                                       \
        if (pvAddr) {                                           \
            allocatedMemSize += u4Size;                         \
            DBGLOG(INIT, INFO, "0x%p(%ld) allocated (%s:%s)\n", \
                   pvAddr, (u32)u4Size, __FILE__, __func__);    \
        }                                                       \
        pvAddr;                                                 \
    })
#else
#define kalMemAlloc(u4Size, eMemType)             \
    ({                                            \
        void *pvAddr;                             \
        if (eMemType == PHY_MEM_TYPE) {           \
            if (in_interrupt())                   \
            pvAddr = kmalloc(u4Size, GFP_ATOMIC); \
            else                                  \
            pvAddr = kmalloc(u4Size, GFP_KERNEL); \
        } else {                                  \
            pvAddr = vmalloc(u4Size);             \
        }                                         \
        pvAddr;                                   \
    })
#endif

#define kalMemZAlloc(u4size, eMemType)          \
    ({                                          \
        void *pvAddr;                           \
        pvAddr = kalMemAlloc(u4size, eMemType); \
        if (pvAddr)                             \
        kalMemSet(pvAddr, 0, u4size);           \
        pvAddr;                                 \
    })

/*----------------------------------------------------------------------------*/
/*!
 * \brief Free allocated cache memory
 *
 * \param[in] pvAddr Required memory size.
 * \param[in] eMemType  Memory allocation type
 * \param[in] u4Size Allocated memory size.
 *
 * \return -
 */
/*----------------------------------------------------------------------------*/
#if DBG
#define kalMemFree(pvAddr, eMemType, u4Size)                 \
    {                                                        \
        if (pvAddr) {                                        \
            allocatedMemSize -= u4Size;                      \
            DBGLOG(INIT, INFO, "0x%p(%ld) freed (%s:%s)\n",  \
                   pvAddr, (u32)u4Size, __FILE__, __func__); \
        }                                                    \
        if (eMemType == PHY_MEM_TYPE) {                      \
            kfree(pvAddr);                                   \
        } else {                                             \
            vfree(pvAddr);                                   \
        }                                                    \
    }
#else
#define kalMemFree(pvAddr, eMemType, u4Size) \
    {                                        \
        if (eMemType == PHY_MEM_TYPE) {      \
            kfree(pvAddr);                   \
        } else {                             \
            vfree(pvAddr);                   \
        }                                    \
    }
#endif

#define kalUdelay(u4USec)    udelay(u4USec)

#define kalMdelay(u4MSec)    mdelay(u4MSec)
#define kalMsleep(u4MSec)    msleep(u4MSec)

/* Copy memory from user space to kernel space */
#define kalMemCopyFromUser(_pvTo, _pvFrom, _u4N) \
    copy_from_user(_pvTo, _pvFrom, _u4N)

/* Copy memory from kernel space to user space */
#define kalMemCopyToUser(_pvTo, _pvFrom, _u4N) \
    copy_to_user(_pvTo, _pvFrom, _u4N)

/* Copy memory block with specific size */
#define kalMemCopy(pvDst, pvSrc, u4Size)    memcpy(pvDst, pvSrc, u4Size)

/* Set memory block with specific pattern */
#define kalMemSet(pvAddr, ucPattern, u4Size)    memset(pvAddr, ucPattern, \
                                                       u4Size)

/* Compare two memory block with specific length.
 * Return zero if they are the same.
 */
#define kalMemCmp(pvAddr1, pvAddr2, u4Size) memcmp(pvAddr1, pvAddr2, u4Size)

/* Zero specific memory block */
#define kalMemZero(pvAddr, u4Size)      memset(pvAddr, 0, u4Size)

/* Move memory block with specific size */
#define kalMemMove(pvDst, pvSrc, u4Size)    memmove(pvDst, pvSrc, u4Size)

#define strnicmp(s1, s2, n)         strncasecmp(s1, s2, n)

/* string operation */
#define kalStrCpy(dest, src)            strcpy(dest, src)
#define kalStrnCpy(dest, src, n)        strncpy(dest, src, n)
#define kalStrCmp(ct, cs)           strcmp(ct, cs)
#define kalStrnCmp(ct, cs, n)           strncmp(ct, cs, n)
#define kalStrChr(s, c)             strchr(s, c)
#define kalStrrChr(s, c)            strrchr(s, c)
#define kalStrnChr(s, n, c)         strnchr(s, n, c)
#define kalStrLen(s)                strlen(s)
#define kalStrnLen(s, b)            strnlen(s, b)

#define kalkStrtou8(cp, base, resp)     kstrtou8(cp, base, resp)
#define kalkStrtou16(cp, base, resp)        kstrtou16(cp, base, resp)
#define kalkStrtou32(cp, base, resp)        kstrtou32(cp, base, resp)
#define kalkStrtos32(cp, base, resp)        kstrtos32(cp, base, resp)
#define kalSnprintf(buf, size, fmt, ...)    snprintf(buf,  \
                                                     size, \
                                                     fmt,  \
                                                     ## __VA_ARGS__)
#define kalScnprintf(buf, size, fmt, ...) \
    scnprintf(buf, size, fmt, ## __VA_ARGS__)
#define kalSprintf(buf, fmt, ...)       sprintf(buf, fmt, __VA_ARGS__)

#define kalStrStr(ct, cs)           strstr(ct, cs)
#define kalStrSep(s, ct)            strsep(s, ct)
#define kalStrCat(dest, src)            strcat(dest, src)
#define kalIsXdigit(c)              isxdigit(c)

/* defined for wince sdio driver only */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Notify OS with SendComplete event of the specific packet. Linux should
 *        free packets here.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 * \param[in] status         Status Code for OS upper layer
 *
 * \return -
 */
/*----------------------------------------------------------------------------*/
#define kalSendComplete(prGlueInfo, pvPacket, status) \
    kalSendCompleteAndAwakeQueue(prGlueInfo, pvPacket)

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is used to locate the starting address of incoming
 * ethernet frame for skb.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 *
 * \return starting address of ethernet frame buffer.
 */
/*----------------------------------------------------------------------------*/
#define kalQueryBufferPointer(prGlueInfo, pvPacket) \
    ((u8 *)((struct sk_buff *)pvPacket)->data)

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is used to query the length of valid buffer which is
 * accessible during port read/write.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 *
 * \return starting address of ethernet frame buffer.
 */
/*----------------------------------------------------------------------------*/
#define kalQueryValidBufferLength(prGlueInfo, pvPacket) \
    ((u32)((struct sk_buff *)pvPacket)->end -           \
     (u32)((struct sk_buff *)pvPacket)->data)

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is used to copy the entire frame from skb to the
 * destination address in the input parameter.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 * \param[in] pucDestBuffer  Destination Address
 *
 * \return -
 */
/*----------------------------------------------------------------------------*/
#define kalCopyFrame(prGlueInfo, pvPacket, pucDestBuffer) \
    do {                                                  \
        struct sk_buff *skb = (struct sk_buff *)pvPacket; \
        memcpy(pucDestBuffer, skb->data, skb->len);       \
    } while (0)

#define kalGetTimeTick()           jiffies_to_msecs(jiffies)

#define WLAN_TAG    "[wlan]"
#define kalPrint(_Fmt ...)         printk(WLAN_TAG _Fmt)

extern int __printk_ratelimit(const char *func);
#define printk_ratelimit()         __printk_ratelimit(__func__)
#define kalPrintRateCtrl()         printk_ratelimit()

#define kalBreakPoint() \
    do {                \
        WARN_ON(1);     \
        panic("Oops");  \
    } while (0)

#define PRINTF_ARG(...)            __VA_ARGS__
#define SPRINTF(buf, arg)                              \
    {                                                  \
        buf += sprintf((char *)(buf), PRINTF_ARG arg); \
    }

#define USEC_TO_SYSTIME(_usec)         ((_usec) / USEC_PER_MSEC)
#define MSEC_TO_SYSTIME(_msec)         (_msec)

#define MSEC_TO_JIFFIES(_msec)         msecs_to_jiffies(_msec)

#define KAL_TIME_INTERVAL_DECLARATION()    struct timeval __rTs, __rTe
#define KAL_REC_TIME_START()           do_gettimeofday(&__rTs)
#define KAL_REC_TIME_END()         do_gettimeofday(&__rTe)
#define KAL_GET_TIME_INTERVAL()                    \
    ((SEC_TO_USEC(__rTe.tv_sec) + __rTe.tv_usec) - \
     (SEC_TO_USEC(__rTs.tv_sec) + __rTs.tv_usec))
#define KAL_ADD_TIME_INTERVAL(_Interval)        \
    {                                           \
        (_Interval) += KAL_GET_TIME_INTERVAL(); \
    }

#define KAL_GET_HOST_CLOCK()           local_clock()

/*----------------------------------------------------------------------------*/
/* Macros of wiphy operations for using in Driver Layer                       */
/*----------------------------------------------------------------------------*/
#define WIPHY_PRIV(_wiphy, _priv) \
    (_priv = *((P_GLUE_INFO_T *)wiphy_priv(_wiphy)))

/*----------------------------------------------------------------------------*/
/* Macros of show stack operations for using in Driver Layer                  */
/*----------------------------------------------------------------------------*/
#define kal_show_stack(_adapter, _task, _sp)                \
    {                                                       \
        if (_adapter->chip_info->showTaskStack) {           \
            _adapter->chip_info->showTaskStack(_task, _sp); \
        }                                                   \
    }

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/* Routines in gl_kal.c                                                       */
/*----------------------------------------------------------------------------*/
void kalAcquireSpinLock(IN P_GLUE_INFO_T prGlueInfo,
                        IN ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
                        OUT unsigned long *plFlags);

void kalReleaseSpinLock(IN P_GLUE_INFO_T prGlueInfo,
                        IN ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
                        IN unsigned long ulFlags);

void kalUpdateMACAddress(IN P_GLUE_INFO_T prGlueInfo, IN u8 *pucMacAddr);

void kalAcquireMutex(IN P_GLUE_INFO_T prGlueInfo,
                     IN ENUM_MUTEX_CATEGORY_E rMutexCategory);

void kalReleaseMutex(IN P_GLUE_INFO_T prGlueInfo,
                     IN ENUM_MUTEX_CATEGORY_E rMutexCategory);

void kalAcquireWDevMutex(IN struct net_device *pDev);
void kalReleaseWDevMutex(IN struct net_device *pDev);

void kalPacketFree(IN P_GLUE_INFO_T prGlueInfo, IN void *pvPacket);

void *kalPacketAlloc(IN P_GLUE_INFO_T prGlueInfo,
                     IN u32 u4Size,
                     OUT u8 **ppucData);

void kalOsTimerInitialize(IN P_GLUE_INFO_T prGlueInfo, IN void *prTimerHandler);

u8 kalSetTimer(IN P_GLUE_INFO_T prGlueInfo, IN u32 rInterval);

WLAN_STATUS
kalProcessRxPacket(IN P_GLUE_INFO_T prGlueInfo,
                   IN void *pvPacket,
                   IN u8 *pucPacketStart,
                   IN u32 u4PacketLen,
                   IN ENUM_CSUM_RESULT_T aeCSUM[]);

WLAN_STATUS kalRxIndicatePkts(IN P_GLUE_INFO_T prGlueInfo,
                              IN void *apvPkts[],
                              IN u8 ucPktNum);

WLAN_STATUS kalRxIndicateOnePkt(IN P_GLUE_INFO_T prGlueInfo, IN void *pvPkt);

void kalIndicateStatusAndComplete(IN P_GLUE_INFO_T prGlueInfo,
                                  IN WLAN_STATUS eStatus,
                                  IN void *pvBuf,
                                  IN u32 u4BufLen);

void kalUpdateReAssocReqInfo(IN P_GLUE_INFO_T prGlueInfo,
                             IN u8 *pucFrameBody,
                             IN u32 u4FrameBodyLen,
                             IN u8 fgReassocRequest);

void kalUpdateReAssocRspInfo(IN P_GLUE_INFO_T prGlueInfo,
                             IN u8 *pucFrameBody,
                             IN u32 u4FrameBodyLen);

#if CFG_TX_FRAGMENT
u8 kalQueryTxPacketHeader(IN P_GLUE_INFO_T prGlueInfo,
                          IN void *pvPacket,
                          OUT u16 *pu2EtherTypeLen,
                          OUT u8 *pucEthDestAddr);
#endif

void kalSendCompleteAndAwakeQueue(IN P_GLUE_INFO_T prGlueInfo,
                                  IN void *pvPacket);

#if CFG_TCP_IP_CHKSUM_OFFLOAD
void kalQueryTxChksumOffloadParam(IN void *pvPacket, OUT u8 *pucFlag);

void kalUpdateRxCSUMOffloadParam(IN void *pvPacket,
                                 IN ENUM_CSUM_RESULT_T eCSUM[]);
#endif

u8 kalRetrieveNetworkAddress(IN P_GLUE_INFO_T prGlueInfo,
                             IN OUT PARAM_MAC_ADDRESS *prMacAddr);

void kalReadyOnChannel(IN P_GLUE_INFO_T prGlueInfo,
                       IN u64 u8Cookie,
                       IN ENUM_BAND_T eBand,
                       IN ENUM_CHNL_EXT_T eSco,
                       IN u8 ucChannelNum,
                       IN u32 u4DurationMs);

void kalRemainOnChannelExpired(IN P_GLUE_INFO_T prGlueInfo,
                               IN u64 u8Cookie,
                               IN ENUM_BAND_T eBand,
                               IN ENUM_CHNL_EXT_T eSco,
                               IN u8 ucChannelNum);

#if CFG_SUPPORT_DFS
void kalIndicateChannelSwitch(IN P_GLUE_INFO_T prGlueInfo,
                              IN ENUM_CHNL_EXT_T eSco,
                              IN u8 ucChannelNum);
#endif

void kalIndicateMgmtTxStatus(IN P_GLUE_INFO_T prGlueInfo,
                             IN u64 u8Cookie,
                             IN u8 fgIsAck,
                             IN u8 *pucFrameBuf,
                             IN u32 u4FrameLen);

void kalIndicateRxMgmtFrame(IN P_GLUE_INFO_T prGlueInfo, IN P_SW_RFB_T prSwRfb);

/*----------------------------------------------------------------------------*/
/* Routines in interface - ehpi/sdio.c */
/*----------------------------------------------------------------------------*/
u8 kalDevRegRead(IN P_GLUE_INFO_T prGlueInfo,
                 IN u32 u4Register,
                 OUT u32 *pu4Value);
u8 kalDevRegRead_mac(IN P_GLUE_INFO_T prGlueInfo,
                     IN u32 u4Register,
                     OUT u32 *pu4Value);

u8 kalDevRegWrite(P_GLUE_INFO_T prGlueInfo, IN u32 u4Register, IN u32 u4Value);
u8 kalDevRegWrite_mac(P_GLUE_INFO_T prGlueInfo, IN u32 u4Register,
                      IN u32 u4Value);

u8 kalDevPortRead(IN P_GLUE_INFO_T prGlueInfo,
                  IN u16 u2Port,
                  IN u32 u2Len,
                  OUT u8 *pucBuf,
                  IN u32 u2ValidOutBufSize);

u8 kalDevPortWrite(P_GLUE_INFO_T prGlueInfo,
                   IN u16 u2Port,
                   IN u32 u2Len,
                   IN u8 *pucBuf,
                   IN u32 u2ValidInBufSize);

u8 kalDevWriteData(IN P_GLUE_INFO_T prGlueInfo, IN P_MSDU_INFO_T prMsduInfo);
#if CFG_MESON_G12A_PATCH
WLAN_STATUS kalDevWriteCmd(IN P_GLUE_INFO_T prGlueInfo,
                           IN P_CMD_INFO_T prCmdInfo,
                           IN u8 ucTC);
#else
u8 kalDevWriteCmd(IN P_GLUE_INFO_T prGlueInfo,
                  IN P_CMD_INFO_T prCmdInfo,
                  IN u8 ucTC);
#endif
u8 kalDevKickData(IN P_GLUE_INFO_T prGlueInfo);
void kalDevReadIntStatus(IN P_ADAPTER_T prAdapter, OUT u32 *pu4IntStatus);

u8 kalDevWriteWithSdioCmd52(IN P_GLUE_INFO_T prGlueInfo,
                            IN u32 u4Addr,
                            IN u8 ucData);

#if CFG_SUPPORT_EXT_CONFIG
u32 kalReadExtCfg(IN P_GLUE_INFO_T prGlueInfo);
#endif

u8 kalQoSFrameClassifierAndPacketInfo(IN P_GLUE_INFO_T prGlueInfo,
                                      IN P_NATIVE_PACKET prPacket,
                                      OUT P_TX_PACKET_INFO prTxPktInfo);

u8 kalGetEthDestAddr(IN P_GLUE_INFO_T prGlueInfo,
                     IN P_NATIVE_PACKET prPacket,
                     OUT u8 *pucEthDestAddr);

void kalOidComplete(IN P_GLUE_INFO_T prGlueInfo,
                    IN u8 fgSetQuery,
                    IN u32 u4SetQueryInfoLen,
                    IN WLAN_STATUS rOidStatus);

WLAN_STATUS
kalIoctl(IN P_GLUE_INFO_T prGlueInfo,
         IN PFN_OID_HANDLER_FUNC pfnOidHandler,
         IN void *pvInfoBuf,
         IN u32 u4InfoBufLen,
         IN u8 fgRead,
         IN u8 fgWaitResp,
         IN u8 fgCmd,
         OUT u32 *pu4QryInfoLen);

WLAN_STATUS
kalIoctlTimeout(IN P_GLUE_INFO_T prGlueInfo,
                IN PFN_OID_HANDLER_FUNC pfnOidHandler,
                IN void *pvInfoBuf,
                IN u32 u4InfoBufLen,
                IN u8 fgRead,
                IN u8 fgWaitResp,
                IN u8 fgCmd,
                IN s32 i4OidTimeout,
                OUT u32 *pu4QryInfoLen);

void kalHandleAssocInfo(IN P_GLUE_INFO_T prGlueInfo,
                        IN P_EVENT_ASSOC_INFO prAssocInfo);

#if CFG_ENABLE_FW_DOWNLOAD
WLAN_STATUS kalFirmwareOpen(IN P_GLUE_INFO_T prGlueInfo, IN u8 **apucNameTable);
WLAN_STATUS kalFirmwareClose(IN P_GLUE_INFO_T prGlueInfo);
WLAN_STATUS kalFirmwareLoad(IN P_GLUE_INFO_T prGlueInfo,
                            OUT void *prBuf,
                            IN u32 u4Offset,
                            OUT u32 *pu4Size);
WLAN_STATUS kalFirmwareSize(IN P_GLUE_INFO_T prGlueInfo, OUT u32 *pu4Size);
void kalConstructDefaultFirmwarePrio(P_GLUE_INFO_T prGlueInfo,
                                     u8 **apucNameTable,
                                     u8 **apucName,
                                     u8 *pucNameIdx,
                                     u8 ucMaxNameIdx);
void *kalFirmwareImageMapping(IN P_GLUE_INFO_T prGlueInfo,
                              OUT void **ppvMapFileBuf,
                              OUT u32 *pu4FileLength,
                              IN ENUM_IMG_DL_IDX_T eDlIdx);
void kalFirmwareImageUnmapping(IN P_GLUE_INFO_T prGlueInfo,
                               IN void *prFwHandle,
                               IN void *pvMapFileBuf);
#endif

/*----------------------------------------------------------------------------*/
/* Card Removal Check                                                         */
/*----------------------------------------------------------------------------*/
u8 kalIsCardRemoved(IN P_GLUE_INFO_T prGlueInfo);

/*----------------------------------------------------------------------------*/
/* TX                                                                         */
/*----------------------------------------------------------------------------*/
void kalFlushPendingTxPackets(IN P_GLUE_INFO_T prGlueInfo);

/*----------------------------------------------------------------------------*/
/* Media State Indication                                                     */
/*----------------------------------------------------------------------------*/
ENUM_PARAM_MEDIA_STATE_T kalGetMediaStateIndicated(IN P_GLUE_INFO_T prGlueInfo);

void kalSetMediaStateIndicated(IN P_GLUE_INFO_T prGlueInfo,
                               IN ENUM_PARAM_MEDIA_STATE_T
                               eParamMediaStateIndicate);

/*----------------------------------------------------------------------------*/
/* OID handling                                                               */
/*----------------------------------------------------------------------------*/
void kalOidCmdClearance(IN P_GLUE_INFO_T prGlueInfo);

void kalEnqueueCommand(IN P_GLUE_INFO_T prGlueInfo,
                       IN P_QUE_ENTRY_T prQueueEntry);

/*----------------------------------------------------------------------------*/
/* Security Frame Clearance                                                   */
/*----------------------------------------------------------------------------*/
void kalClearSecurityFrames(IN P_GLUE_INFO_T prGlueInfo);

void kalClearSecurityFramesByBssIdx(IN P_GLUE_INFO_T prGlueInfo,
                                    IN u8 ucBssIndex);

void kalSecurityFrameSendComplete(IN P_GLUE_INFO_T prGlueInfo,
                                  IN void *pvPacket,
                                  IN WLAN_STATUS rStatus);

/*----------------------------------------------------------------------------*/
/* Management Frame Clearance                                                 */
/*----------------------------------------------------------------------------*/
void kalClearMgmtFrames(IN P_GLUE_INFO_T prGlueInfo);

void kalClearMgmtFramesByBssIdx(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucBssIndex);

u32 kalGetTxPendingFrameCount(IN P_GLUE_INFO_T prGlueInfo);

u32 kalGetTxPendingCmdCount(IN P_GLUE_INFO_T prGlueInfo);

void kalClearCommandQueue(IN P_GLUE_INFO_T prGlueInfo, IN u8 fgIsNeedHandler);

u8 kalSetTimer(IN P_GLUE_INFO_T prGlueInfo, IN u32 u4Interval);

u8 kalCancelTimer(IN P_GLUE_INFO_T prGlueInfo);

void kalScanDone(IN P_GLUE_INFO_T prGlueInfo,
                 IN ENUM_KAL_NETWORK_TYPE_INDEX_T eNetTypeIdx,
                 IN WLAN_STATUS status);

u32 kalRandomNumber(void);

#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
void kalTimeoutHandler(struct timer_list *timer);
#else
void kalTimeoutHandler(unsigned long arg);
#endif

void kalSetEvent(P_GLUE_INFO_T pr);

void kalSetIntEvent(P_GLUE_INFO_T pr);

void kalSetTxEvent2Hif(P_GLUE_INFO_T pr);

void kalSetTxEvent2Rx(P_GLUE_INFO_T pr);

void kalSetTxCmdEvent2Hif(P_GLUE_INFO_T pr);

/*----------------------------------------------------------------------------*/
/* NVRAM/Registry Service                                                     */
/*----------------------------------------------------------------------------*/
u8 kalIsConfigurationExist(IN P_GLUE_INFO_T prGlueInfo);

P_REG_INFO_T kalGetConfiguration(IN P_GLUE_INFO_T prGlueInfo);

void kalGetConfigurationVersion(IN P_GLUE_INFO_T prGlueInfo,
                                OUT u16 *pu2Part1CfgOwnVersion,
                                OUT u16 *pu2Part1CfgPeerVersion,
                                OUT u16 *pu2Part2CfgOwnVersion,
                                OUT u16 *pu2Part2CfgPeerVersion);

u8 kalCfgDataRead16(IN P_GLUE_INFO_T prGlueInfo,
                    IN u32 u4Offset,
                    OUT u16 *pu2Data);

u8 kalCfgDataWrite16(IN P_GLUE_INFO_T prGlueInfo, IN u32 u4Offset,
                     IN u16 u2Data);

/*----------------------------------------------------------------------------*/
/* WSC Connection                                                     */
/*----------------------------------------------------------------------------*/
u8 kalWSCGetActiveState(IN P_GLUE_INFO_T prGlueInfo);

/*----------------------------------------------------------------------------*/
/* RSSI Updating                                                              */
/*----------------------------------------------------------------------------*/
void kalUpdateRSSI(IN P_GLUE_INFO_T prGlueInfo,
                   IN ENUM_KAL_NETWORK_TYPE_INDEX_T eNetTypeIdx,
                   IN s8 cRssi,
                   IN s8 cLinkQuality);

/*----------------------------------------------------------------------------*/
/* I/O Buffer Pre-allocation                                                  */
/*----------------------------------------------------------------------------*/
u8 kalInitIOBuffer(u8 is_pre_alloc);

void kalUninitIOBuffer(void);

void *kalAllocateIOBuffer(IN u32 u4AllocSize);

void kalReleaseIOBuffer(IN void *pvAddr, IN u32 u4Size);

void kalGetChannelList(IN P_GLUE_INFO_T prGlueInfo,
                       IN ENUM_BAND_T eSpecificBand,
                       IN u8 ucMaxChannelNum,
                       IN u8 *pucNumOfChannel,
                       IN P_RF_CHANNEL_INFO_T paucChannelList);

u8 kalIsAPmode(IN P_GLUE_INFO_T prGlueInfo);

#if CFG_SUPPORT_802_11W
/*----------------------------------------------------------------------------*/
/* 802.11W                                                                    */
/*----------------------------------------------------------------------------*/
u32 kalGetMfpSetting(IN P_GLUE_INFO_T prGlueInfo);
u8 kalGetRsnIeMfpCap(IN P_GLUE_INFO_T prGlueInfo);
#endif

/*----------------------------------------------------------------------------*/
/* file opetation                                                             */
/*----------------------------------------------------------------------------*/
u32 kalWriteToFile(const u8 *pucPath, u8 fgDoAppend, u8 *pucData, u32 u4Size);

u32 kalCheckPath(const u8 *pucPath);

u32 kalTrunkPath(const u8 *pucPath);

s32 kalReadToFile(const u8 *pucPath, u8 *pucData, u32 u4Size, u32 *pu4ReadSize);

s32 kalRequestFirmware(const u8 *pucPath,
                       u8 *pucData,
                       u32 u4Size,
                       u32 *pu4ReadSize,
                       struct device *dev);

/*----------------------------------------------------------------------------*/
/* NL80211                                                                    */
/*----------------------------------------------------------------------------*/
void kalIndicateBssInfo(IN P_GLUE_INFO_T prGlueInfo,
                        IN u8 *pucFrameBuf,
                        IN u32 u4BufLen,
                        IN u8 ucChannelNum,
                        IN s32 i4SignalStrength);

/*----------------------------------------------------------------------------*/
/* Net device                                                                 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
kalHardStartXmit(struct sk_buff *prSkb,
                 IN struct net_device *prDev,
                 P_GLUE_INFO_T prGlueInfo,
                 u8 ucBssIndex);

u8 kalIsPairwiseEapolPacket(IN P_NATIVE_PACKET prPacket);

u8 kalGetIPv4Address(IN struct net_device *prDev,
                     IN u32 u4MaxNumOfAddr,
                     OUT u8 *pucIpv4Addrs,
                     OUT u32 *pu4NumOfIpv4Addr);

#if IS_ENABLED(CONFIG_IPV6)
u8 kalGetIPv6Address(IN struct net_device *prDev,
                     IN u32 u4MaxNumOfAddr,
                     OUT u8 *pucIpv6Addrs,
                     OUT u32 *pu4NumOfIpv6Addr);
#else
static inline u8 kalGetIPv6Address(IN struct net_device *prDev,
                                   IN u32 u4MaxNumOfAddr, OUT u8 *pucIpv6Addrs,
                                   OUT u32 *pu4NumOfIpv6Addr)
{
    /* Not support IPv6 */
    *pu4NumOfIpv6Addr = 0;
    return false;
}
#endif

void kalSetNetAddressFromInterface(IN P_GLUE_INFO_T prGlueInfo,
                                   IN struct net_device *prDev,
                                   IN u8 fgSet);

WLAN_STATUS kalResetStats(IN struct net_device *prDev);

void *kalGetStats(IN struct net_device *prDev);

void kalResetPacket(IN P_GLUE_INFO_T prGlueInfo, IN P_NATIVE_PACKET prPacket);

#if CFG_SUPPORT_QA_TOOL
struct file *kalFileOpen(const char *path, int flags, int rights);

void kalFileClose(struct file *file);

u32 kalFileRead(struct file *file,
                unsigned long long offset,
                unsigned char *data,
                unsigned int size);
#endif

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
/*----------------------------------------------------------------------------*/
/* SDIO Read/Write Pattern Support                                            */
/*----------------------------------------------------------------------------*/
u8 kalSetSdioTestPattern(IN P_GLUE_INFO_T prGlueInfo, IN u8 fgEn, IN u8 fgRead);
#endif

/*----------------------------------------------------------------------------*/
/* PNO Support                                                                */
/*----------------------------------------------------------------------------*/
void kalSchedScanResults(IN P_GLUE_INFO_T prGlueInfo);

void kalSchedScanStopped(IN P_GLUE_INFO_T prGlueInfo);

void kalWDevLockThread(IN P_GLUE_INFO_T prGlueInfo,
                       IN struct net_device *pDev,
                       IN enum ENUM_CFG80211_WDEV_LOCK_FUNC fn,
                       IN u8 *pFrameBuf,
                       IN size_t frameLen,
                       IN struct cfg80211_bss *pBss,
                       IN s32 uapsd_queues,
                       const u8 *req_ies,
                       size_t req_ies_len,
                       IN u8 fgIsInterruptContext);

#if CFG_MULTI_ECOVER_SUPPORT

typedef enum _ENUM_WMTCHIN_TYPE_T {
    WMTCHIN_CHIPID       = 0x0,
    WMTCHIN_HWVER        = WMTCHIN_CHIPID + 1,
    WMTCHIN_MAPPINGHWVER = WMTCHIN_HWVER + 1,
    WMTCHIN_FWVER        = WMTCHIN_MAPPINGHWVER + 1,
    WMTCHIN_MAX
} ENUM_WMT_CHIPINFO_TYPE_T,
*P_ENUM_WMT_CHIPINFO_TYPE_T;

u32 mtk_wcn_wmt_ic_info_get(ENUM_WMT_CHIPINFO_TYPE_T type);

#endif

void kalSetFwOwnEvent2Hif(P_GLUE_INFO_T pr);
#if CFG_ASSERT_DUMP
/* Core Dump out put file */
WLAN_STATUS kalOpenCorDumpFile(u8 fgIsN9);
WLAN_STATUS kalWriteCorDumpFile(u8 *pucBuffer, u16 u2Size, u8 fgIsN9);
WLAN_STATUS kalCloseCorDumpFile(u8 fgIsN9);
#endif
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#if CFG_WOW_SUPPORT
void kalWowInit(IN P_GLUE_INFO_T prGlueInfo);
void kalWowProcess(IN P_GLUE_INFO_T prGlueInfo, u8 enable);
#endif

int main_thread(void *data);
int hif_thread(void *data);
int rx_thread(void *data);

u64 kalGetBootTime(void);

int kalMetInitProcfs(IN P_GLUE_INFO_T prGlueInfo);
int kalMetRemoveProcfs(IN P_GLUE_INFO_T prGlueInfo);

void kalFreeTxMsduWorker(struct work_struct *work);
void kalFreeTxMsdu(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo);

static inline void kal_skb_reset_mac_len(struct sk_buff *skb)
{
    skb_reset_mac_len(skb);
}

int kal_sched_set(struct task_struct *p,
                  int policy,
                  const struct sched_param *param,
                  int nice);

WLAN_STATUS kalUpdateBssChannel(IN P_GLUE_INFO_T prGlueInfo,
                                IN u8 aucSSID[],
                                IN u8 ucSsidLength,
                                IN u8 aucBSSID[],
                                IN u8 ucChannelNum);

static inline void kal_eth_hw_addr_set(struct net_device *dev,
                                       const u8 *addr)
{
#if KERNEL_VERSION(5, 17, 0) <= LINUX_VERSION_CODE
    // eth_hw_addr_set(dev, addr);
    dev_addr_set(dev, addr);
#else
    kalMemCopy(dev->dev_addr, addr, ETH_ALEN);
#endif
}

#endif
