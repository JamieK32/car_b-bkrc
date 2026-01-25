#ifndef QRCODE_DATA_MAP_H__
#define QRCODE_DATA_MAP_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    kQrType_None = 0,

    // ✅ 总括号类型（任意括号都归到这里一份）
    kQrType_Bracket_All,

    // ✅ 只保留“左括号类型”
    //    注意：右括号也会映射到对应的左括号类型
    //    '>' -> kQrType_Bracket_LT
    //    '}' -> kQrType_Bracket_LCURLY
    //    ')' -> kQrType_Bracket_LPAREN
    //    ']' -> kQrType_Bracket_LSQUARE
    kQrType_Bracket_LT,      // '<' or '>'
    kQrType_Bracket_LCURLY,  // '{' or '}'
    kQrType_Bracket_LPAREN,  // '(' or ')'
    kQrType_Bracket_LSQUARE, // '[' or ']'

    kQrType_Formula,
    kQrType_Data,

    kQrType_Count,
} QrCodeDataType;

void qr_data_map(void);
const uint8_t* qr_get_typed_data(QrCodeDataType type);
uint16_t qr_get_typed_len(QrCodeDataType type);

/* ============================================================
 * 可配置组合括号特征（新增）
 * spec 语法：由括号对序列组成，每两个字符表示一对括号：
 *   "()[]", "{}()", "[]{}", "<>{}" 等
 * 匹配逻辑：按顺序依次找到 open...close；下一对从上一次 close 之后继续找
 * 每条 spec 只保留“第一条命中”
 * 返回的是子串引用：ptr 指向 raw_slots 内部，需要配合 len 使用
 * ============================================================ */

#ifndef QR_CUSTOM_MAX
#define QR_CUSTOM_MAX 8  // 最多支持 8 条自定义组合特征
#endif

typedef struct {
    const char *spec;        // 例如 "()[]"、"{}()"
} QrCustomRule;

typedef struct {
    uint8_t slot;            // 0..QRCODE_SLOTS-1, 0xFF 表示未命中
    uint8_t start;           // 子串起点
    uint8_t len;             // 子串长度
} QrSubRef;

void qr_custom_map(const QrCustomRule *rules, uint8_t rule_count);
const uint8_t* qr_custom_get(uint8_t rule_index, uint16_t *out_len);

#ifdef __cplusplus
}
#endif

#endif // QRCODE_DATA_MAP_H__
