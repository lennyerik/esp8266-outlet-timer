#include "wifi.h"

#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_netif.h>


// Static variables
static wifi_config_t wifi_config = {
	.sta = {
		.ssid = CONFIG_WIFI_SSID,
		.password = CONFIG_WIFI_PASSWORD
	}
};

#define CONNECTED_BIT BIT(0)
#define DISCONNECTED_BIT BIT(1)
static EventGroupHandle_t wifi_event_group;

static bool wifi_initialised = false;
static bool wifi_enabled = false;


// Event handlers
static void on_got_ip(void *arg, esp_event_base_t e_base, int32_t e_id, void *e_data) {
	xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
}

static void on_disconnected(void *arg, esp_event_base_t e_base, int32_t e_id, void *e_data) {
	xEventGroupSetBits(wifi_event_group, DISCONNECTED_BIT);
}


// Init & Deinit
esp_err_t init_wifi() {
	if (wifi_initialised) {
		return ESP_ERR_INVALID_STATE;
	}

	esp_netif_init();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

	wifi_event_group = xEventGroupCreate();

	wifi_initialised = true;
	return ESP_OK;
}

esp_err_t deinit_wifi() {
	if (!wifi_initialised) {
		return ESP_ERR_INVALID_STATE;
	}

	vEventGroupDelete(wifi_event_group);
	wifi_event_group = NULL;

	ESP_ERROR_CHECK(esp_wifi_deinit());
	esp_netif_deinit();

	wifi_initialised = false;
	return ESP_OK;
}


// Enable & Disable
esp_err_t enable_wifi() {
	if (!wifi_initialised || wifi_enabled) {
		return ESP_ERR_INVALID_STATE;
	}
	
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_disconnected, NULL));
	ESP_ERROR_CHECK(esp_wifi_start());

	wifi_enabled = true;
	return ESP_OK;
}

esp_err_t disable_wifi() {
	if (!wifi_initialised || !wifi_enabled || wifi_connected()) {
		return ESP_ERR_INVALID_STATE;
	}

	ESP_ERROR_CHECK(esp_wifi_stop());
	ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_disconnected));

	wifi_enabled = false;
	return ESP_OK;
}


// Connect & Disconnect
esp_err_t connect_to_wifi() {
	if (!wifi_enabled || wifi_connected()) {
		return ESP_ERR_INVALID_STATE;
	}

	ESP_ERROR_CHECK(esp_wifi_connect());
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, 7000 / portTICK_PERIOD_MS);

	return ESP_OK;
}

esp_err_t disconnect_from_wifi() {
	if (!wifi_enabled) {
		return ESP_ERR_INVALID_STATE;
	}
	if (!wifi_connected()) {
		// We're ok with it 'silently' not disconnecting if connection has already been lost
		return ESP_OK;
	}

	ESP_ERROR_CHECK(esp_wifi_disconnect());
	xEventGroupWaitBits(wifi_event_group, DISCONNECTED_BIT, false, true, portMAX_DELAY);

	xEventGroupClearBits(wifi_event_group, CONNECTED_BIT | DISCONNECTED_BIT);
	
	return ESP_OK;
}

bool wifi_connected() {
	if (!wifi_initialised) {
		return false;
	}
	EventBits_t bits = xEventGroupGetBits(wifi_event_group);
	return (bits & CONNECTED_BIT) && !(bits & DISCONNECTED_BIT);
}