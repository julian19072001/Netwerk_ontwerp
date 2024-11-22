/*Julian Della Guardia*/

#define F_CPU 32000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "clock.h"
#include "serialF0.h"
#include "mesh_radio.h"

#define DEVICE_ADDRESS  0x03                           //address of device (each address can only be used once in a network)


int main(void)
{
	init_clock();
    init_stream(F_CPU);
	radio_init(DEVICE_ADDRESS);

	PMIC.CTRL |= PMIC_LOLVLEN_bm;			//turn on low level interrupts	

	sei();
	
	while(1)
	{	

        print_packages();


		_delay_ms(1000);
	}
}

