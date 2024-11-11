#define F_CPU 32000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <math.h>

#include "clock.h"
#include "serialF0.h"
#include "mesh_radio.h"

int main(void)
{
	PORTF.DIRSET = PIN1_bm; //Rood
	init_clock();
    init_stream(F_CPU);
	radio_init();

	PMIC.CTRL |= PMIC_LOLVLEN_bm;			//turn on low level interrupts	

	sei();
	
	while(1)
	{
        if(CanRead_F0()){                                   
            uint8_t temp_Char = uartF0_getc(); 
            uartF0_putc(temp_Char);

			send_radio_data(1, &temp_Char, 1);
        }
	}
}