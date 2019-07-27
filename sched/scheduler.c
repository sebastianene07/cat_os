/*
 * scheduler.c
 *
 * Created: 4/6/2018 3:09:38 AM
 *  Author: sene
 */

#include <board.h>

#include <scheduler.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

/* Running task list */

LIST_HEAD(g_tcb_list);

/* Waiting for sem task list */

LIST_HEAD(g_tcb_waiting_list);

/* Current running task */

struct list_head *g_current_tcb = NULL;

/**************************************************************************
 * Name:
 *  sched_idle_task
 *
 * Description:
 *  This task is responsible for cleaning up resources used by the tasks. It
 *  can also be used to monitor the HEAP usage, scan for corruptions and
 *  asking the CPU to enter deep sleep mode if no other operation is pending
 *  to be executed.
 *
 * Assumptions:
 *  This task should never exit.
 *
 *************************************************************************/
static void sched_idle_task(void)
{
  while (1)
  {

    /* Check if we need to free any HALTED tasks */

    disable_int();

    bool is_halt_task;
    do {
      is_halt_task = false;
      struct tcb_s *current = NULL;
      list_for_each_entry(current, &g_tcb_waiting_list, next_tcb)
      {
        if (current != NULL && current->t_state == HALTED)
        {
          list_del(&current->next_tcb);
          free(current);
          is_halt_task = true;
          break;
        }
      }
    } while (is_halt_task);

    enable_int();
  }
}

/**************************************************************************
 * Name:
 *  sched_init
 *
 * Description:
 *  Init scheduler resources.
 *
 * Return Value:
 *  OK in case of success otherwise a negate value.
 *
 *************************************************************************/
int sched_init(void)
{
  /* Create idle task */

  int ret = sched_create_task(sched_idle_task,
                              CONFIG_SCHEDULER_IDLE_TASK_STACK_SIZE);
  if (ret < 0)
  {
    return ret;
  }

  g_current_tcb = g_tcb_list.next;

  return 0;
}

/**************************************************************************
 * Name:
 *  sched_default_task_exit_point
 *
 * Description:
 *  Called when a task has finished executing the task entry point and it's
 *  about to tear down it's resources. This function should do the cleanup
 *  look for opened file descriptors and release the accessed resources.
 *
 * Assumptions:
 *  This function does not exit.
 *
 *************************************************************************/
void sched_default_task_exit_point(void)
{
  __disable_irq();

  /* Move this task in the HALT state and wait for the idle task to clean up
   * it's memory.
   */

  struct tcb_s *this_tcb = sched_get_current_task();
  this_tcb->t_state           = HALTED;
  this_tcb->waiting_tcb_sema  = NULL;

  /* Switch context to the next running task */

  NVIC_TriggerSysTick();
  sched_context_switch();
}

/**************************************************************************
 * Name:
 *  sched_create_task
 *
 * Description:
 *  Create a new task.
 *
 * Input Parameters:
 *  task_entry_point - the entry point of a task
 *  stack_size       - the stack size of the new task
 *
 * Assumptions:
 *  Call this function with interrupts disabled.
 *
 * Return Value:
 *  OK in case of success otherwise a negate value.
 *
 *************************************************************************/
int sched_create_task(void (*task_entry_point)(void), uint32_t stack_size)
{
  struct tcb_s *task_tcb = malloc(sizeof(struct tcb_s) + stack_size);
  if (task_tcb == NULL)
  {
    return -ENOMEM;
  }

  task_tcb->entry_point    = task_entry_point;
  task_tcb->stack_ptr_base = (void *)task_tcb + sizeof(struct tcb_s);
  task_tcb->stack_ptr_top  = (void *)task_tcb + stack_size;
  task_tcb->t_state        = READY;

#ifdef CONFIG_SCHEDULER_TASK_COLORATION
  /* The effective stack size is base - top */

  for (uint8_t *ptr = task_tcb->stack_ptr_base;
     ptr < (uint8_t*)task_tcb->stack_ptr_top;
     ptr = ptr + sizeof(uint32_t))
  {
    *((uint32_t *)ptr) = 0xDEADBEEF;
  }
#endif

  /* Initial MCU context */

  task_tcb->mcu_context[0] = NULL;
  task_tcb->mcu_context[1] = NULL;
  task_tcb->mcu_context[2] = NULL;
  task_tcb->mcu_context[3] = NULL;
  task_tcb->mcu_context[4] = NULL;
  task_tcb->mcu_context[5] = (uint32_t *)sched_default_task_exit_point;
  task_tcb->mcu_context[6] = task_entry_point;
  task_tcb->mcu_context[7] = (uint32_t *)0x1000000;

  /* Stack context in interrupt */
  const int unstacked_regs = 8;   /* R4-R11 */
  int i = 0;
  void *ptr_after_int = task_tcb->stack_ptr_top -
    sizeof(void *) * MCU_CONTEXT_SIZE;

  for (uint8_t *ptr = ptr_after_int;
     ptr < (uint8_t *)task_tcb->stack_ptr_top;
     ptr += sizeof(uint32_t))
  {
    *((uint32_t *)ptr) = (uint32_t)task_tcb->mcu_context[i++];
  }

  task_tcb->sp = ptr_after_int - unstacked_regs * sizeof(void *);

  /* Insert the task in the list */

  __disable_irq();

  list_add(&task_tcb->next_tcb, &g_tcb_list);

  __enable_irq();

  return 0;
}

