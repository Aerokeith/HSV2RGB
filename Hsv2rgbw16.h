#include <Arduino.h>
#include <FastLED.h>

#ifndef _COLOR_TYPES
#define _COLOR_TYPES
struct HSV16 {
  uint16_t h;
  uint16_t s;
  uint16_t v;
};

struct RGBW16 {
  uint16_t r;
  uint16_t g;
  uint16_t b;
  uint16_t w;
};

struct HSVFloat {   // floating point version of HSV16 for high-precision fade step computations
  float h;
  float s;
  float v;
};

#endif // _COLOR_TYPES

void HSV16toRGBW16(HSV16, RGBW16 *);
HSV16 HSV8toHSV16(CHSV);
CHSV HSV16toHSV8(HSV16);
