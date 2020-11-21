#include <board.h>

#include <irq_manager.h>
#include <stdint.h>
#include <scheduler.h>
#include <os_start.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_CONSOLE_NRF_INIT_SOFTDEVICE_APP
/* This symbol is defined in the soft device assembly and is the entry point
 * from soft device application. It's usefull to call this because it 
 * calls some initialization functions that help the soft device library.
 * After this we resume the execution to the _start entry point from assembly.
 */
void Reset_Handler(void);
#endif

typedef struct __attribute__((packed)) ContextStateFrame {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr;
  uint32_t return_address;
  uint32_t xpsr;
} sContextStateFrame;

__attribute__((optimize("O0")))
void prvGetRegistersFromStack(sContextStateFrame *frame)
{ 
  printf("***** Hardware exception *****\n");
  printf("R0:%x "
         "R1:%x "
         "R2:%x "
         "R3:%x "
         "R12:%x "
         "LR:%x "
         "RET_ADDR:%x "
         "XPSR:%x \n",
         frame->r0,
         frame->r1,
         frame->r2,
         frame->r3,
         frame->r12,
         frame->lr,
         frame->return_address,
         frame->xpsr);

  printf("***** Running Task *****\n");
  struct tcb_s *tcb = sched_get_current_task();

  printf("Task name: %s\n"
         "addr:%x "
         "SP:%x "
         " TOP:%x BASE:%x\n",
         tcb->task_name == NULL ? "[Noname]" : tcb->task_name,
         tcb,
         tcb->sp,
         tcb->stack_ptr_top,
         tcb->stack_ptr_base);

  __asm volatile("bkpt 1");
}

/* The fault handler implementation calls a function called
prvGetRegistersFromStack(). */
__attribute__((optimize("O0")))
static void HardFault_Handler(void)
{ 
  __asm volatile("tst lr, #4 \n"
                  "ite eq \n"
                  "mrseq r0, msp \n"
                  "mrsne r0, psp \n"
                  "b prvGetRegistersFromStack \n");
}

/* Flash based ISR vector */
__attribute__((section(".isr_vector")))
void (*g_vectors[NUM_IRQS])(void) = {
        STACK_TOP,
#ifdef CONFIG_CONSOLE_NRF_INIT_SOFTDEVICE_APP
        Reset_Handler,
#else
        __start,
#endif
        irq_generic_handler,  /* NMI handler */
        HardFault_Handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,  /* SVC */
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,  /* Pend SV */
        irq_generic_handler,  /* Systick handler */

        /* External interrupts */

        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
        irq_generic_handler,
};
