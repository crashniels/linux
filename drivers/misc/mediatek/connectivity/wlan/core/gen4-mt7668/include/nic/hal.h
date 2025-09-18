/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "hal.h"
 *   \brief  The declaration of hal functions
 *
 *   N/A
 */

#ifndef _HAL_H
#define _HAL_H

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

/* Macros for flag operations for the Adapter structure */
#define HAL_SET_FLAG(_M, _F)      ((_M)->u4HwFlags |= (_F))
#define HAL_CLEAR_FLAG(_M, _F)    ((_M)->u4HwFlags &= ~(_F))
#define HAL_TEST_FLAG(_M, _F)     ((_M)->u4HwFlags & (_F))
#define HAL_TEST_FLAGS(_M, _F)    (((_M)->u4HwFlags & (_F)) == (_F))

#define HAL_MCR_RD(_prAdapter, _u4Offset, _pu4Value)                        \
    do {                                                                    \
        if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == false) {      \
            if (_prAdapter->rAcpiState == ACPI_STATE_D3) {                  \
                ASSERT(0);                                                  \
            }                                                               \
            if (_u4Offset > 0xFFFF) {                                       \
                if (kalDevRegRead_mac(_prAdapter->prGlueInfo,               \
                                      _u4Offset,                            \
                                      _pu4Value) == false) {                \
                    HAL_SET_FLAG(_prAdapter,                                \
                                 ADAPTER_FLAG_HW_ERR);                      \
                    fgIsBusAccessFailed = true;                             \
                    DBGLOG(HAL,                                             \
                           ERROR,                                           \
                           "HAL_MCR_RD (MAC) access fail! 0x%lx: 0x%lx\n",  \
                           (u32)(_u4Offset),                                \
                           *((u32 *)(_pu4Value)));                          \
                }                                                           \
            } else {                                                        \
                if (kalDevRegRead(_prAdapter->prGlueInfo,                   \
                                  _u4Offset,                                \
                                  _pu4Value) == false) {                    \
                    HAL_SET_FLAG(_prAdapter,                                \
                                 ADAPTER_FLAG_HW_ERR);                      \
                    fgIsBusAccessFailed = true;                             \
                    DBGLOG(HAL,                                             \
                           ERROR,                                           \
                           "HAL_MCR_RD (SDIO) access fail! 0x%lx: 0x%lx\n", \
                           (u32)(_u4Offset),                                \
                           *((u32 *)(_pu4Value)));                          \
                }                                                           \
            }                                                               \
        } else {                                                            \
            DBGLOG(HAL, WARN, "ignore HAL_MCR_RD access! 0x%lx\n",          \
                   (u32)(_u4Offset));                                       \
        }                                                                   \
    } while (0)

#define HAL_MCR_WR(_prAdapter, _u4Offset, _u4Value)                         \
    do {                                                                    \
        if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == false) {      \
            if (_prAdapter->rAcpiState == ACPI_STATE_D3) {                  \
                ASSERT(0);                                                  \
            }                                                               \
            if (_u4Offset > 0xFFFF) {                                       \
                if (kalDevRegWrite_mac(_prAdapter->prGlueInfo,              \
                                       _u4Offset,                           \
                                       _u4Value) == false) {                \
                    HAL_SET_FLAG(_prAdapter,                                \
                                 ADAPTER_FLAG_HW_ERR);                      \
                    fgIsBusAccessFailed = true;                             \
                    DBGLOG(HAL,                                             \
                           ERROR,                                           \
                           "HAL_MCR_WR (MAC) access fail! 0x%lx: 0x%lx\n",  \
                           (u32)(_u4Offset),                                \
                           (u32)(_u4Value));                                \
                }                                                           \
            } else {                                                        \
                if (kalDevRegWrite(_prAdapter->prGlueInfo,                  \
                                   _u4Offset,                               \
                                   _u4Value) == false) {                    \
                    HAL_SET_FLAG(_prAdapter,                                \
                                 ADAPTER_FLAG_HW_ERR);                      \
                    fgIsBusAccessFailed = true;                             \
                    DBGLOG(HAL,                                             \
                           ERROR,                                           \
                           "HAL_MCR_WR (SDIO) access fail! 0x%lx: 0x%lx\n", \
                           (u32)(_u4Offset),                                \
                           (u32)(_u4Value));                                \
                }                                                           \
            }                                                               \
        } else {                                                            \
            DBGLOG(HAL, WARN,                                               \
                   "ignore HAL_MCR_WR access! 0x%lx: 0x%lx\n",              \
                   (u32)(_u4Offset), (u32)(_u4Value));                      \
        }                                                                   \
    } while (0)

