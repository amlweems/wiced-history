/**************************************************
 *
 * Part one of the system initialization code, contains low-level
 * initialization, plain thumb variant.
 *
 * Copyright 2008 IAR Systems. All rights reserved.
 *
 * $Revision: 46842 $
 *
 **************************************************/

;
; The modules in this file are included in the libraries, and may be replaced
; by any user-defined modules that define the PUBLIC symbol _program_start or
; a user defined start symbol.
; To override the cstartup defined in the library, simply add your modified
; version to the workbench project.
;
; The vector table is normally located at address 0.
; When debugging in RAM, it can be located in RAM, aligned to at least 2^6.
; The name "__vector_table" has special meaning for C-SPY:
; it is where the SP start value is found, and the NVIC vector
; table register (VTOR) is initialized to this address if != 0.
;
; Cortex-M version
;

        MODULE  ?cstartup

		EXTERN __iar_program_start
        PUBLIC  wiced_program_start


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		PUBWEAK _wiced_iar_program_start
		SECTION .text:CODE:REORDER(1)
		THUMB
_wiced_iar_program_start:
		BL __iar_program_start
		BX LR

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		PUBLIC wiced_program_start
		EXTERN _wiced_iar_program_start
		EXTERN __iar_program_start
		SECTION .text:CODE:REORDER(1)
		THUMB

wiced_program_start:
		BL _wiced_iar_program_start
		BL __iar_program_start
		BX LR

		END




        END
