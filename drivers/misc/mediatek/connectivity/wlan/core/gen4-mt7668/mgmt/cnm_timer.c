// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "cnm_timer.c"
 *    \brief
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

#include "precomp.h"

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

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to initialize a root timer.
 *
 * \param[in] prAdapter
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmTimerInitialize(IN P_ADAPTER_T prAdapter)
{
    P_ROOT_TIMER prRootTimer;

    KAL_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);

    prRootTimer = &prAdapter->rRootTimer;

    /* Note: glue layer have configured timer */

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);
    LINK_INITIALIZE(&prRootTimer->rLinkHead);
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to destroy a root timer.
 *        When WIFI is off, the token shall be returned back to system.
 *
 * \param[in]
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmTimerDestroy(IN P_ADAPTER_T prAdapter)
{
    P_ROOT_TIMER prRootTimer;

    KAL_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);

    prRootTimer = &prAdapter->rRootTimer;

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);
    LINK_INITIALIZE(&prRootTimer->rLinkHead);
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);

    /* Note: glue layer will be responsible for timer destruction */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to initialize a timer.
 *
 * \param[in] prTimer Pointer to a timer structure.
 * \param[in] pfnFunc Pointer to the call back function.
 * \param[in] u4Data Parameter for call back function.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmTimerInitTimer(IN P_ADAPTER_T prAdapter, IN P_TIMER_T prTimer,
                       IN PFN_MGMT_TIMEOUT_FUNC pfFunc,
                       IN unsigned long ulDataPtr)
{
    ASSERT(prAdapter);

    ASSERT(prTimer);

#if DBG
    /* Note: NULL function pointer is permitted for HEM POWER */
    if (pfFunc == NULL) {
        DBGLOG(CNM, WARN, "Init timer with NULL callback function!\n");
    }
#endif

    ASSERT(prAdapter->rRootTimer.rLinkHead.prNext);
    {
        P_LINK_T prTimerList;
        P_LINK_ENTRY_T prLinkEntry;
        P_TIMER_T prPendingTimer;

        prTimerList = &(prAdapter->rRootTimer.rLinkHead);

        LINK_FOR_EACH(prLinkEntry, prTimerList) {
            prPendingTimer =
                LINK_ENTRY(prLinkEntry, TIMER_T, rLinkEntry);
            ASSERT(prPendingTimer);
            ASSERT(prPendingTimer != prTimer);
        }
    }

    LINK_ENTRY_INITIALIZE(&prTimer->rLinkEntry);

    prTimer->pfMgmtTimeOutFunc = pfFunc;
    prTimer->ulDataPtr = ulDataPtr;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to stop a timer.
 *
 * \param[in] prTimer Pointer to a timer structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static void cnmTimerStopTimer_impl(IN P_ADAPTER_T prAdapter,
                                   IN P_TIMER_T prTimer,
                                   IN u8 fgAcquireSpinlock)
{
    P_ROOT_TIMER prRootTimer;

    KAL_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);
    ASSERT(prTimer);

    prRootTimer = &prAdapter->rRootTimer;

    if (fgAcquireSpinlock) {
        KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);
    }

    if (timerPendingTimer(prTimer)) {
        LINK_REMOVE_KNOWN_ENTRY(&prRootTimer->rLinkHead,
                                &prTimer->rLinkEntry);

        /* Reduce dummy timeout for power saving, especially HIF
         * activity. If two or more timers exist and being removed timer
         * is smallest, this dummy timeout will still happen, but it is
         * OK.
         */
        if (LINK_IS_EMPTY(&prRootTimer->rLinkHead)) {
            kalCancelTimer(prAdapter->prGlueInfo);
        }
    }

    if (fgAcquireSpinlock) {
        KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to stop a timer.
 *
 * \param[in] prTimer Pointer to a timer structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmTimerStopTimer(IN P_ADAPTER_T prAdapter, IN P_TIMER_T prTimer)
{
    ASSERT(prAdapter);
    ASSERT(prTimer);

    cnmTimerStopTimer_impl(prAdapter, prTimer, true);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to start a timer with wake_lock.
 *
 * \param[in] prTimer Pointer to a timer structure.
 * \param[in] u4TimeoutMs Timeout to issue the timer and call back function
 *                        (unit: ms).
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmTimerStartTimer(IN P_ADAPTER_T prAdapter, IN P_TIMER_T prTimer,
                        IN u32 u4TimeoutMs)
{
    P_ROOT_TIMER prRootTimer;
    P_LINK_T prTimerList;
    u32 rExpiredSysTime, rTimeoutSystime;

    KAL_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);
    ASSERT(prTimer);

    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);

    prRootTimer = &prAdapter->rRootTimer;
    prTimerList = &prRootTimer->rLinkHead;

    /* If timeout interval is larger than 1 minute, the mod value is set
     * to the timeout value first, then per minutue.
     */
    if (u4TimeoutMs > MSEC_PER_MIN) {
        ASSERT(u4TimeoutMs <= ((u32)0xFFFF * MSEC_PER_MIN));

        prTimer->u2Minutes = (u16)(u4TimeoutMs / MSEC_PER_MIN);
        u4TimeoutMs -= (prTimer->u2Minutes * MSEC_PER_MIN);
        if (u4TimeoutMs == 0) {
            u4TimeoutMs = MSEC_PER_MIN;
            prTimer->u2Minutes--;
        }
    } else {
        prTimer->u2Minutes = 0;
    }

    /* The assertion check if MSEC_TO_SYSTIME() may be overflow. */
    ASSERT(u4TimeoutMs < (((u32)0x80000000 - MSEC_PER_SEC) / KAL_HZ));
    rTimeoutSystime = MSEC_TO_SYSTIME(u4TimeoutMs);
    if (rTimeoutSystime == 0) {
        rTimeoutSystime = 1;
    }
    rExpiredSysTime = kalGetTimeTick() + rTimeoutSystime;

    /* If no timer pending or the fast time interval is used. */
    if (LINK_IS_EMPTY(prTimerList) ||
        TIME_BEFORE(rExpiredSysTime, prRootTimer->rNextExpiredSysTime)) {
        prRootTimer->rNextExpiredSysTime = rExpiredSysTime;
    }

    /* Add this timer to checking list */
    prTimer->rExpiredSysTime = rExpiredSysTime;

    if (!timerPendingTimer(prTimer)) {
        LINK_INSERT_TAIL(prTimerList, &prTimer->rLinkEntry);
    }

    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to check the timer list.
 *
 * \param[in]
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmTimerDoTimeOutCheck(IN P_ADAPTER_T prAdapter)
{
    P_ROOT_TIMER prRootTimer;
    P_LINK_T prTimerList;
    P_LINK_ENTRY_T prLinkEntry;
    P_TIMER_T prTimer;
    u32 rCurSysTime;
    PFN_MGMT_TIMEOUT_FUNC pfMgmtTimeOutFunc;
    unsigned long ulTimeoutDataPtr;
    u8 fgNeedWakeLock;

    KAL_SPIN_LOCK_DECLARATION();

    ASSERT(prAdapter);

    /* acquire spin lock */
    KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);

    prRootTimer = &prAdapter->rRootTimer;
    prTimerList = &prRootTimer->rLinkHead;

    rCurSysTime = kalGetTimeTick();

    /* Set the permitted max timeout value for new one */
    prRootTimer->rNextExpiredSysTime =
        rCurSysTime + MGMT_MAX_TIMEOUT_INTERVAL;

    LINK_FOR_EACH(prLinkEntry, prTimerList) {
        prTimer = LINK_ENTRY(prLinkEntry, TIMER_T, rLinkEntry);
        ASSERT(prTimer);

        /* Check if this entry is timeout. */
        if (!TIME_BEFORE(rCurSysTime, prTimer->rExpiredSysTime)) {
            cnmTimerStopTimer_impl(prAdapter, prTimer, false);

            pfMgmtTimeOutFunc = prTimer->pfMgmtTimeOutFunc;
            ulTimeoutDataPtr = prTimer->ulDataPtr;

            if (prTimer->u2Minutes > 0) {
                prTimer->u2Minutes--;
                prTimer->rExpiredSysTime =
                    rCurSysTime +
                    MSEC_TO_SYSTIME(MSEC_PER_MIN);
                LINK_INSERT_TAIL(prTimerList,
                                 &prTimer->rLinkEntry);
            } else if (pfMgmtTimeOutFunc) {
                KAL_RELEASE_SPIN_LOCK(prAdapter,
                                      SPIN_LOCK_TIMER);
                (pfMgmtTimeOutFunc)(prAdapter,
                                    ulTimeoutDataPtr);
                KAL_ACQUIRE_SPIN_LOCK(prAdapter,
                                      SPIN_LOCK_TIMER);
            }

            /* Search entire list again because of nest del and add
             * timers and current MGMT_TIMER could be volatile after
             * stopped
             */
            prLinkEntry = (P_LINK_ENTRY_T)prTimerList;

            prRootTimer->rNextExpiredSysTime =
                rCurSysTime + MGMT_MAX_TIMEOUT_INTERVAL;
        } else if (TIME_BEFORE(prTimer->rExpiredSysTime,
                               prRootTimer->rNextExpiredSysTime)) {
            prRootTimer->rNextExpiredSysTime =
                prTimer->rExpiredSysTime;
        }
    }  /* end of for loop */

    /* Setup the prNext timeout event. It is possible the timer was already
     * set in the above timeout callback function.
     */
    fgNeedWakeLock = false;
    if (!LINK_IS_EMPTY(prTimerList)) {
        ASSERT(TIME_AFTER(prRootTimer->rNextExpiredSysTime,
                          rCurSysTime));
    }

    /* release spin lock */
    KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);
}