#define HAL_PORT_RD(_prAdapter, _u4Port, _u4Len, _pucBuf, _u4ValidBufSize) \
    {                                                                      \
        /*fgResult = false; */                                             \
        if (_prAdapter->rAcpiState == ACPI_STATE_D3) {                     \
            ASSERT(0);                                                     \
        }                                                                  \
        if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == false) {     \
            if (kalDevPortRead(_prAdapter->prGlueInfo, _u4Port,            \
                               _u4Len, _pucBuf,                            \
                               _u4ValidBufSize) == false) {                \
                HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR);             \
                fgIsBusAccessFailed = true;                                \
                DBGLOG(HAL, ERROR,                                         \
                       "HAL_PORT_RD access fail! 0x%lx\n",                 \
                       (u32)(_u4Port));                                    \
            } else {                                                       \
                /*fgResult = true;*/ }                                     \
        } else {                                                           \
            DBGLOG(HAL, WARN,                                              \
                   "ignore HAL_PORT_RD access! 0x%lx\n",                   \
                   (u32)(_u4Port));                                        \
        }                                                                  \
    }

#define HAL_PORT_WR(_prAdapter, _u4Port, _u4Len, _pucBuf, _u4ValidBufSize) \
    {                                                                      \
        /*fgResult = false; */                                             \
        if (_prAdapter->rAcpiState == ACPI_STATE_D3) {                     \
            ASSERT(0);                                                     \
        }                                                                  \
        if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == false) {     \
            if (kalDevPortWrite(_prAdapter->prGlueInfo, _u4Port,           \
                                _u4Len, _pucBuf,                           \
                                _u4ValidBufSize) == false) {               \
                HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR);             \
                fgIsBusAccessFailed = true;                                \
                DBGLOG(HAL, ERROR,                                         \
                       "HAL_PORT_WR access fail! 0x%lx\n",                 \
                       (u32)(_u4Port));                                    \
            }                                                              \
        } else {                                                           \
            DBGLOG(HAL, WARN,                                              \
                   "ignore HAL_PORT_WR access! 0x%lx\n",                   \
                   (u32)(_u4Port));                                        \
        }                                                                  \
    }

#define HAL_BYTE_WR(_prAdapter, _u4Port, _ucBuf)                       \
    {                                                                  \
        if (_prAdapter->rAcpiState == ACPI_STATE_D3) {                 \
            ASSERT(0);                                                 \
        }                                                              \
        if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == false) { \
            if (kalDevWriteWithSdioCmd52(_prAdapter->prGlueInfo,       \
                                         _u4Port,                      \
                                         _ucBuf) == false) {           \
                HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR);         \
                fgIsBusAccessFailed = true;                            \
                DBGLOG(HAL, ERROR,                                     \
                       "HAL_BYTE_WR access fail! 0x%lx\n",             \
                       (u32)(_u4Port));                                \
            } else {                                                   \
                /* Todo:: Nothing*/                                    \
            }                                                          \
        } else {                                                       \
            DBGLOG(HAL, WARN,                                          \
                   "ignore HAL_BYTE_WR access! 0x%lx\n",               \
                   (u32)(_u4Port));                                    \
        }                                                              \
    }

#define HAL_DRIVER_OWN_BY_SDIO_CMD52(_prAdapter, _pfgDriverIsOwnReady)      \
    {                                                                       \
        u8 ucBuf = BIT(1);                                                  \
        if (_prAdapter->rAcpiState == ACPI_STATE_D3) {                      \
            ASSERT(0);                                                      \
        }                                                                   \
        if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == false) {      \
            if (kalDevReadAfterWriteWithSdioCmd52(                          \
                    _prAdapter->prGlueInfo, MCR_WHLPCR_BYTE1,               \
                    &ucBuf, 1) == false) {                                  \
                HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR);              \
                fgIsBusAccessFailed = true;                                 \
                DBGLOG(HAL,                                                 \
                       ERROR,                                               \
                       "kalDevReadAfterWriteWithSdioCmd52 access fail!\n"); \
            } else {                                                        \
                *_pfgDriverIsOwnReady =                                     \
                    (ucBuf & BIT(0)) ? true : false;                        \
            }                                                               \
        } else {                                                            \
            DBGLOG(HAL, WARN,                                               \
                   "ignore HAL_DRIVER_OWN_BY_SDIO_CMD52 access!\n");        \
        }                                                                   \
    }

