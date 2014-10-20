imodelnewperipheral -name videoin -vendor itiv.kit.edu -library peripheral -version 1.0 -constructor constructor -destructor destructor
iadddocumentation   -name Licensing   -text "Open Source Apache 2.0"
iadddocumentation   -name Description -text "V4L2 to OVP video input peripheral"

  imodeladdbusslaveport -name CFGBUS -size 28
  imodeladdaddressblock -name AB0  -width 32 -offset 0 -size 28
  imodeladdmmregister   -name CONTROL         -offset 0  -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 0
    imodeladdfield        -name ENABLE           -bitoffset 0  -width 1  -access rw -mmregister CFGBUS/AB0/CONTROL
      iadddocumentation -name Description -text "Device enable, usually set to 1 once on startup"
    imodeladdfield        -name INTERLACE_ENABLE -bitoffset 1  -width 1  -access rw -mmregister CFGBUS/AB0/CONTROL
      iadddocumentation -name Description -text "Interlace enable, ineffective in this implemtation due to v4l2 always delivering full frames"
    imodeladdfield        -name CAPTURE_ENABLE   -bitoffset 3  -width 1  -access rw -mmregister CFGBUS/AB0/CONTROL
      iadddocumentation -name Description -text "Capture MAX_FRAME_INDEX+1 frames to memory"
    imodeladdfield        -name SYNC_POLARITY    -bitoffset 4  -width 1  -access rw -mmregister CFGBUS/AB0/CONTROL
      iadddocumentation -name Description -text "Sync polarity. Does nothing."
    imodeladdfield        -name LAST_FRAME_DONE  -bitoffset 15 -width 1  -access rw -mmregister CFGBUS/AB0/CONTROL
      iadddocumentation -name Description -text "Indicates whether all MAX_FRAME_INDEX+1 frames are captured"
    imodeladdfield        -name FRAME_INDEX      -bitoffset 16 -width 16 -access rw -mmregister CFGBUS/AB0/CONTROL
      iadddocumentation -name Description -text "Index of last completely captured frame"

  imodeladdmmregister   -name XSIZE           -offset 4  -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 1
    imodeladdfield        -name XSIZE            -bitoffset 0  -width 32  -access rw -mmregister CFGBUS/AB0/XSIZE
      iadddocumentation -name Description -text "Line length, will always return 640 in this implementation"

  imodeladdmmregister   -name YSIZE           -offset 8  -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 2
    imodeladdfield        -name YSIZE            -bitoffset 0  -width 32  -access rw -mmregister CFGBUS/AB0/YSIZE
      iadddocumentation -name Description -text "Frame height, will always return 480 in this implementation"

  imodeladdmmregister   -name STRIDE          -offset 12 -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 3
    imodeladdfield        -name YSIZE            -bitoffset 0  -width 32  -access rw -mmregister CFGBUS/AB0/STRIDE
      iadddocumentation -name Description -text "Line length in bytes"

  imodeladdmmregister   -name FRM0_BASEADDR   -offset 16 -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 4
    imodeladdfield        -name FRM0_BASEADDR    -bitoffset 0  -width 32  -access rw -mmregister CFGBUS/AB0/FRM0_BASEADDR
      iadddocumentation -name Description -text "Base address of first frame"

  imodeladdmmregister   -name FRM_ADDROFFSET  -offset 20 -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 5
    imodeladdfield        -name FRM_ADDROFFSET   -bitoffset 0  -width 32  -access rw -mmregister CFGBUS/AB0/FRM_ADDROFFSET
      iadddocumentation -name Description -text "Address offset between two frames, usually 0"

  imodeladdmmregister   -name MAX_FRAME_INDEX -offset 24 -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 6
    imodeladdfield        -name MAX_FRAME_INDEX -bitoffset 0 -width 32 -access rw -mmregister CFGBUS/AB0/MAX_FRAME_INDEX
      iadddocumentation -name Description -text "Maximum frame index, will capture this+1 frames"

  imodeladdnetport -name VINSYNCINT -type output

  imodeladdbusslaveport -name VINMEMBUS -size 0xbb8000 -remappable -mustbeconnected
    iadddocumentation -name Description -text "Video memory, sized VIN_DEFAULT_MAX_FRAME_INDEX=10 * 640x480x4Byte=10*1228800Byte"

  imodeladdformal -name "bigEndianGuest" -type bool
  imodeladdformal -name "scale" -type bool
  imodeladdformal -name "device" -type string
  imodeladdformal -name "byteswap" -type bool
