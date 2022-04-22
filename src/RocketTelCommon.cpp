#include "RocketTelCommon.h"
    
DataPacket::DataPacket(uint32_t size) {
    _buffer = new uint8_t(size);
    _size = size;
    bzero(_buffer, _size);
    _endbit = 0;
}

DataPacket::DataPacket(uint8_t *buffer, uint32_t length) {
    _buffer = new uint8_t(length);
    _size = length;
    memcpy(_buffer, buffer, _size);
    _endbit = _size*8;
}

DataPacket::~DataPacket() {
    if (_buffer != NULL) {
        free(_buffer);
    }
}


inline uint32_t 
DataPacket::readBits(uint32_t count, uint32_t accum = 0)
{
    // Base of recursion
    if (count == 0)
        return accum;
   // Look at the source, how many bits there left in the current byte
    const uint8_t *cur_src = _buffer + _curbit / 8;
    const unsigned bits_left = 8 - _curbit % 8;
    const uint8_t cur_data = *cur_src << (8 - bits_left);

    // How many bits we need and can write now
    const unsigned bits_to_use = bits_left < count ? bits_left : count;

    // Write the desired bits to the accumulator
    accum <<= bits_to_use;
    uint8_t mask = (1 << bits_to_use) - 1;
    const unsigned off = 8 - bits_to_use;
    accum |= (cur_data & (mask << off)) >> off;

    // Tail-recurse into the rest of required bits
    _curbit += bits_to_use;
    return readBits(count - bits_to_use, accum);
}

inline void 
DataPacket::writeBits(uint32_t count, uint32_t value)
{
    // Recursion base
    if (count == 0)
        return;

    // How many bits there left in the current target byte
    uint8_t *cur_dst = _buffer + _curbit / 8;
    const unsigned bits_left = 8 - _curbit % 8;

    // How many bits are necessary and the mask for that count of bits
    const unsigned bits_to_use = bits_left < count ? bits_left : count;
    const uint8_t mask = (1 << bits_to_use) - 1;

    // The desired range of bits from the source value
    const unsigned value_off = count - bits_to_use;
    const uint8_t value_bits = (value >> value_off) & mask;

    // Writing the bits to the destination
    uint8_t cur_data = *cur_dst;
    const unsigned off = bits_left - bits_to_use;
    cur_data &= ~(mask << off);
    cur_data |= value_bits << off;
    *cur_dst = cur_data;

    _curbit += bits_to_use;
    if (_endbit < _curbit) {
        _endbit = _curbit;
    }
    return writeBits(count - bits_to_use, value);
}

uint32_t
DataPacket::readBitsInt(uint8_t bits) {
    uint32_t ret = readBits(bits, 0);
    return ret;
}

void
DataPacket::writeBitsInt(uint8_t bits, uint32_t value) {
    writeBits(bits, value);
    if (_endbit < _curbit) {
        _endbit = _curbit;
    }
}

void 
DataPacket::getBuffer(uint8_t *buffer, uint32_t *length) {
    *length = _endbit/8;
    if (_endbit % 8) { *length += 1; }
    memcpy(buffer, _buffer, *length);
}

void
DataPacket::initHeader() {
    writeBitsInt(4, ROCKETTEL_HEADER4_1);
    writeBitsInt(8, ROCKETTEL_VERSION);
    writeBitsInt(4, ROCKETTEL_HEADER4_2);
}

#ifdef ROCKETTEL_BASESTATION
void
DataPacket::unpackToJSON(DynamicJsonDocument &output) {
    uint8_t hdr4_1 = readBitsInt(4);
    uint8_t version = readBitsInt(8);
    uint8_t hdr4_2 = readBitsInt(4);
    if (hdr4_1 != ROCKETTEL_HEADER4_1 || hdr4_2 != ROCKETTEL_HEADER4_2) {
        // fixme error logging
        return;
    }
    if (version > ROCKETTEL_VERSION) {
        // fixme error logging
        return;
    }
    
}
#endif


