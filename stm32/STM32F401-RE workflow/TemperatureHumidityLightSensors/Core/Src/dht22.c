#include "dht22.h"
#include <string.h>

#define DHT22_START_LOW_TIME_US 1100
#define DHT22_START_HIGH_TIME_US 30
#define DHT22_RESPONSE_MIN_US 60
#define DHT22_RESPONSE_MAX_US 100
#define DHT22_BIT_ONE_THRESHOLD_US 50
#define DHT22_RECEIVE_TIMEOUT_US 10000

static TIM_HandleTypeDef *dhtTimer = NULL;
static volatile DHT22_State dhtState = DHT22_STATE_IDLE;

static DHT22_Data dhtData = {
    .temperature = 0.0f,
    .humidity = 0.0f,
    .valid = false
};

static volatile bool measurementReady = false;

static uint32_t stateStartTime = 0;
static uint8_t dhtBytes[5] = {0};
static uint8_t bitIndex = 0;
static uint32_t risingEdgeTime = 0;

typedef enum
{
    DHT22_CAPTURE_RESPONSE_RISING,
    DHT22_CAPTURE_RESPONSE_FALLING,
    DHT22_CAPTURE_BIT_RISING,
    DHT22_CAPTURE_BIT_FALLING
} DHT22_CaptureState;

static DHT22_CaptureState captureState = DHT22_CAPTURE_RESPONSE_RISING;


static void DHT22_SetPinOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}


static void DHT22_SetPinInputCapture(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}


static void DHT22_StopCapture(void)
{
    HAL_TIM_IC_Stop_IT(dhtTimer, TIM_CHANNEL_1);
    HAL_TIM_Base_Stop(dhtTimer);
}


static void DHT22_SetError(void)
{
    DHT22_StopCapture();

    dhtData.valid = false;
    measurementReady = true;
    dhtState = DHT22_STATE_IDLE;
}


static void DHT22_ProcessReceivedData(void)
{
    uint8_t checksum = (uint8_t)(dhtBytes[0] + dhtBytes[1] + dhtBytes[2] + dhtBytes[3]);

    if (checksum != dhtBytes[4])
    {
        dhtData.valid = false;
        measurementReady = true;
        dhtState = DHT22_STATE_IDLE;
        return;
    }

    uint16_t rawHumidity = ((uint16_t)dhtBytes[0] << 8) | dhtBytes[1];
    uint16_t rawTemperature = ((uint16_t)(dhtBytes[2] & 0x7F) << 8) | dhtBytes[3];

    dhtData.humidity = rawHumidity / 10.0f;
    dhtData.temperature = rawTemperature / 10.0f;

    if (dhtBytes[2] & 0x80) dhtData.temperature = -dhtData.temperature;

    dhtData.valid = true;
    measurementReady = true;
    dhtState = DHT22_STATE_IDLE;
}


void DHT22_Init(TIM_HandleTypeDef *htim)
{
    dhtTimer = htim;
    dhtState = DHT22_STATE_IDLE;

    dhtData.temperature = 0.0f;
    dhtData.humidity = 0.0f;
    dhtData.valid = false;

    measurementReady = false;
    stateStartTime = 0;
    bitIndex = 0;
    risingEdgeTime = 0;

    memset(dhtBytes, 0, sizeof(dhtBytes));
}


bool DHT22_StartMeasurement(void)
{
    if (dhtTimer == NULL) return false;
    if (dhtState != DHT22_STATE_IDLE) return false;

    measurementReady = false;
    dhtData.valid = false;

    memset(dhtBytes, 0, sizeof(dhtBytes));

    bitIndex = 0;
    risingEdgeTime = 0;
    captureState = DHT22_CAPTURE_RESPONSE_RISING;

    DHT22_SetPinOutput();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

    __HAL_TIM_SET_COUNTER(dhtTimer, 0);
    HAL_TIM_Base_Start(dhtTimer);

    stateStartTime = __HAL_TIM_GET_COUNTER(dhtTimer);
    dhtState = DHT22_STATE_START_LOW;

    return true;
}


