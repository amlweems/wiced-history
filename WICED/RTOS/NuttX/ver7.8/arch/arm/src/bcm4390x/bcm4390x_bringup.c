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
#include <nuttx/init.h>
#include <nuttx/arch.h>
#include <nuttx/watchdog.h>
#include <nuttx/pwm.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <debug.h>
#include <fcntl.h>

#include "platform_appscr4.h"
#include "platform_assert.h"
#include "RTOS/wwd_rtos_interface.h"

#define BCM4390X_BRINGUP_AUX_TASK_ARG          0xABCDEF

#define BCM4390X_BRINGUP_ASSERT( assertion )   do { if (!(assertion)) { WICED_ASSERTION_FAIL_ACTION();} } while(0)

#define BCM4390X_BRINGUP_WATCHDOG_PATH         CONFIG_WATCHDOG_DEVPATH

#define BCM4390X_BRINGUP_PWM_PATH              "/dev/pwm3"
#define BCM4390X_BRINGUP_PWM_FREQ_1            5000
#define BCM4390X_BRINGUP_PWM_DUTY_PCT_1        20

#define BCM4390X_BRINGUP_PWM_FREQ_2            10000
#define BCM4390X_BRINGUP_PWM_DUTY_PCT_2        50

#if 0
#define BCM4390X_BRINGUP_MAIN_TASK_SLEEP()     sched_yield()
#define BCM4390X_BRINGUP_AUX_TASK_SLEEP()      sched_yield()
#define BCM4390X_BRINGUP_PUTS(s)
#define BCM4390X_BRINGUP_INTERRUPT_EACH_N_ITER 100000
#else
#define BCM4390X_BRINGUP_MAIN_TASK_SLEEP()     host_rtos_delay_milliseconds(100)
#define BCM4390X_BRINGUP_AUX_TASK_SLEEP()      host_rtos_delay_milliseconds(500)
#define BCM4390X_BRINGUP_PUTS(format, ...)     dbg(format, ##__VA_ARGS__)
#define BCM4390X_BRINGUP_INTERRUPT_EACH_N_ITER 10
#endif

volatile unsigned bcm4390x_bringup_main_task_counter = 0;
volatile unsigned bcm4390x_bringup_aux_task_counter = 0;

volatile unsigned bcm4390x_bringup_aux_interrupt_counter = 0;
volatile unsigned bcm4390x_bringup_get_interrupt_counter = 0;

volatile unsigned bcm4390x_bringup_sigusr_counter = 0;

volatile wwd_time_t bcm4390x_bringup_current_time = 0;

static host_thread_type_t bcm4390x_bringup_aux_thread;
static int bcm4390x_bringup_main_task_pid = -1;

static int bcm4390x_bringup_watchdog_fd = -1;

static int bcm4390x_bringup_pwm_fd = -1;

static void bcm4390x_bringup_trigger_interrupt(void)
{
  PLATFORM_APPSCR4->sw_int = 1;

  asm volatile ("DSB" : : : "memory");
}

static int bcm4390x_sw0_extirq(int irq, void *context)
{
  bcm4390x_bringup_get_interrupt_counter++;

  pthread_kill(bcm4390x_bringup_aux_thread.handle, SIGUSR1);
  kill(bcm4390x_bringup_main_task_pid, SIGUSR1);

  sched_yield();

  return OK;
}

static void bcm4390x_siguser_action(int signo, siginfo_t *siginfo, void *arg)
{
  bcm4390x_bringup_sigusr_counter++;

  BCM4390X_BRINGUP_PUTS("Hello from SIGUSR1 signal handler (pid=%d)!\n", getpid());
}

static void bcm4390x_set_siguser_action(void)
{
  struct sigaction act;
  int status;

  memset(&act, 0, sizeof(struct sigaction));
  act.sa_sigaction = bcm4390x_siguser_action;
  act.sa_flags     = SA_SIGINFO;

  (void)sigemptyset(&act.sa_mask);

  status = sigaction(SIGUSR1, &act, NULL);
  BCM4390X_BRINGUP_ASSERT(status == 0);
}

