#ifndef STREET_LIGHT_H__
#define STREET_LIGHT_H__

#include "stdint.h"

void Infrared_Street_Light_Set(uint8_t gear);
uint8_t Infrared_Street_Light_GetGear(void);

#endif 
