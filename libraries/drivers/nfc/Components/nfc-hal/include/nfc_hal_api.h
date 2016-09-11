/******************************************************************************
 *
 *  Copyright (C) 2012-2013 Broadcom Corporation
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
 *  NFC Hardware Abstraction Layer API
 *
 ******************************************************************************/
#ifndef NFC_HAL_API_H
#define NFC_HAL_API_H
#include "data_types.h"

/****************************************************************************
** NFC_HDR header definition for NFC messages
*****************************************************************************/
typedef struct
{
    UINT16          event;
    UINT16          len;
    UINT16          offset;
    UINT16          layer_specific;
} NFC_HDR;
#define NFC_HDR_SIZE (sizeof (NFC_HDR))

/*******************************************************************************
** tHAL_STATUS Definitions
*******************************************************************************/
#define HAL_NFC_STATUS_OK               0
#define HAL_NFC_STATUS_FAILED           1
#define HAL_NFC_STATUS_ERR_TRANSPORT    2
#define HAL_NFC_STATUS_ERR_CMD_TIMEOUT  3
#define HAL_NFC_STATUS_REFUSED          4

typedef UINT8 tHAL_NFC_STATUS;

/*******************************************************************************
** tHAL_HCI_NETWK_CMD Definitions
*******************************************************************************/
#define HAL_NFC_HCI_NO_UICC_HOST    0x00
#define HAL_NFC_HCI_UICC0_HOST      0x01
#define HAL_NFC_HCI_UICC1_HOST      0x02
#define HAL_NFC_HCI_UICC2_HOST      0x04

/*******************************************************************************
** tHAL_NFC_CBACK Definitions
*******************************************************************************/

/* tHAL_NFC_CBACK events */
#define HAL_NFC_OPEN_CPLT_EVT           0x00
#define HAL_NFC_CLOSE_CPLT_EVT          0x01
#define HAL_NFC_POST_INIT_CPLT_EVT      0x02
#define HAL_NFC_PRE_DISCOVER_CPLT_EVT   0x03
#define HAL_NFC_REQUEST_CONTROL_EVT     0x04
#define HAL_NFC_RELEASE_CONTROL_EVT     0x05
#define HAL_NFC_ERROR_EVT               0x06

#if (NFC_BRCM_NOT_OPEN_INCLUDED == TRUE)
#define HAL_NFC_RESTARTING_EVT          0x80
#define HAL_NFC_BRCM_CMPL_EVT           0x81
#endif

typedef void (tHAL_NFC_STATUS_CBACK) (tHAL_NFC_STATUS status);
typedef void (tHAL_NFC_CBACK) (UINT8 event, tHAL_NFC_STATUS status);
typedef void (tHAL_NFC_DATA_CBACK) (UINT16 data_len, UINT8   *p_data);

/*******************************************************************************
** tHAL_NFC_ENTRY HAL entry-point lookup table
*******************************************************************************/

typedef void (tHAL_API_INITIALIZE) (void);
typedef void (tHAL_API_TERMINATE) (void);
typedef void (tHAL_API_OPEN) (tHAL_NFC_CBACK *p_hal_cback, tHAL_NFC_DATA_CBACK *p_data_cback);
typedef void (tHAL_API_CLOSE) (void);
typedef void (tHAL_API_CORE_INITIALIZED) (UINT8 *p_core_init_rsp_params);
typedef void (tHAL_API_WRITE) (UINT16 data_len, UINT8 *p_data);
typedef BOOLEAN (tHAL_API_PREDISCOVER) (void);
typedef void (tHAL_API_CONTROL_GRANTED) (void);
typedef void (tHAL_API_POWER_CYCLE) (void);
typedef UINT8 (tHAL_API_GET_MAX_NFCEE) (void);

#if (NFC_BRCM_NOT_OPEN_INCLUDED == TRUE)
/* snooze mode setting */
typedef struct
{
    UINT8   snooze_mode;
    UINT8   idle_threshold_dh;
    UINT8   idle_threshold_nfcc;
    UINT8   nfc_wake_active_mode;
    UINT8   dh_wake_active_mode;
} tHAL_NFC_SNOOZE_CONFIG;

typedef void (tHAL_BRCM_API_WRITE_BUF) (NFC_HDR *p_buf);
typedef void (tHAL_BRCM_API_PRINT_BUFFER_USAGE) (void);
typedef void (tHAL_BRCM_API_ENTER_CE_LOW_POWER_MODE) (void);
typedef void (tHAL_BRCM_API_ENTER_FULL_POWER_MODE ) (void);
typedef void (tHAL_BRCM_API_UPDATE_SNOOZE_MODE) (tHAL_NFC_SNOOZE_CONFIG *p_cfg);
typedef void (tHAL_BRCM_API_UPDATE_BAUD_RATE) (UINT8 baud_rate_index);
#endif

