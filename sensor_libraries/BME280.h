/**************************************************************************/
/*
@file     	bme280.h
@author   	Julian Della Guardia
@date		13-4-2022
@version	1.2

Rewitten version of the arduino library to use on the HVA xMega.
@orignal	David Zovko
@link		https://github.com/e-radionicacom/BME280-Arduino-Library
*/
/**************************************************************************/
#ifndef BME280_H_
#define BME280_H_

#include "i2c.h"
#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define BME280_ADDRESS_1      0x77         	//bme I2C address 1
#define BME280_ADDRESS_2      0x76			//bme I2C address 2


//address numbers for calibration registers
#define    BME280_DIG_T1_REG   0x88
#define    BME280_DIG_T2_REG   0x8A
#define    BME280_DIG_T3_REG   0x8C
#define    BME280_DIG_P1_REG   0x8E
#define    BME280_DIG_P2_REG   0x90
#define    BME280_DIG_P3_REG   0x92
#define    BME280_DIG_P4_REG   0x94
#define    BME280_DIG_P5_REG   0x96
#define    BME280_DIG_P6_REG   0x98
#define    BME280_DIG_P7_REG   0x9A
#define    BME280_DIG_P8_REG   0x9C
#define    BME280_DIG_P9_REG   0x9E


#define    BME280_DIG_H1_REG   0xA1
#define    BME280_DIG_H2_REG   0xE1
#define    BME280_DIG_H3_REG   0xE3
#define    BME280_DIG_H4_REG   0xE4
#define    BME280_DIG_H5_REG   0xE5
#define    BME280_DIG_H6_REG   0xE7

//remaining register addresses 
#define    BME280_REGISTER_CHIPID       0xD0
#define    BME280_REGISTER_VERSION      0xD1
#define    BME280_REGISTER_SOFTRESET    0xE0
#define    BME280_REGISTER_CAL26        0xE1
#define    BME280_REGISTER_CONTROLHUMID     0xF2
#define    BME280_REGISTER_CONTROL          0xF4
#define    BME280_REGISTER_CONFIG           0xF5
#define    BME280_REGISTER_PRESSUREDATA     0xF7
#define    BME280_REGISTER_TEMPDATA         0xFA
#define    BME280_REGISTER_HUMIDDATA        0xFD

#define TWI_BAUD(F_SYS, F_TWI) ((F_SYS/(2*F_TWI))-5)
#define BAUD_400K 400000UL

//Struct for storing calibration data
struct BME280_Calibration_Data
{
	uint16_t dig_T1;
	int16_t  dig_T2;
	int16_t  dig_T3;
	
	uint16_t dig_P1;
	int16_t  dig_P2;
	int16_t  dig_P3;
	int16_t  dig_P4;
	int16_t  dig_P5;
	int16_t  dig_P6;
	int16_t  dig_P7;
	int16_t  dig_P8;
	int16_t  dig_P9;
	
	uint8_t  dig_H1;
	int16_t  dig_H2;
	uint8_t  dig_H3;
	int16_t  dig_H4;
	int16_t  dig_H5;
	int8_t   dig_H6;
};

void init_BME280(uint32_t f_sys);

void read_BME280(int16_t input_address);    //read the sensor for data

void setTempCal(float);						//we can set a calibration ofsset for the temperature.
float getTemperature_C(void);               //temprature in celsius
float getTemperature_F(void);               //temprature in fahrenheit
float getHumidity(void);                 	//humidity in %
float getPressure_HP(void);                 //pressure in hectapascals
float getPressure_MB(void);                 //pressure in millibars
float getAltitude(int offset);				//get altitude in meters given offset

#endif