#ifndef ALGORITGM
#define ALGORITGM

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char *name;
  double value;
} VarBinding;

typedef int (*algo_cmp_fn)(const void* a, const void* b, void* user);

// 计算表达式（+ - * / ()），成功返回 0，失败返回错误码
int algo_calc_eval(const char *expr, double *out_value);

// 提取第一对匹配括号里的内容（支持嵌套）
// 成功返回 malloc 的字符串，调用者需要 free；失败返回 NULL
char* algo_extract_bracket(const char* s, char open);
char* algo_extract_bracket_n(const char* s, char open, size_t n);

void algo_get_A_B(const char *str, uint8_t str_size, uint16_t* A, uint16_t *B);
int algo_str_to_hex(const char *str, uint8_t str_size,
                    uint8_t *out, uint8_t out_max);

int* digits_to_intv(const char *s);

int bind_expr(char *dst, size_t dst_sz,
                     const char *tmpl,
                     const VarBinding *vars, size_t var_cnt);



/* uint8_t 升序比较 */
static int cmp_u8_asc(const void *pa, const void *pb)
{
    uint8_t a = *(const uint8_t *)pa;
    uint8_t b = *(const uint8_t *)pb;
    return (a > b) - (a < b);
}

/* uint8_t 降序比较 */
static int cmp_u8_desc(const void *pa, const void *pb)
{
    uint8_t a = *(const uint8_t *)pa;
    uint8_t b = *(const uint8_t *)pb;
    return (b > a) - (b < a);
}

/* 指针 + 长度：最通用 */
static inline void sort_u8_asc(uint8_t *buf, size_t n)
{
    qsort(buf, n, sizeof(buf[0]), cmp_u8_asc);
}

static inline void sort_u8_desc(uint8_t *buf, size_t n)
{
    qsort(buf, n, sizeof(buf[0]), cmp_u8_desc);
}

/* 真数组：更省心（只能用于“真正的数组”，不能是指针） */
#define SORT_U8_ASC(arr)  qsort((arr), sizeof(arr)/sizeof((arr)[0]), sizeof((arr)[0]), cmp_u8_asc)
#define SORT_U8_DESC(arr) qsort((arr), sizeof(arr)/sizeof((arr)[0]), sizeof((arr)[0]), cmp_u8_desc)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ALGORITGM
