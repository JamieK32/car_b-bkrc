#ifndef __K210_PROTOCOL_HPP__
#define __K210_PROTOCOL_HPP__

#include <Arduino.h>
#include <stdint.h>

/* ================= FPGA SRAM 地址映射 ================= */
#define K210_TX_ADDR        0x6008
#define K210_RX_BASE_ADDR   0x6038
#define K210_RX_SAFE_END    0x607F          // 不跨 0x6080
#define K210_RX_MAX_LEN     (K210_RX_SAFE_END - K210_RX_BASE_ADDR + 1) // 72 字节

/* ================= 接收缓冲区清空保护 ================= */
#define K210_RX_CLEAR_GUARD_BYTES   15

/* ================= 帧固定常量 ================= */
#define K210_PKT_HEAD_MSB   0x55
#define K210_PKT_HEAD_LSB   0x02
#define K210_PKT_GATE_CMD   0x91
#define K210_PKT_TAIL       0xBB

/* ================= 命令 ================= */
typedef enum {
    CMD_VAR_LEN            = 0x03,
    CMD_SET_SERVO_POSITIVE = 0x04,
    CMD_SET_SERVO_NEGATIVE = 0x05,
    CMD_TRACE_START        = 0x06,
    CMD_TRACE_STOP         = 0x07,
    CMD_TRACE_RESULT       = 0x08,
    CMD_TRAFFIC_START      = 0x09,
    CMD_TRAFFIC_STOP       = 0x0A,
    CMD_TRAFFIC_RESULT     = 0x0B,
    CMD_QR_CODE_START      = 0x0C,
    CMD_QR_CODE_STOP       = 0x0D,
} K210_CMD;


/* ================= 解析后的帧结构 ================= */
typedef struct
{
    uint8_t  cmd;
    uint16_t clear_len;   // ✅ 本帧建议清除的 RX 字节数

    union
    {
        struct
        {
            uint8_t data1;
            uint8_t data2;
        } fixed;

        struct
        {
            uint8_t var_id;
            uint8_t data_len;
            uint8_t data[K210_RX_MAX_LEN];
        } var;
    };
} K210_Frame;

typedef enum {
    K210_READ_FIXED = 0x01,
    K210_READ_VAR_LEN,
} K210_ReadMode;



/* ================= 接口函数 ================= */
void K210_SerialInit(void);
void K210_SendCmd(uint8_t cmd, uint8_t data1, uint8_t data2);
bool K210_ReadFrame(K210_Frame *frame, K210_ReadMode mode);
void K210_ClearRxByFrame(const K210_Frame *frame);

#endif /* __K210_PROTOCOL_HPP__ */
