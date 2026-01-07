#include "k210_protocol.hpp"
#include "ExtSRAMInterface.h"
#include "log.hpp"
#include <string.h>


/******************************************************************************
 * K210 ↔ FPGA ↔ Arduino 通信机制说明（实测 & 硬件约束版）
 *
 * 【核心结论】
 * FPGA 并非 UART 透明转发器，而是具备：
 *   - 帧同步识别
 *   - 校验判断
 *   - SRAM 缓冲写入
 *
 * 并且：
 *   - 发送方向（K210 → FPGA）存在【严格帧格式限制】
 *   - 接收方向（FPGA → Arduino）为【变长裸数据缓冲】
 *
 * 两个方向规则完全不同，必须区别对待。
 *
 * ============================================================================
 * 1. K210 → FPGA 发送规则（硬件强制，必须遵守）
 * ============================================================================
 *
 * FPGA 仅在检测到【完整、合法】的固定长度帧后，
 * 才会将有效数据写入 SRAM 并对外转发。
 *
 * -------------------------
 * 固定帧格式（8 字节）
 * -------------------------
 *
 *   Byte0 : K210_PKT_HEAD_MSB   = 0x55
 *   Byte1 : K210_PKT_HEAD_LSB   = 0x02
 *   Byte2 : K210_PKT_GATE_CMD   = 0x91   // 硬件门控字节（固定）
 *   Byte3 : data1
 *   Byte4 : data2
 *   Byte5 : data3
 *   Byte6 : chk                // chk = Byte2~Byte5 求和 & 0xFF
 *   Byte7 : K210_PKT_TAIL       = 0xBB
 *
 * ⚠【重要硬件限制】
 *   - Byte2 仅允许 0x91 或 0x92
 *   - 若为其他值：FPGA 直接丢弃整帧
 *   - 为避免误发，工程上【强制固定为 0x91】
 *
 * 因此：
 *   - 帧长度必须为 8 字节
 *   - 帧头 / 校验 / 帧尾缺一不可
 *   - 否则 FPGA 不写 SRAM、不转发
 *
 * 上述行为为 FPGA 硬件逻辑限制，非软件 Bug。
 *
 * ============================================================================
 * 2. FPGA → Arduino 接收规则（SRAM 读取）
 * ============================================================================
 *
 * FPGA 会将 UART 接收到的【有效载荷】连续写入 SRAM，
 * Arduino 侧通过外部 SRAM 接口主动读取。
 *
 * 接收基地址：
 *
 *   #define K210_RX_BASE_ADDR   0x6038
 *
 * 读取方式示例：
 *
 *   ExtSRAMInterface.ExMem_Read_Bytes(
 *       K210_RX_BASE_ADDR,
 *       frame->buf,
 *       N           // N 为变长，可 > 8
 *   );
 *
 * 【关键特性】
 *   - 接收数据为【变长数据】
 *   - FPGA 不解析协议，仅做 UART → SRAM 写入
 *
 * ============================================================================
 * 3. 帧头吞噬现象（极易踩坑，已实测）
 * ============================================================================
 *
 * 在第一次读取 SRAM 时，可能出现：
 *
 *   buf[0] == 0x55
 *
 * 但在第二次及后续读取同一地址时：
 *
 *   buf[0] 会被刷新为 0x00
 *   buf[1] 及后续字节保持不变
 *
 * 【原因分析】
 *   - FPGA 使用 0x55 作为“帧同步触发字节”
 *   - 该字节不属于稳定的数据载荷
 *
 * ⚠【接收端强制规范】
 *   - 禁止使用 buf[0] == 0x55 作为帧判断条件
 *   - 必须从 buf[1]（0x02）或 CMD 字段开始解析
 *
 * ============================================================================
 * 4. SRAM 缓冲区管理建议（强烈建议遵守）
 * ============================================================================
 *
 * SRAM 缓冲区内容不会自动清空：
 *
 *   - 每次解析完一帧数据后
 *   - 必须主动清空或覆盖对应缓冲区
 *
 * 否则可能导致：
 *   - 新旧数据混合
 *   - 重复解析历史数据
 *
 * ============================================================================
 * 5. 变长数据 SRAM 有效区间（经验 + 推测）
 * ============================================================================
 *
 * 已确认起始地址：
 *
 *   K210_RX_BASE_ADDR = 0x6038
 *
 * 推测有效范围：
 *
 *   [0x6038, 0x6080)
 *
 * 说明：
 *   - 0x6080 在官方文档中用于其他功能
 *   - 因此建议仅使用 0x6038 ~ 0x607F
 *   - 实际上限需结合 FPGA RTL 进一步确认
 *
 * ⚠ 注意：
 *   - 禁止使用 K210_RX_BASE_ADDR + 1 作为起始读取地址
 *   - 必须始终从 K210_RX_BASE_ADDR 开始读取
 *
 *****************************************************************************/

