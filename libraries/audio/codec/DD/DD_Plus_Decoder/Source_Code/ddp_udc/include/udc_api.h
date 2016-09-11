
/******************************************************************************
*   This program is protected under international and U.S. copyright laws as
*   an unpublished work. This program is confidential and proprietary to the
*   copyright owners. Reproduction or disclosure, in whole or in part, or the
*   production of derivative works therefrom without the express permission 
*   of the copyright owners is prohibited.
*
*                Copyright (C) 2003-2015 by Dolby Laboratories.
*                            All rights reserved.
*
*   Module:     Unified Decoder Converter API
*
*   File:       udc_api.h
*
\***************************************************************************/

/*!
*  \file udc_api.h
*
*   \brief UDC API header file.
*
*  Part of the Unified Decoder Converter. */

#if !defined(UDC_API_H)
#define UDC_API_H

#include "dlb_buffer.h"

/****  C++ Compatibility ****/
#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

/**** API equates and enumerations ****/
#define DDPI_UDC_API_VERSION                    18

#define DDPI_UDC_NUM_OUTMODES                   17     /*!< Number of 7.1 ch output modes (-m option) */
#define DDPI_UDC_OUTMODE_LTSRTS                 28     /*!< Lts/Rts output mode in bitstream */
#define DDPI_UDC_OUTMODE_RAW                    -1     /*!< "raw" output mode all channels in bitstream */
#define DDPI_UDC_OUTMODE                        {-1, 0, 1, 2, 3, 4, 5, 6, 7, 13, 14, 16, 17, 18, 19, 21, 28}
    /*!< Supported output mode list
        -1 = raw mode (outputs all standard (non-replaced) channels in 7.1-channel bitstream; no channel mixing or remapping performed)
         0 = reserved
         1 = 1/0 (C)
         2 = 2/0 (L, R)
         3 = 3/0 (L, R, C)
         4 = 2/1 (L, R, l)
         5 = 3/1 (L, R, C, l)
         6 = 2/2 (L, R, l, r)
         7 = 3/2 (L, R, C, l, r) (default)
         13 = 2/2/2 (L, R, l, r, Lw, Rw)
         14 = 2/2/2 (L, R, l, r, Lvh, Rvh)
         16 = 2/2/2 (L, R, l, r, Lrs, Rrs)
         17 = 3/2/2 (L, R, C, l, r, Lc, Rc)
         18 = 3/2/2 (L, R, C, l, r, Lw, Rw)
         19 = 3/2/2 (L, R, C, l, r, Lvh, Rvh)
         21 = 3/2/2 (L, R, C, l, r, Lrs, Rrs)
         28 = 3/2/2 (L, R, C, l, r, Lts, Rts)
    */
#define DDPI_UDC_MAXPCMOUTCHANS                 8      /*!< Maximum number of channels per individual output */
#define DDPI_UDC_MAXSIMPCMOUTCHANS              6      /*!< Maximum number of channels per individual output for simultaneous PCM output */
#define DDPI_UDC_MINSCALEFACTOR                 0      /*!< Minimum scale factor */
#define DDPI_UDC_MAXSCALEFACTOR                 100    /*!< Maximum scale factor */
#define DDPI_UDC_ELEMENT_NOT_PRESENT            -1     /*!< Value returned when bit stream element is not present and no default was used */

#define DDPI_UDC_MAX_DUALMONO_CHANS             2      /*!< Maximum number of dual mono channels */
#define DDPI_UDC_MAX_NUM_SIM_OUTPUTS            8      /*!< Maximum number of simultaneous discrete PCM outputs */
#define DDPI_UDC_MAX_BLOCKS_PER_FRM             6      /*!< Maximum number of blocks per frame */
#define DDPI_UDC_MAX_INDEP_FRMS                 1      /*!< Maximum number of independent frames per program */
#define DDPI_UDC_MAX_DEPEN_FRMS                 8      /*!< Maximum number of dependent frames per program */
#define DDPI_UDC_MAX_FRMS_PER_PGRM              (DDPI_UDC_MAX_INDEP_FRMS + DDPI_UDC_MAX_DEPEN_FRMS)         /*!< Maximum number of frames per program */
#define DDPI_UDC_MAX_PRGMS                      8      /*!< Maximum number of programs per time slice */
#define DDPI_UDC_MAX_PRGMS_DECODED              2      /*!< Maximum number of programs this component can simultaneously decode */
#define DDPI_UDC_MAX_FRMS_PER_TS                (DDPI_UDC_MAX_FRMS_PER_PGRM * DDPI_UDC_MAX_PRGMS) /*!< Maximum number of frames per time slice */
#define DDPI_UDC_MAX_ADDBSI_BYTES               64     /*!< Maximum number of bytes for addbsi */
#define DDPI_UDC_MAX_MIXDEFLEN_BYTES            33     /*!< Maximum number of bytes defined by mixdeflen */
#define DDPI_UDC_MAX_SKIPFLD_BYTES              512    /*!< Maximum number of bytes for skipfld */
#define DDPI_UDC_MAX_AUXDATA_BYTES              2048   /*!< Maximum number of bytes for auxdata */
#define DDPI_UDC_MIN_MEMORY_ALIGNMENT           32     /*!< Minimum memory alignment necessary for correct operation of UDC */
#define DDPI_UDC_MIN_PCM_MIXING_LEVEL           -32    /*!< Minimum PCM mixing level of UDC */
#define DDPI_UDC_MAX_PCM_MIXING_LEVEL           32     /*!< Maximum PCM mixing level of UDC */

/**** API Error Codes ****/
#define DDPI_UDC_ERR_NO_ERROR                                    0
#define DDPI_UDC_ERR_GENERIC                                     1001
#define DDPI_UDC_ERR_DEBUG                                       1002
#define DDPI_UDC_ERR_UNSUPPORTED_PARAM                           1003
#define DDPI_UDC_ERR_INVALID_PARAM                               1004
#define DDPI_UDC_ERR_INVALID_PARAM_SIZE                          1005
#define DDPI_UDC_ERR_ADDFRAME_COMPLETETS                         1006
#define DDPI_UDC_ERR_INTERNAL_BUFFER                             1007
#define DDPI_UDC_ERR_PROGRAMCHANGE                               1008
#define DDPI_UDC_ERR_PROCESSING                                  1010
#define DDPI_UDC_ERR_INVALID_TIMESLICE                           1011
#define DDPI_UDC_ERR_PROG_INCONSISTENT                           1012
#define DDPI_UDC_ERR_PROG_MISSING                                1013
#define DDPI_UDC_ERR_INVALID_LICDATA                             1016    /*!< Invalid license data */
#define DDPI_UDC_ERR_INVALID_LICTYPE                             1017    /*!< Invalid license type - LICTYPE_NONE */
#define DDPI_UDC_ERR_INVALID_INPUT_FOR_EVAL                      1018    /*!< Invalid input stream for evaluation mode */
#define DDPI_UDC_ERR_FRAME_NOT_PRESENT                           1019    /*!< Metadata function called on frame that was not present */
#define DDPI_UDC_ERR_MDAT_NOT_AVAILABLE                          1020    /*!< Metadata not available for requested frame */
#define DDPI_UDC_ERR_MDAT_CORRUPT                                1021    /*!< Metadata corrupt for requested frame */
#define DDPI_UDC_ERR_MDAT_INVALID_TIMESLICE                      1022    /*!< Invalid time slice, no metadata available */
#define DDPI_UDC_ERR_MDAT_INCOMPLETE_TIMESLICE                   1023    /*!< Incomplete time slice, no metadata available */
#define DDPI_UDC_ERR_INVALID_MDAT_ID                             1024    /*!< Invalid metadata ID, no metadata available */
#define DDPI_UDC_ERR_MEMORY_NOT_ALIGNED                          1025    /*!< Memory must be 32-bit aligned */
#define DDPI_UDC_ERR_EVO_MDAT_PROGRAM_ID                         1026    /*!< Invalid program ID, exceeds max program ID */
#define DDPI_UDC_ERR_EVO_GENERAL                                 1028    /*!< General error from Evolution decoder module */
#define DDPI_UDC_ERR_INTLOUD_GENERAL                             1029    /*!< General error from intelligent loudness decoder module */
#define DDPI_UDC_ERR_ABKMDAT_UNDER_CONVERTER                     1030    /*!< Extracting audio block metadata when converter is enabled is not supported */
#define DDPI_UDC_ERR_JOC_DECODER                                 1031    /*!< General error from internal JOC decoder */
#define DDPI_UDC_ERR_CONFIG_JOC_DECODER                          1032    /*!< Error when configuring the internal JOC decoder */
#define DDPI_UDC_ERR_MIXER_UNDO                                  1033    /*!< Undo mixing process when decoding configuration does not match mixer condition */
#define DDPI_UDC_ERR_MIXER_PROCESS                               1034    /*!< Error from mixing process */
#define DDPI_UDC_ERR_MIXER_UNSUPPORTED_PARAM                     1035    /*!< Invalid parameter for mixer */
#define DDPI_UDC_ERR_OUTPUT_DATA_TYPE_SIZE_BIGGER_THAN_LFRACT    1036    /*!< The output data type size bigger than LFRACT */

#define DDPI_UDC_WARN_MIXING_DATA                                2001    /*!< Mixing data doesn't match the bit length */

/* Channel location in UDC, matches result produced in outputmap[](a member of structure ddpi_udc_pt_op) */
#define DDPI_UDC_CHANNEL_L     (0x8000 >> 0)  /*!< Left Channel location */
#define DDPI_UDC_CHANNEL_C     (0x8000 >> 1)  /*!< Center Channel location */
#define DDPI_UDC_CHANNEL_R     (0x8000 >> 2)  /*!< Right Channel location */
#define DDPI_UDC_CHANNEL_l     (0x8000 >> 3)  /*!< Left Surround Channel location */
#define DDPI_UDC_CHANNEL_r     (0x8000 >> 4)  /*!< Right Surround Channel location */
#define DDPI_UDC_CHANNEL_Cpr   (0x8000 >> 5)  /*!< Center Pair (Lc/Rc) Channel locations*/
#define DDPI_UDC_CHANNEL_RSpr  (0x8000 >> 6)  /*!< Rear Surround pair (Lrs/Rrs) Channel locations*/
#define DDPI_UDC_CHANNEL_Cs    (0x8000 >> 7)  /*!< Center Surround (Cs) Channel location */
#define DDPI_UDC_CHANNEL_Ts    (0x8000 >> 8)  /*!< Top Surround (Ts) Channel location */
#define DDPI_UDC_CHANNEL_SDpr  (0x8000 >> 9)  /*!< Surround Directs (Lsd/Rsd) Channel locations*/
#define DDPI_UDC_CHANNEL_Wpr   (0x8000 >> 10) /*!< Wide pair (Lw/Rw) Channel locations*/
#define DDPI_UDC_CHANNEL_VHpr  (0x8000 >> 11) /*!< Vertical High Pair (Lvh/Rvh) Channel locations*/
#define DDPI_UDC_CHANNEL_Cvh   (0x8000 >> 12) /*!< Center Vertical High (Cvh) Channel location */
#define DDPI_UDC_CHANNEL_TSpr  (0x8000 >> 13) /*!< Top Surround Pair (Lts/Rts) Channel locations*/
#define DDPI_UDC_CHANNEL_LFE2  (0x8000 >> 14) /*!< Secondary LFE (LFE2) Channel location */
#define DDPI_UDC_CHANNEL_LFE   (0x8000 >> 15) /*!< LFE Channel location */

