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
#include "radio_libraries/mesh_radio.h"
#include "radio_libraries/address.h"
#include "sensor_libraries/BME280.h"
#include "sensor_libraries/i2c.h"

// Function to blink LED on request
void statusBlink();
// While loop function for the temperature and humidity node
void temparureAndHumidity(uint8_t address);

int main(void)
{
	init_clock();
    init_stream(F_CPU);

	// Get network address from hardware switches
	uint8_t address = getSensorAddress();
	radioInit(address);

	printf("found address: %08u\n", address);

	// Setup for identifying blink
	PORTC.DIRSET = PIN0_bm;		

	PMIC.CTRL |= PMIC_LOLVLEN_bm;			// Turn on low level interrupts	

	sei();

	// Initialze correct sensor for selected address
	if(address >= TEMP_HUMID_START_ADDRESS && address <= TEMP_HUMID_END_ADDRESS){
		init_BME280(F_CPU);       // Initialize BME280 sensor
		
		// Setup power for the temperature sensor
		PORTE.DIRSET = PIN2_bm | PIN3_bm;
		PORTE.OUTCLR = PIN2_bm;
		PORTE.OUTSET = PIN3_bm;
	}
	else if(address >= LIGHT_START_ADDRESS && address <= LIGHT_END_ADDRESS)
		printf("init light things\n"); // Initialize light sensor
	else if(address >= GROUND_WATER_START_ADDRESS && address <= GROUND_WATER_END_ADDRESS)
		printf("init ground water things\n"); // Initialize light sensor
	
	while(1)
	{	
		statusBlink();

		// If node is tempature and humidity node
		if(address >= TEMP_HUMID_START_ADDRESS && address <= TEMP_HUMID_END_ADDRESS){
			temparureAndHumidity(address);
			printf("send temp things\n"); 	
		}
		else if(address >= LIGHT_START_ADDRESS && address <= LIGHT_END_ADDRESS)
			printf("send light things\n"); 		
		else if(address >= GROUND_WATER_START_ADDRESS && address <= GROUND_WATER_END_ADDRESS)
			printf("send ground water things\n");

		_delay_ms(TIME_BETWEEN_DATA); 
	}
}

// Function to convert float into uint8_t array
void float_to_uint8_array(float value, uint8_t* array) {
    // Use a pointer to access the float's bytes
    uint8_t* float_ptr = (uint8_t*)&value;
    for (int i = 0; i < sizeof(float); i++) {
        array[i] = float_ptr[i];
    }
}

// Function to blink LED on request
void statusBlink(){
	static uint8_t blinking = 254;

	uint8_t data[MAX_DATA_LENGTH] = {0};
	uint8_t numByte = readRadioMessage(data);	// Get last message from NRF

	// Check if the received data is for blinking, if so then activated the blinking
	if(numByte){	
		if(data[0] == BASE_ADDRESS && data[1] == DATA_IDENTFY) blinking = 0;
	} 

	// If the blinking variable hasnt passed the time then toggle the LED
	if(blinking < BLINK_LENGHT * 5){
		PORTC.OUTTGL = PIN0_bm;
		blinking ++;
	}
	// Else turn LED off
	else PORTC.OUTCLR = PIN0_bm;
}

// While loop function for the temperature and humidity node
void temparureAndHumidity(uint8_t address){
	uint8_t sendData[MAX_DATA_LENGTH];	// Create an array for the data to be send

	uint8_t BME280Data[4];
	read_BME280(BME280_ADDRESS_1);  // Read data from BME280 

	float_to_uint8_array(getTemperature_C(), BME280Data);	// Read temperature from BME280 data and move into uint8_t array

	memcpy(&sendData[2], BME280Data, 4);
	sendData[0] = address;			// Set first byte as original sender address
	sendData[1] = DATA_TEMP;		// Set the data type in message as temperature
	sendRadioData(BASE_ADDRESS, sendData, 6, true);
	memset(BME280Data, 0, sizeof(BME280Data));

	_delay_ms(TIME_BETWEEN_DATA);

	float_to_uint8_array(getHumidity(), BME280Data);	// Read humidity from BME280 data and move into uint8_t array

	memcpy(&sendData[2], BME280Data, 4);
	sendData[0] = address;			// Set first byte as original sender address
	sendData[1] = DATA_HUMID;		// Set the data type in message as humidity
	sendRadioData(BASE_ADDRESS, sendData, 6, true);
	memset(BME280Data, 0, sizeof(BME280Data));
}
