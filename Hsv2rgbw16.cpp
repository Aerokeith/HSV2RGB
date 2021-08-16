#include <Arduino.h>
#include <FastLED.h>
#include "Hsv2rgbw16.h"


HSV16 HSV8toHSV16(CHSV inVal) {
    HSV16 retVal;

    retVal.h = (uint16_t) (inVal.h) << 8;
    if (inVal.s == 0xFF)
        retVal.s = 0xFFFF;
    else
        retVal.s = (uint16_t) (inVal.s) << 8;
    if (inVal.v == 0xFF) 
        retVal.v = 0xFFFF;
    else
        retVal.v = (uint16_t) (inVal.v) << 8;
    return retVal;
}

CHSV HSV16toHSV8(HSV16 inVal) {
    CHSV retVal;

    retVal.h = (uint8_t) (inVal.h >> 8);
    retVal.s = (uint8_t) (inVal.s >> 8);
    retVal.v = (uint8_t) (inVal.v >> 8);
    return retVal;
}


void HSV16toRGBW16(HSV16 cHSV16, RGBW16 *cRGBW16) {
  // Yellow has a higher inherent brightness than
  // any other color; 'pure' yellow is perceived to
  // be 93% as bright as white.  In order to make
  // yellow appear the correct relative brightness,
  // it has to be rendered brighter than all other
  // colors.
  // Level Y1 is a moderate boost, the default.
  // Level Y2 is a strong boost.
  const uint8_t Y1 = 1;
  const uint8_t Y2 = 0;
  
  // G2: Whether to divide all greens by two.
  // Depends GREATLY on your particular LEDs
  const uint8_t G2 = 0;
  
  // Gscale: what to scale green down by.
  // Depends GREATLY on your particular LEDs
  const uint16_t Gscale = 0;
  
  uint16_t hue = cHSV16.h;
  uint16_t sat = cHSV16.s;
  uint16_t val = cHSV16.v;

  uint16_t offset = hue & (0x1FFF); // 0..31
  //Serial.print("Offset = ");
  //Serial.print(offset);

  
  // offset8 = offset * 8
  uint16_t offset8 = (offset << 3);

  //Serial.print("  Offset8 = ");
  //Serial.print(offset8);


  uint16_t third = scale16( offset8, (65536 / 3)); // max = 85
    
  //Serial.print("  Third = ");
  //Serial.print(third);
  
  uint16_t r, g, b, w;
    
  w = 0;
  if( ! (hue & 0x8000) ) {
      // 0XX
      if( ! (hue & 0x4000) ) {
          // 00X
          //section 0-1
          if( ! (hue & 0x2000) ) {
              // 000
              //case 0: // R -> O
              r = (65535) - third;
              g = third;
              b = 0;
          } else {
              // 001
              //case 1: // O -> Y
              if( Y1 ) {
                  r = (171 << 8); 
                  g = (85 << 8) + third ;
                  b = 0;
              }
              if( Y2 ) {
                  r = (170 << 8) + third;
                  uint16_t twothirds = (third << 1);
                  //old: uint8_t twothirds = scale8( offset8, ((256 * 2) / 3)); // max=170
                  g = (85 << 8) + twothirds;
                  b = 0;
              }
          }
      } else {
          //01X
          // section 2-3
          if( !  (hue & 0x2000) ) {
              // 010
              //case 2: // Y -> G
              if( Y1 ) {
                  uint16_t twothirds = (third << 1);
                  // old: uint8_t twothirds = scale8( offset8, ((256 * 2) / 3)); // max=170
                  r = (171 << 8) - twothirds;
                  g = (170 << 8) + third;
                  b = 0;
              }
              if( Y2 ) {
                  r = 65535 - offset8;
                  g = 65535;
                  b = 0;
              }
          } else {
              // 011
              // case 3: // G -> A
              r = 0;
              g = 65535 - third;
              b = third;
          }
      }
  } else {
      // section 4-7
      // 1XX
      if( ! (hue & 0x4000) ) {
          // 10X
          if( ! ( hue & 0x2000) ) {
              // 100
              //case 4: // A -> B
              r = 0;
              uint16_t twothirds = (third << 1);
              // old: uint8_t twothirds = scale8( offset8, ((256 * 2) / 3)); // max=170
              g = (171 << 8) - twothirds; //K170?
              b = (85 << 8)  + twothirds;
              
          } else {
              // 101
              //case 5: // B -> P
              r = third;
              g = 0;
              b = 65535 - third;
              
          }
      } else {
          if( !  (hue & 0x2000)  ) {
              // 110
              //case 6: // P -- K
              r = (85 << 8) + third;
              g = 0;
              b = (171 << 8) - third;
              
          } else {
              // 111
              //case 7: // K -> R
              r = (170 << 8) + third;
              g = 0;
              b = (85 << 8) - third;
              
          }
      }
  }
  // This is one of the good places to scale the green down,
  // although the client can scale green down as well.
  if( G2 ) g = g >> 1;
  // old: if( Gscale ) g = scale8_video_LEAVING_R1_DIRTY( g, Gscale);
  if ( Gscale ) g = scale16(g, Gscale); // Note this is not a "video" scale
  
  // Scale down colors if we're desaturated at all
  // and add the brightness_floor to r, g, and b.
  if( sat != 65535 ) {
    if( sat == 0) {
      r = 0; b = 0; g = 0; w = 65535;
    } 
    else {
      w = 0;
      if( r ) r = scale16( r, sat);
      if( g ) g = scale16( g, sat);
      if( b ) b = scale16( b, sat);
      //Serial.print("  SScaled R = ");
      //Serial.print(r);

            
      uint16_t desat = 65535 - sat;
      //Serial.print("  Desat = ");
      //Serial.print(desat);

      desat = scale16( desat, desat);
      
      uint16_t brightness_floor = desat;
      //Serial.print("  Floor = ");
      //Serial.print(brightness_floor);

      r += brightness_floor;
      g += brightness_floor;
      b += brightness_floor;
      //Serial.print("  Floored R = ");
      //Serial.print(r);

    }
  }
    
  // Now scale everything down if we're at value < 255.
  if( val != 65535 ) {
    val = scale16( val, val);
    //Serial.print("  Scaled Val = ");
    //Serial.print(val);

    if( val == 0 ) {
      r=0; g=0; b=0; w = 0;
    } 
    else {
      // nscale8x3_video( r, g, b, val);
      if( r ) r = scale16( r, val);
      if( g ) g = scale16( g, val);
      if( b ) b = scale16( b, val);
      if( w ) w = scale16( w, val);
      //Serial.print("  VScaled R = ");
      //Serial.println(r);

    }
  }
    
  (*cRGBW16).r = r;
  (*cRGBW16).g = g;
  (*cRGBW16).b = b;
  (*cRGBW16).w = w;
}
