#ifndef INFRARED_DRIVER_HPP_
#define INFRARED_DRIVER_HPP_

#include "stdint.h"

void Infrared_Activate_FHT(uint8_t* code, uint8_t repeat);
void FHT_Zigbee_RemoteActive(uint8_t* custom_code);

#endif 
