/**************************************************************************/ 
/*                                                                        */ 
/*            Copyright (c) 1996-2014 by Express Logic Inc.               */ 
/*                                                                        */ 
/*  This software is copyrighted by and is the sole property of Express   */ 
/*  Logic, Inc.  All rights, title, ownership, or other interests         */ 
/*  in the software remain the property of Express Logic, Inc.  This      */ 
/*  software may only be used in accordance with the corresponding        */ 
/*  license agreement.  Any unauthorized use, duplication, transmission,  */ 
/*  distribution, or disclosure of this software is expressly forbidden.  */ 
/*                                                                        */
/*  This Copyright notice may not be removed or modified without prior    */ 
/*  written consent of Express Logic, Inc.                                */ 
/*                                                                        */ 
/*  Express Logic, Inc. reserves the right to modify this software        */ 
/*  without notice.                                                       */ 
/*                                                                        */ 
/*  Express Logic, Inc.                     info@expresslogic.com         */
/*  11423 West Bernardo Court               http://www.expresslogic.com   */
/*  San Diego, CA  92127                                                  */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** USBX Component                                                        */ 
/**                                                                       */
/**   Device Storage Class                                                */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    ux_device_class_storage.h                           PORTABLE C      */ 
/*                                                           5.7          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    Thierry Giron, Express Logic Inc.                                   */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file contains all the header and extern functions used by the  */
/*    USBX device storage class.                                          */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  07-01-2007     TCRG                     Initial Version 5.0           */ 
/*  11-11-2008     TCRG                     Modified comment(s), and      */ 
/*                                            added new read capacity     */ 
/*                                            response constants,         */ 
/*                                            resulting in version 5.2    */ 
/*  07-10-2009     TCRG                     Modified comment(s), and      */ 
/*                                            added trace logic,          */ 
/*                                            resulting in version 5.3    */ 
/*  06-13-2010     TCRG                     Modified comment(s),          */ 
/*                                            resulting in version 5.4    */ 
/*  09-01-2011     TCRG                     Modified comment(s),          */ 
/*                                            resulting in version 5.5    */ 
/*  10-10-2012     TCRG                     Modified comment(s),          */ 
/*                                            resulting in version 5.6    */ 
/*  06-01-2014     TCRG                     Modified comment(s),          */ 
/*                                            resulting in version 5.7    */ 
/*                                                                        */ 
/**************************************************************************/ 

#ifndef UX_DEVICE_CLASS_STORAGE_H
#define UX_DEVICE_CLASS_STORAGE_H

/* Define User configurable Storage Class constants.  */

#ifndef UX_MAX_SLAVE_LUN
#define UX_MAX_SLAVE_LUN                                            2
#endif

/* Define Storage Class USB Class constants.  */

#define UX_SLAVE_CLASS_STORAGE_CLASS                                8
#define UX_SLAVE_CLASS_STORAGE_SUBCLASS_RBC                         1
#define UX_SLAVE_CLASS_STORAGE_SUBCLASS_SFF8020                     2
#define UX_SLAVE_CLASS_STORAGE_SUBCLASS_UFI                         4
#define UX_SLAVE_CLASS_STORAGE_SUBCLASS_SFF8070                     5
#define UX_SLAVE_CLASS_STORAGE_SUBCLASS_SCSI                        6
#define UX_SLAVE_CLASS_STORAGE_PROTOCOL_CBI                         0
#define UX_SLAVE_CLASS_STORAGE_PROTOCOL_CB                          1
#define UX_SLAVE_CLASS_STORAGE_PROTOCOL_BO                          0x50

/* Define Storage Class USB MEDIA types.  */
#define UX_SLAVE_CLASS_STORAGE_MEDIA_FAT_DISK                       0
#define UX_SLAVE_CLASS_STORAGE_MEDIA_CDROM                          5
#define UX_SLAVE_CLASS_STORAGE_MEDIA_OPTICAL_DISK                   7
#define UX_SLAVE_CLASS_STORAGE_MEDIA_IOMEGA_CLICK                   0x55



/* Define Storage Class SCSI command constants.  */

