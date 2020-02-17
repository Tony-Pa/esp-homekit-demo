#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

//#include <wifi_config.h>
#include <adv_button.h>
#include <ws2812_i2s/ws2812_i2s.h>
#include <math.h>
#include <stdlib.h>

#include "wifi.h"

#include "gyverLamp.h"
#include "libraries/scale8.h"
#include "effects.h"

#define LOW 0
#define HIGH 1


#define COLORS 0
#define FIRE 1
#define LIGHTERS 2

homekit_characteristic_t led_on_hk;
homekit_characteristic_t led_brightness_hk;
homekit_characteristic_t led_hue_hk;
homekit_characteristic_t led_saturation_hk;
homekit_characteristic_t on_fire;
homekit_characteristic_t on_colors;
homekit_characteristic_t on_lighters;
uint8_t mode = 0;

void esp_ws2812(void *pvParameters) {
  ws2812_i2s_init(LEDS_COUNT, PIXEL_RGB);
  memset(pixels, 0, sizeof(ws2812_pixel_t) * LEDS_COUNT);

  uint32_t delay = 50;

  while (1) {
    switch (mode) {
      case FIRE: fireRoutine(led_hue_hk.value.float_value, led_saturation_hk.value.float_value);
        break;

      case LIGHTERS: lightersRoutine();
        break;

      default:
        colorsRoutine(led_hue_hk.value.float_value, led_saturation_hk.value.float_value, 100);
    }
    setMaxBrightness(led_brightness_hk.value.int_value * 2.55);
    ws2812_i2s_update(pixels, PIXEL_RGB);

    vTaskDelay(delay / portTICK_PERIOD_MS);
  }
}

void esp_ws2812_init() {
  xTaskCreate(&esp_ws2812, "ws2812", 256, NULL, 10, NULL);
}

//const int button_gpio = 5;

void led_identify_task(void *_args) {
//  for (int i = 0; i < 3; i++) {
//    for (int j = 0; j < 2; j++) {
//      led_write(led_gpio, true);
//      vTaskDelay(100 / portTICK_PERIOD_MS);
//      led_write(led_gpio, false);
//      vTaskDelay(100 / portTICK_PERIOD_MS);
//    }
//
//    vTaskDelay(250 / portTICK_PERIOD_MS);
//  }
//
//  led_write(led_gpio, led_on_hk.value.bool_value);

  vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
  printf("LED identify\n");
  xTaskCreate(led_identify_task, "LED identify", 128, NULL, 2, NULL);
}

void led_on_update(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  printf("led_on_update: %d \n", led_on_hk.value.bool_value);
}

void led_brightness_update(homekit_characteristic_t *_ch, homekit_value_t value, void *context) {
  printf("led_brightness_update: %d \n", led_brightness_hk.value.int_value);
}

void led_hue_update(homekit_characteristic_t *_ch, homekit_value_t value, void *context) {
  printf("led_hue_update: %.0f \n", led_hue_hk.value.float_value);
}

void led_saturation_update(homekit_characteristic_t *_ch, homekit_value_t value, void *context) {
  printf("led_saturation_update: %.0f \n", led_saturation_hk.value.float_value);
}

void on_fire_update(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  printf("on_fire_update: %d \n", on_fire.value.bool_value);

  if (on_fire.value.bool_value) {
    mode = FIRE;
    on_colors.value = HOMEKIT_BOOL(false);
    on_lighters.value = HOMEKIT_BOOL(false);


    homekit_characteristic_notify(&on_colors, on_colors.value);
    homekit_characteristic_notify(&on_lighters, on_lighters.value);
  }
}

void on_colors_update(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  printf("on_colors_update: %d \n", on_colors.value.bool_value);

  if (on_colors.value.bool_value) {
    mode = COLORS;
    on_fire.value = HOMEKIT_BOOL(false);
    on_lighters.value = HOMEKIT_BOOL(false);

    homekit_characteristic_notify(&on_fire, on_fire.value);
    homekit_characteristic_notify(&on_lighters, on_lighters.value);
  }
}

void on_lighters_update(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
  printf("on_colors_update: %d \n", on_colors.value.bool_value);

  if (on_lighters.value.bool_value) {
    mode = LIGHTERS;
    on_fire.value = HOMEKIT_BOOL(false);
    on_colors.value = HOMEKIT_BOOL(false);


    homekit_characteristic_notify(&on_fire, on_fire.value);
    homekit_characteristic_notify(&on_colors, on_colors.value);
  }
}


//
//void button_callback(uint8_t gpio, void *args, const uint8_t param) {
//  printf("btn click: %d\n", gpio);
//  led_on_hk.value = HOMEKIT_BOOL(!led_on_hk.value.bool_value);
//  homekit_characteristic_notify(&led_on_hk, led_on_hk.value);
//}
//
//void buttons_init() {
//  bool pullup_resistor = true;
//  bool inverted = false;
//  uint8_t button_type = 1;
//  adv_button_create(button_gpio, pullup_resistor, inverted);
//  adv_button_register_callback_fn(button_gpio, button_callback, button_type, NULL, 0);
//}

homekit_characteristic_t led_on_hk = HOMEKIT_CHARACTERISTIC_(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_on_update));
homekit_characteristic_t led_brightness_hk = HOMEKIT_CHARACTERISTIC_(BRIGHTNESS, 100, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_brightness_update));
homekit_characteristic_t led_hue_hk = HOMEKIT_CHARACTERISTIC_(HUE, 0, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_hue_update));
homekit_characteristic_t led_saturation_hk = HOMEKIT_CHARACTERISTIC_(SATURATION, 100, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_saturation_update));

homekit_characteristic_t on_colors = HOMEKIT_CHARACTERISTIC_(ON, true, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_colors_update));
homekit_characteristic_t on_fire = HOMEKIT_CHARACTERISTIC_(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_fire_update));
homekit_characteristic_t on_lighters = HOMEKIT_CHARACTERISTIC_(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_lighters_update));


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id = 1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t *[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics = (homekit_characteristic_t *[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Gyver Lamp"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "HaPK"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "000"),
            HOMEKIT_CHARACTERISTIC(MODEL, "GL001"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light"),
            &led_on_hk,
            &led_brightness_hk,
            &led_hue_hk,
            &led_saturation_hk,
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Fire"), &on_fire, NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Colors"), &on_colors, NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Lighters"), &on_lighters, NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void on_wifi_ready() {
  homekit_server_init(&config);
}

static void wifi_init() {
  struct sdk_station_config wifi_config = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASSWORD,
  };

  sdk_wifi_set_opmode(STATION_MODE);
  sdk_wifi_station_set_config(&wifi_config);
  sdk_wifi_station_connect();
}

void user_init(void) {
  uart_set_baud(0, 115200);

  wifi_init();
  on_wifi_ready();
//  wifi_config_init("gyverLamp", NULL, on_wifi_ready);
//
//  buttons_init();

  esp_ws2812_init();
}
