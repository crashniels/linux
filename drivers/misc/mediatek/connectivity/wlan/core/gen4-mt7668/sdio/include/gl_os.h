/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_os.h
 *    \brief  List the external reference to OS for GLUE Layer.
 *
 *    In this file we define the data structure - GLUE_INFO_T to store those
 * objects we acquired from OS - e.g. TIMER, SPINLOCK, NET DEVICE ... . And all
 * the external reference (header file, extern func() ..) to OS for GLUE Layer
 * should also list down here.
 */

#ifndef _GL_OS_H
#define _GL_OS_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*------------------------------------------------------------------------------
 * Flags for LINUX(OS) dependent
 *------------------------------------------------------------------------------
 */
#define CFG_MAX_WLAN_DEVICES              1  /* number of wlan card will coexist */

#define CFG_MAX_TXQ_NUM               4  /* number of tx queue for support multi-queue h/w */

/* 1: Enable use of SPIN LOCK Bottom Half for LINUX */
/* 0: Disable - use SPIN LOCK IRQ SAVE instead */
#define CFG_USE_SPIN_LOCK_BOTTOM_HALF         0

/* 1: Enable - Drop ethernet packet if it < 14 bytes.
 * And pad ethernet packet with dummy 0 if it < 60 bytes.
 * 0: Disable
 */
#define CFG_TX_PADDING_SMALL_ETH_PACKET       0

#define CFG_TX_STOP_NETIF_QUEUE_THRESHOLD     256  /* packets */

#define CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD     512  /* packets */
#define CFG_TX_START_NETIF_PER_QUEUE_THRESHOLD    128  /* packets */

/* WMM Certification Related */
#define CFG_CERT_WMM_MAX_TX_PENDING       20
#define CFG_CERT_WMM_MAX_RX_NUM           10
#define CFG_CERT_WMM_HIGH_STOP_TX_WITH_RX \
    (CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD * 3)
#define CFG_CERT_WMM_HIGH_STOP_TX_WO_RX \
    (CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD * 2)
#define CFG_CERT_WMM_LOW_STOP_TX_WITH_RX \
    (CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD >> 4)
#define CFG_CERT_WMM_LOW_STOP_TX_WO_RX \
    (CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD >> 3)

#define CHIP_NAME                 "MT6632"

#define DRV_NAME                  "[" CHIP_NAME "]: "

/* for CFG80211 IE buffering mechanism */
#define CFG_CFG80211_IE_BUF_LEN           (512)

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include <linux/version.h>  /* constant of kernel version */

#include <linux/kernel.h>  /* bitops.h */

#include <linux/timer.h>  /* struct timer_list */
#include <linux/jiffies.h>  /* jiffies */
#include <linux/delay.h>  /* udelay and mdelay macro */

#include <linux/irq.h>  /* IRQT_FALLING */

#include <linux/netdevice.h>  /* struct net_device, struct net_device_stats */
#include <linux/etherdevice.h>  /* for eth_type_trans() function */
#include <linux/wireless.h>  /* struct iw_statistics */
#include <linux/if_arp.h>
#include <linux/inetdevice.h>  /* struct in_device */

#include <linux/ip.h>  /* struct iphdr */

#include <linux/string.h>  /* for memcpy()/memset() function */
#include <linux/stddef.h>  /* for offsetof() macro */

#include <linux/proc_fs.h>  /* The proc filesystem constants/structures */

#include <linux/rtnetlink.h>  /* for rtnl_lock() and rtnl_unlock() */
#include <linux/kthread.h>  /* kthread_should_stop(), kthread_run() */
#include <linux/uaccess.h>  /* for copy_from_user() */
#include <linux/fs.h>  /* for firmware download */
#include <linux/vmalloc.h>

#include <linux/kfifo.h>  /* for kfifo interface */
#include <linux/cdev.h>  /* for cdev interface */

#include <linux/firmware.h>  /* for firmware download */
#include <linux/ctype.h>

#include <linux/interrupt.h>

#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>

#include <linux/random.h>

#include <linux/io.h>  /* readw and writew */

#if WIRELESS_EXT > 12
#include <net/iw_handler.h>
#endif

#ifdef CFG_CFG80211_VERSION
#define CFG80211_VERSION_CODE    CFG_CFG80211_VERSION
#else
#define CFG80211_VERSION_CODE    LINUX_VERSION_CODE
#endif

#include "version.h"
#include "config.h"

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
#include <linux/wireless.h>
#include <net/cfg80211.h>
#endif

