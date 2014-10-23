#include <byteswap.h>
#include <gd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <vmi/vmiMessage.h>
#include <vmi/vmiOSAttrs.h>
#include <vmi/vmiOSLib.h>
#include <vmi/vmiPSE.h>
#include <vmi/vmiTypes.h>

#include "v4l.h"
#include "../vin-cfg.h"

typedef struct vmiosObjectS {
  //register descriptions for get/return arguments
  vmiRegInfoCP resultLow;
  vmiRegInfoCP resultHigh;
  vmiRegInfoCP stackPointer;

  //memory information for mapping
  Uns32 vmemAddr;
  Uns32 framebufferCount;
  Uns32 framebufferOffset;
  Uns32 nextBuffer;
  memDomainP guestDomain;
  v4lBufT framebuffers[VIN_DEFAULT_MAX_FRAMES];

  //input parameters
  char device[16];
  uint32_t scale;
  uint32_t byteswap;
  
  //runtime variables
  v4lT* v4l;
  pthread_t streamer;
  int streamerState;
  uint32_t captureImages;
} vmiosObject;

static void getArg(vmiProcessorP processor, vmiosObjectP object, Uns32 *index, void* result, Uns32 argSize) {
  memDomainP domain = vmirtGetProcessorDataDomain(processor);
  Uns32 spAddr;
  vmirtRegRead(processor, object->stackPointer, &spAddr);
  vmirtReadNByteDomain(domain, spAddr+*index+4, result, argSize, 0, MEM_AA_FALSE);
  *index += argSize;
}

#define GET_ARG(_PROCESSOR, _OBJECT, _INDEX, _ARG) getArg(_PROCESSOR, _OBJECT, &_INDEX, &_ARG, sizeof(_ARG))

inline static void retArg(vmiProcessorP processor, vmiosObjectP object, Uns64 result) {
  vmirtRegWrite(processor, object->resultLow, &result);
  result >>= 32;
  vmirtRegWrite(processor, object->resultHigh, &result);
}

void byteswapVmem(v4lBufT* framebuffer) {
  uint32_t* d = (uint32_t*)framebuffer->data;
  uint32_t* dEnd = d + framebuffer->length/4; //if length%4!=0, last <4 bytes won't be swapped (undefined?)
  for( ; d < dEnd; d++ )
    *d = bswap_32(*d);
}

static void* streamerThread(void* objectV) {
  vmiosObjectP object = (vmiosObjectP)objectV;
  object->streamerState = 1; //thread is running

  vmiMessage("I", "VIN_SH", "Streamer thread started, enabling capture streaming");
  if( v4lStartStreaming(object->v4l) ) {
    vmiMessage("W", "VIN_SH", "Failed to enable capture streaming");
    object->streamerState = 0; //thread is stopped
    pthread_exit(0);
  }

  while( object->streamerState == 1 ) {
    v4lBufT* buf = v4lGetImage(object->v4l); //get every frame to avoid framedropping and having stale images in the queue, this function will block until a buffer is filled
    if( !buf ) {
      vmiMessage("W", "VIN_SH", "THREAD: Failed to get image");
      continue;
    }
    if( object->captureImages ) {
      if( v4lDecodeImage(object->v4l, &object->framebuffers[object->nextBuffer], buf, VIN_VMEM_WIDTH, VIN_VMEM_HEIGHT) ) {
        vmiMessage("W", "VIN_SH", "THREAD: Could not decode captured image, dropping");
        continue;
      }
      if( object->byteswap )
        byteswapVmem(&object->framebuffers[object->nextBuffer]);
      //vmiMessage("I", "VIN_SH", "THREAD: CAPTURE frame %d", object->nextBuffer);
      if( object->nextBuffer >= object->framebufferCount-1 ) //if last buffer is captured, stop capturing (original behaviour of video_to_vfbc is unknown)
        object->captureImages = 0;
      else
        object->nextBuffer++;
    }
  }
  object->streamerState = 0; //thread is stopped
  pthread_exit(0);
}

memDomainP getSimulatedVmemDomain(vmiProcessorP processor, char* name) {
  Addr lo, hi;
  Bool isMaster, isDynamic;
  memDomainP simDomain = vmipsePlatformPortAttributes(processor, VIN_VMEM_BUS_NAME, &lo, &hi, &isMaster, &isDynamic);
  if( !simDomain )
    vmiMessage("F", "VIN_SH", "Failed to obtain %s platform port", VIN_VMEM_BUS_NAME);
  return simDomain;
}

