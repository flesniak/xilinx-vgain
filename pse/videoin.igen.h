
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20131018.0
//                          Tue Oct 21 12:43:48 2014
//
////////////////////////////////////////////////////////////////////////////////

#ifndef VIDEOIN_IGEN_H
#define VIDEOIN_IGEN_H

#ifdef _PSE_
#    include "peripheral/impTypes.h"
#    include "peripheral/bhm.h"
#    include "peripheral/ppm.h"
#else
#    include "hostapi/impTypes.h"
#endif

//////////////////////////////////// Externs ///////////////////////////////////

    extern Uns32 diagnosticLevel;

/////////////////////////// Register data declaration //////////////////////////

typedef struct CFGBUS_AB0_dataS { 
    union { 
        Uns32 value;
        struct {
            unsigned ENABLE : 1;
            unsigned INTERLACE_ENABLE : 1;
            unsigned __pad2 : 1;
            unsigned CAPTURE_ENABLE : 1;
            unsigned SYNC_POLARITY : 1;
            unsigned __pad5 : 10;
            unsigned LAST_FRAME_DONE : 1;
            unsigned FRAME_INDEX : 16;
        } bits;
    } CONTROL;
    union { 
        Uns32 value;
        struct {
            unsigned XSIZE : 32;
        } bits;
    } XSIZE;
    union { 
        Uns32 value;
        struct {
            unsigned YSIZE : 32;
        } bits;
    } YSIZE;
    union { 
        Uns32 value;
        struct {
            unsigned YSIZE : 32;
        } bits;
    } STRIDE;
    union { 
        Uns32 value;
        struct {
            unsigned FRM0_BASEADDR : 32;
        } bits;
    } FRM0_BASEADDR;
    union { 
        Uns32 value;
        struct {
            unsigned FRM_ADDROFFSET : 32;
        } bits;
    } FRM_ADDROFFSET;
    union { 
        Uns32 value;
        struct {
            unsigned MAX_FRAME_INDEX : 32;
        } bits;
    } MAX_FRAME_INDEX;
} CFGBUS_AB0_dataT, *CFGBUS_AB0_dataTP;

/////////////////////////////// Port Declarations //////////////////////////////

extern CFGBUS_AB0_dataT CFGBUS_AB0_data;

#ifdef _PSE_
///////////////////////////////// Port handles /////////////////////////////////

typedef struct handlesS {
    void                 *CFGBUS;
    ppmNetHandle          VINSYNCINT;
} handlesT, *handlesTP;

extern handlesT handles;

////////////////////////////// Callback prototypes /////////////////////////////

PPM_REG_READ_CB(readReg);
PPM_REG_WRITE_CB(writeReg);
PPM_CONSTRUCTOR_CB(periphConstructor);
PPM_DESTRUCTOR_CB(periphDestructor);
PPM_CONSTRUCTOR_CB(constructor);
PPM_DESTRUCTOR_CB(destructor);

#endif

#endif
