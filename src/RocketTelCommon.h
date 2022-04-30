#ifndef __ROCKTTEL_COMMON_H__
#define __ROCKETTEL_COMMON_H__

#include <Arduino.h>

#ifdef ROCKETTEL_BASESTATION
#include <ArduinoJson.h>
#endif

#ifdef ROCKETTEL_AVPACK
#include <TinyGPSPlus.h>
#endif

#define RETVAL_OK       0
#define RETVAL_ERR      -1
#define RETVAL_IGNORE   -2
#define RETVAL_FATAL    -3

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

#define CMD_HEADER_FLIGHTMODE  0x01
#define CMD_HEADER_ID          0x02
#define CMD_HEADER_TEST        0x03

#ifdef __ROCKETTEL_COMMON_CPP__
struct rt_data_type rt_data_types[] = {
    {HEADER_BATTERY_LEVEL, "battery_level", 0x01, 1, 7, 0},
};

struct rt_data_type rt_cmd_data_types[] = {
    {CMD_HEADER_FLIGHTMODE, "flightMode", 0x01, 1, 1, 0},
    {CMD_HEADER_TEST, "testData", 0x01, 1, 1, 0},
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
        void unpackGPS(JsonDocument &output, int version);
        void unpackTPH(JsonDocument &output, int version);
        void unpackFlags(JsonDocument &output, int version);
#endif
        inline uint64_t readBits(uint32_t count, uint64_t accum);
        inline void writeBits(uint32_t count, uint64_t value);
    public:
        DataPacket();
        DataPacket(uint8_t *buffer, size_t length);
        ~DataPacket();
        
        
        uint32_t readBitsInt(uint8_t bits);
        void writeBitsInt(uint8_t bits, uint32_t value);
       
        bool readBool();
        void writeBool(bool value);

        float readFloat(float min, float max, uint8_t bits);
        void writeFloat(float val, float min, float max, uint8_t bits);
        
        void getBuffer(uint8_t *buffer, size_t *length);

        void initHeader(uint8_t groupId, uint8_t rocketId);

#ifdef ROCKETTEL_AVPACK
        int32_t unpackFromBaseStation(uint8_t groupId, uint8_t rocketId);

        void packFlags(bool flightMode);
        void packTPHData(float temperature, float pressure, float humidity);
        void packGPSData(TinyGPSPlus gps);
#endif

#ifdef ROCKETTEL_BASESTATION
        int32_t packToRocket(JsonDocument &input);
        int32_t unpackToJSON(JsonDocument &output);
#endif
};

#endif
