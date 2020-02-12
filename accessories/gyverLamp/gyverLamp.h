#define LOW 0
#define HIGH 1


#define WIDTH 16              // ширина матрицы
#define HEIGHT 16             // высота матрицы

#define LEDS_COUNT  256
#define LEDS_SATURATION 160
#define LEDS_LUMINANCE 40
#define HUE_STEP 3

#include "globals.h"


unsigned char matrixValue[8][16];
int scale = 1;
byte hue;
int globalBrightness = 255;



// -------------------------------------- огонь ---------------------------------------------


// эффект "огонь"
#define SPARKLES 1        // вылетающие угольки вкл выкл
int line[WIDTH];
int pcnt = 0;
bool loadingFlag = true;
//these values are substracetd from the generated values to give a shape to the animation
const int valueMask[8][16] = {
    {32,  0,   0,   0,   0,   0,   0,   32,  32,  0,   0,   0,   0,   0,   0,   32},
    {64,  0,   0,   0,   0,   0,   0,   64,  64,  0,   0,   0,   0,   0,   0,   64},
    {96,  32,  0,   0,   0,   0,   32,  96,  96,  32,  0,   0,   0,   0,   32,  96},
    {128, 64,  32,  0,   0,   32,  64,  128, 128, 64,  32,  0,   0,   32,  64,  128},
    {160, 96,  64,  32,  32,  64,  96,  160, 160, 96,  64,  32,  32,  64,  96,  160},
    {192, 128, 96,  64,  64,  96,  128, 192, 192, 128, 96,  64,  64,  96,  128, 192},
    {255, 160, 128, 96,  96,  128, 160, 255, 255, 160, 128, 96,  96,  128, 160, 255},
    {255, 192, 160, 128, 128, 160, 192, 255, 255, 192, 160, 128, 128, 160, 192, 255}
};

//these are the hues for the fire,
//should be between 0 (red) to about 25 (yellow)
const int hueMask[8][16] = {
    {1, 11, 19, 25, 25, 22, 11, 1, 1, 11, 19, 25, 25, 22, 11, 1},
    {1, 8,  13, 19, 25, 19, 8,  1, 1, 8,  13, 19, 25, 19, 8,  1},
    {1, 8,  13, 16, 19, 16, 8,  1, 1, 8,  13, 16, 19, 16, 8,  1},
    {1, 5,  11, 13, 13, 13, 5,  1, 1, 5,  11, 13, 13, 13, 5,  1},
    {1, 5,  11, 11, 11, 11, 5,  1, 1, 5,  11, 11, 11, 11, 5,  1},
    {0, 1,  5,  8,  8,  5,  1,  0, 0, 1,  5,  8,  8,  5,  1,  0},
    {0, 0,  1,  5,  5,  1,  0,  0, 0, 0,  1,  5,  5,  1,  0,  0},
    {0, 0,  0,  1,  1,  0,  0,  0, 0, 0,  0,  1,  1,  0,  0,  0}
};

void fireRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    //FastLED.clear();
//    fillAll(blackRGB);
    generateLine();
  }
  if (pcnt >= 100) {
    shiftUp();
    generateLine();
    pcnt = 0;
  }
  drawFrame(pcnt);
  pcnt += 30;
}

// Randomly generate the next line (matrix row)

void generateLine() {
  for (uint8_t x = 0; x < WIDTH; x++) {
    line[x] = hwrand() % 191 + 64;
  }
}

void shiftUp() {
  for (uint8_t y = 7; y > 0; y--) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      uint8_t newX = x;
      if (x > 15) newX = x - 15;
      matrixValue[y][newX] = matrixValue[y - 1][newX];
    }
  }

  for (uint8_t x = 0; x < WIDTH; x++) {
    uint8_t newX = x;
    if (x > 15) newX = x - 15;
    matrixValue[0][newX] = line[newX];
  }
}

