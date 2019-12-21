#pragma once

#define MUX_CHANNELS 16
#define MUX_CONTROLS 4

void set_mux(uint8_t channel, uint8_t controlPins[MUX_CONTROLS], uint8_t chanelPins[], uint8_t value);

uint8_t get_mux(uint8_t channel, uint8_t controlPins[MUX_CONTROLS], uint8_t chanelPins[]);