#include <Arduino.h>

#define LOG_K210_PROTOCOL_EN 0

#if LOG_K210_PROTOCOL_EN
  #define log_k210(fmt, ...)  LOG_P("[K210_PROTOCOL] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define log_k210(...)  do {} while(0)
#endif


/* ================= 编译开关：启用 2 ================= */
#define K210_USE_SERIAL

#ifdef K210_USE_SERIAL
  #define K210_PORT                 Serial1
  #ifndef K210_BAUDRATE
    #define K210_BAUDRATE           115200
  #endif
  #ifndef K210_SERIAL_TIMEOUT_MS
    #define K210_SERIAL_TIMEOUT_MS  50
  #endif
#endif

/* ============ 2 辅助函数（仅 2 模式需要） ============ */
#ifdef K210_USE_SERIAL

static bool K210_SerialReadExact(uint8_t *dst, size_t len, uint32_t timeout_ms)
{
    uint32_t t0 = millis();
    size_t got = 0;
    while (got < len)
    {
        if (K210_PORT.available() > 0)
        {
            int c = K210_PORT.read();
            if (c >= 0) dst[got++] = (uint8_t)c;
        }
        if ((millis() - t0) > timeout_ms) return false;
    }
    return true;
}

/* 找到帧头并对齐成：buf[0]=0x55, buf[1]=0x02
 * 兼容：0x55 被吞（直接从 0x02 开始）
 */
static bool K210_SerialSyncHead(uint32_t timeout_ms)
{
    uint32_t t0 = millis();
    while ((millis() - t0) <= timeout_ms)
    {
        if (K210_PORT.available() <= 0) continue;

        int c = K210_PORT.read();
        if (c < 0) continue;

        uint8_t b0 = (uint8_t)c;

        if (b0 == K210_PKT_HEAD_MSB) // 0x55
        {
            uint8_t b1;
            if (!K210_SerialReadExact(&b1, 1, timeout_ms)) return false;
            if (b1 == K210_PKT_HEAD_LSB) return true;     // 0x55 0x02
            // 否则继续找（把 b1 当作流的一部分丢掉即可）
        }
        else if (b0 == K210_PKT_HEAD_LSB) // 0x02（认为 0x55 被吞）
        {
            return true;
        }
    }
    return false;
}

static void K210_SerialDrain(size_t max_bytes)
{
    while (K210_PORT.available() > 0 && max_bytes--)
    {
        (void)K210_PORT.read();
    }
}

#endif // K210_USE_SERIAL


void K210_SerialInit(void)
{
#ifdef K210_USE_SERIAL
    K210_PORT.begin(K210_BAUDRATE);
    // 可选：等待串口稳定
    delay(10);

    // 可选：清空串口缓冲
    while (K210_PORT.available() > 0) (void)K210_PORT.read();
#else
    // 你原来若有 SRAM/FPGA 初始化放这里；没有就保持空
#endif
}


