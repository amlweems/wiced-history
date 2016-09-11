/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <nuttx/config.h>
#include <nuttx/arch.h>
#include <nuttx/init.h>

#include "up_internal.h"

#include <arch/irq.h>
#include "irq/irq.h"

#include "platform_appscr4.h"

void up_disable_irq(int irq)
{
  platform_irq_disable_irq( irq );
}

void up_enable_irq(int irq)
{
  platform_irq_enable_irq( irq );
}

uint32_t platform_irq_demuxer_hook(uint32_t mask)
{
  int i;

  for (i = 0; mask && (i < NR_IRQS); i++)
  {
    const uint32_t single_irq_mask = IRQN2MASK(i);

    if ((mask & single_irq_mask) && g_irqvector[i] && (g_irqvector[i] != irq_unexpected_isr))
    {
      irq_dispatch(i, (uint32_t *)current_regs);

      /*
       * This makes all registered interrupts be handled here and not propagated to main WICED code.
       * It is possible (by not clearing bit here) to make double handling - here and then in main WICED code.
       */
      mask &= ~single_irq_mask;
    }
  }

  return mask;
}
