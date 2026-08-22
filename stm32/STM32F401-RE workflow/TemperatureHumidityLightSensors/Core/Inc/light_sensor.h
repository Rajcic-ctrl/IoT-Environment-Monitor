#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

void LightSensor_Init(ADC_HandleTypeDef *hadc);

bool LightSensor_StartMeasurement(void);

bool LightSensor_IsReady(void);

uint16_t LightSensor_GetRaw(void);

void LightSensor_HandleAdcComplete(ADC_HandleTypeDef *hadc);

uint8_t LightSensor_RawToPercent(uint16_t raw);
#endif
