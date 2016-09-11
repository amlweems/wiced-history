/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * WICED Over The Air 2 Background Service interface (OTA2)
 *
 *        ***  PRELIMINARY - SUBJECT TO CHANGE  ***
 *
 * NOTE: Network must be up and connected to an AP before starting the service
 * NOTE: The platfrom must have an RTC for the interval update checking
 *
 * Before calling this API
 * - Network must be up and connected to an AP with
 *      access to the update server
 *
 * The OTA2 Service will periodically check and perform OTA updates
 *
 *  if no callback is registered
 *      OTA2 Service will perform default actions:
 *          - check for updates at check_interval
 *          - download updates when available
 *          - extract & perform update on next power cycle
 *  else
 *      Inform the application via callback
 *          If Application returns WICED_SUCCESS
 *              OTA Service will perform default action
 *          If application returns WICED_ERROR
 *              Application will perform action
 */
#pragma once


#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "wiced.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    OTA2_SERVICE_CHECK_FOR_UPDATE,  /* Time to check for updates.
                                    * return - WICED_SUCCESS = Service will check for update availability
                                    *        - WICED_ERROR   = Application will check for update availability   */

    OTA2_SERVICE_UPDATE_AVAILABLE,  /* Service has contacted server, update is available
                                    * return - WICED_SUCCESS = Application indicating that it wants the
                                    *                           OTA Service to perform the download
                                    *        - WICED_ERROR   = Application indicating that it will perform
                                    *                           the download, the OTA Service will do nothing.  */

    OTA2_SERVICE_DOWNLOAD_STATUS,   /* Download status - value has % complete (0-100)
                                    *   NOTE: This will only occur when Service is performing download
                                    * return - WICED_SUCCESS = Service will continue download
                                    *        - WICED_ERROR   = Service will STOP download and service will
                                    *                          issue OTA2_SERVICE_TIME_TO_UPDATE_ERROR           */

    OTA2_SERVICE_PERFORM_UPDATE,    /* Download is complete
                                    * return - WICED_SUCCESS = Service will inform Bootloader to extract
                                    *                          and update on next power cycle
                                    *        - WICED_ERROR   = Service will inform Bootloader that download
                                    *                          is complete - Bootloader will NOT extract        */

    OTA2_SERVICE_UPDATE_ERROR,      /* There was an error in transmission
                                    * This will only occur if Error during Service performing data transfer
                                    * return - WICED_SUCCESS = Service will retry immediately
                                    *        - WICED_ERROR   = Service will retry on next check_interval
                                    *            Application can call
                                    *            wiced_ota2_service_check_for_updates()
                                    *            to run another check earlier                                  */

} wiced_ota2_service_status_t;

/******************************************************
 *               Callback Function Definition
 ******************************************************/

/**
 *  Application callback for OTA service
 *  NOTE: This callback is called rather than the
 *          default checking for an update. Return value tells
 *          service how to handle the notification, or if the
 *          Application will handle the downloads - see .
 *
 * @param[in]  session - value returned from wiced_ota2_service_init()
 * @param[in]  status  - current status of service (wiced_ota2_service_status_t)
 * @param[in]  value   - value associated with status
 * @param[in]  opaque  - user supplied opaque pointer
 *
 * @return - WICED_SUCCESS  - Service will perform default action
 *           WICED_ERROR    - Application will perform action
 */
typedef wiced_result_t (*ota2_service_callback)(void* session_id,
                                               wiced_ota2_service_status_t status, int value,
                                               void* opaque );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    char*           url;                    /* url to "get" updates from                                */
    char*           file_name;              /* filename to "get"                                        */
    uint32_t        check_interval;         /* seconds between update checks                            */
    uint32_t        retry_check_interval;   /* seconds between re-try if initial contact to
                                             * server for update info fails
                                             * 0 = wait until next check_interval                       */

    uint8_t         auto_update;            /* Callback return value over-rides this parameter
                                             * Auto-update behavior if no callback registered.
                                             *   1 = Service will inform Bootloader to extract
                                             *       and update on next power cycle after download
                                             *   0 = Service will inform Bootloader that download
                                             *       is complete - Bootloader will NOT extract/update
                                             *       until user / application requests                  */
} wiced_ota2_backround_service_params_t;

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/**
 * Initialize a timed backgound service to check for updates
 *
 * @param[in]  session - value returned from wiced_ota2_service_init()
 *
 * @return - session pointer
 *           NULL indicates error
 */
void*  wiced_ota2_service_init(wiced_ota2_backround_service_params_t *params, void* opaque);

/**
 * De-initialize the service
 *
 * @param[in]  session_id - value returned from wiced_ota2_service_init()
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG
 */
wiced_result_t  wiced_ota2_service_deinit(void* session_id);

/**
 * Start the service
 *
 * @param[in]  session_id - value returned from wiced_ota2_service_init()
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG
 */
wiced_result_t  wiced_ota2_service_start(void* session_id);

/**
 * Stop the service
 *
 * @param[in]  session_id - value returned from wiced_ota2_service_init()
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG

 */
wiced_result_t  wiced_ota2_service_stop(void* session_id);

/**
 * Register or Un-register a callback function to handle the actual update check
 *
 * @param[in]  session_id  - value returned from wiced_ota2_service_init()
 * @param[in]  callback - callback function pointer (NULL to disable)
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG

 */
wiced_result_t  wiced_ota2_service_register_callback(void* session_id, ota2_service_callback update_callback);

/**
 * Force an update check now
 * NOTE: does not affect the timed checks - this is separate
 *
 * @param[in]  session_id - value returned from wiced_ota2_service_init()
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG
 */
wiced_result_t  wiced_ota2_service_check_for_updates(void* session_id);

#ifdef __cplusplus
} /*extern "C" */
#endif