static VMIOS_INTERCEPT_FN(initDevice) {
  Uns32 index = 0, deviceStrP = 0;
  GET_ARG(processor, object, index, deviceStrP);
  GET_ARG(processor, object, index, object->scale);
  GET_ARG(processor, object, index, object->byteswap);

  //read device string
  object->device[15] = 0;
  memDomainP domain = vmirtGetProcessorDataDomain(processor);
  vmirtReadNByteDomain(domain, deviceStrP, object->device, 16, 0, MEM_AA_FALSE);
  vmiMessage("I", "VIN_SH", "Using video device %s\n", object->device);

  object->v4l = calloc(1, sizeof(v4lT));
  if( v4lOpen(object->v4l, object->device) ) {
    vmiMessage("W", "VIN_SH", "Error while opening the video device");
    retArg(processor, object, 1); //return failure
    return;
  }

  if( v4lCheckCapabilities(object->v4l) ) {
    vmiMessage("W", "VIN_SH", "Device does not have STREAMING capability");
    retArg(processor, object, 1); //return failure
    return;
  }
  if( v4lCheckFormats(object->v4l, V4L2_PIX_FMT_YUYV) ) {
    vmiMessage("W", "VIN_SH", "Failed to check formats");
    retArg(processor, object, 1); //return failure
    return;
  }
  if( v4lSetFormat(object->v4l, object->v4l->preferredPixFmtIndex, VIN_VMEM_WIDTH, VIN_VMEM_HEIGHT) ) {
    vmiMessage("W", "VIN_SH", "Failed to set format");
    retArg(processor, object, 1); //return failure
    return;
  }
  vmiMessage("I", "VIN_SH", "Set format to %dx%d\n", object->v4l->fmt.fmt.pix.width, object->v4l->fmt.fmt.pix.height);
  if( v4lSetupMmap(object->v4l, VIN_DEFAULT_MAX_FRAMES) ) {
    vmiMessage("W", "VIN_SH", "Failed to setup memory mapping");
    retArg(processor, object, 1); //return failure
    return;
  }
  vmiMessage("I", "VIN_SH", "Got %d mmap buffers\n", object->v4l->rqbuf.count);

  object->guestDomain = getSimulatedVmemDomain(processor, VIN_VMEM_BUS_NAME);

  vmiMessage("I", "VIN_SH", "Launching streamer thread");
  pthread_create(&object->streamer, 0, streamerThread, (void*)object);

  retArg(processor, object, 0); //return success
}

static VMIOS_INTERCEPT_FN(configureDevice) {
  if( object->framebufferCount ) {
  	vmiMessage("W", "VIN_SH", "Superfluous call to configureDevice suppressed, already configured");
    retArg(processor, object, 1);
	}

  Uns32 index = 0;
  GET_ARG(processor, object, index, object->vmemAddr);
  GET_ARG(processor, object, index, object->framebufferOffset);
  GET_ARG(processor, object, index, object->framebufferCount);
  if( ++object->framebufferCount > VIN_DEFAULT_MAX_FRAMES ) //look at that pre-increment, it's important. maybe not the most intelligent spot to do that.
    vmiMessage("F", "VIN_SH", "More than VIN_DEFAULT_MAX_FRAMES frames requested, aborting");
  vmiMessage("I", "VIN_SH", "Initializing %d buffers at addr 0x%08x offset 0x%08x", object->framebufferCount, object->vmemAddr, object->framebufferOffset);

  unsigned int currentAddr = object->vmemAddr;
  for( uint32_t i = 0; i < object->framebufferCount; i++ ) {
    object->framebuffers[i].data = calloc(1, VIN_VMEM_SIZE_FRAME);
    if( object->framebuffers[i].data == 0 )
    	vmiMessage("F", "VIN_SH", "Failed to allocate buffer %d", i);
    object->framebuffers[i].length = VIN_VMEM_SIZE_FRAME;
    vmiMessage("I", "VIN_SH", "Mapping buffer %d at addr 0x%08x", i, currentAddr);
    if( !vmirtMapNativeMemory(object->guestDomain, currentAddr, currentAddr+VIN_VMEM_SIZE_FRAME-1, object->framebuffers[i].data) )
    	vmiMessage("F", "VIN_SH", "Failed to map native vmem to semihost memory domain");
  	currentAddr += VIN_VMEM_SIZE_FRAME + object->framebufferOffset;
	}

  retArg(processor, object, 1);
}

