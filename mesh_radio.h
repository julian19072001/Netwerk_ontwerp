/*Julian Della Guardia*/

// First byte:  Device Address 
// Second byte: Message number
// Third byte   Information type


#ifndef MESH_RADIO_H_
#define MESH_RADIO_H_

#include <stdio.h>
#include "nrf24L01.h"

#define NRF_RETRY_SPEED NRF_SETUP_ARD_1000US_gc         //if failed retry with a delay of 1000 us
#define NRF_NUM_RETRIES NRF_SETUP_ARC_8RETRANSMIT_gc    //if failed retry 8 times

#define NRF_POWER_LEVEL NRF_RF_SETUP_PWR_18DBM_gc       //power mode -6dB
#define NRF_DATA_RATE   NRF_RF_SETUP_RF_DR_250K_gc      //data rate: 250kbps
#define NRF_CRC_LENGTH  NRF_CONFIG_CRC_16_gc            //CRC check length
#define NRF_AUTO_ACK    0                               //turn off auto acknowlage

#define NRF_CHANNEL     24                              //NRF channel 24
#define NRF_PIPE  {0x30, 0x47, 0x72, 0x70, 0x44} 	    //pipe address ""

#define MAX_SENDERS 50                                  //Maximum number of senders
#define MAX_DATA_LENGTH 28                              //Length of the payload data (excluding the ID)
#define WEIGHT_THRESHOLD 30                             //Threshold above which a package is trusted
#define WEIGHT_LOWER_TIME 100                           //Time in milliseconds it takes for the weight to lower by 1 
#define WEIGHT_MAX 100                                  //Maximium number a weight can be 

#define COMMAND_PING        0x01
#define COMMAND_PING_END    0x02
#define COMMAND_DATA        0x03

void radioInit(uint8_t set_address);

void sendRadioData(uint8_t target_id, uint8_t* data, uint8_t data_size);

void printPackages();

#endif // MESH_RADIO_H_