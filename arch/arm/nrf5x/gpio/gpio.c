#include <stdint.h>

#include <gpio.h>

#define GPIO_BASE_PORT0   0x50000000
#define GPIO_BASE_PORT1   0x50000300

#define GPIO_OUTSET(BASE) (*((volatile uint32_t*) ((BASE) + 0x508)))
#define GPIO_OUTCLR(BASE) (*((volatile uint32_t*) ((BASE) + 0x50C)))
#define GPIO_IN(BASE)     (*((volatile uint32_t*) ((BASE) + 0x510)))
#define GPIO_DIR(BASE)    (*((volatile uint32_t*) ((BASE) + 0x514)))
#define GPIO_PIN_CNF(BASE, pin) (*((volatile uint32_t*) ((BASE) + 0x700 + (pin) * 4)))

#define GPIO_PULLUP_VALUE   (3)
#define GPIO_PULLUP_OFFSET  (2)

static uint8_t *g_base_addr[] =
{
  (uint8_t *)GPIO_BASE_PORT0,
  (uint8_t *)GPIO_BASE_PORT1,
};

void gpio_init(void)
{

}

void gpio_configure(int pin, int port, gpio_direction_t cfg, gpio_pin_input_t input,
                    gpio_pull_cfg_t pull_cfg, gpio_drive_cfg_t drive_cfg, gpio_pin_sense_t sense_input)
{
  GPIO_DIR(GPIO_BASE_PORT0) = (cfg << pin) | (~(1 << pin) & GPIO_DIR(GPIO_BASE_PORT0));

  if (cfg == GPIO_DIRECTION_IN)
  {
    GPIO_PIN_CNF(g_base_addr[port], pin) = GPIO_DIRECTION_IN | (input << 1) | (pull_cfg << GPIO_PULLUP_OFFSET) | (drive_cfg << 8) | (sense_input << 16);
  }
  else
  {
    GPIO_PIN_CNF(g_base_addr[port], pin) = GPIO_DIRECTION_OUT | (input << 1) | (pull_cfg << GPIO_PULLUP_OFFSET) | (drive_cfg << 8) | (sense_input << 16);
  }
}

int gpio_read(int pin, int port)
{
  if (port < 0 || port >= 2)
  {
    return -1;
  }

  return ((GPIO_IN(g_base_addr[port]) & (1 << pin)) >> pin);
}

void gpio_toogle(int enable, int pin, int port)
{
  if (port < 0 || port >= 2)
  {
    return;
  }

  if (enable)
  {
    GPIO_OUTSET(g_base_addr[port]) = (1 << pin);
  }
  else
  {
    GPIO_OUTCLR(g_base_addr[port]) = (1 << pin);
  }
}
