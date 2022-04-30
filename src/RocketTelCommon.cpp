#define __ROCKETTEL_COMMON_CPP__
#include "RocketTelCommon.h"
    
DataPacket::DataPacket() {
    _writeable = true;
    bzero(_buffer, RT_BUF_SIZE);
    _endbit = 0;
}

DataPacket::DataPacket(uint8_t *buffer, size_t length) {
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

bool
DataPacket::readBool() {
    uint32_t ret = readBits(1, 0);
    if (ret == 1) {
        return true;
    }
    return false;
}

void
DataPacket::writeBool(bool value) {
    if (value) { writeBits(1,1); } else { writeBits(1,0); }
}

float 
DataPacket::readFloat(float min, float max, uint8_t bits) {
    int32_t temp_i = readBits(bits);
    float val = (float)temp_i / pow(2.0, bits);
    val *= (max-min);
    val += min;
    return val;
}


void 
DataPacket::writeFloat(float val, float min, float max, uint8_t bits) {
    if (val < min) { val = min; }
    if (val > max) { val = max; }
    val -= min; 
    val /= (max-min); 
    val *= pow(2.0, bits); // now to bitrange above.
    writeBits(bits, val);
}


void 
DataPacket::getBuffer(uint8_t *buffer, size_t *length) {
    *length = _endbit/8;
    if (_endbit % 8) { *length += 1; }
    memcpy(buffer, _buffer, *length);
}

void
DataPacket::initHeader(uint8_t groupId, uint8_t rocketId) {
#ifdef ROCKETTEL_AVPACK
    writeBitsInt(4, ROCKETTEL_HEADER4_1);
    writeBitsInt(8, ROCKETTEL_VERSION);
    writeBitsInt(4, ROCKETTEL_HEADER4_2);
#else // BASESTATION
    writeBitsInt(4, ROCKETTEL_HEADER4_2);
    writeBitsInt(8, ROCKETTEL_VERSION);
    writeBitsInt(4, ROCKETTEL_HEADER4_1);
#endif
    writeBitsInt(6, groupId);
    writeBitsInt(6, rocketId);

}

const int latVals = int(pow(2.0, 23.5));
const int lonVals = int(pow(2.0, 24.5));
// 10 bits apiece.  We ought to revisit.
const int tempBits = 10;
const int pressBits = 10;

#ifdef ROCKETTEL_AVPACK

void 
DataPacket::packFlags(bool flightMode) {
    writeBool(flightMode);
}

void
DataPacket::packGPSData(TinyGPSPlus gps) {
    float lat = gps.location.lat();
    float lon = gps.location.lng();
    int64_t ulat=(int64_t(lat*latVals/180)+latVals/2)%latVals;
    int64_t ulon=(int64_t(lon*lonVals/360)+lonVals/2)%lonVals;
    writeBits(6, HEADER_GPS);
    writeBits(48, (ulat*lonVals)+ulon);
    writeBitsInt(15, gps.altitude.meters());
    writeBitsInt(4, gps.satellites.value());
}

void
DataPacket::packTPHData(float temperature, float pressure, float humidity) {
    // no humidity for now
    // packing specific to a BMP280.   FIXME?  It's an OK range, though.
    // if somebody starts launching in Antarctica we'll do something
    // temp range -40 - 85.  Res 0.01
    // pressure range 300 - 1100.  Res 0.01. 
    


    writeBits(6, HEADER_OUTSIDE_TPH);
    writeFloat(temperature, -40.0, 85.0, tempBits);
    writeFloat(pressure, 300.0, 1100.0, pressBits);
}

int32_t
DataPacket::unpackFromBaseStation(uint8_t myGroupId, uint8_t myRocketId,
                            struct rt_cmd_value *vals, size_t &nvals) {
    size_t maxvals = nvals;
    nvals = 0;
    uint8_t hdr4_1 = readBitsInt(4);
    uint8_t version = readBitsInt(8);
    uint8_t hdr4_2 = readBitsInt(4);
    if (hdr4_1 == ROCKETTEL_HEADER4_1 && hdr4_2 == ROCKETTEL_HEADER4_2) {
        /* This is another rocket talking to a basestation */
        return RETVAL_IGNORE;
    }

    if (hdr4_1 != ROCKETTEL_HEADER4_2 || hdr4_2 != ROCKETTEL_HEADER4_1) {
        return RETVAL_ERR;
    }
    uint8_t groupId = readBitsInt(6);
    uint8_t rocketId = readBitsInt(6);

    if (groupId != myGroupId || rocketId != myRocketId) {
        return RETVAL_IGNORE;
    }
    
    if (version > ROCKETTEL_VERSION) {
        return RETVAL_ERR;
    }

    while((_endbit-_curbit) > 6) {
        uint8_t hdr = readBitsInt(6);
        if (hdr == 0x0) {
          return RETVAL_OK;
        }

        switch(hdr) {
        default:
           for (int i=0; i<(sizeof(rt_cmd_data_types)/sizeof(struct rt_data_type)); i++) {
                struct rt_data_type cmd = rt_cmd_data_types[i];
                if (hdr == cmd.header) {
                    vals[nvals].header = hdr;
                    vals[nvals].i_value = readBits(cmd.bits);
                    nvals++;
                }
            }
            break;
        }
    }
    return RETVAL_OK;
}
#endif




#ifdef ROCKETTEL_BASESTATION
void 
DataPacket::unpackGPS(JsonDocument &output, int version) {
    int64_t c = readBits(48);
    int64_t ulat=(c/lonVals-latVals/2)*180;
    int64_t ulon=(c%lonVals-lonVals/2)*360;
    int64_t alt = readBitsInt(15);
    int64_t sats = readBitsInt(5);

    
    output["rocket"]["gps"]["lat"] = (float)ulat/(float)latVals;
    output["rocket"]["gps"]["lon"] = (float)ulon/(float)lonVals;
    output["rocket"]["gps"]["alt"] = alt;
    output["rocket"]["gps"]["satellites"] = sats;

}

void 
DataPacket::unpackTPH(JsonDocument &output, int version) {


    output["rocket"]["outsideTemp"] = readFloat(-40, 85, tempBits);
    output["rocket"]["outsidePressure"] = readFloat(300, 1100, pressBits);
}

void 
DataPacket::unpackFlags(JsonDocument &output, int version) {
    output["rocket"]["flightMode"] = readBool();
}

int32_t
DataPacket::packToRocket(JsonDocument &input) {
    if (!input.containsKey("groupId") || !input.containsKey("rocketId")) {
        return RETVAL_ERR;
    }
    int groupId = input["groupId"];
    int rocketId = input["rocketId"];

    initHeader(groupId, rocketId);

    
    for (int i=0; i<(sizeof(rt_cmd_data_types)/sizeof(struct rt_data_type)); i++) {
        struct rt_data_type rt_cmd = rt_cmd_data_types[i];

        if (input.containsKey(rt_cmd.name)) {
            switch(rt_cmd.bits) {
            case 1: { 
                bool val = input[rt_cmd.name];
                writeBits(6,rt_cmd.header);
                writeBool(val);
                break;
            }
            default:
                break;
            }
        }
    }
    writeBits(6, 0);
    
    return RETVAL_OK;

}

int32_t
DataPacket::unpackToJSON(JsonDocument &output) {
    uint8_t hdr4_1 = readBitsInt(4);
    uint8_t version = readBitsInt(8);
    uint8_t hdr4_2 = readBitsInt(4);
    if (hdr4_1 == ROCKETTEL_HEADER4_2 && hdr4_2 == ROCKETTEL_HEADER4_1) {
        /* This is another basestation talking to a rocket -- ignore */
        return RETVAL_IGNORE;
    }


    if (hdr4_1 != ROCKETTEL_HEADER4_1 || hdr4_2 != ROCKETTEL_HEADER4_2) {
        output["rocket"]["error"] = "Not a RocketTel packet";
        return RETVAL_ERR;
    }
    output["rocket"]["rocketId"]["group"] = readBitsInt(6);
    output["rocket"]["rocketId"]["rocket"] = readBitsInt(6);
    if (version > ROCKETTEL_VERSION) {
        output["rocket"]["error"] = "RocketTel AVPack too new";
        return RETVAL_ERR;
    }

    /*** Single-Bit FLAGS ***/
    unpackFlags(output, version);
    

    while((_endbit-_curbit) > 6) {
        uint8_t hdr = readBitsInt(6);
        if (hdr == 0x0) {
            return RETVAL_OK;
        }

        switch(hdr) {
        case HEADER_GPS:
            unpackGPS(output, version);
            break;
        case HEADER_OUTSIDE_TPH:
            unpackTPH(output, version);
            break;
        default:
            for (int i=0; i<(sizeof(rt_data_types)/sizeof(struct rt_data_type)); i++) {
                if (hdr == rt_data_types[i].header) {

                }
            }
            break;
        }
    }
    return RETVAL_OK;
}
#endif


