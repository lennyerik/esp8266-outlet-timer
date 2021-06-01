#ifndef WIFI_H
#define WIFI_H

#include <freertos/FreeRTOS.h>

esp_err_t init_wifi();
esp_err_t deinit_wifi();
esp_err_t enable_wifi();
esp_err_t disable_wifi();
esp_err_t connect_to_wifi();
esp_err_t disconnect_from_wifi();
bool wifi_connected();

#endif // WIFI_H