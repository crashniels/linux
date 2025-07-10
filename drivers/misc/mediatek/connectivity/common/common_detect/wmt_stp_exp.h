/*
* Copyright (C) 2011-2014 MediaTek Inc.
* 
* This program is free software: you can redistribute it and/or modify it under the terms of the 
* GNU General Public License version 2 as published by the Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _WMT_STP_EXP_H_
#define _WMT_STP_EXP_H_
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/firmware.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <mt-plat/mtk_wcn_cmb_stub.h>
#include "osal_typedef.h"
#include "stp_exp.h"
#include "wmt_exp.h"

#ifndef MTK_WCN_CMB_FOR_SDIO_1V_AUTOK
#define MTK_WCN_CMB_FOR_SDIO_1V_AUTOK 0
#endif

#ifdef MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT
#ifdef WMT_IDC_SUPPORT
#if (WMT_IDC_SUPPORT)
#define CFG_WMT_LTE_COEX_HANDLING 1
#define CFG_WMT_LTE_ENABLE_MSGID_MAPPING 0
#else
#define CFG_WMT_LTE_COEX_HANDLING 0
#endif
#endif

#define BT_TASK_INDX        (0)
#define FM_TASK_INDX        (1)
#define GPS_TASK_INDX       (2)
#define WIFI_TASK_INDX      (3)
#define WMT_TASK_INDX       (4)
#define STP_TASK_INDX       (5)
#define INFO_TASK_INDX      (6)
#define ANT_TASK_INDX       (7)
#if CFG_WMT_LTE_COEX_HANDLING
#define COEX_TASK_INDX		(8)
#define MTKSTP_MAX_TASK_NUM (9)
#else
#define MTKSTP_MAX_TASK_NUM	(8)
#endif

#define MTKSTP_BUFFER_SIZE  (16384) 


typedef void (*MTK_WCN_STP_EVENT_CB)(void);
typedef INT32 (*MTK_WCN_STP_IF_TX)(const PUINT8 data, const UINT32 size, PUINT32 written_size);
typedef void (*MTK_WCN_STP_IF_RX)(const PUINT8 data, INT32 size);

/*typedef enum {
    STP_UART_IF_TX = 0,
    STP_SDIO_IF_TX = 1,
    STP_BTIF_IF_TX = 2,
    STP_MAX_IF_TX
}ENUM_STP_TX_IF_TYPE;*/



typedef INT32 (*MTK_WCN_STP_SEND_DATA) (const PUINT8 buffer, const UINT32 length, const UINT8 type);
typedef INT32 (*MTK_WCN_STP_PARSER_DATA)(PUINT8 buffer, UINT32 length);
typedef INT32 (*MTK_WCN_STP_RECV_DATA)(PUINT8 buffer, UINT32 length, UINT8 type);
typedef MTK_WCN_BOOL (*MTK_WCN_STP_IS_RXQ_EMPTY)(UINT8 type);
typedef MTK_WCN_BOOL (*MTK_WCN_STP_IS_RDY)(VOID);
typedef VOID (*MTK_WCN_STP_SET_BLUEZ)(MTK_WCN_BOOL flags);
typedef INT32 (*MTK_WCN_STP_REG_IF_TX)(ENUM_STP_TX_IF_TYPE stp_if, MTK_WCN_STP_IF_TX func);
typedef INT32 (*MTK_WCN_STP_REG_IF_RX)(MTK_WCN_STP_IF_RX func);
typedef INT32 (*MTK_WCN_STP_REG_EVENT_CB)(INT32 type, MTK_WCN_STP_EVENT_CB func);
typedef INT32 (*MTK_WCN_STP_RGE_TX_EVENT_CB)(INT32 type, MTK_WCN_STP_EVENT_CB func);
typedef INT32 (*MTK_WCN_STP_COREDUMP_START_GET)(VOID);

typedef struct _MTK_WCN_STP_EXP_CB_INFO_{
	MTK_WCN_STP_SEND_DATA stp_send_data_cb;
	MTK_WCN_STP_SEND_DATA stp_send_data_raw_cb;
	MTK_WCN_STP_PARSER_DATA stp_parser_data_cb;
	MTK_WCN_STP_RECV_DATA stp_receive_data_cb;
	MTK_WCN_STP_IS_RXQ_EMPTY stp_is_rxqueue_empty_cb;
	MTK_WCN_STP_IS_RDY stp_is_ready_cb;
	MTK_WCN_STP_SET_BLUEZ stp_set_bluez_cb;
	MTK_WCN_STP_REG_IF_TX stp_if_tx_cb;
	MTK_WCN_STP_REG_IF_RX stp_if_rx_cb;
	MTK_WCN_STP_REG_EVENT_CB stp_reg_event_cb;
	MTK_WCN_STP_RGE_TX_EVENT_CB stp_reg_tx_event_cb;
	MTK_WCN_STP_COREDUMP_START_GET stp_coredump_start_get_cb;
}MTK_WCN_STP_EXP_CB_INFO,*P_MTK_WCN_STP_EXP_CB_INFO;



