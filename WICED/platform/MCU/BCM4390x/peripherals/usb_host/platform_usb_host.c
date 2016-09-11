/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * WICED 4390x USB Host driver.
 */

#include "typedefs.h"
#include "sbchipc.h"
#include "platform_peripheral.h"
#include "platform_map.h"
#include "platform_config.h"
#include "platform_usb.h"
#include "platform_appscr4.h"
#include "platform_pinmux.h"
#include "hndsoc.h"
#include "wiced_osl.h"
#include "wiced_platform.h"
#include "wwd_rtos_isr.h"
#include "platform/wwd_platform_interface.h"


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

/* BCM4390x USB20 Host HCI (host controller interface) number = 2 (EHCI+OHCI) */
//#define BCM4390X_USB_HOST_CONTROLLER_INTERFACE_NUM  (2)

/*
 * The below register defines can be moved to structure-pointer format after
 * ChipCommon and GCI structure layouts are fully defined in platform_appscr4.h
 */
#define CHIPCOMMON_CHIP_STATUS_REG                  *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x02C)))

/* ChipCommon ChipStatus register bits */
/*
 * The 4-byte Chip Status register is read-only and has a device-dependent reset value.
 * This register is used to read HW status information such as strapping options.
 */
#define CC_CHIP_STATUS_USB20_DEVICE_SELECTION       (1 << 16)   /* 0: USB Host mode; 1: USB Device mode */
#define CC_CHIP_STATUS_HOST_IFACE_STRAP_2           (1 << 22)   /* 0: HSIC-PHY; 1: USB-PHY */
#define CC_CHIP_STATUS_USB_PHY_SELECTION            CC_CHIP_STATUS_HOST_IFACE_STRAP_2

/* OHCI Registers */
/* HcRhDescriptorA */
#define OHCI_RH_POWER_SWITCH_MODE_PER_PORT          (1 << 8)
#define OHCI_RH_OVER_CURRENT_PROTECT_PER_PORT       (1 << 11)
/* HcRhDescriptorB */
#define OHCI_RH_PORT_POWER_CONTROL_MASK_SHIFT       16
#define OHCI_RH_PORT_POWER_CONTROL_MASK_DEFAULT     (0x6 << OHCI_RH_PORT_POWER_CONTROL_MASK_SHIFT)

/* HcRhPortStatus */
#define OHCI_RH_PORT_SET_PORT_POWER                 (1 << 8)
#define OHCI_RH_PORT_CLEAR_PORT_POWER               (1 << 9)

