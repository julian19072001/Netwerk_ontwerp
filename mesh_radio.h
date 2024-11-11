// First byte:  Device Address 
// Second byte: Message number
// Third byte   Information type


#ifndef MESH_RADIO_H_
#define MESH_RADIO_H_

#include <stdio.h>
#include "nrf24L01.h"

#define NRF_RETRY_SPEED NRF_SETUP_ARD_1000US_gc         //if failed retry with a delay of 1000 us
#define NRF_NUM_RETRIES NRF_SETUP_ARC_8RETRANSMIT_gc    //if failed retry 8 times

#define NRF_POWER_LEVEL NRF_RF_SETUP_PWR_6DBM_gc        //power mode -6dB
#define NRF_DATA_RATE   NRF_RF_SETUP_RF_DR_250K_gc      //data rate: 250kbps
#define NRF_CRC_LENGTH  NRF_CONFIG_CRC_16_gc            //CRC check length

#define NRF_CHANNEL     24                              //NRF channel 24
#define NRF_AUTO_ACK    0                               //turn off auto acknowlage

#define DEVICE_ADDRESS  0x01                           //address of device (each address can only be used once in a network)

void radio_init(void);

void send_radio_data(uint8_t data_type, uint8_t* data, uint8_t data_size);
uint8_t get_radio_data(uint8_t* data);


#endif // MESH_RADIO_H_