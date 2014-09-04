
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20131018.0
//                          Thu Sep  4 18:04:00 2014
//
////////////////////////////////////////////////////////////////////////////////


#ifdef _PSE_
#    include "peripheral/impTypes.h"
#    include "peripheral/bhm.h"
#    include "peripheral/ppm.h"
#else
#    include "hostapi/impTypes.h"
#endif


static ppmBusPort busPorts[] = {
    {
        .name            = "CFGBUS",
        .type            = PPM_SLAVE_PORT,
        .addrHi          = 0x7LL,
        .mustBeConnected = 0,
        .remappable      = 0,
        .description     = 0,
    },
    {
        .name            = "VINMEMBUS",
        .type            = PPM_SLAVE_PORT,
        .addrHi          = 0x12bfffLL,
        .mustBeConnected = 1,
        .remappable      = 1,
        .description     = "Video memory, sized 640x480x4Byte=1228800Byte",
    },
    { 0 }
};

static PPM_BUS_PORT_FN(nextBusPort) {
    if(!busPort) {
        return busPorts;
    } else {
        busPort++;
    }
    return busPort->name ? busPort : 0;
}

static ppmNetPort netPorts[] = {
    {
        .name            = "VINSYNCINT",
        .type            = PPM_OUTPUT_PORT,
        .mustBeConnected = 0,
        .description     = 0
    },
    { 0 }
};

static PPM_NET_PORT_FN(nextNetPort) {
    if(!netPort) {
        return netPorts;
    } else {
        netPort++;
    }
    return netPort->name ? netPort : 0;
}

static ppmParameter parameters[] = {
    {
        .name        = "bigEndianGuest",
        .type        = ppm_PT_BOOL,
        .description = 0,
    },
    {
        .name        = "scale",
        .type        = ppm_PT_BOOL,
        .description = 0,
    },
    {
        .name        = "device",
        .type        = ppm_PT_STRING,
        .description = 0,
    },
    { 0 }
};

static PPM_PARAMETER_FN(nextParameter) {
    if(!parameter) {
        return parameters;
    } else {
        parameter++;
    }
    return parameter->name ? parameter : 0;
}

ppmModelAttr modelAttrs = {

    .versionString = PPM_VERSION_STRING,
    .type          = PPM_MT_PERIPHERAL,

    .busPortsCB    = nextBusPort,  
    .netPortsCB    = nextNetPort,  
    .paramSpecCB   = nextParameter,

    .vlnv          = {
        .vendor  = "itiv.kit.edu",
        .library = "peripheral",
        .name    = "videoin",
        .version = "1.0"
    },

    .family    = "itiv.kit.edu",           

};
