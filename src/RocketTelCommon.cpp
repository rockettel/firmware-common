#include "RocketTelCommon.h"
    
DataPacket::DataPacket(uint32_t size) {
    _buffer = new uint8_t(size);
    _size = size;
    _endbit = 0;
}

DataPacket::DataPacket(String str) {
    _buffer = new uint8_t(str.length());
    _size = str.length();
    _endbit = _size*8;
}

DataPacket::~DataPacket() {
    if (_buffer != NULL) {
        free(_buffer);
    }
}

/*
template <unsigned offset, unsigned count>
inline uint32_t ReadBits(const uint8_t *src, uint32_t accum = 0)
{
    // Base of recursion
    if (count == 0)
        return accum;
}

uint32_t
readBitsInt(uint8_t bits) {
    

}
*/

//void
//unpackToJSON(DynamicJsonDocument output) {
/*
    uint8_t hdrByte1 = readBitsInt(8);
    uint8_t version = readBitsInt(8);
    uint8_t hdrByte2 = readBitsInt(8);
*/
//}


