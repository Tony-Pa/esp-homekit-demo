#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp/hwrand.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

//#include <wifi_config.h>
#include <adv_button.h>
#include <ws2812_i2s/ws2812_i2s.h>

#include "wifi.h"

#include "gyverLamp.h"
#include "libraries/power_mgt.h"

void esp_ws2812(void *pvParameters) {
  ws2812_i2s_init(LEDS_COUNT, PIXEL_RGB);
  memset(pixels, 0, sizeof(ws2812_pixel_t) * LEDS_COUNT);

  uint32_t pos = 0;
  uint8_t fx = 5;
  uint8_t fx_count = 6;

  uint32_t my_time = sdk_system_get_time();
  uint32_t delay = 50;
  int a = 0;

  while (1) {
//    colorsRoutine2();
    fireRoutine();
//    lightersRoutine();
//    colorsRoutine();


//    globalBrightness = (globalBrightness + 1) % 255;
//
//    printf("globalBrightness: %d\n", globalBrightness);


    if (globalBrightness != 255) {
      for (int i = 0; i < LEDS_COUNT; i++) {
        if (!pixels[i].red && !pixels[i].green && !pixels[i].blue) {
          continue;
        }


        rgb_t color, finalColor;

        color.red = pixels[i].red;
        color.green = pixels[i].green;
        color.blue = pixels[i].blue;

//      if (i ==0 ) printf("orig c r: %d, g: %d, b: %d\n", color.red, color.green, color.blue);

        hsv_t hsv = rgb_to_hsv(color);
//      printf("hsv.v: %d, \n", hsv.v );


        int16_t value = hsv.v - 255 + globalBrightness;

//      printf("globalBrightness: %d, value: %d, new: %d\n", globalBrightness, value, constrain(value, 0, 255));


        hsv_to_rgb(hsv.h, hsv.s, constrain(value, 0, 255), &finalColor);

//      if (i ==0 ) printf("finalColor c r: %d, g: %d, b: %d\n", finalColor.red, finalColor.green, finalColor.blue);

        pixels[i].red = finalColor.red;
        pixels[i].green = finalColor.green;
        pixels[i].blue = finalColor.blue;
      }
    }

    ws2812_i2s_update(pixels, PIXEL_RGB);

    vTaskDelay(delay / portTICK_PERIOD_MS);
  }
}

void esp_ws2812_init() {
  xTaskCreate(&esp_ws2812, "ws2812", 256, NULL, 10, NULL);
}


//
//
//
//const int led_gpio = 2;
//const int button_gpio = 5;
//homekit_characteristic_t led_on_hk;
//
//void led_write(int pin, bool status) {
//  gpio_write(pin, status ? 1 : 0);
//}
//
//void led_init() {
//  gpio_enable(led_gpio, GPIO_OUTPUT);
//
//  led_write(led_gpio, LOW);
//}
//
//void led_identify_task(void *_args) {
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
//
//  vTaskDelete(NULL);
//}
//
//void led_identify(homekit_value_t _value) {
//  printf("LED identify\n");
//  xTaskCreate(led_identify_task, "LED identify", 128, NULL, 2, NULL);
//}
//
//
//void led_on_update(int index) {
//  printf("led_on_update_%d: %d \n", index, led_on_hk.value.bool_value);
//
//  led_write(led_gpio, led_on_hk.value.bool_value);
//}
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
//
//homekit_characteristic_t led_on_hk = HOMEKIT_CHARACTERISTIC_(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(led_on_update));
//
//homekit_accessory_t *accessories[] = {
//    HOMEKIT_ACCESSORY(.id = 1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t *[]) {
//        HOMEKIT_SERVICE(ACCESSORY_INFORMATION,.characteristics = (homekit_characteristic_t *[]) {
//            HOMEKIT_CHARACTERISTIC(NAME, "Gyver Lamp"),
//            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "HaPK"),
//            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "000"),
//            HOMEKIT_CHARACTERISTIC(MODEL, "GL001"),
//            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
//            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
//            NULL
//        }),
//        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
//          HOMEKIT_CHARACTERISTIC(NAME, "Light"),
//              &led_on_hk,
//              NULL
//        }),
//        NULL
//    }),
//    NULL
//};
//
//homekit_server_config_t config = {
//    .accessories = accessories,
//    .password = "111-11-111"
//};
//
//void on_wifi_ready() {
//  homekit_server_init(&config);
//}

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

void user_init(void) {
  uart_set_baud(0, 115200);

//  wifi_init();
//  wifi_config_init("gyverLamp", NULL, on_wifi_ready);
//
//  led_init();
//  buttons_init();

  memset(matrixValue, 0, sizeof(matrixValue));

  esp_ws2812_init();
}
