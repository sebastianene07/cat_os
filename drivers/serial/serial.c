#include <board.h>

#include <semaphore.h>
#include <serial.h>
#include <string.h>
#include <scheduler.h>
#include <stdlib.h>
#include <vfs.h>
#include <errno.h>

/****************************************************************************
 * Private Function Definitions
 ****************************************************************************/

static int uart_open(void *priv, const char *pathname, int flags, mode_t mode);
static int uart_close(void *priv);
static int uart_write(void *priv, const void *buf, size_t count);
static int uart_read(void *priv, void *buf, size_t count);

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct vfs_ops_s g_uart_ops = {
  .open  = uart_open,
  .close = uart_close,
  .write = uart_write,
  .read  = uart_read,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int uart_open(void *priv, const char *pathname, int flags, mode_t mode)
{
  struct uart_upper_s *uart_upper = (struct uart_upper_s *)priv;

  /* Call into the lowerhalf open method */

  if (uart_upper->lower->open_cb == NULL) {
    return -ENODEV;
  }

  int ret = uart_upper->lower->open_cb(uart_upper->lower);
  if (ret != OK) {
    return ret;
  }

  /* Grab an entry from the tcb FILE structure. This should be wrapped inside
   * generic open call. */

  struct opened_resource_s *res =
    sched_allocate_resource(priv, &g_uart_ops, mode);
  if (res == NULL) {
    return -ENFILE;
  }

  return res->fd;
}

static int uart_close(void *priv)
{
  return 0;
}

static int uart_write(void *priv, const void *buf, size_t count)
{
  struct uart_upper_s *uart_upper = (struct uart_upper_s *)priv;

  /* Call into the lowerhalf open method */

  if (uart_upper->lower->write_cb == NULL) {
    return -ENODEV;
  }

  int ret = uart_upper->lower->write_cb(uart_upper->lower, buf, count);
  if (ret != OK) {
    return ret;
  }
  return 0;
}

static int uart_read(void *priv, void *buf, size_t count)
{
  struct uart_upper_s *uart_up = (struct uart_upper_s *)priv;

  /* Blocking read. Wait until 'count' bytes are available from
   * this device.
   */
  struct uart_lower_s *lower = uart_up->lower;

  do {
    uint8_t available_rx_bytes = 0;

    if (lower->index_read_rx_buffer < lower->index_write_rx_buffer)
    {
      available_rx_bytes = lower->index_write_rx_buffer -
        lower->index_read_rx_buffer;
    }
    else if (lower->index_read_rx_buffer != lower->index_write_rx_buffer)
    {
      available_rx_bytes = UART_RX_BUFFER -
      (lower->index_read_rx_buffer - lower->index_write_rx_buffer);
    }

    uint8_t min_copy = count > available_rx_bytes ? available_rx_bytes : count;
    if (available_rx_bytes > 0)
    {
      memcpy(buf, lower->rx_buffer, min_copy);

      lower->index_read_rx_buffer += min_copy;
      lower->index_read_rx_buffer = lower->index_read_rx_buffer % UART_RX_BUFFER;
      return min_copy;
    }
    else
    {
      sem_wait(&lower->rx_notify);
    }
  } while (1);

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int uart_register(const char *name, const struct uart_lower_s *uart_lowerhalf)
{
  /* Called from lowerhalf driver. Allocate a new upper driver instance where
   * we store the lowerhalf. struct uart_upper_s
   */

  struct uart_upper_s *uart_upper = calloc(sizeof(struct uart_upper_s), 1);
  if (uart_upper == NULL) {
    return -ENOMEM;
  }

  uart_upper->lower = uart_lowerhalf;

  /* Register the upper half node with the VFS */

  int ret = vfs_register_node(name, strlen(name), &g_uart_ops, VFS_TYPE_DEVICE,
    uart_upper);
  if (ret != OK) {
    free(uart_upper);
  }

  return ret;
}
