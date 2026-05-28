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

/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

#ifndef _STP_EXP_H_
#define _STP_EXP_H_

#include <osal.h>
#include <osal_typedef.h>
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


#ifdef MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT

#if (WMT_IDC_SUPPORT)
#define CFG_WMT_LTE_COEX_HANDLING 1
#define CFG_WMT_LTE_ENABLE_MSGID_MAPPING 0
#else
#define CFG_WMT_LTE_COEX_HANDLING 0
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

#define MTKSTP_BUFFER_SIZE  (16384)	/* Size of RX Queue */

#define STP_EXP_HID_API_EXPORT 0

#else

#define STP_EXP_HID_API_EXPORT 1

#endif

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
#ifdef MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT

typedef VOID (*MTK_WCN_STP_EVENT_CB) (VOID);
typedef INT32 (*MTK_WCN_STP_IF_TX) (const PUINT8 data, const UINT32 size, PUINT32 written_size);
/* export for HIF driver */
typedef VOID(*MTK_WCN_STP_IF_RX)(const PUINT8 data, INT32 size);
typedef INT32 (*MTK_WCN_STP_RX_HAS_PENDING_DATA) (VOID);
typedef INT32 (*MTK_WCN_STP_TX_HAS_PENDING_DATA) (VOID);
typedef P_OSAL_THREAD (*MTK_WCN_STP_RX_THREAD_GET) (VOID);

typedef enum {
	STP_UART_IF_TX = 0,
	STP_SDIO_IF_TX = 1,
	STP_BTIF_IF_TX = 2,
	STP_MAX_IF_TX
} ENUM_STP_TX_IF_TYPE;
#endif
/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
#ifdef MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_receive_data
* DESCRIPTION
*  receive data from serial protocol engine
* PARAMETERS
*  buffer      [IN]        data buffer
*  length      [IN]        data buffer length
* RETURNS
*  INT32    >= 0: size of data received; < 0: error
*****************************************************************************/
extern INT32 mtk_wcn_stp_receive_data(PUINT8 buffer, UINT32 length, UINT8 type);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_send_data
* DESCRIPTION
*  subfunction send data through STP
* PARAMETERS
*  buffer      [IN]        data buffer
*  length      [IN]        data buffer length
*  type        [IN]        subfunction type
* RETURNS
*  INT32    >= 0: length transmitted; < 0: error
*****************************************************************************/
extern INT32 mtk_wcn_stp_send_data(const PUINT8 buffer, const UINT32 length, const UINT8 type);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_is_rxqueue_empty
* DESCRIPTION
*  Is certain rx queue empty?
* PARAMETERS
*  type        [IN]        subfunction type
* RETURNS
*  INT32    0: queue is NOT empyt; !0: queue is empty
*****************************************************************************/
extern MTK_WCN_BOOL mtk_wcn_stp_is_rxqueue_empty(UINT8 type);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_is_enable
* DESCRIPTION
*  Is STP ready?
* PARAMETERS
*  none.
* RETURNS
*  MTK_WCN_BOOL    TRUE:ready, FALSE:not ready
*****************************************************************************/
extern MTK_WCN_BOOL mtk_wcn_stp_is_ready(VOID);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_parser_data
* DESCRIPTION
*  push data to serial transport protocol parser engine
* PARAMETERS
*  buffer      [IN]        data buffer
*  length      [IN]        data buffer length
* RETURNS
*  void
*****************************************************************************/
extern INT32 mtk_wcn_stp_parser_data(PUINT8 buffer, UINT32 length);

