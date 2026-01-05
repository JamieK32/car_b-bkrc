
#include "k210_app.hpp"
#include "qrcode_datamap.hpp"



// ------------------------------
// 外部：原始扫码结果（按slot）
// 约定：每个slot是一段以 '\0' 结尾的字符串（或全0表示空）
// ------------------------------
extern uint8_t g_qr_raw_slots[QRCODE_SLOTS][QRCODE_SLOT_SIZE];

// ------------------------------
// 本模块：归类后的数据（按类型）
// 约定：每种类型只保留“第一条/最新一条”（见 data_map() 里策略）
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

static inline QrCodeDataType bracket_char_to_type(uint8_t c)
{
    switch (c) {
        case '<': return kQrType_Bracket_LT;
        case '>': return kQrType_Bracket_GT;
        case '{': return kQrType_Bracket_LCURLY;
        case '}': return kQrType_Bracket_RCURLY;
        case '(': return kQrType_Bracket_LPAREN;
        case ')': return kQrType_Bracket_RPAREN;
        case '[': return kQrType_Bracket_LSQUARE;
        case ']': return kQrType_Bracket_RSQUARE;
        default:  return kQrType_None;
    }
}

static inline bool is_bracket_type(QrCodeDataType t)
{
    return (t >= kQrType_Bracket_LT) && (t <= kQrType_Bracket_RSQUARE);
}

static inline bool is_printable_data(uint8_t c)
{
    // 这里的“DATA”我按“有可见字符且不属于公式/括号优先”来判断
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

        // ✅ 括号优先：返回具体括号类型
        QrCodeDataType bt = bracket_char_to_type(c);
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
//   - 如果你想“总是覆盖成最新一条”，把 if 判断删掉即可
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

        // ✅ 如果是“具体括号类型”，再顺带写一份到“总括号类型”
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