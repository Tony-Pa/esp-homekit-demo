#include <stdio.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#include "iot_button.h"
#include "wifi.h"

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

int enabled_leds = 2;
#define led_channel_num 4


const int led_gpio[led_channel_num] = {2, 16, 14, 12};
const int buttons_gpio[led_channel_num] = {4, 5, 0, 0};

int indexes[led_channel_num] = {0, 1, 2, 3};

button_handle_t btn_handle[led_channel_num];
homekit_characteristic_t led_on[led_channel_num];

void led_write(int pin, bool status) {
  gpio_set_level(led_gpio[pin], status ? 1 : 0);
}

static void push_btn_cb(void *_args) {
  int *index = _args;

  printf("push_btn_cb-%d: %d\n", *index, !led_on[*index].value.bool_value);

  led_on[*index].value = HOMEKIT_BOOL(!led_on[*index].value.bool_value);

  homekit_characteristic_notify(&led_on[*index], led_on[*index].value);
  led_write(*index, led_on[*index].value.bool_value);
}

void buttons_init() {
  for (int i = 0; i < enabled_leds; i++) {
    btn_handle[i] = iot_button_create(buttons_gpio[i], 1);
    iot_button_set_evt_cb(btn_handle[i], BUTTON_CB_PUSH, push_btn_cb, &indexes[i]);
  }
}

void led_init() {
  for (int i = 0; i < enabled_leds; i++) {
    gpio_set_direction(led_gpio[i], GPIO_MODE_OUTPUT);

    led_write(i, led_on[i].value.bool_value);
  }
}

void led_identify_task(void *_args) {
  uint8_t *index = _args;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      led_write(*index, true);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      led_write(*index, false);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelay(250 / portTICK_PERIOD_MS);
  }

  led_write(*index, led_on[*index].value.bool_value);

  vTaskDelete(NULL);
}

typedef void (*fn_t)(homekit_value_t);

void led_identify_0(homekit_value_t _value) {
  printf("LED identify 0\n");
  xTaskCreate(led_identify_task, "Switch identify 0", 2048, &indexes[0], 2, NULL);
}

void led_identify_1(homekit_value_t _value) {
  printf("LED identify 1\n");
  xTaskCreate(led_identify_task, "Switch identify 1", 2048, &indexes[1], 2, NULL);
}

void led_identify_2(homekit_value_t _value) {
  printf("LED identify 2\n");
  xTaskCreate(led_identify_task, "Switch identify 1", 2048, &indexes[2], 2, NULL);
}

void led_identify_3(homekit_value_t _value) {
  printf("LED identify 3\n");
  xTaskCreate(led_identify_task, "Switch identify 1", 2048, &indexes[3], 2, NULL);
}

fn_t led_identify(int i) {
  switch (i) {
    case 0:
      return led_identify_0;
    case 1:
      return led_identify_1;
    case 2:
      return led_identify_2;
    case 3:
      return led_identify_3;
    default:
      return led_identify_0;
  }
}

void led_on_update(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  uint8_t *index = context;
  printf("led_on_update_%d: %d \n", *index, led_on[*index].value.bool_value);

  led_write(*index, led_on[*index].value.bool_value);
}

homekit_accessory_t *accessories[led_channel_num];

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void init_accessory() {
  for (int i = 0; i < enabled_leds; i++) {
    led_on[i] = *(NEW_HOMEKIT_CHARACTERISTIC(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_on_update, .context = &indexes[i])));

    int relay_name_len = snprintf(NULL, 0, "Switch %d", i + 1);
    char *relay_name_value = malloc(relay_name_len + 1);
    snprintf(relay_name_value, relay_name_len + 1, "Switch %d", i + 1);

    accessories[i] = NEW_HOMEKIT_ACCESSORY(.category = homekit_accessory_category_switch, .services = (homekit_service_t *[]) {
      NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics = (homekit_characteristic_t *[]) {
        NEW_HOMEKIT_CHARACTERISTIC(NAME, relay_name_value),
        NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "TP Inc"),
        NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "000"),
        NEW_HOMEKIT_CHARACTERISTIC(MODEL, "SRHK 1"),
        NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
        NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify(i)),
        NULL
      }),
      NEW_HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
        NEW_HOMEKIT_CHARACTERISTIC(NAME, relay_name_value),
        &led_on[i],
        NULL
      }),
      NULL
    });
  }

  accessories[enabled_leds] = NULL;
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
  led_init();
  buttons_init();
  init_accessory();
}