/*typedef enum _ENUM_WMTDRV_TYPE_T {
    WMTDRV_TYPE_BT = 0,
    WMTDRV_TYPE_FM = 1,
    WMTDRV_TYPE_GPS = 2,
    WMTDRV_TYPE_WIFI = 3,
    WMTDRV_TYPE_WMT = 4,
    WMTDRV_TYPE_ANT = 5,
    WMTDRV_TYPE_STP = 6,
    WMTDRV_TYPE_SDIO1 = 7,
    WMTDRV_TYPE_SDIO2 = 8,
    WMTDRV_TYPE_LPBK = 9,
    WMTDRV_TYPE_COREDUMP = 10,
#if MTK_WCN_CMB_FOR_SDIO_1V_AUTOK
    WMTDRV_TYPE_AUTOK = 11,
#endif
    WMTDRV_TYPE_MAX
} ENUM_WMTDRV_TYPE_T, *P_ENUM_WMTDRV_TYPE_T;*/

/*typedef enum _ENUM_WMTDSNS_TYPE_T{
    WMTDSNS_FM_DISABLE = 0,
    WMTDSNS_FM_ENABLE = 1,
    WMTDSNS_FM_GPS_DISABLE = 2,
    WMTDSNS_FM_GPS_ENABLE = 3,
    WMTDSNS_MAX
} ENUM_WMTDSNS_TYPE_T, *P_ENUM_WMTDSNS_TYPE_T;*/

/*typedef enum _ENUM_WMTHWVER_TYPE_T{
    WMTHWVER_E1 = 0x0,
    WMTHWVER_E2 = 0x1,
    WMTHWVER_E3 = 0x2,
    WMTHWVER_E4 = 0x3,
    WMTHWVER_E5 = 0x4,
    WMTHWVER_E6 = 0x5,
    WMTHWVER_E7 = 0x6,
    WMTHWVER_MAX,
    WMTHWVER_INVALID = 0xff
} ENUM_WMTHWVER_TYPE_T, *P_ENUM_WMTHWVER_TYPE_T;*/

/*typedef enum _ENUM_WMTTHERM_TYPE_T{
    WMTTHERM_ZERO = 0,
    WMTTHERM_ENABLE = WMTTHERM_ZERO + 1,
    WMTTHERM_READ = WMTTHERM_ENABLE + 1,
    WMTTHERM_DISABLE = WMTTHERM_READ + 1,
    WMTTHERM_MAX
}ENUM_WMTTHERM_TYPE_T, *P_ENUM_WMTTHERM_TYPE_T;*/

/*typedef enum _ENUM_WMTMSG_TYPE_T {
    WMTMSG_TYPE_POWER_ON = 0,
    WMTMSG_TYPE_POWER_OFF = 1,
    WMTMSG_TYPE_RESET = 2,
    WMTMSG_TYPE_STP_RDY= 3,
    WMTMSG_TYPE_HW_FUNC_ON= 4,
    WMTMSG_TYPE_MAX
} ENUM_WMTMSG_TYPE_T, *P_ENUM_WMTMSG_TYPE_T;*/

typedef void (*PF_WMT_CB)(ENUM_WMTDRV_TYPE_T, ENUM_WMTDRV_TYPE_T, ENUM_WMTMSG_TYPE_T, VOID *, UINT32);

/*typedef enum _SDIO_PS_OP{
    OWN_SET = 0,
    OWN_CLR = 1,
    OWN_STATE = 2,
} SDIO_PS_OP;*/

typedef INT32 (*PF_WMT_SDIO_PSOP)(SDIO_PS_OP);


/*typedef enum _ENUM_WMTCHIN_TYPE_T{
   WMTCHIN_CHIPID = 0x0,
   WMTCHIN_HWVER = WMTCHIN_CHIPID + 1,
   WMTCHIN_MAPPINGHWVER = WMTCHIN_HWVER + 1,
   WMTCHIN_FWVER = WMTCHIN_MAPPINGHWVER + 1,
   WMTCHIN_MAX,
   
}ENUM_WMT_CHIPINFO_TYPE_T, *P_ENUM_WMT_CHIPINFO_TYPE_T;*/


typedef MTK_WCN_BOOL (*MTK_WCN_WMT_FUNC_CTRL)(ENUM_WMTDRV_TYPE_T type);
typedef INT8(*MTK_WCN_WMT_THERM_CTRL)(ENUM_WMTTHERM_TYPE_T eType);
typedef ENUM_WMTHWVER_TYPE_T(*MTK_WCN_WMT_HWVER_GET)(VOID);
typedef MTK_WCN_BOOL (*MTK_WCN_WMT_DSNS_CTRL)(ENUM_WMTDSNS_TYPE_T eType);
typedef INT32 (*MTK_WCN_WMT_MSGCB_REG)(ENUM_WMTDRV_TYPE_T eType,PF_WMT_CB pCb);
typedef INT32 (*MTK_WCN_WMT_MSGCB_UNREG) (ENUM_WMTDRV_TYPE_T eType);
typedef INT32 (*MTK_WCN_WMT_SDIO_OP_REG)(PF_WMT_SDIO_PSOP own_cb);
typedef INT32 (*MTK_WCN_WMT_SDIO_HOST_AWAKE)(VOID);
typedef MTK_WCN_BOOL(*MTK_WCN_WMT_ASSERT)(ENUM_WMTDRV_TYPE_T type,UINT32 reason);
typedef UINT32 (*MTK_WCN_WMT_IC_INFO_GET) (ENUM_WMT_CHIPINFO_TYPE_T type);
typedef INT32 (*MTK_WCN_WMT_PSM_CTRL)(MTK_WCN_BOOL flag);


