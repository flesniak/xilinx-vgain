#include <peripheral/bhm.h>
#include <string.h>

#include "byteswap.h"
#include "videoin.igen.h"
#include "../vin-cfg.h"

typedef _Bool bool;
static bool bigEndianGuest;

Uns32 initDevice(char* device, bool scale) {
  bhmMessage("F", "VIN_PSE", "Failed to intercept : initdevice");
  return 0;
}

void mapMemory(Uns32 address) {
  bhmMessage("F", "VIN_PSE", "Failed to intercept : mapMemory");
}

//requests a new frame if request==1, in any case returns if the last requested frame is ready
int requestFrame(bool request) {
  bhmMessage("F", "VIN_PSE", "Failed to intercept : requestFrame");
  return 0;
}

PPM_REG_READ_CB(readReg) {
  Uns32 reg = (Uns32)user;
  switch( reg ) {
    case 0 : //address register
      bhmMessage("D", "VIN_PSE", "Read from AR for %d bytes at 0x%08x", bytes, (Uns32)addr);
      return bigEndianGuest ? bswap_32(CFGBUS_AB0_data.AR.value) : CFGBUS_AB0_data.AR.value;
      break;
    case 1 : //interaction register
      bhmMessage("D", "VIN_PSE", "Read from IR for %d bytes at 0x%08x", bytes, (Uns32)addr);
      return bigEndianGuest ? bswap_32(requestFrame(0)) : requestFrame(0);
      break;
    default:
      bhmMessage("W", "VIN_PSE", "Invalid user data on readReg\n");
  }

  return 0;
}

PPM_REG_WRITE_CB(writeReg) {
  Uns32 reg = (Uns32)user;
  switch( reg ) {
    case 0 : //address register
      bhmMessage("D", "VIN_PSE", "Write to AR at 0x%08x data 0x%08x", (Uns32)addr, data);
      CFGBUS_AB0_data.AR.value = bigEndianGuest ? bswap_32(data) : data;
      mapMemory(CFGBUS_AB0_data.AR.value);
      break;
    case 1 : //interaction register
      bhmMessage("D", "VIN_PSE", "Write to IR at 0x%08x data 0x%08x", (Uns32)addr, data);
      CFGBUS_AB0_data.IR.value = bigEndianGuest ? bswap_32(data) : data;
      if( CFGBUS_AB0_data.IR.value & VIN_IR_REQ_MASK ) //only react if request bit is set, writing to 0 has no effect
        requestFrame(1);
      break;
    default:
      bhmMessage("W", "VIN_PSE", "Invalid user data on writeReg\n");
  }
}

PPM_CONSTRUCTOR_CB(constructor) {
  bhmMessage("I", "VIN_PSE", "Constructing");
  periphConstructor();

  bigEndianGuest = bhmBoolAttribute("bigEndianGuest");
  bool scale = bhmBoolAttribute("scale");

  char device[4] = {};
  //device[3] = 0;
  if( !bhmStringAttribute("device", device, 4) )
    bhmMessage("W", "VIN_PSE", "Reading device attribute failed");
  if( device[0] == 0 ) { //default if no device attribute is set
    strcpy(device, VIN_DEFAULT_DEVICE);
    bhmMessage("F", "VIN_PSE", "Using default device %s", VIN_DEFAULT_DEVICE);
  }

  bhmMessage("I", "VIN_PSE", "Initializing display initDisplay()");
  Uns32 error = initDevice(device, scale);

  if( !error )
    bhmMessage("I", "VIN_PSE", "Video input device initialized successfully");
  else
    bhmMessage("F", "VIN_PSE", "Failed to initialize input device");

  //initialize registers to reset values
  CFGBUS_AB0_data.AR.value = VIN_DEFAULT_VMEM_ADDRESS;

  mapMemory(CFGBUS_AB0_data.AR.value);
}

PPM_DESTRUCTOR_CB(destructor) {
  bhmMessage("I", "VIN_PSE", "Destructing");
}

