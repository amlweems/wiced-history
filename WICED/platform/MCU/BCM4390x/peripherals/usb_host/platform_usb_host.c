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
 * WICED 4390x USB host driver.
 */

#include "typedefs.h"
#include "sbchipc.h"
#include "platform_peripheral.h"
#include "platform_map.h"
#include "platform_config.h"
#include "platform_usb.h"
#include "platform_appscr4.h"
#include "hndsoc.h"
#include "wiced_osl.h"
#include "wiced_platform.h"
#include "wwd_rtos_isr.h"
#include "platform/wwd_platform_interface.h"
#include "bcm4390x_usb_host.h"


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* Clock Control Status (offset 0x1e0) register bits and value */
#define USB20H_CLKCTLST_REG_FORCE_ALP_REQ           (0x1 << 0)
#define USB20H_CLKCTLST_REG_FORCE_HT_REQ            (0x1 << 1)
#define USB20H_CLKCTLST_REG_FORCE_ILP_REQ           (0x1 << 2)
#define USB20H_CLKCTLST_REG_ALP_AVAIL_REQ           (0x1 << 3)
#define USB20H_CLKCTLST_REG_HT_AVAIL_REQ            (0x1 << 4)
#define USB20H_CLKCTLST_REG_FORCE_HWCLOCK_REQ_OFF   (0x1 << 5)
#define USB20H_CLKCTLST_REG_EXT_RSRC_REQ            (0x1 << 8)
#define USB20H_CLKCTLST_REG_ALP_CLK_AVAIL           (0x1 << 16)
#define USB20H_CLKCTLST_REG_HT_CLK_AVAIL            (0x1 << 17)
#define USB20H_CLKCTLST_REG_BP_ON_ALP               (0x1 << 18)
#define USB20H_CLKCTLST_REG_BP_ON_HT                (0x1 << 19)
#define USB20H_CLKCTLST_REG_EXT_RSRC_STATUS         (0x1 << 24)

/* UTMI Control1 (offset 0x510) register bits and value */
//#define USB20H_UTMICTL1_REG_DEF_SETUP               (0xC7F85000)
#define USB20H_UTMICTL1_VAL_DEF_SETUP               (0xC7F85000)
#define USB20H_UTMICTL1_VAL_PWR_UP_LDO              (0xC7F85003)
#define USB20H_UTMICTL1_VAL_DISABLE_PHY_ISO_IDDQ    (0xC7F8D003)
#define USB20H_UTMICTL1_VAL_OOR_PLL                 (0x07F8D007)

/* Host Control (offset 0x200) register bits and value */
#define USB20H_HOSTCTL_REG_REMOVE_CORE_RESET        (0x1 << 8)
#define USB20H_HOSTCTL_REG_REMOVE_PHY_HARD_RESET    (0x1 << 9)
#define USB20H_HOSTCTL_REG_REMOVE_PHY_BERT_RESET    (0x1 << 10)

#define USB20H_HOSTCTL_VAL_OOR_SEQ1                 (0xFF | USB20H_HOSTCTL_REG_REMOVE_PHY_BERT_RESET)
#define USB20H_HOSTCTL_VAL_OOR_SEQ2                 (USB20H_HOSTCTL_VAL_OOR_SEQ1 | USB20H_HOSTCTL_REG_REMOVE_PHY_HARD_RESET)
#define USB20H_HOSTCTL_VAL_OOR_SEQ3                 (USB20H_HOSTCTL_VAL_OOR_SEQ2 | USB20H_HOSTCTL_REG_REMOVE_CORE_RESET)

/* PLL Control Registers for USB20H PHY */
#define USB20H_PLL_CONTROL_6_REGISTER               (6)
#define USB20H_PLL_CONTROL_6_REGISTER_DEFAULT       (0x005360C9)
#define USB20H_PLL_CONTROL_7_REGISTER               (7)
#define USB20H_PLL_CONTROL_7_REGISTER_DEFAULT       (0X000AB1F7)


