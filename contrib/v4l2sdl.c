#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <SDL.h>

#include "v4l.h"

#define SCALE 1
#define OUTPUT_WIDTH  640
#define OUTPUT_HEIGHT 480

SDL_Surface* initSDL(int w, int h) {
  if( SDL_Init(SDL_INIT_VIDEO) ) {
    printf("Couldn't initialize SDL: %s\n", SDL_GetError());
    return 0;
  }

  SDL_Surface* display = SDL_SetVideoMode(w, h, 0, SDL_SWSURFACE);
  if( !display ) {
    printf("Couldn't initialize surface: %s\n", SDL_GetError());
    return 0;
  }

  SDL_FillRect(display, 0, SDL_MapRGB(display->format, 0, 0, 0));
  SDL_UpdateRect(display, 0, 0, 0, 0);

  return display;
}

int main(int argc, char** argv) {
  if( argc > 3 ) {
    fprintf(stderr, "Usage: %s [/dev/videoX]\n", *argv);
    exit(EXIT_FAILURE);
  }

  char* device = "/dev/video0";
  if( argc == 2 )
    device = *++argv;

  v4lT* v4l = calloc(1, sizeof(v4lT));
  if( v4lOpen(v4l, device) )
    exit(EXIT_FAILURE);

  if( v4lCheckCapabilities(v4l) )
    exit(EXIT_FAILURE);

  if( v4lGetInput(v4l) )
    exit(EXIT_FAILURE);
  printf("Current input %d: %s\n", v4l->curIn, v4l->in.name);

  if( v4lGetFormats(v4l) )
    exit(EXIT_FAILURE);
  printf("%d supported formats:\n", v4l->fmtsCount);
  for( int i = 0; i < v4l->fmtsCount; i++ )
    printf("fmt: index %d type %d flags 0x%08x desc %s pixfmt %c%c%c%c\n", v4l->fmts[i].index, v4l->fmts[i].type, v4l->fmts[i].flags, v4l->fmts[i].description, (char)v4l->fmts[i].pixelformat, (char)(v4l->fmts[i].pixelformat>>8), (char)(v4l->fmts[i].pixelformat>>16), (char)(v4l->fmts[i].pixelformat>>24));

  if( v4lSetFormat(v4l, OUTPUT_WIDTH, OUTPUT_HEIGHT) ) //request best resolution
    exit(EXIT_FAILURE);
  printf("Format info: type %d width %d height %d fmt %c%c%c%c field 0x%x pitch %d imagesize %d colorspace 0x%x\n", v4l->fmt.type, v4l->fmt.fmt.pix.width, v4l->fmt.fmt.pix.height, (char)v4l->fmt.fmt.pix.pixelformat, (char)(v4l->fmt.fmt.pix.pixelformat>>8), (char)(v4l->fmt.fmt.pix.pixelformat>>16), (char)(v4l->fmt.fmt.pix.pixelformat>>24), v4l->fmt.fmt.pix.field, v4l->fmt.fmt.pix.bytesperline, v4l->fmt.fmt.pix.sizeimage, v4l->fmt.fmt.pix.colorspace);

  if( v4lSetupMmap(v4l, 5) )
    exit(EXIT_FAILURE);
  printf("Got %d mmap buffers\n", v4l->rqbuf.count);

  printf("Starting capture streaming\n");
  if( v4lStartStreaming(v4l) )
    exit(EXIT_FAILURE);

  SDL_Surface* display = initSDL(OUTPUT_WIDTH, OUTPUT_HEIGHT);
  SDL_Rect rect = { .x = 0, .y = 0, .w = 0, .h = 0 };
  SDL_Surface* camSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, OUTPUT_WIDTH, OUTPUT_HEIGHT, 32, 0xFF0000, 0x00FF00, 0x0000FF, 0);

  if( !display )
    exit(EXIT_FAILURE);

  SDL_Event event;
  bool run = true;
  time_t startTime = time(0);
  time_t currentTime = startTime;
  time_t newTime = startTime;
  unsigned int frames = 0;
  while( run ) {
    while( SDL_PollEvent(&event) ) {
      switch( event.type ) {
        case SDL_QUIT:
          printf("Quit.\n");
          run = false;
          break;
        /*default:
          printf("Unhandled SDL_Event\n");*/
      }
    }

    v4lBufT* buf = v4lGetImage(v4l);
    if( !buf )
      break;

    gdImagePtr img = v4lDecodeMJPEG(buf->data, buf->length);
    if( !img || !gdImageTrueColor(img) ) {
      fprintf(stderr, "Could not read image, conversion error?\n");
      continue;
    }

    rect.w = gdImageSX(img);
    rect.h = gdImageSY(img);

    if( SCALE && (rect.w != OUTPUT_WIDTH || rect.h != OUTPUT_HEIGHT) ) {
      gdImagePtr src = img;
      img = gdImageCreateTrueColor(OUTPUT_WIDTH, OUTPUT_HEIGHT);
      gdImageCopyResized(img, src, rect.x, rect.y, rect.x, rect.y, OUTPUT_WIDTH, OUTPUT_HEIGHT, rect.w, rect.h);
      gdImageDestroy(src);
      rect.w = OUTPUT_WIDTH;
      rect.h = OUTPUT_HEIGHT;
    }

    for( int y = rect.y; y < rect.h; y++ )
      for( int x = rect.x; x < rect.w; x++ )
        *(((uint32_t*)camSurf->pixels)+y*camSurf->pitch/4+x) = gdImageGetPixel(img, x, y);
    gdImageDestroy(img);

    SDL_BlitSurface(camSurf, &rect, display, 0);
    SDL_UpdateRect(display, rect.x, rect.y, rect.w, rect.h);

    frames++;
    newTime = time(0);
    if( currentTime != newTime ) {
      float fps = (float)frames/(newTime-startTime);
      printf("\rfps: %#6.2f", fps);
      fflush(stdout);
      currentTime = newTime;
    }
  }

  v4lClose(v4l);
  if( camSurf )
    SDL_FreeSurface(camSurf);
  SDL_FreeSurface(display);
  SDL_Quit();
  exit(EXIT_SUCCESS);
}
