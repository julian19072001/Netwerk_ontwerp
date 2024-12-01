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

#define DEVICE_ADDRESS  0x01                           //address of device (each address can only be used once in a network)


int main(void)
{
	init_clock();
    init_stream(F_CPU);
	radioInit(DEVICE_ADDRESS);

	PORTC.DIRSET = PIN0_bm;

	PMIC.CTRL |= PMIC_LOLVLEN_bm;			//turn on low level interrupts	

	sei();
	
	while(1)
	{	
		printPackages();
		_delay_ms(500);
		/*
		Package* package = get_radio_data(DEVICE_ADDRESS);

		if (package != NULL){
			printf("From: %02X, Payload: %s\n", package->id, package->payload);  // Process the payload
		} 

		if(CanRead_F0()){
			
			static uint8_t *string = {0};    
			static int index = 0;                               
            uint8_t temp_Char = uartF0_getc();

			if(temp_Char == '\r') {
				uartF0_putc('\r');
				uartF0_putc('\n');

				string[index++] = '\0';

				// Copy the input string to a temporary buffer to avoid modifying the original
				char temp_string[32];
				strcpy(temp_string, string);

				uint8_t first_str[30];
				uint8_t second_str[30];  // Array to store the second part (string)
			
				// Find the first space and separate the parts
				char* token = strtok(temp_string, " ");
				if (token != NULL) {
					strcpy(first_str, token); // First part
					// Get the rest of the string (after the first space)
					char* rest = string + (token - temp_string) + strlen(first_str);
					strcpy(second_str, rest);
				}
				
				send_radio_data(atoi(first_str), second_str, sizeof(second_str));	
				free(string);
				index = 0;
			} 
			else{
				uartF0_putc(temp_Char);
				string[index++] = temp_Char;
			}
        } */
	}
}