/* EHCI Registers */
/* PORTSC register */
#define EHCI_RH_PORT_POWER_ON                       (1 << 12)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/
/* BCM4390x USB20 Host register */
typedef struct
{
    uint32_t hc_cap_version;                            /* 0x000 */
    uint32_t hcsparams;                                 /* 0x004 */
    uint32_t hccparams;                                 /* 0x008 */
    uint32_t hcsp_portroute;                            /* 0x00c */
    uint32_t usbcmd;                                    /* 0x010 */
    uint32_t usbsts;                                    /* 0x014 */
    uint32_t usbintr;                                   /* 0x018 */
    uint32_t frindex;                                   /* 0x01c */
    uint32_t ctrldsssegment;                            /* 0x020 */
    uint32_t periodclistbase;                           /* 0x024 */
    uint32_t asynclistaddr;                             /* 0x028 */
    uint32_t reserved0[9];                              /* 0x02c */
    uint32_t configflag;                                /* 0x050 */
    uint32_t portsc[2];                                 /* 0x054 */
    uint32_t reserved1[0x184/sizeof(uint32_t)];         /* 0x05c */
    uint32_t clk_ctl_st;                                /* 0x1e0 */
    uint32_t resetved2[0x1c/sizeof(uint32_t)];
    uint32_t hostcontrol;                               /* 0x200 */
    uint32_t reserved3[0xfc/sizeof(uint32_t)];
    uint32_t framlengthadjust;                          /* 0x300 */
    uint32_t shimcontrol;                               /* 0x304 */
    uint32_t shimslavestatus;                           /* 0x308 */
    uint32_t shimmasterstatus;                          /* 0x30c */
    uint32_t reserved4;                                 /* 0x310 */
    uint32_t ehcistatus;                                /* 0x314 */
    uint32_t reserved5;                                 /* 0x318 */
    uint32_t reserved6;                                 /* 0x31c */
    uint32_t sb_addr_hi;                                /* 0x320 */
    uint32_t reserved7[0x1dc/sizeof(uint32_t)];
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

STRUCTURE_CHECK(100, platform_4390x_usb_host_registers_t, clk_ctl_st, 0x1e0);
STRUCTURE_CHECK(101, platform_4390x_usb_host_registers_t, hostcontrol, 0x200);
STRUCTURE_CHECK(102, platform_4390x_usb_host_registers_t, framlengthadjust, 0x300);
STRUCTURE_CHECK(104, platform_4390x_usb_host_registers_t, bert_control1, 0x500);

typedef struct {
    uint32_t hc_revision;                               /* 0x00 */
    uint32_t hc_control;                                /* 0x04 */
    uint32_t hc_command_status;                         /* 0x08 */
    uint32_t hc_interrupt_status;                       /* 0x0C */
    uint32_t hc_interrupt_enable;                       /* 0x10 */
    uint32_t hc_interrupt_disable;                      /* 0x14 */
    uint32_t hc_hccca;                                  /* 0x18 */
    uint32_t hc_period_current_ed;                      /* 0x1C */
    uint32_t hc_control_head_ed;                        /* 0x20 */
    uint32_t hc_control_current_ed;                     /* 0x24 */
    uint32_t hc_bulk_head_ed;                           /* 0x28 */
    uint32_t hc_bulk_current_ed;                        /* 0x2C */
    uint32_t hc_done_head;                              /* 0x30 */
    uint32_t hc_fm_interval;                            /* 0x34 */
    uint32_t hc_fm_remaining;                           /* 0x38 */
    uint32_t hc_fm_number;                              /* 0x3C */
    uint32_t hc_periodic_start;                         /* 0x40 */
    uint32_t hc_ls_threashold;                          /* 0x44 */
    uint32_t hc_rh_descriptor_a;                        /* 0x48 */
    uint32_t hc_rh_descriptor_b;                        /* 0x4C */
    uint32_t hc_rh_status;                              /* 0x50 */
    uint32_t hc_rh_port_status[15];                     /* 0x54 */
} platform_usb_ohci_register_t;

STRUCTURE_CHECK(105, platform_usb_ohci_register_t, hc_hccca, 0x18);
STRUCTURE_CHECK(106, platform_usb_ohci_register_t, hc_rh_descriptor_a, 0x48);
STRUCTURE_CHECK(107, platform_usb_ohci_register_t, hc_rh_port_status, 0x54);

/* BCM4390x USB20 Host config */
typedef struct
{
    uint32_t                        usb20h_rev;
    uint32_t                        reserve1;
    uint32_t                        reserve2;
} platform_4390x_usb_host_config_t;

/* BCM4390x USB20 Host driver */
typedef struct
{
    volatile platform_4390x_usb_host_registers_t*   core_registers;
    wiced_gpio_t                                    overcurrent_pin;
    wiced_gpio_t                                    power_enable_pin;
    ExtIRQn_Type                                    irq_number;
    platform_usb_host_irq_handler_t                 irq_handler;
} platform_4390x_usb_host_driver_t;

/******************************************************
 *                 Static Variables
 ******************************************************/

/* BCM4390x USB20 Host config data base */
//static platform_4390x_usb_host_config_t bcm4390x_usb_host_config_db = {0}; //Rsvd

/* BCM4390x USB20 Host driver data base */
static platform_4390x_usb_host_driver_t bcm4390x_usb_host_default_driver =
{
   .core_registers          = (volatile platform_4390x_usb_host_registers_t*) PLATFORM_EHCI_REGBASE(0),     /* USB20 Host controller register base */
   .overcurrent_pin         = WICED_USB_HOST_OVERCURRENT,   /* Port0 OC: GPIO0 (WICED_GPIO_1) */
   .power_enable_pin        = WICED_USB_HOST_POWER_ENABLE,  /* Port0 Power Control: GPIO11 (WICED_GPIO_12) */
   .irq_number              = USB_REMAPPED_ExtIRQn,         /* USB20 Host remapped IRQ number (EHCI/OHCI share the same IRQ) */
   .irq_handler             = NULL                          /* USB20 Host IRQ handler */
};

static platform_4390x_usb_host_driver_t *bcm4390x_usb_host_driver = &bcm4390x_usb_host_default_driver;

/* BCM4390x USB20 Host HCI (host controller interface) resource list for USB stack driver */
static platform_usb_host_hci_resource_t bcm4390x_usb_host_hci_resource_list[] =
{
    /* USB20 Host OHCI controller */
    {
        .usb_host_hci_type          = USB_HOST_CONTROLLER_INTERFACE_OHCI,   /* USB20 Host OHCI controller */
        .usb_host_hci_ioaddress     = PLATFORM_OHCI_REGBASE(0),             /* USB20 Host OHCI controller register base */
        .usb_host_hci_irq_number    = USB_REMAPPED_ExtIRQn                  /* USB20 Host OHCI IRQ number (EHCI/OHCI share the same IRQ) */
    },
    /* USB20 Host EHCI controller */
    {
        .usb_host_hci_type          = USB_HOST_CONTROLLER_INTERFACE_EHCI,   /* USB20 Host EHCI controller */
        .usb_host_hci_ioaddress     = PLATFORM_EHCI_REGBASE(0),             /* USB20 Host EHCI controller register base */
        .usb_host_hci_irq_number    = USB_REMAPPED_ExtIRQn                  /* USB20 Host EHCI IRQ number (EHCI/OHCI share the same IRQ) */
    }
};

/******************************************************
 *               Function Declarations
 ******************************************************/
static void bcm4390x_usb_host_enable( void );
static void bcm4390x_usb_host_map_irq( uint8_t target_irq );
static int bcm4390x_usb_host_power_enable( int port );
static void bcm4390x_usb_host_disable( void );
static void bcm4390x_usb_host_enable_irq( void );
static void bcm4390x_usb_host_disable_irq( void );
static wiced_bool_t bcm4390x_is_board_in_usb_phy_mode( void );
static wiced_bool_t bcm4390x_is_board_in_usb_host_mode( void );

/******************************************************
 *               Function Definitions
 ******************************************************/
static void bcm4390x_usb_host_enable( void )
{
    volatile platform_4390x_usb_host_registers_t *usb_host_registers = NULL;
    volatile platform_usb_ohci_register_t *ohci_controller = (volatile platform_usb_ohci_register_t *) PLATFORM_OHCI_REGBASE(0);

    if (!bcm4390x_usb_host_driver)
    {
        WPRINT_PLATFORM_ERROR( ("Null usb host drv!\n") );
        return;
    }

    /* Add platform USB using ALP clock fixup */
    PLATFORM_USB_ALP_CLOCK_RES_UP();

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
    platform_pinmux_init(bcm4390x_usb_host_driver->overcurrent_pin, PIN_FUNCTION_USB20H_CTL1);
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

    /* Set OHCI controller power switching mode and over current mode*/
    ohci_controller->hc_rh_descriptor_a |= OHCI_RH_POWER_SWITCH_MODE_PER_PORT | OHCI_RH_OVER_CURRENT_PROTECT_PER_PORT;
    ohci_controller->hc_rh_descriptor_b |= OHCI_RH_PORT_POWER_CONTROL_MASK_DEFAULT;
}

static void bcm4390x_usb_host_map_irq( uint8_t target_irq )
{
    /*
     * Route this bus line to target_irq bit
     * i.e. Relpace target irq with USB20 host irq
     */
    platform_irq_remap_sink( Core11_ExtIRQn, target_irq );
}

static int bcm4390x_usb_host_power_enable( int port )
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
    /* Add platform USB using ALP clock fixup */
    PLATFORM_USB_ALP_CLOCK_RES_DOWN( NULL, WICED_FALSE );

    return;
}