/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
/* USB20 Host interrupt handler */
typedef void (*platform_usb20h_irq_handler_t)( void );

/******************************************************
 *                    Structures
 ******************************************************/

/* USB20 Host register */
typedef struct
{
    uint32_t reserved0[0x1e0/sizeof(uint32_t)];
    uint32_t clk_ctl_st;                                /* 0x1e0 */
    uint32_t resetved1[0x1c/sizeof(uint32_t)];
    uint32_t hostcontrol;                               /* 0x200 */
    uint32_t reserved2[0xfc/sizeof(uint32_t)];
    uint32_t framlengthadjust;                          /* 0x300 */
    uint32_t shimcontrol;                               /* 0x304 */
    uint32_t shimslavestatus;                           /* 0x308 */
    uint32_t shimmasterstatus;                          /* 0x30c */
    uint32_t reserved3;                                 /* 0x310 */
    uint32_t ehcistatus;                                /* 0x314 */
    uint32_t reserved4;                                 /* 0x318 */
    uint32_t reserved5;                                 /* 0x31c */
    uint32_t sb_addr_hi;                                /* 0x320 */
    uint32_t reserved6[0x1dc/sizeof(uint32_t)];
    uint32_t bert_control1;                             /* 0x500 */
    uint32_t bert_control2;                             /* 0x504 */
    uint32_t bert_status1;                              /* 0x508 */
    uint32_t bert_status2;                              /* 0x50c */
    uint32_t utmi_control1;                             /* 0x510 */
    uint32_t tp_control;                                /* 0x514 */
    uint32_t tp_out;                                    /* 0x518 */
    uint32_t gpio_sel;                                  /* 0x51c */
    uint32_t gpio_oe;                                   /* 0x520 */
    uint32_t mdio_control;                              /* 0x524 */
    uint32_t mdio_rddata;                               /* 0x528 */
    uint32_t pll_ctrl;                                  /* 0x52c */
} platform_4390x_usb_host_registers_t;

STRUCTURE_CHECK(100, platform_4390x_usb_host_registers_t, clk_ctl_st,       0x1e0);
STRUCTURE_CHECK(101, platform_4390x_usb_host_registers_t, hostcontrol,      0x200);
STRUCTURE_CHECK(102, platform_4390x_usb_host_registers_t, framlengthadjust, 0x300);
STRUCTURE_CHECK(104, platform_4390x_usb_host_registers_t, bert_control1,    0x500);

/* USB20 Host config */
typedef struct
{
    uint32_t                        usb20h_rev;
    uint32_t                        rsv1;
    uint32_t                        rsv2;
} platform_4390x_usb20h_config_t;

/* USB20 Host driver */
typedef struct
{
    volatile platform_4390x_usb_host_registers_t* core_registers;
    uint32_t                        enhanced_host_controller_interface_registers;
    uint32_t                        open_host_controller_interface_registers;
    wiced_gpio_t                    overcurrent_pin;
    wiced_gpio_t                    power_enable_pin;
    ExtIRQn_Type                    irq_number;
    platform_usb20h_irq_handler_t   irq_handler;
} platform_4390x_bcm4390x_usb_host_driver_t;

/******************************************************
 *                 Static Variables
 ******************************************************/

/* USB20 Host config data base */
//static platform_4390x_usb20h_config_t usb20h_config_db = {0}; //Rsvd