/* data members for NFC_HAL-HCI */
typedef struct
{
    BOOLEAN nfc_hal_prm_nvm_required;       /* set nfc_hal_prm_nvm_required to TRUE, if the platform wants to abort PRM process without NVM */
    UINT16  nfc_hal_nfcc_enable_timeout;    /* max time to wait for RESET NTF after setting REG_PU to high */
    UINT16  nfc_hal_post_xtal_timeout;      /* max time to wait for RESET NTF after setting Xtal frequency */
#if (defined(NFC_HAL_HCI_INCLUDED) && (NFC_HAL_HCI_INCLUDED == TRUE))
    UINT8   nfc_hal_hci_uicc_support;       /* set nfc_hal_hci_uicc_support to Zero, if no UICC is supported otherwise set corresponding bit(s) for every supported UICC(s) */
#endif
} tNFC_HAL_CFG;

typedef struct
{
    tHAL_API_INITIALIZE *initialize;
    tHAL_API_TERMINATE *terminate;
    tHAL_API_OPEN *open;
    tHAL_API_CLOSE *close;
    tHAL_API_CORE_INITIALIZED *core_initialized;
    tHAL_API_WRITE *write;
    tHAL_API_PREDISCOVER *prediscover;
    tHAL_API_CONTROL_GRANTED *control_granted;
    tHAL_API_POWER_CYCLE *power_cycle;
    tHAL_API_GET_MAX_NFCEE *get_max_ee;

#if (NFC_BRCM_NOT_OPEN_INCLUDED == TRUE)
    tHAL_BRCM_API_WRITE_BUF                 *write_buf;
    tHAL_BRCM_API_PRINT_BUFFER_USAGE        *print_buffer_usage;
    tHAL_BRCM_API_ENTER_CE_LOW_POWER_MODE   *enter_ce_low_power_mode;
    tHAL_BRCM_API_ENTER_FULL_POWER_MODE     *enter_full_power_mode;
    tHAL_BRCM_API_UPDATE_SNOOZE_MODE        *update_snooze_mode;
    tHAL_BRCM_API_UPDATE_BAUD_RATE          *update_baud_rate;
#endif

} tHAL_NFC_ENTRY;


/*******************************************************************************
** HAL API Function Prototypes
*******************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/* Toolset-specific macro for exporting API funcitons */
#if (defined(NFC_HAL_TARGET) && (NFC_HAL_TARGET == TRUE)) && (defined(_WINDLL))
#define EXPORT_HAL_API  __declspec(dllexport)
#else
#define EXPORT_HAL_API
#endif

/*******************************************************************************
**
** Function         HAL_NfcInitialize
**
** Description      Called when HAL library is loaded.
**
**                  Initialize GKI and start the HCIT task
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcInitialize(void);

/*******************************************************************************
**
** Function         HAL_NfcTerminate
**
** Description      Called to terminate NFC HAL
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcTerminate(void);

/*******************************************************************************
**
** Function         HAL_NfcOpen
**
** Description      Open transport and intialize the NFCC, and
**                  Register callback for HAL event notifications,
**
**                  HAL_OPEN_CPLT_EVT will notify when operation is complete.
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcOpen (tHAL_NFC_CBACK *p_hal_cback, tHAL_NFC_DATA_CBACK *p_data_cback);

/*******************************************************************************
**
** Function         HAL_NfcClose
**
** Description      Prepare for shutdown. A HAL_CLOSE_CPLT_EVT will be
**                  reported when complete.
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcClose (void);

/*******************************************************************************
**
** Function         HAL_NfcCoreInitialized
**
** Description      Called after the CORE_INIT_RSP is received from the NFCC.
**                  At this time, the HAL can do any chip-specific configuration,
**                  and when finished signal the libnfc-nci with event
**                  HAL_POST_INIT_CPLT_EVT.
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcCoreInitialized (UINT8 *p_core_init_rsp_params);

/*******************************************************************************
**
** Function         HAL_NfcWrite
**
** Description      Send an NCI control message or data packet to the
**                  transport. If an NCI command message exceeds the transport
**                  size, HAL is responsible for fragmenting it, Data packets
**                  must be of the correct size.
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcWrite (UINT16 data_len, UINT8 *p_data);

/*******************************************************************************
**
** Function         HAL_NfcPreDiscover
**
** Description      Perform any vendor-specific pre-discovery actions (if needed)
**                  If any actions were performed TRUE will be returned, and
**                  HAL_PRE_DISCOVER_CPLT_EVT will notify when actions are
**                  completed.
**
** Returns          TRUE if vendor-specific pre-discovery actions initialized
**                  FALSE if no vendor-specific pre-discovery actions are needed.
**
*******************************************************************************/
EXPORT_HAL_API BOOLEAN HAL_NfcPreDiscover (void);

