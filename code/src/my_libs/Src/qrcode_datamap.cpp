#include "k210_app.hpp"
#include "qrcode_datamap.hpp"

// ------------------------------
// 外部：原始扫码结果（按slot）
// ------------------------------
extern uint8_t g_qr_raw_slots[QRCODE_SLOTS][QRCODE_SLOT_SIZE];

// ------------------------------
// 本模块：归类后的数据（按类型）
// ------------------------------
static uint8_t g_qr_typed_data[kQrType_Count][QRCODE_SLOT_SIZE];

// ------------------------------
// 小工具：安全计算字符串长度（最多到 max_len）
// ------------------------------
static uint16_t strnlen_u8(const uint8_t *s, uint16_t max_len)
{
    uint16_t i = 0;
    for (; i < max_len; i++) {
        if (s[i] == 0) break;
    }
    return i;
}

static inline bool is_formula_op(uint8_t c)
{
    return (c == '+') || (c == '-') || (c == '*') || (c == '/') || (c == '^');
}

// ✅ 左/右括号都映射到“左括号类型”
static inline QrCodeDataType bracket_char_to_left_type(uint8_t c)
{
    switch (c) {
        case '<':
        case '>': return kQrType_Bracket_LT;

        case '{':
        case '}': return kQrType_Bracket_LCURLY;

        case '(':
        case ')': return kQrType_Bracket_LPAREN;

        case '[':
        case ']': return kQrType_Bracket_LSQUARE;

        default:  return kQrType_None;
    }
}

static inline bool is_bracket_type(QrCodeDataType t)
{
    return (t >= kQrType_Bracket_LT) && (t <= kQrType_Bracket_LSQUARE);
}

static inline bool is_printable_data(uint8_t c)
{
    return (c >= 33 && c <= 126);
}

// ------------------------------
// 判别类型：括号优先，其次公式，否则普通数据/空
// ------------------------------
static QrCodeDataType judge_datatype(const uint8_t *data, uint16_t data_len)
{
    if (!data || data_len == 0) return kQrType_None;

    bool has_printable = false;

    for (uint16_t i = 0; i < data_len; i++) {
        uint8_t c = data[i];
        if (c == 0) break;

        // ✅ 括号优先：返回“左括号类型”
        QrCodeDataType bt = bracket_char_to_left_type(c);
        if (bt != kQrType_None) return bt;

        if (is_formula_op(c))  return kQrType_Formula;
        if (is_printable_data(c)) has_printable = true;
    }

    return has_printable ? kQrType_Data : kQrType_None;
}

// ------------------------------
// 归类映射：从 raw_slots -> typed_data
// 策略：每种类型只保存一条：
//   - 如果该类型还没被写入（typed_data[type][0]==0），就写入第一条
// ------------------------------
void qr_data_map(void)
{
    memset(g_qr_typed_data, 0, sizeof(g_qr_typed_data));

    for (uint8_t slot = 0; slot < QRCODE_SLOTS; slot++) {
        uint8_t *src = g_qr_raw_slots[slot];

        uint16_t len = strnlen_u8(src, QRCODE_SLOT_SIZE);
        if (len == 0) continue;

        QrCodeDataType type = judge_datatype(src, len);
        if (type >= kQrType_Count || type == kQrType_None) continue;

        // 计算安全拷贝长度并补 '\0'
        uint16_t n = len;
        if (n > (QRCODE_SLOT_SIZE - 1)) n = QRCODE_SLOT_SIZE - 1;

        // ✅ 先写“具体类型”（每类型只保留第一条）
        if (g_qr_typed_data[type][0] == 0) {
            memcpy(g_qr_typed_data[type], src, n);
            g_qr_typed_data[type][n] = 0;
        }

        // ✅ 如果是“括号类型”，再顺带写一份到“总括号类型”
        if (is_bracket_type(type)) {
            if (g_qr_typed_data[kQrType_Bracket_All][0] == 0) {
                memcpy(g_qr_typed_data[kQrType_Bracket_All], src, n);
                g_qr_typed_data[kQrType_Bracket_All][n] = 0;
            }
        }
    }
}

// 如果你需要提供给外部读取：
const uint8_t* qr_get_typed_data(QrCodeDataType type)
{
    if (type >= kQrType_Count) return NULL;
    return g_qr_typed_data[type];
}

uint16_t qr_get_typed_len(QrCodeDataType type)
{
    if (type >= kQrType_Count) return 0;

    const uint8_t *p = g_qr_typed_data[type];
    if (!p) return 0;

    return strnlen_u8(p, QRCODE_SLOT_SIZE);
}

