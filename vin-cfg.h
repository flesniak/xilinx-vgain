#ifndef VIN_MEM
#define VIN_MEM

#define VIN_DEFAULT_DEVICE              "/dev/video0"
#define VIN_DEFAULT_MAX_FRAME_INDEX     9 //equals 10 max frames, setting MAXFRMINDEX to values >=10 will not work!
#define VIN_DEFAULT_MAX_FRAMES          (VIN_DEFAULT_MAX_FRAME_INDEX+1)

//PERIPHERAL PARAMETERS - you should not need to change anything below this point!
#define VIN_DEFAULT_ADDRESS             0x90000000U
#define VIN_CONTROL_REGS_SIZE           28U

#define VIN_REGS_BUS_NAME               "CFGBUS"
#define VIN_VMEM_BUS_NAME               "VINMEMBUS"

//The following values define the layout of the framebuffer memory
#define VIN_DEFAULT_VMEM_ADDRESS        (VIN_DEFAULT_ADDRESS+VIN_CONTROL_REGS_SIZE)
#define VIN_VMEM_SIZE_FRAME             0x12c000U //640x480 pixels 32bpp 0xaaggbbrr (notation in little endian), for VIN_DEFAULT_MAX_FRAME_INDEX frames
#define VIN_VMEM_SIZE                   (VIN_DEFAULT_MAX_FRAME_INDEX*VIN_VMEM_SIZE_FRAME)
#define VIN_VMEM_WIDTH                  640
#define VIN_VMEM_HEIGHT                 480
#define VIN_VMEM_BYTES_PER_PIXEL        4
#define VIN_VMEM_BITS_PER_PIXEL         (VIN_VMEM_BYTES_PER_PIXEL*8)
#define VIN_VMEM_SCANLINE_PIXELS        VIN_VMEM_WIDTH
#define VIN_VMEM_SCANLINE_BYTES         (VIN_VMEM_SCANLINE_PIXELS*VIN_VMEM_BYTES_PER_PIXEL)
#define VIN_VMEM_RMASK                  0x000000ff
#define VIN_VMEM_GMASK                  0x0000ff00
#define VIN_VMEM_BMASK                  0x00ff0000

#define VIN_CONTROL_OFFSET_BYTES        0
#define VIN_CONTROL_OFFSET_INT          0
#define VIN_CONTROL_ADDR                (VIN_DEFAULT_ADDRESS+VIN_CONTROL_OFFSET_BYTES)
#define VIN_CONTROL_ENABLE_OFFSET       0
#define VIN_CONTROL_ENABLE_MASK         0x00000001
#define VIN_CONTROL_INTERLACE_OFFSET    1
#define VIN_CONTROL_INTERLACE_MASK      0x00000002
#define VIN_CONTROL_CAPTURE_OFFSET      3
#define VIN_CONTROL_CAPTURE_MASK        0x00000008
#define VIN_CONTROL_SYNCPOL_OFFSET      4
#define VIN_CONTROL_SYNCPOL_MASK        0x00000010
#define VIN_CONTROL_LASTFRAME_OFFSET    15
#define VIN_CONTROL_LASTFRAME_MASK      0x00008000
#define VIN_CONTROL_FRAMEIDX_OFFSET     16
#define VIN_CONTROL_FRAMEIDX_MASK       0xffff0000

#define VIN_XSIZE_OFFSET_BYTES          4
#define VIN_XSIZE_OFFSET_INT            1
#define VIN_XSIZE_ADDR                  (VIN_DEFAULT_ADDRESS+VIN_XSIZE_OFFSET_BYTES)

#define VIN_YSIZE_OFFSET_BYTES          8
#define VIN_YSIZE_OFFSET_INT            2
#define VIN_YSIZE_ADDR                  (VIN_DEFAULT_ADDRESS+VIN_YSIZE_OFFSET_BYTES)

#define VIN_STRIDE_OFFSET_BYTES         12
#define VIN_STRIDE_OFFSET_INT           3
#define VIN_STRIDE_ADDR                 (VIN_DEFAULT_ADDRESS+VIN_STRIDE_OFFSET_BYTES)

#define VIN_FRM0ADDR_OFFSET_BYTES       16
#define VIN_FRM0ADDR_OFFSET_INT         4
#define VIN_FRM0ADDR_ADDR               (VIN_DEFAULT_ADDRESS+VIN_FRM0ADDR_OFFSET_BYTES)

#define VIN_FRMOFFSET_OFFSET_BYTES      20
#define VIN_FRMOFFSET_OFFSET_INT        5
#define VIN_FRMOFFSET_ADDR              (VIN_DEFAULT_ADDRESS+VIN_FRMOFFSET_OFFSET_BYTES)

#define VIN_MAXFRMINDEX_OFFSET_BYTES    24
#define VIN_MAXFRMINDEX_OFFSET_INT      6
#define VIN_MAXFRMINDEX_ADDR            (VIN_DEFAULT_ADDRESS+VIN_MAXFRMINDEX_OFFSET_BYTES)

#endif //DVI_MEM