/*******************************************************************************
**
** Function         HAL_NfcControlGranted
**
** Description      Grant control to HAL control for sending NCI commands.
**
**                  Call in response to HAL_REQUEST_CONTROL_EVT.
**
**                  Must only be called when there are no NCI commands pending.
**
**                  HAL_RELEASE_CONTROL_EVT will notify when HAL no longer
**                  needs control of NCI.
**
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcControlGranted (void);

/*******************************************************************************
**
** Function         HAL_NfcPowerCycle
**
** Description      Restart NFCC by power cyle
**
**                  HAL_OPEN_CPLT_EVT will notify when operation is complete.
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcPowerCycle (void);

/*******************************************************************************
**
** Function         HAL_NfcGetMaxNfcee
**
** Description      Retrieve the maximum number of NFCEEs supported by NFCC
**
** Returns          the maximum number of NFCEEs supported by NFCC
**
*******************************************************************************/
EXPORT_HAL_API UINT8 HAL_NfcGetMaxNfcee (void);

#if (NFC_BRCM_NOT_OPEN_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         HAL_NfcEnterCELowPowerMode
**
** Description      Notify HAL that NFCC is ready to enter CE low power mode
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcEnterCELowPowerMode (void);

/*******************************************************************************
**
** Function         HAL_NfcEnterFullPowerMode
**
** Description      Notify HAL that NFCC is booting up to full power mode
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcEnterFullPowerMode (void);

/*******************************************************************************
**
** Function         HAL_NfcUpdateSnoozeMode
**
** Description      Update Snooze mode
**                  HAL_NFC_BRCM_CMPL_EVT will be returned with status
**
**                  tHAL_NFC_SNOOZE_CONFIG
**                     snooze_mode : NFC_HAL_LP_SNOOZE_MODE_NONE
**                                   NFC_HAL_LP_SNOOZE_MODE_UART
**                                   NFC_HAL_LP_SNOOZE_MODE_SPI_I2C
**
**                     NFCC goes to snooze mode if no activity from DH and internally:
**                      - idle_threshold_dh   : idle threshold for DH in 100ms unit
**                      - idle_threshold_nfcc : idle threshold for NFCC in 100ms unit
**
**                     GPIO configuration to wake up NFCC or DH
**                      - nfc_wake_active_mode: NFC_HAL_LP_ACTIVE_LOW or NFC_HAL_LP_ACTIVE_HIGH
**                      - dh_wake_active_mode:  NFC_HAL_LP_ACTIVE_LOW or NFC_HAL_LP_ACTIVE_HIGH
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcUpdateSnoozeMode (tHAL_NFC_SNOOZE_CONFIG *p_snooze_cfg);

/*******************************************************************************
**
** Function         HAL_NfcUpdateUartBaudRate
**
** Description      Update UART baud rate
**                  HAL_NFC_BRCM_CMPL_EVT will be returned with status
**
**                  baud_rate_index : USERIAL_BAUD_115200
**                                    USERIAL_BAUD_230400
**                                    USERIAL_BAUD_460800
**                                    USERIAL_BAUD_921600
**                                    USERIAL_BAUD_1M
**                                    USERIAL_BAUD_1_5M
**                                    USERIAL_BAUD_2M
**                                    USERIAL_BAUD_3M
**                                    USERIAL_BAUD_4M
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcUpdateUartBaudRate (UINT8 baud_rate_index);

/*******************************************************************************
**
** Function         HAL_NfcPrintBufferUsage
**
** Description      Print GKI buffer usage
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcPrintBufferUsage (void);


#ifdef NFC_HAL_SHARED_GKI
/*******************************************************************************
**
** Function         HAL_NfcWriteBuf
**
** Description      Shared GKI version of HAL_NfcWrite.
**
**                  Send an NCI control message or data packet to the
**                  transport. If an NCI command message exceeds the transport
**                  size, HAL is responsible for fragmenting it, Data packets
**                  must be of the correct size.
**
** Returns          void
**
*******************************************************************************/
EXPORT_HAL_API void HAL_NfcWriteBuf (NFC_HDR *p_msg);
#endif /* NFC_HAL_SHARED_GKI */

#endif /* (NFC_BRCM_NOT_OPEN_INCLUDED == TRUE) */

#ifdef __cplusplus
}
#endif

#endif /* NFC_HAL_API_H  */
