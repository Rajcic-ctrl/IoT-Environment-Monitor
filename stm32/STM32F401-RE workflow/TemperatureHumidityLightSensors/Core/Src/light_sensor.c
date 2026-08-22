#include "light_sensor.h"

static ADC_HandleTypeDef *lightAdc = NULL;

static volatile uint16_t lightRaw = 0;

static volatile bool measurementReady = false;

static volatile bool measurementInProgress = false;

#define LIGHT_RAW_MIN 40
#define LIGHT_RAW_MAX 3200

void LightSensor_Init(ADC_HandleTypeDef *hadc)
{
    lightAdc = hadc;

    lightRaw = 0;

    measurementReady = false;

    measurementInProgress = false;
}

bool LightSensor_StartMeasurement(void)
{
    if (lightAdc == NULL)
    {
        return false;
    }

    if (measurementInProgress)
    {
        return false;
    }

    measurementReady = false;
    measurementInProgress = true;

    if (HAL_ADC_Start_IT(lightAdc) != HAL_OK)
    {
        measurementInProgress = false;

        return false;
    }

    return true;
}


void LightSensor_HandleAdcComplete(ADC_HandleTypeDef *hadc)
{
    if (hadc != lightAdc)
    {
        return;
    }

    lightRaw = (uint16_t) HAL_ADC_GetValue(hadc);

    measurementReady = true;
    measurementInProgress = false;
}


bool LightSensor_IsReady(void)
{
    return measurementReady;
}

uint16_t LightSensor_GetRaw(void)
{
	measurementReady = false;
    return lightRaw;
}


uint8_t LightSensor_RawToPercent(uint16_t raw)
{
    if (raw <= LIGHT_RAW_MIN) return 0;
    if (raw >= LIGHT_RAW_MAX) return 100;

    return (uint8_t)(((uint32_t)(raw - LIGHT_RAW_MIN) * 100) / (LIGHT_RAW_MAX - LIGHT_RAW_MIN));
}