#include <linux/module.h>
#include <linux/can/netlink.h>
#include <net/netlink.h>

#if IS_ENABLED(CONFIG_IPV6)
#include <linux/ipv6.h>
#include <linux/in6.h>
#include <net/if_inet6.h>
#endif

#include <uapi/linux/nl80211.h>

#include "gl_typedef.h"
#include "typedef.h"
#include "queue.h"
#include "gl_kal.h"
#include "hif.h"

#if CFG_SUPPORT_TDLS
#include "tdls.h"
#endif

#include "debug.h"

#include "wlan_lib.h"
#include "wlan_oid.h"

#include <linux/time.h>
#include <linux/capability.h>
#include <linux/skbuff.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
#include <linux/sched/clock.h>
#include <uapi/linux/sched/types.h>

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched.h>
#include <uapi/linux/sched/types.h>
#endif

extern u8 fgIsBusAccessFailed;
extern const struct ieee80211_iface_combination *p_mtk_iface_combinations_sta;
extern const s32 mtk_iface_combinations_sta_num;

#if CFG_ENABLE_UNIFY_WIPHY
extern const struct ieee80211_iface_combination *p_mtk_iface_combinations_p2p;
extern const s32 mtk_iface_combinations_p2p_num;
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define GLUE_FLAG_HALT                BIT(0)
#define GLUE_FLAG_INT                 BIT(1)
#define GLUE_FLAG_OID                 BIT(2)
#define GLUE_FLAG_TIMEOUT             BIT(3)
#define GLUE_FLAG_TXREQ               BIT(4)
#define GLUE_FLAG_SUB_MOD_MULTICAST       BIT(7)
#define GLUE_FLAG_FRAME_FILTER            BIT(8)
#define GLUE_FLAG_FRAME_FILTER_AIS        BIT(9)

#define GLUE_FLAG_HALT_BIT            (0)
#define GLUE_FLAG_INT_BIT             (1)
#define GLUE_FLAG_OID_BIT             (2)
#define GLUE_FLAG_TIMEOUT_BIT             (3)
#define GLUE_FLAG_TXREQ_BIT           (4)
#define GLUE_FLAG_SUB_MOD_MULTICAST_BIT       (7)
#define GLUE_FLAG_FRAME_FILTER_BIT        (8)
#define GLUE_FLAG_FRAME_FILTER_AIS_BIT        (9)

#define GLUE_FLAG_RX                  BIT(10)
#define GLUE_FLAG_TX_CMD_DONE             BIT(11)
#define GLUE_FLAG_HIF_TX              BIT(12)
#define GLUE_FLAG_HIF_TX_CMD              BIT(13)
#define GLUE_FLAG_RX_TO_OS            BIT(14)
#define GLUE_FLAG_HIF_FW_OWN              BIT(15)
#define GLUE_FLAG_HIF_PRT_HIF_DBG_INFO        BIT(16)

#define GLUE_FLAG_RX_BIT              (10)
#define GLUE_FLAG_TX_CMD_DONE_BIT         (11)
#define GLUE_FLAG_HIF_TX_BIT              (12)
#define GLUE_FLAG_HIF_TX_CMD_BIT          (13)
#define GLUE_FLAG_RX_TO_OS_BIT            (14)
#define GLUE_FLAG_HIF_FW_OWN_BIT          (15)
#define GLUE_FLAG_HIF_PRT_HIF_DBG_INFO_BIT    (16)
#define GLUE_FLAG_ADAPT_RDY           BIT(17)

#define GLUE_FLAG_ADAPT_RDY_BIT           (17)

#define GLUE_BOW_KFIFO_DEPTH              (1024)
#define GLUE_BOW_DEVICE_NAME              "ampc0"

#define WAKE_LOCK_RX_TIMEOUT              300  /* ms */
#define WAKE_LOCK_THREAD_WAKEUP_TIMEOUT       50  /* ms */

/* EFUSE Auto Mode Support */
#define LOAD_EFUSE                0
#define LOAD_EEPROM_BIN               1
#define LOAD_AUTO                 2
#define EFUSE_AUTO_CHEK               0x76

#if KERNEL_VERSION(4, 12, 0) > CFG80211_VERSION_CODE
#define WLAN_AKM_SUITE_8021X_SUITE_B          0x000FAC0B
#define WLAN_AKM_SUITE_8021X_SUITE_B_192      0x000FAC0C
#endif

#if KERNEL_VERSION(6, 0, 0) > CFG80211_VERSION_CODE
#if CFG_SUPPORT_OWE
#define WLAN_AKM_SUITE_OWE            0x000FAC12
#endif
#endif

