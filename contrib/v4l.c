#include "v4l.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

#include <libv4l2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>

//open v4l2 device
int v4lOpen(v4lT* s, char* device) {
  s->cam = v4l2_open(device, O_RDWR);
  if( s->cam <= 0 ) {
    perror("Failed to open the video device");
    return 1;
  }
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

  if( s->cap.capabilities & V4L2_CAP_STREAMING ) {
    s->param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    s->param.parm.capture.capturemode = V4L2_MODE_HIGHQUALITY;
    if( v4l2_ioctl(s->cam, VIDIOC_S_PARM, &s->param) ) {
      perror("Setting high quality mode failed");
      v4lClose(s);
      return 1;
    }
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

//get available formats
int v4lGetFormats(v4lT* s) {
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
  int preferredFmt = -1;
  while( fmtsCaptured < s->fmtsCount ) {
    s->fmts[fmtsCaptured].index = fmtsCaptured;
    s->fmts[fmtsCaptured].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if( v4l2_ioctl(s->cam, VIDIOC_ENUM_FMT, s->fmts+fmtsCaptured) == -1 )
      break;
    if( preferredFmt == -1 && !(s->fmts[fmtsCaptured].flags & V4L2_FMT_FLAG_EMULATED) && s->fmts[fmtsCaptured].pixelformat == V4L2_PIX_FMT_MJPEG )
      preferredFmt = fmtsCaptured;
    fmtsCaptured++;
  }
  if( errno != EINVAL || fmtsCaptured < s->fmtsCount ) {
	  perror("Error while querying available image formats");
    v4lClose(s);
	  return 1;
  }
  if( preferredFmt == -1 ) {
    fprintf(stderr, "No supported video format (MJPEG)\n");
    v4lClose(s);
	  return 1;
  }
  return 0;
}

int v4lSetFormat(v4lT* s, int width, int height) {
  s->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  s->fmt.fmt.pix.width = width;
  s->fmt.fmt.pix.height = height;
  s->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
  s->fmt.fmt.pix.field = V4L2_FIELD_NONE;
  if( v4l2_ioctl(s->cam, VIDIOC_G_FMT, &s->fmt) ) {
	  perror("Failed to set video format");
    v4lClose(s);
	  return 1;
  }
  return 0;
}

int v4lSetupMmap(v4lT* s, int nBuffers) {
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
  for( int i = 0; i < s->rqbuf.count; i++ ) {
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
  for( int i = 0; i < s->rqbuf.count; i++ ) {
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

  //Handle buffer
  //uint32_t* d = (uint32_t*)s->bufs[s->buf.index].data;
  //printf("Buffer received! Dump: 0x%08x%08x%08x%08x\n", *(d+0), *(d+1), *(d+2), *(d+3));
  return s->bufs+s->buf.index;
}

gdImagePtr v4lDecodeMJPEG(void* data, int len) {
  while( len > 3 ) {
    unsigned char* d = (unsigned char*)data;
    if( d[0] == 0xFF && d[1] == 0xD8 && d[2] == 0xFF )
      break;
    data = d+1; len--;
  }
  if( len <= 3 ) {
    fprintf(stderr, "No valid jpeg magic found\n");
    return 0;
  }
  gdImagePtr img = gdImageCreateFromJpegPtr(len, data);
  return img;
}

//close and deallocate everything opened by the other functions
int v4lClose(v4lT* s) {
  if( s->fmts ) {
    free(s->fmts);
    s->fmts = 0;
    s->fmtsCount = 0;
  }
  if( s->bufs ) {
    for( int i = 0; i < s->rqbuf.count; i++ )
    	munmap(s->bufs[i].data, s->bufs[i].length);
    free(s->bufs);
    s->bufs = 0;
  }
  v4l2_close(s->cam);
  return 0;
}
