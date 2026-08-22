#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float temperature;
    float humidity;
    uint16_t lightRaw;
    uint8_t lightPercent;
    bool dhtValid;
    bool lightValid;
} SensorData;

typedef enum
{
    SYSTEM_STATUS_GOOD,
    SYSTEM_STATUS_WARNING,
    SYSTEM_STATUS_BAD,
    SYSTEM_STATUS_ERROR
} SystemStatus;

#endif
