/* COLORUTILS.H - Constants and variable type definitions for ColorUtils.cpp. See SysConfig.h for related parameters.
*/

#include <Arduino.h>


#ifndef _COLOR_TYPES  // prevent duplicate type definitions when this file is included in multiple places
#define _COLOR_TYPES

/* Struct used to represent HSV colors. Each component value ranges from 0 - 1.
*/
struct hsvF {  
  float h;
  float s;
  float v;
};

/* Struct used to represent RGB and RGBW colors after being converted from HSV (hsvF). Each component value ranges from 0 - 1. 
    Use of the w component (white) is optional.
*/
struct rgbwF {  
  float r;
  float g;
  float b;
  float w;
};

/* Struct used to represent RGB and RGBW colors in unsigned integer format, using either 8, 12 or 16 bits, depending on the method
    used to convert from the rgbwF (float) format to rgbwI. Use of the w component (white) is optional.
*/
struct rgbwI {
  uint16_t r;
  uint16_t g;
  uint16_t b;
  uint16_t w;
};

#endif // _COLOR_TYPES

rgbwF Hsv2Rgb(hsvF hsv, bool wFlag);
rgbwI RgbF2RgbI(rgbwF in, uint8_t bits);
uint16_t Dither(float val); 
float HueDistance(float startH, float endH);
float WrapHue(float h);
hsvF BlendHsv(hsvF color1, hsvF color2, float scaleV2);
hsvF InterpHsv(hsvF color1, hsvF color2, float ctrl);
float Interpolate(float val1, float val2, float ctrl);
