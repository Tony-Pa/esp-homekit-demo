#include <stdio.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/adc.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#include "wifi.h"
#include "mux.h"
#include "config.h"

#define RELAY_ON 0x00
#define RELAY_OFF 0x01

#define LED_CHANNEL_NUM 27

uint8_t enabled_switches = LED_CHANNEL_NUM;
uint8_t iItem[LED_CHANNEL_NUM];
uint8_t currentSensor = 0;
bool relayBusy = false;

homekit_characteristic_t switch_on[LED_CHANNEL_NUM];

const uint8_t relaysControlPins[MUX_CONTROLS] = {21, 18, 19, 5};
const uint8_t relaysChanelPins[2] = {16, 17};

const uint8_t sensorsControlPins[MUX_CONTROLS] = {25, 26, 27, 33};
const uint8_t sensorsChanelPins[2] = {ADC1_CHANNEL_0, ADC1_CHANNEL_6}; // 36, 34

void pin_init() {
  for (uint8_t i = 0; i < MUX_CONTROLS; i++) {
    gpio_set_direction(relaysControlPins[i], GPIO_MODE_OUTPUT);
    gpio_set_level(relaysControlPins[i], 0);

    gpio_set_direction(sensorsControlPins[i], GPIO_MODE_OUTPUT);
    gpio_set_level(sensorsControlPins[i], 0);
  }

  adc1_config_width(ADC_WIDTH_12Bit);
  for (uint8_t i = 0; i < 2; i++) {
    gpio_set_direction(relaysChanelPins[i], GPIO_MODE_OUTPUT);
    gpio_set_level(relaysChanelPins[i], RELAY_OFF);

    adc1_config_channel_atten(sensorsChanelPins[i], ADC_ATTEN_11db);
  }
}

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

void set_relay_status(uint8_t channel, bool status) {
  set_mux(channel, relaysControlPins, relaysChanelPins, status ? 1 : 0);
}

void led_identify_task(void *_args) {
  uint8_t *index = _args;
  for (uint8_t i = 0; i < 2; i++) {
    for (uint8_t j = 0; j < 2; j++) {
      set_relay_status(accessories_config[*index].relay, RELAY_ON);
      vTaskDelay(200 / portTICK_PERIOD_MS);
      set_relay_status(accessories_config[*index].relay, RELAY_OFF);

      vTaskDelay(1000 / portTICK_PERIOD_MS);

      set_relay_status(accessories_config[*index].relay, RELAY_ON);
      vTaskDelay(200 / portTICK_PERIOD_MS);
      set_relay_status(accessories_config[*index].relay, RELAY_OFF);

      vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  vTaskDelete(NULL);
}

void led_identify(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  uint8_t *index = context;

  printf("LED identify 0\n");
  xTaskCreate(led_identify_task, "Switch identify 0", 2048, index, 2, NULL);
}

bool get_switch_status(uint8_t index) {
  uint8_t on = 0;
  uint8_t off = 0;

  for (uint8_t i = 0; i < 11; i++) {
    if (get_mux(accessories_config[index].sensor, sensorsControlPins, sensorsChanelPins) < 100 ? true : false) {
      on++;
    }
    else {
      off++;
    }
  }
  if (on != 11 && off != 11) {
    printf("get_switch_status %s: %d - %d\n", accessories_config[index].name, on, off);
  }

  return on > off;
}

void toggle_relay_task(void *_args) {
  uint8_t i = 0;
  while (relayBusy) {
    vTaskDelay(220 / portTICK_PERIOD_MS);
    i++;

    if (i < 10) {
      printf("toggle_relay_task: exit by iterator\n");
      break;
    }
  }

  relayBusy = true;
  uint8_t *channel = _args;

  set_relay_status(*channel, RELAY_ON);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  set_relay_status(*channel, RELAY_OFF);

  relayBusy = false;
  vTaskDelete(NULL);
}

void toggle_relay(uint8_t index) {
  printf("toggle_relay: %d - %d\n", index, accessories_config[index].relay);
  xTaskCreate(toggle_relay_task, "Toggle relay", 2048, &accessories_config[index].relay, 2, NULL);
}

void switch_on_update(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  uint8_t *index = context;
  printf("switch_on_update_%d: %d \n", *index, switch_on[*index].value.bool_value);

  bool switch_status = get_switch_status(*index);

 if (switch_on[*index].value.bool_value != switch_status) {
   toggle_relay(*index);
 }
}

homekit_accessory_t *accessories[LED_CHANNEL_NUM + 2];

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void init_accessory() {
  accessories[0] = NEW_HOMEKIT_ACCESSORY(.category = homekit_accessory_category_bridge, .services = (homekit_service_t *[]) {
      NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION,.characteristics = (homekit_characteristic_t *[]) {
          NEW_HOMEKIT_CHARACTERISTIC(NAME, "Light bridge"),
          NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "TP Inc"),
          NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "000"),
          NEW_HOMEKIT_CHARACTERISTIC(MODEL, "SRHK 1"),
          NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
          NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, NULL),
          NULL
      }),
      NULL
  });

  for (uint8_t i = 0; i < enabled_switches; i++) {
    iItem[i] = i;
    switch_on[i] = *(NEW_HOMEKIT_CHARACTERISTIC(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_update, .context = &iItem[i])));

    accessories[i + 1] = NEW_HOMEKIT_ACCESSORY(.category = homekit_accessory_category_switch, .services = (homekit_service_t *[]) {
      NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION,.characteristics = (homekit_characteristic_t *[]) {
        NEW_HOMEKIT_CHARACTERISTIC(NAME, accessories_config[i].name),
        NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "TP Inc"),
        NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "000"),
        NEW_HOMEKIT_CHARACTERISTIC(MODEL, "SRHK 1"),
        NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
        NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, NULL, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_identify, .context = &iItem[i])),
        NULL
      }),
      NEW_HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
        NEW_HOMEKIT_CHARACTERISTIC(NAME, accessories_config[i].name),
          &switch_on[i],
          NULL
      }),
      NULL
    });
  }

  accessories[enabled_switches + 1] = NULL;
}

void check_sensors_task(void *_args) {
  while(1) {
    bool switch_status = get_switch_status(currentSensor);

    if (switch_status != switch_on[currentSensor].value.bool_value) {
      switch_on[currentSensor].value = HOMEKIT_BOOL(!switch_on[currentSensor].value.bool_value);
      homekit_characteristic_notify(&switch_on[currentSensor], switch_on[currentSensor].value);

      printf("check_sensors_task: %d: %d - %d\n", currentSensor, accessories_config[currentSensor].sensor, switch_status);
    }

    currentSensor++;

    if (currentSensor >= enabled_switches) {
      currentSensor = 0;
    }

    vTaskDelay(1000 / enabled_switches / portTICK_PERIOD_MS);
  }
}

void on_wifi_ready() {
  homekit_server_init(&config);

  xTaskCreate(check_sensors_task, "Check sensors", 2048, NULL, 2, NULL);
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
