#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include "mux.h"

uint8_t muxChannel[MUX_CHANNELS][MUX_CONTROLS] = {
    {0,0,0,0}, //channel 0
    {1,0,0,0}, //channel 1
    {0,1,0,0}, //channel 2
    {1,1,0,0}, //channel 3
    {0,0,1,0}, //channel 4
    {1,0,1,0}, //channel 5
    {0,1,1,0}, //channel 6
    {1,1,1,0}, //channel 7
    {0,0,0,1}, //channel 8
    {1,0,0,1}, //channel 9
    {0,1,0,1}, //channel 10
    {1,1,0,1}, //channel 11
    {0,0,1,1}, //channel 12
    {1,0,1,1}, //channel 13
    {0,1,1,1}, //channel 14
    {1,1,1,1}  //channel 15
};

void set_mux(uint8_t channel, uint8_t controlPins[MUX_CONTROLS], uint8_t chanelPins[], uint8_t value) {
  uint8_t mux_channel = channel ? channel % MUX_CHANNELS : 0;
  uint8_t mux_number = channel ? channel / MUX_CHANNELS : 0;

  for (uint8_t i = MUX_CONTROLS; i--;) {
    gpio_set_level(controlPins[i], muxChannel[mux_channel][i]);
  }


  gpio_set_level(chanelPins[mux_number], value);
}

uint8_t get_mux(uint8_t channel, uint8_t controlPins[MUX_CONTROLS], uint8_t chanelPins[]) {
  uint8_t mux_channel = channel ? channel % MUX_CHANNELS : 0;
  uint8_t mux_number = channel ? channel / MUX_CHANNELS : 0;

  for (uint8_t i = MUX_CONTROLS; i--;) {
    gpio_set_level(controlPins[i], muxChannel[mux_channel][i]);
  }

  return adc1_get_voltage(chanelPins[mux_number]);
//  return gpio_get_level(chanelPins[mux_number]);
}