/* ================= 发送命令 ================= */
void K210_SendCmd(uint8_t cmd, uint8_t data1, uint8_t data2)
{
    uint8_t chk = (K210_PKT_GATE_CMD + cmd + data1 + data2) & 0xFF;

    uint8_t pkt[8] =
    {
        K210_PKT_HEAD_MSB,     // 0x55
        K210_PKT_HEAD_LSB,     // 0x02
        K210_PKT_GATE_CMD,     // 0x91
        cmd,
        data1,
        data2,
        chk,
        K210_PKT_TAIL          // 0xBB
    };

    log_k210("K210 Send Pkt: %02X %02X %02X %02X %02X %02X %02X %02X",
          pkt[0], pkt[1], pkt[2], pkt[3],
          pkt[4], pkt[5], pkt[6], pkt[7]);

#ifdef K210_USE_SERIAL
    K210_PORT.write(pkt, 8);
    K210_PORT.flush();
#else
    ExtSRAMInterface.ExMem_Write_Bytes(K210_TX_ADDR, pkt, 8);
    delay(50);
#endif
}


void K210_ClearRxByFrame(const K210_Frame *frame)
{
#ifdef K210_USE_SERIAL
    // Serial 模式：把串口接收缓冲区“读掉”
    // 优先按 frame->clear_len 清；如果不合理就清到空
    size_t n = frame ? frame->clear_len : 0;
    if (n == 0 || n > K210_RX_MAX_LEN) n = K210_RX_MAX_LEN;

    K210_SerialDrain(n);

    // 保险：再把残留全部清空（可按需注释掉）
    while (K210_PORT.available() > 0) (void)K210_PORT.read();
#else
    uint16_t max_clear_len = K210_RX_MAX_LEN - K210_RX_CLEAR_GUARD_BYTES;
    uint16_t clear_len = frame->clear_len;

    if (clear_len == 0 || clear_len > max_clear_len) {
        clear_len = max_clear_len;
    }

    static uint8_t zero_buf[K210_RX_MAX_LEN] = {0};

    ExtSRAMInterface.ExMem_Write_Bytes(
        K210_RX_BASE_ADDR,
        zero_buf,
        clear_len
    );
#endif
}