#define HAL_WRITE_TX_DATA(_prAdapter, _prMsduInfo)            \
    {                                                         \
        if (_prAdapter->rAcpiState == ACPI_STATE_D3) {        \
            ASSERT(0);                                        \
        }                                                     \
        kalDevWriteData(_prAdapter->prGlueInfo, _prMsduInfo); \
    }

#define HAL_KICK_TX_DATA(_prAdapter)                   \
    {                                                  \
        if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
            ASSERT(0);                                 \
        }                                              \
        kalDevKickData(_prAdapter->prGlueInfo);        \
    }

#if CFG_MESON_G12A_PATCH
#define HAL_WRITE_TX_CMD(_prAdapter, _prCmdInfo, _ucTC)             \
    ({                                                              \
        u32 retval;                                                 \
        if (_prAdapter->rAcpiState == ACPI_STATE_D3) {              \
            ASSERT(0);                                              \
        }                                                           \
        retval = kalDevWriteCmd(_prAdapter->prGlueInfo, _prCmdInfo, \
                                _ucTC);                             \
        retval;                                                     \
    })
#else
#define HAL_WRITE_TX_CMD(_prAdapter, _prCmdInfo, _ucTC)            \
    {                                                              \
        if (_prAdapter->rAcpiState == ACPI_STATE_D3) {             \
            ASSERT(0);                                             \
        }                                                          \
        kalDevWriteCmd(_prAdapter->prGlueInfo, _prCmdInfo, _ucTC); \
    }
#endif

#define HIF_H2D_SW_INT_SHFT    (16)
#define SDIO_MAILBOX_FUNC_READ_REG_IDX \
    (BIT(0) << HIF_H2D_SW_INT_SHFT)  /* bit16 */
#define SDIO_MAILBOX_FUNC_WRITE_REG_IDX \
    (BIT(1) << HIF_H2D_SW_INT_SHFT)  /* bit17 */
#define SDIO_MAILBOX_FUNC_CHECKSUN16_IDX \
    (BIT(2) << HIF_H2D_SW_INT_SHFT)  /* bit18 */

#define HAL_READ_RX_PORT(prAdapter, u4PortId, u4Len, pvBuf, _u4ValidBufSize) \
    {                                                                        \
        ASSERT(u4PortId < 2);                                                \
        HAL_PORT_RD(prAdapter,                                               \
                    ((u4PortId == 0) ? MCR_WRDR0 : MCR_WRDR1), u4Len,        \
                    pvBuf, _u4ValidBufSize  /*temp!!*/ /*4Kbyte*/ )          \
    }

#define HAL_WRITE_TX_PORT(_prAdapter, _u4PortId, _u4Len, _pucBuf, \
                          _u4ValidBufSize)                        \
    {                                                             \
        if ((_u4ValidBufSize - _u4Len) >= sizeof(u32)) {          \
            /* fill with single dword of zero as TX-aggregation \
             * termination */                                \
            *(u32 *)(&((_pucBuf)[ALIGN_4(_u4Len)])) = 0;     \
        }                                                    \
        HAL_PORT_WR(_prAdapter, MCR_WTDR1, _u4Len, _pucBuf,  \
                    _u4ValidBufSize  /*temp!!*/ /*4KByte*/ ) \
    }

/* The macro to read the given MCR several times to check if the wait
 *  condition come true.
 */
#define HAL_MCR_RD_AND_WAIT(_pAdapter, _offset, _pReadValue, _waitCondition, \
                            _waitDelay, _waitCount, _status)                 \
    {                                                                        \
        u32 count;                                                           \
        (_status) = false;                                                   \
        for (count = 0; count < (_waitCount); count++) {                     \
            HAL_MCR_RD((_pAdapter), (_offset), (_pReadValue));               \
            if ((_waitCondition)) {                                          \
                (_status) = true;                                            \
                break;                                                       \
            }                                                                \
            kalUdelay((_waitDelay));                                         \
        }                                                                    \
    }

/* The macro to write 1 to a R/S bit and read it several times to check if the
 *  command is done
 */
