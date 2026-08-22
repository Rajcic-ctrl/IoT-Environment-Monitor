#include "status_led.h"
#include "main.h"

static void StatusLed_AllOff(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5, GPIO_PIN_RESET);
}

void StatusLed_Init(void)
{
    StatusLed_AllOff();
}

void StatusLed_Set(SystemStatus status)
{
    StatusLed_AllOff();

    switch (status)
    {
        case SYSTEM_STATUS_GOOD:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
            break;

        case SYSTEM_STATUS_WARNING:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
            break;

        case SYSTEM_STATUS_BAD:
        case SYSTEM_STATUS_ERROR:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
            break;
    }
}
