/**************************************************************************/
/*!
@file     	i2c.c
@author   	Julian Della Guardia

Rewitten version of the i2c library to use on the HVA xMega.
@orignal	Wim Dolman
*/
/**************************************************************************/
#include "i2c.h"
#define F_CPU 32000000
#include <util/delay.h> 

uint64_t time;

#ifdef __AVR_ATxmega256A3U__
void i2c_init(TWI_t *twi, uint8_t baudRateRegisterSetting)
{
  PORTE.DIRSET	= PIN1_bm|PIN0_bm;                        //set pin E0 and E1 as outputs
	PORTE.PIN0CTRL	= PORT_OPC_WIREDANDPULL_gc;             //enable pullup for E0
	PORTE.PIN1CTRL	= PORT_OPC_WIREDANDPULL_gc;             //enable pullup for E1

  twi->MASTER.BAUD   = baudRateRegisterSetting;
  twi->MASTER.CTRLC  = 0;
  twi->MASTER.CTRLA  = TWI_MASTER_ENABLE_bm;
  twi->MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;
}

uint8_t i2c_start(TWI_t *twi, uint8_t address, uint8_t rw)
{
  if ( (twi->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) !=                      // if bus available
                       TWI_MASTER_BUSSTATE_IDLE_gc ) return I2C_STATUS_BUSY; //
  twi->MASTER.ADDR = (address << 1) | rw;                                    // send slave address
  while( ! (twi->MASTER.STATUS & (TWI_MASTER_WIF_bm << rw)) ){               // wait until sent
    time++;
    if(time >= TIMEOUT_TIME) return I2C_STATUS_NO_RESP;
    _delay_us(1);
  }
  if ( twi->MASTER.STATUS & TWI_MASTER_RXACK_bm ) {                          // if no ack
    twi->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
    return I2C_STATUS_NO_ACK;
  }

  return I2C_STATUS_OK;
}

uint8_t i2c_restart(TWI_t *twi, uint8_t address, uint8_t rw)
{
  twi->MASTER.ADDR = (address << 1) | rw;                                    // send slave address
  while( ! (twi->MASTER.STATUS & (TWI_MASTER_WIF_bm << rw)) ){               // wait until sent
    time++;
    if(time >= TIMEOUT_TIME) return I2C_STATUS_NO_RESP;
    _delay_us(1);
  }

  if ( twi->MASTER.STATUS & TWI_MASTER_RXACK_bm ) {                          // if no ack
    twi->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
    return I2C_STATUS_NO_ACK;
  }

  return I2C_STATUS_OK;
}

void i2c_stop(TWI_t *twi)
{
  twi->MASTER.CTRLC  = TWI_MASTER_CMD_STOP_gc;
  twi->MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;
}

uint8_t i2c_write(TWI_t *twi, uint8_t data)
{
  twi->MASTER.DATA = data;                                                   // send data
  while( ! (twi->MASTER.STATUS & TWI_MASTER_WIF_bm) ){               // wait until sent
    time++;
    if(time >= TIMEOUT_TIME) return I2C_STATUS_NO_RESP;
    _delay_us(1);
  }

  if ( twi->MASTER.STATUS & TWI_MASTER_RXACK_bm ) {                          // if no ack
    twi->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
    return I2C_STATUS_NO_ACK;
  }

  return I2C_STATUS_OK;
}

uint8_t i2c_read(TWI_t *twi, uint8_t ack)
{
  uint8_t data;

  while( ! (twi->MASTER.STATUS & TWI_MASTER_WIF_bm) ){               // wait until sent
    time++;
    if(time >= TIMEOUT_TIME) return 0;
    _delay_us(1);
  }

  data = twi->MASTER.DATA;                                                   // read data
  twi->MASTER.CTRLC = ((ack==I2C_ACK) ? TWI_MASTER_CMD_RECVTRANS_gc :        // send ack (go on) or
                               TWI_MASTER_ACKACT_bm|TWI_MASTER_CMD_STOP_gc); //     nack (and stop)

  if ( ack == I2C_NACK ) {
    while( ! (twi->MASTER.STATUS & TWI_MASTER_BUSSTATE_IDLE_gc) );
  }

  return data;
}
#endif


#ifdef __AVR_ATtiny414__
void i2c_init(TWI_t *twi, uint8_t baudRateRegisterSetting)
{
  PORTB.DIRSET	= PIN1_bm|PIN0_bm;                                      //set pin B0 and B1 as outputs
	PORTB.PIN0CTRL	= PORT_PULLUPEN_bp;                                   //enable pullup for B0
	PORTB.PIN1CTRL	= PORT_PULLUPEN_bp;                                   //enable pullup for B1

  twi->MBAUD   = baudRateRegisterSetting;
  twi->MCTRLA  = TWI_ENABLE_bm;
  twi->MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

uint8_t i2c_start(TWI_t *twi, uint8_t address, uint8_t rw)
{
  if ( (twi->MSTATUS & TWI_BUSSTATE_gm) !=                              // if bus available
                       TWI_BUSSTATE_IDLE_gc ) return I2C_STATUS_BUSY;   //
  twi->MADDR = (address << 1) | rw;                                     // send slave address
  while( ! (twi->MSTATUS & (TWI_WIF_bm << rw)) ){                       // wait until sent
    time++;
    if(time >= TIMEOUT_TIME) return I2C_STATUS_NO_RESP;
    _delay_us(1);
  }

  if ( twi->MSTATUS & TWI_RXACK_bm ) {                                  // if no ack
    return I2C_STATUS_NO_ACK;
  }

  return I2C_STATUS_OK;
}

uint8_t i2c_restart(TWI_t *twi, uint8_t address, uint8_t rw)
{
  twi->MADDR = (address << 1) | rw;                                     // send slave address
  while( ! (twi->MSTATUS & (TWI_WIF_bm << rw)) ){                       // wait until sent
    time++;
    if(time >= TIMEOUT_TIME) return I2C_STATUS_NO_RESP;
    _delay_us(1);
  }

  if ( twi->MSTATUS & TWI_RXACK_bm ) {                                  // if no ack
    return I2C_STATUS_NO_ACK;
  }

  return I2C_STATUS_OK;
}

void i2c_stop(TWI_t *twi)
{
  twi->MSTATUS = TWI_BUSSTATE_IDLE_gc;
  twi->MCTRLB |= TWI_MCMD_STOP_gc;
}

uint8_t i2c_write(TWI_t *twi, uint8_t data)
{
  twi->MDATA = data;                                                    // send data
  while( ! (twi->MSTATUS & TWI_WIF_bm) ){                               // wait until sent
    time++;
    if(time >= TIMEOUT_TIME) return I2C_STATUS_NO_RESP;
    _delay_us(1);
  }

  if ( twi->MSTATUS & TWI_RXACK_bm ) {                                  // if no ack
    return I2C_STATUS_NO_ACK;
  }

  return I2C_STATUS_OK;
}

uint8_t i2c_read(TWI_t *twi, uint8_t ack)
{
  uint8_t data;

  while( ! (twi->MSTATUS & TWI_RIF_bm) ){                               // wait until sent
    time++;
    if(time >= TIMEOUT_TIME) return 0;
    _delay_us(1);
  }

  data = twi->MDATA;                                                    // read data

  if ( ack == I2C_NACK ) {
    while( ! (twi->MSTATUS & TWI_BUSSTATE_IDLE_gc) );
  }

  return data;
}
#endif