#define IW_AUTH_CIPHER_GCMP256            0x00000080

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

typedef struct _GL_WPA_INFO_T {
    u32 u4WpaVersion;
    u32 u4KeyMgmt;
    u32 u4CipherGroup;
    u32 u4CipherPairwise;
    u32 u4AuthAlg;
    u8 fgPrivacyInvoke;
#if CFG_SUPPORT_802_11W
    u32 u4CipherGroupMgmt;
    u32 u4Mfp;
    u8 ucRSNMfpCap;
#endif
    u8 ucRsneLen;
    u8 aucKek[NL80211_KEK_LEN];
    u8 aucKck[NL80211_KCK_LEN];
    u8 aucReplayCtr[NL80211_REPLAY_CTR_LEN];
} GL_WPA_INFO_T, *P_GL_WPA_INFO_T;

#if CFG_SUPPORT_REPLAY_DETECTION
struct SEC_REPLEY_PN_INFO {
    u8 auPN[16];
    u8 fgRekey;
    u8 fgFirstPkt;
};
struct SEC_DETECT_REPLAY_INFO {
    u8 ucCurKeyId;
    u8 ucKeyType;
    struct SEC_REPLEY_PN_INFO arReplayPNInfo[4];
    u32 u4KeyLength;
    u8 aucKeyMaterial[32];
    u8 fgPairwiseInstalled;
    u8 fgKeyRscFresh;
};
#endif

typedef enum _ENUM_NET_DEV_IDX_T {
    NET_DEV_WLAN_IDX = 0,
    NET_DEV_P2P_IDX,
    NET_DEV_BOW_IDX,
    NET_DEV_NUM
} ENUM_NET_DEV_IDX_T;

typedef enum _ENUM_RSSI_TRIGGER_TYPE {
    ENUM_RSSI_TRIGGER_NONE,
    ENUM_RSSI_TRIGGER_GREATER,
    ENUM_RSSI_TRIGGER_LESS,
    ENUM_RSSI_TRIGGER_TRIGGERED,
    ENUM_RSSI_TRIGGER_NUM
} ENUM_RSSI_TRIGGER_TYPE;

#if CFG_ENABLE_WIFI_DIRECT
typedef enum _ENUM_NET_REG_STATE_T {
    ENUM_NET_REG_STATE_UNREGISTERED,
    ENUM_NET_REG_STATE_REGISTERING,
    ENUM_NET_REG_STATE_REGISTERED,
    ENUM_NET_REG_STATE_UNREGISTERING,
    ENUM_NET_REG_STATE_NUM
} ENUM_NET_REG_STATE_T;

typedef enum _ENUM_P2P_REG_STATE_T {
    ENUM_P2P_REG_STATE_UNREGISTERED,
    ENUM_P2P_REG_STATE_REGISTERING,
    ENUM_P2P_REG_STATE_REGISTERED,
    ENUM_P2P_REG_STATE_UNREGISTERING,
    ENUM_P2P_REG_STATE_NUM
} ENUM_P2P_REG_STATE_T;
#endif

typedef enum _ENUM_PKT_FLAG_T {
    ENUM_PKT_802_11,  /* 802.11 or non-802.11 */
    ENUM_PKT_802_3,  /* 802.3 or ethernetII */
    ENUM_PKT_1X,  /* 1x frame or not */
    ENUM_PKT_PROTECTED_1X,  /* protected 1x frame */
    ENUM_PKT_NON_PROTECTED_1X,  /* Non protected 1x frame */
    ENUM_PKT_VLAN_EXIST,  /* VLAN tag exist */
    ENUM_PKT_DHCP,  /* DHCP frame */
    ENUM_PKT_ARP,  /* ARP */
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT
    ENUM_PKT_FIRST_DUP,  /* The First Duplicate packet */
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
    ENUM_PKT_LB,  /* PKT LB flag */
#endif
#endif
    ENUM_PKT_FLAG_NUM
} ENUM_PKT_FLAG_T;

typedef struct _GL_IO_REQ_T {
    QUE_ENTRY_T rQueEntry;
    /* wait_queue_head_t       cmdwait_q; */
    u8 fgRead;
    u8 fgWaitResp;
    P_ADAPTER_T prAdapter;
    PFN_OID_HANDLER_FUNC pfnOidHandler;
    void *pvInfoBuf;
    u32 u4InfoBufLen;
    u32 *pu4QryInfoLen;
    WLAN_STATUS rStatus;
    u32 u4Flag;
    u32 u4Timeout;
} GL_IO_REQ_T, *P_GL_IO_REQ_T;

