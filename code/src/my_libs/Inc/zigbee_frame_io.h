#ifndef ZIGBEE_FRAME_IO_H__
#define ZIGBEE_FRAME_IO_H__


/* FPGA SRAM addr */
#define TX_DATA_ADDR 0x6080
#define RX_DATA_ADDR 0x6100

#define ZIGBEE_HEAD0 0x55
#define ZIGBEE_HEAD1 0x02
#define ZIGBEE_TAIL  0xBB

#define CAR_A 1
#define CAR_B 2
#define CURRENT_CAR CAR_B

#if CURRENT_CAR == CAR_A
    #include "app_include.h"
    #include "fifo_drv.h"
    #define get_ms() gt_get()
    /* CAR_A 若没有 delay_ms，就让 yield 为空 */
    #ifndef delay_ms
    #define delay_ms(ms) do { (void)(ms); } while(0)
    #endif
#elif CURRENT_CAR == CAR_B
    #include "ExtSRAMInterface.h"
    #include "log.hpp"
    #define delay_ms(ms) delay(ms)
    #define get_ms() millis()
#endif

/* ===================== send 8-byte frame ===================== */
void SendFrame8(uint8_t type, uint8_t ctrl, uint8_t arg, uint8_t data);
bool ZigbeeRead8(uint8_t out8[8]);


#endif 
