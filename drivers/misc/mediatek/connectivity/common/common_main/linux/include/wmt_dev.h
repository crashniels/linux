/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef _WMT_DEV_H_
#define _WMT_DEV_H_

#include "osal.h"

#define STP_UART_FULL 0x01
#define STP_UART_MAND 0x02
#define STP_BTIF_FULL 0x03
#define STP_SDIO      0x04

#define CFG_WMT_PROC_FOR_AEE 1
#define CFG_WMT_PROC_FOR_DUMP_INFO 1

VOID wmt_dev_rx_event_cb(VOID);
INT32 wmt_dev_rx_timeout(P_OSAL_EVENT pEvent);
INT32 wmt_dev_patch_get(PUINT8 pPatchName, osal_firmware **ppPatch);
INT32 wmt_dev_patch_put(osal_firmware **ppPatch);
MTK_WCN_BOOL wmt_dev_is_file_exist(PUINT8 pFileName);
VOID wmt_dev_patch_info_free(VOID);
VOID wmt_dev_send_cmd_to_daemon(UINT32 cmd);
MTK_WCN_BOOL wmt_dev_get_early_suspend_state(VOID);
INT32 wmt_lpbk_handler(UINT32 on_off_flag, UINT32 retry);
VOID wmt_dev_blank_handler(VOID);
UINT32 wmt_dev_get_blank_state(VOID);
INT32 wmt_dev_apo_ctrl(UINT32 enable);
VOID wmt_dev_set_temp_threshold(INT32 val);
UINT8 wmt_dev_is_close(VOID);
extern LONG wmt_dev_tm_temp_query(VOID);
extern INT32 wmt_dev_tra_bitf_update(VOID); 
extern INT32 wmt_dev_tra_uart_update(VOID);

INT32 mtk_wcn_common_drv_init(VOID);
VOID mtk_wcn_common_drv_exit(VOID);

ssize_t WMT_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
ssize_t WMT_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
UINT32 WMT_poll(struct file *filp, poll_table *wait);
LONG WMT_unlocked_ioctl(struct file *filp, UINT32 cmd, ULONG arg);
LONG WMT_compat_ioctl(struct file *filp, UINT32 cmd, ULONG arg);

VOID wmt_dev_bgw_desense_init(VOID);
VOID wmt_dev_bgw_desense_deinit(VOID);

#endif /*_WMT_DEV_H_*/
