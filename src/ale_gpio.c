/*
 * Copyright 2016 Herman verschooten
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * GPIO port implementation.
 *
 * This code is based upon the works of WiringPi.
 * see http://wiringpi.com
 * And elixir_ale by Frank Hunleth
 * see https://github.com/fhunleth/elixir_ale
 */

#include <err.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "erlcmd.h"

//#define DEBUG
#ifdef DEBUG
#define debug(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\r\n"); } while(0)
#else
#define debug(...)
#endif

/* PERI_BASE for early A/B models is 0x20000000, for all others it is 0x3F000000 */
#define PERI_BASE_1   0x20000000
#define PERI_BASE_2   0x3F000000
#define GPIO_BASE     0x200000
#define BLOCK_SIZE    4096
#define BOARD_TYPE_1  1
#define BOARD_TYPE_2  2

static volatile unsigned int RASPBERRY_PI_PERI_BASE;

static volatile unsigned int *gpio_mem;

enum gpio_state {
  GPIO_OUTPUT,
  GPIO_INPUT,
  GPIO_INPUT_WITH_INTERRUPTS
};

enum pullup_mode {
  PULLUP_NOTSET = -1,
  PULLUP_NONE = 0,
  PULLUP_DOWN = 1,
  PULLUP_UP = 2
};

struct gpio {
  enum gpio_state state;
  int fd;
  int pin_number;
};

int get_rpi_type(void)
{
  FILE *fd;
  char line[120];
  int rev=0;

  if((fd = fopen("/proc/cpuinfo", "r")) == NULL)
    errx(EXIT_FAILURE, "Unable to read /proc/cpuinfo");

  while(fgets(line, 120, fd) != NULL)
    if(strncmp(line, "Hardware", 8) == 0)
      break;

  if(strncmp(line, "Hardware", 8) != 0)
    errx(EXIT_FAILURE, "No Hardware line");

  if(strstr(line, "BCM2709") != NULL)
  {
    fclose(fd);
    debug("Returning board type 2\n");
    return BOARD_TYPE_2;
  }

  debug("Hardware not conclusive\n%s\nChecking revision\n", line);

  rewind(fd);

  while(fgets(line, 120, fd) != NULL)
    if(strncmp(line, "Revision", 8) == 0)
      break;

  if(strncmp(line, "Revision", 8) != 0)
    errx(EXIT_FAILURE, "No Revision line");

  fclose(fd);

  debug("%s", line);

  if(scanf(line, "Revision\t: %x\n", rev) < 0)
    errx(EXIT_FAILURE, "Error reading revision");

  rev &= 0xffff;
  if((rev== 0x0002) || (rev == 0x0003))
    return BOARD_TYPE_1;

  return BOARD_TYPE_2;
}

void init_gpio_mem(void)
{
  int fd;
  void *gpio_map;
  int gpio_base;

  if(get_rpi_type() == BOARD_TYPE_1)
    RASPBERRY_PI_PERI_BASE = PERI_BASE_1;
  else
    RASPBERRY_PI_PERI_BASE = PERI_BASE_2;

  gpio_base = RASPBERRY_PI_PERI_BASE + GPIO_BASE;

  fd = open("/dev/mem", O_RDWR | O_SYNC);
  if(fd < 0) {
    errx(EXIT_FAILURE, "error: unable to open /dev/mem");
    exit(-1);
  }

  gpio_map = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpio_base);

  if(gpio_map == (void *)-1)
    errx(EXIT_FAILURE, "Cannot open /dev/mem on the memory!\n");

  close(fd);

  gpio_mem = (unsigned int *) gpio_map;
}

/**
 * @brief write a string to a sysfs file
 * @return returns 0 on failure, >0 on success
 */
int sysfs_write_file(const char *pathname, const char *value)
{
  int fd = open(pathname, O_WRONLY);
  if (fd < 0) {
    debug("Error opening %s", pathname);
    return 0;
  }

  size_t count = strlen(value);
  ssize_t written = write(fd, value, count);
  close(fd);

  if (written < 0 || (size_t) written != count) {
    debug("Error writing '%s' to %s", value, pathname);
    return 0;
  }

  return written;
}

int export_pin(struct gpio *pin, unsigned int pin_number)
{
  char value_path[64];
  sprintf(value_path, "/sys/class/hpio/gpio%d/value", pin_number);

  // Check to see if pin has already been exported?
  if(access(value_path, F_OK) == -1) {
    // Nope! Export it!
    char pinstr[64];
    sprintf(pinstr, "%d", pin_number);
    if(!sysfs_write_file("/sys/class/gpio/export", pinstr))
      return -1;
  }

  // Open the value file for quick access later
  int fd = open(value_path, pin->state == GPIO_OUTPUT ? O_RDWR : O_RDONLY);

  return fd;
}

