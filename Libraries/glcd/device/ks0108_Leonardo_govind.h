/*
 * ks0108_Leonardo.h - User specific configuration for Arduino GLCD library
 *
 * Use this file to set io pins
 * This version is for a standard ks0108 display
 * connected using the Leonardo wiring
 *
*/

#ifndef GLCD_PIN_CONFIG_H
#define GLCD_PIN_CONFIG_H

/*
 * define name for pin configuration
 */
#define glcd_PinConfigName "ks0108-Leonardo"

/*********************************************************/
/*  Configuration for assigning LCD bits to Arduino Pins */
/*********************************************************/
/* Data pin definitions
 */
#define glcdData0Pin        13
#define glcdData1Pin        12
#define glcdData2Pin        11
#define glcdData3Pin        10
#define glcdData4Pin        9
#define glcdData5Pin        8
#define glcdData6Pin        7
#define glcdData7Pin        6

/* Arduino pins used for Commands
 * default assignment uses the first five analog pins
 */

#define glcdCSEL1        4
#define glcdCSEL2        5

#if NBR_CHIP_SELECT_PINS > 2
#define glcdCSEL3         3   // third chip select if needed
#endif

#if NBR_CHIP_SELECT_PINS > 3
#define glcdCSEL4         2   // fourth chip select if needed
#endif

#define glcdRW           3
#define glcdDI           2
#define glcdEN           A0
// Reset Bit  - uncomment the next line if reset is connected to an output pin
//#define glcdRES          A5    // Reset Bit

#endif //GLCD_PIN_CONFIG_H