void DHT22_Process(void)
{
    if (dhtTimer == NULL) return;

    uint32_t now = __HAL_TIM_GET_COUNTER(dhtTimer);

    switch (dhtState)
    {
        case DHT22_STATE_IDLE:
            break;

        case DHT22_STATE_START_LOW:
            if ((uint16_t)(now - stateStartTime) >= DHT22_START_LOW_TIME_US)
            {
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
                stateStartTime = now;
                dhtState = DHT22_STATE_START_HIGH;
            }
            break;

        case DHT22_STATE_START_HIGH:
            if ((uint16_t)(now - stateStartTime) >= DHT22_START_HIGH_TIME_US)
            {
                DHT22_SetPinInputCapture();

                __HAL_TIM_SET_COUNTER(dhtTimer, 0);

                captureState = DHT22_CAPTURE_RESPONSE_RISING;

                __HAL_TIM_SET_CAPTUREPOLARITY(dhtTimer, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
                dhtState = DHT22_STATE_RECEIVING;

                if (HAL_TIM_IC_Start_IT(dhtTimer, TIM_CHANNEL_1) != HAL_OK)
                {
                    dhtState = DHT22_STATE_ERROR;
                }
            }
            break;

        case DHT22_STATE_RECEIVING:
            if (now >= DHT22_RECEIVE_TIMEOUT_US) dhtState = DHT22_STATE_ERROR;
            break;

        case DHT22_STATE_COMPLETE:
            HAL_TIM_Base_Stop(dhtTimer);
            DHT22_ProcessReceivedData();
            break;

        case DHT22_STATE_ERROR:
            DHT22_SetError();
            break;
    }
}


void DHT22_HandleCapture(TIM_HandleTypeDef *htim)
{
    if (htim != dhtTimer) return;
    if (dhtState != DHT22_STATE_RECEIVING) return;

    uint32_t capturedTime = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

    switch (captureState)
    {
        case DHT22_CAPTURE_RESPONSE_RISING:
            risingEdgeTime = capturedTime;

            __HAL_TIM_SET_CAPTUREPOLARITY(dhtTimer, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
            captureState = DHT22_CAPTURE_RESPONSE_FALLING;
            break;

        case DHT22_CAPTURE_RESPONSE_FALLING:
        {
            uint16_t responseHighTime = (uint16_t)(capturedTime - risingEdgeTime);

            if (responseHighTime < DHT22_RESPONSE_MIN_US || responseHighTime > DHT22_RESPONSE_MAX_US)
            {
                HAL_TIM_IC_Stop_IT(dhtTimer, TIM_CHANNEL_1);
                dhtState = DHT22_STATE_ERROR;
                break;
            }

            __HAL_TIM_SET_CAPTUREPOLARITY(dhtTimer, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
            captureState = DHT22_CAPTURE_BIT_RISING;
            break;
        }

        case DHT22_CAPTURE_BIT_RISING:
            risingEdgeTime = capturedTime;

            __HAL_TIM_SET_CAPTUREPOLARITY(dhtTimer, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
            captureState = DHT22_CAPTURE_BIT_FALLING;
            break;

        case DHT22_CAPTURE_BIT_FALLING:
        {
            uint16_t highTime = (uint16_t)(capturedTime - risingEdgeTime);
            uint8_t byteIndex = bitIndex / 8;

            dhtBytes[byteIndex] <<= 1;

            if (highTime > DHT22_BIT_ONE_THRESHOLD_US) dhtBytes[byteIndex] |= 1;

            bitIndex++;

            if (bitIndex >= 40)
            {
                HAL_TIM_IC_Stop_IT(dhtTimer, TIM_CHANNEL_1);
                dhtState = DHT22_STATE_COMPLETE;
                break;
            }

            __HAL_TIM_SET_CAPTUREPOLARITY(dhtTimer, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
            captureState = DHT22_CAPTURE_BIT_RISING;
            break;
        }
    }
}


bool DHT22_IsReady(void)
{
    return measurementReady;
}


DHT22_Data DHT22_GetData(void)
{
    measurementReady = false;
    return dhtData;
}
