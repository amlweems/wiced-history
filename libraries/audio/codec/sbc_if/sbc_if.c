/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/************************************************************************************/
#include <string.h>
#include "wiced_codec_if.h"
#include "wiced_codec_sbc_params.h"
#include "sbc_decoder.h"



/************************************************************************************/
#define BTA_AVK_SBC_MAX_BITPOOL  0x59
#define WICED_SBC_IE_MIN_BITPOOL      2


/************************************************************************************/
extern UINT16 bta_audio_ucodec_alloc_buffer(SINT16 **ppSbcBuffer, UINT16 Length);
extern UINT16 bta_audio_ucodec_fill_buffer(UINT8 *pPacket, UINT16 u16NumberOfBytes);
extern UINT16 bta_audio_ucodec_dump_buffer(SINT16 *PacketPtr, UINT16 u16NumberOfPcm);
SBC_API extern SINT16 SBC_Decoder(SBC_DEC_PARAMS *strDecParams);

/************************************************************************************/
SINT32 SBCScratchMem[240+512+128+128+16+16] __attribute__ ((section (".ccm")));
SINT32 SBCStaticMem[DEC_VX_BUFFER_SIZE + (SBC_MAX_PACKET_LENGTH >> 2)] __attribute__ ((section (".ccm")));
SBC_DEC_PARAMS strDecParams;

/************************************************************************************/
/* SBC codec capabilities */
const wiced_sbc_codec_params_t bta_avk_co_sbc_cap = {
    ((WICED_SBC_IE_SAMP_FREQ_44) | (WICED_SBC_IE_SAMP_FREQ_48)),    /* samp_freq */
    (WICED_SBC_IE_CH_MD_MONO | WICED_SBC_IE_CH_MD_STEREO |
     WICED_SBC_IE_CH_MD_JOINT | WICED_SBC_IE_CH_MD_DUAL),       /* ch_mode */
    ((WICED_SBC_IE_BLOCKS_16| WICED_SBC_IE_BLOCKS_12 |
     WICED_SBC_IE_BLOCKS_8 | WICED_SBC_IE_BLOCKS_4)),            /* block_len */
    (WICED_SBC_IE_SUBBAND_4 | WICED_SBC_IE_SUBBAND_8),          /* num_subbands */
    (WICED_SBC_IE_ALLOC_MD_L | WICED_SBC_IE_ALLOC_MD_S),        /* alloc_mthd */
    BTA_AVK_SBC_MAX_BITPOOL,                                 /* max_bitpool */
    WICED_SBC_IE_MIN_BITPOOL                                  /* min_bitpool */
};

/* SBC codec preferences */
wiced_sbc_codec_params_t bta_avk_co_sbc_pref = {
    WICED_SBC_IE_SAMP_FREQ_44,                                /* samp_freq */
    WICED_SBC_IE_CH_MD_JOINT,                                 /* ch_mode */
    WICED_SBC_IE_BLOCKS_16,                                   /* block_len */
    WICED_SBC_IE_SUBBAND_8,                                   /* num_subbands */
    WICED_SBC_IE_ALLOC_MD_L,                                  /* alloc_mthd */
    0,                                                      /* max_bitpool */
    0                                                       /* min_bitpool */
};


/************************************************************************************/
static wiced_codec_status_t sbc_init(wiced_codec_data_transfer_api_t* memory_methods,void* arg);
static wiced_codec_status_t sbc_close(void);
static void sbc_getconfig(void* codec_capabilities, void* codec_preferred_params);
static wiced_codec_status_t sbc_setconfig(void* config);
static wiced_codec_status_t sbc_decode(void);
/************************************************************************************/
wiced_codec_interface_t codec_sbc;
wiced_codec_data_transfer_api_t memory_functions;
static int decoder_initialized = 0;
/************************************************************************************/
/**
 * codec_status_t sbc_init(void* data)
 * Initialize the codec.
 * Pass in a valid pointer to "data".
 * @param data [OUT]
 * @return
 */
wiced_codec_status_t sbc_init(wiced_codec_data_transfer_api_t* memory_methods, void* arg)
{

    if ( memory_methods != NULL )
    {
        memory_functions.alloc_output_buffer_fp = memory_methods->alloc_output_buffer_fp;
        memory_functions.read_encoded_data_fp = memory_methods->read_encoded_data_fp;
        memory_functions.write_decoded_data_fp = memory_methods->write_decoded_data_fp;

        if ( (memory_functions.alloc_output_buffer_fp == NULL) || \
             (memory_functions.read_encoded_data_fp == NULL) || \
             (memory_functions.write_decoded_data_fp == NULL ) )
        {
            /* this is an error condition */
            codec_sbc.configured = FALSE;
        }
        else
        {
            strDecParams.s32StaticMem  = (SINT32 *)SBCStaticMem;
            strDecParams.s32ScartchMem = (SINT32 *)SBCScratchMem;
            strDecParams.AllocBufferFP  = memory_functions.alloc_output_buffer_fp;
            strDecParams.FillBufferFP  = memory_functions.read_encoded_data_fp;
            strDecParams.EmptyBufferFP = memory_functions.write_decoded_data_fp;
            codec_sbc.configured = TRUE;
        }
    }
    decoder_initialized = 0;
    return 0;

}

/**
 *
 * @return
 */
wiced_codec_status_t sbc_close(void)
{


    return 0;
}


/**
 *
 * @return
 */
void sbc_getconfig(void* codec_capabilities, void* codec_preferred_params)
{

    if(codec_capabilities)
        memcpy(codec_capabilities,(void*)&bta_avk_co_sbc_cap, sizeof(bta_avk_co_sbc_cap));

    if(codec_preferred_params)
        memcpy(codec_preferred_params,(void*)(&bta_avk_co_sbc_pref),sizeof(bta_avk_co_sbc_pref));
}

/**
 *
 * @param config
 * @return
 */
wiced_codec_status_t sbc_setconfig(void* p_codec_info)
{
    wiced_codec_status_t codec_status = 0;
    uint8_t status = 0;

    if(status == 1)
        codec_status = -1;

    decoder_initialized = 0;
    return codec_status;
}

/**
 * codec_status_t sbc_decode(void* src)
 * @param src
 * @return
 */
wiced_codec_status_t sbc_decode(void)
{

    wiced_codec_status_t status = 1;

    if( (!decoder_initialized) && codec_sbc.configured)
    {
        if ( (status = SBC_Decoder_Init(&strDecParams)) != SBC_SUCCESS )
        {
            /* error */
        }
        decoder_initialized = 1 ;
        return WICED_SUCCESS;
    }
    strDecParams.pcmBuffer = NULL;
    status = SBC_Decoder(&strDecParams);
    if (status != SBC_SUCCESS ) {
        strDecParams.EmptyBufferFP(strDecParams.pcmBuffer,0);
        status = WICED_ERROR;
    }
    return status;
}

int32_t sbc_get_output_size(void)
{
    return 512;//this is 128 samples per channel per decode
}

wiced_codec_interface_t codec_sbc = {
        .type = WICED_CODEC_SBC,
        .configured = FALSE,
        .init = sbc_init,
        .close = sbc_close,
        .get_capabilities = sbc_getconfig,
        .set_configuration = sbc_setconfig,
        .get_decoded_output_size = sbc_get_output_size,
        .decode = sbc_decode,
        .encode = NULL
};