/* UDC control parameter identification for use with ddpi_udc_setprocessparam and ddpi_udc_getprocessparam, 
        starting from DDPI_UDC_CTL_RUNNING_MODE_ID to DDPI_UDC_CTL_FORCE_JOC_OUTPUT_DMX_ID. */

#define DDPI_UDC_CTL_RUNNING_MODE_ID            0
    /*!< Decoding and converting running mode. This parameter is purely static, that is, it shall be set only once at initialization stage and 
        cannot be changed dynamically at run time. Changes to running mode should be made by reinitializing the unified decoder-converter instance.
        There are 5 modes, check the structure DDPI_UDC_RUNNING_MODE for more information.
         - DDPI_UDC_DECODE_ONLY = Decoding only mode
         - DDPI_UDC_CONVERT_ONLY = Converting only mode
         - DDPI_UDC_DECODE_CONVERT = Decoding and converting mode 
         - DDPI_UDC_JOC_DECODE_PCM_OUT = JOC decoding mode, PCM output
         - DDPI_UDC_JOC_DECODE_QMF_OUT = JOC decoding mode, QMF output
         */
#define DDPI_UDC_CTL_ERRORCONCEAL_ID            2
    /*!< Decoder error concealment enable flag. This parameter is invalid in converting only mode.
         - 0 = Decoder error concealment disabled.
         - 1 = Decoder error concealment enabled. If an error occurs that prevents the frame from being decoded (for example,
               CRC frame error or decoding error), then error concealment is performed. The error concealment
               strategy fills the output PCM buffers with data repeated from the last block decoded without error
               until the maximum repeat count is reached. After that, the output buffers are filled with mute data. */
#define DDPI_UDC_CTL_ERRORMAXRPTS_ID            3
    /*!< Decoder error max block repeats. The maximum number of consecutive block repeats that should be allowed before the
         output channels are automatically muted. If this parameter is zero, then block repeats continue indefinitely. 
         This parameter is invalid in converting only mode. */
#define DDPI_UDC_CTL_CNV_ERRORCONCEAL_ID        4
    /*!< Converter error concealment enable flag.
         This parameter is only valid when converter is enabled in converting only mode or decoding and converting mode.
         - 0 = Converter error concealment disabled.
         - 1 = Converter error concealment enabled. When there are errors (for example, bitstream errors or CRC errors) within a frame, 
               the converter generates a DD frame with a wrong CRC (intentionally wrong) to force a frame error when it is decoded 
               in the downstream decoder. In this way, the downstream decoder will decide how to handle the error. */
#define DDPI_UDC_CTL_EXTBOOST_ID                5
    /*!< The external boost value. Please note that this parameter can only be got by ddpi_udc_getprocessparam() and not be set by ddpi_udc_setprocessparam(). */
#define DDPI_UDC_CTL_SUBSTREAMSELECT_ID         6
    /*!< ID number of associated substream to select for decode, value refers to enum DDPI_UDC_SUBSTREAMID. 
        This parameter is only valid in dual decoding mode.
        - DDPI_UDC_SUBSTREAMID_UNDEFINED = Disable associate decoding.
        - DDPI_UDC_SUBSTREAMID_0 = Select substream 0 for associate decoding(default value for dual input mode, equal to DDPI_UDC_SUBSTREAMID_UNDEFINED in single input mode).
        - DDPI_UDC_SUBSTREAMID_1 = Select substream 1 for associate decoding(default value for single input mode, equal to DDPI_UDC_SUBSTREAMID_0 in dual input mode).
        - DDPI_UDC_SUBSTREAMID_2 = Select substream 2 for associate decoding(equal to DDPI_UDC_SUBSTREAMID_0 in dual input mode).
        - DDPI_UDC_SUBSTREAMID_3 = Select substream 3 for associate decoding(equal to DDPI_UDC_SUBSTREAMID_0 in dual input mode). */
#define DDPI_UDC_CTL_INPUTMODE_ID               7
    /*!< Single or dual input mode for dual-stream decoding. This parameter is static, that is, it shall be set only once at initialization stage 
         and cannot be changed dynamically at run time. Changes to input mode should be made by reinitializing the unified decoder-converter instance. 
         Check the enum DDPI_UDC_INPUTMODE for supported modes. 
         - DDPI_UDC_INPUTMODE_SINGLEINPUT = Single input mode.
         - DDPI_UDC_INPUTMODE_DUALINPUT = Dual input mode (Only valid in dual decoding mode). */
#define DDPI_UDC_CTL_EVOMODE_ID                 8
    /*!< Evolution decoder mode, value refers to enum DDPI_UDC_EVOLUTION_MODE. 
         Please note that Hash proctection will be disabled in JOC decoding mode.
         - DDPI_UDC_EVO_DISABLE_PCMHASH = Disable Hash protection in outbound Evolution data 
         - DDPI_UDC_EVO_ENABLE_PCMHASH = Enable Hash protection in outbound Evolution data (default)*/
#define DDPI_UDC_CTL_EVOQUICK_SWITCH            9
    /*!< The flag to switch on/off the Evolution framework quick access functionality. 
         Please note that Evolution quick access is not supported in dual input decoding mode.
         - 0 = Disable Evolution framework quick access functionality
         - 1 = Enable Evolution framework quick access functionality */
/*!< The combination of DDPI_UDC_CTL_EVOQUICK_SUBSTREAM_ID and DDPI_UDC_CTL_EVOQUICK_STREAMTYPE defines the substream in which 
     Evolution framework metadata is quick accessed. For example, when set DDPI_UDC_CTL_EVOQUICK_SUBSTREAM_ID as 0 and 
     DDPI_UDC_CTL_EVOQUICK_STREAMTYPE as DDPI_UDC_SUBSTREAM_TYPE_DEP, it means the decoder quick accesses the Evolution metadata in D0. 
     The supported substreams are I0, I1, I2, I3, D0. */
#define DDPI_UDC_CTL_EVOQUICK_SUBSTREAM_ID      10 /*!< The substream ID for Evolution framework quick access, supported IDs range from 0 to 3. 
                                                    Please note that Evolution quick access does not support dual input decoding mode. */
#define DDPI_UDC_CTL_EVOQUICK_STREAMTYPE        11 /*!< The substream type for Evolution framework quick access, check DDPI_UDC_SUBSTREAM_TYPE for the supported types */
#define DDPI_UDC_CTL_MIXPREF_ID                 12 
    /*!<  User balance adjustment for mixing main and associated audio. This parameter is only valid when mixer is enabled in dual decoding mode.
          -32: Associated fully muted\n
          -1..-31: dB to favor main program (attenuate associated)\n
          0: Neutral (no balance adjustment)\n
          1..31: dB to favor associated program (attenuate main)\n
          32: Main fully muted\n */
#define DDPI_UDC_CTL_MIXER_SWITCH_ID            13 /*!< The switcher for mixer on/off. This parameter is only vaild in dual decoding mode. */
#define DDPI_UDC_CTL_FORCE_JOC_OUTPUT_DMX_ID     14
    /*!< Force UDC to output downmix when UDC is decoding JOC content, no matter JOC metadata exists or not
         This parameter is only valid in JOC decoding mode. 
         - 0 = Disable force, JOC will output object or downmix based on the JOC metadata exists or not
         - 1 = Enable force, JOC will always output downmix content */

#define DDPI_UDC_CTL_ID_MAX                     15 /*!< Maximum number of process control parameters */

/* UDC control parameter identification for use with ddpi_udc_setoutparam and ddpi_udc_getoutparam, starting from DDPI_UDC_OUTCTL_OUTMODE_ID 
        to DDPI_UDC_OUTCTL_DECORRELATOR_ID. Please note that all these parameters are invalid in converting only mode.*/

#define DDPI_UDC_OUTCTL_OUTMODE_ID              0
    /*!< Decoder output mode. If the output mode does not equal the bitstream audio coding mode (acmod), then the decoder will perform downmixing 
         or fill channels with zero values to meet the desired output configuration. Supported output mode is listed in DDPI_UDC_OUTMODE. 
         Please note that in JOC decoding mode, the output mode is forced to raw output mode 
         and with some channel remapping performed */
#define DDPI_UDC_OUTCTL_OUTLFEON_ID             1
    /*!< Decoder output LFE on, check the enum DDPI_UDC_LFEOUTMODES for more information. 
         - If <code>OUTLFEON</code> is DDPI_UDC_LFEOUTOFF, the LFE channel is not included in the output PCM data. <br>
         - If <code>OUTLFEON</code> is DDPI_UDC_LFEOUTSINGLE, then the LFE channel is included in the output PCM data if it exists in the bitstream.
         Please note that in JOC decoding mode, the LFE output mode is forced to DDPI_UDC_LFEOUTSINGLE. */
#define DDPI_UDC_OUTCTL_COMPMODE_ID             2
    /*!< Decoder compression mode, check the enum DDPI_UDC_COMP for supported modes. 
         Please note that portable mode(compression mode is DDPI_UDC_COMP_PORTABLE_L8, DDPI_UDC_COMP_PORTABLE_L11 or DDPI_UDC_COMP_PORTABLE_L14) 
         is invalid in JOC decoding mode.
         - DDPI_UDC_COMP_CUSTOM_0 = Custom mode (no digital dialogue normalization)
         - DDPI_UDC_COMP_CUSTOM_1 = 2/0 Dolby Surround mode
         - DDPI_UDC_COMP_LINE = Line out mode
         - DDPI_UDC_COMP_RF = RF mode
         - DDPI_UDC_COMP_PORTABLE_L8 = Portable mode -8dB 
         - DDPI_UDC_COMP_PORTABLE_L11 = Portable mode -11dB 
         - DDPI_UDC_COMP_PORTABLE_L14 = Portable mode -14dB */
