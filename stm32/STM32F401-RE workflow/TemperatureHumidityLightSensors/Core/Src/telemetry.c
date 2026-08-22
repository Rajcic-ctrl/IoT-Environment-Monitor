#include "telemetry.h"
#include <stdio.h>
#include <string.h>

#define TELEMETRY_BUFFER_SIZE 128

static UART_HandleTypeDef *telemetryUart = NULL;
static uint8_t txBuffer[TELEMETRY_BUFFER_SIZE];
static volatile bool txBusy = false;

static const char *Telemetry_GetStatusText(SystemStatus status)
{
    switch (status)
    {
        case SYSTEM_STATUS_GOOD: return "GOOD";
        case SYSTEM_STATUS_WARNING: return "WARNING";
        case SYSTEM_STATUS_BAD: return "BAD";
        case SYSTEM_STATUS_ERROR: return "ERROR";
        default: return "ERROR";
    }
}

void Telemetry_Init(UART_HandleTypeDef *huart)
{
    telemetryUart = huart;
    txBusy = false;
}

bool Telemetry_Send(const SensorData *data, SystemStatus status)
{
    if (telemetryUart == NULL || data == NULL || txBusy) return false;

    int32_t temperatureScaled = (int32_t)(data->temperature * 10.0f + (data->temperature >= 0.0f ? 0.5f : -0.5f));
    int32_t humidityScaled = (int32_t)(data->humidity * 10.0f + 0.5f);

    bool temperatureNegative = temperatureScaled < 0;
    uint32_t temperatureAbs = temperatureNegative ? (uint32_t)(-temperatureScaled) : (uint32_t)temperatureScaled;

    int length = snprintf((char *)txBuffer, TELEMETRY_BUFFER_SIZE,
        "{\"temperature\":%s%lu.%lu,\"humidity\":%ld.%ld,\"light\":%u,\"status\":\"%s\"}\r\n",
        temperatureNegative ? "-" : "",
        temperatureAbs / 10,
        temperatureAbs % 10,
        humidityScaled / 10,
        humidityScaled % 10,
        data->lightPercent,
        Telemetry_GetStatusText(status));

    if (length <= 0 || length >= TELEMETRY_BUFFER_SIZE) return false;

    txBusy = true;

    if (HAL_UART_Transmit_IT(telemetryUart, txBuffer, (uint16_t)length) != HAL_OK)
    {
        txBusy = false;
        return false;
    }

    return true;
}

bool Telemetry_IsBusy(void)
{
    return txBusy;
}

void Telemetry_HandleTxComplete(UART_HandleTypeDef *huart)
{
    if (huart != telemetryUart) return;
    txBusy = false;
}

void Telemetry_HandleError(UART_HandleTypeDef *huart)
{
    if (huart != telemetryUart) return;
    txBusy = false;
}
