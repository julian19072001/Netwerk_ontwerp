/*Julian Della Guardia*/

#define F_CPU 32000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
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
// While loop function for sending light level data
void lightLevel(uint8_t address, uint16_t sensorData);
// While loop function for ground water
void groundWater(uint8_t address, uint16_t sensorData);

// Convert ADC value to real value float for water level
float WaterLevelConverter(double raw_adc){
	static float Sensordata;
	
	if(raw_adc > 4050) Sensordata = 100;
	else Sensordata = (float) (raw_adc/(40.96));
	
	return Sensordata;
}

// Convert ADC value to real value float for light level
float lightConverter(double raw_adc){
	static float Sensordata;
	
	if(raw_adc > 4050) Sensordata = 1000;
	else Sensordata = (float) (raw_adc/(4.096));
	
	return Sensordata;
}

void init_adc(void){
  PORTA.DIRCLR     = PIN2_bm;                          // configure PA2 as input for ADCA
  ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN2_gc;            // PA2 to channel 0
  ADCA.CH0.CTRL    = ADC_CH_INPUTMODE_SINGLEENDED_gc;  // channel 0 single-ended
  ADCA.REFCTRL     = ADC_REFSEL_INTVCC_gc;             // internal VCC/1.6 reference
  ADCA.CTRLB       = ADC_RESOLUTION_12BIT_gc;          // 12 bits conversion, unsigned, no freerun
  ADCA.PRESCALER   = ADC_PRESCALER_DIV16_gc;           // 2MHz/16 = 125kHz
  ADCA.CTRLA       = ADC_ENABLE_bm;                    // enable adc
}

uint16_t readAdc(void){
  uint16_t res;

  ADCA.CH0.CTRL |= ADC_CH_START_bm;                    // start ADC conversion
  while ( !(ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm) ) ;    // wait until it's ready
  res = ADCA.CH0.RES;
  ADCA.CH0.INTFLAGS |= ADC_CH_CHIF_bm;                 // reset interrupt flag

  return res;                                          // return measured value
}

int main(void){
	init_clock();
    init_stream(F_CPU);
	init_adc();
	wdt_enable(WDT_PER_4KCLK_gc); 	// Enable a watchdog to prevent process from becoming stuck 

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
	else if(address >= LIGHT_START_ADDRESS && address <= LIGHT_END_ADDRESS){
		PORTA.DIRSET = PIN1_bm | PIN3_bm;
		PORTA.OUTCLR = PIN1_bm;
		PORTA.OUTSET = PIN3_bm;
	}
	else if(address >= GROUND_WATER_START_ADDRESS && address <= GROUND_WATER_END_ADDRESS){
		PORTA.DIRSET = PIN1_bm | PIN3_bm;
		PORTA.OUTCLR = PIN1_bm;
		PORTA.OUTSET = PIN3_bm;
	}

	uint8_t testNum = 0;

	while(1){	
		statusBlink();

		uint16_t sensorValue = readAdc();
		// If node is tempature and humidity node
		if(address >= TEMP_HUMID_START_ADDRESS && address <= TEMP_HUMID_END_ADDRESS)
			temparureAndHumidity(address);	
		else if(address >= LIGHT_START_ADDRESS && address <= LIGHT_END_ADDRESS)
			lightLevel(address, sensorValue); 		
		else if(address >= GROUND_WATER_START_ADDRESS && address <= GROUND_WATER_END_ADDRESS)
			groundWater(address, sensorValue); 
		else if(address >= TEST_START_ADDRESS && address <= TEST_END_ADDRESS){
			uint8_t sendData[MAX_DATA_LENGTH];	// Create an array for the data to be send

			sendData[0] = address;
			sendData[1] = DATA_TEST;
			sendData[2] = testNum;
			sendRadioData(BASE_ADDRESS, sendData, 3, false);
			printf("send test things\n");

			testNum++;
		}
		
		wdt_reset();	// Reset watchdog timer
		_delay_ms(TIME_BETWEEN_DATA); 
	}
}

// Function to convert float into uint8_t array
void floatToUint8Array(float value, uint8_t* array) {
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

	floatToUint8Array(getTemperature_C(), BME280Data);	// Read temperature from BME280 data and move into uint8_t array

	memcpy(&sendData[2], BME280Data, 4);
	sendData[0] = address;			// Set first byte as original sender address
	sendData[1] = DATA_TEMP;		// Set the data type in message as temperature
	sendRadioData(BASE_ADDRESS, sendData, 6, true);
	memset(BME280Data, 0, sizeof(BME280Data));

	_delay_ms(TIME_BETWEEN_DATA);

	floatToUint8Array(getHumidity(), BME280Data);	// Read humidity from BME280 data and move into uint8_t array

	memcpy(&sendData[2], BME280Data, 4);
	sendData[0] = address;			// Set first byte as original sender address
	sendData[1] = DATA_HUMID;		// Set the data type in message as humidity
	sendRadioData(BASE_ADDRESS, sendData, 6, true);
	memset(BME280Data, 0, sizeof(BME280Data));

	printf("send temp things\n"); 
}

// While loop function for sending light level data
void lightLevel(uint8_t address, uint16_t sensorData){
	uint8_t sendData[MAX_DATA_LENGTH];	// Create an array for the data to be send

	uint8_t lightLevel[4];

	floatToUint8Array(lightConverter(sensorData), lightLevel);

	memcpy(&sendData[2], lightLevel, 4);
	sendData[0] = address;			// Set first byte as original sender address
	sendData[1] = DATA_LIGHT;		// Set the data type in message as temperature
	sendRadioData(BASE_ADDRESS, sendData, 6, true);
	memset(lightLevel, 0, sizeof(lightLevel));

	printf("send light things\n"); 
}

// While loop function for ground water
void groundWater(uint8_t address, uint16_t sensorData){
	uint8_t sendData[MAX_DATA_LENGTH];	// Create an array for the data to be send

	uint8_t waterLevel[4];

	floatToUint8Array(WaterLevelConverter(sensorData), waterLevel);

	memcpy(&sendData[2], waterLevel, 4);
	sendData[0] = address;			// Set first byte as original sender address
	sendData[1] = DATA_GROUND_HUMID;		// Set the data type in message as temperature
	sendRadioData(BASE_ADDRESS, sendData, 6, true);
	memset(waterLevel, 0, sizeof(waterLevel));

	printf("send ground water things\n"); 
}