#define DDPI_UDC_OUTCTL_STEREOMODE_ID           3
    /*!< Decoder stereo output mode.  This parameter specifies the reproduction mode associated with stereo
         output (output mode = 2/0). Check DDPI_UDC_STEREOMODE structure for supported modes. 
         - DDPI_UDC_STEREOMODE_AUTO = Automatically detects stereo mode based on setting in the bit stream.
         - DDPI_UDC_STEREOMODE_SRND = 2/0 Dolby Surround compatible (Lt/Rt)
         - DDPI_UDC_STEREOMODE_STEREO = 2/0 Stereo (Lo/Ro) */
#define DDPI_UDC_OUTCTL_DUALMODE_ID             4
    /*!< Decoder dual mono mode (Only valid when audio coding mode is 1+1), check the enum DDPI_UDC_DUALMONO_DOWNMIX for supported modes.
         - DDPI_UDC_DUAL_STEREO = Stereo
         - DDPI_UDC_DUAL_LEFTMONO = Left mono
         - DDPI_UDC_DUAL_RGHTMONO = Right mono
         - DDPI_UDC_DUAL_MIXMONO = Mixed mono */
#define DDPI_UDC_OUTCTL_DRCSCALEHIGH_ID         5
    /*!< Decoder dynamic range cut scale factor (for high level signals).
         This is a 32-bit integer value between 0 and 100. This number is used to scale the dynamic range control
         value for high-level signals that would otherwise be reduced. A value of 0 disables high-level compression.
         A value of 100 indicates full scale of DRC cut parameter. This parameter is ignored if the compression mode is set to RF mode or portable mode. */
#define DDPI_UDC_OUTCTL_DRCSCALELOW_ID          6
    /*!< Decoder dynamic range boost scale factor (for low level signals).
         This is the same as DDPI_UDC_OUTCTL_DRCSCALEHIGH_ID, except that it is applied to boost low-level signals. */
#define DDPI_UDC_OUTCTL_OUTPCMSCALE_ID          7
    /*!< Decoder output PCM scale factor.  This is a fractional number between 0.0 and 1.0, which is used to scale 
         the final output PCM data prior to writing to the PCM output buffers. A value of 0.0 mutes the PCM output.*/
#define DDPI_UDC_OUTCTL_MDCTBANDLIMIT_ID        8   /*!< MDCT bandlimiting mode */
#define DDPI_UDC_OUTCTL_DRCSUPPRESS_ID          9  /*!< Suppress bitstream DRC*/
#define DDPI_UDC_OUTCTL_DECORRELATOR_ID         10 
    /*!< MDCT decorrelator mode, check the enum DDPI_UDC_DECORR_MODE for more information. 
         - DDPI_UDC_DECORR_OFF = No decorrelation.
         - DDPI_UDC_DECORR_UNGUIDED = Unguided decorrelation. 
         - DDPI_UDC_DECORR_ADAPTIVE = Adaptive decorrelation (Only valid in decoding only mode or decoding and converting mode; 
                                                              deactivate the decorrelation for JOC content if the output mode matches the input audio coding mode(acmod),
                                                              otherwise, the decoder will activate unguided decorrelation automatically.)
         Please note that decorrelator is forced to be deactivated for JOC content in JOC decoding mode. */
#define DDPI_UDC_OUTCTL_ID_MAX                  11

/* Frame status */
#define DDPI_UDC_FRM_STATUS_UNKNOWN             -1  /*!< Unknown CRC status; CRC was not run on this frame */
#define DDPI_UDC_FRM_STATUS_VALID                0  /*!< Entire frame is valid; no CRC errors */
#define DDPI_UDC_FRM_STATUS_FULL_ERR             1  /*!< Full frame error; CRC2 failed */
#define DDPI_UDC_FRM_STATUS_PARTIAL_ERR          2  /*!< Partial frame error (first 5/8ths of frame is valid); CRC1 passed but CRC2 failed (Only applicable for DD) */

/* Metadata id used with ddpi_udc_getframe_mdat() to identify which metadata to extract */
#define DDPI_UDC_BSI_MDAT_ID                     0  /*!< Identifies void pointer as ddpi_udc_bsi_mdat */
#define DDPI_UDC_AUDBLK_MDAT_ID                  1  /*!< Identifies void pointer as ddpi_udc_audblk_mdat */
#define DDPI_UDC_AUXDATA_MDAT_ID                 2  /*!< Identifies void pointer as ddpi_udc_auxdata_mdat */

/* Maximum objects and output channels supported by the JOC decoder */
#define DDPI_JOCD_MAX_NUM_OBJECTS               15
#define DDPI_JOCD_MAX_CHANNELS                  ((DDPI_JOCD_MAX_NUM_OBJECTS)+1)
/* JOC QMF output parameters */
#define DDPI_JOCD_QMF_MAX_SLOTS                 24  /*!< JOC QMF output max slots per frame */
#define DDPI_JOCD_QMF_BANDS                     64  /*!< JOC QMF output bands */
#define DDPI_JOCD_QMF_SLOTS_PER_BLOCK           4   /*!< JOC QMF output slots per block */
#define DDPI_JOCD_QMF_SCALE_BITS                4   /*!< JOC QMF output scale bits */

/* Define the maximum size (in bytes) of the serialized Evolution framework output */
#define DDPI_UDC_MAX_SERIALIZED_EVOLUTION_SIZE  3066

/* Define the latency introduced by mixer */
#define DDPI_UDC_MIXER_DELAY_SAMPLES            288

/* Define the value to indicate that the parameter is invalid in current running mode */
#define DDPI_UDC_PARM_INVALID                   -1

/*! \brief Running mode */
typedef enum
{
    DDPI_UDC_DECODE_ONLY = 0,
    DDPI_UDC_CONVERT_ONLY,
    DDPI_UDC_DECODE_CONVERT,
    DDPI_UDC_JOC_DECODE_PCM_OUT,
    DDPI_UDC_JOC_DECODE_QMF_OUT
} DDPI_UDC_RUNNING_MODE;

/*! \brief UDC JOC decoding mode */
typedef enum
{
    DDPI_UDC_JOCD_DISABLE    = 0,   /*!< Disable the JOC decoding */
    DDPI_UDC_JOCD_PCM_OUT    = 1,   /*!< PCM output for the JOC decoding */
    DDPI_UDC_JOCD_QMF_OUT    = 2    /*!< QMF output for the JOC decoding */
} DDPI_UDC_JOCD_MODE;

/*! \brief UDC JOC output content type */
typedef enum
{
    DDPI_UDC_JOCD_UNDEFINE_OUT = -1, /*!< The frame output is undefined */
    DDPI_UDC_JOCD_OBJ_OUT = 0,       /*!< The frame output is JOC object content */
    DDPI_UDC_JOCD_DMX_OUT            /*!< The frame output is downmixed content */
} DDPI_UDC_JOCD_OUT_MODE;

/*! \brief JOC decoder status */
typedef enum
{
    DDPI_UDC_JOCD_STATUS_NOERR = 0,      /*!< No error status */
    DDPI_UDC_JOCD_STATUS_NO_JOC_MD,      /*!< No JOC metadata in present frame bitstream */
    DDPI_UDC_JOCD_STATUS_EXTERNAL_ERR,   /*!< Error during the extraction of JOC metadata from the Evolution container or error downmix bitstream configuration before the JOC decoding */
    DDPI_UDC_JOCD_STATUS_INTERNAL_ERR,   /*!< Internal error inside the JOC decoder */
    DDPI_UDC_JOCD_STATUS_IGNORE_JOC_MD   /*!< Ignoring JOC metadata which will happen in force downmix case */
} DDPI_UDC_JOCD_STATUS;

/*! \brief Object Audio metadata status */
typedef enum {
     DDPI_UDC_OAMD_STATUS_VALID = 0,    /*!< No error status */
     DDPI_UDC_OAMD_STATUS_NO_DATA,      /*!< No OAMD applied to present frame */ 
     DDPI_UDC_OAMD_STATUS_ERROR,        /*!< Error during the extraction of OAMD information */
     DDPI_UDC_OAMD_STATUS_UNMATCH       /*!< OAMD doesn't match the JOC output */
} DDPI_UDC_OAMD_STATUS;

/*! \brief UDC decoder status */
typedef enum
{
    DDPI_UDC_DEC_STATUS_NOERR = 0,          /*!< Indicates no error detected in 
                                                independent or dependent frame */
    DDPI_UDC_DEC_STATUS_IND_PARTIALERR_DEP_NOERROR,
                                            /*!< Indicates partial frame error detected in 
                                                independent frame, no error in dependent */
    DDPI_UDC_DEC_STATUS_IND_FULLERR_DEP_NOERROR,
                                            /*!< Indicates full frame error detected in 
                                                independent frame, no error in dependent */
    DDPI_UDC_DEC_STATUS_IND_NOERR_DEP_FULLERROR,
                                            /*!< Indicates no error detected in independent
                                                frame, full frame error in dependent */
    DDPI_UDC_DEC_STATUS_IND_PARTIALERR_DEP_FULLERROR,
                                            /*!< Indicates partial frame error detected in 
                                                independent frame, full frame error in dependent */
    DDPI_UDC_DEC_STATUS_IND_FULLERR_DEP_FULLERROR
                                            /*!< Indicates full frame errors detected in 
                                                both independent and dependent frames */
} DDPI_UDC_DEC_STATUS;

/*! \brief Evolution Data status */
typedef enum
{
    DDPI_UDC_EVO_STATUS_VALID = 0,          /*!< Indicates that the Evolution data is found and valid */
    DDPI_UDC_EVO_STATUS_NO_DATA,            /*!< Indicates that no Evolution data found, and no previous delayed payload */
    DDPI_UDC_EVO_STATUS_ERROR,              /*!< Indicates certain error occurs in Evolution */
}DDPI_UDC_EVO_STATUS;

/*! \brief Intelligent Loudness Data status */
typedef enum
{
    DDPI_UDC_INTLOUD_STATUS_NO_FOUND = 0,   /*!< Indicates that no Intelligent loudness metadata found */
    DDPI_UDC_INTLOUD_STATUS_OMITTED,        /*!< Indicates that Intelligent loudness metadata will be omitted when decoder is in downmix mode*/
    DDPI_UDC_INTLOUD_STATUS_VALID,          /*!< Indicates that the Intelligent loudness metadata is found and valid */
    DDPI_UDC_INTLOUD_STATUS_INVALID,        /*!< Indicates the Intelligent loudness metadata is invalid */
}DDPI_UDC_INTLOUD_STATUS;

/*! \brief UDC converter status */
typedef enum
{
    DDPI_UDC_CNV_STATUS_NO_DATA = 0,        /*!< Indicates no converted metadata available */
    DDPI_UDC_CNV_STATUS_FRMSET_OK,          /*!< Indicates converted metadata available */
    DDPI_UDC_CNV_STATUS_FRMSET_ERR          /*!< Indicates a full frame-set of metadata has
                                                been received but not converted due to frame error */
} DDPI_UDC_CNV_STATUS;

