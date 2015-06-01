#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <SDL.h>

#include "v4l.h"

#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480
#define MMAP_BUFFERS 10

static bool scale = false;

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

void usage(char* name) {
  fprintf(stderr, "Usage: %s [/dev/videoX] [widthxheight>]\n", name);
  exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
  if( argc > 3 )
    usage(*argv);

  char* device = "/dev/video0";
  int targetWidth = DEFAULT_WIDTH, targetHeight = DEFAULT_HEIGHT;
  if( argc >= 2 )
    device = *++argv;
  if( argc >= 3 )
    sscanf(*++argv, "%dx%d", &targetWidth, &targetHeight);
  printf("Targeting %dx%d resolution\n", targetWidth, targetWidth);

  v4lT* v4l = calloc(1, sizeof(v4lT));
  if( v4lOpen(v4l, device) )
    exit(EXIT_FAILURE);

  if( v4lCheckCapabilities(v4l) )
    exit(EXIT_FAILURE);

  if( v4lGetInput(v4l) )
    exit(EXIT_FAILURE);
  printf("Current input %d: %s\n", v4l->curIn, v4l->in.name);

  if( v4lCheckFormats(v4l, V4L2_PIX_FMT_MJPEG) )
    exit(EXIT_FAILURE);
  printf("%d supported formats:\n", v4l->fmtsCount);
  for( int i = 0; i < v4l->fmtsCount; i++ )
    printf("fmt: index %d type %d flags 0x%08x desc %s pixfmt %c%c%c%c\n", v4l->fmts[i].index, v4l->fmts[i].type, v4l->fmts[i].flags, v4l->fmts[i].description, (char)v4l->fmts[i].pixelformat, (char)(v4l->fmts[i].pixelformat>>8), (char)(v4l->fmts[i].pixelformat>>16), (char)(v4l->fmts[i].pixelformat>>24));
  printf("preferred format: index %d. below framesizes and intervals are for this format.\n", v4l->preferredPixFmtIndex);

  if( v4lGetFramesizes(v4l, v4l->preferredPixFmtIndex) )
    exit(EXIT_FAILURE);
  for( int i = 0; i < v4l->frmSizeCount; i++ )
    printf("frmsize %d: %c%c%c%c type %d %dx%d\n", v4l->frmSizes[i].index, (char)v4l->frmSizes[i].pixel_format, (char)(v4l->frmSizes[i].pixel_format>>8), (char)(v4l->frmSizes[i].pixel_format>>16), (char)(v4l->frmSizes[i].pixel_format>>24), v4l->frmSizes[i].type, v4l->frmSizes[i].discrete.width, v4l->frmSizes[i].discrete.height);

  if( !v4lGetFrameintervals(v4l, v4l->preferredPixFmtIndex, v4l->frmSizes[v4l->frmSizeCount-1].discrete.width, v4l->frmSizes[v4l->frmSizeCount-1].discrete.height) ) {
    printf("frame intervals for format index %d and %dx%d:\n", v4l->preferredPixFmtIndex, v4l->frmSizes[v4l->frmSizeCount-1].discrete.width, v4l->frmSizes[v4l->frmSizeCount-1].discrete.height);
    for( int i = 0; i < v4l->frmIvalCount; i++ )
      printf("ival %d: %d/%d (%f fps)\n", v4l->frmIvals[i].index, v4l->frmIvals[i].discrete.numerator, v4l->frmIvals[i].discrete.denominator, (float)v4l->frmIvals[i].discrete.denominator/v4l->frmIvals[i].discrete.numerator);
  }

  if( v4lSetFormat(v4l, v4l->preferredPixFmtIndex, targetWidth, targetHeight) )
    exit(EXIT_FAILURE);
  printf("Format info: type %d width %d height %d fmt %c%c%c%c field 0x%x pitch %d imagesize %d colorspace 0x%x\n", v4l->fmt.type, v4l->fmt.fmt.pix.width, v4l->fmt.fmt.pix.height, (char)v4l->fmt.fmt.pix.pixelformat, (char)(v4l->fmt.fmt.pix.pixelformat>>8), (char)(v4l->fmt.fmt.pix.pixelformat>>16), (char)(v4l->fmt.fmt.pix.pixelformat>>24), v4l->fmt.fmt.pix.field, v4l->fmt.fmt.pix.bytesperline, v4l->fmt.fmt.pix.sizeimage, v4l->fmt.fmt.pix.colorspace);

  struct v4l2_fract ival = { .numerator = 1, .denominator = 24 };
  if( v4lSetCaptureParam(v4l, &ival, true) )
    exit(EXIT_FAILURE);
  printf("Streaming param: ival %d/%d hq %d\n", v4l->param.parm.capture.timeperframe.numerator, v4l->param.parm.capture.timeperframe.denominator, v4l->param.parm.capture.capturemode);

  if( v4lSetupMmap(v4l, MMAP_BUFFERS) )
    exit(EXIT_FAILURE);
  printf("Got %d mmap buffers\n", v4l->rqbuf.count);

  printf("Starting capture streaming\n");
  if( v4lStartStreaming(v4l) )
    exit(EXIT_FAILURE);

  SDL_Surface* display = initSDL(targetWidth, targetHeight);
  SDL_Rect rect = { .x = 0, .y = 0, .w = 0, .h = 0 };
  SDL_Surface* camSurf = 0;
  v4lBufT surfaceBuffer;

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

    if( !camSurf ) {
      if( scale )
        camSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, targetWidth, targetHeight, 32, 0xFF0000, 0x00FF00, 0x0000FF, 0);
      else
        camSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, buf->w, buf->h, 32, 0xFF0000, 0x00FF00, 0x0000FF, 0);
      surfaceBuffer.length = camSurf->w*camSurf->h*camSurf->format->BytesPerPixel;
      surfaceBuffer.data = camSurf->pixels;
      surfaceBuffer.w = camSurf->w;
      surfaceBuffer.h = camSurf->h;
      rect.w = surfaceBuffer.w;
      rect.h = surfaceBuffer.h;
    }

    if( scale ) {
      if( v4lDecodeImage(v4l, &surfaceBuffer, buf, rect.w, rect.h, true) )
        continue;
    } else {
      if( v4lDecodeImage(v4l, &surfaceBuffer, buf, buf->w, buf->h, false) )
        continue;
    }

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
