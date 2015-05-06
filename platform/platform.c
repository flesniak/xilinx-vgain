#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <icm/icmCpuManager.h>

#include "../vin-cfg.h"
#include "../../xilinx-dvi/dvi-mem.h"

#define INSTRUCTION_COUNT_DEFAULT 100000

static struct optionsS {
  int verbosity;
  icmNewProcAttrs processorAttributes;
  unsigned int memorySize; //MiB
  int instructionCount;
  char* display;
  char* capture;
  double wallclockFactor;
  char* program;
} options = { 1, ICM_ATTR_DEFAULT, UINT_MAX, INSTRUCTION_COUNT_DEFAULT, 0, 0, 0, 0 };

void usage( char* argv0 ) {
  fprintf( stderr, "Usage: %s [-v] [-t simple|count|regs] [-m memsize|[-d|-c]] [-i instructions] <program.elf>\n", argv0 );
  fprintf( stderr, "  -v                   | increase verbosity (1 -> OVP stats, 2 -> stack analysis debug\n" );
  fprintf( stderr, "  -t simple|count|regs | enable instruction tracing, with instruction cound or register dump\n" );
  fprintf( stderr, "  -m memsize           | attach a memory of size memsize instead of the implicite one\n" );
  fprintf( stderr, "  -i instructions      | simulate instructions at once (default 100000)\n" );
  fprintf( stderr, "  -w factor            | set wallclock factor \n" );
  fprintf( stderr, "  -d sdl|dlo           | enable dvi display\n" );
  fprintf( stderr, "  -c /dev/videoX       | enable video capture\n" );
  fprintf( stderr, "  <program.elf>        | program file to simulate\n" );
  exit( 1 );
}