/*! \brief UDC converter sync status */
typedef enum
{
    DDPI_UDC_CNV_SYNC_STATUS_NOCHANGE_NOSYNC,   /*!< No change in sync state (still out of sync) */
    DDPI_UDC_CNV_SYNC_STATUS_NOCHANGE_INSYNC,   /*!< No change in sync state (still in sync) */
    DDPI_UDC_CNV_SYNC_STATUS_FOUND,             /*!< Sync found (now in sync) */
    DDPI_UDC_CNV_SYNC_STATUS_FOUNDEARLY,        /*!< Unexpected sync found (still in sync) */
    DDPI_UDC_CNV_SYNC_STATUS_LOST               /*!< Sync lost (now out of sync) */
} DDPI_UDC_CNV_SYNC_STATUS;

/*! \brief Define LFE states */
typedef enum
{
    DDPI_UDC_LFEOUTOFF,
    DDPI_UDC_LFEOUTSINGLE
} DDPI_UDC_LFEOUTMODES;

/*! \brief Compression mode */
typedef enum
{
    DDPI_UDC_COMP_CUSTOM_0,
    DDPI_UDC_COMP_CUSTOM_1,
    DDPI_UDC_COMP_LINE,
    DDPI_UDC_COMP_RF,
    DDPI_UDC_COMP_PORTABLE_L8,
    DDPI_UDC_COMP_PORTABLE_L11,
    DDPI_UDC_COMP_PORTABLE_L14,
    DDPI_UDC_COMP_PORTABLE_TEST
} DDPI_UDC_COMP;

/*! \brief Dual mono downmix mode */
typedef enum
{
    DDPI_UDC_DUAL_STEREO,
    DDPI_UDC_DUAL_LEFTMONO,
    DDPI_UDC_DUAL_RGHTMONO,
    DDPI_UDC_DUAL_MIXMONO
} DDPI_UDC_DUALMONO_DOWNMIX;

/*! \brief Error conceal mode */
typedef enum
{
    DDPI_UDC_PCMCONCEAL_ALWAYSRPT = -1,
    DDPI_UDC_PCMCONCEAL_ALWAYSMUTE
} DDPI_UDC_PCMCONCEAL;

/*! \brief Preferred stereo mode */
typedef enum
{
    DDPI_UDC_STEREOMODE_AUTO,
    DDPI_UDC_STEREOMODE_SRND,
    DDPI_UDC_STEREOMODE_STEREO
} DDPI_UDC_STEREOMODE;

/*! \brief DD+ inputs */
typedef enum
{
    DDPI_UDC_DDPIN_0 = 0,               /*!< Primary (main) DD+ input */
    DDPI_UDC_DDPIN_1 = 1,               /*!< Secondary (associated) DD+ input */
    DDPI_UDC_DDPIN_COUNT = 2            /*!< Total DD+ input count */
} DDPI_UDC_DDPIN;

/*! \brief  PCM outputs */
typedef enum
{
    DDPI_UDC_PCMOUT_MAIN = 0,           /*!< Main PCM output */
    DDPI_UDC_PCMOUT_ASSOC = 1,          /*!< Associated PCM output - OBSOLETE */
    DDPI_UDC_PCMOUT_COUNT = 8           /*!< Total PCM output count */
} DDPI_UDC_PCMOUT; 

/*! \brief  Evolution Framework outputs */
typedef enum
{
    DDPI_UDC_EVOLUTION_OUT_MAIN_INDEP = 0,     /*!< Main program Evolution output for I0 substream */
    DDPI_UDC_EVOLUTION_OUT_MAIN_DEP = 1,       /*!< Main program Evolution output for D0 substream */
    DDPI_UDC_EVOLUTION_OUT_ASSOC = 2,          /*!< Associated program Evolution output for I0, I1, I2 or I3 audio */
    DDPI_UDC_EVOLUTION_OUT_COUNT = 3           /*!< Total Evolution output count */
}DDPI_UDC_EVOLUTION_OUT;

/*! \brief Intelligent Loudness outputs */
typedef enum
{
    DDPI_UDC_INTLOUD_OUT_MAIN = 0,      /*!< Main program Intelligent loudness output */
    DDPI_UDC_INTLOUD_OUT_ASSOC = 1,     /*!< Associated program Intelligent loudness output */
    DDPI_UDC_INTLOUD_OUT_COUNT = 2      /*!< Total Intelligent loudness output count */
}DDPI_UDC_INTLOUD_OUT;

/*! \brief Programs */
typedef enum
{
    DDPI_UDC_PROG_MAIN =    0,          /*!< Main program                         */
    DDPI_UDC_PROG_ASSOC =   1,          /*!< Associated program                   */
    DDPI_UDC_PROG_COUNT =   2           /*!< Total program count                  */
} DDPI_UDC_PROG;

/*! \brief Substream selection */
typedef enum
{
    DDPI_UDC_SUBSTREAMID_UNDEFINED = -1,
    DDPI_UDC_SUBSTREAMID_0 = 0,
    DDPI_UDC_SUBSTREAMID_1 = 1,
    DDPI_UDC_SUBSTREAMID_2 = 2,
    DDPI_UDC_SUBSTREAMID_3 = 3
} DDPI_UDC_SUBSTREAMID;

/*! \brief Substream type selection */
typedef enum
{
    DDPI_UDC_SUBSTREAM_TYPE_UNDEFINE = -1,
    DDPI_UDC_SUBSTREAM_TYPE_INDEP = 0,
    DDPI_UDC_SUBSTREAM_TYPE_DEP = 1
} DDPI_UDC_SUBSTREAM_TYPE;

/*! \brief UDC input mode */
typedef enum
{
    DDPI_UDC_INPUTMODE_SINGLEINPUT,
    DDPI_UDC_INPUTMODE_DUALINPUT,
    DDPI_UDC_INPUTMODE_COUNT
} DDPI_UDC_INPUTMODE;

/*! \brief UDC decorrelator modes */
typedef enum
{
    DDPI_UDC_DECORR_OFF,
    DDPI_UDC_DECORR_UNGUIDED,
    DDPI_UDC_DECORR_ADAPTIVE
} DDPI_UDC_DECORR_MODE;

/*! \brief UDC Evolution decoder mode */
typedef enum
{
    DDPI_UDC_EVO_DISABLE_PCMHASH = 0,   /*!< Parse Evolution data, but no calculation for PCM protection, only available in source code release */
    DDPI_UDC_EVO_ENABLE_PCMHASH = 1     /*!< Parse Evolution data, and calculate PCM protection */
} DDPI_UDC_EVOLUTION_MODE;

/*! \brief Intelligent Loudness metadata substream*/
typedef enum
{
    DDPI_UDC_INTLOUD_MDAT_UNDEFINED=-1,
    DDPI_UDC_INTLOUD_MDAT_MAIN_I0,
    DDPI_UDC_INTLOUD_MDAT_MAIN_D0,
    DDPI_UDC_INTLOUD_MDAT_ASSOC,
} DDPI_UDC_INTLOUD_MDAT_SUBSTREAMID;

/*! \brief Intelligent Loudness "Dialogue Corrected" identifiers */
typedef enum
{
    DDPI_UDC_INTLOUD_DIAL_CORR_NOT_PRESENT = -2,                 /*!< Not present */
    DDPI_UDC_INTLOUD_DIAL_CORR_INVALID,                          /*!< Invalid */
    DDPI_UDC_INTLOUD_DIAL_CORR_FALSE,                            /*!< Associated stream was not dialogue corrected */
    DDPI_UDC_INTLOUD_DIAL_CORR_TRUE,                             /*!< Associated stream was dialogue corrected */
} DDPI_UDC_INTLOUD_DIAL_CORR;

/*! \brief Intelligent Loudness Speech channel ID's */
typedef enum
{
    DDPI_UDC_INTLOUD_DIAL_CHAN_NOT_PRESENT = -2,                 /*!< Not present */
    DDPI_UDC_INTLOUD_DIAL_CHAN_INVALID,                          /*!< Invalid */
    DDPI_UDC_INTLOUD_DIAL_CHAN_NONE,                             /*!< None */
    DDPI_UDC_INTLOUD_DIAL_CHAN_L,                                /*!< Left */
    DDPI_UDC_INTLOUD_DIAL_CHAN_R,                                /*!< Right */
    DDPI_UDC_INTLOUD_DIAL_CHAN_LR,                               /*!< Left and Right */
    DDPI_UDC_INTLOUD_DIAL_CHAN_C,                                /*!< Center */
    DDPI_UDC_INTLOUD_DIAL_CHAN_LC,                               /*!< Left and Center */
    DDPI_UDC_INTLOUD_DIAL_CHAN_RC,                               /*!< Right and Center */
    DDPI_UDC_INTLOUD_DIAL_CHAN_LRC,                              /*!< Left, Right and Center */
} DDPI_UDC_INTLOUD_DIAL_CHAN;

/*! \brief Loudness correction type identifiers */
typedef enum
{
    DDPI_UDC_INTLOUD_LOUD_CORR_TYPE_NOT_PRESENT = -2,           /*!< Not present */
    DDPI_UDC_INTLOUD_LOUD_CORR_TYPE_INVALID,                    /*!< Invalid */
    DDPI_UDC_INTLOUD_LOUD_CORR_TYPE_STREAMING,                  /*!< Real-time correction */
    DDPI_UDC_INTLOUD_LOUD_CORR_TYPE_PROGRAM,                    /*!< File-based (program) correction */
} DDPI_UDC_INTLOUD_LOUD_CORR_TYPE;

/*! \brief Loudness Regulation type identifiers */
typedef enum
{
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_INVALID = -1,                /*!< Invalid */
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_NOT_INDICATED,               /*!< Program loudness regulation compliance not indicated */
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_ATSC,                        /*!< Program loudness complies with ATSC A/85 */
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_EBU,                         /*!< Program loudness complies with EBU R128 */
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_ARIB,                        /*!< Program loudness complies with ARIB TR-B32 */
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_FREETV,                      /*!< Program loudness complies with FreeTV OP-59 */
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_RESERVED_5,                  /*!< Reserved bits 5 - 13 */
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_RESERVED_6,
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_RESERVED_7,
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_RESERVED_8,
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_RESERVED_9,
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_RESERVED_10,
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_RESERVED_11,
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_RESERVED_12,
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_RESERVED_13,
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_MANUAL,
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE_CONSUMER_LEVEL
} DDPI_UDC_INTLOUD_LOUD_REG_TYPE;

