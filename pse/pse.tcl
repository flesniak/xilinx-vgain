imodelnewperipheral -name videoin -vendor itiv.kit.edu -library peripheral -version 1.0 -constructor constructor -destructor destructor
iadddocumentation   -name Licensing   -text "Open Source Apache 2.0"
iadddocumentation   -name Description -text "V4L2 to OVP video input peripheral"

  imodeladdbusslaveport -name CFGBUS -size 8
  imodeladdaddressblock -name AB0  -width 32 -offset 0 -size 8
  imodeladdmmregister   -name AR   -offset 0  -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 0
    imodeladdfield        -name BASEADDR -bitoffset 0 -width 32 -access rw -mmregister CFGBUS/AB0/AR
      iadddocumentation -name Description -text "Framebuffer target address in host memory"
  imodeladdmmregister   -name IR   -offset 0  -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 1
    imodeladdfield        -name BASEADDR -bitoffset 0 -width 1 -access rw -mmregister CFGBUS/AB0/IR
      iadddocumentation -name Description -text "Set to request a picture from input, cleared by hardware"

  imodeladdnetport -name VINSYNCINT -type output

  imodeladdbusslaveport -name VINMEMBUS -size 0x12c000 -remappable -mustbeconnected
    iadddocumentation -name Description -text "Video memory, sized 640x480x4Byte=1228800Byte"

  imodeladdformal -name "bigEndianGuest" -type bool
  imodeladdformal -name "scale" -type bool
  imodeladdformal -name "device" -type string
  imodeladdformal -name "byteswap" -type bool