#define UX_SLAVE_CLASS_STORAGE_SCSI_TEST_READY                      0x00
#define UX_SLAVE_CLASS_STORAGE_SCSI_REQUEST_SENSE                   0x03
#define UX_SLAVE_CLASS_STORAGE_SCSI_FORMAT                          0x04
#define UX_SLAVE_CLASS_STORAGE_SCSI_INQUIRY                         0x12
#define UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE_SHORT                0x1a
#define UX_SLAVE_CLASS_STORAGE_SCSI_START_STOP                      0x1b
#define UX_SLAVE_CLASS_STORAGE_SCSI_PREVENT_ALLOW_MEDIA_REMOVAL     0x1e
#define UX_SLAVE_CLASS_STORAGE_SCSI_READ_FORMAT_CAPACITY            0x23
#define UX_SLAVE_CLASS_STORAGE_SCSI_READ_CAPACITY                   0x25
#define UX_SLAVE_CLASS_STORAGE_SCSI_READ16                          0x28
#define UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16                         0x2a
#define UX_SLAVE_CLASS_STORAGE_SCSI_VERIFY                          0x2f
#define UX_SLAVE_CLASS_STORAGE_SCSI_READ_TOC                        0x43
#define UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SELECT                     0x55
#define UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE                      0x5a
#define UX_SLAVE_CLASS_STORAGE_SCSI_READ32                          0xa8 
#define UX_SLAVE_CLASS_STORAGE_SCSI_WRITE32                         0xaa


/* Define Storage Class SCSI command block wrapper constants.  */

#define UX_SLAVE_CLASS_STORAGE_CBW_SIGNATURE_MASK                   0x43425355
#define UX_SLAVE_CLASS_STORAGE_CBW_SIGNATURE                        0
#define UX_SLAVE_CLASS_STORAGE_CBW_TAG                              4
#define UX_SLAVE_CLASS_STORAGE_CBW_DATA_LENGTH                      8
#define UX_SLAVE_CLASS_STORAGE_CBW_FLAGS                            12
#define UX_SLAVE_CLASS_STORAGE_CBW_LUN                              13
#define UX_SLAVE_CLASS_STORAGE_CBW_CB_LENGTH                        14
#define UX_SLAVE_CLASS_STORAGE_CBW_CB                               15
#define UX_SLAVE_CLASS_STORAGE_CBW_LENGTH                           31


/* Define Storage Class SCSI response status wrapper constants.  */

#define UX_SLAVE_CLASS_STORAGE_CSW_SIGNATURE_MASK                   0x53425355
#define UX_SLAVE_CLASS_STORAGE_CSW_SIGNATURE                        0
#define UX_SLAVE_CLASS_STORAGE_CSW_TAG                              4
#define UX_SLAVE_CLASS_STORAGE_CSW_DATA_RESIDUE                     8
#define UX_SLAVE_CLASS_STORAGE_CSW_STATUS                           12
#define UX_SLAVE_CLASS_STORAGE_CSW_LENGTH                           13


/* Define Storage Class SCSI inquiry command constants.  */ 

#define UX_SLAVE_CLASS_STORAGE_INQUIRY_OPERATION                    0
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_LUN                          1
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_PAGE_CODE                    2
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_ALLOCATION_LENGTH            4
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_COMMAND_LENGTH_UFI           12
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_COMMAND_LENGTH_SBC           06


/* Define Storage Class SCSI inquiry response constants.  */

#define UX_SLAVE_CLASS_STORAGE_INQUIRY_RESPONSE_PERIPHERAL_TYPE     0
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_RESPONSE_REMOVABLE_MEDIA     1
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_RESPONSE_DATA_FORMAT         3
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_RESPONSE_ADDITIONAL_LENGTH   4
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_RESPONSE_VENDOR_INFORMATION  8
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_RESPONSE_PRODUCT_ID          16
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_RESPONSE_PRODUCT_REVISION    32
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_RESPONSE_LENGTH              36
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_RESPONSE_LENGTH_CD_ROM       0x5b


/* Define Storage Class SCSI start/stop command constants.  */

#define UX_SLAVE_CLASS_STORAGE_START_STOP_OPERATION                 0
#define UX_SLAVE_CLASS_STORAGE_START_STOP_LBUFLAGS                  1
#define UX_SLAVE_CLASS_STORAGE_START_STOP_START_BIT                 4
#define UX_SLAVE_CLASS_STORAGE_START_STOP_COMMAND_LENGTH_UFI        12
#define UX_SLAVE_CLASS_STORAGE_START_STOP_COMMAND_LENGTH_SBC        6


/* Define Storage Class SCSI mode sense command constants.  */

#define UX_SLAVE_CLASS_STORAGE_MODE_SENSE_OPERATION                 0
#define UX_SLAVE_CLASS_STORAGE_MODE_SENSE_LUN                       1
#define UX_SLAVE_CLASS_STORAGE_MODE_SENSE_PC_PAGE_CODE              2
#define UX_SLAVE_CLASS_STORAGE_MODE_SENSE_PARAMETER_LIST_LENGTH     7
#define UX_SLAVE_CLASS_STORAGE_MODE_SENSE_COMMAND_LENGTH_UFI        12
#define UX_SLAVE_CLASS_STORAGE_MODE_SENSE_COMMAND_LENGTH_SBC        12


/* Define Storage Class SCSI request sense command constants.  */

#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_OPERATION              0
#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_LUN                    1
#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_ALLOCATION_LENGTH      4
#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_COMMAND_LENGTH_UFI     12
#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_COMMAND_LENGTH_SBC     12


