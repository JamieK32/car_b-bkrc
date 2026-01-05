#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "vehicle_exchange.h"

/* ===================== globals ===================== */
static uint8_t g_seq = 0;

/* ===================== CRC8 (message-level) ===================== */
/* 多项式 0x07, 初值 0x00，简单可靠，开销低 */
static uint8_t crc8_update(uint8_t crc, uint8_t data)
{
    crc ^= data;
    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x07);
        else           crc = (uint8_t)(crc << 1);
    }
    return crc;
}

static uint8_t crc8_calc(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) crc = crc8_update(crc, data[i]);
    return crc;
}

static inline void SendAck(DataType type, uint8_t seq, uint8_t ack_byte)
{
    /* 用 META 帧回 ACK：type+seq 绑定 */
    SendFrame8((uint8_t)type, (uint8_t)(CTRL_META | (seq & CTRL_SEQ_MASK)), META_ACK, ack_byte);
}

/* 等待 ACK：收到 META_ACK(type,seq) 且 data==ACK_OK 即成功 */
static bool WaitAck(DataType type, uint8_t seq, uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    uint8_t pkt[8];
    ParsedFrame f;

    while (elapsed < timeout_ms) {
        if (ZigbeeRead8(pkt)) {
            if (ParseFrame(pkt, &f)) {
                if (f.meta &&
                    f.arg == META_ACK &&
                    f.type == (uint8_t)type &&
                    f.seq  == (uint8_t)(seq & CTRL_SEQ_MASK))
                {
                    return (f.data == ACK_OK);
                }
            }
        }
        delay_ms(1);
        elapsed++;
    }
    return false;
}

static bool ParseFrame(const uint8_t pkt[8], ParsedFrame *f)
{
    if (!pkt || !f) return false;
    if (pkt[0] != ZIGBEE_HEAD0 || pkt[1] != ZIGBEE_HEAD1) return false;
    if (pkt[7] != ZIGBEE_TAIL) return false;   // ✅补上

    uint8_t type = pkt[2];
    uint8_t ctrl = pkt[3];
    uint8_t arg  = pkt[4];
    uint8_t dat  = pkt[5];
    uint8_t cs   = pkt[6];

    if (calc_cs(type, ctrl, arg, dat) != cs) return false;

    f->type  = type;
    f->ctrl  = ctrl;
    f->arg   = arg;
    f->data  = dat;
    f->seq   = (uint8_t)(ctrl & CTRL_SEQ_MASK);
    f->first = (ctrl & CTRL_FIRST) != 0;
    f->last  = (ctrl & CTRL_LAST)  != 0;
    f->meta  = (ctrl & CTRL_META)  != 0;
    return true;
}

/* ===================== RX reassembly (simple, in-order) ===================== */
static uint8_t g_rx_buf[RX_BUF_LEN];
static uint8_t g_rx_len = 0;

/* 当前正在接收的一条消息状态 */
static bool    g_rx_busy = false;
static uint8_t g_rx_type = 0;
static uint8_t g_rx_seq  = 0;
static uint8_t g_rx_expect_len = 0;
static uint8_t g_rx_expect_crc = 0;
static bool    g_rx_got_crc = false;
static uint8_t g_rx_next_idx = 0;
static uint8_t g_rx_crc_calc = 0;

/* 外部过滤：WaitData(type=0xFF) 接任意类型；GetData 会限定 type+seq */
static uint8_t g_expect_type = 0xFF;
static bool    g_expect_seq_valid = false;
static uint8_t g_expect_seq = 0;

uint8_t GetDataLen(void) { return g_rx_len; }

static void rx_reset(void)
{
    g_rx_busy = false;
    g_rx_len = 0;
    g_rx_got_crc = false;
    g_rx_next_idx = 0;
    g_rx_crc_calc = 0;
}

/* 处理一帧：返回 true 表示“完成一条消息并可交付 g_rx_buf” */
static bool rx_consume_frame(const ParsedFrame *f)
{
    if (!f) return false;

    /* type 过滤 */
    if (g_expect_type != 0xFF && f->type != g_expect_type) return false;

    /* seq 过滤（GetData 使用） */
    if (g_expect_seq_valid && f->seq != (g_expect_seq & CTRL_SEQ_MASK)) return false;

    if (f->meta) {
        /* META_START: data = len */
        if (f->arg == META_START) {
            uint8_t len = f->data;
            if (len == 0 || len > RX_BUF_LEN) {
                rx_reset();
                return false;
            }

            g_rx_busy = true;
            g_rx_type = f->type;
            g_rx_seq  = f->seq;
            g_rx_expect_len = len;
            g_rx_got_crc = false;
            g_rx_next_idx = 0;
            g_rx_crc_calc = 0;
            g_rx_len = 0;
            return false;
        }

        /* META_CRC8: data = crc8 */
        if (f->arg == META_CRC8) {
            if (!g_rx_busy) return false;
            if (f->type != g_rx_type || f->seq != g_rx_seq) return false;

            g_rx_expect_crc = f->data;
            g_rx_got_crc = true;
            return false;
        }

        /* META_ACK：这里是对方回的 ACK，默认不在 RX 重组里处理 */
        if (f->arg == META_ACK) {
            return false;
        }

        /* META_GET / 其他 META：这里不处理 */
        return false;
    }

    /* DATA frame */
    if (!g_rx_busy) return false;
    if (f->type != g_rx_type || f->seq != g_rx_seq) return false;

    uint8_t idx = f->arg;

    /* 允许重复帧（因为发送端可能整包重复发送） */
    if (idx < g_rx_next_idx) {
        /* 若内容一致则忽略，否则当作噪声 */
        if (idx < RX_BUF_LEN && g_rx_buf[idx] == f->data) return false;
        rx_reset();
        return false;
    }

    /* 简化：要求按序 */
    if (idx != g_rx_next_idx) {
        rx_reset();
        return false;
    }

    if (idx >= g_rx_expect_len || idx >= RX_BUF_LEN) {
        rx_reset();
        return false;
    }

    /* FIRST 仅作为一致性检查（可选） */
    if (idx == 0 && !f->first) {
        /* 容忍：不强制 */
    }

    g_rx_buf[idx] = f->data;
    g_rx_crc_calc = crc8_update(g_rx_crc_calc, f->data);
    g_rx_next_idx++;

    if (f->last) {
        /* 必须收齐、且拿到 CRC 才交付 */
        if (g_rx_next_idx != g_rx_expect_len) {
            rx_reset();
            return false;
        }
        if (!g_rx_got_crc) {
            rx_reset();
            return false;
        }
        if (g_rx_crc_calc != g_rx_expect_crc) {
            rx_reset();
            return false;
        }

        /* ✅ 收到完整消息且 CRC 正确：立刻回 ACK（1字节） */
        SendAck((DataType)g_rx_type, g_rx_seq, ACK_OK);

        g_rx_len = g_rx_expect_len;
        g_rx_busy = false;
        return true;
    }

    return false;
}

