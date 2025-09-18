/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   link.h
 *    \brief  Definition for simple doubly linked list operations.
 *
 *    In this file we define the simple doubly linked list data structure and
 * its operation MACROs and INLINE functions.
 */

#ifndef _LINK_H
#define _LINK_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "gl_typedef.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/* May cause page fault & unalignment issue (data abort) */
#define INVALID_LINK_POISON1    ((void *)0x00100101)

/* Used to verify that nonbody uses non-initialized link entries. */
#define INVALID_LINK_POISON2    ((void *)0x00100201)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* Simple Doubly Linked List Structures - Entry Part */
typedef struct _LINK_ENTRY_T {
    struct _LINK_ENTRY_T *prNext, *prPrev;
} LINK_ENTRY_T, *P_LINK_ENTRY_T;

/* Simple Doubly Linked List Structures - List Part */
typedef struct _LINK_T {
    P_LINK_ENTRY_T prNext;
    P_LINK_ENTRY_T prPrev;
    u32 u4NumElem;
} LINK_T, *P_LINK_T;

/* Support AP Selection */
typedef struct _LINK_MGMT_T {
    LINK_T rUsingLink;
    LINK_T rFreeLink;
} LINK_MGMT_T, *P_LINK_MGMT_T;

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

#define LINK_INITIALIZE(prLink)                                  \
    do {                                                         \
        ((P_LINK_T)(prLink))->prNext = (P_LINK_ENTRY_T)(prLink); \
        ((P_LINK_T)(prLink))->prPrev = (P_LINK_ENTRY_T)(prLink); \
        ((P_LINK_T)(prLink))->u4NumElem = 0;                     \
    } while (0)

#define LINK_ENTRY_INITIALIZE(prEntry)                              \
    do {                                                            \
        ((P_LINK_ENTRY_T)(prEntry))->prNext = (P_LINK_ENTRY_T)NULL; \
        ((P_LINK_ENTRY_T)(prEntry))->prPrev = (P_LINK_ENTRY_T)NULL; \
    } while (0)

#define LINK_ENTRY_INVALID(prEntry)               \
    do {                                          \
        ((P_LINK_ENTRY_T)(prEntry))->prNext =     \
            (P_LINK_ENTRY_T)INVALID_LINK_POISON1; \
        ((P_LINK_ENTRY_T)(prEntry))->prPrev =     \
            (P_LINK_ENTRY_T)INVALID_LINK_POISON2; \
    } while (0)

/* Support AP Selection */
#define LINK_MGMT_INIT(prLinkMgmt)                                 \
    do {                                                           \
        LINK_INITIALIZE(&((P_LINK_MGMT_T)prLinkMgmt)->rUsingLink); \
        LINK_INITIALIZE(&((P_LINK_MGMT_T)prLinkMgmt)->rFreeLink);  \
    } while (0)

#define LINK_MGMT_GET_ENTRY(prLinkMgmt, prEntry, EntryType, memType) \
    do {                                                             \
        LINK_REMOVE_HEAD(&((P_LINK_MGMT_T)prLinkMgmt)->rFreeLink,    \
                         prEntry, EntryType *);                      \
        if (!prEntry)                                                \
        prEntry = kalMemAlloc(sizeof(EntryType), memType);           \
        if (prEntry) {                                               \
            kalMemZero(prEntry, sizeof(EntryType));                  \
            LINK_INSERT_TAIL(                                        \
                &((P_LINK_MGMT_T)prLinkMgmt)->rUsingLink,            \
                &prEntry->rLinkEntry);                               \
        }                                                            \
    } while (0)

#define LINK_MGMT_UNINIT(prLinkMgmt, EntryType, memType)               \
    do {                                                               \
        EntryType *prEntry = NULL;                                     \
        P_LINK_T prFreeList = &((P_LINK_MGMT_T)prLinkMgmt)->rFreeLink; \
        P_LINK_T prUsingList =                                         \
            &((P_LINK_MGMT_T)prLinkMgmt)->rUsingLink;                  \
        LINK_REMOVE_HEAD(prFreeList, prEntry, EntryType *);            \
        while (prEntry) {                                              \
            kalMemFree(prEntry, memType, sizeof(EntryType));           \
            LINK_REMOVE_HEAD(prFreeList, prEntry, EntryType *);        \
        }                                                              \
        LINK_REMOVE_HEAD(prUsingList, prEntry, EntryType *);           \
        while (prEntry) {                                              \
            kalMemFree(prEntry, memType, sizeof(EntryType));           \
            LINK_REMOVE_HEAD(prUsingList, prEntry, EntryType *);       \
        }                                                              \
    } while (0)
/* end Support AP Selection */

#define LINK_IS_EMPTY(prLink) \
    (((P_LINK_T)(prLink))->prNext == (P_LINK_ENTRY_T)(prLink))

/* NOTE: We should do memory zero before any LINK been initiated, so we can
 * check if it is valid before parsing the LINK.
 */