/* USB20 Host driver data base */
static platform_4390x_bcm4390x_usb_host_driver_t bcm4390x_usb_host_default_driver =
{
   .core_registers                               = (volatile platform_4390x_usb_host_registers_t*) PLATFORM_EHCI_REGBASE(0),     /* USB20 Host controller register base */
   .enhanced_host_controller_interface_registers = PLATFORM_EHCI_REGBASE(0),         /* USB20 Host EHCI controller register base */
   .open_host_controller_interface_registers     = PLATFORM_OHCI_REGBASE(0),         /* USB20 Host OHCI controller register base */
   .overcurrent_pin                              = WICED_USB_HOST_OVERCURRENT_PIN,   /* Port0 OC: GPIO0 (WICED_GPIO_1) */
   .power_enable_pin                             = WICED_USB_HOST_POWER_ENABLE_PIN,  /* Port0 Power Control: GPIO11 (WICED_GPIO_12) */
   .irq_number                                   = USB_REMAPPED_ExtIRQn,             /* USB20 Host remapped IRQ number (EHCI/OHCI share the same IRQ) */
   .irq_handler                                  = NULL                              /* USB20 Host IRQ handler */
};

static platform_4390x_bcm4390x_usb_host_driver_t *bcm4390x_usb_host_driver = &bcm4390x_usb_host_default_driver;

/******************************************************
 *               Function Declarations
 ******************************************************/

extern platform_result_t    platform_usb_host_init_irq( platform_usb20h_irq_handler_t irq_handler );
static void                 bcm4390x_usb_host_enable_irq( void );
static void                 bcm4390x_usb_host_disable_irq( void );
static void                 bcm4390x_usb_host_disable( void );
static void                 bcm4390x_usb_host_enable_irq( void );


/******************************************************
 *               Function Definitions
 ******************************************************/
static void bcm4390x_usb_host_enable(void)
{
    volatile platform_4390x_usb_host_registers_t *usb_host_registers = NULL;

    if (!bcm4390x_usb_host_driver)
    {
        WPRINT_PLATFORM_ERROR( ("Null usb20h drv!\n") );
        return;
    }

    /* Obtain usb20h register base  */
    usb_host_registers = (volatile platform_4390x_usb_host_registers_t *)(bcm4390x_usb_host_driver->core_registers);

    /* Enable usb20h core  */
    osl_core_enable(USB20H_CORE_ID);
    osl_udelay(1000);

    /* Enable misc PLL and wait till it's ready */
    usb_host_registers->clk_ctl_st |= USB20H_CLKCTLST_REG_EXT_RSRC_REQ;
    while ((usb_host_registers->clk_ctl_st & USB20H_CLKCTLST_REG_EXT_RSRC_STATUS) == 0);

    /* GCI pad control  */
    /* Port0 OC (input) */
    platform_pin_function_init(bcm4390x_usb_host_driver->overcurrent_pin, PIN_FUNCTION_USB20H_CTL1, PIN_FUNCTION_CONFIG_UNKNOWN);
    /* Port0 power control (output) */
    wiced_gpio_init(bcm4390x_usb_host_driver->power_enable_pin, OUTPUT_PUSH_PULL);
    wiced_gpio_output_low(bcm4390x_usb_host_driver->power_enable_pin);

    /* Use gci_chip_cntr[372] to de-assert utmi_suspend_i signal */
    platform_gci_chipcontrol(11, 0xd00000, 0xd00000);

    /* Default UTMI control  */
    usb_host_registers->utmi_control1 = USB20H_UTMICTL1_VAL_DEF_SETUP; //0xc7f85000;
    osl_udelay(10);

    /* Power up LDO */
    usb_host_registers->utmi_control1 = USB20H_UTMICTL1_VAL_PWR_UP_LDO; //0xc7f85003;
    osl_udelay(70);

    /* Program usb20 PHY PLL  */
    PLATFORM_PMU->pllcontrol_addr = USB20H_PLL_CONTROL_6_REGISTER; //6;
    PLATFORM_PMU->pllcontrol_data = USB20H_PLL_CONTROL_6_REGISTER_DEFAULT; //0x5360c9;
    PLATFORM_PMU->pllcontrol_addr = USB20H_PLL_CONTROL_7_REGISTER; //7;
    PLATFORM_PMU->pllcontrol_data = USB20H_PLL_CONTROL_7_REGISTER_DEFAULT; //0xab1f7;
    /* Flush deferred pll control registers writes */
    PLATFORM_PMU->pmucontrol |= PCTL_PLL_PLLCTL_UPD; //0x400;

    osl_udelay(2000);

    /* Disable PHY ISO/IDDQ  */
    usb_host_registers->utmi_control1 = USB20H_UTMICTL1_VAL_DISABLE_PHY_ISO_IDDQ; //0xc7f8d003;
    osl_udelay(125);

    /* Take PLL out of reset  */
    usb_host_registers->utmi_control1 = USB20H_UTMICTL1_VAL_OOR_PLL; //0x7f8d007;
    /* PLL lock delay + RCAL delay  */
    osl_udelay(500);

    /* Take usb20h out of reset */
    usb_host_registers->hostcontrol = USB20H_HOSTCTL_VAL_OOR_SEQ1; //1'b10
    usb_host_registers->hostcontrol = USB20H_HOSTCTL_VAL_OOR_SEQ2; //1'b9 b10
    usb_host_registers->hostcontrol = USB20H_HOSTCTL_VAL_OOR_SEQ3; //1'b8 b9 b10
}

