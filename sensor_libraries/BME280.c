/**************************************************************************/
/*
@file     	bme280.c
@author   	Julian Della Guardia
@date		18-3-2022
@version	1.2

Rewitten version of the arduino library to use on the HVA xMega.
@orignal	David Zovko
@link		https://github.com/e-radionicacom/BME280-Arduino-Library
*/
/**************************************************************************/

#include "bme280.h"

struct BME280_Calibration_Data cal_data;	//holds all of the sensor calibration data

void readTemperature(void);
void readPressure(void);
void readHumidity(void);
void readSensorCoefficients(void);

float    tempcal;							//stores the temp offset calibration
float    temperature;                       //stores temperature value
float    humidity;                          //stores humidity value
float    pressure;                          //stores pressure value


void      write8(int8_t reg, int8_t value);	//write 8 bits of data
uint8_t   read8(int8_t reg);				//read 8 bits of data
uint16_t  read16(int8_t reg);				//read 16 bits of data
uint32_t  read24(int8_t reg);				//read 24 bits of data
int16_t   readS16(int8_t reg);				//read 16 signed bits of data
uint16_t  read16_LE(int8_t reg); 			//little endian, read 16 bits of data
int16_t   readS16_LE(int8_t reg); 			//little endian, read 16 signed bits of data

uint8_t   _i2caddr;							//stores I2C address for sensor
int32_t   _sensorID;						
int32_t   t_fine;		
TWI_t* 	  twi;	

void init_BME280(uint32_t f_sys)
{
	twi = &TWIE;											//store the selecte twi channel
	_i2caddr = BME280_ADDRESS_1;							//store the I2C address in _i2caddr

	tempcal = 0;											//set tempcal to 0
	temperature = 0;										//set tempature to 0
	humidity = 0;											//set humidity to 0
	pressure = 0;											//set pressure to 0

	PORTE.DIRSET	= PIN1_bm|PIN0_bm;                      //set pin E0 and E1 as outputs
	PORTE.PIN0CTRL	= PORT_OPC_WIREDANDPULL_gc;             //enable pullup for E0
	PORTE.PIN1CTRL	= PORT_OPC_WIREDANDPULL_gc;             //enable pullup for E1

	i2c_init(twi,TWI_BAUD(f_sys,BAUD_400K));              	//start i2c
}

void setTempCal(float tcal)
{
	tempcal = tcal;											
}

void read_BME280(int16_t input_address)
{
	_i2caddr = input_address;								//set choosen I2C address
	
	readSensorCoefficients();								//read the sensor specific Coefficeints for calculating accurate values
	
	write8(BME280_REGISTER_CONTROLHUMID, 0x01);				//set the humidity controls
	write8(BME280_REGISTER_CONTROL, 0x3F);					//set oversampling and mode
	
	readTemperature();										//read the sensors measured temprature
	readHumidity();											//read the sensors measured humidity
	readPressure();											//read the sensors measured pressure
}

float getTemperature_C(void)
{
	return (temperature + tempcal);							//return the temprature in celsius
}

float getTemperature_F(void)
{	
	return (temperature + tempcal) * 1.8 + 32;				//return the temprature in fahrenheit
}

float getHumidity(void)
{
	return humidity;										//return the humidity
}

float getPressure_MB(void)
{
	return pressure / 100.0F;								//return the pressure in millibar
}

float getPressure_HP(void)
{
	return pressure;										//return the pressure in hectopascal
}

float getAltitude(int offset)
{	
	float altitude = (44330.77 * (1 - pow((getPressure_MB() / 1013.26), 0.1902632)) + offset);
	return altitude;										//return the altitude in meters
}


//calculate the temprature according to datasheet
void readTemperature(void)
{
	int32_t var1, var2;
	int32_t adc_T = read24(BME280_REGISTER_TEMPDATA);
	
	adc_T >>= 4;
	var1  = ((((adc_T>>3) - ((int32_t)cal_data.dig_T1 <<1))) * ((int32_t)cal_data.dig_T2)) >> 11;
	var2  = (((((adc_T>>4) - ((int32_t)cal_data.dig_T1)) * ((adc_T>>4) - ((int32_t)cal_data.dig_T1))) >> 12) * ((int32_t)cal_data.dig_T3)) >> 14;
	
	t_fine = var1 + var2;
	temperature  = (t_fine * 5 + 128) >> 8;
	temperature = temperature / 100;
}


//calculate the pressure according to datasheet
void readPressure(void)
{
	int64_t var1, var2, p;
	int32_t adc_P = read24(BME280_REGISTER_PRESSUREDATA);
	
	adc_P >>= 4;
	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)cal_data.dig_P6;
	var2 = var2 + ((var1*(int64_t)cal_data.dig_P5)<<17);
	var2 = var2 + (((int64_t)cal_data.dig_P4)<<35);
	var1 = ((var1 * var1 * (int64_t)cal_data.dig_P3)>>8) + ((var1 * (int64_t)cal_data.dig_P2)<<12);
	var1 = (((((int64_t)1)<<47)+var1))*((int64_t)cal_data.dig_P1)>>33;
	
	if (var1 == 0)
	{
		pressure = 0.0;
	}
	
	p = 1048576 - adc_P;
	p = (((p<<31) - var2)*3125) / var1;
	
	var1 = (((int64_t)cal_data.dig_P9) * (p>>13) * (p>>13)) >> 25;
	var2 = (((int64_t)cal_data.dig_P8) * p) >> 19;

	p = ((p + var1 + var2) >> 8) + (((int64_t)cal_data.dig_P7)<<4);

	pressure = (float)p/256;
}


