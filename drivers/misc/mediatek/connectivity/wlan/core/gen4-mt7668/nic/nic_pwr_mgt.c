// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "nic_pwr_mgt.c"
 *    \brief  In this file we define the STATE and EVENT for Power Management
 * FSM.
 *
 *    The SCAN FSM is responsible for performing SCAN behavior when the Arbiter
 * enter ARB_STATE_SCAN. The STATE and EVENT for SCAN FSM are defined here with
 * detail description.
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
void nicpmWakeUpWiFi(IN P_ADAPTER_T prAdapter)
{
    if (!nicVerifyChipID(prAdapter)) {
        DBGLOG(INIT, ERROR, "Chip id verify error!\n");
        return;
    }
    HAL_WAKE_UP_WIFI(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to process the POWER ON procedure.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void nicpmSetFWOwn(IN P_ADAPTER_T prAdapter, IN u8 fgEnableGlobalInt)
{
    halSetFWOwn(prAdapter, fgEnableGlobalInt);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to process the POWER OFF procedure.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
u8 nicpmSetDriverOwn(IN P_ADAPTER_T prAdapter)
{
    return halSetDriverOwn(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set ACPI power mode to D0.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
u8 nicpmSetAcpiPowerD0(IN P_ADAPTER_T prAdapter)
{
    return true;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is used to set ACPI power mode to D3.
 *
 * @param prAdapter pointer to the Adapter handler
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
u8 nicpmSetAcpiPowerD3(IN P_ADAPTER_T prAdapter)
{
    ASSERT(prAdapter);

    return true;
}