/**************************************************************************
* Name:
* sched_get_next_task
*
* Description:
*  Init scheduler resources. Should not be called before sched_init
*
* Return Value:
*  The TCB of the next task or NULL if the scheduler is not initialized.
*
*************************************************************************/
struct tcb_s *sched_get_next_task(void)
{
  g_current_tcb = g_current_tcb->next;
  if (g_current_tcb == &g_tcb_list)
    {
      g_current_tcb = g_current_tcb->next;
    }

  struct tcb_s *next_tcb = (struct tcb_s *)container_of(g_current_tcb,
                                                        struct tcb_s,
                                                        next_tcb);
  return next_tcb;
}

/**************************************************************************
* Name:
* sched_allocate_resource
*
* Description:
*  Allocate a new resource and store the file desscriptor in the resource
*  structure.
*
* Return Value:
*  The opened container resource or NULL in case we are running out of memory.
*
*************************************************************************/
struct opened_resource_s *sched_allocate_resource(void)
{
  disable_int();
  struct tcb_s *curr_tcb = sched_get_current_task();
  assert(curr_tcb->curr_resource_opened >= 0);

  size_t new_size = sizeof(struct opened_resource_s) *
    (curr_tcb->curr_resource_opened + 1);
  struct opened_resource_s *new_res = realloc(curr_tcb->res,
                                              new_size);
  if (!res) {
    enable_int();
    return NULL;
  }

  curr_tcb->res = new_res;
  new_res[curr_tcb->curr_resource_opened].fd = curr_tcb->curr_resource_opened;
  curr_tcb->curr_resource_opened++;

  enable_int();

  return &new_res[curr_tcb->curr_resource_opened - 1];
}

/**************************************************************************
* Name:
* sched_run
*
* Description:
*  Pick the next task to be run.
*
* Return Value:
*  The TCB of the next task or NULL if the scheduler is not initialized.
*
*************************************************************************/
void sched_run(void)
{
  sched_idle_task();
}

/**************************************************************************
* Name:
* sched_get_current_task
*
* Description:
*  Get current runing task.
*
* Return Value:
*  The TCB of the next task or NULL if the scheduler is not initialized.
*
*************************************************************************/
struct tcb_s *sched_get_current_task(void)
{
  struct tcb_s *current_tcb = (struct tcb_s *)container_of(g_current_tcb,
    struct tcb_s, next_tcb);
  return current_tcb;
}

/**************************************************************************
 * Name:
 *  sched_desroy
 *
 * Description:
 *  Init scheduler resources.
 *
 * Return Value:
 *  OK in case of success otherwise a negate value.
 *
 *************************************************************************/
int sched_desroy(void)
{
  return 0;
}

/**************************************************************************
 * Name:
 *  sched_preempt_task
 *
 * Description:
 *  Move the task from running to waiting list.
 *
 * Return Value:
 *  The task that was moved from running to ready.
 *
 *************************************************************************/
struct tcb_s *sched_preempt_task(void)
{
  struct tcb_s *tcb = sched_get_current_task();

  g_current_tcb = &sched_get_next_task()->next_tcb;

  list_del(&tcb->next_tcb);
  list_add(&tcb->next_tcb, &g_tcb_waiting_list);

  return tcb;
}

/**************************************************************************
 * Name:
 *  disable_int
 *
 * Description:
 *  Disable all interrupts.
 *
 *************************************************************************/
void disable_int(void)
{
  __disable_irq();
}

/**************************************************************************
 * Name:
 *  enable_int
 *
 * Description:
 *  Enable all interrupts.
 *
 *************************************************************************/
void enable_int(void)
{
  __enable_irq();
}