// draw a frame, interpolating between 2 "key frames"
// @param pcnt percentage of interpolation

void drawFrame(int pcnt) {
  int nextv;
  rgb_t color;

  //each row interpolates with the one before it
  for (unsigned char y = HEIGHT - 1; y > 0; y--) {
    for (unsigned char x = 0; x < WIDTH; x++) {
      if (y < 8) {
        nextv = (((100 - pcnt) * matrixValue[y][x] + pcnt * matrixValue[y - 1][x]) / 100) - valueMask[y][x];
        int16_t hue = scale * 2.5 + hueMask[y][x];
        hsv_to_rgb(hue, 255, (uint8_t) max(0, nextv), &color);

        drawPixelXY(x, y, color);

      } else if (y == 8 && SPARKLES) {
        if ((hwrand() % 20) == 0 && getPixHEXXY(x, y - 1) != 0) drawPixelXY(x, y, getPixColorXY(x, y - 1));
        else drawPixelXYbyHEX(x, y, 0);
      } else if (SPARKLES) {

        // старая версия для яркости
        if (getPixHEXXY(x, y - 1) > 0)
          drawPixelXY(x, y, getPixColorXY(x, y - 1));
        else drawPixelXYbyHEX(x, y, 0);
      }
    }
  }

  //first row interpolates with the "next" line
  for (uint8_t x = 0; x < WIDTH; x++) {
    uint8_t newX = x;
    if (x > 15) newX = x - 15;

    rgb_t color;
    int hue = scale * 2.5 + hueMask[0][newX];
    int val = ((100 - pcnt) * matrixValue[0][newX] + pcnt * line[newX]) / 100;
    hsv_to_rgb(hue, 255, val, &color);

    drawPixelXY(newX, 0, color);
  }
}

// ---------------------------------------- цвета ------------------------------------------


void colorsRoutine() {
//  hue += scale;
  for (int i = 0; i < LEDS_COUNT; i++) {
    rgb_t color;
    hsv_to_rgb(hue, 255, 255, &color);

    pixels[i].red = color.red;
    pixels[i].green = color.green;
    pixels[i].blue = color.blue;
  }
}

void colorsRoutine2() {
//  hue += scale;
  fillAll(blackRGB);

  for (unsigned char y = 8; y > 0; y--) {
    for (unsigned char x = 0; x < 16; x++) {
      rgb_t color;

      int val = valueMask[y][x];
      hsv_to_rgb(hue, 255, val, &color);

      drawPixelXY(x, y, color);
    }
  }

}


// ---------------------------------------- радуга ------------------------------------------
void rainbowVertical() {
  rgb_t thisColor;
  hue += 2;
  for (byte j = 0; j < HEIGHT; j++) {
    hsv_to_rgb(hue + j * 40, LEDS_SATURATION, LEDS_LUMINANCE, &thisColor);
    for (byte i = 0; i < WIDTH; i++)
      drawPixelXY(i, j, thisColor);
  }
}

void rainbowHorizontal() {
  rgb_t thisColor;
  hue += 2;
  for (byte i = 0; i < WIDTH; i++) {
    hsv_to_rgb(hue + i * 40, LEDS_SATURATION, LEDS_LUMINANCE, &thisColor);
    for (byte j = 0; j < HEIGHT; j++)
      drawPixelXY(i, j, thisColor);
  }
}

// ------------------------------ снегопад 2.0 --------------------------------
void snowRoutine() {
  // сдвигаем всё вниз
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT - 1; y++) {
      drawPixelXY(x, y, getPixColorXY(x, y + 1));
    }
  }

  for (byte x = 0; x < WIDTH; x++) {
    // заполняем случайно верхнюю строку
    // а также не даём двум блокам по вертикали вместе быть
    if (getPixHEXXY(x, HEIGHT - 2) == 0 && ((hwrand() % 40) == 0))
      drawPixelXYbyHEX(x, HEIGHT - 1, 0xE0FFFF - 0x101010 * (hwrand() % 4));
    else
      drawPixelXYbyHEX(x, HEIGHT - 1, 0x000000);
  }
}

