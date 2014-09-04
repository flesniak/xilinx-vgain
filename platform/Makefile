ifeq ($(origin IMPERAS_HOME), undefined)
  $(error IMPERAS_HOME is not set. Run /opt/set_ovp_env.sh)
endif
include $(IMPERAS_HOME)/bin/Makefile.include

PLATFORM_SRC = platform.c
PLATFORM_SRCINC = 
PLATFORM_CFLAGS = -std=gnu99 -g -Wall -Wextra -O0 -lCpuManager

PLATFORM_INCOBJ = $(patsubst %.c,%.o,$(PLATFORM_SRCINC))
PLATFORM_EXE = $(patsubst %.c,%,$(PLATFORM_SRC))

CROSS = OR1K
CROSS_SRC = 
CROSS_SRCINC = 
CROSS_CFLAGS = -std=gnu99 -g -Wall -Wextra -O2 -T or1k-link.lds

CROSS_INCOBJ = $(patsubst %.c,%.$(CROSS).o,$(CROSS_SRCINC))
CROSS_EXE = $(patsubst %.c,%.$(CROSS).elf,$(CROSS_SRC))

include $(IMPERAS_HOME)/lib/Linux32/CrossCompiler/$(CROSS).makefile.include

.PHONY : all

all : application platform

platform : $(PLATFORM_INCOBJ) $(PLATFORM_SRC)
	gcc $(SIM_CFLAGS) $(PLATFORM_CFLAGS) -o $(PLATFORM_EXE) $(PLATFORM_SRC) $(PLATFORM_INCOBJ) $(SIM_LDFLAGS)

$(PLATFORM_INCOBJ) : $(PLATFORM_SRCINC)
	gcc $(SIM_CFLAGS) $(PLATFORM_CFLAGS) -c -o $@ $(@:%.o=%.c)

application : $(CROSS_EXE)

$(CROSS_EXE) : $(CROSS_INCOBJ) $(CROSS_SRC)
	$(IMPERAS_LINK) $(CROSS_CFLAGS) $(IMPERAS_LDFLAGS) -o $@ $(@:%.$(CROSS).elf=%.c)

$(CROSS_INCOBJ) : $(CROSS_SRCINC)
	$(IMPERAS_CC) $(CROSS_CFLAGS) -c -o $@ $(@:%.o=%.c)

clean :
	@rm -f $(PLATFORM_EXE) $(CROSS_EXE) $(PLATFORM_INCOBJ) $(CROSS_INCOBJ)
