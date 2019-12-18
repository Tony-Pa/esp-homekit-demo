#include "mux.h"

#define MUX_CHANNELS 16
#define MUX_CONTROLS 4

uint32_t muxChannel[MUX_CHANNELS][MUX_CONTROLS] = {
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

void set_mux(uint32_t channel, uint32_t controlPins[MUX_CONTROLS]) {
  int mux_channel = channel ? channel % MUX_CHANNELS : 0;

  for (uint32_t i = MUX_CONTROLS; i--;)
    gpio_set_level(controlPins[i], muxChannel[channel][i]);
  }
}