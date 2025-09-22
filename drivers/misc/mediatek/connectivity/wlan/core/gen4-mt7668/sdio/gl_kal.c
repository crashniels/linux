// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_kal.c
 *    \brief  GLUE Layer will export the required procedures here for internal
 * driver stack.
 *
 *    This file contains all routines which are exported from GLUE Layer to
 * internal driver stack.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include <linux/ctype.h>
#include <net/netlink.h>

#include "gl_os.h"
#include "gl_kal.h"
#include "gl_wext.h"
#include "precomp.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define FILE_NAME_MAX \
    CFG_FW_NAME_MAX_LEN  /* the maximum length of a file name */
#define FILE_NAME_TOTAL    8  /* the maximum number of all possible file name */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

#if DBG
int allocatedMemSize;
#endif

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

static void *pvIoBuffer;
static u32 pvIoBufferSize;
static u32 pvIoBufferUsage;

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#if CFG_ENABLE_FW_DOWNLOAD

#define KUIDT_VALUE(v)    (v.val)
#define KGIDT_VALUE(v)    (v.val)

const struct firmware *fw_entry;

/* Default */
static u8 *apucFwName[] = { (u8 *)CFG_FW_FILENAME "_MT", NULL };

static u8 *apucCr4FwName[] = { (u8 *)CFG_CR4_FW_FILENAME "_" HIF_NAME "_MT",
                               (u8 *)CFG_CR4_FW_FILENAME "_MT", NULL };

static u8 *apucPatchName[] = { (u8 *)"mt6632_patch_e1_hdr.bin",
                               (u8 *)"mt7666_patch_e1_hdr.bin", NULL };

#if !DBG_DISABLE_ALL_LOG
static u8 **appucFwNameTable[] = { apucFwName };
#endif
#if CFG_ASSERT_DUMP
/* Core dump debug usage */

