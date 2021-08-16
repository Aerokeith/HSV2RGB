/* COLORUTILS.CPP - Functions for manipulating and converting HSV and RGB color representations using floating point formats. 
*/

#include <Arduino.h>
#include "SysConfig.h"
#include "Brightness.h"
#include "ColorUtils.h"

extern uint32_t frameCount;   // frame count maintained in main.cpp; used for dithering
extern brightness globalBrightness; // object defined in Brightness.cpp


/* Hsv2Rgb() implements the HSV to RGBW color space conversion algorithm defined in https://www.easyrgb.com/en/math.php and
    https://www.cs.rit.edu/~ncs/color/t_convert.html and https://en.wikipedia.org/wiki/HSL_and_HSV#From_HSV. 
    The use of white is limited to the case of a fully-unsatured HSV color (s == 0), where the white brightness (w) is set
    to the HSV brightness value (v) and the RGB LEDs are not used (r=g=b=0).
    The conversion algorithm also performs Gamma correction to compensate for the non-linear perception of brightness variations 
    by the human eye, as well as color scaling to compensate for per-color brightness variations for specific LED types.
*/
rgbwF Hsv2Rgb(hsvF hsv, bool wFlag) {
  float h6, f;
  uint8_t hi; // hue range index (0 - 5)  
  float p, q, t;
  float r, g, b, w; // temp rgbw values
  
  hsv.h = constrain(hsv.h, 0, 1);   // ensure that all components of hsv are in range 0-1
  hsv.s = constrain(hsv.s, 0, 1);
  hsv.v = constrain(hsv.v, 0, 1);
  hsv.v *=  globalBrightness.getVal(); // scale v by globalBrightness factor (0-1)
  if (hsv.s == 0) {   // if fully unsaturated (no color)
    if (wFlag) {  // use white LED is available
      r = 0; g = 0; b = 0;  // turn off RGB LEDs
      w = hsv.v;  // white LED brightness = v
    }
    else {  // no white LED available
      r = hsv.v; 
      g = r; b = r; // set r=g=b=v
      w = 0; 
    }
  }
  else {
    w = 0;  // don't use w if not fully unsaturated
    if (hsv.h == 1) {   // if at end of hue range, wrap around to h = 0
      hi = 0;
      f = 0;
    }
    else {
      h6 = hsv.h * 6; // floating point version of hue index (0.0 - 5.9999)
      hi = (uint8_t) h6;  // get integer part (0 - 5)
      f = h6 - hi;  // get fractional part (0.0 - 0.9999)
    }
    p = (hsv.v * (1 - hsv.s));  // also called var_1 in some algorithms
    q = (hsv.v * (1 - (f * hsv.s)));  // var_2
    t = (hsv.v * (1 - ((1 - f) * hsv.s)));  // var_3
    switch (hi) {
      case 0: r = hsv.v;  g = t;  b = p;
      break;
      case 1: r = q;  g = hsv.v;  b = p;
      break;
      case 2: r = p;  g = hsv.v;  b = t;
      break;
      case 3: r = p;  g = q;  b = hsv.v;
      break;
      case 4: r = t;  g = p;  b = hsv.v;
      break;
      case 5: r = hsv.v;  g = p;  b = q;
      break;
      default: r = 0; g = 0;  b = 0;  // should never get here
      break;
    }
  }
    // perform basic Gamma correction
  r = pow(r, LED_GAMMA); 
  g = pow(g, LED_GAMMA);   
  b = pow(b, LED_GAMMA);   
  w = pow(w, LED_GAMMA); 
  
    // scale each color component to account for major differences in brightness (dependent on LED type)
  r *= SCALE_RED;
  g *= SCALE_GRN;
  b *= SCALE_BLU;

  return {r, g, b, w};
}


/* RgbF2RgbI() concerts a floating point structure of type rgbwF to an equivalent integer structure of type rgbwI. The floating point
    values are assumed to be in the range 0 - 1, and before integer conversion they are upscaled based on a parameter (bits) that indicates
    the scaled range. 
    bits = 8: range = 0 - 0xFF
    bits = 12: range = 0 - 0x0FFF
    bits = 16: range = 0 - 0xFFFF
*/
rgbwI RgbF2RgbI(rgbwF in, uint8_t bits) {
  uint16_t max;
  rgbwI out;

  switch (bits) { // determine max output value based on value of bits parameter
    case 16: 
      max = 0xFFFF;
    break;
    case 12:
      max = 0x0FFF;
    break;
    case 8:
    default:
      max = 0x00FF;
    break;
  }
  out.r = Dither(in.r * max); // scale up and apply rounding rules with optional temporal dithering
  out.g = Dither(in.g * max);
  out.b = Dither(in.b * max);
  out.w = Dither(in.w * max);
  return out;
}


