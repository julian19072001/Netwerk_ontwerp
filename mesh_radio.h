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

#define NRF_CHANNEL     54                              //NRF channel 54
#define NRF_PIPE  {0x30, 0x47, 0x72, 0x70, 0x45} 	    //pipe address ""

#define MAX_SENDERS 50                                  //Maximum number of senders
#define MAX_DATA_LENGTH 28                              //Length of the payload data (excluding the ID)
#define WEIGHT_THRESHOLD 30                             //Threshold above which a package is trusted
#define WEIGHT_LOWER_TIME 100                           //Time in milliseconds it takes for the weight to lower by 1 
#define WEIGHT_MAX 70                                   //Maximium number a weight can be
#define MAX_HOPS 15                                     //Maximium amount of hops before sensor gets dropped (to prevent ghost nodes) 

#define RX_BUFFER_DEPTH     200

#define COMMAND_PING        0x01
#define COMMAND_PING_END    0x02
#define COMMAND_DATA        0x03

#define BASE_ADDRESS        0x40

// Initialize the NRF radio, needs to be given a address so the network can identify how it is
void radioInit(uint8_t set_address);


// Send data over NRF
// targetId = Intented destination for data
// data = data to be send
// dataSize = number of bytes to be send (Cannot go beyond MAX_DATA_LENGHT)
void sendRadioData(uint8_t target_id, uint8_t* data, uint8_t data_size, bool encrypt);

// Check if there is data in the buffer to be read
uint8_t canReadRadio(void);

// read the first received message in buffer, returns the number of bytes in the message
// dataLocation = place where data needs to be saved
uint8_t readRadioMessage(uint8_t *dataLocation);


// Prints all the neighbors and there important values
void printNeighbors();

#endif // MESH_RADIO_H_