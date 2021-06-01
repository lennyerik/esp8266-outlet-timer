#include <string.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <lwip/apps/sntp.h>
#include <freertos/task.h>
#include <esp_sleep.h>
#include "wifi.h"
#include "sunrise_sunset.h"
#include "outlet.h"

#define TIME_IS_VALID(t) (t > 1451602800)

static const char *TAG = "outlet_timer";

void app_main() {
	// Init the GPIOs
	init_outlet_gpio();

	// Connect to WiFi
	ESP_ERROR_CHECK(init_wifi());

	ESP_LOGI(TAG, "Connecting to WiFi...");
	ESP_ERROR_CHECK(enable_wifi())
	ESP_ERROR_CHECK(connect_to_wifi());
	if (wifi_connected()) {
		ESP_LOGI(TAG, "Connected to WiFi!");

		// Initialise SNTP
		sntp_setoperatingmode(SNTP_OPMODE_POLL);
		sntp_setservername(0, CONFIG_SNTP_SERVER);
		sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
		sntp_init();

		// Get the current time
		ESP_LOGI(TAG, "Waiting 5 seconds for time sync...");
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		time_t now = time(NULL);
		for (size_t i = 0; i < 5 && !TIME_IS_VALID(now); i++) {
			ESP_LOGI(TAG, "(%d/5) Time still not set...", i + 1);
			if (!wifi_connected()) {
				ESP_LOGE(TAG, "Lost WiFi connection, trying to reconnect...");
				ESP_ERROR_CHECK(connect_to_wifi());
				if (!wifi_connected()) {
					ESP_LOGE(TAG, "Failed to reconnect to WiFi!");
					break;
				}
			}
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			time(&now);
		}

		// Check if we successfully got the time
		if (TIME_IS_VALID(now)) {
			// Save unlocalised time for calculating GMT offset
			setenv("TZ", "UTC+0", 1);
			tzset();
			struct tm tm_unlocalised;
			localtime_r(&now, &tm_unlocalised);

			// Set timezone
			setenv("TZ", CONFIG_TIMEZONE, 1);
			tzset();

			// Get current local time
			struct tm ti = { 0 };
			localtime_r(&now, &ti);
			ESP_LOGI(TAG, "Got time: %ld", now);
			ESP_LOGI(TAG, "          %d:%d:%d", ti.tm_hour, ti.tm_min, ti.tm_sec);

			// Calculate GMT offset of timezone
			tm_unlocalised.tm_isdst = ti.tm_isdst; // We need to set daylight saving time flag correctly
			time_t gmt_offset = now - mktime(&tm_unlocalised);

			// Figure out ON / OFF times
			#ifdef CONFIG_ON_DURING_NIGHT
			time_t on_time = calculate_sunset(&ti, gmt_offset) + CONFIG_ON_OFFSET * 3600;
			time_t tomorrow_time_t = now + 86400;
			localtime_r(&tomorrow_time_t, &ti);
			time_t off_time = calculate_sunrise(&ti, gmt_offset) + CONFIG_OFF_OFFSET * 3600;
			#else // CONFIG_ON_DURING_NIGHT has to be set
			time_t on_time = calculate_sunrise(&ti, gmt_offset) + CONFIG_ON_OFFSET * 3600;
			time_t off_time = calculate_sunset(&ti, gmt_offset) + CONFIG_OFF_OFFSET * 3600;
			#endif

			ESP_LOGI(TAG, "ON time:  %ld", on_time);
			ESP_LOGI(TAG, "OFF time: %ld", off_time);

			// Check and decide whether to toggle outlet on or off
			if (now > on_time && now < off_time) {
				ESP_LOGI(TAG, "Turning outlet output ON.");
				set_outlet(true);
			} else {
				ESP_LOGI(TAG, "Turning outlet output OFF.");
				set_outlet(false);
			}
		} else {
			ESP_LOGE(TAG, "Getting time failed, giving up.");
		}

		// Deinit SNTP & Disconnect from WiFi
		sntp_stop();
		ESP_ERROR_CHECK(disconnect_from_wifi());
		ESP_LOGI(TAG, "Disconnected from WiFi!");
	} else {
		ESP_LOGE(TAG, "Unable to connect to WiFi.");
	}

	ESP_ERROR_CHECK(disable_wifi());
	ESP_ERROR_CHECK(deinit_wifi());

	// Go into deep sleep
	ESP_LOGI(TAG, "Going to sleep...");
	esp_deep_sleep(CONFIG_SLEEP_TIME * 1000000);
}