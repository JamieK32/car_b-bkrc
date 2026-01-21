#include "vehicle_exchange_v2.h"


/* ===================== 可调参数（稳） ===================== */
#define ACK_TIMEOUT_MS       200    // 等ACK超时：建议 200~300
#define RETRY_GAP_MS          15    // 重试间隔：10~20
#define ACK_REPEAT_COUNT       3    // ACK重复发：2~3
#define ACK_REPEAT_GAP_MS      5

/* ====== 超时判断（支持回卷） ====== */
static inline bool time_reached(uint32_t start_ms, uint32_t timeout_ms)
{
    return (timeout_ms == 0) ? false : ((uint32_t)(get_ms() - start_ms) >= timeout_ms);
}

/* ====== 帧校验（head/tail/cs） ====== */
static inline uint8_t calc_cs_local(uint8_t type, uint8_t b0, uint8_t b1, uint8_t b2)
{
    return (uint8_t)((type + b0 + b1 + b2) & 0xFF);
}

static bool frame_valid(const uint8_t buf[8])
{
    if (buf[0] != ZIGBEE_HEAD0 || buf[1] != ZIGBEE_HEAD1) return false;
    if (buf[7] != ZIGBEE_TAIL) return false;
    return (buf[6] == calc_cs_local(buf[2], buf[3], buf[4], buf[5]));
}

/* ====== seq 生成：每次发送一个 item 递增 ====== */
static uint8_t g_tx_seq = 0;
static inline uint8_t next_seq(void) { return ++g_tx_seq; }

/* ====== 发送 ACK（少量重复即可） ====== */
static void send_ack(uint8_t item_id, uint8_t seq, uint8_t offset)
{
    uint8_t type = (uint8_t)(TYPE_ACK_BASE + (offset & 0x3F));
    for (int i = 0; i < ACK_REPEAT_COUNT; i++) {
        SendFrame8(type, item_id, seq, 0x00);
        delay_ms(ACK_REPEAT_GAP_MS);
    }
}

/* ====== 等待某个 offset 的 ACK（改进：有流量就延长等待，避免假超时） ====== */
static bool wait_ack(uint8_t item_id, uint8_t seq, uint8_t offset, uint32_t ack_timeout_ms)
{
    uint8_t buf[8];
    uint8_t expect_type = (uint8_t)(TYPE_ACK_BASE + (offset & 0x3F));

    uint32_t start = get_ms();
    uint32_t last_rx = start;

    while (true) {

        /* 没有任何数据流量超过 ack_timeout_ms 才算真正超时 */
        if ((uint32_t)(get_ms() - last_rx) >= ack_timeout_ms) {
            return false;
        }

        if (ZigbeeRead8(buf)) {
            last_rx = get_ms();  // 只要FIFO在动，就刷新 last_rx

            if (!frame_valid(buf)) continue;

            /* 只认本 offset 的 ACK */
            if (buf[2] != expect_type) continue;
            if (buf[3] != item_id) continue;
            if (buf[4] != seq) continue;

            return true;
        } else {
            delay_ms(1);
        }

        /* 防御：避免 get_ms 异常导致死循环（可选） */
        if ((uint32_t)(get_ms() - start) > (ack_timeout_ms + 2000U)) {
            return false;
        }
    }
}

/* ===================== 发送：逐字节停等ACK（最稳） ===================== */
bool Zigbee_SendItemReliable(uint8_t item_id,
                             const uint8_t *buf,
                             uint8_t len,
                             uint32_t timeout_ms)
{
    if (!buf || len == 0) return false;
    if (len > MAX_OFFSET) return false;

    uint8_t seq = next_seq();
    uint32_t overall_start = get_ms();

    /* 小随机退避，减少双方同时发时碰撞 */
    delay_ms((uint32_t)(seq & 0x07));

    for (uint8_t off = 0; off < len; off++) {
        uint8_t type = (uint8_t)(TYPE_DATA_BASE + (off & 0x3F));

        while (true) {
            SendFrame8(type, item_id, seq, buf[off]);

            if (wait_ack(item_id, seq, off, ACK_TIMEOUT_MS)) break;

            if (timeout_ms != 0 && time_reached(overall_start, timeout_ms)) return false;
            delay_ms(RETRY_GAP_MS);
        }
    }
    return true;
}

/* ===================== 接收：收满len才返回（bitmap 改成局部变量，更稳） ===================== */
bool Zigbee_WaitItemReliable(uint8_t *out_item_id,
                             uint8_t *out_buf,
                             uint8_t *inout_len,
                             uint32_t timeout_ms)
{
    if (!out_item_id || !out_buf || !inout_len) return false;
    uint8_t expect_len = *inout_len;
    if (expect_len == 0 || expect_len > MAX_OFFSET) return false;

    uint8_t buf[8];
    uint32_t start = get_ms();

    bool in_session = false;
    uint8_t cur_item = 0;
    uint8_t cur_seq  = 0;

    uint8_t got = 0;

    /* bitmap 局部变量：最多 64 bit */
    uint8_t bitmap[8];
    memset(bitmap, 0, sizeof(bitmap));

    memset(out_buf, 0, expect_len);

    while (!time_reached(start, timeout_ms)) {

        if (!ZigbeeRead8(buf)) {
            delay_ms(1);
            continue;
        }
        if (!frame_valid(buf)) continue;

        uint8_t type = buf[2];

        /* 只收 DATA */
        if (type < TYPE_DATA_BASE || type >= (TYPE_DATA_BASE + MAX_OFFSET)) {
            continue;
        }

        uint8_t off     = (uint8_t)(type - TYPE_DATA_BASE);
        uint8_t item_id = buf[3];
        uint8_t seq     = buf[4];
        uint8_t byte    = buf[5];

        /* offset 超出期望长度：也ACK一下让对方别卡住 */
        if (off >= expect_len) {
            send_ack(item_id, seq, off);
            continue;
        }

        if (!in_session) {
            /* 新会话 */
            in_session = true;
            cur_item = item_id;
            cur_seq  = seq;
            got = 0;

            memset(out_buf, 0, expect_len);
            memset(bitmap, 0, sizeof(bitmap));
        }

        /* 发现新 item/seq：切换会话（测试阶段这样最直观） */
        if (item_id != cur_item || seq != cur_seq) {
            cur_item = item_id;
            cur_seq  = seq;
            got = 0;

            memset(out_buf, 0, expect_len);
            memset(bitmap, 0, sizeof(bitmap));
        }

        /* bitmap 判断是否已收 */
        uint8_t bi = (uint8_t)(off >> 3);
        uint8_t bm = (uint8_t)(1u << (off & 7u));
        bool already = (bitmap[bi] & bm) != 0;

        if (!already) {
            out_buf[off] = byte;
            bitmap[bi] |= bm;
            got++;
        }

        /* 一定要 ACK（重复包也ACK） */
        send_ack(item_id, seq, off);

        if (got >= expect_len) {
            *out_item_id = cur_item;
            *inout_len = expect_len;
            return true;
        }
    }

    return false;
}
