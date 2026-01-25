#pragma once

#include "isp_config.h"

void isp_start_pmode();
void isp_end_pmode();
uint8_t isp_spi_transaction(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
