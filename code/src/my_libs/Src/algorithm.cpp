// algorithm.cpp (C-style implementation, C/C++ compatible)
#include <ctype.h>
#include <stdlib.h>
#include "string.h"
#include "algorithm.hpp"
#include "stdio.h"
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif

enum algo_calc_err {
    ALGO_CALC_OK = 0,
    ALGO_CALC_ERR_SYNTAX = 1,
    ALGO_CALC_ERR_DIV0   = 2
};

static void skip_ws(const char **p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

static int precedence(char op) {
    switch (op) {
        case '+': case '-': return 1;
        case '*': case '/': case '%': return 2;
        case '~': return 3;          // unary minus
        case '^': return 4;          // exponent
        default:  return 0;
    }
}
static int is_right_assoc(char op) { return (op == '^' || op == '~'); }

static int should_pop(char top, char cur) {
    if (top == '(') return 0;
    int pt = precedence(top), pc = precedence(cur);
    return is_right_assoc(cur) ? (pt > pc) : (pt >= pc);
}

static int apply_one(double *nums, int *ntop, char op) {
    if (op == '~') {
        if (*ntop < 0) return ALGO_CALC_ERR_SYNTAX;
        nums[*ntop] = -nums[*ntop];
        return ALGO_CALC_OK;
    }

    if (*ntop < 1) return ALGO_CALC_ERR_SYNTAX;
    double b = nums[(*ntop)--];
    double a = nums[(*ntop)--];
    double r = 0.0;

    switch (op) {
        case '+': r = a + b; break;
        case '-': r = a - b; break;
        case '*': r = a * b; break;
        case '/':
            if (b == 0.0) return ALGO_CALC_ERR_DIV0;
            r = a / b;
            break;
        case '%':
            if (b == 0.0) return ALGO_CALC_ERR_DIV0;
            r = fmod(a, b);
            break;
        case '^':
            r = pow(a, b);
            break;
        default:
            return ALGO_CALC_ERR_SYNTAX;
    }

    nums[++(*ntop)] = r;
    return ALGO_CALC_OK;
}

int algo_calc_eval(const char *expr, double *out_value) {
    if (!expr || !out_value) return ALGO_CALC_ERR_SYNTAX;

    size_t n = strlen(expr);
    // 栈容量：按字符长度给足（很保守）
    size_t cap = n + 8;

    double *nums = (double*)malloc(sizeof(double) * cap);
    char   *ops  = (char*)malloc(sizeof(char) * cap);
    if (!nums || !ops) {
        free(nums); free(ops);
        return ALGO_CALC_ERR_SYNTAX; // 也可以定义一个 ALGO_CALC_ERR_OOM
    }

    int ntop = -1, otop = -1;
    const char *p = expr;
    int expect_value = 1;

    while (1) {
        skip_ws(&p);
        char c = *p;
        if (c == '\0') break;

        // number
        if (isdigit((unsigned char)c) || c == '.') {
            char *end = NULL;
            double v = strtod(p, &end);
            if (end == p) { p++; continue; }
            if ((size_t)(ntop + 1) >= cap) { free(nums); free(ops); return ALGO_CALC_ERR_SYNTAX; }
            nums[++ntop] = v;
            p = end;
            expect_value = 0;
            continue;
        }

        // '('
        if (c == '(') {
            if ((size_t)(otop + 1) >= cap) { free(nums); free(ops); return ALGO_CALC_ERR_SYNTAX; }
            ops[++otop] = '(';
            p++;
            expect_value = 1;
            continue;
        }

        // ')'
        if (c == ')') {
            if (expect_value) { free(nums); free(ops); return ALGO_CALC_ERR_SYNTAX; }
            while (otop >= 0 && ops[otop] != '(') {
                int e = apply_one(nums, &ntop, ops[otop--]);
                if (e) { free(nums); free(ops); return e; }
            }
            if (otop < 0 || ops[otop] != '(') { free(nums); free(ops); return ALGO_CALC_ERR_SYNTAX; }
            otop--; // pop '('
            p++;
            expect_value = 0;
            continue;
        }

        // operators
        if (c=='+' || c=='-' || c=='*' || c=='/' || c=='%' || c=='^') {
            if (expect_value) {
                if (c == '+') { p++; continue; }
                if (c == '-') {
                    // unary minus：直接压 '~'，不触发 pop（修复 2^-3 ）
                    if ((size_t)(otop + 1) >= cap) { free(nums); free(ops); return ALGO_CALC_ERR_SYNTAX; }
                    ops[++otop] = '~';
                    p++;
                    continue;
                }
                free(nums); free(ops);
                return ALGO_CALC_ERR_SYNTAX;
            } else {
                char cur = c;
                while (otop >= 0 && should_pop(ops[otop], cur)) {
                    int e = apply_one(nums, &ntop, ops[otop--]);
                    if (e) { free(nums); free(ops); return e; }
                }
                if ((size_t)(otop + 1) >= cap) { free(nums); free(ops); return ALGO_CALC_ERR_SYNTAX; }
                ops[++otop] = cur;
                p++;
                expect_value = 1;
                continue;
            }
        }

        // 非法字符跳过：标识符整段跳过（保留你原功能）
        if (isalpha((unsigned char)c) || c == '_') {
            p++;
            while (*p && (isalnum((unsigned char)*p) || *p == '_')) p++;
            continue;
        }

        // 其他非法字符跳过 1 个（保留你原功能）
        p++;
    }

    if (expect_value) { free(nums); free(ops); return ALGO_CALC_ERR_SYNTAX; }

    while (otop >= 0) {
        char op = ops[otop--];
        if (op == '(') { free(nums); free(ops); return ALGO_CALC_ERR_SYNTAX; }
        int e = apply_one(nums, &ntop, op);
        if (e) { free(nums); free(ops); return e; }
    }

    if (ntop != 0) { free(nums); free(ops); return ALGO_CALC_ERR_SYNTAX; }

    *out_value = nums[0];
    free(nums); free(ops);
    return ALGO_CALC_OK;
}






/* 
使用方式
double n = get_n();
double x = get_x();
double y = get_y();
VarBinding vars[] = {
  {"n", n},
  {"x", x},
  {"y", y},
};
char expr2[128];
if (bind_expr(expr2, sizeof(expr2), "n+2*x+y", vars, 3) == 0) {
  double out;
  algo_calc_eval(expr2, &out);
}
*/

int bind_expr(char *dst, size_t dst_sz,
                     const char *tmpl,
                     const VarBinding *vars, size_t var_cnt)
{
  size_t di = 0;
  for (size_t i = 0; tmpl[i] != '\0'; ) {
    // 尝试匹配变量名
    int matched = 0;
    for (size_t v = 0; v < var_cnt; v++) {
      size_t len = strlen(vars[v].name);
      if (strncmp(&tmpl[i], vars[v].name, len) == 0) {
        // 防止把 "nn" 误匹配成 "n"：检查边界（可按需增强）
        long iv = (long)lround(vars[v].value); // 需要 math.h
        int n = snprintf(dst + di, dst_sz - di, "%ld", iv);
        if (n < 0 || (size_t)n >= dst_sz - di) return -1;
        di += (size_t)n;
        i += len;
        matched = 1;
        break;
      }
    }
    if (matched) continue;

    // 普通字符拷贝
    if (di + 1 >= dst_sz) return -1;
    dst[di++] = tmpl[i++];
  }
  dst[di] = '\0';
  return 0;
}





static char algo_match_bracket(char open) {
    switch (open) {
        case '(': return ')';
        case '[': return ']';
        case '{': return '}';
        case '<': return '>';
        default:  return 0;
    }
}

// 提取第一对匹配括号里的内容（支持嵌套）
// 成功返回 malloc 的字符串，调用者需要 free；失败返回 NULL
char* algo_extract_bracket(const char* s, char open) {
    if (!s) return NULL;

    char close = algo_match_bracket(open);
    if (!close) return NULL;

    // 1) 找到第一个 open
    const char* p = s;
    while (*p && *p != open) p++;
    if (*p != open) return NULL;

    // 2) 从 open 后开始扫描，支持嵌套
    const char* start = p + 1; // 内容起点（不含 open）
    int depth = 1;
    p = start;

    while (*p) {
        if (*p == open) {
            depth++;
        } else if (*p == close) {
            depth--;
            if (depth == 0) {
                // 找到了匹配 close，内容为 [start, p)
                size_t len = (size_t)(p - start);
                char* out = (char*)malloc(len + 1);
                if (!out) return NULL;
                if (len) memcpy(out, start, len);
                out[len] = '\0';
                return out;
            }
        }
        p++;
    }

    // 没找到匹配 close
    return NULL;
}

// 提取第 n 个括号对里的内容（n 从 1 开始）
// 返回 malloc 的字符串，调用者 free；失败返回 NULL
char* algo_extract_bracket_n(const char* s, char open, size_t n) {
    if (!s || n == 0) return NULL;

    char close = algo_match_bracket(open);
    if (!close) return NULL;

    size_t idx = 0;        // 已遇到的“顶层”括号对数量
    int depth = 0;         // 当前嵌套深度
    const char* start = NULL;

    for (const char* p = s; *p; ++p) {
        if (*p == open) {
            if (depth == 0) {          // 新的顶层括号对开始
                idx++;
                if (idx == n) start = p + 1; // 记录内容起点（不含 open）
            }
            depth++;
        } else if (*p == close) {
            if (depth == 0) {
                // 遇到多余的 close，忽略或视为格式错误都行；这里选择忽略
                continue;
            }
            depth--;
            if (depth == 0 && idx == n && start) {
                // 第 n 个顶层括号对闭合，提取 [start, p)
                size_t len = (size_t)(p - start);
                char* out = (char*)malloc(len + 1);
                if (!out) return NULL;
                if (len) memcpy(out, start, len);
                out[len] = '\0';
                return out;
            }
        }
    }

    // 没找到第 n 个完整括号对
    return NULL;
}


int* digits_to_intv(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    if (n == 0) return NULL;

    int *a = (int*)malloc(n * sizeof(int));
    if (!a) return NULL;

    for (size_t i = 0; i < n; i++) {
        a[i] = (s[i] >= '0' && s[i] <= '9') ? (s[i] - '0') : 0; // 简单粗暴
    }
    return a;
}



#ifdef __cplusplus
} // extern "C"
#endif