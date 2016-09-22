# Variables to override
#
# CC            C compiler
# CROSSCOMPILE	crosscompiler prefix, if any
# CFLAGS	compiler flags for compiling all C files
# ERL_CFLAGS	additional compiler flags for files using Erlang header files
# ERL_EI_LIBDIR path to libei.a
# LDFLAGS	linker flags for linking all binaries
# ERL_LDFLAGS	additional linker flags for projects referencing Erlang libraries

# Check that we're on a supported build platform
ifeq ($(CROSSCOMPILE),)
    # Not crosscompiling, so check that we're on Linux.
    ifneq ($(shell uname -s),Linux)
        $(error Elixir GPIO_RPI only works on Linux. Crosscompiling is possible if $$CROSSCOMPILE is set.)
    endif
endif

# Look for the EI library and header files
ERL_EI_INCLUDE_DIR ?= $(shell find /usr/local/lib/erlang /usr/lib/erlang ~/.asdf/installs/erlang/19.0/lib/erlang/usr/include/ -name ei.h -printf '%h\n' 2> /dev/null | head -1)
ERL_EI_LIBDIR ?= $(shell find /usr/local/lib/erlang /usr/lib/erlang ~/.asdf/installs/erlang/19.0/lib/erlang/usr/lib -name libei.a -printf '%h\n' 2> /dev/null | head -1)

ifeq ($(ERL_EI_INCLUDE_DIR),)
   $(error Could not find include directory for ei.h. Check that Erlang header files are available)
endif
ifeq ($(ERL_EI_LIBDIR),)
   $(error Could not find libei.a. Check your Erlang installation)
endif

# Set Erlang-specific compile and linker flags
ERL_CFLAGS ?= -I$(ERL_EI_INCLUDE_DIR)
ERL_LDFLAGS ?= -L$(ERL_EI_LIBDIR) -lei

LDFLAGS +=
CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter
CC ?= $(CROSSCOMPILER)gcc

.PHONY: all clean

all: priv/gpio_rpi

%.o: %.c
	$(CC) -c $(ERL_CFLAGS) $(CFLAGS) -o $@ $<

priv/gpio_rpi: src/erlcmd.o src/gpio_port.o src/gpio_port_rpi.o
	@mkdir -p priv
	$(CC) $^ $(ERL_LDFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f priv/gpio_rpi src/*.o
