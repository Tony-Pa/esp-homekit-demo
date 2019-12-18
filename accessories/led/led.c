#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>
//#include "wifi.h"
#include "button.h"


//static void wifi_init() {
//  struct sdk_station_config wifi_config = {
//      .ssid = WIFI_SSID,
//      .password = WIFI_PASSWORD,
//  };
//
//  sdk_wifi_set_opmode(STATION_MODE);
//  sdk_wifi_station_set_config(&wifi_config);
//  sdk_wifi_station_connect();
//}

int enabled_leds = 2;

#define led_channel_num 4
const int led_gpio[led_channel_num] = {2, 13, 14, 12};
const int buttons_gpio[led_channel_num] = {5, 4, 0, 0};

bool led_on[led_channel_num] = {false, false, false, false};

int get_channel_index(int pin) {
  for (int i = 0; i < enabled_leds; i++) {
    if (pin == buttons_gpio[i]) {
      return i;
    }
  }

  printf("wrong led pin\n");
  return -1;
}

void led_write(int pin, bool status) {
  gpio_write(pin, status ? 1 : 0);
}

void led_init() {
  for (int i = 0; i < enabled_leds; i++) {
    gpio_enable(led_gpio[i], GPIO_OUTPUT);

    led_write(led_gpio[i], led_on[i]);
  }
}

void buttons_init() {
  for (int i = 0; i < enabled_leds; i++) {
    if (button_create(buttons_gpio[i], button_callback)) {
      printf("Failed to initialize button\n");
    }
  }
}

void led_identify_task_0(void *_args) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      led_write(led_gpio[0], true);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      led_write(led_gpio[0], false);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelay(250 / portTICK_PERIOD_MS);
  }

  led_write(led_gpio[0], led_on_hk[0].value.bool_value);

  vTaskDelete(NULL);
}

void led_identify_task_1(void *_args) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      led_write(led_gpio[1], true);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      led_write(led_gpio[1], false);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelay(250 / portTICK_PERIOD_MS);
  }

  led_write(led_gpio[1], led_on_hk[1].value.bool_value);

  vTaskDelete(NULL);
}

void led_identify_0(homekit_value_t _value) {
  printf("LED identify\n");
  xTaskCreate(led_identify_task_0, "LED identify 0", 128, NULL, 2, NULL);
}

void led_identify_1(homekit_value_t _value) {
  printf("LED identify\n");
  xTaskCreate(led_identify_task_1, "LED identify 1", 128, NULL, 2, NULL);
}

void led_on_update_0(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
void led_on_update_1(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
void led_on_update_2(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
void led_on_update_3(homekit_characteristic_t *_ch, homekit_value_t on, void *context);

homekit_characteristic_t led_on_hk[led_channel_num] = {
    HOMEKIT_CHARACTERISTIC_(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_on_update_0)),
    HOMEKIT_CHARACTERISTIC_(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_on_update_1)),
    HOMEKIT_CHARACTERISTIC_(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_on_update_3)),
    HOMEKIT_CHARACTERISTIC_(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_on_update_4))
};

void led_on_update(int index) {
  printf("led_on_update_$d: %d \n", index, led_on_hk[index].value.bool_value);

  led_on[index] = led_on_hk[index].value.bool_value;
  led_write(led_gpio[index], led_on_hk[index].value.bool_value);
}

void led_on_update_0(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  led_on_update(0);
}
void led_on_update_1(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  led_on_update(1);
}
void led_on_update_2(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  led_on_update(2);
}
void led_on_update_3(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  led_on_update(3);
}

void button_callback(uint8_t gpio, button_event_t event) {
  switch (event) {
    case button_event_single_press:
    case button_event_double_press:
    case button_event_long_press:
      printf("btn click %d\n", gpio);
      int index = get_channel_index(gpio);
      led_on_hk[index].value = HOMEKIT_BOOL(!led_on_hk[index].value.bool_value);
      homekit_characteristic_notify(&led_on_hk[index], led_on_hk[index].value);
      break;
    default:
      printf("unknown button event: %d\n", event);
  }
}

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id = 1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t *[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION,.characteristics = (homekit_characteristic_t *[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Sample LED 0"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "HaPK"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "000"),
            HOMEKIT_CHARACTERISTIC(MODEL, "MyLED"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify_0),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
          HOMEKIT_CHARACTERISTIC(NAME, "Sample LED 0"),
              &led_on_hk[0],
              NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id=2, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
      HOMEKIT_SERVICE(ACCESSORY_INFORMATION,.characteristics = (homekit_characteristic_t *[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Sample LED 1"),
          HOMEKIT_CHARACTERISTIC(MANUFACTURER, "HaPK"),
          HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "001"),
          HOMEKIT_CHARACTERISTIC(MODEL, "MyLED"),
          HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
          HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify_1),
          NULL
      }),
      HOMEKIT_SERVICE(LIGHTBULB,.primary = true, .characteristics = (homekit_characteristic_t *[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Sample LED 1"),
          &led_on_hk[1],
          NULL
      }),
      NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void user_init(void) {
  uart_set_baud(0, 115200);

//  wifi_init();
  wifi_config_init("smart-relays", NULL, on_wifi_ready);

  led_init();
  buttons_init();

  homekit_server_init(&config);
}
