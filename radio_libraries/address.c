/*Julian Della Guardia*/

#define F_CPU 32000000UL

#include "address.h"
#include <avr/io.h>
#include <util/delay.h>

uint8_t getSensorAddress(void){
    // Configure all pins of Port B as input
    PORTB.DIR = 0x00; 

    // Enable pull-up resistors for Port B
    PORTB.PIN0CTRL = PORT_OPC_PULLUP_gc;
    PORTB.PIN1CTRL = PORT_OPC_PULLUP_gc;
    PORTB.PIN2CTRL = PORT_OPC_PULLUP_gc;
    PORTB.PIN3CTRL = PORT_OPC_PULLUP_gc;
    PORTB.PIN4CTRL = PORT_OPC_PULLUP_gc;
    PORTB.PIN5CTRL = PORT_OPC_PULLUP_gc;
    PORTB.PIN6CTRL = PORT_OPC_PULLUP_gc;
    PORTB.PIN7CTRL = PORT_OPC_PULLUP_gc;

    _delay_ms(10);

    // Read the value of Port B input pins
    return ~PORTB.IN;
}