bool K210_ReadFrame(K210_Frame *frame, K210_ReadMode mode)
{
    static uint8_t buf[K210_RX_MAX_LEN];

#ifdef K210_USE_SERIAL

    if (!frame) return false;

    /* 先同步到帧头（0x55 0x02 或 0x02 视为 0x55 被吞） */
    if (!K210_SerialSyncHead(K210_SERIAL_TIMEOUT_MS)) return false;

    // 对齐写入 buf[0..1]
    buf[0] = K210_PKT_HEAD_MSB; // 即便 0x55 被吞，也补上，保证索引一致
    buf[1] = K210_PKT_HEAD_LSB;

    /* ================= 定长帧 ================= */
    if (mode == K210_READ_FIXED)
    {
        // 已经有 2 字节头，再读剩余 6 字节凑够 8
        if (!K210_SerialReadExact(&buf[2], 6, K210_SERIAL_TIMEOUT_MS)) return false;

        if (buf[1] != K210_PKT_HEAD_LSB) return false;
        if (buf[2] != K210_PKT_GATE_CMD) return false;
        if (buf[3] == CMD_VAR_LEN)       return false;
        if (buf[7] != K210_PKT_TAIL)     return false;

        uint8_t chk = (uint8_t)((buf[2] + buf[3] + buf[4] + buf[5]) & 0xFF);
        if (chk != buf[6]) return false;

        frame->cmd         = buf[3];
        frame->fixed.data1 = buf[4];
        frame->fixed.data2 = buf[5];
        frame->clear_len   = 8;
        return true;
    }
    /* ================= 变长帧 ================= */
    else
    {
        // 已经有 buf[0..1]，再读 [2][3][4] => gate/cmd/len
        if (!K210_SerialReadExact(&buf[2], 3, K210_SERIAL_TIMEOUT_MS)) return false;

        if (buf[1] != K210_PKT_HEAD_LSB) return false;
        if (buf[2] != K210_PKT_GATE_CMD) return false;
        if (buf[3] != CMD_VAR_LEN)       return false;

        uint8_t payload_len = buf[4];     // payload = [var_id][data...]
        if (payload_len < 1) return false;

        uint16_t total_len = (uint16_t)payload_len + 7; // 与你原逻辑一致
        if (total_len > K210_RX_MAX_LEN) return false;

        // 还差 total_len - 5 字节：从 buf[5] 开始读 payload/chk/tail
        if (!K210_SerialReadExact(&buf[5], (size_t)(total_len - 5), K210_SERIAL_TIMEOUT_MS))
            return false;

        if (buf[total_len - 1] != K210_PKT_TAIL) return false;

        uint8_t chk = 0;
        for (uint16_t i = 2; i < (uint16_t)(total_len - 2); i++) {
            chk += buf[i];
        }
        chk &= 0xFF;

        if (chk != buf[total_len - 2]) return false;

        frame->cmd           = CMD_VAR_LEN;
        frame->var.var_id    = buf[5];
        frame->var.data_len  = payload_len - 1;

        if (frame->var.data_len > 0) {
            memcpy(frame->var.data, &buf[6], frame->var.data_len);
        }

        frame->clear_len = total_len;
        return true;
    }

#else
    /* ============ 你原来的 SRAM 版本原封不动保留 ============ */

    /* ================= 定长帧 ================= */
    if (mode == K210_READ_FIXED)
    {
        ExtSRAMInterface.ExMem_Read_Bytes(K210_RX_BASE_ADDR, buf, 8);

        if (buf[1] != K210_PKT_HEAD_LSB) return false;
        if (buf[2] != K210_PKT_GATE_CMD) return false;
        if (buf[3] == CMD_VAR_LEN) return false;
        if (buf[7] != K210_PKT_TAIL) return false;

        uint8_t chk = (uint8_t)((buf[2] + buf[3] + buf[4] + buf[5]) & 0xFF);
        if (chk != buf[6]) return false;

        frame->cmd         = buf[3];
        frame->fixed.data1 = buf[4];
        frame->fixed.data2 = buf[5];
        frame->clear_len   = 8;

        return true;
    }
    /* ================= 变长帧 ================= */
    else
    {
        ExtSRAMInterface.ExMem_Read_Bytes(K210_RX_BASE_ADDR, buf, 5);

        if (buf[1] != K210_PKT_HEAD_LSB) return false;
        if (buf[2] != K210_PKT_GATE_CMD) return false;
        if (buf[3] != CMD_VAR_LEN)       return false;

        uint8_t payload_len = buf[4];
        if (payload_len < 1) return false;

        uint16_t total_len = (uint16_t)payload_len + 7;
        if (total_len > K210_RX_MAX_LEN) return false;

        ExtSRAMInterface.ExMem_Read_Bytes(
            K210_RX_BASE_ADDR + 5,
            &buf[5],
            total_len - 5
        );

        if (buf[total_len - 1] != K210_PKT_TAIL) return false;

        uint8_t chk = 0;
        for (uint16_t i = 2; i < (uint16_t)(total_len - 2); i++) {
            chk += buf[i];
        }
        chk &= 0xFF;

        if (chk != buf[total_len - 2]) return false;

        frame->cmd        = CMD_VAR_LEN;
        frame->var.var_id = buf[5];
        frame->var.data_len = payload_len - 1;

        if (frame->var.data_len > 0) {
            memcpy(frame->var.data, &buf[6], frame->var.data_len);
        }

        frame->clear_len = total_len;
        return true;
    }
#endif
}