static void usb20h_map_irq( uint8_t target_irq )
{
    /*
     * Route this bus line to target_irq bit
     * i.e. Relpace target irq with USB20 host irq
     */
    platform_irq_remap_sink( Core11_ExtIRQn, target_irq );
}

static int platform_usb_host_power_enable( int port )
{
    if (port == 0)
    {
        wiced_gpio_output_high( bcm4390x_usb_host_driver->power_enable_pin );
        return 0;
    }
    return -1;
}

static void bcm4390x_usb_host_disable( void )
{
    return;
}

platform_result_t platform_usb_host_init( void )
{
    bcm4390x_usb_host_enable();
    platform_usb_host_power_enable(0);
    return PLATFORM_SUCCESS;
}

void platform_usb_host_deinit( void )
{
    bcm4390x_usb_host_disable_irq();
    bcm4390x_usb_host_disable();
}

platform_result_t platform_usb_host_init_irq( platform_usb20h_irq_handler_t irq_handler )
{
    usb20h_map_irq(bcm4390x_usb_host_driver->irq_number);

    if (irq_handler == NULL)
    {
        WPRINT_PLATFORM_ERROR( ("Null input!\n") );
        return PLATFORM_ERROR;
    }
    bcm4390x_usb_host_driver->irq_handler = irq_handler;

    bcm4390x_usb_host_enable_irq();

    return PLATFORM_SUCCESS;
}

static void bcm4390x_usb_host_enable_irq( void )
{
    if (bcm4390x_usb_host_driver->irq_handler == NULL)
    {
        WPRINT_PLATFORM_DEBUG( ("No isr set before irq enable!\n") );
    }
    platform_irq_enable_irq( bcm4390x_usb_host_driver->irq_number );
}

static void bcm4390x_usb_host_disable_irq( void )
{
    platform_irq_disable_irq( bcm4390x_usb_host_driver->irq_number );
}

WWD_RTOS_DEFINE_ISR( platform_usb_host_isr )
{
    bcm4390x_usb_host_driver->irq_handler();
}
WWD_RTOS_MAP_ISR( platform_usb_host_isr, USB_HOST_ISR )

uint32_t bcm4390x_usb_host_get_irq_number( void )
{
    return bcm4390x_usb_host_driver->irq_number;
}

uint32_t bcm4390x_usb_host_get_enhanced_host_controller_interface_address( void )
{
    return bcm4390x_usb_host_driver->enhanced_host_controller_interface_registers;
}

uint32_t bcm4390x_usb_host_get_open_host_controller_interface_address( void )
{
    return bcm4390x_usb_host_driver->open_host_controller_interface_registers;
}
