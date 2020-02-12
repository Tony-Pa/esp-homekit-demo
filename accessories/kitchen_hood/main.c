#include <stdio.h>
#include <string.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <sysparam.h>

#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
//#include <wifi_config.h>
#include <adv_button.h>
#include "wifi.h"

#define SPEED_OFF 0x00
#define SPEED_1 25
#define SPEED_2 50
#define SPEED_3 75
#define SPEED_4 100
#define SPEED_STEP 25

#define RELAY_ON 0x00
#define RELAY_OFF 0x01

#define LPF_SHIFT 4  // divide by 16
#define LPF_INTERVAL 100  // in milliseconds

#define SPEED_RELAY_1 16
#define SPEED_RELAY_2 14
#define SPEED_RELAY_3 12
#define SPEED_RELAY_4 13

#define SPEED_BUTTON_SPEED 5
#define LIGHT_BUTTON 4

#define LIGHT_RELAY 2

char *speedPersist = NULL;

homekit_characteristic_t active;
homekit_characteristic_t rotation_speed;

void setLED(uint8_t status) {
  gpio_write(LIGHT_RELAY, status);
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

void identify_task(void *_args) {
  printf("identify_task\n");

  setLED(RELAY_ON);

  vTaskDelay(3000 / portTICK_PERIOD_MS);

  setLED(RELAY_OFF);

  vTaskDelete(NULL);
}

void identify(homekit_value_t _value) {
  printf("identify\n");
  xTaskCreate(identify_task, "identify", 128, NULL, 2, NULL);
}

void reset_configuration_task() {
//  Flash the Light first before we start the reset
  for (int i = 0; i < 5; i++) {
    setLED(RELAY_ON);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    setLED(RELAY_OFF);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  printf("Resetting Wifi Config\n");

  wifi_config_reset();

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  printf("Resetting HomeKit Config\n");

  homekit_server_reset();

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  printf("Restarting\n");

  sdk_system_restart();

  vTaskDelete(NULL);
}

void reset_configuration() {
  printf("Resetting configuration\n");
  xTaskCreate(reset_configuration_task, "Reset configuration", 256, NULL, 2, NULL);
}

void changeSpeed(uint8_t speed) {
  if (speed == SPEED_OFF) {
    gpio_write(SPEED_RELAY_1, RELAY_OFF);
    gpio_write(SPEED_RELAY_2, RELAY_OFF);
    gpio_write(SPEED_RELAY_3, RELAY_OFF);
    gpio_write(SPEED_RELAY_4, RELAY_OFF);
  }

  if (speed > SPEED_OFF && speed < SPEED_1) {
    gpio_write(SPEED_RELAY_1, RELAY_ON);
    gpio_write(SPEED_RELAY_2, RELAY_OFF);
    gpio_write(SPEED_RELAY_3, RELAY_OFF);
    gpio_write(SPEED_RELAY_4, RELAY_OFF);
    speedPersist = "speed1";
  }

  if (speed >= SPEED_1 && speed < SPEED_2) {
    gpio_write(SPEED_RELAY_1, RELAY_OFF);
    gpio_write(SPEED_RELAY_2, RELAY_ON);
    gpio_write(SPEED_RELAY_3, RELAY_OFF);
    gpio_write(SPEED_RELAY_4, RELAY_OFF);

    speedPersist = "speed2";
  }

  if (speed >= SPEED_2 && speed < SPEED_3) {
    gpio_write(SPEED_RELAY_1, RELAY_OFF);
    gpio_write(SPEED_RELAY_2, RELAY_OFF);
    gpio_write(SPEED_RELAY_3, RELAY_ON);
    gpio_write(SPEED_RELAY_4, RELAY_OFF);

    speedPersist = "speed3";
  }

  if (speed >= SPEED_3) {
    gpio_write(SPEED_RELAY_1, RELAY_OFF);
    gpio_write(SPEED_RELAY_2, RELAY_OFF);
    gpio_write(SPEED_RELAY_3, RELAY_OFF);
    gpio_write(SPEED_RELAY_4, RELAY_ON);

    speedPersist = "speed4";
  }

  printf("sysparam_set_string speedPersist :%s \n", speedPersist);
  sysparam_set_string("speedPersist", speedPersist);
}

void update_speed(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
  printf("update_speed  %.0f\n", value.float_value);

  changeSpeed(value.float_value);
}

void update_state(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
  printf("update_state  %d\n", value.int_value);

  changeSpeed(value.bool_value ? rotation_speed.value.float_value : SPEED_OFF);

  char *actionPersist = value.bool_value ? "on" : "off";
  printf("sysparam_set_string actionPersist :%s\n", actionPersist);
  sysparam_set_string("actionPersist", actionPersist);
}

void light_state(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
  printf("light_state  %d\n", value.int_value);

  setLED(!value.int_value);
}

homekit_characteristic_t light_on = HOMEKIT_CHARACTERISTIC_(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(light_state));
homekit_characteristic_t active = HOMEKIT_CHARACTERISTIC_(ON, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(update_state));
homekit_characteristic_t rotation_speed = HOMEKIT_CHARACTERISTIC_(ROTATION_SPEED,SPEED_OFF, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(update_speed));

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id = 1, .category=homekit_accessory_category_fan, .services=(homekit_service_t *[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION,.characteristics = (homekit_characteristic_t *[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Kitchen hood"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Electrolux"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "00001"),
            HOMEKIT_CHARACTERISTIC(MODEL, "001"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
            NULL
        }),
        HOMEKIT_SERVICE(FAN, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Kitchen hood"),
              &active,
              &rotation_speed,
              NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=false, .characteristics=(homekit_characteristic_t*[]){
          HOMEKIT_CHARACTERISTIC(NAME, "Light"),
              &light_on,
              NULL
        }),
        NULL
    }),
    NULL
};

void on_speed_button(const uint8_t gpio, void *args, const uint8_t param) {
  printf("on_speed_button\n");

  if (active.value.bool_value) {
    rotation_speed.value.float_value = rotation_speed.value.float_value + SPEED_STEP;

    if (rotation_speed.value.float_value > SPEED_4) {
      rotation_speed.value.float_value = SPEED_1 - 1;
    }

    homekit_characteristic_notify(&rotation_speed, rotation_speed.value);
  } else {
    active.value.bool_value = !active.value.bool_value;
    homekit_characteristic_notify(&active, active.value);
  }
}

void on_speed_button_long_press(const uint8_t gpio, void *args, const uint8_t param) {
  printf("on_speed_button_long_press\n");

  active.value.bool_value = !active.value.bool_value;
  homekit_characteristic_notify(&active, active.value);

}

void on_light_button(const uint8_t gpio, void *args, const uint8_t param) {
  printf("on_light_button\n");

  light_on.value.bool_value = !light_on.value.bool_value;
  homekit_characteristic_notify(&light_on, light_on.value);
}

void on_light_button_long_press(const uint8_t gpio, void *args, const uint8_t param) {
  printf("on_light_button\n");

  reset_configuration();
}

void init_buttons() {
  adv_button_set_evaluate_delay(10);

  adv_button_create(SPEED_BUTTON_SPEED, true, false);

  adv_button_register_callback_fn(SPEED_BUTTON_SPEED, on_speed_button, SINGLEPRESS_TYPE, NULL, 0);
  adv_button_register_callback_fn(SPEED_BUTTON_SPEED, on_speed_button_long_press, VERYLONGPRESS_TYPE, NULL, 0);

  adv_button_create(LIGHT_BUTTON, true, false);

  adv_button_register_callback_fn(LIGHT_BUTTON, on_light_button, SINGLEPRESS_TYPE, NULL, 0);
//  adv_button_register_callback_fn(LIGHT_BUTTON, on_light_button_long_press, HOLDPRESS_TYPE, NULL, 0);
}

void init_gpio() {
  gpio_enable(SPEED_RELAY_1, GPIO_OUTPUT);
  gpio_enable(SPEED_RELAY_2, GPIO_OUTPUT);
  gpio_enable(SPEED_RELAY_3, GPIO_OUTPUT);
  gpio_enable(SPEED_RELAY_4, GPIO_OUTPUT);

  gpio_enable(LIGHT_RELAY, GPIO_OUTPUT);

  gpio_write(LIGHT_RELAY, RELAY_OFF);

  gpio_write(SPEED_RELAY_1, RELAY_OFF);
  gpio_write(SPEED_RELAY_2, RELAY_OFF);
  gpio_write(SPEED_RELAY_3, RELAY_OFF);
  gpio_write(SPEED_RELAY_4, RELAY_OFF);
}

void init_persist_values() {
  char *actionPersist = NULL;
  sysparam_get_string("actionPersist", &actionPersist);
  sysparam_get_string("speedPersist", &speedPersist);


  if (actionPersist && strcmp(actionPersist, "on") == 0) {
    printf("TURN ON AS LAST STATE\n");

    if (speedPersist) {
      printf("SET LAST SPEED\n");

      if (strcmp(speedPersist, "speed1") == 0) {
        printf("SET FIRST SPEED\n");
        rotation_speed.value = HOMEKIT_FLOAT(SPEED_1);
        speedPersist = "speed1";
      }

      if (strcmp(speedPersist, "speed2") == 0) {
        printf("SET SECOND SPEED\n");
        rotation_speed.value = HOMEKIT_FLOAT(SPEED_2);
        speedPersist = "speed2";
      }

      if (strcmp(speedPersist, "speed3") == 0) {
        printf("SET FIRST SPEED\n");
        rotation_speed.value = HOMEKIT_FLOAT(SPEED_3);
        speedPersist = "speed3";
      }

      if (strcmp(speedPersist, "speed4") == 0) {
        printf("SET SECOND SPEED\n");
        rotation_speed.value = HOMEKIT_FLOAT(SPEED_4);
        speedPersist = "speed4";
      }

      homekit_characteristic_notify(&rotation_speed, rotation_speed.value);
    }

    active.value = HOMEKIT_UINT8(1);
    homekit_characteristic_notify(&active, active.value);
  }
}

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "321-67-123"
};

void on_wifi_ready() {
  homekit_server_init(&config);
  init_persist_values();
  init_buttons();
}

void user_init(void) {
  uart_set_baud(0, 115200);
  init_gpio();

  wifi_init();
  on_wifi_ready();
//  wifi_config_init("Kitchen-hood", NULL, on_wifi_ready);
}
