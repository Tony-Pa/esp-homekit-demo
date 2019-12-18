#include <stdio.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#include "wifi.h"

#define RELAY_ON 0x00
#define RELAY_OFF 0x01

uint32_t enabled_switches = 2;
#define LED_CHANNEL_NUM 4




const uint32_t led_gpio[LED_CHANNEL_NUM] = {2, 16, 14, 12};

void on_wifi_ready();

esp_err_t event_handler(void *ctx, system_event_t *event) {
  switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
      printf("STA start\n");
      esp_wifi_connect();
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      printf("WiFI ready\n");
      on_wifi_ready();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      printf("STA disconnected\n");
      esp_wifi_connect();
      break;
    default:
      break;
  }
  return ESP_OK;
}

static void wifi_init() {
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  wifi_config_t wifi_config = {
      .sta = {
          .ssid = WIFI_SSID,
          .password = WIFI_PASSWORD,
      },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}


void pin_write(uint32_t pin, bool status) {
  gpio_set_level(led_gpio[pin], status ? 1 : 0);
}

void pin_init() {
  for (uint32_t i = 0; i < enabled_switches; i++) {
    gpio_set_direction(led_gpio[i], GPIO_MODE_OUTPUT);
    gpio_set_level(led_gpio[i], RELAY_OFF);
  }
}

homekit_characteristic_t switch_on[LED_CHANNEL_NUM];

void led_identify_task(void *_args) {
  uint8_t *index = _args;
  for (uint32_t i = 0; i < 2; i++) {
    for (uint32_t j = 0; j < 2; j++) {
      gpio_set_level(led_gpio[*pin], RELAY_ON);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      gpio_set_level(led_gpio[*pin], RELAY_OFF);

      vTaskDelay(1000 / portTICK_PERIOD_MS);

      gpio_set_level(led_gpio[*pin], RELAY_ON);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      gpio_set_level(led_gpio[*pin], RELAY_OFF);

      vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  pin_write(*index, switch_on[*index].value.bool_value);

  vTaskDelete(NULL);
}

void led_identify(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  uint8_t *index = context;

  printf("LED identify 0\n");
  xTaskCreate(led_identify_task, "Switch identify 0", 2048, index, 2, NULL);
}


bool get_switch_status(*index) {



  return true;
}


void toggle_relay_task(void *_args) {
  uint8_t *pin = _args;

  gpio_set_level(*pin, RELAY_ON);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  gpio_set_level(*pin, RELAY_OFF);

  vTaskDelete(NULL);
}

void toggle_relay(uint32_t index) {
  printf("toggle_relay\n");
  xTaskCreate(toggle_relay_task, "Toggle relay", 2048, &led_gpio[index], 2, NULL);
}

void switch_on_update(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  uint8_t *index = context;
  printf("switch_on_update_%d: %d \n", *index, switch_on[*index].value.bool_value);


//  bool switch_status = get_switch_status(*index);

// compare with current HK state
// if (switch_on[*index].value.bool_value == switch_status)
  toggle_relay(*index);
}

homekit_accessory_t *accessories[LED_CHANNEL_NUM];

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void noop() {}

void init_accessory() {
  for (uint32_t i = 0; i < enabled_switches; i++) {
    switch_on[i] = *(NEW_HOMEKIT_CHARACTERISTIC(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_update, .context = &i)));

    uint32_t relay_name_len = snprintf(NULL, 0, "Switch %d", i + 1);
    char *relay_name_value = malloc(relay_name_len + 1);
    snprintf(relay_name_value, relay_name_len + 1, "Switch %d", i + 1);

    accessories[i] = NEW_HOMEKIT_ACCESSORY(.category = homekit_accessory_category_switch, .services = (homekit_service_t *[]) {
      NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics = (homekit_characteristic_t *[]) {
        NEW_HOMEKIT_CHARACTERISTIC(NAME, relay_name_value),
        NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "TP Inc"),
        NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "000"),
        NEW_HOMEKIT_CHARACTERISTIC(MODEL, "SRHK 1"),
        NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
        NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, noop, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_identify, .context = &i)),
        NULL
      }),
      NEW_HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
        NEW_HOMEKIT_CHARACTERISTIC(NAME, relay_name_value),
          &switch_on[i],
          NULL
      }),
      NULL
    });
  }

  accessories[enabled_switches] = NULL;
}

void on_wifi_ready() {
  homekit_server_init(&config);
}

void app_main(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  wifi_init();
  pin_init();
  init_accessory();
}