#define HAL_MCR_WR_AND_WAIT(_pAdapter, _offset, _writeValue, _busyMask, \
                            _waitDelay, _waitCount, _status)            \
    {                                                                   \
        u32 u4Temp;                                                     \
        u32 u4Count = _waitCount;                                       \
        (_status) = false;                                              \
        HAL_MCR_WR((_pAdapter), (_offset), (_writeValue));              \
        do {                                                            \
            kalUdelay((_waitDelay));                                    \
            HAL_MCR_RD((_pAdapter), (_offset), &u4Temp);                \
            if (!(u4Temp & (_busyMask))) {                              \
                (_status) = true;                                       \
                break;                                                  \
            }                                                           \
            u4Count--;                                                  \
        } while (u4Count);                                              \
    }

#define HAL_GET_CHIP_ID_VER(_prAdapter, pu2ChipId, pu2Version) \
    {                                                          \
        u32 u4Value;                                           \
        HAL_MCR_RD(_prAdapter, MCR_WCIR, &u4Value);            \
        *pu2ChipId = (u16)(u4Value & WCIR_CHIP_ID);            \
        *pu2Version = (u16)(u4Value & WCIR_REVISION_ID) >> 16; \
    }

#define HAL_WIFI_FUNC_READY_CHECK(_prAdapter, _checkItem, _pfgResult) \
    do {                                                              \
        u32 u4Value;                                                  \
        *_pfgResult = false;                                          \
        HAL_MCR_RD(_prAdapter, MCR_WCIR, &u4Value);                   \
        if (u4Value & WCIR_WLAN_READY) {                              \
            *_pfgResult = true;                                       \
        }                                                             \
    } while (0)

#define HAL_WIFI_FUNC_OFF_CHECK(_prAdapter, _checkItem, _pfgResult) \
    do {                                                            \
        u32 u4Value;                                                \
        *_pfgResult = false;                                        \
        HAL_MCR_RD(_prAdapter, MCR_WCIR, &u4Value);                 \
        if ((u4Value & WCIR_WLAN_READY) == 0) {                     \
            *_pfgResult = true;                                     \
        }                                                           \
        halPrintMailbox(prAdapter);                                 \
        halPollDbgCr(_prAdapter, LP_DBGCR_POLL_ROUND);              \
    } while (0)

#define HAL_WIFI_FUNC_GET_STATUS(_prAdapter, _u4Result) \
    halGetMailbox(_prAdapter, 0, &_u4Result)

#define HAL_INTR_DISABLE(_prAdapter) \
    HAL_MCR_WR(_prAdapter, MCR_WHLPCR, WHLPCR_INT_EN_CLR)

#define HAL_INTR_ENABLE(_prAdapter) \
    HAL_MCR_WR(_prAdapter, MCR_WHLPCR, WHLPCR_INT_EN_SET)

#define HAL_INTR_ENABLE_AND_LP_OWN_SET(_prAdapter) \
    HAL_MCR_WR(_prAdapter, MCR_WHLPCR,             \
               (WHLPCR_INT_EN_SET | WHLPCR_FW_OWN_REQ_SET))

#define HAL_LP_OWN_RD(_prAdapter, _pfgResult)            \
    {                                                    \
        u32 u4RegValue;                                  \
        *_pfgResult = false;                             \
        HAL_MCR_RD(_prAdapter, MCR_WHLPCR, &u4RegValue); \
        if (u4RegValue & WHLPCR_IS_DRIVER_OWN) {         \
            *_pfgResult = true;                          \
        }                                                \
    }

#define HAL_LP_OWN_SET(_prAdapter, _pfgResult)                     \
    {                                                              \
        u32 u4RegValue;                                            \
        *_pfgResult = false;                                       \
        HAL_MCR_WR(_prAdapter, MCR_WHLPCR, WHLPCR_FW_OWN_REQ_SET); \
        HAL_MCR_RD(_prAdapter, MCR_WHLPCR, &u4RegValue);           \
        if (u4RegValue & WHLPCR_IS_DRIVER_OWN) {                   \
            *_pfgResult = true;                                    \
        }                                                          \
    }

