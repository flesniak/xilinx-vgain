#include <peripheral/bhm.h>
#include <string.h>

#include "byteswap.h"
#include "videoin.igen.h"
#include "../vin-cfg.h"

typedef _Bool bool;
static bool bigEndianGuest;
static bool deviceConfigured;

Uns32 initDevice(Uns32 deviceStrP, bool scale, bool byteswap) {
  bhmMessage("F", "VIN_PSE", "Failed to intercept : initdevice");
  return 0;
}

Uns32 configureDevice(Uns32 frm0BaseAddr, Uns32 frmOffset, Uns32 maxFrmIdx) {
  bhmMessage("F", "VIN_PSE", "Failed to intercept : configureDevice");
  return 0;
}

void enable(Uns32 e) {
  bhmMessage("F", "VIN_PSE", "Failed to intercept : enable");
}

//get the current state (frame index, last frame done, frame capture enabled)
unsigned int getState() {
  bhmMessage("F", "VIN_PSE", "Failed to intercept : getState");
  return 0;
}

PPM_REG_READ_CB(readReg) {
  Uns32 reg = (Uns32)user;
  switch( reg ) {
    case 0 : //control register
      bhmMessage("D", "VIN_PSE", "Read from CONTROL for %d bytes at 0x%08x", bytes, (Uns32)addr);
      unsigned int ret = getState();
      return bigEndianGuest ? bswap_32(ret) : ret;
      break;
    case 1 : //xsize register
      bhmMessage("D", "VIN_PSE", "Read from XSIZE for %d bytes at 0x%08x", bytes, (Uns32)addr);
      return bigEndianGuest ? bswap_32(CFGBUS_AB0_data.XSIZE.value) : CFGBUS_AB0_data.XSIZE.value;
      break;
    case 2 : //ysize register
      bhmMessage("D", "VIN_PSE", "Read from YSIZE for %d bytes at 0x%08x", bytes, (Uns32)addr);
      return bigEndianGuest ? bswap_32(CFGBUS_AB0_data.YSIZE.value) : CFGBUS_AB0_data.YSIZE.value;
      break;
    case 3 : //stride register
      bhmMessage("D", "VIN_PSE", "Read from STRIDE for %d bytes at 0x%08x", bytes, (Uns32)addr);
      return bigEndianGuest ? bswap_32(CFGBUS_AB0_data.STRIDE.value) : CFGBUS_AB0_data.STRIDE.value;
      break;
    case 4 : //frm0baseaddr register
      bhmMessage("D", "VIN_PSE", "Read from FRM0_BASEADDR for %d bytes at 0x%08x", bytes, (Uns32)addr);
      return bigEndianGuest ? bswap_32(CFGBUS_AB0_data.FRM0_BASEADDR.value) : CFGBUS_AB0_data.FRM0_BASEADDR.value;
      break;
    case 5 : //frmoffset register
      bhmMessage("D", "VIN_PSE", "Read from FRM_ADDROFFSET for %d bytes at 0x%08x", bytes, (Uns32)addr);
      return bigEndianGuest ? bswap_32(CFGBUS_AB0_data.FRM_ADDROFFSET.value) : CFGBUS_AB0_data.FRM_ADDROFFSET.value;
      break;
    case 6 : //maxFrameIndex register
      bhmMessage("D", "VIN_PSE", "Read from MAX_FRAME_INDEX for %d bytes at 0x%08x", bytes, (Uns32)addr);
      return bigEndianGuest ? bswap_32(CFGBUS_AB0_data.MAX_FRAME_INDEX.value) : CFGBUS_AB0_data.MAX_FRAME_INDEX.value;
      break;
    default:
      bhmMessage("W", "VIN_PSE", "Invalid user data on readReg\n");
  }
  return 0;
}

