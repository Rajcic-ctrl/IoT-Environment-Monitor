#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "main.h"
#include "sensor_data.h"
#include <stdbool.h>

void Telemetry_Init(UART_HandleTypeDef *huart);
bool Telemetry_Send(const SensorData *data, SystemStatus status);
bool Telemetry_IsBusy(void);
void Telemetry_HandleTxComplete(UART_HandleTypeDef *huart);
void Telemetry_HandleError(UART_HandleTypeDef *huart);

#endif
