/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  NVRAM variables which define BCM43362 Parameters for the USI module used on the BCM943362WCD2 board
 *
 *  As received from USI
 *
 */

#ifndef INCLUDED_NVRAM_IMAGE_H_
#define INCLUDED_NVRAM_IMAGE_H_

#include <string.h>
#include <stdint.h>
#include "../generated_mac_address.txt"

/**
 * Character array of NVRAM image
 */

static const char wifi_nvram_image[] =
        "cbuckout=1500"                                                      "\x00"
        "manfid=0x2d0"                                                       "\x00"
        "prodid=0x492"                                                       "\x00"
        "vendid=0x14e4"                                                      "\x00"
        "devid=0x4343"                                                       "\x00"
        "boardtype=0x0598"                                                   "\x00"
        "boardrev=0x1207"                                                    "\x00"
        "boardnum=777"                                                       "\x00"
        "xtalfreq=26000"                                                     "\x00"
        "boardflags=0x200"                                                   "\x00"
        "sromrev=3"                                                          "\x00"
        "wl0id=0x431b"                                                       "\x00"
        "macaddr=00:90:4c:07:71:12"                                          "\x00"
        "aa2g=1"                                                             "\x00"
        "ag0=2"                                                              "\x00"
        "maxp2ga0=70"                                                        "\x00"
        "ofdm2gpo=0x44111111"                                                "\x00"
        "mcs2gpo0=0x4444"                                                    "\x00"
        "mcs2gpo1=0x6444"                                                    "\x00"
         "pa0maxpwr=80"                                                      "\x00"
        "pa0b0=5317"                                                         "\x00"
        "pa0b1=-637"                                                         "\x00"
        "pa0b2=-160"                                                         "\x00"
        "pa0itssit=62"                                                       "\x00"
        "pa1itssit=62"                                                       "\x00"
        "cckPwrOffset=5"                                                     "\x00"
        "cckdigfilttype=22"                                                  "\x00"
        "ccode=0"                                                            "\x00"
        "rssismf2g=0xa"                                                      "\x00"
        "rssismc2g=0x3"                                                      "\x00"
        "rssisav2g=0x7"                                                      "\x00"
        "triso2g=1"                                                          "\x00"
        "noise_cal_enable_2g=0"                                              "\x00"
        "noise_cal_po_2g=0"                                                  "\x00"
        "swctrlmap_2g=0x04040404,0x02020202,0x04040404,0x010101,0x1ff"       "\x00"
        "temp_add=29767"                                                     "\x00"
        "temp_mult=425"                                                      "\x00"
        "temp_q=10"                                                          "\x00"
        "initxidx2g=45"                                                      "\x00"
        "tssitime=1"                                                         "\x00"
        "rfreg033=0x19"                                                      "\x00"
        "rfreg033_cck=0x1f"                                                  "\x00"
        "cckPwrIdxCorr=-8"                                                   "\x00"
        "spuravoid_enable2g=1"                                               "\x00"
        "\x00\x00";

#else /* ifndef INCLUDED_NVRAM_IMAGE_H_ */

#error Wi-Fi NVRAM image included twice

#endif /* ifndef INCLUDED_NVRAM_IMAGE_H_ */
