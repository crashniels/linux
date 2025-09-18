// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "platform.c"
 *    \brief  This file including the protocol layer privacy function.
 *
 *    This file provided the macros and functions library support for the
 *    protocol layer security setting from wlan_oid.c and for parse.c and
 *    rsn.c and nic_privacy.c
 *
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/uaccess.h>
#include "precomp.h"
#include "gl_os.h"

#if CFG_ENABLE_EARLY_SUSPEND
#include <linux/earlysuspend.h>
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define WIFI_NVRAM_FILE_NAME      "/lib/firmware/WIFI"
#define WIFI_NVRAM_CUSTOM_NAME    "/data/nvram/APCFG/APRDEB/WIFI_CUSTOM"

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
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static int netdev_event(struct notifier_block *nb, unsigned long notification,
                        void *ptr)
{
    struct in_ifaddr *ifa = (struct in_ifaddr *)ptr;
    struct net_device *prDev = ifa->ifa_dev->dev;
    P_GLUE_INFO_T prGlueInfo = NULL;

    if (prDev == NULL) {
        /* DBGLOG(REQ, INFO, ("netdev_event: device is empty.\n")); */
        return NOTIFY_DONE;
    }

    if ((strncmp(prDev->name, "p2p", 3) != 0) &&
        (strncmp(prDev->name, "wlan", 4) != 0)) {
        /* DBGLOG(REQ, INFO, ("netdev_event: xxx\n")); */
        return NOTIFY_DONE;
    }

    if ((prDev != gPrDev) && (prDev != gPrP2pDev[0]) &&
        (prDev != gPrP2pDev[1])) {
        /* DBGLOG(REQ, INFO, ("netdev_event: device is not mine.\n"));
         */
        return NOTIFY_DONE;
    }

    prGlueInfo = *((P_GLUE_INFO_T *)netdev_priv(prDev));
    if (prGlueInfo == NULL) {
        DBGLOG(REQ, INFO, "netdev_event: prGlueInfo is empty.\n");
        return NOTIFY_DONE;
    }

#if CFG_GARP_KEEPALIVE
    // garp keepalive needs IP address update when host is awake
#else
    if (prGlueInfo->fgIsInSuspendMode == false) {
        /* DBGLOG(REQ, INFO,
         *  ("netdev_event: PARAM_MEDIA_STATE_DISCONNECTED. (%d)\n",
         * prGlueInfo->eParamMediaStateIndicated));
         */
        return NOTIFY_DONE;
    }
#endif  // CFG_GARP_KEEPALIVE

    kalSetNetAddressFromInterface(prGlueInfo, prDev, true);

    return NOTIFY_DONE;
}

static struct notifier_block inetaddr_notifier = {
    .notifier_call = netdev_event,
};

void wlanRegisterNotifier(void)
{
#if CFG_ENABLE_NET_DEV_NOTIFY
    register_inetaddr_notifier(&inetaddr_notifier);
#endif
}

void wlanUnregisterNotifier(void)
{
#if CFG_ENABLE_NET_DEV_NOTIFY
    unregister_inetaddr_notifier(&inetaddr_notifier);
#endif
}

#if CFG_ENABLE_EARLY_SUSPEND
/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will register platform driver to os
 *
 * \param[in] wlanSuspend    Function pointer to platform suspend function
 * \param[in] wlanResume   Function pointer to platform resume   function
 *
 * \return The result of registering earlysuspend
 */
/*----------------------------------------------------------------------------*/