/*
 * type definition of pointer to p2p structure
 */
typedef struct _GL_P2P_INFO_T GL_P2P_INFO_T, *P_GL_P2P_INFO_T;
typedef struct _GL_P2P_DEV_INFO_T GL_P2P_DEV_INFO_T, *P_GL_P2P_DEV_INFO_T;

struct _GLUE_INFO_T {
    /* Device handle */
    struct net_device *prDevHandler;

    /* Device */
    struct device *prDev;

    /* Device Index(index of arWlanDevInfo[]) */
    s32 i4DevIdx;

    /* Device statistics */
    /* struct net_device_stats rNetDevStats; */

    /* Wireless statistics struct net_device */
    struct iw_statistics rIwStats;

    /* spinlock to sync power save mechanism */
    spinlock_t rSpinLock[SPIN_LOCK_NUM];

    /* Mutex to protect interruptible section */
    struct mutex arMutex[MUTEX_NUM];

    /* semaphore for ioctl */
    struct semaphore ioctl_sem;

    u64 u8Cookie;

    volatile unsigned long ulFlag;  /* GLUE_FLAG_XXX */
    u32 u4PendFlag;
    /* u32 u4TimeoutFlag; */
    u32 u4OidCompleteFlag;
    u32 u4ReadyFlag;  /* check if card is ready */

    u32 u4OsMgmtFrameFilter;

    /* Number of pending frames, also used for debuging if any frame is
     * missing during the process of unloading Driver.
     *
     * NOTE(Kevin): In Linux, we also use this variable as the threshold
     * for manipulating the netif_stop(wake)_queue() func.
     */
    s32 ai4TxPendingFrameNumPerQueue[HW_BSSID_NUM + 1][CFG_MAX_TXQ_NUM];
    s32 i4TxPendingFrameNum;
    s32 i4TxPendingSecurityFrameNum;
    s32 i4TxPendingCmdNum;

    /* Tx: for NetDev to BSS index mapping */
    NET_INTERFACE_INFO_T arNetInterfaceInfo[HW_BSSID_NUM + 1];

    /* Rx: for BSS index to NetDev mapping */
    /* P_NET_INTERFACE_INFO_T  aprBssIdxToNetInterfaceInfo[HW_BSSID_NUM]; */

    /* current IO request for kalIoctl */
    GL_IO_REQ_T OidEntry;

    /* registry info */
    REG_INFO_T rRegInfo;

    /* firmware */
    struct firmware *prFw;

    /* Host interface related information */
    /* defined in related hif header file */
    GL_HIF_INFO_T rHifInfo;

    /*! \brief wext wpa related information */
    GL_WPA_INFO_T rWpaInfo;

    /* Pointer to ADAPTER_T - main data structure of internal protocol stack
     */
    P_ADAPTER_T prAdapter;

#if WLAN_INCLUDE_PROC
    struct proc_dir_entry *pProcRoot;
#endif

    /* Indicated media state */
    ENUM_PARAM_MEDIA_STATE_T eParamMediaStateIndicated;

    /* Device power state D0~D3 */
    PARAM_DEVICE_POWER_STATE ePowerState;

    struct completion rScanComp;  /* indicate scan complete */
    struct completion rHaltComp;  /* indicate main thread halt complete */
    struct completion rPendComp;  /* indicate main thread halt complete */
    struct completion rHifHaltComp;  /* indicate hif_thread halt complete */
    struct completion rRxHaltComp;  /* indicate hif_thread halt complete */

    u32 u4TxThreadPid;
    u32 u4RxThreadPid;
    u32 u4HifThreadPid;
    WLAN_STATUS rPendStatus;

    QUE_T rTxQueue;

    /* OID related */
    QUE_T rCmdQueue;

    wait_queue_head_t waitq;
    struct task_struct *main_thread;

    wait_queue_head_t waitq_hif;
    struct task_struct *hif_thread;

    wait_queue_head_t waitq_rx;
    struct task_struct *rx_thread;

    struct tasklet_struct rRxTask;
    struct tasklet_struct rTxCompleteTask;

    struct work_struct rTxMsduFreeWork;
    struct delayed_work rRxPktDeAggWork;

    struct timer_list tickfn;

#if CFG_SUPPORT_EXT_CONFIG
    u16 au2ExtCfg[256];  /* NVRAM data buffer */
    u32 u4ExtCfgLength;  /* 0 means data is NOT valid */
#endif