static void bcm4390x_usb_host_enable_irq( void )
{
    if (bcm4390x_usb_host_driver->irq_handler == NULL)
    {
        WPRINT_PLATFORM_DEBUG( ("No ISR set before IRQ enable!\n") );
    }
    platform_irq_enable_irq( bcm4390x_usb_host_driver->irq_number );
}

static void bcm4390x_usb_host_disable_irq( void )
{
    platform_irq_disable_irq( bcm4390x_usb_host_driver->irq_number );
}

static wiced_bool_t bcm4390x_is_board_in_usb_phy_mode( void )
{
    wiced_bool_t is_usb_phy_mode = WICED_FALSE;

    if ( (CHIPCOMMON_CHIP_STATUS_REG & CC_CHIP_STATUS_USB_PHY_SELECTION) )
    {
        is_usb_phy_mode = WICED_TRUE;
    }

    WPRINT_PLATFORM_INFO( ("Detected board strapping is in %s mode!!\n", (is_usb_phy_mode == WICED_TRUE)? "USB-PHY":"HSIC-PHY") );
    return is_usb_phy_mode;
}

static wiced_bool_t bcm4390x_is_board_in_usb_host_mode( void )
{
    wiced_bool_t is_host_mode = WICED_FALSE;

    if ( (CHIPCOMMON_CHIP_STATUS_REG & CC_CHIP_STATUS_USB20_DEVICE_SELECTION) == 0 )
    {
        is_host_mode = WICED_TRUE;
    }

    WPRINT_PLATFORM_INFO( ("Detected board is in USB %s mode!!\n", (is_host_mode == WICED_TRUE)? "Host":"Device") );
    return is_host_mode;
}

WWD_RTOS_DEFINE_ISR( platform_usb_host_isr )
{
    bcm4390x_usb_host_driver->irq_handler();
}
WWD_RTOS_MAP_ISR( platform_usb_host_isr, USB_HOST_ISR )

