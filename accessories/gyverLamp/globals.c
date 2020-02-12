#import "globals.h"

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


void hsv_to_rgb(uint8_t hsv_h, uint8_t hsv_s, uint8_t hsv_v) {
  uint8_t sector, frac, p, q, t;
  rgb_t rgb;

  if (hsv_s == 0) {  // achromatic
    rgb->red = rgb->green = rgb->blue = hsv_v;
    return;
  }

  sector = hsv_h / 43;
  frac = (hsv_h - (sector * 43)) * 6;

  p = (hsv_v * (255 - hsv_s)) >> 8;
  q = (hsv_v * (255 - ((hsv_s * frac) >> 8))) >> 8;
  t = (hsv_v * (255 - ((hsv_s * (255 - frac)) >> 8))) >> 8;

  switch (sector) {
    case 0:
      rgb->red = hsv_v;
      rgb->green = t;
      rgb->blue = p;
      break;
    case 1:
      rgb->red = q;
      rgb->green = hsv_v;
      rgb->blue = p;
      break;
    case 2:
      rgb->red = p;
      rgb->green = hsv_v;
      rgb->blue = t;
      break;
    case 3:
      rgb->red = p;
      rgb->green = q;
      rgb->blue = hsv_v;
      break;
    case 4:
      rgb->red = t;
      rgb->green = p;
      rgb->blue = hsv_v;
      break;
    default:        // case 5:
      rgb->red = hsv_v;
      rgb->green = p;
      rgb->blue = q;
      break;
  }

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

