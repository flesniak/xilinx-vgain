
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20131018.0
//                          Mon Sep 29 12:14:04 2014
//
////////////////////////////////////////////////////////////////////////////////

#ifndef VIDEOIN_MACROS_IGEN_H
#define VIDEOIN_MACROS_IGEN_H
/////////////////////////////////// Licensing //////////////////////////////////

// Open Source Apache 2.0

////////////////////////////////// Description /////////////////////////////////

// V4L2 to OVP video input peripheral

// Before including this file in the application, define the indicated macros
// to fix the base address of each slave port.
// Set the macro 'CFGBUS' to the base of port 'CFGBUS'
#ifndef CFGBUS
    #error CFGBUS is undefined.It needs to be set to the port base address
#endif
#define CFGBUS_AB0_CONTROL    (CFGBUS + 0x0)

#define CFGBUS_AB0_CONTROL_ENABLE   0x1
#define CFGBUS_AB0_CONTROL_INTERLACE_ENABLE   (0x1 << 1)
#define CFGBUS_AB0_CONTROL_CAPTURE_ENABLE   (0x1 << 3)
#define CFGBUS_AB0_CONTROL_SYNC_POLARITY   (0x1 << 4)
#define CFGBUS_AB0_CONTROL_LAST_FRAME_DONE   (0x1 << 15)
#define CFGBUS_AB0_CONTROL_FRAME_INDEX   (0xffff << 16)
#define CFGBUS_AB0_XSIZE    (CFGBUS + 0x4)

#define CFGBUS_AB0_XSIZE_XSIZE   0xffffffff
#define CFGBUS_AB0_YSIZE    (CFGBUS + 0x8)

#define CFGBUS_AB0_YSIZE_YSIZE   0xffffffff
#define CFGBUS_AB0_STRIDE    (CFGBUS + 0xc)

#define CFGBUS_AB0_STRIDE_YSIZE   0xffffffff
#define CFGBUS_AB0_FRM0_BASEADDR    (CFGBUS + 0x10)

#define CFGBUS_AB0_FRM0_BASEADDR_FRM0_BASEADDR   0xffffffff
#define CFGBUS_AB0_FRM_ADDROFFSET    (CFGBUS + 0x14)

#define CFGBUS_AB0_FRM_ADDROFFSET_FRM_ADDROFFSET   0xffffffff
#define CFGBUS_AB0_MAX_FRAME_INDEX    (CFGBUS + 0x18)

#define CFGBUS_AB0_MAX_FRAME_INDEX_MAX_FRAME_INDEX   0xffffffff


#endif