    /* Should be large than the PARAM_WAPI_ASSOC_INFO_T */
    u8 aucWapiAssocInfoIEs[42];
    u16 u2WapiAssocInfoIESz;

#if CFG_ENABLE_WIFI_DIRECT
    P_GL_P2P_DEV_INFO_T prP2PDevInfo;
    P_GL_P2P_INFO_T prP2PInfo[KAL_P2P_NUM];
#if CFG_SUPPORT_P2P_RSSI_QUERY
    /* Wireless statistics struct net_device */
    struct iw_statistics rP2pIwStats;
#endif
#endif
    u8 fgWpsActive;
    u8 aucWSCIE[500];  /*for probe req */
    u16 u2WSCIELen;
    u8 aucWSCAssocInfoIE[200];  /*for Assoc req */
    u16 u2WSCAssocInfoIELen;

    /* NVRAM availability */
    u8 fgNvramAvailable;

    u8 fgMcrAccessAllowed;

    /* MAC Address Overridden by IOCTL */
    u8 fgIsMacAddrOverride;
    PARAM_MAC_ADDRESS rMacAddrOverride;

    SET_TXPWR_CTRL_T rTxPwr;

    /* for cfg80211 scan done indication */
    struct cfg80211_scan_request *prScanRequest;

    /* for cfg80211 scheduled scan */
    struct cfg80211_sched_scan_request *prSchedScanRequest;

    /* to indicate registered or not */
    u8 fgIsRegistered;

    /* for cfg80211 connected indication */
    u32 u4RspIeLength;
    u8 aucRspIe[CFG_CFG80211_IE_BUF_LEN];

    u32 u4ReqIeLength;
    u8 aucReqIe[CFG_CFG80211_IE_BUF_LEN];

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
    u8 fgEnSdioTestPattern;
    u8 fgSdioReadWriteMode;
    u8 fgIsSdioTestInitialized;
    u8 aucSdioTestBuffer[256];
#endif

    u8 fgIsInSuspendMode;

#if CFG_MET_PACKET_TRACE_SUPPORT
    u8 fgMetProfilingEn;
    u16 u2MetUdpPort;
#endif

#if CFG_SUPPORT_SNIFFER
    u8 fgIsEnableMon;
    struct net_device *prMonDevHandler;
    struct work_struct monWork;
#endif

    s32 i4RssiCache;
    u32 u4LinkSpeedCache;

#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
    QUE_T rTxLookBackQueue;
    QUE_T rRxLookBackQueue;
#endif
};

typedef irqreturn_t (*PFN_WLANISR)(int irq, void *dev_id, struct pt_regs *regs);

typedef void (*PFN_LINUX_TIMER_FUNC)(unsigned long);

/* generic sub module init/exit handler
 *   now, we only have one sub module, p2p
 */
#if CFG_ENABLE_WIFI_DIRECT
typedef u8 (*SUB_MODULE_INIT)(P_GLUE_INFO_T prGlueInfo);
typedef u8 (*SUB_MODULE_EXIT)(P_GLUE_INFO_T prGlueInfo);

typedef struct _SUB_MODULE_HANDLER {
    SUB_MODULE_INIT subModInit;
    SUB_MODULE_EXIT subModExit;
    u8 fgIsInited;
} SUB_MODULE_HANDLER, *P_SUB_MODULE_HANDLER;

#endif

#ifdef CONFIG_NL80211_TESTMODE

enum TestModeCmdType {
    TESTMODE_CMD_ID_SW_CMD  = 1,
    TESTMODE_CMD_ID_WAPI    = 2,
    TESTMODE_CMD_ID_HS20    = 3,
    TESTMODE_CMD_ID_STR_CMD = 102,
    NUM_OF_TESTMODE_CMD_ID
};

typedef struct _NL80211_DRIVER_TEST_MODE_PARAMS {
    u32 index;
    u32 buflen;
} NL80211_DRIVER_TEST_MODE_PARAMS, *P_NL80211_DRIVER_TEST_MODE_PARAMS;

/*SW CMD */
typedef struct _NL80211_DRIVER_SW_CMD_PARAMS {
    NL80211_DRIVER_TEST_MODE_PARAMS hdr;
    u8 set;
    u32 adr;
    u32 data;
} NL80211_DRIVER_SW_CMD_PARAMS, *P_NL80211_DRIVER_SW_CMD_PARAMS;

