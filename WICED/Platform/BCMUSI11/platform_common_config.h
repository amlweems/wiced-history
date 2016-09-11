/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "platform.h"
#include "platform_sleep.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* The clock configuration utility from ST is used to calculate these values
 * http://www.st.com/st-web-ui/static/active/en/st_prod_software_internet/resource/technical/software/utility/stsw-stm32090.zip
 * The CPU Clock Frequency (CPU_CLOCK_HZ) is independently defined in <WICED-SDK>/Wiced/Platform/BCMUSI11/BCMUSI11.mk
 */
#define HSE_SOURCE              RCC_HSE_ON               /* Use external crystal                 */
#define AHB_CLOCK_DIVIDER       RCC_SYSCLK_Div1          /* AHB clock = System clock             */
#define APB1_CLOCK_DIVIDER      RCC_HCLK_Div4            /* APB1 clock = AHB clock / 4           */
#define APB2_CLOCK_DIVIDER      RCC_HCLK_Div2            /* APB2 clock = AHB clock / 2           */
#define PLL_SOURCE              RCC_PLLSource_HSE        /* PLL source = external crystal        */
#define PLL_M_CONSTANT          26                       /* PLLM = 26                            */
#define PLL_N_CONSTANT          240                      /* PLLN = 240                           */
#define PLL_P_CONSTANT          2                        /* PLLP = 2                             */
#define PPL_Q_CONSTANT          5                        /* PLLQ = 5                             */
#define SYSTEM_CLOCK_SOURCE     RCC_SYSCLKSource_PLLCLK  /* System clock source = PLL clock      */
#define SYSTICK_CLOCK_SOURCE    SysTick_CLKSource_HCLK   /* SysTick clock source = AHB clock     */
#define INT_FLASH_WAIT_STATE    FLASH_Latency_3          /* Internal flash wait state = 3 cycles */

#define SDIO_OOB_IRQ_BANK       GPIOB
#define SDIO_CLK_BANK           GPIOC
#define SDIO_CMD_BANK           GPIOD
#define SDIO_D0_BANK            GPIOC
#define SDIO_D1_BANK            GPIOC
#define SDIO_D2_BANK            GPIOC
#define SDIO_D3_BANK            GPIOC
#define SDIO_OOB_IRQ_BANK_CLK   RCC_AHB1Periph_GPIOB
#define SDIO_CLK_BANK_CLK       RCC_AHB1Periph_GPIOC
#define SDIO_CMD_BANK_CLK       RCC_AHB1Periph_GPIOD
#define SDIO_D0_BANK_CLK        RCC_AHB1Periph_GPIOC
#define SDIO_D1_BANK_CLK        RCC_AHB1Periph_GPIOC
#define SDIO_D2_BANK_CLK        RCC_AHB1Periph_GPIOC
#define SDIO_D3_BANK_CLK        RCC_AHB1Periph_GPIOC
#define SDIO_OOB_IRQ_PIN        9
#define SDIO_CLK_PIN            12
#define SDIO_CMD_PIN            2
#define SDIO_D0_PIN             8
#define SDIO_D1_PIN             9
#define SDIO_D2_PIN             10
#define SDIO_D3_PIN             11

#define WL_GPIO0_BANK           GPIOD
#define WL_GPIO0_PIN            0
#define WL_GPIO0_BANK_CLK       RCC_AHB1Periph_GPIOD
#define WL_GPIO1_BANK           GPIOB
#define WL_GPIO1_PIN            9
#define WL_GPIO1_BANK_CLK       RCC_AHB1Periph_GPIOB

#define WL_REG_ON_BANK          GPIOA
#define WL_REG_ON_PIN           GPIO_Pin_12
#define WL_REG_ON_BANK_CLK      RCC_AHB1Periph_GPIOA

#define WL_RESET_BANK           GPIOB
#define WL_RESET_PIN            GPIO_Pin_8
#define WL_RESET_BANK_CLK       RCC_AHB1Periph_GPIOB


#if ( WICED_WLAN_POWERSAVE_CLOCK_SOURCE == WICED_WLAN_POWERSAVE_CLOCK_IS_MCO )

#define WL_32K_OUT_BANK         GPIOA
#define WL_32K_OUT_PIN          8
#define WL_32K_OUT_BANK_CLK     RCC_AHB1Periph_GPIOA

#else

#define WL_32K_OUT_BANK         GPIOC
#define WL_32K_OUT_PIN          6
#define WL_32K_OUT_BANK_CLK     RCC_AHB1Periph_GPIOC

#endif


#define STDIO_USART                    USART1
#define STDIO_TX_PIN                   GPIO_Pin_9
#define STDIO_RX_PIN                   GPIO_Pin_10
#define STDIO_TX_PINSOURCE             GPIO_PinSource9
#define STDIO_RX_PINSOURCE             GPIO_PinSource10
#define STDIO_TX_BANK                  GPIOA
#define STDIO_RX_BANK                  GPIOA
#define STDIO_TX_BANK_CLK              RCC_AHB1Periph_GPIOA
#define STDIO_RX_BANK_CLK              RCC_AHB1Periph_GPIOA
#define STDIO_GPIO_ALTERNATE_FUNCTION  GPIO_AF_USART1
#define STDIO_TX_GPIO_PERIPH_CLOCK     RCC_AHB1Periph_GPIOA
#define STDIO_RX_GPIO_PERIPH_CLOCK     RCC_AHB1Periph_GPIOA
#define STDIO_IRQ_CHANNEL              37
#define STDIO_PERIPH_CLOCK             RCC_APB2Periph_USART1
#define STDIO_CLOCK_CMD                RCC_APB2PeriphClockCmd
#define STDIO_TX_CLK_CMD               RCC_AHB1PeriphClockCmd
#define STDIO_RX_CLK_CMD               RCC_AHB1PeriphClockCmd

/*  Bootloader LED D1 */
#define BOOTLOADER_LED_GPIO     WICED_LED1
#define BOOTLOADER_LED_ON_STATE WICED_ACTIVE_HIGH

 /* Bootloader Button S1 */
#define BOOTLOADER_BUTTON_GPIO          WICED_BUTTON1
#define BOOTLOADER_BUTTON_PRESSED_STATE WICED_ACTIVE_LOW

/* The number of UART interfaces this hardware platform has */
#define NUMBER_OF_UART_INTERFACES  2

#define STDIO_UART       WICED_UART_1

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
