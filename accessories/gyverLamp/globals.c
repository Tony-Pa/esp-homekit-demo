#include <FreeRTOS.h>
#include <math.h>

#include "gyverLamp.h"
#include "globals.h"
#include "libraries/scale8.h"
#include "libraries/power_mgt.h"

// установка макссимальной яркости
void setMaxBrightness(uint8_t globalBrightness) {
  uint8_t recommendedBrightness = calculate_max_brightness_for_power_mW(pixels, LEDS_COUNT, globalBrightness, MAX_POWER_mW);

  if (recommendedBrightness != 255) {
    for (int i = 0; i < LEDS_COUNT; i++) {
      if (!pixels[i].red && !pixels[i].green && !pixels[i].blue) {
        continue;
      }

      pixels[i].red = scale8(pixels[i].red, recommendedBrightness);
      pixels[i].green = scale8(pixels[i].green, recommendedBrightness);
      pixels[i].blue = scale8(pixels[i].blue, recommendedBrightness);
    }
  }
}


// залить все
void fillAll(rgb_t color) {
  for (int i = 0; i < LEDS_COUNT; i++) {
    pixels[i].red = color.red;
    pixels[i].green = color.green;
    pixels[i].blue = color.blue;
  }
}

// получить номер пикселя в ленте по координатам
uint16_t getPixelNumber(int8_t x, int8_t y) {
  if ((y % 2 == 0)) {               // если чётная строка
    return (y * WIDTH + x);
  } else {                          // если нечётная строка
    return (y * WIDTH + WIDTH - x - 1);
  }
}

// функция отрисовки точки по кномеру
void drawPixel(int16_t num, rgb_t color) {
  pixels[num].red = color.red;
  pixels[num].green = color.green;
  pixels[num].blue = color.blue;
}

// функция отрисовки точки по координатам X Y
void drawPixelXY(int8_t x, int8_t y, rgb_t color) {
  if (x < 0 || x > WIDTH - 1 || y < 0 || y > HEIGHT - 1) return;
  int thisPixel = getPixelNumber(x, y);

  pixels[thisPixel].red = color.red;
  pixels[thisPixel].green = color.green;
  pixels[thisPixel].blue = color.blue;
}

void drawPixelXYbyHEX(int8_t x, int8_t y, uint32_t color) {
  if (x < 0 || x > WIDTH - 1 || y < 0 || y > HEIGHT - 1) return;
  int thisPixel = getPixelNumber(x, y);

  pixels[thisPixel].red = ((color >> 16) & 0xFF) / 255.0;  // Extract the RR byte
  pixels[thisPixel].green = ((color >> 8) & 0xFF) / 255.0;   // Extract the GG byte
  pixels[thisPixel].blue = ((color) & 0xFF) / 255.0;        // Extract the BB byte
}

// функция получения цвета пикселя по его номеру
rgb_t getPixColor(int thisPixel) {
  rgb_t color;
  if (thisPixel < 0 || thisPixel > LEDS_COUNT - 1) return color;

  color.green = pixels[thisPixel].green;
  color.blue = pixels[thisPixel].blue;
  color.red = pixels[thisPixel].red;
  return color;
}

// функция получения цвета пикселя в матрице по его координатам
rgb_t getPixColorXY(int8_t x, int8_t y) {
  return getPixColor(getPixelNumber(x, y));
}

// функция получения номера цвета пикселя по его номеру
uint32_t getPixHEX(int thisPixel) {
  if (thisPixel < 0 || thisPixel > LEDS_COUNT - 1) return 0;
  return (((uint32_t) pixels[thisPixel].red << 16) | ((long) pixels[thisPixel].green << 8) |
          (long) pixels[thisPixel].blue);
}

// функция получения цвета пикселя в матрице по его координатам
uint32_t getPixHEXXY(int8_t x, int8_t y) {
  return getPixHEX(getPixelNumber(x, y));
}

int32_t constrain(int32_t val, int32_t min, int32_t max) {
  if (val < min) {
    return min;
  }

  if (val > max) {
    return max;
  }

  return val;
}

#define LED_RGB_SCALE 255
rgb_t hsv_to_rgb(float h, float s, float i) {
  int r, g, b;
  rgb_t rgb;

  while (h < 0) { h += 360.0F; };     // cycle h around to 0-360 degrees
  while (h >= 360) { h -= 360.0F; };
  h = 3.14159F*h / 180.0F;            // convert to radians.
  s /= 100.0F;                        // from percentage to ratio
  i /= 100.0F;                        // from percentage to ratio
  s = s > 0 ? (s < 1 ? s : 1) : 0;    // clamp s and i to interval [0,1]
  i = i > 0 ? (i < 1 ? i : 1) : 0;    // clamp s and i to interval [0,1]
  i = i * sqrt(i);                    // shape intensity to have finer granularity near 0

  if (h < 2.09439) {
    r = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
    g = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
    b = LED_RGB_SCALE * i / 3 * (1 - s);
  }
  else if (h < 4.188787) {
    h = h - 2.09439;
    g = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
    b = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
    r = LED_RGB_SCALE * i / 3 * (1 - s);
  }
  else {
    h = h - 4.188787;
    b = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
    r = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
    g = LED_RGB_SCALE * i / 3 * (1 - s);
  }

  rgb.red = (uint8_t) r;
  rgb.green = (uint8_t) g;
  rgb.blue = (uint8_t) b;

  return rgb;
}

hsv_t rgb_to_hsv(rgb_t rgb) {
  hsv_t hsv;
  unsigned char rgbMin, rgbMax;

  rgbMin = rgb.red < rgb.green ? (rgb.red < rgb.blue ? rgb.red : rgb.blue) : (rgb.green < rgb.blue ? rgb.green : rgb.blue);
  rgbMax = rgb.red > rgb.green ? (rgb.red > rgb.blue ? rgb.red : rgb.blue) : (rgb.green > rgb.blue ? rgb.green : rgb.blue);

  hsv.v = rgbMax;
  if (hsv.v == 0)
  {
    hsv.h = 0;
    hsv.s = 0;
    return hsv;
  }

  hsv.s = 255 * (rgbMax - rgbMin) / hsv.v;
  if (hsv.s == 0)
  {
    hsv.h = 0;
    return hsv;
  }

  if (rgbMax == rgb.red)
    hsv.h = 0 + 43 * (rgb.green - rgb.blue) / (rgbMax - rgbMin);
  else if (rgbMax == rgb.green)
    hsv.h = 85 + 43 * (rgb.blue - rgb.red) / (rgbMax - rgbMin);
  else
    hsv.h = 171 + 43 * (rgb.red - rgb.green) / (rgbMax - rgbMin);

  return hsv;
}