/* Define Storage Class request sense response constants.  */

#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_ERROR_CODE     0
#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_SENSE_KEY      2
#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_INFORMATION    3
#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_ADD_LENGTH     7
#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_CODE           12
#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_CODE_QUALIFIER 13
#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH         18


/* Define Storage Class read capacity command constants.  */

#define UX_SLAVE_CLASS_STORAGE_READ_CAPACITY_OPERATION              0
#define UX_SLAVE_CLASS_STORAGE_READ_CAPACITY_LUN                    1
#define UX_SLAVE_CLASS_STORAGE_READ_CAPACITY_LBA                    2
#define UX_SLAVE_CLASS_STORAGE_READ_CAPACITY_COMMAND_LENGTH_UFI     12
#define UX_SLAVE_CLASS_STORAGE_READ_CAPACITY_COMMAND_LENGTH_SBC     10


/* Define Storage Class read capacity response constants.  */

#define UX_SLAVE_CLASS_STORAGE_READ_CAPACITY_RESPONSE_LAST_LBA      0
#define UX_SLAVE_CLASS_STORAGE_READ_CAPACITY_RESPONSE_BLOCK_SIZE    4
#define UX_SLAVE_CLASS_STORAGE_READ_CAPACITY_RESPONSE_LENGTH        8

/* Define Storage Class read capacity response constants.  */

#define UX_SLAVE_CLASS_STORAGE_READ_FORMAT_CAPACITY_RESPONSE_SIZE           0
#define UX_SLAVE_CLASS_STORAGE_READ_FORMAT_CAPACITY_RESPONSE_LAST_LBA       4
#define UX_SLAVE_CLASS_STORAGE_READ_FORMAT_CAPACITY_RESPONSE_DESC_CODE      8
#define UX_SLAVE_CLASS_STORAGE_READ_FORMAT_CAPACITY_RESPONSE_BLOCK_SIZE     8
#define UX_SLAVE_CLASS_STORAGE_READ_FORMAT_CAPACITY_RESPONSE_LENGTH         12

/* Define Storage Class test unit read command constants.  */

#define UX_SLAVE_CLASS_STORAGE_TEST_READY_OPERATION                 0
#define UX_SLAVE_CLASS_STORAGE_TEST_READY_LUN                       1
#define UX_SLAVE_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_UFI        12
#define UX_SLAVE_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_SBC        6


/* Define Storage Class SCSI read command constants.  */

#define UX_SLAVE_CLASS_STORAGE_READ_OPERATION                       0
#define UX_SLAVE_CLASS_STORAGE_READ_LUN                             1
#define UX_SLAVE_CLASS_STORAGE_READ_LBA                             2
#define UX_SLAVE_CLASS_STORAGE_READ_TRANSFER_LENGTH_32              6
#define UX_SLAVE_CLASS_STORAGE_READ_TRANSFER_LENGTH_16              7
#define UX_SLAVE_CLASS_STORAGE_READ_COMMAND_LENGTH_UFI              12
#define UX_SLAVE_CLASS_STORAGE_READ_COMMAND_LENGTH_SBC              10


/* Define Storage Class SCSI write command constants.  */

#define UX_SLAVE_CLASS_STORAGE_WRITE_OPERATION                      0
#define UX_SLAVE_CLASS_STORAGE_WRITE_LUN                            1
#define UX_SLAVE_CLASS_STORAGE_WRITE_LBA                            2
#define UX_SLAVE_CLASS_STORAGE_WRITE_TRANSFER_LENGTH_32             6
#define UX_SLAVE_CLASS_STORAGE_WRITE_TRANSFER_LENGTH_16             7
#define UX_SLAVE_CLASS_STORAGE_WRITE_COMMAND_LENGTH_UFI             12
#define UX_SLAVE_CLASS_STORAGE_WRITE_COMMAND_LENGTH_SBC             10


/* Define Storage Class SCSI sense key definition constants.  */

#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_NO_SENSE                   0x0
#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_RECOVERED_ERROR            0x1
#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_NOT_READY                  0x2
#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_MEDIUM_ERROR               0x3
#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_HARDWARE_ERROR             0x4
#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_ILLEGAL_REQUEST            0x5
#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION             0x6
#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_DATA_PROTECT               0x7
#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_BLANK_CHECK                0x8
#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_ABORTED_COMMAND            0x0b
#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_VOLUME_OVERFLOW            0x0d
#define UX_SLAVE_CLASS_STORAGE_SENSE_KEY_MISCOMPARE                 0x0e

/* Define Storage Class SCSI ASC return codes.  */
#define UX_SLAVE_CLASS_STORAGE_ASC_KEY_INVALID_COMMAND              0x20


/* Define Storage Class CSW status.  */

