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

	uint8_t received_data[32] = {0};
	
	while(1)
	{
        if(CanRead_F0()){                                   
            uint8_t temp_Char = uartF0_getc();

			if(temp_Char == '\r') {
				uartF0_putc('\r');
				uartF0_putc('\n');
			} 
            uartF0_putc(temp_Char);

			send_radio_data(1, &temp_Char, 1);
        }

		if(get_radio_data(&received_data)){
			uint8_t temp_Char2 = received_data[4];

			if(temp_Char2 == '\r') {
				uartF0_putc('\r');
				uartF0_putc('\n');
			} 
            uartF0_putc(temp_Char2);
		}
	}
}