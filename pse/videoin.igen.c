
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20131018.0
//                          Tue Oct 21 12:43:48 2014
//
////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////// Licensing //////////////////////////////////

// Open Source Apache 2.0

////////////////////////////////// Description /////////////////////////////////

// V4L2 to OVP video input peripheral


#include "videoin.igen.h"
/////////////////////////////// Port Declarations //////////////////////////////

CFGBUS_AB0_dataT CFGBUS_AB0_data;

handlesT handles;

/////////////////////////////// Diagnostic level ///////////////////////////////

// Test this variable to determine what diagnostics to output.
// eg. if (diagnosticLevel > 0) bhmMessage("I", "videoin", "Example");

Uns32 diagnosticLevel;

/////////////////////////// Diagnostic level callback //////////////////////////

static void setDiagLevel(Uns32 new) {
    diagnosticLevel = new;
}

///////////////////////////// MMR Generic callbacks ////////////////////////////

//////////////////////////////// View callbacks ////////////////////////////////

static PPM_VIEW_CB(view_CFGBUS_AB0_CONTROL) {
    *(Uns32*)data = CFGBUS_AB0_data.CONTROL.value;
}

static PPM_VIEW_CB(view_CFGBUS_AB0_XSIZE) {
    *(Uns32*)data = CFGBUS_AB0_data.XSIZE.value;
}

static PPM_VIEW_CB(view_CFGBUS_AB0_YSIZE) {
    *(Uns32*)data = CFGBUS_AB0_data.YSIZE.value;
}

static PPM_VIEW_CB(view_CFGBUS_AB0_STRIDE) {
    *(Uns32*)data = CFGBUS_AB0_data.STRIDE.value;
}

static PPM_VIEW_CB(view_CFGBUS_AB0_FRM0_BASEADDR) {
    *(Uns32*)data = CFGBUS_AB0_data.FRM0_BASEADDR.value;
}

static PPM_VIEW_CB(view_CFGBUS_AB0_FRM_ADDROFFSET) {
    *(Uns32*)data = CFGBUS_AB0_data.FRM_ADDROFFSET.value;
}

static PPM_VIEW_CB(view_CFGBUS_AB0_MAX_FRAME_INDEX) {
    *(Uns32*)data = CFGBUS_AB0_data.MAX_FRAME_INDEX.value;
}
//////////////////////////////// Bus Slave Ports ///////////////////////////////

static void installSlavePorts(void) {
    handles.CFGBUS = ppmCreateSlaveBusPort("CFGBUS", 28);

}

//////////////////////////// Memory mapped registers ///////////////////////////

static void installRegisters(void) {

    ppmCreateRegister("CONTROL",
        0,
        handles.CFGBUS,
        0,
        4,
        readReg,
        writeReg,
        view_CFGBUS_AB0_CONTROL,
        (void*)0x0,
        True
    );
    ppmCreateRegister("XSIZE",
        0,
        handles.CFGBUS,
        4,
        4,
        readReg,
        writeReg,
        view_CFGBUS_AB0_XSIZE,
        (void*)0x1,
        True
    );
    ppmCreateRegister("YSIZE",
        0,
        handles.CFGBUS,
        8,
        4,
        readReg,
        writeReg,
        view_CFGBUS_AB0_YSIZE,
        (void*)0x2,
        True
    );
    ppmCreateRegister("STRIDE",
        0,
        handles.CFGBUS,
        12,
        4,
        readReg,
        writeReg,
        view_CFGBUS_AB0_STRIDE,
        (void*)0x3,
        True
    );
    ppmCreateRegister("FRM0_BASEADDR",
        0,
        handles.CFGBUS,
        16,
        4,
        readReg,
        writeReg,
        view_CFGBUS_AB0_FRM0_BASEADDR,
        (void*)0x4,
        True
    );
    ppmCreateRegister("FRM_ADDROFFSET",
        0,
        handles.CFGBUS,
        20,
        4,
        readReg,
        writeReg,
        view_CFGBUS_AB0_FRM_ADDROFFSET,
        (void*)0x5,
        True
    );
    ppmCreateRegister("MAX_FRAME_INDEX",
        0,
        handles.CFGBUS,
        24,
        4,
        readReg,
        writeReg,
        view_CFGBUS_AB0_MAX_FRAME_INDEX,
        (void*)0x6,
        True
    );


}

/////////////////////////////////// Net Ports //////////////////////////////////

static void installNetPorts(void) {
// To write to this net, use ppmWriteNet(handles.VINSYNCINT, value);

    handles.VINSYNCINT = ppmOpenNetPort("VINSYNCINT");

}

////////////////////////////////// Constructor /////////////////////////////////

PPM_CONSTRUCTOR_CB(periphConstructor) {
    installSlavePorts();
    installRegisters();
    installNetPorts();
}

///////////////////////////////////// Main /////////////////////////////////////

int main(int argc, char *argv[]) {
    diagnosticLevel = 0;
    bhmInstallDiagCB(setDiagLevel);
    constructor();

    bhmWaitEvent(bhmGetSystemEvent(BHM_SE_END_OF_SIMULATION));
    destructor();
    return 0;
}

