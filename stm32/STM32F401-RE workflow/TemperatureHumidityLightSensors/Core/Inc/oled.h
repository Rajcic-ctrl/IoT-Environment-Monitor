#ifndef OLED_H
#define OLED_H

#include "main.h"
#include "sensor_data.h"
#include <stdbool.h>
#include <stdint.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_BUFFER_SIZE 1024

void OLED_Init(I2C_HandleTypeDef *hi2c);
void OLED_Process(void);

bool OLED_IsReady(void);
bool OLED_IsBusy(void);

void OLED_Clear(void);
void OLED_DrawPixel(uint8_t x, uint8_t y, bool color);
void OLED_DrawChar(uint8_t x, uint8_t y, char c);
void OLED_DrawString(uint8_t x, uint8_t y, const char *text);

bool OLED_Update(void);
bool OLED_ShowData(const SensorData *data, SystemStatus status);

void OLED_HandleTxComplete(I2C_HandleTypeDef *hi2c);
void OLED_HandleError(I2C_HandleTypeDef *hi2c);

#endif