#define HAL_LP_OWN_CLR(_prAdapter, _pfgResult)                     \
    {                                                              \
        u32 u4RegValue;                                            \
        *_pfgResult = false;                                       \
        /* Software get LP ownership */                            \
        HAL_MCR_WR(_prAdapter, MCR_WHLPCR, WHLPCR_FW_OWN_REQ_CLR); \
        HAL_MCR_RD(_prAdapter, MCR_WHLPCR, &u4RegValue);           \
        if (u4RegValue & WHLPCR_IS_DRIVER_OWN) {                   \
            *_pfgResult = true;                                    \
        }                                                          \
    }

#define HAL_GET_ABNORMAL_INTERRUPT_REASON_CODE(_prAdapter, pu4AbnormalReason) \
    {                                                                         \
        HAL_MCR_RD(_prAdapter, MCR_WASR, pu4AbnormalReason);                  \
    }

#define HAL_DISABLE_RX_ENHANCE_MODE(_prAdapter)         \
    {                                                   \
        u32 u4Value;                                    \
        HAL_MCR_RD(_prAdapter, MCR_WHCR, &u4Value);     \
        HAL_MCR_WR(_prAdapter, MCR_WHCR,                \
                   u4Value & ~WHCR_RX_ENHANCE_MODE_EN); \
    }

#define HAL_ENABLE_RX_ENHANCE_MODE(_prAdapter)         \
    {                                                  \
        u32 u4Value;                                   \
        HAL_MCR_RD(_prAdapter, MCR_WHCR, &u4Value);    \
        HAL_MCR_WR(_prAdapter, MCR_WHCR,               \
                   u4Value | WHCR_RX_ENHANCE_MODE_EN); \
    }

#define HAL_CFG_MAX_HIF_RX_LEN_NUM(_prAdapter, _ucNumOfRxLen)          \
    {                                                                  \
        u32 u4Value, ucNum;                                            \
        ucNum = ((_ucNumOfRxLen >= 16) ? 0 : _ucNumOfRxLen);           \
        u4Value = 0;                                                   \
        HAL_MCR_RD(_prAdapter, MCR_WHCR, &u4Value);                    \
        u4Value &= ~WHCR_MAX_HIF_RX_LEN_NUM;                           \
        u4Value |= ((((u32)ucNum) << WHCR_OFFSET_MAX_HIF_RX_LEN_NUM) & \
                    WHCR_MAX_HIF_RX_LEN_NUM);                          \
        HAL_MCR_WR(_prAdapter, MCR_WHCR, u4Value);                     \
    }

#define HAL_SET_INTR_STATUS_READ_CLEAR(prAdapter)              \
    {                                                          \
        u32 u4Value;                                           \
        HAL_MCR_RD(prAdapter, MCR_WHCR, &u4Value);             \
        HAL_MCR_WR(prAdapter, MCR_WHCR,                        \
                   u4Value & ~WHCR_W_INT_CLR_CTRL);            \
        prAdapter->prGlueInfo->rHifInfo.fgIntReadClear = true; \
    }

#define HAL_SET_INTR_STATUS_WRITE_1_CLEAR(prAdapter)            \
    {                                                           \
        u32 u4Value;                                            \
        HAL_MCR_RD(prAdapter, MCR_WHCR, &u4Value);              \
        HAL_MCR_WR(prAdapter, MCR_WHCR,                         \
                   u4Value | WHCR_W_INT_CLR_CTRL);              \
        prAdapter->prGlueInfo->rHifInfo.fgIntReadClear = false; \
    }

/* Note: enhance mode structure may also carried inside the buffer,
 *  if the length of the buffer is long enough
 */
#define HAL_READ_INTR_STATUS(prAdapter, length, pvBuf) \
    HAL_PORT_RD(prAdapter, MCR_WHISR, length, pvBuf, length)

#define HAL_READ_TX_RELEASED_COUNT(_prAdapter, au2TxReleaseCount) \
    {                                                             \
        u32 *pu4Value = (u32 *)au2TxReleaseCount;                 \
        HAL_MCR_RD(_prAdapter, MCR_WTQCR0, &pu4Value[0]);         \
        HAL_MCR_RD(_prAdapter, MCR_WTQCR1, &pu4Value[1]);         \
        HAL_MCR_RD(_prAdapter, MCR_WTQCR2, &pu4Value[2]);         \
        HAL_MCR_RD(_prAdapter, MCR_WTQCR3, &pu4Value[3]);         \
        HAL_MCR_RD(_prAdapter, MCR_WTQCR4, &pu4Value[4]);         \
        HAL_MCR_RD(_prAdapter, MCR_WTQCR5, &pu4Value[5]);         \
        HAL_MCR_RD(_prAdapter, MCR_WTQCR6, &pu4Value[6]);         \
        HAL_MCR_RD(_prAdapter, MCR_WTQCR7, &pu4Value[7]);         \
    }

