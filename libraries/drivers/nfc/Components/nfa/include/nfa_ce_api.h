/******************************************************************************
 *
 *  Copyright (C) 2010-2013 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed pursuant to the
 *  terms and conditions of a separate, written license agreement executed between you
 *  and Broadcom (an "Authorized License").  Except as set forth in an Authorized
 *  License, Broadcom grants no license (express or implied), right to use, or waiver of
 *  any kind with respect to the Software, and Broadcom expressly reserves all rights in
 *  and to the Software and all intellectual property rights therein.  IF YOU HAVE NO
 *  AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 *  WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE
 *  OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization, constitutes the valuable
 *  trade secrets of Broadcom, and you shall use all reasonable efforts to protect the confidentiality
 *  thereof, and to use this information only in connection with your use of Broadcom integrated
 *  circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS
 *  IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR
 *  A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU
 *  ASSUME THE ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE
 *  SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 *  OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ******************************************************************************/


/******************************************************************************
 *
 *  NFA card emulation API functions
 *
 ******************************************************************************/
#ifndef NFA_CE_API_H
#define NFA_CE_API_H

#include "nfc_target.h"
#include "nfa_api.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**
** Function         NFA_CeConfigureLocalTag
**
** Description      Configure local NDEF tag.
**
**                  Tag events will be notifed using the tNFA_CONN_CBACK
**                  (registered during NFA_Enable)
**
**                  The NFA_CE_LOCAL_TAG_CONFIGURED_EVT reports the status of the
**                  operation.
**
**                  Activation and deactivation are reported using the
**                  NFA_ACTIVATED_EVT and NFA_DEACTIVATED_EVT events
**
**                  If a write-request is received to update the tag memory,
**                  an NFA_CE_NDEF_WRITE_CPLT_EVT will notify the application, along
**                  with a buffer containing the updated contents.
**
**                  To disable the local NDEF tag, set protocol_mask=0
**
**                  The NDEF data provided by p_ndef_data must be persistent
**                  as long as the local NDEF tag is enabled. Also, Input parameters p_uid and
**                  uid_len are reserved for future use.
**
**
** Note:            If RF discovery is started, NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT
**                  should happen before calling this function.
**
** Returns:
**                  NFA_STATUS_OK,            if command accepted
**                  NFA_STATUS_INVALID_PARAM,
**                      if protocol_maks is not 0 and p_ndef_data is NULL
**                  (or) uid_len is not 0
**                  (or) if protocol mask is set for Type 1 or Type 2
**
**                  NFA_STATUS_FAILED:        otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_CeConfigureLocalTag (tNFA_PROTOCOL_MASK protocol_mask,
                                                    UINT8     *p_ndef_data,
                                                    UINT16    ndef_cur_size,
                                                    UINT16    ndef_max_size,
                                                    BOOLEAN   read_only,
                                                    UINT8     uid_len,
                                                    UINT8     *p_uid);

/*******************************************************************************
**
** Function         NFA_CeConfigureUiccListenTech
**
** Description      Configure listening for the UICC, using the specified
**                  technologies.
**
**                  Events will be notifed using the tNFA_CONN_CBACK
**                  (registered during NFA_Enable)
**
**                  The NFA_CE_UICC_LISTEN_CONFIGURED_EVT reports the status of the
**                  operation.
**
**                  Activation and deactivation are reported using the
**                  NFA_ACTIVATED_EVT and NFA_DEACTIVATED_EVT events
**
** Note:            If RF discovery is started, NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT
**                  should happen before calling this function
**
** Returns:
**                  NFA_STATUS_OK, if command accepted
**                  NFA_STATUS_FAILED: otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_CeConfigureUiccListenTech (tNFA_HANDLE          ee_handle,
                                                          tNFA_TECHNOLOGY_MASK tech_mask);

/*******************************************************************************
**
** Function         NFA_CeRegisterFelicaSystemCodeOnDH
**
** Description      Register listening callback for Felica system code
**
**                  The NFA_CE_REGISTERED_EVT reports the status of the
**                  operation.
**
** Note:            If RF discovery is started, NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT
**                  should happen before calling this function
**
** Returns:
**                  NFA_STATUS_OK, if command accepted
**                  NFA_STATUS_FAILED: otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_CeRegisterFelicaSystemCodeOnDH (UINT16           system_code,
                                                               UINT8            nfcid2[NCI_RF_F_UID_LEN],
                                                               tNFA_CONN_CBACK  *p_conn_cback);

/*******************************************************************************
**
** Function         NFA_CeDeregisterFelicaSystemCodeOnDH
**
** Description      Deregister listening callback for Felica
**                  (previously registered using NFA_CeRegisterFelicaSystemCodeOnDH)
**
**                  The NFA_CE_DEREGISTERED_EVT reports the status of the
**                  operation.
**
** Note:            If RF discovery is started, NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT
**                  should happen before calling this function
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_BAD_HANDLE if invalid handle
**                  NFA_STATUS_FAILED otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_CeDeregisterFelicaSystemCodeOnDH (tNFA_HANDLE handle);

/*******************************************************************************
**
** Function         NFA_CeRegisterAidOnDH
**
** Description      Register listening callback for the specified ISODEP AID
**
**                  The NFA_CE_REGISTERED_EVT reports the status of the
**                  operation.
**
**                  If no AID is specified (aid_len=0), then p_conn_cback will
**                  will get notifications for any AIDs routed to the DH. This
**                  over-rides callbacks registered for specific AIDs.
**
** Note:            If RF discovery is started, NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT
**                  should happen before calling this function
**
** Returns:
**                  NFA_STATUS_OK, if command accepted
**                  NFA_STATUS_FAILED: otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_CeRegisterAidOnDH (UINT8           aid[NFC_MAX_AID_LEN],
                                                  UINT8           aid_len,
                                                  tNFA_CONN_CBACK *p_conn_cback);

/*******************************************************************************
**
** Function         NFA_CeDeregisterAidOnDH
**
** Description      Deregister listening callback for ISODEP AID
**                  (previously registered using NFA_CeRegisterAidOnDH)
**
**                  The NFA_CE_DEREGISTERED_EVT reports the status of the
**                  operation.
**
** Note:            If RF discovery is started, NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT
**                  should happen before calling this function
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_BAD_HANDLE if invalid handle
**                  NFA_STATUS_FAILED otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_CeDeregisterAidOnDH (tNFA_HANDLE handle);

/*******************************************************************************
**
** Function         NFA_CeSetIsoDepListenTech
**
** Description      Set the technologies (NFC-A and/or NFC-B) to listen for when
**                  NFA_CeConfigureLocalTag or NFA_CeDeregisterAidOnDH are called.
**
**                  By default (if this API is not called), NFA will listen
**                  for both NFC-A and NFC-B for ISODEP.
**
** Note:            If listening for ISODEP on UICC, the DH listen callbacks
**                  may still get activate notifications for ISODEP if the reader/
**                  writer selects an AID that is not routed to the UICC (regardless
**                  of whether A or B was disabled using NFA_CeSetIsoDepListenTech)
**
** Note:            If RF discovery is started, NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT
**                  should happen before calling this function
**
** Returns:
**                  NFA_STATUS_OK, if command accepted
**                  NFA_STATUS_FAILED: otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_CeSetIsoDepListenTech (tNFA_TECHNOLOGY_MASK tech_mask);

#ifdef __cplusplus
}
#endif

#endif /* NFA_CE_API_H */