/*!< Metadata extraction structures */
/*! \brief Minimal BSI data (extracted for all frames present in a time slice) */
typedef struct {
    int strmtyp;        /*!< Stream type */
    int substreamid;    /*!< Sub-stream identification */
    int frmsiz;         /*!< will be set to words per frame for both DD and DD+ */
    int fscod;          /*!< Sample rate code */
    int fscod2;         /*!< Sample rate code 2 (halfrate) */
    int numblkscod;     /*!< Blocks per frame code */
    int acmod;          /*!< Audio coding mode */
    int lfeon;          /*!< Low frequency effects channel flag */
    int bsid;           /*!< Bitstream identification */
    int chanmap;        /*!< Channel map */
    int dialnorm[DDPI_UDC_MAX_DUALMONO_CHANS];       /*!< Dialogue Normalization */
    int bsmod;          /*!< Bitstream mode */
} ddpi_udc_minbsi_mdat;

/*! \brief Frame metadata */
typedef struct {
    int frm_present;        /*!< Frame present */
    int frm_status;         /*!< Frame status (CRC result) */
    int frm_id;             /*!< Frame identifier (used as input to ddpi_udc_getframe_metadata()) */
    int minbsi_valid;       /*!< Minimal BSI data is valid */
    ddpi_udc_minbsi_mdat   minbsi_mdat;   /*!< Minimal BSI data available for all present frames */
} ddpi_udc_frm_mdat;

/*! \brief Program metadata */
typedef struct {
    int pgm_present;                               /*!< Program present */
    int channel_map;                               /*!< Aggregate channel map; default channel map: L R C LFE Ls Rs x1 x2 */
    int datarate;                                  /*!< Aggregate datarate for this program (in kbps) */
    int dep_substream_count;                       /*!< Number of dependent substreams for this independent program */
    ddpi_udc_frm_mdat  ind_frm_mdat;               /*!< Metadata for independent frames in the program */
    ddpi_udc_frm_mdat  dep_frm_mdat[DDPI_UDC_MAX_DEPEN_FRMS];  /*!< Metadata for dependent frames in the program  */
} ddpi_udc_pgm_mdat;

/*! \brief Time slice metadata */
typedef struct {
    int datarate;                                   /*!< Aggregate datarate for the timeslice */
    int pgm_count;                                  /*!< Number of independent programs in the timeslice */
    ddpi_udc_frm_mdat   *frm_mdat[DDPI_UDC_MAX_FRMS_PER_TS];     /*!< Linear representation of frames ordered as they appeared in the bitstream */
    ddpi_udc_pgm_mdat    pgm_mdat[DDPI_UDC_MAX_PRGMS];           /*!< Hierarchical representation of programs containing frames */
} ddpi_udc_timeslice_mdat;

/*! \brief Bit stream information structure */
typedef struct {

    /* DD bitstream information variables */
    int fscod;          /*!< Sample rate code */
    int frmsizecod;     /*!< Frame size code */
    int bsid;           /*!< Bitstream identification */
    int bsmod;          /*!< Bitstream mode */
    int acmod;          /*!< Audio coding mode */
    int lfeon;          /*!< Low frequency effects channel flag */

    int cmixlev;        /*!< Center mix level */
    int surmixlev;      /*!< Surround mix level */
    int dsurmod;        /*!< Dolby surround mode */
    int dialnorm[DDPI_UDC_MAX_DUALMONO_CHANS];    /*!< Dialogue normalization */
    int compre[DDPI_UDC_MAX_DUALMONO_CHANS];      /*!< Compression word exists */
    int compr[DDPI_UDC_MAX_DUALMONO_CHANS];       /*!< Compression word (sign extended in 32-bit word) */

    int langcode[DDPI_UDC_MAX_DUALMONO_CHANS];    /*!< Language code exists */
    int langcod[DDPI_UDC_MAX_DUALMONO_CHANS];     /*!< Language code */
    int audprodie[DDPI_UDC_MAX_DUALMONO_CHANS];   /*!< Audio production info exists */
    int mixlevel[DDPI_UDC_MAX_DUALMONO_CHANS];    /*!< Mixing level */
    int roomtyp[DDPI_UDC_MAX_DUALMONO_CHANS];     /*!< Room type */
    int copyrightb;     /*!< Copyright bit */
    int origbs;         /* Original bitstream flag */

    int timecod1e;      /*!< Time code 1 exists */
    int timecod1;       /*!< Time code 1 */
    int timecod2e;      /*!< Time code 2 exists */
    int timecod2;       /*!< Time code 2 */

    /* Annex D alternate bitstream information variables */

    /* Extended bitstream information 1 */
    int xbsi1e;         /*!< Extra BSI1 info exists */
    int dmixmod;        /*!< Preferred downmix mode */
    int ltrtcmixlev;    /*!< Lt/Rt center mix level */
    int ltrtsurmixlev;  /*!< Lt/Rt surround mix level */
    int lorocmixlev;    /*!< Lo/Ro center mix level */
    int lorosurmixlev;  /*!< Lo/Ro surround mix level */

    /* Extended bitstream information 2 (system data) */
    int xbsi2e;         /*!< Extra BSI2 info exists */
    int dsurexmod;      /*!< Surround EX mode flag */
    int dheadphonmod;   /*!< Dolby Headphone encoded flag */
    int adconvtyp[DDPI_UDC_MAX_DUALMONO_CHANS];   /*!< Advanced converter flag */
    int xbsi2;          /*!< Reserved bsi parameters */
    int encinfo;        /*!< Encoder Information bit */

    /* Additional Bitstream Information */
    int addbsie;                                        /*!< Additional BSI exists */
    int addbsil;                                        /*!< Additional BSI length */
    unsigned char addbsi[DDPI_UDC_MAX_ADDBSI_BYTES];    /*!< Additional BSI data */

    /* DDPlus bitstream information variables */
    int strmtyp;        /*!< Stream type */
    int substreamid;    /*!< Sub-stream identification */
    int frmsiz;         /*!< Frame size (in 16-bit words) */
    int fscod2;         /*!< Sample rate code 2 (halfrate) */
    int blks_per_frm;   /*!< Blocks per frame */
    int chanmape;       /*!< Channel map exists flag */
    int chanmap;        /*!< Channel map data */
    int mixmdate;       /*!< Mixing metadata exists flag */
    int lfemixlevcode;  /*!< LFE Mix Level Code exists flag */
    int lfemixlevcod;   /* LFE Mix Level Code */
    int pgmscle[DDPI_UDC_MAX_DUALMONO_CHANS];     /*!< Program scale factor exists flags */
    int pgmscl[DDPI_UDC_MAX_DUALMONO_CHANS];      /*!< Program scale factor */
    int extpgmscle;     /*!< External program scale factor exists flags */
    int extpgmscl;      /*!< External program scale factor exists */
    int mixdef;         /*!< Mix control type */
    int mixdeflen;      /*!< Length of mixing parameter data field */
    unsigned char mixdata[DDPI_UDC_MAX_MIXDEFLEN_BYTES];   /*!< Raw mixdata defined by mixdeflen */
    int mixdata2e;          /*!< Mixing data 2 exists */
    int premixcmpsel;       /*!< Premix compression word select */
    int drcsrc;             /*!< Dynamic range control word source (external or current) */
    int premixcmpscl;       /*!< Premix compression word scale factor */
    int extpgmlscle;        /*!< External program left scale factor exists */
    int extpgmcscle;        /*!< External program center scale factor exists */
    int extpgmrscle;        /*!< External program right scale factor exists */
    int extpgmlsscle;       /*!< External program left surround scale factor exists */
    int extpgmrsscle;       /*!< External program right surround scale factor exists */
    int extpgmlfescle;      /*!< External program LFE scale factor exists */
    int extpgmlscl;         /*!< External program left scale factor */
    int extpgmcscl;         /*!< External program center scale factor */
    int extpgmrscl;         /*!< External program right scale factor */
    int extpgmlsscl;        /*!< External program left surround scale factor */
    int extpgmrsscl;        /*!< External program right surround scale factor */
    int extpgmlfescl;       /*!< External program LFE scale factor */
    int dmixscle;           /*!< Downmix scale factor exists */
    int dmixscl;            /*!< Downmix scale factor */
    int addche;             /*!< Additional scale factors exist */
    int extpgmaux1scle;     /*!< External program 1st auxiliary channel scale factor exists */
    int extpgmaux1scl;      /*!< External program 1st auxiliary channel scale factor */
    int extpgmaux2scle;     /*!< External program 2nd auxiliary channel scale factor exists */
    int extpgmaux2scl;      /*!< External program 2nd auxiliary channel scale factor */
    int mixdata3e;          /*!< Mixing data 3 exists */
    int spchdat;            /*!< Speech enhancement processing data */
    int addspchdate;        /*!< Additional speech enhancement processing data exists */
    int spchdat1;           /*!< Additional Speech enhancement processing attenuation data */
    int spchan1att;         /*!< Speech enhancement processing attenuation data */
    int addspchdat1e;       /*!< Additional speech enhancement processing data exists */
    int spchdat2;           /*!< Additional speech enhancement processing data */
    int spchan2att;         /*!< Speech enhancement processing attenuation data */
    int frmmixcfginfoe;     /*!< Frame mixing configuration information exists flag */
    int blkmixcfginfo[DDPI_UDC_MAX_BLOCKS_PER_FRM];   /*!< Block mixing configuration information */
    int paninfoe[DDPI_UDC_MAX_DUALMONO_CHANS];        /*!< Pan information exists flag */
    int panmean[DDPI_UDC_MAX_DUALMONO_CHANS];         /*!< Pan mean angle data */
    int paninfo[DDPI_UDC_MAX_DUALMONO_CHANS];         /*!< Pan information */
    int infomdate;          /*!< Information metadata exists flag */
    int sourcefscod;        /*!< Source sample rate code */
    int convsync;           /*!< Converter synchronization flag */
    int blkid;              /*!< Block identification */
} ddpi_udc_bsi_mdat;

/*! \brief Audio Block metadata */
typedef struct {
    int dynrnge[DDPI_UDC_MAX_DUALMONO_CHANS];           /*!< Dynamic range word exists */
    int dynrng[DDPI_UDC_MAX_DUALMONO_CHANS];            /*!< Dynamic range word (sign extended in 32-bit word) */
    int skipflde;                                       /*!< Skip field exists */
    int skiple;                                         /*!< Skip field length exists */
    int skipl;                                          /*!< Skip field length (in bytes) */
    unsigned char skipfld[DDPI_UDC_MAX_SKIPFLD_BYTES];  /*!< Skip field data */
} ddpi_udc_audblk_mdat;

/*! \brief Auxiliary metadata */
typedef struct {
    int auxdatae;                                       /*!< Auxiliary data exists */
    int auxdatal;                                       /*!< Auxiliary data length (in bits) */
    unsigned char auxbits[DDPI_UDC_MAX_AUXDATA_BYTES];  /*!< Auxiliary data */
} ddpi_udc_auxdata_mdat;

