#ifndef POWER_MGT_H
#define POWER_MGT_H

///@defgroup Power Power management functions
/// functions used to limit the amount of power
///@{

// Power Control internal helper functions

/// calculate_unscaled_power_mW tells you how many milliwatts the current
///   LED data would draw at brightness = 255.
///
uint32_t calculate_unscaled_power_mW( rgb_t pixels[], uint16_t numLeds);

/// calculate_max_brightness_for_power_mW tells you the highest brightness
///   level you can use and still stay under the specified power budget for 
///   a given set of leds.  It takes a pointer to an array of CRGB objects, a
///   count, a 'target brightness' which is the brightness you'd ideally like
///   to use, and the max power draw desired in milliwatts.  The result from 
///   this function will be no higher than the target_brightess you supply, but may be lower.
uint8_t calculate_max_brightness_for_power_mW( rgb_t pixels[], uint16_t numLeds, uint8_t target_brightness, uint32_t max_power_mW);

///@}
// POWER_MGT_H

#endif
