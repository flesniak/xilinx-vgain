
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20150205.0
//                          Thu May  7 10:28:35 2015
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


/////////////////////////// Dynamic Diagnostic Macros //////////////////////////

// Bottom two bits of word used for PSE diagnostics
#define PSE_DIAG_LOW      (BHM_DIAG_MASK_LOW(diagnosticLevel))
#define PSE_DIAG_MEDIUM   (BHM_DIAG_MASK_MEDIUM(diagnosticLevel))
#define PSE_DIAG_HIGH     (BHM_DIAG_MASK_HIGH(diagnosticLevel))
// Next two bits of word used for PSE semihost/intercept library diagnostics
#define PSE_DIAG_SEMIHOST (BHM_DIAG_MASK_SEMIHOST(diagnosticLevel))

/////////////////////////// Register data declaration //////////////////////////

typedef struct CFGBUS_AB0_dataS { 
    union { 
        Uns32 value;
        struct {
            unsigned BASEADDR : 32;
        } bits;
    } AR;
    union { 
        Uns32 value;
        struct {
            unsigned REQUEST : 1;
        } bits;
    } IR;
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
