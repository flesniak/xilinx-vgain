#ifndef VIN_MEM
#define VIN_MEM

#define VIN_DEFAULT_DEVICE              "/dev/video0"

//PERIPHERAL PARAMETERS
#define VIN_DEFAULT_ADDRESS             0x90000000U
#define VIN_CONTROL_REGS_SIZE           8U

#define VIN_REGS_BUS_NAME               "CFGBUS"
#define VIN_VMEM_BUS_NAME               "VINMEMBUS"

//The following values define the layout of the framebuffer memory
#define VIN_DEFAULT_VMEM_ADDRESS        0x00400000U
#define VIN_VMEM_WIDTH                  1024
#define VIN_VMEM_HEIGHT                 768
#define VIN_VMEM_BYTES_PER_PIXEL        4
#define VIN_VMEM_BITS_PER_PIXEL         (VIN_VMEM_BYTES_PER_PIXEL*8)
#define VIN_VMEM_SCANLINE_PIXELS        VIN_VMEM_WIDTH
#define VIN_VMEM_SCANLINE_BYTES         (VIN_VMEM_SCANLINE_PIXELS*VIN_VMEM_BYTES_PER_PIXEL)
#define VIN_VMEM_SIZE                   (VIN_VMEM_SCANLINE_BYTES*VIN_VMEM_HEIGHT) //usually 640x480 pixels 32bpp 0xaaggbbrr (notation in little endian)
#define VIN_VMEM_RMASK                  0x000000ff
#define VIN_VMEM_GMASK                  0x0000ff00
#define VIN_VMEM_BMASK                  0x00ff0000

#define VIN_CAPTURE_WIDTH               640
#define VIN_CAPTURE_HEIGHT              480
#define VIN_CAPTURE_BYTES_PER_PIXEL     VIN_VMEM_BYTES_PER_PIXEL
#define VIN_CAPTURE_SCANLINE_BYTES      (VIN_CAPTURE_WIDTH*VIN_CAPTURE_BYTES_PER_PIXEL)
#define VIN_CAPTURE_SIZE                (VIN_CAPTURE_SCANLINE_BYTES*VIN_CAPTURE_HEIGHT) //usually 640x480 pixels 32bpp 0xaaggbbrr (notation in little endian)

#define VIN_AR_OFFSET_BYTES             0
#define VIN_AR_OFFSET_INT               0
#define VIN_AR_ADDR                     (VIN_DEFAULT_ADDRESS+VIN_AR_OFFSET_BYTES)
#define VIN_AR_ADDR_OFFSET              0
#define VIN_AR_ADDR_MASK                0xffffffff

#define VIN_IR_OFFSET_BYTES             4
#define VIN_IR_OFFSET_INT               1
#define VIN_IR_ADDR                     (VIN_DEFAULT_ADDRESS+VIN_IR_OFFSET_BYTES)
#define VIN_IR_REQ_OFFSET               0
#define VIN_IR_REQ_MASK                 0x00000001

#endif //DVI_MEM
