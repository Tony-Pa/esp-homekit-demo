#pragma once
#ifndef MY_GLOBALS_H
#define MY_GLOBALS_H

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} rgb_t;

typedef struct hsv_t {
  unsigned char h;
  unsigned char s;
  unsigned char v;
} hsv_t;

extern rgb_t blackRGB = {0, 0, 0};

extern ws2812_pixel_t pixels[LEDS_COUNT];

// залить все
void fillAll(rgb_t color);

// получить номер пикселя в ленте по координатам
uint16_t getPixelNumber(int8_t x, int8_t y);

// функции отрисовки точки по координатам X Y по номеру цвета
void drawPixelXY(int8_t x, int8_t y, rgb_t color);

// функции отрисовки точки по координатам X Y
void drawPixelXYbyHEX(int8_t x, int8_t y, uint32_t color);

// функция получения цвета пикселя по его номеру
rgb_t getPixColor(int thisPixel);

// функция получения цвета пикселя в матрице по его координатам
rgb_t getPixColorXY(int8_t x, int8_t y);

// функция получения номера цвета пикселя по его номеру
uint32_t getPixHEX(int thisPixel);

// функция получения цвета пикселя в матрице по его координатам
uint32_t getPixHEXXY(int8_t x, int8_t y);

// функция установки лимитов
int32_t constrain(int32_t val, int32_t min, int32_t max);


rgb_t hsv_to_rgb(uint8_t hsv_h, uint8_t hsv_s, uint8_t hsv_v);

hsv_t rgb_to_hsv(rgb_t rgb);

#endif