void parseOptions( int argc, char** argv ) {
  int c;
  opterr = 0;

  while( ( c = getopt( argc, argv, "vt:m:i:w:d:c:" ) ) != -1 )
    switch( c ) {
      case 'v':
        options.verbosity++;
        break;
      case 't':
        if( !strcmp( optarg, "simple" ) ) {
          options.processorAttributes |= ICM_ATTR_TRACE;
          break;
        }
        if( !strcmp( optarg, "count" ) ) {
          options.processorAttributes |= ICM_ATTR_TRACE_ICOUNT;
          break;
        }
        if( !strcmp( optarg, "regs" ) ) {
          options.processorAttributes |= ICM_ATTR_TRACE_REGS_BEFORE;
          break;
        }
        fprintf( stderr, "Invalid argument to option -t.\n" );
        usage( argv[0] );
        break;
      case 'm':
        if( options.display ) {
          fprintf( stderr, "Options -d and -m are not compatible\n" );
          usage( argv[0] );
        }
        sscanf( optarg, "%d", &options.memorySize );
        if( !options.memorySize ) {
          fprintf( stderr, "Invalid argument to option -m.\n" );
          usage( argv[0] );
        }
        break;
      case 'i':
        sscanf( optarg, "%d", &options.instructionCount );
        if( !options.instructionCount ) {
          fprintf( stderr, "Invalid argument to option -i.\n" );
          usage( argv[0] );
        }
        break;
      case 'w':
        sscanf( optarg, "%lf", &options.wallclockFactor );
        if( !options.wallclockFactor ) {
          fprintf( stderr, "Invalid argument to option -w.\n" );
          usage( argv[0] );
        }
        break;
      case 'd':
        if( options.memorySize != UINT_MAX ) {
          fprintf( stderr, "Options -d and -m are not compatible\n" );
          usage( argv[0] );
        }
        options.display = optarg;
        break;
      case 'c':
        if( options.memorySize != UINT_MAX ) {
          fprintf( stderr, "Options -c and -m are not compatible\n" );
          usage( argv[0] );
        }
        options.capture = optarg;
        break;
      case '?':
        if( optopt == 't' || optopt == 'm' || optopt == 'i' )
          fprintf( stderr, "Option -%c requires an argument.\n", optopt );
        else if( isprint( optopt ) )
          fprintf( stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf( stderr, "Unknown option character `\\x%x'.\n", optopt );
      default:
        usage( argv[0] );
    }

   if( optind+1 != argc ) {
    fprintf( stderr, "Invalid number of arguments\n" );
    usage( argv[0] );
  }

  options.program = argv[optind];
}

int main( int argc, char** argv ) {
  parseOptions( argc, argv );

  if( options.verbosity > 1 ) {
    icmInitPlatform( ICM_VERSION, ICM_VERBOSE | ICM_STOP_ON_CTRLC, 0, 0, "xdvi" );
  } else
    icmInitPlatform( ICM_VERSION, ICM_INIT_DEFAULT | ICM_STOP_ON_CTRLC, 0, 0, "xdvi" );

  const char* model = icmGetVlnvString( 0, "xilinx.ovpworld.org", "processor", "microblaze", "1.0", "model" );
  const char* semihosting = icmGetVlnvString( 0, "xilinx.ovpworld.org", "semihosting", "microblazeNewlib", "1.0", "model" );

  icmAttrListP userAttrs = icmNewAttrList();
  icmAddDoubleAttr(userAttrs, "mips", 800.0);
  //icmAddUns32Attr(userAttrs, "C_ENDIANNESS", 0); //microblaze is big-endian by default
  //icmAddStringAttr(userAttrs, "endian", "little"); //or1k toolchain does not seem to support little endian

  icmProcessorP processor1 = icmNewProcessor(
      "cpu1",             // CPU name
      "microblaze",       // CPU type
      0,                  // CPU cpuId
      0,                  // CPU model flags
      32,                 // address bits
      model,              // model file
      "",                 // morpher attributes
      options.processorAttributes, // enable tracing or register values
      userAttrs,          // user-defined attributes
      semihosting,        // semi-hosting file
      ""                  // semi-hosting attributes
  );

  //dvi display emulation requested
  if( options.display || options.capture ) {
    //bus
    icmBusP bus1 = icmNewBus("bus1", 32);
    icmConnectProcessorBusses(processor1, bus1, bus1);

    //map memory matching configuration. attention: fuzzy logic and calculations
    //only works if DVI_BASE_ADDRESS < VIN_DEFAULT_ADDRESS
    //CASE 1: capture only
    //CASE 2: capture & display
    //CASE 3: display only
    icmMemoryP mem[3] = {};
    if( options.display ) {
      mem[0] = icmNewMemory("mem1", ICM_PRIV_RWX, DVI_BASE_ADDRESS-1);
      if( options.capture ) {
        mem[1] = icmNewMemory("mem2", ICM_PRIV_RWX, VIN_DEFAULT_ADDRESS-(DVI_BASE_ADDRESS+DVI_CONTROL_REGS_SIZE)-1);
        mem[2] = icmNewMemory("mem3", ICM_PRIV_RWX, UINT_MAX-(VIN_DEFAULT_ADDRESS+VIN_CONTROL_REGS_SIZE)-1);
        icmConnectMemoryToBus(bus1, "port3", mem[2], VIN_DEFAULT_ADDRESS+VIN_CONTROL_REGS_SIZE);
      } else
        mem[1] = icmNewMemory("mem2", ICM_PRIV_RWX, UINT_MAX-(DVI_BASE_ADDRESS+DVI_CONTROL_REGS_SIZE)-1);
      icmConnectMemoryToBus(bus1, "port2", mem[1], DVI_BASE_ADDRESS+DVI_CONTROL_REGS_SIZE);
    } else {
      mem[0] = icmNewMemory("mem1", ICM_PRIV_RWX, VIN_DEFAULT_ADDRESS-1);
      mem[1] = icmNewMemory("mem2", ICM_PRIV_RWX, UINT_MAX-(VIN_DEFAULT_ADDRESS+VIN_CONTROL_REGS_SIZE)-1);
      icmConnectMemoryToBus(bus1, "port2", mem[1], VIN_DEFAULT_ADDRESS+VIN_CONTROL_REGS_SIZE);
    }
    icmConnectMemoryToBus(bus1, "port1", mem[0], 0);

    if( options.display ) {
      icmAttrListP pseAttrs = icmNewAttrList();
      icmAddBoolAttr(pseAttrs, "bigEndianGuest", 1);
      icmAddStringAttr(pseAttrs, "output", options.display);
      //icmAddUns32Attr(pseAttrs, "polledRedraw", DVI_REDRAW_PSE);

      icmPseP dvi = icmNewPSE( "dvi", "../../xilinx-dvi/pse/pse.pse", pseAttrs, 0, 0 );
      icmConnectPSEBus( dvi, bus1, DVI_REGS_BUS_NAME, 0, DVI_BASE_ADDRESS, DVI_BASE_ADDRESS+DVI_CONTROL_REGS_SIZE-1 );
      icmConnectPSEBusDynamic( dvi, bus1, DVI_VMEM_BUS_NAME, 0 );

      //load semihost library
      icmAddPseInterceptObject( dvi, "dvi", "../../xilinx-dvi/model/model.so", 0, 0);
    }

    if( options.capture ) {
      icmAttrListP pseAttrs = icmNewAttrList();
      icmAddBoolAttr(pseAttrs, "bigEndianGuest", 1);
      icmAddBoolAttr(pseAttrs, "scale", 1);
      icmAddStringAttr(pseAttrs, "device", options.capture);
      icmAddBoolAttr(pseAttrs, "byteswap", 1);

      icmPseP vin = icmNewPSE( "vin", "../../xilinx-vgain/pse/pse.pse", pseAttrs, 0, 0 );
      icmConnectPSEBus( vin, bus1, VIN_REGS_BUS_NAME, 0, VIN_DEFAULT_ADDRESS, VIN_DEFAULT_ADDRESS+VIN_CONTROL_REGS_SIZE-1);
      icmConnectPSEBusDynamic( vin, bus1, VIN_VMEM_BUS_NAME, 0 );

      //load semihost library
      icmAddPseInterceptObject( vin, "vin", "../../xilinx-vgain/model/model.so", 0, 0);
    }
  } else if( options.memorySize != UINT_MAX ) {
    icmBusP bus1 = icmNewBus( "bus1", 32 );
    icmMemoryP mem1 = icmNewMemory( "mem1", ICM_PRIV_RWX, options.memorySize * 1024 * 1024 - 1 );
    icmConnectProcessorBusses( processor1, bus1, bus1 );
    icmConnectMemoryToBus( bus1, "port1", mem1, 0 );
  }

  if( options.wallclockFactor )
    icmSetWallClockFactor(options.wallclockFactor);

  icmLoadProcessorMemory(processor1, options.program, ICM_LOAD_DEFAULT, False, True);

  //do by-instruction simulation if necessary
  if( options.instructionCount != INSTRUCTION_COUNT_DEFAULT ) {
    icmStopReason stopReason = 0;
    while( ( stopReason = icmSimulate( processor1, options.instructionCount ) ) != ICM_SR_EXIT ) {
      switch( stopReason ) {
        case ICM_SR_SCHED : {
          //instructionCount instructions done, nothing special to do
          fprintf( stdout, "Simulation scheduling interrupt\n" );
          //TODO schedule PSE
          break;
        }
        case ICM_SR_WATCHPOINT : {
          icmWatchPointP triggeredWatchpoint = 0;
          while( ( triggeredWatchpoint = icmGetNextTriggeredWatchPoint() ) != 0 ) {
            fprintf( stderr, "Unhandled icmWatchPoint, ignoring!\n" );
          }
          break;
        }
        case ICM_SR_FINISH :
          fprintf( stdout, "Simulation finished\n" );
          stopReason = ICM_SR_EXIT; //to exit loop
          break;
        case ICM_SR_EXIT :
          fprintf( stdout, "Simulation exited\n" );
          break;
        case ICM_SR_INTERRUPT : {
          fprintf( stdout, "Simulation interrupted\n" );
          stopReason = ICM_SR_EXIT; //to exit loop
          break;
        }
        default :
          fprintf( stderr, "Unhandled icmStopReason!\n" );
      }
    }
  }
  else
    icmSimulatePlatform();

  icmTerminate();
  return 0;
}