/*! \brief Mixing metadata */
typedef struct
{
    int pgmscl1;
    int pgmscl2;
    int extpgmscl;
    int premixcmpsel;
    int drcsrc;
    int premixcmpscl;
    int extpgmlscl;
    int extpgmcscl;
    int extpgmrscl;
    int extpgmlsscl;
    int extpgmrsscl;
    int extpgmlfescl;
    int dmixscl;
    int extpgmaux1scl;
    int extpgmaux2scl;
    int panmean1;
    int panmean2;
    int paninfo1;
    int paninfo2;

    int spchdat;
    int spchdat1;
    int spchan1att;
    int spchdat2;

     /*!< Mixdata (opaque field if present, all bytes are output for analysis outside of the decoder) */
} ddpi_udc_mix_mdat;

/*! \brief Reencoding metadata */
typedef struct
{
    unsigned int cmixlev;
    unsigned int surmixlev;
    unsigned int dsurmod;
    unsigned int acmod;
    unsigned int samplerate;
    unsigned int lfeon;
} ddpi_udc_reenc_mdat;

typedef struct
{
    int                    size;                                          /*!< Size of the serialized Evolution data in bytes */ 
    unsigned char          data[DDPI_UDC_MAX_SERIALIZED_EVOLUTION_SIZE];  /*!< Serialized Evolution data */
} ddpi_udc_serialized_evolution_t;

/*! \brief Evolution framework metadata */
typedef struct
{
    ddpi_udc_serialized_evolution_t evo_serialized_frame;
    DDPI_UDC_EVO_STATUS             evo_data_validity;
}ddpi_udc_evolution_data;

typedef struct 
{
    int                             dialchane;      /*!< The flag which indicates whether "dialogue_channels" exists */
    int                             dialcorre;      /*!< The flag which indicates whether "dialogue_corrected" exists */
    int                             loudcorrtype;   /*!< The flag which indicates whether "loud_corr_type" exists*/
    int                             iturelloude;    /*!< The flag which indicates whether "itu_rel_loudness" exists */
    int                             ituspchloude;   /*!< The flag which indicates whether "itu_spch_loudness" exists */
    int                             st3sloude;      /*!< The flag which indicates whether "st_3s_loudness" exists */
    int                             truepke;        /*!< The flag which indicates whether "true_peak" exists*/
    int                             dmixoffste;     /*!< The flag which indicates whether "downmix_offset" exists */
}ddpi_udc_intloud_md_flag;

/** Number of resolution bits in integrated loudness, short-term loudness and peak results
 * i.e. "0" -> resolution is 1/(2^0) = 1   LKFS/dB
 * i.e. "1" -> resolution is 1/(2^1) = 1/2 LKFS/dB
 * i.e. "2" -> resolution is 1/(2^2) = 1/4 LKFS/dB
 * i.e. "3" -> resolution is 1/(2^3) = 1/8 LKFS/dB

#define IL_INT_LOUDNESS_RESOLUTION_BITS    1
#define IL_ST_LOUDNESS_RESOLUTION_BITS     1
#define IL_PEAK_RESOLUTION_BITS            0
#define IL_DMX_OFF_RESOLUTION_BITS         1
*/

/** "Special" values for integrated  loudness, short-term loudness and true peaks 

#define IL_LOUD_INVALID                    -1024
#define IL_LOUD_NOT_PRESENT                -512

#define IL_INT_LOUDNESS_MIN                ((int) (-58   * (1 << IL_INT_LOUDNESS_RESOLUTION_BITS)))
#defineIL_INT_LOUDNESS_MAX                 ((int) (+5.5  * (1 << IL_INT_LOUDNESS_RESOLUTION_BITS)))

#define IL_ST_LOUDNESS_MIN                 ((int) (-116  * (1 << IL_ST_LOUDNESS_RESOLUTION_BITS)))
#define IL_ST_LOUDNESS_MAX                 ((int) (+11.5 * (1 << IL_ST_LOUDNESS_RESOLUTION_BITS)))

#define IL_PEAK_MIN                        ((int) (-116  * (1 << IL_PEAK_RESOLUTION_BITS)))
#define IL_PEAK_MAX                        ((int) (+11.5 * (1 << IL_PEAK_RESOLUTION_BITS)))

#define IL_DMX_OFF_MIN                     ((int) (-7.5  * (1 << IL_DMX_OFF_RESOLUTION_BITS)))
#define IL_DMX_OFF_MAX                     ((int) (+7.5  * (1 << IL_DMX_OFF_RESOLUTION_BITS)))
*/

typedef struct
{
    
    DDPI_UDC_INTLOUD_DIAL_CHAN      dialogue_channels;
    DDPI_UDC_INTLOUD_LOUD_REG_TYPE  loud_reg_type;
    DDPI_UDC_INTLOUD_DIAL_CORR      dialogue_corrected;     
    DDPI_UDC_INTLOUD_LOUD_CORR_TYPE loud_corr_type;   
    int                             itu_rel_loudness;   /*!< The range of itu_rel_loudness is [-116, 11]. 
                                                           "-1024" indicates invalid value. "-512" indicates this variable is not present. */
    int                             itu_spch_loudness;  /*!< The range of itu_spch_loudness is [-116, 11]. 
                                                           "-1024" indicates invalid value. "-512" indicates this variable is not present. */
    int                             st_3s_loudness;     /*!< The range of st_3s_loudness is [-232, 23]. 
                                                           "-1024" indicates invalid value. "-512" indicates this variable is not present. */
    int                             true_peak;          /*!< The range of true_peak is [-116, 11]. 
                                                           "-1024" indicates invalid value. "-512" indicates this variable is not present. */
    int                             downmix_offset;     /*!< The range of downmix_offset is [-15, 15]. 
                                                           "-1024" indicates invalid value. "-512" indicates this variable is not present. */
} ddpi_udc_intloud_unpacked_md_t;

/*! \brief intelligent loudness metadata */
typedef struct
{
    int                                 mdat_present;       /*!< Indicate if the intelligent loudness metadata is present.*/
    int                                 update_mdat_flag;   /*!< Indicate if the intelligent loudness metadata updates in current frame.
                                                               - 1 = The metadata is updated when there is a valid intelligent loudness metadata or downmix is enabled 
                                                               - 0 = The metadata is not updated */
    DDPI_UDC_INTLOUD_MDAT_SUBSTREAMID   mdat_substreamid;   /*!< Indicate which substream the intelligent loudness metadata is extracted from. */
    ddpi_udc_intloud_md_flag            intloud_mdat_flags; /*!< The exist flags of intelligent loudness data */
    ddpi_udc_intloud_unpacked_md_t      intloud_unpacked_md;/*!< The unpacked intelligent loudness metadata */
    int                                 timestamp;          /*!< Indicate the start sample in audio frame, that the payload applies to  */
    
    /*!< Intelligent loudness metadata (opaque field if present, all bytes are output for analysis outside of the decoder) */
} ddpi_udc_intloud_mdat;

/** @brief UDC structure with input and output parameters for the #ddpi_udc_processtimeslice and #ddpi_udc_evolutionquickaccess function.
 *  For the buffer pointer or descriptor of type inout, it should be initialized either to a real address or 
 *  should be initialized as NULL, otherwise the UDC may crash. */
typedef struct
{
    unsigned int               jocd_active_channels;                   /*!< \out: The number of output channels that have active content (object output or downmix output) in JOC decoding mode, 
                                                                                  only valid in JOC decoding mode. */
    int                        jocd_out_mode;                          /*!< \out: Output mode for JOC decoder, object output or downmix output, value refers to enum DDPI_UDC_JOCD_OUT_MODE.
                                                                                  Only valid in JOC decoding mode. */
    void                       **joc_qmfbuf_out;                       /*!< \inout: QMF output buffer pointer for JOC decoder. It is a two dimension pointer that points to 
                                                                                    the DLB_CLVEC data type. The first level of indirection is for channels, which is fixed 
                                                                                    to DDPI_JOCD_MAX_CHANNELS. The second level is for processing slots, which is equal to 
                                                                                    DDPI_JOCD_QMF_SLOTS_PER_BLOCK * nblkspcm and its maximum is DDPI_JOCD_QMF_MAX_SLOTS.
                                                                                    Then each DLB_CLVEC has num_elements elements in it, which is fixed to
                                                                                    DDPI_JOCD_QMF_BANDS. The QMF data is scaled down by DDPI_JOCD_QMF_SCALE_BITS bits.
                                                                                    Only valid in JOC QMF decoding mode. */
    DDPI_UDC_JOCD_STATUS       jocd_status;                            /*!<\out: The JOC decoder status, only valid in JOC decoding mode.*/
    dlb_buffer                 *pcmoutbfds;                            /*!< \inout: PCM output buffer descriptor, either for decoder output or JOC decoding output. 
                                                                                    Invalid in converting only mode. */
    dlb_buffer                 *ddoutbfd;                              /*!< \inout: DDPlus output buffer descriptor, only valid when converter is enabled 
                                                                                    in converting only mode or decoding and converting mode. */
                               
    DDPI_UDC_OAMD_STATUS       oamd_status;                            /*!<\out: The OAMD status for the associated JOC decoding process, 
                                                                                 valid in JOC decoding mode. */
        
    int                        pcmdatavalid[DDPI_UDC_PCMOUT_COUNT];    /*!< \out: PCM data valid flag, invalid in converting only mode. */
    int                        nblkspcm;                               /*!< \out: Number of sample blocks, it applies to both PCM and QMF output data. Invalid in converting only mode. */
                               
    int                        dddatavalid;                            /*!< \out: DD data valid flag, only valid in converting only mode or decoding and converting mode. */
    int                        dddatasize;                             /*!< \out: Size in bytes of the DD frame, only valid in converting only mode or decoding and converting mode. */
                               
    int                        pcmsamplerate;                          /*!< \out: PCM sample rate, invalid in converting only mode.  */
    int                        numofoutputs;                           /*!< \out: Number of outputs (Not implemented)                */
    int                        sourcechcfg[DDPI_UDC_PROG_COUNT];       /*!< \out: Source channel configuration of independent frames */
    int                        decodechcfg[DDPI_UDC_PCMOUT_COUNT];     /*!< \out: Decoded channel configuration (Not implemented)    */
    int                        pcmupsampleflag;                        /*!< \out: PCM upsample flag, invalid in converting only mode.*/
    int                        errflag[DDPI_UDC_PCMOUT_COUNT];         /*!< \out: Error flag, the error codes is defined above,
                                                                                  between DDPI_UDC_ERR_NO_ERROR and DDPI_UDC_ERR_OUTPUT_DATA_TYPE_SIZE_BIGGER_THAN_LFRACT */
    unsigned int               outputmap[DDPI_UDC_PCMOUT_COUNT];       /*!< \out: Output channel map, invalid for Object output in JOC decoding mode. 
                                                                                  The channel location in UDC is defined above, starting from DDPI_UDC_CHANNEL_L to DDPI_UDC_CHANNEL_LFE.*/
    unsigned int               lfe2idx;                                /*!< \out: LFE2 channel index into UDC buffer arrays, invalid for Object output 
                                                                                  in JOC decoding mode. */
    unsigned int               dec_timesliceblks;                      /*!< \out: Number of blocks represented by time-slice       */
                               
    DDPI_UDC_CNV_STATUS        cnv_status;                             /*!< \out: Output status of the converter (Not implemented) */
    DDPI_UDC_CNV_SYNC_STATUS   cnv_syncstatus;                         /*!< \out: Synchronization status of the converter (Not implemented) */
    DDPI_UDC_DEC_STATUS        dec_status[DDPI_UDC_MAX_PRGMS_DECODED]; /*!< \out: Output status of the decoders                    */
                               
    int                        reencdatavalid;                         /*!< \out: Reencoding metadata status                       */
    ddpi_udc_reenc_mdat        *p_reencoding_mdat;                     /*!< \inout: Reencoding metadata (optional pointer)         */
                               
    int                        mixdatavalid;                           /*!< \out: Mixing metadata status                           */
    ddpi_udc_mix_mdat          *p_mixdata;                             /*!< \inout: Mixing metadata (optional pointer)         */
                               
    int                        intloud_status[DDPI_UDC_INTLOUD_OUT_COUNT];     /*!< \out: Intelligent loudness metadata status, value refers to enum DDPI_UDC_INTLOUD_STATUS  */
    ddpi_udc_intloud_mdat      *p_intlouddata[DDPI_UDC_INTLOUD_OUT_COUNT];     /*!< \inout: Intelligent loudness metadata (optional pointer) */
    DDPI_UDC_EVO_STATUS        evo_status_substream[DDPI_UDC_EVOLUTION_OUT_COUNT];    /*!<\out: Evolution metadata status per audio program    */
    DDPI_UDC_EVO_STATUS        evo_quickaccess_status;                         /*!<\out: Evolution metadata quick access status */
} ddpi_udc_pt_op;
/**
 * @brief Maximum number of channels
 *
 *  When maximum channel more than the number of channels produced by UDC,
 *  it will insert the silence channel into the output.
 *  When maximum channel less than the number of channels produced by UDC,
 *  it will do downmix and output.
 */
