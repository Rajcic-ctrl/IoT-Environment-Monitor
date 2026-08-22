#include "oled.h"
#include <string.h>

#define OLED_ADDRESS (0x3C << 1)
#define OLED_CONTROL_COMMAND 0x00
#define OLED_CONTROL_DATA 0x40

#define OLED_INIT_DELAY_MS 100
#define OLED_TX_CHUNK_SIZE 16

typedef enum
{
    OLED_STATE_OFF,
    OLED_STATE_WAIT_STARTUP,
    OLED_STATE_INITIALIZING,
    OLED_STATE_READY,
    OLED_STATE_SENDING_SETUP,
    OLED_STATE_SENDING_BUFFER,
    OLED_STATE_ERROR
} OLED_State;

static I2C_HandleTypeDef *oledI2c = NULL;
static OLED_State oledState = OLED_STATE_OFF;

static uint8_t oledBuffer[OLED_BUFFER_SIZE];
static uint8_t txBuffer[OLED_TX_CHUNK_SIZE + 1];

static uint32_t initStartTime = 0;
static uint8_t initIndex = 0;
static uint8_t setupIndex = 0;
static uint16_t bufferOffset = 0;

static volatile bool txComplete = false;
static volatile bool txError = false;

static const uint8_t oledInitSequence[] =
{
    0xAE,
    0xD5, 0x80,
    0xA8, 0x3F,
    0xD3, 0x00,
    0x40,
    0x8D, 0x14,
    0x20, 0x00,
    0xA1,
    0xC8,
    0xDA, 0x12,
    0x81, 0xCF,
    0xD9, 0xF1,
    0xDB, 0x40,
    0xA4,
    0xA6,
    0xAF
};

static const uint8_t oledUpdateSetup[] =
{
    0x21, 0x00, 0x7F,
    0x22, 0x00, 0x07
};

#define OLED_INIT_SEQUENCE_SIZE ((uint8_t)sizeof(oledInitSequence))
#define OLED_UPDATE_SETUP_SIZE ((uint8_t)sizeof(oledUpdateSetup))


static bool OLED_SendCommand(uint8_t command)
{
    if (oledI2c == NULL) return false;
    if (HAL_I2C_GetState(oledI2c) != HAL_I2C_STATE_READY) return false;

    txBuffer[0] = OLED_CONTROL_COMMAND;
    txBuffer[1] = command;

    txComplete = false;
    txError = false;

    return HAL_I2C_Master_Transmit_IT(oledI2c, OLED_ADDRESS, txBuffer, 2) == HAL_OK;
}


static bool OLED_SendBufferChunk(void)
{
    if (oledI2c == NULL) return false;
    if (HAL_I2C_GetState(oledI2c) != HAL_I2C_STATE_READY) return false;
    if (bufferOffset >= OLED_BUFFER_SIZE) return false;

    uint16_t remaining = OLED_BUFFER_SIZE - bufferOffset;
    uint16_t chunkSize = remaining > OLED_TX_CHUNK_SIZE ? OLED_TX_CHUNK_SIZE : remaining;

    txBuffer[0] = OLED_CONTROL_DATA;
    memcpy(&txBuffer[1], &oledBuffer[bufferOffset], chunkSize);

    txComplete = false;
    txError = false;

    if (HAL_I2C_Master_Transmit_IT(oledI2c, OLED_ADDRESS, txBuffer, chunkSize + 1) != HAL_OK) return false;

    bufferOffset += chunkSize;
    return true;
}


