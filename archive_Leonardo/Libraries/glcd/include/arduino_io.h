/*
 * arduino_io.h
 * this file maps arduino pins to avr ports and pins
 *
 * The header file: pins_arduino.h is used if this exits
 * otherwise the following controllers are defined in this file
 * Arduino (ATmega8,168,328), Mega, Sanguino (ATmega644P)
 * 
 * Thanks to Paul Stoffregen (http://www.pjrc.com/teensy) 
 * for his expertise in Arduino pin mapping macros
 *
 * Warning: when adding new board/procesor types ther are other headers
 * that have similar dependencies most notabily the ones in glcd/config
*/ 

#include "pins_arduino.h"

#if !(defined(digitalPinToPortReg) && defined(digitalPinToBit))
#if defined(__AVR_ATmega8__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)

	// Standard Arduino Pins
#define digitalPinToPortReg(P) \
    (((P) >= 0 && (P) <= 7) ? &PORTD : (((P) >= 8 && (P) <= 13) ? &PORTB : &PORTC))
#define digitalPinToBit(P) \
    (((P) >= 0 && (P) <= 7) ? (P) : (((P) >= 8 && (P) <= 13) ? (P) - 8 : (P) - 14))

#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	// Arduino Mega Pins
#define digitalPinToPortReg(P) \
    (((P) >= 22 && (P) <= 29) ? &PORTA : \
	((((P) >= 10 && (P) <= 13) || ((P) >= 50 && (P) <= 53)) ? &PORTB : \
    (((P) >= 30 && (P) <= 37) ? &PORTC : \
	((((P) >= 18 && (P) <= 21) || (P) == 38) ? &PORTD : \
	((((P) >= 0 && (P) <= 3) || (P) == 5) ? &PORTE : \
    (((P) >= 54 && (P) <= 61) ? &PORTF : \
	((((P) >= 39 && (P) <= 41) || (P) == 4) ? &PORTG : \
	((((P) >= 6 && (P) <= 9) || (P) == 16 || (P) == 17) ? &PORTH : \
	(((P) == 14 || (P) == 15) ? &PORTJ : \
    (((P) >= 62 && (P) <= 69) ? &PORTK : &PORTL))))))))))
#define digitalPinToBit(P) \
	(((P) >=  7 && (P) <=  9) ? (P) - 3 : \
	(((P) >= 10 && (P) <= 13) ? (P) - 6 : \
	(((P) >= 22 && (P) <= 29) ? (P) - 22 : \
	(((P) >= 30 && (P) <= 37) ? 37 - (P) : \
	(((P) >= 39 && (P) <= 41) ? 41 - (P) : \
	(((P) >= 42 && (P) <= 49) ? 49 - (P) : \
	(((P) >= 50 && (P) <= 53) ? 53 - (P) : \
	(((P) >= 54 && (P) <= 61) ? (P) - 54 : \
	(((P) >= 62 && (P) <= 69) ? (P) - 62 : \
	(((P) == 0 || (P) == 15 || (P) == 17 || (P) == 21) ? 0 : \
	(((P) == 1 || (P) == 14 || (P) == 16 || (P) == 20) ? 1 : \
	(((P) == 19) ? 2 : \
	(((P) == 5 || (P) == 6 || (P) == 18) ? 3 : \
	(((P) == 2) ? 4 : \
	(((P) == 3 || (P) == 4) ? 5 : 7)))))))))))))))

#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__)  
// Sanguino or other ATmega644 controller
#define digitalPinToPortReg(P) \
    (((P) >= 0 && (P) <= 7)   ? &PORTB : \
	(((P) >= 8 && (P) <= 15)  ? &PORTD : \
 	(((P) >= 16 && (P) <= 23) ? &PORTC : &PORTA)))
#define digitalPinToBit(P) \
     (((P) >= 0 && (P) <= 23) ? (P%8) : (7-(P%8)) )
//#error "ATmega644 has not been tested"
#elif defined(__AVR_ATmega32U4__)
// Leonardo
#define digitalPinToPortReg(P) \
	(\
	(P) == 0 ? &PORTD : \
	(P) == 1 ? &PORTD : \
	(P) == 2 ? &PORTD : \
	(P) == 3 ? &PORTD : \
	(P) == 4 ? &PORTD : \
	(P) == 5 ? &PORTC : \
	(P) == 6 ? &PORTD : \
	(P) == 7 ? &PORTE : \
	(P) == 8 ? &PORTB : \
	(P) == 9 ? &PORTB : \
	(P) == 10 ? &PORTB : \
	(P) == 11 ? &PORTB : \
	(P) == 12 ? &PORTD : \
	(P) == 13 ? &PORTC : \
	(P) == 14 ? &PORTB : \
	(P) == 15 ? &PORTB : \
	(P) == 16 ? &PORTB : \
	(P) == 17 ? &PORTB : \
	(P) == 18 ? &PORTF : \
	(P) == 19 ? &PORTF : \
	(P) == 20 ? &PORTF : \
	(P) == 21 ? &PORTF : \
	(P) == 22 ? &PORTF : \
	(P) == 23 ? &PORTF : \
	(P) == 24 ? &PORTD : \
	(P) == 25 ? &PORTD : \
	(P) == 26 ? &PORTB : \
	(P) == 27 ? &PORTB : \
	(P) == 28 ? &PORTB : \
	(P) == 29 ? &PORTD : \
	&PORTD) // dummy entry (should never happen)

#define digitalPinToBit(P) \
	(\
	(P) == 0 ? 2 : \
	(P) == 1 ? 3 : \
	(P) == 2 ? 1 : \
	(P) == 3 ? 0 : \
	(P) == 4 ? 4 : \
	(P) == 5 ? 6 : \
	(P) == 6 ? 7 : \
	(P) == 7 ? 6 : \
	(P) == 8 ? 4 : \
	(P) == 9 ? 5 : \
	(P) == 10 ? 6 : \
	(P) == 11 ? 7 : \
	(P) == 12 ? 6 : \
	(P) == 13 ? 7 : \
	(P) == 14 ? 3 : \
	(P) == 15 ? 1 : \
	(P) == 16 ? 2 : \
	(P) == 17 ? 0 : \
	(P) == 18 ? 7 : \
	(P) == 19 ? 6 : \
	(P) == 20 ? 5 : \
	(P) == 21 ? 4 : \
	(P) == 22 ? 1 : \
	(P) == 23 ? 0 : \
	(P) == 24 ? 4 : \
	(P) == 25 ? 7 : \
	(P) == 26 ? 4 : \
	(P) == 27 ? 5 : \
	(P) == 28 ? 6 : \
	(P) == 29 ? 6 : \
	0) // dummy entry (should never happen)

#else	
#error "Arduino pin mapping not defined for this board"
#endif

#endif
