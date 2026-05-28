/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   gl_typedef.h
 *    \brief  Definition of basic data type(os dependent).
 *
 *    In this file we define the basic data type.
 */

#ifndef _GL_TYPEDEF_H
#define _GL_TYPEDEF_H

#include <linux/mmc/ioctl.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>

#if CFG_ENABLE_EARLY_SUSPEND
#include <linux/earlysuspend.h>
#endif

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

/* Define HZ of timer tick for function kalGetTimeTick() */
#define KAL_HZ    (1000)

#ifndef NULL
#if defined(__cplusplus)
#define NULL      0
#else
#define NULL      ((void *)0)
#endif
#endif

#if CFG_ENABLE_EARLY_SUSPEND
typedef void (*early_suspend_callback)(struct early_suspend *h);
typedef void (*late_resume_callback)(struct early_suspend *h);
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* Type definition of large integer (64bits) union to be comptaible with
 * Windows definition, so we won't apply our own coding style to these data
 * types. NOTE: LARGE_INTEGER must NOT be floating variable. <TODO>: Check for
 * big-endian compatibility.
 */
typedef union _LARGE_INTEGER {
    struct {
        u32 LowPart;
        s32 HighPart;
    } u;
    s64 QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
    struct {
        u32 LowPart;
        u32 HighPart;
    } u;
    u64 QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

typedef s32 (*probe_card)(struct sdio_func *pvData, void *pvDriverData);
typedef void (*remove_card)(void);

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

#define IN                        /* volatile */
#define OUT                       /* volatile */

#define __KAL_INLINE__        inline
#define __KAL_ATTRIB_PACKED__     __attribute__((__packed__))
#define __KAL_ATTRIB_ALIGN_4__    __aligned(4)

#ifndef BIT
#define BIT(n)              ((u32)1UL << (n))
#endif

#ifndef BITS
/* bits range: for example BITS(16,23) = 0xFF0000
 *   ==>  (BIT(m)-1)   = 0x0000FFFF     ~(BIT(m)-1)   => 0xFFFF0000
 *   ==>  (BIT(n+1)-1) = 0x00FFFFFF
 */
#define BITS(m, n)          (~(BIT(m) - 1) & ((BIT(n) - 1) | BIT(n)))
#endif

/* This macro returns the byte offset of a named field in a known structure
 *   type.
 *   _type - structure name,
 *   _field - field name of the structure
 */
#ifndef OFFSET_OF
#define OFFSET_OF(_type, _field)    ((unsigned long)&(((_type *)0)->_field))
#endif

/* This macro returns the base address of an instance of a structure
 * given the type of the structure and the address of a field within the
 * containing structure.
 * _addrOfField - address of current field of the structure,
 * _type - structure name,
 * _field - field name of the structure
 */
#ifndef ENTRY_OF
#define ENTRY_OF(_addrOfField, _type, _field) \
    ((_type *)((s8 *)(_addrOfField) - (s8 *)OFFSET_OF(_type, _field)))
#endif

/* This macro align the input value to the DW boundary.
 * _value - value need to check
 */
#ifndef ALIGN_4
#define ALIGN_4(_value)       (((_value) + 3) & ~3u)
#endif

/* This macro check the DW alignment of the input value.
 * _value - value of address need to check
 */
#ifndef IS_ALIGN_4
#define IS_ALIGN_4(_value)    (((_value) & 0x3) ? false : true)
#endif

#ifndef IS_NOT_ALIGN_4
#define IS_NOT_ALIGN_4(_value)    (((_value) & 0x3) ? true : false)
#endif

/* This macro evaluate the input length in unit of Double Word(4 Bytes).
 * _value - value in unit of Byte, output will round up to DW boundary.
 */
#ifndef BYTE_TO_DWORD
#define BYTE_TO_DWORD(_value)     ((_value + 3) >> 2)
#endif

/* This macro evaluate the input length in unit of Byte.
 * _value - value in unit of DW, output is in unit of Byte.
 */
#ifndef DWORD_TO_BYTE
#define DWORD_TO_BYTE(_value)     ((_value) << 2)
#endif

#define CONST_NTOHS(_x)       ntohs(_x)

#define CONST_HTONS(_x)       htons(_x)

#define NTOHS(_x)         ntohs(_x)

#define HTONS(_x)         htons(_x)

#define NTOHL(_x)         ntohl(_x)

#define HTONL(_x)         htonl(_x)

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
