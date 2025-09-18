// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file wlan_bow.c
 *    \brief This file contains the Wi-Fi Direct commands processing routines
 * for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */

/******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "precomp.h"
#include "gl_p2p_ioctl.h"

/******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief command packet generation utility
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] ucCID              Command ID
 * \param[in] fgSetQuery         Set or Query
 * \param[in] fgNeedResp         Need for response
 * \param[in] pfCmdDoneHandler   Function pointer when command is done
 * \param[in] u4SetQueryInfoLen  The length of the set/query buffer
 * \param[in] pucInfoBuffer      Pointer to set/query buffer
 *
 *
 * \retval WLAN_STATUS_PENDING
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidSendSetQueryP2PCmd(IN P_ADAPTER_T prAdapter, IN u8 ucCID, IN u8 ucBssIdx,
                          IN u8 fgSetQuery, IN u8 fgNeedResp, IN u8 fgIsOid,
                          IN PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
                          IN PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
                          IN u32 u4SetQueryInfoLen, IN u8 *pucInfoBuffer,
                          OUT void *pvSetQueryBuffer,
                          IN u32 u4SetQueryBufferLen)
{
    P_GLUE_INFO_T prGlueInfo;
    P_CMD_INFO_T prCmdInfo;
    P_WIFI_CMD_T prWifiCmd;
    u8 ucCmdSeqNum;

    ASSERT(prAdapter);

    prGlueInfo = prAdapter->prGlueInfo;
    ASSERT(prGlueInfo);

    DEBUGFUNC("wlanoidSendSetQueryP2PCmd");
    DBGLOG(REQ, TRACE, "Command ID = 0x%08X\n", ucCID);

    prCmdInfo = cmdBufAllocateCmdInfo(prAdapter,
                                      (CMD_HDR_SIZE + u4SetQueryInfoLen));

    if (!prCmdInfo) {
        DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
        return WLAN_STATUS_FAILURE;
    }
    /* increase command sequence number */
    ucCmdSeqNum = nicIncreaseCmdSeqNum(prAdapter);
    DBGLOG(REQ, TRACE, "ucCmdSeqNum =%d\n", ucCmdSeqNum);

    /* Setup common CMD Info Packet */
    prCmdInfo->eCmdType = COMMAND_TYPE_NETWORK_IOCTL;
    prCmdInfo->u2InfoBufLen = (u16)(CMD_HDR_SIZE + u4SetQueryInfoLen);
    prCmdInfo->pfCmdDoneHandler = pfCmdDoneHandler;
    prCmdInfo->pfCmdTimeoutHandler = pfCmdTimeoutHandler;
    prCmdInfo->fgIsOid = fgIsOid;
    prCmdInfo->ucCID = ucCID;
    prCmdInfo->fgSetQuery = fgSetQuery;
    prCmdInfo->fgNeedResp = fgNeedResp;
    prCmdInfo->ucCmdSeqNum = ucCmdSeqNum;
    prCmdInfo->u4SetInfoLen = u4SetQueryInfoLen;
    prCmdInfo->pvInformationBuffer = pvSetQueryBuffer;
    prCmdInfo->u4InformationBufferLength = u4SetQueryBufferLen;

    /* Setup WIFI_CMD_T (no payload) */
    prWifiCmd = (P_WIFI_CMD_T)(prCmdInfo->pucInfoBuffer);
    prWifiCmd->u2TxByteCount = prCmdInfo->u2InfoBufLen;
    prWifiCmd->ucCID = prCmdInfo->ucCID;
    prWifiCmd->ucSetQuery = prCmdInfo->fgSetQuery;
    prWifiCmd->ucSeqNum = prCmdInfo->ucCmdSeqNum;

    if (u4SetQueryInfoLen > 0 && pucInfoBuffer != NULL) {
        kalMemCopy(prWifiCmd->aucBuffer, pucInfoBuffer,
                   u4SetQueryInfoLen);
    }
    /* insert into prCmdQueue */
    kalEnqueueCommand(prGlueInfo, (P_QUE_ENTRY_T)prCmdInfo);

    /* wakeup txServiceThread later */
    GLUE_SET_EVENT(prGlueInfo);
    return WLAN_STATUS_PENDING;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Setting the IP address for pattern search function.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \return WLAN_STATUS_SUCCESS
 * \return WLAN_STATUS_ADAPTER_NOT_READY
 * \return WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidSetP2pNetworkAddress(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                            IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 i, j;
    P_CMD_SET_NETWORK_ADDRESS_LIST prCmdNetworkAddressList;
    P_PARAM_NETWORK_ADDRESS_LIST prNetworkAddressList =
        (P_PARAM_NETWORK_ADDRESS_LIST)pvSetBuffer;
    P_PARAM_NETWORK_ADDRESS prNetworkAddress;
    P_PARAM_NETWORK_ADDRESS_IP prNetAddrIp;
    u32 u4IpAddressCount, u4CmdSize;

    DEBUGFUNC("wlanoidSetP2pNetworkAddress");
    DBGLOG(INIT, TRACE, "\n");

    ASSERT(prAdapter);
    ASSERT(pu4SetInfoLen);

    *pu4SetInfoLen = 4;

    if (u4SetBufferLen < sizeof(PARAM_NETWORK_ADDRESS_LIST)) {
        return WLAN_STATUS_INVALID_DATA;
    }

    *pu4SetInfoLen = 0;
    u4IpAddressCount = 0;

    prNetworkAddress = prNetworkAddressList->arAddress;
    for (i = 0; i < prNetworkAddressList->u4AddressCount; i++) {
        if (prNetworkAddress->u2AddressType ==
            PARAM_PROTOCOL_ID_TCP_IP &&
            prNetworkAddress->u2AddressLength ==
            sizeof(PARAM_NETWORK_ADDRESS_IP)) {
            u4IpAddressCount++;
        }

        prNetworkAddress =
            (P_PARAM_NETWORK_ADDRESS)((unsigned long)
                                      prNetworkAddress +
                                      (unsigned long)(
                                          prNetworkAddress
                                          ->
                                          u2AddressLength +
                                          OFFSET_OF(
                                              PARAM_NETWORK_ADDRESS,
                                              aucAddress)));
    }

    /* construct payload of command packet */
    u4CmdSize = OFFSET_OF(CMD_SET_NETWORK_ADDRESS_LIST, arNetAddress) +
                sizeof(IPV4_NETWORK_ADDRESS) * u4IpAddressCount;

    prCmdNetworkAddressList = (P_CMD_SET_NETWORK_ADDRESS_LIST)kalMemAlloc(
        u4CmdSize, VIR_MEM_TYPE);

    if (prCmdNetworkAddressList == NULL) {
        return WLAN_STATUS_FAILURE;
    }

    kalMemZero(prCmdNetworkAddressList, u4CmdSize);

    /* fill P_CMD_SET_NETWORK_ADDRESS_LIST */
    prCmdNetworkAddressList->ucBssIndex = prNetworkAddressList->ucBssIdx;
    prCmdNetworkAddressList->ucAddressCount = (u8)u4IpAddressCount;
    prNetworkAddress = prNetworkAddressList->arAddress;
    for (i = 0, j = 0; i < prNetworkAddressList->u4AddressCount; i++) {
        if (prNetworkAddress->u2AddressType ==
            PARAM_PROTOCOL_ID_TCP_IP &&
            prNetworkAddress->u2AddressLength ==
            sizeof(PARAM_NETWORK_ADDRESS_IP)) {
            prNetAddrIp = (P_PARAM_NETWORK_ADDRESS_IP)
                          prNetworkAddress->aucAddress;

            kalMemCopy(prCmdNetworkAddressList->arNetAddress[j]
                       .aucIpAddr,
                       &(prNetAddrIp->in_addr), sizeof(u32));

            j++;
        }

        prNetworkAddress =
            (P_PARAM_NETWORK_ADDRESS)((unsigned long)
                                      prNetworkAddress +
                                      (unsigned long)(
                                          prNetworkAddress
                                          ->
                                          u2AddressLength +
                                          OFFSET_OF(
                                              PARAM_NETWORK_ADDRESS,
                                              aucAddress)));
    }

    rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_IP_ADDRESS, true,
                                  false, true, nicCmdEventSetIpAddress,
                                  nicOidCmdTimeoutCommon, u4CmdSize,
                                  (u8 *)prCmdNetworkAddressList,
                                  pvSetBuffer, u4SetBufferLen);

    kalMemFree(prCmdNetworkAddressList, VIR_MEM_TYPE, u4CmdSize);
    return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to query the power save profile.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidQueryP2pPowerSaveProfile(IN P_ADAPTER_T prAdapter,
                                IN void *pvQueryBuffer, IN u32 u4QueryBufferLen,
                                OUT u32 *pu4QueryInfoLen)
{
    DEBUGFUNC("wlanoidQueryP2pPowerSaveProfile");

    ASSERT(prAdapter);
    ASSERT(pu4QueryInfoLen);

    if (u4QueryBufferLen != 0) {
        ASSERT(pvQueryBuffer);
        /* TODO: FIXME */
        /**(PPARAM_POWER_MODE) pvQueryBuffer =
         * (PARAM_POWER_MODE)(prAdapter->rWlanInfo.arPowerSaveMode[P2P_DEV_BSS_INDEX].ucPsProfile);
         */
        /**pu4QueryInfoLen = sizeof(PARAM_POWER_MODE); */
    }

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set the power save profile.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidSetP2pPowerSaveProfile(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                              IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    WLAN_STATUS status;
    PARAM_POWER_MODE ePowerMode;

    DEBUGFUNC("wlanoidSetP2pPowerSaveProfile");

    ASSERT(prAdapter);
    ASSERT(pu4SetInfoLen);

    *pu4SetInfoLen = sizeof(PARAM_POWER_MODE);
    if (u4SetBufferLen < sizeof(PARAM_POWER_MODE)) {
        DBGLOG(REQ, WARN, "Invalid length %ld\n", u4SetBufferLen);
        return WLAN_STATUS_INVALID_LENGTH;
    } else if (*(PPARAM_POWER_MODE)pvSetBuffer >= Param_PowerModeMax) {
        DBGLOG(REQ, WARN, "Invalid power mode %d\n",
               *(PPARAM_POWER_MODE)pvSetBuffer);
        return WLAN_STATUS_INVALID_DATA;
    }

    ePowerMode = *(PPARAM_POWER_MODE)pvSetBuffer;

    if (prAdapter->fgEnCtiaPowerMode) {
        if (ePowerMode == Param_PowerModeCAM) {
            /*Todo::  Nothing */
            /*Todo::  Nothing */
        } else {
            /* User setting to PS mode (Param_PowerModeMAX_PSP or
             * Param_PowerModeFast_PSP) */

            if (prAdapter->u4CtiaPowerMode == 0) {
                /* force to keep in CAM mode */
                ePowerMode = Param_PowerModeCAM;
            } else if (prAdapter->u4CtiaPowerMode == 1) {
                ePowerMode = Param_PowerModeMAX_PSP;
            } else if (prAdapter->u4CtiaPowerMode == 2) {
                ePowerMode = Param_PowerModeFast_PSP;
            }
        }
    }

    status = nicConfigPowerSaveProfile(prAdapter,
                                       P2P_DEV_BSS_INDEX,  /* TODO:
                                                            * FIXME
                                                            */
                                       ePowerMode, true);
    return status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set the power save profile.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidSetP2pSetNetworkAddress(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                               IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
    u32 i, j;
    P_CMD_SET_NETWORK_ADDRESS_LIST prCmdNetworkAddressList;
    P_PARAM_NETWORK_ADDRESS_LIST prNetworkAddressList =
        (P_PARAM_NETWORK_ADDRESS_LIST)pvSetBuffer;
    P_PARAM_NETWORK_ADDRESS prNetworkAddress;
    P_PARAM_NETWORK_ADDRESS_IP prNetAddrIp;
    u32 u4IpAddressCount, u4CmdSize;
    u8 *pucBuf = (u8 *)pvSetBuffer;

    DEBUGFUNC("wlanoidSetP2pSetNetworkAddress");
    DBGLOG(INIT, TRACE, "\n");
    DBGLOG(INIT, INFO, "wlanoidSetP2pSetNetworkAddress (%d)\n",
           (s16)u4SetBufferLen);

    ASSERT(prAdapter);
    ASSERT(pu4SetInfoLen);

    *pu4SetInfoLen = 4;

    if (u4SetBufferLen < sizeof(PARAM_NETWORK_ADDRESS_LIST)) {
        return WLAN_STATUS_INVALID_DATA;
    }

    *pu4SetInfoLen = 0;
    u4IpAddressCount = 0;

    prNetworkAddress = prNetworkAddressList->arAddress;
    for (i = 0; i < prNetworkAddressList->u4AddressCount; i++) {
        if (prNetworkAddress->u2AddressType ==
            PARAM_PROTOCOL_ID_TCP_IP &&
            prNetworkAddress->u2AddressLength ==
            sizeof(PARAM_NETWORK_ADDRESS_IP)) {
            u4IpAddressCount++;
        }

        prNetworkAddress =
            (P_PARAM_NETWORK_ADDRESS)((unsigned long)
                                      prNetworkAddress +
                                      (unsigned long)(
                                          prNetworkAddress
                                          ->
                                          u2AddressLength +
                                          OFFSET_OF(
                                              PARAM_NETWORK_ADDRESS,
                                              aucAddress)));
    }

    /* construct payload of command packet */
    u4CmdSize = OFFSET_OF(CMD_SET_NETWORK_ADDRESS_LIST, arNetAddress) +
                sizeof(IPV4_NETWORK_ADDRESS) * u4IpAddressCount;

    if (u4IpAddressCount == 0) {
        u4CmdSize = sizeof(CMD_SET_NETWORK_ADDRESS_LIST);
    }

    prCmdNetworkAddressList = (P_CMD_SET_NETWORK_ADDRESS_LIST)kalMemAlloc(
        u4CmdSize, VIR_MEM_TYPE);

    if (prCmdNetworkAddressList == NULL) {
        return WLAN_STATUS_FAILURE;
    }

    kalMemZero(prCmdNetworkAddressList, u4CmdSize);

    /* fill P_CMD_SET_NETWORK_ADDRESS_LIST */
    prCmdNetworkAddressList->ucBssIndex = prNetworkAddressList->ucBssIdx;

    /* only to set IP address to FW once ARP filter is enabled */
    if (prAdapter->fgEnArpFilter) {
        prCmdNetworkAddressList->ucAddressCount = (u8)u4IpAddressCount;
        prNetworkAddress = prNetworkAddressList->arAddress;

        DBGLOG(INIT, INFO, "u4IpAddressCount (%ld)\n",
               (s32)u4IpAddressCount);
        for (i = 0, j = 0; i < prNetworkAddressList->u4AddressCount;
             i++) {
            if (prNetworkAddress->u2AddressType ==
                PARAM_PROTOCOL_ID_TCP_IP &&
                prNetworkAddress->u2AddressLength ==
                sizeof(PARAM_NETWORK_ADDRESS_IP)) {
                prNetAddrIp =
                    (P_PARAM_NETWORK_ADDRESS_IP)
                    prNetworkAddress->aucAddress;

                kalMemCopy(
                    prCmdNetworkAddressList->arNetAddress[j]
                    .aucIpAddr,
                    &(prNetAddrIp->in_addr), sizeof(u32));

                j++;

                pucBuf = (u8 *)&prNetAddrIp->in_addr;
                DBGLOG(INIT, INFO,
                       "prNetAddrIp->in_addr:%d:%d:%d:%d\n",
                       (u8)pucBuf[0], (u8)pucBuf[1],
                       (u8)pucBuf[2], (u8)pucBuf[3]);
            }

            prNetworkAddress =
                (P_PARAM_NETWORK_ADDRESS)((unsigned long)
                                          prNetworkAddress +
                                          (unsigned long)(
                                              prNetworkAddress
                                              ->
                                              u2AddressLength
                                              +
                                              OFFSET_OF
                                              (
                                                  PARAM_NETWORK_ADDRESS,
                                                  aucAddress)));
        }
    } else {
        prCmdNetworkAddressList->ucAddressCount = 0;
    }

    rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_IP_ADDRESS, true,
                                  false, true, nicCmdEventSetIpAddress,
                                  nicOidCmdTimeoutCommon, u4CmdSize,
                                  (u8 *)prCmdNetworkAddressList,
                                  pvSetBuffer, u4SetBufferLen);

    kalMemFree(prCmdNetworkAddressList, VIR_MEM_TYPE, u4CmdSize);
    return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set Multicast Address List.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 * set. \param[in] u4SetBufferLen The length of the set buffer. \param[out]
 * pu4SetInfoLen If the call is successful, returns the number of bytes read
 * from the set buffer. If the call failed due to invalid length of the set
 * buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidSetP2PMulticastList(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                           IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    CMD_MAC_MCAST_ADDR rCmdMacMcastAddr;

    ASSERT(prAdapter);
    ASSERT(pu4SetInfoLen);

    /* The data must be a multiple of the Ethernet address size. */
    if ((u4SetBufferLen % MAC_ADDR_LEN)) {
        DBGLOG(REQ, WARN, "Invalid MC list length %ld\n",
               u4SetBufferLen);

        *pu4SetInfoLen =
            (((u4SetBufferLen + MAC_ADDR_LEN) - 1) / MAC_ADDR_LEN) *
            MAC_ADDR_LEN;

        return WLAN_STATUS_INVALID_LENGTH;
    }

    *pu4SetInfoLen = u4SetBufferLen;

    /* Verify if we can support so many multicast addresses. */
    if (u4SetBufferLen > MAX_NUM_GROUP_ADDR * MAC_ADDR_LEN) {
        DBGLOG(REQ, WARN, "Too many MC addresses\n");

        return WLAN_STATUS_MULTICAST_FULL;
    }

    /* NOTE(Kevin): Windows may set u4SetBufferLen == 0 &&
     * pvSetBuffer == NULL to clear exist Multicast List.
     */
    if (u4SetBufferLen) {
        ASSERT(pvSetBuffer);
    }

    if (prAdapter->rAcpiState == ACPI_STATE_D3) {
        DBGLOG(REQ,
               WARN,
               "Fail in set multicast list! (Adapter not ready). ACPI=D%d, Radio=%d\n",
               prAdapter->rAcpiState,
               prAdapter->fgIsRadioOff);
        return WLAN_STATUS_ADAPTER_NOT_READY;
    }

    rCmdMacMcastAddr.u4NumOfGroupAddr = u4SetBufferLen / MAC_ADDR_LEN;
    rCmdMacMcastAddr.ucBssIndex = P2P_DEV_BSS_INDEX;  /* TODO: */
    kalMemCopy(rCmdMacMcastAddr.arAddress, pvSetBuffer, u4SetBufferLen);

    return wlanoidSendSetQueryP2PCmd(
        prAdapter, CMD_ID_MAC_MCAST_ADDR, P2P_DEV_BSS_INDEX,
        /* TODO: */
        /* This CMD response is no need to complete the OID. Or the
         * event would unsync. */
        true, false, false, nicCmdEventSetCommon,
        nicOidCmdTimeoutCommon, sizeof(CMD_MAC_MCAST_ADDR),
        (u8 *)&rCmdMacMcastAddr, pvSetBuffer, u4SetBufferLen);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to send GAS frame for P2P Service Discovery
 * Request
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 * set. \param[in] u4SetBufferLen The length of the set buffer. \param[out]
 * pu4SetInfoLen If the call is successful, returns the number of bytes read
 * from the set buffer. If the call failed due to invalid length of the set
 * buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidSendP2PSDRequest(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                        IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;

    ASSERT(prAdapter);
    ASSERT(pu4SetInfoLen);

    if (u4SetBufferLen) {
        ASSERT(pvSetBuffer);
    }

    if (u4SetBufferLen < sizeof(PARAM_P2P_SEND_SD_REQUEST)) {
        *pu4SetInfoLen = sizeof(PARAM_P2P_SEND_SD_REQUEST);
        return WLAN_STATUS_BUFFER_TOO_SHORT;
    }
    /* rWlanStatus = p2pFsmRunEventSDRequest(prAdapter,
     * (P_PARAM_P2P_SEND_SD_REQUEST)pvSetBuffer); */

    return rWlanStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to send GAS frame for P2P Service Discovery
 * Response
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 * set. \param[in] u4SetBufferLen The length of the set buffer. \param[out]
 * pu4SetInfoLen If the call is successful, returns the number of bytes read
 * from the set buffer. If the call failed due to invalid length of the set
 * buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidSendP2PSDResponse(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                         IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;

    ASSERT(prAdapter);
    ASSERT(pu4SetInfoLen);

    if (u4SetBufferLen) {
        ASSERT(pvSetBuffer);
    }

    if (u4SetBufferLen < sizeof(PARAM_P2P_SEND_SD_RESPONSE)) {
        *pu4SetInfoLen = sizeof(PARAM_P2P_SEND_SD_RESPONSE);
        return WLAN_STATUS_BUFFER_TOO_SHORT;
    }
    /* rWlanStatus = p2pFsmRunEventSDResponse(prAdapter,
     * (P_PARAM_P2P_SEND_SD_RESPONSE)pvSetBuffer); */

    return rWlanStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to get GAS frame for P2P Service Discovery
 * Request
 *
 * \param[in]  prAdapter        Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer    A pointer to the buffer that holds the result of
 *                              the query.
 * \param[in]  u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen  If the call is successful, returns the number of
 *                              bytes written into the query buffer. If the call
 *                              failed due to invalid length of the query
 * buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidGetP2PSDRequest(IN P_ADAPTER_T prAdapter, IN void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen, OUT u32 *pu4QueryInfoLen)
{
    WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;
    /* u8 * pucChannelNum = NULL; */
    /* u8 ucChannelNum = 0, ucSeqNum = 0; */

    ASSERT(prAdapter);
    ASSERT(pu4QueryInfoLen);

    if (u4QueryBufferLen) {
        ASSERT(pvQueryBuffer);
    }

    if (u4QueryBufferLen < sizeof(PARAM_P2P_GET_SD_REQUEST)) {
        *pu4QueryInfoLen = sizeof(PARAM_P2P_GET_SD_REQUEST);
        return WLAN_STATUS_BUFFER_TOO_SHORT;
    }

    DBGLOG(P2P, TRACE, "Get Service Discovery Request\n");

    *pu4QueryInfoLen = 0;
    return rWlanStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to get GAS frame for P2P Service Discovery
 * Response
 *
 * \param[in]  prAdapter        Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer    A pointer to the buffer that holds the result of
 *                              the query.
 * \param[in]  u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen  If the call is successful, returns the number of
 *                              bytes written into the query buffer. If the call
 *                              failed due to invalid length of the query
 * buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidGetP2PSDResponse(IN P_ADAPTER_T prAdapter, IN void *pvQueryBuffer,
                        IN u32 u4QueryBufferLen, OUT u32 *pu4QueryInfoLen)
{
    WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;
    /* u8 ucSeqNum = 0, */

    ASSERT(prAdapter);
    ASSERT(pu4QueryInfoLen);

    if (u4QueryBufferLen) {
        ASSERT(pvQueryBuffer);
    }

    if (u4QueryBufferLen < sizeof(PARAM_P2P_GET_SD_RESPONSE)) {
        *pu4QueryInfoLen = sizeof(PARAM_P2P_GET_SD_RESPONSE);
        return WLAN_STATUS_BUFFER_TOO_SHORT;
    }

    DBGLOG(P2P, TRACE, "Get Service Discovery Response\n");

    *pu4QueryInfoLen = 0;
    return rWlanStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to terminate P2P Service Discovery Phase
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 * set. \param[in] u4SetBufferLen The length of the set buffer. \param[out]
 * pu4SetInfoLen If the call is successful, returns the number of bytes read
 * from the set buffer. If the call failed due to invalid length of the set
 * buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidSetP2PTerminateSDPhase(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                              IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;
    P_PARAM_P2P_TERMINATE_SD_PHASE prP2pTerminateSD =
        (P_PARAM_P2P_TERMINATE_SD_PHASE)NULL;
    u8 aucNullAddr[] = NULL_MAC_ADDR;

    do {
        if ((prAdapter == NULL) || (pu4SetInfoLen == NULL)) {
            break;
        }

        if ((u4SetBufferLen) && (pvSetBuffer == NULL)) {
            break;
        }

        if (u4SetBufferLen < sizeof(PARAM_P2P_TERMINATE_SD_PHASE)) {
            *pu4SetInfoLen = sizeof(PARAM_P2P_TERMINATE_SD_PHASE);
            rWlanStatus = WLAN_STATUS_BUFFER_TOO_SHORT;
            break;
        }

        prP2pTerminateSD = (P_PARAM_P2P_TERMINATE_SD_PHASE)pvSetBuffer;

        if (EQUAL_MAC_ADDR(prP2pTerminateSD->rPeerAddr, aucNullAddr)) {
            DBGLOG(P2P, TRACE, "Service Discovery Version 2.0\n");
            /* p2pFuncSetVersionNumOfSD(prAdapter, 2); */
        }
        /* rWlanStatus = p2pFsmRunEventSDAbort(prAdapter); */
    } while (false);

    return rWlanStatus;
}

#if CFG_SUPPORT_ANTI_PIRACY
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 * set. \param[in] u4SetBufferLen The length of the set buffer. \param[out]
 * pu4SetInfoLen If the call is successful, returns the number of bytes read
 * from the set buffer. If the call failed due to invalid length of the set
 * buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidSetSecCheckRequest(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                          IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    ASSERT(prAdapter);
    ASSERT(pu4SetInfoLen);

    if (u4SetBufferLen) {
        ASSERT(pvSetBuffer);
    }

    return WLAN_STATUS_NOT_SUPPORTED;
}

#endif

WLAN_STATUS
wlanoidSetNoaParam(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                   IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    P_PARAM_CUSTOM_NOA_PARAM_STRUCT_T prNoaParam;
    CMD_CUSTOM_NOA_PARAM_STRUCT_T rCmdNoaParam;

    DEBUGFUNC("wlanoidSetNoaParam");
    DBGLOG(INIT, TRACE, "\n");

    ASSERT(prAdapter);
    ASSERT(pu4SetInfoLen);

    *pu4SetInfoLen = sizeof(PARAM_CUSTOM_NOA_PARAM_STRUCT_T);

    if (u4SetBufferLen < sizeof(PARAM_CUSTOM_NOA_PARAM_STRUCT_T)) {
        return WLAN_STATUS_INVALID_LENGTH;
    }

    ASSERT(pvSetBuffer);

    prNoaParam = (P_PARAM_CUSTOM_NOA_PARAM_STRUCT_T)pvSetBuffer;

    kalMemZero(&rCmdNoaParam, sizeof(CMD_CUSTOM_NOA_PARAM_STRUCT_T));
    rCmdNoaParam.u4NoaDurationMs = prNoaParam->u4NoaDurationMs;
    rCmdNoaParam.u4NoaIntervalMs = prNoaParam->u4NoaIntervalMs;
    rCmdNoaParam.u4NoaCount = prNoaParam->u4NoaCount;

    return wlanoidSendSetQueryP2PCmd(prAdapter, CMD_ID_SET_NOA_PARAM,
                                     prNoaParam->ucBssIdx, true, false,
                                     true, NULL, nicOidCmdTimeoutCommon,
                                     sizeof(CMD_CUSTOM_NOA_PARAM_STRUCT_T),
                                     (u8 *)&rCmdNoaParam, pvSetBuffer,
                                     u4SetBufferLen);
}

WLAN_STATUS
wlanoidSetOppPsParam(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    P_PARAM_CUSTOM_OPPPS_PARAM_STRUCT_T prOppPsParam;
    CMD_CUSTOM_OPPPS_PARAM_STRUCT_T rCmdOppPsParam;

    DEBUGFUNC("wlanoidSetOppPsParam");
    DBGLOG(INIT, TRACE, "\n");

    ASSERT(prAdapter);
    ASSERT(pu4SetInfoLen);

    *pu4SetInfoLen = sizeof(PARAM_CUSTOM_OPPPS_PARAM_STRUCT_T);

    if (u4SetBufferLen < sizeof(PARAM_CUSTOM_OPPPS_PARAM_STRUCT_T)) {
        return WLAN_STATUS_INVALID_LENGTH;
    }

    ASSERT(pvSetBuffer);

    prOppPsParam = (P_PARAM_CUSTOM_OPPPS_PARAM_STRUCT_T)pvSetBuffer;

    kalMemZero(&rCmdOppPsParam, sizeof(CMD_CUSTOM_OPPPS_PARAM_STRUCT_T));

    rCmdOppPsParam.ucBssIdx = prOppPsParam->ucBssIdx;

    if (prOppPsParam->ucOppPs) {
        /* [spec. 3.3.2] CTWindow should be at least 10 TU */
        if (prOppPsParam->u4CTwindowMs < 10) {
            rCmdOppPsParam.u4CTwindowMs = 10;
        } else {
            rCmdOppPsParam.u4CTwindowMs =
                prOppPsParam->u4CTwindowMs;
        }
    } else {
        rCmdOppPsParam.u4CTwindowMs = 0;  /* Set 0 means disable OppPs */
    }
    return wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_OPPPS_PARAM, true,
                               false, true, nicCmdEventSetCommon,
                               nicOidCmdTimeoutCommon,
                               sizeof(CMD_CUSTOM_OPPPS_PARAM_STRUCT_T),
                               (u8 *)&rCmdOppPsParam, pvSetBuffer,
                               u4SetBufferLen);
}

WLAN_STATUS
wlanoidSetUApsdParam(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    P_PARAM_CUSTOM_UAPSD_PARAM_STRUCT_T prUapsdParam;
    CMD_CUSTOM_UAPSD_PARAM_STRUCT_T rCmdUapsdParam;
    P_PM_PROFILE_SETUP_INFO_T prPmProfSetupInfo;
    P_BSS_INFO_T prBssInfo;

    DEBUGFUNC("wlanoidSetUApsdParam");
    DBGLOG(INIT, TRACE, "\n");

    ASSERT(prAdapter);
    ASSERT(pu4SetInfoLen);

    *pu4SetInfoLen = sizeof(PARAM_CUSTOM_UAPSD_PARAM_STRUCT_T);

    if (u4SetBufferLen < sizeof(PARAM_CUSTOM_UAPSD_PARAM_STRUCT_T)) {
        return WLAN_STATUS_INVALID_LENGTH;
    }

    ASSERT(pvSetBuffer);

    prUapsdParam = (P_PARAM_CUSTOM_UAPSD_PARAM_STRUCT_T)pvSetBuffer;

    prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prUapsdParam->ucBssIdx);
    prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;

    kalMemZero(&rCmdUapsdParam, sizeof(CMD_CUSTOM_OPPPS_PARAM_STRUCT_T));
    rCmdUapsdParam.fgEnAPSD = prUapsdParam->fgEnAPSD;

    rCmdUapsdParam.fgEnAPSD_AcBe = prUapsdParam->fgEnAPSD_AcBe;
    rCmdUapsdParam.fgEnAPSD_AcBk = prUapsdParam->fgEnAPSD_AcBk;
    rCmdUapsdParam.fgEnAPSD_AcVo = prUapsdParam->fgEnAPSD_AcVo;
    rCmdUapsdParam.fgEnAPSD_AcVi = prUapsdParam->fgEnAPSD_AcVi;
    prPmProfSetupInfo->ucBmpDeliveryAC =
        ((prUapsdParam->fgEnAPSD_AcBe << 0) |
         (prUapsdParam->fgEnAPSD_AcBk << 1) |
         (prUapsdParam->fgEnAPSD_AcVi << 2) |
         (prUapsdParam->fgEnAPSD_AcVo << 3));
    prPmProfSetupInfo->ucBmpTriggerAC =
        ((prUapsdParam->fgEnAPSD_AcBe << 0) |
         (prUapsdParam->fgEnAPSD_AcBk << 1) |
         (prUapsdParam->fgEnAPSD_AcVi << 2) |
         (prUapsdParam->fgEnAPSD_AcVo << 3));

    rCmdUapsdParam.ucMaxSpLen = prUapsdParam->ucMaxSpLen;
    prPmProfSetupInfo->ucUapsdSp = prUapsdParam->ucMaxSpLen;

    return wlanoidSendSetQueryP2PCmd(
        prAdapter, CMD_ID_SET_UAPSD_PARAM, prBssInfo->ucBssIndex, true,
        false, true, NULL, nicOidCmdTimeoutCommon,
        sizeof(CMD_CUSTOM_OPPPS_PARAM_STRUCT_T), (u8 *)&rCmdUapsdParam,
        pvSetBuffer, u4SetBufferLen);
}

WLAN_STATUS
wlanoidQueryP2pVersion(IN P_ADAPTER_T prAdapter, IN void *pvQueryBuffer,
                       IN u32 u4QueryBufferLen, OUT u32 *pu4QueryInfoLen)
{
    WLAN_STATUS rResult = WLAN_STATUS_FAILURE;
    /* u8 * pucVersionNum = (u8 *)pvQueryBuffer; */

    do {
        if ((prAdapter == NULL) || (pu4QueryInfoLen == NULL)) {
            break;
        }

        if ((u4QueryBufferLen) && (pvQueryBuffer == NULL)) {
            break;
        }

        if (u4QueryBufferLen < sizeof(u8)) {
            *pu4QueryInfoLen = sizeof(u8);
            rResult = WLAN_STATUS_BUFFER_TOO_SHORT;
            break;
        }
    } while (false);

    return rResult;
}

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set the WPS mode.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
WLAN_STATUS
wlanoidSetP2pWPSmode(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                     IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    WLAN_STATUS status;
    u32 u4IsWPSmode = 0;
    int i = 0;

    DEBUGFUNC("wlanoidSetP2pWPSmode");
    ASSERT(prAdapter);
    ASSERT(pu4SetInfoLen);

    if (pvSetBuffer) {
        u4IsWPSmode = *(u32 *)pvSetBuffer;
    }else{
        u4IsWPSmode = 0;
    }

    for (i = 0; i < KAL_P2P_NUM; i++) /* set all Role to the same value */
        if (u4IsWPSmode) {
            prAdapter->rWifiVar.prP2PConnSettings[i]->fgIsWPSMode =
                1;
        } else {
            prAdapter->rWifiVar.prP2PConnSettings[i]->fgIsWPSMode =
                0;
        }

    status = nicUpdateBss(prAdapter, P2P_DEV_BSS_INDEX);

    return status;
}

#endif

WLAN_STATUS
wlanoidSetP2pSupplicantVersion(IN P_ADAPTER_T prAdapter, IN void *pvSetBuffer,
                               IN u32 u4SetBufferLen, OUT u32 *pu4SetInfoLen)
{
    WLAN_STATUS rResult = WLAN_STATUS_FAILURE;
    u8 ucVersionNum;

    do {
        if ((prAdapter == NULL) || (pu4SetInfoLen == NULL)) {
            rResult = WLAN_STATUS_INVALID_DATA;
            break;
        }

        if ((u4SetBufferLen) && (pvSetBuffer == NULL)) {
            rResult = WLAN_STATUS_INVALID_DATA;
            break;
        }

        *pu4SetInfoLen = sizeof(u8);

        if (u4SetBufferLen < sizeof(u8)) {
            rResult = WLAN_STATUS_INVALID_LENGTH;
            break;
        }

        ucVersionNum = *((u8 *)pvSetBuffer);

        rResult = WLAN_STATUS_SUCCESS;
    } while (false);

    return rResult;
}

#if CFG_SUPPORT_P2P_RSSI_QUERY
WLAN_STATUS
wlanoidQueryP2pRssi(IN P_ADAPTER_T prAdapter, IN void *pvQueryBuffer,
                    IN u32 u4QueryBufferLen, OUT u32 *pu4QueryInfoLen)
{
    DEBUGFUNC("wlanoidQueryP2pRssi");

    ASSERT(prAdapter);
    ASSERT(pu4QueryInfoLen);
    if (u4QueryBufferLen) {
        ASSERT(pvQueryBuffer);
    }

    if (prAdapter->fgIsEnableLpdvt) {
        return WLAN_STATUS_NOT_SUPPORTED;
    }

    *pu4QueryInfoLen = sizeof(PARAM_RSSI);

    /* Check for query buffer length */
    if (u4QueryBufferLen < *pu4QueryInfoLen) {
        DBGLOG(REQ, WARN, "Too short length %ld\n", u4QueryBufferLen);
        return WLAN_STATUS_BUFFER_TOO_SHORT;
    }

    if (prAdapter->fgIsP2pLinkQualityValid == true &&
        (kalGetTimeTick() - prAdapter->rP2pLinkQualityUpdateTime) <=
        CFG_LINK_QUALITY_VALID_PERIOD) {
        PARAM_RSSI rRssi;

        rRssi = (PARAM_RSSI)prAdapter->rP2pLinkQuality.cRssi;  /* ranged
                                                                * from
                                                                * (-128 ~
                                                                * 30) in
                                                                * unit of
                                                                * dBm */

        if (rRssi > PARAM_WHQL_RSSI_MAX_DBM) {
            rRssi = PARAM_WHQL_RSSI_MAX_DBM;
        }else if (rRssi < PARAM_WHQL_RSSI_MIN_DBM) {
            rRssi = PARAM_WHQL_RSSI_MIN_DBM;
        }

        kalMemCopy(pvQueryBuffer, &rRssi, sizeof(PARAM_RSSI));
        return WLAN_STATUS_SUCCESS;
    }

    return wlanSendSetQueryCmd(prAdapter, CMD_ID_GET_LINK_QUALITY, false,
                               true, true, nicCmdEventQueryLinkQuality,
                               nicOidCmdTimeoutCommon, *pu4QueryInfoLen,
                               pvQueryBuffer, pvQueryBuffer,
                               u4QueryBufferLen);
}
#endif

u32 wlanoidAbortP2pScan(IN P_ADAPTER_T prAdapter, OUT void *pvQueryBuffer,
                        IN u32 u4QueryBufferLen, OUT u32 *pu4QueryInfoLen)
{
    u8 ucBssIdx;

    ASSERT(prAdapter);

    ucBssIdx = *((u8 *)pvQueryBuffer);

    if (ucBssIdx == prAdapter->ucP2PDevBssIdx) {
        p2pDevFsmRunEventScanAbort(prAdapter, ucBssIdx);
    }else{
        p2pRoleFsmRunEventScanAbort(prAdapter, ucBssIdx);
    }

    return WLAN_STATUS_SUCCESS;
}
