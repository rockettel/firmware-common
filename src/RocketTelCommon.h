#ifndef __ROCKTTEL_COMMON__
#define __ROCKETTEL_COMMON__

#include <Arduino.h>
#include <ArduinoJson.h>


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
    /*
        template <uint32_t offset, uint32_t count>
        inline uint32_t ReadBits(const uint8_t *src, uint32_t accum);
    */
    public:
        DataPacket(uint32_t size);
        DataPacket(String str);
        ~DataPacket();
        
        
        
//        uint32_t readBitsInt(uint8_t bits);
        
//        void unpackToJSON(DynamicJsonDocument output);
};

#endif
