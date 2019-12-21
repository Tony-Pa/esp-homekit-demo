#pragma once
#ifndef MY_GLOBALS_H
#define MY_GLOBALS_H

typedef struct ts_accessory {
  char *name;
  uint32_t relay;
  uint32_t sensor;
} ts_accessory;

extern ts_accessory accessories_config[];

#endif