#define LINK_IS_INVALID(prLink) \
    (((P_LINK_T)(prLink))->prNext == (P_LINK_ENTRY_T)NULL)

#define LINK_IS_VALID(prLink) \
    (((P_LINK_T)(prLink))->prNext != (P_LINK_ENTRY_T)NULL)

#define LINK_ENTRY(ptr, type, member)    ENTRY_OF(ptr, type, member)

/* Insert an entry into a link list's head */
#define LINK_INSERT_HEAD(prLink, prEntry) \
    {                                     \
        linkAdd(prEntry, prLink);         \
        ((prLink)->u4NumElem)++;          \
    }

/* Append an entry into a link list's tail */
#define LINK_INSERT_TAIL(prLink, prEntry) \
    {                                     \
        linkAddTail(prEntry, prLink);     \
        ((prLink)->u4NumElem)++;          \
    }

/* Peek head entry, but keep still in link list */
#define LINK_PEEK_HEAD(prLink, _type, _member) \
    (LINK_IS_EMPTY(prLink) ? NULL :            \
     LINK_ENTRY((prLink)->prNext, _type, _member))

/* Peek tail entry, but keep still in link list */
#define LINK_PEEK_TAIL(prLink, _type, _member) \
    (LINK_IS_EMPTY(prLink) ? NULL :            \
     LINK_ENTRY((prLink)->prPrev, _type, _member))

/* Get first entry from a link list */
/* NOTE: We assume the link entry located at the beginning of "prEntry Type",
 * so that we can cast the link entry to other data type without doubts.
 * And this macro also decrease the total entry count at the same time.
 */
#define LINK_REMOVE_HEAD(prLink, prEntry, _P_TYPE)             \
    {                                                          \
        ASSERT(prLink);                                        \
        if (LINK_IS_EMPTY(prLink)) {                           \
            prEntry = (_P_TYPE)NULL;                           \
        } else {                                               \
            prEntry = (_P_TYPE)(((P_LINK_T)(prLink))->prNext); \
            linkDel((P_LINK_ENTRY_T)prEntry);                  \
            ((prLink)->u4NumElem)--;                           \
        }                                                      \
    }

/* Assume the link entry located at the beginning of prEntry Type.
 * And also decrease the total entry count.
 */
#define LINK_REMOVE_KNOWN_ENTRY(prLink, prEntry) \
    {                                            \
        ASSERT(prLink);                          \
        ASSERT(prEntry);                         \
        linkDel((P_LINK_ENTRY_T)prEntry);        \
        ((prLink)->u4NumElem)--;                 \
    }

/* Merge prSrcLink to prDstLink and put prSrcLink ahead of prDstLink */
#define LINK_MERGE_TO_HEAD(prDstLink, prSrcLink)              \
    {                                                         \
        if (!LINK_IS_EMPTY(prSrcLink)) {                      \
            linkMergeToHead((P_LINK_T)prDstLink,              \
                            (P_LINK_T)prSrcLink);             \
            (prDstLink)->u4NumElem += (prSrcLink)->u4NumElem; \
            LINK_INITIALIZE(prSrcLink);                       \
        }                                                     \
    }

/* Merge prSrcLink to prDstLink and put prSrcLink at tail */
#define LINK_MERGE_TO_TAIL(prDstLink, prSrcLink)              \
    {                                                         \
        if (!LINK_IS_EMPTY(prSrcLink)) {                      \
            linkMergeToTail((P_LINK_T)prDstLink,              \
                            (P_LINK_T)prSrcLink);             \
            (prDstLink)->u4NumElem += (prSrcLink)->u4NumElem; \
            LINK_INITIALIZE(prSrcLink);                       \
        }                                                     \
    }

/* Iterate over a link list */
#define LINK_FOR_EACH(prEntry, prLink)                                    \
    for (prEntry = (prLink)->prNext; prEntry != (P_LINK_ENTRY_T)(prLink); \
         prEntry = (P_LINK_ENTRY_T)prEntry->prNext)

/* Iterate over a link list backwards */
#define LINK_FOR_EACH_PREV(prEntry, prLink)                               \
    for (prEntry = (prLink)->prPrev; prEntry != (P_LINK_ENTRY_T)(prLink); \
         prEntry = (P_LINK_ENTRY_T)prEntry->prPrev)

/* Iterate over a link list safe against removal of link entry */
#define LINK_FOR_EACH_SAFE(prEntry, prNextEntry, prLink)            \
    for (prEntry = (prLink)->prNext, prNextEntry = prEntry->prNext; \
         prEntry != (P_LINK_ENTRY_T)(prLink);                       \
         prEntry = prNextEntry, prNextEntry = prEntry->prNext)

/* Iterate over a link list of given type */
#define LINK_FOR_EACH_ENTRY(prObj, prLink, rMember, _TYPE)     \
    for (prObj = LINK_ENTRY((prLink)->prNext, _TYPE, rMember); \
         &prObj->rMember != (P_LINK_ENTRY_T)(prLink);          \
         prObj = LINK_ENTRY(prObj->rMember.prNext, _TYPE, rMember))

