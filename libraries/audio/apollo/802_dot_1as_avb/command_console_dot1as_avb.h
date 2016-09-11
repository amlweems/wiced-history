/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

#define DOT1AS_CONSOLE_COMMANDS \
    { (char*) "avb_init",       dot1as_console_command,    0, NULL, NULL, (char *)"", (char *)"avb_init"}, \
    { (char*) "avb_sync_send",  dot1as_console_command,    0, NULL, NULL, (char *)"", (char *)"avb_sync_send"}, \
    { (char*) "avb_ts_get",     dot1as_console_command,    0, NULL, NULL, (char *)"", (char *)"avb_ts_get"}, \
    { (char*) "init",           dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)"init <device_name> <dbg/debug>"}, \
    { (char*) "auto",           dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)"auto 1 | 0  (on/off)"}, \
    { (char*) "bdelay",         dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)"bdelay_calibrate"}, \
    { (char*) "bmca",           dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)"bmca"}, \
    { (char*) "disable",        dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)"disable <port> | dot1as"}, \
    { (char*) "enable",         dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)"enable  <port> |dot1as "}, \
    { (char*) "get_local",      dot1as_console_command,    0, NULL, NULL, (char *)"", (char *)"get_local"}, \
    { (char*) "get_buscal",     dot1as_console_command,    0, NULL, NULL, (char *)"", (char *)"get_buscal"}, \
    { (char*) "interval",       dot1as_console_command,    2, NULL, NULL, (char *)"", (char *)"interval <port> [sync_tx|bmca_tx|pdelay_tx|sync_rx|bmca_rx] <val>"}, \
    { (char*) "pdelay",         dot1as_console_command,    3, NULL, NULL, (char *)"", (char *)"pdelay num_lst_resp <port> <val>"}, \
    { (char*) "pri1",           dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)"pri1 <val>"}, \
    { (char*) "pri2",           dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)"pri2 <val>"}, \
    { (char*) "stats",          dot1as_console_command,    2, NULL, NULL, (char *)"", (char *)"stats <port> clear"}, \
    { (char*) "timesync",       dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)"timesync"}, \
    { (char*) "send",           dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)"send <announce | sync | follow_up | pdelay_req | pdelay_resp >"}, \
    { (char*) "iovar",          dot1as_console_command,    3, NULL, NULL, (char *)"", (char *)"iovar <do1as wl command>"}, \
    { (char*) "enable",         dot1as_console_command,    0, NULL, NULL, (char *)"", (char *)" (iovar) enable"}, \
    { (char*) "avb_local_time", dot1as_console_command,    0, NULL, NULL, (char *)"", (char *)" (iovar) avb_local_time"}, \
    { (char*) "ascapable",      dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)" (iovar)ascapabl"}, \
    { (char*) "utc_offset",     dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)" (iovar) utc_offset"}, \
    { (char*) "get_cntrs",      dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)" (iovar) get_cntrs"}, \
    { (char*) "clr_cntrs",      dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)" (iovar) clr_cntrs"}, \
    { (char*) "dot1as_role",    dot1as_console_command,    1, NULL, NULL, (char *)"", (char *)" (iovar) dot1as_role"}, \
    { (char*) "as_help",        dot1as_console_command,    0, NULL, NULL, (char *)"", (char *)"dot1as_help"}, \

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

int dot1as_console_command(int argc, char *argv[]);

#ifdef __cplusplus
} /* extern "C" */
#endif