typedef struct
{
    int maxnumchannels;    /*!< Maximum number of channels */
} ddpi_udc_output;

/*! \brief Structure used in ddpi_udc_query_mem and ddpi_udc_open */
typedef struct
{
    int                 num_outputs;                        /*!< Number of discrete PCM outputs. Set to 0 in converting only mode. */
    int                 num_main_outputs;                   /*!< num_outputs = num_main_outputs + num_associated_outputs. Set to 0 in converting only mode. */
    ddpi_udc_output     outputs[DDPI_UDC_MAX_NUM_SIM_OUTPUTS];   /*!< Output descriptor */
    int                 converter;                          /*!< Set to TRUE if the DD output is (optionally) produced in converting only mode or decoding and converting mode. */
    int                 extension_channels;                 /*!< Bitmask with the extension channels(Not implemented) */
    int                 jocd_mode;                          /*!< \inout:  Indicates the mode for JOC decoding, value refers to enum DDPI_UDC_JOCD_MODE.
                                                                          - DDPI_UDC_JOCD_DISABLE for decoding only mode, converting only mode or decoding and converting mode.
                                                                          - DDPI_UDC_JOCD_PCM_OUT for JOC decoding mode with PCM output.
                                                                          - DDPI_UDC_JOCD_QMF_OUT for JOC decoding mode with QMF output.
                                                                                                    */
    int                 is_evolution_quickaccess;           /*!< \inout: Evolution quick access flag  */
    int                 mixer_mode;                         /*!< \inout: Set to true when UDC works with mixer process in dual decoding mode */
} ddpi_udc_query_ip;

/*! \brief Structure used in ddpi_udc_query_init_mem and ddpi_udc_init */
typedef struct
{
    int                 num_outputs;                        /*!< Number of discrete PCM outputs. Set to 0 in converting only mode. */
    int                 num_main_outputs;                   /*!< num_outputs = num_main_outputs + num_associated_outputs. Set to 0 in converting only mode. */
    ddpi_udc_output     outputs[DDPI_UDC_MAX_NUM_SIM_OUTPUTS];   /*!< Output descriptor */
    int                 is_evolution_quickaccess;           /*!< \inout: Evolution quick access flag  */
    int                 mixer_mode;                         /*!< \inout: Set to true when UDC works with mixer process in dual decoding mode */
    int                 runningmode;                        /*!< Running mode of UDC. Please refer to enum DDPI_UDC_RUNNING_MODE. */
    DDPI_UDC_INPUTMODE  inputmode;                          /*!< Input mode: single input or dual input. 
                                                                 Set to DDPI_UDC_INPUTMODE_SINGLEINPUT in converting only mode, JOC decoding mode.
                                                                 Set to DDPI_UDC_INPUTMODE_DUALINPUT for dual input decoding.*/
    int                 disable_associated_decorr;          /*!< Disable/enable associate decorrelator support, 
                                                                 - 1 = Disable 
                                                                 - 0 = Enable(Only valid in dual decoding mode) */
    int                 disable_associated_evolution;       /*!< Disable/enable associate Evolution support, 
                                                                 - 1 = Disable 
                                                                 - 0 = Enable(Only valid in dual decoding mode) */
} ddpi_udc_init_ip;

/*! \brief UDC query output parameters */
typedef struct
{
    unsigned int    version_major;      /*!< Major version number */
    unsigned int    version_minor;      /*!< Minor version number */
    unsigned int    version_update;     /*!< Update version number */
    unsigned int    build_number;       /*!< Build number */
    const char      *p_copyright;       /*!< Pointer to a constant char string containing the copyright string. */
    unsigned int    delaysamples;       /*!< PCM sample delay in decoded output */
    int             debug_build;        /*!< Tells whether this library has been compiled with debug or not */
    int             license_required;   /*!< Set to 1 if the decoder needs a license in order to run */
    char            backend_name[32];   /*!< Dolby Intrinsics backend */
    char            di_version[10];     /*!< Dolby Intrinsics version */
    char            compiler_ver[128];  /*!< Compiler version (if avaialble) */
    char            defines[256];       /*!< Key compilation defines, mainly for debugging purposes */
} ddpi_udc_query_op;

/*! \brief UDC query_mem output parameters */
typedef struct
{
    size_t    udc_static_size;    /*!< Size of the static memory for UDC in bytes */
    size_t    udc_dynamic_size;   /*!< Size of the dynamic buffer for UDC in bytes */
    size_t    outputbuffersize;   /*!< Buffer size (in bytes) for all the PCM/QMF output buffers */
    size_t    dd_buffersize;      /*!< Buffer size (in bytes) for the output DD buffer */
} ddpi_udc_query_mem_op;

/**** DDPlus Interface Functions for 7.1-Channel Decoder (API) ****/

/*!
*****************************************************************
*
*   Get the static parameters of the subroutine
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_query
    (ddpi_udc_query_op *p_outparams         /*!< \out: Query output */
    ,int jocd_mode                          /*!< \in: JOC Decoding mode, value refers to enum DDPI_UDC_JOCD_MODE */
    );

/*!
*****************************************************************
*
*   Get the memory requirement of the subroutine
*   Use this query memory function together with ddpi_udc_open() 
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_query_mem
    (const ddpi_udc_query_ip *p_inparams    /*!< \in: Input parameters */
    ,ddpi_udc_query_mem_op *p_outparams     /*!< \out: Query output */
    );

/*!
****************************************************************
*
*   Initialize the UDC subroutine memory.  This
*   includes initializing buffer pointers to point to allocated
*   buffers and setting control parameters to default values.
*   It is called once at system startup. 
*   IMPORTANT: all the allocated pointers must be on a 
*      DDPI_UDC_MIN_MEMORY_ALIGNMENT-bytes boundary, so extra care
*      has to be taken when allocating memory in the application.
*      If the memory provided is not aligned, the ddpi_udc_open
*      returns DDPI_UDC_ERR_MEMORY_NOT_ALIGNED.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_open
    (const ddpi_udc_query_ip *p_inparams    /*!< \in: Input parameters */
    ,void *p_udc_hdl                        /*!< \mod: Pointer to subroutine memory, aligned to DDPI_UDC_MIN_MEMORY_ALIGNMENT bytes */
    ,char *p_dynamic_buf                    /*!< \in: Pointer to dynamic memory, aligned to DDPI_UDC_MIN_MEMORY_ALIGNMENT bytes */    
    );

/*!
*****************************************************************
*
*   Get the memory requirement of the subroutine
*   Use this query memory function together with ddpi_udc_init() 
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_query_init_mem
    (const ddpi_udc_init_ip *p_inparams      /*!< \in: Input parameters */
    ,ddpi_udc_query_mem_op  *p_outparams     /*!< \out: Query output */
    );

/*!
****************************************************************
*
*   Initialize the UDC subroutine memory and set API static parameters. This function shall be called once at initialization stage.
*   This function first calls ddpi_udc_open to initialize the memory and then set the static parameters. 
*   Currently, only process parameter running mode and input mode are static which shall be set only once at an initialization stage and 
*   can not be changed during run-time. Changes to running mode or input mode should be made by reinitializing the unified decoder-converter instance.
*   When using this function, you don't need to set running mode (DDPI_UDC_CTL_RUNNING_MODE_ID) and input mode (DDPI_UDC_CTL_INPUTMODE_ID) 
*   by calling ddpi_udc_setprocessparams separately. What's more, when using this function, you should not explicitly call 
*   ddpi_udc_setprocessparams to set running mode and input mode at any time.
*   We recommend you to use this function instead of using ddpi_udc_open and ddpi_udc_setprocessparams 
*   to achieve the same functionality.
*   IMPORTANT: all the allocated pointers must be on a 
*      DDPI_UDC_MIN_MEMORY_ALIGNMENT-bytes boundary, so extra care
*      has to be taken when allocating memory in the application.
*      If the memory provided is not aligned, the ddpi_udc_init
*      returns DDPI_UDC_ERR_MEMORY_NOT_ALIGNED.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_init
    (const ddpi_udc_init_ip *p_initparams    /*!< \in: Initialization parameters */
    ,void *p_udc_hdl                        /*!< \mod: Pointer to subroutine memory, aligned to DDPI_UDC_MIN_MEMORY_ALIGNMENT bytes */
    ,char *p_dynamic_buf                    /*!< \in: Pointer to dynamic memory, aligned to DDPI_UDC_MIN_MEMORY_ALIGNMENT bytes */    
    );

