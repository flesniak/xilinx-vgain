#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../vin-cfg.h"
#include "../../xilinx-dvi/dvi-mem.h"

//100 MIPS = 100 kIPms = 100 IPµs
void usleep(unsigned int us) {
  while( us ) {
    for(volatile unsigned char i = 0; i < 80; i++); //this loop takes ~100 instructions
    us--;
  }
}

int main() {
  unsigned int* ir    = (unsigned int*)VIN_IR_ADDR;
  unsigned int* fbIn  = (unsigned int*)VIN_DEFAULT_VMEM_ADDRESS;
  unsigned int* fbOut = (unsigned int*)DVI_VMEM_ADDRESS;

  printf("Requesting a frame\n");
  while( 1 ) {
    *ir = VIN_IR_REQ_MASK; //request frame
    while( *ir & VIN_IR_REQ_MASK ); //poll for requested frame
    printf("Frame received\n");
    for( unsigned int y = 0; y < VIN_VMEM_HEIGHT; y++ )
      for( unsigned int x = 0; x < VIN_VMEM_WIDTH; x++ )
        *(fbOut+y*DVI_VMEM_WIDTH+x) = *(fbIn+y*VIN_VMEM_SCANLINE_PIXELS+x);
    printf("Frame dumped, requesting next frame\n");
  }

  return 0;
}