/*****************************************************************************
* FUNCTION
*  set_bluetooth_rx_interface
* DESCRIPTION
*  Set bluetooth rx interface
* PARAMETERS
*  rx interface type
* RETURNS
*  void
*****************************************************************************/
extern void mtk_wcn_stp_set_bluez(MTK_WCN_BOOL sdio_flag);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_register_tx_event_cb
* DESCRIPTION
*  regiter Tx event callback function
* PARAMETERS
*  func
* RETURNS
*  INT32: 0:successful , -1: fail
*****************************************************************************/
extern INT32 mtk_wcn_stp_register_tx_event_cb(INT32 type, MTK_WCN_STP_EVENT_CB func);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_register_event_cb
* DESCRIPTION
*  regiter Rx event callback function
* PARAMETERS
*  func
* RETURNS
*  INT32: 0:successful , -1: fail
*****************************************************************************/
extern INT32 mtk_wcn_stp_register_event_cb(INT32 type, MTK_WCN_STP_EVENT_CB func);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_register_if_tx
* DESCRIPTION
*  regiter Tx event callback function
* PARAMETERS
*  stp_if: SDIO or UART, fnnc: Call back function
* RETURNS
*  INT32: 0:successful , -1: fail
*****************************************************************************/
extern INT32 mtk_wcn_stp_register_if_tx(ENUM_STP_TX_IF_TYPE stp_if, MTK_WCN_STP_IF_TX func);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_register_if_rx
* DESCRIPTION
*  regiter Rx event callback function
* PARAMETERS
*  stp_if: SDIO or UART, fnnc: Call back function
* RETURNS
*  int: 0:successful , -1: fail
*****************************************************************************/
extern INT32 mtk_wcn_stp_register_if_rx(MTK_WCN_STP_IF_RX func);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_coredump_start_get
* DESCRIPTION
*  get f/w assert flag in STP context
* PARAMETERS
*  VOID
* RETURNS
*  INT32    0= f/w assert flag is not set, others=f/w assert flag is set
*****************************************************************************/
extern INT32 mtk_wcn_stp_coredump_start_get(VOID);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_register_rx_has_pending_data
* DESCRIPTION
*  regiter rx has pending data call back function
* PARAMETERS
*  stp_if: SDIO or UART, fnnc: Call back function
* RETURNS
*  int: 0:successful , -1: fail
*****************************************************************************/
extern INT32 mtk_wcn_stp_register_rx_has_pending_data(ENUM_STP_TX_IF_TYPE stp_if, MTK_WCN_STP_RX_HAS_PENDING_DATA func);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_register_tx_has_pending_data
* DESCRIPTION
*  regiter tx has pending data call back function
* PARAMETERS
*  stp_if: SDIO or UART, fnnc: Call back function
* RETURNS
*  int: 0:successful , -1: fail
*****************************************************************************/
extern INT32 mtk_wcn_stp_register_tx_has_pending_data(ENUM_STP_TX_IF_TYPE stp_if, MTK_WCN_STP_TX_HAS_PENDING_DATA func);

/*****************************************************************************
* FUNCTION
*  mtk_wcn_stp_register_rx_thread_get
* DESCRIPTION
*  regiter rx thread call back function
* PARAMETERS
*  stp_if: SDIO or UART, fnnc: Call back function
* RETURNS
*  int: 0:successful , -1: fail
*****************************************************************************/
extern INT32 mtk_wcn_stp_register_rx_thread_get(ENUM_STP_TX_IF_TYPE stp_if, MTK_WCN_STP_RX_THREAD_GET func);

extern INT32 mtk_stp_dbg_poll_cpupcr(UINT32 times, UINT32 sleep, UINT32 cmd);
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

extern INT32 mtk_wcn_stp_uart_drv_init(VOID);
extern VOID mtk_wcn_stp_uart_drv_exit(VOID);

#else
#define CFG_WMT_LTE_COEX_HANDLING 0

#define BT_TASK_INDX        (0)
#define FM_TASK_INDX        (1)
#define GPS_TASK_INDX       (2)
#define WIFI_TASK_INDX      (3)
#define WMT_TASK_INDX       (4)
#define STP_TASK_INDX       (5)
#define INFO_TASK_INDX      (6)
#define ANT_TASK_INDX       (7)
#if CFG_WMT_LTE_COEX_HANDLING
#define COEX_TASK_INDX          (8)
#define MTKSTP_MAX_TASK_NUM (9)
#else
#define MTKSTP_MAX_TASK_NUM     (8)
#endif

typedef enum _SDIO_PS_OP{
    OWN_SET = 0,
    OWN_CLR = 1,
    OWN_STATE = 2,
} SDIO_PS_OP;

typedef enum _ENUM_WMTMSG_TYPE_T {
    WMTMSG_TYPE_POWER_ON = 0,
    WMTMSG_TYPE_POWER_OFF = 1,
    WMTMSG_TYPE_RESET = 2,
    WMTMSG_TYPE_STP_RDY= 3,
    WMTMSG_TYPE_HW_FUNC_ON= 4,
    WMTMSG_TYPE_MAX
} ENUM_WMTMSG_TYPE_T, *P_ENUM_WMTMSG_TYPE_T;

typedef enum {
    STP_UART_IF_TX = 0,
    STP_SDIO_IF_TX = 1,
    STP_BTIF_IF_TX = 2,
    STP_MAX_IF_TX
} ENUM_STP_TX_IF_TYPE;

typedef enum _ENUM_WMTDRV_TYPE_T {
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
} ENUM_WMTDRV_TYPE_T, *P_ENUM_WMTDRV_TYPE_T;

typedef enum _ENUM_WMTCHIN_TYPE_T {
        WMTCHIN_CHIPID = 0x0,
        WMTCHIN_HWVER = WMTCHIN_CHIPID + 1,
        WMTCHIN_MAPPINGHWVER = WMTCHIN_HWVER + 1,
        WMTCHIN_FWVER = WMTCHIN_MAPPINGHWVER + 1,
        WMTCHIN_IPVER = WMTCHIN_FWVER + 1,
        WMTCHIN_MAX,

} ENUM_WMT_CHIPINFO_TYPE_T, *P_ENUM_WMT_CHIPINFO_TYPE_T;

typedef enum _ENUM_WMTTHERM_TYPE_T{
    WMTTHERM_ZERO = 0,
    WMTTHERM_ENABLE = WMTTHERM_ZERO + 1,
    WMTTHERM_READ = WMTTHERM_ENABLE + 1,
    WMTTHERM_DISABLE = WMTTHERM_READ + 1,
    WMTTHERM_MAX
}ENUM_WMTTHERM_TYPE_T, *P_ENUM_WMTTHERM_TYPE_T;

