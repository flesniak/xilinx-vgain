#ifndef BYTESWAP_H
#define BYTESWAP_H

unsigned int bswap_32(unsigned int v) {
    return ((v>>24)&0x000000ff) | // move byte 3 to byte 0
           ((v<<8) &0x00ff0000) | // move byte 1 to byte 2
           ((v>>8) &0x0000ff00) | // move byte 2 to byte 1
           ((v<<24)&0xff000000); // byte 0 to byte 3
}

#endif //BYTESWAP_H
