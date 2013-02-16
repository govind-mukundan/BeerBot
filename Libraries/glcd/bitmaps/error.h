/* error bitmap file for GLCD library */
/* Bitmap created from error.jpg      */
/* Date: 15 Feb 2013      */
/* Image Pixels = 4032    */
/* Image Bytes  = 504     */

#include <inttypes.h>
#include <avr/pgmspace.h>

#ifndef error_H
#define error_H

static uint8_t error[] PROGMEM = {
  63, // width
  64, // height

  /* page 0 (lines 0-7) */
  0x0,0x0,0x0,0x0,0x80,0xc0,0xe0,0xf0,0xf8,0xf8,0xfc,0xfe,0xfe,0xfc,0xf8,0xf0,
  0xe0,0xc0,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x80,0xc0,0xe0,0xf0,
  0xf8,0xfc,0xfe,0xfe,0xfc,0xf8,0xf0,0xe0,0xc0,0x80,0x0,0x0,0x0,0x0,0x0,
  /* page 1 (lines 8-15) */
  0x8,0x1c,0x3e,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xfe,0xfc,0xf8,0xf0,0xe0,0xc0,0x80,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7e,0x3c,0x18,0x0,
  /* page 2 (lines 16-23) */
  0x0,0x0,0x0,0x0,0x0,0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0xfc,0xf8,0xf0,
  0xf8,0xfc,0xfe,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0x7f,0x3f,0x1f,0xf,0x7,0x3,0x1,0x0,0x0,0x0,0x0,0x0,
  /* page 3 (lines 24-31) */
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x3,0x7,
  0xf,0x1f,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,0x3f,0x1f,0xf,
  0x7,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  /* page 4 (lines 32-39) */
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x80,0xc0,0xe0,
  0xf0,0xf8,0xfc,0xfe,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0xfc,0xf8,0xf0,
  0xe0,0xc0,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  /* page 5 (lines 40-47) */
  0x0,0x0,0x0,0x0,0x0,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,0x3f,0x1f,0xf,
  0x1f,0x3f,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xfe,0xfc,0xf8,0xe0,0xc0,0x80,0x0,0x0,0x0,0x0,0x0,
  /* page 6 (lines 48-55) */
  0x10,0x38,0x7c,0xfe,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0x7f,0x3f,0x1f,0xf,0x7,0x3,0x1,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7e,0x7c,0x38,0x0,
  /* page 7 (lines 56-63) */
  0x0,0x0,0x0,0x0,0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff,0x7f,0x3f,0x1f,0xf,
  0x7,0x3,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x3,0x7,0xf,
  0x1f,0x3f,0x7f,0x7f,0x3f,0x1f,0xf,0xf,0x7,0x1,0x0,0x0,0x0,0x0,0x0,
};
#endif