typedef enum _ENUM_WMT_FLASH_PATCH_SEQ_T {
        WMT_FLASH_PATCH_HEAD_PKT = 0,
        WMT_FLASH_PATCH_START_PKT = WMT_FLASH_PATCH_HEAD_PKT + 1,
        WMT_FLASH_PATCH_CONTINUE_PKT = WMT_FLASH_PATCH_START_PKT + 1,
        WMT_FLASH_PATCH_END_PKT = WMT_FLASH_PATCH_CONTINUE_PKT + 1,
        WMT_FLASH_PATCH_SEQ_MAX,
} ENUM_WMT_FLASH_PATCH_SEQ, *P_ENUM_WMT_FLASH_PATCH_SEQ;

typedef enum _ENUM_WMTHWVER_TYPE_T {
        WMTHWVER_E1 = 0x0,
        WMTHWVER_E2 = 0x1,
        WMTHWVER_E3 = 0x2,
        WMTHWVER_E4 = 0x3,
        WMTHWVER_E5 = 0x4,
        WMTHWVER_E6 = 0x5,
        WMTHWVER_E7 = 0x6,
        WMTHWVER_MAX,
        WMTHWVER_INVALID = 0xff
} ENUM_WMTHWVER_TYPE_T, *P_ENUM_WMTHWVER_TYPE_T;

typedef void (*MTK_WCN_STP_EVENT_CB)(void);
typedef INT32 (*PF_WMT_SDIO_PSOP)(SDIO_PS_OP);
typedef void (*PF_WMT_CB)(ENUM_WMTDRV_TYPE_T, ENUM_WMTDRV_TYPE_T, ENUM_WMTMSG_TYPE_T, VOID *, UINT32);
typedef INT32 (*MTK_WCN_STP_IF_TX) (const PUINT8 data, const UINT32 size, PUINT32 written_size);
typedef INT32 (*MTK_WCN_STP_RX_HAS_PENDING_DATA) (VOID);
typedef INT32 (*MTK_WCN_STP_TX_HAS_PENDING_DATA) (VOID);

extern INT32 mtk_wcn_stp_receive_data(PUINT8 buffer, UINT32 length, UINT8 type);
extern INT32 _mtk_wcn_stp_receive_data(PUINT8 buffer, UINT32 length, UINT8 type);
extern INT32 _mtk_wcn_stp_send_data_raw(const PUINT8 buffer, const UINT32 length, const UINT8 type);
extern INT32 mtk_wcn_stp_send_data(const PUINT8 buffer, const UINT32 length, const UINT8 type);
extern INT32 _mtk_wcn_stp_send_data(const PUINT8 buffer, const UINT32 length, const UINT8 type);
extern MTK_WCN_BOOL _mtk_wcn_stp_is_rxqueue_empty(UINT8 type);
extern MTK_WCN_BOOL mtk_wcn_stp_is_ready(VOID);
extern MTK_WCN_BOOL _mtk_wcn_stp_is_ready(VOID);
extern INT32 mtk_wcn_stp_parser_data(PUINT8 buffer, UINT32 length);
extern INT32 _mtk_wcn_stp_parser_data(PUINT8 buffer, UINT32 length);
extern VOID _mtk_wcn_stp_set_bluez(MTK_WCN_BOOL sdio_flag);
//extern INT32 _mtk_wcn_stp_register_tx_event_cb(INT32 type, MTK_WCN_STP_EVENT_CB func);
extern INT32 mtk_wcn_stp_register_event_cb(INT32 type, MTK_WCN_STP_EVENT_CB func);
//extern INT32 _mtk_wcn_stp_register_if_tx(ENUM_STP_TX_IF_TYPE stp_if, MTK_WCN_STP_IF_TX func);
//extern INT32 _mtk_wcn_stp_register_if_rx(MTK_WCN_STP_IF_RX func);
extern INT32 _mtk_wcn_stp_coredump_start_get(VOID);
//extern INT32 _mtk_wcn_stp_register_rx_has_pending_data(ENUM_STP_TX_IF_TYPE stp_if,
//						       MTK_WCN_STP_RX_HAS_PENDING_DATA func);
//extern INT32 _mtk_wcn_stp_register_tx_has_pending_data(ENUM_STP_TX_IF_TYPE stp_if,
//						       MTK_WCN_STP_TX_HAS_PENDING_DATA func);
//extern INT32 _mtk_wcn_stp_register_rx_thread_get(ENUM_STP_TX_IF_TYPE stp_if, MTK_WCN_STP_RX_THREAD_GET func);
extern VOID mtk_wcn_wmt_func_ctrl_for_plat(UINT32 on, ENUM_WMTDRV_TYPE_T type);
extern VOID mtk_wcn_wmt_exp_init(VOID);

#endif /* MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT */

#endif /* _STP_EXP_H_ */