#define UX_SLAVE_CLASS_STORAGE_CSW_PASSED                           0
#define UX_SLAVE_CLASS_STORAGE_CSW_FAILED                           1
#define UX_SLAVE_CLASS_STORAGE_CSW_PHASE_ERROR                      2


/* Define generic SCSI values.  */

#define UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_ERROR_CODE_VALUE  0x70
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_PAGE_CODE_STANDARD               0x00
#define UX_SLAVE_CLASS_STORAGE_INQUIRY_PERIPHERAL_TYPE                  0x00
#define UX_SLAVE_CLASS_STORAGE_RESET                                    0xff
#define UX_SLAVE_CLASS_STORAGE_GET_MAX_LUN                              0xfe
#define UX_SLAVE_CLASS_STORAGE_MAX_LUN                                  0
#define UX_SLAVE_CLASS_STORAGE_RESPONSE_LENGTH                          64


/* Define buffer length for IN/OUT pipes.  This should match the size of the endpoint maximum buffer size. */

#define UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE                              UX_SLAVE_REQUEST_DATA_MAX_LENGTH


/* Define Slave Storage Class LUN structure.  */

typedef struct UX_SLAVE_CLASS_STORAGE_LUN_STRUCT
{
    ULONG           ux_slave_class_storage_media_last_lba;
    ULONG           ux_slave_class_storage_media_block_length;
    ULONG           ux_slave_class_storage_media_type;
    ULONG           ux_slave_class_storage_media_removable_flag;
    ULONG           ux_slave_class_storage_media_id;
    ULONG           ux_slave_class_storage_scsi_tag;
    UCHAR           ux_slave_class_storage_request_sense_key;
    UCHAR           ux_slave_class_storage_request_code;
    UCHAR           ux_slave_class_storage_request_code_qualifier;
    UINT            (*ux_slave_class_storage_media_read)(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
    UINT            (*ux_slave_class_storage_media_write)(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
    UINT            (*ux_slave_class_storage_media_status)(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status);
} UX_SLAVE_CLASS_STORAGE_LUN;

/* Define Slave Storage Class structure.  */

typedef struct UX_SLAVE_CLASS_STORAGE_STRUCT
{
    UX_SLAVE_INTERFACE          *ux_slave_class_storage_interface;
    ULONG                       ux_slave_class_storage_number_lun;
    UX_SLAVE_CLASS_STORAGE_LUN  ux_slave_class_storage_lun[UX_MAX_SLAVE_LUN];

} UX_SLAVE_CLASS_STORAGE;

/* Define Slave Storage Class Calling Parameter structure */

typedef struct UX_SLAVE_CLASS_STORAGE_PARAMETER_STRUCT
{
    ULONG                       ux_slave_class_storage_parameter_number_lun;
    UX_SLAVE_CLASS_STORAGE_LUN  ux_slave_class_storage_parameter_lun[UX_MAX_SLAVE_LUN];

} UX_SLAVE_CLASS_STORAGE_PARAMETER;

/* Define Device Storage Class prototypes.  */

UINT    _ux_device_class_storage_initialize(UX_SLAVE_CLASS_COMMAND *command);
UINT    _ux_device_class_storage_activate(UX_SLAVE_CLASS_COMMAND *command);
VOID    _ux_device_class_storage_control_request(UX_SLAVE_CLASS_COMMAND *command);
UINT    _ux_device_class_storage_csw_send(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in, UCHAR csw_status);
UINT    _ux_device_class_storage_deactivate(UX_SLAVE_CLASS_COMMAND *command);
UINT    _ux_device_class_storage_entry(UX_SLAVE_CLASS_COMMAND *command);
UINT    _ux_device_class_storage_format(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
UINT    _ux_device_class_storage_inquiry(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
UINT    _ux_device_class_storage_mode_select(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
UINT    _ux_device_class_storage_mode_sense(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
UINT    _ux_device_class_storage_prevent_allow_media_removal(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, 
                    UX_SLAVE_ENDPOINT *endpoint_in, UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
UINT    _ux_device_class_storage_read(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb, UCHAR scsi_command);
UINT    _ux_device_class_storage_read_capacity(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
UINT    _ux_device_class_storage_read_format_capacity(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
UINT    _ux_device_class_storage_read_toc(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                                            UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
UINT    _ux_device_class_storage_request_sense(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
UINT    _ux_device_class_storage_start_stop(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
UINT    _ux_device_class_storage_test_ready(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
VOID    _ux_device_class_storage_thread(ULONG storage_instance);
UINT    _ux_device_class_storage_verify(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb);
UINT    _ux_device_class_storage_write(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                    UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb, UCHAR scsi_command);

/* Define Device Storage Class API prototypes.  */

#define ux_device_class_storage_entry        _ux_device_class_storage_entry

#endif