int create_direction_path(unsigned int pin_number, enum gpio_state dir)
{
  // Construct the gpio control file paths
  char direction_path[64];
  sprintf(direction_path, "/sys/class/gpio/gpio%d/direction", pin_number);

  /* The direction file may not exist if the pin only works one way.
   * It is ok if it doesn't exists, but if it does, it must be writeable.
   */
  if(access(direction_path, F_OK) != -1) {
    const char *dir_string = (dir == GPIO_OUTPUT ? "out" : "in");

    // Retry writing the direction to cope with a race condition on the PI.

    int retries = 1000;

    while(!sysfs_write_file(direction_path, dir_string) &&
        retries > 0) {
      usleep(1000);
      retries--;
    }
    if(retries == 0)
      return -1;

    return 0;
  }
  return 0;
}

int get_pullup_mode(char *mode)
{
  if(strcmp(mode, "none") == 0)
    return PULLUP_NONE;
  if(strcmp(mode, "down") == 0)
    return PULLUP_DOWN;
  if(strcmp(mode, "up") == 0)
    return PULLUP_UP;
  return PULLUP_NOTSET;
}

// GPIO Functions

/*
 * @brief     Change state of pullup register for pin
 *
 * @param     pin     The pin structure
 * @param     mode    The new mode
 *
 * @return    1 for sucess
 */
void gpio_pullup(struct gpio *pin, int mode)
{
  if(mode == PULLUP_NOTSET)
    return;

  gpio_mem[37] = mode & 0x3;
  usleep(1);

  gpio_mem[38] = 0x1 << pin->pin_number;
  usleep(1);

  gpio_mem[37] = 0;
  gpio_mem[38] = 0;
}

/**
 * @brief     Open and configure a GPIO
 *
 * @param     pin         The pin structure
 * @param     pin_number  The GPIO pin
 * @param     dir         The direction of pin {input or output}
 * *param     pullup_mode The mode for the pullup register { no_change, none, off, up }
 *
 * @return    1 for success, -1 for failure
 */
int gpio_init(struct gpio *pin, unsigned int pin_number, enum gpio_state dir, int mode)
{
  // Initialize the pin structure
  pin->state = dir;
  pin->fd = -1;
  pin->pin_number = pin_number;

  // Set sysfs files for this pin
  if((pin->fd = export_pin(pin, pin_number)) == -1)
    return -1;
  if(create_direction_path(pin_number, dir) == -1)
    return -1;

  if(mode != PULLUP_NOTSET)
    gpio_pullup(pin, mode);

  return 1;
}
/**
 * @brief  Set pin with the value "0" or "1"
 *
 * @param  pin           The pin structure
 * @param       value         Value to set (0 or 1)
 *
 * @return   1 for success, -1 for failure
 */
int gpio_write(struct gpio *pin, unsigned int val)
{
  if (pin->state != GPIO_OUTPUT)
    return -1;

  char buf = val ? '1' : '0';
  ssize_t amount_written = pwrite(pin->fd, &buf, sizeof(buf), 0);
  if (amount_written < (ssize_t) sizeof(buf))
    err(EXIT_FAILURE, "pwrite");

  return 1;
}

/**
 * @brief  Read the value of the pin
 *
 * @param  pin            The GPIO pin
 *
 * @return   The pin value if success, -1 for failure
 */
int gpio_read(struct gpio *pin)
{
  char buf;
  ssize_t amount_read = pread(pin->fd, &buf, sizeof(buf), 0);
  if (amount_read < (ssize_t) sizeof(buf))
    err(EXIT_FAILURE, "pread");

  return buf == '1' ? 1 : 0;
}

/**
 * Set isr as the interrupt service routine (ISR) for the pin. Mode
 * should be one of the strings "rising", "falling" or "both" to
 * indicate which edge(s) the ISR is to be triggered on. The function
 * isr is called whenever the edge specified occurs, receiving as
 * argument the number of the pin which triggered the interrupt.
 *
 * @param   pin  Pin number to attach interrupt to
 * @param   mode  Interrupt mode
 *
 * @return  Returns 1 on success.
 */
int gpio_set_int(struct gpio *pin, const char *mode)
{
  char path[64];
  sprintf(path, "/sys/class/gpio/gpio%d/edge", pin->pin_number);
  if (!sysfs_write_file(path, mode))
    return -1;

  if (strcmp(mode, "none") == 0)
    pin->state = GPIO_INPUT;
  else
    pin->state = GPIO_INPUT_WITH_INTERRUPTS;

  return 1;
}

/**
 * Called after poll() returns when the GPIO sysfs file indicates
 * a status change.
 *
 * @param pin which pin to check
 */
void gpio_process(struct gpio *pin)
{
  int value = gpio_read(pin);

  char resp[256];
  int resp_index = sizeof(uint16_t); // Space for payload size
  resp[resp_index++] = 'n'; // Notification
  ei_encode_version(resp, &resp_index);
  ei_encode_tuple_header(resp, &resp_index, 2);
  ei_encode_atom(resp, &resp_index, "gpio_interrupt");
  ei_encode_atom(resp, &resp_index, value ? "rising" : "falling");
  erlcmd_send(resp, resp_index);
}

