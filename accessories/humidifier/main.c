#include <stdio.h>
#include <string.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <sysparam.h>

#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

#include "button.h"
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

#define SPEED_OFF 0x00
#define SPEED_1 50
#define SPEED_2 100

#define LOW 0x00
#define HIGH 0x01

#define RELAY_ON 0x00
#define RELAY_OFF 0x01

#define LPF_INTERVAL 100  // in milliseconds

#define LIMIT_SWITCH 16
#define SPEED_RELAY_1 13
#define SPEED_RELAY_2 14

#define SPEED_BUTTON_1 5
#define SPEED_BUTTON_2 4

#define RED_PWM_PIN 2
#define GREEN_PWM_PIN 12
#define BLUE_PWM_PIN 0

pwm_info_t pwm_info;

uint8_t speedStatus = 0;
bool remoteControl = false;

typedef struct rgb_color_t {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} rgb_color_t;

rgb_color_t colors[6] = {
    {0x00, 0x00, 0x00}, // off
    {0xff, 0xff, 0xff}, // white
    {0xff, 0x00, 0x00}, // red
    {0x00, 0xff, 0x00}, // green
    {0x00, 0x00, 0xff}, // blue
    {0xff, 0x00, 0xff}  // violet
};

rgb_color_t current_color;

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Boneco Humidifier");
homekit_characteristic_t current_relative_humidity = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);
homekit_characteristic_t current_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HUMIDIFIER_DEHUMIDIFIER_STATE, 0);
homekit_characteristic_t target_state = HOMEKIT_CHARACTERISTIC_(TARGET_HUMIDIFIER_DEHUMIDIFIER_STATE, 0);
homekit_characteristic_t water_level;
homekit_characteristic_t active;
homekit_characteristic_t rotation_speed;

void setCurrentColor(rgb_color_t color) {
  current_color = color;

  gpio_write(RED_PWM_PIN, color.red);
  gpio_write(GREEN_PWM_PIN, color.green);
  gpio_write(BLUE_PWM_PIN, color.blue);
}

void reset_configuration_task() {
  //Flash the LED first before we start the reset
  for (int i=0; i<5; i++) {
    setCurrentColor(colors[1]);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    setCurrentColor(colors[0]);
    vTaskDelay(100 / portTICK_PERIOD_MS);
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
  printf("Resetting Sonoff configuration\n");
  xTaskCreate(reset_configuration_task, "Reset configuration", 256, NULL, 2, NULL);
}

void led_identify_task(void *_args) {
  printf("LED identify\n");

  rgb_color_t color = current_color;

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      setCurrentColor(colors[0]);
      vTaskDelay(100 / portTICK_PERIOD_MS);

      setCurrentColor(colors[1]);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelay(250 / portTICK_PERIOD_MS);
  }

  setCurrentColor(color);

  vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
  printf("identify\n");
  xTaskCreate(led_identify_task, "identify", 128, NULL, 2, NULL);
}

void changeStatus(uint8_t speed) {
  char *speedPersist;
  if (speed == 0) {
    gpio_write(SPEED_RELAY_1, RELAY_OFF);
    gpio_write(SPEED_RELAY_2, RELAY_OFF);
  }

  if (speed > 0) {
    gpio_write(SPEED_RELAY_1, RELAY_ON);
    gpio_write(SPEED_RELAY_2, RELAY_OFF);

    speedPersist = "speed1";
    printf("sysparam_set_string speedPersist :%s \n", speedPersist);
    sysparam_set_string("speedPersist", speedPersist);
  }

  if (speed > 50) {
    gpio_write(SPEED_RELAY_1, RELAY_OFF);
    gpio_write(SPEED_RELAY_2, RELAY_ON);

    speedPersist = "speed2";
    printf("sysparam_set_string speedPersist :%s \n", speedPersist);
    sysparam_set_string("speedPersist", speedPersist);
  }
}

void update_waterLevel(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
  printf("update_waterLevel  %.0f\n", value.float_value);

  if (value.float_value == 0) {
    setCurrentColor(colors[2]);

    char *actionP = active.value.int_value ? "on" : "off";
    printf("sysparam_set_string actionPersistWL :%s \n", actionP);
    sysparam_set_string("actionPersistWL", actionP);

    active.value = HOMEKIT_UINT8(0);
    homekit_characteristic_notify(&active, active.value);
  }
  else {
    char *actionPersist = NULL;
    sysparam_get_string("actionPersistWL", &actionPersist);

    if (actionPersist && strcmp(actionPersist, "on") == 0) {
      printf("TURN ON AS PREV STATE\n");

      active.value = HOMEKIT_UINT8(1);
      homekit_characteristic_notify(&active, active.value);
    }
  }
}

void update_speed(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
  printf("update_speed  %.0f\n", value.float_value);

  if (active.value.bool_value) {
    changeStatus(value.float_value);
  }
}

void update_state(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
  printf("update_state  %d\n", value.int_value);

  if (value.bool_value) {
    changeStatus(rotation_speed.value.float_value);
    // ставим синий цвет
    setCurrentColor(colors[4]);
  }
  else {
    changeStatus(SPEED_OFF);

    if (water_level.value.float_value != 0) {
      // ставим фиолетовый цвет
      setCurrentColor(colors[5]);
    }
  }

  char *actionPersist = value.bool_value ? "on" : "off";
  printf("sysparam_set_string actionPersist :%s\n", actionPersist);
  sysparam_set_string("actionPersist", actionPersist);
}

