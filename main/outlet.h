#ifndef OUTLET_H
#define OUTLET_H

#include <freertos/FreeRTOS.h>

void init_outlet_gpio();
void set_outlet(bool val);

#endif // OUTLET_H