platform_result_t platform_usb_host_init( void )
{
    if (bcm4390x_is_board_in_usb_phy_mode() == WICED_FALSE)
    {
        WPRINT_PLATFORM_INFO( ("Detected board strapping is NOT in USB-PHY mode!!!\n") );
        return PLATFORM_ERROR;
    }
    if (bcm4390x_is_board_in_usb_host_mode() == WICED_FALSE)
    {
        WPRINT_PLATFORM_INFO( ("Detected board is NOT in USB Host mode!!!\n") );
        WPRINT_PLATFORM_INFO( ("Please plug micro-A USB cord on bcm4390x uAB DRD port before board booting!\n") );
        return PLATFORM_ERROR;
    }

    bcm4390x_usb_host_enable();
    bcm4390x_usb_host_power_enable(0);
    return PLATFORM_SUCCESS;
}

/*
 * plaftorm_usb_host_post_init:
 *      Put things that must be done after EHCI/OHCI driver initialized in this function.
 */
platform_result_t platform_usb_host_post_init( void )
{
    volatile platform_4390x_usb_host_registers_t *ehci_controller = (volatile platform_4390x_usb_host_registers_t *) PLATFORM_EHCI_REGBASE(0);
    volatile platform_usb_ohci_register_t *ohci_controller = (volatile platform_usb_ohci_register_t *) PLATFORM_OHCI_REGBASE(0);

    /*
     *  Clear EHCI/OHCI port1 port power.
     *  4390x EHCI/OHCI root hub supports 2 port but only port0 is physically connected to external pins. Turn off un-used port1 power.
     *  This operation MUST BE done after the EHCI/OHCI driver initialization because the port power is set during the driver initialization.
     */
    ohci_controller->hc_rh_port_status[1] = OHCI_RH_PORT_CLEAR_PORT_POWER;
    ehci_controller->portsc[1] &= ~ EHCI_RH_PORT_POWER_ON;
    return PLATFORM_SUCCESS;
}

void platform_usb_host_deinit( void )
{
    bcm4390x_usb_host_disable_irq();
    bcm4390x_usb_host_disable();
}

platform_result_t platform_usb_host_init_irq( platform_usb_host_irq_handler_t irq_handler )
{
    bcm4390x_usb_host_map_irq(bcm4390x_usb_host_driver->irq_number);

    if (irq_handler == NULL)
    {
        WPRINT_PLATFORM_ERROR( ("Null input!\n") );
        return PLATFORM_ERROR;
    }
    bcm4390x_usb_host_driver->irq_handler = irq_handler;

    //bcm4390x_usb_host_enable_irq();

    return PLATFORM_SUCCESS;
}

platform_result_t platform_usb_host_enable_irq( void )
{
    if (bcm4390x_usb_host_driver->irq_handler == NULL)
    {
        WPRINT_PLATFORM_ERROR( ("No irq handler!\n") );
        return PLATFORM_ERROR;
    }
    bcm4390x_usb_host_enable_irq();

    return PLATFORM_SUCCESS;
}

platform_result_t platform_usb_host_disable_irq( void )
{
    bcm4390x_usb_host_disable_irq();
    return PLATFORM_SUCCESS;
}

platform_result_t platform_usb_host_get_hci_resource( platform_usb_host_hci_resource_t *resource_list_buf, uint32_t buf_size, uint32_t *resource_total_num )
{
    uint32_t resource_num = (sizeof(bcm4390x_usb_host_hci_resource_list) / sizeof(bcm4390x_usb_host_hci_resource_list[0]));
    uint32_t resource_list_size;

    *resource_total_num = 0;

    /* Error checking  */
    if ((resource_num == 0) || (resource_num > USB_HOST_CONTROLLER_INTERFACE_MAX))
    {
        WPRINT_PLATFORM_ERROR( ("Internal error! Check HCI resource list assign\n") );
        return PLATFORM_ERROR;
    }
    if (resource_list_buf == NULL)
    {
        WPRINT_PLATFORM_ERROR( ("Null input!\n") );
        return PLATFORM_ERROR;
    }

    resource_list_size = (resource_num * sizeof(platform_usb_host_hci_resource_t));
    if (buf_size < resource_list_size)
    {
        WPRINT_PLATFORM_ERROR( ("HCI resource buf size not enough! buf_size=%lu, resource_list_size=%lu\n", buf_size, resource_list_size) );
        return PLATFORM_ERROR;
    }

    /* Get HCI resource list and total resource number */
    memcpy((void *)resource_list_buf, (void *)bcm4390x_usb_host_hci_resource_list, resource_list_size);
    *resource_total_num = resource_num;
    WPRINT_PLATFORM_INFO( ("USB Host support %u HCI resource\n", (unsigned)resource_num) );

    return PLATFORM_SUCCESS;
}
