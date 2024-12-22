/*Julian Della Guardia*/


#ifndef ADDRESS_H_
#define ADDRESS_H_

#include <stdio.h>

#define BASE_ADDRESS                0x40
#define TEMP_HUMID_START_ADDRESS    0x41
#define TEMP_HUMID_END_ADDRESS      0x4F
#define LIGHT_START_ADDRESS         0x50
#define LIGHT_END_ADDRESS           0x5F
#define GROUND_WATER_START_ADDRESS  0x60
#define GROUND_WATER_END_ADDRESS    0x6F

uint8_t getSensorAddress(void);

#endif