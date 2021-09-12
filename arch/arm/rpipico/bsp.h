#ifndef __ARCH_ARM_RPIPICO_BSP_H
#define __ARCH_ARM_RPIPICO_BSP_H

#define REG_R0                (0)
#define REG_R1                (1)
#define REG_R2                (2)
#define REG_R3                (3)
#define REG_R4                (4)
#define REG_R5                (5)
#define REG_R6                (6)
#define REG_R7                (7)
#define REG_R8                (8)
#define REG_R9                (9)

#define REG_R10               (10)
#define REG_R11               (11)
#define REG_R12               (12)
#define REG_SP                (13)
#define REG_LR                (14)
#define REG_PC                (15)
#define REG_XPSR              (16)

#define REG_NUMS              (17)

/****************************************************************************
 * UART bsp functions
 ****************************************************************************/

void bsp_serial_console_init(void);

void bsp_serial_console_putc(int c);

/****************************************************************************
 * GPIO bsp functions
 ****************************************************************************/

void bsp_gpio_led_init();

#endif /* __ARCH_ARM_RPIPICO_BSP_H */