static void OLED_GetCharBitmap(char c, uint8_t bitmap[5])
{
    memset(bitmap, 0, 5);

    switch (c)
    {
        case '0': { uint8_t v[5] = {0x3E, 0x51, 0x49, 0x45, 0x3E}; memcpy(bitmap, v, 5); break; }
        case '1': { uint8_t v[5] = {0x00, 0x42, 0x7F, 0x40, 0x00}; memcpy(bitmap, v, 5); break; }
        case '2': { uint8_t v[5] = {0x42, 0x61, 0x51, 0x49, 0x46}; memcpy(bitmap, v, 5); break; }
        case '3': { uint8_t v[5] = {0x21, 0x41, 0x45, 0x4B, 0x31}; memcpy(bitmap, v, 5); break; }
        case '4': { uint8_t v[5] = {0x18, 0x14, 0x12, 0x7F, 0x10}; memcpy(bitmap, v, 5); break; }
        case '5': { uint8_t v[5] = {0x27, 0x45, 0x45, 0x45, 0x39}; memcpy(bitmap, v, 5); break; }
        case '6': { uint8_t v[5] = {0x3C, 0x4A, 0x49, 0x49, 0x30}; memcpy(bitmap, v, 5); break; }
        case '7': { uint8_t v[5] = {0x01, 0x71, 0x09, 0x05, 0x03}; memcpy(bitmap, v, 5); break; }
        case '8': { uint8_t v[5] = {0x36, 0x49, 0x49, 0x49, 0x36}; memcpy(bitmap, v, 5); break; }
        case '9': { uint8_t v[5] = {0x06, 0x49, 0x49, 0x29, 0x1E}; memcpy(bitmap, v, 5); break; }

        case 'A': { uint8_t v[5] = {0x7E, 0x11, 0x11, 0x11, 0x7E}; memcpy(bitmap, v, 5); break; }
        case 'B': { uint8_t v[5] = {0x7F, 0x49, 0x49, 0x49, 0x36}; memcpy(bitmap, v, 5); break; }
        case 'C': { uint8_t v[5] = {0x3E, 0x41, 0x41, 0x41, 0x22}; memcpy(bitmap, v, 5); break; }
        case 'D': { uint8_t v[5] = {0x7F, 0x41, 0x41, 0x22, 0x1C}; memcpy(bitmap, v, 5); break; }
        case 'E': { uint8_t v[5] = {0x7F, 0x49, 0x49, 0x49, 0x41}; memcpy(bitmap, v, 5); break; }
        case 'G': { uint8_t v[5] = {0x3E, 0x41, 0x49, 0x49, 0x7A}; memcpy(bitmap, v, 5); break; }
        case 'H': { uint8_t v[5] = {0x7F, 0x08, 0x08, 0x08, 0x7F}; memcpy(bitmap, v, 5); break; }
        case 'I': { uint8_t v[5] = {0x00, 0x41, 0x7F, 0x41, 0x00}; memcpy(bitmap, v, 5); break; }
        case 'L': { uint8_t v[5] = {0x7F, 0x40, 0x40, 0x40, 0x40}; memcpy(bitmap, v, 5); break; }
        case 'M': { uint8_t v[5] = {0x7F, 0x02, 0x0C, 0x02, 0x7F}; memcpy(bitmap, v, 5); break; }
        case 'N': { uint8_t v[5] = {0x7F, 0x04, 0x08, 0x10, 0x7F}; memcpy(bitmap, v, 5); break; }
        case 'O': { uint8_t v[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E}; memcpy(bitmap, v, 5); break; }
        case 'P': { uint8_t v[5] = {0x7F, 0x09, 0x09, 0x09, 0x06}; memcpy(bitmap, v, 5); break; }
        case 'R': { uint8_t v[5] = {0x7F, 0x09, 0x19, 0x29, 0x46}; memcpy(bitmap, v, 5); break; }
        case 'S': { uint8_t v[5] = {0x46, 0x49, 0x49, 0x49, 0x31}; memcpy(bitmap, v, 5); break; }
        case 'T': { uint8_t v[5] = {0x01, 0x01, 0x7F, 0x01, 0x01}; memcpy(bitmap, v, 5); break; }
        case 'U': { uint8_t v[5] = {0x3F, 0x40, 0x40, 0x40, 0x3F}; memcpy(bitmap, v, 5); break; }
        case 'W': { uint8_t v[5] = {0x7F, 0x20, 0x18, 0x20, 0x7F}; memcpy(bitmap, v, 5); break; }

        case ':': { uint8_t v[5] = {0x00, 0x36, 0x36, 0x00, 0x00}; memcpy(bitmap, v, 5); break; }
        case '.': { uint8_t v[5] = {0x00, 0x60, 0x60, 0x00, 0x00}; memcpy(bitmap, v, 5); break; }
        case '-': { uint8_t v[5] = {0x08, 0x08, 0x08, 0x08, 0x08}; memcpy(bitmap, v, 5); break; }
        case '%': { uint8_t v[5] = {0x62, 0x64, 0x08, 0x13, 0x23}; memcpy(bitmap, v, 5); break; }
        case ' ': break;
        default: break;
    }
}


static uint8_t OLED_DrawUnsignedNumber(uint8_t x, uint8_t y, uint32_t value)
{
    char digits[10];
    uint8_t count = 0;

    if (value == 0)
    {
        OLED_DrawChar(x, y, '0');
        return x + 6;
    }

    while (value > 0 && count < 10)
    {
        digits[count++] = '0' + (value % 10);
        value /= 10;
    }

    while (count > 0)
    {
        OLED_DrawChar(x, y, digits[--count]);
        x += 6;
    }

    return x;
}


static uint8_t OLED_DrawFloat1(uint8_t x, uint8_t y, float value)
{
    if (value < 0)
    {
        OLED_DrawChar(x, y, '-');
        x += 6;
        value = -value;
    }

    uint32_t scaled = (uint32_t)(value * 10.0f + 0.5f);
    uint32_t whole = scaled / 10;
    uint32_t decimal = scaled % 10;

    x = OLED_DrawUnsignedNumber(x, y, whole);

    OLED_DrawChar(x, y, '.');
    x += 6;

    OLED_DrawChar(x, y, '0' + decimal);
    return x + 6;
}


static const char *OLED_GetStatusText(SystemStatus status)
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


