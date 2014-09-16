
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20131018.0
//                          Tue Sep 16 12:19:46 2014
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
#define CFGBUS_AB0_AR    (CFGBUS + 0x0)

#define CFGBUS_AB0_AR_BASEADDR   0xffffffff
#define CFGBUS_AB0_IR    (CFGBUS + 0x4)

#define CFGBUS_AB0_IR_BASEADDR   0x1


#endif
