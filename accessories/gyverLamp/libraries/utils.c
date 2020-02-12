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
