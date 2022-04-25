#ifndef __ROCKTTEL_COMMON_H__
#define __ROCKETTEL_COMMON_H__

#include <Arduino.h>

#ifdef ROCKETTEL_BASESTATION
#include <ArduinoJson.h>
#endif

#ifdef ROCKETTEL_AVPACK
#include <TinyGPSPlus.h>
#endif



#define ROCKETTEL_HEADER4_1 0x3 // 00111010
#define ROCKETTEL_VERSION     0x01
#define ROCKETTEL_HEADER4_2 0xC // 11000101

struct rt_data_type {
    uint8_t header;
    const char *name;
    uint8_t version;
    uint8_t num_values;
    uint8_t bits;
    uint8_t divisor;
};


// Not really "official" or set in stone but:
// 0x0 is reserved
// 0x1-0x1F for "onboard" stuff
#define HEADER_BATTERY_LEVEL 0x01
#define HEADER_BATTERY_CAPACITY 0x02
// 0x20-0x3F for "outside" stuff
#define HEADER_GPS  0x20
#define HEADER_ACCELEROMETER 0x21
#define HEADER_OUTSIDE_TPH  0x22 // Temp/Pressure/Humidity.  BME/BMP280.


#ifdef __ROCKETTEL_COMMON_CPP__
struct rt_data_type rt_data_types[] = {
    {HEADER_BATTERY_LEVEL, "battery_level", 0x01, 1, 7, 0},
};
#endif

#define RT_BUF_SIZE 64

class DataPacket {
    private:
        uint8_t _buffer[RT_BUF_SIZE];
        uint32_t _endbit = 0;
        uint32_t _curbit = 0;
        bool _writeable;
        
    protected:
#ifdef ROCKETTEL_BASESTATION
        void unpackGPS(JsonDocument &output);
        void unpackTPH(JsonDocument &output);
#endif
        inline uint64_t readBits(uint32_t count, uint64_t accum);
        inline void writeBits(uint32_t count, uint64_t value);
    public:
        DataPacket();
        DataPacket(uint8_t *buffer, uint32_t length);
        ~DataPacket();
        
        
        uint32_t readBitsInt(uint8_t bits);
        void writeBitsInt(uint8_t bits, uint32_t value);
       
        void getBuffer(uint8_t *buffer, uint32_t *length);

        void initHeader(uint8_t group, uint8_t id);

#ifdef ROCKETTEL_AVPACK
        void packTPHData(float temperature, float pressure, float humidity);
        void packGPSData(TinyGPSPlus gps);
#endif

#ifdef ROCKETTEL_BASESTATION
        void unpackToJSON(JsonDocument &output);
#endif
};

#endif
