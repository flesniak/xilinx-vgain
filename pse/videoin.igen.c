
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20150205.0
//                          Thu May  7 10:28:35 2015
//
////////////////////////////////////////////////////////////////////////////////


#include "videoin.igen.h"
/////////////////////////////// Port Declarations //////////////////////////////

CFGBUS_AB0_dataT CFGBUS_AB0_data;

handlesT handles;

/////////////////////////////// Diagnostic level ///////////////////////////////

// Test this variable to determine what diagnostics to output.
// eg. if (diagnosticLevel >= 1) bhmMessage("I", "videoin", "Example");
//     Predefined macros PSE_DIAG_LOW, PSE_DIAG_MEDIUM and PSE_DIAG_HIGH may be used
Uns32 diagnosticLevel;

/////////////////////////// Diagnostic level callback //////////////////////////

static void setDiagLevel(Uns32 new) {
    diagnosticLevel = new;
}

///////////////////////////// MMR Generic callbacks ////////////////////////////

//////////////////////////////// View callbacks ////////////////////////////////

static PPM_VIEW_CB(view_CFGBUS_AB0_AR) {
    *(Uns32*)data = CFGBUS_AB0_data.AR.value;
}

static PPM_VIEW_CB(view_CFGBUS_AB0_IR) {
    *(Uns32*)data = CFGBUS_AB0_data.IR.value;
}
//////////////////////////////// Bus Slave Ports ///////////////////////////////

static void installSlavePorts(void) {
    handles.CFGBUS = ppmCreateSlaveBusPort("CFGBUS", 8);

}

//////////////////////////// Memory mapped registers ///////////////////////////

static void installRegisters(void) {

    ppmCreateRegister("AB0_AR",
        0,
        handles.CFGBUS,
        0,
        4,
        readReg,
        writeReg,
        view_CFGBUS_AB0_AR,
        (void*)0x0,
        True
    );
    ppmCreateRegister("AB0_IR",
        0,
        handles.CFGBUS,
        4,
        4,
        readReg,
        writeReg,
        view_CFGBUS_AB0_IR,
        (void*)0x1,
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

    ppmDocNodeP doc1_node = ppmDocAddSection(0, "Licensing");
    ppmDocAddText(doc1_node, "Open Source Apache 2.0");
    ppmDocNodeP doc_11_node = ppmDocAddSection(0, "Description");
    ppmDocAddText(doc_11_node, "V4L2 to OVP video input peripheral");

    diagnosticLevel = 0;
    bhmInstallDiagCB(setDiagLevel);
    constructor();

    bhmWaitEvent(bhmGetSystemEvent(BHM_SE_END_OF_SIMULATION));
    destructor();
    return 0;
}

