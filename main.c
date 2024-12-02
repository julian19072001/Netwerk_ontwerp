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

#define DEVICE_ADDRESS  0x42                           //address of device (each address can only be used once in a network)


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
		//printNeighbors();
		//_delay_ms(500);
		uint8_t data[MAX_DATA_LENGTH] = {0};
		
		uint8_t numByte = readRadioMessage(data);

		if (numByte){
			data[numByte] = '\0';
			printf("From: %02X, Payload: %s, numByte: %d\n", data[0], data + 1, numByte);  // Process the payload
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

				uint8_t first_str[28];
				uint8_t second_str[28];  // Array to store the second part (string)
			
				// Find the first space and separate the parts
				char* token = strtok(temp_string, " ");
				if (token != NULL) {
					strcpy(first_str, token); // First part
					// Get the rest of the string (after the first space)
					char* rest = string + (token - temp_string) + strlen(first_str);
					strcpy(second_str, rest);
					second_str[0] = DEVICE_ADDRESS;
				}
				
				sendRadioData(atoi(first_str), second_str, strlen(second_str));	
				free(string);
				index = 0;
			} 
			else{
				uartF0_putc(temp_Char);
				string[index++] = temp_Char;
			}
        } 
	}
}

