#ifndef V4L_H
#define V4L_H

#include <stdbool.h>
#include <stdint.h>

#include <gd.h>
#include <linux/videodev2.h>

typedef struct v4lBufS {
  int length;
  void* data;
  int w;
  int h;
} v4lBufT;

typedef struct v4lS {
  int cam;

  struct v4l2_capability cap;
  struct v4l2_streamparm param;

  int curIn;
  struct v4l2_input in;
  struct v4l2_format fmt;

  int fmtsCount;
  struct v4l2_fmtdesc* fmts;
  int preferredPixFmtIndex;

  int frmSizeCount;
  struct v4l2_frmsizeenum* frmSizes;

  int frmIvalCount;
  struct v4l2_frmivalenum* frmIvals;

  struct v4l2_requestbuffers rqbuf;
  struct v4l2_buffer buf;
  v4lBufT* bufs;
} v4lT;

// All functions returning int return 0 on success
// and != 0 on failure (typically 1)

// Try to open the video device (e.g. /dev/vide0)
int v4lOpen(v4lT* s, char* device);

// Query the capabilities of the opened video device
// returns 1 if the device has insufficient capabilities
// to use it with this interface (i.e. it misses CAPTURE
// and STREAMING capabilities)
// Feel free to inspect the capabilities on your own after
// calling this function using s->cap.
int v4lCheckCapabilities(v4lT* s);

// Get the current input. Will be stored in s->in.
int v4lGetInput(v4lT* s);

// Get all supported standards (DEPRECATED and UNTESTED)
// Usually not needed with webcams.
int v4lGetStandards(v4lT* s);

// Query and check supported image formats. Will return 1
// on query failure or if MJPEG or YUYV image formats are
// not supported. You may specify one of V4L2_PIX_FMT_MJPEG
// or V4L2_PIX_FMT_YUYV as preferredFormat if needed, or 0.
// Feel free to inspect s->fmts/s->fmtsCount on your own.
int v4lCheckFormats(v4lT* s, uint32_t preferredFormat);

// Get supported frame sizes with format s->fmts[formatIndex]
// You need to use v4lCheckFormats prior to calling this function.
int v4lGetFramesizes(v4lT* s, int formatIndex);

// Get supported frame intervals (1/fps) for s->fmts[formatIndex]
// and a frame size of widthxheight.
// You need to use v4lCheckFormats before calling this function.
int v4lGetFrameintervals(v4lT* s, int formatIndex, int width, int height);

// Try to set video format s->fmts[formatIndex] with a resolution
// of widthxheight. Inspect s->fmt afterwards, details may change
// to what was requested!
int v4lSetFormat(v4lT* s, int formatIndex, int width, int height);

// Set the frame interval and high quality mode.
// If setting highQuality fails, it will be retried without it.
int v4lSetCaptureParam(v4lT* s, struct v4l2_fract* frameInterval, bool highQuality);

// Allocate nBuffers buffers and setup memory mapping for them
// Buffers are kept in s->bufs.
int v4lSetupMmap(v4lT* s, int nBuffers);

// Start video streaming. Call this when you are ready to process images.
int v4lStartStreaming(v4lT* s);

// Get the oldest filled buffers.
// Returns 0 on failure or the dequeued buffer on success
v4lBufT* v4lGetImage(v4lT* s);

// Decode a image buffer (encoded) to another (decoded) buffer.
// Handles YUYV and MJPEG formats (scaling is not supported for YUYV)
// If w and h are greater than 0, the image will be padded with black space
// or scaled to wxh, depending on the value of scale.
int v4lDecodeImage(v4lT* s, v4lBufT* decoded, v4lBufT* encoded, int w, int h, bool scale);

// Close the camera device.
// Unmaps all buffers and frees everything that was allocated
// by the functions in this interface.
int v4lClose(v4lT* s);

//internal functions
gdImagePtr v4lDecodeMJPEG(void* data, int len);
int v4lDecodeYUYV(v4lBufT* rgb, v4lBufT* yuf);
bool v4lFixHuffman(void** data, int* len);

#endif //V4L_H
