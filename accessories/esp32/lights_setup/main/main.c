//#include <stdio.h>
//#include <esp_wifi.h>
//#include <esp_event_loop.h>
//#include <esp_log.h>
//#include <nvs_flash.h>
//
//#include <freertos/FreeRTOS.h>
//#include <freertos/task.h>
//#include <driver/gpio.h>
//
//#include "wifi.h"
//#include "mux.h"
//#include "config.h"
//
//#define RELAY_ON 0x00
//#define RELAY_OFF 0x01
//
//#define LED_CHANNEL_NUM 32
//
//uint8_t enabled_switches = LED_CHANNEL_NUM;
//uint8_t iItem[LED_CHANNEL_NUM];
//uint8_t currentSensor = 1;
//
//
//const uint8_t relaysControlPins[MUX_CONTROLS] = {21, 18, 19, 5};
//const uint8_t relaysChanelPins[2] = {16, 17};
//
//const uint8_t sensorsControlPins[MUX_CONTROLS] = {25, 26, 27, 33};
//const uint8_t sensorsChanelPins[2] = {36, 34}; // 36, 34
//
//void pin_init() {
//  uint8_t i = 0;
//  for (i = 0; i < MUX_CONTROLS; i++) {
//    gpio_set_direction(relaysControlPins[i], GPIO_MODE_OUTPUT);
//    gpio_set_level(relaysControlPins[i], 0);
//
//    gpio_set_direction(sensorsControlPins[i], GPIO_MODE_OUTPUT);
//    gpio_set_level(sensorsControlPins[i], 0);
//  }
//
//  for (uint8_t i = 0; i < 2; i++) {
//    gpio_set_direction(relaysChanelPins[i], GPIO_MODE_OUTPUT);
//    gpio_set_level(relaysChanelPins[i], RELAY_OFF);
//
//    gpio_set_direction(sensorsChanelPins[i], GPIO_MODE_INPUT);
//  }
//}
//
//void on_wifi_ready();
//esp_err_t event_handler(void *ctx, system_event_t *event) {
//  switch (event->event_id) {
//    case SYSTEM_EVENT_STA_START:
//      printf("STA start\n");
//      esp_wifi_connect();
//      break;
//    case SYSTEM_EVENT_STA_GOT_IP:
//      printf("WiFI ready\n");
//      on_wifi_ready();
//      break;
//    case SYSTEM_EVENT_STA_DISCONNECTED:
//      printf("STA disconnected\n");
//      esp_wifi_connect();
//      break;
//    default:
//      break;
//  }
//  return ESP_OK;
//}
//
//static void wifi_init() {
//  tcpip_adapter_init();
//  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
//
//  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
//  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
//  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
//
//  wifi_config_t wifi_config = {
//      .sta = {
//          .ssid = WIFI_SSID,
//          .password = WIFI_PASSWORD,
//      },
//  };
//
//  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
//  ESP_ERROR_CHECK(esp_wifi_start());
//}
//
//void set_relay_status(uint8_t channel, bool status) {
//  set_mux(channel, relaysControlPins, relaysChanelPins, status ? 1 : 0);
//}
//
//
//int get_switch_status(uint8_t channel) {
//  uint8_t on = 0;
//  uint8_t off = 0;
//
//  for (uint8_t i = 0; i < 11; i++) {
//    if (get_mux(channel, sensorsControlPins, sensorsChanelPins) ? false : true ) {
//      on++;
//    }
//    else {
//      off++;
//    }
//  }
//  if (on != 11 && off != 11) {
//    printf("get_switch_status %d: %d - %d\n", channel, on, off);
//  }
//
//  return on > off ? 1 : 0;
//
//
////  return get_mux(channel, sensorsControlPins, sensorsChanelPins);
//}
//
//void toggle_relay_task(void *_args) {
//  uint8_t *channel = _args;
//
//  set_relay_status(*channel, RELAY_ON);
//  vTaskDelay(500 / portTICK_PERIOD_MS);
//  set_relay_status(*channel, RELAY_OFF);
//
//  vTaskDelete(NULL);
//}
//
//void toggle_relay(uint8_t index) {
//  printf("toggle_relay: %d\n", index);
//  xTaskCreate(toggle_relay_task, "Toggle relay", 2048, &iItem[index], 2, NULL);
//}
//
//void check_sensors_task(void *_args) {
//  while(1) {
//
//      int switch_status = get_switch_status(currentSensor);
//
//      printf("check_sensors_task: %d: %d\n", currentSensor, switch_status);
//
//    vTaskDelay(50 / portTICK_PERIOD_MS);
//
////    toggle_relay(currentSensor);
//
//    currentSensor++;
//
//    if (currentSensor >= enabled_switches) {
//      currentSensor = 0;
//      vTaskDelay(2000 / portTICK_PERIOD_MS);
//      printf("\n");
//
//    }
//  }
//}
//
//void on_wifi_ready() {
//
//  xTaskCreate(check_sensors_task, "Check sensors", 2048, NULL, 2, NULL);
//}
//
//void app_main(void) {
//  // Initialize NVS
//  esp_err_t ret = nvs_flash_init();
//  if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
//    ESP_ERROR_CHECK(nvs_flash_erase());
//    ret = nvs_flash_init();
//  }
//  ESP_ERROR_CHECK(ret);
//
//  wifi_init();
//  pin_init();
//
//  for (uint8_t i = 0; i < 32; i++) {
//    iItem[i] = i;
//  }
//}