/* ===================== Send / Get / Wait APIs ===================== */

/*
 * 发送整条消息，并等待对端 META_ACK（可选）
 * ack_timeout_ms = 0：不等 ACK（兼容旧行为）
 * 可靠性：META_START + META_CRC8 + seq + ACK +（超时重发整包）
 */
bool SendArray(DataType type, const uint8_t *data, uint16_t len, uint32_t ack_timeout_ms)
{
    if (!data || len == 0) return false;
    if (len > 255) return false;              /* 因为 META_START 的 len 只有 1 字节 */
    if (len > MAX_PAYLOAD_LEN) return false;  /* 仍沿用你的上限 */

    uint8_t seq = (uint8_t)(g_seq++ & CTRL_SEQ_MASK);
    uint8_t msg_crc = crc8_calc(data, len);

    /* 用 TX_REPEAT_COUNT 作为“整包重发次数” */
    for (uint8_t attempt = 0; attempt < TX_REPEAT_COUNT; attempt++) {

        /* 1) META_START (len) */
        SendFrame8((uint8_t)type, (uint8_t)(CTRL_META | seq), META_START, (uint8_t)len);

        /* 2) META_CRC8 */
        SendFrame8((uint8_t)type, (uint8_t)(CTRL_META | seq), META_CRC8, msg_crc);

        /* 3) DATA[0..len-1] */
        for (uint16_t i = 0; i < len; i++) {
            uint8_t ctrl = seq;
            if (i == 0)        ctrl |= CTRL_FIRST;
            if (i == (len-1))  ctrl |= CTRL_LAST;
            SendFrame8((uint8_t)type, ctrl, (uint8_t)i, data[i]);
        }

        /* 4) 等 ACK（可选） */
        if (ack_timeout_ms == 0) {
            return true; /* 不等 ACK：直接认为发送完成 */
        }

        if (WaitAck(type, seq, ack_timeout_ms)) {
            return true; /* ✅ 收到 ACK_OK */
        }

        /* 超时：进入下一轮 attempt，重发整包（同 seq） */
    }

    return false; /* 多次尝试仍未收到 ACK */
}

/* 发送 GET 请求（META GET），保持你原有定义 */
static inline void SendGet(DataType type, uint8_t seq)
{
    SendFrame8((uint8_t)type, (uint8_t)(CTRL_META | (seq & CTRL_SEQ_MASK)), META_GET, 0);
}

/* 被动等待（内部实现） */
static uint8_t *WaitDataEx(DataType type, uint32_t timeout_ms, bool lock_seq, uint8_t seq)
{
    uint32_t elapsed = 0;
    uint8_t pkt[8];
    ParsedFrame f;

    g_expect_type = (uint8_t)type;
    g_expect_seq_valid = lock_seq;
    g_expect_seq = (uint8_t)(seq & CTRL_SEQ_MASK);

    rx_reset();

    while (elapsed < timeout_ms) {
        if (ZigbeeRead8(pkt)) {
            if (ParseFrame(pkt, &f)) {
                if (rx_consume_frame(&f)) {
                    /* 完成一条消息 */
                    g_expect_type = 0xFF;
                    g_expect_seq_valid = false;
                    return g_rx_buf;
                }
            }
        }

        delay_ms(1);
        elapsed++;
    }

    g_expect_type = 0xFF;
    g_expect_seq_valid = false;
    return NULL;
}

/*
 * 主动请求：发 GET，然后等对方返回（限定 seq，避免读到别的响应）
 */
uint8_t *GetData(DataType type, uint32_t timeout_ms)
{
    uint8_t seq = (uint8_t)(g_seq++ & CTRL_SEQ_MASK);
    SendGet(type, seq);

    /* 等待响应消息（对方应使用同一 seq 回传） */
    return WaitDataEx(type, timeout_ms, true, seq);
}

/*
 * 被动等待：不主动发 GET，不锁 seq（收到任意 seq 的一条完整消息就返回）
 * type=0xFF 表示接收任意类型
 */
uint8_t *WaitData(DataType type, uint32_t timeout_ms)
{
    return WaitDataEx(type, timeout_ms, false, 0);
}
