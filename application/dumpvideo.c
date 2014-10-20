#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../vin-cfg.h"
#include "../../xilinx-dvi/dvi-mem.h"

int main() {
  volatile unsigned int* ir    = (unsigned int*)VIN_IR_ADDR;
           unsigned int* fbIn  = (unsigned int*)VIN_DEFAULT_VMEM_ADDRESS;
           unsigned int* fbOut = (unsigned int*)DVI_VMEM_ADDRESS;

  int frames = 0;
  printf("Start dumping frames...\n");
  while( 1 ) {
    *ir = VIN_IR_REQ_MASK; //request frame
    while( *ir & VIN_IR_REQ_MASK ); //poll for requested frame
    for( unsigned int y = 0; y < VIN_VMEM_HEIGHT; y++ )
      for( unsigned int x = 0; x < VIN_VMEM_WIDTH; x++ )
        *(fbOut+y*DVI_VMEM_WIDTH+x) = (*(fbIn+y*VIN_VMEM_SCANLINE_PIXELS+x) >> 2) & 0x003F3F3F;
    //printf("dumpvideo: in 0x%08x out 0x%08x\n", *fbIn, *fbOut);
    frames++;
    if( (frames & 0xF) == 0 )
      printf("Dumped %d frames\n", frames);
  }

  return 0;
}