//calculate the humidity according to datasheet
void readHumidity(void)
{
	int32_t adc_H = read16(BME280_REGISTER_HUMIDDATA);
	int32_t v_x1_u32r;

	v_x1_u32r = (t_fine - ((int32_t)76800));
	v_x1_u32r = (((((adc_H << 14) - (((int32_t)cal_data.dig_H4) << 20) - (((int32_t)cal_data.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)cal_data.dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)cal_data.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)cal_data.dig_H2) + 8192) >> 14));
	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)cal_data.dig_H1)) >> 4));
	v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
	v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
	
	float h = (v_x1_u32r>>12);
	
	humidity = h / 1024.0;
}


void readSensorCoefficients(void)
{
	cal_data.dig_T1 = read16_LE(BME280_DIG_T1_REG);
	cal_data.dig_T2 = readS16_LE(BME280_DIG_T2_REG);
	cal_data.dig_T3 = readS16_LE(BME280_DIG_T3_REG);
	cal_data.dig_P1 = read16_LE(BME280_DIG_P1_REG);
	cal_data.dig_P2 = readS16_LE(BME280_DIG_P2_REG);
	cal_data.dig_P3 = readS16_LE(BME280_DIG_P3_REG);
	cal_data.dig_P4 = readS16_LE(BME280_DIG_P4_REG);
	cal_data.dig_P5 = readS16_LE(BME280_DIG_P5_REG);
	cal_data.dig_P6 = readS16_LE(BME280_DIG_P6_REG);
	cal_data.dig_P7 = readS16_LE(BME280_DIG_P7_REG);
	cal_data.dig_P8 = readS16_LE(BME280_DIG_P8_REG);
	cal_data.dig_P9 = readS16_LE(BME280_DIG_P9_REG);
	cal_data.dig_H1 = read8(BME280_DIG_H1_REG);
	cal_data.dig_H2 = readS16_LE(BME280_DIG_H2_REG);
	cal_data.dig_H3 = read8(BME280_DIG_H3_REG);
	cal_data.dig_H4 = (read8(BME280_DIG_H4_REG) << 4) | (read8(BME280_DIG_H4_REG+1) & 0xF);
	cal_data.dig_H5 = (read8(BME280_DIG_H5_REG+1) << 4) | (read8(BME280_DIG_H5_REG) >> 4);
	cal_data.dig_H6 = (int8_t)read8(BME280_DIG_H6_REG);
}


void write8(int8_t reg, int8_t value)
{
	i2c_start(twi, _i2caddr, I2C_WRITE);	//start the I2C communication in write mode
	i2c_write(twi, reg);					//send the register value
	i2c_write(twi, value);					//send the value you want the register to be
	i2c_stop(twi);							//stop I2C communication
}



uint8_t read8(int8_t reg)
{
	uint8_t value;

	i2c_start(twi, _i2caddr, I2C_WRITE);	//start the I2C communication in write mode
	i2c_write(twi, reg);					//send the register value
	i2c_restart(twi, _i2caddr, I2C_READ);	//restart I2C communication in read mode
	
	value = i2c_read(twi, I2C_NACK);
	
	i2c_stop(twi);							//stop I2C communication
	
	return value;							//return the requested value
}



int16_t readS16(int8_t reg)
{
	return (int16_t)read16(reg);			//return the requested value
}

int16_t readS16_LE(int8_t reg)
{
	return (int16_t)read16_LE(reg);			//return the requested value
}

uint16_t read16(int8_t reg)
{
	uint16_t value;
	
	i2c_start(twi, _i2caddr, I2C_WRITE);	//start the I2C communication in write mode
	i2c_write(twi, reg);					//send the register value
	i2c_restart(twi, _i2caddr, I2C_READ);	//restart I2C communication in read mode

	value = (i2c_read(twi, I2C_ACK) << 8) | i2c_read(twi, I2C_NACK);	//save the 16 bit value

	i2c_stop(twi);							//stop I2C communication

	return value;							//return the requested value
}

uint16_t read16_LE(int8_t reg)
{
	uint16_t temp = read16(reg);
	
	return (temp >> 8) | (temp << 8);		//return the requested value
}



uint32_t read24(int8_t reg)
{
	uint32_t value;

	i2c_start(twi, _i2caddr, I2C_WRITE);	//start the I2C communication in write mode
	i2c_write(twi, reg);					//send the register value
	i2c_restart(twi, _i2caddr, I2C_READ);	//restart I2C communication in read mode

	value = i2c_read(twi, I2C_ACK);			//read 8 bits of data
	value <<= 8;							//shift bits 8 positions to the left

	value |= i2c_read(twi, I2C_ACK);		//read 8 bits of data
	value <<= 8;							//shift bits 8 positions to the left

	value |= i2c_read(twi, I2C_NACK);		//read 8 bits of data
	
	i2c_stop(twi);							//stop I2C communication
	return value;							//return the requested value
}