static VMIOS_INTERCEPT_FN(enable) {
  Uns32 index = 0;
  object->nextBuffer = 0;
  GET_ARG(processor, object, index, object->captureImages);
}

//get the current state (frame index, last frame done, frame capture enabled)
static VMIOS_INTERCEPT_FN(getState) {
  unsigned int state = (object->nextBuffer << VIN_CONTROL_FRAMEIDX_OFFSET) | ((object->nextBuffer+1>=object->framebufferCount && !object->captureImages) ? (1 << VIN_CONTROL_LASTFRAME_OFFSET) : 0) | (object->captureImages ? (1 << VIN_CONTROL_CAPTURE_OFFSET) : 0) | (1 << VIN_CONTROL_ENABLE_OFFSET);
  retArg(processor, object, state);
}

static VMIOS_CONSTRUCTOR_FN(constructor) {
  vmiMessage("I" ,"VIN_SH", "Constructing");

  const char *procType = vmirtProcessorType(processor);
  memEndian endian = vmirtGetProcessorDataEndian(processor);

  if( strcmp(procType, "pse") )
      vmiMessage("F", "VIN_SH", "Processor must be a PSE (is %s)\n", procType);
  if( endian != MEM_ENDIAN_NATIVE )
      vmiMessage("F", "VIN_SH", "Host processor must same endianity as a PSE\n");

  //get register description and store to object for further use
  object->resultLow = vmirtGetRegByName(processor, "eax");
  object->resultHigh = vmirtGetRegByName(processor, "edx");
  object->stackPointer = vmirtGetRegByName(processor, "esp");
  //TODO get endianess directly from the simulated processor instead of a formal? is this possible?

  //initialize object
  object->vmemAddr = 0;
  object->framebufferCount = 0;
  object->framebufferOffset = 0;
  object->guestDomain = 0;
  object->nextBuffer = 0;
  object->scale = 0;
  object->device[0] = 0;
  object->v4l = 0;
  object->streamer = 0;
  object->streamerState = 0;
  object->captureImages = 0;
  
  for( int i = 0; i < VIN_DEFAULT_MAX_FRAMES; i++ ) {
    object->framebuffers[i].data = 0; //allocate dynamically later on...
    object->framebuffers[i].length = 0;
    object->framebuffers[i].w = VIN_VMEM_WIDTH;
    object->framebuffers[i].h = VIN_VMEM_HEIGHT;
  }
}

static VMIOS_CONSTRUCTOR_FN(destructor) {
  vmiMessage("I" ,"VIN_SH", "Shutting down");
  object->streamerState = 2; //set stop condition
  pthread_join(object->streamer, 0);
  if( v4lClose(object->v4l) )
    vmiMessage("W", "VIN_SH", "Failed to close input device");
  if( object->v4l )
    free(object->v4l);
  for( int i = 0; i < object->framebufferCount; i++ )
    if( object->framebuffers[i].data )
      free(object->framebuffers[i].data);
  vmiMessage("I", "VIN_SH", "Shutdown complete");
}

vmiosAttr modelAttrs = {
    .versionString  = VMI_VERSION,                // version string (THIS MUST BE FIRST)
    .modelType      = VMI_INTERCEPT_LIBRARY,      // type
    .packageName    = "videoin",                  // description
    .objectSize     = sizeof(vmiosObject),        // size in bytes of object

    .constructorCB  = constructor,                // object constructor
    .destructorCB   = destructor,                 // object destructor

    .morphCB        = 0,                          // morph callback
    .nextPCCB       = 0,                          // get next instruction address
    .disCB          = 0,                          // disassemble instruction

    // -------------------          -------- ------  -----------------
    // Name                         Address  Opaque  Callback
    // -------------------          -------- ------  -----------------
    .intercepts = {
        {"initDevice",         0,       True,   initDevice        },
        {"configureDevice",    0,       True,   configureDevice   },
        {"enable",             0,       True,   enable            },
        {"getState",           0,       True,   getState          },
        {0}
    }
};
