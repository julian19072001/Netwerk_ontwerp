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

#define MAX_PACKAGES 50                                 //Maximum number of packages
#define MAX_DATA_LENGTH 30                              //Length of the payload data (excluding the ID)
#define MAX_MESSAGE_SIZE 28                             //Maximum number of bytes that fit in one message
#define WEIGHT_THRESHOLD 30                             //Threshold above which a package is trusted
#define WEIGHT_LOWER_TIME 100                           //Time in milliseconds it takes for the weight to lower by 1 
#define WEIGHT_MAX 100                                  //Maximium number a weight can be 

// Structure to store package data
typedef struct {
    uint8_t id;                         //ID (first byte of the data)
    uint8_t target_id;                  //target ID (seconde byte of data)
    uint8_t payload[MAX_DATA_LENGTH];   //Data payload (remaining 30 bytes)
    int weight;                         //Weight of the package
    uint8_t unread_data;                //Flag to indicate if data in payload has been used or not
    uint8_t in_use;                     //Flag to indicate if the package slot is in use
    uint8_t trusted;                    //Flag to indicate if the package is trusted
    uint8_t owner;                      //Set from what route the ID is the closest
    uint8_t hops;                       //Number of hops to the receiver
} Package;

void radio_init(uint8_t set_address);

void send_radio_data(uint8_t command, uint8_t target_id, uint8_t* data, uint8_t data_size);
Package* get_radio_data_by_id(uint8_t id);

void print_packages();

#endif // MESH_RADIO_H_