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
#include "gpio_port.h"

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

enum pullup_mode {
  PULLUP_NOTSET = -1,
  PULLUP_NONE = 0,
  PULLUP_DOWN = 1,
  PULLUP_UP = 2
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
void gpio_pullup(struct gpio *pin, char *mode_str)
{
  int mode = get_pullup_mode(mode_str);

  if(mode == PULLUP_NOTSET)
    return;

  gpio_mem[37] = mode & 0x3;
  usleep(1);

  gpio_mem[38] = 0x1 << pin->pin_number;
  usleep(1);

  gpio_mem[37] = 0;
  gpio_mem[38] = 0;
}

