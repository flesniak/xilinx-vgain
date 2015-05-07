
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20150205.0
//                          Thu May  7 10:28:35 2015
//
////////////////////////////////////////////////////////////////////////////////

#ifndef VIDEOIN_MACROS_IGEN_H
#define VIDEOIN_MACROS_IGEN_H
// Before including this file in the application, define the indicated macros
// to fix the base address of each slave port.
// Set the macro 'CFGBUS' to the base of port 'CFGBUS'
#ifndef CFGBUS
    #error CFGBUS is undefined.It needs to be set to the port base address
#endif
#define CFGBUS_AB0_AR    (CFGBUS + 0x0)

#define CFGBUS_AB0_AR_BASEADDR   0xffffffff
#define CFGBUS_AB0_IR    (CFGBUS + 0x4)

#define CFGBUS_AB0_IR_REQUEST   0x1


#endif
