#ifndef VEHICLE_EXCHANGE_H__
#define VEHICLE_EXCHANGE_H__

#include "stdint.h"
#include "stdbool.h"

#define CAR_A 1
#define CAR_B  2
#define CURRENT_CAR CAR_B

#if CURRENT_CAR == CAR_A
    #include "app_include.h"
    #include "fifo_drv.h"
#elif CURRENT_CAR == CAR_B
    #include "ExtSRAMInterface.h"
    #include "log.hpp"
    #define delay_ms(ms) delay(ms)
#endif

/* ===================== DataType ===================== */
typedef enum
{
    DATA_TYPE_UNKNOWN = 0,            // 未知类型
    DATA_TYPE_CUSTOM = 1,            // 自定义类型
    DATA_TYPE_CMD = 2,
    DATA_TYPE_MAX                     // 最大枚举标识
} DataType;

/* FPGA SRAM addr */
#define TX_DATA_ADDR    0x6080
#define RX_DATA_ADDR    0x6100

/* ===================== Protocol ===================== */
#define ZIGBEE_HEAD0    0x55
#define ZIGBEE_HEAD1    0x02
#define ZIGBEE_TAIL     0xBB
#define CTRL_FIRST      0x80
#define CTRL_LAST       0x40
#define CTRL_META       0x20
#define CTRL_SEQ_MASK   0x1F  
#define META_GET        0x00
#define MAX_PAYLOAD_LEN 255
#define RX_BUF_LEN      256
#define READ_REPEAT 	100
#define META_START      0x10
#define META_CRC8       0x11
#define META_ACK        0xF0
#define ACK_OK          0x06  

#define TX_REPEAT_COUNT     3   /* 1=不重复；2=整包发两遍（推荐） */
#define TX_INTERFRAME_GAP_MS  15 /* 帧间隔，CAR_B 如需更大可改 */

/* ===================== frame parse ===================== */
typedef struct {
    uint8_t type;
    uint8_t ctrl;
    uint8_t arg;
    uint8_t data;
    uint8_t seq;
    bool    first;
    bool    last;
    bool    meta;
} ParsedFrame;

bool SendArray(DataType type, const uint8_t *data, uint16_t len, uint32_t ack_timeout_ms);
uint8_t *GetData(DataType type, uint32_t timeout_ms); 
uint8_t *WaitData(DataType type, uint32_t timeout_ms);
uint8_t GetDataLen(void);


static bool ParseFrame(const uint8_t pkt[8], ParsedFrame *f);

/* ===================== checksum (frame-level) ===================== */
static inline uint8_t calc_cs(uint8_t type, uint8_t ctrl, uint8_t arg, uint8_t data)
{
    return (uint8_t)((type + ctrl + arg + data) & 0xFF);
}


static inline void SendFrame8(uint8_t type, uint8_t ctrl, uint8_t arg, uint8_t data)
{
    uint8_t tx[8];
    tx[0] = ZIGBEE_HEAD0;
    tx[1] = ZIGBEE_HEAD1;
    tx[2] = type;
    tx[3] = ctrl;
    tx[4] = arg;
    tx[5] = data;
    tx[6] = calc_cs(tx[2], tx[3], tx[4], tx[5]); /* FPGA 要求 cs 在 tx[6] */
    tx[7] = ZIGBEE_TAIL;

#if CURRENT_CAR == CAR_A
    Send_ZigbeeData_To_Fifo(tx, 8);
#elif CURRENT_CAR == CAR_B
    ExtSRAMInterface.ExMem_Write_Bytes(TX_DATA_ADDR, tx, 8);
#endif
    if (TX_INTERFRAME_GAP_MS) delay(TX_INTERFRAME_GAP_MS);
}

/* ===================== low level read ===================== */
/* ===================== low level read (CAR_A improved) ===================== */
/* 返回 true 表示确实组到 1 帧 8 字节（已经对齐到帧头） */
static inline bool ZigbeeRead8(uint8_t out8[8])
{
#if CURRENT_CAR == CAR_A
    extern Fifo_Drv_Struct Fifo_ZigbRx;

    /* 跨调用保存接收状态，避免“半帧超时导致错位” */
    static uint8_t buf[8];
    static uint8_t state = 0;   // 0:找HEAD0  1:找HEAD1  2:收payload
    static uint8_t idx = 0;

    /* 每次进来尽量多吃一些现有字节，但不要死等 */
    for (uint32_t n = 0; n < READ_REPEAT; n++) {
        uint8_t b;
        if (!FifoDrv_ReadOne(&Fifo_ZigbRx, &b)) {
            return false; // 现在没字节了，下次再来继续
        }

        switch (state) {
        case 0: // seek HEAD0
            if (b == ZIGBEE_HEAD0) {
                buf[0] = b;
                state = 1;
            }
            break;

        case 1: // seek HEAD1
            if (b == ZIGBEE_HEAD1) {
                buf[1] = b;
                idx = 2;
                state = 2;
            } else if (b == ZIGBEE_HEAD0) {
                /* 处理重叠：...HEAD0 HEAD0 HEAD1... */
                buf[0] = b;
                state = 1;
            } else {
                state = 0;
            }
            break;

        case 2: // collect remaining bytes
            buf[idx++] = b;
            if (idx == 8) {
                memcpy(out8, buf, 8);
                state = 0;
                idx = 0;
                return true;
            }
            break;
        }
    }

    return false;

#elif CURRENT_CAR == CAR_B
    uint8_t flag = ExtSRAMInterface.ExMem_Read(RX_DATA_ADDR);
    if (flag == 0x00) return false;

    ExtSRAMInterface.ExMem_Read_Bytes(RX_DATA_ADDR, out8, 8);

    uint8_t zero[8] = {0};
    ExtSRAMInterface.ExMem_Write_Bytes(RX_DATA_ADDR, zero, 8);
    return true;
#endif
}


#endif 
