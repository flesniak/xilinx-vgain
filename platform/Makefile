ifeq ($(origin IMPERAS_HOME), undefined)
  $(error IMPERAS_HOME is not set. Run /opt/set_ovp_env.sh)
endif
include $(IMPERAS_HOME)/bin/Makefile.include

PLATFORM_SRC = platform.c
PLATFORM_SRCINC = 
PLATFORM_CFLAGS = -std=gnu99 -g -Wall -Wextra -O3 -lRuntimeLoader

PLATFORM_INCOBJ = $(patsubst %.c,%.o,$(PLATFORM_SRCINC))
PLATFORM_EXE = $(patsubst %.c,%,$(PLATFORM_SRC))

.PHONY : all

all : platform

platform : $(PLATFORM_INCOBJ) $(PLATFORM_SRC)
	gcc $(SIM_CFLAGS) $(PLATFORM_CFLAGS) -o $(PLATFORM_EXE) $(PLATFORM_SRC) $(PLATFORM_INCOBJ) $(SIM_LDFLAGS)

$(PLATFORM_INCOBJ) : $(PLATFORM_SRCINC)
	gcc $(SIM_CFLAGS) $(PLATFORM_CFLAGS) -c -o $@ $(@:%.o=%.c)

clean :
	@rm -f $(PLATFORM_EXE) $(PLATFORM_INCOBJ)
