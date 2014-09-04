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
  memDomainP bufferDomain;
  unsigned char* framebuffer;

  //input parameters
  char* device;
  bool scale;
  
  //runtime variables
  v4lT* v4l;
  pthread_t streamer;
  int streamerState;
  bool request;
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
      continue;
    }
    if( object->request ) {
      //decode mjpeg image
      gdImagePtr img = v4lDecodeMJPEG(buf->data, buf->length);
      if( !img || !gdImageTrueColor(img) ) {
        vmiMessage("W", "VIN_SH", "Could not read image, conversion error?");
        continue;
      }

      //scale image if activated
      unsigned int w = gdImageSX(img);
      unsigned int h = gdImageSY(img);
      if( object->scale && (w != VIN_VMEM_WIDTH || h != VIN_VMEM_HEIGHT) ) {
        gdImagePtr src = img;
        img = gdImageCreateTrueColor(VIN_VMEM_WIDTH, VIN_VMEM_HEIGHT);
        gdImageCopyResampled(img, src, 0, 0, 0, 0, VIN_VMEM_WIDTH, VIN_VMEM_HEIGHT, w, h);
        gdImageDestroy(src);
        w = VIN_VMEM_WIDTH;
        h = VIN_VMEM_HEIGHT;
      }

      //copy image to mapped framebuffer
      for( int y = 0; y < h; y++ )
        for( int x = 0; x < w; x++ )
          *(((uint32_t*)object->framebuffer)+y*VIN_VMEM_SCANLINE_PIXELS+x) = gdImageGetPixel(img, x, y);
      gdImageDestroy(img);
      object->request = 0;
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
  Uns32 index = 0;
  GET_ARG(processor, object, index, object->scale);
  GET_ARG(processor, object, index, object->device);
  object->v4l = calloc(1, sizeof(v4lT));

  //read device string
  char* dev = calloc(32, 1);
  memDomainP domain = vmirtGetProcessorDataDomain(processor);
  vmirtReadNByteDomain(domain, (Uns32)object->device, dev, 32, 0, MEM_AA_FALSE);
  object->device = dev;

  vmiMessage("F", "VIN_SH", "using device %s\n", object->device); //try&error
  if( v4lOpen(object->v4l, object->device) )
    exit(EXIT_FAILURE);

  if( v4lCheckCapabilities(object->v4l) ) {
    vmiMessage("W", "VIN_SH", "Device does not have STREAMING capability");
    retArg(processor, object, 1); //return failure
    return;
  }
  if( v4lSetFormat(object->v4l, 640, 480) ) {
    vmiMessage("W", "VIN_SH", "Failed to set format - device can not stream MJPEG?");
    retArg(processor, object, 1); //return failure
    return;
  }
  vmiMessage("I", "VIN_SH", "Set format to %dx%d mjpeg\n", object->v4l->fmt.fmt.pix.width, object->v4l->fmt.fmt.pix.height);
  if( v4lSetupMmap(object->v4l, 5) ) {
    vmiMessage("W", "VIN_SH", "Device does not have STREAMING capability");
    retArg(processor, object, 1); //return failure
    return;
  }
  vmiMessage("I", "VIN_SH", "Got %d mmap buffers\n", object->v4l->rqbuf.count);

  object->framebuffer = calloc(1, VIN_VMEM_SIZE);
  object->bufferDomain = vmirtNewDomain("buffer", 32);
  getSimulatedVmemDomain(processor, VIN_VMEM_BUS_NAME); //just to check if VMEMBUS is connected
  if( !vmirtMapNativeMemory(object->bufferDomain, 0, VIN_VMEM_SIZE-1, object->framebuffer) )
  	vmiMessage("F", "VIN_SH", "Failed to map native vmem to semihost memory domain");

  vmiMessage("I", "VIN_SH", "Launching streamer thread");
  pthread_create(&object->streamer, 0, streamerThread, (void*)object);

  retArg(processor, object, 0); //return success
}

static VMIOS_INTERCEPT_FN(mapMemory) {
  Uns32 newVmemAddress = 0, index = 0;
  GET_ARG(processor, object, index, newVmemAddress);
  vmiMessage("I", "VIN_SH", "Mapping external vmem (new addr 0x%08x, old addr 0x%08x)", newVmemAddress, object->vmemAddr);
  if( object->vmemAddr ) {
    vmiMessage("I", "VIN_SH", "Unaliasing previously mapped memory at 0x%08x", object->vmemAddr);
    memDomainP simDomain = getSimulatedVmemDomain(processor, VIN_VMEM_BUS_NAME);
    vmirtUnaliasMemory(simDomain, object->vmemAddr, object->vmemAddr+VIN_VMEM_SIZE-1);
    //vmirtMapMemory(simDomain, object->vmemAddr, object->vmemAddr+VIN_VMEM_SIZE-1, MEM_RAM);
    //NOTE this unalias command does not work as expected. OVPsim seems to not yet have a way to dynamically unmap vmipse-mapped memory
    //The memory will be unmapped completely and not re-mapped to the ordinary platform-initialized RAM
    //Perhaps try to get originally mapped domain, save it and map it back later on here?
  }
  vmipseAliasMemory(object->bufferDomain, VIN_VMEM_BUS_NAME, newVmemAddress, newVmemAddress+VIN_VMEM_SIZE-1);
  object->vmemAddr = newVmemAddress;
}

static VMIOS_INTERCEPT_FN(requestFrame) {
  Uns32 request = 0, index = 0;
  GET_ARG(processor, object, index, request);
  if( request )
    object->request = 1;
  retArg(processor, object, object->request);
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
  object->bufferDomain = 0;
  object->framebuffer = 0;
  object->scale = 0;
  object->device = 0;
  object->v4l = 0;
  object->streamer = 0;
  object->streamerState = 0;
  object->request = 0;
}

static VMIOS_CONSTRUCTOR_FN(destructor) {
  vmiMessage("I" ,"VIN_SH", "Shutting down");
  object->streamerState = 2; //set stop condition
  pthread_join(object->streamer, 0);
  if( v4lClose(object->v4l) )
    vmiMessage("W", "VIN_SH", "Failed to close input device");
  if( object->framebuffer )
    free(object->framebuffer);
  if( object->device )
    free(object->device);
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
        {"mapMemory",          0,       True,   mapMemory         },
        {"requestFrame",       0,       True,   requestFrame      },
        {0}
    }
};
