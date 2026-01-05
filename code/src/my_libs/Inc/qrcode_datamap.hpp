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

    // ✅ 每个括号单独一个类型
    kQrType_Bracket_LT,      // '<'
    kQrType_Bracket_GT,      // '>'
    kQrType_Bracket_LCURLY,  // '{'
    kQrType_Bracket_RCURLY,  // '}'
    kQrType_Bracket_LPAREN,  // '('
    kQrType_Bracket_RPAREN,  // ')'
    kQrType_Bracket_LSQUARE, // '['
    kQrType_Bracket_RSQUARE, // ']'

    kQrType_Formula,
    kQrType_Data,

    kQrType_Count,
} QrCodeDataType;

void qr_data_map(void);
const uint8_t* qr_get_typed_data(QrCodeDataType type);
uint16_t qr_get_typed_len(QrCodeDataType type);

#ifdef __cplusplus
}
#endif

#endif