/* Dither() takes a floating point value (see below) and converts it to an integer value using the 
    following rounding rules:
    1) if dithering is not enabled (DITHER_ENABLE == false), normal rounding rules are applied, otherwise:
    2) if the fractional part of the scaled floating point value is <=0.333 or >=0.667, normal rounding is applied, otherwise:
    3) the value is alternately rounded up or down based on the value of frameCount (odd or even)
  Note: The floating point values are typically scaled up from the normal 0-1 range, such as by Rgb2F2RgbI() above. 
*/
uint16_t Dither(float val) {
  float i, f; // temp values: integer and fractional parts

  i = (uint16_t) val;   // get integer part via truncation
  f = val - i;  // get fractional part
  if (DITHER_ENABLE) {  // if dithering is enabled in SysyConfig.h
    if (frameCount & 1) {   // if frameCount is odd
      if (f < 0.667) // and if fractional part is within "dither down range"
        return i; // round down (truncate) result
      else  
        return (i + 1); // frac is >= 0.667 so round up
    }
    else {  // frameCount is even
      if (f > 0.333)  // and if fractional part is within "dither up range"
        return (i + 1); // round up 
      else 
        return i; // frac is <= 0.333 so round down
    }
  }
  else {  // dithering is not enabled
    if (f < 0.5)  // apply normal rounding rules
      return i;
    else 
      return (i + 1);
  }
}
  

/* HueDistance() is used by certain effects to compute the signed "distance" between two hue (H) values, given that Hue is a 
    is a circular range that wraps around from the maximum value to 0. The distance returned is the minimum (absolute) value of the two
    possible directions.
*/
float HueDistance(float startH, float endH) {
  float nonWrapDist;

  nonWrapDist = endH - startH;  // compute distance without wraparound
  if (abs(nonWrapDist) <= 0.5) {  // non-wrapped distance is shortest
    return (nonWrapDist);
  }
  else if (endH >= startH) { // negative wrap is shortest
    return (nonWrapDist - 1);
  }
  else { // (endH < startH), so positive wrap is shortest
    return (nonWrapDist - 1);
  }
}


/* WrapHue() is used to wrap a computed floating point hue value back into the correct range. For example, if a hue
    value is computed as 0.1 - 0.3 = -0.2, WrapHue() will return the correct value of 1.0 - 0.2 = 0.8. 
*/
float WrapHue(float h) {
  if (h < 0) 
    h = 1 + h;
  else if (h > 1)
    h = h - 1;
  return h;
}


/* BlendHsv() returns an HSV color that is an interpolated blend of the parameters color1 and color2. The blended Hue and 
    Saturation values are computed based on the relative brightness (V) of color1 and color2, where the brightness of 
    color2 is further scaled by the parameter scaleV2. This allows the colors to be blended as if color2 were much brighter
    than the normal range of V (in HSV). Blending is performed based on the shortest distance between the hue values (wrapped
    or unwrapped), as determined by HueDistance(). The parameter scaleV2 has a minimum value of 1 (no scaling).
*/
hsvF BlendHsv(hsvF color1, hsvF color2, float scaleV2) {
  float blendRatio;
  float vRatio;
  float blendDist;
  hsvF blendColor;

  color2.v *= scaleV2;  // scale up the brightness of color2
  vRatio = color2.v / color1.v; // blends are based on ratio of scaled v2 to v1
  blendRatio = vRatio / (1 + vRatio);  // convert ratio to value between 0 and 1
  blendDist = HueDistance(color1.h, color2.h) * blendRatio; // use blendRatio to compute hue distance from color1 to blended color
  blendColor.h = WrapHue(color1.h + blendDist); // add blended distance and apply wrapping if necessary
  blendColor.v = min((color1.v + color2.v), 1.0);  // brightness "blend" is just a summation with range clipping
    // saturation blend is a brightness-weighted average. Protect against div-by-0 error
  blendColor.s = ((color1.v * color1.s) + (color2.v * color2.s)) / max((color1.v + color2.v), 0.1);  
  return blendColor;
}


/* InterpHsv() returns an HSV color interpolated between color1 and color2 based on the value of ctrl (between 0 and 1), using
    the shortest possible hue distance (with wrapping if necessary)
*/
hsvF InterpHsv(hsvF color1, hsvF color2, float ctrl) {
  hsvF iColor;

  iColor.h = WrapHue(color1.h + (HueDistance(color1.h, color2.h) * ctrl));
  iColor.s = color1.s + ((color2.s - color1.s) * ctrl);
  iColor.v = color1.v + ((color2.v - color1.v) * ctrl);
  return (iColor);
}