void setWaterLevel(float waterLevel) {
  printf("setWaterLevel: old: %.0f - new:%.0f\n", water_level.value.float_value, waterLevel);

  if (water_level.value.float_value != waterLevel) {
    printf("setWaterLevel: %.0f\n", waterLevel);

    water_level.value = HOMEKIT_FLOAT(waterLevel);
    homekit_characteristic_notify(&water_level, water_level.value);
  }
}

homekit_characteristic_t water_level = HOMEKIT_CHARACTERISTIC_(WATER_LEVEL, 50, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(update_waterLevel));
homekit_characteristic_t active = HOMEKIT_CHARACTERISTIC_(ACTIVE, false, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(update_state));
homekit_characteristic_t rotation_speed = HOMEKIT_CHARACTERISTIC_(ROTATION_SPEED, 50, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(update_speed));

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id = 1, .category=homekit_accessory_category_humidifier, .services=(homekit_service_t *[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION,.characteristics = (homekit_characteristic_t *[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Boneco smart"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Boneco"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "E2441A0001"),
            HOMEKIT_CHARACTERISTIC(MODEL, "E2441A"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
        HOMEKIT_SERVICE(HUMIDIFIER_DEHUMIDIFIER, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            &name,
            &active,
            &current_relative_humidity,
            &current_state,
            &target_state,
            &rotation_speed,
            &water_level,
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

void init_gpio() {
  gpio_enable(LIMIT_SWITCH, GPIO_INPUT);

  gpio_enable(SPEED_RELAY_1, GPIO_OUTPUT);
  gpio_enable(SPEED_RELAY_2, GPIO_OUTPUT);

  gpio_enable(SPEED_BUTTON_1, GPIO_INPUT);
  gpio_enable(SPEED_BUTTON_2, GPIO_INPUT);

  gpio_write(SPEED_RELAY_1, RELAY_OFF);
  gpio_write(SPEED_RELAY_2, RELAY_OFF);
}

void init_persist_values() {
  char *actionPersist = NULL;
  char *speedPersist = NULL;
  sysparam_get_string("actionPersist", &actionPersist);
  sysparam_get_string("speedPersist", &speedPersist);

  if (actionPersist && strcmp(actionPersist, "on") == 0) {
    printf("TURN ON AS LAST STATE\n");

    if (speedPersist) {
      printf("SET LAST SPEED\n");

      if (strcmp(speedPersist, "speed1") == 0) {
        printf("SET FIRST SPEED\n");
        rotation_speed.value = HOMEKIT_FLOAT(SPEED_1);
      }

      if (strcmp(speedPersist, "speed2") == 0) {
        printf("SET SECOND SPEED\n");
        rotation_speed.value = HOMEKIT_FLOAT(SPEED_2);
      }

      homekit_characteristic_notify(&rotation_speed, rotation_speed.value);
    }

    active.value = HOMEKIT_UINT8(1);
    homekit_characteristic_notify(&active, active.value);
  }
}

void on_speed_button(uint8_t gpio, button_event_t event) {
  printf("on_speed_button\n");

  switch (event) {
    case button_event_single_press:
      if (active.value.bool_value) {
        rotation_speed.value.float_value = rotation_speed.value.float_value + SPEED_1;

        if (rotation_speed.value.float_value > SPEED_2) {
          rotation_speed.value.float_value = SPEED_1 / 2;
        }

        homekit_characteristic_notify(&rotation_speed, rotation_speed.value);
      }
      break;
    case button_event_long_press:
      active.value.bool_value = !active.value.bool_value;
      homekit_characteristic_notify(&active, active.value);
      break;
    case button_event_very_long_press:
      reset_configuration();
      break;
    default:
      printf("Unknown button event: %d\n", event);
  }
}

void init_buttons() {
  if (button_create(SPEED_BUTTON_1, 1, 1000, 10000, on_speed_button)) {
    printf("Failed to initialize speed status button\n");
  }
}

IRAM void controls_task(void *pvParameters) {
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  while (1) {
    bool waterLevelAlert = gpio_read(LIMIT_SWITCH) == LOW

    setWaterLevel(waterLevelAlert ? 0 : 100);

    vTaskDelay(LPF_INTERVAL * 10000 / portTICK_PERIOD_MS);
  }
}

void on_wifi_ready() {
  homekit_server_init(&config);
  init_persist_values();

  xTaskCreate(controls_task, "checkControls", 512, NULL, 2, NULL);
}

void create_accessory_name() {
  uint8_t macaddr[6];
  sdk_wifi_get_macaddr(STATION_IF, macaddr);

  int name_len = snprintf(NULL, 0, "Boneco Humidifier %02X:%02X:%02X",
                          macaddr[3], macaddr[4], macaddr[5]);
  char *name_value = malloc(name_len + 1);
  snprintf(name_value, name_len + 1, "Boneco Humidifier %02X:%02X:%02X",
           macaddr[3], macaddr[4], macaddr[5]);

  name.value = HOMEKIT_STRING(name_value);
}

void user_init(void) {
  uart_set_baud(0, 115200);
  create_accessory_name();
  init_gpio();
  setCurrentColor(colors[0]);

//  wifi_config_init("boneco-humidifier", NULL, on_wifi_ready);
  wifi_init();
  on_wifi_ready();
  water_level.value = HOMEKIT_FLOAT(50); // ?
}