PPM_REG_WRITE_CB(writeReg) {
  Uns32 reg = (Uns32)user;
  switch( reg ) {
    case 0 : //control register
      bhmMessage("D", "VIN_PSE", "Write to CONTROL at 0x%08x data 0x%08x", (Uns32)addr, data);
      if( bigEndianGuest )
        data = bswap_32(data);
      if( deviceConfigured ) {
        if( !(data & VIN_CONTROL_ENABLE_MASK) )
          bhmMessage("W", "VIN_PSE", "Writing ENABLE to 0 is unsupported, use CAPTURE to disable");
        if( data & VIN_CONTROL_INTERLACE_MASK )
          bhmMessage("W", "VIN_PSE", "INTERLACE is unsupported");
        if( (data & VIN_CONTROL_CAPTURE_MASK) != (CFGBUS_AB0_data.CONTROL.value & VIN_CONTROL_CAPTURE_MASK) )
          enable(data & VIN_CONTROL_CAPTURE_MASK);
        if( data & VIN_CONTROL_SYNCPOL_MASK )
          bhmMessage("W", "VIN_PSE", "SYNCPOL is unsupported");
      } else {
        if( data & ~VIN_CONTROL_ENABLE_MASK )
          bhmMessage("W", "VIN_PSE", "Writing to CONTROL before ENABLE set to one is unsupported");
        if( data & VIN_CONTROL_ENABLE_MASK ) {
          bhmMessage("W", "VIN_PSE", "Configureing device to use frm0baseaddr 0x%08x, frmoffset 0x%08x, max frame index %d", CFGBUS_AB0_data.FRM0_BASEADDR.value, CFGBUS_AB0_data.FRM_ADDROFFSET.value, CFGBUS_AB0_data.MAX_FRAME_INDEX.value);
          configureDevice(CFGBUS_AB0_data.FRM0_BASEADDR.value, CFGBUS_AB0_data.FRM_ADDROFFSET.value, CFGBUS_AB0_data.MAX_FRAME_INDEX.value);
        }
      }
      break;
    case 1 : //xsize register
      bhmMessage("W", "VIN_PSE", "Write to XSIZE at 0x%08x data 0x%08x, changing XSIZE is unsupported", (Uns32)addr, data);
      break;
    case 2 : //ysize register
      bhmMessage("W", "VIN_PSE", "Write to YSIZE at 0x%08x data 0x%08x, changing YSIZE is unsupported", (Uns32)addr, data);
      break;
    case 3 : //stride register
      bhmMessage("W", "VIN_PSE", "Write to STRIDE at 0x%08x data 0x%08x, changing STRIDE is unsupported", (Uns32)addr, data);
      break;
    case 4 : //frm0baseaddr register
      if( deviceConfigured )
        bhmMessage("W", "VIN_PSE", "Write to FRM0_BASEADDR at 0x%08x data 0x%08x, changing it after first enable is unsupported", (Uns32)addr, data);
      else {
        bhmMessage("D", "VIN_PSE", "Write to FRM0_BASEADDR at 0x%08x data 0x%08x", (Uns32)addr, data);
        CFGBUS_AB0_data.FRM0_BASEADDR.value = bigEndianGuest ? bswap_32(data) : data;
      }
      break;
    case 5 : //frmoffset register
      if( deviceConfigured )
        bhmMessage("W", "VIN_PSE", "Write to FRM_ADDROFFSET at 0x%08x data 0x%08x, changing it after first enable is unsupported", (Uns32)addr, data);
      else {
        bhmMessage("D", "VIN_PSE", "Write to FRM_ADDROFFSET at 0x%08x data 0x%08x", (Uns32)addr, data);
        CFGBUS_AB0_data.FRM_ADDROFFSET.value = bigEndianGuest ? bswap_32(data) : data;
      }
      break;
    case 6 : //maxFrameIndex register
      if( deviceConfigured )
        bhmMessage("W", "VIN_PSE", "Write to MAX_FRAME_INDEX at 0x%08x data 0x%08x, changing it after first enable is unsupported", (Uns32)addr, data);
      else {
        bhmMessage("D", "VIN_PSE", "Write to MAX_FRAME_INDEX at 0x%08x data 0x%08x", (Uns32)addr, data);
        CFGBUS_AB0_data.MAX_FRAME_INDEX.value = bigEndianGuest ? bswap_32(data) : data;
      }
      break;
    default:
      bhmMessage("W", "VIN_PSE", "Invalid user data on writeReg\n");
  }
}

PPM_CONSTRUCTOR_CB(constructor) {
  bhmMessage("I", "VIN_PSE", "Constructing");
  periphConstructor();

  deviceConfigured = 0;
  bigEndianGuest = bhmBoolAttribute("bigEndianGuest");
  bool scale = bhmBoolAttribute("scale");
  bool byteswap = bhmBoolAttribute("byteswap");

  char device[16] = {};
  if( !bhmStringAttribute("device", device, 16) )
    bhmMessage("W", "VIN_PSE", "Reading device attribute failed");
  if( device[0] == 0 ) { //default if no device attribute is set
    strcpy(device, VIN_DEFAULT_DEVICE);
    bhmMessage("I", "VIN_PSE", "Using default device %s", device);
  } else
    bhmMessage("I", "VIN_PSE", "Using video device %s", device);

  bhmMessage("I", "VIN_PSE", "Initializing display initDisplay()");
  Uns32 error = initDevice((Uns32)device, scale, byteswap);

  if( !error )
    bhmMessage("I", "VIN_PSE", "Video input device initialized successfully");
  else
    bhmMessage("F", "VIN_PSE", "Failed to initialize input device");

  //initialize registers to reset values
  CFGBUS_AB0_data.XSIZE.value = VIN_VMEM_WIDTH;
  CFGBUS_AB0_data.YSIZE.value = VIN_VMEM_HEIGHT;
  CFGBUS_AB0_data.STRIDE.value = VIN_VMEM_SCANLINE_BYTES;
  CFGBUS_AB0_data.FRM0_BASEADDR.value = VIN_DEFAULT_VMEM_ADDRESS;
  CFGBUS_AB0_data.FRM_ADDROFFSET.value = 0;
  CFGBUS_AB0_data.MAX_FRAME_INDEX.value = VIN_DEFAULT_MAX_FRAME_INDEX;
}

PPM_DESTRUCTOR_CB(destructor) {
  bhmMessage("I", "VIN_PSE", "Destructing");
}

