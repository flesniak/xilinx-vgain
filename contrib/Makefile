SRC = v4l2sdl.c v4l.c
OBJ = $(patsubst %.c,%.o,$(SRC))

CFLAGS = -g -I/usr/include/SDL -std=gnu99 -O2 -Wall
LDFLAGS = -lSDL -lv4l2 -lgd

.PHONY : all clean

all: v4l2sdl

v4l2sdl: $(OBJ)
	gcc -o $@ $^ $(LDFLAGS)

%.o : %.c
	gcc -c $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) v4l2sdl