#include <stdio.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/adc.h>

#include "wifi.h"
#include "mux.h"
#include "config.h"

#define RELAY_ON 0x00
#define RELAY_OFF 0x01

#define LED_CHANNEL_NUM 32

uint8_t enabled_switches = LED_CHANNEL_NUM;
uint8_t iItem[LED_CHANNEL_NUM];
uint8_t currentSensor = 1;


const uint8_t relaysControlPins[MUX_CONTROLS] = {21, 18, 19, 5};
const uint8_t relaysChanelPins[2] = {16, 17};

const uint8_t sensorsControlPins[MUX_CONTROLS] = {25, 26, 27, 33};
const uint8_t sensorsChanelPins[2] = {ADC1_CHANNEL_0, ADC1_CHANNEL_6}; // 36, 34

void pin_init() {
  uint8_t i = 0;
  for (i = 0; i < MUX_CONTROLS; i++) {
    gpio_set_direction(relaysControlPins[i], GPIO_MODE_OUTPUT);
    gpio_set_level(relaysControlPins[i], 0);
  }

  for (i = 0; i < MUX_CONTROLS; i++) {
    gpio_set_direction(sensorsControlPins[i], GPIO_MODE_OUTPUT);
    gpio_set_level(sensorsControlPins[i], 0);
  }

  for (uint8_t i = 0; i < 2; i++) {
    gpio_set_direction(relaysChanelPins[i], GPIO_MODE_OUTPUT);
    gpio_set_level(relaysChanelPins[i], RELAY_OFF);
  }

  adc1_config_width(ADC_WIDTH_12Bit);
  for (uint8_t i = 0; i < 2; i++) {
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


int get_switch_status(uint8_t channel) {
  uint8_t on = 0;
  uint8_t off = 0;

  for (uint8_t i = 0; i < 11; i++) {
    if (get_mux(channel, sensorsControlPins, sensorsChanelPins) < 100 ? true : false) {
      on++;
    }
    else {
      off++;
    }
  }
  if (on != 11 && off != 11) {
    printf("get_switch_status %d: %d - %d\n", channel, on, off);
  }

  return on > off ? 1 : 0;


//  return get_mux(channel, sensorsControlPins, sensorsChanelPins);
}

void toggle_relay_task(void *_args) {
  uint8_t *channel = _args;

  set_relay_status(*channel, RELAY_ON);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  set_relay_status(*channel, RELAY_OFF);

  vTaskDelete(NULL);
}

void toggle_relay(uint8_t index) {
  printf("toggle_relay: %d\n", index);
  xTaskCreate(toggle_relay_task, "Toggle relay", 2048, &iItem[index], 2, NULL);
}

void check_sensors_task(void *_args) {
  while(1) {

    int switch_status = get_switch_status(currentSensor);

    printf("check_sensors_task: %d: %d\n", currentSensor, switch_status);

    vTaskDelay(50 / portTICK_PERIOD_MS);

//    toggle_relay(currentSensor);

    currentSensor++;

    if (currentSensor >= enabled_switches) {
      currentSensor = 0;
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      printf("\n");

    }
  }
}

void on_wifi_ready() {

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

  for (uint8_t i = 0; i < 32; i++) {
    iItem[i] = i;
  }
}
