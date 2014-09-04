#ifndef V4L_H
#define V4L_H

#include <gd.h>
#include <linux/videodev2.h>

typedef struct v4lBufS {
  int length;
  void* data;
} v4lBufT;

typedef struct v4lS {
  int cam;
  struct v4l2_capability cap;
  int curIn;
  struct v4l2_input in;
  struct v4l2_format fmt;
  int fmtsCount;
  struct v4l2_fmtdesc* fmts;
  struct v4l2_streamparm param;
  struct v4l2_requestbuffers rqbuf;
  struct v4l2_buffer buf;
  v4lBufT* bufs;
} v4lT;

int v4lOpen(v4lT* s, char* device);
int v4lCheckCapabilities(v4lT* s);
int v4lGetInput(v4lT* s);
int v4lGetStandards(v4lT* s);
int v4lGetFormats(v4lT* s);
int v4lSetFormat(v4lT* s, int width, int height);
int v4lSetupMmap(v4lT* s, int nBuffers);
int v4lStartStreaming(v4lT* s);
v4lBufT* v4lGetImage(v4lT* s);
gdImagePtr v4lDecodeMJPEG(void* data, int len);
int v4lClose(v4lT* s);

#endif //V4L_H