/* Iterate backwards over a link list of given type */
#define LINK_FOR_EACH_ENTRY_PREV(prObj, prLink, rMember, _TYPE) \
    for (prObj = LINK_ENTRY((prLink)->prPrev, _TYPE, rMember);  \
         &prObj->rMember != (P_LINK_ENTRY_T)(prLink);           \
         prObj = LINK_ENTRY(prObj->rMember.prPrev, _TYPE, rMember))

/* Iterate over a link list of given type safe against removal of link entry */
#define LINK_FOR_EACH_ENTRY_SAFE(prObj, prNextObj, prLink, rMember, _TYPE) \
    for (prObj = LINK_ENTRY((prLink)->prNext, _TYPE, rMember),             \
         prNextObj = LINK_ENTRY(prObj->rMember.prNext, _TYPE, rMember);    \
         &prObj->rMember != (P_LINK_ENTRY_T)(prLink); prObj = prNextObj,   \
         prNextObj = LINK_ENTRY(prNextObj->rMember.prNext, _TYPE, rMember))

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is only for internal link list manipulation.
 *
 * \param[in] prNew  Pointer of new link head
 * \param[in] prPrev Pointer of previous link head
 * \param[in] prNext Pointer of next link head
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void __linkAdd(IN P_LINK_ENTRY_T prNew,
                                     IN P_LINK_ENTRY_T prPrev,
                                     IN P_LINK_ENTRY_T prNext)
{
    prNext->prPrev = prNew;
    prNew->prNext = prNext;
    prNew->prPrev = prPrev;
    prPrev->prNext = prNew;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will add a new entry after the specified link head.
 *
 * \param[in] prNew  New entry to be added
 * \param[in] prHead Specified link head to add it after
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void linkAdd(IN P_LINK_ENTRY_T prNew, IN P_LINK_T prLink)
{
    __linkAdd(prNew, (P_LINK_ENTRY_T)prLink, prLink->prNext);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will add a new entry before the specified link head.
 *
 * \param[in] prNew  New entry to be added
 * \param[in] prHead Specified link head to add it before
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void linkAddTail(IN P_LINK_ENTRY_T prNew,
                                       IN P_LINK_T prLink)
{
    __linkAdd(prNew, prLink->prPrev, (P_LINK_ENTRY_T)prLink);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is only for internal link list manipulation.
 *
 * \param[in] prPrev Pointer of previous link head
 * \param[in] prNext Pointer of next link head
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void __linkDel(IN P_LINK_ENTRY_T prPrev,
                                     IN P_LINK_ENTRY_T prNext)
{
    prNext->prPrev = prPrev;
    prPrev->prNext = prNext;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will delete a specified entry from link list.
 *        NOTE: the entry is in an initial state.
 *
 * \param prEntry    Specified link head(entry)
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void linkDel(IN P_LINK_ENTRY_T prEntry)
{
    __linkDel(prEntry->prPrev, prEntry->prNext);

    LINK_ENTRY_INITIALIZE(prEntry);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will delete a specified entry from link list and then
 * add it after the specified link head.
 *
 * \param[in] prEntry        Specified link head(entry)
 * \param[in] prOtherHead    Another link head to add it after
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void linkMove(IN P_LINK_ENTRY_T prEntry,
                                    IN P_LINK_T prLink)
{
    __linkDel(prEntry->prPrev, prEntry->prNext);
    linkAdd(prEntry, prLink);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will delete a specified entry from link list and then
 * add it before the specified link head.
 *
 * \param[in] prEntry        Specified link head(entry)
 * \param[in] prOtherHead    Another link head to add it before
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void linkMoveTail(IN P_LINK_ENTRY_T prEntry,
                                        IN P_LINK_T prLink)
{
    __linkDel(prEntry->prPrev, prEntry->prNext);
    linkAddTail(prEntry, prLink);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will merge source link to the tail of destination link.
 *
 * \param[in] prDst    destination link
 * \param[in] prSrc    source link
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void linkMergeToTail(P_LINK_T prDst, P_LINK_T prSrc)
{
    prSrc->prNext->prPrev = prDst->prPrev;
    prSrc->prPrev->prNext = (P_LINK_ENTRY_T)prDst;
    prDst->prPrev->prNext = prSrc->prNext;
    prDst->prPrev = prSrc->prPrev;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will merge source link to the head of destination link.
 *
 * \param[in] prDst    destination link
 * \param[in] prSrc    source link
 *
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static __KAL_INLINE__ void linkMergeToHead(P_LINK_T prDst, P_LINK_T prSrc)
{
    prSrc->prNext->prPrev = (P_LINK_ENTRY_T)prDst;
    prSrc->prPrev->prNext = prDst->prNext;
    prDst->prNext->prPrev = prSrc->prPrev;
    prDst->prNext = prSrc->prNext;
}

#endif
