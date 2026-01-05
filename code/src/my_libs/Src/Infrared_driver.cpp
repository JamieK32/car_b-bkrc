#include "Infrared_driver.hpp"
#include "infrare.h"
#include "Drive.h"
#include "BEEP.h"
#include "BH1750.h"
#include "ExtSRAMInterface.h"
#include "log.hpp"

#define FHT_DATA_SIZE 6
#define FHT_TX_GAP_MS   30

// ============================================================================
//                              烽火台激活
// ============================================================================
void Infrared_Activate_FHT(uint8_t* code, uint8_t repeat) {
    if (!code) return;
    for (uint8_t i = 0; i < repeat; i++) {
        Infrare.Transmition(code, FHT_DATA_SIZE);
        delay(FHT_TX_GAP_MS);
    }
}