// ------------------------------ МАТРИЦА ------------------------------
void matrixRoutine() {
  for (byte x = 0; x < WIDTH; x++) {
    // заполняем случайно верхнюю строку
    uint32_t thisColor = getPixHEXXY(x, HEIGHT - 1);
    if (thisColor == 0)
      drawPixelXYbyHEX(x, HEIGHT - 1, 0x00FF00 * ((hwrand() % 40) == 0));
    else if (thisColor < 0x002000)
      drawPixelXYbyHEX(x, HEIGHT - 1, 0);
    else
      drawPixelXYbyHEX(x, HEIGHT - 1, thisColor - 0x002000);
  }

  // сдвигаем всё вниз
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT - 1; y++) {
      drawPixelXY(x, y, getPixColorXY(x, y + 1));
    }
  }
}

// ----------------------------- СВЕТЛЯКИ 2.0 ------------------------------
#define LIGHTERS_AM 100
typedef struct {
  int16_t x;
  int16_t y;
  int16_t xSpeed;
  int16_t ySpeed;
  rgb_t color;
  uint8_t changeSpeedCounter;
} lighters_t;

lighters_t lighters[LIGHTERS_AM];

bool lightersRoutineFirstStart = true;
void lightersRoutine() {
  if (lightersRoutineFirstStart) {
    lightersRoutineFirstStart = false;
    for (byte i = 0; i < LIGHTERS_AM; i++) {
      lighters[i].x = (hwrand() % WIDTH) * 10;
      lighters[i].y = (hwrand() % HEIGHT) * 10;
      lighters[i].xSpeed = (hwrand() % 20) - 10;
      lighters[i].ySpeed = (hwrand() % 20) - 10;
      lighters[i].changeSpeedCounter = (hwrand() % 20) + 20;

      hsv_to_rgb(hwrand() % 255, 255, 255, &lighters[i].color);
    }
  }

  fillAll(blackRGB);

  for (byte i = 0; i < 50; i++) {
    lighters[i].changeSpeedCounter -= 1;
    if (!lighters[i].changeSpeedCounter) {     // меняем скорость
      lighters[i].xSpeed += (hwrand() % 10) - 5;
      lighters[i].ySpeed += (hwrand() % 10) - 5;
      lighters[i].xSpeed = constrain(lighters[i].xSpeed, -20, 20);
      lighters[i].ySpeed = constrain(lighters[i].ySpeed, -20, 20);
      lighters[i].changeSpeedCounter = (hwrand() % 10) + 10;
    }

    lighters[i].x += lighters[i].xSpeed;
    lighters[i].y += lighters[i].ySpeed;

    if (lighters[i].x <= 0) lighters[i].x = (WIDTH - 1) * 10;
    if (lighters[i].x >= WIDTH * 10) lighters[i].x = 0;

    if (lighters[i].y < 0) {
      lighters[i].y = 0;
      lighters[i].ySpeed = -lighters[i].ySpeed;
    }
    if (lighters[i].y >= (HEIGHT - 1) * 10) {
      lighters[i].y = (HEIGHT - 1) * 10;
      lighters[i].ySpeed = -lighters[i].ySpeed;
    }
    drawPixelXY(lighters[i].x / 10, lighters[i].y / 10, lighters[i].color);
  }
}

// -----------------------------------------------------------

void hsv_to_rgb(uint8_t hsv_h, uint8_t hsv_s, uint8_t hsv_v, rgb_t *rgb) {
  uint8_t sector, frac, p, q, t;

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
}


typedef struct hsv_t
{
  unsigned char h;
  unsigned char s;
  unsigned char v;
} hsv_t;


hsv_t rgb_to_hsv(rgb_t rgb)
{
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