static void bcm4390x_bringup_aux_task(wwd_thread_arg_t arg)
{
  /* Check argument */
  while (arg != BCM4390X_BRINGUP_AUX_TASK_ARG)
  {
    BCM4390X_BRINGUP_PUTS("Bad aux thread argument!\n");
  }

  /* Setup signal handler for SIGUSR1 */

  bcm4390x_set_siguser_action();

  /* Configure interrupt handler */

  irq_attach(SW0_ExtIRQn, bcm4390x_sw0_extirq);
  up_enable_irq(SW0_ExtIRQn);

  /* Run main loop */

  while (1)
  {
    int result;
    struct pwm_info_s pwm_info;

    bcm4390x_bringup_aux_task_counter++;

    bcm4390x_bringup_aux_interrupt_counter++;
    if (bcm4390x_bringup_aux_interrupt_counter >= BCM4390X_BRINGUP_INTERRUPT_EACH_N_ITER)
    {
      bcm4390x_bringup_aux_interrupt_counter = 0;
      bcm4390x_bringup_trigger_interrupt();

      BCM4390X_BRINGUP_PUTS("AUX thread has just triggered interrupt!\n");
    }

    BCM4390X_BRINGUP_AUX_TASK_SLEEP();

    BCM4390X_BRINGUP_PUTS("Hello from AUX thread!\n");

    kill(bcm4390x_bringup_main_task_pid, SIGUSR1);

    BCM4390X_BRINGUP_PUTS("AUX thread has just sent signal to main!\n");

    result = ioctl(bcm4390x_bringup_watchdog_fd, WDIOC_KEEPALIVE, 0);
    BCM4390X_BRINGUP_ASSERT(result == 0);

    if (bcm4390x_bringup_aux_task_counter & 0x1)
    {
      pwm_info.frequency = BCM4390X_BRINGUP_PWM_FREQ_1;
      pwm_info.duty      = ((uint32_t)BCM4390X_BRINGUP_PWM_DUTY_PCT_1 << 16) / 100;
    }
    else
    {
      pwm_info.frequency = BCM4390X_BRINGUP_PWM_FREQ_2;
      pwm_info.duty      = ((uint32_t)BCM4390X_BRINGUP_PWM_DUTY_PCT_2 << 16) / 100;
    }
    result = ioctl(bcm4390x_bringup_pwm_fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)&pwm_info);
    BCM4390X_BRINGUP_ASSERT(result == 0);
  }
}

static void bcm4390x_bringup_create_aux_thread(void)
{
  wwd_result_t wwd_result;

  wwd_result = host_rtos_create_thread_with_arg(&bcm4390x_bringup_aux_thread,
                                                bcm4390x_bringup_aux_task,
                                                "aux",
                                                NULL,
                                                WWD_THREAD_STACK_SIZE,
                                                RTOS_DEFAULT_THREAD_PRIORITY, BCM4390X_BRINGUP_AUX_TASK_ARG);
  BCM4390X_BRINGUP_ASSERT(wwd_result == WWD_SUCCESS);
}

int bcm4390x_bringup_main(int argc, char *argv[])
{
  wwd_result_t wwd_result;
  struct pwm_info_s pwm_info;
  int result;

  /* Open and start watchdog */

  bcm4390x_bringup_watchdog_fd = open(BCM4390X_BRINGUP_WATCHDOG_PATH, O_RDONLY);
  BCM4390X_BRINGUP_ASSERT(bcm4390x_bringup_watchdog_fd >= 0);

  result = ioctl(bcm4390x_bringup_watchdog_fd, WDIOC_START, 0);
  BCM4390X_BRINGUP_ASSERT(result == 0);

  /* Register, open and start pwm */

  result = pwm_devinit();
  BCM4390X_BRINGUP_ASSERT(result == 0);

  bcm4390x_bringup_pwm_fd = open(BCM4390X_BRINGUP_PWM_PATH, O_RDONLY);
  BCM4390X_BRINGUP_ASSERT(bcm4390x_bringup_pwm_fd >= 0);

  pwm_info.frequency = BCM4390X_BRINGUP_PWM_FREQ_1;
  pwm_info.duty      = ((uint32_t)BCM4390X_BRINGUP_PWM_DUTY_PCT_1 << 16) / 100;
  result = ioctl(bcm4390x_bringup_pwm_fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)&pwm_info);
  BCM4390X_BRINGUP_ASSERT(result == 0);

  result = ioctl(bcm4390x_bringup_pwm_fd, PWMIOC_START, 0);
  BCM4390X_BRINGUP_ASSERT(result == 0);

  result = ioctl(bcm4390x_bringup_pwm_fd, PWMIOC_STOP, 0);
  BCM4390X_BRINGUP_ASSERT(result == 0);

  result = ioctl(bcm4390x_bringup_pwm_fd, PWMIOC_START, 0);
  BCM4390X_BRINGUP_ASSERT(result == 0);

  /* Record main task pid */

  bcm4390x_bringup_main_task_pid = getpid();

  /* Setup signal handler for SIGUSR1 */

  bcm4390x_set_siguser_action();

  /* Create aux thread, destroy it, wait for termination and create again (pid will be new) */

  bcm4390x_bringup_create_aux_thread();

  wwd_result = host_rtos_finish_thread(&bcm4390x_bringup_aux_thread);
  BCM4390X_BRINGUP_ASSERT(wwd_result == WWD_SUCCESS);

  wwd_result = host_rtos_join_thread(&bcm4390x_bringup_aux_thread);
  BCM4390X_BRINGUP_ASSERT(wwd_result == WWD_SUCCESS);

  bcm4390x_bringup_create_aux_thread();

  /* Run main loop */

  while (1)
  {
    bcm4390x_bringup_current_time = host_rtos_get_time();

    bcm4390x_bringup_main_task_counter++;

    BCM4390X_BRINGUP_MAIN_TASK_SLEEP();

    BCM4390X_BRINGUP_PUTS("Hello from MAIN thread!\n");
  }

  return 0;
}
