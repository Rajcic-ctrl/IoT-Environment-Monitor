#ifndef DHT22_H
#define DHT22_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    DHT22_STATE_IDLE,
    DHT22_STATE_START_LOW,
    DHT22_STATE_START_HIGH,
    DHT22_STATE_RECEIVING,
    DHT22_STATE_COMPLETE,
    DHT22_STATE_ERROR
} DHT22_State;

typedef struct
{
    float temperature;
    float humidity;
    bool valid;
} DHT22_Data;

void DHT22_Init(TIM_HandleTypeDef *htim);

bool DHT22_StartMeasurement(void);

void DHT22_Process(void);

void DHT22_HandleCapture(TIM_HandleTypeDef *htim);

bool DHT22_IsReady(void);

DHT22_Data DHT22_GetData(void);

#endif
