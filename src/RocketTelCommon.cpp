#define __ROCKETTEL_COMMON_CPP__
#include "RocketTelCommon.h"
    
DataPacket::DataPacket() {
    _writeable = true;
    bzero(_buffer, RT_BUF_SIZE);
    _endbit = 0;
}

DataPacket::DataPacket(uint8_t *buffer, uint32_t length) {
    _writeable = false;
    bzero(_buffer, RT_BUF_SIZE);
    if (length > RT_BUF_SIZE) {
        length = RT_BUF_SIZE;
    }
    memcpy(_buffer, buffer, length);
    _endbit = length*8;
}

DataPacket::~DataPacket() {
    /*
    if (_buffer != NULL) {
        delete[] _buffer;
    }
    */
}


inline uint64_t 
DataPacket::readBits(uint32_t count, uint64_t accum = 0)
{
    if (_writeable) { return 0; }

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
DataPacket::writeBits(uint32_t count, uint64_t value)
{
    if (!_writeable) { return; }

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
}

void 
DataPacket::getBuffer(uint8_t *buffer, uint32_t *length) {
    *length = _endbit/8;
    if (_endbit % 8) { *length += 1; }
    memcpy(buffer, _buffer, *length);
}

void
DataPacket::initHeader(uint8_t group, uint8_t id) {
    writeBitsInt(4, ROCKETTEL_HEADER4_1);
    writeBitsInt(8, ROCKETTEL_VERSION);
    writeBitsInt(4, ROCKETTEL_HEADER4_2);
    writeBitsInt(6, group);
    writeBitsInt(6, id);

}

const int latVals = int(pow(2.0, 23.5));
const int lonVals = int(pow(2.0, 24.5));

#ifdef ROCKETTEL_AVPACK
void
DataPacket::packGPSData(TinyGPSPlus gps) {
    double lat = gps.location.lat();
    double lon = gps.location.lng();
    int64_t ulat=(int64_t(lat*latVals/180)+latVals/2)%latVals;
    int64_t ulon=(int64_t(lon*lonVals/360)+lonVals/2)%lonVals;
    writeBits(6, HEADER_GPS);
    writeBits(48, (ulat*lonVals)+ulon);
}


// 10 bits apiece.  We ought to revisit.
const int tempBits = 10;
const int pressBits = 10;
void
DataPacket::packTPHData(float temperature, float pressure, float humidity) {
    // no humidity for now
    // packing specific to a BMP280.   FIXME?  It's an OK range, though.
    // if somebody starts launching in Antarctica we'll do something
    // temp range -40 - 85.  Res 0.01
    // pressure range 300 - 1100.  Res 0.01. 
    if (temperature < -40.0) { temperature = -40.0; }
    if (temperature > 85.0) { temperature = 80.0; }
    if (pressure < 300) { pressure = 300.0; }
    if (pressure > 1100) { pressure = 1100.0; }
    temperature += 40.0; // now to 0.0-125
    temperature /= 125.0; // now to 0.0-0.1
    temperature *= pow(2.0, tempBits); // now to bitrange above.

    pressure -= 300; // now to 0-800
    pressure /= 800.0;
    pressure *= pow(2.0, pressBits);

    writeBits(6, HEADER_OUTSIDE_TPH);
    writeBits(tempBits, (int)temperature);
    writeBits(pressBits, (int)pressure);
}
#endif




#ifdef ROCKETTEL_BASESTATION
void 
DataPacket::unpackGPS(JsonDocument &output) {
    int64_t c = readBits(48);
    int64_t ulat=(c/lonVals-latVals/2)*180;
    int64_t ulon=(c%lonVals-lonVals/2)*360;
    
    output["rocket"]["gps"]["lat"] = (double)ulat/(double)latVals;
    output["rocket"]["gps"]["lon"] = (double)ulon/(double)lonVals;
}

void 
DataPacket::unpackTPH(JsonDocument &output) {
    int32_t temp_i = readBits(tempBits);
    int32_T press_i = readBits(pressBits);

    float temperature = temp_i / pow(2.0, tempBits);
    float pressure = press_i / pow(2.0, pressBits);
    temperature *= 125.0;
    pressure *= 800.0;   

    temperature -= 40.0;
    pressure += 300.0;

    output["rocket"]["outside_temp"] = temperature;
    output["rocket"]["outside_pressure"] = pressure;
}

void
DataPacket::unpackToJSON(JsonDocument &output) {
    uint8_t hdr4_1 = readBitsInt(4);
    uint8_t version = readBitsInt(8);
    uint8_t hdr4_2 = readBitsInt(4);
    if (hdr4_1 != ROCKETTEL_HEADER4_1 || hdr4_2 != ROCKETTEL_HEADER4_2) {
        output["rocket"]["error"] = "Not a RocketTel packet";
        return;
    }
    output["rocket"]["groupId"] = readBitsInt(6);
    output["rocket"]["rocketId"] = readBitsInt(6);
    if (version > ROCKETTEL_VERSION) {
        output["rocket"]["error"] = "RocketTel AVPack too new";
        return;
    }


    while((_endbit-_curbit) > 6) {
        uint8_t hdr = readBitsInt(6);
        if (hdr == 0x0) {
            return;
        }

        switch(hdr) {
        case HEADER_GPS:
            unpackGPS(output);
            break;
        case HEADER_OUTSIDE_TPH:
            unpackTPH(output);
            break;
        default:
            for (int i=0; i<(sizeof(rt_data_types)/sizeof(struct rt_data_type)); i++) {
                if (hdr == rt_data_types[i].header) {

                }
            }
            break;
        }
    }


    
}
#endif


