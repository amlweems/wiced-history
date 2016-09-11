/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "platform_peripheral.h"
#include "platform_map.h"

/******************************************************
 *                      Macros
 ******************************************************/

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

static uint32_t
platform_chipcontrol(volatile uint32_t* addr_reg, volatile uint32_t* ctrl_reg,
                     uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    uint32_t ret;
    uint32_t val;
    uint32_t flags;

    WICED_SAVE_INTERRUPTS(flags);

    *addr_reg = reg_offset;
    val = *ctrl_reg;
    ret = (val & ~clear_mask) | set_mask;
    if (val != ret)
    {
        *ctrl_reg = ret;
    }

    WICED_RESTORE_INTERRUPTS(flags);

    return ret;
}

static uint32_t
platform_chipstatus(volatile uint32_t* addr_reg, volatile uint32_t* status_reg,
                    uint8_t reg_offset)
{
    uint32_t ret;
    uint32_t flags;

    WICED_SAVE_INTERRUPTS(flags);

    *addr_reg = reg_offset;
    ret = *status_reg;

    WICED_RESTORE_INTERRUPTS(flags);

    return ret;
}

uint32_t platform_gci_chipcontrol(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(GCI_INDIRECT_ADDR_REG, GCI_CHIPCONTROL_REG,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_gci_gpiocontrol(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(GCI_INDIRECT_ADDR_REG, GCI_GPIOCONTROL_REG,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_gci_gpiostatus(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(GCI_INDIRECT_ADDR_REG, GCI_GPIOSTATUS_REG,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_gci_gpiowakemask(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(GCI_INDIRECT_ADDR_REG, GCI_GPIOWAKEMASK_REG,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_gci_chipstatus(uint8_t reg_offset)
{
    return platform_chipstatus(GCI_INDIRECT_ADDR_REG, GCI_CHIPSTATUS_REG,
                               reg_offset);
}

uint32_t platform_pmu_chipcontrol(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(&PLATFORM_PMU->chipcontrol_addr, &PLATFORM_PMU->chipcontrol_data,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_pmu_res_updown_time(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(&PLATFORM_PMU->res_table_sel, &PLATFORM_PMU->res_updn_timer,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_pmu_res_dep_mask(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(&PLATFORM_PMU->res_table_sel, &PLATFORM_PMU->res_dep_mask,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_pmu_regulatorcontrol(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(&PLATFORM_PMU->regcontrol_addr, &PLATFORM_PMU->regcontrol_data,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_pmu_pllcontrol(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(&PLATFORM_PMU->pllcontrol_addr, &PLATFORM_PMU->pllcontrol_data,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_common_chipcontrol(volatile uint32_t* reg, uint32_t clear_mask, uint32_t set_mask)
{
    uint32_t ret;
    uint32_t val;
    uint32_t flags;

    WICED_SAVE_INTERRUPTS(flags);

    val = *reg;
    ret = (val & ~clear_mask) | set_mask;
    if (val != ret)
    {
        *reg = ret;
    }

    WICED_RESTORE_INTERRUPTS(flags);

    return ret;
}