int glRegisterEarlySuspend(struct early_suspend *prDesc,
                           early_suspend_callback wlanSuspend,
                           late_resume_callback wlanResume)
{
    int ret = 0;

    if (wlanSuspend != NULL) {
        prDesc->suspend = wlanSuspend;
    } else {
        DBGLOG(REQ, INFO,
               "glRegisterEarlySuspend wlanSuspend ERROR.\n");
        ret = -1;
    }

    if (wlanResume != NULL) {
        prDesc->resume = wlanResume;
    } else {
        DBGLOG(REQ, INFO, "glRegisterEarlySuspend wlanResume ERROR.\n");
        ret = -1;
    }

    register_early_suspend(prDesc);
    return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will un-register platform driver to os
 *
 * \return The result of un-registering earlysuspend
 */
/*----------------------------------------------------------------------------*/

int glUnregisterEarlySuspend(struct early_suspend *prDesc)
{
    int ret = 0;

    unregister_early_suspend(prDesc);

    prDesc->suspend = NULL;
    prDesc->resume = NULL;

    return ret;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Utility function for reading data from files on NVRAM-FS
 *
 * \param[in]
 *           filename
 *           len
 *           offset
 * \param[out]
 *           buf
 * \return
 *           actual length of data being read
 */
/*----------------------------------------------------------------------------*/
static int nvram_read(char *filename, char *buf, ssize_t len, int offset)
{
#if CFG_SUPPORT_NVRAM
    struct file *fd;
    int retLen = -1;
    loff_t pos = (loff_t) offset;

#ifdef set_fs
    mm_segment_t old_fs = get_fs();

    set_fs(KERNEL_DS);
#endif

    fd = filp_open(filename, O_RDONLY, 0);

    if (IS_ERR(fd)) {
        DBGLOG(INIT, INFO, "[nvram_read] : failed to open!!\n");
#ifdef set_fs
        set_fs(old_fs);
#endif
        return -1;
    }

    do {
        /*if ((fd->f_op == NULL) || ((fd->f_op->read == NULL) && (fd->f_op->read_iter == NULL))) {
            DBGLOG(INIT, INFO,
                   "[nvram_read] : file can not be read!!\n");
            break;
        }

        if (fd->f_pos != offset) {
            if (fd->f_op->llseek) {
                if (fd->f_op->llseek(fd, offset, 0) != offset) {
                    DBGLOG(INIT,
                           INFO,
                           "[nvram_read] : failed to seek!!\n");
                    break;
                }
            } else {
                fd->f_pos = offset;
            }
        }*/

        retLen = kernel_read(fd, buf, len, &pos /*&fd->f_pos*/); //fd->f_op->read != NULL ? fd->f_op->read(fd, buf, len, &fd->f_pos) 
		//: kernel_read(fd, buf, len, &fd->f_pos) ;
    } while (false);

    filp_close(fd, NULL);

#ifdef set_fs
    set_fs(old_fs);
#endif

    return retLen;

#else  /* !CFG_SUPPORT_NVRAM */
    return -EIO;

#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Utility function for writing data to files on NVRAM-FS
 *
 * \param[in]
 *           filename
 *           buf
 *           len
 *           offset
 * \return
 *           actual length of data being written
 */
/*----------------------------------------------------------------------------*/
static int nvram_write(char *filename, char *buf, ssize_t len, int offset)
{
#if CFG_SUPPORT_NVRAM
    struct file *fd;
    int retLen = -1;

#ifdef set_fs
    mm_segment_t old_fs = get_fs();

    set_fs(KERNEL_DS);
#endif

    fd = filp_open(filename, O_WRONLY | O_CREAT, 0644);

    if (IS_ERR(fd)) {
        DBGLOG(INIT, INFO, "[nvram_write] : failed to open!!\n");
#ifdef set_fs
        set_fs(old_fs);
#endif
        return -1;
    }

    do {
        if ((fd->f_op == NULL) || (fd->f_op->write == NULL)) {
            DBGLOG(INIT, INFO,
                   "[nvram_write] : file can not be write!!\n");
            break;
        }
        /* End of if */
        if (fd->f_pos != offset) {
            if (fd->f_op->llseek) {
                if (fd->f_op->llseek(fd, offset, 0) != offset) {
                    DBGLOG(INIT,
                           INFO,
                           "[nvram_write] : failed to seek!!\n");
                    break;
                }
            } else {
                fd->f_pos = offset;
            }
        }

        retLen = fd->f_op->write(fd, buf, len, &fd->f_pos);
    } while (false);

    filp_close(fd, NULL);

#ifdef set_fs
    set_fs(old_fs);
#endif

    return retLen;

#else  /* !CFG_SUPPORT_NVRAMS */
    return -EIO;

#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief API for reading data on NVRAM
 *
 * \param[in]
 *           prGlueInfo
 *           u4Offset
 * \param[out]
 *           pu2Data
 * \return
 *           true
 *           false
 */
/*----------------------------------------------------------------------------*/
u8 kalCfgDataRead16(IN P_GLUE_INFO_T prGlueInfo, IN u32 u4Offset,
                    OUT u16 *pu2Data)
{
    if (pu2Data == NULL) {
        return false;
    }

    if (nvram_read(WIFI_NVRAM_FILE_NAME, (char *)pu2Data,
                   sizeof(unsigned short),
                   u4Offset) != sizeof(unsigned short)) {
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief API for writing data on NVRAM
 *
 * \param[in]
 *           prGlueInfo
 *           u4Offset
 *           u2Data
 * \return
 *           true
 *           false
 */
/*----------------------------------------------------------------------------*/
u8 kalCfgDataWrite16(IN P_GLUE_INFO_T prGlueInfo, u32 u4Offset, u16 u2Data)
{
    if (nvram_write(WIFI_NVRAM_FILE_NAME, (char *)&u2Data,
                    sizeof(unsigned short),
                    u4Offset) != sizeof(unsigned short)) {
        return false;
    } else {
        return true;
    }
}