void OLED_Init(I2C_HandleTypeDef *hi2c)
{
    oledI2c = hi2c;

    memset(oledBuffer, 0, sizeof(oledBuffer));

    initIndex = 0;
    setupIndex = 0;
    bufferOffset = 0;

    txComplete = false;
    txError = false;

    initStartTime = HAL_GetTick();
    oledState = OLED_STATE_WAIT_STARTUP;
}


void OLED_Process(void)
{
    if (txError)
    {
        oledState = OLED_STATE_ERROR;
        return;
    }

    switch (oledState)
    {
        case OLED_STATE_OFF:
            break;

        case OLED_STATE_WAIT_STARTUP:
            if ((HAL_GetTick() - initStartTime) >= OLED_INIT_DELAY_MS)
            {
                initIndex = 0;
                oledState = OLED_STATE_INITIALIZING;
            }
            break;

        case OLED_STATE_INITIALIZING:
            if (initIndex >= OLED_INIT_SEQUENCE_SIZE)
            {
                OLED_Clear();
                oledState = OLED_STATE_READY;
                break;
            }

            if (HAL_I2C_GetState(oledI2c) == HAL_I2C_STATE_READY)
            {
                if (OLED_SendCommand(oledInitSequence[initIndex])) initIndex++;
            }
            break;

        case OLED_STATE_READY:
            break;

        case OLED_STATE_SENDING_SETUP:
            if (setupIndex >= OLED_UPDATE_SETUP_SIZE)
            {
                bufferOffset = 0;
                oledState = OLED_STATE_SENDING_BUFFER;
                break;
            }

            if (HAL_I2C_GetState(oledI2c) == HAL_I2C_STATE_READY)
            {
                if (OLED_SendCommand(oledUpdateSetup[setupIndex])) setupIndex++;
            }
            break;

        case OLED_STATE_SENDING_BUFFER:
            if (bufferOffset >= OLED_BUFFER_SIZE)
            {
                oledState = OLED_STATE_READY;
                break;
            }

            if (HAL_I2C_GetState(oledI2c) == HAL_I2C_STATE_READY) OLED_SendBufferChunk();
            break;

        case OLED_STATE_ERROR:
            break;
    }
}


bool OLED_IsReady(void)
{
    return oledState == OLED_STATE_READY;
}


bool OLED_IsBusy(void)
{
    return oledState != OLED_STATE_READY && oledState != OLED_STATE_OFF && oledState != OLED_STATE_ERROR;
}


void OLED_Clear(void)
{
    memset(oledBuffer, 0, sizeof(oledBuffer));
}


void OLED_DrawPixel(uint8_t x, uint8_t y, bool color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;

    uint16_t index = x + (y / 8) * OLED_WIDTH;
    uint8_t mask = 1 << (y % 8);

    if (color) oledBuffer[index] |= mask;
    else oledBuffer[index] &= ~mask;
}


void OLED_DrawChar(uint8_t x, uint8_t y, char c)
{
    uint8_t bitmap[5];

    OLED_GetCharBitmap(c, bitmap);

    for (uint8_t column = 0; column < 5; column++)
    {
        for (uint8_t row = 0; row < 7; row++)
        {
            if (bitmap[column] & (1 << row)) OLED_DrawPixel(x + column, y + row, true);
        }
    }
}


void OLED_DrawString(uint8_t x, uint8_t y, const char *text)
{
    while (*text)
    {
        OLED_DrawChar(x, y, *text);
        x += 6;
        text++;
    }
}


bool OLED_Update(void)
{
    if (oledState != OLED_STATE_READY) return false;

    setupIndex = 0;
    bufferOffset = 0;
    oledState = OLED_STATE_SENDING_SETUP;

    return true;
}


bool OLED_ShowData(const SensorData *data, SystemStatus status)
{
    if (data == NULL) return false;
    if (!OLED_IsReady()) return false;

    OLED_Clear();

    OLED_DrawString(2, 4, "TEMP:");
    OLED_DrawFloat1(38, 4, data->temperature);
    OLED_DrawString(76, 4, "C");

    OLED_DrawString(2, 18, "HUM:");
    OLED_DrawFloat1(38, 18, data->humidity);
    OLED_DrawString(76, 18, "%");

    OLED_DrawString(2, 32, "LIGHT:");
    uint8_t x = OLED_DrawUnsignedNumber(44, 32, data->lightPercent);
    OLED_DrawString(x, 32, "%");


    OLED_DrawString(2, 50, "STATUS:");
    OLED_DrawString(50, 50, OLED_GetStatusText(status));

    return OLED_Update();
}


void OLED_HandleTxComplete(I2C_HandleTypeDef *hi2c)
{
    if (hi2c != oledI2c) return;

    txComplete = true;
}


void OLED_HandleError(I2C_HandleTypeDef *hi2c)
{
    if (hi2c != oledI2c) return;

    txError = true;
}
