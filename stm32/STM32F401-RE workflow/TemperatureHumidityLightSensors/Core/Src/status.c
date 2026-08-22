#include "status.h"

#define TEMP_GOOD_MIN 18.0f
#define TEMP_GOOD_MAX 30.0f
#define TEMP_WARNING_MIN 15.0f
#define TEMP_WARNING_MAX 33.0f

#define HUMIDITY_GOOD_MIN 40.0f
#define HUMIDITY_GOOD_MAX 70.0f
#define HUMIDITY_WARNING_MIN 30.0f
#define HUMIDITY_WARNING_MAX 80.0f

#define LIGHT_BAD_MAX 5
#define LIGHT_WARNING_LOW_MAX 20
#define LIGHT_GOOD_MAX 90


static SystemStatus Status_EvaluateTemperature(float temperature)
{
    if (temperature < TEMP_WARNING_MIN || temperature > TEMP_WARNING_MAX) return SYSTEM_STATUS_BAD;
    if (temperature < TEMP_GOOD_MIN || temperature > TEMP_GOOD_MAX) return SYSTEM_STATUS_WARNING;

    return SYSTEM_STATUS_GOOD;
}


static SystemStatus Status_EvaluateHumidity(float humidity)
{
    if (humidity < HUMIDITY_WARNING_MIN || humidity > HUMIDITY_WARNING_MAX) return SYSTEM_STATUS_BAD;
    if (humidity < HUMIDITY_GOOD_MIN || humidity > HUMIDITY_GOOD_MAX) return SYSTEM_STATUS_WARNING;

    return SYSTEM_STATUS_GOOD;
}


static SystemStatus Status_EvaluateLight(uint8_t lightPercent)
{
    if (lightPercent <= LIGHT_BAD_MAX) return SYSTEM_STATUS_BAD;
    if (lightPercent <= LIGHT_WARNING_LOW_MAX) return SYSTEM_STATUS_WARNING;
    if (lightPercent <= LIGHT_GOOD_MAX) return SYSTEM_STATUS_GOOD;

    return SYSTEM_STATUS_WARNING;
}


SystemStatus Status_Evaluate(const SensorData *data)
{
    if (!data->dhtValid || !data->lightValid) return SYSTEM_STATUS_ERROR;

    SystemStatus temperatureStatus = Status_EvaluateTemperature(data->temperature);
    SystemStatus humidityStatus = Status_EvaluateHumidity(data->humidity);
    SystemStatus lightStatus = Status_EvaluateLight(data->lightPercent);

    if (temperatureStatus == SYSTEM_STATUS_BAD || humidityStatus == SYSTEM_STATUS_BAD || lightStatus == SYSTEM_STATUS_BAD) return SYSTEM_STATUS_BAD;
    if (temperatureStatus == SYSTEM_STATUS_WARNING || humidityStatus == SYSTEM_STATUS_WARNING || lightStatus == SYSTEM_STATUS_WARNING) return SYSTEM_STATUS_WARNING;

    return SYSTEM_STATUS_GOOD;
}
