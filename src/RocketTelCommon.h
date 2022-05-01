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

#define ROCKETTEL_RT_HDR_BITS 6
#define ROCKETTEL_ROCKETID_BITS 6
#define ROCKETTEL_ROCKETGRP_BITS 6

struct rt_data_type {
    uint8_t header;
    const char *name;
    uint8_t version;
    uint8_t num_values;
    uint8_t format;
    uint8_t bits;
    int32_t i_min;
    int32_t i_max;
    float f_min;
    float f_max;
};

struct rt_cmd_value {
    uint8_t header;
    uint32_t i_value;
    float f_value;
};

#define ROCKETTEL_RT_FMT_BOOL 0x01
#define ROCKETTEL_RT_FMT_UINT 0x02
#define ROCKETTEL_RT_FMT_INT  0x03
#define ROCKETTEL_RT_FMT_FLOAT 0x04

// Not really "official" or set in stone but:
// 0x0 is reserved
// 0x1-0x1F for "onboard" stuff
#define HEADER_BATTERY_LEVEL 0x01
#define HEADER_BATTERY_CAPACITY 0x02
#define HEADER_BATTERY_VOLTAGE 0x03
// 0x20-0x3F for "outside" stuff
#define HEADER_GPS  0x20
#define HEADER_ACCELEROMETER 0x21
#define HEADER_OUTSIDE_TPH  0x22 // Temp/Pressure/Humidity.  BME/BMP280.

#define CMD_HEADER_FLIGHTMODE  0x01
#define CMD_HEADER_SETGROUPID  0x02
#define CMD_HEADER_SETROCKETID 0x03
#define CMD_HEADER_PIN         0x04
#define CMD_HEADER_SETPIN      0x05

#ifdef __ROCKETTEL_COMMON_CPP__
/* Putting this here because it makes more sense, though it belongs in the cpp */
struct rt_data_type rt_data_types[] = {
    {HEADER_BATTERY_LEVEL, "batteryLevel", 0x01, 1, 7, HEADER_RT_FMT_UINT, 0, 100, 0.0, 0.0},
    {HEADER_BATTERY_VOLTAGE, "batteryVoltage", 0x01, 1, 7, HEADER_RT_FMT_FLOAT, 0, 0, 2.6, 4.3}
};

struct rt_data_type rt_cmd_data_types[] = {
    {CMD_HEADER_FLIGHTMODE, "flightMode", 0x01, 1, 1, HEADER_RT_FMT_BOOL, 0, 0, 0.0, 0.0},
    {CMD_HEADER_TEST, "setRocketId", 0x01, 1, ROCKETTEL_ROCKETID_BITS, HEADER_RT_FMT_UINT, 0, 0, 0.0, 0.0},
    {CMD_HEADER_TEST, "setGroupId", 0x01, 1, ROCKETTEL_GROUPID_BITS, HEADER_RT_FMT_UINT, 0, 0, 0.0, 0.0},
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
        int32_t unpackFromBaseStation(uint8_t groupId, uint8_t rocketId, 
                            struct rt_cmd_value *vals, size_t &nvals);

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