#define HAL_READ_RX_LENGTH(prAdapter, pu2Rx0Len, pu2Rx1Len) \
    {                                                       \
        u32 u4Value;                                        \
        u4Value = 0;                                        \
        HAL_MCR_RD(prAdapter, MCR_WRPLR, &u4Value);         \
        *pu2Rx0Len = (u16)u4Value;                          \
        *pu2Rx1Len = (u16)(u4Value >> 16);                  \
    }

#define HAL_GET_INTR_STATUS_FROM_ENHANCE_MODE_STRUCT(pvBuf, u2Len, pu4Status) \
    {                                                                         \
        u32 *pu4Buf = (u32 *)pvBuf;                                           \
        *pu4Status = pu4Buf[0];                                               \
    }

#define HAL_GET_TX_STATUS_FROM_ENHANCE_MODE_STRUCT(pvInBuf, pu4BufOut, \
                                                   u4LenBufOut)        \
    {                                                                  \
        u32 *pu4Buf = (u32 *)pvInBuf;                                  \
        ASSERT(u4LenBufOut >= 8);                                      \
        pu4BufOut[0] = pu4Buf[1];                                      \
        pu4BufOut[1] = pu4Buf[2];                                      \
    }

#define HAL_GET_RX_LENGTH_FROM_ENHANCE_MODE_STRUCT(          \
        pvInBuf, pu2Rx0Num, au2Rx0Len, pu2Rx1Num, au2Rx1Len) \
    {                                                        \
        u32 *pu4Buf = (u32 *)pvInBuf;                        \
        ASSERT((sizeof(au2Rx0Len) / sizeof(u16)) >= 16);     \
        ASSERT((sizeof(au2Rx1Len) / sizeof(u16)) >= 16);     \
        *pu2Rx0Num = (u16)pu4Buf[3];                         \
        *pu2Rx1Num = (u16)(pu4Buf[3] >> 16);                 \
        kalMemCopy(au2Rx0Len, &pu4Buf[4], 8);                \
        kalMemCopy(au2Rx1Len, &pu4Buf[12], 8);               \
    }

#define HAL_GET_MAILBOX_FROM_ENHANCE_MODE_STRUCT(pvInBuf, pu4Mailbox0, \
                                                 pu4Mailbox1)          \
    {                                                                  \
        u32 *pu4Buf = (u32 *)pvInBuf;                                  \
        *pu4Mailbox0 = (u16)pu4Buf[21];                                \
        *pu4Mailbox1 = (u16)pu4Buf[22];                                \
    }

#define HAL_IS_TX_DONE_INTR(u4IntrStatus) \
    ((u4IntrStatus & WHISR_TX_DONE_INT) ? true : false)

#define HAL_IS_RX_DONE_INTR(u4IntrStatus)                                \
    ((u4IntrStatus & (WHISR_RX0_DONE_INT | WHISR_RX1_DONE_INT)) ? true : \
     false)

#define HAL_IS_ABNORMAL_INTR(u4IntrStatus) \
    ((u4IntrStatus & WHISR_ABNORMAL_INT) ? true : false)

#define HAL_IS_FW_OWNBACK_INTR(u4IntrStatus) \
    ((u4IntrStatus & WHISR_FW_OWN_BACK_INT) ? true : false)

#define HAL_PUT_MAILBOX(prAdapter, u4MboxId, u4Data)              \
    {                                                             \
        ASSERT(u4MboxId < 2);                                     \
        HAL_MCR_WR(prAdapter,                                     \
                   ((u4MboxId == 0) ? MCR_H2DSM0R : MCR_H2DSM1R), \
                   u4Data);                                       \
    }

#define HAL_GET_MAILBOX(prAdapter, u4MboxId, pu4Data)             \
    {                                                             \
        ASSERT(u4MboxId < 2);                                     \
        HAL_MCR_RD(prAdapter,                                     \
                   ((u4MboxId == 0) ? MCR_D2HRM0R : MCR_D2HRM1R), \
                   pu4Data);                                      \
    }

