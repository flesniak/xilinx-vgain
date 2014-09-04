IMPERAS_HOME := $(shell getpath.exe "$(IMPERAS_HOME)")
CFLAGS += -I/usr/include/SDL -std=gnu99
LDFLAGS = -pthread -lgd -lv4l2
include $(IMPERAS_HOME)/ImperasLib/buildutils/Makefile.host
