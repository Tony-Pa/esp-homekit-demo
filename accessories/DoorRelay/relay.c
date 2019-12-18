#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "wifi.h"


static void wifi_init() {
  struct sdk_station_config wifi_config = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASSWORD,
  };

  sdk_wifi_set_opmode(STATION_MODE);
  sdk_wifi_station_set_config(&wifi_config);
  sdk_wifi_station_connect();
}

const int relay_gpio = 5;

typedef enum {
  lock_state_unsecured = 0,
  lock_state_secured = 1,
  lock_state_jammed = 2,
  lock_state_unknown = 3,
} lock_state_t;

homekit_characteristic_t lock_target_state;
homekit_characteristic_t lock_current_state = HOMEKIT_CHARACTERISTIC_(
    LOCK_CURRENT_STATE,
    lock_state_secured,
);

void relay_write(void *_args) {
  gpio_write(relay_gpio, 1);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  gpio_write(relay_gpio, 0);

  vTaskDelete(NULL);
}

void relay_init() {
  gpio_enable(relay_gpio, GPIO_OUTPUT);
  gpio_write(relay_gpio, 0);
}

void relay_identify(homekit_value_t _value) {
  printf("relay identify\n");
}

void lock_control_point(homekit_value_t value) {
  // Nothing to do here
}

void lock_target_state_setter(homekit_value_t value);

homekit_characteristic_t lock_target_state = HOMEKIT_CHARACTERISTIC_(
    LOCK_TARGET_STATE,
    lock_state_secured,
    .setter=lock_target_state_setter,
);

void lock_target_state_setter(homekit_value_t value) {
  lock_target_state.value = value;

  if (value.int_value == 0) {

    lock_current_state.value = HOMEKIT_UINT8(lock_state_unsecured);
    homekit_characteristic_notify(&lock_current_state, lock_current_state.value);

    xTaskCreate(relay_write, "toggle relay", 2048, NULL, 2, NULL);
  } else {
    lock_current_state.value = HOMEKIT_UINT8(lock_state_secured);
    homekit_characteristic_notify(&lock_current_state, lock_current_state.value);
  }
}

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id = 1, .category=homekit_accessory_category_door_lock, .services=(homekit_service_t *[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION,.characteristics = (homekit_characteristic_t *[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Door"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "TP Inc"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "001"),
            HOMEKIT_CHARACTERISTIC(MODEL, "D1"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, relay_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LOCK_MECHANISM, .primary=true, .characteristics=(homekit_characteristic_t*[]){
          HOMEKIT_CHARACTERISTIC(NAME, "Lock"),
              &lock_current_state,
              &lock_target_state,
              NULL
        }),
        HOMEKIT_SERVICE(LOCK_MANAGEMENT, .characteristics=(homekit_characteristic_t*[]){
          HOMEKIT_CHARACTERISTIC(LOCK_CONTROL_POINT,
              .setter=lock_control_point
          ),
          HOMEKIT_CHARACTERISTIC(VERSION, "1"),
              NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "321-67-123"
};

void user_init(void) {
  uart_set_baud(0, 115200);
  relay_init();
  wifi_init();
  homekit_server_init(&config);
}