struct iw_encode_exts {
    __u32 ext_flags;  /*!< IW_ENCODE_EXT_**/
    __u8 tx_seq[IW_ENCODE_SEQ_MAX_SIZE];  /*!< LSB first */
    __u8 rx_seq[IW_ENCODE_SEQ_MAX_SIZE];  /*!< LSB first */
    /*!< ff:ff:ff:ff:ff:ff for broadcast/multicast
     *   (group) keys or unicast address for
     *   individual keys
     */
    __u8 addr[MAC_ADDR_LEN];
    __u16 alg;  /*!< IW_ENCODE_ALG_**/
    __u16 key_len;
    __u8 key[32];
};

/*SET KEY EXT */
typedef struct _NL80211_DRIVER_SET_KEY_EXTS {
    NL80211_DRIVER_TEST_MODE_PARAMS hdr;
    u8 key_index;
    u8 key_len;
    struct iw_encode_exts ext;
} NL80211_DRIVER_SET_KEY_EXTS, *P_NL80211_DRIVER_SET_KEY_EXTS;

#endif

typedef struct _NETDEV_PRIVATE_GLUE_INFO {
    P_GLUE_INFO_T prGlueInfo;
    u8 ucBssIdx;
#if CFG_ENABLE_UNIFY_WIPHY
    u8 ucIsP2p;
#endif
} NETDEV_PRIVATE_GLUE_INFO, *P_NETDEV_PRIVATE_GLUE_INFO;

typedef struct _PACKET_PRIVATE_DATA {
    QUE_ENTRY_T rQueEntry;
    u16 u2Flag;
    u8 ucTid;
    u8 ucBssIdx;

    u8 ucHeaderLen;
    u16 u2FrameLen;

    u8 ucProfilingFlag;
    u32 rArrivalTime;
    u16 u2IpId;
} PACKET_PRIVATE_DATA, *P_PACKET_PRIVATE_DATA;

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
/* Macros of SPIN LOCK operations for using in Glue Layer                     */
/*----------------------------------------------------------------------------*/
#if CFG_USE_SPIN_LOCK_BOTTOM_HALF
#define GLUE_SPIN_LOCK_DECLARATION()
#define GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, rLockCategory)      \
    {                                                          \
        if (rLockCategory < SPIN_LOCK_NUM)                     \
        spin_lock_bh(&(prGlueInfo->rSpinLock[rLockCategory])); \
    }
#define GLUE_RELEASE_SPIN_LOCK(prGlueInfo, rLockCategory) \
    {                                                     \
        if (rLockCategory < SPIN_LOCK_NUM)                \
        spin_unlock_bh(                                   \
            &(prGlueInfo->rSpinLock[rLockCategory]));     \
    }
#else  /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */
#define GLUE_SPIN_LOCK_DECLARATION()    unsigned long __ulFlags = 0
#define GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, rLockCategory) \
    {                                                     \
        if (rLockCategory < SPIN_LOCK_NUM)                \
        spin_lock_irqsave(                                \
            &(prGlueInfo)->rSpinLock[rLockCategory],      \
            __ulFlags);                                   \
    }
#define GLUE_RELEASE_SPIN_LOCK(prGlueInfo, rLockCategory) \
    {                                                     \
        if (rLockCategory < SPIN_LOCK_NUM)                \
        spin_unlock_irqrestore(                           \
            &(prGlueInfo->rSpinLock[rLockCategory]),      \
            __ulFlags);                                   \
    }
#endif

/*----------------------------------------------------------------------------*/
/* Macros for accessing Reserved Fields of native packet                      */
/*----------------------------------------------------------------------------*/

#define GLUE_GET_PKT_PRIVATE_DATA(_p) \
    ((P_PACKET_PRIVATE_DATA)(&(((struct sk_buff *)(_p))->cb[0])))

#define GLUE_GET_PKT_QUEUE_ENTRY(_p) \
    (&(GLUE_GET_PKT_PRIVATE_DATA(_p)->rQueEntry))

#define GLUE_GET_PKT_DESCRIPTOR(_prQueueEntry)          \
    ((P_NATIVE_PACKET)(((unsigned long)_prQueueEntry) - \
                       offsetof(struct sk_buff, cb[0])))

#define GLUE_SET_PKT_TID(_p, _tid) \
    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucTid = (u8)(_tid))

#define GLUE_GET_PKT_TID(_p)           (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucTid)

#define GLUE_SET_PKT_FLAG(_p, _flag) \
    (GLUE_GET_PKT_PRIVATE_DATA(_p)->u2Flag |= BIT(_flag))

#define GLUE_TEST_PKT_FLAG(_p, _flag) \
    (GLUE_GET_PKT_PRIVATE_DATA(_p)->u2Flag & BIT(_flag))