/*!
****************************************************************
*
*   Add a certain amount of bytes to the input of the decoder.
*   If the timeslice is complete, function \ddpi_udc_processtimeslice 
*   can be invoked.
*   Note: If the timeslice is invalid, it will still return timeslice 
*   complete but with an error code DDPI_UDC_ERR_INVALID_TIMESLICE.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_addbytes
    (void *p_udchdl
    ,const char *p_buffer
    ,const unsigned int buflen
    ,int ddpinputnum
    ,unsigned int *p_bytesconsumed
    ,int *p_timeslicecomplete
    );

/**
****************************************************************
*   \section ddp_udc_timeslicecomplete Force Timeslice Decode
* 
*   In certain applications, the size of a timeslice may be available (e.g. from a transport layer). If this is the case, the UDC application* 
*   can reduce latency and can process the timeslice without having to buffer the next I0 (as is done for the standard streaming approach).
*   
*   The UDC subroutine provides the API function ddpi_udc_timeslicecomplete so that the application layer can communicate when it knows that
*   a complete timeslice has been added (using ddpi_udc_addbytes) to the subroutine, thereby avoiding the need to buffer the next I0 frame.
*   
*   The general API calling sequence is slightly changed and makes use of a timeslice size to determine when a timeslice is complete:
*   
*   \code
*       while (1)
*       {
*          // get timeslice size from timeslice header
*          gettimeslicesize(&p_ddpinfile->timeslicebytes);
*   
*           // loop until subroutine has consumed all timeslice bytes
*           timeslicecomplete = 0;
*           while (!timeslicecomplete)
*           {
*               // if no more available bytes in input buffer, refill from file
*               if (p_ddpinfile->avail == 0)
*               {
*                   readbytes = min(p_ddpinfile->timeslicebytes, DDPIN_BUFFER_SIZE);
*                   p_ddpinfile->p_buf = p_buf;
*                   p_ddpinfile->avail = fio_readddpinfile(p_ddpinfile, readbytes, p_ddpinfile->p_buf);
*               }
*   
*               // add buffer bytes to subroutine
*               ddpi_udc_addbytes(p_dechdl,p_ddpinfile->p_buf,p_ddpinfile->avail,ddpin,&bytesconsumed,&timeslicecomplete);
*   
               // perform updates based on bytes consumed
*               p_ddpinfile->p_buf += bytesconsumed;
*               p_ddpinfile->avail -= bytesconsumed;
*               p_ddpinfile->timeslicebytes -= bytesconsumed;
*   
*               // use "OR" assignment (|=) so as to not overwrite possible 
*               // timeslice complete returned by ddpi_udc_addbytes() above
*               timeslicecomplete |= (p_ddpinfile->timeslicebytes == 0);
*           }
*   
*           // let the subroutine know that we've buffered an entire time slice
*           ddpi_udc_timeslicecomplete(p_dechdl, ddpin);
*   
*           // process the buffered timeslice
*           ddpi_udc_processtimeslice(p_dechdl, ptop);
*       }
*   \endcode
*****************************************************************/
int /*! \return error code */
ddpi_udc_timeslicecomplete
    (void           *p_udc_hdl      /*!< \in: Pointer to subroutine memory */
    ,int            ddpinputnum     /*!< \in: DD+ input number */
    );

/**
 * @brief Process a DD/DD+ timeslice
 *
 *  The default PCM output channel order: L R C LFE Ls Rs x1 x2 
 *  Parameters
 *  p_udc_hdl       Pointer to subroutine memory
 *  p_params        Data structure with the input and output parameters
 * @retval          error code
 */
int
ddpi_udc_processtimeslice
    (void           *p_udc_hdl      /*!< \in: Pointer to subroutine memory */
    ,ddpi_udc_pt_op *p_params       /*!< \mod: Data structure with the input and output parameters */
     );

/*!
****************************************************************
*
*   Set the specified UDC control parameter.
*   The change does not take effect until the next call to
*   ddpi_udc_processtimeslice, when the new parameter value is
*   actually loaded for use by the decoder.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_setprocessparam
    (void *p_udc_hdl                /*!< \mod: Pointer to subroutine memory */
    ,const int paramid              /*!< \in: Control parameter identification */
    ,const void *p_paramval         /*!< \in: Pointer to parameter value */
    ,const int paramsize);          /*!< \in: Size of the parameter value in bytes */

/*!
****************************************************************
*
*   Set the specified UDC output parameter.
*   The change does not take effect until the next call to
*   ddpi_udc_processtimeslice, when the new parameter value is
*   actually loaded for use by the decoder.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_setoutparam
    (void *p_udc_hdl                /*!< \mod: Pointer to subroutine memory */
    ,int instance                   /*!< \mod: Instance of the output parameter */
    ,const int   paramid            /*!< \in: Output parameter identification */
    ,const void *p_paramval         /*!< \in: Pointer to parameter value */
    ,const unsigned int paramsize   /*!< \in: Size of the parameter value in bytes */
    );

/*!
****************************************************************
*
*   Get the current value of the specified UDC control parameter.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_getprocessparam
    (const void *p_udc_hdl          /*!< \mod: Pointer to subroutine memory */
    ,const int  paramid             /*!< \in: Control parameter identification */
    ,void *p_paramval               /*!< \out: Pointer to parameter value */
    ,int *p_paramsize               /*!< \out: Size of the parameter value in bytes */
    );

/*!
****************************************************************
*
*   Get the current value of the specified UDC output parameter.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_getoutparam
    (const void *p_udc_hdl          /*!< \mod: Pointer to subroutine memory */
    ,int instance                   /*!< \mod: Instance of the output parameter */
    ,const int paramid              /*!< \in: Output parameter identification */
    ,void *p_paramval               /*!< \out: Pointer to parameter value */
    ,int *p_paramsize               /*!< \out: Size of the parameter value in bytes */
    );

/*!
****************************************************************
*
*   Extract time slice metadata from last time slice decoded.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_gettimeslice_mdat
    (void const *p_udc_hdl                              /*!< \in: Pointer to subroutine memory */      
    ,int ddpin                                          /*!< \in: Input bitstream index */
    ,ddpi_udc_timeslice_mdat *p_timeslice_mdat          /*!< \output: Pointer to timeslice metadata */
    );

/*!
****************************************************************
*
*   Extract specified metadata from a given frame. 
* 
*   Caller must first call ddpi_udc_gettimeslice_mdat() to get 
*   frm_id's for frames present in the time slice. Caller can 
*   then extract specific sections of metadata from a given 
*   frame using ddpi_udc_getframe_mdat().
*
*   The metadata identifier specified must correspond to the 
*   metadata structure being passed in by the caller:
*       DDPI_UDC_BSI_MDAT_ID        - ddpi_udc_bsi_mdat
*       DDPI_UDC_AUDBLK_MDAT_ID     - ddpi_udc_audblk_mdat[6]
*       DDPI_UDC_AUXDATA_MDAT_ID    - ddpi_udc_auxdata_mdat
*   
*   This function can return the following error codes:
*       DDPI_UDC_ERR_NO_ERROR: Requested metadata is available 
*           and valid.
*       DDPI_UDC_ERR_FRAME_NOT_PRESENT: Requested frame is 
*           not present in the last decoded time slice.
*       DDPI_UDC_ERR_MDAT_NOT_AVAILABLE: Requested metadata
*           is not available for the requested frame.
*       DDPI_UDC_ERR_MDAT_CORRUPT: Requested metadata is 
*           corrupt for the requested frame.
*
*   The metadata structure supplied by the caller will only
*   contain valid data when the function returns an error 
*   code of DDPI_UDC_ERR_NO_ERROR=0. 
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_getframe_mdat
    (void *p_udc_hdl             /*!< \in: Pointer to subroutine memory */      
    ,int frm_id                  /*!< \in: Identify which frame to extract metadata from */
    ,int mdat_id                 /*!< \in: Identify which metadata to extract from the frame*/
    ,void *p_mdat                /*!< \output: Pointer to extracted metadata */
    );

/*!
****************************************************************
*
*   Extract evolution metadata for a given substream.
* 
*   The metadata structure supplied by the caller will only
*   contain valid data when the function returns an error 
*   code of DDPI_UDC_ERR_NO_ERROR=0. 
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_get_evolution_mdat
    (void *p_udc_hdl                            /*!< \in: Pointer to subroutine memory */      
    ,int   pgm_id                               /*!< \in: Identify which program to extract metadata from */
    ,ddpi_udc_evolution_data *p_evolution       /*!< \output: Pointer to extracted metadata */
    );

/*!
****************************************************************
*
*   This function sets the license to the decoder
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_set_license
    (         void          *p_udc_hdl              /*!< \mod: Pointer to subroutine memory */
    , const   void          *p_license              /*!< \in: Pointer to license data */
    , const   size_t        size                    /*!< \in: Size of license data in byte */
    , const   unsigned long manufacturer_id         /*!< \in: Manufacturer id */
    );

/*!
****************************************************************
*
*   This function gets the current license type.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_get_licensetype
    (const  void    *p_udc_hdl          /*!< \in: Pointer to subroutine memory */
    ,       int     *license_type       /*!< \mod: License type: 0 - no license; 
                                            1 - full license; 2 - eval license */
    );

/*!
****************************************************************
*
*   Perform all clean up necessary to close the UDC.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_close
    (void *p_udc_hdl                    /*!< \mod: Pointer to subroutine memory */
    );

/*!
****************************************************************
*
*   Open the necessary UDC debug files based on
*   the debug flags passed in.  Called once at system startup.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_opendebug
    (void *p_udc_hdl                    /*!< \in: Pointer to subroutine memory  */
    ,const uint32_t frm_debugflags  /*!< \in: Frame debug flags (both substreams) */
    ,const uint32_t dec_debugflags  /*!< \in: Decode debug flags (both substreams) */
    );

/*!
***************************************************************
*
*   Close all UDC debug output files.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_closedebug
    (void *p_udc_hdl                  /*!< \mod: Pointer to subroutine memory */
    );

/*!
****************************************************************
*
*   Quick access the Evolution data in timeslice.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_evolutionquickaccess
    (void           *p_udc_hdl      /*!< \in: Pointer to subroutine memory */
    ,ddpi_udc_pt_op *p_params       /*!< \mod: Data structure with the input and output parameters */
     );
/*!
****************************************************************
*
*   Mixing the main PCM output and associate PCM output.
*   DDPI_UDC_MIXER_DELAY_SAMPLES samples delay will be introduced
*   if this function is called.
*
*****************************************************************/
int /*! \return error code */
ddpi_udc_do_mixing
    (void           *p_udc_hdl      /*!< \in: Pointer to subroutine memory */
    ,ddpi_udc_pt_op *p_params       /*!< \mod: Data structure with the input and output parameters */
     );

/* C++ Compatibility */
#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* !defined(UDC_API_H) */

