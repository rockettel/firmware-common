#ifndef __ROCKTTEL_COMMON__
#define __ROCKETTEL_COMMON__

#include <Arduino.h>
#ifdef ROCKETTEL_BASESTATION
#include <ArduinoJson.h>
#endif


#define ROCKETTEL_HEADERBYTE1 0x3A // 00111010
#define ROCKETTEL_VERSION     0x01
#define ROCKETTEL_HEADERBYTE2 0xC5 // 11000101

class DataPacket {
    private:
        uint8_t *_buffer = NULL; 
        uint32 _size = 0;
        uint32 _endbit = 0;
        uint32 _curbit = 0;
        
    protected:

        inline uint32_t readBits(uint32_t count, uint32_t accum);
        inline void writeBits(uint32_t count, uint32_t value);
    public:
        DataPacket(uint32_t size);
        DataPacket(uint8_t *buffer, uint32_t length);
        ~DataPacket();
        
        
        
        uint32_t readBitsInt(uint8_t bits);
        void writeBitsInt(uint8_t bits, uint32_t value);
       
        void getBuffer(uint8_t *buffer, uint32_t *length);
#ifdef ROCKETTEL_BASESTATION
        void unpackToJSON(DynamicJsonDocument &output);
#endif
};

#endif