/*typedef struct _MTK_WCN_WMT_EXP_CB_INFO_{
	MTK_WCN_WMT_FUNC_CTRL wmt_func_on_cb;
	MTK_WCN_WMT_FUNC_CTRL wmt_func_off_cb;
	MTK_WCN_WMT_THERM_CTRL wmt_therm_ctrl_cb;
	MTK_WCN_WMT_HWVER_GET wmt_hwver_get_cb;
	MTK_WCN_WMT_DSNS_CTRL wmt_dsns_ctrl_cb;
	MTK_WCN_WMT_MSGCB_REG wmt_msgcb_reg_cb;
	MTK_WCN_WMT_MSGCB_UNREG wmt_msgcb_unreg_cb;
	MTK_WCN_WMT_SDIO_OP_REG wmt_sdio_op_reg_cb;
	MTK_WCN_WMT_SDIO_HOST_AWAKE wmt_sdio_host_awake_cb;
	MTK_WCN_WMT_ASSERT wmt_assert_cb;
	MTK_WCN_WMT_IC_INFO_GET wmt_ic_info_get_cb;
	MTK_WCN_WMT_PSM_CTRL wmt_psm_ctrl_cb;
}MTK_WCN_WMT_EXP_CB_INFO,*P_MTK_WCN_WMT_EXP_CB_INFO;*/



UINT32 mtk_wcn_stp_exp_cb_reg(P_MTK_WCN_STP_EXP_CB_INFO pStpExpCb);

UINT32 mtk_wcn_stp_exp_cb_unreg(VOID);

UINT32 mtk_wcn_wmt_exp_cb_reg(P_MTK_WCN_WMT_EXP_CB_INFO pWmtExpCb);

UINT32 mtk_wcn_wmt_exp_cb_unreg(VOID);


extern INT32 mtk_wcn_stp_send_data(const PUINT8 buffer, const UINT32 length, const UINT8 type);


extern INT32 mtk_wcn_stp_send_data_raw(const PUINT8 buffer, const UINT32 length, const UINT8 type);


extern INT32 mtk_wcn_stp_parser_data(PUINT8 buffer, UINT32 length);


extern INT32 mtk_wcn_stp_receive_data(PUINT8 buffer, UINT32 length, UINT8 type);


extern MTK_WCN_BOOL mtk_wcn_stp_is_rxqueue_empty(UINT8 type);


extern MTK_WCN_BOOL mtk_wcn_stp_is_ready(void);


extern void mtk_wcn_stp_set_bluez(MTK_WCN_BOOL flags);


extern INT32 mtk_wcn_stp_register_if_tx(ENUM_STP_TX_IF_TYPE stp_if, MTK_WCN_STP_IF_TX func);


extern INT32 mtk_wcn_stp_register_if_rx(MTK_WCN_STP_IF_RX func);


extern INT32 mtk_wcn_stp_register_event_cb(INT32 type, MTK_WCN_STP_EVENT_CB func);


extern INT32 mtk_wcn_stp_register_tx_event_cb(INT32 type, MTK_WCN_STP_EVENT_CB func);


extern INT32 mtk_wcn_stp_coredump_start_get(VOID);



extern MTK_WCN_BOOL mtk_wcn_wmt_func_off (ENUM_WMTDRV_TYPE_T type);

extern MTK_WCN_BOOL mtk_wcn_wmt_func_on (ENUM_WMTDRV_TYPE_T type);

extern INT8 mtk_wcn_wmt_therm_ctrl (ENUM_WMTTHERM_TYPE_T eType);

extern ENUM_WMTHWVER_TYPE_T mtk_wcn_wmt_hwver_get (VOID);

extern UINT32
mtk_wcn_wmt_ic_info_get (ENUM_WMT_CHIPINFO_TYPE_T type);


extern MTK_WCN_BOOL mtk_wcn_wmt_dsns_ctrl (ENUM_WMTDSNS_TYPE_T eType);

extern INT32 mtk_wcn_wmt_msgcb_reg (ENUM_WMTDRV_TYPE_T eType,PF_WMT_CB pCb);

extern INT32 mtk_wcn_wmt_msgcb_unreg (ENUM_WMTDRV_TYPE_T eType);

extern INT32 mtk_wcn_stp_wmt_sdio_op_reg (PF_WMT_SDIO_PSOP own_cb);

extern INT32 mtk_wcn_stp_wmt_sdio_host_awake(VOID);

extern MTK_WCN_BOOL mtk_wcn_wmt_assert (ENUM_WMTDRV_TYPE_T type,UINT32 reason);

extern INT32 mtk_wcn_wmt_psm_ctrl(MTK_WCN_BOOL flag);

#endif

#endif