#define GLUE_IS_PKT_FLAG_SET(_p)       (GLUE_GET_PKT_PRIVATE_DATA(_p)->u2Flag)

#define GLUE_SET_PKT_BSS_IDX(_p, _ucBssIndex) \
    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucBssIdx = (u8)(_ucBssIndex))

#define GLUE_GET_PKT_BSS_IDX(_p)       (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucBssIdx)

#define GLUE_SET_PKT_HEADER_LEN(_p, _ucMacHeaderLen) \
    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucHeaderLen = (u8)(_ucMacHeaderLen))

#define GLUE_GET_PKT_HEADER_LEN(_p)    (GLUE_GET_PKT_PRIVATE_DATA( \
                                            _p)->ucHeaderLen)

#define GLUE_SET_PKT_FRAME_LEN(_p, _u2PayloadLen) \
    (GLUE_GET_PKT_PRIVATE_DATA(_p)->u2FrameLen = (u16)(_u2PayloadLen))

#define GLUE_GET_PKT_FRAME_LEN(_p)     (GLUE_GET_PKT_PRIVATE_DATA( \
                                            _p)->u2FrameLen)

#define GLUE_SET_PKT_ARRIVAL_TIME(_p, _rSysTime) \
    (GLUE_GET_PKT_PRIVATE_DATA(_p)->rArrivalTime = (u32)(_rSysTime))

#define GLUE_GET_PKT_ARRIVAL_TIME(_p) \
    (GLUE_GET_PKT_PRIVATE_DATA(_p)->rArrivalTime)

#define GLUE_SET_PKT_IP_ID(_p, _u2IpId) \
    (GLUE_GET_PKT_PRIVATE_DATA(_p)->u2IpId = (u16)(_u2IpId))

#define GLUE_GET_PKT_IP_ID(_p)         (GLUE_GET_PKT_PRIVATE_DATA(_p)->u2IpId)

#define GLUE_SET_PKT_FLAG_PROF_MET(_p) \
    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucProfilingFlag |= BIT(0))

#define GLUE_GET_PKT_IS_PROF_MET(_p) \
    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucProfilingFlag & BIT(0))

#define GLUE_GET_PKT_ETHER_DEST_ADDR(_p) \
    ((u8 *)&(((struct sk_buff *)(_p))->data))

/* Check validity of prDev, private data, and pointers */
#define GLUE_CHK_DEV(prDev) \
    ((prDev && *((P_GLUE_INFO_T *)netdev_priv(prDev))) ? true : false)

#define GLUE_CHK_PR2(prDev,                                 \
                     pr2)          ((GLUE_CHK_DEV(prDev) && \
                                     pr2) ? true : false)

#define GLUE_CHK_PR3(prDev, pr2, pr3) \
    ((GLUE_CHK_PR2(prDev, pr2) && pr3) ? true : false)

#define GLUE_CHK_PR4(prDev, pr2, pr3, pr4) \
    ((GLUE_CHK_PR3(prDev, pr2, pr3) && pr4) ? true : false)

#define GLUE_SET_EVENT(pr)         kalSetEvent(pr)

#define GLUE_INC_REF_CNT(_refCount)    atomic_inc((atomic_t *)&(_refCount))
#define GLUE_DEC_REF_CNT(_refCount)    atomic_dec((atomic_t *)&(_refCount))
#define GLUE_GET_REF_CNT(_refCount)    atomic_read((atomic_t *)&(_refCount))

#define DbgPrint(...)

/*----------------------------------------------------------------------------*/
/* Macros of Data Type Check                                                  */
/*----------------------------------------------------------------------------*/
/* Kevin: we don't have to call following function to inspect the data
 * structure. It will check automatically while at compile time.
 */
static __KAL_INLINE__ void glPacketDataTypeCheck(void)
{
    DATA_STRUCT_INSPECTING_ASSERT(sizeof(PACKET_PRIVATE_DATA) <=
                                  sizeof(((struct sk_buff *)0)->cb));
}

static inline u16 mtk_wlan_ndev_select_queue(struct sk_buff *skb)
{
    static u16 ieee8021d_to_queue[8] = { 1, 0, 0, 1, 2, 2, 3, 3 };

    /* cfg80211_classify8021d returns 0~7 */
    skb->priority = cfg80211_classify8021d(skb, NULL);

    return ieee8021d_to_queue[skb->priority];
}

#if KERNEL_VERSION(2, 6, 34) > LINUX_VERSION_CODE
#define netdev_for_each_mc_addr(mclist, dev) \
    for (mclist = dev->mc_list; mclist; mclist = mclist->next)