u8 *apucCorDumpN9FileName = "/tmp/FW_DUMP_N9";
u8 *apucCorDumpCr4FileName = "/tmp/FW_DUMP_Cr4";
#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        open firmware image in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS kalFirmwareOpen(IN P_GLUE_INFO_T prGlueInfo, IN u8 **apucNameTable)
{
    u8 ucNameIdx;
    /* u8 ** apucNameTable; */
#if !DBG_DISABLE_ALL_LOG
    u8 ucMaxEcoVer = (sizeof(appucFwNameTable) / sizeof(u8 * *));
    u8 ucCurEcoVer = wlanGetEcoVersion(prGlueInfo->prAdapter);
#endif
    u8 fgResult = false;
    int ret;

    /* Try to open FW binary */
    for (ucNameIdx = 0; apucNameTable[ucNameIdx]; ucNameIdx++) {
        /*
         * Driver support request_firmware() to get files
         * Android path: "/etc/firmware", "/vendor/firmware",
         * "/firmware/image" Linux path: "/lib/firmware",
         * "/lib/firmware/update"
         */
        ret = REQUEST_FIRMWARE(&fw_entry, apucNameTable[ucNameIdx],
                               prGlueInfo->prDev);

        if (ret) {
            DBGLOG(INIT, TRACE,
                   "Request FW image: %s failed, errno[%d]\n",
                   apucNameTable[ucNameIdx], fgResult);
            RELEASE_FIRMWARE(fw_entry);
            continue;
        } else {
            DBGLOG(INIT, TRACE, "Request FW image: %s done\n",
                   apucNameTable[ucNameIdx]);
            fgResult = true;
            break;
        }
    }

    /* Check result */
    if (!fgResult) {
        goto error_open;
    }

    return WLAN_STATUS_SUCCESS;

error_open:
    DBGLOG(INIT, ERROR,
           "Request FW image failed! Cur/Max ECO Ver[E%u/E%u]\n",
           ucCurEcoVer, ucMaxEcoVer);

    return WLAN_STATUS_FAILURE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        release firmware image in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS kalFirmwareClose(IN P_GLUE_INFO_T prGlueInfo)
{
    RELEASE_FIRMWARE(fw_entry);

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        load firmware image in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS kalFirmwareLoad(IN P_GLUE_INFO_T prGlueInfo, OUT void *prBuf,
                            IN u32 u4Offset, OUT u32 *pu4Size)
{
    ASSERT(prGlueInfo);
    ASSERT(pu4Size);
    ASSERT(prBuf);

    if ((fw_entry == NULL) || (fw_entry->size == 0) ||
        (fw_entry->data == NULL)) {
        goto error_read;
    } else {
        memcpy(prBuf, fw_entry->data, fw_entry->size);
        *pu4Size = fw_entry->size;
    }

    return WLAN_STATUS_SUCCESS;

error_read:
    return WLAN_STATUS_FAILURE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        query firmware image size in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/

WLAN_STATUS kalFirmwareSize(IN P_GLUE_INFO_T prGlueInfo, OUT u32 *pu4Size)
{
    ASSERT(prGlueInfo);
    ASSERT(pu4Size);

    *pu4Size = fw_entry->size;

    return WLAN_STATUS_SUCCESS;
}

void kalConstructDefaultFirmwarePrio(P_GLUE_INFO_T prGlueInfo,
                                     u8 **apucNameTable, u8 **apucName,
                                     u8 *pucNameIdx, u8 ucMaxNameIdx)
{
    struct chip_info *prChipInfo = prGlueInfo->prAdapter->chip_info;
    u32 chip_id = prChipInfo->chip_id;
    u8 sub_idx = 0;

    for (sub_idx = 0; apucNameTable[sub_idx]; sub_idx++) {
        if ((*pucNameIdx + 3) < ucMaxNameIdx) {
            /* Type 1. WIFI_RAM_CODE_MTxxxx_Ex */
            snprintf(*(apucName + (*pucNameIdx)), FILE_NAME_MAX,
                     "%s%x_E%u", apucNameTable[sub_idx], chip_id,
                     wlanGetEcoVersion(prGlueInfo->prAdapter));
            (*pucNameIdx) += 1;

            /* Type 2. WIFI_RAM_CODE_MTxxxx_Ex.bin */
            snprintf(*(apucName + (*pucNameIdx)), FILE_NAME_MAX,
                     "%s%x_E%u.bin", apucNameTable[sub_idx],
                     chip_id,
                     wlanGetEcoVersion(prGlueInfo->prAdapter));
            (*pucNameIdx) += 1;

            /* Type 3. WIFI_RAM_CODE_MTxxxx */
            snprintf(*(apucName + (*pucNameIdx)), FILE_NAME_MAX,
                     "%s%x", apucNameTable[sub_idx], chip_id);
            (*pucNameIdx) += 1;

            /* Type 4. WIFI_RAM_CODE_MTxxxx.bin */
            snprintf(*(apucName + (*pucNameIdx)), FILE_NAME_MAX,
                     "%s%x.bin", apucNameTable[sub_idx], chip_id);
            (*pucNameIdx) += 1;
        } else {
            /* the table is not large enough */
            DBGLOG(INIT,
                   ERROR,
                   "kalFirmwareImageMapping >> file name array is not enough.\n");
            ASSERT(0);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to load firmware image
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 * \param ppvMapFileBuf  Pointer of pointer to memory-mapped firmware image
 * \param pu4FileLength  File length and memory mapped length as well
 *
 * \retval Map File Handle, used for unammping
 */
/*----------------------------------------------------------------------------*/

void *kalFirmwareImageMapping(IN P_GLUE_INFO_T prGlueInfo,
                              OUT void **ppvMapFileBuf, OUT u32 *pu4FileLength,
                              IN ENUM_IMG_DL_IDX_T eDlIdx)
{
    u8 **apucNameTable = NULL;
    u8 *apucName[FILE_NAME_TOTAL + 1];  /* extra +1, for the purpose of
                                         * detecting the end of the array */
    u8 idx = 0, max_idx, ucRomVer = 0,
       aucNameBody[FILE_NAME_TOTAL][FILE_NAME_MAX], sub_idx = 0;
    struct chip_info *prChipInfo = prGlueInfo->prAdapter->chip_info;
    u32 chip_id = prChipInfo->chip_id;

    DEBUGFUNC("kalFirmwareImageMapping");

    ASSERT(prGlueInfo);
    ASSERT(ppvMapFileBuf);
    ASSERT(pu4FileLength);

    *ppvMapFileBuf = NULL;
    *pu4FileLength = 0;

    do {
        /* <0.0> Get FW name prefix table */
        switch (eDlIdx) {
        case IMG_DL_IDX_N9_FW:
            apucNameTable = apucFwName;
            break;

        case IMG_DL_IDX_CR4_FW:
            apucNameTable = apucCr4FwName;
            break;

        case IMG_DL_IDX_PATCH:
            apucNameTable = apucPatchName;
            break;

        default:
            ASSERT(0);
            break;
        }

        /* <0.2> Construct FW name */
        memset(apucName, 0, sizeof(apucName));

        /* magic number 1: reservation for detection
         * of the end of the array
         */
        max_idx = (sizeof(apucName) / sizeof(u8 *)) - 1;

        idx = 0;
        apucName[idx] = (u8 *)(aucNameBody + idx);

        if (eDlIdx == IMG_DL_IDX_PATCH) {
            /* construct the file name for patch */

            /* mtxxxx_patch_ex_hdr.bin*/
            ucRomVer = wlanGetRomVersion(prGlueInfo->prAdapter) + 1;
            snprintf(apucName[idx], FILE_NAME_MAX,
                     "mt%x_patch_e%u_hdr.bin", chip_id, ucRomVer);

            idx += 1;
        } else {
            for (sub_idx = 0; sub_idx < max_idx; sub_idx++)
                apucName[sub_idx] =
                    (u8 *)(aucNameBody + sub_idx);

            if (prChipInfo->constructFirmwarePrio) {
                prChipInfo->constructFirmwarePrio(
                    prGlueInfo, apucNameTable, apucName,
                    &idx, max_idx);
            } else {
                kalConstructDefaultFirmwarePrio(prGlueInfo,
                                                apucNameTable,
                                                apucName, &idx,
                                                max_idx);
            }
        }

        /* let the last pointer point to NULL
         * so that we can detect the end of the array in
         * kalFirmwareOpen().
         */
        apucName[idx] = NULL;

        apucNameTable = apucName;

        /* <1> Open firmware */
        if (kalFirmwareOpen(prGlueInfo, apucNameTable) !=
            WLAN_STATUS_SUCCESS) {
            break;
        }
        {
            u32 u4FwSize = 0;
            void *prFwBuffer = NULL;
            /* <2> Query firmare size */
            kalFirmwareSize(prGlueInfo, &u4FwSize);
            /* <3> Use vmalloc for allocating large memory trunk */
            prFwBuffer = vmalloc(ALIGN_4(u4FwSize));
            /* <4> Load image binary into buffer */
            if (kalFirmwareLoad(prGlueInfo, prFwBuffer, 0,
                                &u4FwSize) != WLAN_STATUS_SUCCESS) {
                vfree(prFwBuffer);
                kalFirmwareClose(prGlueInfo);
                break;
            }
            /* <5> write back info */
            *pu4FileLength = u4FwSize;
            *ppvMapFileBuf = prFwBuffer;

            return prFwBuffer;
        }
    } while (false);

    return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to unload firmware image mapped memory
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 * \param pvFwHandle     Pointer to mapping handle
 * \param pvMapFileBuf   Pointer to memory-mapped firmware image
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/

void kalFirmwareImageUnmapping(IN P_GLUE_INFO_T prGlueInfo, IN void *prFwHandle,
                               IN void *pvMapFileBuf)
{
    DEBUGFUNC("kalFirmwareImageUnmapping");

    ASSERT(prGlueInfo);

    /* pvMapFileBuf might be NULL when file doesn't exist */
    if (pvMapFileBuf) {
        vfree(pvMapFileBuf);
    }

    kalFirmwareClose(prGlueInfo);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 * acquire OS SPIN_LOCK.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rLockCategory  Specify which SPIN_LOCK
 * \param[out] pu4Flags      Pointer of a variable for saving IRQ flags
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalAcquireSpinLock(IN P_GLUE_INFO_T prGlueInfo,
                        IN ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
                        OUT unsigned long *plFlags)
{
    unsigned long ulFlags = 0;

    ASSERT(prGlueInfo);
    ASSERT(plFlags);

    if (rLockCategory < SPIN_LOCK_NUM) {
        //DBGLOG(INIT, LOUD, "SPIN_LOCK[%u] Try to acquire\n",
        //       rLockCategory);
#if CFG_USE_SPIN_LOCK_BOTTOM_HALF
        spin_lock_bh(&prGlueInfo->rSpinLock[rLockCategory]);
#else  /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */
        spin_lock_irqsave(&prGlueInfo->rSpinLock[rLockCategory],
                          ulFlags);
#endif

        *plFlags = ulFlags;

        //DBGLOG(INIT, LOUD, "SPIN_LOCK[%u] Acquired\n", rLockCategory);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 * release OS SPIN_LOCK.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rLockCategory  Specify which SPIN_LOCK
 * \param[in] u4Flags        Saved IRQ flags
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalReleaseSpinLock(IN P_GLUE_INFO_T prGlueInfo,
                        IN ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
                        IN unsigned long ulFlags)
{
    ASSERT(prGlueInfo);

    if (rLockCategory < SPIN_LOCK_NUM) {
#if CFG_USE_SPIN_LOCK_BOTTOM_HALF
        spin_unlock_bh(&prGlueInfo->rSpinLock[rLockCategory]);
#else  /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */
        spin_unlock_irqrestore(&prGlueInfo->rSpinLock[rLockCategory],
                               ulFlags);
#endif
        //DBGLOG(INIT, LOUD, "SPIN_UNLOCK[%u]\n", rLockCategory);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 * acquire OS MUTEX.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rMutexCategory  Specify which MUTEX
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalAcquireMutex(IN P_GLUE_INFO_T prGlueInfo,
                     IN ENUM_MUTEX_CATEGORY_E rMutexCategory)
{
    ASSERT(prGlueInfo);

    if (rMutexCategory < MUTEX_NUM) {
        DBGLOG(INIT, TRACE, "MUTEX_LOCK[%u] Try to acquire\n",
               rMutexCategory);
        mutex_lock(&prGlueInfo->arMutex[rMutexCategory]);
        DBGLOG(INIT, TRACE, "MUTEX_LOCK[%u] Acquired\n",
               rMutexCategory);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 * release OS MUTEX.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rMutexCategory  Specify which MUTEX
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalReleaseMutex(IN P_GLUE_INFO_T prGlueInfo,
                     IN ENUM_MUTEX_CATEGORY_E rMutexCategory)
{
    ASSERT(prGlueInfo);

    if (rMutexCategory < MUTEX_NUM) {
        mutex_unlock(&prGlueInfo->arMutex[rMutexCategory]);
        DBGLOG(INIT, TRACE, "MUTEX_UNLOCK[%u]\n", rMutexCategory);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        acquire OS MUTEX for wdev.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalAcquireWDevMutex(IN struct net_device *pDev)
{
    ASSERT(pDev);

    /* for user build */
    if (pDev == NULL) {
        return;
    }

    DBGLOG(INIT, TEMP, "WDEV_LOCK Try to acquire\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0)
    mutex_lock(&(pDev->ieee80211_ptr)->mtx);
#else
    wiphy_lock(pDev->ieee80211_ptr->wiphy);
#endif
    DBGLOG(INIT, TEMP, "WDEV_LOCK Acquired\n");
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        release OS MUTEXfor wdev.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalReleaseWDevMutex(IN struct net_device *pDev)
{
    ASSERT(pDev);

    /* for user build */
    if (pDev == NULL) {
        return;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0)
    mutex_unlock(&(pDev->ieee80211_ptr)->mtx);
#else
    wiphy_unlock(pDev->ieee80211_ptr->wiphy);
#endif
    DBGLOG(INIT, TEMP, "WDEV_UNLOCK\n");
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 * update current MAC address.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pucMacAddr     Pointer of current MAC address
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalUpdateMACAddress(IN P_GLUE_INFO_T prGlueInfo, IN u8 *pucMacAddr)
{
    ASSERT(prGlueInfo);
    ASSERT(pucMacAddr);

    if (UNEQUAL_MAC_ADDR(prGlueInfo->prDevHandler->dev_addr, pucMacAddr)) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
        memcpy(prGlueInfo->prDevHandler->dev_addr, pucMacAddr,
               PARAM_MAC_ADDR_LEN);
#else
        dev_addr_set(prGlueInfo->prDevHandler, pucMacAddr);
#endif
#if CFG_MESON_G12A_PATCH
        prGlueInfo->prDevHandler->mtu = 1408;
#endif
    }
}

#if CFG_TCP_IP_CHKSUM_OFFLOAD
/*----------------------------------------------------------------------------*/
/*!
 * \brief To query the packet information for offload related parameters.
 *
 * \param[in] pvPacket Pointer to the packet descriptor.
 * \param[in] pucFlag  Points to the offload related parameter.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalQueryTxChksumOffloadParam(IN void *pvPacket, OUT u8 *pucFlag)
{
    struct sk_buff *skb = (struct sk_buff *)pvPacket;
    u8 ucFlag = 0;

    ASSERT(pvPacket);
    ASSERT(pucFlag);

    if (skb->ip_summed == CHECKSUM_PARTIAL) {
#if DBG
        /* Kevin: do double check, we can remove this part in Normal
         * Driver. Because we register NIC feature with NETIF_F_IP_CSUM
         * for MT5912B MAC, so we'll process IP packet only.
         */
        if (skb->protocol != htons(ETH_P_IP)) {
        } else
#endif
        ucFlag |= (TX_CS_IP_GEN | TX_CS_TCP_UDP_GEN);
    }

    *pucFlag = ucFlag;
}

/* 4 2007/10/8, mikewu, this is rewritten by Mike */
/*----------------------------------------------------------------------------*/
/*!
 * \brief To update the checksum offload status to the packet to be indicated to
 * OS.
 *
 * \param[in] pvPacket Pointer to the packet descriptor.
 * \param[in] pucFlag  Points to the offload related parameter.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalUpdateRxCSUMOffloadParam(IN void *pvPacket,
                                 IN ENUM_CSUM_RESULT_T aeCSUM[])
{
    struct sk_buff *skb = (struct sk_buff *)pvPacket;

    ASSERT(pvPacket);

    if ((aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_SUCCESS ||
         aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_SUCCESS) &&
        ((aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_SUCCESS) ||
         (aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_SUCCESS))) {
        skb->ip_summed = CHECKSUM_UNNECESSARY;
    } else {
        skb->ip_summed = CHECKSUM_NONE;
#if DBG
        if (aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_NONE &&
            aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_NONE) {
            DBGLOG(RX, TRACE, "RX: \"non-IPv4/IPv6\" Packet\n");
        }else if (aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_FAILED) {
            DBGLOG(RX, TRACE, "RX: \"bad IP Checksum\" Packet\n");
        }else if (aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_FAILED) {
            DBGLOG(RX, TRACE, "RX: \"bad TCP Checksum\" Packet\n");
        }else if (aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_FAILED) {
            DBGLOG(RX, TRACE, "RX: \"bad UDP Checksum\" Packet\n");
        }
#endif
    }
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is called to free packet allocated from kalPacketAlloc.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of the packet descriptor
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalPacketFree(IN P_GLUE_INFO_T prGlueInfo, IN void *pvPacket)
{
    dev_kfree_skb((struct sk_buff *)pvPacket);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param prGlueInfo   Pointer of GLUE Data Structure
 * \param u4Size       Pointer of Packet Handle
 * \param ppucData     Status Code for OS upper layer
 *
 * \return NULL: Failed to allocate skb, Not NULL get skb
 */
/*----------------------------------------------------------------------------*/
void *kalPacketAlloc(IN P_GLUE_INFO_T prGlueInfo, IN u32 u4Size,
                     OUT u8 **ppucData)
{
    struct sk_buff *prSkb;

    if (in_interrupt()) {
        prSkb = __dev_alloc_skb(u4Size + NIC_TX_HEAD_ROOM,
                                GFP_ATOMIC | __GFP_NOWARN |
                                __GFP_NORETRY);
    } else {
        prSkb = __dev_alloc_skb(u4Size + NIC_TX_HEAD_ROOM,
                                GFP_KERNEL | __GFP_NORETRY);
    }

    if (prSkb) {
        skb_reserve(prSkb, NIC_TX_HEAD_ROOM);

        *ppucData = (u8 *)(prSkb->data);

        /* DBGLOG(TDLS, INFO, "kalPacketAlloc, skb head[0x%x] data[0x%x]
         * tail[0x%x] end[0x%x]\n", prSkb->head, prSkb->data,
         * prSkb->tail, prSkb->end);
         */

        kalResetPacket(prGlueInfo, (P_NATIVE_PACKET)prSkb);
    }
#if DBG
    {
        u32 *pu4Head = (u32 *)&prSkb->cb[0];
        *pu4Head = (u32)prSkb->head;
        DBGLOG(RX, TRACE, "prSkb->head = %#lx, prSkb->cb = %#lx\n",
               (u32)prSkb->head, *pu4Head);
    }
#endif
    return (void *)prSkb;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Process the received packet for indicating to OS.
 *
 * \param[in] prGlueInfo     Pointer to the Adapter structure.
 * \param[in] pvPacket       Pointer of the packet descriptor
 * \param[in] pucPacketStart The starting address of the buffer of Rx packet.
 * \param[in] u4PacketLen    The packet length.
 * \param[in] pfgIsRetain    Is the packet to be retained.
 * \param[in] aerCSUM        The result of TCP/ IP checksum offload.
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
kalProcessRxPacket(IN P_GLUE_INFO_T prGlueInfo, IN void *pvPacket,
                   IN u8 *pucPacketStart, IN u32 u4PacketLen,
                   IN ENUM_CSUM_RESULT_T aerCSUM[])
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    struct sk_buff *skb = (struct sk_buff *)pvPacket;

    skb->data = (unsigned char *)pucPacketStart;

    /* Reset skb */
    skb_reset_tail_pointer(skb);
    skb_trim(skb, 0);

    /* Put data */
    skb_put(skb, u4PacketLen);

#if CFG_TCP_IP_CHKSUM_OFFLOAD
    if (prGlueInfo->prAdapter->fgIsSupportCsumOffload) {
        kalUpdateRxCSUMOffloadParam(skb, aerCSUM);
    }
#endif

    return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief To indicate an array of received packets is available for higher
 *        level protocol uses.
 *
 * \param[in] prGlueInfo Pointer to the Adapter structure.
 * \param[in] apvPkts The packet array to be indicated
 * \param[in] ucPktNum The number of packets to be indicated
 *
 * \retval true Success.
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS kalRxIndicatePkts(IN P_GLUE_INFO_T prGlueInfo, IN void *apvPkts[],
                              IN u8 ucPktNum)
{
    u8 ucIdx = 0;

    ASSERT(prGlueInfo);
    ASSERT(apvPkts);

    for (ucIdx = 0; ucIdx < ucPktNum; ucIdx++)
        kalRxIndicateOnePkt(prGlueInfo, apvPkts[ucIdx]);

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief To indicate one received packets is available for higher
 *        level protocol uses.
 *
 * \param[in] prGlueInfo Pointer to the Adapter structure.
 * \param[in] pvPkt The packet to be indicated
 *
 * \retval true Success.
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS kalRxIndicateOnePkt(IN P_GLUE_INFO_T prGlueInfo, IN void *pvPkt)
{
    struct net_device *prNetDev = prGlueInfo->prDevHandler;
    struct sk_buff *prSkb = NULL;
    struct chip_info *prChipInfo;

    ASSERT(prGlueInfo);
    ASSERT(pvPkt);

    prSkb = pvPkt;
    prChipInfo = prGlueInfo->prAdapter->chip_info;

    prNetDev = (struct net_device *)wlanGetNetInterfaceByBssIdx(
        prGlueInfo, GLUE_GET_PKT_BSS_IDX(prSkb));
    if (!prNetDev) {
        prNetDev = prGlueInfo->prDevHandler;
    }
#if CFG_SUPPORT_SNIFFER
    if (prGlueInfo->fgIsEnableMon) {
        prNetDev = prGlueInfo->prMonDevHandler;
    }
#endif
    prNetDev->stats.rx_bytes += prSkb->len;
    prNetDev->stats.rx_packets++;

#if KERNEL_VERSION(4, 11, 0) > CFG80211_VERSION_CODE
    prNetDev->last_rx = jiffies;
#endif
#if CFG_SUPPORT_SNIFFER
    if (prGlueInfo->fgIsEnableMon) {
        skb_reset_mac_header(prSkb);
        prSkb->ip_summed = CHECKSUM_UNNECESSARY;
        prSkb->pkt_type = PACKET_OTHERHOST;
        prSkb->protocol = htons(ETH_P_802_2);
    } else {
        prSkb->protocol = eth_type_trans(prSkb, prNetDev);
    }
#else
    prSkb->protocol = eth_type_trans(prSkb, prNetDev);
#endif
    prSkb->dev = prNetDev;
    /* DBGLOG_MEM32(RX, TRACE, (u32 *)prSkb->data, prSkb->len); */
    /* DBGLOG(RX, EVENT, ("kalRxIndicatePkts len = %d\n", prSkb->len)); */
    if (prSkb->tail > prSkb->end) {
        DBGLOG(RX,
               ERROR,
               "kalRxIndicateOnePkt [prSkb = 0x%p][prSkb->len = %d][prSkb->protocol = 0x%02X] %p,%p\n",
               (u8 *)prSkb,
               prSkb->len,
               prSkb->protocol,
               prSkb->tail,
               prSkb->end);
        DBGLOG_MEM32(RX, ERROR, (u32 *)prSkb->data, prSkb->len);
    }

    if (prSkb->protocol == NTOHS(ETH_P_8021Q) &&
        !FEAT_SUP_LLC_VLAN_RX(prChipInfo)) {
        /*
         *  DA-MAC + SA-MAC + 0x8100 was removed in eth_type_trans()
         *  pkt format here is TCI(2-bytes) + Len(2-btyes) +
         * payload-type(2-bytes) + payload Remove "Len" field inserted
         * by RX VLAN header translation Note: TCI+payload-type is a
         * standard 8021Q header
         *
         *  This format update is based on RX VLAN HW header
         * translation. If the setting was changed, you may need to
         * change rules here as well.
         */
        const u8 vlan_skb_mem_move = 2;

        /* Remove "Len" and shift data pointer 2 bytes */
        kalMemCopy(prSkb->data + vlan_skb_mem_move, prSkb->data,
                   vlan_skb_mem_move);
        skb_pull_rcsum(prSkb, vlan_skb_mem_move);

        /* Have to update MAC header properly. Otherwise, wrong MACs
         * woud be passed up */
        kalMemMove(prSkb->data - ETH_HLEN,
                   prSkb->data - ETH_HLEN - vlan_skb_mem_move,
                   ETH_HLEN);
        prSkb->mac_header += vlan_skb_mem_move;

        skb_reset_network_header(prSkb);
        skb_reset_transport_header(prSkb);
        kal_skb_reset_mac_len(prSkb);
    }

    if (prSkb->protocol == HTONS(ETH_P_1X)) {
        DBGLOG(RSN, WARN, "Rx EAPOL Frame [Len: %d]\n", prSkb->len);
    }

#if (KERNEL_VERSION(5, 18, 0) >= CFG80211_VERSION_CODE)
    if (!in_interrupt()) {
        netif_rx_ni(prSkb);  /* only in non-interrupt context */
    }else{
        netif_rx(prSkb);
    }
#else
    netif_rx(prSkb);
#endif

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Called by driver to indicate event to upper layer, for example, the
 * wpa supplicant or wireless tools.
 *
 * \param[in] pvAdapter Pointer to the adapter descriptor.
 * \param[in] eStatus Indicated status.
 * \param[in] pvBuf Indicated message buffer.
 * \param[in] u4BufLen Indicated message buffer size.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalIndicateStatusAndComplete(IN P_GLUE_INFO_T prGlueInfo,
                                  IN WLAN_STATUS eStatus, IN void *pvBuf,
                                  IN u32 u4BufLen)
{
    u32 bufLen;
    P_PARAM_STATUS_INDICATION_T pStatus =
        (P_PARAM_STATUS_INDICATION_T)pvBuf;
    P_PARAM_AUTH_EVENT_T pAuth = (P_PARAM_AUTH_EVENT_T)pStatus;
    P_PARAM_PMKID_CANDIDATE_LIST_T pPmkid =
        (P_PARAM_PMKID_CANDIDATE_LIST_T)(pStatus + 1);
    PARAM_MAC_ADDRESS arBssid;
    struct cfg80211_scan_request *prScanRequest = NULL;
    PARAM_SSID_T ssid;
    struct ieee80211_channel *prChannel = NULL;
    struct cfg80211_bss *bss;
    u8 ucChannelNum;
    P_BSS_DESC_T prBssDesc = NULL;
    gfp_t flags = GFP_KERNEL;

    GLUE_SPIN_LOCK_DECLARATION();

    kalMemZero(arBssid, MAC_ADDR_LEN);

    ASSERT(prGlueInfo);

    switch (eStatus) {
    case WLAN_STATUS_ROAM_OUT_FIND_BEST:
    case WLAN_STATUS_MEDIA_CONNECT:

        prGlueInfo->eParamMediaStateIndicated =
            PARAM_MEDIA_STATE_CONNECTED;

        /* indicate assoc event */
        wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQueryBssid,
                             &arBssid[0], sizeof(arBssid), &bufLen);
        wext_indicate_wext_event(prGlueInfo, SIOCGIWAP, arBssid,
                                 bufLen);

        /* switch netif on */
        netif_carrier_on(prGlueInfo->prDevHandler);
        DBGLOG(INIT, INFO,
               "Skip report CONNECTED when using supplicant SME\n");
        return;

        do {
            /* print message on console */
            wlanQueryInformation(prGlueInfo->prAdapter,
                                 wlanoidQuerySsid, &ssid,
                                 sizeof(ssid), &bufLen);

            ssid.aucSsid[(ssid.u4SsidLen >= PARAM_MAX_LEN_SSID) ?
                         (PARAM_MAX_LEN_SSID - 1) :
                         ssid.u4SsidLen] = '\0';
            DBGLOG(INIT, INFO,
                   "[wifi] %s netif_carrier_on [ssid:%s " MACSTR
                   "]\n",
                   prGlueInfo->prDevHandler->name, ssid.aucSsid,
                   MAC2STR(arBssid));
        } while (0);

        if (prGlueInfo->fgIsRegistered == true) {
            /* retrieve channel */
            ucChannelNum = wlanGetChannelNumberByNetwork(
                prGlueInfo->prAdapter,
                prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex);
            if (ucChannelNum <= 14) {
                prChannel = ieee80211_get_channel(
                    priv_to_wiphy(prGlueInfo),
                    ieee80211_channel_to_frequency(
                        ucChannelNum,
                        NL80211_BAND_2GHZ));
            } else {
                prChannel = ieee80211_get_channel(
                    priv_to_wiphy(prGlueInfo),
                    ieee80211_channel_to_frequency(
                        ucChannelNum,
                        NL80211_BAND_5GHZ));
            }

            bss = cfg80211_get_bss(priv_to_wiphy(prGlueInfo),
                                   prChannel, arBssid, ssid.aucSsid,
                                   ssid.u4SsidLen,
                                   IEEE80211_BSS_TYPE_ESS,
                                   IEEE80211_PRIVACY_ANY);
            if (bss == NULL) {
#if (BUILD_DBG_MSG == 1)
                DBGLOG(INIT,
                       EVENT,
                       "Cannot get BSS from cfg80211 [ssid:%s]\n",
                       ssid.aucSsid);
#endif
                /* create BSS on-the-fly */
                prBssDesc =
                    ((P_AIS_FSM_INFO_T)(&(
                                            prGlueInfo->
                                            prAdapter->
                                            rWifiVar
                                            .rAisFsmInfo)))
                    ->prTargetBssDesc;

                if (prChannel && prBssDesc != NULL) {
                    bss = cfg80211_inform_bss(
                        priv_to_wiphy(prGlueInfo),
                        prChannel,
                        CFG80211_BSS_FTYPE_PRESP,
                        arBssid, 0,  /* TSF */
                        prBssDesc->u2CapInfo,
                        prBssDesc->u2BeaconInterval,  /* beacon
                                                       * interval */
                        prBssDesc->aucIEBuf,  /* IE */
                        prBssDesc->u2IELength,  /* IE
                                                 * Length
                                                 */
                        RCPI_TO_dBm(prBssDesc->ucRCPI) *
                        100,  /* MBM */
                        GFP_KERNEL);
                }
#if (BUILD_DBG_MSG == 1)
                if (bss == NULL) {
                    DBGLOG(INIT,
                           EVENT,
                           "cfg80211_inform_bss still failed\n");
                }
#endif
            }
            /* CFG80211 Indication */
            if (eStatus == WLAN_STATUS_ROAM_OUT_FIND_BEST) {
                struct ieee80211_channel *prChannel = NULL;
                u8 ucChannelNum = wlanGetChannelNumberByNetwork(
                    prGlueInfo->prAdapter,
                    prGlueInfo->prAdapter->prAisBssInfo
                    ->ucBssIndex);
#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
                struct cfg80211_roam_info rRoamInfo = {};
#endif
                if (ucChannelNum <= 14) {
                    prChannel = ieee80211_get_channel(
                        priv_to_wiphy(prGlueInfo),
                        ieee80211_channel_to_frequency(
                            ucChannelNum,
                            NL80211_BAND_2GHZ));
                } else {
                    prChannel = ieee80211_get_channel(
                        priv_to_wiphy(prGlueInfo),
                        ieee80211_channel_to_frequency(
                            ucChannelNum,
                            NL80211_BAND_5GHZ));
                }

#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
                rRoamInfo.links[0].channel = prChannel;
                rRoamInfo.links[0].bss = bss;
                rRoamInfo.links[0].bssid = arBssid;
                rRoamInfo.req_ie = prGlueInfo->aucReqIe;
                rRoamInfo.req_ie_len =
                    prGlueInfo->u4RspIeLength;
#if ((KERNEL_VERSION(4, 13, 0) <= CFG80211_VERSION_CODE) && \
                (KERNEL_VERSION(4, 14,                      \
                                0) >= CFG80211_VERSION_CODE))
                rRoamInfo.authorized = false;
#endif
                cfg80211_roamed(prGlueInfo->prDevHandler,
                                &rRoamInfo, GFP_KERNEL);
#else
                cfg80211_roamed(prGlueInfo->prDevHandler,
                                prChannel, arBssid,
                                prGlueInfo->aucReqIe,
                                prGlueInfo->u4ReqIeLength,
                                prGlueInfo->aucRspIe,
                                prGlueInfo->u4RspIeLength,
                                GFP_KERNEL);
#endif
            } else {
                cfg80211_connect_result(
                    prGlueInfo->prDevHandler, arBssid,
                    prGlueInfo->aucReqIe,
                    prGlueInfo->u4ReqIeLength,
                    prGlueInfo->aucRspIe,
                    prGlueInfo->u4RspIeLength,
                    WLAN_STATUS_SUCCESS, GFP_KERNEL);
            }

            /*20180418 frog: we get the bss, we need put it back. */
            if (bss) {
                cfg80211_put_bss(priv_to_wiphy(prGlueInfo),
                                 bss);
            }
        }

        break;

    case WLAN_STATUS_MEDIA_DISCONNECT:
    case WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY:
        /* indicate disassoc event */
        wext_indicate_wext_event(prGlueInfo, SIOCGIWAP, NULL, 0);
        /* For CR 90 and CR99, While supplicant do reassociate, driver
         * will do netif_carrier_off first, after associated success, at
         * joinComplete(), do netif_carier_on, but for unknown reason,
         * the supplicant 1x pkt will not called the driver
         * hardStartXmit, for template workaround these bugs, add this
         * compiling flag
         */
        /* switch netif off */

        DBGLOG(INIT, INFO, "[wifi] %s netif_carrier_off\n",
               prGlueInfo->prDevHandler->name);

        if (prGlueInfo->prAdapter->fgIsChipAssert) {
            flags = GFP_ATOMIC;
        }

        netif_carrier_off(prGlueInfo->prDevHandler);

        /* Report T/RX deauth/disassociation frame */
        DBGLOG(INIT, INFO,
               "Skip report DISCONNECTED when using supplicant SME\n");

        prGlueInfo->eParamMediaStateIndicated =
            PARAM_MEDIA_STATE_DISCONNECTED;
        break;

    case WLAN_STATUS_SCAN_COMPLETE:
        /* indicate scan complete event */
        wext_indicate_wext_event(prGlueInfo, SIOCGIWSCAN, NULL, 0);

        /* 1. reset first for newly incoming request */
        GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
        if (prGlueInfo->prScanRequest != NULL) {
            prScanRequest = prGlueInfo->prScanRequest;
            prGlueInfo->prScanRequest = NULL;
        }
        GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

        /* 2. then CFG80211 Indication */
        if (prScanRequest) {
            kalCfg80211ScanDone(prScanRequest, false);
        }

        break;

    case WLAN_STATUS_MEDIA_SPECIFIC_INDICATION:
        if (pStatus) {
            switch (pStatus->eStatusType) {
            case ENUM_STATUS_TYPE_AUTHENTICATION:
                /*
                 *  printk(KERN_NOTICE
                 * "ENUM_STATUS_TYPE_AUTHENTICATION: L(%ld) ["
                 * MACSTR "] F:%lx\n", pAuth->Request[0].Length,
                 *  MAC2STR(pAuth->Request[0].Bssid),
                 *  pAuth->Request[0].Flags);
                 */
                /* indicate (UC/GC) MIC ERROR event only */
                if ((pAuth->arRequest[0].u4Flags ==
                     PARAM_AUTH_REQUEST_PAIRWISE_ERROR) ||
                    (pAuth->arRequest[0].u4Flags ==
                     PARAM_AUTH_REQUEST_GROUP_ERROR)) {
                    cfg80211_michael_mic_failure(
                        prGlueInfo->prDevHandler, NULL,
                        (pAuth->arRequest[0].u4Flags ==
                         PARAM_AUTH_REQUEST_PAIRWISE_ERROR)
                    ?
                        NL80211_KEYTYPE_PAIRWISE :
                        NL80211_KEYTYPE_GROUP,
                        0, NULL, GFP_KERNEL);
                    wext_indicate_wext_event(
                        prGlueInfo,
                        IWEVMICHAELMICFAILURE,
                        (unsigned char *)&pAuth
                        ->arRequest[0],
                        pAuth->arRequest[0].u4Length);
                }
                break;

            case ENUM_STATUS_TYPE_CANDIDATE_LIST:
                /*
                 *  printk(KERN_NOTICE
                 * "Param_StatusType_PMKID_CandidateList:
                 * Ver(%ld) Num(%ld)\n", pPmkid->u2Version,
                 *  pPmkid->u4NumCandidates);
                 *  if (pPmkid->u4NumCandidates > 0) {
                 *  printk(KERN_NOTICE "candidate[" MACSTR "]
                 * preAuth Flag:%lx\n",
                 *  MAC2STR(pPmkid->arCandidateList[0].rBSSID),
                 *  pPmkid->arCandidateList[0].fgFlags);
                 *  }
                 */
            {
                u32 i;

                for (i = 0; i < pPmkid->u4NumCandidates;
                     i++) {
                    wext_indicate_wext_event(
                        prGlueInfo,
                        IWEVPMKIDCAND,
                        (unsigned char *)&pPmkid
                        ->arCandidateList
                        [i],
                        pPmkid->u4NumCandidates);
                }
            }
            break;

            default:
                /* case ENUM_STATUS_TYPE_MEDIA_STREAM_MODE */
                /*
                 *  printk(KERN_NOTICE "unknown media specific
                 * indication type:%x\n", pStatus->StatusType);
                 */
                break;
            }
        } else {
            /*
             *  printk(KERN_WARNING "media specific indication
             * buffer NULL\n");
             */
        }
        break;

#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS
    case WLAN_STATUS_BWCS_UPDATE: {
        wext_indicate_wext_event(prGlueInfo, IWEVCUSTOM, pvBuf,
                                 sizeof(PTA_IPC_T));
    }

    break;

#endif
    case WLAN_STATUS_JOIN_TIMEOUT:
    case WLAN_STATUS_JOIN_ABORT: {
        P_BSS_DESC_T prBssDesc = prGlueInfo->prAdapter->rWifiVar
                                 .rAisFsmInfo.prTargetBssDesc;
        P_CONNECTION_SETTINGS_T prConnSettings =
            &prGlueInfo->prAdapter->rWifiVar.rConnSettings;
        DBGLOG(INIT, INFO,
               "Skip report CONNECTED when using supplicant SME\n");
        if (prConnSettings->bss) {
            kalWDevLockThread(prGlueInfo, prGlueInfo->prDevHandler,
                              CFG80211_ASSOC_TIMEOUT,
                              NULL, 0, prConnSettings->bss, 0, NULL,
                              0, false);

            cfg80211_put_bss(priv_to_wiphy(prGlueInfo),
                             prConnSettings->bss);
            prConnSettings->bss = NULL;
        }
        return;

        if (prBssDesc) {
            COPY_MAC_ADDR(arBssid, prBssDesc->aucBSSID);
        }
        cfg80211_connect_result(prGlueInfo->prDevHandler, arBssid,
                                prGlueInfo->aucReqIe,
                                prGlueInfo->u4ReqIeLength,
                                prGlueInfo->aucRspIe,
                                prGlueInfo->u4RspIeLength,
                                WLAN_STATUS_AUTH_TIMEOUT, GFP_KERNEL);
        break;
    }

    case WLAN_STATUS_BEACON_TIMEOUT:
        cfg80211_cqm_beacon_loss_notify(prGlueInfo->prDevHandler,
                                        GFP_KERNEL);
        break;

    case WLAN_STATUS_ASSOC_RESP: {
        P_CONNECTION_SETTINGS_T prConnSettings =
            &prGlueInfo->prAdapter->rWifiVar.rConnSettings;

        DBGLOG(INIT, INFO,
               "Report ASSOC response frame when using supplicant SME\n");

        if (prConnSettings->bss) {
            u8 *prFrameBuf = NULL;
            u8 fgIsInterruptContext = false;

            if (in_interrupt()) {
                prFrameBuf =
                    kalMemAlloc(u4BufLen, PHY_MEM_TYPE);
                fgIsInterruptContext = true;
            } else {
                prFrameBuf =
                    kalMemAlloc(u4BufLen, VIR_MEM_TYPE);
                fgIsInterruptContext = false;
            }

            if (!prFrameBuf) {
                DBGLOG(INIT, ERROR,
                       "Alloc buffer for frame failed\n");
                return;
            }

            kalMemCopy((void *)prFrameBuf, (void *)pvBuf, u4BufLen);

            kalWDevLockThread(prGlueInfo, prGlueInfo->prDevHandler,
                              CFG80211_RX_ASSOC_RESP, prFrameBuf,
                              u4BufLen, prConnSettings->bss, 0,
                              NULL, 0, fgIsInterruptContext);

            cfg80211_put_bss(priv_to_wiphy(prGlueInfo),
                             prConnSettings->bss);

            prConnSettings->bss = NULL;
        } else {
            DBGLOG(SAA, WARN,
                   "Rx Assoc Resp without specific BSS\n");
        }
        break;
    }

    default:
        break;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to update the (re)association request
 *        information to the structure used to query and set
 *        OID_802_11_ASSOCIATION_INFORMATION.
 *
 * \param[in] prGlueInfo Pointer to the Glue structure.
 * \param[in] pucFrameBody Pointer to the frame body of the last (Re)Association
 *                         Request frame from the AP.
 * \param[in] u4FrameBodyLen The length of the frame body of the last
 *                           (Re)Association Request frame.
 * \param[in] fgReassocRequest true, if it is a Reassociation Request frame.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalUpdateReAssocReqInfo(IN P_GLUE_INFO_T prGlueInfo,
                             IN u8 *pucFrameBody,
                             IN u32 u4FrameBodyLen, IN u8 fgReassocRequest)
{
    u8 *cp;

    ASSERT(prGlueInfo);

    /* reset */
    prGlueInfo->u4ReqIeLength = 0;

    if (fgReassocRequest) {
        if (u4FrameBodyLen < 15) {
            /*
             *  printk(KERN_WARNING "frameBodyLen too short:%ld\n",
             * frameBodyLen);
             */
            return;
        }
    } else {
        if (u4FrameBodyLen < 9) {
            /*
             *  printk(KERN_WARNING "frameBodyLen too short:%ld\n",
             * frameBodyLen);
             */
            return;
        }
    }

    cp = pucFrameBody;

    if (fgReassocRequest) {
        /* Capability information field 2 */
        /* Listen interval field 2 */
        /* Current AP address 6 */
        cp += 10;
        u4FrameBodyLen -= 10;
    } else {
        /* Capability information field 2 */
        /* Listen interval field 2 */
        cp += 4;
        u4FrameBodyLen -= 4;
    }

    wext_indicate_wext_event(prGlueInfo, IWEVASSOCREQIE, cp,
                             u4FrameBodyLen);

    if (u4FrameBodyLen <= CFG_CFG80211_IE_BUF_LEN) {
        prGlueInfo->u4ReqIeLength = u4FrameBodyLen;
        kalMemCopy(prGlueInfo->aucReqIe, cp, u4FrameBodyLen);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called to update the (re)association
 *        response information to the structure used to reply with
 *        cfg80211_connect_result
 *
 * @param prGlueInfo      Pointer to adapter descriptor
 * @param pucFrameBody    Pointer to the frame body of the last (Re)Association
 *                         Response frame from the AP
 * @param u4FrameBodyLen  The length of the frame body of the last
 *                          (Re)Association Response frame
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void kalUpdateReAssocRspInfo(IN P_GLUE_INFO_T prGlueInfo,
                             IN u8 *pucFrameBody,
                             IN u32 u4FrameBodyLen)
{
    u32 u4IEOffset = 6;  /* cap_info, status_code & assoc_id */
    u32 u4IELength = u4FrameBodyLen - u4IEOffset;

    ASSERT(prGlueInfo);

    /* reset */
    prGlueInfo->u4RspIeLength = 0;

    if (u4IELength <= CFG_CFG80211_IE_BUF_LEN) {
        prGlueInfo->u4RspIeLength = u4IELength;
        kalMemCopy(prGlueInfo->aucRspIe, pucFrameBody + u4IEOffset,
                   u4IELength);
    }
}

void kalResetPacket(IN P_GLUE_INFO_T prGlueInfo,
                    IN P_NATIVE_PACKET prPacket)
{
    struct sk_buff *prSkb = (struct sk_buff *)prPacket;

    /* Reset cb */
    kalMemZero(prSkb->cb, sizeof(prSkb->cb));
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is to check the pairwise eapol and wapi 1x.
 *
 * \param[in] prPacket  Pointer to struct net_device
 *
 * \retval WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
u8 kalIsPairwiseEapolPacket(IN P_NATIVE_PACKET prPacket)
{
    struct sk_buff *prSkb = (struct sk_buff *)prPacket;
    u8 *pucPacket = (u8 *)prSkb->data;
    u16 u2EthType = 0;
    u16 u2KeyInfo = 0;

    WLAN_GET_FIELD_BE16(&pucPacket[ETHER_HEADER_LEN - ETHER_TYPE_LEN],
                        &u2EthType);

    if (u2EthType != ETH_P_1X) {
        return false;
    }

    u2KeyInfo = pucPacket[5 + ETHER_HEADER_LEN] << 8 |
                pucPacket[6 + ETHER_HEADER_LEN];

    /* BIT3 is pairwise key bit, and check SM is 0.  it means this is 4-way
     * handshake frame */
    DBGLOG(RSN, INFO, "u2KeyInfo=%d\n", u2KeyInfo);
    if ((u2KeyInfo & BIT(3)) && !(u2KeyInfo & BIT(13))) {
        return true;
    }

    return false;
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is TX entry point of NET DEVICE.
 *
 * \param[in] prSkb  Pointer of the sk_buff to be sent
 * \param[in] prDev  Pointer to struct net_device
 * \param[in] prGlueInfo  Pointer of prGlueInfo
 * \param[in] ucBssIndex  BSS index of this net device
 *
 * \retval WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
kalHardStartXmit(struct sk_buff *prOrgSkb, IN struct net_device *prDev,
                 P_GLUE_INFO_T prGlueInfo, u8 ucBssIndex)
{
    P_QUE_ENTRY_T prQueueEntry = NULL;
    P_QUE_T prTxQueue = NULL;
    u16 u2QueueIdx = 0;
    u32 u4MaxTxPendingNum = prGlueInfo->prAdapter->rWifiVar.u4NetifStopTh;
    struct sk_buff *prSkbNew = NULL;
    struct sk_buff *prSkb = NULL;

    ASSERT(prOrgSkb);
    ASSERT(prGlueInfo);

#if CFG_MESON_G12A_PATCH
    if (prOrgSkb->len > 1500) {
        DBGLOG(INIT, INFO, "Packet is too BIG! [%d]\n", prOrgSkb->len);
    }
#endif

    if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
        DBGLOG(INIT, INFO, "GLUE_FLAG_HALT skip tx\n");
        dev_kfree_skb(prOrgSkb);
        return WLAN_STATUS_ADAPTER_NOT_READY;
    }

    if (prGlueInfo->prAdapter->fgIsEnableLpdvt) {
        DBGLOG(INIT, INFO, "LPDVT enable, skip this frame\n");
        dev_kfree_skb(prOrgSkb);
        return WLAN_STATUS_NOT_ACCEPTED;
    }

    if (skb_headroom(prOrgSkb) < NIC_TX_HEAD_ROOM) {
        prSkbNew = skb_realloc_headroom(prOrgSkb, NIC_TX_HEAD_ROOM);
        ASSERT(prSkbNew);
        prSkb = prSkbNew;
        dev_kfree_skb(prOrgSkb);
    } else {
        prSkb = prOrgSkb;
    }

    prQueueEntry = (P_QUE_ENTRY_T)GLUE_GET_PKT_QUEUE_ENTRY(prSkb);
    prTxQueue = &prGlueInfo->rTxQueue;

    GLUE_SET_PKT_BSS_IDX(prSkb, ucBssIndex);

    /* Parsing frame info */
    if (!wlanProcessTxFrame(prGlueInfo->prAdapter,
                            (P_NATIVE_PACKET)prSkb)) {
        /* Cannot extract packet */
        DBGLOG(INIT, INFO, "Cannot extract content, skip this frame\n");
        dev_kfree_skb(prSkb);
        return WLAN_STATUS_INVALID_PACKET;
    }

    /* Tx profiling */
    wlanTxProfilingTagPacket(prGlueInfo->prAdapter, (P_NATIVE_PACKET)prSkb,
                             TX_PROF_TAG_OS_TO_DRV);

    /* Handle normal data frame */
    u2QueueIdx = skb_get_queue_mapping(prSkb);

    if (u2QueueIdx >= CFG_MAX_TXQ_NUM) {
        DBGLOG(INIT, INFO, "Incorrect queue index, skip this frame\n");
        dev_kfree_skb(prSkb);
        return WLAN_STATUS_INVALID_PACKET;
    }

    if (!HAL_IS_TX_DIRECT(prGlueInfo->prAdapter)) {
        GLUE_SPIN_LOCK_DECLARATION();

        GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
        QUEUE_INSERT_TAIL(prTxQueue, prQueueEntry);
        GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
    }

    GLUE_INC_REF_CNT(prGlueInfo->i4TxPendingFrameNum);
    GLUE_INC_REF_CNT(prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
                     [u2QueueIdx]);

    /*
     * WMM flow control
     * 1. To enlarge threshold for WMM certification, WMM phase two may hit
     * netif_stop_subquene Which may cause test case fail due to high
     * priority packets are not enough.
     * 2. Dynamic control threshold for AC queue.
     *    If there is high priority traffic, decrease low priority
     * threshold. If these is low priority traffic, increase high priority
     * threshold. Else, remians the original threshold.
     */
    if (prGlueInfo->prAdapter->rWifiVar.ucTpTestMode ==
        ENUM_TP_TEST_MODE_SIGMA_AC_N_PMF) {
        P_BSS_INFO_T prWmmBssInfo =
            prGlueInfo->prAdapter->aprBssInfo[ucBssIndex];

        if ((u2QueueIdx < 3) &&
            (GLUE_GET_REF_CNT(prGlueInfo->ai4TxPendingFrameNumPerQueue
                              [ucBssIndex][u2QueueIdx + 1]) >
             CFG_CERT_WMM_MAX_TX_PENDING)) {
            /*
             * Use au8Statistics[RX_SIZE_ERR_DROP_COUNT] to track RX
             * traffic in certification.
             */
            if ((prWmmBssInfo->eCurrentOPMode ==
                 OP_MODE_ACCESS_POINT) &&
                ((prDev->stats.rx_packets -
                  (prGlueInfo->prAdapter->rRxCtrl
                   .au8Statistics[RX_SIZE_ERR_DROP_COUNT])) >
                 CFG_CERT_WMM_MAX_RX_NUM)) {
                u4MaxTxPendingNum =
                    CFG_CERT_WMM_LOW_STOP_TX_WITH_RX;
            } else {
                u4MaxTxPendingNum =
                    CFG_CERT_WMM_LOW_STOP_TX_WO_RX;
            }
        } else if ((u2QueueIdx > 0) &&
                   (GLUE_GET_REF_CNT(
                        prGlueInfo->ai4TxPendingFrameNumPerQueue
                        [ucBssIndex][u2QueueIdx - 1]) >
                    CFG_CERT_WMM_MAX_TX_PENDING)) {
            /*
             * Use au8Statistics[RX_SIZE_ERR_DROP_COUNT] to track RX
             * traffic in certification.
             */
            if ((prWmmBssInfo->eCurrentOPMode ==
                 OP_MODE_ACCESS_POINT) &&
                ((prDev->stats.rx_packets -
                  (prGlueInfo->prAdapter->rRxCtrl
                   .au8Statistics[RX_SIZE_ERR_DROP_COUNT])) >
                 CFG_CERT_WMM_MAX_RX_NUM)) {
                u4MaxTxPendingNum =
                    CFG_CERT_WMM_HIGH_STOP_TX_WITH_RX;
            } else {
                u4MaxTxPendingNum =
                    CFG_CERT_WMM_HIGH_STOP_TX_WO_RX;
            }
        } else {
            u4MaxTxPendingNum =
                prGlueInfo->prAdapter->rWifiVar.u4NetifStopTh;
        }
    }

    if (GLUE_GET_REF_CNT(
            prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
            [u2QueueIdx]) >=
        u4MaxTxPendingNum) {
        netif_stop_subqueue(prDev, u2QueueIdx);

        DBGLOG(TX,
               TRACE,
               "Stop subqueue for BSS[%u] QIDX[%u] PKT_LEN[%u] TOT_CNT[%ld] PER-Q_CNT[%ld]\n",
               ucBssIndex,
               u2QueueIdx,
               prSkb->len,
               GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum),
               GLUE_GET_REF_CNT(prGlueInfo->ai4TxPendingFrameNumPerQueue
                                [ucBssIndex][u2QueueIdx]));

        /* Re-use au8Statistics[RX_SIZE_ERR_DROP_COUNT] buffer to track
         * RX traffic in certification */
        if (prGlueInfo->prAdapter->rWifiVar.ucTpTestMode ==
            ENUM_TP_TEST_MODE_SIGMA_AC_N_PMF) {
            prGlueInfo->prAdapter->rRxCtrl
            .au8Statistics[RX_SIZE_ERR_DROP_COUNT] =
                prDev->stats.rx_packets;
        }
    }

    /* Update NetDev statisitcs */
    prDev->stats.tx_bytes += prSkb->len;
    prDev->stats.tx_packets++;

    DBGLOG(TX,
           LOUD,
           "Enqueue frame for BSS[%u] QIDX[%u] PKT_LEN[%u] TOT_CNT[%ld] PER-Q_CNT[%ld]\n",
           ucBssIndex,
           u2QueueIdx,
           prSkb->len,
           GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum),
           GLUE_GET_REF_CNT(
               prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
               [u2QueueIdx]));

    if (HAL_IS_TX_DIRECT(prGlueInfo->prAdapter)) {
        return nicTxDirectStartXmit(prSkb, prGlueInfo);
    }

    kalSetEvent(prGlueInfo);

    return WLAN_STATUS_SUCCESS;
}

WLAN_STATUS kalResetStats(IN struct net_device *prDev)
{
    DBGLOG(QM, INFO, "Reset NetDev[0x%p] statistics\n", prDev);

    kalMemZero(kalGetStats(prDev), sizeof(struct net_device_stats));

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, to get the network interface
 * statistical information.
 *
 * Whenever an application needs to get statistics for the interface, this
 * method is called. This happens, for example, when ifconfig or netstat -i is
 * run.
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \return net_device_stats buffer pointer.
 */
/*----------------------------------------------------------------------------*/
void *kalGetStats(IN struct net_device *prDev)
{
    return (void *)&prDev->stats;
}

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
void kalSendCompleteAndAwakeQueue(IN P_GLUE_INFO_T prGlueInfo,
                                  IN void *pvPacket)
{
    struct net_device *prDev = NULL;
    struct sk_buff *prSkb = NULL;
    u16 u2QueueIdx = 0;
    u8 ucBssIndex = 0;
    u8 fgIsValidDevice = true;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(pvPacket);
    /* ASSERT(prGlueInfo->i4TxPendingFrameNum); */

    prSkb = (struct sk_buff *)pvPacket;
    u2QueueIdx = skb_get_queue_mapping(prSkb);
    ASSERT(u2QueueIdx < CFG_MAX_TXQ_NUM);

    ucBssIndex = GLUE_GET_PKT_BSS_IDX(pvPacket);

    GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingFrameNum);
    GLUE_DEC_REF_CNT(prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
                     [u2QueueIdx]);

    DBGLOG(TX,
           LOUD,
           "Release frame for BSS[%u] QIDX[%u] PKT_LEN[%u] TOT_CNT[%ld] PER-Q_CNT[%ld]\n",
           ucBssIndex,
           u2QueueIdx,
           prSkb->len,
           GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum),
           GLUE_GET_REF_CNT(
               prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
               [u2QueueIdx]));

    prDev = prSkb->dev;

    ASSERT(prDev);

#if CFG_ENABLE_WIFI_DIRECT
    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

    {
        P_BSS_INFO_T prBssInfo = GET_BSS_INFO_BY_INDEX(
            prGlueInfo->prAdapter, ucBssIndex);
        P_GL_P2P_INFO_T prGlueP2pInfo = (P_GL_P2P_INFO_T)NULL;
        struct net_device *prNetdevice = NULL;

        /* in case packet was sent after P2P device is unregistered or
         * the net_device was be free */
        if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
            if (prGlueInfo->prAdapter->fgIsP2PRegistered == false) {
                fgIsValidDevice = false;
            } else {
                ASSERT(prBssInfo->u4PrivateData < KAL_P2P_NUM);
                prGlueP2pInfo =
                    prGlueInfo->prP2PInfo
                    [prBssInfo->u4PrivateData];
                if (prGlueP2pInfo) {
                    prNetdevice =
                        prGlueP2pInfo->aprRoleHandler;
                    /* The net_device may be free */
                    if ((prDev != prNetdevice) &&
                        (prDev !=
                         prGlueP2pInfo->prDevHandler)) {
                        fgIsValidDevice = false;
                        DBGLOG(TX,
                               LOUD,
                               "kalSendCompleteAndAwakeQueue net device deleted! ucBssIndex = %u\n",
                               ucBssIndex);
                    }
                }
            }
        }
    }
#endif

    if (fgIsValidDevice == true) {
        u32 u4StartTh = prGlueInfo->prAdapter->rWifiVar.u4NetifStartTh;

        if (netif_subqueue_stopped(prDev, prSkb) &&
            prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
            [u2QueueIdx] <=
            u4StartTh) {
            netif_wake_subqueue(prDev, u2QueueIdx);
            DBGLOG(TX,
                   TRACE,
                   "WakeUp Queue BSS[%u] QIDX[%u] PKT_LEN[%u] TOT_CNT[%ld] PER-Q_CNT[%ld]\n",
                   ucBssIndex,
                   u2QueueIdx,
                   prSkb->len,
                   GLUE_GET_REF_CNT(
                       prGlueInfo->i4TxPendingFrameNum),
                   GLUE_GET_REF_CNT(
                       prGlueInfo->ai4TxPendingFrameNumPerQueue
                       [ucBssIndex][u2QueueIdx]));
        }
    }

#if CFG_ENABLE_WIFI_DIRECT
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
#endif

    dev_kfree_skb_any((struct sk_buff *)pvPacket);

    DBGLOG(TX, LOUD, "----- pending frame %d -----\n",
           prGlueInfo->i4TxPendingFrameNum);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Copy Mac Address setting from registry. It's All Zeros in Linux.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 *
 * \param[out] paucMacAddr Pointer to the Mac Address buffer
 *
 * \retval WLAN_STATUS_SUCCESS
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
void kalQueryRegistryMacAddr(IN P_GLUE_INFO_T prGlueInfo,
                             OUT u8 *paucMacAddr)
{
    u8 aucZeroMac[MAC_ADDR_LEN] = { 0, 0, 0, 0, 0, 0 }

    DEBUGFUNC("kalQueryRegistryMacAddr");

    ASSERT(prGlueInfo);
    ASSERT(paucMacAddr);

    kalMemCopy((void *)paucMacAddr, (void *)aucZeroMac, MAC_ADDR_LEN);
}

#if CFG_SUPPORT_EXT_CONFIG
/*----------------------------------------------------------------------------*/
/*!
 * \brief Read external configuration, ex. NVRAM or file
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
u32 kalReadExtCfg(IN P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);

    /* External data is given from user space by ioctl or /proc, not read by
     *  driver.
     */
    if (prGlueInfo->u4ExtCfgLength != 0) {
        DBGLOG(INIT, TRACE, "Read external configuration data -- OK\n");
    } else {
        DBGLOG(INIT, TRACE,
               "Read external configuration data -- fail\n");
    }

    return prGlueInfo->u4ExtCfgLength;
}
#endif

u8 kalIPv4FrameClassifier(IN P_GLUE_INFO_T prGlueInfo,
                          IN P_NATIVE_PACKET prPacket, IN u8 *pucIpHdr,
                          OUT P_TX_PACKET_INFO prTxPktInfo)
{
    u8 ucIpVersion;
    /* u16 u2IpId; */

    /* IPv4 version check */
    ucIpVersion = (pucIpHdr[0] & IP_VERSION_MASK) >> IP_VERSION_OFFSET;
    if (ucIpVersion != IP_VERSION_4) {
        DBGLOG(INIT, WARN, "Invalid IPv4 packet version: %u\n",
               ucIpVersion);
        return false;
    }
    /* WLAN_GET_FIELD_16(&pucIpHdr[IPV4_HDR_IP_IDENTIFICATION_OFFSET],
     * &u2IpId); */

    if (pucIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET] == IP_PROTOCOL_UDP) {
        struct sk_buff *prSkb = (struct sk_buff *)prPacket;
        u32 u4PacketLen = prSkb->len;
        u8 ucIHL = (pucIpHdr[0] & 0xF) * 4;  /* IHL unit: DW = 4bytes */
        u8 *pucUdpHdr = &pucIpHdr[ucIHL];
        u16 u2DstPort;
        /* u16 u2SrcPort; */

        /* DBGLOG_MEM8(INIT, INFO, pucUdpHdr, 256); */
        DBGLOG(INIT, INFO, "IP header parsed, ucIHL: %u\n", ucIHL);

        /* UDP packet should have 8 byte header at least */
        if ((ETHER_HEADER_LEN + ucIHL + UDP_HDR_LEN) > u4PacketLen) {
            DBGLOG(INIT,
                   WARN,
                   "Invalid UDP packet, should include 8 bytes header at least, packet length too small: %u < %u\n",
                   u4PacketLen,
                   ETHER_HEADER_LEN + ucIHL + UDP_HDR_LEN);
            return false;
        }

        /* Get UDP DST port */
        WLAN_GET_FIELD_BE16(&pucUdpHdr[UDP_HDR_DST_PORT_OFFSET],
                            &u2DstPort);

        /* DBGLOG(INIT, INFO, ("UDP DST[%u]\n", u2DstPort)); */

        /* Get UDP SRC port */
        /* WLAN_GET_FIELD_BE16(&pucUdpHdr[UDP_HDR_SRC_PORT_OFFSET],
         * &u2SrcPort); */

        /* BOOTP/DHCP protocol */
        if ((u2DstPort == IP_PORT_BOOTP_SERVER) ||
            (u2DstPort == IP_PORT_BOOTP_CLIENT)) {
            P_BOOTP_PROTOCOL_T prBootp =
                (P_BOOTP_PROTOCOL_T)&pucUdpHdr[UDP_HDR_LEN];

            u32 u4DhcpMagicCode;

            if ((ETHER_HEADER_LEN + ucIHL + UDP_HDR_LEN +
                 sizeof(BOOTP_PROTOCOL_T) + sizeof(u32)) >
                u4PacketLen) {
                DBGLOG(INIT,
                       WARN,
                       "Invalid bootp packet w/ DHCP magic code, packet length too small: %u < %u\n",
                       u4PacketLen,
                       ETHER_HEADER_LEN + ucIHL + UDP_HDR_LEN +
                       sizeof(BOOTP_PROTOCOL_T) +
                       sizeof(u32));
                return false;
            }

            WLAN_GET_FIELD_BE32(&prBootp->aucOptions[0],
                                &u4DhcpMagicCode);

            if (u4DhcpMagicCode == DHCP_MAGIC_NUMBER) {
                u32 u4Xid;

                WLAN_GET_FIELD_BE32(&prBootp->u4TransId,
                                    &u4Xid);

                DBGLOG(SW4,
                       INFO,
                       "DHCP PKT[0x%p] XID[0x%08x] OPT[%u] TYPE[%u]\n",
                       prPacket,
                       u4Xid,
                       prBootp->aucOptions[4],
                       prBootp->aucOptions[6]);

                prTxPktInfo->u2Flag |= BIT(ENUM_PKT_DHCP);
            }
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This inline function is to extract some packet information, including
 *        user priority, packet length, destination address, 802.1x and BT over
 * Wi-Fi or not.
 *
 * @param prGlueInfo         Pointer to the glue structure
 * @param prPacket           Packet descriptor
 * @param prTxPktInfo        Extracted packet info
 *
 * @retval true      Success to extract information
 * @retval false     Fail to extract correct information
 */
/*----------------------------------------------------------------------------*/
u8 kalQoSFrameClassifierAndPacketInfo(IN P_GLUE_INFO_T prGlueInfo,
                                      IN P_NATIVE_PACKET prPacket,
                                      OUT P_TX_PACKET_INFO prTxPktInfo)
{
    u32 u4PacketLen;
    u16 u2EtherTypeLen;
    struct sk_buff *prSkb = (struct sk_buff *)prPacket;
    u8 *aucLookAheadBuf = NULL;
    u8 ucEthTypeLenOffset = ETHER_HEADER_LEN - ETHER_TYPE_LEN;
    u8 *pucNextProtocol = NULL;
    u16 u2KeyInfo = 0;
    u8 ucEAPoLKey = 0;
    u8 ucEapOffset = ETHER_HEADER_LEN;

    u4PacketLen = prSkb->len;

    if (u4PacketLen < ETHER_HEADER_LEN) {
        DBGLOG(INIT, WARN, "Invalid Ether packet length: %lu\n",
               u4PacketLen);
        return false;
    }

    aucLookAheadBuf = prSkb->data;

    /* Reset Packet Info */
    kalMemZero(prTxPktInfo, sizeof(TX_PACKET_INFO));

    /* 4 <0> Obtain Ether Type/Len */
    WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ucEthTypeLenOffset],
                        &u2EtherTypeLen);

    /* 4 <1> Skip 802.1Q header (VLAN Tagging) */
    if (u2EtherTypeLen == ETH_P_VLAN) {
        prTxPktInfo->u2Flag |= BIT(ENUM_PKT_VLAN_EXIST);
        ucEthTypeLenOffset += ETH_802_1Q_HEADER_LEN;
        if (u4PacketLen < ucEthTypeLenOffset + sizeof(u16)) {
            DBGLOG(INIT, WARN,
                   "Invalid Ether packet length: %lu < %lu\n",
                   u4PacketLen, ucEthTypeLenOffset + sizeof(u16));
            return false;
        }
        WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ucEthTypeLenOffset],
                            &u2EtherTypeLen);
    }
    /* 4 <2> Obtain next protocol pointer */
    pucNextProtocol = &aucLookAheadBuf[ucEthTypeLenOffset + ETHER_TYPE_LEN];

    /* 4 <3> Handle ethernet format */
    switch (u2EtherTypeLen) {
    /* IPv4 */
    case ETH_P_IPV4:
        /* IPv4 header length check */
        if (u4PacketLen <
            (ucEthTypeLenOffset + ETHER_TYPE_LEN + IPV4_HDR_LEN)) {
            DBGLOG(INIT, WARN, "Invalid IPv4 packet length: %lu\n",
                   u4PacketLen);
            /* Supplicant trying to TX keep alive NULL frame.
             * This behavior would cause some AP issue.
             */
            return false;
        }

        kalIPv4FrameClassifier(prGlueInfo, prPacket, pucNextProtocol,
                               prTxPktInfo);
        break;

    case ETH_P_ARP: {
        u16 u2ArpOp;

        WLAN_GET_FIELD_BE16(&pucNextProtocol[ARP_OPERATION_OFFSET],
                            &u2ArpOp);

        DBGLOG(SW4, INFO,
               "ARP %s PKT[0x%p] TAR MAC/IP[" MACSTR "]/[" IPV4STR
               "]\n",
               u2ArpOp == ARP_OPERATION_REQUEST ? "REQ" : "RSP",
               prPacket,
               MAC2STR(&pucNextProtocol[ARP_TARGET_MAC_OFFSET]),
               IPV4TOSTR(&pucNextProtocol[ARP_TARGET_IP_OFFSET]));

        prTxPktInfo->u2Flag |= BIT(ENUM_PKT_ARP);
    } break;

    case ETH_P_1X:
    case ETH_P_PRE_1X:
        prTxPktInfo->u2Flag |= BIT(ENUM_PKT_1X);
        DBGLOG(RSN, INFO, "T1x like normal data, PKT[0x%p]\n",
               prPacket);

        if (u2EtherTypeLen == ETH_P_1X) {
            /* Leave EAP to check */
            ucEAPoLKey = aucLookAheadBuf[1 + ucEapOffset];
            if (ucEAPoLKey != ETH_EAPOL_KEY) {
                prTxPktInfo->u2Flag |=
                    BIT(ENUM_PKT_NON_PROTECTED_1X);
            } else {
                WLAN_GET_FIELD_BE16(
                    &aucLookAheadBuf[5 + ucEapOffset],
                    &u2KeyInfo);
                /* BIT3 is pairwise key bit */
                DBGLOG(RSN,
                       WARN,
                       "Tx EAPOL Frame [Len: %d] [KeyInfo: %d]\n",
                       u4PacketLen,
                       u2KeyInfo);
                if (u2KeyInfo & BIT(3)) {
                    prTxPktInfo->u2Flag |=
                        BIT(ENUM_PKT_NON_PROTECTED_1X);
                }
            }
        }
        break;

    default:
        /* 4 <4> Handle 802.3 format if LEN <= 1500 */
        if (u2EtherTypeLen <= ETH_802_3_MAX_LEN) {
            prTxPktInfo->u2Flag |= BIT(ENUM_PKT_802_3);
        }
        break;
    }

    /* 4 <4.1> Check for PAL (BT over Wi-Fi) */
    /* Move to kalBowFrameClassifier */

    /* 4 <5> Return the value of Priority Parameter. */
    /* prSkb->priority is assigned by Linux wireless utility
     * function(cfg80211_classify8021d) */
    /* at net_dev selection callback (ndo_select_queue) */
    prTxPktInfo->ucPriorityParam = prSkb->priority;

    /* 4 <6> Retrieve Packet Information - DA */
    /* Packet Length/ Destination Address */
    prTxPktInfo->u4PacketLen = u4PacketLen;

    kalMemCopy(prTxPktInfo->aucEthDestAddr, aucLookAheadBuf,
               PARAM_MAC_ADDR_LEN);

    return true;
}

u8 kalGetEthDestAddr(IN P_GLUE_INFO_T prGlueInfo,
                     IN P_NATIVE_PACKET prPacket,
                     OUT u8 *pucEthDestAddr)
{
    struct sk_buff *prSkb = (struct sk_buff *)prPacket;
    u8 *aucLookAheadBuf = NULL;

    /* Sanity Check */
    if (!prPacket || !prGlueInfo) {
        return false;
    }

    aucLookAheadBuf = prSkb->data;

    kalMemCopy(pucEthDestAddr, aucLookAheadBuf, PARAM_MAC_ADDR_LEN);

    return true;
}

void kalOidComplete(IN P_GLUE_INFO_T prGlueInfo, IN u8 fgSetQuery,
                    IN u32 u4SetQueryInfoLen, IN WLAN_STATUS rOidStatus)
{
    P_GL_IO_REQ_T prIoReq = NULL;

    ASSERT(prGlueInfo);
    /* remove timeout check timer */
    wlanoidClearTimeoutCheck(prGlueInfo->prAdapter);

    prGlueInfo->rPendStatus = rOidStatus;

    prIoReq = &(prGlueInfo->OidEntry);

    /* Not submit req yet, or submit fail, no need to complete */
    if (prIoReq->rStatus == WLAN_STATUS_FAILURE) {
        DBGLOG(INIT, WARN, "Ignore invalid OID complete\n");
        return;
    }

    prGlueInfo->u4OidCompleteFlag = 1;
    /* complete ONLY if there are waiters */
    if (!completion_done(&prGlueInfo->rPendComp)) {
        complete(&prGlueInfo->rPendComp);
    } else {
        DBGLOG(INIT, WARN, "SKIP multiple OID complete!\n");
        /* WARN_ON(true); */
    }

    if (rOidStatus == WLAN_STATUS_SUCCESS) {
        DBGLOG(INIT, TRACE, "Complete OID, status:success\n");
    }else{
        DBGLOG(INIT, WARN, "Complete OID, status:0x%08x\n", rOidStatus);
    }

    /* else let it timeout on kalIoctl entry */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to transfer linux ioctl to OID, and  we
 * need to specify the behavior of the OID by ourself
 *
 * @param prGlueInfo         Pointer to the glue structure
 * @param pvInfoBuf          Data buffer
 * @param u4InfoBufLen       Data buffer length
 * @param fgRead             Is this a read OID
 * @param fgWaitResp         does this OID need to wait for values
 * @param fgCmd              does this OID compose command packet
 * @param pu4QryInfoLen      The data length of the return values
 *
 * @retval true      Success to extract information
 * @retval false     Fail to extract correct information
 */
/*----------------------------------------------------------------------------*/

/* todo: enqueue the i/o requests for multiple processes access */
/*  */
/* currently, return -1 */
/*  */

/* static GL_IO_REQ_T OidEntry; */

WLAN_STATUS
kalIoctl(IN P_GLUE_INFO_T prGlueInfo, IN PFN_OID_HANDLER_FUNC pfnOidHandler,
         IN void *pvInfoBuf, IN u32 u4InfoBufLen, IN u8 fgRead,
         IN u8 fgWaitResp, IN u8 fgCmd, OUT u32 *pu4QryInfoLen)
{
    return kalIoctlTimeout(prGlueInfo, pfnOidHandler, pvInfoBuf,
                           u4InfoBufLen, fgRead, fgWaitResp, fgCmd, -1,
                           pu4QryInfoLen);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to transfer linux ioctl to OID, and  we
 * need to specify the behavior of the OID by ourself
 *
 * @param prGlueInfo         Pointer to the glue structure
 * @param pvInfoBuf          Data buffer
 * @param u4InfoBufLen       Data buffer length
 * @param fgRead             Is this a read OID
 * @param fgWaitResp         does this OID need to wait for values
 * @param fgCmd              does this OID compose command packet
 * @param i4OidTimeout       timeout for this OID
 * @param pu4QryInfoLen      The data length of the return values
 *
 * @retval true      Success to extract information
 * @retval false     Fail to extract correct information
 */
/*----------------------------------------------------------------------------*/

WLAN_STATUS
kalIoctlTimeout(IN P_GLUE_INFO_T prGlueInfo,
                IN PFN_OID_HANDLER_FUNC pfnOidHandler, IN void *pvInfoBuf,
                IN u32 u4InfoBufLen, IN u8 fgRead, IN u8 fgWaitResp,
                IN u8 fgCmd, IN s32 i4OidTimeout, OUT u32 *pu4QryInfoLen)
{
    P_GL_IO_REQ_T prIoReq = NULL;
    WLAN_STATUS ret = WLAN_STATUS_SUCCESS;
#if CFG_IOCTL_WAIT_FOR_COMPLETION_TIMEOUT
    unsigned long ret2 = 0;
    u32 completion_timeout = 0;
#endif

    if (wlanIsChipAssert(prGlueInfo->prAdapter)) {
        return WLAN_STATUS_SUCCESS;
    }

    /* GLUE_SPIN_LOCK_DECLARATION(); */
    ASSERT(prGlueInfo);

    /* <1> Check if driver is halt */
    /* if (prGlueInfo->u4Flag & GLUE_FLAG_HALT) { */
    /* return WLAN_STATUS_ADAPTER_NOT_READY; */
    /* } */


    if (down_interruptible(&g_halt_sem)) {
	    DBGLOG(INIT, INFO, "Halt semaphore is down.\n");
        return WLAN_STATUS_FAILURE;
    }

    if (g_u4HaltFlag) {
        up(&g_halt_sem);
	DBGLOG(INIT, INFO, "Halt flag is true.\n");
        return WLAN_STATUS_ADAPTER_NOT_READY;
    }

    if (down_interruptible(&prGlueInfo->ioctl_sem)) {
        up(&g_halt_sem);
	DBGLOG(INIT, INFO, "Ioctl semaphore is down.\n");
        return WLAN_STATUS_FAILURE;
    }

    /* <2> TODO: thread-safe */

    /* <3> point to the OidEntry of Glue layer */

    prIoReq = &(prGlueInfo->OidEntry);

    ASSERT(prIoReq);

    /* <4> Compose the I/O request */
    prIoReq->prAdapter = prGlueInfo->prAdapter;
    prIoReq->pfnOidHandler = pfnOidHandler;
    prIoReq->pvInfoBuf = pvInfoBuf;
    prIoReq->u4InfoBufLen = u4InfoBufLen;
    prIoReq->pu4QryInfoLen = pu4QryInfoLen;
    prIoReq->fgRead = fgRead;
    prIoReq->fgWaitResp = fgWaitResp;
    prIoReq->rStatus = WLAN_STATUS_FAILURE;

    if (i4OidTimeout >= 0 &&
        i4OidTimeout <= WLAN_OID_TIMEOUT_THRESHOLD_MAX) {
        prIoReq->u4Timeout = (u32)i4OidTimeout;
    }else{
        prIoReq->u4Timeout = WLAN_OID_TIMEOUT_THRESHOLD;
    }

    /* <5> Reset the status of pending OID */
    prGlueInfo->rPendStatus = WLAN_STATUS_FAILURE;
    /* prGlueInfo->u4TimeoutFlag = 0; */
    prGlueInfo->u4OidCompleteFlag = 0;

    /* <6> Check if we use the command queue */
    prIoReq->u4Flag = fgCmd;

    /* <7> schedule the OID bit
     * Use memory barrier to ensure OidEntry is written done and then set
     * bit.
     */
    smp_mb();
    set_bit(GLUE_FLAG_OID_BIT, &prGlueInfo->ulFlag);

    /* <8> Wake up main thread to handle kick start the I/O request.
     * Use memory barrier to ensure set bit is done and then wake up main
     * thread.
     */
    smp_mb();
    wake_up_interruptible(&prGlueInfo->waitq);

    /* <9> Block and wait for event or timeout, current the timeout is 2
     * secs */
#if CFG_IOCTL_WAIT_FOR_COMPLETION_TIMEOUT
    /* set completion timeout to be oid timeout + 1 sec for thread switching
     * and main_thread processing delay so that this
     * wait_for_completion_timeout() will not overkill the case where
     * main_thread actually is just busy working
     */
    completion_timeout = prIoReq->u4Timeout + 1000;

    DBGLOG(INIT, WARN, "CHECK 1\n");

    ret2 = wait_for_completion_timeout(&prGlueInfo->rPendComp,
                                       MSEC_TO_JIFFIES(completion_timeout));

    DBGLOG(INIT, WARN, "CHECK 2\n");

    if (ret2 == 0) {
        DBGLOG(INIT, ERROR, "exceed %d ms, force timeout\n",
               completion_timeout);
        ret = WLAN_STATUS_FAILURE;
    } else
#else
    wait_for_completion(&prGlueInfo->rPendComp);
#endif
    {
        /* Case 1: No timeout. */
        /* if return WLAN_STATUS_PENDING, the status of cmd is stored in
         * prGlueInfo  */
        if (prIoReq->rStatus == WLAN_STATUS_PENDING) {
            ret = prGlueInfo->rPendStatus;
        }else{
            ret = prIoReq->rStatus;
        }
    }

    /* <10> Clear bit for error handling */
    clear_bit(GLUE_FLAG_OID_BIT, &prGlueInfo->ulFlag);

    up(&prGlueInfo->ioctl_sem);
    up(&g_halt_sem);

    return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all pending security frames
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearSecurityFrames(IN P_GLUE_INFO_T prGlueInfo)
{
    P_QUE_T prCmdQue;
    QUE_T rTempCmdQue;
    P_QUE_T prTempCmdQue = &rTempCmdQue;
    QUE_T rReturnCmdQue;
    P_QUE_T prReturnCmdQue = &rReturnCmdQue;
    P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T)NULL;

    P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T)NULL;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);

    QUEUE_INITIALIZE(prReturnCmdQue);
    /* Clear pending security frames in prGlueInfo->rCmdQueue */
    prCmdQue = &prGlueInfo->rCmdQueue;

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

    QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    while (prQueueEntry) {
        prCmdInfo = (P_CMD_INFO_T)prQueueEntry;

        if (prCmdInfo->eCmdType == COMMAND_TYPE_SECURITY_FRAME) {
            if (prCmdInfo->pfCmdTimeoutHandler) {
                prCmdInfo->pfCmdTimeoutHandler(
                    prGlueInfo->prAdapter, prCmdInfo);
            } else {
                wlanReleaseCommand(prGlueInfo->prAdapter,
                                   prCmdInfo,
                                   TX_RESULT_QUEUE_CLEARANCE);
            }
            cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
            GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
        } else {
            QUEUE_INSERT_TAIL(prReturnCmdQue, prQueueEntry);
        }

        QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    }

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prReturnCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear pending security frames
 *        belongs to dedicated network type
 *
 * \param prGlueInfo         Pointer of GLUE Data Structure
 * \param eNetworkTypeIdx    Network Type Index
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearSecurityFramesByBssIdx(IN P_GLUE_INFO_T prGlueInfo,
                                    IN u8 ucBssIndex)
{
    P_QUE_T prCmdQue;
    QUE_T rTempCmdQue;
    P_QUE_T prTempCmdQue = &rTempCmdQue;
    QUE_T rReturnCmdQue;
    P_QUE_T prReturnCmdQue = &rReturnCmdQue;
    P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T)NULL;
    P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T)NULL;
    P_MSDU_INFO_T prMsduInfo;
    u8 fgFree;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);

    QUEUE_INITIALIZE(prReturnCmdQue);
    /* Clear pending security frames in prGlueInfo->rCmdQueue */
    prCmdQue = &prGlueInfo->rCmdQueue;

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

    QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    while (prQueueEntry) {
        prCmdInfo = (P_CMD_INFO_T)prQueueEntry;
        prMsduInfo = prCmdInfo->prMsduInfo;
        fgFree = false;

        if (prCmdInfo->eCmdType == COMMAND_TYPE_SECURITY_FRAME &&
            prMsduInfo) {
            if (prMsduInfo->ucBssIndex == ucBssIndex) {
                fgFree = true;
            }
        }

        if (fgFree) {
            if (prCmdInfo->pfCmdTimeoutHandler) {
                prCmdInfo->pfCmdTimeoutHandler(
                    prGlueInfo->prAdapter, prCmdInfo);
            } else {
                wlanReleaseCommand(prGlueInfo->prAdapter,
                                   prCmdInfo,
                                   TX_RESULT_QUEUE_CLEARANCE);
            }
            cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
            GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
        } else {
            QUEUE_INSERT_TAIL(prReturnCmdQue, prQueueEntry);
        }

        QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    }

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prReturnCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all pending management frames
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearMgmtFrames(IN P_GLUE_INFO_T prGlueInfo)
{
    P_QUE_T prCmdQue;
    QUE_T rTempCmdQue;
    P_QUE_T prTempCmdQue = &rTempCmdQue;
    QUE_T rReturnCmdQue;
    P_QUE_T prReturnCmdQue = &rReturnCmdQue;
    P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T)NULL;
    P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T)NULL;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);

    QUEUE_INITIALIZE(prReturnCmdQue);
    /* Clear pending management frames in prGlueInfo->rCmdQueue */
    prCmdQue = &prGlueInfo->rCmdQueue;

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

    QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    while (prQueueEntry) {
        prCmdInfo = (P_CMD_INFO_T)prQueueEntry;

        if (prCmdInfo->eCmdType == COMMAND_TYPE_MANAGEMENT_FRAME) {
            wlanReleaseCommand(prGlueInfo->prAdapter, prCmdInfo,
                               TX_RESULT_QUEUE_CLEARANCE);
            cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
            GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
        } else {
            QUEUE_INSERT_TAIL(prReturnCmdQue, prQueueEntry);
        }

        QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    }

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prReturnCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all pending management frames
 *           belongs to dedicated network type
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearMgmtFramesByBssIdx(IN P_GLUE_INFO_T prGlueInfo,
                                IN u8 ucBssIndex)
{
    P_QUE_T prCmdQue;
    QUE_T rTempCmdQue;
    P_QUE_T prTempCmdQue = &rTempCmdQue;
    QUE_T rReturnCmdQue;
    P_QUE_T prReturnCmdQue = &rReturnCmdQue;
    P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T)NULL;
    P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T)NULL;
    P_MSDU_INFO_T prMsduInfo;
    u8 fgFree;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);

    QUEUE_INITIALIZE(prReturnCmdQue);
    /* Clear pending management frames in prGlueInfo->rCmdQueue */
    prCmdQue = &prGlueInfo->rCmdQueue;

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

    QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    while (prQueueEntry) {
        prCmdInfo = (P_CMD_INFO_T)prQueueEntry;
        prMsduInfo = prCmdInfo->prMsduInfo;
        fgFree = false;

        if (prCmdInfo->eCmdType == COMMAND_TYPE_MANAGEMENT_FRAME &&
            prMsduInfo) {
            if (prMsduInfo->ucBssIndex == ucBssIndex) {
                fgFree = true;
            }
        }

        if (fgFree) {
            wlanReleaseCommand(prGlueInfo->prAdapter, prCmdInfo,
                               TX_RESULT_QUEUE_CLEARANCE);
            cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
            GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
        } else {
            QUEUE_INSERT_TAIL(prReturnCmdQue, prQueueEntry);
        }

        QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    }

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prReturnCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all commands in command queue
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearCommandQueue(IN P_GLUE_INFO_T prGlueInfo,
                          IN u8 fgIsNeedHandler)
{
    P_QUE_T prCmdQue;
    QUE_T rTempCmdQue;
    P_QUE_T prTempCmdQue = &rTempCmdQue;
    QUE_T rReturnCmdQue;
    P_QUE_T prReturnCmdQue = &rReturnCmdQue;
    P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T)NULL;
    P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T)NULL;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);

    QUEUE_INITIALIZE(prReturnCmdQue);

    /* Clear ALL in prGlueInfo->rCmdQueue */
    prCmdQue = &prGlueInfo->rCmdQueue;

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

    QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    while (prQueueEntry) {
        prCmdInfo = (P_CMD_INFO_T)prQueueEntry;

        if (prCmdInfo->pfCmdTimeoutHandler) {
            prCmdInfo->pfCmdTimeoutHandler(prGlueInfo->prAdapter,
                                           prCmdInfo);
        } else {
            wlanReleaseCommandEx(prGlueInfo->prAdapter, prCmdInfo,
                                 TX_RESULT_QUEUE_CLEARANCE,
                                 fgIsNeedHandler);
        }

        cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
        GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);

        QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    }
}

u32 kalProcessTxPacket(P_GLUE_INFO_T prGlueInfo, struct sk_buff *prSkb)
{
    u32 u4Status = WLAN_STATUS_SUCCESS;

    if (prSkb == NULL) {
        DBGLOG(INIT, WARN, "prSkb == NULL in tx\n");
        return u4Status;
    }

    /* Handle security frame */
    if (0  /* GLUE_TEST_PKT_FLAG(prSkb, ENUM_PKT_1X) */
           /* No more sending via cmd */) {
        if (wlanProcessSecurityFrame(prGlueInfo->prAdapter,
                                     (P_NATIVE_PACKET)prSkb)) {
            u4Status = WLAN_STATUS_SUCCESS;
            GLUE_INC_REF_CNT(
                prGlueInfo->i4TxPendingSecurityFrameNum);
        } else {
            u4Status = WLAN_STATUS_RESOURCES;
        }
    }
    /* Handle normal frame */
    else {
        u4Status = wlanEnqueueTxPacket(prGlueInfo->prAdapter,
                                       (P_NATIVE_PACKET)prSkb);
    }

    return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to process Tx request to main_thread
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalProcessTxReq(P_GLUE_INFO_T prGlueInfo, u8 *pfgNeedHwAccess)
{
    P_QUE_T prCmdQue = NULL;
    P_QUE_T prTxQueue = NULL;
    QUE_T rTempQue;
    P_QUE_T prTempQue = &rTempQue;
    QUE_T rTempReturnQue;
    P_QUE_T prTempReturnQue = &rTempReturnQue;
    P_QUE_ENTRY_T prQueueEntry = NULL;
    u32 u4Status;
    u32 u4CmdCount = 0;
    u32 u4TxLoopCount;

    /* for spin lock acquire and release */
    GLUE_SPIN_LOCK_DECLARATION();

    prTxQueue = &prGlueInfo->rTxQueue;
    prCmdQue = &prGlueInfo->rCmdQueue;

    QUEUE_INITIALIZE(prTempQue);
    QUEUE_INITIALIZE(prTempReturnQue);

    u4TxLoopCount = prGlueInfo->prAdapter->rWifiVar.u4TxFromOsLoopCount;

    /* Process Mailbox Messages */
    wlanProcessMboxMessage(prGlueInfo->prAdapter);

    /* Process CMD request */
    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    u4CmdCount = prCmdQue->u4NumElem;
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    if (u4CmdCount > 0) {
        wlanProcessCommandQueue(prGlueInfo->prAdapter, prCmdQue);
    }

    while (u4TxLoopCount--) {
        while (QUEUE_IS_NOT_EMPTY(prTxQueue)) {
            GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
            QUEUE_MOVE_ALL(prTempQue, prTxQueue);
            GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);

            /* Handle Packet Tx */
            while (QUEUE_IS_NOT_EMPTY(prTempQue)) {
                QUEUE_REMOVE_HEAD(prTempQue, prQueueEntry,
                                  P_QUE_ENTRY_T);

                if (prQueueEntry == NULL) {
                    break;
                }

                u4Status = kalProcessTxPacket(
                    prGlueInfo,
                    (struct sk_buff *)
                    GLUE_GET_PKT_DESCRIPTOR(
                        prQueueEntry));

                /* Enqueue packet back into TxQueue if resource
                 * is not enough */
                if (u4Status == WLAN_STATUS_RESOURCES) {
                    QUEUE_INSERT_TAIL(prTempReturnQue,
                                      prQueueEntry);
                    break;
                }
                if (u4Status == WLAN_STATUS_PENDING) {
                    QUEUE_INSERT_HEAD(prTempReturnQue,
                                      prQueueEntry);
                    break;
                }
            }

            if (wlanGetTxPendingFrameCount(prGlueInfo->prAdapter) >
                0) {
                wlanTxPendingPackets(prGlueInfo->prAdapter,
                                     pfgNeedHwAccess);
            }

            /* Enqueue packet back into TxQueue if resource is not
             * enough */
            if (QUEUE_IS_NOT_EMPTY(prTempReturnQue)) {
                QUEUE_CONCATENATE_QUEUES(prTempReturnQue,
                                         prTempQue);

                GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo,
                                       SPIN_LOCK_TX_QUE);
                QUEUE_CONCATENATE_QUEUES_HEAD(prTxQueue,
                                              prTempReturnQue);
                GLUE_RELEASE_SPIN_LOCK(prGlueInfo,
                                       SPIN_LOCK_TX_QUE);

                break;
            }
        }

        if (wlanGetTxPendingFrameCount(prGlueInfo->prAdapter) > 0) {
            wlanTxPendingPackets(prGlueInfo->prAdapter,
                                 pfgNeedHwAccess);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param data       data pointer to private data of hif_thread
 *
 * @retval           If the function succeeds, the return value is 0.
 * Otherwise, an error code is returned.
 *
 */
/*----------------------------------------------------------------------------*/

int hif_thread(void *data)
{
    struct net_device *dev = data;
    P_GLUE_INFO_T prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(dev));
    int ret = 0;

    DBGLOG(INIT, INFO, "%s:%u starts running...\n",
           KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

    prGlueInfo->u4HifThreadPid = KAL_GET_CURRENT_THREAD_ID();

    set_user_nice(current, prGlueInfo->prAdapter->rWifiVar.cThreadNice);

    while (true) {
        if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
            DBGLOG(INIT, INFO, "hif_thread should stop now...\n");
            break;
        }

        /*
         * sleep on waitqueue if no events occurred. Event contain (1)
         * GLUE_FLAG_INT (2) GLUE_FLAG_OID (3) GLUE_FLAG_TXREQ (4)
         * GLUE_FLAG_HALT
         *
         */
        do {
            ret = wait_event_interruptible(
                prGlueInfo->waitq_hif,
                ((prGlueInfo->ulFlag & GLUE_FLAG_HIF_PROCESS) !=
                 0));
        } while (ret != 0);

        wlanAcquirePowerControl(prGlueInfo->prAdapter);

        /* Handle Interrupt */
        if (test_and_clear_bit(GLUE_FLAG_INT_BIT,
                               &prGlueInfo->ulFlag)) {
            /* the Wi-Fi interrupt is already disabled in mmc
             * thread, so we set the flag only to enable the
             * interrupt later
             */
            prGlueInfo->prAdapter->fgIsIntEnable = false;
            if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
                /* Should stop now... skip pending interrupt */
                DBGLOG(INIT, INFO,
                       "ignore pending interrupt\n");
            } else {
                /* DBGLOG(INIT, INFO, ("HIF Interrupt!\n")); */
                wlanIST(prGlueInfo->prAdapter);
            }
        }

        /* Skip Tx request if SER is operating */
        if (!nicSerIsTxStop(prGlueInfo->prAdapter)) {
            /* TX Commands */
            if (test_and_clear_bit(GLUE_FLAG_HIF_TX_CMD_BIT,
                                   &prGlueInfo->ulFlag)) {
                wlanTxCmdMthread(prGlueInfo->prAdapter);
            }

            /* Process TX data packet to HIF */
            if (test_and_clear_bit(GLUE_FLAG_HIF_TX_BIT,
                                   &prGlueInfo->ulFlag)) {
                nicTxMsduQueueMthread(prGlueInfo->prAdapter);
            }
        }

        /* Read chip status when chip no response */
        if (test_and_clear_bit(GLUE_FLAG_HIF_PRT_HIF_DBG_INFO_BIT,
                               &prGlueInfo->ulFlag)) {
            halPrintHifDbgInfo(prGlueInfo->prAdapter);
        }

        /* Set FW own */
        if (test_and_clear_bit(GLUE_FLAG_HIF_FW_OWN_BIT,
                               &prGlueInfo->ulFlag)) {
            prGlueInfo->prAdapter->fgWiFiInSleepyState = true;
        }

        /* Release to FW own */
        wlanReleasePowerControl(prGlueInfo->prAdapter);
    }

    complete(&prGlueInfo->rHifHaltComp);

    DBGLOG(INIT, INFO, "%s:%u stopped!\n", KAL_GET_CURRENT_THREAD_NAME(),
           KAL_GET_CURRENT_THREAD_ID());

    return 0;
}

int rx_thread(void *data)
{
    struct net_device *dev = data;
    P_GLUE_INFO_T prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(dev));

    QUE_T rTempRxQue;
    P_QUE_T prTempRxQue = NULL;
    P_QUE_ENTRY_T prQueueEntry = NULL;

    int ret = 0;

    u32 u4LoopCount;

    /* for spin lock acquire and release */
    KAL_SPIN_LOCK_DECLARATION();

    DBGLOG(INIT, INFO, "%s:%u starts running...\n",
           KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

    prGlueInfo->u4RxThreadPid = KAL_GET_CURRENT_THREAD_ID();

    set_user_nice(current, prGlueInfo->prAdapter->rWifiVar.cThreadNice);

    prTempRxQue = &rTempRxQue;

    while (true) {
        if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
            DBGLOG(INIT, INFO, "rx_thread should stop now...\n");
            break;
        }

        /*
         * sleep on waitqueue if no events occurred.
         */
        do {
            ret = wait_event_interruptible(
                prGlueInfo->waitq_rx,
                ((prGlueInfo->ulFlag & GLUE_FLAG_RX_PROCESS) !=
                 0));
        } while (ret != 0);

        if (test_and_clear_bit(GLUE_FLAG_RX_TO_OS_BIT,
                               &prGlueInfo->ulFlag)) {
            u4LoopCount =
                prGlueInfo->prAdapter->rWifiVar.u4Rx2OsLoopCount;

            while (u4LoopCount--) {
                while (QUEUE_IS_NOT_EMPTY(
                           &prGlueInfo->prAdapter->rRxQueue)) {
                    QUEUE_INITIALIZE(prTempRxQue);

                    GLUE_ACQUIRE_SPIN_LOCK(
                        prGlueInfo,
                        SPIN_LOCK_RX_TO_OS_QUE);
                    QUEUE_MOVE_ALL(prTempRxQue,
                                   &prGlueInfo->prAdapter
                                   ->rRxQueue);
                    GLUE_RELEASE_SPIN_LOCK(
                        prGlueInfo,
                        SPIN_LOCK_RX_TO_OS_QUE);

                    while (QUEUE_IS_NOT_EMPTY(
                               prTempRxQue)) {
                        QUEUE_REMOVE_HEAD(
                            prTempRxQue,
                            prQueueEntry,
                            P_QUE_ENTRY_T);
                        kalRxIndicateOnePkt(
                            prGlueInfo,
                            (void *)
                            GLUE_GET_PKT_DESCRIPTOR(
                                prQueueEntry));
                    }
                }
            }
        }
    }

    complete(&prGlueInfo->rRxHaltComp);

    DBGLOG(INIT, INFO, "%s:%u stopped!\n", KAL_GET_CURRENT_THREAD_NAME(),
           KAL_GET_CURRENT_THREAD_ID());

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is a kernel thread function for handling command packets
 * Tx requests and interrupt events
 *
 * @param data       data pointer to private data of main_thread
 *
 * @retval           If the function succeeds, the return value is 0.
 * Otherwise, an error code is returned.
 *
 */
/*----------------------------------------------------------------------------*/

int main_thread(void *data)
{
    struct net_device *dev = data;
    P_GLUE_INFO_T prGlueInfo = ((P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(dev))->prGlueInfo;
    P_GL_IO_REQ_T prIoReq = NULL;
    int ret = 0;
    u8 fgNeedHwAccess = false;

    prGlueInfo->u4TxThreadPid = KAL_GET_CURRENT_THREAD_ID();

    current->flags |= PF_NOFREEZE;
    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prAdapter);
    set_user_nice(current, prGlueInfo->prAdapter->rWifiVar.cThreadNice);

    DBGLOG(INIT, INFO, "%s:%u starts running...\n",
           KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

    while (true) {
        /*run p2p multicast list work. */
        if (test_and_clear_bit(GLUE_FLAG_SUB_MOD_MULTICAST_BIT,
                               &prGlueInfo->ulFlag)) {
            p2pSetMulticastListWorkQueueWrapper(prGlueInfo);
        }

        if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
            DBGLOG(INIT, INFO, "%s should stop now...\n",
                   KAL_GET_CURRENT_THREAD_NAME());
            break;
        }

        /*
         * sleep on waitqueue if no events occurred. Event contain (1)
         * GLUE_FLAG_INT (2) GLUE_FLAG_OID (3) GLUE_FLAG_TXREQ (4)
         * GLUE_FLAG_HALT
         *
         */
        do {
            ret = wait_event_interruptible(
                prGlueInfo->waitq, ((prGlueInfo->ulFlag &
                  GLUE_FLAG_MAIN_PROCESS) != 0));
        } while (ret != 0);

#if CFG_ENABLE_WIFI_DIRECT
        /*run p2p multicast list work. */
        if (test_and_clear_bit(GLUE_FLAG_SUB_MOD_MULTICAST_BIT,
                               &prGlueInfo->ulFlag)) {
            p2pSetMulticastListWorkQueueWrapper(prGlueInfo);
        }

        if (test_and_clear_bit(GLUE_FLAG_FRAME_FILTER_BIT,
                               &prGlueInfo->ulFlag)) {
            p2pFuncUpdateMgmtFrameRegister(
                prGlueInfo->prAdapter,
                prGlueInfo->prP2PDevInfo->u4OsMgmtFrameFilter);
        }
#endif
        if (test_and_clear_bit(GLUE_FLAG_FRAME_FILTER_AIS_BIT,
                               &prGlueInfo->ulFlag)) {
            P_AIS_FSM_INFO_T prAisFsmInfo = (P_AIS_FSM_INFO_T)NULL;

            prAisFsmInfo =
                &(prGlueInfo->prAdapter->rWifiVar.rAisFsmInfo);
#if CFG_SUPPORT_PER_BSS_FILTER
            if (prAisFsmInfo->u4AisPacketFilter ^
                prGlueInfo->u4OsMgmtFrameFilter) {
                /* Filter setings changed. */
                CMD_RX_PACKET_FILTER rSetRxPacketFilter;
                u32 u4OsFilter = 0;
                prAisFsmInfo->u4AisPacketFilter =
                    prGlueInfo->u4OsMgmtFrameFilter;

                kalMemZero(&rSetRxPacketFilter,
                           sizeof(rSetRxPacketFilter));

                /* For not impact original functionality. */
                rSetRxPacketFilter.u4RxPacketFilter =
                    prGlueInfo->prAdapter->u4OsPacketFilter;

                if (prGlueInfo->prAdapter->prAisBssInfo) {
                    rSetRxPacketFilter.ucIsPerBssFilter =
                        true;
                    rSetRxPacketFilter.ucBssIndex =
                        prGlueInfo->prAdapter
                        ->prAisBssInfo
                        ->ucBssIndex;
                    rSetRxPacketFilter.u4BssMgmtFilter =
                        prAisFsmInfo->u4AisPacketFilter;

                    wlanSendSetQueryCmd(
                        prGlueInfo->prAdapter,
                        CMD_ID_SET_RX_FILTER, true,
                        false, false,
                        nicCmdEventSetCommon,
                        nicOidCmdTimeoutCommon,
                        sizeof(CMD_RX_PACKET_FILTER),
                        (u8 *)&rSetRxPacketFilter,
                        &u4OsFilter,
                        sizeof(u4OsFilter));
                }
            }
#else
            prAisFsmInfo->u4AisPacketFilter =
                prGlueInfo->u4OsMgmtFrameFilter;
#endif
        }

        if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
            DBGLOG(INIT, INFO, "%s should stop now...\n",
                   KAL_GET_CURRENT_THREAD_NAME());
            break;
        }

        fgNeedHwAccess = false;

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
        if (prGlueInfo->fgEnSdioTestPattern == true) {
            if (fgNeedHwAccess == false) {
                fgNeedHwAccess = true;

                wlanAcquirePowerControl(prGlueInfo->prAdapter);
            }

            if (prGlueInfo->fgIsSdioTestInitialized == false) {
                /* enable PRBS mode */
                kalDevRegWrite(prGlueInfo, MCR_WTMCR,
                               0x00080002);
                prGlueInfo->fgIsSdioTestInitialized = true;
            }

            if (prGlueInfo->fgSdioReadWriteMode == true) {
                /* read test */
                kalDevPortRead(
                    prGlueInfo, MCR_WTMDR, 256,
                    prGlueInfo->aucSdioTestBuffer,
                    sizeof(prGlueInfo->aucSdioTestBuffer));
            } else {
                /* write test */
                kalDevPortWrite(
                    prGlueInfo, MCR_WTMDR, 172,
                    prGlueInfo->aucSdioTestBuffer,
                    sizeof(prGlueInfo->aucSdioTestBuffer));
            }
        }
#endif

        /* transfer ioctl to OID request */
        do {
            if (test_and_clear_bit(GLUE_FLAG_OID_BIT,
                                   &prGlueInfo->ulFlag)) {
                /* get current prIoReq */
                prIoReq = &(prGlueInfo->OidEntry);
                if (prIoReq->fgRead == false) {
                    prIoReq->rStatus = wlanSetInformation(
                        prIoReq->prAdapter,
                        prIoReq->pfnOidHandler,
                        prIoReq->pvInfoBuf,
                        prIoReq->u4InfoBufLen,
                        prIoReq->pu4QryInfoLen);
                } else {
                    prIoReq->rStatus = wlanQueryInformation(
                        prIoReq->prAdapter,
                        prIoReq->pfnOidHandler,
                        prIoReq->pvInfoBuf,
                        prIoReq->u4InfoBufLen,
                        prIoReq->pu4QryInfoLen);
                }

                if (prIoReq->rStatus != WLAN_STATUS_PENDING) {
                    /* complete ONLY if there are waiters */
                    if (!completion_done(&prGlueInfo->rPendComp)) {
                        complete(&prGlueInfo->rPendComp);
                    } else {
                        DBGLOG(INIT,
                               WARN,
                               "SKIP multiple OID complete!\n");
                    }
                } else {
                    wlanoidTimeoutCheck(
                        prGlueInfo->prAdapter,
                        prIoReq->pfnOidHandler,
                        prIoReq->u4Timeout);
                }
            }
        } while (false);

        /*
         *
         * if TX request, clear the TXREQ flag. TXREQ set by
         * kalSetEvent/GlueSetEvent indicates the following requests
         * occur
         *
         */

        if (test_and_clear_bit(GLUE_FLAG_TXREQ_BIT,
                               &prGlueInfo->ulFlag)) {
            kalProcessTxReq(prGlueInfo, &fgNeedHwAccess);
        }
        /* Process RX */
        if (test_and_clear_bit(GLUE_FLAG_RX_BIT, &prGlueInfo->ulFlag)) {
            nicRxProcessRFBs(prGlueInfo->prAdapter);
        }
        if (test_and_clear_bit(GLUE_FLAG_TX_CMD_DONE_BIT,
                               &prGlueInfo->ulFlag)) {
            wlanTxCmdDoneMthread(prGlueInfo->prAdapter);
        }
        /* handle cnmTimer time out */
        if (test_and_clear_bit(GLUE_FLAG_TIMEOUT_BIT,
                               &prGlueInfo->ulFlag)) {
            wlanTimerTimeoutCheck(prGlueInfo->prAdapter);
        }
#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
        if (prGlueInfo->fgEnSdioTestPattern == true) {
            kalSetEvent(prGlueInfo);
        }
#endif
    }

    /* flush the pending TX packets */
    if (GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum) > 0) {
        kalFlushPendingTxPackets(prGlueInfo);
    }

    /* flush pending security frames */
    if (GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingSecurityFrameNum) > 0) {
        kalClearSecurityFrames(prGlueInfo);
    }

    /* remove pending oid */
    wlanReleasePendingOid(prGlueInfo->prAdapter, 0);

    complete(&prGlueInfo->rHaltComp);

    DBGLOG(INIT, INFO, "%s:%u stopped!\n", KAL_GET_CURRENT_THREAD_NAME(),
           KAL_GET_CURRENT_THREAD_ID());

    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to check if card is removed
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval true:     card is removed
 *         false:    card is still attached
 */
/*----------------------------------------------------------------------------*/
u8 kalIsCardRemoved(IN P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);

    return false;
    /* Linux MMC doesn't have removal notification yet */
}

#ifdef CONFIG_IDME
#define IDME_MACADDR    "/proc/idme/mac_addr"
static int idme_get_mac_addr(unsigned char *mac_addr, size_t addr_len)
{
    unsigned char buf[IFHWADDRLEN * 2 + 1] = { "" }, str[3] = { "" };
    int i, mac[IFHWADDRLEN];
    mm_segment_t old_fs;
    struct file *f;
    size_t len;

    if (!mac_addr || addr_len < IFHWADDRLEN) {
        DBGLOG(INIT, ERROR, "invalid mac_addr ptr or buf\n");
        return -1;
    }

    f = filp_open(IDME_MACADDR, O_RDONLY, 0);
    if (IS_ERR(f)) {
        DBGLOG(INIT, ERROR, "can't open mac addr file\n");
        return -1;
    }

    old_fs = get_fs();
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
    set_fs(KERNEL_DS);
#else
    set_fs(get_ds());
#endif
    f->f_op->read(f, buf, IFHWADDRLEN * 2, &f->f_pos);
    filp_close(f, NULL);
    set_fs(old_fs);

    if (strlen(buf) != IFHWADDRLEN * 2) {
        goto bailout;
    }

    for (i = 0; i < IFHWADDRLEN; i++) {
        str[0] = buf[i * 2];
        str[1] = buf[i * 2 + 1];
        if (!isxdigit(str[0]) || !isxdigit(str[1])) {
            goto bailout;
        }
        len = sscanf(str, "%02x", &mac[i]);
        if (len != 1) {
            goto bailout;
        }
    }
    for (i = 0; i < IFHWADDRLEN; i++)
        mac_addr[i] = (unsigned char)mac[i];
    return 0;

bailout:
    DBGLOG(INIT, ERROR, "wrong mac addr %02x %02x\n", buf[0], buf[1]);
    return -1;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to send command to firmware for overriding
 * netweork address
 *
 * \param pvGlueInfo Pointer of GLUE Data Structure
 *
 * \retval true
 *         false
 */
/*----------------------------------------------------------------------------*/
u8 kalRetrieveNetworkAddress(IN P_GLUE_INFO_T prGlueInfo,
                             IN OUT PARAM_MAC_ADDRESS *prMacAddr)
{
    P_ADAPTER_T prAdapter;
    ASSERT(prGlueInfo);

    prAdapter = prGlueInfo->prAdapter;

    /* Get MAC address override from wlan feature option */
    prGlueInfo->fgIsMacAddrOverride = prAdapter->rWifiVar.ucMacAddrOverride;

    wlanHwAddrToBin(prAdapter->rWifiVar.aucMacAddrStr,
                    prGlueInfo->rMacAddrOverride);

#ifdef CONFIG_IDME
    if (prMacAddr && 0 == idme_get_mac_addr((unsigned char *)prMacAddr,
                                            sizeof(PARAM_MAC_ADDRESS))) {
        DBGLOG(INIT, INFO, "use IDME mac addr\n");
        return true;
    }
#endif

    if (prGlueInfo->fgIsMacAddrOverride == false) {
        u32 i;
        u8 fgIsReadError = false;

#if !defined(CONFIG_X86)
        for (i = 0; i < MAC_ADDR_LEN; i += 2) {
            if (kalCfgDataRead16(prGlueInfo,
                                 OFFSET_OF(WIFI_CFG_PARAM_STRUCT,
                                           aucMacAddress) +
                                 i,
                                 (u16 *)(((u8 *)prMacAddr) + i)) ==
                false) {
                fgIsReadError = true;
                break;
            }
        }
#else
        /* x86 Linux doesn't need to override network address so far */
        /*return false;*/
        /*Modify for Linux PC support NVRAM Setting*/
        for (i = 0; i < MAC_ADDR_LEN; i += 2) {
            if (kalCfgDataRead16(prGlueInfo,
                                 OFFSET_OF(WIFI_CFG_PARAM_STRUCT,
                                           aucMacAddress) +
                                 i,
                                 (u16 *)(((u8 *)prMacAddr) + i)) ==
                false) {
                fgIsReadError = true;
                break;
            }
        }
#endif

#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
        /* retrieve buffer mode efuse */
        if (prAdapter->rWifiVar.ucEfuseBufferModeCal ==
            LOAD_EEPROM_BIN) {
            if (wlanExtractBufferBin(prAdapter) ==
                WLAN_STATUS_SUCCESS) {
                u32 u4BinOffset =
                    prAdapter->u4EfuseMacAddrOffset;

                /* Update MAC address */
                kalMemCopy(prMacAddr,
                           &uacEEPROMImage[u4BinOffset],
                           MAC_ADDR_LEN);
                fgIsReadError = false;
            } else {
                fgIsReadError = true;
            }
        }
#endif
        /* return retrieve result */
        if (fgIsReadError == true) {
            return false;
        }else{
            return true;
        }
    } else {
        COPY_MAC_ADDR(prMacAddr, prGlueInfo->rMacAddrOverride);

        return true;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to flush pending TX packets in glue layer
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalFlushPendingTxPackets(IN P_GLUE_INFO_T prGlueInfo)
{
    P_QUE_T prTxQue;
    P_QUE_ENTRY_T prQueueEntry;
    void *prPacket;
#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
    P_QUE_T prTxLookBackQue;
#endif
    ASSERT(prGlueInfo);

    prTxQue = &(prGlueInfo->rTxQueue);

    if (GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum) == 0) {
        return;
    }

    if (HAL_IS_TX_DIRECT()) {
        nicTxDirectClearSkbQ(prGlueInfo->prAdapter);
    } else {
        GLUE_SPIN_LOCK_DECLARATION();

        while (true) {
            GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
            QUEUE_REMOVE_HEAD(prTxQue, prQueueEntry, P_QUE_ENTRY_T);
            GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);

            if (prQueueEntry == NULL) {
                break;
            }

            prPacket = GLUE_GET_PKT_DESCRIPTOR(prQueueEntry);

            kalSendComplete(prGlueInfo, prPacket,
                            WLAN_STATUS_NOT_ACCEPTED);
        }
    }

#ifdef CFG_SUPPORT_MULTICAST_ENHANCEMENT_LOOKBACK
    /* Flush out look back queu */
    prTxLookBackQue = &(prGlueInfo->rTxLookBackQueue);
    while (true) {
        GLUE_SPIN_LOCK_DECLARATION();

        GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);
        QUEUE_REMOVE_HEAD(prTxLookBackQue, prQueueEntry, P_QUE_ENTRY_T);
        GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_LB_QUE);

        if (prQueueEntry == NULL) {
            break;
        }

        prPacket = GLUE_GET_PKT_DESCRIPTOR(prQueueEntry);
        dev_kfree_skb(prPacket);
    }
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is get indicated media state
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval
 */
/*----------------------------------------------------------------------------*/
ENUM_PARAM_MEDIA_STATE_T kalGetMediaStateIndicated(
    IN P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);

    return prGlueInfo->eParamMediaStateIndicated;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set indicated media state
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalSetMediaStateIndicated(IN P_GLUE_INFO_T prGlueInfo,
                               IN ENUM_PARAM_MEDIA_STATE_T
                               eParamMediaStateIndicate)
{
    ASSERT(prGlueInfo);

    prGlueInfo->eParamMediaStateIndicated = eParamMediaStateIndicate;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear pending OID staying in command queue
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalOidCmdClearance(IN P_GLUE_INFO_T prGlueInfo)
{
    P_QUE_T prCmdQue;
    QUE_T rTempCmdQue;
    P_QUE_T prTempCmdQue = &rTempCmdQue;
    QUE_T rReturnCmdQue;
    P_QUE_T prReturnCmdQue = &rReturnCmdQue;
    P_QUE_ENTRY_T prQueueEntry = (P_QUE_ENTRY_T)NULL;
    P_CMD_INFO_T prCmdInfo = (P_CMD_INFO_T)NULL;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);

    QUEUE_INITIALIZE(prReturnCmdQue);

    prCmdQue = &prGlueInfo->rCmdQueue;

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

    QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    while (prQueueEntry) {
        if (((P_CMD_INFO_T)prQueueEntry)->fgIsOid) {
            prCmdInfo = (P_CMD_INFO_T)prQueueEntry;
            break;
        }
        QUEUE_INSERT_TAIL(prReturnCmdQue, prQueueEntry);
        QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, P_QUE_ENTRY_T);
    }

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prReturnCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

    if (prCmdInfo) {
        if (prCmdInfo->pfCmdTimeoutHandler) {
            prCmdInfo->pfCmdTimeoutHandler(prGlueInfo->prAdapter,
                                           prCmdInfo);
        } else {
            kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, 0,
                           WLAN_STATUS_NOT_ACCEPTED);
        }

        prGlueInfo->u4OidCompleteFlag = 1;
        cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
        GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to insert command into prCmdQueue
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *        prQueueEntry   Pointer of queue entry to be inserted
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalEnqueueCommand(IN P_GLUE_INFO_T prGlueInfo,
                       IN P_QUE_ENTRY_T prQueueEntry)
{
    P_QUE_T prCmdQue;
    P_CMD_INFO_T prCmdInfo;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);
    ASSERT(prQueueEntry);

    prCmdQue = &prGlueInfo->rCmdQueue;

    prCmdInfo = (P_CMD_INFO_T)prQueueEntry;

    DBGLOG(INIT, INFO, "EN-Q CMD TYPE[%u] ID[0x%02X] SEQ[%u] to CMD Q\n",
           prCmdInfo->eCmdType, prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);
    GLUE_INC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Handle EVENT_ID_ASSOC_INFO event packet by indicating to OS with
 *        proper information
 *
 * @param pvGlueInfo     Pointer of GLUE Data Structure
 * @param prAssocInfo    Pointer of EVENT_ID_ASSOC_INFO Packet
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void kalHandleAssocInfo(IN P_GLUE_INFO_T prGlueInfo,
                        IN P_EVENT_ASSOC_INFO prAssocInfo)
{
    /* to do */
}

/*----------------------------------------------------------------------------*/
/*!
** @brief Notify OS with SendComplete event of the specific packet. Linux should
**        free packets here.
**
** @param pvGlueInfo     Pointer of GLUE Data Structure
** @param pvPacket       Pointer of Packet Handle
** @param status         Status Code for OS upper layer
**
** @return none
*/
/*----------------------------------------------------------------------------*/

/* / Todo */
void kalSecurityFrameSendComplete(IN P_GLUE_INFO_T prGlueInfo,
                                  IN void *pvPacket,
                                  IN WLAN_STATUS rStatus)
{
    ASSERT(pvPacket);

    /* dev_kfree_skb((struct sk_buff *) pvPacket); */
    kalSendCompleteAndAwakeQueue(prGlueInfo, pvPacket);
    GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingSecurityFrameNum);
}

u32 kalGetTxPendingFrameCount(IN P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);

    return (u32)(GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum));
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to retrieve the number of pending commands
 *        (including MMPDU, 802.1X and command packets)
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval
 */
/*----------------------------------------------------------------------------*/
u32 kalGetTxPendingCmdCount(IN P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);

    return (u32)GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Timer Initialization Procedure
 *
 * \param[in] prGlueInfo     Pointer to GLUE Data Structure
 * \param[in] prTimerHandler Pointer to timer handling function, whose only
 *                           argument is "prAdapter"
 *
 * \retval none
 *
 */
/*----------------------------------------------------------------------------*/

/* static struct timer_list tickfn; */

void kalOsTimerInitialize(IN P_GLUE_INFO_T prGlueInfo,
                          IN void *prTimerHandler)
{
    ASSERT(prGlueInfo);

#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
    timer_setup(&(prGlueInfo->tickfn), prTimerHandler, 0);
#else
    init_timer(&(prGlueInfo->tickfn));
    prGlueInfo->tickfn.function = prTimerHandler;
    prGlueInfo->tickfn.data = (unsigned long)prGlueInfo;
#endif
}

/* Todo */
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the time to do the time out check.
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 * \param[in] rInterval  Time out interval from current time.
 *
 * \retval true Success.
 */
/*----------------------------------------------------------------------------*/
u8 kalSetTimer(IN P_GLUE_INFO_T prGlueInfo, IN u32 u4Interval)
{
    ASSERT(prGlueInfo);

    if (HAL_IS_RX_DIRECT(prGlueInfo->prAdapter)) {
        mod_timer(&prGlueInfo->tickfn,
                  jiffies + u4Interval * HZ / MSEC_PER_SEC);
    } else {
        timer_delete_sync(&(prGlueInfo->tickfn));

        prGlueInfo->tickfn.expires =
            jiffies + u4Interval * HZ / MSEC_PER_SEC;
        add_timer(&(prGlueInfo->tickfn));
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to cancel
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 *
 * \retval true  :   Timer has been canceled
 *         FALAE :   Timer doens't exist
 */
/*----------------------------------------------------------------------------*/
u8 kalCancelTimer(IN P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);

    clear_bit(GLUE_FLAG_TIMEOUT_BIT, &prGlueInfo->ulFlag);

    if (timer_delete_sync(&(prGlueInfo->tickfn)) >= 0) {
        return true;
    }else{
        return false;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is a callback function for scanning done
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 *
 * \retval none
 *
 */
/*----------------------------------------------------------------------------*/
void kalScanDone(IN P_GLUE_INFO_T prGlueInfo,
                 IN ENUM_KAL_NETWORK_TYPE_INDEX_T eNetTypeIdx,
                 IN WLAN_STATUS status)
{
    ASSERT(prGlueInfo);

    scanReportBss2Cfg80211(prGlueInfo->prAdapter, BSS_TYPE_INFRASTRUCTURE,
                           NULL);

    /* check for system configuration for generating error message on scan
     * list */
    wlanCheckSystemConfiguration(prGlueInfo->prAdapter);

    kalIndicateStatusAndComplete(prGlueInfo, WLAN_STATUS_SCAN_COMPLETE,
                                 NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to generate a random number
 *
 * \param none
 *
 * \retval u32
 */
/*----------------------------------------------------------------------------*/
u32 kalRandomNumber(void)
{
    u32 number = 0;

    get_random_bytes(&number, 4);

    return number;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief command timeout call-back function
 *
 * \param[in] prGlueInfo Pointer to the GLUE data structure.
 *
 * \retval (none)
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
void kalTimeoutHandler(struct timer_list *timer)
#else
void kalTimeoutHandler(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
    P_GLUE_INFO_T prGlueInfo = from_timer(prGlueInfo, timer, tickfn);
#else
    P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)arg;
#endif

    ASSERT(prGlueInfo);

    /* Notify tx thread  for timeout event */
    set_bit(GLUE_FLAG_TIMEOUT_BIT, &prGlueInfo->ulFlag);
    wake_up_interruptible(&prGlueInfo->waitq);
}

void kalSetEvent(P_GLUE_INFO_T pr)
{
    set_bit(GLUE_FLAG_TXREQ_BIT, &pr->ulFlag);
    wake_up_interruptible(&pr->waitq);
}

void kalSetIntEvent(P_GLUE_INFO_T pr)
{
    set_bit(GLUE_FLAG_INT_BIT, &pr->ulFlag);

    /* when we got interrupt, we wake up servie thread */
    wake_up_interruptible(&pr->waitq_hif);
}

void kalSetTxEvent2Hif(P_GLUE_INFO_T pr)
{
    if (!pr->hif_thread) {
        return;
    }

    set_bit(GLUE_FLAG_HIF_TX_BIT, &pr->ulFlag);
    wake_up_interruptible(&pr->waitq_hif);
}

void kalSetFwOwnEvent2Hif(P_GLUE_INFO_T pr)
{
    if (!pr->hif_thread) {
        return;
    }

    set_bit(GLUE_FLAG_HIF_FW_OWN_BIT, &pr->ulFlag);
    wake_up_interruptible(&pr->waitq_hif);
}

void kalSetTxEvent2Rx(P_GLUE_INFO_T pr)
{
    if (!pr->rx_thread) {
        return;
    }

    set_bit(GLUE_FLAG_RX_TO_OS_BIT, &pr->ulFlag);
    wake_up_interruptible(&pr->waitq_rx);
}

void kalSetTxCmdEvent2Hif(P_GLUE_INFO_T pr)
{
    if (!pr->hif_thread) {
        return;
    }

    set_bit(GLUE_FLAG_HIF_TX_CMD_BIT, &pr->ulFlag);
    wake_up_interruptible(&pr->waitq_hif);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if configuration file (NVRAM/Registry) exists
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           true
 *           false
 */
/*----------------------------------------------------------------------------*/
u8 kalIsConfigurationExist(IN P_GLUE_INFO_T prGlueInfo)
{
#if !defined(CONFIG_X86)
    ASSERT(prGlueInfo);

    return prGlueInfo->fgNvramAvailable;

#else
    /* there is no configuration data for x86-linux */
    /*return false;*/

    /*Modify for Linux PC support NVRAM Setting*/
    return prGlueInfo->fgNvramAvailable;

#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to retrieve Registry information
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           Pointer of REG_INFO_T
 */
/*----------------------------------------------------------------------------*/
P_REG_INFO_T kalGetConfiguration(IN P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);

    return &(prGlueInfo->rRegInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to retrieve version information of corresponding configuration file
 *
 * \param[in]
 *           prGlueInfo
 *
 * \param[out]
 *           pu2Part1CfgOwnVersion
 *           pu2Part1CfgPeerVersion
 *           pu2Part2CfgOwnVersion
 *           pu2Part2CfgPeerVersion
 *
 * \return
 *           NONE
 */
/*----------------------------------------------------------------------------*/
void kalGetConfigurationVersion(IN P_GLUE_INFO_T prGlueInfo,
                                OUT u16 *pu2Part1CfgOwnVersion,
                                OUT u16 *pu2Part1CfgPeerVersion,
                                OUT u16 *pu2Part2CfgOwnVersion,
                                OUT u16 *pu2Part2CfgPeerVersion)
{
    ASSERT(prGlueInfo);

    ASSERT(pu2Part1CfgOwnVersion);
    ASSERT(pu2Part1CfgPeerVersion);
    ASSERT(pu2Part2CfgOwnVersion);
    ASSERT(pu2Part2CfgPeerVersion);

    kalCfgDataRead16(prGlueInfo,
                     OFFSET_OF(WIFI_CFG_PARAM_STRUCT, u2Part1OwnVersion),
                     pu2Part1CfgOwnVersion);

    kalCfgDataRead16(prGlueInfo,
                     OFFSET_OF(WIFI_CFG_PARAM_STRUCT, u2Part1PeerVersion),
                     pu2Part1CfgPeerVersion);

    kalCfgDataRead16(prGlueInfo,
                     OFFSET_OF(WIFI_CFG_PARAM_STRUCT, u2Part2OwnVersion),
                     pu2Part2CfgOwnVersion);

    kalCfgDataRead16(prGlueInfo,
                     OFFSET_OF(WIFI_CFG_PARAM_STRUCT, u2Part2PeerVersion),
                     pu2Part2CfgPeerVersion);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if the WPS is active or not
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           true
 *           false
 */
/*----------------------------------------------------------------------------*/
u8 kalWSCGetActiveState(IN P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);

    return prGlueInfo->fgWpsActive;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief update RSSI and LinkQuality to GLUE layer
 *
 * \param[in]
 *           prGlueInfo
 *           eNetTypeIdx
 *           cRssi
 *           cLinkQuality
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
void kalUpdateRSSI(IN P_GLUE_INFO_T prGlueInfo,
                   IN ENUM_KAL_NETWORK_TYPE_INDEX_T eNetTypeIdx,
                   IN s8 cRssi,
                   IN s8 cLinkQuality)
{
    struct iw_statistics *pStats = (struct iw_statistics *)NULL;

    ASSERT(prGlueInfo);

    switch (eNetTypeIdx) {
    case KAL_NETWORK_TYPE_AIS_INDEX:
        pStats = (struct iw_statistics *)(&(prGlueInfo->rIwStats));
        break;

#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_P2P_RSSI_QUERY
    case KAL_NETWORK_TYPE_P2P_INDEX:
        pStats = (struct iw_statistics *)(&(prGlueInfo->rP2pIwStats));
        break;

#endif
#endif
    default:
        break;
    }

    if (pStats) {
        pStats->qual.qual = cLinkQuality;
        pStats->qual.noise = 0;
        pStats->qual.updated = IW_QUAL_QUAL_UPDATED |
                               IW_QUAL_NOISE_UPDATED;
        pStats->qual.level = 0x100 + cRssi;
        pStats->qual.updated |= IW_QUAL_LEVEL_UPDATED;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Pre-allocate I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           true
 *           false
 */
/*----------------------------------------------------------------------------*/
u8 kalInitIOBuffer(u8 is_pre_alloc)
{
    u32 u4Size;

    /* not pre-allocation for all memory usage */
    if (!is_pre_alloc) {
        pvIoBuffer = NULL;
        return false;
    }

    /* pre-allocation for all memory usage */
    if (HIF_TX_COALESCING_BUFFER_SIZE > HIF_RX_COALESCING_BUFFER_SIZE) {
        u4Size = HIF_TX_COALESCING_BUFFER_SIZE;
    }else{
        u4Size = HIF_RX_COALESCING_BUFFER_SIZE;
    }

    u4Size += HIF_EXTRA_IO_BUFFER_SIZE;

    pvIoBuffer = kmalloc(u4Size, GFP_KERNEL);
    if (pvIoBuffer) {
        pvIoBufferSize = u4Size;
        pvIoBufferUsage = 0;

        return true;
    }

    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Free pre-allocated I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalUninitIOBuffer(void)
{
    kfree(pvIoBuffer);

    pvIoBuffer = (void *)NULL;
    pvIoBufferSize = 0;
    pvIoBufferUsage = 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Dispatch pre-allocated I/O buffer
 *
 * \param[in]
 *           u4AllocSize
 *
 * \return
 *           void * for pointer of pre-allocated I/O buffer
 */
/*----------------------------------------------------------------------------*/
void *kalAllocateIOBuffer(IN u32 u4AllocSize)
{
    void *ret = (void *)NULL;

    if (pvIoBuffer) {
        if (u4AllocSize <= (pvIoBufferSize - pvIoBufferUsage)) {
            ret = (void *)&(((u8 *)(pvIoBuffer))[pvIoBufferUsage]);
            pvIoBufferUsage += u4AllocSize;
        }
    } else {
        /* fault tolerance */
        ret = (void *)kalMemAlloc(u4AllocSize, PHY_MEM_TYPE);
    }

    return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Release all dispatched I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalReleaseIOBuffer(IN void *pvAddr, IN u32 u4Size)
{
    if (pvIoBuffer) {
        pvIoBufferUsage -= u4Size;
    } else {
        /* fault tolerance */
        kalMemFree(pvAddr, PHY_MEM_TYPE, u4Size);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void kalGetChannelList(IN P_GLUE_INFO_T prGlueInfo,
                       IN ENUM_BAND_T eSpecificBand, IN u8 ucMaxChannelNum,
                       IN u8 *pucNumOfChannel,
                       IN P_RF_CHANNEL_INFO_T paucChannelList)
{
    rlmDomainGetChnlList(prGlueInfo->prAdapter, eSpecificBand, false,
                         ucMaxChannelNum, pucNumOfChannel, paucChannelList);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
u8 kalIsAPmode(IN P_GLUE_INFO_T prGlueInfo)
{
    return false;
}

#if CFG_SUPPORT_802_11W
/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if the MFP is active or not
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           true
 *           false
 */
/*----------------------------------------------------------------------------*/
u32 kalGetMfpSetting(IN P_GLUE_INFO_T prGlueInfo)
{
    u32 u4RsnMfp = RSN_AUTH_MFP_DISABLED;

    ASSERT(prGlueInfo);

    switch (prGlueInfo->rWpaInfo.u4Mfp) {
    case IW_AUTH_MFP_DISABLED:
        u4RsnMfp = RSN_AUTH_MFP_DISABLED;
        break;

    case IW_AUTH_MFP_OPTIONAL:
        u4RsnMfp = RSN_AUTH_MFP_OPTIONAL;
        break;

    case IW_AUTH_MFP_REQUIRED:
        u4RsnMfp = RSN_AUTH_MFP_REQUIRED;
        break;

    default:
        u4RsnMfp = RSN_AUTH_MFP_DISABLED;
        break;
    }

    return u4RsnMfp;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if the RSN IE CAP setting from supplicant
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           true
 *           false
 */
/*----------------------------------------------------------------------------*/
u8 kalGetRsnIeMfpCap(IN P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);

    return prGlueInfo->rWpaInfo.ucRSNMfpCap;
}
#endif

struct file *kalFileOpen(const char *path, int flags, int rights)
{
    struct file *filp = NULL;
#ifdef set_fs
    mm_segment_t oldfs;
#endif
    int err = 0;

#ifdef set_fs
    oldfs = get_fs();
    set_fs(KERNEL_DS);
#endif
    filp = filp_open(path, flags, rights);

#ifdef set_fs
    set_fs(oldfs);
#endif
    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

void kalFileClose(struct file *file)
{
    filp_close(file, NULL);
}

u32 kalFileRead(struct file *file, unsigned long long offset,
                unsigned char *data, unsigned int size)
{
#if KERNEL_VERSION(4, 14, 0) <= CFG80211_VERSION_CODE
    return kernel_read(file, data, size, (loff_t *)&offset);

#else
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    ret = vfs_read(file, data, size, &offset);

    set_fs(oldfs);
    return ret;

#endif
}

u32 kalFileWrite(struct file *file, unsigned long long offset,
                 unsigned char *data, unsigned int size)
{
#if KERNEL_VERSION(4, 14, 0) <= CFG80211_VERSION_CODE
    return kernel_write(file, data, size, (loff_t *)&offset);

#else
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    ret = vfs_write(file, data, size, &offset);

    set_fs(oldfs);
    return ret;

#endif
}

u32 kalWriteToFile(const u8 *pucPath, u8 fgDoAppend, u8 *pucData,
                   u32 u4Size)
{
    struct file *file = NULL;
    u32 ret = 0;  /* size been written */
    u32 u4Flags = 0;

    if (fgDoAppend) {
        u4Flags = O_APPEND;
    }

    file = kalFileOpen(pucPath, O_WRONLY | O_CREAT | u4Flags, S_IRWXU);
    if (file) {
        ret = kalFileWrite(file, 0, pucData, u4Size);
        kalFileClose(file);
    }

    return ret;
}

s32 kalReadToFile(const u8 *pucPath, u8 *pucData, u32 u4Size,
                  u32 *pu4ReadSize)
{
    struct file *file = NULL;
    s32 ret = -1;
    u32 u4ReadSize = 0;

    DBGLOG(INIT, INFO, "kalReadToFile() path %s\n", pucPath);

    file = kalFileOpen(pucPath, O_RDONLY, 0);

    if ((file != NULL) && !IS_ERR(file)) {
        u4ReadSize = kalFileRead(file, 0, pucData, u4Size);
        kalFileClose(file);
        if (pu4ReadSize) {
            *pu4ReadSize = u4ReadSize;
        }
        ret = 0;
    }
    return ret;
}

u32 kalCheckPath(const u8 *pucPath)
{
    struct file *file = NULL;
    u32 u4Flags = 0;

    file = kalFileOpen(pucPath, O_WRONLY | O_CREAT | u4Flags, S_IRWXU);
    if (!file) {
        return -1;
    }

    kalFileClose(file);
    return 1;
}

u32 kalTrunkPath(const u8 *pucPath)
{
    struct file *file = NULL;
    u32 u4Flags = O_TRUNC;

    file = kalFileOpen(pucPath, O_WRONLY | O_CREAT | u4Flags, S_IRWXU);
    if (!file) {
        return -1;
    }

    kalFileClose(file);
    return 1;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief read request firmware file binary to pucData
 *
 * \param[in] pucPath  file name
 * \param[out] pucData  Request file output buffer
 * \param[in] u4Size  read size
 * \param[out] pu4ReadSize  real read size
 * \param[in] dev
 *
 * \return
 *           0 success
 *           >0 fail
 */
/*----------------------------------------------------------------------------*/
s32 kalRequestFirmware(const u8 *pucPath, u8 *pucData, u32 u4Size,
                       u32 *pu4ReadSize, struct device *dev)
{
    const struct firmware *fw;
    int ret = 0;

    /*
     * Driver support request_firmware() to get files
     * Android path: "/etc/firmware", "/vendor/firmware", "/firmware/image"
     * Linux path: "/lib/firmware", "/lib/firmware/update"
     */
    ret = REQUEST_FIRMWARE(&fw, pucPath, dev);

    if (ret != 0) {
        DBGLOG(INIT, INFO, "kalRequestFirmware %s Fail, errno[%d]!!\n",
               pucPath, ret);
        pucData = NULL;
        *pu4ReadSize = 0;
        release_firmware(fw);
        return ret;
    }

    DBGLOG(INIT, INFO, "kalRequestFirmware(): %s OK\n", pucPath);

    if (fw->size < u4Size) {
        u4Size = fw->size;
    }

    memcpy(pucData, fw->data, u4Size);
    if (pu4ReadSize) {
        *pu4ReadSize = u4Size;
    }

    release_firmware(fw);

    return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate BSS-INFO to NL80211 as scanning result
 *
 * \param[in]
 *           prGlueInfo
 *           pucBeaconProbeResp
 *           u4FrameLen
 *
 *
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalIndicateBssInfo(IN P_GLUE_INFO_T prGlueInfo,
                        IN u8 *pucBeaconProbeResp,
                        IN u32 u4FrameLen, IN u8 ucChannelNum,
                        IN s32 i4SignalStrength)
{
    struct wiphy *wiphy;
    struct ieee80211_channel *prChannel = NULL;

    ASSERT(prGlueInfo);
    wiphy = priv_to_wiphy(prGlueInfo);

    /* search through channel entries */
    if (ucChannelNum <= 14) {
        prChannel = ieee80211_get_channel(
            wiphy, ieee80211_channel_to_frequency(
                ucChannelNum, NL80211_BAND_2GHZ));
    } else {
        prChannel = ieee80211_get_channel(
            wiphy, ieee80211_channel_to_frequency(
                ucChannelNum, NL80211_BAND_5GHZ));
    }

    if (prChannel != NULL && prGlueInfo->fgIsRegistered == true) {
        struct cfg80211_bss *bss;
#if CFG_SUPPORT_TSF_USING_BOOTTIME
        struct ieee80211_mgmt *prMgmtFrame =
            (struct ieee80211_mgmt *)pucBeaconProbeResp;

        prMgmtFrame->u.beacon.timestamp = kalGetBootTime();
#endif

        /* indicate to NL80211 subsystem */
        bss = cfg80211_inform_bss_frame(
            wiphy, prChannel,
            (struct ieee80211_mgmt *)pucBeaconProbeResp, u4FrameLen,
            i4SignalStrength * 100, GFP_KERNEL);

        if (!bss) {
            DBGLOG(REQ,
                   WARN,
                   "cfg80211_inform_bss_frame() returned with NULL\n");
        } else {
            cfg80211_put_bss(wiphy, bss);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate channel ready
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalReadyOnChannel(IN P_GLUE_INFO_T prGlueInfo, IN u64 u8Cookie,
                       IN ENUM_BAND_T eBand, IN ENUM_CHNL_EXT_T eSco,
                       IN u8 ucChannelNum, IN u32 u4DurationMs)
{
    struct ieee80211_channel *prChannel = NULL;
    enum nl80211_channel_type rChannelType;

    /* ucChannelNum = wlanGetChannelNumberByNetwork(prGlueInfo->prAdapter,
     * NETWORK_TYPE_AIS_INDEX); */

    if (prGlueInfo->fgIsRegistered == true) {
        if (ucChannelNum <= 14) {
            prChannel = ieee80211_get_channel(
                priv_to_wiphy(prGlueInfo),
                ieee80211_channel_to_frequency(
                    ucChannelNum, NL80211_BAND_2GHZ));
        } else {
            prChannel = ieee80211_get_channel(
                priv_to_wiphy(prGlueInfo),
                ieee80211_channel_to_frequency(
                    ucChannelNum, NL80211_BAND_5GHZ));
        }

        if (!prChannel) {
            DBGLOG(INIT, ERROR, "ieee80211_get_channel fail!\n");
            return;
        }

        switch (eSco) {
        case CHNL_EXT_SCN:
            rChannelType = NL80211_CHAN_NO_HT;
            break;

        case CHNL_EXT_SCA:
            rChannelType = NL80211_CHAN_HT40MINUS;
            break;

        case CHNL_EXT_SCB:
            rChannelType = NL80211_CHAN_HT40PLUS;
            break;

        case CHNL_EXT_RES:
        default:
            rChannelType = NL80211_CHAN_HT20;
            break;
        }

        cfg80211_ready_on_channel(
            prGlueInfo->prDevHandler->ieee80211_ptr, u8Cookie,
            prChannel, u4DurationMs, GFP_KERNEL);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate channel expiration
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalRemainOnChannelExpired(IN P_GLUE_INFO_T prGlueInfo, IN u64 u8Cookie,
                               IN ENUM_BAND_T eBand,
                               IN ENUM_CHNL_EXT_T eSco,
                               IN u8 ucChannelNum)
{
    struct ieee80211_channel *prChannel = NULL;
    enum nl80211_channel_type rChannelType;

    ucChannelNum = wlanGetChannelNumberByNetwork(
        prGlueInfo->prAdapter,
        prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex);

    if (prGlueInfo->fgIsRegistered == true) {
        if (ucChannelNum <= 14) {
            prChannel = ieee80211_get_channel(
                priv_to_wiphy(prGlueInfo),
                ieee80211_channel_to_frequency(
                    ucChannelNum, NL80211_BAND_2GHZ));
        } else {
            prChannel = ieee80211_get_channel(
                priv_to_wiphy(prGlueInfo),
                ieee80211_channel_to_frequency(
                    ucChannelNum, NL80211_BAND_5GHZ));
        }

        if (!prChannel) {
            DBGLOG(INIT, ERROR, "ieee80211_get_channel fail!\n");
            return;
        }

        switch (eSco) {
        case CHNL_EXT_SCN:
            rChannelType = NL80211_CHAN_NO_HT;
            break;

        case CHNL_EXT_SCA:
            rChannelType = NL80211_CHAN_HT40MINUS;
            break;

        case CHNL_EXT_SCB:
            rChannelType = NL80211_CHAN_HT40PLUS;
            break;

        case CHNL_EXT_RES:
        default:
            rChannelType = NL80211_CHAN_HT20;
            break;
        }

        cfg80211_remain_on_channel_expired(
            prGlueInfo->prDevHandler->ieee80211_ptr, u8Cookie,
            prChannel, GFP_KERNEL);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate Mgmt tx status
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalIndicateMgmtTxStatus(IN P_GLUE_INFO_T prGlueInfo, IN u64 u8Cookie,
                             IN u8 fgIsAck, IN u8 *pucFrameBuf,
                             IN u32 u4FrameLen)
{
    do {
        if ((prGlueInfo == NULL) || (pucFrameBuf == NULL) ||
            (u4FrameLen == 0)) {
            DBGLOG(AIS, TRACE,
                   "Unexpected pointer PARAM. 0x%lx, 0x%lx, %ld.",
                   prGlueInfo, pucFrameBuf, u4FrameLen);
            ASSERT(false);
            break;
        }

        cfg80211_mgmt_tx_status(prGlueInfo->prDevHandler->ieee80211_ptr,
                                u8Cookie, pucFrameBuf, u4FrameLen,
                                fgIsAck, GFP_KERNEL);
    } while (false);
}

void kalIndicateRxMgmtFrame(IN P_GLUE_INFO_T prGlueInfo,
                            IN P_SW_RFB_T prSwRfb)
{
    s32 i4Freq = 0;
    u8 ucChnlNum = 0;

    do {
        if ((prGlueInfo == NULL) || (prSwRfb == NULL)) {
            ASSERT(false);
            break;
        }

        ucChnlNum = (u8)HAL_RX_STATUS_GET_CHNL_NUM(prSwRfb->prRxStatus);
        i4Freq = nicChannelNum2Freq(ucChnlNum) / 1000;

        cfg80211_rx_mgmt(prGlueInfo->prDevHandler->ieee80211_ptr,
                         i4Freq,  /* in MHz */
                         RCPI_TO_dBm((u8)nicRxGetRcpiValueFromRxv(
                                         RCPI_MODE_WF0, prSwRfb)),
                         prSwRfb->pvHeader, prSwRfb->u2PacketLen,
                         NL80211_RXMGMT_FLAG_ANSWERED);
    } while (false);
}

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
/*----------------------------------------------------------------------------*/
/*!
 * \brief    To configure SDIO test pattern mode
 *
 * \param[in]
 *           prGlueInfo
 *           fgEn
 *           fgRead
 *
 * \return
 *           true
 *           false
 */
/*----------------------------------------------------------------------------*/
u8 kalSetSdioTestPattern(IN P_GLUE_INFO_T prGlueInfo, IN u8 fgEn,
                         IN u8 fgRead)
{
    const u8 aucPattern[] = {
        0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
        0x80, 0x80, 0x80, 0x7f, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x7f,
        0x80, 0x7f, 0x7f, 0x7f, 0x40, 0x40, 0x40, 0xbf, 0x40, 0x40,
        0x40, 0xbf, 0xbf, 0xbf, 0x40, 0xbf, 0xbf, 0xbf, 0x20, 0x20,
        0x20, 0xdf, 0x20, 0x20, 0x20, 0xdf, 0xdf, 0xdf, 0x20, 0xdf,
        0xdf, 0xdf, 0x10, 0x10, 0x10, 0xef, 0x10, 0x10, 0x10, 0xef,
        0xef, 0xef, 0x10, 0xef, 0xef, 0xef, 0x08, 0x08, 0x08, 0xf7,
        0x08, 0x08, 0x08, 0xf7, 0xf7, 0xf7, 0x08, 0xf7, 0xf7, 0xf7,
        0x04, 0x04, 0x04, 0xfb, 0x04, 0x04, 0x04, 0xfb, 0xfb, 0xfb,
        0x04, 0xfb, 0xfb, 0xfb, 0x02, 0x02, 0x02, 0xfd, 0x02, 0x02,
        0x02, 0xfd, 0xfd, 0xfd, 0x02, 0xfd, 0xfd, 0xfd, 0x01, 0x01,
        0x01, 0xfe, 0x01, 0x01, 0x01, 0xfe, 0xfe, 0xfe, 0x01, 0xfe,
        0xfe, 0xfe, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
        0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff,
        0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
        0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0x00, 0xff
    };
    u32 i;

    ASSERT(prGlueInfo);

    /* access to MCR_WTMCR to engage PRBS mode */
    prGlueInfo->fgEnSdioTestPattern = fgEn;
    prGlueInfo->fgSdioReadWriteMode = fgRead;

    if (fgRead == false) {
        /* fill buffer for data to be written */
        for (i = 0; i < sizeof(aucPattern); i++)
            prGlueInfo->aucSdioTestBuffer[i] = aucPattern[i];
    }

    return true;
}
#endif

#if (CFG_MET_PACKET_TRACE_SUPPORT == 1)
#define PROC_MET_PROF_CTRL    "met_ctrl"
#define PROC_MET_PROF_PORT    "met_port"

struct proc_dir_entry *pMetProcDir;
void *pMetGlobalData = NULL;

#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate scheduled scan results are avilable
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
void kalSchedScanResults(IN P_GLUE_INFO_T prGlueInfo)
{
    ASSERT(prGlueInfo);

#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
    cfg80211_sched_scan_results(priv_to_wiphy(prGlueInfo),
                                prGlueInfo->prSchedScanRequest->reqid);
#else
    cfg80211_sched_scan_results(priv_to_wiphy(prGlueInfo));
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate scheduled scan has been stopped
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
void kalSchedScanStopped(IN P_GLUE_INFO_T prGlueInfo)
{
    /* DBGLOG(SCN, INFO, ("-->kalSchedScanStopped\n" )); */

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);

    /* 1. reset first for newly incoming request */
    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
    if (prGlueInfo->prSchedScanRequest != NULL) {
        prGlueInfo->prSchedScanRequest = NULL;
    }
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

    DBGLOG(SCN, INFO, "cfg80211_sched_scan_stopped send event\n");

    /* 2. indication to cfg80211 */
    /* 20150205 change cfg80211_sched_scan_stopped to work queue to use K
     * thread to send event instead of Tx thread due to sched_scan_mtx dead
     * lock issue by Tx thread serves oid cmds and send event in the same
     * time
     */
    DBGLOG(SCN, INFO, "start work queue to send event\n");
    schedule_delayed_work(&sched_workq, 0);
    DBGLOG(SCN, INFO, "main_thread return from kalSchedScanStoppped\n");
}

void kalWDevLockThread(IN P_GLUE_INFO_T prGlueInfo,
                       IN struct net_device *pDev,
                       IN enum ENUM_CFG80211_WDEV_LOCK_FUNC fn,
                       IN u8 *pFrameBuf, IN size_t frameLen,
                       IN struct cfg80211_bss *pBss, IN s32 uapsd_queues,
                       const u8 *req_ies, size_t req_ies_len,
                       IN u8 fgIsInterruptContext)
{
    P_PARAM_WDEV_LOCK_THREAD_T pParamWDevLock = NULL;
    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);

    DBGLOG(REQ, INFO, "kalWDevLockThread\n");

    if (in_interrupt() && fgIsInterruptContext) {
        DBGLOG(REQ,
               STATE,
               "pParamWDevLock is allocated as PHY_MEM_TYPE in intr context\n");
        pParamWDevLock = (P_PARAM_WDEV_LOCK_THREAD_T)kalMemAlloc(
            sizeof(PARAM_WDEV_LOCK_THREAD_T), PHY_MEM_TYPE);
    } else {
        pParamWDevLock = (P_PARAM_WDEV_LOCK_THREAD_T)kalMemAlloc(
            sizeof(PARAM_WDEV_LOCK_THREAD_T), VIR_MEM_TYPE);
    }

    if (pParamWDevLock == NULL) {
        DBGLOG(REQ, ERROR, "pParamWDevLock Alloc Failed\n");
        return;
    }

    pParamWDevLock->pDev = pDev;
    pParamWDevLock->fn = fn;
    pParamWDevLock->pFrameBuf = pFrameBuf;
    pParamWDevLock->frameLen = frameLen;
    pParamWDevLock->req_ies = req_ies;
    pParamWDevLock->req_ies_len = req_ies_len;
    pParamWDevLock->fgIsInterruptContext = fgIsInterruptContext;
    if (pBss) {
        cfg80211_ref_bss(priv_to_wiphy(prGlueInfo), pBss);
        pParamWDevLock->pBss = pBss;
    } else {
        pParamWDevLock->pBss = NULL;
    }
    pParamWDevLock->uapsd_queues = uapsd_queues;

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_WDEV_LOCK);
    QUEUE_INSERT_TAIL(&prGlueInfo->prAdapter->rWDevLockQueue,
                      &pParamWDevLock->rQueEntry);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_WDEV_LOCK);

    if (!schedule_delayed_work(&wdev_lock_workq, 0)) {
        DBGLOG(REQ, INFO, "work is already in wdev_lock_workq\n");
    }
}

u8 kalGetIPv4Address(IN struct net_device *prDev, IN u32 u4MaxNumOfAddr,
                     OUT u8 *pucIpv4Addrs, OUT u32 *pu4NumOfIpv4Addr)
{
    u32 u4NumIPv4 = 0;
    u32 u4AddrLen = IPV4_ADDR_LEN;
    struct in_ifaddr *prIfa;

    /* 4 <1> Sanity check of netDevice */
    if (!prDev || !(prDev->ip_ptr) ||
        !((struct in_device *)(prDev->ip_ptr))->ifa_list) {
        DBGLOG(INIT, INFO,
               "IPv4 address is not available for dev(0x%p)\n", prDev);

        *pu4NumOfIpv4Addr = 0;
        return false;
    }

    prIfa = ((struct in_device *)(prDev->ip_ptr))->ifa_list;

    /* 4 <2> copy the IPv4 address */
    while ((u4NumIPv4 < u4MaxNumOfAddr) && prIfa) {
        kalMemCopy(&pucIpv4Addrs[u4NumIPv4 * u4AddrLen],
                   &prIfa->ifa_local, u4AddrLen);
        prIfa = prIfa->ifa_next;

        DBGLOG(INIT, INFO, "IPv4 addr [%u][" IPV4STR "]\n", u4NumIPv4,
               IPV4TOSTR(&pucIpv4Addrs[u4NumIPv4 * u4AddrLen]));

        u4NumIPv4++;
    }

    *pu4NumOfIpv4Addr = u4NumIPv4;

    return true;
}

#if IS_ENABLED(CONFIG_IPV6)
u8 kalGetIPv6Address(IN struct net_device *prDev, IN u32 u4MaxNumOfAddr,
                     OUT u8 *pucIpv6Addrs, OUT u32 *pu4NumOfIpv6Addr)
{
    u32 u4NumIPv6 = 0;
    u32 u4AddrLen = IPV6_ADDR_LEN;
    struct inet6_ifaddr *prIfa;

    /* 4 <1> Sanity check of netDevice */
    if (!prDev || !(prDev->ip6_ptr)) {
        DBGLOG(INIT, INFO,
               "IPv6 address is not available for dev(0x%p)\n", prDev);

        *pu4NumOfIpv6Addr = 0;
        return false;
    }

    /* 4 <2> copy the IPv6 address */
    LIST_FOR_EACH_IPV6_ADDR(prIfa, prDev->ip6_ptr) {
        kalMemCopy(&pucIpv6Addrs[u4NumIPv6 * u4AddrLen], &prIfa->addr,
                   u4AddrLen);

        DBGLOG(INIT, INFO, "IPv6 addr [%u][" IPV6STR "]\n", u4NumIPv6,
               IPV6TOSTR(&pucIpv6Addrs[u4NumIPv6 * u4AddrLen]));

        if ((u4NumIPv6 + 1) >= u4MaxNumOfAddr) {
            break;
        }
        u4NumIPv6++;
    }

    *pu4NumOfIpv6Addr = u4NumIPv6;

    return true;
}
#endif

void kalSetNetAddress(IN P_GLUE_INFO_T prGlueInfo, IN u8 ucBssIdx,
                      IN u8 *pucIPv4Addr, IN u32 u4NumIPv4Addr,
                      IN u8 *pucIPv6Addr, IN u32 u4NumIPv6Addr)
{
    WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
    u32 u4SetInfoLen = 0;
    u32 u4Len = OFFSET_OF(PARAM_NETWORK_ADDRESS_LIST, arAddress);
    P_PARAM_NETWORK_ADDRESS_LIST prParamNetAddrList;
    P_PARAM_NETWORK_ADDRESS prParamNetAddr;
    u32 i, u4AddrLen;

    DBGLOG(INIT, INFO, "Set network address for BSS[%u]\n", ucBssIdx);

    /* 4 <1> Calculate buffer size */
    /* IPv4 */
    u4Len += (((sizeof(PARAM_NETWORK_ADDRESS) - 1) + IPV4_ADDR_LEN) *
              u4NumIPv4Addr);
    /* IPv6 */
    u4Len += (((sizeof(PARAM_NETWORK_ADDRESS) - 1) + IPV6_ADDR_LEN) *
              u4NumIPv6Addr);

    /* 4 <2> Allocate buffer */
    prParamNetAddrList =
        (P_PARAM_NETWORK_ADDRESS_LIST)kalMemAlloc(u4Len, VIR_MEM_TYPE);

    if (!prParamNetAddrList) {
        DBGLOG(INIT,
               WARN,
               "Fail to alloc buffer for setting BSS[%u] network address!\n",
               ucBssIdx);
        return;
    }
    /* 4 <3> Fill up network address */
    prParamNetAddrList->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;
    prParamNetAddrList->u4AddressCount = 0;
    prParamNetAddrList->ucBssIdx = ucBssIdx;

    /* 4 <3.1> Fill up IPv4 address */
    u4AddrLen = IPV4_ADDR_LEN;
    prParamNetAddr = prParamNetAddrList->arAddress;
    for (i = 0; i < u4NumIPv4Addr; i++) {
        prParamNetAddr->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;
        prParamNetAddr->u2AddressLength = u4AddrLen;
        kalMemCopy(prParamNetAddr->aucAddress,
                   &pucIPv4Addr[i * u4AddrLen], u4AddrLen);

        prParamNetAddr =
            (P_PARAM_NETWORK_ADDRESS)((unsigned long)prParamNetAddr
                                      +
                                      (unsigned long)(u4AddrLen +
                                                      OFFSET_OF(
                                                          PARAM_NETWORK_ADDRESS,
                                                          aucAddress)));
    }
    prParamNetAddrList->u4AddressCount += u4NumIPv4Addr;

    /* 4 <3.2> Fill up IPv6 address */
    u4AddrLen = IPV6_ADDR_LEN;
    for (i = 0; i < u4NumIPv6Addr; i++) {
        prParamNetAddr->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;
        prParamNetAddr->u2AddressLength = u4AddrLen;
        kalMemCopy(prParamNetAddr->aucAddress,
                   &pucIPv6Addr[i * u4AddrLen], u4AddrLen);

        prParamNetAddr =
            (P_PARAM_NETWORK_ADDRESS)((unsigned long)prParamNetAddr
                                      +
                                      (unsigned long)(u4AddrLen +
                                                      OFFSET_OF(
                                                          PARAM_NETWORK_ADDRESS,
                                                          aucAddress)));
    }
    prParamNetAddrList->u4AddressCount += u4NumIPv6Addr;

    /* 4 <4> IOCTL to main_thread */
    rStatus = kalIoctl(prGlueInfo, wlanoidSetNetworkAddress,
                       (void *)prParamNetAddrList, u4Len, false, false,
                       true, &u4SetInfoLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "%s: Fail to set network address\n",
               __func__);
    }

    kalMemFree(prParamNetAddrList, VIR_MEM_TYPE, u4Len);
}

void kalSetNetAddressFromInterface(IN P_GLUE_INFO_T prGlueInfo,
                                   IN struct net_device *prDev,
                                   IN u8 fgSet)
{
    u32 u4NumIPv4, u4NumIPv6;
    u8 pucIPv4Addr[IPV4_ADDR_LEN * CFG_PF_ARP_NS_MAX_NUM],
       pucIPv6Addr[IPV6_ADDR_LEN * CFG_PF_ARP_NS_MAX_NUM];
    P_NETDEV_PRIVATE_GLUE_INFO prNetDevPrivate =
        (P_NETDEV_PRIVATE_GLUE_INFO)NULL;

    prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO)netdev_priv(prDev);

    if (prNetDevPrivate->prGlueInfo != prGlueInfo) {
        DBGLOG(REQ, WARN, "%s: unexpected prGlueInfo(0x%p)!\n",
               __func__, prNetDevPrivate->prGlueInfo);
    }

    u4NumIPv4 = 0;
    u4NumIPv6 = 0;

    if (fgSet) {
        kalGetIPv4Address(prDev, CFG_PF_ARP_NS_MAX_NUM, pucIPv4Addr,
                          &u4NumIPv4);
        kalGetIPv6Address(prDev, CFG_PF_ARP_NS_MAX_NUM, pucIPv6Addr,
                          &u4NumIPv6);
    }

    if (u4NumIPv4 + u4NumIPv6 > CFG_PF_ARP_NS_MAX_NUM) {
        if (u4NumIPv4 >= CFG_PF_ARP_NS_MAX_NUM) {
            u4NumIPv4 = CFG_PF_ARP_NS_MAX_NUM;
            u4NumIPv6 = 0;
        } else {
            u4NumIPv6 = CFG_PF_ARP_NS_MAX_NUM - u4NumIPv4;
        }
    }

    kalSetNetAddress(prGlueInfo, prNetDevPrivate->ucBssIdx, pucIPv4Addr,
                     u4NumIPv4, pucIPv6Addr, u4NumIPv6);
}

#if CFG_MET_PACKET_TRACE_SUPPORT

u8 kalMetCheckProfilingPacket(IN P_GLUE_INFO_T prGlueInfo,
                              IN P_NATIVE_PACKET prPacket)
{
    u32 u4PacketLen;
    u16 u2EtherTypeLen;
    struct sk_buff *prSkb = (struct sk_buff *)prPacket;
    u8 *aucLookAheadBuf = NULL;
    u8 ucEthTypeLenOffset = ETHER_HEADER_LEN - ETHER_TYPE_LEN;
    u8 *pucNextProtocol = NULL;

    u4PacketLen = prSkb->len;

    if (u4PacketLen < ETHER_HEADER_LEN) {
        DBGLOG(INIT, WARN, "Invalid Ether packet length: %lu\n",
               u4PacketLen);
        return false;
    }

    aucLookAheadBuf = prSkb->data;

    /* 4 <0> Obtain Ether Type/Len */
    WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ucEthTypeLenOffset],
                        &u2EtherTypeLen);

    /* 4 <1> Skip 802.1Q header (VLAN Tagging) */
    if (u2EtherTypeLen == ETH_P_VLAN) {
        ucEthTypeLenOffset += ETH_802_1Q_HEADER_LEN;
        WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ucEthTypeLenOffset],
                            &u2EtherTypeLen);
    }
    /* 4 <2> Obtain next protocol pointer */
    pucNextProtocol = &aucLookAheadBuf[ucEthTypeLenOffset + ETHER_TYPE_LEN];

    /* 4 <3> Handle ethernet format */
    switch (u2EtherTypeLen) {
    /* IPv4 */
    case ETH_P_IPV4: {
        u8 *pucIpHdr = pucNextProtocol;
        u8 ucIpVersion;

        /* IPv4 header length check */
        if (u4PacketLen <
            (ucEthTypeLenOffset + ETHER_TYPE_LEN + IPV4_HDR_LEN)) {
            DBGLOG(INIT, WARN, "Invalid IPv4 packet length: %lu\n",
                   u4PacketLen);
            return false;
        }

        /* IPv4 version check */
        ucIpVersion = (pucIpHdr[0] & IP_VERSION_MASK) >>
                      IP_VERSION_OFFSET;
        if (ucIpVersion != IP_VERSION_4) {
            DBGLOG(INIT, WARN, "Invalid IPv4 packet version: %u\n",
                   ucIpVersion);
            return false;
        }

        if (pucIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET] == IP_PROTOCOL_UDP) {
            u8 *pucUdpHdr = &pucIpHdr[IPV4_HDR_LEN];
            u16 u2UdpDstPort;
            u16 u2UdpSrcPort;

            /* Get UDP DST port */
            WLAN_GET_FIELD_BE16(&pucUdpHdr[UDP_HDR_DST_PORT_OFFSET],
                                &u2UdpDstPort);

            /* Get UDP SRC port */
            WLAN_GET_FIELD_BE16(&pucUdpHdr[UDP_HDR_SRC_PORT_OFFSET],
                                &u2UdpSrcPort);

            if (u2UdpSrcPort == prGlueInfo->u2MetUdpPort) {
                u16 u2IpId;

                /* Store IP ID for Tag */
                WLAN_GET_FIELD_BE16(
                    &pucIpHdr[
                        IPV4_HDR_IP_IDENTIFICATION_OFFSET],
                    &u2IpId);
                GLUE_SET_PKT_IP_ID(prPacket, u2IpId);

                return true;
            }
        }
    } break;

    default:
        break;
    }

    return false;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0))
static unsigned long __read_mostly tracing_mark_write_addr;

static int __mt_find_tracing_mark_write_symbol_fn(void *prData,
                                                  const char *pcNameBuf,
                                                  struct module *prModule,
                                                  unsigned long ulAddress)
{
    if (strcmp(pcNameBuf, "tracing_mark_write") == 0) {
        tracing_mark_write_addr = ulAddress;
        return 1;
    }
    return 0;
}
#endif

static inline void __mt_update_tracing_mark_write_addr(void)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0))
    if (unlikely(tracing_mark_write_addr == 0)) {
        kallsyms_on_each_symbol(__mt_find_tracing_mark_write_symbol_fn,
                                NULL);
    }
#endif
}

void kalMetTagPacket(IN P_GLUE_INFO_T prGlueInfo,
                     IN P_NATIVE_PACKET prPacket,
                     IN ENUM_TX_PROFILING_TAG_T eTag)
{
    if (!prGlueInfo->fgMetProfilingEn) {
        return;
    }

    switch (eTag) {
    case TX_PROF_TAG_OS_TO_DRV:
        if (kalMetCheckProfilingPacket(prGlueInfo, prPacket)) {
            /* trace_printk("S|%d|%s|%d\n", current->pid,
             * "WIFI-CHIP", GLUE_GET_PKT_IP_ID(prPacket)); */
            __mt_update_tracing_mark_write_addr();

            GLUE_SET_PKT_FLAG_PROF_MET(prPacket);
        }
        break;

    case TX_PROF_TAG_DRV_TX_DONE:
        if (GLUE_GET_PKT_IS_PROF_MET(prPacket)) {
            __mt_update_tracing_mark_write_addr();
        }
        break;

    case TX_PROF_TAG_MAC_TX_DONE:
        break;

    default:
        break;
    }
}

void kalMetInit(IN P_GLUE_INFO_T prGlueInfo)
{
    prGlueInfo->fgMetProfilingEn = false;
    prGlueInfo->u2MetUdpPort = 0;
}

static ssize_t kalMetCtrlWriteProcfs(struct file *file,
                                     const char __user *buffer,
                                     size_t count,
                                     loff_t *off)
{
    char acBuf[128 + 1];  /* + 1 for "\0" */
    u32 u4CopySize;
    int u8MetProfEnable;

    IN P_GLUE_INFO_T prGlueInfo;

    u4CopySize = (count < (sizeof(acBuf) - 1)) ? count :
                 (sizeof(acBuf) - 1);
    if (copy_from_user(acBuf, buffer, u4CopySize)) {
        DBGLOG(INIT, ERROR, "error of copy from user\n");
        return -EFAULT;
    }
    acBuf[u4CopySize] = '\0';

    if (sscanf(acBuf, " %d", &u8MetProfEnable) == 1) {
        DBGLOG(INIT, INFO, "MET_PROF: Write MET PROC Enable=%d\n",
               u8MetProfEnable);
    }
    if (pMetGlobalData != NULL) {
        prGlueInfo = (P_GLUE_INFO_T)pMetGlobalData;
        prGlueInfo->fgMetProfilingEn = (u8)u8MetProfEnable;
    }
    return count;
}

static ssize_t kalMetPortWriteProcfs(struct file *file,
                                     const char __user *buffer,
                                     size_t count,
                                     loff_t *off)
{
    char acBuf[128 + 1];  /* + 1 for "\0" */
    u32 u4CopySize;
    int u16MetUdpPort;

    IN P_GLUE_INFO_T prGlueInfo;

    u4CopySize = (count < (sizeof(acBuf) - 1)) ? count :
                 (sizeof(acBuf) - 1);
    if (copy_from_user(acBuf, buffer, u4CopySize)) {
        DBGLOG(INIT, ERROR, "error of copy from user\n");
        return -EFAULT;
    }
    acBuf[u4CopySize] = '\0';

    if (sscanf(acBuf, " %d", &u16MetUdpPort) == 1) {
        DBGLOG(INIT, INFO, "MET_PROF: Write MET PROC UDP_PORT=%d\n",
               u16MetUdpPort);
    }
    if (pMetGlobalData != NULL) {
        prGlueInfo = (P_GLUE_INFO_T)pMetGlobalData;
        prGlueInfo->u2MetUdpPort = (u16)u16MetUdpPort;
    }
    return count;
}

#if KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE
const struct proc_ops rMetProcCtrlFops = { .proc_write =
                                               kalMetCtrlWriteProcfs };

const struct proc_ops rMetProcPortFops = { .proc_write =
                                               kalMetPortWriteProcfs };
#else
const struct file_operations rMetProcCtrlFops = {
    .write = kalMetCtrlWriteProcfs
};

const struct file_operations rMetProcPortFops = {
    .write = kalMetPortWriteProcfs
};
#endif

int kalMetInitProcfs(IN P_GLUE_INFO_T prGlueInfo)
{
    /* struct proc_dir_entry *pMetProcDir; */
    if (init_net.proc_net == (struct proc_dir_entry *)NULL) {
        DBGLOG(INIT, INFO, "init proc fs fail: proc_net == NULL\n");
        return -ENOENT;
    }
    /*
     * Directory: Root (/proc/net/wlan0)
     * For dual driver design, we should setup wlan dynamically
     */
    pMetProcDir =
        proc_mkdir(prGlueInfo->prDevHandler->name, init_net.proc_net);

    if (pMetProcDir == NULL) {
        return -ENOENT;
    }

    /*
     *  /proc/net/wlan0
     *  |-- met_ctrl         (PROC_MET_PROF_CTRL)
     */
    /* proc_create(PROC_MET_PROF_CTRL, 0x0644, pMetProcDir, &rMetProcFops);
     */
    proc_create(PROC_MET_PROF_CTRL, 0, pMetProcDir, &rMetProcCtrlFops);
    proc_create(PROC_MET_PROF_PORT, 0, pMetProcDir, &rMetProcPortFops);

    pMetGlobalData = (void *)prGlueInfo;

    return 0;
}

int kalMetRemoveProcfs(IN P_GLUE_INFO_T prGlueInfo)
{
    if (init_net.proc_net == (struct proc_dir_entry *)NULL) {
        DBGLOG(INIT, WARN, "remove proc fs fail: proc_net == NULL\n");
        return -ENOENT;
    }

    if (pMetGlobalData == NULL) {
        DBGLOG(INIT, WARN,
               "Skip MET remove Procfs due to init was not done\n");
        return 0;
    }

    remove_proc_entry(PROC_MET_PROF_CTRL, pMetProcDir);
    remove_proc_entry(PROC_MET_PROF_PORT, pMetProcDir);
    /* remove root directory (proc/net/wlan0) */
    remove_proc_entry(prGlueInfo->prDevHandler->name, init_net.proc_net);

    /* clear MetGlobalData */
    pMetGlobalData = NULL;

    return 0;
}
#endif

u8 kalIndicateAgpsNotify(P_ADAPTER_T prAdapter, u8 cmd, u8 *data,
                         u16 dataLen)
{
#ifdef CONFIG_NL80211_TESTMODE
    P_GLUE_INFO_T prGlueInfo = prAdapter->prGlueInfo;
    struct sk_buff *skb = cfg80211_testmode_alloc_event_skb(
        priv_to_wiphy(prGlueInfo), dataLen, GFP_KERNEL);

    /* DBGLOG(CCX, INFO, ("WLAN_STATUS_AGPS_NOTIFY, cmd=%d\n", cmd)); */
    if (unlikely(nla_put(skb, MTK_ATTR_AGPS_CMD, sizeof(cmd), &cmd) < 0)) {
        goto nla_put_failure;
    }
    if (dataLen > 0 && data &&
        unlikely(nla_put(skb, MTK_ATTR_AGPS_DATA, dataLen, data) < 0)) {
        goto nla_put_failure;
    }
    if (unlikely(nla_put(skb, MTK_ATTR_AGPS_IFINDEX, sizeof(u32),
                         &prGlueInfo->prDevHandler->ifindex) < 0)) {
        goto nla_put_failure;
    }
    /* currently, the ifname maybe wlan0, p2p0, so the maximum name length
     * will be 5 bytes */
    if (unlikely(nla_put(skb, MTK_ATTR_AGPS_IFNAME, 5,
                         prGlueInfo->prDevHandler->name) < 0)) {
        goto nla_put_failure;
    }

    cfg80211_testmode_event(skb, GFP_KERNEL);
    return true;

nla_put_failure:
    kfree_skb(skb);
    return false;

#else
    return false;

#endif
}

u64 kalGetBootTime(void)
{
#if KERNEL_VERSION(4, 20, 0) <= LINUX_VERSION_CODE
    struct timespec64 ts;
#else
    struct timespec ts;
#endif
    u64 bootTime = 0;

#if KERNEL_VERSION(4, 20, 0) <= LINUX_VERSION_CODE
    ktime_get_boottime_ts64(&ts);
#elif KERNEL_VERSION(2, 6, 39) <= LINUX_VERSION_CODE
    get_monotonic_boottime(&ts);
#else
    ts = ktime_to_timespec(ktime_get());
#endif

    bootTime = ts.tv_sec;
    bootTime *= USEC_PER_SEC;
    bootTime += ts.tv_nsec / NSEC_PER_USEC;
    return bootTime;
}

#if CFG_ASSERT_DUMP
WLAN_STATUS kalOpenCorDumpFile(u8 fgIsN9)
{
    /* Move open-op to kalWriteCorDumpFile(). Empty files only */
    u32 ret;
    u8 *apucFileName;

    if (fgIsN9) {
        apucFileName = apucCorDumpN9FileName;
    }else{
        apucFileName = apucCorDumpCr4FileName;
    }

    ret = kalTrunkPath(apucFileName);

    return (ret >= 0) ? WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE;
}

WLAN_STATUS kalWriteCorDumpFile(u8 *pucBuffer, u16 u2Size, u8 fgIsN9)
{
    u32 ret;
    u8 *apucFileName;

    DBGLOG(INIT, WARN, "kalWriteCorDumpFile undo...\n");
    return WLAN_STATUS_SUCCESS;

    if (fgIsN9) {
        apucFileName = apucCorDumpN9FileName;
    }else{
        apucFileName = apucCorDumpCr4FileName;
    }

    ret = kalWriteToFile(apucFileName, true, pucBuffer, u2Size);

    return (ret >= 0) ? WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE;
}

WLAN_STATUS kalCloseCorDumpFile(u8 fgIsN9)
{
    /* Move close-op to kalWriteCorDumpFile(). Do nothing here */

    return WLAN_STATUS_SUCCESS;
}
#endif

#if CFG_WOW_SUPPORT
void kalWowInit(IN P_GLUE_INFO_T prGlueInfo)
{
    kalMemZero(&prGlueInfo->prAdapter->rWowCtrl.stWowPort,
               sizeof(WOW_PORT_T));
    prGlueInfo->prAdapter->rWowCtrl.ucReason = INVALID_WOW_WAKE_UP_REASON;
}

void kalWowCmdEventSetCb(IN P_ADAPTER_T prAdapter,
                         IN P_CMD_INFO_T prCmdInfo,
                         IN u8 *pucEventBuf, IN u32 u4EventBufLen)
{
    ASSERT(prAdapter);
    ASSERT(prCmdInfo);

    if (prCmdInfo->ucCID == CMD_ID_SET_PF_CAPABILITY) {
        DBGLOG(INIT, STATE, "CMD_ID_SET_PF_CAPABILITY cmd submitted\n");
        prAdapter->fgSetPfCapabilityDone = true;
    }

    if (prCmdInfo->ucCID == CMD_ID_SET_WOWLAN) {
        DBGLOG(INIT, STATE, "CMD_ID_SET_WOWLAN cmd submitted\n");
        prAdapter->fgSetWowDone = true;
    }
}

void kalWowProcess(IN P_GLUE_INFO_T prGlueInfo, u8 enable)
{
    CMD_WOWLAN_PARAM_T rCmdWowlanParam;
    CMD_PACKET_FILTER_CAP_T rCmdPacket_Filter_Cap;
    CMD_FW_LOG_2_HOST_CTRL_T rFwLog2HostCtrl;
    P_WOW_CTRL_T pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 ii, u4BufLen, wait = 0;

    kalMemZero(&rCmdWowlanParam, sizeof(CMD_WOWLAN_PARAM_T));
    kalMemZero(&rFwLog2HostCtrl, sizeof(rFwLog2HostCtrl));
    kalMemZero(&rCmdPacket_Filter_Cap, sizeof(CMD_PACKET_FILTER_CAP_T));

    prGlueInfo->prAdapter->fgSetPfCapabilityDone = false;
    prGlueInfo->prAdapter->fgSetWowDone = false;

    DBGLOG(PF, INFO,
           "PF, pAd AIS ucBssIndex=%d, ucOwnMacIndex=%d, band=%d\n",
           prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex,
           prGlueInfo->prAdapter->prAisBssInfo->ucOwnMacIndex,
           prGlueInfo->prAdapter->prAisBssInfo->eDBDCBand);

    DBGLOG(PF, INFO, "profile wow=%d, GpioInterval=%d\n",
           prGlueInfo->prAdapter->rWifiVar.ucWow,
           prGlueInfo->prAdapter->rWowCtrl.astWakeHif[0].u4GpioInterval);

    /* Kernel log timestamp correction */
    if (prGlueInfo->prAdapter->rWifiVar.ucN9Log2HostCtrl > 0) {
        rFwLog2HostCtrl.ucMcuDest = 0;
        rFwLog2HostCtrl.ucFwLog2HostCtrl =
            prGlueInfo->prAdapter->rWifiVar.ucN9Log2HostCtrl;

        rStatus = kalIoctl(prGlueInfo, wlanoidSetFwLog2Host,
                           &rFwLog2HostCtrl,
                           sizeof(CMD_FW_LOG_2_HOST_CTRL_T), true, true,
                           true, &u4BufLen);
    }

    /* add band info */
    rCmdWowlanParam.ucDbdcBand =
        (u8)prGlueInfo->prAdapter->prAisBssInfo->eDBDCBand;

    rCmdPacket_Filter_Cap.packet_cap_type |= PACKETF_CAP_TYPE_MAGIC;
    /* 20160627 Bennett: if receive BMC magic, PF search by bssid index,
     * which is different with OM index */
    /* After discussion, enable all bssid bits */
    /* rCmdPacket_Filter_Cap.ucBssid |=
     * BIT(prGlueInfo->prAdapter->prAisBssInfo->ucOwnMacIndex); */
    rCmdPacket_Filter_Cap.ucBssid |= BITS(0, 3);

    if (enable) {
        rCmdPacket_Filter_Cap.usEnableBits |= PACKETF_CAP_TYPE_MAGIC;
    }else{
        rCmdPacket_Filter_Cap.usEnableBits &= ~PACKETF_CAP_TYPE_MAGIC;
    }

    rStatus = wlanSendSetQueryCmd(prGlueInfo->prAdapter,
                                  CMD_ID_SET_PF_CAPABILITY, true, false,
                                  false, kalWowCmdEventSetCb,
                                  nicOidCmdTimeoutCommon,
                                  sizeof(CMD_PACKET_FILTER_CAP_T),
                                  (u8 *)&rCmdPacket_Filter_Cap, NULL, 0);

    /* ARP and DHCP offload */
    wlanSetSuspendMode(prGlueInfo, enable);
    /* p2pSetSuspendMode(prGlueInfo, true); */

    /* wake up reason reset to default only when enter wow mode */
    if (enable) {
        pWOW_CTRL->ucReason = INVALID_WOW_WAKE_UP_REASON;
    }
    /* Let WOW enable/disable as last command, so we can back/restore DMA
     * classify filter in FW */
    rCmdWowlanParam.ucScenarioID = pWOW_CTRL->ucScenarioId;
    rCmdWowlanParam.ucBlockCount = pWOW_CTRL->ucBlockCount;
    kalMemCopy(&rCmdWowlanParam.astWakeHif[0], &pWOW_CTRL->astWakeHif[0],
               sizeof(WOW_WAKE_HIF_T));

    /* copy UDP/TCP port setting */
    kalMemCopy(&rCmdWowlanParam.stWowPort,
               &prGlueInfo->prAdapter->rWowCtrl.stWowPort,
               sizeof(WOW_PORT_T));

    DBGLOG(PF, INFO,
           "Cmd: IPV4/UDP=%d, IPV4/TCP=%d, IPV6/UDP=%d, IPV6/TCP=%d\n",
           rCmdWowlanParam.stWowPort.ucIPv4UdpPortCnt,
           rCmdWowlanParam.stWowPort.ucIPv4TcpPortCnt,
           rCmdWowlanParam.stWowPort.ucIPv6UdpPortCnt,
           rCmdWowlanParam.stWowPort.ucIPv6TcpPortCnt);

    for (ii = 0; ii < rCmdWowlanParam.stWowPort.ucIPv4UdpPortCnt; ii++)
        DBGLOG(PF, INFO, "IPV4/UDP port[%d]=%d\n", ii,
               rCmdWowlanParam.stWowPort.ausIPv4UdpPort[ii]);

    for (ii = 0; ii < rCmdWowlanParam.stWowPort.ucIPv4TcpPortCnt; ii++)
        DBGLOG(PF, INFO, "IPV4/TCP port[%d]=%d\n", ii,
               rCmdWowlanParam.stWowPort.ausIPv4TcpPort[ii]);

    for (ii = 0; ii < rCmdWowlanParam.stWowPort.ucIPv6UdpPortCnt; ii++)
        DBGLOG(PF, INFO, "IPV6/UDP port[%d]=%d\n", ii,
               rCmdWowlanParam.stWowPort.ausIPv6UdpPort[ii]);

    for (ii = 0; ii < rCmdWowlanParam.stWowPort.ucIPv6TcpPortCnt; ii++)
        DBGLOG(PF, INFO, "IPV6/TCP port[%d]=%d\n", ii,
               rCmdWowlanParam.stWowPort.ausIPv6TcpPort[ii]);

    /* GPIO parameter is necessary in suspend/resume */
    if (enable == 1) {
        rCmdWowlanParam.ucCmd = PM_WOWLAN_REQ_START;
        rCmdWowlanParam.ucDetectType =
            WOWLAN_DETECT_TYPE_MAGIC |
            WOWLAN_DETECT_TYPE_ONLY_PHONE_SUSPEND;
        rCmdWowlanParam.u2FilterFlag =
            WOWLAN_FF_DROP_ALL | WOWLAN_FF_SEND_MAGIC_TO_HOST |
            WOWLAN_FF_ALLOW_1X | WOWLAN_FF_ALLOW_ARP_REQ2ME;
    } else {
        rCmdWowlanParam.ucCmd = PM_WOWLAN_REQ_STOP;
    }

    rStatus = wlanSendSetQueryCmd(prGlueInfo->prAdapter, CMD_ID_SET_WOWLAN,
                                  true, false, false, kalWowCmdEventSetCb,
                                  nicOidCmdTimeoutCommon,
                                  sizeof(CMD_WOWLAN_PARAM_T),
                                  (u8 *)&rCmdWowlanParam, NULL, 0);

    while (1) {
        kalMsleep(5);

        if (wait > 100) {
            DBGLOG(INIT, ERROR, "WoW process timeout\n\n");
            break;
        }
        if ((prGlueInfo->prAdapter->fgSetPfCapabilityDone == true) &&
            (prGlueInfo->prAdapter->fgSetWowDone == true)) {
            DBGLOG(INIT, STATE, "WoW process done\n\n");
            break;
        }
        wait++;
    }
}
#endif

void kalFreeTxMsduWorker(struct work_struct *work)
{
    P_GLUE_INFO_T prGlueInfo;
    P_ADAPTER_T prAdapter;
    QUE_T rTmpQue;
    P_QUE_T prTmpQue = &rTmpQue;
    P_MSDU_INFO_T prMsduInfo;

    if (g_u4HaltFlag) {
        return;
    }

    prGlueInfo = ENTRY_OF(work, GLUE_INFO_T, rTxMsduFreeWork);
    prAdapter = prGlueInfo->prAdapter;

    if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
        return;
    }

    KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);
    QUEUE_MOVE_ALL(prTmpQue, &prAdapter->rTxDataDoneQueue);
    KAL_RELEASE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);

    while (QUEUE_IS_NOT_EMPTY(prTmpQue)) {
        QUEUE_REMOVE_HEAD(prTmpQue, prMsduInfo, P_MSDU_INFO_T);

        nicTxFreePacket(prAdapter, prMsduInfo, false);
        nicTxReturnMsduInfo(prAdapter, prMsduInfo);
    }
}

void kalFreeTxMsdu(P_ADAPTER_T prAdapter, P_MSDU_INFO_T prMsduInfo)
{
    KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);
    QUEUE_INSERT_TAIL(&prAdapter->rTxDataDoneQueue,
                      (P_QUE_ENTRY_T)prMsduInfo);
    KAL_RELEASE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);

    schedule_work(&prAdapter->prGlueInfo->rTxMsduFreeWork);
}

#if CFG_SUPPORT_DFS
void kalIndicateChannelSwitch(IN P_GLUE_INFO_T prGlueInfo,
                              IN ENUM_CHNL_EXT_T eSco, IN u8 ucChannelNum)
{
    struct cfg80211_chan_def chandef;
    struct ieee80211_channel *prChannel = NULL;
    enum nl80211_channel_type rChannelType;

    if (ucChannelNum <= 14) {
        prChannel = ieee80211_get_channel(
            priv_to_wiphy(prGlueInfo),
            ieee80211_channel_to_frequency(ucChannelNum,
                                           NL80211_BAND_2GHZ));
    } else {
        prChannel = ieee80211_get_channel(
            priv_to_wiphy(prGlueInfo),
            ieee80211_channel_to_frequency(ucChannelNum,
                                           NL80211_BAND_5GHZ));
    }

    if (!prChannel) {
        DBGLOG(REQ, ERROR, "ieee80211_get_channel fail!\n");
        return;
    }

    switch (eSco) {
    case CHNL_EXT_SCN:
        rChannelType = NL80211_CHAN_NO_HT;
        break;

    case CHNL_EXT_SCA:
        rChannelType = NL80211_CHAN_HT40MINUS;
        break;

    case CHNL_EXT_SCB:
        rChannelType = NL80211_CHAN_HT40PLUS;
        break;

    case CHNL_EXT_RES:
    default:
        rChannelType = NL80211_CHAN_HT20;
        break;
    }

    DBGLOG(REQ, STATE, "DFS channel switch to %d\n", ucChannelNum);

    cfg80211_chandef_create(&chandef, prChannel, rChannelType);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 2))
    cfg80211_ch_switch_notify(prGlueInfo->prDevHandler, &chandef);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0))
    cfg80211_ch_switch_notify(prGlueInfo->prDevHandler, &chandef, 0);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(6, 9, 0))
    cfg80211_ch_switch_notify(prGlueInfo->prDevHandler, &chandef, 0, 0);
#else
    cfg80211_ch_switch_notify(prGlueInfo->prDevHandler, &chandef, 0);
#endif
}
#endif

int kal_sched_set(struct task_struct *p, int policy,
                  const struct sched_param *param, int nice)
{
    if (p == NULL || &(p->static_prio) < (int *)p) {
        return -1;
    }

#if KERNEL_VERSION(5, 9, 0) <= LINUX_VERSION_CODE
    struct sched_attr attr = {
        .sched_policy = policy,
        .sched_priority = param->sched_priority,
        .sched_nice = nice,
    };

    if (policy == SCHED_NORMAL) {
        sched_set_normal(p, nice);
    }else if (policy == SCHED_FIFO) {
        sched_set_fifo(p);
    }else{
        sched_set_fifo_low(p);
    }

    sched_setattr_nocheck(p, &attr);
#else
    sched_setscheduler(p, policy, param);
#endif

    return 0;
}

WLAN_STATUS kalUpdateBssChannel(IN P_GLUE_INFO_T prGlueInfo,
                                IN u8 aucSSID[],
                                IN u8 ucSsidLength, IN u8 aucBSSID[],
                                IN u8 ucChannelNum)
{
    WLAN_STATUS rStatus = WLAN_STATUS_NOT_ACCEPTED;
    struct cfg80211_bss *bss = NULL;
    struct ieee80211_channel *prChannel = NULL;

    if (ucChannelNum <= 14) {
        prChannel = ieee80211_get_channel(
            priv_to_wiphy(prGlueInfo),
            ieee80211_channel_to_frequency(ucChannelNum,
                                           NL80211_BAND_2GHZ));
    } else {
        prChannel = ieee80211_get_channel(
            priv_to_wiphy(prGlueInfo),
            ieee80211_channel_to_frequency(ucChannelNum,
                                           NL80211_BAND_5GHZ));
    }

    bss = cfg80211_get_bss(priv_to_wiphy(prGlueInfo), NULL,  /* channel */
                           aucBSSID, aucSSID,  /* ssid */
                           (u32)ucSsidLength,  /* ssid length */
                           IEEE80211_BSS_TYPE_ESS, IEEE80211_PRIVACY_ANY);

    if (bss) {
        if (prChannel) {
            bss->channel = prChannel;
            rStatus = WLAN_STATUS_SUCCESS;
        }
        cfg80211_put_bss(priv_to_wiphy(prGlueInfo), bss);
    }

    return rStatus;
}
