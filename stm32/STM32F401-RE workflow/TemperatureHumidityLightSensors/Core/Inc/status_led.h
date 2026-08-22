#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "sensor_data.h"

void StatusLed_Init(void);
void StatusLed_Set(SystemStatus status);

#endif