#endif

#if KERNEL_VERSION(2, 6, 34) > LINUX_VERSION_CODE
#define GET_ADDR(ha)    (ha->da_addr)
#else
#define GET_ADDR(ha)    (ha->addr)
#endif

#if KERNEL_VERSION(2, 6, 35) <= LINUX_VERSION_CODE
#define LIST_FOR_EACH_IPV6_ADDR(_prIfa, _ip6_ptr) \
    list_for_each_entry(                          \
        _prIfa, &((struct inet6_dev *)_ip6_ptr)->addr_list, if_list)
#else
#define LIST_FOR_EACH_IPV6_ADDR(_prIfa, _ip6_ptr)                    \
    for (_prIfa = ((struct inet6_dev *)_ip6_ptr)->addr_list; _prIfa; \
         _prIfa = _prIfa->if_next)
#endif

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

#if WLAN_INCLUDE_PROC
s32 procCreateFsEntry(P_GLUE_INFO_T prGlueInfo);
s32 procRemoveProcfs(void);

s32 procInitFs(void);
s32 procUninitProcFs(void);

s32 procInitProcfs(struct net_device *prDev, char *pucDevName);
#endif

#if CFG_ENABLE_WIFI_DIRECT
void p2pSetMulticastListWorkQueueWrapper(P_GLUE_INFO_T prGlueInfo);
#endif

P_GLUE_INFO_T wlanGetGlueInfo(void);

#if KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE
u16 wlanSelectQueue(struct net_device *dev,
                    struct sk_buff *skb,
                    struct net_device *sb_dev);
#elif KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE
u16 wlanSelectQueue(struct net_device *dev,
                    struct sk_buff *skb,
                    struct net_device *sb_dev,
                    select_queue_fallback_t fallback);
#else
u16 wlanSelectQueue(struct net_device *dev,
                    struct sk_buff *skb,
                    void *accel_priv,
                    select_queue_fallback_t fallback);
#endif

void wlanDebugInit(void);

WLAN_STATUS wlanSetDebugLevel(IN u32 u4DbgIdx, IN u32 u4DbgMask);

WLAN_STATUS wlanGetDebugLevel(IN u32 u4DbgIdx, OUT u32 *pu4DbgMask);

void wlanSetSuspendMode(P_GLUE_INFO_T prGlueInfo, u8 fgEnable);

void wlanGetConfig(P_ADAPTER_T prAdapter);

WLAN_STATUS wlanExtractBufferBin(P_ADAPTER_T prAdapter);

struct wireless_dev *wlanGetWirelessDevice(void);
/*******************************************************************************
 *  E X T E R N A L   F U N C T I O N S / V A R I A B L E
 *******************************************************************************
 */

extern struct net_device *gPrP2pDev[KAL_P2P_NUM];
extern struct net_device *gPrDev;
extern struct wireless_dev *gprWdev;

#ifdef CFG_DRIVER_INF_NAME_CHANGE
extern char *gprifnameap;
extern char *gprifnamep2p;
extern char *gprifnamesta;
#endif

extern void wlanRegisterNotifier(void);
extern void wlanUnregisterNotifier(void);

extern void wlanRegisterRebootNotifier(void);
extern void wlanUnregisterRebootNotifier(void);

#if CFG_ENABLE_EARLY_SUSPEND
extern int glRegisterEarlySuspend(struct early_suspend *prDesc,
                                  early_suspend_callback wlanSuspend,
                                  late_resume_callback wlanResume);

extern int glUnregisterEarlySuspend(struct early_suspend *prDesc);
#endif

#if CFG_MET_PACKET_TRACE_SUPPORT
void kalMetTagPacket(IN P_GLUE_INFO_T prGlueInfo,
                     IN P_NATIVE_PACKET prPacket,
                     IN ENUM_TX_PROFILING_TAG_T eTag);

void kalMetInit(IN P_GLUE_INFO_T prGlueInfo);
#endif

void wlanUpdateChannelTable(P_GLUE_INFO_T prGlueInfo);

#if CFG_SUPPORT_SAP_DFS_CHANNEL
void wlanUpdateDfsChannelTable(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucChannel);
#endif

int set_p2p_mode_handler(struct net_device *netdev,
                         PARAM_CUSTOM_P2P_SET_STRUCT_T p2pmode);

#if CFG_ENABLE_UNIFY_WIPHY
const struct net_device_ops *wlanGetNdevOps(void);
#endif

#endif  /* _GL_OS_H */
