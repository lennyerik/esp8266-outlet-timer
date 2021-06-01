#ifndef SUNRISE_SUNSET_H
#define SUNRISE_SUNSET_H

#include <freertos/FreeRTOS.h>
#include <time.h>

time_t calculate_sunrise(const struct tm *now, int local_offset);
time_t calculate_sunset(const struct tm *now, int local_offset);

#endif // SUNRISE_SUNSET_H