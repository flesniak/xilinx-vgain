#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../vin-cfg.h"
#include "../../xilinx-dvi/dvi-mem.h"

int main() {
  const unsigned int maxFrmIdx = 2; //get 3 frames: 0, 1, 2
  const unsigned int frmOffset = 0; //in tupels of 4 bytes

  volatile unsigned int* const control = (unsigned int*)VIN_CONTROL_ADDR;
  volatile unsigned int* const maxIdx  = (unsigned int*)VIN_MAXFRMINDEX_ADDR;
  volatile unsigned int* const frmAddr = (unsigned int*)VIN_FRM0ADDR_ADDR;
  volatile unsigned int* const frmOff  = (unsigned int*)VIN_FRMOFFSET_ADDR;
  volatile unsigned int* const xsize   = (unsigned int*)VIN_XSIZE_ADDR;
  volatile unsigned int* const ysize   = (unsigned int*)VIN_YSIZE_ADDR;
  volatile unsigned int* const stride  = (unsigned int*)VIN_STRIDE_ADDR;
           unsigned int* const fbIn    = (unsigned int*)0x88008800;
           unsigned int* const fbOut   = (unsigned int*)DVI_VMEM_ADDRESS;

  printf("Configuring video_to_vfbc addr %p offset 0 maxFrms 3\n", fbIn);
  *maxIdx   = maxFrmIdx;
  *frmAddr  = (unsigned int)fbIn;
  *frmOff   = frmOffset;
  *xsize    = VIN_VMEM_WIDTH;
  *ysize    = VIN_VMEM_HEIGHT;
  *stride   = VIN_VMEM_SCANLINE_BYTES;
  *control |= VIN_CONTROL_ENABLE_MASK; //configure device
  *control |= VIN_CONTROL_CAPTURE_MASK; //start first capture

  int frames = 0;
  unsigned int state = 0;
  unsigned int nextFrame = 0;
  printf("Start dumping frames...\n");
  while( 1 ) {
    state = *control;
    if( nextFrame >= maxFrmIdx+1 && !(state & VIN_CONTROL_CAPTURE_MASK) ) { //capturing is not yet enabled or disabled after capturing last frame
      *control |= VIN_CONTROL_CAPTURE_MASK;
      nextFrame = 0;
      continue;
    }
    if( (state >> 16) <= nextFrame && !(state & VIN_CONTROL_LASTFRAME_MASK) ) { //retry if expected frame is still being written
      continue;
    }
    //printf("Dumping frame %d state 0x%08x addr 0x%08x\n", nextFrame, state, (unsigned int)(fbIn+(nextFrame*(VIN_VMEM_SIZE_FRAME/4+frmOffset))));
    for( unsigned int y = 0; y < VIN_VMEM_HEIGHT; y++ )
      for( unsigned int x = 0; x < VIN_VMEM_WIDTH; x++ )
        *(fbOut+y*DVI_VMEM_WIDTH+x) = (*(fbIn+(nextFrame*(VIN_VMEM_SIZE_FRAME/4+frmOffset))+y*VIN_VMEM_SCANLINE_PIXELS+x) >> 2) & 0x003F3F3F;
    nextFrame++;
    frames++;
    if( (frames & 0xF) == 0 )
      printf("Dumped %d frames\n", frames);
  }

  return 0;
}