void gpio_handle_request(const char *req, void *cookie)
{
  struct gpio *pin = (struct gpio *) cookie;

  // Commands are of the form {Command, Arguments}:
  // { atom(), term() }
  int req_index = sizeof(uint16_t);
  if (ei_decode_version(req, &req_index, NULL) < 0)
    errx(EXIT_FAILURE, "Message version issue?");

  int arity;
  if (ei_decode_tuple_header(req, &req_index, &arity) < 0 ||
      arity != 2)
    errx(EXIT_FAILURE, "expecting {cmd, args} tuple");

  char cmd[MAXATOMLEN];
  if (ei_decode_atom(req, &req_index, cmd) < 0)
    errx(EXIT_FAILURE, "expecting command atom");

  char resp[256];
  int resp_index = sizeof(uint16_t); // Space for payload size
  resp[resp_index++] = 'r'; // Response
  ei_encode_version(resp, &resp_index);
  if (strcmp(cmd, "read") == 0) {
    debug("read");
    int value = gpio_read(pin);
    if (value !=-1)
      ei_encode_long(resp, &resp_index, value);
    else {
      ei_encode_tuple_header(resp, &resp_index, 2);
      ei_encode_atom(resp, &resp_index, "error");
      ei_encode_atom(resp, &resp_index, "gpio_read_failed");
    }
  } else if (strcmp(cmd, "write") == 0) {
    long value;
    if (ei_decode_long(req, &req_index, &value) < 0)
      errx(EXIT_FAILURE, "write: didn't get value to write");
    debug("write %d", value);
    if (gpio_write(pin, value))
      ei_encode_atom(resp, &resp_index, "ok");
    else {
      ei_encode_tuple_header(resp, &resp_index, 2);
      ei_encode_atom(resp, &resp_index, "error");
      ei_encode_atom(resp, &resp_index, "gpio_write_failed");
    }
  } else if (strcmp(cmd, "set_int") == 0) {
    char mode[32];
    if (ei_decode_atom(req, &req_index, mode) < 0)
      errx(EXIT_FAILURE, "set_int: didn't get value");
    debug("write %s", mode);

    if (gpio_set_int(pin, mode))
      ei_encode_atom(resp, &resp_index, "ok");
    else {
      ei_encode_tuple_header(resp, &resp_index, 2);
      ei_encode_atom(resp, &resp_index, "error");
      ei_encode_atom(resp, &resp_index, "gpio_set_int_failed");
    }
  } else if (strcmp(cmd, "set_mode") == 0) {
    char mode[32];
    if(ei_decode_atom(req, &req_index, mode) < 0)
      errx(EXIT_FAILURE, "set_mode: didn't get value");
    debug("set_mode %s", mode);

    gpio_pullup(pin, get_pullup_mode(mode));
    ei_encode_atom(resp, &resp_index, "ok");
  } else
    errx(EXIT_FAILURE, "unknown command: %s", cmd);

  debug("sending response: %d bytes", resp_index);
  erlcmd_send(resp, resp_index);
}

int main(int argc, char *argv[])
{
  if((argc < 3) || (argc > 4))
    errx(EXIT_FAILURE, "%s <pin#> <input|output> [none|down|up]", argv[0]);

  int pin_number = strtol(argv[1], NULL, 0);
  enum gpio_state initial_state;
  int mode = PULLUP_NOTSET;

  if(strcmp(argv[2], "input") == 0)
    initial_state = GPIO_INPUT;
  else if(strcmp(argv[2], "output") == 0 )
    initial_state = GPIO_OUTPUT;
  else
    errx(EXIT_FAILURE, "Specify 'input' or 'output'");

  if(argc == 4) {
    if(strcmp(argv[3], "none") == 0)
      mode = PULLUP_NONE;
    else if(strcmp(argv[3], "down") == 0 )
      mode = PULLUP_DOWN;
    else if(strcmp(argv[3], "up") == 0 )
      mode = PULLUP_UP;
    else
      errx(EXIT_FAILURE, "Specify 'none', 'down' or 'up'");
  }

  struct gpio pin;
  if(gpio_init(&pin, pin_number, initial_state, mode) < 0)
    errx(EXIT_FAILURE, "Error initialize GPIO %d as %s", pin_number, argv[2]);

  struct erlcmd handler;
  erlcmd_init(&handler, gpio_handle_request, &pin);

  for(;;) {
    struct pollfd fdset[2];

    fdset[0].fd = STDIN_FILENO;
    fdset[0].events = POLLIN;
    fdset[0].revents = 0;

    fdset[1].fd = pin.fd;
    fdset[1].events = POLLPRI;
    fdset[1].revents = 0;

    /* Allways fill out the fdset structure, but only have poll() monitor
     * the sysfs file if interrupts are enabled.
     */

    int rc = poll(fdset, pin.state == GPIO_INPUT_WITH_INTERRUPTS ? 2 : 1, -1);
    if(rc < 0) {
      // Retry if EINTR
      if(errno == EINTR)
        continue;

      errx(EXIT_FAILURE, "poll");
    }

    if(fdset[0].revents & (POLLIN | POLLHUP))
      erlcmd_process(&handler);

    if(fdset[1].revents & POLLPRI)
      gpio_process(&pin);
  }

  return EXIT_SUCCESS;
}
