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
  memDomainP guestDomain;
  v4lBufT framebuffer;

  //input parameters
  char device[16];
  uint32_t scale;
  uint32_t byteswap;
  
  //runtime variables
  v4lT* v4l;
  pthread_t streamer;
  int streamerState;
  uint32_t request;

  //stats
  unsigned int frames;
  unsigned int droppedFrames;
  unsigned int errorFrames;
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
  uint32_t* dEnd = d + framebuffer->length/4;
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
    v4lBufT* buf = v4lGetImage(object->v4l); //get every frame to avoid framedropping and having stale images in the queue
    if( !buf ) {
      vmiMessage("W", "VIN_SH", "Failed to get image");
      object->errorFrames++;
      continue;
    }
    object->frames++;
    if( object->request ) {
      if( v4lDecodeImage(object->v4l, &object->framebuffer, buf, VIN_VMEM_WIDTH, VIN_VMEM_HEIGHT, object->scale) ) {
        vmiMessage("W", "VIN_SH", "Could not decode captured image, dropping");
        object->errorFrames++;
        continue;
      }
      if( object->byteswap )
        byteswapVmem(&object->framebuffer);
      object->request = 0;
    } else
      object->droppedFrames++;
  }
  object->streamerState = 0; //thread is stopped
  pthread_exit(0);
}

memDomainP getSimulatedVmemDomain(vmiProcessorP processor, char* name) {
  Addr lo, hi;
  Bool isMaster, isDynamic;
  memDomainP simDomain = vmipsePlatformPortAttributes(processor, name, &lo, &hi, &isMaster, &isDynamic);
  if( !simDomain )
    vmiMessage("F", "VIN_SH", "Failed to obtain %s platform port", name);
  return simDomain;
}

static VMIOS_INTERCEPT_FN(initDevice) {
  (void)thisPC; (void)context; (void)userData; (void)atOpaqueIntercept;
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
  if( v4lCheckFormats(object->v4l, V4L2_PIX_FMT_MJPEG) ) {
    vmiMessage("W", "VIN_SH", "Failed to check formats");
    retArg(processor, object, 1); //return failure
    return;
  }
  if( v4lSetFormat(object->v4l, object->v4l->preferredPixFmtIndex, VIN_CAPTURE_WIDTH, VIN_CAPTURE_HEIGHT) ) {
    vmiMessage("W", "VIN_SH", "Failed to set format");
    retArg(processor, object, 1); //return failure
    return;
  }
  vmiMessage("I", "VIN_SH", "Set format to %dx%d\n", object->v4l->fmt.fmt.pix.width, object->v4l->fmt.fmt.pix.height);
  if( v4lSetupMmap(object->v4l, 5) ) {
    vmiMessage("W", "VIN_SH", "Failed to setup memory mapping");
    retArg(processor, object, 1); //return failure
    return;
  }
  vmiMessage("I", "VIN_SH", "Got %d mmap buffers\n", object->v4l->rqbuf.count);

  object->framebuffer.data = calloc(1, VIN_VMEM_SIZE);
  object->framebuffer.length = VIN_VMEM_SIZE;
  object->framebuffer.w = VIN_VMEM_WIDTH;
  object->framebuffer.h = VIN_VMEM_HEIGHT;
  object->guestDomain = getSimulatedVmemDomain(processor, VIN_VMEM_BUS_NAME); //just to check if VMEMBUS is connected
  if( !object->framebuffer.data ) {
    vmiMessage("W", "VIN_SH", "Failed to allocate internal framebuffer");
    retArg(processor, object, 1); //return failure
    return;
  }

  vmiMessage("I", "VIN_SH", "Launching streamer thread");
  pthread_create(&object->streamer, 0, streamerThread, (void*)object);

  retArg(processor, object, 0); //return success
}

static VMIOS_INTERCEPT_FN(mapMemory) {
  (void)thisPC; (void)context; (void)userData; (void)atOpaqueIntercept;
  Uns32 newVmemAddress = 0, index = 0;
  GET_ARG(processor, object, index, newVmemAddress);
  if( object->vmemAddr ) {
    vmiMessage("I", "TFT_SH", "Unaliasing previously mapped memory at 0x%08x", object->vmemAddr);
    vmirtUnaliasMemory(object->guestDomain, object->vmemAddr, object->vmemAddr+VIN_VMEM_SIZE-1);
    vmirtMapMemory(object->guestDomain, object->vmemAddr, object->vmemAddr+VIN_VMEM_SIZE-1, MEM_RAM);
    vmirtWriteNByteDomain(object->guestDomain, object->vmemAddr, object->framebuffer.data, object->framebuffer.length, 0, MEM_AA_FALSE);
  }
  vmiMessage("I", "VIN_SH", "Mapping external vmem (new addr 0x%08x, old addr 0x%08x)", newVmemAddress, object->vmemAddr);
  vmirtReadNByteDomain(object->guestDomain, newVmemAddress, object->framebuffer.data, object->framebuffer.length, 0, MEM_AA_FALSE);
  vmirtMapNativeMemory(object->guestDomain, newVmemAddress, newVmemAddress+object->framebuffer.length-1, object->framebuffer.data);
  object->vmemAddr = newVmemAddress;
}

static VMIOS_INTERCEPT_FN(requestFrame) {
  (void)thisPC; (void)context; (void)userData; (void)atOpaqueIntercept;
  Uns32 request = 0, index = 0;
  GET_ARG(processor, object, index, request);
  if( request )
    object->request = 1;
  retArg(processor, object, object->request);
}

static VMIOS_CONSTRUCTOR_FN(constructor) {
  (void)parameterValues;
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
  object->guestDomain = 0;
  memset(&object->framebuffer, 0, sizeof(v4lBufT));
  object->scale = 0;
  object->device[0] = 0;
  object->v4l = 0;
  object->streamer = 0;
  object->streamerState = 0;
  object->request = 0;
  object->frames = 0;
  object->droppedFrames = 0;
  object->errorFrames = 0;
}

static VMIOS_DESTRUCTOR_FN(destructor) {
  (void)processor;
  vmiMessage("I" ,"VIN_SH", "Shutting down");
  object->streamerState = 2; //set stop condition
  pthread_join(object->streamer, 0);
  if( v4lClose(object->v4l) )
    vmiMessage("W", "VIN_SH", "Failed to close input device");
  if( object->v4l )
    free(object->v4l);
  if( object->framebuffer.data )
    free(object->framebuffer.data);
  vmiMessage("I", "VIN_SH", "Shutdown complete: %d frames captured, thereof %d frames dropped and %d frames with errors", object->frames, object->droppedFrames, object->errorFrames);
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
        {"initDevice",         0,       True,   initDevice,   0, 0 },
        {"mapMemory",          0,       True,   mapMemory,    0, 0 },
        {"requestFrame",       0,       True,   requestFrame, 0, 0 },
        {0}
    }
};