/* ============================================================
 * 可配置组合括号特征（新增，不影响原 typed_data）
 * ============================================================ */

typedef struct {
    uint8_t open;
    uint8_t close;
} Pair;

typedef struct {
    Pair pairs[4];       // 最多 4 对括号，可按需要调大
    uint8_t pair_count;
} PairSeqPattern;

static QrSubRef g_custom_refs[QR_CUSTOM_MAX];
static QrCustomRule g_custom_rules[QR_CUSTOM_MAX];
static uint8_t g_custom_rule_count = 0;

static bool parse_pairseq_spec(const char *spec, PairSeqPattern *out)
{
    if (!spec || !out) return false;

    out->pair_count = 0;

    // 计算长度并校验必须偶数
    int len = 0;
    while (spec[len]) len++;
    if (len <= 0 || (len & 1) != 0) return false;

    for (int i = 0; i < len; i += 2) {
        if (out->pair_count >= 4) return false; // 超出支持对数
        out->pairs[out->pair_count++] = (Pair){ (uint8_t)spec[i], (uint8_t)spec[i+1] };
    }

    return out->pair_count > 0;
}

static bool find_pair_range(const uint8_t *s, uint8_t n,
                            uint8_t open, uint8_t close,
                            uint8_t pos,
                            uint8_t *out_open_i,
                            uint8_t *out_close_i)
{
    uint8_t i = pos;
    for (; i < n; i++) {
        if (s[i] == open) break;
    }
    if (i >= n) return false;

    uint8_t j = (uint8_t)(i + 1);
    for (; j < n; j++) {
        if (s[j] == close) break;
    }
    if (j >= n) return false;

    if (out_open_i)  *out_open_i  = i;
    if (out_close_i) *out_close_i = j;
    return true;
}

// 匹配多对括号序列：依次找到 open..close，下一对从上一个 close 后继续
static bool match_pairseq(const uint8_t *s, uint8_t n,
                          const PairSeqPattern *pat,
                          uint8_t *out_start, uint8_t *out_end)
{
    if (!s || !pat || pat->pair_count == 0) return false;

    uint8_t pos = 0;
    uint8_t first_start = 0;
    uint8_t last_end = 0;
    bool has_any = false;

    for (uint8_t k = 0; k < pat->pair_count; k++) {
        uint8_t a=0,b=0;
        if (!find_pair_range(s, n, pat->pairs[k].open, pat->pairs[k].close, pos, &a, &b)) {
            return false;
        }
        if (!has_any) { first_start = a; has_any = true; }
        last_end = b;
        pos = (uint8_t)(b + 1);
    }

    if (out_start) *out_start = first_start;
    if (out_end)   *out_end   = last_end;
    return true;
}

void qr_custom_map(const QrCustomRule *rules, uint8_t rule_count)
{
    if (!rules) rule_count = 0;
    if (rule_count > QR_CUSTOM_MAX) rule_count = QR_CUSTOM_MAX;

    g_custom_rule_count = rule_count;

    for (uint8_t i = 0; i < rule_count; i++) {
        g_custom_rules[i] = rules[i];
        g_custom_refs[i] = (QrSubRef){ .slot = 0xFF, .start = 0, .len = 0 };
    }

    for (uint8_t ri = 0; ri < rule_count; ri++) {
        PairSeqPattern pat;
        if (!parse_pairseq_spec(g_custom_rules[ri].spec, &pat)) continue;

        for (uint8_t slot = 0; slot < QRCODE_SLOTS; slot++) {
            const uint8_t *src = g_qr_raw_slots[slot];
            if (!src || src[0] == 0) continue;

            uint8_t n = (uint8_t)strnlen_u8(src, QRCODE_SLOT_SIZE);
            if (n == 0) continue;

            uint8_t st=0, ed=0;
            if (match_pairseq(src, n, &pat, &st, &ed)) {
                g_custom_refs[ri].slot  = slot;
                g_custom_refs[ri].start = st;
                g_custom_refs[ri].len   = (uint8_t)(ed - st + 1);
                break; // ✅ 每条 spec 只要第一条命中
            }
        }
    }
}

const uint8_t* qr_custom_get(uint8_t rule_index, uint16_t *out_len)
{
    if (rule_index >= g_custom_rule_count) return NULL;

    QrSubRef r = g_custom_refs[rule_index];
    if (r.slot == 0xFF) return NULL;

    if (out_len) *out_len = r.len;
    return &g_qr_raw_slots[r.slot][r.start]; // 注意：不是 \0 结尾子串，需用 len
}
