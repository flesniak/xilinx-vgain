#include "v4l.h"
#include "huffman.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libv4l2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>

#define MIN(a,b) (a<b?a:b)

//open v4l2 device
int v4lOpen(v4lT* s, char* device) {
  s->cam = v4l2_open(device, O_RDWR);
  if( s->cam <= 0 ) {
    perror("Failed to open the video device");
    return 1;
  }
  s->fmts = 0;
  s->frmSizes = 0;
  s->frmIvals = 0;
  s->bufs = 0;
  return 0;
}

//get and check device capabilities
int v4lCheckCapabilities(v4lT* s) {
  if( v4l2_ioctl(s->cam, VIDIOC_QUERYCAP, &s->cap) ) {
    perror("Getting capabilities via ioctl() failed");
    v4lClose(s);
    return 1;
  }
  printf("Using v4l device \"%s\" (driver %s, version %u.%u.%u)", s->cap.card, s->cap.driver, (s->cap.version >> 16) & 0xFF, (s->cap.version >> 8) & 0xFF, s->cap.version & 0xFF);
  printf(" capabilities: 0x%08x\n", s->cap.capabilities);
  if( !(s->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ) {
    fprintf(stderr, "Device does not support capture, aborting\n");
    v4lClose(s);
    return 1;
  }
  return 0;
}

//get current input and enumerate it
int v4lGetInput(v4lT* s) {
  if( v4l2_ioctl(s->cam, VIDIOC_G_INPUT, &s->curIn) ) {
    perror("Failed to get current input");
    v4lClose(s);
    return 1;
  }
  s->in.index = s->curIn;
  if( v4l2_ioctl(s->cam, VIDIOC_ENUMINPUT, &s->in) ) {
    perror("Failed to get current input information");
    v4lClose(s);
    return 1;
  }
  return 0;
}

//print current standard and all standards the current input supports
int v4lGetStandards(v4lT* s) {
  v4l2_std_id camCurrentStd;
  if( ioctl(s->cam, VIDIOC_G_STD, &camCurrentStd) ) {
    perror("Faiuled to get current video standard");
    return 1;
  }
  printf("Current input's standard: 0x%016llx\n", camCurrentStd);

  struct v4l2_standard camStd;
  memset(&camStd, 0, sizeof(camStd));
  camStd.index = 0;
  while( !ioctl(s->cam, VIDIOC_ENUMSTD, &camStd) ) {
    printf("%d: %s\n", camStd.index, camStd.name);
    if( camStd.id == camCurrentStd )
      printf(" (current)\n");
    else
      putchar('\n');
    camStd.index++;
  }
  if( errno != EINVAL || camStd.index == 0 ) {
  perror("Error while querying available standards");
  return 1;
  }
  return 0;
}

int v4lCheckFormats(v4lT* s, uint32_t preferredFormat) {
  if( s->fmts )
    free(s->fmts);
  struct v4l2_fmtdesc camFmtDesc;
  memset(&camFmtDesc, 0, sizeof(camFmtDesc));
  camFmtDesc.index = 0;
  camFmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  s->fmtsCount = 0;
  while( v4l2_ioctl(s->cam, VIDIOC_ENUM_FMT, &camFmtDesc) != -1 ) {
    s->fmtsCount++;
    camFmtDesc.index++;
  }
  s->fmts = calloc(s->fmtsCount, sizeof(struct v4l2_fmtdesc));
  int fmtsCaptured = 0;
  while( fmtsCaptured < s->fmtsCount ) {
    s->fmts[fmtsCaptured].index = fmtsCaptured;
    s->fmts[fmtsCaptured].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if( v4l2_ioctl(s->cam, VIDIOC_ENUM_FMT, s->fmts+fmtsCaptured) == -1 )
      break;
    fmtsCaptured++;
  }
  if( errno != EINVAL || fmtsCaptured < s->fmtsCount ) {
    perror("Error while querying available image formats");
    v4lClose(s);
    return 1;
  }

  s->preferredPixFmtIndex = -1;
  for( int fmt = 0; fmt < s->fmtsCount; fmt++ )
    if( !(s->fmts[fmt].flags & V4L2_FMT_FLAG_EMULATED) && s->fmts[fmt].pixelformat == preferredFormat ) {
      s->preferredPixFmtIndex = fmt;
      break;
    }
  if( s->preferredPixFmtIndex == -1 )
    for( int fmt = 0; fmt < s->fmtsCount; fmt++ )
      if( !(s->fmts[fmt].flags & V4L2_FMT_FLAG_EMULATED) && (s->fmts[fmt].pixelformat == V4L2_PIX_FMT_MJPEG || s->fmts[fmt].pixelformat == V4L2_PIX_FMT_YUYV) ) {
        s->preferredPixFmtIndex = fmt;
        break;
      }
  if( s->preferredPixFmtIndex == -1 ) {
    fprintf(stderr, "No supported video format (YUV/MJPEG)\n");
    v4lClose(s);
    return 1;
  }

  return 0;
}

int v4lGetFramesizes(v4lT* s, int formatIndex) {
  if( s->frmSizes )
    free(s->frmSizes);
  struct v4l2_fmtdesc* fmt = s->fmts+formatIndex;
  struct v4l2_frmsizeenum frmsize;
  memset(&frmsize, 0, sizeof(frmsize));
  frmsize.pixel_format = fmt->pixelformat;
  s->frmSizeCount = 0;
  while( v4l2_ioctl(s->cam, VIDIOC_ENUM_FRAMESIZES, &frmsize) != -1 ) {
    s->frmSizeCount++;
    frmsize.index++;
  }
  s->frmSizes = calloc(s->frmSizeCount, sizeof(struct v4l2_frmsizeenum));
  for( int frm = 0; frm < s->frmSizeCount; frm++ ) {
    s->frmSizes[frm].index = frm;
    s->frmSizes[frm].pixel_format = fmt->pixelformat;
    if( v4l2_ioctl(s->cam, VIDIOC_ENUM_FRAMESIZES, &s->frmSizes[frm]) == -1 ) {
      perror("Failed to query framesize");
      return 1;
    }
  }
  return 0;
}

int v4lGetFrameintervals(v4lT* s, int formatIndex, int width, int height) {
  if( s->frmIvals )
    free(s->frmIvals);
  s->frmIvalCount = 0;
  struct v4l2_frmivalenum frmival;
  memset(&frmival, 0, sizeof(frmival));
  frmival.pixel_format = s->fmts[formatIndex].pixelformat;
  frmival.width = width;
  frmival.height = height;
  while( v4l2_ioctl(s->cam, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) != -1 ) {
    s->frmIvalCount++;
    frmival.index++;
  }
  s->frmIvals = calloc(s->frmIvalCount, sizeof(struct v4l2_frmivalenum));
  for( int i = 0; i < s->frmIvalCount; i++ ) {
    s->frmIvals[i].index = i;
    s->frmIvals[i].pixel_format = s->fmts[formatIndex].pixelformat;
    s->frmIvals[i].width = width;
    s->frmIvals[i].height = height;
    if( v4l2_ioctl(s->cam, VIDIOC_ENUM_FRAMEINTERVALS, s->frmIvals+i) == -1 ) {
      perror("Failed to query frame interval");
      return 1;
    }
  }
  return 0;
}

int v4lSetFormat(v4lT* s, int formatIndex, int width, int height) {
  s->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  s->fmt.fmt.pix.width = width;
  s->fmt.fmt.pix.height = height;
  s->fmt.fmt.pix.pixelformat = s->fmts[formatIndex].pixelformat; //use preferred supported format (YUYV/MJPEG)
  s->fmt.fmt.pix.field = V4L2_FIELD_NONE;
  if( v4l2_ioctl(s->cam, VIDIOC_S_FMT, &s->fmt) ) {
    perror("Failed to set video format");
    v4lClose(s);
    return 1;
  }
  return 0;
}

int v4lSetCaptureParam(v4lT* s, struct v4l2_fract* frameInterval, bool highQuality) {
  s->param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  s->param.parm.capture.capturemode = highQuality ? V4L2_MODE_HIGHQUALITY : 0;
  memcpy(&s->param.parm.capture.timeperframe, frameInterval, sizeof(struct v4l2_fract));
  if( v4l2_ioctl(s->cam, VIDIOC_S_PARM, &s->param) ) {
    if( highQuality ) {
      perror("Setting capture parameters with highQuality failed, retrying without");
      return v4lSetCaptureParam(s, frameInterval, false);
    } else {
      perror("Setting capture parameters failed");
      return 1;
    }
  }
  return 0;
}

int v4lSetupMmap(v4lT* s, int nBuffers) {
  if( s->bufs ) {
    fprintf(stderr, "Buffers are already allocated and mapped?\n");
    return 1;
  }

  s->rqbuf.count = nBuffers;
  s->rqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  s->rqbuf.memory = V4L2_MEMORY_MMAP;

  printf("Requesting %d mmap buffers\n", s->rqbuf.count);
  if( ioctl(s->cam, VIDIOC_REQBUFS, &s->rqbuf) ) {
    perror("Failed to request mmap buffers");
    v4lClose(s);
    return 1;
  }

  s->bufs = calloc(s->rqbuf.count, sizeof(struct v4lBufS));
  if( s->bufs == 0 ) {
    perror("Failed to allocate buffers");
    v4lClose(s);
    return 1;
  }
  for( unsigned int i = 0; i < s->rqbuf.count; i++ ) {
    memset(&s->buf, 0, sizeof(s->buf));
    s->buf.index = i;
    s->buf.type = s->rqbuf.type;
    s->buf.memory = s->rqbuf.memory;
    if( v4l2_ioctl(s->cam, VIDIOC_QUERYBUF, &s->buf) ) {
      perror("Failed to query mmap()ed buffer");
      v4lClose(s);
      return 1;
    }

    s->bufs[i].length = s->buf.length;
    s->bufs[i].data = mmap(0, s->bufs[i].length, PROT_READ | PROT_WRITE, MAP_SHARED, s->cam, s->buf.m.offset);
    s->bufs[i].w = s->fmt.fmt.pix.width; //init w and h to actual format size, note that it can be compressed (aka MJPEG) data!
    s->bufs[i].h = s->fmt.fmt.pix.height;
    if( MAP_FAILED == s->bufs[i].data ) {
      perror("mmap() failed");
      v4lClose(s);
      return 1;
    }
  }
  return 0;
}

int v4lStartStreaming(v4lT* s) {
  if( ioctl(s->cam, VIDIOC_STREAMON, &s->rqbuf.type) ) {
    perror("Failed to start capture streaming");
    v4lClose(s);
    return 1;
  }
  //enqueue all ya buffers
  for( unsigned int i = 0; i < s->rqbuf.count; i++ ) {
    memset(&s->buf, 0, sizeof(s->buf));
    s->buf.index = i;
    s->buf.type = s->rqbuf.type;
    s->buf.memory = s->rqbuf.memory;
    if( ioctl(s->cam, VIDIOC_QBUF, &s->buf) ) {
      perror("Failed to queue buffer");
      return 0;
    }
  }
  s->buf.index = s->rqbuf.count; //initial condition for v4lGetImage()
  return 0;
}

v4lBufT* v4lGetImage(v4lT* s) {
  //enqueue the previous (and now hopefully processed) buffer
  if( s->buf.index < s->rqbuf.count && ioctl(s->cam, VIDIOC_QBUF, &s->buf) ) {
    perror("Failed to queue buffer");
    return 0;
  }

  //Poll a buffer (wait for at least one buffer to be ready)
  /*int pollRes = 0;
  struct pollfd pfd = { .fd = s->cam, .events = POLLIN, .revents = 0 };
  do {
    pollRes = poll(&pfd, 1, 1000);
    if( pollRes == -1 ) {
      perror("Failed to poll buffer");
      return 0;
    }
  } while( pollRes <= 0 );*/

  //now dequeue a filled buffer (usually the oldest of nBuffers)
  //it seems as if VIDIOC_DQBUF also waits until a buffer is available
  memset(&s->buf, 0, sizeof(struct v4l2_buffer));
  s->buf.type = s->rqbuf.type;
  s->buf.memory = s->rqbuf.memory;
  if( ioctl(s->cam, VIDIOC_DQBUF, &s->buf) ) {
    perror("Failed to dequeue buffer");
    return 0;
  }
  /*if( s->buf.length != s->bufs[s->buf.index].length ) {
    printf("new buffer length %d -> %d\n", s->bufs[s->buf.index].length, s->buf.length);
    s->bufs[s->buf.index].length = s->buf.length;
  }*/

  return s->bufs+s->buf.index;
}

int v4lDecodeImage(v4lT* s, v4lBufT* decoded, v4lBufT* encoded, int w, int h, bool scale) {
  if( s->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV ) {
    if( scale )
      fprintf(stderr, "Warning: scaling YUYV input is not implemented yet, will pad instead\n");
    if( decoded->length < encoded->length*2 )
      fprintf(stderr, "Decode buffer is too small to hold the decoded image\n");
    //v4lDecodeYUYV uses w/h of encoded/decoded to calculate padding (if necessary)
    encoded->w = s->fmt.fmt.pix.width;
    encoded->h = s->fmt.fmt.pix.height;
    decoded->w = w;
    decoded->h = h;
    return v4lDecodeYUYV(decoded, encoded);
  }

  if( s->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG ) {
    gdImagePtr img = v4lDecodeMJPEG(encoded->data, encoded->length);
    if( !img || !gdImageTrueColor(img) ) {
      fprintf(stderr, "Could not decode jpeg image, conversion error?\n");
      return 1;
    }

    int imgWidth = gdImageSX(img);
    int imgHeight = gdImageSY(img);

    if( w && h && (imgWidth != w || imgHeight != h) ) {
      if( scale ) { //scale image if necessary, padding will be done below
        gdImagePtr src = img;
        img = gdImageCreateTrueColor(w, h);
        gdImageCopyResized(img, src, 0, 0, 0, 0, w, h, imgWidth, imgHeight);
        gdImageDestroy(src);
      }
      decoded->w = w; //decoded image will be wxh (scaled or padded)
      decoded->h = h;
    } else {
      decoded->w = imgWidth; //decoded image is exactly as big as decoded image
      decoded->h = imgHeight;
    }

    if( decoded->length < decoded->w*decoded->h*4 ) {
      fprintf(stderr, "Decode buffer is too small to hold the decoded image\n");
      return 1;
    }

    //copy pixel data (and implicitly pad with black borders)
    for( int y = 0; y < imgHeight; y++ )
      for( int x = 0; x < imgWidth; x++ )
        *(((uint32_t*)decoded->data)+y*decoded->w+x) = gdImageGetPixel(img, x, y);
    gdImageDestroy(img);
    return 0;
  }

  fprintf(stderr, "Could not decode image. No supported format.\n");
  return 1;
}

//searches for a valid huffman table, if none is found, injects one
bool v4lFixHuffman(void** data, int* len) {
  int origLen = *len;
  unsigned char* d = (unsigned char*)*data;
  bool dhtFound = 0;

  //search for huffman segment
  while( !dhtFound ) {
    if( d[0] == 0xff && d[1] == 0xd8 ) {
      d += 2; //skip SOI
      continue;
    }
    if( d[0] != 0xff ) //jfif segment should always start with 0xFF
      dhtFound = 1; //no valid segment start
    if( d[1] == 0xda ) //start of scan -> every jfif segment scanned, no dht found. insert it here.
      break;
    if( d[1] == 0xc4 ) //dht segment is 0xc4
      dhtFound = 1;
    else
      d += ((unsigned int)d[2] << 8) + d[3] + 2; //go to next segment
  }

  //insert huffman table if not found
  if( !dhtFound ) {
    *len = origLen + sizeof(huffmanTable);
    unsigned char* newData = malloc( *len );
    const unsigned int offset = (void*)d-*data;
    memcpy(newData, *data, offset);
    memcpy(newData+offset, huffmanTable, sizeof(huffmanTable));
    memcpy(newData+offset+sizeof(huffmanTable), d, origLen-offset);
    *data = newData;
    return true;
  }
  return false;
}

gdImagePtr v4lDecodeMJPEG(void* data, int len) {
  while( len > 3 ) { //skip potential garbage till JPEG header is found
    unsigned char* d = (unsigned char*)data;
    if( d[0] == 0xFF && d[1] == 0xD8 && d[2] == 0xFF ) //SOI + start of first segment (usually JFIF)
      break;
    data = d+1; len--;
  }
  if( len <= 3 ) {
    fprintf(stderr, "No valid jpeg magic found\n");
    return 0;
  }
  bool fixed = v4lFixHuffman(&data, &len);
  gdImagePtr img = gdImageCreateFromJpegPtr(len, data);
  if( fixed )
    free(data);
  return img;
}

#define CLAMP(a) ((a) > 0xFF ? 0xff : ((a) < 0 ? 0 : (a)))

int v4lDecodeYUYV(v4lBufT* rgb, v4lBufT* yuf) {
  unsigned char* src;
  unsigned char* dst;
  for( int y = 0; y < yuf->h; y++ ) {
    src = yuf->data + y*yuf->w;
    dst = rgb->data + y*rgb->w; //offset calculation using width implements padding
    for( int nibble = 0; nibble < yuf->w/2; nibble++ ) { // one yuyv nibble is 2 pixels
      unsigned char* y1 = src+4*nibble+0;
      unsigned char* u  = src+4*nibble+1;
      unsigned char* y2 = src+4*nibble+2;
      unsigned char* v  = src+4*nibble+3;
      unsigned char* r1 = dst+8*nibble+2;
      unsigned char* g1 = dst+8*nibble+1;
      unsigned char* b1 = dst+8*nibble+0;
      unsigned char* r2 = dst+8*nibble+6;
      unsigned char* g2 = dst+8*nibble+5;
      unsigned char* b2 = dst+8*nibble+4;
      *r1 = CLAMP((float)*y1 + 1.14    * ((float)*v-128));
      *g1 = CLAMP((float)*y1 - 0.39393 * ((float)*u-128) - 0.58081 * ((float)*v-128));
      *b1 = CLAMP((float)*y1 + 2.028   * ((float)*u-128));
      *r2 = CLAMP((float)*y2 + 1.14    * ((float)*v-128));
      *g2 = CLAMP((float)*y2 - 0.39393 * ((float)*u-128) - 0.58081 * ((float)*v-128));
      *b2 = CLAMP((float)*y2 + 2.028   * ((float)*u-128));
    }
  }
/*  //old implementation without using width/height (does not pad)
  int dataMax = MIN(yuf->length, rgb->length/2);
  for( int i = 0; i < dataMax; i+=4 ) {
    unsigned char* y1 = src+i+0;
    unsigned char* u  = src+i+1;
    unsigned char* y2 = src+i+2;
    unsigned char* v  = src+i+3;
    unsigned char* r1 = dst+2*i+2;
    unsigned char* g1 = dst+2*i+1;
    unsigned char* b1 = dst+2*i+0;
    unsigned char* r2 = dst+2*i+6;
    unsigned char* g2 = dst+2*i+5;
    unsigned char* b2 = dst+2*i+4;
    *r1 = CLAMP((float)*y1 + 1.14    * ((float)*v-128));
    *g1 = CLAMP((float)*y1 - 0.39393 * ((float)*u-128) - 0.58081 * ((float)*v-128));
    *b1 = CLAMP((float)*y1 + 2.028   * ((float)*u-128));
    *r2 = CLAMP((float)*y2 + 1.14    * ((float)*v-128));
    *g2 = CLAMP((float)*y2 - 0.39393 * ((float)*u-128) - 0.58081 * ((float)*v-128));
    *b2 = CLAMP((float)*y2 + 2.028   * ((float)*u-128));
  }*/
  return 0;
}

//close and deallocate everything opened by the other functions
int v4lClose(v4lT* s) {
  if( s->fmts ) {
    free(s->fmts);
    s->fmts = 0;
    s->fmtsCount = 0;
  }
  if( s->frmSizes ) {
    free(s->frmSizes);
    s->frmSizes = 0;
    s->frmSizeCount = 0;
  }
  if( s->frmIvals ) {
    free(s->frmIvals);
    s->frmIvals = 0;
    s->frmIvalCount = 0;
  }
  if( s->bufs ) {
    for( unsigned int i = 0; i < s->rqbuf.count; i++ )
      munmap(s->bufs[i].data, s->bufs[i].length);
    free(s->bufs);
    s->bufs = 0;
  }
  v4l2_close(s->cam);
  return 0;
}
