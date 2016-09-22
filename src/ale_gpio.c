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

/* PERI_BASE for early A/B models is 0x20000000, for all others it is 0x3F000000
#define PERI_BASE 0x20000000
*/
#define PERI_BASE 0x3F000000
#define GPIO_BASE (PERI_BASE + 0x200000)
#define BLOCK_SIZE 4096

static volatile unsigned int *gpio;

enum gpio_pull_state {
  GPIO_PULL_OFF  = 0x0,
  GPIO_PULL_DOWN = 0x1,
  GPIO_PULL_UP   = 0x2
};

void
gpio_init()
{
  int fd;
  void *gpio_map;

  fd = open("/dev/mem", O_RDWR | O_SYNC);
  if(fd < 0) {
    errx(EXIT_FAILURE, "error: unable to open /dev/mem");
    exit(-1);
  }

  gpio_map = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_BASE);

  if((int) gpio_map == -1)
    errx(EXIT_FAILURE, "Cannot open /dev/mem on the memory!\n");

  close(fd);

  gpio = (unsigned int *) gpio_map;
}

void
gpio_pullup(int pin, int mode)
{
  gpio[37] = mode & 0x3;
  usleep(1);

  gpio[38] = 0x1 << pin;
  usleep(1);

  gpio[37] = 0;
  gpio[38] = 0;
}

int
main(int argc, char *argv[])
{
  if(argc < 3)
    errx(EXIT_FAILURE, "%s <pin#> <up|down|off>", argv[0]);

  int pin_number = strtol(argv[1], NULL, 0);

  enum gpio_pull_state new_state = GPIO_PULL_OFF;

  if(strcmp(argv[2], "up") == 0)
    new_state = GPIO_PULL_UP;
  else if(strcmp(argv[2], "down") == 0 )
    new_state = GPIO_PULL_DOWN;
  else if(strcmp(argv[2], "off") == 0 )
    new_state =GPIO_PULL_OFF;
  else
    errx(EXIT_FAILURE, "Specify 'up', 'down' or 'off'");

  gpio_init();

  gpio_pullup(pin_number, new_state);


  return EXIT_SUCCESS;
}