#define HAL_SET_MAILBOX_READ_CLEAR(prAdapter, fgEnableReadClear) \
    {                                                            \
        u32 u4Value;                                             \
        HAL_MCR_RD(prAdapter, MCR_WHCR, &u4Value);               \
        HAL_MCR_WR(prAdapter, MCR_WHCR,                          \
                   (fgEnableReadClear) ?                         \
                   (u4Value | WHCR_RECV_MAILBOX_RD_CLR_EN) :     \
                   (u4Value & ~WHCR_RECV_MAILBOX_RD_CLR_EN));    \
        prAdapter->prGlueInfo->rHifInfo.fgMbxReadClear =         \
            fgEnableReadClear;                                   \
    }

#define HAL_GET_MAILBOX_READ_CLEAR(prAdapter) \
    (prAdapter->prGlueInfo->rHifInfo.fgMbxReadClear)

#define HAL_READ_INT_STATUS(prAdapter, _pu4IntStatus)  \
    {                                                  \
        kalDevReadIntStatus(prAdapter, _pu4IntStatus); \
    }

#define HAL_HIF_INIT(_prAdapter) \
    {                            \
        halDevInit(_prAdapter);  \
    }

#define HAL_ENABLE_FWDL(_prAdapter, _fgEnable)

#define HAL_WAKE_UP_WIFI(_prAdapter) \
    {                                \
        halWakeUpWiFi(_prAdapter);   \
    }

#define HAL_IS_TX_DIRECT(_prAdapter)    false

#define HAL_IS_RX_DIRECT(_prAdapter)    false

#define INVALID_VERSION    0xFFFF  /* used by HW/FW version */
/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

u8 halVerifyChipID(IN P_ADAPTER_T prAdapter);
u32 halGetChipHwVer(IN P_ADAPTER_T prAdapter);
u32 halGetChipSwVer(IN P_ADAPTER_T prAdapter);

WLAN_STATUS halRxWaitResponse(IN P_ADAPTER_T prAdapter,
                              IN u8 ucPortIdx,
                              OUT u8 *pucRspBuffer,
                              IN u32 u4MaxRespBufferLen,
                              OUT u32 *pu4Length);

void halEnableInterrupt(IN P_ADAPTER_T prAdapter);
void halDisableInterrupt(IN P_ADAPTER_T prAdapter);

u8 halSetDriverOwn(IN P_ADAPTER_T prAdapter);
void halSetFWOwn(IN P_ADAPTER_T prAdapter, IN u8 fgEnableGlobalInt);

void halDevInit(IN P_ADAPTER_T prAdapter);
void halEnableFWDownload(IN P_ADAPTER_T prAdapter, IN u8 fgEnable);
void halWakeUpWiFi(IN P_ADAPTER_T prAdapter);
void halTxCancelAllSending(IN P_ADAPTER_T prAdapter);
void halProcessTxInterrupt(IN P_ADAPTER_T prAdapter);
WLAN_STATUS halTxPollingResource(IN P_ADAPTER_T prAdapter, IN u8 ucTC);
void halSerHifReset(IN P_ADAPTER_T prAdapter);

void halProcessRxInterrupt(IN P_ADAPTER_T prAdapter);
void halProcessSoftwareInterrupt(IN P_ADAPTER_T prAdapter);
/* Hif power off wifi */
WLAN_STATUS halHifPowerOffWifi(IN P_ADAPTER_T prAdapter);
u32 halDumpHifStatus(IN P_ADAPTER_T prAdapter, IN u8 *pucBuf, IN u32 u4Max);
u8 halIsPendingRx(IN P_ADAPTER_T prAdapter);
u32 halGetValidCoalescingBufSize(IN P_ADAPTER_T prAdapter);
WLAN_STATUS halAllocateIOBuffer(IN P_ADAPTER_T prAdapter);
WLAN_STATUS halReleaseIOBuffer(IN P_ADAPTER_T prAdapter);
void halDeAggRxPktWorker(struct work_struct *work);
void halPrintHifDbgInfo(IN P_ADAPTER_T prAdapter);
u8 halIsTxResourceControlEn(IN P_ADAPTER_T prAdapter);
void halTxResourceResetHwTQCounter(IN P_ADAPTER_T prAdapter);

void halWpdmaSetup(P_GLUE_INFO_T prGlueInfo, u8 enable);

bool halIsHifStateReady(IN P_ADAPTER_T prAdapter, u8 *pucState);

bool halIsHifStateSuspend(IN P_ADAPTER_T prAdapter);

#endif
