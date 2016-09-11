/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** USBX Component                                                        */
/**                                                                       */
/**   Utility                                                             */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"


/**
 *  LwIP init complete callback
 *
 *  This function is called by LwIP when initialisation has finished.
 *  A semaphore is posted to allow the startup thread to resume, and to run the app_main function
 *
 *  @param arg : the handle for the semaphore to post (cast to a void pointer)
 */
UINT _ux_utility_device_dump(UX_DEVICE *device)
{
	if(device == UX_NULL)
	{
		return (UX_MEMORY_INSUFFICIENT);
	}

	printf("\n");

	printf("#UX#: Device:\n");
	printf("\t State: %lu\n", device->ux_device_state);
	printf("\t Address: %lu\n", device->ux_device_address);
	printf("\t HCD name: %s\n", device->ux_device_hcd->ux_hcd_name);
	printf("\t Class name: %s\n", device->ux_device_class->ux_host_class_name);
	printf("\t Speed: ");
	switch(device->ux_device_speed)
	{
		case UX_LOW_SPEED_DEVICE:
			printf("0 (LOW)\n");
			break;
		case UX_FULL_SPEED_DEVICE:
			printf("1 (FULL)\n");
			break;
		case UX_HIGH_SPEED_DEVICE:
			printf("2 (HIGH)\n");
			break;
		default:
			printf("%lu (Unknown)\n", device->ux_device_speed);
			break;
	}
	printf("\t Port Index: %lu\n", device->ux_device_port_location);
	printf("\t Max Power: %lu\n", device->ux_device_max_power);

    printf("\n");
    return (UX_SUCCESS);
}

