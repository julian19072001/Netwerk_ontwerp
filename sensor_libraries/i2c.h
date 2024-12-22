/**************************************************************************/
/*!
@file     	i2c.h
@author   	Julian Della Guardia

Rewitten version of the i2c library to use on the HVA xMega.
@orignal	Wim Dolman
*/
/**************************************************************************/
#include <avr/io.h>

#define I2C_ACK     0
#define I2C_NACK    1
#define I2C_READ    1
#define I2C_WRITE   0

#define I2C_STATUS_OK      0
#define I2C_STATUS_BUSY    1
#define I2C_STATUS_NO_ACK  2
#define I2C_STATUS_NO_RESP 3

#define TWI_BAUD(F_SYS, F_TWI) ((F_SYS/(2*F_TWI))-5)
#define BAUD_400K 400000UL

#define TIMEOUT_TIME    1000    //Time in micro seconds to wait for a response of the slave

void    i2c_init(TWI_t *twi, uint8_t baudRateRegisterSetting);
uint8_t i2c_start(TWI_t *twi, uint8_t address, uint8_t rw);
uint8_t i2c_restart(TWI_t *twi, uint8_t address, uint8_t rw);
void    i2c_stop(TWI_t *twi);
uint8_t i2c_write(TWI_t *twi, uint8_t data);
uint8_t i2c_read(TWI_t *twi, uint8_t ack);